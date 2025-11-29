/**
 * @file test_phasor.cpp
 * @brief Unit tests for Phasor UGen
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/Phasor.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_phasor() {
    int failures = 0;

    // Test initialization
    {
        Phasor phasor;
        phasor.init(48000.0f);
        TEST("Phasor init: sample rate is set", phasor.sampleRate == 48000.0f);
        TEST("Phasor init: rate is 1.0", phasor.rate == 1.0f);
        TEST("Phasor init: start is 0.0", phasor.start == 0.0f);
        TEST("Phasor init: end is 1.0", phasor.end == 1.0f);
        TEST("Phasor init: resetPos is 0.0", phasor.resetPos == 0.0f);
        TEST("Phasor init: value is start", phasor.value == 0.0f);
    }

    // Test set parameters
    {
        Phasor phasor;
        phasor.init(48000.0f);
        phasor.set(2.0f, 10.0f, 20.0f, 15.0f);
        TEST("Phasor set: rate is 2.0", phasor.rate == 2.0f);
        TEST("Phasor set: start is 10.0", phasor.start == 10.0f);
        TEST("Phasor set: end is 20.0", phasor.end == 20.0f);
        TEST("Phasor set: resetPos is 15.0", phasor.resetPos == 15.0f);
        TEST("Phasor set: value is start", phasor.value == 10.0f);
    }

    // Test basic ramp with rate = 1
    {
        Phasor phasor;
        phasor.init(48000.0f);
        phasor.set(1.0f, 0.0f, 10.0f);

        Sample first = phasor.tick();
        TEST("Phasor tick: first value is 0.0", std::abs(first - 0.0f) < 0.001f);

        Sample second = phasor.tick();
        TEST("Phasor tick: second value is 1.0", std::abs(second - 1.0f) < 0.001f);

        Sample third = phasor.tick();
        TEST("Phasor tick: third value is 2.0", std::abs(third - 2.0f) < 0.001f);
    }

    // Test wrap-around at end
    {
        Phasor phasor;
        phasor.init(48000.0f);
        phasor.set(1.0f, 0.0f, 5.0f);

        // Run to just before wrap
        for (int i = 0; i < 4; ++i) phasor.tick();
        Sample beforeWrap = phasor.tick(); // Should be 4.0
        TEST("Phasor wrap: value before wrap is 4.0", std::abs(beforeWrap - 4.0f) < 0.001f);

        Sample afterWrap = phasor.tick(); // Should wrap to 0.0
        TEST("Phasor wrap: value after wrap is 0.0", std::abs(afterWrap - 0.0f) < 0.001f);
    }

    // Test that end value is never output
    {
        Phasor phasor;
        phasor.init(48000.0f);
        phasor.set(1.0f, 0.0f, 10.0f);

        bool endReached = false;
        for (int i = 0; i < 100; ++i) {
            Sample val = phasor.tick();
            if (std::abs(val - 10.0f) < 0.001f) {
                endReached = true;
                break;
            }
        }
        TEST("Phasor: end value is never output", !endReached);
    }

    // Test trigger resets to resetPos
    {
        Phasor phasor;
        phasor.init(48000.0f);
        phasor.set(1.0f, 0.0f, 10.0f, 5.0f);  // resetPos = 5.0

        // Advance a few samples
        for (int i = 0; i < 3; ++i) phasor.tick();

        // Trigger with positive edge
        Sample val = phasor.tick(1.0f);  // Trigger crosses from 0 to 1
        TEST("Phasor trigger: value jumps to resetPos", std::abs(val - 5.0f) < 0.001f);
    }

    // Test trigger edge detection (non-positive to positive)
    {
        Phasor phasor;
        phasor.init(48000.0f);
        phasor.set(1.0f, 0.0f, 100.0f, 50.0f);

        // Start at 0
        Sample v1 = phasor.tick(0.0f);  // No trigger, value = 0
        TEST("Phasor edge: no trigger at 0", std::abs(v1 - 0.0f) < 0.001f);

        // Still no trigger (already at 0)
        Sample v2 = phasor.tick(0.0f);  // value = 1
        TEST("Phasor edge: no trigger still at 0", std::abs(v2 - 1.0f) < 0.001f);

        // Negative value, no trigger
        Sample v3 = phasor.tick(-1.0f);  // value = 2
        TEST("Phasor edge: no trigger at negative", std::abs(v3 - 2.0f) < 0.001f);

        // Positive edge from negative to positive - trigger!
        Sample v4 = phasor.tick(1.0f);  // Trigger! value = 50
        TEST("Phasor edge: trigger on positive edge", std::abs(v4 - 50.0f) < 0.001f);

        // Staying positive - no new trigger
        Sample v5 = phasor.tick(1.0f);  // value = 51
        TEST("Phasor edge: no trigger staying positive", std::abs(v5 - 51.0f) < 0.001f);
    }

    // Test setFrequency
    {
        Phasor phasor;
        phasor.init(48000.0f);
        phasor.set(0.0f, 0.0f, 1.0f);  // rate will be set by setFrequency
        phasor.setFrequency(1.0f);  // 1 Hz

        // rate = (end - start) * freq / sr = (1 - 0) * 1 / 48000
        Sample expectedRate = 1.0f / 48000.0f;
        TEST("Phasor setFrequency: rate is correct",
             std::abs(phasor.rate - expectedRate) < 0.0000001f);

        // After ~47520 samples (48000 * 0.99), should be near 0.99
        // Note: exact number depends on floating point precision
        phasor.reset();
        for (int i = 0; i < 47520; ++i) phasor.tick();
        Sample nearEnd = phasor.tick();
        TEST("Phasor setFrequency: near end of cycle",
             nearEnd > 0.98f && nearEnd < 1.0f);
    }

    // Test individual setters
    {
        Phasor phasor;
        phasor.init(48000.0f);

        phasor.setRate(3.0f);
        TEST("Phasor setRate: rate is 3.0", phasor.rate == 3.0f);

        phasor.setStart(5.0f);
        TEST("Phasor setStart: start is 5.0", phasor.start == 5.0f);

        phasor.setEnd(15.0f);
        TEST("Phasor setEnd: end is 15.0", phasor.end == 15.0f);

        phasor.setResetPos(10.0f);
        TEST("Phasor setResetPos: resetPos is 10.0", phasor.resetPos == 10.0f);
    }

    // Test reset
    {
        Phasor phasor;
        phasor.init(48000.0f);
        phasor.set(1.0f, 0.0f, 10.0f);

        // Advance
        for (int i = 0; i < 5; ++i) phasor.tick();

        phasor.reset();
        TEST("Phasor reset: value is start", phasor.value == 0.0f);
        TEST("Phasor reset: prevTrig is 0", phasor.prevTrig == 0.0f);
    }

    // Test reset with position
    {
        Phasor phasor;
        phasor.init(48000.0f);
        phasor.set(1.0f, 0.0f, 10.0f);

        phasor.reset(7.5f);
        TEST("Phasor reset(pos): value is 7.5", phasor.value == 7.5f);
    }

    // Test block processing
    {
        Phasor phasor;
        phasor.init(48000.0f);
        phasor.set(1.0f, 0.0f, 100.0f);

        Sample buffer[64];
        phasor.process(buffer, 64);

        bool allValid = true;
        for (int i = 0; i < 64; ++i) {
            if (std::isnan(buffer[i]) || std::isinf(buffer[i])) {
                allValid = false;
                break;
            }
        }
        TEST("Phasor process: no NaN or Inf in output", allValid);

        // Check values are increasing
        bool increasing = true;
        for (int i = 1; i < 64; ++i) {
            if (buffer[i] <= buffer[i - 1]) {
                increasing = false;
                break;
            }
        }
        TEST("Phasor process: values are increasing", increasing);
    }

    // Test block processing with trigger
    {
        Phasor phasor;
        phasor.init(48000.0f);
        phasor.set(1.0f, 0.0f, 100.0f, 50.0f);

        Sample buffer[64];
        Sample trig[64];
        for (int i = 0; i < 64; ++i) {
            trig[i] = (i == 32) ? 1.0f : 0.0f;  // Trigger at sample 32
        }

        phasor.process(buffer, trig, 64);

        // Sample 32 should be 50 (resetPos) due to trigger
        TEST("Phasor process with trigger: trigger resets value",
             std::abs(buffer[32] - 50.0f) < 0.001f);
    }

    // Test processAdd
    {
        Phasor phasor;
        phasor.init(48000.0f);
        phasor.set(1.0f, 0.0f, 100.0f);

        Sample buffer[64];
        for (int i = 0; i < 64; ++i) {
            buffer[i] = 100.0f;
        }

        phasor.processAdd(buffer, 64);

        // First sample should be 100.0 + 0.0 = 100.0
        TEST("Phasor processAdd: first sample correct",
             std::abs(buffer[0] - 100.0f) < 0.001f);

        // Sample 10 should be 100.0 + 10.0 = 110.0
        TEST("Phasor processAdd: adds to buffer",
             std::abs(buffer[10] - 110.0f) < 0.001f);
    }

    // Test non-zero start value
    {
        Phasor phasor;
        phasor.init(48000.0f);
        phasor.set(1.0f, 10.0f, 20.0f);

        Sample first = phasor.tick();
        TEST("Phasor non-zero start: first value is 10.0",
             std::abs(first - 10.0f) < 0.001f);

        // Run to wrap
        for (int i = 0; i < 9; ++i) phasor.tick();
        Sample atWrap = phasor.tick();
        TEST("Phasor non-zero start: wraps to 10.0",
             std::abs(atWrap - 10.0f) < 0.001f);
    }

    // Test backward ramp (start > end)
    {
        Phasor phasor;
        phasor.init(48000.0f);
        phasor.set(-1.0f, 10.0f, 5.0f);  // Negative rate, start=10, end=5

        Sample first = phasor.tick();
        TEST("Phasor backward: first value is 10.0",
             std::abs(first - 10.0f) < 0.001f);

        Sample second = phasor.tick();
        TEST("Phasor backward: second value is 9.0",
             std::abs(second - 9.0f) < 0.001f);

        // Run to wrap
        for (int i = 0; i < 3; ++i) phasor.tick();
        Sample atWrap = phasor.tick();  // Should wrap from 5 back to 10
        TEST("Phasor backward: wraps to 10.0",
             std::abs(atWrap - 10.0f) < 0.001f);
    }

    // Test fractional rate
    {
        Phasor phasor;
        phasor.init(48000.0f);
        phasor.set(0.5f, 0.0f, 10.0f);

        Sample first = phasor.tick();
        TEST("Phasor fractional rate: first value is 0.0",
             std::abs(first - 0.0f) < 0.001f);

        Sample second = phasor.tick();
        TEST("Phasor fractional rate: second value is 0.5",
             std::abs(second - 0.5f) < 0.001f);

        Sample third = phasor.tick();
        TEST("Phasor fractional rate: third value is 1.0",
             std::abs(third - 1.0f) < 0.001f);
    }

    // Test large rate (multiple wraps per sample)
    {
        Phasor phasor;
        phasor.init(48000.0f);
        phasor.set(25.0f, 0.0f, 10.0f);  // Large rate, will wrap

        (void)phasor.tick();  // 0
        Sample second = phasor.tick(); // 25 -> wraps to 5
        TEST("Phasor large rate: wraps correctly",
             std::abs(second - 5.0f) < 0.001f);
    }

    // Test continuous output over many cycles
    {
        Phasor phasor;
        phasor.init(48000.0f);
        phasor.set(1.0f, 0.0f, 100.0f);

        bool allInRange = true;
        for (int i = 0; i < 10000; ++i) {
            Sample val = phasor.tick();
            if (val < 0.0f || val >= 100.0f) {
                allInRange = false;
                break;
            }
        }
        TEST("Phasor continuous: all values in range [start, end)",
             allInRange);
    }

    return failures;
}
