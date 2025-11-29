# SubCollider

[![CI](https://github.com/schollz/subcollider/actions/workflows/CI.yml/badge.svg)](https://github.com/schollz/subcollider/actions/workflows/CI.yml)

A lightweight, embedded-friendly C++ DSP engine for audio synthesis.

## Features

- **Header-only library** - Easy integration, no linking required
- **Static UGen structs** - No virtual calls, predictable performance
- **Per-sample inline processing** - Optimized for CPU cache efficiency
- **Block-based execution** - Efficient batch processing of audio buffers
- **No heap in audio thread** - ISR-safe, suitable for real-time embedded systems
- **Control-rate parameter updates** - Smooth, glitch-free parameter changes

## UGens

The following unit generators (UGens) are included:

### SinOsc
Sine wave oscillator using phase accumulator.

```cpp
#include <subcollider/ugens/SinOsc.h>

subcollider::ugens::SinOsc osc;
osc.init(48000.0f);         // Initialize at 48kHz sample rate
osc.setFrequency(440.0f);   // Set frequency to 440 Hz

// Per-sample processing
float sample = osc.tick();

// Block processing
float buffer[64];
osc.process(buffer, 64);
```

### EnvelopeAR
Attack-release envelope generator with exponential curves.

```cpp
#include <subcollider/ugens/EnvelopeAR.h>

subcollider::ugens::EnvelopeAR env;
env.init(48000.0f);
env.setAttack(0.01f);   // 10ms attack
env.setRelease(0.5f);   // 500ms release

env.trigger();          // Start envelope

// Per-sample processing
float amplitude = env.tick();

// Apply to audio
audioSample *= amplitude;
```

### LFNoise2
Quadratically interpolated low-frequency noise generator.

```cpp
#include <subcollider/ugens/LFNoise2.h>

subcollider::ugens::LFNoise2 noise;
noise.init(48000.0f);
noise.setFrequency(4.0f);   // 4 Hz rate of change

// Get modulation value
float mod = noise.tick();   // Returns value in [-1, 1]
```

## Example Voice

The `ExampleVoice` demonstrates combining multiple UGens:

```cpp
#include <subcollider/ExampleVoice.h>

subcollider::ExampleVoice voice;
voice.init(48000.0f);
voice.setFrequency(440.0f);
voice.setAttack(0.01f);
voice.setRelease(0.3f);
voice.setVibratoDepth(0.1f);
voice.setVibratoRate(5.0f);
voice.setAmplitude(0.5f);

voice.trigger();    // Note on

float buffer[64];
voice.process(buffer, 64);

voice.release();    // Note off
```

## Audio Loop

ISR-safe audio processing loop with double buffering:

```cpp
#include <subcollider/AudioLoop.h>

subcollider::AudioLoop<64> loop;
loop.init(48000.0f);

// In audio interrupt/callback:
float* processingBuffer = loop.getProcessingBuffer();
loop.clearProcessingBuffer();
myVoice.process(processingBuffer, 64);
loop.swapBuffers();

// For output (e.g., DMA):
const float* outputBuffer = loop.getOutputBuffer();
```

## Building

### Requirements

- CMake 3.14 or later
- C++17 compatible compiler
- JACK Audio (optional, for JACK example)

### Build Commands

```bash
# Configure
cmake -B build -DSUBCOLLIDER_BUILD_TESTS=ON -DSUBCOLLIDER_BUILD_EXAMPLES=ON

# Build
cmake --build build

# Run tests
cd build && ctest --output-on-failure

# Run basic example
./build/example_basic
```

### Build with JACK support

```bash
# Install JACK development files (Ubuntu/Debian)
sudo apt-get install libjack-jackd2-dev

# Configure with JACK
cmake -B build -DSUBCOLLIDER_BUILD_JACK_EXAMPLE=ON

# Build
cmake --build build

# Run JACK example (requires JACK server running)
./build/example_jack
```

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `SUBCOLLIDER_BUILD_TESTS` | ON | Build unit tests |
| `SUBCOLLIDER_BUILD_EXAMPLES` | ON | Build example applications |
| `SUBCOLLIDER_BUILD_JACK_EXAMPLE` | OFF | Build JACK audio example |

## Integration

### Header-only usage

Simply copy the `include/` directory to your project and add it to your include path:

```cmake
target_include_directories(myproject PRIVATE path/to/subcollider/include)
```

### As CMake subdirectory

```cmake
add_subdirectory(subcollider)
target_link_libraries(myproject PRIVATE subcollider)
```

## Project Structure

```
subcollider/
├── include/
│   ├── subcollider.h              # Main include header
│   └── subcollider/
│       ├── types.h                # Core types and utilities
│       ├── AudioBuffer.h          # Fixed-size audio buffer
│       ├── AudioLoop.h            # ISR-safe audio loop
│       ├── ExampleVoice.h         # Example synthesizer voice
│       └── ugens/
│           ├── SinOsc.h           # Sine oscillator
│           ├── EnvelopeAR.h       # Attack-release envelope
│           └── LFNoise2.h         # Interpolated noise
├── examples/
│   ├── basic_example.cpp          # Basic usage example
│   └── jack_example.cpp           # JACK audio example
├── tests/
│   └── *.cpp                      # Unit tests
├── CMakeLists.txt
└── README.md
```

## Design Principles

1. **No virtual functions** - All UGens are concrete structs with inline processing
2. **No heap allocation** - All buffers are stack-allocated or statically sized
3. **Control-rate updates** - Parameters are updated once per block, not per sample
4. **ISR-safe** - Audio processing code can run in interrupt context
5. **Modular** - Each UGen is self-contained and can be used independently

## License

MIT License