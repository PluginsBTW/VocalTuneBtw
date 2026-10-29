#!/usr/bin/env bash
# Package a Release build for distribution. Usage: scripts/package-linux.sh [build-dir]
# Writes dist/VocalTuneBtw-<version>-linux-x86_64.tar.gz (VST3 + CLAP + Standalone,
# stripped, with LICENSE.md and third-party licences) and dist/SHA256SUMS.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="$(cd "${1:-$root/build}" && pwd)"
version="$(sed -n 's/^project(VocalTuneBtw VERSION \([0-9.]*\).*/\1/p' "$root/CMakeLists.txt")"
artefacts="$build_dir/src/plugin/VocalTuneBtw_artefacts/Release"
juce="$build_dir/_deps/juce-src"
clap="$build_dir/_deps/clap_juce_extensions-src"
name="VocalTuneBtw-$version-linux-x86_64"
stage="$root/dist/$name"

[[ -n $version ]] || { echo "no version in CMakeLists.txt" >&2; exit 1; }
[[ -d $artefacts/VST3/VocalTuneBtw.vst3 ]] || { echo "no Release build in $build_dir" >&2; exit 1; }
[[ -f $artefacts/CLAP/VocalTuneBtw.clap ]] || { echo "no CLAP in $build_dir (configure with -DPITCHPILOT_BUILD_CLAP=ON)" >&2; exit 1; }

rm -rf "$root/dist"
mkdir -p "$stage/third-party-licenses"
cp -r "$artefacts/VST3/VocalTuneBtw.vst3" "$stage/"
cp "$artefacts/CLAP/VocalTuneBtw.clap" "$stage/"
cp "$artefacts/Standalone/VocalTuneBtw" "$stage/"
strip --strip-unneeded "$stage/VocalTuneBtw.vst3/Contents/x86_64-linux/VocalTuneBtw.so" "$stage/VocalTuneBtw.clap" "$stage/VocalTuneBtw"

cp "$root/LICENSE.md" "$stage/"
# Code compiled into the binaries, with the notices their licences ask us to ship.
licence () { cp "$juce/$1" "$stage/third-party-licenses/$2"; }
licence LICENSE.md JUCE.md
licence modules/juce_audio_processors_headless/format_types/VST3_SDK/LICENSE.txt VST3-SDK.txt
licence modules/juce_core/zip/zlib/LICENSE zlib.txt
licence modules/juce_graphics/image_formats/pnglib/LICENSE libpng.txt
licence modules/juce_graphics/drawables/lunasvg/LICENSE lunasvg.txt
licence modules/juce_graphics/unicode/sheenbidi/LICENSE SheenBidi.txt
cp "$clap/LICENSE.md" "$stage/third-party-licenses/clap-juce-extensions.md"
cp "$clap/clap-libs/clap/LICENSE" "$stage/third-party-licenses/CLAP.txt"
cp "$clap/clap-libs/clap-helpers/LICENSE" "$stage/third-party-licenses/clap-helpers.txt"
cp "$root/assets/fonts/UFL.txt" "$stage/third-party-licenses/Ubuntu-Font-Licence.txt"

cat > "$stage/INSTALL.txt" <<EOF
VocalTuneBtw $version for Linux (x86-64)

Install the plugin for your user, as VST3, CLAP or both:

    mkdir -p ~/.vst3 && cp -r VocalTuneBtw.vst3 ~/.vst3/
    mkdir -p ~/.clap && cp VocalTuneBtw.clap ~/.clap/

Then rescan plugins in your host. To run without a host, start ./VocalTuneBtw.

Source code, documentation and licence: https://github.com/PluginsBTW/VocalTuneBtw
VocalTuneBtw is licensed under AGPL-3.0-or-later (LICENSE.md). Licences of
the third-party code it contains are in third-party-licenses/.
EOF

tar -C "$root/dist" -czf "$root/dist/$name.tar.gz" "$name"
(cd "$root/dist" && sha256sum "$name.tar.gz" > SHA256SUMS)
echo "$root/dist/$name.tar.gz"
