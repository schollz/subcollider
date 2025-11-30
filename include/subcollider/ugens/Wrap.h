/**
 * @file Wrap.h
 * @brief Wrap a signal into a specified range [lo, hi).
 */

#ifndef SUBCOLLIDER_UGENS_WRAP_H
#define SUBCOLLIDER_UGENS_WRAP_H

#include "../types.h"
#include <cmath>
#include <cstddef>

namespace subcollider {
namespace ugens {

/**
 * @brief Wraps a signal into a range using modular arithmetic.
 *
 * Wrap brings any input into the interval [lo, hi) with an equal range
 * on each wrap. If hi <= lo, the input returns lo to avoid division by
 * zero or negative ranges.
 */
struct Wrap {
    /**
     * @brief Wrap a single sample using explicit bounds.
     * @param input Input sample
     * @param low Lower bound
     * @param high Upper bound (exclusive)
     * @return Wrapped sample
     */
    inline Sample process(Sample input, Sample low = 0.0f, Sample high = 1.0f) const noexcept {
        Sample range = high - low;
        if (range <= 0.0f) {
            return low;
        }
        return wrapValue(input, low, range);
    }

    /**
     * @brief Wrap a single sample using cached bounds (set via setRange()).
     * @param input Input sample
     * @return Wrapped sample
     */
    inline Sample tick(Sample input) const noexcept {
        if (cachedRange <= 0.0f) {
            return cachedLow;
        }
        return wrapValue(input, cachedLow, cachedRange);
    }

    /**
     * @brief Wrap a buffer using cached bounds.
     * @param input Input buffer
     * @param output Output buffer
     * @param numSamples Number of samples
     */
    void process(const Sample* input, Sample* output, size_t numSamples) const noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            output[i] = tick(input[i]);
        }
    }

    /**
     * @brief Set wrap bounds for tick()/process().
     * @param low Lower bound
     * @param high Upper bound (exclusive)
     */
    void setRange(Sample low, Sample high) noexcept {
        cachedLow = low;
        cachedRange = high - low;
        if (cachedRange <= 0.0f) {
            cachedRange = 0.0f;
        }
    }

private:
    inline Sample wrapValue(Sample input, Sample low, Sample range) const noexcept {
        // Wrap via modulo in the shifted domain to avoid iterative subtraction
        Sample normalized = (input - low) / range;
        Sample wrapped = input - range * std::floor(normalized);
        return wrapped;
    }

    Sample cachedLow = 0.0f;
    Sample cachedRange = 1.0f;
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_WRAP_H
