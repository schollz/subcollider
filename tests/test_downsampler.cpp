/**
 * @file test_downsampler.cpp
 * @brief Unit tests for Downsampler UGen
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/Downsampler.h>
#include <subcollider/ugens/SinOsc.h>
#include <subcollider/types.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_downsampler() {
    int failures = 0;

    // Test initialization
    {
        Downsampler ds;
        ds.init(48000.0f, 2);
        TEST("Downsampler init: output sample rate is set", ds.outputSampleRate == 48000.0f);
        TEST("Downsampler init: oversample factor is set", ds.oversampleFactor == 2);
        TEST("Downsampler init: accumulator is zero", ds.accumulator == 0.0f);
        TEST("Downsampler init: sample count is zero", ds.sampleCount == 0);
    }

    // Test factor clamping
    {
        Downsampler ds;
        ds.init(48000.0f, 0);
        TEST("Downsampler factor: 0 clamped to 1", ds.oversampleFactor == 1);

        ds.init(48000.0f, 100);
        TEST("Downsampler factor: 100 clamped to MAX_OVERSAMPLE", ds.oversampleFactor == Downsampler::MAX_OVERSAMPLE);
    }

    // Test write and read with 2x oversampling
    {
        Downsampler ds;
        ds.init(48000.0f, 2);

        // Write 2 samples at high rate
        ds.write(1.0f);
        ds.write(1.0f);

        // Read should return average (after filtering)
        Sample out = ds.read();
        TEST("Downsampler 2x: produces output after 2 writes", out != 0.0f);
    }

    // Test write and read with 4x oversampling
    {
        Downsampler ds;
        ds.init(48000.0f, 4);

        // Write 4 samples at high rate
        ds.write(1.0f);
        ds.write(1.0f);
        ds.write(1.0f);
        ds.write(1.0f);

        Sample out = ds.read();
        TEST("Downsampler 4x: produces output after 4 writes", out != 0.0f);
    }

    // Test isReady()
    {
        Downsampler ds;
        ds.init(48000.0f, 2);

        TEST("Downsampler isReady: false initially", !ds.isReady());

        ds.write(1.0f);
        TEST("Downsampler isReady: false after 1 write", !ds.isReady());

        ds.write(1.0f);
        TEST("Downsampler isReady: true after 2 writes", ds.isReady());
    }

    // Test reset
    {
        Downsampler ds;
        ds.init(48000.0f, 2);

        ds.write(1.0f);
        ds.write(1.0f);
        ds.reset();

        TEST("Downsampler reset: accumulator is zero", ds.accumulator == 0.0f);
        TEST("Downsampler reset: sample count is zero", ds.sampleCount == 0);
    }

    // Test anti-aliasing filter (output should be stable, no NaN/Inf)
    {
        Downsampler ds;
        ds.init(48000.0f, 2);

        bool valid = true;
        for (int i = 0; i < 1000; ++i) {
            // Write alternating high-frequency signal (would alias without filter)
            ds.write(i % 2 == 0 ? 1.0f : -1.0f);
            ds.write(i % 2 == 0 ? 1.0f : -1.0f);

            Sample out = ds.read();
            if (std::isnan(out) || std::isinf(out)) {
                valid = false;
                break;
            }
        }
        TEST("Downsampler filter: no NaN or Inf in output", valid);
    }

    // Test with sine wave - output should be within reasonable range
    {
        constexpr size_t OVERSAMPLE = 2;
        constexpr Sample OUTPUT_RATE = 48000.0f;
        constexpr Sample INPUT_RATE = OUTPUT_RATE * OVERSAMPLE;

        SinOsc osc;
        osc.init(INPUT_RATE);
        osc.setFrequency(440.0f);

        Downsampler ds;
        ds.init(OUTPUT_RATE, OVERSAMPLE);

        bool inRange = true;
        for (int i = 0; i < 1000; ++i) {
            for (size_t j = 0; j < OVERSAMPLE; ++j) {
                Sample sample = osc.tick();
                ds.write(sample);
            }
            Sample out = ds.read();
            if (std::abs(out) > 1.5f) {  // Allow some headroom for filter transients
                inRange = false;
                break;
            }
        }
        TEST("Downsampler with SinOsc: output in valid range", inRange);
    }

    // Test block processing
    {
        constexpr size_t OVERSAMPLE = 2;
        constexpr size_t OUTPUT_SAMPLES = 64;
        constexpr size_t INPUT_SAMPLES = OUTPUT_SAMPLES * OVERSAMPLE;

        Sample input[INPUT_SAMPLES];
        Sample output[OUTPUT_SAMPLES];

        // Fill input with simple pattern
        for (size_t i = 0; i < INPUT_SAMPLES; ++i) {
            input[i] = std::sin(static_cast<Sample>(i) * 0.1f);
        }

        Downsampler ds;
        ds.init(48000.0f, OVERSAMPLE);
        ds.process(input, output, OUTPUT_SAMPLES);

        bool hasOutput = false;
        for (size_t i = 0; i < OUTPUT_SAMPLES; ++i) {
            if (output[i] != 0.0f) {
                hasOutput = true;
                break;
            }
        }
        TEST("Downsampler process: produces output", hasOutput);
    }

    // Test StereoDownsampler initialization
    {
        StereoDownsampler sds;
        sds.init(48000.0f, 2);
        TEST("StereoDownsampler init: left sample rate set", sds.left.outputSampleRate == 48000.0f);
        TEST("StereoDownsampler init: right sample rate set", sds.right.outputSampleRate == 48000.0f);
        TEST("StereoDownsampler init: left factor set", sds.left.oversampleFactor == 2);
        TEST("StereoDownsampler init: right factor set", sds.right.oversampleFactor == 2);
    }

    // Test StereoDownsampler write/read with Stereo struct
    {
        StereoDownsampler sds;
        sds.init(48000.0f, 2);

        sds.write(Stereo(1.0f, 0.5f));
        sds.write(Stereo(1.0f, 0.5f));

        Stereo out = sds.read();
        TEST("StereoDownsampler: left output non-zero", out.left != 0.0f);
        TEST("StereoDownsampler: right output non-zero", out.right != 0.0f);
    }

    // Test StereoDownsampler write/read with separate samples
    {
        StereoDownsampler sds;
        sds.init(48000.0f, 2);

        sds.write(1.0f, 0.5f);
        sds.write(1.0f, 0.5f);

        Stereo out = sds.read();
        TEST("StereoDownsampler separate write: left output non-zero", out.left != 0.0f);
        TEST("StereoDownsampler separate write: right output non-zero", out.right != 0.0f);
    }

    // Test StereoDownsampler isReady
    {
        StereoDownsampler sds;
        sds.init(48000.0f, 2);

        TEST("StereoDownsampler isReady: false initially", !sds.isReady());

        sds.write(1.0f, 1.0f);
        TEST("StereoDownsampler isReady: false after 1 write", !sds.isReady());

        sds.write(1.0f, 1.0f);
        TEST("StereoDownsampler isReady: true after 2 writes", sds.isReady());
    }

    // Test StereoDownsampler reset
    {
        StereoDownsampler sds;
        sds.init(48000.0f, 2);

        sds.write(1.0f, 1.0f);
        sds.write(1.0f, 1.0f);
        sds.reset();

        TEST("StereoDownsampler reset: left accumulator zero", sds.left.accumulator == 0.0f);
        TEST("StereoDownsampler reset: right accumulator zero", sds.right.accumulator == 0.0f);
    }

    // Test setOversampleFactor
    {
        Downsampler ds;
        ds.init(48000.0f, 2);
        ds.setOversampleFactor(4);
        TEST("Downsampler setOversampleFactor: factor updated", ds.oversampleFactor == 4);
    }

    // Test StereoDownsampler setOversampleFactor
    {
        StereoDownsampler sds;
        sds.init(48000.0f, 2);
        sds.setOversampleFactor(4);
        TEST("StereoDownsampler setOversampleFactor: left updated", sds.left.oversampleFactor == 4);
        TEST("StereoDownsampler setOversampleFactor: right updated", sds.right.oversampleFactor == 4);
    }

    // Test downsampling preserves DC signal
    {
        Downsampler ds;
        ds.init(48000.0f, 2);

        // Warm up filter with DC signal
        for (int i = 0; i < 1000; ++i) {
            ds.write(0.5f);
            ds.write(0.5f);
            ds.read();
        }

        // Now test that DC is preserved
        ds.write(0.5f);
        ds.write(0.5f);
        Sample out = ds.read();

        // After filter settling, output should be close to input DC
        TEST("Downsampler DC: output close to input", std::abs(out - 0.5f) < 0.01f);
    }

    // Test 1x oversampling (pass-through)
    {
        Downsampler ds;
        ds.init(48000.0f, 1);

        ds.write(0.75f);
        Sample out = ds.read();

        // With 1x, after filter warmup, should pass through
        for (int i = 0; i < 100; ++i) {
            ds.write(0.75f);
            out = ds.read();
        }
        TEST("Downsampler 1x: near pass-through after settling", std::abs(out - 0.75f) < 0.05f);
    }

    return failures;
}
