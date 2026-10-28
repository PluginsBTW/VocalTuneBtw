#include "PluginEditor.h"

#include "ui/Theme.h"

namespace pitchpilot
{
PitchPilotEditor::PitchPilotEditor (PitchPilotProcessor& owner) : AudioProcessorEditor (owner), panel (owner)
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (panel);

    setResizable (false, false);
    setSize (ui::designWidth, ui::designHeight);
    startTimerHz (30);
}

PitchPilotEditor::~PitchPilotEditor()
{
    setLookAndFeel (nullptr);
}

void PitchPilotEditor::resized()
{
    panel.setBounds (getLocalBounds());
}

void PitchPilotEditor::timerCallback()
{
    panel.refresh();
}
} // namespace pitchpilot
