#include "Parameters.h"

namespace pitchpilot
{
namespace
{
    constexpr int parameterVersion = 1;

    const juce::StringArray noteNames { "C", "C#/Db", "D", "D#/Eb", "E", "F", "F#/Gb", "G", "G#/Ab", "A", "A#/Bb", "B" };

    juce::ParameterID id (const juce::String& name) { return { name, parameterVersion }; }

    juce::StringArray scaleNames()
    {
        juce::StringArray names;
        for (int s = 0; s < static_cast<int> (dsp::Scale::Count); ++s)
            names.add (dsp::scaleName (static_cast<dsp::Scale> (s)));
        return names;
    }

    std::atomic<float>* raw (juce::AudioProcessorValueTreeState& state, const juce::String& paramId)
    {
        auto* p = state.getRawParameterValue (paramId);
        jassert (p != nullptr);
        return p;
    }
} // namespace

juce::String ParamID::note (int pitchClass)
{
    return "note" + juce::String (pitchClass);
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using Float = juce::AudioParameterFloat;
    using Attributes = juce::AudioParameterFloatAttributes;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (id (ParamID::root), "Root Note", noteNames, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (id (ParamID::scale), "Scale", scaleNames(),
                                                              static_cast<int> (dsp::Scale::Major)));

    juce::NormalisableRange<float> speedRange { 1.0f, 200.0f, 0.1f };
    speedRange.setSkewForCentre (25.0f);
    layout.add (std::make_unique<Float> (id (ParamID::speed), "Correction Speed", speedRange, 25.0f,
                                         Attributes().withLabel ("ms")));
    layout.add (std::make_unique<Float> (id (ParamID::strength), "Strength", juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f },
                                         100.0f, Attributes().withLabel ("%")));
    layout.add (std::make_unique<Float> (id (ParamID::detune), "Detune", juce::NormalisableRange<float> { -100.0f, 100.0f, 0.1f },
                                         0.0f, Attributes().withLabel ("cents")));
    layout.add (std::make_unique<Float> (id (ParamID::tolerance), "Tolerance", juce::NormalisableRange<float> { 0.0f, 50.0f, 0.1f },
                                         0.0f, Attributes().withLabel ("cents")));
    layout.add (std::make_unique<Float> (id (ParamID::reference), "Reference A4", juce::NormalisableRange<float> { 415.0f, 466.0f, 0.1f },
                                         440.0f, Attributes().withLabel ("Hz")));
    layout.add (std::make_unique<juce::AudioParameterChoice> (id (ParamID::range), "Voice Range",
                                                              juce::StringArray { "Low", "Normal", "High" },
                                                              static_cast<int> (dsp::VoiceRange::Normal)));
    layout.add (std::make_unique<juce::AudioParameterBool> (id (ParamID::bypass), "Bypass", false));

    for (int pc = 0; pc < 12; ++pc)
        layout.add (std::make_unique<juce::AudioParameterBool> (id (ParamID::note (pc)), "Note " + noteNames[pc], true));

    return layout;
}

ParameterReader::ParameterReader (juce::AudioProcessorValueTreeState& state)
    : root (raw (state, ParamID::root)),
      scale (raw (state, ParamID::scale)),
      speed (raw (state, ParamID::speed)),
      strength (raw (state, ParamID::strength)),
      detune (raw (state, ParamID::detune)),
      tolerance (raw (state, ParamID::tolerance)),
      reference (raw (state, ParamID::reference)),
      range (raw (state, ParamID::range)),
      bypass (raw (state, ParamID::bypass))
{
    for (int pc = 0; pc < 12; ++pc)
        notes[static_cast<size_t> (pc)] = raw (state, ParamID::note (pc));
}

dsp::EngineParameters ParameterReader::read() const noexcept
{
    dsp::NoteMask enabled = 0;
    for (int pc = 0; pc < 12; ++pc)
        if (notes[static_cast<size_t> (pc)]->load (std::memory_order_relaxed) >= 0.5f)
            enabled = static_cast<dsp::NoteMask> (enabled | (1u << pc));

    const auto load = [] (const std::atomic<float>* p) { return p->load (std::memory_order_relaxed); };
    return {
        .rootNote = juce::roundToInt (load (root)),
        .scale = static_cast<dsp::Scale> (juce::jlimit (0, static_cast<int> (dsp::Scale::Count) - 1, juce::roundToInt (load (scale)))),
        .enabledNotes = enabled,
        .speedMs = load (speed),
        .strength = load (strength) * 0.01f,
        .detuneCents = load (detune),
        .toleranceCents = load (tolerance),
        .referenceHz = load (reference),
        .range = static_cast<dsp::VoiceRange> (juce::jlimit (0, static_cast<int> (dsp::VoiceRange::Count) - 1, juce::roundToInt (load (range)))),
        .bypass = load (bypass) >= 0.5f,
    };
}
} // namespace pitchpilot
