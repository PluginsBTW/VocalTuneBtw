#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace pitchpilot
{
PitchPilotProcessor::PitchPilotProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PitchPilot", createParameterLayout()),
      reader (parameters)
{
}

void PitchPilotProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumInputChannels());
    engine.setParameters (reader.read());
    engine.reset();
    setLatencySamples (engine.latencySamples());
}

bool PitchPilotProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void PitchPilotProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int channels = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels());
    for (int ch = channels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    engine.setParameters (reader.read());
    engine.process (buffer.getArrayOfWritePointers(), channels, buffer.getNumSamples());

    const auto& t = engine.telemetry();
    liveTelemetry.inputHz.store (t.inputHz, std::memory_order_relaxed);
    liveTelemetry.targetNote.store (t.targetNote, std::memory_order_relaxed);
    liveTelemetry.deviationCents.store (t.deviationCents, std::memory_order_relaxed);
    liveTelemetry.appliedShiftCents.store (t.appliedShiftCents, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* PitchPilotProcessor::createEditor()
{
    return new PitchPilotEditor (*this);
}

void PitchPilotProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PitchPilotProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorParameter* PitchPilotProcessor::getBypassParameter() const
{
    return parameters.getParameter (ParamID::bypass);
}
} // namespace pitchpilot

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new pitchpilot::PitchPilotProcessor();
}
