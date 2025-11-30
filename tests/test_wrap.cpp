/**
 * @file test_wrap.cpp
 * @brief Unit tests for Wrap UGen.
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/Wrap.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_wrap() {
    int failures = 0;

    // Pass-through when in range
    {
        Wrap w;
        Sample out = w.process(0.25f, 0.0f, 1.0f);
        TEST("Wrap pass-through in range", std::abs(out - 0.25f) < 0.0001f);
    }

    // Wrap above high bound
    {
        Wrap w;
        Sample out = w.process(1.2f, 0.0f, 1.0f);
        TEST("Wrap above high wraps to low", std::abs(out - 0.2f) < 0.0001f);
    }

    // Wrap below low bound (negative inputs)
    {
        Wrap w;
        Sample out = w.process(-0.25f, 0.0f, 1.0f);
        TEST("Wrap below low wraps to top of range", std::abs(out - 0.75f) < 0.0001f);
    }

    // Custom range with tick() and setRange()
    {
        Wrap w;
        w.setRange(-2.0f, 2.0f);
        Sample out = w.tick(5.0f);
        TEST("Wrap custom range wraps down", std::abs(out - 1.0f) < 0.0001f);
    }

    // Degenerate range (hi <= lo) returns low
    {
        Wrap w;
        Sample out = w.process(0.5f, 1.0f, 1.0f);
        TEST("Wrap degenerate range returns low", std::abs(out - 1.0f) < 0.0001f);
    }

    // Buffer processing mirrors tick()
    {
        Wrap w;
        w.setRange(0.0f, 1.0f);
        Sample input[4] = {0.0f, 0.5f, 1.25f, -0.1f};
        Sample output[4] = {};
        w.process(input, output, 4);
        bool matches = true;
        for (int i = 0; i < 4; ++i) {
            Sample expected = w.tick(input[i]);
            if (std::abs(expected - output[i]) > 0.0001f) {
                matches = false;
                break;
            }
        }
        TEST("Wrap buffer processing matches tick", matches);
    }

    return failures;
}
