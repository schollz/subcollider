/**
 * @file Phasor.h
 * @brief Linear ramp UGen with trigger reset and wrap-around.
 *
 * Phasor is a linear ramp between start and end values.
 * When its trigger input crosses from non-positive to positive, Phasor's
 * output will jump to its reset position. Upon reaching the end of its ramp
 * Phasor will wrap back to its start.
 * NOTE: Since end is defined as the wrap point, its value is never actually output.
 * Designed for embedded use with no heap allocation and no virtual calls.
 */

#ifndef SUBCOLLIDER_UGENS_PHASOR_H
#define SUBCOLLIDER_UGENS_PHASOR_H

#include "../types.h"

namespace subcollider {
namespace ugens {

/**
 * @brief Linear ramp generator with trigger reset and wrap-around.
 *
 * This is a static struct with inline per-sample processing,
 * suitable for embedded DSP with no heap allocation.
 *
 * Phasor generates a linear ramp from start to end at a given rate.
 * When the trigger input crosses from non-positive to positive,
 * the output jumps to the reset position. When the ramp reaches
 * the end value, it wraps back to the start value.
 *
 * Common use: index control with BufRd and BufWr.
 *
 * To output a signal with frequency freq oscillating between start and end:
 * rate = (end - start) * freq / sampleRate
 *
 * Usage:
 * @code
 * Phasor phasor;
 * phasor.init(48000.0f);
 * phasor.set(1.0f, 0.0f, 1.0f);  // rate=1, start=0, end=1
 *
 * // Per-sample processing
 * float value = phasor.tick();
 *
 * // Per-sample processing with trigger
 * float triggerSignal = getTrigger();  // e.g., from another UGen
 * float value = phasor.tick(triggerSignal);
 *
 * // Block processing
 * float buffer[64];
 * phasor.process(buffer, 64);
 * @endcode
 */
struct Phasor {
    /// Current output value
    Sample value;

    /// Rate of change per sample
    Sample rate;

    /// Start value (ramp beginning)
    Sample start;

    /// End value (wrap point, never actually output)
    Sample end;

    /// Reset position (value to jump to on trigger)
    Sample resetPos;

    /// Previous trigger value (for edge detection)
    Sample prevTrig;

    /// Sample rate in Hz
    Sample sampleRate;

    /**
     * @brief Initialize the Phasor.
     * @param sr Sample rate in Hz (default: 48000)
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE) noexcept {
        sampleRate = sr;
        rate = 1.0f;
        start = 0.0f;
        end = 1.0f;
        resetPos = 0.0f;
        value = start;
        prevTrig = 0.0f;
    }

    /**
     * @brief Set Phasor parameters.
     * @param rateVal Rate of change per sample (default: 1.0)
     * @param startVal Start value (default: 0.0)
     * @param endVal End value / wrap point (default: 1.0)
     * @param resetPosVal Reset position on trigger (default: 0.0)
     */
    void set(Sample rateVal, Sample startVal = 0.0f,
             Sample endVal = 1.0f, Sample resetPosVal = 0.0f) noexcept {
        rate = rateVal;
        start = startVal;
        end = endVal;
        resetPos = resetPosVal;
        value = start;
    }

    /**
     * @brief Set rate from frequency.
     *
     * Convenience method to calculate rate from a desired frequency.
     * rate = (end - start) * freq / sampleRate
     *
     * @param freq Desired frequency in Hz
     */
    void setFrequency(Sample freq) noexcept {
        rate = (end - start) * freq / sampleRate;
    }

    /**
     * @brief Set rate directly.
     * @param rateVal Rate of change per sample
     */
    void setRate(Sample rateVal) noexcept {
        rate = rateVal;
    }

    /**
     * @brief Set start value.
     * @param startVal Start value
     */
    void setStart(Sample startVal) noexcept {
        start = startVal;
    }

    /**
     * @brief Set end value (wrap point).
     * @param endVal End value
     */
    void setEnd(Sample endVal) noexcept {
        end = endVal;
    }

    /**
     * @brief Set reset position.
     * @param resetPosVal Reset position
     */
    void setResetPos(Sample resetPosVal) noexcept {
        resetPos = resetPosVal;
    }

    /**
     * @brief Generate single sample without trigger (inline per-sample processing).
     * @return Current value before advancing
     */
    inline Sample tick() noexcept {
        Sample out = value;

        // Advance value by rate
        value += rate;

        // Handle wrap-around
        if (end > start) {
            // Forward ramp
            while (value >= end) {
                value -= (end - start);
            }
            while (value < start) {
                value += (end - start);
            }
        } else if (end < start) {
            // Backward ramp (start > end)
            while (value <= end) {
                value += (start - end);
            }
            while (value > start) {
                value -= (start - end);
            }
        }
        // If end == start, value stays constant

        return out;
    }

    /**
     * @brief Generate single sample with trigger input (inline per-sample processing).
     *
     * When trig crosses from non-positive to positive, value jumps to resetPos.
     *
     * @param trig Trigger input signal
     * @return Current value before advancing
     */
    inline Sample tick(Sample trig) noexcept {
        // Detect positive edge: previous <= 0 and current > 0
        if (prevTrig <= 0.0f && trig > 0.0f) {
            value = resetPos;
        }
        prevTrig = trig;

        return tick();
    }

    /**
     * @brief Process a block of samples (no trigger).
     * @param output Output buffer
     * @param numSamples Number of samples to generate
     */
    void process(Sample* output, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            output[i] = tick();
        }
    }

    /**
     * @brief Process a block of samples with trigger input.
     * @param output Output buffer
     * @param trig Trigger input buffer
     * @param numSamples Number of samples to generate
     */
    void process(Sample* output, const Sample* trig, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            output[i] = tick(trig[i]);
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
     * @brief Reset Phasor to start value.
     */
    void reset() noexcept {
        value = start;
        prevTrig = 0.0f;
    }

    /**
     * @brief Reset Phasor to a specific position.
     * @param pos Position to reset to
     */
    void reset(Sample pos) noexcept {
        value = pos;
        prevTrig = 0.0f;
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_PHASOR_H
