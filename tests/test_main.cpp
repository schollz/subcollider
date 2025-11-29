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
int test_lfnoise2();
int test_pan2();
int test_audioloop();
int test_examplevoice();
int test_moogladders();

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

    std::cout << "--- LFNoise2 Tests ---" << std::endl;
    failures += test_lfnoise2();

    std::cout << "--- Pan2 Tests ---" << std::endl;
    failures += test_pan2();

    std::cout << "--- AudioLoop Tests ---" << std::endl;
    failures += test_audioloop();

    std::cout << "--- ExampleVoice Tests ---" << std::endl;
    failures += test_examplevoice();

    std::cout << "--- MoogLadder Tests ---" << std::endl;
    failures += test_moogladders();

    std::cout << std::endl;
    if (failures == 0) {
        std::cout << "All tests passed!" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << failures << " test(s) failed!" << std::endl;
        return EXIT_FAILURE;
    }
}
