#pragma once

#include <juce_graphics/juce_graphics.h>

namespace pitchpilot::ui
{
namespace colours
{
    // Dark navy with one blue accent.
    inline const juce::Colour frame { 0xff0a0f18 };
    inline const juce::Colour background { 0xff111927 };
    inline const juce::Colour panel { 0xff172131 };
    inline const juce::Colour edge { 0xff263347 };
    inline const juce::Colour text { 0xffe6edf7 };
    inline const juce::Colour textDim { 0xff8394ab };
    inline const juce::Colour textOnAccent { 0xff0b1220 };
    inline const juce::Colour accent { 0xff3d9df5 };
    inline const juce::Colour accentDim { 0xff1d3f66 };
    inline const juce::Colour track { 0xff223044 };
    inline const juce::Colour knobTop { 0xff1d2838 };
    inline const juce::Colour knobBottom { 0xff121a27 };
    inline const juce::Colour meterFace { 0xff0b1220 };
    inline const juce::Colour keyWhite { 0xffd9e2ee };
    inline const juce::Colour keyWhiteOff { 0xff55637a };
    inline const juce::Colour keyBlack { 0xff1f2a3a };
    inline const juce::Colour keyBlackOff { 0xff0d141f };
    inline const juce::Colour keyActive { 0xff3d9df5 };
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
