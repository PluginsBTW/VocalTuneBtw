#include "PluginEditor.h"

namespace pitchpilot
{
namespace
{
    juce::String noteName (int midiNote)
    {
        return juce::MidiMessage::getMidiNoteName (midiNote, true, true, 4);
    }
} // namespace

PitchPilotEditor::PitchPilotEditor (PitchPilotProcessor& owner)
    : AudioProcessorEditor (owner), pluginProcessor (owner), parameterEditor (owner)
{
    readout.setFont (juce::FontOptions (18.0f));
    readout.setJustificationType (juce::Justification::centred);
    readout.setColour (juce::Label::backgroundColourId, juce::Colour (0xff1e2124));
    readout.setColour (juce::Label::textColourId, juce::Colour (0xff35d0e0));
    addAndMakeVisible (readout);
    addAndMakeVisible (parameterEditor);

    setResizable (true, true);
    setSize (520, 640);
    startTimerHz (30);
}

void PitchPilotEditor::resized()
{
    auto area = getLocalBounds();
    readout.setBounds (area.removeFromTop (48));
    parameterEditor.setBounds (area);
}

void PitchPilotEditor::timerCallback()
{
    const auto& t = pluginProcessor.telemetry();
    const int target = t.targetNote.load (std::memory_order_relaxed);
    if (target < 0)
    {
        readout.setText ("—", juce::dontSendNotification);
        return;
    }
    const float hz = t.inputHz.load (std::memory_order_relaxed);
    const float deviation = t.deviationCents.load (std::memory_order_relaxed);
    readout.setText (juce::String (hz, 1) + " Hz  →  " + noteName (target) + "   (" + (deviation >= 0 ? "+" : "")
                         + juce::String (deviation, 0) + " cents)",
                     juce::dontSendNotification);
}
} // namespace pitchpilot
