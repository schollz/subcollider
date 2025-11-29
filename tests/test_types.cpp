/**
 * @file test_types.cpp
 * @brief Unit tests for types.h
 */

#include <iostream>
#include <cmath>
#include <subcollider/types.h>

using namespace subcollider;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_types() {
    int failures = 0;

    // Test constants
    TEST("PI is approximately 3.14159", std::abs(PI - 3.14159265f) < 0.0001f);
    TEST("TWO_PI is 2*PI", std::abs(TWO_PI - 2.0f * PI) < 0.0001f);
    TEST("DEFAULT_SAMPLE_RATE is 48000", DEFAULT_SAMPLE_RATE == 48000.0f);
    TEST("DEFAULT_BLOCK_SIZE is 64", DEFAULT_BLOCK_SIZE == 64);

    // Test lerp
    TEST("lerp(0, 10, 0) = 0", lerp(0.0f, 10.0f, 0.0f) == 0.0f);
    TEST("lerp(0, 10, 1) = 10", lerp(0.0f, 10.0f, 1.0f) == 10.0f);
    TEST("lerp(0, 10, 0.5) = 5", std::abs(lerp(0.0f, 10.0f, 0.5f) - 5.0f) < 0.0001f);
    TEST("lerp(-1, 1, 0.25) = -0.5", std::abs(lerp(-1.0f, 1.0f, 0.25f) - (-0.5f)) < 0.0001f);

    // Test clamp
    TEST("clamp(5, 0, 10) = 5", clamp(5.0f, 0.0f, 10.0f) == 5.0f);
    TEST("clamp(-5, 0, 10) = 0", clamp(-5.0f, 0.0f, 10.0f) == 0.0f);
    TEST("clamp(15, 0, 10) = 10", clamp(15.0f, 0.0f, 10.0f) == 10.0f);
    TEST("clamp(0, -1, 1) = 0", clamp(0.0f, -1.0f, 1.0f) == 0.0f);

    return failures;
}
