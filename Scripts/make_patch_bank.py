#!/usr/bin/env python3
"""Makes Torqwave Synth's second patch bank (Source/WaveSynthPatchBank2.cpp).

The bank is not drawn at random and kept: each kind of sound in patch_bank_kinds.py is drawn several
times for every patch wanted, every candidate is played through the synth and measured
(TorqwaveTests --patch-measure), and a patch is kept only if it passes what
Tests/WaveSynthBankTests.cpp holds the bank to, each with a margin:

  - a held chord peaks near -9 dB once its level is set (the test allows -24 to -1);
  - a note in the category's register is not silent;
  - nothing clicks when the note is struck again, released or its voice is taken;
  - a patch that rises for seconds stays under full scale with its chord held ten seconds;
  - each of its four macros changes the sound;
  - it is not a near-copy of a patch already in its category, the first bank's and the hand-made
    ones included (the test's distance and threshold).

Within a category the candidates furthest from what is already kept are taken first, kind by kind,
so the bank spreads out instead of clustering.

The steps, with <work> a scratch folder and <tool> a built TorqwaveTests:

  make_patch_bank.py draw <work>                      writes <work>/candidates.txt
  <tool> --patch-measure <work>/candidates.meas --patches <work>/candidates.txt
  make_patch_bank.py level <work>                     sets levels; writes <work>/levelled.txt
  <tool> --patch-measure <work>/levelled.meas --patches <work>/levelled.txt
  make_patch_bank.py level <work>                     and again, until it says none are left to level
  <tool> --patch-measure <work>/existing.meas         the synth's own patches (the second bank's are left out)
  make_patch_bank.py select <work>                    writes Source/WaveSynthPatchBank2.cpp

Then build, write Scripts/patch_settings.txt and Scripts/voice_names.txt again (TorqwaveTests
--patch-settings and --voice-names) and run TorqwaveTests --instruments.

The result is a C++ file of plain settings: the same on every build and machine. This script only
needs running again to change the bank, and a change to a kind's recipe changes which patches come out.
"""

import json
import math
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from patch_bank_builder import Draw, Patch          # noqa: E402
import patch_bank_kinds                              # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
SOURCE = os.path.join(os.path.dirname(HERE), "Source")

CANDIDATES_PER_PATCH = 4.0     # drawn for each patch wanted
TARGET_DB = -9.0               # a held chord's peak, as the first bank's
LEVEL_WITHIN = 1.0             # dB either side of the target
SILENT_DB = -27.0              # the test's -30 with a margin
CLICK_RATIO = 3.4              # the test's 4 with a margin
MACRO_DB = 1.5                 # the test's 1 with a margin
LONG_DB = -3.0                 # a slow patch's chord held ten seconds: the test's -1 with a margin
NEAR_COPY = 3.2                # the test's 3.0 with a margin

# The browser's categories with the new ones in place (Source/WaveSynthPatches.cpp has the same list).
CATEGORIES = ["Init", "Bass", "Sub", "Reese", "Acid", "Lead", "Stab", "Chord", "Pluck", "Arp", "Keys", "Organ", "Guitar", "Brass", "Strings",
              "Pad", "Drone", "Bell", "Sequence", "Riser", "Impact", "Cinematic", "Perc", "Drum", "FX", "Glitch", "Game", "Chip", "Texture", "Vox"]


# ---------------------------------------------------------------------------------------------
# Names: an adjective and the noun of the kind of sound. The adjective is drawn from the pools that
# suit how the patch is set (dark, bright, harsh...) and from a general pool.

