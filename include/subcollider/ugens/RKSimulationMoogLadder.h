/**
 * @file RKSimulationMoogLadder.h
 * @brief Runge-Kutta Simulation Moog Ladder filter UGen.
 *
 * Copyright (c) 2015, Miller Puckette. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SUBCOLLIDER_UGENS_RKSIMULATIONMOOGLADDER_H
#define SUBCOLLIDER_UGENS_RKSIMULATIONMOOGLADDER_H

#include "../types.h"
#include <cmath>
#include <cstring>

namespace subcollider {
namespace ugens {

/**
 * @brief Runge-Kutta Simulation Moog Ladder filter.
 *
 * Imitates a Moog resonant filter by Runge-Kutta numerical integration of
 * a differential equation approximately describing the dynamics of the circuit.
 *
 * The differential equations are:
 *   y1' = k * (S(x - r * y4) - S(y1))
 *   y2' = k * (S(y1) - S(y2))
 *   y3' = k * (S(y2) - S(y3))
 *   y4' = k * (S(y3) - S(y4))
 *
 * where k controls the cutoff frequency, r is feedback (<= 4 for stability), 
 * and S(x) is a saturation function.
 *
 * Usage:
 * @code
 * RKSimulationMoogLadder filter;
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
struct RKSimulationMoogLadder {
    /// Sample rate in Hz
    Sample sampleRate;

    /// Cutoff frequency in Hz
    Sample cutoff;

    /// Resonance [0, 1]
    Sample resonance;

    /// Input drive multiplier
    double drive;

    /// Makeup gain to compensate level loss at low cutoff
    double makeupGain;

    /**
     * @brief Initialize the filter.
     * @param sr Sample rate in Hz (default: 48000)
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE) noexcept {
        sampleRate = sr;
        std::memset(state, 0, sizeof(state));

        saturation = 3.0;
        saturationInv = 1.0 / saturation;
        cutoffCoeff = 0.0;
        drive = 1.0;
        makeupGain = 1.0;

        oversampleFactor = 1;
        stepSize = 1.0 / (oversampleFactor * sampleRate);

        setCutoff(1000.0f);
        setResonance(0.1f);
    }

    /**
     * @brief Set the cutoff frequency.
     * @param c Cutoff frequency in Hz
     */
    void setCutoff(Sample c) noexcept {
        cutoff = c;
        cutoffCoeff = 2.0 * PI * cutoff;

        // Simple compensation to keep output level more consistent at low cutoff
        double nyquist = sampleRate * 0.5;
        double norm = nyquist > 0.0 ? static_cast<double>(cutoff) / nyquist : 0.0;
        norm = norm < 0.0 ? 0.0 : (norm > 1.0 ? 1.0 : norm);
        makeupGain = 1.0 / (0.2 + norm);
        if (makeupGain > 8.0) makeupGain = 8.0;
    }

    /**
     * @brief Set the resonance.
     * @param r Resonance [0, 1] (internally scaled to [0, 10])
     */
    void setResonance(Sample r) noexcept {
        resonance = r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r);
    }

    /**
     * @brief Set input drive (pre-saturation gain).
     * @param d Drive multiplier (1.0 = unity)
     */
    void setDrive(Sample d) noexcept {
        drive = d < 0.0f ? 0.0f : static_cast<double>(d);
    }

    /**
     * @brief Set oversampling factor.
     * @param factor Oversampling factor (1 = no oversampling, 2 = 2x, 4 = 4x, etc.)
     */
    void setOversampleFactor(int factor) noexcept {
        oversampleFactor = factor < 1 ? 1 : factor;
        stepSize = 1.0 / (oversampleFactor * sampleRate);
    }

    /**
     * @brief Generate single filtered sample.
     * @param input Input sample
     * @return Filtered sample
     */
    inline Sample tick(Sample input) noexcept {
        for (int j = 0; j < oversampleFactor; j++) {
            rungekutteSolver(input);
        }

        return static_cast<Sample>(state[3] * makeupGain);
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
    }

private:
    double state[4];
    double saturation;
    double saturationInv;
    double cutoffCoeff;
    int oversampleFactor;
    double stepSize;

    inline double clip(double value) noexcept {
        double v2 = (value * saturationInv > 1.0 ? 1.0 :
                    (value * saturationInv < -1.0 ? -1.0 :
                     value * saturationInv));
        return saturation * (v2 - (1.0 / 3.0) * v2 * v2 * v2);
    }

    void calculateDerivatives(double input, double* dstate, double* currentState) noexcept {
        double res = resonance * 10.0; // Scale resonance to [0, 10]

        double satstate0 = clip(currentState[0]);
        double satstate1 = clip(currentState[1]);
        double satstate2 = clip(currentState[2]);

        dstate[0] = cutoffCoeff * (clip(input * drive - res * currentState[3]) - satstate0);
        dstate[1] = cutoffCoeff * (satstate0 - satstate1);
        dstate[2] = cutoffCoeff * (satstate1 - satstate2);
        dstate[3] = cutoffCoeff * (satstate2 - clip(currentState[3]));
    }

    void rungekutteSolver(double input) noexcept {
        double deriv1[4], deriv2[4], deriv3[4], deriv4[4], tempState[4];

        calculateDerivatives(input, deriv1, state);

        for (int i = 0; i < 4; i++)
            tempState[i] = state[i] + 0.5 * stepSize * deriv1[i];

        calculateDerivatives(input, deriv2, tempState);

        for (int i = 0; i < 4; i++)
            tempState[i] = state[i] + 0.5 * stepSize * deriv2[i];

        calculateDerivatives(input, deriv3, tempState);

        for (int i = 0; i < 4; i++)
            tempState[i] = state[i] + stepSize * deriv3[i];

        calculateDerivatives(input, deriv4, tempState);

        for (int i = 0; i < 4; i++)
            state[i] += (1.0 / 6.0) * stepSize * (deriv1[i] + 2.0 * deriv2[i] + 2.0 * deriv3[i] + deriv4[i]);
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_RKSIMULATIONMOOGLADDER_H
