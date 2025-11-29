/**
 * @file EnvelopeADSR.h
 * @brief Attack-Decay-Sustain-Release envelope generator UGen.
 *
 * EnvelopeADSR generates a classic ADSR envelope suitable for
 * synthesizer voice control. Designed for embedded use with no heap
 * allocation and no virtual calls.
 */

#ifndef SUBCOLLIDER_UGENS_ENVELOPEADSR_H
#define SUBCOLLIDER_UGENS_ENVELOPEADSR_H

#include "../types.h"
#include <cmath>

namespace subcollider {
namespace ugens {

/**
 * @brief ADSR envelope generator.
 *
 * This is a static struct with inline per-sample processing,
 * suitable for embedded DSP with no heap allocation.
 *
 * The envelope uses exponential curves for natural-sounding
 * amplitude envelopes with four distinct phases:
 * - Attack: 0 -> 1 over attack time
 * - Decay: 1 -> sustain level over decay time
 * - Sustain: hold at sustain level while gate is high
 * - Release: current level -> 0 over release time when gate goes low
 *
 * Usage:
 * @code
 * EnvelopeADSR env;
 * env.init(48000.0f);
 * env.setAttack(0.01f);    // 10ms attack
 * env.setDecay(0.05f);     // 50ms decay
 * env.setSustain(0.7f);    // 70% sustain level
 * env.setRelease(0.3f);    // 300ms release
 *
 * env.gate(1.0f);  // Start envelope (note on)
 *
 * // Per-sample processing
 * float ampMod = env.tick();
 *
 * env.gate(0.0f);  // Release envelope (note off)
 * @endcode
 */
struct EnvelopeADSR {
    /// Envelope states
    enum class State : uint8_t {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    /// Done actions (similar to SuperCollider)
    enum class DoneAction : uint8_t {
        None = 0,      // Do nothing when envelope completes
        Pause = 1,     // Pause (for future use)
        Free = 2       // Mark as done/free (for voice management)
    };

    /// Current envelope value [0, 1]
    Sample value;

    /// Attack coefficient (exponential)
    Sample attackCoeff;

    /// Decay coefficient (exponential)
    Sample decayCoeff;

    /// Release coefficient (exponential)
    Sample releaseCoeff;

    /// Attack time in seconds
    Sample attackTime;

    /// Decay time in seconds
    Sample decayTime;

    /// Sustain level [0, 1]
    Sample sustainLevel;

    /// Release time in seconds
    Sample releaseTime;

    /// Sample rate in Hz
    Sample sampleRate;

    /// Current envelope state
    State state;

    /// Gate state (>0 = on, 0 = off)
    Sample gateValue;

    /// Done action
    DoneAction doneAction;

    /// Done flag (set when envelope completes)
    bool done;

    /**
     * @brief Initialize the envelope generator.
     * @param sr Sample rate in Hz (default: 48000)
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE) noexcept {
        sampleRate = sr;
        value = 0.0f;
        attackTime = 0.01f;    // 10ms default
        decayTime = 0.1f;      // 100ms default
        sustainLevel = 0.7f;   // 70% default
        releaseTime = 0.3f;    // 300ms default
        state = State::Idle;
        gateValue = 0.0f;
        doneAction = DoneAction::None;
        done = false;
        updateCoefficients();
    }

    /**
     * @brief Set attack time.
     * @param time Attack time in seconds
     */
    void setAttack(Sample time) noexcept {
        attackTime = time > 0.0001f ? time : 0.0001f;
        updateAttackCoefficient();
    }

    /**
     * @brief Set decay time.
     * @param time Decay time in seconds
     */
    void setDecay(Sample time) noexcept {
        decayTime = time > 0.0001f ? time : 0.0001f;
        updateDecayCoefficient();
    }

    /**
     * @brief Set sustain level.
     * @param level Sustain level [0, 1]
     */
    void setSustain(Sample level) noexcept {
        sustainLevel = clamp(level, 0.0f, 1.0f);
    }

    /**
     * @brief Set release time.
     * @param time Release time in seconds
     */
    void setRelease(Sample time) noexcept {
        releaseTime = time > 0.0001f ? time : 0.0001f;
        updateReleaseCoefficient();
    }

    /**
     * @brief Set done action.
     * @param action Done action to perform when envelope completes
     */
    void setDoneAction(DoneAction action) noexcept {
        doneAction = action;
    }

    /**
     * @brief Update attack coefficient from current attack time.
     */
    void updateAttackCoefficient() noexcept {
        attackCoeff = std::exp(-1.0f / (attackTime * sampleRate));
    }

    /**
     * @brief Update decay coefficient from current decay time.
     */
    void updateDecayCoefficient() noexcept {
        decayCoeff = std::exp(-1.0f / (decayTime * sampleRate));
    }

    /**
     * @brief Update release coefficient from current release time.
     */
    void updateReleaseCoefficient() noexcept {
        releaseCoeff = std::exp(-1.0f / (releaseTime * sampleRate));
    }

    /**
     * @brief Update all coefficients.
     */
    void updateCoefficients() noexcept {
        updateAttackCoefficient();
        updateDecayCoefficient();
        updateReleaseCoefficient();
    }

    /**
     * @brief Set gate value (audio rate).
     * @param gate Gate value (>0 = on, 0 = off)
     *
     * When gate transitions from 0 to >0, envelope starts from Attack.
     * When gate transitions from >0 to 0, envelope moves to Release.
     */
    void gate(Sample gate) noexcept {
        Sample prevGate = gateValue;
        gateValue = gate;

        // Gate on (positive edge)
        if (prevGate <= 0.0f && gate > 0.0f) {
            state = State::Attack;
            done = false;
        }
        // Gate off (negative edge)
        else if (prevGate > 0.0f && gate <= 0.0f) {
            if (state != State::Idle) {
                state = State::Release;
            }
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
     * @brief Check if envelope is done.
     * @return True if envelope has completed
     */
    bool isDone() const noexcept {
        return done;
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
                    // Move to decay phase
                    state = State::Decay;
                }
                // Check for early release during attack
                if (gateValue <= 0.0f) {
                    state = State::Release;
                }
                break;

            case State::Decay:
                // Exponential decay toward sustain level
                value = sustainLevel + decayCoeff * (value - sustainLevel);
                if (std::abs(value - sustainLevel) < 0.001f) {
                    value = sustainLevel;
                    // Move to sustain phase
                    state = State::Sustain;
                }
                // Check for early release during decay
                if (gateValue <= 0.0f) {
                    state = State::Release;
                }
                break;

            case State::Sustain:
                // Hold at sustain level
                value = sustainLevel;
                // Check for release
                if (gateValue <= 0.0f) {
                    state = State::Release;
                }
                break;

            case State::Release:
                // Exponential release toward 0.0
                value = releaseCoeff * value;
                if (value <= 0.0001f) {
                    value = 0.0f;
                    state = State::Idle;
                    done = true;
                    // Perform done action
                    if (doneAction == DoneAction::Free) {
                        // Mark as free for voice management
                        // (application can check isDone())
                    }
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
        gateValue = 0.0f;
        done = false;
    }

    /**
     * @brief Get current envelope state.
     * @return Current state
     */
    State getState() const noexcept {
        return state;
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_ENVELOPEADSR_H
