/**
 * @file test_sinosc.cpp
 * @brief Unit tests for SinOsc UGen
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/SinOsc.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_sinosc() {
    int failures = 0;

    // Test initialization
    {
        SinOsc osc;
        osc.init(48000.0f);
        TEST("SinOsc init: phase starts at 0", osc.phase == 0.0f);
        TEST("SinOsc init: default frequency is 440", osc.frequency == 440.0f);
        TEST("SinOsc init: sample rate is set", osc.sampleRate == 48000.0f);
    }

    // Test tick produces values in range
    {
        SinOsc osc;
        osc.init(48000.0f);
        osc.setFrequency(440.0f);

        bool inRange = true;
        for (int i = 0; i < 1000; ++i) {
            Sample s = osc.tick();
            if (s < -1.0f || s > 1.0f) {
                inRange = false;
                break;
            }
        }
        TEST("SinOsc tick: output in range [-1, 1]", inRange);
    }

    // Test first sample is ~0 (sin(0) = 0)
    {
        SinOsc osc;
        osc.init(48000.0f);
        Sample first = osc.tick();
        TEST("SinOsc tick: first sample is ~0", std::abs(first) < 0.001f);
    }

    // Test frequency setting
    {
        SinOsc osc;
        osc.init(48000.0f);
        osc.setFrequency(1000.0f);
        TEST("SinOsc setFrequency: frequency is updated", osc.frequency == 1000.0f);

        // Phase increment should be (2*PI*f)/sr
        Sample expectedIncrement = (TWO_PI * 1000.0f) / 48000.0f;
        TEST("SinOsc setFrequency: phase increment is correct",
             std::abs(osc.phaseIncrement - expectedIncrement) < 0.0001f);
    }

    // Test block processing
    {
        SinOsc osc;
        osc.init(48000.0f);
        Sample buffer[64];
        osc.process(buffer, 64);

        bool allValid = true;
        for (int i = 0; i < 64; ++i) {
            if (std::isnan(buffer[i]) || std::isinf(buffer[i])) {
                allValid = false;
                break;
            }
        }
        TEST("SinOsc process: no NaN or Inf in output", allValid);
    }

    // Test reset
    {
        SinOsc osc;
        osc.init(48000.0f);
        for (int i = 0; i < 100; ++i) osc.tick();
        osc.reset();
        TEST("SinOsc reset: phase returns to 0", osc.phase == 0.0f);
    }

    return failures;
}
