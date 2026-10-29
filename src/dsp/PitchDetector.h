#pragma once

#include <vector>

namespace pitchpilot::dsp
{
struct PitchEstimate
{
    float frequencyHz = 0.0f;
    float confidence = 0.0f; // 1 − YIN aperiodicity at the chosen lag
    bool voiced = false;
};

// Monophonic YIN pitch detector. The input is low-passed and decimated to
// about 16 kHz, then analysed every ~2.5 ms over a window sized to the current
// frequency range.
class PitchDetector
{
public:
    static constexpr float minSupportedHz = 50.0f;
    static constexpr float maxSupportedHz = 1500.0f;

    // Allocates. Call off the audio thread.
    void prepare (double sampleRate);
    void reset() noexcept;

    // Clamped to [minSupportedHz, maxSupportedHz]. Real-time safe.
    void setRange (float minHz, float maxHz) noexcept;
    void setThreshold (float yinThreshold) noexcept { threshold = yinThreshold; }

    // Returns true when this sample completed a hop and estimate() changed.
    bool push (float sample) noexcept;

    const PitchEstimate& estimate() const noexcept { return current; }
    double analysisRate() const noexcept { return rate; }

private:
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        double z1 = 0, z2 = 0;

        void setLowpass (double cutoffHz, double sampleRate) noexcept;
        double process (double x) noexcept
        {
            const double y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };

    void analyse() noexcept;

    double inputRate = 48000.0, rate = 16000.0;
    int decimation = 3, decimationCounter = 0;
    int hop = 80, hopCounter = 0;
    int maxLag = 0, minLag = 2, lagLimit = 0;
    float threshold = 0.15f;
    float rangeMinHz = 80.0f, rangeMaxHz = 800.0f;

    Biquad lowpass1, lowpass2;
    std::vector<float> ring, frame, difference, normalised;
    int writePos = 0, filled = 0;

    PitchEstimate current;
};
} // namespace pitchpilot::dsp
