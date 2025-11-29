/**
 * @file test_lftri.cpp
 * @brief Unit tests for LFTri UGen
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/LFTri.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_lftri() {
    int failures = 0;

    // Test initialization
    {
        LFTri tri;
        tri.init(48000.0f);
        TEST("LFTri init: default frequency is 440", tri.frequency == 440.0f);
        TEST("LFTri init: sample rate is set", tri.sampleRate == 48000.0f);
    }

    // Test initial phase offset
    {
        LFTri tri;
        tri.init(48000.0f, 0.0f);  // iphase = 0 maps to phase 0.0
        TEST("LFTri init: iphase 0.0 maps to phase 0.0",
             std::abs(tri.phase - 0.0f) < 0.001f);

        LFTri tri2;
        tri2.init(48000.0f, 2.0f);  // iphase = 2 maps to phase 0.5
        TEST("LFTri init: iphase 2.0 maps to phase 0.5",
             std::abs(tri2.phase - 0.5f) < 0.001f);

        LFTri tri3;
        tri3.init(48000.0f, 4.0f);  // iphase = 4 wraps to phase 0.0
        TEST("LFTri init: iphase 4.0 wraps to phase 0.0",
             std::abs(tri3.phase - 0.0f) < 0.001f);
    }

    // Test tick produces values in range [-1, 1]
    {
        LFTri tri;
        tri.init(48000.0f);
        tri.setFrequency(440.0f);

        bool inRange = true;
        for (int i = 0; i < 1000; ++i) {
            Sample s = tri.tick();
            if (s < -1.0f || s > 1.0f) {
                inRange = false;
                break;
            }
        }
        TEST("LFTri tick: output in range [-1, 1]", inRange);
    }

    // Test first sample is -1 when iphase = 0 (trough)
    {
        LFTri tri;
        tri.init(48000.0f, 0.0f);
        Sample first = tri.tick();
        TEST("LFTri tick: first sample is -1 at iphase 0", std::abs(first - (-1.0f)) < 0.001f);
    }

    // Test first sample is +1 at iphase = 2 (peak)
    {
        LFTri tri;
        tri.init(48000.0f, 2.0f);  // phase = 0.5
        Sample first = tri.tick();
        TEST("LFTri tick: first sample is +1 at iphase 2", std::abs(first - 1.0f) < 0.001f);
    }

    // Test frequency setting
    {
        LFTri tri;
        tri.init(48000.0f);
        tri.setFrequency(1000.0f);
        TEST("LFTri setFrequency: frequency is updated", tri.frequency == 1000.0f);

        // Phase increment should be f/sr
        Sample expectedIncrement = 1000.0f / 48000.0f;
        TEST("LFTri setFrequency: phase increment is correct",
             std::abs(tri.phaseIncrement - expectedIncrement) < 0.0001f);
    }

    // Test DC offset (should be close to zero over time)
    {
        LFTri tri;
        tri.init(48000.0f);
        tri.setFrequency(440.0f);

        Sample sum = 0.0f;
        const int numSamples = 48000;  // 1 second
        for (int i = 0; i < numSamples; ++i) {
            sum += tri.tick();
        }
        Sample dcOffset = sum / numSamples;
        TEST("LFTri: DC offset is close to zero", std::abs(dcOffset) < 0.01f);
    }

    // Test triangle shape (output reaches +1 and -1)
    {
        LFTri tri;
        tri.init(48000.0f);
        tri.setFrequency(100.0f);

        Sample maxVal = -2.0f;
        Sample minVal = 2.0f;
        
        // Generate several cycles
        for (int i = 0; i < 10000; ++i) {
            Sample s = tri.tick();
            if (s > maxVal) maxVal = s;
            if (s < minVal) minVal = s;
        }
        TEST("LFTri: output reaches +1", std::abs(maxVal - 1.0f) < 0.01f);
        TEST("LFTri: output reaches -1", std::abs(minVal - (-1.0f)) < 0.01f);
    }

    // Test triangle wave symmetry (rising slope = falling slope)
    {
        LFTri tri;
        tri.init(48000.0f);
        tri.setFrequency(480.0f);  // 480 Hz = 100 samples/cycle at 48kHz

        // Count samples rising vs falling
        int risingCount = 0;
        int fallingCount = 0;
        Sample prev = tri.tick();
        
        for (int i = 0; i < 1000; ++i) {
            Sample curr = tri.tick();
            if (curr > prev) {
                risingCount++;
            } else if (curr < prev) {
                fallingCount++;
            }
            prev = curr;
        }
        
        // Should be approximately equal for symmetric triangle
        float ratio = static_cast<float>(risingCount) / static_cast<float>(fallingCount);
        TEST("LFTri: symmetric rising/falling slopes", std::abs(ratio - 1.0f) < 0.1f);
    }

    // Test block processing
    {
        LFTri tri;
        tri.init(48000.0f);
        Sample buffer[64];
        tri.process(buffer, 64);

        bool allValid = true;
        for (int i = 0; i < 64; ++i) {
            if (std::isnan(buffer[i]) || std::isinf(buffer[i])) {
                allValid = false;
                break;
            }
        }
        TEST("LFTri process: no NaN or Inf in output", allValid);
    }

    // Test processAdd
    {
        LFTri tri;
        tri.init(48000.0f);
        Sample buffer[64];
        for (int i = 0; i < 64; ++i) {
            buffer[i] = 0.5f;
        }
        tri.processAdd(buffer, 64);

        bool allAdded = true;
        for (int i = 0; i < 64; ++i) {
            // Buffer started at 0.5, triangle values are in [-1, 1]
            // So result should be in [-0.5, 1.5]
            if (buffer[i] < -0.6f || buffer[i] > 1.6f) {
                allAdded = false;
                break;
            }
        }
        TEST("LFTri processAdd: adds to existing buffer", allAdded);
    }

    // Test reset
    {
        LFTri tri;
        tri.init(48000.0f);
        for (int i = 0; i < 100; ++i) tri.tick();
        tri.reset();
        TEST("LFTri reset: phase returns to 0", tri.phase == 0.0f);
    }

    // Test different frequencies produce different waveforms
    {
        LFTri tri1, tri2;
        tri1.init(48000.0f);
        tri2.init(48000.0f);
        tri1.setFrequency(220.0f);
        tri2.setFrequency(440.0f);

        bool different = false;
        for (int i = 0; i < 100; ++i) {
            Sample s1 = tri1.tick();
            Sample s2 = tri2.tick();
            if (std::abs(s1 - s2) > 0.01f) {
                different = true;
                break;
            }
        }
        TEST("LFTri: different frequencies produce different output", different);
    }

    // Test continuity (no sudden jumps in output)
    {
        LFTri tri;
        tri.init(48000.0f);
        tri.setFrequency(440.0f);

        bool continuous = true;
        Sample prev = tri.tick();
        Sample maxDiff = 0.0f;
        
        for (int i = 0; i < 10000; ++i) {
            Sample curr = tri.tick();
            Sample diff = std::abs(curr - prev);
            if (diff > maxDiff) maxDiff = diff;
            prev = curr;
        }
        
        // Max difference between samples should be small for 440Hz at 48kHz
        // Phase increment = 440/48000 ≈ 0.00917
        // Slope of triangle = 4 per phase unit, so max sample diff ≈ 4 * 0.00917 ≈ 0.037
        TEST("LFTri: continuous output (no jumps)", maxDiff < 0.1f);
    }

    return failures;
}
