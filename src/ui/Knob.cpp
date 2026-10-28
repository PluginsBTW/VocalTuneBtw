#include "Knob.h"

#include "Theme.h"

namespace pitchpilot::ui
{
double Knob::Dial::proportionOfLengthToValue (double proportion)
{
    return Slider::proportionOfLengthToValue (reversed ? 1.0 - proportion : proportion);
}

double Knob::Dial::valueToProportionOfLength (double value)
{
    const double proportion = Slider::valueToProportionOfLength (value);
    return reversed ? 1.0 - proportion : proportion;
}

Knob::Knob (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId, Style knobStyle)
    : style (std::move (knobStyle))
{
    dial.reversed = style.reversed;
    dial.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    dial.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    dial.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    addAndMakeVisible (dial);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, parameterId, dial);
    dial.onValueChange = [this] { updateValueText(); };
    updateValueText();
}

void Knob::updateValueText()
{
    repaint (valueArea.getSmallestIntegerContainer());
}

void Knob::paint (juce::Graphics& g)
{
    g.setColour (colours::textDim);
    g.setFont (font (style.titleHeight * 0.62f, true).withExtraKerningFactor (0.08f));
    g.drawText (style.title.toUpperCase(), titleArea, juce::Justification::centred);

    g.setFont (font (11.5f));
    g.drawText (style.minCaption, captionArea, juce::Justification::centredLeft);
    g.drawText (style.maxCaption, captionArea, juce::Justification::centredRight);
}

void Knob::paintOverChildren (juce::Graphics& g)
{
    // The value sits on the knob body, which the dial draws.
    g.setColour (colours::text);
    g.setFont (font (juce::jlimit (12.0f, 19.0f, valueArea.getHeight() * 0.42f), true));
    g.drawFittedText (style.format ? style.format (dial.getValue()) : juce::String (dial.getValue()),
                      valueArea.toNearestInt(), juce::Justification::centred, 1, 0.8f);
}

void Knob::resized()
{
    auto area = getLocalBounds().toFloat();
    titleArea = area.removeFromBottom (style.titleHeight);
    captionArea = area.removeFromBottom (style.minCaption.isEmpty() ? 0.0f : 14.0f);
    const float side = juce::jmin (area.getWidth(), area.getHeight());
    const auto dialBounds = area.withSizeKeepingCentre (side, side);
    dial.setBounds (dialBounds.toNearestInt());
    valueArea = dialBounds.withSizeKeepingCentre (side * 0.5f, side * 0.3f);
    // Captions sit under the ends of the arc.
    captionArea = captionArea.withSizeKeepingCentre (juce::jmin (side * 1.05f, area.getWidth()), captionArea.getHeight())
                      .translated (0.0f, -12.0f);
}
} // namespace pitchpilot::ui
