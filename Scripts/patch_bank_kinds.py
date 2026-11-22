"""The kinds of sound Torqwave Synth's second patch bank is drawn from. Each kind is one way of
making a sound (an FM bass, a clap, a braam), written once with its settings drawn in the ranges
that suit it; Scripts/make_patch_bank.py draws candidates from each, measures them and keeps the
ones that pass.

What the engine does that shapes these (Source/WaveSynth.cpp):
  - a slot's amount is a share of the destination's whole range: a tenth of the cutoff's is an
    octave, 25 of an oscillator's pitch is an octave (and pitch moves in whole semitones, so slow
    sweeps go on Fine, the Sync warp or a resonating filter instead), 13 of the amp level is 6 dB;
  - a Steps LFO plays its 16 steps once a cycle, so a pattern of sixteenths runs at one bar;
  - only a Steps LFO can be smoothed, so gates are Steps patterns with Smooth, never a bare square;
  - a synced LFO has no rate to modulate, and a filter's key tracking turns about middle C: a comb
    tuned to the key has its cutoff at 261.6 Hz;
  - the FM warp takes the oscillator before it as its modulator (oscillator 1 takes oscillator 3),
    which must sound, if only at 1 %.
"""

from patch_bank_builder import *

KINDS = []


def kind(category, count, nouns):
    """Registers a kind: its category, how many patches to aim for, and the nouns its names end in."""
    def register(fn):
        KINDS.append({"category": category, "name": fn.__name__, "count": count, "nouns": nouns.split(), "make": fn})
        return fn
    return register


# ---------------------------------------------------------------------------------------------
# Shared pieces.

def semis(st):
    """A slot amount that moves an oscillator's pitch by this many semitones at full."""
    return round(st / 48.0 * 100.0, 3)


def step_notes(notes, amount=50.0):
    """Step values that play these semitone offsets through a slot of `amount` on an oscillator's pitch."""
    return [round(n / (48.0 * amount / 100.0) * 100.0, 2) for n in notes]


def gate(pattern, soft=-35):
    """A 16-step gate from a string: X open, x softer, . closed."""
    assert len(pattern) == 16, pattern
    return [0 if c == "X" else soft if c == "x" else -100 for c in pattern]


def need_drive(p, kind=tube, tone=9000):
    if not p.has("fxDriveOn"):
        p.drive(kind, 0, tone)


def need_reverb(p, size=70, decay=3.0, damping=45):
    if not p.has("fxReverbOn"):
        p.reverb(0, size, decay, damping)


def need_delay(p, division=eighthD, feedback=40, tone=4500):
    if not p.has("fxDelayOn"):
        p.delay(division, feedback, 0, tone)


def need_chorus(p, rate=0.5, depth=50):
    if not p.has("fxChorusOn"):
        p.chorus(0, rate, depth)


def need_phaser(p, rate=0.2, feedback=55):
    if not p.has("fxPhaserOn"):
        p.phaser(0, rate, feedback)


def knobs(p, r, *choices):
    """Four macros. Each choice is (name, destination, amount) or (name, destination, amount, prepare);
    a list of choices is one drawn from it. More than four given: the first ones are kept and the
    rest drawn from."""
    choices = [r.pick(c) if isinstance(c, list) else c for c in choices]
    assert len(choices) >= 4, p.kind

    if len(choices) > 4:
        keep = choices[:2]
        choices = keep + r.sample(choices[2:], 2)

    names = set()

    for i, c in enumerate(choices):
        name, destination, amount = c[0], c[1], c[2]
        assert name not in names, (p.kind, name)
        names.add(name)

        if len(c) > 3:
            c[3](p)

        p.knob(i + 1, name, destination, amount)


def k_cut(a=30):       return ("Cutoff", "filter1Cutoff", a)
def k_damp(a=30):      return ("Damp", "filter1Cutoff", -a)      # for sounds whose filter is already open
def k_res(a=30):       return ("Resonance", "filter1Resonance", a)
def k_bite(a=60):      return ("Bite", "filter1Drive", a)
def k_grit(a=60, kind=tube, tone=9000):   return ("Grit", "fxDriveAmount", a, lambda p: need_drive(p, kind, tone))
def k_space(a=45, size=70, decay=3.0):    return ("Space", "fxReverbMix", a, lambda p: need_reverb(p, size, decay))
def k_echo(a=40, division=eighthD):       return ("Echo", "fxDelayMix", a, lambda p: need_delay(p, division))
def k_wide(a=55):      return ("Wide", "fxChorusMix", a, need_chorus)
def k_phase(a=55):     return ("Phase", "fxPhaserMix", a, need_phaser)
def k_sub(a=50):       return ("Sub", "subLevel", a)
def k_noise(a=45, name="Noise"):          return (name, "noiseLevel", a)
def k_morph(n=1, a=45, name="Morph"):     return (name, f"osc{n}Position", a)
def k_warp(n=1, a=50, name="Warp"):       return (name, f"osc{n}WarpAmount", a)
def k_detune(n=1, a=30):                  return ("Detune", f"osc{n}UnisonDetune", a)
def k_motion(l=1, a=30, name="Motion"):   return (name, f"lfo{l}Rate", a)
def k_level(n, a, name):                  return (name, f"osc{n}Volume", a)


def room(p, r, mix=None):
    p.reverb(mix if mix is not None else r.u(10, 25, 5), r.u(30, 55, 5), r.u(0.6, 1.6, 0.2), r.u(40, 70, 10))


def hall(p, r, mix=None):
    p.reverb(mix if mix is not None else r.u(20, 45, 5), r.u(70, 100, 5), r.u(3, 9, 0.5), r.u(30, 70, 5))


def echo(p, r, mix=None, divisions=(eighthD, eighth, quarter, sixteenthD, quarterT, eighthT)):
    p.delay(r.pick(list(divisions)), r.u(25, 55, 5), mix if mix is not None else r.u(12, 30, 2), r.log(1800, 8000), r.chance(0.7))


def dirt(p, r, kinds=(tube, hard, fuzz, soft), low=15, high=55, tone=(2500, 12000)):
    p.drive(r.pick(list(kinds)), r.u(low, high, 5), r.log(*tone))


def vibrato(p, r, oscs=(1,), cents=12, lfo=2, rate=None, fade=None):
    """A late vibrato on Fine (pitch itself moves in semitones)."""
    p.lfo(lfo, sine, rate if rate is not None else r.u(4.5, 6.5, 0.25), 1)
    p.lfoFade(lfo, fade if fade is not None else r.u(250, 700, 50))

    for o in oscs:
        p.mod(lfo1 + lfo - 1, f"osc{o}Fine", round(cents / 2.0, 1))


def wave(r, tables, analog_positions=(sawPos, sawPos, squarePos, 80)):
    """A table and a position that suits it. A position means something different in each table: 100
    is a square in Analog and a 3 % needle in Pulse, so a recipe that draws its table draws the
    position to match."""
    table = r.pick(list(tables))

    if table == analog:
        return table, r.pick(list(analog_positions))

    low, high = {pulse: (0, 70), sync: (15, 60), fm: (15, 60), growl: (25, 65), metal: (10, 70), vowel: (0, 100), additive: (20, 70), sub: (0, 60)}[table]
    return table, r.u(low, high, 5)


def strike(p, r, burst=(4, 25), noise=(55, 90), kinds=(0, 0, 3, 2)):
    """What sets a comb ringing: a few milliseconds of a bright wave at the note's pitch and of noise.
    Noise alone is too quiet to ring a comb at a useful level."""
    p.osc(1, r.pick([analog, pulse]), r.pick([sawPos, 60, 85]), 1)
    p.noise(1, r.pick(list(kinds)), r.u(-20, 60, 20))
    p.env(2, 0, r.u(burst[0], burst[1]), 0, 15, r.u(0, 40, 20)).mod(env2, "osc1Volume", 99).mod(env2, "noiseLevel", r.u(noise[0], noise[1], 5))


LOWPASS = [ladder24, ladder24, ladder12, svfLow]

# Rhythms a held note is gated into, as sixteenths over a bar.
GATES = ["X.X.X.X.X.X.X.X.", "X.xxX.xxX.xxX.xx", "XxxxXxxxXxxxXxxx", "X..X..X.X..X..X.", "X.XxX.XxX.XxX.Xx", "XX.XX.XX.XX.XX.X",
         "X..xX..xX..xX.xx", "..X...X...X...X.", "X.X.XX.XX.X.XX.X", "XxX.XxX.XxX.XxXx", "X...X.X.X...X.Xx", "X.xX.xX.X.xX.xXx",
         "XXX.XXX.XXX.XXX.", "X..X.X..X..X.X..", "X.X..X.XX.X..X.X", "Xx.xXx.xXx.xXxXx", "X.XX..X.X.XX..Xx", ".X.X.X.X.X.X.X.X",
         "X.x.X.x.XxX.x.Xx", "XX..XX..XX..XXXx"]


# =============================================================================================
# Bass
# =============================================================================================

@kind("Bass", 9, "Lever Crank Ram Throttle Piston Plunger Gauge Valve Crusher Stamper Winch Jack")
def bass_sync_sweep(p, r):
    # A hard-synced saw whose sync falls with each note: the snarl of EBM sequencer basses.
    p.osc(1, sync, r.u(10, 40, 5)).subOsc(r.u(30, 55, 5), 0, r.pick([0, 1]))
    p.filter(1, r.pick([ladder24, ladder12]), r.log(600, 2200), r.u(10, 35, 5), r.u(0, 40, 10))
    p.env(2, 0, r.u(90, 320, 10), r.pick([0, 0, 10, 20]), r.u(80, 160, 20), r.u(60, 85, 5))
    p.mod(env2, "osc1Position", r.u(25, 55, 5)).mod(env2, "filter1Cutoff", r.u(8, 24, 2))
    p.amp(1, r.u(200, 500, 20), r.pick([0, 60, 80]), r.u(50, 110, 10)).mono(r.pick([0, 0, 40, 70]), r.chance(0.6))
    if r.chance(0.5): dirt(p, r, (tube, hard), 15, 40)
    p.level(-9)
    knobs(p, r, ("Sync", "osc1Position", 40), k_cut(28), k_grit(55, hard, 6000), k_sub(40), k_bite(60), k_space(35, 50, 1.2))


@kind("Bass", 9, "Pulse Relay Shunt Solenoid Breaker Contact Switch Coil Stator Rotor Armature")
def bass_pwm(p, r):
    # A pulse whose width an LFO keeps moving, over a sub: body music's sequencer bass.
    p.osc(1, pulse, r.u(15, 60, 5)).subOsc(r.u(30, 50, 5), 0, r.pick([0, 2]))
    if r.chance(0.4): p.osc(2, pulse, r.u(30, 75, 5), r.u(45, 65, 5)).pitch(2, r.pick([12, -12, 0]), r.u(4, 9))
    p.filter(1, r.pick([svfLow, ladder12, ladder24]), r.log(400, 1500), r.u(15, 45, 5), r.u(0, 30, 10))
    p.env(2, 0, r.u(100, 260, 20), 0, r.u(80, 140, 20), r.u(65, 85, 5)).mod(env2, "filter1Cutoff", r.u(15, 35, 5))
    p.lfo(1, r.pick([triangle, sine]), r.log(0.15, 2.5), 0).mod(lfo1, "osc1Position", r.u(15, 30, 5))
    p.amp(r.pick([0, 1, 2]), r.u(160, 400, 20), r.pick([0, 0, 70, 85]), r.u(50, 90, 10)).mono(0, r.chance(0.5))
    if r.chance(0.4): dirt(p, r, (tube, soft), 15, 35)
    p.level(-9)
    knobs(p, r, ("Width", "osc1Position", 35), k_cut(30), k_res(35), k_grit(55), k_sub(40), k_motion(1, 30))


@kind("Bass", 9, "Bore Shaft Drill Auger Tap Reamer Gouge Burr Chisel Awl Punch")
def bass_fm_pair(p, r):
    # Two-operator FM: oscillator 1 (heard at 1 %) bends oscillator 2's sine, most at the strike.
    ratio = r.pick([0, 0, 12, 12, 19, 24, 7])
    p.osc(1, analog, sinePos, 1).pitch(1, ratio)
    p.osc(2, analog, r.pick([sinePos, sinePos, 10, trianglePos])).warp(2, wFm, r.u(5, 25, 5))
    p.subOsc(r.u(25, 50, 5))
    p.env(2, 0, r.u(80, 380, 20), r.pick([0, 0, 10, 25]), r.u(80, 160, 20), r.u(60, 85, 5)).mod(env2, "osc2WarpAmount", r.u(25, 65, 5))
    p.mod(velocity, "osc2WarpAmount", r.u(8, 20, 2))
    p.filter(1, r.pick([ladder24, ladder12, svfLow]), r.log(900, 4500), r.u(5, 20, 5))
    p.amp(1, r.u(220, 600, 20), r.pick([0, 50, 75]), r.u(60, 120, 10)).mono(r.pick([0, 0, 50]), True)
    if r.chance(0.5): dirt(p, r, (tube, soft, hard), 15, 35, (3000, 9000))
    p.level(-9)
    knobs(p, r, ("Index", "osc2WarpAmount", 45), k_cut(25), k_grit(55), k_sub(40), k_space(30, 45, 1.0), k_bite(60))


@kind("Bass", 8, "Maw Gullet Jaw Mandible Fang Snarl Gnash Grip Vise Clench")
def bass_fold_wobble(p, r):
    # A growling wave folded harder as the filter opens, an LFO in time rocking both.
    p.osc(1, r.pick([growl, growl, sub, fm]), r.u(25, 65, 5)).warp(1, wFold, r.u(15, 40, 5)).subOsc(r.u(30, 50, 5))
    p.filter(1, ladder24, r.log(250, 700), r.u(25, 50, 5), r.u(20, 60, 10))
    p.lfoSync(1, r.pick([sine, triangle, sawDown]), r.pick([quarter, eighth, eighthD, quarterT, half, eighthT]), 1)
    p.mod(lfo1, "filter1Cutoff", r.u(10, 22, 2)).mod(lfo1, "osc1WarpAmount", r.u(10, 30, 5))
    p.amp(2, 300, r.u(80, 95, 5), r.u(60, 120, 10)).mono(r.pick([0, 30, 60]), True)
    dirt(p, r, (hard, tube, fuzz), 20, 45, (3000, 7000))
    p.level(-9)
    knobs(p, r, ("Fold", "osc1WarpAmount", 45), k_cut(28), ("Growl", "osc1Position", 35), k_sub(40), k_grit(40, hard))


@kind("Bass", 8, "Cellar Vault Crypt Undercroft Pit Sump Trench Culvert Shaft Bilge Hold")
def bass_rumble(p, r):
    # A thump with a dark room after it: the rumble under a techno kick.
    p.osc(1, r.pick([analog, sub]), r.pick([sinePos, 10, 20, trianglePos])).subOsc(r.u(30, 60, 10), 0, r.pick([0, 1]))
    p.env(2, 0, r.u(40, 120, 10), 0, 60, r.u(75, 90, 5)).mod(env2, "osc1Pitch", semis(r.pick([5, 7, 12, 12, 19])))
    p.filter(1, ladder24, r.log(140, 400), r.u(10, 35, 5), r.u(20, 60, 10)).mod(env2, "filter1Cutoff", r.u(10, 25, 5))
    p.amp(r.pick([1, 2, 4]), r.u(500, 1400, 100), 0, r.u(200, 400, 50), r.u(55, 75, 5))
    p.reverb(r.u(30, 50, 5), r.u(70, 95, 5), r.u(2.5, 6, 0.5), r.u(65, 90, 5))
    if r.chance(0.6): dirt(p, r, (tube, soft, hard), 20, 45, (800, 2500))
    p.level(-9)
    knobs(p, r, ("Tail", "fxReverbMix", 40), ("Thump", "filter1Drive", 60), k_cut(25), k_sub(40), k_grit(50, tube, 1500), k_res(40))


@kind("Bass", 8, "Girder Rail Sleeper Truss Rebar Ingot Billet Flange Bracket Stanchion")
def bass_metal(p, r):
    # The metal table's low partials over a sub: a bass with a clang in it.
    p.osc(1, metal, r.u(5, 45, 5)).subOsc(r.u(40, 60, 5), 0, r.pick([0, 1, 2]))
    if r.chance(0.5): p.osc(2, analog, r.pick([sawPos, squarePos]), r.u(40, 60, 5)).pitch(2, 0, r.u(-8, 8))
    p.filter(1, r.pick([ladder24, ladder12, ladderBand]), r.log(500, 2400), r.u(15, 40, 5), r.u(10, 50, 10))
    p.env(2, 0, r.u(90, 300, 10), 0, r.u(80, 140, 20), r.u(65, 85, 5))
    p.mod(env2, "filter1Cutoff", r.u(12, 30, 2)).mod(env2, "osc1Position", r.u(10, 35, 5))
    p.amp(1, r.u(200, 500, 20), r.pick([0, 0, 60, 80]), r.u(60, 110, 10)).mono(0, r.chance(0.5))
    dirt(p, r, (hard, tube, fuzz), 15, 45, (2500, 8000))
    p.level(-9)
    knobs(p, r, ("Clang", "osc1Position", 40), k_cut(28), k_grit(45, hard), k_sub(35), k_res(35), k_space(35, 45, 1.0))


@kind("Bass", 8, "Cable Hawser Tether Guy Halyard Stay Cord Tendon Sinew Strap")
def bass_string(p, r):
    # A plucked string an octave down, clipped: a picked bass with wire in it.
    p.osc(1, string, r.u(35, 75, 5), 90).warp(1, wOff, r.u(35, 70, 5))
    if r.chance(0.5): p.unison(1, 2, r.u(4, 10), r.u(20, 50, 10), r.u(0, 12, 4))
    p.subOsc(r.u(30, 55, 5), 0, r.pick([0, 1]))
    p.filter(1, r.pick([ladder24, ladder12, svfLow]), r.log(900, 4000), r.u(5, 25, 5), r.u(0, 40, 10))
    p.env(2, 0, r.u(100, 300, 20), 0, 120, 75).mod(env2, "filter1Cutoff", r.u(8, 22, 2)).mod(velocity, "osc1Position", r.u(10, 25, 5))
    p.amp(1, r.u(700, 2200, 100), r.pick([0, 0, 40]), r.u(80, 200, 20)).mono(0, False)
    dirt(p, r, (tube, hard, fuzz), 20, 55, (2500, 7000))
    p.level(-9)
    knobs(p, r, ("Bright", "osc1Position", 30), k_cut(25), k_grit(40, hard), k_sub(40), ("Ring", "osc1WarpAmount", 30), k_space(30, 45, 1.0))


@kind("Bass", 8, "Throat Tongue Larynx Gorge Mouth Growl Mutter Drawl Grunt")
def bass_vowel(p, r):
    # The vowel table read through as the note decays: a bass that says something.
    p.osc(1, vowel, r.u(0, 80, 5)).subOsc(r.u(40, 60, 5), 0, r.pick([0, 1]))
    if r.chance(0.4): p.osc(2, analog, sawPos, r.u(35, 55, 5)).pitch(2, r.pick([0, -12]), r.u(3, 9))
    p.env(2, r.pick([0, 0, 20, 60]), r.u(120, 500, 20), r.pick([0, 20, 40]), 150, r.u(40, 80, 10))
    p.mod(env2, "osc1Position", r.pick([1, -1]) * r.u(20, 50, 5))
    p.filter(1, r.pick([ladder24, ladder12]), r.log(700, 2600), r.u(10, 40, 5), r.u(10, 50, 10)).mod(env2, "filter1Cutoff", r.u(6, 18, 2))
    p.amp(2, r.u(250, 600, 50), r.pick([0, 70, 85]), r.u(60, 120, 10)).mono(r.pick([0, 40, 80]), True)
    dirt(p, r, (tube, hard), 20, 45, (3000, 8000))
    p.level(-9)
    knobs(p, r, ("Vowel", "osc1Position", 50), k_cut(25), k_grit(45, hard), k_sub(35), k_bite(60), k_res(35))


@kind("Bass", 8, "Duct Flue Conduit Manifold Pipe Sluice Nozzle Stack Vent")
def bass_comb(p, r):
    # A saw through a ladder and then a comb tuned off the note: a pipe resonating around the bass.
    p.osc(1, analog, r.pick([sawPos, sawPos, squarePos, 80])).subOsc(r.u(35, 55, 5))
    p.filter(1, ladder24, r.log(350, 1300), r.u(15, 40, 5), r.u(10, 40, 10))
    p.filter(2, comb, r.pick([196, 261.6, 261.6, 392, 523.3, 784]), r.u(35, 70, 5), 0, 100)   # at middle C: the key, a fourth under, a fifth or an octave over
    p.env(2, 0, r.u(100, 300, 20), 0, 120, 75).mod(env2, "filter1Cutoff", r.u(15, 35, 5))
    if r.chance(0.5): p.lfo(1, r.pick([sine, drift]), r.log(0.08, 0.8), 0).mod(lfo1, "filter2Cutoff", r.u(2, 6))
    p.amp(1, r.u(220, 500, 20), r.pick([0, 60, 80]), r.u(60, 110, 10)).mono(0, r.chance(0.5))
    if r.chance(0.5): dirt(p, r, (tube, hard), 15, 40)
    p.level(-9)
    knobs(p, r, ("Pipe", "filter2Resonance", 25), k_cut(28), ("Tune", "filter2Cutoff", 12), k_sub(40), k_grit(50), k_res(35))


@kind("Bass", 8, "Twin Split Fork Yoke Clamp Brace Tandem Couple Double")
def bass_dual_filter(p, r):
    # One oscillator into a low-pass for weight, the other into a band-pass for bite, side by side.
    p.osc(1, analog, r.pick([sawPos, squarePos, 80])).route(1, 0)
    p.osc(2, r.pick([analog, pulse, sync]), r.u(20, 70, 10), r.u(55, 75, 5)).pitch(2, r.pick([0, 12, 12, 7]), r.u(3, 10)).route(2, 100)
    p.subOsc(r.u(30, 50, 5)).parallel()
    p.filter(1, ladder24, r.log(200, 600), r.u(15, 40, 5), r.u(10, 40, 10))
    p.filter(2, r.pick([svfBand, ladderBand]), r.log(900, 3200), r.u(30, 60, 5), 0, r.pick([0, 50]))
    p.env(2, 0, r.u(90, 280, 10), 0, 120, r.u(65, 85, 5)).mod(env2, "filter1Cutoff", r.u(15, 30, 5)).mod(env2, "filter2Cutoff", r.u(8, 20, 2))
    p.amp(1, r.u(200, 450, 50), r.pick([0, 60, 80]), r.u(60, 100, 10)).mono(0, r.chance(0.5))
    dirt(p, r, (tube, hard), 15, 40)
    p.level(-9)
    knobs(p, r, ("Weight", "filter1Cutoff", 28), ("Bite", "filter2Cutoff", 20), k_grit(45, hard), k_sub(40), ("Edge", "filter2Resonance", 30))


@kind("Bass", 9, "Saw Rasp File Rip Tear Shear Cleave Hack Slash Notch")
def bass_detuned_saws(p, r):
    # Two saws a few cents apart through a driven ladder with a short filter envelope: the techno saw bass.
    p.osc(1, analog, sawPos).osc(2, analog, r.pick([sawPos, sawPos, squarePos]), r.u(60, 80, 5)).pitch(2, r.pick([0, 0, -12, 12]), r.u(5, 16))
    if r.chance(0.4): p.unison(1, 2, r.u(6, 14, 2), r.u(20, 50, 10))
    p.subOsc(r.u(25, 50, 5))
    p.filter(1, r.pick([ladder24, ladder24, ladder12]), r.log(180, 600), r.u(20, 55, 5), r.u(20, 60, 10), r.pick([0, 0, 30]))
    p.env(2, 0, r.u(70, 240, 10), r.pick([0, 0, 10]), r.u(80, 140, 20), r.u(70, 88, 6))
    p.mod(env2, "filter1Cutoff", r.u(20, 42, 2)).mod(velocity, "filter1Cutoff", r.u(5, 15, 5))
    p.amp(r.pick([0, 1]), r.u(150, 350, 50), r.pick([0, 0, 60, 80]), r.u(40, 90, 10)).mono(0, r.chance(0.4))
    if r.chance(0.6): dirt(p, r, (tube, hard, fuzz), 15, 45, (3500, 9000))
    p.level(-9)
    knobs(p, r, k_cut(28), k_res(30), k_grit(55, hard, 6000), k_sub(40), ("Beat", "osc2Fine", 20), k_bite(50))


@kind("Bass", 7, "Knock Thud Donk Bump Butt Shove Jolt Buck")
def bass_knock(p, r):
    # A short FM knock with a pitch drop: hard dance music's bouncing bass.
    p.osc(1, fm, r.u(20, 60, 5)).subOsc(r.u(20, 45, 5))
    p.env(2, 0, r.u(30, 90, 10), 0, 60, r.u(80, 92, 4)).mod(env2, "osc1Pitch", semis(r.pick([7, 12, 12, 19, 24])))
    p.env(3, 0, r.u(80, 220, 20), 0, 100, r.u(70, 85, 5)).mod(env3, "osc1Position", r.u(15, 40, 5))
    p.filter(1, r.pick([ladder24, svfLow, ladderBand]), r.log(700, 3000), r.u(10, 40, 5), r.u(0, 40, 10))
    p.amp(0, r.u(160, 380, 20), 0, r.u(60, 110, 10), r.u(55, 75, 5)).mono(0, False)
    dirt(p, r, (tube, hard, soft), 15, 45, (3000, 9000))
    p.level(-9)
    knobs(p, r, ("Knock", "osc1Position", 35), k_cut(25), k_grit(45, hard), k_sub(40), k_res(35), k_space(30, 40, 0.8))


@kind("Bass", 8, "Squelch Ooze Sludge Slurry Seep Mire Silt Dreg Slop")
def bass_squelch(p, r):
    # A 12 dB ladder with resonance, opened by the envelope and by how hard the key is struck.
    p.osc(1, *wave(r, [analog, analog, pulse], [sawPos, squarePos, 85])).subOsc(r.u(30, 50, 5), 0, r.pick([0, 1]))
    p.filter(1, r.pick([ladder12, svfLow]), r.log(160, 480), r.u(50, 75, 5), r.u(10, 40, 10), r.pick([0, 30]))
    p.env(2, 0, r.u(90, 260, 10), 0, r.u(80, 140, 20), r.u(70, 85, 5))
    p.mod(env2, "filter1Cutoff", r.u(25, 42, 2)).mod(velocity, "filter1Cutoff", r.u(8, 18, 2)).mod(velocity, "filter1Drive", r.u(20, 40, 10))
    p.amp(1, r.u(180, 380, 20), r.pick([0, 0, 60]), r.u(50, 90, 10)).mono(r.pick([0, 30, 60]), True)
    if r.chance(0.5): dirt(p, r, (tube, hard), 15, 35)
    p.level(-9)
    knobs(p, r, k_cut(28), k_res(22), k_grit(55, hard, 6000), k_sub(40), k_echo(30, sixteenthD), k_bite(50))


@kind("Bass", 7, "Byte Nibble Bit Glyph Cipher Token Packet Sector")
def bass_crushed(p, r):
    # A triangle or saw with its wave cut into a few steps: a bass out of a cheap chip.
    p.osc(1, analog, r.pick([trianglePos, sawPos, 50, 15])).warp(1, wCrush, r.u(45, 75, 5)).subOsc(r.u(30, 50, 5), 0, r.pick([0, 2]))
    p.filter(1, r.pick([svfLow, ladder12, ladder24]), r.log(500, 2600), r.u(10, 40, 5))
    p.env(2, 0, r.u(90, 280, 10), 0, 120, 75).mod(env2, "filter1Cutoff", r.u(10, 30, 5))
    p.amp(1, r.u(180, 420, 20), r.pick([0, 60, 80]), r.u(50, 100, 10)).mono(0, r.chance(0.5))
    if r.chance(0.5): p.drive(crush, r.u(15, 40, 5), r.log(4000, 12000))
    p.level(-9)
    knobs(p, r, ("Steps", "osc1WarpAmount", 30), k_cut(28), k_grit(45, crush, 8000), k_sub(40), k_res(35), k_echo(30, sixteenthD))


# =============================================================================================
# Sub
# =============================================================================================

@kind("Sub", 8, "Floor Bedrock Footing Slab Plinth Base Keel Sole")
def sub_pure(p, r):
    # A sine and the sub an octave under it, held: nothing but the bottom.
    p.osc(1, analog, r.pick([sinePos, sinePos, 5, 10])).subOsc(r.u(30, 70, 10), r.pick([0, 0, 1]), r.pick([0, 0, 1]))
    p.filter(1, ladder24, r.log(300, 1200), 5)
    p.amp(r.pick([1, 3, 6, 12]), r.u(300, 800, 100), r.u(85, 100, 5), r.u(60, 220, 20)).mono(r.pick([0, 0, 40, 90]), True)
    p.level(-9)
    knobs(p, r, ("Octave", "subLevel", 30), ("Warm", "osc1Position", 25), k_grit(50, tube, 2000), ("Push", "filter1Drive", 60))


@kind("Sub", 8, "Boom Swell Surge Heave Tide Billow Roll")
def sub_long_decay(p, r):
    # A sine with a quick pitch drop at the front and a long fall: the long boom of electro.
    p.osc(1, r.pick([analog, sub]), r.pick([sinePos, 10, 15])).subOsc(r.pick([10, 20, 40]))
    p.env(2, 0, r.u(25, 80, 5), 0, 60, r.u(80, 92, 4)).mod(env2, "osc1Pitch", semis(r.pick([5, 7, 12, 12, 19])))
    p.filter(1, ladder24, r.log(400, 2000), 5, r.u(0, 40, 10))
    p.amp(1, r.u(900, 3200, 100), 0, r.u(200, 600, 100), r.u(40, 70, 10)).mono(r.pick([0, 0, 60]), False)
    if r.chance(0.6): dirt(p, r, (tube, soft), 15, 40, (1200, 4000))
    p.level(-9)
    knobs(p, r, ("Drop", "osc1Pitch", semis(7)), k_grit(50, tube, 2500), ("Warm", "osc1Position", 25), ("Push", "filter1Drive", 60))


@kind("Sub", 7, "Pressure Ballast Load Burden Mass Bulk Heft Tonnage")
def sub_saturated(p, r):
    # A rounded square or driven sine: a sub that small speakers still show.
    p.osc(1, sub, r.u(20, 75, 5)).subOsc(r.u(20, 50, 10), 0, r.pick([0, 2]))
    p.filter(1, r.pick([ladder24, ladder12]), r.log(180, 700), r.u(5, 25, 5), r.u(20, 70, 10))
    p.amp(r.pick([1, 2, 5]), r.u(300, 700, 100), r.u(70, 95, 5), r.u(60, 160, 20)).mono(r.pick([0, 50, 100]), True)
    p.drive(r.pick([tube, soft, hard]), r.u(20, 50, 5), r.log(900, 3500))
    p.level(-9)
    knobs(p, r, ("Square", "osc1Position", 30), ("Open", "filter1Cutoff", 25), k_grit(40), ("Push", "filter1Drive", 30))


@kind("Sub", 7, "Tick Tap Nudge Poke Prod Flick Dab")
def sub_click(p, r):
    # A sub with a short burst of noise at its front, so the note is heard to start.
    p.osc(1, analog, r.pick([sinePos, 10, trianglePos])).subOsc(r.u(30, 60, 10))
    p.noise(1, r.pick([0, 1, 3]), r.u(-20, 40, 20))
    p.env(2, 0, r.u(8, 40, 4), 0, 30, 90).mod(env2, "noiseLevel", r.u(25, 55, 5))
    p.filter(1, r.pick([ladder24, svfLow]), r.log(700, 3000), 5)
    p.amp(1, r.u(250, 900, 50), r.pick([0, 70, 90]), r.u(60, 160, 20)).mono(0, r.chance(0.5))
    p.level(-9)
    knobs(p, r, ("Click", "noiseLevel", 30), ("Warm", "osc1Position", 25), k_grit(50, tube, 2500), ("Octave", "subLevel", 30))


@kind("Sub", 7, "Slide Slope Lurch Sag Dip Sink Slump")
def sub_glide(p, r):
    # A sub that always slides to the next note.
    p.osc(1, r.pick([analog, sub]), r.pick([sinePos, 10, 20, trianglePos])).subOsc(r.u(20, 50, 10), 0, r.pick([0, 1]))
    p.filter(1, ladder24, r.log(250, 900), r.u(5, 30, 5), r.u(0, 40, 10))
    p.amp(r.pick([2, 5, 10]), 400, r.u(85, 100, 5), r.u(80, 200, 20)).mono(r.u(80, 320, 20), r.chance(0.5)).glideAlways()
    if r.chance(0.5): dirt(p, r, (tube, soft), 15, 40, (1000, 3000))
    p.level(-9)
    knobs(p, r, ("Warm", "osc1Position", 25), ("Push", "filter1Drive", 60), k_grit(50, tube, 2000), ("Octave", "subLevel", 30))


@kind("Sub", 7, "Tremor Quiver Shudder Throb Shiver Flutter")
def sub_tremor(p, r):
    # A sub with a slow beat in its level: the room shaking.
    p.osc(1, analog, r.pick([sinePos, 10, trianglePos])).osc(2, analog, sinePos, r.u(60, 80, 5)).pitch(2, 0, r.u(6, 30, 2))
    p.subOsc(r.u(20, 50, 10))
    p.filter(1, ladder24, r.log(200, 800), 5, r.u(0, 30, 10))
    p.lfo(1, r.pick([sine, triangle]), r.log(1.5, 9), 0).mod(lfo1, "ampLevel", r.u(4, 12, 2))
    p.amp(r.pick([5, 20, 60]), 500, r.u(85, 100, 5), r.u(100, 400, 50)).mono(r.pick([0, 60]), True)
    p.level(-9)
    knobs(p, r, ("Beat", "osc2Fine", 15), k_motion(1, 25, "Rate"), ("Depth", "ampLevel", -12), k_grit(50, tube, 2000))


# =============================================================================================
# Reese
# =============================================================================================

