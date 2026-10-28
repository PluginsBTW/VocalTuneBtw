#pragma once

#include "CorrectionController.h"
#include "PitchDetector.h"
#include "PitchShifter.h"
#include "Scales.h"

#include <vector>

namespace pitchpilot::dsp
{
// Order is persisted in plugin state: append only.
enum class VoiceRange : int
{
    Low,
    Normal,
    High,
    Count
};

struct RangeLimits
{
    float minHz, maxHz;
};

RangeLimits voiceRangeLimits (VoiceRange range) noexcept;

struct EngineParameters
{
    int rootNote = 0;
    Scale scale = Scale::Major;
    NoteMask enabledNotes = allNotes;
    float speedMs = 25.0f;
    float strength = 1.0f;
    float detuneCents = 0.0f;
    float toleranceCents = 0.0f;
    float referenceHz = 440.0f;
    VoiceRange range = VoiceRange::Normal;
    bool bypass = false;
};

struct EngineTelemetry
{
    float inputHz = 0.0f;
    bool voiced = false;
    int targetNote = -1;          // MIDI note, −1 when none
    float deviationCents = 0.0f;  // input relative to target
    float appliedShiftCents = 0.0f;
};

// The complete real-time chain: detection → mapping → correction → shifting,
// plus a click-free bypass crossfade against a latency-matched dry path.
class PitchCorrector
{
public:
    // Allocates. Call off the audio thread.
    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset() noexcept;

    // Real-time safe. Call before process() whenever parameters may have changed.
    void setParameters (const EngineParameters& parameters) noexcept;

    // In-place, real-time safe.
    void process (float* const* channels, int numChannels, int numSamples) noexcept;

    int latencySamples() const noexcept { return shifter.latencySamples(); }
    const EngineTelemetry& telemetry() const noexcept { return stats; }

private:
    void processChunk (float* const* channels, int numChannels, int offset, int numSamples) noexcept;

    PitchDetector detector;
    ScaleMapper mapper;
    CorrectionController controller;
    PitchShifter shifter;

    std::vector<float> ratios;
    std::vector<float*> chunkPointers;
    std::vector<std::vector<float>> dryLines;
    double sampleRate = 48000.0;
    int dryMask = 0, dryWritePos = 0;
    int maxBlock = 0, preparedChannels = 0;

    float bypassMix = 0.0f, bypassTarget = 0.0f, bypassStep = 0.0f;
    VoiceRange currentRange = VoiceRange::Count;
    EngineTelemetry stats;
};
} // namespace pitchpilot::dsp
