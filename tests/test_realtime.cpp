// End-to-end engine tests, including a check that process() never allocates.

#include "Signals.h"
#include "dsp/PitchCorrector.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <atomic>
#include <cstdlib>
#include <new>

namespace
{
std::atomic<bool> trackAllocations { false };
std::atomic<int> allocationCount { 0 };
} // namespace

void* operator new (std::size_t size)
{
    if (trackAllocations.load (std::memory_order_relaxed))
        allocationCount.fetch_add (1, std::memory_order_relaxed);
    if (void* p = std::malloc (size == 0 ? 1 : size))
        return p;
    throw std::bad_alloc();
}

void operator delete (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }

using namespace pitchpilot;

namespace
{
// Runs a mono signal through the engine in host-sized blocks and returns the output.
std::vector<float> runEngine (std::vector<float> signal, double sampleRate, int blockSize,
                              const dsp::EngineParameters& params, int* allocations = nullptr)
{
    dsp::PitchCorrector engine;
    engine.prepare (sampleRate, blockSize, 1);
    engine.setParameters (params);

    allocationCount = 0;
    trackAllocations = true;
    for (size_t pos = 0; pos < signal.size(); pos += static_cast<size_t> (blockSize))
    {
        float* channels[] = { signal.data() + pos };
        const auto n = std::min (static_cast<size_t> (blockSize), signal.size() - pos);
        engine.setParameters (params);
        engine.process (channels, 1, static_cast<int> (n));
    }
    trackAllocations = false;
    if (allocations)
        *allocations = allocationCount;
    return signal;
}
} // namespace

TEST_CASE ("Engine process() does not allocate")
{
    const int blockSize = GENERATE (32, 64, 128, 256, 1024);
    int allocations = -1;
    runEngine (test::sine (228.0, 48000.0, 0.5), 48000.0, blockSize, {}, &allocations);
    CHECK (allocations == 0);
}

TEST_CASE ("Engine corrects an off-key sine to the nearest scale note")
{
    const double sampleRate = GENERATE (44100.0, 48000.0, 96000.0);
    const int blockSize = GENERATE (64, 256);

    // 228 Hz sits between A3 (220) and A#3 (233.08); C major has only A.
    const auto out = runEngine (test::sine (228.0, sampleRate, 1.0), sampleRate, blockSize, { .speedMs = 5.0f });
    const auto detected = test::medianDetectedHz (out, sampleRate, 0.3);
    INFO ("rate " << sampleRate << ", block " << blockSize << ", detected " << detected << " Hz");
    CHECK (std::abs (test::centsBetween (detected, 220.0)) < 8.0);
}

TEST_CASE ("Root note and scale change the correction target")
{
    // 228 Hz in A# major moves up to A#3 instead.
    const auto out = runEngine (test::sine (228.0, 48000.0, 1.0), 48000.0, 128,
                                { .rootNote = 10, .scale = dsp::Scale::Major, .speedMs = 5.0f });
    CHECK (std::abs (test::centsBetween (test::medianDetectedHz (out, 48000.0, 0.3), 233.08)) < 8.0);
}

TEST_CASE ("Bypass outputs the input delayed by the reported latency")
{
    const double sampleRate = 48000.0;
    const auto input = test::sine (228.0, sampleRate, 0.5);
    const auto out = runEngine (input, sampleRate, 128, { .bypass = true });

    dsp::PitchCorrector engine;
    engine.prepare (sampleRate, 128, 1);
    engine.setParameters ({ .bypass = true });
    const auto latency = static_cast<size_t> (engine.latencySamples());

    float maxError = 0.0f;
    for (size_t i = latency; i < out.size(); ++i)
        maxError = std::max (maxError, std::abs (out[i] - input[i - latency]));
    CHECK (maxError < 1.0e-6f);
}

TEST_CASE ("Engine reports telemetry for the detected note")
{
    dsp::PitchCorrector engine;
    engine.prepare (48000.0, 256, 1);
    engine.setParameters ({});
    auto signal = test::sine (228.0, 48000.0, 0.3);
    for (size_t pos = 0; pos + 256 <= signal.size(); pos += 256)
    {
        float* channels[] = { signal.data() + pos };
        engine.process (channels, 1, 256);
    }
    const auto& t = engine.telemetry();
    CHECK (t.voiced);
    CHECK (t.targetNote == 57); // A3
    CHECK (std::abs (t.deviationCents - test::centsBetween (228.0, 220.0)) < 3.0);
}

TEST_CASE ("Latency follows the voice range")
{
    dsp::PitchCorrector engine;
    engine.prepare (48000.0, 256, 1);
    const auto latencyFor = [&] (dsp::VoiceRange range)
    {
        engine.setParameters ({ .range = range });
        return engine.latencySamples();
    };
    // Rest delay = 2 ms fade headroom + half of 1.05 × the longest period.
    CHECK (latencyFor (dsp::VoiceRange::Low) == 602);    // 12.5 ms
    CHECK (latencyFor (dsp::VoiceRange::Normal) == 413); //  8.6 ms
    CHECK (latencyFor (dsp::VoiceRange::High) == 266);   //  5.5 ms
}

TEST_CASE ("Switching voice range mid-stream keeps correcting without allocating")
{
    const double sampleRate = 48000.0;
    auto signal = test::sine (228.0, sampleRate, 1.5);
    dsp::PitchCorrector engine;
    engine.prepare (sampleRate, 128, 1);

    allocationCount = 0;
    trackAllocations = true;
    for (size_t pos = 0; pos + 128 <= signal.size(); pos += 128)
    {
        const auto range = pos < signal.size() / 3 ? dsp::VoiceRange::Low
                           : pos < 2 * signal.size() / 3 ? dsp::VoiceRange::High : dsp::VoiceRange::Normal;
        engine.setParameters ({ .speedMs = 5.0f, .range = range });
        float* channels[] = { signal.data() + pos };
        engine.process (channels, 1, 128);
    }
    trackAllocations = false;
    CHECK (allocationCount == 0);

    const std::vector<float> tail (signal.end() - static_cast<long> (sampleRate * 0.4), signal.end());
    CHECK (std::abs (test::centsBetween (test::medianDetectedHz (tail, sampleRate, 0.05), 220.0)) < 8.0);
}
