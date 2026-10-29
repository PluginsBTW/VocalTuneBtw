# DSP evaluation

Objective measurements of the DSP engine. Every number here comes from
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
  is then divided by the largest such score of the input near the same moment
  (output time − latency), with the input term floored at √2. "Near" is
  ±`latencySpreadSamples()`, how far the shifter's tap can stray from the
  latency, and at least ±10 ms (before 2026-10-29 it was always ±10 ms).
  About 1 means "no sharper than the input". Events above 2 are listed with
  timestamps. The exit code is 3 when any are listed.
- **CPU**: time spent in `engine.process` only, as a share of real time, and
  µs per block (mean, p99, max) against the block's duration. `--repeat N`
  renders N times from a fresh engine and reports the fastest run.

What the click score can and cannot see (measured on `test::vocalPhrase`, a
pulse train rolled off at −12 dB/oct with vibrato, glides, rests and noise
bursts):

| Shifter variant | Worst score over the test matrix |
|---|---|
| v0.1.1 – v0.2.0 | 1.44 |
| `0e25b15` + 1 ms crossfade (current) | 1.39 |
| `0e25b15` (the reverted latency cut) | ≤ 1.98, no clicks |
| v0.1.1 with the 2 ms crossfade removed | 2.2 – 5.5, fails 10 of 24 cases |
| v0.1.1 with splices not period-aligned (crossfade kept) | 1.37, no clicks |

So the score catches hard discontinuities. It does not catch the cancellation
dips that a crossfade between tap positions out of phase with each other would
cause. A plain sawtooth is a poor test input: it is nearly all edge, and the
shifter's linear interpolation softens its top octave, so a clean render
scores about 6.

## The v0.1.0 popping (closed)

`0e25b15` (latency −1.6 ms) was reverted after pops were heard on real vocals
in Bitwig. Offline, neither the synthetic phrase (shifts forced up to ±6
semitones, ±100 cents of detune) nor the user's take showed a click. On
2026-10-29 the user confirmed that none of the takes popped and put it down to
a user error, so the change was landed again (see "Latency cut" below).

## Baseline on the synthetic phrase (v0.1.1, 48 kHz, block 512, Normal range)

| Settings | Pitch error in → out (median / p90, cents) | Voiced in → out | Click score |
|---|---|---|---|
| A natural minor, speed 5 ms | 30.0 / 61.1 → 7.2 / 23.1 | 95.3 % → 95.0 % | 1.35 |
| A natural minor, speed 25 ms | 30.0 / 61.1 → 12.3 / 41.9 | 95.3 % → 95.2 % | 1.42 |
| A minor pentatonic, speed 1 ms | 41.1 / 88.4 → 5.5 / 26.1 | 95.3 % → 95.4 % | 1.35 |

## Real vocal: `dry.wav` (user recording, not in the repo, v0.1.1 shifter)

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

## CPU

Measured 2026-10-29 on the v0.1.1 shifter, with `vtb-render --repeat 20` on `dry.wav` (44.1 kHz,
2 channels, 10.9 s), C natural minor, speed 1 ms. Only `engine.process` is
timed, per block, with `std::chrono::steady_clock`. Release build (`-O3`),
GCC, offline, not inside a host. CPU: Intel Core i7-14700K (hybrid: CPUs 0–15
are P-core threads up to 5.5 GHz, 16–27 E-cores up to 4.3 GHz). Each run was
pinned with `taskset` to CPU 2 (P-core) or CPU 20 (E-core). The process ran
at normal priority on a desktop under light load.

```bash
taskset -c 2 build-tests/tools/vtb-render dry.wav --root C --scale "natural minor" --speed 1 --range normal --block 128 --repeat 20
```

**Average load**, as a share of one core in real time (fastest of 20 runs).
It doesn't depend on the block size (spread across blocks 32–512 ≤ 0.01
percentage points on the P-core, ≤ 0.02 on the E-core):

