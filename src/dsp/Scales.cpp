#include "Scales.h"

#include "PitchMath.h"

#include <cmath>
#include <initializer_list>

namespace pitchpilot::dsp
{
namespace
{
    constexpr NoteMask bits (std::initializer_list<int> intervals) noexcept
    {
        NoteMask m = 0;
        for (int i : intervals)
            m = static_cast<NoteMask> (m | (1u << i));
        return m;
    }
} // namespace

const char* scaleName (Scale scale) noexcept
{
    switch (scale)
    {
        case Scale::Chromatic:       return "Chromatic";
        case Scale::Major:           return "Major";
        case Scale::NaturalMinor:    return "Natural Minor";
        case Scale::HarmonicMinor:   return "Harmonic Minor";
        case Scale::MelodicMinor:    return "Melodic Minor";
        case Scale::Dorian:          return "Dorian";
        case Scale::Phrygian:        return "Phrygian";
        case Scale::Lydian:          return "Lydian";
        case Scale::Mixolydian:      return "Mixolydian";
        case Scale::Locrian:         return "Locrian";
        case Scale::MajorPentatonic: return "Major Pentatonic";
        case Scale::MinorPentatonic: return "Minor Pentatonic";
        case Scale::Blues:           return "Blues";
        case Scale::Custom:          return "Custom";
        case Scale::Count:           break;
    }
    return "";
}

NoteMask scaleIntervals (Scale scale) noexcept
{
    switch (scale)
    {
        case Scale::Chromatic:       return allNotes;
        case Scale::Major:           return bits ({ 0, 2, 4, 5, 7, 9, 11 });
        case Scale::NaturalMinor:    return bits ({ 0, 2, 3, 5, 7, 8, 10 });
        case Scale::HarmonicMinor:   return bits ({ 0, 2, 3, 5, 7, 8, 11 });
        case Scale::MelodicMinor:    return bits ({ 0, 2, 3, 5, 7, 9, 11 });
        case Scale::Dorian:          return bits ({ 0, 2, 3, 5, 7, 9, 10 });
        case Scale::Phrygian:        return bits ({ 0, 1, 3, 5, 7, 8, 10 });
        case Scale::Lydian:          return bits ({ 0, 2, 4, 6, 7, 9, 11 });
        case Scale::Mixolydian:      return bits ({ 0, 2, 4, 5, 7, 9, 10 });
        case Scale::Locrian:         return bits ({ 0, 1, 3, 5, 6, 8, 10 });
        case Scale::MajorPentatonic: return bits ({ 0, 2, 4, 7, 9 });
        case Scale::MinorPentatonic: return bits ({ 0, 3, 5, 7, 10 });
        case Scale::Blues:           return bits ({ 0, 3, 5, 6, 7, 10 });
        case Scale::Custom:          return allNotes;
        case Scale::Count:           break;
    }
    return allNotes;
}

NoteMask scaleMask (int rootNote, Scale scale) noexcept
{
    const auto intervals = scaleIntervals (scale);
    const int root = pitchClass (rootNote);
    NoteMask m = 0;
    for (int i = 0; i < 12; ++i)
        if (intervals & (1u << i))
            m = static_cast<NoteMask> (m | (1u << ((i + root) % 12)));
    return m;
}

bool ScaleMapper::isAllowed (int midiNote) const noexcept
{
    return (mask & (1u << pitchClass (midiNote))) != 0;
}

std::optional<int> ScaleMapper::nearest (double midiPitch) const noexcept
{
    if (mask == 0 || ! std::isfinite (midiPitch))
        return std::nullopt;

    // Every non-empty mask has an allowed note within six semitones.
    const int centre = static_cast<int> (std::lround (midiPitch));
    std::optional<int> best;
    double bestDistance = 0.0;
    for (int note = centre - 6; note <= centre + 6; ++note)
    {
        if (! isAllowed (note))
            continue;
        const double distance = std::abs (midiPitch - note);
        if (! best || distance < bestDistance)
        {
            best = note;
            bestDistance = distance;
        }
    }
    return best;
}
} // namespace pitchpilot::dsp
