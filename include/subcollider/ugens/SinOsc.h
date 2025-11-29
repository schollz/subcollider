/**
 * @file SinOsc.h
 * @brief Sine wave oscillator UGen.
 *
 * SinOsc generates a sine wave using a phase accumulator.
 * Designed for embedded use with no heap allocation and no virtual calls.
 */

#ifndef SUBCOLLIDER_UGENS_SINOSC_H
#define SUBCOLLIDER_UGENS_SINOSC_H

#include "../types.h"
#include <cmath>

namespace subcollider {
namespace ugens {

/**
 * @brief Sine wave oscillator using phase accumulator.
 *
 * This is a static struct with inline per-sample processing,
 * suitable for embedded DSP with no heap allocation.
 *
 * Usage:
 * @code
 * SinOsc osc;
 * osc.init(48000.0f);
 * osc.setFrequency(440.0f);
 * 
 * // Per-sample processing
 * float sample = osc.tick();
 *
 * // Block processing
 * AudioBuffer<64> buffer;
 * osc.process(buffer.data, 64);
 * @endcode
 */
struct SinOsc {
    /// Current phase [0, 2*PI)
    Sample phase;

    /// Phase increment per sample
    Sample phaseIncrement;

    /// Oscillator frequency in Hz
    Sample frequency;

    /// Sample rate in Hz
    Sample sampleRate;

    /**
     * @brief Initialize the oscillator.
     * @param sr Sample rate in Hz (default: 48000)
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE) noexcept {
        sampleRate = sr;
        phase = 0.0f;
        frequency = 440.0f;
        updatePhaseIncrement();
    }

    /**
     * @brief Set oscillator frequency (control-rate parameter update).
     * @param freq Frequency in Hz
     */
    void setFrequency(Sample freq) noexcept {
        frequency = freq;
        updatePhaseIncrement();
    }

    /**
     * @brief Update phase increment from current frequency.
     *
     * Call this at control rate when frequency changes.
     */
    void updatePhaseIncrement() noexcept {
        phaseIncrement = (TWO_PI * frequency) / sampleRate;
    }

    /**
     * @brief Generate single sample (per-sample inline processing).
     * @return Next sample value [-1, 1]
     */
    inline Sample tick() noexcept {
        Sample out = std::sin(phase);
        phase += phaseIncrement;

        // Wrap phase to prevent float precision issues
        if (phase >= TWO_PI) {
            phase -= TWO_PI;
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
     * @brief Reset oscillator phase.
     * @param newPhase Phase value [0, 2*PI)
     */
    void reset(Sample newPhase = 0.0f) noexcept {
        phase = newPhase;
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_SINOSC_H
