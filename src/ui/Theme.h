#pragma once

#include <juce_graphics/juce_graphics.h>

namespace pitchpilot::ui
{
namespace colours
{
    inline const juce::Colour background { 0xff1c1e21 };
    inline const juce::Colour header { 0xff222528 };
    inline const juce::Colour panel { 0xff26292d };
    inline const juce::Colour edge { 0xff383c41 };
    inline const juce::Colour text { 0xffe6e9ec };
    inline const juce::Colour textDim { 0xff8d949b };
    inline const juce::Colour accent { 0xff3fb6e6 };
    inline const juce::Colour accentDim { 0xff1d4e66 };
    inline const juce::Colour warm { 0xffe8a33d };
    inline const juce::Colour knobTop { 0xff3c4045 };
    inline const juce::Colour knobBottom { 0xff1d2023 };
    inline const juce::Colour meterFace { 0xff16181a };
    inline const juce::Colour keyWhite { 0xffdfe3e7 };
    inline const juce::Colour keyWhiteOff { 0xff80878e };
    inline const juce::Colour keyBlack { 0xff2c3034 };
    inline const juce::Colour keyBlackOff { 0xff141618 };
    inline const juce::Colour keyActive { 0xff2f7fc1 };
} // namespace colours

inline juce::Font font (float height, bool bold = false)
{
    auto options = juce::FontOptions (height);
    return juce::Font (bold ? options.withStyle ("Bold") : options);
}

// Design size of the editor. The editor scales this to its actual size.
inline constexpr int designWidth = 760;
inline constexpr int designHeight = 480;
} // namespace pitchpilot::ui
