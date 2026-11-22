"""What Scripts/make_patch_bank.py writes patches with: Source/WaveSynthPatchBuilder.h's Builder again,
keeping both the settings (to measure a patch before it is compiled) and the C++ that sets them."""

import random
import struct

# The builder's names for tables, warps, filters, drives, LFO shapes and divisions, in its order.
TABLES = ["analog", "pulse", "vowel", "fm", "metal", "sub", "additive", "sync", "growl", "string"]
WARPS = ["wOff", "wSync", "wBend", "wSqueeze", "wMirror", "wFm", "wCrush", "wFold"]
FILTERS = ["off", "ladder24", "ladder12", "ladderHigh", "ladderBand", "svfLow", "svfBand", "svfHigh", "svfNotch", "comb"]
DRIVES = ["soft", "hard", "tube", "fuzz", "foldDrive", "crush"]
SHAPES = ["sine", "triangle", "sawUp", "sawDown", "square", "randomShape", "drift", "steps"]
DIVISIONS = ["bars8", "bars4", "bars2", "bar1", "half", "halfT", "quarterD", "quarter", "quarterT", "eighthD", "eighth", "eighthT",
             "sixteenthD", "sixteenth", "sixteenthT", "thirtySecond", "sixtyFourth"]
SOURCES = ["none", "env1", "env2", "env3", "env4", "lfo1", "lfo2", "velocity", "modWheel", "aftertouch", "key",
           "macro1", "macro2", "macro3", "macro4", "WaveSynth::random"]


def destination_ids():
    ids = []

    for o in (1, 2, 3):
        ids += [f"osc{o}{f}" for f in ("Position", "WarpAmount", "Pitch", "Fine", "Volume", "Pan", "UnisonDetune", "UnisonSpread", "Route")]

    ids += ["subLevel", "noiseLevel", "noiseColour"]

    for f in (1, 2):
        ids += [f"filter{f}Cutoff", f"filter{f}Resonance", f"filter{f}Drive"]

    return ids + ["ampLevel", "ampPan", "lfo1Rate", "lfo2Rate", "fxDriveAmount", "fxChorusMix", "fxPhaserMix", "fxDelayMix",
                  "fxDelayFeedback", "fxReverbMix"]


DESTINATIONS = destination_ids()

for _i, _n in enumerate(TABLES): globals()[_n] = _i
for _i, _n in enumerate(WARPS): globals()[_n] = _i
for _i, _n in enumerate(FILTERS): globals()[_n] = _i
for _i, _n in enumerate(DRIVES): globals()[_n] = _i
for _i, _n in enumerate(SHAPES): globals()[_n] = _i
for _i, _n in enumerate(DIVISIONS): globals()[_n] = _i
for _i, _n in enumerate(SOURCES[:-1]): globals()[_n] = _i
perNote = len(SOURCES) - 1   # the Random source: a new value for each note

# Analog's positions: sine, triangle, saw, square.
sinePos, trianglePos, sawPos, squarePos = 0.0, 33.33, 66.67, 100.0
ANALOG_POSITIONS = {sinePos: "sinePos", trianglePos: "trianglePos", sawPos: "sawPos", squarePos: "squarePos"}

# The builder has two things called crush (a drive) and off (a filter); the warps are wCrush and wOff.
crush = DRIVES.index("crush")
off = FILTERS.index("off")


