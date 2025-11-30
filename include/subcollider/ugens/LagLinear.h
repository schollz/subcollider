/**
 * @file LagLinear.h
 * @brief Linear lag for smoothing control signals with constant-rate ramps.
 *
 * Unlike Lag (exponential), LagLinear moves toward a new target at a fixed
 * rate so the transition always completes after the specified time.
 */

#ifndef SUBCOLLIDER_UGENS_LAGLINEAR_H
#define SUBCOLLIDER_UGENS_LAGLINEAR_H

#include "../types.h"
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace subcollider {
namespace ugens {

/**
 * @brief Linear ramp smoothing.
 *
 * LagLinear moves linearly toward the latest input value over the configured
 * time in seconds. Every input sample is treated as a potential new target;
 * if it changes, a new ramp starts from the current value toward the new
 * target, completing exactly after `timeSeconds`.
 */
struct LagLinear {
    /// Sample rate in Hz
    Sample sampleRate;

    /// Ramp time in seconds
    Sample timeSeconds;

    /// Current output value
    Sample currentValue;

    /// Target value
    Sample targetValue;

    /// Per-sample increment toward the target
    Sample increment;

    /// Samples remaining in the current ramp
    int samplesRemaining;

    /**
     * @brief Initialize the lag.
     * @param sr Sample rate in Hz (default: 48000)
     * @param initialValue Initial value/output (default: 0)
     * @param time Ramp time in seconds (default: 0.1)
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE, Sample initialValue = 0.0f, Sample time = 0.1f) noexcept {
        sampleRate = sr;
        currentValue = initialValue;
        targetValue = initialValue;
        increment = 0.0f;
        samplesRemaining = 0;
        setTime(time);
    }

    /**
     * @brief Set ramp time (seconds).
     *
     * If a ramp is in progress, its duration is recalculated based on the new
     * time from the current value to the target.
     */
    void setTime(Sample time) noexcept {
        timeSeconds = time;
        if (timeSeconds <= 0.0f) {
            // Instant changes
            increment = 0.0f;
            samplesRemaining = 0;
            currentValue = targetValue;
            return;
        }

        if (targetValue != currentValue) {
            samplesRemaining = std::max(1, static_cast<int>(std::round(timeSeconds * sampleRate)));
            recalcIncrement();
        }
    }

    /**
     * @brief Process a single sample/target.
     * @param input New target value for this sample
     * @return Smoothed output value
     */
    inline Sample tick(Sample input) noexcept {
        if (input != targetValue) {
            targetValue = input;
            if (timeSeconds <= 0.0f) {
                currentValue = targetValue;
                samplesRemaining = 0;
                increment = 0.0f;
                return currentValue;
            }
            samplesRemaining = std::max(1, static_cast<int>(std::round(timeSeconds * sampleRate)));
            recalcIncrement();
        }

        if (samplesRemaining > 0) {
            currentValue += increment;
            samplesRemaining--;
            if (samplesRemaining == 0) {
                // Snap to target to avoid residual error
                currentValue = targetValue;
                increment = 0.0f;
            }
        } else {
            currentValue = targetValue;
        }

        return currentValue;
    }

    /**
     * @brief Process a block in-place.
     * @param samples Input/output buffer of target values
     * @param numSamples Number of samples
     */
    void process(Sample* samples, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            Sample target = samples[i];
            samples[i] = tick(target);
        }
    }

    /**
     * @brief Process a block with separate input/output buffers.
     * @param input Target values
     * @param output Smoothed output
     * @param numSamples Number of samples
     */
    void process(const Sample* input, Sample* output, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            output[i] = tick(input[i]);
        }
    }

    /**
     * @brief Reset state to zero.
     */
    void reset() noexcept {
        currentValue = 0.0f;
        targetValue = 0.0f;
        samplesRemaining = 0;
        increment = 0.0f;
    }

    /**
     * @brief Force the current/target value (clears ramps).
     * @param value New fixed value
     */
    void setValue(Sample value) noexcept {
        currentValue = value;
        targetValue = value;
        samplesRemaining = 0;
        increment = 0.0f;
    }

private:
    inline void recalcIncrement() noexcept {
        if (samplesRemaining <= 0) {
            increment = 0.0f;
            return;
        }
        increment = (targetValue - currentValue) / static_cast<Sample>(samplesRemaining);
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_LAGLINEAR_H