@kind("Reese", 9, "Undertow Riptide Eddy Vortex Gyre Whirl Churn Swirl Maelstrom")
def reese_notch(p, r):
    # Detuned saws with a notch drifting through them.
    p.osc(1, analog, sawPos).osc(2, analog, sawPos).pitch(2, 0, r.u(8, 22, 2))
    p.unison(1, r.pick([2, 2, 3]), r.u(8, 18, 2), r.u(30, 60, 10))
    p.filter(1, ladder24, r.log(500, 1800), r.u(10, 30, 5), r.u(10, 50, 10))
    p.filter(2, svfNotch, r.log(300, 2000), r.u(30, 65, 5))
    p.lfo(1, r.pick([sine, triangle, drift]), r.log(0.05, 0.6), 0).mod(lfo1, "filter2Cutoff", r.u(8, 20, 2))
    p.subOsc(r.u(25, 45, 5)).amp(r.pick([3, 5, 10]), 400, 90, r.u(120, 300, 20)).mono(r.pick([0, 40, 80]), True)
    if r.chance(0.5): dirt(p, r, (tube, hard), 15, 40)
    p.level(-9)
    knobs(p, r, k_cut(28), ("Notch", "filter2Cutoff", 20), ("Detune", "osc2Fine", 20), k_grit(50), k_sub(35), k_motion(1, 30))


@kind("Reese", 8, "Hum Buzz Whirr Purr Drone Thrum Murmur")
def reese_pulse(p, r):
    # Two pulses of different widths, detuned, their widths moving against each other.
    p.osc(1, pulse, r.u(10, 50, 5)).osc(2, pulse, r.u(40, 85, 5)).pitch(2, r.pick([0, 0, -12]), r.u(8, 20, 2))
    p.lfo(1, r.pick([triangle, sine]), r.log(0.08, 0.8), 0).mod(lfo1, "osc1Position", r.u(12, 25)).mod(lfo1, "osc2Position", -r.u(10, 20))
    p.filter(1, r.pick([ladder24, ladderBand, svfLow]), r.log(450, 1600), r.u(20, 50, 5), r.u(10, 40, 10))
    p.subOsc(r.u(30, 50, 5)).amp(5, 400, 90, r.u(120, 300, 20)).mono(r.pick([0, 50]), True)
    if r.chance(0.5): p.phaser(r.u(25, 45, 5), r.log(0.05, 0.4), r.u(40, 70, 10))
    p.level(-9)
    knobs(p, r, k_cut(28), ("Width", "osc1Position", 30), k_grit(55, hard), k_sub(35), k_phase(50), k_res(30))


@kind("Reese", 8, "Hornet Wasp Locust Cicada Mantis Scarab Weevil Midge")
def reese_growl(p, r):
    # The growl table in unison through a band of filters and a clipper: a reese with teeth.
    p.osc(1, growl, r.u(25, 75, 5)).unison(1, r.pick([2, 3, 4]), r.u(10, 24, 2), r.u(40, 80, 10))
    p.lfoSync(1, r.pick([sine, triangle]), r.pick([bar1, bars2, half, bars4]), 0).mod(lfo1, "osc1Position", r.u(15, 35, 5))
    p.filter(1, r.pick([ladder24, svfLow]), r.log(600, 2400), r.u(20, 45, 5), r.u(20, 50, 10)).mod(lfo1, "filter1Cutoff", r.u(5, 14))
    p.subOsc(r.u(30, 45, 5)).amp(5, 500, 90, r.u(140, 300, 20)).mono(r.pick([0, 40, 90]), True)
    dirt(p, r, (hard, fuzz, tube), 20, 45, (3000, 8000))
    p.level(-9)
    knobs(p, r, k_bite(60), ("Growl", "osc1Position", 35), k_detune(1, 25), k_sub(35), k_grit(40, hard), k_res(30))


@kind("Reese", 8, "Shear Strain Torque Stress Warp Buckle Flex Creep")
def reese_fm(p, r):
    # A detuned pair where one bends the other's phase: the beating turns into grit.
    p.osc(1, analog, r.pick([sinePos, trianglePos, sawPos])).pitch(1, 0, -r.u(5, 14))
    p.osc(2, analog, r.pick([sawPos, sawPos, trianglePos])).pitch(2, 0, r.u(5, 14)).warp(2, wFm, r.u(15, 50, 5))
    p.unison(2, 2, r.u(6, 14, 2), r.u(30, 60, 10))
    p.lfo(1, r.pick([sine, drift]), r.log(0.05, 0.5), 0).mod(lfo1, "osc2WarpAmount", r.u(10, 25, 5))
    p.filter(1, ladder24, r.log(500, 2000), r.u(15, 40, 5), r.u(10, 40, 10))
    p.subOsc(r.u(30, 45, 5)).amp(5, 400, 90, r.u(120, 300, 20)).mono(r.pick([0, 60]), True)
    p.level(-9)
    knobs(p, r, k_cut(28), ("Bend", "osc2WarpAmount", 40), k_grit(55, hard), k_sub(35), ("Detune", "osc2Fine", 20), k_res(30))


@kind("Reese", 8, "Drain Siphon Funnel Spout Gutter Weir Flume")
def reese_comb(p, r):
    # Detuned saws through a comb an LFO pulls about: a reese inside a pipe.
    p.osc(1, analog, sawPos).unison(1, r.pick([2, 3, 4]), r.u(10, 22, 2), r.u(40, 70, 10))
    if r.chance(0.5): p.osc(2, *wave(r, [analog, pulse], [sawPos, squarePos, 80]), 60).pitch(2, -12, r.u(4, 10))
    if r.chance(0.5): p.filter(1, comb, r.pick([98, 130.8, 196, 261.6]), r.u(40, 70, 5))
    else: p.filter(1, comb, r.pick([261.6, 392, 523.3]), r.u(40, 70, 5), 0, 100)
    p.filter(2, ladder24, r.log(700, 2600), r.u(10, 30, 5), r.u(0, 40, 10))
    p.lfo(1, r.pick([sine, triangle, drift]), r.log(0.05, 0.5), 0).mod(lfo1, "filter1Cutoff", r.u(3, 9))
    p.subOsc(r.u(30, 45, 5)).amp(5, 400, 90, r.u(120, 300, 20)).mono(r.pick([0, 60]), True)
    if r.chance(0.5): dirt(p, r, (tube, hard), 15, 35)
    p.level(-9)
    knobs(p, r, ("Cutoff", "filter2Cutoff", 28), ("Pipe", "filter1Resonance", 25), k_detune(1, 25), k_sub(35), k_grit(50), k_motion(1, 30))


@kind("Reese", 8, "Abyss Chasm Rift Gulf Fissure Hollow Void Deep")
def reese_wide(p, r):
    # Wide unison saws high-passed out of the sub's way, the sub straight to the output: wide on top, mono below.
    p.osc(1, analog, sawPos).unison(1, r.pick([4, 5, 6]), r.u(14, 28, 2), r.u(70, 100, 10))
    p.osc(2, analog, r.pick([sawPos, squarePos]), r.u(50, 70, 5)).pitch(2, r.pick([-12, 0, 12]), r.u(4, 12))
    p.filter(1, ladder24, r.log(700, 2800), r.u(10, 35, 5), r.u(0, 40, 10))
    p.filter(2, svfHigh, r.log(90, 180), 10)
    p.lfo(1, r.pick([sine, drift]), r.log(0.05, 0.4), 0).mod(lfo1, "filter1Cutoff", r.u(4, 12))
    p.subOsc(r.u(35, 55, 5), 0, 0, True).amp(8, 500, 90, r.u(150, 400, 50)).mono(r.pick([0, 60, 120]), True)
    p.chorus(r.u(20, 40, 5), r.u(0.2, 0.8, 0.1), r.u(30, 60, 10))
    p.level(-9)
    knobs(p, r, k_cut(28), k_detune(1, 25), k_grit(55, hard), k_sub(30), ("Wide", "fxChorusMix", 40), k_res(30))


# =============================================================================================
# Acid
# =============================================================================================

def acid_tail(p, r):
    if r.chance(0.5): p.delay(r.pick([sixteenthD, eighthD, eighth]), r.u(30, 50, 5), r.u(10, 22, 2), r.log(2500, 7000))


@kind("Acid", 9, "Etch Solvent Reagent Caustic Lye Bleach Brine Vitriol Tincture")
def acid_classic(p, r):
    # One saw or square into a resonant 24 dB ladder, slides between overlapping notes, accents from velocity.
    p.osc(1, analog, r.pick([sawPos, sawPos, squarePos, 85]))
    p.filter(1, ladder24, r.log(180, 520), r.u(68, 90, 2), r.u(10, 50, 10), r.pick([20, 30, 40]))
    p.env(2, 0, r.u(110, 420, 10), 0, r.u(80, 160, 20), r.u(65, 85, 5))
    p.mod(env2, "filter1Cutoff", r.u(28, 48, 2)).mod(velocity, "filter1Cutoff", r.u(8, 20, 2)).mod(velocity, "filter1Resonance", r.u(5, 12))
    p.mono(r.u(30, 110, 10), True).amp(1, r.u(160, 320, 20), r.u(50, 80, 10), r.u(40, 80, 10))
    if r.chance(0.6): dirt(p, r, (hard, tube, fuzz), 20, 55, (3500, 9000))
    acid_tail(p, r)
    p.level(-9)
    knobs(p, r, k_cut(32), k_res(20), ("Env Mod", "filter1Drive", 55), k_grit(50, hard, 6000), k_echo(30, sixteenthD), k_space(30, 50, 1.4))


@kind("Acid", 8, "Rubber Gum Latex Resin Pitch Tar Putty Gel")
def acid_soft(p, r):
    # The 12 dB ladder: rounder, rubbery acid.
    p.osc(1, analog, r.pick([sawPos, squarePos, 80, 90])).subOsc(r.pick([1, 20, 30]))
    p.filter(1, ladder12, r.log(220, 700), r.u(60, 88, 4), r.u(0, 30, 10), r.pick([20, 30, 50]))
    p.env(2, 0, r.u(100, 300, 10), 0, 120, r.u(70, 88, 6))
    p.mod(env2, "filter1Cutoff", r.u(28, 46, 2)).mod(velocity, "filter1Cutoff", r.u(8, 18, 2))
    p.mono(r.u(20, 80, 10), True).amp(1, r.u(160, 300, 20), r.u(40, 70, 10), r.u(40, 80, 10))
    acid_tail(p, r)
    p.level(-9)
    knobs(p, r, k_cut(32), k_res(22), k_grit(60, tube, 7000), k_sub(40), k_echo(30, sixteenthD), k_bite(55))


@kind("Acid", 8, "Needle Syringe Lancet Probe Stylus Pin Barb")
def acid_pulse(p, r):
    # A narrow pulse into the ladder, its width tied to the filter envelope.
    p.osc(1, pulse, r.u(20, 75, 5))
    p.filter(1, ladder24, r.log(200, 650), r.u(65, 88, 3), r.u(10, 40, 10), 30)
    p.env(2, 0, r.u(110, 340, 10), 0, 120, r.u(65, 85, 5))
    p.mod(env2, "filter1Cutoff", r.u(28, 45, 3)).mod(env2, "osc1Position", r.pick([-1, 1]) * r.u(15, 30, 5)).mod(velocity, "filter1Cutoff", r.u(8, 16, 2))
    p.mono(r.u(30, 100, 10), True).amp(1, r.u(160, 300, 20), r.u(50, 75, 5), r.u(40, 80, 10))
    if r.chance(0.5): dirt(p, r, (tube, hard), 20, 45)
    acid_tail(p, r)
    p.level(-9)
    knobs(p, r, k_cut(32), k_res(20), ("Width", "osc1Position", 25), k_grit(50, hard, 6000), k_echo(30, sixteenthD), k_bite(50))


@kind("Acid", 8, "Fifth Dyad Pair Tandem Diode Bridge Junction")
def acid_fifth(p, r):
    # Two oscillators a fifth or an octave apart into one ladder: acid with a chord in it.
    p.osc(1, analog, r.pick([sawPos, squarePos])).osc(2, analog, r.pick([sawPos, squarePos]), r.u(50, 75, 5)).pitch(2, r.pick([7, 7, 12, -12, 5]), r.u(0, 8))
    p.filter(1, ladder24, r.log(200, 600), r.u(65, 86, 3), r.u(10, 50, 10), 30)
    p.env(2, 0, r.u(110, 360, 10), 0, 140, r.u(65, 85, 5)).mod(env2, "filter1Cutoff", r.u(28, 44, 2)).mod(velocity, "filter1Cutoff", r.u(8, 16, 2))
    p.mono(r.u(30, 100, 10), True).amp(1, r.u(180, 320, 20), r.u(50, 80, 10), r.u(40, 80, 10))
    if r.chance(0.6): dirt(p, r, (tube, hard, fuzz), 20, 50)
    acid_tail(p, r)
    p.level(-9)
    knobs(p, r, k_cut(32), k_res(20), ("Second", "osc2Volume", -45), k_grit(50, hard, 6000), k_echo(30, sixteenthD), k_bite(50))


@kind("Acid", 7, "Whine Squeal Shriek Keen Whistle Howl Yelp")
def acid_bandpass(p, r):
    # The ladder's band-pass, nearly singing: thin, nasal acid for the top of a track.
    p.osc(1, analog, r.pick([sawPos, squarePos])).unison(1, r.pick([1, 2]), r.u(6, 14, 2), 40)
    p.filter(1, ladderBand, r.log(300, 900), r.u(70, 90, 4), r.u(0, 40, 10), 40)
    p.env(2, 0, r.u(120, 400, 10), 0, 140, r.u(60, 80, 5)).mod(env2, "filter1Cutoff", r.u(25, 40, 3)).mod(velocity, "filter1Cutoff", r.u(8, 16, 2))
    p.mono(r.u(40, 120, 10), True).amp(1, 260, r.u(60, 85, 5), r.u(50, 90, 10))
    dirt(p, r, (tube, hard, fuzz), 25, 55)
    p.delay(r.pick([eighthD, sixteenthD, eighth]), r.u(35, 55, 5), r.u(14, 26, 2), r.log(3000, 8000))
    p.level(-9)
    knobs(p, r, k_cut(30), k_res(15), k_grit(35, hard), k_echo(30), k_space(35, 60, 2.0), k_bite(50))


# =============================================================================================
# Lead
# =============================================================================================

@kind("Lead", 9, "Arc Flare Torch Beacon Spark Lance Jet Tracer")
def lead_sync(p, r):
    # A sawtooth hard-synced and swept: the Sync warp's ratio moves smoothly, so the sweep has no steps.
    p.osc(1, analog, sawPos).warp(1, wSync, r.u(10, 40, 5))
    if r.chance(0.5): p.osc(2, analog, r.pick([sawPos, squarePos]), r.u(40, 60, 5)).pitch(2, r.pick([-12, 0]), r.u(4, 10))
    p.env(2, r.pick([0, 0, 30, 120]), r.u(250, 900, 50), r.pick([0, 20, 40]), 200, r.u(40, 70, 10)).mod(env2, "osc1WarpAmount", r.u(20, 50, 5))
    p.filter(1, r.pick([ladder24, ladder12]), r.log(2500, 9000), r.u(10, 30, 5))
    p.mono(r.u(30, 90, 10), True).amp(2, 300, 85, r.u(120, 260, 20))
    vibrato(p, r, (1,), r.u(8, 16, 2))
    if r.chance(0.6): dirt(p, r, (tube, hard, fuzz), 15, 45, (5000, 11000))
    echo(p, r)
    p.level(-9)
    knobs(p, r, ("Sync", "osc1WarpAmount", 45), k_cut(22), k_grit(55, hard), k_echo(30), k_space(35), k_res(35))


@kind("Lead", 9, "Reed Pipe Chanter Fife Shawm Horn Cornet")
def lead_pwm(p, r):
    # A pulse with moving width and a late vibrato: hollow, vocal, cuts without harshness.
    p.osc(1, pulse, r.u(10, 55, 5))
    if r.chance(0.5): p.osc(2, pulse, r.u(30, 70, 5), r.u(45, 65, 5)).pitch(2, r.pick([12, -12, 0]), r.u(5, 10))
    p.lfo(1, r.pick([triangle, sine]), r.log(0.2, 3), 0).mod(lfo1, "osc1Position", r.u(15, 30, 5))
    p.filter(1, r.pick([ladder24, ladder12, svfLow]), r.log(1800, 6000), r.u(10, 35, 5), r.u(0, 30, 10), r.pick([0, 30, 50]))
    p.env(2, 0, r.u(200, 600, 50), r.u(20, 50, 10), 200, 60).mod(env2, "filter1Cutoff", r.u(6, 18, 2))
    p.mono(r.u(20, 90, 10), True).amp(r.pick([2, 5, 12]), 300, 85, r.u(120, 260, 20))
    vibrato(p, r, (1,), r.u(8, 18, 2))
    echo(p, r)
    if r.chance(0.5): hall(p, r, r.u(12, 25))
    p.level(-9)
    knobs(p, r, ("Width", "osc1Position", 30), k_cut(25), k_grit(55), k_echo(30), k_space(35), k_res(35))


@kind("Lead", 9, "Filament Element Cathode Anode Electrode Plasma Ion")
def lead_fm(p, r):
    # FM between two oscillators with the index on an envelope and the wheel: glassy to harsh.
    ratio = r.pick([0, 12, 12, 19, 24, 7])
    p.osc(1, analog, sinePos, 1).pitch(1, ratio)
    p.osc(2, analog, r.pick([sinePos, trianglePos, 15])).warp(2, wFm, r.u(10, 35, 5))
    if r.chance(0.4): p.unison(2, 2, r.u(5, 12), r.u(30, 60, 10))
    p.env(2, 0, r.u(200, 700, 50), r.u(20, 60, 10), 250, 60).mod(env2, "osc2WarpAmount", r.u(15, 45, 5)).mod(modWheel, "osc2WarpAmount", 30)
    p.filter(1, r.pick([ladder12, svfLow]), r.log(3500, 12000), 10)
    p.mono(r.u(20, 80, 10), True).amp(2, 300, 85, r.u(120, 260, 20))
    vibrato(p, r, (2,), r.u(8, 16, 2))
    if r.chance(0.5): dirt(p, r, (tube, soft, hard), 15, 40, (5000, 11000))
    echo(p, r)
    p.level(-9)
    knobs(p, r, ("Index", "osc2WarpAmount", 40), k_damp(30), k_grit(55), k_echo(30), k_space(35))


@kind("Lead", 9, "Signal Flare Strobe Glare Blaze Dazzle Surge Burst")
def lead_unison(p, r):
    # Saws in wide unison with a fast filter envelope: the rave lead, for one note at a time or chords.
    p.osc(1, analog, sawPos).unison(1, r.pick([4, 5, 6, 7]), r.u(14, 30, 2), r.u(70, 100, 10))
    if r.chance(0.6): p.osc(2, analog, r.pick([sawPos, squarePos]), r.u(45, 65, 5)).pitch(2, r.pick([12, -12, 7]), r.u(3, 9))
    p.filter(1, r.pick([ladder24, ladder12, svfLow]), r.log(1500, 5000), r.u(10, 30, 5), r.u(0, 30, 10), 30)
    p.env(2, 0, r.u(150, 500, 50), r.u(10, 40, 10), 200, r.u(60, 80, 10)).mod(env2, "filter1Cutoff", r.u(10, 25, 5))
    p.amp(r.pick([1, 3, 8]), 400, r.u(70, 90, 10), r.u(140, 320, 20))
    if r.chance(0.5): p.mono(r.u(20, 80, 10), True)
    p.chorus(r.u(15, 35, 5), r.u(0.3, 0.9, 0.1), r.u(30, 50, 10))
    echo(p, r)
    if r.chance(0.5): hall(p, r, r.u(12, 25))
    p.level(-9)
    knobs(p, r, k_cut(25), k_detune(1, 25), k_grit(55, hard), k_echo(30), k_space(35), k_res(35))


@kind("Lead", 9, "Scream Wail Howl Cry Shriek Roar Bellow")
def lead_screamer(p, r):
    # A sawtooth and a fifth above it, band-passed and pushed into fuzz, with feedback-heavy delay.
    p.osc(1, r.pick([analog, growl, sync]), r.pick([sawPos, 50, 30])).osc(2, analog, sawPos, r.u(40, 65, 5)).pitch(2, r.pick([7, 12, 7, 19]), r.u(2, 8))
    p.filter(1, r.pick([svfBand, ladderBand, ladder24]), r.log(1200, 3500), r.u(30, 60, 5), r.u(20, 50, 10), r.pick([30, 50]))
    p.lfo(1, r.pick([randomShape, drift]), r.log(2, 8), 0).mod(lfo1, "filter1Cutoff", r.u(3, 9))
    p.mono(r.u(40, 120, 10), True).amp(r.pick([2, 5]), 300, 85, r.u(120, 260, 20))
    vibrato(p, r, (1, 2), r.u(10, 20, 2))
    p.drive(r.pick([fuzz, hard, fuzz]), r.u(40, 65, 5), r.log(4000, 8000))
    p.delay(r.pick([eighthD, quarter, quarterT]), r.u(45, 65, 5), r.u(16, 28, 2), r.log(2500, 6000))
    p.level(-9)
    knobs(p, r, k_cut(25), ("Scream", "fxDriveAmount", 30), ("Fifth", "osc2Volume", -45), k_echo(30), k_space(35), k_res(30))


@kind("Lead", 7, "Tongue Voice Cant Chant Call Hail Shout")
def lead_vowel(p, r):
    # The vowel table with its position on the wheel and an LFO: a lead that talks.
    p.osc(1, vowel, r.u(0, 80, 10)).unison(1, r.pick([1, 2, 3]), r.u(6, 14, 2), r.u(30, 60, 10))
    p.lfoSync(1, r.pick([triangle, sine, sawUp]), r.pick([bar1, half, quarter, bars2]), 1).mod(lfo1, "osc1Position", r.u(15, 40, 5))
    p.mod(modWheel, "osc1Position", 40)
    p.filter(1, r.pick([ladder12, svfLow]), r.log(3000, 9000), r.u(10, 25, 5))
    p.mono(r.u(30, 100, 10), True).amp(r.pick([3, 8, 15]), 400, 85, r.u(140, 280, 20))
    dirt(p, r, (tube, hard), 20, 45, (4000, 10000))
    echo(p, r)
    p.level(-9)
    knobs(p, r, ("Vowel", "osc1Position", 55), k_cut(22), k_grit(40, hard), k_echo(30), k_space(35))


@kind("Lead", 7, "Ribbon Thread Strand Tendril Vein Trace")
def lead_soft_square(p, r):
    # A square or triangle, a little chorus and a long slide: a calm lead for slow lines.
    p.osc(1, analog, r.pick([squarePos, 85, trianglePos, 45])).osc(2, analog, r.pick([sinePos, trianglePos]), r.u(40, 60, 5)).pitch(2, r.pick([12, 12, 24, -12]))
    p.filter(1, r.pick([ladder24, svfLow]), r.log(1200, 4000), r.u(5, 25, 5), 0, 40)
    p.mono(r.u(80, 260, 20), True).glideAlways().amp(r.pick([8, 20, 40]), 400, 85, r.u(200, 500, 50))
    vibrato(p, r, (1, 2), r.u(8, 16, 2))
    p.chorus(r.u(20, 40, 5), r.u(0.3, 0.9, 0.1), 40)
    echo(p, r)
    hall(p, r, r.u(15, 30, 5))
    p.level(-9)
    knobs(p, r, k_cut(25), ("Shape", "osc1Position", -30), k_grit(55), k_echo(30), k_space(35))


# =============================================================================================
# Stab
# =============================================================================================

@kind("Stab", 8, "Strike Jab Thrust Lunge Slash Swipe Cut Chop")
def stab_octaves(p, r):
    # Saws stacked in octaves with a snap of filter: a one-note stab that fills a bar.
    p.osc(1, analog, sawPos).unison(1, r.pick([2, 3, 4]), r.u(8, 20, 2), r.u(50, 90, 10))
    p.osc(2, analog, r.pick([sawPos, squarePos]), r.u(55, 75, 5)).pitch(2, r.pick([12, -12]), r.u(3, 9))
    if r.chance(0.5): p.osc(3, *wave(r, [analog, pulse], [sawPos, squarePos, 80]), r.u(40, 55, 5)).pitch(3, r.pick([24, 19, -12, 7]))
    p.filter(1, r.pick([ladder24, ladder12, svfLow]), r.log(500, 2200), r.u(15, 40, 5), r.u(0, 30, 10))
    p.env(2, 0, r.u(80, 260, 10), 0, r.u(80, 160, 20), r.u(70, 88, 6)).mod(env2, "filter1Cutoff", r.u(18, 38, 2)).mod(velocity, "filter1Cutoff", r.u(5, 12))
    p.amp(0, r.u(160, 420, 20), 0, r.u(100, 220, 20))
    if r.chance(0.5): dirt(p, r, (tube, hard), 15, 40)
    if r.chance(0.6): echo(p, r)
    room(p, r) if r.chance(0.5) else hall(p, r, r.u(15, 30, 5))
    p.level(-9)
    knobs(p, r, k_cut(28), k_detune(1, 25), k_grit(55, hard), k_space(35), k_echo(30), k_res(30))


@kind("Stab", 8, "Shard Splinter Sliver Flake Chip Fragment Spall")
def stab_fm(p, r):
    # A short FM hit, bright at the front: glass and tin.
    p.osc(1, analog, sinePos, 1).pitch(1, r.pick([7, 12, 19, 22, 24]))
    p.osc(2, analog, r.pick([sinePos, trianglePos])).warp(2, wFm, r.u(5, 20, 5))
    if r.chance(0.5): p.osc(3, fm, r.u(20, 70, 10), r.u(40, 60, 5)).pitch(3, r.pick([12, 0, -12]), r.u(3, 8))
    p.env(2, 0, r.u(60, 220, 10), 0, 120, r.u(75, 90, 5)).mod(env2, "osc2WarpAmount", r.u(30, 70, 5)).mod(velocity, "osc2WarpAmount", r.u(8, 18, 2))
    p.filter(1, r.pick([ladder12, svfLow, svfHigh]), r.log(250, 6000), 10)
    p.amp(0, r.u(160, 480, 20), 0, r.u(100, 240, 20))
    if r.chance(0.6): echo(p, r)
    room(p, r) if r.chance(0.5) else hall(p, r, r.u(15, 30, 5))
    p.level(-9)
    knobs(p, r, ("Index", "osc2WarpAmount", 40), k_cut(25), k_grit(55, hard), k_space(35), k_echo(30))


@kind("Stab", 8, "Bark Snap Bite Nip Snarl Yap Bay")
def stab_brass(p, r):
    # Saws through a ladder that opens a moment after the strike: a brass section's short note.
    p.osc(1, analog, sawPos).unison(1, r.pick([2, 3, 4]), r.u(8, 16, 2), r.u(40, 70, 10))
    p.osc(2, analog, r.pick([sawPos, 80]), r.u(55, 75, 5)).pitch(2, r.pick([0, -12, 7]), r.u(4, 10))
    p.filter(1, ladder24, r.log(300, 900), r.u(10, 30, 5), r.u(10, 40, 10), 30)
    p.env(2, r.u(10, 45, 5), r.u(120, 320, 20), r.u(0, 30, 10), 150, r.u(55, 75, 5)).mod(env2, "filter1Cutoff", r.u(25, 42, 3)).mod(velocity, "filter1Cutoff", r.u(5, 14))
    p.amp(r.pick([2, 5, 10]), r.u(200, 450, 50), r.pick([0, 0, 30]), r.u(100, 200, 20))
    if r.chance(0.5): dirt(p, r, (tube, hard), 15, 35)
    room(p, r) if r.chance(0.5) else hall(p, r, r.u(15, 30, 5))
    p.level(-9)
    knobs(p, r, k_cut(28), ("Blare", "filter1Resonance", 35), k_grit(55, hard), k_space(35), k_detune(1, 25), k_echo(30))


@kind("Stab", 7, "Shout Bark Yell Hey Hup Cry Call")
def stab_vowel(p, r):
    # A vowel thrown open and shut: a crowd's shout in one key.
    p.osc(1, vowel, r.u(0, 60, 10)).unison(1, r.pick([2, 3, 4]), r.u(8, 18, 2), r.u(50, 90, 10))
    if r.chance(0.5): p.osc(2, analog, sawPos, r.u(35, 55, 5)).pitch(2, r.pick([0, -12]), r.u(4, 9))
    p.env(2, 0, r.u(90, 300, 10), 0, 140, r.u(50, 80, 10)).mod(env2, "osc1Position", r.pick([-1, 1]) * r.u(20, 45, 5))
    p.filter(1, r.pick([ladder12, svfLow, svfBand]), r.log(1500, 5000), r.u(10, 30, 5))
    p.amp(r.pick([0, 3, 8]), r.u(180, 420, 20), 0, r.u(100, 220, 20))
    dirt(p, r, (tube, hard), 15, 45)
    room(p, r) if r.chance(0.5) else hall(p, r, r.u(15, 30, 5))
    p.level(-9)
    knobs(p, r, ("Vowel", "osc1Position", 45), k_cut(25), k_grit(40, hard), k_space(35), k_echo(30), k_detune(1, 25))


@kind("Stab", 8, "Clang Slam Bash Crash Smash Whack Wham")
def stab_metal_comb(p, r):
    # Metal partials and noise through a comb: a struck sheet for the offbeat.
    p.osc(1, metal, r.u(30, 90, 5)).noise(r.u(15, 40, 5), r.pick([0, 2, 3]))
    if r.chance(0.5): p.osc(2, *wave(r, [analog, pulse], [sawPos, squarePos, 80]), r.u(45, 65, 5)).pitch(2, r.pick([0, -12, 7]))
    p.filter(1, comb, r.log(120, 900), r.u(45, 75, 5), 0, r.pick([0, 50, 100]))
    p.filter(2, r.pick([svfHigh, svfBand, ladder24]), r.log(250, 2500), r.u(10, 30, 5))
    p.env(2, 0, r.u(70, 200, 10), 0, 120, 85).mod(env2, "filter1Cutoff", r.pick([-1, 1]) * r.u(5, 15)).mod(env2, "osc1Position", r.u(10, 30, 5))
    p.amp(0, r.u(180, 450, 20), 0, r.u(120, 260, 20))
    if r.chance(0.6): dirt(p, r, (hard, tube, fuzz), 15, 45)
    room(p, r) if r.chance(0.6) else hall(p, r, r.u(15, 30, 5))
    p.level(-9)
    knobs(p, r, ("Ring", "filter1Resonance", 22), ("Tone", "filter2Cutoff", 25), k_grit(50, hard), k_space(35), ("Clang", "osc1Position", 30))


@kind("Stab", 7, "Pulse Blip Flash Blink Glint Flicker Wink")
def stab_pulse_delay(p, r):
    # A pulse stab with its width snapping shut, into a delay: the dub techno hit, one note.
    p.osc(1, pulse, r.u(10, 50, 5)).unison(1, r.pick([2, 3]), r.u(8, 16, 2), r.u(50, 80, 10))
    p.env(2, 0, r.u(90, 260, 10), 0, 140, r.u(70, 85, 5)).mod(env2, "osc1Position", r.u(15, 35, 5)).mod(env2, "filter1Cutoff", r.u(15, 32, 3))
    p.filter(1, r.pick([ladder24, svfLow, svfBand]), r.log(400, 1600), r.u(20, 45, 5))
    p.amp(0, r.u(180, 400, 20), 0, r.u(120, 240, 20))
    p.delay(r.pick([quarterD, eighthD, quarterT, eighth]), r.u(45, 65, 5), r.u(22, 38, 2), r.log(1500, 4500))
    hall(p, r, r.u(15, 35, 5))
    p.level(-9)
    knobs(p, r, k_cut(28), ("Width", "osc1Position", 30), ("Echo", "fxDelayMix", 30), k_space(35), k_grit(55), k_res(30))


@kind("Stab", 7, "Hit Blow Impact Punch Wallop Clout Belt")
def stab_orchestral(p, r):
    # Everything at once for an instant: octaves, a fifth, noise for the bows and a room. The old sampler hit, synthesised.
    p.osc(1, analog, sawPos).unison(1, r.pick([4, 5, 6]), r.u(14, 26, 2), r.u(70, 100, 10))
    p.osc(2, r.pick([analog, pulse]), r.pick([sawPos, 30, 50])).pitch(2, r.pick([-12, 12]), r.u(5, 10)).unison(2, 2, r.u(8, 14, 2))
    p.osc(3, r.pick([analog, additive]), r.u(40, 80, 10), r.u(45, 60, 5)).pitch(3, r.pick([7, 19, 24]))
    p.noise(r.u(15, 30, 5), 1)
    p.filter(1, r.pick([ladder12, svfLow]), r.log(1800, 5000), r.u(5, 20, 5))
    p.env(2, 0, r.u(120, 300, 20), 0, 200, 70).mod(env2, "filter1Cutoff", r.u(10, 22, 2)).mod(env2, "noiseLevel", r.u(10, 25, 5))
    p.amp(r.pick([2, 8, 15]), r.u(260, 600, 20), 0, r.u(160, 320, 20))
    hall(p, r, r.u(25, 40, 5))
    p.level(-9)
    knobs(p, r, k_cut(25), k_detune(1, 25), k_grit(55, hard), k_space(35), ("Bows", "noiseLevel", 30), k_echo(30))


# =============================================================================================
# Pluck
# =============================================================================================

@kind("Pluck", 8, "Wire Strand Filament Fibre Thread Hair Whisker Bristle")
def pluck_string_pair(p, r):
    # Two plucked strings an interval apart, one brighter: a zither with a dark room.
    p.osc(1, string, r.u(40, 85, 5), 90).warp(1, wOff, r.u(25, 60, 5))
    p.osc(2, string, r.u(25, 65, 5), r.u(55, 80, 5)).warp(2, wOff, r.u(25, 60, 5)).pitch(2, r.pick([12, -12, 7, 0, 19]), r.u(0, 8))
    if r.chance(0.5): p.unison(1, 2, r.u(3, 9), r.u(40, 80, 10), r.u(0, 20, 5))
    p.filter(1, r.pick([ladder24, ladder12, svfLow]), r.log(2500, 9000), r.u(5, 20, 5))
    p.mod(velocity, "osc1Position", r.u(10, 25, 5))
    p.amp(0, r.u(1200, 3000, 100), 0, r.u(300, 800, 100))
    if r.chance(0.7): echo(p, r)
    hall(p, r, r.u(15, 35, 5))
    p.level(-9)
    knobs(p, r, ("Bright", "osc1Position", 30), ("Ring", "osc1WarpAmount", 30), k_grit(55, hard), k_space(35), k_echo(30), k_cut(20))