def parameter_ranges():
    """Each numeric parameter's range (Source/WaveSynth.cpp): a setting outside it is a mistake in a recipe."""
    r = {"subLevel": (0, 100), "noiseLevel": (0, 100), "noiseColour": (-100, 100), "ampLevel": (-40, 6), "glideTime": (0, 2000),
         "fxDriveAmount": (0, 100), "fxDriveTone": (500, 20000), "fxDriveMix": (0, 100), "fxChorusRate": (0.05, 5), "fxChorusDepth": (0, 100),
         "fxChorusMix": (0, 100), "fxPhaserRate": (0.02, 5), "fxPhaserFeedback": (0, 100), "fxPhaserMix": (0, 100), "fxDelayFeedback": (0, 100),
         "fxDelayTone": (500, 20000), "fxDelayMix": (0, 100), "fxReverbSize": (0, 100), "fxReverbDecay": (0.2, 20), "fxReverbDamping": (0, 100),
         "fxReverbMix": (0, 100)}

    for o in (1, 2, 3):
        r.update({f"osc{o}Position": (0, 100), f"osc{o}WarpAmount": (0, 100), f"osc{o}Pitch": (-24, 24), f"osc{o}Fine": (-100, 100),
                  f"osc{o}Volume": (0, 100), f"osc{o}Pan": (-100, 100), f"osc{o}Unison": (1, 8), f"osc{o}UnisonDetune": (0, 100),
                  f"osc{o}UnisonSpread": (0, 100), f"osc{o}UnisonStrum": (0, 60), f"osc{o}UnisonDrift": (0, 100), f"osc{o}Route": (0, 100)})

    for f in (1, 2):
        r.update({f"filter{f}Cutoff": (20, 20000), f"filter{f}Resonance": (0, 100), f"filter{f}Drive": (0, 100), f"filter{f}KeyTrack": (0, 100)})

    for e in (1, 2, 3, 4):
        r.update({f"env{e}Attack": (0, 10000), f"env{e}Decay": (0, 10000), f"env{e}Release": (0, 10000), f"env{e}Sustain": (0, 100),
                  f"env{e}Curve": (-100, 100)})

    for l in (1, 2):
        r.update({f"lfo{l}Rate": (0.01, 40), f"lfo{l}Fade": (0, 5000), f"lfo{l}Smooth": (0, 100)})
        r.update({f"lfo{l}Step{i}": (-100, 100) for i in range(1, 17)})

    return r


RANGES = parameter_ranges()


def f32(x):
    """The value as the float the synth takes."""
    return struct.unpack("f", struct.pack("f", float(x)))[0]


def num(x):
    """A number as C++ writes it: 35, 0.6f."""
    if float(x) == int(x):
        return str(int(x))

    return f"{round(float(x), 4):g}f"


