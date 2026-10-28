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
    g.setColour (colours::text);
    g.setFont (font (style.titleHeight * 0.8f, true));
    g.drawText (style.title, titleArea, juce::Justification::centred);

    g.setColour (colours::textDim);
    g.setFont (font (13.0f));
    g.drawText (style.format ? style.format (dial.getValue()) : juce::String (dial.getValue()), valueArea,
                juce::Justification::centred);

    g.drawText (style.minCaption, captionArea, juce::Justification::centredLeft);
    g.drawText (style.maxCaption, captionArea, juce::Justification::centredRight);
}

void Knob::resized()
{
    auto area = getLocalBounds().toFloat();
    titleArea = area.removeFromTop (style.titleHeight);
    valueArea = area.removeFromTop (16.0f);
    captionArea = area.removeFromBottom (style.minCaption.isEmpty() ? 0.0f : 16.0f);
    const float side = juce::jmin (area.getWidth(), area.getHeight());
    dial.setBounds (area.withSizeKeepingCentre (side, side).toNearestInt());
    // Captions sit under the ends of the arc.
    captionArea = captionArea.withSizeKeepingCentre (juce::jmin (side * 1.25f, area.getWidth()), captionArea.getHeight())
                      .translated (0.0f, -8.0f);
}
} // namespace pitchpilot::ui
