#pragma once

#include "Parameters.h"
#include "dsp/PitchCorrector.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace pitchpilot
{
// Written by the audio thread, polled by the editor. Relaxed atomics only.
struct Telemetry
{
    std::atomic<float> inputHz { 0.0f };
    std::atomic<int> targetNote { -1 };
    std::atomic<float> deviationCents { 0.0f };
    std::atomic<float> appliedShiftCents { 0.0f };
};

class PitchPilotProcessor final : public juce::AudioProcessor
{
public:
    PitchPilotProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override;

    juce::AudioProcessorValueTreeState& state() noexcept { return parameters; }
    const Telemetry& telemetry() const noexcept { return liveTelemetry; }

private:
    juce::AudioProcessorValueTreeState parameters;
    ParameterReader reader;
    dsp::PitchCorrector engine;
    Telemetry liveTelemetry;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchPilotProcessor)
};
} // namespace pitchpilot