| Range | P-core | E-core |
|---|---|---|
| Low | 1.05 % | 2.37 % |
| Normal | 0.44 % | 1.15 % |
| High | 0.17 % | 0.47 % |

After the latency cut, block 32 on both cores gives the same load and p99 to
within 0.01 percentage points and 0.2 µs.

Low costs about 2.4 × Normal, close to the O(window × lags) cost of the YIN
difference function (longer window, more lags).

**Per block**, in µs: mean and p99 of the fastest run, max over all 20 runs.
The budget is the block's duration at 44.1 kHz.

| Range | Block | Budget | P-core mean / p99 / max | E-core mean / p99 / max |
|---|---|---|---|---|
| Low | 32 | 726 | 7.7 / 65.9 / 94.8 | 17.3 / 140.7 / 172.2 |
| Low | 64 | 1451 | 15.3 / 67.2 / 94.8 | 34.5 / 148.0 / 160.1 |
| Low | 128 | 2902 | 30.5 / 69.8 / 93.2 | 68.9 / 164.3 / 183.6 |
| Low | 256 | 5805 | 61.1 / 139.1 / 215.0 | 137.7 / 325.0 / 352.3 |
| Low | 512 | 11610 | 122.1 / 214.1 / 219.0 | 274.8 / 516.9 / 653.4 |
| Normal | 32 | 726 | 3.2 / 25.1 / 42.5 | 8.4 / 58.8 / 88.5 |
| Normal | 64 | 1451 | 6.4 / 26.3 / 51.3 | 16.7 / 66.0 / 77.7 |
| Normal | 128 | 2902 | 12.9 / 28.9 / 44.3 | 33.3 / 80.5 / 1901.2 * |
| Normal | 256 | 5805 | 25.7 / 57.6 / 59.9 | 66.6 / 160.8 / 171.9 |
| Normal | 512 | 11610 | 51.3 / 91.5 / 95.0 | 133.0 / 270.1 / 287.0 |
| High | 32 | 726 | 1.3 / 7.8 / 16.8 | 3.4 / 21.4 / 37.0 |
| High | 64 | 1451 | 2.5 / 9.1 / 23.6 | 6.8 / 28.7 / 47.2 |
| High | 128 | 2902 | 5.0 / 11.6 / 30.2 | 13.6 / 43.1 / 54.4 |
| High | 256 | 5805 | 9.9 / 23.1 / 26.2 | 27.1 / 86.1 / 90.6 |
| High | 512 | 11610 | 19.8 / 39.7 / 43.1 | 54.3 / 158.2 / 162.1 |

\* Preemption by the OS, not the engine: three more runs of the same case gave
max 1727, 97 and 87 µs with the same p99 (80.5 µs). The engine is
deterministic on the same input, so a spike it caused would recur in every
run.

What this shows:

- The cost per block is uneven because detection runs once per 5 ms hop.
  At small blocks the p99 and max are the blocks that contain a hop, so they
  barely change from block 32 to 128. The worst share of the budget is at
  block 32: Low range, max 94.8 of 726 µs (13 %) on a P-core, 172.2 µs (24 %)
  on an E-core.
- Even the heaviest setting stays far below one core, so an FFT-based YIN
  isn't needed for CPU. Spreading the hop
  across blocks would only flatten the per-block peaks.
- Not measured: load inside a host (needs Bitwig), other sample rates and
  other CPUs.

## Latency cut: `0e25b15` + 1 ms crossfade

The rest delay is `minDelay` + half of 1.05 × the lowest period of the range.
Only `minDelay`, the crossfade headroom, can shrink without changing the voice
ranges. Four variants were compared with v0.1.1 (v0):

| Variant | `minDelay` (44.1 kHz) | Normal latency (44.1 / 48 kHz) |
|---|---|---|
| v0: 2 ms fade, headroom for a full octave | 90 | 380 / 413 samples (8.62 / 8.60 ms) |
| v1: `0e25b15` (headroom for 3 semitones, larger upward shifts splice earlier) | 19 | 309 / 336 (7.01 / 7.00 ms) |
| v2: v1 with no fixed headroom | 2 | 292 / 317 (6.62 / 6.60 ms) |
| v3: v0 with a 1 ms fade | 46 | 336 / 365 (7.62 / 7.60 ms) |
| **v4: v1 with a 1 ms fade (shipped)** | 11 | 301 / 327 (6.83 / 6.81 ms) |

