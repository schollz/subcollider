# Subcollider UGen Development Guide

This document outlines the architecture and design principles for creating Unit Generators (UGens) in the Subcollider synthesis engine. The goal is to provide a clear and consistent framework for extending the library with new audio processing modules.

There appear to be two primary designs for UGens in the codebase: a modern `struct`-based approach and a legacy class-based approach. For all new development, the **`struct`-based approach is strongly preferred**.

## Guiding Principles

- **Performance First:** UGens should be optimized for high-performance, real-time audio processing. This means avoiding heap allocations, virtual function calls, and other sources of overhead in the audio-processing path.
- **Value Semantics:** UGens are `struct`s that can be passed by value, enabling them to be used directly in arrays or other data structures without pointer indirection.
- **Explicit State:** All state is stored in member variables of the struct.
- **Control-Rate vs. Audio-Rate:** The design distinguishes between infrequent updates (control-rate, e.g., setting a filter's cutoff) and per-sample processing (audio-rate).

---

## Modern UGen Design (`struct`-based)

The modern UGen is a `struct` containing its state, an `init()` method for setup, methods for parameter setting, and one or more methods for processing audio (`tick()` for single samples, `process()` for blocks).

### Anatomy of a Modern UGen

```cpp
// include/subcollider/ugens/MyUgen.h
#pragma once

#include "../types.h" // For Sample, Stereo, etc.
#include "../Buffer.h" // If interacting with shared buffers

namespace subcollider {
namespace ugens {

struct MyUgen {
    // 1. STATE: All internal state is stored as public member variables.
    Sample m_state_variable;
    Sample m_parameter;
    Sample m_sample_rate;

    // 2. INITIALIZATION: A method to set the initial state.
    void init(Sample sampleRate) {
        m_sample_rate = sampleRate;
        m_state_variable = 0.0f;
        m_parameter = 1.0f;
    }

    // 3. PARAMETER SETTERS: Methods for control-rate updates.
    // These are called infrequently, not per-sample.
    void setParameter(Sample value) {
        m_parameter = value;
        // Optionally pre-calculate coefficients here
    }

    // 4. PROCESSING (AUDIO-RATE):
    
    /**
     * @brief Process a single sample. An input sample is passed as an argument.
     * @return The processed output sample.
     */
    inline Sample tick(Sample input) {
        // Perform DSP calculation
        Sample output = input * m_parameter + m_state_variable;
        
        // Update state
        m_state_variable = output;
        
        return output;
    }

    /**
     * @brief Process a block of samples from an input buffer to an output buffer.
     * This is for connecting UGens in a graph.
     */
    void process(const Sample* input, Sample* output, size_t numSamples) {
        for (size_t i = 0; i < numSamples; ++i) {
            output[i] = tick(input[i]);
        }
    }

    /**
     * @brief Process a block where the input is a control signal (e.g., a Phasor).
     */
    void process(Sample* output, const Sample* control_input, size_t numSamples) {
        for (size_t i = 0; i < numSamples; ++i) {
            // In this case, tick might take the control input
            // output[i] = tick(control_input[i]);
        }
    }
};

} // namespace ugens
} // namespace subcollider
```

### Key Components

1.  **State (`struct` members):** All variables needed to run the UGen (e.g., `phase`, filter history, parameter values) are public members of the struct. This allows for direct manipulation and introspection if needed.

2.  **`init(Sample sampleRate)`:** This method initializes the UGen. It **must** be called before processing. It takes the `sampleRate` as an argument to allow for calculations of frequency-dependent coefficients.

3.  **Parameter Setters (`set...()`):** Public methods that allow external code to update the UGen's parameters at the control rate (e.g., `setFrequency(float freq)`). To maintain performance, these methods may pre-calculate values (like filter coefficients) that are then used by the audio-rate `tick()` or `process()` methods.

4.  **Processing (`tick()` and `process()`):**
    *   An `inline Sample tick(...)` method is used for the core, per-sample DSP logic. It should be as fast as possible. It can take input samples or control values as arguments.
    *   A `void process(...)` method is provided for block-based processing, which is the standard way of connecting UGens. It typically loops and calls `tick()`. There may be multiple overloads of `process` to handle different input types (e.g., audio + control inputs).

### Examples

-   **`SinOsc.h`**: A minimal UGen that generates a signal but has no audio input. Its `tick()` method takes no arguments.
-   **`BufRd.h`**: A UGen that takes a control-rate input (`phase`) to its `tick()` method. It reads from a shared `Buffer`.
-   **`FVerb.h`**: A more complex example showing separate `tick` logic for stereo inputs.

---

## Modulated Filter Example (`OnePoleLPF`)

`OnePoleLPF` shows a minimal filter that still follows the standard `init`/`tick`/`process` pattern while supporting per-sample modulation:

```cpp
struct OnePoleLPF {
    void init(Sample sr = DEFAULT_SAMPLE_RATE, Sample cutoffHz = 1000.0f) noexcept;
    void setCutoff(Sample cutoffHz) noexcept;

    inline Sample tick(Sample input) noexcept {
        // Uses the currently cached cutoff
        z = gain * input + pole * z;
        return z;
    }

    inline Sample tick(Sample input, Sample cutoffHz) noexcept {
        // Per-sample modulation path
        Sample localPole = std::exp(-TWO_PI * clamp(cutoffHz, 1.0f, sampleRate * 0.5f) / sampleRate);
        Sample localGain = 1.0f - localPole;
        z = localGain * input + localPole * z;
        return z;
    }

    void process(const Sample* in, const Sample* cutoffBuf, Sample* out, size_t n) noexcept {
        for (size_t i = 0; i < n; ++i) {
            out[i] = tick(in[i], cutoffBuf[i]);
        }
    }

    void process(Sample* samples, size_t n) noexcept {
        for (size_t i = 0; i < n; ++i) {
            samples[i] = tick(samples[i]);
        }
    }
};
```

The API mirrors other UGens, making it easy to swap into existing processing chains without special-case buffer plumbing.
