#!/usr/bin/env python3
"""Writes Torqwave's factory presets: Data/Presets/<Role>/<Sub-role>/<Name>.tqseq, and the starter
kits in Data/Presets/Kits.

Every preset is made here from a compact definition (a 16-step grid per drum, a step list for a
line, chords by bar), so a change to the conventions is one edit and a rerun. The rules they keep
(docs/plans/PLAN-0.19.3.md, step 6), which the test suite checks again:

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


SYNTH = dict(name="Torqwave Synth", file="torqwave:basic-synth", uid="53425359", tag="TORQWAVE_BASIC_SYNTH")
DRUMS = dict(name="Basic Drums", file="torqwave:basic-drums", uid="53424452", tag="TORQWAVE_BASIC_DRUMS")

# Torqwave Synth settings for each kind of part (its parameters by ID; anything left out is its default).
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

    def xml(self, indent="", trim_db=0.0):
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
            label, instrument, settings = f"Torqwave Synth: {patch}", SYNTH, {"factoryPatch": patch}

        out.append(f"{indent}  <VOICE>{voice_xml(label, instrument, settings, GAINS.get(self.name, 0.0) + trim_db)}</VOICE>")
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

# Each kind of part: the patches it may play on, by name. The lists are fixed: a patch added to the synth
# later changes no preset (a preset picks from its list by its own name, so the list must not move).
# To put presets on a new patch, add its name here, run the script, and calibrate again.
VOICE_PATCHES = {
    "sub": ["Warehouse Sub", "Iron Sub"],
    "roll": ["Rolling Offbeat", "Rumble Floor", "Bunker Kick Bass"],
    "ebm": ["Pressure Bass", "Grinder", "Warehouse Sub", "Rumble Floor", "Bunker Kick Bass", "Rolling Offbeat", "Iron Sub", "Pulse Bunker",
        "Distorted 909 Bass", "FM Basement", "EBM Sequence Bass", "Body Music", "Abandoned Piston", "Abandoned Pump"],
    "acid": ["Acid Corrosion", "Square Acid", "Acid Scream", "Rubber Acid", "Acid Drip", "Abandoned Drip", "Basalt Sting", "Bitter Squeal", "Bleach Bite",
        "Bleak Sting"],
    "dark": ["Warehouse Sub", "Rumble Floor", "Pressure Bass", "Bunker Kick Bass", "Rolling Offbeat", "Iron Sub", "Grinder", "Pulse Bunker",
        "Distorted 909 Bass", "FM Basement", "EBM Sequence Bass", "Body Music", "Abandoned Piston", "Abandoned Pump", "Abyss Reese", "Tar Reese",
        "Corroded Reese", "Wormhole"],
    "reese": ["Abyss Reese", "Tar Reese", "Corroded Reese", "Wormhole", "Pipe Reese", "Acid Helix", "Ash Swell", "Ash Wire", "Basalt Rope",
        "Bitter Current"],
    "psy": ["Rubber Acid", "Pressure Bass", "Rolling Offbeat"],
    "chipbass": ["Square Acid", "Iron Sub"],
    "stab": ["Dub Chord Stab", "Minor Stab", "Metal Stab", "Rave Hoover Stab", "Strobe Stab", "Factory Hit", "Organ Stab", "Abandoned Clamp",
        "Abandoned Crack", "Abandoned Jab"],
    "chord": ["Dub Chord Stab", "Minor Stab", "Organ Stab", "Pump Chord"],
    "organ": ["Organ Stab"],
    "brass": ["Rave Hoover Stab", "Factory Hit", "Minor Stab"],
    "pad": ["Concrete Pad", "Ashfall", "Cathedral Of Rust", "Low Ceiling", "Grey Sky", "Sodium Lamp", "Tension Choir", "Fog Machine", "Acid Embers",
        "Acid Field", "Acid Smoke", "Ash Haze"],
    "warm": ["Grey Sky", "Sodium Lamp", "Low Ceiling"],
    "drone": ["Reactor Drone", "Turbine", "Substation", "Engine Room", "Void Hum", "Power Line", "Abandoned Mains", "Abandoned Reactor", "Acid Furnace",
        "Acid Grid"],
    "arp": ["Black Pluck", "Glass Shard", "Wire Pluck", "Rubber Pluck", "Bit Pluck", "Dark Kalimba", "Acid Shard", "Basalt Wire", "Clockwork"],
    "pluck": ["Black Pluck", "Glass Shard", "Wire Pluck", "Rubber Pluck", "Bit Pluck", "Dark Kalimba", "Acid Shard", "Basalt Wire", "Black Pin",
        "Black Tick"],
    "glass": ["Glass Shard", "Dark Kalimba", "Cold Bell"],
    "bell": ["Cold Bell", "Prison Bell", "Rusted Chime", "FM Toll", "Dead Glockenspiel", "Minor Bell Motif", "Gamelan Machine", "Abandoned Chime"],
    "lead": ["Siren Burn", "Mercury Lead", "Feedback Wire", "Industrial Whistle", "Rust Lead", "Cold Saw Lead", "Vowel Scream", "Abandoned Cutter",
        "Acid Signal", "Bitter Arc"],
    "saw": ["Cold Saw Lead", "Rust Lead", "Mercury Lead"],
    "chip": ["Bit Pluck", "Industrial Whistle"],
    "sweep": ["Tension Riser", "Siren Climb", "Machine Spin Up"],
    "noise": ["Downlifter Noise", "Radio Static", "Ventilation"],
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
    # One kit each, for the grooves written for that kit (after the generated ones, below).
    "breaks":      ["Breaks"],
    "modern":      ["Modern"],
    "warehouse":   ["Warehouse"],
    "bodymusic":   ["Body Music"],
    "foundry":     ["Foundry"],
    "brokenbeat":  ["Broken Beat"],
    "bunker":      ["Bunker"],
    "machineshop": ["Machine Shop"],
    "voltage":     ["Voltage"],
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


# Presets moved by name to a patch that suits them, where the patch their name drew does not: a pad on a
# pluck that does not hold, a drone on a patch that takes bars to arrive.
PATCH_FOR = {
    "Arcade Wash": "Platinum Square", "High Score Bloom": "Platinum Square",                 # were on Bit Pluck, which does not hold
    "Haunted Hum": "Reactor Drone", "Slag Drone": "Abandoned Reactor",   # took 4 to 8 seconds to arrive
}


def patch_for(sound, preset_name):
    if preset_name in PATCH_FOR:
        assert any(PATCH_FOR[preset_name] in v for v in FACTORY_PATCHES.values()), PATCH_FOR[preset_name] + " is not a factory patch"
        return PATCH_FOR[preset_name]

    # "=Name" is that factory patch and no other: a figure written for one sound.
    if sound.startswith("="):
        assert any(sound[1:] in v for v in FACTORY_PATCHES.values()), sound[1:] + " is not a factory patch"
        return sound[1:]

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
       "The sub on the root, an 8th on each offbeat and a 16th more before the bar", Part(1).line(". . a1 . . . a1 . . . a1 . . . a1 a1", gate=0.9), 134, synth("sub"))
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
       "A minor, C, G, F under a verse, each over its root an octave down",
       Part(4).chords([["a2"] + Am, ["c3"] + C, ["g2"] + G, ["f2"] + F], velocity=70), 126, synth("pad"))
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
       "A low A and the A above it, the filter breathing every bar",
       Part(4).add(pitch("a1"), 0, 4 * BAR - 40, 90).add(pitch("a2"), 0, 4 * BAR - 40, 80).lane(74, [(i * BAR // 2, 0.5 if i % 2 == 0 else 0.8) for i in range(9)]), 150, synth("drone"))
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
       "A sub hit on a low E at the drop, bending down over a bar", Part(1).add(pitch("e1"), 0, BAR - 40, 127).lane(128, [(0, 1.0), (BAR - 1, 0.0)]), 150, synth("sub"))

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

# More Torqwave Synth settings for the styles the set reaches into.
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

BREAKS = [
    ("X.x.......x.....X.........x..x..", "....X..g.g..X.......X..g.g..X.g.", "a two-bar break, the kick moving in bar 2 and the snare ghosting round the backbeat"),
    ("X.........X.....X.x.......x.....", "....X.......X..g....X..g....X...", "a two-step break with a kick pickup in bar 2"),
    ("X..X......X.....X..X....X.......", "....X..g.g..X.......X..g.g..X..X", "a chopped break, the snare doubling at the end of bar 2"),
    ("X.....x...X..x..X.....x.x.......", "....X.......X.......X..g....X.gX", "a rolling break, two kicks answering the snare in bar 2"),
    ("X..x....x.X.....X..x......X.x...", "....X.g.....X..g....X.g..g..X...", "a funk break, the kick pushed and the snare ghosted"),
    ("X.x...x...X.....X.x...x.X.......", "....X.......X.g.....X.......XgXg", "a break with a kick in threes and a snare flurry at the end"),
]

# How a kick line gets into the next bar, a snare line likewise, and the fills of each kind: the first of
# each is what there was.
KICK_TAILS = [
    ("..........g..g..", "with ghost kicks leading into the next bar at the end of bar 2"),
    ("..............x.", "with a pickup kick an 8th before the next bar"),
    ("............g.gx", "with three kicks crowding into the next bar"),
    (".......g.......g", "with a ghost kick before beat 3 and before the next bar in bar 2"),
]

BACKBEAT_TAILS = [
    (SNARE, "............gogx", "with a snare pickup ending bar 2"),
    (SNARE, "..............xx", "with two snares on the last 8th of bar 2"),
    (CLAP, "..........x...x.", "with claps on the offbeats of 3 and 4 in bar 2"),
    (SNARE, "......g..g....g.", "with ghost snares scattered through bar 2"),
    (RIM, "..o...o...o...o.", "with a rim on the offbeats of bar 2"),
]

FILLS = {
    "snare": [
        ([(SNARE, "........gogoxxXX")], "the kick for two beats, then a snare roll rising to the next bar"),
        ([(SNARE, "........x.x.x.xX")], "the kick for two beats, then the snare in 8ths and a flam into the bar"),
        ([(SNARE, "........X..x..xX")], "the kick for two beats, then the snare in a dotted figure"),
        ([(SNARE, "..........gggxxX")], "the kick for two beats, a gap, then a short snare roll"),
        ([(SNARE, "........x..xx.xX"), (CH, "........o.o.o.o.")], "the kick for two beats, then a broken snare figure over hats"),
        ([(SNARE, "........gxgxgxgX")], "the kick for two beats, then ghost and full snares alternating"),
        ([(SNARE, "............XXXX"), (KICK, "........x.x.....")], "the kick for two beats, two more in 8ths, then four snares"),
        ([(SNARE, "........X.....xX"), (CRASH, "........x.......")], "the kick for two beats, a snare and crash on 3, two snares into the bar"),
    ],
    "toms": [
        ([(HT, "........xx......"), (MT, "..........xx...."), (LT, "............xxXX")], "the kick for two beats, then toms falling high to low"),
        ([(LT, "........xx......"), (MT, "..........xx...."), (HT, "............xxXX")], "the kick for two beats, then toms climbing low to high"),
        ([(HT, "........x..x...."), (MT, "..........x..x.."), (LT, "..............xX")], "the kick for two beats, then high and mid toms trading and a low tom into the bar"),
        ([(LT, "........x.x.x.xX")], "the kick for two beats, then the low tom alone in 8ths"),
        ([(HT, "........x...x..."), (LT, "..........x...xX")], "the kick for two beats, then high and low toms answering each other"),
        ([(MT, "........xxx....."), (LT, "...........xxxXX")], "the kick for two beats, then three on the mid tom and five on the low"),
        ([(HT, "........X......."), (MT, "..........X....."), (LT, "............X.X.")], "the kick for two beats, then one tom to a beat, down"),
        ([(LT, "........x..x..x."), (SNARE, "...............X")], "the kick for two beats, the low tom in threes, a snare on the last 16th"),
    ],
    "clap": [
        ([(CLAP, "........x.x.xxxX")], "the kick for two beats, then claps stuttering into the next bar"),
        ([(CLAP, "........x..x..xX")], "the kick for two beats, then claps in a dotted figure"),
        ([(CLAP, "............xxxX")], "the kick for two beats, a beat of nothing, then four claps"),
        ([(CLAP, "........X...x.xX"), (SNARE, "........x.......")], "the kick for two beats, a clap and snare on 3, then claps into the bar"),
        ([(CLAP, "........xx..xx.X")], "the kick for two beats, then claps in pairs"),
        ([(CLAP, "..........x.x.xX"), (RIM, "........o.o.....")], "the kick for two beats, two rims, then claps into the bar"),
    ],
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


def root_names(mode, degrees):
    return ", ".join(LETTERS[MODES[mode][d % 7]] for d in degrees)


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

# Pads. A texture lays a bar's chord out (the chords as pitches, lowest first): what is held, where
# it sits and when it comes in. Each returns the part.

def pad_part(bars, chords, voice):
    """A part from voice(chord, bar) -> [(pitch, start step in the bar, length in steps)]."""
    part = Part(bars)
    for b in range(bars):
        for p, start, length in voice(chords[b % len(chords)], b):
            part.add(p, b * BAR + start * S, length * S - 40, 80)
    return part


def step_above(p):
    """The next note of A minor above a pitch."""
    return next(q for q in LADDER["minor"] if q > p)


def tex_block(ch, bars):    return pad_part(bars, ch, lambda c, b: [(p, 0, 16) for p in c])
def tex_low(ch, bars):      return pad_part(bars, ch, lambda c, b: [(c[0] - 12, 0, 16)] + [(p, 0, 16) for p in c])
def tex_open(ch, bars):     return pad_part(bars, ch, lambda c, b: [(c[0] - 12, 0, 16), (c[2], 0, 16), (c[1] + 12, 0, 16)])
def tex_dyad(ch, bars):     return pad_part(bars, ch, lambda c, b: [(c[0], 0, 16), (c[1] + 12, 0, 16)])
def tex_fifths(ch, bars):   return pad_part(bars, ch, lambda c, b: [(c[0] - 12, 0, 16), (c[2] - 12, 0, 16)])
def tex_air(ch, bars):      return pad_part(bars, ch, lambda c, b: [(p + 12, 0, 16) for p in c[1:]])
def tex_bloom(ch, bars):    return pad_part(bars, ch, lambda c, b: [(p, 4 * i, 16 - 4 * i) for i, p in enumerate(c[:4])])
def tex_top(ch, bars):      return pad_part(bars, ch, lambda c, b: [(p, 0, 16) for p in c[:-1]] + [(c[-1], 0, 8), (step_above(c[-1]), 8, 8)])
def tex_early(ch, bars):    return _early(ch, bars)
def tex_pedal(ch, bars):    return pad_part(bars, ch, lambda c, b: ([(ch[0][0] - 12, 0, 16)]) + [(p, 0, 16) for p in c[1:]])
def tex_rise(ch, bars):     return pad_part(bars, ch, lambda c, b: [(p + (12 if i < b else 0), 0, 16) for i, p in enumerate(c[:3])])
def tex_fall(ch, bars):     return pad_part(bars, ch, lambda c, b: [(p + 12 - (12 if i >= 3 - b else 0), 0, 16) for i, p in enumerate(c[:3])])


def _early(ch, bars):
    """Each chord comes an 8th before its bar: the first on the one, the rest pushed."""
    part = Part(bars)
    for b in range(bars):
        start = b * 16 - (2 if b else 0)
        end = (b + 1) * 16 - (2 if b < bars - 1 else 0)
        for p in ch[b % len(ch)]:
            part.add(p, start * S, (end - start) * S - 40, 80)
    return part


def tex_half(ch, bars):
    """Two chords to a bar: the progression goes round twice."""
    part = Part(bars)
    for h in range(bars * 2):
        for p in ch[h % len(ch)]:
            part.add(p, h * 8 * S, 8 * S - 40, 80)
    return part


def tex_twobar(ch, bars):
    """Two bars a chord: the first, then the first that differs from it."""
    other = next((c for c in ch[1:] if c != ch[0]), ch[0])
    part = Part(bars)
    for i, c in enumerate([ch[0], other][: max(1, bars // 2)]):
        for p in c:
            part.add(p, i * 2 * BAR, min(2, bars) * BAR - 40, 80)
    return part


PAD_TEXTURES = {
    "block":  (tex_block, "held a bar each"),
    "low":    (tex_low, "held a bar each over the root an octave down"),
    "open":   (tex_open, "spread wide: the root low, the fifth, the third on top"),
    "dyad":   (tex_dyad, "as two notes only, the root and the third a tenth above"),
    "fifths": (tex_fifths, "as bare fifths, low"),
    "air":    (tex_air, "without the root, an octave up"),
    "bloom":  (tex_bloom, "each coming in a note a beat from the bottom"),
    "top":    (tex_top, "the top note stepping up at half the bar"),
    "early":  (tex_early, "each pushed an 8th ahead of its bar"),
    "pedal":  (tex_pedal, "over an A held underneath all four"),
    "rise":   (tex_rise, "each voiced higher than the last"),
    "fall":   (tex_fall, "each voiced lower than the last"),
    "half":   (tex_half, "two to a bar, round twice"),
    "twobar": (tex_twobar, "the first two of them, two bars each"),
}

# An evolving pad's filter: (where in the preset, how open), and what to say.
PAD_LANES = [
    ([(0, 0.45), (0.5, 0.9), (1, 0.5)], "the filter opening and closing again"),
    ([(0, 0.4), (1, 0.9)], "the filter opening the whole way"),
    ([(0, 0.9), (1, 0.45)], "the filter closing the whole way"),
    ([(0, 0.45), (0.25, 0.8), (0.5, 0.45), (0.75, 0.8), (1, 0.45)], "the filter breathing twice"),
    ([(0, 0.7), (0.5, 0.4), (1, 0.9)], "the filter dipping at the middle and opening past where it began"),
]

PADS_FOR = {
    "Techno": ["block", "fifths", "pedal", "twobar", "low"],
    "Techno/Dark": ["pedal", "fifths", "twobar", "low", "dyad"],
    "Techno/Melodic": ["open", "top", "bloom", "air", "rise"],
    "Techno/Dub": ["early", "air", "dyad", "open"],
    "Techno/Hypnotic": ["twobar", "pedal", "half", "dyad"],
    "Industrial": ["fifths", "low", "pedal", "block", "fall"],
    "Industrial/Darkwave": ["open", "top", "fall", "bloom"],
    "Industrial/Coldwave": ["air", "dyad", "top", "open"],
    "Industrial/Futurepop": ["rise", "open", "half", "top"],
    "Electro": ["half", "early", "dyad", "air"],
    "Breakbeat": ["early", "half", "open", "air"],
    "Drum and Bass": ["open", "air", "bloom", "top", "early"],
    "Ambient": ["twobar", "bloom", "pedal", "open", "air"],
    "House": ["early", "top", "air", "open"],
    "Synthwave": ["rise", "low", "top", "half"],
    "Trance": ["pedal", "rise", "half"],
    "Game": ["bloom", "top", "fall", "open", "dyad"],
}

# Drones: what is held (a note, where in the preset it comes in and where it leaves, 0 to 1), what to
# call it, the tags it needs. Nothing lasts under two bars.
DRONE_FIGURES = {
    "fifth":    ([("a1", 0, 1), ("e2", 0, 1)], "A and E held together", []),
    "root":     ([("a1", 0, 1)], "one low A held", []),
    "octaves":  ([("a1", 0, 1), ("a2", 0, 1), ("a3", 0, 1)], "A held in three octaves", []),
    "flattwo":  ([("a1", 0, 1), ("bb2", 0.5, 1)], "a low A, the B flat a ninth above it joining halfway", ["Phrygian"]),
    "fourth":   ([("a1", 0, 1), ("d2", 0, 1), ("a2", 0.5, 1)], "A and D held, the A above joining halfway", []),
    "tritone":  ([("a1", 0, 1), ("eb2", 0.25, 1)], "a low A, the tritone above it joining after a quarter", ["Chromatic"]),
    "cluster":  ([("a2", 0, 1), ("bb2", 0.25, 1), ("b2", 0.5, 1)], "A, B flat and B coming in a quarter apart, a cluster", ["Chromatic"]),
    "latefifth": ([("a1", 0, 1), ("e2", 0.5, 1)], "a low A, the fifth joining halfway", []),
    "lateoctave": ([("a1", 0, 1), ("a2", 0.75, 1)], "a low A, the octave joining for the last quarter", []),
    "stack":    ([("a1", 0, 1), ("e2", 0, 1), ("a2", 0, 1), ("e3", 0, 1)], "A and E stacked through two octaves", []),
    "sixth":    ([("a1", 0, 1), ("e2", 0, 0.5), ("f2", 0.5, 1)], "a low A under an E that moves up to F halfway", []),
    "ninth":    ([("a1", 0, 1), ("a2", 0, 0.5), ("b2", 0.5, 1)], "a low A under its octave, which steps up to B halfway", []),
    "seventh":  ([("a1", 0, 1), ("e2", 0.25, 1), ("g2", 0.5, 1)], "A, then E, then G, a quarter apart: a seventh without its third", []),
    "third":    ([("a1", 0, 1), ("e2", 0, 1), ("c3", 0.5, 1)], "A and E held, the minor third joining on top halfway", []),
    "leading":  ([("a1", 0, 1), ("g#2", 0.5, 0.75), ("a2", 0.75, 1)], "a low A, the G sharp under its octave pulling up to it at the end", ["Harmonic minor"]),
    "low":      ([("e1", 0, 1), ("a1", 0.5, 1)], "a low E, the A above it joining halfway", []),
}

# A drone is there from its first bar: its filter never starts or ends near closed.
DRONE_LANES = [
    ([(0, 0.5), (0.5, 0.85), (1, 0.5)], "the filter breathing in and out"),
    ([(0, 0.45), (1, 0.9)], "the filter opening slowly"),
    ([(0, 0.9), (1, 0.5)], "the filter closing slowly"),
    ([(0, 0.5), (0.25, 0.8), (0.5, 0.5), (0.75, 0.8), (1, 0.5)], "the filter breathing twice"),
]

# Drones written for one style: a drone that only holds is a hum. These move inside, by a lane that
# keeps time or a note that changes, and every note still lasts two bars or more.

def ticking(bars, pattern, low, high, weak=None, rise=0.0):
    """A lane that strikes on a bar's 16ths ('x' full, 'o' weaker, '.' not) and falls back before the
    next: a filter or a level that keeps time. `rise` lifts the strikes by that much from first to last."""
    points, steps = [(0, low)], bars * 16
    for i in range(steps):
        c = pattern[i % len(pattern)]
        if c != ".":
            top = (high if c == "x" else weak if weak is not None else (low + high) / 2) - rise * (1 - i / steps)
            points += [(i * S + 1, max(low, top)), (i * S + S - 20, low)]
    return points


def drone_ebm(voice):
    part = held([("a1", 0, 100), ("e2", 0, 92), ("a2", 0, 84)], 4).lane(74, ticking(4, "x.oxx.ox", 0.25, 0.9, weak=0.6))
    return "a bare fifth and its octave, A, E and A, held for four bars while the filter marches in 16ths: an 8th and two 16ths, over and over", part, synth(voice), []


def drone_driving(voice):
    part = held([("a1", 0, 100), ("e2", 0, 90), ("a2", 6, 88)], 8)
    lane = ticking(4, "x...x...x...x...", 0.3, 0.95, rise=0.35) + [(t + 4 * BAR, v) for t, v in ticking(4, "x.x.x.x.x.x.x.x.", 0.3, 0.95, rise=0.1)[1:]]
    return "A and E held for eight bars, the filter pulsing on every beat and brighter each time, then on every 8th, the A above joining for the last two bars: a motor picking up", part.lane(74, lane), synth(voice), []


def drone_combat(voice):
    part = held([("a1", 0, 100), ("e2", 0, 88), ("bb1", 2, 96)], 4)
    part.lane(128, [(0, 0.5), (BAR, 0.44), (BAR + 2 * TPB, 0.56), (2 * BAR + TPB, 0.46), (3 * BAR, 0.6), (3 * BAR + 3 * TPB, 0.5)])
    part.lane(74, [(0, 0.4), (BAR + 2 * TPB, 0.9), (BAR + 2 * TPB + S, 0.35), (3 * BAR, 0.85), (3 * BAR + S, 0.4), (4 * BAR - 1, 0.95)])
    return "A and E held, the B flat pressing in at bar 3, the pitch wavering a little either way and the filter surging and snapping shut", part, synth(voice), ["Phrygian"]


def drone_exploration(voice):
    part = held([("a2", 0, 92), ("e3", 0, 84)], 8)
    for i, name in enumerate(["b3", "c4", "e4", "d4"]):
        part.add(pitch(name), i * 2 * BAR, 2 * BAR - 40, 80)
    part.lane(74, [(0, 0.45), (2 * BAR, 0.75), (4 * BAR, 0.55), (6 * BAR, 0.9), (8 * BAR - 1, 0.6)])
    return "A and E held for eight bars under a top note that moves every two: B, C, E, D, the filter opening and settling as it goes", part, synth(voice), []


def drone_hard(voice):
    part = held([("a1", 0, 104), ("a2", 0, 90)], 4).lane(74, ticking(4, ".xxo", 0.3, 0.85, weak=0.6))
    return "A low A and the A above it, the filter rolling through the three 16ths after every beat, as a rumble does", part, synth(voice), []


def drone_ebm_tension(voice):
    part = held([("a1", 0, 100), ("a2", 0, 90), ("eb3", 1, 92)], 4)
    part.lane(11, ticking(4, "x.x.x.x.x.x.xxxx", 0.35, 1.0)).lane(74, [(0, 0.45), (4 * BAR - 1, 0.9)])
    return "An octave on A gated in 8ths, with 16ths at the end of each bar, the tritone above joining at bar 2 as the filter opens", part, synth(voice), ["Chromatic"]


def rewrite(name, written):
    """A preset written by hand above, written again here where the lanes that keep time are."""
    words, part, voice, tags = written
    old = next(p for p in PRESETS if p.name == name)
    remember(old.role + " > " + old.sub, old.style, old.part, old.voice[0] == "drums")   # what it played stays taken
    old.description, old.part, old.voice, old.tags = words, part, voice, list(tags)


# The style, what writes its drone, and the voice it is written for.
DRONES_WRITTEN = {
    "Industrial/EBM": (drone_ebm, "Pulse Bunker"), "Techno/Driving": (drone_driving, "Abyss Reese"),
    "Game/Combat": (drone_combat, "Corroded Reese"), "Game/Exploration": (drone_exploration, "Cathedral Of Rust"),
}

DRONES_FOR = {
    "Techno": ["fifth", "root", "latefifth", "stack"],
    "Techno/Dark": ["flattwo", "root", "low", "fifth"],
    "Techno/Industrial": ["tritone", "cluster", "low"],
    "Industrial": ["tritone", "low", "fourth", "cluster"],
    "Industrial/Darkwave": ["third", "leading", "sixth"],
    "Industrial/Coldwave": ["ninth", "sixth", "third"],
    "Electro": ["fourth", "seventh"],
    "Breakbeat": ["seventh", "fourth"],
    "Drum and Bass": ["ninth", "seventh", "lateoctave"],
    "Ambient": ["octaves", "sixth", "ninth", "lateoctave", "third"],
    "Ambient/Dark ambient": ["cluster", "flattwo", "low", "tritone"],
    "House": ["seventh", "ninth"],
    "Synthwave": ["stack", "lateoctave", "third"],
    "Trance": ["flattwo", "leading", "stack"],
    "Game": ["third", "fourth", "sixth"],
    "Game/Horror": ["cluster", "tritone", "leading"],
}

# How a progression is played: the rhythm in a bar (None: held), how long each strike lasts, what to say.
CHORD_RHYTHMS = [
    (None, None, "a bar each"),
    ("x.......x.......", 8, "in half notes"),
    ("..x...x...x...x.", 2, "struck on the offbeats"),
    ("x.x.x.x.x.x.x.x.", 1, "pulsing in 8ths"),
    ("x.....x.....x...", 3, "three times a bar in a three-three-two figure"),
    ("x...........x...", 4, "on the one and again on beat 4"),
    ("x..x..x..x..x...", 2, "in dotted 8ths across the bar"),
    ("....x.......x...", 4, "on beats 2 and 4"),
    ("x.......x.x.....", 2, "on the one, on 3 and its offbeat"),
    ("x...x...x...x...", 2, "on every beat, short"),
    ("..x.......x.....", 6, "on the offbeats of 1 and 3, held"),
    ("x.....x.........", 6, "twice at the head of the bar, held"),
    ("x.x...x.x...x.x.", 1, "in pairs of 8ths"),
    ("......x.......x.", 2, "only on the offbeats of 2 and 4"),
]

COMP_RHYTHMS = [
    ("..x...x...x...x.", "on the offbeats"),
    ("x..x..x...x..x..", "in a dotted figure that turns round at beat 3"),
    ("..x..x....x..x..", "off the beat in dotted pairs"),
    ("x.....x.....x...", "three times a bar"),
    (".x..x..x....x...", "a 16th late, then gathering"),
    ("x.x.....x.x.....", "in pairs of 8ths on 1 and 3"),
    ("...x..x...x..x..", "pushed ahead of the beats"),
    ("x..x....x.x.....", "a dotted pair, then a straight one"),
    ("..x.x.....x.x...", "in pairs of 8ths off 1 and 3"),
    ("x.....x.x.....x.", "leaning into beat 3 and the next bar"),
    ("....x..x....x..x", "on 2 and 4, each with a 16th before the next beat"),
    ("x...x.x...x.x...", "on the one, then pairs across the beats"),
]

# A strum: 16ths between its notes, whether it goes down, whether it is struck twice a bar, what to say.
STRUMS = [
    (1, False, False, "each strummed upwards a 16th a note"),
    (1, True, False, "each strummed downwards a 16th a note"),
    (2, False, False, "each spread upwards an 8th a note"),
    (1, False, True, "each strummed upwards twice a bar"),
    (1, True, True, "each strummed downwards twice a bar"),
    (2, True, False, "each spread downwards an 8th a note"),
]

# What a stab is struck with, and when. A chord: its notes, what to call it, the tags it needs.
STAB_CHORDS = {
    "minor":     (["a3", "c4", "e4"], "A minor", []),
    "seventh":   (["a3", "c4", "e4", "g4"], "A minor seventh", []),
    "ninth":     (["a3", "c4", "e4", "g4", "b4"], "A minor ninth", []),
    "power":     (["a2", "e3", "a3"], "a bare fifth, A and E", []),
    "octaves":   (["a2", "a3", "a4"], "A in three octaves", []),
    "phrygian":  (["a3", "bb3", "e4"], "A with the flat second against it", ["Phrygian"]),
    "flattwo":   (["bb3", "d4", "f4"], "B flat major, the flat second's chord", ["Phrygian"]),
    "tritone":   (["a3", "eb4"], "the tritone, A and E flat", ["Chromatic"]),
    "diminished": (["a3", "c4", "eb4"], "A diminished", ["Chromatic"]),
    "cluster":   (["a3", "bb3", "b3"], "a three-semitone cluster on A", ["Chromatic"]),
    "sus":       (["a3", "d4", "e4"], "A with the fourth in place of the third", []),
    "quartal":   (["a3", "d4", "g4"], "fourths stacked on A", []),
    "second":    (["a3", "b3", "e4"], "A with the second in place of the third", []),
    "first":     (["c4", "e4", "a4"], "A minor with the third at the bottom", []),
    "wide":      (["a2", "e3", "c4"], "A minor spread over a tenth", []),
    "sixth":     (["f3", "a3", "c4"], "F major", []),
    "seventhflat": (["g3", "b3", "d4"], "G major", []),
    "majorseven": (["f3", "a3", "c4", "e4"], "F major seventh", []),
    "leading":   (["e3", "g#3", "b3"], "E major, the dominant", ["Harmonic minor"]),
    "minorsixth": (["a3", "c4", "f4"], "A and C under an F", []),
}

STAB_RHYTHMS = [
    ("x.....x.....x...", "on the one, the and of 2 and beat 4"),
    ("..x.....x.x.....", "on the and of 1, then beat 3 and its offbeat"),
    ("x..x..x.........", "three times in dotted 8ths from the one, then nothing"),
    ("x.x...x...x.x...", "twice in 8ths, on the and of 2, and twice more into beat 4"),
    ("...x......x.....", "twice only, a 16th before beat 2 and on the and of 3"),
    ("x..x..x..x..x...", "every dotted 8th across the bar"),
    ("..x..x..x.......", "three times in dotted 8ths off the beat"),
    ("x.......x..x....", "on 1, on 3 and a dotted 8th after it"),
    (".x..x.....x.....", "a 16th late, then on beat 2 and the and of 3"),
    ("x.x.....x.x.....", "twice in 8ths on 1 and again on 3"),
    ("....x.x.......x.", "on beat 2 and its offbeat, then the last 8th of the bar"),
    ("x..x....x..x....", "a dotted pair on 1 and again on 3"),
    ("..x...x.x.....x.", "on the offbeats of 1 and 2, on 3, and the last 8th"),
    ("x.....x..x..x.x.", "on the one, then gathering through the back half of the bar"),
    ("...x..x......x..", "pushed: a 16th before 2, the and of 2, a 16th after 4"),
    ("x...........x.x.", "on the one and twice into the next bar"),
    ("..x.x.....x.x...", "two 8ths off each of beats 1 and 3"),
    ("x....x....x....x", "every five 16ths, four times across the bar"),
]

# The chords a genre stabs with first, or one style where it differs from its genre.
STABS_FOR = {
    "Techno": ["minor", "power", "seventh", "sus", "first"],
    "Techno/Dark": ["phrygian", "flattwo", "power", "minor", "cluster"],
    "Techno/Industrial": ["tritone", "cluster", "power", "phrygian"],
    "Techno/Hard": ["power", "octaves", "tritone", "minor"],
    "Techno/Peak-time": ["minor", "first", "wide", "seventh"],
    "Techno/Melodic": ["ninth", "majorseven", "second", "sixth"],
    "Techno/Dub": ["seventh", "ninth", "minorsixth", "quartal"],
    "Techno/Acid": ["power", "phrygian", "octaves", "minor"],
    "Industrial": ["power", "octaves", "minor", "tritone", "sus"],
    "Industrial/EBM": ["power", "octaves", "minor", "seventhflat"],
    "Industrial/Electro-industrial": ["tritone", "diminished", "phrygian", "cluster"],
    "Industrial/Aggrotech": ["diminished", "power", "cluster", "phrygian"],
    "Industrial/Darkwave": ["minor", "leading", "sixth", "second"],
    "Industrial/Coldwave": ["second", "sus", "minor", "sixth"],
    "Industrial/Futurepop": ["first", "sixth", "seventhflat", "wide"],
    "Electro": ["quartal", "seventh", "octaves", "sus"],
    "Breakbeat": ["seventh", "quartal", "minor", "first"],
    "Drum and Bass": ["ninth", "diminished", "wide", "seventh"],
    "Ambient": ["wide", "second", "ninth", "sus"],
    "House": ["ninth", "seventh", "majorseven", "minorsixth"],
    "Synthwave": ["first", "sixth", "seventhflat", "leading"],
    "Trance": ["phrygian", "leading", "power", "minor"],
    "Game": ["minor", "leading", "sus", "sixth"],
    "Game/Horror": ["diminished", "cluster", "tritone", "leading"],
    "Game/Chiptune": ["first", "seventhflat", "sixth", "minor"],
    "Game/Menu": ["second", "sus", "majorseven", "sixth"],
}

BASS.update({
    "rollfive":  (". 1 1 5 . 1 1 1 . 1 1 5 . 1 1 8", "rolling 16ths in the kick's gaps, touching the fifth and ending on the octave"),
    "rollback":  (". 1 1 . . 1 1 1 . 1 1 . . 1 8 1", "rolling 16ths in the kick's gaps with a breath in the first and third"),
    "offdrop":   (". . 1 . . . 1 . . . 1 . . . b .", "an 8th on every offbeat, the last a step under the root"),
    "offpair":   (". . 1 . . . 1 1 . . 1 . . . 1 1", "offbeat 8ths, every second one doubled"),
    "gallopback": ("1 1 . 1 1 1 . 1 1 1 . 1 8 8 . 8", "the gallop turned round: two 16ths and an 8th"),
    "ebmthird":  (". 1 8 1 . 1 8 3 . 1 8 1 . 1 7 8", "16ths in the kick's gaps with the octave, touching the third and the seventh"),
    "ebmfive":   ("1 1 8 1 5 1 8 1 1 1 8 1 7 1 8 1", "driving 16ths, the octave on the offbeats and the fifth and seventh on beats 2 and 4"),
    "stomp":     ("1 . 1 . 1 1 . 1 . 1 1 . 1 . 8 .", "8ths and 16ths trading places across the bar"),
    "threes":    ("1 . . 1 . . 1 . . 1 . . 1 . 8 .", "the root every dotted 8th, the octave before the bar"),
    "threesoct": ("1 . 8 1 . 8 1 . 8 1 . 8 1 . 8 8", "root and octave in threes against the beat"),
    "pushpull":  ("1 - . . . 1 . 1 - . . . 5 . 1 .", "a held root, a push before beat 3, and the fifth before 4"),
    "upstep":    ("1 . 1 . 3 . 3 . 5 . 5 . 7 . 8 .", "8ths in pairs climbing the chord to the octave"),
    "downstep":  ("8 . 8 . 7 . 7 . 5 . 5 . 3 . 1 .", "8ths in pairs falling from the octave to the root"),
    "octjump":   ("1 . . 8 . . 1 . 8 . . 1 . 8 . .", "root and octave trading in a three-three-two figure"),
    "fifthwalk": ("1 . 5 . 8 . 5 . 1 . 5 . 7 . 5 .", "8ths walking root, fifth, octave, fifth, with the seventh once"),
    "sixteenturn": ("1 3 5 3 1 3 5 8 1 3 5 3 1 5 8 5", "16ths turning through the chord"),
    "dropout":   ("1 1 1 1 . . . . 1 1 1 1 . . 8 8", "a beat of 16ths, a beat of nothing"),
    "lazy":      ("1 - - - . . 1 . . . 1 - - - . b", "a long root, two short ones, and a step under before the bar"),
    "underturn": ("1 . b . 1 . . 1 . b 1 . 5 . . .", "the root and the step under it turning, ending on the fifth"),
    "thirdfall": ("5 - . 3 - . 1 - . . 1 . 3 . 1 .", "falling fifth, third, root in dotted 8ths, then a turn on the third"),
    "anticipate": (". . . 1 - . . 1 - . . 1 - . 1 .", "each note a 16th ahead of the beat"),
    "pedalseven": ("1 . 1 . 1 . 7 . 1 . 1 . 1 . 5 .", "8ths on the root with the seventh at the end of beat 2 and the fifth at the end of 4"),
})

SYNTH_BASS_KEYS = ["roll", "rollacc", "rollfive", "rollback", "offbeat", "offsixteen", "offdrop", "offpair", "gallop", "gallopback", "ebm", "ebmrest", "ebmthird", "ebmfive",
                   "eighths", "sixteens", "sixteensoct", "stomp", "threes", "threesoct", "pushpull", "electro", "electrob", "reese", "reesemove", "halftime", "march",
                   "psy", "synthwave", "dropout", "lazy", "underturn", "anticipate", "pedalseven"]
ARP_BASS_KEYS = ["chip", "octave8", "ebm", "electro", "upstep", "downstep", "octjump", "fifthwalk", "sixteenturn", "ebmthird", "ebmfive", "threesoct", "sixteensoct", "thirdfall"]
PICKED_KEYS = ["deepwalk", "darkwave", "rock", "eighths", "deep", "lazy", "underturn", "thirdfall", "pushpull", "fifthwalk", "pedalseven", "halftime"]

# Sub figures, a bar each. Nothing here leaves the sub register.
SUB_FIGURES = {
    "sub":      ("1 - - - - - - - - - - - - - - -", "a held sub on the root"),
    "subhalf":  ("1 - - - - - - - 1 - - - - - - -", "the sub in half notes"),
    "offbeat":  (". . 1 . . . 1 . . . 1 . . . 1 .", "an 8th on every offbeat"),
    "pulse":    ("1 . . . 1 . . . 1 . . . 1 . . .", "a quarter-note pulse on the root"),
    "kickgap":  (". 1 - - . 1 - - . 1 - - . 1 - -", "the sub held in the gap after each kick"),
    "dotted":   ("1 - - 1 - - 1 - - 1 - - 1 - 1 -", "the sub in dotted 8ths, twice as fast into the bar"),
    "backbeat": (". . . . 1 - - - . . . . 1 - - -", "the sub on beats 2 and 4 only"),
    "sync":     ("1 - - . . . 1 - . . 1 - - - . .", "a syncopated sub: the one, the and of 2, and a long note before 4"),
    "fall":     ("1 - - - - - - - b - - - - - - -", "the sub on the root, falling a step at half the bar"),
    "fifth":    ("1 - - - - - - - - - - - 5 - - -", "a held root with the fifth on beat 4"),
    "three":    ("1 - - - - - 1 - - - - - 1 - - -", "the sub every dotted quarter, three across the bar"),
    "pickup":   ("1 - - - - - - - - - - - - - 1 -", "a held sub struck again an 8th before the bar"),
    "short":    ("1 - . . . . . . 1 - . . . . . .", "a short sub on 1 and 3"),
    "tresillo": ("1 - - 1 - - 1 - 1 - - 1 - - 1 -", "the sub in a three-three-two figure stated twice"),
    "late":     (". . . . . . 1 - - - - - - - - -", "the sub coming in on the and of 2 and held"),
    "stepback": ("1 - - - b - - - 1 - - - 5 - - -", "quarter notes: root, the step under it, root, fifth"),
    "pushed":   (". . . 1 - - - - . . . 1 - - - -", "the sub pushed a 16th ahead of beats 2 and 4"),
    "twelve":   ("1 - - - - - - - - - - - 1 - 1 -", "a held sub with two 8ths at the end of the bar"),
}

# The sub figures a genre plays first, or one style where it differs from its genre.
SUBS_FOR = {
    "Techno": ["kickgap", "offbeat", "dotted", "three", "sub", "pickup"],
    "Techno/Dark": ["kickgap", "sub", "fall", "pickup", "late"],
    "Techno/Hypnotic": ["three", "dotted", "tresillo", "kickgap"],
    "Techno/Dub": ["late", "backbeat", "fifth", "sub"],
    "Techno/Minimal": ["short", "late", "sync", "backbeat"],
    "Industrial": ["pulse", "short", "sync", "stepback", "twelve"],
    "Industrial/EBM": ["pulse", "tresillo", "short", "stepback"],
    "Industrial/Darkwave": ["subhalf", "fall", "fifth", "stepback"],
    "Industrial/Coldwave": ["subhalf", "fifth", "fall", "twelve"],
    "Electro": ["sync", "pushed", "tresillo", "short"],
    "Breakbeat": ["sync", "pushed", "backbeat", "twelve"],
    "Drum and Bass": ["sync", "late", "fall", "pushed", "twelve"],
    "Ambient": ["sub", "fall", "fifth", "pickup", "late"],
    "House": ["offbeat", "pushed", "dotted", "fifth"],
    "Synthwave": ["pulse", "subhalf", "twelve", "stepback"],
    "Trance": ["kickgap", "offbeat", "three"],
    "Game": ["subhalf", "fall", "stepback", "fifth", "short"],
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

ARP_FIGURES += [
    ("0 1 2 0+ 1+ 0+ 2 1 0 1 2 0+ 1+ 0+ 2 1", "up past the octave to the third above it and back, in 16ths", "Up & Down"),
    ("0+ 2 1 0 1 2 0+ 2 1 0 1 2 0+ 2 1 0", "down from the octave first, then up and down in 16ths", "Up & Down"),
    ("0 2 0+ 2 0 2 0+ 2 0 2 0+ 2 0 2 0+ 2", "root, fifth and octave up and down, the third left out", "Up & Down"),
    ("0 0 1 1 2 2 0+ 0+ 2 2 1 1 0 0 1 1", "up and down with each note twice", "Up & Down"),
    ("0 1 2 . 0+ 2 1 . 0 1 2 . 0+ 2 1 .", "three up, a rest, three down, a rest", "Up & Down"),
    ("0 . 1 2 0+ . 2 1 0 . 1 2 0+ . 2 1", "up and down with a gap after each turn", "Up & Down"),
    ("0 1 2 0+ 1+ 2+ 1+ 0+ 2 1 0 1 2 0+ 2 1", "up through two octaves and back", "Up & Down"),
    ("2 1 0 1 2 0+ 2 1 0 1 2 0+ 1+ 0+ 2 1", "starting from the fifth, down, then up past the octave", "Up & Down"),
    ("0 1 0 2 1 0+ 2 1+ 0+ 2 1 0+ 2 1 0 1", "zigzagging: up two notes and back one, then the same down", "Up & Down"),
    ("0 1 2 1 0 1 2 0+ 2 1 2 0+ 1+ 0+ 2 1", "waves climbing a note higher each time", "Up & Down"),
    ("0 0+ 0 0+ 2 0+ 2 0+ 1 0+ 1 0+ 2 0+ 2 0+", "the octave on every other 16th, the chord's notes under it", "16th Sequences"),
    ("0 2 0 2 1 2 1 2 0 2 0 2 0+ 2 0+ 2", "a pedal on the fifth with the other notes against it", "16th Sequences"),
    ("0 1 0 2 0 0+ 0 2 0 1 0 2 0 0+ 2 1", "the root between each of the chord's notes as they climb", "16th Sequences"),
    ("0 . 0 2 . 2 1 . 1 0+ . 0+ 2 . 1 .", "pairs of notes with a gap after each, three against the beat", "16th Sequences"),
    ("0 2 1 . 0 2 1 . 0+ 2 1 . 0 2 1 .", "root, fifth, third and a rest, the octave leading the third time", "16th Sequences"),
    ("0 0 0 2 0 0 1 0 0 0 0+ 0 0 2 1 0", "mostly the root, a chord note breaking in once a beat", "16th Sequences"),
    ("0 1 2 0 1 2 0 1 2 0 1 2 0+ 2 1 0", "three notes against four beats, turning back at the octave", "16th Sequences"),
    ("0 2 0+ 1 0 2 0+ 1 0 2 0+ 1 0+ 2 1 0", "root, fifth, octave, third, round and round", "16th Sequences"),
    ("0+ 0 2 0 1 0 2 0 0+ 0 2 0 1 0 2 1", "falling to the root after every note", "16th Sequences"),
    ("0 . 2 0+ . 1 2 . 0 . 2 0+ . 1 0+ .", "a syncopated sequence with a rest every third 16th", "16th Sequences"),
    ("0 1 . 2 0+ . 2 1 . 0 1 . 2 0+ 2 .", "pairs climbing and falling with rests between", "16th Sequences"),
    ("0 . . 1 . . 2 . . 0+ . . 2 . 1 .", "dotted-8th plucks climbing to the octave", "Plucks"),
    ("0 . . . 2 . . . 1 . . . 0+ . . .", "a pluck on every beat: root, fifth, third, octave", "Plucks"),
    ("0 . 2 . . . 1 . 0+ . . . 2 . . .", "sparse plucks, five to the bar", "Plucks"),
    (". . 0 . . . 2 . . . 1 . . . 0+ .", "plucks on the offbeats only", "Plucks"),
    ("0 . 0+ . 2 . 0+ . 1 . 0+ . 2 . 0+ .", "8th plucks with the octave between each of the chord's notes", "Plucks"),
    ("0 . . 2 . 1 . . 0+ . . 2 . 1 . .", "plucks in a three-two figure", "Plucks"),
    ("0 2 . . 1 0+ . . 2 1 . . 0 2 . .", "pairs of 16th plucks on each beat", "Plucks"),
    (". 0 . 1 . 2 . 0+ . 2 . 1 . 0 . 1", "plucks a 16th after every 8th, up and down", "Plucks"),
    ("0 . 1 . 2 . 0+ . 1+ . 0+ . 2 . 1 .", "8th plucks up past the octave and back", "Plucks"),
    ("0+ . 2 . 1 . 0 . 1 . 2 . 0+ . 2 .", "8th plucks down from the octave and up again", "Plucks"),
    ("0 . . . . . 2 . 1 . . . . . 0+ .", "four plucks, wide apart", "Plucks"),
    ("0 . 0 0 . 0 . 0 0 . 0 0 . 0 . 0", "the root gated in an uneven figure", "Gated"),
    ("0 0 . 2 2 . 1 1 . 0+ 0+ . 2 2 . 1", "pairs with a gap, up the chord and back", "Gated"),
    ("0 . . 0 . . 0 . 2 . . 2 . . 1 .", "a dotted gate moving from the root to the fifth and third", "Gated"),
    ("0 0 0 0 . . . . 2 2 2 2 . . . .", "a beat on, a beat off: the root, then the fifth", "Gated"),
    ("0 0 . . 0 0 . . 1 1 . . 2 2 . .", "two 16ths on every beat, climbing through the chord in the back half", "Gated"),
    ("0 . 0 . 0 0 0 . 2 . 2 . 2 2 2 .", "two 8ths and three 16ths, on the root and then the fifth", "Gated"),
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


# ---------------------------------------------------------------------------------------------
# Risers, one written for each style: what rises is the pitch, the level, the filter or how often it
# strikes, and no two styles share the notes. A lane's 74 is the filter, 11 the level, 128 the bend
# (0.5 at rest, 1.0 two semitones up).

OPEN = lambda bars, low=0.12: [(0, low), (bars * BAR - 1, 1.0)]
SWELL = lambda bars, low=0.25: [(0, low), (bars * BAR - 1, 1.0)]
BEND_UP = lambda bars, start=0: [(0, 0.5), (start, 0.5), (bars * BAR - 1, 1.0)] if start else [(0, 0.5), (bars * BAR - 1, 1.0)]


def ramp(step, steps, low=52, high=124):
    """A velocity rising with the step."""
    return low + (high - low) * step // max(1, steps - 1)


def held(entries, bars):
    """Notes held to the end: (name, the bar it comes in at, velocity)."""
    part = Part(bars)
    for name, bar, velocity in entries:
        part.add(pitch(name), int(bar * BAR), int((bars - bar) * BAR) - 40, velocity)
    return part


def struck(bars, at, low=52, high=124, length=S // 2):
    """Notes struck: `at` gives the pitches at a step (a list, or nothing), louder as it goes."""
    part = Part(bars)
    for step in range(bars * 16):
        for p in at(step) or []:
            part.add(pitch(p) if isinstance(p, str) else p, step * S, length, ramp(step, bars * 16, low, high))
    return part


def every(gaps):
    """Whether a step is struck, where each bar has its own gap between strikes: [4, 4, 2, 1] is
    quarters for two bars, then 8ths, then 16ths."""
    return lambda step: step % gaps[min(step // 16, len(gaps) - 1)] == 0


def riser_dark():
    part = held([("a1", 0, 100), ("bb1", 2, 96)], 4).lane(74, OPEN(4, 0.05)).lane(11, SWELL(4))
    return "a low A held for four bars, the B flat above it joining at bar 3, the filter opening", part, synth("=Tar Tension"), ["Phrygian"]


def riser_driving():
    on = every([4, 2, 1, 1])
    part = struck(4, lambda i: (["a3"] if i < 48 else ["a3", "a4"]) if on(i) else None).lane(74, OPEN(4, 0.2))
    return "the root struck in quarters, then 8ths, then 16ths, the octave on top in bar 4", part, synth("stab"), []


def riser_peak():
    notes = ["a3", "c4", "e4", "g4", "b4", "c5", "e5", "a5"]
    part = held([(n, i / 2, 78 + 5 * i) for i, n in enumerate(notes)], 4).lane(74, OPEN(4, 0.2))
    return "an A minor ninth stacking a note every half bar from the root to the high A, the filter opening", part, synth("=Floodlight Ramp"), []


def riser_industrial():
    part = struck(2, lambda i: ([COWBELL] if every([4, 2])(i) else []) + ([SNARE] if i >= 24 else []) + ([KICK] if i % 4 == 0 else []), length=S // 2)
    return "metal struck in quarters then 8ths over a kick on every beat, snare 16ths through the last half bar", part, drums("machineshop"), []


def riser_hard():
    on = every([4, 4, 2, 1])
    part = struck(4, lambda i: ([KICK] if on(i) else []) + ([CLAP] if i >= 48 and i % 4 == 0 else []) + ([RIDE] if i >= 32 and i % 2 == 1 else []))
    return "the kick in quarters for two bars, 8ths in bar 3 with a ride between, 16ths in bar 4 with a clap on every beat", part, drums("bunker"), []


def riser_hypnotic():
    part = struck(4, lambda i: ["e3"] if i % 3 == 0 else None, low=40, high=118).lane(74, OPEN(4, 0.08))
    return "one E every third 16th for four bars, three against the bar, the filter opening as it gets louder", part, synth("pluck"), []


def riser_acid():
    octave = "...............8.......8.......8...8...8.8.8.8.888.88888"   # where the octave takes over, an 8th at a time
    octave = "." * (64 - len(octave)) + octave
    part = Part(4)
    for i in range(64):
        high = octave[i] == "8"
        part.add(pitch("a2" if high else "a1"), i * S, S // 2 + (SLIDE + S // 2 if i % 8 == 6 else 0), ramp(i, 64, 70, 124) if not high else 124)
    return "16ths on a low A, the octave above breaking in more and more often, a slide into every fourth beat, the resonant filter opening", part.lane(74, OPEN(4, 0.1)), synth("acid"), []


def riser_minimal():
    part = struck(2, lambda i: [CH] + ([OH] if i in (26, 30) else []) + ([RIM] if i >= 16 and i % 8 == 4 else []), low=28, high=112)
    return "closed hats in 16ths from a ghost to an accent over two bars, a rim on 2 and 4 of bar 2 and open hats on its last two offbeats", part, drums("tight"), []


def riser_melodic():
    notes = ["a2", "c3", "e3", "g3", "b3", "c4", "e4", "g4", "b4", "c5", "e5", "g5", "b5", "c6", "e6", "a6"]
    part = struck(2, lambda i: [notes[i // 2]] if i % 2 == 0 else None, low=60, high=118, length=S).lane(74, OPEN(2, 0.25))
    return "an A minor ninth arpeggio in 8ths climbing four octaves over two bars", part, synth("pluck"), []


def riser_percussive():
    part = struck(2, lambda i: [[LT, MT, MT, HT][i // 8]] if i < 24 and i % 2 == 0 or i >= 24 else None)
    part.notes += [(SHAKER, i * S, S // 2, ramp(i, 32, 40, 100)) for i in range(32)]
    return "toms in 8ths climbing from low to high over a shaker in 16ths, the high tom in 16ths through the last half bar", part, drums(), []


def riser_dub():
    chord = ["a3", "c4", "e4", "g4"]
    part = struck(4, lambda i: chord if i % 4 == 2 else None, low=60, high=112, length=S).lane(74, OPEN(4, 0.05))
    return "an A minor seventh on every offbeat for four bars, the filter opening from closed", part, synth("=Dub Chord Stab"), []


def riser_ebm():
    part = Part(2)
    for i in range(32):
        beat = i // 4
        high = i % 4 >= 4 - [0, 0, 1, 1, 2, 2, 3, 4][beat]
        part.add(pitch("a2" if high else "a1"), i * S, S // 2, 124 if high else ramp(i, 32, 76, 108))
    return "a 16th bass on A, the octave taking one more 16th of every other beat until the last beat is all octave", part.lane(74, OPEN(2, 0.2)), synth("ebm"), []


def riser_electro_industrial():
    part = struck(2, lambda i: [pitch("a2") + i // 4 + (6 if i % 4 == 2 else 0)] if i % 2 == 0 else None, low=70, high=124, length=S)
    return "8ths leaping the tritone and back, the pair a semitone higher on every beat for two bars", part, synth("=Metal Stab"), ["Chromatic"]


def riser_aggrotech():
    part = struck(4, lambda i: [pitch("a2") + i // 16, pitch("e3") + i // 16] if i % 4 != 3 else None, low=80, high=124)
    return "a gated power fifth, three 16ths and a gap, a semitone higher each bar from A to C", part.lane(74, OPEN(4, 0.3)), synth("saw"), ["Chromatic"]


def riser_techno_body():
    on = every([4, 4, 2, 1])
    part = struck(4, lambda i: ([SNARE] if on(i) else []) + ([KICK] if i % 4 == 0 else []) + ([CH] if i >= 32 and i % 4 == 2 else []))
    return "a marching snare over the kick: quarters for two bars, 8ths in bar 3 with a hat on the offbeats, 16ths in bar 4", part, drums("voltage"), []


def riser_futurepop():
    part = struck(4, lambda i: [pitch("a2") + 12 * (i // 16)], low=64, high=120).lane(74, OPEN(4, 0.2))
    return "16ths on A, an octave higher at each bar for four bars", part, synth("saw"), []


def riser_synth_ebm():
    notes = ["a2", "e3", "a3", "e4", "a4", "e5", "a5", "e6"]
    part = held([(n, i / 4, 80 + 5 * i) for i, n in enumerate(notes)], 2).lane(74, OPEN(2, 0.25))
    return "bare fifths stacking a note every beat for two bars, A and E up four octaves", part, synth("=Voltage Ladder"), []


def riser_darkwave():
    notes = ["e3", "f3", "g#3", "a3", "b3", "c4", "d4", "e4"]
    part = Part(4)
    for i, n in enumerate(notes):
        part.add(pitch(n), i * 8 * S, 8 * S - 20, 70 + 6 * i)
    return "a line in half notes climbing the harmonic minor from E to the E above, swelling", part.lane(11, SWELL(4, 0.3)), synth("pad"), ["Harmonic minor"]


def riser_industrial_rock():
    part = struck(2, lambda i: ([LT] if i < 16 and i % 2 == 0 else []) + ([SNARE] if i >= 16 else []) + ([KICK] if i % 4 == 0 else []) + ([CRASH] if i == 0 else []))
    return "a floor tom in 8ths for a bar, then a snare roll in 16ths, the kick under every beat", part, drums("break"), []


def riser_dark_electro():
    part = struck(2, lambda i: [pitch("a2") + 3 * (i // 4)], low=66, high=122).lane(74, OPEN(2, 0.2))
    return "16ths in fours, each four a minor third above the last: a diminished climb of two octaves", part, synth("pluck"), ["Chromatic"]


def riser_coldwave():
    part = struck(2, lambda i: (["e4"] + (["g4"] if i >= 16 else []) + (["b4"] if i >= 24 else [])) if i % 2 == 0 else None, low=58, high=112, length=S)
    return "8ths on E, the G above joining in bar 2 and the B in its last half", part.lane(74, OPEN(2, 0.3)), synth("chord"), []


def riser_electro():
    part = struck(2, lambda i: ([CLAP] if every([4, 1])(i) and i >= 16 or i < 16 and i % 4 == 0 else []) + ([COWBELL] if i % 2 == 0 else []))
    return "a clap on every beat of bar 1 and in 16ths through bar 2, over a cowbell in 8ths", part, drums("boom"), []


def riser_breaks():
    part = struck(2, lambda i: ([SNARE] if (i % 2 == 0 and i >= 8) or i >= 24 else []) + ([KICK] if i in (0, 6, 16, 22) else []) + ([OH] if i == 30 else []))
    return "a snare roll coming in on beat 3, 8ths then 16ths, over a broken kick, an open hat before the bar", part, drums("breaks"), []


def riser_neurofunk():
    part = held([("a1", 0, 110)], 2).lane(128, BEND_UP(2, BAR)).lane(74, OPEN(2, 0.1))
    return "a low A held for two bars, bending up two semitones through the second as the filter opens", part, synth("reese"), []


def riser_liquid():
    part = held([(n, 0, 84) for n in ("a3", "c4", "e4", "g4", "b4")], 4).lane(11, SWELL(4, 0.05)).lane(74, OPEN(4, 0.3))
    return "an A minor ninth chord swelling from nothing over four bars", part, synth("warm"), []


def riser_dark_ambient():
    part = held([("a1", 0, 96), ("bb2", 0, 80)], 4).lane(11, SWELL(4, 0.02)).lane(74, OPEN(4, 0.05))
    return "a low A and the B flat a ninth above it, swelling from nothing over four bars", part, synth("drone"), ["Phrygian"]


def riser_drone():
    part = held([("a2", 0, 96), ("a3", 4, 84)], 8).lane(74, OPEN(8, 0.05))
    return "one A held for eight bars, the octave joining at bar 5, the filter opening the whole way", part, synth("drone"), []


def riser_deep_house():
    part = struck(2, lambda i: [SHAKER] + ([OH] if i >= 16 and i % 4 == 2 else []) + ([CLAP] if i in (20, 28) else []), low=36, high=108)
    return "a shaker in 16ths rising for two bars, open hats on the offbeats of bar 2 and a clap on its 2 and 4", part, drums(), []


def riser_darksynth():
    ladder = [pitch(n) for n in ("a2", "c3", "e3", "a3", "c4", "e4", "a4", "c5", "e5", "a5", "c6")]
    part = struck(2, lambda i: [ladder[i // 4 + i % 4]], low=64, high=120).lane(74, OPEN(2, 0.25))
    return "four-note A minor arpeggios in 16ths, each starting a chord note higher than the last", part, synth("arp"), []


def riser_psytrance():
    part = struck(4, lambda i: ["a2"], low=70, high=120).lane(128, BEND_UP(4)).lane(74, OPEN(4, 0.15))
    return "16ths on one A for four bars, bending up two semitones as the filter opens", part, synth("psy"), []


def riser_combat():
    triads = [["a3", "c4", "e4"], ["b3", "d4", "f4"], ["c4", "e4", "g4"], ["d4", "f4", "a4"], ["e4", "g4", "b4"], ["f4", "a4", "c5"], ["g4", "b4", "d5"], ["a4", "c5", "e5"]]
    part = struck(2, lambda i: triads[i // 4] if i < 16 and i % 4 == 0 else triads[4 + (i - 16) // 4] if i >= 16 and i % 2 == 0 else None, low=80, high=124, length=S)
    return "triads climbing the scale from A minor: a quarter each for a bar, then each twice in 8ths up to the A minor above", part, synth("brass"), []


def riser_exploration():
    notes = ["a3", "c4", "d4", "e4", "g4", "a4", "c5", "d5", "e5", "g5", "a5", "c6"]
    part = struck(2, lambda i: [notes[i * 3 // 8]] if i % 8 in (0, 3, 6) else None, low=56, high=110, length=2 * S)
    return "a pentatonic run climbing three octaves in dotted 8ths over two bars", part, synth("bell"), []


def riser_horror():
    part = held([("a3", 0, 80), ("bb3", 1, 84), ("b3", 2, 88), ("c4", 3, 92)], 4).lane(128, BEND_UP(4, 2 * BAR)).lane(11, SWELL(4, 0.15))
    return "a cluster growing a semitone at a time, A, B flat, B and C a bar apart, all of it bending upward through the last two bars", part, synth("=Brutal Tension"), ["Chromatic"]


def riser_boss():
    on = every([4, 4, 2, 1])
    part = struck(4, lambda i: [pitch("a1") + i // 16, pitch("a2") + i // 16] if on(i) else None, low=84, high=126, length=S)
    return "low octaves a semitone higher each bar, A to C: quarters for two bars, then 8ths, then 16ths", part, synth("brass"), ["Chromatic"]


# ---------------------------------------------------------------------------------------------
# Transitions and impacts, one written for each style that has one, as the risers are.

def drum_part(bars, rows):
    """Rows of (drum, pattern): a pattern of 16 steps is a bar's, a longer one runs on across the bars."""
    part = Part(bars)
    for note, pattern in rows:
        part.grid(note, pattern)
    return part


