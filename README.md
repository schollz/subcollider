# SubCollider

[![CI](https://github.com/schollz/subcollider/actions/workflows/CI.yml/badge.svg)](https://github.com/schollz/subcollider/actions/workflows/CI.yml)

A lightweight, embedded-friendly C++ DSP engine for audio synthesis.

## Quick start

Run tests:

```bash 
make run-tests
```

Run JACK example (requires JACK server running):

```bash
make jack-run
```


## Features

- **Header-only library** - Easy integration, no linking required
- **Static UGen structs** - No virtual calls, predictable performance
- **Per-sample inline processing** - Optimized for CPU cache efficiency
- **Block-based execution** - Efficient batch processing of audio buffers
- **No heap in audio thread** - ISR-safe, suitable for real-time embedded systems
- **Control-rate parameter updates** - Smooth, glitch-free parameter changes

## UGens

The following unit generators (UGens) are included:

- `SinOsc` - Sine wave oscillator
- `EnvelopeAR` - Attack-release envelope generator
- `LFNoise2` - Interpolated low-frequency noise generator
- `Pan2` - Stereo panner


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

| Option                           | Default | Description                |
| -------------------------------- | ------- | -------------------------- |
| `SUBCOLLIDER_BUILD_TESTS`        | ON      | Build unit tests           |
| `SUBCOLLIDER_BUILD_EXAMPLES`     | ON      | Build example applications |
| `SUBCOLLIDER_BUILD_JACK_EXAMPLE` | OFF     | Build JACK audio example   |

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

## Design Principles

1. **No virtual functions** - All UGens are concrete structs with inline processing
2. **No heap allocation** - All buffers are stack-allocated or statically sized
3. **Control-rate updates** - Parameters are updated once per block, not per sample
4. **ISR-safe** - Audio processing code can run in interrupt context
5. **Modular** - Each UGen is self-contained and can be used independently

## License

MIT License