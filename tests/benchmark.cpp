/**
 * @file benchmark.cpp
 * @brief Benchmarks for SubCollider UGens.
 *
 * Measures tick() performance for each UGen in ticks/sec.
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>

#include <subcollider/ugens/SinOsc.h>
#include <subcollider/ugens/SawDPW.h>
#include <subcollider/ugens/EnvelopeAR.h>
#include <subcollider/ugens/LFNoise2.h>
#include <subcollider/ugens/Pan2.h>
#include <subcollider/ugens/XLine.h>

using namespace subcollider;
using namespace subcollider::ugens;

// Number of iterations for benchmarking
static constexpr int WARMUP_ITERATIONS = 10000;
static constexpr int BENCHMARK_ITERATIONS = 1000000;

/**
 * @brief Print benchmark result in the required format.
 * @param name UGen name
 * @param ticksPerSec Measured ticks per second
 */
void printResult(const std::string& name, double ticksPerSec) {
    // Convert to millions and format as integer ticks/sec
    long long ticksPerSecInt = static_cast<long long>(ticksPerSec);
    std::cout << std::left << std::setw(12) << name 
              << ticksPerSecInt << " ticks/sec" << std::endl;
}

/**
 * @brief Benchmark SinOsc tick().
 */
void benchmarkSinOsc() {
    SinOsc osc;
    osc.init(48000.0f);
    osc.setFrequency(440.0f);

    // Warmup
    volatile Sample sink = 0.0f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        sink = osc.tick();
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        sink = osc.tick();
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("SinOsc", ticksPerSec);
    (void)sink; // Suppress unused warning
}

/**
 * @brief Benchmark SawDPW tick().
 */
void benchmarkSawDPW() {
    SawDPW saw;
    saw.init(48000.0f);
    saw.setFrequency(440.0f);

    // Warmup
    volatile Sample sink = 0.0f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        sink = saw.tick();
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        sink = saw.tick();
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("SawDPW", ticksPerSec);
    (void)sink;
}

/**
 * @brief Benchmark EnvelopeAR tick().
 */
void benchmarkEnvelopeAR() {
    EnvelopeAR env;
    env.init(48000.0f);
    env.setAttack(0.01f);
    env.setRelease(0.5f);
    env.trigger();

    // Warmup
    volatile Sample sink = 0.0f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        sink = env.tick();
        // Retrigger if envelope becomes idle
        if (!env.isActive()) {
            env.trigger();
        }
    }

    // Benchmark - retrigger envelope when idle to keep it active
    env.trigger();
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        sink = env.tick();
        // Retrigger if envelope becomes idle
        if (!env.isActive()) {
            env.trigger();
        }
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("EnvelopeAR", ticksPerSec);
    (void)sink;
}

/**
 * @brief Benchmark LFNoise2 tick().
 */
void benchmarkLFNoise2() {
    LFNoise2 noise;
    noise.init(48000.0f);
    noise.setFrequency(4.0f);

    // Warmup
    volatile Sample sink = 0.0f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        sink = noise.tick();
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        sink = noise.tick();
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("LFNoise2", ticksPerSec);
    (void)sink;
}

/**
 * @brief Benchmark Pan2 tick().
 */
void benchmarkPan2() {
    Pan2 panner;
    panner.setPan(0.0f);

    // Warmup
    Stereo result(0.0f, 0.0f);
    volatile Sample sinkL = 0.0f;
    volatile Sample sinkR = 0.0f;
    Sample input = 0.5f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        result = panner.tick(input);
        sinkL = result.left;
        sinkR = result.right;
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        result = panner.tick(input);
        sinkL = result.left;
        sinkR = result.right;
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("Pan2", ticksPerSec);
    (void)sinkL;
    (void)sinkR;
}

/**
 * @brief Benchmark XLine tick().
 */
void benchmarkXLine() {
    XLine line;
    line.init(48000.0f);
    line.set(1.0f, 10.0f, 100.0f);  // Long duration to avoid retriggering

    // Warmup
    volatile Sample sink = 0.0f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        sink = line.tick();
    }

    // Benchmark
    line.reset();
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        sink = line.tick();
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("XLine", ticksPerSec);
    (void)sink;
}

int main() {
    std::cout << "=== SubCollider UGen Benchmarks ===" << std::endl;
    std::cout << std::endl;

    benchmarkSinOsc();
    benchmarkSawDPW();
    benchmarkEnvelopeAR();
    benchmarkLFNoise2();
    benchmarkPan2();
    benchmarkXLine();

    std::cout << std::endl;

    return 0;
}