@kind("Pluck", 8, "Pick Pin Tack Stud Nib Tine Prong Tooth")
def pluck_comb(p, r):
    # A burst of a bright wave and noise into a comb tuned to the key: the string is the filter.
    strike(p, r, (8, 30), (50, 80), (0, 1, 0))
    p.filter(1, comb, 261.6, r.u(94, 100), 0, 100)
    p.filter(2, r.pick([ladder12, svfLow, ladder24]), r.log(2000, 8000), r.u(5, 20, 5), 0, r.pick([0, 50]))
    p.amp(0, r.u(900, 2600, 100), 0, r.u(250, 700, 50))
    if r.chance(0.6): echo(p, r)
    hall(p, r, r.u(12, 30))
    p.level(-9)
    knobs(p, r, ("Damp", "filter1Resonance", -15), ("Bright", "filter2Cutoff", 22), k_grit(55, hard), k_space(35), k_echo(30))


@kind("Pluck", 8, "Drop Bead Pearl Dew Globule Blot Speck Dot")
def pluck_ladder(p, r):
    # The synth pluck: bright oscillators, a ladder shut fast, delay behind.
    p.osc(1, *wave(r, [analog, pulse, sync], [sawPos, squarePos]))
    p.osc(2, *wave(r, [analog, pulse], [sawPos, squarePos]), r.u(45, 70, 5)).pitch(2, r.pick([12, 7, -12, 0, 19]), r.u(0, 9))
    p.filter(1, r.pick([ladder24, ladder24, svfLow]), r.log(250, 800), r.u(20, 55, 5), 0, r.pick([30, 50, 70]))
    p.env(2, 0, r.u(90, 320, 10), 0, r.u(120, 240, 20), r.u(75, 90, 5)).mod(env2, "filter1Cutoff", r.u(30, 50, 2)).mod(velocity, "filter1Cutoff", r.u(6, 16, 2))
    p.amp(0, r.u(260, 800, 20), 0, r.u(200, 400, 50))
    echo(p, r)
    if r.chance(0.6): hall(p, r, r.u(12, 30))
    p.level(-9)
    knobs(p, r, k_cut(28), k_res(30), k_grit(55, hard), k_space(35), k_echo(30), ("Second", "osc2Volume", -45))


@kind("Pluck", 8, "Chime Ping Tink Tinkle Plink Clink Tick")
def pluck_fm(p, r):
    # A short FM note: marimba to thumb piano as the ratio changes.
    p.osc(1, analog, sinePos, 1).pitch(1, r.pick([0, 12, 16, 19, 22, 24]))
    p.osc(2, analog, sinePos).warp(2, wFm, r.u(0, 12, 4))
    if r.chance(0.5): p.osc(3, analog, r.pick([sinePos, trianglePos]), r.u(40, 60, 5)).pitch(3, r.pick([12, 24, -12]))
    p.env(2, 0, r.u(40, 200, 10), 0, 100, r.u(78, 92, 4)).mod(env2, "osc2WarpAmount", r.u(20, 60, 5)).mod(velocity, "osc2WarpAmount", r.u(8, 20, 2))
    p.filter(1, r.pick([ladder12, svfLow]), r.log(3000, 12000), 5, 0, 30)
    p.amp(0, r.u(350, 1400, 50), 0, r.u(250, 600, 50), r.u(55, 75, 5))
    if r.chance(0.6): echo(p, r)
    hall(p, r, r.u(12, 30))
    p.level(-9)
    knobs(p, r, ("Index", "osc2WarpAmount", 40), k_damp(30), k_grit(55, hard), k_space(35), k_echo(30))


@kind("Pluck", 7, "Mute Damp Choke Hush Stifle Gag Muffle")
def pluck_muted(p, r):
    # A damped string: a short ring, a low-pass close above it, tight for sixteenths.
    p.osc(1, string, r.u(20, 55, 5), 90).warp(1, wOff, r.u(5, 25, 5))
    if r.chance(0.5): p.osc(2, analog, r.pick([sawPos, squarePos, trianglePos]), r.u(35, 55, 5)).pitch(2, r.pick([0, 12, -12]))
    p.filter(1, r.pick([ladder24, svfLow]), r.log(700, 2400), r.u(10, 35, 5), r.u(0, 30, 10), 50)
    p.env(2, 0, r.u(60, 180, 10), 0, 100, 85).mod(env2, "filter1Cutoff", r.u(10, 28, 2)).mod(velocity, "filter1Cutoff", r.u(6, 14, 2))
    p.amp(0, r.u(200, 500, 20), 0, r.u(80, 180, 20))
    if r.chance(0.5): dirt(p, r, (tube, hard), 15, 40)
    if r.chance(0.6): echo(p, r, r.u(10, 20, 2), (sixteenthD, eighthD, eighth))
    room(p, r)
    p.level(-9)
    knobs(p, r, k_cut(28), ("Ring", "osc1WarpAmount", 35), k_grit(55, hard), k_space(35), k_echo(30), ("Bright", "osc1Position", 30))


# =============================================================================================
# Pad
# =============================================================================================

def pad_amp(p, r):
    p.amp(r.u(500, 2400, 100), 1000, r.u(85, 100, 5), r.u(1500, 4500, 100))


@kind("Pad", 8, "Haze Mist Vapour Smog Fume Murk Pall Shroud")
def pad_pwm(p, r):
    # Pulses with their widths moving at different rates: a pad that never sits still.
    p.osc(1, pulse, r.u(10, 50, 5)).unison(1, r.pick([2, 3, 4]), r.u(8, 18, 2), r.u(70, 100, 10))
    p.osc(2, pulse, r.u(30, 70, 5), r.u(55, 75, 5)).pitch(2, r.pick([0, 12, -12]), r.u(4, 10))
    p.lfo(1, triangle, r.log(0.05, 0.5), 0).mod(lfo1, "osc1Position", r.u(15, 30, 5))
    p.lfo(2, sine, r.log(0.05, 0.4), 0).mod(lfo2, "osc2Position", r.u(12, 25))
    p.filter(1, r.pick(LOWPASS), r.log(900, 3500), r.u(10, 30, 5))
    pad_amp(p, r)
    if r.chance(0.5): p.chorus(r.u(25, 45, 5), r.u(0.2, 0.8, 0.1), 50)
    hall(p, r, r.u(30, 55, 5))
    p.level(-9)
    knobs(p, r, k_cut(28), ("Width", "osc1Position", 30), k_space(35), k_motion(1, 30), k_grit(55), k_detune(1, 25))


@kind("Pad", 8, "Frost Rime Glaze Sheen Lustre Gleam Aura")
def pad_fm_glass(p, r):
    # Slow FM between sines, the index drifting: glass that breathes.
    p.osc(1, analog, sinePos, r.pick([1, 1, 30])).pitch(1, r.pick([12, 19, 24, 7]))
    p.osc(2, analog, sinePos).warp(2, wFm, r.u(5, 25, 5)).unison(2, r.pick([2, 3, 4]), r.u(6, 14, 2), r.u(70, 100, 10))
    if r.chance(0.5): p.osc(3, analog, r.pick([sinePos, trianglePos]), r.u(40, 60, 5)).pitch(3, r.pick([12, -12, 7])).unison(3, 2, r.u(6, 12, 2), 100)
    p.lfo(1, r.pick([sine, drift, triangle]), r.log(0.04, 0.4), 0).mod(lfo1, "osc2WarpAmount", r.u(8, 25))
    p.filter(1, r.pick([ladder12, svfLow]), r.log(2000, 8000), r.u(5, 20, 5))
    pad_amp(p, r)
    if r.chance(0.5): p.chorus(r.u(25, 45, 5), r.u(0.2, 0.8, 0.1), 50)
    hall(p, r, r.u(35, 55, 5))
    p.level(-9)
    knobs(p, r, ("Index", "osc2WarpAmount", 35), k_damp(30), k_space(35), k_motion(1, 30), k_echo(30), k_wide(40))


@kind("Pad", 8, "Sheet Plate Foil Leaf Panel Membrane Film Skin")
def pad_metal(p, r):
    # The metal table in unison, high-passed and slow: a shimmer of partials.
    p.osc(1, metal, r.u(10, 70, 5)).unison(1, r.pick([3, 4, 6]), r.u(8, 20, 2), r.u(80, 100, 10))
    p.osc(2, r.pick([analog, additive, vowel]), r.u(0, 60, 10), r.u(45, 65, 5)).pitch(2, r.pick([0, -12, 12]), r.u(4, 9)).unison(2, 2, r.u(6, 12, 2), 100)
    p.lfo(1, r.pick([drift, sine, triangle]), r.log(0.03, 0.3), 0).mod(lfo1, "osc1Position", r.u(10, 30, 5))
    p.filter(1, r.pick([ladder12, svfLow, svfBand]), r.log(1200, 5000), r.u(10, 30, 5))
    if r.chance(0.5): p.filter(2, svfHigh, r.log(200, 800), 10)
    pad_amp(p, r)
    if r.chance(0.5): p.phaser(r.u(25, 45, 5), r.log(0.04, 0.3), r.u(40, 65, 5))
    hall(p, r, r.u(35, 55, 5))
    p.level(-9)
    knobs(p, r, k_cut(25), ("Partials", "osc1Position", 35), k_space(35), k_motion(1, 30), k_detune(1, 25), k_grit(55))


@kind("Pad", 8, "Dusk Gloom Dim Eclipse Umbra Shade Twilight Nightfall")
def pad_dark_sweep(p, r):
    # Saws under a low filter that a very slow LFO opens and closes: the pad at the back of a dark track.
    p.osc(1, analog, sawPos).unison(1, r.pick([3, 4, 5, 6]), r.u(10, 22, 2), r.u(70, 100, 10))
    p.osc(2, analog, r.pick([sawPos, squarePos, trianglePos]), r.u(50, 70, 5)).pitch(2, r.pick([-12, 0, 7, 12]), r.u(4, 10))
    p.filter(1, ladder24, r.log(350, 1200), r.u(15, 45, 5), r.u(0, 30, 10))
    p.lfo(1, r.pick([sine, triangle, drift]), r.log(0.03, 0.2), 0).mod(lfo1, "filter1Cutoff", r.u(8, 20, 2))
    if r.chance(0.5): p.noise(r.u(8, 16, 2), 1, r.u(-60, 0, 20))
    pad_amp(p, r)
    if r.chance(0.5): p.chorus(r.u(20, 40, 5), r.u(0.2, 0.7, 0.1), 50)
    hall(p, r, r.u(30, 55, 5))
    p.level(-9)
    knobs(p, r, k_cut(28), k_res(35), k_space(35), k_motion(1, 30), k_detune(1, 25), k_grit(55))


@kind("Pad", 8, "Breath Sigh Exhale Draught Gust Waft Whisper")
def pad_breath(p, r):
    # Noise through a band-pass that follows the key, under a soft tone: a pad made mostly of air.
    p.set("osc1On", 0).noise(r.u(45, 70, 5), r.pick([0, 1]), r.u(-20, 40, 20))
    p.osc(2, r.pick([analog, additive, vowel]), r.u(0, 40, 10), r.u(45, 65, 5)).unison(2, r.pick([2, 3]), r.u(6, 14, 2), 100).route(2, 100)
    p.parallel()
    p.filter(1, r.pick([svfBand, ladderBand]), r.pick([523.3, 1046.5, 261.6]), r.u(55, 85, 5), 0, 100)
    p.filter(2, r.pick([ladder12, svfLow]), r.log(900, 3500), 10)
    p.lfo(1, r.pick([drift, sine]), r.log(0.05, 0.5), 0).mod(lfo1, "filter1Cutoff", r.u(2, 6)).mod(lfo1, "noiseLevel", r.u(5, 15, 5))
    pad_amp(p, r)
    hall(p, r, r.u(35, 55, 5))
    p.level(-9)
    knobs(p, r, ("Air", "noiseLevel", 30), ("Tone", "filter2Cutoff", 25), k_space(35), ("Whistle", "filter1Resonance", 14), k_motion(1, 30), k_echo(30))


@kind("Pad", 8, "Fifth Vigil Watch Wake Rite Mass Office")
def pad_fifths(p, r):
    # Three oscillators as root, fifth and octave: one key is an open chord.
    t = r.pick([analog, analog, additive, pulse, vowel])
    pos = sawPos if t == analog else r.u(20, 70, 10)
    p.osc(1, t, pos).unison(1, r.pick([2, 3, 4]), r.u(8, 18, 2), r.u(70, 100, 10))
    p.osc(2, t, pos, r.u(55, 75, 5)).pitch(2, 7, r.u(0, 6)).unison(2, 2, r.u(6, 14, 2), 100)
    p.osc(3, r.pick([analog, additive]), r.u(0, 60, 10), r.u(45, 65, 5)).pitch(3, r.pick([12, -12]), r.u(3, 8))
    p.filter(1, r.pick(LOWPASS), r.log(700, 3000), r.u(10, 30, 5))
    p.lfo(1, r.pick([sine, triangle, drift]), r.log(0.04, 0.3), 0).mod(lfo1, "filter1Cutoff", r.u(5, 14))
    pad_amp(p, r)
    if r.chance(0.6): p.chorus(r.u(25, 45, 5), r.u(0.2, 0.8, 0.1), 50)
    hall(p, r, r.u(30, 55, 5))
    p.level(-9)
    knobs(p, r, k_cut(28), ("Fifth", "osc2Volume", -50), k_space(35), k_detune(1, 25), k_motion(1, 30), k_grit(55))


@kind("Pad", 8, "Rust Corrosion Scale Tarnish Verdigris Patina Oxide Slag")
def pad_industrial(p, r):
    # A growling wave in unison, driven, then filtered and drowned in a hall: a pad with dirt under its nails.
    p.osc(1, r.pick([growl, sync, fm]), r.u(20, 65, 5)).unison(1, r.pick([3, 4, 5]), r.u(10, 22, 2), r.u(70, 100, 10))
    if r.chance(0.6): p.osc(2, r.pick([metal, analog, pulse]), r.u(20, 70, 10), r.u(40, 60, 5)).pitch(2, r.pick([-12, 0, 12, 7]), r.u(4, 9))
    p.lfo(1, r.pick([drift, sine, triangle]), r.log(0.04, 0.4), 0).mod(lfo1, "osc1Position", r.u(10, 30, 5))
    p.filter(1, r.pick([ladder24, svfLow, ladderBand]), r.log(700, 2600), r.u(15, 45, 5), r.u(20, 60, 10))
    pad_amp(p, r)
    p.drive(r.pick([hard, tube, fuzz, crush]), r.u(20, 45, 5), r.log(2500, 7000))
    hall(p, r, r.u(35, 55, 5))
    p.level(-9)
    knobs(p, r, k_cut(28), ("Growl", "osc1Position", 35), k_space(35), k_grit(40, hard), k_motion(1, 30), k_detune(1, 25))


@kind("Pad", 8, "Hymn Canticle Vesper Requiem Litany Dirge Lament")
def pad_formant(p, r):
    # Saws through two band-passes side by side, set to a vowel's formants and drifting: a choir without words.
    f1, f2 = r.pick([(800, 1150), (400, 1600), (300, 2250), (450, 800), (325, 700), (600, 1000), (500, 1500)])
    p.osc(1, analog, sawPos).unison(1, r.pick([4, 5, 6]), r.u(10, 20, 2), r.u(80, 100, 10)).route(1, 50)
    if r.chance(0.5): p.osc(2, pulse, r.u(20, 60, 10), r.u(45, 65, 5)).pitch(2, r.pick([-12, 0, 12]), r.u(4, 9)).route(2, 50)
    p.parallel()
    p.filter(1, svfBand, f1, r.u(55, 75, 5)).filter(2, svfBand, f2, r.u(55, 75, 5))
    p.lfo(1, r.pick([drift, sine]), r.log(0.05, 0.4), 0).mod(lfo1, "filter1Cutoff", r.u(2, 6)).mod(lfo1, "filter2Cutoff", -r.u(2, 6))
    pad_amp(p, r)
    if r.chance(0.5): p.chorus(r.u(25, 45, 5), r.u(0.2, 0.8, 0.1), 50)
    hall(p, r, r.u(35, 55, 5))
    p.level(-9)
    knobs(p, r, ("Mouth", "filter1Cutoff", 12), ("Vowel", "filter2Cutoff", -12), k_space(35), k_detune(1, 25), k_motion(1, 30), k_grit(55))


# =============================================================================================
# Drone
# =============================================================================================

def drone_amp(p, r):
    p.amp(r.u(1500, 4000, 100), 1000, 100, r.u(3000, 6000, 500))


@kind("Drone", 8, "Vigil Watch Wake Stillness Standstill Stasis Limbo Torpor")
def drone_beating(p, r):
    # Sines and triangles a few cents and a fifth apart: nothing moves but the beats between them.
    shape = r.pick([sinePos, sinePos, trianglePos, 15])
    p.osc(1, analog, shape).unison(1, r.pick([2, 3]), r.u(3, 12), 100, 0, r.u(10, 40, 10))
    p.osc(2, analog, shape, r.u(60, 80, 5)).pitch(2, r.pick([7, 12, 19, -12]), r.u(2, 9))
    p.osc(3, r.pick([analog, additive]), r.u(0, 30, 10), r.u(40, 60, 5)).pitch(3, r.pick([12, 24, 7, 0]), -r.u(2, 9))
    p.filter(1, ladder24, r.log(400, 2500), r.u(5, 30, 5))
    p.lfo(1, r.pick([drift, sine]), r.log(0.02, 0.15), 0).mod(lfo1, "osc3Volume", r.u(10, 25, 5))
    drone_amp(p, r)
    hall(p, r, r.u(30, 50, 5))
    p.level(-9)
    knobs(p, r, ("Beat", "osc1UnisonDetune", 25), k_cut(25), k_space(35), k_grit(55), ("High", "osc3Volume", 30), k_motion(1, 30))


@kind("Drone", 8, "Duct Shaft Flue Stack Culvert Bore Tunnel Adit")
def drone_comb(p, r):
    # Noise and a quiet wave blown through a comb tuned to the key: a note out of air in a pipe.
    p.osc(1, r.pick([analog, pulse, additive]), r.pick([sawPos, 40, 70]), r.u(40, 65, 5)).noise(r.u(60, 90, 5), r.pick([0, 1, 1]), r.u(-40, 20, 20))
    p.filter(1, comb, r.pick([261.6, 261.6, 392, 523.3]), r.u(86, 96), 0, 100)
    p.filter(2, r.pick([ladder24, svfLow]), r.log(600, 4000), r.u(10, 35, 5))
    p.lfo(1, drift, r.log(0.03, 0.3), 0).mod(lfo1, "filter2Cutoff", r.u(5, 15)).mod(lfo1, "noiseLevel", r.u(5, 15, 5))
    drone_amp(p, r)
    hall(p, r, r.u(30, 50, 5))
    p.level(-9)
    knobs(p, r, ("Blow", "noiseLevel", 25), ("Cutoff", "filter2Cutoff", 25), k_space(35), ("Ring", "filter1Resonance", 3), k_grit(55), k_motion(1, 30))


@kind("Drone", 8, "Reactor Core Pile Furnace Crucible Kiln Smelter Retort")
def drone_fm(p, r):
    # FM at a ratio that is no harmonic, the index wandering: a drone that grinds against itself.
    p.osc(1, analog, sinePos, r.pick([1, 1, 25])).pitch(1, r.pick([5, 7, 14, 17, 21, -5]), r.u(-30, 30, 5))
    p.osc(2, analog, r.pick([sinePos, trianglePos])).warp(2, wFm, r.u(10, 40, 5)).unison(2, r.pick([1, 2, 3]), r.u(4, 12), 100, 0, r.u(10, 40, 10))
    p.lfo(1, r.pick([drift, sine]), r.log(0.02, 0.2), 0).mod(lfo1, "osc2WarpAmount", r.u(10, 30, 5))
    p.lfo(2, r.pick([sine, triangle]), r.log(0.02, 0.1), 0).mod(lfo2, "filter1Cutoff", r.u(5, 14))
    p.filter(1, r.pick([ladder24, svfLow, ladderBand]), r.log(500, 3000), r.u(10, 45, 5), r.u(0, 40, 10))
    p.subOsc(r.u(20, 45, 5))
    drone_amp(p, r)
    if r.chance(0.5): dirt(p, r, (tube, hard), 15, 40, (2500, 7000))
    hall(p, r, r.u(30, 50, 5))
    p.level(-9)
    knobs(p, r, ("Index", "osc2WarpAmount", 35), k_cut(25), k_space(35), k_sub(35), k_grit(55), k_motion(1, 30))


@kind("Drone", 8, "Generator Dynamo Turbine Compressor Idler Motor Rotor Alternator")
def drone_machine(p, r):
    # A narrow pulse and a hum of harmonics with a fast shake in their level: a machine left running.
    p.osc(1, pulse, r.u(50, 95, 5)).osc(2, additive, r.u(5, 40, 5), r.u(55, 75, 5)).pitch(2, r.pick([0, -12, 7]), r.u(2, 8))
    p.lfo(1, r.pick([sine, triangle]), r.log(6, 28), 0).mod(lfo1, "ampLevel", r.u(3, 9)).mod(lfo1, "filter1Cutoff", r.u(1, 4))
    p.lfo(2, drift, r.log(0.05, 0.4), 0).mod(lfo2, "lfo1Rate", r.u(3, 9)).mod(lfo2, "osc1Position", r.u(5, 15, 5))
    p.filter(1, r.pick([ladder24, comb, ladderBand]), r.log(120, 900), r.u(25, 60, 5), r.u(0, 40, 10))
    p.subOsc(r.u(20, 45, 5))
    p.amp(r.u(600, 2500, 100), 1000, 100, r.u(1500, 4000, 500))
    if r.chance(0.6): dirt(p, r, (tube, hard, crush), 15, 40, (2000, 6000))
    hall(p, r, r.u(20, 40, 5))
    p.level(-9)
    knobs(p, r, ("Speed", "lfo1Rate", 20), k_cut(25), k_space(35), k_sub(35), k_grit(55), ("Hum", "osc2Volume", 25))


@kind("Drone", 8, "Throat Gullet Monk Cantor Elder Seer Hermit")
def drone_throat(p, r):
    # A low vowel through a resonant band that moves slowly: a throat singer in a tank.
    p.osc(1, vowel, r.u(0, 100, 10)).unison(1, r.pick([2, 3, 4]), r.u(5, 14), 100, 0, r.u(10, 40, 10))
    p.osc(2, r.pick([growl, analog, pulse]), r.u(20, 70, 10), r.u(40, 60, 5)).pitch(2, r.pick([-12, 0, 7]), r.u(2, 8))
    p.lfo(1, r.pick([drift, sine]), r.log(0.03, 0.2), 0).mod(lfo1, "osc1Position", r.u(15, 40, 5))
    p.lfo(2, r.pick([sine, triangle]), r.log(0.03, 0.15), 0).mod(lfo2, "filter1Cutoff", r.u(6, 16, 2))
    p.filter(1, r.pick([svfBand, ladderBand, ladder24]), r.log(400, 1800), r.u(30, 60, 5), r.u(0, 30, 10))
    drone_amp(p, r)
    hall(p, r, r.u(30, 50, 5))
    p.level(-9)
    knobs(p, r, ("Vowel", "osc1Position", 45), k_cut(22), k_space(35), k_grit(55), k_res(25), k_motion(2, 30))


@kind("Drone", 8, "Dread Menace Omen Threat Malice Unease Foreboding Portent")
def drone_cluster(p, r):
    # Three notes a semitone and a tritone apart, in unison and drifting: the drone under a bad scene.
    t = r.pick([analog, analog, pulse, additive, growl])
    pos = r.pick([sawPos, trianglePos, 80]) if t == analog else r.u(20, 60, 10)
    p.osc(1, t, pos).unison(1, r.pick([2, 3, 4]), r.u(6, 16, 2), 100, 0, r.u(20, 60, 10))
    p.osc(2, t, pos, r.u(55, 75, 5)).pitch(2, r.pick([1, 1, 13, -11]), r.u(0, 8))
    p.osc(3, t, pos, r.u(45, 65, 5)).pitch(3, r.pick([6, 6, 11, -6, 18]), -r.u(0, 8))
    p.filter(1, r.pick([ladder24, svfLow]), r.log(350, 1800), r.u(10, 40, 5), r.u(0, 40, 10))
    p.lfo(1, r.pick([drift, sine]), r.log(0.02, 0.15), 0).mod(lfo1, "filter1Cutoff", r.u(6, 16, 2)).mod(lfo1, "osc2Volume", -r.u(10, 25, 5))
    drone_amp(p, r)
    hall(p, r, r.u(35, 55, 5))
    p.level(-9)
    knobs(p, r, k_cut(25), ("Semitone", "osc2Volume", -50), k_space(35), k_detune(1, 25), k_grit(55), k_motion(1, 30))


# =============================================================================================
# Bell
# =============================================================================================

@kind("Bell", 9, "Toll Peal Knell Chime Carillon Tocsin Angelus")
def bell_fm(p, r):
    # FM with the modulator off the harmonics: the partials of cast metal, decaying from the strike.
    p.osc(1, analog, sinePos, 1).pitch(1, r.pick([9, 14, 16, 17, 21, 23]), r.u(-20, 20, 5))
    p.osc(2, analog, sinePos).warp(2, wFm, r.u(5, 20, 5))
    p.osc(3, analog, sinePos, r.u(35, 55, 5)).pitch(3, r.pick([12, 19, 24, 0]), r.u(0, 6))
    p.env(2, 0, r.u(600, 2600, 100), 0, 1500, r.u(60, 80, 5)).mod(env2, "osc2WarpAmount", r.u(20, 55, 5)).mod(velocity, "osc2WarpAmount", r.u(8, 20, 2))
    p.filter(1, r.pick([ladder12, svfLow]), r.log(4000, 14000), 5)
    p.amp(0, r.u(1800, 5000, 100), 0, r.u(1500, 3500, 100), r.u(55, 75, 5))
    if r.chance(0.5): echo(p, r)
    hall(p, r, r.u(25, 45, 5))
    p.level(-9)
    knobs(p, r, ("Strike", "osc2WarpAmount", 35), k_cut(20), k_space(35), k_echo(30), k_grit(55), ("Hum", "osc3Volume", 30))


@kind("Bell", 8, "Tube Rod Bar Rail Pipe Girder Beam")
def bell_tubular(p, r):
    # Drawbar harmonics and a high sine partial: a hung tube struck with a hammer.
    p.osc(1, additive, r.u(5, 35, 5)).osc(2, analog, sinePos, r.u(40, 65, 5)).pitch(2, r.pick([16, 19, 22, 24]), r.u(-15, 15, 5))
    p.osc(3, metal, r.u(0, 40, 10), r.u(30, 50, 5)).pitch(3, r.pick([0, 12]))
    p.noise(1, 0, 40)
    p.env(2, 0, r.u(15, 60, 5), 0, 40, 85).mod(env2, "noiseLevel", r.u(25, 50, 5))
    p.env(3, 0, r.u(300, 1500, 100), 0, 800, 70).mod(env3, "osc3Volume", -r.u(15, 35, 5)).mod(env3, "osc1Position", r.u(10, 30, 5))
    p.filter(1, r.pick([ladder12, svfLow]), r.log(3000, 10000), 5)
    p.amp(0, r.u(2500, 6000, 100), 0, r.u(2000, 4000, 500), r.u(55, 75, 5))
    hall(p, r, r.u(30, 50, 5))
    p.level(-9)
    knobs(p, r, ("Hammer", "noiseLevel", 30), k_cut(20), k_space(35), ("Partial", "osc2Volume", -45), k_echo(30), k_grit(55))


@kind("Bell", 8, "Glass Crystal Prism Icicle Vial Ampoule Lens")
def bell_glass(p, r):
    # The FM table high and short over a sine: glass, a music box, a glockenspiel in a cold room.
    p.osc(1, fm, r.u(35, 95, 5)).osc(2, analog, sinePos, r.u(55, 80, 5)).pitch(2, r.pick([12, 0, 24]))
    p.env(2, 0, r.u(60, 400, 20), 0, 200, r.u(75, 90, 5)).mod(env2, "osc1Position", r.u(15, 40, 5)).mod(env2, "osc1Volume", r.u(10, 30, 5))
    p.filter(1, r.pick([svfHigh, ladder12, svfLow]), r.log(200, 9000), 5)
    p.amp(0, r.u(500, 2200, 100), 0, r.u(500, 1800, 100), r.u(60, 80, 5))
    echo(p, r)
    hall(p, r, r.u(25, 45, 5))
    p.level(-9)
    knobs(p, r, ("Glass", "osc1Position", 30), k_cut(22), k_space(35), k_echo(30), k_wide(45), ("Body", "osc2Volume", -45))


@kind("Bell", 7, "Gong Disc Shield Cauldron Vessel Bowl Urn")
def bell_gong(p, r):
    # Metal partials in slow unison, low and long, opening after the strike: a gong.
    p.osc(1, metal, r.u(20, 80, 5)).unison(1, r.pick([2, 3]), r.u(4, 12), 100, 0, r.u(20, 50, 10))
    p.osc(2, r.pick([metal, additive, fm]), r.u(10, 60, 10), r.u(45, 65, 5)).pitch(2, r.pick([-12, -5, 7, -7]), r.u(0, 12))
    p.env(2, r.u(100, 900, 100), r.u(1500, 4000, 100), 20, 2000, 40).mod(env2, "filter1Cutoff", r.u(10, 25, 5)).mod(env2, "osc1Position", r.u(10, 25, 5))
    p.filter(1, r.pick([ladder12, svfLow, ladder24]), r.log(600, 2500), r.u(5, 25, 5))
    p.amp(r.pick([0, 2, 10]), r.u(4000, 8000, 500), 0, r.u(3000, 5000, 500), r.u(45, 65, 5))
    hall(p, r, r.u(35, 55, 5))
    p.level(-9)
    knobs(p, r, k_cut(22), ("Shimmer", "osc1Position", 30), k_space(35), k_detune(1, 25), k_grit(55), k_echo(30))


# =============================================================================================
# Sequence: patterns in time with the song, from one held note
# =============================================================================================

def seq_voice(p, r, low=False):
    """A sustained tone for a pattern to cut up."""
    t = r.pick([analog, analog, pulse, sync, growl, metal, vowel, fm])
    pos = r.pick([sawPos, squarePos, 80]) if t == analog else r.u(15, 70, 5)
    p.osc(1, t, pos).unison(1, r.pick([1, 2, 3, 4]), r.u(8, 18, 2), r.u(50, 90, 10))
    if r.chance(0.5): p.osc(2, *wave(r, [analog, pulse], [sawPos, squarePos]), r.u(45, 65, 5)).pitch(2, r.pick([-12, 12, 7, 0]), r.u(3, 9))
    if low: p.subOsc(r.u(30, 50, 5))


@kind("Sequence", 10, "Gate Shutter Turnstile Sluice Louvre Baffle Damper Portcullis")
def seq_gate(p, r):
    # A held chord cut into sixteenths by a step pattern, its edges smoothed so it never clicks.
    seq_voice(p, r)
    p.filter(1, r.pick(LOWPASS), r.log(900, 4000), r.u(15, 45, 5), r.u(0, 30, 10))
    p.lfoSync(1, steps, r.pick([bar1, bar1, bars2, half]), 1, r.u(86, 95, 3)).stepPattern(1, gate(r.pick(GATES), r.pick([-25, -35, -50])))
    p.mod(lfo1, "ampLevel", r.u(70, 85, 5))
    p.lfo(2, r.pick([sine, triangle]), r.log(0.05, 0.4), 0).mod(lfo2, "filter1Cutoff", r.u(6, 16, 2))
    p.amp(2, 300, 100, r.u(150, 400, 50))
    if r.chance(0.5): dirt(p, r, (tube, hard), 15, 35)
    echo(p, r, r.u(10, 22, 2), (sixteenthD, eighthD, eighth))
    if r.chance(0.6): hall(p, r, r.u(12, 28, 4))
    p.level(-9)
    knobs(p, r, k_cut(28), ("Gate", "ampLevel", 14), k_space(35), k_echo(30), k_grit(55), k_res(30))


@kind("Sequence", 10, "Cipher Program Routine Register Ledger Index Tally Script")
def seq_filter_steps(p, r):
    # A resonant ladder moved by a step pattern: an acid line's filter, played by the patch.
    p.osc(1, analog, r.pick([sawPos, squarePos, 85])).subOsc(r.pick([1, 25, 40]))
    if r.chance(0.4): p.osc(2, analog, r.pick([sawPos, squarePos]), r.u(45, 60, 5)).pitch(2, r.pick([-12, 12, 7]), r.u(3, 8))
    p.filter(1, r.pick([ladder24, ladder24, ladder12]), r.log(250, 900), r.u(50, 80, 5), r.u(10, 50, 10))
    pattern = [r.pick([-100, -60, -30, 0, 30, 60, 100]) for _ in range(8)]
    pattern = pattern + (pattern if r.chance(0.5) else [r.pick([-100, -60, -30, 0, 30, 60, 100]) for _ in range(8)])
    p.lfoSync(1, steps, r.pick([bar1, bar1, bars2, half]), 1, r.u(40, 90, 10)).stepPattern(1, pattern)
    p.mod(lfo1, "filter1Cutoff", r.u(12, 24, 2))
    p.amp(2, 300, 100, r.u(80, 200, 20)).mono(0, True)
    if r.chance(0.6): dirt(p, r, (tube, hard, fuzz), 20, 50)
    if r.chance(0.6): echo(p, r, r.u(10, 22, 2), (sixteenthD, eighthD, eighth))
    p.level(-9)
    knobs(p, r, k_cut(25), k_res(18), ("Depth", "filter1Drive", 55), k_grit(50, hard), k_echo(30), k_sub(40))


@kind("Sequence", 10, "Piston Ratchet Escapement Treadle Flywheel Crank Camshaft Tappet")
def seq_plucked(p, r):
    # Sixteenth notes from one key: a second pattern shapes each as a short note, the first accents the filter.
    seq_voice(p, r, low=True)
    p.filter(1, r.pick([ladder24, ladder24, ladder12, svfLow]), r.log(300, 1400), r.u(20, 55, 5), r.u(0, 40, 10))
    shape = r.pick([[0, -25, -55, -100], [0, -40, -80, -100], [0, -15, -40, -70], [0, 0, -60, -100], [0, -50, -100, -100]])
    accents = r.pick([[0, 0, 0, 0], [0, -15, 0, -15], [0, -10, -20, -10], [0, -100, 0, 0], [0, 0, -100, 0], [0, -20, 0, -100]])
    pattern = [max(-100, s + a) for a in accents for s in shape]
    fast = r.chance(0.75)
    p.lfoSync(2, steps, quarter if fast else half, 1, r.u(96, 99)).stepPattern(2, pattern)
    p.mod(lfo2, "ampLevel", r.u(72, 85)).mod(lfo2, "filter1Cutoff", r.u(8, 20, 2))
    accent = [r.pick([0, 0, 40, 70, 100, -40]) for _ in range(16)]
    p.lfoSync(1, steps, bar1 if fast else bars2, 1, 50).stepPattern(1, accent).mod(lfo1, "filter1Cutoff", r.u(6, 16, 2))
    p.amp(2, 300, 100, r.u(80, 200, 20))
    if r.chance(0.6): dirt(p, r, (tube, hard), 15, 45)
    if r.chance(0.5): echo(p, r, r.u(8, 18, 2), (sixteenthD, eighthD))
    p.level(-9)
    knobs(p, r, k_cut(25), k_res(25), ("Accent", "filter1Drive", 55), k_grit(50, hard), k_echo(30), k_space(30, 50, 1.4))


