#include <iostream>
#include <cmath>
#include <subcollider/ugens/LinLin.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

inline bool approx(Sample a, Sample b, Sample epsilon = 0.0001f) {
    return std::abs(a - b) < epsilon;
}

int test_linlin() {
    int failures = 0;

    // --- Basic mapping ---
    {
        Sample outLow = LinLin(0.0f, 0.0f, 1.0f, -1.0f, 1.0f);
        Sample outHigh = LinLin(1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
        Sample outMid = LinLin(0.5f, 0.0f, 1.0f, -1.0f, 1.0f);
        TEST("LinLin maps low endpoint", approx(outLow, -1.0f));
        TEST("LinLin maps high endpoint", approx(outHigh, 1.0f));
        TEST("LinLin maps midpoint", approx(outMid, 0.0f));
    }

    // --- Handles inverted destination range ---
    {
        Sample val = LinLin(0.25f, 0.0f, 1.0f, 10.0f, 0.0f);
        TEST("LinLin inverted dest range", approx(val, 7.5f));
    }

    // --- Handles inverted source range ---
    {
        Sample val = LinLin(0.25f, 1.0f, 0.0f, 0.0f, 1.0f);
        TEST("LinLin inverted src range", approx(val, 0.75f));
    }

    // --- Zero-length source returns destLo ---
    {
        Sample val = LinLin(5.0f, 1.0f, 1.0f, -2.0f, 2.0f);
        TEST("LinLin zero range returns destLo", approx(val, -2.0f));
    }

    // --- Works with values outside source range (no clamp) ---
    {
        Sample below = LinLin(-1.0f, 0.0f, 1.0f, 0.0f, 10.0f);
        Sample above = LinLin(2.0f, 0.0f, 1.0f, 0.0f, 10.0f);
        TEST("LinLin below range extrapolates", approx(below, -10.0f));
        TEST("LinLin above range extrapolates", approx(above, 20.0f));
    }

    return failures;
}
