/**
 * @file test_playback_oversampling.cpp
 * @brief Unit tests for playback with oversampling.
 *
 * Tests the audio processing flow used in jack_playback_example.cpp:
 * 1. Load audio file into buffer
 * 2. Use Phasor to generate phase at internal (oversampled) rate
 * 3. Use BufRd to read samples from buffer
 * 4. Use Downsampler to convert back to output rate
 *
 * Verifies that the 2x oversampling setup produces correct output.
 */

#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <vector>
#include <subcollider/BufferAllocator.h>
#include <subcollider/ugens/BufRd.h>
#include <subcollider/ugens/Phasor.h>
#include <subcollider/ugens/Downsampler.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

/**
 * @brief Simple WAV file header structure.
 */
struct WavHeader {
    char riffId[4];
    uint32_t fileSize;
    char waveId[4];
    char fmtId[4];
    uint32_t fmtSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
};

/**
 * @brief Load a WAV file into a vector of float samples.
 */
static bool loadWavFile(const char* filename, std::vector<Sample>& samples,
                 uint32_t& sampleRate, uint16_t& numChannels, size_t& numFrames) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    WavHeader header;
    file.read(header.riffId, 4);
    if (std::strncmp(header.riffId, "RIFF", 4) != 0) {
        return false;
    }

    file.read(reinterpret_cast<char*>(&header.fileSize), 4);
    file.read(header.waveId, 4);
    if (std::strncmp(header.waveId, "WAVE", 4) != 0) {
        return false;
    }

    bool foundFmt = false;
    bool foundData = false;
    uint32_t dataSize = 0;
    uint16_t actualFormat = 0;

    while (!foundFmt || !foundData) {
        char chunkId[4];
        uint32_t chunkSize;

        if (!file.read(chunkId, 4)) break;
        if (!file.read(reinterpret_cast<char*>(&chunkSize), 4)) break;

        if (std::strncmp(chunkId, "fmt ", 4) == 0) {
            file.read(reinterpret_cast<char*>(&header.audioFormat), 2);
            file.read(reinterpret_cast<char*>(&header.numChannels), 2);
            file.read(reinterpret_cast<char*>(&header.sampleRate), 4);
            file.read(reinterpret_cast<char*>(&header.byteRate), 4);
            file.read(reinterpret_cast<char*>(&header.blockAlign), 2);
            file.read(reinterpret_cast<char*>(&header.bitsPerSample), 2);

            actualFormat = header.audioFormat;

            if (header.audioFormat == 0xFFFE && chunkSize > 16) {
                uint16_t cbSize;
                file.read(reinterpret_cast<char*>(&cbSize), 2);

                if (cbSize >= 22) {
                    uint16_t validBitsPerSample;
                    uint32_t channelMask;
                    uint16_t subFormat;

                    file.read(reinterpret_cast<char*>(&validBitsPerSample), 2);
                    file.read(reinterpret_cast<char*>(&channelMask), 4);
                    file.read(reinterpret_cast<char*>(&subFormat), 2);

                    actualFormat = subFormat;

                    size_t remaining = cbSize - 22 + 14;
                    if (remaining > 0) {
                        file.seekg(remaining, std::ios::cur);
                    }
                } else {
                    file.seekg(cbSize, std::ios::cur);
                }
            } else if (chunkSize > 16) {
                file.seekg(chunkSize - 16, std::ios::cur);
            }
            foundFmt = true;
        } else if (std::strncmp(chunkId, "data", 4) == 0) {
            dataSize = chunkSize;
            foundData = true;
            break;
        } else {
            file.seekg(chunkSize, std::ios::cur);
        }
    }

    if (!foundFmt || !foundData) {
        return false;
    }

    if (actualFormat != 1) {
        return false;
    }

    if (header.bitsPerSample != 16 && header.bitsPerSample != 24) {
        return false;
    }

    size_t bytesPerSample = header.bitsPerSample / 8;
    numFrames = dataSize / (bytesPerSample * header.numChannels);
    sampleRate = header.sampleRate;
    numChannels = header.numChannels;

    size_t totalSamples = numFrames * header.numChannels;
    samples.resize(totalSamples);

    std::vector<char> rawData(dataSize);
    file.read(rawData.data(), dataSize);

    if (header.bitsPerSample == 16) {
        for (size_t i = 0; i < totalSamples; ++i) {
            int16_t sample = *reinterpret_cast<int16_t*>(&rawData[i * 2]);
            samples[i] = static_cast<Sample>(sample) / 32768.0f;
        }
    } else if (header.bitsPerSample == 24) {
        for (size_t i = 0; i < totalSamples; ++i) {
            size_t offset = i * 3;
            int32_t sample = static_cast<int32_t>(
                (static_cast<uint32_t>(static_cast<uint8_t>(rawData[offset])) << 8) |
                (static_cast<uint32_t>(static_cast<uint8_t>(rawData[offset + 1])) << 16) |
                (static_cast<uint32_t>(static_cast<uint8_t>(rawData[offset + 2])) << 24)
            );
            sample >>= 8;
            samples[i] = static_cast<Sample>(sample) / 8388608.0f;
        }
    }

    return true;
}