@kind("Sequence", 10, "Scanner Reader Sweep Sampler Sorter Sifter Probe Tracer")
def seq_wave_steps(p, r):
    # A step pattern moves the wavetable's position: a new wave on every sixteenth.
    t = r.pick([sync, fm, vowel, metal, growl, additive, pulse])
    p.osc(1, t, 50).unison(1, r.pick([1, 2, 3]), r.u(8, 16, 2), r.u(50, 90, 10))
    if r.chance(0.5): p.subOsc(r.u(25, 45, 5))
    pattern = [r.pick([-100, -70, -40, -10, 20, 50, 80, 100]) for _ in range(16)]
    p.lfoSync(1, steps, r.pick([bar1, bar1, bars2, half]), 1, r.u(0, 60, 20)).stepPattern(1, pattern)
    p.mod(lfo1, "osc1Position", r.u(30, 48, 3))
    p.lfoSync(2, steps, r.pick([bar1, bars2]), 1, r.u(88, 95)).stepPattern(2, gate(r.pick(GATES), -30)).mod(lfo2, "ampLevel", r.u(40, 80, 10))
    p.filter(1, r.pick(LOWPASS + [svfBand]), r.log(900, 5000), r.u(15, 45, 5))
    p.amp(2, 300, 100, r.u(120, 300, 20))
    if r.chance(0.5): dirt(p, r, (tube, hard, crush), 15, 40)
    echo(p, r, r.u(10, 22, 2), (sixteenthD, eighthD, eighth))
    p.level(-9)
    knobs(p, r, k_cut(25), ("Range", "osc1Position", 30), k_space(35), k_echo(30), k_grit(55), k_res(30))


@kind("Sequence", 9, "Pump Bellows Lung Billow Heave Swell Surge")
def seq_pump(p, r):
    # The level ducks on every beat and swells back: a pad or bass pumping as if a kick pushed it down.
    seq_voice(p, r, low=r.chance(0.5))
    p.filter(1, r.pick(LOWPASS), r.log(600, 3500), r.u(10, 40, 5), r.u(0, 30, 10))
    duck = r.pick([[-100, -80, -55, -35, -20, -10, -4, 0], [-100, -60, -30, -10, 0, 0, 0, 0], [-100, -90, -75, -60, -45, -30, -15, 0]])
    p.lfoSync(1, steps, r.pick([quarter, quarter, half]), 1, r.u(95, 99)).stepPattern(1, duck + [0] * 8 if r.chance(0.5) else [d for d in duck for _ in (0, 1)])
    p.mod(lfo1, "ampLevel", r.u(45, 80, 5)).mod(lfo1, "filter1Cutoff", r.u(4, 14, 2))
    p.amp(r.pick([5, 50, 200]), 300, 100, r.u(200, 800, 100))
    if r.chance(0.5): p.chorus(r.u(20, 40, 5), r.u(0.2, 0.8, 0.1), 50)
    hall(p, r, r.u(15, 35, 5))
    p.level(-9)
    knobs(p, r, k_cut(28), ("Duck", "ampLevel", 14), k_space(35), k_grit(55), k_res(30), k_echo(30))


@kind("Sequence", 9, "Lattice Mesh Weave Braid Tangle Knot Plait Web")
def seq_cross_rhythm(p, r):
    # A gate in sixteenths against a filter sweep three beats or a dotted quarter long: two cycles that drift apart and meet.
    seq_voice(p, r)
    p.filter(1, r.pick([ladder24, ladder12, svfLow, svfBand]), r.log(500, 2400), r.u(25, 60, 5), r.u(0, 30, 10))
    p.lfoSync(1, steps, r.pick([bar1, half]), 1, r.u(86, 95, 3)).stepPattern(1, gate(r.pick(GATES), -30)).mod(lfo1, "ampLevel", r.u(65, 85, 5))
    p.lfoSync(2, r.pick([triangle, sine, sawDown, sawUp]), r.pick([quarterD, halfT, eighthD, quarterT]), 0).mod(lfo2, "filter1Cutoff", r.u(10, 22, 2))
    p.amp(2, 300, 100, r.u(120, 300, 20))
    if r.chance(0.5): dirt(p, r, (tube, hard), 15, 40)
    echo(p, r, r.u(10, 22, 2), (sixteenthD, eighthD, quarterT))
    p.level(-9)
    knobs(p, r, k_cut(25), ("Gate", "ampLevel", 14), k_res(25), k_echo(30), k_grit(55), k_space(35))


@kind("Sequence", 10, "March Drill Patrol Column Convoy Parade Advance Charge")
def seq_octave_bass(p, r):
    # The EBM octave bass from one key: a step pattern jumps the pitch an octave, another makes the notes.
    p.osc(1, *wave(r, [analog, analog, pulse, sync], [sawPos, squarePos]))
    if r.chance(0.5): p.osc(2, analog, r.pick([sawPos, squarePos]), r.u(50, 70, 5)).pitch(2, 0, r.u(5, 12))
    p.subOsc(r.u(25, 45, 5))
    notes = r.pick([[0, 0, 12, 0, 0, 12, 0, 12], [0, 12, 0, 12, 0, 12, 0, 12], [0, 0, 0, 12, 0, 0, 12, 12], [0, 12, 12, 0, 12, 0, 12, 12],
                    [0, 0, 12, 0, 12, 0, 0, 12], [0, 12, 0, 0, 12, 0, 12, 0], [0, 0, 12, 12, 0, 0, 12, 0], [0, 0, 0, 0, 12, 0, 0, 12]])
    notes = notes + (notes if r.chance(0.6) else r.shuffle(notes))
    p.lfoSync(1, steps, bar1, 1, 0).stepPattern(1, step_notes(notes)).mod(lfo1, "osc1Pitch", 50)
    if p.has("osc2On"): p.mod(lfo1, "osc2Pitch", 50)
    shape = r.pick([[0, -30, -65, -100], [0, -20, -50, -85], [0, 0, -50, -100], [0, -45, -90, -100]])
    p.lfoSync(2, steps, quarter, 1, r.u(96, 99)).stepPattern(2, shape * 4).mod(lfo2, "ampLevel", r.u(72, 85)).mod(lfo2, "filter1Cutoff", r.u(8, 22, 2))
    p.filter(1, r.pick([ladder24, ladder24, ladder12]), r.log(300, 1200), r.u(20, 50, 5), r.u(10, 50, 10))
    p.amp(2, 300, 100, r.u(60, 150, 10)).mono(0, True)
    if r.chance(0.6): dirt(p, r, (tube, hard, fuzz), 15, 45)
    p.level(-9)
    knobs(p, r, k_cut(25), k_res(25), k_grit(50, hard), k_sub(40))


# =============================================================================================
# Riser
# =============================================================================================

def riser_amp(p, r):
    # The level is nearly there within two seconds; what rises after that is the sound itself.
    p.amp(r.u(800, 1800, 100), 1000, 100, r.u(400, 900, 100), r.u(-30, 30, 10))
    p.target = -14


@kind("Riser", 7, "Ascent Climb Ramp Incline Escalation Lift Hoist")
def riser_sync(p, r):
    # A synced saw whose sync ratio climbs for seconds: the pitch seems to rise without a step in it.
    p.osc(1, analog, sawPos).warp(1, wSync, r.u(0, 10, 5)).unison(1, r.pick([2, 3, 4]), r.u(8, 20, 2), r.u(60, 100, 10))
    if r.chance(0.5): p.noise(r.u(15, 35, 5), r.pick([0, 1]))
    length = r.u(3500, 8000, 500)
    p.env(2, length, 1000, 100, 500, r.u(-60, 0, 20)).mod(env2, "osc1WarpAmount", r.u(55, 90, 5)).mod(env2, "filter1Cutoff", r.u(20, 40, 5))
    p.filter(1, r.pick([svfHigh, ladderHigh, svfBand, ladder24]), r.log(200, 900), r.u(15, 45, 5))
    riser_amp(p, r)
    if r.chance(0.5): echo(p, r, r.u(12, 24, 2))
    hall(p, r, r.u(30, 50, 5))
    p.level(-9)
    knobs(p, r, ("Sync", "osc1WarpAmount", 30), k_cut(20), k_space(35), k_grit(55, hard), k_detune(1, 25), k_echo(30))


@kind("Riser", 7, "Flutter Stutter Judder Shiver Rattle Strobe")
def riser_tremolo(p, r):
    # A tone whose tremolo gets faster and faster as it brightens: tension by acceleration.
    seq_voice(p, r)
    length = r.u(3500, 8000, 500)
    p.lfo(1, r.pick([sine, triangle]), r.log(1.5, 4), 1).mod(lfo1, "ampLevel", r.u(12, 26, 2))
    p.env(2, length, 1000, 100, 500, r.u(-40, 20, 10)).mod(env2, "lfo1Rate", r.u(25, 45, 5)).mod(env2, "filter1Cutoff", r.u(20, 40, 5))
    p.filter(1, r.pick([ladder24, svfLow, svfBand]), r.log(250, 900), r.u(15, 45, 5))
    riser_amp(p, r)
    if r.chance(0.5): dirt(p, r, (tube, hard), 15, 40)
    hall(p, r, r.u(25, 45, 5))
    p.level(-9)
    knobs(p, r, ("Depth", "ampLevel", 12), k_cut(20), k_space(35), k_grit(55, hard), k_res(30), k_echo(30))


@kind("Riser", 7, "Jet Turbine Afterburner Thruster Exhaust Backdraft Blast")
def riser_comb(p, r):
    # Noise and a saw through a comb whose pitch climbs: a jet coming up to speed.
    p.osc(1, analog, sawPos, r.u(40, 65, 5)).unison(1, r.pick([2, 3, 4]), r.u(10, 24, 2), 100).noise(r.u(50, 80, 5), r.pick([0, 1]))
    length = r.u(3500, 8000, 500)
    p.filter(1, comb, r.log(60, 200), r.u(70, 92, 2))
    p.filter(2, r.pick([svfHigh, ladder24, svfBand]), r.log(200, 1500), r.u(10, 35, 5))
    p.env(2, length, 1000, 100, 500, r.u(-50, 20, 10)).mod(env2, "filter1Cutoff", r.u(25, 50, 5)).mod(env2, "filter2Cutoff", r.u(10, 30, 5))
    riser_amp(p, r)
    hall(p, r, r.u(25, 45, 5))
    p.level(-9)
    knobs(p, r, ("Ring", "filter1Resonance", 7), ("Tone", "filter2Cutoff", 20), k_space(35), k_grit(55, hard), k_noise(-40, "Air"), k_echo(30))


@kind("Riser", 7, "Swarm Horde Throng Multitude Legion Host Plague")
def riser_swarm(p, r):
    # Eight voices that start together and pull apart as the filter opens: a note coming to pieces.
    p.osc(1, r.pick([analog, analog, pulse, sync]), r.pick([sawPos, 30, 50])).unison(1, 8, r.u(0, 4), 100, 0, r.u(0, 40, 20)).pitch(1, 0, -100)
    p.osc(2, analog, sawPos, r.u(45, 65, 5)).pitch(2, r.pick([12, -12, 7])).unison(2, r.pick([3, 4]), r.u(5, 10), 100)
    length = r.u(3500, 8000, 500)
    p.env(2, length, 1000, 100, 500, r.u(-50, 20, 10)).mod(env2, "osc1UnisonDetune", r.u(45, 80, 5)).mod(env2, "osc1Fine", r.u(50, 100, 10))
    p.mod(env2, "filter1Cutoff", r.u(20, 40, 5))
    p.filter(1, r.pick([ladder24, svfLow, svfBand]), r.log(300, 1200), r.u(10, 40, 5))
    riser_amp(p, r)
    hall(p, r, r.u(30, 50, 5))
    p.level(-9)
    knobs(p, r, ("Scatter", "osc1UnisonDetune", 20), k_cut(20), k_space(35), k_grit(55, hard), k_echo(30))


@kind("Riser", 7, "Descent Plunge Dive Collapse Slump Sinking Drain")
def riser_fall(p, r):
    # The other way: a resonating filter and its noise falling for seconds after the hit.
    p.osc(1, r.pick([analog, sync, pulse]), r.pick([sawPos, 40, 30]), r.u(40, 70, 5)).unison(1, r.pick([2, 3, 4]), r.u(10, 24, 2), 100)
    p.noise(r.u(35, 60, 5), r.pick([0, 1]))
    length = r.u(3000, 7000, 500)
    p.filter(1, r.pick([ladder24, svfBand, ladderBand]), r.log(120, 400), r.u(55, 92), r.u(0, 30, 10))
    p.env(2, 0, length, 0, length, r.u(20, 70, 10)).mod(env2, "filter1Cutoff", r.u(35, 60, 5))
    p.amp(0, length, 0, r.u(1500, 3000, 500), r.u(20, 60, 10))
    if r.chance(0.6): echo(p, r, r.u(12, 24, 2))
    hall(p, r, r.u(30, 50, 5))
    p.level(-9)
    knobs(p, r, k_res(8), ("Height", "filter1Cutoff", 15), k_space(35), k_grit(55, hard), k_noise(-35, "Air"))


# =============================================================================================
# Impact
# =============================================================================================

@kind("Impact", 8, "Drop Plummet Freefall Sinkhole Subsidence Cave-In Landslide")
def impact_sub_drop(p, r):
    # A sine falling in pitch under a burst of noise: the floor going out.
    p.osc(1, r.pick([analog, sub]), r.pick([sinePos, 10, 20])).subOsc(r.u(30, 60, 10))
    p.noise(1, r.pick([0, 1, 3]), r.u(-40, 20, 20))
    p.env(2, 0, r.u(200, 900, 50), 0, 400, r.u(60, 85, 5)).mod(env2, "osc1Pitch", semis(r.pick([12, 19, 24, 31, 36])))
    p.env(3, 0, r.u(40, 300, 20), 0, 200, 80).mod(env3, "noiseLevel", r.u(40, 75, 5))
    p.filter(1, ladder24, r.log(500, 3000), r.u(5, 25, 5), r.u(0, 50, 10))
    p.amp(0, r.u(1200, 3500, 100), 0, r.u(1500, 3000, 500), r.u(45, 70, 5))
    if r.chance(0.6): dirt(p, r, (tube, hard, fuzz), 20, 45, (1500, 6000))
    hall(p, r, r.u(30, 50, 5))
    p.level(-9)
    knobs(p, r, ("Fall", "osc1Pitch", semis(12)), k_cut(22), k_space(35), k_grit(50, hard, 4000), ("Debris", "noiseLevel", 30), k_sub(35))


@kind("Impact", 8, "Slam Clang Bulkhead Hatch Gate Shutter Lid Trapdoor")
def impact_slam(p, r):
    # Metal and noise through a low comb into a big room: a steel door shut hard.
    p.osc(1, metal, r.u(30, 95, 5)).noise(r.u(40, 70, 5), r.pick([0, 2, 3, 1]))
    if r.chance(0.5): p.osc(2, r.pick([analog, sub]), sinePos, r.u(55, 75, 5)).pitch(2, r.pick([-12, 0]))
    p.filter(1, comb, r.log(50, 300), r.u(55, 85, 5))
    p.filter(2, r.pick([ladder24, svfLow, svfBand]), r.log(500, 4000), r.u(10, 30, 5))
    p.env(2, 0, r.u(80, 400, 20), 0, 300, 85).mod(env2, "filter2Cutoff", r.u(10, 25, 5)).mod(env2, "osc1Position", r.u(10, 30, 5))
    p.amp(0, r.u(500, 2000, 100), 0, r.u(800, 2500, 100), r.u(55, 75, 5))
    if r.chance(0.6): dirt(p, r, (hard, fuzz, tube), 20, 50)
    hall(p, r, r.u(35, 55, 5))
    p.level(-9)
    knobs(p, r, ("Ring", "filter1Resonance", 25), ("Tone", "filter2Cutoff", 22), k_space(35), k_grit(45, hard), ("Clang", "osc1Position", 30))


@kind("Impact", 8, "Thunder Rumble Tremor Aftershock Shockwave Avalanche Rockfall")
def impact_thunder(p, r):
    # Dark noise that rolls on after the hit, its filter thrown about at random.
    p.set("osc1On", 0).noise(r.u(70, 95, 5), r.pick([1, 1, 0, 2]), r.u(-80, -20, 20)).subOsc(r.pick([1, 30, 45]))
    p.filter(1, r.pick([ladder24, svfLow, ladder12]), r.log(200, 900), r.u(10, 40, 5), r.u(20, 60, 10))
    p.lfo(1, r.pick([randomShape, drift]), r.log(3, 14), 1).mod(lfo1, "filter1Cutoff", r.u(6, 18, 2))
    p.env(2, 0, r.u(300, 1500, 100), 0, 500, 70).mod(env2, "filter1Cutoff", r.u(10, 30, 5))
    p.amp(r.pick([0, 5, 30]), r.u(2500, 6000, 500), 0, r.u(2000, 4000, 500), r.u(40, 65, 5))
    if r.chance(0.5): dirt(p, r, (tube, hard, fuzz), 20, 45, (1200, 5000))
    hall(p, r, r.u(30, 50, 5))
    p.level(-9)
    knobs(p, r, k_cut(25), ("Roll", "lfo1Rate", 25), k_space(35), k_grit(50, hard, 3000), k_sub(40), k_res(30))


@kind("Impact", 8, "Shatter Smash Fracture Breakage Burst Splinter Wreckage")
def impact_glass(p, r):
    # Bright metal partials and digital noise, short, with echoes: something breaking.
    p.osc(1, metal, r.u(60, 100, 5)).pitch(1, r.pick([12, 19, 24, 7])).unison(1, r.pick([2, 3, 4]), r.u(15, 40, 5), 100)
    p.noise(r.u(40, 70, 5), r.pick([3, 2, 0]), r.u(0, 80, 20))
    p.filter(1, r.pick([svfHigh, ladderHigh, svfBand]), r.log(1500, 6000), r.u(10, 40, 5))
    p.env(2, 0, r.u(60, 300, 20), 0, 200, 85).mod(env2, "osc1Position", r.u(10, 30, 5)).mod(env2, "noiseLevel", r.u(10, 30, 5))
    p.amp(0, r.u(250, 1200, 50), 0, r.u(400, 1200, 100), r.u(60, 80, 5))
    p.delay(r.pick([sixteenthT, thirtySecond, sixteenth, eighthT]), r.u(40, 65, 5), r.u(18, 35), r.log(4000, 12000))
    hall(p, r, r.u(25, 45, 5))
    p.level(-9)
    knobs(p, r, ("Shards", "fxDelayMix", 35), k_cut(20), k_space(35), k_grit(55, crush), ("Glass", "osc1Position", 30), k_detune(1, 25))


@kind("Impact", 8, "Verdict Sentence Decree Edict Judgment Reckoning Ultimatum")
def impact_tonal(p, r):
    # Low saws in octaves and a fifth, shut fast and thrown into a long hall: the hit under a title card.
    p.osc(1, analog, sawPos).unison(1, r.pick([3, 4, 6]), r.u(12, 26, 2), r.u(70, 100, 10))
    p.osc(2, r.pick([analog, growl, sync]), r.u(30, 70, 10)).pitch(2, r.pick([-12, 12, 7]), r.u(4, 10))
    p.subOsc(r.u(40, 60, 5)).noise(1, 1)
    p.filter(1, ladder24, r.log(200, 700), r.u(10, 35, 5), r.u(20, 60, 10))
    p.env(2, 0, r.u(150, 700, 50), 0, 400, r.u(65, 85, 5)).mod(env2, "filter1Cutoff", r.u(25, 45, 5)).mod(env2, "noiseLevel", r.u(20, 45, 5))
    p.amp(0, r.u(1000, 3000, 100), 0, r.u(1500, 3000, 500), r.u(50, 70, 5))
    dirt(p, r, (hard, tube, fuzz), 20, 50, (2500, 7000))
    hall(p, r, r.u(35, 55, 5))
    p.level(-9)
    knobs(p, r, k_cut(25), k_detune(1, 25), k_space(35), k_grit(40, hard), k_sub(35))


# =============================================================================================
# Perc: metal and found percussion
# =============================================================================================

@kind("Perc", 7, "Anvil Hammer Sledge Maul Mallet Rivet Forge")
def perc_anvil(p, r):
    # The metal table struck: clang with a hard front.
    p.osc(1, metal, r.u(40, 100, 5)).pitch(1, r.pick([0, 7, 12, 12, 19, -5]))
    if r.chance(0.5): p.osc(2, metal, r.u(20, 80, 10), r.u(45, 65, 5)).pitch(2, r.pick([5, 11, 14, 18, -7]), r.u(0, 20, 5))
    p.noise(1, r.pick([0, 3]))
    p.env(2, 0, r.u(6, 30, 2), 0, 20, 90).mod(env2, "noiseLevel", r.u(30, 60, 5))
    p.env(3, 0, r.u(60, 300, 20), 0, 200, 85).mod(env3, "osc1Position", r.pick([-1, 1]) * r.u(10, 30, 5))
    p.filter(1, r.pick([svfBand, svfHigh, ladderBand, ladderHigh]), r.log(600, 4000), r.u(15, 50, 5), r.u(0, 40, 10))
    p.amp(0, r.u(120, 700, 20), 0, r.u(100, 500, 20), r.u(60, 85, 5))
    if r.chance(0.6): dirt(p, r, (hard, tube, fuzz), 15, 50)
    room(p, r)
    p.level(-9)
    knobs(p, r, ("Clang", "osc1Position", 30), k_cut(22), k_space(40, 55, 1.6), k_grit(50, hard), ("Strike", "noiseLevel", 35), k_res(30))


@kind("Perc", 7, "Pipe Conduit Tube Barrel Drum Canister Flask Cylinder")
def perc_pipe(p, r):
    # A click into a comb tuned to the key or a fifth or an octave off it: a pipe hit with a spanner.
    strike(p, r, (6, 25))
    p.filter(1, comb, r.pick([261.6, 523.3, 392, 196]), r.u(93, 100), 0, 100)
    p.filter(2, r.pick([svfHigh, svfBand, ladder12]), r.log(300, 3000), r.u(10, 35, 5))
    p.amp(0, r.u(250, 1200, 50), 0, r.u(150, 600, 50), r.u(55, 75, 5))
    if r.chance(0.5): dirt(p, r, (hard, tube), 15, 45)
    room(p, r)
    p.level(-9)
    knobs(p, r, ("Damp", "filter1Resonance", -15), ("Tone", "filter2Cutoff", 22), k_space(40, 55, 1.6), k_grit(55, hard), ("Tune", "filter1Cutoff", 12))


@kind("Perc", 7, "Clank Clunk Clatter Chink Clash Jangle Rattle")
def perc_fm_clank(p, r):
    # FM at an awkward ratio for a moment: scrap dropped on concrete.
    p.osc(1, analog, r.pick([sinePos, trianglePos, squarePos]), 1).pitch(1, r.pick([6, 11, 14, 17, 22, 23]), r.u(-40, 40, 10))
    p.osc(2, analog, r.pick([sinePos, trianglePos])).warp(2, wFm, r.u(20, 60, 5)).pitch(2, r.pick([0, 0, 12, 7]))
    p.env(2, 0, r.u(40, 250, 10), 0, 150, r.u(75, 90, 5)).mod(env2, "osc2WarpAmount", r.u(20, 40, 5))
    p.filter(1, r.pick([svfBand, svfHigh, ladderBand, ladder12]), r.log(500, 5000), r.u(10, 45, 5))
    p.amp(0, r.u(80, 500, 20), 0, r.u(80, 400, 20), r.u(60, 85, 5))
    if r.chance(0.6): dirt(p, r, (hard, crush, fuzz), 15, 45)
    room(p, r)
    p.level(-9)
    knobs(p, r, ("Index", "osc2WarpAmount", 35), k_cut(22), k_space(40, 55, 1.6), k_grit(50, hard), k_echo(30, sixteenth))


@kind("Perc", 7, "Bell Cowbell Gong Triangle Agogo Plate Disc")
def perc_two_tone(p, r):
    # Two squares a little under a fifth and a half apart through a band-pass: the drum machine's cowbell, and its cousins.
    p.osc(1, *wave(r, [analog, pulse], [squarePos, 90])).osc(2, *wave(r, [analog, pulse], [squarePos, 90]))
    p.pitch(1, r.pick([5, 7, 9, 12])).pitch(2, p.get("osc1Pitch") + r.pick([6, 7, 7, 8, 11]), r.u(-40, 40, 10))
    p.filter(1, r.pick([svfBand, ladderBand]), r.log(700, 3500), r.u(25, 60, 5))
    p.env(2, 0, r.u(15, 60, 5), 0, 30, 85).mod(env2, "ampLevel", r.u(8, 16, 2))
    p.amp(0, r.u(120, 600, 20), 0, r.u(100, 400, 20), r.u(55, 80, 5))
    if r.chance(0.5): dirt(p, r, (hard, tube), 15, 40)
    room(p, r)
    p.level(-9)
    knobs(p, r, k_cut(18), k_res(30), k_space(40, 55, 1.6), k_grit(55, hard), ("Interval", "osc2Fine", 40))


@kind("Perc", 7, "Ping Tick Click Tock Peck Rap Knock")
def perc_ping(p, r):
    # One or two sine partials and a click, over in a moment: rim shots, claves and blocks.
    p.osc(1, analog, r.pick([sinePos, sinePos, trianglePos])).pitch(1, r.pick([0, 5, 7, 12, 12]))
    if r.chance(0.6): p.osc(2, analog, sinePos, r.u(45, 75, 5)).pitch(2, p.get("osc1Pitch") + r.pick([5, 6, 8, 11]), r.u(-40, 40, 10))
    p.noise(1, r.pick([0, 0, 3]), r.u(0, 60, 20))
    p.env(2, 0, r.u(2, 12), 0, 8, 90).mod(env2, "noiseLevel", r.u(40, 80, 5)).mod(env2, "osc1Fine", r.u(10, 50, 10))
    p.filter(1, r.pick([svfHigh, svfBand, ladder12]), r.log(250, 2500), r.u(10, 45, 5), 0, 50)
    p.amp(0, r.u(60, 400, 20), 0, r.u(50, 250, 10), r.u(60, 85, 5))
    if r.chance(0.4): dirt(p, r, (hard, tube), 15, 40)
    room(p, r)
    p.level(-9)
    knobs(p, r, ("Tune", "osc1Pitch", semis(7)), ("Click", "noiseLevel", 35), k_space(40, 55, 1.6), k_grit(55, hard), k_cut(20))


@kind("Perc", 7, "Zap Bolt Spark Jolt Shock Discharge Flash")
def perc_zap(p, r):
    # A sine thrown down several octaves in an instant, with a snap of noise: the electronic tom and the zap.
    p.osc(1, r.pick([analog, sub, fm]), r.pick([sinePos, 10, 25, trianglePos])).pitch(1, r.pick([-12, -5, 0, 0, 7]))
    p.env(2, 0, r.u(30, 220, 10), 0, 100, r.u(70, 92, 2)).mod(env2, "osc1Pitch", semis(r.pick([12, 19, 24, 31, 36, 43])))
    if r.chance(0.6):
        p.noise(1, r.pick([0, 3]), r.u(0, 60, 20))
        p.env(3, 0, r.u(5, 40, 5), 0, 30, 90).mod(env3, "noiseLevel", r.u(30, 60, 5))
    p.filter(1, r.pick([ladder24, svfLow, svfBand]), r.log(1200, 9000), r.u(5, 35, 5), r.u(0, 40, 10))
    p.amp(0, r.u(120, 600, 20), 0, r.u(100, 400, 20), r.u(55, 80, 5))
    if r.chance(0.5): dirt(p, r, (hard, tube, crush), 15, 45)
    if r.chance(0.4): echo(p, r, r.u(10, 20, 2), (sixteenth, sixteenthD, eighth))
    room(p, r)
    p.level(-9)
    knobs(p, r, ("Sweep", "osc1Pitch", semis(12)), k_cut(20), k_space(40, 55, 1.6), k_grit(55, hard), k_echo(30, sixteenth))


@kind("Perc", 7, "Shaker Rattle Sand Grit Gravel Seed Husk Chaff")
def perc_shaker(p, r):
    # High noise with a soft front: shakers, and with crackle in it, something looser.
    p.set("osc1On", 0).noise(r.u(70, 95, 5), r.pick([0, 0, 2, 3]), r.u(0, 80, 20))
    p.filter(1, r.pick([svfHigh, ladderHigh, svfBand]), r.log(3000, 9000), r.u(10, 45, 5))
    p.env(2, r.u(5, 30, 5), r.u(40, 150, 10), 0, 60, 60).mod(env2, "filter1Cutoff", r.u(4, 14, 2))
    p.amp(r.u(4, 30, 2), r.u(50, 220, 10), 0, r.u(40, 160, 20), r.u(40, 70, 10))
    room(p, r, r.u(8, 20, 4))
    p.level(-9)
    knobs(p, r, k_cut(15), k_res(35), k_space(40, 55, 1.6), k_grit(55, crush), ("Colour", "noiseColour", -40))


@kind("Perc", 7, "Spring Coil Recoil Twang Boing Bounce Whip")
def perc_spring(p, r):
    # A click into a comb whose pitch drops as it rings: a spring let go.
    strike(p, r, (6, 30))
    p.filter(1, comb, r.log(150, 700), r.u(93, 100))
    p.env(3, 0, r.u(80, 500, 20), 0, 300, r.u(40, 85, 5)).mod(env3, "filter1Cutoff", r.pick([1, 1, -1]) * r.u(5, 18))
    p.filter(2, r.pick([svfHigh, svfBand]), r.log(200, 1500), r.u(10, 30, 5))
    p.amp(0, r.u(300, 1200, 50), 0, r.u(200, 600, 50), r.u(55, 75, 5))
    room(p, r)
    p.level(-9)
    knobs(p, r, ("Damp", "filter1Resonance", -15), ("Tune", "filter1Cutoff", 12), k_space(40, 55, 1.6), k_grit(55, hard), ("Tone", "filter2Cutoff", 22))


@kind("Perc", 7, "Tin Can Lid Drum Pail Bucket Pan Tray")
def perc_tin(p, r):
    # A wave cut into a few coarse steps and cut short: a tin can, a trash lid.
    p.osc(1, r.pick([analog, analog, metal]), r.pick([sawPos, trianglePos, 50])).pitch(1, r.pick([0, 5, 7, 12, -5])).warp(1, wCrush, r.u(45, 75, 5))
    p.noise(r.u(20, 50, 5), r.pick([0, 3, 2]))
    p.env(2, 0, r.u(30, 120, 10), 0, 60, 88).mod(env2, "osc1Pitch", semis(r.pick([3, 5, 7, 12]))).mod(env2, "filter1Cutoff", r.u(5, 15))
    p.filter(1, r.pick([svfBand, ladderBand, svfHigh]), r.log(700, 4000), r.u(25, 60, 5))
    p.amp(0, r.u(60, 300, 10), 0, r.u(60, 250, 10), r.u(60, 85, 5))
    if r.chance(0.5): dirt(p, r, (hard, crush), 15, 45)
    room(p, r)
    p.level(-9)
    knobs(p, r, k_cut(18), k_res(30), k_space(40, 55, 1.6), k_grit(55, hard), ("Steps", "osc1WarpAmount", 30))


@kind("Perc", 7, "Chain Shackle Manacle Fetter Link Tether Leash")
def perc_chain(p, r):
    # Crackle through a comb and a band-pass: links dragged over metal.
    p.set("osc1On", 0).noise(r.u(70, 95, 5), 2, r.u(0, 60, 20))
    p.filter(1, comb, r.log(200, 1500), r.u(60, 90, 5))
    p.filter(2, r.pick([svfBand, svfHigh]), r.log(1200, 5000), r.u(15, 50, 5))
    p.lfo(1, randomShape, r.log(8, 30), 1).mod(lfo1, "filter1Cutoff", r.u(2, 8))
    p.amp(r.pick([0, 5, 15]), r.u(150, 700, 50), 0, r.u(150, 500, 50), r.u(40, 70, 10))
    if r.chance(0.5): dirt(p, r, (hard, tube), 15, 40)
    room(p, r)
    p.level(-9)
    knobs(p, r, ("Ring", "filter1Resonance", 25), ("Tone", "filter2Cutoff", 20), k_space(40, 55, 1.6), k_grit(55, hard), ("Rattle", "lfo1Rate", 25))


# =============================================================================================
# Texture
# =============================================================================================

def bed_amp(p, r):
    p.amp(r.u(200, 1500, 100), 1000, 100, r.u(800, 2500, 100))


@kind("Texture", 8, "Embers Cinders Kindling Tinder Ash Soot Smoulder")
def tex_crackle(p, r):
    # Crackle, filtered and never quite even: a fire, a worn record, a bad cable.
    p.set("osc1On", 0).noise(r.u(75, 100, 5), 2, r.u(-60, 60, 20))
    p.filter(1, r.pick([svfBand, svfHigh, ladderBand, svfLow]), r.log(500, 6000), r.u(10, 50, 5))
    p.lfo(1, r.pick([randomShape, drift]), r.log(0.3, 9), 0).mod(lfo1, "filter1Cutoff", r.u(5, 18)).mod(lfo1, "ampLevel", r.u(4, 12, 2))
    bed_amp(p, r)
    if r.chance(0.5): dirt(p, r, (tube, crush, hard), 15, 40)
    hall(p, r, r.u(10, 35, 5))
    p.level(-9)
    knobs(p, r, k_cut(20), ("Colour", "noiseColour", -40), k_space(35), k_grit(55, crush), k_motion(1, 25), k_res(35))


