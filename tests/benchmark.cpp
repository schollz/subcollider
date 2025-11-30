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
#include <subcollider/ugens/XFade2.h>
#include <subcollider/ugens/XLine.h>
#include <subcollider/ugens/Phasor.h>
#include <subcollider/ugens/SuperSaw.h>
#include <subcollider/ugens/CombC.h>
#include <subcollider/ugens/LagLinear.h>
#include <subcollider/ugens/StilsonMoogLadder.h>
#include <subcollider/ugens/MicrotrackerMoogLadder.h>
#include <subcollider/ugens/KrajeskiMoogLadder.h>
#include <subcollider/ugens/MusicDSPMoogLadder.h>
#include <subcollider/ugens/OberheimMoogLadder.h>
#include <subcollider/ugens/ImprovedMoogLadder.h>
#include <subcollider/ugens/RKSimulationMoogLadder.h>
#include <subcollider/ugens/RLPF.h>
#include <subcollider/ugens/OnePoleLPF.h>
#include <subcollider/ugens/LinLin.h>

using namespace subcollider;
using namespace subcollider::ugens;

// Number of iterations for benchmarking
static constexpr int WARMUP_ITERATIONS = 10000;
static constexpr int BENCHMARK_ITERATIONS = 1000000;
static constexpr int PARAM_CHANGE_BLOCK_SIZE = 64;

float cutoffForBlock(int blockIndex) {
    static constexpr float cutoffCycle[] = {600.0f, 1200.0f, 2400.0f, 4800.0f};
    return cutoffCycle[blockIndex % (sizeof(cutoffCycle) / sizeof(float))];
}

float resonanceForBlock(int blockIndex) {
    static constexpr float resonanceCycle[] = {0.2f, 0.35f, 0.5f, 0.65f};
    return resonanceCycle[blockIndex % (sizeof(resonanceCycle) / sizeof(float))];
}

float delayTimeForBlock(int blockIndex) {
    static constexpr float delayCycle[] = {0.03f, 0.06f, 0.09f, 0.12f};
    return delayCycle[blockIndex % (sizeof(delayCycle) / sizeof(float))];
}

float decayTimeForBlock(int blockIndex) {
    static constexpr float decayCycle[] = {0.8f, 1.2f, 1.8f, 2.4f};
    return decayCycle[blockIndex % (sizeof(decayCycle) / sizeof(float))];
}

template <typename LadderFilter>
inline void updateLadderParams(LadderFilter& filter, int sampleIndex) {
    if (sampleIndex % PARAM_CHANGE_BLOCK_SIZE == 0) {
        int blockIndex = sampleIndex / PARAM_CHANGE_BLOCK_SIZE;
        filter.setCutoff(cutoffForBlock(blockIndex));
        filter.setResonance(resonanceForBlock(blockIndex));
    }
}

inline void updateRLPFParams(RLPF& filter, int sampleIndex) {
    if (sampleIndex % PARAM_CHANGE_BLOCK_SIZE == 0) {
        int blockIndex = sampleIndex / PARAM_CHANGE_BLOCK_SIZE;
        filter.setFreq(cutoffForBlock(blockIndex));
        filter.setResonance(resonanceForBlock(blockIndex));
    }
}

inline void updateCombParams(CombC& filter, int sampleIndex) {
    if (sampleIndex % PARAM_CHANGE_BLOCK_SIZE == 0) {
        int blockIndex = sampleIndex / PARAM_CHANGE_BLOCK_SIZE;
        filter.setDelayTime(delayTimeForBlock(blockIndex));
        filter.setDecayTime(decayTimeForBlock(blockIndex));
    }
}

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
 * @brief Benchmark XFade2 tick().
 */