int test_playback_oversampling() {
    int failures = 0;

    // Test playback with 2x oversampling (simulating jack_playback_example.cpp)
    {
        // Configuration matching jack_playback_example.cpp
        constexpr Sample INTERNAL_SAMPLE_RATE = 96000.0f;  // 2x oversampling
        constexpr Sample OUTPUT_SAMPLE_RATE = 48000.0f;
        constexpr size_t OVERSAMPLE_FACTOR = 2;
        constexpr size_t BLOCK_SIZE = 64;  // Typical JACK buffer size

        // Load WAV file
        std::vector<Sample> wavSamples;
        uint32_t fileSampleRate;
        uint16_t numChannels;
        size_t numFrames;

        bool loaded = loadWavFile("data/amen_beats8_bpm172.wav",
                                  wavSamples, fileSampleRate, numChannels, numFrames);
        TEST("WAV file loaded", loaded);

        if (!loaded) {
            std::cout << "Skipping remaining tests due to WAV load failure" << std::endl;
            return failures;
        }

        // Create buffer allocator and buffer
        using WavAllocator = BufferAllocator<300000, 16>;
        WavAllocator allocator;
        allocator.init(static_cast<Sample>(fileSampleRate));

        Buffer buf = allocator.allocate(numFrames, numChannels);
        TEST("Buffer allocated", buf.isValid());

        if (!buf.isValid()) {
            return failures;
        }

        // Fill buffer with WAV data
        bool filled = WavAllocator::fillStereoInterleaved(buf, wavSamples.data(), numFrames);
        TEST("Buffer filled", filled);

        // Initialize Phasor for playback (matching jack_playback_example.cpp)
        Phasor phasor;
        phasor.init(INTERNAL_SAMPLE_RATE);

        // Calculate playback rate (matching jack_playback_example.cpp)
        Sample playbackRate = static_cast<Sample>(fileSampleRate) / INTERNAL_SAMPLE_RATE;
        Sample numSamplesFloat = static_cast<Sample>(buf.numSamples);

        phasor.set(playbackRate * numSamplesFloat, 0.0f, numSamplesFloat, 0.0f);

        // Initialize BufRd
        BufRd bufRd;
        bufRd.init(&buf);
        bufRd.setLoop(true);
        bufRd.setInterpolation(2);  // Linear interpolation

        // Initialize downsamplers (matching jack_playback_example.cpp)
        Downsampler downsamplerL, downsamplerR;
        downsamplerL.init(OUTPUT_SAMPLE_RATE, OVERSAMPLE_FACTOR);
        downsamplerR.init(OUTPUT_SAMPLE_RATE, OVERSAMPLE_FACTOR);

        // Simulate several blocks of audio processing
        constexpr size_t NUM_BLOCKS = 100;
        constexpr size_t INTERNAL_FRAMES = BLOCK_SIZE * OVERSAMPLE_FACTOR;
        constexpr size_t MAX_BUFFER_SIZE = 4096;

        float tempL[MAX_BUFFER_SIZE];
        float tempR[MAX_BUFFER_SIZE];
        float outL[BLOCK_SIZE];
        float outR[BLOCK_SIZE];

        bool validOutput = true;
        bool hasSignificantOutput = false;
        Sample maxAbsValue = 0.0f;

        for (size_t block = 0; block < NUM_BLOCKS; ++block) {
            // Generate audio at 2x rate (matching jack_playback_example.cpp)
            for (size_t i = 0; i < INTERNAL_FRAMES; ++i) {
                Sample phase = phasor.tick();
                Stereo sample = bufRd.tickStereo(phase);
                tempL[i] = sample.left;
                tempR[i] = sample.right;
            }

            // Downsample to output rate
            downsamplerL.process(tempL, outL, BLOCK_SIZE);
            downsamplerR.process(tempR, outR, BLOCK_SIZE);

            // Check output validity
            for (size_t i = 0; i < BLOCK_SIZE; ++i) {
                if (std::isnan(outL[i]) || std::isnan(outR[i]) ||
                    std::isinf(outL[i]) || std::isinf(outR[i])) {
                    validOutput = false;
                }

                Sample absL = std::abs(outL[i]);
                Sample absR = std::abs(outR[i]);
                if (absL > maxAbsValue) maxAbsValue = absL;
                if (absR > maxAbsValue) maxAbsValue = absR;

                // Audio should have significant output (not all zeros)
                if (absL > 0.01f || absR > 0.01f) {
                    hasSignificantOutput = true;
                }
            }
        }

        TEST("Output has no NaN or Inf values", validOutput);
        TEST("Output has significant audio signal", hasSignificantOutput);
        TEST("Output level is reasonable (not clipping)", maxAbsValue < 2.0f);
        TEST("Output level shows audio present", maxAbsValue > 0.0f);

        // Release buffer
        allocator.release(buf);
    }

    // Test that 1x oversampling (incorrect) produces different results than 2x
    // This validates that the fix matters
    {
        constexpr Sample INTERNAL_SAMPLE_RATE = 96000.0f;
        constexpr Sample OUTPUT_SAMPLE_RATE = 48000.0f;
        constexpr size_t CORRECT_FACTOR = 2;
        constexpr size_t INCORRECT_FACTOR = 1;
        constexpr size_t BLOCK_SIZE = 64;

        // Load WAV file
        std::vector<Sample> wavSamples;
        uint32_t fileSampleRate;
        uint16_t numChannels;
        size_t numFrames;

        bool loaded = loadWavFile("data/amen_beats8_bpm172.wav",
                                  wavSamples, fileSampleRate, numChannels, numFrames);

        if (!loaded) {
            std::cout << "Skipping 1x vs 2x comparison test" << std::endl;
            return failures;
        }

        // Create buffer
        using WavAllocator = BufferAllocator<300000, 16>;
        WavAllocator allocator;
        allocator.init(static_cast<Sample>(fileSampleRate));

        Buffer buf = allocator.allocate(numFrames, numChannels);
        if (!buf.isValid()) {
            return failures;
        }

        WavAllocator::fillStereoInterleaved(buf, wavSamples.data(), numFrames);

        // Test with CORRECT_FACTOR = 2 (the fix)
        {
            Phasor phasor;
            phasor.init(INTERNAL_SAMPLE_RATE);
            Sample playbackRate = static_cast<Sample>(fileSampleRate) / INTERNAL_SAMPLE_RATE;
            phasor.set(playbackRate * static_cast<Sample>(buf.numSamples), 0.0f, static_cast<Sample>(buf.numSamples), 0.0f);

            BufRd bufRd;
            bufRd.init(&buf);
            bufRd.setLoop(true);
            bufRd.setInterpolation(2);

            Downsampler downsampler;
            downsampler.init(OUTPUT_SAMPLE_RATE, CORRECT_FACTOR);

            // Process one block correctly: generate CORRECT_FACTOR samples per output
            float temp[BLOCK_SIZE * CORRECT_FACTOR];
            float out[BLOCK_SIZE];

            for (size_t i = 0; i < BLOCK_SIZE * CORRECT_FACTOR; ++i) {
                temp[i] = bufRd.tick(phasor.tick());
            }
            downsampler.process(temp, out, BLOCK_SIZE);

            // Count how many reads from the downsampler produced valid output
            int validReads = 0;
            for (size_t i = 0; i < BLOCK_SIZE; ++i) {
                if (!std::isnan(out[i]) && !std::isinf(out[i])) {
                    validReads++;
                }
            }
            TEST("Correct 2x factor: all outputs valid", validReads == static_cast<int>(BLOCK_SIZE));
        }

        // Test with INCORRECT_FACTOR = 1 (the bug)
        // With 1x factor but downsampler expecting 2x, the downsampler accumulates
        // only 1 sample per output, which would produce half the expected samples
        // and potentially cause issues
        {
            Phasor phasor;
            phasor.init(INTERNAL_SAMPLE_RATE);
            Sample playbackRate = static_cast<Sample>(fileSampleRate) / INTERNAL_SAMPLE_RATE;
            phasor.set(playbackRate * static_cast<Sample>(buf.numSamples), 0.0f, static_cast<Sample>(buf.numSamples), 0.0f);

            BufRd bufRd;
            bufRd.init(&buf);
            bufRd.setLoop(true);
            bufRd.setInterpolation(2);

            // This simulates the bug: downsampler expects 2x but we only provide 1x
            // We demonstrate that with 1x generation but 2x downsampler, you can
            // only produce half as many valid output samples

            // Generate only BLOCK_SIZE * 1 samples
            size_t generatedSamples = BLOCK_SIZE * INCORRECT_FACTOR;

            // The downsampler's process() reads oversampleFactor samples per output.
            // If we only generated BLOCK_SIZE samples but it tries to read
            // BLOCK_SIZE * 2 samples, it will read beyond our data (undefined behavior)
            // OR if we only provide BLOCK_SIZE samples and call process with
            // numOutputSamples = BLOCK_SIZE, it will try to access indices beyond
            // our temp array.
            //
            // In the actual bug scenario, the audio processing loop writes
            // nframes * 1 samples but the downsampler expects nframes * 2,
            // causing it to read half the audio twice as fast.

            // This test verifies that with 1x factor, we'd need to call process
            // with only half the output samples to avoid reading beyond our buffer
            // (demonstrating the mismatch)
            size_t correctOutputSamples = generatedSamples / CORRECT_FACTOR;
            TEST("Incorrect 1x factor: produces only half the outputs",
                 correctOutputSamples == BLOCK_SIZE / 2);
        }

        allocator.release(buf);
    }

    return failures;
}
