/**
 * @file BufRd.h
 * @brief Buffer reader UGen with variable interpolation.
 *
 * BufRd reads the content of a buffer at a given phase index.
 * Unlike PlayBuf, BufRd does not advance through the buffer by itself;
 * the read position is controlled entirely by the phase input.
 * Designed for embedded use with no heap allocation and no virtual calls.
 */

#ifndef SUBCOLLIDER_UGENS_BUFRD_H
#define SUBCOLLIDER_UGENS_BUFRD_H

#include "../types.h"
#include "../Buffer.h"
#include <cmath>

namespace subcollider {
namespace ugens {

/**
 * @brief Buffer reader with variable interpolation.
 *
 * This is a static struct with inline per-sample processing,
 * suitable for embedded DSP with no heap allocation.
 *
 * BufRd reads audio samples from a Buffer at a phase position.
 * The phase is typically provided by a Phasor UGen for continuous
 * playback or by any other modulation source.
 *
 * Interpolation modes:
 * - 1: No interpolation (sample & hold)
 * - 2: Linear interpolation
 * - 4: Cubic interpolation
 *
 * Any other interpolation value defaults to no interpolation.
 *
 * Usage:
 * @code
 * Buffer buf(audioData, 1, 48000.0f, numSamples);
 * BufRd reader;
 * reader.init(&buf);
 * reader.setLoop(true);
 * reader.setInterpolation(2);  // Linear interpolation
 *
 * // Per-sample processing with Phasor
 * Phasor phasor;
 * phasor.init(48000.0f);
 * phasor.set(1.0f, 0.0f, static_cast<float>(numSamples));
 *
 * float sample = reader.tick(phasor.tick());  // For mono
 * Stereo stereoSample = reader.tickStereo(phasor.tick());  // For stereo
 * @endcode
 */
struct BufRd {
    /// Pointer to the buffer to read from
    const Buffer* buffer;

    /// Loop mode: true means wrap around, false means clamp to bounds
    bool loop;

    /// Interpolation mode: 1=none, 2=linear, 4=cubic
    uint8_t interpolation;

    /**
     * @brief Initialize BufRd with a buffer.
     * @param buf Pointer to the buffer to read from
     */
    void init(const Buffer* buf = nullptr) noexcept {
        buffer = buf;
        loop = true;
        interpolation = 2;  // Linear interpolation by default
    }

    /**
     * @brief Set the buffer to read from.
     * @param buf Pointer to the buffer
     */
    void setBuffer(const Buffer* buf) noexcept {
        buffer = buf;
    }

    /**
     * @brief Set loop mode.
     * @param loopEnabled true for looping, false for clamping
     */
    void setLoop(bool loopEnabled) noexcept {
        loop = loopEnabled;
    }

    /**
     * @brief Set interpolation mode.
     * @param mode 1=none, 2=linear, 4=cubic (others default to none)
     */
    void setInterpolation(uint8_t mode) noexcept {
        interpolation = mode;
    }

    /**
     * @brief Read a mono sample from the buffer at the given phase.
     *
     * For mono buffers, returns the sample directly.
     * For stereo buffers, returns the left channel.
     *
     * @param phase Index into the buffer (can be fractional)
     * @return Sample value at the given phase
     */
    inline Sample tick(Sample phase) const noexcept {
        if (buffer == nullptr || !buffer->isValid()) {
            return 0.0f;
        }

        const size_t numSamples = buffer->numSamples;
        const Sample numSamplesF = static_cast<Sample>(numSamples);

        // Handle phase wrapping or clamping
        Sample adjustedPhase = phase;
        if (loop) {
            // Wrap phase to [0, numSamples)
            adjustedPhase = std::fmod(phase, numSamplesF);
            if (adjustedPhase < 0.0f) {
                adjustedPhase += numSamplesF;
            }
        } else {
            // Clamp phase to [0, numSamples - 1]
            adjustedPhase = clamp(phase, 0.0f, numSamplesF - 1.0f);
        }

        // Get integer and fractional parts
        const size_t index0 = static_cast<size_t>(adjustedPhase);
        const Sample frac = adjustedPhase - static_cast<Sample>(index0);

        // Apply interpolation
        if (interpolation == 2) {
            // Linear interpolation
            const size_t index1 = wrapIndex(index0 + 1, numSamples);
            const Sample s0 = buffer->getSample(index0);
            const Sample s1 = buffer->getSample(index1);
            return lerp(s0, s1, frac);
        } else if (interpolation == 4) {
            // Cubic interpolation (Catmull-Rom spline)
            const size_t indexM1 = wrapIndex(index0 + numSamples - 1, numSamples);
            const size_t index1 = wrapIndex(index0 + 1, numSamples);
            const size_t index2 = wrapIndex(index0 + 2, numSamples);

            const Sample sM1 = buffer->getSample(indexM1);
            const Sample s0 = buffer->getSample(index0);
            const Sample s1 = buffer->getSample(index1);
            const Sample s2 = buffer->getSample(index2);

            return cubicInterp(sM1, s0, s1, s2, frac);
        } else {
            // No interpolation (sample & hold)
            return buffer->getSample(index0);
        }
    }

