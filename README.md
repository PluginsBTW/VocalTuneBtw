# VocalTuneBtw

A native Linux pitch-correction plugin for monophonic vocals. It builds VST3,
CLAP and a Standalone app.

> **Status: M1 prototype.** Detection, scale mapping, correction and shifting
> work and are covered by tests. The shifter is a pitch-synchronous splice
> design: good on steady tones, but it hasn't been evaluated on real vocals
> yet. See
> [docs/milestones.md](docs/milestones.md).

## Build

Requirements: CMake ≥ 3.25, a C++20 compiler, git, and the JUCE Linux
dependencies (ALSA, X11/Xrandr/Xinerama/Xcursor, FreeType, fontconfig).
Ninja is used if it is installed.

```bash
scripts/build-linux.sh           # Release build of all formats and the tests
scripts/build-linux.sh --tests   # DSP tests only, no JUCE download
```

CLAP is optional. To build VST3 and Standalone only, configure with
`-DPITCHPILOT_BUILD_CLAP=OFF`.

Artifacts:

```
build/src/plugin/VocalTuneBtw_artefacts/Release/VST3/VocalTuneBtw.vst3
build/src/plugin/VocalTuneBtw_artefacts/Release/CLAP/VocalTuneBtw.clap
build/src/plugin/VocalTuneBtw_artefacts/Release/Standalone/VocalTuneBtw
```

To install for your user:

```bash
cp -r build/src/plugin/VocalTuneBtw_artefacts/Release/VST3/VocalTuneBtw.vst3 ~/.vst3/
mkdir -p ~/.clap && cp build/src/plugin/VocalTuneBtw_artefacts/Release/CLAP/VocalTuneBtw.clap ~/.clap/
```

The plugin used to be called PitchPilot. It keeps the same plugin ID, so old
projects still load it. Delete `~/.vst3/PitchPilot.vst3` so the host doesn't
see the plugin twice.

## Controls

Root note, Scale (14 options including modes, pentatonics, blues and custom),
per-note enable switches, Correction Speed (1–200 ms), Strength, Detune,
Tolerance, Reference A4 and Voice Range (Low/Normal/High). Bypass is click-free
and exposed to the host. Parameter details are in
[docs/architecture.md](docs/architecture.md).

## Latency

The latency depends on the Voice Range setting (behind the gear icon): Low
12.5 ms, Normal 8.6 ms, High 5.5 ms. The plugin reports the current value to
the host. Pick the highest range that still covers the singer's lowest note.

## License

AGPL-3.0-or-later. See [docs/licensing.md](docs/licensing.md).
