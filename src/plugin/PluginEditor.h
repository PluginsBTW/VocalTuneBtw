#pragma once

#include "PluginProcessor.h"
#include "ui/LookAndFeel.h"
#include "ui/MainPanel.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace pitchpilot
{
// Hosts the main panel in a fixed-size window.
class PitchPilotEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PitchPilotEditor (PitchPilotProcessor& owner);
    ~PitchPilotEditor() override;

    void resized() override;

private:
    void timerCallback() override;

    ui::LookAndFeel lookAndFeel;
    ui::MainPanel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchPilotEditor)
};
} // namespace pitchpilot
