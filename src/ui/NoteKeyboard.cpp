#include "NoteKeyboard.h"

#include "Theme.h"
#include "plugin/Parameters.h"

namespace pitchpilot::ui
{
namespace
{
    constexpr int whitePitchClasses[] = { 0, 2, 4, 5, 7, 9, 11 };
    constexpr int blackPitchClasses[] = { 1, 3, 6, 8, 10 };
    constexpr int blackAfterWhite[] = { 0, 1, 3, 4, 5 }; // white key index each black key sits after
    constexpr const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
} // namespace

NoteKeyboard::NoteKeyboard (juce::AudioProcessorValueTreeState& s) : state (s)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

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

std::vector<NoteKeyboard::Key> NoteKeyboard::keys() const
{
    const auto bounds = getLocalBounds().toFloat();
    const float whiteWidth = bounds.getWidth() / (7.0f * octaves);
    const float blackWidth = whiteWidth * 0.62f;
    const float blackHeight = bounds.getHeight() * 0.6f;

    std::vector<Key> result;
    for (int octave = 0; octave < octaves; ++octave)
        for (int i = 0; i < 7; ++i)
            result.push_back ({ whitePitchClasses[i], octave, false,
                                { bounds.getX() + (octave * 7 + i) * whiteWidth, bounds.getY(), whiteWidth, bounds.getHeight() } });
    // Black keys last: they're drawn on top and hit-tested first.
    for (int octave = 0; octave < octaves; ++octave)
        for (int i = 0; i < 5; ++i)
        {
            const float x = bounds.getX() + (octave * 7 + blackAfterWhite[i] + 1) * whiteWidth - blackWidth * 0.5f;
            result.push_back ({ blackPitchClasses[i], octave, true, { x, bounds.getY(), blackWidth, blackHeight } });
        }
    return result;
}

void NoteKeyboard::paint (juce::Graphics& g)
{
    // The target lands on one of the two drawn octaves.
    const int activePitchClass = activeNote >= 0 ? activeNote % 12 : -1;
    const int activeOctave = activeNote >= 0 ? (activeNote / 12) % octaves : -1;

    for (const auto& key : keys())
    {
        const bool lit = inScale (key.pitchClass) && enabled (key.pitchClass);
        const bool active = key.pitchClass == activePitchClass && key.octave == activeOctave;

        auto fill = key.black ? (lit ? colours::keyBlack : colours::keyBlackOff) : (lit ? colours::keyWhite : colours::keyWhiteOff);
        if (active)
            fill = colours::keyActive;

        const auto area = key.black ? key.area : key.area.reduced (1.0f, 0.0f);
        g.setColour (fill);
        g.fillRoundedRectangle (area, 3.0f);
        if (key.black)
        {
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.drawRoundedRectangle (area, 3.0f, 1.0f);
        }

        if (! inScale (key.pitchClass))
            continue;

        // Label in-scale notes; removed notes get a dimmed label.
        const auto label = area.withTrimmedTop (area.getHeight() - 24.0f).reduced (3.0f, 4.0f);
        auto textColour = key.black || active ? colours::text : colours::background;
        if (! enabled (key.pitchClass))
            textColour = textColour.withAlpha (0.35f);
        g.setColour (textColour);
        g.setFont (font (12.0f, true));
        g.drawText (names[key.pitchClass], label, juce::Justification::centred);
    }
}

void NoteKeyboard::mouseDown (const juce::MouseEvent& event)
{
    const auto allKeys = keys();
    for (auto it = allKeys.rbegin(); it != allKeys.rend(); ++it)
    {
        if (! it->area.contains (event.position))
            continue;
        if (inScale (it->pitchClass))
            if (auto* parameter = state.getParameter (ParamID::note (it->pitchClass)))
            {
                parameter->beginChangeGesture();
                parameter->setValueNotifyingHost (enabled (it->pitchClass) ? 0.0f : 1.0f);
                parameter->endChangeGesture();
            }
        return;
    }
}
} // namespace pitchpilot::ui
