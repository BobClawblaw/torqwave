#!/usr/bin/env python3
"""Regenerates the comparison matrix and its graphics from the data.

    python3 docs/compare/generate.py                  # MATRIX.md, graphics/*.svg (and *.png when rsvg-convert is found)
    python3 docs/compare/generate.py --posts FILE.md  # counts each ```text block as X counts it, and each ```alt block

Python's standard library only. The data is data/comparison.json (rows, products, method) and one file per
Torqwave release in data/snapshots/. The newest snapshot is drawn; every snapshot feeds the trend.
"""
import json, math, os, re, shutil, subprocess, sys
from html import escape

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(HERE, "data")
OUT = os.path.join(HERE, "graphics")

# Studio Obsidian + Torqwave Hot Orange (#ff5c1f) brand chrome + Emerald Green matrix semantics.
BG_TOP, BG_MID, BG_BOT = "#07090e", "#0c1018", "#07090e"
SURFACE, SURFACE_HI = "#111621", "#171e2d"
RAISED, RAISED_HI = "#151c29", "#1d2638"
LINE, LINE2 = "#232d40", "#37445c"
WHITE, TEXT, DIM, FAINT = "#f8fafc", "#dce3ee", "#929eb3", "#68758c"
BRAND, BRAND_HI, BRAND_DK = "#ff5c1f", "#ff8a3d", "#c84110"
GOOD, GOOD_DK = "#34e8a8", "#18a877"
WARN, WARN_DK = "#f4ca53", "#a88321"
ACCENT, CYAN = "#ff7840", "#ff9f5a"
WAVE = "#ff5c1f"
FONT = "SF Pro Display, -apple-system, BlinkMacSystemFont, Helvetica Neue, Inter, Arial, sans-serif"
W, H = 1600, 900

SHORT = {
    "editing": "Editing", "repair": "Repair", "loudness_analysis": "Loudness",
    "multitrack": "Multitrack", "midi": "MIDI", "instruments": "Instruments",
    "plugins": "Plug-ins", "picture": "Picture", "library": "Library",
    "ai": "AI help", "platform": "Platforms",
}
WORD = {"yes": "Yes", "partial": "Partial", "no": "No", "not confirmed": "Not confirmed", "info": ""}


def vkey(v):
    return tuple(int(x) for x in re.findall(r"\d+", v))


def load():
    defs = json.load(open(os.path.join(DATA, "comparison.json"), encoding="utf-8"))
    snaps = []
    folder = os.path.join(DATA, "snapshots")
    for name in os.listdir(folder):
        if name.endswith(".json"):
            snaps.append(json.load(open(os.path.join(folder, name), encoding="utf-8")))
    snaps.sort(key=lambda s: vkey(s["release"]))
    return defs, snaps


def coverage(cells, rows):
    c = {"yes": 0, "partial": 0, "no": 0, "not confirmed": 0}
    for r in rows:
        c[cells[r["id"]]["value"]] += 1
    return c


def all_rows(defs):
    return [r for g in defs["groups"] for r in g["rows"]]


def label(defs, snap, pid):
    p = snap["products"][pid]
    if pid.startswith("torqwave"):
        return "Torqwave " + p["version"]
    return p["name"]


def own_columns(defs, snap):
    P = snap["products"]
    if "torqwave_prev" in P and any(
        P["torqwave"]["cells"][r["id"]]["value"] != P["torqwave_prev"]["cells"][r["id"]]["value"]
        for r in all_rows(defs)
    ):
        return ["torqwave", "torqwave_prev"]
    return ["torqwave"]


def product_columns(defs, snap):
    own = own_columns(defs, snap)
    return [p["id"] for p in defs["products"] if not p["id"].startswith("torqwave") or p["id"] in own]


def wrap_lines(text, max_chars, max_lines=3):
    words = str(text).split(" ")
    lines, cur = [], ""
    for w in words:
        if cur and len(cur) + len(w) + 1 > max_chars:
            lines.append(cur)
            cur = w
        else:
            cur = (cur + " " + w).strip()
    if cur:
        lines.append(cur)
    return lines[:max_lines]


def split_name(name):
    words = name.split(" ")
    if len(words) == 1:
        return [name]
    best = min(range(1, len(words)), key=lambda i: max(len(" ".join(words[:i])), len(" ".join(words[i:]))))
    return [" ".join(words[:best]), " ".join(words[best:])]


def fmt(n):
    return f"{n:,}"


def delta_text(prev, now):
    if not isinstance(prev, (int, float)) or not isinstance(now, (int, float)):
        return None
    d = now - prev
    if d == 0:
        return None
    sign = "+" if d > 0 else ""
    return f"{sign}{fmt(d)}"


# ---------------------------------------------------------------- SVG builder

