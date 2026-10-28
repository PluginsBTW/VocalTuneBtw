#include "CorrectionController.h"

#include "PitchMath.h"

#include <algorithm>
#include <cmath>

namespace pitchpilot::dsp
{
void CorrectionController::prepare (double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    setSettings (settings);
    reset();
}

void CorrectionController::reset() noexcept
{
    goal = shift = settings.detuneCents / 100.0f;
    input = 0.0;
    target = -1;
    voiced = false;
}

void CorrectionController::setSettings (const CorrectionSettings& newSettings) noexcept
{
    settings = newSettings;
    const double timeConstantSamples = std::max (0.1f, settings.speedMs) * 0.001 * sampleRate;
    smoothing = static_cast<float> (1.0 - std::exp (-1.0 / timeConstantSamples));
}

void CorrectionController::update (const PitchEstimate& estimate, const ScaleMapper& mapper) noexcept
{
    const float detune = settings.detuneCents / 100.0f;
    voiced = estimate.voiced && estimate.frequencyHz > 0.0f;
    if (! voiced)
    {
        goal = detune;
        return;
    }

    input = frequencyToMidi (estimate.frequencyHz, settings.referenceHz);
    const auto candidate = mapper.nearest (input);
    if (! candidate)
    {
        target = -1;
        goal = detune;
        return;
    }

    // Stay on the current note unless another allowed note is clearly closer.
    const bool keep = target >= 0 && mapper.isAllowed (target)
                      && std::abs (input - *candidate) + settings.hysteresisSemitones >= std::abs (input - target);
    if (! keep)
        target = *candidate;

    const double error = target - input;
    const bool withinTolerance = std::abs (error) * 100.0 <= settings.toleranceCents;
    goal = (withinTolerance ? 0.0f : std::clamp (settings.strength, 0.0f, 1.0f) * static_cast<float> (error)) + detune;
}
} // namespace pitchpilot::dsp
