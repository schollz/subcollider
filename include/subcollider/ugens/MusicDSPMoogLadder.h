/**
 * @file MusicDSPMoogLadder.h
 * @brief MusicDSP Moog Ladder filter UGen.
 *
 * This file is unlicensed and uncopyright as found at:
 * http://www.musicdsp.org/showone.php?id=24
 * Considering how widely this same code has been used, it might be 
 * reasonable to suggest that the license is CC-BY-SA
 */

#ifndef SUBCOLLIDER_UGENS_MUSICDSPMOOGLADDER_H
#define SUBCOLLIDER_UGENS_MUSICDSPMOOGLADDER_H

#include "../types.h"
#include <cmath>
#include <cstring>

namespace subcollider {
namespace ugens {

/**
 * @brief MusicDSP Moog Ladder filter.
 *
 * Classic Moog filter implementation using bilinear transform for
 * four cascaded one-pole filters.
 *
 * Usage:
 * @code
 * MusicDSPMoogLadder filter;
 * filter.init(48000.0f);
 * filter.setCutoff(1000.0f);
 * filter.setResonance(0.5f);
 *
 * // Per-sample processing
 * float output = filter.tick(input);
 *
 * // Block processing
 * filter.process(buffer, 64);
 * @endcode
 */
struct MusicDSPMoogLadder {
    /// Sample rate in Hz
    Sample sampleRate;

    /// Cutoff frequency in Hz
    Sample cutoff;

    /// Resonance [0, 1]
    Sample resonance;

    /**
     * @brief Initialize the filter.
     * @param sr Sample rate in Hz (default: 48000)
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE) noexcept {
        sampleRate = sr;
        std::memset(stage, 0, sizeof(stage));
        std::memset(delay, 0, sizeof(delay));
        p = 0.0;
        k = 0.0;
        t1 = 0.0;
        t2 = 0.0;
        resonanceCoeff = 0.0;
        setCutoff(1000.0f);
        setResonance(0.1f);
    }

    /**
     * @brief Set the cutoff frequency.
     * @param c Cutoff frequency in Hz
     */
    void setCutoff(Sample c) noexcept {
        cutoff = c;
        double fc = 2.0 * c / sampleRate;

        p = fc * (1.8 - 0.8 * fc);
        k = 2.0 * std::sin(fc * PI * 0.5) - 1.0;
        t1 = (1.0 - p) * 1.386249;
        t2 = 12.0 + t1 * t1;

        setResonance(resonance);
    }

    /**
     * @brief Set the resonance.
     * @param r Resonance [0, 1]
     */
    void setResonance(Sample r) noexcept {
        resonance = r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r);
        resonanceCoeff = resonance * (t2 + 6.0 * t1) / (t2 - 6.0 * t1);
    }

    /**
     * @brief Generate single filtered sample.
     * @param input Input sample
     * @return Filtered sample
     */
    inline Sample tick(Sample input) noexcept {
        double x = input - resonanceCoeff * stage[3];

        // Four cascaded one-pole filters (bilinear transform)
        stage[0] = x * p + delay[0] * p - k * stage[0];
        stage[1] = stage[0] * p + delay[1] * p - k * stage[1];
        stage[2] = stage[1] * p + delay[2] * p - k * stage[2];
        stage[3] = stage[2] * p + delay[3] * p - k * stage[3];

        // Clipping band-limited sigmoid
        stage[3] -= (stage[3] * stage[3] * stage[3]) / 6.0;

        delay[0] = x;
        delay[1] = stage[0];
        delay[2] = stage[1];
        delay[3] = stage[2];

        return static_cast<Sample>(stage[3]);
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
        std::memset(stage, 0, sizeof(stage));
        std::memset(delay, 0, sizeof(delay));
    }

private:
    double stage[4];
    double delay[4];

    double p;
    double k;
    double t1;
    double t2;
    double resonanceCoeff;
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_MUSICDSPMOOGLADDER_H
