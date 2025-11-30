/**
 * @file FVerb.h
 * @brief FVerb reverb UGen - high-quality algorithmic reverb.
 *
 * FVerb is a Faust-based reverb implementation by Jean Pierre Cimalando.
 * This is a block-based stereo effect (no single-sample tick).
 *
 * License: BSD-2-Clause
 * Author: Jean Pierre Cimalando
 */

#ifndef SUBCOLLIDER_UGENS_FVERB_H
#define SUBCOLLIDER_UGENS_FVERB_H

#include "../types.h"
#include <cstddef>
#include <cstring>

// Forward declaration of the Faust DSP implementation (in global namespace)
class FVerbDSP;

namespace subcollider {
namespace ugens {

/**
 * @brief High-quality algorithmic stereo reverb.
 *
 * FVerb is a block-based stereo reverb effect. It processes stereo input
 * to stereo output in blocks (no single-sample processing).
 *
 * Usage:
 * @code
 * FVerb reverb;
 * reverb.init(48000.0f);
 * reverb.setDecay(0.82f);
 * reverb.setDamping(5500.0f);
 *
 * // Process stereo block
 * Sample leftBuffer[64];
 * Sample rightBuffer[64];
 * reverb.process(leftBuffer, rightBuffer, 64);
 * @endcode
 */
struct FVerb {
    /// Sample rate in Hz
    Sample sampleRate;

    /**
     * @brief Initialize the reverb.
     * @param sr Sample rate in Hz (default: 48000)
     */
    void init(Sample sr = DEFAULT_SAMPLE_RATE) noexcept;

    /**
     * @brief Process a stereo block of samples in-place.
     * @param left Left channel buffer (input/output)
     * @param right Right channel buffer (input/output)
     * @param numSamples Number of samples to process
     */
    void process(Sample* left, Sample* right, size_t numSamples) noexcept;

    /**
     * @brief Set predelay time.
     * @param ms Predelay time in milliseconds [0, 300]
     */
    void setPredelay(Sample ms) noexcept;

    /**
     * @brief Set input amount.
     * @param amount Input amount percentage [0, 100]
     */
    void setInputAmount(Sample amount) noexcept;

    /**
     * @brief Set input low-pass filter cutoff.
     * @param hz Cutoff frequency in Hz [1, 20000]
     */
    void setInputLowPassCutoff(Sample hz) noexcept;

    /**
     * @brief Set input high-pass filter cutoff.
     * @param hz Cutoff frequency in Hz [1, 1000]
     */
    void setInputHighPassCutoff(Sample hz) noexcept;

    /**
     * @brief Set input diffusion 1.
     * @param amount Diffusion percentage [0, 100]
     */
    void setInputDiffusion1(Sample amount) noexcept;

    /**
     * @brief Set input diffusion 2.
     * @param amount Diffusion percentage [0, 100]
     */
    void setInputDiffusion2(Sample amount) noexcept;

    /**
     * @brief Set tail density.
     * @param amount Tail density percentage [0, 100]
     */
    void setTailDensity(Sample amount) noexcept;

    /**
     * @brief Set decay time.
     * @param amount Decay percentage [0, 100]
     */
    void setDecay(Sample amount) noexcept;

    /**
     * @brief Set damping frequency.
     * @param hz Damping frequency in Hz [10, 20000]
     */
    void setDamping(Sample hz) noexcept;

    /**
     * @brief Set modulation frequency.
     * @param hz Modulation frequency in Hz [0.01, 4.0]
     */
    void setModulatorFrequency(Sample hz) noexcept;

    /**
     * @brief Set modulation depth.
     * @param ms Modulation depth in milliseconds [0, 10]
     */
    void setModulatorDepth(Sample ms) noexcept;

    /**
     * @brief Reset reverb state.
     */
    void reset() noexcept;

    /**
     * @brief Destructor - cleanup DSP resources.
     */
    ~FVerb();

private:
    FVerbDSP* dsp = nullptr;
    Sample** inputBuffers = nullptr;
    Sample** outputBuffers = nullptr;
    size_t allocatedBlockSize = 0;

    void allocateBuffers(size_t numSamples) noexcept;
    void freeBuffers() noexcept;
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_FVERB_H
