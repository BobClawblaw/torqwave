#!/usr/bin/env python3
"""Torqwave Synth's patches by what they are made of.

Reads the JSON lines `TorqwaveTests --patch-values <file>` writes and gives each patch a recipe
fingerprint: the parts that do audible work (each heard oscillator's table and shape, its warp,
its register, unison, FM chains), the filters and their routing, how the amp is shaped, the voice
mode, and what every LFO, envelope and velocity route moves. Two patches' likeness is the weighted
overlap of their fingerprints (1 the same recipe, 0 nothing shared), the loudest oscillator
counting most, as the ear hears a patch by its body first.

    python3 Scripts/patch_sweep.py values.jsonl nearest [name ...]   each patch's nearest factory patches
    python3 Scripts/patch_sweep.py values.jsonl calibrate            the user's verdicts against the measure
    python3 Scripts/patch_sweep.py values.jsonl sweep all.meas [out.json]  near-copies, the bank's size at three
                                                                     thresholds, and its crowded and empty corners

`all.meas` is `TorqwaveTests --patch-measure all.meas`: every factory patch's note at its category's
register, told by the second bank's numbers (Scripts/make_patch_bank.py), whose distance a patch of
that bank had to keep from every other in its category (3.0 dB).
"""

import json
import sys
from collections import Counter

TABLES = ["analog", "pulse", "vowel", "fm", "metal", "sub", "additive", "sync", "growl", "string", "sampled"]
WARPS = ["off", "sync", "bend", "squeeze", "mirror", "fm", "crush", "fold"]
FILTERS = ["off", "ladder24", "ladder12", "ladderHigh", "ladderBand", "svfLow", "svfBand", "svfHigh", "svfNotch", "comb"]
SHAPES = ["sine", "triangle", "sawUp", "sawDown", "square", "random", "drift", "steps"]
SOURCES = ["none", "amp", "env2", "env3", "env4", "lfo1", "lfo2", "velocity", "modWheel", "aftertouch", "key",
           "macro1", "macro2", "macro3", "macro4", "random", "lfo3", "lfo4"]
OSC_FIELDS = ["Position", "WarpAmount", "Pitch", "Fine", "Volume", "Pan", "UnisonDetune", "UnisonSpread", "Route"]
FIRST = ["subLevel", "noiseLevel", "noiseColour", "filter1Cutoff", "filter1Resonance", "filter1Drive",
         "filter2Cutoff", "filter2Resonance", "filter2Drive", "ampLevel", "ampPan", "lfo1Rate", "lfo2Rate",
         "fxDriveAmount", "fxChorusMix", "fxPhaserMix", "fxDelayMix", "fxDelayFeedback", "fxReverbMix"]
DESTINATIONS = ([f"osc{o}{f}" for o in range(1, 4) for f in OSC_FIELDS] + FIRST
                + [f"osc{o}{f}" for o in range(4, 10) for f in OSC_FIELDS] + ["lfo3Rate", "lfo4Rate"]
                + [f"mod{s}Amount" for s in range(1, 25)])

DEFAULTS = {"osc1On": 1, "osc1Position": 66.67, "filter1Type": 1, "filter1Cutoff": 8000, "env1Attack": 2,
            "env1Decay": 300, "env1Sustain": 80, "env1Release": 120}


def value(v, key, default=0.0):
    if key in v:
        return v[key]
    if key in DEFAULTS:
        return DEFAULTS[key]
    if key.endswith("Volume"):
        return 80.0
    if key.endswith("Unison"):
        return 1.0
    return default


def family(dest):
    """What a modulation destination does, coarsely: what the ear would name."""
    if dest.startswith("osc"):
        field = dest[4:] if dest[4].isalpha() else dest[5:]
        return {"Position": "timbre", "WarpAmount": "warp", "Pitch": "pitch", "Fine": "pitch", "Volume": "level",
                "Pan": "pan", "UnisonDetune": "width", "UnisonSpread": "width", "Route": "route"}[field]
    if dest.startswith("filter"):
        return "filter"
    if dest.startswith("fx"):
        return "fx"
    if dest.startswith("lfo"):
        return "lfo-rate"
    if dest.startswith("mod"):
        return "depth"
    if dest in ("ampLevel",):
        return "level"
    if dest in ("ampPan",):
        return "pan"
    return "noise"


