#!/usr/bin/env python3
"""Regenerates the comparison matrix and its graphics from the data.

    python3 docs/compare/generate.py                  # MATRIX.md, graphics/*.svg (and *.png when rsvg-convert is found)
    python3 docs/compare/generate.py --posts FILE.md  # counts each ```text block as X counts it, and each ```alt block

Python's standard library only. The data is data/comparison.json (rows, products, method) and one file per
Torqwave release in data/snapshots/. The newest snapshot is drawn; every snapshot feeds the trend.
"""
import json, os, re, shutil, subprocess, sys
from html import escape

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(HERE, "data")
OUT = os.path.join(HERE, "graphics")

# The Dark theme's palette (Source/Theme.cpp), so the graphics sit beside the app's screenshots.
BG, SURFACE, RAISED, LINE, LINE2 = "#111317", "#16191d", "#1d2126", "#2a2f36", "#434a54"
WHITE, TEXT, DIM, FAINT = "#ffffff", "#d8dde3", "#8b939e", "#6b737d"
GOOD, WARN, ACCENT, WAVE = "#4cc38a", "#e3c74f", "#4a9fe0", "#4fc3a1"
FONT = "Helvetica Neue, Helvetica, Arial, sans-serif"
W, H = 1600, 900
SHORT = {"editing": "Editing", "repair": "Repair", "loudness_analysis": "Loudness", "multitrack": "Multitrack", "midi": "MIDI",
         "instruments": "Instruments", "plugins": "Plug-ins", "picture": "Picture", "library": "Library", "ai": "AI help",
         "platform": "Platforms"}
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


# ---------------------------------------------------------------- SVG helpers

