/**
 * @file ImprovedMoogLadder.h
 * @brief Improved Moog Ladder filter UGen.
 *
 * Copyright 2012 Stefano D'Angelo <zanga.mail@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef SUBCOLLIDER_UGENS_IMPROVEDMOOGLADDER_H
#define SUBCOLLIDER_UGENS_IMPROVEDMOOGLADDER_H

#include "../types.h"
#include <cmath>
#include <cstring>

namespace subcollider {
namespace ugens {

/**
 * @brief Improved Moog Ladder filter.
 *
 * Based on a reference implementation of an algorithm developed by
 * Stefano D'Angelo and Vesa Valimaki, presented in a paper published at ICASSP in 2013.
 * This improved model is based on a circuit analysis and compared against a reference
 * Ngspice simulation. In the paper, it is noted that this particular model is
 * more accurate in preserving the self-oscillating nature of the real filter.
 *
 * References: "An Improved Virtual Analog Model of the Moog Ladder Filter"
 *
 * Usage:
 * @code
 * ImprovedMoogLadder filter;
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
struct ImprovedMoogLadder {
    /// Sample rate in Hz
    Sample sampleRate;

    /// Cutoff frequency in Hz
    Sample cutoff;

    /// Resonance [0, 1]
    Sample resonance;

    /// Makeup gain to compensate for level loss at low cutoff
    double makeupGain;

    /**
     * @brief Initialize the filter.
     * @param sr Sample rate in Hz (default: 48000)
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE) noexcept {
        sampleRate = sr;
        std::memset(V, 0, sizeof(V));
        std::memset(dV, 0, sizeof(dV));
        std::memset(tV, 0, sizeof(tV));

        drive = 1.0;
        x = 0.0;
        g = 0.0;
        makeupGain = 1.0;

        setCutoff(1000.0f);
        setResonance(0.1f);
    }

    /**
     * @brief Set the cutoff frequency.
     * @param c Cutoff frequency in Hz
     */
    void setCutoff(Sample c) noexcept {
        // Limit cutoff to avoid instability when the bilinear transform mapping
        // approaches the Nyquist frequency (x >= 1 makes g negative and blows up)
        Sample maxCutoff = (sampleRate / PI) * 0.99f;
        cutoff = clamp(c, 0.0f, maxCutoff);

        x = (PI * cutoff) / sampleRate;
        g = 4.0 * PI * VT * cutoff * (1.0 - x) / (1.0 + x);

        // Gentle compensation so output loudness stays more consistent at low cutoff
        double nyquist = sampleRate * 0.5;
        double norm = nyquist > 0.0 ? static_cast<double>(cutoff) / nyquist : 0.0;
        norm = norm < 0.0 ? 0.0 : (norm > 1.0 ? 1.0 : norm);
        // Reciprocal curve with floor to avoid extreme boosts
        makeupGain = 1.0 / (0.2 + norm);
        if (makeupGain > 8.0) makeupGain = 8.0;
    }

    /**
     * @brief Set the resonance.
     * @param r Resonance [0, 1] (internally scaled to [0, 4])
     */
    void setResonance(Sample r) noexcept {
        resonance = r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r);
    }

    /**
     * @brief Set the input drive/saturation amount.
     * @param d Drive multiplier (1.0 = no drive)
     */
    void setDrive(Sample d) noexcept {
        drive = d < 0.0 ? 0.0 : static_cast<double>(d);
    }

    /**
     * @brief Generate single filtered sample.
     * @param input Input sample
     * @return Filtered sample
     */
    inline Sample tick(Sample input) noexcept {
        double dV0, dV1, dV2, dV3;
        double res = resonance * 4.0; // Scale resonance to [0, 4]

        dV0 = -g * (std::tanh((drive * input + res * V[3]) / (2.0 * VT)) + tV[0]);
        V[0] += (dV0 + dV[0]) / (2.0 * sampleRate);
        dV[0] = dV0;
        tV[0] = std::tanh(V[0] / (2.0 * VT));

        dV1 = g * (tV[0] - tV[1]);
        V[1] += (dV1 + dV[1]) / (2.0 * sampleRate);
        dV[1] = dV1;
        tV[1] = std::tanh(V[1] / (2.0 * VT));

        dV2 = g * (tV[1] - tV[2]);
        V[2] += (dV2 + dV[2]) / (2.0 * sampleRate);
        dV[2] = dV2;
        tV[2] = std::tanh(V[2] / (2.0 * VT));

        dV3 = g * (tV[2] - tV[3]);
        V[3] += (dV3 + dV[3]) / (2.0 * sampleRate);
        dV[3] = dV3;
        tV[3] = std::tanh(V[3] / (2.0 * VT));

        return static_cast<Sample>(V[3] * makeupGain);
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
        std::memset(V, 0, sizeof(V));
        std::memset(dV, 0, sizeof(dV));
        std::memset(tV, 0, sizeof(tV));
    }

private:
    // Thermal voltage (26 milliwatts at room temperature)
    static constexpr double VT = 0.312;

    double V[4];
    double dV[4];
    double tV[4];

    double x;
    double g;
    double drive;
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_IMPROVEDMOOGLADDER_H
