/**
 * @file test_main.cpp
 * @brief Simple test framework main entry point.
 *
 * This is a minimal test framework that doesn't require external dependencies.
 */

#include <iostream>
#include <cstdlib>

// Forward declarations of test functions
int test_types();
int test_sinosc();
int test_sawdpw();
int test_lftri();
int test_envelopear();
int test_envelopeadsr();
int test_lfnoise2();
int test_pan2();
int test_balance2();
int test_audioloop();
int test_examplevoice();
int test_moogladders();
int test_xline();
int test_phasor();
int test_supersaw();
int test_xfade2();
int test_wrap();
int test_dbamp();
int test_downsampler();
int test_buffer();
int test_bufferallocator();
int test_bufrd();
int test_bufrd_phasor();
int test_playback_oversampling();
int test_fverb();
int test_combc();
int test_tape();
int test_onepolelpf();
int test_dcblock();
int test_laglinear();
int test_linlin();

int main() {
    int failures = 0;

    std::cout << "=== SubCollider Unit Tests ===" << std::endl << std::endl;

    std::cout << "--- Types Tests ---" << std::endl;
    failures += test_types();

    std::cout << "--- SinOsc Tests ---" << std::endl;
    failures += test_sinosc();

    std::cout << "--- SawDPW Tests ---" << std::endl;
    failures += test_sawdpw();

    std::cout << "--- LFTri Tests ---" << std::endl;
    failures += test_lftri();

    std::cout << "--- EnvelopeAR Tests ---" << std::endl;
    failures += test_envelopear();

    std::cout << "--- EnvelopeADSR Tests ---" << std::endl;
    failures += test_envelopeadsr();

    std::cout << "--- LFNoise2 Tests ---" << std::endl;
    failures += test_lfnoise2();

    std::cout << "--- Pan2 Tests ---" << std::endl;
    failures += test_pan2();

    std::cout << "--- Balance2 Tests ---" << std::endl;
    failures += test_balance2();

    std::cout << "--- AudioLoop Tests ---" << std::endl;
    failures += test_audioloop();

    std::cout << "--- ExampleVoice Tests ---" << std::endl;
    failures += test_examplevoice();

    std::cout << "--- MoogLadder Tests ---" << std::endl;
    failures += test_moogladders();

    std::cout << "--- XLine Tests ---" << std::endl;
    failures += test_xline();

    std::cout << "--- Phasor Tests ---" << std::endl;
    failures += test_phasor();

    std::cout << "--- SuperSaw Tests ---" << std::endl;
    failures += test_supersaw();

    std::cout << "--- XFade2 Tests ---" << std::endl;
    failures += test_xfade2();

    std::cout << "--- Wrap Tests ---" << std::endl;
    failures += test_wrap();

    std::cout << "--- DBAmp Tests ---" << std::endl;
    failures += test_dbamp();

    std::cout << "--- Downsampler Tests ---" << std::endl;
    failures += test_downsampler();

    std::cout << "--- Buffer Tests ---" << std::endl;
    failures += test_buffer();

    std::cout << "--- BufferAllocator Tests ---" << std::endl;
    failures += test_bufferallocator();

    std::cout << "--- BufRd Tests ---" << std::endl;
    failures += test_bufrd();

    std::cout << "--- BufRd + Phasor Tests ---" << std::endl;
    failures += test_bufrd_phasor();

    std::cout << "--- Playback Oversampling Tests ---" << std::endl;
    failures += test_playback_oversampling();

    std::cout << "--- FVerb Tests ---" << std::endl;
    failures += test_fverb();

    std::cout << "--- CombC Tests ---" << std::endl;
    failures += test_combc();

    std::cout << "--- Tape Tests ---" << std::endl;
    failures += test_tape();

    std::cout << "--- DCBlock Tests ---" << std::endl;
    failures += test_dcblock();

    std::cout << "--- OnePoleLPF Tests ---" << std::endl;
    failures += test_onepolelpf();

    std::cout << "--- LagLinear Tests ---" << std::endl;
    failures += test_laglinear();

    std::cout << "--- LinLin Tests ---" << std::endl;
    failures += test_linlin();

    std::cout << std::endl;
    if (failures == 0) {
        std::cout << "All tests passed!" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << failures << " test(s) failed!" << std::endl;
        return EXIT_FAILURE;
    }
}
