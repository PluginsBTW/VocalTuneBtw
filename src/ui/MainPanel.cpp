#include "MainPanel.h"

#include "Theme.h"
#include "plugin/PluginProcessor.h"

#include <cmath>

namespace pitchpilot::ui
{
namespace
{
    // The two mode buttons write these values.
    constexpr float naturalSpeedMs = 50.0f, naturalHumanizeCents = 10.0f;
    constexpr float hardSpeedMs = 1.0f, hardHumanizeCents = 0.0f;

    void fillFromChoices (juce::ComboBox& box, juce::AudioProcessorValueTreeState& state, const juce::String& id)
    {
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (id)))
            box.addItemList (choice->choices, 1);
    }

    juce::String signedCents (double v)
    {
        return (v > 0.0 ? "+" : "") + juce::String (v, 1) + " cents";
    }
} // namespace

//==============================================================================
IconButton::IconButton (const juce::String& name, Icon i) : Button (name), icon (i)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void IconButton::paintButton (juce::Graphics& g, bool highlighted, bool)
{
    const auto bounds = getLocalBounds().toFloat().reduced (4.0f);
    const auto centre = bounds.getCentre();
    const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;

    // Power is lit while processing (bypass off). The gear is lit while open.
    const bool lit = icon == Icon::power ? ! getToggleState() : getToggleState();
    auto colour = lit ? colours::accent : colours::textDim;
    if (highlighted)
        colour = colour.brighter (0.3f);
    g.setColour (colour);
    const juce::PathStrokeType stroke (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path path;
    if (icon == Icon::power)
    {
        path.addCentredArc (centre.x, centre.y, r * 0.8f, r * 0.8f, 0.0f, juce::degreesToRadians (35.0f),
                            juce::degreesToRadians (325.0f), true);
        path.startNewSubPath (centre.x, centre.y - r);
        path.lineTo (centre.x, centre.y - r * 0.15f);
        g.strokePath (path, stroke);
        return;
    }

    constexpr int teeth = 8;
    for (int i = 0; i < teeth; ++i)
    {
        const float angle = juce::MathConstants<float>::twoPi * static_cast<float> (i) / teeth;
        path.startNewSubPath (centre.getPointOnCircumference (r * 0.62f, angle));
        path.lineTo (centre.getPointOnCircumference (r * 0.95f, angle));
    }
    g.strokePath (path, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
    g.drawEllipse (juce::Rectangle<float> (r * 1.3f, r * 1.3f).withCentre (centre), 2.5f);
    g.drawEllipse (juce::Rectangle<float> (r * 0.5f, r * 0.5f).withCentre (centre), 1.5f);
}

//==============================================================================
SettingsPanel::SettingsPanel (PitchPilotProcessor& p)
    : processor (p),
      strength (p.state(), ParamID::strength, { "Strength", "0%", "100%", [] (double v) { return juce::String (v, 0) + " %"; } }),
      detune (p.state(), ParamID::detune, { "Detune", "-100", "+100", signedCents }),
      reference (p.state(), ParamID::reference, { "Reference A4", "415", "466", [] (double v) { return juce::String (v, 1) + " Hz"; } })
{
    addAndMakeVisible (strength);
    addAndMakeVisible (detune);
    addAndMakeVisible (reference);

    fillFromChoices (range, p.state(), ParamID::range);
    rangeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.state(), ParamID::range, range);
    addAndMakeVisible (range);
}

void SettingsPanel::refresh()
{
    const double rate = processor.getSampleRate();
    const auto text = rate > 0.0 ? "Latency " + juce::String (1000.0 * processor.getLatencySamples() / rate, 1) + " ms ("
                                       + juce::String (processor.getLatencySamples()) + " samples)"
                                 : juce::String();
    if (text != latencyText)
    {
        latencyText = text;
        repaint();
    }
}

void SettingsPanel::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (colours::panel);
    g.fillRoundedRectangle (bounds, 10.0f);
    g.setColour (colours::edge);
    g.drawRoundedRectangle (bounds, 10.0f, 1.0f);

    const auto box = range.getBounds().toFloat();
    g.setColour (colours::background);
    g.fillRoundedRectangle (box, 5.0f);
    g.setColour (colours::edge);
    g.drawRoundedRectangle (box, 5.0f, 1.0f);

    auto column = juce::Rectangle<float> (box.getX(), box.getY() - 26.0f, bounds.getRight() - box.getX() - 16.0f, 200.0f);
    g.setColour (colours::text);
    g.setFont (font (14.4f, true));
    g.drawText ("Voice Range", column.removeFromTop (18.0f), juce::Justification::centredLeft);
    column.removeFromTop (48.0f);
    g.setFont (font (12.5f));
    g.setColour (colours::textDim);
    g.drawFittedText ("Low reaches 50 Hz, Normal 80 Hz, High 150 Hz.\nA higher range lowers latency.",
                      column.removeFromTop (48.0f).toNearestInt(), juce::Justification::topLeft, 3);
    g.setColour (colours::accent);
    g.drawText (latencyText, column.removeFromTop (20.0f), juce::Justification::centredLeft);
}