class Svg:
    def __init__(self, w=W, h=H):
        self.w, self.h = w, h
        self.parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}" font-family="{FONT}">',
                      f'<rect width="{w}" height="{h}" fill="{BG}"/>']

    def rect(self, x, y, w, h, fill, rx=0, stroke=None, dash=None):
        s = f' stroke="{stroke}" stroke-width="1.5"' if stroke else ""
        d = f' stroke-dasharray="{dash}"' if dash else ""
        self.parts.append(f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" rx="{rx}" fill="{fill}"{s}{d}/>')

    def line(self, x1, y1, x2, y2, stroke=LINE, width=1):
        self.parts.append(f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" stroke="{stroke}" stroke-width="{width}"/>')

    def text(self, x, y, s, size=20, fill=TEXT, anchor="start", weight="normal", turn=0):
        t = f' transform="rotate({turn} {x:.1f} {y:.1f})"' if turn else ""
        self.parts.append(f'<text x="{x:.1f}" y="{y:.1f}" font-size="{size}" fill="{fill}" text-anchor="{anchor}" '
                          f'font-weight="{weight}"{t}>{escape(str(s))}</text>')

    def mark(self, cx, cy, value, r=11):
        if value == "yes":
            self.parts.append(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{r}" fill="{GOOD}"/>')
        elif value == "partial":
            self.parts.append(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{r - 1}" fill="none" stroke="{WARN}" stroke-width="2"/>')
            self.parts.append(f'<path d="M {cx:.1f} {cy - r:.1f} A {r} {r} 0 0 0 {cx:.1f} {cy + r:.1f} Z" fill="{WARN}"/>')
        elif value == "no":
            self.line(cx - 8, cy, cx + 8, cy, LINE2, 3)
        else:
            self.text(cx, cy + 7, "?", 21, DIM, "middle", "bold")

    def legend(self, x, y):
        for value, word in (("yes", "yes"), ("partial", "partial"), ("no", "no"), ("not confirmed", "not confirmed")):
            self.mark(x + 10, y - 6, value, 9)
            self.text(x + 28, y, word, 17, DIM)
            x += 46 + len(word) * 9.5

    def frame(self, title, sub, snap):
        self.text(50, 76, title, 44, WHITE, weight="bold")
        self.text(50, 116, sub, 23, DIM)
        self.text(self.w - 50, self.h - 28, f'As of {snap["as_of"]}. Torqwave {snap["release"]} is {snap["status"]}. '
                  'Sources and method: docs/compare in the Torqwave repository.', 17, FAINT, "end")

    def save(self, name):
        self.parts.append("</svg>")
        path = os.path.join(OUT, name)
        with open(path, "w", encoding="utf-8") as f:
            f.write("\n".join(self.parts) + "\n")
        return path


def split_name(name):
    words = name.split(" ")
    if len(words) == 1:
        return [name]
    best = min(range(1, len(words)), key=lambda i: max(len(" ".join(words[:i])), len(" ".join(words[i:]))))
    return [" ".join(words[:best]), " ".join(words[best:])]


def fmt(n):
    return f"{n:,}"


# ---------------------------------------------------------------- the four graphics

def graphic_numbers(defs, snap):
    s = Svg()
    s.frame(f'Torqwave {snap["release"]}: what it adds over {snap["previous"]}', "By the numbers, counted from the release's own files and documentation", snap)
    nums = snap["release_numbers"]
    x0, y0, tw, th, gap = 50, 160, 320, 300, 20
    for i, n in enumerate(nums[:6]):
        x, y = x0 + (i % 3) * (tw + gap), y0 + (i // 3) * (th + gap)
        s.rect(x, y, tw, th, SURFACE, 10, LINE)
        s.rect(x, y, tw, 5, WAVE, 2)
        s.text(x + 24, y + 108, fmt(n["now"]), 84, WAVE, weight="bold")
        s.text(x + 24, y + 156, n["label"], 28, WHITE, weight="bold")
        s.text(x + 24, y + 194, f'{snap["previous"]}: {fmt(n["prev"])}', 22, DIM)
        words, lines, cur = n["note"].split(" "), [], ""
        for w in words:
            if len(cur) + len(w) + 1 > 30:
                lines.append(cur)
                cur = w
            else:
                cur = (cur + " " + w).strip()
        lines.append(cur)
        for j, ln in enumerate(lines[:3]):
            s.text(x + 24, y + 230 + j * 24, ln, 18, FAINT)
    x = x0 + 3 * (tw + gap) + 10
    s.rect(x, y0, W - 50 - x, 2 * th + gap, RAISED, 10, LINE)
    s.text(x + 28, y0 + 52, f'Also new since {snap["previous"]}', 28, WHITE, weight="bold")
    y = y0 + 104
    for item in snap["release_firsts"]:
        words, lines, cur = item.split(" "), [], ""
        for w in words:
            if len(cur) + len(w) + 1 > 34:
                lines.append(cur)
                cur = w
            else:
                cur = (cur + " " + w).strip()
        lines.append(cur)
        s.rect(x + 28, y - 16, 8, 8, ACCENT, 2)
        for ln in lines:
            s.text(x + 50, y - 4, ln, 22, TEXT)
            y += 30
        y += 22
    return s.save("1-release-by-the-numbers.svg")


def graphic_matrix(defs, snap, m):
    s = Svg()
    s.frame(m["title"].replace("{release}", snap["release"]), "What ships in each product, from its own documentation", snap)
    rows = [r for r in all_rows(defs) if m["rows"] in r["graphics"]]
    pids = ["torqwave", "torqwave_prev"] + [p["id"] for p in defs["products"] if p.get("matrix") == m["id"]]
    lx, cx0, top, bottom = 50, 440, 150, H - 70
    cw = (W - 50 - cx0) / len(pids)
    head = 70
    rh = (bottom - top - head) / len(rows)
    s.rect(cx0, top, cw * 2, bottom - top, RAISED, 8)
    s.rect(cx0, top, cw * 2, 5, WAVE, 2)
    for i, pid in enumerate(pids):
        x = cx0 + cw * (i + 0.5)
        p = snap["products"][pid]
        if pid.startswith("torqwave"):
            s.text(x, top + 34, "Torqwave", 21, WHITE, "middle", "bold")
            s.text(x, top + 58, p["version"], 19, WAVE if pid == "torqwave" else DIM, "middle", "bold")
        else:
            lines = split_name(p["name"])
            for j, ln in enumerate(lines):
                s.text(x, top + (34 if len(lines) > 1 else 46) + j * 24, ln, 20, TEXT, "middle", "bold")
    for k, r in enumerate(rows):
        y = top + head + k * rh
        if k % 2 == 0:
            s.rect(lx - 10, y, cx0 - lx + 10, rh, SURFACE)
            s.rect(cx0 + cw * 2, y, W - 50 - cx0 - cw * 2 + 10, rh, SURFACE)
        s.text(lx, y + rh / 2 + 7, r["title"], 21, TEXT)
        for i, pid in enumerate(pids):
            s.mark(cx0 + cw * (i + 0.5), y + rh / 2, snap["products"][pid]["cells"][r["id"]]["value"])
    s.legend(50, H - 28)
    return s.save(m["file"] + ".svg")


def graphic_whole(defs, snap):
    """Every row of the matrix against every product, in one tall picture."""
    pids = [p["id"] for p in defs["products"]]
    groups = defs["groups"]
    lx, cx0, top, head, rh, gh = 50, 420, 150, 262, 30, 44
    h = top + head + sum(gh + rh * len(g["rows"]) for g in groups) + 90
    s = Svg(W, h)
    s.frame(f'Torqwave {snap["release"]} against the alternatives: the whole matrix',
            f'{len(all_rows(defs))} rows in {len(groups)} areas, {len(pids) - 2} products, each from its own documentation', snap)
    cw = (W - 50 - cx0) / len(pids)
    bottom = h - 70
    s.rect(cx0, top, cw * 2, bottom - top, RAISED, 8)
    s.rect(cx0, top, cw * 2, 5, WAVE, 2)
    first_daw = next(i for i, p in enumerate(defs["products"]) if p["group"] == "daw")
    for i, pid in enumerate(pids):
        x = cx0 + cw * (i + 0.5)
        own = pid.startswith("torqwave")
        s.text(x - 2, top + head - 14, label(defs, snap, pid), 19, (WAVE if pid == "torqwave" else WHITE) if own else TEXT, "start", "bold", -65)
    y = top + head
    for g in groups:
        s.rect(lx - 10, y + 8, W - 2 * lx + 20, gh - 12, LINE, 4)
        s.text(lx, y + gh - 13, g["title"], 20, WHITE, weight="bold")
        y += gh
        for k, r in enumerate(g["rows"]):
            if k % 2 == 0:
                s.rect(lx - 10, y, cx0 - lx + 10, rh, SURFACE)
                s.rect(cx0 + cw * 2, y, W - 50 - cx0 - cw * 2 + 10, rh, SURFACE)
            s.text(lx, y + rh / 2 + 6, r["title"], 18, TEXT)
            for i, pid in enumerate(pids):
                s.mark(cx0 + cw * (i + 0.5), y + rh / 2, snap["products"][pid]["cells"][r["id"]]["value"], 9)
            y += rh
    for i in (2, first_daw):
        s.line(cx0 + cw * i, top + head - 6, cx0 + cw * i, bottom, LINE2, 1)
    s.text(cx0 + cw * (2 + (first_daw - 2) / 2), top + 22, "The editors", 17, DIM, "middle")
    s.text(cx0 + cw * (first_daw + (len(pids) - first_daw) / 2), top + 22, "The DAWs", 17, DIM, "middle")
    s.legend(50, h - 28)
    return s.save(f"{len(defs['matrices']) + 3}-the-whole-matrix.svg"), h


def trend_points(defs, snaps):
    rows = all_rows(defs)
    pts = []
    for i, snap in enumerate(snaps):
        if i == 0 and "torqwave_prev" in snap["products"]:
            pts.append((snap["products"]["torqwave_prev"]["version"], coverage(snap["products"]["torqwave_prev"]["cells"], rows)))
        pts.append((snap["products"]["torqwave"]["version"], coverage(snap["products"]["torqwave"]["cells"], rows)))
    return pts, len(rows)


def graphic_standing(defs, snaps):
    snap = snaps[-1]
    s = Svg()
    s.frame("Where Torqwave stands, area by area", "Rows covered in each area, of the rows this comparison tracks. A count of features, not of depth or maturity.", snap)
    pids = [p["id"] for p in defs["products"]]
    lx, gx, top, bottom = 50, 285, 150, H - 70
    cw, head = 91, 62
    rh = (bottom - top - head) / len(pids)
    fs = 20 if rh >= 44 else 18
    for j, g in enumerate(defs["groups"]):
        x = gx + j * cw
        s.text(x + cw / 2, top + 26, SHORT.get(g["id"], g["title"]), 15, TEXT, "middle", "bold")
        s.text(x + cw / 2, top + 46, f'{len(g["rows"])} rows', 13, FAINT, "middle")
    for i, pid in enumerate(pids):
        y = top + head + i * rh
        mine = pid.startswith("torqwave")
        if mine:
            s.rect(lx - 10, y, gx + len(defs["groups"]) * cw - lx + 14, rh, RAISED)
        s.text(lx, y + rh / 2 + 6, label(defs, snap, pid), fs, WHITE if mine else TEXT, weight="bold" if mine else "normal")
        for j, g in enumerate(defs["groups"]):
            c = coverage(snap["products"][pid]["cells"], g["rows"])
            n = len(g["rows"])
            pad = 7 if rh >= 44 else 4
            x, bw, bh = gx + j * cw + 6, cw - 12, rh - 2 * pad
            by = y + pad
            s.rect(x, by, bw, bh, BG if mine else SURFACE, 4, LINE)
            wy, wp, wu = bw * c["yes"] / n, bw * c["partial"] / n, bw * c["not confirmed"] / n
            if wy:
                s.rect(x, by, wy, bh, "#2f7f5b", 4)
            if wp:
                s.rect(x + wy, by, wp, bh, "#8a7a2c", 0)
            if wu:
                s.rect(x + bw - wu, by, wu, bh, LINE2, 0)
            s.text(x + bw / 2, by + bh / 2 + 6, f'{c["yes"]}/{n}', fs - 1, WHITE, "middle", "bold")
    # the trend across releases
    pts, total = trend_points(defs, snaps)
    px = gx + len(defs["groups"]) * cw + 30
    pw = W - 50 - px
    s.rect(px, top, pw, bottom - top, SURFACE, 10, LINE)
    s.text(px + 18, top + 34, "Torqwave by release", 19, WHITE, weight="bold")
    s.text(px + 18, top + 58, f"rows covered, of {total}", 15, DIM)
    slots = max(4, len(pts) + 1)
    shown = pts[-slots:]
    bx0, by0, bh = px + 18, top + 90, bottom - top - 150
    bw = (pw - 36) / slots
    for k in range(slots):
        x = bx0 + k * bw + 5
        if k < len(shown):
            ver, c = shown[k]
            hy, hp = bh * c["yes"] / total, bh * c["partial"] / total
            s.rect(x, by0, bw - 10, bh, BG, 4, LINE)
            s.rect(x, by0 + bh - hy, bw - 10, hy, "#2f7f5b", 4)
            s.rect(x, by0 + bh - hy - hp, bw - 10, hp, "#8a7a2c", 0)
            s.text(x + (bw - 10) / 2, by0 + bh - hy - hp - 8, c["yes"], 17, WHITE, "middle", "bold")
            s.text(x + (bw - 10) / 2, by0 + bh + 24, ver, 13, TEXT, "middle")
        else:
            s.rect(x, by0, bw - 10, bh, "none", 4, LINE, "4 5")
    s.text(px + 18, bottom - 14, "empty slots: the releases to come", 13, FAINT)
    # legend
    x = 50
    for col, word in (("#2f7f5b", "yes (the number)"), ("#8a7a2c", "partial"), (LINE2, "not confirmed"), (SURFACE, "no")):
        s.rect(x, H - 42, 18, 18, col, 3, LINE)
        s.text(x + 26, H - 28, word, 17, DIM)
        x += 44 + len(word) * 9.5
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
    pics = [("1-release-by-the-numbers", "What the release adds over the one before, by the numbers")]
    pics += [(m["file"], m["title"].replace("{release}", snap["release"])) for m in defs["matrices"]]
    pics += [(f"{len(defs['matrices']) + 2}-where-torqwave-stands", "Rows covered in each area by each product, and Torqwave's trend by release")]
    for f, alt in pics:
        o.append(f"![{alt}](graphics/{f}.png)\n")
    o.append("## Coverage by area\n")
    o.append("Rows that are yes, with partial rows after a plus sign and rows not confirmed after a question mark. Counts, not a score.\n")
    tq = ["torqwave", "torqwave_prev"]
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
    pts, total = trend_points(defs, snaps)
    o.append("\n## Torqwave by release\n")
    o.append("| Release | Yes | Partial | No | Rows |")
    o.append("|---|---|---|---|---|")
    for ver, c in pts:
        o.append(f'| {ver} | {c["yes"]} | {c["partial"]} | {c["no"]} | {total} |')
    # behind and ahead, computed
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
    svgs = [graphic_numbers(defs, snap)] + [graphic_matrix(defs, snap, m) for m in defs["matrices"]] + [graphic_standing(defs, snaps)]
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
