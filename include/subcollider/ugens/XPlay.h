/**
 * @file XPlay.h
 * @brief Crossfading buffer player inspired by a SuperCollider SynthDef.
 *
 * Plays a looped segment of a buffer with dual-phase crossfading,
 * optional reverse playback, envelope gating, and low-frequency
 * modulation for balance and level.
 */

#ifndef SUBCOLLIDER_UGENS_XPLAY_H
#define SUBCOLLIDER_UGENS_XPLAY_H

#include "../Buffer.h"
#include "../types.h"
#include "Balance2.h"
#include "BufRd.h"
#include "DBAmp.h"
#include "LagLinear.h"
#include "LFNoise2.h"
#include "Wrap.h"
#include "XFade2.h"
#include "LinLin.h"
#include "EnvelopeADSR.h"
#include <algorithm>
#include <cmath>

namespace subcollider {
namespace ugens {

/**
 * @brief Crossfading buffer loop player with linear lag crossfade and modulation.
 *
 * Parameters:
 * - start/end: normalized loop points [0..1]
 * - rate: playback rate multiplier
 * - fade: crossfade/envelope time in seconds
 * - gate: ADSR gate (attack/release = fade, decay=0, sustain=1)
 *
 * The phasor traverses a 2x loop window; the second half crossfades to a
 * second read head offset by the loop size. Toggle detection is done with
 * simple comparisons (no Changed/Select). Linear lag replaces VarLag.
 */
struct XPlay {
    enum class PlayMode : uint8_t {
        Loop = 0,
        Bounce = 1
    };

    const Buffer* buffer = nullptr;
    Sample sampleRate = DEFAULT_SAMPLE_RATE;
    Sample start = 0.0f;
    Sample end = 1.0f;
    Sample rate = 1.0f;
    Sample fadeTime = 0.05f;
    Sample gateValue = 1.0f;
    PlayMode playMode = PlayMode::Loop;

    // Derived state
    Sample frames = 0.0f;
    Sample loopStart = 0.0f;
    Sample loopEnd = 0.0f;
    Sample loopSize = 0.0f;
    Sample phasor = 0.0f;
    bool isReverse = false;
    bool inSecondHalf = false;

    // UGens
    BufRd reader;
    Wrap wrapper;
    XFade2 xfader;
    LagLinear fadeLag;
    EnvelopeADSR env;
    DBAmp dbAmp;

    /**
     * @brief Initialize XPlay.
     * @param sr Sample rate in Hz
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE) noexcept {
        sampleRate = sr;
        reader.init();
        fadeLag.init(sr, -1.0f, fadeTime);
        env.init(sr);
        env.setAttack(fadeTime);
        env.setDecay(0.0f);
        env.setSustain(1.0f);
        env.setRelease(fadeTime);
        env.setDoneAction(EnvelopeADSR::DoneAction::ActionFree);
        env.gate(gateValue);
        updateLoopBounds();
    }

    /**
     * @brief Set playback buffer.
     */
    void setBuffer(const Buffer* buf) noexcept {
        buffer = buf;
        reader.setBuffer(buf);
        updateLoopBounds();
    }

    /**
     * @brief Set start/end points (normalized 0..1).
     */
    void setStartEnd(Sample startNorm, Sample endNorm, bool preservePhasor = false) noexcept {
        Sample clampedStart = clamp(startNorm, 0.0f, 1.0f);
        Sample clampedEnd = clamp(endNorm, 0.0f, 1.0f);
        if (std::abs(clampedStart - start) < 1e-9f &&
            std::abs(clampedEnd - end) < 1e-9f) {
            return;
        }
        start = clampedStart;
        end = clampedEnd;
        updateLoopBounds(!preservePhasor);
    }

    /**
     * @brief Set playback rate multiplier.
     */
    void setRate(Sample newRate) noexcept { rate = newRate; }

    /**
     * @brief Set play mode (Loop or Bounce).
     */
    void setPlayMode(PlayMode mode) noexcept { playMode = mode; }

    /**
     * @brief Set crossfade/envelope time in seconds.
     */
    void setFadeTime(Sample timeSeconds) noexcept {
        fadeTime = std::max(0.0f, timeSeconds);
        fadeLag.setTime(fadeTime);
        env.setAttack(fadeTime);
        env.setRelease(fadeTime);
    }

    /**
     * @brief Set gate (starts/stops the ADSR).
     */
    void setGate(Sample gate) noexcept {
        gateValue = gate;
        env.gate(gate);
    }

