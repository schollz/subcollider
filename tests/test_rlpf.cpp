/**
 * @file test_rlpf.cpp
 * @brief Unit tests for RLPF (Resonant Low Pass Filter) UGen
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/RLPF.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_rlpf() {
    int failures = 0;

    std::cout << "  RLPF:" << std::endl;

    // Test initialization
    {
        RLPF filter;
        filter.init(48000.0f);
        TEST("RLPF init: sample rate is set", filter.sampleRate == 48000.0f);
        TEST("RLPF init: default freq is 440", filter.freq == 440.0f);
        TEST("RLPF init: default resonance is 0.707", std::abs(filter.resonance - 0.707f) < 0.001f);
    }

    // Test setFreq
    {
        RLPF filter;
        filter.init(48000.0f);
        filter.setFreq(1000.0f);
        TEST("RLPF setFreq: freq is updated", filter.freq == 1000.0f);
        filter.setFreq(5000.0f);
        TEST("RLPF setFreq: freq is updated to 5000", filter.freq == 5000.0f);
    }

    // Test setResonance
    {
        RLPF filter;
        filter.init(48000.0f);
        filter.setResonance(0.5f);
        TEST("RLPF setResonance: resonance is updated", std::abs(filter.resonance - 0.5f) < 0.01f);
        filter.setResonance(2.0f);
        TEST("RLPF setResonance: high resonance is set", std::abs(filter.resonance - 2.0f) < 0.01f);
    }

    // Test resonance clamping
    {
        RLPF filter;
        filter.init(48000.0f);
        filter.setResonance(0.01f);
        TEST("RLPF setResonance: very low resonance is clamped", filter.resonance >= 0.1f);
        filter.setResonance(100.0f);
        TEST("RLPF setResonance: very high resonance is clamped", filter.resonance <= 30.0f);
    }

    // Test frequency clamping
    {
        RLPF filter;
        filter.init(48000.0f);
        filter.setFreq(0.0f);
        TEST("RLPF setFreq: zero frequency is clamped", filter.freq >= 1.0f);
        filter.setFreq(30000.0f);  // Above Nyquist for 48kHz
        TEST("RLPF setFreq: above Nyquist is clamped", filter.freq < 24000.0f);
    }

    // Test tick produces valid output
    {
        RLPF filter;
        filter.init(48000.0f);
        filter.setFreq(1000.0f);
        filter.setResonance(0.707f);

        bool allValid = true;
        for (int i = 0; i < 100; ++i) {
            Sample out = filter.tick(0.5f);
            if (std::isnan(out) || std::isinf(out)) {
                allValid = false;
                break;
            }
        }
        TEST("RLPF tick: no NaN or Inf in output", allValid);
    }

    // Test block processing
    {
        RLPF filter;
        filter.init(48000.0f);
        Sample buffer[64];
        for (int i = 0; i < 64; ++i) buffer[i] = (i % 2 == 0) ? 1.0f : -1.0f;
        filter.process(buffer, 64);

        bool allValid = true;
        for (int i = 0; i < 64; ++i) {
            if (std::isnan(buffer[i]) || std::isinf(buffer[i])) {
                allValid = false;
                break;
            }
        }
        TEST("RLPF process: no NaN or Inf in output", allValid);
    }

    // Test reset
    {
        RLPF filter;
        filter.init(48000.0f);
        for (int i = 0; i < 100; ++i) filter.tick(0.5f);
        filter.reset();
        Sample out = filter.tick(0.0f);
        TEST("RLPF reset: filter state resets", std::abs(out) < 0.1f);
    }

    // Test low pass filtering behavior
    // High frequency content should be attenuated
    {
        RLPF filter;
        filter.init(48000.0f);
        filter.setFreq(500.0f);  // Low cutoff
        filter.setResonance(0.707f);

        // Feed alternating +1/-1 (high frequency content - Nyquist)
        Sample peakOutput = 0.0f;
        for (int i = 0; i < 1000; ++i) {
            Sample input = (i % 2 == 0) ? 1.0f : -1.0f;
            Sample out = filter.tick(input);
            if (std::abs(out) > peakOutput) peakOutput = std::abs(out);
        }
        // High frequency should be heavily attenuated with 500Hz cutoff
        TEST("RLPF lowpass: high frequency is attenuated", peakOutput < 0.5f);
    }

    // Test that DC signal passes through
    {
        RLPF filter;
        filter.init(48000.0f);
        filter.setFreq(1000.0f);
        filter.setResonance(0.707f);

        // Feed constant signal (DC)
        Sample output = 0.0f;
        for (int i = 0; i < 10000; ++i) {
            output = filter.tick(1.0f);
        }
        // DC should pass through a lowpass filter
        TEST("RLPF lowpass: DC passes through", std::abs(output - 1.0f) < 0.01f);
    }

    // Test high resonance creates peak
    {
        RLPF filter1, filter2;
        filter1.init(48000.0f);
        filter1.setFreq(1000.0f);
        filter1.setResonance(0.5f);

        filter2.init(48000.0f);
        filter2.setFreq(1000.0f);
        filter2.setResonance(5.0f);

        // Feed impulse and measure response
        Sample peak1 = 0.0f, peak2 = 0.0f;
        for (int i = 0; i < 500; ++i) {
            Sample input = (i == 0) ? 1.0f : 0.0f;
            Sample out1 = filter1.tick(input);
            Sample out2 = filter2.tick(input);
            if (std::abs(out1) > peak1) peak1 = std::abs(out1);
            if (std::abs(out2) > peak2) peak2 = std::abs(out2);
        }
        // Higher resonance should create higher peak response
        TEST("RLPF resonance: higher Q creates higher peak", peak2 > peak1);
    }

    // Test filter stability at extreme settings
    {
        RLPF filter;
        filter.init(48000.0f);
        filter.setFreq(20000.0f);  // High frequency
        filter.setResonance(10.0f);  // High resonance

        bool stable = true;
        for (int i = 0; i < 10000; ++i) {
            Sample input = static_cast<Sample>(std::sin(i * 0.1));
            Sample out = filter.tick(input);
            if (std::isnan(out) || std::isinf(out) || std::abs(out) > 1000.0f) {
                stable = false;
                break;
            }
        }
        TEST("RLPF stability: stable at extreme settings", stable);
    }

    // Test with different sample rates
    {
        RLPF filter;
        filter.init(44100.0f);
        filter.setFreq(1000.0f);
        TEST("RLPF sample rate: works at 44100", filter.sampleRate == 44100.0f);

        filter.init(96000.0f);
        filter.setFreq(5000.0f);
        bool valid = true;
        for (int i = 0; i < 100; ++i) {
            Sample out = filter.tick(0.5f);
            if (std::isnan(out) || std::isinf(out)) {
                valid = false;
                break;
            }
        }
        TEST("RLPF sample rate: produces valid output at 96000", valid);
    }

    return failures;
}
