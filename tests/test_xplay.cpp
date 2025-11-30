/**
 * @file test_xplay.cpp
 * @brief Unit tests for XPlay UGen.
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/XPlay.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_xplay() {
    int failures = 0;

    // Deterministic first tick with known noise seeds and unity buffer
    {
        const Sample sr = 48000.0f;
        Sample data[8] = {1, 1, 1, 1, 1, 1, 1, 1}; // 4 stereo frames of 1.0
        Buffer buf(data, 2, sr, 4);

        XPlay xplay;
        xplay.init(sr);
        xplay.setBuffer(&buf);
        xplay.setStartEnd(0.0f, 1.0f);
        xplay.setRate(0.0f);          // Hold phasor steady
        xplay.setFadeTime(0.0f);      // Instant fade for static crossfade
        xplay.setGate(1.0f);

        // Make envelope flat and fade fixed
        xplay.fadeLag.setValue(-1.0f); // Force fade to first head
        xplay.env.value = 1.0f;
        xplay.env.state = EnvelopeADSR::State::Sustain;
        xplay.env.sustainLevel = 1.0f;
        xplay.env.gateValue = 1.0f;

        Stereo out = xplay.tick();
        TEST("XPlay deterministic left", std::abs(out.left - 1.0f) < 1e-5f);
        TEST("XPlay deterministic right", std::abs(out.right - 1.0f) < 1e-5f);
    }

    // Reverse playback keeps phasor within 2x window
    {
        const Sample sr = 1.0f;
        Sample data[4] = {0, 0, 0, 0}; // 2 stereo frames
        Buffer buf(data, 2, sr, 2);

        XPlay xplay;
        xplay.init(sr);
        xplay.setBuffer(&buf);
        xplay.setStartEnd(1.0f, 0.0f); // Reverse
        xplay.setRate(1.0f);
        xplay.setFadeTime(0.0f);
        xplay.setGate(1.0f);
        xplay.fadeLag.setValue(-1.0f);
        xplay.env.value = 1.0f;
        xplay.env.state = EnvelopeADSR::State::Sustain;
        xplay.env.sustainLevel = 1.0f;
        xplay.env.gateValue = 1.0f;

        Sample windowStart = xplay.loopStart;
        Sample windowEnd = xplay.loopStart + (xplay.loopSize * 2.0f);

        bool inRange = true;
        for (int i = 0; i < 10; ++i) {
            xplay.tick();
            if (xplay.phasor < windowStart || xplay.phasor >= windowEnd) {
                inRange = false;
                break;
            }
        }
        TEST("XPlay reverse phasor wraps within window", inRange);
    }

    return failures;
}
