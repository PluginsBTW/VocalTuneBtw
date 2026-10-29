#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace pitchpilot::ui
{
class LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos, float startAngle,
                           float endAngle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool down, int buttonX, int buttonY, int buttonW,
                       int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;

    // Menus open inside the editor, so their rounded corners can be transparent
    // without a compositing window manager.
    juce::PopupMenu::Options getOptionsForComboBoxPopupMenu (juce::ComboBox&, juce::Label&) override;
    void preparePopupMenuWindow (juce::Component&) override;
    void drawPopupMenuBackgroundWithOptions (juce::Graphics&, int width, int height, const juce::PopupMenu::Options&) override;
    int getPopupMenuBorderSizeWithOptions (const juce::PopupMenu::Options&) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                            bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                            const juce::String& shortcutKeyText, const juce::Drawable* icon,
                            const juce::Colour* textColour) override;
};

// A combo box is hot while the pointer is over it or its menu is open. Checked
// against the pointer position, because JUCE sends no mouse-enter to a box the
// pointer is already over when its menu closes.
bool isHot (const juce::ComboBox&);

// Repaints the box when its hot state changed. Call from the editor's timer.
void updateHover (juce::ComboBox&);
} // namespace pitchpilot::ui
