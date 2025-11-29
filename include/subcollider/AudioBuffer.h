/**
 * @file AudioBuffer.h
 * @brief Fixed-size audio buffer for block-based processing.
 *
 * AudioBuffer provides a statically-sized buffer for audio samples,
 * suitable for embedded use with no heap allocation.
 */

#ifndef SUBCOLLIDER_AUDIO_BUFFER_H
#define SUBCOLLIDER_AUDIO_BUFFER_H

#include "types.h"
#include <cstring>

namespace subcollider {

/**
 * @brief Fixed-size audio buffer for block-based processing.
 * @tparam N Maximum buffer size in samples
 *
 * This buffer is stack-allocated and suitable for ISR-safe audio processing.
 * No heap allocation occurs.
 */
template<size_t N = DEFAULT_BLOCK_SIZE>
struct AudioBuffer {
    /// Audio sample data
    Sample data[N];

    /// Current number of valid samples
    size_t size;

    /// Default constructor - zero-initializes buffer
    AudioBuffer() noexcept : size(N) {
        clear();
    }

    /// Clear buffer to zero
    void clear() noexcept {
        for (size_t i = 0; i < N; ++i) {
            data[i] = 0.0f;
        }
    }

    /// Get maximum capacity
    static constexpr size_t capacity() noexcept {
        return N;
    }

    /// Access sample by index
    Sample& operator[](size_t index) noexcept {
        return data[index];
    }

    /// Access sample by index (const)
    const Sample& operator[](size_t index) const noexcept {
        return data[index];
    }

    /// Get pointer to sample data
    Sample* begin() noexcept {
        return data;
    }

    /// Get const pointer to sample data
    const Sample* begin() const noexcept {
        return data;
    }

    /// Get pointer past end of valid data
    Sample* end() noexcept {
        return data + size;
    }

    /// Get const pointer past end of valid data
    const Sample* end() const noexcept {
        return data + size;
    }
};

} // namespace subcollider

#endif // SUBCOLLIDER_AUDIO_BUFFER_H