void SettingsPanel::resized()
{
    auto area = getLocalBounds().reduced (16, 16);
    auto knobs = area.removeFromLeft (juce::roundToInt (getWidth() * 0.66f) - 16);
    const int knobWidth = knobs.getWidth() / 3;
    strength.setBounds (knobs.removeFromLeft (knobWidth).reduced (8, 0));
    detune.setBounds (knobs.removeFromLeft (knobWidth).reduced (8, 0));
    reference.setBounds (knobs.reduced (8, 0));

    range.setBounds (area.getX() + 8, area.getY() + 36, 150, 28);
}

//==============================================================================
MainPanel::MainPanel (PitchPilotProcessor& p)
    : processor (p),
      state (p.state()),
      speed (state, ParamID::speed,
             { "Retune Speed", "Slow", "Fast", [] (double v) { return juce::String (v, v < 10.0 ? 1 : 0) + " ms"; }, true }),
      humanize (state, ParamID::tolerance,
                { "Humanize", "Off", "Max", [max = state.getParameterRange (ParamID::tolerance).end] (double v)
                  { return v < 0.05 ? juce::String ("Off") : juce::String (100.0 * v / max, 0) + " %"; } }),
      keyboard (state),
      settings (p)
{
    addAndMakeVisible (settingsButton);
    addAndMakeVisible (powerButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, ParamID::bypass, powerButton);
    powerButton.setClickingTogglesState (true);
    settingsButton.setClickingTogglesState (true);
    settingsButton.onClick = [this] { settings.setVisible (settingsButton.getToggleState()); };

    naturalButton.onClick = [this]
    {
        setParameter (ParamID::speed, naturalSpeedMs);
        setParameter (ParamID::tolerance, naturalHumanizeCents);
    };
    hardButton.onClick = [this]
    {
        setParameter (ParamID::speed, hardSpeedMs);
        setParameter (ParamID::tolerance, hardHumanizeCents);
    };
    addAndMakeVisible (naturalButton);
    addAndMakeVisible (hardButton);

    fillFromChoices (keyBox, state, ParamID::root);
    fillFromChoices (scaleBox, state, ParamID::scale);
    keyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, ParamID::root, keyBox);
    scaleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, ParamID::scale, scaleBox);
    addAndMakeVisible (keyBox);
    addAndMakeVisible (scaleBox);

    addAndMakeVisible (speed);
    addAndMakeVisible (humanize);
    addAndMakeVisible (meter);
    addAndMakeVisible (keyboard);
    addChildComponent (settings);

    setSize (designWidth, designHeight);
    refresh();
}

void MainPanel::setParameter (const juce::String& id, float value)
{
    if (auto* parameter = state.getParameter (id))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
        parameter->endChangeGesture();
    }
}

bool MainPanel::parameterIs (const juce::String& id, float value) const
{
    return std::abs (state.getRawParameterValue (id)->load() - value) < 0.05f;
}

