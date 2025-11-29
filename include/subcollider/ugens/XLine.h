/**
 * @file XLine.h
 * @brief Exponential line generator UGen.
 *
 * XLine generates an exponential curve from the start value to the end value.
 * Both the start and end values must be non-zero and have the same sign.
 * Designed for embedded use with no heap allocation and no virtual calls.
 */

#ifndef SUBCOLLIDER_UGENS_XLINE_H
#define SUBCOLLIDER_UGENS_XLINE_H

#include "../types.h"
#include <cmath>

namespace subcollider {
namespace ugens {

/**
 * @brief Exponential line generator.
 *
 * This is a static struct with inline per-sample processing,
 * suitable for embedded DSP with no heap allocation.
 *
 * The XLine UGen generates an exponential curve from a start value
 * to an end value over a specified duration. Both values must be
 * non-zero and have the same sign.
 *
 * Usage:
 * @code
 * XLine line;
 * line.init(48000.0f);
 * line.set(1.0f, 2.0f, 1.0f);  // Start at 1.0, end at 2.0, over 1 second
 *
 * // Per-sample processing
 * float value = line.tick();
 *
 * // Block processing
 * AudioBuffer<64> buffer;
 * line.process(buffer.data, 64);
 * @endcode
 */
struct XLine {
    /// Current output value
    Sample value;

    /// Starting value
    Sample start;

    /// Ending value
    Sample end;

    /// Duration in seconds
    Sample dur;

    /// Multiplication factor applied to output
    Sample mul;

    /// Addition offset applied to output
    Sample add;

    /// Growth factor per sample (multiplicative)
    Sample growthFactor;

    /// Total samples for the line duration
    Sample durSamples;

    /// Current sample counter
    Sample counter;

    /// Sample rate in Hz
    Sample sampleRate;

    /// Flag indicating if the line has completed
    bool done;

    /**
     * @brief Initialize the XLine generator.
     * @param sr Sample rate in Hz (default: 48000)
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE) noexcept {
        sampleRate = sr;
        value = 1.0f;
        start = 1.0f;
        end = 2.0f;
        dur = 1.0f;
        mul = 1.0f;
        add = 0.0f;
        counter = 0.0f;
        durSamples = dur * sampleRate;
        done = false;
        updateGrowthFactor();
    }

    /**
     * @brief Set the line parameters.
     * @param startVal Starting value (must be non-zero)
     * @param endVal Ending value (must be non-zero and same sign as start)
     * @param duration Duration in seconds
     * @param mulVal Output multiplier (default: 1.0)
     * @param addVal Output offset (default: 0.0)
     *
     * Note: Both start and end must be non-zero and have the same sign
     * for the exponential curve to be mathematically valid.
     *
     * Automatic adjustments are made for invalid inputs:
     * - Zero values are adjusted to 0.0001 (or -0.0001 if other value is negative)
     * - If signs differ, both values are converted to their absolute values
     * - Duration <= 0 is adjusted to 0.001 seconds
     *
     * These adjustments prevent undefined behavior (NaN/Inf) but may not
     * produce the intended output. For predictable results, ensure inputs
     * meet the constraints.
     */
    void set(Sample startVal, Sample endVal, Sample duration,
             Sample mulVal = 1.0f, Sample addVal = 0.0f) noexcept {
        // Ensure non-zero values
        if (startVal == 0.0f) startVal = 0.0001f;
        if (endVal == 0.0f) endVal = 0.0001f;

        // Ensure same sign - if different signs, use absolute values
        if ((startVal > 0.0f) != (endVal > 0.0f)) {
            startVal = std::abs(startVal);
            endVal = std::abs(endVal);
        }

        start = startVal;
        end = endVal;
        dur = duration > 0.0f ? duration : 0.001f;
        mul = mulVal;
        add = addVal;
        value = start;
        counter = 0.0f;
        durSamples = dur * sampleRate;
        done = false;
        updateGrowthFactor();
    }

    /**
     * @brief Update the growth factor from current parameters.
     *
     * The exponential growth factor is calculated as:
     * growthFactor = (end/start)^(1/durSamples)
     *
     * This means: value *= growthFactor each sample
     * After durSamples: value = start * growthFactor^durSamples = end
     *
     * Note: For negative values with the same sign, end/start is positive,
     * so the logarithm is valid. The value stays negative because we
     * multiply a negative start by a positive growth factor.
     */
    void updateGrowthFactor() noexcept {
        if (durSamples > 0.0f && start != 0.0f) {
            // growthFactor = (end/start)^(1/durSamples)
            // = exp(log(end/start) / durSamples)
            // Note: end/start is always positive when start and end have the same sign
            growthFactor = std::exp(std::log(end / start) / durSamples);
        } else {
            growthFactor = 1.0f;
        }
    }

    /**
     * @brief Generate single sample (per-sample inline processing).
     * @return Next sample value with mul and add applied
     */
    inline Sample tick() noexcept {
        Sample out = value * mul + add;

        if (!done) {
            counter += 1.0f;
            if (counter >= durSamples) {
                value = end;
                done = true;
            } else {
                value *= growthFactor;
            }
        }

        return out;
    }

    /**
     * @brief Process a block of samples.
     * @param output Output buffer
     * @param numSamples Number of samples to generate
     */
    void process(Sample* output, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            output[i] = tick();
        }
    }

    /**
     * @brief Process a block, multiplying with existing buffer.
     * @param buffer Buffer to multiply in-place
     * @param numSamples Number of samples to process
     */
    void processMul(Sample* buffer, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            buffer[i] *= tick();
        }
    }

    /**
     * @brief Process a block, adding to existing buffer.
     * @param output Output buffer to add to
     * @param numSamples Number of samples to generate
     */
    void processAdd(Sample* output, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            output[i] += tick();
        }
    }

    /**
     * @brief Check if the line has completed.
     * @return True if the line has reached its end value
     */
    bool isDone() const noexcept {
        return done;
    }

    /**
     * @brief Reset the line to its initial state.
     */
    void reset() noexcept {
        value = start;
        counter = 0.0f;
        done = false;
    }

    /**
     * @brief Trigger a new line from current parameters.
     *
     * Resets the line to start a new exponential curve.
     */
    void trigger() noexcept {
        reset();
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_XLINE_H
