#!/usr/bin/env bash
# Configure and build PitchPilot. Usage: scripts/build-linux.sh [--tests] [--debug]
#   --tests  build and run only the DSP tests (skips fetching JUCE)
#   --debug  Debug instead of Release
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_type=Release
plugin=ON
for arg in "$@"; do
    case "$arg" in
        --tests) plugin=OFF ;;
        --debug) build_type=Debug ;;
        *) echo "unknown option: $arg" >&2; exit 2 ;;
    esac
done

build_dir="$root/build"
[[ $plugin == OFF ]] && build_dir="$root/build-tests"

generator=()
command -v ninja >/dev/null && generator=(-G Ninja)

cmake -S "$root" -B "$build_dir" "${generator[@]}" \
    -DCMAKE_BUILD_TYPE="$build_type" -DPITCHPILOT_BUILD_PLUGIN="$plugin"
cmake --build "$build_dir" --parallel "$(nproc)"
ctest --test-dir "$build_dir" --output-on-failure --parallel "$(nproc)"