def fingerprint(v):
    """A patch's recipe as weighted tokens."""
    tokens = Counter()
    heard = []

    # Oscillators that sound, and the ones others' FM names (silent modulators).
    fm_from = {o: int(value(v, f"osc{o}FmFrom")) for o in range(1, 10)}
    named = {f for o, f in fm_from.items() if f and int(value(v, f"osc{o}Warp")) == 5}
    level_modded = set()

    for s in range(1, 25):
        src, dst = int(value(v, f"mod{s}Source")), int(value(v, f"mod{s}Destination"))
        if src and dst:
            d = DESTINATIONS[dst - 1]
            if d.endswith("Volume") and d.startswith("osc"):
                level_modded.add(int(d[3]))

    for o in range(1, 10):
        on = value(v, f"osc{o}On") > 0.5
        level = value(v, f"osc{o}Volume")
        if not on or (level < 12 and o not in level_modded):
            continue
        heard.append((level, o))

    heard.sort(reverse=True)

    for rank, (level, o) in enumerate(heard):
        weight = 3.0 if rank == 0 else 1.5 if rank == 1 else 1.0
        table = TABLES[int(value(v, f"osc{o}Table"))]
        position = value(v, f"osc{o}Position")
        shape = f"{table}:{int(position // 25)}"
        tokens[f"osc:{shape}"] += weight
        tokens[f"table:{table}"] += weight * 0.5

        warp, amount = WARPS[int(value(v, f"osc{o}Warp"))], value(v, f"osc{o}WarpAmount")
        if warp != "off" and amount >= 10:
            tokens[f"warp:{table}:{warp}"] += weight
            if warp == "fm" and fm_from[o]:
                tokens["fm-from-another"] += weight

        pitch = value(v, f"osc{o}Pitch")
        tokens[f"register:{int(round(pitch / 12))}"] += weight * 0.5

        if value(v, f"osc{o}Unison") >= 3:
            tokens[f"unison:{table}"] += weight

        if value(v, f"osc{o}EnvOn") > 0.5:
            hold, decay, sustain = value(v, f"osc{o}EnvHold"), value(v, f"osc{o}EnvDecay"), value(v, f"osc{o}EnvSustain")
            tokens["layer:strike" if decay + hold <= 40 and sustain < 5 else
                   "layer:arrives" if value(v, f"osc{o}EnvAttack") > 300 else "layer:dies"] += 1.0

    for o in named:
        if not value(v, f"osc{o}On") > 0.5:
            tokens["silent-modulator"] += 1.0

    if value(v, "subLevel") > 10:
        tokens["sub-osc"] += 1.0
    if value(v, "noiseLevel") > 8:
        tokens[f"noise:{int(value(v, 'noiseType'))}"] += 1.0

    for f in (1, 2):
        kind = FILTERS[int(value(v, f"filter{f}Type"))]
        if kind != "off":
            tokens[f"filter{f}:{kind}"] += 1.5 if f == 1 else 1.0
    if value(v, "filterRouting") > 0.5:
        tokens["parallel"] += 1.0

    attack, decay, sustain = value(v, "env1Attack"), value(v, "env1Decay"), value(v, "env1Sustain")
    tokens["amp:swell" if attack > 300 else "amp:pluck" if sustain < 20 else "amp:held"] += 1.5
    tokens[["voice:poly", "voice:mono", "voice:legato"][int(value(v, "voiceMode"))]] += 1.0

    if value(v, "fxDriveOn") > 0.5 and value(v, "fxDriveAmount") >= 10:
        tokens[f"drive:{int(value(v, 'fxDriveType'))}"] += 1.0
    for fx in ("Chorus", "Phaser", "Delay", "Reverb"):
        if value(v, f"fx{fx}On") > 0.5 and value(v, f"fx{fx}Mix") >= 10:
            tokens[f"fx:{fx.lower()}"] += 0.5

    for s in range(1, 25):
        src, dst = int(value(v, f"mod{s}Source")), int(value(v, f"mod{s}Destination"))
        if not src or not dst or abs(value(v, f"mod{s}Amount")) < 5:
            continue
        source = SOURCES[src]
        if source.startswith("macro"):
            continue
        if source.startswith("lfo"):
            l = source[-1]
            shape = SHAPES[int(value(v, f"lfo{l}Shape"))]
            synced = "sync" if value(v, f"lfo{l}Sync") > 0.5 else "free"
            source = f"lfo:{shape}:{synced}"
        tokens[f"mod:{source}>{family(DESTINATIONS[dst - 1])}"] += 1.0

    return tokens


