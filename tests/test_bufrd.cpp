/**
 * @file test_bufrd.cpp
 * @brief Unit tests for BufRd UGen
 */

#include <iostream>
#include <cmath>
#include <subcollider/Buffer.h>
#include <subcollider/ugens/BufRd.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_bufrd() {
    int failures = 0;

    // Test initialization
    {
        BufRd reader;
        reader.init();
        TEST("BufRd init: buffer is nullptr", reader.buffer == nullptr);
        TEST("BufRd init: loop is true", reader.loop == true);
        TEST("BufRd init: interpolation is 2 (linear)", reader.interpolation == 2);
    }

    // Test initialization with buffer
    {
        Sample data[4] = {0.1f, 0.2f, 0.3f, 0.4f};
        Buffer buf(data, 1, 48000.0f, 4);
        BufRd reader;
        reader.init(&buf);
        TEST("BufRd init with buffer: buffer is set", reader.buffer == &buf);
    }

    // Test setters
    {
        BufRd reader;
        reader.init();

        reader.setLoop(false);
        TEST("BufRd setLoop: loop is false", reader.loop == false);

        reader.setLoop(true);
        TEST("BufRd setLoop: loop is true", reader.loop == true);

        reader.setInterpolation(1);
        TEST("BufRd setInterpolation: is 1", reader.interpolation == 1);

        reader.setInterpolation(4);
        TEST("BufRd setInterpolation: is 4", reader.interpolation == 4);

        Sample data[4] = {0.1f, 0.2f, 0.3f, 0.4f};
        Buffer buf(data, 1, 48000.0f, 4);
        reader.setBuffer(&buf);
        TEST("BufRd setBuffer: buffer is set", reader.buffer == &buf);
    }

    // Test tick with nullptr buffer
    {
        BufRd reader;
        reader.init();
        Sample result = reader.tick(0.0f);
        TEST("BufRd nullptr: tick returns 0", result == 0.0f);

        Stereo stereoResult = reader.tickStereo(0.0f);
        TEST("BufRd nullptr: tickStereo left is 0", stereoResult.left == 0.0f);
        TEST("BufRd nullptr: tickStereo right is 0", stereoResult.right == 0.0f);
    }

    // Test no interpolation with mono buffer
    {
        Sample data[4] = {0.0f, 1.0f, 2.0f, 3.0f};
        Buffer buf(data, 1, 48000.0f, 4);
        BufRd reader;
        reader.init(&buf);
        reader.setInterpolation(1);  // No interpolation

        TEST("BufRd no interp: phase 0.0", std::abs(reader.tick(0.0f) - 0.0f) < 0.001f);
        TEST("BufRd no interp: phase 1.0", std::abs(reader.tick(1.0f) - 1.0f) < 0.001f);
        TEST("BufRd no interp: phase 2.0", std::abs(reader.tick(2.0f) - 2.0f) < 0.001f);
        TEST("BufRd no interp: phase 3.0", std::abs(reader.tick(3.0f) - 3.0f) < 0.001f);

        // Fractional phase should truncate (sample & hold)
        TEST("BufRd no interp: phase 0.5", std::abs(reader.tick(0.5f) - 0.0f) < 0.001f);
        TEST("BufRd no interp: phase 1.9", std::abs(reader.tick(1.9f) - 1.0f) < 0.001f);
    }

    // Test linear interpolation with mono buffer
    {
        Sample data[4] = {0.0f, 1.0f, 2.0f, 3.0f};
        Buffer buf(data, 1, 48000.0f, 4);
        BufRd reader;
        reader.init(&buf);
        reader.setInterpolation(2);  // Linear interpolation

        TEST("BufRd linear: phase 0.0", std::abs(reader.tick(0.0f) - 0.0f) < 0.001f);
        TEST("BufRd linear: phase 1.0", std::abs(reader.tick(1.0f) - 1.0f) < 0.001f);
        TEST("BufRd linear: phase 0.5", std::abs(reader.tick(0.5f) - 0.5f) < 0.001f);
        TEST("BufRd linear: phase 1.5", std::abs(reader.tick(1.5f) - 1.5f) < 0.001f);
        TEST("BufRd linear: phase 2.25", std::abs(reader.tick(2.25f) - 2.25f) < 0.001f);
    }

    // Test cubic interpolation with mono buffer
    {
        Sample data[8] = {0.0f, 1.0f, 4.0f, 9.0f, 16.0f, 25.0f, 36.0f, 49.0f};
        Buffer buf(data, 1, 48000.0f, 8);
        BufRd reader;
        reader.init(&buf);
        reader.setInterpolation(4);  // Cubic interpolation

        // At integer positions, should match exact values
        TEST("BufRd cubic: phase 0.0", std::abs(reader.tick(0.0f) - 0.0f) < 0.001f);
        TEST("BufRd cubic: phase 1.0", std::abs(reader.tick(1.0f) - 1.0f) < 0.001f);
        TEST("BufRd cubic: phase 2.0", std::abs(reader.tick(2.0f) - 4.0f) < 0.001f);

        // Fractional positions should be smoothly interpolated
        Sample val = reader.tick(1.5f);
        TEST("BufRd cubic: phase 1.5 is between 1 and 4", val > 1.0f && val < 4.0f);
    }

    // Test looping mode
    {
        Sample data[4] = {0.0f, 1.0f, 2.0f, 3.0f};
        Buffer buf(data, 1, 48000.0f, 4);
        BufRd reader;
        reader.init(&buf);
        reader.setLoop(true);
        reader.setInterpolation(1);  // No interpolation for easier testing

        // Phase beyond buffer should wrap
        TEST("BufRd loop: phase 4.0 wraps to 0", std::abs(reader.tick(4.0f) - 0.0f) < 0.001f);
        TEST("BufRd loop: phase 5.0 wraps to 1", std::abs(reader.tick(5.0f) - 1.0f) < 0.001f);
        TEST("BufRd loop: phase 8.0 wraps to 0", std::abs(reader.tick(8.0f) - 0.0f) < 0.001f);

        // Negative phase should wrap
        TEST("BufRd loop: phase -1.0 wraps to 3", std::abs(reader.tick(-1.0f) - 3.0f) < 0.001f);
        TEST("BufRd loop: phase -4.0 wraps to 0", std::abs(reader.tick(-4.0f) - 0.0f) < 0.001f);
    }

    // Test non-looping mode (clamping)
    {
        Sample data[4] = {0.0f, 1.0f, 2.0f, 3.0f};
        Buffer buf(data, 1, 48000.0f, 4);
        BufRd reader;
        reader.init(&buf);
        reader.setLoop(false);
        reader.setInterpolation(1);  // No interpolation

        // Phase beyond buffer should clamp
        TEST("BufRd no loop: phase 4.0 clamps to 3", std::abs(reader.tick(4.0f) - 3.0f) < 0.001f);
        TEST("BufRd no loop: phase 10.0 clamps to 3", std::abs(reader.tick(10.0f) - 3.0f) < 0.001f);

        // Negative phase should clamp to 0
        TEST("BufRd no loop: phase -1.0 clamps to 0", std::abs(reader.tick(-1.0f) - 0.0f) < 0.001f);
    }

    // Test stereo buffer with no interpolation
    {
        // Interleaved stereo: L0, R0, L1, R1, ...
        Sample data[8] = {0.1f, 0.5f, 0.2f, 0.6f, 0.3f, 0.7f, 0.4f, 0.8f};
        Buffer buf(data, 2, 48000.0f, 4);
        BufRd reader;
        reader.init(&buf);
        reader.setInterpolation(1);  // No interpolation

        Stereo s0 = reader.tickStereo(0.0f);
        TEST("BufRd stereo no interp: phase 0 left", std::abs(s0.left - 0.1f) < 0.001f);
        TEST("BufRd stereo no interp: phase 0 right", std::abs(s0.right - 0.5f) < 0.001f);

        Stereo s1 = reader.tickStereo(1.0f);
        TEST("BufRd stereo no interp: phase 1 left", std::abs(s1.left - 0.2f) < 0.001f);
        TEST("BufRd stereo no interp: phase 1 right", std::abs(s1.right - 0.6f) < 0.001f);

        Stereo s3 = reader.tickStereo(3.0f);
        TEST("BufRd stereo no interp: phase 3 left", std::abs(s3.left - 0.4f) < 0.001f);
        TEST("BufRd stereo no interp: phase 3 right", std::abs(s3.right - 0.8f) < 0.001f);
    }

    // Test stereo buffer with linear interpolation
    {
        Sample data[8] = {0.0f, 1.0f, 1.0f, 2.0f, 2.0f, 3.0f, 3.0f, 4.0f};
        Buffer buf(data, 2, 48000.0f, 4);
        BufRd reader;
        reader.init(&buf);
        reader.setInterpolation(2);  // Linear interpolation

        Stereo s = reader.tickStereo(0.5f);
        // Left: lerp(0.0, 1.0, 0.5) = 0.5
        // Right: lerp(1.0, 2.0, 0.5) = 1.5
        TEST("BufRd stereo linear: phase 0.5 left", std::abs(s.left - 0.5f) < 0.001f);
        TEST("BufRd stereo linear: phase 0.5 right", std::abs(s.right - 1.5f) < 0.001f);
    }

    // Test stereo buffer with cubic interpolation
    {
        Sample data[16] = {
            0.0f, 1.0f,   // sample 0
            1.0f, 2.0f,   // sample 1
            4.0f, 5.0f,   // sample 2
            9.0f, 10.0f,  // sample 3
            16.0f, 17.0f, // sample 4
            25.0f, 26.0f, // sample 5
            36.0f, 37.0f, // sample 6
            49.0f, 50.0f  // sample 7
        };
        Buffer buf(data, 2, 48000.0f, 8);
        BufRd reader;
        reader.init(&buf);
        reader.setInterpolation(4);  // Cubic interpolation

        // At integer positions, should match exact values
        Stereo s2 = reader.tickStereo(2.0f);
        TEST("BufRd stereo cubic: phase 2.0 left", std::abs(s2.left - 4.0f) < 0.001f);
        TEST("BufRd stereo cubic: phase 2.0 right", std::abs(s2.right - 5.0f) < 0.001f);

        // Fractional should be interpolated
        Stereo s = reader.tickStereo(2.5f);
        TEST("BufRd stereo cubic: phase 2.5 left in range", s.left > 4.0f && s.left < 9.0f);
        TEST("BufRd stereo cubic: phase 2.5 right in range", s.right > 5.0f && s.right < 10.0f);
    }

    // Test mono tick returns left channel for stereo buffer
    {
        Sample data[8] = {0.1f, 0.9f, 0.2f, 0.8f, 0.3f, 0.7f, 0.4f, 0.6f};
        Buffer buf(data, 2, 48000.0f, 4);
        BufRd reader;
        reader.init(&buf);
        reader.setInterpolation(1);

        // tick() should return left channel
        TEST("BufRd tick on stereo: returns left", std::abs(reader.tick(0.0f) - 0.1f) < 0.001f);
        TEST("BufRd tick on stereo: returns left", std::abs(reader.tick(1.0f) - 0.2f) < 0.001f);
    }

    // Test tickStereo on mono buffer (should duplicate to both channels)
    {
        Sample data[4] = {0.1f, 0.2f, 0.3f, 0.4f};
        Buffer buf(data, 1, 48000.0f, 4);
        BufRd reader;
        reader.init(&buf);
        reader.setInterpolation(1);

        Stereo s = reader.tickStereo(1.0f);
        TEST("BufRd tickStereo on mono: left equals mono", std::abs(s.left - 0.2f) < 0.001f);
        TEST("BufRd tickStereo on mono: right equals mono", std::abs(s.right - 0.2f) < 0.001f);
    }

    // Test process (block processing)
    {
        Sample data[4] = {0.0f, 1.0f, 2.0f, 3.0f};
        Buffer buf(data, 1, 48000.0f, 4);
        BufRd reader;
        reader.init(&buf);
        reader.setInterpolation(1);

        Sample phase[4] = {0.0f, 1.0f, 2.0f, 3.0f};
        Sample output[4];
        reader.process(output, phase, 4);

        TEST("BufRd process: output[0]", std::abs(output[0] - 0.0f) < 0.001f);
        TEST("BufRd process: output[1]", std::abs(output[1] - 1.0f) < 0.001f);
        TEST("BufRd process: output[2]", std::abs(output[2] - 2.0f) < 0.001f);
        TEST("BufRd process: output[3]", std::abs(output[3] - 3.0f) < 0.001f);
    }

    // Test processStereo (block processing)
    {
        Sample data[8] = {0.1f, 0.5f, 0.2f, 0.6f, 0.3f, 0.7f, 0.4f, 0.8f};
        Buffer buf(data, 2, 48000.0f, 4);
        BufRd reader;
        reader.init(&buf);
        reader.setInterpolation(1);

        Sample phase[2] = {0.0f, 2.0f};
        Sample left[2], right[2];
        reader.processStereo(left, right, phase, 2);

        TEST("BufRd processStereo: left[0]", std::abs(left[0] - 0.1f) < 0.001f);
        TEST("BufRd processStereo: right[0]", std::abs(right[0] - 0.5f) < 0.001f);
        TEST("BufRd processStereo: left[1]", std::abs(left[1] - 0.3f) < 0.001f);
        TEST("BufRd processStereo: right[1]", std::abs(right[1] - 0.7f) < 0.001f);
    }

    // Test invalid interpolation mode defaults to no interpolation
    {
        Sample data[4] = {0.0f, 1.0f, 2.0f, 3.0f};
        Buffer buf(data, 1, 48000.0f, 4);
        BufRd reader;
        reader.init(&buf);
        reader.setInterpolation(3);  // Invalid mode

        // Should behave like no interpolation
        TEST("BufRd invalid interp: phase 0.5 truncates", std::abs(reader.tick(0.5f) - 0.0f) < 0.001f);
    }

    // Test linear interpolation wrapping at buffer boundary
    {
        Sample data[4] = {0.0f, 1.0f, 2.0f, 3.0f};
        Buffer buf(data, 1, 48000.0f, 4);
        BufRd reader;
        reader.init(&buf);
        reader.setLoop(true);
        reader.setInterpolation(2);

        // At phase 3.5, should interpolate between samples 3 (3.0) and 0 (0.0)
        Sample val = reader.tick(3.5f);
        // lerp(3.0, 0.0, 0.5) = 1.5
        TEST("BufRd linear wrap: phase 3.5 interpolates", std::abs(val - 1.5f) < 0.001f);
    }

    return failures;
}
