/**
 * @file test_xline.cpp
 * @brief Unit tests for XLine UGen
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/XLine.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_xline() {
    int failures = 0;

    // Test initialization
    {
        XLine line;
        line.init(48000.0f);
        TEST("XLine init: value starts at 1.0", line.value == 1.0f);
        TEST("XLine init: start is 1.0", line.start == 1.0f);
        TEST("XLine init: end is 2.0", line.end == 2.0f);
        TEST("XLine init: dur is 1.0", line.dur == 1.0f);
        TEST("XLine init: sample rate is set", line.sampleRate == 48000.0f);
        TEST("XLine init: mul is 1.0", line.mul == 1.0f);
        TEST("XLine init: add is 0.0", line.add == 0.0f);
        TEST("XLine init: not done", !line.isDone());
    }

    // Test set parameters
    {
        XLine line;
        line.init(48000.0f);
        line.set(2.0f, 4.0f, 0.5f, 2.0f, 1.0f);
        TEST("XLine set: start is 2.0", line.start == 2.0f);
        TEST("XLine set: end is 4.0", line.end == 4.0f);
        TEST("XLine set: dur is 0.5", line.dur == 0.5f);
        TEST("XLine set: mul is 2.0", line.mul == 2.0f);
        TEST("XLine set: add is 1.0", line.add == 1.0f);
        TEST("XLine set: value starts at start", line.value == 2.0f);
    }

    // Test exponential growth from 1.0 to 2.0
    {
        XLine line;
        line.init(48000.0f);
        line.set(1.0f, 2.0f, 1.0f);  // 1 second from 1.0 to 2.0

        Sample first = line.tick();
        TEST("XLine tick: first value is 1.0", std::abs(first - 1.0f) < 0.001f);

        // Run for half the duration
        for (int i = 0; i < 23999; ++i) line.tick();

        // At halfway point, exponential should be sqrt(2) ≈ 1.414
        Sample midValue = line.value;
        TEST("XLine tick: midpoint value ~1.414", std::abs(midValue - 1.414f) < 0.01f);
    }

    // Test that value increases monotonically for start < end
    {
        XLine line;
        line.init(48000.0f);
        line.set(1.0f, 10.0f, 0.1f);  // 0.1 seconds from 1.0 to 10.0

        Sample prev = line.tick();
        bool increasing = true;
        for (int i = 0; i < 4800 - 1 && !line.isDone(); ++i) {
            Sample val = line.tick();
            if (val < prev) {
                increasing = false;
                break;
            }
            prev = val;
        }
        TEST("XLine: value increases monotonically for start < end", increasing);
    }

    // Test that value decreases monotonically for start > end
    {
        XLine line;
        line.init(48000.0f);
        line.set(10.0f, 1.0f, 0.1f);  // 0.1 seconds from 10.0 to 1.0

        Sample prev = line.tick();
        bool decreasing = true;
        for (int i = 0; i < 4800 - 1 && !line.isDone(); ++i) {
            Sample val = line.tick();
            if (val > prev) {
                decreasing = false;
                break;
            }
            prev = val;
        }
        TEST("XLine: value decreases monotonically for start > end", decreasing);
    }

    // Test that line reaches end value
    {
        XLine line;
        line.init(48000.0f);
        line.set(1.0f, 4.0f, 0.1f);  // 0.1 seconds = 4800 samples

        // Run past the duration
        for (int i = 0; i < 5000; ++i) line.tick();

        TEST("XLine: value reaches end", std::abs(line.value - 4.0f) < 0.001f);
        TEST("XLine: isDone is true after completion", line.isDone());
    }

    // Test done flag
    {
        XLine line;
        line.init(48000.0f);
        line.set(1.0f, 2.0f, 0.01f);  // 0.01 seconds = 480 samples

        TEST("XLine: not done at start", !line.isDone());

        // Run through the duration
        for (int i = 0; i < 500; ++i) line.tick();

        TEST("XLine: done after duration", line.isDone());
    }

    // Test mul and add
    {
        XLine line;
        line.init(48000.0f);
        line.set(2.0f, 2.0f, 1.0f, 3.0f, 5.0f);  // value=2, mul=3, add=5

        Sample out = line.tick();
        // Expected: 2.0 * 3.0 + 5.0 = 11.0
        TEST("XLine: mul and add applied correctly", std::abs(out - 11.0f) < 0.001f);
    }

    // Test reset
    {
        XLine line;
        line.init(48000.0f);
        line.set(1.0f, 4.0f, 0.1f);

        // Run partway
        for (int i = 0; i < 2000; ++i) line.tick();

        line.reset();
        TEST("XLine reset: value returns to start", line.value == 1.0f);
        TEST("XLine reset: counter is 0", line.counter == 0.0f);
        TEST("XLine reset: not done", !line.isDone());
    }

    // Test trigger (same as reset)
    {
        XLine line;
        line.init(48000.0f);
        line.set(1.0f, 4.0f, 0.1f);

        for (int i = 0; i < 5000; ++i) line.tick();
        TEST("XLine: done before trigger", line.isDone());

        line.trigger();
        TEST("XLine trigger: value returns to start", line.value == 1.0f);
        TEST("XLine trigger: not done", !line.isDone());
    }

    // Test negative values (same sign)
    {
        XLine line;
        line.init(48000.0f);
        line.set(-1.0f, -4.0f, 0.1f);  // Negative values, same sign

        Sample first = line.tick();
        TEST("XLine negative: first value is -1.0", std::abs(first - (-1.0f)) < 0.001f);

        // Run to completion
        for (int i = 0; i < 5000; ++i) line.tick();

        TEST("XLine negative: reaches end value -4.0", std::abs(line.value - (-4.0f)) < 0.001f);
    }

    // Test zero handling (should adjust to small non-zero value)
    {
        XLine line;
        line.init(48000.0f);
        line.set(0.0f, 1.0f, 0.1f);

        TEST("XLine zero start: adjusted to non-zero", line.start != 0.0f);
    }

    // Test block processing
    {
        XLine line;
        line.init(48000.0f);
        line.set(1.0f, 2.0f, 1.0f);
        Sample buffer[64];
        line.process(buffer, 64);

        bool allValid = true;
        for (int i = 0; i < 64; ++i) {
            if (std::isnan(buffer[i]) || std::isinf(buffer[i])) {
                allValid = false;
                break;
            }
        }
        TEST("XLine process: no NaN or Inf in output", allValid);
    }

    // Test that value stays at end after completion
    {
        XLine line;
        line.init(48000.0f);
        line.set(1.0f, 4.0f, 0.01f);  // 480 samples

        // Run well past completion
        for (int i = 0; i < 1000; ++i) line.tick();

        Sample afterDone = line.tick();
        TEST("XLine: value stays at end after done", std::abs(afterDone - 4.0f) < 0.001f);
    }

    return failures;
}
