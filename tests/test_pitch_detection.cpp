#include "Signals.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <random>

using namespace pitchpilot;

TEST_CASE ("YIN detects sine pitch within 3 cents across sample rates")
{
    const double sampleRate = GENERATE (44100.0, 48000.0, 88200.0, 96000.0);
    const double hz = GENERATE (82.41, 110.0, 220.0, 329.63, 523.25, 780.0);

    const auto detected = test::medianDetectedHz (test::sine (hz, sampleRate, 0.5), sampleRate);
    INFO ("rate " << sampleRate << " Hz, input " << hz << " Hz, detected " << detected << " Hz");
    REQUIRE (detected > 0.0);
    CHECK (std::abs (test::centsBetween (detected, hz)) < 3.0);
}

TEST_CASE ("YIN avoids octave errors on harmonic-rich input")
{
    const double hz = GENERATE (98.0, 146.83, 261.63, 440.0);
    const auto detected = test::medianDetectedHz (test::sawtooth (hz, 48000.0, 0.5), 48000.0);
    INFO ("input " << hz << " Hz, detected " << detected << " Hz");
    CHECK (std::abs (test::centsBetween (detected, hz)) < 5.0);
}

TEST_CASE ("Low and high voice ranges reach their extremes")
{
    CHECK (std::abs (test::centsBetween (test::medianDetectedHz (test::sine (55.0, 48000.0, 0.5), 48000.0, 0.15, 50.0f, 400.0f), 55.0)) < 3.0);
    CHECK (std::abs (test::centsBetween (test::medianDetectedHz (test::sine (1400.0, 48000.0, 0.5), 48000.0, 0.1, 150.0f, 1500.0f), 1400.0)) < 5.0);
}

TEST_CASE ("Silence and white noise are unvoiced")
{
    double voiced = 1.0;
    test::medianDetectedHz (std::vector<float> (48000, 0.0f), 48000.0, 0.1, 80.0f, 800.0f, &voiced);
    CHECK (voiced == 0.0);

    std::mt19937 rng (1234);
    std::uniform_real_distribution<float> dist (-0.5f, 0.5f);
    std::vector<float> noise (48000);
    for (auto& s : noise)
        s = dist (rng);
    test::medianDetectedHz (noise, 48000.0, 0.1, 80.0f, 800.0f, &voiced);
    CHECK (voiced < 0.1);
}
