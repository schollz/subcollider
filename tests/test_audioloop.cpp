/**
 * @file test_audioloop.cpp
 * @brief Unit tests for AudioLoop and AudioBuffer
 */

#include <iostream>
#include <cmath>
#include <subcollider/AudioBuffer.h>
#include <subcollider/AudioLoop.h>

using namespace subcollider;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_audioloop() {
    int failures = 0;

    // Test AudioBuffer initialization
    {
        AudioBuffer<64> buffer;
        TEST("AudioBuffer: size is set to capacity", buffer.size == 64);
        TEST("AudioBuffer: capacity is correct", buffer.capacity() == 64);

        bool allZero = true;
        for (size_t i = 0; i < buffer.size; ++i) {
            if (buffer[i] != 0.0f) {
                allZero = false;
                break;
            }
        }
        TEST("AudioBuffer: initialized to zero", allZero);
    }

    // Test AudioBuffer clear
    {
        AudioBuffer<64> buffer;
        for (size_t i = 0; i < buffer.size; ++i) {
            buffer[i] = static_cast<Sample>(i);
        }
        buffer.clear();

        bool allZero = true;
        for (size_t i = 0; i < buffer.size; ++i) {
            if (buffer[i] != 0.0f) {
                allZero = false;
                break;
            }
        }
        TEST("AudioBuffer clear: all zeros", allZero);
    }

    // Test AudioBuffer iterators
    {
        AudioBuffer<64> buffer;
        buffer[0] = 1.0f;
        buffer[63] = 2.0f;

        TEST("AudioBuffer begin: points to first element", *buffer.begin() == 1.0f);
        TEST("AudioBuffer end: points past last element", *(buffer.end() - 1) == 2.0f);
    }

    // Test AudioLoop initialization
    {
        AudioLoop<64> loop;
        loop.init(48000.0f);
        TEST("AudioLoop init: sample rate set", loop.sampleRate == 48000.0f);
        TEST("AudioLoop init: current buffer is 0", loop.currentBuffer == 0);
        TEST("AudioLoop getBlockSize: returns correct size", loop.getBlockSize() == 64);
    }

    // Test AudioLoop buffer access
    {
        AudioLoop<64> loop;
        loop.init(48000.0f);

        Sample* procBuf = loop.getProcessingBuffer();
        const Sample* outBuf = loop.getOutputBuffer();

        TEST("AudioLoop: processing and output buffers are different", procBuf != outBuf);
    }

    // Test AudioLoop buffer swap
    {
        AudioLoop<64> loop;
        loop.init(48000.0f);

        Sample* procBuf1 = loop.getProcessingBuffer();
        loop.swapBuffers();
        Sample* procBuf2 = loop.getProcessingBuffer();

        TEST("AudioLoop swapBuffers: processing buffer changes", procBuf1 != procBuf2);
    }

    // Test AudioCallbackHandler
    {
        AudioCallbackHandler handler;
        handler.init(48000.0f);

        TEST("AudioCallbackHandler init: callback is null", handler.callback == nullptr);
        TEST("AudioCallbackHandler init: userData is null", handler.userData == nullptr);
        TEST("AudioCallbackHandler init: sample rate set", handler.sampleRate == 48000.0f);
    }

    // Test AudioCallbackHandler with callback
    {
        static bool callbackCalled = false;
        static size_t callbackSamples = 0;

        auto testCallback = [](Sample* buffer, size_t numSamples, void*) {
            callbackCalled = true;
            callbackSamples = numSamples;
            for (size_t i = 0; i < numSamples; ++i) {
                buffer[i] = 1.0f;
            }
        };

        AudioCallbackHandler handler;
        handler.init(48000.0f);
        handler.setCallback(testCallback);

        Sample buffer[64] = {};
        handler.process(buffer, 64);

        TEST("AudioCallbackHandler: callback was called", callbackCalled);
        TEST("AudioCallbackHandler: correct sample count passed", callbackSamples == 64);
        TEST("AudioCallbackHandler: buffer was modified", buffer[0] == 1.0f);
    }

    return failures;
}