ADJECTIVES = {
    "dark": """Sunken Buried Murky Dim Blackened Unlit Shadowed Smothered Muffled Drowned Tarred Sooty Charred Ashen Dusky
               Midnight Lightless Entombed Cavernous Abyssal Sunless Starless Moonless Bottomless Nocturnal Umbral Coal""",
    "bright": """Glassy Polished Silver Brittle Crystal Glinting Halogen Neon Fluorescent White Pale Icy Frosted Mirror Platinum
                 Needle Thin Burnished Gleaming Stainless Quicksilver Magnesium Phosphor Lucid Glacial Diamond Bleached""",
    "harsh": """Jagged Serrated Abrasive Caustic Rabid Vicious Savage Hostile Violent Scalding Seared Blistered Raw Gnarled Ragged
                Shredded Mangled Crushed Overloaded Clipped Fried Rusted Splintered Flayed Bitten Acrid Rancid Livid Furious""",
    "soft": """Soft Gentle Velvet Felt Padded Mellow Tender Quiet Hushed Faint Drowsy Sleepy Languid Calm Still Mild Warm Rounded
               Smooth Woollen Downy Dormant Placid Slow Patient Weary""",
    "wet": """Distant Echoing Vaulted Far Remote Drifting Floating Flooded Submerged Ghost Phantom Haunted Spectral Fogbound Misted
              Vast Endless Boundless Receding Lingering Trailing Lost Adrift Sprawling""",
    "dry": """Dry Tight Close Blunt Stubby Compact Dense Packed Solid Flat Damped Boxed Sealed Short Curt Terse Squat Stocky Bare
              Plain Stark""",
    "metal": """Steel Iron Brass Copper Zinc Tin Cobalt Titanium Tungsten Alloy Galvanised Forged Cast Welded Riveted Bolted Plated
                Anodised Tempered Annealed Wrought Pewter Bronze Nickel Chromium Manganese""",
    "digital": """Binary Digital Encoded Encrypted Bitwise Quantised Sampled Corrupt Glitched Logic Synthetic Virtual Coded Serial
                  Hashed Scrambled Buffered Indexed Parsed Compiled Latched Clocked Gated""",
    "motion": """Restless Twitching Shifting Pulsing Turning Circling Marching Ticking Rolling Stalking Creeping Crawling Swaying
                 Lurching Racing Relentless Nervous Wandering Spinning Pacing Prowling Flickering Stuttering Looping""",
    "any": """Concrete Asphalt Bunker Factory Depot Sector Grid Zone District Outpost Border Curfew Ration Barrack Checkpoint
              Perimeter Quarantine Sterile Clinical Surgical Toxic Chemical Nuclear Atomic Solar Lunar Arctic Polar Tundra Desert
              Wasteland Derelict Condemned Forgotten Nameless Faceless Hooded Masked Veiled Silent Sullen Grim Bleak Austere
              Spartan Severe Stern Cold Frozen Numb Hard Heavy Grey Black Red Crimson Scarlet Amber Ochre Umber Sepia Violet
              Indigo Cyan Jade Olive Khaki Ivory Bone Pallid Wan Gaunt Lean Hungry Feral Stray Rogue Outlaw Exiled Banished
              Fugitive Nomad Vagrant Orphan Lone Terminal Fatal Mortal Grave Solemn Sacred Profane Unholy Cursed Hexed Doomed
              Fallen Cracked Split Torn Severed Shattered Ruined Wrecked Gutted Stripped Exposed Locked Chained Bound Caged
              Trapped Cornered Hunted Watched Tracked Tagged Marked Branded Scarred Bruised Bloodless Airless Dreamless
              Sleepless Ruthless Merciless Heartless Soulless Godless Lawless Reckless Fearless Tireless Timeless Ageless
              Weightless Shapeless Formless Voiceless Wordless Hollowed Salted Scoured Flooded Parched Sodden Brackish Stagnant
              Septic Fevered Infected Feverish Clammy Waxen Greased Oiled Pitted Dented Buckled Warped Bent Twisted Knotted
              Frayed Threadbare Mothballed Shuttered Boarded Fenced Walled Gated Barred Sentinel Vigilant Wary Guarded Covert
              Hidden Secret Crooked Skewed Tilted Listing Leaning Slanted Northern Eastern
              Southern Western Inland Coastal Harbour Dockside Railside Roadside Trackside Kerbside Basement Attic Rooftop Cellar
              Backroom Sublevel Lowlight Halflit Afterhours Overnight Daybreak Dawnless Nightshift Graveyard Dayshift Overtime""",
}

