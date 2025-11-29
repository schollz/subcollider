/**
 * @file OberheimMoogLadder.h
 * @brief Oberheim Variation Moog Ladder filter UGen.
 *
 * See: http://www.willpirkle.com/forum/licensing-and-book-code/licensing-and-using-book-code/
 * The license is "You may also use the code from the FX and Synth books without licensing or fees.
 * The code is for you to develop your own plugins for your own use or for commercial use."
 */

#ifndef SUBCOLLIDER_UGENS_OBERHEIMMOOGLADDER_H
#define SUBCOLLIDER_UGENS_OBERHEIMMOOGLADDER_H

#include "../types.h"
#include <cmath>

namespace subcollider {
namespace ugens {

/**
 * @brief Oberheim Variation Moog Ladder filter.
 *
 * Based on Will Pirkle's virtual analog model using four cascaded
 * one-pole filters with feedback.
 *
 * Usage:
 * @code
 * OberheimMoogLadder filter;
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
struct OberheimMoogLadder {
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

        // Initialize one-pole filters
        for (int i = 0; i < 4; i++) {
            lpf[i].alpha = 1.0;
            lpf[i].beta = 0.0;
            lpf[i].gamma = 1.0;
            lpf[i].delta = 0.0;
            lpf[i].epsilon = 0.0;
            lpf[i].a0 = 1.0;
            lpf[i].feedback = 0.0;
            lpf[i].z1 = 0.0;
        }

        saturation = 1.0;
        K = 0.0;
        gamma = 0.0;
        alpha0 = 1.0;

        for (int i = 0; i < 5; i++) {
            oberheimCoefs[i] = 0.0;
        }
        oberheimCoefs[4] = 1.0;

        setCutoff(1000.0f);
        setResonance(0.1f);
    }

    /**
     * @brief Set the cutoff frequency.
     * @param c Cutoff frequency in Hz
     */
    void setCutoff(Sample c) noexcept {
        cutoff = c;

        // prewarp for BZT
        double wd = 2.0 * PI * cutoff;
        double T = 1.0 / sampleRate;
        double wa = (2.0 / T) * std::tan(wd * T / 2.0);
        double g = wa * T / 2.0;

        // Feedforward coeff
        double G = g / (1.0 + g);

        for (int i = 0; i < 4; i++) {
            lpf[i].alpha = G;
        }

        lpf[0].beta = G * G * G / (1.0 + g);
        lpf[1].beta = G * G / (1.0 + g);
        lpf[2].beta = G / (1.0 + g);
        lpf[3].beta = 1.0 / (1.0 + g);

        gamma = G * G * G * G;
        alpha0 = 1.0 / (1.0 + K * gamma);

        // Oberheim variations / LPF4
        oberheimCoefs[0] = 0.0;
        oberheimCoefs[1] = 0.0;
        oberheimCoefs[2] = 0.0;
        oberheimCoefs[3] = 0.0;
        oberheimCoefs[4] = 1.0;
    }

    /**
     * @brief Set the resonance.
     * @param r Resonance [0, 1]
     */
    void setResonance(Sample r) noexcept {
        resonance = r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r);
        // this maps resonance = 0->1 to K = 0 -> 4
        K = 4.0 * resonance;
        alpha0 = 1.0 / (1.0 + K * gamma);
    }

    /**
     * @brief Generate single filtered sample.
     * @param input Input sample
     * @return Filtered sample
     */
    inline Sample tick(Sample input) noexcept {
        double sigma =
            getFeedbackOutput(0) +
            getFeedbackOutput(1) +
            getFeedbackOutput(2) +
            getFeedbackOutput(3);

        double in = input * (1.0 + K);

        // calculate input to first filter
        double u = (in - K * sigma) * alpha0;

        u = std::tanh(saturation * u);

        double stage1 = tickOnePole(0, u);
        double stage2 = tickOnePole(1, stage1);
        double stage3 = tickOnePole(2, stage2);
        double stage4 = tickOnePole(3, stage3);

        // Oberheim variations
        double out =
            oberheimCoefs[0] * u +
            oberheimCoefs[1] * stage1 +
            oberheimCoefs[2] * stage2 +
            oberheimCoefs[3] * stage3 +
            oberheimCoefs[4] * stage4;

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
        for (int i = 0; i < 4; i++) {
            lpf[i].z1 = 0.0;
            lpf[i].feedback = 0.0;
        }
    }

private:
    struct OnePole {
        double alpha;
        double beta;
        double gamma;
        double delta;
        double epsilon;
        double a0;
        double feedback;
        double z1;
    };

    OnePole lpf[4];

    double K;
    double gamma;
    double alpha0;
    double saturation;
    double oberheimCoefs[5];

    inline double getFeedbackOutput(int index) noexcept {
        return lpf[index].beta * (lpf[index].z1 + lpf[index].feedback * lpf[index].delta);
    }

    inline double tickOnePole(int index, double s) noexcept {
        s = s * lpf[index].gamma + lpf[index].feedback + lpf[index].epsilon * getFeedbackOutput(index);
        double vn = (lpf[index].a0 * s - lpf[index].z1) * lpf[index].alpha;
        double out = vn + lpf[index].z1;
        lpf[index].z1 = vn + out;
        return out;
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_OBERHEIMMOOGLADDER_H
