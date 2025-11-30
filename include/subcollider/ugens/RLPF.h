/**
 * @file RLPF.h
 * @brief Resonant Low Pass Filter UGen.
 *
 * Implements a 2-pole biquad resonant low pass filter.
 * Based on the Audio EQ Cookbook by Robert Bristow-Johnson.
 */

#ifndef SUBCOLLIDER_UGENS_RLPF_H
#define SUBCOLLIDER_UGENS_RLPF_H

#include "../types.h"
#include <cmath>

namespace subcollider {
namespace ugens {

/**
 * @brief Resonant Low Pass Filter (2-pole biquad).
 *
 * A resonant low pass filter with adjustable cutoff frequency and resonance.
 * Uses the standard biquad filter structure (Direct Form II Transposed).
 *
 * The resonance parameter (Q) controls the peak at the cutoff frequency.
 * Higher values create a more pronounced resonance peak.
 *
 * Usage:
 * @code
 * RLPF filter;
 * filter.init(48000.0f);
 * filter.setFreq(440.0f);
 * filter.setResonance(0.707f);
 *
 * // Per-sample processing
 * float output = filter.tick(input);
 *
 * // Block processing
 * filter.process(buffer, 64);
 * @endcode
 */
struct RLPF {
    /// Sample rate in Hz
    Sample sampleRate;

    /// Cutoff frequency in Hz
    Sample freq;

    /// Resonance (Q factor), typically 0.707 for Butterworth response
    Sample resonance;

    /**
     * @brief Initialize the filter.
     * @param sr Sample rate in Hz (default: 48000)
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE) noexcept {
        sampleRate = sr;
        z1 = 0.0;
        z2 = 0.0;
        freq = 440.0f;
        resonance = 0.707f;
        updateCoefficients();
    }

    /**
     * @brief Set the cutoff frequency.
     * @param f Cutoff frequency in Hz
     */
    void setFreq(Sample f) noexcept {
        // Clamp frequency to valid range (avoid division by zero and Nyquist issues)
        Sample nyquist = sampleRate * 0.5f;
        freq = f < 1.0f ? 1.0f : (f > nyquist * 0.99f ? nyquist * 0.99f : f);
        updateCoefficients();
    }

    /**
     * @brief Set the resonance (Q factor).
     * @param r Resonance value (typically 0.5 to 10+, 0.707 is Butterworth)
     */
    void setResonance(Sample r) noexcept {
        // Clamp resonance to avoid instability
        resonance = r < 0.1f ? 0.1f : (r > 30.0f ? 30.0f : r);
        updateCoefficients();
    }

    /**
     * @brief Generate single filtered sample.
     * @param input Input sample
     * @return Filtered sample
     */
    inline Sample tick(Sample input) noexcept {
        // Direct Form II Transposed
        double in = static_cast<double>(input);
        double out = b0 * in + z1;
        z1 = b1 * in - a1 * out + z2;
        z2 = b2 * in - a2 * out;
        return static_cast<Sample>(out);
    }

    /**
     * @brief Process a block of samples in-place.
     * @param samples Sample buffer (input/output)
     * @param numSamples Number of samples to process
     */
    void process(Sample* samples, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            samples[i] = tick(samples[i]);
        }
    }

    /**
     * @brief Reset filter state.
     */
    void reset() noexcept {
        z1 = 0.0;
        z2 = 0.0;
    }

private:
    // Filter state (Direct Form II Transposed)
    double z1;
    double z2;

    // Biquad coefficients (normalized)
    double b0, b1, b2;
    double a1, a2;

    /**
     * @brief Update biquad coefficients based on frequency and resonance.
     *
     * Uses the Audio EQ Cookbook formulas for a lowpass filter.
     */
    void updateCoefficients() noexcept {
        // Angular frequency
        double omega = 2.0 * static_cast<double>(PI) * static_cast<double>(freq) / static_cast<double>(sampleRate);
        double sinOmega = std::sin(omega);
        double cosOmega = std::cos(omega);

        // Q is the resonance parameter
        double Q = static_cast<double>(resonance);
        double alpha = sinOmega / (2.0 * Q);

        // Lowpass filter coefficients (Audio EQ Cookbook)
        double b0_raw = (1.0 - cosOmega) / 2.0;
        double b1_raw = 1.0 - cosOmega;
        double b2_raw = (1.0 - cosOmega) / 2.0;
        double a0_raw = 1.0 + alpha;
        double a1_raw = -2.0 * cosOmega;
        double a2_raw = 1.0 - alpha;

        // Normalize coefficients
        b0 = b0_raw / a0_raw;
        b1 = b1_raw / a0_raw;
        b2 = b2_raw / a0_raw;
        a1 = a1_raw / a0_raw;
        a2 = a2_raw / a0_raw;
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_RLPF_H
