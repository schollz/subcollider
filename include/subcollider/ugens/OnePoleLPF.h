/**
 * @file OnePoleLPF.h
 * @brief Simple one-pole lowpass filter.
 */

#ifndef SUBCOLLIDER_UGENS_ONEPOLELPF_H
#define SUBCOLLIDER_UGENS_ONEPOLELPF_H

#include "../types.h"
#include <cmath>
#include <cstddef>

namespace subcollider {
namespace ugens {

/**
 * @brief One-pole lowpass filter with optional per-sample cutoff modulation.
 *
 * The filter uses the standard difference equation:
 *   y[n] = g * x[n] + p * y[n-1]
 * where p = exp(-2 * pi * cutoff / sampleRate) and g = 1 - p.
 */
struct OnePoleLPF {
    /// Sample rate in Hz
    Sample sampleRate;

    /// Current cutoff in Hz
    Sample cutoff;

    /// Cached pole coefficient
    Sample pole;

    /// Cached input gain (1 - pole)
    Sample gain;

    /// Filter state
    Sample z;

    /**
     * @brief Initialize the filter.
     * @param sr Sample rate in Hz
     * @param cutoffHz Initial cutoff frequency in Hz
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE, Sample cutoffHz = 1000.0f) noexcept {
        sampleRate = sr;
        z = 0.0f;
        setCutoff(cutoffHz);
    }

    /**
     * @brief Set the cutoff frequency.
     * @param cutoffHz Cutoff in Hz (clamped to [1 Hz, Nyquist])
     */
    void setCutoff(Sample cutoffHz) noexcept {
        Sample nyquist = sampleRate * 0.5f;
        cutoff = clamp(cutoffHz, 1.0f, nyquist);
        pole = std::exp(-TWO_PI * cutoff / sampleRate);
        gain = 1.0f - pole;
    }

    /**
     * @brief Process a single sample using the current cutoff.
     * @param input Input sample
     * @return Filtered output sample
     */
    inline Sample tick(Sample input) noexcept {
        z = gain * input + pole * z;
        return z;
    }

    /**
     * @brief Process a single sample with per-sample cutoff modulation.
     * @param input Input sample
     * @param cutoffHz Cutoff for this sample
     * @return Filtered output sample
     */
    inline Sample tick(Sample input, Sample cutoffHz) noexcept {
        Sample nyquist = sampleRate * 0.5f;
        Sample c = clamp(cutoffHz, 1.0f, nyquist);
        Sample localPole = std::exp(-TWO_PI * c / sampleRate);
        Sample localGain = 1.0f - localPole;
        z = localGain * input + localPole * z;
        return z;
    }

    /**
     * @brief Process a buffer in-place using the current cutoff.
     * @param samples Sample buffer
     * @param numSamples Number of samples
     */
    void process(Sample* samples, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            samples[i] = tick(samples[i]);
        }
    }

    /**
     * @brief Process a buffer with per-sample cutoff modulation.
     * @param input Input buffer
     * @param cutoffBuffer Cutoff buffer (Hz) per sample
     * @param output Output buffer
     * @param numSamples Number of samples
     */
    void process(const Sample* input, const Sample* cutoffBuffer, Sample* output, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            output[i] = tick(input[i], cutoffBuffer[i]);
        }
    }

    /**
     * @brief Reset filter state.
     */
    void reset() noexcept {
        z = 0.0f;
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_ONEPOLELPF_H
