#include "PitchShifter.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <numbers>

namespace pitchpilot::dsp
{
void PitchShifter::prepare (double sampleRate, int numChannels, float minLowestHz, float fadeMs)
{
    rate = sampleRate;
    lowestAllocatedHz = minLowestHz;
    fadeLength = std::max (16.0f, std::round (static_cast<float> (fadeMs * 0.001 * sampleRate)));
    // During a fade the old tap keeps moving towards the write position by
    // (ratio − 1) samples per sample, so it needs that much headroom below
    // minDelay. process() raises the splice point for larger ratios.
    minDelay = std::ceil (fadeLength * (restHeadroomRatio - 1.0f)) + 2.0f;
    setLowestFrequency (minLowestHz);

    // Room for taps overshooting maxDelay during a fade, plus the window
    // matchingJump() compares behind the farthest candidate.
    const auto size = std::bit_ceil (static_cast<unsigned> (maxDelay + 4.0f * fadeLength) + 4u);
    mask = static_cast<int> (size) - 1;
    delayLines.assign (static_cast<size_t> (std::max (1, numChannels)), std::vector<float> (size, 0.0f));
    reset();
}

void PitchShifter::setLowestFrequency (float hz) noexcept
{
    const float longestPeriod = static_cast<float> (rate) / std::max (hz, lowestAllocatedHz);
    maxDelay = minDelay + std::max (64.0f, std::ceil (longestPeriod * 1.05f));
    const float newCentre = std::round ((minDelay + maxDelay) * 0.5f);
    if (newCentre == centre)
        return;
    centre = newCentre;
    fadingDelay = delay;
    delay = centre;
    fadeRemaining = static_cast<int> (fadeLength);
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

float PitchShifter::jumpLength (bool up) const noexcept
{
    const float range = maxDelay - minDelay;
    if (period < 2.0f || period > range)
        return matchingJump (up);
    return std::floor (range / period) * period;
}

float PitchShifter::matchingJump (bool up) const noexcept
{
    // Compare the samples the current tap has just read with those behind each
    // candidate tap position. The score is 1 for identical windows and falls
    // with both shape and level differences. Ties keep the longest jump, so
    // silence splices as far as the range allows.
    const float range = maxDelay - minDelay;
    const int window = static_cast<int> (2.0f * fadeLength);
    const int longest = static_cast<int> (range), shortest = static_cast<int> (std::ceil (range * 0.5f));
    const int current = writePos - 1 - static_cast<int> (std::lround (delay));

    float currentEnergy = 0.0f;
    for (const auto& line : delayLines)
        for (int k = 0; k < window; ++k)
        {
            const float a = line[static_cast<size_t> ((current - k) & mask)];
            currentEnergy += a * a;
        }

    int best = longest;
    float bestScore = -1.0f;
    for (int jump = longest; jump >= shortest; --jump)
    {
        const int candidate = up ? current - jump : current + jump;
        float cross = 0.0f, energy = 0.0f;
        for (const auto& line : delayLines)
            for (int k = 0; k < window; ++k)
            {
                const float a = line[static_cast<size_t> ((current - k) & mask)];
                const float b = line[static_cast<size_t> ((candidate - k) & mask)];
                cross += a * b;
                energy += b * b;
            }
        const float score = 2.0f * cross / (currentEnergy + energy + 1.0e-12f);
        if (score > bestScore)
        {
            bestScore = score;
            best = jump;
        }
    }
    return static_cast<float> (best);
}

void PitchShifter::process (float* const* channels, int numChannels, int numSamples, const float* ratios) noexcept
{
    const int channelsToProcess = std::min (numChannels, static_cast<int> (delayLines.size()));
    // At unity ratio, drift back to the rest delay (≤ 2 cents of detune while
    // drifting) so latency returns to what we report.
    constexpr float maxDrift = 0.00116f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float ratio = std::clamp (ratios[i], minRatio, maxRatio);
        const float spliceUpAt = std::max (minDelay, fadeLength * (ratio - 1.0f) + 2.0f);
        if (fadeRemaining == 0 && (delay <= spliceUpAt || delay >= maxDelay))
        {
            fadingDelay = delay;
            const bool up = delay <= spliceUpAt;
            delay += up ? jumpLength (up) : -jumpLength (up);
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

        float step = 1.0f - ratio;
        if (std::abs (step) < 1.0e-4f && ! fading)
            step += std::clamp (centre - delay, -maxDrift, maxDrift);
        // If the ratio rises during a fade, neither tap may read ahead of the
        // write position. A downward splice lands the new tap near minDelay,
        // and no splice can start until the fade is over.
        delay = std::max (delay + step, 1.0f);
        fadingDelay = std::max (fadingDelay + step, 1.0f);
        if (fading)
            --fadeRemaining;
        writePos = (writePos + 1) & mask;
    }
}
} // namespace pitchpilot::dsp
