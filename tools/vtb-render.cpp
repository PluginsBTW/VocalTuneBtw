// vtb-render: run a WAV file through the VocalTuneBtw DSP engine offline and
// report pitch error, voiced fraction and clicks. No JUCE.
//
//   vtb-render input.wav [-o output.wav] [--root A] [--scale "natural minor"]
//              [--speed 25] [--strength 100] [--detune 0] [--tolerance 0]
//              [--reference 440] [--range normal] [--block 512]
//              [--threshold 2] [--clicks 20] [--repeat 1]

#include "AudioMetrics.h"
#include "Wav.h"
#include "dsp/PitchCorrector.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace pitchpilot;

namespace
{
[[noreturn]] void usage (const char* message = nullptr)
{
    if (message)
        std::fprintf (stderr, "vtb-render: %s\n\n", message);
    std::fprintf (stderr,
                  "usage: vtb-render input.wav [-o output.wav] [options]\n"
                  "  --root NOTE        C, C#, Db … B (default C)\n"
                  "  --scale NAME       chromatic, major, \"natural minor\", \"minor pentatonic\", … (default major)\n"
                  "  --speed MS         correction speed, 1–200 ms (default 25)\n"
                  "  --strength PCT     0–100 (default 100)\n"
                  "  --detune CENTS     −100…100 (default 0)\n"
                  "  --tolerance CENTS  0–50 (default 0)\n"
                  "  --reference HZ     A4, 415–466 (default 440)\n"
                  "  --range NAME       low, normal, high (default normal)\n"
                  "  --block N          host block size (default 512)\n"
                  "  --threshold X      click score above which a click is listed (default 2)\n"
                  "  --clicks N         list at most N clicks (default 20)\n"
                  "  --repeat N         render N times for timing; the fastest run is reported (default 1)\n");
    std::exit (2);
}

std::string normalised (std::string s)
{
    std::string out;
    for (char c : s)
        if (! std::isspace (static_cast<unsigned char> (c)) && c != '-' && c != '_')
            out += static_cast<char> (std::tolower (static_cast<unsigned char> (c)));
    return out;
}

int parseRoot (const std::string& text)
{
    static const char* names[] = { "c", "c#", "d", "d#", "e", "f", "f#", "g", "g#", "a", "a#", "b" };
    static const char* flats[] = { "c", "db", "d", "eb", "e", "f", "gb", "g", "ab", "a", "bb", "b" };
    const auto key = normalised (text);
    for (int i = 0; i < 12; ++i)
        if (key == names[i] || key == flats[i])
            return i;
    usage (("unknown root " + text).c_str());
}

dsp::Scale parseScale (const std::string& text)
{
    const auto key = normalised (text);
    for (int i = 0; i < static_cast<int> (dsp::Scale::Count); ++i)
    {
        const auto scale = static_cast<dsp::Scale> (i);
        if (normalised (dsp::scaleName (scale)) == key)
            return scale;
    }
    if (key == "minor")
        return dsp::Scale::NaturalMinor;
    usage (("unknown scale " + text).c_str());
}

dsp::VoiceRange parseRange (const std::string& text)
{
    const auto key = normalised (text);
    if (key == "low")
        return dsp::VoiceRange::Low;
    if (key == "normal")
        return dsp::VoiceRange::Normal;
    if (key == "high")
        return dsp::VoiceRange::High;
    usage (("unknown range " + text).c_str());
}

float parseNumber (const std::string& text)
{
    char* end = nullptr;
    const float value = std::strtof (text.c_str(), &end);
    if (end == text.c_str() || *end != '\0')
        usage (("not a number: " + text).c_str());
    return value;
}

std::vector<float> mono (const eval::Audio& audio)
{
    std::vector<float> out (audio.frames(), 0.0f);
    for (const auto& channel : audio.channels)
        for (size_t i = 0; i < out.size(); ++i)
            out[i] += channel[i] / static_cast<float> (audio.channels.size());
    return out;
}
} // namespace

