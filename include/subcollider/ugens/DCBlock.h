/**
 * @file DCBlock.h
 * @brief High-pass DC blocking filter UGen.
 *
 * Implements a simple leaky integrator:
 *   y[n] = x[n] - x[n-1] + R * y[n-1]
 *
 * R is derived from the desired cutoff frequency (default ~20 Hz).
 */

#ifndef SUBCOLLIDER_UGENS_DCBLOCK_H
#define SUBCOLLIDER_UGENS_DCBLOCK_H

#include "../types.h"
#include <cmath>

namespace subcollider {
namespace ugens {

struct DCBlock {
    Sample sampleRate;
    Sample cutoff;
    Sample coeff;
    Sample prevInputL;
    Sample prevOutputL;
    Sample prevInputR;
    Sample prevOutputR;

    void init(Sample sr = DEFAULT_SAMPLE_RATE, Sample cutoffHz = 20.0f) noexcept {
        sampleRate = sr;
        prevInputL = 0.0f;
        prevOutputL = 0.0f;
        prevInputR = 0.0f;
        prevOutputR = 0.0f;
        setCutoff(cutoffHz);
    }

    void setCutoff(Sample cutoffHz) noexcept {
        cutoff = cutoffHz;
        if (cutoff <= 0.0f) {
            coeff = 0.0f;
            return;
        }
        Sample normalized = cutoff / sampleRate;
        // Ensure stability and clamp to Nyquist/2 guard.
        if (normalized > 0.25f) {
            normalized = 0.25f;
        }
        coeff = std::exp(-2.0f * PI * normalized);
    }

    inline Sample tick(Sample input) noexcept {
        Sample output = input - prevInputL + coeff * prevOutputL;
        prevInputL = input;
        prevOutputL = output;
        return output;
    }

    inline Stereo tickStereo(Sample inputL, Sample inputR) noexcept {
        Stereo out;
        out.left = inputL - prevInputL + coeff * prevOutputL;
        out.right = inputR - prevInputR + coeff * prevOutputR;
        prevInputL = inputL;
        prevOutputL = out.left;
        prevInputR = inputR;
        prevOutputR = out.right;
        return out;
    }

    void process(Sample* samples, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            samples[i] = tick(samples[i]);
        }
    }

    void processStereo(Sample* samplesL, Sample* samplesR, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            Stereo out = tickStereo(samplesL[i], samplesR[i]);
            samplesL[i] = out.left;
            samplesR[i] = out.right;
        }
    }

    void reset() noexcept {
        prevInputL = 0.0f;
        prevOutputL = 0.0f;
        prevInputR = 0.0f;
        prevOutputR = 0.0f;
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_DCBLOCK_H
