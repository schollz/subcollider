/**
 * @file BufferAllocator.h
 * @brief Memory pool allocator for audio buffers.
 *
 * BufferAllocator manages a pre-allocated memory pool for audio sample storage.
 * It provides efficient allocation and deallocation of Buffer objects without
 * runtime heap allocation after initialization.
 */

#ifndef SUBCOLLIDER_BUFFER_ALLOCATOR_H
#define SUBCOLLIDER_BUFFER_ALLOCATOR_H

#include "types.h"
#include "Buffer.h"
#include <cstddef>

namespace subcollider {

/**
 * @brief Memory pool allocator for audio buffers.
 *
 * This class manages a contiguous block of memory that can be used to allocate
 * Buffer objects. It pre-allocates a large array of samples (default 5 minutes
 * at 48kHz stereo) at initialization time, and provides methods to allocate
 * and release portions of this memory for use as audio buffers.
 *
 * The allocator uses a simple first-fit free list algorithm for allocation.
 * Released buffers are merged with adjacent free blocks when possible.
 *
 * Example usage:
 * @code
 *     BufferAllocator<> allocator;  // Uses default 5 minute pool
 *     allocator.init(48000.0f);
 *
 *     // Allocate a mono buffer of 1000 samples
 *     Buffer buf = allocator.allocate(1000, 1);
 *     if (buf.isValid()) {
 *         // Fill with audio data...
 *         buf.data[0] = 0.5f;
 *     }
 *
 *     // Release when done
 *     allocator.release(buf);
 * @endcode
 *
 * @tparam PoolSamples Total number of float samples in the memory pool.
 *                     Default is 5 minutes at 48kHz stereo = 28,800,000 floats.
 * @tparam MaxBlocks Maximum number of allocation blocks to track.
 *                   Default is 256 blocks.
 */
template<size_t PoolSamples = 5 * 60 * 48000 * 2, size_t MaxBlocks = 256>
class BufferAllocator {
public:
    /// Default constructor - pool is zero-initialized
    BufferAllocator() noexcept
        : sampleRate_(DEFAULT_SAMPLE_RATE)
        , blockCount_(0)
        , initialized_(false) {
    }

    /**
     * @brief Initialize the allocator with a sample rate.
     * @param sr Sample rate in Hz for allocated buffers
     *
     * This clears the pool and sets up the initial free block.
     * Must be called before allocate().
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE) noexcept {
        sampleRate_ = sr;
        blockCount_ = 1;
        blocks_[0] = Block{0, PoolSamples, false};
        initialized_ = true;
        
        // Zero-initialize the pool
        for (size_t i = 0; i < PoolSamples; ++i) {
            pool_[i] = 0.0f;
        }
    }

    /**
     * @brief Allocate a buffer from the pool.
     * @param numSamples Number of samples (per channel for stereo)
     * @param channels Number of channels (1 for mono, 2 for stereo)
     * @return Buffer object pointing to allocated memory, or invalid Buffer if allocation fails
     *
     * For stereo buffers, the actual float count is numSamples * 2 (interleaved).
     * Returns an invalid Buffer (data == nullptr) if allocation fails.
     */
    Buffer allocate(size_t numSamples, uint8_t channels = 1) noexcept {
        if (!initialized_ || numSamples == 0 || (channels != 1 && channels != 2)) {
            return Buffer();
        }

        const size_t floatsNeeded = numSamples * channels;

        // First-fit search for a free block
        for (size_t i = 0; i < blockCount_; ++i) {
            if (!blocks_[i].used && blocks_[i].size >= floatsNeeded) {
                // Found a suitable block
                const size_t offset = blocks_[i].offset;
                const size_t remaining = blocks_[i].size - floatsNeeded;

                if (remaining > 0 && blockCount_ < MaxBlocks) {
                    // Split the block
                    blocks_[i].size = floatsNeeded;
                    blocks_[i].used = true;

                    // Insert new free block after current
                    for (size_t j = blockCount_; j > i + 1; --j) {
                        blocks_[j] = blocks_[j - 1];
                    }
                    blocks_[i + 1] = Block{offset + floatsNeeded, remaining, false};
                    ++blockCount_;
                } else {
                    // Use the entire block
                    blocks_[i].used = true;
                }

                return Buffer(&pool_[offset], channels, sampleRate_, numSamples);
            }
        }

        // No suitable block found
        return Buffer();
    }

    /**
     * @brief Release a previously allocated buffer.
     * @param buf Buffer to release
     * @return true if buffer was successfully released, false otherwise
     *
     * After release, the buffer's memory becomes available for new allocations.
     * Adjacent free blocks are merged to reduce fragmentation.
     * The Buffer object should not be used after release.
     */
    bool release(const Buffer& buf) noexcept {
        if (!initialized_ || buf.data == nullptr) {
            return false;
        }

        // Find the block corresponding to this buffer
        // Note: pool_ + PoolSamples is a valid one-past-the-end pointer for bounds checking
        const Sample* start = buf.data;
        if (start < pool_ || start >= pool_ + PoolSamples) {
            return false;  // Not from this pool
        }

        // Safe cast: we've verified start is within pool_ bounds
        const size_t offset = static_cast<size_t>(start - pool_);

        for (size_t i = 0; i < blockCount_; ++i) {
            if (blocks_[i].offset == offset && blocks_[i].used) {
                blocks_[i].used = false;
                mergeAdjacentFreeBlocks();
                return true;
            }
        }

        return false;  // Block not found
    }