# Words and whole names that are someone's band, record, game or product, or read as one.
BLOCKED_WORDS = {"Massive", "Serum", "Virus", "Prophet", "Jupiter", "Juno", "Nord", "Diva", "Zebra", "Battery", "Kontakt", "Absynth",
                 "Reaktor", "Maschine", "Omnisphere", "Sylenth", "Monark", "Pigments", "Vital", "Hive", "Spire", "Dune", "Falcon",
                 "Nexus", "Predator", "Avenger", "Razor", "Thor", "Europa", "Alchemy", "Operator", "Sculpture", "Icarus", "Vanguard",
                 "Albino", "Rapture", "Harmor", "Sytrus", "Moog", "Korg", "Roland", "Oberheim", "Hoover", "Tesla", "Hammond", "Leslie",
                 "Rhodes", "Wurlitzer", "Clavinet", "Mellotron", "Solina", "Fairlight", "Atari", "Nintendo", "Commodore", "Amiga",
                 "Tetris", "Covenant", "Behemoth", "Oracle", "Telex", "Ebb", "Halo", "Orbital", "Phaser", "Zapper", "Sprite", "Pixel",
                 "Android", "Quake", "Ministry", "Prodigy", "Kraftwerk", "Rammstein", "Laibach", "Violator", "Headhunter", "Tyranny",
                 "Mindphaser", "Worlock", "Testure", "Sabbath", "Maiden", "Zelda", "Mario", "Sonic", "Metroid", "Doom", "Diablo"}
BLOCKED_NAMES = {"Red Alert", "Final Countdown", "Last Rite", "Crimson Tide", "Iron Lung", "Wild Hunt", "Black Mass", "Cold Fusion",
                 "Heavy Metal", "Hard Drive", "Black Box", "Silent Watch", "Iron Horn", "Lone Star", "Red Star", "Black Star", "Solar Star",
                 "Atomic Clock", "Iron Bell", "Black Gate", "Iron Gate", "Dead Ring", "Last Call", "Final Call", "Black Key", "Red Ring",
                 "Solid Bass", "Black Hat", "White Hat", "Red Hat", "Grey Hat", "Hard Hat", "Tin Hat", "Top Hat", "Cold Snap",
                 "Sacred Heart", "Black Kick", "Mortal Blow", "Fatal Error", "Fatal Blow", "Hard Kick", "Soft Kick"}


def words(text):
    return text.split()


def existing_patches():
    """(name, category) for the hand-made patches and the first bank, read from the source."""
    found = []

    for file in ("WaveSynthPatches.cpp", "WaveSynthPatchBank.cpp"):
        with open(os.path.join(SOURCE, file), encoding="utf-8") as f:
            found += re.findall(r'add \("([^"]+)", "([^"]+)"\)', f.read())

    return found


def tags_of(p):
    """What the patch is like, for choosing its adjective."""
    v = p.values
    tags = set()
    cutoff, kind = v.get("filter1Cutoff", 8000.0), int(v.get("filter1Type", 1))
    low_pass = kind in (1, 2, 5)

    if (low_pass and cutoff < 700) or p.category in ("Sub", "Drone"):
        tags.add("dark")
    if (low_pass and cutoff > 4500) or kind in (3, 7) or p.category in ("Bell", "Game", "Chip"):
        tags.add("bright")
    if v.get("fxDriveOn", 0) and (v.get("fxDriveAmount", 0) >= 40 or int(v.get("fxDriveType", 0)) in (1, 3, 4)):
        tags.add("harsh")
    if v.get("env1Attack", 2) >= 150 and not v.get("fxDriveOn", 0):
        tags.add("soft")
    if (v.get("fxReverbOn", 0) and v.get("fxReverbMix", 0) >= 35) or (v.get("fxDelayOn", 0) and v.get("fxDelayMix", 0) >= 28):
        tags.add("wet")
    if not (v.get("fxReverbOn", 0) and v.get("fxReverbMix", 0) > 0) and not (v.get("fxDelayOn", 0) and v.get("fxDelayMix", 0) > 0):
        tags.add("dry")
    if any(v.get(f"osc{o}On", 1 if o == 1 else 0) and int(v.get(f"osc{o}Table", 0)) == 4 for o in (1, 2, 3)) or 9 in (kind, int(v.get("filter2Type", 0))):
        tags.add("metal")
    if int(v.get("fxDriveType", 0)) == 5 and v.get("fxDriveOn", 0) or int(v.get("noiseType", 0)) == 3 or any(int(v.get(f"osc{o}Warp", 0)) == 6 for o in (1, 2, 3)):
        tags.add("digital")
    if any(int(v.get(f"lfo{l}Shape", 0)) == 7 for l in (1, 2)) or p.category in ("Sequence", "Arp", "Glitch"):
        tags.add("motion")

    return tags


