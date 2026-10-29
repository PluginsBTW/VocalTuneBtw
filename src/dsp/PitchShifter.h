#pragma once

#include <cmath>
#include <vector>

namespace pitchpilot::dsp
{
// Pitch-synchronous splice shifter. One read tap moves through a delay line at
// (1 − ratio) samples per sample. When it leaves the allowed delay range, a
// second tap starts a whole number of input periods away and the output
// crossfades to it. Because the jump is period-aligned, the two taps are in
// phase for periodic input, so there's no phase cancellation and no pitch error.
// Without a usable period, the jump goes to where the recent waveform matches
// best instead. All channels share the tap positions, so stereo stays coherent.
class PitchShifter
{
public:
    static constexpr float minRatio = 0.5f;
    static constexpr float maxRatio = 2.0f;
    // Shifts up to this ratio (3 semitones, more than scale correction usually
    // needs) splice at the fixed minimum delay. Bigger upward shifts start
    // their splice earlier instead, so they don't add latency for everyone.
    static constexpr float restHeadroomRatio = 1.18921f;

    // Allocates for notes down to minLowestHz. Call off the audio thread.
    void prepare (double sampleRate, int numChannels, float minLowestHz = 50.0f, float fadeMs = 1.0f);
    void reset() noexcept;

    // Size the delay range to one period of the lowest expected note (plus a
    // margin). Lower notes need more delay, so this sets the latency.
    // Real-time safe. If the rest delay moves, the tap splices over to it.
    void setLowestFrequency (float hz) noexcept;

    // Period of the input signal in samples (0 when unknown). Real-time safe.
    void setPeriodHint (float periodSamples) noexcept { period = periodSamples; }

    // Delay at rest, in samples. The tap wanders around this while shifting.
    int latencySamples() const noexcept { return static_cast<int> (centre); }

    // How far the actual delay can stray from latencySamples() while shifting:
    // half the delay range, plus the overshoot of splices and fading taps.
    int latencySpreadSamples() const noexcept
    {
        return static_cast<int> (std::ceil ((maxDelay - minDelay) * 0.5f + 2.0f * fadeLength));
    }

    // In-place. ratios[i] is the pitch ratio for sample i.
    void process (float* const* channels, int numChannels, int numSamples, const float* ratios) noexcept;

private:
    float readTap (const std::vector<float>& buffer, float delay) const noexcept;
    float jumpLength (bool up) const noexcept;
    float matchingJump (bool up) const noexcept;

    std::vector<std::vector<float>> delayLines;
    int mask = 0, writePos = 0;
    double rate = 48000.0;
    float lowestAllocatedHz = 50.0f;
    float minDelay = 0.0f, maxDelay = 0.0f, centre = 0.0f;
    float fadeLength = 1.0f;
    float period = 0.0f;

    float delay = 0.0f, fadingDelay = 0.0f;
    int fadeRemaining = 0;
};
} // namespace pitchpilot::dsp
