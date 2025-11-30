/**
 * @file Tape.h
 * @brief Tape saturation UGen with envelope follower and DC blocking.
 */

#ifndef SUBCOLLIDER_UGENS_TAPE_H
#define SUBCOLLIDER_UGENS_TAPE_H

#include "../types.h"
#include <cmath>

namespace subcollider {
namespace ugens {

/// Simple envelope follower used to drive the tape bias.
struct TapeFollower {
    Sample attackCoeff;
    Sample releaseCoeff;
    Sample state;

    void init(Sample sr = DEFAULT_SAMPLE_RATE,
              Sample attackTime = 0.001f,
              Sample releaseTime = 0.020f) noexcept {
        attackCoeff = std::exp(-1.0f / (attackTime * sr));
        releaseCoeff = std::exp(-1.0f / (releaseTime * sr));
        state = 0.0f;
    }

    inline Sample process(Sample x) noexcept {
        Sample absX = std::fabs(x);
        if (absX > state) {
            state = attackCoeff * state + (1.0f - attackCoeff) * absX;
        } else {
            state = releaseCoeff * state + (1.0f - releaseCoeff) * absX;
        }
        return state;
    }

    void reset() noexcept {
        state = 0.0f;
    }
};

/// One-pole DC blocking filter.
struct TapeDcBlocker {
    Sample prevInput;
    Sample prevOutput;
    Sample gain;

    void init(Sample coeff = 0.99f) noexcept {
        prevInput = 0.0f;
        prevOutput = 0.0f;
        gain = coeff;
    }

    inline Sample process(Sample input) noexcept {
        Sample output = input - prevInput + (gain * prevOutput);
        prevInput = input;
        prevOutput = output;
        return output;
    }

    void reset() noexcept {
        prevInput = 0.0f;
        prevOutput = 0.0f;
    }
};

/// Tape saturation effect built from the legacy TapeFX implementation.
struct Tape {
    Sample sampleRate;
    Sample bias;
    Sample pregain;
    TapeFollower follower;
    TapeDcBlocker dcLeft;
    TapeDcBlocker dcRight;
    Sample followerValue;

    void init(Sample sr = DEFAULT_SAMPLE_RATE) noexcept {
        sampleRate = sr;
        bias = 0.0f;
        pregain = 1.0f;
        follower.init(sr);
        dcLeft.init();
        dcRight.init();
        followerValue = 0.0f;
    }

    void setBias(Sample b) noexcept {
        bias = b;
    }

    void setPregain(Sample pg) noexcept {
        pregain = pg;
    }

    Sample getFollowerValue() const noexcept {
        return followerValue;
    }

    inline Sample tick(Sample input) noexcept {
        Sample out = input * pregain;
        followerValue = follower.process(out);
        out = std::tanh(out + (followerValue * bias));
        out = dcLeft.process(out);
        out = std::tanh(out);
        return out;
    }

    inline Stereo tickStereo(Sample inputL, Sample inputR) noexcept {
        Stereo out;
        out.left = inputL * pregain;
        out.right = inputR * pregain;

        followerValue = follower.process(out.left);

        out.left = std::tanh(out.left + (followerValue * bias));
        out.right = std::tanh(out.right + (followerValue * bias));

        out.left = dcLeft.process(out.left);
        out.right = dcRight.process(out.right);

        out.left = std::tanh(out.left);
        out.right = std::tanh(out.right);

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
        follower.reset();
        dcLeft.reset();
        dcRight.reset();
        followerValue = 0.0f;
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_TAPE_H
