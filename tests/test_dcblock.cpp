/**
 * @file test_dcblock.cpp
 * @brief Tests for DCBlock UGen.
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/DCBlock.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_dcblock() {
    int failures = 0;

    // Initialization
    {
        DCBlock dc;
        dc.init(48000.0f, 20.0f);
        TEST("DCBlock init: sample rate set", dc.sampleRate == 48000.0f);
        TEST("DCBlock init: cutoff set", std::fabs(dc.cutoff - 20.0f) < 1e-6f);
        TEST("DCBlock init: state zero", dc.prevInputL == 0.0f && dc.prevOutputL == 0.0f);
    }

    // DC offset removal in mono
    {
        DCBlock dc;
        dc.init();
        Sample last = 0.0f;
        for (size_t i = 0; i < 2048; ++i) {
            last = dc.tick(1.0f); // constant DC
        }
        TEST("DCBlock mono: output decays toward zero", std::fabs(last) < 0.05f);
    }

    // AC signal passes through largely unchanged
    {
        DCBlock dc;
        dc.init(48000.0f, 5.0f); // low cutoff for minimal ripple
        Sample maxError = 0.0f;
        for (size_t i = 0; i < 1024; ++i) {
            Sample in = std::sin(2.0f * PI * 440.0f * (static_cast<Sample>(i) / 48000.0f));
            Sample out = dc.tick(in);
            maxError = std::max(maxError, std::fabs(out - in));
        }
        TEST("DCBlock mono: minimal distortion on AC", maxError < 0.1f);
    }

    // Stereo channels maintain independent state
    {
        DCBlock dc;
        dc.init();
        Stereo out = dc.tickStereo(1.0f, -1.0f);
        for (size_t i = 0; i < 512; ++i) {
            out = dc.tickStereo(1.0f, -1.0f);
        }
        TEST("DCBlock stereo: left and right maintain opposite polarity",
             out.left > 0.0f && out.right < 0.0f);
    }

    // Reset clears state
    {
        DCBlock dc;
        dc.init();
        dc.tick(1.0f);
        dc.reset();
        TEST("DCBlock reset: state cleared", dc.prevInputL == 0.0f && dc.prevOutputL == 0.0f);
    }

    return failures;
}
