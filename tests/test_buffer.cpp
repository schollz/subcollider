/**
 * @file test_buffer.cpp
 * @brief Unit tests for Buffer
 */

#include <iostream>
#include <cmath>
#include <subcollider/Buffer.h>

using namespace subcollider;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_buffer() {
    int failures = 0;

    // Test default constructor
    {
        Buffer buf;
        TEST("Buffer default: data is nullptr", buf.data == nullptr);
        TEST("Buffer default: channels is 1", buf.channels == 1);
        TEST("Buffer default: sampleRate is DEFAULT_SAMPLE_RATE", buf.sampleRate == DEFAULT_SAMPLE_RATE);
        TEST("Buffer default: numSamples is 0", buf.numSamples == 0);
        TEST("Buffer default: isValid is false", buf.isValid() == false);
        TEST("Buffer default: isMono is true", buf.isMono() == true);
        TEST("Buffer default: isStereo is false", buf.isStereo() == false);
        TEST("Buffer default: totalFloats is 0", buf.totalFloats() == 0);
        TEST("Buffer default: duration is 0", buf.duration() == 0.0f);
    }

    // Test parameterized constructor with mono buffer
    {
        Sample monoData[100];
        for (size_t i = 0; i < 100; ++i) {
            monoData[i] = static_cast<Sample>(i) / 100.0f;
        }

        Buffer buf(monoData, 1, 44100.0f, 100);
        TEST("Buffer mono: data points to array", buf.data == monoData);
        TEST("Buffer mono: channels is 1", buf.channels == 1);
        TEST("Buffer mono: sampleRate is 44100", buf.sampleRate == 44100.0f);
        TEST("Buffer mono: numSamples is 100", buf.numSamples == 100);
        TEST("Buffer mono: isValid is true", buf.isValid() == true);
        TEST("Buffer mono: isMono is true", buf.isMono() == true);
        TEST("Buffer mono: isStereo is false", buf.isStereo() == false);
        TEST("Buffer mono: totalFloats is 100", buf.totalFloats() == 100);
    }

    // Test parameterized constructor with stereo buffer
    {
        Sample stereoData[200];  // 100 samples * 2 channels
        for (size_t i = 0; i < 200; ++i) {
            stereoData[i] = static_cast<Sample>(i) / 200.0f;
        }

        Buffer buf(stereoData, 2, 48000.0f, 100);
        TEST("Buffer stereo: data points to array", buf.data == stereoData);
        TEST("Buffer stereo: channels is 2", buf.channels == 2);
        TEST("Buffer stereo: sampleRate is 48000", buf.sampleRate == 48000.0f);
        TEST("Buffer stereo: numSamples is 100", buf.numSamples == 100);
        TEST("Buffer stereo: isValid is true", buf.isValid() == true);
        TEST("Buffer stereo: isMono is false", buf.isMono() == false);
        TEST("Buffer stereo: isStereo is true", buf.isStereo() == true);
        TEST("Buffer stereo: totalFloats is 200", buf.totalFloats() == 200);
    }

    // Test getSample with mono buffer
    {
        Sample monoData[4] = {0.1f, 0.2f, 0.3f, 0.4f};
        Buffer buf(monoData, 1, 48000.0f, 4);

        TEST("Buffer mono getSample(0)", std::abs(buf.getSample(0) - 0.1f) < 0.0001f);
        TEST("Buffer mono getSample(1)", std::abs(buf.getSample(1) - 0.2f) < 0.0001f);
        TEST("Buffer mono getSample(2)", std::abs(buf.getSample(2) - 0.3f) < 0.0001f);
        TEST("Buffer mono getSample(3)", std::abs(buf.getSample(3) - 0.4f) < 0.0001f);
        TEST("Buffer mono getSample out of bounds", buf.getSample(10) == 0.0f);
    }

    // Test getSample with stereo buffer (returns left channel)
    {
        Sample stereoData[8] = {0.1f, 0.5f, 0.2f, 0.6f, 0.3f, 0.7f, 0.4f, 0.8f};
        Buffer buf(stereoData, 2, 48000.0f, 4);

        TEST("Buffer stereo getSample(0) returns left", std::abs(buf.getSample(0) - 0.1f) < 0.0001f);
        TEST("Buffer stereo getSample(1) returns left", std::abs(buf.getSample(1) - 0.2f) < 0.0001f);
        TEST("Buffer stereo getSample(2) returns left", std::abs(buf.getSample(2) - 0.3f) < 0.0001f);
        TEST("Buffer stereo getSample(3) returns left", std::abs(buf.getSample(3) - 0.4f) < 0.0001f);
    }

    // Test getStereoSample with mono buffer
    {
        Sample monoData[4] = {0.1f, 0.2f, 0.3f, 0.4f};
        Buffer buf(monoData, 1, 48000.0f, 4);

        Stereo s0 = buf.getStereoSample(0);
        Stereo s1 = buf.getStereoSample(1);

        TEST("Buffer mono getStereoSample(0).left", std::abs(s0.left - 0.1f) < 0.0001f);
        TEST("Buffer mono getStereoSample(0).right", std::abs(s0.right - 0.1f) < 0.0001f);
        TEST("Buffer mono getStereoSample(1).left", std::abs(s1.left - 0.2f) < 0.0001f);
        TEST("Buffer mono getStereoSample(1).right", std::abs(s1.right - 0.2f) < 0.0001f);
    }

    // Test getStereoSample with stereo buffer
    {
        Sample stereoData[8] = {0.1f, 0.5f, 0.2f, 0.6f, 0.3f, 0.7f, 0.4f, 0.8f};
        Buffer buf(stereoData, 2, 48000.0f, 4);

        Stereo s0 = buf.getStereoSample(0);
        Stereo s1 = buf.getStereoSample(1);
        Stereo s2 = buf.getStereoSample(2);
        Stereo s3 = buf.getStereoSample(3);

        TEST("Buffer stereo getStereoSample(0).left", std::abs(s0.left - 0.1f) < 0.0001f);
        TEST("Buffer stereo getStereoSample(0).right", std::abs(s0.right - 0.5f) < 0.0001f);
        TEST("Buffer stereo getStereoSample(1).left", std::abs(s1.left - 0.2f) < 0.0001f);
        TEST("Buffer stereo getStereoSample(1).right", std::abs(s1.right - 0.6f) < 0.0001f);
        TEST("Buffer stereo getStereoSample(2).left", std::abs(s2.left - 0.3f) < 0.0001f);
        TEST("Buffer stereo getStereoSample(2).right", std::abs(s2.right - 0.7f) < 0.0001f);
        TEST("Buffer stereo getStereoSample(3).left", std::abs(s3.left - 0.4f) < 0.0001f);
        TEST("Buffer stereo getStereoSample(3).right", std::abs(s3.right - 0.8f) < 0.0001f);
    }

    // Test getStereoSample out of bounds
    {
        Sample monoData[4] = {0.1f, 0.2f, 0.3f, 0.4f};
        Buffer buf(monoData, 1, 48000.0f, 4);

        Stereo s = buf.getStereoSample(10);
        TEST("Buffer getStereoSample out of bounds left", s.left == 0.0f);
        TEST("Buffer getStereoSample out of bounds right", s.right == 0.0f);
    }

    // Test duration calculation
    {
        Sample data[48000];
        Buffer buf(data, 1, 48000.0f, 48000);
        TEST("Buffer duration 1 second", std::abs(buf.duration() - 1.0f) < 0.0001f);

        Buffer buf2(data, 1, 44100.0f, 44100);
        TEST("Buffer duration 1 second at 44100", std::abs(buf2.duration() - 1.0f) < 0.0001f);

        Buffer buf3(data, 1, 48000.0f, 24000);
        TEST("Buffer duration 0.5 seconds", std::abs(buf3.duration() - 0.5f) < 0.0001f);

        Buffer buf4(data, 2, 48000.0f, 48000);
        TEST("Buffer stereo duration 1 second", std::abs(buf4.duration() - 1.0f) < 0.0001f);
    }

    // Test isValid with invalid channel count
    {
        Sample data[100];
        Buffer buf(data, 0, 48000.0f, 100);
        TEST("Buffer invalid channels 0", buf.isValid() == false);

        Buffer buf2(data, 3, 48000.0f, 100);
        TEST("Buffer invalid channels 3", buf2.isValid() == false);
    }

    // Test with nullptr data
    {
        Buffer buf(nullptr, 1, 48000.0f, 100);
        TEST("Buffer nullptr: isValid is false", buf.isValid() == false);
        TEST("Buffer nullptr: getSample returns 0", buf.getSample(0) == 0.0f);
        Stereo s = buf.getStereoSample(0);
        TEST("Buffer nullptr: getStereoSample.left returns 0", s.left == 0.0f);
        TEST("Buffer nullptr: getStereoSample.right returns 0", s.right == 0.0f);
    }

    // Test duration with zero sample rate
    {
        Sample data[100];
        Buffer buf(data, 1, 0.0f, 100);
        TEST("Buffer zero sampleRate: duration is 0", buf.duration() == 0.0f);
    }

    return failures;
}
