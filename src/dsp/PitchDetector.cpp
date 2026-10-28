#include "PitchDetector.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace pitchpilot::dsp
{
namespace
{
    constexpr double targetAnalysisRate = 16000.0;
    constexpr double lowpassCutoffHz = 2500.0;
    constexpr double hopSeconds = 0.005;
    constexpr float silenceMeanSquare = 1.0e-6f; // about −60 dBFS RMS
} // namespace

void PitchDetector::Biquad::setLowpass (double cutoffHz, double sampleRate) noexcept
{
    // RBJ cookbook low-pass, Q = 1/sqrt(2).
    const double w0 = 2.0 * std::numbers::pi * cutoffHz / sampleRate;
    const double alpha = std::sin (w0) / std::numbers::sqrt2;
    const double cosw = std::cos (w0);
    const double a0 = 1.0 + alpha;
    b0 = (1.0 - cosw) * 0.5 / a0;
    b1 = (1.0 - cosw) / a0;
    b2 = b0;
    a1 = -2.0 * cosw / a0;
    a2 = (1.0 - alpha) / a0;
}

void PitchDetector::prepare (double sampleRate)
{
    inputRate = sampleRate;
    decimation = std::max (1, static_cast<int> (sampleRate / targetAnalysisRate));
    rate = sampleRate / decimation;
    hop = std::max (1, static_cast<int> (std::lround (rate * hopSeconds)));

    // Window and lag both span one period of the lowest supported note.
    maxLag = static_cast<int> (std::ceil (rate / minSupportedHz)) + 3;
    const auto capacity = static_cast<size_t> (2 * maxLag + 2);
    ring.assign (capacity, 0.0f);
    frame.assign (capacity, 0.0f);
    difference.assign (static_cast<size_t> (maxLag + 2), 0.0f);
    normalised.assign (static_cast<size_t> (maxLag + 2), 1.0f);

    const double cutoff = std::min (lowpassCutoffHz, 0.4 * rate);
    lowpass1.setLowpass (cutoff, inputRate);
    lowpass2.setLowpass (cutoff, inputRate);

    setRange (rangeMinHz, rangeMaxHz);
    reset();
}

void PitchDetector::reset() noexcept
{
    lowpass1.z1 = lowpass1.z2 = lowpass2.z1 = lowpass2.z2 = 0.0;
    std::fill (ring.begin(), ring.end(), 0.0f);
    writePos = filled = decimationCounter = hopCounter = 0;
    current = {};
}

void PitchDetector::setRange (float minHz, float maxHz) noexcept
{
    rangeMinHz = std::clamp (minHz, minSupportedHz, maxSupportedHz);
    rangeMaxHz = std::clamp (maxHz, rangeMinHz, maxSupportedHz);
    if (maxLag == 0)
        return;
    minLag = std::max (2, static_cast<int> (std::floor (rate / rangeMaxHz)));
    lagLimit = std::min (maxLag, static_cast<int> (std::ceil (rate / rangeMinHz)) + 3);
}

bool PitchDetector::push (float sample) noexcept
{
    const double filtered = lowpass2.process (lowpass1.process (sample));
    if (++decimationCounter < decimation)
        return false;
    decimationCounter = 0;

    ring[static_cast<size_t> (writePos)] = static_cast<float> (filtered);
    writePos = (writePos + 1) % static_cast<int> (ring.size());
    filled = std::min (filled + 1, static_cast<int> (ring.size()));

    if (++hopCounter < hop)
        return false;
    hopCounter = 0;
    analyse();
    return true;
}

void PitchDetector::analyse() noexcept
{
    const int window = lagLimit;
    const int length = window + lagLimit;
    const int capacity = static_cast<int> (ring.size());
    if (filled < length)
    {
        current = {};
        return;
    }

    // Unwrap the newest `length` samples.
    float energy = 0.0f;
    for (int i = 0, src = (writePos - length + capacity) % capacity; i < length; ++i, src = (src + 1) % capacity)
    {
        const float x = ring[static_cast<size_t> (src)];
        frame[static_cast<size_t> (i)] = x;
        energy += x * x;
    }
    if (energy / static_cast<float> (length) < silenceMeanSquare)
    {
        current = {};
        return;
    }

    // YIN steps 2 and 3: difference function and cumulative mean normalisation.
    const float* x = frame.data();
    float runningSum = 0.0f;
    normalised[0] = 1.0f;
    for (int lag = 1; lag <= lagLimit; ++lag)
    {
        float sum = 0.0f;
        for (int j = 0; j < window; ++j)
        {
            const float d = x[j] - x[j + lag];
            sum += d * d;
        }
        difference[static_cast<size_t> (lag)] = sum;
        runningSum += sum;
        normalised[static_cast<size_t> (lag)] = runningSum > 0.0f ? sum * static_cast<float> (lag) / runningSum : 1.0f;
    }

    // YIN step 4: first dip below the threshold, then walk to its minimum.
    int best = -1;
    float minimum = 1.0f;
    for (int lag = minLag; lag < lagLimit; ++lag)
    {
        const float value = normalised[static_cast<size_t> (lag)];
        minimum = std::min (minimum, value);
        if (value < threshold)
        {
            while (lag + 1 < lagLimit && normalised[static_cast<size_t> (lag + 1)] < normalised[static_cast<size_t> (lag)])
                ++lag;
            best = lag;
            break;
        }
    }
    if (best < 0)
    {
        current = { 0.0f, 1.0f - minimum, false };
        return;
    }

    // YIN step 5: parabolic interpolation on the raw difference function.
    double refinedLag = best;
    if (best > 1 && best < lagLimit)
    {
        const double a = difference[static_cast<size_t> (best - 1)];
        const double b = difference[static_cast<size_t> (best)];
        const double c = difference[static_cast<size_t> (best + 1)];
        const double denominator = a - 2.0 * b + c;
        if (denominator > 0.0)
            refinedLag += std::clamp (0.5 * (a - c) / denominator, -1.0, 1.0);
    }

    const auto hz = static_cast<float> (rate / refinedLag);
    const bool inRange = hz >= rangeMinHz * 0.9f && hz <= rangeMaxHz * 1.1f;
    current = { inRange ? hz : 0.0f, 1.0f - normalised[static_cast<size_t> (best)], inRange };
}
} // namespace pitchpilot::dsp
