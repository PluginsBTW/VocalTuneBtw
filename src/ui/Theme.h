#pragma once

#include <juce_graphics/juce_graphics.h>

namespace pitchpilot::ui
{
// PluginsBTW UI tokens (dark theme), shared with HalfSpeedBtw. Names follow
// the design system's tokens.json; translucent ones are base colours at the
// listed alpha.
namespace colours
{
    inline const juce::Colour background { 0xff0a0e15 };   // window ground; label on a selected control
    inline const juce::Colour surface { 0xff111824 };      // the main panel
    inline const juce::Colour raised { 0xff1a2333 };       // resting control fill
    inline const juce::Colour raisedHover { 0xff222e43 };  // hovered control fill, empty arc tracks
    inline const juce::Colour edge { 0xff263247 };         // 1px hairlines, decorative only
    inline const juce::Colour edgeFaint { 0x99263247 };    // edge at 60%: grid lines in the display
    inline const juce::Colour display { 0xff07101f };      // inset screen for live data
    inline const juce::Colour accent { 0xff3d9bff };       // every value a person has set
    inline const juce::Colour accentHalo { 0x263d9bff };   // 15%: halo under active things
    inline const juce::Colour accentHaloStrong { 0x2e3d9bff }; // 18%: halo around a selected segment
    inline const juce::Colour accentDim { 0x593d9bff };    // 35%
    inline const juce::Colour glow { 0xff5fd0ff };         // live and "on": knob tip, power when on
    inline const juce::Colour glowHalo { 0x335fd0ff };     // 20%
    inline const juce::Colour text { 0xffe3eaf5 };         // values and primary text
    inline const juce::Colour textDim { 0xff8a95a9 };      // text-dim-aa: captions, 4.5:1 on every surface
    inline const juce::Colour scrim { 0xe60a0e15 };        // background at 90%
} // namespace colours

// The five PluginsBTW type sizes, in pixels (point height). Don't add others.
namespace type
{
    inline constexpr float wordmark = 22.0f;
    inline constexpr float controlLabel = 13.0f; // labels in buttons and menus
    inline constexpr float valueLarge = 13.0f;   // value inside a large knob
    inline constexpr float value = 12.0f;        // other values and readouts
    inline constexpr float caption = 11.0f;      // UPPERCASE control names, hints, status
} // namespace type

// Ubuntu Light (weight 300), embedded so the plugin needs no system fonts.
juce::Typeface::Ptr ubuntuLight();

inline juce::Font font (float size)
{
    return juce::Font (juce::FontOptions (ubuntuLight()).withPointHeight (size));
}

// Design size of the editor. The editor scales this to its actual size.
inline constexpr int designWidth = 760;
inline constexpr int designHeight = 480;
} // namespace pitchpilot::ui
