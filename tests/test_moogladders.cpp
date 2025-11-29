/**
 * @file test_moogladders.cpp
 * @brief Unit tests for Moog Ladder filter UGens
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/StilsonMoogLadder.h>
#include <subcollider/ugens/MicrotrackerMoogLadder.h>
#include <subcollider/ugens/KrajeskiMoogLadder.h>
#include <subcollider/ugens/MusicDSPMoogLadder.h>
#include <subcollider/ugens/OberheimMoogLadder.h>
#include <subcollider/ugens/ImprovedMoogLadder.h>
#include <subcollider/ugens/RKSimulationMoogLadder.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_moogladders() {
    int failures = 0;

    // Test StilsonMoogLadder
    std::cout << "  StilsonMoogLadder:" << std::endl;
    {
        StilsonMoogLadder filter;
        filter.init(48000.0f);
        TEST("StilsonMoogLadder init: sample rate is set", filter.sampleRate == 48000.0f);
        TEST("StilsonMoogLadder init: default cutoff is 1000", filter.cutoff == 1000.0f);
        TEST("StilsonMoogLadder init: default resonance is 0.1", std::abs(filter.resonance - 0.1f) < 0.01f);
    }
    {
        StilsonMoogLadder filter;
        filter.init(48000.0f);
        filter.setCutoff(2000.0f);
        TEST("StilsonMoogLadder setCutoff: cutoff is updated", filter.cutoff == 2000.0f);
        filter.setResonance(0.5f);
        TEST("StilsonMoogLadder setResonance: resonance is updated", std::abs(filter.resonance - 0.5f) < 0.01f);
    }
    {
        StilsonMoogLadder filter;
        filter.init(48000.0f);
        Sample buffer[64];
        for (int i = 0; i < 64; ++i) buffer[i] = (i % 2 == 0) ? 1.0f : -1.0f;
        filter.process(buffer, 64);

        bool allValid = true;
        for (int i = 0; i < 64; ++i) {
            if (std::isnan(buffer[i]) || std::isinf(buffer[i])) {
                allValid = false;
                break;
            }
        }
        TEST("StilsonMoogLadder process: no NaN or Inf in output", allValid);
    }
    {
        StilsonMoogLadder filter;
        filter.init(48000.0f);
        for (int i = 0; i < 100; ++i) filter.tick(0.5f);
        filter.reset();
        Sample out = filter.tick(0.0f);
        TEST("StilsonMoogLadder reset: filter state resets", std::abs(out) < 0.1f);
    }

    // Test MicrotrackerMoogLadder
    std::cout << "  MicrotrackerMoogLadder:" << std::endl;
    {
        MicrotrackerMoogLadder filter;
        filter.init(48000.0f);
        TEST("MicrotrackerMoogLadder init: sample rate is set", filter.sampleRate == 48000.0f);
        TEST("MicrotrackerMoogLadder init: default cutoff is 1000", filter.cutoff == 1000.0f);
        TEST("MicrotrackerMoogLadder init: default resonance is 0.1", std::abs(filter.resonance - 0.1f) < 0.01f);
    }
    {
        MicrotrackerMoogLadder filter;
        filter.init(48000.0f);
        filter.setCutoff(2000.0f);
        TEST("MicrotrackerMoogLadder setCutoff: cutoff is updated", filter.cutoff == 2000.0f);
        filter.setResonance(0.5f);
        TEST("MicrotrackerMoogLadder setResonance: resonance is updated", std::abs(filter.resonance - 0.5f) < 0.01f);
    }
    {
        MicrotrackerMoogLadder filter;
        filter.init(48000.0f);
        Sample buffer[64];
        for (int i = 0; i < 64; ++i) buffer[i] = (i % 2 == 0) ? 1.0f : -1.0f;
        filter.process(buffer, 64);

        bool allValid = true;
        for (int i = 0; i < 64; ++i) {
            if (std::isnan(buffer[i]) || std::isinf(buffer[i])) {
                allValid = false;
                break;
            }
        }
        TEST("MicrotrackerMoogLadder process: no NaN or Inf in output", allValid);
    }
    {
        MicrotrackerMoogLadder filter;
        filter.init(48000.0f);
        for (int i = 0; i < 100; ++i) filter.tick(0.5f);
        filter.reset();
        Sample out = filter.tick(0.0f);
        TEST("MicrotrackerMoogLadder reset: filter state resets", std::abs(out) < 0.1f);
    }

    // Test KrajeskiMoogLadder
    std::cout << "  KrajeskiMoogLadder:" << std::endl;
    {
        KrajeskiMoogLadder filter;
        filter.init(48000.0f);
        TEST("KrajeskiMoogLadder init: sample rate is set", filter.sampleRate == 48000.0f);
        TEST("KrajeskiMoogLadder init: default cutoff is 1000", filter.cutoff == 1000.0f);
        TEST("KrajeskiMoogLadder init: default resonance is 0.1", std::abs(filter.resonance - 0.1f) < 0.01f);
    }
    {
        KrajeskiMoogLadder filter;
        filter.init(48000.0f);
        filter.setCutoff(2000.0f);
        TEST("KrajeskiMoogLadder setCutoff: cutoff is updated", filter.cutoff == 2000.0f);
        filter.setResonance(0.5f);
        TEST("KrajeskiMoogLadder setResonance: resonance is updated", std::abs(filter.resonance - 0.5f) < 0.01f);
    }
    {
        KrajeskiMoogLadder filter;
        filter.init(48000.0f);
        Sample buffer[64];
        for (int i = 0; i < 64; ++i) buffer[i] = (i % 2 == 0) ? 1.0f : -1.0f;
        filter.process(buffer, 64);

        bool allValid = true;
        for (int i = 0; i < 64; ++i) {
            if (std::isnan(buffer[i]) || std::isinf(buffer[i])) {
                allValid = false;
                break;
            }
        }
        TEST("KrajeskiMoogLadder process: no NaN or Inf in output", allValid);
    }
    {
        KrajeskiMoogLadder filter;
        filter.init(48000.0f);
        for (int i = 0; i < 100; ++i) filter.tick(0.5f);
        filter.reset();
        Sample out = filter.tick(0.0f);
        TEST("KrajeskiMoogLadder reset: filter state resets", std::abs(out) < 0.1f);
    }

    // Test MusicDSPMoogLadder
    std::cout << "  MusicDSPMoogLadder:" << std::endl;
    {
        MusicDSPMoogLadder filter;
        filter.init(48000.0f);
        TEST("MusicDSPMoogLadder init: sample rate is set", filter.sampleRate == 48000.0f);
        TEST("MusicDSPMoogLadder init: default cutoff is 1000", filter.cutoff == 1000.0f);
        TEST("MusicDSPMoogLadder init: default resonance is 0.1", std::abs(filter.resonance - 0.1f) < 0.01f);
    }
    {
        MusicDSPMoogLadder filter;
        filter.init(48000.0f);
        filter.setCutoff(2000.0f);
        TEST("MusicDSPMoogLadder setCutoff: cutoff is updated", filter.cutoff == 2000.0f);
        filter.setResonance(0.5f);
        TEST("MusicDSPMoogLadder setResonance: resonance is updated", std::abs(filter.resonance - 0.5f) < 0.01f);
    }
    {
        MusicDSPMoogLadder filter;
        filter.init(48000.0f);
        Sample buffer[64];
        for (int i = 0; i < 64; ++i) buffer[i] = (i % 2 == 0) ? 1.0f : -1.0f;
        filter.process(buffer, 64);

        bool allValid = true;
        for (int i = 0; i < 64; ++i) {
            if (std::isnan(buffer[i]) || std::isinf(buffer[i])) {
                allValid = false;
                break;
            }
        }
        TEST("MusicDSPMoogLadder process: no NaN or Inf in output", allValid);
    }
    {
        MusicDSPMoogLadder filter;
        filter.init(48000.0f);
        for (int i = 0; i < 100; ++i) filter.tick(0.5f);
        filter.reset();
        Sample out = filter.tick(0.0f);
        TEST("MusicDSPMoogLadder reset: filter state resets", std::abs(out) < 0.1f);
    }

    // Test OberheimMoogLadder
    std::cout << "  OberheimMoogLadder:" << std::endl;
    {
        OberheimMoogLadder filter;
        filter.init(48000.0f);
        TEST("OberheimMoogLadder init: sample rate is set", filter.sampleRate == 48000.0f);
        TEST("OberheimMoogLadder init: default cutoff is 1000", filter.cutoff == 1000.0f);
        TEST("OberheimMoogLadder init: default resonance is 0.1", std::abs(filter.resonance - 0.1f) < 0.01f);
    }
    {
        OberheimMoogLadder filter;
        filter.init(48000.0f);
        filter.setCutoff(2000.0f);
        TEST("OberheimMoogLadder setCutoff: cutoff is updated", filter.cutoff == 2000.0f);
        filter.setResonance(0.5f);
        TEST("OberheimMoogLadder setResonance: resonance is updated", std::abs(filter.resonance - 0.5f) < 0.01f);
    }
    {
        OberheimMoogLadder filter;
        filter.init(48000.0f);
        Sample buffer[64];
        for (int i = 0; i < 64; ++i) buffer[i] = (i % 2 == 0) ? 1.0f : -1.0f;
        filter.process(buffer, 64);

        bool allValid = true;
        for (int i = 0; i < 64; ++i) {
            if (std::isnan(buffer[i]) || std::isinf(buffer[i])) {
                allValid = false;
                break;
            }
        }
        TEST("OberheimMoogLadder process: no NaN or Inf in output", allValid);
    }
    {
        OberheimMoogLadder filter;
        filter.init(48000.0f);
        for (int i = 0; i < 100; ++i) filter.tick(0.5f);
        filter.reset();
        Sample out = filter.tick(0.0f);
        TEST("OberheimMoogLadder reset: filter state resets", std::abs(out) < 0.1f);
    }

    // Test ImprovedMoogLadder
    std::cout << "  ImprovedMoogLadder:" << std::endl;
    {
        ImprovedMoogLadder filter;
        filter.init(48000.0f);
        TEST("ImprovedMoogLadder init: sample rate is set", filter.sampleRate == 48000.0f);
        TEST("ImprovedMoogLadder init: default cutoff is 1000", filter.cutoff == 1000.0f);
        TEST("ImprovedMoogLadder init: default resonance is 0.1", std::abs(filter.resonance - 0.1f) < 0.01f);
    }
    {
        ImprovedMoogLadder filter;
        filter.init(48000.0f);
        filter.setCutoff(2000.0f);
        TEST("ImprovedMoogLadder setCutoff: cutoff is updated", filter.cutoff == 2000.0f);
        filter.setResonance(0.5f);
        TEST("ImprovedMoogLadder setResonance: resonance is updated", std::abs(filter.resonance - 0.5f) < 0.01f);
    }
    {
        ImprovedMoogLadder filter;
        filter.init(48000.0f);
        Sample buffer[64];
        for (int i = 0; i < 64; ++i) buffer[i] = (i % 2 == 0) ? 1.0f : -1.0f;
        filter.process(buffer, 64);

        bool allValid = true;
        for (int i = 0; i < 64; ++i) {
            if (std::isnan(buffer[i]) || std::isinf(buffer[i])) {
                allValid = false;
                break;
            }
        }
        TEST("ImprovedMoogLadder process: no NaN or Inf in output", allValid);
    }
    {
        ImprovedMoogLadder filter;
        filter.init(48000.0f);
        for (int i = 0; i < 100; ++i) filter.tick(0.5f);
        filter.reset();
        Sample out = filter.tick(0.0f);
        TEST("ImprovedMoogLadder reset: filter state resets", std::abs(out) < 0.1f);
    }

    // Test RKSimulationMoogLadder
    std::cout << "  RKSimulationMoogLadder:" << std::endl;
    {
        RKSimulationMoogLadder filter;
        filter.init(48000.0f);
        TEST("RKSimulationMoogLadder init: sample rate is set", filter.sampleRate == 48000.0f);
        TEST("RKSimulationMoogLadder init: default cutoff is 1000", filter.cutoff == 1000.0f);
        TEST("RKSimulationMoogLadder init: default resonance is 0.1", std::abs(filter.resonance - 0.1f) < 0.01f);
    }
    {
        RKSimulationMoogLadder filter;
        filter.init(48000.0f);
        filter.setCutoff(2000.0f);
        TEST("RKSimulationMoogLadder setCutoff: cutoff is updated", filter.cutoff == 2000.0f);
        filter.setResonance(0.5f);
        TEST("RKSimulationMoogLadder setResonance: resonance is updated", std::abs(filter.resonance - 0.5f) < 0.01f);
    }
    {
        RKSimulationMoogLadder filter;
        filter.init(48000.0f);
        Sample buffer[64];
        for (int i = 0; i < 64; ++i) buffer[i] = (i % 2 == 0) ? 1.0f : -1.0f;
        filter.process(buffer, 64);

        bool allValid = true;
        for (int i = 0; i < 64; ++i) {
            if (std::isnan(buffer[i]) || std::isinf(buffer[i])) {
                allValid = false;
                break;
            }
        }
        TEST("RKSimulationMoogLadder process: no NaN or Inf in output", allValid);
    }
    {
        RKSimulationMoogLadder filter;
        filter.init(48000.0f);
        for (int i = 0; i < 100; ++i) filter.tick(0.5f);
        filter.reset();
        Sample out = filter.tick(0.0f);
        TEST("RKSimulationMoogLadder reset: filter state resets", std::abs(out) < 0.1f);
    }

    return failures;
}
