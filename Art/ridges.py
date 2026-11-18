# The Torqwave mark, shared by make-icon.py and make-banner.py: a stack of waveforms, each slice
# hiding those behind it, twisted along its depth. Slice i lies in a plane at depth i, turned
# about a line through the waveforms' middle by an angle that runs from -twist/2 at the front to
# +twist/2 at the back. Drawn back to front, each slice filled with the background below its line.
import math, random

PEAK = 175.0                 # tallest a waveform gets, in world units
AXIS = 60.0                  # the twist's axis, above each slice's base
SPAN = 900.0                 # width of a slice
DEPTH = 1500.0               # front to back
TILT, DIST = math.radians(32), 2300.0


def slice_heights(i, rng, n):
    """One waveform: quiet tails and a busy middle, like a burst of sound."""
    phase = rng.uniform(0, 6.28)
    f1, f2 = rng.uniform(9, 14), rng.uniform(25, 40)
    jitter = [rng.gauss(0, 1) for _ in range(n + 1)]
    hs = []
    for k in range(n + 1):
        u = k / n * 2 - 1
        env = math.exp(-(u / 0.33) ** 2) * (0.6 + 0.4 * math.sin(i * 0.45 + phase))
        osc = abs(math.sin(f1 * u * math.pi + phase)) * 0.65 + abs(math.sin(f2 * u * math.pi + i)) * 0.35
        noise = 0.035 * jitter[k] + 0.03 * (jitter[k - 1] if k else 0)
        hs.append(max(0.0, PEAK * (env * osc + 0.05 * math.exp(-(u / 0.6) ** 2)) + PEAK * noise * (0.3 + env)))
    return [(hs[max(0, k - 1)] + 2 * hs[k] + hs[min(n, k + 1)]) / 4 for k in range(n + 1)]


def view(x, y, z):
    """Tilted so the stack climbs as it goes back, then in perspective."""
    y, z = y * math.cos(TILT) + z * math.sin(TILT), -y * math.sin(TILT) + z * math.cos(TILT)
    k = DIST / (DIST + z)
    return x * k, -y * k


HOT = "#ff5c1f"               # Torqwave's one colour: the front slice and the name


def hot_line(i, f, bright):
    """White slices, the front one hot orange."""
    if i == 0:
        return tuple(int(HOT[k:k + 2], 16) / 255 for k in (1, 3, 5))
    return (0.9, 0.9, 0.92)


def mark(box, lines=64, twist=45, stroke=2.6, peak=1.0, points=260, seed=11, background="#08070d", bright=0.0,
         colour=hot_line):
    """The mark fitted inside box = (x, y, width, height): SVG elements, with a bloom filter that
    the caller defines as id "bloom". bright (0..1) lifts the far lines, for small sizes. colour
    gives a slice's (r, g, b), 0..1, from its index i and its depth f (0 front .. 1 back)."""
    rng = random.Random(seed)
    turn = math.radians(twist)
    polys, strokes, allpts = [], [], []
    for i in reversed(range(lines)):                  # back to front
        f = i / (lines - 1)                           # 0 front .. 1 back
        z = f * DEPTH - DEPTH * 0.35
        th = turn * (f - 0.5)
        hs = [h * peak for h in slice_heights(i, rng, points)]
        n = len(hs) - 1
        line, below = [], []
        for k in range(n + 1):
            u = (k / n - 0.5) * SPAN
            hc = hs[k] - AXIS
            line.append(view(u * math.cos(th) - hc * math.sin(th), u * math.sin(th) + hc * math.cos(th), z))
            below.append(view(u * math.cos(th) + (AXIS + 26) * math.sin(th), u * math.sin(th) - (AXIS + 26) * math.cos(th), z))
        r, g, b = colour(i, f, bright)
        width = stroke * DIST / (DIST + z * math.cos(TILT))
        polys.append((line + list(reversed(below)), line, "#%02x%02x%02x" % (int(r * 255), int(g * 255), int(b * 255)),
                      width, (0.55 + 0.45 * bright) + (0.45 - 0.45 * bright) * (1 - f)))
        allpts += line + below

    xs = [p[0] for p in allpts]
    ys = [p[1] for p in allpts]
    bx, by, bw, bh = box
    sc = min(bw / (max(xs) - min(xs)), bh / (max(ys) - min(ys)))
    ox = bx + bw / 2 - sc * (max(xs) + min(xs)) / 2
    oy = by + bh / 2 - sc * (max(ys) + min(ys)) / 2

    def pts(ps):
        return " ".join(f"{ox + sc * x:.1f},{oy + sc * y:.1f}" for x, y in ps)

    out = []
    for fill, line, colour, width, opacity in polys:
        out.append(f'<polygon points="{pts(fill)}" fill="{background}"/>')
        out.append(f'<polyline points="{pts(line)}" fill="none" stroke="{colour}" stroke-width="{width * sc:.2f}" '
                   f'stroke-linejoin="round" stroke-linecap="round" opacity="{opacity:.2f}"/>')
    return f'<g filter="url(#bloom)">{"".join(out)}</g>'


def bloom(deviation):
    """The glow the mark's lines carry."""
    return (f'<filter id="bloom" x="-5%" y="-5%" width="110%" height="110%" color-interpolation-filters="sRGB">'
            f'<feGaussianBlur stdDeviation="{deviation}" result="b"/>'
            f'<feComponentTransfer in="b" result="bb"><feFuncA type="linear" slope="0.6"/></feComponentTransfer>'
            f'<feMerge><feMergeNode in="bb"/><feMergeNode in="SourceGraphic"/></feMerge></filter>')
