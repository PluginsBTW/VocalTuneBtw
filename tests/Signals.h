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

struct SungNote
{
    double midi;    // continuous pitch, so notes can be off-key
    double seconds; // including the glide into it
    bool consonant; // a short noise burst before the note
};

// A sung phrase: a band-limited pulse train rolled off at −12 dB/octave (like a
// glottal source; a plain sawtooth is all sharp edges, which no voice is) with
// 5.5 Hz vibrato (±50 cents,
// fading in), a slight tremolo, 60 ms glides between legato notes, smooth
// attacks and releases around rests, and 40 ms noise bursts for consonants.
// Unlike a steady sine it makes the corrector change its ratio constantly.
inline std::vector<float> vocalLike (double sampleRate, const std::vector<SungNote>& notes, float amplitude = 0.3f,
                                     unsigned seed = 1)
{
    constexpr double vibratoHz = 5.5, vibratoCents = 50.0, glideSeconds = 0.06;
    constexpr double attackSeconds = 0.015, releaseSeconds = 0.03, consonantSeconds = 0.04;

    double total = 0.0, highest = 0.0;
    for (const auto& note : notes)
    {
        total += note.seconds + (note.consonant ? consonantSeconds : 0.0);
        highest = std::max (highest, note.midi + vibratoCents / 100.0);
    }
    const int harmonics = std::max (1, static_cast<int> (sampleRate * 0.45 / dsp::midiToFrequency (highest)));

    std::vector<float> out (static_cast<size_t> (total * sampleRate), 0.0f);
    unsigned noise = seed;
    const auto white = [&noise] {
        noise = noise * 1664525u + 1013904223u;
        return static_cast<float> (noise >> 8) / static_cast<float> (1u << 23) - 1.0f;
    };

    double phase = 0.0, previousMidi = notes.empty() ? 60.0 : notes.front().midi;
    size_t pos = 0;
    for (size_t n = 0; n < notes.size(); ++n)
    {
        const auto& note = notes[n];
        // Rests surround consonants; legato notes glide from the previous pitch.
        const bool legato = ! note.consonant && n > 0;
        const bool releaseAfter = n + 1 == notes.size() || notes[n + 1].consonant;
        if (note.consonant)
        {
            const auto length = static_cast<size_t> (consonantSeconds * sampleRate);
            for (size_t i = 0; i < length && pos < out.size(); ++i, ++pos)
            {
                const double t = static_cast<double> (i) / static_cast<double> (length);
                out[pos] = 0.5f * amplitude * white() * static_cast<float> (std::sin (std::numbers::pi * t));
            }
        }
        const auto length = static_cast<size_t> (note.seconds * sampleRate);
        for (size_t i = 0; i < length && pos < out.size(); ++i, ++pos)
        {
            const double t = static_cast<double> (i) / sampleRate;
            double midi = note.midi;
            if (legato && t < glideSeconds)
                midi = previousMidi + (note.midi - previousMidi) * (0.5 - 0.5 * std::cos (std::numbers::pi * t / glideSeconds));
            const double depth = std::clamp ((t - 0.15) / 0.2, 0.0, 1.0);
            const double vibrato = std::sin (2.0 * std::numbers::pi * vibratoHz * t);
            midi += depth * vibratoCents / 100.0 * vibrato;

            double gain = 1.0 + 0.1 * depth * vibrato;
            if (! legato)
                gain *= std::min (1.0, t / attackSeconds);
            if (releaseAfter)
                gain *= std::clamp ((note.seconds - t) / releaseSeconds, 0.0, 1.0);

            phase += 2.0 * std::numbers::pi * dsp::midiToFrequency (midi) / sampleRate;
            if (phase > 2.0 * std::numbers::pi)
                phase -= 2.0 * std::numbers::pi;
            double sum = 0.0;
            for (int k = 1; k <= harmonics; ++k)
                sum += std::sin (k * phase) / (k * k);
            out[pos] = amplitude * static_cast<float> (gain * sum);
        }
        previousMidi = note.midi;
    }
    return out;
}

// A phrase in A minor / C major, sung 15–45 cents off with wide leaps, legato
// runs and consonants. About 4.5 s.
inline std::vector<float> vocalPhrase (double sampleRate)
{
    return vocalLike (sampleRate, { { 57.3, 0.5, true },
                                    { 59.8, 0.35, false },
                                    { 60.4, 0.35, false },
                                    { 64.35, 0.6, true },
                                    { 62.2, 0.3, false },
                                    { 59.25, 0.3, false },
                                    { 55.6, 0.7, true },
                                    { 67.3, 0.5, false },
                                    { 64.8, 0.8, true } });
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
