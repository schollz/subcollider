/**
 * @file Pan2.h
 * @brief Stereo panner UGen for positioning mono signals in stereo field.
 *
 * Pan2 uses equal-power panning for smooth, natural stereo imaging.
 */

#ifndef SUBCOLLIDER_UGENS_PAN2_H
#define SUBCOLLIDER_UGENS_PAN2_H

#include "../types.h"
#include <cmath>

namespace subcollider {
namespace ugens {

/**
 * @brief Stereo panner using equal-power panning law.
 *
 * Pan2 positions a mono signal in the stereo field using equal-power
 * panning curves (constant power law). This ensures consistent perceived
 * loudness as the signal moves across the stereo field.
 *
 * Pan position:
 * - -1.0 = full left
 * -  0.0 = center (equal in both channels)
 * - +1.0 = full right
 *
 * Usage:
 * @code
 * Pan2 panner;
 * Sample monoSignal = 0.5f;
 * Sample panPosition = -0.3f;  // Slightly left
 * Stereo output = panner.process(monoSignal, panPosition);
 * @endcode
 */
struct Pan2 {
    /**
     * @brief Process mono input to stereo output with panning.
     * @param input Mono input signal
     * @param pan Pan position [-1.0 = left, 0.0 = center, 1.0 = right]
     * @return Stereo output with equal-power panning applied
     */
    inline Stereo process(Sample input, Sample pan) noexcept {
        // Clamp pan to valid range
        pan = clamp(pan, -1.0f, 1.0f);

        // Equal-power panning using sin/cos for smooth curves
        // Map pan from [-1, 1] to [0, PI/2] for the panning angle
        // angle = (pan + 1) * PI/4 maps [-1,1] to [0, PI/2]
        Sample angle = (pan + 1.0f) * 0.78539816339f; // PI/4 ≈ 0.78539816339

        // Equal-power panning law:
        // left  = cos(angle)
        // right = sin(angle)
        Sample left = std::cos(angle);
        Sample right = std::sin(angle);

        return Stereo(input * left, input * right);
    }

    /**
     * @brief Simplified tick for constant pan position.
     * @param input Mono input signal
     * @return Stereo output (requires setPan to be called first)
     */
    inline Stereo tick(Sample input) noexcept {
        return Stereo(input * cachedLeft, input * cachedRight);
    }

    /**
     * @brief Set pan position and cache coefficients for tick().
     * @param pan Pan position [-1.0 = left, 0.0 = center, 1.0 = right]
     */
    void setPan(Sample pan) noexcept {
        pan = clamp(pan, -1.0f, 1.0f);
        Sample angle = (pan + 1.0f) * 0.78539816339f;
        cachedLeft = std::cos(angle);
        cachedRight = std::sin(angle);
    }

private:
    Sample cachedLeft = 0.70710678118f;   // cos(PI/4) for center pan
    Sample cachedRight = 0.70710678118f;  // sin(PI/4) for center pan
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_PAN2_H
