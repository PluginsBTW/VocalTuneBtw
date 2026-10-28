#include "TuningMeter.h"

#include "Theme.h"

#include <cmath>

namespace pitchpilot::ui
{
namespace
{
    constexpr float sweep = juce::MathConstants<float>::pi * 0.75f; // ±135° for ±100 cents
    constexpr const char* letters[] = { "C", "C", "D", "D", "E", "F", "F", "G", "G", "A", "A", "B" };
    constexpr bool sharp[] = { false, true, false, true, false, false, true, false, true, false, true, false };

    float angleFor (float cents) { return juce::jlimit (-1.0f, 1.0f, cents / 100.0f) * sweep; }
} // namespace

void TuningMeter::setReading (int midiNote, float cents)
{
    const float rounded = std::round (cents * 2.0f) * 0.5f;
    if (midiNote == note && juce::exactlyEqual (rounded, deviation))
        return;
    note = midiNote;
    deviation = rounded;
    repaint();
}

void TuningMeter::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto centre = bounds.getCentre();
    const float outer = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 24.0f;
    const bool active = note >= 0;

    // Scale labels around the dial.
    g.setFont (font (12.0f));
    g.setColour (colours::textDim);
    for (int cents = -100; cents <= 100; cents += 25)
    {
        const auto p = centre.getPointOnCircumference (outer + 14.0f, angleFor (static_cast<float> (cents)));
        const auto text = cents > 0 ? "+" + juce::String (cents) : juce::String (cents);
        g.drawText (text, juce::Rectangle<float> (34.0f, 14.0f).withCentre (p), juce::Justification::centred);
    }

    // Face.
    const auto face = juce::Rectangle<float> (outer * 2.0f, outer * 2.0f).withCentre (centre);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (face.translated (0.0f, 5.0f).expanded (3.0f));
    g.setColour (colours::meterFace);
    g.fillEllipse (face);
    g.setColour (colours::edge);
    g.drawEllipse (face, 1.5f);

    // Ticks every 2.5 cents; lit from 0 toward the current deviation.
    for (int step = -40; step <= 40; ++step)
    {
        const float cents = static_cast<float> (step) * 2.5f;
        const float angle = angleFor (cents);
        const bool major = step % 10 == 0;
        const float inner = outer * (major ? 0.74f : 0.79f);
        const bool lit = active && step != 0 && cents * deviation > 0.0f && std::abs (cents) <= std::abs (deviation) + 1.25f;

        auto colour = colours::edge;
        if (step == 0)
            colour = active ? colours::text : colours::edge.brighter (0.3f);
        else if (lit)
            colour = colours::accent;

        g.setColour (colour);
        g.drawLine (juce::Line<float> (centre.getPointOnCircumference (inner, angle),
                                       centre.getPointOnCircumference (outer * 0.92f, angle)),
                    major || step == 0 ? 2.5f : 1.6f);
    }

    // Inner disc.
    const float discRadius = outer * 0.64f;
    const auto disc = juce::Rectangle<float> (discRadius * 2.0f, discRadius * 2.0f).withCentre (centre);
    g.setGradientFill (juce::ColourGradient (colours::knobTop, centre.x, disc.getY(), colours::knobBottom, centre.x,
                                             disc.getBottom(), false));
    g.fillEllipse (disc);
    g.setColour (colours::edge);
    g.drawEllipse (disc, 1.0f);

    if (! active)
    {
        g.setColour (colours::textDim);
        g.setFont (font (discRadius * 0.6f));
        g.drawText (juce::String::fromUTF8 ("\xe2\x80\x93"), disc, juce::Justification::centred);
        return;
    }

    const int pitchClass = ((note % 12) + 12) % 12;
    const auto letterFont = font (discRadius * 0.95f);
    const juce::String letter (letters[pitchClass]);
    const float letterWidth = juce::GlyphArrangement::getStringWidth (letterFont, letter);
    const auto letterArea = juce::Rectangle<float> (letterWidth + 4.0f, discRadius).withCentre (centre.translated (0.0f, -discRadius * 0.08f));

    g.setColour (colours::text);
    g.setFont (letterFont);
    g.drawText (letter, letterArea, juce::Justification::centred);
    if (sharp[pitchClass])
    {
        g.setFont (font (discRadius * 0.42f));
        g.drawText ("#", juce::Rectangle<float> (discRadius * 0.4f, discRadius * 0.4f).withPosition (letterArea.getRight(), letterArea.getY()),
                    juce::Justification::centredLeft);
    }

    g.setColour (colours::textDim);
    g.setFont (font (13.0f));
    const auto detail = juce::String (note / 12 - 1) + "  " + (deviation >= 0.0f ? "+" : "") + juce::String (juce::roundToInt (deviation)) + " ct";
    g.drawText ("octave " + detail, disc.withTrimmedTop (discRadius * 1.3f).withTrimmedBottom (discRadius * 0.25f), juce::Justification::centred);
}
} // namespace pitchpilot::ui
