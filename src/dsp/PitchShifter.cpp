#include "PitchShifter.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <numbers>

namespace pitchpilot::dsp
{
void PitchShifter::prepare (double sampleRate, int numChannels, float rangeMs, float fadeMs)
{
    fadeLength = std::max (16.0f, std::round (static_cast<float> (fadeMs * 0.001 * sampleRate)));
    // During a fade the old tap keeps moving by up to one sample per sample
    // (ratio 2), so it needs fadeLength of headroom below minDelay.
    minDelay = fadeLength + 2.0f;
    maxDelay = minDelay + std::max (64.0f, static_cast<float> (rangeMs * 0.001 * sampleRate));
    centre = std::round ((minDelay + maxDelay) * 0.5f);

    const auto size = std::bit_ceil (static_cast<unsigned> (maxDelay + fadeLength) + 4u);
    mask = static_cast<int> (size) - 1;
    delayLines.assign (static_cast<size_t> (std::max (1, numChannels)), std::vector<float> (size, 0.0f));
    reset();
}

void PitchShifter::reset() noexcept
{
    for (auto& line : delayLines)
        std::fill (line.begin(), line.end(), 0.0f);
    writePos = 0;
    delay = fadingDelay = centre;
    fadeRemaining = 0;
}

float PitchShifter::readTap (const std::vector<float>& buffer, float tapDelay) const noexcept
{
    const float position = static_cast<float> (writePos) - tapDelay;
    const float floored = std::floor (position);
    const float fraction = position - floored;
    const int i0 = static_cast<int> (floored) & mask;
    const int i1 = (i0 + 1) & mask;
    return buffer[static_cast<size_t> (i0)] + fraction * (buffer[static_cast<size_t> (i1)] - buffer[static_cast<size_t> (i0)]);
}

float PitchShifter::jumpLength() const noexcept
{
    const float range = maxDelay - minDelay;
    if (period < 2.0f || period > range)
        return range;
    return std::floor (range / period) * period;
}

void PitchShifter::process (float* const* channels, int numChannels, int numSamples, const float* ratios) noexcept
{
    const int channelsToProcess = std::min (numChannels, static_cast<int> (delayLines.size()));
    // At unity ratio, drift back to the rest delay (≤ 2 cents of detune while
    // drifting) so latency returns to what we report.
    constexpr float maxDrift = 0.00116f;

    for (int i = 0; i < numSamples; ++i)
    {
        if (fadeRemaining == 0 && (delay <= minDelay || delay >= maxDelay))
        {
            fadingDelay = delay;
            delay += delay <= minDelay ? jumpLength() : -jumpLength();
            fadeRemaining = static_cast<int> (fadeLength);
        }

        const bool fading = fadeRemaining > 0;
        const float t = fading ? 1.0f - static_cast<float> (fadeRemaining) / fadeLength : 1.0f;
        const float gain = 0.5f - 0.5f * std::cos (std::numbers::pi_v<float> * t);

        for (int ch = 0; ch < channelsToProcess; ++ch)
        {
            auto& line = delayLines[static_cast<size_t> (ch)];
            line[static_cast<size_t> (writePos)] = channels[ch][i];
            const float current = readTap (line, delay);
            channels[ch][i] = fading ? readTap (line, fadingDelay) * (1.0f - gain) + current * gain : current;
        }

        const float ratio = std::clamp (ratios[i], minRatio, maxRatio);
        float step = 1.0f - ratio;
        if (std::abs (step) < 1.0e-4f && ! fading)
            step += std::clamp (centre - delay, -maxDrift, maxDrift);
        delay += step;
        fadingDelay += step;
        if (fading)
            --fadeRemaining;
        writePos = (writePos + 1) & mask;
    }
}
} // namespace pitchpilot::dsp