def run(names, step=1, length=None, bars=1, velocity=100):
    """Notes one after another, `step` 16ths apart: a name, a list of names struck together, or None for a rest."""
    part = Part(bars)
    for i, n in enumerate(names):
        for name in ([] if n is None else n if isinstance(n, list) else [n]):
            part.add(pitch(name), i * step * S, (length or step) * S - 20, velocity)
    return part


def transition_dark():
    part = drum_part(2, [(KICK, "x...x...x......." + "." * 16), (CH, "o.o.o.o.o.o.o.o." * 2), (LT, "." * 16 + "....x.....x.x.xX")])
    return "the kick drops out after beat 3 of the first bar, a low tom answers in the gap and rolls into the next", part, drums("rumble"), []


def transition_driving():
    part = drum_part(2, [(KICK, "x...x...x...x..." + "x...x..........."), (OH, "." * 16 + "..x...x........."), (SNARE, "." * 16 + "........x.x.xxxX")])
    return "a bar of four to the floor, then the kick stops at beat 3 and the snare takes the last two beats, 8ths then 16ths", part, drums("hard"), []


def transition_peak():
    part = drum_part(1, [(KICK, "X..............."), (SNARE, "x.ox.ox.ox.ox.oX")])
    return "a snare roll for a bar accented in threes across the beat, a kick under the first", part, drums("hard"), []


