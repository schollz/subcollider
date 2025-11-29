/**
 * @file test_bufrd_phasor.cpp
 * @brief Unit tests for BufRd + Phasor combination.
 *
 * Tests BufRd and Phasor working together to play through
 * data/amen_beats8_bpm172.wav once. Verifies that BufRd output
 * matches the original WAV file data exactly (no interpolation).
 */

#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <vector>
#include <subcollider/BufferAllocator.h>
#include <subcollider/ugens/BufRd.h>
#include <subcollider/ugens/Phasor.h>

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
    char riffId[4];         // "RIFF"
    uint32_t fileSize;      // File size - 8
    char waveId[4];         // "WAVE"
    char fmtId[4];          // "fmt "
    uint32_t fmtSize;       // Format chunk size (16 for PCM)
    uint16_t audioFormat;   // Audio format (1 = PCM)
    uint16_t numChannels;   // Number of channels
    uint32_t sampleRate;    // Sample rate in Hz
    uint32_t byteRate;      // Bytes per second
    uint16_t blockAlign;    // Bytes per sample frame
    uint16_t bitsPerSample; // Bits per sample
};

/**
 * @brief Load a WAV file into a vector of float samples (interleaved stereo).
 *
 * Supports 16-bit and 24-bit PCM WAV files, including WAVE_FORMAT_EXTENSIBLE.
 *
 * @param filename Path to the WAV file
 * @param samples Output vector of samples (interleaved if stereo)
 * @param sampleRate Output sample rate
 * @param numChannels Output number of channels
 * @param numFrames Output number of frames (samples per channel)
 * @return true if successful, false otherwise
 */
