#include <iostream>
#include <cmath>
#include <subcollider/ugens/LagLinear.h>

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

int test_laglinear() {
    int failures = 0;

    // --- Initialization ---
    {
        LagLinear lag;
        lag.init(48000.0f, 0.25f, 0.05f);
        TEST("LagLinear init sets sample rate", lag.sampleRate == 48000.0f);
        TEST("LagLinear init sets initial value", approx(lag.currentValue, 0.25f));
        TEST("LagLinear init sets target value", approx(lag.targetValue, 0.25f));
        TEST("LagLinear init sets time", approx(lag.timeSeconds, 0.05f));
    }

    // --- Linear ramp hits target in expected time ---
    {
        LagLinear lag;
        lag.init(1000.0f, 0.0f, 1.0f); // 1 second, 1000 samples

        Sample first = lag.tick(1.0f);
        TEST("LagLinear ramp starts moving toward target", first > 0.0f && first < 0.01f);

        Sample mid = 0.0f;
        for (int i = 1; i < 500; ++i) {
            mid = lag.tick(1.0f);
        }
        TEST("LagLinear halfway around 0.5", mid > 0.45f && mid < 0.55f);

        Sample last = mid;
        for (int i = 500; i < 1000; ++i) {
            last = lag.tick(1.0f);
        }
        TEST("LagLinear reaches target", approx(last, 1.0f, 0.001f));
    }

    // --- New target resets ramp from current position ---
    {
        LagLinear lag;
        lag.init(1000.0f, 0.0f, 1.0f);
        Sample val = 0.0f;
        for (int i = 0; i < 250; ++i) { // ramp toward 1
            val = lag.tick(1.0f);
        }
        TEST("LagLinear mid-ramp is positive", val > 0.2f);

        // New target halfway through: ramp from current to -1 over 1s
        Sample afterRetarget = lag.tick(-1.0f);
        TEST("LagLinear retarget changes direction", afterRetarget < val);

        Sample end = afterRetarget;
        for (int i = 1; i < 1000; ++i) {
            end = lag.tick(-1.0f);
        }
        TEST("LagLinear reaches new target", approx(end, -1.0f, 0.001f));
    }

    // --- Zero time snaps immediately ---
    {
        LagLinear lag;
        lag.init(48000.0f, 0.0f, 0.0f);
        Sample v1 = lag.tick(5.0f);
        Sample v2 = lag.tick(2.0f);
        TEST("LagLinear zero time jumps to first target", approx(v1, 5.0f));
        TEST("LagLinear zero time jumps to new target", approx(v2, 2.0f));
    }

    // --- Block processing uses per-sample targets ---
    {
        const size_t n = 200;
        Sample input[n];
        Sample output[n];

        for (size_t i = 0; i < n; ++i) {
            input[i] = i < 100 ? 0.0f : 1.0f;
        }

        LagLinear lag;
        lag.init(1000.0f, 0.0f, 0.1f); // 100 ms -> 100 samples
        lag.process(input, output, n);

        TEST("LagLinear block start at 0", approx(output[0], 0.0f));
        TEST("LagLinear block mid-rise near half", output[150] > 0.4f && output[150] < 0.7f);
        TEST("LagLinear block end near 1", approx(output[n - 1], 1.0f, 0.01f));
    }

    return failures;
}
