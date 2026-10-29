#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>

namespace pitchpilot::ui
{
// One octave as a row of twelve equal keys, white and black side by side, so
// each pitch class appears exactly once. Notes in the scale are lit. Clicking
// one removes it from (or returns it to) the correction targets. The key of
// the current target note is highlighted.
class NoteKeyboard final : public juce::Component
{
public:
    explicit NoteKeyboard (juce::AudioProcessorValueTreeState& state);

    // Re-reads the parameters. Call from the editor's timer.
    void refresh (int targetMidiNote);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> keyArea (int pitchClass) const;
    int pitchClassAt (juce::Point<float> position) const;
    bool inScale (int pitchClass) const { return (scaleMask >> pitchClass) & 1u; }
    bool enabled (int pitchClass) const { return (enabledMask >> pitchClass) & 1u; }

    juce::AudioProcessorValueTreeState& state;
    std::uint16_t scaleMask = 0, enabledMask = 0;
    int activeNote = -1;
};
} // namespace pitchpilot::ui