def transition_industrial():
    part = drum_part(1, [(COWBELL, "x..x..x..x..x.x."), (KICK, "x.......x......."), (SNARE, "............xxxX")])
    return "metal in threes over two kicks, a snare 16th run through the last beat", part, drums("machineshop"), []


def transition_hard():
    part = drum_part(1, [(KICK, "x.x.x.x.xxxxxxxx"), (CLAP, "....x.......x...")])
    return "the kick in 8ths for half a bar and 16ths for the rest, the clap staying on 2 and 4", part, drums("bunker"), []


def transition_ebm():
    part = drum_part(1, [(SNARE, "X..x..x.X..x..x."), (KICK, "x.......x......."), (HT, "..............xx")])
    return "a gated snare in a dotted figure stated twice, a kick under each start, two high toms into the bar", part, drums("machine"), []


def transition_electro_industrial():
    part = Part(1)
    for step, length in ((0, 8), (8, 4), (12, 2), (14, 1)):
        part.add(pitch("a3"), step * S, length * S - 20, 104)
    part.add(pitch("eb4"), 15 * S, S - 20, 120)
    return "one note stuttering, each repeat half as long as the last, the tritone above on the last 16th", part, synth("=Metal Stab"), ["Chromatic"]


def transition_aggrotech():
    fifths = [["a2", "e3"], None, None, ["g2", "d3"], None, None, ["f2", "c3"], None, None, ["e2", "b2"], None, None, ["d2", "a2"], None, ["e2", "b2"], None]
    return "power fifths falling from A to D on every third 16th, the E under the bar's last 8th", run(fifths, length=2, velocity=112), synth("saw"), []


