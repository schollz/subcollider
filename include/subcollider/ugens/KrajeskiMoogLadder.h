/**
 * @file KrajeskiMoogLadder.h
 * @brief Krajeski Moog Ladder filter UGen.
 *
 * This code is Unlicensed (i.e. public domain); Aaron Krajeski stated:
 * "That work is under no copyright. You may use it however you might like."
 *
 * Source: http://song-swap.com/MUMT618/aaron/Presentation/demo.html
 */

#ifndef SUBCOLLIDER_UGENS_KRAJESKIMOOGLADDER_H
#define SUBCOLLIDER_UGENS_KRAJESKIMOOGLADDER_H

#include "../types.h"
#include <cmath>
#include <cstring>

namespace subcollider {
namespace ugens {

/**
 * @brief Krajeski Moog Ladder filter.
 *
 * This implements Tim Stilson's MoogVCF filter using 'compromise' poles at z = -0.3
 * with several improvements including corrections for cutoff and resonance parameters,
 * and a smoothly saturating tanh() function.
 *
 * Usage:
 * @code
 * KrajeskiMoogLadder filter;
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
struct KrajeskiMoogLadder {
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
        std::memset(state, 0, sizeof(state));
        std::memset(delay, 0, sizeof(delay));

        drive = 1.0;
        gComp = 1.0;
        wc = 0.0;
        g = 0.0;
        gRes = 0.0;

        setCutoff(1000.0f);
        setResonance(0.1f);
    }

    /**
     * @brief Set the cutoff frequency.
     * @param c Cutoff frequency in Hz
     */
    void setCutoff(Sample c) noexcept {
        cutoff = c;
        wc = 2.0 * PI * cutoff / sampleRate;
        g = 0.9892 * wc - 0.4342 * std::pow(wc, 2) + 0.1381 * std::pow(wc, 3) - 0.0202 * std::pow(wc, 4);
    }

    /**
     * @brief Set the resonance.
     * @param r Resonance [0, 1]
     */
    void setResonance(Sample r) noexcept {
        resonance = r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r);
        gRes = resonance * (1.0029 + 0.0526 * wc - 0.926 * std::pow(wc, 2) + 0.0218 * std::pow(wc, 3));
    }

    /**
     * @brief Generate single filtered sample.
     * @param input Input sample
     * @return Filtered sample
     */
    inline Sample tick(Sample input) noexcept {
        state[0] = std::tanh(drive * (input - 4.0 * gRes * (state[4] - gComp * input)));

        for (int i = 0; i < 4; i++) {
            state[i + 1] = fclamp(g * (0.3 / 1.3 * state[i] + 1.0 / 1.3 * delay[i] - state[i + 1]) + state[i + 1], -1e30, 1e30);
            delay[i] = state[i];
        }

        return static_cast<Sample>(state[4]);
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
        std::memset(state, 0, sizeof(state));
        std::memset(delay, 0, sizeof(delay));
    }

private:
    double state[5];
    double delay[5];
    double wc;      // The angular frequency of the cutoff.
    double g;       // A derived parameter for the cutoff frequency
    double gRes;    // A similar derived parameter for resonance.
    double gComp;   // Compensation factor.
    double drive;   // A parameter that controls intensity of nonlinearities.

    inline double fclamp(double in, double min, double max) noexcept {
        return std::fmin(std::fmax(in, min), max);
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_KRAJESKIMOOGLADDER_H
