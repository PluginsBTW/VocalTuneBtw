#pragma once

#include "Knob.h"
#include "NoteKeyboard.h"
#include "TuningMeter.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace pitchpilot
{
class PitchPilotProcessor;
}

namespace pitchpilot::ui
{
// Small header icon: a gear or a power symbol.
class IconButton final : public juce::Button
{
public:
    enum class Icon { settings, power };
    IconButton (const juce::String& name, Icon icon);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    Icon icon;
};

// Strength, detune, reference, voice range and latency, behind the gear.
class SettingsPanel final : public juce::Component
{
public:
    explicit SettingsPanel (PitchPilotProcessor& processor);
    void refresh();
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PitchPilotProcessor& processor;
    Knob strength, detune, reference;
    juce::ComboBox range;
    juce::String latencyText;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rangeAttachment;
};

// Everything inside the editor, laid out at designWidth × designHeight.
class MainPanel final : public juce::Component
{
public:
    explicit MainPanel (PitchPilotProcessor& processor);

    // Pulls telemetry and parameter state. Call from the editor's timer.
    void refresh();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void setParameter (const juce::String& id, float value);
    bool parameterIs (const juce::String& id, float value) const;

    PitchPilotProcessor& processor;
    juce::AudioProcessorValueTreeState& state;

    // Invisible link over the painted logo and wordmark.
    juce::HyperlinkButton logoLink { {}, juce::URL ("https://github.com/PluginsBTW/VocalTuneBtw") };
    IconButton settingsButton { "Settings", IconButton::Icon::settings };
    IconButton powerButton { "Bypass", IconButton::Icon::power };
    juce::TextButton naturalButton { "Natural" }, hardButton { "Hard" };
    juce::ComboBox keyBox, scaleBox;
    Knob speed, humanize;
    TuningMeter meter;
    NoteKeyboard keyboard;
    SettingsPanel settings;

    juce::Rectangle<float> keyScalePill;
    float displayedCents = 0.0f;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAttachment, scaleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
};
} // namespace pitchpilot::ui
