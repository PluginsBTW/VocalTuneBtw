#pragma once

#include <cstdint>
#include <optional>

namespace pitchpilot::dsp
{
// Bit n set = pitch class n allowed (0 = C, 11 = B).
using NoteMask = std::uint16_t;
inline constexpr NoteMask allNotes = 0x0FFF;

// Order is persisted in plugin state: append only.
enum class Scale : int
{
    Chromatic,
    Major,
    NaturalMinor,
    HarmonicMinor,
    MelodicMinor,
    Dorian,
    Phrygian,
    Lydian,
    Mixolydian,
    Locrian,
    MajorPentatonic,
    MinorPentatonic,
    Blues,
    Custom,
    Count
};

const char* scaleName (Scale scale) noexcept;

// Intervals relative to the root (bit 0 = root).
NoteMask scaleIntervals (Scale scale) noexcept;

// Absolute pitch-class mask for a scale on the given root (0 = C).
NoteMask scaleMask (int rootNote, Scale scale) noexcept;

class ScaleMapper
{
public:
    void setMask (NoteMask newMask) noexcept { mask = static_cast<NoteMask> (newMask & allNotes); }
    NoteMask getMask() const noexcept { return mask; }

    bool isAllowed (int midiNote) const noexcept;

    // Nearest allowed MIDI note to a continuous pitch; ties resolve downwards.
    // Empty when no note is allowed.
    std::optional<int> nearest (double midiPitch) const noexcept;

private:
    NoteMask mask = allNotes;
};
} // namespace pitchpilot::dsp
