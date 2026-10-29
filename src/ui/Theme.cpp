#include "Theme.h"

#include "VocalTuneBtwFonts.h"

namespace pitchpilot::ui
{
juce::Typeface::Ptr ubuntuLight()
{
    static const auto typeface = juce::Typeface::createSystemTypefaceFor (VocalTuneBtwFonts::UbuntuLight_ttf,
                                                                          VocalTuneBtwFonts::UbuntuLight_ttfSize);
    return typeface;
}
} // namespace pitchpilot::ui
