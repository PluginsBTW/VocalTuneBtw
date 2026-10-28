# Handoff: start M2 — DSP quality on real vocals (2026-10-28)

Repo `PluginsBTW/VocalTuneBtw` (public), branch `main`. At handoff `main` was
`0d57d90` plus this handoff commit; tags `v0.1.0`, `v0.1.1`. The local checkout
is `/home/roman/code/VocalTuneBtw` (it was called `autotune` earlier; the old
path no longer exists).

## Status

- DONE: M1 (DSP engine, VST3/Standalone plugin, 25 Catch2 tests). Verified by
  `scripts/build-linux.sh` on 2026-10-28: build clean, 25/25 tests pass.
- DONE: M3 first pass (custom navy/blue UI, fixed 760×480 window). Verified by
  screenshots of the Standalone app only; the user checked it in Bitwig.
- DONE: releases. v0.1.1 is "Latest" on GitHub; v0.1.0 has a warning banner
  (it pops). Verified by re-downloading v0.1.1 and `sha256sum -c` (OK).
- DONE: the user's installed plugin is the 0.1.1 build in `~/.vst3/VocalTuneBtw.vst3`.
- NOT STARTED: everything in M2 (`docs/milestones.md`).

## What happened with latency (read before touching the shifter)

- Commit `0e25b15` cut shifter latency by ~1.6 ms: the crossfade headroom below
  `minDelay` was sized for shifts ≤ 3 semitones, and larger upward shifts moved
  the splice point up (`spliceUpAt`) instead. All tests, including a new
  click test on sine tones at +3/+7/+12 semitones, passed.
- The user then heard **audio popping on real vocals** in Bitwig. It was
  reverted in `4c33415` and shipped as v0.1.1. Latency is back to Low 12.5 ms,
  Normal 8.6 ms, High 5.5 ms at 48 kHz (602 / 413 / 266 samples).
- The cause of the popping was NEVER diagnosed. Unverified hypotheses: (1) the
  ratio rises during a fade and the `fadingDelay` clamp at 1 sample creates a
  discontinuity; (2) `spliceUpAt` moves with the ratio, so splices fire at odd
  moments while correction tracks vibrato; (3) the period hint is off on real
  voice, so splices aren't phase-aligned and less headroom exposes it.
- Lesson: steady sine tests can't catch this. Any shifter change needs a test
  on vocal-like input (pitch and amplitude modulation, onsets, changing ratio)
  and a listen by the user before it ships.

## Decisions (don't reverse without asking)

- C++20 + JUCE 9.0.4, AGPL-3.0-or-later. `src/dsp` has no JUCE; no allocation
  on the audio path (`tests/test_realtime.cpp`). See `CLAUDE.md`.
- VST3 + Standalone only. The user said CLAP isn't needed; it still builds
  (`PITCHPILOT_BUILD_CLAP`) but is untested and not shipped.
- Plugin name VocalTuneBtw, but plugin codes `Ptpl`/`Ppc1`, the APVTS state
  type `"PitchPilot"`, parameter IDs, the `pitchpilot` C++ namespace and the
  `PITCHPILOT_*` CMake options stay. Why: changing codes, state type or IDs
  breaks saved Bitwig projects; the rest isn't worth the churn.
- Releases are built locally on Arch and published with `gh` (the user's
  choice). They need glibc 2.43+. `.github/workflows/release.yml` is manual-only
  so it can't race a manual release; it builds on Ubuntu 22.04 (run
  40710946643: 25/25 tests, newest symbol GLIBC_2.35) but its publish step fails
  when the release already exists.
