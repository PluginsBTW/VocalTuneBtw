# VocalTuneBtw architecture

VocalTuneBtw is a monophonic, real-time vocal pitch-correction plugin for Linux.
It builds VST3, CLAP and a Standalone app from one codebase.

## Layers

```
┌──────────────────────────────────────────────────────────────┐
│ Format wrappers   JUCE VST3 · JUCE Standalone · CLAP          │
│                   (clap-juce-extensions)                      │
├──────────────────────────────────────────────────────────────┤
│ Plugin layer      src/plugin   (JUCE)                         │
│   PluginProcessor  parameters (APVTS), state, bus layouts,    │
│                    latency reporting, telemetry atomics       │
│   PluginEditor     hosts src/ui/MainPanel (fixed-size UI)     │
├──────────────────────────────────────────────────────────────┤
│ DSP engine        src/dsp      (plain C++20, no JUCE)         │
│   PitchCorrector   facade: one prepare() + process() call     │
│     PitchDetector       YIN on a low-passed, decimated signal │
│     ScaleMapper         root + scale + note mask → targets    │
│     CorrectionController hysteresis, tolerance, strength,     │
│                          detune, speed smoothing              │
│     PitchShifter        pitch-synchronous splice delay line   │
│     bypass crossfade    against a latency-matched dry path    │
└──────────────────────────────────────────────────────────────┘
```

The DSP library knows nothing about JUCE, hosts or GUIs. The unit tests use it
directly, and it could later sit behind a different wrapper (for example a
native CLAP one) without changes.

## Signal flow per sample

1. Mix the input channels to mono for analysis.
2. `PitchDetector::push` runs two cascaded 2.5 kHz low-pass biquads and
   decimates to about 16 kHz. Every hop (about 2.5 ms) it runs YIN over a window
   sized to the selected voice range.
3. When a new estimate arrives, `CorrectionController::update` converts the
   frequency to continuous MIDI pitch, `m = 69 + 12·log2(f / ref)`. It then
   picks the nearest allowed note, with hysteresis, and computes the goal shift
   `strength · (target − m) + detune`.
4. `CorrectionController::nextShiftSemitones` smooths toward the goal with a
   one-pole filter. Its time constant is the **Correction speed** parameter.
5. `PitchShifter` reads one tap from a delay line, moving at `(1 − ratio)`
   samples per sample. When the tap leaves its delay range (one period of the
   range's lowest note, plus a margin), a second tap starts a whole number of
   detected periods away, and a 1 ms raised-cosine crossfade hands over to it. Period-aligned splices stay in phase, which avoids
   the cancellation and pitch error of a fixed two-tap design. Without a
   detected period (consonants, breaths, onsets the detector hasn't caught
   yet), the jump goes to wherever the last 2 ms of waveform match best,
   between half and a full delay range away. Neither tap ever reads closer
   than 1 sample to the write position.
6. The output crossfades between the shifted signal and a dry signal delayed by
   the shifter latency, over a 10 ms ramp, for click-free bypass.

## Threading and real-time rules

- `prepare()` is the only place that allocates. `process()` performs no
  allocation, locking, I/O or unbounded loops. `tests/test_realtime.cpp`
  enforces the no-allocation rule by overriding the global `operator new`.
- The audio thread reads parameters from APVTS `std::atomic<float>` values once
  per block.
- The audio thread publishes telemetry (detected Hz, target note, deviation,
  applied shift) through relaxed `std::atomic`s. The GUI polls them on a timer.
  Nothing blocks the audio thread.
- No LLM, network or file access happens inside the plugin. The LLM is used
  only as a development agent (see `CLAUDE.md`).

## Latency

| Stage | Latency | Reported to host |
|---|---|---|
| Pitch shifter | rest delay = crossfade headroom for shifts up to 3 semitones (12 samples, 0.25 ms at 48 kHz) + half of 1.05 × the longest period in the voice range: Low 10.8 ms, Normal 6.8 ms, High 3.8 ms (516 / 327 / 180 samples at 48 kHz). Bigger upward shifts start their splice earlier instead of raising the latency. The tap wanders within that range while shifting. | yes (rest delay; updated from the message thread when the range changes) |
| Pitch detection | about 2 periods of the lowest range note + one hop (Normal range: about 28 ms) | no: the correction arrives late, the audio is not delayed |

Detection lag means that note onsets reach their target about one detection
window late. Fixing this would need look-ahead, which would add latency.

## Parameters

| ID | Range | Default | Notes |
|---|---|---|---|
| `root` | C … B | C | 12 choices |
| `scale` | Chromatic, Major, Natural/Harmonic/Melodic Minor, 5 modes, Major/Minor Pentatonic, Blues, Custom | Major | Custom = all 12 notes |
| `note0`…`note11` | on/off | on | absolute pitch classes; effective mask = scale mask AND these |
| `speed` | 1 – 200 ms | 25 ms | smoothing time constant |
| `strength` | 0 – 100 % | 100 % | |
| `detune` | −100 … +100 cents | 0 | added after correction |
| `tolerance` | 0 – 50 cents | 0 | deviations smaller than this are not corrected |
| `reference` | 415 – 466 Hz | 440 Hz | A4 |
| `range` | Low (50–400 Hz), Normal (80–800 Hz), High (150–1500 Hz) | Normal | detector search range |
| `bypass` | on/off | off | exposed as the host bypass parameter |