void MainPanel::refresh()
{
    const auto& t = processor.telemetry();
    const int note = t.targetNote.load (std::memory_order_relaxed);
    const float cents = t.deviationCents.load (std::memory_order_relaxed);
    displayedCents = note >= 0 ? displayedCents + (cents - displayedCents) * 0.35f : 0.0f;

    meter.setReading (note, displayedCents);
    keyboard.refresh (note);
    settings.refresh();

    naturalButton.setToggleState (parameterIs (ParamID::speed, naturalSpeedMs) && parameterIs (ParamID::tolerance, naturalHumanizeCents),
                                  juce::dontSendNotification);
    hardButton.setToggleState (parameterIs (ParamID::speed, hardSpeedMs) && parameterIs (ParamID::tolerance, hardHumanizeCents),
                               juce::dontSendNotification);
}

void MainPanel::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.fillAll (colours::frame);
    const auto card = bounds.reduced (4.0f);
    g.setColour (colours::background);
    g.fillRoundedRectangle (card, 10.0f);
    g.setColour (colours::edge);
    g.drawRoundedRectangle (card, 10.0f, 1.0f);

    // Logo: a pitch curve settling onto a line, inside a ring.
    const juce::Point<float> logoCentre (34.0f, 27.0f);
    g.setColour (colours::accent);
    g.drawEllipse (juce::Rectangle<float> (26.0f, 26.0f).withCentre (logoCentre), 2.2f);
    juce::Path logo;
    const float lx = logoCentre.x - 7.0f, ly = logoCentre.y;
    logo.startNewSubPath (lx, ly + 4.0f);
    logo.cubicTo (lx + 3.0f, ly - 8.0f, lx + 6.0f, ly + 8.0f, lx + 10.0f, ly - 1.0f);
    logo.lineTo (lx + 14.0f, ly - 1.0f);
    g.strokePath (logo, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto logoFont = font (24.0f);
    g.setFont (logoFont);
    g.setColour (colours::text);
    g.drawText ("Vocal", juce::Rectangle<float> (56.0f, 0.0f, 80.0f, 54.0f), juce::Justification::centredLeft);
    g.setColour (colours::accent);
    g.drawText ("TuneBtw", juce::Rectangle<float> (56.0f + juce::GlyphArrangement::getStringWidth (logoFont, "Vocal"), 0.0f, 120.0f, 54.0f),
                juce::Justification::centredLeft);

    // Key / Scale pill.
    g.setColour (colours::panel);
    g.fillRoundedRectangle (keyScalePill, 6.0f);
    g.setColour (colours::edge);
    g.drawRoundedRectangle (keyScalePill, 6.0f, 1.0f);
    g.setColour (colours::text);
    g.setFont (font (15.0f));
    g.drawText ("Key", keyScalePill.withWidth (44.0f).translated (10.0f, 0.0f), juce::Justification::centredLeft);
    g.drawText ("Scale", juce::Rectangle<float> (static_cast<float> (scaleBox.getX()) - 48.0f, keyScalePill.getY(), 46.0f, keyScalePill.getHeight()),
                juce::Justification::centredLeft);

    // Hint above the keyboard.
    g.setColour (colours::textDim);
    g.setFont (font (13.0f));
    g.drawText ("Click a lit key to remove that note from the scale", juce::Rectangle<float> (0.0f, 340.0f, bounds.getWidth(), 18.0f),
                juce::Justification::centred);

    // Keyboard frame.
    const auto frame = keyboard.getBounds().toFloat().expanded (6.0f);
    g.setColour (colours::meterFace);
    g.fillRoundedRectangle (frame, 8.0f);
    g.setColour (colours::edge);
    g.drawRoundedRectangle (frame, 8.0f, 1.0f);
}

void MainPanel::resized()
{
    settingsButton.setBounds (designWidth - 82, 12, 28, 28);
    powerButton.setBounds (designWidth - 44, 12, 28, 28);

    naturalButton.setBounds (28, 70, 96, 30);
    hardButton.setBounds (132, 70, 96, 30);

    keyScalePill = { 412.0f, 70.0f, 320.0f, 30.0f };
    keyBox.setBounds (452, 70, 104, 30);
    scaleBox.setBounds (610, 70, 122, 30);

    speed.setBounds (40, 118, 200, 210);
    meter.setBounds (designWidth / 2 - 125, 106, 250, 236);
    humanize.setBounds (designWidth - 240, 118, 200, 210);

    keyboard.setBounds (16, 366, designWidth - 32, 104);
    settings.setBounds (20, 110, designWidth - 40, 232);
}
} // namespace pitchpilot::ui