- Move a published tag? No: a force-push was blocked; ship a new version
  instead (that's how 0.1.1 happened).
- Humanize shows percent of the 0–50 cent tolerance; the parameter is unchanged.

## How the user works

- Install and commit only when they say so ("Install and commit", "Commit it").
  `CLAUDE.md`: ask before installing into `~/.vst3`.
- They test by ear in Bitwig and report back briefly. Their system locale is
  German (build and git messages are German: "Fehler" = error).
- Commands typed with `!` in the Claude desktop app fail with
  `bash: cd: Zu viele Argumente` before running. Tell them to use a real
  terminal, or run the command yourself.

## Environment

- Push over SSH (`origin` = `git@github.com:PluginsBTW/VocalTuneBtw.git`). The
  `gh` token lacks the `workflow` scope, so HTTPS pushes that touch
  `.github/workflows` are rejected. `gh` works for releases and runs.
- Build: `scripts/build-linux.sh` (Release, all formats, runs tests; first run
  downloads JUCE). `scripts/build-linux.sh --tests` = DSP tests only, fast.
- Package: `scripts/package-linux.sh` → `dist/VocalTuneBtw-<version>-linux-x86_64.tar.gz`
  + `SHA256SUMS`. The version comes from `project(... VERSION x.y.z)` in `CMakeLists.txt`.
- Install: `rm -rf ~/.vst3/VocalTuneBtw.vst3 && cp -r build/src/plugin/VocalTuneBtw_artefacts/Release/VST3/VocalTuneBtw.vst3 ~/.vst3/`
- Screenshot of the UI: start
  `build/src/plugin/VocalTuneBtw_artefacts/Release/Standalone/VocalTuneBtw` in
  the background, then `import -window VocalTuneBtw out.png` (retry for a few
  seconds), then kill it. The editor is at offset +1+57 in that capture
  (`magick out.png -crop 760x480+1+57 +repage panel.png`). To show a fixed
  note or the settings page, patch `MainPanel` temporarily and restore it.
- No containers (podman/docker) on this machine.

## Blockers

- M2 needs real vocal recordings and there are none in the repo. Unblocked by
  the user supplying dry mono vocal WAVs (ideally including the passage where
  they heard the popping). Don't commit recordings to this public repo unless
  the user confirms they may be published; keep them outside the repo or in a
  gitignored folder.

## Next action

1. Ask the user for 2–3 dry vocal WAVs (low and high voice, vibrato, fast
   phrases) and the key/scale/settings they used when they heard popping.
2. While waiting, build an offline renderer: `tools/vtb-render.cpp` (new CMake
   target linking `pitchpilot_dsp`, no JUCE) that reads a 16/24-bit or float WAV,
   runs `dsp::PitchCorrector` with settings from command-line flags, writes the
   output WAV, and prints metrics per file: pitch error to target (cents, from
   the detector on the output), voiced fraction, and a click score (largest
   sample-to-sample jump relative to a local RMS, plus its timestamp).
3. Add a synthetic vocal-like test signal to `tests/Signals.h` (sawtooth with
   vibrato 5–6 Hz ±50 cents, amplitude envelope, short noise bursts for
   consonants) and a test that runs the full `PitchCorrector` with it and
   checks the click score. This is the regression net the latency cut lacked.
4. Then work through M2 in `docs/milestones.md`, writing results to
   `docs/dsp-evaluation.md`.

## Follow-ups (not M2)

- Workflow: make "Publish release" upload with `--clobber` when the release
  exists, so a portable (glibc 2.35) build can be added to v0.1.1. Needs the
  user's OK to add a second archive to a published release.
- M4: test in Carla, run the Steinberg validator, block-size and sample-rate
  stress.

## Skills Review

- ✅ tdd-skill: the latency change had a test written first and checked to fail
  without the fix. ❌ But the test signals (steady sines) didn't represent the
  real input, so a regression shipped. Fix: next action steps 2–3.
- ✅ git-sync: everything committed and pushed; single `main` branch, no stray
  worktrees or branches.
- ⚠️ security-and-hardening: workflow actions are pinned to commit SHAs; the
  workflow token has `contents: write` only.
- ⚠️ design-guide: the UI was only checked at 1× in the Standalone app.

Environment review:

- Automated checks: no test exercises the shifter with a changing ratio on
  modulated input. That gap let the popping through (see Next action 3).
- Information access: the agent can't hear audio. Pops need an objective click
  metric (Next action 2) or the user's ears before a release.