@kind("Texture", 8, "Wind Gale Draught Squall Gust Breeze Flurry")
def tex_wind(p, r):
    # Dark noise through a resonant filter that wanders: wind round a building.
    p.set("osc1On", 0).noise(r.u(70, 95, 5), r.pick([1, 1, 0]), r.u(-60, 0, 20))
    p.filter(1, r.pick([svfBand, ladderBand, ladder24]), r.log(300, 1800), r.u(55, 88, 3))
    p.lfo(1, drift, r.log(0.08, 0.7), 0).mod(lfo1, "filter1Cutoff", r.u(8, 20, 2))
    p.lfo(2, r.pick([drift, sine]), r.log(0.05, 0.4), 0).mod(lfo2, "ampLevel", r.u(6, 16, 2)).mod(lfo2, "filter1Resonance", r.u(5, 12))
    bed_amp(p, r)
    hall(p, r, r.u(20, 45, 5))
    p.level(-9)
    knobs(p, r, k_cut(20), ("Howl", "filter1Resonance", 10), k_space(35), ("Gusts", "lfo1Rate", 25), k_grit(55))


@kind("Texture", 8, "Plant Works Mill Foundry Refinery Depot Yard Shed")
def tex_machine_room(p, r):
    # A low pulse and noise through a comb, with a pattern knocking the filter about: a room full of plant.
    p.osc(1, pulse, r.u(50, 95, 5), r.u(50, 70, 5)).pitch(1, r.pick([-12, -24, -12, 0])).noise(r.u(40, 70, 5), r.pick([0, 1, 2, 3]))
    p.filter(1, comb, r.log(60, 400), r.u(55, 85, 5))
    p.filter(2, r.pick([ladder24, svfBand, svfLow]), r.log(500, 3500), r.u(15, 45, 5))
    pattern = [r.pick([-100, -50, 0, 50, 100]) for _ in range(16)]
    p.lfoSync(1, steps, r.pick([bar1, bars2, half]), 0, r.u(30, 80, 10)).stepPattern(1, pattern).mod(lfo1, "filter2Cutoff", r.u(8, 18, 2))
    p.lfo(2, drift, r.log(0.05, 0.4), 0).mod(lfo2, "filter1Cutoff", r.u(2, 8))
    bed_amp(p, r)
    if r.chance(0.6): dirt(p, r, (tube, hard, crush), 15, 40)
    hall(p, r, r.u(15, 35, 5))
    p.level(-9)
    knobs(p, r, ("Cutoff", "filter2Cutoff", 22), ("Ring", "filter1Resonance", 25), k_space(35), k_grit(55, hard), k_noise(-40, "Air"))


@kind("Texture", 8, "Broadcast Carrier Shortwave Longwave Static Band Channel Relay")
def tex_radio(p, r):
    # FM thrown about at random behind a narrow band, with crackle: a radio between stations.
    p.osc(1, analog, sinePos, r.pick([1, 20])).pitch(1, r.pick([7, 12, 17, 19, 23]), r.u(-30, 30, 10))
    p.osc(2, analog, r.pick([sinePos, trianglePos])).warp(2, wFm, r.u(20, 60, 5))
    p.noise(r.u(25, 55, 5), r.pick([2, 0, 3]))
    p.lfo(1, r.pick([randomShape, drift]), r.log(1, 12), 0, 0).mod(lfo1, "osc2WarpAmount", r.u(15, 40, 5)).mod(lfo1, "osc2Fine", r.u(10, 40, 10))
    p.lfo(2, drift, r.log(0.05, 0.5), 0).mod(lfo2, "filter1Cutoff", r.u(5, 15))
    p.filter(1, r.pick([svfBand, ladderBand]), r.log(800, 3500), r.u(30, 65, 5))
    bed_amp(p, r)
    if r.chance(0.5): dirt(p, r, (crush, tube, hard), 15, 40)
    hall(p, r, r.u(10, 30, 5))
    p.level(-9)
    knobs(p, r, ("Tune", "filter1Cutoff", 18), ("Static", "noiseLevel", 35), k_space(35), k_grit(55, crush), k_motion(1, 25))


@kind("Texture", 8, "Stream Feed Dump Log Trace Readout Printout")
def tex_data(p, r):
    # Pitched digital noise with a pattern on its colour: data going past.
    p.set("osc1On", 0).noise(r.u(70, 95, 5), 3, 0)
    pattern = [r.pick([-100, -60, -20, 20, 60, 100]) for _ in range(16)]
    p.lfoSync(1, steps, r.pick([bar1, half, quarter, bars2]), 0, r.u(0, 40, 20)).stepPattern(1, pattern).mod(lfo1, "noiseColour", r.u(20, 45, 5))
    p.lfo(2, r.pick([drift, randomShape]), r.log(0.2, 6), 0).mod(lfo2, "filter1Cutoff", r.u(5, 15))
    p.filter(1, r.pick([svfBand, svfHigh, ladderBand, svfLow]), r.log(700, 5000), r.u(15, 55, 5))
    bed_amp(p, r)
    if r.chance(0.6): echo(p, r, r.u(12, 26, 2), (sixteenth, sixteenthT, thirtySecond, eighthT))
    hall(p, r, r.u(10, 30, 5))
    p.level(-9)
    knobs(p, r, k_cut(20), ("Rate", "noiseColour", 30), k_space(35), k_grit(55, crush), k_res(35))


@kind("Texture", 8, "Swarm Nest Colony Brood Infestation Cloud")
def tex_swarm(p, r):
    # Eight detuned, drifting voices of a thin wave, high-passed: insects.
    p.osc(1, r.pick([pulse, metal, analog, sync]), r.u(50, 95, 5)).unison(1, 8, r.u(25, 70, 5), 100, 0, r.u(50, 100, 10)).pitch(1, r.pick([0, 12, 12, 24]))
    p.filter(1, r.pick([svfHigh, svfBand, ladderHigh]), r.log(900, 5000), r.u(10, 45, 5))
    p.lfo(1, r.pick([drift, randomShape]), r.log(0.3, 7), 0).mod(lfo1, "osc1UnisonDetune", r.u(8, 20, 2)).mod(lfo1, "ampLevel", r.u(3, 10))
    bed_amp(p, r)
    if r.chance(0.5): dirt(p, r, (hard, tube, crush), 15, 35)
    hall(p, r, r.u(15, 40, 5))
    p.level(-9)
    knobs(p, r, k_cut(18), ("Scatter", "osc1UnisonDetune", 20), k_space(35), k_grit(55, hard), k_motion(1, 25), k_res(35))


# =============================================================================================
# Vox
# =============================================================================================

FORMANTS = [(800, 1150), (400, 1600), (300, 2250), (450, 800), (325, 700), (650, 1080), (500, 1500), (270, 2300), (570, 840)]


def formants(p, r, res=(60, 80)):
    f1, f2 = r.pick(FORMANTS)
    p.parallel().filter(1, svfBand, f1, r.u(res[0], res[1], 5)).filter(2, svfBand, f2, r.u(res[0], res[1], 5))


@kind("Vox", 6, "Choir Chorus Congregation Assembly Throng Crowd Mass")
def vox_formant_choir(p, r):
    # Saws in unison through a vowel's two formants: a choir, its vowel on a macro.
    p.osc(1, analog, r.pick([sawPos, sawPos, 80])).unison(1, r.pick([4, 5, 6, 8]), r.u(10, 22, 2), 100, 0, r.u(10, 40, 10)).route(1, 50)
    formants(p, r)
    p.lfo(1, sine, r.u(4.5, 6, 0.25), 1).lfoFade(1, r.u(400, 900, 100)).mod(lfo1, "osc1Fine", r.u(3, 8))
    p.lfo(2, drift, r.log(0.05, 0.4), 0).mod(lfo2, "filter2Cutoff", r.u(2, 6))
    p.amp(r.u(150, 900, 50), 1000, r.u(85, 100, 5), r.u(600, 2000, 100))
    hall(p, r, r.u(30, 50, 5))
    p.level(-9)
    knobs(p, r, ("Vowel", "filter2Cutoff", -12), ("Mouth", "filter1Cutoff", 12), k_space(35), k_detune(1, 25))


@kind("Vox", 6, "Whisper Hiss Murmur Rumour Secret Hush Sigh")
def vox_whisper(p, r):
    # Noise through the formants, with no pitch: whispering.
    p.set("osc1On", 0).noise(100, r.pick([0, 1]), r.u(-20, 40, 20)).route(1, 50)
    formants(p, r, (40, 65))
    p.lfo(1, r.pick([drift, randomShape]), r.log(0.5, 6), 0).mod(lfo1, "filter2Cutoff", r.u(3, 9)).mod(lfo1, "filter1Cutoff", -r.u(2, 6))
    p.lfo(2, drift, r.log(0.3, 3), 0).mod(lfo2, "ampLevel", r.u(5, 14))
    p.amp(r.u(60, 600, 20), 800, r.u(80, 100, 5), r.u(400, 1500, 100))
    hall(p, r, r.u(25, 50, 5))
    p.level(-9)
    knobs(p, r, ("Vowel", "filter2Cutoff", -12), ("Mouth", "filter1Cutoff", 12), k_space(35), ("Speech", "lfo1Rate", 25))


@kind("Vox", 6, "Automaton Drone Unit Proxy Golem Puppet")
def vox_robot(p, r):
    # A pulse through formants that a step pattern moves in time, then crushed: a machine reading aloud.
    p.osc(1, *wave(r, [pulse, analog], [squarePos, sawPos])).route(1, 50)
    formants(p, r, (60, 85))
    pattern = [r.pick([-100, -50, 0, 50, 100]) for _ in range(16)]
    p.lfoSync(1, steps, r.pick([bar1, bars2, half]), 1, r.u(60, 92, 4)).stepPattern(1, pattern)
    p.mod(lfo1, "filter2Cutoff", r.u(6, 14, 2)).mod(lfo1, "filter1Cutoff", -r.u(3, 8))
    p.amp(5, 300, 95, r.u(100, 300, 20)).mono(r.pick([0, 30, 80]), True)
    p.drive(r.pick([crush, crush, hard]), r.u(20, 50, 5), r.log(4000, 10000))
    if r.chance(0.5): echo(p, r, r.u(10, 22, 2))
    p.level(-9)
    knobs(p, r, ("Vowel", "filter2Cutoff", -12), ("Mouth", "filter1Cutoff", 12), k_grit(35, crush), k_echo(30))


@kind("Vox", 6, "Growl Snarl Rasp Croak Gurgle Rattle Retch")
def vox_throat(p, r):
    # The growl table through low formants and a clipper, one note at a time: a voice with a throat.
    p.osc(1, growl, r.u(20, 70, 5)).unison(1, r.pick([1, 2, 3]), r.u(6, 14, 2), r.u(30, 70, 10)).route(1, 50)
    formants(p, r, (55, 78))
    p.env(2, r.pick([0, 30, 80]), r.u(200, 700, 50), r.u(20, 50, 10), 200, 60).mod(env2, "filter2Cutoff", r.u(4, 12, 2)).mod(env2, "osc1Position", r.u(10, 30, 5))
    p.amp(r.pick([3, 10, 30]), 400, 90, r.u(150, 400, 50)).mono(r.u(30, 120, 10), True)
    dirt(p, r, (hard, fuzz, tube), 25, 55, (3000, 8000))
    hall(p, r, r.u(10, 30, 5))
    p.level(-9)
    knobs(p, r, ("Vowel", "filter2Cutoff", -12), ("Growl", "osc1Position", 30), k_space(35), k_grit(35, hard))


@kind("Vox", 6, "Chant Mantra Incantation Recital Liturgy Verse Refrain")
def vox_chant(p, r):
    # The vowel table stepped through in time: syllables from a held note.
    p.osc(1, vowel, 50).unison(1, r.pick([2, 3, 4, 6]), r.u(8, 18, 2), 100, 0, r.u(0, 30, 10))
    if r.chance(0.5): p.osc(2, vowel, r.u(20, 80, 10), r.u(45, 65, 5)).pitch(2, r.pick([-12, 7, 12]), r.u(3, 8))
    pattern = [r.pick([-100, -60, -20, 20, 60, 100]) for _ in range(8)] * 2
    p.lfoSync(1, steps, r.pick([bar1, bars2, bars2, bars4]), 1, r.u(85, 97)).stepPattern(1, pattern).mod(lfo1, "osc1Position", r.u(35, 50, 5))
    p.filter(1, r.pick([ladder12, svfLow]), r.log(1500, 5000), r.u(10, 30, 5))
    p.amp(r.u(30, 400, 10), 600, r.u(85, 100, 5), r.u(400, 1500, 100))
    hall(p, r, r.u(30, 50, 5))
    p.level(-9)
    knobs(p, r, ("Vowel", "osc1Position", 30), k_cut(22), k_space(35), k_detune(1, 25))


# =============================================================================================
# Keys
# =============================================================================================

@kind("Keys", 9, "Tine Reed Fork Prong Rod Bar Key")
def keys_fm_tine(p, r):
    # The FM electric piano: a sine bent by another at the same pitch, hard when struck hard, and a high tine that dies at once.
    p.osc(1, analog, sinePos, 1).pitch(1, r.pick([0, 0, 0, 12]))
    p.osc(2, analog, sinePos).warp(2, wFm, r.u(0, 10, 5))
    p.osc(3, analog, sinePos, 1).pitch(3, r.pick([16, 19, 22, 24]), r.u(0, 10, 5))
    p.env(2, 0, r.u(500, 1800, 100), r.u(0, 15, 5), 400, r.u(60, 80, 5)).mod(env2, "osc2WarpAmount", r.u(12, 35)).mod(velocity, "osc2WarpAmount", r.u(15, 30, 5))
    p.env(3, 0, r.u(60, 250, 10), 0, 100, 85).mod(env3, "osc3Volume", r.u(30, 55, 5))
    p.filter(1, r.pick([ladder12, svfLow]), r.log(3500, 10000), 5, 0, 30)
    p.amp(0, r.u(1500, 4500, 100), r.pick([0, 0, 15]), r.u(250, 600, 50), r.u(50, 70, 5))
    fx = r.pick(["chorus", "phaser", "tremolo", "none"])
    if fx == "chorus": p.chorus(r.u(25, 45, 5), r.u(0.3, 1.0, 0.1), r.u(30, 50, 10))
    if fx == "phaser": p.phaser(r.u(25, 45, 5), r.log(0.1, 0.8), r.u(30, 55, 5))
    if fx == "tremolo": p.lfo(1, sine, r.u(3.5, 6.5, 0.5), 0).mod(lfo1, "ampPan", r.u(15, 35, 5))
    room(p, r) if r.chance(0.5) else hall(p, r, r.u(12, 28, 4))
    p.level(-9)
    knobs(p, r, ("Bark", "osc2WarpAmount", 30), ("Tine", "osc3Volume", 35), k_space(35), k_grit(55), k_wide(45), k_damp(30))


@kind("Keys", 8, "Reed Bellows Squeezebox Wheeze Drone Pump Lung")
def keys_reed(p, r):
    # A pulse and a saw under a soft low-pass that opens with velocity, with tremolo: the reed piano, worn.
    p.osc(1, pulse, r.u(20, 65, 5)).osc(2, analog, r.pick([sawPos, 80, trianglePos]), r.u(45, 70, 5)).pitch(2, r.pick([0, 12, -12]), r.u(2, 7))
    p.filter(1, r.pick([ladder24, ladder12, svfLow]), r.log(500, 1800), r.u(5, 25, 5), r.u(0, 40, 10), r.pick([40, 60]))
    p.env(2, 0, r.u(200, 900, 50), r.u(10, 40, 10), 250, 70).mod(env2, "filter1Cutoff", r.u(8, 20, 2)).mod(velocity, "filter1Cutoff", r.u(10, 22, 2))
    p.lfo(1, sine, r.u(3.5, 7, 0.5), 0).mod(lfo1, "ampLevel", r.u(3, 9))
    p.amp(r.pick([1, 3, 6]), r.u(1200, 3500, 100), r.u(20, 55, 5), r.u(200, 500, 50), r.u(50, 70, 5))
    if r.chance(0.5): dirt(p, r, (tube, soft), 15, 40, (3000, 8000))
    room(p, r) if r.chance(0.5) else hall(p, r, r.u(12, 28, 4))
    p.level(-9)
    knobs(p, r, k_cut(25), ("Tremolo", "lfo1Rate", 20), k_space(35), k_grit(55), k_wide(45), ("Width", "osc1Position", 30))


@kind("Keys", 8, "Clav Pick Pluck Twang Snap Spank Chank")
def keys_clav(p, r):
    # A thin pulse through a high-pass and a snapping band: the clav, for funk played by machines.
    p.osc(1, pulse, r.u(45, 85, 5)).osc(2, r.pick([pulse, analog]), r.pick([60, 75, sawPos]), r.u(40, 60, 5)).pitch(2, r.pick([0, 12]), r.u(2, 6))
    p.filter(1, r.pick([ladderBand, svfBand, ladder12]), r.log(500, 1600), r.u(25, 55, 5), r.u(0, 40, 10), r.pick([50, 70, 100]))
    p.filter(2, svfHigh, r.log(120, 400), 10)
    p.env(2, 0, r.u(80, 300, 10), 0, 150, r.u(75, 88, 4)).mod(env2, "filter1Cutoff", r.u(15, 35)).mod(velocity, "filter1Cutoff", r.u(8, 18, 2))
    p.amp(0, r.u(500, 1500, 50), 0, r.u(80, 200, 20), r.u(55, 75, 5))
    if r.chance(0.6): dirt(p, r, (tube, hard), 15, 45)
    if r.chance(0.5): p.phaser(r.u(25, 45, 5), r.log(0.15, 1.2), r.u(40, 65, 5))
    room(p, r)
    p.level(-9)
    knobs(p, r, k_cut(25), k_res(30), k_space(35), k_grit(55, hard), k_phase(50), ("Width", "osc1Position", -30))


@kind("Keys", 9, "Upright Grand Spinet Parlour Salon Cabinet Cottage")
def keys_string_piano(p, r):
    # Plucked strings, two or three to a note and slow to die, under a low-pass that opens with velocity: a piano in the dark.
    p.osc(1, string, r.u(30, 65, 5), 90).warp(1, wOff, r.u(55, 85, 5)).unison(1, r.pick([2, 3]), r.u(2, 7), r.u(20, 60, 10))
    p.osc(2, string, r.u(20, 50, 5), r.u(50, 75, 5)).warp(2, wOff, r.u(50, 80, 5)).pitch(2, r.pick([-12, 12, 0]), r.u(0, 5))
    p.filter(1, r.pick([ladder12, svfLow, ladder24]), r.log(900, 4000), 5, 0, r.pick([40, 60]))
    p.mod(velocity, "osc1Position", r.u(15, 30, 5)).mod(velocity, "filter1Cutoff", r.u(8, 20, 2))
    p.amp(0, r.u(3000, 7000, 500), 0, r.u(250, 700, 50), r.u(45, 65, 5))
    if r.chance(0.4): p.chorus(r.u(15, 30, 5), r.u(0.2, 0.6, 0.1), 40)
    hall(p, r, r.u(15, 40, 5))
    p.level(-9)
    knobs(p, r, ("Bright", "osc1Position", 30), k_cut(22), k_space(35), ("Ring", "osc1WarpAmount", 15), k_grit(55), k_echo(30))


@kind("Keys", 8, "Quill Jack Plectrum Virginal Manual Stop Register")
def keys_harpsichord(p, r):
    # Bright strings an octave apart with a short ring and the lows taken out: a harpsichord.
    p.osc(1, string, r.u(75, 100, 5), 90).warp(1, wOff, r.u(30, 55, 5))
    p.osc(2, string, r.u(70, 100, 5), r.u(55, 80, 5)).warp(2, wOff, r.u(30, 55, 5)).pitch(2, r.pick([12, 12, -12]), r.u(0, 4))
    if r.chance(0.4): p.osc(3, pulse, r.u(60, 90, 5), r.u(30, 45, 5)).pitch(3, r.pick([0, 12]))
    p.filter(1, svfHigh, r.log(150, 500), r.u(5, 20, 5))
    p.filter(2, r.pick([ladder12, svfLow]), r.log(5000, 14000), 5)
    p.amp(0, r.u(1200, 3000, 100), 0, r.u(150, 400, 50), r.u(50, 70, 5))
    room(p, r) if r.chance(0.5) else hall(p, r, r.u(12, 30))
    p.level(-9)
    knobs(p, r, ("Bright", "osc1Position", -40), ("Ring", "osc1WarpAmount", 25), k_space(35), k_grit(55, hard), ("Thin", "filter1Cutoff", 20), k_echo(30))


@kind("Keys", 8, "Toy Nursery Trinket Bauble Charm Lullaby Mobile")
def keys_toy(p, r):
    # Bars struck by small hammers: a toy piano, a celesta with a few teeth missing.
    p.osc(1, r.pick([additive, metal, fm]), r.u(5, 45, 5)).osc(2, analog, sinePos, r.u(55, 80, 5)).pitch(2, r.pick([12, 24, 0]))
    p.noise(1, 0, 40)
    p.env(2, 0, r.u(8, 30, 2), 0, 20, 90).mod(env2, "noiseLevel", r.u(20, 45, 5))
    p.env(3, 0, r.u(100, 500, 20), 0, 200, 80).mod(env3, "osc1Position", r.u(8, 25))
    p.filter(1, r.pick([ladder12, svfLow]), r.log(3000, 10000), 5, 0, 30)
    p.amp(0, r.u(500, 1600, 50), 0, r.u(200, 500, 50), r.u(55, 75, 5))
    room(p, r) if r.chance(0.5) else hall(p, r, r.u(12, 30))
    if r.chance(0.5): echo(p, r, r.u(8, 18, 2))
    p.level(-9)
    knobs(p, r, ("Hammer", "noiseLevel", 30), k_damp(30), k_space(35), ("Bars", "osc1Position", 30), k_echo(30), k_wide(45))


# =============================================================================================
# Organ
# =============================================================================================

@kind("Organ", 9, "Drawbar Console Manual Tonewheel Stop Swell Cabinet")
def organ_drawbar(p, r):
    # Three sets of harmonics at the drawbars' pitches, a click of the highest at the front, and a chorus for the spinning speaker.
    feet = r.pick([(0, 12, 19), (-12, 0, 12), (0, 12, 24), (-12, 0, 19), (0, 7, 12), (-12, 12, 24)])
    p.osc(1, additive, r.u(0, 25, 5)).pitch(1, feet[0]).osc(2, additive, r.u(0, 20, 5), r.u(55, 80, 5)).pitch(2, feet[1])
    p.osc(3, additive, r.u(0, 10, 5), r.u(30, 50, 5)).pitch(3, feet[2])
    p.env(2, 0, r.u(120, 400, 20), 0, 100, 80).mod(env2, "osc3Volume", r.u(20, 45, 5))
    p.filter(1, r.pick([ladder12, svfLow]), r.log(2500, 9000), 5)
    p.amp(r.pick([1, 2, 4]), 300, 100, r.u(40, 120, 10))
    p.chorus(r.u(30, 55, 5), r.pick([0.7, 0.8, 4.5, 5.0, 5.0]), r.u(20, 45, 5))
    if r.chance(0.6): dirt(p, r, (tube, soft), 15, 45, (3500, 9000))
    room(p, r) if r.chance(0.6) else hall(p, r, r.u(15, 30, 5))
    p.level(-9)
    knobs(p, r, ("Drawbars", "osc1Position", 30), ("Percussion", "osc3Volume", 35), k_grit(55), k_space(35), ("Rotor", "fxChorusMix", 40), k_cut(20))


@kind("Organ", 9, "Nave Chapel Chancel Cloister Transept Vestry Loft")
def organ_pipe(p, r):
    # Ranks of pipes: harmonics in octaves that speak slowly, with breath at the start of each note, in a church.
    p.osc(1, additive, r.u(5, 40, 5)).osc(2, additive, r.u(0, 25, 5), r.u(55, 80, 5)).pitch(2, r.pick([12, -12, 12]))
    p.osc(3, r.pick([additive, analog, pulse]), r.u(0, 40, 10), r.u(35, 55, 5)).pitch(3, r.pick([24, 19, 7, -12]))
    p.noise(1, 1, r.u(0, 60, 20))
    p.env(2, r.u(5, 30, 5), r.u(60, 250, 10), 0, 80, 70).mod(env2, "noiseLevel", r.u(20, 45, 5))
    p.filter(1, r.pick([ladder12, svfLow]), r.log(1800, 7000), 5)
    p.amp(r.u(25, 120, 5), 300, 100, r.u(150, 500, 50))
    p.reverb(r.u(35, 55, 5), r.u(85, 100, 5), r.u(4, 9, 0.5), r.u(30, 55, 5))
    p.level(-9)
    knobs(p, r, ("Ranks", "osc1Position", 30), ("Breath", "noiseLevel", 30), ("Mixture", "osc3Volume", 35), ("Space", "fxReverbMix", 35))


@kind("Organ", 9, "Combo Transistor Console Portable Rig Stage Tour")
def organ_combo(p, r):
    # Squares and pulses with a fast vibrato and no bass to speak of: the thin electric organ.
    p.osc(1, *wave(r, [analog, pulse], [squarePos, 90])).osc(2, *wave(r, [analog, pulse], [squarePos]), r.u(50, 75, 5)).pitch(2, r.pick([12, 12, 24, 7]))
    p.lfo(1, sine, r.u(5.5, 7.5, 0.25), 0).mod(lfo1, "osc1Fine", r.u(4, 12)).mod(lfo1, "osc2Fine", r.u(4, 12))
    p.filter(1, r.pick([ladder12, svfLow, svfBand]), r.log(1800, 6000), r.u(5, 30, 5))
    p.filter(2, svfHigh, r.log(120, 350), 10)
    p.amp(r.pick([1, 2]), 300, 100, r.u(30, 100, 10))
    if r.chance(0.6): dirt(p, r, (tube, hard, soft), 15, 45, (3500, 9000))
    room(p, r) if r.chance(0.6) else hall(p, r, r.u(15, 30, 5))
    p.level(-9)
    knobs(p, r, k_cut(22), ("Octave", "osc2Volume", -45), k_grit(55), k_space(35), k_wide(45))


@kind("Organ", 9, "Sermon Heresy Penance Vespers Benediction Confession Absolution")
def organ_dirty(p, r):
    # A full organ into a clipper and a fast rotor: what an organ does in an industrial track.
    p.osc(1, additive, r.u(15, 60, 5)).osc(2, additive, r.u(10, 50, 5), r.u(60, 80, 5)).pitch(2, r.pick([-12, 12, 7]))
    p.osc(3, r.pick([analog, growl, sync]), r.u(20, 70, 10), r.u(35, 55, 5)).pitch(3, r.pick([0, -12, 19]), r.u(3, 9))
    p.filter(1, r.pick([ladder24, ladder12, svfLow]), r.log(1200, 5000), r.u(10, 35, 5), r.u(20, 60, 10))
    p.amp(r.pick([1, 3, 10]), 300, 100, r.u(60, 200, 20))
    p.drive(r.pick([hard, fuzz, tube, foldDrive]), r.u(35, 65, 5), r.log(2500, 7000))
    p.chorus(r.u(25, 50, 5), r.pick([0.6, 4.0, 5.0, 5.0]), r.u(20, 45, 5))
    hall(p, r, r.u(15, 35, 5))
    p.level(-9)
    knobs(p, r, k_cut(22), k_res(35), ("Fuzz", "fxDriveAmount", 30), k_space(35), ("Rotor", "fxChorusMix", 40), k_bite(40))


@kind("Organ", 8, "Harmonium Bellows Treadle Reed Wheeze Lung Squeeze")
def organ_reed(p, r):
    # Reeds that swell in and waver: a harmonium pumped by foot.
    p.osc(1, r.pick([sync, growl, pulse]), r.u(10, 45, 5)).osc(2, additive, r.u(10, 45, 5), r.u(55, 80, 5)).pitch(2, r.pick([0, 12, -12]), r.u(3, 8))
    p.unison(1, 2, r.u(5, 12), r.u(30, 70, 10))
    p.filter(1, r.pick([ladder12, svfLow, ladder24]), r.log(1200, 4000), r.u(5, 30, 5))
    p.lfo(1, r.pick([sine, drift]), r.log(0.3, 2.5), 0).mod(lfo1, "ampLevel", r.u(3, 9)).mod(lfo1, "filter1Cutoff", r.u(2, 7))
    p.amp(r.u(60, 350, 10), 400, 100, r.u(200, 600, 50))
    room(p, r) if r.chance(0.5) else hall(p, r, r.u(15, 35, 5))
    p.level(-9)
    knobs(p, r, k_cut(22), ("Reeds", "osc1Position", 30), k_grit(55), k_space(35), ("Pump", "lfo1Rate", 20), k_detune(1, 25))


# =============================================================================================
# Brass
# =============================================================================================

def brass_env(p, r, amount=(22, 40), attack=(30, 130)):
    p.env(2, r.u(attack[0], attack[1], 10), r.u(250, 900, 50), r.u(40, 75, 5), 250, r.u(40, 65, 5))
    p.mod(env2, "filter1Cutoff", r.u(amount[0], amount[1], 2)).mod(velocity, "filter1Cutoff", r.u(6, 16, 2))


@kind("Brass", 9, "Section Rank Cohort Brigade Phalanx Guard")
def brass_section(p, r):
    # Saws in unison behind a ladder that opens a little late: synth brass.
    p.osc(1, analog, sawPos).unison(1, r.pick([2, 3, 4, 5]), r.u(8, 18, 2), r.u(50, 90, 10))
    p.osc(2, analog, r.pick([sawPos, 80, squarePos]), r.u(55, 75, 5)).pitch(2, r.pick([0, -12, 12]), r.u(4, 10))
    p.filter(1, ladder24, r.log(250, 800), r.u(10, 35, 5), r.u(0, 40, 10), r.pick([30, 50]))
    brass_env(p, r)
    p.amp(r.u(10, 60, 5), 400, r.u(80, 95, 5), r.u(150, 400, 50))
    vibrato(p, r, (1,), r.u(6, 12, 2), 1)
    if r.chance(0.5): p.chorus(r.u(15, 35, 5), r.u(0.3, 0.8, 0.1), 40)
    room(p, r) if r.chance(0.4) else hall(p, r, r.u(15, 35, 5))
    p.level(-9)
    knobs(p, r, k_cut(25), ("Blare", "filter1Resonance", 35), k_space(35), k_grit(55, hard), k_detune(1, 25), k_bite(50))


@kind("Brass", 8, "Herald Clarion Fanfare Reveille Tattoo Salute Summons")
def brass_fm(p, r):
    # The FM table's index rising with the envelope: FM brass, bright at the top of each note.
    p.osc(1, fm, r.u(5, 25, 5)).unison(1, r.pick([1, 2, 3]), r.u(6, 12, 2), r.u(40, 80, 10))
    p.osc(2, analog, sawPos, r.u(40, 60, 5)).pitch(2, r.pick([0, -12]), r.u(3, 8))
    p.env(2, r.u(30, 120, 10), r.u(300, 900, 50), r.u(40, 70, 5), 250, 50).mod(env2, "osc1Position", r.u(20, 45, 5)).mod(velocity, "osc1Position", r.u(8, 18, 2))
    p.filter(1, r.pick([ladder12, svfLow, ladder24]), r.log(2000, 7000), r.u(5, 25, 5))
    p.amp(r.u(10, 50, 5), 400, r.u(80, 95, 5), r.u(150, 400, 50))
    vibrato(p, r, (1,), r.u(6, 12, 2), 1)
    room(p, r) if r.chance(0.4) else hall(p, r, r.u(15, 35, 5))
    p.level(-9)
    knobs(p, r, ("Index", "osc1Position", 30), k_cut(22), k_space(35), k_grit(55, hard), k_wide(45), k_echo(30))


@kind("Brass", 8, "Horn Bugle Hunt Call Valley Summit Ridge")
def brass_soft_horn(p, r):
    # A wave between triangle and saw, a low filter and a slow start: horns across a valley.
    p.osc(1, analog, r.u(40, 62, 2)).unison(1, r.pick([2, 3]), r.u(5, 12), r.u(40, 80, 10))
    p.osc(2, analog, r.u(35, 60, 5), r.u(50, 70, 5)).pitch(2, r.pick([0, -12, 7]), r.u(3, 8))
    p.filter(1, r.pick([ladder24, ladder12]), r.log(220, 600), r.u(5, 25, 5), 0, 50)
    brass_env(p, r, (15, 28), (80, 250))
    p.amp(r.u(60, 220, 10), 500, r.u(85, 100, 5), r.u(300, 800, 100))
    hall(p, r, r.u(30, 50, 5))
    p.level(-9)
    knobs(p, r, k_cut(22), ("Brass", "osc1Position", 30), k_space(35), k_grit(55), k_detune(1, 25), k_res(30))


@kind("Brass", 8, "Blare Blast Bray Honk Klaxon Trump Bellow")
def brass_sync(p, r):
    # The sync table opening with the envelope: brass with an edge that tears.
    p.osc(1, sync, r.u(5, 25, 5)).unison(1, r.pick([2, 3, 4]), r.u(8, 16, 2), r.u(50, 90, 10))
    p.osc(2, analog, sawPos, r.u(45, 65, 5)).pitch(2, r.pick([-12, 0]), r.u(4, 9))
    p.env(2, r.u(20, 100, 10), r.u(250, 800, 50), r.u(30, 60, 5), 250, 55).mod(env2, "osc1Position", r.u(15, 40, 5)).mod(env2, "filter1Cutoff", r.u(10, 25, 5))
    p.filter(1, ladder24, r.log(500, 1800), r.u(10, 35, 5), r.u(10, 50, 10), 40)
    p.amp(r.u(5, 40, 5), 400, r.u(80, 95, 5), r.u(150, 400, 50))
    if r.chance(0.6): dirt(p, r, (hard, tube), 15, 40)
    room(p, r) if r.chance(0.4) else hall(p, r, r.u(15, 35, 5))
    p.level(-9)
    knobs(p, r, ("Sync", "osc1Position", 30), k_cut(22), k_space(35), k_grit(50, hard), k_detune(1, 25), k_res(30))


