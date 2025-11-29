/**
 * @file Buffer.h
 * @brief Audio sample buffer for UGen use.
 *
 * Buffer provides a structure for storing audio samples that can be used by UGens.
 * Supports both mono and interleaved stereo audio data.
 */

#ifndef SUBCOLLIDER_BUFFER_H
#define SUBCOLLIDER_BUFFER_H

#include "types.h"

namespace subcollider {

/**
 * @brief Audio sample buffer for UGen use.
 *
 * This struct holds a pointer to audio sample data along with metadata
 * about the buffer's format. The buffer can contain either mono or
 * interleaved stereo float audio.
 *
 * For mono buffers (channels == 1):
 *   data[0], data[1], data[2], ... are consecutive samples
 *
 * For stereo buffers (channels == 2):
 *   data[0], data[1] = left[0], right[0]
 *   data[2], data[3] = left[1], right[1]
 *   etc. (interleaved format)
 *
 * Note: This struct does not own the memory pointed to by `data`.
 * The caller is responsible for managing the lifetime of the audio data.
 */
struct Buffer {
    /// Pointer to float array containing audio samples
    Sample* data;

    /// Number of channels (1 for mono, 2 for stereo)
    uint8_t channels;

    /// Original sample rate in Hz
    Sample sampleRate;

    /// Number of samples in the buffer (per channel for stereo)
    size_t numSamples;

    /// Default constructor - initialize to empty buffer
    constexpr Buffer() noexcept
        : data(nullptr)
        , channels(1)
        , sampleRate(DEFAULT_SAMPLE_RATE)
        , numSamples(0) {}

    /**
     * @brief Construct a buffer with the given parameters.
     * @param d Pointer to audio sample data
     * @param ch Number of channels (1 or 2)
     * @param sr Sample rate in Hz
     * @param n Number of samples (per channel for stereo)
     */
    constexpr Buffer(Sample* d, uint8_t ch, Sample sr, size_t n) noexcept
        : data(d)
        , channels(ch)
        , sampleRate(sr)
        , numSamples(n) {}

    /**
     * @brief Check if the buffer is valid (has data and samples).
     * @return true if buffer is valid, false otherwise
     */
    constexpr bool isValid() const noexcept {
        return data != nullptr && numSamples > 0 && (channels == 1 || channels == 2);
    }

    /**
     * @brief Check if the buffer is mono.
     * @return true if buffer is mono, false otherwise
     */
    constexpr bool isMono() const noexcept {
        return channels == 1;
    }

    /**
     * @brief Check if the buffer is stereo.
     * @return true if buffer is stereo, false otherwise
     */
    constexpr bool isStereo() const noexcept {
        return channels == 2;
    }

    /**
     * @brief Get the total number of float values in the data array.
     * @return Total number of floats (numSamples * channels)
     */
    constexpr size_t totalFloats() const noexcept {
        return numSamples * channels;
    }

    /**
     * @brief Get mono sample at the given index.
     * @param index Sample index
     * @return Sample value at index (for mono buffers)
     *
     * For mono buffers, returns data[index].
     * For stereo buffers, returns the left channel sample at that position.
     */
    Sample getSample(size_t index) const noexcept {
        if (data == nullptr || index >= numSamples) {
            return 0.0f;
        }
        return data[index * channels];
    }

    /**
     * @brief Get stereo sample at the given index.
     * @param index Sample index
     * @return Stereo sample at index
     *
     * For mono buffers, returns the same value in both channels.
     * For stereo buffers, returns the interleaved left/right pair.
     */
    Stereo getStereoSample(size_t index) const noexcept {
        if (data == nullptr || index >= numSamples) {
            return Stereo();
        }
        if (channels == 1) {
            return Stereo(data[index]);
        }
        const size_t i = index * 2;
        return Stereo(data[i], data[i + 1]);
    }

    /**
     * @brief Get the duration of the buffer in seconds.
     * @return Duration in seconds
     */
    constexpr Sample duration() const noexcept {
        if (sampleRate <= 0.0f) {
            return 0.0f;
        }
        return static_cast<Sample>(numSamples) / sampleRate;
    }
};

} // namespace subcollider

#endif // SUBCOLLIDER_BUFFER_H
