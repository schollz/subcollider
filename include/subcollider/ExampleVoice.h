/**
 * @file ExampleVoice.h
 * @brief Example synthesizer voice combining multiple UGens.
 *
 * ExampleVoice demonstrates how to combine SinOsc, EnvelopeAR, and
 * LFNoise2 into a complete synthesizer voice. This serves as a
 * template for building more complex instruments.
 */

#ifndef SUBCOLLIDER_EXAMPLE_VOICE_H
#define SUBCOLLIDER_EXAMPLE_VOICE_H

#include "types.h"
#include "ugens/SinOsc.h"
#include "ugens/EnvelopeAR.h"
#include "ugens/LFNoise2.h"

namespace subcollider {

/**
 * @brief Example synthesizer voice combining multiple UGens.
 *
 * This voice demonstrates:
 * - Sine wave oscillator for the main tone
 * - Attack-release envelope for amplitude
 * - LFNoise2 for vibrato modulation
 *
 * All processing is inline and heap-free, suitable for embedded use.
 *
 * Usage:
 * @code
 * ExampleVoice voice;
 * voice.init(48000.0f);
 * voice.setFrequency(440.0f);
 * voice.trigger();
 *
 * // Per-sample processing
 * float sample = voice.tick();
 *
 * // Block processing
 * AudioBuffer<64> buffer;
 * voice.process(buffer.data, 64);
 * @endcode
 */
struct ExampleVoice {
    /// Main oscillator
    ugens::SinOsc osc;

    /// Amplitude envelope
    ugens::EnvelopeAR env;

    /// Vibrato modulator
    ugens::LFNoise2 vibrato;

    /// Base frequency in Hz
    Sample baseFrequency;

    /// Vibrato depth (semitones)
    Sample vibratoDepth;

    /// Master amplitude [0, 1]
    Sample amplitude;

    /// Sample rate in Hz
    Sample sampleRate;

    /**
     * @brief Initialize the voice.
     * @param sr Sample rate in Hz (default: 48000)
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE) noexcept {
        sampleRate = sr;

        // Initialize all UGens
        osc.init(sr);
        env.init(sr);
        vibrato.init(sr);

        // Set default parameters
        baseFrequency = 440.0f;
        vibratoDepth = 0.0f;  // No vibrato by default
        amplitude = 0.5f;

        // Configure envelope with musical defaults
        env.setAttack(0.01f);   // 10ms attack
        env.setRelease(0.3f);   // 300ms release

        // Configure vibrato
        vibrato.setFrequency(5.0f);  // 5 Hz vibrato rate
    }

    /**
     * @brief Set voice frequency (control-rate parameter update).
     * @param freq Frequency in Hz
     */
    void setFrequency(Sample freq) noexcept {
        baseFrequency = freq;
        osc.setFrequency(freq);
    }

    /**
     * @brief Set attack time (control-rate parameter update).
     * @param time Attack time in seconds
     */
    void setAttack(Sample time) noexcept {
        env.setAttack(time);
    }

    /**
     * @brief Set release time (control-rate parameter update).
     * @param time Release time in seconds
     */
    void setRelease(Sample time) noexcept {
        env.setRelease(time);
    }

    /**
     * @brief Set vibrato depth (control-rate parameter update).
     * @param semitones Vibrato depth in semitones
     */
    void setVibratoDepth(Sample semitones) noexcept {
        vibratoDepth = semitones;
    }

    /**
     * @brief Set vibrato rate (control-rate parameter update).
     * @param rate Vibrato rate in Hz
     */
    void setVibratoRate(Sample rate) noexcept {
        vibrato.setFrequency(rate);
    }

    /**
     * @brief Set master amplitude (control-rate parameter update).
     * @param amp Amplitude [0, 1]
     */
    void setAmplitude(Sample amp) noexcept {
        amplitude = clamp(amp, 0.0f, 1.0f);
    }

    /**
     * @brief Trigger the voice (note on).
     */
    void trigger() noexcept {
        env.trigger();
    }

    /**
     * @brief Release the voice (note off).
     */
    void release() noexcept {
        env.release();
    }

    /**
     * @brief Set gate state directly.
     * @param gateOn True for note on, false for note off
     */
    void setGate(bool gateOn) noexcept {
        env.setGate(gateOn);
    }

    /**
     * @brief Check if voice is active.
     * @return True if voice is producing output
     */
    bool isActive() const noexcept {
        return env.isActive();
    }

    /**
     * @brief Generate single sample (per-sample inline processing).
     * @return Next sample value
     */
    inline Sample tick() noexcept {
        // Apply vibrato modulation to frequency
        if (vibratoDepth > 0.0f) {
            Sample vibMod = vibrato.tick();
            // Convert semitones to frequency ratio using fast exp2 approximation
            // 2^(semitones/12) = exp(semitones * ln(2) / 12)
            Sample semitones = vibMod * vibratoDepth;
            // Fast approximation: for small x, 2^x ≈ 1 + x * ln(2)
            // For vibrato depth typically < 1 semitone, this is sufficient
            Sample ratio = 1.0f + semitones * 0.057762265f;  // ln(2)/12 ≈ 0.057762265
            osc.setFrequency(baseFrequency * ratio);
        }

        // Generate oscillator output
        Sample oscOut = osc.tick();

        // Apply envelope
        Sample envOut = env.tick();

        // Return final output
        return oscOut * envOut * amplitude;
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
     * @brief Reset voice to initial state.
     */
    void reset() noexcept {
        osc.reset();
        env.reset();
        vibrato.reset();
    }
};

} // namespace subcollider

#endif // SUBCOLLIDER_EXAMPLE_VOICE_H
