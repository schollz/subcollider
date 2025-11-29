/**
 * @file MicrotrackerMoogLadder.h
 * @brief Microtracker Moog Ladder filter UGen.
 *
 * Based on an implementation by Magnus Jonsson
 * https://github.com/magnusjonsson/microtracker (unlicense)
 */

#ifndef SUBCOLLIDER_UGENS_MICROTRACKERMOOGLADDER_H
#define SUBCOLLIDER_UGENS_MICROTRACKERMOOGLADDER_H

#include "../types.h"
#include <cmath>

namespace subcollider {
namespace ugens {

/**
 * @brief Microtracker Moog Ladder filter.
 *
 * A digital model of the classic Moog filter using tanh saturation
 * for nonlinear behavior.
 *
 * Usage:
 * @code
 * MicrotrackerMoogLadder filter;
 * filter.init(48000.0f);
 * filter.setCutoff(1000.0f);
 * filter.setResonance(0.5f);
 *
 * // Per-sample processing
 * float output = filter.tick(input);
 *
 * // Block processing
 * filter.process(buffer, 64);
 * @endcode
 */
struct MicrotrackerMoogLadder {
    /// Sample rate in Hz
    Sample sampleRate;

    /// Cutoff frequency in Hz
    Sample cutoff;

    /// Resonance [0, 1]
    Sample resonance;

    /**
     * @brief Initialize the filter.
     * @param sr Sample rate in Hz (default: 48000)
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE) noexcept {
        sampleRate = sr;
        p0 = p1 = p2 = p3 = p32 = p33 = p34 = 0.0;
        cutoffCoeff = 0.0;
        setCutoff(1000.0f);
        setResonance(0.1f);
    }

    /**
     * @brief Set the cutoff frequency.
     * @param c Cutoff frequency in Hz
     */
    void setCutoff(Sample c) noexcept {
        cutoff = c;
        cutoffCoeff = c * 2.0 * PI / sampleRate;
        if (cutoffCoeff > 1.0) cutoffCoeff = 1.0;
    }

    /**
     * @brief Set the resonance.
     * @param r Resonance [0, 1]
     */
    void setResonance(Sample r) noexcept {
        resonance = r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r);
    }

    /**
     * @brief Generate single filtered sample.
     * @param input Input sample
     * @return Filtered sample
     */
    inline Sample tick(Sample input) noexcept {
        double k = resonance * 4;

        // Coefficients optimized using differential evolution
        // to make feedback gain 4.0 correspond closely to the
        // border of instability, for all values of omega.
        double out = p3 * 0.360891 + p32 * 0.417290 + p33 * 0.177896 + p34 * 0.0439725;

        p34 = p33;
        p33 = p32;
        p32 = p3;

        p0 += (fastTanh(input - k * out) - fastTanh(p0)) * cutoffCoeff;
        p1 += (fastTanh(p0) - fastTanh(p1)) * cutoffCoeff;
        p2 += (fastTanh(p1) - fastTanh(p2)) * cutoffCoeff;
        p3 += (fastTanh(p2) - fastTanh(p3)) * cutoffCoeff;

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
        p0 = p1 = p2 = p3 = p32 = p33 = p34 = 0.0;
    }

private:
    double p0;
    double p1;
    double p2;
    double p3;
    double p32;
    double p33;
    double p34;
    double cutoffCoeff;

    inline double fastTanh(double x) noexcept {
        double x2 = x * x;
        return x * (27.0 + x2) / (27.0 + 9.0 * x2);
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_MICROTRACKERMOOGLADDER_H
