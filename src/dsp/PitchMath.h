#pragma once

#include <cmath>

namespace pitchpilot::dsp
{
inline double frequencyToMidi (double hz, double referenceHz = 440.0) noexcept
{
    return 69.0 + 12.0 * std::log2 (hz / referenceHz);
}

inline double midiToFrequency (double midi, double referenceHz = 440.0) noexcept
{
    return referenceHz * std::exp2 ((midi - 69.0) / 12.0);
}

inline double semitonesToRatio (double semitones) noexcept
{
    return std::exp2 (semitones / 12.0);
}

inline int pitchClass (int midiNote) noexcept
{
    return ((midiNote % 12) + 12) % 12;
}
} // namespace pitchpilot::dsp
