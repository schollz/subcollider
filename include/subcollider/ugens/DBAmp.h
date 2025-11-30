/**
 * @file DBAmp.h
 * @brief Convert decibels to linear amplitude.
 */

#ifndef SUBCOLLIDER_UGENS_DBAMP_H
#define SUBCOLLIDER_UGENS_DBAMP_H

#include "../types.h"
#include <cmath>
#include <cstddef>

namespace subcollider {
namespace ugens {

/**
 * @brief Utility UGen to convert dB values to linear amplitude.
 *
 * Converts using the relationship: amp = 10^(dB / 20).
 * Inputs are plain Sample values representing decibels.
 */
struct DBAmp {
    /**
     * @brief Convert a single dB value to amplitude.
     * @param db Input in decibels
     * @return Linear amplitude
     */
    inline Sample process(Sample db) const noexcept {
        return std::pow(10.0f, db * 0.05f); // 0.05 = 1/20
    }

    /**
     * @brief Convenience alias for process().
     */
    inline Sample tick(Sample db) const noexcept {
        return process(db);
    }

    /**
     * @brief Convert a buffer of dB values to amplitudes.
     * @param input Input dB buffer
     * @param output Output amplitude buffer
     * @param numSamples Number of samples
     */
    void process(const Sample* input, Sample* output, size_t numSamples) const noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            output[i] = process(input[i]);
        }
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_DBAMP_H
