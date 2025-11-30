/**
 * @file test_dbamp.cpp
 * @brief Unit tests for DBAmp UGen.
 */

#include <iostream>
#include <cmath>
#include <subcollider/ugens/DBAmp.h>

using namespace subcollider;
using namespace subcollider::ugens;

#define TEST(name, condition) \
    if (!(condition)) { \
        std::cout << "FAIL: " << name << std::endl; \
        failures++; \
    } else { \
        std::cout << "PASS: " << name << std::endl; \
    }

int test_dbamp() {
    int failures = 0;
    DBAmp dbAmp;

    // 0 dB should be unity
    TEST("DBAmp 0 dB = 1.0", std::abs(dbAmp.process(0.0f) - 1.0f) < 1e-6f);

    // +20 dB should be 10x
    TEST("DBAmp +20 dB = 10.0", std::abs(dbAmp.process(20.0f) - 10.0f) < 1e-5f);

    // -6 dB should be ~0.501
    TEST("DBAmp -6 dB ~0.501", std::abs(dbAmp.process(-6.0f) - 0.501187f) < 1e-6f);

    // Buffer processing
    {
        Sample input[4] = {0.0f, -6.0f, -12.0f, 6.0f};
        Sample output[4] = {};
        dbAmp.process(input, output, 4);

        bool matches = true;
        for (int i = 0; i < 4; ++i) {
            Sample expected = dbAmp.process(input[i]);
            if (std::abs(expected - output[i]) > 1e-6f) {
                matches = false;
                break;
            }
        }
        TEST("DBAmp buffer processing matches scalar", matches);
    }

    return failures;
}
