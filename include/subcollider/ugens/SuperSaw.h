/**
 * @file SuperSaw.h
 * @brief SuperSaw synthesizer UGen - unison saw oscillators with vibrato and filtering.
 *
 * SuperSaw is a composite UGen that combines multiple detuned saw oscillators
 * with vibrato, stereo spreading, drive, and lowpass filtering for a rich,
 * powerful sound reminiscent of classic supersaw patches.
 */

#ifndef SUBCOLLIDER_UGENS_SUPERSAW_H
#define SUBCOLLIDER_UGENS_SUPERSAW_H

#include "../types.h"
#include "SawDPW.h"
#include "LFTri.h"
#include "Pan2.h"
#include "EnvelopeADSR.h"
#include "RKSimulationMoogLadder.h"
#include "XLine.h"
#include <cmath>
#include <random>

namespace subcollider {
namespace ugens {

/**
 * @brief SuperSaw synthesizer with 7 unison voices.
 *
 * The SuperSaw generates a rich, detuned sound using:
 * - 7 saw oscillators with individual detuning and vibrato
 * - Stereo spreading via panning
 * - Drive/saturation
 * - Lowpass filtering with envelope modulation
 * - ADSR amplitude envelope
 *
 * Usage:
 * @code
 * SuperSaw supersaw;
 * supersaw.init(48000.0f);
 * supersaw.setFrequency(440.0f);
 * supersaw.setAttack(0.01f);
 * supersaw.setDecay(0.1f);
 * supersaw.setSustain(0.7f);
 * supersaw.setRelease(0.5f);
 * supersaw.setDetune(0.2f);
 * supersaw.setSpread(0.6f);
 *
 * supersaw.gate(1.0f);  // Note on
 * Stereo output = supersaw.tick();
 * supersaw.gate(0.0f);  // Note off
 * @endcode
 */
struct SuperSaw {
    static constexpr int NUM_VOICES = 7;

    // Voice components
    struct Voice {
        SawDPW saw;
        LFTri vibrato;
        Pan2 panner;
        Sample detuneOffset;     // Detune offset for this voice
        Sample vibratoPhase;     // Random vibrato phase
        Sample sawPhase;         // Random saw phase
    };

    Voice voices[NUM_VOICES];

    // Shared components
    EnvelopeADSR envelope;
    RKSimulationMoogLadder filter;
    XLine filterLine;

    // Parameters
    Sample frequency;
    Sample vibrRate;
    Sample vibrDepth;
    Sample drive;
    Sample detune;
    Sample spread;
    Sample lpenv;          // Lowpass envelope amount (octaves)
    Sample lpa;            // Lowpass attack time
    Sample cutoff;
    Sample duration;       // Duration for XLine

    Sample sampleRate;

    // Random number generator for initialization
    std::mt19937 rng;
    std::uniform_real_distribution<Sample> dist;

    /**
     * @brief Initialize the SuperSaw.
     * @param sr Sample rate in Hz (default: 48000)
     * @param seed Random seed for voice initialization (default: 42)
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE, unsigned int seed = 42) noexcept {
        sampleRate = sr;

        // Initialize random number generator
        rng.seed(seed);
        dist = std::uniform_real_distribution<Sample>(0.0f, 1.0f);

        // Default parameters
        frequency = 400.0f;
        vibrRate = 6.0f;
        vibrDepth = 0.3f;
        drive = 1.5f;
        detune = 0.2f;
        spread = 0.6f;
        lpenv = 0.0f;
        lpa = 0.0f;
        cutoff = 20000.0f;
        duration = 1.0f;

        // Initialize envelope
        envelope.init(sr);
        envelope.setAttack(0.01f);
        envelope.setDecay(0.1f);
        envelope.setSustain(0.7f);
        envelope.setRelease(0.3f);

        // Initialize filter
        filter.init(sr);
        filter.setCutoff(cutoff);
        filter.setResonance(0.1f);
        filter.setDrive(drive);

        // Initialize filter line
        filterLine.init(sr);
        filterLine.set(cutoff, cutoff, 1.0f);

        // Initialize voices
        for (int i = 0; i < NUM_VOICES; ++i) {
            // Random phases
            voices[i].vibratoPhase = dist(rng);
            voices[i].sawPhase = dist(rng);

            // Calculate detune offset for this voice
            // Spread voices from -detune/2 to +detune/2 in semitones
            voices[i].detuneOffset = ((static_cast<Sample>(i) / (NUM_VOICES - 1)) - 0.5f) * detune;

            // Initialize oscillators
            voices[i].saw.init(sr, voices[i].sawPhase * 2.0f - 1.0f);
            voices[i].vibrato.init(sr, voices[i].vibratoPhase * 4.0f);
            voices[i].vibrato.setFrequency(vibrRate);
        }
    }

    /**
     * @brief Set the base frequency.
     * @param freq Frequency in Hz
     */
    void setFrequency(Sample freq) noexcept {
        frequency = freq;
    }

    /**
     * @brief Set vibrato rate.
     * @param rate Vibrato rate in Hz
     */
    void setVibratoRate(Sample rate) noexcept {
        vibrRate = rate;
        for (int i = 0; i < NUM_VOICES; ++i) {
            voices[i].vibrato.setFrequency(rate);
        }
    }

    /**
     * @brief Set vibrato depth.
     * @param depth Vibrato depth in semitones
     */
    void setVibratoDepth(Sample depth) noexcept {
        vibrDepth = depth;
    }

    /**
     * @brief Set drive amount.
     * @param d Drive amount (1.0 = no drive)
     */
    void setDrive(Sample d) noexcept {
        drive = d;
        filter.setDrive(d);
    }

