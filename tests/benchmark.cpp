/**
 * @file benchmark.cpp
 * @brief Benchmarks for SubCollider UGens.
 *
 * Measures tick() performance as instances per 1 sample at 44.1 kHz.
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>

#include <subcollider/ugens/SinOsc.h>
#include <subcollider/ugens/SawDPW.h>
#include <subcollider/ugens/LFTri.h>
#include <subcollider/ugens/EnvelopeAR.h>
#include <subcollider/ugens/EnvelopeADSR.h>
#include <subcollider/ugens/LFNoise2.h>
#include <subcollider/ugens/Pan2.h>
#include <subcollider/ugens/XLine.h>
#include <subcollider/ugens/Phasor.h>
#include <subcollider/ugens/SuperSaw.h>
#include <subcollider/ugens/StilsonMoogLadder.h>
#include <subcollider/ugens/MicrotrackerMoogLadder.h>
#include <subcollider/ugens/KrajeskiMoogLadder.h>
#include <subcollider/ugens/MusicDSPMoogLadder.h>
#include <subcollider/ugens/OberheimMoogLadder.h>
#include <subcollider/ugens/ImprovedMoogLadder.h>
#include <subcollider/ugens/RKSimulationMoogLadder.h>

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
    // Calculate how many instances can be processed in one block (512 samples at 44.1 kHz)
    // Time for one block = 512 / 44100 seconds
    // Number of instances = ticksPerSec * (512 / 44100)
    static constexpr double BLOCK_SIZE = 512.0;
    static constexpr double SAMPLE_RATE = 44100.0;
    double instancesPerBlock = ticksPerSec * (BLOCK_SIZE / SAMPLE_RATE);
    std::cout << std::left << std::setw(14) << name
              << std::fixed << std::setprecision(2) << instancesPerBlock
              << " instances/block (512 samples @ 44.1kHz)" << std::endl;
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
 * @brief Benchmark LFTri tick().
 */
void benchmarkLFTri() {
    LFTri tri;
    tri.init(48000.0f);
    tri.setFrequency(440.0f);

    // Warmup
    volatile Sample sink = 0.0f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        sink = tri.tick();
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        sink = tri.tick();
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("LFTri", ticksPerSec);
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
 * @brief Benchmark EnvelopeADSR tick().
 */
void benchmarkEnvelopeADSR() {
    EnvelopeADSR env;
    env.init(48000.0f);
    env.setAttack(0.01f);
    env.setDecay(0.1f);
    env.setSustain(0.7f);
    env.setRelease(0.3f);
    env.gate(1.0f);

    // Warmup
    volatile Sample sink = 0.0f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        sink = env.tick();
        // Retrigger if envelope becomes idle
        if (!env.isActive()) {
            env.gate(1.0f);
        }
    }

    // Benchmark - retrigger envelope when idle to keep it active
    env.gate(1.0f);
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        sink = env.tick();
        // Retrigger if envelope becomes idle
        if (!env.isActive()) {
            env.gate(1.0f);
        }
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("EnvelopeADSR", ticksPerSec);
    (void)sink;
}

/**
 * @brief Benchmark SuperSaw tick().
 */
void benchmarkSuperSaw() {
    SuperSaw supersaw;
    supersaw.init(48000.0f);
    supersaw.setFrequency(440.0f);
    supersaw.setAttack(0.01f);
    supersaw.setDecay(0.1f);
    supersaw.setSustain(0.7f);
    supersaw.setRelease(0.3f);
    supersaw.setDetune(0.2f);
    supersaw.setSpread(0.6f);
    supersaw.gate(1.0f);

    // Warmup
    Stereo result(0.0f, 0.0f);
    volatile Sample sinkL = 0.0f;
    volatile Sample sinkR = 0.0f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        result = supersaw.tick();
        sinkL = result.left;
        sinkR = result.right;
        // Retrigger if envelope becomes idle
        if (!supersaw.isActive()) {
            supersaw.gate(1.0f);
        }
    }

    // Benchmark - retrigger when idle to keep it active
    supersaw.gate(1.0f);
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        result = supersaw.tick();
        sinkL = result.left;
        sinkR = result.right;
        // Retrigger if envelope becomes idle
        if (!supersaw.isActive()) {
            supersaw.gate(1.0f);
        }
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("SuperSaw", ticksPerSec);
    (void)sinkL;
    (void)sinkR;
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

/**
 * @brief Benchmark Phasor tick().
 */
void benchmarkPhasor() {
    Phasor phasor;
    phasor.init(48000.0f);
    phasor.set(1.0f, 0.0f, 1000000.0f);  // Large end to avoid wrapping during benchmark

    // Warmup
    volatile Sample sink = 0.0f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        sink = phasor.tick();
    }

    // Benchmark
    phasor.reset();
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        sink = phasor.tick();
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("Phasor", ticksPerSec);
    (void)sink;
}

/**
 * @brief Benchmark StilsonMoogLadder tick().
 */
