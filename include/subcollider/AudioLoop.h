/**
 * @file AudioLoop.h
 * @brief ISR-safe audio processing loop.
 *
 * AudioLoop provides a block-based audio processing framework
 * designed for interrupt-safe operation on embedded systems.
 * No heap allocation occurs during processing.
 */

#ifndef SUBCOLLIDER_AUDIO_LOOP_H
#define SUBCOLLIDER_AUDIO_LOOP_H

#include "types.h"
#include "AudioBuffer.h"

namespace subcollider {

/**
 * @brief ISR-safe audio processing loop.
 *
 * @tparam BlockSize Size of audio blocks in samples
 * @tparam VoiceType Type of the voice/synth to process
 *
 * This class manages block-based audio processing in an ISR-safe manner.
 * It maintains a double buffer for output and processes audio in fixed-size
 * blocks to minimize cache misses and optimize for embedded targets.
 *
 * Usage:
 * @code
 * struct MyVoice {
 *     void init(float sampleRate) { ... }
 *     void process(float* buffer, size_t numSamples) { ... }
 * };
 *
 * AudioLoop<64, MyVoice> loop;
 * loop.init(48000.0f);
 *
 * // In audio interrupt or callback:
 * loop.processBlock();
 * const float* output = loop.getOutputBuffer();
 * @endcode
 */
template<size_t BlockSize = DEFAULT_BLOCK_SIZE, typename VoiceType = void>
struct AudioLoop {
    /// Output buffer A
    AudioBuffer<BlockSize> bufferA;

    /// Output buffer B
    AudioBuffer<BlockSize> bufferB;

    /// Current buffer index (0 = A, 1 = B)
    volatile uint8_t currentBuffer;

    /// Sample rate in Hz
    Sample sampleRate;

    /// Block size in samples
    static constexpr size_t blockSize = BlockSize;

    /**
     * @brief Initialize the audio loop.
     * @param sr Sample rate in Hz
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE) noexcept {
        sampleRate = sr;
        currentBuffer = 0;
        bufferA.clear();
        bufferB.clear();
    }

    /**
     * @brief Get pointer to the current output buffer (ISR-safe read).
     * @return Pointer to sample data ready for output
     */
    const Sample* getOutputBuffer() const noexcept {
        return currentBuffer == 0 ? bufferB.data : bufferA.data;
    }

    /**
     * @brief Get pointer to the processing buffer.
     * @return Pointer to sample data for writing
     */
    Sample* getProcessingBuffer() noexcept {
        return currentBuffer == 0 ? bufferA.data : bufferB.data;
    }

    /**
     * @brief Clear the processing buffer.
     */
    void clearProcessingBuffer() noexcept {
        if (currentBuffer == 0) {
            bufferA.clear();
        } else {
            bufferB.clear();
        }
    }

    /**
     * @brief Swap buffers after processing (ISR-safe).
     *
     * Call this after processing a block to make the new
     * samples available for output.
     */
    void swapBuffers() noexcept {
        currentBuffer = currentBuffer == 0 ? 1 : 0;
    }

    /**
     * @brief Get the block size.
     * @return Number of samples per block
     */
    static constexpr size_t getBlockSize() noexcept {
        return BlockSize;
    }
};

/**
 * @brief Simple audio callback handler for non-templated use.
 *
 * This struct provides a simple interface for handling audio
 * callbacks without templates. Useful for integration with
 * C-style audio APIs.
 */
struct AudioCallbackHandler {
    /// Function pointer type for audio processing callback
    using ProcessCallback = void (*)(Sample* buffer, size_t numSamples, void* userData);

    /// Audio processing callback
    ProcessCallback callback;

    /// User data passed to callback
    void* userData;

    /// Sample rate in Hz
    Sample sampleRate;

    /**
     * @brief Initialize the callback handler.
     * @param sr Sample rate in Hz
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE) noexcept {
        sampleRate = sr;
        callback = nullptr;
        userData = nullptr;
    }

    /**
     * @brief Set the processing callback.
     * @param cb Callback function
     * @param data User data pointer
     */
    void setCallback(ProcessCallback cb, void* data = nullptr) noexcept {
        callback = cb;
        userData = data;
    }

    /**
     * @brief Process a block of audio.
     * @param buffer Output buffer
     * @param numSamples Number of samples to generate
     */
    void process(Sample* buffer, size_t numSamples) noexcept {
        if (callback) {
            callback(buffer, numSamples, userData);
        }
    }
};

} // namespace subcollider

#endif // SUBCOLLIDER_AUDIO_LOOP_H
