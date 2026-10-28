#include "AudioMetrics.h"
#include "Signals.h"
#include "dsp/PitchCorrector.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using namespace pitchpilot;

namespace
{
std::vector<float> render (std::vector<float> signal, double sampleRate, const dsp::EngineParameters& parameters,
                           int blockSize, int* latency = nullptr)
{
    dsp::PitchCorrector engine;
    engine.prepare (sampleRate, blockSize, 1);
    engine.setParameters (parameters);
    for (size_t offset = 0; offset < signal.size(); offset += static_cast<size_t> (blockSize))
    {
        float* channels[] = { signal.data() + offset };
        engine.process (channels, 1, static_cast<int> (std::min (static_cast<size_t> (blockSize), signal.size() - offset)));
    }
    if (latency)
        *latency = engine.latencySamples();
    return signal;
}
} // namespace

TEST_CASE ("Click score ignores smooth audio and finds a discontinuity")
{
    const double sampleRate = 48000.0;
    const auto clean = test::sine (220.0, sampleRate, 0.5);

    const auto same = eval::clickScores (clean, clean, sampleRate, 0);
    CHECK (same.maxScore < 1.1f);
    CHECK (same.clicks.empty());

    // A 0.05 step in the output at 0.3 s.
    auto popped = clean;
    for (size_t i = 14400; i < popped.size(); ++i)
        popped[i] += 0.05f;
    const auto report = eval::clickScores (clean, popped, sampleRate, 0);
    INFO ("score " << report.maxScore << " at " << report.maxSeconds << " s");
    REQUIRE (report.clicks.size() == 1);
    CHECK (report.clicks.front().score > 4.0f);
    CHECK (std::abs (report.clicks.front().seconds - 0.3) < 1.0e-3);
}

TEST_CASE ("Vocal-like input is click-free through the whole engine")
{
    // The regression net steady sines lacked: a changing ratio on modulated
    // input with onsets, rests and consonants.
    const double sampleRate = GENERATE (44100.0, 48000.0);
    const float speedMs = GENERATE (1.0f, 25.0f);
    // Custom with only A allowed forces shifts of up to ±6 semitones.
    const auto scale = GENERATE (dsp::Scale::Major, dsp::Scale::MinorPentatonic, dsp::Scale::Custom);
    const int blockSize = GENERATE (64, 512);

    dsp::EngineParameters parameters;
    parameters.rootNote = 9; // A
    parameters.scale = scale;
    parameters.speedMs = speedMs;
    if (scale == dsp::Scale::Custom)
        parameters.enabledNotes = 1 << 9;

    const auto input = test::vocalPhrase (sampleRate);
    int latency = 0;
    const auto output = render (input, sampleRate, parameters, blockSize, &latency);
    const auto report = eval::clickScores (input, output, sampleRate, latency);

    INFO ("rate " << sampleRate << ", speed " << speedMs << " ms, scale " << dsp::scaleName (scale) << ", block "
                  << blockSize << ": worst click score " << report.maxScore << " at " << report.maxSeconds << " s");
    CHECK (report.clicks.empty());
}

TEST_CASE ("The engine pulls a vocal-like phrase onto the scale")
{
    const double sampleRate = 48000.0;
    dsp::EngineParameters parameters;
    parameters.rootNote = 9;
    parameters.scale = dsp::Scale::NaturalMinor;
    parameters.speedMs = 5.0f;

    const auto input = test::vocalPhrase (sampleRate);
    const auto output = render (input, sampleRate, parameters, 256);
    const auto mask = dsp::scaleMask (parameters.rootNote, parameters.scale);
    const auto before = eval::pitchError (input, sampleRate, mask, 80.0f, 800.0f);
    const auto after = eval::pitchError (output, sampleRate, mask, 80.0f, 800.0f);

    INFO ("median error " << before.medianAbsCents << " → " << after.medianAbsCents << " cents, voiced "
                          << before.voicedFraction << " → " << after.voicedFraction);
    CHECK (after.medianAbsCents < 0.5 * before.medianAbsCents);
    CHECK (after.voicedFraction > 0.8 * before.voicedFraction);
}