def transition_techno_body():
    part = drum_part(1, [(KICK, "x..x..x..x..x..."), (CH, "..x..x..x..x...."), (SNARE, "..............xX")])
    return "the kick in threes against the bar with a hat after each, two snares into the next bar", part, drums("voltage"), []


def transition_hypnotic():
    part = drum_part(1, [(CH, "x.xx.xx.xx.xx.xx"), (OH, ".o..o..o..o..o..")])
    return "hats alone for a bar, an open hat every third 16th and closed hats between", part, drums("tight"), []


def transition_acid():
    part = Part(1)
    for i, n in enumerate(["a2", "g2", "f2", "e2", "d2", "c2", "b1", "a1"]):
        part.add(pitch(n), i * 2 * S, 2 * S + (SLIDE if i < 7 else -20), 124 if i % 2 == 0 else 96)
    return "8ths sliding down the scale an octave from A to the low A as the resonant filter closes", part.lane(74, [(0, 0.9), (BAR - 1, 0.1)]), synth("acid"), []


def transition_minimal():
    part = drum_part(1, [(KICK, "x..............."), (RIM, "x.....x...x....."), (CLAP, "............x...")])
    return "one kick, three rims in the space after it and a single clap on beat 4", part, drums("tight"), []


def transition_melodic():
    notes = ["e5", "c5", "a4", "e4", "c4", "a3", "e3", "c3", "a2", "c3", "e3", "a3", "c4", "e4", "a4", "c5"]
    return "an A minor arpeggio in 16ths falling two and a half octaves and turning back up", run(notes).lane(74, [(0, 0.8), (BAR // 2, 0.3), (BAR - 1, 0.9)]), synth("pluck"), []


def transition_percussive():
    part = drum_part(1, [(HT, "x.x..x.x..x.x..x"), (MT, ".x..x..x.x..x.x."), (LT, "........x.....xX")])
    return "high and mid toms interlocking in 16ths, the low tom on beat 3 and into the bar", part, drums(), []


def transition_dub():
    chord = ["a3", "c4", "e4", "g4"]
    part = Part(2)
    for step, velocity in ((2, 108), (9, 84)):
        for n in chord:
            part.add(pitch(n), step * S, S, velocity)
    return "two A minor seventh stabs off the beat in the first bar, then a bar left empty for them to echo", part, synth("=Dub Chord Stab"), []


def transition_futurepop():
    part = drum_part(1, [(KICK, "x...x...x...x..."), (SNARE, "..x...x.x.xxxxxX"), (CRASH, "x...............")])
    return "four to the floor under a snare on the offbeats, then 8ths, then 16ths through the last beat and a half, a crash on the one", part, drums("tight"), []


def transition_synth_ebm():
    notes = ["a2", "a1", "a2", "a1", "g2", "g1", "g2", "g1", "f2", "f1", "f2", "f1", "e2", "e1", "e2", "e2"]
    return "an octave bass in 16ths stepping down from A through G and F to E", run(notes, velocity=104), synth("ebm"), []


def transition_darkwave():
    part = drum_part(1, [(HT, "x.x.x..........."), (MT, "......x.x......."), (LT, "..........x.x.xX"), (SNARE, "....x...........")])
    return "toms in 8ths from high to low across the bar, a snare on beat 2, the low tom doubling into the next", part, drums("tight"), []


def transition_industrial_rock():
    part = drum_part(1, [(CRASH, "x..............."), (KICK, "x.....x.x......."), (SNARE, "....x.....x.xxxX")])
    return "a crash and kick, a snare on 2, two kicks before 3, and a snare run from the and of 3 into the bar", part, drums("break"), []


def transition_dark_electro():
    return "a tritone falling in quarters on a bell: E flat, A, E flat, A, two octaves down", run(["eb5", "a4", "eb4", "a3"], step=4, velocity=104), synth("bell"), ["Chromatic"]


def transition_coldwave():
    return "a bass in 8ths walking up from E through G, A and B to the C", run(["e2", "e2", "g2", "g2", "a2", "a2", "b2", "c3"], step=2, velocity=102), synth("dark"), []


def transition_electro():
    part = drum_part(1, [(KICK, "x.....x........."), (COWBELL, "x.xx.x.xx.x.xx.x"), (CLAP, "............x.xX")])
    return "a cowbell figure over two kicks, the clap stuttering through the last beat", part, drums("boom"), []


def transition_breaks():
    part = drum_part(1, [(KICK, "x..x......x....."), (SNARE, "....x..x.x..xxxX"), (OH, "..............o.")])
    return "a broken kick under a snare that answers it and runs through the last beat", part, drums("breaks"), []


def transition_neurofunk():
    part = Part(1).add(pitch("a1"), 0, BAR - 40, 118).lane(128, [(0, 0.5), (BAR // 2, 0.0), (BAR - 1, 0.5)]).lane(74, [(0, 0.9), (BAR // 2, 0.15), (BAR - 1, 1.0)])
    return "a low A dipping two semitones and coming back over a bar, the filter closing and opening with it", part, synth("reese"), []


def transition_liquid():
    part = drum_part(1, [(RIDE, "x.x.x.x.x.x.x.x."), (KICK, "x.........x....."), (SNARE, "......g..g..g.gX")])
    return "a ride in 8ths over two kicks, ghost snares gathering into an accent on the last 16th", part, drums("break"), []


def transition_deep_house():
    part = Part(1)
    for start, chord in ((0, ["d3", "f3", "a3", "c4"]), (8, ["e3", "g3", "b3", "d4"])):
        for n in chord:
            part.add(pitch(n), start * S, 8 * S - 40, 88)
    return "D minor seventh for two beats, then E minor seventh, leading home", part, synth("warm"), []


def transition_darksynth():
    part = drum_part(1, [(HT, "xxxx............"), (MT, "....xxxx........"), (LT, "........xxxx...."), (KICK, "............x..."), (SNARE, "..............xX")])
    return "16ths down the toms a beat each, then a kick and two snares", part, drums("boom"), []


def transition_psytrance():
    part = Part(1)
    for i in range(12):
        part.add(pitch("a2" if i == 11 else "a1"), i * S, S // 2, 100 + (20 if i % 4 == 1 else 0))
    return "the 16th bass on A for three beats, its last note an octave up, stopping a beat early as the filter closes", part.lane(74, [(0, 0.8), (3 * TPB, 0.1)]), synth("psy"), []


TRANSITIONS = {
    "Techno/Dark": transition_dark, "Techno/Driving": transition_driving, "Techno/Peak-time": transition_peak, "Techno/Industrial": transition_industrial,
    "Techno/Hard": transition_hard, "Techno/Hypnotic": transition_hypnotic, "Techno/Acid": transition_acid, "Techno/Minimal": transition_minimal,
    "Techno/Melodic": transition_melodic, "Techno/Percussive": transition_percussive, "Techno/Dub": transition_dub,
    "Industrial/EBM": transition_ebm, "Industrial/Electro-industrial": transition_electro_industrial, "Industrial/Aggrotech": transition_aggrotech,
    "Industrial/Techno-body": transition_techno_body, "Industrial/Futurepop": transition_futurepop, "Industrial/Synth-EBM": transition_synth_ebm,
    "Industrial/Darkwave": transition_darkwave, "Industrial/Industrial rock": transition_industrial_rock, "Industrial/Dark electro": transition_dark_electro,
    "Industrial/Coldwave": transition_coldwave, "Electro/Electro": transition_electro, "Breakbeat/Breaks": transition_breaks,
    "Drum and Bass/Neurofunk": transition_neurofunk, "Drum and Bass/Liquid": transition_liquid, "House/Deep house": transition_deep_house,
    "Synthwave/Darksynth": transition_darksynth, "Trance/Psytrance": transition_psytrance,
}


def impact_dark():
    part = Part(1).add(pitch("a0"), 0, BAR - 40, 127).add(pitch("e1"), 0, BAR - 40, 112).lane(128, [(0, 0.5), (BAR - 1, 0.2)])
    return "a low fifth, A and E, struck and sinking", part, synth("=Low Boom"), []


def impact_driving():
    return "kick, clap and crash together on the one", drum_part(1, [(KICK, "X"), (CLAP, "X"), (CRASH, "x")]), drums("hard"), []


def impact_peak():
    part = Part(1)
    for n in ("a1", "a2", "e3", "a3", "c4", "e4"):
        part.add(pitch(n), 0, BAR - 40, 118)
    return "A minor struck across three octaves and its filter closing over the bar", part.lane(74, [(0, 1.0), (BAR - 1, 0.1)]), synth("stab"), []


def impact_industrial():
    part = drum_part(1, [(KICK, "X.o............."), (COWBELL, "X.o............."), (SNARE, "X...............")])
    return "kick, metal and snare on the one, the kick and metal again softer an 8th later", part, drums("machineshop"), []


def impact_hard():
    part = drum_part(1, [(KICK, "Xxx............."), (CRASH, "X...............")])
    return "three kicks in 16ths from the one under a crash", part, drums("bunker"), []


def impact_ebm():
    return "kick, snare and clap together on the one", drum_part(1, [(KICK, "X"), (SNARE, "X"), (CLAP, "x")]), drums("machine"), []


def impact_electro_industrial():
    part = Part(1).add(pitch("a1"), 0, BAR - 40, 124).add(pitch("eb2"), 0, BAR - 40, 112).add(pitch("a2"), 0, BAR - 40, 100)
    return "a low tritone inside an octave, A, E flat and A, struck and left to ring", part, synth("=Factory Hit"), ["Chromatic"]


def impact_aggrotech():
    part = Part(1).add(pitch("a1"), 0, 8 * S, 127).add(pitch("a2"), 0, 8 * S, 120).add(pitch("bb2"), 0, 8 * S, 116).add(pitch("e3"), 0, 8 * S, 108)
    return "octave A with the B flat and the fifth above it, a distorted cluster cut off at half the bar", part, synth("saw"), ["Phrygian"]


def impact_techno_body():
    return "kick, low tom and cowbell on the one, the tom again on the next 16th", drum_part(1, [(KICK, "X"), (LT, "Xx"), (COWBELL, "x")]), drums("voltage"), []


IMPACTS = {
    "Techno/Dark": impact_dark, "Techno/Driving": impact_driving, "Techno/Peak-time": impact_peak, "Techno/Industrial": impact_industrial,
    "Techno/Hard": impact_hard, "Industrial/EBM": impact_ebm, "Industrial/Electro-industrial": impact_electro_industrial,
    "Industrial/Aggrotech": impact_aggrotech, "Industrial/Techno-body": impact_techno_body,
}

LEFT_OUT = []   # (style, name) of each preset not written

# Sameness, as the tests measure it: what a preset plays against the others of its category. The same
# notes are the same pitches (a drum part's sounds) at the same places; the same shape is the same
# rhythm with each note stepping up, down or staying as the other's does, whatever the chord.
PLAYED = {}       # category -> the notes of each preset written
SHAPES = {}       # (category, shape) -> the styles that have it
SHAPE_STYLES = 3  # a shape may be in this many styles


def notes_of(part):
    return tuple(sorted((n[1], n[0]) for n in part.notes))


def shape_of(part, on_drums):
    if on_drums:
        return notes_of(part)

    shape, last = [], None
    for start, p in notes_of(part):
        shape.append((start, 0 if last is None or p == last else 1 if p > last else -1))
        last = p
    return tuple(shape)


def is_new(category, style, part, on_drums):
    """Whether a part plays what no preset of its category does, in a shape not yet in three other styles."""
    styles = SHAPES.get((category, shape_of(part, on_drums)), set())
    return notes_of(part) not in PLAYED.get(category, set()) and (style in styles or len(styles) < SHAPE_STYLES)


def remember(category, style, part, on_drums):
    PLAYED.setdefault(category, set()).add(notes_of(part))
    SHAPES.setdefault((category, shape_of(part, on_drums)), set()).add(style)

DRAWN = {}   # how often each UI sound and stinger has been drawn, over every style


RISERS = {
    "Techno/Dark": riser_dark, "Techno/Driving": riser_driving, "Techno/Peak-time": riser_peak, "Techno/Industrial": riser_industrial,
    "Techno/Hard": riser_hard, "Techno/Hypnotic": riser_hypnotic, "Techno/Acid": riser_acid, "Techno/Minimal": riser_minimal,
    "Techno/Melodic": riser_melodic, "Techno/Percussive": riser_percussive, "Techno/Dub": riser_dub,
    "Industrial/EBM": riser_ebm, "Industrial/Electro-industrial": riser_electro_industrial, "Industrial/Aggrotech": riser_aggrotech,
    "Industrial/Techno-body": riser_techno_body, "Industrial/Futurepop": riser_futurepop, "Industrial/Synth-EBM": riser_synth_ebm,
    "Industrial/Darkwave": riser_darkwave, "Industrial/Industrial rock": riser_industrial_rock, "Industrial/Dark electro": riser_dark_electro,
    "Industrial/Coldwave": riser_coldwave, "Electro/Electro": riser_electro, "Breakbeat/Breaks": riser_breaks,
    "Drum and Bass/Neurofunk": riser_neurofunk, "Drum and Bass/Liquid": riser_liquid, "Ambient/Dark ambient": riser_dark_ambient,
    "Ambient/Drone": riser_drone, "House/Deep house": riser_deep_house, "Synthwave/Darksynth": riser_darksynth, "Trance/Psytrance": riser_psytrance,
    "Game/Combat": riser_combat, "Game/Exploration": riser_exploration, "Game/Horror": riser_horror, "Game/Boss": riser_boss,
}


# ---------------------------------------------------------------------------------------------
# A style's language: what makes its arps, its bass figures, its comping and its fills its own.
#   density  how full a bar is, 0 (a few notes) to 1 (every 16th)
#   wide     how far a figure travels, 0 (round one note) to 1 (through the octaves)
#   off      how far off the beat it sits, 0 (on the beats) to 1 (between them)
#   cross    whether it likes threes against the bar's four
#   register where its arps sit: the lowest root
#   colours  what its arps and comping are made of, in turn (see COLOURS)
# A style with no line of its own speaks its genre's.

def lang(density, wide, off, cross, register, colours):
    return dict(density=density, wide=wide, off=off, cross=cross, register=register, colours=colours)


LANGUAGE = {
    "Techno": lang(0.7, 0.4, 0.4, False, 57, ["triad", "open"]),
    "Techno/Dark": lang(0.6, 0.2, 0.5, True, 45, ["phrygian", "open", "triad"]),
    "Techno/Driving": lang(0.9, 0.4, 0.3, False, 57, ["triad", "open", "seventh"]),
    "Techno/Peak-time": lang(1.0, 0.7, 0.3, False, 57, ["triad", "seventh", "open"]),
    "Techno/Industrial": lang(0.8, 0.3, 0.6, True, 45, ["tritone", "open", "phrygian"]),
    "Techno/Hard": lang(1.0, 0.3, 0.4, True, 45, ["open", "tritone"]),
    "Techno/Hypnotic": lang(0.6, 0.2, 0.6, True, 57, ["open", "sus"]),
    "Techno/Acid": lang(0.8, 0.5, 0.6, False, 45, ["phrygian", "open"]),
    "Techno/Minimal": lang(0.3, 0.2, 0.8, False, 57, ["sus", "seventh"]),
    "Techno/Melodic": lang(0.9, 0.9, 0.3, False, 57, ["ninth", "seventh", "triad"]),
    "Techno/Percussive": lang(0.7, 0.3, 0.7, True, 57, ["open", "sus"]),
    "Techno/Dub": lang(0.3, 0.4, 0.9, False, 57, ["seventh", "ninth"]),
    "Industrial": lang(0.8, 0.3, 0.4, False, 45, ["open", "triad"]),
    "Industrial/EBM": lang(1.0, 0.3, 0.3, False, 45, ["open", "triad"]),
    "Industrial/Electro-industrial": lang(0.7, 0.4, 0.7, True, 52, ["diminished", "tritone", "phrygian"]),
    "Industrial/Aggrotech": lang(1.0, 0.4, 0.4, False, 52, ["diminished", "open"]),
    "Industrial/Techno-body": lang(0.9, 0.3, 0.4, True, 45, ["open", "triad"]),
    "Industrial/Futurepop": lang(0.9, 0.8, 0.3, False, 64, ["triad", "seventh"]),
    "Industrial/Synth-EBM": lang(0.8, 0.6, 0.3, False, 57, ["triad", "open"]),
    "Industrial/Darkwave": lang(0.5, 0.6, 0.4, False, 57, ["triad", "sus"]),
    "Industrial/Industrial rock": lang(0.7, 0.3, 0.3, False, 45, ["open", "phrygian"]),
    "Industrial/Dark electro": lang(0.7, 0.5, 0.5, False, 52, ["diminished", "phrygian"]),
    "Industrial/Coldwave": lang(0.5, 0.5, 0.5, False, 57, ["sus", "seventh"]),
    "Electro": lang(0.6, 0.5, 0.8, False, 57, ["quartal", "seventh"]),
    "Breakbeat": lang(0.6, 0.5, 0.8, False, 57, ["seventh", "quartal"]),
    "Drum and Bass": lang(0.6, 0.7, 0.7, False, 57, ["ninth", "seventh"]),
    "Drum and Bass/Neurofunk": lang(0.7, 0.4, 0.8, True, 45, ["diminished", "tritone"]),
    "Ambient": lang(0.2, 0.7, 0.5, False, 57, ["open", "sus", "ninth"]),
    "Ambient/Dark ambient": lang(0.2, 0.6, 0.6, False, 45, ["phrygian", "tritone", "open"]),
    "House": lang(0.5, 0.6, 0.8, False, 57, ["ninth", "seventh"]),
    "Synthwave": lang(0.9, 0.8, 0.2, False, 57, ["triad", "seventh"]),
    "Trance": lang(1.0, 0.5, 0.4, True, 57, ["phrygian", "open"]),
    "Game": lang(0.6, 0.7, 0.4, False, 57, ["triad", "sus"]),
    "Game/Combat": lang(0.9, 0.5, 0.3, False, 52, ["open", "triad"]),
    "Game/Exploration": lang(0.4, 0.9, 0.4, False, 64, ["sus", "ninth"]),
    "Game/Menu": lang(0.4, 0.6, 0.5, False, 64, ["sus", "triad"]),
    "Game/Chiptune": lang(1.0, 1.0, 0.2, False, 64, ["triad", "seventh"]),
    "Game/Horror": lang(0.3, 0.8, 0.8, False, 52, ["diminished", "tritone", "phrygian"]),
    "Game/Boss": lang(0.9, 0.5, 0.4, True, 45, ["diminished", "open"]),
}

# What a chord is made of on a root, in semitones (a name: the scale step above the root, in the mode),
# and how to say it.
COLOURS = {
    "triad":      (["third", "fifth"], "the triads"),
    "open":       ([7, 12], "root, fifth and octave, no third"),
    "phrygian":   ([1, 7], "the root, the semitone above it and the fifth"),
    "tritone":    ([6, 12], "the root, its tritone and the octave"),
    "diminished": ([3, 6], "diminished triads"),
    "sus":        ([5, 7], "the fourth in place of the third"),
    "quartal":    ([5, 10], "fourths stacked on the root"),
    "seventh":    (["third", "seventh"], "the root, the third and the seventh"),
    "ninth":      (["fifth", "ninth"], "the root, the fifth and the ninth"),
}


def language_of(style):
    return LANGUAGE.get(style) or LANGUAGE[style.split("/")[0]]


def coloured(mode, degree, low, colour):
    """A chord of three notes on the mode's degree in a colour, as pitches from low up."""
    root = degree_pitch(mode, degree, low, low + 11)
    i = ladder_index(mode, root)
    step = {"third": 2, "fifth": 4, "seventh": 6, "ninth": 8}
    return [root] + [LADDER[mode][i + step[x]] if isinstance(x, str) else root + x for x in COLOURS[colour][0]]


def tags_for(pitches):
    """The tags a part's notes need: Phrygian's B flat, harmonic minor's G sharp, anything else chromatic."""
    pcs = {p % 12 for p in pitches} - {9, 11, 0, 2, 4, 5, 7}
    return (["Phrygian"] if 10 in pcs else []) + (["Harmonic minor"] if 8 in pcs else []) + (["Chromatic"] if pcs - {10, 8} else [])


def traits(tokens):
    """What a figure of 16 steps does, from its tokens: how full, how wide, how far off the beat, whether
    it runs in threes. A token is a rest ('.'), a hold ('-') or a note; `wide` counts the different notes."""
    notes = [(i, t.rstrip("!~_")) for i, t in enumerate(tokens) if t not in (".", "-")]
    starts = [i for i, _ in notes]
    density = len(notes) / 16
    wide = min(1.0, (len({t for _, t in notes}) - 1) / 4)
    off = sum(1 for i in starts if i % 4 != 0) / max(1, len(starts)) / 0.75
    threes = sum(1 for a, b in zip(starts, starts[1:]) if b - a == 3) >= 3 or sum(1 for i in range(13) if tokens[i] == tokens[i + 3] != ".") >= 8
    return density, wide, min(1.0, off), threes


def fits(tokens, language):
    """How well a figure fits a style's language: higher is nearer."""
    density, wide, off, threes = traits(tokens)
    return -(abs(density - language["density"]) + abs(wide - language["wide"]) + 0.7 * abs(off - language["off"])) + (0.4 if threes and language["cross"] else 0.0)


# The kits made from sources, for the styles they were made for: every second groove of the style plays one.
KIT_MOVES = {
    "Techno/Dark": "bunker", "Techno/Hard": "bunker",
    "Techno/Industrial": "machineshop", "Industrial/Electro-industrial": "machineshop", "Industrial/Aggrotech": "machineshop",
    "Industrial/EBM": "voltage", "Industrial/Techno-body": "voltage", "Industrial/Synth-EBM": "voltage",
}


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
        remember(role + " > " + sub, self.s.style, part, voice[0] == "drums")
        self.made.setdefault(kind, []).append(name)

    def leave_out(self, nouns, category="SFX"):
        """A preset that is not written, since it would only play what another does. Its name and its
        energy are drawn as if it were, so the presets after it keep their names."""
        LEFT_OUT.append((self.s.style, category, self.name(nouns)))
        self.energy()

    def add_new(self, kind, nouns, role, sub, candidates, **more):
        """Writes the first of the candidates (description, part, voice, tags) that plays what no preset
        of the category plays yet, in a shape not yet in three other styles; none of them, no preset."""
        category = role + " > " + sub

        for description, part, voice, tags in candidates:
            if is_new(category, self.s.style, part, voice[0] == "drums"):
                self.add(kind, nouns, role, sub, description, part, voice, tags, **more)
                return True

        self.leave_out(nouns, category)

        if not more.get("tempo"):
            self.tempo()   # a preset draws its tempo when it is written: drawn here too, for the names after it

        return False

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

        # A style the source kits were made for puts every second groove on one.
        if sub == "Full Grooves" and s.style in KIT_MOVES:
            self.grooves_made = getattr(self, "grooves_made", 0) + 1
            if self.grooves_made % 2 == 0:
                sound = drums(KIT_MOVES[s.style])

        if sub == "Full Grooves":
            bars = self.pick([1, 2, 4])
            part, said = self.groove(bars)
            if bars == 4:
                for n, p in TOMRUN:
                    part.grid(n, p, bar=3)
                said.append("a tom run ends bar 4")
            if not said:
                return False
            self.add_new("groove", DRUM_NOUNS[sub], "Drums", sub, [(join(said), part, sound, [])])
        elif sub == "Kick":
            drawn = self.pick(s.kicks)
            candidates = []

            # The kick drawn, then the style's others, then the rest; each with the ways into the next bar in turn.
            for key in [drawn] + [k for k in s.kicks if k != drawn] + [k for k in KICKS if k not in s.kicks]:
                pieces, words = KICKS[key]

                for tail, how in KICK_TAILS:
                    part = Part(2)
                    put(part, pieces)
                    part.grid(KICK, tail, bar=1)
                    candidates.append((words + ", " + how, part, sound, []))

            self.add_new("kick", DRUM_NOUNS[sub], "Drums", sub, candidates)
        elif sub == "Snare & Clap":
            mine = [b for b in s.backbeats if b != "none"] or ["rim24"]
            drawn = self.pick(mine)
            candidates = []

            for key in [drawn] + [k for k in mine if k != drawn] + [k for k in BACKBEATS if k not in mine and k != "none"]:
                pieces, words = BACKBEATS[key]

                for drum, tail, how in BACKBEAT_TAILS:
                    part = Part(2)
                    put(part, pieces)
                    part.grid(drum, tail, bar=1)
                    candidates.append((words + ", " + how, part, sound, []))

            self.add_new("snare", DRUM_NOUNS[sub], "Drums", sub, candidates)
        elif sub == "Hats & Tops":
            mine = [h for h in s.hats if h != "none"] or ["sparse"]
            one, two = self.pick(mine), self.pick(["ride", "pedal", "offopen", "eighths"])
            candidates = []

            for a in [one] + [h for h in mine if h != one] + [h for h in HATS if h not in mine and h != "none"]:
                for b in [two] + [h for h in ("ride", "pedal", "offopen", "eighths", "rideoff", "shuffle", "offclosed", "sparse") if h != two]:
                    first, second = HATS[a], HATS[b]
                    part = Part(2)
                    put(part, first[0])

                    if first[1] != second[1]:
                        put(part, [(n, p) for n, p in second[0]], bar=1)
                        said = f"{first[1]}, with {second[1]} added in bar 2"
                    else:
                        said = first[1]

                    candidates.append((said, part, sound, []))

            self.add_new("hats", DRUM_NOUNS[sub], "Drums", sub, candidates)
        elif sub == "Percussion":
            choices = [p for p in s.percs if p not in ("none", "crash")] or ["rim3"]
            one, two = self.pick(choices), self.pick(["conga", "rimsync", "shaker", "cowbell", "toms"])
            candidates = []

            for x in [one] + [p for p in choices if p != one] + [p for p in PERCS if p not in choices and p not in ("none", "crash")]:
                for y in [two] + [p for p in ("conga", "rimsync", "shaker", "cowbell", "toms", "metal") if p != two]:
                    a, b = PERCS[x], PERCS[y]
                    part = Part(4)
                    put(part, a[0])
                    for n_p in b[0]:
                        if n_p[0] != "cycle":
                            part.grid(n_p[0], n_p[1], bar=2).grid(n_p[0], n_p[1], bar=3)
                    candidates.append((a[1] if a[1] == b[1] else f"{a[1]}, with {b[1]} joining in bars 3 and 4", part, sound, []))

            self.add_new("perc", DRUM_NOUNS[sub], "Drums", sub, candidates)
        elif sub == "Fills":
            kind = self.pick(["snare", "toms", "clap"])
            drawn = self.pick(s.kicks)
            start = zlib.crc32(s.style.encode("utf-8"))
            bodies = FILLS[kind]
            candidates = []

            # The first of its kind as it was, then the others from a place of the style's own; under each
            # the kick drawn, then the style's others.
            dense = lambda body: sum(1 for _, pattern in body for c in pattern if c != ".") / 8
            for body, words in sorted(bodies, key=lambda b: abs(dense(b[0]) - language_of(s.style)["density"])):
                for key in [drawn] + [k for k in s.kicks if k != drawn] + [k for k in KICKS if k not in s.kicks]:
                    part = Part(1)
                    put(part, [(n, p[:8] + "........") for n, p in KICKS[key][0] if isinstance(n, int)])
                    for n, p in body:
                        part.grid(n, p)
                    candidates.append((words, part, sound, []))

            self.add_new("fill", DRUM_NOUNS[{"snare": "snare fill", "toms": "tom fill", "clap": "clap fill"}[kind]], "Drums", sub, candidates)
        elif sub == "Breaks":
            part = Part(2)
            pattern = self.pick(BREAKS[:3])   # one of the three there were
            hats = self.pick(["eighths", "sixteen", "shuffle"])
            voice = drums(self.pick(["break", s.drums_sound[0]]))
            candidates = []

            for kick, snare, words in [pattern] + [b for b in BREAKS if b != pattern]:
                for h in [hats] + [x for x in ("eighths", "sixteen", "shuffle", "ride", "dnb") if x != hats]:
                    part = Part(2)
                    part.grid(KICK, kick).grid(SNARE, snare)
                    put(part, HATS[h][0])
                    candidates.append((words + ", under " + HATS[h][1], part, voice, []))

            self.add_new("break", DRUM_NOUNS[sub], "Drums", sub, candidates)
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
            bars = self.pick([2, 4]) if len(set(degrees)) > 1 else self.pick([1, 2, 4])
            return self.sub_bass(mode, degrees, key, bars)
        elif sub == "Arp Bass":
            key = "acid" if s.acid else self.pick(["chip", "octave8", "ebm", "electro"] if not s.game else ["chip", "octave8"])
            patch = "acid" if s.acid else self.pick(s.bass_patches)
        elif sub == "Picked & Played":
            key = self.pick(["deepwalk", "darkwave", "rock", "eighths", "deep"])
            patch = self.pick(["pluck", "dark", "organ"] if not s.game else s.bass_patches)
        else:
            key = self.pick(s.bass)
            patch = self.pick(s.bass_patches)

        acid = (self.acid_rhythm(), "a 303 line with accents and slides") if key == "acid" else None
        bars = self.pick([2, 4]) if len(set(degrees)) > 1 else self.pick([1, 2, 4])

        # The rhythm drawn first, then the style's own, then the rest of its kind; as long as drawn, then the others.
        nearest = lambda keys: sorted(keys, key=lambda k: -fits(BASS[k][0].split(), language_of(s.style)))

        if sub == "Synth Bass":
            keys = [key] + [k for k in s.bass if k != key] + nearest([k for k in SYNTH_BASS_KEYS if k not in s.bass and k != key])
        elif key == "acid":
            keys = [key]
        else:
            keys = nearest(ARP_BASS_KEYS if sub == "Arp Bass" else PICKED_KEYS)
        candidates = []

        for k in keys:
            rhythm, words = acid if k == "acid" else BASS[k]

            for length in [bars] + [b for b in (4, 2, 1) if b != bars]:
                moving = length == 4 and len(set(degrees)) > 1
                part = Part(length)

                for b in range(length):
                    part.line(self.bass_bar(mode, rhythm, degrees[b] if moving else 0), gate=0.9 if k in ("sub", "subhalf", "drone", "reese", "reesemove") else 0.6, bar=b, repeat=False)

                said = words + ((", moving through the roots of " + chord_names(mode, degrees)) if moving else ", on A")
                candidates.append((said, part, synth(patch), self.mode_tags(mode, [n[0] for n in part.notes])))

        self.add_new("bass", ROLE_NOUNS[sub], "Bass", sub, candidates)

    def sub_bass(self, mode, degrees, drawn, bars):
        """A sub: the figures its kind of style plays first (the one drawn before them where it is one of
        those), on the root and through the style's roots, then the rest, until one is new."""
        s = self.s
        mine = SUBS_FOR.get(s.style) or SUBS_FOR[s.style.split("/")[0]]
        keys = ([drawn] if drawn in mine else []) + [k for k in mine if k != drawn] + [k for k in SUB_FIGURES if k not in mine]
        candidates = []

        for key in keys:
            rhythm, words = SUB_FIGURES[key]

            for moving in ([False, True] if bars < 4 or len(set(degrees)) == 1 else [True, False]):
                length = 4 if moving else min(bars, 2)
                part = Part(length)

                for b in range(length):
                    part.line(self.sub_bar(mode, rhythm, degrees[b] if moving else 0), gate=0.95, bar=b, repeat=False)

                said = words + (", moving through the roots of " + chord_names(mode, degrees) if moving else ", on A")
                candidates.append((said, part, synth("sub"), self.mode_tags(mode, [n[0] for n in part.notes])))

        self.add_new("bass", ROLE_NOUNS["Sub"], "Bass", "Sub", candidates)

    def sub_bar(self, mode, rhythm, degree):
        """A bar of a sub figure on a degree, kept at or under A2: '1' the root, '5' the fifth above it
        (below, where above would leave the sub register), 'b' the step under the root."""
        root = degree_pitch(mode, degree, 28, 39)
        i = ladder_index(mode, root)
        fifth = LADDER[mode][i + 4] if LADDER[mode][i + 4] <= 45 else LADDER[mode][i - 3]
        tokens = []

        for t in rhythm.split():
            tokens.append(t if t in (".", "-") else name_of({"1": root, "5": fifth, "b": LADDER[mode][i - 1]}[t]))

        return " ".join(tokens)

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

        progressions = [(degrees, mode)] + [PROGRESSIONS[p] for p in s.progressions if PROGRESSIONS[p] != (degrees, mode)]
        start = zlib.crc32((s.style + sub).encode("utf-8"))

        if sub == "Progressions":
            drawn = self.pick([0, 1, 2, 3])   # one of the four there were
            order = [drawn] + [(start + i) % len(CHORD_RHYTHMS) for i in range(len(CHORD_RHYTHMS)) if (start + i) % len(CHORD_RHYTHMS) != drawn]
            candidates = []

            for i in order:
                rhythm, steps, how = CHORD_RHYTHMS[i]

                for d, m in progressions:
                    for with_seventh in (seventh, not seventh):
                        chords = [names(triad(m, x, seventh=with_seventh)) for x in d]
                        part = Part(4)
                        part.chords(chords, velocity=84) if rhythm is None else part.chords(chords, rhythm=rhythm, length_steps=steps, velocity=86)
                        candidates.append((f"{chord_names(m, d, with_seventh)}, {how}", part, synth(patch), self.mode_tags(m, [n[0] for n in part.notes])))

            return self.add_new("harmony", ROLE_NOUNS[sub], "Harmony", sub, candidates)
        elif sub == "Keys & Comping":
            self.pick([0, 1, 2, 3])   # drawn as before
            order = sorted(range(len(COMP_RHYTHMS)), key=lambda i: -fits(list(COMP_RHYTHMS[i][0]), language_of(s.style)))
            candidates = []

            for i in order:
                rhythm, how = COMP_RHYTHMS[i]

                for d, m in progressions:
                    for colour in language_of(s.style)["colours"]:
                        chords = [names(coloured(m, x, 52, colour)) for x in d]
                        part = Part(2)
                        part.chords(chords[:2], rhythm=rhythm, length_steps=1, velocity=82)
                        candidates.append((f"short chords of {COLOURS[colour][1]} on {root_names(m, d[:2])}, comping over two bars, {how}", part, synth(patch),
                                           tags_for([n[0] for n in part.notes])))

            return self.add_new("harmony", ROLE_NOUNS[sub], "Harmony", sub, candidates)
        elif sub == "Stabs":
            self.pick([0, 1, 2, 3, 4])   # the rhythm, drawn as before so the names after it stay
            patch = self.pick(["stab"] + s.chord_patches)
            return self.stabs(patch)
        else:   # Strums: the chord's notes one after another, each held to the bar's end
            candidates = []

            for apart, down, twice, how in STRUMS[start % len(STRUMS):] + STRUMS[:start % len(STRUMS)]:
                for d, m in progressions:
                    for with_seventh in (seventh, not seventh):
                        part = Part(4)

                        for b, x in enumerate(d):
                            ch = triad(m, x, seventh=with_seventh)
                            ch = ch[::-1] if down else ch

                            for at in ([0, 8] if twice else [0]):
                                end = 8 if twice and at == 0 else 16
                                for k, p in enumerate(ch):
                                    part.add(p, b * BAR + (at + k * apart) * S, (end - at - k * apart) * S - 40, 90 - 6 * k)

                        candidates.append((f"{chord_names(m, d, with_seventh)}, {how}", part, synth(patch), self.mode_tags(m, [n[0] for n in part.notes])))

            return self.add_new("harmony", ROLE_NOUNS[sub], "Harmony", sub, candidates)

    def stabs(self, patch):
        """Stabs: the chords its kind of style stabs with, each in the rhythms it favours, until one is new."""
        s = self.s
        chords = STABS_FOR.get(s.style) or STABS_FOR[s.style.split("/")[0]]
        start = zlib.crc32(s.style.encode("utf-8"))
        candidates = []

        for turn in range(len(STAB_RHYTHMS)):
            for c, key in enumerate(chords):
                notes, said, tags = STAB_CHORDS[key]
                rhythm, how = STAB_RHYTHMS[(start + turn + 5 * c) % len(STAB_RHYTHMS)]
                part = Part(1)
                part.chords([notes], rhythm=rhythm, length_steps=1, velocity=104)
                candidates.append((f"{said} {how}", part, synth(patch), tags))

        self.add_new("harmony", ROLE_NOUNS["Stabs"], "Harmony", "Stabs", candidates)

    def pads(self, sub):
        s = self.s
        degrees, mode = self.progression()
        seventh = self.r.random() < 0.4
        patch = self.pick(s.pad_patches)
        start = zlib.crc32((s.style + sub).encode("utf-8"))

        if sub == "Drones":
            bars = self.pick([4, 8])
            patch = self.pick(["drone", "noise"] if "noise" in s.pad_patches else ["drone"])
            mine = DRONES_FOR.get(s.style) or DRONES_FOR[s.style.split("/")[0]]
            candidates = []

            for key in mine + [k for k in DRONE_FIGURES if k not in mine]:
                notes, said, tags = DRONE_FIGURES[key]

                for length in (bars, 12 - bars):
                    part = Part(length)
                    for name, enters, leaves in notes:
                        part.add(pitch(name), int(enters * length) * BAR, int((leaves - enters) * length) * BAR - 40, 92 if enters == 0 else 78)
                    if any(n[2] < 2 * BAR - 40 for n in part.notes):
                        continue   # a drone holds: nothing under two bars
                    breath, how = DRONE_LANES[(start + len(candidates)) % len(DRONE_LANES)]
                    part.lane(74, [(int(t * length * BAR) - (1 if t == 1 else 0), v) for t, v in breath])
                    candidates.append((f"{said}, for {length} bars, {how}", part, synth(patch), tags))

            # A style with a drone written for it plays that. The figure it would have drawn is kept in mind
            # all the same, so the styles after it draw what they drew before.
            if s.style in DRONES_WRITTEN:
                for _, part, voice, _ in candidates:
                    if is_new("Pads > Drones", s.style, part, False):
                        remember("Pads > Drones", s.style, part, False)
                        break

                candidates = [DRONES_WRITTEN[s.style][0]("=" + DRONES_WRITTEN[s.style][1])]

            return self.add_new("pad", ROLE_NOUNS[sub], "Pads", sub, candidates)

        # The style's own progression first, then its others; the chords as drawn, then with or without the seventh.
        progressions = [(degrees, mode)] + [PROGRESSIONS[p] for p in s.progressions if PROGRESSIONS[p] != (degrees, mode)]
        mine = PADS_FOR.get(s.style) or PADS_FOR[s.style.split("/")[0]]
        candidates = []

        for degrees, mode in progressions:
            for with_seventh in (seventh, not seventh):
                chords = [triad(mode, d, seventh=with_seventh) for d in degrees]
                said = chord_names(mode, degrees, with_seventh)

                for key in mine + [k for k in PAD_TEXTURES if k not in mine]:
                    make, how = PAD_TEXTURES[key]

                    if sub == "Swells":
                        part = make(chords[:1] * 2, 2).lane(11, [(0, 0.0), (2 * BAR - 1, 1.0)])
                        words = f"{chord_name(mode, degrees[0], with_seventh)} swelling up from silence over two bars, {how}"
                    elif sub == "Evolving":
                        lane, moving = PAD_LANES[(start + len(candidates)) % len(PAD_LANES)]
                        part = make(chords, 4).lane(74, [(int(t * 4 * BAR) - (1 if t == 1 else 0), v) for t, v in lane])
                        words = f"{said}, {how}, {moving}"
                    else:
                        part = make(chords, 4)
                        words = f"{said}, {how}"

                    candidates.append((words, part, synth(patch), self.mode_tags(mode, [n[0] for n in part.notes])))

        self.add_new("pad", ROLE_NOUNS[sub], "Pads", sub, candidates)

    # -- Arps and melody --------------------------------------------------------------------

    def arps(self, sub):
        s = self.s
        degrees, mode = self.progression()
        figures = [f for f in ARP_FIGURES if f[2] == sub]
        drawn = self.pick(figures[:{"Up & Down": 2, "16th Sequences": 3, "Plucks": 3, "Gated": 3}[sub]])   # drawn from as many as there were
        octave_low = self.pick([52, 57, 64])
        bars = self.pick([1, 2, 4])
        patch = self.pick(s.arp_patches)

        # The style's language chooses: the figures nearest it first, in the colour whose turn it is and then
        # its others, where its arps sit; as long as drawn, then the other lengths.
        language = language_of(s.style)
        self.arps_made = getattr(self, "arps_made", 0) + 1
        colours = language["colours"][self.arps_made % len(language["colours"]):] + language["colours"][:self.arps_made % len(language["colours"])]
        ranked = sorted(figures, key=lambda f: -fits(f[0].split(), language))
        candidates = []

        for figure, words, _ in ranked:
            for colour in colours:
                for length in [bars] + [b for b in (2, 4, 1) if b != bars]:
                    chords = [names(coloured(mode, d, language["register"], colour)) for d in degrees]
                    chords = chords[:length] if length > 1 else chords[:1]
                    part = arp(chords, figure, gate=0.4 if sub == "Gated" else 0.5)
                    where = "through the roots " + root_names(mode, degrees[:length]) if length > 1 else "on " + LETTERS[MODES[mode][degrees[0] % 7]]
                    candidates.append((f"{words}, {COLOURS[colour][1]}, {where}", part, synth(patch), tags_for([n[0] for n in part.notes])))

        self.add_new("arp", ROLE_NOUNS[sub], "Arps", sub, candidates)

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
            self.pick([2, 4])           # drawn as before the risers were written style by style,
            self.pick(["sweep", "saw"])   # so every preset after this one keeps its name
            words, part, voice, tags = RISERS[s.style]()
            self.add("build", ROLE_NOUNS[sub], "SFX", sub, words, part, voice, tags, tempo=tempo)
        elif sub == "Downlifters":
            part = Part(2).add(degree_pitch("minor", 0, 57, 68), 0, 2 * BAR - 40, 100).lane(128, [(0, 0.5), (2 * BAR - 1, 0.0)]).lane(74, [(0, 1.0), (2 * BAR - 1, 0.1)])
            self.add("build", ROLE_NOUNS[sub], "SFX", sub, "a held A bending down two semitones as its filter closes over two bars", part, synth("sweep"), tempo=tempo)
        elif sub == "Impacts":
            self.pick(["boom", "rumble", "hard"])   # drawn as before, so the names after it stay
            words, part, voice, tags = IMPACTS[s.style]()
            self.add("build", ROLE_NOUNS[sub], "SFX", sub, words, part, voice, tags, tempo=tempo)
        elif sub == "Sweeps":
            degrees, mode = self.progression()
            part = Part(4).chords([names(triad(mode, degrees[0]))] * 4, velocity=80).lane(74, [(0, 0.0), (4 * BAR - 1, 1.0)])
            self.add("build", ROLE_NOUNS[sub], "SFX", sub, f"{chord_name(mode, degrees[0])} held while its filter opens over four bars", part, synth("sweep"),
                     self.mode_tags(mode, [n[0] for n in part.notes]), tempo=tempo)
        elif sub == "Transitions":
            self.pick(s.drums_sound)   # drawn as before, so the names after it stay
            words, part, voice, tags = TRANSITIONS[s.style]()
            self.add("build", ROLE_NOUNS[sub], "SFX", sub, words, part, voice, tags, tempo=tempo)
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

        # The second style to draw a sound gets its second form, so no two styles play the same notes.
        DRAWN[label] = DRAWN.get(label, 0) + 1
        if DRAWN[label] > 1:
            words, notes = self.UI_AGAIN[label]

        part = Part(1)
        for n, step, length in notes:
            part.add(pitch(n), step * S, length * S - 20, 110)
        patch = self.pick(["chip", "bell", "glass", "pluck"] if self.s.style != "Game/Chiptune" else ["chip"])

        # A third style to draw it has nothing new to play: the sound is left out.
        if DRAWN[label] > 2:
            return self.leave_out([label, label + " Blip", label + " Sound"])

        self.add("build", [label, label + " Blip", label + " Sound"], "SFX", "UI Sounds", words, part, synth(patch), tempo=tempo)

    STINGERS = [
        ("Victory", "a victory fanfare: A minor, F and G struck, landing on a held C major", [["a3", "c4", "e4"], ["f3", "a3", "c4"], ["g3", "b3", "d4"], ["g3", "c4", "e4"]], "x.x.x...x......."),
        ("Defeat", "a defeat sting: E minor and D minor falling to a low held A minor", [["e3", "g3", "b3"], ["d3", "f3", "a3"], ["a2", "c3", "e3"]], "x...x...x......."),
        ("Discovery", "a discovery sting: an A minor ninth rising a note a 16th and held", None, None),
        ("Danger", "a danger sting: A and G sharp struck low together, twice", [["a2", "g#3"], ["a2", "g#3"]], "x.....x........."),
        ("Quest Complete", "A minor, C and G struck, then E held on top", [["a3", "c4", "e4"], ["g3", "c4", "e4"], ["g3", "b3", "d4"], ["a3", "c4", "e4", "a4"]], "x..x..x.x......."),
        ("Game Over", "a slow fall through F, E minor and D minor to A minor", [["f3", "a3", "c4"], ["e3", "g3", "b3"], ["d3", "f3", "a3"], ["a2", "c3", "e3"]], "x...x...x...x..."),
    ]

    STINGERS_AGAIN = {
        "Victory": ("a victory fanfare: C, G and A minor struck, landing on a held E with its octave", [["c4", "e4", "g4"], ["g3", "b3", "d4"], ["a3", "c4", "e4"], ["e4", "a4", "e5"]], "x..x..x...x....."),
        "Defeat": ("a defeat sting: A minor falling by step through G and F to a low held E", [["a3", "c4", "e4"], ["g3", "b3", "d4"], ["f3", "a3", "c4"], ["e2", "e3", "b3"]], "x.x.x...x......."),
        "Discovery": ("a discovery sting: fifths and fourths stacking upward from A a 16th apart and held", None, None),
        "Danger": ("a danger sting: B and F low together, a tritone, struck three times and the last held", [["b2", "f3"], ["b2", "f3"], ["b2", "f3"]], "x..x..x........."),
        "Quest Complete": ("F, G and A minor climbing in 8ths, then the A minor above held", [["f3", "a3", "c4"], ["g3", "b3", "d4"], ["a3", "c4", "e4"], ["a4", "c5", "e5"]], "x.x.x.x........."),
        "Game Over": ("a slow fall through A minor, G and F to a held E major", [["a3", "c4", "e4"], ["g3", "b3", "d4"], ["f3", "a3", "c4"], ["e3", "g#3", "b3"]], "x...x...x...x..."),
    }

    UI_AGAIN = {
        "Confirm": ("two notes rising a fourth, E to A, for a confirm or select", [("e5", 0, 1), ("a5", 1, 2)]),
        "Cancel": ("two notes falling a fourth, A to E, for a cancel or back", [("a5", 0, 1), ("e5", 1, 2)]),
        "Pickup": ("E, A and the E above in quick 16ths for collecting an item", [("e5", 0, 1), ("a5", 1, 1), ("e6", 2, 2)]),
        "Error": ("E and F together, a semitone, struck twice low for an error", [("e4", 0, 1), ("f4", 0, 1), ("e4", 2, 1), ("f4", 2, 1)]),
        "Level Up": ("an A minor ninth run up in 16ths, A, B, C, E, G, B, the high A held", [("a4", 0, 1), ("b4", 1, 1), ("c5", 2, 1), ("e5", 3, 1), ("g5", 4, 1), ("b5", 5, 1), ("a6", 6, 4)]),
        "Cursor": ("two quick notes a step apart, D to E, for moving a cursor", [("d6", 0, 1), ("e6", 1, 1)]),
        "Notification": ("three notes, E, A and C, rising for a notice", [("e5", 0, 1), ("a5", 1, 1), ("c6", 2, 3)]),
        "Power Down": ("five notes falling in thirds from the high E for losing a power", [("e6", 0, 1), ("c6", 1, 1), ("a5", 2, 1), ("f5", 3, 1), ("d5", 4, 3)]),
        "Open": ("A, C and E rising quickly for opening a menu", [("a4", 0, 1), ("c5", 1, 1), ("e5", 2, 2)]),
        "Close": ("E, C and A falling quickly for closing a menu", [("e5", 0, 1), ("c5", 1, 1), ("a4", 2, 2)]),
        "Unlock": ("a rising fifth then the octave, A, E, A, for something unlocked", [("a4", 0, 1), ("e5", 1, 1), ("a5", 3, 3)]),
        "Countdown": ("a short E on each beat, the last a fifth up", [("e5", 0, 1), ("e5", 4, 1), ("e5", 8, 1), ("b5", 12, 3)]),
    }

    def stinger(self, tempo):
        label, words, chords, rhythm = self.STINGERS[self.sting_index % len(self.STINGERS)]
        self.sting_index += 1
        part = Part(2)
        rising = ["a3", "c4", "e4", "g4", "b4"]

        DRAWN[label] = DRAWN.get(label, 0) + 1
        if DRAWN[label] > 1:
            words, chords, rhythm = self.STINGERS_AGAIN[label]
            rising = ["a3", "e4", "b4", "e5", "a5"]

        if chords is None:
            for k, n in enumerate(rising):
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

        if DRAWN[label] > 2:
            return self.leave_out([label, label + " Sting", label + " Stinger"])

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


rewrite("Rumble Drone", drone_hard("=Corroded Reese"))
rewrite("Tension Drone", drone_ebm_tension("=Cold Saw Lead"))


def generate():
    used = {p.name for p in PRESETS} | {k[0] for k in KITS}

    # The presets written by hand come first: what they play is theirs.
    for p in PRESETS:
        remember(p.role + " > " + p.sub, p.style, p.part, p.voice[0] == "drums")

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


# ---------------------------------------------------------------------------------------------
# Grooves written for one kit each: the Breaks kit, which no generated groove landed on, and the kits
# of Basic Drums' Modern sounds, whose ghost notes are duller as well as quieter. They come after the
# generated presets, so none of those changes its name or its voice.

def kit_groove(name, style, energy, character, description, part, tempo, sound):
    assert name not in {p.name for p in PRESETS} | {k[0] for k in KITS}, name
    preset(name, D, "Full Grooves", style, energy, character, description, part, tempo, drums(sound))


g = Part(2).grid(KICK, "X.........X.....X.x.......x.....").grid(SNARE, "....X.......X..g....X..g....X...").grid(CH, "x.o.x.o.x.o.x.o.").grid(RIDE, "x...x...x...x...")
kit_groove("Back Room Break", "Breakbeat/Breaks", 3, ["Dark", "Dense", "Straight", "Distorted"],
           "A two-step break with a kick pickup in bar 2, ghost snares before the backbeat, 8th hats and a ride on the beat", g, 130, "breaks")

g = Part(2).grid(KICK, "X.x.......x.....X.........x..x..").grid(SNARE, "....X..g.g..X.......X..g.g..X.g.").grid(CH, "wwxwwwxwwwxwwwxw").grid(OH, "......x.......x.")
kit_groove("Tape Splice Groove", "Breakbeat/Breaks", 4, ["Dark", "Dense", "Straight", "Distorted"],
           "A two-bar break, the kick moving in bar 2 and the snare ghosting round the backbeat, 16th hats and an open hat before 3 and 1", g, 128, "breaks")

g = Part(2).grid(KICK, "X...X...X...X...").grid(KICK, "...g.......g....").grid(CLAP, "....X.......X...").grid(SNARE, "....o.......o...") \
    .grid(CH, "wwxwwwxwwwxwwwxw").grid(SHAKER, ".w.w.w.w.w.w.w.w").grid(OH, "..x...x...x...x.", bar=1)
kit_groove("Clean Room Groove", "Techno/Driving", 4, ["Bright", "Dense", "Straight", "Clean"],
           "Four to the floor with ghost kicks before 2 and 4, a clap over a snare, 16th hats with the offbeat lifted, a shaker between them and open hats in bar 2", g, 134, "modern")

g = Part(4).grid(KICK, "X...X...X...X...").grid(KICK, ".gg..gg..gg..gg.").grid(CH, "w.ow..ow..ow..ow").grid(OH, "..x...x...x...x.").grid(CLAP, "....x.......x...") \
    .grid(RIM, ".......o......o.")
for b in (2, 3):
    g.grid(RIDE, "x.o.x.o.x.o.x.o.", bar=b)
g.grid(CLAP, "..............xX", bar=3)
kit_groove("Concrete Floor Roller", "Techno/Dark", 4, ["Dark", "Dense", "Straight", "Distorted"],
           "Four to the floor with a rumble between the kicks, offbeat open hats, a clap on 2 and 4 and a rim against it; a ride comes in for bars 3 and 4, and a clap pickup ends bar 4", g, 132, "warehouse")

g = Part(4).grid(KICK, "X...X...X...X...").cycle(RIM, 3, "o").cycle(SHAKER, 5, "w", 2).grid(CH, "..x...x...x...x.")
for b in (1, 3):
    g.grid(LT, "..........o.....", bar=b)
g.grid(OH, "..x...x...x...x.", bar=3)
kit_groove("Strobe Tunnel Pulse", "Techno/Hypnotic", 3, ["Dark", "Sparse", "Straight", "Clean"],
           "A rim every three 16ths and a shaker every five turn against the kick and offbeat hats, a low tom answering in bars 2 and 4, open hats over bar 4", g, 130, "warehouse")

g = Part(2).grid(KICK, "X...X...X...X...").grid(KICK, "..............x.", bar=1).grid(CLAP, "....X.......X...").grid(OH, "..x...x...x...x.").grid(CH, "x.wwx.wwx.wwx.ww") \
    .grid(RIDE, "x.o.x.o.x.o.x.o.", bar=1).grid(CRASH, "x...............", bar=0)
kit_groove("Night Shift Hammer", "Techno/Peak-time", 5, ["Dark", "Dense", "Straight", "Distorted"],
           "A peak-time groove: a crash on the one, a clap on 2 and 4, open hats on the offbeats over closed ones, a ride in 8ths and a skipped kick in bar 2", g, 136, "warehouse")

g = Part(2).grid(KICK, "X.......X.x.....", bar=0).grid(KICK, "X.....x.X.x.....", bar=1).grid(SNARE, "....X.......X...").grid(CH, "x.o.x.o.x.o.x.o.") \
    .grid(HT, ".............x..", bar=1).grid(MT, "..............x.", bar=1).grid(LT, "...............X", bar=1)
kit_groove("Muscle Memory Beat", "Industrial/EBM", 3, ["Dark", "Sparse", "Straight", "Clean"],
           "The machine backbeat: kick on 1 and 3 with a push, a big snare on 2 and 4, 8th hats with the downbeats stronger, three toms falling into the next bar", g, 118, "bodymusic")

g = Part(1).grid(KICK, "X...X...X...X...").grid(SNARE, "....X.......X...").grid(CLAP, "....x.......x...").grid(OH, "..x...x...x...x.").grid(CH, "x.xxx.xxx.xxx.xx")
kit_groove("Marching Order Beat", "Industrial/EBM", 4, ["Dark", "Dense", "Straight", "Clean"],
           "Four to the floor, a big snare and clap together on 2 and 4, open hats on the offbeats and flat machine hats round them", g, 124, "bodymusic")

g = Part(4).grid(KICK, "X...X...X...X...").grid(SNARE, "....x.......x...").grid(CH, "wwowwwowwwowwwow").grid(COWBELL, "...o.....o.o....").grid(SNARE, "........gogoxxXX", bar=3)
kit_groove("Cold Assembly Drive", "Industrial/Techno-body", 4, ["Dark", "Dense", "Straight", "Distorted"],
           "Techno kick and 16th hats with a snare on 2 and 4 and a metal figure, ready for an EBM bass in the gaps; a snare roll rises through the end of bar 4", g, 128, "bodymusic")

g = Part(2).grid(KICK, "X...X...X...X...").grid(LT, "..x...x...x..x.x").grid(SNARE, "....X.......X...").grid(CH, "x.x.x.x.x.x.x.x.").grid(RIM, "...o......o.....") \
    .grid(CRASH, "x...............", bar=0)
kit_groove("Slag Hammer Pound", "Techno/Industrial", 4, ["Dark", "Dense", "Straight", "Distorted"],
           "A pounding kick with low toms in its gaps, a hard snare on 2 and 4, flat 8th hats, a rim against them and a crash on the one", g, 128, "foundry")

g = Part(2).grid(KICK, "X.....x...X..x..").grid(SNARE, "....X..g....X.g.").grid(CH, "xwowxwowxwowxwow").grid(COWBELL, "x..x..x..x..x...")
kit_groove("Rivet Line Stomp", "Industrial/Electro-industrial", 4, ["Dark", "Dense", "Straight", "Distorted"],
           "A broken kick with a push before the 4, a snare on 2 and 4 with ghost notes after them, 16th hats shaped by velocity and metal struck in threes", g, 112, "foundry")

g = Part(4).grid(KICK, "X...X...X...X...").grid(KICK, "..gg..gg..gg..gg").grid(OH, "..o...o...o...o.").grid(CLAP, "....x.......x...")
for b in (0, 2):
    g.grid(CRASH, "x...............", bar=b)
g.grid(KICK, "........xxxxXXXX", bar=3)
kit_groove("Blast Furnace Engine", "Techno/Hard", 5, ["Dark", "Dense", "Straight", "Distorted"],
           "Fast four-to-the-floor with a rumble between the kicks, offbeat open hats, a clap on 2 and 4 and a crash every other bar; a kick roll ends bar 4", g, 148, "foundry")

g = Part(2).grid(KICK, "X.x.......x.....X.........x..x..").grid(SNARE, "....X..g.g..X.......X..g.g..X.g.").grid(CH, "x.o.x.o.x.o.x.o.").grid(OH, "..............x.", bar=1)
kit_groove("Cracked Pavement Break", "Breakbeat/Breaks", 4, ["Dark", "Dense", "Straight", "Distorted"],
           "A two-bar break, the kick moving in bar 2 and the snare ghosting round the backbeat, 8th hats and an open hat into the next bar", g, 132, "brokenbeat")

g = Part(2).grid(KICK, "X..X......X.....X..X....X.......").grid(SNARE, "....X..g.g..X.......X..g.g..X..X").grid(CH, "x..ox..ox..ox..o").grid(CRASH, "x...............", bar=0)
kit_groove("Loose Wire Break", "Breakbeat/Breaks", 4, ["Dark", "Dense", "Straight", "Distorted"],
           "A chopped break, the snare doubling at the end of bar 2, hats pushing into each beat and a crash on the one", g, 135, "brokenbeat")


# A kit's six parts play together, so each sits lower in a kit than it does alone: alone a preset peaks
# near -9 dB, and six of them added up reached +4.7 dB (Terror Kit). At -6 dB each, the loudest kit peaks at -1.3 dB.
KIT_TRIM_DB = -6.0


def kit_xml(name, tempo, style, parts):
    by_name = {p.name: p for p in PRESETS}
    out = [f'<TORQWAVE_KIT version="1" name={quoteattr(name)} key="{KEY}" tags="Starter kit" '
           f'note={quoteattr("A starting point for " + style.split("/")[1] + ": one of each part, in A minor at " + str(tempo) + " BPM")} '
           f'style={quoteattr(style)}>']

    for part_name in parts:
        p = by_name[part_name]
        tempo_before = p.tempo
        p.tempo = tempo
        out.append(p.xml("  ", KIT_TRIM_DB))
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
