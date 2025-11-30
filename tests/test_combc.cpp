/**
 * @file test_combc.cpp
 * @brief Tests for CombC UGen.
 */

#include <iostream>
#include <cmath>
#include <limits>
#include <subcollider/ugens/CombC.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_combc() {
    int failures = 0;

    // Test 1: Initialization
    {
        CombC comb;
        comb.init(48000.0f, 1.0f);
        TEST("CombC init: sample rate", comb.sampleRate == 48000.0f);
        TEST("CombC init: max delay time", comb.maxDelayTime == 1.0f);
        TEST("CombC init: buffer allocated", comb.buffer != nullptr);
    }

    // Test 2: Delay time setting
    {
        CombC comb;
        comb.init(48000.0f, 1.0f);
        comb.setDelayTime(0.5f);
        TEST("CombC setDelayTime: value set", std::fabs(comb.delayTime - 0.5f) < 1e-6f);
    }

    // Test 3: Decay time setting
    {
        CombC comb;
        comb.init(48000.0f, 1.0f);
        comb.setDecayTime(2.0f);
        TEST("CombC setDecayTime: value set", std::fabs(comb.decayTime - 2.0f) < 1e-6f);
    }

    // Test 4: Feedback coefficient calculation (positive decay)
    {
        CombC comb;
        comb.init(48000.0f, 1.0f);
        comb.setDelayTime(0.1f);  // 100ms delay
        comb.setDecayTime(1.0f);  // 1 second decay

        // fb = 0.001^(0.1 / 1.0) = 0.001^0.1 ≈ 0.5012
        Sample expectedFb = std::pow(0.001f, 0.1f);
        TEST("CombC feedback: positive decay",
             std::fabs(comb.feedbackCoeff - expectedFb) < 0.01f);
    }

    // Test 5: Feedback coefficient calculation (negative decay)
    {
        CombC comb;
        comb.init(48000.0f, 1.0f);
        comb.setDelayTime(0.1f);   // 100ms delay
        comb.setDecayTime(-1.0f);  // -1 second decay (negative feedback)

        // fb = 0.001^(0.1 / 1.0) * -1 ≈ -0.5012
        Sample expectedFb = -std::pow(0.001f, 0.1f);
        TEST("CombC feedback: negative decay",
             std::fabs(comb.feedbackCoeff - expectedFb) < 0.01f);
    }

    // Test 6: Infinite decay time (positive)
    {
        CombC comb;
        comb.init(48000.0f, 1.0f);
        comb.setDelayTime(0.1f);
        comb.setDecayTime(std::numeric_limits<Sample>::infinity());

        TEST("CombC feedback: infinite positive decay",
             std::fabs(comb.feedbackCoeff - 1.0f) < 1e-6f);
    }

    // Test 7: Infinite decay time (negative)
    {
        CombC comb;
        comb.init(48000.0f, 1.0f);
        comb.setDelayTime(0.1f);
        comb.setDecayTime(-std::numeric_limits<Sample>::infinity());

        TEST("CombC feedback: infinite negative decay",
             std::fabs(comb.feedbackCoeff + 1.0f) < 1e-6f);
    }

    // Test 8: Zero decay time
    {
        CombC comb;
        comb.init(48000.0f, 1.0f);
        comb.setDelayTime(0.1f);
        comb.setDecayTime(0.0f);

        TEST("CombC feedback: zero decay",
             std::fabs(comb.feedbackCoeff) < 1e-6f);
    }

    // Test 9: Simple delay - impulse response
    {
        CombC comb;
        comb.init(48000.0f, 1.0f);
        comb.setDelayTime(0.001f);  // 1ms = 48 samples at 48kHz
        comb.setDecayTime(0.0f);    // No feedback

        // Send impulse
        Sample output = comb.tick(1.0f);
        TEST("CombC impulse: first sample is zero", std::fabs(output) < 1e-6f);

        // Wait for delay
        for (int i = 0; i < 47; ++i) {
            output = comb.tick(0.0f);
        }

        // Should see impulse after ~48 samples
        output = comb.tick(0.0f);
        TEST("CombC impulse: delayed output", std::fabs(output - 1.0f) < 0.1f);
    }

    // Test 10: Feedback creates multiple echoes
    {
        CombC comb;
        comb.init(48000.0f, 1.0f);
        comb.setDelayTime(0.001f);  // 1ms
        comb.setDecayTime(0.1f);     // Short decay for visible echoes

        // Send impulse and collect output
        Sample firstEcho = 0.0f;
        Sample secondEcho = 0.0f;

        comb.tick(1.0f);
        for (int i = 0; i < 48; ++i) {
            Sample out = comb.tick(0.0f);
            if (i == 47) firstEcho = out;
        }

        for (int i = 0; i < 48; ++i) {
            Sample out = comb.tick(0.0f);
            if (i == 47) secondEcho = out;
        }

        TEST("CombC echoes: first echo present", firstEcho > 0.5f);
        TEST("CombC echoes: second echo decayed",
             secondEcho > 0.0f && secondEcho < firstEcho);
    }

    // Test 11: Output range for sinusoidal input
    {
        CombC comb;
        comb.init(48000.0f, 1.0f);
        comb.setDelayTime(0.01f);  // 10ms
        comb.setDecayTime(0.5f);   // 500ms decay

        Sample maxOut = 0.0f;
        for (int i = 0; i < 10000; ++i) {
            Sample in = std::sin(2.0f * PI * 440.0f * i / 48000.0f);
            Sample out = comb.tick(in);
            maxOut = std::max(maxOut, std::fabs(out));
        }

        TEST("CombC output: bounded for sinusoidal input", maxOut < 5.0f);
    }

    // Test 12: Process method
    {
        CombC comb;
        comb.init(48000.0f, 1.0f);
        comb.setDelayTime(0.01f);  // 10ms delay
        comb.setDecayTime(2.0f);   // 2 second decay

        Sample buffer[512];
        for (int i = 0; i < 512; ++i) {
            buffer[i] = std::sin(2.0f * PI * 440.0f * i / 48000.0f);
        }

        comb.process(buffer, 512);

        // Check output is reasonable
        bool hasOutput = false;
        bool allBounded = true;
        for (int i = 0; i < 512; ++i) {
            if (std::fabs(buffer[i]) > 0.01f) {
                hasOutput = true;
            }
            if (std::fabs(buffer[i]) >= 10.0f) {
                allBounded = false;
            }
        }
        TEST("CombC process: output bounded", allBounded);
        TEST("CombC process: produces output", hasOutput);
    }

    // Test 13: Reset clears state
    {
        CombC comb;
        comb.init(48000.0f, 1.0f);
        comb.setDelayTime(0.01f);
        comb.setDecayTime(1.0f);

        // Fill with signal
        for (int i = 0; i < 1000; ++i) {
            comb.tick(1.0f);
        }

        // Reset
        comb.reset();

        // Check buffer is cleared (should output near zero for zero input)
        Sample output = comb.tick(0.0f);
        TEST("CombC reset: clears buffer", std::fabs(output) < 1e-6f);
    }

    // Test 14: Delay time clamping
    {
        CombC comb;
        comb.init(48000.0f, 0.5f);  // Max 0.5s

        comb.setDelayTime(1.0f);  // Try to set beyond max
        TEST("CombC setDelayTime: clamps to max", comb.delayTime <= 0.5f);

        comb.setDelayTime(-0.1f);  // Try negative
        TEST("CombC setDelayTime: clamps to zero", comb.delayTime >= 0.0f);
    }

    // Test 15: Cubic interpolation smoothness
    // (Test that fractional delays work correctly)
    {
        CombC comb;
        comb.init(48000.0f, 1.0f);
        comb.setDelayTime(0.001042f);  // Non-integer sample delay (50.016 samples)
        comb.setDecayTime(0.0f);        // No feedback

        // Send impulse
        comb.tick(1.0f);

        // Wait for approximate delay
        for (int i = 0; i < 49; ++i) {
            comb.tick(0.0f);
        }

        // Should get interpolated output around the 50th sample
        Sample out1 = comb.tick(0.0f);
        Sample out2 = comb.tick(0.0f);

        TEST("CombC interpolation: handles fractional delays",
             out1 > 0.5f || out2 > 0.5f);
    }

    return failures;
}
