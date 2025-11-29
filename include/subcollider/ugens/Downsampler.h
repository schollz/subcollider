/**
 * @file Downsampler.h
 * @brief Downsampler UGen with anti-aliasing filter.
 *
 * Downsampler provides accurate sample rate conversion from a higher rate
 * to a lower rate, using a lowpass filter to prevent aliasing.
 *
 * This is useful for oversampling workflows where UGens are computed at
 * a higher sample rate for better quality, then downsampled to the
 * output sample rate.
 */

#ifndef SUBCOLLIDER_UGENS_DOWNSAMPLER_H
#define SUBCOLLIDER_UGENS_DOWNSAMPLER_H

#include "../types.h"
#include <cmath>
#include <cstddef>
#include <cstring>

namespace subcollider {
namespace ugens {

/**
 * @brief Downsampler with anti-aliasing filter.
 *
 * The Downsampler accumulates samples at a higher rate and outputs
 * samples at a lower rate. It uses a simple but effective multi-stage
 * approach with a lowpass filter to prevent aliasing artifacts.
 *
 * Supports integer oversampling ratios (2x, 4x, etc.) for best quality.
 *
 * Usage:
 * @code
 * // Initialize UGens at 2x oversampled rate
 * constexpr size_t OVERSAMPLE = 2;
 * constexpr float OUTPUT_RATE = 48000.0f;
 * constexpr float INTERNAL_RATE = OUTPUT_RATE * OVERSAMPLE;
 *
 * SinOsc osc;
 * osc.init(INTERNAL_RATE);
 *
 * Downsampler downsampler;
 * downsampler.init(OUTPUT_RATE, OVERSAMPLE);
 *
 * // In audio callback (at output rate):
 * for (int i = 0; i < nframes; ++i) {
 *     // Generate OVERSAMPLE samples at the higher rate
 *     for (size_t j = 0; j < OVERSAMPLE; ++j) {
 *         Sample sample = osc.tick();
 *         downsampler.write(sample);
 *     }
 *     // Read one downsampled output
 *     output[i] = downsampler.read();
 * }
 * @endcode
 */
struct Downsampler {
    /// Maximum supported oversampling factor
    static constexpr size_t MAX_OVERSAMPLE = 16;

    /// Filter order for anti-aliasing (4-pole = 24dB/oct)
    static constexpr size_t FILTER_ORDER = 4;

    /// Output sample rate in Hz
    Sample outputSampleRate;

    /// Oversampling factor
    size_t oversampleFactor;

    /// Accumulator for averaging samples
    Sample accumulator;

    /// Sample counter for decimation
    size_t sampleCount;

    /// Anti-aliasing filter state (4-pole lowpass)
    Sample filterState[FILTER_ORDER];

    /// Filter coefficient
    Sample filterCoeff;

    /**
     * @brief Initialize the downsampler.
     * @param outputRate Output sample rate in Hz (default: 48000)
     * @param factor Oversampling factor (default: 2)
     *
     * The input sample rate is outputRate * factor.
     */
    void init(Sample outputRate = DEFAULT_SAMPLE_RATE, size_t factor = 2) noexcept {
        outputSampleRate = outputRate;
        oversampleFactor = factor > MAX_OVERSAMPLE ? MAX_OVERSAMPLE : (factor < 1 ? 1 : factor);
        accumulator = 0.0f;
        sampleCount = 0;

        std::memset(filterState, 0, sizeof(filterState));

        // Calculate filter coefficient for lowpass at Nyquist/2 of output rate
        // This provides good anti-aliasing with minimal high-frequency loss
        // Using a simple one-pole coefficient repeated for steeper rolloff
        Sample inputRate = outputRate * static_cast<Sample>(oversampleFactor);
        Sample cutoffFreq = outputRate * 0.45f;  // Just below Nyquist
        Sample omega = TWO_PI * cutoffFreq / inputRate;
        filterCoeff = 1.0f - std::exp(-omega);
    }

    /**
     * @brief Set the oversampling factor.
     * @param factor New oversampling factor
     *
     * Also resets the filter state.
     */
    void setOversampleFactor(size_t factor) noexcept {
        init(outputSampleRate, factor);
    }

    /**
     * @brief Write one high-rate sample into the downsampler.
     * @param sample Input sample at the higher rate
     *
     * Call this once for each sample generated at the oversampled rate.
     * After writing oversampleFactor samples, read() will return the
     * downsampled output.
     */
    inline void write(Sample sample) noexcept {
        // Apply 4-pole lowpass filter for anti-aliasing
        Sample filtered = sample;
        for (size_t i = 0; i < FILTER_ORDER; ++i) {
            filterState[i] += filterCoeff * (filtered - filterState[i]);
            filtered = filterState[i];
        }

        // Accumulate for averaging
        accumulator += filtered;
        ++sampleCount;
    }

