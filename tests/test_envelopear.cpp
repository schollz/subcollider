/**
 * @file test_envelopear.cpp
 * @brief Unit tests for EnvelopeAR UGen
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/EnvelopeAR.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_envelopear() {
    int failures = 0;

    // Test initialization
    {
        EnvelopeAR env;
        env.init(48000.0f);
        TEST("EnvelopeAR init: value starts at 0", env.value == 0.0f);
        TEST("EnvelopeAR init: state is Idle", env.state == EnvelopeAR::State::Idle);
        TEST("EnvelopeAR init: gate is false", env.gate == false);
        TEST("EnvelopeAR init: sample rate is set", env.sampleRate == 48000.0f);
    }

    // Test trigger
    {
        EnvelopeAR env;
        env.init(48000.0f);
        env.trigger();
        TEST("EnvelopeAR trigger: state is Attack", env.state == EnvelopeAR::State::Attack);
        TEST("EnvelopeAR trigger: gate is true", env.gate == true);
        TEST("EnvelopeAR trigger: isActive returns true", env.isActive());
    }

    // Test attack phase increases value
    {
        EnvelopeAR env;
        env.init(48000.0f);
        env.setAttack(0.01f);  // 10ms
        env.trigger();

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
        TEST("EnvelopeAR attack: value increases", increasing);
    }

    // Test release phase decreases value
    {
        EnvelopeAR env;
        env.init(48000.0f);
        env.setAttack(0.001f);  // Very short attack
        env.setRelease(0.1f);
        env.trigger();

        // Run through attack
        for (int i = 0; i < 500; ++i) env.tick();

        env.release();

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
        TEST("EnvelopeAR release: value decreases", decreasing);
    }

    // Test output in range [0, 1]
    {
        EnvelopeAR env;
        env.init(48000.0f);
        env.setAttack(0.01f);
        env.setRelease(0.1f);
        env.trigger();

        bool inRange = true;
        for (int i = 0; i < 10000; ++i) {
            if (i == 500) env.release();
            Sample val = env.tick();
            if (val < 0.0f || val > 1.0f) {
                inRange = false;
                break;
            }
        }
        TEST("EnvelopeAR tick: output in range [0, 1]", inRange);
    }

    // Test idle state when released fully
    {
        EnvelopeAR env;
        env.init(48000.0f);
        env.setAttack(0.001f);
        env.setRelease(0.001f);
        env.trigger();
        env.release();

        for (int i = 0; i < 1000; ++i) env.tick();

        TEST("EnvelopeAR: returns to idle after release", env.state == EnvelopeAR::State::Idle);
        TEST("EnvelopeAR: isActive false when idle", !env.isActive());
    }

    // Test reset
    {
        EnvelopeAR env;
        env.init(48000.0f);
        env.trigger();
        for (int i = 0; i < 100; ++i) env.tick();

        env.reset();
        TEST("EnvelopeAR reset: value is 0", env.value == 0.0f);
        TEST("EnvelopeAR reset: state is Idle", env.state == EnvelopeAR::State::Idle);
        TEST("EnvelopeAR reset: gate is false", env.gate == false);
    }

    return failures;
}
