/**
 * @file StilsonMoogLadder.h
 * @brief Stilson Moog Ladder filter UGen.
 *
 * Based on an implementation by David Lowenfels, released as the moog~ pd extern.
 * This code is Unlicensed (i.e. public domain).
 *
 * References: Stilson and Smith (1996), DAFX - Zolzer (ed) (2nd ed)
 * Original implementation: Tim Stilson, David Lowenfels
 */

#ifndef SUBCOLLIDER_UGENS_STILSONMOOGLADDER_H
#define SUBCOLLIDER_UGENS_STILSONMOOGLADDER_H

#include "../types.h"
#include <cmath>
#include <cstring>

namespace subcollider {
namespace ugens {

/**
 * @brief Stilson Moog Ladder filter.
 *
 * A digital model of the classic Moog filter using a cascade of one-pole IIR
 * filters in series with global feedback to produce resonance.
 *
 * Usage:
 * @code
 * StilsonMoogLadder filter;
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
struct StilsonMoogLadder {
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
        std::memset(state, 0, sizeof(state));
        output = 0.0;
        p = 0.0;
        Q = 0.0;
        setCutoff(1000.0f);
        setResonance(0.1f);
    }

    /**
     * @brief Set the cutoff frequency.
     * @param c Cutoff frequency in Hz
     */
    void setCutoff(Sample c) noexcept {
        cutoff = c;

        // Normalized cutoff between [0, 1]
        double fc = static_cast<double>(cutoff) / sampleRate;
        double x2 = fc * fc;
        double x3 = fc * fc * fc;

        // Frequency & amplitude correction (Cubic Fit)
        p = -0.69346 * x3 - 0.59515 * x2 + 3.2937 * fc - 1.0072;

        setResonance(resonance);
    }

    /**
     * @brief Set the resonance.
     * @param r Resonance [0, 1]
     */
    void setResonance(Sample r) noexcept {
        r = r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r);
        resonance = r;

        double ix;
        double ixfrac;
        int ixint;

        ix = p * 99;
        ixint = static_cast<int>(std::floor(ix));
        ixfrac = ix - ixint;

        // Clamp index to valid range
        if (ixint < -99) ixint = -99;
        if (ixint > 98) ixint = 98;

        Q = r * lerp(static_cast<Sample>(S_GAINTABLE[ixint + 99]),
                     static_cast<Sample>(S_GAINTABLE[ixint + 100]),
                     static_cast<Sample>(ixfrac));
    }

    /**
     * @brief Generate single filtered sample.
     * @param input Input sample
     * @return Filtered sample
     */
    inline Sample tick(Sample input) noexcept {
        double localState;

        // Scale by arbitrary value on account of our saturation function
        const double in = input * 0.65;

        // Negative Feedback
        output = 0.25 * (in - output);

        for (int pole = 0; pole < 4; ++pole) {
            localState = state[pole];
            output = saturate(output + p * (output - localState));
            state[pole] = output;
            output = saturate(output + localState);
        }

        snapToZero(output);
        double result = output;
        output *= Q; // Scale stateful output by Q

        return static_cast<Sample>(result);
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
        std::memset(state, 0, sizeof(state));
        output = 0.0;
    }

private:
    double p;
    double Q;
    double state[4];
    double output;

    static constexpr float S_GAINTABLE[199] = {
        0.999969f, 0.990082f, 0.980347f, 0.970764f, 0.961304f, 0.951996f, 0.94281f, 0.933777f, 0.924866f, 0.916077f,
        0.90741f, 0.898865f, 0.890442f, 0.882141f, 0.873962f, 0.865906f, 0.857941f, 0.850067f, 0.842346f, 0.834686f,
        0.827148f, 0.819733f, 0.812378f, 0.805145f, 0.798004f, 0.790955f, 0.783997f, 0.77713f, 0.770355f, 0.763672f,
        0.75708f, 0.75058f, 0.744141f, 0.737793f, 0.731537f, 0.725342f, 0.719238f, 0.713196f, 0.707245f, 0.701355f,
        0.695557f, 0.689819f, 0.684174f, 0.678558f, 0.673035f, 0.667572f, 0.66217f, 0.65686f, 0.651581f, 0.646393f,
        0.641235f, 0.636169f, 0.631134f, 0.62619f, 0.621277f, 0.616425f, 0.611633f, 0.606903f, 0.602234f, 0.597626f,
        0.593048f, 0.588531f, 0.584045f, 0.579651f, 0.575287f, 0.570953f, 0.566681f, 0.562469f, 0.558289f, 0.554169f,
        0.550079f, 0.546051f, 0.542053f, 0.538116f, 0.53421f, 0.530334f, 0.52652f, 0.522736f, 0.518982f, 0.515289f,
        0.511627f, 0.507996f, 0.504425f, 0.500885f, 0.497375f, 0.493896f, 0.490448f, 0.487061f, 0.483704f, 0.480377f,
        0.477081f, 0.473816f, 0.470581f, 0.467377f, 0.464203f, 0.46109f, 0.457977f, 0.454926f, 0.451874f, 0.448883f,
        0.445892f, 0.442932f, 0.440033f, 0.437134f, 0.434265f, 0.431427f, 0.428619f, 0.425842f, 0.423096f, 0.42038f,
        0.417664f, 0.415009f, 0.412354f, 0.409729f, 0.407135f, 0.404572f, 0.402008f, 0.399506f, 0.397003f, 0.394501f,
        0.392059f, 0.389618f, 0.387207f, 0.384827f, 0.382477f, 0.380127f, 0.377808f, 0.375488f, 0.37323f, 0.370972f,
        0.368713f, 0.366516f, 0.364319f, 0.362122f, 0.359985f, 0.357849f, 0.355713f, 0.353607f, 0.351532f, 0.349457f,
        0.347412f, 0.345398f, 0.343384f, 0.34137f, 0.339417f, 0.337463f, 0.33551f, 0.333588f, 0.331665f, 0.329773f,
        0.327911f, 0.32605f, 0.324188f, 0.322357f, 0.320557f, 0.318756f, 0.316986f, 0.315216f, 0.313446f, 0.311707f,
        0.309998f, 0.308289f, 0.30658f, 0.304901f, 0.303223f, 0.301575f, 0.299927f, 0.298309f, 0.296692f, 0.295074f,
        0.293488f, 0.291931f, 0.290375f, 0.288818f, 0.287262f, 0.285736f, 0.284241f, 0.282715f, 0.28125f, 0.279755f,
        0.27829f, 0.276825f, 0.275391f, 0.273956f, 0.272552f, 0.271118f, 0.269745f, 0.268341f, 0.266968f, 0.265594f,
        0.264252f, 0.262909f, 0.261566f, 0.260223f, 0.258911f, 0.257599f, 0.256317f, 0.255035f, 0.25375f
    };

    inline double saturate(double input) noexcept {
        double x1 = std::fabs(input + 0.95);
        double x2 = std::fabs(input - 0.95);
        return 0.5 * (x1 - x2);
    }

    inline void snapToZero(double& n) noexcept {
        if (!(n < -1.0e-8 || n > 1.0e-8)) n = 0;
    }
};

} // namespace ugens
} // namespace subcollider

#endif // SUBCOLLIDER_UGENS_STILSONMOOGLADDER_H
