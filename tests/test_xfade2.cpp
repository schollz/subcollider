/**
 * @file test_xfade2.cpp
 * @brief Unit tests for XFade2 UGen.
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/XFade2.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_xfade2() {
    int failures = 0;

    // Full A and full B positions
    {
        XFade2 xfade;
        Sample inA = 1.0f;
        Sample inB = -0.5f;
        TEST("XFade2 pos -1 returns A", std::abs(xfade.process(inA, inB, -1.0f) - inA) < 0.0001f);
        TEST("XFade2 pos +1 returns B", std::abs(xfade.process(inA, inB, 1.0f) - inB) < 0.0001f);
    }

    // Center position uses equal gains (~0.707)
    {
        XFade2 xfade;
        Sample out = xfade.process(1.0f, 0.0f, 0.0f);
        TEST("XFade2 center gain ~0.707 on A", std::abs(out - 0.7071f) < 0.01f);
    }

    // Level scaling
    {
        XFade2 xfade;
        Sample out = xfade.process(1.0f, 1.0f, 0.0f, 0.5f);
        TEST("XFade2 level scales output", std::abs(out - 0.7071f) < 0.01f);
    }

    // Constant-power relationship gainA^2 + gainB^2 ~= 1
    {
        XFade2 xfade;
        bool constantPower = true;
        for (Sample pos = -1.0f; pos <= 1.0f; pos += 0.2f) {
            Sample gainA = xfade.process(1.0f, 0.0f, pos);
            Sample gainB = xfade.process(0.0f, 1.0f, pos);
            Sample power = gainA * gainA + gainB * gainB;
            if (std::abs(power - 1.0f) > 0.01f) {
                constantPower = false;
                break;
            }
        }
        TEST("XFade2 equal power law", constantPower);
    }

    // Stereo crossfade uses same gains per channel
    {
        XFade2 xfade;
        Stereo a(1.0f, 0.5f);
        Stereo b(-1.0f, -0.5f);
        Stereo out = xfade.process(a, b, 0.0f);
        TEST("XFade2 stereo center left ~0", std::abs(out.left) < 0.01f);
        TEST("XFade2 stereo center right ~0", std::abs(out.right) < 0.01f);
    }

    // Clamping of position
    {
        XFade2 xfade;
        Sample clampedHigh = xfade.process(1.0f, 0.0f, 2.0f);
        Sample atHigh = xfade.process(1.0f, 0.0f, 1.0f);
        Sample clampedLow = xfade.process(0.0f, 1.0f, -2.0f);
        Sample atLow = xfade.process(0.0f, 1.0f, -1.0f);
        TEST("XFade2 clamp high", std::abs(clampedHigh - atHigh) < 0.0001f);
        TEST("XFade2 clamp low", std::abs(clampedLow - atLow) < 0.0001f);
    }

    // setPosition/tick matches process
    {
        XFade2 xfade;
        xfade.setPosition(0.5f);
        Sample tickOut = xfade.tick(1.0f, 0.0f);
        Sample procOut = xfade.process(1.0f, 0.0f, 0.5f);
        TEST("XFade2 tick matches process", std::abs(tickOut - procOut) < 0.0001f);
    }

    // setLevel scales cached gains
    {
        XFade2 xfade;
        xfade.setPosition(-0.2f, 1.0f);
        Sample first = xfade.tick(1.0f, 1.0f);
        xfade.setLevel(0.25f);
        Sample scaled = xfade.tick(1.0f, 1.0f);
        TEST("XFade2 setLevel scales output", std::abs(scaled - first * 0.25f) < 0.0001f);
    }

    return failures;
}
