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
    CHECK (latency == 516); // lowest note 50 Hz at 48 kHz: about 10.8 ms

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
    // During a splice the fading tap must never read samples that haven't
    // been written yet.
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

TEST_CASE ("Ratio jumps during a splice stay click-free")
{
    // The ratio jumps up and down while splices fade, so the fading tap's
    // headroom is used up faster than the splice point planned for.
    const double sampleRate = GENERATE (44100.0, 48000.0);
    const double semitones = GENERATE (1.0, 3.0, 7.0, 12.0);
    const int hold = GENERATE (37, 101, 263, 1009);
    const double input = 220.0;
    const double ratio = dsp::semitonesToRatio (semitones);

    auto signal = test::sine (input, sampleRate, 1.0);
    std::vector<float> ratios (signal.size());
    for (size_t i = 0; i < ratios.size(); ++i)
        ratios[i] = (i / static_cast<size_t> (hold)) % 2 ? static_cast<float> (ratio) : 1.0f;
    dsp::PitchShifter shifter;
    shifter.prepare (sampleRate, 1);
    shifter.setPeriodHint (static_cast<float> (sampleRate / input));
    float* channels[] = { signal.data() };
    shifter.process (channels, 1, static_cast<int> (signal.size()), ratios.data());

    const double maxSlope = 2.0 * std::numbers::pi * input * ratio * 0.5 / sampleRate;
    float maxStep = 0.0f;
    for (size_t i = 2400; i < signal.size(); ++i)
        maxStep = std::max (maxStep, std::abs (signal[i] - signal[i - 1]));
    INFO ("rate " << sampleRate << ", shift " << semitones << " st every " << hold << " samples, largest step " << maxStep
                  << ", sine slope " << maxSlope);
    CHECK (maxStep < 1.5 * maxSlope);
}

TEST_CASE ("The delay stays within latencySpreadSamples() of the latency")
{
    // An impulse every 3000 samples: each output impulse is an input impulse
    // read back with the delay the tap had at that moment.
    const double sampleRate = GENERATE (44100.0, 48000.0);
    const double semitones = GENERATE (-12.0, -5.0, 1.0, 5.0, 12.0);
    const float periodHint = GENERATE (0.0f, 218.0f);
    const bool jumping = GENERATE (false, true);
    constexpr size_t spacing = 3000;

    std::vector<float> signal (static_cast<size_t> (sampleRate * 2.0), 0.0f);
    for (size_t i = 0; i < signal.size(); i += spacing)
        signal[i] = 1.0f;
    std::vector<float> ratios (signal.size());
    for (size_t i = 0; i < ratios.size(); ++i)
        ratios[i] = static_cast<float> (jumping && (i / 37) % 2 ? 1.0 / dsp::semitonesToRatio (semitones)
                                                                : dsp::semitonesToRatio (semitones));

    dsp::PitchShifter shifter;
    shifter.prepare (sampleRate, 1);
    shifter.setPeriodHint (periodHint);
    const int latency = shifter.latencySamples();
    const int spread = shifter.latencySpreadSamples();
    REQUIRE (spread > 0);
    REQUIRE (static_cast<size_t> (latency + spread + 2) < spacing);
    float* channels[] = { signal.data() };
    shifter.process (channels, 1, static_cast<int> (signal.size()), ratios.data());

    int worst = 0;
    for (size_t t = spacing; t < signal.size(); ++t)
        if (std::abs (signal[t]) > 1.0e-4f)
        {
            // The impulse it came from is the latest one at least 1 sample earlier.
            const auto source = (t - 1) / spacing * spacing;
            worst = std::max (worst, std::abs (static_cast<int> (t - source) - latency));
        }
    INFO ("rate " << sampleRate << ", shift " << semitones << " st, hint " << periodHint << (jumping ? ", jumping" : "")
                  << ": latency " << latency << ", spread " << spread << ", worst offset " << worst);
    CHECK (worst <= spread + 1); // +1: linear interpolation spreads an impulse over two samples
}

TEST_CASE ("Splices without a period hint stay in phase on periodic input")
{
    // When the detector has no pitch, the splice still has to land on a
    // matching part of the waveform, or the crossfade mixes out-of-phase
    // copies and the level dips.
    const double sampleRate = GENERATE (44100.0, 48000.0);
    const double semitones = GENERATE (-3.0, -1.0, 1.0, 3.0);
    const double input = 220.0;

    const double ratio = dsp::semitonesToRatio (semitones);

    const auto out = shift (test::sine (input, sampleRate, 1.0), sampleRate, static_cast<float> (ratio), 0.0f);

    // Demodulate at the output frequency over one output period. Within a
    // splice that isn't in phase, the window averages two phases, so the
    // amplitude drops; an in-phase splice keeps it at 0.5.
    const double w = 2.0 * std::numbers::pi * input * ratio / sampleRate;
    const auto period = static_cast<size_t> (std::lround (2.0 * std::numbers::pi / w));
    double minAmplitude = 1.0;
    for (size_t start = 2400; start + period < out.size(); start += 4)
    {
        double re = 0.0, im = 0.0;
        for (size_t i = start; i < start + period; ++i)
        {
            re += out[i] * std::cos (w * static_cast<double> (i));
            im += out[i] * std::sin (w * static_cast<double> (i));
        }
        minAmplitude = std::min (minAmplitude, 2.0 * std::sqrt (re * re + im * im) / static_cast<double> (period));
    }
    const double dipDb = 20.0 * std::log10 (minAmplitude / 0.5);
    INFO ("rate " << sampleRate << ", shift " << semitones << " st: deepest dip " << dipDb << " dB");
    CHECK (dipDb > -0.5);
}
