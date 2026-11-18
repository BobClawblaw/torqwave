#!/bin/bash
# Installs Torqwave from this folder for you alone (~/.local), or for everyone with --system
# (/usr/local, run with sudo). --uninstall removes it again.
#
# Usage: ./install.sh [--system] [--uninstall]
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
prefix="$HOME/.local"
uninstall=0

while [ $# -gt 0 ]; do
    case "$1" in
        --system) prefix="/usr/local"; shift ;;
        --uninstall) uninstall=1; shift ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

bin="$prefix/bin/Torqwave"
desktop="$prefix/share/applications/torqwave.desktop"
# The icon theme's scalable folder: desktops look there for an app's icon, in every size.
icon="$prefix/share/icons/hicolor/scalable/apps/torqwave.svg"
licences="$prefix/share/doc/torqwave"
# What the app installed when it was called SpectraBlade (up to 0.15.10), removed either way.
previous=(
    "$prefix/bin/SpectraBlade"
    "$prefix/share/applications/spectrablade.desktop"
    "$prefix/share/icons/hicolor/scalable/apps/spectrablade.svg"
    "$prefix/share/pixmaps/spectrablade.png"
    "$prefix/share/doc/spectrablade"
)

if [ "$uninstall" = 1 ]; then
    rm -f "$bin" "$desktop" "$icon"
    rm -rf "$licences" "${previous[@]}"
    echo "Torqwave removed from $prefix"
else
    install -Dm 755 "$here/Torqwave" "$bin"
    install -Dm 644 "$here/torqwave.svg" "$icon"
    rm -rf "${previous[@]}"
    install -Dm 644 "$here/torqwave.desktop" "$desktop"
    sed -i "s|^Exec=Torqwave|Exec=$bin|" "$desktop"
    mkdir -p "$licences"
    cp "$here"/*LICENSE*.txt "$licences/"
    echo "Torqwave installed in $prefix (run it from your applications menu, or $bin)"
fi

command -v update-desktop-database > /dev/null && update-desktop-database -q "$prefix/share/applications" 2>/dev/null || true
command -v gtk-update-icon-cache > /dev/null && gtk-update-icon-cache -q "$prefix/share/icons/hicolor" 2>/dev/null || true