@kind("Brass", 8, "Banner Standard Ensign Pennant Colours Crest Mantle")
def brass_fifths(p, r):
    # Brass in fifths and octaves from one key: a fanfare's open chord.
    p.osc(1, analog, sawPos).unison(1, r.pick([2, 3, 4]), r.u(8, 16, 2), r.u(50, 90, 10))
    p.osc(2, analog, sawPos, r.u(60, 80, 5)).pitch(2, 7, r.u(0, 6)).unison(2, 2, r.u(6, 12, 2), 80)
    p.osc(3, analog, r.pick([sawPos, squarePos, 80]), r.u(45, 65, 5)).pitch(3, r.pick([12, -12]), r.u(3, 8))
    p.filter(1, ladder24, r.log(280, 900), r.u(10, 30, 5), r.u(0, 40, 10), 40)
    brass_env(p, r)
    p.amp(r.u(10, 70, 5), 400, r.u(80, 95, 5), r.u(200, 500, 50))
    hall(p, r, r.u(20, 40, 5))
    p.level(-9)
    knobs(p, r, k_cut(25), ("Fifth", "osc2Volume", -50), k_space(35), k_grit(55, hard), k_detune(1, 25), k_res(30))


@kind("Brass", 8, "Mute Stopper Plunger Bucket Cup Derby Felt")
def brass_muted(p, r):
    # A narrow pulse through a band that the envelope moves: a muted trumpet, pinched and nasal.
    p.osc(1, pulse, r.u(55, 90, 5)).osc(2, analog, sawPos, r.u(40, 60, 5)).pitch(2, 0, r.u(3, 9))
    p.filter(1, r.pick([svfBand, ladderBand]), r.log(700, 1800), r.u(35, 65, 5), r.u(0, 30, 10), 50)
    p.env(2, r.u(15, 80, 5), r.u(200, 700, 50), r.u(30, 60, 5), 200, 55).mod(env2, "filter1Cutoff", r.u(8, 20, 2)).mod(velocity, "filter1Cutoff", r.u(5, 12))
    p.amp(r.u(8, 40, 4), 400, r.u(80, 95, 5), r.u(120, 300, 20)).mono(r.pick([0, 30, 60]), True)
    vibrato(p, r, (1, 2), r.u(8, 16, 2), 1)
    room(p, r) if r.chance(0.5) else hall(p, r, r.u(15, 35, 5))
    if r.chance(0.5): echo(p, r, r.u(10, 20, 2))
    p.level(-9)
    knobs(p, r, k_cut(18), ("Pinch", "filter1Resonance", 25), k_space(35), k_grit(55, hard), ("Width", "osc1Position", -30), k_echo(30))


# =============================================================================================
# Strings
# =============================================================================================

@kind("Strings", 8, "Ensemble Section Desk Orchestra Consort Chamber Quartet")
def strings_ensemble(p, r):
    # Many saws a few cents apart through a soft filter and a chorus: the string machine.
    p.osc(1, analog, sawPos).unison(1, r.pick([5, 6, 7, 8]), r.u(12, 24, 2), r.u(80, 100, 10), 0, r.u(10, 40, 10))
    p.osc(2, analog, r.pick([sawPos, 80]), r.u(50, 75, 5)).pitch(2, r.pick([12, -12, 0]), r.u(4, 10)).unison(2, 2, r.u(6, 12, 2), 100)
    p.filter(1, r.pick([ladder12, svfLow]), r.log(1500, 6000), r.u(5, 20, 5))
    p.filter(2, svfHigh, r.log(120, 350), 10)
    p.amp(r.u(120, 700, 20), 600, r.u(85, 100, 5), r.u(400, 1400, 100))
    p.chorus(r.u(30, 55, 5), r.u(0.3, 0.9, 0.1), r.u(40, 60, 10))
    hall(p, r, r.u(25, 45, 5))
    p.level(-9)
    knobs(p, r, k_cut(22), k_detune(1, 25), k_space(35), ("Chorus", "fxChorusMix", 35), k_grit(55), ("Octave", "osc2Volume", -45))


@kind("Strings", 8, "Pizzicato Pluck Finger Thumb Nail Snap Tap")
def strings_pizzicato(p, r):
    # Short plucked strings with a little saw under them: pizzicato, for ostinatos.
    p.osc(1, string, r.u(30, 60, 5), 90).warp(1, wOff, r.u(15, 40, 5)).unison(1, r.pick([2, 3, 4]), r.u(3, 9), r.u(50, 90, 10), r.u(0, 12, 4))
    p.osc(2, analog, sawPos, r.u(30, 50, 5)).pitch(2, r.pick([0, -12]), r.u(3, 8))
    p.filter(1, r.pick([ladder12, svfLow, ladder24]), r.log(1200, 4000), 5, 0, 50)
    p.env(2, 0, r.u(60, 220, 10), 0, 120, 80).mod(env2, "filter1Cutoff", r.u(8, 20, 2)).mod(env2, "osc2Volume", r.u(10, 30, 5))
    p.amp(0, r.u(250, 700, 50), 0, r.u(150, 400, 50), r.u(55, 75, 5))
    hall(p, r, r.u(20, 40, 5))
    p.level(-9)
    knobs(p, r, ("Bright", "osc1Position", 30), ("Ring", "osc1WarpAmount", 30), k_space(35), k_cut(20), k_echo(30), k_grit(55))


@kind("Strings", 8, "Spiccato Staccato Marcato Bounce Strike Jab Stroke")
def strings_short(p, r):
    # Short bowed notes: saws in unison with a quick swell and a quick end.
    p.osc(1, analog, sawPos).unison(1, r.pick([4, 5, 6]), r.u(10, 20, 2), r.u(70, 100, 10))
    p.osc(2, analog, r.pick([sawPos, 80]), r.u(50, 70, 5)).pitch(2, r.pick([-12, 0, 12]), r.u(4, 9))
    p.noise(1, 1, 20)
    p.env(2, r.u(5, 25, 5), r.u(80, 250, 10), 0, 100, 70).mod(env2, "filter1Cutoff", r.u(10, 24, 2)).mod(env2, "noiseLevel", r.u(15, 35, 5))
    p.filter(1, r.pick([ladder12, svfLow, ladder24]), r.log(900, 3000), r.u(5, 25, 5))
    p.filter(2, svfHigh, r.log(100, 300), 10)
    p.amp(r.u(8, 40, 4), r.u(200, 600, 50), r.pick([0, 0, 30]), r.u(100, 300, 20))
    hall(p, r, r.u(20, 40, 5))
    p.level(-9)
    knobs(p, r, k_cut(22), ("Bow", "noiseLevel", 30), k_space(35), k_detune(1, 25), k_grit(55, hard), k_echo(30))


@kind("Strings", 8, "Tremolo Shiver Tremble Quiver Shudder Flutter Ripple")
def strings_tremolo(p, r):
    # The section shaking its bows: a fast, even beat in the level.
    p.osc(1, analog, sawPos).unison(1, r.pick([5, 6, 8]), r.u(12, 22, 2), r.u(80, 100, 10))
    p.osc(2, analog, sawPos, r.u(50, 70, 5)).pitch(2, r.pick([12, -12]), r.u(4, 9))
    p.lfo(1, r.pick([sine, triangle]), r.u(7, 13, 0.5), 0).mod(lfo1, "ampLevel", r.u(8, 18, 2))
    p.filter(1, r.pick([ladder12, svfLow]), r.log(1500, 5000), r.u(5, 20, 5))
    p.filter(2, svfHigh, r.log(150, 500), 10)
    p.amp(r.u(150, 900, 50), 600, r.u(85, 100, 5), r.u(400, 1200, 100))
    hall(p, r, r.u(25, 45, 5))
    p.level(-9)
    knobs(p, r, k_cut(22), ("Shake", "ampLevel", 10), k_space(35), ("Speed", "lfo1Rate", 12), k_detune(1, 25), k_grit(55))


@kind("Strings", 8, "Bow Rosin Hair Bridge Scroll Fingerboard Peg")
def strings_bowed_comb(p, r):
    # Noise and a saw driving a comb tuned to the key: a string under a bow, scratch and all.
    p.osc(1, analog, sawPos, r.u(40, 65, 5)).noise(r.u(30, 55, 5), 1, r.u(0, 60, 20))
    p.filter(1, comb, 261.6, r.u(80, 93), 0, 100)
    p.filter(2, r.pick([ladder12, svfLow]), r.log(1500, 6000), r.u(5, 20, 5), 0, 40)
    p.lfo(1, sine, r.u(4.5, 6.5, 0.25), 1).lfoFade(1, r.u(300, 800, 100)).mod(lfo1, "osc1Fine", r.u(4, 10)).mod(lfo1, "filter1Cutoff", round(r.u(0.1, 0.4, 0.1), 1))
    p.amp(r.u(60, 400, 20), 500, r.u(85, 100, 5), r.u(250, 800, 50))
    if r.chance(0.5): p.mono(r.u(40, 150, 10), True)
    hall(p, r, r.u(25, 45, 5))
    p.level(-9)
    knobs(p, r, ("Scratch", "noiseLevel", 30), ("Tone", "filter2Cutoff", 22), k_space(35), ("Ring", "filter1Resonance", 5), k_grit(55), k_echo(30))


@kind("Strings", 8, "Cello Viol Bass Contra Tenor Basso Burden")
def strings_low_section(p, r):
    # Low strings: saws an octave apart with a band for the body of the instrument beside the low-pass.
    p.osc(1, analog, sawPos).unison(1, r.pick([3, 4, 5]), r.u(8, 18, 2), r.u(60, 90, 10)).route(1, r.u(30, 60, 10))
    p.osc(2, analog, r.pick([sawPos, 80]), r.u(55, 75, 5)).pitch(2, -12, r.u(3, 8)).route(2, r.u(20, 50, 10))
    p.parallel()
    p.filter(1, r.pick([ladder12, ladder24]), r.log(700, 2500), r.u(5, 20, 5))
    p.filter(2, svfBand, r.log(250, 700), r.u(40, 65, 5))
    p.amp(r.u(80, 500, 20), 600, r.u(85, 100, 5), r.u(300, 1000, 100))
    vibrato(p, r, (1,), r.u(6, 12, 2), 1)
    hall(p, r, r.u(25, 45, 5))
    p.level(-9)
    knobs(p, r, k_cut(22), ("Body", "filter2Cutoff", 12), k_space(35), k_detune(1, 25), k_grit(55), ("Wood", "filter2Resonance", 25))


# =============================================================================================
# Chord: a chord from one key
# =============================================================================================

CHORDS = [(3, 7), (3, 7), (3, 10), (5, 7), (2, 7), (7, 12), (7, 15), (5, 10), (7, 10), (3, 6), (10, 15), (-5, 3), (-9, 7), (3, 14), (4, 7), (8, 15)]


def chord_oscs(p, r, table, position, unison=1, levels=(65, 85)):
    a, b = r.pick(CHORDS)
    p.osc(1, table, position)
    p.osc(2, table, position, r.u(levels[0], levels[1], 5)).pitch(2, a, r.u(-4, 4))
    p.osc(3, table, position, r.u(levels[0], levels[1], 5)).pitch(3, b, r.u(-4, 4))
    if unison > 1:
        for o in (1, 2, 3):
            p.unison(o, unison, r.u(6, 14, 2), r.u(50, 100, 10))


@kind("Chord", 10, "Verdict Creed Dogma Canon Tenet Decree Doctrine")
def chord_stab(p, r):
    # A chord of saws with a snap of filter into a delay: the techno chord stab, from one finger.
    chord_oscs(p, r, analog, r.pick([sawPos, sawPos, 80]), r.pick([1, 2, 2]))
    p.filter(1, r.pick([ladder24, ladder24, ladder12, svfLow]), r.log(400, 1600), r.u(15, 45, 5), r.u(0, 30, 10))
    p.env(2, 0, r.u(90, 320, 10), 0, r.u(100, 220, 20), r.u(70, 88, 6)).mod(env2, "filter1Cutoff", r.u(18, 36, 2)).mod(velocity, "filter1Cutoff", r.u(5, 14))
    p.amp(0, r.u(200, 500, 20), 0, r.u(120, 280, 20))
    p.delay(r.pick([quarterD, eighthD, quarterT, eighth]), r.u(40, 62, 2), r.u(18, 36, 2), r.log(1500, 5000))
    hall(p, r, r.u(15, 35, 5))
    p.level(-9)
    knobs(p, r, k_cut(28), k_res(30), k_space(35), k_grit(55), ("Echo", "fxDelayMix", 35))


@kind("Chord", 10, "Shelter Refuge Haven Asylum Sanctuary Harbour Retreat")
def chord_pad(p, r):
    # The chord held and slow, in unison: a whole pad part from one key.
    t = r.pick([analog, analog, pulse, additive, vowel])
    chord_oscs(p, r, t, r.pick([sawPos, 80]) if t == analog else r.u(15, 60, 5), r.pick([2, 3]))
    p.filter(1, r.pick(LOWPASS), r.log(600, 3000), r.u(10, 35, 5))
    p.lfo(1, r.pick([sine, triangle, drift]), r.log(0.04, 0.4), 0).mod(lfo1, "filter1Cutoff", r.u(5, 14))
    pad_amp(p, r)
    if r.chance(0.6): p.chorus(r.u(25, 45, 5), r.u(0.2, 0.8, 0.1), 50)
    hall(p, r, r.u(30, 55, 5))
    p.level(-9)
    knobs(p, r, k_cut(28), ("Third", "osc2Volume", -55), k_space(35), k_detune(1, 25), k_motion(1, 30), k_grit(55))


@kind("Chord", 10, "Vow Oath Pledge Pact Bond Troth")
def chord_organ(p, r):
    # The chord on drawbar harmonics with a rotor: an organ chord that needs no left hand.
    chord_oscs(p, r, additive, r.u(5, 45, 5))
    p.filter(1, r.pick([ladder12, svfLow]), r.log(1800, 7000), 5)
    p.amp(r.pick([1, 3, 10, 30]), 300, r.pick([100, 100, 40, 0]), r.u(60, 300, 20))
    p.chorus(r.u(30, 55, 5), r.pick([0.7, 4.5, 5.0]), r.u(20, 45, 5))
    if r.chance(0.6): dirt(p, r, (tube, hard, soft), 15, 50, (3000, 8000))
    room(p, r) if r.chance(0.5) else hall(p, r, r.u(15, 35, 5))
    p.level(-9)
    knobs(p, r, ("Drawbars", "osc1Position", 30), ("Third", "osc2Volume", -55), k_space(35), k_grit(55), ("Rotor", "fxChorusMix", 40), k_cut(20))


@kind("Chord", 10, "Mirage Vision Phantom Spectre Reverie Trance Daydream")
def chord_fm(p, r):
    # The chord from the FM table, struck and fading: electric-piano chords for the breakdown.
    chord_oscs(p, r, fm, r.u(5, 35, 5))
    p.env(2, 0, r.u(300, 1500, 100), 0, 400, r.u(60, 80, 5))
    p.mod(env2, "osc1Position", r.u(10, 30, 5)).mod(env2, "osc2Position", r.u(10, 30, 5)).mod(env2, "osc3Position", r.u(10, 30, 5))
    p.filter(1, r.pick([ladder12, svfLow]), r.log(2000, 8000), 5)
    p.amp(0, r.u(1200, 4000, 100), r.pick([0, 0, 20]), r.u(300, 900, 100), r.u(50, 70, 5))
    fx = r.pick(["chorus", "phaser", "none"])
    if fx == "chorus": p.chorus(r.u(25, 45, 5), r.u(0.3, 1.0, 0.1), 40)
    if fx == "phaser": p.phaser(r.u(25, 45, 5), r.log(0.1, 0.8), r.u(30, 55, 5))
    if r.chance(0.6): echo(p, r)
    hall(p, r, r.u(15, 40, 5))
    p.level(-9)
    knobs(p, r, ("Index", "osc1Position", 30), k_damp(30), k_space(35), ("Third", "osc2Volume", -55))


@kind("Chord", 10, "Siege Riot Uprising Revolt Mutiny Unrest Curfew")
def chord_rave(p, r):
    # The chord on wide pulses with the pitch falling into place: the rave stab of the early nineties.
    chord_oscs(p, r, pulse, r.u(10, 45, 5), r.pick([2, 3, 4]))
    p.env(2, 0, r.u(60, 220, 10), 0, 150, r.u(60, 80, 5)).mod(env2, "osc1Fine", r.pick([-1, 1]) * r.u(20, 50, 10)).mod(env2, "filter1Cutoff", r.u(10, 26, 2))
    p.filter(1, r.pick([ladder24, ladder12, svfLow]), r.log(900, 3500), r.u(10, 35, 5))
    p.amp(0, r.u(250, 700, 50), r.pick([0, 30, 50]), r.u(150, 350, 50))
    p.chorus(r.u(25, 45, 5), r.u(0.3, 0.9, 0.1), 50)
    if r.chance(0.6): dirt(p, r, (tube, hard), 15, 40)
    room(p, r) if r.chance(0.5) else hall(p, r, r.u(15, 35, 5))
    p.level(-9)
    knobs(p, r, k_cut(25), ("Width", "osc1Position", 30), k_space(35), k_grit(55, hard), k_detune(1, 25), k_echo(30))


@kind("Chord", 10, "Harp Lyre Zither Psaltery Dulcimer Cittern Lute")
def chord_strum(p, r):
    # Three plucked strings tuned to the chord, two of them doubled by strings that come in a moment later: a strum from one key.
    a, b = r.pick(CHORDS)
    bright, ring = r.u(40, 80, 5), r.u(35, 70, 5)
    p.osc(1, string, bright, 90).warp(1, wOff, ring)
    p.osc(2, string, bright, r.u(70, 90, 5)).warp(2, wOff, ring).pitch(2, a).unison(2, 2, r.u(0, 6), 60, r.u(10, 30, 5))
    p.osc(3, string, bright, r.u(70, 90, 5)).warp(3, wOff, ring).pitch(3, b).unison(3, 2, r.u(0, 6), 60, r.u(25, 55, 5))
    p.filter(1, r.pick([ladder12, svfLow]), r.log(2500, 9000), 5)
    p.amp(0, r.u(1500, 4000, 100), 0, r.u(400, 1000, 100))
    if r.chance(0.5): dirt(p, r, (tube, hard, fuzz), 20, 55, (3000, 8000))
    if r.chance(0.5): echo(p, r)
    hall(p, r, r.u(15, 40, 5))
    p.level(-9)
    knobs(p, r, ("Bright", "osc1Position", 30), ("Ring", "osc1WarpAmount", 25), k_space(35), k_grit(55, hard), k_cut(20), k_echo(30))


# =============================================================================================
# Arp: a pattern of notes from one key
# =============================================================================================

ARPS = [[0, 3, 7, 12, 15, 19, 24, 19, 15, 12, 7, 3, 0, 3, 7, 12], [0, 12, 7, 12, 0, 12, 7, 12, 0, 12, 3, 12, 0, 12, 7, 10],
        [0, 7, 12, 7] * 4, [0, 3, 7, 10] * 4, [0, 7, 12, 19, 24, 19, 12, 7] * 2, [0, 5, 7, 12] * 4,
        [0, 3, 5, 7, 10, 12, 15, 17, 19, 17, 15, 12, 10, 7, 5, 3], [12, 7, 3, 0] * 4, [0, 12, 24, 12] * 4,
        [0, 2, 3, 7, 0, 2, 3, 10, 0, 2, 3, 7, 12, 10, 7, 3], [0, -5, 0, 3, 0, -5, 0, 7, 0, -5, 0, 3, 7, 3, 0, -2],
        [0, 1, 0, 1, 0, 3, 0, 1, 0, 1, 0, 1, 7, 6, 3, 1], [0, 0, 0, 12, 0, 0, 10, 0, 0, 0, 0, 12, 0, 7, 10, 12],
        [0, 7, 3, 10, 7, 14, 10, 17, 12, 7, 3, 10, 7, 3, 0, -5], [0, 3, 7, 3, 0, 3, 8, 3, 0, 3, 7, 3, -2, 3, 7, 3],
        [0, 12, 3, 12, 7, 12, 3, 12, 0, 12, 3, 12, 8, 12, 7, 12], [0, 7, 12, 15, 12, 7, 0, 7, 0, 7, 12, 17, 12, 7, 0, 7],
        [24, 19, 15, 12, 7, 3, 0, 3, 7, 12, 15, 19, 24, 19, 15, 12], [0, 0, 12, 0, 0, 12, 0, 12, 0, 0, 12, 0, 7, 12, 0, 12],
        [0, 3, 0, 7, 0, 3, 0, 10, 0, 3, 0, 7, 0, 12, 0, 10], [0, 5, 10, 15, 10, 5, 0, 5, 0, 5, 10, 15, 20, 15, 10, 5],
        [0, 6, 12, 6, 0, 6, 12, 18, 0, 6, 12, 6, 1, 7, 13, 7], [0, 2, 7, 9, 14, 9, 7, 2, 0, 2, 7, 9, 14, 19, 14, 9],
        [12, 0, 12, 0, 7, 0, 7, 0, 10, 0, 10, 0, 7, 0, 3, 0]]

NOTE_SHAPES = [[0, -25, -55, -100], [0, -40, -80, -100], [0, -15, -40, -70], [0, 0, -60, -100], [0, -50, -100, -100], [0, -10, -25, -45]]


def arps_within(p, oscs):
    """The patterns these oscillators can play whole: an oscillator's pitch stops at two octaves either way."""
    highest = 24 - max(p.get(f"osc{o}Pitch") for o in oscs)
    lowest = -24 - min(p.get(f"osc{o}Pitch") for o in oscs)
    return [a for a in ARPS if max(a) <= highest and min(a) >= lowest]


def arp_motion(p, r, oscs=(1,), rates=("16", "16", "16", "8", "32"), shapes=None, gate_filter=True):
    """LFO 1 steps the pitch; LFO 2 makes each step a note. Returns the rate chosen."""
    rate = r.pick(list(rates))
    cycle, note, smooth = {"8": (bars2, half, 99), "16": (bar1, quarter, 97), "32": (half, eighth, 94)}[rate]
    p.lfoSync(1, steps, cycle, 1, 0).stepPattern(1, step_notes(r.pick(arps_within(p, oscs))))

    for o in oscs:
        p.mod(lfo1, f"osc{o}Pitch", 50)

    shape = r.pick(shapes or NOTE_SHAPES)
    accents = r.pick([[0, 0, 0, 0], [0, -12, 0, -12], [0, -10, -20, -10], [0, 0, -15, 0]])
    p.lfoSync(2, steps, note, 1, smooth).stepPattern(2, [max(-100, s + a) for a in accents for s in shape])
    p.mod(lfo2, "ampLevel", r.u(70, 85, 5))

    if gate_filter:
        p.mod(lfo2, "filter1Cutoff", r.u(8, 22, 2))

    return rate


@kind("Arp", 10, "Clockwork Mechanism Movement Escapement Pendulum Balance Mainspring")
def arp_pluck(p, r):
    # Bright oscillators, a ladder that shuts on every step: the arpeggio as a row of plucks.
    p.osc(1, *wave(r, [analog, pulse, sync], [sawPos, squarePos]))
    p.osc(2, *wave(r, [analog, pulse], [sawPos, squarePos]), r.u(45, 70, 5)).pitch(2, r.pick([12, 0, -12, 7]), r.u(0, 8))
    arp_motion(p, r, (1, 2))
    p.filter(1, r.pick([ladder24, ladder12, svfLow]), r.log(500, 2400), r.u(15, 50, 5), 0, 30)
    p.amp(2, 300, 100, r.u(100, 300, 20))
    echo(p, r, r.u(12, 26, 2), (eighthD, sixteenthD, quarter))
    if r.chance(0.6): hall(p, r, r.u(12, 30))
    p.level(-9)
    knobs(p, r, k_cut(25), k_res(30), k_space(35), k_echo(30))


@kind("Arp", 10, "Patrol Sentry Picket Watch Rounds Beat Circuit")
def arp_bass(p, r):
    # A bassline from one key: the pattern moves the oscillator while the sub stays on the root under it.
    p.osc(1, *wave(r, [analog, analog, pulse, sync, fm], [sawPos, squarePos])).pitch(1, r.pick([-12, -12, 0]))
    p.subOsc(r.u(30, 50, 5), r.pick([0, 0, 1]))
    arp_motion(p, r, (1,), ("16", "16", "8"), NOTE_SHAPES[:5])
    p.filter(1, r.pick([ladder24, ladder24, ladder12]), r.log(300, 1300), r.u(20, 55, 5), r.u(10, 50, 10))
    p.amp(2, 300, 100, r.u(60, 160, 20)).mono(0, True)
    if r.chance(0.7): dirt(p, r, (tube, hard, fuzz), 15, 45)
    p.level(-9)
    knobs(p, r, k_cut(25), k_res(25), k_grit(50, hard), k_sub(40))


@kind("Arp", 10, "Carillon Chimes Peal Changes Rounds Tower Belfry")
def arp_bell(p, r):
    # FM bells stepping through the pattern, each ringing into the delay.
    p.osc(1, analog, sinePos, 1).pitch(1, r.pick([0, 7, 12, 12]), r.pick([0, 0, 20, -30]))
    p.osc(2, analog, sinePos).warp(2, wFm, r.u(10, 35, 5))
    arp_motion(p, r, (1, 2), ("16", "8", "8"), [NOTE_SHAPES[2], NOTE_SHAPES[5], NOTE_SHAPES[0]], False)
    p.mod(lfo2, "osc2WarpAmount", r.u(15, 40, 5))
    p.filter(1, r.pick([ladder12, svfLow]), r.log(3000, 10000), 5)
    p.amp(2, 300, 100, r.u(300, 900, 100))
    echo(p, r, r.u(16, 30, 2), (eighthD, quarter, quarterT))
    hall(p, r, r.u(20, 40, 5))
    p.level(-9)
    knobs(p, r, ("Index", "osc2WarpAmount", 30), k_damp(30), k_space(35), k_echo(30))


@kind("Arp", 10, "Solvent Reagent Catalyst Titrant Etchant Developer Fixer")
def arp_acid(p, r):
    # An acid line from one key: the pitch steps without a break while a second pattern accents the filter.
    p.osc(1, analog, r.pick([sawPos, squarePos, 85])).pitch(1, r.pick([-12, 0, -12]))
    cycle = r.pick([bar1, bar1, bars2])
    p.lfoSync(1, steps, cycle, 1, 0).stepPattern(1, step_notes(r.pick(arps_within(p, (1,))))).mod(lfo1, "osc1Pitch", 50)
    accents = [r.pick([-100, -100, -40, 0, 50, 100]) for _ in range(16)]
    p.lfoSync(2, steps, cycle, 1, r.u(30, 70, 10)).stepPattern(2, accents).mod(lfo2, "filter1Cutoff", r.u(10, 22, 2)).mod(lfo2, "filter1Resonance", r.u(4, 10))
    p.filter(1, ladder24, r.log(250, 800), r.u(62, 86, 3), r.u(10, 50, 10), 30)
    p.amp(2, 300, 100, r.u(60, 140, 20)).mono(0, True)
    if r.chance(0.7): dirt(p, r, (hard, tube, fuzz), 20, 55)
    if r.chance(0.6): echo(p, r, r.u(10, 22, 2), (sixteenthD, eighthD))
    p.level(-9)
    knobs(p, r, k_cut(28), k_res(15), k_grit(45, hard), ("Accent", "filter1Drive", 55))


@kind("Arp", 10, "Aurora Corona Nimbus Zodiac Meridian Zenith")
def arp_pad(p, r):
    # Soft notes that run into each other in a hall: an arpeggio you can leave under a scene.
    t = r.pick([analog, pulse, additive, vowel, fm])
    p.osc(1, t, r.pick([sawPos, trianglePos, 80]) if t == analog else r.u(10, 50, 5)).unison(1, r.pick([2, 3]), r.u(6, 14, 2), 100)
    p.osc(2, r.pick([analog, additive]), r.pick([sinePos, trianglePos]), r.u(50, 70, 5)).pitch(2, r.pick([12, 0]))
    arp_motion(p, r, (1, 2), ("16", "8", "8"), [[0, -8, -18, -30], [0, -10, -25, -45], [-30, 0, -10, -25]], False)
    p.filter(1, r.pick([ladder12, svfLow, ladder24]), r.log(900, 4000), r.u(10, 30, 5))
    p.amp(r.pick([20, 80, 200]), 300, 100, r.u(600, 1800, 100))
    if r.chance(0.6): p.chorus(r.u(25, 45, 5), r.u(0.2, 0.8, 0.1), 50)
    echo(p, r, r.u(16, 30, 2), (eighthD, quarter, quarterT))
    hall(p, r, r.u(30, 50, 5))
    p.level(-9)
    knobs(p, r, k_cut(25), k_detune(1, 25), k_space(35), k_echo(30))


@kind("Arp", 10, "Ricochet Scatter Volley Salvo Burst Cascade Spray")
def arp_fast(p, r):
    # Thirty-second notes: the arpeggio as a blur, between a chord and a trill.
    p.osc(1, *wave(r, [analog, pulse, sync, metal], [sawPos, squarePos]))
    if r.chance(0.5): p.osc(2, *wave(r, [analog, pulse], [sawPos, squarePos]), r.u(45, 65, 5)).pitch(2, r.pick([12, -12]), r.u(0, 8))
    arp_motion(p, r, (1, 2) if p.has("osc2On") else (1,), ("32",), [[0, 0, -40, -100], [0, -30, -70, -100], [0, 0, 0, -60]])
    p.filter(1, r.pick([ladder24, ladder12, svfLow, svfBand]), r.log(700, 3500), r.u(15, 50, 5))
    p.amp(2, 300, 100, r.u(100, 300, 20))
    if r.chance(0.5): dirt(p, r, (tube, hard, crush), 15, 40)
    echo(p, r, r.u(12, 26, 2), (eighthD, sixteenthD, eighth))
    if r.chance(0.5): hall(p, r, r.u(12, 30))
    p.level(-9)
    knobs(p, r, k_cut(25), k_res(30), k_space(35), k_echo(30))


# =============================================================================================
# Drum: kicks, snares, claps, hats, toms and cymbals from the oscillators. C3 is each drum's own pitch.
# =============================================================================================

@kind("Drum", 12, "Kick Kick Kick Kick Stomp Boot Thump")
def drum_kick(p, r):
    # A sine an octave or more down that starts high and falls in a few hundredths of a second, with a tick of noise at the front.
    p.osc(1, r.pick([analog, analog, sub]), r.pick([sinePos, sinePos, 8, 15])).pitch(1, r.pick([-12, -14, -16, -17, -19]))
    p.env(2, 0, r.u(20, 75, 5), 0, 40, r.u(80, 94, 2)).mod(env2, "osc1Pitch", semis(r.pick([12, 17, 24, 24, 31, 36])))
    p.noise(1, r.pick([0, 0, 3]), r.u(-20, 60, 20))
    p.env(3, 0, r.u(3, 14), 0, 10, 90).mod(env3, "noiseLevel", r.u(15, 50, 5))
    p.filter(1, r.pick([ladder24, svfLow]), r.log(1500, 9000), 5, r.u(0, 40, 10))
    p.amp(0, r.u(220, 700, 20), 0, r.u(60, 200, 20), r.u(50, 75, 5))
    if r.chance(0.6): dirt(p, r, (tube, soft, hard), 15, 40, (2500, 9000))
    p.level(-9)
    knobs(p, r, ("Punch", "osc1Pitch", semis(12)), ("Click", "noiseLevel", 35), k_grit(50, tube, 5000), k_bite(60))


@kind("Drum", 10, "Kick Kick Kick Stomp Hoof Ram Pile")
def drum_kick_hard(p, r):
    # The kick driven hard, with the sub oscillator for its body and a longer fall: hard techno and industrial.
    p.osc(1, r.pick([analog, sub, fm]), r.pick([sinePos, 10, 20, 30])).pitch(1, r.pick([-12, -12, -17, -19]))
    p.subOsc(r.u(30, 60, 10), 0, r.pick([0, 1]))
    p.env(2, 0, r.u(30, 110, 10), 0, 60, r.u(78, 92, 2)).mod(env2, "osc1Pitch", semis(r.pick([17, 24, 31, 36])))
    p.env(3, 0, r.u(60, 220, 20), 0, 100, 80).mod(env3, "osc1Position", r.u(8, 30))
    p.filter(1, ladder24, r.log(700, 5000), r.u(5, 30, 5), r.u(20, 70, 10))
    p.amp(0, r.u(350, 1100, 50), 0, r.u(80, 300, 20), r.u(45, 70, 5))
    p.drive(r.pick([hard, fuzz, tube, foldDrive]), r.u(30, 65, 5), r.log(1800, 7000))
    if r.chance(0.4): p.reverb(r.u(8, 22, 2), r.u(40, 80, 10), r.u(0.8, 2.5, 0.1), r.u(60, 85, 5))
    p.level(-9)
    knobs(p, r, ("Punch", "osc1Pitch", semis(12)), ("Crunch", "fxDriveAmount", 30), k_sub(35), ("Tone", "filter1Cutoff", -30))


@kind("Drum", 8, "Kick Kick Knock Thud Punt Stamp")
def drum_kick_fm(p, r):
    # A kick from the FM table: the index falls with the pitch, so the front knocks.
    p.osc(1, fm, r.u(5, 30, 5)).pitch(1, r.pick([-12, -14, -17, -19]))
    p.env(2, 0, r.u(20, 70, 5), 0, 40, r.u(82, 94, 2)).mod(env2, "osc1Pitch", semis(r.pick([12, 19, 24, 31]))).mod(env2, "osc1Position", r.u(20, 55, 5))
    p.filter(1, r.pick([ladder24, svfLow]), r.log(900, 6000), 5, r.u(0, 40, 10))
    p.amp(0, r.u(200, 600, 20), 0, r.u(60, 200, 20), r.u(50, 75, 5))
    if r.chance(0.6): dirt(p, r, (tube, hard, soft), 15, 45, (2500, 9000))
    p.level(-9)
    knobs(p, r, ("Punch", "osc1Pitch", semis(12)), ("Knock", "osc1Position", 30), k_grit(50, tube, 5000), ("Tone", "filter1Cutoff", -25))


def snare_layers(p, r, tone_pitch, tone_decay, noise_decay, noise_type):
    # The body is oscillator 2 through filter 2; the wires are noise through filter 1 (noise follows oscillator 1's route).
    p.set("osc1On", 0).parallel()
    p.osc(2, analog, r.pick([sinePos, trianglePos, 20]), 1).pitch(2, tone_pitch).route(2, 100)
    p.env(2, 0, tone_decay, 0, 60, 85).mod(env2, "osc2Volume", r.u(60, 85, 5)).mod(env2, "osc2Pitch", semis(r.pick([3, 5, 7, 12])))
    p.noise(1, noise_type, r.u(-20, 60, 20))
    p.env(3, 0, noise_decay, 0, 100, r.u(60, 85, 5)).mod(env3, "noiseLevel", r.u(65, 95, 5))


