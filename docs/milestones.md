# Milestones

Each milestone ends with a clean build and passing tests. Every claim is
checked against something we can measure.

## M1 — Skeleton and DSP prototype *(done; released as v0.1.1)*

- Architecture, licensing and milestone documents.
- CMake project that builds VST3 and Standalone (primary) and CLAP (optional).
- The JUCE-free DSP library: YIN detector, scale mapper, correction
  controller, pitch-synchronous splice shifter and bypass crossfade.
- Catch2 tests for pitch accuracy, scale mapping, correction behaviour, shifter
  accuracy and no allocation on the audio path.
- JUCE generic editor, so every parameter can be used and automated.

## M2 — DSP quality

- Measure on real vocal recordings: octave errors, breaths, sibilants, vibrato.
- Improve the shifter: measure the splice artifacts on vocals. Candidates are
  full PSOLA (formant-preserving at large shifts) or a phase vocoder with
  formant correction.
- Make the detector's difference function FFT-based if profiling shows a need.
- Add voiced/unvoiced hold logic so short consonants don't reset the target.
- Write `docs/dsp-evaluation.md` with objective numbers (pitch error, latency,
  CPU per instance).

## M3 — Custom UI *(second pass in v0.2.0: PluginsBTW styling, one-octave note keyboard, rounded menus. Open: pitch-history trace, keyboard accessibility)*

- Dark navy and blue design: root and scale selectors, a piano-key note
  mask editor, rotary knobs for speed, detune and strength.
- Tuning display: detected note, target note, cents meter and a pitch-history
  trace from the telemetry atomics.
- Fixed-size layout, keyboard accessibility, correct sync with host automation.

## M4 — Formats, latency and stress

- Load the VST3 in Bitwig and Carla (CLAP only if still wanted). Test automation, preset recall and
  reload.
- Validate with `clap-validator` and the Steinberg `validator`.
- Run 32/64/128/256-sample blocks at 44.1, 48, 88.2 and 96 kHz under CPU load.
- Decide on optional look-ahead (more latency in exchange for on-time onsets).

## M5 — Optional features and packaging

- MIDI note guidance (MIDI input sets the target notes).
- Install script and tarball. Document limitations. *(Tarball done:
  `scripts/package-linux.sh`, GitHub releases. Install script not started.)*