def name_patches(patches, taken):
    """Names every patch: unique among themselves and against `taken`, by draws seeded from the kind and its place."""
    taken = {t.lower() for t in taken}
    pools = {k: words(t) for k, t in ADJECTIVES.items()}
    kinds = {k["name"]: k for k in patch_bank_kinds.KINDS}

    for p in patches:
        r = Draw("name/" + p.seed)
        nouns = kinds[p.kind]["nouns"]
        tagged = sorted(tags_of(p))

        for attempt in range(400):
            pool = pools[r.pick(tagged)] if tagged and r.chance(0.6) else pools["any"]
            name = r.pick(pool) + " " + r.pick(nouns)

            if name.lower() in taken or name in BLOCKED_NAMES or any(w in BLOCKED_WORDS for w in name.split()) or len(set(name.split())) < 2:
                continue

            taken.add(name.lower())
            p.name = name
            break
        else:
            raise SystemExit("no name left for a " + p.kind)


# ---------------------------------------------------------------------------------------------
# Candidates and their measurements.

def draw_candidates():
    """Every kind drawn CANDIDATES_PER_PATCH times for each patch it is to give. A candidate's seed
    (kind/number) is its name until it is chosen."""
    out = []

    for k in patch_bank_kinds.KINDS:
        for i in range(int(math.ceil(k["count"] * CANDIDATES_PER_PATCH))):
            seed = f"{k['name']}/{i}"
            p = Patch(seed, k["category"])
            p.kind, p.seed, p.target = k["name"], seed, TARGET_DB
            k["make"](p, Draw(seed))
            assert all(p.macros) and "ampLevel" in p.values, seed
            out.append(p)

    return out


def read_measures(path):
    """name -> its measurements, from TorqwaveTests --patch-measure."""
    out = {}

    with open(path, encoding="utf-8") as f:
        for line in f:
            parts = line.rstrip("\n").split("|")

            if len(parts) < 10:
                continue

            out[parts[0]] = {"category": parts[1], "chord": float(parts[2]), "note": float(parts[4]), "click": float(parts[5]),
                             "clickStep": float(parts[6]), "clickAt": float(parts[7]), "macros": [float(x) for x in parts[8].split()],
                             "features": [float(x) for x in parts[9].split()], "long": float(parts[10]) if len(parts) > 10 else -100.0}

    return out


# The numbers a sound is told by (Tests/WaveSynthBankTests.cpp): 15 levels over time, 96 band levels,
# 10 rates of movement and a width, each with the floor it is cut off at. The levels over time count three times.
FLOORS = [-60.0] * 15 + [-50.0] * 96 + [-40.0] * 10 + [-40.0]
WEIGHTS = [3.0] * 15 + [1.0] * 107
LEAST_WEIGHT = 20.0


def distance(a, b):
    """The same as the test's: the weighted mean of the absolute differences in dB, over the numbers
    that either sound has above its floor (silence in both is not a likeness)."""
    total = weight = 0.0

    for x, y, floor, w in zip(a, b, FLOORS, WEIGHTS):
        if x > floor + 0.005 or y > floor + 0.005:
            total += abs(x - y) * w
            weight += w

    return total / max(weight, LEAST_WEIGHT)


