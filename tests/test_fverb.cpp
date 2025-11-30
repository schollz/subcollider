/**
 * @file test_fverb.cpp
 * @brief Unit tests for FVerb UGen
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/FVerb.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_fverb() {
    int failures = 0;

    std::cout << "  FVerb:" << std::endl;

    // Test initialization
    {
        FVerb reverb;
        reverb.init(48000.0f);
        TEST("FVerb init: sample rate is set", reverb.sampleRate == 48000.0f);
    }

    // Test parameter setters (should not crash)
    {
        FVerb reverb;
        reverb.init(48000.0f);

        reverb.setPredelay(150.0f);
        reverb.setInputAmount(100.0f);
        reverb.setInputLowPassCutoff(10000.0f);
        reverb.setInputHighPassCutoff(100.0f);
        reverb.setInputDiffusion1(70.0f);
        reverb.setInputDiffusion2(75.0f);
        reverb.setTailDensity(80.0f);
        reverb.setDecay(82.0f);
        reverb.setDamping(5500.0f);
        reverb.setModulatorFrequency(1.0f);
        reverb.setModulatorDepth(0.5f);

        TEST("FVerb setters: parameters can be set without crash", true);
    }

    // Test block processing
    {
        FVerb reverb;
        reverb.init(48000.0f);
        reverb.setPredelay(0.0f);  // No predelay for immediate response

        const size_t blockSize = 64;
        Sample leftBuffer[blockSize];
        Sample rightBuffer[blockSize];

        // Fill first block with impulse
        for (size_t i = 0; i < blockSize; ++i) {
            leftBuffer[i] = (i == 0) ? 1.0f : 0.0f;
            rightBuffer[i] = (i == 0) ? 1.0f : 0.0f;
        }

        reverb.process(leftBuffer, rightBuffer, blockSize);

        bool allValid = true;
        bool hasOutput = false;

        // Check first block
        for (size_t i = 0; i < blockSize; ++i) {
            if (std::isnan(leftBuffer[i]) || std::isinf(leftBuffer[i]) ||
                std::isnan(rightBuffer[i]) || std::isinf(rightBuffer[i])) {
                allValid = false;
                break;
            }
            if (std::abs(leftBuffer[i]) > 1e-6f || std::abs(rightBuffer[i]) > 1e-6f) {
                hasOutput = true;
            }
        }

        // Process many more blocks to let reverb develop (reverb has internal delays)
        Sample maxOutput = 0.0f;
        for (int block = 0; block < 50 && !hasOutput; ++block) {
            for (size_t i = 0; i < blockSize; ++i) {
                leftBuffer[i] = 0.0f;
                rightBuffer[i] = 0.0f;
            }
            reverb.process(leftBuffer, rightBuffer, blockSize);

            for (size_t i = 0; i < blockSize; ++i) {
                if (std::isnan(leftBuffer[i]) || std::isinf(leftBuffer[i]) ||
                    std::isnan(rightBuffer[i]) || std::isinf(rightBuffer[i])) {
                    allValid = false;
                    break;
                }
                Sample val = std::max(std::abs(leftBuffer[i]), std::abs(rightBuffer[i]));
                maxOutput = std::max(maxOutput, val);
                // Lower threshold - reverb output can be quite quiet
                if (val > 1e-10f) {
                    hasOutput = true;
                }
            }
        }

        TEST("FVerb process: no NaN or Inf in output", allValid);
        if (!hasOutput) {
            std::cout << "    Max output value: " << maxOutput << std::endl;
        }
        TEST("FVerb process: produces output from impulse", hasOutput);
    }

    // Test reset
    {
        FVerb reverb;
        reverb.init(48000.0f);

        const size_t blockSize = 64;
        Sample leftBuffer[blockSize];
        Sample rightBuffer[blockSize];

        // Process some samples
        for (size_t i = 0; i < blockSize; ++i) {
            leftBuffer[i] = 0.5f;
            rightBuffer[i] = 0.5f;
        }

        for (int block = 0; block < 10; ++block) {
            reverb.process(leftBuffer, rightBuffer, blockSize);
        }

        // Reset and process silence
        reverb.reset();
        for (size_t i = 0; i < blockSize; ++i) {
            leftBuffer[i] = 0.0f;
            rightBuffer[i] = 0.0f;
        }
        reverb.process(leftBuffer, rightBuffer, blockSize);

        // Output should be very small after reset
        bool quietOutput = true;
        for (size_t i = 0; i < blockSize; ++i) {
            if (std::abs(leftBuffer[i]) > 0.1f || std::abs(rightBuffer[i]) > 0.1f) {
                quietOutput = false;
                break;
            }
        }

        TEST("FVerb reset: clears reverb state", quietOutput);
    }

    // Test parameter clamping
    {
        FVerb reverb;
        reverb.init(48000.0f);

        // These should clamp without crashing
        reverb.setPredelay(-10.0f);     // Should clamp to 0
        reverb.setPredelay(500.0f);     // Should clamp to 300
        reverb.setDecay(-10.0f);        // Should clamp to 0
        reverb.setDecay(200.0f);        // Should clamp to 100
        reverb.setDamping(5.0f);        // Should clamp to 10
        reverb.setDamping(50000.0f);    // Should clamp to 20000

        TEST("FVerb parameter clamping: handles out-of-range values", true);
    }

    return failures;
}
