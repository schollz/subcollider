/**
 * @file Lag.h
 * @brief Exponential lag filter for smoothing control signals.
 *
 * Lag is essentially a one-pole lowpass filter where the coefficient is
 * calculated from a 60 dB lag time. This is the time required for the
 * filter to converge to within 0.01% of a value (60 dB attenuation).
 *
 * This is useful for smoothing out control signals, particularly for
 * parameters controlled by MIDI, OSC, or mouse input.
 *
 * For linear and other alternatives, see VarLag (future implementation).
 */

#ifndef SUBCOLLIDER_UGENS_LAG_H
#define SUBCOLLIDER_UGENS_LAG_H

#include "../types.h"
#include <cmath>

namespace subcollider {
namespace ugens {

/**
 * @brief Exponential lag (smoothing) filter.
 *
 * The Lag filter provides exponential smoothing with a specified lag time.
 * The lag time is defined as the time required for the filter to reach
 * within 0.01% (60 dB) of the target value.
 *
 * The filter equation is:
 *   y[n] = coeff * y[n-1] + (1 - coeff) * x[n]
 *
 * Where coeff is calculated from the lag time:
 *   coeff = exp(log(0.001) / (lagTime * sampleRate))
 *
 * Usage:
 * @code
 * Lag smoother;
 * smoother.init(48000.0f, 0.1f);  // 100ms lag time
 *
 * // Per-sample processing
 * float smoothed = smoother.tick(noisyInput);
 *
 * // Change lag time
 * smoother.setLagTime(0.2f);
 * @endcode
 */
struct Lag {
    /// Sample rate in Hz
    Sample sampleRate;

    /// Lag time in seconds (time to reach 60 dB attenuation)
    Sample lagTime;

    /// Filter coefficient (calculated from lag time)
    Sample coeff;

    /// Previous output value (filter state)
    Sample prevOutput;

    /**
     * @brief Initialize the lag filter.
     * @param sr Sample rate in Hz (default: 48000)
     * @param lt Lag time in seconds (default: 0.1)
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE, Sample lt = 0.1f) noexcept {
        sampleRate = sr;
        prevOutput = 0.0f;
        setLagTime(lt);
    }

    /**
     * @brief Set the lag time.
     * @param lt Lag time in seconds (60 dB settling time)
     *
     * The lag time is the time it takes for the filter to reach within
     * 0.01% (60 dB) of a step change in the input.
     */
    void setLagTime(Sample lt) noexcept {
        lagTime = lt;

        // Calculate coefficient from lag time
        // For 60 dB (0.001 amplitude), we want:
        // coeff^(lagTime * sampleRate) = 0.001
        // coeff = exp(log(0.001) / (lagTime * sampleRate))

        if (lagTime <= 0.0f) {
            // No lag - pass through
            coeff = 0.0f;
        } else {
            Sample numSamples = lagTime * sampleRate;
            // log(0.001) ≈ -6.907755
            coeff = std::exp(-6.907755f / numSamples);
        }
    }

    /**
     * @brief Process a single sample.
     * @param input Input sample
     * @return Smoothed output sample
     */
    inline Sample tick(Sample input) noexcept {
        // One-pole filter: y[n] = coeff * y[n-1] + (1 - coeff) * x[n]
        prevOutput = coeff * prevOutput + (1.0f - coeff) * input;
        return prevOutput;
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
     * @brief Process a block with separate input and output buffers.
     * @param input Input buffer
     * @param output Output buffer
     * @param numSamples Number of samples to process
     */
    void process(const Sample* input, Sample* output, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            output[i] = tick(input[i]);
        }
    }

    /**
     * @brief Reset filter state.
     */
    void reset() noexcept {
        prevOutput = 0.0f;
    }

    /**
     * @brief Set initial value for the filter state.
     * @param value Initial value
     *
     * Useful to prevent an initial transient when starting with a
     * known value instead of zero.
     */
    void setValue(Sample value) noexcept {
        prevOutput = value;
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_LAG_H
