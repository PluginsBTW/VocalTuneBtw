#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace pitchpilot::ui
{
// Round cents meter (−100 … +100) with the target note in the middle.
class TuningMeter final : public juce::Component
{
public:
    // midiNote < 0 shows the idle state.
    void setReading (int midiNote, float cents);
    void paint (juce::Graphics&) override;

private:
    int note = -1;
    float deviation = 0.0f;
};
} // namespace pitchpilot::ui
