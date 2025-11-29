/**
 * @file test_balance2.cpp
 * @brief Unit tests for Balance2 UGen
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/Balance2.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_balance2() {
    int failures = 0;

    // Test center position (0.0) creates equal left/right at 0.707
    {
        Balance2 balance;
        Sample leftIn = 1.0f;
        Sample rightIn = 1.0f;
        Stereo output = balance.process(leftIn, rightIn, 0.0f);

        // At center, both channels should be about 0.707 (1/sqrt(2))
        // due to equal-power balancing
        TEST("Balance2 center: left ~0.707", std::abs(output.left - 0.707f) < 0.01f);
        TEST("Balance2 center: right ~0.707", std::abs(output.right - 0.707f) < 0.01f);
    }

    // Test full left position (-1.0)
    {
        Balance2 balance;
        Sample leftIn = 1.0f;
        Sample rightIn = 1.0f;
        Stereo output = balance.process(leftIn, rightIn, -1.0f);

        // Full left: left should be ~1.0, right should be ~0.0
        TEST("Balance2 full left: left channel ~1.0", std::abs(output.left - 1.0f) < 0.01f);
        TEST("Balance2 full left: right channel ~0.0", std::abs(output.right - 0.0f) < 0.01f);
    }

    // Test full right position (+1.0)
    {
        Balance2 balance;
        Sample leftIn = 1.0f;
        Sample rightIn = 1.0f;
        Stereo output = balance.process(leftIn, rightIn, 1.0f);

        // Full right: left should be ~0.0, right should be ~1.0
        TEST("Balance2 full right: left channel ~0.0", std::abs(output.left - 0.0f) < 0.01f);
        TEST("Balance2 full right: right channel ~1.0", std::abs(output.right - 1.0f) < 0.01f);
    }

    // Test different input levels
    {
        Balance2 balance;
        Sample leftIn = 0.5f;
        Sample rightIn = 0.8f;
        Stereo output = balance.process(leftIn, rightIn, 0.0f);

        // At center, outputs should be inputs * 0.707
        TEST("Balance2 different inputs: left scaled", std::abs(output.left - (0.5f * 0.707f)) < 0.01f);
        TEST("Balance2 different inputs: right scaled", std::abs(output.right - (0.8f * 0.707f)) < 0.01f);
    }

    // Test level parameter
    {
        Balance2 balance;
        Sample leftIn = 1.0f;
        Sample rightIn = 1.0f;
        Sample level = 0.5f;
        Stereo output = balance.process(leftIn, rightIn, 0.0f, level);

        // With level=0.5, outputs should be half of normal
        TEST("Balance2 level: left scaled by level", std::abs(output.left - (0.707f * 0.5f)) < 0.01f);
        TEST("Balance2 level: right scaled by level", std::abs(output.right - (0.707f * 0.5f)) < 0.01f);
    }

    // Test half-left position (-0.5)
    {
        Balance2 balance;
        Sample leftIn = 1.0f;
        Sample rightIn = 1.0f;
        Stereo output = balance.process(leftIn, rightIn, -0.5f);

        // Half-left: left should be louder than right
        TEST("Balance2 half-left: left > right", output.left > output.right);
    }

    // Test half-right position (+0.5)
    {
        Balance2 balance;
        Sample leftIn = 1.0f;
        Sample rightIn = 1.0f;
        Stereo output = balance.process(leftIn, rightIn, 0.5f);

        // Half-right: right should be louder than left
        TEST("Balance2 half-right: right > left", output.right > output.left);
    }

    // Test equal-power balancing law (constant power with equal inputs)
    {
        Balance2 balance;
        Sample leftIn = 1.0f;
        Sample rightIn = 1.0f;

        bool powerConstant = true;
        Sample expectedPower = 1.0f; // For unit inputs, power should be ~1.0

        for (Sample pos = -1.0f; pos <= 1.0f; pos += 0.1f) {
            Stereo output = balance.process(leftIn, rightIn, pos);
            Sample power = output.left * output.left + output.right * output.right;

            // Power should be constant (within tolerance)
            if (std::abs(power - expectedPower) > 0.01f) {
                powerConstant = false;
                break;
            }
        }
        TEST("Balance2 equal-power: constant power across position range", powerConstant);
    }

    // Test clamping of position values
    {
        Balance2 balance;
        Sample leftIn = 1.0f;
        Sample rightIn = 1.0f;

        // Test position > 1.0 (should be clamped to 1.0)
        Stereo output1 = balance.process(leftIn, rightIn, 2.0f);
        Stereo output2 = balance.process(leftIn, rightIn, 1.0f);
        TEST("Balance2 clamp: pos > 1.0 clamped",
             std::abs(output1.left - output2.left) < 0.001f &&
             std::abs(output1.right - output2.right) < 0.001f);

        // Test position < -1.0 (should be clamped to -1.0)
        Stereo output3 = balance.process(leftIn, rightIn, -2.0f);
        Stereo output4 = balance.process(leftIn, rightIn, -1.0f);
        TEST("Balance2 clamp: pos < -1.0 clamped",
             std::abs(output3.left - output4.left) < 0.001f &&
             std::abs(output3.right - output4.right) < 0.001f);
    }

    // Test setPosition() and tick() methods
    {
        Balance2 balance;
        Sample leftIn = 1.0f;
        Sample rightIn = 1.0f;

        balance.setPosition(-0.5f);
        Stereo output1 = balance.tick(leftIn, rightIn);
        Stereo output2 = balance.process(leftIn, rightIn, -0.5f);

        TEST("Balance2 setPosition/tick: matches process()",
             std::abs(output1.left - output2.left) < 0.001f &&
             std::abs(output1.right - output2.right) < 0.001f);
    }

    // Test setPosition() with level parameter
    {
        Balance2 balance;
        Sample leftIn = 1.0f;
        Sample rightIn = 1.0f;

        balance.setPosition(0.0f, 0.5f);
        Stereo output1 = balance.tick(leftIn, rightIn);
        Stereo output2 = balance.process(leftIn, rightIn, 0.0f, 0.5f);

        TEST("Balance2 setPosition with level: matches process()",
             std::abs(output1.left - output2.left) < 0.001f &&
             std::abs(output1.right - output2.right) < 0.001f);
    }

    // Test zero inputs
    {
        Balance2 balance;
        Stereo output = balance.process(0.0f, 0.0f, 0.5f);

        TEST("Balance2 zero inputs: left is zero", output.left == 0.0f);
        TEST("Balance2 zero inputs: right is zero", output.right == 0.0f);
    }

    // Test negative inputs
    {
        Balance2 balance;
        Sample leftIn = -0.8f;
        Sample rightIn = -0.6f;
        Stereo output = balance.process(leftIn, rightIn, 0.0f);

        // Output should preserve sign
        TEST("Balance2 negative inputs: left is negative", output.left < 0.0f);
        TEST("Balance2 negative inputs: right is negative", output.right < 0.0f);
    }

    // Test asymmetric inputs with position
    {
        Balance2 balance;
        Sample leftIn = 0.5f;
        Sample rightIn = 1.0f;

        // At full left, should only hear left input
        Stereo outputLeft = balance.process(leftIn, rightIn, -1.0f);
        TEST("Balance2 asymmetric full left: left input preserved",
             std::abs(outputLeft.left - leftIn) < 0.01f);
        TEST("Balance2 asymmetric full left: right channel near zero",
             std::abs(outputLeft.right) < 0.01f);

        // At full right, should only hear right input
        Stereo outputRight = balance.process(leftIn, rightIn, 1.0f);
        TEST("Balance2 asymmetric full right: left channel near zero",
             std::abs(outputRight.left) < 0.01f);
        TEST("Balance2 asymmetric full right: right input preserved",
             std::abs(outputRight.right - rightIn) < 0.01f);
    }

    // Test independence of channels
    {
        Balance2 balance;

        // Left input only
        Stereo output1 = balance.process(1.0f, 0.0f, -1.0f);
        TEST("Balance2 left only: left channel has signal", output1.left > 0.9f);
        TEST("Balance2 left only: right channel is zero", std::abs(output1.right) < 0.01f);

        // Right input only
        Stereo output2 = balance.process(0.0f, 1.0f, 1.0f);
        TEST("Balance2 right only: left channel is zero", std::abs(output2.left) < 0.01f);
        TEST("Balance2 right only: right channel has signal", output2.right > 0.9f);
    }

    return failures;
}
