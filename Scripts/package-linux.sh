#!/bin/bash
# Builds Torqwave for Linux (x86_64 or the machine's own architecture), runs the tests with the
# built program as the plug-in scan worker, and packs it into a tarball:
#   dist/Torqwave-<version>-linux-<arch>.tar.gz (the program, an installer, the desktop entry,
#   the icon and the licences) and its .sha256
#
# Usage: Scripts/package-linux.sh [--juce <JUCE checkout>] [--jobs <n>] [--skip-tests]
#
# The tests need an audio device and a desktop session (X11, or Wayland with XWayland).
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
build="$root/build-release-linux"
dist="$root/dist"
juce=""
jobs="$(nproc)"
tests=1

while [ $# -gt 0 ]; do
    case "$1" in
        --juce) juce="$2"; shift 2 ;;
        --jobs) jobs="$2"; shift 2 ;;
        --skip-tests) tests=0; shift ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

version="$(sed -n 's/^project(Torqwave VERSION \([0-9.]*\).*/\1/p' "$root/CMakeLists.txt")"
[ -n "$version" ] || { echo "no version in CMakeLists.txt" >&2; exit 1; }
arch="$(uname -m)"

generator="Unix Makefiles"
command -v ninja > /dev/null && generator="Ninja"

echo "== Building Torqwave $version ($arch)"
cmake -S "$root" -B "$build" -G "$generator" -DCMAKE_BUILD_TYPE=Release ${juce:+-DTORQWAVE_JUCE_DIR="$juce"}
cmake --build "$build" --config Release -j "$jobs"

app="$build/Torqwave_artefacts/Release/Torqwave"
tester="$build/TorqwaveTests_artefacts/Release/TorqwaveTests"

if [ "$tests" = 1 ]; then
    echo "== Tests, with the built program as the scan worker"
    if ! "$tester" --app "$app" > "$build/tests.log" 2>&1; then
        grep FAIL "$build/tests.log" >&2 || true
        echo "tests failed; the full output is in $build/tests.log" >&2
        exit 1
    fi
    tail -n 1 "$build/tests.log"
fi

echo "== Tarball"
name="Torqwave-$version-linux-$arch"
stage="$(mktemp -d)"
trap 'rm -rf "$stage"' EXIT
mkdir "$stage/$name"
install -m 755 "$app" "$stage/$name/Torqwave"
strip "$stage/$name/Torqwave"
install -m 644 "$root/Art/torqwave-icon.svg" "$stage/$name/torqwave.svg"
install -m 644 "$root/Scripts/linux/torqwave.desktop" "$stage/$name/torqwave.desktop"
install -m 755 "$root/Scripts/linux/install.sh" "$stage/$name/install.sh"
cp "$root/LICENSE" "$stage/$name/LICENSE.txt"
cp "$build/_deps/lame-src/COPYING" "$stage/$name/LAME-LICENSE.txt"
cp "$build/_deps/signalsmith_stretch-src/LICENSE.txt" "$stage/$name/SIGNALSMITH-STRETCH-LICENSE.txt"
cp "$build/_deps/signalsmith_linear-src/LICENSE.txt" "$stage/$name/SIGNALSMITH-LINEAR-LICENSE.txt"
mkdir -p "$dist"
tarball="$dist/$name.tar.gz"
rm -f "$tarball"
tar -C "$stage" -czf "$tarball" "$name"
(cd "$dist" && sha256sum "$(basename "$tarball")" > "$(basename "$tarball").sha256")

echo "== Done"
ls -l "$tarball"
cat "$tarball.sha256"
