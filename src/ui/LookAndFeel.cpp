#include "LookAndFeel.h"

#include "Theme.h"

namespace pitchpilot::ui
{
namespace
{
    juce::Point<float> polar (juce::Point<float> centre, float radius, float angle)
    {
        return centre.getPointOnCircumference (radius, angle);
    }
} // namespace

LookAndFeel::LookAndFeel()
{
    setColour (juce::Label::textColourId, colours::text);
    setColour (juce::TextButton::textColourOffId, colours::text);
    setColour (juce::TextButton::textColourOnId, colours::text);
    setColour (juce::ComboBox::textColourId, colours::accent);
    setColour (juce::ComboBox::arrowColourId, colours::textDim);
    setColour (juce::PopupMenu::backgroundColourId, colours::panel);
    setColour (juce::PopupMenu::textColourId, colours::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::accentDim);
    setColour (juce::PopupMenu::highlightedTextColourId, colours::text);
}

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                    float startAngle, float endAngle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                static_cast<float> (width), static_cast<float> (height));
    const auto centre = bounds.getCentre();
    const float arcRadius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 4.0f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const juce::PathStrokeType stroke (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (colours::edge);
    g.strokePath (track, stroke);

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, angle, true);
    g.setColour (colours::accent);
    g.strokePath (value, stroke);

    const float bodyRadius = arcRadius * 0.76f;
    const auto body = juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (body.translated (0.0f, 4.0f).expanded (2.0f));
    g.setGradientFill (juce::ColourGradient (colours::knobTop, centre.x, body.getY(), colours::knobBottom, centre.x,
                                             body.getBottom(), false));
    g.fillEllipse (body);
    g.setColour (colours::edge);
    g.drawEllipse (body, 1.0f);

    juce::Path pointer;
    pointer.startNewSubPath (polar (centre, bodyRadius * 0.55f, angle));
    pointer.lineTo (polar (centre, bodyRadius * 0.85f, angle));
    g.setColour (colours::text);
    g.strokePath (pointer, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&, bool highlighted,
                                        bool down)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = button.getToggleState();
    auto fill = on ? colours::accentDim : colours::panel;
    if (highlighted || down)
        fill = fill.brighter (down ? 0.2f : 0.1f);
    g.setColour (fill);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (on ? colours::accent : colours::edge);
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}

juce::Font LookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return font (14.0f);
}

void LookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    const float arrowX = static_cast<float> (width) - 14.0f;
    const float midY = static_cast<float> (height) * 0.5f;
    juce::Path arrow;
    arrow.startNewSubPath (arrowX - 4.0f, midY - 2.0f);
    arrow.lineTo (arrowX, midY + 2.0f);
    arrow.lineTo (arrowX + 4.0f, midY - 2.0f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.strokePath (arrow, juce::PathStrokeType (1.5f));
}

juce::Font LookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return font (15.0f);
}

void LookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (0, 0, box.getWidth() - 22, box.getHeight());
    label.setFont (getComboBoxFont (box));
}

juce::Font LookAndFeel::getPopupMenuFont()
{
    return font (15.0f);
}
} // namespace pitchpilot::ui
