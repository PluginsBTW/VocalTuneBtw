#include "dsp/PitchMath.h"
#include "dsp/Scales.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace pitchpilot::dsp;

TEST_CASE ("MIDI and frequency conversions round-trip")
{
    CHECK (frequencyToMidi (440.0) == Catch::Approx (69.0));
    CHECK (frequencyToMidi (261.6256) == Catch::Approx (60.0).margin (1e-4));
    CHECK (midiToFrequency (57.0) == Catch::Approx (220.0));
    CHECK (frequencyToMidi (432.0, 432.0) == Catch::Approx (69.0));
    CHECK (pitchClass (-1) == 11);
}

TEST_CASE ("Scale masks rotate with the root")
{
    CHECK (scaleMask (0, Scale::Major) == 0b1010'1011'0101);       // C D E F G A B
    CHECK (scaleMask (9, Scale::NaturalMinor) == scaleMask (0, Scale::Major)); // A minor = C major
    CHECK (scaleMask (7, Scale::Major) == static_cast<NoteMask> ((scaleMask (0, Scale::Major) & ~(1u << 5)) | (1u << 6))); // G major: F#
    CHECK (scaleMask (3, Scale::Chromatic) == allNotes);
    CHECK (scaleMask (0, Scale::MajorPentatonic) == 0b0010'1001'0101);
}

TEST_CASE ("Every scale has a name and a non-empty mask")
{
    for (int s = 0; s < static_cast<int> (Scale::Count); ++s)
    {
        CHECK (std::string_view (scaleName (static_cast<Scale> (s))).size() > 0);
        CHECK (scaleIntervals (static_cast<Scale> (s)) & 1u); // root always included
    }
}

TEST_CASE ("ScaleMapper picks the nearest allowed note")
{
    ScaleMapper mapper;
    mapper.setMask (scaleMask (0, Scale::Major));

    CHECK (mapper.nearest (60.3) == 60); // C
    CHECK (mapper.nearest (60.9) == 60); // flat C# → C
    CHECK (mapper.nearest (61.2) == 62); // sharp C# → D
    CHECK (mapper.nearest (61.0) == 60); // C# is a tie: resolves down
    CHECK (mapper.nearest (64.4) == 64); // E (F is a semitone above)
    CHECK (mapper.nearest (-3.0) == -3); // A below MIDI 0 still maps
}

TEST_CASE ("ScaleMapper handles sparse and empty masks")
{
    ScaleMapper mapper;
    mapper.setMask (1u << 0); // C only
    CHECK (mapper.nearest (65.9) == 60);
    CHECK (mapper.nearest (66.1) == 72);

    mapper.setMask (0);
    CHECK_FALSE (mapper.nearest (60.0).has_value());
}