    /**
     * @brief Fill a buffer with mono sample data.
     * @param buf Buffer to fill (must be mono)
     * @param data Source data array
     * @param count Number of samples to copy
     * @return true if successful, false if buffer is invalid or wrong format
     *
     * Copies up to 'count' samples from 'data' into the buffer.
     * If count exceeds buffer's numSamples, only numSamples are copied.
     */
    static bool fillMono(Buffer& buf, const Sample* data, size_t count) noexcept {
        if (!buf.isValid() || data == nullptr || buf.channels != 1) {
            return false;
        }

        const size_t toCopy = count < buf.numSamples ? count : buf.numSamples;
        for (size_t i = 0; i < toCopy; ++i) {
            buf.data[i] = data[i];
        }
        return true;
    }

    /**
     * @brief Fill a buffer with stereo sample data.
     * @param buf Buffer to fill (must be stereo)
     * @param left Left channel source data
     * @param right Right channel source data
     * @param count Number of sample frames to copy
     * @return true if successful, false if buffer is invalid or wrong format
     *
     * Interleaves left and right channel data into the buffer.
     * If count exceeds buffer's numSamples, only numSamples frames are copied.
     */
    static bool fillStereo(Buffer& buf, const Sample* left, const Sample* right, size_t count) noexcept {
        if (!buf.isValid() || left == nullptr || right == nullptr || buf.channels != 2) {
            return false;
        }

        const size_t toCopy = count < buf.numSamples ? count : buf.numSamples;
        for (size_t i = 0; i < toCopy; ++i) {
            buf.data[i * 2] = left[i];
            buf.data[i * 2 + 1] = right[i];
        }
        return true;
    }

    /**
     * @brief Fill a stereo buffer with interleaved sample data.
     * @param buf Buffer to fill (must be stereo)
     * @param interleaved Source data in interleaved format (L0, R0, L1, R1, ...)
     * @param count Number of sample frames to copy
     * @return true if successful, false if buffer is invalid or wrong format
     */
    static bool fillStereoInterleaved(Buffer& buf, const Sample* interleaved, size_t count) noexcept {
        if (!buf.isValid() || interleaved == nullptr || buf.channels != 2) {
            return false;
        }

        const size_t toCopy = count < buf.numSamples ? count : buf.numSamples;
        const size_t floats = toCopy * 2;
        for (size_t i = 0; i < floats; ++i) {
            buf.data[i] = interleaved[i];
        }
        return true;
    }

    /**
     * @brief Get the total size of the memory pool in samples.
     * @return Total number of float samples in the pool
     */
    static constexpr size_t poolSize() noexcept {
        return PoolSamples;
    }

    /**
     * @brief Get the number of currently allocated blocks.
     * @return Number of blocks (both used and free)
     */
    size_t blockCount() const noexcept {
        return blockCount_;
    }

    /**
     * @brief Get the amount of free memory in the pool.
     * @return Number of unallocated float samples
     */
    size_t freeSpace() const noexcept {
        size_t free = 0;
        for (size_t i = 0; i < blockCount_; ++i) {
            if (!blocks_[i].used) {
                free += blocks_[i].size;
            }
        }
        return free;
    }

    /**
     * @brief Get the amount of used memory in the pool.
     * @return Number of allocated float samples
     */
    size_t usedSpace() const noexcept {
        return PoolSamples - freeSpace();
    }

    /**
     * @brief Check if the allocator has been initialized.
     * @return true if init() has been called
     */
    bool isInitialized() const noexcept {
        return initialized_;
    }

    /**
     * @brief Reset the allocator, releasing all allocations.
     *
     * After reset, all previously allocated buffers become invalid.
     * The pool is zeroed and a single free block is created.
     */
    void reset() noexcept {
        if (initialized_) {
            init(sampleRate_);
        }
    }

    /**
     * @brief Get the sample rate used for allocated buffers.
     * @return Sample rate in Hz
     */
    Sample sampleRate() const noexcept {
        return sampleRate_;
    }

private:
    /// Block metadata for tracking allocations
    struct Block {
        size_t offset;  ///< Offset into pool_ array
        size_t size;    ///< Size in floats
        bool used;      ///< Whether block is allocated
    };

    /// Merge adjacent free blocks to reduce fragmentation
    void mergeAdjacentFreeBlocks() noexcept {
        if (blockCount_ <= 1) {
            return;  // Nothing to merge
        }
        size_t i = 0;
        while (i < blockCount_ - 1) {
            if (!blocks_[i].used && !blocks_[i + 1].used) {
                // Merge blocks i and i+1
                blocks_[i].size += blocks_[i + 1].size;
                
                // Remove block i+1
                for (size_t j = i + 1; j < blockCount_ - 1; ++j) {
                    blocks_[j] = blocks_[j + 1];
                }
                --blockCount_;
                // Don't increment i, check for more merges at same position
            } else {
                ++i;
            }
        }
    }

    Sample pool_[PoolSamples];   ///< Pre-allocated sample memory
    Block blocks_[MaxBlocks];     ///< Block metadata
    Sample sampleRate_;           ///< Sample rate for allocated buffers
    size_t blockCount_;           ///< Number of blocks in use
    bool initialized_;            ///< Whether init() has been called
};

} // namespace subcollider

#endif // SUBCOLLIDER_BUFFER_ALLOCATOR_H
