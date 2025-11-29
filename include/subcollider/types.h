/**
 * @file types.h
 * @brief Core type definitions for the SubCollider DSP engine.
 *
 * This header defines fundamental types used throughout the DSP engine.
 * All types are designed for embedded use with no heap allocation.
 */

#ifndef SUBCOLLIDER_TYPES_H
#define SUBCOLLIDER_TYPES_H

#include <cstdint>
#include <cstddef>

namespace subcollider {

/// Sample type for audio processing (single-precision float for embedded targets)
using Sample = float;

/// Default sample rate in Hz
constexpr Sample DEFAULT_SAMPLE_RATE = 48000.0f;

/// Default block size for audio processing
constexpr size_t DEFAULT_BLOCK_SIZE = 64;

/// Pi constant
constexpr Sample PI = 3.14159265358979323846f;

/// Two Pi constant
constexpr Sample TWO_PI = 2.0f * PI;

/// 2^31 for normalizing LCG output to [-1, 1]
constexpr Sample LCG_NORM = 2147483648.0f;

/**
 * @brief Linear interpolation between two values.
 * @param a Start value
 * @param b End value
 * @param t Interpolation factor [0, 1]
 * @return Interpolated value
 */
inline constexpr Sample lerp(Sample a, Sample b, Sample t) noexcept {
    return a + t * (b - a);
}

/**
 * @brief Clamp value to range [min, max].
 * @param value Value to clamp
 * @param min Minimum value
 * @param max Maximum value
 * @return Clamped value
 */
inline constexpr Sample clamp(Sample value, Sample min, Sample max) noexcept {
    return value < min ? min : (value > max ? max : value);
}

} // namespace subcollider

#endif // SUBCOLLIDER_TYPES_H
