/**
 * @file fverb/FVerb.cpp
 * @brief FVerb implementation wrapping Faust DSP.
 */

#include "../FVerb.h"
#include "FVerbDSP.h"
#include <cmath>
#include <cstring>

namespace subcollider {
namespace ugens {

void FVerb::init(Sample sr) noexcept {
    sampleRate = sr;

    // Clean up existing DSP if reinitializing
    if (dsp) {
        delete dsp;
        dsp = nullptr;
    }

    // Initialize the Faust DSP with the sample rate
    dsp = new FVerbDSP();
    dsp->init(static_cast<int>(sr));

    // Set default parameters
    setPredelay(150.0f);
    setDecay(82.0f);
    setTailDensity(80.0f);
    setInputDiffusion1(70.0f);
    setInputDiffusion2(75.0f);
}

void FVerb::allocateBuffers(size_t numSamples) noexcept {
    if (allocatedBlockSize >= numSamples) {
        return;  // Already allocated large enough
    }

    // Free old buffers
    freeBuffers();

    // Allocate new buffers
    inputBuffers = new Sample*[2];
    outputBuffers = new Sample*[2];

    for (int i = 0; i < 2; ++i) {
        inputBuffers[i] = new Sample[numSamples];
        outputBuffers[i] = new Sample[numSamples];
        std::memset(inputBuffers[i], 0, numSamples * sizeof(Sample));
        std::memset(outputBuffers[i], 0, numSamples * sizeof(Sample));
    }

    allocatedBlockSize = numSamples;
}

void FVerb::freeBuffers() noexcept {
    if (inputBuffers) {
        for (int i = 0; i < 2; ++i) {
            if (inputBuffers[i]) {
                delete[] inputBuffers[i];
                inputBuffers[i] = nullptr;
            }
        }
        delete[] inputBuffers;
        inputBuffers = nullptr;
    }

    if (outputBuffers) {
        for (int i = 0; i < 2; ++i) {
            if (outputBuffers[i]) {
                delete[] outputBuffers[i];
                outputBuffers[i] = nullptr;
            }
        }
        delete[] outputBuffers;
        outputBuffers = nullptr;
    }

    allocatedBlockSize = 0;
}

void FVerb::process(Sample* left, Sample* right, size_t numSamples) noexcept {
    if (!dsp) {
        return;  // Not initialized
    }

    // Ensure buffers are allocated
    allocateBuffers(numSamples);

    // Copy input to internal buffers
    std::memcpy(inputBuffers[0], left, numSamples * sizeof(Sample));
    std::memcpy(inputBuffers[1], right, numSamples * sizeof(Sample));

    // Process through Faust DSP
    dsp->compute(static_cast<int>(numSamples), inputBuffers, outputBuffers);

    // Copy output back to caller's buffers
    std::memcpy(left, outputBuffers[0], numSamples * sizeof(Sample));
    std::memcpy(right, outputBuffers[1], numSamples * sizeof(Sample));
}

void FVerb::setPredelay(Sample ms) noexcept {
    if (dsp) {
        Sample clamped = ms < 0.0f ? 0.0f : (ms > 300.0f ? 300.0f : ms);
        dsp->SetParamValue(FVerbDSP::PREDELAY, clamped);
    }
}

void FVerb::setInputAmount(Sample amount) noexcept {
    if (dsp) {
        Sample clamped = amount < 0.0f ? 0.0f : (amount > 100.0f ? 100.0f : amount);
        dsp->SetParamValue(FVerbDSP::INPUT_AMOUNT, clamped);
    }
}

void FVerb::setInputLowPassCutoff(Sample hz) noexcept {
    if (dsp) {
        Sample clamped = hz < 1.0f ? 1.0f : (hz > 20000.0f ? 20000.0f : hz);
        dsp->SetParamValue(FVerbDSP::INPUT_LOW_PASS_CUTOFF, clamped);
    }
}

void FVerb::setInputHighPassCutoff(Sample hz) noexcept {
    if (dsp) {
        Sample clamped = hz < 1.0f ? 1.0f : (hz > 1000.0f ? 1000.0f : hz);
        dsp->SetParamValue(FVerbDSP::INPUT_HIGH_PASS_CUTOFF, clamped);
    }
}

void FVerb::setInputDiffusion1(Sample amount) noexcept {
    if (dsp) {
        Sample clamped = amount < 0.0f ? 0.0f : (amount > 100.0f ? 100.0f : amount);
        dsp->SetParamValue(FVerbDSP::INPUT_DIFFUSION_1, clamped);
    }
}

void FVerb::setInputDiffusion2(Sample amount) noexcept {
    if (dsp) {
        Sample clamped = amount < 0.0f ? 0.0f : (amount > 100.0f ? 100.0f : amount);
        dsp->SetParamValue(FVerbDSP::INPUT_DIFFUSION_2, clamped);
    }
}

void FVerb::setTailDensity(Sample amount) noexcept {
    if (dsp) {
        Sample clamped = amount < 0.0f ? 0.0f : (amount > 100.0f ? 100.0f : amount);
        dsp->SetParamValue(FVerbDSP::TAIL_DENSITY, clamped);
    }
}

void FVerb::setDecay(Sample amount) noexcept {
    if (dsp) {
        Sample clamped = amount < 0.0f ? 0.0f : (amount > 100.0f ? 100.0f : amount);
        dsp->SetParamValue(FVerbDSP::DECAY, clamped);
    }
}

void FVerb::setDamping(Sample hz) noexcept {
    if (dsp) {
        Sample clamped = hz < 10.0f ? 10.0f : (hz > 20000.0f ? 20000.0f : hz);
        dsp->SetParamValue(FVerbDSP::DAMPING, clamped);
    }
}

void FVerb::setModulatorFrequency(Sample hz) noexcept {
    if (dsp) {
        Sample clamped = hz < 0.01f ? 0.01f : (hz > 4.0f ? 4.0f : hz);
        dsp->SetParamValue(FVerbDSP::MODULATOR_FREQUENCY, clamped);
    }
}

void FVerb::setModulatorDepth(Sample ms) noexcept {
    if (dsp) {
        Sample clamped = ms < 0.0f ? 0.0f : (ms > 10.0f ? 10.0f : ms);
        dsp->SetParamValue(FVerbDSP::MODULATOR_DEPTH, clamped);
    }
}

void FVerb::reset() noexcept {
    if (dsp) {
        dsp->instanceClear();
    }
}

FVerb::~FVerb() {
    freeBuffers();
    if (dsp) {
        delete dsp;
        dsp = nullptr;
    }
}

} // namespace ugens
} // namespace subcollider
