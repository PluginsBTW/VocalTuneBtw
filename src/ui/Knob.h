#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace pitchpilot::ui
{
// A rotary control bound to one parameter: live value inside the dial,
// captions under each end of the arc, and the title below.
class Knob final : public juce::Component
{
public:
    struct Style
    {
        juce::String title, minCaption, maxCaption;
        std::function<juce::String (double)> format;
        bool reversed = false; // high values on the left (e.g. Slow … Fast for a time in ms)
        float titleHeight = 20.0f;
    };

    Knob (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId, Style style);

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    class Dial final : public juce::Slider
    {
    public:
        bool reversed = false;
        double proportionOfLengthToValue (double proportion) override;
        double valueToProportionOfLength (double value) override;
    };

    void updateValueText();

    Style style;
    Dial dial;
    juce::Rectangle<float> titleArea, valueArea, captionArea;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};
} // namespace pitchpilot::ui