    /**
     * @brief Get whether envelope is done (idle).
     */
    bool isDone() const noexcept { return env.isDone(); }

    /**
     * @brief Reset phasor to loop start.
     */
    void resetPhasor() noexcept { phasor = loopStart; }

    /**
     * @brief Process one stereo sample.
     * @return Stereo output
     */
    inline Stereo tick() noexcept {
        if (buffer == nullptr || !buffer->isValid() || loopSize <= 0.0f) {
            return Stereo();
        }

        // Compute effective rate with buffer rate scaling and reverse flag
        Sample rateScale = buffer->sampleRate > 0.0f ? buffer->sampleRate / sampleRate : 1.0f;
        Sample effectiveRate = rate * rateScale * (isReverse ? -1.0f : 1.0f);

        Sample currentPhasor = phasor;

        // Fade toggle when crossing half-point
        bool nowSecondHalf = currentPhasor >= (loopStart + loopSize);
        if (nowSecondHalf != inSecondHalf) {
            inSecondHalf = nowSecondHalf;
        }
        Sample fadeTarget = inSecondHalf ? 1.0f : -1.0f;  // map to [-1,1]
        Sample fadeCtrl = fadeLag.tick(fadeTarget);

        // Positions for two read heads
        Sample pos1 = 0.0f;
        Sample pos2 = 0.0f;

        if (playMode == PlayMode::Loop) {
            pos1 = wrapper.process(currentPhasor, 0.0f, frames);
            pos2 = wrapper.process(currentPhasor - loopSize, 0.0f, frames);
        } else { // Bounce
            Sample twoSize = loopSize * 2.0f;
            Sample phaseWithin = currentPhasor - loopStart;
            // Ensure positive modulo inside [0, 2*loopSize)
            phaseWithin = std::fmod(phaseWithin, twoSize);
            if (phaseWithin < 0.0f) {
                phaseWithin += twoSize;
            }

            auto pingpong = [&](Sample p) -> Sample {
                return (p <= loopSize) ? p : (twoSize - p);
            };

            // Head 1 pingpongs, head 2 mirrors to travel opposite direction at turns
            Sample p1 = pingpong(phaseWithin);
            Sample p2 = pingpong(twoSize - phaseWithin);

            pos1 = loopStart + p1;
            pos2 = loopStart + p2;
        }

        Stereo sig1 = reader.tickStereo(pos1);
        Stereo sig2 = reader.tickStereo(pos2);

        // Crossfade
        Stereo snd = xfader.process(sig1, sig2, fadeCtrl);
        Sample envVal = env.tick();
        snd.left *= envVal;
        snd.right *= envVal;

        // Advance phasor over 2x loop window
        currentPhasor += effectiveRate;
        Sample loopWindowStart = loopStart;
        Sample loopWindowEnd = loopStart + (loopSize * 2.0f);
        if (currentPhasor >= loopWindowEnd) {
            currentPhasor -= (loopSize * 2.0f);
            inSecondHalf = false;
        } else if (currentPhasor < loopWindowStart) {
            currentPhasor += (loopSize * 2.0f);
            inSecondHalf = currentPhasor >= (loopStart + loopSize);
        }
        phasor = currentPhasor;

        return snd;
    }

    /**
     * @brief Process a block of stereo samples.
     */
    void process(Sample* outL, Sample* outR, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            Stereo s = tick();
            outL[i] = s.left;
            outR[i] = s.right;
        }
    }

private:
    void updateLoopBounds(bool resetPhasor = true) noexcept {
        frames = (buffer && buffer->isValid()) ? static_cast<Sample>(buffer->numSamples) : 0.0f;
        loopStart = std::min(start, end) * frames;
        loopEnd = std::max(start, end) * frames;
        loopSize = std::max(0.0f, loopEnd - loopStart);
        isReverse = start > end;
        if (resetPhasor || loopSize <= 0.0f) {
            phasor = loopStart;
            inSecondHalf = false;
            return;
        }

        Sample loopWindowStart = loopStart;
        Sample loopWindowEnd = loopStart + (loopSize * 2.0f);

        // Keep phasor inside the new window
        while (phasor < loopWindowStart) {
            phasor += (loopSize * 2.0f);
        }
        while (phasor >= loopWindowEnd) {
            phasor -= (loopSize * 2.0f);
        }
        inSecondHalf = phasor >= (loopStart + loopSize);
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_XPLAY_H
