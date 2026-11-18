# Writes torqwave-icon.svg, the app icon (1024 x 1024, macOS icon grid): the mark of ridges.py, a
# twisted stack of waveforms, with fewer and bolder slices than the banner's. Also writes
# torqwave-icon-small.svg, the same drawn for the Dock and smaller (CMake's ICON_SMALL, at 256 px):
# 12 slices, three times as thick and larger in the tile, the second slice lifted so it shows
# behind the front one.
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ridges

S = 1024
BG = "#08080a"
MARKS = {
    "torqwave-icon.svg": ridges.mark((172, 196, 680, 620), lines=26, twist=45, stroke=7.0, peak=1.25, points=180, seed=5,
                                     background=BG, bright=0.7),
    "torqwave-icon-small.svg": ridges.mark((130, 150, 764, 720), lines=12, twist=45, stroke=20, peak=1.5, points=120,
                                           seed=5, background=BG, bright=0.7, lift={1: 2.6}),
}

for name, mark in MARKS.items():
    svg = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {S} {S}" width="{S}" height="{S}">
  <defs>
    <radialGradient id="light" cx="50%" cy="36%" r="78%">
      <stop offset="0.55" stop-color="#000000" stop-opacity="0"/><stop offset="1" stop-color="#000000" stop-opacity="0.45"/>
    </radialGradient>
    <linearGradient id="rim" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0" stop-color="#ffffff" stop-opacity="0.22"/><stop offset="1" stop-color="#ffffff" stop-opacity="0.03"/>
    </linearGradient>
    <filter id="tileShadow" x="-10%" y="-10%" width="120%" height="125%">
      <feDropShadow dx="0" dy="12" stdDeviation="14" flood-color="#000" flood-opacity="0.45"/>
    </filter>
    <clipPath id="tile"><rect x="100" y="100" width="824" height="824" rx="185"/></clipPath>
    {ridges.bloom(9)}
  </defs>

  <rect x="100" y="100" width="824" height="824" rx="185" fill="{BG}" filter="url(#tileShadow)"/>
  <g clip-path="url(#tile)">
    {mark}
    <rect x="100" y="100" width="824" height="824" fill="url(#light)"/>
  </g>
  <rect x="101" y="101" width="822" height="822" rx="184" fill="none" stroke="url(#rim)" stroke-width="2"/>
</svg>'''
    open(os.path.join(os.path.dirname(os.path.abspath(__file__)), name), "w").write(svg)
