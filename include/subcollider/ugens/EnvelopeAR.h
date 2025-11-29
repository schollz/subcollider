/**
 * @file EnvelopeAR.h
 * @brief Attack-Release envelope generator UGen.
 *
 * EnvelopeAR generates an attack-release envelope suitable for
 * amplitude modulation. Designed for embedded use with no heap
 * allocation and no virtual calls.
 */

#ifndef SUBCOLLIDER_UGENS_ENVELOPEAR_H
#define SUBCOLLIDER_UGENS_ENVELOPEAR_H

#include "../types.h"
#include <cmath>

namespace subcollider {
namespace ugens {

/**
 * @brief Attack-Release envelope generator.
 *
 * This is a static struct with inline per-sample processing,
 * suitable for embedded DSP with no heap allocation.
 *
 * The envelope uses exponential curves for natural-sounding
 * amplitude envelopes.
 *
 * Usage:
 * @code
 * EnvelopeAR env;
 * env.init(48000.0f);
 * env.setAttack(0.01f);   // 10ms attack
 * env.setRelease(0.5f);   // 500ms release
 *
 * env.trigger();  // Start envelope
 *
 * // Per-sample processing
 * float ampMod = env.tick();
 *
 * // Block processing
 * AudioBuffer<64> buffer;
 * env.process(buffer.data, 64);
 * @endcode
 */
struct EnvelopeAR {
    /// Envelope states
    enum class State : uint8_t {
        Idle,
        Attack,
        Release
    };

    /// Current envelope value [0, 1]
    Sample value;

    /// Attack coefficient (exponential)
    Sample attackCoeff;

    /// Release coefficient (exponential)
    Sample releaseCoeff;

    /// Attack time in seconds
    Sample attackTime;

    /// Release time in seconds
    Sample releaseTime;

    /// Sample rate in Hz
    Sample sampleRate;

    /// Current envelope state
    State state;

    /// Gate state (true = on)
    bool gate;

    /**
     * @brief Initialize the envelope generator.
     * @param sr Sample rate in Hz (default: 48000)
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE) noexcept {
        sampleRate = sr;
        value = 0.0f;
        attackTime = 0.01f;   // 10ms default
        releaseTime = 0.1f;   // 100ms default
        state = State::Idle;
        gate = false;
        updateCoefficients();
    }

    /**
     * @brief Set attack time (control-rate parameter update).
     * @param time Attack time in seconds
     */
    void setAttack(Sample time) noexcept {
        attackTime = time > 0.0001f ? time : 0.0001f;
        updateAttackCoefficient();
    }

    /**
     * @brief Set release time (control-rate parameter update).
     * @param time Release time in seconds
     */
    void setRelease(Sample time) noexcept {
        releaseTime = time > 0.0001f ? time : 0.0001f;
        updateReleaseCoefficient();
    }

    /**
     * @brief Update attack coefficient from current attack time.
     */
    void updateAttackCoefficient() noexcept {
        // Exponential coefficient for ~99.3% target in attackTime seconds
        attackCoeff = std::exp(-1.0f / (attackTime * sampleRate));
    }

    /**
     * @brief Update release coefficient from current release time.
     */
    void updateReleaseCoefficient() noexcept {
        // Exponential coefficient for ~99.3% decay in releaseTime seconds
        releaseCoeff = std::exp(-1.0f / (releaseTime * sampleRate));
    }

    /**
     * @brief Update both coefficients.
     */
    void updateCoefficients() noexcept {
        updateAttackCoefficient();
        updateReleaseCoefficient();
    }

    /**
     * @brief Trigger the envelope (gate on).
     */
    void trigger() noexcept {
        gate = true;
        state = State::Attack;
    }

    /**
     * @brief Release the envelope (gate off).
     */
    void release() noexcept {
        gate = false;
        if (state != State::Idle) {
            state = State::Release;
        }
    }

    /**
     * @brief Set gate state directly.
     * @param gateOn True to trigger, false to release
     */
    void setGate(bool gateOn) noexcept {
        if (gateOn && !gate) {
            trigger();
        } else if (!gateOn && gate) {
            release();
        }
    }

    /**
     * @brief Check if envelope is active (not idle).
     * @return True if envelope is active
     */
    bool isActive() const noexcept {
        return state != State::Idle;
    }

    /**
     * @brief Generate single sample (per-sample inline processing).
     * @return Next envelope value [0, 1]
     */
    inline Sample tick() noexcept {
        switch (state) {
            case State::Attack:
                // Exponential attack toward 1.0
                value = 1.0f - attackCoeff * (1.0f - value);
                if (value >= 0.999f) {
                    value = 1.0f;
                    if (!gate) {
                        state = State::Release;
                    }
                }
                break;

            case State::Release:
                // Exponential release toward 0.0
                value = releaseCoeff * value;
                if (value <= 0.0001f) {
                    value = 0.0f;
                    state = State::Idle;
                }
                break;

            case State::Idle:
            default:
                value = 0.0f;
                break;
        }

        return value;
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
     * @brief Process a block, multiplying with existing buffer.
     * @param buffer Buffer to multiply in-place
     * @param numSamples Number of samples to process
     */
    void processMul(Sample* buffer, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            buffer[i] *= tick();
        }
    }

    /**
     * @brief Reset envelope to idle state.
     */
    void reset() noexcept {
        value = 0.0f;
        state = State::Idle;
        gate = false;
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_ENVELOPEAR_H
