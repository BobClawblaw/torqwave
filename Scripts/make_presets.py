#!/usr/bin/env python3
"""Writes Torqwave's factory presets: Data/Presets/<Role>/<Sub-role>/<Name>.tqseq, and the starter
kits in Data/Presets/Kits.

Every preset is made here from a compact definition (a 16-step grid per drum, a step list for a
line, chords by bar), so a change to the conventions is one edit and a rerun. The rules they keep
(docs/PLAN-0.19.3.md, step 6), which the test suite checks again:

- on the 16th grid; drums on the General MIDI map; bass lines mono except an acid slide, which
  overlaps the next note by 20 ticks;
- velocities carry the groove: accents 110-127, normal 90-105, weak hats 60-80, ghosts 25-50;
- one home key, A minor (Phrygian's B flat and harmonic minor's G sharp where tagged so);
- 1, 2, 4 or 8 bars; a 4-bar preset changes in bar 4;
- original patterns written to a style's conventions, named for the style, never a band or a song;
- a one-line description (the sequence's note) and a style ("Techno/Dark").

Run it from anywhere: python3 Scripts/make_presets.py. --check writes into a temporary folder and
says whether that matches Data/Presets byte for byte.
"""

import base64
import filecmp
import os
import zlib
import random
import shutil
import sys
import tempfile
from xml.sax.saxutils import quoteattr

TPB = 960            # ticks to a beat
S = TPB // 4         # a 16th
BAR = 4 * TPB
SLIDE = 20           # an acid slide's overlap, in ticks
KEY = "A minor"

# Velocities by letter, for drum grids and lines.
VEL = {"X": 120, "x": 98, "o": 74, "w": 64, "g": 40}

# The General MIDI drum notes the presets use.
KICK, RIM, SNARE, CLAP, CH, PH, OH, LT, MT, HT, CRASH, RIDE, COWBELL, SHAKER = 36, 37, 38, 39, 42, 44, 46, 41, 45, 48, 49, 51, 56, 70

NOTE_NAMES = {"c": 0, "c#": 1, "db": 1, "d": 2, "d#": 3, "eb": 3, "e": 4, "f": 5, "f#": 6, "gb": 6,
              "g": 7, "g#": 8, "ab": 8, "a": 9, "a#": 10, "bb": 10, "b": 11}


SPELLING = ["c", "c#", "d", "eb", "e", "f", "f#", "g", "g#", "a", "bb", "b"]


def name_of(p):
    """60 is 'c4'."""
    return f"{SPELLING[p % 12]}{p // 12 - 1}"


def pitch(name):
    """'a1' is 33 (A1, 55 Hz), 'c4' is 60."""
    name = name.lower()
    i = 2 if len(name) > 2 and name[1] in "#b" and not name[1:].lstrip("-").isdigit() else 1
    return NOTE_NAMES[name[:i]] + 12 * (int(name[i:]) + 1)


# ---------------------------------------------------------------------------------------------
# JUCE's own encodings, so the voices carry their instrument's settings.

JUCE_B64 = ".ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+"


def juce_base64(data):
    """As juce::MemoryBlock::toBase64Encoding: the size, a dot, then 6 bits a character, low bits first."""
    value = int.from_bytes(data, "little")
    chars = (len(data) * 8 + 5) // 6
    return str(len(data)) + "." + "".join(JUCE_B64[(value >> (6 * i)) & 63] for i in range(chars))


def binary_state(tag, values):
    """As juce::AudioProcessor::copyXmlToBinary of <tag version="1" ...values/>."""
    attrs = " ".join(f'{k}="{v}"' for k, v in [("version", 1)] + list(values.items()))
    text = f"<{tag} {attrs}/>".encode("utf-8") + b"\0"
    return (0x21324356).to_bytes(4, "little") + (len(text) - 1).to_bytes(4, "little") + text


SYNTH = dict(name="Basic Synth", file="torqwave:basic-synth", uid="53425359", tag="TORQWAVE_BASIC_SYNTH")
DRUMS = dict(name="Basic Drums", file="torqwave:basic-drums", uid="53424452", tag="TORQWAVE_BASIC_DRUMS")

# Basic Synth settings for each kind of part (its parameters by ID; anything left out is its default).
PATCHES = {
    "sub":    dict(wave=0, level=-9.0, attack=2.0, decay=100.0, sustain=100.0, release=60.0),
    "roll":   dict(wave=2, level=-12.0, attack=1.0, decay=160.0, sustain=55.0, release=40.0, cutoff=420.0, resonance=25.0,
                   osc2Wave=3, osc2Octave=-1.0, osc2Level=45.0, filterEnv=45.0, filterDecay=120.0),
    "ebm":    dict(wave=2, level=-12.0, attack=1.0, decay=120.0, sustain=30.0, release=30.0, cutoff=650.0, resonance=20.0,
                   osc2Wave=2, osc2Detune=8.0, osc2Level=70.0, filterEnv=40.0, filterDecay=90.0, velocityToFilter=30.0),
    "acid":   dict(wave=2, level=-12.0, attack=1.0, decay=220.0, sustain=40.0, release=20.0, cutoff=320.0, resonance=75.0,
                   filterEnv=70.0, filterDecay=180.0, velocityToFilter=60.0, glide=60.0),
    "dark":   dict(wave=3, level=-14.0, attack=1.0, decay=200.0, sustain=50.0, release=60.0, cutoff=500.0, resonance=30.0,
                   osc2Wave=2, osc2Octave=-1.0, osc2Level=60.0, filterEnv=30.0, filterDecay=150.0),
    "stab":   dict(wave=2, level=-17.0, attack=2.0, decay=220.0, sustain=0.0, release=140.0, cutoff=1800.0, resonance=30.0,
                   osc2Wave=2, osc2Detune=-12.0, osc2Level=80.0, osc3Wave=3, osc3Octave=1.0, osc3Level=30.0,
                   filterEnv=50.0, filterDecay=140.0),
    "chord":  dict(wave=2, level=-18.0, attack=8.0, decay=600.0, sustain=60.0, release=400.0, cutoff=2400.0, resonance=10.0,
                   osc2Wave=2, osc2Detune=7.0, osc2Level=70.0),
    "pad":    dict(wave=2, level=-18.0, attack=800.0, decay=1000.0, sustain=80.0, release=1500.0, cutoff=1800.0, resonance=10.0,
                   osc2Wave=2, osc2Detune=9.0, osc2Level=80.0, osc3Wave=1, osc3Octave=-1.0, osc3Level=50.0),
    "drone":  dict(wave=2, level=-16.0, attack=2000.0, decay=1000.0, sustain=100.0, release=3000.0, cutoff=700.0, resonance=35.0,
                   osc2Wave=2, osc2Octave=-1.0, osc2Detune=6.0, osc2Level=70.0),
    "arp":    dict(wave=2, level=-15.0, attack=1.0, decay=160.0, sustain=20.0, release=120.0, cutoff=2200.0, resonance=25.0,
                   osc2Wave=3, osc2Detune=5.0, osc2Level=40.0, filterEnv=35.0, filterDecay=110.0),
    "pluck":  dict(wave=1, level=-12.0, attack=1.0, decay=180.0, sustain=0.0, release=150.0, cutoff=2500.0,
                   osc2Wave=2, osc2Octave=1.0, osc2Level=40.0, filterEnv=40.0, filterDecay=120.0),
    "lead":   dict(wave=3, level=-15.0, attack=5.0, decay=300.0, sustain=70.0, release=200.0, cutoff=4000.0, resonance=20.0,
                   osc2Wave=2, osc2Detune=5.0, osc2Level=50.0),
    "saw":    dict(wave=2, level=-16.0, attack=2.0, decay=200.0, sustain=60.0, release=120.0, cutoff=5000.0, resonance=15.0,
                   osc2Wave=2, osc2Detune=12.0, osc2Level=90.0, osc3Wave=2, osc3Detune=-12.0, osc3Level=90.0),
    "bell":   dict(wave=0, level=-12.0, attack=1.0, decay=900.0, sustain=0.0, release=900.0,
                   osc2Wave=0, osc2Octave=2.0, osc2Level=40.0, osc3Wave=1, osc3Octave=1.0, osc3Detune=7.0, osc3Level=25.0),
    "sweep":  dict(wave=2, level=-21.0, attack=50.0, decay=500.0, sustain=100.0, release=800.0, cutoff=900.0, resonance=60.0,
                   osc2Wave=3, osc2Detune=7.0, osc2Level=60.0),
}

# Basic Drums settings: the machine as it starts, and a long, low kick for rumbles.
KITS_SOUND = {
    "standard": dict(),
    "rumble":   dict(kickTune=-3.0, kickDecay=260.0, hatDecay=80.0),
    "machine":  dict(snareDecay=160.0, hatDecay=70.0),
    "hard":     dict(kickTune=2.0, kickDecay=80.0, hatDecay=60.0),
}


def voice_xml(name, instrument, settings, gain_db):
    state = binary_state(instrument["tag"], settings) if settings else None
    plugin = (f'<PLUGIN name="{instrument["name"]}" descriptiveName="{instrument["name"]}" format="Torqwave" category="Instrument" '
              f'manufacturer="Torqwave" version="1" file="{instrument["file"]}" uniqueId="{instrument["uid"]}" isInstrument="1" '
              f'numInputs="0" numOutputs="2" isShell="0" hasARAExtension="0" uid="{instrument["uid"]}"/>')
    slot = f'<SLOT bypassed="0" id="1">{plugin}' + (f"<STATE>{juce_base64(state)}</STATE>" if state else "") + "</SLOT>"
    return (f'<TRACKTEMPLATE name={quoteattr(name)} bus="0" instrument="1" musical="1" gainDb="{gain_db:.1f}" pan="0.0" colour="-1" note="">'
            f"<EFFECTS>{slot}</EFFECTS></TRACKTEMPLATE>")


# ---------------------------------------------------------------------------------------------
# Writing notes.