bool loadWavFile(const char* filename, std::vector<Sample>& samples,
                 uint32_t& sampleRate, uint16_t& numChannels, size_t& numFrames) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open WAV file: " << filename << std::endl;
        return false;
    }

    // Read RIFF header
    WavHeader header;
    file.read(header.riffId, 4);
    if (std::strncmp(header.riffId, "RIFF", 4) != 0) {
        std::cerr << "Not a valid RIFF file" << std::endl;
        return false;
    }

    file.read(reinterpret_cast<char*>(&header.fileSize), 4);
    file.read(header.waveId, 4);
    if (std::strncmp(header.waveId, "WAVE", 4) != 0) {
        std::cerr << "Not a valid WAVE file" << std::endl;
        return false;
    }

    // Find format chunk
    bool foundFmt = false;
    bool foundData = false;
    uint32_t dataSize = 0;
    uint16_t actualFormat = 0;  // The actual format (1 = PCM)

    while (!foundFmt || !foundData) {
        char chunkId[4];
        uint32_t chunkSize;

        if (!file.read(chunkId, 4)) break;
        if (!file.read(reinterpret_cast<char*>(&chunkSize), 4)) break;

        if (std::strncmp(chunkId, "fmt ", 4) == 0) {
            // Read format chunk
            file.read(reinterpret_cast<char*>(&header.audioFormat), 2);
            file.read(reinterpret_cast<char*>(&header.numChannels), 2);
            file.read(reinterpret_cast<char*>(&header.sampleRate), 4);
            file.read(reinterpret_cast<char*>(&header.byteRate), 4);
            file.read(reinterpret_cast<char*>(&header.blockAlign), 2);
            file.read(reinterpret_cast<char*>(&header.bitsPerSample), 2);

            actualFormat = header.audioFormat;

            // Handle WAVE_FORMAT_EXTENSIBLE (0xFFFE = 65534)
            if (header.audioFormat == 0xFFFE && chunkSize > 16) {
                uint16_t cbSize;
                file.read(reinterpret_cast<char*>(&cbSize), 2);

                if (cbSize >= 22) {
                    uint16_t validBitsPerSample;
                    uint32_t channelMask;
                    uint16_t subFormat;

                    file.read(reinterpret_cast<char*>(&validBitsPerSample), 2);
                    file.read(reinterpret_cast<char*>(&channelMask), 4);
                    file.read(reinterpret_cast<char*>(&subFormat), 2);  // First 2 bytes of GUID

                    actualFormat = subFormat;  // e.g., 1 for PCM

                    // Skip rest of GUID and any remaining extension bytes
                    size_t remaining = cbSize - 22 + 14;  // 14 = remaining GUID bytes
                    if (remaining > 0) {
                        file.seekg(remaining, std::ios::cur);
                    }
                } else {
                    // Skip unknown extension
                    file.seekg(cbSize, std::ios::cur);
                }
            } else if (chunkSize > 16) {
                // Skip any extra format bytes for non-extensible format
                file.seekg(chunkSize - 16, std::ios::cur);
            }
            foundFmt = true;
        } else if (std::strncmp(chunkId, "data", 4) == 0) {
            dataSize = chunkSize;
            foundData = true;
            break;  // Stop here, data follows
        } else {
            // Skip unknown chunk
            file.seekg(chunkSize, std::ios::cur);
        }
    }

    if (!foundFmt || !foundData) {
        std::cerr << "Could not find fmt or data chunk" << std::endl;
        return false;
    }

    // Validate format (check actual format, not extensible wrapper)
    if (actualFormat != 1) {
        std::cerr << "Only PCM format is supported (format code: " << actualFormat << ")" << std::endl;
        return false;
    }

    if (header.bitsPerSample != 16 && header.bitsPerSample != 24) {
        std::cerr << "Only 16-bit and 24-bit samples are supported" << std::endl;
        return false;
    }

    // Calculate number of frames
    size_t bytesPerSample = header.bitsPerSample / 8;
    numFrames = dataSize / (bytesPerSample * header.numChannels);
    sampleRate = header.sampleRate;
    numChannels = header.numChannels;

    // Read and convert samples
    size_t totalSamples = numFrames * header.numChannels;
    samples.resize(totalSamples);

    std::vector<char> rawData(dataSize);
    file.read(rawData.data(), dataSize);

    if (header.bitsPerSample == 16) {
        // 16-bit samples
        for (size_t i = 0; i < totalSamples; ++i) {
            int16_t sample = *reinterpret_cast<int16_t*>(&rawData[i * 2]);
            samples[i] = static_cast<Sample>(sample) / 32768.0f;
        }
    } else if (header.bitsPerSample == 24) {
        // 24-bit samples (little-endian, sign-extend to 32-bit)
        for (size_t i = 0; i < totalSamples; ++i) {
            size_t offset = i * 3;
            int32_t sample = static_cast<int32_t>(
                (static_cast<uint8_t>(rawData[offset]) << 8) |
                (static_cast<uint8_t>(rawData[offset + 1]) << 16) |
                (static_cast<uint8_t>(rawData[offset + 2]) << 24)
            ) >> 8;  // Sign-extend by shifting
            samples[i] = static_cast<Sample>(sample) / 8388608.0f;  // 2^23
        }
    }

    return true;
}

