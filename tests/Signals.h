#pragma once

#include "dsp/PitchDetector.h"
#include "dsp/PitchMath.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace pitchpilot::test
{
inline std::vector<float> sine (double hz, double sampleRate, double seconds, float amplitude = 0.5f)
{
    std::vector<float> out (static_cast<size_t> (seconds * sampleRate));
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = amplitude * static_cast<float> (std::sin (2.0 * std::numbers::pi * hz * static_cast<double> (i) / sampleRate));
    return out;
}

// Band-limited sawtooth: strong harmonics provoke octave errors.
inline std::vector<float> sawtooth (double hz, double sampleRate, double seconds, float amplitude = 0.3f)
{
    std::vector<float> out (static_cast<size_t> (seconds * sampleRate), 0.0f);
    const int harmonics = static_cast<int> (sampleRate * 0.45 / hz);
    for (size_t i = 0; i < out.size(); ++i)
    {
        double sum = 0.0;
        const double t = static_cast<double> (i) / sampleRate;
        for (int k = 1; k <= harmonics; ++k)
            sum += std::sin (2.0 * std::numbers::pi * hz * k * t) / k;
        out[i] = amplitude * static_cast<float> (sum * 2.0 / std::numbers::pi);
    }
    return out;
}

// Median of voiced detector estimates after skipping `skipSeconds`.
inline double medianDetectedHz (const std::vector<float>& signal, double sampleRate, double skipSeconds = 0.1,
                                float minHz = 80.0f, float maxHz = 800.0f, double* voicedFraction = nullptr)
{
    dsp::PitchDetector detector;
    detector.prepare (sampleRate);
    detector.setRange (minHz, maxHz);
    std::vector<double> estimates;
    int hops = 0;
    const auto skip = static_cast<size_t> (skipSeconds * sampleRate);
    for (size_t i = 0; i < signal.size(); ++i)
    {
        if (detector.push (signal[i]) && i >= skip)
        {
            ++hops;
            if (detector.estimate().voiced)
                estimates.push_back (detector.estimate().frequencyHz);
        }
    }
    if (voicedFraction)
        *voicedFraction = hops > 0 ? static_cast<double> (estimates.size()) / hops : 0.0;
    if (estimates.empty())
        return 0.0;
    std::sort (estimates.begin(), estimates.end());
    return estimates[estimates.size() / 2];
}

inline double centsBetween (double hz, double referenceHz)
{
    return 1200.0 * std::log2 (hz / referenceHz);
}
} // namespace pitchpilot::test