void benchmarkXFade2() {
    XFade2 xfade;
    static constexpr Sample posCycle[] = {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f};
    constexpr int cycleLength = sizeof(posCycle) / sizeof(Sample);

    Sample inA = 0.5f;
    Sample inB = -0.5f;
    volatile Sample sink = 0.0f;

    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        if (i % PARAM_CHANGE_BLOCK_SIZE == 0) {
            int blockIndex = (i / PARAM_CHANGE_BLOCK_SIZE) % cycleLength;
            xfade.setPosition(posCycle[blockIndex]);
        }
        sink = xfade.tick(inA, inB);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        if (i % PARAM_CHANGE_BLOCK_SIZE == 0) {
            int blockIndex = (i / PARAM_CHANGE_BLOCK_SIZE) % cycleLength;
            xfade.setPosition(posCycle[blockIndex]);
        }
        sink = xfade.tick(inA, inB);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("XFade2", ticksPerSec);
    (void)sink;
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
 * @brief Benchmark LagLinear tick().
 */
void benchmarkLagLinear() {
    LagLinear lag;
    lag.init(48000.0f, 0.0f, 0.02f); // 20 ms ramps

    auto targetForSample = [](int sampleIndex) {
        static constexpr Sample targets[] = {0.0f, 1.0f, -0.5f, 0.5f};
        int blockIndex = sampleIndex / PARAM_CHANGE_BLOCK_SIZE;
        return targets[blockIndex % (sizeof(targets) / sizeof(Sample))];
    };

    // Warmup
    volatile Sample sink = 0.0f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        sink = lag.tick(targetForSample(i));
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        sink = lag.tick(targetForSample(i));
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("LagLinear", ticksPerSec);
    (void)sink;
}

/**
 * @brief Benchmark LinLin mapping.
 */
void benchmarkLinLin() {
    static constexpr Sample srcLo = -1.0f;
    static constexpr Sample srcHi = 1.0f;
    static constexpr Sample destLo = 0.0f;
    static constexpr Sample destHi = 5.0f;

    volatile Sample sink = 0.0f;

    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        Sample input = static_cast<Sample>(i % 200 - 100) / 100.0f; // cycle through [-1, 1]
        sink = LinLin(input, srcLo, srcHi, destLo, destHi);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        Sample input = static_cast<Sample>(i % 200 - 100) / 100.0f;
        sink = LinLin(input, srcLo, srcHi, destLo, destHi);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("LinLin", ticksPerSec);
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
        updateLadderParams(filter, i);
        sink = filter.tick(input);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        updateLadderParams(filter, i);
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
        updateLadderParams(filter, i);
        sink = filter.tick(input);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        updateLadderParams(filter, i);
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
        updateLadderParams(filter, i);
        sink = filter.tick(input);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        updateLadderParams(filter, i);
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
        updateLadderParams(filter, i);
        sink = filter.tick(input);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        updateLadderParams(filter, i);
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
        updateLadderParams(filter, i);
        sink = filter.tick(input);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        updateLadderParams(filter, i);
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
        updateLadderParams(filter, i);
        sink = filter.tick(input);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        updateLadderParams(filter, i);
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
        updateLadderParams(filter, i);
        sink = filter.tick(input);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        updateLadderParams(filter, i);
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
        updateLadderParams(filter, i);
        sink = filter.tick(input);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        updateLadderParams(filter, i);
        sink = filter.tick(input);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("RKSimulMoog2x", ticksPerSec);
    (void)sink;
}
/**
 * @brief Benchmark RLPF tick().
 */
void benchmarkRLPF() {
    RLPF filter;
    filter.init(48000.0f);
    filter.setFreq(1000.0f);
    filter.setResonance(0.707f);

    // Warmup
    volatile Sample sink = 0.0f;
    Sample input = 0.5f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        updateRLPFParams(filter, i);
        sink = filter.tick(input);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        updateRLPFParams(filter, i);
        sink = filter.tick(input);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("RLPF", ticksPerSec);
    (void)sink;
}

/**
 * @brief Benchmark CombC tick().
 */
void benchmarkCombC() {
    CombC comb;
    comb.init(48000.0f, 1.0f);
    comb.setDelayTime(0.1f);
    comb.setDecayTime(2.0f);

    // Warmup
    volatile Sample sink = 0.0f;
    Sample input = 0.5f;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        updateCombParams(comb, i);
        sink = comb.tick(input);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        updateCombParams(comb, i);
        sink = comb.tick(input);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = BENCHMARK_ITERATIONS / seconds;

    printResult("CombC", ticksPerSec);
    (void)sink;
}

void benchmarkOnePoleLPF() {
    const int n = 64;
    OnePoleLPF lpf;
    lpf.init();
    Sample input[n];
    Sample output[n];
    for (int i = 0; i < n; ++i) {
        input[i] = 0.5f;
    }

    constexpr int cutoffVariants = 4;
    Sample cutoffBlocks[cutoffVariants][n];
    for (int variant = 0; variant < cutoffVariants; ++variant) {
        Sample cutoffValue = cutoffForBlock(variant);
        for (int i = 0; i < n; ++i) {
            cutoffBlocks[variant][i] = cutoffValue;
        }
    }

    auto cutoffBlockForIndex = [&](int blockIndex) -> const Sample* {
        return cutoffBlocks[blockIndex % cutoffVariants];
    };

    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        lpf.process(input, cutoffBlockForIndex(i), output, n);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        lpf.process(input, cutoffBlockForIndex(i), output, n);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double seconds = duration.count() / 1e9;
    double ticksPerSec = (double)BENCHMARK_ITERATIONS * n / seconds;

    printResult("OnePoleLPF", ticksPerSec);
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
    benchmarkXFade2();
    benchmarkXLine();
    benchmarkLagLinear();
    benchmarkLinLin();
    benchmarkPhasor();
    benchmarkCombC();
    benchmarkStilsonMoogLadder();
    benchmarkMicrotrackerMoogLadder();
    benchmarkKrajeskiMoogLadder();
    benchmarkMusicDSPMoogLadder();
    benchmarkOberheimMoogLadder();
    benchmarkImprovedMoogLadder();
    benchmarkRKSimulationMoogLadder();
    benchmarkRKSimulationMoogLadder2x();
    benchmarkRLPF();
    benchmarkOnePoleLPF();

    std::cout << std::endl;

    return 0;
}