int test_bufrd_phasor() {
    int failures = 0;

    // Test BufRd + Phasor playing through amen_beats8_bpm172.wav
    {
        // Load WAV file
        std::vector<Sample> wavSamples;
        uint32_t sampleRate;
        uint16_t numChannels;
        size_t numFrames;

        bool loaded = loadWavFile("data/amen_beats8_bpm172.wav",
                                  wavSamples, sampleRate, numChannels, numFrames);
        TEST("WAV file loaded successfully", loaded);

        if (!loaded) {
            std::cout << "Skipping remaining tests due to WAV load failure" << std::endl;
            return failures;
        }

        TEST("WAV file is stereo", numChannels == 2);
        TEST("WAV sample rate is 44100 Hz", sampleRate == 44100);
        TEST("WAV has expected number of frames", numFrames == 123069);

        // Create BufferAllocator with enough space for the WAV file
        // Need at least numFrames * 2 floats for stereo
        using WavAllocator = BufferAllocator<300000, 16>;
        WavAllocator allocator;
        allocator.init(static_cast<Sample>(sampleRate));

        // Allocate stereo buffer
        Buffer buf = allocator.allocate(numFrames, numChannels);
        TEST("Buffer allocated successfully", buf.isValid());
        TEST("Buffer is stereo", buf.isStereo());
        TEST("Buffer has correct sample count", buf.numSamples == numFrames);
        TEST("Buffer has correct sample rate", buf.sampleRate == static_cast<Sample>(sampleRate));

        if (!buf.isValid()) {
            std::cout << "Skipping remaining tests due to buffer allocation failure" << std::endl;
            return failures;
        }

        // Fill buffer with WAV data (already interleaved)
        bool filled = WavAllocator::fillStereoInterleaved(buf, wavSamples.data(), numFrames);
        TEST("Buffer filled with WAV data", filled);

        // Create Phasor at the same sample rate as the buffer
        // Rate of 1.0 means advance by 1 sample per tick
        Phasor phasor;
        phasor.init(static_cast<Sample>(sampleRate));
        phasor.set(1.0f, 0.0f, static_cast<Sample>(numFrames));

        // Create BufRd with no interpolation (1) for exact 1:1 sample reading
        BufRd bufRd;
        bufRd.init(&buf);
        bufRd.setLoop(false);  // Don't loop, play through once
        bufRd.setInterpolation(1);  // No interpolation for exact sample matching

        // Play through the entire file and verify output matches input
        bool allSamplesMatch = true;
        size_t mismatchCount = 0;
        Sample maxDiff = 0.0f;
        size_t firstMismatchIndex = 0;

        for (size_t i = 0; i < numFrames; ++i) {
            // Get phase from Phasor
            Sample phase = phasor.tick();

            // Read sample from buffer using BufRd
            Stereo output = bufRd.tickStereo(phase);

            // Get expected values directly from WAV data
            Sample expectedLeft = wavSamples[i * 2];
            Sample expectedRight = wavSamples[i * 2 + 1];

            // Compare output to expected
            Sample diffLeft = std::abs(output.left - expectedLeft);
            Sample diffRight = std::abs(output.right - expectedRight);

            // With no interpolation and integer phase values, should be exact match
            const Sample tolerance = 0.0001f;
            if (diffLeft > tolerance || diffRight > tolerance) {
                if (allSamplesMatch) {
                    firstMismatchIndex = i;
                }
                allSamplesMatch = false;
                mismatchCount++;
                if (diffLeft > maxDiff) maxDiff = diffLeft;
                if (diffRight > maxDiff) maxDiff = diffRight;
            }
        }

        TEST("All samples match (BufRd output mirrors WAV data)", allSamplesMatch);

        if (!allSamplesMatch) {
            std::cout << "  Mismatch count: " << mismatchCount << std::endl;
            std::cout << "  Max difference: " << maxDiff << std::endl;
            std::cout << "  First mismatch at sample: " << firstMismatchIndex << std::endl;
        }

        // Verify Phasor is at the end of the buffer
        // After numFrames ticks, phasor value should be at numFrames (wrapped back to 0 if looping,
        // but we didn't loop so it should be at the end)
        TEST("Phasor completed full playback", phasor.value >= static_cast<Sample>(numFrames));

        // Additional verification: spot-check some specific samples
        {
            // Reset and verify first sample
            phasor.reset();
            Sample phase = phasor.tick();
            Stereo first = bufRd.tickStereo(phase);
            TEST("First sample left matches", std::abs(first.left - wavSamples[0]) < 0.0001f);
            TEST("First sample right matches", std::abs(first.right - wavSamples[1]) < 0.0001f);
        }

        {
            // Test middle sample
            size_t midIndex = numFrames / 2;
            phasor.reset(static_cast<Sample>(midIndex));
            Sample phase = phasor.tick();
            Stereo mid = bufRd.tickStereo(phase);
            TEST("Middle sample left matches",
                 std::abs(mid.left - wavSamples[midIndex * 2]) < 0.0001f);
            TEST("Middle sample right matches",
                 std::abs(mid.right - wavSamples[midIndex * 2 + 1]) < 0.0001f);
        }

        {
            // Test last sample
            size_t lastIndex = numFrames - 1;
            phasor.reset(static_cast<Sample>(lastIndex));
            Sample phase = phasor.tick();
            Stereo last = bufRd.tickStereo(phase);
            TEST("Last sample left matches",
                 std::abs(last.left - wavSamples[lastIndex * 2]) < 0.0001f);
            TEST("Last sample right matches",
                 std::abs(last.right - wavSamples[lastIndex * 2 + 1]) < 0.0001f);
        }

        // Release buffer
        allocator.release(buf);
    }

    return failures;
}
