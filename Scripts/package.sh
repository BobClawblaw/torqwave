#!/bin/bash
# Builds Torqwave as a universal app (Apple silicon and Intel), signs it, runs the tests with the
# signed app as the plug-in scan worker (each half: the Intel one under Rosetta), and packs it into
# a disk image:
#   dist/Torqwave-<version>.dmg (the app and the licence) and its .sha256
#
# Usage: Scripts/package.sh [--juce <JUCE checkout>] [--jobs <n>] [--skip-tests] [--release-kit]
#
# Without a Developer ID the app is signed ad hoc: it runs on any Mac, but Gatekeeper asks the
# first time it is opened (see README, "Installing").
#
# TORQWAVE_SIGN_IDENTITY names a code-signing identity in the keychain to sign with instead, such
# as a self-signed certificate made for development (Keychain Access > Certificate Assistant >
# Create a Certificate..., type Code Signing). Builds signed by one identity keep macOS's
# permissions (the microphone, the local network) from one build to the next; ad hoc builds are
# a new app to macOS each time, so it asks again.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
build="$root/build-release"
dist="$root/dist"
juce=""
jobs=4
tests=1
release_kit=0

while [ $# -gt 0 ]; do
    case "$1" in
        --juce) juce="$2"; shift 2 ;;
        --jobs) jobs="$2"; shift 2 ;;
        --skip-tests) tests=0; shift ;;
        --release-kit) release_kit=1; shift ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

version="$(sed -n 's/^project(Torqwave VERSION \([0-9.]*\).*/\1/p' "$root/CMakeLists.txt")"
[ -n "$version" ] || { echo "no version in CMakeLists.txt" >&2; exit 1; }

echo "== Building Torqwave $version (arm64 and x86_64)"
cmake -S "$root" -B "$build" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release \
      "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64" ${juce:+-DTORQWAVE_JUCE_DIR="$juce"}
cmake --build "$build" --config Release -j "$jobs"

app="$build/Torqwave_artefacts/Release/Torqwave.app"
tester="$build/TorqwaveTests_artefacts/Release/TorqwaveTests"

identity="${TORQWAVE_SIGN_IDENTITY:-}"
echo "== Signing (${identity:-ad hoc})"
codesign --force --deep --sign "${identity:--}" "$app"
codesign --verify --deep --strict "$app"
archs="$(lipo -archs "$app/Contents/MacOS/Torqwave")"
[ "$archs" = "x86_64 arm64" ] || { echo "expected a universal app (x86_64 arm64), got: $archs" >&2; exit 1; }
echo "$archs"

if [ "$tests" = 1 ]; then
    arch -x86_64 /usr/bin/true 2>/dev/null || { echo "the Intel half's tests need Rosetta (softwareupdate --install-rosetta)" >&2; exit 1; }

    for half in arm64 x86_64; do
        echo "== Tests ($half), with the signed app as the scan worker"
        log="$build/tests-$half.log"
        if ! arch "-$half" "$tester" --app "$app/Contents/MacOS/Torqwave" > "$log" 2>&1; then
            grep FAIL "$log" >&2 || true
            echo "tests failed; the full output is in $log" >&2
            exit 1
        fi
        tail -n 1 "$log"
    done
fi

echo "== Disk image"
stage="$(mktemp -d)"
trap 'rm -rf "$stage"' EXIT
ditto "$app" "$stage/Torqwave.app"
ln -s /Applications "$stage/Applications"
cp "$root/LICENSE" "$stage/LICENSE.txt"
cp "$build/_deps/lame-src/COPYING" "$stage/LAME-LICENSE.txt"
cp "$build/_deps/signalsmith_stretch-src/LICENSE.txt" "$stage/SIGNALSMITH-STRETCH-LICENSE.txt"
cp "$build/_deps/signalsmith_linear-src/LICENSE.txt" "$stage/SIGNALSMITH-LINEAR-LICENSE.txt"
cp "$root/ThirdParty/clap/LICENSE" "$stage/CLAP-LICENSE.txt"
mkdir -p "$dist"
dmg="$dist/Torqwave-$version.dmg"
rm -f "$dmg"
# The image's size is given: left to hdiutil, it has come out too small for the app ("No space
# left on device"), and with -quiet that failed without a word.
megabytes=$(( $(du -sm "$stage" | cut -f1) * 5 / 4 + 32 ))
hdiutil create -volname "Torqwave $version" -srcfolder "$stage" -size "${megabytes}m" -format UDZO "$dmg"
(cd "$dist" && shasum -a 256 "$(basename "$dmg")" > "$(basename "$dmg").sha256")

echo "== Done"
ls -l "$dmg"
cat "$dmg.sha256"

if [ "$release_kit" = 1 ]; then
    echo "== Release marketing kit (docs/releases/$version)"
    python3 "$root/Scripts/release_kit.py" --version "$version"
fi