v4 at 48 kHz: Low 516 samples (10.75 ms), Normal 327 (6.81 ms), High 180
(3.75 ms), asserted in `tests/test_realtime.cpp`.

Checks on `dry.wav`, 72 settings per variant (Normal and Low; natural minor,
minor pentatonic, chromatic; speed 1/10/25 ms; blocks 64/512; detune 0 and
+100 cents). Events are click-score events after the first 50 ms; "new" means
no v0 event within 5 ms of the same input time.

| Variant | Events, ±10 ms reference | New vs v0 (max) | Events, ±20 ms reference | New vs v0 (max) |
|---|---|---|---|---|
| v0 | 32 | – | 1 | – |
| v1 | 36 | 6 (4.39) | 2 | 1 (3.56) |
| v2 | 45 | 17 (15.86) | – | – |
| v3 | 32 | 2 (2.84) | 1 | 0 |
| v4 | 36 | 6 (3.41) | 4 | 3 (2.99) |

- At detune 0, every variant gives the same events as v0. All differences
  appear at detune +100 cents, where the tap sweeps all the time.
- The click score compares the output with the input within ±10 ms of
  output − latency. While shifting, the tap wanders by up to half the delay
  range (±10.5 ms in Low), so input edges fall outside that window and are
  scored as clicks. With ±20 ms almost all events disappear, in v0 as well.
- Every remaining event sits within 1.5 ms after a splice made with no period
  hint (period 0: the detector has no pitch), so the jump is a full range and
  not period-aligned. 44 % of all splices on this take are like that, in
  every variant. None of the events is at a period-aligned splice, and the
  fading tap never reached the write-position clamp. A spot at 7.70 s is
  flagged in v0, v1, v3 and v4.
- v2 is rejected: new events of 15.9 and 9.5. It would save 0.2 ms more than v4.
- Level (12 ms RMS against the dry signal ±8 ms around the same moment): v1,
  v3 and v4 are within 0.12 dB of v0 in all 10 settings checked.
- Synthetic: the vocal-like click test passes (worst 1.39). Steady and
  jumping upward shifts up to 12 semitones show no steps above the sine slope
  and no level dips.

**Cost: pitch accuracy at fast speeds.** Pitch error out on `dry.wav` (median
/ p90 cents, block 512):

| Settings | v0 | v4 | v4 with v0's delay |
|---|---|---|---|
| Normal, natural minor, speed 1 ms | 8.2 / 41.9 | 9.8 / 47.0 | 8.2 / 41.9 |
| Normal, minor pentatonic, speed 1 ms | 8.3 / 45.2 | 9.9 / 49.9 | 8.3 / 45.2 |
| Normal, natural minor, speed 25 ms | 19.4 / 61.2 | 20.3 / 63.0 | 19.4 / 61.2 |
| Low, natural minor, speed 1 ms | 12.8 / 47.5 | 14.1 / 49.7 | 12.8 / 47.5 |

