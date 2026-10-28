#pragma once

#include "dsp/PitchCorrector.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>

namespace pitchpilot
{
// IDs are persisted in saved state and host automation: never rename them.
namespace ParamID
{
    inline constexpr const char* root = "root";
    inline constexpr const char* scale = "scale";
    inline constexpr const char* speed = "speed";
    inline constexpr const char* strength = "strength";
    inline constexpr const char* detune = "detune";
    inline constexpr const char* tolerance = "tolerance";
    inline constexpr const char* reference = "reference";
    inline constexpr const char* range = "range";
    inline constexpr const char* bypass = "bypass";
    juce::String note (int pitchClass); // "note0" … "note11"
} // namespace ParamID

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

// Cached raw-value pointers, read on the audio thread once per block.
class ParameterReader
{
public:
    explicit ParameterReader (juce::AudioProcessorValueTreeState& state);
    dsp::EngineParameters read() const noexcept;

private:
    std::atomic<float>* root;
    std::atomic<float>* scale;
    std::atomic<float>* speed;
    std::atomic<float>* strength;
    std::atomic<float>* detune;
    std::atomic<float>* tolerance;
    std::atomic<float>* reference;
    std::atomic<float>* range;
    std::atomic<float>* bypass;
    std::array<std::atomic<float>*, 12> notes {};
};
} // namespace pitchpilot
