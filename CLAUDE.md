# PitchPilot — agent notes

Monophonic real-time pitch-correction plugin (VST3, CLAP, Standalone) for Linux.
C++20 + JUCE. Licensed AGPL-3.0-or-later. Design: `docs/architecture.md`.
Roadmap: `docs/milestones.md`.

## Commands

```bash
scripts/build-linux.sh            # configure + build everything (Release) into build/
scripts/build-linux.sh --tests    # tests only, without fetching JUCE (fast)
ctest --test-dir build --output-on-failure
```

Artifacts land in `build/src/plugin/PitchPilot_artefacts/Release/{VST3,CLAP,Standalone}`.
The build never copies plugins into `~/.vst3` or `~/.clap`. Ask before installing.

## Rules

- `src/dsp` must not include JUCE headers. Keep it testable in isolation.
- `process()` paths must not allocate, lock, do I/O or loop unboundedly.
  `tests/test_realtime.cpp` must keep passing. Allocate in `prepare()` only.
- New DSP behaviour gets a test in `tests/` first. Run the tests before calling
  anything done.
- Never claim audio quality, latency or real-time safety without a measurement
  or test that shows it.
- Parameter IDs in `src/plugin/Parameters.cpp` are part of saved state and
  host automation. Never rename or reorder existing ones; add new ones only.
