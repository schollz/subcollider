/**
 * @file LFNoise2.h
 * @brief Quadratically interpolated low-frequency noise generator UGen.
 *
 * LFNoise2 generates smoothly varying random values using quadratic
 * interpolation between random control points. Designed for embedded
 * use with no heap allocation and no virtual calls.
 */

#ifndef SUBCOLLIDER_UGENS_LFNOISE2_H
#define SUBCOLLIDER_UGENS_LFNOISE2_H

#include "../types.h"

namespace subcollider {
namespace ugens {

/**
 * @brief Quadratically interpolated low-frequency noise generator.
 *
 * This is a static struct with inline per-sample processing,
 * suitable for embedded DSP with no heap allocation.
 *
 * LFNoise2 generates smoothly varying random values, useful for
 * modulating parameters like filter cutoff, amplitude, or pitch.
 *
 * Usage:
 * @code
 * LFNoise2 noise;
 * noise.init(48000.0f);
 * noise.setFrequency(4.0f);  // 4 Hz noise rate
 *
 * // Per-sample processing
 * float mod = noise.tick();
 *
 * // Block processing
 * AudioBuffer<64> buffer;
 * noise.process(buffer.data, 64);
 * @endcode
 */
struct LFNoise2 {
    /// Control points for quadratic interpolation
    Sample points[4];

    /// Current interpolation phase [0, 1)
    Sample phase;

    /// Phase increment per sample
    Sample phaseIncrement;

    /// Noise rate in Hz
    Sample frequency;

    /// Sample rate in Hz
    Sample sampleRate;

    /// Random state (simple LCG PRNG - no heap allocation)
    uint32_t seed;

    /**
     * @brief Initialize the noise generator.
     * @param sr Sample rate in Hz (default: 48000)
     * @param initialSeed Random seed (default: 12345)
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE, uint32_t initialSeed = 12345) noexcept {
        sampleRate = sr;
        seed = initialSeed;
        frequency = 1.0f;
        phase = 0.0f;
        updatePhaseIncrement();

        // Initialize all control points with random values
        for (int i = 0; i < 4; ++i) {
            points[i] = nextRandom();
        }
    }

    /**
     * @brief Set noise rate (control-rate parameter update).
     * @param freq Frequency in Hz (rate of change)
     */
    void setFrequency(Sample freq) noexcept {
        frequency = freq > 0.0f ? freq : 0.001f;
        updatePhaseIncrement();
    }

    /**
     * @brief Update phase increment from current frequency.
     */
    void updatePhaseIncrement() noexcept {
        phaseIncrement = frequency / sampleRate;
    }

    /**
     * @brief Generate next random value [-1, 1].
     *
     * Uses a Linear Congruential Generator (LCG) for deterministic,
     * heap-free random number generation.
     *
     * @return Random value in range [-1, 1]
     */
    Sample nextRandom() noexcept {
        // LCG parameters (Numerical Recipes)
        seed = seed * 1664525u + 1013904223u;
        // Convert to float [-1, 1]
        return (static_cast<Sample>(seed) / LCG_NORM) - 1.0f;
    }

    /**
     * @brief Set random seed.
     * @param newSeed New seed value
     */
    void setSeed(uint32_t newSeed) noexcept {
        seed = newSeed;
    }

    /**
     * @brief Generate single sample (per-sample inline processing).
     *
     * Uses Catmull-Rom spline interpolation for smooth output.
     *
     * @return Next noise value [-1, 1]
     */
    inline Sample tick() noexcept {
        // Catmull-Rom spline interpolation
        Sample t = phase;
        Sample t2 = t * t;
        Sample t3 = t2 * t;

        // Catmull-Rom coefficients
        Sample c0 = -0.5f * t3 + t2 - 0.5f * t;
        Sample c1 = 1.5f * t3 - 2.5f * t2 + 1.0f;
        Sample c2 = -1.5f * t3 + 2.0f * t2 + 0.5f * t;
        Sample c3 = 0.5f * t3 - 0.5f * t2;

        Sample out = c0 * points[0] + c1 * points[1] + c2 * points[2] + c3 * points[3];

        // Advance phase
        phase += phaseIncrement;

        // Check for phase wrap and shift control points
        if (phase >= 1.0f) {
            phase -= 1.0f;

            // Shift points and generate new endpoint
            points[0] = points[1];
            points[1] = points[2];
            points[2] = points[3];
            points[3] = nextRandom();
        }

        return clamp(out, -1.0f, 1.0f);
    }

    /**
     * @brief Process a block of samples.
     * @param output Output buffer
     * @param numSamples Number of samples to generate
     */
    void process(Sample* output, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            output[i] = tick();
        }
    }

    /**
     * @brief Reset noise generator.
     * @param newSeed Optional new seed value
     */
    void reset(uint32_t newSeed = 12345) noexcept {
        seed = newSeed;
        phase = 0.0f;
        for (int i = 0; i < 4; ++i) {
            points[i] = nextRandom();
        }
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_LFNOISE2_H
