# Writes torqwave-banner.svg (1600 x 900): the mark of ridges.py, a twisted stack of waveforms,
# above the name.
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ridges

W, H = 1600, 900
cx = W / 2
BG = "#050506"
mark = ridges.mark((cx - 545, 40, 1040, 560), lines=64, twist=45, stroke=2.6, background=BG)

svg = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}">
  <defs>
    <radialGradient id="light" cx="50%" cy="38%" r="70%">
      <stop offset="0.55" stop-color="#000000" stop-opacity="0"/><stop offset="1" stop-color="#000000" stop-opacity="0.45"/>
    </radialGradient>
    <linearGradient id="edge" x1="0" y1="0" x2="1" y2="0">
      <stop offset="0" stop-color="{ridges.HOT}" stop-opacity="0"/><stop offset="0.5" stop-color="{ridges.HOT}"/>
      <stop offset="1" stop-color="{ridges.HOT}" stop-opacity="0"/>
    </linearGradient>
    {ridges.bloom(5)}
  </defs>

  <rect width="{W}" height="{H}" fill="{BG}"/>
  {mark}
  <rect width="{W}" height="{H}" fill="url(#light)"/>

  <text x="{cx}" y="732" text-anchor="middle" font-family="Avenir Next, Helvetica Neue, Arial, sans-serif"
        font-weight="800" font-size="104" letter-spacing="22" fill="#f2f2f2">TORQ<tspan dx="18" fill="{ridges.HOT}">WAVE</tspan></text>
  <rect x="{cx-330}" y="766" width="660" height="2" fill="url(#edge)"/>
  <text x="{cx}" y="810" text-anchor="middle" font-family="Avenir Next, Helvetica Neue, Arial, sans-serif"
        font-weight="500" font-size="24" letter-spacing="12" fill="#8a8a90">CUT · SHAPE · RESTORE</text>
</svg>'''
open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "torqwave-banner.svg"), "w").write(svg)