class Svg:
    def __init__(self, w=W, h=H, hero_wave=False):
        self.w, self.h = w, h
        self.parts = [
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}" font-family="{FONT}">',
            "<defs>",
            # Canvas background gradient
            '<linearGradient id="bgGrad" x1="0" y1="0" x2="0.4" y2="1">'
            f'<stop offset="0%" stop-color="{BG_TOP}"/>'
            f'<stop offset="52%" stop-color="{BG_MID}"/>'
            f'<stop offset="100%" stop-color="{BG_BOT}"/>'
            "</linearGradient>",
            # Ambient studio glows (Torqwave Hot Orange + warm amber counterweight)
            '<radialGradient id="glowBrand" cx="0.16" cy="0.06" r="0.56">'
            '<stop offset="0%" stop-color="#ff5c1f" stop-opacity="0.18"/>'
            '<stop offset="55%" stop-color="#ff5c1f" stop-opacity="0.045"/>'
            '<stop offset="100%" stop-color="#07090e" stop-opacity="0"/>'
            "</radialGradient>",
            '<radialGradient id="glowWarm" cx="0.88" cy="0.92" r="0.54">'
            '<stop offset="0%" stop-color="#ff8a3d" stop-opacity="0.11"/>'
            '<stop offset="55%" stop-color="#c84110" stop-opacity="0.03"/>'
            '<stop offset="100%" stop-color="#07090e" stop-opacity="0"/>'
            "</radialGradient>",
            # Card gradients
            '<linearGradient id="cardGrad" x1="0" y1="0" x2="0" y2="1">'
            '<stop offset="0%" stop-color="#151b28"/>'
            '<stop offset="100%" stop-color="#0d111a"/>'
            "</linearGradient>",
            '<linearGradient id="panelGrad" x1="0" y1="0" x2="0" y2="1">'
            '<stop offset="0%" stop-color="#171e2c"/>'
            '<stop offset="100%" stop-color="#0e131d"/>'
            "</linearGradient>",
            # Featured Torqwave column gradient (subtle warm studio ember tint)
            '<linearGradient id="tqColGrad" x1="0" y1="0" x2="0" y2="1">'
            '<stop offset="0%" stop-color="#291914" stop-opacity="0.96"/>'
            '<stop offset="100%" stop-color="#14141d" stop-opacity="0.94"/>'
            "</linearGradient>",
            # Torqwave Hot Orange -> Warm Amber brand accent gradients
            '<linearGradient id="accentH" x1="0" y1="0" x2="1" y2="0">'
            '<stop offset="0%" stop-color="#ff5c1f"/>'
            '<stop offset="65%" stop-color="#ff7a33"/>'
            '<stop offset="100%" stop-color="#ffa35c"/>'
            "</linearGradient>",
            '<linearGradient id="accentFade" x1="0" y1="0" x2="1" y2="0">'
            '<stop offset="0%" stop-color="#ff5c1f" stop-opacity="0.92"/>'
            '<stop offset="45%" stop-color="#ff8a3d" stop-opacity="0.45"/>'
            '<stop offset="100%" stop-color="#ff8a3d" stop-opacity="0"/>'
            "</linearGradient>",
            '<linearGradient id="barYes" x1="0" y1="0" x2="1" y2="0">'
            '<stop offset="0%" stop-color="#1fa87a"/>'
            '<stop offset="100%" stop-color="#34e8a8"/>'
            "</linearGradient>",
            '<linearGradient id="barYesV" x1="0" y1="1" x2="0" y2="0">'
            '<stop offset="0%" stop-color="#147a5b"/>'
            '<stop offset="65%" stop-color="#22bd89"/>'
            '<stop offset="100%" stop-color="#34e8a8"/>'
            "</linearGradient>",
            '<linearGradient id="barPart" x1="0" y1="0" x2="1" y2="0">'
            '<stop offset="0%" stop-color="#8b6918"/>'
            '<stop offset="100%" stop-color="#bf9226"/>'
            "</linearGradient>",
            '<linearGradient id="kpiText" x1="0" y1="0" x2="1" y2="1">'
            '<stop offset="0%" stop-color="#ff6e35"/>'
            '<stop offset="100%" stop-color="#ffa159"/>'
            "</linearGradient>",
            '<linearGradient id="pillBg" x1="0" y1="0" x2="1" y2="0">'
            '<stop offset="0%" stop-color="#301810"/>'
            '<stop offset="100%" stop-color="#241512"/>'
            "</linearGradient>",
            '<linearGradient id="deltaBg" x1="0" y1="0" x2="1" y2="0">'
            '<stop offset="0%" stop-color="#113025"/>'
            '<stop offset="100%" stop-color="#112829"/>'
            "</linearGradient>",
            # Subtle studio grid pattern
            '<pattern id="studioGrid" width="48" height="48" patternUnits="userSpaceOnUse">'
            '<path d="M 48 0 L 0 0 0 48" fill="none" stroke="#ffffff" stroke-opacity="0.025" stroke-width="1"/>'
            "</pattern>",
            "</defs>",
            f'<rect width="{w}" height="{h}" fill="url(#bgGrad)"/>',
            f'<rect width="{w}" height="{h}" fill="url(#studioGrid)"/>',
            f'<rect width="{w}" height="{h}" fill="url(#glowBrand)"/>',
            f'<rect width="{w}" height="{h}" fill="url(#glowWarm)"/>',
        ]
        if hero_wave:
            self._draw_hero_waveform()
        else:
            self._draw_header_waveform()

    def _draw_header_waveform(self):
        # Delicate harmonic ribbon sweeping across the upper-right header
        pts1, pts2 = [], []
        for x in range(880, self.w + 20, 12):
            t = (x - 880) / (self.w - 880)
            env = math.sin(t * math.pi) ** 1.2
            y1 = 74 + env * (22 * math.sin(t * 14.0) + 9 * math.sin(t * 31.0))
            y2 = 74 + env * (15 * math.cos(t * 11.0 + 0.8) + 6 * math.sin(t * 23.0))
            pts1.append(f"{x},{y1:.1f}")
            pts2.append(f"{x},{y2:.1f}")
        self.parts.append(
            f'<polyline points="{" ".join(pts1)}" fill="none" stroke="url(#accentH)" stroke-opacity="0.26" stroke-width="2"/>'
        )
        self.parts.append(
            f'<polyline points="{" ".join(pts2)}" fill="none" stroke="#ff8a3d" stroke-opacity="0.14" stroke-width="1.5"/>'
        )

    def _draw_hero_waveform(self):
        # Rich multi-oscillator harmonic ribbons across the hero header backdrop
        for idx, (amp, freq1, freq2, phase, op, sw, col) in enumerate((
            (54, 9.5, 23.0, 0.0, 0.34, 2.5, "url(#accentH)"),
            (36, 7.2, 17.0, 1.1, 0.20, 1.8, "#ff8a3d"),
            (24, 13.0, 29.0, 2.3, 0.14, 1.3, "#ff5c1f"),
        )):
            pts = []
            for x in range(560, self.w + 20, 10):
                t = (x - 560) / (self.w - 560)
                env = (math.sin(min(1.0, t * 1.05) * math.pi) ** 1.1) * (0.35 + 0.65 * t)
                y = 185 + env * (amp * math.sin(t * freq1 + phase) + (amp * 0.35) * math.sin(t * freq2 - phase))
                pts.append(f"{x},{y:.1f}")
            self.parts.append(
                f'<polyline points="{" ".join(pts)}" fill="none" stroke="{col}" stroke-opacity="{op}" stroke-width="{sw}"/>'
            )

    def raw(self, s):
        self.parts.append(s)

    def rect(self, x, y, w, h, fill, rx=0, stroke=None, dash=None, sw=1.2, opacity=None):
        s = f' stroke="{stroke}" stroke-width="{sw}"' if stroke else ""
        d = f' stroke-dasharray="{dash}"' if dash else ""
        o = f' opacity="{opacity}"' if opacity is not None else ""
        self.parts.append(f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" rx="{rx}" fill="{fill}"{s}{d}{o}/>')

    def line(self, x1, y1, x2, y2, stroke=LINE, width=1, opacity=None):
        o = f' opacity="{opacity}"' if opacity is not None else ""
        self.parts.append(f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" stroke="{stroke}" stroke-width="{width}"{o}/>')

    def text(self, x, y, s, size=20, fill=TEXT, anchor="start", weight="normal", turn=0, ls=None):
        t = f' transform="rotate({turn} {x:.1f} {y:.1f})"' if turn else ""
        l = f' letter-spacing="{ls}"' if ls else ""
        self.parts.append(
            f'<text x="{x:.1f}" y="{y:.1f}" font-size="{size}" fill="{fill}" text-anchor="{anchor}" '
            f'font-weight="{weight}"{t}{l}>{escape(str(s))}</text>'
        )

    def pill(self, x, y, label_text, fill="url(#pillBg)", stroke="#ff5c1f", text_col="#ff8a4d", size=14, h=28, pad_x=14):
        pw = max(60, len(label_text) * (size * 0.62) + pad_x * 2)
        self.rect(x, y, pw, h, fill, h / 2, stroke, sw=1.1)
        self.text(x + pw / 2, y + h / 2 + size * 0.35, label_text, size, text_col, "middle", "bold", ls="0.06em")
        return pw

    def mark(self, cx, cy, value, r=12, highlight=False):
        if value == "yes":
            if highlight:
                # Warm Torqwave Hot Orange halo highlighting a brand-new row in this release
                self.parts.append(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{r + 5:.1f}" fill="#ff5c1f" fill-opacity="0.20" stroke="#ff7a3d" stroke-opacity="0.85" stroke-width="1.5"/>')
            self.parts.append(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{r:.1f}" fill="url(#barYes)"/>')
            # Crisp dark checkmark inside
            s = r / 12.0
            self.parts.append(
                f'<path d="M {cx - 4.6*s:.1f} {cy + 0.4*s:.1f} L {cx - 1.3*s:.1f} {cy + 3.8*s:.1f} L {cx + 5.0*s:.1f} {cy - 3.6*s:.1f}" '
                f'fill="none" stroke="#071512" stroke-width="{2.3*s:.1f}" stroke-linecap="round" stroke-linejoin="round"/>'
            )
        elif value == "partial":
            self.parts.append(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{r - 0.8:.1f}" fill="#261f0d" stroke="{WARN}" stroke-width="1.8"/>')
            self.parts.append(f'<path d="M {cx:.1f} {cy - r + 1.5:.1f} A {r - 1.5:.1f} {r - 1.5:.1f} 0 0 0 {cx:.1f} {cy + r - 1.5:.1f} Z" fill="{WARN}"/>')
        elif value == "no":
            self.parts.append(f'<line x1="{cx - 7:.1f}" y1="{cy:.1f}" x2="{cx + 7:.1f}" y2="{cy:.1f}" stroke="{LINE2}" stroke-width="2.6" stroke-linecap="round"/>')
        else:
            self.parts.append(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{r - 1:.1f}" fill="#171f2e" stroke="{LINE2}" stroke-width="1.2"/>')
            self.text(cx, cy + 5.5, "?", 15, DIM, "middle", "bold")

    def legend(self, x, y, show_new=False):
        items = [("yes", "Yes"), ("partial", "Partial"), ("no", "No"), ("not confirmed", "Not confirmed")]
        for value, word in items:
            self.mark(x + 10, y - 5, value, 9)
            self.text(x + 27, y, word, 15, DIM, weight="500")
            x += 46 + len(word) * 8.6
        if show_new:
            self.mark(x + 12, y - 5, "yes", 9, highlight=True)
            self.text(x + 31, y, "New in this release", 15, BRAND_HI, weight="600")

    def brand_header(self, title, sub, snap, right_badge=None):
        # Top meta row: TORQ (white) + WAVE (Torqwave Hot Orange) + release pill + right badge
        self.rect(50, 26, 4.5, 20, BRAND, 2.2)
        self.text(62, 43, "TORQ", 16, WHITE, weight="800", ls="0.14em")
        self.text(117, 43, "WAVE", 16, BRAND, weight="800", ls="0.14em")
        self.pill(188, 24, f'v{snap["release"]}', size=12, h=24, pad_x=11)
        badge = right_badge or "FREE · OPEN SOURCE · macOS / WIN / LINUX"
        self.text(self.w - 50, 42, badge, 13, DIM, "end", "600", ls="0.09em")
        # Title & subtitle
        self.text(50, 92, title, 40, WHITE, weight="800", ls="-0.02em")
        self.text(50, 124, sub, 20, DIM, weight="400")
        # Thin gradient rule under header
        self.rect(50, 138, self.w - 100, 1.5, "url(#accentFade)")

    def footer(self, snap):
        self.line(50, self.h - 52, self.w - 50, self.h - 52, LINE, 1, 0.7)
        self.text(
            self.w - 50, self.h - 24,
            f'As of {snap["as_of"]} · Torqwave {snap["release"]} ({snap["status"]}) · Sources & method: docs/compare',
            14, FAINT, "end"
        )

    def icon(self, kind, cx, cy):
        # Minimal crisp geometric vector badges for the hero cards (warm studio ember badge)
        self.rect(cx - 20, cy - 20, 40, 40, "#241611", 10, "#5e301e", sw=1.1)
        if kind == "fx":
            # Three equalizer sliders
            for dx, ky in ((-9, -4), (0, 5), (9, -7)):
                self.line(cx + dx, cy - 11, cx + dx, cy + 11, "#6b4738", 2)
                self.raw(f'<circle cx="{cx + dx}" cy="{cy + ky}" r="3.4" fill="{BRAND}"/>')
        elif kind == "synth":
            # Saw / harmonic wave
            self.raw(f'<polyline points="{cx-12},{cy+6} {cx-4},{cy-8} {cx-4},{cy+6} {cx+4},{cy-8} {cx+4},{cy+6} {cx+12},{cy-8}" fill="none" stroke="{BRAND_HI}" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"/>')
        elif kind == "midi":
            # Piano roll notes
            self.rect(cx - 11, cy - 8, 9, 4, BRAND_HI, 2)
            self.rect(cx - 2, cy - 2, 13, 4, BRAND, 2)
            self.rect(cx - 8, cy + 4, 11, 4, WARN, 2)
        elif kind == "wave":
            # Audio waveform -> note
            for dx, hh in ((-11, 4), (-7, 9), (-3, 6), (1, 11), (5, 5), (9, 8)):
                self.raw(f'<line x1="{cx+dx}" y1="{cy-hh}" x2="{cx+dx}" y2="{cy+hh}" stroke="{BRAND_HI}" stroke-width="2.2" stroke-linecap="round"/>')
        elif kind == "drums":
            # 2x2 drum pads
            for dx in (-8, 2):
                for dy in (-8, 2):
                    col = BRAND if (dx == -8 and dy == -8) or (dx == 2 and dy == 2) else "#53362a"
                    self.rect(cx + dx, cy + dy, 7, 7, col, 2)
        else:
            # Modular node / CLAP plug-in connector
            self.raw(f'<circle cx="{cx - 7}" cy="{cy}" r="4.5" fill="none" stroke="{BRAND}" stroke-width="2"/>')
            self.raw(f'<circle cx="{cx + 7}" cy="{cy}" r="4.5" fill="none" stroke="{BRAND_HI}" stroke-width="2"/>')
            self.line(cx - 2.5, cy, cx + 2.5, cy, WHITE, 2)

    def save(self, name):
        self.parts.append("</svg>")
        path = os.path.join(OUT, name)
        with open(path, "w", encoding="utf-8") as f:
            f.write("\n".join(self.parts) + "\n")
        return path


# ---------------------------------------------------------------- graphics

def graphic_hero(defs, snap):
    """Flagship release announcement card (1600x900) designed for X.com & release pages."""
    s = Svg(W, H, hero_wave=True)

    # Top bar
    s.pill(56, 48, f'RELEASE v{snap["release"]}', size=14, h=30, pad_x=16)
    s.text(W - 56, 68, "FREE · OPEN SOURCE (AGPL-3.0) · macOS / WINDOWS / LINUX", 14, DIM, "end", "600", ls="0.10em")

    # Giant Display Title
    s.text(56, 174, "TORQ", 86, WHITE, weight="800", ls="-0.03em")
    s.text(294, 174, "WAVE", 86, "url(#kpiText)", weight="800", ls="-0.03em")
    s.text(562, 174, snap["release"], 86, DIM, weight="300", ls="-0.02em")

    tagline = snap.get("hero_tagline", "An audio editor and DAW in one — spectral repair, mastering, instruments & multitrack")
    s.text(58, 218, tagline, 24, TEXT, weight="400")

    # 4 Hero KPI pills strip
    rows = all_rows(defs)
    cov = coverage(snap["products"]["torqwave"]["cells"], rows)
    prev_cov = coverage(snap["products"]["torqwave_prev"]["cells"], rows) if "torqwave_prev" in snap["products"] else None
    cov_str = f'{cov["yes"]}/{len(rows)} Matrix Rows' + (f' (+{cov["yes"] - prev_cov["yes"]})' if prev_cov and cov["yes"] > prev_cov["yes"] else "")
    chips = [
        ("32 Built-In FX & Instruments", BRAND),
        (cov_str, GOOD),
        ("CLAP 1.2 · VST3 · AU · LV2", BRAND_HI),
        ("2,782 Verified Checks", GOOD),
    ]
    cx = 56
    for txt, dot_col in chips:
        cw = len(txt) * 9.8 + 44
        s.rect(cx, 246, cw, 38, "#121a2a", 19, "#24344d", sw=1.1)
        s.raw(f'<circle cx="{cx + 18:.1f}" cy="265" r="4.5" fill="{dot_col}"/>')
        s.text(cx + 32, 270, txt, 16, WHITE, weight="600")
        cx += cw + 14

    # 2x3 Bento Grid of feature pillars
    cards = snap.get("hero_cards")
    if not cards:
        cards = [{"icon": "fx", "title": item[:36], "body": item} for item in snap.get("release_firsts", [])[:6]]

    gx0, gy0, cw, ch, gap_x, gap_y = 56, 316, 480, 244, 24, 24
    for i, card in enumerate(cards[:6]):
        col, row = i % 3, i // 3
        x = gx0 + col * (cw + gap_x)
        y = gy0 + row * (ch + gap_y)
        s.rect(x, y, cw, ch, "url(#cardGrad)", 16, LINE, sw=1.2)
        # Subtle top luminous edge
        s.rect(x + 18, y, 96, 3, "url(#accentH)", 1.5)
        s.icon(card.get("icon", "fx"), x + 44, y + 48)
        s.text(x + 78, y + 56, card["title"], 23, WHITE, weight="700", ls="-0.01em")
        lines = wrap_lines(card["body"], 38, max_lines=4)
        for j, ln in enumerate(lines):
            s.text(x + 26, y + 106 + j * 30, ln, 19, DIM, weight="400")

    # Bottom footer callout
    s.line(56, H - 56, W - 56, H - 56, LINE, 1, 0.7)
    s.text(56, H - 24, "bobclawblaw.github.io/torqwave  ·  1,243 built-in audio clips  ·  No account, no subscription", 16, TEXT, weight="500")
    s.text(W - 56, H - 24, f'Released {snap["as_of"]} · AGPL-3.0', 15, FAINT, "end")
    return s.save("0-release-hero.svg")


def graphic_numbers(defs, snap):
    s = Svg()
    s.brand_header(
        f'Torqwave {snap["release"]}: what it adds over {snap["previous"]}',
        "By the numbers, counted directly from the release's build, tests and documentation",
        snap,
    )
    nums = snap["release_numbers"]
    x0, y0, tw, th, gap = 50, 166, 312, 310, 20
    for i, n in enumerate(nums[:6]):
        x, y = x0 + (i % 3) * (tw + gap), y0 + (i // 3) * (th + gap)
        s.rect(x, y, tw, th, "url(#cardGrad)", 14, LINE, sw=1.2)
        s.rect(x + 16, y, tw - 32, 3.5, "url(#accentH)", 2)
        # Delta pill top-right
        d = delta_text(n["prev"], n["now"])
        if d:
            pw = max(54, len(d) * 9.5 + 22)
            s.rect(x + tw - pw - 18, y + 20, pw, 28, "url(#deltaBg)", 14, "#279e78", sw=1)
            s.text(x + tw - pw / 2 - 18, y + 39, d, 15, GOOD, "middle", "700")
        s.text(x + 24, y + 114, fmt(n["now"]), 78, "url(#kpiText)", weight="800", ls="-0.03em")
        s.text(x + 24, y + 158, n["label"], 25, WHITE, weight="700")
        s.text(x + 24, y + 190, f'{snap["previous"]}: {fmt(n["prev"])}', 18, DIM, weight="500")
        s.line(x + 24, y + 208, x + tw - 24, y + 208, LINE, 1, 0.7)
        for j, ln in enumerate(wrap_lines(n["note"], 31, max_lines=3)):
            s.text(x + 24, y + 236 + j * 23, ln, 16, FAINT)

    # Right spotlight panel
    rx = x0 + 3 * (tw + gap) + 8
    rw = W - 50 - rx
    rh = 2 * th + gap
    s.rect(rx, y0, rw, rh, "url(#panelGrad)", 14, LINE, sw=1.2)
    s.rect(rx + 20, y0, 120, 3.5, "url(#accentH)", 2)
    s.text(rx + 28, y0 + 50, f'Highlights since {snap["previous"]}', 25, WHITE, weight="700")
    s.line(rx + 28, y0 + 68, rx + rw - 28, y0 + 68, LINE, 1, 0.8)

    items = snap["release_firsts"]
    y = y0 + 104
    many = len(items) > 6
    size, step, space, wrap_c = (19, 26, 18, 41) if many else (20, 28, 22, 38)
    for item in items:
        lines = wrap_lines(item, wrap_c, max_lines=3)
        # Glowing diamond bullet
        bx, by = rx + 34, y - 9
        s.raw(f'<polygon points="{bx},{by-5} {bx+5},{by} {bx},{by+5} {bx-5},{by}" fill="url(#accentH)"/>')
        for ln in lines:
            s.text(rx + 52, y - 2, ln, size, TEXT)
            y += step
        y += space

    s.footer(snap)
    return s.save("1-release-by-the-numbers.svg")


def graphic_matrix(defs, snap, m):
    s = Svg()
    s.brand_header(
        m["title"].replace("{release}", snap["release"]),
        "What ships built into each product, read from its own documentation",
        snap,
    )
    rows = [r for r in all_rows(defs) if m["rows"] in r["graphics"]]
    own = own_columns(defs, snap)
    nt = len(own)
    pids = own + [p["id"] for p in defs["products"] if p.get("matrix") == m["id"]]
    lx, cx0, top, bottom = 50, 435, 158, H - 70
    cw = (W - 50 - cx0) / len(pids)
    head = 74
    rh = (bottom - top - head) / len(rows)

    # Outer table frame
    s.rect(lx - 12, top, W - 2 * lx + 24, bottom - top, "#0c111b", 12, LINE, sw=1, opacity="0.65")
    # Highlighted Torqwave column(s)
    s.rect(cx0, top, cw * nt, bottom - top, "url(#tqColGrad)", 10, "#5c3322", sw=1.2)
    s.rect(cx0 + 12, top, cw * nt - 24, 3.5, "url(#accentH)", 2)

    for i, pid in enumerate(pids):
        x = cx0 + cw * (i + 0.5)
        p = snap["products"][pid]
        if pid.startswith("torqwave"):
            is_cur = pid == "torqwave"
            s.text(x, top + 34, "Torqwave", 20, WHITE, "middle", "700")
            s.text(x, top + 58, p["version"], 18, BRAND_HI if is_cur else DIM, "middle", "700")
        else:
            lines = split_name(p["name"])
            for j, ln in enumerate(lines):
                s.text(x, top + (33 if len(lines) > 1 else 46) + j * 23, ln, 18, TEXT, "middle", "600")

    s.line(lx - 12, top + head, W - lx + 12, top + head, LINE, 1)

    has_new = False
    for k, r in enumerate(rows):
        y = top + head + k * rh
        if k % 2 == 0:
            s.rect(lx - 10, y, cx0 - lx + 10, rh, "#121a29", 0, opacity="0.65")
            s.rect(cx0 + cw * nt, y, W - 50 - cx0 - cw * nt + 10, rh, "#121a29", 0, opacity="0.65")
        prev_v = snap["products"]["torqwave_prev"]["cells"][r["id"]]["value"] if nt == 2 else None
        cur_v = snap["products"]["torqwave"]["cells"][r["id"]]["value"]
        flipped = prev_v is not None and cur_v == "yes" and prev_v != "yes"
        if flipped:
            has_new = True
            s.rect(lx - 10, y + 1, cx0 - lx + 8, rh - 2, "#2c1812", 4, opacity="0.78")
        s.text(lx + 4, y + rh / 2 + 6, r["title"], 19, WHITE if flipped else TEXT, weight="600" if flipped else "400")
        for i, pid in enumerate(pids):
            val = snap["products"][pid]["cells"][r["id"]]["value"]
            s.mark(cx0 + cw * (i + 0.5), y + rh / 2, val, r=10.5, highlight=(pid == "torqwave" and flipped))

    s.footer(snap)
    s.legend(50, H - 24, show_new=has_new)
    return s.save(m["file"] + ".svg")


def graphic_whole(defs, snap):
    pids = product_columns(defs, snap)
    nt = len(own_columns(defs, snap))
    groups = defs["groups"]
    lx, cx0, top, head, rh, gh = 50, 420, 158, 256, 30, 44
    h = top + head + sum(gh + rh * len(g["rows"]) for g in groups) + 92
    s = Svg(W, h)
    s.brand_header(
        f'Torqwave {snap["release"]} against the alternatives: full matrix',
        f'{len(all_rows(defs))} rows across {len(groups)} areas, {len(pids) - nt} products, each from its own documentation',
        snap,
    )
    cw = (W - 50 - cx0) / len(pids)
    bottom = h - 72
    s.rect(cx0, top, cw * nt, bottom - top, "url(#tqColGrad)", 10, "#5c3322", sw=1.2)
    s.rect(cx0 + 8, top, cw * nt - 16, 3.5, "url(#accentH)", 2)

    group_of = {p["id"]: p["group"] for p in defs["products"]}
    first_daw = next(i for i, pid in enumerate(pids) if group_of[pid] == "daw")
    for i, pid in enumerate(pids):
        x = cx0 + cw * (i + 0.5)
        own = pid.startswith("torqwave")
        s.text(
            x + 6, top + head - 14, label(defs, snap, pid), 18,
            (BRAND_HI if pid == "torqwave" else WHITE) if own else TEXT, "start", "700" if own else "600", -90
        )

    y = top + head
    has_new = False
    for g in groups:
        s.rect(lx - 10, y + 7, W - 2 * lx + 20, gh - 12, "#192438", 6, LINE, sw=0.8)
        s.text(lx + 4, y + gh - 14, g["title"], 18, WHITE, weight="700")
        y += gh
        for k, r in enumerate(g["rows"]):
            if k % 2 == 0:
                s.rect(lx - 10, y, cx0 - lx + 10, rh, "#111827", opacity="0.65")
                s.rect(cx0 + cw * nt, y, W - 50 - cx0 - cw * nt + 10, rh, "#111827", opacity="0.65")
            prev_v = snap["products"]["torqwave_prev"]["cells"][r["id"]]["value"] if nt == 2 else None
            cur_v = snap["products"]["torqwave"]["cells"][r["id"]]["value"]
            flipped = prev_v is not None and cur_v == "yes" and prev_v != "yes"
            if flipped:
                has_new = True
            s.text(lx + 4, y + rh / 2 + 6, r["title"], 17, WHITE if flipped else TEXT, weight="600" if flipped else "400")
            for i, pid in enumerate(pids):
                s.mark(cx0 + cw * (i + 0.5), y + rh / 2, snap["products"][pid]["cells"][r["id"]]["value"], 8.5, highlight=(pid == "torqwave" and flipped))
            y += rh

    for i in (nt, first_daw):
        s.line(cx0 + cw * i, top + head - 6, cx0 + cw * i, bottom, LINE2, 1)
    s.text(cx0 + cw * (nt + (first_daw - nt) / 2), top + 24, "EDITORS", 14, DIM, "middle", "700", ls="0.12em")
    s.text(cx0 + cw * (first_daw + (len(pids) - first_daw) / 2), top + 24, "DAWS", 14, DIM, "middle", "700", ls="0.12em")
    s.footer(snap)
    s.legend(50, h - 24, show_new=has_new)
    return s.save(f"{len(defs['matrices']) + 3}-the-whole-matrix.svg"), h


def trend_points(defs, snaps, announced_only=False):
    rows = all_rows(defs)
    pts = []
    for i, snap in enumerate(snaps):
        if i == 0 and "torqwave_prev" in snap["products"]:
            pts.append((snap["products"]["torqwave_prev"]["version"], coverage(snap["products"]["torqwave_prev"]["cells"], rows)))
        if announced_only and snap.get("announced") is False:
            continue
        pts.append((snap["products"]["torqwave"]["version"], coverage(snap["products"]["torqwave"]["cells"], rows)))
    if announced_only:
        pts = [p for p in pts if vkey(p[0]) >= vkey(snaps[-1]["previous"])]
    return pts, len(rows)


def standing_trend(s, pts, total, px, pw, top, bottom):
    s.text(px + 20, top + 36, "Release trend", 20, WHITE, weight="700")
    s.text(px + 20, top + 58, f"rows covered, of {total}", 14, DIM)
    if len(pts) >= 2:
        diff = pts[-1][1]["yes"] - pts[0][1]["yes"]
        if diff > 0:
            s.pill(px + pw - 78, top + 18, f"+{diff}", fill="url(#deltaBg)", stroke="#279e78", text_col=GOOD, size=13, h=24, pad_x=10)
    slots = max(2, len(pts))
    shown = pts[-slots:]
    bx0, by0, bh = px + 20, top + 92, bottom - top - 152
    bw = (pw - 40) / slots
    for k in range(slots):
        x = bx0 + k * bw + 6
        w = bw - 12
        if k < len(shown):
            ver, c = shown[k]
            is_last = k == len(shown) - 1
            hy, hp = bh * c["yes"] / total, bh * c["partial"] / total
            s.rect(x, by0, w, bh, "#0a0f18", 8, "#254d58" if is_last else LINE, sw=1.2)
            s.rect(x + 1, by0 + bh - hy, w - 2, hy, "url(#barYesV)" if is_last else "#1b6b54", 7)
            if hp:
                s.rect(x + 1, by0 + bh - hy - hp, w - 2, hp, "url(#barPart)", 0)
            s.text(x + w / 2, by0 + bh - hy - hp - 10, c["yes"], 20, GOOD if is_last else TEXT, "middle", "800")
            s.text(x + w / 2, by0 + bh + 26, ver, 14, WHITE if is_last else DIM, "middle", "700" if is_last else "500")
        else:
            s.rect(x, by0, w, bh, "none", 8, LINE, "4 5")


def standing_gaps(s, defs, snap, px, pw, top):
    cells = snap["products"]["torqwave"]["cells"]
    rows = all_rows(defs)
    c = coverage(cells, rows)
    s.text(px + 20, top + 36, f'Torqwave {snap["release"]}', 20, WHITE, weight="700")
    s.text(px + 20, top + 58, f"of {len(rows)} rows", 14, DIM)
    s.text(px + 20, top + 130, c["yes"], 70, "url(#kpiText)", weight="800")
    s.text(px + 20, top + 158, "rows covered", 16, TEXT)
    y = top + 206
    for value, word, colour in (("partial", "Partial", WARN), ("no", "No", DIM)):
        names = [r["title"] for r in rows if cells[r["id"]]["value"] == value]
        if not names:
            continue
        s.text(px + 20, y, f"{word}: {len(names)}", 16, colour, weight="700")
        y += 26
        for name in names:
            for ln in wrap_lines(name, 24, max_lines=2):
                s.text(px + 20, y, ln, 15, TEXT)
                y += 20
            y += 6
        y += 16


def graphic_standing(defs, snaps):
    snap = snaps[-1]
    s = Svg()
    s.brand_header(
        "Where Torqwave stands, area by area",
        "Rows covered in each area of the 53-row matrix. A count of documented features, not of depth or maturity.",
        snap,
    )
    pids = product_columns(defs, snap)
    lx, gx, top, bottom = 50, 282, 158, H - 70
    cw, head = 90, 58
    rh = (bottom - top - head) / len(pids)
    fs = 19 if rh >= 42 else 17

    for j, g in enumerate(defs["groups"]):
        x = gx + j * cw
        s.text(x + cw / 2, top + 24, SHORT.get(g["id"], g["title"]), 14, TEXT, "middle", "700")
        s.text(x + cw / 2, top + 43, f'{len(g["rows"])} rows', 12, FAINT, "middle")

    for i, pid in enumerate(pids):
        y = top + head + i * rh
        mine = pid.startswith("torqwave")
        is_cur = pid == "torqwave"
        if mine:
            s.rect(lx - 12, y + 1, gx + len(defs["groups"]) * cw - lx + 16, rh - 2, "url(#tqColGrad)" if is_cur else "#141d2d", 6)
            if is_cur:
                s.rect(lx - 12, y + 4, 4, rh - 8, "url(#accentH)", 2)
        s.text(lx + 2, y + rh / 2 + 6, label(defs, snap, pid), fs, WHITE if mine else TEXT, weight="700" if mine else "400")
        for j, g in enumerate(defs["groups"]):
            c = coverage(snap["products"][pid]["cells"], g["rows"])
            n = len(g["rows"])
            pad = 6 if rh >= 42 else 4
            x, bw, bh = gx + j * cw + 5, cw - 10, rh - 2 * pad
            by = y + pad
            s.rect(x, by, bw, bh, "#0a0f18" if mine else "#101623", 6, "#264654" if is_cur else LINE, sw=1)
            wy, wp, wu = bw * c["yes"] / n, bw * c["partial"] / n, bw * c["not confirmed"] / n
            if wy:
                s.rect(x, by, wy, bh, "url(#barYes)" if is_cur else "#248763", 6)
            if wp:
                s.rect(x + wy, by, wp, bh, "url(#barPart)", 0)
            if wu:
                s.rect(x + bw - wu, by, wu, bh, LINE2, 0)
            s.text(x + bw / 2, by + bh / 2 + 5.5, f'{c["yes"]}/{n}', fs - 2, WHITE, "middle", "700")

    pts, total = trend_points(defs, snaps, announced_only=True)
    px = gx + len(defs["groups"]) * cw + 26
    pw = W - 50 - px
    s.rect(px, top, pw, bottom - top, "url(#panelGrad)", 14, LINE, sw=1.2)
    s.rect(px + 18, top, pw - 36, 3.5, "url(#accentH)", 2)
    if len({(c["yes"], c["partial"]) for _, c in pts}) > 1:
        standing_trend(s, pts, total, px, pw, top, bottom)
    else:
        standing_gaps(s, defs, snap, px, pw, top)

    s.footer(snap)
    x = 50
    for col, word in (("url(#barYes)", "Yes (count)"), ("url(#barPart)", "Partial"), (LINE2, "Not confirmed"), ("#101623", "No")):
        s.rect(x, H - 37, 16, 16, col, 4, LINE)
        s.text(x + 24, H - 24, word, 15, DIM)
        x += 42 + len(word) * 8.6
    return s.save(f"{len(defs['matrices']) + 2}-where-torqwave-stands.svg")


# ---------------------------------------------------------------- the Markdown matrix

def md_escape(t):
    return str(t).replace("|", "\\|").replace("\n", " ")


def matrix_md(defs, snaps):
    snap = snaps[-1]
    P = snap["products"]
    pids = [p["id"] for p in defs["products"]]
    comp = [p for p in pids if not p.startswith("torqwave")]
    rows = all_rows(defs)
    o = []
    o.append("# Torqwave against the alternatives\n")
    o.append("Generated by `generate.py` from `data/`. Do not edit by hand: change the data and run the script.\n")
    o.append(f'**As of {snap["as_of"]}.** Torqwave {snap["release"]} is {snap["status"]}; its column describes this repository '
             f'{"at that release" if snap["status"] == "released" else "before the release tag"}. '
             + snap.get("previous_note", f'The {snap["previous"]} column is the release before it.') + "\n")
    o.append("## How to read it\n")
    for k, v in defs["values"].items():
        o.append(f"- **{WORD.get(k) or 'Info'}:** {v}")
    o.append("")
    for m in defs["method"]:
        o.append(f"- {m}")
    o.append("\n## Products\n")
    o.append("| Product | Version assessed | Edition | Looked up | Why it is here |")
    o.append("|---|---|---|---|---|")
    why = {p["id"]: p["why"] for p in defs["products"]}
    for pid in pids:
        p = P[pid]
        date = f' ({p["version_date"]})' if p.get("version_date") else ""
        o.append(f'| {label(defs, snap, pid)} | {md_escape(p["version"])}{md_escape(date)} | {md_escape(p["edition"])} | {p["looked_up"]} | {md_escape(why[pid])} |')
    o.append("\nLeft out, and why:\n")
    for l in defs["left_out"]:
        o.append(f'- **{l["name"]}:** {l["why"]}')
    o.append("\n## Graphics\n")
    pics = [
        ("0-release-hero", f'Torqwave {snap["release"]} release overview'),
        ("1-release-by-the-numbers", "What the release adds over the one before, by the numbers"),
    ]
    pics += [(m["file"], m["title"].replace("{release}", snap["release"])) for m in defs["matrices"]]
    pics += [(f"{len(defs['matrices']) + 2}-where-torqwave-stands", "Rows covered in each area by each product, and Torqwave's release trend")]
    for f, alt in pics:
        o.append(f"![{alt}](graphics/{f}.png)\n")
    o.append("## Coverage by area\n")
    o.append("Rows that are yes, with partial rows after a plus sign and rows not confirmed after a question mark. Counts, not a score.\n")
    tq = own_columns(defs, snap)
    sets = [("The editors", tq + [p["id"] for p in defs["products"] if p["group"] == "editor"]),
            ("The DAWs", tq + [p["id"] for p in defs["products"] if p["group"] == "daw"])]
    def cov_text(c):
        t = str(c["yes"])
        if c["partial"]:
            t += f' +{c["partial"]}'
        if c["not confirmed"]:
            t += f' ?{c["not confirmed"]}'
        return t
    for name, cols in sets:
        o.append(f"**{name}**\n")
        o.append("| Area | Rows | " + " | ".join(label(defs, snap, p) for p in cols) + " |")
        o.append("|---|---|" + "---|" * len(cols))
        for g in defs["groups"]:
            o.append(f'| {g["title"]} | {len(g["rows"])} | ' + " | ".join(cov_text(coverage(P[p]["cells"], g["rows"])) for p in cols) + " |")
        o.append(f'| **All rows** | {len(rows)} | ' + " | ".join(cov_text(coverage(P[p]["cells"], rows)) for p in cols) + " |\n")
    pts, total = trend_points(defs, snaps, announced_only=True)
    o.append("\n## Torqwave by release\n")
    o.append("| Release | Yes | Partial | No | Rows |")
    o.append("|---|---|---|---|---|")
    for ver, c in pts:
        o.append(f'| {ver} | {c["yes"]} | {c["partial"]} | {c["no"]} | {total} |')
    quiet = [s["release"] for s in snaps if s.get("announced") is False]
    if quiet:
        names = quiet[0] if len(quiet) == 1 else ", ".join(quiet[:-1]) + " and " + quiet[-1]
        o.append(f'\nThe releases the comparison is drawn for. {names} came out between them; their snapshots are in `data/snapshots/`.')
    o.append("\n## Where Torqwave is behind\n")
    o.append(f"Rows where Torqwave {snap['release']} is no or partial and at least two of the {len(comp)} other products are yes.\n")
    o.append("| Row | Torqwave | Others that are yes |")
    o.append("|---|---|---|")
    for r in rows:
        v = P["torqwave"]["cells"][r["id"]]["value"]
        yes = [p for p in comp if P[p]["cells"][r["id"]]["value"] == "yes"]
        if v in ("no", "partial") and len(yes) >= 2:
            o.append(f'| {r["title"]} | {WORD[v]} | {len(yes)}: {", ".join(P[p]["name"] for p in yes)} |')
    o.append("\nThe facts under [Maturity and terms](#maturity-and-terms) belong here too: Torqwave is new (see First public release), "
             "its installers are not signed or notarized, it has no scripting API, and its release notes say which parts of a release were not tried by hand.\n")
    o.append("## Where Torqwave is ahead\n")
    o.append(f"Rows where Torqwave {snap['release']} is yes and at most two of the {len(comp)} other products are yes.\n")
    o.append("| Row | Others that are yes | Others partial | Not confirmed |")
    o.append("|---|---|---|---|")
    for r in rows:
        v = P["torqwave"]["cells"][r["id"]]["value"]
        vals = {p: P[p]["cells"][r["id"]]["value"] for p in comp}
        yes = [p for p in comp if vals[p] == "yes"]
        if v == "yes" and len(yes) <= 2:
            par = [P[p]["name"] for p in comp if vals[p] == "partial"]
            unc = [P[p]["name"] for p in comp if vals[p] == "not confirmed"]
            o.append(f'| {r["title"]} | {", ".join(P[p]["name"] for p in yes) or "none"} | {", ".join(par) or "none"} | {", ".join(unc) or "none"} |')
    o.append(f"\n## What {snap['release']} changes against {snap['previous']}\n")
    o.append("| Row | " + snap["previous"] + " | " + snap["release"] + " |")
    o.append("|---|---|---|")
    for r in rows:
        a, b = P["torqwave_prev"]["cells"][r["id"]], P["torqwave"]["cells"][r["id"]]
        if a["value"] != b["value"] or a["note"] != b["note"]:
            o.append(f'| {r["title"]} | {WORD[a["value"]]}: {md_escape(a["note"])} | {WORD[b["value"]]}: {md_escape(b["note"])} |')
    o.append("\n| Number | " + snap["previous"] + " | " + snap["release"] + " | |")
    o.append("|---|---|---|---|")
    for n in snap["release_numbers"]:
        o.append(f'| {n["label"]} | {fmt(n["prev"])} | {fmt(n["now"])} | {md_escape(n["note"])} |')
    o.append("\n## The matrix\n")
    o.append(f"![Every row of the matrix against every product: yes, partial, no or not confirmed](graphics/{len(defs['matrices']) + 3}-the-whole-matrix.png)\n")
    o.append("The whole matrix in one picture. The tables under it say the same, row by row, and the notes at the end give each cell's source.\n")
    for g in defs["groups"]:
        o.append(f'### {g["title"]}\n')
        for name, cols in sets:
            o.append("| " + name + " | " + " | ".join(label(defs, snap, p) for p in cols) + " |")
            o.append("|---|" + "---|" * len(cols))
            for r in g["rows"]:
                o.append(f'| {r["title"]} | ' + " | ".join(WORD[P[p]["cells"][r["id"]]["value"]] for p in cols) + " |")
            o.append("")
    o.append("## Maturity and terms\n")
    o.append("Facts, not counted in any coverage figure. Prices are list prices on the date looked up, in the currency the vendor's page showed.\n")
    for r in defs["info_rows"]:
        o.append(f'### {r["title"]}\n')
        for pid in pids:
            c = P[pid]["cells"][r["id"]]
            lead = "" if c["value"] == "info" else WORD[c["value"]] + ". "
            src = f' ([source]({c["source"]}))' if c["source"].startswith("http") else (f' (`{c["source"]}`)' if c["source"] else "")
            o.append(f'- **{label(defs, snap, pid)}:** {lead}{md_escape(c["note"])}{src}')
        o.append("")
    o.append("## Cells not confirmed\n")
    any_unc = False
    for pid in pids:
        ids = [r for r in rows + defs["info_rows"] if P[pid]["cells"][r["id"]]["value"] == "not confirmed"]
        if ids:
            any_unc = True
            o.append(f'- **{label(defs, snap, pid)}** ({len(ids)}): ' + "; ".join(r["title"] for r in ids))
    if not any_unc:
        o.append("None.")
    o.append("\n## Notes and sources\n")
    o.append("Every cell with its note, the page it was read on and what the page said. Row definitions are in `data/comparison.json`.\n")
    for pid in pids:
        p = P[pid]
        o.append(f'### {label(defs, snap, pid)}\n')
        o.append(f'Version {md_escape(p["version"])}, {md_escape(p["edition"])}. Looked up {p["looked_up"]}.' + (f' {p["source_note"]}' if p.get("source_note") else "") + "\n")
        for x in p.get("extra_notes", []):
            src = f' ([source]({x["source"]}))' if x.get("source", "").startswith("http") else ""
            o.append(f'- **{md_escape(x["topic"])}:** {md_escape(x["note"])}{src}')
        if p.get("extra_notes"):
            o.append("")
        o.append("| Row | Value | Note | Source | Evidence |")
        o.append("|---|---|---|---|---|")
        for r in rows + defs["info_rows"]:
            c = p["cells"][r["id"]]
            src = f'[link]({c["source"]})' if c["source"].startswith("http") else (f'`{c["source"]}`' if c["source"] else "")
            o.append(f'| {r["title"]} | {WORD[c["value"]] or "Info"} | {md_escape(c["note"])} | {src} | {md_escape(c.get("evidence", ""))} |')
        o.append("")
    path = os.path.join(HERE, "MATRIX.md")
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(o).rstrip() + "\n")
    return path


# ---------------------------------------------------------------- X post lengths

def x_length(text):
    """Characters as X counts them: a link is 23, and characters outside the Latin ranges count as 2."""
    text = re.sub(r"https?://\S+|<[a-z ]*link>", "\u0001", text.strip())
    n = 0
    for ch in text:
        cp = ord(ch)
        if ch == "\u0001":
            n += 23
        elif cp <= 0x10FF or 0x2000 <= cp <= 0x200D or 0x2010 <= cp <= 0x201F or 0x2032 <= cp <= 0x2037:
            n += 1
        else:
            n += 2
    return n


def count_posts(path):
    text = open(path, encoding="utf-8").read()
    bad = 0
    for i, block in enumerate(re.findall(r"```text\n(.*?)```", text, re.S), 1):
        n = x_length(block)
        first = block.strip().split("\n")[0][:50]
        print(f"{i:2}  {n:3}/280  {'OK  ' if n <= 280 else 'OVER'}  {first}")
        bad += n > 280
    for i, block in enumerate(re.findall(r"```alt\n(.*?)```", text, re.S), 1):
        n = len(block.strip())
        print(f"alt {i}  {n:4}/1000  {'OK  ' if n <= 1000 else 'OVER'}")
        bad += n > 1000
    return bad


def main():
    if len(sys.argv) == 3 and sys.argv[1] == "--posts":
        sys.exit(1 if count_posts(sys.argv[2]) else 0)
    defs, snaps = load()
    snap = snaps[-1]
    ids = [r["id"] for r in all_rows(defs) + defs["info_rows"]]
    for pid in [p["id"] for p in defs["products"]]:
        missing = [i for i in ids if i not in snap["products"][pid]["cells"]]
        if missing:
            sys.exit(f"{pid} has no cell for: {', '.join(missing)}")
    os.makedirs(OUT, exist_ok=True)
    svgs = (
        [graphic_hero(defs, snap), graphic_numbers(defs, snap)]
        + [graphic_matrix(defs, snap, m) for m in defs["matrices"]]
        + [graphic_standing(defs, snaps)]
    )
    whole, tall = graphic_whole(defs, snap)
    svgs.append(whole)
    heights = {whole: tall}
    print("wrote", os.path.relpath(matrix_md(defs, snaps)))
    conv = shutil.which("rsvg-convert")
    for svg in svgs:
        print("wrote", os.path.relpath(svg))
        if conv:
            png = svg[:-4] + ".png"
            subprocess.run([conv, "-w", str(W), "-h", str(heights.get(svg, H)), "-o", png, svg], check=True)
            print("wrote", os.path.relpath(png))
    if not conv:
        print("rsvg-convert not found: SVGs only. Any SVG converter will make the PNGs.")


if __name__ == "__main__":
    main()