    /**
     * @brief Read one downsampled output sample.
     * @return Downsampled sample at the output rate
     *
     * Call this once for each output sample. This should be called
     * after writing oversampleFactor samples.
     */
    inline Sample read() noexcept {
        if (sampleCount == 0) {
            return 0.0f;
        }

        // Average the accumulated samples
        Sample output = accumulator / static_cast<Sample>(sampleCount);

        // Reset for next output sample
        accumulator = 0.0f;
        sampleCount = 0;

        return output;
    }

    /**
     * @brief Process a block of high-rate samples and produce low-rate output.
     * @param input Input buffer at high rate (size = numOutputSamples * oversampleFactor)
     * @param output Output buffer at low rate (size = numOutputSamples)
     * @param numOutputSamples Number of output samples to generate
     */
    void process(const Sample* input, Sample* output, size_t numOutputSamples) noexcept {
        for (size_t i = 0; i < numOutputSamples; ++i) {
            // Write oversampleFactor high-rate samples
            for (size_t j = 0; j < oversampleFactor; ++j) {
                write(input[i * oversampleFactor + j]);
            }
            // Read one low-rate sample
            output[i] = read();
        }
    }

    /**
     * @brief Check if enough samples have been written for a read.
     * @return True if ready to read
     */
    bool isReady() const noexcept {
        return sampleCount >= oversampleFactor;
    }

    /**
     * @brief Reset the downsampler state.
     */
    void reset() noexcept {
        accumulator = 0.0f;
        sampleCount = 0;
        std::memset(filterState, 0, sizeof(filterState));
    }
};

/**
 * @brief Stereo downsampler for dual-channel audio.
 *
 * Wraps two Downsampler instances for stereo processing.
 *
 * Usage:
 * @code
 * StereoDownsampler downsampler;
 * downsampler.init(48000.0f, 2);
 *
 * // In audio callback:
 * for (int i = 0; i < nframes; ++i) {
 *     for (size_t j = 0; j < OVERSAMPLE; ++j) {
 *         Stereo sample = supersaw.tick();
 *         downsampler.write(sample);
 *     }
 *     Stereo output = downsampler.read();
 *     outL[i] = output.left;
 *     outR[i] = output.right;
 * }
 * @endcode
 */
struct StereoDownsampler {
    Downsampler left;
    Downsampler right;

    /**
     * @brief Initialize the stereo downsampler.
     * @param outputRate Output sample rate in Hz (default: 48000)
     * @param factor Oversampling factor (default: 2)
     */
    void init(Sample outputRate = DEFAULT_SAMPLE_RATE, size_t factor = 2) noexcept {
        left.init(outputRate, factor);
        right.init(outputRate, factor);
    }

    /**
     * @brief Set the oversampling factor for both channels.
     * @param factor New oversampling factor
     */
    void setOversampleFactor(size_t factor) noexcept {
        left.setOversampleFactor(factor);
        right.setOversampleFactor(factor);
    }

    /**
     * @brief Write one stereo sample at the high rate.
     * @param sample Stereo input sample
     */
    inline void write(Stereo sample) noexcept {
        left.write(sample.left);
        right.write(sample.right);
    }

    /**
     * @brief Write separate left and right samples at the high rate.
     * @param l Left channel sample
     * @param r Right channel sample
     */
    inline void write(Sample l, Sample r) noexcept {
        left.write(l);
        right.write(r);
    }

    /**
     * @brief Read one stereo sample at the output rate.
     * @return Downsampled stereo sample
     */
    inline Stereo read() noexcept {
        return Stereo(left.read(), right.read());
    }

    /**
     * @brief Process stereo blocks.
     * @param inputL Left input buffer (high rate)
     * @param inputR Right input buffer (high rate)
     * @param outputL Left output buffer (low rate)
     * @param outputR Right output buffer (low rate)
     * @param numOutputSamples Number of output samples
     */
    void process(const Sample* inputL, const Sample* inputR,
                 Sample* outputL, Sample* outputR,
                 size_t numOutputSamples) noexcept {
        left.process(inputL, outputL, numOutputSamples);
        right.process(inputR, outputR, numOutputSamples);
    }

    /**
     * @brief Check if ready to read.
     * @return True if both channels are ready
     */
    bool isReady() const noexcept {
        return left.isReady() && right.isReady();
    }

    /**
     * @brief Reset both channels.
     */
    void reset() noexcept {
        left.reset();
        right.reset();
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_DOWNSAMPLER_H
