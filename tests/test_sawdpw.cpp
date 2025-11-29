/**
 * @file test_sawdpw.cpp
 * @brief Unit tests for SawDPW UGen
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/SawDPW.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_sawdpw() {
    int failures = 0;

    // Test initialization
    {
        SawDPW saw;
        saw.init(48000.0f);
        TEST("SawDPW init: phase starts at 0.5 (default iphase=0)",
             std::abs(saw.phase - 0.5f) < 0.001f);
        TEST("SawDPW init: default frequency is 440", saw.frequency == 440.0f);
        TEST("SawDPW init: sample rate is set", saw.sampleRate == 48000.0f);
        TEST("SawDPW init: prevParabolic starts at 0", saw.prevParabolic == 0.0f);
    }

    // Test initial phase offset
    {
        SawDPW saw;
        saw.init(48000.0f, -1.0f);  // iphase = -1 should map to phase = 0
        TEST("SawDPW init: iphase -1.0 maps to phase 0.0",
             std::abs(saw.phase - 0.0f) < 0.001f);

        SawDPW saw2;
        saw2.init(48000.0f, 1.0f);  // iphase = 1 should map to phase = 1
        TEST("SawDPW init: iphase 1.0 maps to phase 1.0",
             std::abs(saw2.phase - 1.0f) < 0.001f);
    }

    // Test tick produces values in range
    {
        SawDPW saw;
        saw.init(48000.0f);
        saw.setFrequency(440.0f);

        bool inRange = true;
        Sample maxVal = 0.0f;
        Sample minVal = 0.0f;

        // Generate several cycles
        for (int i = 0; i < 10000; ++i) {
            Sample s = saw.tick();
            if (s > maxVal) maxVal = s;
            if (s < minVal) minVal = s;

            // Allow some headroom beyond [-1, 1] due to DPW characteristics
            if (s < -2.0f || s > 2.0f) {
                inRange = false;
                break;
            }
        }
        TEST("SawDPW tick: output in reasonable range", inRange);
        TEST("SawDPW tick: output has positive values", maxVal > 0.5f);
        TEST("SawDPW tick: output has negative values", minVal < -0.5f);
    }

    // Test frequency setting
    {
        SawDPW saw;
        saw.init(48000.0f);
        saw.setFrequency(1000.0f);
        TEST("SawDPW setFrequency: frequency is updated", saw.frequency == 1000.0f);

        // Phase increment should be f/sr
        Sample expectedIncrement = 1000.0f / 48000.0f;
        TEST("SawDPW setFrequency: phase increment is correct",
             std::abs(saw.phaseIncrement - expectedIncrement) < 0.0001f);
    }

    // Test DC offset (should be close to zero over time)
    {
        SawDPW saw;
        saw.init(48000.0f);
        saw.setFrequency(440.0f);

        Sample sum = 0.0f;
        const int numSamples = 48000;  // 1 second
        for (int i = 0; i < numSamples; ++i) {
            sum += saw.tick();
        }
        Sample dcOffset = sum / numSamples;
        TEST("SawDPW: DC offset is close to zero", std::abs(dcOffset) < 0.1f);
    }

    // Test spectral richness (sawtooth should have harmonics)
    {
        SawDPW saw;
        saw.init(48000.0f);
        saw.setFrequency(100.0f);  // 100 Hz

        // Generate one period and verify it's not a simple waveform
        const int period = 480;
        Sample buffer[480];

        for (int i = 0; i < period; ++i) {
            buffer[i] = saw.tick();
        }

        // Check that output varies throughout the period
        bool hasVariation = false;
        Sample first = buffer[0];
        for (int i = 1; i < period; ++i) {
            if (std::abs(buffer[i] - first) > 0.3f) {
                hasVariation = true;
                break;
            }
        }
        TEST("SawDPW: output has spectral variation", hasVariation);
    }

    // Test block processing
    {
        SawDPW saw;
        saw.init(48000.0f);
        Sample buffer[64];
        saw.process(buffer, 64);

        bool allValid = true;
        for (int i = 0; i < 64; ++i) {
            if (std::isnan(buffer[i]) || std::isinf(buffer[i])) {
                allValid = false;
                break;
            }
        }
        TEST("SawDPW process: no NaN or Inf in output", allValid);
    }

    // Test reset
    {
        SawDPW saw;
        saw.init(48000.0f);
        for (int i = 0; i < 100; ++i) saw.tick();
        saw.reset();
        TEST("SawDPW reset: phase returns to 0", saw.phase == 0.0f);
        TEST("SawDPW reset: prevParabolic returns to 0", saw.prevParabolic == 0.0f);
    }

    // Test different frequencies produce different waveforms
    {
        SawDPW saw1, saw2;
        saw1.init(48000.0f);
        saw2.init(48000.0f);
        saw1.setFrequency(220.0f);
        saw2.setFrequency(440.0f);

        bool different = false;
        for (int i = 0; i < 100; ++i) {
            Sample s1 = saw1.tick();
            Sample s2 = saw2.tick();
            if (std::abs(s1 - s2) > 0.01f) {
                different = true;
                break;
            }
        }
        TEST("SawDPW: different frequencies produce different output", different);
    }

    // Test sawtooth shape (should have sharp negative jump)
    {
        SawDPW saw;
        saw.init(48000.0f);
        saw.setFrequency(100.0f);  // Low frequency for clear observation

        bool foundJump = false;
        Sample prev = saw.tick();
        for (int i = 0; i < 1000; ++i) {
            Sample curr = saw.tick();
            // Look for sharp negative transition (sawtooth reset)
            if ((curr - prev) < -1.0f) {
                foundJump = true;
                break;
            }
            prev = curr;
        }
        TEST("SawDPW: exhibits sawtooth characteristic (sharp negative jump)", foundJump);
    }

    // Test harmonic content is rich (sawtooth should have odd and even harmonics)
    {
        SawDPW saw;
        saw.init(48000.0f);
        saw.setFrequency(440.0f);

        // Generate samples and check for variety
        bool hasVariety = false;
        Sample samples[100];
        for (int i = 0; i < 100; ++i) {
            samples[i] = saw.tick();
        }

        // Check that not all samples are similar
        Sample first = samples[0];
        for (int i = 1; i < 100; ++i) {
            if (std::abs(samples[i] - first) > 0.5f) {
                hasVariety = true;
                break;
            }
        }
        TEST("SawDPW: output has harmonic variety", hasVariety);
    }

    return failures;
}