int main (int argc, char** argv)
{
    std::string inputPath, outputPath;
    dsp::EngineParameters parameters;
    int blockSize = 512;
    float threshold = 2.0f;
    int maxClicks = 20;
    int repeats = 1;

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        const auto value = [&]() -> std::string {
            if (i + 1 >= argc)
                usage ((arg + " needs a value").c_str());
            return argv[++i];
        };
        if (arg == "-h" || arg == "--help")
            usage();
        else if (arg == "-o" || arg == "--output")
            outputPath = value();
        else if (arg == "--root")
            parameters.rootNote = parseRoot (value());
        else if (arg == "--scale")
            parameters.scale = parseScale (value());
        else if (arg == "--speed")
            parameters.speedMs = std::clamp (parseNumber (value()), 1.0f, 200.0f);
        else if (arg == "--strength")
            parameters.strength = std::clamp (parseNumber (value()), 0.0f, 100.0f) / 100.0f;
        else if (arg == "--detune")
            parameters.detuneCents = std::clamp (parseNumber (value()), -100.0f, 100.0f);
        else if (arg == "--tolerance")
            parameters.toleranceCents = std::clamp (parseNumber (value()), 0.0f, 50.0f);
        else if (arg == "--reference")
            parameters.referenceHz = std::clamp (parseNumber (value()), 415.0f, 466.0f);
        else if (arg == "--range")
            parameters.range = parseRange (value());
        else if (arg == "--block")
            blockSize = std::max (1, static_cast<int> (parseNumber (value())));
        else if (arg == "--threshold")
            threshold = parseNumber (value());
        else if (arg == "--clicks")
            maxClicks = std::max (0, static_cast<int> (parseNumber (value())));
        else if (arg == "--repeat")
            repeats = std::max (1, static_cast<int> (parseNumber (value())));
        else if (! arg.empty() && arg[0] == '-')
            usage (("unknown option " + arg).c_str());
        else if (inputPath.empty())
            inputPath = arg;
        else
            usage ("more than one input file");
    }
    if (inputPath.empty())
        usage ("no input file");

    eval::Audio input;
    try
    {
        input = eval::readWav (inputPath);
    }
    catch (const std::exception& e)
    {
        std::fprintf (stderr, "vtb-render: %s\n", e.what());
        return 1;
    }

    // Each run starts from a freshly prepared engine. Only engine.process is
    // timed, per block; the fastest run gives the totals, the slowest block
    // over all runs gives the worst case.
    using Clock = std::chrono::steady_clock;
    const int numChannels = static_cast<int> (input.channels.size());
    const size_t numBlocks = (input.frames() + static_cast<size_t> (blockSize) - 1) / static_cast<size_t> (blockSize);
    eval::Audio output;
    int latency = 0, spread = 0;
    double bestTotal = 0.0, worstBlock = 0.0;
    std::vector<double> blockTimes (numBlocks), bestBlockTimes;
    for (int run = 0; run < repeats; ++run)
    {
        output = input;
        dsp::PitchCorrector engine;
        engine.prepare (input.sampleRate, blockSize, numChannels);
        engine.setParameters (parameters);
        std::vector<float*> pointers (output.channels.size());
        double total = 0.0;
        for (size_t block = 0; block < numBlocks; ++block)
        {
            const auto offset = block * static_cast<size_t> (blockSize);
            const auto count = std::min (static_cast<size_t> (blockSize), output.frames() - offset);
            for (size_t ch = 0; ch < pointers.size(); ++ch)
                pointers[ch] = output.channels[ch].data() + offset;
            const auto start = Clock::now();
            engine.process (pointers.data(), numChannels, static_cast<int> (count));
            const double seconds = std::chrono::duration<double> (Clock::now() - start).count();
            blockTimes[block] = seconds;
            total += seconds;
            worstBlock = std::max (worstBlock, seconds);
        }
        if (run == 0 || total < bestTotal)
        {
            bestTotal = total;
            bestBlockTimes = blockTimes;
        }
        latency = engine.latencySamples();
        spread = engine.latencySpreadSamples();
    }
    std::sort (bestBlockTimes.begin(), bestBlockTimes.end());
    const double p99Block = bestBlockTimes.empty() ? 0.0 : bestBlockTimes[(bestBlockTimes.size() - 1) * 99 / 100];
    const double audioSeconds = static_cast<double> (input.frames()) / input.sampleRate;
    const double budget = blockSize / input.sampleRate;

    if (! outputPath.empty())
    {
        try
        {
            eval::writeWav (outputPath, output);
        }
        catch (const std::exception& e)
        {
            std::fprintf (stderr, "vtb-render: %s\n", e.what());
            return 1;
        }
    }

    const auto dryMono = mono (input), wetMono = mono (output);
    const auto mask = parameters.scale == dsp::Scale::Custom ? parameters.enabledNotes
                                                             : dsp::scaleMask (parameters.rootNote, parameters.scale);
    const auto limits = dsp::voiceRangeLimits (parameters.range);
    const auto before = eval::pitchError (dryMono, input.sampleRate, mask, limits.minHz, limits.maxHz, parameters.referenceHz);
    const auto after = eval::pitchError (wetMono, input.sampleRate, mask, limits.minHz, limits.maxHz, parameters.referenceHz);
    // The tap wanders around the latency, so look for the input's edges as
    // far away as it can stray.
    const double searchSeconds = std::max (0.01, spread / input.sampleRate);
    const auto clicks = eval::clickScores (dryMono, wetMono, input.sampleRate, latency, threshold, static_cast<size_t> (maxClicks),
                                           searchSeconds);

    std::printf ("file        %s\n", inputPath.c_str());
    std::printf ("format      %.0f Hz, %d ch, %.2f s, block %d\n", input.sampleRate, numChannels,
                 static_cast<double> (input.frames()) / input.sampleRate, blockSize);
    std::printf ("settings    root %d, scale %s, speed %.0f ms, strength %.0f %%, detune %.0f c, tolerance %.0f c, ref %.1f Hz, range %s\n",
                 parameters.rootNote, dsp::scaleName (parameters.scale), static_cast<double> (parameters.speedMs),
                 static_cast<double> (parameters.strength * 100.0f), static_cast<double> (parameters.detuneCents),
                 static_cast<double> (parameters.toleranceCents), static_cast<double> (parameters.referenceHz),
                 parameters.range == dsp::VoiceRange::Low ? "low" : parameters.range == dsp::VoiceRange::High ? "high" : "normal");
    std::printf ("latency     %d samples (%.2f ms)\n", latency, latency * 1000.0 / input.sampleRate);
    std::printf ("cpu         %.3f %% of real time (fastest of %d runs), %.2f µs per block mean, p99 %.2f µs, max %.2f µs; budget %.0f µs\n",
                 bestTotal / audioSeconds * 100.0, repeats, bestTotal / static_cast<double> (numBlocks) * 1e6, p99Block * 1e6,
                 worstBlock * 1e6, budget * 1e6);
    std::printf ("pitch error median %.1f c, p90 %.1f c in → median %.1f c, p90 %.1f c out (to the nearest allowed note)\n",
                 before.medianAbsCents, before.p90AbsCents, after.medianAbsCents, after.p90AbsCents);
    std::printf ("voiced      %.1f %% in → %.1f %% out (of %d detector hops)\n", before.voicedFraction * 100.0,
                 after.voicedFraction * 100.0, after.hops);
    std::printf ("click score max %.2f at %.4f s (threshold %.2f, input searched ±%.1f ms; ~1 = no worse than the input)\n",
                 static_cast<double> (clicks.maxScore), clicks.maxSeconds, static_cast<double> (threshold), searchSeconds * 1000.0);
    if (! clicks.clicks.empty())
    {
        std::printf ("clicks      %zu listed, output time (input time = output − latency):\n", clicks.clicks.size());
        for (const auto& click : clicks.clicks)
            std::printf ("  %9.4f s  (in %9.4f s)  score %6.2f\n", click.seconds,
                         click.seconds - latency / input.sampleRate, static_cast<double> (click.score));
    }
    return clicks.clicks.empty() ? 0 : 3;
}
