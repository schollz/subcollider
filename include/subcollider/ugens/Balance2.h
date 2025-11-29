/**
 * @file Balance2.h
 * @brief Equal power stereo balancer UGen.
 *
 * Balance2 provides equal-power balancing between two stereo channels.
 * Unlike Pan2 which pans a mono signal, Balance2 takes a stereo input
 * and balances between the left and right channels.
 */

#ifndef SUBCOLLIDER_UGENS_BALANCE2_H
#define SUBCOLLIDER_UGENS_BALANCE2_H

#include "../types.h"
#include <cmath>

namespace subcollider {
namespace ugens {

/**
 * @brief Equal power stereo balance control.
 *
 * Balance2 balances between two input channels (left and right) using
 * equal-power panning curves. By panning from left (pos = -1) to right
 * (pos = 1), you decrement the level of the left channel from 1 to 0
 * using a square root curve, while incrementing the right channel from
 * 0 to 1 using the same curve.
 *
 * Position:
 * - -1.0 = full left (left channel only)
 * -  0.0 = center (both channels at 0.707 or -3dB)
 * - +1.0 = full right (right channel only)
 *
 * The output remains a stereo signal.
 *
 * Usage:
 * @code
 * Balance2 balance;
 * Sample leftIn = 0.5f;
 * Sample rightIn = 0.3f;
 * Sample position = 0.5f;  // Favor right
 * Stereo output = balance.process(leftIn, rightIn, position);
 * @endcode
 */
struct Balance2 {
    /**
     * @brief Process stereo input to balanced stereo output.
     * @param left Left channel input
     * @param right Right channel input
     * @param pos Balance position [-1.0 = left, 0.0 = center, 1.0 = right]
     * @param level Overall output level (default: 1.0)
     * @return Stereo output with equal-power balancing applied
     */
    inline Stereo process(Sample left, Sample right, Sample pos, Sample level = 1.0f) noexcept {
        // Clamp position to valid range
        pos = clamp(pos, -1.0f, 1.0f);

        // Equal-power balancing using sin/cos for smooth curves
        // Map pos from [-1, 1] to [0, PI/2] for the balance angle
        // angle = (pos + 1) * PI/4 maps [-1,1] to [0, PI/2]
        Sample angle = (pos + 1.0f) * 0.78539816339f; // PI/4 ≈ 0.78539816339

        // Equal-power balance law:
        // leftGain  = cos(angle)  (1.0 at pos=-1, 0.707 at pos=0, 0.0 at pos=1)
        // rightGain = sin(angle)  (0.0 at pos=-1, 0.707 at pos=0, 1.0 at pos=1)
        Sample leftGain = std::cos(angle);
        Sample rightGain = std::sin(angle);

        return Stereo(left * leftGain * level, right * rightGain * level);
    }

    /**
     * @brief Simplified tick for constant balance position and level.
     * @param left Left channel input
     * @param right Right channel input
     * @return Stereo output (requires setPosition to be called first)
     */
    inline Stereo tick(Sample left, Sample right) noexcept {
        return Stereo(left * cachedLeftGain, right * cachedRightGain);
    }

    /**
     * @brief Set balance position and cache coefficients for tick().
     * @param pos Balance position [-1.0 = left, 0.0 = center, 1.0 = right]
     * @param level Overall output level (default: 1.0)
     */
    void setPosition(Sample pos, Sample level = 1.0f) noexcept {
        pos = clamp(pos, -1.0f, 1.0f);
        Sample angle = (pos + 1.0f) * 0.78539816339f;
        cachedLeftGain = std::cos(angle) * level;
        cachedRightGain = std::sin(angle) * level;
    }

    /**
     * @brief Set output level without changing balance position.
     * @param level Overall output level
     */
    void setLevel(Sample level) noexcept {
        // Recalculate gains with new level, preserving the ratio
        Sample ratio = cachedRightGain / (cachedLeftGain + 1e-10f); // Avoid division by zero
        Sample angle = std::atan(ratio);
        cachedLeftGain = std::cos(angle) * level;
        cachedRightGain = std::sin(angle) * level;
    }

private:
    Sample cachedLeftGain = 0.70710678118f;   // cos(PI/4) for center position
    Sample cachedRightGain = 0.70710678118f;  // sin(PI/4) for center position
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_BALANCE2_H
