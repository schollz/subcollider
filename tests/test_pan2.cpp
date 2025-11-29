/**
 * @file test_pan2.cpp
 * @brief Unit tests for Pan2 UGen
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/Pan2.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_pan2() {
    int failures = 0;

    // Test center pan (0.0) creates equal left/right
    {
        Pan2 panner;
        Sample input = 1.0f;
        Stereo output = panner.process(input, 0.0f);

        // At center, both channels should be equal and about 0.707 (1/sqrt(2))
        // due to equal-power panning
        TEST("Pan2 center: left == right", std::abs(output.left - output.right) < 0.001f);
        TEST("Pan2 center: value ~0.707", std::abs(output.left - 0.707f) < 0.01f);
    }

    // Test full left pan (-1.0)
    {
        Pan2 panner;
        Sample input = 1.0f;
        Stereo output = panner.process(input, -1.0f);

        // Full left: left should be ~1.0, right should be ~0.0
        TEST("Pan2 full left: left channel ~1.0", std::abs(output.left - 1.0f) < 0.01f);
        TEST("Pan2 full left: right channel ~0.0", std::abs(output.right - 0.0f) < 0.01f);
    }

    // Test full right pan (+1.0)
    {
        Pan2 panner;
        Sample input = 1.0f;
        Stereo output = panner.process(input, 1.0f);

        // Full right: left should be ~0.0, right should be ~1.0
        TEST("Pan2 full right: left channel ~0.0", std::abs(output.left - 0.0f) < 0.01f);
        TEST("Pan2 full right: right channel ~1.0", std::abs(output.right - 1.0f) < 0.01f);
    }

    // Test half-left pan (-0.5)
    {
        Pan2 panner;
        Sample input = 1.0f;
        Stereo output = panner.process(input, -0.5f);

        // Half-left: left should be louder than right
        TEST("Pan2 half-left: left > right", output.left > output.right);
    }

    // Test half-right pan (+0.5)
    {
        Pan2 panner;
        Sample input = 1.0f;
        Stereo output = panner.process(input, 0.5f);

        // Half-right: right should be louder than left
        TEST("Pan2 half-right: right > left", output.right > output.left);
    }

    // Test equal-power panning law (constant power)
    {
        Pan2 panner;
        Sample input = 1.0f;

        bool powerConstant = true;
        Sample expectedPower = 1.0f; // For unit input, power should be ~1.0

        for (Sample pan = -1.0f; pan <= 1.0f; pan += 0.1f) {
            Stereo output = panner.process(input, pan);
            Sample power = output.left * output.left + output.right * output.right;

            // Power should be constant (within tolerance)
            if (std::abs(power - expectedPower) > 0.01f) {
                powerConstant = false;
                break;
            }
        }
        TEST("Pan2 equal-power: constant power across pan range", powerConstant);
    }

    // Test input signal scaling
    {
        Pan2 panner;
        Sample input = 0.5f;
        Stereo output = panner.process(input, 0.0f);

        // Output should scale with input
        TEST("Pan2 scaling: output scales with input", std::abs(output.left - 0.3535f) < 0.01f);
    }

    // Test clamping of pan values
    {
        Pan2 panner;
        Sample input = 1.0f;

        // Test pan value > 1.0 (should be clamped to 1.0)
        Stereo output1 = panner.process(input, 2.0f);
        Stereo output2 = panner.process(input, 1.0f);
        TEST("Pan2 clamp: pan > 1.0 clamped",
             std::abs(output1.left - output2.left) < 0.001f &&
             std::abs(output1.right - output2.right) < 0.001f);

        // Test pan value < -1.0 (should be clamped to -1.0)
        Stereo output3 = panner.process(input, -2.0f);
        Stereo output4 = panner.process(input, -1.0f);
        TEST("Pan2 clamp: pan < -1.0 clamped",
             std::abs(output3.left - output4.left) < 0.001f &&
             std::abs(output3.right - output4.right) < 0.001f);
    }

    // Test setPan() and tick() methods
    {
        Pan2 panner;
        Sample input = 1.0f;

        panner.setPan(-0.5f);
        Stereo output1 = panner.tick(input);
        Stereo output2 = panner.process(input, -0.5f);

        TEST("Pan2 setPan/tick: matches process()",
             std::abs(output1.left - output2.left) < 0.001f &&
             std::abs(output1.right - output2.right) < 0.001f);
    }

    // Test zero input
    {
        Pan2 panner;
        Stereo output = panner.process(0.0f, 0.5f);

        TEST("Pan2 zero input: left is zero", output.left == 0.0f);
        TEST("Pan2 zero input: right is zero", output.right == 0.0f);
    }

    // Test negative input
    {
        Pan2 panner;
        Sample input = -0.8f;
        Stereo output = panner.process(input, 0.0f);

        // Output should preserve sign
        TEST("Pan2 negative input: left is negative", output.left < 0.0f);
        TEST("Pan2 negative input: right is negative", output.right < 0.0f);
    }

    // Test symmetry: pan(x, p) left/right == pan(x, -p) right/left
    {
        Pan2 panner;
        Sample input = 1.0f;
        Sample pan = 0.3f;

        Stereo output1 = panner.process(input, pan);
        Stereo output2 = panner.process(input, -pan);

        TEST("Pan2 symmetry: symmetric panning",
             std::abs(output1.left - output2.right) < 0.001f &&
             std::abs(output1.right - output2.left) < 0.001f);
    }

    return failures;
}