class Part:
    """A preset being written: its notes (pitch, start, length, velocity) and controller lanes."""

    def __init__(self, bars):
        self.bars = bars
        self.notes = []
        self.lanes = {}

    def add(self, p, start, length, velocity):
        self.notes.append((p, start, max(1, length), velocity))
        return self

    def grid(self, note, pattern, bar=None, length=S // 2):
        """A drum's grid of 16th steps ('X' accent, 'x' normal, 'o'/'w' weak, 'g' ghost, '.' rest):
        a bar's 16 steps repeated over every bar, or put in one bar; a longer pattern runs on
        across the bars from the first."""
        pattern = pattern.replace(" ", "")

        if bar is not None:
            steps = [(bar * 16 + i, c) for i, c in enumerate(pattern[:16])]
        elif len(pattern) <= 16:
            steps = [(b * 16 + i, c) for b in range(self.bars) for i, c in enumerate(pattern)]
        else:
            steps = list(enumerate(pattern))

        for step, c in steps:
            if c in VEL and step < self.bars * 16:
                self.add(note, step * S, length, VEL[c])

        return self

    def cycle(self, note, every, velocity="o", offset=0):
        """A hit every `every` 16ths through the whole part: 3 or 5 against the bar's 16."""
        for step in range(offset, self.bars * 16, every):
            self.add(note, step * S, S // 2, VEL[velocity])

        return self

    def line(self, tokens, gate=0.5, bar=0, repeat=True):
        """A line from 16th-step tokens: a pitch ('a1'), '.' a rest, '-' holding the note before.
        A pitch ends '!' for an accent, '_' for a soft note, '~' to slide into the next note
        (overlapping it by 20 ticks). One token list of a bar's 16 steps (or more) is repeated over
        the bars from `bar` unless repeat is off."""
        tokens = tokens.split()
        count = len(tokens)
        bars = range(bar, self.bars) if repeat else [bar]
        starts = []

        for b in bars:
            for i, t in enumerate(tokens):
                step = b * 16 + i if count <= 16 else bar * 16 + i

                if step >= self.bars * 16:
                    break

                starts.append((step, t))

            if count > 16:
                break

        notes = []

        for k, (step, t) in enumerate(starts):
            if t in (".", "-"):
                continue

            slide = t.endswith("~")
            accent = t.endswith("!")
            soft = t.endswith("_")
            name = t.rstrip("~!_")
            held = 1

            while k + held < len(starts) and starts[k + held][1] == "-":
                held += 1

            velocity = 118 if accent else (64 if soft else 96)
            length = int(held * S * (gate if held == 1 else 1.0)) if not slide else held * S + SLIDE
            notes.append([pitch(name), step * S, length, velocity])

        for n in notes:
            self.add(*n)

        return self

    def chords(self, chords, rhythm=None, length_steps=None, velocity=92, bar=0):
        """Chords one to a bar (a list of pitch-name lists) from `bar`, held the bar, or struck on
        the 16th steps of `rhythm` (an 'x'/'X' string) for length_steps each."""
        for i, chord in enumerate(chords):
            b = bar + i

            if b >= self.bars:
                break

            ps = [pitch(n) for n in chord]

            if rhythm is None:
                for p in ps:
                    self.add(p, b * BAR, BAR - 40, velocity)
            else:
                for step, c in enumerate(rhythm.replace(" ", "")):
                    if c in VEL:
                        for p in ps:
                            self.add(p, b * BAR + step * S, (length_steps or 1) * S - 20, VEL[c] if c != "x" else velocity)

        return self

    def lane(self, controller, points):
        """A controller's points: (tick, value 0-1)."""
        self.lanes[controller] = points
        return self


class Preset:
    def __init__(self, name, role, sub, style, energy, character, description, part, tempo, voice, tags=(), key=KEY):
        self.name, self.role, self.sub, self.style = name, role, sub, style
        self.energy, self.character, self.description = energy, character, description
        self.part, self.tempo, self.voice, self.tags = part, tempo, voice, list(tags)
        self.key = key if voice[0] != "drums" else ""

    def xml(self, indent=""):
        p = self.part
        tags = ",".join(self.tags)
        out = [f'{indent}<TORQWAVE_SEQUENCE version="1" name={quoteattr(self.name)} role={quoteattr(self.role)} subRole={quoteattr(self.sub)} '
               f'key={quoteattr(self.key)} tags={quoteattr(tags)} note={quoteattr(self.description)} style={quoteattr(self.style)} '
               f'energy="{self.energy}" character={quoteattr(",".join(self.character))} ticksPerBeat="{TPB}" '
               f'writtenTempo="{self.tempo:.1f}" beatsPerBar="4" channel="{10 if self.voice[0] == "drums" else 1}" offset="0" '
               f'length="{p.bars * BAR}" loopLength="0">',
               f"{indent}  <NOTES>"]

        for n in sorted(p.notes, key=lambda n: (n[1], n[0], n[2])):
            out.append(f'{indent}    <N p="{n[0]}" s="{n[1]}" l="{n[2]}" v="{n[3] / 127:.6f}"/>')

        out.append(f"{indent}  </NOTES>")

        for controller in sorted(p.lanes):
            out.append(f'{indent}  <LANE controller="{controller}">')

            for tick, value in p.lanes[controller]:
                out.append(f'{indent}    <P t="{tick}" v="{value:.6f}"/>')

            out.append(f"{indent}  </LANE>")

        kind, sound = self.voice

        # The voice names a factory kit or patch, which the instrument loads as it is now.
        if kind == "drums":
            kit = kit_for(sound, self.name)
            label, instrument, settings = f"Basic Drums: {kit}", DRUMS, {"factoryKit": kit}
        else:
            patch = patch_for(sound, self.name)
            label, instrument, settings = f"Basic Synth: {patch}", SYNTH, {"factoryPatch": patch}

        out.append(f"{indent}  <VOICE>{voice_xml(label, instrument, settings, GAINS.get(self.name, 0.0))}</VOICE>")
        out.append(f"{indent}</TORQWAVE_SEQUENCE>")
        return "\n".join(out)


PRESETS = []

# ---------------------------------------------------------------------------------------------
# Voices: the factory patch or kit each kind of part plays on.

HERE = os.path.dirname(os.path.abspath(__file__))


def read_voices():
    """Scripts/voice_names.txt, as TorqwaveTests --voice-names prints it: the patches by category, and the kits."""
    patches, kits = {}, []

    with open(os.path.join(HERE, "voice_names.txt"), encoding="utf-8") as f:
        for line in f:
            kind, a, b = (line.rstrip("\n").split("|", 2) + ["", ""])[:3]

            if kind == "patch":
                patches.setdefault(a, []).append(b)
            elif kind == "kit":
                kits.append(a)

    return patches, kits


FACTORY_PATCHES, FACTORY_KITS = read_voices()

# Each kind of part: the patches it may play on, as (category, how many from its start) or names.
# The patches made by hand come first in each category.
VOICE_PATCHES = {
    "sub":      ["Warehouse Sub", "Iron Sub"],
    "roll":     ["Rolling Offbeat", "Rumble Floor", "Bunker Kick Bass"],
    "ebm":      ["Pressure Bass", "Grinder", ("Bass", 14)],
    "acid":     [("Acid", 10)],
    "dark":     [("Bass", 14), ("Reese", 4)],
    "reese":    [("Reese", 10)],
    "psy":      ["Rubber Acid", "Pressure Bass", "Rolling Offbeat"],
    "chipbass": ["Square Acid", "Iron Sub"],
    "stab":     [("Stab", 10)],
    "chord":    ["Dub Chord Stab", "Minor Stab", "Organ Stab", "Pump Chord"],
    "organ":    ["Organ Stab"],
    "brass":    ["Rave Hoover Stab", "Factory Hit", "Minor Stab"],
    "pad":      [("Pad", 12)],
    "warm":     ["Grey Sky", "Sodium Lamp", "Low Ceiling"],
    "drone":    [("Drone", 10)],
    "arp":      [("Pluck", 8), "Clockwork"],
    "pluck":    [("Pluck", 10)],
    "glass":    ["Glass Shard", "Dark Kalimba", "Cold Bell"],
    "bell":     [("Bell", 8)],
    "lead":     [("Lead", 10)],
    "saw":      ["Cold Saw Lead", "Rust Lead", "Mercury Lead"],
    "chip":     ["Bit Pluck", "Industrial Whistle"],
    "sweep":    ["Tension Riser", "Siren Climb", "Machine Spin Up"],
    "noise":    ["Downlifter Noise", "Radio Static", "Ventilation"],
}

VOICE_KITS = {
    "standard": ["Standard"],
    "rumble":   ["Dark Techno"],
    "machine":  ["EBM Machine"],
    "hard":     ["Industrial"],
    "tight":    ["EBM Machine", "Standard"],
    "boom":     ["Electro"],
    "break":    ["Breaks", "Industrial Rock"],
    "chip":     ["Electro"],
}


def choices_for(sound):
    names = []

    for entry in VOICE_PATCHES[sound]:
        if isinstance(entry, tuple):
            names += [n for n in FACTORY_PATCHES[entry[0]][:entry[1]] if n not in names]
        elif entry not in names:
            assert any(entry in v for v in FACTORY_PATCHES.values()), entry + " is not a factory patch"
            names.append(entry)

    return names


def pick(names, preset_name):
    """One of the names for a preset, the same every time."""
    return names[zlib.crc32(preset_name.encode("utf-8")) % len(names)]


def patch_for(sound, preset_name):
    return pick(choices_for(sound), preset_name)


def kit_for(sound, preset_name):
    names = VOICE_KITS[sound]
    assert all(n in FACTORY_KITS for n in names), names
    return pick(names, preset_name)


def read_gains():
    """Scripts/preset_gains.txt: each preset's track gain in dB (name|gain), set by --calibrate from measured peaks."""
    gains, path = {}, os.path.join(HERE, "preset_gains.txt")

    if os.path.exists(path):
        with open(path, encoding="utf-8") as f:
            for line in f:
                name, _, gain = line.rstrip("\n").rpartition("|")

                if name:
                    gains[name] = float(gain)

    return gains


GAINS = read_gains()


def preset(name, role, sub, style, energy, character, description, part, tempo, voice, tags=()):
    PRESETS.append(Preset(name, role, sub, style, energy, character, description, part, tempo, voice, tags))


def drums(sound="standard"):
    return ("drums", sound)


def synth(patch):
    return ("synth", patch)


# ---------------------------------------------------------------------------------------------
# Drums (30).

D = "Drums"

g = Part(1).grid(KICK, "X...X...X...X...").grid(KICK, "..g...g...g...g.").grid(OH, "..o...o...o...o.").grid(RIDE, "x.o.x.o.x.o.x.o.").grid(CLAP, "....x.......x...")
preset("Rumble Roller", D, "Full Grooves", "Techno/Dark", 3, ["Dark", "Dense", "Straight", "Distorted"],
       "A short kick with a low rumble on the 16th after it, offbeat open hats and a ride in 8ths", g, 132, drums("rumble"))

g = Part(2).grid(KICK, "X...X...X...X...").grid(KICK, "...g.......g....").grid(CLAP, "....X.......X...").grid(SNARE, "....o.......o...") \
    .grid(CH, "wwxwwwxwwwxwwwxw").grid(OH, "..x...x...x...x.", bar=1)
preset("Peak Hour Driver", D, "Full Grooves", "Techno/Driving", 5, ["Dark", "Dense", "Straight", "Clean"],
       "Four to the floor, clap and snare on 2 and 4, ghost kicks before them, 16th hats with the offbeat lifted", g, 134, drums())

g = Part(4).grid(KICK, "X...X...X...X...").grid(CLAP, "....X.......X...").grid(OH, "..x...x...x...x.").grid(RIDE, "x.x.x.x.x.x.x.x.") \
    .grid(SNARE, "............gogX", bar=3)
preset("Warehouse Stomp", D, "Full Grooves", "Techno/Peak-time", 5, ["Dark", "Dense", "Straight", "Distorted"],
       "A peak-time groove with open hats on the offbeats and a ride on the 8ths; a snare pickup ends bar 4", g, 136, drums("hard"))

g = Part(2).grid(KICK, "X...X...X...X...").grid(LT, "..x...o..x....o.").grid(MT, ".o...x...o..x...").grid(HT, "...o...x.....x.o") \
    .grid(COWBELL, "......x.......x.", bar=1).grid(RIM, "g.g.g.g.g.g.g.g.")
preset("Tool Loop Percussion", D, "Full Grooves", "Techno/Percussive", 4, ["Dark", "Dense", "Straight", "Clean"],
       "A loop track of toms in 16ths with shifting accents over a straight kick, the cowbell coming in on bar 2", g, 138, drums())

g = Part(4).grid(KICK, "X...X...X...X...").cycle(RIM, 3, "o").cycle(SHAKER, 5, "w", 2).grid(OH, "..w...w...w...w.")
preset("Three Against Four", D, "Full Grooves", "Techno/Hypnotic", 3, ["Dark", "Sparse", "Straight", "Clean"],
       "A rim every three 16ths and a shaker every five turn against the kick, meeting again after four bars", g, 130, drums())

g = Part(2).grid(KICK, "X...X...X...X...").grid(RIM, "...o..o.......o.").grid(CH, "..w...w...w...w.").grid(CLAP, "............o...", bar=1)
preset("Sparse Click Groove", D, "Full Grooves", "Techno/Minimal", 2, ["Bright", "Sparse", "Straight", "Clean"],
       "Kick, a few syncopated rims, soft offbeat hats and one clap on the fourth beat of bar 2", g, 126, drums())

g = Part(2).grid(KICK, "X...X...X...X...").grid(LT, "..x...x...x..x.x").grid(SNARE, "....X.......X...").grid(COWBELL, ".w.w.w.w.w.w.w.w")
preset("Industrial Pound", D, "Full Grooves", "Techno/Industrial", 4, ["Dark", "Dense", "Straight", "Distorted"],
       "A pounding kick with low toms in its gaps, a hard snare on 2 and 4 and metal on every offbeat 16th", g, 128, drums("hard"))

g = Part(4).grid(KICK, "X...X...X...X...").grid(KICK, "..gg..gg..gg..gg").grid(COWBELL, "x..x..x..x..x..x").grid(OH, "..o...o...o...o.") \
    .grid(KICK, "........xxxxXXXX", bar=3)
preset("Hard Offbeat Rumble", D, "Full Grooves", "Techno/Hard", 5, ["Dark", "Dense", "Straight", "Distorted"],
       "Fast four-to-the-floor with a rumble between the kicks and metal accented every three 16ths; a kick roll ends bar 4", g, 150, drums("rumble"))

g = Part(2).grid(KICK, "x.......x.x.....").grid(SNARE, "....X.......X...").grid(CH, "x.x.x.x.x.x.x.x.")
preset("Body Music Beat", D, "Full Grooves", "Industrial/EBM", 3, ["Dark", "Sparse", "Straight", "Clean"],
       "The machine backbeat: kick on 1 and 3 with a push, a big snare on 2 and 4, flat 8th hats", g, 120, drums("machine"))

g = Part(1).grid(KICK, "x...x...x...x...").grid(CLAP, "....x.......x...").grid(SNARE, "....x.......x...").grid(OH, "..x...x...x...x.").grid(CH, "xx.xxx.xxx.xxx.x")
preset("Machine Disco Beat", D, "Full Grooves", "Industrial/EBM", 4, ["Dark", "Dense", "Straight", "Clean"],
       "Four-to-the-floor at flat machine velocities, clap and snare together, open hats on the offbeats", g, 124, drums("machine"))

g = Part(2).grid(KICK, "x.....x.x.......").grid(SNARE, "....x.......x...").grid(CH, "o.o.o.o.o.o.o.o.").grid(KICK, "x.....x.x.....x.", bar=1)
preset("Darkwave Drum Machine", D, "Full Grooves", "Industrial/Darkwave", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "A drum-machine pop beat: kick on 1, the and of 2 and 3, snare on 2 and 4, soft 8th hats", g, 110, drums("machine"))

g = Part(4).grid(KICK, "X...X...X...X...").grid(CH, "wwowwwowwwowwwow").grid(CLAP, "....x.......x...").grid(COWBELL, "...o.....o.o....")
preset("Techno Body Groove", D, "Full Grooves", "Industrial/Techno-body", 4, ["Dark", "Dense", "Straight", "Distorted"],
       "Techno kick and 16th hats with a metal figure, ready for an EBM bass in the gaps", g, 132, drums())

preset("Kick Only Four", D, "Kick", "Techno/Club tools", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "The kick alone on every beat, to build on", Part(1).grid(KICK, "x...x...x...x..."), 128, drums())

preset("Kick and Rumble Tool", D, "Kick", "Techno/Dark", 3, ["Dark", "Dense", "Straight", "Distorted"],
       "Kick on the beat with a quieter rumble on the three 16ths after it", Part(1).grid(KICK, "Xggg" * 4), 130, drums("rumble"))

g = Part(4).grid(KICK, "x...x...x...x...", bar=0).grid(KICK, "x...x...x...x...", bar=1).grid(KICK, "x.x.x.x.x.x.x.x.", bar=2).grid(KICK, "xxxxxxxxXXXXXXXX", bar=3)
preset("Kick Roll Builder", D, "Kick", "Techno/Hard", 5, ["Dark", "Dense", "Straight", "Distorted"],
       "Quarters for two bars, then 8ths, then 16ths into the drop", g, 150, drums("hard"))

preset("Backbeat Clap Layer", D, "Snare & Clap", "Techno/Driving", 3, ["Bright", "Sparse", "Straight", "Clean"],
       "Clap and snare together on 2 and 4", Part(1).grid(CLAP, "....X.......X...").grid(SNARE, "....o.......o..."), 134, drums())
preset("Gated Snare Machine", D, "Snare & Clap", "Industrial/EBM", 3, ["Dark", "Sparse", "Straight", "Clean"],
       "A big flat snare on 2 and 4, with a pickup on the last 16th of bar 2",
       Part(2).grid(SNARE, "....X.......X...").grid(SNARE, "...............x", bar=1), 120, drums("machine"))
preset("Flam Snare Figure", D, "Snare & Clap", "Industrial/Electro-industrial", 3, ["Dark", "Sparse", "Straight", "Distorted"],
       "Snares with a ghost a 16th before them, syncopated across two bars",
       Part(2).grid(SNARE, "...gX......gX...").grid(SNARE, "...gX....gX..gX.", bar=1), 118, drums())

preset("Offbeat Open Hats", D, "Hats & Tops", "Techno/Club tools", 2, ["Bright", "Sparse", "Straight", "Clean"],
       "An open hat on every offbeat", Part(1).grid(OH, "..x...x...x...x."), 128, drums())
preset("Shaped Sixteenth Hats", D, "Hats & Tops", "Techno/Driving", 3, ["Bright", "Dense", "Straight", "Clean"],
       "Closed hats on every 16th, the offbeat louder and the rest under 70", Part(1).grid(CH, "wwxwwwxwwwxwwwxw"), 134, drums())
preset("Ride Eighths", D, "Hats & Tops", "Techno/Dark", 3, ["Dark", "Sparse", "Straight", "Clean"],
       "A ride on the 8ths, every second hit quieter", Part(1).grid(RIDE, "x.o.x.o.x.o.x.o."), 132, drums())
preset("Percussive Top Loop", D, "Hats & Tops", "Techno/Percussive", 4, ["Bright", "Dense", "Straight", "Clean"],
       "Closed hats, shaker and an open hat woven into one top loop",
       Part(2).grid(CH, "x.wx.wx.x.wx.wx.").grid(SHAKER, ".w.w.w.w.w.w.w.w").grid(OH, "..o.......o.....", bar=1), 138, drums())

preset("Metal Sixteenths", D, "Percussion", "Techno/Hard", 4, ["Dark", "Dense", "Straight", "Distorted"],
       "Metallic 16ths accented every three steps, then every six", Part(2).grid(COWBELL, "x..x..x..x..x..x").grid(COWBELL, "x.....x.....x...", bar=1)
       .grid(COWBELL, ".gg.gg.gg.gg.gg.").grid(COWBELL, ".ggggg.ggggg.ggg", bar=1), 148, drums())
preset("Tom Conga Roll", D, "Percussion", "Techno/Percussive", 4, ["Dark", "Dense", "Straight", "Clean"],
       "Low, mid and high toms rolling through the bar like congas",
       Part(1).grid(LT, "x.....o...x.....").grid(MT, "..o.x.....o.x...").grid(HT, ".w.....x.w.....x"), 136, drums())
preset("Five-Step Shaker", D, "Percussion", "Techno/Hypnotic", 3, ["Bright", "Sparse", "Straight", "Clean"],
       "A shaker every five 16ths, coming round against the bar over four bars", Part(4).cycle(SHAKER, 5, "o").cycle(RIM, 7, "w", 3), 130, drums())

preset("Snare Roll Fill", D, "Fills", "Techno/Driving", 4, ["Bright", "Dense", "Straight", "Clean"],
       "A bar's fill: 8th snares, then 16ths climbing to an accent",
       Part(1).grid(SNARE, "g.o.x.x.ggooxxXX"), 134, drums())
preset("Tom Run Down", D, "Fills", "Techno/Industrial", 4, ["Dark", "Dense", "Straight", "Clean"],
       "16th toms falling from high to low over the last two beats",
       Part(1).grid(KICK, "x...x...........").grid(HT, "........xxx.....").grid(MT, "...........xxx..").grid(LT, "..............XX"), 128, drums())
preset("Clap Stutter Fill", D, "Fills", "Industrial/EBM", 3, ["Dark", "Sparse", "Straight", "Clean"],
       "Kick and snare, then claps stuttering on the last beat", Part(1).grid(KICK, "x.......x.......").grid(SNARE, "....x...........")
       .grid(CLAP, "............xxxX"), 122, drums("machine"))

preset("Half-time Break", D, "Breaks", "Industrial/Darkwave", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "Kick on 1 and snare on 3: the groove at half speed for a breakdown",
       Part(2).grid(KICK, "x.........x.....").grid(SNARE, "........X.......").grid(CH, "o...o...o...o..."), 110, drums("machine"))
preset("Industrial Breakdown", D, "Breaks", "Techno/Industrial", 3, ["Dark", "Sparse", "Straight", "Distorted"],
       "No kick: low toms, rims and metal holding the pulse for a breakdown",
       Part(2).grid(LT, "x.....x...x.....").grid(RIM, "....o.......o..o").grid(COWBELL, "..w...w...w...w."), 128, drums())

# ---------------------------------------------------------------------------------------------
# Bass (30), mono, round A1 (55 Hz).

B = "Bass"

preset("Sub Root Pulse", B, "Sub", "Techno/Driving", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "The sub on the root, an 8th on each offbeat", Part(1).line(". . a1 . . . a1 . . . a1 . . . a1 .", gate=1.0), 134, synth("sub"))
preset("Offbeat Sub", B, "Sub", "Techno/Club tools", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "A short sub on every offbeat, out of the kick's way", Part(1).line(". . a1 . . . a1 . . . a1 . . . a1 .", gate=0.9), 128, synth("sub"))
preset("Sub Drone Hold", B, "Sub", "Techno/Dark", 1, ["Dark", "Sparse", "Straight", "Clean"],
       "A held A1 for three bars, falling to F1 in bar 4",
       Part(4).line("a1 - - - - - - - - - - - - - - -", gate=1.0, bar=0, repeat=False).line("a1 - - - - - - - - - - - - - - -", gate=1.0, bar=1, repeat=False)
       .line("a1 - - - - - - - - - - - - - - -", gate=1.0, bar=2, repeat=False).line("f1 - - - - - - - - - - - - - - -", gate=1.0, bar=3, repeat=False), 130, synth("sub"))
preset("Rolling Sub Sixteenths", B, "Sub", "Techno/Peak-time", 4, ["Dark", "Dense", "Straight", "Clean"],
       "Kick-bass-bass-bass: the sub on the three 16ths after each kick", Part(1).line(". a1 a1 a1 . a1 a1 a1 . a1 a1 a1 . a1 a1 a1", gate=0.8), 136, synth("sub"))
preset("Darkwave Sub Roots", B, "Sub", "Industrial/Darkwave", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "8th roots under A minor, F, C and G, a bar each",
       Part(4).line("a1 . a1 . a1 . a1 . a1 . a1 . a1 . a1 .", bar=0, repeat=False).line("f1 . f1 . f1 . f1 . f1 . f1 . f1 . f1 .", bar=1, repeat=False)
       .line("c2 . c2 . c2 . c2 . c2 . c2 . c2 . c2 .", bar=2, repeat=False).line("g1 . g1 . g1 . g1 . g1 . g1 . g1 . g1 .", bar=3, repeat=False), 110, synth("sub"))


def bars_of(*lines):
    """Lines, one to a bar."""
    part = Part(len(lines))

    for b, tokens in enumerate(lines):
        part.line(tokens, bar=b, repeat=False)

    return part


preset("Rolling Kick Gap Bass", B, "Synth Bass", "Techno/Driving", 5, ["Dark", "Dense", "Straight", "Distorted"],
       "The driving roll: three 16ths between the kicks, moving to C and G in bar 4",
       bars_of(*([". a1 a1 a1! . a1 a1 a1 . a1 a1 a1! . a1 a1 a1"] * 3 + [". c2 c2 c2! . c2 c2 c2 . g1 g1 g1! . g1 g1 g1"])), 134, synth("roll"))
preset("Peak Rumble Bass", B, "Synth Bass", "Techno/Dark", 4, ["Dark", "Dense", "Straight", "Distorted"],
       "Offbeat 16ths on the root with an octave accent at the end of each half bar",
       Part(2).line(". . a1 a1 . . a1 a2! . . a1 a1 . . a1 a1"), 132, synth("roll"))
preset("Belgian Body Bass", B, "Synth Bass", "Industrial/EBM", 4, ["Dark", "Dense", "Straight", "Clean"],
       "16ths in the kick's gaps with the jump to the octave, moving A to F to G",
       bars_of(". a1 a2 a1 . a1 a2 a1 . a1 a2 a1 . a1 a2 a1", ". a1 a2 a1 . a1 a2 a1 . a1 a2 a1 . a1 a2 a1",
               ". f1 f2 f1 . f1 f2 f1 . f1 f2 f1 . f1 f2 f1", ". g1 g2 g1 . g1 g2 g1 . g1 g2 g1 . a1 a2 a1"), 122, synth("ebm"))
preset("Body Gallop", B, "Synth Bass", "Industrial/EBM", 4, ["Dark", "Dense", "Straight", "Clean"],
       "The gallop: an 8th then two 16ths on every beat, the last beat jumping up an octave",
       Part(2).line("a1 . a1 a1 a1 . a1 a1 a1 . a1 a1 a2 . a2 a2"), 120, synth("ebm"))
preset("Octave Sequencer Bass", B, "Synth Bass", "Industrial/Synth-EBM", 3, ["Dark", "Dense", "Straight", "Clean"],
       "8ths leaping between root and octave on A, F, C and G",
       bars_of("a1 . a2 . a1 . a2 . a1 . a2 . a1 . a2 .", "f1 . f2 . f1 . f2 . f1 . f2 . f1 . f2 .",
               "c2 . c3 . c2 . c3 . c2 . c3 . c2 . c3 .", "g1 . g2 . g1 . g2 . g1 . g2 . g1 . g2 ."), 126, synth("ebm"))
preset("Phrygian Shift Bass", B, "Synth Bass", "Industrial/Electro-industrial", 4, ["Dark", "Dense", "Straight", "Distorted"],
       "16th root with a semitone shift up to B flat at the end of every bar",
       Part(2).line("a1 . a1 a1 . a1 a1 . a1 . a1 a1 . bb1 bb1 ."), 118, synth("dark"), tags=["Phrygian"])
preset("Techno Body Sixteenths", B, "Synth Bass", "Industrial/Techno-body", 4, ["Dark", "Dense", "Straight", "Distorted"],
       "EBM 16ths in a techno kick's gaps, moving to F in bar 3",
       bars_of(". a1 a1 a2 . a1 a1 a2 . a1 a1 a2 . a1 a1 a2", ". a1 a1 a2 . a1 a1 a2 . a1 a1 a2 . a1 a1 a2",
               ". f1 f1 f2 . f1 f1 f2 . f1 f1 f2 . f1 f1 f2", ". a1 a1 a2 . a1 a1 a2 . g1 g1 g2 . g1 g1 g2"), 132, synth("ebm"))
preset("Offbeat Pump Bass", B, "Synth Bass", "Industrial/Futurepop", 4, ["Bright", "Sparse", "Straight", "Clean"],
       "Offbeat 8ths on the roots of A minor, F, C and G",
       bars_of(". . a2 . . . a2 . . . a2 . . . a2 .", ". . f2 . . . f2 . . . f2 . . . f2 .",
               ". . c2 . . . c2 . . . c2 . . . c2 .", ". . g2 . . . g2 . . . g2 . . . g2 ."), 132, synth("roll"))
preset("Distorted Offbeat Bass", B, "Synth Bass", "Industrial/Aggrotech", 5, ["Dark", "Dense", "Straight", "Distorted"],
       "Offbeat 8ths with a 16th after each, a harsh drive under a hard kick",
       Part(1).line(". . a1! a1 . . a1! a1 . . a1! a1 . . a1! c2"), 140, synth("saw"))
preset("Melodic Root Roller", B, "Synth Bass", "Techno/Melodic", 3, ["Bright", "Dense", "Straight", "Clean"],
       "Rolling 16ths on the roots of A minor, F, C and G",
       bars_of(". a1 a1 a1 . a1 a1 a1 . a1 a1 a1 . a1 a1 a1", ". f1 f1 f1 . f1 f1 f1 . f1 f1 f1 . f1 f1 f1",
               ". c2 c2 c2 . c2 c2 c2 . c2 c2 c2 . c2 c2 c2", ". g1 g1 g1 . g1 g1 g1 . g1 g1 g1 . g1 g1 g1"), 124, synth("roll"))
preset("Minimal Syncopated Bass", B, "Synth Bass", "Techno/Minimal", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "A few short syncopated notes, room left for the groove",
       Part(2).line(". . . a1 . . . . . . a1 . . e2 . ."), 126, synth("dark"))
preset("Darkwave Eighth Roots", B, "Synth Bass", "Industrial/Darkwave", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "Straight 8th roots under A minor, F, G and E minor",
       bars_of("a1 . a1 . a1 . a1 . a1 . a1 . a1 . a1 .", "f1 . f1 . f1 . f1 . f1 . f1 . f1 . f1 .",
               "g1 . g1 . g1 . g1 . g1 . g1 . g1 . g1 .", "e1 . e1 . e1 . e1 . e1 . e1 . e1 . e1 ."), 110, synth("ebm"))
preset("Industrial Rock Riff Bass", B, "Synth Bass", "Industrial/Industrial rock", 4, ["Dark", "Dense", "Straight", "Distorted"],
       "8ths on the root with flat-second and flat-fifth moves",
       Part(2).line("a1 . a1 . a1 . bb1 . a1 . a1 . eb2 . d2 .", gate=0.8), 120, synth("saw"), tags=["Phrygian", "Chromatic"])
preset("Hard Offbeat Drive", B, "Synth Bass", "Techno/Hard", 5, ["Dark", "Dense", "Straight", "Distorted"],
       "Fast offbeat 16th pairs on the root", Part(1).line(". . a1 a1 . . a1 a1 . . a1 a1 . . a1 a2"), 150, synth("roll"))

preset("Acid Root Jumper", B, "Arp Bass", "Techno/Acid", 4, ["Dark", "Dense", "Straight", "Distorted"],
       "A 303 line: root and octave jumps, rests, accents and two slides",
       Part(1).line("a1! . a1 a2~ a1 . c2! a1 . a1 a2 . g1~ a1! . e2", gate=0.55), 132, synth("acid"))
preset("Acid Phrygian Crawl", B, "Arp Bass", "Techno/Acid", 4, ["Dark", "Dense", "Straight", "Distorted"],
       "A creeping acid line through the flat second, sliding back to the root",
       Part(2).line("a1 . bb1~ a1 . a1! . c2 a1 . bb1! . a1~ g1 a1 .", gate=0.5), 136, synth("acid"), tags=["Phrygian"])
preset("Acid Pentatonic Squelch", B, "Arp Bass", "Techno/Acid", 4, ["Dark", "Dense", "Straight", "Distorted"],
       "Minor pentatonic 16ths with accents on the jumps",
       Part(1).line("a1 a1 c2 . d2! a1 . e2~ g2 . a1 . c2! d2 . a1", gate=0.5), 130, synth("acid"))
preset("Acid Slide Rider", B, "Arp Bass", "Techno/Acid", 5, ["Dark", "Dense", "Straight", "Distorted"],
       "A two-bar 303 line with longer slides and the answer an octave up",
       bars_of("a1! a1 . a2~ g2 . e2 . a1 . a1! c2~ d2 . a1 .", "a1! a1 . a2~ g2 . e2! . c3~ a2 . g2 e2 . d2~ c2"), 138, synth("acid"))
preset("Broken Octave Arp Bass", B, "Arp Bass", "Techno/Peak-time", 4, ["Dark", "Dense", "Straight", "Clean"],
       "Root, fifth, octave and minor third broken over the bar",
       Part(1).line("a1 e2 a2 c3 a1 e2 a2 c3 a1 e2 a2 c3 a1 e2 a2 e2", gate=0.5), 134, synth("dark"))
preset("Hypnotic Two-Note Bass", B, "Arp Bass", "Techno/Hypnotic", 3, ["Dark", "Sparse", "Straight", "Clean"],
       "Two notes on a three-step cycle drifting against the bar",
       Part(4).line(" ".join("a1" if i % 6 == 0 else "c2" if i % 6 == 3 else "." for i in range(64)), gate=0.6), 130, synth("dark"))
preset("Percussive Tool Bass", B, "Arp Bass", "Techno/Percussive", 3, ["Dark", "Sparse", "Straight", "Clean"],
       "Short root blips placed like percussion between the kicks",
       Part(1).line(". a1 . a1 . . a1 . . a1 . a1 . . . a2", gate=0.35), 138, synth("dark"))
preset("Harmonic Minor Arp Bass", B, "Arp Bass", "Industrial/Electro-industrial", 4, ["Dark", "Dense", "Straight", "Distorted"],
       "16ths climbing to the sharp seventh and back, in harmonic minor",
       Part(1).line("a1 c2 e2 g#2 a2 g#2 e2 c2 a1 c2 e2 g#2 b2 g#2 e2 c2", gate=0.5), 120, synth("ebm"), tags=["Harmonic minor"])
preset("Picked Eighths Downstroke", B, "Picked & Played", "Industrial/Industrial rock", 3, ["Dark", "Dense", "Straight", "Distorted"],
       "Steady picked 8ths, dropping to F and G in the last two bars",
       bars_of("a1 . a1 . a1 . a1 . a1 . a1 . a1 . a1 .", "a1 . a1 . a1 . a1 . a1 . a1 . a1 . a1 .", "f1 . f1 . f1 . f1 . f1 . f1 . f1 . f1 .",
               "g1 . g1 . g1 . g1 . g1 . g1 . g1 . g1 ."), 130, synth("saw"))
preset("Walking Dark Line", B, "Picked & Played", "Industrial/Darkwave", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "Quarter notes walking down from A to E and back", Part(2).line("a1 - - - g1 - - - f1 - - - e1 - - -"), 112, synth("ebm"))
preset("Played Root Fifth", B, "Picked & Played", "Industrial/Synth-EBM", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "Root and fifth in half notes, played", Part(2).line("a1 - - - - - - . e2 - - - - - - ."), 126, synth("sub"))

# ---------------------------------------------------------------------------------------------
# Harmony (20): chords in A minor, voiced round A3.

H = "Harmony"
Am, F, C, G, Dm, Em, Bb = ["a3", "c4", "e4"], ["f3", "a3", "c4"], ["g3", "c4", "e4"], ["g3", "b3", "d4"], ["a3", "d4", "f4"], ["g3", "b3", "e4"], ["bb3", "d4", "f4"]
Am7, Fmaj7, Am9 = ["a3", "c4", "e4", "g4"], ["f3", "a3", "c4", "e4"], ["a3", "c4", "e4", "g4", "b4"]

preset("Melodic Minor Turn", H, "Progressions", "Techno/Melodic", 3, ["Bright", "Sparse", "Straight", "Clean"],
       "i, flat VI, flat III, flat VII: A minor, F, C, G, a bar each", Part(4).chords([Am, F, C, G], velocity=84), 124, synth("chord"))
preset("Melodic Four-Six Turn", H, "Progressions", "Techno/Melodic", 3, ["Dark", "Sparse", "Straight", "Clean"],
       "i, iv, flat VI, flat VII: A minor, D minor, F, G", Part(4).chords([Am, Dm, F, G], velocity=84), 122, synth("chord"))
preset("Futurepop Minor Loop", H, "Progressions", "Industrial/Futurepop", 4, ["Bright", "Dense", "Straight", "Clean"],
       "A minor, F, C, G struck on the offbeats", Part(4).chords([Am, F, C, G], rhythm="..x...x...x...x.", length_steps=2), 132, synth("stab"))
preset("Darkwave Chorus Chords", H, "Progressions", "Industrial/Darkwave", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "A minor, F, G, E minor in half notes", Part(4).chords([Am, F, G, Em], rhythm="x.......x.......", length_steps=8, velocity=80), 110, synth("chord"))
preset("Synth-EBM Verse Chords", H, "Progressions", "Industrial/Synth-EBM", 3, ["Dark", "Dense", "Straight", "Clean"],
       "A minor, C, G, F pulsing in 8ths", Part(4).chords([Am, C, G, F], rhythm="x.x.x.x.x.x.x.x.", length_steps=1, velocity=86), 126, synth("chord"))
preset("Phrygian Descent", H, "Progressions", "Industrial/Electro-industrial", 3, ["Dark", "Sparse", "Straight", "Distorted"],
       "A minor, B flat, A minor, G: the Phrygian turn", Part(4).chords([Am, Bb, Am, G], velocity=84), 118, synth("chord"), tags=["Phrygian"])

preset("Dub Chord Echo", H, "Keys & Comping", "Techno/Minimal", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "One minor seventh on the offbeat, echoing quieter after it",
       Part(1).chords([Am7], rhythm="..x..o..w.......", length_steps=1, velocity=80), 124, synth("chord"))
preset("Offbeat Minor Comp", H, "Keys & Comping", "Techno/Driving", 3, ["Dark", "Sparse", "Straight", "Clean"],
       "Short A minor chords on the offbeats, F in bar 2", Part(2).chords([Am, F], rhythm="..x...x...x...x.", length_steps=1), 132, synth("stab"))
preset("Hypnotic One Chord", H, "Keys & Comping", "Techno/Hypnotic", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "An A minor ninth placed sparsely, shifting a 16th each bar",
       Part(4).chords([Am9], rhythm="...x............", velocity=72).chords([Am9], rhythm="....x...........", velocity=72, bar=1)
       .chords([Am9], rhythm=".....x..........", velocity=72, bar=2).chords([Am9], rhythm="......x.....x...", velocity=72, bar=3), 130, synth("chord"))
preset("Deep Minor Seventh Comp", H, "Keys & Comping", "Techno/Melodic", 3, ["Bright", "Sparse", "Straight", "Clean"],
       "A minor seventh and F major seventh in a two-bar comp",
       Part(2).chords([Am7], rhythm="x.....x.....x...", length_steps=2).chords([Fmaj7], rhythm="..x.....x.......", length_steps=3, bar=1), 122, synth("chord"))

preset("One-Note Minor Stab", H, "Stabs", "Techno/Dark", 3, ["Dark", "Sparse", "Straight", "Distorted"],
       "A short minor stab on the and of every second beat", Part(1).chords([Am], rhythm="......x.......x.", length_steps=1), 132, synth("stab"))
preset("Peak Stab Call", H, "Stabs", "Techno/Peak-time", 5, ["Dark", "Dense", "Straight", "Distorted"],
       "A stab call in bar 1, answered higher in bar 2",
       Part(2).chords([Am], rhythm="x..x..x.....x...", length_steps=1).chords([["c4", "e4", "a4"]], rhythm="..x..x..x.x.....", length_steps=1, bar=1), 136, synth("stab"))
preset("Rave Stab Fifths", H, "Stabs", "Techno/Peak-time", 5, ["Bright", "Dense", "Straight", "Distorted"],
       "Open fifths on A stabbing through a peak-time bar", Part(1).chords([["a3", "e4", "a4"]], rhythm="x..x..x...x..x..", length_steps=1), 138, synth("stab"))
preset("Industrial Metal Stab", H, "Stabs", "Techno/Industrial", 4, ["Dark", "Sparse", "Straight", "Distorted"],
       "A cluster of A, B flat and E hit like metal", Part(1).chords([["a3", "bb3", "e4"]], rhythm="x.....x.........", length_steps=1), 128, synth("stab"), tags=["Phrygian"])
preset("Phrygian Stab", H, "Stabs", "Techno/Dark", 4, ["Dark", "Sparse", "Straight", "Distorted"],
       "A minor stabs leaning on B flat at the end of the bar",
       Part(1).chords([Am], rhythm="..x...x.....", length_steps=1).chords([Bb], rhythm="..............x.", length_steps=1), 132, synth("stab"), tags=["Phrygian"])
preset("Hard Gate Stab", H, "Stabs", "Techno/Hard", 5, ["Dark", "Dense", "Straight", "Distorted"],
       "16th stabs gated in groups of three", Part(1).chords([Am], rhythm="xx.xx.xx.xx.xx.x", length_steps=1), 150, synth("stab"))
preset("Techno Body Stab", H, "Stabs", "Industrial/Techno-body", 4, ["Dark", "Sparse", "Straight", "Distorted"],
       "A minor stab answered by F in the second bar",
       Part(2).chords([Am], rhythm="x.....x.........", length_steps=1).chords([F], rhythm="x.....x...x.....", length_steps=1, bar=1), 132, synth("stab"))
preset("Alarm Stab Tritone", H, "Stabs", "Industrial/EBM", 4, ["Dark", "Sparse", "Straight", "Distorted"],
       "A and E flat together: the alarm interval, struck on the beat", Part(1).chords([["a3", "eb4"]], rhythm="x...x...x...x.x.", length_steps=1), 122, synth("stab"), tags=["Chromatic"])

preset("Power Chord Riff", H, "Strums", "Industrial/Industrial rock", 4, ["Dark", "Dense", "Straight", "Distorted"],
       "8th power chords on A with a flat-second push", Part(2).chords([["a2", "e3"]], rhythm="x.x.x.x.x.x.....", length_steps=1)
       .chords([["bb2", "f3"]], rhythm="............x.x.", length_steps=1), 120, synth("saw"), tags=["Phrygian"])
preset("Power Fifths Gallop", H, "Strums", "Industrial/Aggrotech", 5, ["Dark", "Dense", "Straight", "Distorted"],
       "Galloping power fifths: an 8th and two 16ths", Part(1).chords([["a2", "e3"]], rhythm="x.xxx.xxx.xxx.xx", length_steps=1), 140, synth("saw"))

# ---------------------------------------------------------------------------------------------
# Pads (20).

P = "Pads"
preset("Minor Bed", P, "Sustained", "Techno/Melodic", 2, ["Bright", "Sparse", "Straight", "Clean"],
       "A minor held for four bars", Part(4).chords([Am] * 4, velocity=70), 124, synth("pad"))
preset("Dark Minor Hold", P, "Sustained", "Techno/Dark", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "A minor with the ninth, low and held", Part(4).chords([["a2", "e3", "b3", "c4"]] * 4, velocity=66), 130, synth("pad"))
preset("Futurepop Pad Loop", P, "Sustained", "Industrial/Futurepop", 3, ["Bright", "Sparse", "Straight", "Clean"],
       "A minor, F, C, G held a bar each", Part(4).chords([Am, F, C, G], velocity=72), 132, synth("pad"))
preset("Darkwave Chorus Pad", P, "Sustained", "Industrial/Darkwave", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "A minor, F, G, E minor held a bar each", Part(4).chords([Am, F, G, Em], velocity=70), 110, synth("pad"))
preset("Synth-EBM Pad", P, "Sustained", "Industrial/Synth-EBM", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "A minor, C, G, F under a verse", Part(4).chords([Am, C, G, F], velocity=70), 126, synth("pad"))
preset("Hypnotic Fifth Bed", P, "Sustained", "Techno/Hypnotic", 1, ["Dark", "Sparse", "Straight", "Clean"],
       "A and E held for eight bars, nothing else", Part(8).chords([["a2", "e3", "a3"]] * 8, velocity=64), 130, synth("pad"))

part = Part(8)
for i, chord in enumerate([Am, Dm, F, Em]):
    for p_ in chord:
        part.add(pitch(p_), i * 2 * BAR, 2 * BAR - 40, 70)
preset("Slow Minor Progression", P, "Evolving", "Techno/Melodic", 2, ["Bright", "Sparse", "Straight", "Clean"],
       "A minor, D minor, F, E minor, two bars each", part, 122, synth("pad"))
preset("Harmonic Minor Cluster", P, "Evolving", "Industrial/Electro-industrial", 2, ["Dark", "Sparse", "Straight", "Distorted"],
       "A, C, E and G sharp: the harmonic minor's tension held", Part(4).chords([["a3", "c4", "e4", "g#4"]] * 4, velocity=66), 118, synth("pad"), tags=["Harmonic minor"])
preset("Phrygian Cloud", P, "Evolving", "Techno/Dark", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "A and E with B flat rubbing above them", Part(4).chords([["a2", "e3", "bb3"]] * 4, velocity=62), 130, synth("pad"), tags=["Phrygian"])
part = Part(4).chords([["a3", "e4"]] * 4, velocity=68)
for i, top in enumerate(["c5", "d5", "e5", "c5"]):
    part.add(pitch(top), i * BAR, BAR - 40, 64)
preset("Moving Inner Voice", P, "Evolving", "Techno/Melodic", 2, ["Bright", "Sparse", "Straight", "Clean"],
       "A and E held while the top moves C, D, E and back", part, 124, synth("pad"))
preset("Night Drive Pad", P, "Evolving", "Industrial/Synth-EBM", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "A minor seventh to F major seventh, two bars each",
       Part(4).chords([Am7, Am7, Fmaj7, Fmaj7], velocity=68), 126, synth("pad"))

swell = [(i * BAR // 2, min(1.0, i / 8)) for i in range(9)]
preset("Root and Fifth Swell", P, "Swells", "Techno/Dark", 3, ["Dark", "Sparse", "Straight", "Clean"],
       "A and E swelling up over four bars", Part(4).chords([["a2", "e3"]] * 4, velocity=72).lane(11, swell), 130, synth("pad"))
preset("Octave Swell", P, "Swells", "Techno/Hypnotic", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "Octave As swelling in and out over four bars",
       Part(4).chords([["a2", "a3"]] * 4, velocity=70).lane(11, [(0, 0.1), (2 * BAR, 1.0), (4 * BAR - 1, 0.1)]), 130, synth("pad"))
preset("Minor Swell Build", P, "Swells", "Techno/Driving", 4, ["Dark", "Dense", "Straight", "Clean"],
       "A minor rising from nothing over eight bars, the filter opening with it",
       Part(8).chords([Am] * 8, velocity=76).lane(11, [(i * BAR, i / 8) for i in range(9)]).lane(74, [(i * BAR, 0.3 + 0.7 * i / 8) for i in range(9)]), 134, synth("pad"))
preset("Breakdown Swell", P, "Swells", "Techno/Melodic", 3, ["Bright", "Sparse", "Straight", "Clean"],
       "F to G, swelling into the drop", Part(4).chords([F, F, G, G], velocity=74).lane(11, [(0, 0.2), (4 * BAR - 1, 1.0)]), 124, synth("pad"))

preset("Low A Drone", P, "Drones", "Techno/Dark", 1, ["Dark", "Sparse", "Straight", "Clean"],
       "A1 held for eight bars", Part(8).add(pitch("a1"), 0, 8 * BAR - 40, 80), 130, synth("drone"))
preset("Fifth Drone", P, "Drones", "Techno/Industrial", 1, ["Dark", "Sparse", "Straight", "Distorted"],
       "A1 and E2 held for eight bars", Part(8).add(pitch("a1"), 0, 8 * BAR - 40, 80).add(pitch("e2"), 0, 8 * BAR - 40, 70), 128, synth("drone"))
preset("Phrygian Drone", P, "Drones", "Techno/Industrial", 2, ["Dark", "Sparse", "Straight", "Distorted"],
       "A1 with B flat 2 grinding above it", Part(8).add(pitch("a1"), 0, 8 * BAR - 40, 80).add(pitch("bb2"), 0, 8 * BAR - 40, 60), 128, synth("drone"), tags=["Phrygian"])
preset("Rumble Drone", P, "Drones", "Techno/Hard", 3, ["Dark", "Dense", "Straight", "Distorted"],
       "A low A drone with the filter breathing every bar",
       Part(4).add(pitch("a1"), 0, 4 * BAR - 40, 90).lane(74, [(i * BAR // 2, 0.35 if i % 2 == 0 else 0.6) for i in range(9)]), 150, synth("drone"))
preset("Tension Drone", P, "Drones", "Industrial/EBM", 2, ["Dark", "Sparse", "Straight", "Distorted"],
       "A and E flat a tritone apart, held under a breakdown", Part(4).add(pitch("a2"), 0, 4 * BAR - 40, 76).add(pitch("eb3"), 0, 4 * BAR - 40, 66), 122, synth("drone"), tags=["Chromatic"])

# ---------------------------------------------------------------------------------------------
# Arps (20).

A = "Arps"


def arp(chords, figure, gate=0.5):
    """An arpeggio, a bar per chord: `figure` picks the chord's notes by index on each 16th
    ('.' a rest, '+' an index an octave up)."""
    part = Part(len(chords))

    for b, chord in enumerate(chords):
        tokens = []

        for f in figure.split():
            if f == ".":
                tokens.append(".")
                continue

            up = f.endswith("+")
            tokens.append(name_of(pitch(chord[int(f.rstrip("+")) % len(chord)]) + (12 if up else 0)))

        part.line(" ".join(tokens), gate=gate, bar=b, repeat=False)

    return part


UPDOWN = "0 1 2 0+ 2 1 0 1 2 0+ 2 1 0 1 2 1"
preset("Minor Triad Up-Down", A, "Up & Down", "Techno/Melodic", 3, ["Bright", "Dense", "Straight", "Clean"],
       "A minor up and down in 16ths", arp([Am], UPDOWN), 124, synth("arp"))
preset("Seventh Up-Down", A, "Up & Down", "Techno/Melodic", 3, ["Bright", "Dense", "Straight", "Clean"],
       "A minor seventh and F major seventh climbing and falling", arp([Am7, Fmaj7], "0 1 2 3 0+ 3 2 1 0 1 2 3 0+ 3 2 1"), 122, synth("arp"))
preset("Futurepop Arp Loop", A, "Up & Down", "Industrial/Futurepop", 4, ["Bright", "Dense", "Straight", "Clean"],
       "Up and down through A minor, F, C and G", arp([Am, F, C, G], UPDOWN), 132, synth("arp"))
preset("Darkwave Arp", A, "Up & Down", "Industrial/Darkwave", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "8th arpeggios through A minor, F, G and E minor", arp([Am, F, G, Em], "0 . 1 . 2 . 0+ . 2 . 1 . 0 . 2 ."), 110, synth("pluck"))
preset("Harmonic Minor Up-Down", A, "Up & Down", "Industrial/Electro-industrial", 3, ["Dark", "Dense", "Straight", "Distorted"],
       "Up and down through A minor with the sharp seventh", arp([["a3", "c4", "e4", "g#4"]], "0 1 2 3 0+ 3 2 1 0 1 2 3 0+ 3 2 1"), 118, synth("arp"), tags=["Harmonic minor"])

preset("Broken Root Fifth Octave", A, "16th Sequences", "Techno/Driving", 4, ["Dark", "Dense", "Straight", "Clean"],
       "The 1-5-8-flat 3 figure: A, E, A, C in 16ths", arp([["a3", "e4", "a4", "c5"]], "0 1 2 3 0 1 2 3 0 1 2 3 0 1 2 1"), 134, synth("arp"))
preset("Peak Sixteenth Sequence", A, "16th Sequences", "Techno/Peak-time", 5, ["Dark", "Dense", "Straight", "Distorted"],
       "A driving 16th sequence over two bars, the second rising",
       Part(2).line("a3 a3 c4 a3 e4 a3 c4 a3 a3 a3 c4 a3 g4 a3 e4 c4", gate=0.45, bar=0, repeat=False)
       .line("a3 a3 c4 a3 e4 a3 g4 a3 a4 a3 g4 a3 e4 a3 c4 e4", gate=0.45, bar=1, repeat=False), 136, synth("arp"))
preset("Hypnotic Five-Step Arp", A, "16th Sequences", "Techno/Hypnotic", 3, ["Dark", "Dense", "Straight", "Clean"],
       "Five notes cycling over 16ths, coming round against the bar",
       Part(4).line(" ".join(["a3", "c4", "e4", "a3", "d4"][i % 5] for i in range(64)), gate=0.4), 130, synth("pluck"))
preset("Acid Arp", A, "16th Sequences", "Techno/Acid", 4, ["Dark", "Dense", "Straight", "Distorted"],
       "An arp with 303 accents and slides", Part(1).line("a2! a3 c3~ e3 a2 . a3! g3~ e3 a2 c3! . a3 e3~ g3 a2", gate=0.5), 134, synth("acid"))
preset("EBM Sequence Arp", A, "16th Sequences", "Industrial/EBM", 4, ["Dark", "Dense", "Straight", "Clean"],
       "16ths on A with octave jumps, F in bar 3 and G in bar 4",
       bars_of("a2 a3 a2 a2 a3 a2 a2 a3 a2 a2 a3 a2 a2 a3 a2 a3", "a2 a3 a2 a2 a3 a2 a2 a3 a2 a2 a3 a2 a2 a3 a2 a3",
               "f2 f3 f2 f2 f3 f2 f2 f3 f2 f2 f3 f2 f2 f3 f2 f3", "g2 g3 g2 g2 g3 g2 g2 g3 g2 g2 g3 g2 g2 g3 g2 g3"), 122, synth("ebm"))
preset("Techno Body Arp", A, "16th Sequences", "Industrial/Techno-body", 4, ["Dark", "Dense", "Straight", "Distorted"],
       "A short repeating arp with rests on the kicks", Part(1).line(". a3 c4 e4 . a3 c4 g4 . a3 c4 e4 . a3 e4 a4", gate=0.4), 132, synth("arp"))
preset("Melodic Rolling Arp", A, "16th Sequences", "Techno/Melodic", 3, ["Bright", "Dense", "Straight", "Clean"],
       "16ths through A minor, F, C and G with the octave on top", arp([Am, F, C, G], "0 1 2 1 0+ 1 2 1 0 1 2 1 0+ 2 1 2"), 124, synth("arp"))

preset("Dub Pluck Echoes", A, "Plucks", "Techno/Minimal", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "A pluck repeated quieter like an echo", Part(1).line("e4! . . e4 . . e4_ . . . . . . . . .", gate=0.4), 124, synth("pluck"))
preset("Plucked Minor Riff", A, "Plucks", "Techno/Melodic", 3, ["Bright", "Sparse", "Straight", "Clean"],
       "A two-bar plucked riff in A minor",
       bars_of("a4 . e4 . c5 . a4 . e4 . . c5 . b4 a4 .", "a4 . e4 . c5 . d5 . e5 . . d5 . c5 b4 ."), 124, synth("pluck"))
preset("Pluck Call", A, "Plucks", "Techno/Driving", 4, ["Dark", "Sparse", "Straight", "Clean"],
       "A plucked call on the offbeats", Part(1).line(". . a4 . . . a4 . . . c5 . . . a4 e4", gate=0.4), 134, synth("pluck"))
preset("Synth-EBM Pluck", A, "Plucks", "Industrial/Synth-EBM", 3, ["Dark", "Sparse", "Straight", "Clean"],
       "8th plucks on the chord tones of A minor and F", arp([Am, F], "0 . 2 . 1 . 0+ . 0 . 2 . 1 . 2 ."), 126, synth("pluck"))

GATE = "0 0 . 0 0 . 0 0 0 . 0 0 . 0 0 ."
preset("Gated Sixteenths", A, "Gated", "Industrial/Futurepop", 4, ["Bright", "Dense", "Straight", "Clean"],
       "A minor chord tones gated in 16ths", arp([["a4"], ["a4"], ["f4"], ["g4"]], GATE, gate=0.4), 132, synth("saw"))
preset("Trance-Gate Minor", A, "Gated", "Industrial/Futurepop", 4, ["Bright", "Dense", "Straight", "Clean"],
       "A trance gate on A, C and E in turn", arp([Am], "0 . 0 1 . 1 2 . 2 0 . 0 1 . 2 ."), 134, synth("saw"))
preset("Hard Gate Arp", A, "Gated", "Techno/Hard", 5, ["Dark", "Dense", "Straight", "Distorted"],
       "A hard 16th gate on the root with octave accents", Part(1).line("a3 a3 . a4! a3 . a3 a4! . a3 a3 . a4! a3 . a3", gate=0.35), 150, synth("arp"))
preset("Aggro Saw Gate", A, "Gated", "Industrial/Aggrotech", 5, ["Dark", "Dense", "Straight", "Distorted"],
       "Supersaw 16ths gated round the root and fifth", Part(1).line("a3 a3 . e4 a3 . a3 e4 . a3 c4 . a3 e4 . g4", gate=0.4), 140, synth("saw"))

# ---------------------------------------------------------------------------------------------
# Melody (20).

M = "Melody"
preset("Melodic One-Bar Lead", M, "Leads", "Techno/Melodic", 3, ["Bright", "Sparse", "Straight", "Clean"],
       "A one-bar lead stated four times, the last time rising", bars_of(*(["e5 - . c5 . a4 . . c5 - . d5 . e5 . ."] * 3 + ["e5 - . c5 . a4 . . c5 - . d5 . g5 - -"])), 124, synth("lead"))
preset("Square Lead Phrase", M, "Leads", "Industrial/Synth-EBM", 3, ["Dark", "Sparse", "Straight", "Clean"],
       "A two-bar square lead in A minor", bars_of("a4 - - . c5 - . e5 - - - . d5 - c5 .", "b4 - - . a4 - . g4 - - - . a4 - - ."), 126, synth("lead"))
preset("Darkwave Lead", M, "Leads", "Industrial/Darkwave", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "A slow lead over A minor, F, G and E minor",
       bars_of("e5 - - - - - c5 - d5 - - - e5 - - -", "c5 - - - - - a4 - c5 - - - a4 - - -", "b4 - - - - - g4 - b4 - - - d5 - - -", "b4 - - - - - - - - - - - . . . ."), 110, synth("lead"))
preset("Supersaw Riff", M, "Leads", "Industrial/Aggrotech", 5, ["Dark", "Dense", "Straight", "Distorted"],
       "A 16th supersaw riff on the root, fifth and flat seventh", Part(1).line("a4 a4 e5 a4 g5 a4 e5 a4 a4 a4 e5 a4 c5 d5 e5 g5", gate=0.5), 140, synth("saw"))
preset("Peak Lead Motif", M, "Leads", "Techno/Peak-time", 4, ["Dark", "Sparse", "Straight", "Distorted"],
       "A three-note motif pushing up a step in bar 2", bars_of("a4 . . a4 . . c5 . . . . . . . . .", "a4 . . a4 . . d5 . . c5 . . . . . ."), 136, synth("lead"))
preset("Phrygian Lead", M, "Leads", "Industrial/Electro-industrial", 3, ["Dark", "Sparse", "Straight", "Distorted"],
       "A lead falling from B flat to the root", bars_of("bb4 - - . a4 - . . e5 - - - d5 . c5 .", "bb4 - - - a4 - - - . . . . . . . ."), 118, synth("lead"), tags=["Phrygian"])

preset("Call and Answer Hook", M, "Hooks", "Techno/Driving", 4, ["Dark", "Sparse", "Straight", "Clean"],
       "A two-note call in bar 1, a falling answer in bar 2", bars_of("a4 . a4 . . . c5 - . . . . . . . .", "e5 . d5 . c5 . a4 - . . . . . . . ."), 134, synth("lead"))
preset("Alarm Line", M, "Hooks", "Industrial/EBM", 4, ["Dark", "Dense", "Straight", "Distorted"],
       "A and B flat a semitone apart, alternating in 8ths like an alarm", Part(1).line("a4 . bb4 . a4 . bb4 . a4 . bb4 . a4 . bb4 .", gate=0.9), 122, synth("lead"), tags=["Phrygian"])
preset("Siren Tritone", M, "Hooks", "Industrial/EBM", 4, ["Dark", "Sparse", "Straight", "Distorted"],
       "A and E flat a tritone apart, a beat each", Part(1).line("a4 - - - eb5 - - - a4 - - - eb5 - - -"), 122, synth("lead"), tags=["Chromatic"])
preset("Short Minor Motif", M, "Hooks", "Techno/Dark", 3, ["Dark", "Sparse", "Straight", "Clean"],
       "Three notes, then space", Part(2).line("e4 . c4 . a3 - - - . . . . . . . ."), 132, synth("pluck"))
preset("Hypnotic Two-Note Hook", M, "Hooks", "Techno/Hypnotic", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "Two notes trading places over four bars",
       bars_of("a4 . . e4 . . . . . . . . . . . .", ". . . a4 . . e4 . . . . . . . . .", "a4 . . e4 . . . . . . . . . . . .", ". . . . . . a4 . . e4 . . . . . ."), 130, synth("bell"))
preset("Futurepop Chorus Hook", M, "Hooks", "Industrial/Futurepop", 4, ["Bright", "Sparse", "Straight", "Clean"],
       "A two-bar hook over A minor and F, answered over C and G",
       bars_of("e5 - . e5 . d5 c5 - . c5 . d5 e5 - - -", "c5 - . c5 . b4 a4 - . . . . . . . .",
               "e5 - . e5 . d5 c5 - . c5 . d5 e5 - g5 -", "d5 - - - - - . . . . . . . . . ."), 132, synth("lead"))
preset("Industrial Rock Riff", M, "Hooks", "Industrial/Industrial rock", 4, ["Dark", "Dense", "Straight", "Distorted"],
       "An 8th riff with the flat second and flat fifth", Part(1).line("a3 . a3 . bb3 . a3 . eb4 . d4 . c4 . a3 .", gate=0.8), 120, synth("saw"), tags=["Phrygian", "Chromatic"])

preset("Low Counter-line", M, "Counter-lines", "Techno/Melodic", 2, ["Bright", "Sparse", "Straight", "Clean"],
       "A slow line under a lead: C, B, A, G", Part(4).line("c4 - - - - - - - b3 - - - - - - -", bar=0, repeat=False).line("a3 - - - - - - - g3 - - - - - - -", bar=1, repeat=False)
       .line("c4 - - - - - - - d4 - - - - - - -", bar=2, repeat=False).line("e4 - - - - - - - - - - - - - - -", bar=3, repeat=False), 124, synth("lead"))
preset("Answering Line", M, "Counter-lines", "Industrial/Synth-EBM", 3, ["Dark", "Sparse", "Straight", "Clean"],
       "Silent in bar 1, answering in bar 2", bars_of(". . . . . . . . . . . . . . . .", ". . e4 . g4 . a4 - . . c5 . a4 - . ."), 126, synth("lead"))
preset("Darkwave Counter-melody", M, "Counter-lines", "Industrial/Darkwave", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "Half notes moving against the chords", Part(4).line("a4 - - - - - - - g4 - - - - - - -", bar=0, repeat=False).line("f4 - - - - - - - e4 - - - - - - -", bar=1, repeat=False)
       .line("d4 - - - - - - - e4 - - - - - - -", bar=2, repeat=False).line("e4 - - - - - - - - - - - - - - -", bar=3, repeat=False), 110, synth("lead"))

preset("Minor Bell Motif", M, "Bells & Mallets", "Techno/Melodic", 2, ["Bright", "Sparse", "Straight", "Clean"],
       "A four-note bell motif", Part(2).line("a5 . . e5 . . c5 . . . . . b4 . . .", gate=1.0), 124, synth("bell"))
preset("Hypnotic Bell Fives", M, "Bells & Mallets", "Techno/Hypnotic", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "A bell every five 16ths, cycling over four bars", Part(4).line(" ".join(("e5" if i % 5 == 0 else ".") for i in range(64)), gate=1.0), 130, synth("bell"))
preset("Dark Bell Toll", M, "Bells & Mallets", "Techno/Dark", 1, ["Dark", "Sparse", "Straight", "Clean"],
       "A low bell once a bar, falling to F in bar 4",
       bars_of("a4 - - - . . . . . . . . . . . .", "a4 - - - . . . . . . . . . . . .", "a4 - - - . . . . . . . . . . . .", "f4 - - - . . . . . . . . . . . ."), 130, synth("bell"))
preset("Music Box Minor", M, "Bells & Mallets", "Industrial/Darkwave", 2, ["Bright", "Sparse", "Straight", "Clean"],
       "A small music-box tune in A minor", bars_of("a5 . c6 . e6 . c6 . b5 . a5 . g5 . . .", "a5 . c6 . e6 . d6 . c6 . b5 . a5 . . ."), 108, synth("bell"))

# ---------------------------------------------------------------------------------------------
# SFX (20).

X = "SFX"
part = Part(4)
for step in range(64):
    part.add(pitch("a3") + step // 4, step * S, S // 2, 50 + step)
preset("Note Ramp Riser", X, "Risers", "Techno/Peak-time", 5, ["Bright", "Dense", "Straight", "Distorted"],
       "16ths climbing a semitone a beat for four bars, louder and brighter as they go",
       part.lane(74, [(0, 0.2), (4 * BAR - 1, 1.0)]), 134, synth("sweep"), tags=["Chromatic"])
preset("Pitch-bend Riser", X, "Risers", "Techno/Driving", 4, ["Bright", "Sparse", "Straight", "Distorted"],
       "One held note bending up two semitones over four bars as the filter opens",
       Part(4).add(pitch("a3"), 0, 4 * BAR - 40, 90).lane(128, [(0, 0.5), (4 * BAR - 1, 1.0)]).lane(74, [(0, 0.2), (4 * BAR - 1, 1.0)]), 134, synth("sweep"))
part = Part(8)
for step in range(0, 64, 2):
    part.add(SNARE, step * S, S // 2, 40 + step)
for step in range(64, 128):
    part.add(SNARE, step * S, S // 2, min(127, 72 + (step - 64)))
preset("Snare Build", X, "Risers", "Techno/Driving", 5, ["Bright", "Dense", "Straight", "Clean"],
       "Eight bars of snare: 8ths for four bars, then 16ths, ramping from 40 to 127", part, 134, drums())
part = Part(4)
for step in range(64):
    if step >= 32 or step % 2 == 0:
        part.add(CLAP, step * S, S // 2, min(127, 40 + int(step * 87 / 63)))
preset("Clap Roll Build", X, "Risers", "Industrial/EBM", 4, ["Bright", "Dense", "Straight", "Clean"],
       "Claps in 8ths, then 16ths, rising from 40 to 127 over four bars", part, 124, drums("machine"))

preset("Pitch-bend Dive", X, "Downlifters", "Industrial/Electro-industrial", 3, ["Dark", "Sparse", "Straight", "Distorted"],
       "A held note diving two semitones over two bars",
       Part(2).add(pitch("a3"), 0, 2 * BAR - 40, 100).lane(128, [(0, 0.5), (2 * BAR - 1, 0.0)]), 118, synth("sweep"))
part = Part(2)
for step in range(32):
    part.add(pitch("a4") - step // 2, step * S, S // 2, max(30, 110 - step * 2))
preset("Falling Note Ramp", X, "Downlifters", "Techno/Peak-time", 3, ["Dark", "Dense", "Straight", "Clean"],
       "16ths falling a semitone every 8th, quieter as they go", part, 134, synth("sweep"), tags=["Chromatic"])
preset("Filter Close Down", X, "Downlifters", "Techno/Melodic", 2, ["Dark", "Sparse", "Straight", "Clean"],
       "An A minor chord closing its filter over four bars", Part(4).chords([Am] * 4, velocity=84).lane(74, [(0, 1.0), (4 * BAR - 1, 0.05)]), 124, synth("sweep"))

preset("Crash and Kick Impact", X, "Impacts", "Techno/Peak-time", 5, ["Bright", "Sparse", "Straight", "Clean"],
       "Kick and crash together on the one", Part(1).add(KICK, 0, S // 2, 127).add(CRASH, 0, S // 2, 120), 134, drums("rumble"))
preset("Low Boom", X, "Impacts", "Techno/Dark", 4, ["Dark", "Sparse", "Straight", "Distorted"],
       "A low A struck and bent down", Part(1).add(pitch("a1"), 0, BAR - 40, 127).lane(128, [(0, 0.5), (BAR - 1, 0.0)]), 130, synth("sub"))
preset("Tom Impact", X, "Impacts", "Techno/Industrial", 4, ["Dark", "Sparse", "Straight", "Clean"],
       "Low tom, kick and crash together", Part(1).add(LT, 0, S // 2, 127).add(KICK, 0, S // 2, 120).add(CRASH, 0, S // 2, 110), 128, drums("rumble"))
preset("Sub Drop Impact", X, "Impacts", "Techno/Hard", 5, ["Dark", "Sparse", "Straight", "Distorted"],
       "A sub hit at the drop, bending down over a bar", Part(1).add(pitch("a1"), 0, BAR - 40, 127).lane(128, [(0, 1.0), (BAR - 1, 0.0)]), 150, synth("sub"))

preset("Filter Sweep Up", X, "Sweeps", "Techno/Driving", 4, ["Bright", "Dense", "Straight", "Clean"],
       "An A minor chord opening its filter over four bars", Part(4).chords([Am] * 4, velocity=80).lane(74, [(0, 0.0), (4 * BAR - 1, 1.0)]), 134, synth("sweep"))
preset("Filter Sweep Down", X, "Sweeps", "Techno/Dark", 3, ["Dark", "Dense", "Straight", "Clean"],
       "A fifth on A closing its filter over two bars", Part(2).chords([["a2", "e3"]] * 2, velocity=86).lane(74, [(0, 1.0), (2 * BAR - 1, 0.0)]), 130, synth("sweep"))
preset("Resonant Sweep", X, "Sweeps", "Techno/Acid", 3, ["Dark", "Sparse", "Straight", "Distorted"],
       "A low A with its resonant filter sweeping up and down over four bars",
       Part(4).add(pitch("a1"), 0, 4 * BAR - 40, 100).lane(74, [(0, 0.1), (2 * BAR, 0.9), (4 * BAR - 1, 0.1)]), 132, synth("acid"))

part = Part(2)
for step in range(32):
    if step >= 8 or step % 2 == 0:
        part.add([HT, MT, LT][(step // 4) % 3], step * S, S // 2, min(127, 50 + step * 2))
preset("Tom Build Transition", X, "Transitions", "Techno/Percussive", 4, ["Dark", "Dense", "Straight", "Clean"],
       "Toms in 8ths, then 16ths, louder through two bars", part, 138, drums())
preset("Reverse Swell Transition", X, "Transitions", "Techno/Melodic", 3, ["Bright", "Sparse", "Straight", "Clean"],
       "An A minor chord swelling up from silence into the next bar",
       Part(1).chords([Am], velocity=90).lane(11, [(0, 0.0), (BAR - 1, 1.0)]), 124, synth("pad"))
preset("Stop Gap", X, "Transitions", "Industrial/EBM", 3, ["Dark", "Sparse", "Straight", "Clean"],
       "A crash and kick on the one, then a bar of silence before the drop", Part(2).add(KICK, 0, S // 2, 120).add(CRASH, 0, S // 2, 116), 122, drums())

preset("Minor Chord Hit", X, "Hits", "Techno/Peak-time", 4, ["Dark", "Sparse", "Straight", "Distorted"],
       "One A minor stab with the octave", Part(1).chords([["a3", "c4", "e4", "a4"]], rhythm="x...............", length_steps=2, velocity=104), 134, synth("stab"))
preset("Industrial Metal Hit", X, "Hits", "Techno/Industrial", 4, ["Dark", "Sparse", "Straight", "Distorted"],
       "Snare, metal and crash struck together", Part(1).add(SNARE, 0, S // 2, 124).add(COWBELL, 0, S // 2, 110).add(CRASH, 0, S // 2, 100), 128, drums("hard"))
preset("Siren Hit", X, "Hits", "Industrial/EBM", 4, ["Dark", "Sparse", "Straight", "Distorted"],
       "A and E flat struck together and bent up", Part(1).chords([["a4", "eb5"]], rhythm="x...............", length_steps=8, velocity=116)
       .lane(128, [(0, 0.5), (2 * TPB, 1.0)]), 122, synth("lead"), tags=["Chromatic"])

# ---------------------------------------------------------------------------------------------
# Starter kits: a groove, a bass, a stab or chord, a lead or arp, a fill and a build for each main style.

KITS = [
    ("Dark Techno Kit", 132, "Techno/Dark", ["Rumble Roller", "Peak Rumble Bass", "One-Note Minor Stab", "Short Minor Motif", "Snare Roll Fill", "Snare Build"]),
    ("Peak-time Techno Kit", 134, "Techno/Peak-time", ["Peak Hour Driver", "Rolling Kick Gap Bass", "Peak Stab Call", "Broken Root Fifth Octave", "Snare Roll Fill", "Note Ramp Riser"]),
    ("Percussive Techno Kit", 138, "Techno/Percussive", ["Tool Loop Percussion", "Percussive Tool Bass", "Offbeat Minor Comp", "Pluck Call", "Tom Run Down", "Tom Build Transition"]),
    ("Hypnotic Techno Kit", 130, "Techno/Hypnotic", ["Three Against Four", "Hypnotic Two-Note Bass", "Hypnotic One Chord", "Hypnotic Five-Step Arp", "Tom Run Down", "Octave Swell"]),
    ("Industrial Techno Kit", 128, "Techno/Industrial", ["Industrial Pound", "Distorted Offbeat Bass", "Industrial Metal Stab", "Techno Body Arp", "Tom Run Down", "Snare Build"]),
    ("Acid Techno Kit", 134, "Techno/Acid", ["Warehouse Stomp", "Acid Root Jumper", "Rave Stab Fifths", "Acid Arp", "Snare Roll Fill", "Pitch-bend Riser"]),
    ("EBM Kit", 122, "Industrial/EBM", ["Body Music Beat", "Belgian Body Bass", "Alarm Stab Tritone", "Alarm Line", "Clap Stutter Fill", "Clap Roll Build"]),
    ("Darkwave Kit", 110, "Industrial/Darkwave", ["Darkwave Drum Machine", "Darkwave Eighth Roots", "Darkwave Chorus Chords", "Darkwave Lead", "Clap Stutter Fill", "Reverse Swell Transition"]),
]


# =============================================================================================
# The larger set: presets made from each style's conventions.
#
# The hand-written presets above are the core. These are made by rule, style by style: each style
# names its tempos, the drum patterns, bass rhythms, chord moves, arp figures, melody rhythms and
# sounds that mark it, and the words its names are made from; a seeded generator (one seed per
# style, so a rerun writes the same files) combines them into presets in every role, and three
# kits per style from those. Every one keeps the rules in the docstring at the top, and its
# description says what it actually plays.

MODES = {"minor": [9, 11, 0, 2, 4, 5, 7], "phrygian": [9, 10, 0, 2, 4, 5, 7], "harmonic": [9, 11, 0, 2, 4, 5, 8]}
MODE_TAGS = {"minor": [], "phrygian": ["Phrygian"], "harmonic": ["Harmonic minor"]}
LADDER = {m: [p for p in range(128) if p % 12 in pcs] for m, pcs in MODES.items()}

# More Basic Synth settings for the styles the set reaches into.
PATCHES.update({
    "reese":  dict(wave=2, level=-14.0, attack=2.0, decay=300.0, sustain=90.0, release=80.0, cutoff=700.0, resonance=20.0,
                   osc2Wave=2, osc2Detune=22.0, osc2Level=100.0, osc3Wave=0, osc3Octave=-1.0, osc3Level=60.0),
    "chip":   dict(wave=3, level=-18.0, attack=1.0, decay=120.0, sustain=60.0, release=40.0),
    "chipbass": dict(wave=1, level=-12.0, attack=1.0, decay=100.0, sustain=80.0, release=30.0),
    "organ":  dict(wave=0, level=-14.0, attack=4.0, decay=200.0, sustain=90.0, release=120.0,
                   osc2Wave=0, osc2Octave=1.0, osc2Level=60.0, osc3Wave=0, osc3Octave=2.0, osc3Level=30.0),
    "psy":    dict(wave=2, level=-13.0, attack=1.0, decay=90.0, sustain=0.0, release=30.0, cutoff=380.0, resonance=40.0,
                   filterEnv=65.0, filterDecay=70.0, velocityToFilter=40.0),
    "brass":  dict(wave=2, level=-18.0, attack=30.0, decay=400.0, sustain=70.0, release=250.0, cutoff=1400.0, resonance=15.0,
                   osc2Wave=2, osc2Detune=10.0, osc2Level=85.0, filterEnv=45.0, filterDecay=300.0),
    "glass":  dict(wave=1, level=-14.0, attack=1.0, decay=600.0, sustain=10.0, release=700.0,
                   osc2Wave=0, osc2Octave=1.0, osc2Detune=5.0, osc2Level=50.0),
    "warm":   dict(wave=1, level=-16.0, attack=400.0, decay=900.0, sustain=85.0, release=1200.0, cutoff=1200.0, resonance=5.0,
                   osc2Wave=2, osc2Detune=6.0, osc2Level=45.0),
    "noise":  dict(wave=2, level=-22.0, attack=20.0, decay=400.0, sustain=100.0, release=600.0, cutoff=600.0, resonance=85.0,
                   osc2Wave=3, osc2Detune=41.0, osc2Level=100.0, osc3Wave=2, osc3Detune=-37.0, osc3Level=100.0),
})

KITS_SOUND.update({
    "tight":  dict(kickDecay=60.0, snareDecay=70.0, hatDecay=50.0),
    "boom":   dict(kickTune=-5.0, kickDecay=340.0, snareDecay=140.0),
    "break":  dict(snareTune=2.0, snareDecay=120.0, hatDecay=90.0, kickDecay=90.0),
    "chip":   dict(kickTune=4.0, kickDecay=50.0, snareTune=5.0, snareDecay=50.0, hatTune=6.0, hatDecay=40.0),
})

# ---------------------------------------------------------------------------------------------
# Drum parts: a pattern for a drum and what it is, to say in a description.

KICKS = {
    "four":      ([(KICK, "X...X...X...X...")], "four to the floor"),
    "fourghost": ([(KICK, "X...X...X...X..."), (KICK, "..g...g...g...g.")], "four to the floor with ghost kicks after each"),
    "fourpush":  ([(KICK, "X...X...X...X..."), (KICK, "...........g...g")], "four to the floor with ghost kicks pushing into beats 4 and 1"),
    "rumble":    ([(KICK, "X...X...X...X..."), (KICK, ".gg..gg..gg..gg.")], "four to the floor with a rolling rumble between the kicks"),
    "skip":      ([(KICK, "X...X...X...X.x.")], "four to the floor with a skipped kick before the one"),
    "broken":    ([(KICK, "X.....x...X.....")], "a broken kick on 1, the and of 2 and 3"),
    "brokenb":   ([(KICK, "X.....x...X...x.")], "a broken kick that lands on the last 16th"),
    "electro":   ([(KICK, "X......x..X.....")], "an electro kick on 1, the a of 2 and 3"),
    "electrob":  ([(KICK, "X.....X...x..x..")], "a syncopated electro kick"),
    "break":     ([(KICK, "X.x.......x.....")], "a breakbeat kick on 1, the and of 1 and the and of 3"),
    "breakb":    ([(KICK, "X.........X..x..")], "a breakbeat kick with a push before the 4"),
    "dnb":       ([(KICK, "X.........X.....")], "the two-step kick on 1 and the and of 3"),
    "dnbroll":   ([(KICK, "X.........X...x.")], "a two-step kick with a pickup into the next bar"),
    "half":      ([(KICK, "X.......x.......")], "a half-time kick on 1 and 3"),
    "halfsparse": ([(KICK, "X...............")], "one kick a bar"),
    "ebm":       ([(KICK, "X...X...X...X...")], "a flat machine kick on every beat"),
    "march":     ([(KICK, "X...X...X.x.X...")], "a marching kick with a pickup before beat 3"),
    "rock":      ([(KICK, "X.......X.X.....")], "a rock kick on 1, 3 and the and of 3"),
    "psy":       ([(KICK, "X...X...X...X...")], "a short kick on every beat, leaving the 16ths after it to the bass"),
    "gallop":    ([(KICK, "X...X..xX...X..x")], "four to the floor with a gallop into beats 3 and 1"),
}

BACKBEATS = {
    "clap24":    ([(CLAP, "....X.......X...")], "a clap on 2 and 4"),
    "claplayer": ([(CLAP, "....X.......X..."), (SNARE, "....o.......o...")], "a clap layered with a snare on 2 and 4"),
    "snare24":   ([(SNARE, "....X.......X...")], "a snare on 2 and 4"),
    "snareghost": ([(SNARE, "....X..g....X.g.")], "a snare on 2 and 4 with ghost notes after them"),
    "gated":     ([(SNARE, "....X.......X..."), (CLAP, "....x.......x...")], "a big snare and clap together on 2 and 4"),
    "half":      ([(SNARE, "........X.......")], "a half-time snare on 3"),
    "dnb":       ([(SNARE, "....X.......X...")], "the snare on 2 and 4"),
    "dnbghost":  ([(SNARE, "....X..g.g..X..g")], "the snare on 2 and 4 with ghosted snares rolling round it"),
    "breakghost": ([(SNARE, "....X..g.g..X...")], "a snare on 2 and 4 with ghost notes in between"),
    "clap4":     ([(CLAP, "............X...")], "a clap on 4 only"),
    "rim24":     ([(RIM, "....x.......x...")], "a rimshot on 2 and 4"),
    "none":      ([], ""),
}

HATS = {
    "sixteen":   ([(CH, "wwxwwwxwwwxwwwxw")], "16th hats with the offbeat lifted"),
    "offopen":   ([(OH, "..x...x...x...x.")], "open hats on the offbeats"),
    "offclosed": ([(CH, "..x...x...x...x.")], "closed hats on the offbeats"),
    "eighths":   ([(CH, "x.o.x.o.x.o.x.o.")], "8th hats, the downbeats stronger"),
    "mixed":     ([(CH, "x.w.x.w.x.w.x.w."), (OH, "..x...x...x...x.")], "closed hats on the beat and open ones between"),
    "ride":      ([(RIDE, "x.o.x.o.x.o.x.o.")], "a ride in 8ths"),
    "rideoff":   ([(RIDE, "..x...x...x...x.")], "a ride on the offbeats"),
    "sixteenflat": ([(CH, "xxxxxxxxxxxxxxxx")], "flat machine 16th hats"),
    "psy":       ([(OH, "..x...x...x...x."), (CH, "x.w.x.w.x.w.x.w.")], "open hats on the offbeats over 8th closed hats"),
    "dnb":       ([(CH, "x.x.x.x.x.x.x.x."), (OH, "......x.......x.")], "8th hats with an open hat before 3 and 1"),
    "sparse":    ([(CH, "x.......x.......")], "a hat on 1 and 3"),
    "pedal":     ([(PH, "..x...x...x...x.")], "a pedal hat on the offbeats"),
    "shuffle":   ([(CH, "x..ox..ox..ox..o")], "hats pushing into each beat"),
    "none":      ([], ""),
}

PERCS = {
    "rim3":      ([("cycle", RIM, 3, "o", 0)], "a rimshot every three 16ths, against the beat"),
    "shaker":    ([(SHAKER, "xwowxwowxwowxwow")], "a shaker in 16ths"),
    "cowbell":   ([(COWBELL, "......x.......x.")], "a cowbell on the and of 2 and 4"),
    "toms":      ([(LT, "..x...o..x....o."), (MT, ".o...x...o..x...")], "toms in shifting 16ths"),
    "metal":     ([(COWBELL, "x..x..x..x..x...")], "metal struck in threes"),
    "tomfive":   ([("cycle", LT, 5, "o", 2)], "a low tom every five 16ths"),
    "rimsync":   ([(RIM, ".x..x..x..x..x.x")], "a syncopated rimshot"),
    "conga":     ([(HT, "...x..x....x..x."), (MT, "x.....x.x.......")], "high and mid toms playing like congas"),
    "crash":     ([(CRASH, "x...............")], "a crash on the one"),
    "none":      ([], ""),
}

TOMRUN = [(HT, "........xxx....."), (MT, "...........xxx.."), (LT, "..............xx")]


def put(part, pieces, bar=None):
    for piece in pieces:
        if piece[0] == "cycle":
            part.cycle(piece[1], piece[2], piece[3], piece[4])
        else:
            part.grid(piece[0], piece[1], bar=bar)


# ---------------------------------------------------------------------------------------------
# Harmony: chords built on the mode's degrees, so every note is in it.

def ladder_index(mode, p):
    return LADDER[mode].index(p)


def degree_pitch(mode, degree, low, high):
    """The mode's degree (0 the root, A) as a pitch from low to high."""
    pc = MODES[mode][degree % 7]
    return next(p for p in range(low, high + 1) if p % 12 == pc)


def triad(mode, degree, low=52, high=63, seventh=False, size=3):
    root = degree_pitch(mode, degree, low, high)
    i = ladder_index(mode, root)
    steps = [0, 2, 4] + ([6] if seventh else []) + ([8] if size > 4 else [])
    return [LADDER[mode][i + s] for s in steps]


LETTERS = {9: "A", 10: "B flat", 11: "B", 0: "C", 2: "D", 4: "E", 5: "F", 7: "G", 8: "G sharp"}


def chord_name(mode, degree, seventh=False):
    notes = triad(mode, degree, seventh=seventh)
    third, fifth = (notes[1] - notes[0]) % 12, (notes[2] - notes[0]) % 12
    quality = " diminished" if fifth == 6 else (" minor" if third == 3 else "")

    if seventh:
        flat_seventh = (notes[3] - notes[0]) % 12 == 10
        quality = (" minor seventh" if third == 3 else (" seventh" if flat_seventh else " major seventh")) if fifth != 6 else " half-diminished"

    return LETTERS[notes[0] % 12] + quality


def chord_names(mode, degrees, seventh=False):
    return ", ".join(chord_name(mode, d, seventh) for d in degrees)


def names(pitches):
    return [name_of(p) for p in pitches]


# ---------------------------------------------------------------------------------------------
# Styles.

class Style:
    def __init__(self, style, tempos, size, *, words, drums_sound, kicks, backbeats, hats, percs, bass, bass_patches,
                 progressions, chord_patches, arp_patches, lead_patches, pad_patches, energy, dark=True, distorted=False,
                 modes=("minor",), breaks=False, game=False, acid=False):
        self.style, self.tempos, self.size, self.words = style, tempos, size, words
        self.drums_sound, self.kicks, self.backbeats, self.hats, self.percs = drums_sound, kicks, backbeats, hats, percs
        self.bass, self.bass_patches, self.progressions = bass, bass_patches, progressions
        self.chord_patches, self.arp_patches, self.lead_patches, self.pad_patches = chord_patches, arp_patches, lead_patches, pad_patches
        self.energy, self.dark, self.distorted, self.modes = energy, dark, distorted, modes
        self.breaks, self.game, self.acid = breaks, game, acid

    @property
    def label(self):
        return self.style.split("/")[1]

    @property
    def kit_label(self):
        genre, style = self.style.split("/")
        if genre == "Techno":
            return style + " Techno"
        if genre == "Game":
            return style + " Game"
        return style[0].upper() + style[1:]


# Bass rhythms: '1' the chord's root, '8' its octave, '5' its fifth, '3' its third, '7' the
# seventh below the octave, 'b' the step under the root, '.' a rest, '-' holding the note before;
# '!' accents and '~' slides as in Part.line.
BASS = {
    "roll":      (". 1 1 1 . 1 1 1 . 1 1 1 . 1 1 1", "kick-bass-bass-bass: three 16ths in each gap between the kicks"),
    "rollacc":   (". 1 1 1! . 1 1 1 . 1 1 1! . 1 1 8", "rolling 16ths in the kick's gaps, accented, jumping to the octave at the end"),
    "offbeat":   (". . 1 . . . 1 . . . 1 . . . 1 .", "an 8th on every offbeat"),
    "offsixteen": (". . 1 1 . . 1 1 . . 1 1 . . 1 8", "two 16ths on each offbeat, the last up an octave"),
    "gallop":    ("1 . 1 1 1 . 1 1 1 . 1 1 8 . 8 8", "the gallop: an 8th and two 16ths on every beat, the last beat up an octave"),
    "ebm":       (". 1 8 1 . 1 8 1 . 1 8 1 . 1 8 1", "16ths in the kick's gaps with the jump to the octave"),
    "ebmrest":   (". 1 8 . . 1 8 1 . 1 8 . . 1 1 8", "16ths in the kick's gaps with rests to breathe"),
    "octave8":   ("1 . 8 . 1 . 8 . 1 . 8 . 1 . 8 .", "8ths leaping between the root and the octave"),
    "eighths":   ("1 . 1 . 1 . 1 . 1 . 1 . 1 . 1 .", "straight 8ths on the root"),
    "sixteens":  ("1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1", "driving 16ths on the root"),
    "sixteensoct": ("1 1 8 1 1 1 8 1 1 1 8 1 1 1 8 1", "driving 16ths with the octave on each offbeat"),
    "electro":   ("1 . . 1 . . 1 . . . 1 . 5 . 1 .", "a syncopated electro line round the root and fifth"),
    "electrob":  ("1 - . 8 . 1 . . 1 - . 5 . . 3 .", "an electro line with held roots, the fifth and the third"),
    "reese":     ("1 - - - - - - - . . 1 - - - - -", "long held roots, out of the drums' way"),
    "reesemove": ("1 - - - - - . 1 - - . . 5 - - -", "held notes moving from the root to the fifth"),
    "sub":       ("1 - - - - - - - - - - - - - - -", "a held sub on the root"),
    "subhalf":   ("1 - - - - - - - 1 - - - - - - -", "the sub in half notes"),
    "deep":      ("1 . . 1 . . 8 . . 1 . . 5 . . .", "a deep line in dotted 8ths round the root, octave and fifth"),
    "deepwalk":  ("1 . . . 1 . 3 . . . 5 . . 3 . .", "a walking line up through the chord"),
    "psy":       (". 1 1 1 . 1 1 1 . 1 1 1 . 1 1 1", "the rolling triplet-feel 16ths after every kick"),
    "synthwave": ("1 1 8 1 1 1 8 1 1 1 8 1 1 1 8 1", "pumping 16ths with the octave"),
    "darkwave":  ("1 . 1 . 1 . 1 . 1 . 1 . 8 . 1 .", "8th roots with an octave before the last"),
    "chip":      ("1 . 8 . 5 . 8 . 1 . 8 . 5 . 8 .", "root, octave and fifth bouncing in 8ths"),
    "halftime":  ("1 - - . . . . . 1 - . . 5 - . .", "a half-time line, held roots and a fifth"),
    "rock":      ("1 1 1 1 1 1 1 1 b b b b 1 1 1 1", "16ths on the root dipping a step under it"),
    "march":     ("1 . 1 1 1 . 1 1 1 . 1 1 1 . 8 .", "a marching 16th line"),
    "drone":     ("1 - - - - - - - - - - - - - - -", "one held note a bar"),
    "pulse":     ("1 . . . 1 . . . 1 . . . 1 . . .", "a quarter-note pulse on the root"),
}

PROGRESSIONS = {
    "i-VI-III-VII": ([0, 5, 2, 6], "minor"),
    "i-iv-VI-VII":  ([0, 3, 5, 6], "minor"),
    "i-VI-VII-i":   ([0, 5, 6, 0], "minor"),
    "i-VII-VI-VII": ([0, 6, 5, 6], "minor"),
    "i-i-VI-VII":   ([0, 0, 5, 6], "minor"),
    "i-III-VII-iv": ([0, 2, 6, 3], "minor"),
    "i-v-VI-iv":    ([0, 4, 5, 3], "minor"),
    "i-VII-i-VI":   ([0, 6, 0, 5], "minor"),
    "i-i-i-VI":     ([0, 0, 0, 5], "minor"),
    "i-i-iv-i":     ([0, 0, 3, 0], "minor"),
    "i-II-i-VII":   ([0, 1, 0, 6], "phrygian"),
    "i-II-VII-i":   ([0, 1, 6, 0], "phrygian"),
    "i-iv-V-i":     ([0, 3, 4, 0], "harmonic"),
    "i-VI-iv-V":    ([0, 5, 3, 4], "harmonic"),
    "i-i-VI-V":     ([0, 0, 5, 4], "harmonic"),
    "i-i-i-i":      ([0, 0, 0, 0], "minor"),
}

ARP_FIGURES = [
    ("0 1 2 0+ 2 1 0 1 2 0+ 2 1 0 1 2 1", "up and down in 16ths", "Up & Down"),
    ("0 1 2 1 0+ 1 2 1 0 1 2 1 0+ 2 1 2", "rolling 16ths with the octave on top", "Up & Down"),
    ("0 2 1 0+ 0 2 1 0+ 0 2 1 0+ 0 2 1 2", "a turning figure, root, fifth, third and octave", "16th Sequences"),
    ("0 0 1 0 2 0 1 0 0 0 1 0 2 0 0+ 0", "a pedal on the root with the chord's notes between", "16th Sequences"),
    ("0 1 2 0+ 0 1 2 0+ 0 1 2 0+ 0 1 2 0+", "straight up the chord in 16ths, again and again", "16th Sequences"),
    ("0 . 2 . 1 . 0+ . 0 . 2 . 1 . 2 .", "8th plucks on the chord's notes", "Plucks"),
    ("0+ . . 2 . . 0+ . . 1 . . 2 . . .", "dotted-8th plucks falling from the octave", "Plucks"),
    ("0 . 1 . 2 . 1 . 0 . 1 . 2 . 0+ .", "8ths up and down", "Plucks"),
    ("0 0 . 0 0 . 0 0 0 . 0 0 . 0 0 .", "the root gated in 16ths", "Gated"),
    ("0 . 0 1 . 1 2 . 2 0 . 0 1 . 2 .", "a trance gate through the chord", "Gated"),
    ("0 0 0 . 1 1 1 . 2 2 2 . 1 1 1 .", "each note of the chord three times over", "Gated"),
]

MELODY_RHYTHMS = [
    ("x - . x . . x . x - . x . . x .", "syncopated"),
    ("x - - . x - . . x - - - x . x .", "held"),
    ("x . x . x . . . x . x . x - - -", "call-and-answer"),
    ("x - - - - - x - x - - - - - . .", "long"),
    ("x . . x . . x . . . x . x . . .", "dotted"),
    (". . x . . . x . . . x . x - . .", "offbeat"),
    ("x x x . x x x . x x . x x - - .", "busy"),
    ("x - - - . . . . x . x - - - . .", "sparse"),
]


def W(*w):
    return list(w)


DRUM_NOUNS = {
    "Full Grooves": W("Groove", "Beat", "Stomp", "Pound", "Drive", "Machine", "Loop", "Pulse", "Engine", "Thump"),
    "Kick": W("Kick Line", "Kick Pattern", "Kick Push", "Low End Kick"),
    "Snare & Clap": W("Backbeat", "Backbeat Loop", "Crack"),
    "Hats & Tops": W("Hats", "Tops", "Hat Line", "Top Loop"),
    "Percussion": W("Percussion", "Perc Loop", "Perc Groove"),
    "snare fill": W("Snare Fill", "Snare Roll", "Roll Fill"),
    "tom fill": W("Tom Fill", "Tom Run", "Tom Fall"),
    "clap fill": W("Clap Fill", "Clap Stutter", "Clap Roll"),
    "Breaks": W("Break", "Breakbeat", "Chopped Break", "Break Loop"),
}
ROLE_NOUNS = {
    "Sub": W("Sub", "Sub Pulse", "Sub Line"),
    "Synth Bass": W("Bass", "Bassline", "Roller", "Throb", "Drive Bass", "Pressure"),
    "Arp Bass": W("Bass Sequence", "Bass Arp", "Bass Run"),
    "Picked & Played": W("Played Bass", "Bass Walk", "Bass Riff"),
    "Progressions": W("Progression", "Chords", "Changes", "Turn"),
    "Keys & Comping": W("Comp", "Keys", "Chord Pulse"),
    "Stabs": W("Stab", "Stabs", "Hit Chords"),
    "Strums": W("Strum", "Strummed Chords"),
    "Sustained": W("Pad", "Wash", "Bed"),
    "Evolving": W("Evolving Pad", "Shifting Pad", "Bloom"),
    "Swells": W("Swell", "Rising Pad"),
    "Drones": W("Drone", "Hum", "Low Drone"),
    "Up & Down": W("Arp", "Up-Down Arp", "Climb"),
    "16th Sequences": W("Sequence", "Sixteenths", "Pattern"),
    "Plucks": W("Plucks", "Pluck Line", "Pluck"),
    "Gated": W("Gate", "Gated Chord", "Chopper"),
    "Leads": W("Lead", "Theme", "Line"),
    "Hooks": W("Hook", "Motif", "Riff"),
    "Counter-lines": W("Counter-line", "Answer", "Under-line"),
    "Bells & Mallets": W("Bells", "Chime", "Mallets"),
    "Risers": W("Riser", "Build", "Lift"),
    "Downlifters": W("Downlifter", "Fall", "Drop-off"),
    "Impacts": W("Impact", "Boom", "Slam"),
    "Sweeps": W("Sweep", "Filter Sweep"),
    "Transitions": W("Transition", "Bridge"),
    "Hits": W("Hit", "Strike"),
}

STYLES = [
    # Techno: the core, most presets.
    Style("Techno/Dark", [128, 130, 132], 32, words=W("Basement", "Black", "Concrete", "Midnight", "Void", "Tunnel", "Smoke", "Low-Light", "Shadow", "Cellar", "Obsidian", "Murk", "Eclipse", "Undertow"),
          drums_sound=["rumble", "standard"], kicks=["rumble", "fourghost", "four", "skip"], backbeats=["clap24", "claplayer", "rim24", "clap4"],
          hats=["offopen", "ride", "sixteen", "rideoff"], percs=["rim3", "tomfive", "metal", "none"], bass=["rollacc", "offbeat", "roll", "offsixteen", "drone"],
          bass_patches=["roll", "dark", "sub"], progressions=["i-i-i-VI", "i-II-i-VII", "i-VII-i-VI", "i-i-i-i"], chord_patches=["stab", "chord"],
          arp_patches=["pluck", "arp"], lead_patches=["lead", "bell"], pad_patches=["drone", "pad"], energy=3, modes=("minor", "phrygian")),
    Style("Techno/Driving", [130, 132, 134], 32, words=W("Autobahn", "Steel", "Motor", "Night Drive", "Piston", "Highway", "Momentum", "Gearbox", "Overpass", "Cruise", "Throttle", "Signal"),
          drums_sound=["standard", "hard"], kicks=["four", "fourpush", "fourghost", "gallop"], backbeats=["claplayer", "clap24", "snare24"],
          hats=["sixteen", "offopen", "mixed"], percs=["shaker", "rim3", "cowbell", "none"], bass=["roll", "rollacc", "offsixteen", "sixteensoct"],
          bass_patches=["roll", "ebm"], progressions=["i-VI-VII-i", "i-i-VI-VII", "i-VII-VI-VII"], chord_patches=["stab", "chord"],
          arp_patches=["arp", "pluck"], lead_patches=["lead", "saw"], pad_patches=["pad", "sweep"], energy=4),
    Style("Techno/Peak-time", [134, 136, 138], 32, words=W("Mainroom", "Lasers", "Strobe", "Hands Up", "Fog Machine", "Peak", "Crowd", "Floodlight", "Afterhours", "Prime Time", "Rave Hall", "Euphoria"),
          drums_sound=["hard", "standard"], kicks=["four", "fourpush", "skip", "rumble"], backbeats=["claplayer", "clap24", "gated"],
          hats=["sixteen", "offopen", "mixed", "ride"], percs=["shaker", "cowbell", "crash", "none"], bass=["rollacc", "roll", "sixteensoct", "offsixteen"],
          bass_patches=["roll", "acid", "ebm"], progressions=["i-VI-III-VII", "i-VI-VII-i", "i-i-VI-VII"], chord_patches=["stab", "saw"],
          arp_patches=["arp", "saw"], lead_patches=["lead", "saw"], pad_patches=["sweep", "pad"], energy=5),
    Style("Techno/Industrial", [126, 128, 130, 132], 32, words=W("Foundry", "Rivet", "Furnace", "Scrapyard", "Slag", "Girder", "Factory", "Grinder", "Anvil", "Corrosion", "Smokestack", "Pressworks"),
          drums_sound=["hard", "machine", "rumble"], kicks=["four", "rumble", "march", "fourghost"], backbeats=["gated", "snare24", "clap24"],
          hats=["sixteenflat", "offopen", "ride", "eighths"], percs=["metal", "toms", "tomfive", "rimsync"], bass=["offbeat", "rollacc", "sixteens", "march"],
          bass_patches=["dark", "roll", "reese"], progressions=["i-II-i-VII", "i-i-i-VI", "i-II-VII-i"], chord_patches=["stab", "brass"],
          arp_patches=["arp"], lead_patches=["saw", "lead"], pad_patches=["noise", "drone"], energy=4, distorted=True, modes=("phrygian", "minor")),
    Style("Techno/Hard", [145, 150, 155], 32, words=W("Hammer", "Hardline", "Bulldozer", "Overdrive", "Iron Fist", "Jackhammer", "Riot", "Bunker", "Breaker", "Warhead", "Full Force", "Blast Wall"),
          drums_sound=["hard"], kicks=["four", "skip", "gallop", "rumble"], backbeats=["claplayer", "gated", "clap24"],
          hats=["offopen", "sixteenflat", "mixed"], percs=["crash", "metal", "none"], bass=["rollacc", "offsixteen", "sixteens"],
          bass_patches=["acid", "roll", "reese"], progressions=["i-i-i-VI", "i-VII-VI-VII", "i-II-i-VII"], chord_patches=["saw", "stab"],
          arp_patches=["arp", "saw"], lead_patches=["saw"], pad_patches=["sweep", "noise"], energy=5, distorted=True, modes=("minor", "phrygian"), acid=True),
    Style("Techno/Hypnotic", [126, 128, 130], 22, words=W("Trance State", "Spiral", "Orbit", "Mantra", "Tide", "Fog", "Loopwise", "Inner Space", "Drift", "Pendulum"),
          drums_sound=["rumble", "standard"], kicks=["rumble", "four"], backbeats=["rim24", "clap4", "clap24"],
          hats=["rideoff", "offclosed", "shuffle"], percs=["rim3", "tomfive", "rimsync"], bass=["offbeat", "drone", "roll"],
          bass_patches=["dark", "sub"], progressions=["i-i-i-i", "i-i-i-VI"], chord_patches=["chord", "glass"],
          arp_patches=["pluck", "glass"], lead_patches=["bell", "glass"], pad_patches=["drone", "pad"], energy=3),
    Style("Techno/Acid", [130, 134, 138], 22, words=W("Squelch", "Acid Rain", "Three-Oh", "Resonance", "Rubber", "Acidic", "Slide", "Bleep", "Corrosive", "Fizz"),
          drums_sound=["standard", "hard"], kicks=["four", "fourghost"], backbeats=["clap24", "claplayer"],
          hats=["offopen", "sixteen"], percs=["cowbell", "shaker", "none"], bass=["acid"], bass_patches=["acid"],
          progressions=["i-i-i-VI", "i-II-i-VII"], chord_patches=["stab"], arp_patches=["acid", "arp"], lead_patches=["lead"],
          pad_patches=["sweep"], energy=4, distorted=True, modes=("minor", "phrygian"), acid=True),
    Style("Techno/Minimal", [122, 124, 126], 22, words=W("Click", "Pinpoint", "Dot", "Microdub", "Sparse Room", "Thin Line", "Tick", "Residue", "Space", "Ghost Note"),
          drums_sound=["tight", "standard"], kicks=["four", "fourpush"], backbeats=["rim24", "clap4", "clap24"],
          hats=["offclosed", "pedal", "shuffle"], percs=["rimsync", "rim3", "shaker"], bass=["offbeat", "pulse", "deep"],
          bass_patches=["sub", "pluck"], progressions=["i-i-i-i", "i-i-iv-i"], chord_patches=["chord", "glass"],
          arp_patches=["pluck"], lead_patches=["bell", "pluck"], pad_patches=["pad"], energy=2),
    Style("Techno/Melodic", [120, 122, 124], 22, words=W("Horizon", "Aurora", "Northern", "Glacier", "Afterglow", "Skyline", "Lumen", "Daybreak", "Halo", "Starfield"),
          drums_sound=["standard"], kicks=["four", "fourpush"], backbeats=["clap24", "snare24"],
          hats=["offopen", "sixteen", "ride"], percs=["shaker", "rim3"], bass=["offbeat", "rollacc", "octave8"],
          bass_patches=["roll", "sub"], progressions=["i-VI-III-VII", "i-iv-VI-VII", "i-III-VII-iv"], chord_patches=["chord", "warm"],
          arp_patches=["arp", "pluck", "glass"], lead_patches=["lead", "bell"], pad_patches=["pad", "warm"], energy=3, dark=False),
    Style("Techno/Percussive", [132, 134, 138], 22, words=W("Tribal", "Drum Circle", "Skin", "Bongo Room", "Hand Drum", "Rhythm Pit", "Woodblock", "Kinetic", "Clatter", "Stickwork"),
          drums_sound=["standard", "tight"], kicks=["four", "fourghost"], backbeats=["rim24", "clap24", "none"],
          hats=["offclosed", "shuffle", "ride"], percs=["toms", "conga", "rimsync", "cowbell"], bass=["offbeat", "pulse", "roll"],
          bass_patches=["roll", "sub"], progressions=["i-i-i-i", "i-i-i-VI"], chord_patches=["stab"], arp_patches=["pluck"],
          lead_patches=["pluck", "bell"], pad_patches=["drone"], energy=4),
    Style("Techno/Dub", [118, 120, 122], 22, words=W("Echo Chamber", "Dub Siren", "Tape Delay", "Deep Water", "Haze", "Spring Room", "Reverb Hall", "Mist", "Submerged", "Undercurrent"),
          drums_sound=["rumble", "standard"], kicks=["four", "rumble"], backbeats=["rim24", "clap4"],
          hats=["offclosed", "pedal", "rideoff"], percs=["rimsync", "none"], bass=["subhalf", "offbeat", "deep"],
          bass_patches=["sub", "dark"], progressions=["i-i-i-i", "i-i-iv-i"], chord_patches=["chord", "glass"],
          arp_patches=["pluck"], lead_patches=["glass", "bell"], pad_patches=["pad", "drone"], energy=2),

    # Industrial and its neighbours: the core again.
    Style("Industrial/EBM", [118, 120, 122, 124], 32, words=W("Body", "Marching", "Leather", "Iron Curtain", "Drill", "Boot Camp", "Uniform", "Muscle", "Parade", "Command", "Sweat", "Discipline"),
          drums_sound=["machine", "standard"], kicks=["ebm", "march", "four"], backbeats=["gated", "snare24", "clap24"],
          hats=["eighths", "sixteenflat", "offclosed"], percs=["metal", "none", "cowbell"], bass=["ebm", "ebmrest", "gallop", "octave8"],
          bass_patches=["ebm", "dark"], progressions=["i-VI-VII-i", "i-II-i-VII", "i-i-VI-VII"], chord_patches=["stab", "brass"],
          arp_patches=["arp", "ebm"], lead_patches=["lead", "saw"], pad_patches=["pad", "drone"], energy=4, modes=("minor", "phrygian")),
    Style("Industrial/Electro-industrial", [110, 114, 118, 122], 32, words=W("Static", "Circuit", "Wiretap", "Transmission", "Cold Wire", "Sector", "Surveillance", "Interference", "Datastream", "Blackout", "Signal Loss", "Glitch"),
          drums_sound=["machine", "hard"], kicks=["broken", "electro", "ebm"], backbeats=["snare24", "gated", "breakghost"],
          hats=["eighths", "sixteen", "pedal"], percs=["metal", "rimsync", "none"], bass=["electro", "ebm", "sixteens"],
          bass_patches=["dark", "ebm", "reese"], progressions=["i-II-i-VII", "i-iv-V-i", "i-II-VII-i"], chord_patches=["chord", "stab"],
          arp_patches=["arp"], lead_patches=["lead", "saw"], pad_patches=["drone", "noise"], energy=3, distorted=True, modes=("phrygian", "harmonic")),
    Style("Industrial/Aggrotech", [135, 138, 140, 145], 32, words=W("Rage", "Terror", "Hellfire", "Barbed", "Venom", "Assault", "Detonator", "Snarl", "Riot Gear", "Razor", "Bloodline", "Torment"),
          drums_sound=["hard", "machine"], kicks=["four", "gallop", "skip"], backbeats=["gated", "claplayer"],
          hats=["sixteenflat", "offopen"], percs=["crash", "metal", "none"], bass=["sixteensoct", "rollacc", "gallop"],
          bass_patches=["reese", "ebm"], progressions=["i-VI-VII-i", "i-VII-VI-VII", "i-iv-V-i"], chord_patches=["saw", "stab"],
          arp_patches=["saw", "arp"], lead_patches=["saw"], pad_patches=["noise", "sweep"], energy=5, distorted=True, modes=("minor", "harmonic")),
    Style("Industrial/Techno-body", [126, 128, 132, 134], 32, words=W("Body Shop", "Steel Body", "Muscle Machine", "Ironworks", "Engine Room", "Assembly", "Hydraulic", "Pistonbody", "Workshop", "Torque", "Chain Drive", "Conveyor"),
          drums_sound=["hard", "machine"], kicks=["four", "rumble", "fourghost"], backbeats=["claplayer", "gated"],
          hats=["sixteen", "offopen", "ride"], percs=["metal", "rim3", "cowbell"], bass=["ebm", "ebmrest", "rollacc"],
          bass_patches=["ebm", "roll"], progressions=["i-II-i-VII", "i-i-VI-VII", "i-i-i-VI"], chord_patches=["stab"],
          arp_patches=["arp", "ebm"], lead_patches=["lead", "saw"], pad_patches=["drone"], energy=4, modes=("minor", "phrygian")),
    Style("Industrial/Futurepop", [128, 130, 132, 135], 22, words=W("Neon", "Starlight", "Hologram", "Satellite", "Afterimage", "Future Tense", "Pulse Star", "Synthetic", "Daylight", "Prism"),
          drums_sound=["standard", "machine"], kicks=["four", "fourpush"], backbeats=["claplayer", "snare24"],
          hats=["offopen", "sixteen"], percs=["shaker", "none"], bass=["octave8", "offsixteen", "sixteensoct"],
          bass_patches=["ebm", "roll"], progressions=["i-VI-III-VII", "i-iv-VI-VII", "i-VI-VII-i"], chord_patches=["saw", "stab"],
          arp_patches=["saw", "arp"], lead_patches=["lead", "saw"], pad_patches=["pad", "sweep"], energy=4, dark=False),
    Style("Industrial/Synth-EBM", [120, 124, 126, 130], 22, words=W("Cold Heart", "Machine Love", "Chrome", "Silhouette", "Mirror", "Velvet", "Neon Rain", "Synthetic Tears", "Night Shift", "Glass Heart"),
          drums_sound=["machine", "standard"], kicks=["ebm", "four"], backbeats=["snare24", "clap24"],
          hats=["eighths", "offclosed"], percs=["none", "shaker"], bass=["octave8", "ebm", "eighths"],
          bass_patches=["ebm"], progressions=["i-VI-III-VII", "i-III-VII-iv", "i-VI-VII-i"], chord_patches=["chord", "stab"],
          arp_patches=["pluck", "arp"], lead_patches=["lead"], pad_patches=["pad"], energy=3),
    Style("Industrial/Darkwave", [100, 108, 112, 116], 22, words=W("Funeral", "Velvet Dark", "Raven", "Cathedral", "Mourning", "Candle", "Nocturne", "Black Lace", "Crypt", "Twilight"),
          drums_sound=["machine", "standard"], kicks=["half", "rock", "ebm"], backbeats=["gated", "snare24"],
          hats=["eighths", "offclosed"], percs=["none", "crash"], bass=["darkwave", "eighths", "octave8"],
          bass_patches=["dark", "ebm"], progressions=["i-VI-VII-i", "i-iv-VI-VII", "i-VI-iv-V"], chord_patches=["chord", "warm"],
          arp_patches=["pluck", "glass"], lead_patches=["lead", "bell"], pad_patches=["pad", "warm"], energy=2, modes=("minor", "harmonic")),
    Style("Industrial/Industrial rock", [112, 116, 120, 126], 22, words=W("Rust Belt", "Sledge", "Scrap Metal", "Wrecking Ball", "Live Wire", "Fuse Box", "Grind", "Barbwire", "Overload", "Breakdown"),
          drums_sound=["hard", "standard"], kicks=["rock", "march"], backbeats=["gated", "snare24"],
          hats=["eighths", "ride"], percs=["crash", "metal"], bass=["rock", "sixteens", "eighths"],
          bass_patches=["reese", "dark"], progressions=["i-II-i-VII", "i-VII-VI-VII", "i-i-VI-VII"], chord_patches=["saw", "brass"],
          arp_patches=["saw"], lead_patches=["saw", "lead"], pad_patches=["noise"], energy=4, distorted=True, modes=("phrygian", "minor")),
    Style("Industrial/Dark electro", [110, 115, 120, 125], 22, words=W("Abyss", "Plague", "Dead Signal", "Malice", "Ritual", "Nightmare", "Grave", "Bonecrusher", "Ashes", "Hex"),
          drums_sound=["machine", "hard"], kicks=["broken", "ebm", "march"], backbeats=["gated", "snare24"],
          hats=["eighths", "sixteenflat"], percs=["metal", "crash"], bass=["electro", "gallop", "sixteens"],
          bass_patches=["reese", "dark"], progressions=["i-iv-V-i", "i-II-i-VII", "i-i-VI-V"], chord_patches=["brass", "chord"],
          arp_patches=["arp"], lead_patches=["lead", "saw"], pad_patches=["noise", "drone"], energy=4, distorted=True, modes=("harmonic", "phrygian")),
    Style("Industrial/Coldwave", [104, 110, 116, 120], 22, words=W("Frost", "Grey Sky", "Concrete Winter", "Pale", "Distant", "Overcast", "Rainfall", "Isolation", "Empty Street", "Monochrome"),
          drums_sound=["machine", "tight"], kicks=["ebm", "half", "rock"], backbeats=["snare24", "rim24"],
          hats=["eighths", "offclosed"], percs=["none", "shaker"], bass=["eighths", "darkwave", "pulse"],
          bass_patches=["dark", "pluck"], progressions=["i-VI-VII-i", "i-i-iv-i", "i-III-VII-iv"], chord_patches=["chord", "glass"],
          arp_patches=["pluck", "glass"], lead_patches=["lead", "glass"], pad_patches=["pad"], energy=2),

    # Further afield.
    Style("Electro/Electro", [124, 128, 130], 18, words=W("Robot", "Mainframe", "Kraftwerk Lane", "Bodypop", "Computer", "Bassline Bot", "Transistor", "Mechanoid", "Data", "Breakdance"),
          drums_sound=["machine", "tight"], kicks=["electro", "electrob"], backbeats=["snare24", "clap24"],
          hats=["sixteen", "eighths"], percs=["cowbell", "none"], bass=["electro", "electrob"], bass_patches=["ebm", "dark"],
          progressions=["i-i-VI-VII", "i-II-i-VII"], chord_patches=["stab", "chord"], arp_patches=["arp", "pluck"], lead_patches=["lead", "bell"],
          pad_patches=["pad"], energy=4, modes=("minor", "phrygian"), breaks=True),
    Style("Breakbeat/Breaks", [128, 132, 135], 18, words=W("Breaker", "Rough Cut", "Street", "Spin Back", "Chopshop", "Funk Box", "Big Beat", "Skate", "Rewind", "Scratch"),
          drums_sound=["break", "standard"], kicks=["break", "breakb"], backbeats=["breakghost", "snareghost"],
          hats=["eighths", "sixteen", "shuffle"], percs=["crash", "none"], bass=["electrob", "halftime", "rollacc"], bass_patches=["reese", "roll"],
          progressions=["i-i-VI-VII", "i-VII-i-VI"], chord_patches=["stab"], arp_patches=["arp"], lead_patches=["lead", "saw"],
          pad_patches=["sweep"], energy=4, breaks=True),
    Style("Drum and Bass/Neurofunk", [172, 174], 18, words=W("Neuro", "Cyborg", "Synapse", "Mutant", "Nanotech", "Biohazard", "Implant", "Wired", "Overclock", "Mainline"),
          drums_sound=["break", "hard"], kicks=["dnb", "dnbroll"], backbeats=["dnb", "dnbghost"],
          hats=["dnb", "eighths"], percs=["none", "crash"], bass=["reese", "reesemove", "halftime"], bass_patches=["reese"],
          progressions=["i-II-i-VII", "i-i-i-i"], chord_patches=["stab", "brass"], arp_patches=["arp"], lead_patches=["saw", "lead"],
          pad_patches=["noise", "drone"], energy=5, distorted=True, modes=("phrygian", "minor"), breaks=True),
    Style("Drum and Bass/Liquid", [170, 172, 174], 18, words=W("Rainwater", "Silk", "Riverbed", "Soft Focus", "Lagoon", "Sunset", "Flowing", "Coastline", "Moonlit", "Breeze"),
          drums_sound=["break", "tight"], kicks=["dnb", "dnbroll"], backbeats=["dnbghost", "dnb"],
          hats=["dnb", "shuffle"], percs=["shaker", "none"], bass=["reesemove", "subhalf", "halftime"], bass_patches=["sub", "warm"],
          progressions=["i-VI-III-VII", "i-iv-VI-VII", "i-III-VII-iv"], chord_patches=["warm", "chord", "organ"], arp_patches=["glass", "pluck"],
          lead_patches=["bell", "glass"], pad_patches=["warm", "pad"], energy=3, dark=False, breaks=True),
    Style("Ambient/Dark ambient", [70, 80, 90], 18, words=W("Catacomb", "Deep Space", "Abandoned", "Hollow", "Ruin", "Fallout", "Lightless", "Wasteland", "Vault", "Silence"),
          drums_sound=["boom"], kicks=["halfsparse", "half"], backbeats=["none", "rim24"], hats=["none", "sparse"],
          percs=["tomfive", "crash", "none"], bass=["drone", "sub"], bass_patches=["sub", "drone"],
          progressions=["i-i-i-i", "i-II-i-VII", "i-i-VI-V"], chord_patches=["warm", "pad"], arp_patches=["glass"], lead_patches=["bell", "glass"],
          pad_patches=["drone", "noise", "pad"], energy=1, modes=("phrygian", "harmonic")),
    Style("Ambient/Drone", [60, 72, 80], 18, words=W("Monolith", "Stillness", "Horizon Line", "Long Shadow", "Slow Light", "Glacial", "Endless", "Static Field", "Dusk", "Plateau"),
          drums_sound=["boom"], kicks=["halfsparse"], backbeats=["none"], hats=["none", "sparse"], percs=["tomfive", "none"],
          bass=["drone", "sub"], bass_patches=["sub", "drone"], progressions=["i-i-i-i", "i-i-iv-i", "i-i-i-VI"], chord_patches=["warm", "pad"],
          arp_patches=["glass"], lead_patches=["glass", "bell"], pad_patches=["drone", "warm", "pad"], energy=1),
    Style("House/Deep house", [118, 120, 122], 18, words=W("Late Night", "Velvet Room", "Smoky", "Soulful", "Basement Jam", "Lounge", "Afterparty", "Low Lamp", "Rooftop", "Warm Up"),
          drums_sound=["standard", "tight"], kicks=["four", "fourpush"], backbeats=["clap24", "snare24"],
          hats=["offopen", "shuffle", "sixteen"], percs=["shaker", "conga", "rimsync"], bass=["deep", "deepwalk", "offbeat"],
          bass_patches=["sub", "organ"], progressions=["i-iv-VI-VII", "i-VII-VI-VII", "i-III-VII-iv"], chord_patches=["organ", "warm", "chord"],
          arp_patches=["pluck", "glass"], lead_patches=["bell", "organ"], pad_patches=["warm", "pad"], energy=3, dark=False),
    Style("Synthwave/Darksynth", [100, 110, 118, 124], 18, words=W("Midnight Run", "Chrome Night", "Nightcall", "Turbo", "Outrun", "Vice", "Laser Grid", "Neon Blood", "Highway Ghost", "Overdrive"),
          drums_sound=["hard", "machine"], kicks=["four", "rock"], backbeats=["gated", "snare24"],
          hats=["eighths", "sixteen"], percs=["crash", "none"], bass=["synthwave", "sixteensoct", "darkwave"], bass_patches=["reese", "ebm"],
          progressions=["i-VI-VII-i", "i-VI-iv-V", "i-VII-VI-VII"], chord_patches=["brass", "saw"], arp_patches=["arp", "saw"],
          lead_patches=["saw", "lead"], pad_patches=["pad", "sweep"], energy=4, distorted=True, modes=("minor", "harmonic")),
    Style("Trance/Psytrance", [140, 142, 145], 18, words=W("Mushroom", "Fractal", "Shaman", "Lotus", "Kaleidoscope", "Third Eye", "Forest Night", "Goa", "Mandala", "Spore"),
          drums_sound=["tight", "hard"], kicks=["psy"], backbeats=["clap4", "clap24"], hats=["psy", "offopen"],
          percs=["shaker", "rimsync"], bass=["psy"], bass_patches=["psy"], progressions=["i-i-i-i", "i-II-i-VII"], chord_patches=["stab"],
          arp_patches=["acid", "arp"], lead_patches=["saw", "lead"], pad_patches=["sweep", "noise"], energy=5, modes=("phrygian", "harmonic")),

    # Game audio: loops for the moments a game has, and its interface sounds and stingers.
    Style("Game/Combat", [140, 150, 160], 24, words=W("Skirmish", "Frontline", "Ambush", "Clash", "Onslaught", "Siege", "Battle", "Charge", "Vanguard", "Counterstrike"),
          drums_sound=["hard", "boom"], kicks=["march", "gallop", "four"], backbeats=["gated", "snare24"],
          hats=["sixteen", "eighths"], percs=["toms", "crash"], bass=["march", "sixteens", "gallop"], bass_patches=["reese", "ebm"],
          progressions=["i-VI-VII-i", "i-iv-V-i", "i-II-i-VII"], chord_patches=["brass", "stab"], arp_patches=["arp", "saw"],
          lead_patches=["brass", "saw"], pad_patches=["sweep", "pad"], energy=5, distorted=True, modes=("minor", "harmonic", "phrygian"), game=True),
    Style("Game/Exploration", [90, 100, 110], 24, words=W("Wanderer", "Open Field", "Old Ruins", "Pathfinder", "Valley", "Lost Woods", "Overworld", "Far Shore", "Caverns", "Compass"),
          drums_sound=["tight", "standard"], kicks=["half", "halfsparse"], backbeats=["rim24", "none"], hats=["shuffle", "sparse"],
          percs=["conga", "shaker"], bass=["pulse", "deepwalk", "subhalf"], bass_patches=["sub", "warm"],
          progressions=["i-VI-III-VII", "i-III-VII-iv", "i-iv-VI-VII"], chord_patches=["warm", "glass"], arp_patches=["glass", "pluck"],
          lead_patches=["bell", "glass"], pad_patches=["warm", "pad"], energy=2, dark=False, game=True),
    Style("Game/Menu", [90, 96, 100], 24, words=W("Title Screen", "Start Menu", "Save Point", "Lobby", "Loading", "Options", "Main Menu", "Inventory", "Hub", "Pause"),
          drums_sound=["tight"], kicks=["half", "halfsparse"], backbeats=["rim24", "none"], hats=["sparse", "offclosed"],
          percs=["shaker", "none"], bass=["pulse", "subhalf"], bass_patches=["sub", "warm"], progressions=["i-VI-III-VII", "i-iv-VI-VII"],
          chord_patches=["glass", "warm"], arp_patches=["glass", "pluck"], lead_patches=["bell", "glass"], pad_patches=["warm"],
          energy=1, dark=False, game=True),
    Style("Game/Chiptune", [140, 150, 160], 24, words=W("Pixel", "8-Bit", "Cartridge", "Arcade", "Bitcrush", "Joypad", "Sprite", "High Score", "Bonus Stage", "Power-Up"),
          drums_sound=["chip"], kicks=["four", "break", "march"], backbeats=["snare24", "snareghost"], hats=["eighths", "sixteen"],
          percs=["none", "crash"], bass=["chip", "octave8", "eighths"], bass_patches=["chipbass"],
          progressions=["i-VI-VII-i", "i-VI-III-VII", "i-iv-V-i"], chord_patches=["chip"], arp_patches=["chip"], lead_patches=["chip"],
          pad_patches=["chip"], energy=4, dark=False, modes=("minor", "harmonic"), game=True),
    Style("Game/Horror", [70, 80, 90], 24, words=W("Haunted", "Asylum", "Whisper", "Dread", "Cellar Door", "Flicker", "Creeping", "Bloodmoon", "Specter", "Lockdown"),
          drums_sound=["boom"], kicks=["halfsparse", "half"], backbeats=["none", "rim24"], hats=["none", "sparse"],
          percs=["tomfive", "metal"], bass=["drone", "pulse"], bass_patches=["sub", "drone"], progressions=["i-II-i-VII", "i-i-VI-V", "i-iv-V-i"],
          chord_patches=["warm", "chord"], arp_patches=["glass"], lead_patches=["bell", "glass"], pad_patches=["noise", "drone"],
          energy=2, modes=("harmonic", "phrygian"), game=True),
    Style("Game/Boss", [160, 170, 175], 24, words=W("Final Boss", "Showdown", "Titan", "Overlord", "Last Stand", "Colossus", "Dark Lord", "Arena", "Doom", "Rampage"),
          drums_sound=["hard", "boom"], kicks=["gallop", "four", "dnb"], backbeats=["gated", "snareghost"], hats=["sixteen", "sixteenflat"],
          percs=["toms", "crash", "metal"], bass=["gallop", "sixteensoct", "march"], bass_patches=["reese", "dark"],
          progressions=["i-II-i-VII", "i-iv-V-i", "i-i-VI-V"], chord_patches=["brass", "saw"], arp_patches=["saw", "arp"],
          lead_patches=["brass", "saw"], pad_patches=["noise", "sweep"], energy=5, distorted=True, modes=("harmonic", "phrygian"), game=True),
]

# How many of each role a style gets, by its size.
SPLITS = {
    32: dict(Drums=9, Bass=6, Harmony=4, Pads=3, Arps=4, Melody=3, SFX=3),
    24: dict(Drums=5, Bass=3, Harmony=3, Pads=3, Arps=3, Melody=3, SFX=4),
    22: dict(Drums=6, Bass=4, Harmony=3, Pads=2, Arps=3, Melody=2, SFX=2),
    18: dict(Drums=5, Bass=3, Harmony=2, Pads=2, Arps=2, Melody=2, SFX=2),
}
AMBIENT_SPLIT = dict(Drums=3, Bass=3, Harmony=2, Pads=5, Arps=2, Melody=2, SFX=1)
DRUM_ORDER = ["Full Grooves", "Fills", "Full Grooves", "Kick", "Hats & Tops", "Full Grooves", "Snare & Clap", "Percussion", "Breaks"]


class Maker:
    """Makes one style's presets, choosing with a generator seeded by the style's name."""

    def __init__(self, s, used):
        self.s = s
        self.r = random.Random(s.style)
        self.used = used
        self.made = {}   # what each preset is for, for the kits: "groove", "fill", "bass", ...

    def pick(self, items):
        return items[self.r.randrange(len(items))]

    def tempo(self):
        return self.pick(self.s.tempos)

    def name(self, nouns):
        words = list(self.s.words)
        nouns = list(nouns)
        self.r.shuffle(words)
        self.r.shuffle(nouns)

        for noun in nouns:
            for word in words:
                candidate = f"{word} {noun}"
                if candidate not in self.used:
                    self.used.add(candidate)
                    return candidate

        raise RuntimeError("out of names for " + self.s.style)

    def energy(self, shift=0):
        return max(1, min(5, self.s.energy + shift + self.r.choice([-1, 0, 0, 1])))

    def character(self, part, distorted=None):
        per_bar = len(part.notes) / part.bars
        return ["Dark" if self.s.dark else "Bright", "Dense" if per_bar >= 10 else "Sparse", "Straight",
                "Distorted" if (self.s.distorted if distorted is None else distorted) else "Clean"]

    def add(self, kind, nouns, role, sub, description, part, voice, tags=(), energy_shift=0, distorted=None, tempo=None):
        name = self.name(nouns)
        description = description[0].upper() + description[1:]
        PRESETS.append(Preset(name, role, sub, self.s.style, self.energy(energy_shift), self.character(part, distorted),
                              description, part, tempo or self.tempo(), voice, sorted(set(tags))))
        self.made.setdefault(kind, []).append(name)

    # -- Drums ------------------------------------------------------------------------------

    def groove(self, bars, parts=("kick", "back", "hats", "perc")):
        part = Part(bars)
        said = []
        chosen = {"kick": KICKS[self.pick(self.s.kicks)], "back": BACKBEATS[self.pick(self.s.backbeats)],
                  "hats": HATS[self.pick(self.s.hats)], "perc": PERCS[self.pick(self.s.percs)]}

        for k in parts:
            pieces, words = chosen[k]
            put(part, pieces)

            if words:
                said.append(words)

        return part, said

    def drums(self, sub):
        s = self.s
        sound = drums(self.pick(s.drums_sound))

        if sub == "Full Grooves":
            bars = self.pick([1, 2, 4])
            part, said = self.groove(bars)
            if bars == 4:
                for n, p in TOMRUN:
                    part.grid(n, p, bar=3)
                said.append("a tom run ends bar 4")
            if not said:
                return False
            self.add("groove", DRUM_NOUNS[sub], "Drums", sub, join(said), part, sound)
        elif sub == "Kick":
            pieces, words = KICKS[self.pick(s.kicks)]
            part = Part(2)
            put(part, pieces)
            part.grid(KICK, "..........g..g..", bar=1)
            self.add("kick", DRUM_NOUNS[sub], "Drums", sub, words + ", with ghost kicks leading into the next bar at the end of bar 2", part, sound)
        elif sub == "Snare & Clap":
            pieces, words = BACKBEATS[self.pick([b for b in s.backbeats if b != "none"] or ["rim24"])]
            part = Part(2)
            put(part, pieces)
            part.grid(SNARE, "............gogx", bar=1)
            self.add("snare", DRUM_NOUNS[sub], "Drums", sub, words + ", with a snare pickup ending bar 2", part, sound)
        elif sub == "Hats & Tops":
            first, second = HATS[self.pick([h for h in s.hats if h != "none"] or ["sparse"])], HATS[self.pick(["ride", "pedal", "offopen", "eighths"])]
            part = Part(2)
            put(part, first[0])
            if first[1] != second[1]:
                put(part, [(n, p) for n, p in second[0]], bar=1)
                said = f"{first[1]}, with {second[1]} added in bar 2"
            else:
                said = first[1]
            self.add("hats", DRUM_NOUNS[sub], "Drums", sub, said, part, sound)
        elif sub == "Percussion":
            choices = [p for p in s.percs if p not in ("none", "crash")] or ["rim3"]
            a = PERCS[self.pick(choices)]
            b = PERCS[self.pick(["conga", "rimsync", "shaker", "cowbell", "toms"])]
            part = Part(4)
            put(part, a[0])
            for n_p in b[0]:
                if n_p[0] != "cycle":
                    part.grid(n_p[0], n_p[1], bar=2).grid(n_p[0], n_p[1], bar=3)
            said = a[1] if a[1] == b[1] else f"{a[1]}, with {b[1]} joining in bars 3 and 4"
            self.add("perc", DRUM_NOUNS[sub], "Drums", sub, said, part, sound)
        elif sub == "Fills":
            kind = self.pick(["snare", "toms", "clap"])
            part = Part(1)
            kick = KICKS[self.pick(s.kicks)]
            put(part, [(n, p[:8] + "........") for n, p in kick[0] if isinstance(n, int)])
            if kind == "snare":
                part.grid(SNARE, "........gogoxxXX")
                words = "the kick for two beats, then a snare roll rising to the next bar"
            elif kind == "toms":
                for n, p in [(HT, "........xx......"), (MT, "..........xx...."), (LT, "............xxXX")]:
                    part.grid(n, p)
                words = "the kick for two beats, then toms falling high to low"
            else:
                part.grid(CLAP, "........x.x.xxxX")
                words = "the kick for two beats, then claps stuttering into the next bar"
            self.add("fill", DRUM_NOUNS[{"snare": "snare fill", "toms": "tom fill", "clap": "clap fill"}[kind]], "Drums", sub, words, part, sound)
        elif sub == "Breaks":
            part = Part(2)
            pattern = self.pick([
                ("X.x.......x.....X.........x..x..", "....X..g.g..X.......X..g.g..X.g.", "a two-bar break, the kick moving in bar 2 and the snare ghosting round the backbeat"),
                ("X.........X.....X.x.......x.....", "....X.......X..g....X..g....X...", "a two-step break with a kick pickup in bar 2"),
                ("X..X......X.....X..X....X.......", "....X..g.g..X.......X..g.g..X..X", "a chopped break, the snare doubling at the end of bar 2"),
            ])
            part.grid(KICK, pattern[0]).grid(SNARE, pattern[1])
            put(part, HATS[self.pick(["eighths", "sixteen", "shuffle"])][0])
            self.add("break", DRUM_NOUNS[sub], "Drums", sub, pattern[2], part, drums(self.pick(["break", s.drums_sound[0]])))
        return True

    # -- Bass -------------------------------------------------------------------------------

    def bass_bar(self, mode, rhythm, degree):
        root = degree_pitch(mode, degree, 28, 39)
        i = ladder_index(mode, root)
        tokens = []

        for t in rhythm.split():
            if t in (".", "-"):
                tokens.append(t)
                continue

            mark = "".join(c for c in t if c in "!~_")
            core = t.rstrip("!~_")
            p = {"1": root, "8": root + 12, "5": LADDER[mode][i + 4], "3": LADDER[mode][i + 2], "7": LADDER[mode][i + 6],
                 "b": LADDER[mode][i - 1]}[core]
            tokens.append(name_of(p) + mark)

        return " ".join(tokens)

    def acid_rhythm(self):
        tokens = []

        for step in range(16):
            r = self.r.random()
            if step % 4 == 0 or r < 0.55:
                note = self.pick(["1", "1", "1", "8", "5", "3", "7"])
                mark = "!" if self.r.random() < 0.25 else ("~" if self.r.random() < 0.2 else "")
                tokens.append(note + mark)
            else:
                tokens.append(".")

        # A slide needs a note to slide into.
        for k in range(16):
            if tokens[k].endswith("~") and (k == 15 or tokens[k + 1] == "."):
                tokens[k] = tokens[k][:-1]

        return " ".join(tokens)

    def bass(self, sub):
        s = self.s
        mode = self.pick(s.modes)
        prog_name = self.pick([p for p in s.progressions if PROGRESSIONS[p][1] in (mode, "minor")] or s.progressions)
        degrees, prog_mode = PROGRESSIONS[prog_name]
        mode = prog_mode if prog_mode != "minor" else mode

        if sub == "Sub":
            key = self.pick(["sub", "subhalf", "offbeat", "pulse", "drone"])
            patch = "sub"
        elif sub == "Arp Bass":
            key = "acid" if s.acid else self.pick(["chip", "octave8", "ebm", "electro"] if not s.game else ["chip", "octave8"])
            patch = "acid" if s.acid else self.pick(s.bass_patches)
        elif sub == "Picked & Played":
            key = self.pick(["deepwalk", "darkwave", "rock", "eighths", "deep"])
            patch = self.pick(["pluck", "dark", "organ"] if not s.game else s.bass_patches)
        else:
            key = self.pick(s.bass)
            patch = self.pick(s.bass_patches)

        rhythm, words = (self.acid_rhythm(), "a 303 line with accents and slides") if key == "acid" else BASS[key]
        bars = self.pick([2, 4]) if len(set(degrees)) > 1 else self.pick([1, 2, 4])
        moving = bars == 4 and len(set(degrees)) > 1
        part = Part(bars)

        for b in range(bars):
            degree = degrees[b] if moving else 0
            part.line(self.bass_bar(mode, rhythm, degree), gate=0.9 if key in ("sub", "subhalf", "drone", "reese", "reesemove") else 0.6, bar=b, repeat=False)

        words += (", moving through the roots of " + chord_names(mode, degrees)) if moving else ", on A"
        self.add("bass", ROLE_NOUNS[sub], "Bass", sub, words, part, synth(patch), self.mode_tags(mode, [n[0] for n in part.notes]))

    def mode_tags(self, mode, pitches):
        pcs = {p % 12 for p in pitches}
        tags = []
        if 10 in pcs:
            tags.append("Phrygian")
        if 8 in pcs:
            tags.append("Harmonic minor")
        return tags

    # -- Harmony and pads -------------------------------------------------------------------

    def progression(self):
        return PROGRESSIONS[self.pick(self.s.progressions)]

    def harmony(self, sub):
        s = self.s
        degrees, mode = self.progression()
        seventh = self.r.random() < 0.3 and not s.game
        chords = [names(triad(mode, d, seventh=seventh)) for d in degrees]
        chord_words = chord_names(mode, degrees, seventh)
        part = Part(4)
        patch = self.pick(s.chord_patches)

        if sub == "Progressions":
            rhythm = self.pick([None, "x.......x.......", "..x...x...x...x.", "x.x.x.x.x.x.x.x."])
            if rhythm is None:
                part.chords(chords, velocity=84)
                words = f"{chord_words}, a bar each"
            else:
                part.chords(chords, rhythm=rhythm, length_steps={"x.......x.......": 8, "..x...x...x...x.": 2, "x.x.x.x.x.x.x.x.": 1}[rhythm], velocity=86)
                words = f"{chord_words}, " + {"x.......x.......": "in half notes", "..x...x...x...x.": "struck on the offbeats", "x.x.x.x.x.x.x.x.": "pulsing in 8ths"}[rhythm]
        elif sub == "Keys & Comping":
            rhythm = self.pick(["..x...x...x...x.", "x..x..x...x..x..", "..x..x....x..x..", "x.....x.....x..."])
            part = Part(2)
            part.chords(chords[:2], rhythm=rhythm, length_steps=1, velocity=82)
            words = f"short {chord_names(mode, degrees[:2], seventh)} chords comping over two bars"
        elif sub == "Stabs":
            rhythm = self.pick(["x.....x.....x...", "..x.....x.x.....", "x..x..x.........", "x.x...x...x.x...", "...x......x....."])
            part = Part(1)
            part.chords([chords[0]], rhythm=rhythm, length_steps=1, velocity=104)
            words = f"{chord_name(mode, degrees[0], seventh)} stabs in a syncopated one-bar pattern"
            patch = self.pick(["stab"] + s.chord_patches)
        else:   # Strums: the chord's notes a 16th apart, each held to the bar's end
            part = Part(4)
            for b, ch in enumerate(chords):
                for k, n in enumerate(ch):
                    part.add(pitch(n), b * BAR + k * S, BAR - k * S - 40, 90 - 6 * k)
            words = f"{chord_words}, each strummed upwards a 16th a note"

        tags = self.mode_tags(mode, [n[0] for n in part.notes])
        self.add("harmony", ROLE_NOUNS[sub], "Harmony", sub, words, part, synth(patch), tags)

    def pads(self, sub):
        s = self.s
        degrees, mode = self.progression()
        seventh = self.r.random() < 0.4
        chords = [names(triad(mode, d, seventh=seventh)) for d in degrees]
        part = Part(4)
        patch = self.pick(s.pad_patches)

        if sub == "Sustained":
            part.chords(chords, velocity=80)
            words = f"{chord_names(mode, degrees, seventh)}, held a bar each"
        elif sub == "Evolving":
            part.chords(chords, velocity=78).lane(74, [(0, 0.2), (2 * BAR, 0.8), (4 * BAR - 1, 0.3)])
            words = f"{chord_names(mode, degrees, seventh)}, the filter opening and closing again over four bars"
        elif sub == "Swells":
            part = Part(2)
            part.chords([chords[0], chords[0]], velocity=84).lane(11, [(0, 0.0), (2 * BAR - 1, 1.0)])
            words = f"{chord_name(mode, degrees[0], seventh)} swelling up from silence over two bars"
        else:   # Drones
            part = Part(self.pick([4, 8]))
            root = degree_pitch(mode, 0, 40, 51)
            fifth = LADDER[mode][ladder_index(mode, root) + 4]
            for b in range(part.bars):
                part.add(root, b * BAR, BAR - 40, 90).add(fifth, b * BAR, BAR - 40, 70)
            part.lane(74, [(0, 0.1), (part.bars * BAR // 2, 0.6), (part.bars * BAR - 1, 0.1)])
            words = f"A and E held together for {part.bars} bars, the filter breathing in and out"
            patch = self.pick(["drone", "noise"] if "noise" in s.pad_patches else ["drone"])

        tags = self.mode_tags(mode, [n[0] for n in part.notes])
        self.add("pad", ROLE_NOUNS[sub], "Pads", sub, words, part, synth(patch), tags)

    # -- Arps and melody --------------------------------------------------------------------

    def arps(self, sub):
        s = self.s
        degrees, mode = self.progression()
        figures = [f for f in ARP_FIGURES if f[2] == sub]
        figure, words, _ = self.pick(figures)
        octave_low = self.pick([52, 57, 64])
        chords = [names(triad(mode, d, low=octave_low, high=octave_low + 11)) for d in degrees]
        bars = self.pick([1, 2, 4])
        chords = chords[:bars] if bars > 1 else chords[:1]
        part = arp(chords, figure, gate=0.4 if sub == "Gated" else 0.5)
        patch = self.pick(s.arp_patches)
        words += ", through " + chord_names(mode, degrees[:bars]) if bars > 1 else ", on " + chord_name(mode, degrees[0])
        tags = self.mode_tags(mode, [n[0] for n in part.notes])
        self.add("arp", ROLE_NOUNS[sub], "Arps", sub, words, part, synth(patch), tags)

    def motif(self, mode, low, high, rhythm):
        """A bar's notes for a rhythm, stepping through the mode from a chord tone."""
        ladder = [p for p in LADDER[mode] if low <= p <= high]
        start = self.pick([i for i, p in enumerate(ladder) if p % 12 in (9, 0, 4)])
        pos = start
        tokens = []

        for t in rhythm.split():
            if t in (".", "-"):
                tokens.append(t)
                continue

            tokens.append(name_of(ladder[pos]))
            pos = max(0, min(len(ladder) - 1, pos + self.pick([-2, -1, -1, 1, 1, 2, 0])))

        return tokens

    def melody(self, sub):
        s = self.s
        mode = self.pick(s.modes)
        rhythm, feel = self.pick(MELODY_RHYTHMS)
        low, high = {"Leads": (69, 88), "Hooks": (64, 84), "Counter-lines": (55, 72), "Bells & Mallets": (72, 93)}[sub]
        first = self.motif(mode, low, high, rhythm)
        second = self.motif(mode, low, high, rhythm)
        bars = self.pick([2, 4])

        if bars == 2:
            part = Part(2).line(" ".join(first), bar=0, repeat=False).line(" ".join(second), bar=1, repeat=False)
            words = f"a two-bar {feel} phrase"
        else:
            ending = self.motif(mode, low, high, self.pick(MELODY_RHYTHMS)[0])
            part = Part(4)
            for b, tokens in enumerate([first, second, first, ending]):
                part.line(" ".join(tokens), bar=b, repeat=False)
            words = f"a {feel} phrase stated twice, the second time ending another way"

        if not part.notes:
            part.add(pitch("a4"), 0, S, 96)

        if sub == "Bells & Mallets":
            patch = "chip" if s.style == "Game/Chiptune" else self.pick(["bell", "glass"])
        else:
            patch = self.pick(s.lead_patches)
        tags = self.mode_tags(mode, [n[0] for n in part.notes])
        self.add("melody", ROLE_NOUNS[sub], "Melody", sub, words + (" in " + {"minor": "A minor", "phrygian": "A Phrygian", "harmonic": "A harmonic minor"}[mode]),
                 part, synth(patch), tags)

    # -- SFX ----------------------------------------------------------------------------------

    def sfx(self, sub):
        s = self.s
        tempo = self.tempo()

        if sub == "Risers":
            bars = self.pick([2, 4])
            part = Part(bars)
            steps = bars * 16
            low = degree_pitch("minor", 0, 45, 56)
            ladder = [p for p in LADDER["minor"] if p >= low]
            for step in range(steps):
                part.add(ladder[step * 14 // steps], step * S, S // 2, min(127, 50 + step * 70 // steps))
            part.lane(74, [(0, 0.15), (bars * BAR - 1, 1.0)])
            words = f"16ths climbing the A minor scale for {bars} bars, louder and brighter as they go"
            patch = "chip" if s.style == "Game/Chiptune" else self.pick(["sweep", "saw"])
            self.add("build", ROLE_NOUNS[sub], "SFX", sub, words, part, synth(patch), tempo=tempo)
        elif sub == "Downlifters":
            part = Part(2).add(degree_pitch("minor", 0, 57, 68), 0, 2 * BAR - 40, 100).lane(128, [(0, 0.5), (2 * BAR - 1, 0.0)]).lane(74, [(0, 1.0), (2 * BAR - 1, 0.1)])
            self.add("build", ROLE_NOUNS[sub], "SFX", sub, "a held A bending down two semitones as its filter closes over two bars", part, synth("sweep"), tempo=tempo)
        elif sub == "Impacts":
            part = Part(1).add(KICK, 0, S // 2, 127).add(CRASH, 0, S // 2, 118).add(LT, 0, S // 2, 110)
            self.add("build", ROLE_NOUNS[sub], "SFX", sub, "kick, low tom and crash struck together on the one", part, drums(self.pick(["boom", "rumble", "hard"])), tempo=tempo)
        elif sub == "Sweeps":
            degrees, mode = self.progression()
            part = Part(4).chords([names(triad(mode, degrees[0]))] * 4, velocity=80).lane(74, [(0, 0.0), (4 * BAR - 1, 1.0)])
            self.add("build", ROLE_NOUNS[sub], "SFX", sub, f"{chord_name(mode, degrees[0])} held while its filter opens over four bars", part, synth("sweep"),
                     self.mode_tags(mode, [n[0] for n in part.notes]), tempo=tempo)
        elif sub == "Transitions":
            part = Part(2)
            for step in range(0, 32):
                if step >= 16 or step % 2 == 0:
                    part.add([HT, MT, LT][(step // 4) % 3], step * S, S // 2, min(127, 50 + step * 2))
            part.add(CRASH, 31 * S, S // 2, 120)
            self.add("build", ROLE_NOUNS[sub], "SFX", sub, "toms in 8ths, then 16ths, louder through two bars, a crash on the last 16th", part, drums(self.pick(s.drums_sound)), tempo=tempo)
        elif sub == "Hits":
            degrees, mode = self.progression()
            ch = names(triad(mode, degrees[0]))
            part = Part(1).chords([ch], rhythm="x...............", length_steps=4, velocity=110)
            self.add("build", ROLE_NOUNS[sub], "SFX", sub, f"one {chord_name(mode, degrees[0])} chord struck hard", part, synth(self.pick(["stab", "brass", "saw"])),
                     self.mode_tags(mode, [n[0] for n in part.notes]), tempo=tempo)
        elif sub == "UI Sounds":
            self.ui_sound(tempo)
        elif sub == "Stingers":
            self.stinger(tempo)

    UI = [
        ("Confirm", "two notes rising a fifth, A to E, for a confirm or select", [("a5", 0, 1), ("e6", 1, 2)]),
        ("Cancel", "two notes falling a fifth, E to A, for a cancel or back", [("e5", 0, 1), ("a4", 1, 2)]),
        ("Pickup", "a quick B to E for collecting an item or a coin", [("b5", 0, 1), ("e6", 1, 3)]),
        ("Error", "B and F together, a tritone, struck twice for an error", [("b4", 0, 1), ("f5", 0, 1), ("b4", 2, 1), ("f5", 2, 1)]),
        ("Level Up", "A minor arpeggiated up two octaves in 16ths, the last held", [("a4", 0, 1), ("c5", 1, 1), ("e5", 2, 1), ("a5", 3, 1), ("c6", 4, 1), ("e6", 5, 1), ("a6", 6, 4)]),
        ("Cursor", "one short high note for moving a cursor", [("e6", 0, 1)]),
        ("Notification", "three notes, C, E and A, rising for a notice", [("c6", 0, 1), ("e6", 1, 1), ("a6", 2, 3)]),
        ("Power Down", "five notes falling from A down the scale for losing a power", [("a5", 0, 1), ("g5", 1, 1), ("e5", 2, 1), ("c5", 3, 1), ("a4", 4, 3)]),
        ("Open", "C, E and G rising quickly for opening a menu", [("c5", 0, 1), ("e5", 1, 1), ("g5", 2, 2)]),
        ("Close", "G, E and C falling quickly for closing a menu", [("g5", 0, 1), ("e5", 1, 1), ("c5", 2, 2)]),
        ("Unlock", "a rising fourth then the octave for something unlocked", [("e5", 0, 1), ("a5", 1, 1), ("a6", 3, 3)]),
        ("Countdown", "a short note on each beat, the last an octave up", [("a5", 0, 1), ("a5", 4, 1), ("a5", 8, 1), ("a6", 12, 3)]),
    ]

    def ui_sound(self, tempo):
        label, words, notes = self.UI[self.ui_index % len(self.UI)]
        self.ui_index += 1
        part = Part(1)
        for n, step, length in notes:
            part.add(pitch(n), step * S, length * S - 20, 110)
        patch = self.pick(["chip", "bell", "glass", "pluck"] if self.s.style != "Game/Chiptune" else ["chip"])
        self.add("build", [label, label + " Blip", label + " Sound"], "SFX", "UI Sounds", words, part, synth(patch), tempo=tempo)

    STINGERS = [
        ("Victory", "a victory fanfare: A minor, F and G struck, landing on a held C major", [["a3", "c4", "e4"], ["f3", "a3", "c4"], ["g3", "b3", "d4"], ["g3", "c4", "e4"]], "x.x.x...x......."),
        ("Defeat", "a defeat sting: E minor and D minor falling to a low held A minor", [["e3", "g3", "b3"], ["d3", "f3", "a3"], ["a2", "c3", "e3"]], "x...x...x......."),
        ("Discovery", "a discovery sting: an A minor ninth rising a note a 16th and held", None, None),
        ("Danger", "a danger sting: A and G sharp struck low together, twice", [["a2", "g#3"], ["a2", "g#3"]], "x.....x........."),
        ("Quest Complete", "A minor, C and G struck, then E held on top", [["a3", "c4", "e4"], ["g3", "c4", "e4"], ["g3", "b3", "d4"], ["a3", "c4", "e4", "a4"]], "x..x..x.x......."),
        ("Game Over", "a slow fall through F, E minor and D minor to A minor", [["f3", "a3", "c4"], ["e3", "g3", "b3"], ["d3", "f3", "a3"], ["a2", "c3", "e3"]], "x...x...x...x..."),
    ]

    def stinger(self, tempo):
        label, words, chords, rhythm = self.STINGERS[self.sting_index % len(self.STINGERS)]
        self.sting_index += 1
        part = Part(2)

        if chords is None:
            for k, n in enumerate(["a3", "c4", "e4", "g4", "b4"]):
                part.add(pitch(n), k * S, 2 * BAR - k * S - 40, 100)
        else:
            steps = [i for i, c in enumerate(rhythm) if c == "x"]
            for i, (step, ch) in enumerate(zip(steps, chords)):
                last = i == len(chords) - 1
                length = (2 * BAR - step * S - 40) if last else 2 * S - 20
                for n in ch:
                    part.add(pitch(n), step * S, length, 110 if i == 0 else 100)

        tags = self.mode_tags("harmonic", [n[0] for n in part.notes])
        patch = "chip" if self.s.style == "Game/Chiptune" else self.pick(["brass", "warm", "stab"] if label != "Discovery" else ["glass", "warm", "bell"])
        self.add("build", [label, label + " Sting", label + " Stinger"], "SFX", "Stingers", words, part, synth(patch), tags, tempo=tempo)

    def make(self):
        s = self.s
        self.ui_index = self.r.randrange(len(self.UI))
        self.sting_index = self.r.randrange(len(self.STINGERS))
        split = AMBIENT_SPLIT if s.style.startswith("Ambient/") else SPLITS[s.size]

        drum_subs = [d for d in DRUM_ORDER if d != "Breaks" or s.breaks]
        if s.breaks:
            drum_subs.insert(1, "Breaks")
        for i in range(split["Drums"]):
            self.drums(drum_subs[i % len(drum_subs)])

        bass_subs = ["Synth Bass", "Sub", "Synth Bass", "Arp Bass", "Picked & Played", "Synth Bass"]
        if s.acid:
            bass_subs = ["Arp Bass", "Synth Bass", "Arp Bass", "Sub", "Arp Bass", "Synth Bass"]
        if s.style.startswith("Ambient/") or s.style in ("Game/Horror", "Game/Menu"):
            bass_subs = ["Sub", "Synth Bass", "Sub"]
        for i in range(split["Bass"]):
            self.bass(bass_subs[i % len(bass_subs)])

        for i in range(split["Harmony"]):
            self.harmony(["Progressions", "Stabs", "Keys & Comping", "Strums"][i % 4])
        for i in range(split["Pads"]):
            self.pads(["Sustained", "Evolving", "Drones", "Swells", "Sustained"][i % 5])
        for i in range(split["Arps"]):
            self.arps(["16th Sequences", "Plucks", "Up & Down", "Gated"][i % 4])
        for i in range(split["Melody"]):
            self.melody(["Hooks", "Leads", "Counter-lines", "Bells & Mallets"][i % 4])

        if s.game:
            sfx_subs = ["UI Sounds", "Stingers", "UI Sounds", "Stingers"] if s.style in ("Game/Menu", "Game/Chiptune") else ["Stingers", "UI Sounds", "Stingers", "Risers"]
        else:
            sfx_subs = ["Risers", "Transitions", "Impacts", "Downlifters", "Sweeps", "Hits"]
        for i in range(split["SFX"]):
            self.sfx(sfx_subs[i % len(sfx_subs)])

        return self

    def kits(self, count=3):
        """Starter kits: a groove, a bass, chords or a pad, an arp or a melody, a fill and a build."""
        m = self.made
        get = lambda keys, i: next((m[k][i % len(m[k])] for k in keys if m.get(k)), None)
        out = []
        words = list(self.s.words)
        self.r.shuffle(words)

        for i in range(count):
            parts = [get(["groove", "break"], i), get(["bass"], i), get(["harmony", "pad"], i), get(["arp", "melody"], i),
                     get(["fill", "perc", "snare"], i), get(["build"], i)]

            if None in parts:
                raise RuntimeError(f"{self.s.style}: a kit is missing a part")

            name = f"{self.s.kit_label} Kit" if i == 0 else f"{words[i]} Kit"
            tempo = self.s.tempos[len(self.s.tempos) // 2] if i == 0 else self.pick(self.s.tempos)
            out.append((name, tempo, self.s.style, parts))

        return out


def join(phrases):
    phrases = [p for p in phrases if p]
    return phrases[0] if len(phrases) == 1 else ", ".join(phrases[:-1]) + " and " + phrases[-1]


def generate():
    used = {p.name for p in PRESETS} | {k[0] for k in KITS}

    for s in STYLES:
        maker = Maker(s, used).make()

        for kit in maker.kits():
            name = kit[0]
            if name in used:
                name = name.replace(" Kit", " Starter Kit")
            assert name not in used, name
            used.add(name)
            KITS.append((name,) + kit[1:])


generate()


def kit_xml(name, tempo, style, parts):
    by_name = {p.name: p for p in PRESETS}
    out = [f'<TORQWAVE_KIT version="1" name={quoteattr(name)} key="{KEY}" tags="Starter kit" '
           f'note={quoteattr("A starting point for " + style.split("/")[1] + ": one of each part, in A minor at " + str(tempo) + " BPM")} '
           f'style={quoteattr(style)}>']

    for part_name in parts:
        p = by_name[part_name]
        tempo_before = p.tempo
        p.tempo = tempo
        out.append(p.xml("  "))
        p.tempo = tempo_before

    out.append("</TORQWAVE_KIT>")
    return "\n".join(out)


# ---------------------------------------------------------------------------------------------

def write(into):
    names = [p.name for p in PRESETS] + [k[0] for k in KITS]
    assert len(names) == len(set(names)), "names must be unique"

    for p in PRESETS:
        folder = os.path.join(into, p.role, p.sub)
        os.makedirs(folder, exist_ok=True)

        with open(os.path.join(folder, p.name + ".tqseq"), "w", encoding="utf-8", newline="\n") as f:
            f.write('<?xml version="1.0" encoding="UTF-8"?>\n\n' + p.xml() + "\n")

    os.makedirs(os.path.join(into, "Kits"), exist_ok=True)

    for name, tempo, style, parts in KITS:
        with open(os.path.join(into, "Kits", name + ".tqkit"), "w", encoding="utf-8", newline="\n") as f:
            f.write('<?xml version="1.0" encoding="UTF-8"?>\n\n' + kit_xml(name, tempo, style, parts) + "\n")


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    target = os.path.join(here, "..", "Data", "Presets")

    if "--calibrate" in sys.argv:
        # Each preset's track gain from its measured peak without it (name|peak in dB, as the tests write
        # with TQ_PRESET_LEVELS set): the gain that puts the peak at -9 dB, to the half dB.
        levels = sys.argv[sys.argv.index("--calibrate") + 1]
        gains = {}

        with open(levels, encoding="utf-8") as f:
            for line in f:
                name, _, peak = line.rstrip("\n").rpartition("|")

                # A preset in a kit is measured again at the kit's tempo: the loudest of them sets its gain.
                if name and float(peak) > -100.0:
                    gain = max(-30.0, min(18.0, round((-9.0 - float(peak)) * 2.0) / 2.0))
                    gains[name] = min(gain, gains.get(name, gain))

        with open(os.path.join(here, "preset_gains.txt"), "w", encoding="utf-8") as f:
            for name in sorted(gains):
                f.write(f"{name}|{gains[name]:.1f}\n")

        print(f"{len(gains)} gains written; run again to write the presets with them")
        return 0

    if "--check" in sys.argv:
        with tempfile.TemporaryDirectory() as scratch:
            write(scratch)
            compared = filecmp.dircmp(scratch, target)
            same = True

            def walk(c):
                nonlocal same
                if c.left_only or c.right_only or c.diff_files or c.funny_files:
                    same = False
                for sub in c.subdirs.values():
                    walk(sub)

            walk(compared)

            # dircmp compares shallowly; compare every file's bytes too.
            for root, _, files in os.walk(scratch):
                for name in files:
                    a = os.path.join(root, name)
                    b = os.path.join(target, os.path.relpath(a, scratch))
                    if not os.path.exists(b) or not filecmp.cmp(a, b, shallow=False):
                        same = False

            print("same" if same else "different")
            return 0 if same else 1

    if os.path.isdir(target):
        shutil.rmtree(target)

    write(target)
    counts = {}
    for p in PRESETS:
        counts[p.role] = counts.get(p.role, 0) + 1
    print(f"{len(PRESETS)} presets and {len(KITS)} kits in {os.path.normpath(target)}: " + ", ".join(f"{r} {n}" for r, n in counts.items()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
