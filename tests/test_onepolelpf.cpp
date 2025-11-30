#include <iostream>
#include <cmath>
#include <numeric>
#include <cstdlib>
#include <subcollider/ugens/OnePoleLPF.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

inline bool approx(Sample a, Sample b, Sample epsilon = 0.0001f) {
    return std::abs(a - b) < epsilon;
}

int test_onepolelpf() {
    int failures = 0;
    const size_t numSamples = 64;

    Sample input[numSamples];
    Sample cutoff[numSamples];
    Sample output[numSamples];

    // --- High cutoff passes signal ---
    {
        OnePoleLPF lpf;
        lpf.init();

        std::srand(0);
        for (size_t i = 0; i < numSamples; ++i) {
            input[i] = (static_cast<Sample>(std::rand()) / RAND_MAX) * 2.0f - 1.0f;
            cutoff[i] = 20000.0f; // High cutoff
        }
        lpf.process(input, cutoff, output, numSamples);

        Sample maxError = 0.0f;
        for (size_t i = 8; i < numSamples; ++i) { // allow a short settle time
            maxError = std::max(maxError, std::abs(output[i] - input[i]));
        }
        TEST("OnePoleLPF High cutoff passes signal", maxError < 0.2f);
    }

    // --- Low cutoff filters signal ---
    {
        OnePoleLPF lpf;
        lpf.init();

        for (size_t i = 0; i < numSamples; ++i) {
            input[i] = i < 32 ? 0.0f : 1.0f; // step function
            cutoff[i] = 10.0f;              // Low cutoff
        }
        lpf.process(input, cutoff, output, numSamples);
        TEST("OnePoleLPF Low cutoff starts slowly",
             output[32] > 0.0f && output[32] < 0.05f);
        TEST("OnePoleLPF Low cutoff rises gradually",
             output[63] > output[32] && output[63] < 0.2f);
    }

    // --- DC convergence ---
    {
        const size_t longBlock = 1024;
        Sample longInput[longBlock];
        Sample longCutoff[longBlock];
        Sample longOutput[longBlock];

        OnePoleLPF lpf;
        lpf.init();

        for (size_t i = 0; i < longBlock; ++i) {
            longInput[i] = 0.5f;
            longCutoff[i] = 100.0f;
        }
        lpf.process(longInput, longCutoff, longOutput, longBlock);
        TEST("OnePoleLPF DC convergence", approx(longOutput[longBlock - 1], 0.5f, 0.01f));
    }

    return failures;
}
