# Licensing

**Decision:** VocalTuneBtw is open source under **AGPL-3.0-or-later**
(full text in `LICENSE.md`). This lets us use JUCE under its AGPLv3 option,
with no commercial JUCE licence.

## Dependency audit

Versions are pinned in `CMakeLists.txt` and `tests/CMakeLists.txt`. Every
licence was read from the fetched sources on 2026-10-28.

| Dependency | Version | Licence | Used for | Ships in binary |
|---|---|---|---|---|
| JUCE | 9.0.4 | AGPLv3 / commercial JUCE 9 EULA (dual) | plugin framework, GUI, VST3 and Standalone wrappers | yes |
| VST3 SDK (bundled in JUCE) | the version bundled in JUCE 9.0.4 | MIT (Steinberg, 2025) | VST3 interfaces | yes |
| clap-juce-extensions | `7adee3a` (main, after 0.26.0) | MIT | CLAP wrapper (optional, `PITCHPILOT_BUILD_CLAP`) | CLAP build only |
| CLAP, clap-helpers | submodules of the above | MIT | CLAP ABI | CLAP build only |
| Catch2 | 3.7.1 | BSL-1.0 | unit tests | no |
| Ubuntu Light font | as shipped in `epaint_default_fonts` 0.37.0 (`assets/fonts/`) | Ubuntu Font Licence 1.0 (`assets/fonts/UFL.txt`) | the PluginsBTW UI face, embedded in the editor | yes |

All of these licences are compatible with distributing the combined work under
AGPL-3.0-or-later.

## Obligations when distributing binaries

- Provide the complete corresponding source, including build scripts, under
  the AGPL. A link to the public repository at the same revision meets this.
- Keep the MIT notices for the VST3 SDK and CLAP libraries, and the Ubuntu
  Font Licence (`assets/fonts/UFL.txt`), in the release archive.
- AGPL §13 (network interaction) doesn't apply in practice. VocalTuneBtw is a
  local plugin with no network features.

## If plans change to closed source

A closed-source release would need a commercial JUCE licence (or a move off
JUCE), plus relicensing of all project code. Every non-JUCE dependency is
already permissive, so JUCE is the only blocker.
