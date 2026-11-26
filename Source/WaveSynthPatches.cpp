#include "WaveSynthPatchBuilder.h"

// Torqwave Synth's factory patches: sounds for dark techno and industrial music. Each is written as
// the settings it changes from the defaults (a patch loads onto a fresh synth), its modulation
// slots and its macros. Their levels are checked by a test: every patch plays a held chord
// between -24 and -1 dB.

namespace WaveSynthPatchBuilder
{
    static std::vector<WaveSynth::Patch> build()
    {
        std::vector<Builder> b;
        const auto add = [&b] (const char* name, const char* category) -> Builder& { b.emplace_back (name, category); return b.back(); };

        add ("Init", "Init").osc (1, analog, sawPos).level (-12);

        // The layered patches lead their categories.
        addLayered (b);
        addLayeredPads (b);
        addLayeredSequences (b);
        addLayeredStabs (b);
        addLayeredPlucks (b);
        addLayeredHits (b);
        addLayeredRisers (b);

        //======================================================================
        // Bass: the low end of a techno track, mostly mono and dry.
        add ("Warehouse Sub", "Bass").osc (1, sub, 20).subOsc (40).filter (1, ladder24, 600, 5).amp (1, 400, 90, 60).mono()
            .level (-4).macro (1, "Grit").mod (macro1, "osc1Position", 70).macro (2, "Body").mod (macro2, "subLevel", 50);
        add ("Rumble Floor", "Bass").osc (1, analog, trianglePos).subOsc (60, 0).filter (1, ladder24, 180, 30, 40).amp (2, 1200, 0, 300)
            .env (2, 0, 220, 0, 200, 80).mod (env2, "filter1Cutoff", 25).reverb (35, 80, 4.0f, 70).level (-13)
            .macro (1, "Tail").mod (macro1, "fxReverbMix", 40).macro (2, "Thump").mod (macro2, "filter1Drive", 60);
        add ("Pressure Bass", "Bass").osc (1, analog, sawPos).osc (2, analog, squarePos, 60).pitch (2, -12).subOsc (40)
            .filter (1, ladder24, 300, 35, 30).env (2, 0, 180, 10, 150, 70).mod (env2, "filter1Cutoff", 30).amp (1, 300, 80, 60).mono()
            .level (-4).standardMacros (25);
        add ("Bunker Kick Bass", "Bass").osc (1, sub, 0).subOsc (30).amp (0, 260, 0, 80).env (2, 0, 60, 0, 60, 90)
            .mod (env2, "osc1Pitch", 30).mod (env2, "osc1Position", 50).filter (1, ladder24, 900, 0).level (-11)
            .macro (1, "Punch").mod (macro1, "osc1Pitch", 20).macro (2, "Click").mod (macro2, "osc1Position", 40);
        add ("Rolling Offbeat", "Bass").osc (1, analog, sawPos).unison (1, 2, 6, 30).filter (1, ladder24, 220, 45, 20)
            .env (2, 0, 140, 0, 100, 75).mod (env2, "filter1Cutoff", 35).amp (1, 160, 0, 60).mono (0, false).level (-3).standardMacros (25);
        add ("Iron Sub", "Bass").osc (1, analog, sinePos).subOsc (40, 0, 1).drive (tube, 45, 3000).amp (2, 600, 90, 100).mono()
            .level (-13).macro (1, "Saturate").mod (macro1, "fxDriveAmount", 50).macro (2, "Octave").mod (macro2, "subLevel", 50);
        add ("Grinder", "Bass").osc (1, growl, 40).warp (1, wFold, 30).filter (1, ladder24, 500, 40, 60).env (2, 0, 250, 20, 150, 60)
            .mod (env2, "filter1Cutoff", 30).subOsc (40).mono().amp (1, 300, 85, 80).drive (hard, 40, 5000).level (-17)
            .macro (1, "Fold").mod (macro1, "osc1WarpAmount", 60).macro (2, "Growl").mod (macro2, "osc1Position", 55);
        add ("Pulse Bunker", "Bass").osc (1, pulse, 40).subOsc (35).filter (1, svfLow, 700, 30).env (2, 0, 200, 0, 150, 70)
            .mod (env2, "filter1Cutoff", 25).lfo (1, triangle, 0.15f, 0).mod (lfo1, "osc1Position", 25).mono().amp (1, 300, 80, 80).level (-4)
            .macro (1, "Width").mod (macro1, "osc1Position", 50).macro (2, "Bite").mod (macro2, "filter1Resonance", 50);
        add ("Distorted 909 Bass", "Bass").osc (1, analog, sawPos).filter (1, ladder24, 400, 50, 50).env (2, 0, 120, 0, 100, 80)
            .mod (env2, "filter1Cutoff", 40).drive (fuzz, 45, 4500).amp (0, 220, 0, 60).mono (0, false).level (-24).standardMacros (25);
        add ("FM Basement", "Bass").osc (1, fm, 30).subOsc (40).env (2, 0, 180, 10, 120, 70).mod (env2, "osc1Position", 40)
            .filter (1, ladder24, 1200, 15).amp (1, 300, 80, 80).mono().level (-4)
            .macro (1, "Index").mod (macro1, "osc1Position", 60).macro (2, "Bark").mod (macro2, "filter1Drive", 70);
        add ("Machine Sequence", "Bass").osc (1, analog, sawPos).osc (2, analog, squarePos, 50).pitch (2, 0, 7).filter (1, ladder24, 350, 40, 25)
            .env (2, 0, 110, 0, 90, 80).mod (env2, "filter1Cutoff", 40).amp (0, 180, 0, 50).mono (0, false).level (-3).standardMacros (30);
        add ("Iron Motion", "Bass").osc (1, pulse, 20).osc (2, analog, sawPos, 70).pitch (2, -12).filter (1, ladder12, 600, 30, 20)
            .env (2, 0, 160, 0, 120, 70).mod (env2, "filter1Cutoff", 30).amp (0, 250, 0, 60).mono (0, false).drive (tube, 25).level (-12)
            .standardMacros (30);
        // Target Lock Bass: a sequenced bass matched by spectrum to an early EBM classic: one sawtooth through a
        // resonant 24 dB low-pass that follows the key, ringing on the 3rd harmonic with a cliff after the 4th, its
        // fundamental thinned by a high-pass and a little tube drive on top.
        add ("Target Lock Bass", "Bass").osc (1, analog, sawPos).filter (1, ladder24, 780, 90, 30, 100).filter (2, svfHigh, 90, 10).env (2, 0, 140, 20, 90, 70)
            .mod (env2, "filter1Cutoff", 12).drive (tube, 30, 6000).amp (0, 300, 70, 40).mono (0, false).level (-9).standardMacros (30);

        // Reese: detuned and moving, for drum and bass and dark techno alike.
        add ("Abyss Reese", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos).pitch (2, 0, 12).unison (1, 2, 10, 40).unison (2, 2, 14, 40)
            .filter (1, ladder24, 700, 25, 30).lfo (1, sine, 0.2f, 0).mod (lfo1, "filter1Cutoff", 8).subOsc (30).mono().amp (5, 400, 90, 150)
            .level (-4).standardMacros (25);
        add ("Tar Reese", "Reese").osc (1, analog, sawPos).unison (1, 4, 22, 50).filter (1, ladder24, 450, 30, 50).phaser (40, 0.1f, 60)
            .subOsc (35).mono().amp (5, 400, 90, 200).level (-4).macro (1, "Detune").mod (macro1, "osc1UnisonDetune", 40)
            .macro (2, "Open").mod (macro2, "filter1Cutoff", 30);
        add ("Corroded Reese", "Reese").osc (1, sync, 30).osc (2, analog, sawPos).pitch (2, 0, -9).unison (2, 3, 16, 60)
            .lfo (1, triangle, 0.3f, 0).mod (lfo1, "osc1Position", 25).filter (1, ladder24, 900, 30, 40).drive (hard, 35, 6000)
            .subOsc (30).mono().amp (5, 400, 90, 150).level (-15).standardMacros (25);
        add ("Wormhole", "Reese").osc (1, growl, 50).unison (1, 4, 18, 70).lfoSync (1, sine, bars2, 0).mod (lfo1, "osc1Position", 35)
            .filter (1, svfLow, 1200, 40).mod (lfo1, "filter1Cutoff", 10).subOsc (30).mono().amp (5, 500, 90, 200).level (-8)
            .standardMacros (25);
        add ("Pipe Reese", "Reese").osc (1, pulse, 60).osc (2, pulse, 30).pitch (2, 0, 15).filter (1, ladderBand, 700, 45, 30)
            .lfo (1, sine, 0.15f, 0).mod (lfo1, "filter1Cutoff", 12).subOsc (40).mono().amp (5, 400, 90, 200).level (-5).standardMacros (25);

        // Acid: one oscillator into a screaming resonant ladder.
        add ("Acid Corrosion", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 300, 75, 30, 30).env (2, 0, 180, 0, 120, 75)
            .mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 15).mono (60).amp (1, 200, 70, 60).drive (hard, 40, 6000)
            .delay (sixteenthD, 35, 15).level (-15).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 22).macro (3, "Env Mod").mod (macro3, "filter1Drive", 50);
        add ("Square Acid", "Acid").osc (1, analog, squarePos).filter (1, ladder24, 260, 80, 20, 30).env (2, 0, 220, 0, 120, 75)
            .mod (env2, "filter1Cutoff", 45).mod (velocity, "filter1Cutoff", 12).mono (50).amp (1, 220, 70, 60).drive (tube, 35)
            .level (-14).standardMacros (35);
        add ("Acid Scream", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 600, 88, 50, 30).env (2, 0, 300, 10, 150, 70)
            .mod (env2, "filter1Cutoff", 35).mono (80).amp (1, 300, 80, 80).drive (fuzz, 55, 5000).delay (eighthD, 45, 20).level (-28)
            .standardMacros (30);
        add ("Rubber Acid", "Acid").osc (1, analog, sawPos).filter (1, ladder12, 400, 70, 20, 30).env (2, 0, 150, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 40).mono (40).amp (1, 180, 60, 60).level (-7).standardMacros (30);
        add ("Acid Drip", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 350, 70, 20, 30).env (2, 0, 150, 0, 100, 80)
            .mod (env2, "filter1Cutoff", 40).lfoSync (1, steps, bar1, 0).stepPattern (1, { 100, -40, 30, -60, 80, -20, 0, -80, 100, -30, 40, -70, 60, -10, 20, -90 })
            .mod (lfo1, "filter1Cutoff", 12).mono (30).amp (1, 160, 60, 60).delay (eighthD, 40, 20).level (-4).standardMacros (30);

        //======================================================================
        // Leads.
        add ("Siren Burn", "Lead").osc (1, analog, sawPos).unison (1, 3, 12, 50).filter (1, ladder24, 2500, 40, 40).drive (fuzz, 50, 7000)
            .lfo (1, sine, 5.5f, 1).lfoFade (1, 400).mod (lfo1, "osc1Pitch", 0.6f).mono (80)
            .amp (5, 400, 80, 200).delay (eighthD, 40, 20).level (-26).standardMacros (25);
        add ("Mercury Lead", "Lead").osc (1, sync, 40).env (2, 0, 400, 20, 200, 60).mod (env2, "osc1Position", 40)
            .filter (1, ladder24, 4000, 20).mono (50).amp (2, 300, 80, 200).delay (eighthD, 35, 20).reverb (20).level (0)
            .macro (1, "Sync").mod (macro1, "osc1Position", 50).macro (2, "Grit").drive (hard, 0, 9000).mod (macro2, "fxDriveAmount", 70)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Feedback Wire", "Lead").osc (1, fm, 50).osc (2, analog, sawPos, 50).pitch (2, 12, 5).filter (1, svfBand, 1800, 60)
            .lfo (1, randomShape, 6, 0, 40).mod (lfo1, "filter1Cutoff", 8).drive (crush, 30, 8000).mono (30).amp (3, 300, 80, 150)
            .level (-2).standardMacros (25);
        add ("Industrial Whistle", "Lead").osc (1, analog, trianglePos).noise (10, 1).filter (1, svfBand, 2500, 75, 0, 100)
            .lfo (1, sine, 4.5f, 1).lfoFade (1, 600).mod (lfo1, "osc1Pitch", 0.5f).mono (120).amp (20, 300, 90, 300).reverb (30, 70, 3.5f)
            .level (6).standardMacros (20);
        add ("Rust Lead", "Lead").osc (1, growl, 60).warp (1, wBend, 30).unison (1, 2, 8, 40).filter (1, ladder24, 3000, 30, 30)
            .drive (fuzz, 40, 6000).mono (40).amp (3, 300, 80, 150).delay (quarterT, 35, 15).level (-20).standardMacros (25);
        add ("Cold Saw Lead", "Lead").osc (1, analog, sawPos).osc (2, analog, sawPos).pitch (2, 0, 9).unison (1, 2, 10, 60)
            .filter (1, ladder24, 3500, 25).env (2, 0, 300, 30, 200, 60).mod (env2, "filter1Cutoff", 15).mono (60).amp (3, 400, 80, 200)
            .delay (eighthD, 35, 20).reverb (20).level (-7).standardMacros (25);
        add ("Vowel Scream", "Lead").osc (1, vowel, 20).lfoSync (1, triangle, bar1, 1).mod (lfo1, "osc1Position", 40)
            .filter (1, ladder12, 4000, 20).drive (tube, 40).mono (50).amp (5, 400, 85, 200).level (-14)
            .macro (1, "Vowel").mod (macro1, "osc1Position", 70).macro (2, "Grit").mod (macro2, "fxDriveAmount", 60);

        //======================================================================
        // Stabs: short, chordal, for hits on the offbeat.
        add ("Dub Chord Stab", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 60).pitch (2, 7).unison (1, 2, 8, 50)
            .filter (1, ladder24, 900, 30).env (2, 0, 200, 0, 150, 80).mod (env2, "filter1Cutoff", 25).amp (0, 350, 0, 200)
            .delay (quarterD, 55, 35, 2500).reverb (30, 80, 3.0f, 60).level (-9).standardMacros (30);
        add ("Minor Stab", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos).pitch (2, 3).osc (3, analog, sawPos).pitch (3, 7)
            .filter (1, ladder24, 1600, 25).env (2, 0, 180, 0, 150, 80).mod (env2, "filter1Cutoff", 25).amp (0, 260, 0, 160)
            .reverb (25).level (-12).standardMacros (30);
        add ("Metal Stab", "Stab").osc (1, metal, 50).osc (2, analog, sawPos, 60).filter (1, svfBand, 1500, 40).env (2, 0, 150, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 30).amp (0, 220, 0, 150).drive (hard, 30).reverb (25, 60, 1.8f).level (-10).standardMacros (25);
        add ("Rave Hoover Stab", "Stab").osc (1, pulse, 20).unison (1, 6, 30, 90).pitch (1, 0).env (2, 0, 150, 0, 150, 70)
            .mod (env2, "osc1Pitch", -1.5f).filter (1, ladder24, 3000, 20).amp (0, 300, 40, 200).chorus (40).level (-6).standardMacros (25);
        add ("Strobe Stab", "Stab").osc (1, sync, 60).unison (1, 3, 12, 60).filter (1, ladder24, 2200, 35, 30).env (2, 0, 90, 0, 80, 85)
            .mod (env2, "filter1Cutoff", 30).amp (0, 140, 0, 100).delay (sixteenthD, 40, 25).level (-7).standardMacros (25);
        add ("Factory Hit", "Stab").osc (1, metal, 70).noise (30, 2).filter (1, comb, 400, 70).filter (2, svfHigh, 300, 10)
            .env (2, 0, 120, 0, 120, 85).mod (env2, "filter1Cutoff", 15).amp (0, 250, 0, 200).reverb (30, 60, 1.5f).level (-2)
            .standardMacros (20);
        add ("Organ Stab", "Stab").osc (1, additive, 30).osc (2, additive, 10, 60).pitch (2, 12).filter (1, ladder12, 3000, 10)
            .amp (1, 300, 30, 150).drive (tube, 40).chorus (35, 4.5f, 20).level (-15).standardMacros (25);

        //======================================================================
        // Plucks.
        add ("Black Pluck", "Pluck").osc (1, analog, sawPos).osc (2, analog, squarePos, 50).pitch (2, 12).filter (1, ladder24, 400, 30, 0, 50)
            .env (2, 0, 220, 0, 200, 85).mod (env2, "filter1Cutoff", 40).amp (0, 400, 0, 300).delay (eighthD, 40, 25).reverb (25)
            .level (-10).standardMacros (30);
        add ("Glass Shard", "Pluck").osc (1, fm, 60).env (2, 0, 150, 0, 150, 85).mod (env2, "osc1Position", 40).amp (0, 600, 0, 400)
            .filter (1, svfHigh, 300, 10).delay (eighthD, 45, 30, 8000).reverb (30, 70, 3.0f).level (-14).standardMacros (25);
        add ("Wire Pluck", "Pluck").osc (1, string, 60, 90).warp (1, wOff, 40).amp (0, 2000, 0, 300).filter (1, ladder24, 6000, 10)
            .delay (eighthD, 35, 20).level (-15).macro (1, "Bright").mod (macro1, "osc1Position", 40).macro (2, "Grit")
            .drive (fuzz, 0).mod (macro2, "fxDriveAmount", 60).macro (3, "Space").reverb (0).mod (macro3, "fxReverbMix", 45);
        add ("Rubber Pluck", "Pluck").osc (1, analog, squarePos).filter (1, ladder24, 300, 55, 0, 50).env (2, 0, 160, 0, 120, 85)
            .mod (env2, "filter1Cutoff", 45).amp (0, 300, 0, 200).delay (sixteenthD, 35, 20).level (-10).standardMacros (30);
        add ("Bit Pluck", "Pluck").osc (1, analog, sawPos).warp (1, wCrush, 60).filter (1, svfLow, 2000, 30).env (2, 0, 180, 0, 150, 80)
            .mod (env2, "filter1Cutoff", 30).amp (0, 300, 0, 200).delay (eighthT, 40, 25).level (-10).standardMacros (25);
        add ("Dark Kalimba", "Pluck").osc (1, metal, 10).osc (2, analog, sinePos, 70).pitch (2, 12).amp (0, 700, 0, 500)
            .filter (1, ladder24, 2500, 10).reverb (30, 60, 2.5f).level (-9).standardMacros (25);

        //======================================================================
        // Guitar: strings tuned alike and played at once.
        // Wailing Fuzz Lead: a slow, wailing fuzz lead, one note at a time, matched by ear and spectrum to
        // a 2017 industrial ballad's lead: strings that ring on over a sustained wave at the same
        // pitch, fuzz at nearly full, long slides between notes, and a vibrato of about 17 cents at
        // 4 Hz that comes in late, with a slower waver on top.
        add ("Wailing Fuzz Lead", "Guitar").osc (1, string, 62, 90).warp (1, wOff, 100).unison (1, 3, 7, 45, 4, 40)
            .osc (2, analog, 50, 60).unison (2, 2, 6, 30)
            .filter (1, ladder24, 1800, 30, 35, 100).filter (2, svfHigh, 85, 10).drive (fuzz, 90, 9000)
            .amp (3, 600, 92, 350).mono (350).bend (2).lfo (1, sine, 4.0f, 1, 30).lfoFade (1, 450).lfo (2, drift, 1.5f, 0)
            .mod (lfo1, "osc1Fine", 18).mod (lfo1, "osc2Fine", 18).mod (lfo2, "osc1Fine", 8).mod (lfo2, "osc2Fine", 8)
            .reverb (18, 55, 1.8f).level (-12).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25)
            .macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 10).macro (3, "Bright").mod (macro3, "filter1Cutoff", 30)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 45);
        add ("Swarm Drone", "Guitar").osc (1, string, 60, 90).warp (1, wOff, 100).unison (1, 8, 14, 100, 60, 60)
            .osc (2, string, 45, 70).warp (2, wOff, 100).pitch (2, -12).unison (2, 6, 18, 100, 60, 70)
            .osc (3, analog, 50, 45).unison (3, 4, 10, 100, 0, 40)   // the feedback under the strings: it holds as they fade
            .filter (1, ladder24, 3500, 20, 20).lfo (1, drift, 0.08f, 0).mod (lfo1, "filter1Cutoff", 6).drive (fuzz, 60, 4500)
            .amp (200, 1000, 100, 2500).reverb (40, 90, 6.0f, 60).level (-33).macro (1, "Swarm").mod (macro1, "osc1UnisonDrift", 40)
            .macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 35).macro (3, "Bright").mod (macro3, "filter1Cutoff", 25)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Downward Strum", "Guitar").osc (1, string, 75, 90).warp (1, wOff, 40).unison (1, 6, 6, 90, 45, 15)
            .filter (1, ladder24, 4500, 20).drive (fuzz, 55, 5000).amp (0, 2500, 0, 600).delay (quarter, 30, 15).level (-27).standardMacros (25);
        add ("Broken Amp", "Guitar").osc (1, string, 80, 90).warp (1, wOff, 85).unison (1, 3, 5, 60, 12, 20)
            .drive (crush, 55, 4000).filter (1, svfBand, 1800, 30).amp (0, 1800, 40, 400).level (-7).standardMacros (25);
        add ("Feedback Choir", "Guitar").osc (1, string, 50, 90).warp (1, wOff, 100).unison (1, 6, 7, 100, 30, 50)
            .osc (2, analog, 50, 50).unison (2, 4, 7, 100, 0, 50)   // the feedback that holds as the strings fade
            .filter (1, ladderBand, 1500, 55, 30).lfo (1, sine, 0.12f, 0).mod (lfo1, "filter1Cutoff", 12).drive (fuzz, 75, 3500)
            .amp (300, 1000, 100, 2000).reverb (45, 90, 5.0f).level (-31).standardMacros (25);
        add ("Baritone Riff", "Guitar").osc (1, string, 65, 90).warp (1, wOff, 85).pitch (1, -12).unison (1, 2, 5, 40, 8, 10)
            .subOsc (20).filter (1, ladder24, 3000, 20).drive (fuzz, 65, 4000).amp (0, 1500, 50, 200).mono (0, false).level (-23)
            .standardMacros (25);

        // The early-90s EBM synth guitar: a power chord (root, fifth, octave) from one key, clipped
        // hard and left bright, its spectrum matched to a recording of one: nearly flat to 4 kHz,
        // a strong fifth, a quiet octave, and the low end thinned so the bass has room.
        add ("Body Riff Power Chord", "Guitar").osc (1, analog, sawPos).unison (1, 2, 8, 50).osc (2, analog, sawPos, 0).pitch (2, 7)
            .osc (3, analog, sawPos, 12).pitch (3, 12).env (2, 0, 90, 0, 80, 85).mod (env2, "osc2Position", 30)
            .filter (1, ladder24, 20000, 20, 30, 30).filter (2, svfHigh, 200, 10).drive (hard, 90, 18000).amp (1, 900, 60, 150)
            .reverb (12, 40, 1.2f).level (-31).macro (1, "Mute").mod (macro1, "filter1Cutoff", -30).macro (2, "Fuzz")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Fifth", 100).mod (macro3, "osc2Volume", 35).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Palm Mute Chug", "Guitar").osc (1, analog, sawPos).unison (1, 2, 6, 40).osc (2, sync, 30, 70).pitch (2, 7)
            .osc (3, analog, squarePos, 45).pitch (3, -12).filter (1, ladder24, 900, 30, 40, 30).env (2, 0, 120, 0, 90, 85)
            .mod (env2, "filter1Cutoff", 30).mod (env2, "osc2Position", 25).mod (velocity, "filter1Cutoff", 12)
            .filter (2, svfHigh, 90, 10).drive (hard, 75, 3800).amp (0, 190, 0, 70).level (-19).macro (1, "Open")
            .mod (macro1, "filter1Cutoff", 35).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 25).macro (3, "Space").reverb (0, 40, 1.0f)
            .mod (macro3, "fxReverbMix", 35);
        add ("Barbed Riff", "Guitar").osc (1, sync, 35).unison (1, 3, 10, 60).osc (2, analog, sawPos, 60).pitch (2, -12)
            .env (2, 0, 160, 15, 120, 80).mod (env2, "osc1Position", 35).filter (1, ladder24, 3200, 25, 30, 30)
            .filter (2, svfHigh, 120, 10).drive (fuzz, 70, 4500).mono (0, false).amp (1, 600, 70, 120).delay (sixteenthD, 25, 12, 3000)
            .level (-20).macro (1, "Bite").mod (macro1, "osc1Position", 40).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30)
            .macro (3, "Tone").mod (macro3, "filter1Cutoff", 30).macro (4, "Space").reverb (0, 50, 1.5f).mod (macro4, "fxReverbMix", 40);

        //======================================================================
        // Pads.
        add ("Concrete Pad", "Pad").osc (1, analog, sawPos).osc (2, analog, sawPos).pitch (2, 0, 8).unison (1, 4, 18, 90).unison (2, 4, 14, 90)
            .filter (1, ladder24, 1200, 20).lfo (1, sine, 0.08f, 0).mod (lfo1, "filter1Cutoff", 8).amp (800, 1000, 90, 1800)
            .chorus (30).reverb (40, 90, 5.0f).level (-11).standardMacros (25);
        add ("Ashfall", "Pad").osc (1, vowel, 40).unison (1, 6, 20, 100).lfoSync (1, triangle, bars4, 0).mod (lfo1, "osc1Position", 30)
            .filter (1, svfLow, 1800, 25).noise (8, 1, -40).amp (1200, 1000, 90, 2500).reverb (45, 95, 7.0f).level (-10).standardMacros (25);
        add ("Cathedral Of Rust", "Pad").osc (1, additive, 50).osc (2, metal, 30, 40).pitch (2, 12).unison (1, 4, 10, 100)
            .lfo (1, drift, 0.1f, 0).mod (lfo1, "osc1Position", 20).filter (1, ladder12, 2500, 15).amp (1500, 1000, 90, 3000)
            .reverb (50, 100, 9.0f).level (-11).standardMacros (25);
        add ("Low Ceiling", "Pad").osc (1, analog, trianglePos).osc (2, analog, sawPos, 50).pitch (2, -12).unison (2, 3, 12, 80)
            .filter (1, ladder24, 500, 30, 20).lfoSync (1, sine, bars2, 0).mod (lfo1, "filter1Cutoff", 10).amp (600, 1000, 90, 1500)
            .phaser (35, 0.08f, 50).reverb (35, 80, 4.0f).level (-9).standardMacros (25);
        add ("Grey Sky", "Pad").osc (1, pulse, 50).unison (1, 5, 14, 100).lfo (1, triangle, 0.2f, 0).mod (lfo1, "osc1Position", 25)
            .filter (1, ladder24, 1500, 20).amp (900, 1000, 90, 2000).chorus (35).reverb (40, 90, 5.0f).level (-7).standardMacros (25);
        add ("Sodium Lamp", "Pad").osc (1, fm, 15).osc (2, additive, 20, 60).unison (2, 4, 10, 90).lfo (1, sine, 0.1f, 0)
            .mod (lfo1, "osc1Position", 15).filter (1, svfLow, 2200, 15).amp (1000, 1000, 90, 2500).reverb (45, 90, 6.0f).level (-15)
            .standardMacros (25);
        add ("Tension Choir", "Pad").osc (1, vowel, 60).osc (2, vowel, 20).pitch (2, 0, 10).unison (1, 4, 12, 100).unison (2, 4, 12, 100)
            .lfo (1, sine, 0.07f, 0).mod (lfo1, "osc1Position", 25).mod (lfo1, "osc2Position", -25).filter (1, ladder12, 3000, 10)
            .amp (1500, 1000, 90, 3000).reverb (50, 100, 8.0f).level (-12).standardMacros (25);
        add ("Fog Machine", "Pad").noise (60, 1, -30).osc (1, analog, sinePos, 40).filter (1, svfBand, 800, 50).lfo (1, drift, 0.15f, 0)
            .mod (lfo1, "filter1Cutoff", 15).amp (2000, 1000, 100, 3000).reverb (55, 100, 9.0f).level (0).standardMacros (25);

        //======================================================================
        // Drones: hold one note for a minute.
        add ("Reactor Drone", "Drone").osc (1, growl, 30).osc (2, analog, sawPos, 60).pitch (2, -12, 4).unison (1, 4, 14, 100)
            .lfo (1, drift, 0.05f, 0).mod (lfo1, "osc1Position", 30).lfo (2, sine, 0.03f, 0).mod (lfo2, "filter1Cutoff", 10)
            .filter (1, ladder24, 700, 40, 40).amp (2000, 1000, 100, 4000).reverb (40, 100, 8.0f).level (-11).standardMacros (25);
        add ("Turbine", "Drone").osc (1, metal, 40).noise (25, 1).filter (1, comb, 110, 85).lfo (1, sine, 0.07f, 0)
            .mod (lfo1, "filter1Cutoff", 5).amp (1500, 1000, 100, 3000).reverb (35, 90, 6.0f).level (-11).standardMacros (15);
        add ("Substation", "Drone").osc (1, sub, 40).osc (2, sync, 40, 40).pitch (2, 12).lfo (1, triangle, 0.1f, 0).mod (lfo1, "osc2Position", 40)
            .filter (1, ladder24, 600, 50, 50).amp (1500, 1000, 100, 3000).reverb (30, 80, 5.0f).level (-13).standardMacros (25);
        add ("Engine Room", "Drone").osc (1, analog, sawPos).unison (1, 8, 40, 100).filter (1, ladder24, 300, 60, 60)
            .lfoSync (1, square, eighth, 0, 60).mod (lfo1, "filter1Cutoff", 10).amp (1000, 1000, 100, 2500).drive (hard, 30, 3000)
            .level (-20).standardMacros (25);
        add ("Void Hum", "Drone").osc (1, analog, sinePos).osc (2, analog, sinePos, 60).pitch (2, 0, 3).osc (3, additive, 20, 30).pitch (3, 19)
            .lfo (1, drift, 0.05f, 0).mod (lfo1, "osc3Position", 30).amp (3000, 1000, 100, 5000).reverb (50, 100, 12.0f).level (-11)
            .standardMacros (20);
        add ("Power Line", "Drone").osc (1, pulse, 80).osc (2, pulse, 75).pitch (2, 0, 2).filter (1, svfBand, 1200, 70)
            .noise (10, 3).amp (500, 1000, 100, 1500).drive (crush, 20).level (-6).standardMacros (20);

        //======================================================================
        // Bells and mallets, metallic and cold.
        add ("Cold Bell", "Bell").osc (1, metal, 30).amp (0, 3000, 0, 2500, 70).filter (1, svfHigh, 200, 10).reverb (40, 90, 5.0f)
            .level (-9).standardMacros (25);
        add ("Prison Bell", "Bell").osc (1, metal, 70).osc (2, fm, 50, 50).pitch (2, 12).amp (0, 4000, 0, 3000, 70)
            .env (2, 0, 1500, 0, 1500, 70).mod (env2, "osc2Position", 40).reverb (45, 100, 7.0f).level (-7).standardMacros (25);
        add ("Rusted Chime", "Bell").osc (1, metal, 50).warp (1, wFold, 25).amp (0, 2500, 0, 2000, 70).filter (1, ladder12, 5000, 20)
            .delay (quarterD, 45, 25, 4000).reverb (40, 80, 4.0f).level (-9).standardMacros (25);
        add ("FM Toll", "Bell").osc (1, fm, 80).pitch (1, 0).env (2, 0, 2000, 0, 2000, 70).mod (env2, "osc1Position", 50)
            .amp (0, 3500, 0, 3000, 70).reverb (40, 100, 6.0f).level (-10).standardMacros (25);
        add ("Dead Glockenspiel", "Bell").osc (1, fm, 40).osc (2, metal, 20, 50).pitch (2, 24).amp (0, 1200, 0, 800, 80)
            .filter (1, svfHigh, 400, 10).delay (eighthD, 30, 20, 6000).level (-6).standardMacros (25);
        add ("Minor Bell Motif", "Bell").osc (1, metal, 40).osc (2, analog, sinePos, 50).pitch (2, 12).amp (0, 1800, 0, 1500, 70)
            .delay (eighthD, 40, 25, 4500).reverb (35, 80, 4.0f).level (-7).standardMacros (25);
        add ("Gamelan Machine", "Bell").osc (1, metal, 90).warp (1, wMirror, 30).amp (0, 2000, 0, 1500, 70).filter (1, svfBand, 2000, 20)
            .reverb (35, 70, 3.0f).level (-5).standardMacros (25);

        //======================================================================
        // Sequences: patches with motion built in, for one held note or a sparse pattern.
        add ("Gated Machine", "Sequence").osc (1, analog, sawPos).unison (1, 3, 12, 70).filter (1, ladder24, 1500, 40)
            .lfoSync (1, steps, bar1, 1).stepPattern (1, { 100, -100, 60, -100, 100, -100, -100, 80, 100, -100, 60, -100, 100, 40, -100, -100 })
            .mod (lfo1, "ampLevel", 35).mod (lfo1, "filter1Cutoff", 10).amp (2, 300, 90, 200).delay (sixteenthD, 30, 15).level (-27)
            .standardMacros (25);
        add ("Trance Gate Industrial", "Sequence").osc (1, growl, 40).unison (1, 4, 16, 90).filter (1, ladder24, 1800, 35, 30)
            .lfoSync (1, square, sixteenth, 1, 15).mod (lfo1, "ampLevel", 35).amp (10, 300, 90, 300).reverb (25).level (-26).standardMacros (25);
        add ("Clockwork", "Sequence").osc (1, metal, 50).lfoSync (1, steps, bar1, 1)
            .stepPattern (1, { 100, -60, 20, -60, 80, -60, 40, -60, 100, -60, 20, -60, 60, -20, -60, -100 }).mod (lfo1, "osc1Position", 40)
            .filter (1, svfBand, 2000, 40).mod (lfo1, "filter1Cutoff", 15).amp (0, 1000, 70, 300).delay (sixteenthD, 40, 20).level (-4)
            .standardMacros (25);
        add ("Wobble Press", "Sequence").osc (1, analog, sawPos).osc (2, growl, 50, 60).subOsc (40).filter (1, ladder24, 300, 50, 40)
            .lfo (1, sine, 4.0f, 1).mod (lfo1, "filter1Cutoff", 25).mono().amp (2, 300, 90, 100).drive (hard, 30).level (-14)
            .macro (1, "Rate").mod (macro1, "lfo1Rate", 30).macro (2, "Depth").mod (macro2, "filter1Cutoff", 20);
        add ("Pump Chord", "Sequence").osc (1, analog, sawPos).osc (2, analog, sawPos).pitch (2, 7).unison (1, 3, 12, 80)
            .filter (1, ladder24, 1800, 20).lfoSync (1, sawUp, quarter, 1).mod (lfo1, "ampLevel", 25).amp (5, 300, 90, 300)
            .reverb (25).level (-22).standardMacros (25);
        add ("Morse Code", "Sequence").osc (1, analog, squarePos).filter (1, svfBand, 1500, 50).lfoSync (1, steps, bar1, 1)
            .stepPattern (1, { 100, 100, -100, 100, -100, -100, 100, 100, 100, -100, 100, -100, -100, -100, 100, -100 })
            .mod (lfo1, "ampLevel", 40).amp (2, 300, 90, 100).delay (eighthD, 30, 20).level (-29).standardMacros (20);

        //======================================================================
        // Risers and fallers.
        add ("Tension Riser", "Riser").noise (70, 0).osc (1, analog, sawPos, 40).unison (1, 6, 30, 100).filter (1, svfBand, 300, 40)
            .env (2, 6000, 1000, 100, 500, -50).mod (env2, "filter1Cutoff", 60).mod (env2, "osc1Pitch", 25).amp (4000, 1000, 100, 800)
            .reverb (40, 90, 5.0f).level (4).standardMacros (25);
        add ("Siren Climb", "Riser").osc (1, sync, 30).env (2, 5000, 1000, 100, 500, -50).mod (env2, "osc1Pitch", 50)
            .mod (env2, "osc1Position", 60).filter (1, ladder24, 3000, 30).amp (500, 1000, 100, 600).delay (sixteenthD, 50, 30)
            .level (-10).standardMacros (25);
        add ("Falling Note Ramp", "Riser").osc (1, analog, sawPos).unison (1, 3, 15, 80).env (2, 0, 2500, 0, 2500, 60)
            .mod (env2, "osc1Pitch", 50).filter (1, ladder24, 2500, 30).amp (0, 3000, 0, 1500).reverb (35, 80, 4.0f).level (-10)
            .standardMacros (25);
        add ("Downlifter Noise", "Riser").set ("osc1On", 0).noise (80, 0).filter (1, svfBand, 6000, 50).env (2, 0, 3000, 0, 3000, 60)
            .mod (env2, "filter1Cutoff", 50).amp (0, 3500, 0, 2000).reverb (40, 90, 5.0f).set ("filter1Cutoff", 200).level (-2)
            .standardMacros (25);
        add ("Machine Spin Up", "Riser").osc (1, metal, 30).warp (1, wFm, 50).lfo (1, sawUp, 2, 1).mod (lfo1, "osc1Position", 40)
            .env (2, 4000, 1000, 100, 300, -40).mod (env2, "lfo1Rate", 50).mod (env2, "osc1Pitch", 25).amp (300, 1000, 100, 500)
            .level (-7).standardMacros (25);

        //======================================================================
        // Impacts: one hit and a tail.
        add ("Low Boom", "Impact").osc (1, analog, sinePos).subOsc (60).env (2, 0, 400, 0, 400, 80).mod (env2, "osc1Pitch", 50)
            .noise (30, 1).filter (1, ladder24, 500, 20).amp (0, 2500, 0, 2000, 60).reverb (40, 100, 6.0f, 70).level (-12).standardMacros (20);
        add ("Steel Door", "Impact").osc (1, metal, 80).noise (40, 2).filter (1, comb, 90, 80).env (2, 0, 300, 0, 300, 80)
            .mod (env2, "filter1Cutoff", 10).amp (0, 1500, 0, 1500, 70).reverb (45, 90, 5.0f).level (-5).standardMacros (15);
        add ("Hydraulic Slam", "Impact").noise (70, 0).osc (1, sub, 30).env (2, 0, 250, 0, 250, 85).mod (env2, "osc1Pitch", 40)
            .filter (1, ladder24, 1500, 30, 50).mod (env2, "filter1Cutoff", 30).amp (0, 900, 0, 900).drive (hard, 40).reverb (30, 70, 2.5f)
            .level (-20).standardMacros (20);
        add ("Shatter", "Impact").noise (60, 3).osc (1, metal, 100).warp (1, wCrush, 60).filter (1, svfHigh, 1500, 30).amp (0, 800, 0, 800, 75)
            .reverb (40, 80, 3.0f).level (-4).standardMacros (20);
        add ("Distant Explosion", "Impact").noise (80, 1, -60).osc (1, sub, 0, 70).env (2, 0, 800, 0, 800, 80).mod (env2, "osc1Pitch", 30)
            .filter (1, ladder24, 800, 15).mod (env2, "filter1Cutoff", 20).amp (5, 3000, 0, 3000, 60).reverb (50, 100, 9.0f, 80).level (-10)
            .standardMacros (20);
        add ("Gunmetal Hit", "Impact").osc (1, fm, 90).osc (2, analog, squarePos, 50).pitch (2, -12).env (2, 0, 120, 0, 120, 90)
            .mod (env2, "osc1Position", -50).amp (0, 700, 0, 600).drive (fuzz, 50, 5000).reverb (30, 60, 2.0f).level (-33).standardMacros (20);

        //======================================================================
        // Textures: noise and grit for the space between hits.
        add ("Radio Static", "Texture").set ("osc1On", 0).noise (80, 3).filter (1, svfBand, 2500, 40).lfo (1, randomShape, 8, 0, 50).mod (lfo1, "filter1Cutoff", 15)
            .amp (50, 1000, 100, 500).drive (crush, 40).level (-4).standardMacros (25);
        add ("Vinyl Crackle Bed", "Texture").noise (70, 2).filter (1, svfLow, 6000, 10).osc (1, analog, sinePos, 15).amp (100, 1000, 100, 800)
            .level (0).standardMacros (25);
        add ("Ventilation", "Texture").set ("osc1On", 0).noise (80, 1, -50).filter (1, svfBand, 600, 30).lfo (1, drift, 0.1f, 0).mod (lfo1, "filter1Cutoff", 20)
            .amp (1500, 1000, 100, 2000).reverb (35, 90, 4.0f).level (2).standardMacros (25);
        add ("Circuit Bent", "Texture").osc (1, sync, 50).warp (1, wCrush, 70).lfo (1, randomShape, 12, 0).mod (lfo1, "osc1Position", 50)
            .mod (lfo1, "osc1Pitch", 10).filter (1, svfBand, 2000, 40).amp (2, 300, 80, 200).delay (sixteenthT, 45, 30).level (-9)
            .standardMacros (25);
        add ("Geiger", "Texture").set ("osc1On", 0).noise (100, 2).filter (1, svfHigh, 2000, 30).amp (0, 1000, 100, 300).delay (sixteenthD, 40, 30)
            .reverb (25).level (-5).standardMacros (25);
        add ("Ghost Signal", "Texture").osc (1, vowel, 50).warp (1, wFm, 40).lfo (1, drift, 0.3f, 0).mod (lfo1, "osc1Position", 40)
            .lfo (2, randomShape, 3, 0, 60).mod (lfo2, "osc1Pitch", 1.0f).filter (1, svfBand, 1500, 40).amp (500, 1000, 100, 2000)
            .delay (quarterD, 55, 35, 3000).reverb (45, 100, 7.0f).level (-12).standardMacros (25);

        //======================================================================
        // Three layers: every one of these plays all three oscillators, each a different wave with a level
        // envelope of its own, so a layer strikes and falls away, or swells in late, under the others.
        // Each comes after its category's other hand-made patches.
        add ("Three Stage Bass", "Bass").osc (1, analog, sawPos, 85).osc (2, pulse, 35, 70).pitch (2, -12).osc (3, fm, 60, 60).pitch (3, 12)
            .levelEnv (1, 0, 40, 500, 45).levelEnv (3, 0, 0, 70, 0).filter (1, ladder24, 420, 35, 25, 30).env (2, 0, 180, 0, 120, 70)
            .mod (env2, "filter1Cutoff", 32).amp (0, 400, 80, 60).mono (0, false).level (-5).standardMacros (28).macro (4, "Bite").mod (macro4, "osc3Volume", 40);
        add ("Girder Bass", "Bass").osc (1, growl, 40, 80).osc (2, analog, squarePos, 75).pitch (2, -12).osc (3, metal, 30, 55).pitch (3, 12)
            .levelEnv (3, 0, 0, 110, 0).levelEnv (1, 0, 0, 700, 60).filter (1, ladder12, 600, 25, 35, 40).env (2, 0, 240, 0, 150, 70).mod (env2, "filter1Cutoff", 26)
            .mod (env2, "osc1Position", 30).amp (0, 500, 75, 70).mono (0, false).drive (hard, 35, 5000).level (-15).standardMacros (25).macro (4, "Clang").mod (macro4, "osc3Volume", 45);
        add ("Undertow Bass", "Bass").osc (1, sub, 30, 90).osc (2, analog, sawPos, 70).unison (2, 3, 14, 60).osc (3, vowel, 50, 60)
            .levelEnv (3, 350, 0, 2000, 100).levelEnv (2, 0, 0, 900, 50).lfo (1, sine, 0.3f, 0).mod (lfo1, "osc3Position", 30).filter (1, ladder24, 500, 30, 20, 30)
            .env (2, 0, 300, 20, 200, 60).mod (env2, "filter1Cutoff", 22).amp (2, 600, 85, 150).mono (60).level (-4).standardMacros (25).macro (4, "Voice").mod (macro4, "osc3Volume", 40);
        add ("Detonator Bass", "Bass").osc (1, sync, 30, 80).warp (1, wSync, 40).osc (2, fm, 70, 70).osc (3, analog, sinePos, 90).pitch (3, -12)
            .levelEnv (2, 0, 0, 120, 20).env (2, 0, 220, 0, 150, 75).mod (env2, "osc1WarpAmount", 45).filter (1, ladder24, 900, 20, 30, 30).mod (env2, "filter1Cutoff", 20)
            .levelEnv (1, 0, 30, 600, 55).amp (0, 350, 80, 70).mono (0, false).drive (tube, 30, 6000).level (-12).standardMacros (25).macro (4, "Rip").mod (macro4, "osc1WarpAmount", 40);
        add ("Rivet Bass", "Bass").osc (1, pulse, 20, 85).osc (2, pulse, 80, 60).pitch (2, 7).osc (3, additive, 40, 80).pitch (3, -12)
            .levelEnv (2, 0, 0, 90, 0).levelEnv (3, 0, 0, 400, 70).filter (1, ladder24, 700, 30, 30, 40).env (2, 0, 110, 0, 90, 80).mod (env2, "filter1Cutoff", 30)
            .amp (0, 200, 60, 50).mono (0, false).level (-4).standardMacros (28).macro (4, "Fifth").mod (macro4, "osc2Volume", 40);
        add ("Slow Jaw Bass", "Bass").osc (1, growl, 20, 80).osc (2, vowel, 30, 60).osc (3, analog, sawPos, 80).pitch (3, -12)
            .levelEnv (2, 500, 0, 3000, 100).levelEnv (3, 0, 0, 1200, 60).env (3, 400, 1500, 60, 400, 30).mod (env3, "osc1Position", 45).mod (env3, "osc2Position", 40)
            .filter (1, ladder12, 800, 30, 30, 30).mod (env3, "filter1Cutoff", 14).amp (5, 800, 90, 200).mono (80).drive (tube, 25, 5000).level (-11)
            .standardMacros (25).macro (4, "Chew").mod (macro4, "osc1Position", 40);

        add ("Triple Reese", "Reese").osc (1, analog, sawPos, 80).pitch (1, 0, -19).osc (2, analog, sawPos, 80).pitch (2, 0, 19).osc (3, sync, 40, 70).warp (3, wSync, 30).pitch (3, -12)
            .levelEnv (3, 400, 0, 3000, 100).lfo (1, sine, 0.15f, 0).mod (lfo1, "filter1Cutoff", 10).lfo (2, triangle, 0.09f, 0).mod (lfo2, "osc3WarpAmount", 45)
            .filter (1, ladder24, 650, 28, 30).amp (4, 800, 95, 250).mono (70).phaser (35, 0.12f).level (-4).standardMacros (25).macro (4, "Beat").mod (macro4, "osc2Fine", 30);
        add ("Vowel Reese", "Reese").osc (1, analog, sawPos, 80).pitch (1, 0, -14).osc (2, analog, sawPos, 80).pitch (2, 0, 14).osc (3, vowel, 20, 65)
            .levelEnv (3, 700, 0, 4000, 100).lfo (1, triangle, 0.2f, 0).mod (lfo1, "osc3Position", 45).lfo (2, sine, 0.11f, 0).mod (lfo2, "filter1Cutoff", 9)
            .filter (1, ladder24, 800, 25, 25).amp (10, 900, 95, 300).mono (90).level (-4).standardMacros (25).macro (4, "Talk").mod (macro4, "osc3Volume", 40);
        add ("Metal Reese", "Reese").osc (1, growl, 35, 75).unison (1, 2, 22, 70).osc (2, analog, sawPos, 80).pitch (2, 0, 16).osc (3, metal, 40, 50).pitch (3, 12)
            .levelEnv (3, 0, 0, 300, 15).levelEnv (1, 200, 0, 3000, 100).lfo (1, sine, 0.18f, 0).mod (lfo1, "osc1Position", 30).mod (lfo1, "filter1Cutoff", 8)
            .filter (1, ladder12, 900, 25, 35).amp (3, 800, 95, 250).mono (60).drive (tube, 30, 5000).level (-13).standardMacros (25).macro (4, "Edge").mod (macro4, "osc3Volume", 45);

        add ("Stacked Acid", "Acid").osc (1, analog, sawPos, 85).osc (2, pulse, 22, 65).pitch (2, 7).osc (3, sync, 50, 55).warp (3, wSync, 50)
            .levelEnv (3, 0, 0, 140, 0).levelEnv (2, 0, 0, 260, 10).filter (1, ladder24, 380, 72, 40).env (2, 0, 200, 0, 130, 80).mod (env2, "filter1Cutoff", 42)
            .mod (velocity, "filter1Cutoff", 14).mod (env2, "osc3WarpAmount", 40).amp (0, 300, 70, 60).mono (50).drive (tube, 35, 6000).delay (sixteenthD, 38, 22, 3500).level (-13)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30).macro (2, "Reso").mod (macro2, "filter1Resonance", 25).macro (3, "Drive").mod (macro3, "fxDriveAmount", 40)
            .macro (4, "Zap").mod (macro4, "osc3Volume", 45);
        add ("Acid Choir", "Acid").osc (1, analog, sawPos, 80).osc (2, vowel, 60, 60).osc (3, fm, 40, 50).pitch (3, 12)
            .levelEnv (3, 0, 0, 80, 0).levelEnv (2, 120, 0, 1500, 70).env (3, 0, 600, 20, 200, 50).mod (env3, "osc2Position", 45).filter (1, ladder24, 450, 68, 35)
            .env (2, 0, 240, 0, 150, 80).mod (env2, "filter1Cutoff", 38).mod (velocity, "filter1Cutoff", 12).amp (0, 400, 70, 80).mono (40).delay (eighthD, 30, 18, 4000)
            .level (-4).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30).macro (2, "Reso").mod (macro2, "filter1Resonance", 25).macro (3, "Voice").mod (macro3, "osc2Volume", 40)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 40);

        add ("Tri Blade Lead", "Lead").osc (1, sync, 40, 80).warp (1, wSync, 35).osc (2, analog, sawPos, 70).unison (2, 3, 12, 70).osc (3, pulse, 30, 55).pitch (3, 12)
            .levelEnv (3, 0, 0, 250, 30).env (2, 0, 500, 30, 200, 60).mod (env2, "osc1WarpAmount", 35).lfo (1, sine, 5.5f, 1).lfoFade (1, 600).mod (lfo1, "osc1Fine", 8)
            .mod (lfo1, "osc2Fine", 8).filter (1, ladder12, 2400, 20, 20, 50).amp (3, 500, 85, 180).mono (40).delay (eighthD, 30, 20, 5000).reverb (18)
            .level (-5).standardMacros (25).macro (4, "Sync").mod (macro4, "osc1WarpAmount", 40);
        add ("Glass Engine Lead", "Lead").osc (1, fm, 55, 80).osc (2, metal, 60, 50).pitch (2, 12).osc (3, analog, sinePos, 80).pitch (3, -12)
            .levelEnv (2, 0, 0, 180, 10).levelEnv (1, 0, 60, 900, 65).env (2, 0, 700, 20, 300, 60).mod (env2, "osc1Position", 35).lfo (1, sine, 5.0f, 1).lfoFade (1, 500)
            .mod (lfo1, "osc1Fine", 7).filter (1, svfLow, 4200, 15, 0, 50).amp (2, 600, 80, 220).mono (30).delay (quarter, 30, 22, 5000).reverb (22)
            .level (-9).standardMacros (25).macro (4, "Glass").mod (macro4, "osc2Volume", 45);
        add ("Swell Lead", "Lead").osc (1, analog, sawPos, 80).osc (2, vowel, 40, 70).osc (3, additive, 70, 60).pitch (3, 12)
            .levelEnv (2, 600, 0, 4000, 100).levelEnv (3, 1200, 0, 4000, 100).levelEnv (1, 0, 0, 1800, 50).lfo (1, sine, 0.25f, 0).mod (lfo1, "osc2Position", 35)
            .lfo (2, sine, 5.2f, 1).lfoFade (2, 900).mod (lfo2, "osc1Fine", 8).filter (1, ladder12, 3000, 18, 15, 40).amp (5, 700, 90, 300).mono (70)
            .reverb (28, 70, 3.0f).level (-4).standardMacros (25).macro (4, "Bloom").mod (macro4, "osc3Volume", 40);
        add ("Split Field Lead", "Lead").osc (1, pulse, 35, 75).pan (1, -70).osc (2, pulse, 65, 75).pan (2, 70).pitch (2, 0, 9).osc (3, growl, 30, 70)
            .levelEnv (3, 0, 0, 400, 40).env (2, 0, 450, 30, 200, 60).mod (env2, "osc3Position", 40).lfo (1, triangle, 0.4f, 0).mod (lfo1, "osc1Position", 25)
            .mod (lfo1, "osc2Position", -25).filter (1, ladder24, 2200, 25, 25, 50).mod (env2, "filter1Cutoff", 16).amp (2, 500, 85, 160).mono (35)
            .delay (eighth, 28, 18, 5000).level (-4).standardMacros (25).macro (4, "Growl").mod (macro4, "osc3Volume", 40);
        add ("Howl Stack Lead", "Lead").osc (1, growl, 60, 80).osc (2, sync, 30, 60).warp (2, wSync, 55).pitch (2, 7).osc (3, analog, sinePos, 75).pitch (3, -12)
            .levelEnv (2, 0, 0, 350, 25).env (2, 30, 900, 40, 300, 50).mod (env2, "osc1Position", 35).mod (env2, "osc2WarpAmount", 30).lfo (1, sine, 5.8f, 1).lfoFade (1, 400)
            .mod (lfo1, "osc1Fine", 10).filter (1, ladder24, 1800, 30, 35, 50).mod (env2, "filter1Cutoff", 14).amp (4, 700, 85, 250).mono (120)
            .drive (fuzz, 35, 5000).reverb (20).level (-21).standardMacros (25).macro (4, "Howl").mod (macro4, "osc1Position", 35);
        add ("Wire Chorus Lead", "Lead").osc (1, analog, sawPos, 75).unison (1, 2, 10, 60).osc (2, analog, squarePos, 55).pitch (2, 12).osc (3, fm, 30, 50).pitch (3, 19)
            .levelEnv (3, 0, 0, 220, 0).levelEnv (2, 80, 0, 1200, 60).lfo (1, sine, 5.3f, 1).lfoFade (1, 700).mod (lfo1, "osc1Fine", 7).filter (1, ladder12, 2800, 20, 20, 60)
            .env (2, 0, 400, 30, 200, 60).mod (env2, "filter1Cutoff", 14).amp (2, 500, 85, 200).mono (45).chorus (35).delay (eighthD, 30, 20, 5000)
            .level (-2).standardMacros (25).macro (4, "Ping").mod (macro4, "osc3Volume", 45);

        add ("Triad Engine Stab", "Stab").osc (1, analog, sawPos, 80).osc (2, pulse, 40, 75).pitch (2, 3).osc (3, fm, 50, 70).pitch (3, 7)
            .levelEnv (1, 0, 0, 260, 0).levelEnv (2, 0, 30, 340, 0).levelEnv (3, 0, 60, 420, 0).filter (1, ladder24, 900, 30, 25).env (2, 0, 220, 0, 150, 70)
            .mod (env2, "filter1Cutoff", 34).amp (0, 500, 0, 200).delay (eighthD, 30, 18, 4500).reverb (20).level (-10).standardMacros (28)
            .macro (4, "Top").mod (macro4, "osc3Volume", 40);
        add ("Fifth Hammer Stab", "Stab").osc (1, growl, 30, 80).osc (2, analog, sawPos, 75).pitch (2, 7).osc (3, metal, 45, 55).pitch (3, 12)
            .levelEnv (3, 0, 0, 120, 0).levelEnv (2, 0, 0, 380, 0).levelEnv (1, 0, 0, 500, 0).filter (1, ladder12, 1400, 25, 35).env (2, 0, 180, 0, 150, 70)
            .mod (env2, "filter1Cutoff", 30).amp (0, 600, 0, 220).drive (hard, 30, 6000).reverb (22, 60, 2.2f).level (-19).standardMacros (25)
            .macro (4, "Anvil").mod (macro4, "osc3Volume", 45);
        add ("Minor Ninth Stab", "Stab").osc (1, analog, sawPos, 80).unison (1, 2, 10, 60).osc (2, vowel, 55, 70).pitch (2, 3).osc (3, analog, squarePos, 60).pitch (3, 14)
            .levelEnv (2, 0, 0, 450, 0).levelEnv (3, 0, 0, 200, 0).filter (1, ladder24, 1100, 28, 20).env (2, 0, 260, 0, 180, 70).mod (env2, "filter1Cutoff", 32)
            .mod (env2, "osc2Position", 30).amp (0, 550, 0, 250).chorus (30).delay (eighthD, 32, 20, 4500).level (-8).standardMacros (28)
            .macro (4, "Ninth").mod (macro4, "osc3Volume", 45);
        add ("Pipe Organ Hit", "Stab").osc (1, additive, 60, 80).osc (2, additive, 30, 70).pitch (2, 12).osc (3, pulse, 50, 55).pitch (3, -12)
            .levelEnv (2, 0, 0, 300, 20).levelEnv (3, 0, 0, 600, 50).env (2, 0, 300, 0, 200, 60).mod (env2, "osc1Position", 30).filter (1, svfLow, 3200, 15)
            .amp (2, 700, 30, 300).reverb (30, 80, 3.5f).level (-12).standardMacros (25).macro (4, "Pedal").mod (macro4, "osc3Volume", 40);

        add ("Layer Pluck", "Pluck").osc (1, fm, 50, 70).pitch (1, 12).osc (2, analog, sawPos, 80).osc (3, analog, sinePos, 80)
            .levelEnv (1, 0, 0, 60, 0).levelEnv (2, 0, 0, 350, 0).levelEnv (3, 0, 0, 2400, 0).filter (1, ladder24, 1600, 22, 15, 50).env (2, 0, 200, 0, 200, 70)
            .mod (env2, "filter1Cutoff", 30).amp (0, 2500, 0, 600).delay (eighthD, 32, 22, 5000).reverb (22).level (-11).standardMacros (28)
            .macro (4, "Body").mod (macro4, "osc3Volume", 40);
        add ("Mallet Stack", "Pluck").osc (1, metal, 20, 60).pitch (1, 12).osc (2, additive, 30, 80).osc (3, sub, 50, 75).pitch (3, -12)
            .levelEnv (1, 0, 0, 140, 0).levelEnv (2, 0, 0, 700, 10).levelEnv (3, 0, 0, 500, 0).filter (1, svfLow, 3500, 12, 0, 60).env (2, 0, 300, 0, 200, 70)
            .mod (env2, "filter1Cutoff", 16).mod (velocity, "osc1Volume", 30).amp (0, 1500, 0, 500).reverb (26, 65, 2.4f).level (-13).standardMacros (25)
            .macro (4, "Strike").mod (macro4, "osc1Volume", 45);
        add ("Wide Triple Pluck", "Pluck").osc (1, pulse, 30, 75).pan (1, -60).osc (2, pulse, 70, 75).pan (2, 60).pitch (2, 12).osc (3, growl, 25, 70)
            .levelEnv (1, 0, 0, 300, 0).levelEnv (2, 0, 40, 260, 0).levelEnv (3, 0, 0, 500, 15).filter (1, ladder24, 1300, 30, 25, 40).env (2, 0, 160, 0, 150, 75)
            .mod (env2, "filter1Cutoff", 34).mod (env2, "osc3Position", 30).amp (0, 900, 0, 350).delay (sixteenthD, 35, 22, 5000).level (-11).standardMacros (28)
            .macro (4, "Octave").mod (macro4, "osc2Volume", 40);
        add ("Glass Fork Pluck", "Pluck").osc (1, fm, 75, 75).osc (2, metal, 70, 45).pitch (2, 19).osc (3, analog, trianglePos, 80)
            .levelEnv (2, 0, 0, 90, 0).levelEnv (1, 0, 0, 500, 0).levelEnv (3, 0, 0, 1800, 0).env (2, 0, 400, 0, 200, 70).mod (env2, "osc1Position", -35)
            .filter (1, svfLow, 6000, 10, 0, 50).amp (0, 2000, 0, 800).delay (quarter, 30, 20, 6000).reverb (30, 75, 3.0f).level (-14).standardMacros (20)
            .macro (4, "Tine").mod (macro4, "osc2Volume", 45);

        add ("Three Tide Pad", "Pad").osc (1, analog, sawPos, 75).unison (1, 4, 16, 90).osc (2, vowel, 30, 70).osc (3, additive, 60, 60).pitch (3, 12)
            .levelEnv (1, 200, 0, 5000, 60).levelEnv (2, 1500, 0, 6000, 100).levelEnv (3, 4000, 0, 6000, 100).lfo (1, sine, 0.12f, 0).mod (lfo1, "osc2Position", 40)
            .lfo (2, sine, 0.07f, 0).mod (lfo2, "filter1Cutoff", 10).filter (1, ladder12, 1800, 15).amp (300, 1000, 100, 2500).chorus (35).reverb (40, 85, 5.0f)
            .level (-11).standardMacros (25).macro (4, "Tide").mod (macro4, "osc3Volume", 40);
        add ("Cathedral Stack Pad", "Pad").osc (1, additive, 50, 80).osc (2, vowel, 70, 65).pitch (2, 12).osc (3, analog, sinePos, 85).pitch (3, -12)
            .levelEnv (2, 2500, 0, 6000, 100).levelEnv (1, 400, 0, 6000, 100).lfo (1, drift, 0.2f, 0).mod (lfo1, "osc1Position", 30).mod (lfo1, "osc2Position", 25)
            .filter (1, svfLow, 2600, 12).amp (600, 1000, 100, 3500).reverb (50, 95, 7.0f).level (-19).standardMacros (22).macro (4, "Choir").mod (macro4, "osc2Volume", 40);
        add ("Slow Bloom Pad", "Pad").osc (1, growl, 15, 65).unison (1, 2, 14, 80).osc (2, analog, sawPos, 70).unison (2, 3, 18, 90).pitch (2, 12).osc (3, metal, 30, 40).pitch (3, 24)
            .levelEnv (1, 100, 0, 6000, 70).levelEnv (2, 3000, 0, 6000, 100).levelEnv (3, 6000, 0, 6000, 100).env (3, 5000, 4000, 80, 2000, 0).mod (env3, "filter1Cutoff", 22)
            .mod (env3, "osc1Position", 40).filter (1, ladder24, 700, 20).amp (500, 1000, 100, 3000).chorus (30).reverb (45, 90, 6.0f).level (-9)
            .standardMacros (25).macro (4, "Shine").mod (macro4, "osc3Volume", 45);
        add ("Frost Layers Pad", "Pad").osc (1, analog, trianglePos, 80).unison (1, 4, 12, 100).osc (2, fm, 30, 55).pitch (2, 12).osc (3, metal, 75, 35).pitch (3, 19)
            .levelEnv (2, 2000, 0, 6000, 100).levelEnv (3, 0, 0, 2500, 25).lfo (1, sine, 0.1f, 0).mod (lfo1, "osc2Position", 35).lfo (2, triangle, 0.23f, 0)
            .mod (lfo2, "osc3Volume", 25).filter (1, svfLow, 5000, 10).filter (2, svfHigh, 180, 10).amp (800, 1000, 100, 4000).delay (quarterD, 40, 22, 6000)
            .reverb (50, 95, 8.0f).level (-13).standardMacros (20).macro (4, "Frost").mod (macro4, "osc3Volume", 45);
        add ("Dark Triad Pad", "Pad").osc (1, analog, sawPos, 75).unison (1, 3, 15, 80).osc (2, analog, sawPos, 70).unison (2, 3, 15, 80).pitch (2, 7).osc (3, vowel, 20, 65).pitch (3, 12)
            .levelEnv (2, 1200, 0, 6000, 100).levelEnv (3, 3500, 0, 6000, 100).lfo (1, sine, 0.09f, 0).mod (lfo1, "filter1Cutoff", 12).mod (lfo1, "osc3Position", 35)
            .filter (1, ladder24, 900, 22).amp (700, 1000, 100, 3500).phaser (30, 0.15f).reverb (42, 90, 6.0f).level (-13).standardMacros (25)
            .macro (4, "Fifth").mod (macro4, "osc2Volume", 40);
        add ("Breathing Layers Pad", "Pad").osc (1, vowel, 45, 75).unison (1, 2, 12, 70).osc (2, additive, 35, 65).pitch (2, -12).osc (3, growl, 20, 50).pitch (3, 12).noise (12, 1, -30)
            .levelEnv (1, 800, 0, 6000, 100).levelEnv (3, 3000, 0, 6000, 60).lfo (1, sine, 0.16f, 0).mod (lfo1, "osc1Position", 40).mod (lfo1, "ampLevel", 8)
            .lfo (2, drift, 0.3f, 0).mod (lfo2, "osc3Position", 30).filter (1, ladder12, 1500, 18).amp (900, 1000, 100, 3500).reverb (45, 90, 6.5f).level (-18)
            .standardMacros (22).macro (4, "Breath").mod (macro4, "noiseLevel", 40);

        add ("Three Engine Drone", "Drone").osc (1, growl, 30, 75).osc (2, fm, 35, 75).pitch (2, -12).osc (3, metal, 50, 40).pitch (3, 7)
            .levelEnv (3, 5000, 0, 6000, 100).levelEnv (1, 1500, 0, 6000, 100).lfo (1, drift, 0.08f, 0).mod (lfo1, "osc1Position", 40).mod (lfo1, "filter1Cutoff", 12)
            .lfo (2, sine, 0.05f, 0).mod (lfo2, "osc3Position", 40).mod (lfo2, "osc2Position", 35).mod (lfo2, "osc3Volume", 25).filter (1, ladder24, 600, 30, 25).amp (1500, 1000, 100, 4000)
            .drive (tube, 25, 5000).phaser (40, 0.07f).reverb (40, 90, 6.0f).level (-20).standardMacros (25).macro (4, "Third Engine").mod (macro4, "osc3Volume", 45);
        add ("Tidal Layers Drone", "Drone").osc (1, vowel, 20, 75).unison (1, 2, 16, 80).osc (2, additive, 50, 70).pitch (2, 12).osc (3, sub, 40, 85).pitch (3, -12)
            .levelEnv (2, 6000, 0, 6000, 100).lfo (1, sine, 0.06f, 0).mod (lfo1, "osc1Position", 45).mod (lfo1, "osc2Volume", 30).lfo (2, sine, 0.04f, 0)
            .mod (lfo2, "filter1Cutoff", 14).mod (lfo2, "osc3Volume", -20).filter (1, ladder12, 1200, 22).amp (2000, 1000, 100, 5000).phaser (25, 0.1f)
            .reverb (50, 95, 8.0f).level (-15).standardMacros (22).macro (4, "Swell").mod (macro4, "osc2Volume", 40);
        add ("Harmonic Ladder Drone", "Drone").osc (1, analog, sinePos, 85).osc (2, analog, trianglePos, 70).pitch (2, 7).osc (3, fm, 25, 50).pitch (3, 19)
            .levelEnv (2, 3000, 0, 6000, 100).levelEnv (3, 7000, 0, 6000, 100).lfo (1, sine, 0.07f, 0).mod (lfo1, "osc3Position", 40).mod (lfo1, "osc2Position", 20)
            .lfo (2, drift, 0.2f, 0).mod (lfo2, "osc1Fine", 6).filter (1, svfLow, 3500, 12).amp (2500, 1000, 100, 5000).delay (halfT, 45, 22, 5000)
            .reverb (50, 95, 9.0f).level (-17).standardMacros (20).macro (4, "Rung").mod (macro4, "osc3Volume", 45);

        add ("Layer Bell", "Bell").osc (1, metal, 40, 70).pitch (1, 12).osc (2, fm, 60, 75).osc (3, additive, 20, 80)
            .levelEnv (1, 0, 0, 200, 0).levelEnv (2, 0, 0, 900, 0).levelEnv (3, 0, 0, 3500, 0).env (2, 0, 600, 0, 400, 70).mod (env2, "osc2Position", -35)
            .filter (1, svfLow, 7000, 10, 0, 50).amp (0, 4000, 0, 2500).reverb (40, 85, 5.0f).level (-12).standardMacros (20).macro (4, "Hum").mod (macro4, "osc3Volume", 40);
        add ("Glass Triad Bell", "Bell").osc (1, fm, 70, 75).osc (2, fm, 50, 65).pitch (2, 7).osc (3, metal, 80, 40).pitch (3, 24)
            .levelEnv (3, 0, 0, 150, 0).levelEnv (2, 0, 200, 1500, 0).levelEnv (1, 0, 0, 2800, 0).filter (1, svfLow, 8000, 8, 0, 50).amp (0, 3500, 0, 2500)
            .delay (quarterD, 35, 20, 6000).reverb (45, 90, 6.0f).level (-13).standardMacros (18).macro (4, "Sparkle").mod (macro4, "osc3Volume", 45);
        add ("Deep Gong Stack", "Bell").osc (1, metal, 15, 80).pitch (1, -12).osc (2, metal, 55, 60).osc (3, growl, 30, 45).pitch (3, 7)
            .levelEnv (2, 0, 0, 1200, 10).levelEnv (3, 300, 0, 4000, 30).levelEnv (1, 0, 0, 6000, 0).env (2, 0, 2500, 0, 1500, 60).mod (env2, "osc1Position", 40)
            .mod (env2, "osc3Position", 30).filter (1, ladder12, 2400, 20, 15, 30).amp (0, 6000, 0, 3500).reverb (45, 95, 7.0f).level (-7).standardMacros (22)
            .macro (4, "Shimmer").mod (macro4, "osc3Volume", 45);

        add ("Cascade Layers", "Sequence").osc (1, analog, sawPos, 80).osc (2, pulse, 40, 75).pitch (2, 7).osc (3, fm, 50, 70).pitch (3, 12)
            .levelEnv (1, 0, 0, 220, 0).levelEnv (2, 0, 125, 220, 0).levelEnv (3, 0, 250, 300, 0).filter (1, ladder24, 1200, 28, 20).env (2, 0, 400, 0, 200, 70)
            .mod (env2, "filter1Cutoff", 26).amp (0, 900, 0, 250).delay (eighthD, 40, 28, 5000).reverb (20).level (-10).standardMacros (25)
            .macro (4, "Top").mod (macro4, "osc3Volume", 40);
        add ("Layer Gate Sequence", "Sequence").osc (1, growl, 35, 80).osc (2, analog, sawPos, 70).pitch (2, 12).osc (3, pulse, 25, 65).pitch (3, -12)
            .levelEnv (2, 600, 0, 4000, 100).lfoSync (1, steps, bar1, 1, 88).stepPattern (1, { 0, -100, 0, 0, -100, 0, -100, 0, 0, -100, 0, 0, -100, 0, 0, -100 })
            .mod (lfo1, "ampLevel", 75).lfoSync (2, triangle, bars2, 0).mod (lfo2, "osc1Position", 40).mod (lfo2, "filter1Cutoff", 14)
            .filter (1, ladder24, 1100, 30, 30).amp (5, 500, 100, 250).delay (eighthD, 35, 20, 5000).level (-12).standardMacros (25)
            .macro (4, "Top").mod (macro4, "osc2Volume", 40);

        add ("Three Layer Static", "Texture").osc (1, metal, 60, 50).warp (1, wCrush, 50).osc (2, fm, 80, 45).pitch (2, 19).osc (3, growl, 70, 50).pitch (3, -12).noise (30, 2)
            .levelEnv (2, 0, 0, 800, 20).levelEnv (3, 1500, 0, 6000, 100).lfo (1, randomShape, 9, 0, 60).mod (lfo1, "osc1Position", 45).mod (lfo1, "osc2Position", 40)
            .lfo (2, drift, 0.3f, 0).mod (lfo2, "filter1Cutoff", 18).filter (1, svfBand, 1800, 35).amp (200, 1000, 100, 1500).delay (sixteenthD, 40, 25)
            .reverb (30).level (-5).standardMacros (25).macro (4, "Crackle").mod (macro4, "noiseLevel", 40);
        add ("Engine Room Layers", "Texture").osc (1, growl, 10, 70).pitch (1, -12).osc (2, sync, 40, 50).warp (2, wSync, 30).osc (3, metal, 20, 40).pitch (3, 7).noise (20, 1, -40)
            .levelEnv (2, 2500, 0, 6000, 100).levelEnv (3, 5000, 0, 6000, 100).lfo (1, drift, 0.15f, 0).mod (lfo1, "osc1Position", 40).mod (lfo1, "osc2WarpAmount", 35)
            .lfo (2, sine, 0.08f, 0).mod (lfo2, "filter1Cutoff", 16).filter (1, ladder12, 900, 30, 30).amp (1500, 1000, 100, 3000).drive (tube, 30, 4000)
            .reverb (40, 90, 5.5f).level (-19).standardMacros (25).macro (4, "Machinery").mod (macro4, "osc2Volume", 45);

        // Then the banks, each category's patches after the hand-made ones in browser order: the first
        // bank's, then the second's (the sequence presets take a category's first patches, so those stay).
        addBank (b);
        addBank2 (b);
        std::stable_sort (b.begin(), b.end(), [] (const Builder& x, const Builder& y)
        {
            return patchCategories().indexOf (x.patch.category) < patchCategories().indexOf (y.patch.category);
        });

        std::vector<Patch> result;
        result.reserve (b.size());

        for (auto& p : b)
            result.push_back (std::move (p.patch));

        return result;
    }
}

namespace WaveSynth
{
    const std::vector<Patch>& factoryPatches()
    {
        static const auto all = WaveSynthPatchBuilder::build();
        return all;
    }

    juce::String patchNamedNow (const juce::String& name)
    {
        // Two factory patches that carried a style's name, renamed with the styles in 0.22.0.
        if (name.equalsIgnoreCase ("EBM Sequence Bass")) return "Machine Sequence";
        if (name.equalsIgnoreCase ("Body Music"))        return "Iron Motion";
        return name;
    }

    const juce::StringArray& patchCategories()
    {
        static const juce::StringArray c { "Init", "Bass", "Sub", "Reese", "Acid", "Lead", "Stab", "Chord", "Pluck", "Arp", "Keys", "Organ", "Guitar", "Brass", "Strings",
                                           "Pad", "Drone", "Bell", "Sequence", "Riser", "Impact", "Cinematic", "Perc", "Drum", "FX", "Glitch", "Game", "Chip", "Texture", "Vox" };
        return c;
    }
}
