#include "Signals.h"
#include "dsp/PitchShifter.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using namespace pitchpilot;

namespace
{
std::vector<float> shift (std::vector<float> signal, double sampleRate, float ratio, float periodHint)
{
    dsp::PitchShifter shifter;
    shifter.prepare (sampleRate, 1);
    shifter.setPeriodHint (periodHint);
    std::vector<float> ratios (signal.size(), ratio);
    float* channels[] = { signal.data() };
    shifter.process (channels, 1, static_cast<int> (signal.size()), ratios.data());
    return signal;
}
} // namespace

TEST_CASE ("Shifter moves pitch by the requested ratio")
{
    const double sampleRate = GENERATE (44100.0, 48000.0, 96000.0);
    const double semitones = GENERATE (-7.0, -1.0, -0.3, 0.5, 2.0, 7.0);
    const double input = 220.0;
    const double expected = input * dsp::semitonesToRatio (semitones);

    const auto out = shift (test::sine (input, sampleRate, 1.0), sampleRate, static_cast<float> (dsp::semitonesToRatio (semitones)),
                          static_cast<float> (sampleRate / input));
    const auto detected = test::medianDetectedHz (out, sampleRate, 0.1);
    INFO ("rate " << sampleRate << ", shift " << semitones << " st, expected " << expected << " Hz, detected " << detected << " Hz");
    CHECK (std::abs (test::centsBetween (detected, expected)) < 5.0);
}

TEST_CASE ("Unity ratio is a pure delay of latencySamples()")
{
    const double sampleRate = 48000.0;
    dsp::PitchShifter shifter;
    shifter.prepare (sampleRate, 1);
    const int latency = shifter.latencySamples();
    CHECK (latency == 525); // lowest note 50 Hz at 48 kHz: about 10.9 ms

    auto signal = test::sine (330.0, sampleRate, 0.2);
    const auto original = signal;
    std::vector<float> ratios (signal.size(), 1.0f);
    float* channels[] = { signal.data() };
    shifter.process (channels, 1, static_cast<int> (signal.size()), ratios.data());

    float maxError = 0.0f;
    for (size_t i = static_cast<size_t> (latency); i < signal.size(); ++i)
        maxError = std::max (maxError, std::abs (signal[i] - original[i - static_cast<size_t> (latency)]));
    CHECK (maxError < 1.0e-5f);
}

TEST_CASE ("Large upward shifts stay click-free")
{
    // The splice trigger moves up for big ratios so the fading tap never
    // reads samples that haven't been written yet.
    const double sampleRate = 48000.0;
    const double semitones = GENERATE (3.0, 7.0, 12.0);
    const double input = 220.0;
    const double ratio = dsp::semitonesToRatio (semitones);

    const auto out = shift (test::sine (input, sampleRate, 1.0), sampleRate, static_cast<float> (ratio),
                            static_cast<float> (sampleRate / input));

    // A 0.5-amplitude sine at the output pitch changes by at most this per sample.
    const double maxSlope = 2.0 * std::numbers::pi * input * ratio * 0.5 / sampleRate;
    float maxStep = 0.0f;
    for (size_t i = 2400; i < out.size(); ++i)
        maxStep = std::max (maxStep, std::abs (out[i] - out[i - 1]));
    INFO ("shift " << semitones << " st, largest step " << maxStep << ", sine slope " << maxSlope);
    CHECK (maxStep < 1.5 * maxSlope);
}
