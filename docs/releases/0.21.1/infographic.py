#!/usr/bin/env python3
"""Draws the Instagram infographic for Torqwave 0.21.1.

    python3 docs/releases/0.21.1/infographic.py

Writes infographic-portrait (1080 x 1350) and infographic-square (1080 x 1080) beside itself, as SVG, and
as PNG when rsvg-convert is found. Python's standard library only. The palette and the SVG helpers are the
comparison graphics' (docs/compare/generate.py), which take them from the app's Dark theme. Every number is
from the changelog's entries for 0.21.1.
"""
import os, shutil, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "compare"))
from generate import Svg, SURFACE, RAISED, LINE, WHITE, TEXT, DIM, FAINT, ACCENT, WAVE  # noqa: E402

VERSION = "0.21.1"
SITE = "bobclawblaw.github.io/torqwave"
# The 30 sounds' pads by family, in the window's order and colours; the 34 after them are the user's.
FAMILIES = [("#e0604f", 1), ("#e0a040", 3), ("#3fc0a8", 3), ("#a070d8", 3), ("#4a9fe0", 5), ("#6cc24a", 15)]
TILES = [("54", ["synthesized", "sources"]),
         ("16", ["velocity layers", "a pad, round robin"]),
         ("5", ["effects on", "every pad"]),
         ("15", ["kits"]),
         ("8", ["choke groups", "of your own"])]
EFFECTS = "Envelope, transient shaper, saturator, EQ, compressor"
IMPORT = ["Imports a folder of sounds,", "or Battery 4 kit files you own"]
ALSO = "Also: an unsaved session comes back on Continue; a Panic command"


def pads(s, x, y, size):
    """The window's 64 pads, eight across: 30 sounds coloured by family, 34 user pads."""
    colours = [c for c, n in FAMILIES for _ in range(n)]
    gap = size * 0.018
    cell = (size - 7 * gap) / 8
    for i in range(64):
        px, py = x + (i % 8) * (cell + gap), y + (i // 8) * (cell + gap)
        if i < len(colours):
            s.rect(px, py, cell, cell, RAISED, 7, LINE)
            s.parts.append(f'<rect x="{px:.1f}" y="{py:.1f}" width="{cell:.1f}" height="{cell:.1f}" rx="7" fill="{colours[i]}" opacity="0.2"/>')
            s.rect(px + cell * 0.16, py + cell * 0.16, cell * 0.3, cell * 0.09, colours[i], 2)
        else:
            s.rect(px, py, cell, cell, SURFACE, 7, LINE)
            s.rect(px + cell * 0.16, py + cell * 0.16, cell * 0.3, cell * 0.09, "#434a54", 2)


def tile(s, x, y, w, h, number, lines, big, small):
    s.rect(x, y, w, h, SURFACE, 12, LINE)
    s.rect(x, y, w, 5, WAVE, 2)
    s.text(x + 24, y + 30 + big * 0.78, number, big, WAVE, weight="bold")
    for i, ln in enumerate(lines):
        s.text(x + 24, y + 30 + big * 0.78 + small * 1.45 + i * small * 1.22, ln, small, WHITE, weight="bold")


def draw(name, h):
    w, m = 1080, 60
    tall = h > 1200
    s = Svg(w, h)
    # the name and the version
    s.rect(m, 62, 8, 34, ACCENT, 2)
    s.text(m + 24, 91, "TORQWAVE", 34, WHITE, weight="bold")
    s.text(w - m, 91, VERSION, 34, WAVE, "end", "bold")
    s.line(m, 122, w - m, 122, LINE, 2)
    # what the release is
    s.text(m, 206, "Basic Drums: a drum synth", 66, WHITE, weight="bold")
    s.text(m, 284, "and a sampler in one", 66, WHITE, weight="bold")
    # the pads and their number
    gy, gs = 326, 350 if tall else 300
    pads(s, m, gy, gs)
    rx = m + gs + 56
    big = 240 if tall else 190
    s.text(rx - 8, gy + big * 0.76, "64", big, WAVE, weight="bold")
    ty = gy + big * 0.76 + (68 if tall else 54)
    s.text(rx, ty, "pads", 60 if tall else 52, WHITE, weight="bold")
    s.text(rx, ty + 54, "30 drum sounds", 34, TEXT)
    s.text(rx, ty + 98, "34 for your samples", 34, TEXT)
    # the numbers
    y = gy + gs + (44 if tall else 30)
    gap = 18
    if tall:
        tw, th = (w - 2 * m - 2 * gap) / 3, 184
        for i, (n, lines) in enumerate(TILES[:3]):
            tile(s, m + i * (tw + gap), y, tw, th, n, lines, 84, 29)
        y += th + gap
        tw2 = (w - 2 * m - gap) / 2
        for i, (n, lines) in enumerate(TILES[3:]):
            x = m + i * (tw2 + gap)
            s.rect(x, y, tw2, 108, SURFACE, 12, LINE)
            s.rect(x, y, tw2, 5, WAVE, 2)
            s.text(x + 24, y + 82, n, 80, WAVE, weight="bold")
            nx = x + 24 + (112 if len(n) == 2 else 66)
            if len(lines) == 1:
                s.text(nx, y + 70, lines[0], 34, WHITE, weight="bold")
            else:
                s.text(nx, y + 52, lines[0], 29, WHITE, weight="bold")
                s.text(nx, y + 88, lines[1], 29, WHITE, weight="bold")
        y += 108 + 50
        s.text(m, y, "The five: " + EFFECTS[0].lower() + EFFECTS[1:], 27, DIM)
        y += 58
        for i, ln in enumerate(IMPORT):
            s.text(m, y + i * 44, ln, 36, WHITE, weight="bold")
        s.text(m, y + 44 + 52, ALSO, 26, DIM)
    else:
        tw, th = (w - 2 * m - 4 * gap) / 5, 170
        for i, (n, lines) in enumerate(TILES):
            x = m + i * (tw + gap)
            s.rect(x, y, tw, th, SURFACE, 12, LINE)
            s.rect(x, y, tw, 5, WAVE, 2)
            s.text(x + 18, y + 86, n, 76, WAVE, weight="bold")
            short = {"54": ["synth", "sources"], "16": ["velocity", "layers a pad"], "5": ["effects", "a pad"], "15": ["kits"], "8": ["choke", "groups"]}[n]
            for j, ln in enumerate(short):
                s.text(x + 18, y + 124 + j * 30, ln, 25, WHITE, weight="bold")
        y += th + 52
        for i, ln in enumerate(IMPORT):
            s.text(m, y + i * 42, ln, 33, WHITE, weight="bold")
    # where to get it
    s.line(m, h - 100, w - m, h - 100, LINE, 2)
    s.text(m, h - 44, SITE, 36, WAVE, weight="bold")
    s.text(w - m, h - 44, "Free, open source", 28, FAINT, "end")
    s.parts.append("</svg>")
    svg = os.path.join(HERE, name + ".svg")
    with open(svg, "w", encoding="utf-8") as f:
        f.write("\n".join(s.parts) + "\n")
    print("wrote", os.path.relpath(svg))
    if shutil.which("rsvg-convert"):
        png = svg[:-4] + ".png"
        subprocess.run(["rsvg-convert", "-w", str(w), "-h", str(h), svg, "-o", png], check=True)
        print("wrote", os.path.relpath(png))


if __name__ == "__main__":
    draw("infographic-portrait", 1350)
    draw("infographic-square", 1080)