void benchmarkStilsonMoogLadder() {
    StilsonMoogLadder filter;
    filter.init(48000.0f);
    filter.setCutoff(1000.0f);
    filter.setResonance(0.4f);

    // Warmup
    volatile Sample sink = 0.0f;
    Sample input = 0.5f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        sink = filter.tick(input);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        sink = filter.tick(input);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("StilsonMoog", ticksPerSec);
    (void)sink;
}

/**
 * @brief Benchmark MicrotrackerMoogLadder tick().
 */
void benchmarkMicrotrackerMoogLadder() {
    MicrotrackerMoogLadder filter;
    filter.init(48000.0f);
    filter.setCutoff(1000.0f);
    filter.setResonance(0.4f);

    // Warmup
    volatile Sample sink = 0.0f;
    Sample input = 0.5f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        sink = filter.tick(input);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        sink = filter.tick(input);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("MicrotrkMoog", ticksPerSec);
    (void)sink;
}

/**
 * @brief Benchmark KrajeskiMoogLadder tick().
 */
void benchmarkKrajeskiMoogLadder() {
    KrajeskiMoogLadder filter;
    filter.init(48000.0f);
    filter.setCutoff(1000.0f);
    filter.setResonance(0.4f);

    // Warmup
    volatile Sample sink = 0.0f;
    Sample input = 0.5f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        sink = filter.tick(input);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        sink = filter.tick(input);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("KrajeskiMoog", ticksPerSec);
    (void)sink;
}

/**
 * @brief Benchmark MusicDSPMoogLadder tick().
 */
void benchmarkMusicDSPMoogLadder() {
    MusicDSPMoogLadder filter;
    filter.init(48000.0f);
    filter.setCutoff(1000.0f);
    filter.setResonance(0.4f);

    // Warmup
    volatile Sample sink = 0.0f;
    Sample input = 0.5f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        sink = filter.tick(input);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        sink = filter.tick(input);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("MusicDSPMoog", ticksPerSec);
    (void)sink;
}

/**
 * @brief Benchmark OberheimMoogLadder tick().
 */
void benchmarkOberheimMoogLadder() {
    OberheimMoogLadder filter;
    filter.init(48000.0f);
    filter.setCutoff(1000.0f);
    filter.setResonance(0.4f);

    // Warmup
    volatile Sample sink = 0.0f;
    Sample input = 0.5f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        sink = filter.tick(input);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        sink = filter.tick(input);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("OberheimMoog", ticksPerSec);
    (void)sink;
}

/**
 * @brief Benchmark ImprovedMoogLadder tick().
 */
void benchmarkImprovedMoogLadder() {
    ImprovedMoogLadder filter;
    filter.init(48000.0f);
    filter.setCutoff(1000.0f);
    filter.setResonance(0.4f);

    // Warmup
    volatile Sample sink = 0.0f;
    Sample input = 0.5f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        sink = filter.tick(input);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        sink = filter.tick(input);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("ImprovedMoog", ticksPerSec);
    (void)sink;
}

/**
 * @brief Benchmark RKSimulationMoogLadder tick().
 */
void benchmarkRKSimulationMoogLadder() {
    RKSimulationMoogLadder filter;
    filter.init(48000.0f);
    filter.setCutoff(1000.0f);
    filter.setResonance(0.4f);

    // Warmup
    volatile Sample sink = 0.0f;
    Sample input = 0.5f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        sink = filter.tick(input);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        sink = filter.tick(input);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("RKSimulMoog", ticksPerSec);
    (void)sink;
}

/**
 * @brief Benchmark RKSimulationMoogLadder tick() with 2x oversampling.
 */
void benchmarkRKSimulationMoogLadder2x() {
    RKSimulationMoogLadder filter;
    filter.init(48000.0f);
    filter.setOversampleFactor(2);  // 2x oversampling
    filter.setCutoff(1000.0f);
    filter.setResonance(0.4f);

    // Warmup
    volatile Sample sink = 0.0f;
    Sample input = 0.5f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        sink = filter.tick(input);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        sink = filter.tick(input);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("RKSimulMoog2x", ticksPerSec);
    (void)sink;
}

int main() {
    std::cout << "=== SubCollider UGen Benchmarks ===" << std::endl;
    std::cout << std::endl;

    benchmarkSinOsc();
    benchmarkSawDPW();
    benchmarkLFTri();
    benchmarkEnvelopeAR();
    benchmarkEnvelopeADSR();
    benchmarkSuperSaw();
    benchmarkLFNoise2();
    benchmarkPan2();
    benchmarkXLine();
    benchmarkPhasor();
    benchmarkStilsonMoogLadder();
    benchmarkMicrotrackerMoogLadder();
    benchmarkKrajeskiMoogLadder();
    benchmarkMusicDSPMoogLadder();
    benchmarkOberheimMoogLadder();
    benchmarkImprovedMoogLadder();
    benchmarkRKSimulationMoogLadder();
    benchmarkRKSimulationMoogLadder2x();

    std::cout << std::endl;

    return 0;
}
