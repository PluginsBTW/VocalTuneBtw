#pragma once

#include <vector>

namespace pitchpilot::dsp
{
// Pitch-synchronous splice shifter. One read tap moves through a delay line at
// (1 − ratio) samples per sample. When it leaves the allowed delay range, a
// second tap starts a whole number of input periods away and the output
// crossfades to it. Because the jump is period-aligned, the two taps are in
// phase for periodic input, so there's no phase cancellation and no pitch error.
// All channels share the tap positions, so stereo stays coherent.
class PitchShifter
{
public:
    static constexpr float minRatio = 0.5f;
    static constexpr float maxRatio = 2.0f;

    // Allocates. Call off the audio thread.
    void prepare (double sampleRate, int numChannels, float rangeMs = 22.0f, float fadeMs = 4.0f);
    void reset() noexcept;

    // Period of the input signal in samples (0 when unknown). Real-time safe.
    void setPeriodHint (float periodSamples) noexcept { period = periodSamples; }

    // Delay at rest, in samples. The tap wanders around this while shifting.
    int latencySamples() const noexcept { return static_cast<int> (centre); }

    // In-place. ratios[i] is the pitch ratio for sample i.
    void process (float* const* channels, int numChannels, int numSamples, const float* ratios) noexcept;

private:
    float readTap (const std::vector<float>& buffer, float delay) const noexcept;
    float jumpLength() const noexcept;

    std::vector<std::vector<float>> delayLines;
    int mask = 0, writePos = 0;
    float minDelay = 0.0f, maxDelay = 0.0f, centre = 0.0f;
    float fadeLength = 1.0f;
    float period = 0.0f;

    float delay = 0.0f, fadingDelay = 0.0f;
    int fadeRemaining = 0;
};
} // namespace pitchpilot::dsp
