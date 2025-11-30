/**
 * @file CombC.h
 * @brief Comb delay line with cubic interpolation.
 *
 * CombC implements a comb filter with cubic interpolation for smoother
 * delay time modulation compared to linear or no interpolation.
 *
 * The feedback coefficient is calculated as:
 *   fb = 0.001^(delaytime / |decaytime|) * sign(decaytime)
 * where 0.001 represents -60 dBFS.
 */

#ifndef SUBCOLLIDER_UGENS_COMBC_H
#define SUBCOLLIDER_UGENS_COMBC_H

#include "../types.h"
#include <cmath>
#include <cstring>
#include <limits>

namespace subcollider {
namespace ugens {

/**
 * @brief Comb delay line with cubic interpolation.
 *
 * Provides a comb filter effect using cubic (4-point Hermite) interpolation
 * for high-quality delay time modulation.
 *
 * Usage:
 * @code
 * CombC comb;
 * comb.init(48000.0f, 1.0f);  // 1 second max delay
 * comb.setDelayTime(0.1f);     // 100ms delay
 * comb.setDecayTime(2.0f);     // 2 second decay
 *
 * // Per-sample processing
 * float output = comb.tick(input);
 *
 * // Block processing
 * comb.process(buffer, 64);
 * @endcode
 */
struct CombC {
    /// Sample rate in Hz
    Sample sampleRate;

    /// Delay buffer
    Sample* buffer;

    /// Maximum delay time in seconds
    Sample maxDelayTime;

    /// Current delay time in seconds
    Sample delayTime;

    /// Decay time in seconds
    Sample decayTime;

    /// Feedback coefficient
    Sample feedbackCoeff;

    /// Buffer size (in samples)
    size_t bufferSize;

    /// Write position in buffer
    size_t writePos;

    /**
     * @brief Initialize the comb filter.
     * @param sr Sample rate in Hz (default: 48000)
     * @param maxDelay Maximum delay time in seconds (default: 0.2)
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE, Sample maxDelay = 0.2f) noexcept {
        sampleRate = sr;
        maxDelayTime = maxDelay;

        // Allocate buffer (add extra samples for interpolation)
        bufferSize = static_cast<size_t>(std::ceil(maxDelayTime * sampleRate)) + 4;
        buffer = new Sample[bufferSize];
        std::memset(buffer, 0, bufferSize * sizeof(Sample));

        writePos = 0;
        delayTime = 0.2f;
        decayTime = 1.0f;

        updateFeedback();
    }

    /**
     * @brief Destructor - free the delay buffer.
     */
    ~CombC() noexcept {
        if (buffer) {
            delete[] buffer;
            buffer = nullptr;
        }
    }

    /**
     * @brief Set delay time.
     * @param dt Delay time in seconds
     */
    void setDelayTime(Sample dt) noexcept {
        delayTime = clamp(dt, 0.0f, maxDelayTime);
        updateFeedback();
    }

    /**
     * @brief Set decay time.
     * @param dc Decay time in seconds (can be negative for odd harmonics)
     */
    void setDecayTime(Sample dc) noexcept {
        decayTime = dc;
        updateFeedback();
    }

    /**
     * @brief Generate single filtered sample.
     * @param input Input sample
     * @return Filtered sample
     */
    inline Sample tick(Sample input) noexcept {
        // Calculate delay in samples
        Sample delaySamples = delayTime * sampleRate;

        // Calculate read position (with fractional part)
        Sample readPosFloat = static_cast<Sample>(writePos) - delaySamples;

        // Handle wraparound
        while (readPosFloat < 0.0f) {
            readPosFloat += static_cast<Sample>(bufferSize);
        }

        // Get integer and fractional parts
        size_t readPosInt = static_cast<size_t>(readPosFloat);
        Sample frac = readPosFloat - static_cast<Sample>(readPosInt);

        // Get 4 samples for cubic interpolation (y0, y1, y2, y3)
        size_t idx0 = (readPosInt + bufferSize - 1) % bufferSize;
        size_t idx1 = readPosInt % bufferSize;
        size_t idx2 = (readPosInt + 1) % bufferSize;
        size_t idx3 = (readPosInt + 2) % bufferSize;

        Sample y0 = buffer[idx0];
        Sample y1 = buffer[idx1];
        Sample y2 = buffer[idx2];
        Sample y3 = buffer[idx3];

        // 4-point Hermite interpolation
        Sample c0 = y1;
        Sample c1 = 0.5f * (y2 - y0);
        Sample c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        Sample c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        Sample delayedSample = ((c3 * frac + c2) * frac + c1) * frac + c0;

        // Apply feedback and write to buffer
        Sample output = input + feedbackCoeff * delayedSample;
        buffer[writePos] = output;

        // Advance write position
        writePos = (writePos + 1) % bufferSize;

        return delayedSample;
    }

    /**
     * @brief Process a block of samples in-place.
     * @param samples Sample buffer (input/output)
     * @param numSamples Number of samples to process
     */
    void process(Sample* samples, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            samples[i] = tick(samples[i]);
        }
    }

    /**
     * @brief Reset filter state.
     */
    void reset() noexcept {
        if (buffer) {
            std::memset(buffer, 0, bufferSize * sizeof(Sample));
        }
        writePos = 0;
    }

private:
    /**
     * @brief Update feedback coefficient based on delay and decay times.
     *
     * Feedback coefficient: fb = 0.001^(delaytime / |decaytime|) * sign(decaytime)
     * where 0.001 represents -60 dBFS.
     */
    void updateFeedback() noexcept {
        if (std::isinf(decayTime)) {
            // Infinite decay: fb = 1 (positive) or -1 (negative)
            feedbackCoeff = decayTime > 0.0f ? 1.0f : -1.0f;
        } else if (decayTime == 0.0f) {
            // No decay
            feedbackCoeff = 0.0f;
        } else {
            // fb = 0.001^(delay / |decay|) * sign(decay)
            Sample absDecay = std::fabs(decayTime);
            Sample exponent = delayTime / absDecay;

            // 0.001^x = e^(x * ln(0.001)) = e^(x * -6.907755)
            Sample fb = std::exp(exponent * -6.907755f);

            // Apply sign
            feedbackCoeff = decayTime < 0.0f ? -fb : fb;
        }
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_COMBC_H
