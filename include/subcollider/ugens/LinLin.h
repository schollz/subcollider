/**
 * @file LinLin.h
 * @brief Linear mapping from one range to another.
 *
 * Maps an input value from [srcLo, srcHi] to [destLo, destHi] using a
 * straight line. If the source range is zero, the destination lower bound
 * is returned to avoid division by zero.
 */

#ifndef SUBCOLLIDER_UGENS_LINLIN_H
#define SUBCOLLIDER_UGENS_LINLIN_H

#include "../types.h"

namespace subcollider {
namespace ugens {

/**
 * @brief Map a value linearly between ranges.
 * @param input Input value
 * @param srcLo Source range low
 * @param srcHi Source range high
 * @param destLo Destination range low
 * @param destHi Destination range high
 * @return Mapped value
 */
inline Sample LinLin(Sample input, Sample srcLo, Sample srcHi, Sample destLo, Sample destHi) noexcept {
    Sample denom = srcHi - srcLo;
    if (denom == 0.0f) {
        return destLo;
    }
    Sample normalized = (input - srcLo) / denom;
    return destLo + normalized * (destHi - destLo);
}

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_LINLIN_H
