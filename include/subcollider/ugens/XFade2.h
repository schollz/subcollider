/**
 * @file XFade2.h
 * @brief Equal-power crossfader for mono or stereo signals.
 */

#ifndef SUBCOLLIDER_UGENS_XFADE2_H
#define SUBCOLLIDER_UGENS_XFADE2_H

#include "../types.h"
#include <cmath>
#include <utility>

namespace subcollider {
namespace ugens {

/**
 * @brief Two-channel equal-power crossfader.
 *
 * XFade2 crossfades between two inputs using an equal-power law so the
 * combined power remains consistent as position moves from A (-1) to
 * B (+1). Works with mono (Sample) or stereo (Stereo) inputs.
 */
struct XFade2 {
    /**
     * @brief Crossfade mono inputs to a mono output.
     * @param inA Input signal A
     * @param inB Input signal B
     * @param pos Crossfade position [-1 = A, 0 = equal, 1 = B]
     * @param level Overall output level (default 1.0)
     * @return Crossfaded mono sample
     */
    inline Sample process(Sample inA, Sample inB, Sample pos, Sample level = 1.0f) noexcept {
        auto gains = computeGains(pos, level);
        Sample gainA = gains.first;
        Sample gainB = gains.second;
        return inA * gainA + inB * gainB;
    }

    /**
     * @brief Crossfade stereo inputs to a stereo output.
     * @param inA Stereo input A
     * @param inB Stereo input B
     * @param pos Crossfade position [-1 = A, 0 = equal, 1 = B]
     * @param level Overall output level (default 1.0)
     * @return Crossfaded stereo signal
     */
    inline Stereo process(const Stereo& inA, const Stereo& inB, Sample pos, Sample level = 1.0f) noexcept {
        auto gains = computeGains(pos, level);
        Sample gainA = gains.first;
        Sample gainB = gains.second;
        return Stereo(inA.left * gainA + inB.left * gainB,
                      inA.right * gainA + inB.right * gainB);
    }

    /**
     * @brief Crossfade mono inputs using cached gains set by setPosition().
     */
    inline Sample tick(Sample inA, Sample inB) noexcept {
        return inA * cachedGainA + inB * cachedGainB;
    }

    /**
     * @brief Crossfade stereo inputs using cached gains set by setPosition().
     */
    inline Stereo tick(const Stereo& inA, const Stereo& inB) noexcept {
        return Stereo(inA.left * cachedGainA + inB.left * cachedGainB,
                      inA.right * cachedGainA + inB.right * cachedGainB);
    }

    /**
     * @brief Set crossfade position and cache gains for tick().
     * @param pos Crossfade position [-1 = A, 0 = equal, 1 = B]
     * @param level Overall output level (default 1.0)
     */
    void setPosition(Sample pos, Sample level = 1.0f) noexcept {
        auto gains = computeGains(pos, level);
        Sample gainA = gains.first;
        Sample gainB = gains.second;
        cachedBaseA = gainA / (level + 1e-12f);
        cachedBaseB = gainB / (level + 1e-12f);
        cachedGainA = gainA;
        cachedGainB = gainB;
    }

    /**
     * @brief Adjust level without changing crossfade position.
     * @param level New overall output level
     */
    void setLevel(Sample level) noexcept {
        cachedGainA = cachedBaseA * level;
        cachedGainB = cachedBaseB * level;
    }

private:
    /**
     * @brief Compute equal-power gains for a given position/level.
     */
    inline std::pair<Sample, Sample> computeGains(Sample pos, Sample level) const noexcept {
        pos = clamp(pos, -1.0f, 1.0f);
        Sample angle = (pos + 1.0f) * 0.78539816339f; // PI/4
        Sample gainA = std::cos(angle) * level;
        Sample gainB = std::sin(angle) * level;
        return {gainA, gainB};
    }

    Sample cachedBaseA = 0.70710678118f; // cos(PI/4)
    Sample cachedBaseB = 0.70710678118f; // sin(PI/4)
    Sample cachedGainA = 0.70710678118f;
    Sample cachedGainB = 0.70710678118f;
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_XFADE2_H
