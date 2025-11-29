/**
 * @file test_lfnoise2.cpp
 * @brief Unit tests for LFNoise2 UGen
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/LFNoise2.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_lfnoise2() {
    int failures = 0;

    // Test initialization
    {
        LFNoise2 noise;
        noise.init(48000.0f);
        TEST("LFNoise2 init: phase starts at 0", noise.phase == 0.0f);
        TEST("LFNoise2 init: default frequency is 1", noise.frequency == 1.0f);
        TEST("LFNoise2 init: sample rate is set", noise.sampleRate == 48000.0f);
    }

    // Test output in range [-1, 1]
    {
        LFNoise2 noise;
        noise.init(48000.0f);
        noise.setFrequency(10.0f);

        bool inRange = true;
        for (int i = 0; i < 10000; ++i) {
            Sample s = noise.tick();
            if (s < -1.0f || s > 1.0f) {
                inRange = false;
                break;
            }
        }
        TEST("LFNoise2 tick: output in range [-1, 1]", inRange);
    }

    // Test smoothness (quadratic interpolation should produce smooth output)
    {
        LFNoise2 noise;
        noise.init(48000.0f);
        noise.setFrequency(1.0f);  // Slow changes

        Sample prev = noise.tick();
        Sample maxDiff = 0.0f;
        for (int i = 0; i < 1000; ++i) {
            Sample curr = noise.tick();
            Sample diff = std::abs(curr - prev);
            if (diff > maxDiff) maxDiff = diff;
            prev = curr;
        }
        // With quadratic interpolation at 1Hz on 48kHz, changes should be tiny
        TEST("LFNoise2 tick: output is smooth", maxDiff < 0.01f);
    }

    // Test deterministic with same seed
    {
        LFNoise2 noise1, noise2;
        noise1.init(48000.0f, 12345);
        noise2.init(48000.0f, 12345);
        noise1.setFrequency(10.0f);
        noise2.setFrequency(10.0f);

        bool identical = true;
        for (int i = 0; i < 1000; ++i) {
            if (noise1.tick() != noise2.tick()) {
                identical = false;
                break;
            }
        }
        TEST("LFNoise2: same seed produces same output", identical);
    }

    // Test different seed produces different output
    {
        LFNoise2 noise1, noise2;
        noise1.init(48000.0f, 12345);
        noise2.init(48000.0f, 67890);
        noise1.setFrequency(10.0f);
        noise2.setFrequency(10.0f);

        bool different = false;
        for (int i = 0; i < 100; ++i) {
            if (noise1.tick() != noise2.tick()) {
                different = true;
                break;
            }
        }
        TEST("LFNoise2: different seed produces different output", different);
    }

    // Test block processing
    {
        LFNoise2 noise;
        noise.init(48000.0f);
        noise.setFrequency(5.0f);
        Sample buffer[64];
        noise.process(buffer, 64);

        bool allValid = true;
        for (int i = 0; i < 64; ++i) {
            if (std::isnan(buffer[i]) || std::isinf(buffer[i])) {
                allValid = false;
                break;
            }
        }
        TEST("LFNoise2 process: no NaN or Inf in output", allValid);
    }

    // Test reset
    {
        LFNoise2 noise;
        noise.init(48000.0f);
        for (int i = 0; i < 1000; ++i) noise.tick();

        noise.reset(12345);
        TEST("LFNoise2 reset: phase returns to 0", noise.phase == 0.0f);
    }

    return failures;
}