def load_levels(work):
    path = os.path.join(work, "levels.json")
    return json.load(open(path)) if os.path.exists(path) else {}


def write_patches(path, patches):
    with open(path, "w", encoding="utf-8") as f:
        for p in patches:
            f.write(p.line() + "\n")


# ---------------------------------------------------------------------------------------------
def step_draw(work):
    os.makedirs(work, exist_ok=True)
    patches = draw_candidates()
    write_patches(os.path.join(work, "candidates.txt"), patches)
    print(f"{len(patches)} candidates from {len(patch_bank_kinds.KINDS)} kinds")


def step_level(work):
    """Sets each candidate's amp level from the chord's measured peak. Drive and level modulation make
    the peak follow the level less than exactly, so it is measured again until it is within a decibel."""
    patches = {p.seed: p for p in draw_candidates()}
    levels = load_levels(work)       # seed -> {"level": dB, "state": "open" | "done" | "failed", "tries": n}
    latest = os.path.join(work, "levelled.meas") if os.path.exists(os.path.join(work, "levelled.meas")) else os.path.join(work, "candidates.meas")
    measured = read_measures(latest)
    todo = []

    for seed, p in patches.items():
        state = levels.setdefault(seed, {"level": p.values["ampLevel"], "state": "open", "tries": 0})

        if state["state"] != "open" or seed not in measured:
            continue

        peak = measured[seed]["chord"]
        error = p.target - peak

        if abs(error) <= LEVEL_WITHIN:
            state["state"] = "done"
            continue

        new = max(-40.0, min(6.0, round(state["level"] + error)))

        if state["tries"] >= 5 or new == state["level"] or peak < -90.0:
            state["state"] = "failed"
            continue

        state["level"], state["tries"] = new, state["tries"] + 1
        p.set_level(new)
        todo.append(p)

    json.dump(levels, open(os.path.join(work, "levels.json"), "w"))
    write_patches(os.path.join(work, "levelled.txt"), todo)

    # What has settled is kept beside what is still open, so the last measurements of each stay.
    settled = os.path.join(work, "settled.meas")
    keep = read_measures(settled) if os.path.exists(settled) else {}
    lines = {}

    for path in (settled, latest):
        if os.path.exists(path):
            with open(path, encoding="utf-8") as f:
                for line in f:
                    lines[line.split("|", 1)[0]] = line

    with open(settled, "w", encoding="utf-8") as f:
        for seed, line in lines.items():
            if levels.get(seed, {}).get("state") == "done":
                f.write(line)

    states = [s["state"] for s in levels.values()]
    print(f"{states.count('done')} levelled, {states.count('failed')} failed, {len(todo)} to measure again"
          + ("" if todo else ": none are left to level"))