@kind("Drum", 11, "Snare Snare Snare Crack Snap Rap")
def drum_snare(p, r):
    # A short tone for the shell and filtered noise for the wires, each with its own envelope.
    snare_layers(p, r, r.pick([3, 5, 7, 9, 12]), r.u(50, 160, 10), r.u(90, 320, 10), r.pick([0, 0, 1]))
    p.filter(1, r.pick([svfHigh, svfBand, ladderHigh]), r.log(900, 4500), r.u(10, 40, 5))
    p.filter(2, r.pick([ladder12, svfLow]), r.log(1500, 6000), 5)
    p.amp(0, r.u(250, 500, 50), 0, r.u(100, 250, 50), 40)
    if r.chance(0.6): dirt(p, r, (tube, hard, soft), 15, 40)
    if r.chance(0.6): room(p, r, r.u(8, 25))
    p.level(-9)
    knobs(p, r, ("Wires", "noiseLevel", -45), ("Tone", "filter1Cutoff", 18), k_grit(55, hard), k_space(40, 50, 1.2))


@kind("Drum", 10, "Snare Snare Lash Whip Crack Gunshot")
def drum_snare_trash(p, r):
    # A snare with a comb in its noise and a clipper after it: the snare of an industrial record.
    snare_layers(p, r, r.pick([0, 3, 5, 7]), r.u(60, 200, 10), r.u(150, 500, 10), r.pick([0, 2, 3, 0]))
    p.filter(1, comb, r.log(150, 900), r.u(40, 80, 5))
    p.filter(2, r.pick([ladder12, svfLow, svfBand]), r.log(800, 5000), r.u(5, 30, 5))
    p.amp(0, r.u(300, 800, 50), 0, r.u(150, 400, 50), 40)
    p.drive(r.pick([hard, fuzz, crush, tube]), r.u(25, 60, 5), r.log(3000, 9000))
    p.reverb(r.u(15, 35, 5), r.u(40, 80, 10), r.u(0.6, 2.2, 0.2), r.u(40, 70, 10))
    p.level(-9)
    knobs(p, r, ("Wires", "noiseLevel", -45), ("Ring", "filter1Resonance", 15), ("Crunch", "fxDriveAmount", 30), ("Space", "fxReverbMix", 35))


@kind("Drum", 10, "Clap Clap Clap Slap Smack")
def drum_clap(p, r):
    # Band-passed noise let through in three quick bursts and then a tail: hands, more or less together.
    p.set("osc1On", 0).noise(r.u(75, 100, 5), r.pick([0, 0, 1]), r.u(-20, 40, 20))
    p.filter(1, r.pick([svfBand, ladderBand]), r.log(900, 2400), r.u(25, 55, 5))
    if r.chance(0.5): p.filter(2, svfHigh, r.log(400, 1200), 10)
    gap = r.pick([-70, -85, -100])
    bursts = r.pick([[0, gap, 0, gap, 0, gap], [0, gap, gap, 0, gap, 0, gap], [0, gap, 0, gap, gap, 0, gap, 0, gap]])
    p.lfo(1, steps, r.u(8, 16, 0.5), 2, r.u(40, 75, 5)).stepPattern(1, bursts + [0] * (16 - len(bursts))).mod(lfo1, "ampLevel", r.u(55, 80, 5))
    p.amp(0, r.u(180, 600, 20), 0, r.u(120, 350, 10), r.u(40, 70, 10))
    if r.chance(0.5): dirt(p, r, (tube, hard), 15, 40)
    p.reverb(r.u(12, 35, 1), r.u(35, 75, 5), r.u(0.6, 2.0, 0.2), r.u(40, 70, 10))
    p.level(-9)
    knobs(p, r, ("Tone", "filter1Cutoff", 15), ("Spread", "lfo1Rate", -12), k_grit(55, hard), ("Space", "fxReverbMix", 35))


@kind("Drum", 10, "Hat Hat Hat Tick Tsk Hiss")
def drum_hat_closed(p, r):
    # High noise and a few metal partials, over in a few hundredths of a second.
    p.osc(1, metal, r.u(60, 100, 5), r.u(35, 65, 5)).pitch(1, r.pick([12, 19, 24, 24])).unison(1, r.pick([2, 3, 4]), r.u(20, 60, 5), r.u(30, 80, 10))
    p.noise(r.u(60, 90, 5), r.pick([0, 0, 3]), r.u(20, 80, 20))
    p.filter(1, r.pick([svfHigh, ladderHigh, svfBand]), r.log(5000, 11000), r.u(10, 40, 5))
    p.amp(0, r.u(30, 110, 5), 0, r.u(25, 80, 5), r.u(55, 80, 5))
    p.mod(velocity, "filter1Cutoff", r.u(3, 9))
    if r.chance(0.3): dirt(p, r, (hard, crush), 15, 35)
    p.level(-9)
    knobs(p, r, ("Tone", "filter1Cutoff", -12), ("Metal", "osc1Volume", 30), ("Hiss", "noiseLevel", -45), k_space(35, 40, 0.8))


@kind("Drum", 8, "Hat Hat Sizzle Wash Shimmer")
def drum_hat_open(p, r):
    # The open hat: the same metal and noise left to ring, with a band to make it sizzle.
    p.osc(1, metal, r.u(60, 100, 5), r.u(40, 70, 5)).pitch(1, r.pick([12, 19, 24])).unison(1, r.pick([3, 4, 6]), r.u(25, 70, 5), r.u(50, 100, 10))
    p.noise(r.u(55, 85, 5), r.pick([0, 0, 3]), r.u(20, 80, 20))
    p.filter(1, r.pick([svfHigh, svfBand, ladderHigh]), r.log(4000, 9000), r.u(15, 50, 5))
    p.amp(0, r.u(250, 800, 50), 0, r.u(80, 300, 20), r.u(40, 70, 10))
    if r.chance(0.4): room(p, r, r.u(8, 20, 4))
    p.level(-9)
    knobs(p, r, ("Tone", "filter1Cutoff", -12), ("Metal", "osc1Volume", 30), ("Sizzle", "filter1Resonance", 35), k_space(35, 40, 0.8))


@kind("Drum", 10, "Tom Tom Tom Drum Tub Barrel")
def drum_tom(p, r):
    # A sine that falls a few semitones as it rings, with a little skin noise: toms, tuned from the keyboard.
    p.osc(1, analog, r.pick([sinePos, sinePos, 10, trianglePos])).pitch(1, r.pick([-5, 0, 0, 4, 7]))
    p.env(2, 0, r.u(50, 180, 10), 0, 80, r.u(70, 88, 6)).mod(env2, "osc1Pitch", semis(r.pick([3, 5, 7, 9, 12])))
    p.noise(1, r.pick([0, 1]), r.u(-40, 20, 20))
    p.env(3, 0, r.u(15, 70, 5), 0, 40, 85).mod(env3, "noiseLevel", r.u(25, 55, 5))
    p.filter(1, r.pick([ladder24, svfLow]), r.log(900, 5000), r.u(5, 25, 5), r.u(0, 40, 10))
    p.amp(0, r.u(250, 800, 50), 0, r.u(100, 300, 50), r.u(50, 75, 5))
    if r.chance(0.5): dirt(p, r, (tube, hard), 15, 40)
    if r.chance(0.6): room(p, r, r.u(8, 25))
    p.level(-9)
    knobs(p, r, ("Bend", "osc1Pitch", semis(7)), ("Skin", "noiseLevel", 35), k_grit(55, hard), k_space(40, 55, 1.4))


@kind("Drum", 8, "Cymbal Crash Ride Splash Gong")
def drum_cymbal(p, r):
    # Many detuned metal partials and high noise with a long fall: crashes and rides.
    p.osc(1, metal, r.u(70, 100, 5)).pitch(1, r.pick([12, 17, 19, 24])).unison(1, r.pick([4, 6, 8]), r.u(30, 80, 5), 100)
    p.osc(2, metal, r.u(40, 90, 10), r.u(50, 75, 5)).pitch(2, r.pick([7, 13, 18, 23]), r.u(-30, 30, 10)).unison(2, r.pick([2, 3, 4]), r.u(20, 60, 5), 100)
    p.noise(r.u(40, 70, 5), 0, r.u(20, 80, 20))
    p.filter(1, r.pick([svfHigh, ladderHigh]), r.log(2500, 7000), r.u(5, 30, 5))
    p.env(2, 0, r.u(60, 400, 20), 0, 200, 80).mod(env2, "noiseLevel", r.u(10, 30, 5)).mod(env2, "filter1Cutoff", -r.u(3, 10))
    p.amp(0, r.u(900, 3500, 100), 0, r.u(400, 1500, 100), r.u(45, 70, 5))
    if r.chance(0.5): hall(p, r, r.u(10, 25, 5))
    p.level(-9)
    knobs(p, r, ("Tone", "filter1Cutoff", -12), ("Bright", "osc1Position", -40), ("Wash", "noiseLevel", 30), k_grit(55, hard))


# =============================================================================================
# FX
# =============================================================================================

@kind("FX", 8, "Laser Ray Beam Bolt Lance Dart Shot")
def fx_laser(p, r):
    # A synced wave whose sync and pitch both fall in an instant: the ray gun.
    p.osc(1, *wave(r, [analog, pulse], [sawPos, squarePos])).warp(1, wSync, r.u(40, 90, 5))
    p.env(2, 0, r.u(80, 600, 20), 0, 200, r.u(50, 90, 10)).mod(env2, "osc1Pitch", semis(r.pick([12, 19, 24, 36, -12, -24]))).mod(env2, "osc1WarpAmount", -r.u(30, 70, 10))
    p.filter(1, r.pick([svfLow, ladder12, svfBand, svfHigh]), r.log(900, 9000), r.u(10, 45, 5))
    p.amp(0, r.u(150, 900, 50), 0, r.u(100, 400, 50), r.u(50, 80, 10))
    if r.chance(0.5): dirt(p, r, (hard, crush, tube), 15, 45)
    echo(p, r, r.u(12, 30, 2), (sixteenth, sixteenthD, eighth, eighthT))
    if r.chance(0.5): hall(p, r, r.u(12, 30))
    p.level(-9)
    knobs(p, r, ("Sync", "osc1WarpAmount", -35), ("Sweep", "osc1Pitch", semis(12)), k_echo(35, sixteenth), k_space(35), k_grit(55, hard), k_cut(20))


@kind("FX", 8, "Alarm Siren Alert Warning Klaxon Lockdown Evacuation")
def fx_alarm(p, r):
    # Two notes, back and forth: pitch moves in semitones, which is what an alarm wants.
    p.osc(1, *wave(r, [analog, pulse, sync], [squarePos, sawPos])).unison(1, r.pick([1, 2, 3]), r.u(6, 16, 2), r.u(30, 70, 10))
    p.lfo(1, r.pick([square, square, sawUp, sawDown, triangle]), r.log(0.6, 9), 1).mod(lfo1, "osc1Pitch", semis(r.pick([1, 2, 3, 4, 5, 6, 7])))
    p.filter(1, r.pick([ladder12, svfLow, svfBand, ladderBand]), r.log(1200, 6000), r.u(10, 50, 5), r.u(0, 30, 10))
    p.amp(5, 300, 100, r.u(80, 300, 20))
    if r.chance(0.6): dirt(p, r, (hard, tube, crush), 15, 50)
    if r.chance(0.5): echo(p, r, r.u(10, 22, 2))
    hall(p, r, r.u(12, 35))
    p.level(-9)
    knobs(p, r, ("Rate", "lfo1Rate", 20), k_cut(22), k_space(35), k_grit(55, hard), k_echo(30), k_res(30))


@kind("FX", 8, "Computer Mainframe Terminal Console Processor Cortex Logic")
def fx_computer(p, r):
    # A random note several times a second: the computer of old films, thinking.
    p.osc(1, *wave(r, [analog, pulse, fm], [sinePos, squarePos, trianglePos]))
    p.lfo(1, randomShape, r.log(4, 20), 0).mod(lfo1, "osc1Pitch", r.u(15, 45, 5))
    if r.chance(0.5): p.lfo(2, randomShape, r.log(2, 12), 0).mod(lfo2, "ampPan", r.u(20, 45, 5))
    p.filter(1, r.pick([ladder12, svfLow, svfBand]), r.log(1500, 9000), r.u(5, 40, 5))
    p.amp(5, 300, 100, r.u(60, 300, 20))
    echo(p, r, r.u(15, 32, 1), (sixteenth, sixteenthD, eighth, eighthT, sixteenthT))
    if r.chance(0.6): hall(p, r, r.u(12, 30))
    p.level(-9)
    knobs(p, r, ("Speed", "lfo1Rate", 20), ("Range", "osc1Pitch", 20), k_echo(35), k_space(35), k_grit(55, crush), k_cut(20))


@kind("FX", 8, "Sonar Ping Echo Sounding Beacon Transponder Pulse")
def fx_sonar(p, r):
    # One short sine into long echoes and a big room: the ping.
    p.osc(1, analog, r.pick([sinePos, sinePos, trianglePos])).pitch(1, r.pick([0, 7, 12, 12, 19]))
    if r.chance(0.5): p.osc(2, analog, sinePos, r.u(40, 60, 5)).pitch(2, p.get("osc1Pitch") + r.pick([-12, -7, 5]), r.u(0, 10))
    p.env(2, 0, r.u(60, 300, 20), 0, 100, 70).mod(env2, "osc1Fine", r.pick([-1, 1]) * r.u(5, 30, 5))
    p.filter(1, r.pick([svfBand, ladder12]), r.log(1200, 5000), r.u(10, 50, 5), 0, 50)
    p.amp(r.pick([0, 2, 8]), r.u(120, 700, 20), 0, r.u(150, 600, 50), r.u(50, 75, 5))
    p.delay(r.pick([quarter, quarterD, half, eighthD]), r.u(50, 75, 5), r.u(30, 50, 5), r.log(2000, 7000))
    hall(p, r, r.u(30, 50, 5))
    p.level(-9)
    knobs(p, r, ("Echo", "fxDelayMix", 35), ("Tone", "filter1Cutoff", 20), k_space(35), k_res(30), k_grit(55))


@kind("FX", 8, "Whoosh Sweep Flyby Pass Swoop Rush Wake")
def fx_whoosh(p, r):
    # Noise through a band that rises and falls as it crosses from one side to the other: something going past.
    p.set("osc1On", 0).noise(r.u(75, 100, 5), r.pick([0, 1, 1]), r.u(-40, 40, 20))
    length = r.u(400, 2500, 100)
    p.filter(1, r.pick([svfBand, ladderBand, ladder24]), r.log(200, 900), r.u(30, 75, 5))
    p.env(2, length, length, 0, length, r.u(-30, 40, 10)).mod(env2, "filter1Cutoff", r.u(20, 45, 5))
    p.lfo(1, r.pick([sawUp, sawDown]), round(1000.0 / (2 * length), 2), 2).mod(lfo1, "ampPan", r.u(25, 50, 5))
    p.amp(length, length, 0, length, r.u(-20, 40, 10))
    if r.chance(0.5): p.phaser(r.u(30, 55, 5), r.log(0.2, 2), r.u(40, 70, 10))
    hall(p, r, r.u(15, 40, 5))
    p.level(-9)
    knobs(p, r, ("Height", "filter1Cutoff", 15), k_res(20), k_space(35), k_grit(55), ("Colour", "noiseColour", 40), k_phase(50))


@kind("FX", 8, "Shutdown Blackout Outage Brownout Failure Stall Seizure")
def fx_power_down(p, r):
    # A machine losing power: the sync falls, the filter closes and the flutter in its level slows to nothing.
    p.osc(1, r.pick([analog, pulse, growl]), r.pick([sawPos, 40, 30])).warp(1, wSync, r.u(40, 80, 5)).unison(1, r.pick([1, 2, 3]), r.u(8, 18, 2), 70)
    length = r.u(1200, 5000, 100)
    p.env(2, 0, length, 0, length, r.u(0, 60, 10)).mod(env2, "osc1WarpAmount", r.u(30, 70, 10)).mod(env2, "filter1Cutoff", r.u(20, 40, 5)).mod(env2, "lfo1Rate", r.u(20, 45, 5))
    p.lfo(1, r.pick([sine, triangle]), r.log(0.5, 3), 1).mod(lfo1, "ampLevel", r.u(6, 18, 2))
    p.filter(1, r.pick([ladder24, svfLow]), r.log(120, 500), r.u(15, 50, 5))
    p.amp(0, length, 0, r.u(500, 1500, 100), r.u(-20, 40, 10))
    if r.chance(0.5): dirt(p, r, (tube, hard), 15, 40)
    hall(p, r, r.u(12, 35))
    p.level(-9)
    knobs(p, r, ("Sync", "osc1WarpAmount", 25), k_cut(15), k_space(35), k_grit(55, hard))


@kind("FX", 8, "Telemetry Uplink Downlink Handshake Beacon Burst Squawk")
def fx_telemetry(p, r):
    # Two fixed tones keyed on and off in a pattern: signalling.
    p.osc(1, analog, r.pick([sinePos, sinePos, trianglePos])).osc(2, analog, sinePos, r.u(60, 80, 5)).pitch(2, r.pick([3, 4, 5, 7, 8, 11]), r.u(-20, 20, 5))
    pattern = [r.pick([0, 0, -100]) for _ in range(16)]
    pattern[0] = 0
    p.lfoSync(1, steps, r.pick([bar1, half, bars2, quarter]), 1, r.u(80, 92, 4)).stepPattern(1, pattern).mod(lfo1, "ampLevel", r.u(70, 85, 5))
    other = [r.pick([100, -100]) for _ in range(16)]
    p.lfoSync(2, steps, r.pick([bar1, half, bars2]), 1, r.u(70, 90, 10)).stepPattern(2, other).mod(lfo2, "osc2Volume", r.u(25, 45, 5))
    p.filter(1, r.pick([svfBand, ladder12]), r.log(1200, 5000), r.u(10, 45, 5), 0, 50)
    p.amp(3, 300, 100, r.u(60, 250, 10))
    if r.chance(0.5): p.drive(crush, r.u(15, 45, 5), r.log(3000, 9000))
    echo(p, r, r.u(10, 26, 2))
    p.level(-9)
    knobs(p, r, k_cut(18), ("Second", "osc2Volume", -50), k_echo(35), k_space(35), k_grit(55, crush), k_res(30))


@kind("FX", 8, "Arc Spark Short Discharge Flashover Corona Surge")
def fx_spark(p, r):
    # Crackle through a ringing band that jumps at random: electricity getting out.
    p.set("osc1On", 0).noise(r.u(80, 100, 5), 2, r.u(0, 80, 20))
    p.filter(1, r.pick([svfBand, ladderBand]), r.log(900, 5000), r.u(60, 92, 4))
    p.lfo(1, randomShape, r.log(5, 30), 0).mod(lfo1, "filter1Cutoff", r.u(8, 22, 2))
    p.lfo(2, r.pick([randomShape, drift]), r.log(1, 9), 0).mod(lfo2, "ampLevel", r.u(8, 25))
    p.amp(r.pick([0, 5, 30]), 400, r.u(70, 100, 10), r.u(100, 600, 50))
    if r.chance(0.5): dirt(p, r, (hard, crush, fuzz), 15, 45)
    hall(p, r, r.u(10, 30, 5))
    p.level(-9)
    knobs(p, r, k_cut(18), ("Jump", "lfo1Rate", 20), k_space(35), k_grit(55, hard), k_res(10))


@kind("FX", 8, "Saucer Probe Visitor Orbiter Craft Drone Satellite")
def fx_wobble(p, r):
    # A sine shaken fast by an LFO and passed through a phaser: the flying saucer.
    p.osc(1, analog, r.pick([sinePos, trianglePos, 15])).osc(2, analog, sinePos, r.u(50, 75, 5)).pitch(2, r.pick([7, 12, -12, 5]), r.u(0, 12))
    p.lfo(1, r.pick([sine, triangle]), r.log(5, 30), 0).mod(lfo1, "osc1Pitch", semis(r.pick([1, 2, 3, 5, 7]))).mod(lfo1, "osc2Fine", -r.u(20, 50, 5))
    p.lfo(2, r.pick([sine, drift]), r.log(0.08, 1), 0).mod(lfo2, "lfo1Rate", r.u(8, 20, 2))
    p.filter(1, r.pick([ladder12, svfLow]), r.log(2000, 9000), 10)
    p.amp(r.u(20, 600, 20), 400, 100, r.u(300, 1500, 100))
    p.phaser(r.u(35, 60, 5), r.log(0.1, 1.5), r.u(45, 75, 5))
    hall(p, r, r.u(20, 45, 5))
    p.level(-9)
    knobs(p, r, ("Wobble", "lfo1Rate", 20), ("Second", "osc2Volume", -50), k_space(35), ("Phase", "fxPhaserMix", 35), k_echo(30), k_grit(55))


# =============================================================================================
# Glitch
# =============================================================================================

@kind("Glitch", 8, "Fault Error Exception Overflow Underrun Timeout Abort")
def glitch_random_notes(p, r):
    # A new note at random every sixteenth or faster, crushed: a sequencer with a fault.
    p.osc(1, r.pick([analog, fm, sync]), r.pick([sawPos, trianglePos, 40, 20])).warp(1, wCrush, r.u(40, 70, 5))
    p.lfoSync(1, randomShape, r.pick([sixteenth, sixteenth, thirtySecond, sixteenthT, eighth]), 1).mod(lfo1, "osc1Pitch", r.u(15, 50, 5))
    p.lfoSync(2, randomShape, r.pick([sixteenth, eighth, thirtySecond, eighthT]), 1).mod(lfo2, "filter1Cutoff", r.u(8, 22, 2)).mod(lfo2, "osc1Position", r.u(10, 40, 10))
    p.filter(1, r.pick([svfBand, ladder12, svfLow, ladderBand]), r.log(900, 5000), r.u(20, 60, 5))
    p.amp(2, 300, 100, r.u(40, 200, 20))
    if r.chance(0.6): p.drive(r.pick([crush, hard]), r.u(15, 45, 5), r.log(3000, 10000))
    echo(p, r, r.u(10, 26, 2), (sixteenth, sixteenthT, thirtySecond, eighthT))
    p.level(-9)
    knobs(p, r, ("Range", "osc1Pitch", 20), k_cut(20), ("Steps", "osc1WarpAmount", 30), k_echo(35, sixteenth), k_res(30), k_space(35))


@kind("Glitch", 8, "Stutter Stammer Hiccup Jitter Skip Repeat Hitch")
def glitch_stutter(p, r):
    # A tone cut into thirty-second notes by an uneven pattern while another jumps its wave: a buffer stuck.
    t = r.pick([sync, fm, growl, vowel, metal, pulse])
    p.osc(1, t, 50).unison(1, r.pick([1, 2]), r.u(6, 14, 2), 60)
    cut = [r.pick([0, 0, -100, -100, -40]) for _ in range(8)]
    cut[0] = 0
    cut = cut + (cut if r.chance(0.5) else [r.pick([0, -100, -100]) for _ in range(8)])
    p.lfoSync(1, steps, r.pick([half, half, quarter, bar1]), 1, r.u(70, 88, 6)).stepPattern(1, cut).mod(lfo1, "ampLevel", r.u(72, 85))
    jumps = [r.pick([-100, -50, 0, 50, 100]) for _ in range(16)]
    p.lfoSync(2, steps, r.pick([half, bar1, quarter]), 1, 0).stepPattern(2, jumps).mod(lfo2, "osc1Position", r.u(30, 48, 3)).mod(lfo2, "filter1Cutoff", r.u(5, 16))
    p.filter(1, r.pick([svfBand, ladder12, svfLow, ladder24]), r.log(900, 5000), r.u(15, 55, 5))
    p.amp(2, 300, 100, r.u(40, 200, 20))
    if r.chance(0.6): p.drive(r.pick([crush, hard, tube]), r.u(15, 45, 5), r.log(3000, 10000))
    if r.chance(0.5): echo(p, r, r.u(10, 22, 2), (sixteenth, thirtySecond, sixteenthT))
    p.level(-9)
    knobs(p, r, k_cut(20), ("Cut", "ampLevel", 14), ("Jump", "osc1Position", 30), k_grit(55, crush), k_res(30), k_space(35))


@kind("Glitch", 8, "Bitrot Decay Corruption Artefact Residue Remnant Dropout")
def glitch_bitrot(p, r):
    # The wave's resolution thrown about at random under pitched digital noise: data going bad.
    p.osc(1, r.pick([analog, additive, vowel, fm]), r.u(20, 100, 10)).warp(1, wCrush, r.u(30, 60, 5))
    p.noise(r.u(25, 55, 5), 3, r.u(-40, 40, 20))
    p.lfo(1, randomShape, r.log(3, 25), 0).mod(lfo1, "osc1WarpAmount", r.u(20, 40, 5)).mod(lfo1, "noiseColour", r.u(15, 40, 5))
    p.lfo(2, r.pick([drift, randomShape]), r.log(0.3, 5), 0).mod(lfo2, "filter1Cutoff", r.u(6, 18, 2))
    p.filter(1, r.pick([svfBand, ladder12, svfLow, svfHigh]), r.log(700, 5000), r.u(15, 50, 5))
    p.amp(r.pick([2, 20, 200]), 300, 100, r.u(100, 800, 50))
    p.drive(crush, r.u(20, 50, 5), r.log(3000, 10000))
    if r.chance(0.5): hall(p, r, r.u(10, 30, 5))
    p.level(-9)
    knobs(p, r, k_cut(20), ("Rot", "osc1WarpAmount", 30), ("Speed", "lfo1Rate", 20), ("Crush", "fxDriveAmount", 35), k_noise(-40, "Hash"), k_space(35))


@kind("Glitch", 8, "Modem Handshake Carrier Baud Parity Protocol Negotiation")
def glitch_modem(p, r):
    # FM whose modulator jumps between ratios on a pattern: a modem making a connection.
    p.osc(1, analog, sinePos, r.pick([1, 15])).osc(2, analog, r.pick([sinePos, trianglePos])).warp(2, wFm, r.u(25, 70, 5))
    ratios = [r.pick([-100, -50, -25, 0, 25, 50, 100]) for _ in range(16)]
    p.lfoSync(1, steps, r.pick([bar1, half, quarter, bars2]), 1, 0).stepPattern(1, ratios).mod(lfo1, "osc1Pitch", r.u(20, 50, 5))
    p.lfoSync(2, randomShape, r.pick([sixteenth, eighth, thirtySecond]), 1).mod(lfo2, "osc2WarpAmount", r.u(10, 30, 5)).mod(lfo2, "filter1Cutoff", r.u(4, 14, 2))
    p.filter(1, r.pick([svfBand, ladderBand, ladder12]), r.log(900, 3500), r.u(25, 60, 5))
    p.amp(2, 300, 100, r.u(40, 200, 20))
    if r.chance(0.6): p.drive(r.pick([crush, hard]), r.u(15, 45, 5), r.log(3000, 9000))
    if r.chance(0.5): echo(p, r, r.u(10, 22, 2), (sixteenth, thirtySecond, sixteenthT))
    p.level(-9)
    knobs(p, r, ("Index", "osc2WarpAmount", 25), k_cut(18), ("Ratio", "osc1Pitch", 20), k_grit(55, crush), k_res(30), k_space(35))


@kind("Glitch", 8, "Skip Scratch Loop Splice Shard Fragment Slice")
def glitch_skip(p, r):
    # A blip into a very short delay with a lot of feedback: a disc skipping on one grain.
    p.osc(1, *wave(r, [analog, pulse, sync, fm, metal], [sawPos, squarePos, trianglePos]))
    if r.chance(0.5): p.noise(r.u(20, 50, 5), r.pick([3, 0, 2]))
    p.env(2, 0, r.u(20, 90, 10), 0, 40, 80).mod(env2, "osc1Position", r.pick([-1, 1]) * r.u(15, 40, 5)).mod(env2, "filter1Cutoff", r.u(8, 22, 2))
    p.filter(1, r.pick([svfBand, ladder12, svfLow, svfHigh]), r.log(700, 5000), r.u(15, 55, 5))
    p.amp(0, r.u(40, 160, 10), 0, r.u(30, 120, 10), r.u(50, 80, 10))
    p.delay(r.pick([sixtyFourth, thirtySecond, thirtySecond, sixteenthT, sixteenth]), r.u(70, 88, 2), r.u(35, 50, 5), r.log(2500, 12000), r.chance(0.5))
    if r.chance(0.5): p.drive(r.pick([crush, hard]), r.u(15, 40, 5), r.log(3000, 10000))
    p.level(-9)
    knobs(p, r, ("Repeats", "fxDelayFeedback", 10), k_cut(20), ("Grain", "osc1Position", 30), k_grit(55, crush), ("Skip", "fxDelayMix", 30), k_space(35))


@kind("Glitch", 8, "Scan Sweep Raster Trace Readhead Cursor Pointer")
def glitch_scan(p, r):
    # A pattern runs the sync table up and down in thirty-seconds while the filter is thrown about: a scan head hunting.
    p.osc(1, sync, 50).unison(1, r.pick([1, 2, 3]), r.u(6, 16, 2), 70)
    if r.chance(0.5): p.subOsc(r.u(25, 45, 5))
    ramp = r.pick([[-100, -75, -50, -25, 0, 25, 50, 75], [100, 60, 20, -20, -60, -100, -60, -20], [-100, 100, -60, 60, -20, 20, 0, 0]])
    p.lfoSync(1, steps, r.pick([half, quarter, bar1]), 1, r.u(0, 50, 10)).stepPattern(1, ramp + (ramp if r.chance(0.5) else ramp[::-1])).mod(lfo1, "osc1Position", r.u(35, 50, 5))
    p.lfoSync(2, randomShape, r.pick([sixteenth, eighth, thirtySecond, eighthT]), 1).mod(lfo2, "filter1Cutoff", r.u(8, 22, 2))
    p.filter(1, r.pick([svfBand, ladder24, ladder12, ladderBand]), r.log(700, 4000), r.u(25, 65, 5))
    p.amp(2, 300, 100, r.u(40, 200, 20))
    if r.chance(0.6): dirt(p, r, (hard, crush, tube), 15, 45)
    if r.chance(0.5): echo(p, r, r.u(10, 22, 2), (sixteenth, thirtySecond, sixteenthT))
    p.level(-9)
    knobs(p, r, k_cut(20), ("Scan", "osc1Position", 30), k_res(25), k_grit(55, crush), k_echo(30, sixteenth), k_space(35))


# =============================================================================================
# Cinematic
# =============================================================================================

@kind("Cinematic", 9, "Braam Leviathan Colossus Monolith Titan Juggernaut")
def cine_braam(p, r):
    # The trailer's low brass blast: saws and a growl in octaves, a ladder that tears open, a clipper and a long hall.
    p.osc(1, analog, sawPos).unison(1, r.pick([3, 4, 5, 6]), r.u(10, 24, 2), r.u(60, 100, 10))
    p.osc(2, r.pick([growl, sync, analog]), r.u(30, 70, 10)).pitch(2, r.pick([-12, 0, 7, 12]), r.u(4, 10)).unison(2, 2, r.u(8, 14, 2), 80)
    p.subOsc(r.u(35, 55, 5))
    p.filter(1, ladder24, r.log(160, 500), r.u(10, 35, 5), r.u(20, 60, 10))
    p.env(2, r.u(60, 280, 20), r.u(800, 2500, 100), r.u(50, 80, 10), 1500, r.u(30, 60, 10)).mod(env2, "filter1Cutoff", r.u(25, 42)).mod(env2, "osc2Position", r.u(10, 30, 5))
    p.amp(r.u(20, 140, 10), 600, r.u(85, 100, 5), r.u(1200, 3000, 100))
    p.drive(r.pick([hard, tube, fuzz]), r.u(30, 55, 5), r.log(2500, 7000))
    hall(p, r, r.u(30, 50, 5))
    p.level(-9)
    knobs(p, r, k_cut(25), ("Growl", "osc2Position", 30), k_space(35), ("Tear", "fxDriveAmount", 35), k_detune(1, 25), k_sub(35))


@kind("Cinematic", 8, "Hit Blow Impact Strike Collision Landfall Touchdown")
def cine_low_hit(p, r):
    # A drum the size of a room and a chord of low saws behind it, falling away for seconds.
    p.osc(1, r.pick([analog, sub]), r.pick([sinePos, 15])).subOsc(r.u(40, 60, 5))
    p.osc(2, analog, sawPos, r.u(50, 75, 5)).unison(2, r.pick([3, 4, 5]), r.u(12, 26, 2), 100).pitch(2, r.pick([0, 7, 12]))
    p.noise(1, 1, r.u(-60, 0, 20))
    p.env(2, 0, r.u(60, 250, 10), 0, 100, 88).mod(env2, "osc1Pitch", semis(r.pick([12, 19, 24]))).mod(env2, "noiseLevel", r.u(30, 60, 5))
    p.env(3, 0, r.u(300, 1500, 100), 0, 500, 70).mod(env3, "filter1Cutoff", r.u(20, 40, 5))
    p.filter(1, ladder24, r.log(150, 500), r.u(10, 30, 5), r.u(20, 60, 10))
    p.amp(0, r.u(2000, 5000, 500), 0, r.u(2000, 4000, 500), r.u(45, 65, 5))
    dirt(p, r, (hard, tube, fuzz), 20, 50, (2500, 7000))
    hall(p, r, r.u(35, 55, 5))
    p.level(-9)
    knobs(p, r, k_cut(22), ("Drum", "osc1Pitch", semis(7)), k_space(35), k_grit(40, hard), ("Chord", "osc2Volume", -50), k_sub(35))


@kind("Cinematic", 8, "Pulse Ostinato Pursuit Chase Countdown Deadline Fuse")
def cine_pulse(p, r):
    # Low strings in sixteenths that slowly brighten: the pulse under a chase.
    p.osc(1, analog, sawPos).unison(1, r.pick([3, 4, 5]), r.u(8, 18, 2), r.u(60, 100, 10))
    p.osc(2, analog, r.pick([sawPos, squarePos]), r.u(50, 70, 5)).pitch(2, r.pick([-12, 0, 12]), r.u(3, 8))
    shape = r.pick([[0, -30, -65, -100], [0, -20, -50, -85], [0, -45, -90, -100]])
    accents = r.pick([[0, -15, -8, -15], [0, 0, 0, 0], [0, -20, 0, -20], [0, -100, 0, -20]])
    p.lfoSync(1, steps, r.pick([quarter, quarter, half]), 1, r.u(96, 99)).stepPattern(1, [max(-100, s + a) for a in accents for s in shape])
    p.mod(lfo1, "ampLevel", r.u(70, 85, 5)).mod(lfo1, "filter1Cutoff", r.u(6, 16, 2))
    p.env(2, r.u(3000, 9000, 500), 1000, 100, 500, r.u(-30, 30, 10)).mod(env2, "filter1Cutoff", r.u(12, 28, 2))
    p.filter(1, r.pick([ladder24, ladder12]), r.log(250, 900), r.u(10, 40, 5), r.u(0, 40, 10))
    p.amp(2, 300, 100, r.u(150, 500, 50))
    if r.chance(0.5): dirt(p, r, (tube, hard), 15, 40)
    hall(p, r, r.u(15, 35, 5))
    p.level(-9)
    p.target = -12
    knobs(p, r, k_cut(22), ("Pulse", "ampLevel", 14), k_space(35), k_grit(55, hard), k_detune(1, 25), k_res(30))


