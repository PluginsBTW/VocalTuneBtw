#include "PitchCorrector.h"

#include "PitchMath.h"

#include <algorithm>
#include <bit>
#include <cmath>

namespace pitchpilot::dsp
{
namespace
{
    constexpr double bypassFadeSeconds = 0.01;
}

RangeLimits voiceRangeLimits (VoiceRange range) noexcept
{
    switch (range)
    {
        case VoiceRange::Low:    return { 50.0f, 400.0f };
        case VoiceRange::Normal: return { 80.0f, 800.0f };
        case VoiceRange::High:   return { 150.0f, 1500.0f };
        case VoiceRange::Count:  break;
    }
    return { 80.0f, 800.0f };
}

void PitchCorrector::prepare (double newSampleRate, int maxBlockSize, int numChannels)
{
    sampleRate = newSampleRate;
    maxBlock = std::max (1, maxBlockSize);
    preparedChannels = std::max (1, numChannels);

    detector.prepare (sampleRate);
    controller.prepare (sampleRate);
    shifter.prepare (sampleRate, preparedChannels);

    ratios.assign (static_cast<size_t> (maxBlock), 1.0f);
    chunkPointers.assign (static_cast<size_t> (preparedChannels), nullptr);

    // A whole chunk is written before it is read back `latency` samples late.
    const auto drySize = std::bit_ceil (static_cast<unsigned> (latencySamples() + maxBlock) + 1u);
    dryMask = static_cast<int> (drySize) - 1;
    dryLines.assign (static_cast<size_t> (preparedChannels), std::vector<float> (drySize, 0.0f));

    bypassStep = static_cast<float> (1.0 / (bypassFadeSeconds * sampleRate));
    currentRange = VoiceRange::Count;
    reset();
}

void PitchCorrector::reset() noexcept
{
    detector.reset();
    controller.reset();
    shifter.reset();
    for (auto& line : dryLines)
        std::fill (line.begin(), line.end(), 0.0f);
    dryWritePos = 0;
    bypassMix = bypassTarget;
    stats = {};
}

void PitchCorrector::setParameters (const EngineParameters& p) noexcept
{
    const auto base = p.scale == Scale::Custom ? allNotes : scaleMask (p.rootNote, p.scale);
    mapper.setMask (static_cast<NoteMask> (base & p.enabledNotes));

    controller.setSettings ({ p.speedMs, p.strength, p.detuneCents, p.toleranceCents, p.referenceHz });

    if (p.range != currentRange)
    {
        const auto limits = voiceRangeLimits (p.range);
        detector.setRange (limits.minHz, limits.maxHz);
        currentRange = p.range;
    }

    bypassTarget = p.bypass ? 1.0f : 0.0f;
}

void PitchCorrector::process (float* const* channels, int numChannels, int numSamples) noexcept
{
    const int channelsToProcess = std::min (numChannels, preparedChannels);
    for (int offset = 0; offset < numSamples; offset += maxBlock)
        processChunk (channels, channelsToProcess, offset, std::min (maxBlock, numSamples - offset));
}

void PitchCorrector::processChunk (float* const* channels, int numChannels, int offset, int numSamples) noexcept
{
    if (numChannels <= 0)
        return;

    const float channelScale = 1.0f / static_cast<float> (numChannels);
    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            mono += channels[ch][offset + i];
        if (detector.push (mono * channelScale))
        {
            const auto& estimate = detector.estimate();
            controller.update (estimate, mapper);
            shifter.setPeriodHint (estimate.voiced ? static_cast<float> (sampleRate / estimate.frequencyHz) : 0.0f);
        }
        ratios[static_cast<size_t> (i)] = static_cast<float> (semitonesToRatio (controller.nextShiftSemitones()));
    }

    // Keep a latency-aligned dry copy for the bypass crossfade.
    const int latency = latencySamples();
    const int startWrite = dryWritePos;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto& line = dryLines[static_cast<size_t> (ch)];
        for (int i = 0, w = startWrite; i < numSamples; ++i, w = (w + 1) & dryMask)
            line[static_cast<size_t> (w)] = channels[ch][offset + i];
        chunkPointers[static_cast<size_t> (ch)] = channels[ch] + offset;
    }

    shifter.process (chunkPointers.data(), numChannels, numSamples, ratios.data());

    const bool fading = bypassMix != bypassTarget;
    if (fading || bypassMix > 0.0f)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            if (bypassMix < bypassTarget)
                bypassMix = std::min (bypassTarget, bypassMix + bypassStep);
            else if (bypassMix > bypassTarget)
                bypassMix = std::max (bypassTarget, bypassMix - bypassStep);

            const int r = (startWrite + i - latency) & dryMask;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float& sample = channels[ch][offset + i];
                const float dry = dryLines[static_cast<size_t> (ch)][static_cast<size_t> (r)];
                sample += bypassMix * (dry - sample);
            }
        }
    }
    dryWritePos = (startWrite + numSamples) & dryMask;

    const auto& estimate = detector.estimate();
    stats.inputHz = estimate.voiced ? estimate.frequencyHz : 0.0f;
    stats.voiced = controller.hasTarget();
    stats.targetNote = controller.hasTarget() ? controller.targetNote() : -1;
    stats.deviationCents = controller.hasTarget()
                               ? static_cast<float> ((controller.inputPitch() - controller.targetNote()) * 100.0)
                               : 0.0f;
    stats.appliedShiftCents = controller.currentShiftSemitones() * 100.0f;
}
} // namespace pitchpilot::dsp
