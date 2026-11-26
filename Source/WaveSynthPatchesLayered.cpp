#include "WaveSynthPatchBuilder.h"

// The layered patches: hand-made, first in their categories. Every one plays all three oscillators
// (and the sub where the low end needs it), each layer its own wave, and moves on more than one
// timescale: an envelope that shapes each note, a tempo-synced LFO or step pattern under the beat,
// and often a slow drift over bars. Their macros change the character, not just one knob.

namespace WaveSynthPatchBuilder
{
    void addLayered (std::vector<Builder>& b)
    {
        const auto add = [&b] (const char* name, const char* category) -> Builder& { b.emplace_back (name, category); return b.back(); };

        //======================================================================
        // Bass. Growls that wobble: folded and mirrored tables swept by synced LFOs, a second motion
        // under the first, through fold or fuzz drive.
        add ("Hydraulic Maw", "Bass").osc (1, growl, 30, 80).warp (1, wFold, 35).osc (2, growl, 70, 65).pitch (2, -12).warp (2, wMirror, 20)
            .osc (3, vowel, 40, 45).pitch (3, 12).levelEnv (3, 0, 0, 250, 30).subOsc (40).filter (1, ladder24, 700, 35, 40, 20)
            .lfoSync (1, sine, eighth, 1).mod (lfo1, "osc1Position", 35).mod (lfo1, "osc2Position", -25).mod (lfo1, "filter1Cutoff", 22).mod (lfo1, "osc1WarpAmount", 20)
            .lfoSync (2, steps, bar1, 1, 30).stepPattern (2, { 100, -60, 20, -100, 60, -20, 100, -60, -100, 40, -20, 80, -60, 100, -40, 0 })
            .mod (lfo2, "osc3Position", 40).mod (lfo2, "osc3Volume", 30).env (2, 0, 300, 0, 150, 70).mod (env2, "osc1WarpAmount", 25)
            .amp (1, 300, 90, 80).mono().drive (foldDrive, 35, 4500).level (-14.5)
            .macro (1, "Fold").mod (macro1, "osc1WarpAmount", 40).macro (2, "Mouth").mod (macro2, "osc3Volume", 40)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Ferrous Yawn", "Bass").osc (1, growl, 55, 80).warp (1, wFold, 25).osc (2, metal, 30, 45).pitch (2, 0, 7).levelEnv (2, 0, 0, 180, 25)
            .osc (3, sub, 50, 70).pitch (3, -12).filter (1, ladder12, 900, 40, 35, 20).filter (2, svfNotch, 600, 30)
            .lfoSync (1, triangle, quarterT, 1).mod (lfo1, "osc1Position", 40).mod (lfo1, "filter1Cutoff", 18)
            .lfo (2, sine, 0.4f, 0).mod (lfo2, "filter2Cutoff", 35).env (2, 0, 200, 0, 100, 70).mod (env2, "osc1WarpAmount", 30)
            .amp (1, 300, 90, 80).mono().drive (fuzz, 30, 4000).level (-22.5)
            .macro (1, "Yawn").mod (macro1, "osc1Position", 40).macro (2, "Fold").mod (macro2, "osc1WarpAmount", 45)
            .macro (3, "Metal").mod (macro3, "osc2Volume", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Turbine Throat", "Bass").osc (1, growl, 20, 80).unison (1, 2, 10, 50).osc (2, fm, 50, 40).pitch (2, 7).levelEnv (2, 200, 0, 1500, 80)
            .osc (3, analog, sawPos, 60).pitch (3, -12).filter (1, ladder24, 600, 45, 50, 20).filter (2, comb, 220, 40, 0, 100)
            .lfoSync (1, sawDown, sixteenth, 1).mod (lfo1, "filter1Cutoff", 25).mod (lfo1, "osc1Position", 30)
            .lfoSync (2, sawUp, bars2, 1).mod (lfo2, "filter2Resonance", 30).amp (1, 300, 90, 80).mono().drive (foldDrive, 30, 5000).level (-11.5)
            .macro (1, "Throat").mod (macro1, "filter2Resonance", 35).macro (2, "Spin").mod (macro2, "osc1Position", 35)
            .macro (3, "Fifth").mod (macro3, "osc2Volume", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Slag Wobble", "Bass").osc (1, growl, 70, 80).warp (1, wCrush, 15).osc (2, pulse, 20, 60).pitch (2, -12).warp (2, wBend, 30)
            .osc (3, vowel, 60, 40).levelEnv (3, 0, 0, 300, 40).pitch (3, 12).subOsc (35).filter (1, ladder24, 750, 35, 40, 20)
            .lfoSync (1, sine, eighthT, 1, 20).mod (lfo1, "osc1Position", 30).mod (lfo1, "osc3Position", 45).mod (lfo1, "filter1Cutoff", 25)
            .lfoSync (2, steps, bar1, 1, 20).stepPattern (2, { 100, 100, -100, 0, 100, -100, 50, -50, 100, 100, -100, 0, -100, 50, 100, -100 })
            .mod (lfo2, "osc2WarpAmount", 40).mod (lfo2, "osc1WarpAmount", 20).amp (1, 300, 90, 80).mono().drive (crush, 25, 6000, 50).level (-4.5)
            .macro (1, "Wobble").mod (macro1, "filter1Cutoff", 25).macro (2, "Crush").mod (macro2, "osc1WarpAmount", 40)
            .macro (3, "Mouth").mod (macro3, "osc3Volume", 40).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Magma Pulley", "Bass").osc (1, growl, 40, 80).warp (1, wFold, 40).pan (1, 40).osc (2, growl, 10, 70).pitch (2, 0, -12).warp (2, wMirror, 30).pan (2, -40)
            .osc (3, sync, 30, 40).pitch (3, 12).warp (3, wSync, 30).levelEnv (3, 0, 0, 120, 0).subOsc (40).filter (1, ladder24, 800, 30, 45, 20)
            .lfoSync (1, sine, quarter, 1).mod (lfo1, "osc1WarpAmount", 30).mod (lfo1, "osc2WarpAmount", -30)
            .lfo (2, drift, 0.13f, 0).mod (lfo2, "filter1Cutoff", 10).env (2, 0, 250, 0, 120, 70).mod (env2, "osc3WarpAmount", 40)
            .amp (1, 300, 90, 80).mono().drive (foldDrive, 40, 5000).level (-16)
            .macro (1, "Fold").mod (macro1, "osc1WarpAmount", 40).macro (2, "Width").mod (macro2, "osc2Pan", -40)
            .macro (3, "Bite").mod (macro3, "osc3Volume", 45).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Rotor Gnash", "Bass").osc (1, growl, 25, 80).osc (2, growl, 75, 50).pitch (2, 12).osc (3, analog, squarePos, 70).pitch (3, -12)
            .levelEnv (3, 0, 0, 900, 60).filter (1, ladder24, 500, 40, 35, 20).filter (2, ladderHigh, 45, 10)
            .lfoSync (1, square, sixteenth, 1, 40).mod (lfo1, "osc1Position", 35).mod (lfo1, "osc2Position", -35)
            .lfoSync (2, triangle, bars2, 1).mod (lfo2, "filter1Cutoff", 30).amp (1, 300, 90, 80).mono().drive (fuzz, 30, 4000).level (-21.5)
            .macro (1, "Gnash").mod (macro1, "osc2Volume", 40).macro (2, "Open").mod (macro2, "filter1Cutoff", 30)
            .macro (3, "Square").mod (macro3, "osc3Volume", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Sulphur Choke", "Bass").osc (1, growl, 50, 80).warp (1, wFold, 30).osc (2, vowel, 20, 55).osc (3, fm, 70, 60).pitch (3, -12)
            .levelEnv (3, 0, 50, 400, 40).filter (1, ladder12, 900, 35, 30, 20)
            .lfoSync (1, sawUp, eighth, 1, 20).mod (lfo1, "osc2Position", 50).mod (lfo1, "filter1Cutoff", 20)
            .lfoSync (2, sine, quarterD, 1).mod (lfo2, "osc1WarpAmount", 30).amp (1, 300, 90, 80).mono().drive (tube, 40, 4500).level (-14.5)
            .macro (1, "Choke").mod (macro1, "filter1Resonance", 30).macro (2, "Vowel").mod (macro2, "osc2Position", 40)
            .macro (3, "Fold").mod (macro3, "osc1WarpAmount", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Piston Howl", "Bass").osc (1, growl, 35, 80).unison (1, 3, 12, 60).osc (2, fm, 30, 50).warp (2, wFm, 30).osc (3, metal, 50, 30).pitch (3, 19)
            .levelEnv (3, 0, 0, 90, 0).subOsc (40).filter (1, ladder24, 650, 30, 40, 20)
            .lfoSync (1, sine, half, 1).mod (lfo1, "osc1Position", 45).lfoSync (2, triangle, eighth, 1).mod (lfo2, "osc2WarpAmount", 35).mod (lfo2, "filter1Cutoff", 15)
            .amp (1, 300, 90, 80).mono().drive (foldDrive, 30, 5000).level (-13)
            .macro (1, "Howl").mod (macro1, "osc1Position", 40).macro (2, "FM").mod (macro2, "osc2WarpAmount", 40)
            .macro (3, "Clank").mod (macro3, "osc3Volume", 50).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Tar Siren", "Bass").osc (1, growl, 60, 80).warp (1, wBend, 30).osc (2, analog, sawPos, 60).pitch (2, -12, 8).osc (3, vowel, 80, 35).levelEnv (3, 60, 0, 900, 70).pitch (3, 12)
            .filter (1, ladder24, 700, 40, 35, 20).lfoSync (1, sine, quarter, 1).mod (lfo1, "osc1Position", 30).mod (lfo1, "filter1Cutoff", 25)
            .lfoSync (2, sawDown, bar1, 1).mod (lfo2, "osc3Position", 50).env (3, 0, 400, 0, 100, 70).mod (env3, "osc1Pitch", 4.17f)
            .amp (1, 300, 90, 80).mono (60).drive (hard, 30, 4500).level (-11.5)
            .macro (1, "Siren").mod (macro1, "osc3Volume", 40).macro (2, "Bend").mod (macro2, "osc1WarpAmount", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Cinder Mandible", "Bass").osc (1, growl, 15, 80).warp (1, wFold, 50).osc (2, growl, 85, 70).pitch (2, -12).warp (2, wFold, 20)
            .osc (3, sync, 40, 35).pitch (3, 7).levelEnv (3, 0, 0, 200, 15).noise (15, 1).filter (1, ladder24, 850, 30, 40, 20)
            .lfoSync (1, steps, bar1, 1, 25).stepPattern (1, { 100, -40, 60, -100, 100, -40, -100, 60, 100, -40, 60, -100, 40, 100, -100, -40 })
            .mod (lfo1, "osc1WarpAmount", 40).mod (lfo1, "osc2Position", 35).mod (lfo1, "filter1Cutoff", 20)
            .lfoSync (2, triangle, sixteenthT, 1).mod (lfo2, "osc1Position", 15).amp (1, 300, 90, 80).mono().drive (foldDrive, 35, 5000).level (-13)
            .macro (1, "Fold").mod (macro1, "osc1WarpAmount", 40).macro (2, "Jaw").mod (macro2, "osc2Position", 40)
            .macro (3, "Hiss").mod (macro3, "noiseLevel", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        // Bass that bites: hard-synced and FM tops swept by fast envelopes, a snap on every note.
        add ("Riveted Snap", "Bass").osc (1, sync, 20, 80).warp (1, wSync, 50).osc (2, analog, sawPos, 70).pitch (2, -12).osc (3, fm, 60, 45).pitch (3, 12)
            .levelEnv (3, 0, 0, 80, 0).filter (1, ladder24, 500, 30, 30, 30).env (2, 0, 180, 0, 100, 80).mod (env2, "osc1WarpAmount", 50).mod (env2, "filter1Cutoff", 30)
            .env (3, 0, 60, 0, 50, 80).mod (env3, "osc3Position", 40).mod (velocity, "osc1WarpAmount", 20).amp (0, 300, 70, 60).mono (0, false)
            .drive (hard, 35, 5000).level (-11.5)
            .macro (1, "Snap").mod (macro1, "osc1WarpAmount", 40).macro (2, "Click").mod (macro2, "osc3Volume", 45)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Torque Fang", "Bass").osc (1, sync, 50, 80).warp (1, wSync, 30).unison (1, 2, 8, 50).osc (2, pulse, 30, 60).pitch (2, 0, -6).warp (2, wBend, 20)
            .osc (3, analog, sinePos, 85).pitch (3, -12).levelEnv (3, 120, 0, 2000, 100).filter (1, ladder12, 1400, 25, 30, 30).filter (2, svfLow, 4000, 10)
            .env (2, 0, 260, 10, 120, 75).mod (env2, "osc1WarpAmount", 45).mod (env2, "osc2WarpAmount", 30).lfo (1, drift, 0.2f, 0).mod (lfo1, "osc1Fine", 6)
            .amp (0, 400, 80, 70).mono (0, false).drive (tube, 40, 5000).level (-12.5)
            .macro (1, "Fang").mod (macro1, "osc1WarpAmount", 40).macro (2, "Body").mod (macro2, "osc3Volume", 35)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Spark Gap", "Bass").osc (1, fm, 70, 75).warp (1, wFm, 40).osc (2, analog, squarePos, 65).pitch (2, -12).osc (3, metal, 20, 30).pitch (3, 24)
            .levelEnv (3, 0, 0, 60, 0).filter (1, ladder24, 900, 20, 35, 30).env (2, 0, 120, 0, 80, 80).mod (env2, "osc1WarpAmount", 50).mod (env2, "osc1Position", 30)
            .env (3, 0, 30, 0, 30, 80).mod (env3, "osc1Pitch", 4.17f).amp (0, 300, 70, 60).mono (0, false).drive (hard, 30, 5500).level (-7.5)
            .macro (1, "Spark").mod (macro1, "osc1WarpAmount", 40).macro (2, "Arc").mod (macro2, "osc3Volume", 45)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Clamp Jaw", "Bass").osc (1, sync, 70, 80).warp (1, wSync, 60).osc (2, sync, 30, 45).pitch (2, 7).warp (2, wSync, 40).osc (3, analog, sawPos, 70).pitch (3, -12)
            .levelEnv (2, 0, 0, 300, 40).filter (1, ladder24, 650, 35, 35, 30).env (2, 0, 350, 0, 200, 70).mod (env2, "osc1WarpAmount", 50).mod (env2, "osc2WarpAmount", 40)
            .mod (env2, "filter1Cutoff", 25).mod (velocity, "filter1Cutoff", 15).amp (1, 400, 80, 80).mono (30).drive (foldDrive, 25, 5000).level (-9.5)
            .macro (1, "Clamp").mod (macro1, "osc1WarpAmount", 40).macro (2, "Fifth").mod (macro2, "osc2Volume", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Hot Wire Bass", "Bass").osc (1, pulse, 15, 80).warp (1, wSqueeze, 40).osc (2, sync, 40, 55).warp (2, wSync, 35).levelEnv (2, 0, 0, 150, 20)
            .osc (3, fm, 40, 65).pitch (3, -12).filter (1, ladder24, 1100, 30, 25, 40).env (2, 0, 200, 0, 120, 75).mod (env2, "osc2WarpAmount", 45).mod (env2, "osc1WarpAmount", 25)
            .lfoSync (1, randomShape, sixteenth, 1, 30).mod (lfo1, "filter1Cutoff", 8).amp (0, 350, 75, 70).mono (0, false).drive (crush, 15, 6000, 40).level (-3)
            .macro (1, "Wire").mod (macro1, "osc2WarpAmount", 40).macro (2, "Squeeze").mod (macro2, "osc1WarpAmount", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Crush").mod (macro4, "fxDriveAmount", 45);
        add ("Ratchet Bite", "Bass").osc (1, analog, sawPos, 80).osc (2, sync, 60, 50).pitch (2, 12).warp (2, wSync, 50).levelEnv (2, 0, 0, 100, 0)
            .osc (3, sub, 40, 80).pitch (3, -12).filter (1, ladder24, 700, 25, 40, 30).env (2, 0, 90, 0, 60, 85).mod (env2, "osc2WarpAmount", 60)
            .env (3, 0, 40, 0, 30, 85).mod (env3, "osc1Pitch", 20.83f).mod (env3, "osc3Pitch", 20.83f).amp (0, 250, 0, 80).mono (0, false)
            .drive (hard, 40, 5000).level (-13.5)
            .macro (1, "Ratchet").mod (macro1, "osc2Volume", 40).macro (2, "Knock").mod (macro2, "filter1Resonance", 35)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Lockjaw Engine", "Bass").osc (1, sync, 35, 80).warp (1, wSync, 25).unison (1, 3, 10, 60).osc (2, fm, 55, 40).pitch (2, 12).warp (2, wFm, 30)
            .osc (3, analog, squarePos, 65).levelEnv (3, 0, 0, 600, 55).pitch (3, -12).filter (1, ladder24, 800, 30, 30, 30).env (2, 0, 500, 20, 200, 70).mod (env2, "osc1WarpAmount", 35)
            .lfoSync (1, square, eighth, 1, 50).mod (lfo1, "osc2Volume", -30).amp (1, 400, 85, 80).mono().drive (tube, 35, 5000).level (-10.5)
            .macro (1, "Lock").mod (macro1, "osc1WarpAmount", 40).macro (2, "FM").mod (macro2, "osc2WarpAmount", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Arc Furnace", "Bass").osc (1, sync, 15, 80).warp (1, wSync, 70).osc (2, growl, 40, 60).warp (2, wFold, 20).osc (3, analog, sinePos, 80).pitch (3, -12)
            .levelEnv (3, 150, 0, 2000, 100).filter (1, ladder12, 1200, 35, 40, 30).env (2, 0, 600, 0, 250, 60).mod (env2, "osc1WarpAmount", -45).mod (env2, "filter1Cutoff", 30)
            .lfo (1, sine, 0.25f, 0).mod (lfo1, "osc2Position", 25).amp (1, 400, 85, 90).mono().drive (foldDrive, 30, 5000).level (-13.5)
            .macro (1, "Arc").mod (macro1, "osc1WarpAmount", 40).macro (2, "Growl").mod (macro2, "osc2Volume", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Tension Strap", "Bass").osc (1, string, 30, 80).osc (2, sync, 45, 55).warp (2, wSync, 40).levelEnv (2, 0, 0, 180, 30).osc (3, analog, sinePos, 80).pitch (3, -12)
            .filter (1, ladder24, 1000, 30, 30, 30).env (2, 0, 220, 0, 120, 75).mod (env2, "osc2WarpAmount", 45).mod (env2, "filter1Cutoff", 22)
            .lfoSync (1, triangle, bar1, 1).mod (lfo1, "osc1Position", 25).amp (0, 500, 40, 100).mono (0, false)
            .drive (hard, 25, 5000).level (-9)
            .macro (1, "Strap").mod (macro1, "osc1Position", 40).macro (2, "Sync").mod (macro2, "osc2WarpAmount", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Bolt Cutter", "Bass").osc (1, fm, 30, 80).warp (1, wFm, 50).osc (2, fm, 80, 60).pitch (2, -12).osc (3, sync, 60, 35).pitch (3, 12).warp (3, wSync, 60)
            .levelEnv (3, 0, 0, 70, 0).filter (1, ladder24, 1000, 20, 30, 40).env (2, 0, 160, 0, 80, 80).mod (env2, "osc1WarpAmount", 50).mod (env2, "osc2Position", 30)
            .mod (velocity, "osc1WarpAmount", 25).amp (0, 300, 75, 70).mono (0, false).drive (fuzz, 25, 4500).level (-16)
            .macro (1, "Cut").mod (macro1, "osc1WarpAmount", 40).macro (2, "Spark").mod (macro2, "osc3Volume", 45)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        // Bass that pumps: wide detuned stacks, filter plucks, ducking step patterns and echoes on the beat.
        add ("Pylon Bloom", "Bass").osc (1, analog, sawPos, 80).unison (1, 5, 18, 90).osc (2, analog, squarePos, 60).pitch (2, -12).unison (2, 3, 12, 60)
            .osc (3, pulse, 30, 35).pitch (3, 12).levelEnv (3, 0, 0, 200, 20).filter (1, ladder24, 300, 45, 20, 20).env (2, 0, 140, 0, 100, 80).mod (env2, "filter1Cutoff", 38)
            .lfoSync (1, steps, bar1, 1, 60).stepPattern (1, { -100, -50, -15, 0, -100, -50, -15, 0, -100, -50, -15, 0, -100, -50, -15, 0 }).mod (lfo1, "ampLevel", 40)
            .amp (1, 300, 80, 80).mono (0, false).delay (eighthD, 30, 18, 3500).level (-3.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30).macro (2, "Width").mod (macro2, "osc1UnisonSpread", -40)
            .macro (3, "Pump").mod (macro3, "ampLevel", -15).macro (4, "Echo").mod (macro4, "fxDelayMix", 40);
        add ("Grid Pump", "Bass").osc (1, analog, sawPos, 80).unison (1, 4, 14, 80).osc (2, analog, sawPos, 50).pitch (2, 7).unison (2, 2, 10, 60)
            .osc (3, vowel, 30, 30).levelEnv (3, 0, 0, 300, 0).subOsc (45).filter (1, ladder24, 500, 30, 20, 20).env (2, 0, 160, 0, 100, 80).mod (env2, "filter1Cutoff", 25)
            .lfoSync (1, steps, bar1, 1, 60).stepPattern (1, { -100, -50, -15, 0, -100, -50, -15, 0, -100, -50, -15, 0, -100, -50, -15, 0 })
            .mod (lfo1, "ampLevel", 45).mod (lfo1, "filter1Cutoff", 15).amp (1, 300, 80, 80).mono (0, false).delay (eighth, 25, 15, 3500).level (-2.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30).macro (2, "Fifth").mod (macro2, "osc2Volume", 40)
            .macro (3, "Sub").mod (macro3, "subLevel", 40).macro (4, "Echo").mod (macro4, "fxDelayMix", 40);
        add ("Strobe Saw Bass", "Bass").osc (1, analog, sawPos, 80).unison (1, 7, 22, 100).osc (2, pulse, 50, 55).pitch (2, -12).unison (2, 3, 10, 50)
            .osc (3, sync, 30, 30).pitch (3, 12).levelEnv (3, 0, 0, 120, 0).filter (1, ladder12, 400, 40, 20, 20).env (2, 0, 150, 0, 100, 80).mod (env2, "filter1Cutoff", 30)
            .lfoSync (1, square, sixteenth, 1, 30).mod (lfo1, "filter1Cutoff", 22).amp (1, 300, 80, 80).mono (0, false).chorus (25).delay (sixteenthD, 25, 12, 4000).level (-4.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30).macro (2, "Strobe").mod (macro2, "filter1Resonance", 30)
            .macro (3, "Width").mod (macro3, "osc1UnisonSpread", -40).macro (4, "Echo").mod (macro4, "fxDelayMix", 40);
        add ("Concourse Throb", "Bass").osc (1, analog, sawPos, 80).unison (1, 4, 16, 80).osc (2, analog, sawPos, 70).pitch (2, -12).unison (2, 2, 6, 40)
            .osc (3, fm, 50, 25).pitch (3, 19).levelEnv (3, 0, 0, 90, 0).filter (1, ladder24, 260, 50, 25, 20).env (2, 0, 180, 0, 120, 80).mod (env2, "filter1Cutoff", 40)
            .lfoSync (1, triangle, eighth, 1).mod (lfo1, "osc1UnisonDetune", 25).amp (1, 300, 80, 80).mono (0, false).delay (quarterD, 20, 14, 3500).reverb (10)
            .level (-3.5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30).macro (2, "Throb").mod (macro2, "osc1UnisonDetune", 40)
            .macro (3, "Ping").mod (macro3, "osc3Volume", 45).macro (4, "Echo").mod (macro4, "fxDelayMix", 40);
        add ("Afterburn Unison", "Bass").osc (1, analog, sawPos, 80).unison (1, 6, 20, 90).osc (2, growl, 40, 55).pitch (2, -12).warp (2, wFold, 15)
            .osc (3, analog, squarePos, 30).pitch (3, 12).levelEnv (3, 0, 0, 250, 0).filter (1, ladder24, 450, 35, 30, 20).env (2, 0, 200, 0, 120, 80).mod (env2, "filter1Cutoff", 35)
            .lfoSync (1, sawUp, bar1, 1).mod (lfo1, "filter1Cutoff", 15).amp (1, 300, 80, 80).mono (0, false).drive (tube, 30, 5000).delay (eighthD, 22, 12, 3500).level (-10.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30).macro (2, "Burn").mod (macro2, "fxDriveAmount", 50)
            .macro (3, "Growl").mod (macro3, "osc2Volume", 40).macro (4, "Echo").mod (macro4, "fxDelayMix", 40);
        add ("Velvet Piston", "Bass").osc (1, analog, sawPos, 80).unison (1, 3, 8, 60).osc (2, analog, sinePos, 85).pitch (2, -12).levelEnv (2, 0, 0, 800, 70)
            .osc (3, additive, 40, 35).pitch (3, 12).levelEnv (3, 0, 0, 300, 10).filter (1, ladder24, 380, 25, 0, 30).env (2, 0, 240, 10, 150, 75).mod (env2, "filter1Cutoff", 32)
            .mod (velocity, "filter1Cutoff", 15).lfoSync (1, steps, bar1, 1, 60).stepPattern (1, { -100, -40, 0, 0, -100, -40, 0, 0, -100, -40, 0, 0, -100, -40, 0, 0 })
            .mod (lfo1, "ampLevel", 35).amp (1, 300, 80, 80).mono (0, false).reverb (8).level (-4)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30).macro (2, "Bell").mod (macro2, "osc3Volume", 40)
            .macro (3, "Pump").mod (macro3, "ampLevel", -15).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Neon Stack Bass", "Bass").osc (1, pulse, 30, 75).unison (1, 4, 14, 70).pan (1, -30).osc (2, pulse, 70, 75).unison (2, 4, 14, 70).pitch (2, 0, 7).pan (2, 30)
            .osc (3, analog, sawPos, 60).levelEnv (3, 0, 0, 300, 40).pitch (3, -12).filter (1, ladder24, 600, 35, 20, 20).lfoSync (1, sine, eighth, 1).mod (lfo1, "osc1Position", 20).mod (lfo1, "osc2Position", -20)
            .env (2, 0, 180, 0, 120, 80).mod (env2, "filter1Cutoff", 30).amp (1, 300, 80, 80).mono (0, false).chorus (30).delay (eighthD, 22, 15, 3500).level (-2)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30).macro (2, "Width").mod (macro2, "osc1UnisonSpread", -40)
            .macro (3, "Low").mod (macro3, "osc3Volume", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 40);
        add ("Treadmill Bass", "Bass").osc (1, analog, sawPos, 80).unison (1, 5, 16, 80).osc (2, sync, 25, 40).warp (2, wSync, 20).levelEnv (2, 0, 0, 150, 15)
            .osc (3, analog, sinePos, 80).pitch (3, -12).filter (1, ladder24, 320, 40, 20, 20).env (2, 0, 100, 0, 60, 85).mod (env2, "filter1Cutoff", 42)
            .lfoSync (1, steps, bar1, 1, 20).stepPattern (1, { 100, -60, -20, 40, -60, 100, -20, -60, 100, -60, 40, -20, -60, 100, -20, 40 }).mod (lfo1, "filter1Cutoff", 20)
            .amp (1, 300, 80, 80).mono (0, false).delay (sixteenthD, 28, 14, 3500).level (-4)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30).macro (2, "Sync").mod (macro2, "osc2Volume", 40)
            .macro (3, "Sub").mod (macro3, "osc3Volume", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 40);
        add ("Bulkhead Swell", "Bass").osc (1, analog, sawPos, 75).unison (1, 6, 18, 90).levelEnv (1, 80, 0, 1500, 100).osc (2, analog, squarePos, 65).pitch (2, -12).unison (2, 2, 8, 40)
            .osc (3, vowel, 30, 35).pitch (3, 12).levelEnv (3, 150, 0, 1500, 80).filter (1, ladder24, 500, 30, 20, 20)
            .lfoSync (1, sine, half, 1).mod (lfo1, "filter1Cutoff", 25)
            .lfoSync (2, steps, bar1, 1, 60).stepPattern (2, { -100, -50, -15, 0, -100, -50, -15, 0, -100, -50, -15, 0, -100, -50, -15, 0 }).mod (lfo2, "ampLevel", 40)
            .amp (5, 500, 90, 150).mono().reverb (12).level (-3)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30).macro (2, "Voice").mod (macro2, "osc3Volume", 40)
            .macro (3, "Pump").mod (macro3, "ampLevel", -15).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Ribbon Throb", "Bass").osc (1, analog, sawPos, 80).unison (1, 3, 10, 70).osc (2, string, 50, 40).pitch (2, 12).levelEnv (2, 0, 0, 400, 10)
            .osc (3, analog, squarePos, 65).pitch (3, -12).filter (1, ladder24, 420, 35, 20, 20).env (2, 0, 200, 0, 120, 80).mod (env2, "filter1Cutoff", 35)
            .lfoSync (1, triangle, quarter, 1).mod (lfo1, "osc1UnisonDetune", 20).amp (1, 300, 80, 80).mono (0, false).chorus (20).delay (eighthD, 30, 18, 3500).level (-3.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30).macro (2, "Pluck").mod (macro2, "osc2Volume", 40)
            .macro (3, "Square").mod (macro3, "osc3Volume", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 40);

        // Bass that talks: vowel tables and band-pass formants moving through words, the sub under them untouched.
        add ("Mouth Of Iron", "Bass").osc (1, analog, sawPos, 80).osc (2, growl, 30, 65).pitch (2, -12).osc (3, vowel, 40, 55).levelEnv (3, 60, 0, 900, 70).route (3, 100).subOsc (45)
            .parallel().filter (1, ladder24, 900, 20, 25, 20).filter (2, svfBand, 700, 55, 20, 20)
            .lfoSync (1, triangle, quarter, 1).mod (lfo1, "filter2Cutoff", 30)
            .lfoSync (2, steps, bar1, 1, 40).stepPattern (2, { -100, 50, 0, 100, -50, 100, -100, 0, 50, -100, 100, -50, 0, 100, -100, 50 }).mod (lfo2, "osc3Position", 45)
            .amp (1, 300, 90, 80).mono().drive (tube, 35, 5000).level (-14)
            .macro (1, "Talk").mod (macro1, "filter2Resonance", 30).macro (2, "Vowel").mod (macro2, "osc3Position", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Yammer Bass", "Bass").osc (1, vowel, 10, 80).osc (2, vowel, 90, 60).pitch (2, -12).osc (3, analog, sawPos, 50).levelEnv (3, 0, 0, 600, 55).pitch (3, 0, 10).subOsc (40)
            .filter (1, ladder24, 1200, 30, 25, 30).lfoSync (1, sawUp, eighth, 1, 15).mod (lfo1, "osc1Position", 60).mod (lfo1, "osc2Position", -40)
            .env (2, 0, 200, 0, 120, 75).mod (env2, "filter1Cutoff", 20).amp (1, 300, 90, 80).mono().drive (tube, 30, 5000).level (-11.5)
            .macro (1, "Yammer").mod (macro1, "osc1Position", 40).macro (2, "Low Vowel").mod (macro2, "osc2Position", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Formant Furnace", "Bass").osc (1, growl, 50, 80).warp (1, wFold, 20).osc (2, analog, sawPos, 65).pitch (2, -12).osc (3, vowel, 50, 35).levelEnv (3, 0, 0, 300, 40).pitch (3, 12).subOsc (45)
            .filter (1, svfBand, 600, 50, 30, 20).filter (2, ladder24, 3000, 10).lfoSync (1, sine, eighthT, 1, 20).mod (lfo1, "filter1Cutoff", 35)
            .lfo (2, sine, 0.3f, 0).mod (lfo2, "osc3Position", 40).amp (1, 300, 90, 80).mono().drive (foldDrive, 30, 5000).level (-10.5)
            .macro (1, "Formant").mod (macro1, "filter1Cutoff", 30).macro (2, "Fold").mod (macro2, "osc1WarpAmount", 40)
            .macro (3, "Voice").mod (macro3, "osc3Volume", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Chatter Valve", "Bass").osc (1, analog, squarePos, 80).osc (2, pulse, 30, 40).pitch (2, 12).osc (3, vowel, 60, 55).levelEnv (3, 60, 0, 900, 70).subOsc (45)
            .filter (1, comb, 300, 55, 0, 100).filter (2, svfBand, 900, 40, 20, 20)
            .lfoSync (1, steps, bar1, 1, 30).stepPattern (1, { 100, -40, 60, -100, 0, 80, -60, 40, 100, -100, 20, -40, 60, 0, -80, 100 }).mod (lfo1, "filter2Cutoff", 40)
            .env (2, 0, 150, 0, 80, 80).mod (env2, "filter2Resonance", 20).amp (1, 300, 90, 80).mono().drive (tube, 25, 5000).level (-11)
            .macro (1, "Chatter").mod (macro1, "filter2Resonance", 30).macro (2, "Pipe").mod (macro2, "filter1Resonance", 30)
            .macro (3, "Vowel").mod (macro3, "osc3Position", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Gargle Line", "Bass").osc (1, growl, 60, 80).osc (2, vowel, 30, 60).osc (3, fm, 40, 60).pitch (3, -12).levelEnv (3, 0, 0, 500, 40).subOsc (40)
            .filter (1, svfBand, 500, 60, 20, 20).filter (2, ladder24, 2500, 10).env (3, 0, 600, 20, 200, 60).mod (env3, "filter1Cutoff", 40)
            .lfoSync (1, randomShape, sixteenth, 1, 60).mod (lfo1, "osc2Position", 25).amp (1, 300, 90, 80).mono().drive (fuzz, 25, 4500).level (-17)
            .macro (1, "Gargle").mod (macro1, "osc2Position", 40).macro (2, "Formant").mod (macro2, "filter1Cutoff", 30)
            .macro (3, "Growl").mod (macro3, "osc1Position", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Vox Hammer Bass", "Bass").osc (1, vowel, 0, 85).osc (2, analog, sawPos, 65).pitch (2, -12).osc (3, sync, 40, 30).pitch (3, 12).levelEnv (3, 0, 0, 90, 0).subOsc (40)
            .filter (1, ladder24, 1300, 30, 30, 30).env (2, 0, 250, 0, 120, 75).mod (env2, "osc1Position", 70).mod (env2, "filter1Cutoff", 15)
            .amp (0, 400, 80, 80).mono (0, false).drive (hard, 30, 5000).level (-11.5)
            .macro (1, "Ya").mod (macro1, "osc1Position", 40).macro (2, "Click").mod (macro2, "osc3Volume", 45)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Tongue Twister", "Bass").osc (1, analog, sawPos, 75).osc (2, analog, sawPos, 75).pitch (2, 0, 12).osc (3, vowel, 50, 50).levelEnv (3, 0, 0, 600, 55).route (2, 100).route (3, 60).subOsc (50)
            .parallel().filter (1, svfBand, 800, 55, 20, 20).filter (2, svfBand, 1600, 45, 20, 20)
            .lfoSync (1, sine, quarter, 1).mod (lfo1, "filter1Cutoff", 30).lfoSync (2, triangle, half, 1).mod (lfo2, "filter2Cutoff", -30)
            .amp (1, 300, 90, 80).mono().drive (tube, 25, 5000).level (-14)
            .macro (1, "Tongue").mod (macro1, "filter1Resonance", 30).macro (2, "Lips").mod (macro2, "filter2Cutoff", 30)
            .macro (3, "Vowel").mod (macro3, "osc3Position", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Baritone Engine", "Bass").osc (1, vowel, 40, 80).unison (1, 3, 8, 50).osc (2, analog, squarePos, 70).pitch (2, -12).osc (3, additive, 50, 25).levelEnv (3, 0, 0, 300, 40).pitch (3, 7).subOsc (45)
            .filter (1, ladderBand, 700, 45, 20, 20).lfo (1, drift, 0.18f, 0).mod (lfo1, "osc1Position", 40)
            .lfoSync (2, steps, bar1, 1, 40).stepPattern (2, { 100, 100, 100, 100, -60, -60, -60, -60, 40, 40, 40, 40, -100, -100, 60, 60 }).mod (lfo2, "filter1Cutoff", 25)
            .amp (1, 300, 90, 80).mono().drive (tube, 30, 5000).chorus (15).level (-14)
            .macro (1, "Vowel").mod (macro1, "osc1Position", 40).macro (2, "Formant").mod (macro2, "filter1Cutoff", 30)
            .macro (3, "Fifth").mod (macro3, "osc3Volume", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Wah Crane", "Bass").osc (1, analog, sawPos, 80).unison (1, 2, 6, 40).osc (2, growl, 40, 60).pitch (2, -12).osc (3, vowel, 80, 30).levelEnv (3, 100, 0, 1000, 60).subOsc (45)
            .filter (1, svfBand, 450, 60, 30, 20).env (2, 0, 400, 30, 200, 30).mod (env2, "filter1Cutoff", 45).mod (velocity, "filter1Cutoff", 15)
            .amp (1, 400, 85, 100).mono (40).drive (hard, 25, 5000).level (-9.5)
            .macro (1, "Wah").mod (macro1, "filter1Cutoff", 30).macro (2, "Growl").mod (macro2, "osc2Volume", 40)
            .macro (3, "Voice").mod (macro3, "osc3Volume", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Larynx Press", "Bass").osc (1, growl, 20, 80).warp (1, wMirror, 30).osc (2, vowel, 50, 55).osc (3, analog, sawPos, 65).levelEnv (3, 60, 0, 900, 70).pitch (3, -12).subOsc (40)
            .filter (1, ladder24, 900, 30, 35, 20).filter (2, svfNotch, 1000, 40)
            .lfoSync (1, triangle, eighth, 1).mod (lfo1, "filter2Cutoff", 35).mod (lfo1, "osc2Position", 30)
            .lfoSync (2, sawUp, bars2, 1).mod (lfo2, "osc1WarpAmount", 30).amp (1, 300, 90, 80).mono().drive (foldDrive, 30, 5000).level (-13)
            .macro (1, "Press").mod (macro1, "osc1WarpAmount", 40).macro (2, "Vowel").mod (macro2, "osc2Position", 40)
            .macro (3, "Notch").mod (macro3, "filter2Cutoff", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        //======================================================================
        // Lead. Screamers: hard sync and fold swept by envelopes and LFOs, vibrato that arrives late,
        // a second voice that answers the first.
        add ("Voltage Scream", "Lead").osc (1, sync, 40, 80).warp (1, wSync, 45).osc (2, analog, sawPos, 65).unison (2, 3, 12, 70).pitch (2, 0, 7)
            .osc (3, pulse, 25, 40).pitch (3, 12).levelEnv (3, 0, 0, 300, 20).filter (1, ladder12, 3200, 25, 30, 50)
            .env (2, 0, 700, 30, 300, 55).mod (env2, "osc1WarpAmount", 40).mod (env2, "filter1Cutoff", 15)
            .lfo (1, sine, 5.5f, 1).lfoFade (1, 700).mod (lfo1, "osc1Fine", 9).mod (lfo1, "osc2Fine", 9)
            .lfoSync (2, triangle, bar1, 1).mod (lfo2, "osc1WarpAmount", 20).amp (2, 600, 85, 250).mono (45).drive (hard, 25, 6000)
            .delay (eighthD, 30, 20, 5000).reverb (18).level (-12)
            .macro (1, "Scream").mod (macro1, "osc1WarpAmount", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Folded Halo", "Lead").osc (1, analog, sinePos, 80).warp (1, wFold, 30).osc (2, analog, trianglePos, 60).pitch (2, 12).warp (2, wFold, 20)
            .osc (3, sync, 30, 35).pitch (3, 19).warp (3, wSync, 30).levelEnv (3, 0, 0, 200, 10).filter (1, svfLow, 5000, 15, 0, 50)
            .env (2, 0, 900, 40, 400, 50).mod (env2, "osc1WarpAmount", 45).mod (env2, "osc2WarpAmount", 30)
            .lfo (1, sine, 5.0f, 1).lfoFade (1, 800).mod (lfo1, "osc1Fine", 8).lfoSync (2, sine, half, 1).mod (lfo2, "osc1WarpAmount", 15)
            .amp (3, 700, 85, 300).mono (35).delay (quarter, 30, 20, 5000).reverb (22, 70, 3.0f).level (-7.5)
            .macro (1, "Fold").mod (macro1, "osc1WarpAmount", 40).macro (2, "Octave").mod (macro2, "osc2Volume", 40)
            .macro (3, "Glint").mod (macro3, "osc3Volume", 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Razor Siren", "Lead").osc (1, sync, 60, 80).warp (1, wSync, 60).osc (2, sync, 30, 60).pitch (2, 0, -9).warp (2, wSync, 40)
            .osc (3, analog, squarePos, 50).levelEnv (3, 150, 0, 1500, 60).pitch (3, -12).filter (1, ladder24, 2600, 35, 40, 50)
            .lfoSync (1, triangle, quarter, 1).mod (lfo1, "osc1WarpAmount", 30).mod (lfo1, "osc2WarpAmount", -30)
            .lfo (2, sine, 6.0f, 1).lfoFade (2, 500).mod (lfo2, "osc1Fine", 10).env (2, 0, 300, 0, 200, 70).mod (env2, "filter1Cutoff", 20)
            .amp (1, 500, 90, 200).mono (60).drive (fuzz, 30, 5000).delay (eighth, 25, 18, 5000).level (-18)
            .macro (1, "Siren").mod (macro1, "osc1WarpAmount", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28)
            .macro (3, "Low").mod (macro3, "osc3Volume", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Ion Whip", "Lead").osc (1, fm, 50, 80).warp (1, wFm, 30).osc (2, sync, 50, 50).warp (2, wSync, 50).levelEnv (2, 0, 0, 250, 30)
            .osc (3, analog, sawPos, 60).pitch (3, -12).filter (1, ladder12, 2800, 25, 30, 50)
            .env (2, 0, 180, 0, 120, 80).mod (env2, "osc2WarpAmount", 50).mod (env2, "osc1WarpAmount", 25).env (3, 0, 90, 0, 60, 80).mod (env3, "osc1Pitch", 6.25f)
            .lfo (1, sine, 5.8f, 1).lfoFade (1, 600).mod (lfo1, "osc1Fine", 9).amp (1, 400, 85, 200).mono (30).drive (hard, 25, 6000)
            .delay (sixteenthD, 30, 18, 5000).reverb (15).level (-10)
            .macro (1, "Whip").mod (macro1, "osc3Volume", 40).macro (2, "FM").mod (macro2, "osc1WarpAmount", 40)
            .macro (3, "Sync").mod (macro3, "osc2WarpAmount", 40).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Wire Saint", "Lead").osc (1, sync, 20, 80).warp (1, wSync, 35).unison (1, 2, 8, 60).osc (2, vowel, 40, 45).levelEnv (2, 300, 0, 2000, 80)
            .osc (3, analog, sinePos, 70).pitch (3, -12).filter (1, ladder12, 3000, 20, 20, 50)
            .lfoSync (1, sine, bar1, 1).mod (lfo1, "osc1WarpAmount", 25).mod (lfo1, "osc2Position", 35)
            .lfo (2, sine, 5.2f, 1).lfoFade (2, 900).mod (lfo2, "osc1Fine", 9).amp (4, 700, 85, 350).mono (50).delay (quarterD, 35, 22, 4500)
            .reverb (25, 75, 3.5f).level (-5)
            .macro (1, "Sync").mod (macro1, "osc1WarpAmount", 40).macro (2, "Choir").mod (macro2, "osc2Volume", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Feedback Hawk", "Lead").osc (1, growl, 60, 75).warp (1, wFold, 35).osc (2, sync, 40, 60).pitch (2, 12).warp (2, wSync, 45)
            .osc (3, pulse, 15, 45).levelEnv (3, 0, 0, 400, 35).pitch (3, -12).filter (1, ladder24, 2200, 40, 40, 50).filter (2, comb, 880, 30, 0, 100)
            .lfoSync (1, sawDown, eighth, 1).mod (lfo1, "osc1Position", 25).mod (lfo1, "filter1Cutoff", 15)
            .lfo (2, sine, 5.5f, 1).lfoFade (2, 600).mod (lfo2, "osc1Fine", 10).mod (lfo2, "osc2Fine", 10).env (2, 0, 400, 20, 200, 60).mod (env2, "osc2WarpAmount", 35)
            .amp (1, 500, 85, 250).mono (40).drive (fuzz, 35, 5000).delay (eighthD, 35, 20, 4500).level (-20.5)
            .macro (1, "Hawk").mod (macro1, "filter2Resonance", 35).macro (2, "Fold").mod (macro2, "osc1WarpAmount", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Static Comet", "Lead").osc (1, sync, 70, 80).warp (1, wSync, 25).osc (2, analog, sawPos, 60).pitch (2, 0, 12).osc (3, metal, 40, 30).pitch (3, 24)
            .levelEnv (3, 0, 0, 150, 0).noise (12, 2).filter (1, ladder12, 3500, 25, 25, 50)
            .env (2, 0, 1200, 20, 400, 40).mod (env2, "osc1WarpAmount", 50).mod (env2, "filter1Cutoff", 18)
            .lfo (1, randomShape, 12, 0, 70).mod (lfo1, "osc1WarpAmount", 8).lfo (2, sine, 5.6f, 1).lfoFade (2, 700).mod (lfo2, "osc1Fine", 9)
            .amp (2, 700, 85, 300).mono (40).delay (eighthD, 30, 20, 5000).reverb (20).level (-5)
            .macro (1, "Comet").mod (macro1, "osc1WarpAmount", 40).macro (2, "Static").mod (macro2, "noiseLevel", 35)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Hex Wail", "Lead").osc (1, analog, sawPos, 75).warp (1, wMirror, 25).osc (2, analog, squarePos, 65).pitch (2, 7).warp (2, wSqueeze, 30)
            .osc (3, sync, 50, 40).pitch (3, 12).warp (3, wSync, 40).levelEnv (3, 200, 0, 1500, 60).filter (1, ladder24, 2400, 35, 30, 50)
            .lfoSync (1, sine, quarterT, 1).mod (lfo1, "osc1WarpAmount", 25).mod (lfo1, "osc2WarpAmount", -25)
            .lfo (2, triangle, 5.4f, 1).lfoFade (2, 600).mod (lfo2, "osc1Fine", 9).mod (lfo2, "osc2Fine", 9)
            .amp (2, 500, 85, 250).mono (55).drive (tube, 30, 5500).delay (quarter, 30, 18, 4500).reverb (16).level (-13)
            .macro (1, "Wail").mod (macro1, "osc3Volume", 45).macro (2, "Mirror").mod (macro2, "osc1WarpAmount", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Live Wire Lead", "Lead").osc (1, pulse, 20, 80).warp (1, wBend, 35).osc (2, sync, 30, 55).warp (2, wSync, 30).levelEnv (2, 0, 0, 400, 35)
            .osc (3, analog, sawPos, 50).pitch (3, -12).pitch (1, 0, -5).filter (1, ladder12, 2800, 30, 30, 50)
            .lfoSync (1, steps, bar1, 1, 30).stepPattern (1, { 100, 0, 60, 0, -60, 0, 100, 0, 40, 0, -100, 0, 80, 0, -40, 0 })
            .mod (lfo1, "osc2WarpAmount", 35).mod (lfo1, "filter1Cutoff", 12).lfo (2, sine, 5.6f, 1).lfoFade (2, 500).mod (lfo2, "osc1Fine", 9)
            .amp (1, 500, 85, 200).mono (30).drive (hard, 25, 6000).delay (sixteenthD, 30, 16, 5000).level (-12)
            .macro (1, "Wire").mod (macro1, "osc2WarpAmount", 40).macro (2, "Bend").mod (macro2, "osc1WarpAmount", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Solar Flare Lead", "Lead").osc (1, sync, 10, 80).warp (1, wSync, 80).osc (2, additive, 60, 50).pitch (2, 12).osc (3, analog, sawPos, 60).levelEnv (3, 300, 0, 2500, 75).unison (3, 3, 10, 70)
            .filter (1, ladder12, 3200, 25, 25, 50).env (2, 300, 1500, 40, 600, 40).mod (env2, "osc1WarpAmount", -50).mod (env2, "osc2Position", 30)
            .lfo (1, sine, 5.3f, 1).lfoFade (1, 900).mod (lfo1, "osc1Fine", 8).mod (lfo1, "osc3Fine", 8).lfo (2, drift, 0.2f, 0).mod (lfo2, "filter1Cutoff", 8)
            .amp (5, 800, 85, 400).mono (60).delay (quarterD, 35, 22, 4500).reverb (25, 75, 3.5f).level (-4.5)
            .macro (1, "Flare").mod (macro1, "osc1WarpAmount", 40).macro (2, "Glow").mod (macro2, "osc2Volume", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Space").mod (macro4, "fxReverbMix", 35);

        // Leads that sing: vowel tables and formants that open and close as a voice does.
        add ("Cantor Lead", "Lead").osc (1, vowel, 20, 80).osc (2, vowel, 60, 55).pitch (2, 0, 8).osc (3, analog, sawPos, 45).levelEnv (3, 0, 0, 800, 50).pitch (3, -12).route (1, 100)
            .parallel().filter (1, ladder12, 3000, 15, 0, 50).filter (2, svfBand, 1100, 40, 0, 40)
            .env (2, 0, 800, 30, 400, 50).mod (env2, "osc1Position", 40).mod (env2, "filter2Cutoff", 20)
            .lfo (1, sine, 5.0f, 1).lfoFade (1, 900).mod (lfo1, "osc1Fine", 10).mod (lfo1, "osc2Fine", 10).lfo (2, sine, 0.35f, 0).mod (lfo2, "osc2Position", 30)
            .amp (5, 700, 90, 350).mono (60).reverb (28, 75, 3.5f).delay (quarter, 25, 15, 4000).level (-5)
            .macro (1, "Vowel").mod (macro1, "osc1Position", 40).macro (2, "Formant").mod (macro2, "filter2Cutoff", 30)
            .macro (3, "Breath").mod (macro3, "noiseLevel", 25).noise (0, 1).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Talking Torch", "Lead").osc (1, analog, sawPos, 80).warp (1, wSqueeze, 25).osc (2, pulse, 30, 60).pitch (2, 12, 10).osc (3, vowel, 50, 65).levelEnv (3, 150, 0, 1500, 60).route (3, 100)
            .parallel().filter (1, ladder12, 1800, 30, 20, 50).filter (2, svfBand, 1300, 60, 20, 40)
            .lfoSync (1, steps, bar1, 1, 50).stepPattern (1, { -100, -100, 60, 60, 0, 0, 100, 100, -60, -60, 30, 30, 100, 100, -30, -30 })
            .mod (lfo1, "filter2Cutoff", 35).mod (lfo1, "osc3Position", 30).lfo (2, sine, 5.4f, 1).lfoFade (2, 700).mod (lfo2, "osc1Fine", 9)
            .amp (2, 600, 85, 250).mono (40).drive (tube, 25, 5000).delay (eighthD, 30, 18, 4500).level (-12.5)
            .macro (1, "Talk").mod (macro1, "filter2Resonance", 30).macro (2, "Vowel").mod (macro2, "osc3Position", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Oracle Throat", "Lead").osc (1, vowel, 70, 80).warp (1, wBend, 20).osc (2, growl, 30, 45).osc (3, analog, sinePos, 70).levelEnv (3, 0, 0, 400, 35).pitch (3, -12)
            .filter (1, svfBand, 1200, 45, 15, 50).filter (2, ladder12, 4500, 10)
            .lfoSync (1, sine, half, 1).mod (lfo1, "osc1Position", 40).mod (lfo1, "filter1Cutoff", 20)
            .lfo (2, sine, 4.8f, 1).lfoFade (2, 800).mod (lfo2, "osc1Fine", 10).env (2, 0, 400, 0, 200, 70).mod (env2, "osc2Volume", 30)
            .amp (3, 700, 85, 300).mono (50).delay (quarterD, 30, 18, 4500).reverb (22).level (-2.5)
            .macro (1, "Oracle").mod (macro1, "osc1Position", 40).macro (2, "Growl").mod (macro2, "osc2Volume", 40)
            .macro (3, "Formant").mod (macro3, "filter1Cutoff", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Muezzin Wire", "Lead").osc (1, vowel, 40, 75).osc (2, sync, 40, 50).warp (2, wSync, 35).osc (3, additive, 30, 40).pitch (3, 12).levelEnv (3, 400, 0, 2000, 70)
            .filter (1, ladder12, 3000, 20, 20, 50).env (2, 0, 1200, 30, 500, 40).mod (env2, "osc1Position", 50).mod (env2, "osc2WarpAmount", 30)
            .lfo (1, sine, 5.6f, 1).lfoFade (1, 1000).mod (lfo1, "osc1Fine", 14).mod (lfo1, "osc2Fine", 14).lfoSync (2, triangle, bar1, 1).mod (lfo2, "osc3Position", 30)
            .amp (8, 800, 85, 400).mono (90).delay (quarter, 35, 20, 4000).reverb (25, 70, 3.0f).level (-5)
            .macro (1, "Call").mod (macro1, "osc1Position", 40).macro (2, "Sync").mod (macro2, "osc2WarpAmount", 40)
            .macro (3, "Halo").mod (macro3, "osc3Volume", 40).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Gospel Engine", "Lead").osc (1, vowel, 10, 75).unison (1, 3, 10, 70).osc (2, vowel, 80, 60).pitch (2, 12).levelEnv (2, 300, 0, 3000, 80)
            .osc (3, analog, sawPos, 55).pitch (3, -12).filter (1, ladder12, 3200, 15, 0, 50)
            .lfo (1, sine, 0.25f, 0).mod (lfo1, "osc1Position", 35).mod (lfo1, "osc2Position", -35)
            .lfo (2, sine, 5.0f, 1).lfoFade (2, 900).mod (lfo2, "osc1Fine", 9).mod (lfo2, "osc2Fine", 9)
            .amp (8, 900, 90, 450).mono (50).chorus (25).reverb (30, 80, 4.0f).level (-4.5)
            .macro (1, "Choir").mod (macro1, "osc2Volume", 40).macro (2, "Vowel").mod (macro2, "osc1Position", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Larynx Lead", "Lead").osc (1, growl, 40, 80).osc (2, vowel, 50, 55).osc (3, sync, 30, 35).pitch (3, 12).warp (3, wSync, 40).levelEnv (3, 0, 0, 200, 10)
            .filter (1, svfBand, 800, 50, 25, 50).filter (2, ladder12, 4000, 10)
            .lfoSync (1, sawUp, quarter, 1, 20).mod (lfo1, "filter1Cutoff", 35).mod (lfo1, "osc2Position", 30)
            .lfo (2, sine, 5.5f, 1).lfoFade (2, 700).mod (lfo2, "osc1Fine", 9).amp (2, 500, 85, 250).mono (40).drive (tube, 30, 5000)
            .delay (eighthD, 30, 18, 4500).level (-8.5)
            .macro (1, "Larynx").mod (macro1, "filter1Resonance", 30).macro (2, "Vowel").mod (macro2, "osc2Position", 40)
            .macro (3, "Bite").mod (macro3, "osc3Volume", 45).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Phantom Aria", "Lead").osc (1, additive, 30, 75).osc (2, vowel, 30, 55).levelEnv (2, 500, 0, 3000, 90).osc (3, analog, sinePos, 70).pitch (3, -12)
            .noise (10, 1).filter (1, svfLow, 4500, 15, 0, 50).filter (2, svfBand, 1400, 30)
            .env (2, 200, 1500, 50, 800, 40).mod (env2, "osc1Position", 40).lfo (1, sine, 4.8f, 1).lfoFade (1, 1200).mod (lfo1, "osc1Fine", 10).mod (lfo1, "osc2Fine", 10)
            .lfo (2, drift, 0.15f, 0).mod (lfo2, "filter2Cutoff", 20).amp (20, 1000, 90, 600).mono (80).reverb (35, 85, 5.0f).delay (quarterD, 30, 18, 4000).level (-4)
            .macro (1, "Aria").mod (macro1, "osc2Volume", 40).macro (2, "Breath").mod (macro2, "noiseLevel", 30)
            .macro (3, "Air").mod (macro3, "filter2Cutoff", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Sermon Saw", "Lead").osc (1, analog, sawPos, 75).unison (1, 3, 12, 60).osc (2, vowel, 20, 60).osc (3, analog, squarePos, 50).levelEnv (3, 300, 0, 2500, 75).pitch (3, -12)
            .filter (1, ladder24, 1800, 35, 20, 50).filter (2, svfBand, 1000, 35, 0, 40)
            .env (2, 0, 600, 30, 300, 50).mod (env2, "osc2Position", 60).mod (env2, "filter1Cutoff", 20)
            .lfoSync (1, sine, quarter, 1).mod (lfo1, "filter2Cutoff", 25).lfo (2, sine, 5.2f, 1).lfoFade (2, 800).mod (lfo2, "osc1Fine", 9)
            .amp (2, 600, 85, 300).mono (45).drive (tube, 20, 5000).delay (eighthD, 30, 18, 4500).reverb (18).level (-7)
            .macro (1, "Sermon").mod (macro1, "osc2Position", 40).macro (2, "Formant").mod (macro2, "filter2Cutoff", 30)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Mantra Line", "Lead").osc (1, vowel, 50, 80).warp (1, wSqueeze, 20).osc (2, fm, 40, 40).pitch (2, 12).levelEnv (2, 0, 0, 300, 20)
            .osc (3, analog, sawPos, 50).pitch (3, -12).filter (1, ladder12, 2600, 20, 20, 50)
            .lfoSync (1, steps, bars2, 1, 60).stepPattern (1, { -100, -50, 0, 50, 100, 50, 0, -50, -100, 0, 100, 0, -100, 50, 100, -50 }).mod (lfo1, "osc1Position", 40)
            .lfo (2, sine, 5.0f, 1).lfoFade (2, 900).mod (lfo2, "osc1Fine", 9).amp (3, 700, 85, 350).mono (60).delay (quarter, 30, 18, 4000).reverb (20).level (-4)
            .macro (1, "Mantra").mod (macro1, "osc1Position", 40).macro (2, "Bell").mod (macro2, "osc2Volume", 40)
            .macro (3, "Squeeze").mod (macro3, "osc1WarpAmount", 40).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Siren Psalm", "Lead").osc (1, vowel, 90, 80).osc (2, sync, 20, 45).pitch (2, 7).warp (2, wSync, 30).levelEnv (2, 300, 0, 2500, 70)
            .osc (3, analog, trianglePos, 60).pitch (3, -12).filter (1, ladder12, 3200, 20, 0, 50)
            .env (3, 0, 300, 0, 200, 70).mod (env3, "osc1Pitch", 2.08f).lfo (1, sine, 5.4f, 1).lfoFade (1, 600).mod (lfo1, "osc1Fine", 12)
            .lfoSync (2, triangle, bar1, 1).mod (lfo2, "osc1Position", -40).amp (3, 700, 85, 350).mono (80).delay (quarterD, 35, 20, 4000).reverb (25).level (-6)
            .macro (1, "Psalm").mod (macro1, "osc1Position", 40).macro (2, "Fifth").mod (macro2, "osc2Volume", 40)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 40).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Hollow Chant", "Lead").osc (1, vowel, 30, 75).osc (2, vowel, 70, 75).pitch (2, 0, 15).pan (1, -40).pan (2, 40).osc (3, sub, 30, 60).levelEnv (3, 0, 0, 800, 50).pitch (3, -12)
            .filter (1, svfBand, 900, 40, 0, 50).filter (2, ladder12, 3500, 10)
            .lfo (1, triangle, 0.3f, 0).mod (lfo1, "osc1Position", 35).mod (lfo1, "osc2Position", -35).lfoSync (2, sine, bar1, 1).mod (lfo2, "filter1Cutoff", 25)
            .amp (10, 900, 90, 500).mono (70).chorus (20).reverb (30, 80, 4.0f).level (-4)
            .macro (1, "Chant").mod (macro1, "filter1Resonance", 30).macro (2, "Vowel").mod (macro2, "osc1Position", 40)
            .macro (3, "Width").mod (macro3, "osc1Pan", 40).macro (4, "Space").mod (macro4, "fxReverbMix", 35);

        // Anthem leads: wide detuned stacks with layers that bloom, gate and echo with the beat.
        add ("Skyline Stack", "Lead").osc (1, analog, sawPos, 75).unison (1, 7, 22, 100).osc (2, analog, sawPos, 60).pitch (2, 12).unison (2, 5, 18, 100)
            .levelEnv (2, 400, 0, 3000, 80).osc (3, analog, squarePos, 55).pitch (3, -12).filter (1, ladder12, 3500, 20, 0, 50)
            .env (2, 0, 600, 30, 300, 55).mod (env2, "filter1Cutoff", 20).lfoSync (1, sine, bars2, 1).mod (lfo1, "osc1UnisonDetune", 20)
            .lfo (2, sine, 5.2f, 1).lfoFade (2, 900).mod (lfo2, "osc1Fine", 6).amp (3, 700, 85, 350).delay (eighthD, 30, 20, 5000).reverb (25, 75, 3.0f).level (-14)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Octave").mod (macro2, "osc2Volume", 40)
            .macro (3, "Width").mod (macro3, "osc1UnisonSpread", -40).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Strobe Anthem", "Lead").osc (1, analog, sawPos, 75).unison (1, 6, 20, 100).osc (2, pulse, 40, 55).pitch (2, 7).unison (2, 3, 12, 80)
            .osc (3, analog, sawPos, 50).pitch (3, 12).levelEnv (3, 0, 0, 300, 20).filter (1, ladder12, 3000, 25, 0, 50)
            .lfoSync (1, steps, bar1, 1, 50).stepPattern (1, { 0, -100, 0, 0, -100, 0, -100, 0, 0, -100, 0, 0, -100, 0, 0, -100 }).mod (lfo1, "ampLevel", 45)
            .lfoSync (2, sawUp, bars2, 1).mod (lfo2, "filter1Cutoff", 20).amp (2, 600, 85, 250).delay (sixteenthD, 30, 18, 5000).reverb (18).level (-11.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Fifth").mod (macro2, "osc2Volume", 40)
            .macro (3, "Gate").mod (macro3, "ampLevel", -15).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Victory Lap", "Lead").osc (1, analog, sawPos, 75).unison (1, 5, 18, 90).osc (2, sync, 30, 45).warp (2, wSync, 30).pitch (2, 12)
            .osc (3, analog, squarePos, 60).levelEnv (3, 150, 0, 1500, 60).pitch (3, -12).unison (3, 2, 6, 40).filter (1, ladder12, 2800, 25, 15, 50)
            .env (2, 0, 900, 40, 400, 45).mod (env2, "osc2WarpAmount", 40).mod (env2, "filter1Cutoff", 18)
            .lfo (1, sine, 5.3f, 1).lfoFade (1, 800).mod (lfo1, "osc1Fine", 7).lfoSync (2, triangle, bar1, 1).mod (lfo2, "osc2WarpAmount", 15)
            .amp (2, 700, 85, 300).mono (40).delay (quarterD, 30, 20, 5000).reverb (22).level (-5.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Sync").mod (macro2, "osc2WarpAmount", 40)
            .macro (3, "Width").mod (macro3, "osc1UnisonSpread", -40).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Laser Choir Lead", "Lead").osc (1, analog, sawPos, 75).unison (1, 6, 20, 100).osc (2, vowel, 40, 55).unison (2, 3, 12, 80).levelEnv (2, 500, 0, 3000, 85)
            .osc (3, pulse, 20, 40).pitch (3, 12).levelEnv (3, 0, 0, 250, 15).filter (1, ladder12, 3200, 20, 0, 50)
            .lfo (1, sine, 0.2f, 0).mod (lfo1, "osc2Position", 35).lfo (2, sine, 5.0f, 1).lfoFade (2, 800).mod (lfo2, "osc1Fine", 6).mod (lfo2, "osc2Fine", 6)
            .amp (4, 800, 85, 400).chorus (20).delay (eighthD, 30, 18, 5000).reverb (28, 80, 3.5f).level (-8.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Choir").mod (macro2, "osc2Volume", 40)
            .macro (3, "Laser").mod (macro3, "osc3Volume", 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Rave Torch", "Lead").osc (1, analog, sawPos, 75).unison (1, 7, 25, 100).osc (2, analog, squarePos, 55).pitch (2, -12).unison (2, 3, 10, 70)
            .osc (3, fm, 50, 35).pitch (3, 12).levelEnv (3, 0, 0, 200, 0).filter (1, ladder24, 2400, 30, 20, 50)
            .env (2, 0, 250, 30, 150, 70).mod (env2, "filter1Cutoff", 25).lfoSync (1, sine, quarter, 1).mod (lfo1, "osc1UnisonDetune", 20)
            .lfoSync (2, sawUp, bars2, 1).mod (lfo2, "filter1Cutoff", 20).amp (1, 500, 85, 200).drive (tube, 20, 6000).delay (eighthD, 30, 18, 5000).reverb (15).level (-15)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Torch").mod (macro2, "osc1UnisonDetune", 40)
            .macro (3, "Ping").mod (macro3, "osc3Volume", 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Wide Signal Lead", "Lead").osc (1, pulse, 30, 70).unison (1, 4, 14, 100).pan (1, -50).osc (2, pulse, 70, 70).unison (2, 4, 14, 100).pan (2, 50).pitch (2, 0, 8)
            .osc (3, analog, sawPos, 55).pitch (3, 12).levelEnv (3, 300, 0, 2000, 60).filter (1, ladder12, 3000, 20, 0, 50)
            .lfoSync (1, sine, half, 1).mod (lfo1, "osc1Position", 25).mod (lfo1, "osc2Position", -25)
            .lfo (2, sine, 5.2f, 1).lfoFade (2, 800).mod (lfo2, "osc1Fine", 7).mod (lfo2, "osc2Fine", 7)
            .amp (2, 600, 85, 300).delay (quarterD, 30, 20, 5000).reverb (20).level (-14)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Width").mod (macro2, "osc1Pan", 40)
            .macro (3, "Octave").mod (macro3, "osc3Volume", 40).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Hyperion Lead", "Lead").osc (1, analog, sawPos, 75).unison (1, 6, 18, 100).osc (2, additive, 50, 50).pitch (2, 19).levelEnv (2, 600, 0, 3000, 70)
            .osc (3, analog, sawPos, 60).pitch (3, -12).unison (3, 3, 10, 80).filter (1, ladder12, 2600, 25, 10, 50)
            .env (2, 300, 2000, 50, 800, 40).mod (env2, "filter1Cutoff", 22).mod (env2, "osc2Position", 30)
            .lfo (1, drift, 0.2f, 0).mod (lfo1, "osc1UnisonDetune", 15).lfo (2, sine, 5.0f, 1).lfoFade (2, 1000).mod (lfo2, "osc1Fine", 7)
            .amp (10, 900, 90, 500).delay (quarter, 35, 22, 4500).reverb (30, 85, 4.5f).level (-12)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Halo").mod (macro2, "osc2Volume", 40)
            .macro (3, "Width").mod (macro3, "osc1UnisonSpread", -40).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Gate Crasher Lead", "Lead").osc (1, analog, sawPos, 75).unison (1, 5, 20, 90).osc (2, sync, 40, 50).warp (2, wSync, 40).pitch (2, 12)
            .osc (3, analog, squarePos, 55).levelEnv (3, 0, 0, 400, 35).pitch (3, -12).filter (1, ladder24, 2600, 30, 20, 50)
            .lfoSync (1, steps, bar1, 1, 40).stepPattern (1, { 0, -100, 0, -100, 0, 0, -100, 0, 0, -100, 0, -100, 0, -100, 0, 0 }).mod (lfo1, "ampLevel", 45)
            .lfoSync (2, triangle, bar1, 1).mod (lfo2, "osc2WarpAmount", 30).mod (lfo2, "filter1Cutoff", 12)
            .amp (1, 500, 85, 200).drive (tube, 20, 6000).delay (sixteenthD, 25, 15, 5000).reverb (15).level (-15)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Sync").mod (macro2, "osc2WarpAmount", 40)
            .macro (3, "Gate").mod (macro3, "ampLevel", -15).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Supernova Lead", "Lead").osc (1, analog, sawPos, 75).unison (1, 7, 24, 100).osc (2, fm, 60, 40).pitch (2, 12).levelEnv (2, 0, 0, 500, 20)
            .osc (3, vowel, 40, 45).levelEnv (3, 800, 0, 3000, 80).filter (1, ladder12, 2800, 25, 10, 50)
            .env (2, 0, 1500, 30, 500, 45).mod (env2, "filter1Cutoff", 22).mod (env2, "osc2Position", 35)
            .lfo (1, sine, 0.15f, 0).mod (lfo1, "osc3Position", 30).lfo (2, sine, 5.2f, 1).lfoFade (2, 900).mod (lfo2, "osc1Fine", 7)
            .amp (3, 800, 85, 400).chorus (20).delay (eighthD, 30, 20, 5000).reverb (28).level (-9)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Nova").mod (macro2, "osc2Volume", 40)
            .macro (3, "Voice").mod (macro3, "osc3Volume", 40).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Prism Saw Lead", "Lead").osc (1, analog, sawPos, 75).unison (1, 5, 16, 100).osc (2, analog, sawPos, 55).pitch (2, 7).unison (2, 3, 12, 100)
            .osc (3, analog, sawPos, 50).pitch (3, 12).unison (3, 3, 12, 100).levelEnv (3, 300, 0, 2000, 60).filter (1, ladder12, 3000, 20, 0, 50)
            .lfoSync (1, triangle, bar1, 1).mod (lfo1, "osc2Volume", 25).mod (lfo1, "osc3Volume", -25)
            .lfo (2, sine, 5.3f, 1).lfoFade (2, 800).mod (lfo2, "osc1Fine", 7).amp (2, 700, 85, 350).delay (quarterD, 30, 20, 5000).reverb (22).level (-13)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Fifth").mod (macro2, "osc2Volume", 40)
            .macro (3, "Octave").mod (macro3, "osc3Volume", 40).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Midnight Arena", "Lead").osc (1, analog, sawPos, 75).unison (1, 6, 20, 100).osc (2, growl, 30, 45).pitch (2, -12).warp (2, wFold, 20)
            .osc (3, sync, 40, 40).pitch (3, 12).warp (3, wSync, 35).levelEnv (3, 0, 0, 300, 20).filter (1, ladder24, 1300, 45, 25, 50)
            .env (2, 0, 400, 30, 250, 60).mod (env2, "filter1Cutoff", 22).mod (env2, "osc3WarpAmount", 35)
            .lfoSync (1, triangle, quarterT, 1).mod (lfo1, "osc2Position", 30).mod (lfo1, "filter1Cutoff", 18).lfo (2, sine, 5.2f, 1).lfoFade (2, 800).mod (lfo2, "osc1Fine", 7)
            .amp (2, 600, 85, 300).phaser (40, 0.2f, 60).drive (tube, 25, 6000).delay (eighthD, 30, 18, 5000).reverb (22).level (-14.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Growl").mod (macro2, "osc2Volume", 40)
            .macro (3, "Bite").mod (macro3, "osc3Volume", 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);

        // Glass and metal: FM and metal tables, bell-like attacks over a sustaining voice.
        add ("Crystal Engine", "Lead").osc (1, fm, 40, 80).warp (1, wFm, 20).osc (2, metal, 50, 40).pitch (2, 12).levelEnv (2, 0, 0, 250, 10)
            .osc (3, analog, sinePos, 70).pitch (3, -12).filter (1, svfLow, 5000, 15, 0, 50)
            .env (2, 0, 800, 30, 300, 50).mod (env2, "osc1WarpAmount", 35).mod (env2, "osc1Position", 25)
            .lfo (1, sine, 5.2f, 1).lfoFade (1, 700).mod (lfo1, "osc1Fine", 8).lfoSync (2, triangle, bar1, 1).mod (lfo2, "osc1Position", 20)
            .amp (2, 700, 80, 350).mono (30).delay (quarter, 30, 22, 5000).reverb (25).level (-9.5)
            .macro (1, "Crystal").mod (macro1, "osc2Volume", 45).macro (2, "FM").mod (macro2, "osc1WarpAmount", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Chrome Kestrel", "Lead").osc (1, metal, 30, 70).osc (2, fm, 60, 65).warp (2, wFm, 30).osc (3, analog, sawPos, 45).levelEnv (3, 300, 0, 2500, 75).pitch (3, -12)
            .filter (1, ladder12, 3800, 20, 15, 50).env (2, 0, 300, 20, 150, 70).mod (env2, "osc1Position", 40).mod (env2, "osc2WarpAmount", 30)
            .lfoSync (1, sine, eighthT, 1).mod (lfo1, "osc1Position", 15).lfo (2, sine, 5.6f, 1).lfoFade (2, 500).mod (lfo2, "osc2Fine", 9)
            .amp (1, 500, 80, 250).mono (30).drive (hard, 15, 7000).delay (eighthD, 30, 18, 5000).reverb (16).level (-5)
            .macro (1, "Chrome").mod (macro1, "osc1Volume", 40).macro (2, "FM").mod (macro2, "osc2WarpAmount", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Glass Harpoon", "Lead").osc (1, fm, 70, 75).osc (2, additive, 20, 55).pitch (2, 12).osc (3, metal, 70, 30).pitch (3, 24).levelEnv (3, 0, 0, 90, 0)
            .filter (1, svfLow, 6000, 10, 0, 50).env (2, 0, 200, 10, 120, 80).mod (env2, "osc1Position", 40)
            .env (3, 0, 60, 0, 40, 80).mod (env3, "osc1Pitch", 4.17f).lfo (1, sine, 5.5f, 1).lfoFade (1, 600).mod (lfo1, "osc1Fine", 9)
            .lfoSync (2, sine, half, 1).mod (lfo2, "osc2Position", 30).amp (0, 500, 75, 300).mono (20).delay (eighthD, 35, 22, 5000).reverb (22).level (-5.5)
            .macro (1, "Echo").mod (macro1, "fxDelayFeedback", 40).macro (2, "Glass").mod (macro2, "osc3Volume", 45)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Mercury Thread", "Lead").osc (1, fm, 20, 80).warp (1, wFm, 45).osc (2, fm, 80, 50).pitch (2, 0, 7).osc (3, analog, trianglePos, 60).levelEnv (3, 0, 0, 800, 50).pitch (3, -12)
            .filter (1, ladder12, 4000, 15, 0, 50).lfo (1, sine, 0.3f, 0).mod (lfo1, "osc1WarpAmount", 25).mod (lfo1, "osc2Position", -30)
            .lfo (2, sine, 5.0f, 1).lfoFade (2, 900).mod (lfo2, "osc1Fine", 8).mod (lfo2, "osc2Fine", 8).env (2, 0, 600, 30, 300, 55).mod (env2, "osc1WarpAmount", 25)
            .amp (3, 700, 85, 350).mono (60).chorus (20).delay (quarterD, 30, 20, 5000).reverb (22).level (-5.5)
            .macro (1, "Mercury").mod (macro1, "osc1WarpAmount", 40).macro (2, "Shimmer").mod (macro2, "osc2Volume", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Ice Lattice", "Lead").osc (1, additive, 70, 75).osc (2, metal, 20, 40).pitch (2, 7).levelEnv (2, 0, 0, 300, 15).osc (3, fm, 40, 50).pitch (3, -12)
            .filter (1, svfLow, 5500, 15, 0, 50).filter (2, comb, 1760, 25, 0, 100)
            .lfoSync (1, steps, bar1, 1, 40).stepPattern (1, { 100, 0, 50, 0, -50, 0, 100, 0, -100, 0, 50, 0, 0, 0, -50, 0 }).mod (lfo1, "osc1Position", 30)
            .lfo (2, sine, 5.2f, 1).lfoFade (2, 700).mod (lfo2, "osc1Fine", 8).amp (2, 600, 80, 350).mono (40).delay (sixteenthD, 35, 22, 5000).reverb (25).level (-1)
            .macro (1, "Lattice").mod (macro1, "filter2Resonance", 35).macro (2, "Ice").mod (macro2, "osc2Volume", 45)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Titanium Flute", "Lead").osc (1, additive, 10, 80).osc (2, fm, 30, 35).pitch (2, 12).levelEnv (2, 0, 0, 150, 10).osc (3, analog, sinePos, 50).pitch (3, -12)
            .noise (18, 1, 40).filter (1, svfLow, 4500, 20, 0, 50).filter (2, svfBand, 2200, 20)
            .env (2, 80, 400, 50, 300, 50).mod (env2, "noiseLevel", 30).lfo (1, sine, 5.0f, 1).lfoFade (1, 900).mod (lfo1, "osc1Fine", 10)
            .lfo (2, drift, 0.25f, 0).mod (lfo2, "osc1Position", 20).amp (40, 700, 90, 300).mono (60).delay (quarter, 30, 20, 5000).reverb (25).level (6)
            .macro (1, "Breath").mod (macro1, "noiseLevel", 30).macro (2, "Edge").mod (macro2, "osc2Volume", 45)
            .macro (3, "Tone").mod (macro3, "osc1Position", 40).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Frostbite Lead", "Lead").osc (1, fm, 60, 75).warp (1, wCrush, 15).osc (2, metal, 40, 40).pitch (2, 12).osc (3, pulse, 30, 50).pitch (3, -12)
            .levelEnv (2, 0, 0, 200, 15).filter (1, ladder12, 3500, 25, 20, 50).env (2, 0, 250, 20, 150, 70).mod (env2, "osc1Position", 35)
            .lfoSync (1, square, sixteenth, 1, 40).mod (lfo1, "osc1WarpAmount", 15).lfo (2, sine, 5.4f, 1).lfoFade (2, 600).mod (lfo2, "osc1Fine", 8)
            .amp (1, 500, 80, 250).mono (30).drive (crush, 15, 7000, 40).delay (eighthD, 30, 18, 5000).reverb (18).level (-5.5)
            .macro (1, "Frost").mod (macro1, "osc1WarpAmount", 40).macro (2, "Metal").mod (macro2, "osc2Volume", 45)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Mirror Bell Lead", "Lead").osc (1, fm, 50, 75).warp (1, wMirror, 30).osc (2, additive, 40, 55).pitch (2, 0, 5).osc (3, metal, 30, 35).pitch (3, 19).levelEnv (3, 0, 0, 400, 0)
            .filter (1, svfLow, 5000, 15, 0, 50).env (2, 0, 1000, 30, 400, 45).mod (env2, "osc1WarpAmount", 35).mod (env2, "osc2Position", 25)
            .lfo (1, sine, 0.2f, 0).mod (lfo1, "osc1Position", 25).lfo (2, sine, 5.1f, 1).lfoFade (2, 900).mod (lfo2, "osc1Fine", 8).mod (lfo2, "osc2Fine", 8)
            .amp (2, 800, 80, 400).mono (40).chorus (18).delay (quarterD, 30, 22, 5000).reverb (28).level (-7)
            .macro (1, "Mirror").mod (macro1, "osc1WarpAmount", 40).macro (2, "Bell").mod (macro2, "osc3Volume", 45)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Anvil Spark Lead", "Lead").osc (1, metal, 60, 60).levelEnv (1, 0, 0, 200, 10).osc (2, sync, 30, 70).warp (2, wSync, 35).osc (3, fm, 50, 50).pitch (3, -12)
            .filter (1, ladder12, 3000, 25, 25, 50).env (2, 0, 500, 20, 250, 60).mod (env2, "osc2WarpAmount", 40).mod (env2, "osc3Position", 25)
            .lfoSync (1, sine, quarter, 1).mod (lfo1, "osc2WarpAmount", 15).lfo (2, sine, 5.5f, 1).lfoFade (2, 600).mod (lfo2, "osc2Fine", 9)
            .amp (1, 600, 85, 300).mono (30).drive (hard, 20, 6000).delay (eighthD, 30, 18, 5000).reverb (16).level (-8.5)
            .macro (1, "Anvil").mod (macro1, "osc1Volume", 40).macro (2, "Sync").mod (macro2, "osc2WarpAmount", 40)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 28).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
    }
}