def likeness(a, b):
    keys = set(a) | set(b)
    top = sum(max(a[k], b[k]) for k in keys)
    return sum(min(a[k], b[k]) for k in keys) / top if top else 1.0


def load(path):
    return [json.loads(line) for line in open(path) if line.strip()]


def nearest(patches, names=None, count=3):
    factory = [p for p in patches if p["origin"] != "complex"]
    prints = {p["name"]: fingerprint(p["values"]) for p in patches}
    for p in patches:
        if names and p["name"] not in names:
            continue
        if not names and p["origin"] != "complex":
            continue
        near = sorted(((likeness(prints[p["name"]], prints[f["name"]]), f["name"], f["category"]) for f in factory
                       if f["name"] != p["name"]), reverse=True)[:count]
        print(f"{p['name']:20} {p['category']:10} " + ", ".join(f"{s:.2f} {n} ({c})" for s, n, c in near))


# The user's verdicts on the first 58 as they heard them (2026-11-25).
LIKE_THE_BANK = ["Spark Foreman", "Basalt Floor", "Pressure Vault", "Sprite Acid"]
DECENT = ["Rivet Engine", "Tidal Shear", "Caustic Lattice", "Bulkhead Chord", "Conveyor Hex", "Corroding Halo"]


def calibrate(patches):
    factory = [p for p in patches if p["origin"] != "complex"]
    prints = {p["name"]: fingerprint(p["values"]) for p in patches}
    best = {}
    for p in patches:
        if p["origin"] == "complex":
            best[p["name"]] = max((likeness(prints[p["name"]], prints[f["name"]]), f["name"]) for f in factory)
    print("like the bank:", ", ".join(f"{n} {best[n][0]:.2f} ({best[n][1]})" for n in LIKE_THE_BANK))
    print("decent:       ", ", ".join(f"{n} {best[n][0]:.2f} ({best[n][1]})" for n in DECENT))
    threshold = min(best[n][0] for n in LIKE_THE_BANK)
    others = [n for n in best if n not in LIKE_THE_BANK]
    caught = [n for n in others if best[n][0] >= threshold]
    print(f"a threshold at {threshold:.2f} (rejecting all four) also rejects {len(caught)} of the other {len(others)}:",
          ", ".join(caught))
    print("decent ones it rejects:", [n for n in DECENT if best[n][0] >= threshold])


# ---------------------------------------------------------------------------------------------
# The sweep: count, don't cut.

FLOORS = [-60.0] * 15 + [-50.0] * 96 + [-40.0] * 10 + [-40.0]
WEIGHTS = [3.0] * 15 + [1.0] * 107
LEAST_WEIGHT = 20.0
THRESHOLDS = {"strict": 1.5, "middle": 2.25, "loose": 3.0}   # loose is the second bank's own near-copy
ORIGIN_ORDER = ["hand-made", "bank", "layered", "bank2"]    # kept first: the hand-made, then the oldest


def read_measures(path):
    out = {}
    for line in open(path, encoding="utf-8"):
        parts = line.rstrip("\n").split("|")
        if len(parts) >= 10:
            out[parts[0]] = {"category": parts[1], "features": [float(x) for x in parts[9].split()]}
    return out


def distances(features):
    """Every pair's distance in a category at once, as make_patch_bank.py's distance()."""
    import numpy as np
    x = np.array(features)
    floors, weights = np.array(FLOORS), np.array(WEIGHTS)
    above = x > floors + 0.005
    d = np.zeros((len(x), len(x)))
    for i in range(len(x)):
        counted = above[i] | above
        w = counted * weights
        d[i] = (np.abs(x[i] - x) * w).sum(1) / np.maximum(w.sum(1), LEAST_WEIGHT)
    return d


