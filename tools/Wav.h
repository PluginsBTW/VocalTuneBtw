#pragma once

// Minimal WAV reading and writing for the offline tools: PCM 16/24/32-bit and
// 32/64-bit float, any channel count, WAVE_FORMAT_EXTENSIBLE included.

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace pitchpilot::eval
{
struct Audio
{
    double sampleRate = 48000.0;
    std::vector<std::vector<float>> channels; // planar

    size_t frames() const { return channels.empty() ? 0 : channels.front().size(); }
};

namespace detail
{
    inline std::uint32_t u32 (const unsigned char* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (std::uint32_t (p[3]) << 24); }
    inline std::uint16_t u16 (const unsigned char* p) { return static_cast<std::uint16_t> (p[0] | (p[1] << 8)); }

    inline void put32 (std::ofstream& out, std::uint32_t v)
    {
        const unsigned char b[] = { static_cast<unsigned char> (v), static_cast<unsigned char> (v >> 8),
                                    static_cast<unsigned char> (v >> 16), static_cast<unsigned char> (v >> 24) };
        out.write (reinterpret_cast<const char*> (b), 4);
    }
    inline void put16 (std::ofstream& out, std::uint16_t v)
    {
        const unsigned char b[] = { static_cast<unsigned char> (v), static_cast<unsigned char> (v >> 8) };
        out.write (reinterpret_cast<const char*> (b), 2);
    }
} // namespace detail

inline Audio readWav (const std::string& path)
{
    std::ifstream in (path, std::ios::binary);
    if (! in)
        throw std::runtime_error ("cannot open " + path);
    const std::vector<unsigned char> bytes ((std::istreambuf_iterator<char> (in)), std::istreambuf_iterator<char>());
    if (bytes.size() < 12 || std::memcmp (bytes.data(), "RIFF", 4) != 0 || std::memcmp (bytes.data() + 8, "WAVE", 4) != 0)
        throw std::runtime_error (path + " is not a RIFF/WAVE file");

    int format = 0, numChannels = 0, bits = 0;
    double sampleRate = 0.0;
    const unsigned char* data = nullptr;
    size_t dataSize = 0;
    for (size_t pos = 12; pos + 8 <= bytes.size();)
    {
        const unsigned char* chunk = bytes.data() + pos;
        const size_t size = detail::u32 (chunk + 4);
        const size_t available = std::min (size, bytes.size() - pos - 8);
        if (std::memcmp (chunk, "fmt ", 4) == 0 && available >= 16)
        {
            format = detail::u16 (chunk + 8);
            numChannels = detail::u16 (chunk + 10);
            sampleRate = detail::u32 (chunk + 12);
            bits = detail::u16 (chunk + 22);
            if (format == 0xFFFE && available >= 26)
                format = detail::u16 (chunk + 32); // first two bytes of the sub-format GUID
        }
        else if (std::memcmp (chunk, "data", 4) == 0)
        {
            data = chunk + 8;
            dataSize = available; // tolerate truncated files and size 0xFFFFFFFF from streaming writers
        }
        pos += 8 + size + (size & 1);
    }
    if (! data || numChannels <= 0 || sampleRate <= 0.0)
        throw std::runtime_error (path + ": missing fmt or data chunk");
    const bool isFloat = format == 3 && (bits == 32 || bits == 64);
    const bool isPcm = format == 1 && (bits == 16 || bits == 24 || bits == 32);
    if (! isFloat && ! isPcm)
        throw std::runtime_error (path + ": unsupported format " + std::to_string (format) + " / " + std::to_string (bits) + " bit");

    const size_t bytesPerSample = static_cast<size_t> (bits / 8);
    const size_t frames = dataSize / (bytesPerSample * static_cast<size_t> (numChannels));
    Audio audio;
    audio.sampleRate = sampleRate;
    audio.channels.assign (static_cast<size_t> (numChannels), std::vector<float> (frames));
    for (size_t f = 0; f < frames; ++f)
    {
        for (size_t ch = 0; ch < audio.channels.size(); ++ch)
        {
            const unsigned char* p = data + (f * audio.channels.size() + ch) * bytesPerSample;
            float value = 0.0f;
            if (isFloat && bits == 32)
            {
                const std::uint32_t raw = detail::u32 (p);
                std::memcpy (&value, &raw, 4);
            }
            else if (isFloat)
            {
                const std::uint64_t raw = detail::u32 (p) | (std::uint64_t (detail::u32 (p + 4)) << 32);
                double d;
                std::memcpy (&d, &raw, 8);
                value = static_cast<float> (d);
            }
            else if (bits == 16)
                value = static_cast<float> (static_cast<std::int16_t> (detail::u16 (p))) / 32768.0f;
            else if (bits == 24)
                value = static_cast<float> (static_cast<std::int32_t> ((std::uint32_t (p[0]) << 8) | (std::uint32_t (p[1]) << 16) | (std::uint32_t (p[2]) << 24)) >> 8) / 8388608.0f;
            else
                value = static_cast<float> (static_cast<double> (static_cast<std::int32_t> (detail::u32 (p))) / 2147483648.0);
            audio.channels[ch][f] = value;
        }
    }
    return audio;
}

// Writes 32-bit float.
inline void writeWav (const std::string& path, const Audio& audio)
{
    std::ofstream out (path, std::ios::binary);
    if (! out)
        throw std::runtime_error ("cannot write " + path);
    const auto numChannels = static_cast<std::uint16_t> (audio.channels.size());
    const auto dataSize = static_cast<std::uint32_t> (audio.frames() * numChannels * 4);
    const auto rate = static_cast<std::uint32_t> (audio.sampleRate);
    out.write ("RIFF", 4);
    detail::put32 (out, 36 + dataSize);
    out.write ("WAVEfmt ", 8);
    detail::put32 (out, 16);
    detail::put16 (out, 3);
    detail::put16 (out, numChannels);
    detail::put32 (out, rate);
    detail::put32 (out, rate * numChannels * 4u);
    detail::put16 (out, static_cast<std::uint16_t> (numChannels * 4));
    detail::put16 (out, 32);
    out.write ("data", 4);
    detail::put32 (out, dataSize);
    for (size_t f = 0; f < audio.frames(); ++f)
        for (const auto& channel : audio.channels)
        {
            std::uint32_t raw;
            std::memcpy (&raw, &channel[f], 4);
            detail::put32 (out, raw);
        }
    if (! out)
        throw std::runtime_error ("error writing " + path);
}
} // namespace pitchpilot::eval
