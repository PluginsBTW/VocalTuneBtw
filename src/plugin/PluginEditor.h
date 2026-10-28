#pragma once

#include "PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace pitchpilot
{
// Placeholder UI until milestone M3: JUCE's generic parameter editor plus a
// live readout of the detected and target notes.
class PitchPilotEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PitchPilotEditor (PitchPilotProcessor& owner);

    void resized() override;

private:
    void timerCallback() override;

    PitchPilotProcessor& pluginProcessor;
    juce::Label readout;
    juce::GenericAudioProcessorEditor parameterEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchPilotEditor)
};
} // namespace pitchpilot
