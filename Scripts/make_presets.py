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
        instrument = DRUMS if kind == "drums" else SYNTH
        settings = KITS_SOUND[sound] if kind == "drums" else PATCHES[sound]
        label = f"Basic Drums: {sound.capitalize()}" if kind == "drums" else f"Basic Synth: {sound.capitalize()}"
        out.append(f"{indent}  <VOICE>{voice_xml(label, instrument, settings, -3.0 if kind == 'drums' else 0.0)}</VOICE>")
        out.append(f"{indent}</TORQWAVE_SEQUENCE>")
        return "\n".join(out)


PRESETS = []


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
