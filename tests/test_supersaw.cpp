/**
 * @file test_supersaw.cpp
 * @brief Unit tests for SuperSaw UGen
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/SuperSaw.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_supersaw() {
    int failures = 0;

    // Test initialization
    {
        SuperSaw supersaw;
        supersaw.init(48000.0f);
        TEST("SuperSaw init: sample rate is set", supersaw.sampleRate == 48000.0f);
        TEST("SuperSaw init: frequency is set", supersaw.frequency == 400.0f);
        TEST("SuperSaw init: not active", !supersaw.isActive());
    }

    // Test gate on makes it active
    {
        SuperSaw supersaw;
        supersaw.init(48000.0f);
        supersaw.gate(1.0f);
        TEST("SuperSaw gate on: becomes active", supersaw.isActive());
    }

    // Test output is non-zero when gated
    {
        SuperSaw supersaw;
        supersaw.init(48000.0f, 42);
        supersaw.setFrequency(440.0f);
        supersaw.gate(1.0f);

        bool hasNonZero = false;
        for (int i = 0; i < 1000; ++i) {
            Stereo sample = supersaw.tick();
            if (std::abs(sample.left) > 0.001f || std::abs(sample.right) > 0.001f) {
                hasNonZero = true;
                break;
            }
        }
        TEST("SuperSaw gated: produces non-zero output", hasNonZero);
    }

    // Test output is in valid range
    {
        SuperSaw supersaw;
        supersaw.init(48000.0f, 42);
        supersaw.setFrequency(440.0f);
        supersaw.gate(1.0f);

        bool inRange = true;
        for (int i = 0; i < 10000; ++i) {
            Stereo sample = supersaw.tick();
            if (std::abs(sample.left) > 10.0f || std::abs(sample.right) > 10.0f) {
                inRange = false;
                break;
            }
            if (std::isnan(sample.left) || std::isnan(sample.right)) {
                inRange = false;
                break;
            }
            if (std::isinf(sample.left) || std::isinf(sample.right)) {
                inRange = false;
                break;
            }
        }
        TEST("SuperSaw output: in valid range (no NaN/Inf)", inRange);
    }

    // Test frequency setting
    {
        SuperSaw supersaw;
        supersaw.init(48000.0f);
        supersaw.setFrequency(880.0f);
        TEST("SuperSaw setFrequency: frequency is set", supersaw.frequency == 880.0f);
    }

    // Test detune setting
    {
        SuperSaw supersaw;
        supersaw.init(48000.0f);
        supersaw.setDetune(0.5f);
        TEST("SuperSaw setDetune: detune is set", supersaw.detune == 0.5f);
    }

    // Test vibrato rate setting
    {
        SuperSaw supersaw;
        supersaw.init(48000.0f);
        supersaw.setVibratoRate(8.0f);
        TEST("SuperSaw setVibratoRate: rate is set", supersaw.vibrRate == 8.0f);
    }

    // Test vibrato depth setting
    {
        SuperSaw supersaw;
        supersaw.init(48000.0f);
        supersaw.setVibratoDepth(0.5f);
        TEST("SuperSaw setVibratoDepth: depth is set", supersaw.vibrDepth == 0.5f);
    }

    // Test spread setting
    {
        SuperSaw supersaw;
        supersaw.init(48000.0f);
        supersaw.setSpread(0.8f);
        TEST("SuperSaw setSpread: spread is set", supersaw.spread == 0.8f);
    }

    // Test ADSR envelope parameters
    {
        SuperSaw supersaw;
        supersaw.init(48000.0f);
        supersaw.setAttack(0.05f);
        supersaw.setDecay(0.2f);
        supersaw.setSustain(0.5f);
        supersaw.setRelease(0.4f);

        TEST("SuperSaw ADSR: attack is set", supersaw.envelope.attackTime == 0.05f);
        TEST("SuperSaw ADSR: decay is set", supersaw.envelope.decayTime == 0.2f);
        TEST("SuperSaw ADSR: sustain is set", supersaw.envelope.sustainLevel == 0.5f);
        TEST("SuperSaw ADSR: release is set", supersaw.envelope.releaseTime == 0.4f);
    }

    // Test gate off makes it eventually inactive
    {
        SuperSaw supersaw;
        supersaw.init(48000.0f);
        supersaw.setRelease(0.01f);  // Short release
        supersaw.gate(1.0f);

        // Run for a bit
        for (int i = 0; i < 1000; ++i) supersaw.tick();

        supersaw.gate(0.0f);

        // Run through release
        for (int i = 0; i < 10000; ++i) supersaw.tick();

        TEST("SuperSaw gate off: becomes inactive after release", !supersaw.isActive());
    }

    // Test reset
    {
        SuperSaw supersaw;
        supersaw.init(48000.0f);
        supersaw.gate(1.0f);
        for (int i = 0; i < 100; ++i) supersaw.tick();

        supersaw.reset();

        TEST("SuperSaw reset: not active", !supersaw.isActive());
    }

    // Test stereo output differs when spread is non-zero
    {
        SuperSaw supersaw;
        supersaw.init(48000.0f, 42);
        supersaw.setFrequency(440.0f);
        supersaw.setSpread(1.0f);  // Maximum spread
        supersaw.gate(1.0f);

        // Skip initial samples
        for (int i = 0; i < 500; ++i) supersaw.tick();

        bool hasStereoDifference = false;
        for (int i = 0; i < 1000; ++i) {
            Stereo sample = supersaw.tick();
            // With non-zero spread, the panner should produce channel differences
            if (std::abs(sample.left - sample.right) > 0.001f) {
                hasStereoDifference = true;
                break;
            }
        }
        TEST("SuperSaw output: stereo (left and right differ with spread)", hasStereoDifference);
    }

    // Test output increases during attack
    {
        SuperSaw supersaw;
        supersaw.init(48000.0f, 42);
        supersaw.setFrequency(440.0f);
        supersaw.setAttack(0.1f);  // 100ms attack
        supersaw.gate(1.0f);

        Sample firstSample = 0.0f;
        for (int i = 0; i < 10; ++i) {
            Stereo s = supersaw.tick();
            firstSample += std::abs(s.left) + std::abs(s.right);
        }
        firstSample /= 20.0f;

        // Skip ahead in attack
        for (int i = 0; i < 1000; ++i) supersaw.tick();

        Sample laterSample = 0.0f;
        for (int i = 0; i < 10; ++i) {
            Stereo s = supersaw.tick();
            laterSample += std::abs(s.left) + std::abs(s.right);
        }
        laterSample /= 20.0f;

        TEST("SuperSaw attack: amplitude increases over time", laterSample > firstSample);
    }

    // Test all 7 voices are different (due to detuning and random phases)
    {
        SuperSaw supersaw;
        supersaw.init(48000.0f, 42);

        bool allUnique = true;
        for (int i = 0; i < SuperSaw::NUM_VOICES - 1; ++i) {
            for (int j = i + 1; j < SuperSaw::NUM_VOICES; ++j) {
                // Check detune offsets are different
                if (supersaw.voices[i].detuneOffset == supersaw.voices[j].detuneOffset) {
                    allUnique = false;
                }
                // Check random phases are different
                if (supersaw.voices[i].sawPhase == supersaw.voices[j].sawPhase) {
                    allUnique = false;
                }
            }
        }
        TEST("SuperSaw voices: all have unique detune/phases", allUnique);
    }

    return failures;
}
