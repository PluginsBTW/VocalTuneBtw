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
    setDefaultSansSerifTypeface (ubuntuLight());
    setColour (juce::Label::textColourId, colours::text);
    setColour (juce::TextButton::textColourOffId, colours::textDim);
    setColour (juce::TextButton::textColourOnId, colours::background);
    setColour (juce::ComboBox::textColourId, colours::accent);
    setColour (juce::ComboBox::arrowColourId, colours::textDim);
    setColour (juce::PopupMenu::backgroundColourId, colours::raised);
    setColour (juce::PopupMenu::textColourId, colours::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::raisedHover);
    setColour (juce::PopupMenu::highlightedTextColourId, colours::text);
}

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                    float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                static_cast<float> (width), static_cast<float> (height));
    const auto centre = bounds.getCentre();
    const float side = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const float thickness = juce::jmax (4.0f, side * 0.04f);
    // Leave room for the glow and the end dot's halo inside the slider bounds.
    const float arcRadius = side * 0.5f - thickness * 1.8f - 1.0f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    auto stroke = [] (float w) { return juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded); };

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (colours::raisedHover);
    g.strokePath (track, stroke (thickness));

    // Value arc with a soft glow.
    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, angle, true);
    g.setColour (colours::accentHalo);
    g.strokePath (value, stroke (thickness * 3.0f));
    g.setColour (colours::accent);
    g.strokePath (value, stroke (thickness));

    // Flat body: no shadow, no gradient.
    const float bodyRadius = arcRadius - thickness * 2.2f;
    const auto body = juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre);
    g.setColour (slider.isMouseOverOrDragging() ? colours::raisedHover : colours::raised);
    g.fillEllipse (body);
    g.setColour (colours::edge);
    g.drawEllipse (body, 1.0f);

    // Glow dot at the end of the value arc.
    const float dot = thickness * 1.1f;
    const auto tip = polar (centre, arcRadius, angle);
    g.setColour (colours::glowHalo);
    g.fillEllipse (juce::Rectangle<float> (dot * 3.0f, dot * 3.0f).withCentre (tip));
    g.setColour (colours::glow);
    g.fillEllipse (juce::Rectangle<float> (dot * 2.0f, dot * 2.0f).withCentre (tip));
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&, bool highlighted,
                                        bool down)
{
    // Selected: solid accent with a halo. Otherwise raised, raised-hover while hovered.
    const auto bounds = button.getLocalBounds().toFloat().reduced (3.5f);
    if (button.getToggleState())
    {
        g.setColour (colours::accentHaloStrong);
        g.fillRoundedRectangle (bounds.expanded (3.0f), 10.0f);
        g.setColour (colours::accent);
        g.fillRoundedRectangle (bounds, 7.0f);
        return;
    }
    g.setColour (highlighted || down ? colours::raisedHover : colours::raised);
    g.fillRoundedRectangle (bounds, 7.0f);
    g.setColour (colours::edge);
    g.drawRoundedRectangle (bounds, 7.0f, 1.0f);
}

juce::Font LookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return font (type::controlLabel);
}

void LookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    // Like the buttons: raised-hover while hovered or open, and the chevron lights to text.
    const bool hot = isHot (box);
    if (hot)
    {
        g.setColour (colours::raisedHover);
        g.fillRoundedRectangle (juce::Rectangle<float> (static_cast<float> (width), static_cast<float> (height)).reduced (3.0f), 4.0f);
    }

    const float arrowX = static_cast<float> (width) - 14.0f;
    const float midY = static_cast<float> (height) * 0.5f;
    juce::Path arrow;
    arrow.startNewSubPath (arrowX - 4.0f, midY - 2.0f);
    arrow.lineTo (arrowX, midY + 2.0f);
    arrow.lineTo (arrowX + 4.0f, midY - 2.0f);
    g.setColour (hot ? colours::text : box.findColour (juce::ComboBox::arrowColourId));
    g.strokePath (arrow, juce::PathStrokeType (1.5f));
}

juce::Font LookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return font (type::controlLabel);
}

void LookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (4, 0, box.getWidth() - 26, box.getHeight());
    label.setFont (getComboBoxFont (box));
}

juce::Font LookAndFeel::getPopupMenuFont()
{
    return font (type::controlLabel);
}

juce::PopupMenu::Options LookAndFeel::getOptionsForComboBoxPopupMenu (juce::ComboBox& box, juce::Label& label)
{
    return LookAndFeel_V4::getOptionsForComboBoxPopupMenu (box, label).withParentComponent (box.getTopLevelComponent());
}

void LookAndFeel::preparePopupMenuWindow (juce::Component& window)
{
    window.setOpaque (false);
}

void LookAndFeel::drawPopupMenuBackgroundWithOptions (juce::Graphics& g, int width, int height, const juce::PopupMenu::Options&)
{
    // No border: the raised fill alone sets the menu off the panel.
    g.setColour (colours::raised);
    g.fillRoundedRectangle (juce::Rectangle<float> (static_cast<float> (width), static_cast<float> (height)), 7.0f);
}

int LookAndFeel::getPopupMenuBorderSizeWithOptions (const juce::PopupMenu::Options&)
{
    return 4;
}

void LookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                                     bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                                     const juce::String& shortcutKeyText, const juce::Drawable* icon, const juce::Colour* textColour)
{
    // A rounded highlight inset from the menu's sides (items span the full width);
    // the base class then draws the item without its square one.
    if (isHighlighted && isActive && ! isSeparator)
    {
        g.setColour (colours::raisedHover);
        g.fillRoundedRectangle (area.toFloat().reduced (4.0f, 1.0f), 4.0f);
    }
    LookAndFeel_V4::drawPopupMenuItem (g, area, isSeparator, isActive, false, isTicked, hasSubMenu, text, shortcutKeyText, icon,
                                       textColour);
}

bool isHot (const juce::ComboBox& box)
{
    return box.isEnabled() && box.isShowing()
           && (box.isPopupActive() || box.getScreenBounds().contains (juce::Desktop::getMousePosition()));
}

void updateHover (juce::ComboBox& box)
{
    const bool hot = isHot (box);
    if (box.getProperties()["hot"] != juce::var (hot))
    {
        box.getProperties().set ("hot", hot);
        box.repaint();
    }
}
} // namespace pitchpilot::ui