def step_select(work):
    kinds = patch_bank_kinds.KINDS
    patches = {p.seed: p for p in draw_candidates()}
    levels = load_levels(work)
    measured = read_measures(os.path.join(work, "settled.meas"))
    earlier = {name for name, _ in existing_patches()}
    existing = {name: m for name, m in read_measures(os.path.join(work, "existing.meas")).items() if name in earlier}
    kept = {c: [m["features"] for m in existing.values() if m["category"] == c] for c in CATEGORIES}
    reasons = {}
    passing = {}

    for seed, p in patches.items():
        m, state = measured.get(seed), levels.get(seed, {})

        if m is None or state.get("state") != "done":
            reason = "level"
        elif m["note"] < SILENT_DB:
            reason = "silent"
        elif m["click"] > CLICK_RATIO:
            reason = "click"
        elif min(m["macros"]) < MACRO_DB:
            reason = "macro"
        elif m["long"] > LONG_DB:
            reason = "long"
        else:
            p.set_level(state["level"])
            passing.setdefault(p.kind, []).append(p)
            continue

        reasons.setdefault(p.kind, {}).setdefault(reason, 0)
        reasons[p.kind][reason] += 1

    # Kind by kind in turn, each time the candidate furthest from everything kept in its category.
    # Each candidate's distance to the nearest kept patch is worked out once and lowered as patches are kept.
    nearest = {}

    for pool in passing.values():
        for p in pool:
            f = measured[p.seed]["features"]
            nearest[p.seed] = min((distance(f, o) for o in kept[p.category]), default=99.0)

    chosen = {k["name"]: [] for k in kinds}
    live = True

    while live:
        live = False

        for k in kinds:
            pool = passing.get(k["name"], [])

            if len(chosen[k["name"]]) >= k["count"] or not pool:
                continue

            best = max(pool, key=lambda p: nearest[p.seed])

            if nearest[best.seed] < NEAR_COPY:
                reasons.setdefault(k["name"], {})["near-copy"] = len(pool)
                passing[k["name"]] = []
                continue

            pool.remove(best)
            chosen[k["name"]].append(best)
            added = measured[best.seed]["features"]
            live = True

            for other in kinds:
                if other["category"] == k["category"]:
                    for p in passing.get(other["name"], []):
                        nearest[p.seed] = min(nearest[p.seed], distance(measured[p.seed]["features"], added))

    selected = [p for k in kinds for p in chosen[k["name"]]]
    name_patches(selected, [n for n, _ in existing_patches()])
    by_category = {}

    for p in selected:
        by_category.setdefault(p.category, []).append(p)

    with open(os.path.join(SOURCE, "WaveSynthPatchBank2.cpp"), "w", encoding="utf-8") as f:
        f.write(HEADER.replace("COUNT", f"{len(selected):,}").replace("KINDS", str(len(kinds))))

        for c in CATEGORIES:
            if c in by_category:
                f.write(f"\n        // {c}, by name.\n")

                for p in sorted(by_category[c], key=lambda p: p.name):
                    f.write(p.cpp() + "\n")

        f.write(FOOTER)

    with open(os.path.join(work, "selected.txt"), "w", encoding="utf-8") as f:
        for p in selected:
            f.write(f"{p.name}|{p.category}|{p.kind}|{p.seed}\n")

    print(f"{len(selected)} patches")

    for c in CATEGORIES:
        if c in by_category:
            print(f"  {c}: {len(by_category[c])}")

    for k in kinds:
        if len(chosen[k["name"]]) < k["count"]:
            print(f"  short: {k['name']} {len(chosen[k['name']])} of {k['count']} {reasons.get(k['name'], {})}")


HEADER = """#include "WaveSynthPatchBuilder.h"

// Torqwave Synth's second patch bank: COUNT more patches, in the first bank's categories and in new
// ones (Sub, Chord, Arp, Keys, Organ, Brass, Strings, Cinematic, Drum, FX, Glitch, Game and Chip).
// Scripts/make_patch_bank.py wrote this file: it drew each of KINDS kinds of sound
// (Scripts/patch_bank_kinds.py) many times, played every candidate through the synth, and kept the ones
// that sound in their register, do not click, have four macros that are heard and are not a near-copy
// of another patch in their category; then it set each one's level so a held chord peaks near -9 dB.
// Tests/WaveSynthBankTests.cpp holds the bank to the same things.

namespace WaveSynthPatchBuilder
{
    void addBank2 (std::vector<Builder>& b)
    {
        const auto add = [&b] (const char* name, const char* category) -> Builder& { b.emplace_back (name, category); return b.back(); };
"""

FOOTER = """    }

    const juce::StringArray& secondBankNames()
    {
        static const auto names = []
        {
            std::vector<Builder> b;
            addBank2 (b);
            juce::StringArray n;

            for (auto& p : b)
                n.add (p.patch.name);

            return n;
        }();
        return names;
    }
}
"""


if __name__ == "__main__":
    steps = {"draw": step_draw, "level": step_level, "select": step_select}

    if len(sys.argv) != 3 or sys.argv[1] not in steps:
        raise SystemExit(__doc__)

    steps[sys.argv[1]](sys.argv[2])
