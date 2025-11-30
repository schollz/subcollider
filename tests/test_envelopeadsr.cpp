/**
 * @file test_envelopeadsr.cpp
 * @brief Unit tests for EnvelopeADSR UGen
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/EnvelopeADSR.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_envelopeadsr() {
    int failures = 0;

    // Test initialization
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        TEST("EnvelopeADSR init: value starts at 0", env.value == 0.0f);
        TEST("EnvelopeADSR init: state is Idle", env.state == EnvelopeADSR::State::Idle);
        TEST("EnvelopeADSR init: gate is 0", env.gateValue == 0.0f);
        TEST("EnvelopeADSR init: sample rate is set", env.sampleRate == 48000.0f);
        TEST("EnvelopeADSR init: not done", env.isDone() == false);
    }

    // Test gate on triggers attack
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        env.gate(1.0f);
        TEST("EnvelopeADSR gate on: state is Attack", env.state == EnvelopeADSR::State::Attack);
        TEST("EnvelopeADSR gate on: isActive returns true", env.isActive());
    }

    // Test attack phase increases value
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        env.setAttack(0.01f);  // 10ms
        env.gate(1.0f);

        Sample prev = 0.0f;
        bool increasing = true;
        for (int i = 0; i < 100; ++i) {
            Sample val = env.tick();
            if (val < prev) {
                increasing = false;
                break;
            }
            prev = val;
        }
        TEST("EnvelopeADSR attack: value increases", increasing);
    }

    // Test attack reaches peak and transitions to decay
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        env.setAttack(0.001f);  // Very short attack
        env.setDecay(0.1f);
        env.setSustain(0.7f);
        env.gate(1.0f);

        // Run through attack phase
        for (int i = 0; i < 1000; ++i) {
            env.tick();
        }

        TEST("EnvelopeADSR after attack: state is Decay or Sustain",
             env.state == EnvelopeADSR::State::Decay ||
             env.state == EnvelopeADSR::State::Sustain);
    }

    // Test decay phase moves toward sustain level
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        env.setAttack(0.001f);   // Very short attack
        env.setDecay(0.05f);     // 50ms decay
        env.setSustain(0.5f);
        env.gate(1.0f);

        // Run through attack
        for (int i = 0; i < 500; ++i) env.tick();

        // Should be in decay now, approaching 0.5
        for (int i = 0; i < 5000; ++i) env.tick();

        TEST("EnvelopeADSR decay: approaches sustain level",
             std::abs(env.value - 0.5f) < 0.1f);
    }

    // Test sustain phase holds level
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        env.setAttack(0.001f);
        env.setDecay(0.001f);
        env.setSustain(0.6f);
        env.gate(1.0f);

        // Run through attack and decay
        for (int i = 0; i < 1000; ++i) env.tick();

        // Should be in sustain now
        Sample sustainValue = env.value;
        bool holding = true;
        for (int i = 0; i < 1000; ++i) {
            env.tick();
            if (std::abs(env.value - sustainValue) > 0.01f) {
                holding = false;
                break;
            }
        }
        TEST("EnvelopeADSR sustain: holds level while gate is high", holding);
        TEST("EnvelopeADSR sustain: state is Sustain", env.state == EnvelopeADSR::State::Sustain);
    }

    // Test gate off triggers release
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        env.setAttack(0.001f);
        env.setDecay(0.001f);
        env.setSustain(0.7f);
        env.setRelease(0.1f);
        env.gate(1.0f);

        // Run to sustain
        for (int i = 0; i < 1000; ++i) env.tick();

        // Gate off
        env.gate(0.0f);
        env.tick();

        TEST("EnvelopeADSR gate off: state is Release", env.state == EnvelopeADSR::State::Release);
    }

    // Test release phase decreases value
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        env.setAttack(0.001f);
        env.setDecay(0.001f);
        env.setSustain(0.7f);
        env.setRelease(0.1f);
        env.gate(1.0f);

        // Run to sustain
        for (int i = 0; i < 1000; ++i) env.tick();

        env.gate(0.0f);
        Sample prev = env.value;
        bool decreasing = true;
        for (int i = 0; i < 1000; ++i) {
            Sample val = env.tick();
            if (val > prev) {
                decreasing = false;
                break;
            }
            prev = val;
        }
        TEST("EnvelopeADSR release: value decreases", decreasing);
    }

    // Test release eventually reaches idle
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        env.setAttack(0.001f);
        env.setDecay(0.001f);
        env.setSustain(0.7f);
        env.setRelease(0.01f);  // Short release
        env.gate(1.0f);

        // Run to sustain
        for (int i = 0; i < 1000; ++i) env.tick();

        env.gate(0.0f);

        // Run through release
        for (int i = 0; i < 10000; ++i) env.tick();

        TEST("EnvelopeADSR release complete: state is Idle", env.state == EnvelopeADSR::State::Idle);
        TEST("EnvelopeADSR release complete: value is 0", env.value == 0.0f);
        TEST("EnvelopeADSR release complete: not active", !env.isActive());
    }

    // Test early release (release during attack)
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        env.setAttack(0.1f);   // Long attack
        env.setRelease(0.05f);
        env.gate(1.0f);

        // Tick a few times during attack
        for (int i = 0; i < 100; ++i) env.tick();

        // Gate off during attack
        env.gate(0.0f);
        env.tick();

        TEST("EnvelopeADSR early release: transitions to Release",
             env.state == EnvelopeADSR::State::Release);
    }

    // Test early release (release during decay)
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        env.setAttack(0.001f);
        env.setDecay(0.1f);    // Long decay
        env.setSustain(0.5f);
        env.setRelease(0.05f);
        env.gate(1.0f);

        // Run through attack
        for (int i = 0; i < 500; ++i) env.tick();

        // Gate off during decay
        env.gate(0.0f);
        env.tick();

        TEST("EnvelopeADSR early release from decay: transitions to Release",
             env.state == EnvelopeADSR::State::Release);
    }

    // Test sustain level parameter
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        env.setAttack(0.001f);
        env.setDecay(0.01f);
        env.setSustain(0.3f);  // Low sustain
        env.gate(1.0f);

        // Run to sustain
        for (int i = 0; i < 2000; ++i) env.tick();

        TEST("EnvelopeADSR sustain level: matches set value",
             std::abs(env.value - 0.3f) < 0.05f);
    }

    // Test full envelope cycle
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        env.setAttack(0.01f);
        env.setDecay(0.02f);
        env.setSustain(0.6f);
        env.setRelease(0.03f);
        env.gate(1.0f);

        bool reachedPeak = false;
        bool reachedSustain = false;
        bool completed = false;

        // Run full cycle
        for (int i = 0; i < 30000; ++i) {
            Sample val = env.tick();
            if (val > 0.95f) reachedPeak = true;
            if (env.state == EnvelopeADSR::State::Sustain) reachedSustain = true;

            // Release after being in sustain for a bit
            if (i == 10000) {
                env.gate(0.0f);
            }

            if (env.state == EnvelopeADSR::State::Idle && i > 10000) {
                completed = true;
                break;
            }
        }

        TEST("EnvelopeADSR full cycle: reached peak", reachedPeak);
        TEST("EnvelopeADSR full cycle: reached sustain", reachedSustain);
        TEST("EnvelopeADSR full cycle: completed", completed);
    }

    // Test done flag and done action
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        env.setAttack(0.001f);
        env.setDecay(0.001f);
        env.setSustain(0.7f);
        env.setRelease(0.01f);
        env.setDoneAction(EnvelopeADSR::DoneAction::ActionFree);

        env.gate(1.0f);
        for (int i = 0; i < 1000; ++i) env.tick();

        env.gate(0.0f);
        for (int i = 0; i < 5000; ++i) env.tick();

        TEST("EnvelopeADSR done flag: set when complete", env.isDone());
    }

    // Test reset
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        env.gate(1.0f);
        for (int i = 0; i < 100; ++i) env.tick();

        env.reset();

        TEST("EnvelopeADSR reset: value is 0", env.value == 0.0f);
        TEST("EnvelopeADSR reset: state is Idle", env.state == EnvelopeADSR::State::Idle);
        TEST("EnvelopeADSR reset: gate is 0", env.gateValue == 0.0f);
        TEST("EnvelopeADSR reset: not done", !env.isDone());
    }

    // Test retrigger (gate on while envelope is active)
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        env.setAttack(0.1f);  // Long attack
        env.gate(1.0f);

        for (int i = 0; i < 1000; ++i) env.tick();

        // Gate off and on again
        env.gate(0.0f);
        for (int i = 0; i < 10; ++i) env.tick();
        env.gate(1.0f);

        TEST("EnvelopeADSR retrigger: restarts attack",
             env.state == EnvelopeADSR::State::Attack);
    }

    // Test zero sustain level
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        env.setAttack(0.001f);
        env.setDecay(0.01f);
        env.setSustain(0.0f);  // Zero sustain
        env.gate(1.0f);

        for (int i = 0; i < 3000; ++i) env.tick();

        TEST("EnvelopeADSR zero sustain: value near 0",
             env.value < 0.01f);
    }

    // Test full sustain level
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        env.setAttack(0.001f);
        env.setDecay(0.01f);
        env.setSustain(1.0f);  // Full sustain
        env.gate(1.0f);

        for (int i = 0; i < 3000; ++i) env.tick();

        TEST("EnvelopeADSR full sustain: value near 1",
             env.value > 0.95f);
    }

    // Test processMul
    {
        EnvelopeADSR env;
        env.init(48000.0f);
        env.setAttack(0.001f);
        env.setDecay(0.001f);
        env.setSustain(0.5f);
        env.gate(1.0f);

        Sample buffer[64];
        for (int i = 0; i < 64; ++i) buffer[i] = 1.0f;

        env.processMul(buffer, 64);

        bool allModified = true;
        for (int i = 0; i < 64; ++i) {
            if (buffer[i] == 1.0f) {
                allModified = false;
                break;
            }
        }
        TEST("EnvelopeADSR processMul: modifies buffer", allModified);
    }

    return failures;
}
