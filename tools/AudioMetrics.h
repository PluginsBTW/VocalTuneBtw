#pragma once

// Offline quality metrics for rendered audio. Used by tools/vtb-render and the
// tests. Not real-time code: it allocates freely.

#include "dsp/PitchDetector.h"
#include "dsp/PitchMath.h"
#include "dsp/Scales.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <deque>
#include <numbers>
#include <vector>

namespace pitchpilot::eval
{
// Per-sample jump score: |x[n] − x[n−1]| relative to the RMS of the first
// difference in a ±windowSeconds neighbourhood, leaving out the jump itself
// and its two neighbours on each side. A sine scores √2 at most, white noise
// about 4–5, and a discontinuity in smooth audio far more. Bright, impulsive
// sources (a sawtooth, glottal pulses) score high too, which is why
// clickScores() compares the output against the input. Caveat: the shifter's
// linear interpolation softens the top octave, which lowers the denominator
// and raises the score of such sources (a plain sawtooth reaches about 6).
inline std::vector<float> jumpScores (const std::vector<float>& x, double sampleRate, double windowSeconds = 0.005)
{
    const size_t n = x.size();
    std::vector<float> scores (n, 0.0f);
    if (n < 2)
        return scores;

    // prefix[i] = sum of d[k]² for k < i, where d[k] = x[k] − x[k−1] (d[0] = 0).
    std::vector<double> prefix (n + 1, 0.0);
    for (size_t i = 0; i < n; ++i)
    {
        const double d = i > 0 ? static_cast<double> (x[i]) - x[i - 1] : 0.0;
        prefix[i + 1] = prefix[i] + d * d;
    }
    const auto sum = [&] (size_t from, size_t to) { return prefix[to] - prefix[from]; }; // [from, to)

    const auto half = static_cast<size_t> (std::max (8.0, windowSeconds * sampleRate));
    constexpr size_t gap = 2;
    constexpr double floorRms = 1.0e-5; // about −100 dBFS: silence must not divide by zero
    for (size_t i = 1; i < n; ++i)
    {
        const size_t lo = i > half ? i - half : 0, hi = std::min (n, i + half + 1);
        const size_t exLo = i > gap ? i - gap : 0, exHi = std::min (n, i + gap + 1);
        const double count = static_cast<double> ((hi - lo) - (exHi - exLo));
        const double energy = std::max (0.0, sum (lo, hi) - sum (exLo, exHi));
        const double rms = count > 0 ? std::sqrt (energy / count) : 0.0;
        scores[i] = static_cast<float> (std::abs (static_cast<double> (x[i]) - x[i - 1]) / std::max (rms, floorRms));
    }
    return scores;
}

// Running maximum over ±radius samples (monotonic queue, O(n)).
inline std::vector<float> localMax (const std::vector<float>& x, size_t radius)
{
    const size_t n = x.size();
    std::vector<float> out (n, 0.0f);
    std::deque<size_t> queue; // indices with decreasing values
    size_t next = 0;
    for (size_t i = 0; i < n; ++i)
    {
        for (; next < n && next <= i + radius; ++next)
        {
            while (! queue.empty() && x[queue.back()] <= x[next])
                queue.pop_back();
            queue.push_back (next);
        }
        while (queue.front() + radius < i)
            queue.pop_front();
        out[i] = x[queue.front()];
    }
    return out;
}

struct Click
{
    double seconds = 0.0; // in the output
    float score = 0.0f;
};

struct ClickReport
{
    float maxScore = 0.0f;
    double maxSeconds = 0.0;
    std::vector<Click> clicks; // above the threshold, loudest first, ≥ 10 ms apart
};

// Click score of a processed signal: the output's jump score divided by the
// largest jump score of the input near the same moment (input time = output
// time − latency, ± searchSeconds to allow for the wandering shifter tap). A
// faithful process scores around 1 or below; a splice discontinuity the input
// didn't have scores well above it. The input term is floored at √2 (a sine),
// so a pop in silence still stands out.
inline ClickReport clickScores (const std::vector<float>& input, const std::vector<float>& output, double sampleRate,
                                int latencySamples, float threshold = 2.0f, size_t maxClicks = 20,
                                double searchSeconds = 0.01)
{
    const auto inJumps = jumpScores (input, sampleRate);
    const auto outJumps = jumpScores (output, sampleRate);
    const auto inPeak = localMax (inJumps, static_cast<size_t> (searchSeconds * sampleRate));

    ClickReport report;
    std::vector<Click> candidates;
    const auto separation = static_cast<size_t> (0.01 * sampleRate);
    size_t lastAbove = 0;
    bool open = false;
    for (size_t i = 0; i < outJumps.size(); ++i)
    {
        const auto aligned = static_cast<std::ptrdiff_t> (i) - latencySamples;
        const float reference = aligned >= 0 && static_cast<size_t> (aligned) < inPeak.size()
                                    ? inPeak[static_cast<size_t> (aligned)]
                                    : 0.0f;
        const float score = outJumps[i] / std::max (reference, std::numbers::sqrt2_v<float>);
        const double seconds = static_cast<double> (i) / sampleRate;
        if (score > report.maxScore)
        {
            report.maxScore = score;
            report.maxSeconds = seconds;
        }
        if (score <= threshold)
            continue;
        if (open && i - lastAbove <= separation)
        {
            if (score > candidates.back().score)
                candidates.back() = { seconds, score };
        }
        else
        {
            candidates.push_back ({ seconds, score });
            open = true;
        }
        lastAbove = i;
    }
    std::sort (candidates.begin(), candidates.end(), [] (const Click& a, const Click& b) { return a.score > b.score; });
    if (candidates.size() > maxClicks)
        candidates.resize (maxClicks);
    report.clicks = std::move (candidates);
    return report;
}

struct PitchReport
{
    double voicedFraction = 0.0;   // voiced hops / all hops
    double medianAbsCents = 0.0;   // |pitch − nearest allowed note| over voiced hops
    double p90AbsCents = 0.0;
    int hops = 0;
};

// Tracks the pitch of `signal` with the plugin's own detector and measures how
// far each voiced estimate is from the nearest note the mask allows.
inline PitchReport pitchError (const std::vector<float>& signal, double sampleRate, dsp::NoteMask mask, float minHz,
                               float maxHz, double referenceHz = 440.0)
{
    dsp::PitchDetector detector;
    detector.prepare (sampleRate);
    detector.setRange (minHz, maxHz);
    dsp::ScaleMapper mapper;
    mapper.setMask (mask);

    PitchReport report;
    std::vector<double> errors;
    for (float sample : signal)
    {
        if (! detector.push (sample))
            continue;
        ++report.hops;
        const auto& estimate = detector.estimate();
        if (! estimate.voiced)
            continue;
        const double midi = dsp::frequencyToMidi (estimate.frequencyHz, referenceHz);
        if (const auto note = mapper.nearest (midi))
            errors.push_back (std::abs (midi - *note) * 100.0);
    }
    if (report.hops > 0)
        report.voicedFraction = static_cast<double> (errors.size()) / report.hops;
    if (! errors.empty())
    {
        std::sort (errors.begin(), errors.end());
        report.medianAbsCents = errors[errors.size() / 2];
        report.p90AbsCents = errors[std::min (errors.size() - 1, errors.size() * 9 / 10)];
    }
    return report;
}
} // namespace pitchpilot::eval