    /**
     * @brief Read a stereo sample from the buffer at the given phase.
     *
     * For stereo buffers, returns left and right channels.
     * For mono buffers, returns the same value in both channels.
     *
     * @param phase Index into the buffer (can be fractional)
     * @return Stereo sample at the given phase
     */
    inline Stereo tickStereo(Sample phase) const noexcept {
        if (buffer == nullptr || !buffer->isValid()) {
            return Stereo();
        }

        const size_t numSamples = buffer->numSamples;
        const Sample numSamplesF = static_cast<Sample>(numSamples);

        // Handle phase wrapping or clamping
        Sample adjustedPhase = phase;
        if (loop) {
            // Wrap phase to [0, numSamples)
            adjustedPhase = std::fmod(phase, numSamplesF);
            if (adjustedPhase < 0.0f) {
                adjustedPhase += numSamplesF;
            }
        } else {
            // Clamp phase to [0, numSamples - 1]
            adjustedPhase = clamp(phase, 0.0f, numSamplesF - 1.0f);
        }

        // Get integer and fractional parts
        const size_t index0 = static_cast<size_t>(adjustedPhase);
        const Sample frac = adjustedPhase - static_cast<Sample>(index0);

        // Apply interpolation
        if (interpolation == 2) {
            // Linear interpolation
            const size_t index1 = wrapIndex(index0 + 1, numSamples);
            const Stereo s0 = buffer->getStereoSample(index0);
            const Stereo s1 = buffer->getStereoSample(index1);
            return Stereo(
                lerp(s0.left, s1.left, frac),
                lerp(s0.right, s1.right, frac)
            );
        } else if (interpolation == 4) {
            // Cubic interpolation (Catmull-Rom spline)
            const size_t indexM1 = wrapIndex(index0 + numSamples - 1, numSamples);
            const size_t index1 = wrapIndex(index0 + 1, numSamples);
            const size_t index2 = wrapIndex(index0 + 2, numSamples);

            const Stereo sM1 = buffer->getStereoSample(indexM1);
            const Stereo s0 = buffer->getStereoSample(index0);
            const Stereo s1 = buffer->getStereoSample(index1);
            const Stereo s2 = buffer->getStereoSample(index2);

            return Stereo(
                cubicInterp(sM1.left, s0.left, s1.left, s2.left, frac),
                cubicInterp(sM1.right, s0.right, s1.right, s2.right, frac)
            );
        } else {
            // No interpolation (sample & hold)
            return buffer->getStereoSample(index0);
        }
    }

    /**
     * @brief Process a block of mono samples.
     * @param output Output buffer for mono samples
     * @param phase Phase buffer (index values)
     * @param numSamples Number of samples to process
     */
    void process(Sample* output, const Sample* phase, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            output[i] = tick(phase[i]);
        }
    }

    /**
     * @brief Process a block of stereo samples (interleaved output).
     * @param left Output buffer for left channel
     * @param right Output buffer for right channel
     * @param phase Phase buffer (index values)
     * @param numSamples Number of samples to process
     */
    void processStereo(Sample* left, Sample* right, const Sample* phase,
                       size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            Stereo s = tickStereo(phase[i]);
            left[i] = s.left;
            right[i] = s.right;
        }
    }

private:
    /**
     * @brief Wrap an index within the buffer bounds.
     * @param index Index to wrap
     * @param size Buffer size
     * @return Wrapped index
     */
    inline size_t wrapIndex(size_t index, size_t size) const noexcept {
        if (loop) {
            return index % size;
        } else {
            return index < size ? index : size - 1;
        }
    }

    /**
     * @brief Cubic interpolation using Catmull-Rom spline.
     * @param y0 Sample at t-1
     * @param y1 Sample at t (current)
     * @param y2 Sample at t+1
     * @param y3 Sample at t+2
     * @param t Fractional position [0, 1]
     * @return Interpolated value
     */
    static inline Sample cubicInterp(Sample y0, Sample y1, Sample y2, Sample y3,
                                     Sample t) noexcept {
        // Catmull-Rom spline interpolation
        const Sample t2 = t * t;
        const Sample t3 = t2 * t;

        const Sample a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
        const Sample a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const Sample a2 = -0.5f * y0 + 0.5f * y2;
        const Sample a3 = y1;

        return a0 * t3 + a1 * t2 + a2 * t + a3;
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_BUFRD_H
