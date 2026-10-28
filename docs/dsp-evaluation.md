# DSP evaluation

Objective measurements for milestone M2. Every number here comes from
`tools/vtb-render` or the tests in `tests/test_engine_quality.cpp`; rerun them
after any DSP change.

## Tools and metrics

`vtb-render input.wav [-o output.wav] [--root A --scale "natural minor" …]`
runs a WAV file through `dsp::PitchCorrector` in host-sized blocks, without
JUCE, and prints:

- **Pitch error**: the plugin's YIN detector runs on the input and on the
  output. For each voiced hop it measures the distance to the nearest note the
  scale allows, and prints the median and 90th percentile in cents.
- **Voiced fraction**: voiced hops divided by all hops, for input and output.
- **Click score**: at each output sample, the jump |x[n] − x[n−1]| divided by
  the RMS of the jumps within ±5 ms (excluding ±2 samples around n). That value
  is then divided by the largest such score of the input within ±10 ms of the
  same moment (output time − latency), with the input term floored at √2.
  About 1 means "no sharper than the input". Events above 2 are listed with
  timestamps. The exit code is 3 when any are listed.

What the click score can and cannot see (measured on `test::vocalPhrase`, a
pulse train rolled off at −12 dB/oct with vibrato, glides, rests and noise
bursts):

| Shifter variant | Worst score over the test matrix |
|---|---|
| v0.1.1 (shipped) | 1.44 |
| `0e25b15` (the reverted latency cut) | ≤ 1.98, no clicks |
| v0.1.1 with the 2 ms crossfade removed | 2.2 – 5.5, fails 10 of 24 cases |
| v0.1.1 with splices not period-aligned (crossfade kept) | 1.37, no clicks |

So the score catches hard discontinuities. It does not catch the cancellation
dips that a crossfade between tap positions out of phase with each other would
cause. A plain sawtooth is a poor test input: it is nearly all edge, and the
shifter's linear interpolation softens its top octave, so a clean render
scores about 6.

## Open: the v0.1.0 popping

`0e25b15` (latency −1.6 ms) popped on real vocals in Bitwig. Findings so far:

- With normal settings the shift stays below 3 semitones (at most about 1.5
  from scale correction, plus up to 1 of detune). In that range `0e25b15`
  differs from v0.1.1 only by a lower minimum delay (21 instead of 98 samples
  at 48 kHz). Its other two changes, the earlier splice for large upward
  shifts and the clamp on the fading tap, only act above 3 semitones.
- On the synthetic phrase, with shifts forced up to ±6 semitones and ±100 cents
  of detune, `0e25b15` produces no click by the measure above.
- So the pop is either something the synthetic input lacks, or not a sample
  discontinuity at all. Next: render the user's recording of the popping passage
  through both versions and look at the waveform at the reported time.

## Baseline on the synthetic phrase (v0.1.1, 48 kHz, block 512, Normal range)

| Settings | Pitch error in → out (median / p90, cents) | Voiced in → out | Click score |
|---|---|---|---|
| A natural minor, speed 5 ms | 30.0 / 61.1 → 7.2 / 23.1 | 95.3 % → 95.0 % | 1.35 |
| A natural minor, speed 25 ms | 30.0 / 61.1 → 12.3 / 41.9 | 95.3 % → 95.2 % | 1.42 |
| A minor pentatonic, speed 1 ms | 41.1 / 88.4 → 5.5 / 26.1 | 95.3 % → 95.4 % | 1.35 |

## Real vocal: `dry.wav` (user recording, not in the repo)

10.9 s, 44.1 kHz, 24-bit, dual mono, low male voice (detected E2 to G♯3). The
user's Bitwig export of the same take (`wet.wav`) matches an offline render
with root C, natural minor, Normal range, speed 1 ms best (correlation 0.71;
chromatic: 0.08–0.13; Low range: 0.62 at most). Block 512.

| Settings | Pitch error in → out (median / p90, cents) | Voiced in → out | Click score |
|---|---|---|---|
| C natural minor, speed 1 ms | 34.4 / 84.2 → 8.2 / 41.9 | 49.3 % → 48.5 % | 1.82 (at 0.009 s, in silence) |
| C natural minor, speed 25 ms | 34.4 / 84.2 → 19.4 / 61.2 | 49.3 % → 48.9 % | 1.82 (same) |

Popping check, v0.1.1 against `0e25b15`, at speeds 1/10/25 ms and blocks 128/512:

- Click score: at most 1.82 in both versions, in every case, and it falls in
  the silence at the start. No clicks.
- Level: 12 ms RMS of the output compared with the dry signal ±8 ms around the
  same moment. Worst dip −1.6 dB (v0.1.1) and −1.7 dB (`0e25b15`). Worst bump
  +0.2 and +0.3 dB. The Bitwig export: −1.5 / +0.4 dB. The dips line up across
  all three, so they come from the material, not from either version.

On this take, offline, the latency cut renders as cleanly as v0.1.1. Still
open: which build made `wet.wav`, and when the pop is heard (in this take or
not, in the export or only live).