Adding the 79 samples back (same splicing, v0's delay) restores v0's numbers
exactly, so the loss comes from the shorter delay itself: the detector's
estimate describes audio from further back than the audio now being shifted.
v1 and v3 lose about the same in proportion to their cut.

Still needed before release: the user's listening test in Bitwig.

## After 1.0.1: splices without a period, a read-ahead fix, 2.5 ms hops

Measured 2026-10-29 against the 1.0.1 shifter, both with the click window
from `latencySpreadSamples()`. `dry.wav`, 162 settings per version: Low,
Normal and High; natural minor, minor pentatonic, chromatic; speed 1/10/25 ms;
blocks 64/512; detune 0, +100 and −100 cents.

**Splices without a period.** When the detector has no pitch (consonants,
breaths, onsets it hasn't caught yet), the shifter used to jump a full delay
range, which is rarely a whole number of periods. On a 220 Hz sine with no
period hint, that made each crossfade dip by about 12 dB
(`tests/test_pitch_shifter.cpp`, "Splices without a period hint stay in
phase"; measured by demodulating at the output frequency over one period).
The shifter now compares the last 2 fade lengths before the current tap with
every jump between half and a full range and takes the best match. The dip
falls to at most 0.03 dB, the same as with a correct hint.

**Read-ahead fix.** A downward splice lands the new tap near `minDelay`
(11 samples at 44.1 kHz since 1.0.0). If the ratio then jumps far upward
during the 1 ms fade (more than about 3.6 semitones up right after shifting
down, as a sparse Custom scale at speed 1 ms can do), the new tap moved past
the write position and read samples a whole buffer old. 1.0.0 and 1.0.1 do
this; 0.2.0 didn't (its headroom covered a full fade). Found by "The delay
stays within latencySpreadSamples() of the latency". Both taps are now
clamped to at least 1 sample.

| `dry.wav`, 162 settings | 1.0.1 | Period-matched splices | + 2.5 ms hop (current) |
|---|---|---|---|
| Click events, detune 0 | 0 | 0 | 0 |
| Click events, detune ±100 | 18 in 17 runs, max 3.83 | 4, max 3.47 | 6, max 3.41 |

The two largest remaining events (3.41 in High at 10.28 s, 2.83 in Normal at
4.02 s) are low-level: jumps of about −59 dBFS just before a note starts,
where the input sits at −70 to −74 dBFS. The other four (2.07–2.12) weren't
inspected. The
vocal-like click test's worst score is 1.44 (1.0.1: 1.39). A level check
(12 ms RMS against the dry signal within the tap's spread) finds the same
worst spot in every version, the end of a note at 0.48 s, so it shows no
consistent difference.

**2.5 ms detector hop.** Pitch error out on `dry.wav`, block 512, measured
on both outputs with the 5 ms detector so the measuring tool is the same:

| Settings | 1.0.1 (5 ms hop) | 2.5 ms hop | 0.2.0 |
|---|---|---|---|
| Normal, natural minor, speed 1 ms | 9.8 / 47.0 | 9.2 / 44.9 | 8.2 / 41.9 |
| Normal, natural minor, speed 10 ms | 14.8 / 55.8 | 13.7 / 56.7 | – |
| Normal, natural minor, speed 25 ms | 20.3 / 63.0 | 19.3 / 62.6 | 19.4 / 61.2 |
| Normal, minor pentatonic, speed 1 ms | 9.9 / 49.9 | 8.7 / 44.3 | 8.3 / 45.2 |
| Low, natural minor, speed 1 ms | 14.1 / 49.7 | 13.2 / 48.2 | 12.8 / 47.5 |
| High, natural minor, speed 1 ms | 10.8 / 30.7 | 8.9 / 28.7 | – |

(median / p90 cents). It wins back between a third and nearly all of what the
latency cut cost.

**CPU** (block 32, `--repeat 20`, % of one core / p99 µs per block):

| Range | P-core, 1.0.1 → current | E-core, 1.0.1 → current |
|---|---|---|
| Low | 1.06 % → 2.02 % / 66 → 66 µs | 2.38 % → 4.35 % / 141 → 141 µs |
| Normal | 0.45 % → 0.80 % / 25 → 25 µs | 1.16 % → 1.90 % / 59 → 59 µs |
| High | 0.18 % → 0.34 % / 8 → 8 µs | 0.48 % → 1.14 % / 21 → 22 µs |

Detection now runs twice as often; a block of 32 never holds more than one
hop, so the per-block peak stays the same. The jump search adds a burst at
each splice without a period: with detune +100 in Low, the slowest block went
from 78 to 117 µs on a P-core and from 168 to 203 µs on an E-core (budget
726 µs).
