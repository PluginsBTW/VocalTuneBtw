#include "NoteKeyboard.h"

#include "Theme.h"
#include "plugin/Parameters.h"

namespace pitchpilot::ui
{
namespace
{
    constexpr bool black[] = { false, true, false, true, false, false, true, false, true, false, true, false };
    constexpr const char* names[] = { "C", "C#\nDb", "D", "D#\nEb", "E", "F", "F#\nGb", "G", "G#\nAb", "A", "A#\nBb", "B" };
    constexpr float gap = 4.0f;
    constexpr float blackLift = 10.0f; // black keys start higher, like a piano's
    constexpr float radius = 7.0f;     // radius-control
    // A lit black key needs to stand clear of the dark keys outside the scale,
    // so it is lighter than any raised fill: text-dim at 32% over the keyboard.
    const juce::Colour blackKey = colours::display.interpolatedWith (colours::textDim, 0.32f);
} // namespace

NoteKeyboard::NoteKeyboard (juce::AudioProcessorValueTreeState& s) : state (s) {}

void NoteKeyboard::refresh (int targetMidiNote)
{
    const auto load = [this] (const juce::String& id) { return state.getRawParameterValue (id)->load(); };

    const auto scale = static_cast<dsp::Scale> (juce::roundToInt (load (ParamID::scale)));
    const auto newScale = scale == dsp::Scale::Custom ? dsp::allNotes : dsp::scaleMask (juce::roundToInt (load (ParamID::root)), scale);
    std::uint16_t newEnabled = 0;
    for (int pc = 0; pc < 12; ++pc)
        if (load (ParamID::note (pc)) >= 0.5f)
            newEnabled = static_cast<std::uint16_t> (newEnabled | (1u << pc));

    if (newScale != scaleMask || newEnabled != enabledMask || targetMidiNote != activeNote)
    {
        scaleMask = newScale;
        enabledMask = newEnabled;
        activeNote = targetMidiNote;
        repaint();
    }
}

juce::Rectangle<float> NoteKeyboard::keyArea (int pitchClass) const
{
    const auto bounds = getLocalBounds().toFloat();
    const float width = (bounds.getWidth() - 11.0f * gap) / 12.0f;
    const auto column = juce::Rectangle<float> (bounds.getX() + static_cast<float> (pitchClass) * (width + gap), bounds.getY(), width,
                                                bounds.getHeight());
    return black[pitchClass] ? column : column.withTrimmedTop (blackLift);
}

int NoteKeyboard::pitchClassAt (juce::Point<float> position) const
{
    for (int pc = 0; pc < 12; ++pc)
        if (keyArea (pc).expanded (gap * 0.5f, 0.0f).contains (position))
            return pc;
    return -1;
}

void NoteKeyboard::paint (juce::Graphics& g)
{
    const int activePitchClass = activeNote >= 0 ? activeNote % 12 : -1;

    for (int pc = 0; pc < 12; ++pc)
    {
        const auto area = keyArea (pc);
        const bool scale = inScale (pc), lit = scale && enabled (pc), active = pc == activePitchClass;

        // Lit: a filled key. Removed from the scale: a dark key with a hairline
        // and a readable label, as it can be clicked back on. Outside the
        // scale: the same dark key without a border and with a faint label.
        const auto keyColour = black[pc] ? blackKey : colours::text;
        juce::Colour label;
        if (active)
        {
            g.setColour (colours::accentHaloStrong);
            g.fillRoundedRectangle (area.expanded (3.0f), radius + 3.0f);
            g.setColour (colours::accent);
            g.fillRoundedRectangle (area, radius);
            label = colours::background;
        }
        else if (lit)
        {
            g.setColour (keyColour);
            g.fillRoundedRectangle (area, radius);
            label = black[pc] ? colours::text : colours::background;
        }
        else
        {
            g.setColour (colours::display);
            g.fillRoundedRectangle (area, radius);
            if (scale)
            {
                g.setColour (colours::edge);
                g.drawRoundedRectangle (area.reduced (0.5f), radius, 1.0f);
            }
            label = scale ? colours::textDim : colours::textDim.withMultipliedAlpha (0.35f);
        }

        g.setColour (label);
        g.setFont (font (type::controlLabel));
        const float labelHeight = black[pc] ? 36.0f : 22.0f;
        g.drawFittedText (names[pc], area.withTrimmedTop (area.getHeight() - labelHeight - 8.0f).withTrimmedBottom (8.0f).toNearestInt(),
                          juce::Justification::centred, 2);
    }
}

void NoteKeyboard::mouseMove (const juce::MouseEvent& event)
{
    const int pc = pitchClassAt (event.position);
    setMouseCursor (pc >= 0 && inScale (pc) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void NoteKeyboard::mouseDown (const juce::MouseEvent& event)
{
    const int pc = pitchClassAt (event.position);
    if (pc < 0 || ! inScale (pc))
        return;
    if (auto* parameter = state.getParameter (ParamID::note (pc)))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (enabled (pc) ? 0.0f : 1.0f);
        parameter->endChangeGesture();
    }
}
} // namespace pitchpilot::ui
