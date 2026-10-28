#include "dsp/CorrectionController.h"
#include "dsp/PitchMath.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

using namespace pitchpilot::dsp;

namespace
{
constexpr double rate = 48000.0;

PitchEstimate voicedAt (double midi, double reference = 440.0)
{
    return { static_cast<float> (midiToFrequency (midi, reference)), 0.95f, true };
}

float settle (CorrectionController& c, int samples = 48000)
{
    float s = 0.0f;
    for (int i = 0; i < samples; ++i)
        s = c.nextShiftSemitones();
    return s;
}

ScaleMapper cMajor()
{
    ScaleMapper m;
    m.setMask (scaleMask (0, Scale::Major));
    return m;
}
} // namespace

TEST_CASE ("Correction pulls a sharp note onto the scale")
{
    CorrectionController c;
    c.prepare (rate);
    c.update (voicedAt (60.4), cMajor());
    CHECK (c.targetNote() == 60);
    CHECK (settle (c) == Catch::Approx (-0.4).margin (1e-3));
}

TEST_CASE ("Strength, detune and tolerance shape the shift")
{
    CorrectionController c;
    c.prepare (rate);
    const auto mapper = cMajor();

    c.setSettings ({ .speedMs = 25, .strength = 0.5f });
    c.update (voicedAt (62.3), mapper);
    CHECK (settle (c) == Catch::Approx (-0.15).margin (1e-3));

    c.setSettings ({ .speedMs = 25, .strength = 1.0f, .detuneCents = 20.0f });
    c.update (voicedAt (62.3), mapper);
    CHECK (settle (c) == Catch::Approx (-0.3 + 0.2).margin (1e-3));

    c.setSettings ({ .speedMs = 25, .strength = 1.0f, .toleranceCents = 35.0f });
    c.update (voicedAt (62.3), mapper);
    CHECK (settle (c) == Catch::Approx (0.0).margin (1e-3));
    c.update (voicedAt (62.4), mapper);
    CHECK (settle (c) == Catch::Approx (-0.4).margin (1e-3));
}

TEST_CASE ("Reference tuning moves the target grid")
{
    CorrectionController c;
    c.prepare (rate);
    c.setSettings ({ .referenceHz = 432.0f });
    // 432 Hz is exactly A4 relative to a 432 Hz reference: no shift needed.
    c.update ({ 432.0f, 0.95f, true }, cMajor());
    CHECK (c.targetNote() == 69);
    CHECK (settle (c) == Catch::Approx (0.0).margin (1e-3));
}

TEST_CASE ("Speed is the smoothing time constant")
{
    CorrectionController c;
    c.prepare (rate);
    c.setSettings ({ .speedMs = 50.0f });
    c.update (voicedAt (60.5 - 1e-6), cMajor()); // goal ≈ −0.5
    // After one time constant a one-pole filter covers 1 − 1/e of the step.
    const float atTau = settle (c, static_cast<int> (0.05 * rate));
    CHECK (atTau / -0.5f == Catch::Approx (1.0 - std::exp (-1.0)).margin (0.01));
}

TEST_CASE ("Hysteresis keeps the current note near the midpoint")
{
    CorrectionController c;
    c.prepare (rate);
    ScaleMapper chromatic;
    c.update (voicedAt (60.45), chromatic);
    CHECK (c.targetNote() == 60);
    c.update (voicedAt (60.53), chromatic); // nearer to 61, but within hysteresis
    CHECK (c.targetNote() == 60);
    c.update (voicedAt (60.65), chromatic);
    CHECK (c.targetNote() == 61);
}

TEST_CASE ("Unvoiced input returns to the detune offset only")
{
    CorrectionController c;
    c.prepare (rate);
    c.setSettings ({ .detuneCents = -10.0f });
    c.update (voicedAt (60.4), cMajor());
    settle (c);
    c.update ({}, cMajor());
    CHECK_FALSE (c.hasTarget());
    CHECK (settle (c) == Catch::Approx (-0.1).margin (1e-3));
}
