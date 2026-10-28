#include "PluginEditor.h"

#include "ui/Theme.h"

namespace pitchpilot
{
PitchPilotEditor::PitchPilotEditor (PitchPilotProcessor& owner) : AudioProcessorEditor (owner), panel (owner)
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (panel);

    setResizable (true, true);
    setResizeLimits (ui::designWidth * 3 / 4, ui::designHeight * 3 / 4, ui::designWidth * 2, ui::designHeight * 2);
    getConstrainer()->setFixedAspectRatio (static_cast<double> (ui::designWidth) / ui::designHeight);
    setSize (ui::designWidth, ui::designHeight);
    startTimerHz (30);
}

PitchPilotEditor::~PitchPilotEditor()
{
    setLookAndFeel (nullptr);
}

void PitchPilotEditor::resized()
{
    panel.setTransform (juce::AffineTransform::scale (static_cast<float> (getWidth()) / ui::designWidth));
}

void PitchPilotEditor::timerCallback()
{
    panel.refresh();
}
} // namespace pitchpilot