    /**
     * @brief Set detune amount.
     * @param det Detune amount in semitones
     */
    void setDetune(Sample det) noexcept {
        detune = det;
        // Recalculate detune offsets
        for (int i = 0; i < NUM_VOICES; ++i) {
            voices[i].detuneOffset = ((static_cast<Sample>(i) / (NUM_VOICES - 1)) - 0.5f) * detune;
        }
    }

    /**
     * @brief Set stereo spread.
     * @param s Spread amount [0, 1]
     */
    void setSpread(Sample s) noexcept {
        spread = clamp(s, 0.0f, 1.0f);
    }

    /**
     * @brief Set lowpass envelope amount.
     * @param lpe Envelope amount in octaves
     */
    void setLpEnv(Sample lpe) noexcept {
        lpenv = lpe;
    }

    /**
     * @brief Set lowpass attack time.
     * @param lpaTime Attack time in seconds
     */
    void setLpAttack(Sample lpaTime) noexcept {
        lpa = lpaTime;
    }

    /**
     * @brief Set filter cutoff frequency.
     * @param c Cutoff frequency in Hz
     */
    void setCutoff(Sample c) noexcept {
        cutoff = c;
    }

    /**
     * @brief Set duration for XLine.
     * @param dur Duration in seconds
     */
    void setDuration(Sample dur) noexcept {
        duration = dur;
    }

    /**
     * @brief Set ADSR attack time.
     * @param time Attack time in seconds
     */
    void setAttack(Sample time) noexcept {
        envelope.setAttack(time);
    }

    /**
     * @brief Set ADSR decay time.
     * @param time Decay time in seconds
     */
    void setDecay(Sample time) noexcept {
        envelope.setDecay(time);
    }

    /**
     * @brief Set ADSR sustain level.
     * @param level Sustain level [0, 1]
     */
    void setSustain(Sample level) noexcept {
        envelope.setSustain(level);
    }

    /**
     * @brief Set ADSR release time.
     * @param time Release time in seconds
     */
    void setRelease(Sample time) noexcept {
        envelope.setRelease(time);
    }

    /**
     * @brief Set gate value.
     * @param g Gate value (>0 = on, 0 = off)
     */
    void gate(Sample g) noexcept {
        // Check if we need to trigger the filter line (gate going high)
        bool wasActive = envelope.isActive();

        envelope.gate(g);

        // Trigger filter line when gate goes high (transition from off to on)
        if (g > 0.0f && !wasActive) {
            // Calculate target cutoff with envelope
            Sample targetCutoff = cutoff * std::pow(2.0f, lpenv);
            targetCutoff = clamp(targetCutoff, cutoff, 18000.0f);

            // Set up XLine for filter modulation
            if (lpa > 0.0f) {
                filterLine.set(cutoff, targetCutoff, duration * lpa);
            } else {
                filterLine.set(cutoff, cutoff, 0.001f);
            }
        }
    }

    /**
     * @brief Check if voice is active.
     * @return True if envelope is active
     */
    bool isActive() const noexcept {
        return envelope.isActive();
    }

    /**
     * @brief Generate single stereo sample.
     * @return Stereo output sample
     */
    inline Stereo tick() noexcept {
        // Get envelope value
        Sample env = envelope.tick();

        // Use cutoff parameter directly for real-time control
        filter.setCutoff(cutoff);

        // Mix all voices
        Stereo mix(0.0f, 0.0f);

        for (int i = 0; i < NUM_VOICES; ++i) {
            // Calculate vibrato modulation
            Sample vibratoValue = voices[i].vibrato.tick();
            // Convert vibrato depth from semitones to frequency ratio
            Sample vibratoMod = std::pow(2.0f, (vibratoValue * vibrDepth) / 12.0f);

            // Calculate detuned frequency
            Sample detuneMod = std::pow(2.0f, voices[i].detuneOffset / 12.0f);
            Sample voiceFreq = frequency * vibratoMod * detuneMod;

            voices[i].saw.setFrequency(voiceFreq);

            // Generate saw wave
            Sample sawSample = voices[i].saw.tick();

            // Pan the voice (alternate left/right based on index)
            Sample panPos = ((i % 2) * 2.0f - 1.0f) * spread;
            Stereo panned = voices[i].panner.process(sawSample, panPos);

            // Accumulate
            mix.left += panned.left;
            mix.right += panned.right;
        }

        // Normalize by sqrt(NUM_VOICES) for equal power
        Sample norm = 1.0f / std::sqrt(static_cast<Sample>(NUM_VOICES));
        mix.left *= norm;
        mix.right *= norm;

        // Filter stereo signal
        Sample filtered = filter.tick(mix.left + mix.right) / 2.0f;

        // Apply envelope after filtering
        filtered *= env;

        // Return mono filtered signal in both channels
        // (or could keep stereo - adjust as needed)
        return Stereo(filtered, filtered);
    }

    /**
     * @brief Process a block of stereo samples.
     * @param outputL Left channel output buffer
     * @param outputR Right channel output buffer
     * @param numSamples Number of samples to generate
     */
    void process(Sample* outputL, Sample* outputR, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            Stereo sample = tick();
            outputL[i] = sample.left;
            outputR[i] = sample.right;
        }
    }

    /**
     * @brief Reset the SuperSaw to idle state.
     */
    void reset() noexcept {
        envelope.reset();
        for (int i = 0; i < NUM_VOICES; ++i) {
            voices[i].saw.reset();
        }
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_SUPERSAW_H
