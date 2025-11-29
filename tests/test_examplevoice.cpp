/**
 * @file test_examplevoice.cpp
 * @brief Unit tests for ExampleVoice
 */

#include <iostream>
#include <cmath>
#include <subcollider/ExampleVoice.h>

using namespace subcollider;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_examplevoice() {
    int failures = 0;

    // Test initialization
    {
        ExampleVoice voice;
        voice.init(48000.0f);
        TEST("ExampleVoice init: base frequency is 440", voice.baseFrequency == 440.0f);
        TEST("ExampleVoice init: sample rate is set", voice.sampleRate == 48000.0f);
        TEST("ExampleVoice init: amplitude is 0.5", voice.amplitude == 0.5f);
        TEST("ExampleVoice init: not active initially", !voice.isActive());
    }

    // Test output is silent when not triggered
    {
        ExampleVoice voice;
        voice.init(48000.0f);

        bool allSilent = true;
        for (int i = 0; i < 100; ++i) {
            if (voice.tick() != 0.0f) {
                allSilent = false;
                break;
            }
        }
        TEST("ExampleVoice: silent when not triggered", allSilent);
    }

    // Test output is non-zero when triggered
    {
        ExampleVoice voice;
        voice.init(48000.0f);
        voice.trigger();

        bool hasOutput = false;
        for (int i = 0; i < 100; ++i) {
            if (voice.tick() != 0.0f) {
                hasOutput = true;
                break;
            }
        }
        TEST("ExampleVoice: produces output when triggered", hasOutput);
    }

    // Test output is in valid range
    {
        ExampleVoice voice;
        voice.init(48000.0f);
        voice.setAmplitude(1.0f);
        voice.trigger();

        bool inRange = true;
        for (int i = 0; i < 10000; ++i) {
            if (i == 500) voice.release();
            Sample s = voice.tick();
            // Should be in range [-1, 1] with amplitude 1.0
            if (s < -1.1f || s > 1.1f) {  // Small tolerance for floating point
                inRange = false;
                break;
            }
        }
        TEST("ExampleVoice: output in reasonable range", inRange);
    }

    // Test voice becomes inactive after release
    {
        ExampleVoice voice;
        voice.init(48000.0f);
        voice.setAttack(0.001f);
        voice.setRelease(0.001f);
        voice.trigger();

        for (int i = 0; i < 500; ++i) voice.tick();

        voice.release();

        for (int i = 0; i < 1000; ++i) voice.tick();

        TEST("ExampleVoice: becomes inactive after release", !voice.isActive());
    }

    // Test frequency setting
    {
        ExampleVoice voice;
        voice.init(48000.0f);
        voice.setFrequency(880.0f);
        TEST("ExampleVoice setFrequency: base frequency updated", voice.baseFrequency == 880.0f);
    }

    // Test vibrato settings
    {
        ExampleVoice voice;
        voice.init(48000.0f);
        voice.setVibratoDepth(0.5f);
        voice.setVibratoRate(6.0f);
        TEST("ExampleVoice setVibratoDepth: depth updated", voice.vibratoDepth == 0.5f);
    }

    // Test block processing
    {
        ExampleVoice voice;
        voice.init(48000.0f);
        voice.trigger();

        Sample buffer[64] = {};
        voice.process(buffer, 64);

        bool hasOutput = false;
        bool allValid = true;
        for (int i = 0; i < 64; ++i) {
            if (buffer[i] != 0.0f) hasOutput = true;
            if (std::isnan(buffer[i]) || std::isinf(buffer[i])) {
                allValid = false;
                break;
            }
        }
        TEST("ExampleVoice process: produces output", hasOutput);
        TEST("ExampleVoice process: no NaN or Inf", allValid);
    }

    // Test processAdd
    {
        ExampleVoice voice;
        voice.init(48000.0f);
        voice.trigger();

        Sample buffer[64];
        for (int i = 0; i < 64; ++i) buffer[i] = 1.0f;

        voice.processAdd(buffer, 64);

        bool added = false;
        for (int i = 0; i < 64; ++i) {
            if (buffer[i] != 1.0f) {
                added = true;
                break;
            }
        }
        TEST("ExampleVoice processAdd: adds to buffer", added);
    }

    // Test reset
    {
        ExampleVoice voice;
        voice.init(48000.0f);
        voice.trigger();
        for (int i = 0; i < 100; ++i) voice.tick();

        voice.reset();
        TEST("ExampleVoice reset: not active", !voice.isActive());
    }

    // Test gate control
    {
        ExampleVoice voice;
        voice.init(48000.0f);

        voice.setGate(true);
        TEST("ExampleVoice setGate(true): becomes active", voice.isActive());

        voice.setGate(false);
        // Should be in release phase, still active
        TEST("ExampleVoice setGate(false): still active (releasing)", voice.isActive());
    }

    return failures;
}