@kind("Cinematic", 8, "Dread Suspicion Paranoia Vertigo Insomnia Delirium Fever")
def cine_tension(p, r):
    # High strings a semitone apart that slowly spread: the scene before something happens.
    p.osc(1, analog, sawPos).unison(1, r.pick([4, 6, 8]), r.u(8, 20, 2), 100, 0, r.u(30, 80, 10)).pitch(1, r.pick([12, 12, 19, 0]))
    p.osc(2, analog, sawPos, r.u(60, 80, 5)).pitch(2, p.get("osc1Pitch") + r.pick([1, 1, -1, 5])).unison(2, r.pick([3, 4]), r.u(8, 20, 2), 100, 0, r.u(30, 80, 10))
    p.env(2, r.u(3000, 9000, 500), 1000, 100, 1000, r.u(-30, 30, 10)).mod(env2, "osc1UnisonDetune", r.u(20, 45, 5)).mod(env2, "filter1Cutoff", r.u(8, 20, 2))
    p.lfo(1, r.pick([sine, triangle]), r.u(6, 11, 0.5), 0).mod(lfo1, "ampLevel", r.u(3, 12))
    p.filter(1, r.pick([ladder12, svfLow, svfBand]), r.log(1500, 5000), r.u(5, 30, 5))
    p.filter(2, svfHigh, r.log(250, 900), 10)
    p.amp(r.u(600, 2500, 100), 1000, 100, r.u(1500, 3500, 100))
    hall(p, r, r.u(30, 55, 5))
    p.level(-9)
    p.target = -12
    knobs(p, r, k_cut(20), ("Spread", "osc1UnisonDetune", 20), k_space(35), ("Semitone", "osc2Volume", -55), ("Shake", "ampLevel", 8), k_grit(55))


@kind("Cinematic", 8, "Swell Tide Bloom Surge Upwelling Flood Dawn")
def cine_swell(p, r):
    # A chord of fifths that takes seconds to arrive, the filter and the drive rising with it.
    p.osc(1, analog, sawPos).unison(1, r.pick([4, 5, 6]), r.u(10, 22, 2), 100)
    p.osc(2, r.pick([analog, pulse, additive]), r.u(30, 70, 10), r.u(55, 75, 5)).pitch(2, 7, r.u(0, 6)).unison(2, 2, r.u(8, 14, 2), 100)
    p.osc(3, analog, r.pick([sawPos, squarePos]), r.u(45, 65, 5)).pitch(3, r.pick([-12, 12]), r.u(3, 8))
    p.subOsc(r.u(30, 50, 5))
    p.env(2, r.u(2500, 7000, 500), 1000, 100, 1500, r.u(-40, 20, 10)).mod(env2, "filter1Cutoff", r.u(20, 40, 5)).mod(env2, "filter1Drive", r.u(20, 50, 10))
    p.filter(1, ladder24, r.log(150, 500), r.u(10, 35, 5))
    p.amp(r.u(1000, 1800, 100), 1000, 100, r.u(1500, 4000, 500), r.u(-20, 30, 10))
    hall(p, r, r.u(30, 50, 5))
    p.level(-9)
    p.target = -14
    knobs(p, r, k_cut(18), ("Fifth", "osc2Volume", -50), k_space(35), k_grit(55, hard), k_detune(1, 25), k_sub(35))


@kind("Cinematic", 8, "Horn Summons Warhorn Oliphant Call Signal Reckoning")
def cine_horn(p, r):
    # A saw and a pulse with a band for the bell of the instrument, slow to speak, far away: the horn before the battle.
    p.osc(1, analog, r.u(50, 67, 1)).unison(1, r.pick([2, 3, 4]), r.u(6, 14, 2), r.u(50, 90, 10)).route(1, r.u(20, 50, 10))
    p.osc(2, pulse, r.u(20, 60, 5), r.u(45, 65, 5)).pitch(2, r.pick([0, -12, 7]), r.u(3, 8)).route(2, r.u(30, 70, 10))
    p.parallel().subOsc(r.pick([1, 25, 40]))
    p.filter(1, ladder24, r.log(250, 700), r.u(5, 25, 5), r.u(0, 40, 10))
    p.filter(2, svfBand, r.log(350, 900), r.u(35, 60, 5))
    p.env(2, r.u(120, 500, 20), r.u(600, 2000, 100), r.u(50, 80, 10), 800, 50).mod(env2, "filter1Cutoff", r.u(15, 30)).mod(velocity, "filter1Cutoff", r.u(5, 12))
    p.amp(r.u(80, 400, 20), 600, r.u(85, 100, 5), r.u(600, 2000, 100))
    hall(p, r, r.u(35, 55, 5))
    p.level(-9)
    knobs(p, r, k_cut(22), ("Bell", "filter2Cutoff", 12), k_space(35), k_grit(55, hard), k_detune(1, 25), ("Nasal", "filter2Resonance", 25))


@kind("Cinematic", 7, "Heartbeat Pulse Lifesign Monitor Rhythm Ventricle")
def cine_heartbeat(p, r):
    # A low thump let through twice and then not: a heart, slower or faster with the song's tempo.
    p.osc(1, r.pick([analog, sub]), r.pick([sinePos, 10, 20])).subOsc(r.u(30, 60, 10))
    beat = [-100] * 16
    first, second = 0, r.pick([3, 4, 5])
    for i in (first, first + 1): beat[i] = 0
    for i in (second, second + 1): beat[i] = r.pick([-10, -20, -30])
    p.lfoSync(1, steps, r.pick([bar1, half, bars2]), 1, r.u(96, 99)).stepPattern(1, beat).mod(lfo1, "ampLevel", r.u(72, 85)).mod(lfo1, "filter1Cutoff", r.u(8, 20, 2))
    p.filter(1, ladder24, r.log(120, 500), r.u(5, 30, 5), r.u(0, 50, 10))
    p.amp(2, 300, 100, r.u(200, 600, 50))
    if r.chance(0.6): dirt(p, r, (tube, soft, hard), 15, 40, (800, 3000))
    hall(p, r, r.u(10, 35, 5))
    p.level(-9)
    knobs(p, r, k_cut(22), ("Beat", "ampLevel", 14), k_space(35), k_grit(50, tube, 2000), k_sub(35))


@kind("Cinematic", 8, "Sting Shock Reveal Twist Betrayal Ambush Discovery")
def cine_stinger(p, r):
    # A chord that should not be a chord, struck and left in a very large room: the cut to black.
    t = r.pick([analog, analog, pulse, metal, fm])
    pos = sawPos if t == analog else r.u(20, 70, 10)
    a, b = r.pick([(1, 6), (1, 7), (6, 13), (1, 11), (6, 11), (2, 6)])
    p.osc(1, t, pos).unison(1, r.pick([2, 3, 4]), r.u(10, 24, 2), 100)
    p.osc(2, t, pos, r.u(65, 85, 5)).pitch(2, a).osc(3, t, pos, r.u(55, 80, 5)).pitch(3, b)
    p.filter(1, r.pick([ladder12, svfLow, svfBand]), r.log(1200, 6000), r.u(10, 35, 5))
    p.env(2, 0, r.u(150, 800, 50), 0, 400, 70).mod(env2, "filter1Cutoff", r.u(10, 26, 2))
    p.amp(r.pick([0, 3, 15]), r.u(400, 1800, 100), 0, r.u(600, 2000, 100), r.u(50, 70, 5))
    if r.chance(0.5): dirt(p, r, (hard, tube), 15, 45)
    p.delay(r.pick([quarterD, quarter, half, eighthD]), r.u(45, 65, 5), r.u(18, 34, 2), r.log(1500, 5000))
    p.reverb(r.u(40, 60, 5), r.u(90, 100, 5), r.u(6, 12, 0.5), r.u(30, 60, 5))
    p.level(-9)
    knobs(p, r, k_cut(22), k_detune(1, 25), ("Space", "fxReverbMix", 30), k_grit(55, hard), ("Echo", "fxDelayMix", 35), k_res(30))


@kind("Cinematic", 8, "Clock Timer Metronome Watch Stopwatch Tick Chronometer")
def cine_clock(p, r):
    # A tick once a beat, or twice, with its echo: time running out.
    p.osc(1, pulse, r.u(70, 95, 5), r.u(50, 80, 5)).pitch(1, r.pick([12, 19, 24, 24])).noise(r.u(50, 90, 5), r.pick([0, 0, 3]), r.u(0, 60, 20))
    p.filter(1, r.pick([svfBand, ladderBand]), r.log(1500, 5000), r.u(40, 75, 5))
    if r.chance(0.5): p.filter(2, svfHigh, r.log(400, 1500), 10)
    tick = [-100] * 16
    tick[0] = 0
    if r.chance(0.5): tick[8] = r.pick([0, -20, -40])
    p.lfoSync(1, steps, r.pick([quarter, half, half, bar1]), 1, r.u(40, 80, 10)).stepPattern(1, tick).mod(lfo1, "ampLevel", r.u(75, 87, 4))
    p.amp(0, 300, 100, r.u(60, 250, 10))
    if r.chance(0.6): p.delay(r.pick([eighth, eighthD, sixteenthD, quarter]), r.u(25, 50, 5), r.u(12, 30, 2), r.log(2500, 9000))
    hall(p, r, r.u(15, 40, 5))
    p.level(-9)
    knobs(p, r, ("Tone", "filter1Cutoff", 15), k_res(20), k_space(35), k_grit(55, hard), k_echo(35, eighthD))


# =============================================================================================
# Game
# =============================================================================================

def one_shot_melody(p, r, notes, seconds, oscs=(1,), lfo=1):
    """Plays the 16 notes once over `seconds` from each key press, then stays on the last."""
    p.lfo(lfo, steps, round(1.0 / seconds, 2), 2, 0).stepPattern(lfo, step_notes(notes))

    for o in oscs:
        p.mod(lfo1 + lfo - 1, f"osc{o}Pitch", 50)


def game_wave(p, r, n=1):
    t = r.pick([pulse, pulse, analog, analog])
    p.osc(n, t, r.pick([0, 53, 80]) if t == pulse else r.pick([squarePos, trianglePos, sinePos]))


@kind("Game", 8, "Coin Token Gem Ring Star Key Shard")
def game_pickup(p, r):
    # Two notes, the second higher and longer: something collected.
    game_wave(p, r)
    jump = r.pick([5, 7, 7, 12, 12, 4, 9])
    first = r.pick([2, 3, 4])
    one_shot_melody(p, r, [0] * first + [jump] * (16 - first), r.u(0.3, 0.8, 0.05))
    p.filter(1, r.pick([svfLow, ladder12]), r.log(4000, 14000), 5)
    p.amp(0, r.u(250, 700, 50), 0, r.u(100, 300, 50), r.u(50, 70, 10))
    if r.chance(0.5): echo(p, r, r.u(8, 20, 2), (sixteenth, sixteenthD, eighth))
    if r.chance(0.5): room(p, r)
    p.level(-9)
    knobs(p, r, ("Tone", "filter1Cutoff", -30), ("Speed", "lfo1Rate", 15), k_echo(35, sixteenth), k_space(40, 50, 1.2), k_grit(55, crush), k_wide(45))


@kind("Game", 8, "Powerup Upgrade Boost Bonus Charge Refill Restore")
def game_power_up(p, r):
    # A run of notes upward from one key press: gaining something.
    game_wave(p, r)
    scale = r.pick([[0, 2, 4, 5, 7, 9, 11, 12], [0, 2, 3, 5, 7, 8, 10, 12], [0, 3, 7, 12, 15, 19, 24, 24], [0, 4, 7, 12, 16, 19, 24, 24],
                    [0, 2, 4, 6, 8, 10, 12, 14], [0, 1, 2, 3, 4, 5, 6, 7], [0, 5, 7, 12, 17, 19, 24, 24], [0, 7, 12, 7, 12, 19, 12, 24]])
    notes = [n for n in scale for _ in (0, 1)] if r.chance(0.6) else scale + [n + 12 if n + 12 <= 24 else 24 for n in scale]
    one_shot_melody(p, r, notes, r.u(0.35, 1.4, 0.05))
    p.filter(1, r.pick([svfLow, ladder12]), r.log(3500, 14000), 5)
    p.amp(0, 300, 100, r.u(100, 400, 50))
    echo(p, r, r.u(10, 24, 2), (sixteenth, sixteenthD, eighth))
    if r.chance(0.5): room(p, r)
    p.level(-9)
    knobs(p, r, ("Tone", "filter1Cutoff", -30), ("Speed", "lfo1Rate", 15), k_echo(35, sixteenth), k_space(40, 50, 1.2), k_grit(55, crush), k_wide(45))


@kind("Game", 7, "Defeat Loss Fail Fall Forfeit Ruin Wipeout")
def game_power_down(p, r):
    # The run downward, ending low: losing it.
    game_wave(p, r)
    scale = r.pick([[12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, -1, -2, -12], [12, 10, 8, 7, 5, 3, 2, 0, -2, -4, -5, -7, -9, -10, -12, -12],
                    [12, 7, 3, 0, -5, -9, -12, -12] * 2, [7, 6, 5, 4, 3, 2, 1, 0, -1, -2, -3, -4, -5, -6, -7, -12], [0, -1, 0, -2, 0, -3, 0, -5, 0, -7, -8, -9, -10, -11, -12, -12]])
    if len(scale) == 16 and scale[:8] == scale[8:]:
        scale = [n for n in scale[:8] for _ in (0, 1)]
    one_shot_melody(p, r, scale, r.u(0.5, 1.8, 0.05))
    p.filter(1, r.pick([svfLow, ladder12]), r.log(2500, 10000), 5)
    p.amp(0, r.u(700, 2200, 100), r.pick([0, 60]), r.u(100, 400, 50), r.u(20, 60, 10))
    if r.chance(0.5): echo(p, r, r.u(10, 24, 2), (sixteenth, sixteenthD, eighth))
    if r.chance(0.5): p.drive(crush, r.u(15, 40, 5), r.log(4000, 10000))
    p.level(-9)
    knobs(p, r, ("Tone", "filter1Cutoff", -30), ("Speed", "lfo1Rate", 15), k_echo(35, sixteenth), k_space(40, 50, 1.2), k_grit(55, crush), k_wide(45))


@kind("Game", 8, "Jump Hop Leap Spring Vault Bound Dash")
def game_jump(p, r):
    # The pitch runs up in an instant, in semitone steps as an old sound chip does it: a jump.
    game_wave(p, r)
    p.pitch(1, r.pick([-12, -7, 0, -5]))
    p.env(2, r.u(60, 350, 10), r.u(40, 200, 20), r.pick([100, 100, 0]), 100, r.u(-40, 40, 20)).mod(env2, "osc1Pitch", semis(r.pick([7, 12, 12, 17, 19, 24])))
    p.filter(1, r.pick([svfLow, ladder12]), r.log(3500, 14000), 5)
    p.amp(0, r.u(120, 500, 20), 0, r.u(60, 200, 20), r.u(30, 70, 10))
    if r.chance(0.4): echo(p, r, r.u(8, 18, 2), (sixteenth, sixteenthD))
    p.level(-9)
    knobs(p, r, ("Height", "osc1Pitch", semis(7)), ("Tone", "filter1Cutoff", -30), k_grit(55, crush), k_space(40, 50, 1.2), k_echo(35, sixteenth), k_wide(45))


@kind("Game", 8, "Blaster Pew Shot Cannon Turret")
def game_shot(p, r):
    # The pitch falls from high in a tenth of a second: a shot fired.
    game_wave(p, r)
    p.pitch(1, r.pick([0, -12, -5, 7]))
    p.env(2, 0, r.u(60, 320, 10), 0, 100, r.u(40, 90, 10)).mod(env2, "osc1Pitch", semis(r.pick([12, 19, 24, 24, 31, 36])))
    if r.chance(0.4): p.noise(r.u(20, 50, 5), 3, r.u(0, 60, 20))
    p.filter(1, r.pick([svfLow, ladder12, svfBand]), r.log(3000, 14000), r.u(5, 30, 5))
    p.amp(0, r.u(100, 450, 10), 0, r.u(50, 200, 10), r.u(40, 80, 10))
    if r.chance(0.4): echo(p, r, r.u(8, 18, 2), (sixteenth, sixteenthD))
    if r.chance(0.4): p.drive(crush, r.u(15, 40, 5), r.log(4000, 10000))
    p.level(-9)
    knobs(p, r, ("Fall", "osc1Pitch", semis(12)), ("Tone", "filter1Cutoff", -30), k_grit(55, crush), k_space(40, 50, 1.2), k_echo(35, sixteenth), k_wide(45))


@kind("Game", 7, "Explosion Blast Boom Detonation Burst Wreck Crater")
def game_explosion(p, r):
    # Pitched digital noise whose pitch falls as it dies away: an explosion out of an old sound chip.
    p.set("osc1On", 0).noise(r.u(80, 100, 5), 3, r.u(0, 60, 20))
    p.env(2, 0, r.u(200, 1500, 50), 0, 400, r.u(20, 80, 10)).mod(env2, "noiseColour", r.u(20, 50, 5)).mod(env2, "filter1Cutoff", r.u(10, 30, 5))
    p.filter(1, r.pick([svfLow, ladder12, ladder24]), r.log(500, 4000), r.u(5, 30, 5))
    if r.chance(0.5): p.subOsc(r.u(30, 55, 5))
    p.amp(0, r.u(300, 2000, 50), 0, r.u(200, 800, 50), r.u(30, 70, 10))
    if r.chance(0.5): p.drive(r.pick([crush, hard]), r.u(15, 45, 5), r.log(3000, 9000))
    if r.chance(0.4): room(p, r)
    p.level(-9)
    knobs(p, r, ("Size", "noiseColour", -35), ("Tone", "filter1Cutoff", 20), k_grit(55, crush), k_space(40, 55, 1.6))


@kind("Game", 8, "Cursor Select Menu Tab Toggle Prompt Hover")
def game_ui_blip(p, r):
    # A very short, clean note for moving through a menu, a little different each time.
    p.osc(1, analog, r.pick([sinePos, sinePos, trianglePos, 15])).pitch(1, r.pick([0, 0, 5, 7, 12]))
    if r.chance(0.5): p.osc(2, analog, sinePos, r.u(40, 60, 5)).pitch(2, p.get("osc1Pitch") + r.pick([12, 7, 4]))
    p.env(2, 0, r.u(15, 80, 5), 0, 40, 70).mod(env2, "osc1Fine", r.pick([-1, 1]) * r.u(10, 40, 10))
    p.mod(perNote, "osc1Fine", r.u(3, 12))
    p.filter(1, r.pick([svfLow, ladder12]), r.log(3000, 12000), 5)
    p.amp(r.pick([0, 2, 5]), r.u(30, 140, 10), 0, r.u(30, 120, 10), r.u(40, 70, 10))
    if r.chance(0.5): room(p, r, r.u(8, 20, 4))
    p.level(-9)
    knobs(p, r, ("Tone", "filter1Cutoff", -30), ("Chirp", "osc1Fine", 30), k_space(40, 50, 1.0), k_echo(35, sixteenth), k_grit(55), k_wide(45))


@kind("Game", 8, "Confirm Accept Unlock Success Granted Checkpoint Save")
def game_ui_confirm(p, r):
    # Two or three soft notes upward: yes.
    p.osc(1, analog, r.pick([sinePos, trianglePos, 15])).osc(2, analog, sinePos, r.u(40, 65, 5)).pitch(2, 12)
    melody = r.pick([[0, 4, 7], [0, 7, 12], [0, 5, 12], [0, 4, 9], [0, 7], [0, 5], [0, 12], [0, 3, 7], [0, 4, 12]])
    per = 16 // (len(melody) + 1)
    notes = [n for n in melody for _ in range(per)]
    notes += [melody[-1]] * (16 - len(notes))
    one_shot_melody(p, r, notes, r.u(0.3, 0.8, 0.05), (1, 2))
    p.filter(1, r.pick([svfLow, ladder12]), r.log(3000, 10000), 5)
    p.amp(r.pick([0, 2, 5]), r.u(300, 900, 50), 0, r.u(200, 500, 50), r.u(40, 65, 5))
    hall(p, r, r.u(12, 30))
    p.level(-9)
    knobs(p, r, ("Tone", "filter1Cutoff", -30), ("Speed", "lfo1Rate", 15), k_space(35), k_echo(35, sixteenth), ("Octave", "osc2Volume", -45), k_wide(45))


@kind("Game", 7, "Deny Refuse Locked Error Invalid Blocked Reject")
def game_ui_error(p, r):
    # A low buzz, twice: no.
    p.osc(1, *wave(r, [analog, pulse], [squarePos, sawPos])).pitch(1, r.pick([-12, -17, -19, -24]))
    if r.chance(0.5): p.osc(2, analog, squarePos, r.u(50, 70, 5)).pitch(2, p.get("osc1Pitch") + r.pick([1, 6]))
    buzz = r.pick([[0, 0, 0, 0, 0, -100, -100, 0, 0, 0, 0, 0, 0, -100, -100, -100], [0, 0, 0, -100, 0, 0, 0, -100, -100, -100, -100, -100, -100, -100, -100, -100],
                   [0, 0, 0, 0, 0, 0, -100, 0, 0, 0, 0, 0, 0, 0, -100, -100]])
    p.lfo(1, steps, r.u(1.5, 4, 0.25), 2, r.u(75, 90, 5)).stepPattern(1, buzz).mod(lfo1, "ampLevel", r.u(75, 87, 4))
    p.filter(1, r.pick([svfLow, ladder12, svfBand]), r.log(900, 5000), r.u(5, 35, 5))
    p.amp(0, 300, 100, r.u(40, 150, 10))
    if r.chance(0.5): p.drive(r.pick([crush, hard]), r.u(15, 45, 5), r.log(3000, 9000))
    if r.chance(0.4): room(p, r)
    p.level(-9)
    knobs(p, r, ("Tone", "filter1Cutoff", 20), ("Speed", "lfo1Rate", 12), k_grit(55, crush), k_space(40, 50, 1.2))


@kind("Game", 8, "Fanfare Victory Triumph Clear Reward Trophy Encore")
def game_fanfare(p, r):
    # A short tune from one key press: the level cleared.
    game_wave(p, r)
    if r.chance(0.6): p.osc(2, pulse, r.pick([0, 53, 80]), r.u(45, 65, 5)).pitch(2, r.pick([-12, -12, 0]), r.pick([0, 8]))
    tune = r.pick([[0, 0, 4, 4, 7, 7, 12, 12, 12, 12, 7, 7, 12, 12, 12, 12], [0, 0, 0, 0, 0, 0, 4, 4, 7, 7, 7, 7, 12, 12, 12, 12],
                   [7, 7, 7, 12, 12, 12, 16, 16, 19, 19, 19, 19, 19, 19, 19, 19], [0, 4, 7, 12, 0, 4, 7, 12, 4, 7, 12, 16, 12, 12, 12, 12],
                   [0, 0, 5, 5, 0, 0, 5, 5, 7, 7, 7, 7, 12, 12, 12, 12], [12, 12, 7, 7, 9, 9, 5, 5, 7, 7, 4, 4, 0, 0, 0, 0],
                   [0, 0, 3, 3, 7, 7, 10, 10, 12, 12, 12, 12, 15, 15, 12, 12], [0, 2, 4, 5, 7, 7, 7, 7, 9, 9, 11, 11, 12, 12, 12, 12]])
    one_shot_melody(p, r, tune, r.u(0.9, 2.4, 0.1), (1, 2) if p.has("osc2On") else (1,))
    p.filter(1, r.pick([svfLow, ladder12]), r.log(3500, 14000), 5)
    p.amp(0, 300, 100, r.u(100, 400, 50))
    echo(p, r, r.u(8, 20, 2), (sixteenth, sixteenthD, eighth))
    if r.chance(0.5): room(p, r)
    p.level(-9)
    knobs(p, r, ("Tone", "filter1Cutoff", -30), ("Speed", "lfo1Rate", 12), k_echo(35, sixteenth), k_space(40, 50, 1.2), k_grit(55, crush), k_wide(45))


@kind("Game", 7, "Hazard Danger Critical Overheat Breach Intruder Meltdown")
def game_alarm(p, r):
    # Beeps that repeat while the key is held: the warning that the player has little left.
    game_wave(p, r)
    beeps = r.pick([[0, 0, -100, -100] * 4, [0, -100] * 8, [0, 0, 0, -100, 0, 0, 0, -100, -100, -100, -100, -100, -100, -100, -100, -100],
                    [0, 0, -100, 0, 0, -100, -100, -100] * 2])
    p.lfo(1, steps, r.u(0.5, 2.5, 0.25), 1, r.u(75, 90, 5)).stepPattern(1, beeps).mod(lfo1, "ampLevel", r.u(75, 87, 4))
    if r.chance(0.5): p.lfo(2, square, r.u(0.5, 2.5, 0.25), 1).mod(lfo2, "osc1Pitch", semis(r.pick([1, 3, 5, 7, 12])))
    p.filter(1, r.pick([svfLow, ladder12, svfBand]), r.log(2500, 10000), r.u(5, 30, 5))
    p.amp(2, 300, 100, r.u(40, 150, 10))
    if r.chance(0.5): echo(p, r, r.u(8, 18, 2), (sixteenth, eighth))
    p.level(-9)
    knobs(p, r, ("Tone", "filter1Cutoff", -30), ("Speed", "lfo1Rate", 15), k_grit(55, crush), k_space(40, 50, 1.2), k_echo(35, sixteenth), k_wide(45))


# =============================================================================================
# Chip
# =============================================================================================

DUTIES = [0, 53, 80]   # the pulse table's positions for a 50 %, 25 % and 12.5 % duty


@kind("Chip", 8, "Pulse Square Channel Voice Cartridge")
def chip_lead(p, r):
    # One pulse at a fixed duty with a vibrato that arrives late: the lead voice of an 8-bit console.
    p.osc(1, pulse, r.pick(DUTIES))
    p.lfo(1, r.pick([sine, triangle]), r.u(5, 8, 0.25), 1).lfoFade(1, r.u(100, 600, 50)).mod(lfo1, "osc1Fine", r.u(5, 18))
    p.filter(1, off, 8000)
    p.amp(0, r.u(150, 600, 50), r.u(40, 100, 10), r.u(30, 150, 10)).mono(r.pick([0, 0, 30, 60]), r.chance(0.5))
    if r.chance(0.5): echo(p, r, r.u(10, 24, 2), (eighth, eighthD, sixteenthD))
    p.level(-9)
    knobs(p, r, ("Duty", "osc1Position", 30), ("Octave", "osc1Pitch", semis(12)), k_echo(35, eighth), k_space(35, 40, 1.0))


@kind("Chip", 8, "Arpeggio Chord Triad Broken Cascade Flicker Shimmer")
def chip_arp_chord(p, r):
    # A chord played as its notes one after another, fifty a second: how one voice plays three.
    p.osc(1, pulse, r.pick(DUTIES))
    chord = r.pick([[0, 3, 7, 12], [0, 4, 7, 12], [0, 3, 7, 10], [0, 5, 7, 12], [0, 12, 0, 12], [0, 7, 12, 19], [0, 3, 8, 12], [0, 4, 9, 12], [0, 7, 0, 7]])
    p.lfo(1, steps, r.pick([1.5, 2, 2.5, 3, 3.75, 5]), 1, 0).stepPattern(1, step_notes(chord * 4)).mod(lfo1, "osc1Pitch", 50)
    p.filter(1, off, 8000)
    p.amp(0, r.u(200, 900, 50), r.u(0, 100, 20), r.u(30, 200, 10))
    if r.chance(0.5): echo(p, r, r.u(10, 24, 2), (eighth, eighthD, sixteenthD))
    p.level(-9)
    knobs(p, r, ("Duty", "osc1Position", 30), ("Speed", "lfo1Rate", 12), k_echo(35, eighth), k_grit(55, crush))


@kind("Chip", 8, "Triangle Ramp Stair Bass Ladder Terrace Step")
def chip_triangle_bass(p, r):
    # A triangle cut into sixteen levels: the bass channel.
    p.osc(1, analog, trianglePos).warp(1, wCrush, r.u(52, 62, 2)).pitch(1, r.pick([-12, -12, 0]))
    if r.chance(0.4):
        p.env(2, 0, r.u(30, 120, 10), 0, 60, 80).mod(env2, "osc1Pitch", semis(r.pick([12, 7, 24])))
    p.filter(1, off, 8000)
    p.amp(0, r.u(200, 800, 50), r.pick([0, 100, 100]), r.u(20, 120, 10)).mono(0, r.chance(0.5))
    p.level(-9)
    need_delay(p, eighth, 30, 6000)
    knobs(p, r, ("Steps", "osc1WarpAmount", -30), ("Grit", "fxDriveAmount", 55, lambda q: need_drive(q, crush, 8000)), ("Echo", "fxDelayMix", 35), ("Space", "fxReverbMix", 35, lambda q: need_reverb(q, 40, 1.0)))


@kind("Chip", 8, "Noise Hiss Crunch Fuzz Static Burst Crash")
def chip_noise_drum(p, r):
    # The noise channel: pitched digital noise, short for a hat, longer and falling for a snare or a crash.
    p.set("osc1On", 0).noise(r.u(80, 100, 5), 3, r.u(-40, 80, 20))
    length = r.pick([40, 60, 90, 150, 250, 400, 700])
    p.env(2, 0, length, 0, length, r.u(30, 80, 10)).mod(env2, "noiseColour", r.pick([1, 1, -1]) * r.u(10, 40, 5))
    p.filter(1, off, 8000)
    p.amp(0, length, 0, r.u(20, 150, 10), r.u(40, 80, 10))
    p.level(-9)
    knobs(p, r, ("Pitch", "noiseColour", 35), ("Grit", "fxDriveAmount", 55, lambda q: need_drive(q, crush, 8000)), ("Echo", "fxDelayMix", 35, lambda q: need_delay(q, sixteenth, 30, 8000)),
          ("Space", "fxReverbMix", 35, lambda q: need_reverb(q, 40, 1.0)))


@kind("Chip", 8, "Sweep Shift Cycle Morph Rotate Phase Flip")
def chip_duty_sweep(p, r):
    # The duty changed on a pattern, as trackers do it: one note with a moving edge.
    p.osc(1, pulse, 40)
    duties = [r.pick([-100, -30, 30, 100]) for _ in range(4)]
    p.lfo(1, steps, r.pick([0.5, 1, 1.5, 2, 3, 4]), 1, 0).stepPattern(1, duties * 4).mod(lfo1, "osc1Position", r.u(30, 40, 5))
    if r.chance(0.5): p.osc(2, pulse, r.pick(DUTIES), r.u(50, 70, 5)).pitch(2, r.pick([12, -12, 7]), r.pick([0, 0, 8]))
    p.filter(1, off, 8000)
    p.amp(0, r.u(200, 800, 50), r.u(30, 100, 10), r.u(30, 200, 10))
    if r.chance(0.5): echo(p, r, r.u(10, 24, 2), (eighth, eighthD, sixteenthD))
    p.level(-9)
    knobs(p, r, ("Duty", "osc1Position", 30), ("Speed", "lfo1Rate", 12), k_echo(35, eighth), k_space(35, 40, 1.0))


@kind("Chip", 8, "Duet Twin Pair Unison Echo Double Mirror")
def chip_two_voice(p, r):
    # Two pulse channels a few cents apart, or one an octave up and late: the fat lead of a two-channel tune.
    p.osc(1, pulse, r.pick(DUTIES)).osc(2, pulse, r.pick(DUTIES), r.u(60, 80, 5)).pitch(2, r.pick([0, 0, 12, -12, 7]), r.u(6, 16, 2))
    p.pan(1, -r.u(0, 40, 10)).pan(2, r.u(0, 40, 10))
    p.lfo(1, r.pick([sine, triangle]), r.u(5, 8, 0.25), 1).lfoFade(1, r.u(100, 600, 50)).mod(lfo1, "osc1Fine", r.u(4, 14)).mod(lfo1, "osc2Fine", r.u(4, 14))
    p.filter(1, off, 8000)
    p.amp(0, r.u(150, 600, 50), r.u(50, 100, 10), r.u(30, 150, 10))
    p.delay(r.pick([eighth, eighthD, sixteenthD, sixteenth]), r.u(20, 45, 5), r.u(12, 28, 2), r.log(4000, 12000), r.chance(0.5))
    p.level(-9)
    knobs(p, r, ("Duty", "osc1Position", 30), ("Detune", "osc2Fine", 20), ("Echo", "fxDelayMix", 30), k_grit(55, crush))


@kind("Chip", 8, "Drop Dive Slide Fall Tumble Plunge Descent")
def chip_pitch_drop(p, r):
    # A pulse or triangle whose pitch falls in semitone steps at the start of each note: tracker kicks, toms and drops.
    if r.chance(0.65):
        p.osc(1, pulse, r.pick(DUTIES))
    else:
        p.osc(1, analog, trianglePos).warp(1, wCrush, r.u(52, 62, 2))
    p.pitch(1, r.pick([-12, 0, -5]))
    p.env(2, 0, r.u(40, 500, 10), 0, 100, r.u(20, 85, 5)).mod(env2, "osc1Pitch", semis(r.pick([12, 19, 24, 31, 36])))
    p.filter(1, off, 8000)
    p.amp(0, r.u(100, 700, 20), 0, r.u(20, 150, 10), r.u(40, 80, 10))
    p.level(-9)
    knobs(p, r, ("Fall", "osc1Pitch", semis(12)), ("Grit", "fxDriveAmount", 55, lambda q: need_drive(q, crush, 8000)), ("Echo", "fxDelayMix", 35, lambda q: need_delay(q, sixteenth, 30, 8000)),
          ("Space", "fxReverbMix", 35, lambda q: need_reverb(q, 40, 1.0)))