def movement(features):
    return sum(features[111:121]) / 10.0


def sweep(patches, meas_path, out_path=None):
    import numpy as np
    values = {p["name"]: p for p in patches if p["origin"] != "complex"}
    measured = read_measures(meas_path)
    names = [n for n in measured if n in values]
    by_cat = {}
    for n in names:
        by_cat.setdefault(measured[n]["category"], []).append(n)

    pairs, kept = [], {t: {} for t in THRESHOLDS}
    for cat, members in sorted(by_cat.items()):
        d = distances([measured[n]["features"] for n in members])
        for i in range(len(members)):
            for j in range(i + 1, len(members)):
                if d[i, j] < 4.0:
                    pairs.append((float(d[i, j]), cat, members[i], members[j]))
        # Kept at each threshold: in keeping order, a patch stays if it is that far from all kept so far.
        order = sorted(range(len(members)), key=lambda i: (ORIGIN_ORDER.index(values[members[i]]["origin"]),
                                                          -movement(measured[members[i]]["features"])))
        for t, limit in THRESHOLDS.items():
            keep = []
            for i in order:
                if all(d[i, k] >= limit for k in keep):
                    keep.append(i)
            kept[t][cat] = [members[i] for i in keep]

    total = len(names)
    print(f"{total} factory patches measured ({len(values) - total} not in the measure file)")
    print("by origin:", dict(Counter(values[n]["origin"] for n in names)))
    print()
    print(f"{'category':12}{'now':>6}" + "".join(f"{t + ' ' + str(l):>14}" for t, l in THRESHOLDS.items()))
    for cat in sorted(by_cat, key=lambda c: -len(by_cat[c])):
        print(f"{cat:12}{len(by_cat[cat]):6}" + "".join(f"{len(kept[t][cat]):14}" for t in THRESHOLDS))
    print(f"{'all':12}{total:6}" + "".join(f"{sum(len(v) for v in kept[t].values()):14}" for t in THRESHOLDS))
    print()
    for t in THRESHOLDS:
        gone = [n for n in names if n not in set(sum(kept[t].values(), []))]
        print(f"{t}: {len(gone)} would go, by origin", dict(Counter(values[n]["origin"] for n in gone)))

    prints = {n: fingerprint(values[n]["values"]) for n in names}
    pairs.sort()
    result = {"thresholds": THRESHOLDS, "kept": kept,
              "pairs": [{"d": round(d, 3), "category": c, "a": a, "b": b, "recipe": round(likeness(prints[a], prints[b]), 3),
                         "origins": [values[a]["origin"], values[b]["origin"]]} for d, c, a, b in pairs]}

    # Corners: what the body is made of (the loudest oscillator's table and warp) and how the amp is shaped,
    # over the whole bank.
    corners = Counter()
    for n in names:
        tokens = prints[n]
        body = max((k for k in tokens if k.startswith("osc:")), key=lambda k: tokens[k], default="osc:none")
        table = body.split(":")[1]
        warp = next((k.split(":")[2] for k in tokens if k.startswith(f"warp:{table}:") and tokens[k] >= 3.0), "off")
        corners[(table, warp)] += 1
    result["corners"] = {f"{t}/{w}": c for (t, w), c in corners.items()}
    print()
    print("bodies (the loudest oscillator's table / warp), most crowded:")
    print("  " + ", ".join(f"{t}/{w} {c}" for (t, w), c in corners.most_common(15)))
    empty = [f"{t}/{w}" for t in TABLES for w in WARPS if corners[(t, w)] == 0]
    print(f"empty ({len(empty)} of {len(TABLES) * len(WARPS)}):", ", ".join(empty))

    if out_path:
        json.dump(result, open(out_path, "w"), indent=1)
    return result


if __name__ == "__main__":
    patches = load(sys.argv[1])
    what = sys.argv[2] if len(sys.argv) > 2 else "calibrate"
    if what == "nearest":
        nearest(patches, set(sys.argv[3:]) or None)
    elif what == "sweep":
        sweep(patches, sys.argv[3], sys.argv[4] if len(sys.argv) > 4 else None)
    else:
        calibrate(patches)
