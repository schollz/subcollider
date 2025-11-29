/**
 * @file basic_example.cpp
 * @brief Basic example demonstrating SubCollider DSP engine usage.
 *
 * This example shows how to:
 * - Initialize UGens
 * - Process audio in blocks
 * - Combine multiple UGens
 */

#include <subcollider.h>
#include <iostream>
#include <cmath>

using namespace subcollider;
using namespace subcollider::ugens;

int main() {
    constexpr size_t SAMPLE_RATE = 48000;
    constexpr size_t BLOCK_SIZE = 64;
    constexpr size_t NUM_BLOCKS = 100;

    std::cout << "SubCollider Basic Example" << std::endl;
    std::cout << "=========================" << std::endl;
    std::cout << "Sample Rate: " << SAMPLE_RATE << " Hz" << std::endl;
    std::cout << "Block Size: " << BLOCK_SIZE << " samples" << std::endl;
    std::cout << std::endl;

    // Example 1: Basic SinOsc
    std::cout << "Example 1: SinOsc" << std::endl;
    {
        SinOsc osc;
        osc.init(SAMPLE_RATE);
        osc.setFrequency(440.0f);

        AudioBuffer<BLOCK_SIZE> buffer;
        osc.process(buffer.data, BLOCK_SIZE);

        // Print first few samples
        std::cout << "  First 8 samples: ";
        for (int i = 0; i < 8; ++i) {
            std::cout << buffer[i] << " ";
        }
        std::cout << std::endl;
    }

    // Example 2: EnvelopeAR
    std::cout << std::endl << "Example 2: EnvelopeAR" << std::endl;
    {
        EnvelopeAR env;
        env.init(SAMPLE_RATE);
        env.setAttack(0.01f);   // 10ms attack
        env.setRelease(0.1f);   // 100ms release

        env.trigger();

        AudioBuffer<BLOCK_SIZE> buffer;
        env.process(buffer.data, BLOCK_SIZE);

        std::cout << "  Envelope values: ";
        for (int i = 0; i < 8; ++i) {
            std::cout << buffer[i] << " ";
        }
        std::cout << std::endl;
    }

    // Example 3: LFNoise2
    std::cout << std::endl << "Example 3: LFNoise2" << std::endl;
    {
        LFNoise2 noise;
        noise.init(SAMPLE_RATE, 42);
        noise.setFrequency(4.0f);  // 4 Hz modulation

        AudioBuffer<BLOCK_SIZE> buffer;
        noise.process(buffer.data, BLOCK_SIZE);

        std::cout << "  Noise values: ";
        for (int i = 0; i < 8; ++i) {
            std::cout << buffer[i] << " ";
        }
        std::cout << std::endl;
    }

    // Example 4: ExampleVoice (combining all UGens - stereo output)
    std::cout << std::endl << "Example 4: ExampleVoice (Stereo)" << std::endl;
    {
        ExampleVoice voice;
        voice.init(SAMPLE_RATE);
        voice.setFrequency(440.0f);
        voice.setAttack(0.01f);
        voice.setRelease(0.5f);
        voice.setVibratoDepth(0.1f);
        voice.setVibratoRate(5.0f);
        voice.setAmplitude(0.8f);

        voice.trigger();

        AudioBuffer<BLOCK_SIZE> bufferL;
        AudioBuffer<BLOCK_SIZE> bufferR;

        // Generate a few blocks
        Sample maxValL = 0.0f;
        Sample maxValR = 0.0f;
        for (size_t block = 0; block < NUM_BLOCKS; ++block) {
            voice.process(bufferL.data, bufferR.data, BLOCK_SIZE);

            for (size_t i = 0; i < BLOCK_SIZE; ++i) {
                if (std::abs(bufferL[i]) > maxValL) {
                    maxValL = std::abs(bufferL[i]);
                }
                if (std::abs(bufferR[i]) > maxValR) {
                    maxValR = std::abs(bufferR[i]);
                }
            }

            // Release halfway through
            if (block == NUM_BLOCKS / 2) {
                voice.release();
            }
        }

        std::cout << "  Peak amplitude L: " << maxValL << ", R: " << maxValR << std::endl;
        std::cout << "  Voice active: " << (voice.isActive() ? "yes" : "no") << std::endl;
    }

    // Example 5: AudioLoop usage (stereo)
    std::cout << std::endl << "Example 5: AudioLoop (Stereo)" << std::endl;
    {
        AudioLoop<BLOCK_SIZE> loopL;
        AudioLoop<BLOCK_SIZE> loopR;
        loopL.init(SAMPLE_RATE);
        loopR.init(SAMPLE_RATE);

        ExampleVoice voice;
        voice.init(SAMPLE_RATE);
        voice.setFrequency(440.0f);
        voice.trigger();

        // Simulate stereo audio processing loop
        for (int i = 0; i < 10; ++i) {
            // Get processing buffers for left and right
            Sample* procBufL = loopL.getProcessingBuffer();
            Sample* procBufR = loopR.getProcessingBuffer();

            // Clear them
            loopL.clearProcessingBuffer();
            loopR.clearProcessingBuffer();

            // Process voice into stereo buffers
            voice.process(procBufL, procBufR, BLOCK_SIZE);

            // Swap buffers (makes processed data available for output)
            loopL.swapBuffers();
            loopR.swapBuffers();
        }

        std::cout << "  Processed 10 stereo blocks successfully" << std::endl;
    }

    std::cout << std::endl << "All examples completed successfully!" << std::endl;

    return 0;
}