class Patch:
    def __init__(self, name="", category=""):
        self.name, self.category = name, category
        self.values = {}
        self.calls = []
        self.slots = 0
        self.macros = ["", "", "", ""]
        self.kind = ""       # the kind of sound it was drawn from, and the seed of that draw
        self.seed = ""
        self.target = None   # the peak a held chord is levelled to, in dB

    # --- the builder's calls -----------------------------------------------------------------
    def _v(self, **values):
        for k, v in values.items():
            low, high = RANGES.get(k, (-1.0e9, 1.0e9))
            assert low <= v <= high, (self.kind, k, v)
            self.values[k] = f32(v)

    def _call(self, name, *args, defaults=()):
        """Records the call, leaving out trailing arguments that are the builder's own defaults."""
        args = list(args)
        defaults = list(defaults)

        while defaults and args and args[-1] == defaults[-1]:
            args.pop()
            defaults.pop()

        self.calls.append(f"{name} ({', '.join(args)})" if args else name + "()")
        return self

    def set(self, key, value):
        self._v(**{key: value})
        return self._call("set", f'"{key}"', num(value))

    def osc(self, n, table, position, level=80):
        self._v(**{f"osc{n}On": 1, f"osc{n}Table": table, f"osc{n}Position": position, f"osc{n}Volume": level})
        named = ANALOG_POSITIONS.get(position) if table == TABLES.index("analog") else None
        return self._call("osc", str(n), TABLES[table], named or num(position), num(level), defaults=("80",))

    def pitch(self, n, semitones, cents=0):
        self._v(**{f"osc{n}Pitch": semitones, f"osc{n}Fine": cents})
        return self._call("pitch", str(n), num(semitones), num(cents), defaults=("0",))

    def warp(self, n, kind, amount):
        self._v(**{f"osc{n}Warp": kind, f"osc{n}WarpAmount": amount})
        return self._call("warp", str(n), WARPS[kind], num(amount))

    def pan(self, n, p):
        self._v(**{f"osc{n}Pan": p})
        return self._call("pan", str(n), num(p))

    def route(self, n, r):
        self._v(**{f"osc{n}Route": r})
        return self._call("route", str(n), num(r))

    def unison(self, n, voices, detune, spread=80, strum=0, drift=0):
        self._v(**{f"osc{n}Unison": voices, f"osc{n}UnisonDetune": detune, f"osc{n}UnisonSpread": spread,
                   f"osc{n}UnisonStrum": strum, f"osc{n}UnisonDrift": drift})
        return self._call("unison", str(n), str(voices), num(detune), num(spread), num(strum), num(drift), defaults=("80", "0", "0"))

    def subOsc(self, level, octave=0, shape=0, direct=True):
        self._v(subLevel=level, subOctave=octave, subShape=shape, subDirect=1 if direct else 0)
        return self._call("subOsc", num(level), str(octave), str(shape), "true" if direct else "false", defaults=("0", "0", "true"))

    def noise(self, level, kind=0, colour=0):
        self._v(noiseLevel=level, noiseType=kind, noiseColour=colour)
        return self._call("noise", num(level), str(kind), num(colour), defaults=("0", "0"))

    def filter(self, n, kind, hz, res=10, drive=0, key=0):
        self._v(**{f"filter{n}Type": kind, f"filter{n}Cutoff": hz, f"filter{n}Resonance": res, f"filter{n}Drive": drive,
                   f"filter{n}KeyTrack": key})
        return self._call("filter", str(n), FILTERS[kind], num(hz), num(res), num(drive), num(key), defaults=("10", "0", "0"))

    def parallel(self):
        self._v(filterRouting=1)
        return self._call("parallel")

    def env(self, n, a, d, s, r, curve=50):
        self._v(**{f"env{n}Attack": a, f"env{n}Decay": d, f"env{n}Sustain": s, f"env{n}Release": r, f"env{n}Curve": curve})
        return self._call("env", str(n), num(a), num(d), num(s), num(r), num(curve), defaults=("50",))

    def amp(self, a, d, s, r, curve=50):
        self._v(env1Attack=a, env1Decay=d, env1Sustain=s, env1Release=r, env1Curve=curve)
        return self._call("amp", num(a), num(d), num(s), num(r), num(curve), defaults=("50",))

    def level(self, db, velocity=70):
        self._v(ampLevel=db, ampVelocity=velocity)
        return self._call("level", num(db), num(velocity), defaults=("70",))

    def mono(self, glide=0, legato=True):
        self._v(voiceMode=2 if legato else 1, glideTime=glide)
        return self._call("mono", num(glide), "true" if legato else "false", defaults=("0", "true"))

    def glideAlways(self):
        self._v(glideMode=1)
        return self._call("glideAlways")

    def lfo(self, n, shape, hz, mode=1, smooth=0):
        self._v(**{f"lfo{n}Shape": shape, f"lfo{n}Sync": 0, f"lfo{n}Rate": hz, f"lfo{n}Mode": mode, f"lfo{n}Smooth": smooth})
        return self._call("lfo", str(n), SHAPES[shape], num(hz), str(mode), num(smooth), defaults=("1", "0"))

    def lfoSync(self, n, shape, division, mode=1, smooth=0):
        self._v(**{f"lfo{n}Shape": shape, f"lfo{n}Sync": 1, f"lfo{n}Division": division, f"lfo{n}Mode": mode, f"lfo{n}Smooth": smooth})
        return self._call("lfoSync", str(n), SHAPES[shape], DIVISIONS[division], str(mode), num(smooth), defaults=("1", "0"))

    def lfoFade(self, n, ms):
        self._v(**{f"lfo{n}Fade": ms})
        return self._call("lfoFade", str(n), num(ms))

    def stepPattern(self, n, steps):
        assert len(steps) == 16
        self._v(**{f"lfo{n}Step{i + 1}": v for i, v in enumerate(steps)})
        return self._call("stepPattern", str(n), "{ " + ", ".join(num(v) for v in steps) + " }")

    def mod(self, source, destination, amount):
        assert self.slots < 8, (self.kind, "more than eight modulation slots")   # the bank keeps to the first page of slots
        assert amount != 0 and -100 <= amount <= 100
        self.slots += 1
        m = f"mod{self.slots}"
        self._v(**{m + "Source": source, m + "Destination": DESTINATIONS.index(destination) + 1, m + "Amount": amount})
        return self._call("mod", SOURCES[source], f'"{destination}"', num(amount))

    def macro(self, n, name, value=0):
        self.macros[n - 1] = name
        self._v(**{f"macro{n}": value})
        return self._call("macro", str(n), f'"{name}"', num(value), defaults=("0",))

    def drive(self, kind, amount, tone=12000, mix=100):
        self._v(fxDriveOn=1, fxDriveType=kind, fxDriveAmount=amount, fxDriveTone=tone, fxDriveMix=mix)
        return self._call("drive", DRIVES[kind], num(amount), num(tone), num(mix), defaults=("12000", "100"))

    def chorus(self, mix=40, rate=0.6, depth=50):
        self._v(fxChorusOn=1, fxChorusMix=mix, fxChorusRate=rate, fxChorusDepth=depth)
        return self._call("chorus", num(mix), num(rate), num(depth))

    def phaser(self, mix=50, rate=0.3, feedback=50):
        self._v(fxPhaserOn=1, fxPhaserMix=mix, fxPhaserRate=rate, fxPhaserFeedback=feedback)
        return self._call("phaser", num(mix), num(rate), num(feedback))

    def delay(self, division, feedback=35, mix=25, tone=5000, pingPong=True):
        self._v(fxDelayOn=1, fxDelayDivision=division, fxDelayFeedback=feedback, fxDelayMix=mix, fxDelayTone=tone,
                fxDelayPingPong=1 if pingPong else 0)
        return self._call("delay", DIVISIONS[division], num(feedback), num(mix), num(tone), "true" if pingPong else "false", defaults=("35", "25", "5000", "true"))

    def reverb(self, mix=25, size=60, decay=2.5, damping=45):
        self._v(fxReverbOn=1, fxReverbMix=mix, fxReverbSize=size, fxReverbDecay=decay, fxReverbDamping=damping)
        return self._call("reverb", num(mix), num(size), num(decay), num(damping))

    # --- helpers that are several calls ------------------------------------------------------
    def has(self, key):
        return self.values.get(key, 0) != 0

    def get(self, key, default=0.0):
        return self.values.get(key, default)

    def knob(self, n, name, destination, amount):
        """A macro and where it goes."""
        self.macro(n, name)
        return self.mod(macro1 + n - 1, destination, amount)

    def set_level(self, db):
        """Changes the level() call already made."""
        self._v(ampLevel=db)

        for i, c in enumerate(self.calls):
            if c.startswith("level ("):
                velocity = num(self.values["ampVelocity"])
                self.calls[i] = f"level ({num(db)})" if velocity == "70" else f"level ({num(db)}, {velocity})"
                return

        raise AssertionError(self.name + " has no level")

    # --- output ------------------------------------------------------------------------------
    def line(self):
        """The patch as TorqwaveTests --patch-measure --patches reads it."""
        settings = ";".join(f"{k}={v!r}" for k, v in self.values.items())
        return f"{self.name}|{self.category}|{';'.join(self.macros)}|{settings}"

    def cpp(self, width=148):
        """The patch as a statement for the bank, wrapped as the hand-made ones are."""
        parts = [f'add ("{self.name}", "{self.category}")'] + ["." + c for c in self.calls]
        lines, current = [], "        "

        for part in parts:
            if len(current) + len(part) > width and current.strip():
                lines.append(current.rstrip())
                current = "            "

            current += part

        lines.append(current.rstrip() + ";")
        return "\n".join(lines)


class Draw:
    """The generator a kind of sound draws its settings from: seeded by a name, so the same every run."""

    def __init__(self, seed):
        self.r = random.Random(seed)

    def u(self, low, high, step=1):
        """A number from low to high on a grid of step."""
        n = int(round((high - low) / step))
        v = low + step * self.r.randint(0, n)
        return round(v, 4)

    def log(self, low, high, digits=2):
        """A number from low to high, as likely in each octave, to two significant digits."""
        import math
        v = math.exp(self.r.uniform(math.log(low), math.log(high)))
        magnitude = 10 ** (math.floor(math.log10(v)) - digits + 1)
        return round(round(v / magnitude) * magnitude, 4)

    def pick(self, options):
        return options[self.r.randrange(len(options))]

    def weighted(self, options):
        """options: (weight, value) pairs."""
        total = sum(w for w, _ in options)
        x = self.r.uniform(0, total)

        for w, v in options:
            x -= w
            if x <= 0:
                return v

        return options[-1][1]

    def chance(self, p):
        return self.r.random() < p

    def sample(self, options, n):
        return self.r.sample(options, n)

    def shuffle(self, items):
        items = list(items)
        self.r.shuffle(items)
        return items
