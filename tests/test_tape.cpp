/**
 * @file test_tape.cpp
 * @brief Tests for Tape UGen.
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/Tape.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_tape() {
    int failures = 0;

    // Initialization
    {
        Tape tape;
        tape.init(48000.0f);
        TEST("Tape init: sample rate set", tape.sampleRate == 48000.0f);
        TEST("Tape init: bias default zero", tape.bias == 0.0f);
        TEST("Tape init: pregain default unity", tape.pregain == 1.0f);
        TEST("Tape init: follower cleared", tape.getFollowerValue() == 0.0f);
        TEST("Tape init: DC blocker cleared", tape.dcLeft.prevInput == 0.0f && tape.dcLeft.prevOutput == 0.0f);
    }

    // Parameter setters
    {
        Tape tape;
        tape.init();
        tape.setBias(0.5f);
        tape.setPregain(2.0f);
        TEST("Tape setBias", tape.bias == 0.5f);
        TEST("Tape setPregain", tape.pregain == 2.0f);
    }

    // Tick should be near-linear for very small signals
    {
        Tape tape;
        tape.init();
        Sample out = tape.tick(0.01f);
        TEST("Tape tick: small signal nearly passthrough", std::fabs(out - 0.01f) < 0.005f);
    }

    // Bias should increase output energy
    {
        Tape noBias;
        Tape withBias;
        noBias.init();
        withBias.init();
        withBias.setBias(1.0f);

        Sample sumNoBias = 0.0f;
        Sample sumWithBias = 0.0f;
        for (size_t i = 0; i < 256; ++i) {
            Sample input = 0.5f;
            sumNoBias += std::fabs(noBias.tick(input));
            sumWithBias += std::fabs(withBias.tick(input));
        }
        TEST("Tape bias: adds level compared to neutral bias", sumWithBias > sumNoBias);
    }

    // DC blocking should remove steady DC after some time
    {
        Tape tape;
        tape.init();
        Sample last = 0.0f;
        for (size_t i = 0; i < 1024; ++i) {
            last = tape.tick(1.0f);
        }
        TEST("Tape DC blocker: output decays toward zero", std::fabs(last) < 0.05f);
    }

    // Stereo follower should drive both channels from the left input
    {
        Tape stereo;
        Tape mono;
        stereo.init();
        mono.init();
        stereo.setBias(1.0f);
        mono.setBias(1.0f);

        Sample monoOut = 0.0f;
        Stereo stereoOut;
        for (size_t i = 0; i < 128; ++i) {
            stereoOut = stereo.tickStereo(1.0f, 0.1f);
            monoOut = mono.tick(0.1f);
        }
        TEST("Tape stereo: right channel affected by left follower", stereoOut.right > monoOut);
    }

    // Reset clears state
    {
        Tape tape;
        tape.init();
        tape.tickStereo(1.0f, -1.0f);
        tape.reset();
        TEST("Tape reset: follower cleared", tape.getFollowerValue() == 0.0f);
        TEST("Tape reset: DC blocker cleared", tape.dcLeft.prevInput == 0.0f && tape.dcLeft.prevOutput == 0.0f);
        TEST("Tape reset: silent tick after reset", std::fabs(tape.tick(0.0f)) < 1e-6f);
    }

    return failures;
}
