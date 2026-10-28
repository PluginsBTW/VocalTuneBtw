#pragma once

#include "PitchDetector.h"
#include "Scales.h"

namespace pitchpilot::dsp
{
struct CorrectionSettings
{
    float speedMs = 25.0f;       // one-pole time constant toward the goal shift
    float strength = 1.0f;       // 0 … 1
    float detuneCents = 0.0f;    // added after correction
    float toleranceCents = 0.0f; // smaller deviations are left alone
    float referenceHz = 440.0f;  // A4
    float hysteresisSemitones = 0.1f;
};

// Turns pitch estimates into a smoothed pitch shift (in semitones).
class CorrectionController
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    void setSettings (const CorrectionSettings& newSettings) noexcept;

    // Call whenever the detector produces a new estimate.
    void update (const PitchEstimate& estimate, const ScaleMapper& mapper) noexcept;

    // Advance one sample and return the shift to apply.
    float nextShiftSemitones() noexcept
    {
        shift += (goal - shift) * smoothing;
        return shift;
    }

    bool hasTarget() const noexcept { return voiced && target >= 0; }
    int targetNote() const noexcept { return target; }
    double inputPitch() const noexcept { return input; }
    float currentShiftSemitones() const noexcept { return shift; }

private:
    double sampleRate = 48000.0;
    CorrectionSettings settings;
    float smoothing = 1.0f;
    float goal = 0.0f, shift = 0.0f;
    double input = 0.0;
    int target = -1;
    bool voiced = false;
};
} // namespace pitchpilot::dsp
