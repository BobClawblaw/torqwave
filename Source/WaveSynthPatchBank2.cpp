#include "WaveSynthPatchBuilder.h"

// Torqwave Synth's second patch bank: 1,493 more patches, in the first bank's categories and in new
// ones (Sub, Chord, Arp, Keys, Organ, Brass, Strings, Cinematic, Drum, FX, Glitch, Game and Chip).
// Scripts/make_patch_bank.py wrote this file: it drew each of 193 kinds of sound
// (Scripts/patch_bank_kinds.py) many times, played every candidate through the synth, and kept the ones
// that sound in their register, do not click, have four macros that are heard and are not a near-copy
// of another patch in their category; then it set each one's level so a held chord peaks near -9 dB.
// Tests/WaveSynthBankTests.cpp holds the bank to the same things.

namespace WaveSynthPatchBuilder
{
    void addBank2 (std::vector<Builder>& b)
    {
        const auto add = [&b] (const char* name, const char* category) -> Builder& { b.emplace_back (name, category); return b.back(); };

        // Bass, by name.
        add ("Afterhours File", "Bass").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, 12, 16).unison (1, 2, 8, 40).subOsc (30)
            .filter (1, ladder24, 420, 30, 20).env (2, 0, 80, 0, 120, 88).mod (env2, "filter1Cutoff", 32).mod (velocity, "filter1Cutoff", 10)
            .amp (0, 350, 0, 80).mono (0, false).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).macro (3, "Sub").mod (macro3, "subLevel", 40).macro (4, "Beat").mod (macro4, "osc2Fine", 20);
        add ("Amber Valve", "Bass").osc (1, sync, 10).subOsc (35).filter (1, ladder12, 2100, 35, 30).env (2, 0, 240, 10, 80, 85)
            .mod (env2, "osc1Position", 40).mod (env2, "filter1Cutoff", 8).amp (1, 460, 60, 80).mono (70).drive (hard, 35, 2700).level (-14)
            .macro (1, "Sync").mod (macro1, "osc1Position", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).reverb (0, 50, 1.2f, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Bite").mod (macro4, "filter1Drive", 60);
        add ("Bare Shove", "Bass").osc (1, fm, 55).subOsc (30).env (2, 0, 90, 0, 60, 92).mod (env2, "osc1Pitch", 14.583f).env (3, 0, 80, 0, 100, 80)
            .mod (env3, "osc1Position", 20).filter (1, ladderBand, 850, 35).amp (0, 160, 0, 80, 75).mono (0, false).drive (soft, 20, 7300)
            .level (-9).macro (1, "Knock").mod (macro1, "osc1Position", 35).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25)
            .macro (3, "Resonance").mod (macro3, "filter1Resonance", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 45);
        add ("Blunt Shove", "Bass").osc (1, fm, 60).subOsc (35).env (2, 0, 90, 0, 60, 84).mod (env2, "osc1Pitch", 39.583f)
            .env (3, 0, 140, 0, 100, 80).mod (env3, "osc1Position", 15).filter (1, ladderBand, 1300, 40, 10).amp (0, 380, 0, 110, 70)
            .mono (0, false).drive (soft, 20, 8300).level (-10).macro (1, "Knock").mod (macro1, "osc1Position", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Resonance").mod (macro3, "filter1Resonance", 35).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 45);
        add ("Bottomless Gullet", "Bass").osc (1, sub, 40).warp (1, wFold, 35).subOsc (40).filter (1, ladder24, 330, 25, 50)
            .lfoSync (1, sawDown, eighth).mod (lfo1, "filter1Cutoff", 20).mod (lfo1, "osc1WarpAmount", 20).amp (2, 300, 90, 120).mono (30)
            .drive (tube, 25, 4300).level (-9).macro (1, "Fold").mod (macro1, "osc1WarpAmount", 45).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 28).macro (3, "Growl").mod (macro3, "osc1Position", 35).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Bottomless Saw", "Bass").osc (1, analog, sawPos).osc (2, analog, squarePos, 70).pitch (2, 0, 7).unison (1, 2, 12, 40).subOsc (35)
            .filter (1, ladder12, 340, 40, 30, 30).env (2, 0, 140, 10, 120, 88).mod (env2, "filter1Cutoff", 42).mod (velocity, "filter1Cutoff", 15)
            .amp (1, 300, 60, 70).mono (0, false).level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).macro (3, "Beat").mod (macro3, "osc2Fine", 20).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Brass Flange", "Bass").osc (1, metal, 45).subOsc (45, 0, 1).filter (1, ladder12, 2100, 35, 30).env (2, 0, 100, 0, 80, 75)
            .mod (env2, "filter1Cutoff", 14).mod (env2, "osc1Position", 30).amp (1, 220, 60, 70).mono (0, false).drive (hard, 15, 2700).level (-4)
            .macro (1, "Clang").mod (macro1, "osc1Position", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 35).reverb (0, 45, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Caustic Flange", "Bass").osc (1, metal, 5).subOsc (50).filter (1, ladderBand, 620, 25, 20).env (2, 0, 180, 0, 100, 80)
            .mod (env2, "filter1Cutoff", 22).mod (env2, "osc1Position", 25).amp (1, 260, 0, 100).mono (0, false).drive (fuzz, 40, 2500).level (-18)
            .macro (1, "Clang").mod (macro1, "osc1Position", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 35).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Cavernous Lever", "Bass").osc (1, sync, 25).subOsc (45).filter (1, ladder24, 670, 30, 40).env (2, 0, 290, 0, 80, 75)
            .mod (env2, "osc1Position", 40).mod (env2, "filter1Cutoff", 18).amp (1, 420, 80, 100).mono().level (-3).macro (1, "Sync")
            .mod (macro1, "osc1Position", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).drive (hard, 0, 6000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Bite").mod (macro4, "filter1Drive", 60);
        add ("Charred Manifold", "Bass").osc (1, analog, 80).subOsc (45).filter (1, ladder24, 520, 30, 20).filter (2, comb, 392, 60, 0, 100)
            .env (2, 0, 180, 0, 120, 75).mod (env2, "filter1Cutoff", 15).amp (1, 280, 0, 100).mono (0, false).drive (tube, 15, 3000).level (-5)
            .macro (1, "Pipe").mod (macro1, "filter2Resonance", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Tune")
            .mod (macro3, "filter2Cutoff", 12).macro (4, "Resonance").mod (macro4, "filter1Resonance", 35);
        add ("Chemical Gullet", "Bass").osc (1, fm, 40).warp (1, wFold, 30).subOsc (30).filter (1, ladder24, 360, 35, 60).lfoSync (1, sine, half)
            .mod (lfo1, "filter1Cutoff", 18).mod (lfo1, "osc1WarpAmount", 30).amp (2, 300, 95, 110).mono().drive (tube, 45, 4000).level (-13)
            .macro (1, "Fold").mod (macro1, "osc1WarpAmount", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 40).macro (4, "Growl").mod (macro4, "osc1Position", 35);
        add ("Clinical Guy", "Bass").osc (1, string, 60, 90).warp (1, wOff, 70).unison (1, 2, 10, 40, 4).subOsc (30, 0, 1)
            .filter (1, svfLow, 1100, 20, 40).env (2, 0, 300, 0, 120, 75).mod (env2, "filter1Cutoff", 20).mod (velocity, "osc1Position", 25)
            .amp (1, 1800, 40, 120).mono (0, false).drive (fuzz, 40, 2800).level (-21).macro (1, "Bright").mod (macro1, "osc1Position", 30)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Sub").mod (macro3, "subLevel", 40).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 40);
        add ("Coal Undercroft", "Bass").osc (1, analog, 10).subOsc (30).env (2, 0, 110, 0, 60, 75).mod (env2, "osc1Pitch", 25)
            .filter (1, ladder24, 200, 10, 20).mod (env2, "filter1Cutoff", 25).amp (4, 1100, 0, 350, 75).reverb (30, 95, 2.5f, 70)
            .drive (soft, 45, 930).level (-20).macro (1, "Tail").mod (macro1, "fxReverbMix", 40).macro (2, "Thump").mod (macro2, "filter1Drive", 60)
            .macro (3, "Resonance").mod (macro3, "filter1Resonance", 40).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Compact Bore", "Bass").osc (1, analog, sinePos, 1).pitch (1, 0).osc (2, analog, 10).warp (2, wFm, 10).subOsc (50)
            .env (2, 0, 180, 0, 120, 60).mod (env2, "osc2WarpAmount", 30).mod (velocity, "osc2WarpAmount", 8).filter (1, svfLow, 1300, 15)
            .amp (1, 320, 0, 70).mono (50).drive (soft, 25, 6900).level (-11).macro (1, "Index").mod (macro1, "osc2WarpAmount", 45)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).reverb (0, 45, 1, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 30);
        add ("Curt Byte", "Bass").osc (1, analog, trianglePos).warp (1, wCrush, 75).subOsc (50).filter (1, svfLow, 1800).env (2, 0, 140, 0, 120, 75)
            .mod (env2, "filter1Cutoff", 25).amp (1, 240, 0, 60).mono (0, false).drive (crush, 20, 4500).level (-6).macro (1, "Steps")
            .mod (macro1, "osc1WarpAmount", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 35).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Damped Fork", "Bass").osc (1, analog, 80).route (1, 0).osc (2, sync, 70, 65).pitch (2, 7, 5).route (2, 100).subOsc (40).parallel()
            .filter (1, ladder24, 220, 35, 20).filter (2, ladderBand, 2000, 40, 0, 50).env (2, 0, 130, 0, 120, 85).mod (env2, "filter1Cutoff", 15)
            .mod (env2, "filter2Cutoff", 8).amp (1, 200, 0, 90).mono (0, false).drive (hard, 30, 3800).level (-14).macro (1, "Weight")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Bite").mod (macro2, "filter2Cutoff", 20).macro (3, "Edge")
            .mod (macro3, "filter2Resonance", 30).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Dense Couple", "Bass").osc (1, analog, 80).route (1, 0).osc (2, sync, 70, 60).pitch (2, 12, 7).route (2, 100).subOsc (50).parallel()
            .filter (1, ladder24, 540, 35, 30).filter (2, svfBand, 3100, 40).env (2, 0, 160, 0, 120, 70).mod (env2, "filter1Cutoff", 25)
            .mod (env2, "filter2Cutoff", 20).amp (1, 200, 80, 90).mono().drive (tube, 40, 11000).level (-15).macro (1, "Weight")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Bite").mod (macro2, "filter2Cutoff", 20).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 45).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Dented Jolt", "Bass").osc (1, fm, 45).subOsc (35).env (2, 0, 50, 0, 60, 80).mod (env2, "osc1Pitch", 25).env (3, 0, 80, 0, 100, 75)
            .mod (env3, "osc1Position", 35).filter (1, ladderBand, 1500, 15, 30).amp (0, 360, 0, 60, 60).mono (0, false).drive (hard, 20, 5700)
            .level (-11).macro (1, "Knock").mod (macro1, "osc1Position", 35).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25)
            .reverb (0, 40, 0.8f, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 45);
        add ("Depot Bore", "Bass").osc (1, analog, sinePos, 1).pitch (1, 24).osc (2, analog, sinePos).warp (2, wFm, 15).subOsc (35)
            .env (2, 0, 220, 0, 160, 65).mod (env2, "osc2WarpAmount", 40).mod (velocity, "osc2WarpAmount", 8).filter (1, ladder24, 920, 20)
            .amp (1, 320, 0, 70).mono (50).drive (soft, 20, 8200).level (-5).macro (1, "Index").mod (macro1, "osc2WarpAmount", 45)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).reverb (0, 45, 1, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 30)
            .macro (4, "Bite").mod (macro4, "filter1Drive", 60);
        add ("Derelict Grip", "Bass").osc (1, sub, 40).warp (1, wFold, 35).subOsc (45).filter (1, ladder24, 460, 45, 60)
            .lfoSync (1, triangle, quarterT).mod (lfo1, "filter1Cutoff", 22).mod (lfo1, "osc1WarpAmount", 20).amp (2, 300, 85, 110).mono (60)
            .drive (hard, 45, 3500).level (-15).macro (1, "Fold").mod (macro1, "osc1WarpAmount", 45).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 28).macro (3, "Growl").mod (macro3, "osc1Position", 35).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 40);
        add ("Derelict Knock", "Bass").osc (1, fm, 20).subOsc (45).env (2, 0, 40, 0, 60, 92).mod (env2, "osc1Pitch", 39.583f)
            .env (3, 0, 140, 0, 100, 85).mod (env3, "osc1Position", 20).filter (1, ladderBand, 1700, 25, 40).amp (0, 180, 0, 60, 70).mono (0, false)
            .drive (soft, 35, 3600).level (-11).macro (1, "Knock").mod (macro1, "osc1Position", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 45).reverb (0, 40, 0.8f, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 30);
        add ("Desert Vent", "Bass").osc (1, analog, sawPos).subOsc (35).filter (1, ladder24, 370, 20, 30).filter (2, comb, 196, 65, 0, 100)
            .env (2, 0, 160, 0, 120, 75).mod (env2, "filter1Cutoff", 30).lfo (1, drift, 0.13f, 0).mod (lfo1, "filter2Cutoff", 3)
            .amp (1, 320, 60, 80).mono (0, false).level (0).macro (1, "Pipe").mod (macro1, "filter2Resonance", 25).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 28).macro (3, "Tune").mod (macro3, "filter2Cutoff", 12).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 35);
        add ("District Tap", "Bass").osc (1, analog, sinePos, 1).pitch (1, 19).osc (2, analog, trianglePos).warp (2, wFm, 10).subOsc (30)
            .env (2, 0, 200, 10, 100, 80).mod (env2, "osc2WarpAmount", 50).mod (velocity, "osc2WarpAmount", 18).filter (1, ladder12, 920, 20)
            .amp (1, 460, 50, 110).mono().drive (soft, 15, 3200).level (-5).macro (1, "Index").mod (macro1, "osc2WarpAmount", 45)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Sub").mod (macro3, "subLevel", 40).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Drowned Shear", "Bass").osc (1, analog, sawPos).osc (2, analog, squarePos).pitch (2, 12, 5).unison (1, 2, 12, 50).subOsc (30)
            .filter (1, ladder24, 560, 25, 50, 30).env (2, 0, 130, 0, 80, 88).mod (env2, "filter1Cutoff", 42).mod (velocity, "filter1Cutoff", 10)
            .amp (1, 150, 0, 60).mono (0, false).level (-1).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).drive (hard, 0, 6000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Sub")
            .mod (macro4, "subLevel", 40);
        add ("Dry Drawl", "Bass").osc (1, vowel, 5).subOsc (40, 0, 1).env (2, 0, 240, 20, 150, 80).mod (env2, "osc1Position", -50)
            .filter (1, ladder12, 700, 25, 40).mod (env2, "filter1Cutoff", 14).amp (2, 400, 70, 120).mono (40).drive (tube, 25, 3200).level (-9)
            .macro (1, "Vowel").mod (macro1, "osc1Position", 50).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 45).macro (4, "Resonance").mod (macro4, "filter1Resonance", 35);
        add ("Dry Growl", "Bass").osc (1, vowel, 5).subOsc (55, 0, 1).osc (2, analog, sawPos, 45).pitch (2, -12, 4).env (2, 0, 240, 0, 150, 60)
            .mod (env2, "osc1Position", 25).filter (1, ladder12, 1700, 15, 30).mod (env2, "filter1Cutoff", 10).amp (2, 550, 0, 80).mono (80)
            .drive (tube, 30, 4400).level (-9).macro (1, "Vowel").mod (macro1, "osc1Position", 50).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 45);
        add ("Encrypted Nibble", "Bass").osc (1, analog, 50).warp (1, wCrush, 50).subOsc (50, 0, 2).filter (1, ladder12, 1200, 20)
            .env (2, 0, 110, 0, 120, 75).mod (env2, "filter1Cutoff", 20).amp (1, 180, 0, 90).mono (0, false).drive (crush, 25, 5300).level (-4)
            .macro (1, "Steps").mod (macro1, "osc1WarpAmount", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Sub")
            .mod (macro3, "subLevel", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 45);
        add ("Entombed Sluice", "Bass").osc (1, analog, 80).subOsc (35).filter (1, ladder24, 370, 20, 40).filter (2, comb, 261.6f, 55, 0, 100)
            .env (2, 0, 280, 0, 120, 75).mod (env2, "filter1Cutoff", 25).lfo (1, sine, 0.084f, 0).mod (lfo1, "filter2Cutoff", 4)
            .amp (1, 460, 0, 100).mono (0, false).level (-4).macro (1, "Pipe").mod (macro1, "filter2Resonance", 25).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 28).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub")
            .mod (macro4, "subLevel", 40);
        add ("Fatal Jack", "Bass").osc (1, sync, 40).subOsc (55, 0, 1).filter (1, ladder24, 1700, 35, 10).env (2, 0, 290, 0, 160, 85)
            .mod (env2, "osc1Position", 25).mod (env2, "filter1Cutoff", 16).amp (1, 300, 60, 70).mono (40).level (-4).macro (1, "Sync")
            .mod (macro1, "osc1Position", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Sub").mod (macro3, "subLevel", 40)
            .drive (hard, 0, 6000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Fatal Stanchion", "Bass").osc (1, metal, 35).subOsc (40).filter (1, ladder24, 710, 40, 20).env (2, 0, 180, 0, 140, 70)
            .mod (env2, "filter1Cutoff", 22).mod (env2, "osc1Position", 25).amp (1, 440, 0, 100).mono().drive (hard, 30, 4200).level (-6)
            .macro (1, "Clang").mod (macro1, "osc1Position", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).reverb (0, 45, 1, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 35);
        add ("Feverish Awl", "Bass").osc (1, analog, sinePos, 1).pitch (1, 24).osc (2, analog, trianglePos).warp (2, wFm, 15).subOsc (30)
            .env (2, 0, 100, 0, 120, 70).mod (env2, "osc2WarpAmount", 25).mod (velocity, "osc2WarpAmount", 14).filter (1, ladder24, 1000)
            .amp (1, 300, 75, 70).mono().drive (tube, 25, 3900).level (-3).macro (1, "Index").mod (macro1, "osc2WarpAmount", 45).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Bite").mod (macro3, "filter1Drive", 60).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Feverish Rasp", "Bass").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, 0, 13).subOsc (25)
            .filter (1, ladder24, 260, 35, 40).env (2, 0, 240, 0, 80, 88).mod (env2, "filter1Cutoff", 24).mod (velocity, "filter1Cutoff", 5)
            .amp (0, 300, 0, 70).mono (0, false).drive (fuzz, 20, 5700).level (-18).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Sub").mod (macro3, "subLevel", 40).macro (4, "Bite")
            .mod (macro4, "filter1Drive", 50);
        add ("Flat Armature", "Bass").osc (1, pulse, 35).subOsc (45, 0, 2).filter (1, svfLow, 1400, 30, 10).env (2, 0, 120, 0, 120, 65)
            .mod (env2, "filter1Cutoff", 30).lfo (1, sine, 1.4f, 0).mod (lfo1, "osc1Position", 25).amp (2, 320, 0, 70).mono().level (-5)
            .macro (1, "Width").mod (macro1, "osc1Position", 35).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 30).drive (tube, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Flat Burr", "Bass").osc (1, analog, sinePos, 1).pitch (1, 0).osc (2, analog, 10).warp (2, wFm, 20).subOsc (35)
            .env (2, 0, 220, 25, 160, 70).mod (env2, "osc2WarpAmount", 30).mod (velocity, "osc2WarpAmount", 14).filter (1, svfLow, 1200, 5)
            .amp (1, 560, 0, 70).mono().drive (soft, 30, 5400).level (-10).macro (1, "Index").mod (macro1, "osc2WarpAmount", 45).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Sub").mod (macro3, "subLevel", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Flat Relay", "Bass").osc (1, pulse, 55).subOsc (30).filter (1, svfLow, 760, 20, 30).env (2, 0, 260, 0, 140, 85)
            .mod (env2, "filter1Cutoff", 35).lfo (1, sine, 0.45f, 0).mod (lfo1, "osc1Position", 20).amp (0, 180, 0, 90).mono().level (-4)
            .macro (1, "Width").mod (macro1, "osc1Position", 35).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 30).macro (3, "Sub")
            .mod (macro3, "subLevel", 40).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Flayed Drill", "Bass").osc (1, analog, sinePos, 1).pitch (1, 19).osc (2, analog, 10).warp (2, wFm, 25).subOsc (50)
            .env (2, 0, 140, 25, 80, 85).mod (env2, "osc2WarpAmount", 30).mod (velocity, "osc2WarpAmount", 10).filter (1, ladder24, 2100, 20)
            .amp (1, 400, 50, 100).mono (50).drive (hard, 25, 4500).level (-12).macro (1, "Index").mod (macro1, "osc2WarpAmount", 45)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).reverb (0, 45, 1, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 30)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Flayed Gorge", "Bass").osc (1, vowel, 25).subOsc (50, 0, 1).env (2, 20, 180, 40, 150, 70).mod (env2, "osc1Position", 40)
            .filter (1, ladder12, 880, 20, 10).mod (env2, "filter1Cutoff", 6).amp (2, 350, 0, 100).mono (40).drive (hard, 20, 3100).level (-7)
            .macro (1, "Vowel").mod (macro1, "osc1Position", 50).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 35).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Fogbound Culvert", "Bass").osc (1, sub, 20).subOsc (60).env (2, 0, 110, 0, 60, 75).mod (env2, "osc1Pitch", 10.417f)
            .filter (1, ladder24, 230, 30, 40).mod (env2, "filter1Cutoff", 15).amp (4, 500, 0, 300, 65).reverb (35, 70, 4.5f, 70).level (-11)
            .macro (1, "Tail").mod (macro1, "fxReverbMix", 40).macro (2, "Thump").mod (macro2, "filter1Drive", 60).macro (3, "Cutoff")
            .mod (macro3, "filter1Cutoff", 25).drive (tube, 0, 1500).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Fried Clamp", "Bass").osc (1, analog, sawPos).route (1, 0).osc (2, sync, 50, 75).pitch (2, 12, 7).route (2, 100).subOsc (35)
            .parallel().filter (1, ladder24, 250, 20, 40).filter (2, svfBand, 1100, 35).env (2, 0, 230, 0, 120, 75).mod (env2, "filter1Cutoff", 20)
            .mod (env2, "filter2Cutoff", 20).amp (1, 250, 0, 100).mono (0, false).drive (hard, 15, 5200).level (-9).macro (1, "Weight")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Bite").mod (macro2, "filter2Cutoff", 20).macro (3, "Edge")
            .mod (macro3, "filter2Resonance", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 45);
        add ("Gated Hawser", "Bass").osc (1, string, 55, 90).warp (1, wOff, 45).subOsc (55).filter (1, ladder12, 1300, 15, 30)
            .env (2, 0, 220, 0, 120, 75).mod (env2, "filter1Cutoff", 20).mod (velocity, "osc1Position", 10).amp (1, 1900, 0, 160).mono (0, false)
            .drive (fuzz, 30, 6200).level (-22).macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Sub").mod (macro3, "subLevel", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 40);
        add ("Ghost Sump", "Bass").osc (1, sub, 10).subOsc (60).env (2, 0, 70, 0, 60, 75).mod (env2, "osc1Pitch", 10.417f)
            .filter (1, ladder24, 350, 10, 50).mod (env2, "filter1Cutoff", 25).amp (4, 900, 0, 350, 70).reverb (50, 70, 3.5f, 90).level (-14)
            .macro (1, "Tail").mod (macro1, "fxReverbMix", 40).macro (2, "Thump").mod (macro2, "filter1Drive", 60).macro (3, "Cutoff")
            .mod (macro3, "filter1Cutoff", 25).macro (4, "Resonance").mod (macro4, "filter1Resonance", 40);
        add ("Grave Slash", "Bass").osc (1, analog, sawPos).osc (2, analog, sawPos, 65).pitch (2, 0, 9).unison (1, 2, 12, 30).subOsc (35)
            .filter (1, ladder24, 220, 25, 20).env (2, 0, 160, 10, 140, 82).mod (env2, "filter1Cutoff", 38).mod (velocity, "filter1Cutoff", 5)
            .amp (0, 150, 0, 80).mono().level (-3).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).macro (3, "Bite").mod (macro3, "filter1Drive", 50).macro (4, "Beat").mod (macro4, "osc2Fine", 20);
        add ("Grim Tendon", "Bass").osc (1, string, 75, 90).warp (1, wOff, 70).subOsc (30).filter (1, ladder12, 2800, 5, 20)
            .env (2, 0, 200, 0, 120, 75).mod (env2, "filter1Cutoff", 20).mod (velocity, "osc1Position", 15).amp (1, 1800, 0, 160).mono (0, false)
            .drive (tube, 45, 4200).level (-13).macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 40).reverb (0, 45, 1, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 30);
        add ("Hollowed Punch", "Bass").osc (1, analog, sinePos, 1).pitch (1, 19).osc (2, analog, sinePos).warp (2, wFm, 10).subOsc (45)
            .env (2, 0, 80, 10, 120, 75).mod (env2, "osc2WarpAmount", 25).mod (velocity, "osc2WarpAmount", 16).filter (1, ladder12, 1600, 5)
            .amp (1, 600, 0, 60).mono().drive (soft, 30, 3600).level (-11).macro (1, "Index").mod (macro1, "osc2WarpAmount", 45).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Sub").mod (macro3, "subLevel", 40).macro (4, "Bite").mod (macro4, "filter1Drive", 60);
        add ("Hungry Girder", "Bass").osc (1, metal, 40).subOsc (40, 0, 2).filter (1, ladder12, 1200, 20, 50).env (2, 0, 240, 0, 100, 65)
            .mod (env2, "filter1Cutoff", 20).mod (env2, "osc1Position", 35).amp (1, 200, 0, 100).mono().drive (fuzz, 15, 4100).level (-13)
            .macro (1, "Clang").mod (macro1, "osc1Position", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).reverb (0, 45, 1, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 35);
        add ("Ivory Piston", "Bass").osc (1, sync, 40).subOsc (35, 0, 1).filter (1, ladder24, 1100, 25, 20).env (2, 0, 170, 0, 140, 65)
            .mod (env2, "osc1Position", 45).mod (env2, "filter1Cutoff", 10).amp (1, 420, 80, 110).mono (70).drive (hard, 30, 4800).level (-9)
            .macro (1, "Sync").mod (macro1, "osc1Position", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Bite").mod (macro4, "filter1Drive", 60);
        add ("Jade Tether", "Bass").osc (1, string, 45, 90).warp (1, wOff, 40).unison (1, 2, 6, 20, 8).subOsc (50).filter (1, ladder12, 920, 5, 30)
            .env (2, 0, 200, 0, 120, 75).mod (env2, "filter1Cutoff", 14).mod (velocity, "osc1Position", 25).amp (1, 700, 0, 140).mono (0, false)
            .drive (hard, 45, 3100).level (-14).macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Ring").mod (macro3, "osc1WarpAmount", 30).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Jagged Sinew", "Bass").osc (1, string, 45, 90).warp (1, wOff, 65).unison (1, 2, 9, 30, 4).subOsc (35, 0, 1)
            .filter (1, ladder24, 1500, 5, 40).env (2, 0, 120, 0, 120, 75).mod (env2, "filter1Cutoff", 12).mod (velocity, "osc1Position", 10)
            .amp (1, 1800, 0, 120).mono (0, false).drive (hard, 35, 3700).level (-9).macro (1, "Bright").mod (macro1, "osc1Position", 30)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 40).macro (4, "Ring")
            .mod (macro4, "osc1WarpAmount", 30);
        add ("Knotted Maw", "Bass").osc (1, fm, 65).warp (1, wFold, 35).subOsc (30).filter (1, ladder24, 670, 25, 50).lfoSync (1, sine, quarterT)
            .mod (lfo1, "filter1Cutoff", 20).mod (lfo1, "osc1WarpAmount", 15).amp (2, 300, 90, 60).mono().drive (fuzz, 25, 5600).level (-17)
            .macro (1, "Fold").mod (macro1, "osc1WarpAmount", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Sub")
            .mod (macro3, "subLevel", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 40);
        add ("Lingering Hold", "Bass").osc (1, analog, sinePos).subOsc (50, 0, 1).env (2, 0, 110, 0, 60, 90).mod (env2, "osc1Pitch", 39.583f)
            .filter (1, ladder24, 390, 25, 40).mod (env2, "filter1Cutoff", 15).amp (1, 1100, 0, 350, 55).reverb (40, 95, 6, 90).level (-12)
            .macro (1, "Tail").mod (macro1, "fxReverbMix", 40).macro (2, "Thump").mod (macro2, "filter1Drive", 60).drive (tube, 0, 1500)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Resonance").mod (macro4, "filter1Resonance", 40);
        add ("Livid Flange", "Bass").osc (1, metal, 20).subOsc (40, 0, 1).filter (1, ladderBand, 810, 25, 40).env (2, 0, 200, 0, 140, 85)
            .mod (env2, "filter1Cutoff", 22).mod (env2, "osc1Position", 15).amp (1, 340, 0, 80).mono (0, false).drive (hard, 30, 5700).level (-6)
            .macro (1, "Clang").mod (macro1, "osc1Position", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).reverb (0, 45, 1, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 35);
        add ("Locked Pit", "Bass").osc (1, analog, trianglePos).subOsc (60).env (2, 0, 100, 0, 60, 85).mod (env2, "osc1Pitch", 10.417f)
            .filter (1, ladder24, 320, 35, 40).mod (env2, "filter1Cutoff", 25).amp (2, 1300, 0, 400, 70).reverb (45, 70, 6, 90).level (-13)
            .macro (1, "Tail").mod (macro1, "fxReverbMix", 40).macro (2, "Thump").mod (macro2, "filter1Drive", 60).macro (3, "Sub")
            .mod (macro3, "subLevel", 40).macro (4, "Resonance").mod (macro4, "filter1Resonance", 40);
        add ("Marked Slash", "Bass").osc (1, analog, sawPos).osc (2, analog, squarePos, 65).pitch (2, 0, 12).subOsc (35)
            .filter (1, ladder24, 310, 30, 20, 30).env (2, 0, 130, 0, 140, 82).mod (env2, "filter1Cutoff", 22).mod (velocity, "filter1Cutoff", 5)
            .amp (0, 350, 60, 70).mono (0, false).level (-4).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).macro (3, "Bite").mod (macro3, "filter1Drive", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Midnight Fork", "Bass").osc (1, analog, 80).route (1, 0).osc (2, sync, 40, 60).pitch (2, 0, 6).route (2, 100).subOsc (40).parallel()
            .filter (1, ladder24, 270, 35, 10).filter (2, ladderBand, 2600, 50, 0, 50).env (2, 0, 90, 0, 120, 85).mod (env2, "filter1Cutoff", 25)
            .mod (env2, "filter2Cutoff", 20).amp (1, 450, 60, 100).mono (0, false).drive (hard, 15, 4400).level (-9).macro (1, "Weight")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Bite").mod (macro2, "filter2Cutoff", 20).macro (3, "Edge")
            .mod (macro3, "filter2Resonance", 30).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Outlaw Flue", "Bass").osc (1, analog, squarePos).subOsc (55).filter (1, ladder24, 430, 15, 20).filter (2, comb, 392, 50, 0, 100)
            .env (2, 0, 120, 0, 120, 75).mod (env2, "filter1Cutoff", 35).lfo (1, sine, 0.15f, 0).mod (lfo1, "filter2Cutoff", 3).amp (1, 400, 0, 90)
            .mono().level (-6).macro (1, "Pipe").mod (macro1, "filter2Resonance", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28)
            .macro (3, "Resonance").mod (macro3, "filter1Resonance", 35).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Overtime Stamper", "Bass").osc (1, sync, 40).subOsc (55, 0, 1).filter (1, ladder24, 780, 10, 30).env (2, 0, 190, 20, 100, 80)
            .mod (env2, "osc1Position", 35).mod (env2, "filter1Cutoff", 16).amp (1, 320, 60, 70).mono (70, false).drive (hard, 35, 11000)
            .level (-13).macro (1, "Sync").mod (macro1, "osc1Position", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Packed Knock", "Bass").osc (1, fm, 45).subOsc (20).env (2, 0, 60, 0, 60, 92).mod (env2, "osc1Pitch", 14.583f)
            .env (3, 0, 80, 0, 100, 70).mod (env3, "osc1Position", 35).filter (1, ladder24, 2000, 40).amp (0, 240, 0, 90, 65).mono (0, false)
            .drive (hard, 35, 7300).level (-13).macro (1, "Knock").mod (macro1, "osc1Position", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).reverb (0, 40, 0.8f, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 30).macro (4, "Sub")
            .mod (macro4, "subLevel", 40);
        add ("Parched Sinew", "Bass").osc (1, string, 35, 90).warp (1, wOff, 45).subOsc (55, 0, 1).filter (1, ladder24, 2500, 10, 20)
            .env (2, 0, 220, 0, 120, 75).mod (env2, "filter1Cutoff", 10).mod (velocity, "osc1Position", 15).amp (1, 900, 0, 200).mono (0, false)
            .drive (tube, 55, 5100).level (-16).macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).reverb (0, 45, 1, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 30).macro (4, "Sub")
            .mod (macro4, "subLevel", 40);
        add ("Parsed Byte", "Bass").osc (1, analog, trianglePos).warp (1, wCrush, 55).subOsc (35, 0, 2).filter (1, ladder24, 1300)
            .env (2, 0, 120, 0, 120, 75).mod (env2, "filter1Cutoff", 30).amp (1, 420, 60, 100).mono().drive (crush, 15, 6500).level (-4)
            .macro (1, "Steps").mod (macro1, "osc1WarpAmount", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 45).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Pitted Maw", "Bass").osc (1, sub, 25).warp (1, wFold, 25).subOsc (35).filter (1, ladder24, 330, 50, 20).lfoSync (1, triangle, half)
            .mod (lfo1, "filter1Cutoff", 14).mod (lfo1, "osc1WarpAmount", 15).amp (2, 300, 90, 80).mono (30).drive (fuzz, 25, 5300).level (-19)
            .macro (1, "Fold").mod (macro1, "osc1WarpAmount", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Growl")
            .mod (macro3, "osc1Position", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 40);
        add ("Pitted Stamper", "Bass").osc (1, sync, 40).subOsc (30).filter (1, ladder12, 1000, 30, 40).env (2, 0, 320, 0, 100, 65)
            .mod (env2, "osc1Position", 30).mod (env2, "filter1Cutoff", 24).amp (1, 240, 0, 90).mono (70).level (-4).macro (1, "Sync")
            .mod (macro1, "osc1Position", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).drive (hard, 0, 6000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Plain Duct", "Bass").osc (1, analog, squarePos).subOsc (45).filter (1, ladder24, 490, 25, 10).filter (2, comb, 196, 55, 0, 100)
            .env (2, 0, 280, 0, 120, 75).mod (env2, "filter1Cutoff", 35).lfo (1, sine, 0.36f, 0).mod (lfo1, "filter2Cutoff", 5).amp (1, 500, 80, 70)
            .mono().level (-6).macro (1, "Pipe").mod (macro1, "filter2Resonance", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28)
            .macro (3, "Sub").mod (macro3, "subLevel", 40).macro (4, "Resonance").mod (macro4, "filter1Resonance", 35);
        add ("Plain Rip", "Bass").osc (1, analog, sawPos).osc (2, analog, squarePos, 60).pitch (2, 12, 12).unison (1, 2, 8, 40).subOsc (50)
            .filter (1, ladder24, 320, 40, 60).env (2, 0, 200, 0, 100, 76).mod (env2, "filter1Cutoff", 30).mod (velocity, "filter1Cutoff", 15)
            .amp (1, 350, 80, 50).mono (0, false).drive (hard, 35, 4100).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Beat").mod (macro3, "osc2Fine", 20).macro (4, "Sub")
            .mod (macro4, "subLevel", 40);
        add ("Rabid Bump", "Bass").osc (1, fm, 25).subOsc (40).env (2, 0, 50, 0, 60, 88).mod (env2, "osc1Pitch", 25).env (3, 0, 140, 0, 100, 70)
            .mod (env3, "osc1Position", 15).filter (1, ladderBand, 2400, 30, 20).amp (0, 360, 0, 100, 70).mono (0, false).drive (tube, 45, 5400)
            .level (-8).macro (1, "Knock").mod (macro1, "osc1Position", 35).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25)
            .reverb (0, 40, 0.8f, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 30).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Rabid Sleeper", "Bass").osc (1, metal, 10).subOsc (50, 0, 2).osc (2, analog, sawPos, 50).pitch (2, 0, -4)
            .filter (1, ladderBand, 1500, 35, 30).env (2, 0, 300, 0, 120, 65).mod (env2, "filter1Cutoff", 16).mod (env2, "osc1Position", 25)
            .amp (1, 300, 80, 110).mono (0, false).drive (fuzz, 45, 6700).level (-24).macro (1, "Clang").mod (macro1, "osc1Position", 40)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Resonance").mod (macro3, "filter1Resonance", 35).reverb (0, 45, 1, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Reckless Hack", "Bass").osc (1, analog, sawPos).osc (2, analog, squarePos).pitch (2, 12, 6).subOsc (50)
            .filter (1, ladder12, 340, 30, 60, 30).env (2, 0, 160, 10, 120, 76).mod (env2, "filter1Cutoff", 42).mod (velocity, "filter1Cutoff", 15)
            .amp (1, 300, 60, 50).mono (0, false).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).drive (hard, 0, 6000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Sub")
            .mod (macro4, "subLevel", 40);
        add ("Rooftop Cord", "Bass").osc (1, string, 65, 90).warp (1, wOff, 60).subOsc (30).filter (1, ladder12, 3700, 15)
            .env (2, 0, 240, 0, 120, 75).mod (env2, "filter1Cutoff", 14).mod (velocity, "osc1Position", 10).amp (1, 800, 40, 100).mono (0, false)
            .drive (hard, 30, 6700).level (-12).macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Ring").mod (macro3, "osc1WarpAmount", 30).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 40);
        add ("Sampled Bit", "Bass").osc (1, analog, 15).warp (1, wCrush, 70).subOsc (40).filter (1, ladder24, 2300, 35).env (2, 0, 150, 0, 120, 75)
            .mod (env2, "filter1Cutoff", 30).amp (1, 220, 60, 70).mono (0, false).drive (crush, 35).level (-3).macro (1, "Steps")
            .mod (macro1, "osc1WarpAmount", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Sub").mod (macro3, "subLevel", 40)
            .delay (sixteenthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Sampled Byte", "Bass").osc (1, analog, trianglePos).warp (1, wCrush, 50).subOsc (50).filter (1, ladder24, 910, 30)
            .env (2, 0, 180, 0, 120, 75).mod (env2, "filter1Cutoff", 20).amp (1, 220, 60, 50).mono (0, false).level (-4).macro (1, "Steps")
            .mod (macro1, "osc1WarpAmount", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).delay (sixteenthD, 40, 0, 4500)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (crush, 0, 8000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 45);
        add ("Savage Vent", "Bass").osc (1, analog, sawPos).subOsc (50).filter (1, ladder24, 1100, 15, 30).filter (2, comb, 261.6f, 70, 0, 100)
            .env (2, 0, 120, 0, 120, 75).mod (env2, "filter1Cutoff", 25).amp (1, 380, 0, 80).mono().drive (tube, 40, 4000).level (-15)
            .macro (1, "Pipe").mod (macro1, "filter2Resonance", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 35).macro (4, "Tune").mod (macro4, "filter2Cutoff", 12);
        add ("Scarred Token", "Bass").osc (1, analog, trianglePos).warp (1, wCrush, 65).subOsc (40, 0, 2).filter (1, ladder12, 550, 35)
            .env (2, 0, 160, 0, 120, 75).mod (env2, "filter1Cutoff", 10).amp (1, 260, 0, 100).mono (0, false).drive (crush, 30, 8700).level (-3)
            .macro (1, "Steps").mod (macro1, "osc1WarpAmount", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Sub")
            .mod (macro3, "subLevel", 40).macro (4, "Resonance").mod (macro4, "filter1Resonance", 35);
        add ("Secret Drill", "Bass").osc (1, analog, sinePos, 1).pitch (1, 12).osc (2, analog, trianglePos).warp (2, wFm, 15).subOsc (45)
            .env (2, 0, 360, 0, 140, 80).mod (env2, "osc2WarpAmount", 50).mod (velocity, "osc2WarpAmount", 18).filter (1, ladder12, 1000, 5)
            .amp (1, 280, 0, 120).mono().drive (hard, 35, 4500).level (-12).macro (1, "Index").mod (macro1, "osc2WarpAmount", 45)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).reverb (0, 45, 1, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 30)
            .macro (4, "Bite").mod (macro4, "filter1Drive", 60);
        add ("Shapeless Trench", "Bass").osc (1, analog, 20).subOsc (60, 0, 1).env (2, 0, 110, 0, 60, 75).mod (env2, "osc1Pitch", 39.583f)
            .filter (1, ladder24, 230, 25, 50).mod (env2, "filter1Cutoff", 15).amp (4, 600, 0, 200, 70).reverb (35, 75, 5.5f, 75)
            .drive (hard, 40, 1000).level (-22).macro (1, "Tail").mod (macro1, "fxReverbMix", 40).macro (2, "Thump")
            .mod (macro2, "filter1Drive", 60).macro (3, "Resonance").mod (macro3, "filter1Resonance", 40).macro (4, "Sub")
            .mod (macro4, "subLevel", 40);
        add ("Short Gorge", "Bass").osc (1, vowel, 15).subOsc (40).env (2, 20, 360, 0, 150, 70).mod (env2, "osc1Position", -20)
            .filter (1, ladder12, 2500, 20, 40).mod (env2, "filter1Cutoff", 6).amp (2, 600, 85, 120).mono (40).drive (tube, 20, 3500).level (-9)
            .macro (1, "Vowel").mod (macro1, "osc1Position", 50).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Bite")
            .mod (macro3, "filter1Drive", 60).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Shuttered Vault", "Bass").osc (1, sub, 10).subOsc (50, 0, 1).env (2, 0, 80, 0, 60, 75).mod (env2, "osc1Pitch", 14.583f)
            .filter (1, ladder24, 190, 20, 20).mod (env2, "filter1Cutoff", 20).amp (2, 700, 0, 200, 75).reverb (30, 90, 2.5f, 65)
            .drive (soft, 35, 1600).level (-20).macro (1, "Tail").mod (macro1, "fxReverbMix", 40).macro (2, "Thump")
            .mod (macro2, "filter1Drive", 60).macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 25).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Skewed Couple", "Bass").osc (1, analog, sawPos).route (1, 0).osc (2, analog, 70, 75).pitch (2, 7, 9).route (2, 100).subOsc (50)
            .parallel().filter (1, ladder24, 280, 30, 30).filter (2, ladderBand, 1800, 50).env (2, 0, 140, 0, 120, 70)
            .mod (env2, "filter1Cutoff", 25).mod (env2, "filter2Cutoff", 10).amp (1, 350, 60, 90).mono().drive (tube, 15, 9000).level (-9)
            .macro (1, "Weight").mod (macro1, "filter1Cutoff", 28).macro (2, "Bite").mod (macro2, "filter2Cutoff", 20).macro (3, "Sub")
            .mod (macro3, "subLevel", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 45);
        add ("Solid Bracket", "Bass").osc (1, metal, 10).subOsc (55).filter (1, ladder24, 1100, 30, 20).env (2, 0, 230, 0, 120, 75)
            .mod (env2, "filter1Cutoff", 22).mod (env2, "osc1Position", 30).amp (1, 480, 60, 60).mono().drive (fuzz, 15, 2900).level (-14)
            .macro (1, "Clang").mod (macro1, "osc1Position", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Sub")
            .mod (macro3, "subLevel", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 45);
        add ("Solid Jaw", "Bass").osc (1, fm, 55).warp (1, wFold, 30).subOsc (40).filter (1, ladder24, 350, 35, 60).lfoSync (1, triangle, quarterT)
            .mod (lfo1, "filter1Cutoff", 22).mod (lfo1, "osc1WarpAmount", 20).amp (2, 300, 85, 120).mono (60).drive (fuzz, 45, 3500).level (-21)
            .macro (1, "Fold").mod (macro1, "osc1WarpAmount", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Sub")
            .mod (macro3, "subLevel", 40).macro (4, "Growl").mod (macro4, "osc1Position", 35);
        add ("Sooty Relay", "Bass").osc (1, pulse, 20).subOsc (45, 0, 2).filter (1, ladder24, 480, 45).env (2, 0, 120, 0, 140, 75)
            .mod (env2, "filter1Cutoff", 30).lfo (1, triangle, 0.17f, 0).mod (lfo1, "osc1Position", 20).amp (2, 340, 0, 80).mono()
            .drive (soft, 25, 4400).level (-11).macro (1, "Width").mod (macro1, "osc1Position", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 30).macro (3, "Sub").mod (macro3, "subLevel", 40).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Spartan Winch", "Bass").osc (1, sync, 30).subOsc (30, 0, 1).filter (1, ladder12, 2200, 20, 30).env (2, 0, 250, 20, 100, 70)
            .mod (env2, "osc1Position", 35).mod (env2, "filter1Cutoff", 20).amp (1, 280, 60, 100).mono().level (-2).macro (1, "Sync")
            .mod (macro1, "osc1Position", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Sub").mod (macro3, "subLevel", 40)
            .drive (hard, 0, 6000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Spectral Crypt", "Bass").osc (1, analog, 20).subOsc (40).env (2, 0, 80, 0, 60, 75).mod (env2, "osc1Pitch", 39.583f)
            .filter (1, ladder24, 140, 30, 20).mod (env2, "filter1Cutoff", 20).amp (2, 1300, 0, 200, 70).reverb (45, 85, 3.5f, 80)
            .drive (soft, 40, 2100).level (-18).macro (1, "Tail").mod (macro1, "fxReverbMix", 40).macro (2, "Thump")
            .mod (macro2, "filter1Drive", 60).macro (3, "Resonance").mod (macro3, "filter1Resonance", 40).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Squat Mouth", "Bass").osc (1, vowel, 10).subOsc (50).env (2, 0, 280, 40, 150, 70).mod (env2, "osc1Position", -35)
            .filter (1, ladder24, 2000, 35, 40).mod (env2, "filter1Cutoff", 8).amp (2, 300, 0, 120).mono().drive (hard, 45, 6200).level (-16)
            .macro (1, "Vowel").mod (macro1, "osc1Position", 50).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 45);
        add ("Squat Yoke", "Bass").osc (1, analog, squarePos).route (1, 0).osc (2, sync, 20, 65).pitch (2, 12, 5).route (2, 100).subOsc (50)
            .parallel().filter (1, ladder24, 340, 35, 40).filter (2, ladderBand, 1800, 45).env (2, 0, 170, 0, 120, 70)
            .mod (env2, "filter1Cutoff", 25).mod (env2, "filter2Cutoff", 18).amp (1, 250, 0, 100).mono().drive (tube, 25, 6200).level (-11)
            .macro (1, "Weight").mod (macro1, "filter1Cutoff", 28).macro (2, "Bite").mod (macro2, "filter2Cutoff", 20).macro (3, "Edge")
            .mod (macro3, "filter2Resonance", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 45);
        add ("Stagnant Crusher", "Bass").osc (1, sync, 20).subOsc (45).filter (1, ladder12, 1700, 15, 10).env (2, 0, 300, 10, 140, 80)
            .mod (env2, "osc1Position", 25).mod (env2, "filter1Cutoff", 8).amp (1, 460, 0, 90).mono (70, false).drive (tube, 15, 7200).level (-6)
            .macro (1, "Sync").mod (macro1, "osc1Position", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).reverb (0, 50, 1.2f, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Stark Glyph", "Bass").osc (1, analog, 15).warp (1, wCrush, 75).subOsc (50, 0, 2).filter (1, svfLow, 1500, 40)
            .env (2, 0, 210, 0, 120, 75).mod (env2, "filter1Cutoff", 30).amp (1, 420, 60, 60).mono().level (-7).macro (1, "Steps")
            .mod (macro1, "osc1WarpAmount", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).delay (sixteenthD, 40, 0, 4500)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Stark Sleeper", "Bass").osc (1, metal, 15).subOsc (50, 0, 1).osc (2, analog, sawPos, 50).pitch (2, 0, 6)
            .filter (1, ladderBand, 2100, 35, 20).env (2, 0, 180, 0, 120, 65).mod (env2, "filter1Cutoff", 16).mod (env2, "osc1Position", 20)
            .amp (1, 200, 0, 80).mono().drive (hard, 20, 3600).level (-5).macro (1, "Clang").mod (macro1, "osc1Position", 40).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 28).macro (3, "Grit").mod (macro3, "fxDriveAmount", 45).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Stripped Double", "Bass").osc (1, analog, squarePos).route (1, 0).osc (2, sync, 70, 60).pitch (2, 0, 4).route (2, 100).subOsc (30)
            .parallel().filter (1, ladder24, 280, 20, 40).filter (2, svfBand, 2500, 40, 0, 50).env (2, 0, 230, 0, 120, 85)
            .mod (env2, "filter1Cutoff", 15).mod (env2, "filter2Cutoff", 12).amp (1, 450, 0, 80).mono().drive (tube, 15, 8000).level (-9)
            .macro (1, "Weight").mod (macro1, "filter1Cutoff", 28).macro (2, "Bite").mod (macro2, "filter2Cutoff", 20).macro (3, "Sub")
            .mod (macro3, "subLevel", 40).macro (4, "Edge").mod (macro4, "filter2Resonance", 30);
        add ("Stripped Mutter", "Bass").osc (1, vowel, 70).subOsc (55).env (2, 60, 120, 40, 150, 60).mod (env2, "osc1Position", 30)
            .filter (1, ladder24, 1300, 25, 30).mod (env2, "filter1Cutoff", 10).amp (2, 350, 0, 110).mono (80).drive (hard, 20, 6300).level (-8)
            .macro (1, "Vowel").mod (macro1, "osc1Position", 50).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 35).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Tarred Tear", "Bass").osc (1, analog, sawPos).osc (2, analog, squarePos, 70).pitch (2, -12, 5).unison (1, 2, 12, 40).subOsc (25)
            .filter (1, ladder12, 380, 20, 40, 30).env (2, 0, 130, 0, 80, 88).mod (env2, "filter1Cutoff", 42).mod (velocity, "filter1Cutoff", 5)
            .amp (1, 250, 0, 90).mono (0, false).drive (fuzz, 45, 6700).level (-24).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Bite").mod (macro3, "filter1Drive", 50).macro (4, "Sub")
            .mod (macro4, "subLevel", 40);
        add ("Terse Thud", "Bass").osc (1, fm, 40).subOsc (40).env (2, 0, 50, 0, 60, 92).mod (env2, "osc1Pitch", 25).env (3, 0, 180, 0, 100, 75)
            .mod (env3, "osc1Position", 25).filter (1, ladderBand, 2800, 20, 40).amp (0, 220, 0, 60, 55).mono (0, false).drive (tube, 15, 3100)
            .level (-9).macro (1, "Knock").mod (macro1, "osc1Position", 35).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25)
            .reverb (0, 40, 0.8f, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 45);
        add ("Tight Squelch", "Bass").osc (1, pulse, 65).subOsc (45, 0, 1).filter (1, ladder12, 200, 55, 20, 30).env (2, 0, 120, 0, 140, 70)
            .mod (env2, "filter1Cutoff", 25).mod (velocity, "filter1Cutoff", 14).mod (velocity, "filter1Drive", 40).amp (1, 220, 0, 70).mono (60)
            .drive (hard, 30, 2700).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 22).delay (sixteenthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Sub")
            .mod (macro4, "subLevel", 40);
        add ("Tin Manifold", "Bass").osc (1, analog, squarePos).subOsc (35).filter (1, ladder24, 1100, 25, 30).filter (2, comb, 784, 50, 0, 100)
            .env (2, 0, 160, 0, 120, 75).mod (env2, "filter1Cutoff", 35).amp (1, 480, 0, 80).mono().drive (tube, 15, 3000).level (-5)
            .macro (1, "Pipe").mod (macro1, "filter2Resonance", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Tungsten Conduit", "Bass").osc (1, analog, squarePos).subOsc (50).filter (1, ladder24, 1100, 40, 30)
            .filter (2, comb, 261.6f, 60, 0, 100).env (2, 0, 220, 0, 120, 75).mod (env2, "filter1Cutoff", 35).lfo (1, sine, 0.11f, 0)
            .mod (lfo1, "filter2Cutoff", 4).amp (1, 220, 80, 110).mono (0, false).drive (hard, 25, 7600).level (-15).macro (1, "Pipe")
            .mod (macro1, "filter2Resonance", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Sub")
            .mod (macro3, "subLevel", 40).macro (4, "Tune").mod (macro4, "filter2Cutoff", 12);
        add ("Twisted Auger", "Bass").osc (1, analog, sinePos, 1).pitch (1, 24).osc (2, analog, sinePos).warp (2, wFm, 20).subOsc (30)
            .env (2, 0, 80, 25, 160, 75).mod (env2, "osc2WarpAmount", 55).mod (velocity, "osc2WarpAmount", 20).filter (1, svfLow, 4000, 20)
            .amp (1, 260, 0, 90).mono (50).drive (hard, 25, 4300).level (-11).macro (1, "Index").mod (macro1, "osc2WarpAmount", 45)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).reverb (0, 45, 1, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 30)
            .macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Vicious Crank", "Bass").osc (1, sync, 20).subOsc (35, 0, 1).filter (1, ladder12, 1100, 35, 10).env (2, 0, 170, 10, 160, 75)
            .mod (env2, "osc1Position", 50).mod (env2, "filter1Cutoff", 8).amp (1, 460, 60, 70).mono (0, false).level (-4).macro (1, "Sync")
            .mod (macro1, "osc1Position", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Sub").mod (macro3, "subLevel", 40)
            .drive (hard, 0, 6000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Vicious Split", "Bass").osc (1, analog, 80).route (1, 0).osc (2, pulse, 40, 55).pitch (2, 12, 4).route (2, 100).subOsc (35).parallel()
            .filter (1, ladder24, 470, 25, 40).filter (2, svfBand, 3100, 55).env (2, 0, 260, 0, 120, 70).mod (env2, "filter1Cutoff", 25)
            .mod (env2, "filter2Cutoff", 14).amp (1, 300, 80, 70).mono().drive (hard, 25, 3300).level (-11).macro (1, "Weight")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Bite").mod (macro2, "filter2Cutoff", 20).macro (3, "Edge")
            .mod (macro3, "filter2Resonance", 30).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Violet Grip", "Bass").osc (1, sub, 40).warp (1, wFold, 35).subOsc (30).filter (1, ladder24, 260, 40, 40).lfoSync (1, sine, quarter)
            .mod (lfo1, "filter1Cutoff", 20).mod (lfo1, "osc1WarpAmount", 10).amp (2, 300, 90, 120).mono (60).drive (hard, 45, 4900).level (-15)
            .macro (1, "Fold").mod (macro1, "osc1WarpAmount", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 28).macro (3, "Sub")
            .mod (macro3, "subLevel", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 40);
        add ("Western Drawl", "Bass").osc (1, vowel, 40).subOsc (50, 0, 1).osc (2, analog, sawPos, 55).pitch (2, -12, 9)
            .env (2, 60, 320, 40, 150, 80).mod (env2, "osc1Position", 30).filter (1, ladder24, 1400, 30, 50).mod (env2, "filter1Cutoff", 18)
            .amp (2, 250, 0, 60).mono().drive (hard, 30, 5500).level (-9).macro (1, "Vowel").mod (macro1, "osc1Position", 50).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 45).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 35);
        add ("Western Throat", "Bass").osc (1, vowel, 45).subOsc (50).env (2, 0, 400, 0, 150, 80).mod (env2, "osc1Position", 40)
            .filter (1, ladder24, 1600, 20, 40).mod (env2, "filter1Cutoff", 10).amp (2, 600, 85, 120).mono (40).drive (tube, 30, 3300).level (-9)
            .macro (1, "Vowel").mod (macro1, "osc1Position", 50).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Bite")
            .mod (macro3, "filter1Drive", 60).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Wordless Hawser", "Bass").osc (1, string, 35, 90).warp (1, wOff, 35).subOsc (30).filter (1, ladder24, 1800, 25, 30)
            .env (2, 0, 180, 0, 120, 75).mod (env2, "filter1Cutoff", 14).mod (velocity, "osc1Position", 20).amp (1, 2000, 40, 120).mono (0, false)
            .drive (tube, 50, 4600).level (-12).macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Ring").mod (macro3, "osc1WarpAmount", 30).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Wordless Solenoid", "Bass").osc (1, pulse, 30).subOsc (50, 0, 2).filter (1, ladder24, 890, 30).env (2, 0, 160, 0, 120, 75)
            .mod (env2, "filter1Cutoff", 30).lfo (1, triangle, 0.58f, 0).mod (lfo1, "osc1Position", 25).amp (2, 180, 0, 70).mono().level (-5)
            .macro (1, "Width").mod (macro1, "osc1Position", 35).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 30).drive (tube, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Resonance").mod (macro4, "filter1Resonance", 35);
        add ("Wrecked Fang", "Bass").osc (1, sub, 25).warp (1, wFold, 30).subOsc (45).filter (1, ladder24, 260, 35, 40)
            .lfoSync (1, triangle, eighth).mod (lfo1, "filter1Cutoff", 20).mod (lfo1, "osc1WarpAmount", 15).amp (2, 300, 90, 60).mono (30)
            .drive (fuzz, 45, 5500).level (-25).macro (1, "Fold").mod (macro1, "osc1WarpAmount", 45).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 28).macro (3, "Sub").mod (macro3, "subLevel", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 40);

        // Sub, by name.
        add ("Bare Shiver", "Sub").osc (1, analog, trianglePos).osc (2, analog, sinePos, 75).pitch (2, 0, 14).subOsc (30)
            .filter (1, ladder24, 380, 5, 20).lfo (1, sine, 2.3f, 0).mod (lfo1, "ampLevel", 12).amp (60, 500, 85, 100).mono (60).level (-7)
            .macro (1, "Beat").mod (macro1, "osc2Fine", 15).macro (2, "Rate").mod (macro2, "lfo1Rate", 25).macro (3, "Depth")
            .mod (macro3, "ampLevel", -12).drive (tube, 0, 2000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Blackened Flick", "Sub").osc (1, analog, 10).subOsc (30).noise (1, 1, 20).env (2, 0, 16, 0, 30, 90).mod (env2, "noiseLevel", 45)
            .filter (1, ladder24, 770, 5).amp (1, 750, 0, 100).mono (0, false).level (-2).macro (1, "Click").mod (macro1, "noiseLevel", 30)
            .macro (2, "Warm").mod (macro2, "osc1Position", 25).drive (tube, 0, 2500).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50)
            .macro (4, "Octave").mod (macro4, "subLevel", 30);
        add ("Blackened Keel", "Sub").osc (1, analog, sinePos).subOsc (40, 1, 1).filter (1, ladder24, 800, 5).amp (1, 400, 100, 140).mono()
            .level (-3).macro (1, "Octave").mod (macro1, "subLevel", 30).macro (2, "Warm").mod (macro2, "osc1Position", 25).drive (tube, 0, 2000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Push").mod (macro4, "filter1Drive", 60);
        add ("Blunt Billow", "Sub").osc (1, analog, 15).subOsc (10).env (2, 0, 45, 0, 60, 92).mod (env2, "osc1Pitch", 10.417f)
            .filter (1, ladder24, 930, 5, 10).amp (1, 2300, 0, 300, 60).mono (0, false).drive (soft, 35, 1500).level (-9).macro (1, "Drop")
            .mod (macro1, "osc1Pitch", 14.583f).macro (2, "Grit").mod (macro2, "fxDriveAmount", 50).macro (3, "Warm")
            .mod (macro3, "osc1Position", 25).macro (4, "Push").mod (macro4, "filter1Drive", 60);
        add ("Bottomless Flutter", "Sub").osc (1, analog, trianglePos).osc (2, analog, sinePos, 75).pitch (2, 0, 30).subOsc (50)
            .filter (1, ladder24, 460, 5, 30).lfo (1, sine, 3.6f, 0).mod (lfo1, "ampLevel", 4).amp (60, 500, 100, 350).mono (60).level (-6)
            .macro (1, "Beat").mod (macro1, "osc2Fine", 15).macro (2, "Rate").mod (macro2, "lfo1Rate", 25).macro (3, "Depth")
            .mod (macro3, "ampLevel", -12).drive (tube, 0, 2000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Chemical Roll", "Sub").osc (1, analog, sinePos).subOsc (20).env (2, 0, 55, 0, 60, 84).mod (env2, "osc1Pitch", 10.417f)
            .filter (1, ladder24, 1100, 5).amp (1, 3000, 0, 400, 40).mono (60, false).drive (soft, 25, 3600).level (-9).macro (1, "Drop")
            .mod (macro1, "osc1Pitch", 14.583f).macro (2, "Grit").mod (macro2, "fxDriveAmount", 50).macro (3, "Warm")
            .mod (macro3, "osc1Position", 25).macro (4, "Push").mod (macro4, "filter1Drive", 60);
        add ("Clammy Slide", "Sub").osc (1, sub, 33.33f).subOsc (30).filter (1, ladder24, 470, 25, 30).amp (10, 400, 100, 200).mono (100)
            .glideAlways().drive (soft, 30, 2100).level (-11).macro (1, "Warm").mod (macro1, "osc1Position", 25).macro (2, "Push")
            .mod (macro2, "filter1Drive", 60).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Octave").mod (macro4, "subLevel", 30);
        add ("Dense Surge", "Sub").osc (1, analog, sinePos).subOsc (10).env (2, 0, 65, 0, 60, 88).mod (env2, "osc1Pitch", 25)
            .filter (1, ladder24, 1900, 5).amp (1, 1000, 0, 300, 70).mono (0, false).level (-2).macro (1, "Drop").mod (macro1, "osc1Pitch", 14.583f)
            .drive (tube, 0, 2500).macro (2, "Grit").mod (macro2, "fxDriveAmount", 50).macro (3, "Warm").mod (macro3, "osc1Position", 25)
            .macro (4, "Push").mod (macro4, "filter1Drive", 60);
        add ("Dry Poke", "Sub").osc (1, analog, sinePos).subOsc (30).noise (1, 0, 20).env (2, 0, 16, 0, 30, 90).mod (env2, "noiseLevel", 55)
            .filter (1, ladder24, 760, 5).amp (1, 500, 0, 100).mono().level (-2).macro (1, "Click").mod (macro1, "noiseLevel", 30).macro (2, "Warm")
            .mod (macro2, "osc1Position", 25).drive (tube, 0, 2500).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Octave")
            .mod (macro4, "subLevel", 30);
        add ("Entombed Heave", "Sub").osc (1, analog, 15).subOsc (20).env (2, 0, 70, 0, 60, 92).mod (env2, "osc1Pitch", 25)
            .filter (1, ladder24, 580, 5, 20).amp (1, 1900, 0, 500, 40).mono (0, false).drive (tube, 35, 3700).level (-9).macro (1, "Drop")
            .mod (macro1, "osc1Pitch", 14.583f).macro (2, "Grit").mod (macro2, "fxDriveAmount", 50).macro (3, "Warm")
            .mod (macro3, "osc1Position", 25).macro (4, "Push").mod (macro4, "filter1Drive", 60);
        add ("Gutted Surge", "Sub").osc (1, sub, 15).subOsc (40).env (2, 0, 80, 0, 60, 80).mod (env2, "osc1Pitch", 25)
            .filter (1, ladder24, 720, 5, 30).amp (1, 1000, 0, 600).mono (60, false).drive (tube, 30, 2200).level (-10).macro (1, "Drop")
            .mod (macro1, "osc1Pitch", 14.583f).macro (2, "Grit").mod (macro2, "fxDriveAmount", 50).macro (3, "Warm")
            .mod (macro3, "osc1Position", 25).macro (4, "Push").mod (macro4, "filter1Drive", 60);
        add ("Leaning Billow", "Sub").osc (1, analog, 15).subOsc (10).env (2, 0, 30, 0, 60, 80).mod (env2, "osc1Pitch", 25)
            .filter (1, ladder24, 1300, 5, 20).amp (1, 1200, 0, 200).mono (0, false).drive (soft, 30, 1900).level (-9).macro (1, "Drop")
            .mod (macro1, "osc1Pitch", 14.583f).macro (2, "Grit").mod (macro2, "fxDriveAmount", 50).macro (3, "Warm")
            .mod (macro3, "osc1Position", 25).macro (4, "Push").mod (macro4, "filter1Drive", 60);
        add ("Listing Ballast", "Sub").osc (1, sub, 65).subOsc (50).filter (1, ladder24, 700, 15, 40).amp (2, 600, 70, 160).mono (100)
            .drive (hard, 30, 1300).level (-14).macro (1, "Square").mod (macro1, "osc1Position", 30).macro (2, "Open")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 40).macro (4, "Push")
            .mod (macro4, "filter1Drive", 30);
        add ("Muffled Flutter", "Sub").osc (1, analog, 10).osc (2, analog, sinePos).pitch (2, 0, 20).subOsc (30).filter (1, ladder24, 690, 5)
            .lfo (1, sine, 7.7f, 0).mod (lfo1, "ampLevel", 10).amp (5, 500, 95, 250).mono (60).level (-11).macro (1, "Beat")
            .mod (macro1, "osc2Fine", 15).macro (2, "Rate").mod (macro2, "lfo1Rate", 25).macro (3, "Depth").mod (macro3, "ampLevel", -12)
            .drive (tube, 0, 2000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Nomad Base", "Sub").osc (1, analog, 10).subOsc (70).filter (1, ladder24, 1100, 5).amp (1, 700, 95, 160).mono (90).level (-6)
            .macro (1, "Octave").mod (macro1, "subLevel", 30).macro (2, "Warm").mod (macro2, "osc1Position", 25).drive (tube, 0, 2000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Push").mod (macro4, "filter1Drive", 60);
        add ("Packed Throb", "Sub").osc (1, analog, sinePos).osc (2, analog, sinePos, 75).pitch (2, 0, 16).subOsc (40).filter (1, ladder24, 490, 5)
            .lfo (1, triangle, 3.8f, 0).mod (lfo1, "ampLevel", 12).amp (20, 500, 90, 400).mono().level (-11).macro (1, "Beat")
            .mod (macro1, "osc2Fine", 15).macro (2, "Rate").mod (macro2, "lfo1Rate", 25).macro (3, "Depth").mod (macro3, "ampLevel", -12)
            .drive (tube, 0, 2000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Short Shudder", "Sub").osc (1, analog, 10).osc (2, analog, sinePos, 70).pitch (2, 0, 26).subOsc (50).filter (1, ladder24, 240, 5, 10)
            .lfo (1, sine, 2.6f, 0).mod (lfo1, "ampLevel", 6).amp (60, 500, 85, 250).mono (60).level (-4).macro (1, "Beat")
            .mod (macro1, "osc2Fine", 15).macro (2, "Rate").mod (macro2, "lfo1Rate", 25).macro (3, "Depth").mod (macro3, "ampLevel", -12)
            .drive (tube, 0, 2000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Squat Tremor", "Sub").osc (1, analog, trianglePos).osc (2, analog, sinePos, 60).pitch (2, 0, 8).subOsc (40)
            .filter (1, ladder24, 490, 5).lfo (1, triangle, 5.1f, 0).mod (lfo1, "ampLevel", 6).amp (20, 500, 100, 300).mono (60).level (-7)
            .macro (1, "Beat").mod (macro1, "osc2Fine", 15).macro (2, "Rate").mod (macro2, "lfo1Rate", 25).macro (3, "Depth")
            .mod (macro3, "ampLevel", -12).drive (tube, 0, 2000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Stocky Flutter", "Sub").osc (1, analog, trianglePos).osc (2, analog, sinePos, 75).pitch (2, 0, 26).subOsc (40)
            .filter (1, ladder24, 790, 5).lfo (1, triangle, 1.6f, 0).mod (lfo1, "ampLevel", 12).amp (20, 500, 90, 350).mono (60).level (-9)
            .macro (1, "Beat").mod (macro1, "osc2Fine", 15).macro (2, "Rate").mod (macro2, "lfo1Rate", 25).macro (3, "Depth")
            .mod (macro3, "ampLevel", -12).drive (tube, 0, 2000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Sunless Heave", "Sub").osc (1, sub, 10).subOsc (10).env (2, 0, 40, 0, 60, 92).mod (env2, "osc1Pitch", 10.417f)
            .filter (1, ladder24, 1600, 5, 30).amp (1, 1700, 0, 300).mono (60, false).drive (soft, 40, 3600).level (-11).macro (1, "Drop")
            .mod (macro1, "osc1Pitch", 14.583f).macro (2, "Grit").mod (macro2, "fxDriveAmount", 50).macro (3, "Warm")
            .mod (macro3, "osc1Position", 25).macro (4, "Push").mod (macro4, "filter1Drive", 60);
        add ("Terse Slump", "Sub").osc (1, sub, 10).subOsc (50).filter (1, ladder24, 620, 20, 30).amp (10, 400, 90, 80).mono (180, false)
            .glideAlways().drive (soft, 40, 1800).level (-15).macro (1, "Warm").mod (macro1, "osc1Position", 25).macro (2, "Push")
            .mod (macro2, "filter1Drive", 60).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Octave").mod (macro4, "subLevel", 30);
        add ("Terse Tick", "Sub").osc (1, analog, sinePos).subOsc (50).noise (1, 1).env (2, 0, 40, 0, 30, 90).mod (env2, "noiseLevel", 50)
            .filter (1, ladder24, 2900, 5).amp (1, 250, 0, 140).mono (0, false).level (-3).macro (1, "Click").mod (macro1, "noiseLevel", 30)
            .macro (2, "Warm").mod (macro2, "osc1Position", 25).drive (tube, 0, 2500).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50)
            .macro (4, "Octave").mod (macro4, "subLevel", 30);
        add ("Tight Heave", "Sub").osc (1, sub, 15).subOsc (40).env (2, 0, 50, 0, 60, 88).mod (env2, "osc1Pitch", 10.417f)
            .filter (1, ladder24, 990, 5, 30).amp (1, 2700, 0, 500, 70).mono (0, false).drive (tube, 40, 2900).level (-13).macro (1, "Drop")
            .mod (macro1, "osc1Pitch", 14.583f).macro (2, "Grit").mod (macro2, "fxDriveAmount", 50).macro (3, "Warm")
            .mod (macro3, "osc1Position", 25).macro (4, "Push").mod (macro4, "filter1Drive", 60);
        add ("Tight Mass", "Sub").osc (1, sub, 45).subOsc (20, 0, 2).filter (1, ladder12, 530, 15, 70).amp (1, 400, 80, 60).mono()
            .drive (hard, 45, 2200).level (-14).macro (1, "Square").mod (macro1, "osc1Position", 30).macro (2, "Open")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 40).macro (4, "Push")
            .mod (macro4, "filter1Drive", 30);

        // Reese, by name.
        add ("Atomic Stress", "Reese").osc (1, analog, sinePos).pitch (1, 0, -12).osc (2, analog, trianglePos).pitch (2, 0, 8).warp (2, wFm, 40)
            .unison (2, 2, 10, 40).lfo (1, drift, 0.15f, 0).mod (lfo1, "osc2WarpAmount", 10).filter (1, ladder24, 1700, 40, 10).subOsc (45)
            .amp (5, 400, 90, 140).mono (60).level (-6).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Bend")
            .mod (macro2, "osc2WarpAmount", 40).macro (3, "Sub").mod (macro3, "subLevel", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Brackish Fissure", "Reese").osc (1, analog, sawPos).unison (1, 4, 14, 90).osc (2, analog, squarePos, 65).pitch (2, -12, 10)
            .filter (1, ladder24, 2200, 30, 20).filter (2, svfHigh, 100).lfo (1, sine, 0.071f, 0).mod (lfo1, "filter1Cutoff", 6).subOsc (55)
            .amp (8, 500, 90, 400).mono (60).chorus (30, 0.3f, 40).level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).macro (4, "Sub")
            .mod (macro4, "subLevel", 30);
        add ("Close Hum", "Reese").osc (1, pulse, 40).osc (2, pulse, 75).pitch (2, 0, 12).lfo (1, sine, 0.42f, 0).mod (lfo1, "osc1Position", 24)
            .mod (lfo1, "osc2Position", -12).filter (1, ladderBand, 1300, 30, 20).subOsc (50).amp (5, 400, 90, 180).mono().phaser (35, 0.35f, 50)
            .level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Width").mod (macro2, "osc1Position", 30)
            .macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).macro (4, "Phase").mod (macro4, "fxPhaserMix", 50);
        add ("Close Locust", "Reese").osc (1, growl, 30).unison (1, 2, 10, 50).lfoSync (1, sine, bar1, 0).mod (lfo1, "osc1Position", 25)
            .filter (1, ladder24, 1200, 20, 40).mod (lfo1, "filter1Cutoff", 5).subOsc (45).amp (5, 500, 90, 140).mono (40).drive (fuzz, 45, 3900)
            .level (-25).macro (1, "Bite").mod (macro1, "filter1Drive", 60).macro (2, "Growl").mod (macro2, "osc1Position", 35).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Curt Thrum", "Reese").osc (1, pulse, 50).osc (2, pulse, 45).pitch (2, 0, 12).lfo (1, triangle, 0.74f, 0)
            .mod (lfo1, "osc1Position", 15).mod (lfo1, "osc2Position", -14).filter (1, ladderBand, 740, 25, 20).subOsc (30).amp (5, 400, 90, 120)
            .mono().level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Width").mod (macro2, "osc1Position", 30)
            .drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Damped Chasm", "Reese").osc (1, analog, sawPos).unison (1, 6, 28, 90).osc (2, analog, squarePos, 55).pitch (2, 12, 11)
            .filter (1, ladder24, 1600, 20).filter (2, svfHigh, 170).lfo (1, sine, 0.3f, 0).mod (lfo1, "filter1Cutoff", 11).subOsc (55)
            .amp (8, 500, 90, 350).mono (60).chorus (35, 0.5f, 40).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).macro (4, "Sub")
            .mod (macro4, "subLevel", 30);
        add ("Damped Flume", "Reese").osc (1, analog, sawPos).unison (1, 2, 20, 40).filter (1, comb, 523.3f, 70, 0, 100)
            .filter (2, ladder24, 2500, 15).lfo (1, drift, 0.18f, 0).mod (lfo1, "filter1Cutoff", 5).subOsc (40).amp (5, 400, 90, 200).mono()
            .drive (tube, 30).level (-9).macro (1, "Cutoff").mod (macro1, "filter2Cutoff", 28).macro (2, "Pipe")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Dense Abyss", "Reese").osc (1, analog, sawPos).unison (1, 4, 20, 90).osc (2, analog, squarePos, 65).pitch (2, 0, 4)
            .filter (1, ladder24, 1100, 20, 30).filter (2, svfHigh, 170).lfo (1, sine, 0.1f, 0).mod (lfo1, "filter1Cutoff", 8).subOsc (45)
            .amp (8, 500, 90, 150).mono().chorus (40, 0.2f, 60).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Detune")
            .mod (macro2, "osc1UnisonDetune", 25).macro (3, "Wide").mod (macro3, "fxChorusMix", 40).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Dense Mantis", "Reese").osc (1, growl, 55).unison (1, 2, 18, 60).lfoSync (1, sine, bar1, 0).mod (lfo1, "osc1Position", 30)
            .filter (1, svfLow, 1900, 40, 50).mod (lfo1, "filter1Cutoff", 9).subOsc (45).amp (5, 500, 90, 240).mono (40).drive (tube, 20, 3000)
            .level (-9).macro (1, "Bite").mod (macro1, "filter1Drive", 60).macro (2, "Growl").mod (macro2, "osc1Position", 35).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 40).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Dense Undertow", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos).pitch (2, 0, 16).unison (1, 3, 8, 40)
            .filter (1, ladder24, 1400, 20, 30).filter (2, svfNotch, 300, 65).lfo (1, sine, 0.3f, 0).mod (lfo1, "filter2Cutoff", 14).subOsc (45)
            .amp (5, 400, 90, 240).mono (40).drive (tube, 15, 9700).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Notch").mod (macro2, "filter2Cutoff", 20).macro (3, "Motion").mod (macro3, "lfo1Rate", 30).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Dented Thrum", "Reese").osc (1, pulse, 30).osc (2, pulse, 45).pitch (2, 0, 18).lfo (1, sine, 0.78f, 0).mod (lfo1, "osc1Position", 13)
            .mod (lfo1, "osc2Position", -16).filter (1, svfLow, 450, 45, 10).subOsc (30).amp (5, 400, 90, 160).mono().level (-9).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Width").mod (macro2, "osc1Position", 30).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Formless Vortex", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos).pitch (2, 0, 16).unison (1, 2, 14, 30)
            .filter (1, ladder24, 730, 30, 40).filter (2, svfNotch, 360, 55).lfo (1, triangle, 0.16f, 0).mod (lfo1, "filter2Cutoff", 20).subOsc (45)
            .amp (3, 400, 90, 120).mono (80).drive (hard, 30, 2600).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Notch").mod (macro2, "filter2Cutoff", 20).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Frayed Hollow", "Reese").osc (1, analog, sawPos).unison (1, 5, 28, 100).osc (2, analog, squarePos, 70).pitch (2, -12, 5)
            .filter (1, ladder24, 760, 25, 10).filter (2, svfHigh, 160).lfo (1, sine, 0.11f, 0).mod (lfo1, "filter1Cutoff", 5).subOsc (50)
            .amp (8, 500, 90, 350).mono (120).chorus (40, 0.5f, 30).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Wide").mod (macro4, "fxChorusMix", 40);
        add ("Gnarled Siphon", "Reese").osc (1, analog, sawPos).unison (1, 4, 12, 50).osc (2, analog, sawPos, 60).pitch (2, -12, 5)
            .filter (1, comb, 130.8f, 70).filter (2, ladder24, 1800, 15, 20).lfo (1, triangle, 0.4f, 0).mod (lfo1, "filter1Cutoff", 4).subOsc (35)
            .amp (5, 400, 90, 160).mono (60).drive (hard, 35, 4700).level (-14).macro (1, "Cutoff").mod (macro1, "filter2Cutoff", 28)
            .macro (2, "Pipe").mod (macro2, "filter1Resonance", 25).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Heavy Midge", "Reese").osc (1, growl, 70).unison (1, 2, 14, 40).lfoSync (1, triangle, bar1, 0).mod (lfo1, "osc1Position", 30)
            .filter (1, svfLow, 970, 30, 20).mod (lfo1, "filter1Cutoff", 9).subOsc (35).amp (5, 500, 90, 200).mono (40).drive (hard, 20, 3400)
            .level (-10).macro (1, "Bite").mod (macro1, "filter1Drive", 60).macro (2, "Growl").mod (macro2, "osc1Position", 35).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 40).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Heavy Strain", "Reese").osc (1, analog, sawPos).pitch (1, 0, -6).osc (2, analog, trianglePos).pitch (2, 0, 14).warp (2, wFm, 50)
            .unison (2, 2, 12, 30).lfo (1, sine, 0.24f, 0).mod (lfo1, "osc2WarpAmount", 15).filter (1, ladder24, 1600, 35, 10).subOsc (40)
            .amp (5, 400, 90, 300).mono().level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Bend")
            .mod (macro2, "osc2WarpAmount", 40).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Detune").mod (macro4, "osc2Fine", 20);
        add ("Lean Riptide", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos).pitch (2, 0, 16).unison (1, 2, 8, 40)
            .filter (1, ladder24, 990, 30, 20).filter (2, svfNotch, 370, 50).lfo (1, drift, 0.1f, 0).mod (lfo1, "filter2Cutoff", 18).subOsc (45)
            .amp (10, 400, 90, 220).mono().level (-6).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Notch")
            .mod (macro2, "filter2Cutoff", 20).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub")
            .mod (macro4, "subLevel", 35);
        add ("Midnight Drone", "Reese").osc (1, pulse, 20).osc (2, pulse, 75).pitch (2, -12, 20).lfo (1, sine, 0.61f, 0)
            .mod (lfo1, "osc1Position", 14).mod (lfo1, "osc2Position", -18).filter (1, ladder24, 480, 45, 10).subOsc (50).amp (5, 400, 90, 300)
            .mono().phaser (45, 0.26f, 40).level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Width")
            .mod (macro2, "osc1Position", 30).macro (3, "Phase").mod (macro3, "fxPhaserMix", 50).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Nuclear Flex", "Reese").osc (1, analog, trianglePos).pitch (1, 0, -7).osc (2, analog, trianglePos).pitch (2, 0, 12).warp (2, wFm, 35)
            .unison (2, 2, 14, 50).lfo (1, drift, 0.097f, 0).mod (lfo1, "osc2WarpAmount", 25).filter (1, ladder24, 1500, 15, 10).subOsc (45)
            .amp (5, 400, 90, 280).mono().level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Bend")
            .mod (macro2, "osc2WarpAmount", 40).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Numb Hollow", "Reese").osc (1, analog, sawPos).unison (1, 6, 14, 70).osc (2, analog, squarePos, 50).pitch (2, -12, 4)
            .filter (1, ladder24, 2700, 30, 30).filter (2, svfHigh, 150).lfo (1, sine, 0.072f, 0).mod (lfo1, "filter1Cutoff", 9).subOsc (40)
            .amp (8, 500, 90, 200).mono (120).chorus (20, 0.7f, 30).level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Wide").mod (macro3, "fxChorusMix", 40).macro (4, "Sub")
            .mod (macro4, "subLevel", 30);
        add ("Perimeter Eddy", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos).pitch (2, 0, 16).unison (1, 2, 16, 40)
            .filter (1, ladder24, 1800, 10, 30).filter (2, svfNotch, 1400, 35).lfo (1, drift, 0.34f, 0).mod (lfo1, "filter2Cutoff", 16).subOsc (25)
            .amp (5, 400, 90, 300).mono (40).level (-3).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Notch")
            .mod (macro2, "filter2Cutoff", 20).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub")
            .mod (macro4, "subLevel", 35);
        add ("Plain Scarab", "Reese").osc (1, growl, 45).unison (1, 3, 14, 70).lfoSync (1, triangle, bar1, 0).mod (lfo1, "osc1Position", 35)
            .filter (1, svfLow, 2200, 40, 40).mod (lfo1, "filter1Cutoff", 13).subOsc (30).amp (5, 500, 90, 140).mono().drive (tube, 40, 5300)
            .level (-14).macro (1, "Bite").mod (macro1, "filter1Drive", 60).macro (2, "Growl").mod (macro2, "osc1Position", 35).macro (3, "Sub")
            .mod (macro3, "subLevel", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 40);
        add ("Plain Weevil", "Reese").osc (1, growl, 75).unison (1, 2, 18, 40).lfoSync (1, sine, bars4, 0).mod (lfo1, "osc1Position", 25)
            .filter (1, svfLow, 1400, 20, 50).mod (lfo1, "filter1Cutoff", 5).subOsc (45).amp (5, 500, 90, 300).mono (90).drive (hard, 25, 4400)
            .level (-12).macro (1, "Bite").mod (macro1, "filter1Drive", 60).macro (2, "Growl").mod (macro2, "osc1Position", 35).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 40).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Ragged Locust", "Reese").osc (1, growl, 40).unison (1, 2, 16, 60).lfoSync (1, triangle, bars2, 0).mod (lfo1, "osc1Position", 25)
            .filter (1, ladder24, 750, 35, 20).mod (lfo1, "filter1Cutoff", 11).subOsc (40).amp (5, 500, 90, 140).mono (40).drive (fuzz, 30, 6200)
            .level (-22).macro (1, "Bite").mod (macro1, "filter1Drive", 60).macro (2, "Growl").mod (macro2, "osc1Position", 35)
            .macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Railside Buzz", "Reese").osc (1, pulse, 20).osc (2, pulse, 70).pitch (2, 0, 8).lfo (1, sine, 0.59f, 0).mod (lfo1, "osc1Position", 13)
            .mod (lfo1, "osc2Position", -14).filter (1, ladder24, 510, 50, 30).subOsc (35).amp (5, 400, 90, 280).mono().level (-6)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Width").mod (macro2, "osc1Position", 30).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Sealed Drain", "Reese").osc (1, analog, sawPos).unison (1, 4, 10, 60).osc (2, analog, 80, 60).pitch (2, -12, 10)
            .filter (1, comb, 392, 60, 0, 100).filter (2, ladder24, 1400, 15).lfo (1, sine, 0.48f, 0).mod (lfo1, "filter1Cutoff", 6).subOsc (40)
            .amp (5, 400, 90, 280).mono (60).drive (tube, 15, 4400).level (-10).macro (1, "Cutoff").mod (macro1, "filter2Cutoff", 28)
            .macro (2, "Pipe").mod (macro2, "filter1Resonance", 25).macro (3, "Motion").mod (macro3, "lfo1Rate", 30).macro (4, "Sub")
            .mod (macro4, "subLevel", 35);
        add ("Short Void", "Reese").osc (1, analog, sawPos).unison (1, 6, 16, 70).osc (2, analog, squarePos, 55).pitch (2, 0, 11)
            .filter (1, ladder24, 1600, 20).filter (2, svfHigh, 140).lfo (1, sine, 0.074f, 0).mod (lfo1, "filter1Cutoff", 11).subOsc (35)
            .amp (8, 500, 90, 150).mono().chorus (30, 0.8f, 60).level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Detune")
            .mod (macro2, "osc1UnisonDetune", 25).macro (3, "Sub").mod (macro3, "subLevel", 30).macro (4, "Wide").mod (macro4, "fxChorusMix", 40);
        add ("Shredded Drone", "Reese").osc (1, pulse, 45).osc (2, pulse, 80).pitch (2, 0, 10).lfo (1, triangle, 0.082f, 0)
            .mod (lfo1, "osc1Position", 14).mod (lfo1, "osc2Position", -14).filter (1, ladderBand, 980, 40, 20).subOsc (35).amp (5, 400, 90, 160)
            .mono (50).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Width").mod (macro2, "osc1Position", 30)
            .drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Solid Maelstrom", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos).pitch (2, 0, 16).unison (1, 3, 8, 40)
            .filter (1, ladder24, 580, 15, 50).filter (2, svfNotch, 300, 35).lfo (1, sine, 0.075f, 0).mod (lfo1, "filter2Cutoff", 14).subOsc (25)
            .amp (10, 400, 90, 240).mono().drive (hard, 20, 11000).level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Notch").mod (macro2, "filter2Cutoff", 20).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Solid Rift", "Reese").osc (1, analog, sawPos).unison (1, 6, 16, 70).osc (2, analog, sawPos, 60).pitch (2, 12, 12)
            .filter (1, ladder24, 840, 15, 20).filter (2, svfHigh, 91).lfo (1, drift, 0.053f, 0).mod (lfo1, "filter1Cutoff", 12).subOsc (45)
            .amp (8, 500, 90, 350).mono().chorus (25, 0.7f, 50).level (-2).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Detune")
            .mod (macro2, "osc1UnisonDetune", 25).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Wide")
            .mod (macro4, "fxChorusMix", 40);
        add ("Sullen Void", "Reese").osc (1, analog, sawPos).unison (1, 5, 26, 90).osc (2, analog, squarePos, 70).pitch (2, 0, 10)
            .filter (1, ladder24, 1100, 35, 10).filter (2, svfHigh, 110).lfo (1, drift, 0.052f, 0).mod (lfo1, "filter1Cutoff", 9).subOsc (35)
            .amp (8, 500, 90, 400).mono().chorus (40, 0.6f, 50).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Detune")
            .mod (macro2, "osc1UnisonDetune", 25).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Sub")
            .mod (macro4, "subLevel", 30);
        add ("Terse Churn", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos).pitch (2, 0, 10).unison (1, 3, 12, 40)
            .filter (1, ladder24, 850, 20, 40).filter (2, svfNotch, 750, 30).lfo (1, drift, 0.3f, 0).mod (lfo1, "filter2Cutoff", 20).subOsc (40)
            .amp (5, 400, 90, 260).mono (40).level (0).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Notch")
            .mod (macro2, "filter2Cutoff", 20).macro (3, "Detune").mod (macro3, "osc2Fine", 20).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Tight Scarab", "Reese").osc (1, growl, 30).unison (1, 3, 16, 50).lfoSync (1, sine, half, 0).mod (lfo1, "osc1Position", 30)
            .filter (1, svfLow, 2300, 45, 30).mod (lfo1, "filter1Cutoff", 6).subOsc (35).amp (5, 500, 90, 300).mono (40).drive (tube, 30, 7700)
            .level (-12).macro (1, "Bite").mod (macro1, "filter1Drive", 60).macro (2, "Growl").mod (macro2, "osc1Position", 35)
            .macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Twisted Spout", "Reese").osc (1, analog, sawPos).unison (1, 4, 12, 70).filter (1, comb, 523.3f, 50, 0, 100)
            .filter (2, ladder24, 790, 20, 10).lfo (1, sine, 0.055f, 0).mod (lfo1, "filter1Cutoff", 3).subOsc (45).amp (5, 400, 90, 240).mono()
            .level (-4).macro (1, "Cutoff").mod (macro1, "filter2Cutoff", 28).macro (2, "Pipe").mod (macro2, "filter1Resonance", 25)
            .drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Zone Purr", "Reese").osc (1, pulse, 40).osc (2, pulse, 70).pitch (2, -12, 8).lfo (1, sine, 0.19f, 0).mod (lfo1, "osc1Position", 14)
            .mod (lfo1, "osc2Position", -13).filter (1, ladder24, 1000, 20, 40).subOsc (45).amp (5, 400, 90, 260).mono().level (-4)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Width").mod (macro2, "osc1Position", 30).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Sub").mod (macro4, "subLevel", 35);

        // Acid, by name.
        add ("Ashen Resin", "Acid").osc (1, analog, 80).subOsc (20).filter (1, ladder12, 240, 84, 0, 20).env (2, 0, 260, 0, 120, 76)
            .mod (env2, "filter1Cutoff", 46).mod (velocity, "filter1Cutoff", 12).mono (70).amp (1, 200, 40, 60).delay (eighth, 50, 18, 3000)
            .level (-6).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32).macro (2, "Resonance").mod (macro2, "filter1Resonance", 22)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (tube, 0, 7000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 60);
        add ("Blistered Shriek", "Acid").osc (1, analog, squarePos).unison (1, 2, 12, 40).filter (1, ladderBand, 330, 70, 30, 40)
            .env (2, 0, 390, 0, 140, 60).mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 10).mono (40).amp (1, 260, 75, 90)
            .drive (fuzz, 35, 6500).delay (eighthD, 40, 20, 3900).level (-22).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 15).macro (3, "Grit").mod (macro3, "fxDriveAmount", 35).reverb (0, 60, 2, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Border Solvent", "Acid").osc (1, analog, 85).filter (1, ladder24, 220, 78, 40, 40).env (2, 0, 390, 0, 140, 70)
            .mod (env2, "filter1Cutoff", 48).mod (velocity, "filter1Cutoff", 10).mod (velocity, "filter1Resonance", 12).mono (50)
            .amp (1, 280, 50, 80).delay (eighth, 45, 18, 5100).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 20).macro (3, "Env Mod").mod (macro3, "filter1Drive", 55)
            .reverb (0, 50, 1.4f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 30);
        add ("Buried Tar", "Acid").osc (1, analog, 90).subOsc (1).filter (1, ladder12, 280, 88, 30, 30).env (2, 0, 100, 0, 120, 70)
            .mod (env2, "filter1Cutoff", 34).mod (velocity, "filter1Cutoff", 14).mono (70).amp (1, 240, 60, 80).delay (eighth, 50, 18, 2800)
            .level (-8).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32).macro (2, "Resonance").mod (macro2, "filter1Resonance", 22)
            .macro (3, "Sub").mod (macro3, "subLevel", 40).drive (tube, 0, 7000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 60);
        add ("Caustic Keen", "Acid").osc (1, analog, squarePos).unison (1, 2, 12, 40).filter (1, ladderBand, 580, 86, 40, 40)
            .env (2, 0, 380, 0, 140, 80).mod (env2, "filter1Cutoff", 28).mod (velocity, "filter1Cutoff", 14).mono (100).amp (1, 260, 75, 50)
            .drive (fuzz, 40, 10000).delay (sixteenthD, 35, 24, 7000).level (-22).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 15).macro (3, "Grit").mod (macro3, "fxDriveAmount", 35).macro (4, "Bite")
            .mod (macro4, "filter1Drive", 50);
        add ("Clipped Etch", "Acid").osc (1, analog, 85).filter (1, ladder24, 190, 70, 40, 40).env (2, 0, 170, 0, 80, 75)
            .mod (env2, "filter1Cutoff", 42).mod (velocity, "filter1Cutoff", 10).mod (velocity, "filter1Resonance", 5).mono (40)
            .amp (1, 300, 80, 40).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 20).reverb (0, 50, 1.4f, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 30).drive (hard, 0, 6000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Cold Fifth", "Acid").osc (1, analog, sawPos).osc (2, analog, sawPos, 70).pitch (2, -12, 2).filter (1, ladder24, 260, 65, 30, 30)
            .env (2, 0, 300, 0, 140, 85).mod (env2, "filter1Cutoff", 30).mod (velocity, "filter1Cutoff", 8).mono (40).amp (1, 180, 50, 60)
            .drive (hard, 45, 6300).level (-17).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 20).macro (3, "Bite").mod (macro3, "filter1Drive", 50).macro (4, "Second")
            .mod (macro4, "osc2Volume", -45);
        add ("Dreamless Needle", "Acid").osc (1, pulse, 70).filter (1, ladder24, 470, 77, 10, 30).env (2, 0, 200, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 31).mod (env2, "osc1Position", 30).mod (velocity, "filter1Cutoff", 16).mono (50).amp (1, 280, 70, 40)
            .drive (hard, 20, 8500).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 20).macro (3, "Bite").mod (macro3, "filter1Drive", 50).macro (4, "Width")
            .mod (macro4, "osc1Position", 25);
        add ("Gnarled Probe", "Acid").osc (1, pulse, 75).filter (1, ladder24, 270, 74, 20, 30).env (2, 0, 300, 0, 120, 85)
            .mod (env2, "filter1Cutoff", 34).mod (env2, "osc1Position", -30).mod (velocity, "filter1Cutoff", 10).mono (90).amp (1, 260, 50, 60)
            .drive (tube, 45, 11000).delay (sixteenthD, 30, 14, 6800).level (-16).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 20).macro (3, "Width").mod (macro3, "osc1Position", 25).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Heartless Squeal", "Acid").osc (1, analog, sawPos).unison (1, 1, 14, 40).filter (1, ladderBand, 450, 70, 10, 40)
            .env (2, 0, 380, 0, 140, 60).mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 10).mono (120).amp (1, 260, 75, 90)
            .drive (tube, 55, 8400).delay (sixteenthD, 35, 14, 6000).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 15).macro (3, "Bite").mod (macro3, "filter1Drive", 50).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Muffled Junction", "Acid").osc (1, analog, squarePos).osc (2, analog, squarePos, 60).pitch (2, 12, 7)
            .filter (1, ladder24, 220, 74, 40, 30).env (2, 0, 340, 0, 140, 75).mod (env2, "filter1Cutoff", 36).mod (velocity, "filter1Cutoff", 8)
            .mono (60).amp (1, 180, 70, 70).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 20).macro (3, "Second").mod (macro3, "osc2Volume", -45).delay (sixteenthD, 40, 0, 4500)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Nameless Squeal", "Acid").osc (1, analog, squarePos).unison (1, 2, 6, 40).filter (1, ladderBand, 670, 70, 20, 40)
            .env (2, 0, 240, 0, 140, 65).mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 16).mono (120).amp (1, 260, 70, 80)
            .drive (hard, 55, 5400).delay (eighthD, 50, 20, 4400).level (-17).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 15).macro (3, "Bite").mod (macro3, "filter1Drive", 50).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Ochre Fifth", "Acid").osc (1, analog, sawPos).osc (2, analog, squarePos, 75).pitch (2, 5, 3).filter (1, ladder24, 540, 68, 20, 30)
            .env (2, 0, 140, 0, 140, 70).mod (env2, "filter1Cutoff", 44).mod (velocity, "filter1Cutoff", 10).mono (40).amp (1, 260, 60, 50)
            .delay (sixteenthD, 35, 16, 3000).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 20).macro (3, "Second").mod (macro3, "osc2Volume", -45).macro (4, "Bite")
            .mod (macro4, "filter1Drive", 50);
        add ("Outlaw Shriek", "Acid").osc (1, analog, squarePos).unison (1, 1, 12, 40).filter (1, ladderBand, 630, 74, 40, 40)
            .env (2, 0, 220, 0, 140, 75).mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 10).mono (80).amp (1, 260, 80, 80)
            .drive (hard, 45, 5700).delay (eighth, 50, 14, 3400).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 15).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 35);
        add ("Pallid Lancet", "Acid").osc (1, pulse, 70).filter (1, ladder24, 220, 77, 40, 30).env (2, 0, 240, 0, 120, 65)
            .mod (env2, "filter1Cutoff", 28).mod (env2, "osc1Position", 15).mod (velocity, "filter1Cutoff", 10).mono (90).amp (1, 220, 55, 50)
            .level (-3).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32).macro (2, "Resonance").mod (macro2, "filter1Resonance", 20)
            .drive (hard, 0, 6000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Bite").mod (macro4, "filter1Drive", 50);
        add ("Railside Barb", "Acid").osc (1, pulse, 65).filter (1, ladder24, 580, 86, 10, 30).env (2, 0, 120, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 28).mod (env2, "osc1Position", 25).mod (velocity, "filter1Cutoff", 12).mono (80).amp (1, 200, 75, 80)
            .delay (eighthD, 35, 20, 4000).level (-3).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 20).macro (3, "Bite").mod (macro3, "filter1Drive", 50).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Savage Keen", "Acid").osc (1, analog, squarePos).unison (1, 2, 12, 40).filter (1, ladderBand, 390, 90, 20, 40)
            .env (2, 0, 140, 0, 140, 70).mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 12).mono (110).amp (1, 260, 85, 80)
            .drive (tube, 55, 10000).delay (eighth, 40, 14, 3700).level (-20).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 15).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).reverb (0, 60, 2, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Sepia Diode", "Acid").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, 7, 7).filter (1, ladder24, 200, 65, 20, 30)
            .env (2, 0, 110, 0, 140, 85).mod (env2, "filter1Cutoff", 38).mod (velocity, "filter1Cutoff", 8).mono (40).amp (1, 220, 60, 50)
            .drive (fuzz, 50, 3100).delay (sixteenthD, 45, 10, 3400).level (-27).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 20).macro (3, "Bite").mod (macro3, "filter1Drive", 50).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Shuttered Fifth", "Acid").osc (1, analog, sawPos).osc (2, analog, squarePos, 70).pitch (2, 5, 7).filter (1, ladder24, 230, 68, 50, 30)
            .env (2, 0, 170, 0, 140, 80).mod (env2, "filter1Cutoff", 44).mod (velocity, "filter1Cutoff", 8).mono (100).amp (1, 220, 70, 50)
            .drive (hard, 40, 9200).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 20).macro (3, "Bite").mod (macro3, "filter1Drive", 50).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Sooty Tandem", "Acid").osc (1, analog, sawPos).osc (2, analog, sawPos, 60).pitch (2, -12, 5).filter (1, ladder24, 600, 71, 20, 30)
            .env (2, 0, 230, 0, 140, 70).mod (env2, "filter1Cutoff", 42).mod (velocity, "filter1Cutoff", 16).mono (100).amp (1, 320, 60, 50)
            .drive (tube, 30, 4400).level (-10).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 20).macro (3, "Bite").mod (macro3, "filter1Drive", 50).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Soulless Probe", "Acid").osc (1, pulse, 75).filter (1, ladder24, 200, 71, 30, 30).env (2, 0, 150, 0, 120, 65)
            .mod (env2, "filter1Cutoff", 28).mod (env2, "osc1Position", -15).mod (velocity, "filter1Cutoff", 16).mono (50).amp (1, 260, 75, 50)
            .drive (hard, 25, 3400).delay (sixteenthD, 35, 22, 3500).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 20).macro (3, "Bite").mod (macro3, "filter1Drive", 50).macro (4, "Width")
            .mod (macro4, "osc1Position", 25);
        add ("Soulless Resin", "Acid").osc (1, analog, squarePos).subOsc (1).filter (1, ladder12, 250, 76, 20, 30).env (2, 0, 140, 0, 120, 88)
            .mod (env2, "filter1Cutoff", 28).mod (velocity, "filter1Cutoff", 8).mono (30).amp (1, 200, 40, 80).delay (eighth, 45, 22, 3900)
            .level (-8).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32).macro (2, "Resonance").mod (macro2, "filter1Resonance", 22)
            .drive (tube, 0, 7000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 60).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Splintered Shriek", "Acid").osc (1, analog, sawPos).unison (1, 1, 12, 40).filter (1, ladderBand, 490, 86, 10, 40)
            .env (2, 0, 130, 0, 140, 70).mod (env2, "filter1Cutoff", 31).mod (velocity, "filter1Cutoff", 14).mono (70).amp (1, 260, 85, 90)
            .drive (hard, 25, 3000).delay (sixteenthD, 40, 14, 3500).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 15).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 35);
        add ("Stocky Tandem", "Acid").osc (1, analog, squarePos).osc (2, analog, squarePos, 75).pitch (2, -12, 2)
            .filter (1, ladder24, 380, 65, 20, 30).env (2, 0, 250, 0, 140, 75).mod (env2, "filter1Cutoff", 28).mod (velocity, "filter1Cutoff", 10)
            .mono (70).amp (1, 320, 50, 60).drive (tube, 20, 9400).level (-10).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 20).macro (3, "Second").mod (macro3, "osc2Volume", -45).macro (4, "Bite")
            .mod (macro4, "filter1Drive", 50);
        add ("Sullen Diode", "Acid").osc (1, analog, sawPos).osc (2, analog, squarePos, 70).pitch (2, 7, 1).filter (1, ladder24, 510, 74, 20, 30)
            .env (2, 0, 110, 0, 140, 85).mod (env2, "filter1Cutoff", 42).mod (velocity, "filter1Cutoff", 14).mono (70).amp (1, 300, 50, 40)
            .drive (hard, 35, 7400).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 20).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Second")
            .mod (macro4, "osc2Volume", -45);
        add ("Tight Pin", "Acid").osc (1, pulse, 20).filter (1, ladder24, 250, 65, 20, 30).env (2, 0, 330, 0, 120, 65)
            .mod (env2, "filter1Cutoff", 37).mod (env2, "osc1Position", -25).mod (velocity, "filter1Cutoff", 8).mono (80).amp (1, 260, 50, 70)
            .drive (hard, 30, 4100).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 20).macro (3, "Bite").mod (macro3, "filter1Drive", 50).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Voiceless Tincture", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 210, 82, 50, 40).env (2, 0, 260, 0, 120, 70)
            .mod (env2, "filter1Cutoff", 38).mod (velocity, "filter1Cutoff", 12).mod (velocity, "filter1Resonance", 6).mono (100)
            .amp (1, 200, 60, 50).drive (hard, 45, 3700).delay (eighth, 35, 22, 4300).level (-16).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 32).macro (2, "Resonance").mod (macro2, "filter1Resonance", 20).reverb (0, 50, 1.4f, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 30).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Wasteland Gel", "Acid").osc (1, analog, sawPos).subOsc (30).filter (1, ladder12, 400, 84, 20, 50).env (2, 0, 210, 0, 120, 88)
            .mod (env2, "filter1Cutoff", 44).mod (velocity, "filter1Cutoff", 18).mono (50).amp (1, 260, 40, 60).delay (eighth, 50, 20, 3800)
            .level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 32).macro (2, "Resonance").mod (macro2, "filter1Resonance", 22)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Sub").mod (macro4, "subLevel", 40);

        // Lead, by name.
        add ("Abrasive Shout", "Lead").osc (1, vowel, 10).unison (1, 1, 10, 60).lfoSync (1, sawUp, bars2).mod (lfo1, "osc1Position", 15)
            .mod (modWheel, "osc1Position", 40).filter (1, svfLow, 6400, 15).mono (70).amp (3, 400, 85, 140).drive (hard, 40, 7300)
            .delay (eighthT, 55, 22, 5200, false).level (-15).macro (1, "Vowel").mod (macro1, "osc1Position", 55).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 40);
        add ("Acrid Call", "Lead").osc (1, vowel, 50).unison (1, 3, 12, 50).lfoSync (1, sawUp, bar1).mod (lfo1, "osc1Position", 15)
            .mod (modWheel, "osc1Position", 40).filter (1, ladder12, 7300, 20).mono (40).amp (15, 400, 85, 260).drive (hard, 45, 8300)
            .delay (quarterT, 30, 18, 6300, false).level (-16).macro (1, "Vowel").mod (macro1, "osc1Position", 55).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 40);
        add ("Arctic Tendril", "Lead").osc (1, analog, 45).osc (2, analog, sinePos, 45).pitch (2, -12).filter (1, ladder24, 3400, 25, 0, 40)
            .mono (240).glideAlways().amp (20, 400, 85, 350).lfo (2, sine, 6.25f).lfoFade (2, 600).mod (lfo2, "osc1Fine", 6)
            .mod (lfo2, "osc2Fine", 6).chorus (20, 0.8f, 40).delay (quarter, 35, 18, 4000, false).reverb (30, 70, 9, 65).level (-4)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Shape").mod (macro2, "osc1Position", -30).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Attic Plasma", "Lead").osc (1, analog, sinePos, 1).pitch (1, 12).osc (2, analog, sinePos).warp (2, wFm, 25).unison (2, 2, 7, 60)
            .env (2, 0, 550, 50, 250, 60).mod (env2, "osc2WarpAmount", 15).mod (modWheel, "osc2WarpAmount", 30).filter (1, svfLow, 8000).mono (30)
            .amp (2, 300, 85, 120).lfo (2, sine, 6.5f).lfoFade (2, 600).mod (lfo2, "osc2Fine", 8).delay (quarter, 45, 26, 6000, false).level (-7)
            .macro (1, "Index").mod (macro1, "osc2WarpAmount", 40).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).reverb (0, 70, 3, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Backroom Thread", "Lead").osc (1, analog, trianglePos).osc (2, analog, trianglePos, 55).pitch (2, -12)
            .filter (1, svfLow, 2300, 20, 0, 40).mono (200).glideAlways().amp (8, 400, 85, 450).lfo (2, sine, 5.5f).lfoFade (2, 400)
            .mod (lfo2, "osc1Fine", 8).mod (lfo2, "osc2Fine", 8).chorus (25, 0.4f, 40).delay (eighth, 35, 14, 3000).reverb (30, 95, 3.5f, 70)
            .level (-6).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Shape").mod (macro2, "osc1Position", -30).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Basement Electrode", "Lead").osc (1, analog, sinePos, 1).pitch (1, 7).osc (2, analog, sinePos).warp (2, wFm, 35).unison (2, 2, 7, 40)
            .env (2, 0, 300, 60, 250, 60).mod (env2, "osc2WarpAmount", 45).mod (modWheel, "osc2WarpAmount", 30).filter (1, svfLow, 11000).mono (40)
            .amp (2, 300, 85, 200).lfo (2, sine, 5.25f).lfoFade (2, 250).mod (lfo2, "osc2Fine", 5).drive (hard, 40, 6700)
            .delay (eighthT, 45, 18, 2100).level (-18).macro (1, "Index").mod (macro1, "osc2WarpAmount", 40).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Chained Jet", "Lead").osc (1, analog, sawPos).warp (1, wSync, 15).env (2, 0, 750, 0, 200, 70).mod (env2, "osc1WarpAmount", 20)
            .filter (1, ladder24, 8800, 20).mono (50).amp (2, 300, 85, 140).lfo (2, sine, 4.5f).lfoFade (2, 600).mod (lfo2, "osc1Fine", 6)
            .drive (tube, 15, 8700).delay (eighthT, 40, 12, 7700).level (-5).macro (1, "Sync").mod (macro1, "osc1WarpAmount", 45)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Checkpoint Surge", "Lead").osc (1, analog, sawPos).unison (1, 4, 26, 70).filter (1, ladder24, 1600, 10, 20, 30)
            .env (2, 0, 400, 10, 200, 60).mod (env2, "filter1Cutoff", 20).amp (3, 400, 80, 320).mono (50).chorus (15, 0.4f, 40)
            .delay (quarter, 30, 16, 5700).reverb (16, 80, 6, 60).level (-2).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 35);
        add ("Dockside Blaze", "Lead").osc (1, analog, sawPos).unison (1, 6, 18, 90).osc (2, analog, sawPos, 50).pitch (2, 7, 6)
            .filter (1, svfLow, 1500, 30, 10, 30).env (2, 0, 450, 10, 200, 60).mod (env2, "filter1Cutoff", 15).amp (8, 400, 80, 280).mono (20)
            .chorus (25, 0.8f, 50).delay (eighth, 50, 12, 1800).reverb (16, 85, 3, 50).level (-3).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).drive (hard, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Doomed Cant", "Lead").osc (1, vowel, 80).unison (1, 3, 14, 60).lfoSync (1, triangle, bars2).mod (lfo1, "osc1Position", 30)
            .mod (modWheel, "osc1Position", 40).filter (1, svfLow, 5900, 25).mono (60).amp (8, 400, 85, 240).drive (hard, 30, 5800)
            .delay (quarter, 40, 28, 5900).level (-14).macro (1, "Vowel").mod (macro1, "osc1Position", 55).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Grit").mod (macro3, "fxDriveAmount", 40).reverb (0, 70, 3, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Drifting Flare", "Lead").osc (1, analog, sawPos).warp (1, wSync, 30).env (2, 30, 400, 40, 200, 70).mod (env2, "osc1WarpAmount", 35)
            .filter (1, ladder24, 2500, 20).mono (70).amp (2, 300, 85, 220).lfo (2, sine, 5.5f).lfoFade (2, 350).mod (lfo2, "osc1Fine", 7)
            .drive (hard, 45, 5300).delay (quarter, 35, 30, 3200, false).level (-14).macro (1, "Sync").mod (macro1, "osc1WarpAmount", 45)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 35);
        add ("Echoing Filament", "Lead").osc (1, analog, sinePos, 1).pitch (1, 0).osc (2, analog, sinePos).warp (2, wFm, 15)
            .env (2, 0, 300, 40, 250, 60).mod (env2, "osc2WarpAmount", 15).mod (modWheel, "osc2WarpAmount", 30).filter (1, ladder12, 5600).mono (40)
            .amp (2, 300, 85, 260).lfo (2, sine, 6).lfoFade (2, 600).mod (lfo2, "osc2Fine", 8).delay (eighthD, 35, 28, 2100).level (-2)
            .macro (1, "Index").mod (macro1, "osc2WarpAmount", 40).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).drive (tube, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Factory Hail", "Lead").osc (1, vowel, 40).unison (1, 1, 10, 50).lfoSync (1, triangle, quarter).mod (lfo1, "osc1Position", 15)
            .mod (modWheel, "osc1Position", 40).filter (1, svfLow, 7800, 25).mono (60).amp (3, 400, 85, 240).drive (hard, 35, 4200)
            .delay (quarter, 40, 28, 3100).level (-14).macro (1, "Vowel").mod (macro1, "osc1Position", 55).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).reverb (0, 70, 3, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Fried Beacon", "Lead").osc (1, analog, sawPos).warp (1, wSync, 15).osc (2, analog, squarePos, 55).pitch (2, -12, 7)
            .env (2, 120, 400, 0, 200, 40).mod (env2, "osc1WarpAmount", 45).filter (1, ladder24, 4100, 25).mono (40).amp (2, 300, 85, 260)
            .lfo (2, sine, 5.75f).lfoFade (2, 400).mod (lfo2, "osc1Fine", 6).delay (quarter, 55, 12, 4700).level (-5).macro (1, "Sync")
            .mod (macro1, "osc1WarpAmount", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).reverb (0, 70, 3, 45).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Fried Dazzle", "Lead").osc (1, analog, sawPos).unison (1, 5, 18, 70).osc (2, analog, sawPos, 60).pitch (2, -12, 9)
            .filter (1, ladder24, 1700, 15, 0, 30).env (2, 0, 200, 20, 200, 80).mod (env2, "filter1Cutoff", 15).amp (1, 400, 70, 180).mono (70)
            .chorus (30, 0.7f, 30).delay (eighthD, 40, 18, 3900, false).level (-4).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Frosted Dazzle", "Lead").osc (1, analog, sawPos).unison (1, 5, 30, 70).osc (2, analog, squarePos, 60).pitch (2, 7, 7)
            .filter (1, svfLow, 4900, 25, 20, 30).env (2, 0, 300, 20, 200, 80).mod (env2, "filter1Cutoff", 25).amp (3, 400, 70, 140)
            .chorus (35, 0.8f, 30).delay (quarterT, 45, 28, 7300).level (-10).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Gnarled Burst", "Lead").osc (1, analog, sawPos).unison (1, 7, 18, 70).filter (1, svfLow, 1700, 10, 10, 30)
            .env (2, 0, 500, 40, 200, 70).mod (env2, "filter1Cutoff", 10).amp (3, 400, 70, 220).chorus (25, 0.5f, 40)
            .delay (eighthT, 30, 12, 3600, false).reverb (19, 85, 4.5f, 60).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Gnarled Roar", "Lead").osc (1, sync, 66.67f).osc (2, analog, sawPos, 65).pitch (2, 19, 4).filter (1, ladderBand, 1500, 60, 30, 50)
            .lfo (1, drift, 2, 0).mod (lfo1, "filter1Cutoff", 3).mono (40).amp (2, 300, 85, 200).lfo (2, sine, 6).lfoFade (2, 700)
            .mod (lfo2, "osc1Fine", 10).mod (lfo2, "osc2Fine", 10).drive (hard, 55, 6800).delay (eighthD, 45, 16, 2700).level (-23)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Scream").mod (macro2, "fxDriveAmount", 30).macro (3, "Fifth")
            .mod (macro3, "osc2Volume", -45).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Grid Ribbon", "Lead").osc (1, analog, 45).osc (2, analog, sinePos, 50).pitch (2, 12).filter (1, ladder24, 2400, 15, 0, 40).mono (220)
            .glideAlways().amp (40, 400, 85, 300).lfo (2, sine, 4.75f).lfoFade (2, 700).mod (lfo2, "osc1Fine", 5).mod (lfo2, "osc2Fine", 5)
            .chorus (30, 0.5f, 40).delay (eighthD, 40, 18, 4000, false).reverb (20, 80, 5, 45).level (-3).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Shape").mod (macro2, "osc1Position", -30).macro (3, "Echo").mod (macro3, "fxDelayMix", 30)
            .drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Kerbside Cant", "Lead").osc (1, vowel, 40).unison (1, 2, 10, 40).lfoSync (1, triangle, bar1).mod (lfo1, "osc1Position", 15)
            .mod (modWheel, "osc1Position", 40).filter (1, svfLow, 3400, 20).mono (50).amp (8, 400, 85, 220).drive (hard, 45, 10000)
            .delay (eighth, 30, 16, 4700).level (-17).macro (1, "Vowel").mod (macro1, "osc1Position", 55).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 40);
        add ("Lean Flare", "Lead").osc (1, analog, sawPos).warp (1, wSync, 20).osc (2, analog, squarePos, 40).pitch (2, -12, 5)
            .env (2, 120, 400, 40, 200, 70).mod (env2, "osc1WarpAmount", 50).filter (1, ladder12, 4400, 15).mono (50).amp (2, 300, 85, 160)
            .lfo (2, sine, 4.75f).lfoFade (2, 300).mod (lfo2, "osc1Fine", 7).drive (tube, 25, 9600).delay (quarterT, 50, 24, 3600).level (-10)
            .macro (1, "Sync").mod (macro1, "osc1WarpAmount", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Resonance").mod (macro4, "filter1Resonance", 35);
        add ("Livid Scream", "Lead").osc (1, analog, sawPos).osc (2, analog, sawPos, 50).pitch (2, 7, 3).filter (1, ladderBand, 3200, 35, 50, 50)
            .lfo (1, randomShape, 4, 0).mod (lfo1, "filter1Cutoff", 9).mono (120).amp (5, 300, 85, 140).lfo (2, sine, 6.5f).lfoFade (2, 550)
            .mod (lfo2, "osc1Fine", 5).mod (lfo2, "osc2Fine", 5).drive (fuzz, 45, 6000).delay (quarter, 55, 24, 5700).level (-22)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Scream").mod (macro2, "fxDriveAmount", 30).macro (3, "Fifth")
            .mod (macro3, "osc2Volume", -45).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Locked Vein", "Lead").osc (1, analog, 45).osc (2, analog, sinePos, 55).pitch (2, 24).filter (1, ladder24, 2400, 5, 0, 40).mono (260)
            .glideAlways().amp (40, 400, 85, 300).lfo (2, sine, 6.5f).lfoFade (2, 500).mod (lfo2, "osc1Fine", 6).mod (lfo2, "osc2Fine", 6)
            .chorus (40, 0.9f, 40).delay (eighthT, 50, 22, 6900, false).reverb (20, 75, 4, 55).level (-4).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Shape").mod (macro2, "osc1Position", -30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Magnesium Filament", "Lead").osc (1, analog, sinePos, 1).pitch (1, 19).osc (2, analog, sinePos).warp (2, wFm, 30)
            .env (2, 0, 350, 40, 250, 60).mod (env2, "osc2WarpAmount", 20).mod (modWheel, "osc2WarpAmount", 30).filter (1, ladder12, 9700).mono (60)
            .amp (2, 300, 85, 120).lfo (2, sine, 6).lfoFade (2, 650).mod (lfo2, "osc2Fine", 8).delay (eighth, 55, 16, 3800).level (-2)
            .macro (1, "Index").mod (macro1, "osc2WarpAmount", 40).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).drive (tube, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).reverb (0, 70, 3, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Needle Pipe", "Lead").osc (1, pulse, 25).osc (2, pulse, 65, 55).pitch (2, -12, 5).lfo (1, sine, 2.1f, 0)
            .mod (lfo1, "osc1Position", 30).filter (1, svfLow, 4900, 30, 10, 30).env (2, 0, 250, 30, 200, 60).mod (env2, "filter1Cutoff", 8)
            .mono (30).amp (5, 300, 85, 200).lfo (2, sine, 4.5f).lfoFade (2, 700).mod (lfo2, "osc1Fine", 5).delay (sixteenthD, 30, 12, 1800)
            .level (-6).macro (1, "Width").mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Needle Torch", "Lead").osc (1, analog, sawPos).warp (1, wSync, 25).osc (2, analog, squarePos, 40).pitch (2, -12, 5)
            .env (2, 0, 250, 40, 200, 70).mod (env2, "osc1WarpAmount", 20).filter (1, ladder24, 6400).mono (50).amp (2, 300, 85, 180)
            .lfo (2, sine, 5.5f).lfoFade (2, 450).mod (lfo2, "osc1Fine", 5).delay (sixteenthD, 35, 12, 3600).level (-2).macro (1, "Sync")
            .mod (macro1, "osc1WarpAmount", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Resonance").mod (macro4, "filter1Resonance", 35);
        add ("Overloaded Cant", "Lead").osc (1, vowel, 10).unison (1, 1, 12, 40).lfoSync (1, sawUp, bar1).mod (lfo1, "osc1Position", 40)
            .mod (modWheel, "osc1Position", 40).filter (1, svfLow, 7800, 25).mono (70).amp (15, 400, 85, 200).drive (hard, 40, 8500)
            .delay (eighthT, 40, 14).level (-15).macro (1, "Vowel").mod (macro1, "osc1Position", 55).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 40);
        add ("Overloaded Flare", "Lead").osc (1, analog, sawPos).warp (1, wSync, 10).env (2, 0, 700, 40, 200, 70).mod (env2, "osc1WarpAmount", 50)
            .filter (1, ladder12, 8100, 20).mono (80).amp (2, 300, 85, 220).lfo (2, sine, 6.25f).lfoFade (2, 500).mod (lfo2, "osc1Fine", 4)
            .drive (fuzz, 40, 7500).delay (quarterT, 55, 18, 6600).level (-23).macro (1, "Sync").mod (macro1, "osc1WarpAmount", 45)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Overnight Strand", "Lead").osc (1, analog, squarePos).osc (2, analog, sinePos, 60).pitch (2, 12).filter (1, svfLow, 3100, 25, 0, 40)
            .mono (120).glideAlways().amp (20, 400, 85, 300).lfo (2, sine, 5.25f).lfoFade (2, 300).mod (lfo2, "osc1Fine", 5)
            .mod (lfo2, "osc2Fine", 5).chorus (35, 0.6f, 40).delay (eighthD, 35, 18, 3200, false).reverb (30, 90, 7.5f, 50).level (-9)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Shape").mod (macro2, "osc1Position", -30).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Overtime Shawm", "Lead").osc (1, pulse, 45).lfo (1, sine, 1, 0).mod (lfo1, "osc1Position", 20).filter (1, ladder24, 1900, 30)
            .env (2, 0, 200, 40, 200, 60).mod (env2, "filter1Cutoff", 6).mono (20).amp (5, 300, 85, 140).lfo (2, sine, 4.75f).lfoFade (2, 600)
            .mod (lfo2, "osc1Fine", 8).delay (sixteenthD, 30, 20, 5300).reverb (17, 70, 7, 40).level (-3).macro (1, "Width")
            .mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Phantom Electrode", "Lead").osc (1, analog, sinePos, 1).pitch (1, 19).osc (2, analog, 15).warp (2, wFm, 35)
            .env (2, 0, 400, 60, 250, 60).mod (env2, "osc2WarpAmount", 30).mod (modWheel, "osc2WarpAmount", 30).filter (1, ladder12, 3900).mono (60)
            .amp (2, 300, 85, 200).lfo (2, sine, 6.5f).lfoFade (2, 600).mod (lfo2, "osc2Fine", 8).delay (eighth, 35, 30, 7000).level (-3)
            .macro (1, "Index").mod (macro1, "osc2WarpAmount", 40).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Polished Element", "Lead").osc (1, analog, sinePos, 1).pitch (1, 7).osc (2, analog, 15).warp (2, wFm, 25).env (2, 0, 650, 60, 250, 60)
            .mod (env2, "osc2WarpAmount", 15).mod (modWheel, "osc2WarpAmount", 30).filter (1, svfLow, 5600).mono (40).amp (2, 300, 85, 180)
            .lfo (2, sine, 5).lfoFade (2, 700).mod (lfo2, "osc2Fine", 6).delay (quarter, 40, 20, 4100).level (-4).macro (1, "Index")
            .mod (macro1, "osc2WarpAmount", 40).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).drive (tube, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Quarantine Surge", "Lead").osc (1, analog, sawPos).unison (1, 7, 20).osc (2, analog, squarePos, 45).pitch (2, -12, 8)
            .filter (1, ladder12, 2100, 20, 20, 30).env (2, 0, 500, 40, 200, 60).mod (env2, "filter1Cutoff", 10).amp (1, 400, 80, 300).mono (80)
            .chorus (30, 0.5f, 40).delay (sixteenthD, 40, 26, 6800, false).reverb (23, 75, 6.5f, 35).level (-4).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 35);
        add ("Ragged Flare", "Lead").osc (1, analog, sawPos).unison (1, 7, 22, 90).osc (2, analog, squarePos, 60).pitch (2, -12, 8)
            .filter (1, ladder24, 2400, 20, 20, 30).env (2, 0, 300, 40, 200, 80).mod (env2, "filter1Cutoff", 15).amp (1, 400, 90, 320).mono (40)
            .chorus (25, 0.3f, 30).delay (eighthT, 30, 26, 4100).level (-3).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .reverb (0, 70, 3, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Raw Roar", "Lead").osc (1, sync, 50).osc (2, analog, sawPos, 55).pitch (2, 7, 8).filter (1, svfBand, 2800, 55, 20, 30)
            .lfo (1, drift, 7.5f, 0).mod (lfo1, "filter1Cutoff", 3).mono (80).amp (2, 300, 85, 220).lfo (2, sine, 5).lfoFade (2, 300)
            .mod (lfo2, "osc1Fine", 6).mod (lfo2, "osc2Fine", 6).drive (fuzz, 55, 6900).delay (quarterT, 50, 24, 2500).level (-26)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Scream").mod (macro2, "fxDriveAmount", 30).macro (3, "Fifth")
            .mod (macro3, "osc2Volume", -45).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Receding Spark", "Lead").osc (1, analog, sawPos).warp (1, wSync, 10).env (2, 0, 700, 20, 200).mod (env2, "osc1WarpAmount", 30)
            .filter (1, ladder24, 6400, 15).mono (70).amp (2, 300, 85, 140).lfo (2, sine, 6).lfoFade (2, 300).mod (lfo2, "osc1Fine", 5)
            .delay (eighth, 55, 28, 2400).level (-2).macro (1, "Sync").mod (macro1, "osc1WarpAmount", 45).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Reckless Fife", "Lead").osc (1, pulse, 40).osc (2, pulse, 50, 55).pitch (2, -12, 10).lfo (1, sine, 0.65f, 0)
            .mod (lfo1, "osc1Position", 30).filter (1, ladder24, 3800, 35, 30).env (2, 0, 500, 50, 200, 60).mod (env2, "filter1Cutoff", 10)
            .mono (90).amp (2, 300, 85, 220).lfo (2, sine, 5.5f).lfoFade (2, 400).mod (lfo2, "osc1Fine", 6).delay (sixteenthD, 40, 26, 1800, false)
            .reverb (14, 100, 8.5f, 70).level (-5).macro (1, "Width").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Roadside Arc", "Lead").osc (1, analog, sawPos).warp (1, wSync, 20).osc (2, analog, squarePos, 50).pitch (2, 0, 9)
            .env (2, 30, 550, 40, 200, 70).mod (env2, "osc1WarpAmount", 20).filter (1, ladder12, 4100, 30).mono (60).amp (2, 300, 85, 200)
            .lfo (2, sine, 4.5f).lfoFade (2, 250).mod (lfo2, "osc1Fine", 7).delay (sixteenthD, 25, 22, 5700).level (-6).macro (1, "Sync")
            .mod (macro1, "osc1WarpAmount", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).reverb (0, 70, 3, 45).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 35);
        add ("Ruined Torch", "Lead").osc (1, analog, sawPos).warp (1, wSync, 35).osc (2, analog, sawPos, 60).pitch (2, 0, 8).env (2, 0, 650, 0, 200)
            .mod (env2, "osc1WarpAmount", 30).filter (1, ladder24, 6400, 25).mono (40).amp (2, 300, 85, 160).lfo (2, sine, 4.5f).lfoFade (2, 300)
            .mod (lfo2, "osc1Fine", 5).delay (eighthD, 40, 24, 5200, false).level (-5).macro (1, "Sync").mod (macro1, "osc1WarpAmount", 45)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Resonance").mod (macro3, "filter1Resonance", 35).reverb (0, 70, 3, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Ruthless Burst", "Lead").osc (1, analog, sawPos).unison (1, 7, 30, 90).osc (2, analog, squarePos, 50).pitch (2, 7, 8)
            .filter (1, ladder24, 3200, 20, 30, 30).env (2, 0, 450, 30, 200, 70).mod (env2, "filter1Cutoff", 25).amp (1, 400, 70, 260)
            .chorus (30, 0.3f, 30).delay (quarter, 35, 28, 2300, false).level (-8).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .reverb (0, 70, 3, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Secret Trace", "Lead").osc (1, analog, squarePos).osc (2, analog, sinePos, 40).pitch (2, -12).filter (1, ladder24, 2200, 25, 0, 40)
            .mono (180).glideAlways().amp (40, 400, 85, 350).lfo (2, sine, 5.75f).lfoFade (2, 550).mod (lfo2, "osc1Fine", 5)
            .mod (lfo2, "osc2Fine", 5).chorus (25, 0.8f, 40).delay (quarter, 45, 22, 2300).reverb (30, 80, 6, 40).level (-4).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Shape").mod (macro2, "osc1Position", -30).drive (tube, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Sector Strobe", "Lead").osc (1, analog, sawPos).unison (1, 5, 30).osc (2, analog, squarePos, 45).pitch (2, -12, 9)
            .filter (1, ladder12, 3200, 20, 20, 30).env (2, 0, 400, 20, 200, 60).mod (env2, "filter1Cutoff", 20).amp (1, 400, 70, 180).mono (80)
            .chorus (30, 0.9f, 40).delay (quarterT, 40, 28, 2100, false).reverb (23, 90, 4.5f, 60).level (-3).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Serrated Wail", "Lead").osc (1, analog, 30).osc (2, analog, sawPos, 55).pitch (2, 7, 7).filter (1, ladder24, 1500, 50, 30, 30)
            .lfo (1, randomShape, 2.1f, 0).mod (lfo1, "filter1Cutoff", 5).mono (80).amp (2, 300, 85, 200).lfo (2, sine, 5.25f).lfoFade (2, 400)
            .mod (lfo2, "osc1Fine", 5).mod (lfo2, "osc2Fine", 5).drive (hard, 45, 5600).delay (eighthD, 55, 26).level (-15).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Scream").mod (macro2, "fxDriveAmount", 30).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Shredded Roar", "Lead").osc (1, growl, 50).osc (2, analog, sawPos, 50).pitch (2, 7, 2).filter (1, ladderBand, 3200, 40, 20, 50)
            .lfo (1, drift, 6.5f, 0).mod (lfo1, "filter1Cutoff", 5).mono (50).amp (2, 300, 85, 140).lfo (2, sine, 5.5f).lfoFade (2, 400)
            .mod (lfo2, "osc1Fine", 8).mod (lfo2, "osc2Fine", 8).drive (hard, 40, 4300).delay (quarterT, 65, 26, 3300).level (-9)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Scream").mod (macro2, "fxDriveAmount", 30).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).reverb (0, 70, 3, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Silent Scream", "Lead").osc (1, analog, 30).osc (2, analog, sawPos, 40).pitch (2, 12, 2).filter (1, ladder24, 3000, 50, 30, 30)
            .lfo (1, randomShape, 7.5f, 0).mod (lfo1, "filter1Cutoff", 6).mono (60).amp (5, 300, 85, 240).lfo (2, sine, 6.5f).lfoFade (2, 650)
            .mod (lfo2, "osc1Fine", 7).mod (lfo2, "osc2Fine", 7).drive (fuzz, 50, 5500).delay (eighthD, 45, 20, 4500).level (-24)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Scream").mod (macro2, "fxDriveAmount", 30).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Fifth").mod (macro4, "osc2Volume", -45);
        add ("Silver Cant", "Lead").osc (1, vowel, 0).unison (1, 1, 8, 50).lfoSync (1, triangle, bar1).mod (lfo1, "osc1Position", 35)
            .mod (modWheel, "osc1Position", 40).filter (1, svfLow, 7000, 25).mono (100).amp (8, 400, 85, 280).drive (hard, 25, 7600)
            .delay (eighth, 25, 24, 2900).level (-11).macro (1, "Vowel").mod (macro1, "osc1Position", 55).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 40);
        add ("Silver Electrode", "Lead").osc (1, analog, sinePos, 1).pitch (1, 24).osc (2, analog, 15).warp (2, wFm, 15)
            .env (2, 0, 600, 40, 250, 60).mod (env2, "osc2WarpAmount", 40).mod (modWheel, "osc2WarpAmount", 30).filter (1, svfLow, 7600).mono (50)
            .amp (2, 300, 85, 160).lfo (2, sine, 5.25f).lfoFade (2, 500).mod (lfo2, "osc2Fine", 6).delay (sixteenthD, 55, 24, 3800).level (-4)
            .macro (1, "Index").mod (macro1, "osc2WarpAmount", 40).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).drive (tube, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Southern Plasma", "Lead").osc (1, analog, sinePos, 1).pitch (1, 12).osc (2, analog, 15).warp (2, wFm, 30).env (2, 0, 400, 20, 250, 60)
            .mod (env2, "osc2WarpAmount", 20).mod (modWheel, "osc2WarpAmount", 30).filter (1, ladder12, 6600).mono (70).amp (2, 300, 85, 220)
            .lfo (2, sine, 6).lfoFade (2, 600).mod (lfo2, "osc2Fine", 4).delay (eighth, 30, 16, 2500).level (-2).macro (1, "Index")
            .mod (macro1, "osc2WarpAmount", 40).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).reverb (0, 70, 3, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Split Bellow", "Lead").osc (1, sync, 30).osc (2, analog, sawPos, 40).pitch (2, 7, 7).filter (1, ladderBand, 1600, 50, 20, 50)
            .lfo (1, randomShape, 3.6f, 0).mod (lfo1, "filter1Cutoff", 7).mono (50).amp (5, 300, 85, 220).lfo (2, sine, 4.5f).lfoFade (2, 300)
            .mod (lfo2, "osc1Fine", 9).mod (lfo2, "osc2Fine", 9).drive (hard, 60, 5300).delay (eighthD, 45, 20, 2600).level (-20)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Scream").mod (macro2, "fxDriveAmount", 30).reverb (0, 70, 3, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Fifth").mod (macro4, "osc2Volume", -45);
        add ("Warped Electrode", "Lead").osc (1, analog, sinePos, 1).pitch (1, 12).osc (2, analog, 15).warp (2, wFm, 25).unison (2, 2, 8, 30)
            .env (2, 0, 350, 60, 250, 60).mod (env2, "osc2WarpAmount", 45).mod (modWheel, "osc2WarpAmount", 30).filter (1, ladder12, 3800).mono (80)
            .amp (2, 300, 85, 220).lfo (2, sine, 5.5f).lfoFade (2, 250).mod (lfo2, "osc2Fine", 6).drive (soft, 30, 10000)
            .delay (eighth, 45, 14, 4000).level (-11).macro (1, "Index").mod (macro1, "osc2WarpAmount", 40).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Waxen Wail", "Lead").osc (1, sync, 30).osc (2, analog, sawPos, 60).pitch (2, 7, 6).filter (1, ladder24, 1700, 45, 20, 50)
            .lfo (1, randomShape, 6.6f, 0).mod (lfo1, "filter1Cutoff", 3).mono (40).amp (2, 300, 85, 260).lfo (2, sine, 5).lfoFade (2, 600)
            .mod (lfo2, "osc1Fine", 8).mod (lfo2, "osc2Fine", 8).drive (fuzz, 60, 5300).delay (quarterT, 60, 24, 3700).level (-29)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Scream").mod (macro2, "fxDriveAmount", 30).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Western Howl", "Lead").osc (1, sync, 50).osc (2, analog, sawPos, 40).pitch (2, 7, 6).filter (1, ladderBand, 2400, 35, 40, 30)
            .lfo (1, drift, 7.2f, 0).mod (lfo1, "filter1Cutoff", 9).mono (50).amp (5, 300, 85, 200).lfo (2, sine, 4.75f).lfoFade (2, 250)
            .mod (lfo2, "osc1Fine", 6).mod (lfo2, "osc2Fine", 6).drive (hard, 55, 5700).delay (quarterT, 60, 20, 3600).level (-19)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Scream").mod (macro2, "fxDriveAmount", 30).reverb (0, 70, 3, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Western Strand", "Lead").osc (1, analog, squarePos).osc (2, analog, trianglePos, 55).pitch (2, -12)
            .filter (1, svfLow, 3900, 15, 0, 40).mono (240).glideAlways().amp (20, 400, 85, 500).lfo (2, sine, 6).lfoFade (2, 500)
            .mod (lfo2, "osc1Fine", 5).mod (lfo2, "osc2Fine", 5).chorus (25, 0.6f, 40).delay (eighth, 40, 16, 6500).reverb (25, 95, 5, 65)
            .level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Shape").mod (macro2, "osc1Position", -30)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Wordless Chanter", "Lead").osc (1, pulse, 20).lfo (1, sine, 0.35f, 0).mod (lfo1, "osc1Position", 25)
            .filter (1, svfLow, 2000, 30, 30, 50).env (2, 0, 350, 50, 200, 60).mod (env2, "filter1Cutoff", 14).mono (90).amp (12, 300, 85, 220)
            .lfo (2, sine, 5.5f).lfoFade (2, 350).mod (lfo2, "osc1Fine", 4).delay (eighthD, 55, 16, 3000, false).reverb (15, 100, 7, 70).level (-5)
            .macro (1, "Width").mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 35);

        // Stab, by name.
        add ("Abrasive Wallop", "Stab").osc (1, analog, sawPos).unison (1, 4, 22).osc (2, analog, sawPos).pitch (2, -12, 9).unison (2, 2, 14)
            .osc (3, additive, 60, 50).pitch (3, 19).noise (30, 1).filter (1, ladder12, 4400, 15).env (2, 0, 280, 0, 200, 70)
            .mod (env2, "filter1Cutoff", 22).mod (env2, "noiseLevel", 25).amp (15, 600, 0, 280).reverb (35, 80, 6.5f, 70).level (-11)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Bows").mod (macro4, "noiseLevel", 30);
        add ("Airless Strike", "Stab").osc (1, analog, sawPos).unison (1, 2, 14, 60).osc (2, analog, sawPos, 65).pitch (2, 12, 9)
            .filter (1, svfLow, 1400, 35, 20).env (2, 0, 110, 0, 120, 70).mod (env2, "filter1Cutoff", 38).mod (velocity, "filter1Cutoff", 6)
            .amp (0, 420, 0, 200).drive (tube, 15, 3600).reverb (10, 35, 1, 60).level (-16).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Blistered Bash", "Stab").osc (1, metal, 90).noise (40).filter (1, comb, 890, 65).filter (2, ladder24, 810, 30)
            .env (2, 0, 180, 0, 120, 85).mod (env2, "filter1Cutoff", -5).mod (env2, "osc1Position", 25).amp (0, 460, 0, 180).drive (fuzz, 15, 9000)
            .reverb (10, 30, 0.8f, 40).level (-11).macro (1, "Ring").mod (macro1, "filter1Resonance", 22).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Bone Bark", "Stab").osc (1, analog, sawPos).unison (1, 4, 8, 70).osc (2, analog, sawPos, 60).pitch (2, -12, 4)
            .filter (1, ladder24, 560, 10, 10, 30).env (2, 40, 200, 0, 150, 75).mod (env2, "filter1Cutoff", 34).mod (velocity, "filter1Cutoff", 12)
            .amp (2, 200, 30, 200).reverb (25, 50, 0.6f, 40).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Blare")
            .mod (macro2, "filter1Resonance", 35).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Burnished Splinter", "Stab").osc (1, analog, sinePos, 1).pitch (1, 22).osc (2, analog, trianglePos).warp (2, wFm, 10)
            .env (2, 0, 130, 0, 120, 80).mod (env2, "osc2WarpAmount", 50).mod (velocity, "osc2WarpAmount", 14).filter (1, svfHigh, 2100)
            .amp (0, 280, 0, 200).reverb (20, 35, 0.6f, 50).level (-9).macro (1, "Index").mod (macro1, "osc2WarpAmount", 40).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Caustic Bite", "Stab").osc (1, analog, sawPos).unison (1, 4, 12, 50).osc (2, analog, sawPos, 75).pitch (2, 7, 6)
            .filter (1, ladder24, 730, 15, 30, 30).env (2, 35, 200, 30, 150, 65).mod (env2, "filter1Cutoff", 37).mod (velocity, "filter1Cutoff", 9)
            .amp (2, 450, 30, 180).reverb (20, 85, 6, 60).level (-10).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Blare")
            .mod (macro2, "filter1Resonance", 35).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Clipped Hup", "Stab").osc (1, vowel, 10).unison (1, 4, 10, 60).env (2, 0, 200, 0, 140, 70).mod (env2, "osc1Position", -30)
            .filter (1, svfBand, 1800, 30).amp (0, 220, 0, 160).drive (hard, 25, 3800).reverb (20, 50, 1.2f, 70).level (-12).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).macro (4, "Grit").mod (macro4, "fxDriveAmount", 40);
        add ("Coastal Pulse", "Stab").osc (1, pulse, 15).unison (1, 2, 16, 50).env (2, 0, 210, 0, 140, 75).mod (env2, "osc1Position", 15)
            .mod (env2, "filter1Cutoff", 21).filter (1, svfBand, 460, 35).amp (0, 400, 0, 240).delay (quarterD, 45, 26, 2500)
            .reverb (15, 85, 7.5f, 30).level (-8).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Width")
            .mod (macro2, "osc1Position", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Cold Call", "Stab").osc (1, vowel, 20).unison (1, 2, 8, 70).env (2, 0, 130, 0, 140, 60).mod (env2, "osc1Position", -30)
            .filter (1, svfBand, 2100, 30).amp (3, 240, 0, 140).drive (hard, 20, 4000).reverb (30, 90, 6.5f, 30).level (-14).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).delay (eighthD, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Cracked Bash", "Stab").osc (1, metal, 45).noise (20, 2).filter (1, comb, 290, 45, 0, 100).filter (2, ladder24, 600, 20)
            .env (2, 0, 80, 0, 120, 85).mod (env2, "filter1Cutoff", -11).mod (env2, "osc1Position", 15).amp (0, 420, 0, 220).reverb (30, 85, 6, 60)
            .level (6).macro (1, "Ring").mod (macro1, "filter1Resonance", 22).macro (2, "Tone").mod (macro2, "filter2Cutoff", 25)
            .drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Clang").mod (macro4, "osc1Position", 30);
        add ("Cracked Wallop", "Stab").osc (1, analog, sawPos).unison (1, 6, 16, 70).osc (2, pulse, 30).pitch (2, 12, 6).unison (2, 2, 14)
            .osc (3, additive, 80, 60).pitch (3, 7).noise (30, 1).filter (1, ladder12, 2000, 5).env (2, 0, 240, 0, 200, 70)
            .mod (env2, "filter1Cutoff", 14).mod (env2, "noiseLevel", 15).amp (2, 480, 0, 200).reverb (40, 90, 5, 70).level (-13)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25)
            .delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Dusky Bay", "Stab").osc (1, analog, sawPos).unison (1, 3, 8, 40).osc (2, analog, sawPos, 70).pitch (2, 0, 5)
            .filter (1, ladder24, 540, 20, 30, 30).env (2, 20, 180, 30, 150, 70).mod (env2, "filter1Cutoff", 25).mod (velocity, "filter1Cutoff", 10)
            .amp (10, 200, 0, 160).reverb (25, 30, 0.8f, 70).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Blare")
            .mod (macro2, "filter1Resonance", 35).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Flayed Swipe", "Stab").osc (1, analog, sawPos).unison (1, 3, 14, 50).osc (2, analog, squarePos, 75).pitch (2, 12, 8)
            .osc (3, analog, 80, 40).pitch (3, -12).filter (1, ladder24, 700, 40, 30).env (2, 0, 130, 0, 120, 70).mod (env2, "filter1Cutoff", 30)
            .mod (velocity, "filter1Cutoff", 12).amp (0, 160, 0, 220).delay (quarter, 40, 16, 7600).reverb (10, 55, 1.4f, 70).level (-10)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Flooded Flash", "Stab").osc (1, pulse, 25).unison (1, 2, 8).env (2, 0, 220, 0, 140, 70).mod (env2, "osc1Position", 15)
            .mod (env2, "filter1Cutoff", 15).filter (1, svfLow, 540, 35).amp (0, 380, 0, 220).delay (eighth, 60, 32, 1700).reverb (25, 75, 9, 30)
            .level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Width").mod (macro2, "osc1Position", 30)
            .macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Flooded Wink", "Stab").osc (1, pulse, 25).unison (1, 2, 8, 60).env (2, 0, 250, 0, 140, 70).mod (env2, "osc1Position", 25)
            .mod (env2, "filter1Cutoff", 27).filter (1, svfBand, 1100, 25).amp (0, 240, 0, 120).delay (quarterT, 45, 36, 2000)
            .reverb (30, 90, 6, 65).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Width")
            .mod (macro2, "osc1Position", 30).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Fluorescent Fragment", "Stab").osc (1, analog, sinePos, 1).pitch (1, 19).osc (2, analog, trianglePos).warp (2, wFm, 10)
            .env (2, 0, 140, 0, 120, 85).mod (env2, "osc2WarpAmount", 40).mod (velocity, "osc2WarpAmount", 8).filter (1, svfHigh, 5000)
            .amp (0, 160, 0, 220).delay (eighthT, 25, 24, 5800, false).reverb (25, 100, 8, 40).level (3).macro (1, "Index")
            .mod (macro1, "osc2WarpAmount", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Fugitive Clout", "Stab").osc (1, analog, sawPos).unison (1, 6, 16, 90).osc (2, pulse, 50).pitch (2, -12, 9).unison (2, 2, 10)
            .osc (3, additive, 40, 55).pitch (3, 24).noise (20, 1).filter (1, ladder12, 2400, 20).env (2, 0, 160, 0, 200, 70)
            .mod (env2, "filter1Cutoff", 12).mod (env2, "noiseLevel", 20).amp (2, 540, 0, 180).reverb (25, 75, 5, 65).level (-13)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Galvanised Smash", "Stab").osc (1, metal, 65).noise (15, 2).filter (1, comb, 680, 50, 0, 100).filter (2, svfHigh, 1500, 30)
            .env (2, 0, 110, 0, 120, 85).mod (env2, "filter1Cutoff", -12).mod (env2, "osc1Position", 25).amp (0, 280, 0, 200)
            .reverb (15, 100, 8.5f, 45).level (-2).macro (1, "Ring").mod (macro1, "filter1Resonance", 22).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Graveyard Chip", "Stab").osc (1, analog, sinePos, 1).pitch (1, 12).osc (2, analog, sinePos).warp (2, wFm, 10)
            .env (2, 0, 170, 0, 120, 80).mod (env2, "osc2WarpAmount", 60).mod (velocity, "osc2WarpAmount", 10).filter (1, svfHigh, 570)
            .amp (0, 180, 0, 200).reverb (30, 100, 8, 55).level (-13).macro (1, "Index").mod (macro1, "osc2WarpAmount", 40).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Hard Yell", "Stab").osc (1, vowel, 10).unison (1, 2, 18).env (2, 0, 280, 0, 140, 70).mod (env2, "osc1Position", 40)
            .filter (1, svfLow, 1800, 20).amp (0, 380, 0, 100).drive (tube, 40, 10000).reverb (25, 100, 5, 30).level (-16).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Jade Swipe", "Stab").osc (1, analog, sawPos).unison (1, 2, 18, 90).osc (2, analog, squarePos, 70).pitch (2, 12, 8)
            .osc (3, pulse, 65, 50).pitch (3, -12).filter (1, ladder24, 1500, 20, 30).env (2, 0, 140, 0, 100, 76).mod (env2, "filter1Cutoff", 30)
            .mod (velocity, "filter1Cutoff", 7).amp (0, 160, 0, 140).drive (hard, 35, 3400).reverb (30, 70, 8.5f, 70).level (-20)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Lean Flake", "Stab").osc (1, analog, sinePos, 1).pitch (1, 19).osc (2, analog, trianglePos).warp (2, wFm, 20).osc (3, fm, 30, 45)
            .pitch (3, 0, 4).env (2, 0, 80, 0, 120, 90).mod (env2, "osc2WarpAmount", 70).mod (velocity, "osc2WarpAmount", 16)
            .filter (1, svfLow, 270).amp (0, 320, 0, 160).delay (sixteenthD, 45, 18, 4800, false).reverb (20, 40, 0.8f, 60).level (1)
            .macro (1, "Index").mod (macro1, "osc2WarpAmount", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Lucid Flake", "Stab").osc (1, analog, sinePos, 1).pitch (1, 12).osc (2, analog, sinePos).warp (2, wFm, 10).osc (3, fm, 20, 55)
            .pitch (3, -12, 4).env (2, 0, 70, 0, 120, 85).mod (env2, "osc2WarpAmount", 70).mod (velocity, "osc2WarpAmount", 14)
            .filter (1, svfHigh, 660).amp (0, 220, 0, 240).delay (eighthD, 40, 26, 7000).reverb (10, 55, 1, 40).level (-12).macro (1, "Index")
            .mod (macro1, "osc2WarpAmount", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Mangled Wham", "Stab").osc (1, metal, 70).noise (40).filter (1, comb, 380, 50, 0, 100).filter (2, svfBand, 1600, 20)
            .env (2, 0, 140, 0, 120, 85).mod (env2, "filter1Cutoff", -5).mod (env2, "osc1Position", 10).amp (0, 460, 0, 120).drive (fuzz, 45, 4000)
            .reverb (20, 85, 5, 60).level (-21).macro (1, "Ring").mod (macro1, "filter1Resonance", 22).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 25).macro (3, "Clang").mod (macro3, "osc1Position", 30).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Marked Cut", "Stab").osc (1, analog, sawPos).unison (1, 2, 14, 90).osc (2, analog, squarePos, 75).pitch (2, -12, 3)
            .filter (1, ladder24, 1400, 15, 10).env (2, 0, 90, 0, 100, 88).mod (env2, "filter1Cutoff", 30).mod (velocity, "filter1Cutoff", 8)
            .amp (0, 180, 0, 100).drive (tube, 15, 6200).delay (eighthT, 40, 12, 5400).reverb (10, 35, 1, 70).level (-13).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Northern Wallop", "Stab").osc (1, analog, sawPos).unison (1, 5, 20, 90).osc (2, analog, sawPos).pitch (2, 12, 7).unison (2, 2, 8)
            .osc (3, additive, 80, 60).pitch (3, 19).noise (30, 1).filter (1, svfLow, 1900, 15).env (2, 0, 220, 0, 200, 70)
            .mod (env2, "filter1Cutoff", 14).mod (env2, "noiseLevel", 25).amp (8, 280, 0, 220).reverb (35, 75, 7.5f, 65).level (-13)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Bows")
            .mod (macro3, "noiseLevel", 30).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Pitted Blink", "Stab").osc (1, pulse, 45).unison (1, 3, 8, 60).env (2, 0, 190, 0, 140, 80).mod (env2, "osc1Position", 35)
            .mod (env2, "filter1Cutoff", 15).filter (1, svfLow, 430, 45).amp (0, 400, 0, 120).delay (eighth, 55, 32, 2500).reverb (15, 80, 5.5f, 60)
            .level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Width").mod (macro2, "osc1Position", 30).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Pitted Slam", "Stab").osc (1, metal, 35).noise (25, 2).osc (2, analog, squarePos, 50).pitch (2, 0).filter (1, comb, 250, 65, 0, 100)
            .filter (2, ladder24, 780, 30).env (2, 0, 200, 0, 120, 85).mod (env2, "filter1Cutoff", 10).mod (env2, "osc1Position", 20)
            .amp (0, 300, 0, 200).drive (hard, 15, 2500).reverb (10, 45, 0.6f, 50).level (-6).macro (1, "Ring").mod (macro1, "filter1Resonance", 22)
            .macro (2, "Tone").mod (macro2, "filter2Cutoff", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Rabid Lunge", "Stab").osc (1, analog, sawPos).unison (1, 3, 18, 60).osc (2, analog, squarePos, 70).pitch (2, -12, 8)
            .osc (3, pulse, 65, 40).pitch (3, 19).filter (1, svfLow, 640, 35, 20).env (2, 0, 100, 0, 80, 70).mod (env2, "filter1Cutoff", 24)
            .mod (velocity, "filter1Cutoff", 6).amp (0, 200, 0, 200).reverb (25, 80, 5, 65).level (-12).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).drive (hard, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Ragged Belt", "Stab").osc (1, analog, sawPos).unison (1, 6, 24, 90).osc (2, pulse, 30).pitch (2, 12, 5).unison (2, 2, 10)
            .osc (3, analog, 80, 55).pitch (3, 24).noise (20, 1).filter (1, ladder12, 4200).env (2, 0, 180, 0, 200, 70)
            .mod (env2, "filter1Cutoff", 18).mod (env2, "noiseLevel", 15).amp (15, 300, 0, 260).reverb (40, 80, 5, 45).level (-14)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Bows").mod (macro4, "noiseLevel", 30);
        add ("Ragged Snarl", "Stab").osc (1, analog, sawPos).unison (1, 2, 10, 70).osc (2, analog, sawPos, 60).pitch (2, -12, 7)
            .filter (1, ladder24, 450, 20, 10, 30).env (2, 15, 300, 0, 150, 70).mod (env2, "filter1Cutoff", 37).mod (velocity, "filter1Cutoff", 10)
            .amp (10, 350, 0, 180).drive (hard, 25, 5700).reverb (10, 35, 1, 50).level (-16).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Blare").mod (macro2, "filter1Resonance", 35).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Rancid Swipe", "Stab").osc (1, analog, sawPos).unison (1, 3, 8, 60).osc (2, analog, squarePos, 75).pitch (2, -12, 8)
            .filter (1, svfLow, 1600, 40).env (2, 0, 160, 0, 140, 82).mod (env2, "filter1Cutoff", 38).mod (velocity, "filter1Cutoff", 8)
            .amp (0, 160, 0, 100).reverb (20, 75, 4.5f, 35).level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Detune")
            .mod (macro2, "osc1UnisonDetune", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Rusted Bash", "Stab").osc (1, metal, 85).noise (15).filter (1, comb, 730, 50, 0, 50).filter (2, svfHigh, 2200, 30)
            .env (2, 0, 80, 0, 120, 85).mod (env2, "filter1Cutoff", -7).mod (env2, "osc1Position", 15).amp (0, 280, 0, 220)
            .reverb (15, 30, 0.8f, 70).level (-1).macro (1, "Ring").mod (macro1, "filter1Resonance", 22).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Rusted Splinter", "Stab").osc (1, analog, sinePos, 1).pitch (1, 24).osc (2, analog, trianglePos).warp (2, wFm, 20).osc (3, fm, 50, 60)
            .pitch (3, -12, 6).env (2, 0, 70, 0, 120, 75).mod (env2, "osc2WarpAmount", 40).mod (velocity, "osc2WarpAmount", 16)
            .filter (1, svfHigh, 2100).amp (0, 480, 0, 140).delay (eighthT, 30, 12, 5200, false).reverb (25, 100, 8.5f, 35).level (-9)
            .macro (1, "Index").mod (macro1, "osc2WarpAmount", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Sacred Wink", "Stab").osc (1, pulse, 45).unison (1, 3, 14, 50).env (2, 0, 130, 0, 140, 75).mod (env2, "osc1Position", 30)
            .mod (env2, "filter1Cutoff", 27).filter (1, ladder24, 980, 45).amp (0, 260, 0, 220).delay (eighthD, 55, 28, 2900)
            .reverb (35, 90, 8.5f, 55).level (-6).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Width")
            .mod (macro2, "osc1Position", 30).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Salted Cut", "Stab").osc (1, analog, sawPos).unison (1, 4, 18, 90).osc (2, analog, sawPos, 75).pitch (2, -12, 9)
            .osc (3, analog, sawPos, 45).pitch (3, 24).filter (1, svfLow, 700, 35, 20).env (2, 0, 210, 0, 80, 88).mod (env2, "filter1Cutoff", 24)
            .mod (velocity, "filter1Cutoff", 8).amp (0, 260, 0, 200).drive (hard, 35, 9600).delay (quarter, 25, 30, 1900).reverb (15, 50, 0.6f, 50)
            .level (-21).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Scalding Sliver", "Stab").osc (1, analog, sinePos, 1).pitch (1, 12).osc (2, analog, sinePos).warp (2, wFm, 15).osc (3, fm, 60, 55)
            .pitch (3, 0, 7).env (2, 0, 200, 0, 120, 90).mod (env2, "osc2WarpAmount", 65).mod (velocity, "osc2WarpAmount", 10)
            .filter (1, svfHigh, 1600).amp (0, 280, 0, 100).reverb (25, 50, 1, 70).level (-9).macro (1, "Index").mod (macro1, "osc2WarpAmount", 40)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Sector Lunge", "Stab").osc (1, analog, sawPos).unison (1, 3, 14, 60).osc (2, analog, squarePos, 70).pitch (2, 12, 8)
            .filter (1, svfLow, 550, 20).env (2, 0, 100, 0, 160, 82).mod (env2, "filter1Cutoff", 34).mod (velocity, "filter1Cutoff", 12)
            .amp (0, 200, 0, 160).drive (hard, 15, 3400).delay (sixteenthD, 40, 14, 2800).reverb (20, 100, 7.5f, 65).level (-18).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Shattered Call", "Stab").osc (1, vowel, 30).unison (1, 3, 16, 90).env (2, 0, 210, 0, 140).mod (env2, "osc1Position", -25)
            .filter (1, svfLow, 2400, 25).amp (8, 320, 0, 200).drive (hard, 25, 4100).reverb (20, 95, 7.5f, 35).level (-15).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 40).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Splintered Yap", "Stab").osc (1, analog, sawPos).unison (1, 3, 8, 40).osc (2, analog, 80, 70).pitch (2, -12, 4)
            .filter (1, ladder24, 640, 30, 20, 30).env (2, 30, 280, 0, 150, 75).mod (env2, "filter1Cutoff", 31).mod (velocity, "filter1Cutoff", 13)
            .amp (2, 200, 30, 100).reverb (15, 95, 6, 30).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Blare")
            .mod (macro2, "filter1Resonance", 35).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Sprawling Clout", "Stab").osc (1, analog, sawPos).unison (1, 4, 26).osc (2, pulse, 66.67f).pitch (2, 12, 9).unison (2, 2, 14)
            .osc (3, analog, 60, 50).pitch (3, 19).noise (20, 1).filter (1, ladder12, 4200, 20).env (2, 0, 160, 0, 200, 70)
            .mod (env2, "filter1Cutoff", 22).mod (env2, "noiseLevel", 20).amp (2, 280, 0, 320).reverb (35, 95, 6.5f, 65).level (-12)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25)
            .delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Submerged Blink", "Stab").osc (1, pulse, 30).unison (1, 3, 16).env (2, 0, 150, 0, 140, 70).mod (env2, "osc1Position", 20)
            .mod (env2, "filter1Cutoff", 15).filter (1, svfLow, 730, 35).amp (0, 400, 0, 160).delay (quarterD, 55, 34, 1600).reverb (30, 90, 4, 60)
            .level (-10).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Width").mod (macro2, "osc1Position", 30)
            .macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Sullen Bark", "Stab").osc (1, analog, sawPos).unison (1, 4, 14, 40).osc (2, analog, sawPos, 60).pitch (2, 7, 10)
            .filter (1, ladder24, 420, 30, 20, 30).env (2, 10, 160, 30, 150, 55).mod (env2, "filter1Cutoff", 28).mod (velocity, "filter1Cutoff", 6)
            .amp (10, 250, 0, 200).reverb (10, 45, 0.8f, 40).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Blare")
            .mod (macro2, "filter1Resonance", 35).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Tarred Splinter", "Stab").osc (1, analog, sinePos, 1).pitch (1, 7).osc (2, analog, trianglePos).warp (2, wFm, 15).osc (3, fm, 20, 55)
            .pitch (3, -12, 8).env (2, 0, 220, 0, 120, 85).mod (env2, "osc2WarpAmount", 30).mod (velocity, "osc2WarpAmount", 16)
            .filter (1, svfLow, 340).amp (0, 260, 0, 160).delay (quarterT, 30, 28, 2200).reverb (15, 70, 4, 70).level (-6).macro (1, "Index")
            .mod (macro1, "osc2WarpAmount", 40).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Terminal Bite", "Stab").osc (1, analog, sawPos).unison (1, 3, 14, 40).osc (2, analog, sawPos, 55).pitch (2, 0, 6)
            .filter (1, ladder24, 800, 10, 10, 30).env (2, 30, 280, 30, 150, 60).mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 13)
            .amp (10, 450, 30, 160).drive (tube, 20, 8800).reverb (20, 50, 0.6f, 50).level (-13).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Blare").mod (macro2, "filter1Resonance", 35).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Tilted Blip", "Stab").osc (1, pulse, 20).unison (1, 3, 8, 50).env (2, 0, 180, 0, 140, 80).mod (env2, "osc1Position", 20)
            .mod (env2, "filter1Cutoff", 33).filter (1, svfBand, 770, 25).amp (0, 260, 0, 140).delay (quarterD, 65, 22, 3400).reverb (15, 75, 6, 60)
            .level (-4).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Width").mod (macro2, "osc1Position", 30)
            .drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Tilted Smash", "Stab").osc (1, metal, 55).noise (15, 2).filter (1, comb, 300, 50).filter (2, svfBand, 1200, 30)
            .env (2, 0, 170, 0, 120, 85).mod (env2, "filter1Cutoff", 13).mod (env2, "osc1Position", 30).amp (0, 220, 0, 180).drive (tube, 20, 3300)
            .reverb (30, 80, 6.5f, 35).level (-4).macro (1, "Ring").mod (macro1, "filter1Resonance", 22).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Tundra Crash", "Stab").osc (1, metal, 80).noise (25).filter (1, comb, 220, 60, 0, 50).filter (2, svfBand, 310, 25)
            .env (2, 0, 150, 0, 120, 85).mod (env2, "filter1Cutoff", 8).mod (env2, "osc1Position", 20).amp (0, 200, 0, 220).drive (fuzz, 40, 10000)
            .reverb (20, 50, 1.2f, 50).level (-14).macro (1, "Ring").mod (macro1, "filter1Resonance", 22).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 25).macro (3, "Clang").mod (macro3, "osc1Position", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Umbral Bark", "Stab").osc (1, analog, sawPos).unison (1, 2, 8, 50).osc (2, analog, 80, 75).pitch (2, 0, 4)
            .filter (1, ladder24, 320, 20, 20, 30).env (2, 30, 320, 0, 150, 65).mod (env2, "filter1Cutoff", 37).mod (velocity, "filter1Cutoff", 5)
            .amp (5, 400, 30, 140).reverb (20, 75, 4.5f, 55).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Blare")
            .mod (macro2, "filter1Resonance", 35).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Vicious Yell", "Stab").osc (1, vowel, 10).unison (1, 2, 12).env (2, 0, 260, 0, 140, 80).mod (env2, "osc1Position", 45)
            .filter (1, svfBand, 1800).amp (3, 220, 0, 100).drive (hard, 45, 3200).reverb (20, 35, 1, 40).level (-16).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).macro (4, "Grit").mod (macro4, "fxDriveAmount", 40);
        add ("Violent Call", "Stab").osc (1, vowel, 10).unison (1, 4, 16, 70).osc (2, analog, sawPos, 50).pitch (2, -12, 9).env (2, 0, 220, 0, 140)
            .mod (env2, "osc1Position", 30).filter (1, svfBand, 2700, 20).amp (0, 320, 0, 220).drive (hard, 25).reverb (15, 80, 7.5f, 40)
            .level (-10).macro (1, "Vowel").mod (macro1, "osc1Position", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 40).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Warped Bark", "Stab").osc (1, vowel, 60).unison (1, 3, 10, 50).env (2, 0, 90, 0, 140, 80).mod (env2, "osc1Position", -20)
            .filter (1, svfBand, 1900, 30).amp (0, 180, 0, 220).drive (tube, 15, 6000).reverb (25, 85, 8, 65).level (-7).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).delay (eighthD, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Warped Blow", "Stab").osc (1, analog, sawPos).unison (1, 6, 18, 90).osc (2, analog, 50).pitch (2, -12, 6).unison (2, 2, 14)
            .osc (3, analog, 80, 50).pitch (3, 24).noise (25, 1).filter (1, svfLow, 2900, 5).env (2, 0, 140, 0, 200, 70)
            .mod (env2, "filter1Cutoff", 22).mod (env2, "noiseLevel", 10).amp (15, 580, 0, 280).reverb (25, 75, 6, 30).level (-17)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);

        // Chord, by name.
        add ("Adrift Trance", "Chord").osc (1, fm, 10).osc (2, fm, 10).pitch (2, 3).osc (3, fm, 10, 75).pitch (3, 14, 2).env (2, 0, 700, 0, 400, 70)
            .mod (env2, "osc1Position", 10).mod (env2, "osc2Position", 30).mod (env2, "osc3Position", 10).filter (1, svfLow, 3200, 5)
            .amp (0, 3100, 20, 900, 70).chorus (25, 0.6f, 40).delay (sixteenthD, 55, 30, 2900).reverb (15, 80, 5, 30).level (-15).macro (1, "Index")
            .mod (macro1, "osc1Position", 30).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Third").mod (macro4, "osc2Volume", -55);
        add ("Banished Dulcimer", "Chord").osc (1, string, 80, 90).warp (1, wOff, 65).osc (2, string, 80, 90).warp (2, wOff, 65).pitch (2, 7)
            .unison (2, 2, 2, 60, 25).osc (3, string, 80, 85).warp (3, wOff, 65).pitch (3, 12).unison (3, 2, 2, 60, 30).filter (1, svfLow, 7100, 5)
            .amp (0, 3400, 0, 800).drive (tube, 30, 3600).delay (quarter, 25, 26, 5200).reverb (40, 95, 8, 60).level (-18).macro (1, "Bright")
            .mod (macro1, "osc1Position", 30).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 25).macro (3, "Echo").mod (macro3, "fxDelayMix", 30)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Bruised Oath", "Chord").osc (1, additive, 15).osc (2, additive, 15, 70).pitch (2, 5).osc (3, additive, 15, 75).pitch (3, 7)
            .filter (1, svfLow, 2300, 5).amp (1, 300, 100, 100).chorus (35, 0.7f, 45).drive (tube, 25, 4400).reverb (15, 85, 7.5f, 60).level (-20)
            .macro (1, "Drawbars").mod (macro1, "osc1Position", 30).macro (2, "Third").mod (macro2, "osc2Volume", -55).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Caustic Harp", "Chord").osc (1, string, 40, 90).warp (1, wOff, 35).osc (2, string, 40).warp (2, wOff, 35).pitch (2, 3)
            .unison (2, 2, 0, 60, 25).osc (3, string, 40, 75).warp (3, wOff, 35).pitch (3, 7).unison (3, 2, 1, 60, 55).filter (1, ladder12, 2800, 5)
            .amp (0, 3100, 0, 500).drive (tube, 55, 6300).reverb (20, 80, 4, 50).level (-20).macro (1, "Bright").mod (macro1, "osc1Position", 30)
            .macro (2, "Ring").mod (macro2, "osc1WarpAmount", 25).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Chained Vision", "Chord").osc (1, fm, 15).osc (2, fm, 15, 85).pitch (2, 3, 2).osc (3, fm, 15, 70).pitch (3, 14, -3)
            .env (2, 0, 1300, 0, 400, 75).mod (env2, "osc1Position", 25).mod (env2, "osc2Position", 30).mod (env2, "osc3Position", 30)
            .filter (1, ladder12, 4300, 5).amp (0, 1500, 0, 900, 65).reverb (15, 100, 4.5f, 30).level (-12).macro (1, "Index")
            .mod (macro1, "osc1Position", 30).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Third").mod (macro4, "osc2Volume", -55);
        add ("Cracked Creed", "Chord").osc (1, analog, 80).osc (2, analog, 80, 70).pitch (2, 2, -3).osc (3, analog, 80, 75).pitch (3, 7, -1)
            .filter (1, ladder24, 730, 15, 30).env (2, 0, 180, 0, 180, 82).mod (env2, "filter1Cutoff", 28).mod (velocity, "filter1Cutoff", 5)
            .amp (0, 460, 0, 240).delay (eighthD, 40, 34, 4900).reverb (25, 80, 7.5f, 30).level (-11).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Crystal Phantom", "Chord").osc (1, fm, 35).osc (2, fm, 35, 75).pitch (2, 8, 1).osc (3, fm, 35, 85).pitch (3, 15)
            .env (2, 0, 1200, 0, 400, 60).mod (env2, "osc1Position", 15).mod (env2, "osc2Position", 15).mod (env2, "osc3Position", 30)
            .filter (1, ladder12, 7500, 5).amp (0, 2900, 20, 500, 70).delay (eighth, 35, 12, 2200).reverb (25, 70, 9, 50).level (-16)
            .macro (1, "Index").mod (macro1, "osc1Position", 30).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Third").mod (macro4, "osc2Volume", -55);
        add ("Dawnless Dogma", "Chord").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, 5, -3).osc (3, analog, sawPos, 70)
            .pitch (3, 10, 1).unison (1, 2, 6, 60).unison (2, 2, 8, 70).unison (3, 2, 12, 50).filter (1, ladder12, 750, 45)
            .env (2, 0, 270, 0, 180, 76).mod (env2, "filter1Cutoff", 28).mod (velocity, "filter1Cutoff", 14).amp (0, 240, 0, 180)
            .delay (quarterT, 54, 36, 3900).reverb (35, 75, 7.5f, 45).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Dormant Sanctuary", "Chord").osc (1, pulse, 15).osc (2, pulse, 15, 85).pitch (2, -9, -4).osc (3, pulse, 15, 65).pitch (3, 7, 3)
            .unison (1, 3, 12, 90).unison (2, 3, 14, 90).unison (3, 3, 12).filter (1, ladder24, 2800, 20).lfo (1, sine, 0.32f, 0)
            .mod (lfo1, "filter1Cutoff", 12).amp (900, 1000, 95, 4300).chorus (35, 0.6f, 50).reverb (55, 80, 7.5f, 70).level (-16)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Third").mod (macro2, "osc2Volume", -55).macro (3, "Motion")
            .mod (macro3, "lfo1Rate", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Drifting Asylum", "Chord").osc (1, analog, sawPos).osc (2, analog, sawPos, 70).pitch (2, -5, -4).osc (3, analog, sawPos, 75)
            .pitch (3, 3, -2).unison (1, 3, 10, 90).unison (2, 3, 14).unison (3, 3, 12, 70).filter (1, svfLow, 2500, 35).lfo (1, sine, 0.13f, 0)
            .mod (lfo1, "filter1Cutoff", 11).amp (1400, 1000, 100, 3700).reverb (40, 100, 6, 40).level (-16).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Third").mod (macro2, "osc2Volume", -55).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Echoing Haven", "Chord").osc (1, additive, 35).osc (2, additive, 35, 70).pitch (2, 7, -1).osc (3, additive, 35, 75).pitch (3, 10, 1)
            .unison (1, 2, 10, 60).unison (2, 2, 6, 70).unison (3, 2, 14, 60).filter (1, ladder24, 2100, 35).lfo (1, sine, 0.093f, 0)
            .mod (lfo1, "filter1Cutoff", 14).amp (2000, 1000, 90, 1600).chorus (30, 0.3f, 50).reverb (40, 95, 7.5f, 65).level (-11)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Third").mod (macro2, "osc2Volume", -55).drive (tube, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Faceless Doctrine", "Chord").osc (1, analog, sawPos).osc (2, analog, sawPos, 65).pitch (2, 7, 3).osc (3, analog, sawPos, 85)
            .pitch (3, 12, -3).unison (1, 2, 14, 70).unison (2, 2, 10, 100).unison (3, 2, 12, 100).filter (1, ladder24, 1300, 40)
            .env (2, 0, 150, 0, 200, 88).mod (env2, "filter1Cutoff", 28).mod (velocity, "filter1Cutoff", 9).amp (0, 220, 0, 120)
            .delay (quarterD, 52, 20, 2000).reverb (20, 70, 5, 40).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Far Oath", "Chord").osc (1, additive, 35).osc (2, additive, 35, 75).pitch (2, -9, 4).osc (3, additive, 35).pitch (3, 7, -1)
            .filter (1, ladder12, 2200, 5).amp (10, 300, 40, 100).chorus (40, 0.7f, 20).reverb (35, 70, 8, 30).level (-9).macro (1, "Drawbars")
            .mod (macro1, "osc1Position", 30).macro (2, "Third").mod (macro2, "osc2Volume", -55).drive (tube, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Cutoff").mod (macro4, "filter1Cutoff", 20);
        add ("Fearless Pledge", "Chord").osc (1, additive, 5).osc (2, additive, 5, 65).pitch (2, 5, 1).osc (3, additive, 5, 70).pitch (3, 7)
            .filter (1, ladder12, 3200, 5).amp (1, 300, 40, 200).chorus (30, 0.7f, 45).reverb (30, 80, 6, 30).level (-9).macro (1, "Drawbars")
            .mod (macro1, "osc1Position", 30).macro (2, "Third").mod (macro2, "osc2Volume", -55).drive (tube, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Rotor").mod (macro4, "fxChorusMix", 40);
        add ("Flooded Harbour", "Chord").osc (1, pulse, 50).osc (2, pulse, 50, 75).pitch (2, 4, 4).osc (3, pulse, 50, 70).pitch (3, 7, -3)
            .unison (1, 3, 8, 50).unison (2, 3, 10, 90).unison (3, 3, 12, 60).filter (1, ladder24, 1800, 30).lfo (1, drift, 0.15f, 0)
            .mod (lfo1, "filter1Cutoff", 5).amp (500, 1000, 100, 3200).reverb (40, 90, 8, 65).level (-15).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Third").mod (macro2, "osc2Volume", -55).drive (tube, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Frosted Harp", "Chord").osc (1, string, 60, 90).warp (1, wOff, 35).osc (2, string, 60, 75).warp (2, wOff, 35).pitch (2, 8)
            .unison (2, 2, 3, 60, 25).osc (3, string, 60, 70).warp (3, wOff, 35).pitch (3, 15).unison (3, 2, 5, 60, 35).filter (1, svfLow, 4700, 5)
            .amp (0, 3700, 0, 600).drive (hard, 55, 3200).delay (eighthT, 55, 24, 2000).reverb (15, 75, 8, 50).level (-24).macro (1, "Bright")
            .mod (macro1, "osc1Position", 30).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 25).macro (3, "Echo").mod (macro3, "fxDelayMix", 30)
            .macro (4, "Cutoff").mod (macro4, "filter1Cutoff", 20);
        add ("Gated Sanctuary", "Chord").osc (1, analog, 80).osc (2, analog, 80, 85).pitch (2, 3, 2).osc (3, analog, 80, 65).pitch (3, 7, -1)
            .unison (1, 2, 8, 100).unison (2, 2, 14, 70).unison (3, 2, 12, 90).filter (1, svfLow, 900, 25).lfo (1, drift, 0.15f, 0)
            .mod (lfo1, "filter1Cutoff", 12).amp (1700, 1000, 100, 3000).reverb (45, 75, 6.5f, 55).level (-18).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Third").mod (macro2, "osc2Volume", -55).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Gaunt Daydream", "Chord").osc (1, fm, 15).osc (2, fm, 15, 75).pitch (2, 2, 2).osc (3, fm, 15).pitch (3, 7, 4)
            .env (2, 0, 800, 0, 400, 60).mod (env2, "osc1Position", 15).mod (env2, "osc2Position", 30).mod (env2, "osc3Position", 10)
            .filter (1, svfLow, 6400, 5).amp (0, 2800, 0, 800).delay (eighth, 35, 18, 5200).reverb (15, 100, 3, 70).level (-16).macro (1, "Index")
            .mod (macro1, "osc1Position", 30).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Third").mod (macro4, "osc2Volume", -55);
        add ("Ghost Lute", "Chord").osc (1, string, 40, 90).warp (1, wOff, 65).osc (2, string, 40).warp (2, wOff, 65).pitch (2, 7)
            .unison (2, 2, 6, 60, 25).osc (3, string, 40, 90).warp (3, wOff, 65).pitch (3, 12).unison (3, 2, 0, 60, 55).filter (1, svfLow, 3800, 5)
            .amp (0, 3000, 0, 400).drive (tube, 45, 3200).delay (quarterT, 40, 16, 6400).reverb (40, 85, 3.5f, 50).level (-21).macro (1, "Bright")
            .mod (macro1, "osc1Position", 30).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 25).macro (3, "Echo").mod (macro3, "fxDelayMix", 30)
            .macro (4, "Cutoff").mod (macro4, "filter1Cutoff", 20);
        add ("Gnarled Zither", "Chord").osc (1, string, 80, 90).warp (1, wOff, 70).osc (2, string, 80, 90).warp (2, wOff, 70).pitch (2, 5)
            .unison (2, 2, 3, 60, 10).osc (3, string, 80, 75).warp (3, wOff, 70).pitch (3, 7).unison (3, 2, 0, 60, 30).filter (1, svfLow, 5800, 5)
            .amp (0, 1600, 0, 500).drive (hard, 50, 3400).delay (quarter, 40, 20, 4400).reverb (15, 85, 3.5f, 45).level (-25).macro (1, "Bright")
            .mod (macro1, "osc1Position", 30).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 25).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Cutoff").mod (macro4, "filter1Cutoff", 20);
        add ("Grave Revolt", "Chord").osc (1, pulse, 10).osc (2, pulse, 10, 85).pitch (2, -5, -1).osc (3, pulse, 10, 70).pitch (3, 3, 1)
            .unison (1, 3, 12).unison (2, 3, 10, 100).unison (3, 3, 12, 90).env (2, 0, 70, 0, 150, 70).mod (env2, "osc1Fine", -30)
            .mod (env2, "filter1Cutoff", 18).filter (1, ladder24, 940).amp (0, 300, 50, 350).chorus (40, 0.7f, 50).reverb (25, 100, 7.5f, 50)
            .level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Width").mod (macro2, "osc1Position", 30)
            .delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Grey Lyre", "Chord").osc (1, string, 60, 90).warp (1, wOff, 45).osc (2, string, 60, 75).warp (2, wOff, 45).pitch (2, 3)
            .unison (2, 2, 3, 60, 20).osc (3, string, 60, 85).warp (3, wOff, 45).pitch (3, 7).unison (3, 2, 4, 60, 50).filter (1, svfLow, 7400, 5)
            .amp (0, 3100, 0, 700).delay (eighthD, 35, 12, 2100).reverb (30, 75, 3, 30).level (-12).macro (1, "Bright")
            .mod (macro1, "osc1Position", 30).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 25).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Hard Siege", "Chord").osc (1, pulse, 20).osc (2, pulse, 20, 85).pitch (2, -9).osc (3, pulse, 20).pitch (3, 7, -2)
            .unison (1, 4, 12, 100).unison (2, 4, 14, 70).unison (3, 4, 12).env (2, 0, 120, 0, 150, 60).mod (env2, "osc1Fine", 40)
            .mod (env2, "filter1Cutoff", 26).filter (1, ladder12, 1700, 25).amp (0, 600, 0, 300).chorus (40, 0.8f, 50).reverb (15, 75, 5, 60)
            .level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Width").mod (macro2, "osc1Position", 30)
            .delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Haunted Asylum", "Chord").osc (1, vowel, 55).osc (2, vowel, 55).pitch (2, 8).osc (3, vowel, 55, 65).pitch (3, 15, -2)
            .unison (1, 2, 14, 60).unison (2, 2, 6, 70).unison (3, 2, 8, 90).filter (1, ladder24, 910, 20).lfo (1, sine, 0.29f, 0)
            .mod (lfo1, "filter1Cutoff", 10).amp (2200, 1000, 100, 2500).chorus (45, 0.4f, 50).reverb (40, 90, 7.5f, 60).level (-8)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Third").mod (macro2, "osc2Volume", -55).macro (3, "Motion")
            .mod (macro3, "lfo1Rate", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Hostile Zither", "Chord").osc (1, string, 70, 90).warp (1, wOff, 70).osc (2, string, 70, 85).warp (2, wOff, 70).pitch (2, -9)
            .unison (2, 2, 3, 60, 15).osc (3, string, 70, 70).warp (3, wOff, 70).pitch (3, 7).unison (3, 2, 1, 60, 45).filter (1, ladder12, 3900, 5)
            .amp (0, 3200, 0, 800).reverb (30, 90, 5.5f, 70).level (-9).macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Ring")
            .mod (macro2, "osc1WarpAmount", 25).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Hunted Pledge", "Chord").osc (1, additive, 15).osc (2, additive, 15, 85).pitch (2, 3, -4).osc (3, additive, 15).pitch (3, 7, 1)
            .filter (1, ladder12, 4100, 5).amp (1, 300, 40, 240).chorus (35, 5, 35).reverb (20, 40, 1.4f, 70).level (-11).macro (1, "Drawbars")
            .mod (macro1, "osc1Position", 30).macro (2, "Third").mod (macro2, "osc2Volume", -55).macro (3, "Cutoff")
            .mod (macro3, "filter1Cutoff", 20).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Indigo Sanctuary", "Chord").osc (1, vowel, 25).osc (2, vowel, 25).pitch (2, -9, -3).osc (3, vowel, 25, 65).pitch (3, 7, 4)
            .unison (1, 3, 12, 70).unison (2, 3, 8).unison (3, 3, 8).filter (1, ladder12, 2300, 30).lfo (1, sine, 0.39f, 0)
            .mod (lfo1, "filter1Cutoff", 10).amp (2300, 1000, 90, 3600).chorus (45, 0.6f, 50).reverb (40, 85, 6, 65).level (-10).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Third").mod (macro2, "osc2Volume", -55).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Infected Mirage", "Chord").osc (1, fm, 20).osc (2, fm, 20, 85).pitch (2, 2, -1).osc (3, fm, 20, 65).pitch (3, 7, 2)
            .env (2, 0, 900, 0, 400, 70).mod (env2, "osc1Position", 25).mod (env2, "osc2Position", 25).mod (env2, "osc3Position", 25)
            .filter (1, svfLow, 2200, 5).amp (0, 1800, 0, 300, 55).chorus (40, 0.8f, 40).delay (eighth, 30, 22, 4700).reverb (15, 90, 8.5f, 35)
            .level (-14).macro (1, "Index").mod (macro1, "osc1Position", 30).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Third").mod (macro4, "osc2Volume", -55);
        add ("Ivory Spectre", "Chord").osc (1, fm, 35).osc (2, fm, 35, 70).pitch (2, 4, 4).osc (3, fm, 35, 65).pitch (3, 7, 1)
            .env (2, 0, 900, 0, 400, 60).mod (env2, "osc1Position", 15).mod (env2, "osc2Position", 20).mod (env2, "osc3Position", 20)
            .filter (1, ladder12, 5500, 5).amp (0, 2400, 0, 400, 55).chorus (35, 0.3f, 40).reverb (35, 100, 7.5f, 55).level (-12).macro (1, "Index")
            .mod (macro1, "osc1Position", 30).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Third").mod (macro4, "osc2Volume", -55);
        add ("Lingering Lyre", "Chord").osc (1, string, 60, 90).warp (1, wOff, 35).osc (2, string, 60, 90).warp (2, wOff, 35).pitch (2, 5)
            .unison (2, 2, 4, 60, 15).osc (3, string, 60, 75).warp (3, wOff, 35).pitch (3, 10).unison (3, 2, 4, 60, 40)
            .filter (1, ladder12, 3300, 5).amp (0, 1800, 0, 400).delay (quarter, 55, 14, 7800, false).reverb (40, 100, 7, 30).level (-8)
            .macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 25).macro (3, "Cutoff")
            .mod (macro3, "filter1Cutoff", 20).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Mangled Revolt", "Chord").osc (1, pulse, 20).osc (2, pulse, 20, 65).pitch (2, 3, 1).osc (3, pulse, 20, 85).pitch (3, 7, 2)
            .unison (1, 3, 14, 90).unison (2, 3, 12).unison (3, 3, 12, 90).env (2, 0, 210, 0, 150, 60).mod (env2, "osc1Fine", -30)
            .mod (env2, "filter1Cutoff", 26).filter (1, ladder12, 2200, 30).amp (0, 300, 30, 200).chorus (35, 0.3f, 50).reverb (30, 70, 6, 55)
            .level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Width").mod (macro2, "osc1Position", 30)
            .drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Masked Vow", "Chord").osc (1, additive, 40).osc (2, additive, 40).pitch (2, -5, 2).osc (3, additive, 40).pitch (3, 3, -4)
            .filter (1, svfLow, 3300, 5).amp (10, 300, 40, 60).chorus (40, 4.5f, 25).drive (hard, 40, 4900).reverb (20, 75, 4, 45).level (-22)
            .macro (1, "Drawbars").mod (macro1, "osc1Position", 30).macro (2, "Third").mod (macro2, "osc2Volume", -55).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Misted Creed", "Chord").osc (1, analog, 80).osc (2, analog, 80, 85).pitch (2, 7).osc (3, analog, 80, 70).pitch (3, 15, 1)
            .filter (1, ladder12, 410, 40, 30).env (2, 0, 300, 0, 120, 70).mod (env2, "filter1Cutoff", 18).mod (velocity, "filter1Cutoff", 12)
            .amp (0, 200, 0, 260).delay (eighthD, 60, 34, 2200).reverb (20, 95, 8.5f, 50).level (-10).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Misted Verdict", "Chord").osc (1, analog, 80).osc (2, analog, 80, 65).pitch (2, 7, 4).osc (3, analog, 80, 70).pitch (3, 12, 1)
            .unison (1, 2, 8, 90).unison (2, 2, 14, 70).unison (3, 2, 12, 60).filter (1, ladder24, 1200, 20).env (2, 0, 200, 0, 160, 88)
            .mod (env2, "filter1Cutoff", 36).mod (velocity, "filter1Cutoff", 12).amp (0, 500, 0, 120).delay (eighth, 42, 30, 1600)
            .reverb (15, 80, 3, 60).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Nameless Pledge", "Chord").osc (1, additive, 30).osc (2, additive, 30).pitch (2, 5, 4).osc (3, additive, 30, 75).pitch (3, 7, -1)
            .filter (1, ladder12, 5900, 5).amp (1, 300, 0, 100).chorus (55, 5, 35).reverb (20, 95, 7.5f, 70).level (-9).macro (1, "Drawbars")
            .mod (macro1, "osc1Position", 30).macro (2, "Third").mod (macro2, "osc2Volume", -55).macro (3, "Rotor").mod (macro3, "fxChorusMix", 40)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Nomad Mutiny", "Chord").osc (1, pulse, 15).osc (2, pulse, 15, 65).pitch (2, 3, -4).osc (3, pulse, 15, 65).pitch (3, 10, 1)
            .unison (1, 4, 10, 90).unison (2, 4, 8, 100).unison (3, 4, 10, 50).env (2, 0, 150, 0, 150, 75).mod (env2, "osc1Fine", -50)
            .mod (env2, "filter1Cutoff", 20).filter (1, svfLow, 920, 15).amp (0, 700, 30, 200).chorus (30, 0.7f, 50).drive (tube, 20, 3400)
            .reverb (35, 100, 4.5f, 60).level (-19).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Width")
            .mod (macro2, "osc1Position", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Overnight Vision", "Chord").osc (1, fm, 35).osc (2, fm, 35).pitch (2, -9, -1).osc (3, fm, 35).pitch (3, 7, -1)
            .env (2, 0, 300, 0, 400, 60).mod (env2, "osc1Position", 15).mod (env2, "osc2Position", 25).mod (env2, "osc3Position", 20)
            .filter (1, svfLow, 2400, 5).amp (0, 2300, 0, 800).chorus (35, 1, 40).delay (eighthD, 35, 14, 3900, false).reverb (15, 70, 4, 30)
            .level (-15).macro (1, "Index").mod (macro1, "osc1Position", 30).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Third").mod (macro4, "osc2Volume", -55);
        add ("Padded Sanctuary", "Chord").osc (1, vowel, 15).osc (2, vowel, 15, 70).pitch (2, 10, -4).osc (3, vowel, 15, 65).pitch (3, 15)
            .unison (1, 3, 10, 100).unison (2, 3, 8, 100).unison (3, 3, 6, 70).filter (1, svfLow, 1400, 25).lfo (1, sine, 0.17f, 0)
            .mod (lfo1, "filter1Cutoff", 12).amp (2000, 1000, 100, 3700).reverb (30, 90, 5.5f, 45).level (-15).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Third").mod (macro2, "osc2Volume", -55).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Perimeter Siege", "Chord").osc (1, pulse, 35).osc (2, pulse, 35, 70).pitch (2, 3, 4).osc (3, pulse, 35, 85).pitch (3, 7, 4)
            .unison (1, 4, 6, 50).unison (2, 4, 10, 50).unison (3, 4, 6, 90).env (2, 0, 90, 0, 150, 65).mod (env2, "osc1Fine", 20)
            .mod (env2, "filter1Cutoff", 16).filter (1, ladder12, 1000, 35).amp (0, 700, 0, 300).chorus (25, 0.9f, 50).reverb (20, 35, 1.6f, 50)
            .level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Width").mod (macro2, "osc1Position", 30)
            .delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Phosphor Pact", "Chord").osc (1, additive, 15).osc (2, additive, 15, 65).pitch (2, 8, 2).osc (3, additive, 15, 65).pitch (3, 15, 3)
            .filter (1, svfLow, 4700, 5).amp (30, 300, 100, 220).chorus (45, 5, 35).reverb (30, 95, 4.5f, 70).level (-14).macro (1, "Drawbars")
            .mod (macro1, "osc1Position", 30).macro (2, "Third").mod (macro2, "osc2Volume", -55).macro (3, "Rotor").mod (macro3, "fxChorusMix", 40)
            .macro (4, "Cutoff").mod (macro4, "filter1Cutoff", 20);
        add ("Quarantine Decree", "Chord").osc (1, analog, sawPos).osc (2, analog, sawPos, 85).pitch (2, 3).osc (3, analog, sawPos, 70)
            .pitch (3, 14, 2).unison (1, 2, 12, 100).unison (2, 2, 12, 50).unison (3, 2, 14, 60).filter (1, ladder24, 820, 25)
            .env (2, 0, 290, 0, 100, 88).mod (env2, "filter1Cutoff", 32).mod (velocity, "filter1Cutoff", 13).amp (0, 280, 0, 280)
            .delay (eighth, 60, 30, 3100).reverb (25, 70, 8, 65).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Raw Pact", "Chord").osc (1, additive, 20).osc (2, additive, 20, 75).pitch (2, 7, -2).osc (3, additive, 20, 70).pitch (3, 12, 3)
            .filter (1, ladder12, 6800, 5).amp (30, 300, 40, 120).chorus (55, 5, 35).drive (soft, 50, 5000).reverb (15, 55, 1, 60).level (-20)
            .macro (1, "Drawbars").mod (macro1, "osc1Position", 30).macro (2, "Third").mod (macro2, "osc2Volume", -55).macro (3, "Cutoff")
            .mod (macro3, "filter1Cutoff", 20).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Receding Shelter", "Chord").osc (1, additive, 45).osc (2, additive, 45, 65).pitch (2, -9, 4).osc (3, additive, 45, 70).pitch (3, 7, 3)
            .unison (1, 2, 8, 100).unison (2, 2, 8, 70).unison (3, 2, 14, 90).filter (1, ladder12, 790, 35).lfo (1, drift, 0.076f, 0)
            .mod (lfo1, "filter1Cutoff", 6).amp (2200, 1000, 95, 1700).chorus (40, 0.6f, 50).reverb (55, 80, 3.5f, 55).level (-11)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Third").mod (macro2, "osc2Volume", -55).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Sepia Phantom", "Chord").osc (1, fm, 10).osc (2, fm, 10, 70).pitch (2, 3).osc (3, fm, 10).pitch (3, 6, -3).env (2, 0, 800, 0, 400, 60)
            .mod (env2, "osc1Position", 10).mod (env2, "osc2Position", 30).mod (env2, "osc3Position", 20).filter (1, ladder12, 6700, 5)
            .amp (0, 3300, 20, 300, 65).phaser (30, 0.15f, 50).delay (eighthD, 55, 14, 2000).reverb (30, 80, 7, 45).level (-14).macro (1, "Index")
            .mod (macro1, "osc1Position", 30).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Third").mod (macro4, "osc2Volume", -55);
        add ("Soft Refuge", "Chord").osc (1, vowel, 40).osc (2, vowel, 40, 70).pitch (2, 7, -2).osc (3, vowel, 40, 75).pitch (3, 15, 4)
            .unison (1, 3, 6, 70).unison (2, 3, 10).unison (3, 3, 10).filter (1, svfLow, 690).lfo (1, triangle, 0.24f, 0)
            .mod (lfo1, "filter1Cutoff", 5).amp (1000, 1000, 100, 2200).chorus (45, 0.8f, 50).reverb (35, 70, 6.5f, 35).level (-10)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Third").mod (macro2, "osc2Volume", -55).macro (3, "Motion")
            .mod (macro3, "lfo1Rate", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Spartan Dogma", "Chord").osc (1, analog, sawPos).osc (2, analog, sawPos, 85).pitch (2, 3).osc (3, analog, sawPos, 70)
            .pitch (3, 14, -1).filter (1, ladder24, 420, 25).env (2, 0, 250, 0, 200, 76).mod (env2, "filter1Cutoff", 24)
            .mod (velocity, "filter1Cutoff", 10).amp (0, 340, 0, 180).delay (eighthD, 44, 36, 2000).reverb (20, 100, 4, 65).level (-10)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Spartan Siege", "Chord").osc (1, pulse, 20).osc (2, pulse, 20, 75).pitch (2, 7, 3).osc (3, pulse, 20, 70).pitch (3, 10)
            .unison (1, 4, 8).unison (2, 4, 10, 90).unison (3, 4, 6, 60).env (2, 0, 150, 0, 150, 65).mod (env2, "osc1Fine", 30)
            .mod (env2, "filter1Cutoff", 10).filter (1, ladder12, 1900, 30).amp (0, 450, 0, 150).chorus (35, 0.7f, 50).drive (tube, 35, 9700)
            .reverb (10, 30, 1.2f, 40).level (-20).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Width")
            .mod (macro2, "osc1Position", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Spectral Decree", "Chord").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, 2, 4).osc (3, analog, sawPos, 70)
            .pitch (3, 7).filter (1, svfLow, 900, 30, 10).env (2, 0, 190, 0, 140, 88).mod (env2, "filter1Cutoff", 32)
            .mod (velocity, "filter1Cutoff", 11).amp (0, 500, 0, 220).delay (quarterD, 56, 34, 4400).reverb (25, 80, 6.5f, 65).level (-14)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Split Revolt", "Chord").osc (1, pulse, 35).osc (2, pulse, 35, 70).pitch (2, -9, 1).osc (3, pulse, 35, 65).pitch (3, 7, 1)
            .unison (1, 4, 10, 100).unison (2, 4, 10).unison (3, 4, 10, 50).env (2, 0, 220, 0, 150, 70).mod (env2, "osc1Fine", -30)
            .mod (env2, "filter1Cutoff", 12).filter (1, svfLow, 1600, 35).amp (0, 250, 50, 150).chorus (40, 0.8f, 50).drive (tube, 25, 2700)
            .reverb (15, 55, 1.6f, 40).level (-17).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Width")
            .mod (macro2, "osc1Position", 30).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).delay (eighthD, 40, 0, 4500).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Stray Mutiny", "Chord").osc (1, pulse, 20).osc (2, pulse, 20, 70).pitch (2, 5, 2).osc (3, pulse, 20, 65).pitch (3, 10, 3)
            .unison (1, 2, 10, 100).unison (2, 2, 6, 50).unison (3, 2, 6, 60).env (2, 0, 120, 0, 150, 80).mod (env2, "osc1Fine", -40)
            .mod (env2, "filter1Cutoff", 26).filter (1, svfLow, 1600, 35).amp (0, 300, 0, 150).chorus (25, 0.3f, 50).drive (hard, 30, 3300)
            .reverb (20, 35, 1, 70).level (-23).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Width")
            .mod (macro2, "osc1Position", 30).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Sunless Doctrine", "Chord").osc (1, analog, sawPos).osc (2, analog, sawPos, 65).pitch (2, 3, 4).osc (3, analog, sawPos, 85)
            .pitch (3, 10, -3).unison (1, 2, 8, 100).unison (2, 2, 12, 60).unison (3, 2, 8, 50).filter (1, ladder12, 590, 40, 10)
            .env (2, 0, 210, 0, 120, 82).mod (env2, "filter1Cutoff", 20).mod (velocity, "filter1Cutoff", 7).amp (0, 360, 0, 180)
            .delay (quarterT, 44, 24, 2000).reverb (30, 90, 9, 45).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Thin Pledge", "Chord").osc (1, additive, 40).osc (2, additive, 40, 65).pitch (2, 3, -4).osc (3, additive, 40, 65).pitch (3, 14, 1)
            .filter (1, svfLow, 6500, 5).amp (1, 300, 40, 140).chorus (50, 4.5f, 45).reverb (10, 35, 1.6f, 40).level (-9).macro (1, "Drawbars")
            .mod (macro1, "osc1Position", 30).macro (2, "Third").mod (macro2, "osc2Volume", -55).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .macro (4, "Cutoff").mod (macro4, "filter1Cutoff", 20);
        add ("Tracked Canon", "Chord").osc (1, analog, 80).osc (2, analog, 80, 85).pitch (2, 8, 2).osc (3, analog, 80, 65).pitch (3, 15, -3)
            .unison (1, 2, 8, 90).unison (2, 2, 8, 90).unison (3, 2, 10, 60).filter (1, svfLow, 1200, 35, 30).env (2, 0, 90, 0, 120, 82)
            .mod (env2, "filter1Cutoff", 28).mod (velocity, "filter1Cutoff", 11).amp (0, 360, 0, 120).delay (eighthD, 46, 20, 2600)
            .reverb (25, 100, 5, 35).level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 35);
        add ("Trailing Phantom", "Chord").osc (1, fm, 30).osc (2, fm, 30, 65).pitch (2, 3, 3).osc (3, fm, 30, 65).pitch (3, 10, 3)
            .env (2, 0, 1300, 0, 400, 75).mod (env2, "osc1Position", 30).mod (env2, "osc2Position", 10).mod (env2, "osc3Position", 15)
            .filter (1, ladder12, 2100, 5).amp (0, 1700, 0, 600, 55).chorus (45, 1, 40).reverb (35, 75, 3, 70).level (-9).macro (1, "Index")
            .mod (macro1, "osc1Position", 30).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Third").mod (macro4, "osc2Volume", -55);
        add ("Twisted Mutiny", "Chord").osc (1, pulse, 45).osc (2, pulse, 45, 65).pitch (2, -5, -2).osc (3, pulse, 45, 85).pitch (3, 3, -1)
            .unison (1, 2, 8).unison (2, 2, 6, 70).unison (3, 2, 12, 70).env (2, 0, 210, 0, 150, 60).mod (env2, "osc1Fine", 20)
            .mod (env2, "filter1Cutoff", 26).filter (1, svfLow, 2000, 30).amp (0, 250, 30, 300).chorus (35, 0.5f, 50).drive (hard, 20, 3100)
            .reverb (10, 35, 1.6f, 70).level (-19).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Width")
            .mod (macro2, "osc1Position", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Vicious Curfew", "Chord").osc (1, pulse, 40).osc (2, pulse, 40, 65).pitch (2, 10, -4).osc (3, pulse, 40, 70).pitch (3, 15, -3)
            .unison (1, 2, 10, 60).unison (2, 2, 6, 70).unison (3, 2, 6, 60).env (2, 0, 110, 0, 150, 70).mod (env2, "osc1Fine", -50)
            .mod (env2, "filter1Cutoff", 12).filter (1, ladder24, 1300, 35).amp (0, 550, 0, 250).chorus (25, 0.4f, 50).drive (hard, 20, 4500)
            .reverb (10, 30, 1.6f, 50).level (-17).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Width")
            .mod (macro2, "osc1Position", 30).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Violet Zither", "Chord").osc (1, string, 40, 90).warp (1, wOff, 70).osc (2, string, 40, 90).warp (2, wOff, 70).pitch (2, 2)
            .unison (2, 2, 6, 60, 10).osc (3, string, 40, 90).warp (3, wOff, 70).pitch (3, 7).unison (3, 2, 3, 60, 55).filter (1, svfLow, 7500, 5)
            .amp (0, 3500, 0, 900).drive (hard, 25, 3500).reverb (20, 80, 6.5f, 65).level (-15).macro (1, "Bright").mod (macro1, "osc1Position", 30)
            .macro (2, "Ring").mod (macro2, "osc1WarpAmount", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Cutoff")
            .mod (macro4, "filter1Cutoff", 20);
        add ("Western Daydream", "Chord").osc (1, fm, 10).osc (2, fm, 10, 70).pitch (2, 3, 1).osc (3, fm, 10, 75).pitch (3, 14, 2)
            .env (2, 0, 800, 0, 400, 60).mod (env2, "osc1Position", 30).mod (env2, "osc2Position", 15).mod (env2, "osc3Position", 10)
            .filter (1, svfLow, 7900, 5).amp (0, 1200, 0, 800, 65).phaser (45, 0.37f, 55).reverb (35, 95, 4, 65).level (-12).macro (1, "Index")
            .mod (macro1, "osc1Position", 30).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Third").mod (macro4, "osc2Volume", -55);
        add ("White Cittern", "Chord").osc (1, string, 55, 90).warp (1, wOff, 50).osc (2, string, 55, 70).warp (2, wOff, 50).pitch (2, -9)
            .unison (2, 2, 3, 60, 20).osc (3, string, 55).warp (3, wOff, 50).pitch (3, 7).unison (3, 2, 2, 60, 25).filter (1, svfLow, 7000, 5)
            .amp (0, 2400, 0, 900).delay (eighthT, 55, 26, 6100, false).reverb (20, 70, 4.5f, 65).level (-9).macro (1, "Bright")
            .mod (macro1, "osc1Position", 30).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 25).macro (3, "Echo").mod (macro3, "fxDelayMix", 30)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Zone Vow", "Chord").osc (1, additive, 10).osc (2, additive, 10, 65).pitch (2, 7, 2).osc (3, additive, 10, 85).pitch (3, 15, 2)
            .filter (1, ladder12, 3500, 5).amp (30, 300, 100, 200).chorus (55, 0.7f, 25).reverb (10, 55, 1.2f, 60).level (-13).macro (1, "Drawbars")
            .mod (macro1, "osc1Position", 30).macro (2, "Third").mod (macro2, "osc2Volume", -55).macro (3, "Rotor").mod (macro3, "fxChorusMix", 40)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);

        // Pluck, by name.
        add ("Ashen Drop", "Pluck").osc (1, sync, 40).osc (2, pulse, 0, 45).pitch (2, 19, 4).filter (1, ladder24, 260, 40, 0, 70)
            .env (2, 0, 300, 0, 200, 75).mod (env2, "filter1Cutoff", 30).mod (velocity, "filter1Cutoff", 6).amp (0, 520, 0, 250)
            .delay (sixteenthD, 30, 28, 2900).level (-10).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).macro (3, "Second").mod (macro3, "osc2Volume", -45).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Caged Nib", "Pluck").osc (1, pulse, 60, 1).noise (1, 0, 20).env (2, 0, 9, 0, 15, 40).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 65).filter (1, comb, 261.6f, 97, 0, 100).filter (2, ladder12, 6500, 10, 0, 50).amp (0, 1400, 0, 350)
            .reverb (24, 70, 5, 65).level (-1).macro (1, "Damp").mod (macro1, "filter1Resonance", -15).macro (2, "Bright")
            .mod (macro2, "filter2Cutoff", 22).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Clinical Tinkle", "Pluck").osc (1, analog, sinePos, 1).pitch (1, 16).osc (2, analog, sinePos).warp (2, wFm, 12)
            .osc (3, analog, sinePos, 60).pitch (3, -12).env (2, 0, 100, 0, 100, 86).mod (env2, "osc2WarpAmount", 35)
            .mod (velocity, "osc2WarpAmount", 8).filter (1, ladder12, 9200, 5, 0, 30).amp (0, 350, 0, 250, 60).reverb (24, 90, 8.5f, 40).level (-12)
            .macro (1, "Index").mod (macro1, "osc2WarpAmount", 40).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Condemned Stifle", "Pluck").osc (1, string, 35, 90).warp (1, wOff, 10).osc (2, analog, sawPos, 55).pitch (2, -12)
            .filter (1, ladder24, 1600, 15, 30, 50).env (2, 0, 150, 0, 100, 85).mod (env2, "filter1Cutoff", 18).mod (velocity, "filter1Cutoff", 6)
            .amp (0, 300, 0, 120).drive (tube, 30, 4100).delay (eighthD, 40, 14, 3500, false).reverb (10, 50, 1, 60).level (-12).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 35).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Bright").mod (macro4, "osc1Position", 30);
        add ("Diamond Filament", "Pluck").osc (1, string, 40, 90).warp (1, wOff, 50).osc (2, string, 35, 70).warp (2, wOff, 30).pitch (2, -12, 5)
            .unison (1, 2, 7, 60, 20).filter (1, ladder24, 5100).mod (velocity, "osc1Position", 10).amp (0, 2300, 0, 800)
            .delay (eighthT, 50, 14, 7200, false).reverb (25, 100, 9, 70).level (-7).macro (1, "Bright").mod (macro1, "osc1Position", 30)
            .macro (2, "Ring").mod (macro2, "osc1WarpAmount", 30).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Cutoff")
            .mod (macro4, "filter1Cutoff", 20);
        add ("Flayed Chime", "Pluck").osc (1, analog, sinePos, 1).pitch (1, 16).osc (2, analog, sinePos).warp (2, wFm, 8)
            .osc (3, analog, sinePos, 60).pitch (3, 24).env (2, 0, 140, 0, 100, 90).mod (env2, "osc2WarpAmount", 45)
            .mod (velocity, "osc2WarpAmount", 18).filter (1, ladder12, 6900, 5, 0, 30).amp (0, 500, 0, 300, 65).delay (quarter, 55, 26, 5700, false)
            .reverb (22, 80, 7.5f, 55).level (-14).macro (1, "Index").mod (macro1, "osc2WarpAmount", 40).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Forgotten Dew", "Pluck").osc (1, analog, sawPos).osc (2, analog, squarePos, 65).pitch (2, -12, 6).filter (1, ladder24, 440, 25, 0, 50)
            .env (2, 0, 220, 0, 160, 90).mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 12).amp (0, 540, 0, 350)
            .delay (sixteenthD, 35, 26, 3000, false).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).macro (3, "Second").mod (macro3, "osc2Volume", -45).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Furious Tink", "Pluck").osc (1, analog, sinePos, 1).pitch (1, 24).osc (2, analog, sinePos).warp (2, wFm, 4).env (2, 0, 90, 0, 100, 86)
            .mod (env2, "osc2WarpAmount", 30).mod (velocity, "osc2WarpAmount", 18).filter (1, svfLow, 3800, 5, 0, 30).amp (0, 1200, 0, 400, 55)
            .delay (eighthD, 45, 14, 7300, false).reverb (13, 90, 4, 30).level (-12).macro (1, "Index").mod (macro1, "osc2WarpAmount", 40)
            .macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Hooded Choke", "Pluck").osc (1, string, 25, 90).warp (1, wOff, 10).filter (1, ladder24, 770, 35, 30, 50).env (2, 0, 80, 0, 100, 85)
            .mod (env2, "filter1Cutoff", 14).mod (velocity, "filter1Cutoff", 12).amp (0, 240, 0, 80).drive (hard, 15, 3900)
            .delay (eighthD, 50, 20, 3200).reverb (25, 45, 0.8f, 50).level (-8).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Ring").mod (macro2, "osc1WarpAmount", 35).macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Leaning Bristle", "Pluck").osc (1, string, 40, 90).warp (1, wOff, 25).osc (2, string, 55, 75).warp (2, wOff, 45).pitch (2, 19, 7)
            .unison (1, 2, 7, 40, 5).filter (1, ladder24, 6700, 20).mod (velocity, "osc1Position", 15).amp (0, 2200, 0, 400)
            .delay (quarter, 50, 18, 6700).reverb (25, 80, 4, 35).level (-6).macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Ring")
            .mod (macro2, "osc1WarpAmount", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Lowlight Tinkle", "Pluck").osc (1, analog, sinePos, 1).pitch (1, 24).osc (2, analog, sinePos).warp (2, wFm, 8)
            .osc (3, analog, sinePos, 55).pitch (3, -12).env (2, 0, 90, 0, 100, 86).mod (env2, "osc2WarpAmount", 50)
            .mod (velocity, "osc2WarpAmount", 8).filter (1, ladder12, 4200, 5, 0, 30).amp (0, 600, 0, 250, 55).delay (eighthT, 45, 24, 4300)
            .reverb (27, 75, 4.5f, 60).level (-13).macro (1, "Index").mod (macro1, "osc2WarpAmount", 40).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Lucid Fibre", "Pluck").osc (1, string, 80, 90).warp (1, wOff, 35).osc (2, string, 50, 60).warp (2, wOff, 40).pitch (2, 0, 2)
            .filter (1, svfLow, 6900, 5).mod (velocity, "osc1Position", 15).amp (0, 1500, 0, 500).delay (eighth, 55, 22, 2200)
            .reverb (20, 100, 7.5f, 60).level (-10).macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Ring")
            .mod (macro2, "osc1WarpAmount", 30).macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 20).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Marked Gag", "Pluck").osc (1, string, 35, 90).warp (1, wOff, 5).filter (1, ladder24, 2400, 15, 0, 50).env (2, 0, 160, 0, 100, 85)
            .mod (env2, "filter1Cutoff", 18).mod (velocity, "filter1Cutoff", 8).amp (0, 380, 0, 160).reverb (20, 40, 0.6f, 50).level (-4)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 35).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Bright").mod (macro4, "osc1Position", 30);
        add ("Merciless Pearl", "Pluck").osc (1, sync, 30).osc (2, pulse, 0, 70).pitch (2, -12, 1).filter (1, ladder24, 590, 35, 0, 50)
            .env (2, 0, 230, 0, 120, 85).mod (env2, "filter1Cutoff", 36).mod (velocity, "filter1Cutoff", 16).amp (0, 600, 0, 300)
            .delay (sixteenthD, 25, 14, 2700, false).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Second")
            .mod (macro4, "osc2Volume", -45);
        add ("Midnight Bead", "Pluck").osc (1, analog, sawPos).osc (2, analog, sawPos, 45).pitch (2, -12, 9).filter (1, ladder24, 300, 20, 0, 70)
            .env (2, 0, 140, 0, 240, 85).mod (env2, "filter1Cutoff", 36).mod (velocity, "filter1Cutoff", 8).amp (0, 780, 0, 300)
            .delay (quarter, 40, 24, 4900, false).reverb (18, 85, 6.5f, 35).level (-8).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Second").mod (macro3, "osc2Volume", -45).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Midnight Pearl", "Pluck").osc (1, sync, 55).osc (2, analog, sawPos, 55).pitch (2, 7).filter (1, ladder24, 650, 55, 0, 50)
            .env (2, 0, 150, 0, 200, 80).mod (env2, "filter1Cutoff", 38).mod (velocity, "filter1Cutoff", 16).amp (0, 400, 0, 300)
            .delay (quarter, 25, 18, 1900, false).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).reverb (0, 70, 3, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Murky Pearl", "Pluck").osc (1, sync, 40).osc (2, analog, squarePos, 45).pitch (2, 19, 6).filter (1, ladder24, 500, 25, 0, 50)
            .env (2, 0, 240, 0, 140, 85).mod (env2, "filter1Cutoff", 42).mod (velocity, "filter1Cutoff", 10).amp (0, 680, 0, 300)
            .delay (eighthD, 45, 16, 4900).reverb (16, 70, 9, 50).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Second")
            .mod (macro4, "osc2Volume", -45);
        add ("Nuclear Gag", "Pluck").osc (1, string, 30, 90).warp (1, wOff, 20).filter (1, ladder24, 770, 10, 0, 50).env (2, 0, 80, 0, 100, 85)
            .mod (env2, "filter1Cutoff", 10).mod (velocity, "filter1Cutoff", 12).amp (0, 380, 0, 160).reverb (20, 45, 0.8f, 50).level (-1)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 35).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Orphan Tooth", "Pluck").osc (1, pulse, 60, 1).noise (1, 1).env (2, 0, 30, 0, 15, 0).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 60).filter (1, comb, 261.6f, 96, 0, 100).filter (2, svfLow, 4900, 20).amp (0, 2100, 0, 350)
            .delay (sixteenthD, 35, 30, 6000).reverb (29, 95, 9, 30).level (-12).macro (1, "Damp").mod (macro1, "filter1Resonance", -15)
            .macro (2, "Bright").mod (macro2, "filter2Cutoff", 22).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Pewter Prong", "Pluck").osc (1, pulse, 60, 1).noise (1, 0, 60).env (2, 0, 12, 0, 15, 20).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 75).filter (1, comb, 261.6f, 94, 0, 100).filter (2, ladder12, 2400, 20, 0, 50).amp (0, 1300, 0, 400)
            .reverb (22, 85, 3, 50).level (-4).macro (1, "Damp").mod (macro1, "filter1Resonance", -15).macro (2, "Bright")
            .mod (macro2, "filter2Cutoff", 22).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Phosphor Filament", "Pluck").osc (1, string, 50, 90).warp (1, wOff, 40).osc (2, string, 40, 65).warp (2, wOff, 60).pitch (2, 7, 5)
            .filter (1, ladder12, 4900, 15).mod (velocity, "osc1Position", 25).amp (0, 2500, 0, 300).delay (quarterT, 35, 20, 2400)
            .reverb (30, 90, 8, 45).level (-10).macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Ring")
            .mod (macro2, "osc1WarpAmount", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Profane Pick", "Pluck").osc (1, pulse, 85, 1).noise (1, 1, -20).env (2, 0, 20, 0, 15, 0).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 65).filter (1, comb, 261.6f, 95, 0, 100).filter (2, ladder24, 3100, 20, 0, 50).amp (0, 1000, 0, 500)
            .delay (sixteenthD, 30, 28, 5000, false).reverb (14, 85, 7, 70).level (-6).macro (1, "Damp").mod (macro1, "filter1Resonance", -15)
            .macro (2, "Bright").mod (macro2, "filter2Cutoff", 22).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Ration Damp", "Pluck").osc (1, string, 20, 90).warp (1, wOff, 5).osc (2, analog, sawPos, 40).pitch (2, -12)
            .filter (1, svfLow, 870, 20, 20, 50).env (2, 0, 80, 0, 100, 85).mod (env2, "filter1Cutoff", 22).mod (velocity, "filter1Cutoff", 10)
            .amp (0, 320, 0, 140).reverb (10, 30, 1.6f, 60).level (-4).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Ring")
            .mod (macro2, "osc1WarpAmount", 35).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Bright")
            .mod (macro4, "osc1Position", 30);
        add ("Roadside Thread", "Pluck").osc (1, string, 60, 90).warp (1, wOff, 25).osc (2, string, 40, 55).warp (2, wOff, 25).pitch (2, 7, 1)
            .unison (1, 2, 3, 40, 15).filter (1, svfLow, 3800, 15).mod (velocity, "osc1Position", 25).amp (0, 1300, 0, 300).reverb (20, 75, 8, 45)
            .level (-7).macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 30)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 20).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Savage Bead", "Pluck").osc (1, sync, 25).osc (2, analog, squarePos, 55).pitch (2, 19, 4).filter (1, ladder24, 480, 40, 0, 50)
            .env (2, 0, 190, 0, 220, 80).mod (env2, "filter1Cutoff", 34).mod (velocity, "filter1Cutoff", 12).amp (0, 340, 0, 250)
            .delay (sixteenthD, 45, 24, 1900, false).level (-8).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).macro (3, "Second").mod (macro3, "osc2Volume", -45).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Savage Clink", "Pluck").osc (1, analog, sinePos, 1).pitch (1, 22).osc (2, analog, sinePos).warp (2, wFm, 0)
            .osc (3, analog, trianglePos, 45).pitch (3, 12).env (2, 0, 170, 0, 100, 90).mod (env2, "osc2WarpAmount", 30)
            .mod (velocity, "osc2WarpAmount", 20).filter (1, ladder12, 10000, 5, 0, 30).amp (0, 600, 0, 400, 60).reverb (16, 95, 8, 45).level (-12)
            .macro (1, "Index").mod (macro1, "osc2WarpAmount", 40).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Severed Ping", "Pluck").osc (1, analog, sinePos, 1).pitch (1, 16).osc (2, analog, sinePos).warp (2, wFm, 0).env (2, 0, 70, 0, 100, 82)
            .mod (env2, "osc2WarpAmount", 40).mod (velocity, "osc2WarpAmount", 14).filter (1, ladder12, 3900, 5, 0, 30).amp (0, 650, 0, 500, 55)
            .reverb (21, 85, 8, 65).level (-11).macro (1, "Index").mod (macro1, "osc2WarpAmount", 40).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Splintered Nib", "Pluck").osc (1, pulse, 60, 1).noise (1, 1, -20).env (2, 0, 11, 0, 15, 40).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 50).filter (1, comb, 261.6f, 95, 0, 100).filter (2, ladder12, 4400, 20, 0, 50).amp (0, 2400, 0, 500)
            .reverb (23, 80, 8.5f, 50).level (-2).macro (1, "Damp").mod (macro1, "filter1Resonance", -15).macro (2, "Bright")
            .mod (macro2, "filter2Cutoff", 22).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Splintered Tinkle", "Pluck").osc (1, analog, sinePos, 1).pitch (1, 12).osc (2, analog, sinePos).warp (2, wFm, 12)
            .osc (3, analog, trianglePos, 40).pitch (3, 24).env (2, 0, 150, 0, 100, 94).mod (env2, "osc2WarpAmount", 60)
            .mod (velocity, "osc2WarpAmount", 14).filter (1, ladder12, 3000, 5, 0, 30).amp (0, 950, 0, 600, 60).reverb (15, 85, 8.5f, 70)
            .level (-11).macro (1, "Index").mod (macro1, "osc2WarpAmount", 40).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30)
            .drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Stern Mute", "Pluck").osc (1, string, 35, 90).warp (1, wOff, 10).osc (2, analog, squarePos, 50).pitch (2, 12)
            .filter (1, ladder24, 1100, 15, 20, 50).env (2, 0, 100, 0, 100, 85).mod (env2, "filter1Cutoff", 16).mod (velocity, "filter1Cutoff", 12)
            .amp (0, 480, 0, 160).drive (hard, 40, 6000).reverb (20, 30, 0.6f, 40).level (-18).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Ring").mod (macro2, "osc1WarpAmount", 35).macro (3, "Space").mod (macro3, "fxReverbMix", 35).delay (eighthD, 40, 0, 4500)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Sullen Plink", "Pluck").osc (1, analog, sinePos, 1).pitch (1, 16).osc (2, analog, sinePos).warp (2, wFm, 0).env (2, 0, 60, 0, 100, 94)
            .mod (env2, "osc2WarpAmount", 20).mod (velocity, "osc2WarpAmount", 10).filter (1, svfLow, 4600, 5, 0, 30).amp (0, 1200, 0, 500, 55)
            .delay (sixteenthD, 45, 26, 4700).reverb (25, 80, 5, 30).level (-13).macro (1, "Index").mod (macro1, "osc2WarpAmount", 40)
            .macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Timeless Stud", "Pluck").osc (1, analog, 85, 1).noise (1, 1, -20).env (2, 0, 21, 0, 15, 20).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 80).filter (1, comb, 261.6f, 96, 0, 100).filter (2, ladder12, 2700, 5).amp (0, 2100, 0, 600)
            .delay (eighthT, 35, 22, 5700).reverb (25, 100, 6, 50).level (-7).macro (1, "Damp").mod (macro1, "filter1Resonance", -15)
            .macro (2, "Bright").mod (macro2, "filter2Cutoff", 22).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Titanium Stud", "Pluck").osc (1, pulse, 66.67f, 1).noise (1, 1, -20).env (2, 0, 18, 0, 15, 20).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 70).filter (1, comb, 261.6f, 98, 0, 100).filter (2, ladder12, 4700, 5, 0, 50).amp (0, 2100, 0, 650)
            .delay (eighth, 40, 14, 2000).reverb (20, 95, 5.5f, 35).level (-5).macro (1, "Damp").mod (macro1, "filter1Resonance", -15)
            .macro (2, "Bright").mod (macro2, "filter2Cutoff", 22).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Trapped Hair", "Pluck").osc (1, string, 40, 90).warp (1, wOff, 25).osc (2, string, 60).warp (2, wOff, 35).pitch (2, 7, 3)
            .unison (1, 2, 6, 70, 20).filter (1, svfLow, 5200, 5).mod (velocity, "osc1Position", 10).amp (0, 1300, 0, 400).reverb (15, 90, 4, 35)
            .level (-9).macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 30)
            .delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Cutoff").mod (macro4, "filter1Cutoff", 20);
        add ("Tundra Tooth", "Pluck").osc (1, pulse, 60, 1).noise (1, 0, -20).env (2, 0, 24, 0, 15, 0).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 80).filter (1, comb, 261.6f, 100, 0, 100).filter (2, svfLow, 3800, 5, 0, 50).amp (0, 1100, 0, 350)
            .delay (eighthD, 40, 26, 5100).reverb (22, 90, 4, 55).level (-11).macro (1, "Damp").mod (macro1, "filter1Resonance", -15)
            .macro (2, "Bright").mod (macro2, "filter2Cutoff", 22).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Umbral Dot", "Pluck").osc (1, sync, 25).osc (2, pulse, 25, 55).pitch (2, 7, 3).filter (1, ladder24, 370, 55, 0, 50)
            .env (2, 0, 130, 0, 140, 90).mod (env2, "filter1Cutoff", 32).mod (velocity, "filter1Cutoff", 8).amp (0, 440, 0, 350)
            .delay (eighth, 25, 12, 3300, false).reverb (29, 75, 8, 40).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Vicious Hush", "Pluck").osc (1, string, 35, 90).warp (1, wOff, 20).osc (2, analog, trianglePos, 40).pitch (2, -12)
            .filter (1, svfLow, 790, 10, 20, 50).env (2, 0, 140, 0, 100, 85).mod (env2, "filter1Cutoff", 24).mod (velocity, "filter1Cutoff", 10)
            .amp (0, 380, 0, 160).drive (hard, 30, 2600).delay (sixteenthD, 50, 20, 4700).reverb (25, 30, 0.8f, 50).level (-14).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 35).macro (3, "Echo").mod (macro3, "fxDelayMix", 30)
            .macro (4, "Bright").mod (macro4, "osc1Position", 30);
        add ("Zone Fibre", "Pluck").osc (1, string, 40, 90).warp (1, wOff, 30).osc (2, string, 65, 75).warp (2, wOff, 35).pitch (2, -12, 6)
            .filter (1, svfLow, 2900, 15).mod (velocity, "osc1Position", 25).amp (0, 2000, 0, 800).reverb (15, 95, 6, 70).level (-9)
            .macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 30).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Cutoff").mod (macro4, "filter1Cutoff", 20);
        add ("Zone Filament", "Pluck").osc (1, string, 40, 90).warp (1, wOff, 60).osc (2, string, 35, 70).warp (2, wOff, 45).pitch (2, -12, 2)
            .filter (1, ladder24, 3000).mod (velocity, "osc1Position", 15).amp (0, 2600, 0, 400).reverb (25, 95, 9, 70).level (-5)
            .macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Cutoff").mod (macro4, "filter1Cutoff", 20);

        // Arp, by name.
        add ("Abyssal Developer", "Arp").osc (1, analog, sawPos).pitch (1, -12).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 8.33f, 29.17f, 37.5f, 58.33f, 37.5f, 29.17f, 8.33f, 0, 8.33f, 29.17f, 37.5f, 58.33f, 79.17f, 58.33f, 37.5f })
            .mod (lfo1, "osc1Pitch", 50).lfoSync (2, steps, bar1, 1, 70)
            .stepPattern (2, { 100, -40, -100, -100, -100, -100, 50, 50, -100, -40, 50, 100, -100, 0, -40, 50 }).mod (lfo2, "filter1Cutoff", 16)
            .mod (lfo2, "filter1Resonance", 8).filter (1, ladder24, 310, 62, 50, 30).amp (2, 300, 100, 120).mono().drive (tube, 30, 4700)
            .level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance").mod (macro2, "filter1Resonance", 15)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 45).macro (4, "Accent").mod (macro4, "filter1Drive", 55);
        add ("Adrift Changes", "Arp").osc (1, analog, sinePos, 1).pitch (1, 7).osc (2, analog, sinePos).warp (2, wFm, 10).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 29.17f, 50, 29.17f, 0, 29.17f, 50, 29.17f, 0, 29.17f, 50, 29.17f, 0, 29.17f, 50, 29.17f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, quarter, 1, 97)
            .stepPattern (2, { 0, -10, -25, -45, -10, -20, -35, -55, -20, -30, -45, -65, -10, -20, -35, -55 }).mod (lfo2, "ampLevel", 85)
            .mod (lfo2, "osc2WarpAmount", 25).filter (1, ladder12, 3600, 5).amp (2, 300, 100, 800).delay (quarter, 35, 30, 2100)
            .reverb (20, 90, 8.5f, 65).level (-13).macro (1, "Index").mod (macro1, "osc2WarpAmount", 30).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Airless Cascade", "Arp").osc (1, sync, 50).lfoSync (1, steps, half)
            .stepPattern (1, { 50, 29.17f, 12.5f, 0, 50, 29.17f, 12.5f, 0, 50, 29.17f, 12.5f, 0, 50, 29.17f, 12.5f, 0 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, eighth, 1, 94).stepPattern (2, { 0, 0, -40, -100, 0, 0, -40, -100, -15, -15, -55, -100, 0, 0, -40, -100 })
            .mod (lfo2, "ampLevel", 85).mod (lfo2, "filter1Cutoff", 16).filter (1, ladder12, 830, 20).amp (2, 300, 100, 100).drive (crush, 25, 8500)
            .delay (eighth, 35, 22, 7700).reverb (16, 95, 4, 60).level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Black Reagent", "Arp").osc (1, analog, 85).pitch (1, -12).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 4.17f, 0, 4.17f, 0, 12.5f, 0, 4.17f, 0, 4.17f, 0, 4.17f, 29.17f, 25, 12.5f, 4.17f }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, bar1, 1, 40).stepPattern (2, { -40, -100, -100, -100, 50, 50, -40, 100, 100, 0, 100, -40, -100, 100, -100, 0 })
            .mod (lfo2, "filter1Cutoff", 20).mod (lfo2, "filter1Resonance", 8).filter (1, ladder24, 580, 83, 20, 30).amp (2, 300, 100, 60).mono()
            .level (-6).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance").mod (macro2, "filter1Resonance", 15)
            .drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 45).macro (4, "Accent").mod (macro4, "filter1Drive", 55);
        add ("Bleached Tower", "Arp").osc (1, analog, sinePos, 1).pitch (1, 7, 20).osc (2, analog, sinePos).warp (2, wFm, 35)
            .lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, 12.5f, 29.17f, 12.5f, 0, 12.5f, 33.33f, 12.5f, 0, 12.5f, 29.17f, 12.5f, -8.33f, 12.5f, 29.17f, 12.5f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -15, -40, -70, 0, -15, -40, -70, -15, -30, -55, -85, 0, -15, -40, -70 }).mod (lfo2, "ampLevel", 85)
            .mod (lfo2, "osc2WarpAmount", 40).filter (1, svfLow, 9400, 5).amp (2, 300, 100, 500).delay (eighthD, 30, 16, 5300)
            .reverb (30, 85, 6, 30).level (-14).macro (1, "Index").mod (macro1, "osc2WarpAmount", 30).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Bronze Burst", "Arp").osc (1, metal, 50).osc (2, pulse, 65, 55).pitch (2, 12, 1).lfoSync (1, steps, half)
            .stepPattern (1, { 0, 20.83f, 29.17f, 50, 0, 20.83f, 29.17f, 50, 0, 20.83f, 29.17f, 50, 0, 20.83f, 29.17f, 50 })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, eighth, 1, 94)
            .stepPattern (2, { 0, 0, 0, -60, -10, -10, -10, -70, -20, -20, -20, -80, -10, -10, -10, -70 }).mod (lfo2, "ampLevel", 75)
            .mod (lfo2, "filter1Cutoff", 22).filter (1, svfLow, 3000, 15).amp (2, 300, 100, 220).delay (eighthD, 30, 22, 3100, false).level (-9)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).reverb (0, 70, 3, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Bruised Aurora", "Arp").osc (1, additive, 45).unison (1, 3, 10, 100).osc (2, analog, trianglePos, 70).pitch (2, 0)
            .lfoSync (1, steps, bars2).stepPattern (1, { 0, 50, 100, 50, 0, 50, 100, 50, 0, 50, 100, 50, 0, 50, 100, 50 })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -8, -18, -30, 0, -8, -18, -30, -15, -23, -33, -45, 0, -8, -18, -30 }).mod (lfo2, "ampLevel", 85)
            .filter (1, ladder12, 1900, 25).amp (20, 300, 100, 1800).chorus (30, 0.7f, 50).delay (quarter, 50, 26, 4700).reverb (45, 95, 3.5f, 40)
            .level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Clinical Zodiac", "Arp").osc (1, additive, 10).unison (1, 3, 8, 100).osc (2, analog, trianglePos, 60).pitch (2, 12)
            .lfoSync (1, steps, bars2).stepPattern (1, { 0, 0, 0, 50, 0, 0, 41.67f, 0, 0, 0, 0, 50, 0, 29.17f, 41.67f, 50 })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -8, -18, -30, -12, -20, -30, -42, 0, -8, -18, -30, -12, -20, -30, -42 }).mod (lfo2, "ampLevel", 85)
            .filter (1, svfLow, 2200, 20).amp (200, 300, 100, 900).chorus (25, 0.8f, 50).delay (quarterT, 55, 24, 2200).reverb (30, 90, 3.5f, 65)
            .level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Cobalt Cascade", "Arp").osc (1, metal, 60).lfoSync (1, steps, half)
            .stepPattern (1, { 100, 79.17f, 62.5f, 50, 29.17f, 12.5f, 0, 12.5f, 29.17f, 50, 62.5f, 79.17f, 100, 79.17f, 62.5f, 50 })
            .mod (lfo1, "osc1Pitch", 50).lfoSync (2, steps, eighth, 1, 94)
            .stepPattern (2, { 0, 0, 0, -60, 0, 0, 0, -60, -15, -15, -15, -75, 0, 0, 0, -60 }).mod (lfo2, "ampLevel", 85)
            .mod (lfo2, "filter1Cutoff", 20).filter (1, ladder24, 720, 15).amp (2, 300, 100, 160).drive (tube, 35, 6700)
            .delay (eighthD, 45, 18, 2600).level (-2).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Compact Beat", "Arp").osc (1, analog, squarePos).pitch (1, -12).subOsc (40, 1).lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, 12.5f, 20.83f, 29.17f, 41.67f, 50, 62.5f, 70.83f, 79.17f, 70.83f, 62.5f, 50, 41.67f, 29.17f, 20.83f, 12.5f })
            .mod (lfo1, "osc1Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -15, -40, -70, -10, -25, -50, -80, -20, -35, -60, -90, -10, -25, -50, -80 }).mod (lfo2, "ampLevel", 70)
            .mod (lfo2, "filter1Cutoff", 22).filter (1, ladder12, 630, 50, 50).amp (2, 300, 100, 100).mono().drive (fuzz, 25, 10000).level (-23)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 25).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Compact Circuit", "Arp").osc (1, pulse, 65).pitch (1, 0).subOsc (35).lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, 12.5f, 29.17f, 50, 62.5f, 79.17f, 100, 79.17f, 62.5f, 50, 29.17f, 12.5f, 0, 12.5f, 29.17f, 50 })
            .mod (lfo1, "osc1Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -15, -40, -70, 0, -15, -40, -70, 0, -15, -40, -70, 0, -15, -40, -70 }).mod (lfo2, "ampLevel", 80)
            .mod (lfo2, "filter1Cutoff", 8).filter (1, ladder24, 660, 45, 30).amp (2, 300, 100, 100).mono().drive (tube, 15).level (-8)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 25).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Crawling Escapement", "Arp").osc (1, pulse, 15).osc (2, analog, squarePos, 55).pitch (2, 12, 2).lfoSync (1, steps, half)
            .stepPattern (1, { 50, 29.17f, 12.5f, 0, 50, 29.17f, 12.5f, 0, 50, 29.17f, 12.5f, 0, 50, 29.17f, 12.5f, 0 }).mod (lfo1, "osc1Pitch", 50)
            .mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, eighth, 1, 94)
            .stepPattern (2, { 0, 0, -60, -100, -10, -10, -70, -100, -20, -20, -80, -100, -10, -10, -70, -100 }).mod (lfo2, "ampLevel", 80)
            .mod (lfo2, "filter1Cutoff", 12).filter (1, svfLow, 690, 15, 0, 30).amp (2, 300, 100, 180).delay (quarter, 25, 14, 2300).level (-13)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).reverb (0, 70, 3, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Creeping Mechanism", "Arp").osc (1, sync, 45).osc (2, pulse, 15, 55).pitch (2, -12).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 12.5f, 29.17f, 12.5f, 0, 12.5f, 33.33f, 12.5f, 0, 12.5f, 29.17f, 12.5f, -8.33f, 12.5f, 29.17f, 12.5f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, quarter, 1, 97)
            .stepPattern (2, { 0, -40, -80, -100, -10, -50, -90, -100, -20, -60, -100, -100, -10, -50, -90, -100 }).mod (lfo2, "ampLevel", 75)
            .mod (lfo2, "filter1Cutoff", 14).filter (1, svfLow, 780, 50, 0, 30).amp (2, 300, 100, 240).delay (quarter, 50, 14, 3000).level (-12)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).reverb (0, 70, 3, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Downy Zenith", "Arp").osc (1, vowel, 45).unison (1, 3, 14, 100).osc (2, analog, trianglePos, 50).pitch (2, 12)
            .lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, 20.83f, 29.17f, 50, 0, 20.83f, 29.17f, 50, 0, 20.83f, 29.17f, 50, 0, 20.83f, 29.17f, 50 })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -10, -25, -45, -12, -22, -37, -57, 0, -10, -25, -45, -12, -22, -37, -57 }).mod (lfo2, "ampLevel", 80)
            .filter (1, svfLow, 1100, 30).amp (200, 300, 100, 1200).delay (quarter, 45, 20, 2200, false).reverb (45, 95, 7, 55).level (-11)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Fevered Beat", "Arp").osc (1, fm, 50).pitch (1, 0).subOsc (50).lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, 50, 29.17f, 50, 0, 50, 29.17f, 50, 0, 50, 12.5f, 50, 0, 50, 29.17f, 41.67f }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, half, 1, 99).stepPattern (2, { 0, -40, -80, -100, -12, -52, -92, -100, 0, -40, -80, -100, -12, -52, -92, -100 })
            .mod (lfo2, "ampLevel", 85).mod (lfo2, "filter1Cutoff", 18).filter (1, ladder24, 960, 20, 30).amp (2, 300, 100, 60).mono()
            .drive (tube, 45, 4000).level (-15).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Frayed Watch", "Arp").osc (1, analog, sawPos).pitch (1, 0).subOsc (40, 1).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 12.5f, 20.83f, 29.17f, 41.67f, 50, 62.5f, 70.83f, 79.17f, 70.83f, 62.5f, 50, 41.67f, 29.17f, 20.83f, 12.5f })
            .mod (lfo1, "osc1Pitch", 50).lfoSync (2, steps, quarter, 1, 97)
            .stepPattern (2, { 0, -40, -80, -100, 0, -40, -80, -100, 0, -40, -80, -100, 0, -40, -80, -100 }).mod (lfo2, "ampLevel", 85)
            .mod (lfo2, "filter1Cutoff", 20).filter (1, ladder24, 650, 55, 50).amp (2, 300, 100, 60).mono().level (-5).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 25).drive (hard, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Grave Picket", "Arp").osc (1, pulse, 55).pitch (1, -12).subOsc (40).lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, 50, 12.5f, 50, 29.17f, 50, 12.5f, 50, 0, 50, 12.5f, 50, 33.33f, 50, 29.17f, 50 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, half, 1, 99).stepPattern (2, { 0, 0, -60, -100, -10, -10, -70, -100, -20, -20, -80, -100, -10, -10, -70, -100 })
            .mod (lfo2, "ampLevel", 70).mod (lfo2, "filter1Cutoff", 16).filter (1, ladder12, 660, 40, 40).amp (2, 300, 100, 120).mono()
            .drive (hard, 30, 8400).level (-15).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Greased Patrol", "Arp").osc (1, analog, squarePos).pitch (1, -12).subOsc (40).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 12.5f, 29.17f, 41.67f, 0, 12.5f, 29.17f, 41.67f, 0, 12.5f, 29.17f, 41.67f, 0, 12.5f, 29.17f, 41.67f })
            .mod (lfo1, "osc1Pitch", 50).lfoSync (2, steps, quarter, 1, 97)
            .stepPattern (2, { 0, -40, -80, -100, -10, -50, -90, -100, -20, -60, -100, -100, -10, -50, -90, -100 }).mod (lfo2, "ampLevel", 70)
            .mod (lfo2, "filter1Cutoff", 10).filter (1, ladder24, 450, 35, 10).amp (2, 300, 100, 120).mono().level (-4).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 25).drive (hard, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Grid Peal", "Arp").osc (1, analog, sinePos, 1).pitch (1, 0).osc (2, analog, sinePos).warp (2, wFm, 10).lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, 20.83f, 29.17f, 50, 0, 20.83f, 29.17f, 50, 0, 20.83f, 29.17f, 50, 0, 20.83f, 29.17f, 50 })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -25, -55, -100, 0, -25, -55, -100, -15, -40, -70, -100, 0, -25, -55, -100 }).mod (lfo2, "ampLevel", 75)
            .mod (lfo2, "osc2WarpAmount", 35).filter (1, svfLow, 8200, 5).amp (2, 300, 100, 900).delay (quarter, 45, 18, 5500)
            .reverb (20, 70, 4.5f, 50).level (-13).macro (1, "Index").mod (macro1, "osc2WarpAmount", 30).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Hard Rounds", "Arp").osc (1, analog, sinePos, 1).pitch (1, 0).osc (2, analog, sinePos).warp (2, wFm, 30).lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, 29.17f, 50, 79.17f, 100, 79.17f, 50, 29.17f, 0, 29.17f, 50, 79.17f, 100, 79.17f, 50, 29.17f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -15, -40, -70, 0, -15, -40, -70, -15, -30, -55, -85, 0, -15, -40, -70 }).mod (lfo2, "ampLevel", 70)
            .mod (lfo2, "osc2WarpAmount", 35).filter (1, svfLow, 8300, 5).amp (2, 300, 100, 700).delay (quarter, 25, 18, 7300)
            .reverb (20, 85, 8, 50).level (-14).macro (1, "Index").mod (macro1, "osc2WarpAmount", 30).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Heartless Beat", "Arp").osc (1, analog, squarePos).pitch (1, -12).subOsc (30, 1).lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, 50, 29.17f, 50, 0, 50, 29.17f, 50, 0, 50, 12.5f, 50, 0, 50, 29.17f, 41.67f }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, half, 1, 99).stepPattern (2, { 0, -50, -100, -100, 0, -50, -100, -100, 0, -50, -100, -100, 0, -50, -100, -100 })
            .mod (lfo2, "ampLevel", 80).mod (lfo2, "filter1Cutoff", 16).filter (1, ladder12, 1100, 40, 20).amp (2, 300, 100, 60).mono().level (-8)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 25).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Hollowed Corona", "Arp").osc (1, analog, trianglePos).unison (1, 3, 10, 100).osc (2, analog, trianglePos, 55).pitch (2, 12)
            .lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, -20.83f, 0, 12.5f, 0, -20.83f, 0, 29.17f, 0, -20.83f, 0, 12.5f, 29.17f, 12.5f, 0, -8.33f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, quarter, 1, 97)
            .stepPattern (2, { -30, 0, -10, -25, -40, -10, -20, -35, -50, -20, -30, -45, -40, -10, -20, -35 }).mod (lfo2, "ampLevel", 80)
            .filter (1, svfLow, 1400, 30).amp (20, 300, 100, 1200).chorus (45, 0.3f, 50).delay (eighthD, 50, 26, 4700).reverb (40, 100, 4.5f, 65)
            .level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Hooded Carillon", "Arp").osc (1, analog, sinePos, 1).pitch (1, 12).osc (2, analog, sinePos).warp (2, wFm, 30)
            .lfoSync (1, steps, bars2).stepPattern (1, { 0, 50, 29.17f, 50, 0, 50, 29.17f, 50, 0, 50, 12.5f, 50, 0, 50, 29.17f, 41.67f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -25, -55, -100, 0, -25, -55, -100, -15, -40, -70, -100, 0, -25, -55, -100 }).mod (lfo2, "ampLevel", 85)
            .mod (lfo2, "osc2WarpAmount", 20).filter (1, svfLow, 4300, 5).amp (2, 300, 100, 500).delay (eighthD, 50, 16, 2700, false)
            .reverb (25, 95, 3, 55).level (-13).macro (1, "Index").mod (macro1, "osc2WarpAmount", 30).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Lingering Tower", "Arp").osc (1, analog, sinePos, 1).pitch (1, 12, 20).osc (2, analog, sinePos).warp (2, wFm, 20)
            .lfoSync (1, steps, bars2).stepPattern (1, { 0, 0, 50, 0, 0, 50, 0, 50, 0, 0, 50, 0, 29.17f, 50, 0, 50 }).mod (lfo1, "osc1Pitch", 50)
            .mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -10, -25, -45, 0, -10, -25, -45, 0, -10, -25, -45, 0, -10, -25, -45 }).mod (lfo2, "ampLevel", 85)
            .mod (lfo2, "osc2WarpAmount", 25).filter (1, ladder12, 3500, 5).amp (2, 300, 100, 600).delay (quarter, 35, 28, 5200)
            .reverb (30, 80, 9, 70).level (-14).macro (1, "Index").mod (macro1, "osc2WarpAmount", 30).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Listing Mainspring", "Arp").osc (1, sync, 40).osc (2, analog, squarePos, 50).pitch (2, 0, 4).lfoSync (1, steps, bar1)
            .stepPattern (1, { 100, 79.17f, 62.5f, 50, 29.17f, 12.5f, 0, 12.5f, 29.17f, 50, 62.5f, 79.17f, 100, 79.17f, 62.5f, 50 })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, quarter, 1, 97)
            .stepPattern (2, { 0, -10, -25, -45, 0, -10, -25, -45, 0, -10, -25, -45, 0, -10, -25, -45 }).mod (lfo2, "ampLevel", 80)
            .mod (lfo2, "filter1Cutoff", 8).filter (1, ladder24, 900, 35, 0, 30).amp (2, 300, 100, 260).delay (sixteenthD, 50, 12, 3000)
            .reverb (27, 90, 3.5f, 45).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Looping Solvent", "Arp").osc (1, analog, 85).pitch (1, 0).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 12.5f, 20.83f, 29.17f, 41.67f, 50, 62.5f, 70.83f, 79.17f, 70.83f, 62.5f, 50, 41.67f, 29.17f, 20.83f, 12.5f })
            .mod (lfo1, "osc1Pitch", 50).lfoSync (2, steps, bar1, 1, 30)
            .stepPattern (2, { 100, -100, -100, -40, -100, 50, 0, 50, -40, 0, 100, -100, -100, -100, -100, -100 }).mod (lfo2, "filter1Cutoff", 20)
            .mod (lfo2, "filter1Resonance", 10).filter (1, ladder24, 250, 71, 20, 30).amp (2, 300, 100, 60).mono().drive (tube, 25, 7400)
            .level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance").mod (macro2, "filter1Resonance", 15)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 45).macro (4, "Accent").mod (macro4, "filter1Drive", 55);
        add ("Marching Carillon", "Arp").osc (1, analog, sinePos, 1).pitch (1, 12).osc (2, analog, sinePos).warp (2, wFm, 25)
            .lfoSync (1, steps, bar1).stepPattern (1, { 0, 4.17f, 0, 4.17f, 0, 12.5f, 0, 4.17f, 0, 4.17f, 0, 4.17f, 29.17f, 25, 12.5f, 4.17f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, quarter, 1, 97)
            .stepPattern (2, { 0, -15, -40, -70, 0, -15, -40, -70, 0, -15, -40, -70, 0, -15, -40, -70 }).mod (lfo2, "ampLevel", 70)
            .mod (lfo2, "osc2WarpAmount", 35).filter (1, ladder12, 5100, 5).amp (2, 300, 100, 300).delay (quarter, 55, 26, 6400)
            .reverb (40, 100, 6, 65).level (-14).macro (1, "Index").mod (macro1, "osc2WarpAmount", 30).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Marching Escapement", "Arp").osc (1, sync, 25).osc (2, pulse, 45, 70).pitch (2, 12, 3).lfoSync (1, steps, bar1)
            .stepPattern (1, { 50, 0, 50, 0, 29.17f, 0, 29.17f, 0, 41.67f, 0, 41.67f, 0, 29.17f, 0, 12.5f, 0 }).mod (lfo1, "osc1Pitch", 50)
            .mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, quarter, 1, 97)
            .stepPattern (2, { 0, -50, -100, -100, 0, -50, -100, -100, -15, -65, -100, -100, 0, -50, -100, -100 }).mod (lfo2, "ampLevel", 80)
            .mod (lfo2, "filter1Cutoff", 12).filter (1, svfLow, 850, 40, 0, 30).amp (2, 300, 100, 180).delay (eighthD, 55, 14, 5600, false)
            .reverb (18, 75, 3, 70).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Nickel Scatter", "Arp").osc (1, metal, 60).osc (2, pulse, 50, 60).pitch (2, 12, 1).lfoSync (1, steps, half)
            .stepPattern (1, { 0, 29.17f, 50, 29.17f, 0, 29.17f, 50, 29.17f, 0, 29.17f, 50, 29.17f, 0, 29.17f, 50, 29.17f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, eighth, 1, 94)
            .stepPattern (2, { 0, -30, -70, -100, 0, -30, -70, -100, 0, -30, -70, -100, 0, -30, -70, -100 }).mod (lfo2, "ampLevel", 75)
            .mod (lfo2, "filter1Cutoff", 16).filter (1, ladder24, 3400, 35).amp (2, 300, 100, 100).drive (tube, 40, 6000)
            .delay (eighth, 45, 22, 3500).level (-18).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Numb Changes", "Arp").osc (1, analog, sinePos, 1).pitch (1, 12, 20).osc (2, analog, sinePos).warp (2, wFm, 15)
            .lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, -20.83f, 0, 12.5f, 0, -20.83f, 0, 29.17f, 0, -20.83f, 0, 12.5f, 29.17f, 12.5f, 0, -8.33f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -25, -55, -100, 0, -25, -55, -100, 0, -25, -55, -100, 0, -25, -55, -100 }).mod (lfo2, "ampLevel", 80)
            .mod (lfo2, "osc2WarpAmount", 20).filter (1, ladder12, 4600, 5).amp (2, 300, 100, 600).delay (quarterT, 25, 16, 3100, false)
            .reverb (20, 90, 4, 45).level (-12).macro (1, "Index").mod (macro1, "osc2WarpAmount", 30).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Olive Scatter", "Arp").osc (1, pulse, 5).osc (2, analog, sawPos, 45).pitch (2, -12, 3).lfoSync (1, steps, half)
            .stepPattern (1, { 100, 79.17f, 62.5f, 50, 29.17f, 12.5f, 0, 12.5f, 29.17f, 50, 62.5f, 79.17f, 100, 79.17f, 62.5f, 50 })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, eighth, 1, 94)
            .stepPattern (2, { 0, -30, -70, -100, -12, -42, -82, -100, 0, -30, -70, -100, -12, -42, -82, -100 }).mod (lfo2, "ampLevel", 70)
            .mod (lfo2, "filter1Cutoff", 16).filter (1, ladder24, 1400, 20).amp (2, 300, 100, 180).drive (crush, 30)
            .delay (sixteenthD, 40, 12, 2900).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Overnight Etchant", "Arp").osc (1, analog, squarePos).pitch (1, -12).lfoSync (1, steps, bar1)
            .stepPattern (1, { 50, 29.17f, 12.5f, 0, 50, 29.17f, 12.5f, 0, 50, 29.17f, 12.5f, 0, 50, 29.17f, 12.5f, 0 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, bar1, 1, 30).stepPattern (2, { -100, -100, 100, -100, 50, 100, 100, 50, 0, 100, 50, -100, 0, -100, 0, -100 })
            .mod (lfo2, "filter1Cutoff", 20).mod (lfo2, "filter1Resonance", 4).filter (1, ladder24, 290, 74, 10, 30).amp (2, 300, 100, 80).mono()
            .drive (fuzz, 20, 3100).delay (eighthD, 55, 20, 3000, false).level (-22).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 15).macro (3, "Grit").mod (macro3, "fxDriveAmount", 45).macro (4, "Accent")
            .mod (macro4, "filter1Drive", 55);
        add ("Pacing Reagent", "Arp").osc (1, analog, 85).pitch (1, 0).lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, 0, 50, 0, 0, 50, 0, 50, 0, 0, 50, 0, 29.17f, 50, 0, 50 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, bars2, 1, 50).stepPattern (2, { 0, 50, 50, 50, -100, -100, 100, -40, -100, 0, -100, -100, -40, -100, -100, 0 })
            .mod (lfo2, "filter1Cutoff", 22).mod (lfo2, "filter1Resonance", 8).filter (1, ladder24, 510, 83, 20, 30).amp (2, 300, 100, 80).mono()
            .level (-6).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance").mod (macro2, "filter1Resonance", 15)
            .drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 45).macro (4, "Accent").mod (macro4, "filter1Drive", 55);
        add ("Packed Picket", "Arp").osc (1, analog, squarePos).pitch (1, 0).subOsc (30, 1).lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, 20.83f, 29.17f, 50, 0, 20.83f, 29.17f, 50, 0, 20.83f, 29.17f, 50, 0, 20.83f, 29.17f, 50 })
            .mod (lfo1, "osc1Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -15, -40, -70, 0, -15, -40, -70, 0, -15, -40, -70, 0, -15, -40, -70 }).mod (lfo2, "ampLevel", 80)
            .mod (lfo2, "filter1Cutoff", 8).filter (1, ladder12, 650, 50, 10).amp (2, 300, 100, 80).mono().level (-9).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 25).drive (hard, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Perimeter Movement", "Arp").osc (1, pulse, 50).osc (2, pulse, 15, 60).pitch (2, 0).lfoSync (1, steps, bars2)
            .stepPattern (1, { 100, 79.17f, 62.5f, 50, 29.17f, 12.5f, 0, 12.5f, 29.17f, 50, 62.5f, 79.17f, 100, 79.17f, 62.5f, 50 })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -40, -80, -100, 0, -40, -80, -100, -15, -55, -95, -100, 0, -40, -80, -100 }).mod (lfo2, "ampLevel", 70)
            .mod (lfo2, "filter1Cutoff", 10).filter (1, ladder24, 670, 35, 0, 30).amp (2, 300, 100, 280).delay (quarter, 45, 14, 3700).level (-13)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).reverb (0, 70, 3, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Prowling Balance", "Arp").osc (1, sync, 50).osc (2, analog, squarePos, 45).pitch (2, -12, 5).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 0, 50, 0, 0, 50, 0, 50, 0, 0, 50, 0, 29.17f, 50, 0, 50 }).mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50)
            .lfoSync (2, steps, quarter, 1, 97).stepPattern (2, { 0, 0, -60, -100, -10, -10, -70, -100, -20, -20, -80, -100, -10, -10, -70, -100 })
            .mod (lfo2, "ampLevel", 85).mod (lfo2, "filter1Cutoff", 14).filter (1, svfLow, 2000, 20, 0, 30).amp (2, 300, 100, 100)
            .delay (sixteenthD, 50, 14, 4100, false).reverb (26, 90, 6, 30).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Receding Zenith", "Arp").osc (1, analog, trianglePos).unison (1, 3, 14, 100).osc (2, analog, sinePos, 50).pitch (2, 0)
            .lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, 20.83f, 29.17f, 50, 0, 20.83f, 29.17f, 50, 0, 20.83f, 29.17f, 50, 0, 20.83f, 29.17f, 50 })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -8, -18, -30, 0, -8, -18, -30, 0, -8, -18, -30, 0, -8, -18, -30 }).mod (lfo2, "ampLevel", 80)
            .filter (1, svfLow, 3800, 30).amp (20, 300, 100, 700).delay (quarterT, 30, 30, 5800).reverb (35, 85, 8, 30).level (-15)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Rolling Volley", "Arp").osc (1, metal, 45).osc (2, pulse, 45, 60).pitch (2, -12, 8).lfoSync (1, steps, half)
            .stepPattern (1, { 0, 29.17f, 50, 62.5f, 50, 29.17f, 0, 29.17f, 0, 29.17f, 50, 70.83f, 50, 29.17f, 0, 29.17f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, eighth, 1, 94)
            .stepPattern (2, { 0, 0, 0, -60, 0, 0, 0, -60, 0, 0, 0, -60, 0, 0, 0, -60 }).mod (lfo2, "ampLevel", 80).mod (lfo2, "filter1Cutoff", 18)
            .filter (1, svfBand, 850, 50).amp (2, 300, 100, 260).drive (crush, 35, 11000).delay (eighthD, 45, 12, 2100).level (-6)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).reverb (0, 70, 3, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Rusted Etchant", "Arp").osc (1, analog, sawPos).pitch (1, 0).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 50, 12.5f, 50, 29.17f, 50, 12.5f, 50, 0, 50, 12.5f, 50, 33.33f, 50, 29.17f, 50 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, bar1, 1, 50).stepPattern (2, { 100, 100, 0, 100, -100, 100, 50, 50, 50, 0, 100, -100, 0, -100, 100, -100 })
            .mod (lfo2, "filter1Cutoff", 12).mod (lfo2, "filter1Resonance", 8).filter (1, ladder24, 770, 80, 30, 30).amp (2, 300, 100, 60).mono()
            .drive (fuzz, 55, 2800).level (-29).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 15).macro (3, "Grit").mod (macro3, "fxDriveAmount", 45).macro (4, "Accent")
            .mod (macro4, "filter1Drive", 55);
        add ("Scalding Rounds", "Arp").osc (1, fm, 30).pitch (1, -12).subOsc (45, 1).lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, 8.33f, 12.5f, 29.17f, 0, 8.33f, 12.5f, 41.67f, 0, 8.33f, 12.5f, 29.17f, 50, 41.67f, 29.17f, 12.5f })
            .mod (lfo1, "osc1Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -15, -40, -70, 0, -15, -40, -70, -15, -30, -55, -85, 0, -15, -40, -70 }).mod (lfo2, "ampLevel", 80)
            .mod (lfo2, "filter1Cutoff", 20).filter (1, ladder24, 310, 40, 50).amp (2, 300, 100, 60).mono().drive (hard, 45, 2900).level (-18)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 25).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Seared Salvo", "Arp").osc (1, sync, 50).osc (2, pulse, 65, 55).pitch (2, 12, 8).lfoSync (1, steps, half)
            .stepPattern (1, { 0, 4.17f, 0, 4.17f, 0, 12.5f, 0, 4.17f, 0, 4.17f, 0, 4.17f, 29.17f, 25, 12.5f, 4.17f }).mod (lfo1, "osc1Pitch", 50)
            .mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, eighth, 1, 94)
            .stepPattern (2, { 0, 0, 0, -60, -10, -10, -10, -70, -20, -20, -20, -80, -10, -10, -10, -70 }).mod (lfo2, "ampLevel", 75)
            .mod (lfo2, "filter1Cutoff", 18).filter (1, ladder24, 970, 45).amp (2, 300, 100, 120).drive (hard, 20, 8100)
            .delay (eighthD, 25, 18, 3200, false).reverb (19, 80, 4.5f, 40).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Sepia Belfry", "Arp").osc (1, analog, sinePos, 1).pitch (1, 12).osc (2, analog, sinePos).warp (2, wFm, 30).lfoSync (1, steps, bars2)
            .stepPattern (1, { 50, 0, 50, 0, 29.17f, 0, 29.17f, 0, 41.67f, 0, 41.67f, 0, 29.17f, 0, 12.5f, 0 }).mod (lfo1, "osc1Pitch", 50)
            .mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -10, -25, -45, -10, -20, -35, -55, -20, -30, -45, -65, -10, -20, -35, -55 }).mod (lfo2, "ampLevel", 80)
            .mod (lfo2, "osc2WarpAmount", 35).filter (1, ladder12, 7000, 5).amp (2, 300, 100, 400).delay (quarterT, 30, 16, 3100)
            .reverb (35, 75, 7.5f, 40).level (-12).macro (1, "Index").mod (macro1, "osc2WarpAmount", 30).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Severed Patrol", "Arp").osc (1, fm, 40).pitch (1, -12).subOsc (30).lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, 12.5f, 20.83f, 29.17f, 41.67f, 50, 62.5f, 70.83f, 79.17f, 70.83f, 62.5f, 50, 41.67f, 29.17f, 20.83f, 12.5f })
            .mod (lfo1, "osc1Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -40, -80, -100, 0, -40, -80, -100, -15, -55, -95, -100, 0, -40, -80, -100 }).mod (lfo2, "ampLevel", 85)
            .mod (lfo2, "filter1Cutoff", 22).filter (1, ladder24, 840, 40, 40).amp (2, 300, 100, 140).mono().drive (hard, 20, 3000).level (-9)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 25).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Shadowed Catalyst", "Arp").osc (1, analog, 85).pitch (1, 0).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 0, 50, 0, 0, 50, 0, 50, 0, 0, 50, 0, 29.17f, 50, 0, 50 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, bar1, 1, 60).stepPattern (2, { -40, 100, 50, 0, -40, 0, -100, 50, 0, 0, 50, -100, -100, 100, -100, -40 })
            .mod (lfo2, "filter1Cutoff", 18).mod (lfo2, "filter1Resonance", 7).filter (1, ladder24, 290, 65, 40, 30).amp (2, 300, 100, 80).mono()
            .delay (eighthD, 40, 10, 2200).level (-6).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 15).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 45).macro (4, "Accent")
            .mod (macro4, "filter1Drive", 55);
        add ("Shifting Escapement", "Arp").osc (1, analog, sawPos).osc (2, analog, sawPos, 70).pitch (2, -12, 2).lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, -20.83f, 0, 12.5f, 0, -20.83f, 0, 29.17f, 0, -20.83f, 0, 12.5f, 29.17f, 12.5f, 0, -8.33f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -10, -25, -45, -10, -20, -35, -55, -20, -30, -45, -65, -10, -20, -35, -55 }).mod (lfo2, "ampLevel", 70)
            .mod (lfo2, "filter1Cutoff", 18).filter (1, ladder24, 2100, 25, 0, 30).amp (2, 300, 100, 100).delay (eighthD, 35, 14, 3100, false)
            .reverb (19, 100, 5.5f, 70).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Sleepless Chimes", "Arp").osc (1, analog, sinePos, 1).pitch (1, 0, 20).osc (2, analog, sinePos).warp (2, wFm, 35)
            .lfoSync (1, steps, bar1).stepPattern (1, { 0, 29.17f, 50, 62.5f, 50, 29.17f, 0, 29.17f, 0, 29.17f, 50, 70.83f, 50, 29.17f, 0, 29.17f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, quarter, 1, 97)
            .stepPattern (2, { 0, -25, -55, -100, 0, -25, -55, -100, -15, -40, -70, -100, 0, -25, -55, -100 }).mod (lfo2, "ampLevel", 85)
            .mod (lfo2, "osc2WarpAmount", 35).filter (1, ladder12, 8100, 5).amp (2, 300, 100, 500).delay (quarter, 40, 28, 6200, false)
            .reverb (30, 80, 3, 30).level (-13).macro (1, "Index").mod (macro1, "osc2WarpAmount", 30).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Solemn Balance", "Arp").osc (1, pulse, 30).osc (2, pulse, 5, 50).pitch (2, 7, 8).lfoSync (1, steps, half)
            .stepPattern (1, { 0, 8.33f, 12.5f, 29.17f, 0, 8.33f, 12.5f, 41.67f, 0, 8.33f, 12.5f, 29.17f, 50, 41.67f, 29.17f, 12.5f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, eighth, 1, 94)
            .stepPattern (2, { 0, -50, -100, -100, -12, -62, -100, -100, 0, -50, -100, -100, -12, -62, -100, -100 }).mod (lfo2, "ampLevel", 85)
            .mod (lfo2, "filter1Cutoff", 8).filter (1, ladder24, 800, 45, 0, 30).amp (2, 300, 100, 280).delay (eighthD, 50, 16, 5100)
            .reverb (27, 70, 5, 45).level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Spinning Etchant", "Arp").osc (1, analog, squarePos).pitch (1, 0).lfoSync (1, steps, bar1)
            .stepPattern (1, { 100, 79.17f, 62.5f, 50, 29.17f, 12.5f, 0, 12.5f, 29.17f, 50, 62.5f, 79.17f, 100, 79.17f, 62.5f, 50 })
            .mod (lfo1, "osc1Pitch", 50).lfoSync (2, steps, bar1, 1, 50)
            .stepPattern (2, { 50, -40, -100, -100, -100, 50, -100, 0, -100, -100, 100, -40, 100, -100, 100, -100 }).mod (lfo2, "filter1Cutoff", 22)
            .mod (lfo2, "filter1Resonance", 7).filter (1, ladder24, 350, 86, 30, 30).amp (2, 300, 100, 120).mono().drive (tube, 30, 3200)
            .level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance").mod (macro2, "filter1Resonance", 15)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 45).macro (4, "Accent").mod (macro4, "filter1Drive", 55);
        add ("Submerged Aurora", "Arp").osc (1, vowel, 15).unison (1, 2, 6, 100).osc (2, analog, sinePos, 60).pitch (2, 0).lfoSync (1, steps, bars2)
            .stepPattern (1, { 50, 0, 50, 0, 29.17f, 0, 29.17f, 0, 41.67f, 0, 41.67f, 0, 29.17f, 0, 12.5f, 0 }).mod (lfo1, "osc1Pitch", 50)
            .mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -8, -18, -30, 0, -8, -18, -30, 0, -8, -18, -30, 0, -8, -18, -30 }).mod (lfo2, "ampLevel", 85)
            .filter (1, ladder12, 2800, 20).amp (80, 300, 100, 1500).chorus (30, 0.3f, 50).delay (eighthD, 55, 20, 2800).reverb (35, 85, 3, 60)
            .level (-10).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Tagged Aurora", "Arp").osc (1, vowel, 40).unison (1, 2, 6, 100).osc (2, analog, sinePos, 50).pitch (2, 12).lfoSync (1, steps, bars2)
            .stepPattern (1, { 50, 0, 50, 0, 29.17f, 0, 29.17f, 0, 41.67f, 0, 41.67f, 0, 29.17f, 0, 12.5f, 0 }).mod (lfo1, "osc1Pitch", 50)
            .mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { -30, 0, -10, -25, -30, 0, -10, -25, -30, 0, -10, -25, -30, 0, -10, -25 }).mod (lfo2, "ampLevel", 75)
            .filter (1, ladder12, 3800, 20).amp (200, 300, 100, 600).chorus (40, 0.7f, 50).delay (quarterT, 40, 24, 7700).reverb (35, 100, 8.5f, 50)
            .level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Threadbare Salvo", "Arp").osc (1, sync, 35).lfoSync (1, steps, half)
            .stepPattern (1, { 0, 8.33f, 29.17f, 37.5f, 58.33f, 37.5f, 29.17f, 8.33f, 0, 8.33f, 29.17f, 37.5f, 58.33f, 79.17f, 58.33f, 37.5f })
            .mod (lfo1, "osc1Pitch", 50).lfoSync (2, steps, eighth, 1, 94)
            .stepPattern (2, { 0, -30, -70, -100, -10, -40, -80, -100, -20, -50, -90, -100, -10, -40, -80, -100 }).mod (lfo2, "ampLevel", 75)
            .mod (lfo2, "filter1Cutoff", 8).filter (1, svfLow, 1000, 20).amp (2, 300, 100, 180).delay (eighthD, 50, 22, 5500, false).level (-8)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).reverb (0, 70, 3, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Ticking Clockwork", "Arp").osc (1, pulse, 40).osc (2, analog, sawPos, 65).pitch (2, 0, 2).lfoSync (1, steps, half)
            .stepPattern (1, { 50, 0, 50, 0, 29.17f, 0, 29.17f, 0, 41.67f, 0, 41.67f, 0, 29.17f, 0, 12.5f, 0 }).mod (lfo1, "osc1Pitch", 50)
            .mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, eighth, 1, 94)
            .stepPattern (2, { 0, 0, -60, -100, 0, 0, -60, -100, 0, 0, -60, -100, 0, 0, -60, -100 }).mod (lfo2, "ampLevel", 70)
            .mod (lfo2, "filter1Cutoff", 20).filter (1, svfLow, 2400, 35, 0, 30).amp (2, 300, 100, 220).delay (sixteenthD, 50, 22, 7100)
            .reverb (13, 75, 8, 45).level (-17).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Timeless Meridian", "Arp").osc (1, analog, sawPos).unison (1, 2, 6, 100).osc (2, analog, trianglePos, 60).pitch (2, 0)
            .lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, 12.5f, 20.83f, 29.17f, 41.67f, 50, 62.5f, 70.83f, 79.17f, 70.83f, 62.5f, 50, 41.67f, 29.17f, 20.83f, 12.5f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, -10, -25, -45, 0, -10, -25, -45, -15, -25, -40, -60, 0, -10, -25, -45 }).mod (lfo2, "ampLevel", 80)
            .filter (1, ladder12, 1400, 30).amp (200, 300, 100, 1400).chorus (45, 0.6f, 50).delay (quarter, 25, 24, 2000).reverb (45, 100, 6, 40)
            .level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Trapped Corona", "Arp").osc (1, additive, 40).unison (1, 3, 6, 100).osc (2, additive, 0, 50).pitch (2, 12).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 29.17f, 50, 29.17f, 0, 29.17f, 50, 29.17f, 0, 29.17f, 50, 29.17f, 0, 29.17f, 50, 29.17f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, quarter, 1, 97)
            .stepPattern (2, { -30, 0, -10, -25, -30, 0, -10, -25, -30, 0, -10, -25, -30, 0, -10, -25 }).mod (lfo2, "ampLevel", 70)
            .filter (1, ladder12, 2200, 25).amp (200, 300, 100, 1100).delay (eighthD, 40, 26, 3800, false).reverb (50, 70, 9, 35).level (-12)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Twitching Aurora", "Arp").osc (1, pulse, 20).unison (1, 3, 14, 100).osc (2, analog, sinePos, 50).pitch (2, 0)
            .lfoSync (1, steps, bars2).stepPattern (1, { 50, 29.17f, 12.5f, 0, 50, 29.17f, 12.5f, 0, 50, 29.17f, 12.5f, 0, 50, 29.17f, 12.5f, 0 })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { -30, 0, -10, -25, -30, 0, -10, -25, -45, -15, -25, -40, -30, 0, -10, -25 }).mod (lfo2, "ampLevel", 80)
            .filter (1, svfLow, 1200, 25).amp (20, 300, 100, 1400).delay (quarterT, 40, 20, 4900).reverb (50, 85, 3, 40).level (-15)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Twitching Clockwork", "Arp").osc (1, pulse, 55).osc (2, analog, sawPos, 65).pitch (2, 0, 6).lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, 8.33f, 29.17f, 37.5f, 58.33f, 37.5f, 29.17f, 8.33f, 0, 8.33f, 29.17f, 37.5f, 58.33f, 79.17f, 58.33f, 37.5f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, half, 1, 99)
            .stepPattern (2, { 0, 0, -60, -100, -10, -10, -70, -100, -20, -20, -80, -100, -10, -10, -70, -100 }).mod (lfo2, "ampLevel", 70)
            .mod (lfo2, "filter1Cutoff", 12).filter (1, svfLow, 1300, 30, 0, 30).amp (2, 300, 100, 140).delay (eighthD, 35, 14, 4700).level (-16)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).reverb (0, 70, 3, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Umbral Developer", "Arp").osc (1, analog, 85).pitch (1, -12).lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, 29.17f, 50, 29.17f, 0, 29.17f, 50, 29.17f, 0, 29.17f, 50, 29.17f, 0, 29.17f, 50, 29.17f })
            .mod (lfo1, "osc1Pitch", 50).lfoSync (2, steps, bars2, 1, 70)
            .stepPattern (2, { -40, 50, -100, 0, 50, -40, 100, -100, -100, 50, 0, -40, -100, -100, 50, 100 }).mod (lfo2, "filter1Cutoff", 12)
            .mod (lfo2, "filter1Resonance", 5).filter (1, ladder24, 250, 65, 30, 30).amp (2, 300, 100, 140).mono().level (-5).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance").mod (macro2, "filter1Resonance", 15).drive (hard, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 45).macro (4, "Accent").mod (macro4, "filter1Drive", 55);
        add ("Unholy Salvo", "Arp").osc (1, analog, squarePos).osc (2, analog, sawPos, 60).pitch (2, 12, 2).lfoSync (1, steps, half)
            .stepPattern (1, { 0, 29.17f, 50, 29.17f, 0, 29.17f, 50, 29.17f, 0, 29.17f, 50, 29.17f, 0, 29.17f, 50, 29.17f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).lfoSync (2, steps, eighth, 1, 94)
            .stepPattern (2, { 0, -30, -70, -100, -12, -42, -82, -100, 0, -30, -70, -100, -12, -42, -82, -100 }).mod (lfo2, "ampLevel", 75)
            .mod (lfo2, "filter1Cutoff", 22).filter (1, svfBand, 3300, 40).amp (2, 300, 100, 160).drive (crush, 30, 8900)
            .delay (eighthD, 40, 24, 3200).reverb (25, 80, 7, 30).level (-8).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Violent Salvo", "Arp").osc (1, metal, 55).lfoSync (1, steps, half)
            .stepPattern (1, { 0, 25, 50, 25, 0, 25, 50, 75, 0, 25, 50, 25, 4.17f, 29.17f, 54.17f, 29.17f }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, eighth, 1, 94).stepPattern (2, { 0, 0, -40, -100, -10, -10, -50, -100, -20, -20, -60, -100, -10, -10, -50, -100 })
            .mod (lfo2, "ampLevel", 85).mod (lfo2, "filter1Cutoff", 18).filter (1, svfBand, 1600, 15).amp (2, 300, 100, 280).drive (hard, 15, 3100)
            .delay (eighth, 50, 22, 7700, false).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Watched Titrant", "Arp").osc (1, analog, squarePos).pitch (1, -12).lfoSync (1, steps, bars2)
            .stepPattern (1, { 0, 0, 0, 50, 0, 0, 41.67f, 0, 0, 0, 0, 50, 0, 29.17f, 41.67f, 50 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, bars2, 1, 30).stepPattern (2, { -100, 100, -40, -100, -40, 0, -100, -100, 50, 50, 100, -100, 100, 100, 100, -100 })
            .mod (lfo2, "filter1Cutoff", 16).mod (lfo2, "filter1Resonance", 10).filter (1, ladder24, 600, 68, 30, 30).amp (2, 300, 100, 80).mono()
            .delay (eighthD, 30, 20, 4600).level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 15).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 45).macro (4, "Accent")
            .mod (macro4, "filter1Drive", 55);

        // Keys, by name.
        add ("Afterhours Spank", "Keys").osc (1, pulse, 80).osc (2, pulse, 66.67f, 40).pitch (2, 12, 6).filter (1, ladderBand, 1600, 55, 30, 50)
            .filter (2, svfHigh, 200).env (2, 0, 160, 0, 150, 87).mod (env2, "filter1Cutoff", 25).mod (velocity, "filter1Cutoff", 14)
            .amp (0, 1050, 0, 100, 70).reverb (10, 40, 1.2f, 60).level (-4).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .phaser (0, 0.2f, 55).macro (4, "Phase").mod (macro4, "fxPhaserMix", 50);
        add ("Attic Manual", "Keys").osc (1, string, 75, 90).warp (1, wOff, 55).osc (2, string, 100, 65).warp (2, wOff, 40).pitch (2, -12)
            .filter (1, svfHigh, 230, 15).filter (2, ladder12, 8000, 5).amp (0, 2000, 0, 250, 70).reverb (30, 70, 9, 45).level (-9)
            .macro (1, "Bright").mod (macro1, "osc1Position", -40).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 25).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Backroom Bellows", "Keys").osc (1, pulse, 40).osc (2, analog, sawPos, 55).pitch (2, 0, 6).filter (1, ladder12, 1700, 15, 20, 60)
            .env (2, 0, 850, 40, 250, 70).mod (env2, "filter1Cutoff", 18).mod (velocity, "filter1Cutoff", 10).lfo (1, sine, 4.5f, 0)
            .mod (lfo1, "ampLevel", 4).amp (1, 2100, 50, 250, 60).drive (tube, 25, 6400).reverb (25, 50, 0.6f, 50).level (-19).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Tremolo").mod (macro2, "lfo1Rate", 20).chorus (0, 0.5f, 50).macro (3, "Wide")
            .mod (macro3, "fxChorusMix", 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Blackened Bellows", "Keys").osc (1, pulse, 50).osc (2, analog, trianglePos, 45).pitch (2, -12, 2).filter (1, svfLow, 550, 5, 40, 40)
            .env (2, 0, 450, 20, 250, 70).mod (env2, "filter1Cutoff", 12).mod (velocity, "filter1Cutoff", 18).lfo (1, sine, 4.5f, 0)
            .mod (lfo1, "ampLevel", 4).amp (3, 2600, 25, 300, 55).drive (tube, 40, 4000).reverb (15, 35, 0.8f, 40).level (-24).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Tremolo").mod (macro2, "lfo1Rate", 20).macro (3, "Width").mod (macro3, "osc1Position", 30)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Blistered Plectrum", "Keys").osc (1, string, 90, 90).warp (1, wOff, 55).osc (2, string, 85, 75).warp (2, wOff, 45).pitch (2, -12, 4)
            .filter (1, svfHigh, 300, 20).filter (2, svfLow, 7700, 5).amp (0, 2000, 0, 300, 65).reverb (14, 100, 3.5f, 35).level (-9)
            .macro (1, "Bright").mod (macro1, "osc1Position", -40).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 25).delay (eighthD, 40, 0, 4500)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Border Register", "Keys").osc (1, string, 95, 90).warp (1, wOff, 50).osc (2, string, 85, 65).warp (2, wOff, 35).pitch (2, 12, 2)
            .filter (1, svfHigh, 300, 15).filter (2, ladder12, 5700, 5).amp (0, 1800, 0, 150, 55).reverb (28, 85, 7, 70).level (-9)
            .macro (1, "Bright").mod (macro1, "osc1Position", -40).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 25).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Brackish Parlour", "Keys").osc (1, string, 40, 90).warp (1, wOff, 85).unison (1, 3, 5, 20).osc (2, string, 30, 60).warp (2, wOff, 75)
            .pitch (2, -12).filter (1, ladder24, 920, 5, 0, 60).mod (velocity, "osc1Position", 25).mod (velocity, "filter1Cutoff", 10)
            .amp (0, 7000, 0, 400).reverb (15, 90, 4.5f, 35).level (1).macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Ring")
            .mod (macro4, "osc1WarpAmount", 15);
        add ("Brass Mobile", "Keys").osc (1, metal, 45).osc (2, analog, sinePos, 60).pitch (2, 24).noise (1, 0, 40).env (2, 0, 30, 0, 20, 90)
            .mod (env2, "noiseLevel", 45).env (3, 0, 160, 0, 200, 80).mod (env3, "osc1Position", 16).filter (1, ladder12, 3300, 5, 0, 30)
            .amp (0, 550, 0, 450, 55).reverb (10, 30, 0.6f, 40).level (-8).macro (1, "Hammer").mod (macro1, "noiseLevel", 30).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Burnished Charm", "Keys").osc (1, fm, 25).osc (2, analog, sinePos).pitch (2, 24).noise (1, 0, 40).env (2, 0, 30, 0, 20, 90)
            .mod (env2, "noiseLevel", 30).env (3, 0, 160, 0, 200, 80).mod (env3, "osc1Position", 17).filter (1, svfLow, 6600, 5, 0, 30)
            .amp (0, 550, 0, 400, 75).reverb (15, 40, 0.6f, 60).delay (sixteenthD, 50, 12, 3100, false).level (-15).macro (1, "Hammer")
            .mod (macro1, "noiseLevel", 30).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).chorus (0, 0.5f, 50).macro (3, "Wide")
            .mod (macro3, "fxChorusMix", 45).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Crooked Mobile", "Keys").osc (1, metal, 10).osc (2, analog, sinePos, 55).pitch (2, 24).noise (1, 0, 40).env (2, 0, 20, 0, 20, 90)
            .mod (env2, "noiseLevel", 40).env (3, 0, 220, 0, 200, 80).mod (env3, "osc1Position", 10).filter (1, ladder12, 6100, 5, 0, 30)
            .amp (0, 1550, 0, 200, 60).reverb (25, 50, 1.6f, 50).delay (eighth, 45, 18, 2000).level (-9).macro (1, "Hammer")
            .mod (macro1, "noiseLevel", 30).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).macro (3, "Echo").mod (macro3, "fxDelayMix", 30)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Daybreak Spank", "Keys").osc (1, pulse, 60).osc (2, pulse, 60, 55).pitch (2, 0, 3).filter (1, svfBand, 1600, 50, 20, 100)
            .filter (2, svfHigh, 140).env (2, 0, 270, 0, 150, 83).mod (env2, "filter1Cutoff", 31).mod (velocity, "filter1Cutoff", 12)
            .amp (0, 550, 0, 200, 70).reverb (25, 30, 0.8f, 60).level (-4).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Width").mod (macro3, "osc1Position", -30).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Drowned Spank", "Keys").osc (1, pulse, 55).osc (2, analog, sawPos, 40).pitch (2, 0, 5).filter (1, ladder12, 520, 35, 10, 50)
            .filter (2, svfHigh, 270).env (2, 0, 270, 0, 150, 87).mod (env2, "filter1Cutoff", 30).mod (velocity, "filter1Cutoff", 10)
            .amp (0, 1050, 0, 180, 55).drive (hard, 35, 7100).reverb (25, 40, 1.2f, 40).level (-19).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Width").mod (macro4, "osc1Position", -30);
        add ("Drowned Squeezebox", "Keys").osc (1, pulse, 45).osc (2, analog, 80, 55).pitch (2, 12, 3).filter (1, ladder24, 620, 5, 0, 40)
            .env (2, 0, 700, 40, 250, 70).mod (env2, "filter1Cutoff", 8).mod (velocity, "filter1Cutoff", 12).lfo (1, sine, 4.5f, 0)
            .mod (lfo1, "ampLevel", 8).amp (1, 3200, 55, 200).drive (tube, 25, 6600).reverb (25, 40, 0.8f, 60).level (-20).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Tremolo").mod (macro2, "lfo1Rate", 20).macro (3, "Width").mod (macro3, "osc1Position", 30)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Entombed Pump", "Keys").osc (1, pulse, 60).osc (2, analog, 80, 50).pitch (2, -12, 4).filter (1, ladder24, 520, 10, 20, 40)
            .env (2, 0, 400, 30, 250, 70).mod (env2, "filter1Cutoff", 16).mod (velocity, "filter1Cutoff", 18).lfo (1, sine, 7, 0)
            .mod (lfo1, "ampLevel", 3).amp (6, 3200, 30, 500, 60).drive (soft, 25, 5300).reverb (16, 90, 6, 65).level (-18).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Tremolo").mod (macro2, "lfo1Rate", 20).macro (3, "Width").mod (macro3, "osc1Position", 30)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Fatal Stop", "Keys").osc (1, string, 95, 90).warp (1, wOff, 45).osc (2, string, 70, 75).warp (2, wOff, 40).pitch (2, 12, 2)
            .osc (3, pulse, 80, 45).pitch (3, 0).filter (1, svfHigh, 470, 20).filter (2, ladder12, 8600, 5).amp (0, 2600, 0, 400, 70)
            .reverb (10, 40, 1.2f, 50).level (-10).macro (1, "Bright").mod (macro1, "osc1Position", -40).macro (2, "Ring")
            .mod (macro2, "osc1WarpAmount", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Fenced Reed", "Keys").osc (1, pulse, 20).osc (2, analog, sawPos, 50).pitch (2, 0, 4).filter (1, ladder12, 1100, 5, 0, 40)
            .env (2, 0, 800, 10, 250, 70).mod (env2, "filter1Cutoff", 16).mod (velocity, "filter1Cutoff", 14).lfo (1, sine, 7, 0)
            .mod (lfo1, "ampLevel", 6).amp (6, 1900, 20, 400, 65).reverb (20, 80, 6, 70).level (-15).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Tremolo").mod (macro2, "lfo1Rate", 20).chorus (0, 0.5f, 50).macro (3, "Wide")
            .mod (macro3, "fxChorusMix", 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Flooded Spinet", "Keys").osc (1, string, 35, 90).warp (1, wOff, 60).unison (1, 3, 7, 30).osc (2, string, 25, 50).warp (2, wOff, 55)
            .pitch (2, 0, 2).filter (1, ladder24, 1500, 5, 0, 40).mod (velocity, "osc1Position", 15).mod (velocity, "filter1Cutoff", 16)
            .amp (0, 4000, 0, 550, 55).reverb (40, 80, 3.5f, 30).level (-4).macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Ring").mod (macro3, "osc1WarpAmount", 15).delay (eighthD, 40, 0, 4500).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Fugitive Spinet", "Keys").osc (1, string, 55, 90).warp (1, wOff, 65).unison (1, 3, 6, 40).osc (2, string, 30, 75).warp (2, wOff, 80)
            .pitch (2, -12, 4).filter (1, ladder24, 1600, 5, 0, 60).mod (velocity, "osc1Position", 25).mod (velocity, "filter1Cutoff", 16)
            .amp (0, 7000, 0, 700, 55).reverb (20, 85, 4, 35).level (-6).macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Ring")
            .mod (macro4, "osc1WarpAmount", 15);
        add ("Glacial Stop", "Keys").osc (1, string, 90, 90).warp (1, wOff, 30).osc (2, string, 90, 65).warp (2, wOff, 55).pitch (2, 12, 4)
            .filter (1, svfHigh, 250, 15).filter (2, svfLow, 5100, 5).amp (0, 1300, 0, 400).reverb (10, 35, 0.6f, 50).level (-9).macro (1, "Bright")
            .mod (macro1, "osc1Position", -40).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 25).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Gleaming Fork", "Keys").osc (1, analog, sinePos, 1).pitch (1, 0).osc (2, analog, sinePos).warp (2, wFm, 0).osc (3, analog, sinePos, 1)
            .pitch (3, 16, 10).env (2, 0, 700, 0, 400, 75).mod (env2, "osc2WarpAmount", 25).mod (velocity, "osc2WarpAmount", 20)
            .env (3, 0, 210, 0, 100, 85).mod (env3, "osc3Volume", 45).filter (1, ladder12, 6000, 5, 0, 30).amp (0, 2700, 0, 400, 65)
            .reverb (20, 40, 1.2f, 60).level (-9).macro (1, "Bark").mod (macro1, "osc2WarpAmount", 30).macro (2, "Tine")
            .mod (macro2, "osc3Volume", 35).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).chorus (0, 0.5f, 50)
            .macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Gleaming Plectrum", "Keys").osc (1, string, 95, 90).warp (1, wOff, 30).osc (2, string, 100, 55).warp (2, wOff, 40).pitch (2, 12)
            .osc (3, pulse, 65, 35).pitch (3, 12).filter (1, svfHigh, 200).filter (2, ladder12, 12000, 5).amp (0, 2700, 0, 200)
            .reverb (14, 95, 7.5f, 45).level (-10).macro (1, "Bright").mod (macro1, "osc1Position", -40).macro (2, "Ring")
            .mod (macro2, "osc1WarpAmount", 25).macro (3, "Thin").mod (macro3, "filter1Cutoff", 20).delay (eighthD, 40, 0, 4500).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Grim Fork", "Keys").osc (1, analog, sinePos, 1).pitch (1, 0).osc (2, analog, sinePos).warp (2, wFm, 0).osc (3, analog, sinePos, 1)
            .pitch (3, 16, 10).env (2, 0, 1400, 5, 400, 80).mod (env2, "osc2WarpAmount", 13).mod (velocity, "osc2WarpAmount", 15)
            .env (3, 0, 90, 0, 100, 85).mod (env3, "osc3Volume", 35).filter (1, svfLow, 4100, 5, 0, 30).amp (0, 2600, 0, 600).lfo (1, sine, 6.5f, 0)
            .mod (lfo1, "ampPan", 30).reverb (28, 70, 4, 35).level (-13).macro (1, "Bark").mod (macro1, "osc2WarpAmount", 30).macro (2, "Tine")
            .mod (macro2, "osc3Volume", 35).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Grim Virginal", "Keys").osc (1, string, 75, 90).warp (1, wOff, 45).osc (2, string, 85, 60).warp (2, wOff, 35).pitch (2, -12, 1)
            .osc (3, pulse, 90, 30).pitch (3, 12).filter (1, svfHigh, 190).filter (2, svfLow, 14000, 5).amp (0, 2200, 0, 300, 65)
            .reverb (20, 40, 0.6f, 40).level (-9).macro (1, "Bright").mod (macro1, "osc1Position", -40).macro (2, "Ring")
            .mod (macro2, "osc1WarpAmount", 25).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Guarded Cabinet", "Keys").osc (1, string, 45, 90).warp (1, wOff, 65).unison (1, 2, 6, 20).osc (2, string, 40, 65).warp (2, wOff, 80)
            .pitch (2, -12, 1).filter (1, ladder24, 2400, 5, 0, 60).mod (velocity, "osc1Position", 15).mod (velocity, "filter1Cutoff", 16)
            .amp (0, 7000, 0, 700, 55).chorus (30, 0.6f, 40).reverb (15, 70, 3, 35).level (-2).macro (1, "Bright").mod (macro1, "osc1Position", 30)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30)
            .drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Gutted Bauble", "Keys").osc (1, fm, 5).osc (2, analog, sinePos, 55).pitch (2, 0).noise (1, 0, 40).env (2, 0, 28, 0, 20, 90)
            .mod (env2, "noiseLevel", 30).env (3, 0, 200, 0, 200, 80).mod (env3, "osc1Position", 22).filter (1, svfLow, 4000, 5, 0, 30)
            .amp (0, 1050, 0, 300, 70).reverb (10, 55, 0.8f, 60).level (-14).macro (1, "Hammer").mod (macro1, "noiseLevel", 30).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).macro (3, "Bars").mod (macro3, "osc1Position", 30).chorus (0, 0.5f, 50).macro (4, "Wide")
            .mod (macro4, "fxChorusMix", 45);
        add ("Indigo Grand", "Keys").osc (1, string, 50, 90).warp (1, wOff, 80).unison (1, 3, 3, 30).osc (2, string, 35, 75).warp (2, wOff, 70)
            .pitch (2, 12, 3).filter (1, svfLow, 1700, 5, 0, 60).mod (velocity, "osc1Position", 15).mod (velocity, "filter1Cutoff", 20)
            .amp (0, 6500, 0, 600, 55).reverb (25, 90, 4, 70).level (-9).macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Khaki Bauble", "Keys").osc (1, fm, 5).osc (2, analog, sinePos, 70).pitch (2, 12).noise (1, 0, 40).env (2, 0, 16, 0, 20, 90)
            .mod (env2, "noiseLevel", 45).env (3, 0, 360, 0, 200, 80).mod (env3, "osc1Position", 12).filter (1, svfLow, 6800, 5, 0, 30)
            .amp (0, 1600, 0, 300, 65).reverb (19, 85, 5.5f, 50).level (-17).macro (1, "Hammer").mod (macro1, "noiseLevel", 30).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Lawless Cabinet", "Keys").osc (1, string, 50, 90).warp (1, wOff, 75).unison (1, 3, 4, 40).osc (2, string, 50, 50).warp (2, wOff, 60)
            .pitch (2, 12, 2).filter (1, svfLow, 3100, 5, 0, 40).mod (velocity, "osc1Position", 20).mod (velocity, "filter1Cutoff", 20)
            .amp (0, 5000, 0, 600, 65).chorus (25, 0.3f, 40).reverb (25, 100, 6.5f, 35).level (-7).macro (1, "Bright")
            .mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).delay (eighthD, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Ring").mod (macro4, "osc1WarpAmount", 15);
        add ("Livid Twang", "Keys").osc (1, pulse, 55).osc (2, pulse, 60, 45).pitch (2, 0, 5).filter (1, ladderBand, 790, 25, 30, 50)
            .filter (2, svfHigh, 310).env (2, 0, 80, 0, 150, 79).mod (env2, "filter1Cutoff", 19).mod (velocity, "filter1Cutoff", 8)
            .amp (0, 650, 0, 180, 70).phaser (25, 0.23f, 45).reverb (25, 40, 0.8f, 40).level (-1).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).drive (hard, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Lucid Virginal", "Keys").osc (1, string, 100, 90).warp (1, wOff, 55).osc (2, string, 100, 55).warp (2, wOff, 30).pitch (2, -12, 3)
            .filter (1, svfHigh, 360).filter (2, ladder12, 13000, 5).amp (0, 2500, 0, 300, 55).reverb (25, 30, 0.6f, 60).level (-11)
            .macro (1, "Bright").mod (macro1, "osc1Position", -40).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 25).delay (eighthD, 40, 0, 4500)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Mortal Mobile", "Keys").osc (1, metal, 40).osc (2, analog, sinePos, 75).pitch (2, 0).noise (1, 0, 40).env (2, 0, 14, 0, 20, 90)
            .mod (env2, "noiseLevel", 40).env (3, 0, 300, 0, 200, 80).mod (env3, "osc1Position", 9).filter (1, svfLow, 6800, 5, 0, 30)
            .amp (0, 1350, 0, 350, 75).reverb (15, 40, 1.6f, 40).level (-14).macro (1, "Hammer").mod (macro1, "noiseLevel", 30).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).macro (3, "Bars").mod (macro3, "osc1Position", 30).delay (eighthD, 40, 0, 4500).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Muffled Pump", "Keys").osc (1, pulse, 45).osc (2, analog, sawPos, 70).pitch (2, -12, 2).filter (1, ladder12, 520, 20, 40, 40)
            .env (2, 0, 600, 10, 250, 70).mod (env2, "filter1Cutoff", 18).mod (velocity, "filter1Cutoff", 16).lfo (1, sine, 6, 0)
            .mod (lfo1, "ampLevel", 9).amp (3, 2000, 45, 350, 60).reverb (28, 85, 3, 70).level (-16).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Tremolo").mod (macro2, "lfo1Rate", 20).macro (3, "Width").mod (macro3, "osc1Position", 30)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Perimeter Key", "Keys").osc (1, analog, sinePos, 1).pitch (1, 0).osc (2, analog, sinePos).warp (2, wFm, 5).osc (3, analog, sinePos, 1)
            .pitch (3, 16).env (2, 0, 1300, 5, 400, 70).mod (env2, "osc2WarpAmount", 25).mod (velocity, "osc2WarpAmount", 25)
            .env (3, 0, 70, 0, 100, 85).mod (env3, "osc3Volume", 35).filter (1, ladder12, 9000, 5, 0, 30).amp (0, 4200, 0, 550, 65)
            .reverb (20, 40, 0.6f, 60).level (-12).macro (1, "Bark").mod (macro1, "osc2WarpAmount", 30).macro (2, "Tine")
            .mod (macro2, "osc3Volume", 35).macro (3, "Space").mod (macro3, "fxReverbMix", 35).chorus (0, 0.5f, 50).macro (4, "Wide")
            .mod (macro4, "fxChorusMix", 45);
        add ("Phosphor Fork", "Keys").osc (1, analog, sinePos, 1).pitch (1, 12).osc (2, analog, sinePos).warp (2, wFm, 10)
            .osc (3, analog, sinePos, 1).pitch (3, 22).env (2, 0, 1400, 10, 400, 60).mod (env2, "osc2WarpAmount", 35)
            .mod (velocity, "osc2WarpAmount", 25).env (3, 0, 150, 0, 100, 85).mod (env3, "osc3Volume", 45).filter (1, ladder12, 9800, 5, 0, 30)
            .amp (0, 3000, 0, 350, 65).chorus (45, 0.9f, 50).reverb (15, 45, 1.6f, 50).level (-9).macro (1, "Bark")
            .mod (macro1, "osc2WarpAmount", 30).macro (2, "Tine").mod (macro2, "osc3Volume", 35).drive (tube, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Phosphor Reed", "Keys").osc (1, analog, sinePos, 1).pitch (1, 12).osc (2, analog, sinePos).warp (2, wFm, 0)
            .osc (3, analog, sinePos, 1).pitch (3, 19, 5).env (2, 0, 1100, 10, 400, 75).mod (env2, "osc2WarpAmount", 30)
            .mod (velocity, "osc2WarpAmount", 30).env (3, 0, 250, 0, 100, 85).mod (env3, "osc3Volume", 40).filter (1, svfLow, 8700, 5, 0, 30)
            .amp (0, 2900, 15, 600).lfo (1, sine, 4, 0).mod (lfo1, "ampPan", 25).reverb (15, 40, 0.8f, 40).level (-15).macro (1, "Bark")
            .mod (macro1, "osc2WarpAmount", 30).macro (2, "Tine").mod (macro2, "osc3Volume", 35).macro (3, "Damp")
            .mod (macro3, "filter1Cutoff", -30).chorus (0, 0.5f, 50).macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Raw Twang", "Keys").osc (1, pulse, 55).osc (2, pulse, 60, 50).pitch (2, 0, 6).filter (1, ladder12, 570, 55, 40, 100)
            .filter (2, svfHigh, 280).env (2, 0, 100, 0, 150, 83).mod (env2, "filter1Cutoff", 21).mod (velocity, "filter1Cutoff", 8)
            .amp (0, 850, 0, 140, 75).drive (hard, 40, 11000).reverb (10, 50, 1.4f, 70).level (-21).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).phaser (0, 0.2f, 55).macro (3, "Phase")
            .mod (macro3, "fxPhaserMix", 50).macro (4, "Width").mod (macro4, "osc1Position", -30);
        add ("Red Tine", "Keys").osc (1, analog, sinePos, 1).pitch (1, 12).osc (2, analog, sinePos).warp (2, wFm, 5).osc (3, analog, sinePos, 1)
            .pitch (3, 19, 10).env (2, 0, 1600, 15, 400, 70).mod (env2, "osc2WarpAmount", 14).mod (velocity, "osc2WarpAmount", 15)
            .env (3, 0, 140, 0, 100, 85).mod (env3, "osc3Volume", 55).filter (1, ladder12, 4500, 5, 0, 30).amp (0, 2800, 0, 250, 70)
            .reverb (15, 35, 1.4f, 60).level (-11).macro (1, "Bark").mod (macro1, "osc2WarpAmount", 30).macro (2, "Tine")
            .mod (macro2, "osc3Volume", 35).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).chorus (0, 0.5f, 50)
            .macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Rooftop Tine", "Keys").osc (1, analog, sinePos, 1).pitch (1, 0).osc (2, analog, sinePos).warp (2, wFm, 0).osc (3, analog, sinePos, 1)
            .pitch (3, 19, 10).env (2, 0, 900, 0, 400, 75).mod (env2, "osc2WarpAmount", 12).mod (velocity, "osc2WarpAmount", 30)
            .env (3, 0, 160, 0, 100, 85).mod (env3, "osc3Volume", 45).filter (1, ladder12, 4100, 5, 0, 30).amp (0, 2100, 0, 600)
            .phaser (35, 0.26f, 50).reverb (15, 40, 0.6f, 50).level (-9).macro (1, "Bark").mod (macro1, "osc2WarpAmount", 30).macro (2, "Tine")
            .mod (macro2, "osc3Volume", 35).macro (3, "Damp").mod (macro3, "filter1Cutoff", -30).chorus (0, 0.5f, 50).macro (4, "Wide")
            .mod (macro4, "fxChorusMix", 45);
        add ("Serrated Pluck", "Keys").osc (1, pulse, 55).osc (2, pulse, 75, 50).pitch (2, 0, 4).filter (1, svfBand, 580, 30, 30, 100)
            .filter (2, svfHigh, 130).env (2, 0, 100, 0, 150, 83).mod (env2, "filter1Cutoff", 33).mod (velocity, "filter1Cutoff", 10)
            .amp (0, 1500, 0, 120, 55).phaser (30, 0.4f, 45).reverb (15, 30, 1, 70).level (-6).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Phase").mod (macro4, "fxPhaserMix", 50);
        add ("Severed Tine", "Keys").osc (1, analog, sinePos, 1).pitch (1, 12).osc (2, analog, sinePos).warp (2, wFm, 5).osc (3, analog, sinePos, 1)
            .pitch (3, 22, 10).env (2, 0, 1200, 15, 400, 60).mod (env2, "osc2WarpAmount", 16).mod (velocity, "osc2WarpAmount", 20)
            .env (3, 0, 170, 0, 100, 85).mod (env3, "osc3Volume", 55).filter (1, ladder12, 7400, 5, 0, 30).amp (0, 1600, 0, 600, 70)
            .lfo (1, sine, 4, 0).mod (lfo1, "ampPan", 30).reverb (10, 30, 0.6f, 50).level (-13).macro (1, "Bark").mod (macro1, "osc2WarpAmount", 30)
            .macro (2, "Tine").mod (macro2, "osc3Volume", 35).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 45)
            .macro (4, "Damp").mod (macro4, "filter1Cutoff", -30);
        add ("Slanted Spank", "Keys").osc (1, pulse, 85).osc (2, pulse, 66.67f, 45).pitch (2, 12, 3).filter (1, ladder12, 640, 55, 10, 50)
            .filter (2, svfHigh, 370).env (2, 0, 180, 0, 150, 83).mod (env2, "filter1Cutoff", 33).mod (velocity, "filter1Cutoff", 12)
            .amp (0, 1000, 0, 200, 55).phaser (35, 0.21f, 60).reverb (20, 55, 1.2f, 50).level (-5).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Phase").mod (macro4, "fxPhaserMix", 50);
        add ("Sleepless Wheeze", "Keys").osc (1, pulse, 55).osc (2, analog, sawPos, 55).pitch (2, 12, 2).filter (1, svfLow, 920, 10, 40, 60)
            .env (2, 0, 550, 20, 250, 70).mod (env2, "filter1Cutoff", 18).mod (velocity, "filter1Cutoff", 10).lfo (1, sine, 6.5f, 0)
            .mod (lfo1, "ampLevel", 8).amp (6, 3500, 25, 450).reverb (25, 30, 1.2f, 60).level (-15).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Tremolo").mod (macro2, "lfo1Rate", 20).drive (tube, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Width").mod (macro4, "osc1Position", 30);
        add ("Southern Grand", "Keys").osc (1, string, 40, 90).warp (1, wOff, 65).unison (1, 2, 3, 60).osc (2, string, 45, 65).warp (2, wOff, 65)
            .pitch (2, 12, 1).filter (1, ladder24, 1000, 5, 0, 40).mod (velocity, "osc1Position", 20).mod (velocity, "filter1Cutoff", 18)
            .amp (0, 3000, 0, 350).reverb (25, 75, 9, 50).level (-3).macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Ring").mod (macro3, "osc1WarpAmount", 15).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Steel Trinket", "Keys").osc (1, metal, 10).osc (2, analog, sinePos, 75).pitch (2, 12).noise (1, 0, 40).env (2, 0, 16, 0, 20, 90)
            .mod (env2, "noiseLevel", 35).env (3, 0, 100, 0, 200, 80).mod (env3, "osc1Position", 19).filter (1, svfLow, 5200, 5, 0, 30)
            .amp (0, 1300, 0, 250, 55).reverb (25, 50, 0.8f, 40).level (-14).macro (1, "Hammer").mod (macro1, "noiseLevel", 30).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Bars")
            .mod (macro4, "osc1Position", 30);
        add ("Sullen Prong", "Keys").osc (1, analog, sinePos, 1).pitch (1, 0).osc (2, analog, sinePos).warp (2, wFm, 10).osc (3, analog, sinePos, 1)
            .pitch (3, 22).env (2, 0, 900, 0, 400, 80).mod (env2, "osc2WarpAmount", 30).mod (velocity, "osc2WarpAmount", 30)
            .env (3, 0, 190, 0, 100, 85).mod (env3, "osc3Volume", 40).filter (1, ladder12, 3900, 5, 0, 30).amp (0, 1800, 0, 350)
            .chorus (25, 0.9f, 40).reverb (20, 30, 0.6f, 40).level (-11).macro (1, "Bark").mod (macro1, "osc2WarpAmount", 30).macro (2, "Tine")
            .mod (macro2, "osc3Volume", 35).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Damp").mod (macro4, "filter1Cutoff", -30);
        add ("Tireless Salon", "Keys").osc (1, string, 65, 90).warp (1, wOff, 55).unison (1, 3, 7, 50).osc (2, string, 25, 50).warp (2, wOff, 55)
            .pitch (2, -12, 2).filter (1, svfLow, 970, 5, 0, 40).mod (velocity, "osc1Position", 30).mod (velocity, "filter1Cutoff", 10)
            .amp (0, 6000, 0, 500, 60).chorus (15, 0.6f, 40).reverb (35, 90, 8.5f, 45).level (-1).macro (1, "Bright")
            .mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Ring")
            .mod (macro3, "osc1WarpAmount", 15).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Trapped Squeezebox", "Keys").osc (1, pulse, 20).osc (2, analog, trianglePos, 65).pitch (2, -12, 6)
            .filter (1, ladder12, 1500, 10, 20, 60).env (2, 0, 400, 40, 250, 70).mod (env2, "filter1Cutoff", 8).mod (velocity, "filter1Cutoff", 12)
            .lfo (1, sine, 7, 0).mod (lfo1, "ampLevel", 6).amp (1, 3300, 50, 300, 65).drive (soft, 25, 3600).reverb (16, 95, 7, 40).level (-20)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Tremolo").mod (macro2, "lfo1Rate", 20).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Width").mod (macro4, "osc1Position", 30);
        add ("Vagrant Cottage", "Keys").osc (1, string, 50, 90).warp (1, wOff, 55).unison (1, 2, 7, 40).osc (2, string, 25, 75).warp (2, wOff, 70)
            .pitch (2, 12).filter (1, svfLow, 3900, 5, 0, 60).mod (velocity, "osc1Position", 20).mod (velocity, "filter1Cutoff", 16)
            .amp (0, 7000, 0, 400, 65).reverb (15, 70, 4.5f, 40).level (-11).macro (1, "Bright").mod (macro1, "osc1Position", 30)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30)
            .macro (4, "Ring").mod (macro4, "osc1WarpAmount", 15);
        add ("Vigilant Spank", "Keys").osc (1, pulse, 55).osc (2, analog, sawPos, 55).pitch (2, 0, 5).filter (1, svfBand, 1500, 55, 20, 70)
            .filter (2, svfHigh, 380).env (2, 0, 100, 0, 150, 87).mod (env2, "filter1Cutoff", 24).mod (velocity, "filter1Cutoff", 10)
            .amp (0, 1050, 0, 80, 60).drive (tube, 30, 6300).reverb (25, 35, 1, 70).level (-11).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).phaser (0, 0.2f, 55).macro (4, "Phase").mod (macro4, "fxPhaserMix", 50);
        add ("Vigilant Toy", "Keys").osc (1, metal, 45).osc (2, analog, sinePos, 60).pitch (2, 24).noise (1, 0, 40).env (2, 0, 28, 0, 20, 90)
            .mod (env2, "noiseLevel", 30).env (3, 0, 260, 0, 200, 80).mod (env3, "osc1Position", 12).filter (1, ladder12, 4400, 5, 0, 30)
            .amp (0, 1600, 0, 300, 70).reverb (14, 90, 5.5f, 60).delay (sixteenthD, 35, 10, 7500).level (-9).macro (1, "Hammer")
            .mod (macro1, "noiseLevel", 30).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).macro (3, "Echo").mod (macro3, "fxDelayMix", 30)
            .macro (4, "Bars").mod (macro4, "osc1Position", 30);

        // Organ, by name.
        add ("Adrift Chapel", "Organ").osc (1, additive, 10).osc (2, additive, 20, 65).pitch (2, 12).osc (3, analog, 40, 50).pitch (3, -12)
            .noise (1, 1, 20).env (2, 30, 100, 0, 80, 70).mod (env2, "noiseLevel", 45).filter (1, svfLow, 4000, 5).amp (65, 300, 100, 400)
            .reverb (50, 95, 6.5f, 30).level (-17).macro (1, "Ranks").mod (macro1, "osc1Position", 30).macro (2, "Breath")
            .mod (macro2, "noiseLevel", 30).macro (3, "Mixture").mod (macro3, "osc3Volume", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Arctic Swell", "Organ").osc (1, additive, 5).pitch (1, -12).osc (2, additive, 5, 75).pitch (2, 0).osc (3, additive, 0, 40)
            .pitch (3, 19).env (2, 0, 160, 0, 100, 80).mod (env2, "osc3Volume", 35).filter (1, ladder12, 3000, 5).amp (4, 300, 100, 40)
            .chorus (45, 4.5f, 40).reverb (15, 90, 5, 30).level (-12).macro (1, "Drawbars").mod (macro1, "osc1Position", 30).macro (2, "Percussion")
            .mod (macro2, "osc3Volume", 35).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Rotor")
            .mod (macro4, "fxChorusMix", 40);
        add ("Asphalt Absolution", "Organ").osc (1, additive, 45).osc (2, additive, 30, 75).pitch (2, 7).osc (3, growl, 20, 45).pitch (3, 0, 6)
            .filter (1, ladder24, 4900, 25, 60).amp (10, 300, 100, 200).drive (fuzz, 65, 4700).chorus (30, 5, 40).reverb (30, 85, 3, 35).level (-36)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Resonance").mod (macro2, "filter1Resonance", 35).macro (3, "Fuzz")
            .mod (macro3, "fxDriveAmount", 30).macro (4, "Bite").mod (macro4, "filter1Drive", 40);
        add ("Basement Lung", "Organ").osc (1, growl, 15).osc (2, additive, 15, 60).pitch (2, -12, 6).unison (1, 2, 12, 60)
            .filter (1, ladder24, 3700, 5).lfo (1, sine, 1.4f, 0).mod (lfo1, "ampLevel", 6).mod (lfo1, "filter1Cutoff", 2).amp (90, 400, 100, 350)
            .reverb (10, 55, 1.4f, 60).level (-18).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Reeds")
            .mod (macro2, "osc1Position", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Bent Transept", "Organ").osc (1, additive, 5).osc (2, additive, 0, 75).pitch (2, 12).osc (3, analog, 40, 35).pitch (3, 24)
            .noise (1, 1, 20).env (2, 20, 250, 0, 80, 70).mod (env2, "noiseLevel", 20).filter (1, svfLow, 3900, 5).amp (70, 300, 100, 350)
            .reverb (40, 95, 8, 40).level (-18).macro (1, "Ranks").mod (macro1, "osc1Position", 30).macro (2, "Breath")
            .mod (macro2, "noiseLevel", 30).macro (3, "Mixture").mod (macro3, "osc3Volume", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Bleak Portable", "Organ").osc (1, analog, squarePos).osc (2, analog, squarePos, 50).pitch (2, 7).lfo (1, sine, 6.25f, 0)
            .mod (lfo1, "osc1Fine", 6).mod (lfo1, "osc2Fine", 4).filter (1, svfBand, 1800, 25).filter (2, svfHigh, 180).amp (2, 300, 100, 60)
            .reverb (20, 70, 6, 45).level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Octave")
            .mod (macro2, "osc2Volume", -45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Caustic Penance", "Organ").osc (1, additive, 35).osc (2, additive, 50, 65).pitch (2, 12).osc (3, growl, 70, 55).pitch (3, -12, 9)
            .filter (1, svfLow, 3900, 25, 30).amp (3, 300, 100, 180).drive (fuzz, 40, 5600).chorus (40, 5, 40).reverb (25, 70, 8.5f, 60).level (-31)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Resonance").mod (macro2, "filter1Resonance", 35).macro (3, "Bite")
            .mod (macro3, "filter1Drive", 40).macro (4, "Rotor").mod (macro4, "fxChorusMix", 40);
        add ("Cellar Combo", "Organ").osc (1, pulse, 55).osc (2, pulse, 35, 70).pitch (2, 12).lfo (1, sine, 6, 0).mod (lfo1, "osc1Fine", 12)
            .mod (lfo1, "osc2Fine", 8).filter (1, ladder12, 4400, 20).filter (2, svfHigh, 210).amp (1, 300, 100, 30).drive (tube, 30, 4400)
            .reverb (25, 95, 5, 50).level (-20).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Octave")
            .mod (macro2, "osc2Volume", -45).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).chorus (0, 0.5f, 50).macro (4, "Wide")
            .mod (macro4, "fxChorusMix", 45);
        add ("Cornered Transistor", "Organ").osc (1, pulse, 15).osc (2, pulse, 35, 65).pitch (2, 12).lfo (1, sine, 7.25f, 0)
            .mod (lfo1, "osc1Fine", 9).mod (lfo1, "osc2Fine", 11).filter (1, svfBand, 2900, 25).filter (2, svfHigh, 190).amp (1, 300, 100, 60)
            .reverb (10, 55, 0.8f, 70).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Octave")
            .mod (macro2, "osc2Volume", -45).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 45).drive (tube, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Crooked Tonewheel", "Organ").osc (1, additive, 25).pitch (1, 0).osc (2, additive, 20, 75).pitch (2, 12).osc (3, additive, 5, 40)
            .pitch (3, 19).env (2, 0, 120, 0, 100, 80).mod (env2, "osc3Volume", 35).filter (1, ladder12, 3100, 5).amp (2, 300, 100, 80)
            .chorus (35, 0.7f, 30).drive (soft, 15, 4900).reverb (15, 45, 0.8f, 60).level (-14).macro (1, "Drawbars")
            .mod (macro1, "osc1Position", 30).macro (2, "Percussion").mod (macro2, "osc3Volume", 35).macro (3, "Rotor")
            .mod (macro3, "fxChorusMix", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Crushed Heresy", "Organ").osc (1, additive, 20).osc (2, additive, 25, 60).pitch (2, 12).osc (3, growl, 70, 50).pitch (3, -12, 6)
            .filter (1, ladder24, 1300, 25, 40).amp (1, 300, 100, 100).drive (fuzz, 40, 3300).chorus (35, 5, 25).reverb (35, 100, 3.5f, 45)
            .level (-30).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Resonance").mod (macro2, "filter1Resonance", 35)
            .macro (3, "Fuzz").mod (macro3, "fxDriveAmount", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Dreamless Penance", "Organ").osc (1, additive, 40).osc (2, additive, 15, 75).pitch (2, -12).osc (3, growl, 20, 35).pitch (3, 19, 4)
            .filter (1, ladder12, 4500, 10, 30).amp (3, 300, 100, 180).drive (tube, 65, 3600).chorus (50, 4, 40).reverb (25, 95, 7.5f, 45)
            .level (-27).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Resonance").mod (macro2, "filter1Resonance", 35)
            .macro (3, "Rotor").mod (macro3, "fxChorusMix", 40).macro (4, "Fuzz").mod (macro4, "fxDriveAmount", 30);
        add ("Fearless Squeeze", "Organ").osc (1, growl, 25).osc (2, additive, 10, 70).pitch (2, 0, 8).unison (1, 2, 5, 70).filter (1, svfLow, 2500)
            .lfo (1, drift, 0.46f, 0).mod (lfo1, "ampLevel", 8).mod (lfo1, "filter1Cutoff", 7).amp (330, 400, 100, 350).reverb (30, 80, 3, 70)
            .level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Reeds").mod (macro2, "osc1Position", 30)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Fearless Vespers", "Organ").osc (1, additive, 55).osc (2, additive, 10, 65).pitch (2, -12).osc (3, analog, 60, 45).pitch (3, -12, 7)
            .filter (1, ladder24, 3000, 25, 20).amp (10, 300, 100, 200).drive (hard, 55, 4200).chorus (35, 0.6f, 30).reverb (30, 100, 9, 60)
            .level (-26).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Resonance").mod (macro2, "filter1Resonance", 35)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Rotor").mod (macro4, "fxChorusMix", 40);
        add ("Felt Wheeze", "Organ").osc (1, sync, 40).osc (2, additive, 25, 60).pitch (2, 12, 4).unison (1, 2, 10, 50)
            .filter (1, ladder12, 1400, 25).lfo (1, sine, 2.4f, 0).mod (lfo1, "ampLevel", 7).mod (lfo1, "filter1Cutoff", 5).amp (170, 400, 100, 250)
            .reverb (25, 30, 1, 50).level (-17).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Reeds")
            .mod (macro2, "osc1Position", 30).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).macro (4, "Pump")
            .mod (macro4, "lfo1Rate", 20);
        add ("Fogbound Cloister", "Organ").osc (1, additive, 5).osc (2, additive, 0).pitch (2, -12).osc (3, additive, 30, 45).pitch (3, 7)
            .noise (1, 1, 20).env (2, 10, 120, 0, 80, 70).mod (env2, "noiseLevel", 35).filter (1, svfLow, 5100, 5).amp (100, 300, 100, 450)
            .reverb (40, 100, 7, 45).level (-18).macro (1, "Ranks").mod (macro1, "osc1Position", 30).macro (2, "Breath")
            .mod (macro2, "noiseLevel", 30).macro (3, "Mixture").mod (macro3, "osc3Volume", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Gnarled Benediction", "Organ").osc (1, additive, 35).osc (2, additive, 30, 65).pitch (2, 12).osc (3, analog, 40, 55).pitch (3, 19, 3)
            .filter (1, svfLow, 1700, 20, 20).amp (1, 300, 100, 120).drive (foldDrive, 35, 4000).chorus (30, 4, 45).reverb (35, 80, 8.5f, 70)
            .level (-23).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Resonance").mod (macro2, "filter1Resonance", 35)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Rotor").mod (macro4, "fxChorusMix", 40);
        add ("Godless Tonewheel", "Organ").osc (1, additive, 10).pitch (1, 0).osc (2, additive, 5, 65).pitch (2, 7).osc (3, additive, 0, 40)
            .pitch (3, 12).env (2, 0, 160, 0, 100, 80).mod (env2, "osc3Volume", 40).filter (1, ladder12, 6200, 5).amp (1, 300, 100, 50)
            .chorus (45, 5, 40).reverb (10, 50, 0.8f, 50).level (-10).macro (1, "Drawbars").mod (macro1, "osc1Position", 30).macro (2, "Percussion")
            .mod (macro2, "osc3Volume", 35).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Rotor")
            .mod (macro4, "fxChorusMix", 40);
        add ("Gutted Harmonium", "Organ").osc (1, growl, 20).osc (2, additive, 30, 75).pitch (2, -12, 8).unison (1, 2, 8, 50)
            .filter (1, ladder12, 1800, 5).lfo (1, sine, 2.1f, 0).mod (lfo1, "ampLevel", 9).mod (lfo1, "filter1Cutoff", 5).amp (230, 400, 100, 200)
            .reverb (10, 50, 1, 40).level (-18).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Reeds")
            .mod (macro2, "osc1Position", 30).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Haunted Vestry", "Organ").osc (1, additive, 5).osc (2, additive, 10).pitch (2, 12).osc (3, additive, 0, 50).pitch (3, 7)
            .noise (1, 1, 20).env (2, 15, 210, 0, 80, 70).mod (env2, "noiseLevel", 35).filter (1, svfLow, 2800, 5).amp (75, 300, 100, 350)
            .reverb (45, 95, 4.5f, 40).level (-18).macro (1, "Ranks").mod (macro1, "osc1Position", 30).macro (2, "Breath")
            .mod (macro2, "noiseLevel", 30).macro (3, "Mixture").mod (macro3, "osc3Volume", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Infected Cabinet", "Organ").osc (1, additive, 5).pitch (1, -12).osc (2, additive, 0, 55).pitch (2, 12).osc (3, additive, 5, 45)
            .pitch (3, 24).env (2, 0, 240, 0, 100, 80).mod (env2, "osc3Volume", 30).filter (1, svfLow, 5400, 5).amp (1, 300, 100, 110)
            .chorus (50, 5, 40).drive (soft, 30, 5900).reverb (20, 80, 8, 70).level (-21).macro (1, "Drawbars").mod (macro1, "osc1Position", 30)
            .macro (2, "Percussion").mod (macro2, "osc3Volume", 35).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Merciless Stop", "Organ").osc (1, additive, 0).pitch (1, -12).osc (2, additive, 0).pitch (2, 12).osc (3, additive, 10, 30)
            .pitch (3, 24).env (2, 0, 120, 0, 100, 80).mod (env2, "osc3Volume", 30).filter (1, ladder12, 2800, 5).amp (1, 300, 100, 90)
            .chorus (35, 5, 35).drive (soft, 25, 5400).reverb (30, 80, 6, 40).level (-22).macro (1, "Drawbars").mod (macro1, "osc1Position", 30)
            .macro (2, "Percussion").mod (macro2, "osc3Volume", 35).macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 20).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Misted Transept", "Organ").osc (1, additive, 40).osc (2, additive, 25).pitch (2, 12).osc (3, analog, 40, 50).pitch (3, -12)
            .noise (1, 1, 40).env (2, 5, 230, 0, 80, 70).mod (env2, "noiseLevel", 35).filter (1, ladder12, 6900, 5).amp (120, 300, 100, 400)
            .reverb (40, 85, 7.5f, 40).level (-17).macro (1, "Ranks").mod (macro1, "osc1Position", 30).macro (2, "Breath")
            .mod (macro2, "noiseLevel", 30).macro (3, "Mixture").mod (macro3, "osc3Volume", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Misted Vestry", "Organ").osc (1, additive, 10).osc (2, additive, 10, 60).pitch (2, 12).osc (3, additive, 0, 50).pitch (3, -12)
            .noise (1, 1, 60).env (2, 15, 240, 0, 80, 70).mod (env2, "noiseLevel", 40).filter (1, ladder12, 3400, 5).amp (110, 300, 100, 200)
            .reverb (35, 85, 6, 40).level (-15).macro (1, "Ranks").mod (macro1, "osc1Position", 30).macro (2, "Breath")
            .mod (macro2, "noiseLevel", 30).macro (3, "Mixture").mod (macro3, "osc3Volume", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Mortal Cabinet", "Organ").osc (1, additive, 15).pitch (1, -12).osc (2, additive, 5).pitch (2, 12).osc (3, additive, 0, 30)
            .pitch (3, 24).env (2, 0, 260, 0, 100, 80).mod (env2, "osc3Volume", 30).filter (1, svfLow, 2500, 5).amp (1, 300, 100, 110)
            .chorus (45, 5, 45).drive (soft, 35, 4400).reverb (15, 45, 1.4f, 70).level (-24).macro (1, "Drawbars").mod (macro1, "osc1Position", 30)
            .macro (2, "Percussion").mod (macro2, "osc3Volume", 35).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Cutoff")
            .mod (macro4, "filter1Cutoff", 20);
        add ("Nuclear Vespers", "Organ").osc (1, additive, 15).osc (2, additive, 30, 65).pitch (2, 7).osc (3, analog, 20, 55).pitch (3, 19, 4)
            .filter (1, ladder24, 2800, 20, 40).amp (10, 300, 100, 100).drive (hard, 65, 3600).chorus (30, 4, 20).reverb (35, 85, 3, 40).level (-29)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Resonance").mod (macro2, "filter1Resonance", 35).macro (3, "Rotor")
            .mod (macro3, "fxChorusMix", 40).macro (4, "Fuzz").mod (macro4, "fxDriveAmount", 30);
        add ("Numb Rig", "Organ").osc (1, analog, squarePos).osc (2, analog, squarePos, 75).pitch (2, 12).lfo (1, sine, 5.5f, 0)
            .mod (lfo1, "osc1Fine", 4).mod (lfo1, "osc2Fine", 4).filter (1, svfBand, 4800, 20).filter (2, svfHigh, 310).amp (2, 300, 100, 60)
            .drive (hard, 40, 5900).reverb (30, 75, 4, 70).level (-21).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Octave")
            .mod (macro2, "osc2Volume", -45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).chorus (0, 0.5f, 50).macro (4, "Wide")
            .mod (macro4, "fxChorusMix", 45);
        add ("Phosphor Swell", "Organ").osc (1, additive, 5).pitch (1, -12).osc (2, additive, 15, 70).pitch (2, 12).osc (3, additive, 5, 50)
            .pitch (3, 24).env (2, 0, 400, 0, 100, 80).mod (env2, "osc3Volume", 20).filter (1, ladder12, 5200, 5).amp (1, 300, 100, 40)
            .chorus (30, 0.8f, 25).reverb (25, 45, 1.6f, 40).level (-12).macro (1, "Drawbars").mod (macro1, "osc1Position", 30)
            .macro (2, "Percussion").mod (macro2, "osc3Volume", 35).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Rotor").mod (macro4, "fxChorusMix", 40);
        add ("Platinum Loft", "Organ").osc (1, additive, 25).osc (2, additive, 5, 75).pitch (2, 12).osc (3, analog, 40, 35).pitch (3, 24)
            .noise (1, 1, 60).env (2, 30, 60, 0, 80, 70).mod (env2, "noiseLevel", 30).filter (1, ladder12, 5400, 5).amp (115, 300, 100, 250)
            .reverb (35, 85, 4.5f, 50).level (-14).macro (1, "Ranks").mod (macro1, "osc1Position", 30).macro (2, "Breath")
            .mod (macro2, "noiseLevel", 30).macro (3, "Mixture").mod (macro3, "osc3Volume", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Ragged Confession", "Organ").osc (1, additive, 45).osc (2, additive, 35, 60).pitch (2, 12).osc (3, growl, 30, 55).pitch (3, 19, 3)
            .filter (1, ladder12, 2400, 35, 40).amp (10, 300, 100, 100).drive (fuzz, 55, 5500).chorus (35, 4, 25).reverb (35, 90, 3, 45).level (-36)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Resonance").mod (macro2, "filter1Resonance", 35).macro (3, "Fuzz")
            .mod (macro3, "fxDriveAmount", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Reckless Tour", "Organ").osc (1, pulse, 65).osc (2, pulse, 45, 70).pitch (2, 12).lfo (1, sine, 6.5f, 0).mod (lfo1, "osc1Fine", 5)
            .mod (lfo1, "osc2Fine", 6).filter (1, svfLow, 1800, 20).filter (2, svfHigh, 190).amp (1, 300, 100, 80).reverb (20, 85, 7.5f, 50)
            .level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Octave").mod (macro2, "osc2Volume", -45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).chorus (0, 0.5f, 50).macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Secret Combo", "Organ").osc (1, pulse, 30).osc (2, analog, squarePos, 70).pitch (2, 24).lfo (1, sine, 6.25f, 0)
            .mod (lfo1, "osc1Fine", 8).mod (lfo1, "osc2Fine", 8).filter (1, svfBand, 6000, 25).filter (2, svfHigh, 150).amp (2, 300, 100, 40)
            .drive (tube, 20, 7200).reverb (15, 70, 6.5f, 45).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Octave")
            .mod (macro2, "osc2Volume", -45).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).chorus (0, 0.5f, 50).macro (4, "Wide")
            .mod (macro4, "fxChorusMix", 45);
        add ("Septic Bellows", "Organ").osc (1, sync, 15).osc (2, additive, 15, 70).pitch (2, 12, 8).unison (1, 2, 8, 50).filter (1, svfLow, 2600)
            .lfo (1, drift, 0.96f, 0).mod (lfo1, "ampLevel", 9).mod (lfo1, "filter1Cutoff", 7).amp (260, 400, 100, 450).reverb (15, 70, 8, 70)
            .level (-16).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Reeds").mod (macro2, "osc1Position", 30)
            .macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Slanted Vespers", "Organ").osc (1, additive, 15).osc (2, additive, 45, 65).pitch (2, -12).osc (3, sync, 70, 45).pitch (3, -12, 3)
            .filter (1, ladder12, 3100, 20, 30).amp (10, 300, 100, 100).drive (hard, 65, 4000).chorus (40, 0.6f, 30).reverb (30, 85, 4, 30)
            .level (-29).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Resonance").mod (macro2, "filter1Resonance", 35)
            .macro (3, "Rotor").mod (macro3, "fxChorusMix", 40).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Soulless Stage", "Organ").osc (1, analog, 90).osc (2, analog, squarePos, 60).pitch (2, 24).lfo (1, sine, 7.25f, 0)
            .mod (lfo1, "osc1Fine", 10).mod (lfo1, "osc2Fine", 11).filter (1, svfBand, 4000, 15).filter (2, svfHigh, 340).amp (2, 300, 100, 60)
            .reverb (20, 40, 0.8f, 60).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Octave")
            .mod (macro2, "osc2Volume", -45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).chorus (0, 0.5f, 50).macro (4, "Wide")
            .mod (macro4, "fxChorusMix", 45);
        add ("Sprawling Chapel", "Organ").osc (1, additive, 20).osc (2, additive, 5, 65).pitch (2, 12).osc (3, pulse, 20, 55).pitch (3, 7)
            .noise (1, 1, 20).env (2, 30, 230, 0, 80, 70).mod (env2, "noiseLevel", 30).filter (1, svfLow, 4800, 5).amp (105, 300, 100, 200)
            .reverb (35, 85, 7.5f, 50).level (-16).macro (1, "Ranks").mod (macro1, "osc1Position", 30).macro (2, "Breath")
            .mod (macro2, "noiseLevel", 30).macro (3, "Mixture").mod (macro3, "osc3Volume", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Stern Stop", "Organ").osc (1, additive, 5).pitch (1, -12).osc (2, additive, 0, 60).pitch (2, 0).osc (3, additive, 5, 50).pitch (3, 19)
            .env (2, 0, 400, 0, 100, 80).mod (env2, "osc3Volume", 30).filter (1, svfLow, 3800, 5).amp (4, 300, 100, 110).chorus (55, 0.7f, 40)
            .reverb (20, 70, 5.5f, 45).level (-14).macro (1, "Drawbars").mod (macro1, "osc1Position", 30).macro (2, "Percussion")
            .mod (macro2, "osc3Volume", 35).macro (3, "Rotor").mod (macro3, "fxChorusMix", 40).macro (4, "Cutoff").mod (macro4, "filter1Cutoff", 20);
        add ("Sullen Squeeze", "Organ").osc (1, pulse, 15).osc (2, additive, 10, 70).pitch (2, 0, 8).unison (1, 2, 6, 50)
            .filter (1, svfLow, 2300, 25).lfo (1, sine, 0.45f, 0).mod (lfo1, "ampLevel", 9).mod (lfo1, "filter1Cutoff", 5).amp (60, 400, 100, 500)
            .reverb (15, 40, 1, 70).level (-21).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Reeds")
            .mod (macro2, "osc1Position", 30).macro (3, "Pump").mod (macro3, "lfo1Rate", 20).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Surgical Combo", "Organ").osc (1, analog, squarePos).osc (2, pulse, 0, 60).pitch (2, 24).lfo (1, sine, 7, 0).mod (lfo1, "osc1Fine", 7)
            .mod (lfo1, "osc2Fine", 11).filter (1, svfLow, 3100, 15).filter (2, svfHigh, 330).amp (2, 300, 100, 40).drive (hard, 40, 5600)
            .reverb (20, 35, 1.2f, 60).level (-26).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Octave")
            .mod (macro2, "osc2Volume", -45).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).chorus (0, 0.5f, 50).macro (4, "Wide")
            .mod (macro4, "fxChorusMix", 45);
        add ("Vaulted Nave", "Organ").osc (1, additive, 15).osc (2, additive, 0, 65).pitch (2, 12).osc (3, pulse, 0, 35).pitch (3, -12).noise (1, 1)
            .env (2, 20, 60, 0, 80, 70).mod (env2, "noiseLevel", 40).filter (1, ladder12, 1800, 5).amp (70, 300, 100, 500).reverb (35, 95, 5.5f, 55)
            .level (-15).macro (1, "Ranks").mod (macro1, "osc1Position", 30).macro (2, "Breath").mod (macro2, "noiseLevel", 30).macro (3, "Mixture")
            .mod (macro3, "osc3Volume", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Velvet Treadle", "Organ").osc (1, pulse, 30).osc (2, additive, 20, 60).pitch (2, 12, 4).unison (1, 2, 5, 70)
            .filter (1, ladder24, 1800, 20).lfo (1, sine, 1.3f, 0).mod (lfo1, "ampLevel", 9).mod (lfo1, "filter1Cutoff", 6).amp (220, 400, 100, 250)
            .reverb (20, 30, 1.4f, 50).level (-17).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Reeds")
            .mod (macro2, "osc1Position", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Pump").mod (macro4, "lfo1Rate", 20);
        add ("Wary Harmonium", "Organ").osc (1, pulse, 30).osc (2, additive, 35).pitch (2, -12, 7).unison (1, 2, 7, 30)
            .filter (1, ladder12, 1800, 30).lfo (1, sine, 0.8f, 0).mod (lfo1, "ampLevel", 9).mod (lfo1, "filter1Cutoff", 4).amp (250, 400, 100, 550)
            .reverb (25, 95, 3.5f, 50).level (-18).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Reeds")
            .mod (macro2, "osc1Position", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("White Cabinet", "Organ").osc (1, additive, 0).pitch (1, -12).osc (2, additive, 20).pitch (2, 12).osc (3, additive, 10, 30)
            .pitch (3, 24).env (2, 0, 240, 0, 100, 80).mod (env2, "osc3Volume", 30).filter (1, svfLow, 4700, 5).amp (1, 300, 100, 90)
            .chorus (30, 0.8f, 25).reverb (15, 50, 1, 40).level (-17).macro (1, "Drawbars").mod (macro1, "osc1Position", 30).macro (2, "Percussion")
            .mod (macro2, "osc3Volume", 35).macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 20).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Wordless Console", "Organ").osc (1, pulse, 20).osc (2, analog, squarePos, 50).pitch (2, 12).lfo (1, sine, 7.5f, 0)
            .mod (lfo1, "osc1Fine", 4).mod (lfo1, "osc2Fine", 5).filter (1, svfLow, 4100, 5).filter (2, svfHigh, 210).amp (1, 300, 100, 30)
            .reverb (10, 50, 0.8f, 40).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Octave")
            .mod (macro2, "osc2Volume", -45).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 45).drive (tube, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);

        // Brass, by name.
        add ("Abrasive Bray", "Brass").osc (1, sync, 10).unison (1, 4, 12, 50).osc (2, analog, sawPos, 65).pitch (2, -12, 8)
            .env (2, 30, 750, 40, 250, 55).mod (env2, "osc1Position", 40).mod (env2, "filter1Cutoff", 20).filter (1, ladder24, 1400, 10, 40, 40)
            .amp (30, 400, 85, 250).drive (hard, 20, 5800).reverb (15, 50, 1, 50).level (-16).macro (1, "Sync").mod (macro1, "osc1Position", 30)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Abyssal Guard", "Brass").osc (1, analog, sawPos).unison (1, 3, 14, 60).osc (2, analog, sawPos, 60).pitch (2, 12, 4)
            .filter (1, ladder24, 360, 10, 20, 50).env (2, 130, 750, 55, 250).mod (env2, "filter1Cutoff", 38).mod (velocity, "filter1Cutoff", 6)
            .amp (45, 400, 95, 400).lfo (1, sine, 5.5f).lfoFade (1, 400).mod (lfo1, "osc1Fine", 3).reverb (25, 90, 9, 70).level (-12)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Blare").mod (macro2, "filter1Resonance", 35).macro (3, "Bite")
            .mod (macro3, "filter1Drive", 50).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Acrid Herald", "Brass").osc (1, fm, 20).unison (1, 2, 10, 60).osc (2, analog, sawPos, 40).pitch (2, -12, 5).env (2, 100, 800, 55, 250)
            .mod (env2, "osc1Position", 45).mod (velocity, "osc1Position", 18).filter (1, svfLow, 2700).amp (50, 400, 80, 150).lfo (1, sine, 5.5f)
            .lfoFade (1, 500).mod (lfo1, "osc1Fine", 5).reverb (25, 90, 4.5f, 50).level (-14).macro (1, "Index").mod (macro1, "osc1Position", 30)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Atomic Standard", "Brass").osc (1, analog, sawPos).unison (1, 2, 14, 70).osc (2, analog, sawPos).pitch (2, 7, 5).unison (2, 2, 12)
            .osc (3, analog, squarePos, 45).pitch (3, 12, 5).filter (1, ladder24, 300, 15, 40, 40).env (2, 30, 900, 40, 250, 45)
            .mod (env2, "filter1Cutoff", 32).mod (velocity, "filter1Cutoff", 6).amp (45, 400, 85, 500).reverb (20, 80, 5.5f, 55).level (-11)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Fifth").mod (macro2, "osc2Volume", -50).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Banished Call", "Brass").osc (1, analog, 42).unison (1, 3, 8, 40).osc (2, analog, 55, 55).pitch (2, -12, 7)
            .filter (1, ladder12, 420, 15, 0, 50).env (2, 240, 550, 55, 250).mod (env2, "filter1Cutoff", 23).mod (velocity, "filter1Cutoff", 12)
            .amp (100, 500, 90, 300).reverb (35, 90, 6, 70).level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Brass")
            .mod (macro2, "osc1Position", 30).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Bent Bucket", "Brass").osc (1, pulse, 90).osc (2, analog, sawPos, 45).pitch (2, 0, 3).filter (1, svfBand, 730, 55, 0, 50)
            .env (2, 65, 250, 60, 200, 55).mod (env2, "filter1Cutoff", 10).mod (velocity, "filter1Cutoff", 6).amp (32, 400, 90, 240).mono (30)
            .lfo (1, sine, 6).lfoFade (1, 550).mod (lfo1, "osc1Fine", 5).mod (lfo1, "osc2Fine", 5).reverb (15, 85, 6, 65).level (-1)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Pinch").mod (macro2, "filter1Resonance", 25).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Bitten Klaxon", "Brass").osc (1, sync, 25).unison (1, 3, 12).osc (2, analog, sawPos, 50).pitch (2, 0, 9).env (2, 50, 500, 45, 250, 55)
            .mod (env2, "osc1Position", 20).mod (env2, "filter1Cutoff", 25).filter (1, ladder24, 1200, 30, 50, 40).amp (5, 400, 90, 300)
            .drive (hard, 30, 8000).reverb (10, 35, 1, 40).level (-19).macro (1, "Sync").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Buckled Section", "Brass").osc (1, analog, sawPos).unison (1, 4, 10, 60).osc (2, analog, sawPos, 70).pitch (2, 12, 9)
            .filter (1, ladder24, 690, 35, 40, 50).env (2, 50, 250, 60, 250, 40).mod (env2, "filter1Cutoff", 34).mod (velocity, "filter1Cutoff", 14)
            .amp (40, 400, 90, 150).lfo (1, sine, 5.25f).lfoFade (1, 250).mod (lfo1, "osc1Fine", 3).reverb (20, 50, 0.6f, 70).level (-11)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Blare").mod (macro2, "filter1Resonance", 35).macro (3, "Bite")
            .mod (macro3, "filter1Drive", 50).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Coastal Mute", "Brass").osc (1, pulse, 70).osc (2, analog, sawPos, 60).pitch (2, 0, 8).filter (1, ladderBand, 1200, 45, 0, 50)
            .env (2, 15, 350, 60, 200, 55).mod (env2, "filter1Cutoff", 8).mod (velocity, "filter1Cutoff", 10).amp (8, 400, 80, 280).mono()
            .lfo (1, sine, 5.75f).lfoFade (1, 550).mod (lfo1, "osc1Fine", 7).mod (lfo1, "osc2Fine", 7).reverb (20, 100, 8.5f, 30).level (-1)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Pinch").mod (macro2, "filter1Resonance", 25).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Cold Blast", "Brass").osc (1, sync, 25).unison (1, 2, 16).osc (2, analog, sawPos, 50).pitch (2, -12, 7).env (2, 90, 350, 50, 250, 55)
            .mod (env2, "osc1Position", 25).mod (env2, "filter1Cutoff", 15).filter (1, ladder24, 1500, 30, 20, 40).amp (10, 400, 85, 300)
            .reverb (20, 85, 3.5f, 55).level (-11).macro (1, "Sync").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Crystal Reveille", "Brass").osc (1, fm, 15).unison (1, 1, 8, 60).osc (2, analog, sawPos, 55).pitch (2, -12, 6)
            .env (2, 80, 650, 55, 250).mod (env2, "osc1Position", 35).mod (velocity, "osc1Position", 16).filter (1, ladder12, 5100, 25)
            .amp (10, 400, 80, 250).lfo (1, sine, 5.5f).lfoFade (1, 450).mod (lfo1, "osc1Fine", 3).reverb (25, 45, 1, 60).level (-11)
            .macro (1, "Index").mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).delay (eighthD, 40, 0, 4500)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Daybreak Stopper", "Brass").osc (1, pulse, 90).osc (2, analog, sawPos, 50).pitch (2, 0, 8).filter (1, svfBand, 1200, 40, 20, 50)
            .env (2, 45, 250, 40, 200, 55).mod (env2, "filter1Cutoff", 18).mod (velocity, "filter1Cutoff", 9).amp (16, 400, 90, 120).mono()
            .lfo (1, sine, 5.25f).lfoFade (1, 500).mod (lfo1, "osc1Fine", 4).mod (lfo1, "osc2Fine", 4).reverb (25, 55, 1.4f, 40).level (-2)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Pinch").mod (macro2, "filter1Resonance", 25).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Width").mod (macro4, "osc1Position", -30);
        add ("Desert Clarion", "Brass").osc (1, fm, 5).unison (1, 3, 6, 40).osc (2, analog, sawPos, 40).pitch (2, 0, 8).env (2, 100, 450, 50, 250)
            .mod (env2, "osc1Position", 30).mod (velocity, "osc1Position", 8).filter (1, ladder24, 4100, 15).amp (15, 400, 80, 200)
            .lfo (1, sine, 4.75f).lfoFade (1, 400).mod (lfo1, "osc1Fine", 4).reverb (25, 30, 1.6f, 70).level (-10).macro (1, "Index")
            .mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Dusky Bray", "Brass").osc (1, sync, 10).unison (1, 4, 10, 60).osc (2, analog, sawPos, 55).pitch (2, -12, 8)
            .env (2, 20, 750, 35, 250, 55).mod (env2, "osc1Position", 20).mod (env2, "filter1Cutoff", 15).filter (1, ladder24, 530, 15, 30, 40)
            .amp (25, 400, 80, 300).reverb (25, 55, 1.4f, 60).level (-10).macro (1, "Sync").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Fevered Ensign", "Brass").osc (1, analog, sawPos).unison (1, 3, 12, 50).osc (2, analog, sawPos, 65).pitch (2, 7, 1).unison (2, 2, 10)
            .osc (3, analog, squarePos, 65).pitch (3, 12, 8).filter (1, ladder24, 330, 20, 30, 40).env (2, 30, 800, 65, 250, 55)
            .mod (env2, "filter1Cutoff", 22).mod (velocity, "filter1Cutoff", 16).amp (40, 400, 80, 400).reverb (30, 90, 3, 60).level (-11)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Fifth").mod (macro2, "osc2Volume", -50).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Feverish Tattoo", "Brass").osc (1, fm, 15).unison (1, 2, 8, 60).osc (2, analog, sawPos, 40).pitch (2, 0, 5).env (2, 100, 800, 65, 250)
            .mod (env2, "osc1Position", 40).mod (velocity, "osc1Position", 8).filter (1, svfLow, 3500, 15).amp (45, 400, 85, 250)
            .lfo (1, sine, 5.75f).lfoFade (1, 250).mod (lfo1, "osc1Fine", 4).reverb (25, 55, 0.8f, 70).level (-13).macro (1, "Index")
            .mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Flayed Rank", "Brass").osc (1, analog, sawPos).unison (1, 5, 16, 90).osc (2, analog, squarePos, 55).pitch (2, -12, 8)
            .filter (1, ladder24, 700, 30, 0, 50).env (2, 110, 450, 45, 250, 55).mod (env2, "filter1Cutoff", 32).mod (velocity, "filter1Cutoff", 16)
            .amp (15, 400, 85, 150).lfo (1, sine, 5.25f).lfoFade (1, 250).mod (lfo1, "osc1Fine", 6).reverb (20, 50, 1.4f, 40).level (-13)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Blare").mod (macro2, "filter1Resonance", 35).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Floating Banner", "Brass").osc (1, analog, sawPos).unison (1, 4, 14).osc (2, analog, sawPos, 70).pitch (2, 7, 4).unison (2, 2, 8)
            .osc (3, analog, sawPos, 65).pitch (3, 12, 4).filter (1, ladder24, 390, 15, 20, 40).env (2, 70, 550, 40, 250, 65)
            .mod (env2, "filter1Cutoff", 30).mod (velocity, "filter1Cutoff", 16).amp (20, 400, 95, 450).reverb (40, 100, 7.5f, 45).level (-13)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Fifth").mod (macro2, "osc2Volume", -50).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Flooded Rank", "Brass").osc (1, analog, sawPos).unison (1, 2, 16, 90).osc (2, analog, 80, 70).pitch (2, 0, 5)
            .filter (1, ladder24, 310, 25, 10, 30).env (2, 40, 350, 55, 250, 40).mod (env2, "filter1Cutoff", 26).mod (velocity, "filter1Cutoff", 8)
            .amp (60, 400, 95, 400).lfo (1, sine, 5.75f).lfoFade (1, 300).mod (lfo1, "osc1Fine", 5).chorus (25, 0.8f, 40).reverb (20, 30, 1.2f, 70)
            .level (-10).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Blare").mod (macro2, "filter1Resonance", 35)
            .drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Gnarled Bray", "Brass").osc (1, sync, 20).unison (1, 4, 16, 50).osc (2, analog, sawPos, 60).pitch (2, -12, 8)
            .env (2, 80, 600, 30, 250, 55).mod (env2, "osc1Position", 40).mod (env2, "filter1Cutoff", 15).filter (1, ladder24, 510, 15, 50, 40)
            .amp (5, 400, 95, 150).drive (hard, 35, 7500).reverb (30, 80, 7, 35).level (-19).macro (1, "Sync").mod (macro1, "osc1Position", 30)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Grave Blare", "Brass").osc (1, sync, 10).unison (1, 4, 10, 70).osc (2, analog, sawPos, 45).pitch (2, 0, 6)
            .env (2, 100, 300, 45, 250, 55).mod (env2, "osc1Position", 40).mod (env2, "filter1Cutoff", 15).filter (1, ladder24, 510, 15, 20, 40)
            .amp (5, 400, 80, 250).reverb (15, 80, 6.5f, 35).level (-7).macro (1, "Sync").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Graveyard Mantle", "Brass").osc (1, analog, sawPos).unison (1, 2, 14, 70).osc (2, analog, sawPos, 60).pitch (2, 7, 5).unison (2, 2, 8)
            .osc (3, analog, sawPos, 65).pitch (3, 12, 7).filter (1, ladder24, 770, 30, 0, 40).env (2, 130, 300, 70, 250, 65)
            .mod (env2, "filter1Cutoff", 24).mod (velocity, "filter1Cutoff", 8).amp (25, 400, 85, 500).reverb (40, 90, 5, 55).level (-13)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Fifth").mod (macro2, "osc2Volume", -50).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Grey Blare", "Brass").osc (1, sync, 25).unison (1, 2, 12, 50).osc (2, analog, sawPos, 45).pitch (2, -12, 7)
            .env (2, 20, 750, 45, 250, 55).mod (env2, "osc1Position", 15).mod (env2, "filter1Cutoff", 10).filter (1, ladder24, 970, 15, 10, 40)
            .amp (15, 400, 90, 250).drive (tube, 15, 5000).reverb (20, 30, 0.8f, 40).level (-11).macro (1, "Sync").mod (macro1, "osc1Position", 30)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Grid Cup", "Brass").osc (1, pulse, 55).osc (2, analog, sawPos, 55).pitch (2, 0, 9).filter (1, ladderBand, 1300, 50, 10, 50)
            .env (2, 15, 600, 30, 200, 55).mod (env2, "filter1Cutoff", 20).mod (velocity, "filter1Cutoff", 9).amp (8, 400, 90, 240).mono (60)
            .lfo (1, sine, 4.5f).lfoFade (1, 550).mod (lfo1, "osc1Fine", 6).mod (lfo1, "osc2Fine", 6).reverb (25, 30, 1.4f, 70)
            .delay (quarter, 25, 18, 2400).level (-2).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Pinch")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Grid Pennant", "Brass").osc (1, analog, sawPos).unison (1, 2, 8, 90).osc (2, analog, sawPos, 60).pitch (2, 7, 4).unison (2, 2, 10)
            .osc (3, analog, 80, 50).pitch (3, -12, 7).filter (1, ladder24, 800, 15, 0, 40).env (2, 80, 600, 55, 250, 60)
            .mod (env2, "filter1Cutoff", 34).mod (velocity, "filter1Cutoff", 8).amp (15, 400, 90, 300).reverb (30, 75, 8.5f, 55).level (-13)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Fifth").mod (macro2, "osc2Volume", -50).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Jagged Clarion", "Brass").osc (1, fm, 10).unison (1, 2, 6, 50).osc (2, analog, sawPos, 60).pitch (2, -12, 7).env (2, 40, 650, 40, 250)
            .mod (env2, "osc1Position", 30).mod (velocity, "osc1Position", 14).filter (1, svfLow, 2200, 25).amp (25, 400, 95, 250)
            .lfo (1, sine, 4.75f).lfoFade (1, 550).mod (lfo1, "osc1Fine", 3).reverb (30, 85, 8, 70).level (-16).macro (1, "Index")
            .mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).drive (hard, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Lingering Ridge", "Brass").osc (1, analog, 60).unison (1, 3, 10, 60).osc (2, analog, 50, 70).pitch (2, -12, 5)
            .filter (1, ladder24, 300, 20, 0, 50).env (2, 100, 550, 60, 250, 55).mod (env2, "filter1Cutoff", 19).mod (velocity, "filter1Cutoff", 6)
            .amp (170, 500, 90, 600).reverb (50, 85, 9, 65).level (-16).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Brass")
            .mod (macro2, "osc1Position", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Midnight Horn", "Brass").osc (1, analog, 40).unison (1, 2, 5, 50).osc (2, analog, 35, 60).pitch (2, 7, 7)
            .filter (1, ladder24, 230, 10, 0, 50).env (2, 170, 900, 60, 250, 55).mod (env2, "filter1Cutoff", 21).mod (velocity, "filter1Cutoff", 6)
            .amp (160, 500, 100, 500).reverb (35, 90, 3, 60).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Brass")
            .mod (macro2, "osc1Position", 30).macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Nameless Bray", "Brass").osc (1, sync, 5).unison (1, 3, 12, 50).osc (2, analog, sawPos, 60).pitch (2, 0, 6)
            .env (2, 90, 650, 30, 250, 55).mod (env2, "osc1Position", 35).mod (env2, "filter1Cutoff", 15).filter (1, ladder24, 1300, 10, 30, 40)
            .amp (5, 400, 95, 150).reverb (25, 80, 7.5f, 65).level (-11).macro (1, "Sync").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Nocturnal Section", "Brass").osc (1, analog, sawPos).unison (1, 3, 18, 90).osc (2, analog, squarePos, 70).pitch (2, 12, 8)
            .filter (1, ladder24, 640, 15, 40, 50).env (2, 130, 700, 65, 250, 40).mod (env2, "filter1Cutoff", 38)
            .mod (velocity, "filter1Cutoff", 12).amp (50, 400, 90, 400).lfo (1, sine, 5.75f).lfoFade (1, 700).mod (lfo1, "osc1Fine", 4)
            .reverb (20, 100, 5.5f, 35).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Blare")
            .mod (macro2, "filter1Resonance", 35).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Outpost Mute", "Brass").osc (1, pulse, 70).osc (2, analog, sawPos, 40).pitch (2, 0, 8).filter (1, ladderBand, 1300, 60, 30, 50)
            .env (2, 20, 550, 60, 200, 55).mod (env2, "filter1Cutoff", 12).mod (velocity, "filter1Cutoff", 11).amp (24, 400, 95, 120).mono (30)
            .lfo (1, sine, 5.25f).lfoFade (1, 600).mod (lfo1, "osc1Fine", 8).mod (lfo1, "osc2Fine", 8).reverb (25, 50, 1.6f, 70).level (-2)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Pinch").mod (macro2, "filter1Resonance", 25).macro (3, "Width")
            .mod (macro3, "osc1Position", -30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Patient Horn", "Brass").osc (1, analog, 52).unison (1, 3, 9, 70).osc (2, analog, 45, 60).pitch (2, 0, 4)
            .filter (1, ladder24, 500, 25, 0, 50).env (2, 100, 700, 50, 250, 55).mod (env2, "filter1Cutoff", 19).mod (velocity, "filter1Cutoff", 6)
            .amp (200, 500, 100, 400).reverb (35, 90, 9, 40).level (-16).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Brass")
            .mod (macro2, "osc1Position", 30).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Ration Rank", "Brass").osc (1, analog, sawPos).unison (1, 5, 12, 90).osc (2, analog, squarePos, 75).pitch (2, 12, 10)
            .filter (1, ladder24, 270, 20, 20, 30).env (2, 90, 700, 75, 250, 65).mod (env2, "filter1Cutoff", 36).mod (velocity, "filter1Cutoff", 12)
            .amp (20, 400, 95, 250).lfo (1, sine, 5.75f).lfoFade (1, 400).mod (lfo1, "osc1Fine", 6).reverb (25, 45, 1.4f, 40).level (-13)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Blare").mod (macro2, "filter1Resonance", 35).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Roadside Mantle", "Brass").osc (1, analog, sawPos).unison (1, 2, 12).osc (2, analog, sawPos).pitch (2, 7, 5).unison (2, 2, 12)
            .osc (3, analog, sawPos, 55).pitch (3, -12, 4).filter (1, ladder24, 750, 30, 0, 40).env (2, 80, 850, 65, 250, 55)
            .mod (env2, "filter1Cutoff", 38).mod (velocity, "filter1Cutoff", 12).amp (45, 400, 90, 250).reverb (25, 80, 6.5f, 30).level (-15)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Fifth").mod (macro2, "osc2Volume", -50).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Ruined Hunt", "Brass").osc (1, analog, 58).unison (1, 3, 5, 40).osc (2, analog, 45, 60).pitch (2, 7, 5)
            .filter (1, ladder12, 520, 20, 0, 50).env (2, 140, 900, 75, 250, 60).mod (env2, "filter1Cutoff", 25).mod (velocity, "filter1Cutoff", 8)
            .amp (200, 500, 100, 400).reverb (45, 75, 3.5f, 60).level (-15).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Brass")
            .mod (macro2, "osc1Position", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Rusted Mute", "Brass").osc (1, pulse, 80).osc (2, analog, sawPos, 40).pitch (2, 0, 4).filter (1, ladderBand, 780, 60, 30, 50)
            .env (2, 65, 650, 40, 200, 55).mod (env2, "filter1Cutoff", 18).mod (velocity, "filter1Cutoff", 7).amp (24, 400, 80, 260).mono (60)
            .lfo (1, sine, 6.5f).lfoFade (1, 400).mod (lfo1, "osc1Fine", 7).mod (lfo1, "osc2Fine", 7).reverb (15, 40, 1.2f, 70)
            .delay (eighthT, 40, 16, 4800).level (1).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Pinch")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Width").mod (macro3, "osc1Position", -30).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Scoured Hunt", "Brass").osc (1, analog, 40).unison (1, 2, 12).osc (2, analog, 60, 50).pitch (2, 7, 5)
            .filter (1, ladder24, 440, 5, 0, 50).env (2, 130, 850, 40, 250, 65).mod (env2, "filter1Cutoff", 17).mod (velocity, "filter1Cutoff", 16)
            .amp (80, 500, 90, 400).reverb (45, 95, 9, 40).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Brass")
            .mod (macro2, "osc1Position", 30).macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Severe Salute", "Brass").osc (1, fm, 25).unison (1, 1, 6, 50).osc (2, analog, sawPos, 55).pitch (2, 0, 3).env (2, 90, 900, 65, 250)
            .mod (env2, "osc1Position", 45).mod (velocity, "osc1Position", 14).filter (1, svfLow, 3600, 5).amp (50, 400, 95, 400)
            .lfo (1, sine, 4.5f).lfoFade (1, 650).mod (lfo1, "osc1Fine", 3).reverb (10, 40, 1.6f, 50).level (-15).macro (1, "Index")
            .mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Smothered Ensign", "Brass").osc (1, analog, sawPos).unison (1, 4, 16, 60).osc (2, analog, sawPos, 70).pitch (2, 7, 4).unison (2, 2, 6)
            .osc (3, analog, sawPos, 50).pitch (3, -12, 3).filter (1, ladder24, 350, 15, 30, 40).env (2, 60, 700, 45, 250, 55)
            .mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 8).amp (15, 400, 85, 250).reverb (30, 90, 5, 40).level (-11)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Fifth").mod (macro2, "osc2Volume", -50).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Splintered Rank", "Brass").osc (1, analog, sawPos).unison (1, 2, 12, 60).osc (2, analog, squarePos, 70).pitch (2, 0, 7)
            .filter (1, ladder24, 600, 10, 10, 30).env (2, 100, 250, 50, 250, 55).mod (env2, "filter1Cutoff", 38)
            .mod (velocity, "filter1Cutoff", 14).amp (35, 400, 85, 400).lfo (1, sine, 4.5f).lfoFade (1, 400).mod (lfo1, "osc1Fine", 6)
            .reverb (20, 95, 6.5f, 35).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Blare")
            .mod (macro2, "filter1Resonance", 35).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Splintered Stopper", "Brass").osc (1, pulse, 55).osc (2, analog, sawPos, 50).pitch (2, 0, 6).filter (1, svfBand, 800, 50, 20, 50)
            .env (2, 80, 600, 50, 200, 55).mod (env2, "filter1Cutoff", 14).mod (velocity, "filter1Cutoff", 7).amp (32, 400, 80, 240).mono (60)
            .lfo (1, sine, 5.5f).lfoFade (1, 250).mod (lfo1, "osc1Fine", 8).mod (lfo1, "osc2Fine", 8).reverb (25, 50, 1, 70).level (-3)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Pinch").mod (macro2, "filter1Resonance", 25).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Starless Ensign", "Brass").osc (1, analog, sawPos).unison (1, 4, 12, 90).osc (2, analog, sawPos, 65).pitch (2, 7).unison (2, 2, 8)
            .osc (3, analog, squarePos, 65).pitch (3, -12, 4).filter (1, ladder24, 390, 15, 30, 40).env (2, 30, 850, 45, 250, 65)
            .mod (env2, "filter1Cutoff", 30).mod (velocity, "filter1Cutoff", 12).amp (35, 400, 90, 400).reverb (30, 80, 7, 40).level (-13)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Fifth").mod (macro2, "osc2Volume", -50).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Sublevel Mute", "Brass").osc (1, pulse, 75).osc (2, analog, sawPos, 55).pitch (2, 0, 4).filter (1, svfBand, 1800, 35, 20, 50)
            .env (2, 70, 300, 60, 200, 55).mod (env2, "filter1Cutoff", 18).mod (velocity, "filter1Cutoff", 6).amp (16, 400, 85, 240).mono()
            .lfo (1, sine, 5.75f).lfoFade (1, 600).mod (lfo1, "osc1Fine", 4).mod (lfo1, "osc2Fine", 4).reverb (35, 70, 7, 40)
            .delay (quarter, 25, 12, 3000).level (-2).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Pinch")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Surgical Cohort", "Brass").osc (1, analog, sawPos).unison (1, 5, 18).osc (2, analog, sawPos, 55).pitch (2, 0, 10)
            .filter (1, ladder24, 790, 20, 20, 50).env (2, 110, 700, 55, 250, 45).mod (env2, "filter1Cutoff", 22).mod (velocity, "filter1Cutoff", 6)
            .amp (60, 400, 85, 150).lfo (1, sine, 5.25f).lfoFade (1, 350).mod (lfo1, "osc1Fine", 4).reverb (25, 35, 0.6f, 70).level (-10)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Blare").mod (macro2, "filter1Resonance", 35).macro (3, "Bite")
            .mod (macro3, "filter1Drive", 50).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Tagged Rank", "Brass").osc (1, analog, sawPos).unison (1, 5, 12, 70).osc (2, analog, sawPos, 75).pitch (2, -12, 7)
            .filter (1, ladder24, 280, 15, 0, 30).env (2, 70, 800, 60, 250, 60).mod (env2, "filter1Cutoff", 34).mod (velocity, "filter1Cutoff", 6)
            .amp (55, 400, 80, 150).lfo (1, sine, 6.5f).lfoFade (1, 450).mod (lfo1, "osc1Fine", 5).reverb (10, 30, 1, 70).level (-12)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Blare").mod (macro2, "filter1Resonance", 35).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Vaulted Horn", "Brass").osc (1, analog, 46).unison (1, 2, 6, 40).osc (2, analog, 45, 50).pitch (2, 7, 3)
            .filter (1, ladder12, 370, 20, 0, 50).env (2, 120, 900, 65, 250, 55).mod (env2, "filter1Cutoff", 21).mod (velocity, "filter1Cutoff", 12)
            .amp (160, 500, 90, 600).reverb (50, 85, 6.5f, 65).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Brass")
            .mod (macro2, "osc1Position", 30).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Violet Clarion", "Brass").osc (1, fm, 10).unison (1, 3, 10, 50).osc (2, analog, sawPos, 50).pitch (2, -12, 5)
            .env (2, 120, 500, 40, 250).mod (env2, "osc1Position", 30).mod (velocity, "osc1Position", 10).filter (1, svfLow, 3800, 20)
            .amp (25, 400, 80, 400).lfo (1, sine, 5).lfoFade (1, 650).mod (lfo1, "osc1Fine", 6).reverb (10, 35, 1.6f, 40).level (-14)
            .macro (1, "Index").mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Wasteland Fanfare", "Brass").osc (1, fm, 20).unison (1, 1, 6, 60).osc (2, analog, sawPos, 50).pitch (2, -12, 6)
            .env (2, 50, 700, 70, 250).mod (env2, "osc1Position", 35).mod (velocity, "osc1Position", 12).filter (1, ladder12, 3500, 20)
            .amp (45, 400, 85, 150).lfo (1, sine, 5.25f).lfoFade (1, 550).mod (lfo1, "osc1Fine", 3).reverb (10, 35, 1.6f, 50).level (-12)
            .macro (1, "Index").mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).delay (eighthD, 40, 0, 4500)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 30).chorus (0, 0.5f, 50).macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Weary Bugle", "Brass").osc (1, analog, 52).unison (1, 2, 11, 50).osc (2, analog, 45, 55).pitch (2, 0, 5)
            .filter (1, ladder12, 510, 25, 0, 50).env (2, 160, 300, 65, 250, 65).mod (env2, "filter1Cutoff", 21).mod (velocity, "filter1Cutoff", 8)
            .amp (190, 500, 95, 400).reverb (30, 75, 9, 70).level (-15).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Brass")
            .mod (macro2, "osc1Position", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);

        // Strings, by name.
        add ("Amber Bounce", "Strings").osc (1, analog, sawPos).unison (1, 5, 14, 90).osc (2, analog, sawPos, 50).pitch (2, -12, 4).noise (1, 1, 20)
            .env (2, 15, 170, 0, 100, 70).mod (env2, "filter1Cutoff", 24).mod (env2, "noiseLevel", 15).filter (1, svfLow, 2100)
            .filter (2, svfHigh, 120).amp (40, 350, 0, 220).reverb (20, 90, 4.5f, 45).level (-11).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Bow").mod (macro2, "noiseLevel", 30).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Bone Nail", "Strings").osc (1, string, 60, 90).warp (1, wOff, 40).unison (1, 4, 9, 60).osc (2, analog, sawPos, 35).pitch (2, -12, 8)
            .filter (1, ladder24, 2000, 5, 0, 50).env (2, 0, 200, 0, 120, 80).mod (env2, "filter1Cutoff", 8).mod (env2, "osc2Volume", 10)
            .amp (0, 300, 0, 300, 70).reverb (20, 90, 6, 70).level (-1).macro (1, "Bright").mod (macro1, "osc1Position", 30).macro (2, "Ring")
            .mod (macro2, "osc1WarpAmount", 30).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Burnished Section", "Strings").osc (1, analog, sawPos).unison (1, 8, 18, 90, 0, 10).osc (2, analog, 80, 60).pitch (2, -12, 8)
            .unison (2, 2, 8, 100).filter (1, ladder12, 5800, 5).filter (2, svfHigh, 210).amp (360, 600, 85, 900).chorus (40, 0.9f, 50)
            .reverb (30, 85, 4, 50).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Detune")
            .mod (macro2, "osc1UnisonDetune", 25).macro (3, "Chorus").mod (macro3, "fxChorusMix", 35).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Calm Cello", "Strings").osc (1, analog, sawPos).unison (1, 5, 16, 70).route (1, 40).osc (2, analog, 80, 75).pitch (2, -12, 7)
            .route (2, 50).parallel().filter (1, ladder24, 820, 15).filter (2, svfBand, 620, 45).amp (240, 600, 95, 900).lfo (1, sine, 6.25f)
            .lfoFade (1, 250).mod (lfo1, "osc1Fine", 3).reverb (30, 95, 3.5f, 45).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22)
            .macro (2, "Body").mod (macro2, "filter2Cutoff", 12).macro (3, "Wood").mod (macro3, "filter2Resonance", 25).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Checkpoint Orchestra", "Strings").osc (1, analog, sawPos).unison (1, 5, 20, 80, 0, 30).osc (2, analog, 80, 60).pitch (2, 12, 10)
            .unison (2, 2, 6, 100).filter (1, ladder12, 4300, 20).filter (2, svfHigh, 200).amp (560, 600, 90, 600).chorus (55, 0.5f, 40)
            .reverb (35, 75, 6, 45).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Detune")
            .mod (macro2, "osc1UnisonDetune", 25).macro (3, "Octave").mod (macro3, "osc2Volume", -45).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Cobalt Rosin", "Strings").osc (1, analog, sawPos, 50).noise (55, 1, 20).filter (1, comb, 261.6f, 93, 0, 100)
            .filter (2, ladder12, 1800, 15, 0, 40).lfo (1, sine, 5.5f).lfoFade (1, 400).mod (lfo1, "osc1Fine", 10).mod (lfo1, "filter1Cutoff", 0.3f)
            .amp (280, 500, 85, 400).reverb (30, 70, 3, 65).level (-15).macro (1, "Scratch").mod (macro1, "noiseLevel", 30).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 22).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Ring")
            .mod (macro4, "filter1Resonance", 5);
        add ("Crimson Staccato", "Strings").osc (1, analog, sawPos).unison (1, 4, 14, 90).osc (2, analog, sawPos, 65).pitch (2, 0, 8)
            .noise (1, 1, 20).env (2, 15, 210, 0, 100, 70).mod (env2, "filter1Cutoff", 24).mod (env2, "noiseLevel", 30)
            .filter (1, ladder12, 2500, 25).filter (2, svfHigh, 180).amp (16, 350, 30, 280).reverb (35, 85, 4, 70).level (-12).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Bow").mod (macro2, "noiseLevel", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Cursed Finger", "Strings").osc (1, string, 60, 90).warp (1, wOff, 25).unison (1, 3, 4, 60, 4).osc (2, analog, sawPos, 40)
            .pitch (2, 0, 3).filter (1, ladder12, 1800, 5, 0, 50).env (2, 0, 80, 0, 120, 80).mod (env2, "filter1Cutoff", 8)
            .mod (env2, "osc2Volume", 15).amp (0, 500, 0, 350, 70).reverb (40, 75, 8.5f, 35).level (-5).macro (1, "Bright")
            .mod (macro1, "osc1Position", 30).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 30).macro (3, "Cutoff")
            .mod (macro3, "filter1Cutoff", 20).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Dawnless Peg", "Strings").osc (1, analog, sawPos, 60).noise (55, 1, 20).filter (1, comb, 261.6f, 80, 0, 100)
            .filter (2, svfLow, 2800, 20, 0, 40).lfo (1, sine, 6.25f).lfoFade (1, 300).mod (lfo1, "osc1Fine", 9).mod (lfo1, "filter1Cutoff", 0.1f)
            .amp (100, 500, 85, 400).reverb (25, 90, 7, 60).level (-15).macro (1, "Scratch").mod (macro1, "noiseLevel", 30).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 22).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Dawnless Shudder", "Strings").osc (1, analog, sawPos).unison (1, 8, 18, 100).osc (2, analog, sawPos, 70).pitch (2, 12, 4)
            .lfo (1, sine, 8.5f, 0).mod (lfo1, "ampLevel", 14).filter (1, svfLow, 1500).filter (2, svfHigh, 320).amp (700, 600, 95, 1100)
            .reverb (25, 90, 6, 40).level (-17).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Shake")
            .mod (macro2, "ampLevel", 10).macro (3, "Speed").mod (macro3, "lfo1Rate", 12).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Distant Orchestra", "Strings").osc (1, analog, sawPos).unison (1, 6, 20, 90, 0, 10).osc (2, analog, 80, 55).pitch (2, 0, 10)
            .unison (2, 2, 10, 100).filter (1, ladder12, 2700, 15).filter (2, svfHigh, 190).amp (340, 600, 85, 500).chorus (50, 0.5f, 40)
            .reverb (35, 90, 7.5f, 35).level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Detune")
            .mod (macro2, "osc1UnisonDetune", 25).macro (3, "Chorus").mod (macro3, "fxChorusMix", 35).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Echoing Pizzicato", "Strings").osc (1, string, 40, 90).warp (1, wOff, 20).unison (1, 2, 4, 90).osc (2, analog, sawPos, 45)
            .pitch (2, -12, 4).filter (1, svfLow, 1600, 5, 0, 50).env (2, 0, 70, 0, 120, 80).mod (env2, "filter1Cutoff", 12)
            .mod (env2, "osc2Volume", 10).amp (0, 700, 0, 200, 60).reverb (40, 80, 4.5f, 70).level (-6).macro (1, "Bright")
            .mod (macro1, "osc1Position", 30).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 30).delay (eighthD, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Cutoff").mod (macro4, "filter1Cutoff", 20);
        add ("Felt Quartet", "Strings").osc (1, analog, sawPos).unison (1, 8, 18, 80, 0, 10).osc (2, analog, sawPos, 60).pitch (2, -12, 8)
            .unison (2, 2, 10, 100).filter (1, svfLow, 2500, 20).filter (2, svfHigh, 150).amp (300, 600, 85, 1400).chorus (50, 0.3f, 60)
            .reverb (45, 80, 5, 70).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Detune")
            .mod (macro2, "osc1UnisonDetune", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Chorus")
            .mod (macro4, "fxChorusMix", 35);
        add ("Flooded Ripple", "Strings").osc (1, analog, sawPos).unison (1, 8, 12).osc (2, analog, sawPos, 55).pitch (2, 12, 4)
            .lfo (1, triangle, 8, 0).mod (lfo1, "ampLevel", 16).filter (1, ladder12, 2800, 5).filter (2, svfHigh, 410).amp (500, 600, 85, 1100)
            .reverb (40, 80, 7.5f, 35).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Shake")
            .mod (macro2, "ampLevel", 10).macro (3, "Speed").mod (macro3, "lfo1Rate", 12).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Fogbound Desk", "Strings").osc (1, analog, sawPos).unison (1, 5, 18, 80, 0, 30).osc (2, analog, sawPos, 65).pitch (2, -12, 7)
            .unison (2, 2, 12, 100).filter (1, ladder12, 2300).filter (2, svfHigh, 350).amp (220, 600, 100, 1100).chorus (40, 0.3f, 50)
            .reverb (40, 95, 7, 65).level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Detune")
            .mod (macro2, "osc1UnisonDetune", 25).macro (3, "Chorus").mod (macro3, "fxChorusMix", 35).macro (4, "Octave")
            .mod (macro4, "osc2Volume", -45);
        add ("Frozen Flutter", "Strings").osc (1, analog, sawPos).unison (1, 8, 22).osc (2, analog, sawPos, 60).pitch (2, 12, 9)
            .lfo (1, sine, 10.5f, 0).mod (lfo1, "ampLevel", 18).filter (1, svfLow, 4700, 15).filter (2, svfHigh, 490).amp (650, 600, 90, 600)
            .reverb (35, 80, 3.5f, 60).level (-17).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Shake")
            .mod (macro2, "ampLevel", 10).macro (3, "Speed").mod (macro3, "lfo1Rate", 12).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Frozen Rosin", "Strings").osc (1, analog, sawPos, 45).noise (30, 1, 20).filter (1, comb, 261.6f, 90, 0, 100)
            .filter (2, ladder12, 5100, 15, 0, 40).lfo (1, sine, 4.75f).lfoFade (1, 300).mod (lfo1, "osc1Fine", 8).mod (lfo1, "filter1Cutoff", 0.4f)
            .amp (380, 500, 95, 300).reverb (35, 70, 9, 40).level (-12).macro (1, "Scratch").mod (macro1, "noiseLevel", 30).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 22).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Gated Scroll", "Strings").osc (1, analog, sawPos, 50).noise (55, 1, 20).filter (1, comb, 261.6f, 93, 0, 100)
            .filter (2, ladder12, 1700, 5, 0, 40).lfo (1, sine, 4.5f).lfoFade (1, 400).mod (lfo1, "osc1Fine", 5).mod (lfo1, "filter1Cutoff", 0.1f)
            .amp (80, 500, 100, 750).reverb (40, 100, 3, 40).level (-15).macro (1, "Scratch").mod (macro1, "noiseLevel", 30).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 22).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Ring")
            .mod (macro4, "filter1Resonance", 5);
        add ("Glacial Shiver", "Strings").osc (1, analog, sawPos).unison (1, 5, 20, 100).osc (2, analog, sawPos, 50).pitch (2, -12, 7)
            .lfo (1, sine, 11.5f, 0).mod (lfo1, "ampLevel", 12).filter (1, svfLow, 4900, 20).filter (2, svfHigh, 200).amp (800, 600, 90, 900)
            .reverb (25, 80, 4, 70).level (-18).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Shake")
            .mod (macro2, "ampLevel", 10).macro (3, "Speed").mod (macro3, "lfo1Rate", 12).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Harbour Staccato", "Strings").osc (1, analog, sawPos).unison (1, 5, 18, 70).osc (2, analog, sawPos, 70).pitch (2, 0, 8)
            .noise (1, 1, 20).env (2, 25, 110, 0, 100, 70).mod (env2, "filter1Cutoff", 14).mod (env2, "noiseLevel", 20)
            .filter (1, ladder24, 970, 25).filter (2, svfHigh, 260).amp (36, 300, 0, 220).reverb (25, 70, 6, 55).level (-7).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Bow").mod (macro2, "noiseLevel", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Haunted Pizzicato", "Strings").osc (1, string, 45, 90).warp (1, wOff, 35).unison (1, 4, 6, 70).osc (2, analog, sawPos, 45)
            .pitch (2, 0, 6).filter (1, svfLow, 1900, 5, 0, 50).env (2, 0, 180, 0, 120, 80).mod (env2, "filter1Cutoff", 12)
            .mod (env2, "osc2Volume", 20).amp (0, 550, 0, 250, 70).reverb (40, 85, 4.5f, 65).level (-9).macro (1, "Bright")
            .mod (macro1, "osc1Position", 30).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Hidden Viol", "Strings").osc (1, analog, sawPos).unison (1, 3, 10, 70).route (1, 60).osc (2, analog, sawPos, 65).pitch (2, -12, 8)
            .route (2, 50).parallel().filter (1, ladder24, 820, 5).filter (2, svfBand, 610, 55).amp (500, 600, 100, 900).lfo (1, sine, 4.75f)
            .lfoFade (1, 650).mod (lfo1, "osc1Fine", 5).reverb (25, 100, 9, 35).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22)
            .macro (2, "Body").mod (macro2, "filter2Cutoff", 12).macro (3, "Wood").mod (macro3, "filter2Resonance", 25).drive (tube, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Hooded Tap", "Strings").osc (1, string, 50, 90).warp (1, wOff, 30).unison (1, 4, 7, 90, 12).osc (2, analog, sawPos, 35)
            .pitch (2, -12, 5).filter (1, ladder24, 2500, 5, 0, 50).env (2, 0, 200, 0, 120, 80).mod (env2, "filter1Cutoff", 12)
            .mod (env2, "osc2Volume", 20).amp (0, 550, 0, 200, 60).reverb (25, 75, 3, 70).level (-3).macro (1, "Bright")
            .mod (macro1, "osc1Position", 30).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 30).drive (tube, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Cutoff").mod (macro4, "filter1Cutoff", 20);
        add ("Hungry Consort", "Strings").osc (1, analog, sawPos).unison (1, 8, 14, 80, 0, 40).osc (2, analog, sawPos, 75).pitch (2, 0, 6)
            .unison (2, 2, 8, 100).filter (1, svfLow, 3200).filter (2, svfHigh, 170).amp (640, 600, 100, 600).chorus (40, 0.5f, 60)
            .reverb (25, 95, 7, 55).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Detune")
            .mod (macro2, "osc1UnisonDetune", 25).macro (3, "Octave").mod (macro3, "osc2Volume", -45).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Lingering Basso", "Strings").osc (1, analog, sawPos).unison (1, 3, 18, 90).route (1, 60).osc (2, analog, 80, 60).pitch (2, -12, 5)
            .route (2, 20).parallel().filter (1, ladder24, 730, 20).filter (2, svfBand, 260, 50).amp (160, 600, 85, 700).lfo (1, sine, 5.5f)
            .lfoFade (1, 300).mod (lfo1, "osc1Fine", 6).reverb (45, 85, 7, 55).level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22)
            .macro (2, "Body").mod (macro2, "filter2Cutoff", 12).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Wood")
            .mod (macro4, "filter2Resonance", 25);
        add ("Listing Pizzicato", "Strings").osc (1, string, 40, 90).warp (1, wOff, 40).unison (1, 3, 9, 50, 12).osc (2, analog, sawPos, 30)
            .pitch (2, 0, 8).filter (1, ladder12, 2400, 5, 0, 50).env (2, 0, 60, 0, 120, 80).mod (env2, "filter1Cutoff", 20)
            .mod (env2, "osc2Volume", 30).amp (0, 350, 0, 350, 65).reverb (30, 100, 4, 50).level (-5).macro (1, "Bright")
            .mod (macro1, "osc1Position", 30).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 30).macro (3, "Cutoff")
            .mod (macro3, "filter1Cutoff", 20).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Listing Stroke", "Strings").osc (1, analog, sawPos).unison (1, 6, 20, 100).osc (2, analog, 80, 65).pitch (2, 0, 4).noise (1, 1, 20)
            .env (2, 20, 210, 0, 100, 70).mod (env2, "filter1Cutoff", 24).mod (env2, "noiseLevel", 35).filter (1, ladder24, 1500, 25)
            .filter (2, svfHigh, 120).amp (12, 200, 0, 180).reverb (25, 85, 6, 50).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22)
            .macro (2, "Bow").mod (macro2, "noiseLevel", 30).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Masked Bounce", "Strings").osc (1, analog, sawPos).unison (1, 6, 14, 70).osc (2, analog, 80, 60).pitch (2, 0, 5).noise (1, 1, 20)
            .env (2, 10, 100, 0, 100, 70).mod (env2, "filter1Cutoff", 14).mod (env2, "noiseLevel", 20).filter (1, ladder24, 1000, 15)
            .filter (2, svfHigh, 280).amp (36, 400, 0, 260).reverb (25, 75, 3.5f, 70).level (-4).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Bow").mod (macro2, "noiseLevel", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Merciless Quiver", "Strings").osc (1, analog, sawPos).unison (1, 5, 18).osc (2, analog, sawPos, 70).pitch (2, 12, 8)
            .lfo (1, sine, 9.5f, 0).mod (lfo1, "ampLevel", 18).filter (1, ladder12, 1700, 20).filter (2, svfHigh, 380).amp (450, 600, 100, 600)
            .reverb (45, 95, 4, 30).level (-19).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Shake")
            .mod (macro2, "ampLevel", 10).macro (3, "Speed").mod (macro3, "lfo1Rate", 12).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Nameless Stroke", "Strings").osc (1, analog, sawPos).unison (1, 6, 20, 90).osc (2, analog, sawPos, 50).pitch (2, 12, 7)
            .noise (1, 1, 20).env (2, 5, 230, 0, 100, 70).mod (env2, "filter1Cutoff", 12).mod (env2, "noiseLevel", 15)
            .filter (1, ladder12, 2500, 25).filter (2, svfHigh, 140).amp (36, 200, 0, 160).reverb (30, 95, 3, 40).level (-9).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Bow").mod (macro2, "noiseLevel", 30).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Oiled Flutter", "Strings").osc (1, analog, sawPos).unison (1, 5, 18, 90).osc (2, analog, sawPos, 70).pitch (2, -12, 5)
            .lfo (1, sine, 8, 0).mod (lfo1, "ampLevel", 18).filter (1, ladder12, 2300, 5).filter (2, svfHigh, 270).amp (450, 600, 90, 800)
            .reverb (30, 70, 9, 50).level (-16).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Shake")
            .mod (macro2, "ampLevel", 10).macro (3, "Speed").mod (macro3, "lfo1Rate", 12).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Padded Shudder", "Strings").osc (1, analog, sawPos).unison (1, 8, 12).osc (2, analog, sawPos, 70).pitch (2, -12, 5)
            .lfo (1, sine, 9, 0).mod (lfo1, "ampLevel", 16).filter (1, svfLow, 1700).filter (2, svfHigh, 340).amp (150, 600, 90, 700)
            .reverb (30, 95, 5, 55).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Shake")
            .mod (macro2, "ampLevel", 10).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Reckless Section", "Strings").osc (1, analog, sawPos).unison (1, 5, 16, 90, 0, 30).osc (2, analog, 80, 75).pitch (2, 12, 4)
            .unison (2, 2, 8, 100).filter (1, svfLow, 3600, 15).filter (2, svfHigh, 190).amp (180, 600, 90, 1400).chorus (45, 0.3f, 40)
            .reverb (25, 90, 4, 35).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Detune")
            .mod (macro2, "osc1UnisonDetune", 25).macro (3, "Octave").mod (macro3, "osc2Volume", -45).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Scalding Spiccato", "Strings").osc (1, analog, sawPos).unison (1, 4, 14, 90).osc (2, analog, 80, 60).pitch (2, -12, 7)
            .noise (1, 1, 20).env (2, 25, 90, 0, 100, 70).mod (env2, "filter1Cutoff", 14).mod (env2, "noiseLevel", 35).filter (1, ladder12, 2600, 5)
            .filter (2, svfHigh, 130).amp (36, 500, 0, 280).reverb (25, 95, 8.5f, 55).level (-11).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Bow").mod (macro2, "noiseLevel", 30).drive (hard, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Septic Basso", "Strings").osc (1, analog, sawPos).unison (1, 5, 14, 90).route (1, 50).osc (2, analog, 80, 75).pitch (2, -12, 4)
            .route (2, 40).parallel().filter (1, ladder12, 700, 20).filter (2, svfBand, 340, 40).amp (340, 600, 95, 300).lfo (1, sine, 5.25f)
            .lfoFade (1, 300).mod (lfo1, "osc1Fine", 3).reverb (40, 75, 6.5f, 60).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22)
            .macro (2, "Body").mod (macro2, "filter2Cutoff", 12).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).drive (tube, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Severed Orchestra", "Strings").osc (1, analog, sawPos).unison (1, 6, 14, 80, 0, 20).osc (2, analog, sawPos, 55).pitch (2, 12, 10)
            .unison (2, 2, 8, 100).filter (1, svfLow, 5300).filter (2, svfHigh, 160).amp (360, 600, 85, 900).chorus (50, 0.7f, 40)
            .reverb (30, 100, 4.5f, 40).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Detune")
            .mod (macro2, "osc1UnisonDetune", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Octave")
            .mod (macro4, "osc2Volume", -45);
        add ("Spectral Contra", "Strings").osc (1, analog, sawPos).unison (1, 5, 10, 70).route (1, 40).osc (2, analog, sawPos, 60).pitch (2, -12, 6)
            .route (2, 30).parallel().filter (1, ladder12, 1800, 5).filter (2, svfBand, 410, 50).amp (80, 600, 90, 300).lfo (1, sine, 4.5f)
            .lfoFade (1, 400).mod (lfo1, "osc1Fine", 3).reverb (40, 100, 8, 60).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22)
            .macro (2, "Body").mod (macro2, "filter2Cutoff", 12).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Stripped Pizzicato", "Strings").osc (1, string, 35, 90).warp (1, wOff, 25).unison (1, 2, 3, 90, 12).osc (2, analog, sawPos, 45)
            .pitch (2, 0, 6).filter (1, ladder24, 2800, 5, 0, 50).env (2, 0, 220, 0, 120, 80).mod (env2, "filter1Cutoff", 16)
            .mod (env2, "osc2Volume", 10).amp (0, 550, 0, 150, 60).reverb (25, 95, 3, 40).level (-9).macro (1, "Bright")
            .mod (macro1, "osc1Position", 30).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 30).macro (3, "Cutoff")
            .mod (macro3, "filter1Cutoff", 20).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Trailing Marcato", "Strings").osc (1, analog, sawPos).unison (1, 4, 12, 70).osc (2, analog, sawPos, 60).pitch (2, 12, 8)
            .noise (1, 1, 20).env (2, 25, 220, 0, 100, 70).mod (env2, "filter1Cutoff", 10).mod (env2, "noiseLevel", 35).filter (1, ladder12, 940)
            .filter (2, svfHigh, 240).amp (12, 300, 30, 140).reverb (40, 95, 6.5f, 65).level (-8).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Bow").mod (macro2, "noiseLevel", 30).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Trailing Scroll", "Strings").osc (1, analog, sawPos, 60).noise (35, 1, 60).filter (1, comb, 261.6f, 90, 0, 100)
            .filter (2, ladder12, 3300, 5, 0, 40).lfo (1, sine, 6.5f).lfoFade (1, 700).mod (lfo1, "osc1Fine", 7).mod (lfo1, "filter1Cutoff", 0.3f)
            .amp (120, 500, 90, 250).mono (130).reverb (45, 85, 3, 50).level (-8).macro (1, "Scratch").mod (macro1, "noiseLevel", 30)
            .macro (2, "Tone").mod (macro2, "filter2Cutoff", 22).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30)
            .macro (4, "Ring").mod (macro4, "filter1Resonance", 5);
        add ("Tundra Burden", "Strings").osc (1, analog, sawPos).unison (1, 4, 16, 90).route (1, 30).osc (2, analog, 80, 55).pitch (2, -12, 6)
            .route (2, 50).parallel().filter (1, ladder12, 1100, 20).filter (2, svfBand, 370, 45).amp (260, 600, 85, 700).lfo (1, sine, 5.5f)
            .lfoFade (1, 650).mod (lfo1, "osc1Fine", 3).reverb (25, 85, 6, 50).level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22)
            .macro (2, "Body").mod (macro2, "filter2Cutoff", 12).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).drive (tube, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Vaulted Finger", "Strings").osc (1, string, 30, 90).warp (1, wOff, 30).unison (1, 3, 3, 70, 8).osc (2, analog, sawPos, 35)
            .pitch (2, -12, 3).filter (1, ladder12, 3000, 5, 0, 50).env (2, 0, 200, 0, 120, 80).mod (env2, "filter1Cutoff", 10)
            .mod (env2, "osc2Volume", 25).amp (0, 450, 0, 150, 55).reverb (40, 90, 7, 45).level (-5).macro (1, "Bright")
            .mod (macro1, "osc1Position", 30).macro (2, "Ring").mod (macro2, "osc1WarpAmount", 30).delay (eighthD, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Wan Bass", "Strings").osc (1, analog, sawPos).unison (1, 5, 12, 90).route (1, 50).osc (2, analog, 80, 75).pitch (2, -12, 8)
            .route (2, 20).parallel().filter (1, ladder12, 2400, 15).filter (2, svfBand, 430, 60).amp (80, 600, 100, 300).lfo (1, sine, 5.75f)
            .lfoFade (1, 500).mod (lfo1, "osc1Fine", 6).reverb (25, 100, 5, 40).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22)
            .macro (2, "Body").mod (macro2, "filter2Cutoff", 12).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Wary Cello", "Strings").osc (1, analog, sawPos).unison (1, 4, 8).route (1, 40).osc (2, analog, sawPos, 70).pitch (2, -12, 4)
            .route (2, 50).parallel().filter (1, ladder24, 1300, 20).filter (2, svfBand, 380, 50).amp (100, 600, 90, 1000).lfo (1, sine, 5)
            .lfoFade (1, 700).mod (lfo1, "osc1Fine", 4).reverb (30, 70, 4.5f, 50).level (-10).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22)
            .macro (2, "Body").mod (macro2, "filter2Cutoff", 12).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Watched Shudder", "Strings").osc (1, analog, sawPos).unison (1, 5, 12, 90).osc (2, analog, sawPos, 65).pitch (2, -12, 6)
            .lfo (1, sine, 8, 0).mod (lfo1, "ampLevel", 14).filter (1, svfLow, 4400, 15).filter (2, svfHigh, 500).amp (150, 600, 90, 1000)
            .reverb (30, 100, 4.5f, 70).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Shake")
            .mod (macro2, "ampLevel", 10).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);

        // Pad, by name.
        add ("Adrift Shroud", "Pad").osc (1, pulse, 40).unison (1, 4, 10, 90).osc (2, pulse, 40, 65).pitch (2, -12, 5).lfo (1, triangle, 0.43f, 0)
            .mod (lfo1, "osc1Position", 15).lfo (2, sine, 0.13f, 0).mod (lfo2, "osc2Position", 12).filter (1, ladder24, 970)
            .amp (1200, 1000, 85, 3600).reverb (35, 75, 3, 50).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Width")
            .mod (macro2, "osc1Position", 30).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Arctic Waft", "Pad").set ("osc1On", 0).noise (60, 0, 40).osc (2, vowel, 30, 65).unison (2, 2, 10, 100).route (2, 100).parallel()
            .filter (1, ladderBand, 261.6f, 75, 0, 100).filter (2, svfLow, 1800).lfo (1, sine, 0.33f, 0).mod (lfo1, "filter1Cutoff", 5)
            .mod (lfo1, "noiseLevel", 10).amp (2000, 1000, 85, 4300).reverb (55, 90, 6.5f, 60).level (-6).macro (1, "Air")
            .mod (macro1, "noiseLevel", 30).macro (2, "Tone").mod (macro2, "filter2Cutoff", 25).macro (3, "Whistle")
            .mod (macro3, "filter1Resonance", 14).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Banished Exhale", "Pad").set ("osc1On", 0).noise (70, 1, 40).osc (2, vowel, 30, 45).unison (2, 3, 10, 100).route (2, 100).parallel()
            .filter (1, ladderBand, 523.3f, 60, 0, 100).filter (2, ladder12, 2400).lfo (1, drift, 0.098f, 0).mod (lfo1, "filter1Cutoff", 4)
            .mod (lfo1, "noiseLevel", 5).amp (1400, 1000, 90, 2900).reverb (45, 95, 6, 50).level (-1).macro (1, "Air")
            .mod (macro1, "noiseLevel", 30).macro (2, "Tone").mod (macro2, "filter2Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Binary Corrosion", "Pad").osc (1, growl, 20).unison (1, 3, 16, 70).osc (2, pulse, 50, 40).pitch (2, -12, 9).lfo (1, sine, 0.34f, 0)
            .mod (lfo1, "osc1Position", 15).filter (1, svfLow, 1000, 15, 50).amp (800, 1000, 95, 1900).drive (crush, 30, 4100)
            .reverb (50, 85, 3, 55).level (-15).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Growl")
            .mod (macro2, "osc1Position", 35).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Black Litany", "Pad").osc (1, analog, sawPos).unison (1, 6, 20, 100).route (1, 50).osc (2, pulse, 40, 50).pitch (2, 12, 9)
            .route (2, 50).parallel().filter (1, svfBand, 300, 65).filter (2, svfBand, 2250, 55).lfo (1, sine, 0.087f, 0)
            .mod (lfo1, "filter1Cutoff", 4).mod (lfo1, "filter2Cutoff", -6).amp (1700, 1000, 95, 1900).chorus (40, 0.7f, 50)
            .reverb (50, 95, 8.5f, 30).level (-3).macro (1, "Mouth").mod (macro1, "filter1Cutoff", 12).macro (2, "Vowel")
            .mod (macro2, "filter2Cutoff", -12).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Boundless Exhale", "Pad").set ("osc1On", 0).noise (60, 1, 40).osc (2, vowel, 0, 55).unison (2, 3, 12, 100).route (2, 100).parallel()
            .filter (1, ladderBand, 261.6f, 60, 0, 100).filter (2, ladder12, 1300).lfo (1, drift, 0.053f, 0).mod (lfo1, "filter1Cutoff", 2)
            .mod (lfo1, "noiseLevel", 5).amp (1900, 1000, 95, 3100).reverb (50, 80, 6, 60).level (-3).macro (1, "Air")
            .mod (macro1, "noiseLevel", 30).macro (2, "Tone").mod (macro2, "filter2Cutoff", 25).macro (3, "Motion").mod (macro3, "lfo1Rate", 30)
            .macro (4, "Whistle").mod (macro4, "filter1Resonance", 14);
        add ("Boundless Leaf", "Pad").osc (1, metal, 40).unison (1, 4, 12).osc (2, vowel, 30, 65).pitch (2, 12, 4).unison (2, 2, 10, 100)
            .lfo (1, drift, 0.12f, 0).mod (lfo1, "osc1Position", 10).filter (1, svfBand, 3400).amp (500, 1000, 100, 3300).phaser (40, 0.18f, 45)
            .reverb (50, 75, 5.5f, 65).level (-6).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Partials")
            .mod (macro2, "osc1Position", 35).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Bronze Panel", "Pad").osc (1, metal, 15).unison (1, 4, 16, 100).osc (2, analog, 10, 55).pitch (2, -12, 5).unison (2, 2, 10, 100)
            .lfo (1, drift, 0.25f, 0).mod (lfo1, "osc1Position", 15).filter (1, svfLow, 3700, 20).amp (1300, 1000, 90, 2300)
            .reverb (50, 70, 8.5f, 60).level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Partials")
            .mod (macro2, "osc1Position", 35).macro (3, "Motion").mod (macro3, "lfo1Rate", 30).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Bruised Pall", "Pad").osc (1, pulse, 45).unison (1, 3, 16, 70).osc (2, pulse, 60, 65).pitch (2, -12, 8).lfo (1, triangle, 0.34f, 0)
            .mod (lfo1, "osc1Position", 15).lfo (2, sine, 0.15f, 0).mod (lfo2, "osc2Position", 12).filter (1, ladder24, 3200, 25)
            .amp (1300, 1000, 100, 2200).chorus (45, 0.3f, 50).reverb (30, 75, 8, 40).level (-11).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Width").mod (macro2, "osc1Position", 30).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Calm Mass", "Pad").osc (1, analog, sawPos).unison (1, 4, 14).osc (2, analog, sawPos, 65).pitch (2, 7, 3).unison (2, 2, 14, 100)
            .osc (3, analog, sinePos, 45).pitch (3, -12, 6).filter (1, ladder24, 750, 30).lfo (1, drift, 0.066f, 0).mod (lfo1, "filter1Cutoff", 9)
            .amp (1300, 1000, 90, 4400).chorus (25, 0.3f, 50).reverb (30, 80, 3.5f, 30).level (-9).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Fifth").mod (macro2, "osc2Volume", -50).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Calm Rite", "Pad").osc (1, vowel, 20).unison (1, 3, 12, 70).osc (2, vowel, 20, 65).pitch (2, 7, 1).unison (2, 2, 8, 100)
            .osc (3, analog, 30, 50).pitch (3, 12, 8).filter (1, svfLow, 930, 30).lfo (1, drift, 0.05f, 0).mod (lfo1, "filter1Cutoff", 7)
            .amp (2200, 1000, 100, 4000).chorus (45, 0.8f, 50).reverb (45, 90, 7, 70).level (-6).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Fifth").mod (macro2, "osc2Volume", -50).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Dayshift Shade", "Pad").osc (1, analog, sawPos).unison (1, 3, 22, 90).osc (2, analog, trianglePos, 60).pitch (2, 12, 8)
            .filter (1, ladder24, 510, 40).lfo (1, drift, 0.034f, 0).mod (lfo1, "filter1Cutoff", 20).amp (1900, 1000, 100, 4300)
            .reverb (30, 90, 7, 50).level (0).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 35).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Dormant Gleam", "Pad").osc (1, analog, sinePos, 1).pitch (1, 24).osc (2, analog, sinePos).warp (2, wFm, 15).unison (2, 4, 6, 90)
            .lfo (1, sine, 0.13f, 0).mod (lfo1, "osc2WarpAmount", 20).filter (1, svfLow, 2200, 5).amp (1300, 1000, 85, 1900).chorus (45, 0.2f, 50)
            .reverb (40, 75, 8.5f, 35).level (-12).macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).macro (3, "Wide").mod (macro3, "fxChorusMix", 40).delay (eighthD, 40, 0, 4500).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Drowsy Sheet", "Pad").osc (1, metal, 35).unison (1, 4, 20).osc (2, vowel, 60, 50).pitch (2, -12, 9).unison (2, 2, 8, 100)
            .lfo (1, drift, 0.073f, 0).mod (lfo1, "osc1Position", 30).filter (1, svfBand, 1500).amp (2100, 1000, 95, 3300).phaser (35, 0.06f, 65)
            .reverb (50, 80, 6, 45).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Partials")
            .mod (macro2, "osc1Position", 35).macro (3, "Motion").mod (macro3, "lfo1Rate", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Encrypted Verdigris", "Pad").osc (1, sync, 35).unison (1, 3, 10).lfo (1, drift, 0.11f, 0).mod (lfo1, "osc1Position", 25)
            .filter (1, ladderBand, 2200, 20, 20).amp (2000, 1000, 95, 2500).drive (crush, 20, 4800).reverb (45, 100, 8.5f, 45).level (-5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Growl").mod (macro2, "osc1Position", 35).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 40);
        add ("Endless Aura", "Pad").osc (1, analog, sinePos, 1).pitch (1, 24).osc (2, analog, sinePos).warp (2, wFm, 10).unison (2, 3, 8, 70)
            .osc (3, analog, sinePos, 40).pitch (3, 7).unison (3, 2, 8, 100).lfo (1, triangle, 0.32f, 0).mod (lfo1, "osc2WarpAmount", 25)
            .filter (1, svfLow, 3400, 15).amp (1100, 1000, 100, 1500).chorus (35, 0.2f, 50).reverb (45, 95, 6, 65).level (-12).macro (1, "Index")
            .mod (macro1, "osc2WarpAmount", 35).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Wide").mod (macro4, "fxChorusMix", 40);
        add ("Endless Leaf", "Pad").osc (1, metal, 55).unison (1, 3, 20, 100).osc (2, analog, 10, 50).pitch (2, 12, 9).unison (2, 2, 10, 100)
            .lfo (1, sine, 0.042f, 0).mod (lfo1, "osc1Position", 15).filter (1, svfLow, 3300, 25).amp (1900, 1000, 85, 3700).phaser (25, 0.12f, 65)
            .reverb (55, 90, 9, 60).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Partials")
            .mod (macro2, "osc1Position", 35).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Endless Patina", "Pad").osc (1, fm, 60).unison (1, 5, 16, 70).osc (2, pulse, 60, 45).pitch (2, 12, 5).lfo (1, drift, 0.17f, 0)
            .mod (lfo1, "osc1Position", 30).filter (1, ladderBand, 1100, 30, 20).amp (800, 1000, 95, 3500).drive (fuzz, 20, 3800)
            .reverb (50, 75, 4, 60).level (-17).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Growl")
            .mod (macro2, "osc1Position", 35).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Fallen Slag", "Pad").osc (1, sync, 55).unison (1, 3, 22, 70).lfo (1, drift, 0.061f, 0).mod (lfo1, "osc1Position", 20)
            .filter (1, ladder24, 1700, 40, 60).amp (1600, 1000, 100, 2000).drive (crush, 25, 3600).reverb (50, 100, 7, 60).level (-12)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Growl").mod (macro2, "osc1Position", 35).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Felt Breath", "Pad").set ("osc1On", 0).noise (55, 0, -20).osc (2, vowel, 10, 50).unison (2, 3, 6, 100).route (2, 100).parallel()
            .filter (1, svfBand, 523.3f, 70, 0, 100).filter (2, ladder12, 2900).lfo (1, drift, 0.12f, 0).mod (lfo1, "filter1Cutoff", 5)
            .mod (lfo1, "noiseLevel", 5).amp (800, 1000, 100, 4300).reverb (55, 80, 4, 45).level (-5).macro (1, "Air")
            .mod (macro1, "noiseLevel", 30).macro (2, "Tone").mod (macro2, "filter2Cutoff", 25).macro (3, "Motion").mod (macro3, "lfo1Rate", 30)
            .macro (4, "Whistle").mod (macro4, "filter1Resonance", 14);
        add ("Felt Mass", "Pad").osc (1, analog, sawPos).unison (1, 4, 10, 70).osc (2, analog, sawPos, 65).pitch (2, 7, 5).unison (2, 2, 14, 100)
            .osc (3, additive, 60, 60).pitch (3, -12, 8).filter (1, ladder12, 2700, 30).lfo (1, sine, 0.21f, 0).mod (lfo1, "filter1Cutoff", 11)
            .amp (2300, 1000, 100, 3200).chorus (40, 0.7f, 50).reverb (45, 70, 5.5f, 30).level (-11).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Fifth").mod (macro2, "osc2Volume", -50).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Fevered Requiem", "Pad").osc (1, analog, sawPos).unison (1, 6, 18, 90).route (1, 50).osc (2, pulse, 60, 55).pitch (2, -12, 4)
            .route (2, 50).parallel().filter (1, svfBand, 600, 60).filter (2, svfBand, 1000, 75).lfo (1, drift, 0.06f, 0)
            .mod (lfo1, "filter1Cutoff", 6).mod (lfo1, "filter2Cutoff", -2).amp (1900, 1000, 90, 1900).reverb (45, 95, 7.5f, 70).level (-6)
            .macro (1, "Mouth").mod (macro1, "filter1Cutoff", 12).macro (2, "Vowel").mod (macro2, "filter2Cutoff", -12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Floating Murk", "Pad").osc (1, pulse, 35).unison (1, 3, 8).osc (2, pulse, 30, 65).pitch (2, -12, 5).lfo (1, triangle, 0.47f, 0)
            .mod (lfo1, "osc1Position", 30).lfo (2, sine, 0.051f, 0).mod (lfo2, "osc2Position", 12).filter (1, ladder24, 1300, 15)
            .amp (1900, 1000, 95, 2100).reverb (55, 90, 8.5f, 50).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Width").mod (macro2, "osc1Position", 30).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Flooded Rime", "Pad").osc (1, analog, sinePos, 30).pitch (1, 12).osc (2, analog, sinePos).warp (2, wFm, 15).unison (2, 4, 8, 70)
            .osc (3, analog, trianglePos, 50).pitch (3, -12).unison (3, 2, 8, 100).lfo (1, drift, 0.2f, 0).mod (lfo1, "osc2WarpAmount", 25)
            .filter (1, svfLow, 2800, 20).amp (700, 1000, 90, 3400).reverb (45, 85, 9, 70).level (-17).macro (1, "Index")
            .mod (macro1, "osc2WarpAmount", 35).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).delay (eighthD, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).chorus (0, 0.5f, 50).macro (4, "Wide").mod (macro4, "fxChorusMix", 40);
        add ("Frosted Glaze", "Pad").osc (1, analog, sinePos, 30).pitch (1, 19).osc (2, analog, sinePos).warp (2, wFm, 10).unison (2, 3, 14, 70)
            .osc (3, analog, trianglePos, 60).pitch (3, 7).unison (3, 2, 12, 100).lfo (1, drift, 0.073f, 0).mod (lfo1, "osc2WarpAmount", 16)
            .filter (1, svfLow, 7000).amp (2100, 1000, 90, 4200).chorus (45, 0.2f, 50).reverb (35, 80, 7.5f, 45).level (-14).macro (1, "Index")
            .mod (macro1, "osc2WarpAmount", 35).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).delay (eighthD, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Wide").mod (macro4, "fxChorusMix", 40);
        add ("Ghost Gust", "Pad").set ("osc1On", 0).noise (50, 0, 40).osc (2, analog, 40, 45).unison (2, 3, 6, 100).route (2, 100).parallel()
            .filter (1, ladderBand, 523.3f, 60, 0, 100).filter (2, ladder12, 1400).lfo (1, drift, 0.27f, 0).mod (lfo1, "filter1Cutoff", 5)
            .mod (lfo1, "noiseLevel", 15).amp (2400, 1000, 95, 2400).reverb (55, 70, 7, 40).level (-1).macro (1, "Air")
            .mod (macro1, "noiseLevel", 30).macro (2, "Tone").mod (macro2, "filter2Cutoff", 25).macro (3, "Motion").mod (macro3, "lfo1Rate", 30)
            .macro (4, "Whistle").mod (macro4, "filter1Resonance", 14);
        add ("Grave Wake", "Pad").osc (1, additive, 20).unison (1, 4, 12, 100).osc (2, additive, 20, 65).pitch (2, 7, 6).unison (2, 2, 6, 100)
            .osc (3, analog, 30, 60).pitch (3, -12, 3).filter (1, ladder24, 2700, 15).lfo (1, drift, 0.092f, 0).mod (lfo1, "filter1Cutoff", 6)
            .amp (600, 1000, 95, 1800).reverb (50, 70, 4.5f, 50).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Fifth").mod (macro2, "osc2Volume", -50).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Haunted Requiem", "Pad").osc (1, analog, sawPos).unison (1, 5, 12, 90).route (1, 50).parallel().filter (1, svfBand, 300, 65)
            .filter (2, svfBand, 2250, 75).lfo (1, drift, 0.17f, 0).mod (lfo1, "filter1Cutoff", 2).mod (lfo1, "filter2Cutoff", -3)
            .amp (600, 1000, 85, 2100).chorus (40, 0.6f, 50).reverb (45, 75, 6, 30).level (1).macro (1, "Mouth").mod (macro1, "filter1Cutoff", 12)
            .macro (2, "Vowel").mod (macro2, "filter2Cutoff", -12).macro (3, "Motion").mod (macro3, "lfo1Rate", 30).drive (tube, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Lightless Gloom", "Pad").osc (1, analog, sawPos).unison (1, 5, 18, 70).osc (2, analog, squarePos, 55).pitch (2, -12, 7)
            .filter (1, ladder24, 510, 15, 30).lfo (1, sine, 0.059f, 0).mod (lfo1, "filter1Cutoff", 20).noise (10, 1).amp (1900, 1000, 100, 2100)
            .reverb (50, 95, 4.5f, 70).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 35).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Listing Scale", "Pad").osc (1, sync, 60).unison (1, 3, 18, 90).lfo (1, sine, 0.14f, 0).mod (lfo1, "osc1Position", 30)
            .filter (1, svfLow, 1900, 15, 20).amp (2200, 1000, 95, 3600).drive (hard, 45, 3700).reverb (40, 75, 8, 50).level (-22)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Growl").mod (macro2, "osc1Position", 35).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 40).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Manganese Sheet", "Pad").osc (1, metal, 45).unison (1, 3, 12).osc (2, analog, 30, 50).pitch (2, -12, 6).unison (2, 2, 10, 100)
            .lfo (1, triangle, 0.054f, 0).mod (lfo1, "osc1Position", 10).filter (1, svfLow, 4100, 20).filter (2, svfHigh, 250)
            .amp (1200, 1000, 95, 4500).reverb (45, 75, 8, 70).level (-6).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Partials").mod (macro2, "osc1Position", 35).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Mellow Glaze", "Pad").osc (1, analog, sinePos, 30).pitch (1, 24).osc (2, analog, sinePos).warp (2, wFm, 15).unison (2, 2, 10, 100)
            .osc (3, analog, sinePos, 60).pitch (3, -12).unison (3, 2, 8, 100).lfo (1, sine, 0.34f, 0).mod (lfo1, "osc2WarpAmount", 20)
            .filter (1, ladder12, 2200, 20).amp (2000, 1000, 90, 3600).chorus (40, 0.5f, 50).reverb (40, 85, 3.5f, 60).level (-13)
            .macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).macro (3, "Wide")
            .mod (macro3, "fxChorusMix", 40).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Muffled Umbra", "Pad").osc (1, analog, sawPos).unison (1, 3, 22, 100).osc (2, analog, trianglePos, 60).pitch (2, 7, 9)
            .filter (1, ladder24, 630, 45).lfo (1, sine, 0.047f, 0).mod (lfo1, "filter1Cutoff", 14).amp (500, 1000, 90, 1800)
            .reverb (30, 100, 3, 55).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 35).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Nickel Plate", "Pad").osc (1, metal, 45).unison (1, 6, 20, 100).osc (2, vowel, 0, 50).pitch (2, -12, 4).unison (2, 2, 8, 100)
            .lfo (1, drift, 0.17f, 0).mod (lfo1, "osc1Position", 20).filter (1, svfLow, 1900, 30).amp (2400, 1000, 90, 2000)
            .reverb (40, 85, 3.5f, 40).level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Partials")
            .mod (macro2, "osc1Position", 35).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Nuclear Rite", "Pad").osc (1, vowel, 60).unison (1, 3, 12).osc (2, vowel, 60, 75).pitch (2, 7, 5).unison (2, 2, 12, 100)
            .osc (3, additive, 20, 55).pitch (3, 12, 8).filter (1, ladder24, 790, 20).lfo (1, triangle, 0.13f, 0).mod (lfo1, "filter1Cutoff", 14)
            .amp (700, 1000, 85, 2700).chorus (45, 0.5f, 50).reverb (30, 80, 7, 35).level (-6).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Fifth").mod (macro2, "osc2Volume", -50).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Overloaded Rust", "Pad").osc (1, sync, 50).unison (1, 3, 18).osc (2, analog, 60, 60).pitch (2, 12, 9).lfo (1, triangle, 0.31f, 0)
            .mod (lfo1, "osc1Position", 10).filter (1, ladderBand, 1700, 25, 40).amp (600, 1000, 85, 3200).drive (crush, 40, 5400)
            .reverb (45, 95, 7, 35).level (-10).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Growl")
            .mod (macro2, "osc1Position", 35).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Patient Lustre", "Pad").osc (1, analog, sinePos, 1).pitch (1, 24).osc (2, analog, sinePos).warp (2, wFm, 15).unison (2, 4, 14, 70)
            .lfo (1, drift, 0.057f, 0).mod (lfo1, "osc2WarpAmount", 22).filter (1, svfLow, 2300, 15).amp (500, 1000, 95, 3300)
            .reverb (45, 85, 6, 35).level (-16).macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Patient Sheet", "Pad").osc (1, metal, 30).unison (1, 6, 14).osc (2, additive, 20, 55).pitch (2, 0, 7).unison (2, 2, 6, 100)
            .lfo (1, drift, 0.16f, 0).mod (lfo1, "osc1Position", 20).filter (1, ladder12, 4300).filter (2, svfHigh, 510).amp (800, 1000, 85, 4300)
            .reverb (40, 95, 9, 45).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Partials")
            .mod (macro2, "osc1Position", 35).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Remote Haze", "Pad").osc (1, pulse, 45).unison (1, 3, 8).osc (2, pulse, 30, 55).pitch (2, 0, 6).lfo (1, triangle, 0.29f, 0)
            .mod (lfo1, "osc1Position", 30).lfo (2, sine, 0.1f, 0).mod (lfo2, "osc2Position", 25).filter (1, ladder12, 3400, 20)
            .amp (500, 1000, 95, 3200).reverb (55, 90, 3.5f, 65).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Width").mod (macro2, "osc1Position", 30).macro (3, "Motion").mod (macro3, "lfo1Rate", 30).drive (tube, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Scarred Dusk", "Pad").osc (1, analog, sawPos).unison (1, 6, 16, 70).osc (2, analog, squarePos, 70).pitch (2, 0, 8)
            .filter (1, ladder24, 350, 35, 30).lfo (1, drift, 0.12f, 0).mod (lfo1, "filter1Cutoff", 8).amp (600, 1000, 85, 2200)
            .chorus (40, 0.7f, 50).reverb (55, 100, 7, 30).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 35).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Severe Corrosion", "Pad").osc (1, growl, 45).unison (1, 5, 12, 70).osc (2, analog, 40, 55).pitch (2, 12, 5).lfo (1, sine, 0.23f, 0)
            .mod (lfo1, "osc1Position", 25).filter (1, ladderBand, 960, 40, 40).amp (1300, 1000, 100, 2600).drive (crush, 40, 2600)
            .reverb (40, 80, 3.5f, 45).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Growl")
            .mod (macro2, "osc1Position", 35).macro (3, "Grit").mod (macro3, "fxDriveAmount", 40).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Severe Vesper", "Pad").osc (1, analog, sawPos).unison (1, 6, 20, 90).route (1, 50).parallel().filter (1, svfBand, 500, 65)
            .filter (2, svfBand, 1500, 55).lfo (1, sine, 0.29f, 0).mod (lfo1, "filter1Cutoff", 2).mod (lfo1, "filter2Cutoff", -4)
            .amp (500, 1000, 100, 2000).chorus (40, 0.7f, 50).reverb (40, 100, 5.5f, 70).level (-5).macro (1, "Mouth")
            .mod (macro1, "filter1Cutoff", 12).macro (2, "Vowel").mod (macro2, "filter2Cutoff", -12).drive (tube, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Shuttered Patina", "Pad").osc (1, sync, 60).unison (1, 4, 10, 90).osc (2, analog, 30, 45).pitch (2, 7, 7).lfo (1, sine, 0.15f, 0)
            .mod (lfo1, "osc1Position", 25).filter (1, ladder24, 2200, 35, 60).amp (1000, 1000, 100, 2000).drive (hard, 20, 3400)
            .reverb (55, 85, 4, 40).level (-17).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Growl")
            .mod (macro2, "osc1Position", 35).macro (3, "Motion").mod (macro3, "lfo1Rate", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Sleepy Mist", "Pad").osc (1, pulse, 50).unison (1, 3, 10, 90).osc (2, pulse, 30, 75).pitch (2, -12, 8).lfo (1, triangle, 0.12f, 0)
            .mod (lfo1, "osc1Position", 15).lfo (2, sine, 0.38f, 0).mod (lfo2, "osc2Position", 22).filter (1, ladder24, 2300, 20)
            .amp (2100, 1000, 85, 2500).reverb (30, 70, 3.5f, 45).level (-15).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Width").mod (macro2, "osc1Position", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Smooth Fifth", "Pad").osc (1, pulse, 20).unison (1, 3, 16, 90).osc (2, pulse, 20, 65).pitch (2, 7, 1).unison (2, 2, 12, 100)
            .osc (3, additive, 50, 45).pitch (3, 12, 7).filter (1, svfLow, 790).lfo (1, drift, 0.16f, 0).mod (lfo1, "filter1Cutoff", 14)
            .amp (1800, 1000, 90, 1500).reverb (35, 80, 3, 30).level (-15).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Fifth")
            .mod (macro2, "osc2Volume", -50).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Spectral Fifth", "Pad").osc (1, vowel, 70).unison (1, 4, 14, 70).osc (2, vowel, 70, 65).pitch (2, 7, 5).unison (2, 2, 6, 100)
            .osc (3, additive, 0, 60).pitch (3, -12, 6).filter (1, ladder24, 1300, 15).lfo (1, triangle, 0.059f, 0).mod (lfo1, "filter1Cutoff", 11)
            .amp (1200, 1000, 85, 1900).chorus (35, 0.2f, 50).reverb (35, 85, 5.5f, 30).level (-9).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Fifth").mod (macro2, "osc2Volume", -50).macro (3, "Motion").mod (macro3, "lfo1Rate", 30)
            .drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Sprawling Fifth", "Pad").osc (1, analog, sawPos).unison (1, 3, 10).osc (2, analog, sawPos, 75).pitch (2, 7, 1).unison (2, 2, 14, 100)
            .osc (3, additive, 40, 45).pitch (3, -12, 4).filter (1, ladder24, 910, 20).lfo (1, triangle, 0.24f, 0).mod (lfo1, "filter1Cutoff", 13)
            .amp (1200, 1000, 100, 4400).chorus (40, 0.4f, 50).reverb (40, 75, 7, 65).level (-11).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Fifth").mod (macro2, "osc2Volume", -50).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Stagnant Membrane", "Pad").osc (1, metal, 25).unison (1, 4, 18, 90).osc (2, additive, 0, 45).pitch (2, -12, 6).unison (2, 2, 6, 100)
            .lfo (1, triangle, 0.036f, 0).mod (lfo1, "osc1Position", 10).filter (1, svfBand, 2500).amp (500, 1000, 90, 3700).phaser (35, 0.12f, 65)
            .reverb (35, 85, 8.5f, 55).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Partials")
            .mod (macro2, "osc1Position", 35).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Sublevel Haze", "Pad").osc (1, pulse, 50).unison (1, 2, 14, 70).osc (2, pulse, 60, 75).pitch (2, -12, 7).lfo (1, triangle, 0.19f, 0)
            .mod (lfo1, "osc1Position", 15).lfo (2, sine, 0.31f, 0).mod (lfo2, "osc2Position", 20).filter (1, ladder24, 3200, 30)
            .amp (800, 1000, 85, 3100).chorus (40, 0.4f, 50).reverb (35, 90, 7, 35).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Width").mod (macro2, "osc1Position", 30).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Submerged Gust", "Pad").set ("osc1On", 0).noise (60, 1, 20).osc (2, analog, 40, 60).unison (2, 3, 10, 100).route (2, 100).parallel()
            .filter (1, ladderBand, 261.6f, 85, 0, 100).filter (2, svfLow, 2100).lfo (1, sine, 0.12f, 0).mod (lfo1, "filter1Cutoff", 3)
            .mod (lfo1, "noiseLevel", 10).amp (500, 1000, 95, 2000).reverb (40, 70, 3, 65).level (-11).macro (1, "Air")
            .mod (macro1, "noiseLevel", 30).macro (2, "Tone").mod (macro2, "filter2Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Trailing Gleam", "Pad").osc (1, analog, sinePos, 1).pitch (1, 7).osc (2, analog, sinePos).warp (2, wFm, 20).unison (2, 4, 12, 100)
            .lfo (1, drift, 0.074f, 0).mod (lfo1, "osc2WarpAmount", 17).filter (1, svfLow, 3300, 20).amp (2100, 1000, 100, 1700)
            .reverb (40, 80, 7, 65).level (-15).macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Damp")
            .mod (macro2, "filter1Cutoff", -30).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 40).delay (eighthD, 40, 0, 4500)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Trailing Whisper", "Pad").set ("osc1On", 0).noise (50, 1, 40).osc (2, analog, 30, 50).unison (2, 3, 10, 100).route (2, 100).parallel()
            .filter (1, ladderBand, 261.6f, 80, 0, 100).filter (2, ladder12, 3100).lfo (1, drift, 0.14f, 0).mod (lfo1, "filter1Cutoff", 3)
            .mod (lfo1, "noiseLevel", 5).amp (1300, 1000, 100, 1900).reverb (50, 70, 6, 60).level (-6).macro (1, "Air")
            .mod (macro1, "noiseLevel", 30).macro (2, "Tone").mod (macro2, "filter2Cutoff", 25).macro (3, "Motion").mod (macro3, "lfo1Rate", 30)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Twisted Requiem", "Pad").osc (1, analog, sawPos).unison (1, 4, 20, 90).route (1, 50).parallel().filter (1, svfBand, 800, 75)
            .filter (2, svfBand, 1150, 65).lfo (1, sine, 0.21f, 0).mod (lfo1, "filter1Cutoff", 3).mod (lfo1, "filter2Cutoff", -6)
            .amp (1600, 1000, 90, 3300).chorus (35, 0.7f, 50).reverb (50, 90, 7, 40).level (-4).macro (1, "Mouth").mod (macro1, "filter1Cutoff", 12)
            .macro (2, "Vowel").mod (macro2, "filter2Cutoff", -12).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Vagrant Dim", "Pad").osc (1, analog, sawPos).unison (1, 5, 18, 90).osc (2, analog, squarePos, 60).pitch (2, -12, 5)
            .filter (1, ladder24, 370, 45, 10).lfo (1, drift, 0.039f, 0).mod (lfo1, "filter1Cutoff", 16).amp (500, 1000, 100, 2000)
            .reverb (35, 95, 8, 30).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 35).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Veiled Murk", "Pad").osc (1, pulse, 50).unison (1, 3, 18).osc (2, pulse, 50, 70).pitch (2, 12, 10).lfo (1, triangle, 0.093f, 0)
            .mod (lfo1, "osc1Position", 15).lfo (2, sine, 0.26f, 0).mod (lfo2, "osc2Position", 13).filter (1, ladder24, 2700, 30)
            .amp (1900, 1000, 90, 1800).reverb (45, 80, 8.5f, 45).level (-15).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Width").mod (macro2, "osc1Position", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Voiceless Eclipse", "Pad").osc (1, analog, sawPos).unison (1, 6, 12, 90).osc (2, analog, sawPos, 50).pitch (2, -12, 10)
            .filter (1, ladder24, 660, 25).lfo (1, drift, 0.17f, 0).mod (lfo1, "filter1Cutoff", 10).noise (14, 1, -60).amp (500, 1000, 90, 2000)
            .reverb (50, 85, 4, 70).level (-10).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 35).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Weary Gleam", "Pad").osc (1, analog, sinePos, 1).pitch (1, 7).osc (2, analog, sinePos).warp (2, wFm, 20).unison (2, 4, 14, 70)
            .osc (3, analog, sinePos, 60).pitch (3, 7).unison (3, 2, 6, 100).lfo (1, sine, 0.33f, 0).mod (lfo1, "osc2WarpAmount", 24)
            .filter (1, svfLow, 3100, 20).amp (2400, 1000, 100, 2600).reverb (40, 95, 8, 60).level (-16).macro (1, "Index")
            .mod (macro1, "osc2WarpAmount", 35).macro (2, "Damp").mod (macro2, "filter1Cutoff", -30).chorus (0, 0.5f, 50).macro (3, "Wide")
            .mod (macro3, "fxChorusMix", 40).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Western Gloom", "Pad").osc (1, analog, sawPos).unison (1, 6, 20, 100).osc (2, analog, squarePos, 60).pitch (2, 0, 5)
            .filter (1, ladder24, 410, 40, 10).lfo (1, drift, 0.051f, 0).mod (lfo1, "filter1Cutoff", 8).noise (16, 1, -20).amp (600, 1000, 85, 3100)
            .chorus (20, 0.7f, 50).reverb (45, 80, 6, 40).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 35).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Wordless Sigh", "Pad").set ("osc1On", 0).noise (50, 0, 20).osc (2, analog, 20, 65).unison (2, 3, 8, 100).route (2, 100).parallel()
            .filter (1, svfBand, 261.6f, 55, 0, 100).filter (2, ladder12, 1000).lfo (1, drift, 0.38f, 0).mod (lfo1, "filter1Cutoff", 6)
            .mod (lfo1, "noiseLevel", 10).amp (700, 1000, 95, 3900).reverb (45, 90, 7, 40).level (-9).macro (1, "Air")
            .mod (macro1, "noiseLevel", 30).macro (2, "Tone").mod (macro2, "filter2Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Wrecked Gloom", "Pad").osc (1, analog, sawPos).unison (1, 6, 22, 90).osc (2, analog, squarePos, 60).pitch (2, 12, 5)
            .filter (1, ladder24, 370, 45).lfo (1, sine, 0.072f, 0).mod (lfo1, "filter1Cutoff", 18).noise (10, 1, -60).amp (500, 1000, 85, 4300)
            .chorus (30, 0.7f, 50).reverb (35, 85, 4.5f, 65).level (-10).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 35).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);

        // Drone, by name.
        add ("Buckled Pile", "Drone").osc (1, analog, sinePos, 25).pitch (1, 17, 20).osc (2, analog, trianglePos).warp (2, wFm, 35)
            .unison (2, 3, 10, 100, 0, 40).lfo (1, sine, 0.064f, 0).mod (lfo1, "osc2WarpAmount", 30).lfo (2, triangle, 0.026f, 0)
            .mod (lfo2, "filter1Cutoff", 14).filter (1, ladderBand, 2200, 35, 40).subOsc (20).amp (1700, 1000, 100, 3000).drive (hard, 15, 6200)
            .reverb (35, 90, 9, 55).level (-16).macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Cavernous Throat", "Drone").osc (1, vowel, 40).unison (1, 4, 14, 100, 0, 30).osc (2, analog, 40, 45).pitch (2, 0, 4)
            .lfo (1, drift, 0.15f, 0).mod (lfo1, "osc1Position", 25).lfo (2, sine, 0.043f, 0).mod (lfo2, "filter1Cutoff", 14)
            .filter (1, ladderBand, 1300, 30, 30).amp (2800, 1000, 100, 4500).reverb (40, 95, 9, 30).level (-5).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).drive (tube, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Cavernous Vigil", "Drone").osc (1, analog, trianglePos).unison (1, 3, 7, 100, 0, 30).osc (2, analog, trianglePos, 75).pitch (2, 12, 6)
            .osc (3, additive, 10, 45).pitch (3, 12, -3).filter (1, ladder24, 1600, 20).lfo (1, drift, 0.031f, 0).mod (lfo1, "osc3Volume", 25)
            .amp (3100, 1000, 100, 5000).reverb (50, 75, 9, 70).level (-13).macro (1, "Beat").mod (macro1, "osc1UnisonDetune", 25)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Charred Flue", "Drone").osc (1, additive, 70, 60).noise (60, 0, -20).filter (1, comb, 392, 95, 0, 100).filter (2, svfLow, 2200, 20)
            .lfo (1, drift, 0.045f, 0).mod (lfo1, "filter2Cutoff", 7).mod (lfo1, "noiseLevel", 10).amp (1600, 1000, 100, 4000)
            .reverb (45, 90, 5, 55).level (-9).macro (1, "Blow").mod (macro1, "noiseLevel", 25).macro (2, "Cutoff")
            .mod (macro2, "filter2Cutoff", 25).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Chemical Culvert", "Drone").osc (1, pulse, 70, 60).noise (60, 1, 20).filter (1, comb, 523.3f, 90, 0, 100)
            .filter (2, ladder24, 1600, 20).lfo (1, drift, 0.046f, 0).mod (lfo1, "filter2Cutoff", 15).mod (lfo1, "noiseLevel", 15)
            .amp (2500, 1000, 100, 3500).reverb (40, 95, 4.5f, 35).level (-9).macro (1, "Blow").mod (macro1, "noiseLevel", 25).macro (2, "Cutoff")
            .mod (macro2, "filter2Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Chromium Alternator", "Drone").osc (1, pulse, 75).osc (2, additive, 25, 60).pitch (2, 0, 3).lfo (1, sine, 17, 0)
            .mod (lfo1, "ampLevel", 9).mod (lfo1, "filter1Cutoff", 1).lfo (2, drift, 0.1f, 0).mod (lfo2, "lfo1Rate", 7)
            .mod (lfo2, "osc1Position", 15).filter (1, comb, 810, 25, 20).subOsc (35).amp (800, 1000, 100, 1500).drive (tube, 40, 2100)
            .reverb (35, 75, 6, 40).level (-26).macro (1, "Speed").mod (macro1, "lfo1Rate", 20).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Hum").mod (macro4, "osc2Volume", 25);
        add ("Clipped Kiln", "Drone").osc (1, analog, sinePos, 1).pitch (1, 17, 25).osc (2, analog, sinePos).warp (2, wFm, 35)
            .unison (2, 1, 8, 100, 0, 30).lfo (1, sine, 0.026f, 0).mod (lfo1, "osc2WarpAmount", 10).lfo (2, triangle, 0.042f, 0)
            .mod (lfo2, "filter1Cutoff", 9).filter (1, ladderBand, 1200, 40, 10).subOsc (45).amp (1500, 1000, 100, 3000).drive (hard, 40, 6900)
            .reverb (40, 100, 6, 60).level (-28).macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Coal Menace", "Drone").osc (1, analog, 80).unison (1, 4, 16, 100, 0, 50).osc (2, analog, 80, 55).pitch (2, 1, 7)
            .osc (3, analog, 80, 45).pitch (3, 6, -3).filter (1, ladder24, 490, 30, 10).lfo (1, drift, 0.11f, 0).mod (lfo1, "filter1Cutoff", 12)
            .mod (lfo1, "osc2Volume", -25).amp (2000, 1000, 100, 6000).reverb (50, 85, 7, 30).level (-12).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Semitone").mod (macro2, "osc2Volume", -50).drive (tube, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Coal Torpor", "Drone").osc (1, analog, sinePos).unison (1, 3, 3, 100, 0, 40).osc (2, analog, sinePos, 70).pitch (2, -12, 7)
            .osc (3, analog, 10, 45).pitch (3, 12, -4).filter (1, ladder24, 620).lfo (1, sine, 0.045f, 0).mod (lfo1, "osc3Volume", 25)
            .amp (3000, 1000, 100, 4500).reverb (45, 100, 7, 55).level (-13).macro (1, "Beat").mod (macro1, "osc1UnisonDetune", 25)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "High")
            .mod (macro4, "osc3Volume", 30);
        add ("Coastal Idler", "Drone").osc (1, pulse, 85).osc (2, additive, 15, 75).pitch (2, 7, 4).lfo (1, triangle, 20, 0)
            .mod (lfo1, "ampLevel", 8).mod (lfo1, "filter1Cutoff", 4).lfo (2, drift, 0.1f, 0).mod (lfo2, "lfo1Rate", 9)
            .mod (lfo2, "osc1Position", 10).filter (1, ladderBand, 130, 45, 30).subOsc (35).amp (700, 1000, 100, 3000).drive (tube, 40, 3100)
            .reverb (30, 85, 7, 35).level (-26).macro (1, "Speed").mod (macro1, "lfo1Rate", 20).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Hum").mod (macro3, "osc2Volume", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Daybreak Kiln", "Drone").osc (1, analog, sinePos, 25).pitch (1, 21, -15).osc (2, analog, trianglePos).warp (2, wFm, 25)
            .unison (2, 2, 12, 100, 0, 10).lfo (1, drift, 0.046f, 0).mod (lfo1, "osc2WarpAmount", 15).lfo (2, triangle, 0.044f, 0)
            .mod (lfo2, "filter1Cutoff", 5).filter (1, ladderBand, 590, 40, 10).subOsc (35).amp (2500, 1000, 100, 5000).reverb (30, 75, 9, 30)
            .level (-9).macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25)
            .macro (3, "Motion").mod (macro3, "lfo1Rate", 30).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Drifting Bore", "Drone").osc (1, analog, 40, 55).noise (75, 1, -40).filter (1, comb, 392, 91, 0, 100).filter (2, svfLow, 3900, 25)
            .lfo (1, drift, 0.11f, 0).mod (lfo1, "filter2Cutoff", 5).mod (lfo1, "noiseLevel", 5).amp (3700, 1000, 100, 5500)
            .reverb (50, 95, 3.5f, 60).level (0).macro (1, "Blow").mod (macro1, "noiseLevel", 25).macro (2, "Cutoff")
            .mod (macro2, "filter2Cutoff", 25).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Dusky Pile", "Drone").osc (1, analog, sinePos, 25).pitch (1, 5, -20).osc (2, analog, trianglePos).warp (2, wFm, 15)
            .unison (2, 3, 10, 100, 0, 30).lfo (1, drift, 0.1f, 0).mod (lfo1, "osc2WarpAmount", 10).lfo (2, triangle, 0.04f, 0)
            .mod (lfo2, "filter1Cutoff", 9).filter (1, svfLow, 1000, 45).subOsc (40).amp (2100, 1000, 100, 5500).drive (hard, 15, 3600)
            .reverb (45, 70, 5, 35).level (-19).macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Eastern Kiln", "Drone").osc (1, analog, sinePos, 25).pitch (1, 14, 25).osc (2, analog, trianglePos).warp (2, wFm, 15)
            .unison (2, 1, 11, 100, 0, 40).lfo (1, drift, 0.14f, 0).mod (lfo1, "osc2WarpAmount", 25).lfo (2, sine, 0.1f, 0)
            .mod (lfo2, "filter1Cutoff", 9).filter (1, ladderBand, 2800, 10, 10).subOsc (40).amp (1600, 1000, 100, 6000).reverb (45, 75, 8, 45)
            .level (-4).macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Encoded Generator", "Drone").osc (1, pulse, 55).osc (2, additive, 30, 55).pitch (2, 7, 5).lfo (1, sine, 23, 0)
            .mod (lfo1, "ampLevel", 3).mod (lfo1, "filter1Cutoff", 2).lfo (2, drift, 0.22f, 0).mod (lfo2, "lfo1Rate", 4)
            .mod (lfo2, "osc1Position", 10).filter (1, ladderBand, 470, 60, 10).subOsc (45).amp (2300, 1000, 100, 4000).drive (crush, 35, 3500)
            .reverb (25, 100, 6, 50).level (-12).macro (1, "Speed").mod (macro1, "lfo1Rate", 20).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Endless Threat", "Drone").osc (1, analog, 80).unison (1, 3, 16, 100, 0, 40).osc (2, analog, 80, 75).pitch (2, -11, 6)
            .osc (3, analog, 80, 45).pitch (3, -6, -1).filter (1, svfLow, 1400, 15, 20).lfo (1, sine, 0.06f, 0).mod (lfo1, "filter1Cutoff", 8)
            .mod (lfo1, "osc2Volume", -20).amp (2600, 1000, 100, 6000).reverb (40, 70, 6, 65).level (-14).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Semitone").mod (macro2, "osc2Volume", -50).macro (3, "Motion")
            .mod (macro3, "lfo1Rate", 30).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Entombed Generator", "Drone").osc (1, pulse, 65).osc (2, additive, 5, 75).pitch (2, 7, 6).lfo (1, triangle, 8.3f, 0)
            .mod (lfo1, "ampLevel", 9).mod (lfo1, "filter1Cutoff", 3).lfo (2, drift, 0.11f, 0).mod (lfo2, "lfo1Rate", 6)
            .mod (lfo2, "osc1Position", 5).filter (1, ladder24, 510, 40, 10).subOsc (25).amp (1000, 1000, 100, 2500).drive (crush, 35, 3500)
            .reverb (40, 75, 3.5f, 55).level (-16).macro (1, "Speed").mod (macro1, "lfo1Rate", 20).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Exiled Unease", "Drone").osc (1, growl, 20).unison (1, 3, 8, 100, 0, 50).osc (2, growl, 20, 75).pitch (2, -11, 6)
            .osc (3, growl, 20, 50).pitch (3, -6, -6).filter (1, svfLow, 530, 15, 40).lfo (1, drift, 0.084f, 0).mod (lfo1, "filter1Cutoff", 6)
            .mod (lfo1, "osc2Volume", -10).amp (3000, 1000, 100, 5500).reverb (45, 75, 7, 55).level (-14).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Semitone").mod (macro2, "osc2Volume", -50).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Faint Monk", "Drone").osc (1, vowel, 50).unison (1, 4, 11, 100, 0, 10).osc (2, growl, 70, 50).pitch (2, -12, 3)
            .lfo (1, sine, 0.16f, 0).mod (lfo1, "osc1Position", 35).lfo (2, triangle, 0.067f, 0).mod (lfo2, "filter1Cutoff", 16)
            .filter (1, ladderBand, 990, 35).amp (2000, 1000, 100, 3500).reverb (30, 100, 7.5f, 70).level (-8).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Floating Foreboding", "Drone").osc (1, growl, 60).unison (1, 4, 12, 100, 0, 40).osc (2, growl, 60, 75).pitch (2, 1, 5)
            .osc (3, growl, 60, 50).pitch (3, 6, -7).filter (1, ladder24, 450, 20).lfo (1, sine, 0.078f, 0).mod (lfo1, "filter1Cutoff", 16)
            .mod (lfo1, "osc2Volume", -15).amp (3000, 1000, 100, 5000).reverb (55, 90, 9, 40).level (-12).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Semitone").mod (macro2, "osc2Volume", -50).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Floating Hermit", "Drone").osc (1, vowel, 20).unison (1, 2, 13, 100, 0, 10).osc (2, analog, 50, 40).pitch (2, 0, 7)
            .lfo (1, drift, 0.11f, 0).mod (lfo1, "osc1Position", 40).lfo (2, sine, 0.046f, 0).mod (lfo2, "filter1Cutoff", 6)
            .filter (1, ladderBand, 480, 40, 30).amp (3500, 1000, 100, 4500).reverb (45, 90, 3, 60).level (-4).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 25);
        add ("Forged Bore", "Drone").osc (1, pulse, 70, 50).noise (90, 1, -20).filter (1, comb, 392, 90, 0, 100).filter (2, svfLow, 1300, 35)
            .lfo (1, drift, 0.14f, 0).mod (lfo1, "filter2Cutoff", 10).mod (lfo1, "noiseLevel", 15).amp (3000, 1000, 100, 4000)
            .reverb (30, 100, 4.5f, 35).level (-4).macro (1, "Blow").mod (macro1, "noiseLevel", 25).macro (2, "Cutoff")
            .mod (macro2, "filter2Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Greased Shaft", "Drone").osc (1, pulse, 70, 50).noise (70, 1, 20).filter (1, comb, 261.6f, 92, 0, 100).filter (2, svfLow, 3000, 35)
            .lfo (1, drift, 0.055f, 0).mod (lfo1, "filter2Cutoff", 14).mod (lfo1, "noiseLevel", 10).amp (2600, 1000, 100, 6000)
            .reverb (40, 75, 7, 35).level (-16).macro (1, "Blow").mod (macro1, "noiseLevel", 25).macro (2, "Cutoff")
            .mod (macro2, "filter2Cutoff", 25).macro (3, "Motion").mod (macro3, "lfo1Rate", 30).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Hexed Shaft", "Drone").osc (1, analog, sawPos, 50).noise (70, 0, 20).filter (1, comb, 261.6f, 93, 0, 100)
            .filter (2, ladder24, 880, 30).lfo (1, drift, 0.059f, 0).mod (lfo1, "filter2Cutoff", 12).mod (lfo1, "noiseLevel", 5)
            .amp (2200, 1000, 100, 4000).reverb (40, 75, 4.5f, 55).level (-15).macro (1, "Blow").mod (macro1, "noiseLevel", 25).macro (2, "Cutoff")
            .mod (macro2, "filter2Cutoff", 25).macro (3, "Motion").mod (macro3, "lfo1Rate", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Jade Watch", "Drone").osc (1, analog, sinePos).unison (1, 3, 5, 100, 0, 40).osc (2, analog, sinePos, 60).pitch (2, 7, 4)
            .osc (3, analog, 30, 60).pitch (3, 7, -8).filter (1, ladder24, 1100, 20).lfo (1, drift, 0.13f, 0).mod (lfo1, "osc3Volume", 15)
            .amp (2600, 1000, 100, 5500).reverb (50, 80, 3, 45).level (-13).macro (1, "Beat").mod (macro1, "osc1UnisonDetune", 25)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "High")
            .mod (macro4, "osc3Volume", 30);
        add ("Languid Elder", "Drone").osc (1, vowel, 10).unison (1, 4, 9, 100, 0, 30).osc (2, growl, 50, 40).pitch (2, -12, 6)
            .lfo (1, drift, 0.077f, 0).mod (lfo1, "osc1Position", 40).lfo (2, triangle, 0.1f, 0).mod (lfo2, "filter1Cutoff", 14)
            .filter (1, ladderBand, 820, 45, 10).amp (3400, 1000, 100, 4000).reverb (40, 85, 5, 30).level (-12).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 25);
        add ("Lightless Malice", "Drone").osc (1, growl, 60).unison (1, 4, 8, 100, 0, 20).osc (2, growl, 60, 55).pitch (2, 13)
            .osc (3, growl, 60, 60).pitch (3, 11, -1).filter (1, svfLow, 1200, 25, 10).lfo (1, drift, 0.075f, 0).mod (lfo1, "filter1Cutoff", 6)
            .mod (lfo1, "osc2Volume", -10).amp (3700, 1000, 100, 4000).reverb (35, 75, 3, 65).level (-9).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Semitone").mod (macro2, "osc2Volume", -50).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Lost Core", "Drone").osc (1, analog, sinePos, 1).pitch (1, 17, -15).osc (2, analog, sinePos).warp (2, wFm, 10)
            .unison (2, 1, 12, 100, 0, 10).lfo (1, sine, 0.064f, 0).mod (lfo1, "osc2WarpAmount", 20).lfo (2, sine, 0.034f, 0)
            .mod (lfo2, "filter1Cutoff", 13).filter (1, ladder24, 510, 40, 20).subOsc (30).amp (3500, 1000, 100, 6000).reverb (35, 85, 8.5f, 50)
            .level (-11).macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25)
            .macro (3, "Sub").mod (macro3, "subLevel", 35).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Murky Hermit", "Drone").osc (1, vowel, 0).unison (1, 2, 7, 100, 0, 40).osc (2, analog, 50, 40).pitch (2, 7, 4).lfo (1, drift, 0.1f, 0)
            .mod (lfo1, "osc1Position", 35).lfo (2, sine, 0.093f, 0).mod (lfo2, "filter1Cutoff", 6).filter (1, svfBand, 1100, 30)
            .amp (3800, 1000, 100, 5500).reverb (50, 70, 3.5f, 30).level (-9).macro (1, "Vowel").mod (macro1, "osc1Position", 45)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Resonance").mod (macro4, "filter1Resonance", 25);
        add ("Murky Stasis", "Drone").osc (1, analog, sinePos).unison (1, 3, 4, 100, 0, 30).osc (2, analog, sinePos, 60).pitch (2, -12, 5)
            .osc (3, analog, 30, 45).pitch (3, 24, -6).filter (1, ladder24, 1200, 15).lfo (1, drift, 0.11f, 0).mod (lfo1, "osc3Volume", 10)
            .amp (2200, 1000, 100, 4500).reverb (30, 85, 8, 45).level (-14).macro (1, "Beat").mod (macro1, "osc1UnisonDetune", 25)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Ochre Idler", "Drone").osc (1, pulse, 80).osc (2, additive, 35, 70).pitch (2, -12, 6).lfo (1, triangle, 19, 0)
            .mod (lfo1, "ampLevel", 7).mod (lfo1, "filter1Cutoff", 2).lfo (2, drift, 0.26f, 0).mod (lfo2, "lfo1Rate", 8)
            .mod (lfo2, "osc1Position", 10).filter (1, comb, 580, 45, 40).subOsc (30).amp (1100, 1000, 100, 1500).drive (hard, 15, 2800)
            .reverb (40, 85, 5.5f, 50).level (-19).macro (1, "Speed").mod (macro1, "lfo1Rate", 20).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Hum").mod (macro3, "osc2Volume", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Outlaw Idler", "Drone").osc (1, pulse, 65).osc (2, additive, 40, 65).pitch (2, 7, 6).lfo (1, triangle, 16, 0)
            .mod (lfo1, "ampLevel", 5).mod (lfo1, "filter1Cutoff", 1).lfo (2, drift, 0.12f, 0).mod (lfo2, "lfo1Rate", 9)
            .mod (lfo2, "osc1Position", 10).filter (1, ladderBand, 390, 45, 40).subOsc (30).amp (900, 1000, 100, 4000).drive (tube, 35, 2800)
            .reverb (30, 75, 3, 40).level (-23).macro (1, "Speed").mod (macro1, "lfo1Rate", 20).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Hum").mod (macro4, "osc2Volume", 25);
        add ("Red Wake", "Drone").osc (1, analog, 15).unison (1, 3, 12, 100, 0, 10).osc (2, analog, 15, 75).pitch (2, 19, 9)
            .osc (3, analog, sinePos, 55).pitch (3, 24, -8).filter (1, ladder24, 830, 30).lfo (1, sine, 0.094f, 0).mod (lfo1, "osc3Volume", 25)
            .amp (1900, 1000, 100, 3500).reverb (50, 75, 6.5f, 45).level (-18).macro (1, "Beat").mod (macro1, "osc1UnisonDetune", 25)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Motion").mod (macro3, "lfo1Rate", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Slow Standstill", "Drone").osc (1, analog, trianglePos).unison (1, 2, 3, 100, 0, 10).osc (2, analog, trianglePos).pitch (2, 7, 7)
            .osc (3, additive, 20, 55).pitch (3, 7, -2).filter (1, ladder24, 1300, 30).lfo (1, drift, 0.027f, 0).mod (lfo1, "osc3Volume", 25)
            .amp (3500, 1000, 100, 5000).reverb (40, 75, 7, 65).level (-11).macro (1, "Beat").mod (macro1, "osc1UnisonDetune", 25)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "High").mod (macro3, "osc3Volume", 30).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Solar Cantor", "Drone").osc (1, vowel, 10).unison (1, 3, 12, 100, 0, 40).osc (2, pulse, 40, 40).pitch (2, 7, 2)
            .lfo (1, sine, 0.059f, 0).mod (lfo1, "osc1Position", 30).lfo (2, triangle, 0.037f, 0).mod (lfo2, "filter1Cutoff", 12)
            .filter (1, ladderBand, 410, 60, 30).amp (3100, 1000, 100, 5500).reverb (30, 70, 8.5f, 55).level (-11).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Motion").mod (macro3, "lfo2Rate", 30)
            .drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Spectral Menace", "Drone").osc (1, growl, 50).unison (1, 3, 6, 100, 0, 20).osc (2, growl, 50, 75).pitch (2, -11, 7)
            .osc (3, growl, 50, 60).pitch (3, -6, -7).filter (1, ladder24, 1500, 15, 10).lfo (1, sine, 0.03f, 0).mod (lfo1, "filter1Cutoff", 6)
            .mod (lfo1, "osc2Volume", -15).amp (1600, 1000, 100, 3500).reverb (55, 100, 5, 50).level (-12).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Semitone").mod (macro2, "osc2Volume", -50).drive (tube, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Starless Tunnel", "Drone").osc (1, analog, 40, 45).noise (60, 1, 20).filter (1, comb, 261.6f, 92, 0, 100).filter (2, svfLow, 990, 15)
            .lfo (1, drift, 0.12f, 0).mod (lfo1, "filter2Cutoff", 14).mod (lfo1, "noiseLevel", 5).amp (2600, 1000, 100, 5500)
            .reverb (50, 75, 4.5f, 30).level (-14).macro (1, "Blow").mod (macro1, "noiseLevel", 25).macro (2, "Cutoff")
            .mod (macro2, "filter2Cutoff", 25).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Ring")
            .mod (macro4, "filter1Resonance", 3);
        add ("Steel Motor", "Drone").osc (1, pulse, 65).osc (2, additive, 40, 75).pitch (2, 0, 2).lfo (1, sine, 24, 0).mod (lfo1, "ampLevel", 4)
            .mod (lfo1, "filter1Cutoff", 4).lfo (2, drift, 0.22f, 0).mod (lfo2, "lfo1Rate", 9).mod (lfo2, "osc1Position", 5)
            .filter (1, comb, 400, 55, 20).subOsc (35).amp (1500, 1000, 100, 2500).reverb (25, 90, 8, 30).level (-15).macro (1, "Speed")
            .mod (macro1, "lfo1Rate", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .macro (4, "Hum").mod (macro4, "osc2Volume", 25);
        add ("Stripped Foreboding", "Drone").osc (1, pulse, 50).unison (1, 4, 12, 100, 0, 50).osc (2, pulse, 50, 65).pitch (2, 13, 4)
            .osc (3, pulse, 50, 45).pitch (3, 6, -4).filter (1, ladder24, 870, 35, 10).lfo (1, drift, 0.1f, 0).mod (lfo1, "filter1Cutoff", 16)
            .mod (lfo1, "osc2Volume", -15).amp (2800, 1000, 100, 6000).reverb (45, 80, 5, 35).level (-8).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Semitone").mod (macro2, "osc2Volume", -50).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Submerged Watch", "Drone").osc (1, analog, trianglePos).unison (1, 3, 5, 100, 0, 40).osc (2, analog, trianglePos, 75)
            .pitch (2, -12, 8).osc (3, additive, 30, 55).pitch (3, 12, -9).filter (1, ladder24, 1600, 25).lfo (1, drift, 0.12f, 0)
            .mod (lfo1, "osc3Volume", 10).amp (1500, 1000, 100, 5500).reverb (45, 100, 5.5f, 70).level (-15).macro (1, "Beat")
            .mod (macro1, "osc1UnisonDetune", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "High")
            .mod (macro3, "osc3Volume", 30).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Sunken Wake", "Drone").osc (1, analog, sinePos).unison (1, 3, 9, 100, 0, 30).osc (2, analog, sinePos, 65).pitch (2, -12, 7)
            .osc (3, additive, 10, 55).pitch (3, 7, -5).filter (1, ladder24, 820, 30).lfo (1, sine, 0.06f, 0).mod (lfo1, "osc3Volume", 15)
            .amp (3600, 1000, 100, 5000).reverb (50, 100, 3.5f, 50).level (-13).macro (1, "Beat").mod (macro1, "osc1UnisonDetune", 25)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "High").mod (macro3, "osc3Volume", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Sunless Tunnel", "Drone").osc (1, additive, 70, 45).noise (80, 1, -20).filter (1, comb, 523.3f, 92, 0, 100)
            .filter (2, svfLow, 620, 15).lfo (1, drift, 0.11f, 0).mod (lfo1, "filter2Cutoff", 15).mod (lfo1, "noiseLevel", 15)
            .amp (2000, 1000, 100, 3500).reverb (50, 75, 8.5f, 35).level (1).macro (1, "Blow").mod (macro1, "noiseLevel", 25).macro (2, "Cutoff")
            .mod (macro2, "filter2Cutoff", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Trailing Reactor", "Drone").osc (1, analog, sinePos, 1).pitch (1, 5, -20).osc (2, analog, trianglePos).warp (2, wFm, 35)
            .unison (2, 2, 4, 100, 0, 20).lfo (1, sine, 0.1f, 0).mod (lfo1, "osc2WarpAmount", 15).lfo (2, sine, 0.042f, 0)
            .mod (lfo2, "filter1Cutoff", 10).filter (1, ladderBand, 2800, 40, 40).subOsc (20).amp (1500, 1000, 100, 5000).reverb (40, 70, 8, 70)
            .level (-1).macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25)
            .macro (3, "Motion").mod (macro3, "lfo1Rate", 30).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Unlit Cantor", "Drone").osc (1, vowel, 30).unison (1, 3, 12, 100, 0, 20).osc (2, analog, 30, 55).pitch (2, 0, 2)
            .lfo (1, sine, 0.17f, 0).mod (lfo1, "osc1Position", 30).lfo (2, sine, 0.035f, 0).mod (lfo2, "filter1Cutoff", 10)
            .filter (1, svfBand, 890, 60, 30).amp (3400, 1000, 100, 4000).reverb (35, 90, 8, 60).level (-7).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 45).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 25).macro (4, "Motion").mod (macro4, "lfo2Rate", 30);
        add ("Vagrant Furnace", "Drone").osc (1, analog, sinePos, 1).pitch (1, 7, -30).osc (2, analog, trianglePos).warp (2, wFm, 40)
            .unison (2, 1, 4, 100, 0, 30).lfo (1, sine, 0.095f, 0).mod (lfo1, "osc2WarpAmount", 10).lfo (2, triangle, 0.022f, 0)
            .mod (lfo2, "filter1Cutoff", 6).filter (1, ladder24, 580, 20, 20).subOsc (45).amp (4000, 1000, 100, 5000).reverb (35, 95, 4, 50)
            .level (-6).macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25)
            .drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Vagrant Turbine", "Drone").osc (1, pulse, 80).osc (2, additive, 30, 55).pitch (2, 0, 7).lfo (1, sine, 12, 0).mod (lfo1, "ampLevel", 4)
            .mod (lfo1, "filter1Cutoff", 2).lfo (2, drift, 0.092f, 0).mod (lfo2, "lfo1Rate", 9).mod (lfo2, "osc1Position", 5)
            .filter (1, ladderBand, 260, 25, 30).subOsc (40).amp (1300, 1000, 100, 4000).drive (crush, 30, 3900).reverb (40, 85, 4.5f, 55)
            .level (-13).macro (1, "Speed").mod (macro1, "lfo1Rate", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 25).macro (3, "Sub")
            .mod (macro3, "subLevel", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Vast Hermit", "Drone").osc (1, vowel, 0).unison (1, 3, 13, 100, 0, 30).osc (2, growl, 40, 45).pitch (2, 0, 4).lfo (1, sine, 0.067f, 0)
            .mod (lfo1, "osc1Position", 35).lfo (2, sine, 0.039f, 0).mod (lfo2, "filter1Cutoff", 6).filter (1, ladderBand, 420, 45, 10)
            .amp (2200, 1000, 100, 5000).reverb (45, 70, 5.5f, 30).level (-12).macro (1, "Vowel").mod (macro1, "osc1Position", 45)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Motion").mod (macro4, "lfo2Rate", 30);

        // Bell, by name.
        add ("Backroom Crystal", "Bell").osc (1, fm, 80).osc (2, analog, sinePos, 60).pitch (2, 24).env (2, 0, 180, 0, 200, 75)
            .mod (env2, "osc1Position", 40).mod (env2, "osc1Volume", 10).filter (1, svfHigh, 380, 5).amp (0, 1900, 0, 1500, 80)
            .delay (quarter, 35, 14, 2400).reverb (25, 90, 4, 65).level (-16).macro (1, "Glass").mod (macro1, "osc1Position", 30)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Body")
            .mod (macro4, "osc2Volume", -45);
        add ("Boundless Pipe", "Bell").osc (1, additive, 5).osc (2, analog, sinePos, 60).pitch (2, 24).osc (3, metal, 40, 35).pitch (3, 0)
            .noise (1, 0, 40).env (2, 0, 40, 0, 40, 85).mod (env2, "noiseLevel", 45).env (3, 0, 1400, 0, 800, 70).mod (env3, "osc3Volume", -25)
            .mod (env3, "osc1Position", 20).filter (1, svfLow, 3500, 5).amp (0, 4700, 0, 2500, 60).reverb (35, 95, 8, 50).level (-13)
            .macro (1, "Hammer").mod (macro1, "noiseLevel", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).delay (eighthD, 40, 0, 4500)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Burnished Crystal", "Bell").osc (1, fm, 80).osc (2, analog, sinePos, 55).pitch (2, 24).env (2, 0, 80, 0, 200, 75)
            .mod (env2, "osc1Position", 20).mod (env2, "osc1Volume", 10).filter (1, svfLow, 1500, 5).amp (0, 500, 0, 1200, 65)
            .delay (eighthT, 30, 12, 2000, false).reverb (35, 90, 3.5f, 60).level (-13).macro (1, "Glass").mod (macro1, "osc1Position", 30)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Burnished Gong", "Bell").osc (1, metal, 55).unison (1, 2, 5, 100, 0, 50).osc (2, additive, 10, 60).pitch (2, 7, 1)
            .env (2, 500, 3200, 20, 2000, 40).mod (env2, "filter1Cutoff", 10).mod (env2, "osc1Position", 15).filter (1, svfLow, 990)
            .amp (2, 8000, 0, 4000, 65).reverb (45, 85, 4.5f, 30).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22)
            .macro (2, "Shimmer").mod (macro2, "osc1Position", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).delay (eighthD, 40, 0, 4500)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Clammy Girder", "Bell").osc (1, additive, 15).osc (2, analog, sinePos, 65).pitch (2, 24, 5).osc (3, metal, 30, 45).pitch (3, 0)
            .noise (1, 0, 40).env (2, 0, 55, 0, 40, 85).mod (env2, "noiseLevel", 50).env (3, 0, 800, 0, 800, 70).mod (env3, "osc3Volume", -15)
            .mod (env3, "osc1Position", 30).filter (1, svfLow, 6400, 5).amp (0, 5800, 0, 2000, 60).reverb (30, 95, 3.5f, 70).level (-14)
            .macro (1, "Hammer").mod (macro1, "noiseLevel", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Drowned Ampoule", "Bell").osc (1, fm, 40).osc (2, analog, sinePos, 70).pitch (2, 24).env (2, 0, 220, 0, 200, 90)
            .mod (env2, "osc1Position", 25).mod (env2, "osc1Volume", 30).filter (1, ladder12, 360, 5).amp (0, 1800, 0, 1300, 70)
            .delay (quarter, 45, 26, 7100, false).reverb (25, 100, 9, 55).level (-9).macro (1, "Glass").mod (macro1, "osc1Position", 30)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 35).chorus (0, 0.5f, 50)
            .macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Floating Glass", "Bell").osc (1, fm, 80).osc (2, analog, sinePos).pitch (2, 12).env (2, 0, 340, 0, 200, 85)
            .mod (env2, "osc1Position", 20).mod (env2, "osc1Volume", 15).filter (1, svfHigh, 5100, 5).amp (0, 1400, 0, 900, 75)
            .delay (sixteenthD, 45, 14, 2200).reverb (35, 70, 6.5f, 65).level (-3).macro (1, "Glass").mod (macro1, "osc1Position", 30)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).chorus (0, 0.5f, 50)
            .macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Fluorescent Knell", "Bell").osc (1, analog, sinePos, 1).pitch (1, 21, -10).osc (2, analog, sinePos).warp (2, wFm, 10)
            .osc (3, analog, sinePos, 45).pitch (3, 24, 2).env (2, 0, 2500, 0, 1500, 65).mod (env2, "osc2WarpAmount", 55)
            .mod (velocity, "osc2WarpAmount", 10).filter (1, ladder12, 11000, 5).amp (0, 2200, 0, 1900, 75).delay (eighth, 50, 14, 7000)
            .reverb (40, 100, 8, 65).level (-11).macro (1, "Strike").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Formless Rod", "Bell").osc (1, additive, 10).osc (2, analog, sinePos, 65).pitch (2, 16).osc (3, metal, 0, 45).pitch (3, 12)
            .noise (1, 0, 40).env (2, 0, 30, 0, 40, 85).mod (env2, "noiseLevel", 50).env (3, 0, 800, 0, 800, 70).mod (env3, "osc3Volume", -25)
            .mod (env3, "osc1Position", 20).filter (1, ladder12, 7600, 5).amp (0, 4500, 0, 3000, 60).reverb (40, 75, 4, 30).level (-13)
            .macro (1, "Hammer").mod (macro1, "noiseLevel", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Partial").mod (macro4, "osc2Volume", -45);
        add ("Ghost Pipe", "Bell").osc (1, additive, 20).osc (2, analog, sinePos, 50).pitch (2, 22, 5).osc (3, metal, 40, 50).pitch (3, 12)
            .noise (1, 0, 40).env (2, 0, 45, 0, 40, 85).mod (env2, "noiseLevel", 25).env (3, 0, 1200, 0, 800, 70).mod (env3, "osc3Volume", -30)
            .mod (env3, "osc1Position", 25).filter (1, ladder12, 3800, 5).amp (0, 5200, 0, 3000, 55).reverb (45, 75, 8, 55).level (-11)
            .macro (1, "Hammer").mod (macro1, "noiseLevel", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Glassy Tocsin", "Bell").osc (1, analog, sinePos, 1).pitch (1, 9, -5).osc (2, analog, sinePos).warp (2, wFm, 10)
            .osc (3, analog, sinePos, 50).pitch (3, 19, 3).env (2, 0, 900, 0, 1500, 70).mod (env2, "osc2WarpAmount", 20)
            .mod (velocity, "osc2WarpAmount", 16).filter (1, svfLow, 5600, 5).amp (0, 2400, 0, 2000, 55).delay (eighthT, 40, 18, 2400)
            .reverb (30, 95, 4, 45).level (-15).macro (1, "Strike").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Heavy Rod", "Bell").osc (1, additive, 25).osc (2, analog, sinePos, 55).pitch (2, 16, -5).osc (3, metal, 40, 35).pitch (3, 0)
            .noise (1, 0, 40).env (2, 0, 35, 0, 40, 85).mod (env2, "noiseLevel", 50).env (3, 0, 1300, 0, 800, 70).mod (env3, "osc3Volume", -35)
            .mod (env3, "osc1Position", 30).filter (1, svfLow, 9300, 5).amp (0, 4300, 0, 2500, 75).reverb (50, 75, 7, 35).level (-14)
            .macro (1, "Hammer").mod (macro1, "noiseLevel", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Hexed Shield", "Bell").osc (1, metal, 40).unison (1, 3, 11, 100, 0, 30).osc (2, additive, 10, 65).pitch (2, -12, 1)
            .env (2, 500, 2800, 20, 2000, 40).mod (env2, "filter1Cutoff", 20).mod (env2, "osc1Position", 20).filter (1, svfLow, 1700, 5)
            .amp (10, 5000, 0, 4000, 45).reverb (55, 100, 5.5f, 30).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22)
            .macro (2, "Shimmer").mod (macro2, "osc1Position", 30).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Hooded Prism", "Bell").osc (1, fm, 75).osc (2, analog, sinePos, 60).pitch (2, 24).env (2, 0, 220, 0, 200, 80)
            .mod (env2, "osc1Position", 25).mod (env2, "osc1Volume", 15).filter (1, ladder12, 750, 5).amp (0, 1600, 0, 1500, 65)
            .delay (eighthT, 25, 28, 2700).reverb (30, 70, 4.5f, 45).level (-10).macro (1, "Glass").mod (macro1, "osc1Position", 30)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Body").mod (macro3, "osc2Volume", -45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Infected Peal", "Bell").osc (1, analog, sinePos, 1).pitch (1, 9, 5).osc (2, analog, sinePos).warp (2, wFm, 15)
            .osc (3, analog, sinePos, 35).pitch (3, 0, 5).env (2, 0, 2400, 0, 1500, 65).mod (env2, "osc2WarpAmount", 45)
            .mod (velocity, "osc2WarpAmount", 14).filter (1, ladder12, 12000, 5).amp (0, 4500, 0, 3500, 70).reverb (40, 75, 5, 30).level (-12)
            .macro (1, "Strike").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Hum")
            .mod (macro3, "osc3Volume", 30).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Lawless Bar", "Bell").osc (1, additive, 35).osc (2, analog, sinePos, 50).pitch (2, 16, -10).osc (3, metal, 40, 45).pitch (3, 12)
            .noise (1, 0, 40).env (2, 0, 15, 0, 40, 85).mod (env2, "noiseLevel", 40).env (3, 0, 600, 0, 800, 70).mod (env3, "osc3Volume", -25)
            .mod (env3, "osc1Position", 15).filter (1, svfLow, 3200, 5).amp (0, 2700, 0, 4000, 70).reverb (45, 100, 6, 35).level (-11)
            .macro (1, "Hammer").mod (macro1, "noiseLevel", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Partial")
            .mod (macro3, "osc2Volume", -45).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Lost Knell", "Bell").osc (1, analog, sinePos, 1).pitch (1, 14, -5).osc (2, analog, sinePos).warp (2, wFm, 10)
            .osc (3, analog, sinePos, 55).pitch (3, 0).env (2, 0, 1100, 0, 1500, 75).mod (env2, "osc2WarpAmount", 45)
            .mod (velocity, "osc2WarpAmount", 8).filter (1, ladder12, 4700, 5).amp (0, 4600, 0, 3500, 70).reverb (45, 100, 5, 70).level (-14)
            .macro (1, "Strike").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20)
            .delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Lucid Disc", "Bell").osc (1, metal, 75).unison (1, 3, 4, 100, 0, 20).osc (2, fm, 30, 60).pitch (2, 7, 3)
            .env (2, 500, 2000, 20, 2000, 40).mod (env2, "filter1Cutoff", 15).mod (env2, "osc1Position", 25).filter (1, ladder24, 730, 25)
            .amp (2, 4000, 0, 5000, 60).reverb (35, 80, 4, 60).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Shimmer")
            .mod (macro2, "osc1Position", 30).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Lucid Tube", "Bell").osc (1, additive, 5).osc (2, analog, sinePos, 55).pitch (2, 19, -15).osc (3, metal, 0, 50).pitch (3, 12)
            .noise (1, 0, 40).env (2, 0, 35, 0, 40, 85).mod (env2, "noiseLevel", 50).env (3, 0, 400, 0, 800, 70).mod (env3, "osc3Volume", -30)
            .mod (env3, "osc1Position", 10).filter (1, svfLow, 4200, 5).amp (0, 5900, 0, 4000, 65).reverb (35, 95, 3, 70).level (-15)
            .macro (1, "Hammer").mod (macro1, "noiseLevel", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).delay (eighthD, 40, 0, 4500)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Partial").mod (macro4, "osc2Volume", -45);
        add ("Neon Icicle", "Bell").osc (1, fm, 70).osc (2, analog, sinePos, 65).pitch (2, 0).env (2, 0, 200, 0, 200, 80)
            .mod (env2, "osc1Position", 25).mod (env2, "osc1Volume", 10).filter (1, ladder12, 200, 5).amp (0, 1800, 0, 1000, 65)
            .delay (eighthT, 35, 18, 7100, false).reverb (30, 95, 6.5f, 45).level (-6).macro (1, "Glass").mod (macro1, "osc1Position", 30)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Pitted Knell", "Bell").osc (1, analog, sinePos, 1).pitch (1, 16, -10).osc (2, analog, sinePos).warp (2, wFm, 15)
            .osc (3, analog, sinePos, 45).pitch (3, 12, 5).env (2, 0, 1300, 0, 1500, 65).mod (env2, "osc2WarpAmount", 40)
            .mod (velocity, "osc2WarpAmount", 12).filter (1, svfLow, 6100, 5).amp (0, 1800, 0, 3200, 75).reverb (35, 70, 7, 40).level (-13)
            .macro (1, "Strike").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).drive (tube, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Platinum Toll", "Bell").osc (1, analog, sinePos, 1).pitch (1, 21, -10).osc (2, analog, sinePos).warp (2, wFm, 15)
            .osc (3, analog, sinePos, 45).pitch (3, 0, 1).env (2, 0, 2000, 0, 1500, 60).mod (env2, "osc2WarpAmount", 55)
            .mod (velocity, "osc2WarpAmount", 10).filter (1, ladder12, 7600, 5).amp (0, 4400, 0, 1600, 70).reverb (35, 95, 6, 65).level (-13)
            .macro (1, "Strike").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Hum").mod (macro4, "osc3Volume", 30);
        add ("Polished Chime", "Bell").osc (1, analog, sinePos, 1).pitch (1, 16).osc (2, analog, sinePos).warp (2, wFm, 20)
            .osc (3, analog, sinePos, 55).pitch (3, 19, 3).env (2, 0, 1500, 0, 1500, 65).mod (env2, "osc2WarpAmount", 35)
            .mod (velocity, "osc2WarpAmount", 8).filter (1, svfLow, 13000, 5).amp (0, 4800, 0, 2300, 65).delay (eighth, 35, 16, 2100, false)
            .reverb (25, 70, 7, 40).level (-16).macro (1, "Strike").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).macro (3, "Hum").mod (macro3, "osc3Volume", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Remote Lens", "Bell").osc (1, fm, 80).osc (2, analog, sinePos, 60).pitch (2, 0).env (2, 0, 100, 0, 200, 85)
            .mod (env2, "osc1Position", 35).mod (env2, "osc1Volume", 15).filter (1, ladder12, 740, 5).amp (0, 600, 0, 1100, 70)
            .delay (quarter, 50, 12, 2800).reverb (35, 90, 4.5f, 65).level (-10).macro (1, "Glass").mod (macro1, "osc1Position", 30)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Scarred Girder", "Bell").osc (1, additive, 35).osc (2, analog, sinePos, 45).pitch (2, 19, -10).osc (3, metal, 0, 35).pitch (3, 12)
            .noise (1, 0, 40).env (2, 0, 20, 0, 40, 85).mod (env2, "noiseLevel", 40).env (3, 0, 300, 0, 800, 70).mod (env3, "osc3Volume", -35)
            .mod (env3, "osc1Position", 15).filter (1, ladder12, 9300, 5).amp (0, 3900, 0, 4000, 75).reverb (50, 95, 8.5f, 65).level (-11)
            .macro (1, "Hammer").mod (macro1, "noiseLevel", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Sector Gong", "Bell").osc (1, metal, 20).unison (1, 2, 5, 100, 0, 50).osc (2, fm, 60, 55).pitch (2, -7, 12)
            .env (2, 300, 3300, 20, 2000, 40).mod (env2, "filter1Cutoff", 25).mod (env2, "osc1Position", 25).filter (1, ladder12, 1600, 20)
            .amp (10, 6500, 0, 3000, 65).reverb (45, 75, 7.5f, 35).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22)
            .macro (2, "Shimmer").mod (macro2, "osc1Position", 30).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30)
            .macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Southern Urn", "Bell").osc (1, metal, 25).unison (1, 3, 9, 100, 0, 50).osc (2, additive, 60, 55).pitch (2, 7)
            .env (2, 800, 4000, 20, 2000, 40).mod (env2, "filter1Cutoff", 25).mod (env2, "osc1Position", 20).filter (1, ladder24, 2400, 20)
            .amp (2, 4500, 0, 3500, 60).reverb (40, 95, 3.5f, 40).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22)
            .macro (2, "Shimmer").mod (macro2, "osc1Position", 30).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Stainless Knell", "Bell").osc (1, analog, sinePos, 1).pitch (1, 14, -15).osc (2, analog, sinePos).warp (2, wFm, 10)
            .osc (3, analog, sinePos, 50).pitch (3, 19).env (2, 0, 2600, 0, 1500, 80).mod (env2, "osc2WarpAmount", 25)
            .mod (velocity, "osc2WarpAmount", 20).filter (1, svfLow, 6000, 5).amp (0, 5000, 0, 2900, 55).delay (eighth, 30, 20, 5900, false)
            .reverb (25, 95, 3, 50).level (-13).macro (1, "Strike").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Sterile Prism", "Bell").osc (1, fm, 60).osc (2, analog, sinePos, 65).pitch (2, 12).env (2, 0, 80, 0, 200, 85)
            .mod (env2, "osc1Position", 40).mod (env2, "osc1Volume", 15).filter (1, svfHigh, 4600, 5).amp (0, 800, 0, 800, 65)
            .delay (eighthD, 45, 12, 5200, false).reverb (35, 90, 9, 55).level (-3).macro (1, "Glass").mod (macro1, "osc1Position", 30)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Body")
            .mod (macro4, "osc2Volume", -45);
        add ("Umber Toll", "Bell").osc (1, analog, sinePos, 1).pitch (1, 16, -20).osc (2, analog, sinePos).warp (2, wFm, 10)
            .osc (3, analog, sinePos, 40).pitch (3, 0, 1).env (2, 0, 1700, 0, 1500, 80).mod (env2, "osc2WarpAmount", 55)
            .mod (velocity, "osc2WarpAmount", 10).filter (1, svfLow, 10000, 5).amp (0, 2600, 0, 2100, 60).delay (eighthT, 50, 24, 7000)
            .reverb (35, 100, 8.5f, 70).level (-14).macro (1, "Strike").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Hum").mod (macro4, "osc3Volume", 30);
        add ("Vaulted Shield", "Bell").osc (1, metal, 20).unison (1, 3, 4, 100, 0, 40).osc (2, additive, 60, 45).pitch (2, -7, 7)
            .env (2, 800, 3100, 20, 2000, 40).mod (env2, "filter1Cutoff", 25).mod (env2, "osc1Position", 25).filter (1, svfLow, 1100, 20)
            .amp (10, 5000, 0, 4500, 45).reverb (45, 80, 4.5f, 60).level (-6).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22)
            .macro (2, "Shimmer").mod (macro2, "osc1Position", 30).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25)
            .delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Wordless Cauldron", "Bell").osc (1, metal, 75).unison (1, 2, 5, 100, 0, 30).osc (2, fm, 20, 50).pitch (2, -5, 6)
            .env (2, 600, 3500, 20, 2000, 40).mod (env2, "filter1Cutoff", 25).mod (env2, "osc1Position", 25).filter (1, ladder12, 610, 15)
            .amp (10, 8000, 0, 3500, 60).reverb (50, 95, 5.5f, 50).level (-8).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22)
            .macro (2, "Shimmer").mod (macro2, "osc1Position", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).delay (eighthD, 40, 0, 4500)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 30);

        // Sequence, by name.
        add ("Airless Ledger", "Sequence").osc (1, analog, 85).subOsc (40).filter (1, ladder24, 340, 80, 20).lfoSync (1, steps, bar1, 1, 90)
            .stepPattern (1, { -60, 0, -100, 30, -60, 0, 60, -100, 100, 100, 100, -100, -60, -100, -30, -60 }).mod (lfo1, "filter1Cutoff", 24)
            .amp (2, 300, 100, 160).mono().drive (tube, 30, 3700).delay (sixteenthD, 45, 22, 3300, false).level (-14).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 18).macro (3, "Depth")
            .mod (macro3, "filter1Drive", 55).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Alloy Sampler", "Sequence").osc (1, metal, 50).unison (1, 2, 12, 50).subOsc (40).lfoSync (1, steps, bar1)
            .stepPattern (1, { 100, -70, 50, 20, -70, 20, -100, -40, 100, -40, -40, -70, 50, 50, 50, 100 }).mod (lfo1, "osc1Position", 42)
            .lfoSync (2, steps, bars2, 1, 89).stepPattern (2, { 0, -100, -30, -30, 0, -100, -30, -30, 0, -100, -30, -30, 0, -100, -30, -30 })
            .mod (lfo2, "ampLevel", 60).filter (1, svfBand, 4700, 15).amp (2, 300, 100, 220).drive (tube, 35, 8200).delay (eighthD, 25, 22, 4600)
            .level (-15).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Range").mod (macro2, "osc1Position", 30).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Asphalt Bellows", "Sequence").osc (1, growl, 20).unison (1, 2, 8, 70).filter (1, ladder24, 1300, 35, 30)
            .lfoSync (1, steps, half, 1, 98).stepPattern (1, { -100, -100, -90, -90, -75, -75, -60, -60, -45, -45, -30, -30, -15, -15, 0, 0 })
            .mod (lfo1, "ampLevel", 80).mod (lfo1, "filter1Cutoff", 4).amp (200, 300, 100, 500).chorus (30, 0.7f, 50).reverb (30, 85, 9, 65)
            .level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Duck").mod (macro2, "ampLevel", 14).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Austere Sampler", "Sequence").osc (1, sync, 50).unison (1, 1, 8, 90).lfoSync (1, steps, bars2, 1, 60)
            .stepPattern (1, { 100, 20, 80, -40, -40, 20, -10, 50, -100, 80, -100, -100, 100, -40, 100, -70 }).mod (lfo1, "osc1Position", 48)
            .lfoSync (2, steps, bars2, 1, 90).stepPattern (2, { 0, -100, -100, 0, -100, 0, -100, -100, 0, -100, -100, 0, -100, 0, -100, -100 })
            .mod (lfo2, "ampLevel", 60).filter (1, svfBand, 2200, 45).amp (2, 300, 100, 180).delay (eighth, 45, 22, 5700).level (-9)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Range").mod (macro2, "osc1Position", 30).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).reverb (0, 70, 3, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Barrack Drill", "Sequence").osc (1, analog, squarePos).subOsc (45).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 50, 0, 0, 50, 0, 50, 0, 50, 0, 0, 50, 50, 0, 0, 0 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, quarter, 1, 96).stepPattern (2, { 0, -45, -90, -100, 0, -45, -90, -100, 0, -45, -90, -100, 0, -45, -90, -100 })
            .mod (lfo2, "ampLevel", 78).mod (lfo2, "filter1Cutoff", 20).filter (1, ladder24, 320, 35, 20).amp (2, 300, 100, 80).mono()
            .drive (fuzz, 20, 4400).level (-20).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Blunt Patrol", "Sequence").osc (1, sync, 40).subOsc (25).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 50, 0, 50, 0, 50, 0, 50, 0, 50, 0, 50, 0, 50, 0, 50 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, quarter, 1, 99).stepPattern (2, { 0, -30, -65, -100, 0, -30, -65, -100, 0, -30, -65, -100, 0, -30, -65, -100 })
            .mod (lfo2, "ampLevel", 81).mod (lfo2, "filter1Cutoff", 12).filter (1, ladder24, 890, 50, 50).amp (2, 300, 100, 80).mono()
            .drive (hard, 45, 4400).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Bolted Lattice", "Sequence").osc (1, metal, 50).unison (1, 2, 18, 50).filter (1, svfBand, 1700, 45, 10)
            .lfoSync (1, steps, half, 1, 95).stepPattern (1, { 0, -100, 0, -30, 0, -100, 0, -30, 0, -100, 0, -30, 0, -100, 0, -30 })
            .mod (lfo1, "ampLevel", 80).lfoSync (2, sawUp, eighthD, 0).mod (lfo2, "filter1Cutoff", 22).amp (2, 300, 100, 140).drive (hard, 25, 4400)
            .delay (eighthD, 40, 20, 6500, false).level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Gate")
            .mod (macro2, "ampLevel", 14).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Bronze Pump", "Sequence").osc (1, metal, 25).unison (1, 2, 18).osc (2, analog, sawPos, 45).pitch (2, 0, 7)
            .filter (1, ladder12, 1600, 15, 20).lfoSync (1, steps, quarter, 1, 96)
            .stepPattern (1, { -100, -60, -30, -10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }).mod (lfo1, "ampLevel", 65).mod (lfo1, "filter1Cutoff", 6)
            .amp (50, 300, 100, 300).chorus (30, 0.5f, 50).reverb (20, 90, 5.5f, 35).level (-7).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Duck").mod (macro2, "ampLevel", 14).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Buried Crank", "Sequence").osc (1, metal, 65).unison (1, 3, 10, 50).subOsc (30).filter (1, ladder24, 550, 20)
            .lfoSync (2, steps, quarter, 1, 98).stepPattern (2, { 0, -40, -80, -100, -100, -100, -100, -100, 0, -40, -80, -100, 0, -40, -80, -100 })
            .mod (lfo2, "ampLevel", 85).mod (lfo2, "filter1Cutoff", 12).lfoSync (1, steps, bar1, 1, 50)
            .stepPattern (1, { 0, 0, -40, 70, 100, 40, 0, 100, 0, 40, 40, 0, 0, 70, 0, 0 }).mod (lfo1, "filter1Cutoff", 16).amp (2, 300, 100, 100)
            .level (-3).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 25)
            .delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).reverb (0, 50, 1.4f, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 30);
        add ("Circling Shutter", "Sequence").osc (1, vowel, 25).unison (1, 2, 14, 60).filter (1, ladder24, 1600, 15, 10)
            .lfoSync (1, steps, half, 1, 89).stepPattern (1, { 0, -100, -100, -100, 0, -100, 0, -100, 0, -100, -100, -100, 0, -100, 0, -25 })
            .mod (lfo1, "ampLevel", 80).lfo (2, sine, 0.12f, 0).mod (lfo2, "filter1Cutoff", 14).amp (2, 300, 100, 200).drive (tube, 30, 3500)
            .delay (sixteenthD, 35, 22, 4200).reverb (24, 75, 6, 65).level (-15).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Gate").mod (macro2, "ampLevel", 14).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Clipped Charge", "Sequence").osc (1, analog, squarePos).subOsc (25).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 0, 0, 50, 0, 0, 50, 50, 0, 0, 0, 50, 0, 0, 50, 50 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, quarter, 1, 98).stepPattern (2, { 0, 0, -50, -100, 0, 0, -50, -100, 0, 0, -50, -100, 0, 0, -50, -100 })
            .mod (lfo2, "ampLevel", 74).mod (lfo2, "filter1Cutoff", 18).filter (1, ladder24, 520, 25, 30).amp (2, 300, 100, 70).mono()
            .drive (hard, 35, 7500).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Close Routine", "Sequence").osc (1, analog, squarePos).subOsc (25).osc (2, analog, squarePos, 55).pitch (2, 12, 7)
            .filter (1, ladder24, 270, 55, 10).lfoSync (1, steps, bars2, 1, 50)
            .stepPattern (1, { 30, -60, -60, -30, 60, -100, -60, -100, 30, -60, -60, -30, 60, -100, -60, -100 }).mod (lfo1, "filter1Cutoff", 12)
            .amp (2, 300, 100, 200).mono().drive (hard, 30, 4100).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 18).delay (eighthD, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Depth").mod (macro4, "filter1Drive", 55);
        add ("Coastal Crank", "Sequence").osc (1, vowel, 30).unison (1, 3, 18, 90).osc (2, analog, squarePos, 65).pitch (2, 12, 7).subOsc (40)
            .filter (1, ladder24, 480, 40, 10).lfoSync (2, steps, half, 1, 98)
            .stepPattern (2, { 0, -15, -40, -70, -100, -100, -100, -100, 0, -15, -40, -70, 0, -15, -40, -70 }).mod (lfo2, "ampLevel", 82)
            .mod (lfo2, "filter1Cutoff", 12).lfoSync (1, steps, bars2, 1, 50)
            .stepPattern (1, { 70, 0, 0, 100, 0, 70, 0, 0, 0, 0, 40, 70, 40, 100, 70, 70 }).mod (lfo1, "filter1Cutoff", 6).amp (2, 300, 100, 160)
            .drive (hard, 45, 4700).delay (eighthD, 30, 8, 3600).level (-26).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 25).macro (3, "Accent").mod (macro3, "filter1Drive", 55).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Cobalt Plait", "Sequence").osc (1, metal, 50).unison (1, 4, 16, 90).osc (2, pulse, 15, 65).pitch (2, 7, 3)
            .filter (1, svfLow, 1300, 35).lfoSync (1, steps, half, 1, 89)
            .stepPattern (1, { 0, -100, -100, -30, 0, -100, -100, -30, 0, -100, -100, -30, 0, -100, -30, -30 }).mod (lfo1, "ampLevel", 70)
            .lfoSync (2, triangle, quarterT, 0).mod (lfo2, "filter1Cutoff", 20).amp (2, 300, 100, 200).delay (sixteenthD, 55, 14, 4300, false)
            .level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Gate").mod (macro2, "ampLevel", 14).drive (tube, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Condemned Baffle", "Sequence").osc (1, pulse, 60).unison (1, 2, 14).osc (2, pulse, 60, 60).pitch (2, 7, 7)
            .filter (1, ladder24, 2500, 45, 20).lfoSync (1, steps, bar1, 1, 86)
            .stepPattern (1, { -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0 }).mod (lfo1, "ampLevel", 80)
            .lfo (2, triangle, 0.085f, 0).mod (lfo2, "filter1Cutoff", 14).amp (2, 300, 100, 350).delay (sixteenthD, 50, 14, 2600).level (-12)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Gate").mod (macro2, "ampLevel", 14).drive (tube, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).reverb (0, 70, 3, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Condemned Charge", "Sequence").osc (1, sync, 25).subOsc (40).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 0, 50, 50, 0, 0, 50, 0, 0, 0, 50, 50, 0, 0, 50, 0 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, quarter, 1, 99).stepPattern (2, { 0, 0, -50, -100, 0, 0, -50, -100, 0, 0, -50, -100, 0, 0, -50, -100 })
            .mod (lfo2, "ampLevel", 73).mod (lfo2, "filter1Cutoff", 16).filter (1, ladder24, 530, 35, 40).amp (2, 300, 100, 110).mono()
            .drive (tube, 15, 6500).level (-3).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Damped Program", "Sequence").osc (1, analog, 85).subOsc (40).osc (2, analog, squarePos, 55).pitch (2, 7, 7)
            .filter (1, ladder24, 460, 75, 40).lfoSync (1, steps, bar1, 1, 70)
            .stepPattern (1, { -100, -30, 60, 100, 0, -30, 60, -100, -30, 60, 0, -30, -100, -30, 60, 60 }).mod (lfo1, "filter1Cutoff", 24)
            .amp (2, 300, 100, 200).mono().drive (fuzz, 30, 6500).level (-24).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 18).macro (3, "Depth").mod (macro3, "filter1Drive", 55)
            .delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Daybreak Patrol", "Sequence").osc (1, sync, 50).subOsc (30).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 0, 0, 50, 0, 0, 50, 50, 0, 0, 0, 50, 0, 0, 50, 50 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, quarter, 1, 99).stepPattern (2, { 0, -20, -50, -85, 0, -20, -50, -85, 0, -20, -50, -85, 0, -20, -50, -85 })
            .mod (lfo2, "ampLevel", 74).mod (lfo2, "filter1Cutoff", 10).filter (1, ladder24, 1200, 30, 10).amp (2, 300, 100, 150).mono()
            .drive (hard, 45, 5600).level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Dented Sorter", "Sequence").osc (1, sync, 50).unison (1, 3, 10, 50).lfoSync (1, steps, bar1, 1, 20)
            .stepPattern (1, { 20, 50, 80, 80, -10, -70, 100, 80, -70, 80, 100, 80, 50, -100, 80, 100 }).mod (lfo1, "osc1Position", 48)
            .lfoSync (2, steps, bars2, 1, 95).stepPattern (2, { -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0 })
            .mod (lfo2, "ampLevel", 40).filter (1, ladder24, 1600, 45).amp (2, 300, 100, 220).delay (sixteenthD, 30, 16, 3800).level (-9)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Range").mod (macro2, "osc1Position", 30).reverb (0, 70, 3, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Depot Damper", "Sequence").osc (1, vowel, 45).unison (1, 3, 8, 90).osc (2, pulse, 55, 55).pitch (2, 7, 8).filter (1, svfLow, 1500, 15)
            .lfoSync (1, steps, half, 1, 86).stepPattern (1, { -100, -100, 0, -100, -100, -100, 0, -100, -100, -100, 0, -100, -100, -100, 0, -100 })
            .mod (lfo1, "ampLevel", 75).lfo (2, sine, 0.06f, 0).mod (lfo2, "filter1Cutoff", 12).amp (2, 300, 100, 150)
            .delay (eighthD, 40, 20, 2200, false).reverb (24, 75, 8, 60).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Gate").mod (macro2, "ampLevel", 14).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Dreamless Heave", "Sequence").osc (1, pulse, 70).unison (1, 3, 14, 90).osc (2, pulse, 20, 60).pitch (2, -12, 7).subOsc (40)
            .filter (1, ladder24, 760, 40, 10).lfoSync (1, steps, quarter, 1, 99)
            .stepPattern (1, { -100, -100, -80, -80, -55, -55, -35, -35, -20, -20, -10, -10, -4, -4, 0, 0 }).mod (lfo1, "ampLevel", 55)
            .mod (lfo1, "filter1Cutoff", 4).amp (50, 300, 100, 300).chorus (25, 0.5f, 50).reverb (30, 90, 7, 60).level (-7).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Duck").mod (macro2, "ampLevel", 14).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Echoing Bellows", "Sequence").osc (1, analog, squarePos).unison (1, 4, 8).filter (1, svfLow, 880, 30).lfoSync (1, steps, half, 1, 98)
            .stepPattern (1, { -100, -80, -55, -35, -20, -10, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0 }).mod (lfo1, "ampLevel", 50)
            .mod (lfo1, "filter1Cutoff", 4).amp (50, 300, 100, 500).chorus (30, 0.2f, 50).reverb (35, 90, 7.5f, 40).level (-15).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Duck").mod (macro2, "ampLevel", 14).delay (eighthD, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Exposed Tally", "Sequence").osc (1, analog, 85).subOsc (25).filter (1, ladder12, 550, 80, 30).lfoSync (1, steps, bar1, 1, 40)
            .stepPattern (1, { -100, -100, 100, 0, -30, 60, -30, -100, -100, -100, 100, 0, -30, 60, -30, -100 }).mod (lfo1, "filter1Cutoff", 24)
            .amp (2, 300, 100, 140).mono().drive (tube, 50, 9500).delay (eighthD, 35, 22, 6300, false).level (-22).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 18).macro (3, "Sub")
            .mod (macro3, "subLevel", 40).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Fried Braid", "Sequence").osc (1, metal, 70).unison (1, 4, 12, 50).osc (2, pulse, 0, 45).pitch (2, 12, 3)
            .filter (1, ladder24, 1200, 25, 30).lfoSync (1, steps, bar1, 1, 86)
            .stepPattern (1, { -100, -100, 0, -100, -100, -100, 0, -100, -100, -100, 0, -100, -100, -100, 0, -100 }).mod (lfo1, "ampLevel", 70)
            .lfoSync (2, sawUp, quarterT, 0).mod (lfo2, "filter1Cutoff", 16).amp (2, 300, 100, 160).drive (hard, 15, 2900)
            .delay (eighthD, 35, 16, 2100).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Gate")
            .mod (macro2, "ampLevel", 14).macro (3, "Resonance").mod (macro3, "filter1Resonance", 25).reverb (0, 70, 3, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Grim Louvre", "Sequence").osc (1, metal, 35).unison (1, 3, 18, 50).osc (2, pulse, 5, 65).pitch (2, -12, 9)
            .filter (1, ladder12, 2900, 35).lfoSync (1, steps, bar1, 1, 95)
            .stepPattern (1, { 0, -100, -35, -100, 0, -100, -35, -100, 0, -35, 0, -100, -35, -100, 0, -35 }).mod (lfo1, "ampLevel", 80)
            .lfo (2, triangle, 0.14f, 0).mod (lfo2, "filter1Cutoff", 8).amp (2, 300, 100, 250).delay (sixteenthD, 45, 16, 3000)
            .reverb (12, 75, 8, 55).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Gate").mod (macro2, "ampLevel", 14)
            .drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Gutted Treadle", "Sequence").osc (1, fm, 20).unison (1, 4, 8, 50).subOsc (50).filter (1, ladder24, 1100, 30, 30)
            .lfoSync (2, steps, quarter, 1, 99).stepPattern (2, { 0, -15, -40, -70, 0, -15, -40, -70, 0, -15, -40, -70, 0, -15, -40, -70 })
            .mod (lfo2, "ampLevel", 84).mod (lfo2, "filter1Cutoff", 8).lfoSync (1, steps, bar1, 1, 50)
            .stepPattern (1, { -40, -40, -40, 40, 100, -40, 0, 70, 40, 70, 40, 100, 0, 0, 70, 0 }).mod (lfo1, "filter1Cutoff", 8)
            .amp (2, 300, 100, 180).delay (eighthD, 40, 12, 2800).level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 25).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).reverb (0, 50, 1.4f, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 30);
        add ("Harbour Plait", "Sequence").osc (1, metal, 70).unison (1, 4, 18, 50).filter (1, svfBand, 1300, 25, 10).lfoSync (1, steps, bar1, 1, 89)
            .stepPattern (1, { 0, -100, -30, -30, 0, -100, -30, -30, 0, -100, -30, -30, 0, -100, -30, -30 }).mod (lfo1, "ampLevel", 65)
            .lfoSync (2, sine, quarterT, 0).mod (lfo2, "filter1Cutoff", 14).amp (2, 300, 100, 280).delay (eighthD, 50, 14, 2600).level (-6)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Gate").mod (macro2, "ampLevel", 14).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).reverb (0, 70, 3, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Hard Tracer", "Sequence").osc (1, metal, 50).unison (1, 3, 12, 90).subOsc (45).lfoSync (1, steps, bar1, 1, 40)
            .stepPattern (1, { -10, -10, -100, -100, 80, -70, -10, -70, 100, -100, -70, 100, 50, -100, 80, -100 }).mod (lfo1, "osc1Position", 30)
            .lfoSync (2, steps, bar1, 1, 89).stepPattern (2, { -100, -100, 0, -100, -100, -100, 0, -100, -100, -100, 0, -100, -100, -100, 0, -100 })
            .mod (lfo2, "ampLevel", 70).filter (1, ladder24, 1300, 25).amp (2, 300, 100, 180).delay (sixteenthD, 50, 10, 1900, false).level (-7)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Range").mod (macro2, "osc1Position", 30).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Heavy Reader", "Sequence").osc (1, metal, 50).unison (1, 2, 10, 60).subOsc (35).lfoSync (1, steps, bar1, 1, 20)
            .stepPattern (1, { -70, 100, 20, 80, -40, -40, -70, 100, 20, 100, -70, -100, -40, 20, 100, 20 }).mod (lfo1, "osc1Position", 45)
            .lfoSync (2, steps, bar1, 1, 93).stepPattern (2, { -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0 })
            .mod (lfo2, "ampLevel", 50).filter (1, svfBand, 2400, 15).amp (2, 300, 100, 160).drive (tube, 20, 3300).delay (eighth, 50, 18, 5900)
            .level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Range").mod (macro2, "osc1Position", 30).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).reverb (0, 70, 3, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Hungry Reader", "Sequence").osc (1, metal, 50).unison (1, 2, 10, 50).subOsc (40).lfoSync (1, steps, bar1)
            .stepPattern (1, { -100, 80, -40, -40, -100, -100, 80, -100, -100, 80, -10, -10, -100, -10, -70, -70 }).mod (lfo1, "osc1Position", 39)
            .lfoSync (2, steps, bar1, 1, 93).stepPattern (2, { 0, -100, -30, -100, 0, -100, -30, -100, 0, -30, 0, -100, -30, -100, 0, -30 })
            .mod (lfo2, "ampLevel", 50).filter (1, ladder12, 2100, 35).amp (2, 300, 100, 140).delay (sixteenthD, 55, 18, 2100).level (-8)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Range").mod (macro2, "osc1Position", 30).drive (tube, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Hunted Sifter", "Sequence").osc (1, vowel, 50).unison (1, 3, 8, 90).lfoSync (1, steps, bar1, 1, 40)
            .stepPattern (1, { -10, 100, 80, 80, -10, 20, 100, 80, -10, 20, 100, 80, 80, -70, -70, 100 }).mod (lfo1, "osc1Position", 36)
            .lfoSync (2, steps, bars2, 1, 95).stepPattern (2, { -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0 })
            .mod (lfo2, "ampLevel", 50).filter (1, svfBand, 3300, 35).amp (2, 300, 100, 200).drive (hard, 35, 4000)
            .delay (eighthD, 40, 16, 6100, false).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Range")
            .mod (macro2, "osc1Position", 30).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Ivory Louvre", "Sequence").osc (1, vowel, 15).unison (1, 4, 10, 70).osc (2, analog, sawPos, 55).pitch (2, 7, 5)
            .filter (1, ladder24, 3000, 45, 20).lfoSync (1, steps, bar1, 1, 89)
            .stepPattern (1, { 0, -100, -100, -100, 0, -100, 0, -100, 0, -100, -100, -100, 0, -100, 0, -35 }).mod (lfo1, "ampLevel", 75)
            .lfo (2, sine, 0.057f, 0).mod (lfo2, "filter1Cutoff", 14).amp (2, 300, 100, 400).delay (eighth, 45, 20, 2700).level (-12)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Gate").mod (macro2, "ampLevel", 14).reverb (0, 70, 3, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Looping Mesh", "Sequence").osc (1, metal, 20).unison (1, 1, 18, 70).filter (1, ladder12, 790, 25, 30).lfoSync (1, steps, bar1, 1, 89)
            .stepPattern (1, { -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0 }).mod (lfo1, "ampLevel", 75)
            .lfoSync (2, sine, halfT, 0).mod (lfo2, "filter1Cutoff", 14).amp (2, 300, 100, 200).delay (sixteenthD, 40, 10, 7500).level (-8)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Gate").mod (macro2, "ampLevel", 14).reverb (0, 70, 3, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Looping Shutter", "Sequence").osc (1, analog, squarePos).unison (1, 3, 14, 90).osc (2, pulse, 10, 50).pitch (2, 7, 7)
            .filter (1, ladder12, 3000, 45).lfoSync (1, steps, bars2, 1, 86)
            .stepPattern (1, { 0, -100, -100, -100, 0, -100, 0, -100, 0, -100, -100, -100, 0, -100, 0, -50 }).mod (lfo1, "ampLevel", 75)
            .lfo (2, sine, 0.095f, 0).mod (lfo2, "filter1Cutoff", 6).amp (2, 300, 100, 350).delay (sixteenthD, 30, 18, 3100, false)
            .reverb (20, 70, 7.5f, 70).level (-16).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Gate")
            .mod (macro2, "ampLevel", 14).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Lurching Louvre", "Sequence").osc (1, fm, 45).unison (1, 2, 16, 90).filter (1, ladder12, 1300, 15, 10).lfoSync (1, steps, half, 1, 92)
            .stepPattern (1, { 0, -25, -25, -25, 0, -25, -25, -25, 0, -25, -25, -25, 0, -25, -25, -25 }).mod (lfo1, "ampLevel", 70)
            .lfo (2, sine, 0.058f, 0).mod (lfo2, "filter1Cutoff", 16).amp (2, 300, 100, 350).drive (tube, 15, 4200).delay (eighth, 35, 18, 3900)
            .level (-15).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Gate").mod (macro2, "ampLevel", 14).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Marked Heave", "Sequence").osc (1, vowel, 65).unison (1, 4, 12).filter (1, ladder12, 2100).lfoSync (1, steps, half, 1, 96)
            .stepPattern (1, { -100, -90, -75, -60, -45, -30, -15, 0, 0, 0, 0, 0, 0, 0, 0, 0 }).mod (lfo1, "ampLevel", 50)
            .mod (lfo1, "filter1Cutoff", 10).amp (200, 300, 100, 600).reverb (35, 70, 4.5f, 55).level (-10).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Duck").mod (macro2, "ampLevel", 14).drive (tube, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Marked Piston", "Sequence").osc (1, fm, 55).unison (1, 3, 14, 60).subOsc (40).filter (1, ladder24, 920, 25)
            .lfoSync (2, steps, quarter, 1, 99).stepPattern (2, { 0, -15, -40, -70, -100, -100, -100, -100, 0, -15, -40, -70, 0, -15, -40, -70 })
            .mod (lfo2, "ampLevel", 81).mod (lfo2, "filter1Cutoff", 18).lfoSync (1, steps, bar1, 1, 50)
            .stepPattern (1, { 0, 0, 100, 100, 70, 0, 70, 40, 0, -40, 100, -40, 0, -40, 100, 0 }).mod (lfo1, "filter1Cutoff", 14)
            .amp (2, 300, 100, 200).drive (tube, 40, 4700).delay (sixteenthD, 45, 14, 1900).level (-21).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 25).reverb (0, 50, 1.4f, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Packed Routine", "Sequence").osc (1, analog, squarePos).subOsc (1).filter (1, ladder24, 530, 65, 50).lfoSync (1, steps, bar1, 1, 60)
            .stepPattern (1, { 60, 100, 60, -30, -60, -30, 0, 30, 60, -30, 0, -60, 0, 30, -60, -100 }).mod (lfo1, "filter1Cutoff", 12)
            .amp (2, 300, 100, 180).mono().drive (hard, 50, 6100).level (-19).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 18).macro (3, "Depth").mod (macro3, "filter1Drive", 55).macro (4, "Sub")
            .mod (macro4, "subLevel", 40);
        add ("Prowling Lung", "Sequence").osc (1, vowel, 35).unison (1, 4, 12, 90).subOsc (50).filter (1, svfLow, 2900, 40, 20)
            .lfoSync (1, steps, quarter, 1, 99).stepPattern (1, { -100, -60, -30, -10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 })
            .mod (lfo1, "ampLevel", 80).mod (lfo1, "filter1Cutoff", 6).amp (50, 300, 100, 600).chorus (20, 0.7f, 50).reverb (35, 100, 6, 35)
            .level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Duck").mod (macro2, "ampLevel", 14)
            .delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Pulsing Reader", "Sequence").osc (1, sync, 50).unison (1, 2, 14, 90).subOsc (30).lfoSync (1, steps, bars2, 1, 60)
            .stepPattern (1, { 100, -70, -70, -100, 100, 80, -70, -70, 100, 80, 80, -100, 100, 80, -100, 80 }).mod (lfo1, "osc1Position", 45)
            .lfoSync (2, steps, bars2, 1, 93).stepPattern (2, { 0, -100, 0, 0, -100, -100, 0, -100, 0, -100, 0, 0, -100, -100, 0, -30 })
            .mod (lfo2, "ampLevel", 40).filter (1, ladder12, 1600, 25).amp (2, 300, 100, 140).drive (hard, 25, 4200)
            .delay (sixteenthD, 30, 14, 4400).level (-16).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Range")
            .mod (macro2, "osc1Position", 30).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Pulsing Turnstile", "Sequence").osc (1, vowel, 55).unison (1, 2, 8, 70).filter (1, svfLow, 1200, 15, 30)
            .lfoSync (1, steps, bars2, 1, 89).stepPattern (1, { 0, -100, -25, -100, 0, -100, -25, -100, 0, -25, 0, -100, -25, -100, 0, -25 })
            .mod (lfo1, "ampLevel", 70).lfo (2, triangle, 0.11f, 0).mod (lfo2, "filter1Cutoff", 16).amp (2, 300, 100, 300)
            .delay (eighthD, 45, 16, 6000).reverb (28, 90, 5, 55).level (-10).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28)
            .macro (2, "Gate").mod (macro2, "ampLevel", 14).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Rancid Ratchet", "Sequence").osc (1, analog, squarePos).unison (1, 2, 16, 70).subOsc (30).filter (1, ladder24, 980, 55, 10)
            .lfoSync (2, steps, quarter, 1, 98)
            .stepPattern (2, { 0, -40, -80, -100, -20, -60, -100, -100, 0, -40, -80, -100, -100, -100, -100, -100 }).mod (lfo2, "ampLevel", 80)
            .mod (lfo2, "filter1Cutoff", 14).lfoSync (1, steps, bar1, 1, 50)
            .stepPattern (1, { -40, 40, 70, 0, 70, 0, 0, 40, 0, 100, 0, 40, 100, 70, 40, 40 }).mod (lfo1, "filter1Cutoff", 16)
            .amp (2, 300, 100, 160).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 25).reverb (0, 50, 1.4f, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 30).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Restless Heave", "Sequence").osc (1, metal, 60).unison (1, 2, 8, 50).osc (2, pulse, 15, 50).pitch (2, -12, 9)
            .filter (1, ladder12, 3200, 25).lfoSync (1, steps, quarter, 1, 95)
            .stepPattern (1, { -100, -80, -55, -35, -20, -10, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0 }).mod (lfo1, "ampLevel", 80)
            .mod (lfo1, "filter1Cutoff", 4).amp (200, 300, 100, 400).reverb (30, 95, 7.5f, 35).level (-9).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 28).macro (2, "Duck").mod (macro2, "ampLevel", 14).delay (eighthD, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Scalding Ratchet", "Sequence").osc (1, vowel, 50).unison (1, 3, 18, 70).subOsc (35).filter (1, ladder24, 620, 30, 30)
            .lfoSync (2, steps, quarter, 1, 99)
            .stepPattern (2, { 0, -40, -80, -100, -10, -50, -90, -100, -20, -60, -100, -100, -10, -50, -90, -100 }).mod (lfo2, "ampLevel", 78)
            .mod (lfo2, "filter1Cutoff", 14).lfoSync (1, steps, bar1, 1, 50)
            .stepPattern (1, { 100, 0, 40, 0, -40, 100, 70, 100, 100, 100, 100, 40, 0, 0, -40, -40 }).mod (lfo1, "filter1Cutoff", 10)
            .amp (2, 300, 100, 180).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 25).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Accent")
            .mod (macro4, "filter1Drive", 55);
        add ("Severed Escapement", "Sequence").osc (1, metal, 30).unison (1, 3, 8, 60).osc (2, pulse, 15, 55).pitch (2, 7, 7).subOsc (45)
            .filter (1, ladder24, 1400, 45, 30).lfoSync (2, steps, half, 1, 97)
            .stepPattern (2, { 0, -15, -40, -70, -100, -100, -100, -100, 0, -15, -40, -70, 0, -15, -40, -70 }).mod (lfo2, "ampLevel", 76)
            .mod (lfo2, "filter1Cutoff", 12).lfoSync (1, steps, bars2, 1, 50)
            .stepPattern (1, { 40, 70, 100, 100, 0, 40, 40, 0, -40, 0, 70, 0, 40, -40, 0, 100 }).mod (lfo1, "filter1Cutoff", 14)
            .amp (2, 300, 100, 80).drive (hard, 25, 3100).level (-18).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Accent").mod (macro3, "filter1Drive", 55).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Shifting Script", "Sequence").osc (1, analog, squarePos).subOsc (40).osc (2, analog, sawPos, 55).pitch (2, 7, 8)
            .filter (1, ladder24, 330, 75, 30).lfoSync (1, steps, bars2, 1, 90)
            .stepPattern (1, { 30, 30, 30, -30, -30, 0, 0, 0, 30, 30, 30, -30, -30, 0, 0, 0 }).mod (lfo1, "filter1Cutoff", 24)
            .amp (2, 300, 100, 200).mono().drive (tube, 45, 3500).delay (eighthD, 45, 22, 2300, false).level (-19).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 18).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Depth").mod (macro4, "filter1Drive", 55);
        add ("Smothered Advance", "Sequence").osc (1, sync, 15).subOsc (45).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 0, 50, 50, 0, 0, 50, 0, 0, 0, 50, 50, 0, 0, 50, 0 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, quarter, 1, 97).stepPattern (2, { 0, 0, -50, -100, 0, 0, -50, -100, 0, 0, -50, -100, 0, 0, -50, -100 })
            .mod (lfo2, "ampLevel", 80).mod (lfo2, "filter1Cutoff", 18).filter (1, ladder24, 440, 50, 50).amp (2, 300, 100, 90).mono().level (-4)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 25).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Spinning Drill", "Sequence").osc (1, sync, 60).subOsc (35).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 50, 0, 0, 50, 0, 50, 0, 0, 50, 0, 0, 50, 0, 50, 0 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, quarter, 1, 97).stepPattern (2, { 0, 0, -50, -100, 0, 0, -50, -100, 0, 0, -50, -100, 0, 0, -50, -100 })
            .mod (lfo2, "ampLevel", 85).mod (lfo2, "filter1Cutoff", 22).filter (1, ladder12, 580, 50, 20).amp (2, 300, 100, 90).mono()
            .drive (fuzz, 25, 2900).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Spinning Mesh", "Sequence").osc (1, analog, squarePos).unison (1, 1, 14, 60).filter (1, svfBand, 660, 40, 30)
            .lfoSync (1, steps, bar1, 1, 86).stepPattern (1, { 0, -100, -100, 0, -100, 0, -100, -100, 0, -100, -100, 0, -100, 0, -100, -100 })
            .mod (lfo1, "ampLevel", 70).lfoSync (2, sawUp, eighthD, 0).mod (lfo2, "filter1Cutoff", 10).amp (2, 300, 100, 240)
            .delay (sixteenthD, 25, 14, 2100).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Gate")
            .mod (macro2, "ampLevel", 14).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Stalking Ledger", "Sequence").osc (1, analog, sawPos).subOsc (40).filter (1, ladder24, 330, 80, 40).lfoSync (1, steps, bars2, 1, 50)
            .stepPattern (1, { -100, 30, 100, -30, -30, 30, 100, -30, -100, 30, 100, -30, -30, 30, 100, -30 }).mod (lfo1, "filter1Cutoff", 20)
            .amp (2, 300, 100, 180).mono().drive (hard, 30, 5800).delay (sixteenthD, 50, 16, 3800).level (-15).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 18).macro (3, "Depth")
            .mod (macro3, "filter1Drive", 55).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Stalking Louvre", "Sequence").osc (1, fm, 65).unison (1, 1, 16).filter (1, ladder24, 930, 30).lfoSync (1, steps, bars2, 1, 89)
            .stepPattern (1, { 0, 0, 0, -100, 0, 0, 0, -100, 0, 0, 0, -100, 0, 0, 0, -100 }).mod (lfo1, "ampLevel", 70).lfo (2, triangle, 0.097f, 0)
            .mod (lfo2, "filter1Cutoff", 16).amp (2, 300, 100, 200).delay (eighthD, 35, 16, 4800, false).reverb (12, 95, 3, 30).level (-12)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Gate").mod (macro2, "ampLevel", 14).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Stalking Patrol", "Sequence").osc (1, sync, 35).osc (2, analog, sawPos, 70).pitch (2, 0, 11).subOsc (40).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 0, 0, 0, 50, 0, 0, 50, 0, 0, 0, 0, 50, 0, 0, 50 }).mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50)
            .lfoSync (2, steps, quarter, 1, 98).stepPattern (2, { 0, 0, -50, -100, 0, 0, -50, -100, 0, 0, -50, -100, 0, 0, -50, -100 })
            .mod (lfo2, "ampLevel", 74).mod (lfo2, "filter1Cutoff", 10).filter (1, ladder24, 830, 20, 40).amp (2, 300, 100, 70).mono().level (-3)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 25).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Stuttering Braid", "Sequence").osc (1, metal, 20).unison (1, 4, 10, 70).osc (2, analog, squarePos, 60).pitch (2, -12, 7)
            .filter (1, ladder24, 690, 50, 20).lfoSync (1, steps, bar1, 1, 92)
            .stepPattern (1, { 0, -30, 0, -100, 0, -30, 0, -100, 0, -30, 0, -100, 0, -30, 0, -30 }).mod (lfo1, "ampLevel", 85)
            .lfoSync (2, triangle, eighthD, 0).mod (lfo2, "filter1Cutoff", 12).amp (2, 300, 100, 120).drive (tube, 15, 4500)
            .delay (quarterT, 35, 12, 6300, false).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Gate")
            .mod (macro2, "ampLevel", 14).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Stuttering Weave", "Sequence").osc (1, metal, 70).unison (1, 2, 16, 60).filter (1, ladder12, 1800, 55, 20)
            .lfoSync (1, steps, half, 1, 95).stepPattern (1, { 0, -100, 0, -30, 0, -100, 0, -30, 0, -100, 0, -30, 0, -100, 0, -30 })
            .mod (lfo1, "ampLevel", 65).lfoSync (2, triangle, quarterD, 0).mod (lfo2, "filter1Cutoff", 16).amp (2, 300, 100, 240)
            .drive (tube, 25, 4000).delay (quarterT, 45, 10, 3200).level (-15).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Gate").mod (macro2, "ampLevel", 14).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 25);
        add ("Sunken Index", "Sequence").osc (1, analog, sawPos).subOsc (40).filter (1, ladder24, 260, 80, 10).lfoSync (1, steps, half, 1, 80)
            .stepPattern (1, { -100, 0, -60, 0, 30, -100, -60, 100, -100, 0, -60, 0, 30, -100, -60, 100 }).mod (lfo1, "filter1Cutoff", 14)
            .amp (2, 300, 100, 140).mono().level (-6).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 18).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Depth")
            .mod (macro4, "filter1Drive", 55);
        add ("Tempered Flywheel", "Sequence").osc (1, metal, 65).unison (1, 2, 16, 60).osc (2, pulse, 65, 65).pitch (2, 12, 5).subOsc (30)
            .filter (1, svfLow, 330, 30, 30).lfoSync (2, steps, quarter, 1, 98)
            .stepPattern (2, { 0, -25, -55, -100, -100, -100, -100, -100, 0, -25, -55, -100, 0, -25, -55, -100 }).mod (lfo2, "ampLevel", 83)
            .mod (lfo2, "filter1Cutoff", 10).lfoSync (1, steps, bar1, 1, 50)
            .stepPattern (1, { -40, 0, 100, 100, 40, 0, 100, 70, 0, 40, 0, 70, 0, 100, 40, 0 }).mod (lfo1, "filter1Cutoff", 12)
            .amp (2, 300, 100, 180).drive (hard, 45, 7500).delay (sixteenthD, 45, 10, 3100).level (-20).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 25).macro (3, "Accent")
            .mod (macro3, "filter1Drive", 55).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Ticking Piston", "Sequence").osc (1, pulse, 50).unison (1, 3, 10, 50).osc (2, pulse, 60, 60).pitch (2, -12, 5).subOsc (45)
            .filter (1, ladder24, 330, 25, 10).lfoSync (2, steps, half, 1, 98)
            .stepPattern (2, { 0, -15, -40, -70, -10, -25, -50, -80, -20, -35, -60, -90, -10, -25, -50, -80 }).mod (lfo2, "ampLevel", 85)
            .mod (lfo2, "filter1Cutoff", 20).lfoSync (1, steps, bars2, 1, 50)
            .stepPattern (1, { 0, 0, -40, 0, 70, 0, -40, -40, 0, 40, 70, 100, 0, 40, -40, 40 }).mod (lfo1, "filter1Cutoff", 16)
            .amp (2, 300, 100, 200).drive (tube, 20, 5100).delay (sixteenthD, 25, 16, 7500).level (-16).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 25).reverb (0, 50, 1.4f, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 30).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Ticking Script", "Sequence").osc (1, analog, squarePos).subOsc (25).filter (1, ladder24, 310, 60, 50).lfoSync (1, steps, bar1, 1, 80)
            .stepPattern (1, { -100, 30, 100, 60, 100, 0, 0, -100, -30, -60, -60, -100, -60, -100, -30, -100 }).mod (lfo1, "filter1Cutoff", 16)
            .amp (2, 300, 100, 140).mono().delay (sixteenthD, 55, 16, 6900).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 18).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Ticking Swell", "Sequence").osc (1, sync, 65).unison (1, 2, 12, 70).subOsc (50).filter (1, ladder24, 1200, 20, 10)
            .lfoSync (1, steps, half, 1, 95).stepPattern (1, { -100, -100, -60, -60, -30, -30, -10, -10, 0, 0, 0, 0, 0, 0, 0, 0 })
            .mod (lfo1, "ampLevel", 80).mod (lfo1, "filter1Cutoff", 10).amp (50, 300, 100, 700).reverb (25, 95, 3, 65).level (-11)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Duck").mod (macro2, "ampLevel", 14).delay (eighthD, 40, 0, 4500)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Twitching Advance", "Sequence").osc (1, sync, 30).subOsc (30).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 0, 0, 50, 0, 0, 50, 50, 0, 0, 0, 50, 0, 0, 50, 50 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, quarter, 1, 97).stepPattern (2, { 0, -45, -90, -100, 0, -45, -90, -100, 0, -45, -90, -100, 0, -45, -90, -100 })
            .mod (lfo2, "ampLevel", 81).mod (lfo2, "filter1Cutoff", 12).filter (1, ladder24, 870, 50, 40).amp (2, 300, 100, 120).mono()
            .drive (tube, 35, 5700).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Twitching Louvre", "Sequence").osc (1, metal, 15).unison (1, 4, 8, 50).osc (2, analog, sawPos, 45).pitch (2, -12, 6)
            .filter (1, ladder24, 1000, 30).lfoSync (1, steps, bar1, 1, 89)
            .stepPattern (1, { 0, -100, 0, -50, 0, -100, 0, -50, 0, -100, 0, -50, 0, -100, 0, -50 }).mod (lfo1, "ampLevel", 75)
            .lfo (2, triangle, 0.16f, 0).mod (lfo2, "filter1Cutoff", 14).amp (2, 300, 100, 300).delay (eighthD, 40, 12, 4100, false)
            .reverb (28, 75, 8.5f, 70).level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Gate")
            .mod (macro2, "ampLevel", 14).macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Twitching Swell", "Sequence").osc (1, vowel, 55).unison (1, 1, 14, 70).filter (1, ladder24, 830, 15, 10)
            .lfoSync (1, steps, quarter, 1, 99).stepPattern (1, { -100, -100, -80, -80, -55, -55, -35, -35, -20, -20, -10, -10, -4, -4, 0, 0 })
            .mod (lfo1, "ampLevel", 75).mod (lfo1, "filter1Cutoff", 12).amp (50, 300, 100, 200).reverb (20, 75, 3, 45).level (-6)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 28).macro (2, "Duck").mod (macro2, "ampLevel", 14).delay (eighthD, 40, 0, 4500)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Twitching Tracer", "Sequence").osc (1, fm, 50).unison (1, 2, 16, 70).subOsc (30).lfoSync (1, steps, bars2, 1, 40)
            .stepPattern (1, { -10, -100, 100, -10, 80, 100, 80, 20, -40, -100, 100, -70, -40, 50, -70, 50 }).mod (lfo1, "osc1Position", 48)
            .lfoSync (2, steps, bars2, 1, 90).stepPattern (2, { 0, -100, -100, -100, 0, -100, 0, -100, 0, -100, -100, -100, 0, -100, 0, -30 })
            .mod (lfo2, "ampLevel", 40).filter (1, ladder24, 1500, 35).amp (2, 300, 100, 220).delay (sixteenthD, 25, 16, 3100).level (-14)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Range").mod (macro2, "osc1Position", 30).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Vagrant Charge", "Sequence").osc (1, sync, 45).subOsc (35).lfoSync (1, steps, bar1)
            .stepPattern (1, { 0, 0, 50, 0, 50, 0, 0, 50, 0, 50, 0, 50, 50, 0, 0, 0 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, steps, quarter, 1, 96).stepPattern (2, { 0, -45, -90, -100, 0, -45, -90, -100, 0, -45, -90, -100, 0, -45, -90, -100 })
            .mod (lfo2, "ampLevel", 80).mod (lfo2, "filter1Cutoff", 20).filter (1, ladder24, 420, 35, 30).amp (2, 300, 100, 130).mono()
            .drive (hard, 25, 7200).level (-3).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Vigilant Escapement", "Sequence").osc (1, sync, 65).unison (1, 3, 10, 70).subOsc (45).filter (1, svfLow, 1200, 30, 10)
            .lfoSync (2, steps, quarter, 1, 99)
            .stepPattern (2, { 0, -50, -100, -100, -20, -70, -100, -100, 0, -50, -100, -100, -100, -100, -100, -100 }).mod (lfo2, "ampLevel", 80)
            .mod (lfo2, "filter1Cutoff", 16).lfoSync (1, steps, bar1, 1, 50)
            .stepPattern (1, { 0, -40, 70, 40, 0, 70, 40, 40, 0, 0, 40, 0, 70, 100, 40, -40 }).mod (lfo1, "filter1Cutoff", 16)
            .amp (2, 300, 100, 200).drive (tube, 20, 5200).delay (sixteenthD, 55, 8, 3000).level (-14).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 25).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Accent").mod (macro4, "filter1Drive", 55);
        add ("Wandering Register", "Sequence").osc (1, analog, squarePos).subOsc (40).filter (1, ladder12, 850, 60, 30)
            .lfoSync (1, steps, bar1, 1, 40).stepPattern (1, { 100, -100, -100, 60, 30, 0, -30, 30, 100, -100, -100, 60, 30, 0, -30, 30 })
            .mod (lfo1, "filter1Cutoff", 24).amp (2, 300, 100, 140).mono().drive (fuzz, 20, 3700).delay (sixteenthD, 55, 16, 2300).level (-22)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Resonance").mod (macro2, "filter1Resonance", 18).macro (3, "Sub")
            .mod (macro3, "subLevel", 40).macro (4, "Depth").mod (macro4, "filter1Drive", 55);
        add ("Warped Tracer", "Sequence").osc (1, sync, 50).unison (1, 3, 16, 70).subOsc (40).lfoSync (1, steps, half, 1, 40)
            .stepPattern (1, { -100, -70, -100, -100, 80, -70, -10, 100, 50, -100, -70, -100, 80, -70, 20, -10 }).mod (lfo1, "osc1Position", 36)
            .lfoSync (2, steps, bars2, 1, 88)
            .stepPattern (2, { -100, -100, 0, -100, -100, -100, 0, -100, -100, -100, 0, -100, -100, -100, 0, -100 }).mod (lfo2, "ampLevel", 50)
            .filter (1, svfLow, 4200, 30).amp (2, 300, 100, 200).delay (sixteenthD, 45, 10, 2900).level (-13).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Range").mod (macro2, "osc1Position", 30).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Watched Mesh", "Sequence").osc (1, metal, 55).unison (1, 4, 14, 50).filter (1, svfBand, 670, 50, 10).lfoSync (1, steps, bar1, 1, 92)
            .stepPattern (1, { 0, -100, 0, 0, -100, -100, 0, -100, 0, -100, 0, 0, -100, -100, 0, -30 }).mod (lfo1, "ampLevel", 85)
            .lfoSync (2, sawDown, halfT, 0).mod (lfo2, "filter1Cutoff", 20).amp (2, 300, 100, 220).delay (quarterT, 55, 18, 3300).level (-3)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Gate").mod (macro2, "ampLevel", 14).drive (tube, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).reverb (0, 70, 3, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);

        // Riser, by name.
        add ("Attic Climb", "Riser").osc (1, analog, sawPos).warp (1, wSync, 0).unison (1, 2, 10).noise (20, 1).env (2, 6000, 1000, 100, 500, -60)
            .mod (env2, "osc1WarpAmount", 65).mod (env2, "filter1Cutoff", 30).filter (1, ladder24, 320, 45).amp (1300, 1000, 100, 900, 30)
            .reverb (40, 90, 4, 35).level (-15).macro (1, "Sync").mod (macro1, "osc1WarpAmount", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Blackened Hoist", "Riser").osc (1, analog, sawPos).warp (1, wSync, 0).unison (1, 3, 14, 90).env (2, 6500, 1000, 100, 500, 0)
            .mod (env2, "osc1WarpAmount", 80).mod (env2, "filter1Cutoff", 25).filter (1, ladder24, 330, 45).amp (1300, 1000, 100, 600, -30)
            .reverb (40, 70, 7.5f, 30).level (-17).macro (1, "Sync").mod (macro1, "osc1WarpAmount", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Bleached Lift", "Riser").osc (1, analog, sawPos).warp (1, wSync, 10).unison (1, 4, 14, 70).env (2, 7500, 1000, 100, 500, -20)
            .mod (env2, "osc1WarpAmount", 70).mod (env2, "filter1Cutoff", 40).filter (1, svfHigh, 280, 25).amp (1600, 1000, 100, 500, -10)
            .reverb (45, 70, 3.5f, 35).level (-20).macro (1, "Sync").mod (macro1, "osc1WarpAmount", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Border Horde", "Riser").osc (1, sync, 30).unison (1, 8, 3, 100).pitch (1, 0, -100).osc (2, analog, sawPos, 45).pitch (2, -12)
            .unison (2, 4, 8, 100).env (2, 7500, 1000, 100, 500, 20).mod (env2, "osc1UnisonDetune", 80).mod (env2, "osc1Fine", 50)
            .mod (env2, "filter1Cutoff", 35).filter (1, svfLow, 420, 40).amp (1500, 1000, 100, 600, 0).reverb (30, 100, 8, 35).level (-18)
            .macro (1, "Scatter").mod (macro1, "osc1UnisonDetune", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20)
            .delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Caustic Blast", "Riser").osc (1, analog, sawPos, 45).unison (1, 2, 16, 100).noise (55).filter (1, comb, 150, 84)
            .filter (2, svfBand, 1300, 35).env (2, 8000, 1000, 100, 500, 10).mod (env2, "filter1Cutoff", 45).mod (env2, "filter2Cutoff", 20)
            .amp (1800, 1000, 100, 400, -20).reverb (35, 75, 8, 55).level (-4).macro (1, "Ring").mod (macro1, "filter1Resonance", 7)
            .macro (2, "Tone").mod (macro2, "filter2Cutoff", 20).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Air").mod (macro4, "noiseLevel", -40);
        add ("Cavernous Flutter", "Riser").osc (1, fm, 15).unison (1, 3, 16).lfo (1, triangle, 1.9f).mod (lfo1, "ampLevel", 24)
            .env (2, 6500, 1000, 100, 500, 10).mod (env2, "lfo1Rate", 25).mod (env2, "filter1Cutoff", 35).filter (1, ladder24, 380, 15)
            .amp (1000, 1000, 100, 600, -10).drive (tube, 35, 2700).reverb (45, 85, 7.5f, 30).level (-37).macro (1, "Depth")
            .mod (macro1, "ampLevel", 12).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Cracked Sinking", "Riser").osc (1, analog, 30, 70).unison (1, 4, 12, 100).noise (60, 1).filter (1, ladder24, 190, 92)
            .env (2, 0, 4000, 0, 4000, 70).mod (env2, "filter1Cutoff", 55).amp (0, 4000, 0, 3000, 60).reverb (50, 70, 5.5f, 50).level (-6)
            .macro (1, "Resonance").mod (macro1, "filter1Resonance", 8).macro (2, "Height").mod (macro2, "filter1Cutoff", 15).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Air").mod (macro4, "noiseLevel", -35);
        add ("Crooked Blast", "Riser").osc (1, analog, sawPos, 45).unison (1, 4, 16, 100).noise (75, 1).filter (1, comb, 110, 92)
            .filter (2, svfHigh, 360, 30).env (2, 4000, 1000, 100, 500, -50).mod (env2, "filter1Cutoff", 25).mod (env2, "filter2Cutoff", 15)
            .amp (1000, 1000, 100, 900, 0).reverb (40, 95, 7, 65).level (-10).macro (1, "Ring").mod (macro1, "filter1Resonance", 7)
            .macro (2, "Tone").mod (macro2, "filter2Cutoff", 20).macro (3, "Air").mod (macro3, "noiseLevel", -40).delay (eighthD, 40, 0, 4500)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Distant Legion", "Riser").osc (1, analog, 30).unison (1, 8, 0, 100, 0, 20).pitch (1, 0, -100).osc (2, analog, sawPos, 60)
            .pitch (2, 12).unison (2, 3, 9, 100).env (2, 4000, 1000, 100, 500, 0).mod (env2, "osc1UnisonDetune", 65).mod (env2, "osc1Fine", 90)
            .mod (env2, "filter1Cutoff", 30).filter (1, ladder24, 470, 25).amp (1700, 1000, 100, 500, 0).reverb (35, 85, 9, 30).level (-18)
            .macro (1, "Scatter").mod (macro1, "osc1UnisonDetune", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("District Drain", "Riser").osc (1, pulse, 66.67f, 55).unison (1, 3, 14, 100).noise (50).filter (1, ladder24, 190, 76)
            .env (2, 0, 3000, 0, 3000, 30).mod (env2, "filter1Cutoff", 60).amp (0, 3000, 0, 2000, 60).delay (quarterT, 30, 20, 6700)
            .reverb (30, 75, 3.5f, 35).level (-4).macro (1, "Resonance").mod (macro1, "filter1Resonance", 8).macro (2, "Height")
            .mod (macro2, "filter1Cutoff", 15).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Air").mod (macro4, "noiseLevel", -35);
        add ("Dormant Flutter", "Riser").osc (1, vowel, 70).unison (1, 1, 18).lfo (1, sine, 3.6f).mod (lfo1, "ampLevel", 14)
            .env (2, 5500, 1000, 100, 500, -30).mod (env2, "lfo1Rate", 40).mod (env2, "filter1Cutoff", 35).filter (1, ladder24, 700, 35)
            .amp (1300, 1000, 100, 800, -20).reverb (35, 90, 7, 60).level (-20).macro (1, "Depth").mod (macro1, "ampLevel", 12).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Factory Judder", "Riser").osc (1, fm, 45).unison (1, 4, 8, 70).lfo (1, triangle, 3.1f).mod (lfo1, "ampLevel", 20)
            .env (2, 7000, 1000, 100, 500, -40).mod (env2, "lfo1Rate", 40).mod (env2, "filter1Cutoff", 35).filter (1, svfBand, 420, 15)
            .amp (1600, 1000, 100, 500, 10).reverb (45, 100, 9, 35).level (-23).macro (1, "Depth").mod (macro1, "ampLevel", 12).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).macro (3, "Space").mod (macro3, "fxReverbMix", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Inland Afterburner", "Riser").osc (1, analog, sawPos, 45).unison (1, 4, 18, 100).noise (70).filter (1, comb, 64, 90)
            .filter (2, svfHigh, 440, 35).env (2, 5500, 1000, 100, 500, -20).mod (env2, "filter1Cutoff", 50).mod (env2, "filter2Cutoff", 25)
            .amp (1800, 1000, 100, 800, 0).reverb (40, 75, 3, 45).level (-8).macro (1, "Ring").mod (macro1, "filter1Resonance", 7).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 20).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Lone Collapse", "Riser").osc (1, analog, 40, 70).unison (1, 3, 10, 100).noise (40, 1).filter (1, ladderBand, 150, 76, 10)
            .env (2, 0, 3000, 0, 3000, 20).mod (env2, "filter1Cutoff", 40).amp (0, 3000, 0, 2500, 20).reverb (35, 100, 5.5f, 65).level (1)
            .macro (1, "Resonance").mod (macro1, "filter1Resonance", 8).macro (2, "Height").mod (macro2, "filter1Cutoff", 15).macro (3, "Air")
            .mod (macro3, "noiseLevel", -35).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Lone Rattle", "Riser").osc (1, metal, 60).unison (1, 2, 8).lfo (1, sine, 2.2f).mod (lfo1, "ampLevel", 14)
            .env (2, 6000, 1000, 100, 500, -20).mod (env2, "lfo1Rate", 30).mod (env2, "filter1Cutoff", 40).filter (1, ladder24, 330, 40)
            .amp (800, 1000, 100, 800, -30).drive (hard, 40, 8600).reverb (35, 75, 6, 65).level (-22).macro (1, "Depth")
            .mod (macro1, "ampLevel", 12).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Lunar Judder", "Riser").osc (1, sync, 35).unison (1, 3, 16, 60).lfo (1, sine, 3.4f).mod (lfo1, "ampLevel", 20)
            .env (2, 5500, 1000, 100, 500, -30).mod (env2, "lfo1Rate", 45).mod (env2, "filter1Cutoff", 40).filter (1, svfBand, 510, 45)
            .amp (1500, 1000, 100, 800, -30).drive (tube, 35, 9700).reverb (45, 90, 7.5f, 65).level (-32).macro (1, "Depth")
            .mod (macro1, "ampLevel", 12).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Nomad Turbine", "Riser").osc (1, analog, sawPos, 45).unison (1, 4, 22, 100).noise (55).filter (1, comb, 110, 86)
            .filter (2, ladder24, 410, 20).env (2, 8000, 1000, 100, 500, -40).mod (env2, "filter1Cutoff", 50).mod (env2, "filter2Cutoff", 20)
            .amp (1700, 1000, 100, 400, 20).reverb (30, 100, 6.5f, 40).level (-9).macro (1, "Ring").mod (macro1, "filter1Resonance", 7)
            .macro (2, "Tone").mod (macro2, "filter2Cutoff", 20).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30)
            .macro (4, "Air").mod (macro4, "noiseLevel", -40);
        add ("Ochre Plague", "Riser").osc (1, analog, 30).unison (1, 8, 2, 100, 0, 40).pitch (1, 0, -100).osc (2, analog, sawPos, 55).pitch (2, 7)
            .unison (2, 4, 5, 100).env (2, 4000, 1000, 100, 500, -40).mod (env2, "osc1UnisonDetune", 70).mod (env2, "osc1Fine", 100)
            .mod (env2, "filter1Cutoff", 30).filter (1, ladder24, 310, 20).amp (1500, 1000, 100, 500, -10).reverb (35, 85, 8.5f, 35).level (-17)
            .macro (1, "Scatter").mod (macro1, "osc1UnisonDetune", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20)
            .delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Outpost Horde", "Riser").osc (1, pulse, 50).unison (1, 8, 1, 100, 0, 40).pitch (1, 0, -100).osc (2, analog, sawPos, 55).pitch (2, -12)
            .unison (2, 3, 10, 100).env (2, 6500, 1000, 100, 500, -40).mod (env2, "osc1UnisonDetune", 55).mod (env2, "osc1Fine", 100)
            .mod (env2, "filter1Cutoff", 35).filter (1, svfLow, 740, 15).amp (1200, 1000, 100, 800, 10).reverb (45, 90, 3, 40).level (-23)
            .macro (1, "Scatter").mod (macro1, "osc1UnisonDetune", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20)
            .delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Rabid Horde", "Riser").osc (1, sync, 50).unison (1, 8, 0, 100, 0, 40).pitch (1, 0, -100).osc (2, analog, sawPos, 45).pitch (2, 7)
            .unison (2, 4, 10, 100).env (2, 7000, 1000, 100, 500, -20).mod (env2, "osc1UnisonDetune", 55).mod (env2, "osc1Fine", 70)
            .mod (env2, "filter1Cutoff", 25).filter (1, svfBand, 1100, 35).amp (1000, 1000, 100, 500, 20).reverb (35, 85, 7.5f, 70).level (-16)
            .macro (1, "Scatter").mod (macro1, "osc1UnisonDetune", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Rabid Judder", "Riser").osc (1, analog, squarePos).unison (1, 1, 14, 60).osc (2, pulse, 65, 50).pitch (2, 12, 7).lfo (1, sine, 2.2f)
            .mod (lfo1, "ampLevel", 22).env (2, 7000, 1000, 100, 500, -30).mod (env2, "lfo1Rate", 45).mod (env2, "filter1Cutoff", 20)
            .filter (1, svfLow, 510, 30).amp (900, 1000, 100, 900, 30).drive (hard, 15, 8200).reverb (40, 95, 9, 35).level (-34).macro (1, "Depth")
            .mod (macro1, "ampLevel", 12).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Raw Hoist", "Riser").osc (1, analog, sawPos).warp (1, wSync, 0).unison (1, 3, 12, 100).env (2, 7500, 1000, 100, 500, 0)
            .mod (env2, "osc1WarpAmount", 85).mod (env2, "filter1Cutoff", 20).filter (1, ladder24, 830, 20).amp (1100, 1000, 100, 900, 20)
            .delay (eighthT, 40, 18, 6000, false).reverb (30, 100, 5, 35).level (-16).macro (1, "Sync").mod (macro1, "osc1WarpAmount", 30)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Raw Thruster", "Riser").osc (1, analog, sawPos, 50).unison (1, 2, 16, 100).noise (60, 1).filter (1, comb, 77, 88)
            .filter (2, ladder24, 390, 30).env (2, 3500, 1000, 100, 500, -40).mod (env2, "filter1Cutoff", 35).mod (env2, "filter2Cutoff", 20)
            .amp (1100, 1000, 100, 400, 30).reverb (45, 75, 7, 35).level (-13).macro (1, "Ring").mod (macro1, "filter1Resonance", 7)
            .macro (2, "Tone").mod (macro2, "filter2Cutoff", 20).delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30)
            .drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Remote Horde", "Riser").osc (1, analog, 30).unison (1, 8, 2, 100, 0, 20).pitch (1, 0, -100).osc (2, analog, sawPos, 45).pitch (2, 7)
            .unison (2, 3, 5, 100).env (2, 4000, 1000, 100, 500, -20).mod (env2, "osc1UnisonDetune", 60).mod (env2, "osc1Fine", 100)
            .mod (env2, "filter1Cutoff", 30).filter (1, svfBand, 780, 40).amp (800, 1000, 100, 500, -20).reverb (40, 90, 7.5f, 45).level (-9)
            .macro (1, "Scatter").mod (macro1, "osc1UnisonDetune", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20)
            .delay (eighthD, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Ruined Dive", "Riser").osc (1, sync, 40, 60).unison (1, 2, 12, 100).noise (45).filter (1, ladderBand, 160, 80, 20)
            .env (2, 0, 4000, 0, 4000, 70).mod (env2, "filter1Cutoff", 60).amp (0, 4000, 0, 2000, 40).reverb (35, 70, 7.5f, 70).level (-7)
            .macro (1, "Resonance").mod (macro1, "filter1Resonance", 8).macro (2, "Height").mod (macro2, "filter1Cutoff", 15).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Scarlet Ascent", "Riser").osc (1, analog, sawPos).warp (1, wSync, 5).unison (1, 3, 10, 70).env (2, 5000, 1000, 100, 500, -60)
            .mod (env2, "osc1WarpAmount", 70).mod (env2, "filter1Cutoff", 20).filter (1, svfBand, 270, 30).amp (1500, 1000, 100, 900, 20)
            .reverb (35, 90, 7.5f, 70).level (-18).macro (1, "Sync").mod (macro1, "osc1WarpAmount", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Shredded Blast", "Riser").osc (1, analog, sawPos, 60).unison (1, 4, 12, 100).noise (75, 1).filter (1, comb, 83, 84)
            .filter (2, svfBand, 250).env (2, 4500, 1000, 100, 500, 10).mod (env2, "filter1Cutoff", 40).mod (env2, "filter2Cutoff", 30)
            .amp (1100, 1000, 100, 700, -20).reverb (25, 70, 5.5f, 55).level (-13).macro (1, "Ring").mod (macro1, "filter1Resonance", 7)
            .macro (2, "Tone").mod (macro2, "filter2Cutoff", 20).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Skewed Exhaust", "Riser").osc (1, analog, sawPos, 55).unison (1, 4, 18, 100).noise (55).filter (1, comb, 170, 80)
            .filter (2, ladder24, 1100, 20).env (2, 5500, 1000, 100, 500, 0).mod (env2, "filter1Cutoff", 30).mod (env2, "filter2Cutoff", 15)
            .amp (1400, 1000, 100, 800, -30).reverb (25, 70, 8.5f, 40).level (-8).macro (1, "Ring").mod (macro1, "filter1Resonance", 7)
            .macro (2, "Tone").mod (macro2, "filter2Cutoff", 20).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Smooth Multitude", "Riser").osc (1, analog, 30).unison (1, 8, 4, 100, 0, 20).pitch (1, 0, -100).osc (2, analog, sawPos, 45)
            .pitch (2, -12).unison (2, 4, 9, 100).env (2, 5500, 1000, 100, 500, -20).mod (env2, "osc1UnisonDetune", 65).mod (env2, "osc1Fine", 70)
            .mod (env2, "filter1Cutoff", 35).filter (1, ladder24, 550, 15).amp (1100, 1000, 100, 500, 20).reverb (35, 70, 4, 40).level (-16)
            .macro (1, "Scatter").mod (macro1, "osc1UnisonDetune", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Smothered Lift", "Riser").osc (1, analog, sawPos).warp (1, wSync, 10).unison (1, 2, 20, 90).env (2, 3500, 1000, 100, 500, -20)
            .mod (env2, "osc1WarpAmount", 80).mod (env2, "filter1Cutoff", 20).filter (1, ladder24, 610, 20).amp (1700, 1000, 100, 600, -30)
            .reverb (35, 70, 4, 60).level (-14).macro (1, "Sync").mod (macro1, "osc1WarpAmount", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).delay (eighthD, 40, 0, 4500)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Solar Collapse", "Riser").osc (1, sync, 66.67f, 60).unison (1, 4, 12, 100).noise (35).filter (1, ladder24, 320, 72, 20)
            .env (2, 0, 6000, 0, 6000, 20).mod (env2, "filter1Cutoff", 35).amp (0, 6000, 0, 3000, 40).delay (quarterT, 35, 14)
            .reverb (35, 100, 8.5f, 60).level (-8).macro (1, "Resonance").mod (macro1, "filter1Resonance", 8).macro (2, "Height")
            .mod (macro2, "filter1Cutoff", 15).macro (3, "Air").mod (macro3, "noiseLevel", -35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Terminal Rattle", "Riser").osc (1, analog, squarePos).unison (1, 1, 10).lfo (1, triangle, 1.5f).mod (lfo1, "ampLevel", 24)
            .env (2, 3500, 1000, 100, 500, -30).mod (env2, "lfo1Rate", 40).mod (env2, "filter1Cutoff", 35).filter (1, svfBand, 410, 30)
            .amp (1700, 1000, 100, 400, -30).reverb (25, 70, 9, 65).level (-26).macro (1, "Depth").mod (macro1, "ampLevel", 12).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Tundra Drain", "Riser").osc (1, sync, 30, 45).unison (1, 2, 22, 100).noise (55).filter (1, ladderBand, 340, 75)
            .env (2, 0, 4000, 0, 4000, 40).mod (env2, "filter1Cutoff", 60).amp (0, 4000, 0, 1500, 30).delay (eighth, 25, 12, 4000, false)
            .reverb (30, 90, 7.5f, 35).level (-1).macro (1, "Resonance").mod (macro1, "filter1Resonance", 8).macro (2, "Height")
            .mod (macro2, "filter1Cutoff", 15).macro (3, "Air").mod (macro3, "noiseLevel", -35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Violet Drain", "Riser").osc (1, sync, 40, 40).unison (1, 4, 16, 100).noise (40, 1).filter (1, ladder24, 290, 75, 30)
            .env (2, 0, 3000, 0, 3000, 70).mod (env2, "filter1Cutoff", 35).amp (0, 3000, 0, 3000, 30).delay (quarterT, 25, 22, 2800)
            .reverb (40, 70, 7.5f, 60).level (-2).macro (1, "Resonance").mod (macro1, "filter1Resonance", 8).macro (2, "Height")
            .mod (macro2, "filter1Cutoff", 15).macro (3, "Air").mod (macro3, "noiseLevel", -35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Wrecked Hoist", "Riser").osc (1, analog, sawPos).warp (1, wSync, 10).unison (1, 3, 8).env (2, 3500, 1000, 100, 500, -40)
            .mod (env2, "osc1WarpAmount", 60).mod (env2, "filter1Cutoff", 25).filter (1, ladder24, 410, 30).amp (1400, 1000, 100, 700, 30)
            .delay (sixteenthD, 30, 24, 5200).reverb (40, 100, 8.5f, 40).level (-16).macro (1, "Sync").mod (macro1, "osc1WarpAmount", 30)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);

        // Impact, by name.
        add ("Adrift Avalanche", "Impact").set ("osc1On", 0).noise (75, 0, -40).subOsc (1).filter (1, ladder24, 510, 25, 60).lfo (1, drift, 4.3f)
            .mod (lfo1, "filter1Cutoff", 18).env (2, 0, 1400, 0, 500, 70).mod (env2, "filter1Cutoff", 25).amp (0, 4000, 0, 2000, 65)
            .reverb (35, 100, 4.5f, 40).level (4).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Roll")
            .mod (macro2, "lfo1Rate", 25).drive (hard, 0, 3000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Austere Aftershock", "Impact").set ("osc1On", 0).noise (95, 0, -80).subOsc (45).filter (1, svfLow, 240, 35, 30)
            .lfo (1, randomShape, 8.6f).mod (lfo1, "filter1Cutoff", 6).env (2, 0, 300, 0, 500, 70).mod (env2, "filter1Cutoff", 10)
            .amp (30, 6000, 0, 3500, 55).drive (tube, 40, 1700).reverb (30, 85, 6.5f, 55).level (-13).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Roll").mod (macro2, "lfo1Rate", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50)
            .macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Banished Tremor", "Impact").set ("osc1On", 0).noise (70, 1, -20).subOsc (45).filter (1, ladder12, 890, 15, 20).lfo (1, drift, 5)
            .mod (lfo1, "filter1Cutoff", 8).env (2, 0, 900, 0, 500, 70).mod (env2, "filter1Cutoff", 30).amp (5, 5000, 0, 3000, 60)
            .drive (hard, 25, 4000).reverb (30, 90, 6, 65).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Roll")
            .mod (macro2, "lfo1Rate", 25).macro (3, "Sub").mod (macro3, "subLevel", 40).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Black Trapdoor", "Impact").osc (1, metal, 55).noise (70, 2).filter (1, comb, 140, 60).filter (2, svfBand, 2600, 30)
            .env (2, 0, 360, 0, 300, 85).mod (env2, "filter2Cutoff", 20).mod (env2, "osc1Position", 20).amp (0, 800, 0, 900, 70)
            .drive (hard, 40, 3100).reverb (55, 95, 8, 50).level (-9).macro (1, "Ring").mod (macro1, "filter1Resonance", 25).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Clang")
            .mod (macro4, "osc1Position", 30);
        add ("Bolted Slam", "Impact").osc (1, metal, 35).noise (50).osc (2, analog, sinePos, 60).pitch (2, 0).filter (1, comb, 240, 55)
            .filter (2, svfLow, 1800, 30).env (2, 0, 300, 0, 300, 85).mod (env2, "filter2Cutoff", 15).mod (env2, "osc1Position", 15)
            .amp (0, 600, 0, 2500, 75).reverb (50, 90, 7, 65).level (-7).macro (1, "Ring").mod (macro1, "filter1Resonance", 25).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 22).macro (3, "Clang").mod (macro3, "osc1Position", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Bottomless Sentence", "Impact").osc (1, analog, sawPos).unison (1, 3, 18, 100).osc (2, sync, 60).pitch (2, 7, 7).subOsc (45)
            .noise (1, 1).filter (1, ladder24, 390, 35, 20).env (2, 0, 450, 0, 400, 70).mod (env2, "filter1Cutoff", 35).mod (env2, "noiseLevel", 35)
            .amp (0, 1600, 0, 1500, 70).drive (tube, 25, 6900).reverb (45, 80, 8.5f, 45).level (-18).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Buried Judgment", "Impact").osc (1, analog, sawPos).unison (1, 6, 20, 70).osc (2, analog, 50).pitch (2, 12, 10).subOsc (55)
            .noise (1, 1).filter (1, ladder24, 310, 10, 60).env (2, 0, 600, 0, 400, 65).mod (env2, "filter1Cutoff", 25).mod (env2, "noiseLevel", 40)
            .amp (0, 2700, 0, 1500, 70).drive (hard, 35, 5400).reverb (40, 90, 3, 50).level (-21).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 40).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Caged Sinkhole", "Impact").osc (1, sub, 10).subOsc (30).noise (1, 0, 20).env (2, 0, 500, 0, 400, 60).mod (env2, "osc1Pitch", 64.583f)
            .env (3, 0, 220, 0, 200, 80).mod (env3, "noiseLevel", 55).filter (1, ladder24, 740, 25, 10).amp (0, 3500, 0, 3000, 65)
            .drive (hard, 40, 4400).reverb (30, 95, 3, 70).level (-22).macro (1, "Fall").mod (macro1, "osc1Pitch", 25).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Debris").mod (macro3, "noiseLevel", 30).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Cobalt Breakage", "Impact").osc (1, metal, 65).pitch (1, 7).unison (1, 2, 15, 100).noise (40, 2, 80).filter (1, svfBand, 5600, 20)
            .env (2, 0, 160, 0, 200, 85).mod (env2, "osc1Position", 30).mod (env2, "noiseLevel", 10).amp (0, 650, 0, 900, 60)
            .delay (sixteenth, 45, 32, 8300).reverb (35, 90, 5, 50).level (-4).macro (1, "Shards").mod (macro1, "fxDelayMix", 35)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Glass").mod (macro3, "osc1Position", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Digital Subsidence", "Impact").osc (1, analog, 10).subOsc (60).noise (1, 3, -40).env (2, 0, 450, 0, 400, 85)
            .mod (env2, "osc1Pitch", 75).env (3, 0, 140, 0, 200, 80).mod (env3, "noiseLevel", 55).filter (1, ladder24, 2500, 25, 30)
            .amp (0, 3500, 0, 2500, 70).drive (hard, 35, 2300).reverb (30, 75, 7.5f, 45).level (-24).macro (1, "Fall").mod (macro1, "osc1Pitch", 25)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Dockside Rumble", "Impact").set ("osc1On", 0).noise (90, 1, -60).subOsc (45).filter (1, ladder24, 450, 35, 60)
            .lfo (1, randomShape, 6.4f).mod (lfo1, "filter1Cutoff", 18).env (2, 0, 800, 0, 500, 70).mod (env2, "filter1Cutoff", 25)
            .amp (0, 4000, 0, 4000, 45).reverb (40, 90, 6.5f, 70).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Roll")
            .mod (macro2, "lfo1Rate", 25).drive (hard, 0, 3000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub")
            .mod (macro4, "subLevel", 40);
        add ("Drifting Avalanche", "Impact").set ("osc1On", 0).noise (80, 0, -40).subOsc (45).filter (1, ladder12, 530, 35, 60)
            .lfo (1, randomShape, 7.6f).mod (lfo1, "filter1Cutoff", 18).env (2, 0, 1100, 0, 500, 70).mod (env2, "filter1Cutoff", 30)
            .amp (30, 6000, 0, 4000, 55).drive (tube, 25, 2800).reverb (35, 85, 4.5f, 45).level (-12).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Roll").mod (macro2, "lfo1Rate", 25).macro (3, "Sub").mod (macro3, "subLevel", 40)
            .macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Drifting Breakage", "Impact").osc (1, metal, 80).pitch (1, 24).unison (1, 3, 20, 100).noise (65, 2, 20).filter (1, svfBand, 2800, 20)
            .env (2, 0, 160, 0, 200, 85).mod (env2, "osc1Position", 25).mod (env2, "noiseLevel", 15).amp (0, 650, 0, 900, 60)
            .delay (sixteenthT, 65, 23, 8100).reverb (45, 100, 6, 70).level (-4).macro (1, "Shards").mod (macro1, "fxDelayMix", 35)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Drifting Bulkhead", "Impact").osc (1, metal, 45).noise (65).filter (1, comb, 120, 65).filter (2, svfBand, 1700, 25)
            .env (2, 0, 220, 0, 300, 85).mod (env2, "filter2Cutoff", 20).mod (env2, "osc1Position", 20).amp (0, 600, 0, 2000, 75)
            .drive (hard, 30, 4600).reverb (35, 100, 4.5f, 60).level (-8).macro (1, "Ring").mod (macro1, "filter1Resonance", 25).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 45);
        add ("Encoded Fracture", "Impact").osc (1, metal, 85).pitch (1, 12).unison (1, 3, 15, 100).noise (45, 3, 40).filter (1, svfHigh, 1800, 15)
            .env (2, 0, 260, 0, 200, 85).mod (env2, "osc1Position", 15).mod (env2, "noiseLevel", 25).amp (0, 300, 0, 1000, 60)
            .delay (eighthT, 45, 28, 6900).reverb (30, 70, 4, 65).level (0).macro (1, "Shards").mod (macro1, "fxDelayMix", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).macro (3, "Glass").mod (macro3, "osc1Position", 30).drive (crush, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Entombed Sentence", "Impact").osc (1, analog, sawPos).unison (1, 4, 14).osc (2, analog, 40).pitch (2, 12, 10).subOsc (60).noise (1, 1)
            .filter (1, ladder24, 220, 25, 30).env (2, 0, 250, 0, 400, 85).mod (env2, "filter1Cutoff", 45).mod (env2, "noiseLevel", 35)
            .amp (0, 1500, 0, 3000).drive (tube, 25, 3500).reverb (45, 80, 3, 45).level (-18).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 40).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Far Sinkhole", "Impact").osc (1, analog, 10).subOsc (30).noise (1, 1).env (2, 0, 750, 0, 400, 80).mod (env2, "osc1Pitch", 25)
            .env (3, 0, 260, 0, 200, 80).mod (env3, "noiseLevel", 50).filter (1, ladder24, 2800, 5).amp (0, 3200, 0, 2000, 70)
            .drive (fuzz, 30, 2200).reverb (45, 95, 6.5f, 55).level (-28).macro (1, "Fall").mod (macro1, "osc1Pitch", 25).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Debris").mod (macro3, "noiseLevel", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Fenced Wreckage", "Impact").osc (1, metal, 95).pitch (1, 24).unison (1, 3, 15, 100).noise (65, 2, 60).filter (1, svfHigh, 2900, 30)
            .env (2, 0, 120, 0, 200, 85).mod (env2, "osc1Position", 10).mod (env2, "noiseLevel", 10).amp (0, 700, 0, 400, 70)
            .delay (sixteenth, 60, 24, 11000).reverb (45, 80, 4, 55).level (1).macro (1, "Shards").mod (macro1, "fxDelayMix", 35)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).drive (crush, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Furious Hatch", "Impact").osc (1, metal, 85).noise (45, 1).filter (1, comb, 60, 75).filter (2, ladder24, 3500, 15)
            .env (2, 0, 220, 0, 300, 85).mod (env2, "filter2Cutoff", 25).mod (env2, "osc1Position", 30).amp (0, 1200, 0, 1400, 75)
            .reverb (45, 75, 3, 65).level (-1).macro (1, "Ring").mod (macro1, "filter1Resonance", 25).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 45);
        add ("Glitched Drop", "Impact").osc (1, sub, 10).subOsc (30).noise (1, 3, -20).env (2, 0, 350, 0, 400, 80).mod (env2, "osc1Pitch", 50)
            .env (3, 0, 200, 0, 200, 80).mod (env3, "noiseLevel", 45).filter (1, ladder24, 1600, 5, 20).amp (0, 2300, 0, 2500, 55)
            .drive (hard, 20, 4200).reverb (50, 75, 7.5f, 30).level (-17).macro (1, "Fall").mod (macro1, "osc1Pitch", 25).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Guarded Smash", "Impact").osc (1, metal, 90).pitch (1, 12).unison (1, 2, 25, 100).noise (60, 0, 20).filter (1, svfBand, 3000, 35)
            .env (2, 0, 60, 0, 200, 85).mod (env2, "osc1Position", 20).mod (env2, "noiseLevel", 10).amp (0, 500, 0, 700, 80)
            .delay (thirtySecond, 40, 26, 4400).reverb (30, 80, 9, 30).level (-1).macro (1, "Shards").mod (macro1, "fxDelayMix", 35)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Hollowed Burst", "Impact").osc (1, metal, 85).pitch (1, 19).unison (1, 2, 35, 100).noise (55, 2, 20).filter (1, ladderHigh, 3700, 20)
            .env (2, 0, 160, 0, 200, 85).mod (env2, "osc1Position", 10).mod (env2, "noiseLevel", 25).amp (0, 600, 0, 700, 80)
            .delay (sixteenthT, 50, 33, 5100).reverb (40, 95, 8, 35).level (4).macro (1, "Shards").mod (macro1, "fxDelayMix", 35)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Detune").mod (macro3, "osc1UnisonDetune", 25).macro (4, "Glass")
            .mod (macro4, "osc1Position", 30);
        add ("Hostile Edict", "Impact").osc (1, analog, sawPos).unison (1, 4, 26, 100).osc (2, growl, 30).pitch (2, 7, 8).subOsc (40).noise (1, 1)
            .filter (1, ladder24, 320, 30, 30).env (2, 0, 400, 0, 400, 75).mod (env2, "filter1Cutoff", 40).mod (env2, "noiseLevel", 35)
            .amp (0, 1800, 0, 1500, 60).drive (hard, 40, 3300).reverb (55, 95, 3.5f, 65).level (-23).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Sub")
            .mod (macro3, "subLevel", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Lingering Clang", "Impact").osc (1, metal, 65).noise (55, 2).osc (2, sub, 0, 65).pitch (2, 0).filter (1, comb, 100, 65)
            .filter (2, svfBand, 2300, 30).env (2, 0, 360, 0, 300, 85).mod (env2, "filter2Cutoff", 25).mod (env2, "osc1Position", 30)
            .amp (0, 1800, 0, 2000, 65).drive (hard, 35, 3100).reverb (50, 80, 3, 65).level (-7).macro (1, "Ring")
            .mod (macro1, "filter1Resonance", 25).macro (2, "Tone").mod (macro2, "filter2Cutoff", 22).macro (3, "Clang")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Lost Sinkhole", "Impact").osc (1, analog, 10).subOsc (50).noise (1, 3, 20).env (2, 0, 800, 0, 400, 85)
            .mod (env2, "osc1Pitch", 64.583f).env (3, 0, 200, 0, 200, 80).mod (env3, "noiseLevel", 60).filter (1, ladder24, 1600, 5)
            .amp (0, 1300, 0, 2000).drive (hard, 20, 1700).reverb (35, 70, 4, 35).level (-17).macro (1, "Fall").mod (macro1, "osc1Pitch", 25)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Sub")
            .mod (macro4, "subLevel", 35);
        add ("Pewter Wreckage", "Impact").osc (1, metal, 85).pitch (1, 7).unison (1, 4, 30, 100).noise (45, 3, 40).filter (1, svfBand, 2400, 20)
            .env (2, 0, 160, 0, 200, 85).mod (env2, "osc1Position", 15).mod (env2, "noiseLevel", 10).amp (0, 600, 0, 800, 75)
            .delay (eighthT, 55, 30, 4100).reverb (35, 80, 3.5f, 35).level (1).macro (1, "Shards").mod (macro1, "fxDelayMix", 35)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).drive (crush, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Raw Plummet", "Impact").osc (1, sub, 0).subOsc (50).noise (1, 0, 20).env (2, 0, 450, 0, 400, 65).mod (env2, "osc1Pitch", 75)
            .env (3, 0, 300, 0, 200, 80).mod (env3, "noiseLevel", 65).filter (1, ladder24, 2800, 5, 10).amp (0, 2200, 0, 3000, 60)
            .reverb (35, 70, 3, 60).level (-14).macro (1, "Fall").mod (macro1, "osc1Pitch", 25).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).drive (hard, 0, 4000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Remote Rockfall", "Impact").set ("osc1On", 0).noise (70, 1, -40).subOsc (1).filter (1, svfLow, 880, 35, 20).lfo (1, drift, 4.2f)
            .mod (lfo1, "filter1Cutoff", 16).env (2, 0, 1300, 0, 500, 70).mod (env2, "filter1Cutoff", 25).amp (0, 6000, 0, 4000, 55)
            .reverb (35, 70, 6, 35).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Roll").mod (macro2, "lfo1Rate", 25)
            .macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Rusted Plummet", "Impact").osc (1, sub, 0).subOsc (30).noise (1, 1, -20).env (2, 0, 900, 0, 400, 70).mod (env2, "osc1Pitch", 64.583f)
            .env (3, 0, 220, 0, 200, 80).mod (env3, "noiseLevel", 70).filter (1, ladder24, 1300, 15).amp (0, 1900, 0, 3000, 60)
            .drive (hard, 30, 2500).reverb (50, 70, 5.5f, 40).level (-20).macro (1, "Fall").mod (macro1, "osc1Pitch", 25).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Debris").mod (macro3, "noiseLevel", 30).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Savage Edict", "Impact").osc (1, analog, sawPos).unison (1, 3, 16, 70).osc (2, sync, 50).pitch (2, 12, 4).subOsc (55).noise (1, 1)
            .filter (1, ladder24, 330, 30, 30).env (2, 0, 600, 0, 400, 85).mod (env2, "filter1Cutoff", 45).mod (env2, "noiseLevel", 30)
            .amp (0, 2200, 0, 2500).drive (fuzz, 40, 5000).reverb (40, 80, 3, 50).level (-32).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 40).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Severe Burst", "Impact").osc (1, metal, 95).pitch (1, 7).unison (1, 2, 40, 100).noise (60, 2, 40).filter (1, ladderHigh, 3700, 30)
            .env (2, 0, 100, 0, 200, 85).mod (env2, "osc1Position", 10).mod (env2, "noiseLevel", 20).amp (0, 1150, 0, 600, 75)
            .delay (eighthT, 55, 30, 8100).reverb (30, 90, 5, 35).level (-4).macro (1, "Shards").mod (macro1, "fxDelayMix", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).macro (3, "Glass").mod (macro3, "osc1Position", 30).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Sooty Sentence", "Impact").osc (1, analog, sawPos).unison (1, 3, 26, 90).osc (2, sync, 40).pitch (2, 12, 9).subOsc (60).noise (1, 1)
            .filter (1, ladder24, 460, 20, 30).env (2, 0, 200, 0, 400, 85).mod (env2, "filter1Cutoff", 45).mod (env2, "noiseLevel", 25)
            .amp (0, 1300, 0, 2000, 55).drive (hard, 40, 3000).reverb (40, 90, 6, 70).level (-23).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Sub")
            .mod (macro3, "subLevel", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Spectral Shockwave", "Impact").set ("osc1On", 0).noise (80, 2, -80).subOsc (30).filter (1, ladder24, 300, 25, 50)
            .lfo (1, randomShape, 6.4f).mod (lfo1, "filter1Cutoff", 6).env (2, 0, 500, 0, 500, 70).mod (env2, "filter1Cutoff", 30)
            .amp (30, 3000, 0, 3500, 60).drive (tube, 20, 2300).reverb (45, 80, 8.5f, 40).level (-8).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Roll").mod (macro2, "lfo1Rate", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Sterile Sentence", "Impact").osc (1, analog, sawPos).unison (1, 3, 26, 100).osc (2, growl, 70).pitch (2, 7, 10).subOsc (50)
            .noise (1, 1).filter (1, ladder24, 460, 30, 60).env (2, 0, 600, 0, 400, 80).mod (env2, "filter1Cutoff", 35).mod (env2, "noiseLevel", 20)
            .amp (0, 2900, 0, 2500, 55).drive (tube, 25, 3500).reverb (50, 85, 6, 50).level (-17).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 40).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Sublevel Avalanche", "Impact").set ("osc1On", 0).noise (95, 2, -20).subOsc (1).filter (1, ladder12, 300, 20, 60)
            .lfo (1, randomShape, 3.1f).mod (lfo1, "filter1Cutoff", 16).env (2, 0, 800, 0, 500, 70).mod (env2, "filter1Cutoff", 25)
            .amp (30, 5000, 0, 3500, 60).reverb (35, 95, 8.5f, 40).level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25)
            .macro (2, "Roll").mod (macro2, "lfo1Rate", 25).drive (hard, 0, 3000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50)
            .macro (4, "Sub").mod (macro4, "subLevel", 40);
        add ("Synthetic Landslide", "Impact").osc (1, sub, 10).subOsc (40).noise (1, 3).env (2, 0, 450, 0, 400, 60).mod (env2, "osc1Pitch", 64.583f)
            .env (3, 0, 280, 0, 200, 80).mod (env3, "noiseLevel", 65).filter (1, ladder24, 3000, 10, 40).amp (0, 3400, 0, 2500, 45)
            .drive (fuzz, 40, 2000).reverb (30, 75, 7.5f, 55).level (-32).macro (1, "Fall").mod (macro1, "osc1Pitch", 25).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Debris").mod (macro3, "noiseLevel", 30).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Tagged Shutter", "Impact").osc (1, metal, 95).noise (40, 1).osc (2, analog, sinePos, 55).pitch (2, 0).filter (1, comb, 130, 85)
            .filter (2, ladder24, 4000, 20).env (2, 0, 220, 0, 300, 85).mod (env2, "filter2Cutoff", 10).mod (env2, "osc1Position", 10)
            .amp (0, 1300, 0, 1100, 70).drive (tube, 20, 4600).reverb (35, 100, 3, 65).level (-10).macro (1, "Ring")
            .mod (macro1, "filter1Resonance", 25).macro (2, "Tone").mod (macro2, "filter2Cutoff", 22).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 45).macro (4, "Clang").mod (macro4, "osc1Position", 30);
        add ("Trailing Judgment", "Impact").osc (1, analog, sawPos).unison (1, 4, 12, 100).osc (2, sync, 40).pitch (2, -12, 4).subOsc (55)
            .noise (1, 1).filter (1, ladder24, 390, 15, 50).env (2, 0, 350, 0, 400, 85).mod (env2, "filter1Cutoff", 40).mod (env2, "noiseLevel", 30)
            .amp (0, 1900, 0, 3000, 70).drive (tube, 50, 4600).reverb (55, 80, 6, 40).level (-23).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 40).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Violent Lid", "Impact").osc (1, metal, 45).noise (50, 3).filter (1, comb, 140, 65).filter (2, svfLow, 1900, 20)
            .env (2, 0, 340, 0, 300, 85).mod (env2, "filter2Cutoff", 15).mod (env2, "osc1Position", 20).amp (0, 1900, 0, 1700, 65)
            .drive (hard, 25, 5600).reverb (55, 90, 8.5f, 55).level (-9).macro (1, "Ring").mod (macro1, "filter1Resonance", 25).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 22).macro (3, "Clang").mod (macro3, "osc1Position", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Wordless Clang", "Impact").osc (1, metal, 80).noise (45, 1).osc (2, analog, sinePos, 55).pitch (2, 0).filter (1, comb, 220, 80)
            .filter (2, svfBand, 3800, 30).env (2, 0, 300, 0, 300, 85).mod (env2, "filter2Cutoff", 25).mod (env2, "osc1Position", 10)
            .amp (0, 1600, 0, 1100, 65).drive (tube, 35, 3200).reverb (55, 70, 8, 40).level (-2).macro (1, "Ring")
            .mod (macro1, "filter1Resonance", 25).macro (2, "Tone").mod (macro2, "filter2Cutoff", 22).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 45);

        // Cinematic, by name.
        add ("Abyssal Lifesign", "Cinematic").osc (1, sub, 20).subOsc (50).lfoSync (1, steps, bars2, 1, 99)
            .stepPattern (1, { 0, 0, -100, -100, -30, -20, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100 }).mod (lfo1, "ampLevel", 72)
            .mod (lfo1, "filter1Cutoff", 14).filter (1, ladder24, 150, 5, 20).amp (2, 300, 100, 450).drive (hard, 40, 1600)
            .reverb (20, 75, 8.5f, 55).level (-20).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Beat")
            .mod (macro2, "ampLevel", 14).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Barred Touchdown", "Cinematic").osc (1, sub, 15).subOsc (40).osc (2, analog, sawPos, 70).unison (2, 3, 24, 100).pitch (2, 12)
            .noise (1, 1, -20).env (2, 0, 240, 0, 100, 88).mod (env2, "osc1Pitch", 25).mod (env2, "noiseLevel", 55).env (3, 0, 400, 0, 500, 70)
            .mod (env3, "filter1Cutoff", 20).filter (1, ladder24, 180, 10, 50).amp (0, 2000, 0, 3500, 65).drive (tube, 45, 2600)
            .reverb (55, 95, 5, 55).level (-21).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Drum")
            .mod (macro2, "osc1Pitch", 14.583f).macro (3, "Grit").mod (macro3, "fxDriveAmount", 40).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Blistered Leviathan", "Cinematic").osc (1, analog, sawPos).unison (1, 5, 10, 60).osc (2, sync, 40).pitch (2, 0, 7).unison (2, 2, 8)
            .subOsc (35).filter (1, ladder24, 290, 35, 30).env (2, 100, 2100, 70, 1500).mod (env2, "filter1Cutoff", 30)
            .mod (env2, "osc2Position", 15).amp (100, 600, 90, 2400).drive (fuzz, 55, 2900).reverb (35, 70, 4.5f, 60).level (-36)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Growl").mod (macro2, "osc2Position", 30).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Blistered Stopwatch", "Cinematic").osc (1, pulse, 80).pitch (1, 24).noise (80).filter (1, ladderBand, 4600, 65)
            .lfoSync (1, steps, bar1, 1, 50)
            .stepPattern (1, { 0, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100 })
            .mod (lfo1, "ampLevel", 75).amp (0, 300, 100, 250).reverb (20, 100, 5.5f, 40).level (-9).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance").mod (macro2, "filter1Resonance", 20).drive (hard, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Bloodless Leviathan", "Cinematic").osc (1, analog, sawPos).unison (1, 4, 10, 100).osc (2, growl, 70).pitch (2, 12, 6)
            .unison (2, 2, 10).subOsc (40).filter (1, ladder24, 180, 20, 20).env (2, 120, 1700, 70, 1500, 40).mod (env2, "filter1Cutoff", 25)
            .mod (env2, "osc2Position", 15).amp (70, 600, 85, 2900).drive (fuzz, 55, 6000).reverb (30, 95, 7.5f, 40).level (-34).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Growl").mod (macro2, "osc2Position", 30).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).macro (4, "Tear").mod (macro4, "fxDriveAmount", 35);
        add ("Boarded Tide", "Cinematic").osc (1, analog, sawPos).unison (1, 5, 22, 100).osc (2, additive, 50, 55).pitch (2, 7, 5)
            .unison (2, 2, 14, 100).osc (3, analog, sawPos, 65).pitch (3, 12, 7).subOsc (30).env (2, 6000, 1000, 100, 1500, 0)
            .mod (env2, "filter1Cutoff", 25).mod (env2, "filter1Drive", 30).filter (1, ladder24, 150).amp (1700, 1000, 100, 2500, 20)
            .reverb (50, 95, 3.5f, 65).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Fifth")
            .mod (macro2, "osc2Volume", -50).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Bone Twist", "Cinematic").osc (1, analog, sawPos).unison (1, 4, 12, 100).osc (2, analog, sawPos, 70).pitch (2, 1)
            .osc (3, analog, sawPos, 75).pitch (3, 11).filter (1, svfBand, 2800, 20).env (2, 0, 150, 0, 400, 70).mod (env2, "filter1Cutoff", 24)
            .amp (15, 1600, 0, 1300).drive (hard, 45, 3800).delay (quarter, 55, 34, 3200).reverb (40, 100, 7, 40).level (-20).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Border Braam", "Cinematic").osc (1, analog, sawPos).unison (1, 5, 20, 70).osc (2, analog, 30).pitch (2, 0, 7).unison (2, 2, 10)
            .subOsc (50).filter (1, ladder24, 160, 15, 50).env (2, 220, 1100, 70, 1500, 40).mod (env2, "filter1Cutoff", 27)
            .mod (env2, "osc2Position", 30).amp (140, 600, 85, 1700).drive (tube, 55, 5500).reverb (45, 100, 6, 60).level (-27).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Growl").mod (macro2, "osc2Position", 30).macro (3, "Tear")
            .mod (macro3, "fxDriveAmount", 35).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Border Tick", "Cinematic").osc (1, pulse, 90, 50).pitch (1, 12).noise (80, 0, 40).filter (1, svfBand, 3100, 40)
            .lfoSync (1, steps, half, 1, 60)
            .stepPattern (1, { 0, -100, -100, -100, -100, -100, -100, -100, 0, -100, -100, -100, -100, -100, -100, -100 })
            .mod (lfo1, "ampLevel", 79).amp (0, 300, 100, 220).delay (sixteenthD, 50, 28, 4300).reverb (25, 100, 7.5f, 60).level (-4)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance").mod (macro2, "filter1Resonance", 20).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Brass Betrayal", "Cinematic").osc (1, metal, 30).unison (1, 4, 10, 100).osc (2, metal, 30, 65).pitch (2, 6).osc (3, metal, 30, 60)
            .pitch (3, 13).filter (1, svfLow, 1900, 35).env (2, 0, 650, 0, 400, 70).mod (env2, "filter1Cutoff", 18).amp (15, 400, 0, 1900, 55)
            .delay (half, 45, 28, 2800).reverb (45, 95, 6, 35).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Detune")
            .mod (macro2, "osc1UnisonDetune", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 30).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 35);
        add ("Bunker Delirium", "Cinematic").osc (1, analog, sawPos).unison (1, 4, 20, 100, 0, 70).pitch (1, 12).osc (2, analog, sawPos)
            .pitch (2, 11).unison (2, 4, 20, 100, 0, 70).env (2, 8500, 1000, 100, 1000, 20).mod (env2, "osc1UnisonDetune", 45)
            .mod (env2, "filter1Cutoff", 16).lfo (1, sine, 8, 0).mod (lfo1, "ampLevel", 11).filter (1, svfLow, 1800, 5).filter (2, svfHigh, 670)
            .amp (1600, 1000, 100, 1700).reverb (55, 90, 5, 55).level (-17).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20)
            .macro (2, "Spread").mod (macro2, "osc1UnisonDetune", 20).macro (3, "Semitone").mod (macro3, "osc2Volume", -55).macro (4, "Shake")
            .mod (macro4, "ampLevel", 8);
        add ("Cavernous Landfall", "Cinematic").osc (1, sub, 15).subOsc (60).osc (2, analog, sawPos, 75).unison (2, 5, 16, 100).pitch (2, 0)
            .noise (1, 1, -60).env (2, 0, 210, 0, 100, 88).mod (env2, "osc1Pitch", 25).mod (env2, "noiseLevel", 30).env (3, 0, 1000, 0, 500, 70)
            .mod (env3, "filter1Cutoff", 35).filter (1, ladder24, 380, 10, 60).amp (0, 2500, 0, 2000, 65).drive (hard, 45, 4100)
            .reverb (50, 85, 3, 60).level (-25).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Drum")
            .mod (macro2, "osc1Pitch", 14.583f).macro (3, "Grit").mod (macro3, "fxDriveAmount", 40).macro (4, "Chord")
            .mod (macro4, "osc2Volume", -50);
        add ("Clipped Upwelling", "Cinematic").osc (1, analog, sawPos).unison (1, 4, 16, 100).osc (2, additive, 70, 70).pitch (2, 7, 4)
            .unison (2, 2, 14, 100).osc (3, analog, squarePos, 65).pitch (3, 12, 3).subOsc (50).env (2, 3500, 1000, 100, 1500, -20)
            .mod (env2, "filter1Cutoff", 30).mod (env2, "filter1Drive", 40).filter (1, ladder24, 260, 15).amp (1300, 1000, 100, 1500, 30)
            .reverb (45, 100, 4, 55).level (-19).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Fifth")
            .mod (macro2, "osc2Volume", -50).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Copper Discovery", "Cinematic").osc (1, metal, 60).unison (1, 3, 22, 100).osc (2, metal, 60, 75).pitch (2, 6).osc (3, metal, 60, 70)
            .pitch (3, 13).filter (1, svfBand, 4100, 35).env (2, 0, 700, 0, 400, 70).mod (env2, "filter1Cutoff", 12).amp (0, 1600, 0, 800, 60)
            .drive (tube, 45, 3200).delay (half, 55, 20, 2600).reverb (55, 100, 8, 55).level (-9).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Derelict Juggernaut", "Cinematic").osc (1, analog, sawPos).unison (1, 5, 20, 60).osc (2, analog, 40).pitch (2, 0, 5).unison (2, 2, 10)
            .subOsc (40).filter (1, ladder24, 480, 30, 50).env (2, 180, 1800, 50, 1500, 60).mod (env2, "filter1Cutoff", 37)
            .mod (env2, "osc2Position", 10).amp (70, 600, 85, 2800).drive (hard, 40, 3600).reverb (30, 85, 3, 65).level (-23).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Growl").mod (macro2, "osc2Position", 30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Dim Collision", "Cinematic").osc (1, sub, 15).subOsc (40).osc (2, analog, sawPos, 70).unison (2, 3, 26, 100).pitch (2, 7).noise (1, 1)
            .env (2, 0, 250, 0, 100, 88).mod (env2, "osc1Pitch", 25).mod (env2, "noiseLevel", 35).env (3, 0, 600, 0, 500, 70)
            .mod (env3, "filter1Cutoff", 20).filter (1, ladder24, 330, 25, 20).amp (0, 4500, 0, 2500, 65).drive (fuzz, 35, 4400)
            .reverb (40, 75, 5, 45).level (-32).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Drum")
            .mod (macro2, "osc1Pitch", 14.583f).macro (3, "Chord").mod (macro3, "osc2Volume", -50).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Dockside Leviathan", "Cinematic").osc (1, analog, sawPos).unison (1, 4, 20, 90).osc (2, sync, 70).pitch (2, 12, 10).unison (2, 2, 14)
            .subOsc (35).filter (1, ladder24, 420, 35, 50).env (2, 140, 1000, 70, 1500).mod (env2, "filter1Cutoff", 37)
            .mod (env2, "osc2Position", 25).amp (110, 600, 100, 2200).drive (hard, 55, 6900).reverb (50, 90, 3, 40).level (-27).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Growl").mod (macro2, "osc2Position", 30).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Doomed Ambush", "Cinematic").osc (1, fm, 70).unison (1, 3, 22, 100).osc (2, fm, 70, 65).pitch (2, 1).osc (3, fm, 70, 65).pitch (3, 7)
            .filter (1, ladder12, 3000, 35).env (2, 0, 750, 0, 400, 70).mod (env2, "filter1Cutoff", 14).amp (15, 1000, 0, 1200, 55)
            .drive (hard, 40, 11000).delay (quarterD, 65, 32, 2100).reverb (50, 90, 10, 50).level (-29).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Faceless Warhorn", "Cinematic").osc (1, analog, 53).unison (1, 3, 14, 50).route (1, 30).osc (2, pulse, 30, 65).pitch (2, 0, 4)
            .route (2, 50).parallel().subOsc (25).filter (1, ladder24, 360, 25).filter (2, svfBand, 560, 55).env (2, 360, 600, 70, 800)
            .mod (env2, "filter1Cutoff", 21).mod (velocity, "filter1Cutoff", 11).amp (360, 600, 90, 1300).reverb (45, 70, 8.5f, 65).level (-15)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Bell").mod (macro2, "filter2Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Factory Summons", "Cinematic").osc (1, analog, 55).unison (1, 3, 8, 90).route (1, 30).osc (2, pulse, 35, 65).pitch (2, 0, 3)
            .route (2, 40).parallel().subOsc (1).filter (1, ladder24, 340, 5, 30).filter (2, svfBand, 720, 55).env (2, 280, 1400, 70, 800)
            .mod (env2, "filter1Cutoff", 24).mod (velocity, "filter1Cutoff", 8).amp (240, 600, 95, 1300).reverb (50, 100, 3.5f, 30).level (-13)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Bell").mod (macro2, "filter2Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Faint Bloom", "Cinematic").osc (1, analog, sawPos).unison (1, 6, 18, 100).osc (2, analog, 30, 60).pitch (2, 7, 1)
            .unison (2, 2, 10, 100).osc (3, analog, sawPos, 50).pitch (3, -12, 4).subOsc (30).env (2, 4500, 1000, 100, 1500, -30)
            .mod (env2, "filter1Cutoff", 30).mod (env2, "filter1Drive", 20).filter (1, ladder24, 360).amp (1700, 1000, 100, 1500, -20)
            .reverb (35, 90, 8, 65).level (-17).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Fifth")
            .mod (macro2, "osc2Volume", -50).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Floating Colossus", "Cinematic").osc (1, analog, sawPos).unison (1, 4, 24, 90).osc (2, growl, 60).pitch (2, -12, 4).unison (2, 2, 14)
            .subOsc (50).filter (1, ladder24, 230, 25, 40).env (2, 160, 800, 50, 1500, 30).mod (env2, "filter1Cutoff", 36)
            .mod (env2, "osc2Position", 30).amp (20, 600, 90, 1400).drive (hard, 40, 4300).reverb (45, 80, 4.5f, 65).level (-25).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Growl").mod (macro2, "osc2Position", 30).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Fried Deadline", "Cinematic").osc (1, analog, sawPos).unison (1, 4, 16, 70).osc (2, analog, squarePos, 65).pitch (2, 0, 8)
            .lfoSync (1, steps, half, 1, 99).stepPattern (1, { 0, -30, -65, -100, -20, -50, -85, -100, 0, -30, -65, -100, -20, -50, -85, -100 })
            .mod (lfo1, "ampLevel", 75).mod (lfo1, "filter1Cutoff", 16).env (2, 3000, 1000, 100, 500, -30).mod (env2, "filter1Cutoff", 22)
            .filter (1, ladder24, 400, 25).amp (2, 300, 100, 300).drive (hard, 15, 6900).reverb (30, 70, 8, 65).level (-19).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Pulse").mod (macro2, "ampLevel", 14).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Grave Warhorn", "Cinematic").osc (1, analog, 59).unison (1, 4, 8, 90).route (1, 50).osc (2, pulse, 20, 50).pitch (2, 7, 3)
            .route (2, 70).parallel().subOsc (25).filter (1, ladder24, 430, 5).filter (2, svfBand, 780, 60).env (2, 420, 800, 60, 800)
            .mod (env2, "filter1Cutoff", 25).mod (velocity, "filter1Cutoff", 8).amp (240, 600, 100, 1200).reverb (35, 95, 6, 50).level (-10)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Bell").mod (macro2, "filter2Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Nasal").mod (macro4, "filter2Resonance", 25);
        add ("Guarded Swell", "Cinematic").osc (1, analog, sawPos).unison (1, 4, 20, 100).osc (2, analog, 50, 70).pitch (2, 7, 5)
            .unison (2, 2, 8, 100).osc (3, analog, sawPos, 60).pitch (3, -12, 6).subOsc (30).env (2, 3500, 1000, 100, 1500, 0)
            .mod (env2, "filter1Cutoff", 25).mod (env2, "filter1Drive", 50).filter (1, ladder24, 180, 35).amp (1500, 1000, 100, 3500, 30)
            .reverb (40, 75, 4.5f, 40).level (-20).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Fifth")
            .mod (macro2, "osc2Volume", -50).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Sub")
            .mod (macro4, "subLevel", 35);
        add ("Hexed Leviathan", "Cinematic").osc (1, analog, sawPos).unison (1, 4, 14, 60).osc (2, growl, 30).pitch (2, -12, 10).unison (2, 2, 14)
            .subOsc (55).filter (1, ladder24, 340, 30, 40).env (2, 280, 2400, 60, 1500, 60).mod (env2, "filter1Cutoff", 26)
            .mod (env2, "osc2Position", 10).amp (110, 600, 85, 2500).drive (tube, 55, 6900).reverb (40, 100, 3.5f, 30).level (-27)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 25).macro (2, "Growl").mod (macro2, "osc2Position", 30).macro (3, "Sub")
            .mod (macro3, "subLevel", 35).macro (4, "Tear").mod (macro4, "fxDriveAmount", 35);
        add ("Hostile Colossus", "Cinematic").osc (1, analog, sawPos).unison (1, 3, 10, 100).osc (2, analog, 60).pitch (2, 7, 4).unison (2, 2, 10)
            .subOsc (50).filter (1, ladder24, 400, 35, 40).env (2, 260, 900, 70, 1500, 40).mod (env2, "filter1Cutoff", 39)
            .mod (env2, "osc2Position", 20).amp (40, 600, 95, 3000).drive (fuzz, 50, 6300).reverb (30, 100, 3, 35).level (-36).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Growl").mod (macro2, "osc2Position", 30).macro (3, "Sub").mod (macro3, "subLevel", 35)
            .macro (4, "Tear").mod (macro4, "fxDriveAmount", 35);
        add ("Khaki Oliphant", "Cinematic").osc (1, analog, 51).unison (1, 3, 8, 90).route (1, 20).osc (2, pulse, 50, 60).pitch (2, 7, 3)
            .route (2, 40).parallel().subOsc (1).filter (1, ladder24, 590, 20, 30).filter (2, svfBand, 790, 60).env (2, 120, 1200, 70, 800)
            .mod (env2, "filter1Cutoff", 22).mod (velocity, "filter1Cutoff", 11).amp (340, 600, 85, 1100).reverb (50, 80, 6.5f, 65).level (-12)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Bell").mod (macro2, "filter2Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Lightless Lifesign", "Cinematic").osc (1, sub, 0).subOsc (30).lfoSync (1, steps, half, 1, 98)
            .stepPattern (1, { 0, 0, -100, -100, -100, -10, -30, -100, -100, -100, -100, -100, -100, -100, -100, -100 }).mod (lfo1, "ampLevel", 78)
            .mod (lfo1, "filter1Cutoff", 10).filter (1, ladder24, 150, 25, 30).amp (2, 300, 100, 600).reverb (20, 85, 5.5f, 35).level (-6)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Beat").mod (macro2, "ampLevel", 14).drive (tube, 0, 2000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Lingering Ambush", "Cinematic").osc (1, fm, 60).unison (1, 4, 10, 100).osc (2, fm, 60).pitch (2, 6).osc (3, fm, 60, 60).pitch (3, 13)
            .filter (1, svfBand, 4000, 25).env (2, 0, 450, 0, 400, 70).mod (env2, "filter1Cutoff", 26).amp (3, 700, 0, 1200, 55)
            .drive (tube, 30, 5900).delay (half, 50, 26, 4000).reverb (60, 90, 12, 55).level (-8).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 30);
        add ("Lingering Fever", "Cinematic").osc (1, analog, sawPos).unison (1, 4, 20, 100, 0, 40).pitch (1, 19).osc (2, analog, sawPos)
            .pitch (2, 20).unison (2, 4, 8, 100, 0, 40).env (2, 9000, 1000, 100, 1000, -10).mod (env2, "osc1UnisonDetune", 35)
            .mod (env2, "filter1Cutoff", 20).lfo (1, triangle, 8.5f, 0).mod (lfo1, "ampLevel", 3).filter (1, svfLow, 3200).filter (2, svfHigh, 310)
            .amp (900, 1000, 100, 2900).reverb (45, 95, 8.5f, 40).level (-20).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20)
            .macro (2, "Spread").mod (macro2, "osc1UnisonDetune", 20).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Semitone").mod (macro4, "osc2Volume", -55);
        add ("Livid Chronometer", "Cinematic").osc (1, pulse, 90, 55).pitch (1, 12).noise (75, 3, 20).filter (1, svfBand, 2500, 75)
            .lfoSync (1, steps, bar1, 1, 80)
            .stepPattern (1, { 0, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100 })
            .mod (lfo1, "ampLevel", 83).amp (0, 300, 100, 230).delay (sixteenthD, 30, 30, 2500).reverb (25, 90, 5.5f, 60).level (1)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance").mod (macro2, "filter1Resonance", 20).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Looping Pulse", "Cinematic").osc (1, analog, sinePos).subOsc (60).lfoSync (1, steps, bars2, 1, 96)
            .stepPattern (1, { 0, 0, -100, -100, -20, -10, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100 }).mod (lfo1, "ampLevel", 83)
            .mod (lfo1, "filter1Cutoff", 12).filter (1, ladder24, 130, 15, 50).amp (2, 300, 100, 450).drive (tube, 15, 1300)
            .reverb (35, 100, 6.5f, 70).level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Beat")
            .mod (macro2, "ampLevel", 14).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Marching Ostinato", "Cinematic").osc (1, analog, sawPos).unison (1, 5, 8, 90).osc (2, analog, squarePos, 65).pitch (2, 12, 7)
            .lfoSync (1, steps, quarter, 1, 99)
            .stepPattern (1, { 0, -30, -65, -100, -100, -100, -100, -100, 0, -30, -65, -100, -20, -50, -85, -100 }).mod (lfo1, "ampLevel", 85)
            .mod (lfo1, "filter1Cutoff", 12).env (2, 4000, 1000, 100, 500, 30).mod (env2, "filter1Cutoff", 26).filter (1, ladder12, 740, 20, 40)
            .amp (2, 300, 100, 350).reverb (20, 85, 8.5f, 70).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Pulse")
            .mod (macro2, "ampLevel", 14).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Midnight Call", "Cinematic").osc (1, analog, 66).unison (1, 3, 10, 50).route (1, 40).osc (2, pulse, 55, 45).pitch (2, 7, 8)
            .route (2, 60).parallel().subOsc (1).filter (1, ladder24, 570, 5, 10).filter (2, svfBand, 830, 35).env (2, 180, 800, 80, 800)
            .mod (env2, "filter1Cutoff", 22).mod (velocity, "filter1Cutoff", 11).amp (80, 600, 90, 800).reverb (55, 85, 7.5f, 30).level (-11)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Bell").mod (macro2, "filter2Cutoff", 12).macro (3, "Nasal")
            .mod (macro3, "filter2Resonance", 25).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Muffled Rhythm", "Cinematic").osc (1, sub, 0).subOsc (60).lfoSync (1, steps, bar1, 1, 96)
            .stepPattern (1, { 0, 0, -100, -100, -20, -20, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100 }).mod (lfo1, "ampLevel", 78)
            .mod (lfo1, "filter1Cutoff", 14).filter (1, ladder24, 240, 20, 10).amp (2, 300, 100, 300).reverb (15, 80, 5.5f, 70).level (-13)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Beat").mod (macro2, "ampLevel", 14).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Nocturnal Pulse", "Cinematic").osc (1, sub, 10).subOsc (50).lfoSync (1, steps, bar1, 1, 99)
            .stepPattern (1, { 0, 0, -100, -10, -30, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100 }).mod (lfo1, "ampLevel", 72)
            .mod (lfo1, "filter1Cutoff", 8).filter (1, ladder24, 390, 5, 50).amp (2, 300, 100, 550).drive (hard, 35, 800).reverb (20, 80, 6.5f, 35)
            .level (-21).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Beat").mod (macro2, "ampLevel", 14).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Nuclear Metronome", "Cinematic").osc (1, pulse, 75, 75).pitch (1, 24).noise (85, 0, 60).filter (1, ladderBand, 4300, 55)
            .filter (2, svfHigh, 860).lfoSync (1, steps, half, 1, 40)
            .stepPattern (1, { 0, -100, -100, -100, -100, -100, -100, -100, -20, -100, -100, -100, -100, -100, -100, -100 })
            .mod (lfo1, "ampLevel", 79).amp (0, 300, 100, 60).reverb (40, 90, 3, 60).level (-8).macro (1, "Tone").mod (macro1, "filter1Cutoff", 15)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 20).delay (eighthD, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Ochre Rhythm", "Cinematic").osc (1, analog, 10).subOsc (30).lfoSync (1, steps, bars2, 1, 98)
            .stepPattern (1, { 0, 0, -100, -30, -30, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100 }).mod (lfo1, "ampLevel", 75)
            .mod (lfo1, "filter1Cutoff", 16).filter (1, ladder24, 360, 30, 20).amp (2, 300, 100, 500).reverb (25, 100, 8, 35).level (-13)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Beat").mod (macro2, "ampLevel", 14).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Phantom Blow", "Cinematic").osc (1, analog, sinePos).subOsc (50).osc (2, analog, sawPos, 75).unison (2, 3, 16, 100).pitch (2, 0)
            .noise (1, 1).env (2, 0, 110, 0, 100, 88).mod (env2, "osc1Pitch", 39.583f).mod (env2, "noiseLevel", 45).env (3, 0, 600, 0, 500, 70)
            .mod (env3, "filter1Cutoff", 30).filter (1, ladder24, 440, 20, 20).amp (0, 3000, 0, 2000, 65).drive (fuzz, 25, 3800)
            .reverb (45, 85, 8.5f, 35).level (-30).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Drum")
            .mod (macro2, "osc1Pitch", 14.583f).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Chord").mod (macro4, "osc2Volume", -50);
        add ("Pulsing Ostinato", "Cinematic").osc (1, analog, sawPos).unison (1, 3, 16, 60).osc (2, analog, squarePos, 50).pitch (2, 12, 5)
            .lfoSync (1, steps, quarter, 1, 97)
            .stepPattern (1, { 0, -45, -90, -100, -100, -100, -100, -100, 0, -45, -90, -100, -20, -65, -100, -100 }).mod (lfo1, "ampLevel", 80)
            .mod (lfo1, "filter1Cutoff", 6).env (2, 4500, 1000, 100, 500, -20).mod (env2, "filter1Cutoff", 26).filter (1, ladder24, 490, 25, 30)
            .amp (2, 300, 100, 200).drive (hard, 30, 7600).reverb (35, 80, 3, 65).level (-22).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22)
            .macro (2, "Pulse").mod (macro2, "ampLevel", 14).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 25);
        add ("Quarantine Insomnia", "Cinematic").osc (1, analog, sawPos).unison (1, 6, 20, 100, 0, 80).pitch (1, 0).osc (2, analog, sawPos, 60)
            .pitch (2, 5).unison (2, 3, 8, 100, 0, 60).env (2, 7500, 1000, 100, 1000, 20).mod (env2, "osc1UnisonDetune", 40)
            .mod (env2, "filter1Cutoff", 12).lfo (1, sine, 9, 0).mod (lfo1, "ampLevel", 12).filter (1, svfBand, 2200, 15).filter (2, svfHigh, 340)
            .amp (2500, 1000, 100, 2700).reverb (45, 70, 3.5f, 50).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20)
            .macro (2, "Spread").mod (macro2, "osc1UnisonDetune", 20).macro (3, "Semitone").mod (macro3, "osc2Volume", -55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Quarantine Summons", "Cinematic").osc (1, analog, 56).unison (1, 3, 14, 60).route (1, 40).osc (2, pulse, 45, 45).pitch (2, 0, 3)
            .route (2, 40).parallel().subOsc (40).filter (1, ladder24, 340, 10, 30).filter (2, svfBand, 530, 60).env (2, 140, 1100, 50, 800)
            .mod (env2, "filter1Cutoff", 28).mod (velocity, "filter1Cutoff", 5).amp (100, 600, 90, 1300).reverb (45, 75, 3.5f, 40).level (-13)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Bell").mod (macro2, "filter2Cutoff", 12).macro (3, "Nasal")
            .mod (macro3, "filter2Resonance", 25).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Quiet Dawn", "Cinematic").osc (1, analog, sawPos).unison (1, 4, 12, 100).osc (2, analog, 60, 75).pitch (2, 7).unison (2, 2, 14, 100)
            .osc (3, analog, sawPos, 55).pitch (3, -12, 8).subOsc (35).env (2, 2500, 1000, 100, 1500, -10).mod (env2, "filter1Cutoff", 40)
            .mod (env2, "filter1Drive", 50).filter (1, ladder24, 370, 20).amp (1200, 1000, 100, 3000, -20).reverb (45, 80, 6, 40).level (-19)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Fifth").mod (macro2, "osc2Volume", -50).macro (3, "Sub")
            .mod (macro3, "subLevel", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Quiet Paranoia", "Cinematic").osc (1, analog, sawPos).unison (1, 6, 14, 100, 0, 40).pitch (1, 0).osc (2, analog, sawPos).pitch (2, 1)
            .unison (2, 3, 12, 100, 0, 50).env (2, 7000, 1000, 100, 1000, 10).mod (env2, "osc1UnisonDetune", 40).mod (env2, "filter1Cutoff", 16)
            .lfo (1, sine, 6.5f, 0).mod (lfo1, "ampLevel", 8).filter (1, svfLow, 4400, 20).filter (2, svfHigh, 250).amp (600, 1000, 100, 2500)
            .reverb (30, 75, 3.5f, 55).level (-22).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Spread")
            .mod (macro2, "osc1UnisonDetune", 20).macro (3, "Shake").mod (macro3, "ampLevel", 8).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Rabid Deadline", "Cinematic").osc (1, analog, sawPos).unison (1, 4, 12, 60).osc (2, analog, sawPos, 55).pitch (2, 0, 4)
            .lfoSync (1, steps, half, 1, 99).stepPattern (1, { 0, -45, -90, -100, -100, -100, -100, -100, 0, -45, -90, -100, -20, -65, -100, -100 })
            .mod (lfo1, "ampLevel", 75).mod (lfo1, "filter1Cutoff", 8).env (2, 8000, 1000, 100, 500, -30).mod (env2, "filter1Cutoff", 24)
            .filter (1, ladder24, 250, 25).amp (2, 300, 100, 450).drive (hard, 25, 2800).reverb (25, 70, 9, 35).level (-16).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Pulse").mod (macro2, "ampLevel", 14).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Ragged Watch", "Cinematic").osc (1, pulse, 80, 55).pitch (1, 19).noise (85, 3).filter (1, svfBand, 4200, 40)
            .lfoSync (1, steps, half, 1, 70)
            .stepPattern (1, { 0, -100, -100, -100, -100, -100, -100, -100, -20, -100, -100, -100, -100, -100, -100, -100 })
            .mod (lfo1, "ampLevel", 79).amp (0, 300, 100, 120).delay (eighthD, 40, 12, 8500).reverb (15, 75, 3.5f, 45).level (-2).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance").mod (macro2, "filter1Resonance", 20).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Remote Warhorn", "Cinematic").osc (1, analog, 63).unison (1, 2, 10, 50).route (1, 40).osc (2, pulse, 30, 45).pitch (2, -12, 4)
            .route (2, 50).parallel().subOsc (40).filter (1, ladder24, 290, 20, 40).filter (2, svfBand, 790, 35).env (2, 180, 1800, 60, 800)
            .mod (env2, "filter1Cutoff", 18).mod (velocity, "filter1Cutoff", 5).amp (200, 600, 85, 1200).reverb (35, 80, 4, 40).level (-10)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Bell").mod (macro2, "filter2Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Restless Chronometer", "Cinematic").osc (1, pulse, 70, 65).pitch (1, 24).noise (55, 0, 60).filter (1, ladderBand, 1800, 55)
            .filter (2, svfHigh, 1500).lfoSync (1, steps, quarter, 1, 70)
            .stepPattern (1, { 0, -100, -100, -100, -100, -100, -100, -100, 0, -100, -100, -100, -100, -100, -100, -100 })
            .mod (lfo1, "ampLevel", 75).amp (0, 300, 100, 120).delay (sixteenthD, 50, 20, 3300).reverb (25, 70, 7.5f, 40).level (-5)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance").mod (macro2, "filter1Resonance", 20).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Rooftop Ambush", "Cinematic").osc (1, metal, 70).unison (1, 4, 18, 100).osc (2, metal, 70, 75).pitch (2, 1).osc (3, metal, 70, 75)
            .pitch (3, 11).filter (1, ladder12, 1900, 15).env (2, 0, 200, 0, 400, 70).mod (env2, "filter1Cutoff", 22).amp (0, 900, 0, 1300, 65)
            .drive (tube, 20, 2900).delay (quarter, 50, 20, 3400).reverb (55, 100, 6, 50).level (-9).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 30);
        add ("Rooftop Insomnia", "Cinematic").osc (1, analog, sawPos).unison (1, 8, 10, 100, 0, 60).pitch (1, 19).osc (2, analog, sawPos, 60)
            .pitch (2, 20).unison (2, 4, 10, 100, 0, 50).env (2, 3500, 1000, 100, 1000, -10).mod (env2, "osc1UnisonDetune", 40)
            .mod (env2, "filter1Cutoff", 12).lfo (1, sine, 6, 0).mod (lfo1, "ampLevel", 9).filter (1, svfBand, 4300, 15).filter (2, svfHigh, 850)
            .amp (1900, 1000, 100, 2500).reverb (55, 95, 8, 55).level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20)
            .macro (2, "Spread").mod (macro2, "osc1UnisonDetune", 20).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Ruined Sting", "Cinematic").osc (1, pulse, 70).unison (1, 4, 22, 100).osc (2, pulse, 70, 75).pitch (2, 6).osc (3, pulse, 70, 75)
            .pitch (3, 11).filter (1, ladder12, 2300, 15).env (2, 0, 750, 0, 400, 70).mod (env2, "filter1Cutoff", 22).amp (15, 400, 0, 1000, 60)
            .drive (hard, 35, 3000).delay (quarter, 55, 18, 2200).reverb (45, 90, 8.5f, 45).level (-21).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Sacred Lifesign", "Cinematic").osc (1, analog, 20).subOsc (30).lfoSync (1, steps, bar1, 1, 99)
            .stepPattern (1, { 0, 0, -100, -100, -20, -10, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100 }).mod (lfo1, "ampLevel", 77)
            .mod (lfo1, "filter1Cutoff", 20).filter (1, ladder24, 270, 20, 10).amp (2, 300, 100, 600).drive (tube, 25, 1900).reverb (30, 100, 9, 45)
            .level (-16).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Beat").mod (macro2, "ampLevel", 14).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Sampled Metronome", "Cinematic").osc (1, pulse, 80, 60).pitch (1, 19).noise (50, 3, 40).filter (1, ladderBand, 3100, 65)
            .lfoSync (1, steps, bar1, 1, 70)
            .stepPattern (1, { 0, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100 })
            .mod (lfo1, "ampLevel", 87).amp (0, 300, 100, 170).reverb (20, 70, 4.5f, 70).level (-4).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance").mod (macro2, "filter1Resonance", 20).delay (eighthD, 40, 0, 4500)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 35).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Sector Deadline", "Cinematic").osc (1, analog, sawPos).unison (1, 4, 18, 70).osc (2, analog, sawPos, 60).pitch (2, -12, 7)
            .lfoSync (1, steps, quarter, 1, 97)
            .stepPattern (1, { 0, -30, -65, -100, -100, -100, -100, -100, 0, -30, -65, -100, -20, -50, -85, -100 }).mod (lfo1, "ampLevel", 75)
            .mod (lfo1, "filter1Cutoff", 16).env (2, 6500, 1000, 100, 500, 30).mod (env2, "filter1Cutoff", 18).filter (1, ladder24, 330, 20, 30)
            .amp (2, 300, 100, 500).drive (hard, 30, 3800).reverb (15, 95, 4.5f, 60).level (-21).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Pulse").mod (macro2, "ampLevel", 14).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Sepia Tick", "Cinematic").osc (1, pulse, 80, 75).pitch (1, 12).noise (50, 0, 40).filter (1, ladderBand, 1800, 55)
            .lfoSync (1, steps, half, 1, 50)
            .stepPattern (1, { 0, -100, -100, -100, -100, -100, -100, -100, -20, -100, -100, -100, -100, -100, -100, -100 })
            .mod (lfo1, "ampLevel", 87).amp (0, 300, 100, 190).delay (eighth, 40, 26, 7300).reverb (35, 90, 7.5f, 60).level (-9).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance").mod (macro2, "filter1Resonance", 20).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Shattered Twist", "Cinematic").osc (1, fm, 30).unison (1, 4, 12, 100).osc (2, fm, 30, 70).pitch (2, 6).osc (3, fm, 30, 75)
            .pitch (3, 13).filter (1, svfBand, 4400, 30).env (2, 0, 450, 0, 400, 70).mod (env2, "filter1Cutoff", 16).amp (15, 1500, 0, 1000, 65)
            .drive (tube, 40, 4600).delay (quarterD, 55, 20, 1800).reverb (60, 95, 8.5f, 40).level (-4).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Detune").mod (macro2, "osc1UnisonDetune", 25).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Skewed Vertigo", "Cinematic").osc (1, analog, sawPos).unison (1, 8, 14, 100, 0, 50).pitch (1, 19).osc (2, analog, sawPos, 60)
            .pitch (2, 24).unison (2, 3, 12, 100, 0, 30).env (2, 4500, 1000, 100, 1000, 30).mod (env2, "osc1UnisonDetune", 25)
            .mod (env2, "filter1Cutoff", 18).lfo (1, triangle, 6.5f, 0).mod (lfo1, "ampLevel", 3).filter (1, svfBand, 2500, 5)
            .filter (2, svfHigh, 460).amp (700, 1000, 100, 1900).reverb (50, 100, 7, 65).level (-16).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 20).macro (2, "Spread").mod (macro2, "osc1UnisonDetune", 20).macro (3, "Shake")
            .mod (macro3, "ampLevel", 8).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Southern Suspicion", "Cinematic").osc (1, analog, sawPos).unison (1, 8, 12, 100, 0, 50).pitch (1, 19).osc (2, analog, sawPos)
            .pitch (2, 20).unison (2, 4, 10, 100, 0, 30).env (2, 3000, 1000, 100, 1000, -30).mod (env2, "osc1UnisonDetune", 40)
            .mod (env2, "filter1Cutoff", 8).lfo (1, triangle, 6.5f, 0).mod (lfo1, "ampLevel", 8).filter (1, svfBand, 1600, 5)
            .filter (2, svfHigh, 260).amp (1100, 1000, 100, 1700).reverb (40, 70, 7.5f, 65).level (-20).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 20).macro (2, "Spread").mod (macro2, "osc1UnisonDetune", 20).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Spartan Impact", "Cinematic").osc (1, analog, 15).subOsc (50).osc (2, analog, sawPos, 70).unison (2, 5, 14, 100).pitch (2, 0)
            .noise (1, 1, -60).env (2, 0, 120, 0, 100, 88).mod (env2, "osc1Pitch", 50).mod (env2, "noiseLevel", 50).env (3, 0, 1300, 0, 500, 70)
            .mod (env3, "filter1Cutoff", 40).filter (1, ladder24, 270, 30, 50).amp (0, 3500, 0, 2000, 45).drive (hard, 45, 4900)
            .reverb (55, 90, 7.5f, 40).level (-25).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Drum")
            .mod (macro2, "osc1Pitch", 14.583f).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Spinning Fuse", "Cinematic").osc (1, analog, sawPos).unison (1, 4, 12, 60).osc (2, analog, squarePos, 60).pitch (2, -12, 3)
            .lfoSync (1, steps, half, 1, 99).stepPattern (1, { 0, -30, -65, -100, -100, -100, -100, -100, 0, -30, -65, -100, -20, -50, -85, -100 })
            .mod (lfo1, "ampLevel", 80).mod (lfo1, "filter1Cutoff", 12).env (2, 4000, 1000, 100, 500, 10).mod (env2, "filter1Cutoff", 24)
            .filter (1, ladder12, 260, 30).amp (2, 300, 100, 500).reverb (15, 70, 3.5f, 45).level (-16).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Pulse").mod (macro2, "ampLevel", 14).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Splintered Touchdown", "Cinematic").osc (1, sub, 0).subOsc (60).osc (2, analog, sawPos, 50).unison (2, 5, 12, 100).pitch (2, 7)
            .noise (1, 1, -60).env (2, 0, 180, 0, 100, 88).mod (env2, "osc1Pitch", 39.583f).mod (env2, "noiseLevel", 45)
            .env (3, 0, 1300, 0, 500, 70).mod (env3, "filter1Cutoff", 35).filter (1, ladder24, 210, 15, 60).amp (0, 3500, 0, 2500, 65)
            .drive (hard, 50, 3100).reverb (35, 90, 7.5f, 65).level (-27).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Drum")
            .mod (macro2, "osc1Pitch", 14.583f).macro (3, "Grit").mod (macro3, "fxDriveAmount", 40).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Starless Pulse", "Cinematic").osc (1, analog, sawPos).unison (1, 4, 18, 100).osc (2, analog, sawPos, 55).pitch (2, -12, 4)
            .lfoSync (1, steps, quarter, 1, 99).stepPattern (1, { 0, -45, -90, -100, 0, -45, -90, -100, 0, -45, -90, -100, 0, -45, -90, -100 })
            .mod (lfo1, "ampLevel", 75).mod (lfo1, "filter1Cutoff", 6).env (2, 9000, 1000, 100, 500, 10).mod (env2, "filter1Cutoff", 18)
            .filter (1, ladder12, 620, 35, 40).amp (2, 300, 100, 500).reverb (20, 80, 6, 65).level (-16).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Pulse").mod (macro2, "ampLevel", 14).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Stern Countdown", "Cinematic").osc (1, analog, sawPos).unison (1, 5, 16, 70).osc (2, analog, sawPos, 55).pitch (2, 12, 8)
            .lfoSync (1, steps, quarter, 1, 98).stepPattern (1, { 0, -20, -50, -85, 0, -20, -50, -85, 0, -20, -50, -85, 0, -20, -50, -85 })
            .mod (lfo1, "ampLevel", 80).mod (lfo1, "filter1Cutoff", 12).env (2, 6000, 1000, 100, 500, 0).mod (env2, "filter1Cutoff", 14)
            .filter (1, ladder24, 560, 20, 40).amp (2, 300, 100, 200).reverb (20, 85, 5, 55).level (-13).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 22).macro (2, "Pulse").mod (macro2, "ampLevel", 14).drive (hard, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Vast Vertigo", "Cinematic").osc (1, analog, sawPos).unison (1, 4, 8, 100, 0, 80).pitch (1, 0).osc (2, analog, sawPos, 60).pitch (2, 5)
            .unison (2, 4, 10, 100, 0, 30).env (2, 8500, 1000, 100, 1000, 0).mod (env2, "osc1UnisonDetune", 25).mod (env2, "filter1Cutoff", 12)
            .lfo (1, sine, 11, 0).mod (lfo1, "ampLevel", 5).filter (1, ladder12, 2800, 30).filter (2, svfHigh, 430).amp (1900, 1000, 100, 3500)
            .reverb (40, 75, 7.5f, 55).level (-14).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Spread")
            .mod (macro2, "osc1UnisonDetune", 20).macro (3, "Semitone").mod (macro3, "osc2Volume", -55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Violent Monolith", "Cinematic").osc (1, analog, sawPos).unison (1, 3, 18, 90).osc (2, analog, 70).pitch (2, 12, 10).unison (2, 2, 10)
            .subOsc (55).filter (1, ladder24, 200, 35, 60).env (2, 120, 1300, 60, 1500, 60).mod (env2, "filter1Cutoff", 40)
            .mod (env2, "osc2Position", 30).amp (50, 600, 90, 2400).drive (hard, 55, 2600).reverb (45, 95, 5.5f, 50).level (-29).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 25).macro (2, "Growl").mod (macro2, "osc2Position", 30).macro (3, "Detune")
            .mod (macro3, "osc1UnisonDetune", 25).macro (4, "Sub").mod (macro4, "subLevel", 35);
        add ("Voiceless Horn", "Cinematic").osc (1, analog, 65).unison (1, 4, 6, 70).route (1, 40).osc (2, pulse, 25, 65).pitch (2, -12, 7)
            .route (2, 30).parallel().subOsc (1).filter (1, ladder24, 370, 15, 20).filter (2, svfBand, 650, 55).env (2, 300, 1100, 50, 800)
            .mod (env2, "filter1Cutoff", 26).mod (velocity, "filter1Cutoff", 8).amp (120, 600, 90, 1200).reverb (35, 100, 3, 45).level (-12)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 22).macro (2, "Bell").mod (macro2, "filter2Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);

        // Perc, by name.
        add ("Abrasive Lid", "Perc").osc (1, analog, 50).pitch (1, -5).warp (1, wCrush, 50).noise (30, 2).env (2, 0, 110, 0, 60, 88)
            .mod (env2, "osc1Pitch", 6.25f).mod (env2, "filter1Cutoff", 13).filter (1, svfBand, 1200, 35).amp (0, 110, 0, 230, 75)
            .drive (hard, 40, 8100).reverb (10, 50, 1.4f, 40).level (-15).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Barrack Tock", "Perc").osc (1, analog, sinePos).pitch (1, 7).osc (2, analog, sinePos, 70).pitch (2, 12, 10).noise (1, 0, 40)
            .env (2, 0, 7, 0, 8, 90).mod (env2, "noiseLevel", 50).mod (env2, "osc1Fine", 20).filter (1, svfBand, 260, 25, 0, 50)
            .amp (0, 320, 0, 90, 70).drive (tube, 40, 3200).reverb (20, 45, 1.6f, 60).level (-23).macro (1, "Tune")
            .mod (macro1, "osc1Pitch", 14.583f).macro (2, "Click").mod (macro2, "noiseLevel", 35).macro (3, "Cutoff")
            .mod (macro3, "filter1Cutoff", 20).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Blistered Agogo", "Perc").osc (1, pulse, 60).osc (2, analog, 90).pitch (1, 7).pitch (2, 15, -10).filter (1, ladderBand, 2200, 25)
            .env (2, 0, 60, 0, 30, 85).mod (env2, "ampLevel", 10).amp (0, 360, 0, 240, 55).reverb (20, 50, 1, 70).level (-10).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 18).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 40).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Bolted Cylinder", "Perc").osc (1, analog, sawPos, 1).noise (1, 0, 20).env (2, 0, 10, 0, 15, 0).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 65).filter (1, comb, 196, 94, 0, 100).filter (2, ladder12, 1700, 15).amp (0, 550, 0, 550, 65)
            .reverb (10, 35, 1.2f, 40).level (-5).macro (1, "Damp").mod (macro1, "filter1Resonance", -15).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 22).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Bruised Clank", "Perc").osc (1, analog, sinePos, 1).pitch (1, 14, -40).osc (2, analog, sinePos).warp (2, wFm, 30).pitch (2, 12)
            .env (2, 0, 130, 0, 150, 85).mod (env2, "osc2WarpAmount", 40).filter (1, svfBand, 1200, 45).amp (0, 400, 0, 140, 75)
            .drive (crush, 15, 9800).reverb (10, 50, 1.4f, 40).level (-11).macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).delay (sixteenth, 40, 0, 4500).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Bunker Boing", "Perc").osc (1, analog, 60, 1).noise (1, 2, 40).env (2, 0, 16, 0, 15, 0).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 80).filter (1, comb, 230, 100).env (3, 0, 160, 0, 300, 45).mod (env3, "filter1Cutoff", -9)
            .filter (2, svfHigh, 280, 25).amp (0, 1100, 0, 300, 65).reverb (25, 30, 1.2f, 40).level (-5).macro (1, "Damp")
            .mod (macro1, "filter1Resonance", -15).macro (2, "Tune").mod (macro2, "filter1Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 40).macro (4, "Tone").mod (macro4, "filter2Cutoff", 22);
        add ("Bunker Recoil", "Perc").osc (1, pulse, 60, 1).noise (1, 3, 40).env (2, 0, 16, 0, 15, 40).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 60).filter (1, comb, 620, 93).env (3, 0, 500, 0, 300, 85).mod (env3, "filter1Cutoff", 18)
            .filter (2, svfBand, 420, 15).amp (0, 650, 0, 550, 65).reverb (20, 45, 1, 40).level (-3).macro (1, "Damp")
            .mod (macro1, "filter1Resonance", -15).macro (2, "Tune").mod (macro2, "filter1Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 40).macro (4, "Tone").mod (macro4, "filter2Cutoff", 22);
        add ("Caged Canister", "Perc").osc (1, analog, sawPos, 1).noise (1, 0, 60).env (2, 0, 18, 0, 15, 20).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 65).filter (1, comb, 392, 96, 0, 100).filter (2, ladder12, 410, 15).amp (0, 400, 0, 400, 75)
            .drive (tube, 30, 3500).reverb (15, 40, 1.2f, 40).level (-9).macro (1, "Damp").mod (macro1, "filter1Resonance", -15).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 22).macro (3, "Tune").mod (macro3, "filter1Cutoff", 12).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Caustic Clank", "Perc").osc (1, analog, squarePos, 1).pitch (1, 17).osc (2, analog, sinePos).warp (2, wFm, 55).pitch (2, 0)
            .env (2, 0, 250, 0, 150, 80).mod (env2, "osc2WarpAmount", 35).filter (1, ladder12, 650).amp (0, 160, 0, 80, 85)
            .reverb (25, 45, 0.8f, 60).level (-7).macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).delay (sixteenth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Caustic Forge", "Perc").osc (1, metal, 75).pitch (1, 7).noise (1).env (2, 0, 14, 0, 20, 90).mod (env2, "noiseLevel", 60)
            .env (3, 0, 200, 0, 200, 85).mod (env3, "osc1Position", -30).filter (1, ladderBand, 1800, 30, 40).amp (0, 160, 0, 320, 75)
            .reverb (10, 45, 1.4f, 70).level (-3).macro (1, "Clang").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Strike").mod (macro3, "noiseLevel", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Cellar Canister", "Perc").osc (1, pulse, 85, 1).noise (1, 0, 20).env (2, 0, 24, 0, 15, 0).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 60).filter (1, comb, 392, 100, 0, 100).filter (2, ladder12, 540, 20).amp (0, 750, 0, 500, 65)
            .reverb (15, 45, 0.8f, 70).level (-3).macro (1, "Damp").mod (macro1, "filter1Resonance", -15).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 22).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Tune")
            .mod (macro4, "filter1Cutoff", 12);
        add ("Chromium Tether", "Perc").set ("osc1On", 0).noise (75, 2).filter (1, comb, 1300, 75).filter (2, svfBand, 1900, 40)
            .lfo (1, randomShape, 9.3f).mod (lfo1, "filter1Cutoff", 6).amp (0, 700, 0, 250).drive (tube, 25, 7500).reverb (20, 30, 1.2f, 50)
            .level (2).macro (1, "Ring").mod (macro1, "filter1Resonance", 25).macro (2, "Tone").mod (macro2, "filter2Cutoff", 20)
            .macro (3, "Rattle").mod (macro3, "lfo1Rate", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Coded Peck", "Perc").osc (1, analog, sinePos).pitch (1, 0).osc (2, analog, sinePos, 70).pitch (2, 6, -20).noise (1, 3, 60)
            .env (2, 0, 9, 0, 8, 90).mod (env2, "noiseLevel", 75).mod (env2, "osc1Fine", 40).filter (1, svfBand, 1800, 20, 0, 50)
            .amp (0, 260, 0, 210, 75).reverb (10, 55, 1.4f, 40).level (1).macro (1, "Tune").mod (macro1, "osc1Pitch", 14.583f).macro (2, "Click")
            .mod (macro2, "noiseLevel", 35).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Cold Rivet", "Perc").osc (1, metal, 90).pitch (1, 19).osc (2, metal, 50, 55).pitch (2, 11, 10).noise (1).env (2, 0, 30, 0, 20, 90)
            .mod (env2, "noiseLevel", 45).env (3, 0, 160, 0, 200, 85).mod (env3, "osc1Position", -15).filter (1, svfHigh, 3000, 35, 10)
            .amp (0, 300, 0, 340, 65).drive (hard, 40, 6900).reverb (20, 50, 0.8f, 70).level (-9).macro (1, "Clang")
            .mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Condemned Chain", "Perc").set ("osc1On", 0).noise (85, 2, 40).filter (1, comb, 1200, 85).filter (2, svfHigh, 1200, 15)
            .lfo (1, randomShape, 14).mod (lfo1, "filter1Cutoff", 8).amp (15, 200, 0, 500).drive (hard, 20, 4000).reverb (10, 30, 1.6f, 70)
            .level (4).macro (1, "Ring").mod (macro1, "filter1Resonance", 25).macro (2, "Tone").mod (macro2, "filter2Cutoff", 20).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 40).macro (4, "Rattle").mod (macro4, "lfo1Rate", 25);
        add ("Curfew Jangle", "Perc").osc (1, analog, sinePos, 1).pitch (1, 22, -30).osc (2, analog, sinePos).warp (2, wFm, 25).pitch (2, 12)
            .env (2, 0, 180, 0, 150, 80).mod (env2, "osc2WarpAmount", 30).filter (1, ladder12, 1600, 25).amp (0, 380, 0, 240, 60)
            .reverb (15, 55, 1.6f, 40).level (-11).macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).delay (sixteenth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Dayshift Spark", "Perc").osc (1, fm, 0).pitch (1, 0).env (2, 0, 220, 0, 100, 72).mod (env2, "osc1Pitch", 25)
            .filter (1, svfBand, 1300, 30, 20).amp (0, 460, 0, 320, 80).reverb (25, 40, 1, 40).level (-4).macro (1, "Sweep")
            .mod (macro1, "osc1Pitch", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).delay (sixteenth, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Fearless Agogo", "Perc").osc (1, analog, squarePos).osc (2, pulse, 65).pitch (1, 5).pitch (2, 12, -10).filter (1, svfBand, 2100, 55)
            .env (2, 0, 60, 0, 30, 85).mod (env2, "ampLevel", 10).amp (0, 520, 0, 380, 55).drive (hard, 25, 5100).reverb (20, 45, 1, 70).level (-17)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Feverish Sledge", "Perc").osc (1, metal, 70).pitch (1, 12).osc (2, metal, 50, 50).pitch (2, -7, 20).noise (1, 3)
            .env (2, 0, 30, 0, 20, 90).mod (env2, "noiseLevel", 50).env (3, 0, 280, 0, 200, 85).mod (env3, "osc1Position", 30)
            .filter (1, svfBand, 2100, 15).amp (0, 700, 0, 320, 85).drive (fuzz, 30, 11000).reverb (10, 45, 0.8f, 60).level (-17).macro (1, "Clang")
            .mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Strike")
            .mod (macro3, "noiseLevel", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);
        add ("Forged Barrel", "Perc").osc (1, pulse, 60, 1).noise (1, 0, 40).env (2, 0, 12, 0, 15, 20).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 80).filter (1, comb, 261.6f, 99, 0, 100).filter (2, svfHigh, 620, 35).amp (0, 1100, 0, 550, 65)
            .reverb (20, 55, 1.4f, 50).level (-3).macro (1, "Damp").mod (macro1, "filter1Resonance", -15).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 40).macro (4, "Tune")
            .mod (macro4, "filter1Cutoff", 12);
        add ("Formless Link", "Perc").set ("osc1On", 0).noise (75, 2).filter (1, comb, 1300, 90).filter (2, svfHigh, 3300, 50)
            .lfo (1, randomShape, 11).mod (lfo1, "filter1Cutoff", 3).amp (15, 500, 0, 400, 70).drive (tube, 30).reverb (25, 35, 1.4f, 60).level (-1)
            .macro (1, "Ring").mod (macro1, "filter1Resonance", 25).macro (2, "Tone").mod (macro2, "filter2Cutoff", 20).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Fugitive Sand", "Perc").set ("osc1On", 0).noise (95, 0, 80).filter (1, svfBand, 3300, 25).env (2, 20, 110, 0, 60, 60)
            .mod (env2, "filter1Cutoff", 8).amp (28, 100, 0, 100, 40).reverb (12, 45, 1.4f, 60).level (-5).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance").mod (macro2, "filter1Resonance", 35).macro (3, "Colour")
            .mod (macro3, "noiseColour", -40).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Galvanised Coil", "Perc").osc (1, pulse, 66.67f, 1).noise (1, 3, -20).env (2, 0, 19, 0, 15, 40).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 65).filter (1, comb, 350, 93).env (3, 0, 420, 0, 300, 85).mod (env3, "filter1Cutoff", 18)
            .filter (2, svfBand, 970, 20).amp (0, 550, 0, 200, 65).reverb (20, 35, 1, 40).level (1).macro (1, "Damp")
            .mod (macro1, "filter1Resonance", -15).macro (2, "Tune").mod (macro2, "filter1Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 40).macro (4, "Tone").mod (macro4, "filter2Cutoff", 22);
        add ("Glacial Shaker", "Perc").set ("osc1On", 0).noise (90).filter (1, ladderHigh, 8900, 25).env (2, 15, 50, 0, 60, 60)
            .mod (env2, "filter1Cutoff", 10).amp (26, 180, 0, 100, 70).reverb (12, 45, 1, 70).level (-3).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance").mod (macro2, "filter1Resonance", 35).macro (3, "Colour")
            .mod (macro3, "noiseColour", -40).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Guarded Gong", "Perc").osc (1, analog, squarePos).osc (2, analog, 90).pitch (1, 12).pitch (2, 19, 10).filter (1, svfBand, 1500, 45)
            .env (2, 0, 15, 0, 30, 85).mod (env2, "ampLevel", 14).amp (0, 600, 0, 280, 60).reverb (15, 40, 0.8f, 60).level (-12).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 18).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 40).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Halogen Seed", "Perc").set ("osc1On", 0).noise (75, 0, 80).filter (1, ladderHigh, 3900, 15).env (2, 10, 110, 0, 60, 60)
            .mod (env2, "filter1Cutoff", 4).amp (8, 210, 0, 120, 60).reverb (20, 40, 1.4f, 70).level (-3).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance").mod (macro2, "filter1Resonance", 35).macro (3, "Colour")
            .mod (macro3, "noiseColour", -40).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Heartless Agogo", "Perc").osc (1, analog, squarePos).osc (2, analog, squarePos).pitch (1, 9).pitch (2, 17, 40)
            .filter (1, svfBand, 1200, 45).env (2, 0, 40, 0, 30, 85).mod (env2, "ampLevel", 10).amp (0, 160, 0, 180, 65).reverb (15, 30, 0.8f, 50)
            .level (-15).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30)
            .drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Heavy Spark", "Perc").osc (1, fm, 0).pitch (1, -12).env (2, 0, 70, 0, 100, 92).mod (env2, "osc1Pitch", 64.583f).noise (1, 0, 60)
            .env (3, 0, 30, 0, 30, 90).mod (env3, "noiseLevel", 35).filter (1, svfBand, 5600, 20, 30).amp (0, 120, 0, 160, 75)
            .drive (crush, 45, 3100).delay (sixteenth, 30, 10, 2100).reverb (25, 50, 1.4f, 40).level (5).macro (1, "Sweep")
            .mod (macro1, "osc1Pitch", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Hungry Forge", "Perc").osc (1, metal, 50).pitch (1, 12).noise (1).env (2, 0, 8, 0, 20, 90).mod (env2, "noiseLevel", 45)
            .env (3, 0, 300, 0, 200, 85).mod (env3, "osc1Position", 30).filter (1, svfHigh, 3800, 35).amp (0, 180, 0, 120, 85)
            .drive (fuzz, 40, 11000).reverb (20, 50, 1.2f, 70).level (-20).macro (1, "Clang").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).macro (4, "Strike")
            .mod (macro4, "noiseLevel", 35);
        add ("Infected Bounce", "Perc").osc (1, analog, 60, 1).noise (1, 3, -20).env (2, 0, 11, 0, 15, 20).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 55).filter (1, comb, 200, 98).env (3, 0, 100, 0, 300, 65).mod (env3, "filter1Cutoff", 9)
            .filter (2, svfHigh, 330, 25).amp (0, 300, 0, 400, 75).reverb (10, 35, 0.8f, 60).level (-3).macro (1, "Damp")
            .mod (macro1, "filter1Resonance", -15).macro (2, "Tune").mod (macro2, "filter1Cutoff", 12).macro (3, "Tone")
            .mod (macro3, "filter2Cutoff", 22).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Inland Jolt", "Perc").osc (1, analog, 25).pitch (1, -12).env (2, 0, 60, 0, 100, 90).mod (env2, "osc1Pitch", 25).noise (1)
            .env (3, 0, 20, 0, 30, 90).mod (env3, "noiseLevel", 40).filter (1, svfLow, 2200, 25, 10).amp (0, 240, 0, 340, 70)
            .reverb (20, 30, 0.8f, 50).level (-11).macro (1, "Sweep").mod (macro1, "osc1Pitch", 25).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).delay (sixteenth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Ivory Rap", "Perc").osc (1, analog, sinePos).pitch (1, 12).noise (1, 3, 20).env (2, 0, 6, 0, 8, 90).mod (env2, "noiseLevel", 50)
            .mod (env2, "osc1Fine", 10).filter (1, svfBand, 1000, 10, 0, 50).amp (0, 100, 0, 190, 65).reverb (20, 45, 1.6f, 60).level (-9)
            .macro (1, "Tune").mod (macro1, "osc1Pitch", 14.583f).macro (2, "Click").mod (macro2, "noiseLevel", 35).macro (3, "Cutoff")
            .mod (macro3, "filter1Cutoff", 20).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Jade Rap", "Perc").osc (1, analog, sinePos).pitch (1, 5).osc (2, analog, sinePos, 50).pitch (2, 11).noise (1, 3, 40)
            .env (2, 0, 4, 0, 8, 90).mod (env2, "noiseLevel", 70).mod (env2, "osc1Fine", 30).filter (1, svfBand, 1000, 20, 0, 50)
            .amp (0, 280, 0, 160, 75).reverb (15, 55, 0.6f, 50).level (-6).macro (1, "Tune").mod (macro1, "osc1Pitch", 14.583f).macro (2, "Click")
            .mod (macro2, "noiseLevel", 35).macro (3, "Space").mod (macro3, "fxReverbMix", 40).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Jade Rivet", "Perc").osc (1, metal, 50).pitch (1, 7).noise (1, 3).env (2, 0, 24, 0, 20, 90).mod (env2, "noiseLevel", 30)
            .env (3, 0, 100, 0, 200, 85).mod (env3, "osc1Position", -15).filter (1, svfHigh, 3000, 20, 10).amp (0, 200, 0, 440, 80)
            .drive (tube, 15, 2600).reverb (25, 30, 1.6f, 60).level (2).macro (1, "Clang").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Strike").mod (macro3, "noiseLevel", 35).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Listing Sledge", "Perc").osc (1, metal, 60).pitch (1, -5).noise (1, 3).env (2, 0, 6, 0, 20, 90).mod (env2, "noiseLevel", 60)
            .env (3, 0, 120, 0, 200, 85).mod (env3, "osc1Position", 30).filter (1, ladderHigh, 620, 30).amp (0, 180, 0, 100, 70)
            .drive (hard, 45, 9200).reverb (15, 55, 1.4f, 60).level (-15).macro (1, "Clang").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Logic Jangle", "Perc").osc (1, analog, squarePos, 1).pitch (1, 6, -10).osc (2, analog, trianglePos).warp (2, wFm, 55).pitch (2, 0)
            .env (2, 0, 210, 0, 150, 85).mod (env2, "osc2WarpAmount", 25).filter (1, ladder12, 1800, 25).amp (0, 480, 0, 320, 60)
            .drive (crush, 15, 2700).reverb (20, 30, 0.8f, 40).level (-9).macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).delay (sixteenth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Logic Tick", "Perc").osc (1, analog, trianglePos).pitch (1, 12).noise (1, 3).env (2, 0, 2, 0, 8, 90).mod (env2, "noiseLevel", 55)
            .mod (env2, "osc1Fine", 40).filter (1, ladder12, 1400, 45, 0, 50).amp (0, 400, 0, 170, 65).drive (tube, 20, 3000)
            .reverb (15, 55, 1.4f, 50).level (-12).macro (1, "Tune").mod (macro1, "osc1Pitch", 14.583f).macro (2, "Click")
            .mod (macro2, "noiseLevel", 35).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Lunar Rattle", "Perc").osc (1, analog, squarePos, 1).pitch (1, 14, 30).osc (2, analog, trianglePos).warp (2, wFm, 45).pitch (2, 12)
            .env (2, 0, 40, 0, 150, 75).mod (env2, "osc2WarpAmount", 35).filter (1, ladder12, 930, 15).amp (0, 380, 0, 100, 75)
            .drive (fuzz, 45, 7200).reverb (15, 45, 1.2f, 50).level (-30).macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 40).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Manganese Bucket", "Perc").osc (1, metal, 66.67f).pitch (1, 12).warp (1, wCrush, 45).noise (35, 2).env (2, 0, 70, 0, 60, 88)
            .mod (env2, "osc1Pitch", 14.583f).mod (env2, "filter1Cutoff", 8).filter (1, svfHigh, 950, 40).amp (0, 70, 0, 180, 80)
            .drive (crush, 25, 7900).reverb (15, 45, 0.8f, 50).level (-1).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Mirror Peck", "Perc").osc (1, analog, trianglePos).pitch (1, 12).osc (2, analog, sinePos, 65).pitch (2, 23, -40).noise (1, 0, 60)
            .env (2, 0, 12, 0, 8, 90).mod (env2, "noiseLevel", 55).mod (env2, "osc1Fine", 40).filter (1, svfHigh, 1800, 40, 0, 50)
            .amp (0, 80, 0, 150, 70).reverb (15, 45, 1.6f, 40).level (4).macro (1, "Tune").mod (macro1, "osc1Pitch", 14.583f).macro (2, "Click")
            .mod (macro2, "noiseLevel", 35).macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 20).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Nomad Agogo", "Perc").osc (1, analog, squarePos).osc (2, pulse, 30).pitch (1, 9).pitch (2, 17, 20).filter (1, ladderBand, 3100, 55)
            .env (2, 0, 40, 0, 30, 85).mod (env2, "ampLevel", 16).amp (0, 460, 0, 100, 65).reverb (10, 55, 1, 40).level (-12).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 18).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).drive (hard, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Northern Seed", "Perc").set ("osc1On", 0).noise (95, 0, 20).filter (1, svfHigh, 5500, 40).env (2, 15, 140, 0, 60, 60)
            .mod (env2, "filter1Cutoff", 10).amp (30, 100, 0, 160, 40).reverb (16, 35, 1.4f, 70).level (-4).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance").mod (macro2, "filter1Resonance", 35).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 40).drive (crush, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Overnight Chaff", "Perc").set ("osc1On", 0).noise (85, 2, 40).filter (1, svfBand, 7700, 25).env (2, 30, 110, 0, 60, 60)
            .mod (env2, "filter1Cutoff", 10).amp (4, 80, 0, 120).reverb (16, 50, 1.2f, 40).level (6).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance").mod (macro2, "filter1Resonance", 35).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 40).macro (4, "Colour").mod (macro4, "noiseColour", -40);
        add ("Pewter Leash", "Perc").set ("osc1On", 0).noise (85, 2).filter (1, comb, 310, 60).filter (2, svfHigh, 1500, 20)
            .lfo (1, randomShape, 25).mod (lfo1, "filter1Cutoff", 7).amp (0, 500, 0, 450, 40).reverb (10, 30, 1.4f, 70).level (4).macro (1, "Ring")
            .mod (macro1, "filter1Resonance", 25).macro (2, "Tone").mod (macro2, "filter2Cutoff", 20).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 40).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Quantised Tin", "Perc").osc (1, metal, 33.33f).pitch (1, 7).warp (1, wCrush, 65).noise (35).env (2, 0, 40, 0, 60, 88)
            .mod (env2, "osc1Pitch", 10.417f).mod (env2, "filter1Cutoff", 12).filter (1, ladderBand, 3400, 30).amp (0, 210, 0, 160, 80)
            .reverb (10, 45, 1.4f, 60).level (0).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).macro (3, "Steps").mod (macro3, "osc1WarpAmount", 30).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Rabid Conduit", "Perc").osc (1, pulse, 60, 1).noise (1, 3, 40).env (2, 0, 25, 0, 15, 40).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 55).filter (1, comb, 196, 97, 0, 100).filter (2, svfBand, 420, 30).amp (0, 650, 0, 550, 60)
            .reverb (10, 40, 0.6f, 70).level (-2).macro (1, "Damp").mod (macro1, "filter1Resonance", -15).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 22).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Tune")
            .mod (macro4, "filter1Cutoff", 12);
        add ("Rancid Leash", "Perc").set ("osc1On", 0).noise (70, 2).filter (1, comb, 690, 65).filter (2, svfBand, 4700, 25)
            .lfo (1, randomShape, 16).mod (lfo1, "filter1Cutoff", 8).amp (15, 550, 0, 300, 60).drive (hard, 30, 5400).reverb (10, 45, 0.8f, 40)
            .level (1).macro (1, "Ring").mod (macro1, "filter1Resonance", 25).macro (2, "Tone").mod (macro2, "filter2Cutoff", 20).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 40).macro (4, "Rattle").mod (macro4, "lfo1Rate", 25);
        add ("Raw Spring", "Perc").osc (1, analog, sawPos, 1).noise (1, 2, 20).env (2, 0, 25, 0, 15, 0).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 85).filter (1, comb, 330, 98).env (3, 0, 300, 0, 300, 65).mod (env3, "filter1Cutoff", -17)
            .filter (2, svfBand, 280).amp (0, 350, 0, 250, 75).reverb (25, 30, 1.4f, 40).level (-4).macro (1, "Damp")
            .mod (macro1, "filter1Resonance", -15).macro (2, "Tune").mod (macro2, "filter1Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 40).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Riveted Cylinder", "Perc").osc (1, analog, sawPos, 1).noise (1, 0, -20).env (2, 0, 13, 0, 15, 20).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 90).filter (1, comb, 523.3f, 94, 0, 100).filter (2, ladder12, 2100, 15).amp (0, 950, 0, 350, 65)
            .reverb (25, 55, 0.8f, 70).level (-5).macro (1, "Damp").mod (macro1, "filter1Resonance", -15).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 22).macro (3, "Tune").mod (macro3, "filter1Cutoff", 12).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Sampled Drum", "Perc").osc (1, analog, sawPos).pitch (1, 5).warp (1, wCrush, 65).noise (25, 2).env (2, 0, 120, 0, 60, 88)
            .mod (env2, "osc1Pitch", 10.417f).mod (env2, "filter1Cutoff", 8).filter (1, svfHigh, 750, 25).amp (0, 150, 0, 140, 70)
            .drive (hard, 25, 2500).reverb (10, 40, 1.6f, 60).level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Steps")
            .mod (macro4, "osc1WarpAmount", 30);
        add ("Sampled Rivet", "Perc").osc (1, metal, 75).pitch (1, 12).noise (1, 3).env (2, 0, 6, 0, 20, 90).mod (env2, "noiseLevel", 40)
            .env (3, 0, 280, 0, 200, 85).mod (env3, "osc1Position", -15).filter (1, ladderHigh, 2100, 15, 40).amp (0, 580, 0, 220, 75)
            .reverb (10, 50, 0.8f, 60).level (-2).macro (1, "Clang").mod (macro1, "osc1Position", 30).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Scarlet Disc", "Perc").osc (1, pulse, 20).osc (2, pulse, 65).pitch (1, 7).pitch (2, 14).filter (1, ladderBand, 940, 60)
            .env (2, 0, 60, 0, 30, 85).mod (env2, "ampLevel", 14).amp (0, 360, 0, 180, 80).drive (tube, 30, 6800).reverb (20, 30, 1.6f, 40)
            .level (-20).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Resonance").mod (macro2, "filter1Resonance", 30)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Serial Bolt", "Perc").osc (1, analog, sinePos).pitch (1, 0).env (2, 0, 70, 0, 100, 70).mod (env2, "osc1Pitch", 75).noise (1, 3, 60)
            .env (3, 0, 15, 0, 30, 90).mod (env3, "noiseLevel", 45).filter (1, svfLow, 2300, 20, 20).amp (0, 560, 0, 100, 80)
            .drive (crush, 35, 11000).reverb (10, 50, 0.8f, 40).level (-13).macro (1, "Sweep").mod (macro1, "osc1Pitch", 25).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Serrated Agogo", "Perc").osc (1, analog, 90).osc (2, analog, squarePos).pitch (1, 9).pitch (2, 20, 20)
            .filter (1, ladderBand, 1100, 55).env (2, 0, 45, 0, 30, 85).mod (env2, "ampLevel", 14).amp (0, 460, 0, 340, 65)
            .reverb (20, 30, 0.8f, 50).level (-16).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Serrated Drum", "Perc").osc (1, analog, trianglePos).pitch (1, 12).warp (1, wCrush, 55).noise (20).env (2, 0, 60, 0, 60, 88)
            .mod (env2, "osc1Pitch", 10.417f).mod (env2, "filter1Cutoff", 10).filter (1, ladderBand, 1300, 55).amp (0, 90, 0, 180, 65)
            .reverb (15, 30, 0.6f, 50).level (6).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Steps")
            .mod (macro4, "osc1WarpAmount", 30);
        add ("Severe Chaff", "Perc").set ("osc1On", 0).noise (70, 0, 40).filter (1, svfHigh, 3200, 35).env (2, 15, 60, 0, 60, 60)
            .mod (env2, "filter1Cutoff", 6).amp (20, 200, 0, 100).reverb (8, 30, 0.6f, 70).level (0).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance").mod (macro2, "filter1Resonance", 35).drive (crush, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Colour").mod (macro4, "noiseColour", -40);
        add ("Shredded Clash", "Perc").osc (1, analog, squarePos, 1).pitch (1, 23, 10).osc (2, analog, trianglePos).warp (2, wFm, 20).pitch (2, 7)
            .env (2, 0, 70, 0, 150, 75).mod (env2, "osc2WarpAmount", 30).filter (1, svfBand, 730, 35).amp (0, 280, 0, 100, 85)
            .drive (fuzz, 30, 8000).reverb (15, 30, 0.6f, 70).level (-24).macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 40).delay (sixteenth, 40, 0, 4500).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Splintered Rattle", "Perc").osc (1, analog, sinePos, 1).pitch (1, 14, -40).osc (2, analog, trianglePos).warp (2, wFm, 55).pitch (2, 7)
            .env (2, 0, 140, 0, 150, 90).mod (env2, "osc2WarpAmount", 30).filter (1, svfBand, 850, 35).amp (0, 480, 0, 340, 70)
            .drive (fuzz, 15, 6700).reverb (20, 40, 0.6f, 50).level (-23).macro (1, "Index").mod (macro1, "osc2WarpAmount", 35).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 40).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 50);
        add ("Sterile Pan", "Perc").osc (1, metal, 66.67f).pitch (1, 5).warp (1, wCrush, 60).noise (40, 3).env (2, 0, 70, 0, 60, 88)
            .mod (env2, "osc1Pitch", 10.417f).mod (env2, "filter1Cutoff", 15).filter (1, svfHigh, 2000, 35).amp (0, 280, 0, 170, 85)
            .drive (hard, 15, 2600).reverb (10, 30, 0.6f, 70).level (4).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40).macro (4, "Steps")
            .mod (macro4, "osc1WarpAmount", 30);
        add ("Sullen Chain", "Perc").set ("osc1On", 0).noise (80, 2, 40).filter (1, comb, 260, 60).filter (2, svfHigh, 3300, 15)
            .lfo (1, randomShape, 16).mod (lfo1, "filter1Cutoff", 6).amp (5, 650, 0, 450).reverb (15, 50, 1, 40).level (5).macro (1, "Ring")
            .mod (macro1, "filter1Resonance", 25).macro (2, "Tone").mod (macro2, "filter2Cutoff", 20).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 40).macro (4, "Rattle").mod (macro4, "lfo1Rate", 25);
        add ("Tin Chain", "Perc").set ("osc1On", 0).noise (90, 2).filter (1, comb, 450, 65).filter (2, svfBand, 1700, 35).lfo (1, randomShape, 12)
            .mod (lfo1, "filter1Cutoff", 8).amp (15, 450, 0, 250).drive (tube, 30, 4200).reverb (10, 30, 1, 60).level (-5).macro (1, "Ring")
            .mod (macro1, "filter1Resonance", 25).macro (2, "Tone").mod (macro2, "filter2Cutoff", 20).macro (3, "Rattle")
            .mod (macro3, "lfo1Rate", 25).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Titanium Conduit", "Perc").osc (1, pulse, 85, 1).noise (1, 0, 60).env (2, 0, 22, 0, 15, 0).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 60).filter (1, comb, 523.3f, 96, 0, 100).filter (2, ladder12, 1600, 20).amp (0, 700, 0, 450, 60)
            .drive (tube, 45, 2800).reverb (15, 55, 1.2f, 60).level (-19).macro (1, "Damp").mod (macro1, "filter1Resonance", -15).macro (2, "Tone")
            .mod (macro2, "filter2Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 40).macro (4, "Tune")
            .mod (macro4, "filter1Cutoff", 12);
        add ("Titanium Twang", "Perc").osc (1, analog, sawPos, 1).noise (1).env (2, 0, 28, 0, 15, 40).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 90).filter (1, comb, 220, 94).env (3, 0, 480, 0, 300, 70).mod (env3, "filter1Cutoff", 16)
            .filter (2, svfHigh, 840, 15).amp (0, 1050, 0, 550, 65).reverb (20, 55, 1, 70).level (-4).macro (1, "Damp")
            .mod (macro1, "filter1Resonance", -15).macro (2, "Tune").mod (macro2, "filter1Cutoff", 12).macro (3, "Tone")
            .mod (macro3, "filter2Cutoff", 22).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Vicious Tick", "Perc").osc (1, analog, sinePos).pitch (1, 12).osc (2, analog, sinePos, 65).pitch (2, 17, 30).noise (1, 0, 60)
            .env (2, 0, 3, 0, 8, 90).mod (env2, "noiseLevel", 50).mod (env2, "osc1Fine", 20).filter (1, svfBand, 2100, 20, 0, 50)
            .amp (0, 220, 0, 140, 70).drive (hard, 30, 5800).reverb (15, 55, 0.6f, 60).level (-12).macro (1, "Tune")
            .mod (macro1, "osc1Pitch", 14.583f).macro (2, "Click").mod (macro2, "noiseLevel", 35).macro (3, "Cutoff")
            .mod (macro3, "filter1Cutoff", 20).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Virtual Husk", "Perc").set ("osc1On", 0).noise (70, 0, 60).filter (1, ladderHigh, 3500, 25).env (2, 5, 130, 0, 60, 60)
            .mod (env2, "filter1Cutoff", 6).amp (4, 100, 0, 160).reverb (12, 30, 1.6f, 70).level (4).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance").mod (macro2, "filter1Resonance", 35).drive (crush, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Wan Spark", "Perc").osc (1, fm, 0).pitch (1, -12).env (2, 0, 110, 0, 100, 92).mod (env2, "osc1Pitch", 39.583f).noise (1)
            .env (3, 0, 25, 0, 30, 90).mod (env3, "noiseLevel", 30).filter (1, svfBand, 7800, 25, 20).amp (0, 520, 0, 400, 55)
            .drive (hard, 45, 8700).reverb (15, 55, 1.4f, 50).level (1).macro (1, "Sweep").mod (macro1, "osc1Pitch", 25).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).delay (sixteenth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Wasteland Flash", "Perc").osc (1, analog, trianglePos).pitch (1, 7).env (2, 0, 120, 0, 100, 76).mod (env2, "osc1Pitch", 75)
            .noise (1, 3, 60).env (3, 0, 10, 0, 30, 90).mod (env3, "noiseLevel", 50).filter (1, svfLow, 8800, 35, 20).amp (0, 200, 0, 140, 70)
            .reverb (20, 35, 0.8f, 40).level (-12).macro (1, "Sweep").mod (macro1, "osc1Pitch", 25).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Waxen Boing", "Perc").osc (1, analog, 60, 1).noise (1, 2, 40).env (2, 0, 6, 0, 15, 20).mod (env2, "osc1Volume", 99)
            .mod (env2, "noiseLevel", 85).filter (1, comb, 530, 94).env (3, 0, 260, 0, 300).mod (env3, "filter1Cutoff", 5)
            .filter (2, svfBand, 620, 30).amp (0, 1200, 0, 250, 70).reverb (25, 55, 0.8f, 60).level (0).macro (1, "Damp")
            .mod (macro1, "filter1Resonance", -15).macro (2, "Tune").mod (macro2, "filter1Cutoff", 12).macro (3, "Tone")
            .mod (macro3, "filter2Cutoff", 22).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Welded Can", "Perc").osc (1, metal, 66.67f).pitch (1, 0).warp (1, wCrush, 60).noise (35, 2).env (2, 0, 90, 0, 60, 88)
            .mod (env2, "osc1Pitch", 14.583f).mod (env2, "filter1Cutoff", 14).filter (1, svfBand, 3200, 50).amp (0, 250, 0, 150, 70)
            .drive (crush, 20, 11000).reverb (10, 35, 1.4f, 50).level (0).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18)
            .macro (2, "Resonance").mod (macro2, "filter1Resonance", 30).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Wordless Jolt", "Perc").osc (1, sub, 33.33f).pitch (1, 0).env (2, 0, 100, 0, 100, 74).mod (env2, "osc1Pitch", 39.583f)
            .noise (1, 3, 20).env (3, 0, 30, 0, 30, 90).mod (env3, "noiseLevel", 45).filter (1, svfBand, 6800, 20, 30).amp (0, 200, 0, 120, 65)
            .delay (sixteenthD, 45, 10, 2300).reverb (20, 55, 1.4f, 40).level (0).macro (1, "Sweep").mod (macro1, "osc1Pitch", 25)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);

        // Drum, by name.
        add ("Abrasive Cymbal", "Drum").osc (1, metal, 95).pitch (1, 19).unison (1, 8, 65, 100).osc (2, metal, 70, 55).pitch (2, 23, -30)
            .unison (2, 4, 20, 100).noise (40, 0, 40).filter (1, svfHigh, 2700, 15).env (2, 0, 180, 0, 200, 80).mod (env2, "noiseLevel", 10)
            .mod (env2, "filter1Cutoff", -3).amp (0, 1100, 0, 1000, 45).level (0).macro (1, "Tone").mod (macro1, "filter1Cutoff", -12)
            .macro (2, "Bright").mod (macro2, "osc1Position", -40).macro (3, "Wash").mod (macro3, "noiseLevel", 30).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Annealed Hat", "Drum").osc (1, metal, 60, 40).pitch (1, 19).unison (1, 6, 35, 100).noise (70, 0, 80).filter (1, svfBand, 7000, 25)
            .amp (0, 300, 0, 300, 70).reverb (8, 40, 1, 40).level (0).macro (1, "Tone").mod (macro1, "filter1Cutoff", -12).macro (2, "Metal")
            .mod (macro2, "osc1Volume", 30).macro (3, "Sizzle").mod (macro3, "filter1Resonance", 35).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Arctic Kick", "Drum").osc (1, fm, 30).pitch (1, -12).env (2, 0, 70, 0, 40, 90).mod (env2, "osc1Pitch", 64.583f)
            .mod (env2, "osc1Position", 25).filter (1, ladder24, 4200, 5, 20).amp (0, 520, 0, 120, 70).level (-9).macro (1, "Punch")
            .mod (macro1, "osc1Pitch", 25).macro (2, "Knock").mod (macro2, "osc1Position", 30).drive (tube, 0, 5000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Tone").mod (macro4, "filter1Cutoff", -25);
        add ("Banished Knock", "Drum").osc (1, fm, 30).pitch (1, -12).env (2, 0, 45, 0, 40, 84).mod (env2, "osc1Pitch", 25)
            .mod (env2, "osc1Position", 50).filter (1, svfLow, 1100, 5, 40).amp (0, 260, 0, 120, 60).drive (soft, 15, 3000).level (-14)
            .macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Knock").mod (macro2, "osc1Position", 30).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Tone").mod (macro4, "filter1Cutoff", -25);
        add ("Barred Hat", "Drum").osc (1, metal, 75, 35).pitch (1, 24).unison (1, 3, 40, 40).noise (60, 0, 60).filter (1, svfHigh, 5300, 35)
            .amp (0, 35, 0, 30, 75).mod (velocity, "filter1Cutoff", 8).level (0).macro (1, "Tone").mod (macro1, "filter1Cutoff", -12)
            .macro (2, "Metal").mod (macro2, "osc1Volume", 30).macro (3, "Hiss").mod (macro3, "noiseLevel", -45).reverb (0, 40, 0.8f, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Blunt Kick", "Drum").osc (1, sub, 0).pitch (1, -12).subOsc (30).env (2, 0, 50, 0, 60, 88).mod (env2, "osc1Pitch", 75)
            .env (3, 0, 120, 0, 100, 80).mod (env3, "osc1Position", 23).filter (1, ladder24, 1300, 25, 70).amp (0, 500, 0, 240, 60)
            .drive (tube, 50, 5200).level (-21).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Crunch")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Tone").mod (macro4, "filter1Cutoff", -30);
        add ("Brittle Snare", "Drum").set ("osc1On", 0).parallel().osc (2, analog, 20, 1).pitch (2, 12).route (2, 100).env (2, 0, 120, 0, 60, 85)
            .mod (env2, "osc2Volume", 60).mod (env2, "osc2Pitch", 10.417f).noise (1, 0, 60).env (3, 0, 180, 0, 100, 80).mod (env3, "noiseLevel", 70)
            .filter (1, ladderHigh, 3200, 35).filter (2, svfLow, 2800, 5).amp (0, 500, 0, 200, 40).level (-5).macro (1, "Wires")
            .mod (macro1, "noiseLevel", -45).macro (2, "Tone").mod (macro2, "filter1Cutoff", 18).drive (hard, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Bunker Barrel", "Drum").osc (1, analog, sinePos).pitch (1, 0).env (2, 0, 180, 0, 80, 88).mod (env2, "osc1Pitch", 10.417f)
            .noise (1, 1, -20).env (3, 0, 30, 0, 40, 85).mod (env3, "noiseLevel", 30).filter (1, ladder24, 1000, 25, 10).amp (0, 250, 0, 150, 65)
            .level (-10).macro (1, "Bend").mod (macro1, "osc1Pitch", 14.583f).macro (2, "Skin").mod (macro2, "noiseLevel", 35).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).reverb (0, 55, 1.4f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Burnished Kick", "Drum").osc (1, sub, 8).pitch (1, -12).env (2, 0, 70, 0, 40, 84).mod (env2, "osc1Pitch", 50).noise (1, 0, 40)
            .env (3, 0, 3, 0, 10, 90).mod (env3, "noiseLevel", 25).filter (1, svfLow, 7900, 5, 30).amp (0, 660, 0, 100, 55).drive (soft, 25, 4500)
            .level (-18).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Click").mod (macro2, "noiseLevel", 35).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Bite").mod (macro4, "filter1Drive", 60);
        add ("Chromium Hat", "Drum").osc (1, metal, 65, 50).pitch (1, 12).unison (1, 4, 45, 60).noise (55, 0, 60).filter (1, ladderHigh, 4800, 50)
            .amp (0, 800, 0, 120, 70).reverb (12, 40, 1.6f, 50).level (0).macro (1, "Tone").mod (macro1, "filter1Cutoff", -12).macro (2, "Metal")
            .mod (macro2, "osc1Volume", 30).macro (3, "Sizzle").mod (macro3, "filter1Resonance", 35).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Close Kick", "Drum").osc (1, analog, sinePos).pitch (1, -12).env (2, 0, 20, 0, 40, 84).mod (env2, "osc1Pitch", 50).noise (1, 0, 60)
            .env (3, 0, 3, 0, 10, 90).mod (env3, "noiseLevel", 20).filter (1, svfLow, 2100, 5, 10).amp (0, 580, 0, 80, 65).drive (soft, 30, 4000)
            .level (-19).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Click").mod (macro2, "noiseLevel", 35).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Bite").mod (macro4, "filter1Drive", 60);
        add ("Close Snare", "Drum").set ("osc1On", 0).parallel().osc (2, analog, 20, 1).pitch (2, 9).route (2, 100).env (2, 0, 70, 0, 60, 85)
            .mod (env2, "osc2Volume", 80).mod (env2, "osc2Pitch", 10.417f).noise (1, 1, -20).env (3, 0, 300, 0, 100, 80)
            .mod (env3, "noiseLevel", 85).filter (1, svfBand, 2000).filter (2, ladder12, 3800, 5).amp (0, 300, 0, 150, 40).drive (soft, 15, 5300)
            .level (-9).macro (1, "Wires").mod (macro1, "noiseLevel", -45).macro (2, "Tone").mod (macro2, "filter1Cutoff", 18).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Coastal Clap", "Drum").set ("osc1On", 0).noise (80, 0, 20).filter (1, ladderBand, 1600, 55).lfo (1, steps, 8.5f, 2, 50)
            .stepPattern (1, { 0, -70, 0, -70, 0, -70, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }).mod (lfo1, "ampLevel", 65).amp (0, 360, 0, 130)
            .reverb (20, 45, 1, 40).level (1).macro (1, "Tone").mod (macro1, "filter1Cutoff", 15).macro (2, "Spread").mod (macro2, "lfo1Rate", -12)
            .drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Cold Tick", "Drum").osc (1, metal, 70, 50).pitch (1, 24).unison (1, 3, 35, 30).noise (85, 0, 40).filter (1, ladderHigh, 10000, 30)
            .amp (0, 45, 0, 50, 65).mod (velocity, "filter1Cutoff", 6).level (0).macro (1, "Tone").mod (macro1, "filter1Cutoff", -12)
            .macro (2, "Metal").mod (macro2, "osc1Volume", 30).macro (3, "Hiss").mod (macro3, "noiseLevel", -45).reverb (0, 40, 0.8f, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Compact Wash", "Drum").osc (1, metal, 100, 55).pitch (1, 12).unison (1, 6, 65, 70).noise (70, 0, 80).filter (1, ladderHigh, 8600, 40)
            .amp (0, 500, 0, 300, 40).level (-2).macro (1, "Tone").mod (macro1, "filter1Cutoff", -12).macro (2, "Metal")
            .mod (macro2, "osc1Volume", 30).macro (3, "Sizzle").mod (macro3, "filter1Resonance", 35).reverb (0, 40, 0.8f, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Cursed Snare", "Drum").set ("osc1On", 0).parallel().osc (2, analog, sinePos, 1).pitch (2, 7).route (2, 100).env (2, 0, 150, 0, 60, 85)
            .mod (env2, "osc2Volume", 65).mod (env2, "osc2Pitch", 6.25f).noise (1, 1, 20).env (3, 0, 100, 0, 100, 85).mod (env3, "noiseLevel", 80)
            .filter (1, svfBand, 3100, 15).filter (2, svfLow, 1500, 5).amp (0, 400, 0, 100, 40).drive (soft, 35, 3300).level (-13)
            .macro (1, "Wires").mod (macro1, "noiseLevel", -45).macro (2, "Tone").mod (macro2, "filter1Cutoff", 18).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Curt Kick", "Drum").osc (1, analog, sinePos).pitch (1, -17).env (2, 0, 50, 0, 40, 84).mod (env2, "osc1Pitch", 25).noise (1, 0, 40)
            .env (3, 0, 4, 0, 10, 90).mod (env3, "noiseLevel", 45).filter (1, svfLow, 2500, 5, 20).amp (0, 240, 0, 100, 60).drive (soft, 30, 2800)
            .level (-19).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Click").mod (macro2, "noiseLevel", 35).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Bite").mod (macro4, "filter1Drive", 60);
        add ("Curt Stomp", "Drum").osc (1, analog, 15).pitch (1, -14).env (2, 0, 20, 0, 40, 94).mod (env2, "osc1Pitch", 75).noise (1, 0, -20)
            .env (3, 0, 11, 0, 10, 90).mod (env3, "noiseLevel", 50).filter (1, ladder24, 1500, 5, 10).amp (0, 660, 0, 180, 65)
            .drive (hard, 35, 6900).level (-20).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Click").mod (macro2, "noiseLevel", 35)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Bite").mod (macro4, "filter1Drive", 60);
        add ("Diamond Ride", "Drum").osc (1, metal, 95).pitch (1, 24).unison (1, 6, 70, 100).osc (2, metal, 40, 75).pitch (2, 13, -20)
            .unison (2, 2, 25, 100).noise (65, 0, 80).filter (1, ladderHigh, 4700, 20).env (2, 0, 80, 0, 200, 80).mod (env2, "noiseLevel", 30)
            .mod (env2, "filter1Cutoff", -7).amp (0, 1100, 0, 1200, 45).level (-3).macro (1, "Tone").mod (macro1, "filter1Cutoff", -12)
            .macro (2, "Bright").mod (macro2, "osc1Position", -40).macro (3, "Wash").mod (macro3, "noiseLevel", 30).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Doomed Cymbal", "Drum").osc (1, metal, 75).pitch (1, 24).unison (1, 6, 75, 100).osc (2, metal, 80, 60).pitch (2, 18)
            .unison (2, 2, 30, 100).noise (40, 0, 40).filter (1, svfHigh, 3700, 25).env (2, 0, 200, 0, 200, 80).mod (env2, "noiseLevel", 20)
            .mod (env2, "filter1Cutoff", -5).amp (0, 2800, 0, 1200, 70).reverb (10, 70, 5, 45).level (-2).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -12).macro (2, "Bright").mod (macro2, "osc1Position", -40).macro (3, "Wash")
            .mod (macro3, "noiseLevel", 30).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Eastern Sizzle", "Drum").osc (1, metal, 60, 45).pitch (1, 19).unison (1, 6, 60, 90).noise (60, 3, 40).filter (1, svfBand, 4500, 30)
            .amp (0, 700, 0, 280).level (4).macro (1, "Tone").mod (macro1, "filter1Cutoff", -12).macro (2, "Metal").mod (macro2, "osc1Volume", 30)
            .macro (3, "Sizzle").mod (macro3, "filter1Resonance", 35).reverb (0, 40, 0.8f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Encoded Gunshot", "Drum").set ("osc1On", 0).parallel().osc (2, analog, 20, 1).pitch (2, 5).route (2, 100).env (2, 0, 70, 0, 60, 85)
            .mod (env2, "osc2Volume", 75).mod (env2, "osc2Pitch", 14.583f).noise (1, 2, 40).env (3, 0, 260, 0, 100, 70).mod (env3, "noiseLevel", 95)
            .filter (1, comb, 370, 75).filter (2, svfLow, 920).amp (0, 650, 0, 300, 40).drive (crush, 25, 5000).reverb (35, 70, 0.8f, 70).level (-6)
            .macro (1, "Wires").mod (macro1, "noiseLevel", -45).macro (2, "Ring").mod (macro2, "filter1Resonance", 15).macro (3, "Crunch")
            .mod (macro3, "fxDriveAmount", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Encoded Lash", "Drum").set ("osc1On", 0).parallel().osc (2, analog, 20, 1).pitch (2, 3).route (2, 100).env (2, 0, 190, 0, 60, 85)
            .mod (env2, "osc2Volume", 85).mod (env2, "osc2Pitch", 25).noise (1, 0, 20).env (3, 0, 420, 0, 100, 70).mod (env3, "noiseLevel", 90)
            .filter (1, comb, 340, 75).filter (2, ladder12, 1800).amp (0, 750, 0, 250, 40).drive (crush, 35, 4200).reverb (25, 60, 0.6f, 60)
            .level (-9).macro (1, "Wires").mod (macro1, "noiseLevel", -45).macro (2, "Ring").mod (macro2, "filter1Resonance", 15)
            .macro (3, "Crunch").mod (macro3, "fxDriveAmount", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Encoded Sizzle", "Drum").osc (1, metal, 80, 55).pitch (1, 12).unison (1, 4, 40, 60).noise (80, 3, 60).filter (1, svfHigh, 7900, 20)
            .amp (0, 450, 0, 120).level (1).macro (1, "Tone").mod (macro1, "filter1Cutoff", -12).macro (2, "Metal").mod (macro2, "osc1Volume", 30)
            .macro (3, "Sizzle").mod (macro3, "filter1Resonance", 35).reverb (0, 40, 0.8f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Exposed Punt", "Drum").osc (1, fm, 25).pitch (1, -17).env (2, 0, 45, 0, 40, 82).mod (env2, "osc1Pitch", 64.583f)
            .mod (env2, "osc1Position", 35).filter (1, svfLow, 3300, 5, 10).amp (0, 580, 0, 160, 65).drive (soft, 40, 8800).level (-21)
            .macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Knock").mod (macro2, "osc1Position", 30).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Tone").mod (macro4, "filter1Cutoff", -25);
        add ("Fevered Clap", "Drum").set ("osc1On", 0).noise (85, 1, 20).filter (1, ladderBand, 2400, 40).filter (2, svfHigh, 990)
            .lfo (1, steps, 9.5f, 2, 40).stepPattern (1, { 0, -70, 0, -70, 0, -70, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }).mod (lfo1, "ampLevel", 75)
            .amp (0, 540, 0, 200, 40).reverb (13, 60, 0.6f, 40).level (3).macro (1, "Tone").mod (macro1, "filter1Cutoff", 15).macro (2, "Spread")
            .mod (macro2, "lfo1Rate", -12).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Fluorescent Crack", "Drum").set ("osc1On", 0).parallel().osc (2, analog, 20, 1).pitch (2, 3).route (2, 100).env (2, 0, 60, 0, 60, 85)
            .mod (env2, "osc2Volume", 65).mod (env2, "osc2Pitch", 6.25f).noise (1, 1, 60).env (3, 0, 190, 0, 100, 80).mod (env3, "noiseLevel", 65)
            .filter (1, svfHigh, 950, 35).filter (2, svfLow, 3600, 5).amp (0, 350, 0, 250, 40).level (-3).macro (1, "Wires")
            .mod (macro1, "noiseLevel", -45).macro (2, "Tone").mod (macro2, "filter1Cutoff", 18).drive (hard, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Fried Clap", "Drum").set ("osc1On", 0).noise (80, 1, 20).filter (1, svfBand, 1800, 30).lfo (1, steps, 12.5f, 2, 40)
            .stepPattern (1, { 0, -85, 0, -85, 0, -85, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }).mod (lfo1, "ampLevel", 70).amp (0, 320, 0, 170, 60)
            .reverb (14, 75, 1.6f, 60).level (3).macro (1, "Tone").mod (macro1, "filter1Cutoff", 15).macro (2, "Spread")
            .mod (macro2, "lfo1Rate", -12).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Fugitive Clap", "Drum").set ("osc1On", 0).noise (100, 1, -20).filter (1, svfBand, 1100, 35).lfo (1, steps, 14, 2, 75)
            .stepPattern (1, { 0, -100, 0, -100, -100, 0, -100, 0, -100, 0, 0, 0, 0, 0, 0, 0 }).mod (lfo1, "ampLevel", 80).amp (0, 260, 0, 330)
            .reverb (35, 35, 2, 60).level (1).macro (1, "Tone").mod (macro1, "filter1Cutoff", 15).macro (2, "Spread").mod (macro2, "lfo1Rate", -12)
            .drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Galvanised Hiss", "Drum").osc (1, metal, 90, 50).pitch (1, 12).unison (1, 4, 45).noise (75, 3, 20).filter (1, svfBand, 8200, 20)
            .amp (0, 110, 0, 30, 70).mod (velocity, "filter1Cutoff", 6).level (2).macro (1, "Tone").mod (macro1, "filter1Cutoff", -12)
            .macro (2, "Metal").mod (macro2, "osc1Volume", 30).macro (3, "Hiss").mod (macro3, "noiseLevel", -45).reverb (0, 40, 0.8f, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Gaunt Ride", "Drum").osc (1, metal, 100).pitch (1, 12).unison (1, 4, 30, 100).osc (2, metal, 80, 65).pitch (2, 23, -30)
            .unison (2, 4, 20, 100).noise (55, 0, 20).filter (1, svfHigh, 2800, 20).env (2, 0, 280, 0, 200, 80).mod (env2, "noiseLevel", 25)
            .mod (env2, "filter1Cutoff", -10).amp (0, 2100, 0, 1400, 70).reverb (15, 90, 5, 70).level (-6).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -12).macro (2, "Bright").mod (macro2, "osc1Position", -40).macro (3, "Wash")
            .mod (macro3, "noiseLevel", 30).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Glacial Thump", "Drum").osc (1, sub, 15).pitch (1, -14).env (2, 0, 30, 0, 40, 84).mod (env2, "osc1Pitch", 35.417f).noise (1)
            .env (3, 0, 5, 0, 10, 90).mod (env3, "noiseLevel", 35).filter (1, svfLow, 4700, 5, 10).amp (0, 280, 0, 100, 65).drive (soft, 40, 8000)
            .level (-22).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Click").mod (macro2, "noiseLevel", 35).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Bite").mod (macro4, "filter1Drive", 60);
        add ("Gnarled Stomp", "Drum").osc (1, sub, 20).pitch (1, -17).subOsc (30, 0, 1).env (2, 0, 40, 0, 60, 90).mod (env2, "osc1Pitch", 64.583f)
            .env (3, 0, 200, 0, 100, 80).mod (env3, "osc1Position", 19).filter (1, ladder24, 980, 20, 40).amp (0, 1100, 0, 160, 65)
            .drive (foldDrive, 50, 3700).level (-25).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Crunch")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Tone").mod (macro4, "filter1Cutoff", -30);
        add ("Grid Lash", "Drum").set ("osc1On", 0).parallel().osc (2, analog, trianglePos, 1).pitch (2, 0).route (2, 100)
            .env (2, 0, 140, 0, 60, 85).mod (env2, "osc2Volume", 80).mod (env2, "osc2Pitch", 25).noise (1, 3, 40).env (3, 0, 200, 0, 100, 60)
            .mod (env3, "noiseLevel", 70).filter (1, comb, 160, 45).filter (2, svfLow, 3800).amp (0, 800, 0, 350, 40).drive (hard, 35, 7400)
            .reverb (20, 60, 0.8f, 50).level (-18).macro (1, "Wires").mod (macro1, "noiseLevel", -45).macro (2, "Ring")
            .mod (macro2, "filter1Resonance", 15).macro (3, "Crunch").mod (macro3, "fxDriveAmount", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Halflit Tom", "Drum").osc (1, analog, 10).pitch (1, 4).env (2, 0, 110, 0, 80, 70).mod (env2, "osc1Pitch", 6.25f).noise (1, 1, 20)
            .env (3, 0, 65, 0, 40, 85).mod (env3, "noiseLevel", 50).filter (1, ladder24, 1600, 10, 40).amp (0, 550, 0, 150, 75)
            .drive (hard, 20, 4600).reverb (13, 30, 1.2f, 50).level (-16).macro (1, "Bend").mod (macro1, "osc1Pitch", 14.583f).macro (2, "Skin")
            .mod (macro2, "noiseLevel", 35).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Hollowed Smack", "Drum").set ("osc1On", 0).noise (85, 0, 20).filter (1, ladderBand, 1200, 55).filter (2, svfHigh, 550)
            .lfo (1, steps, 15, 2, 75).stepPattern (1, { 0, -85, -85, 0, -85, 0, -85, 0, 0, 0, 0, 0, 0, 0, 0, 0 }).mod (lfo1, "ampLevel", 60)
            .amp (0, 180, 0, 170, 60).reverb (16, 35, 1.8f, 40).level (0).macro (1, "Tone").mod (macro1, "filter1Cutoff", 15).macro (2, "Spread")
            .mod (macro2, "lfo1Rate", -12).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Icy Kick", "Drum").osc (1, fm, 30).pitch (1, -12).subOsc (50).env (2, 0, 110, 0, 60, 82).mod (env2, "osc1Pitch", 64.583f)
            .env (3, 0, 60, 0, 100, 80).mod (env3, "osc1Position", 11).filter (1, ladder24, 4800, 5, 60).amp (0, 350, 0, 280, 60)
            .drive (fuzz, 45, 2800).reverb (20, 50, 1.2f, 60).level (-32).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Crunch")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Tone").mod (macro4, "filter1Cutoff", -30);
        add ("Inland Hiss", "Drum").osc (1, metal, 90, 50).pitch (1, 24).unison (1, 2, 30, 70).noise (85, 0, 40).filter (1, ladderHigh, 6100, 30)
            .amp (0, 30, 0, 50, 65).mod (velocity, "filter1Cutoff", 6).drive (crush, 35, 8900).level (0).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -12).macro (2, "Metal").mod (macro2, "osc1Volume", 30).macro (3, "Hiss").mod (macro3, "noiseLevel", -45)
            .reverb (0, 40, 0.8f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Kerbside Smack", "Drum").set ("osc1On", 0).noise (75).filter (1, ladderBand, 980, 25).filter (2, svfHigh, 1000)
            .lfo (1, steps, 12.5f, 2, 55).stepPattern (1, { 0, -70, -70, 0, -70, 0, -70, 0, 0, 0, 0, 0, 0, 0, 0, 0 }).mod (lfo1, "ampLevel", 70)
            .amp (0, 220, 0, 130).drive (tube, 30, 5200).reverb (21, 50, 1.8f, 70).level (0).macro (1, "Tone").mod (macro1, "filter1Cutoff", 15)
            .macro (2, "Spread").mod (macro2, "lfo1Rate", -12).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Knotted Hoof", "Drum").osc (1, sub, 0).pitch (1, -12).subOsc (40, 0, 1).env (2, 0, 80, 0, 60, 86).mod (env2, "osc1Pitch", 64.583f)
            .env (3, 0, 60, 0, 100, 80).mod (env3, "osc1Position", 20).filter (1, ladder24, 4000, 15, 60).amp (0, 700, 0, 80).drive (hard, 35, 2800)
            .reverb (8, 40, 0.9f, 60).level (-20).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Crunch")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Tone").mod (macro4, "filter1Cutoff", -30);
        add ("Lean Tub", "Drum").osc (1, analog, 10).pitch (1, 0).env (2, 0, 180, 0, 80, 70).mod (env2, "osc1Pitch", 14.583f).noise (1, 0, -40)
            .env (3, 0, 40, 0, 40, 85).mod (env3, "noiseLevel", 50).filter (1, ladder24, 1300).amp (0, 450, 0, 300, 70).drive (tube, 35, 11000)
            .level (-17).macro (1, "Bend").mod (macro1, "osc1Pitch", 14.583f).macro (2, "Skin").mod (macro2, "noiseLevel", 35).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).reverb (0, 55, 1.4f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Lone Crack", "Drum").set ("osc1On", 0).parallel().osc (2, analog, 20, 1).pitch (2, 7).route (2, 100).env (2, 0, 80, 0, 60, 85)
            .mod (env2, "osc2Volume", 75).mod (env2, "osc2Pitch", 10.417f).noise (1, 2).env (3, 0, 180, 0, 100, 85).mod (env3, "noiseLevel", 90)
            .filter (1, comb, 830, 80).filter (2, svfBand, 810).amp (0, 550, 0, 350, 40).drive (fuzz, 35, 3300).reverb (25, 40, 1.4f, 60)
            .level (-22).macro (1, "Wires").mod (macro1, "noiseLevel", -45).macro (2, "Ring").mod (macro2, "filter1Resonance", 15)
            .macro (3, "Crunch").mod (macro3, "fxDriveAmount", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Magnesium Hat", "Drum").osc (1, metal, 95, 45).pitch (1, 12).unison (1, 2, 60, 30).noise (75, 0, 60).filter (1, svfHigh, 8600, 35)
            .amp (0, 65, 0, 75, 75).mod (velocity, "filter1Cutoff", 5).level (-4).macro (1, "Tone").mod (macro1, "filter1Cutoff", -12)
            .macro (2, "Metal").mod (macro2, "osc1Volume", 30).macro (3, "Hiss").mod (macro3, "noiseLevel", -45).reverb (0, 40, 0.8f, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Magnesium Kick", "Drum").osc (1, sub, 0).pitch (1, -17).env (2, 0, 20, 0, 40, 82).mod (env2, "osc1Pitch", 50).noise (1, 0, -20)
            .env (3, 0, 8, 0, 10, 90).mod (env3, "noiseLevel", 35).filter (1, ladder24, 7200, 5, 40).amp (0, 680, 0, 140, 60).drive (hard, 35, 3500)
            .level (-20).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Click").mod (macro2, "noiseLevel", 35).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Bite").mod (macro4, "filter1Drive", 60);
        add ("Mirror Gong", "Drum").osc (1, metal, 70).pitch (1, 12).unison (1, 4, 70, 100).osc (2, metal, 70, 75).pitch (2, 23, 20)
            .unison (2, 2, 55, 100).noise (60, 0, 20).filter (1, ladderHigh, 5400, 15).env (2, 0, 120, 0, 200, 80).mod (env2, "noiseLevel", 20)
            .mod (env2, "filter1Cutoff", -9).amp (0, 3400, 0, 1100, 45).level (-2).macro (1, "Tone").mod (macro1, "filter1Cutoff", -12)
            .macro (2, "Bright").mod (macro2, "osc1Position", -40).macro (3, "Wash").mod (macro3, "noiseLevel", 30).drive (hard, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Mirror Snare", "Drum").set ("osc1On", 0).parallel().osc (2, analog, trianglePos, 1).pitch (2, 7).route (2, 100)
            .env (2, 0, 100, 0, 60, 85).mod (env2, "osc2Volume", 60).mod (env2, "osc2Pitch", 14.583f).noise (1, 0, 60).env (3, 0, 120, 0, 100, 60)
            .mod (env3, "noiseLevel", 80).filter (1, svfHigh, 2200).filter (2, ladder12, 3600, 5).amp (0, 450, 0, 100, 40).drive (tube, 20, 2500)
            .level (-6).macro (1, "Wires").mod (macro1, "noiseLevel", -45).macro (2, "Tone").mod (macro2, "filter1Cutoff", 18).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Mortal Crack", "Drum").set ("osc1On", 0).parallel().osc (2, analog, sinePos, 1).pitch (2, 9).route (2, 100).env (2, 0, 80, 0, 60, 85)
            .mod (env2, "osc2Volume", 85).mod (env2, "osc2Pitch", 10.417f).noise (1, 1, 40).env (3, 0, 120, 0, 100, 65).mod (env3, "noiseLevel", 90)
            .filter (1, svfBand, 1400, 15).filter (2, svfLow, 2700, 5).amp (0, 250, 0, 150, 40).level (-7).macro (1, "Wires")
            .mod (macro1, "noiseLevel", -45).macro (2, "Tone").mod (macro2, "filter1Cutoff", 18).drive (hard, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Nervous Smack", "Drum").set ("osc1On", 0).noise (100, 0, 40).filter (1, svfBand, 2100, 45).filter (2, svfHigh, 670)
            .lfo (1, steps, 9, 2, 50).stepPattern (1, { 0, -85, 0, -85, 0, -85, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }).mod (lfo1, "ampLevel", 75)
            .amp (0, 180, 0, 260, 60).drive (tube, 35, 2700).reverb (31, 60, 1.6f, 50).level (-6).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", 15).macro (2, "Spread").mod (macro2, "lfo1Rate", -12).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Northern Kick", "Drum").osc (1, analog, 20).pitch (1, -19).subOsc (60, 0, 1).env (2, 0, 30, 0, 60, 80)
            .mod (env2, "osc1Pitch", 64.583f).env (3, 0, 120, 0, 100, 80).mod (env3, "osc1Position", 17).filter (1, ladder24, 1400, 30, 30)
            .amp (0, 900, 0, 300, 65).drive (tube, 60, 6400).level (-26).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Crunch")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Tone").mod (macro4, "filter1Cutoff", -30);
        add ("Numb Barrel", "Drum").osc (1, analog, sinePos).pitch (1, 0).env (2, 0, 120, 0, 80, 70).mod (env2, "osc1Pitch", 14.583f).noise (1, 1)
            .env (3, 0, 35, 0, 40, 85).mod (env3, "noiseLevel", 35).filter (1, ladder24, 970, 25, 40).amp (0, 350, 0, 300, 70)
            .reverb (25, 50, 0.8f, 70).level (-10).macro (1, "Bend").mod (macro1, "osc1Pitch", 14.583f).macro (2, "Skin")
            .mod (macro2, "noiseLevel", 35).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Overloaded Kick", "Drum").osc (1, analog, 15).pitch (1, -14).env (2, 0, 25, 0, 40, 86).mod (env2, "osc1Pitch", 50).noise (1, 3)
            .env (3, 0, 5, 0, 10, 90).mod (env3, "noiseLevel", 20).filter (1, svfLow, 4500, 5, 10).amp (0, 340, 0, 160).drive (hard, 25, 5000)
            .level (-18).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Click").mod (macro2, "noiseLevel", 35).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Bite").mod (macro4, "filter1Drive", 60);
        add ("Overloaded Tom", "Drum").osc (1, analog, 10).pitch (1, 7).env (2, 0, 50, 0, 80, 88).mod (env2, "osc1Pitch", 25).noise (1, 0, 20)
            .env (3, 0, 15, 0, 40, 85).mod (env3, "noiseLevel", 25).filter (1, svfLow, 2600, 5, 10).amp (0, 300, 0, 200).drive (hard, 40, 4200)
            .level (-23).macro (1, "Bend").mod (macro1, "osc1Pitch", 14.583f).macro (2, "Skin").mod (macro2, "noiseLevel", 35).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).reverb (0, 55, 1.4f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Overtime Snare", "Drum").set ("osc1On", 0).parallel().osc (2, analog, 20, 1).pitch (2, 12).route (2, 100).env (2, 0, 90, 0, 60, 85)
            .mod (env2, "osc2Volume", 65).mod (env2, "osc2Pitch", 6.25f).noise (1).env (3, 0, 110, 0, 100, 70).mod (env3, "noiseLevel", 90)
            .filter (1, ladderHigh, 3700, 40).filter (2, ladder12, 5800, 5).amp (0, 450, 0, 150, 40).drive (soft, 35, 8100).reverb (8, 30, 1.4f, 40)
            .level (-12).macro (1, "Wires").mod (macro1, "noiseLevel", -45).macro (2, "Tone").mod (macro2, "filter1Cutoff", 18).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Pacing Clap", "Drum").set ("osc1On", 0).noise (90).filter (1, ladderBand, 960, 35).lfo (1, steps, 8.5f, 2, 55)
            .stepPattern (1, { 0, -85, 0, -85, 0, -85, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }).mod (lfo1, "ampLevel", 70).amp (0, 300, 0, 290, 60)
            .drive (hard, 30, 2500).reverb (29, 45, 1.4f, 70).level (-5).macro (1, "Tone").mod (macro1, "filter1Cutoff", 15).macro (2, "Spread")
            .mod (macro2, "lfo1Rate", -12).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Packed Hat", "Drum").osc (1, metal, 65, 55).pitch (1, 24).unison (1, 3, 30).noise (65, 0, 40).filter (1, ladderHigh, 8600, 30)
            .amp (0, 95, 0, 30, 65).mod (velocity, "filter1Cutoff", 4).level (1).macro (1, "Tone").mod (macro1, "filter1Cutoff", -12)
            .macro (2, "Metal").mod (macro2, "osc1Volume", 30).macro (3, "Hiss").mod (macro3, "noiseLevel", -45).reverb (0, 40, 0.8f, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Pewter Hat", "Drum").osc (1, metal, 75, 50).pitch (1, 19).unison (1, 2, 60, 60).noise (65, 0, 80).filter (1, svfBand, 6400, 30)
            .amp (0, 40, 0, 55, 65).mod (velocity, "filter1Cutoff", 5).drive (hard, 25, 5300).level (-1).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -12).macro (2, "Metal").mod (macro2, "osc1Volume", 30).macro (3, "Hiss").mod (macro3, "noiseLevel", -45)
            .reverb (0, 40, 0.8f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Phosphor Hat", "Drum").osc (1, metal, 65, 60).pitch (1, 24).unison (1, 4, 50, 100).noise (55, 3, 60).filter (1, ladderHigh, 7800, 20)
            .amp (0, 250, 0, 180).reverb (20, 45, 1.4f, 40).level (6).macro (1, "Tone").mod (macro1, "filter1Cutoff", -12).macro (2, "Metal")
            .mod (macro2, "osc1Volume", 30).macro (3, "Sizzle").mod (macro3, "filter1Resonance", 35).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Phosphor Snap", "Drum").set ("osc1On", 0).parallel().osc (2, analog, sinePos, 1).pitch (2, 5).route (2, 100)
            .env (2, 0, 100, 0, 60, 85).mod (env2, "osc2Volume", 80).mod (env2, "osc2Pitch", 25).noise (1, 0, 60).env (3, 0, 260, 0, 100, 60)
            .mod (env3, "noiseLevel", 90).filter (1, svfHigh, 3000, 40).filter (2, ladder12, 3000, 5).amp (0, 350, 0, 150, 40).level (-10)
            .macro (1, "Wires").mod (macro1, "noiseLevel", -45).macro (2, "Tone").mod (macro2, "filter1Cutoff", 18).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Quarantine Hat", "Drum").osc (1, metal, 60, 55).pitch (1, 24).unison (1, 3, 55, 70).noise (80, 0, 60).filter (1, svfHigh, 5100, 20)
            .amp (0, 100, 0, 70, 60).mod (velocity, "filter1Cutoff", 7).level (-5).macro (1, "Tone").mod (macro1, "filter1Cutoff", -12)
            .macro (2, "Metal").mod (macro2, "osc1Volume", 30).macro (3, "Hiss").mod (macro3, "noiseLevel", -45).reverb (0, 40, 0.8f, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Reckless Gong", "Drum").osc (1, metal, 80).pitch (1, 24).unison (1, 8, 30, 100).osc (2, metal, 40, 60).pitch (2, 7, -30)
            .unison (2, 2, 20, 100).noise (50, 0, 80).filter (1, svfHigh, 2900, 5).env (2, 0, 260, 0, 200, 80).mod (env2, "noiseLevel", 15)
            .mod (env2, "filter1Cutoff", -7).amp (0, 2800, 0, 900).reverb (20, 70, 9, 30).level (-4).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -12).macro (2, "Bright").mod (macro2, "osc1Position", -40).macro (3, "Wash")
            .mod (macro3, "noiseLevel", 30).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Reckless Kick", "Drum").osc (1, sub, 0).pitch (1, -17).subOsc (60, 0, 1).env (2, 0, 100, 0, 60, 80).mod (env2, "osc1Pitch", 75)
            .env (3, 0, 140, 0, 100, 80).mod (env3, "osc1Position", 25).filter (1, ladder24, 1700, 25, 60).amp (0, 900, 0, 200, 70)
            .drive (tube, 35, 3600).reverb (16, 60, 1.6f, 75).level (-21).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Crunch")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Tone").mod (macro4, "filter1Cutoff", -30);
        add ("Roadside Kick", "Drum").osc (1, sub, 8).pitch (1, -12).env (2, 0, 75, 0, 40, 88).mod (env2, "osc1Pitch", 25).noise (1, 0, 40)
            .env (3, 0, 7, 0, 10, 90).mod (env3, "noiseLevel", 30).filter (1, svfLow, 3300, 5, 10).amp (0, 220, 0, 60, 55).drive (soft, 30, 8800)
            .level (-19).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Click").mod (macro2, "noiseLevel", 35).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Bite").mod (macro4, "filter1Drive", 60);
        add ("Ruined Stamp", "Drum").osc (1, fm, 20).pitch (1, -17).env (2, 0, 35, 0, 40, 82).mod (env2, "osc1Pitch", 25)
            .mod (env2, "osc1Position", 45).filter (1, svfLow, 4500, 5, 30).amp (0, 320, 0, 80, 70).drive (hard, 45, 4300).level (-24)
            .macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Knock").mod (macro2, "osc1Position", 30).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Tone").mod (macro4, "filter1Cutoff", -25);
        add ("Rusted Snare", "Drum").set ("osc1On", 0).parallel().osc (2, analog, sinePos, 1).pitch (2, 9).route (2, 100).env (2, 0, 50, 0, 60, 85)
            .mod (env2, "osc2Volume", 85).mod (env2, "osc2Pitch", 14.583f).noise (1, 1, -20).env (3, 0, 300, 0, 100, 60)
            .mod (env3, "noiseLevel", 65).filter (1, svfBand, 4500, 25).filter (2, ladder12, 2500, 5).amp (0, 450, 0, 150, 40).level (-3)
            .macro (1, "Wires").mod (macro1, "noiseLevel", -45).macro (2, "Tone").mod (macro2, "filter1Cutoff", 18).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Salted Thump", "Drum").osc (1, analog, sinePos).pitch (1, -17).env (2, 0, 70, 0, 40, 90).mod (env2, "osc1Pitch", 75).noise (1, 0, 20)
            .env (3, 0, 6, 0, 10, 90).mod (env3, "noiseLevel", 40).filter (1, svfLow, 3500, 5).amp (0, 580, 0, 100, 70).drive (hard, 30, 9000)
            .level (-21).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Click").mod (macro2, "noiseLevel", 35).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Bite").mod (macro4, "filter1Drive", 60);
        add ("Savage Tom", "Drum").osc (1, analog, 10).pitch (1, 7).env (2, 0, 170, 0, 80, 88).mod (env2, "osc1Pitch", 25).noise (1, 0, -40)
            .env (3, 0, 50, 0, 40, 85).mod (env3, "noiseLevel", 35).filter (1, svfLow, 2400, 5).amp (0, 500, 0, 100, 55).drive (hard, 35, 7000)
            .reverb (11, 45, 0.6f, 70).level (-21).macro (1, "Bend").mod (macro1, "osc1Pitch", 14.583f).macro (2, "Skin")
            .mod (macro2, "noiseLevel", 35).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Scalding Kick", "Drum").osc (1, analog, 10).pitch (1, -19).subOsc (60).env (2, 0, 50, 0, 60, 82).mod (env2, "osc1Pitch", 75)
            .env (3, 0, 160, 0, 100, 80).mod (env3, "osc1Position", 27).filter (1, ladder24, 2800, 30, 40).amp (0, 1050, 0, 80, 45)
            .drive (tube, 65, 3500).level (-27).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Crunch")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Tone").mod (macro4, "filter1Cutoff", -30);
        add ("Scalding Tom", "Drum").osc (1, analog, sinePos).pitch (1, 0).env (2, 0, 50, 0, 80, 82).mod (env2, "osc1Pitch", 18.75f)
            .noise (1, 1, -20).env (3, 0, 70, 0, 40, 85).mod (env3, "noiseLevel", 55).filter (1, ladder24, 2000, 15, 30).amp (0, 600, 0, 150, 75)
            .drive (hard, 20, 2800).reverb (22, 40, 1, 60).level (-16).macro (1, "Bend").mod (macro1, "osc1Pitch", 14.583f).macro (2, "Skin")
            .mod (macro2, "noiseLevel", 35).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Scoured Kick", "Drum").osc (1, analog, sinePos).pitch (1, -12).subOsc (30, 0, 1).env (2, 0, 40, 0, 60, 88).mod (env2, "osc1Pitch", 50)
            .env (3, 0, 140, 0, 100, 80).mod (env3, "osc1Position", 18).filter (1, ladder24, 1400, 15, 30).amp (0, 500, 0, 180, 70)
            .drive (tube, 30, 2100).reverb (18, 80, 2.5f, 75).level (-16).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Crunch")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Tone").mod (macro4, "filter1Cutoff", -30);
        add ("Scrambled Shimmer", "Drum").osc (1, metal, 80, 70).pitch (1, 24).unison (1, 4, 60, 70).noise (60, 3, 60).filter (1, svfHigh, 7500, 40)
            .amp (0, 250, 0, 80, 40).reverb (12, 35, 0.8f, 60).level (4).macro (1, "Tone").mod (macro1, "filter1Cutoff", -12).macro (2, "Metal")
            .mod (macro2, "osc1Volume", 30).macro (3, "Sizzle").mod (macro3, "filter1Resonance", 35).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Seared Tub", "Drum").osc (1, analog, 10).pitch (1, -5).env (2, 0, 110, 0, 80, 82).mod (env2, "osc1Pitch", 25).noise (1, 1, -40)
            .env (3, 0, 60, 0, 40, 85).mod (env3, "noiseLevel", 55).filter (1, ladder24, 1700, 15, 10).amp (0, 750, 0, 250, 60)
            .reverb (10, 55, 1, 50).level (-10).macro (1, "Bend").mod (macro1, "osc1Pitch", 14.583f).macro (2, "Skin")
            .mod (macro2, "noiseLevel", 35).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Shredded Tub", "Drum").osc (1, analog, trianglePos).pitch (1, 4).env (2, 0, 170, 0, 80, 76).mod (env2, "osc1Pitch", 18.75f)
            .noise (1, 0, -40).env (3, 0, 15, 0, 40, 85).mod (env3, "noiseLevel", 45).filter (1, svfLow, 3600, 15, 10).amp (0, 700, 0, 300, 60)
            .level (-11).macro (1, "Bend").mod (macro1, "osc1Pitch", 14.583f).macro (2, "Skin").mod (macro2, "noiseLevel", 35).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).reverb (0, 55, 1.4f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Shuttered Knock", "Drum").osc (1, fm, 30).pitch (1, -14).env (2, 0, 20, 0, 40, 82).mod (env2, "osc1Pitch", 39.583f)
            .mod (env2, "osc1Position", 30).filter (1, ladder24, 2200, 5, 30).amp (0, 600, 0, 200, 70).drive (tube, 15, 2900).level (-13)
            .macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Knock").mod (macro2, "osc1Position", 30).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Tone").mod (macro4, "filter1Cutoff", -25);
        add ("Silent Kick", "Drum").osc (1, fm, 15).pitch (1, -19).env (2, 0, 50, 0, 40, 94).mod (env2, "osc1Pitch", 39.583f)
            .mod (env2, "osc1Position", 50).filter (1, svfLow, 1200, 5, 20).amp (0, 220, 0, 60, 70).drive (tube, 40, 7700).level (-21)
            .macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Knock").mod (macro2, "osc1Position", 30).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Tone").mod (macro4, "filter1Cutoff", -25);
        add ("Silver Rap", "Drum").set ("osc1On", 0).parallel().osc (2, analog, 20, 1).pitch (2, 9).route (2, 100).env (2, 0, 160, 0, 60, 85)
            .mod (env2, "osc2Volume", 85).mod (env2, "osc2Pitch", 25).noise (1, 1, 60).env (3, 0, 320, 0, 100, 85).mod (env3, "noiseLevel", 70)
            .filter (1, ladderHigh, 1600, 15).filter (2, ladder12, 1900, 5).amp (0, 450, 0, 100, 40).drive (soft, 30, 3100)
            .reverb (23, 30, 0.6f, 50).level (-16).macro (1, "Wires").mod (macro1, "noiseLevel", -45).macro (2, "Tone")
            .mod (macro2, "filter1Cutoff", 18).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Solid Hat", "Drum").osc (1, metal, 65, 60).pitch (1, 24).unison (1, 2, 60, 60).noise (70, 3, 20).filter (1, svfBand, 5600, 40)
            .amp (0, 50, 0, 65, 60).mod (velocity, "filter1Cutoff", 3).drive (crush, 25, 3200).level (6).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -12).macro (2, "Metal").mod (macro2, "osc1Volume", 30).macro (3, "Hiss").mod (macro3, "noiseLevel", -45)
            .reverb (0, 40, 0.8f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Splintered Snare", "Drum").set ("osc1On", 0).parallel().osc (2, analog, 20, 1).pitch (2, 5).route (2, 100).env (2, 0, 150, 0, 60, 85)
            .mod (env2, "osc2Volume", 75).mod (env2, "osc2Pitch", 25).noise (1, 0, 20).env (3, 0, 320, 0, 100, 60).mod (env3, "noiseLevel", 95)
            .filter (1, comb, 260, 75).filter (2, svfBand, 3400, 25).amp (0, 450, 0, 350, 40).drive (tube, 55, 8700).reverb (15, 80, 2.2f, 50)
            .level (-16).macro (1, "Wires").mod (macro1, "noiseLevel", -45).macro (2, "Ring").mod (macro2, "filter1Resonance", 15)
            .macro (3, "Crunch").mod (macro3, "fxDriveAmount", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Stagnant Thump", "Drum").osc (1, analog, 8).pitch (1, -12).env (2, 0, 25, 0, 40, 82).mod (env2, "osc1Pitch", 75).noise (1, 3, 40)
            .env (3, 0, 5, 0, 10, 90).mod (env3, "noiseLevel", 35).filter (1, svfLow, 3000, 5, 40).amp (0, 480, 0, 140, 55).level (-11)
            .macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Click").mod (macro2, "noiseLevel", 35).drive (tube, 0, 5000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Bite").mod (macro4, "filter1Drive", 60);
        add ("Stark Hoof", "Drum").osc (1, fm, 0).pitch (1, -12).subOsc (50, 0, 1).env (2, 0, 100, 0, 60, 86).mod (env2, "osc1Pitch", 75)
            .env (3, 0, 60, 0, 100, 80).mod (env3, "osc1Position", 19).filter (1, ladder24, 2900, 15, 20).amp (0, 1000, 0, 80, 70)
            .drive (fuzz, 30, 3100).level (-30).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Crunch")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Tone").mod (macro4, "filter1Cutoff", -30);
        add ("Steel Whip", "Drum").set ("osc1On", 0).parallel().osc (2, analog, sinePos, 1).pitch (2, 3).route (2, 100).env (2, 0, 70, 0, 60, 85)
            .mod (env2, "osc2Volume", 60).mod (env2, "osc2Pitch", 14.583f).noise (1, 2, -20).env (3, 0, 370, 0, 100, 65)
            .mod (env3, "noiseLevel", 65).filter (1, comb, 160, 60).filter (2, svfBand, 3700, 30).amp (0, 500, 0, 150, 40).drive (tube, 50, 4500)
            .reverb (30, 50, 1.8f, 70).level (-6).macro (1, "Wires").mod (macro1, "noiseLevel", -45).macro (2, "Ring")
            .mod (macro2, "filter1Resonance", 15).macro (3, "Crunch").mod (macro3, "fxDriveAmount", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Sterile Gong", "Drum").osc (1, metal, 75).pitch (1, 17).unison (1, 6, 50, 100).osc (2, metal, 80, 75).pitch (2, 18, -10)
            .unison (2, 4, 20, 100).noise (65, 0, 20).filter (1, ladderHigh, 5700, 5).env (2, 0, 220, 0, 200, 80).mod (env2, "noiseLevel", 15)
            .mod (env2, "filter1Cutoff", -4).amp (0, 2500, 0, 600).reverb (25, 70, 5.5f, 55).level (-2).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -12).macro (2, "Bright").mod (macro2, "osc1Position", -40).macro (3, "Wash")
            .mod (macro3, "noiseLevel", 30).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Stuttering Clap", "Drum").set ("osc1On", 0).noise (90, 0, 40).filter (1, ladderBand, 2200, 30).filter (2, svfHigh, 430)
            .lfo (1, steps, 12, 2, 75).stepPattern (1, { 0, -70, -70, 0, -70, 0, -70, 0, 0, 0, 0, 0, 0, 0, 0, 0 }).mod (lfo1, "ampLevel", 70)
            .amp (0, 200, 0, 120, 60).reverb (14, 50, 1.4f, 50).level (-1).macro (1, "Tone").mod (macro1, "filter1Cutoff", 15).macro (2, "Spread")
            .mod (macro2, "lfo1Rate", -12).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Tagged Snap", "Drum").set ("osc1On", 0).parallel().osc (2, analog, trianglePos, 1).pitch (2, 3).route (2, 100)
            .env (2, 0, 160, 0, 60, 85).mod (env2, "osc2Volume", 65).mod (env2, "osc2Pitch", 6.25f).noise (1, 1, -20).env (3, 0, 270, 0, 100, 65)
            .mod (env3, "noiseLevel", 70).filter (1, svfHigh, 3900, 40).filter (2, ladder12, 2600, 5).amp (0, 450, 0, 250, 40)
            .drive (tube, 25, 6400).level (-9).macro (1, "Wires").mod (macro1, "noiseLevel", -45).macro (2, "Tone")
            .mod (macro2, "filter1Cutoff", 18).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).reverb (0, 50, 1.2f, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Threadbare Crack", "Drum").set ("osc1On", 0).parallel().osc (2, analog, 20, 1).pitch (2, 7).route (2, 100).env (2, 0, 60, 0, 60, 85)
            .mod (env2, "osc2Volume", 75).mod (env2, "osc2Pitch", 6.25f).noise (1, 3, 60).env (3, 0, 280, 0, 100, 75).mod (env3, "noiseLevel", 90)
            .filter (1, comb, 470, 50).filter (2, svfBand, 950, 20).amp (0, 300, 0, 200, 40).drive (tube, 60, 3300).reverb (35, 60, 0.6f, 70)
            .level (-20).macro (1, "Wires").mod (macro1, "noiseLevel", -45).macro (2, "Ring").mod (macro2, "filter1Resonance", 15)
            .macro (3, "Crunch").mod (macro3, "fxDriveAmount", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Trackside Kick", "Drum").osc (1, sub, 10).pitch (1, -12).subOsc (30, 0, 1).env (2, 0, 70, 0, 60, 84).mod (env2, "osc1Pitch", 50)
            .env (3, 0, 200, 0, 100, 80).mod (env3, "osc1Position", 19).filter (1, ladder24, 2600, 15, 60).amp (0, 1000, 0, 200, 55)
            .drive (foldDrive, 50, 6500).level (-24).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Crunch")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Sub").mod (macro3, "subLevel", 35).macro (4, "Tone").mod (macro4, "filter1Cutoff", -30);
        add ("Trackside Snare", "Drum").set ("osc1On", 0).parallel().osc (2, analog, sinePos, 1).pitch (2, 0).route (2, 100)
            .env (2, 0, 190, 0, 60, 85).mod (env2, "osc2Volume", 75).mod (env2, "osc2Pitch", 10.417f).noise (1, 0, -20).env (3, 0, 210, 0, 100, 85)
            .mod (env3, "noiseLevel", 80).filter (1, comb, 400, 60).filter (2, svfLow, 3300, 15).amp (0, 650, 0, 150, 40).drive (hard, 40, 3600)
            .reverb (30, 40, 2, 70).level (-18).macro (1, "Wires").mod (macro1, "noiseLevel", -45).macro (2, "Ring")
            .mod (macro2, "filter1Resonance", 15).macro (3, "Crunch").mod (macro3, "fxDriveAmount", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Turning Slap", "Drum").set ("osc1On", 0).noise (100).filter (1, svfBand, 1800, 40).lfo (1, steps, 13.5f, 2, 45)
            .stepPattern (1, { 0, -70, -70, 0, -70, 0, -70, 0, 0, 0, 0, 0, 0, 0, 0, 0 }).mod (lfo1, "ampLevel", 75).amp (0, 500, 0, 120, 70)
            .drive (hard, 25, 7700).reverb (22, 60, 0.8f, 70).level (-10).macro (1, "Tone").mod (macro1, "filter1Cutoff", 15).macro (2, "Spread")
            .mod (macro2, "lfo1Rate", -12).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Umber Kick", "Drum").osc (1, fm, 30).pitch (1, -17).env (2, 0, 65, 0, 40, 88).mod (env2, "osc1Pitch", 25)
            .mod (env2, "osc1Position", 50).filter (1, ladder24, 2200, 5, 30).amp (0, 360, 0, 80, 75).level (-10).macro (1, "Punch")
            .mod (macro1, "osc1Pitch", 25).macro (2, "Knock").mod (macro2, "osc1Position", 30).drive (tube, 0, 5000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Tone").mod (macro4, "filter1Cutoff", -25);
        add ("Umber Lash", "Drum").set ("osc1On", 0).parallel().osc (2, analog, sinePos, 1).pitch (2, 3).route (2, 100).env (2, 0, 190, 0, 60, 85)
            .mod (env2, "osc2Volume", 80).mod (env2, "osc2Pitch", 10.417f).noise (1, 0, 40).env (3, 0, 190, 0, 100, 75).mod (env3, "noiseLevel", 90)
            .filter (1, comb, 400, 75).filter (2, ladder12, 2400, 15).amp (0, 500, 0, 300, 40).drive (crush, 40, 3100).reverb (25, 40, 1, 50)
            .level (-9).macro (1, "Wires").mod (macro1, "noiseLevel", -45).macro (2, "Ring").mod (macro2, "filter1Resonance", 15)
            .macro (3, "Crunch").mod (macro3, "fxDriveAmount", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Umber Wash", "Drum").osc (1, metal, 75, 55).pitch (1, 19).unison (1, 6, 30, 60).noise (85, 0, 80).filter (1, svfBand, 7000, 20)
            .amp (0, 750, 0, 100).reverb (20, 30, 1.6f, 60).level (-5).macro (1, "Tone").mod (macro1, "filter1Cutoff", -12).macro (2, "Metal")
            .mod (macro2, "osc1Volume", 30).macro (3, "Sizzle").mod (macro3, "filter1Resonance", 35).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Vicious Crack", "Drum").set ("osc1On", 0).parallel().osc (2, analog, 20, 1).pitch (2, 5).route (2, 100).env (2, 0, 110, 0, 60, 85)
            .mod (env2, "osc2Volume", 65).mod (env2, "osc2Pitch", 6.25f).noise (1).env (3, 0, 170, 0, 100, 75).mod (env3, "noiseLevel", 80)
            .filter (1, comb, 720, 60).filter (2, ladder12, 2600, 20).amp (0, 600, 0, 200, 40).drive (fuzz, 35, 6800).reverb (25, 50, 1.8f, 60)
            .level (-22).macro (1, "Wires").mod (macro1, "noiseLevel", -45).macro (2, "Ring").mod (macro2, "filter1Resonance", 15)
            .macro (3, "Crunch").mod (macro3, "fxDriveAmount", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Vicious Hat", "Drum").osc (1, metal, 85, 45).pitch (1, 12).unison (1, 4, 55, 70).noise (70, 0, 40).filter (1, svfBand, 6300, 35)
            .amp (0, 100, 0, 60, 65).mod (velocity, "filter1Cutoff", 5).drive (hard, 15, 6500).level (-1).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -12).macro (2, "Metal").mod (macro2, "osc1Volume", 30).macro (3, "Hiss").mod (macro3, "noiseLevel", -45)
            .reverb (0, 40, 0.8f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Wary Drum", "Drum").osc (1, analog, sinePos).pitch (1, -5).env (2, 0, 60, 0, 80, 88).mod (env2, "osc1Pitch", 6.25f).noise (1, 0, 20)
            .env (3, 0, 50, 0, 40, 85).mod (env3, "noiseLevel", 35).filter (1, svfLow, 3400, 25, 10).amp (0, 300, 0, 200, 70)
            .reverb (18, 45, 1.6f, 60).level (-12).macro (1, "Bend").mod (macro1, "osc1Pitch", 14.583f).macro (2, "Skin")
            .mod (macro2, "noiseLevel", 35).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Waxen Gong", "Drum").osc (1, metal, 75).pitch (1, 12).unison (1, 6, 50, 100).osc (2, metal, 80, 70).pitch (2, 7, -10)
            .unison (2, 3, 25, 100).noise (50, 0, 60).filter (1, svfHigh, 6600).env (2, 0, 140, 0, 200, 80).mod (env2, "noiseLevel", 25)
            .mod (env2, "filter1Cutoff", -8).amp (0, 2500, 0, 700, 60).reverb (20, 100, 4, 35).level (-5).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -12).macro (2, "Bright").mod (macro2, "osc1Position", -40).macro (3, "Wash")
            .mod (macro3, "noiseLevel", 30).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Weightless Thump", "Drum").osc (1, sub, 15).pitch (1, -17).env (2, 0, 75, 0, 40, 86).mod (env2, "osc1Pitch", 75).noise (1, 0, 20)
            .env (3, 0, 13, 0, 10, 90).mod (env3, "noiseLevel", 35).filter (1, svfLow, 7300, 5, 30).amp (0, 500, 0, 180, 55).drive (hard, 40, 3200)
            .level (-24).macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Click").mod (macro2, "noiseLevel", 35).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Bite").mod (macro4, "filter1Drive", 60);
        add ("Western Stamp", "Drum").osc (1, fm, 10).pitch (1, -19).env (2, 0, 25, 0, 40, 82).mod (env2, "osc1Pitch", 64.583f)
            .mod (env2, "osc1Position", 20).filter (1, svfLow, 1700, 5, 40).amp (0, 440, 0, 140, 65).drive (soft, 40, 2700).level (-21)
            .macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Knock").mod (macro2, "osc1Position", 30).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 50).macro (4, "Tone").mod (macro4, "filter1Cutoff", -25);
        add ("Wrecked Boot", "Drum").osc (1, analog, sinePos).pitch (1, -14).env (2, 0, 30, 0, 40, 82).mod (env2, "osc1Pitch", 50).noise (1, 3, 20)
            .env (3, 0, 12, 0, 10, 90).mod (env3, "noiseLevel", 40).filter (1, ladder24, 7300, 5, 10).amp (0, 240, 0, 80, 75).level (-9)
            .macro (1, "Punch").mod (macro1, "osc1Pitch", 25).macro (2, "Click").mod (macro2, "noiseLevel", 35).drive (tube, 0, 5000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 50).macro (4, "Bite").mod (macro4, "filter1Drive", 60);

        // FX, by name.
        add ("Acrid Seizure", "FX").osc (1, growl, 40).warp (1, wSync, 70).unison (1, 2, 16, 70).env (2, 0, 1700, 0, 1700, 10)
            .mod (env2, "osc1WarpAmount", 60).mod (env2, "filter1Cutoff", 35).mod (env2, "lfo1Rate", 45).lfo (1, sine, 2.1f)
            .mod (lfo1, "ampLevel", 10).filter (1, ladder24, 310, 35).amp (0, 1700, 0, 1300, 0).reverb (25, 100, 4.5f, 65).level (-11)
            .macro (1, "Sync").mod (macro1, "osc1WarpAmount", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 15).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Amber Siren", "FX").osc (1, analog, squarePos).unison (1, 3, 12, 70).lfo (1, sawDown, 7.8f).mod (lfo1, "osc1Pitch", 12.5f)
            .filter (1, svfLow, 1900, 30, 30).amp (5, 300, 100, 260).reverb (28, 85, 4.5f, 50).level (-15).macro (1, "Rate")
            .mod (macro1, "lfo1Rate", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Arctic Lockdown", "FX").osc (1, sync, 20).unison (1, 1, 16, 70).lfo (1, square, 2.9f).mod (lfo1, "osc1Pitch", 8.333f)
            .filter (1, svfLow, 1200, 15, 10).amp (5, 300, 100, 180).drive (tube, 25, 8400).reverb (28, 85, 6, 55).level (-15).macro (1, "Rate")
            .mod (macro1, "lfo1Rate", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).delay (eighthD, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Arctic Sonar", "FX").osc (1, analog, sinePos).pitch (1, 7).env (2, 0, 80, 0, 100, 70).mod (env2, "osc1Fine", -15)
            .filter (1, svfBand, 2700, 25, 0, 50).amp (8, 400, 0, 350, 60).delay (half, 75, 35, 5900).reverb (30, 90, 5, 35).level (5)
            .macro (1, "Echo").mod (macro1, "fxDelayMix", 35).macro (2, "Tone").mod (macro2, "filter1Cutoff", 20).drive (tube, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Banished Echo", "FX").osc (1, analog, trianglePos).pitch (1, 7).env (2, 0, 160, 0, 100, 70).mod (env2, "osc1Fine", -10)
            .filter (1, svfBand, 3200, 15, 0, 50).amp (2, 300, 0, 250, 65).delay (quarterD, 75, 50, 4600).reverb (50, 100, 3, 70).level (5)
            .macro (1, "Echo").mod (macro1, "fxDelayMix", 35).macro (2, "Tone").mod (macro2, "filter1Cutoff", 20).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Bent Terminal", "FX").osc (1, fm, 40).lfo (1, randomShape, 6.3f, 0).mod (lfo1, "osc1Pitch", 30).filter (1, svfLow, 5200, 25)
            .amp (5, 300, 100, 60).delay (sixteenthT, 45, 16, 2600).level (-14).macro (1, "Speed").mod (macro1, "lfo1Rate", 20).macro (2, "Range")
            .mod (macro2, "osc1Pitch", 20).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 70, 3, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Binary Console", "FX").osc (1, fm, 20).lfo (1, randomShape, 12, 0).mod (lfo1, "osc1Pitch", 40).lfo (2, randomShape, 3.7f, 0)
            .mod (lfo2, "ampPan", 25).filter (1, ladder12, 4800).amp (5, 300, 100, 240).delay (sixteenthD, 55, 22, 5200).reverb (19, 70, 3.5f, 40)
            .level (-14).macro (1, "Speed").mod (macro1, "lfo1Rate", 20).macro (2, "Range").mod (macro2, "osc1Pitch", 20).drive (crush, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Bitwise Computer", "FX").osc (1, pulse, 15).lfo (1, randomShape, 8.7f, 0).mod (lfo1, "osc1Pitch", 40).lfo (2, randomShape, 8, 0)
            .mod (lfo2, "ampPan", 45).filter (1, svfBand, 1800, 5).amp (5, 300, 100, 80).delay (sixteenth, 40, 18, 2600).reverb (16, 100, 3.5f, 60)
            .level (-16).macro (1, "Speed").mod (macro1, "lfo1Rate", 20).macro (2, "Range").mod (macro2, "osc1Pitch", 20).drive (crush, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Cutoff").mod (macro4, "filter1Cutoff", 20);
        add ("Bleak Shot", "FX").osc (1, analog, squarePos).warp (1, wSync, 90).env (2, 0, 420, 0, 200, 90).mod (env2, "osc1Pitch", 50)
            .mod (env2, "osc1WarpAmount", -40).filter (1, svfBand, 960, 15).amp (0, 150, 0, 400, 80).delay (sixteenthD, 30, 20, 7000)
            .reverb (29, 100, 3.5f, 30).level (-8).macro (1, "Sync").mod (macro1, "osc1WarpAmount", -35).macro (2, "Sweep")
            .mod (macro2, "osc1Pitch", 25).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Boarded Uplink", "FX").osc (1, analog, sinePos).osc (2, analog, sinePos, 65).pitch (2, 11, -20).lfoSync (1, steps, bars2, 1, 80)
            .stepPattern (1, { 0, 0, 0, -100, -100, 0, 0, -100, 0, 0, 0, -100, -100, 0, 0, 0 }).mod (lfo1, "ampLevel", 70)
            .lfoSync (2, steps, bars2, 1, 90)
            .stepPattern (2, { 100, 100, -100, 100, -100, -100, 100, 100, 100, -100, -100, 100, -100, -100, 100, -100 })
            .mod (lfo2, "osc2Volume", 25).filter (1, svfBand, 1500, 40, 0, 50).amp (3, 300, 100, 120).delay (eighthD, 55, 10, 2600, false)
            .level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Second").mod (macro2, "osc2Volume", -50)
            .reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Border Squawk", "FX").osc (1, analog, sinePos).osc (2, analog, sinePos).pitch (2, 8, 5).lfoSync (1, steps, quarter, 1, 80)
            .stepPattern (1, { 0, 0, -100, 0, -100, -100, -100, 0, 0, -100, 0, -100, -100, 0, -100, -100 }).mod (lfo1, "ampLevel", 80)
            .lfoSync (2, steps, bars2, 1, 80)
            .stepPattern (2, { 100, 100, -100, -100, -100, -100, 100, 100, -100, -100, -100, -100, -100, 100, -100, 100 })
            .mod (lfo2, "osc2Volume", 45).filter (1, ladder12, 1500, 15, 0, 50).amp (3, 300, 100, 170).delay (quarter, 25, 18, 7000).level (-17)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Second").mod (macro2, "osc2Volume", -50).reverb (0, 70, 3, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Border Warning", "FX").osc (1, sync, 60).unison (1, 1, 14, 30).lfo (1, sawUp, 3.1f).mod (lfo1, "osc1Pitch", 2.083f)
            .filter (1, ladderBand, 1900, 40, 10).amp (5, 300, 100, 240).drive (hard, 15, 2600).delay (quarterT, 40, 18, 2500)
            .reverb (27, 80, 4, 65).level (-13).macro (1, "Rate").mod (macro1, "lfo1Rate", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Coal Whoosh", "FX").set ("osc1On", 0).noise (95, 0, -20).filter (1, ladder24, 430, 30).env (2, 700, 700, 0, 700, -30)
            .mod (env2, "filter1Cutoff", 45).lfo (1, sawDown, 0.71f, 2).mod (lfo1, "ampPan", 45).amp (700, 700, 0, 700, 0).phaser (55, 0.64f, 40)
            .reverb (30, 75, 8, 45).level (-1).macro (1, "Height").mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 20).macro (3, "Phase").mod (macro3, "fxPhaserMix", 50).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Corrupt Handshake", "FX").osc (1, analog, trianglePos).osc (2, analog, sinePos).pitch (2, 11, 15).lfoSync (1, steps, quarter, 1, 92)
            .stepPattern (1, { 0, -100, -100, 0, 0, -100, 0, 0, -100, 0, 0, -100, -100, 0, 0, 0 }).mod (lfo1, "ampLevel", 85)
            .lfoSync (2, steps, half, 1, 90)
            .stepPattern (2, { -100, -100, -100, 100, -100, 100, -100, 100, -100, 100, 100, -100, -100, 100, 100, 100 })
            .mod (lfo2, "osc2Volume", 40).filter (1, svfBand, 1900, 35, 0, 50).amp (3, 300, 100, 170).drive (crush, 25, 5700)
            .delay (eighthD, 50, 24, 3100).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Second")
            .mod (macro2, "osc2Volume", -50).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 35);
        add ("Cracked Short", "FX").set ("osc1On", 0).noise (95, 2, 60).filter (1, ladderBand, 2200, 92).lfo (1, randomShape, 28, 0)
            .mod (lfo1, "filter1Cutoff", 16).lfo (2, drift, 2.2f, 0).mod (lfo2, "ampLevel", 11).amp (5, 400, 90, 400).reverb (10, 95, 4.5f, 45)
            .level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Jump").mod (macro2, "lfo1Rate", 20).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Resonance").mod (macro4, "filter1Resonance", 10);
        add ("Depot Wake", "FX").set ("osc1On", 0).noise (90, 1, -40).filter (1, ladder24, 380, 60).env (2, 400, 400, 0, 400, 40)
            .mod (env2, "filter1Cutoff", 25).lfo (1, sawUp, 1.25f, 2).mod (lfo1, "ampPan", 40).amp (400, 400, 0, 400, 40).reverb (40, 100, 3, 35)
            .level (-1).macro (1, "Height").mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance").mod (macro2, "filter1Resonance", 20)
            .drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).phaser (0, 0.2f, 55).macro (4, "Phase")
            .mod (macro4, "fxPhaserMix", 50);
        add ("Distant Echo", "FX").osc (1, analog, trianglePos).pitch (1, 12).osc (2, analog, sinePos, 60).pitch (2, 5, 7)
            .env (2, 0, 80, 0, 100, 70).mod (env2, "osc1Fine", 20).filter (1, svfBand, 3500, 25, 0, 50).amp (2, 380, 0, 600)
            .delay (half, 65, 30, 2300).reverb (50, 75, 8, 30).level (3).macro (1, "Echo").mod (macro1, "fxDelayMix", 35).macro (2, "Tone")
            .mod (macro2, "filter1Cutoff", 20).macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Distant Wake", "FX").set ("osc1On", 0).noise (90, 1, -20).filter (1, ladderBand, 220, 50).env (2, 600, 600, 0, 600, -10)
            .mod (env2, "filter1Cutoff", 25).lfo (1, sawDown, 0.83f, 2).mod (lfo1, "ampPan", 25).amp (600, 600, 0, 600, 20).reverb (35, 85, 6, 30)
            .level (-1).macro (1, "Height").mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance").mod (macro2, "filter1Resonance", 20)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Dreamless Drone", "FX").osc (1, analog, trianglePos).osc (2, analog, sinePos, 55).pitch (2, -12).lfo (1, triangle, 11, 0)
            .mod (lfo1, "osc1Pitch", 14.583f).mod (lfo1, "osc2Fine", -45).lfo (2, drift, 0.84f, 0).mod (lfo2, "lfo1Rate", 12)
            .filter (1, ladder12, 4200).amp (320, 400, 100, 1100).phaser (50, 0.12f, 60).reverb (30, 85, 5, 70).level (-15).macro (1, "Wobble")
            .mod (macro1, "lfo1Rate", 20).macro (2, "Second").mod (macro2, "osc2Volume", -50).macro (3, "Phase").mod (macro3, "fxPhaserMix", 35)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Echoing Pulse", "FX").osc (1, analog, sinePos).pitch (1, 19).env (2, 0, 100, 0, 100, 70).mod (env2, "osc1Fine", -15)
            .filter (1, svfBand, 3300, 15, 0, 50).amp (8, 580, 0, 250).delay (quarterD, 60, 50, 4400).reverb (50, 75, 4, 50).level (-2)
            .macro (1, "Echo").mod (macro1, "fxDelayMix", 35).macro (2, "Tone").mod (macro2, "filter1Cutoff", 20).drive (tube, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Flayed Short", "FX").set ("osc1On", 0).noise (95, 2, 20).filter (1, ladderBand, 4900, 92).lfo (1, randomShape, 11, 0)
            .mod (lfo1, "filter1Cutoff", 18).lfo (2, drift, 1.4f, 0).mod (lfo2, "ampLevel", 19).amp (5, 400, 70, 300).drive (hard, 45, 8900)
            .reverb (20, 90, 9, 55).level (-17).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Jump").mod (macro2, "lfo1Rate", 20)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 10);
        add ("Floating Transponder", "FX").osc (1, analog, sinePos).pitch (1, 12).env (2, 0, 160, 0, 100, 70).mod (env2, "osc1Fine", -5)
            .filter (1, svfBand, 1300, 45, 0, 50).amp (0, 520, 0, 300, 70).delay (half, 65, 35, 4800).reverb (50, 100, 5, 60).level (-3)
            .macro (1, "Echo").mod (macro1, "fxDelayMix", 35).macro (2, "Tone").mod (macro2, "filter1Cutoff", 20).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Flooded Orbiter", "FX").osc (1, analog, sinePos).osc (2, analog, sinePos, 65).pitch (2, -12, 7).lfo (1, triangle, 7.1f, 0)
            .mod (lfo1, "osc1Pitch", 2.083f).mod (lfo1, "osc2Fine", -50).lfo (2, drift, 0.6f, 0).mod (lfo2, "lfo1Rate", 12)
            .filter (1, ladder12, 3000).amp (60, 400, 100, 1300).phaser (60, 0.49f, 50).reverb (25, 80, 5.5f, 35).level (-16).macro (1, "Wobble")
            .mod (macro1, "lfo1Rate", 20).macro (2, "Second").mod (macro2, "osc2Volume", -50).delay (eighthD, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Fugitive Corona", "FX").set ("osc1On", 0).noise (90, 2, 20).filter (1, ladderBand, 1600, 68).lfo (1, randomShape, 12, 0)
            .mod (lfo1, "filter1Cutoff", 8).lfo (2, randomShape, 1.6f, 0).mod (lfo2, "ampLevel", 23).amp (30, 400, 80, 150).drive (hard, 45, 2600)
            .reverb (10, 100, 8.5f, 35).level (-18).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Jump")
            .mod (macro2, "lfo1Rate", 20).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Gated Arc", "FX").set ("osc1On", 0).noise (80, 2, 40).filter (1, ladderBand, 2000, 88).lfo (1, randomShape, 5.4f, 0)
            .mod (lfo1, "filter1Cutoff", 20).lfo (2, randomShape, 7.4f, 0).mod (lfo2, "ampLevel", 25).amp (5, 400, 70, 550).drive (crush, 25, 8200)
            .reverb (25, 75, 4.5f, 40).level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Jump")
            .mod (macro2, "lfo1Rate", 20).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 10);
        add ("Glacial Laser", "FX").osc (1, analog, squarePos).warp (1, wSync, 50).env (2, 0, 380, 0, 200, 80).mod (env2, "osc1Pitch", -25)
            .mod (env2, "osc1WarpAmount", -30).filter (1, svfHigh, 8300, 30).amp (0, 750, 0, 200).delay (eighth, 25, 14, 5200).level (-4)
            .macro (1, "Sync").mod (macro1, "osc1WarpAmount", -35).macro (2, "Sweep").mod (macro2, "osc1Pitch", 25).reverb (0, 70, 3, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Glinting Satellite", "FX").osc (1, analog, 15).osc (2, analog, sinePos, 75).pitch (2, 7, 8).lfo (1, sine, 20, 0)
            .mod (lfo1, "osc1Pitch", 2.083f).mod (lfo1, "osc2Fine", -25).lfo (2, sine, 0.11f, 0).mod (lfo2, "lfo1Rate", 18)
            .filter (1, ladder12, 4600).amp (20, 400, 100, 800).phaser (60, 0.1f, 50).reverb (35, 75, 6, 55).level (-14).macro (1, "Wobble")
            .mod (macro1, "lfo1Rate", 20).macro (2, "Second").mod (macro2, "osc2Volume", -50).drive (tube, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Gnarled Brownout", "FX").osc (1, analog, 30).warp (1, wSync, 40).unison (1, 2, 8, 70).env (2, 0, 1500, 0, 1500, 40)
            .mod (env2, "osc1WarpAmount", 70).mod (env2, "filter1Cutoff", 20).mod (env2, "lfo1Rate", 40).lfo (1, sine, 1.8f)
            .mod (lfo1, "ampLevel", 18).filter (1, ladder24, 360, 30).amp (0, 1500, 0, 500, 40).reverb (31, 100, 7.5f, 35).level (-14)
            .macro (1, "Sync").mod (macro1, "osc1WarpAmount", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 15).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Grim Discharge", "FX").set ("osc1On", 0).noise (90, 2).filter (1, svfBand, 1300, 76).lfo (1, randomShape, 9, 0)
            .mod (lfo1, "filter1Cutoff", 22).lfo (2, drift, 1.5f, 0).mod (lfo2, "ampLevel", 15).amp (0, 400, 70, 250).reverb (15, 100, 7, 45)
            .level (1).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Jump").mod (macro2, "lfo1Rate", 20).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 10);
        add ("Guarded Seizure", "FX").osc (1, analog, sawPos).warp (1, wSync, 50).unison (1, 3, 10, 70).env (2, 0, 1600, 0, 1600, 20)
            .mod (env2, "osc1WarpAmount", 60).mod (env2, "filter1Cutoff", 40).mod (env2, "lfo1Rate", 35).lfo (1, triangle, 1.1f)
            .mod (lfo1, "ampLevel", 14).filter (1, svfLow, 430, 25).amp (0, 1600, 0, 500, 20).reverb (18, 80, 5.5f, 35).level (-16)
            .macro (1, "Sync").mod (macro1, "osc1WarpAmount", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 15).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Halogen Sounding", "FX").osc (1, analog, trianglePos).pitch (1, 7).osc (2, analog, sinePos, 45).pitch (2, -5, 1)
            .env (2, 0, 280, 0, 100, 70).mod (env2, "osc1Fine", -10).filter (1, ladder12, 4700, 50, 0, 50).amp (8, 700, 0, 600, 65)
            .delay (quarter, 60, 45, 5100).reverb (35, 80, 5, 45).level (-7).macro (1, "Echo").mod (macro1, "fxDelayMix", 35).macro (2, "Tone")
            .mod (macro2, "filter1Cutoff", 20).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Harbour Flyby", "FX").set ("osc1On", 0).noise (75, 0, 20).filter (1, ladderBand, 750, 60).env (2, 2100, 2100, 0, 2100, 20)
            .mod (env2, "filter1Cutoff", 20).lfo (1, sawDown, 0.24f, 2).mod (lfo1, "ampPan", 25).amp (2100, 2100, 0, 2100, -20)
            .phaser (45, 1.2f, 40).reverb (25, 95, 7.5f, 55).level (1).macro (1, "Height").mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 20).macro (3, "Phase").mod (macro3, "fxPhaserMix", 50).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Heartless Lance", "FX").osc (1, pulse, 0).warp (1, wSync, 75).env (2, 0, 480, 0, 200, 70).mod (env2, "osc1Pitch", 25)
            .mod (env2, "osc1WarpAmount", -70).filter (1, svfHigh, 1700).amp (0, 450, 0, 300).delay (eighth, 35, 26, 7400).level (-12)
            .macro (1, "Sync").mod (macro1, "osc1WarpAmount", -35).macro (2, "Sweep").mod (macro2, "osc1Pitch", 25).macro (3, "Cutoff")
            .mod (macro3, "filter1Cutoff", 20).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Hooded Burst", "FX").osc (1, analog, trianglePos).osc (2, analog, sinePos, 70).pitch (2, 5, 5).lfoSync (1, steps, bar1, 1, 88)
            .stepPattern (1, { 0, -100, 0, -100, 0, -100, -100, 0, 0, 0, -100, 0, 0, 0, 0, -100 }).mod (lfo1, "ampLevel", 70)
            .lfoSync (2, steps, bars2, 1, 80)
            .stepPattern (2, { -100, -100, 100, 100, -100, -100, 100, -100, -100, 100, 100, -100, 100, -100, 100, -100 })
            .mod (lfo2, "osc2Volume", 45).filter (1, ladder12, 1700, 35, 0, 50).amp (3, 300, 100, 120).drive (crush, 20, 6800)
            .delay (eighthT, 25, 18, 3100).level (-17).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Second")
            .mod (macro2, "osc2Volume", -50).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).reverb (0, 70, 3, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Hushed Orbiter", "FX").osc (1, analog, sinePos).osc (2, analog, sinePos, 60).pitch (2, -12, 6).lfo (1, triangle, 20, 0)
            .mod (lfo1, "osc1Pitch", 10.417f).mod (lfo1, "osc2Fine", -40).lfo (2, sine, 0.24f, 0).mod (lfo2, "lfo1Rate", 20)
            .filter (1, ladder12, 6400).amp (380, 400, 100, 500).phaser (60, 0.2f, 60).reverb (40, 95, 6.5f, 60).level (-17).macro (1, "Wobble")
            .mod (macro1, "lfo1Rate", 20).macro (2, "Second").mod (macro2, "osc2Volume", -50).macro (3, "Phase").mod (macro3, "fxPhaserMix", 35)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Indexed Alarm", "FX").osc (1, analog, squarePos).unison (1, 3, 6, 70).lfo (1, sawDown, 4.6f).mod (lfo1, "osc1Pitch", 2.083f)
            .filter (1, ladderBand, 3100, 30, 20).amp (5, 300, 100, 280).drive (crush, 30, 9800).reverb (28, 90, 9, 30).level (-8).macro (1, "Rate")
            .mod (macro1, "lfo1Rate", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Lawless Outage", "FX").osc (1, pulse, 66.67f).warp (1, wSync, 80).unison (1, 1, 16, 70).env (2, 0, 3700, 0, 3700, 30)
            .mod (env2, "osc1WarpAmount", 50).mod (env2, "filter1Cutoff", 25).mod (env2, "lfo1Rate", 35).lfo (1, triangle, 1.4f)
            .mod (lfo1, "ampLevel", 12).filter (1, svfLow, 150, 40).amp (0, 3700, 0, 1400, 0).reverb (20, 90, 7, 60).level (-3).macro (1, "Sync")
            .mod (macro1, "osc1WarpAmount", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 15).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Lean Lockdown", "FX").osc (1, sync, 60).unison (1, 3, 10, 40).lfo (1, square, 6.1f).mod (lfo1, "osc1Pitch", 12.5f)
            .filter (1, ladderBand, 2200, 40).amp (5, 300, 100, 80).drive (crush, 30, 6600).delay (eighth, 25, 12, 4600, false)
            .reverb (24, 100, 3, 60).level (-11).macro (1, "Rate").mod (macro1, "lfo1Rate", 20).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Lingering Pulse", "FX").osc (1, analog, sinePos).pitch (1, 0).osc (2, analog, sinePos, 50).pitch (2, -7, 2)
            .env (2, 0, 260, 0, 100, 70).mod (env2, "osc1Fine", -25).filter (1, svfBand, 1500, 20, 0, 50).amp (0, 420, 0, 350, 75)
            .delay (quarterD, 60, 40, 5300).reverb (45, 70, 4, 35).level (1).macro (1, "Echo").mod (macro1, "fxDelayMix", 35).macro (2, "Tone")
            .mod (macro2, "filter1Cutoff", 20).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Lowlight Whoosh", "FX").set ("osc1On", 0).noise (95, 0, 20).filter (1, ladderBand, 520, 45).env (2, 400, 400, 0, 400, 10)
            .mod (env2, "filter1Cutoff", 45).lfo (1, sawDown, 1.25f, 2).mod (lfo1, "ampPan", 40).amp (400, 400, 0, 400, 0).reverb (25, 90, 5, 65)
            .level (-6).macro (1, "Height").mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance").mod (macro2, "filter1Resonance", 20)
            .drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).phaser (0, 0.2f, 55).macro (4, "Phase")
            .mod (macro4, "fxPhaserMix", 50);
        add ("Neon Computer", "FX").osc (1, fm, 55).lfo (1, randomShape, 12, 0).mod (lfo1, "osc1Pitch", 25).filter (1, ladder12, 5900, 30)
            .amp (5, 300, 100, 80).delay (sixteenthT, 35, 16, 4800).reverb (15, 95, 6.5f, 35).level (-13).macro (1, "Speed")
            .mod (macro1, "lfo1Rate", 20).macro (2, "Range").mod (macro2, "osc1Pitch", 20).macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 20)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Nightshift Flashover", "FX").set ("osc1On", 0).noise (90, 2, 20).filter (1, svfBand, 3900, 64).lfo (1, randomShape, 17, 0)
            .mod (lfo1, "filter1Cutoff", 20).lfo (2, randomShape, 3.8f, 0).mod (lfo2, "ampLevel", 9).amp (0, 400, 90, 100).reverb (25, 75, 5, 65)
            .level (1).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Jump").mod (macro2, "lfo1Rate", 20).drive (hard, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Resonance").mod (macro4, "filter1Resonance", 10);
        add ("Oiled Cortex", "FX").osc (1, fm, 55).lfo (1, randomShape, 12, 0).mod (lfo1, "osc1Pitch", 20).lfo (2, randomShape, 5.5f, 0)
            .mod (lfo2, "ampPan", 45).filter (1, svfBand, 3300, 30).amp (5, 300, 100, 200).delay (eighthT, 25, 22, 3700).level (-12)
            .macro (1, "Speed").mod (macro1, "lfo1Rate", 20).macro (2, "Range").mod (macro2, "osc1Pitch", 20).reverb (0, 70, 3, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Outlaw Beam", "FX").osc (1, pulse, 30).warp (1, wSync, 65).env (2, 0, 140, 0, 200, 90).mod (env2, "osc1Pitch", 25)
            .mod (env2, "osc1WarpAmount", -40).filter (1, svfBand, 1200, 45).amp (0, 450, 0, 350, 80).drive (hard, 30, 3500)
            .delay (sixteenthD, 25, 12, 6100).level (-19).macro (1, "Sync").mod (macro1, "osc1WarpAmount", -35).macro (2, "Sweep")
            .mod (macro2, "osc1Pitch", 25).macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 20).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Outlaw Klaxon", "FX").osc (1, analog, squarePos).unison (1, 3, 8, 50).lfo (1, square, 1.1f).mod (lfo1, "osc1Pitch", 14.583f)
            .filter (1, svfBand, 4600, 30, 20).amp (5, 300, 100, 100).reverb (35, 70, 3.5f, 40).level (-11).macro (1, "Rate")
            .mod (macro1, "lfo1Rate", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Outlaw Shot", "FX").osc (1, pulse, 60).warp (1, wSync, 85).env (2, 0, 80, 0, 200).mod (env2, "osc1Pitch", 39.583f)
            .mod (env2, "osc1WarpAmount", -60).filter (1, svfLow, 1000, 30).amp (0, 600, 0, 200, 70).drive (crush, 30, 7000)
            .delay (eighthT, 45, 14, 6100, false).level (-3).macro (1, "Sync").mod (macro1, "osc1WarpAmount", -35).macro (2, "Sweep")
            .mod (macro2, "osc1Pitch", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Cutoff").mod (macro4, "filter1Cutoff", 20);
        add ("Padded Satellite", "FX").osc (1, analog, sinePos).osc (2, analog, sinePos, 70).pitch (2, 5, 10).lfo (1, sine, 5.7f, 0)
            .mod (lfo1, "osc1Pitch", 14.583f).mod (lfo1, "osc2Fine", -45).lfo (2, drift, 0.38f, 0).mod (lfo2, "lfo1Rate", 14)
            .filter (1, ladder12, 3000).amp (280, 400, 100, 800).phaser (35, 0.41f, 55).reverb (25, 100, 8.5f, 45).level (-16).macro (1, "Wobble")
            .mod (macro1, "lfo1Rate", 20).macro (2, "Second").mod (macro2, "osc2Volume", -50).delay (eighthD, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Phase").mod (macro4, "fxPhaserMix", 35);
        add ("Perimeter Stall", "FX").osc (1, growl, 30).warp (1, wSync, 50).unison (1, 3, 14, 70).env (2, 0, 1300, 0, 1300, 60)
            .mod (env2, "osc1WarpAmount", 50).mod (env2, "filter1Cutoff", 25).mod (env2, "lfo1Rate", 25).lfo (1, triangle, 2.4f)
            .mod (lfo1, "ampLevel", 18).filter (1, ladder24, 130, 20).amp (0, 1300, 0, 1200, 40).reverb (12, 90, 3.5f, 70).level (-1)
            .macro (1, "Sync").mod (macro1, "osc1WarpAmount", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 15).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Phosphor Lance", "FX").osc (1, analog, sawPos).warp (1, wSync, 55).env (2, 0, 280, 0, 200, 60).mod (env2, "osc1Pitch", -25)
            .mod (env2, "osc1WarpAmount", -30).filter (1, svfHigh, 1200, 45).amp (0, 700, 0, 400, 60).delay (sixteenthD, 35, 22, 4900)
            .reverb (17, 75, 6, 45).level (-12).macro (1, "Sync").mod (macro1, "osc1WarpAmount", -35).macro (2, "Sweep")
            .mod (macro2, "osc1Pitch", 25).macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 20).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Polished Cortex", "FX").osc (1, analog, trianglePos).lfo (1, randomShape, 5.1f, 0).mod (lfo1, "osc1Pitch", 15)
            .filter (1, svfLow, 7200, 40).amp (5, 300, 100, 260).delay (eighthT, 40, 30, 5300).level (-14).macro (1, "Speed")
            .mod (macro1, "lfo1Rate", 20).macro (2, "Range").mod (macro2, "osc1Pitch", 20).macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 20)
            .drive (crush, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Quicksilver Console", "FX").osc (1, analog, squarePos).lfo (1, randomShape, 9.7f, 0).mod (lfo1, "osc1Pitch", 40)
            .filter (1, ladder12, 4800, 25).amp (5, 300, 100, 260).delay (sixteenth, 25, 18, 3100).level (-16).macro (1, "Speed")
            .mod (macro1, "lfo1Rate", 20).macro (2, "Range").mod (macro2, "osc1Pitch", 20).drive (crush, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Quicksilver Drone", "FX").osc (1, analog, trianglePos).osc (2, analog, sinePos, 60).pitch (2, 12, 4).lfo (1, triangle, 9.5f, 0)
            .mod (lfo1, "osc1Pitch", 2.083f).mod (lfo1, "osc2Fine", -25).lfo (2, sine, 0.083f, 0).mod (lfo2, "lfo1Rate", 12)
            .filter (1, svfLow, 5900).amp (300, 400, 100, 600).phaser (35, 0.26f, 70).reverb (40, 75, 7, 70).level (-16).macro (1, "Wobble")
            .mod (macro1, "lfo1Rate", 20).macro (2, "Second").mod (macro2, "osc2Volume", -50).delay (eighthD, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Remote Sweep", "FX").set ("osc1On", 0).noise (85, 1, 40).filter (1, svfBand, 240, 30).env (2, 500, 500, 0, 500, -10)
            .mod (env2, "filter1Cutoff", 40).lfo (1, sawUp, 1, 2).mod (lfo1, "ampPan", 25).amp (500, 500, 0, 500, 0).phaser (30, 1.5f, 70)
            .reverb (40, 90, 3, 60).level (3).macro (1, "Height").mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 20).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Colour")
            .mod (macro4, "noiseColour", 40);
        add ("Ruined Computer", "FX").osc (1, analog, sinePos).lfo (1, randomShape, 4.8f, 0).mod (lfo1, "osc1Pitch", 20)
            .filter (1, svfBand, 2300, 35).amp (5, 300, 100, 180).delay (sixteenthD, 35, 28, 4600).level (3).macro (1, "Speed")
            .mod (macro1, "lfo1Rate", 20).macro (2, "Range").mod (macro2, "osc1Pitch", 20).macro (3, "Echo").mod (macro3, "fxDelayMix", 35)
            .macro (4, "Cutoff").mod (macro4, "filter1Cutoff", 20);
        add ("Rusted Uplink", "FX").osc (1, analog, trianglePos).osc (2, analog, sinePos).pitch (2, 7, -5).lfoSync (1, steps, bars2, 1, 80)
            .stepPattern (1, { 0, 0, -100, 0, -100, 0, 0, 0, 0, 0, -100, 0, 0, 0, 0, 0 }).mod (lfo1, "ampLevel", 75).lfoSync (2, steps, half, 1, 80)
            .stepPattern (2, { -100, 100, -100, -100, -100, -100, -100, -100, -100, 100, 100, -100, -100, -100, 100, 100 })
            .mod (lfo2, "osc2Volume", 25).filter (1, svfBand, 4800, 40, 0, 50).amp (3, 300, 100, 90).drive (crush, 40, 7600)
            .delay (sixteenthD, 30, 14, 5100).level (3).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Second")
            .mod (macro2, "osc2Volume", -50).macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).reverb (0, 70, 3, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Scalding Lance", "FX").osc (1, pulse, 50).warp (1, wSync, 50).env (2, 0, 180, 0, 200).mod (env2, "osc1Pitch", -25)
            .mod (env2, "osc1WarpAmount", -60).filter (1, svfLow, 1600, 40).amp (0, 250, 0, 400, 70).drive (hard, 15).delay (eighthT, 25, 24, 2700)
            .level (-15).macro (1, "Sync").mod (macro1, "osc1WarpAmount", -35).macro (2, "Sweep").mod (macro2, "osc1Pitch", 25).macro (3, "Cutoff")
            .mod (macro3, "filter1Cutoff", 20).reverb (0, 70, 3, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Scalding Outage", "FX").osc (1, pulse, 40).warp (1, wSync, 45).unison (1, 2, 10, 70).env (2, 0, 2200, 0, 2200, 30)
            .mod (env2, "osc1WarpAmount", 30).mod (env2, "filter1Cutoff", 40).mod (env2, "lfo1Rate", 30).lfo (1, sine, 0.71f)
            .mod (lfo1, "ampLevel", 8).filter (1, ladder24, 240, 45).amp (0, 2200, 0, 1200, 30).reverb (20, 80, 3.5f, 40).level (-15)
            .macro (1, "Sync").mod (macro1, "osc1WarpAmount", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 15).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Sentinel Klaxon", "FX").osc (1, analog, squarePos).unison (1, 3, 10, 30).lfo (1, triangle, 1.6f).mod (lfo1, "osc1Pitch", 2.083f)
            .filter (1, ladder12, 5700, 15).amp (5, 300, 100, 300).drive (hard, 30, 3400).reverb (12, 85, 4.5f, 50).level (-23).macro (1, "Rate")
            .mod (macro1, "lfo1Rate", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).delay (eighthD, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Sepia Sonar", "FX").osc (1, analog, trianglePos).pitch (1, 12).osc (2, analog, sinePos, 55).pitch (2, 0, 1)
            .env (2, 0, 160, 0, 100, 70).mod (env2, "osc1Fine", -25).filter (1, svfBand, 3000, 35, 0, 50).amp (8, 180, 0, 200, 75)
            .delay (quarter, 70, 40, 4000).reverb (30, 70, 4, 45).level (5).macro (1, "Echo").mod (macro1, "fxDelayMix", 35).macro (2, "Tone")
            .mod (macro2, "filter1Cutoff", 20).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Serrated Short", "FX").set ("osc1On", 0).noise (90, 2, 80).filter (1, ladderBand, 2800, 72).lfo (1, randomShape, 18, 0)
            .mod (lfo1, "filter1Cutoff", 22).lfo (2, randomShape, 4.3f, 0).mod (lfo2, "ampLevel", 23).amp (0, 400, 90, 400).drive (hard, 25, 9400)
            .reverb (10, 100, 4.5f, 65).level (-18).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Jump")
            .mod (macro2, "lfo1Rate", 20).macro (3, "Resonance").mod (macro3, "filter1Resonance", 10).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Shadowed Brownout", "FX").osc (1, growl, 40).warp (1, wSync, 55).unison (1, 3, 18, 70).env (2, 0, 3900, 0, 3900, 40)
            .mod (env2, "osc1WarpAmount", 60).mod (env2, "filter1Cutoff", 30).mod (env2, "lfo1Rate", 35).lfo (1, triangle, 2)
            .mod (lfo1, "ampLevel", 16).filter (1, ladder24, 200, 15).amp (0, 3900, 0, 1000, -10).drive (tube, 30, 3400).reverb (25, 100, 6, 55)
            .level (-13).macro (1, "Sync").mod (macro1, "osc1WarpAmount", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 15)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Solar Flashover", "FX").set ("osc1On", 0).noise (100, 2, 20).filter (1, ladderBand, 1600, 72).lfo (1, randomShape, 11, 0)
            .mod (lfo1, "filter1Cutoff", 10).lfo (2, randomShape, 5.3f, 0).mod (lfo2, "ampLevel", 23).amp (5, 400, 80, 500).drive (fuzz, 20, 5600)
            .reverb (20, 80, 4.5f, 45).level (-23).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Jump")
            .mod (macro2, "lfo1Rate", 20).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 10);
        add ("Stagnant Handshake", "FX").osc (1, analog, sinePos).osc (2, analog, sinePos, 60).pitch (2, 5, 5).lfoSync (1, steps, bars2, 1, 80)
            .stepPattern (1, { 0, 0, -100, -100, -100, -100, 0, 0, -100, 0, 0, -100, -100, -100, -100, 0 }).mod (lfo1, "ampLevel", 70)
            .lfoSync (2, steps, half, 1, 70)
            .stepPattern (2, { -100, 100, -100, -100, 100, 100, -100, 100, -100, 100, 100, -100, 100, -100, -100, -100 })
            .mod (lfo2, "osc2Volume", 40).filter (1, ladder12, 3400, 40, 0, 50).amp (3, 300, 100, 250).drive (crush, 35, 7400)
            .delay (eighthT, 55, 12, 4200).level (-17).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Second")
            .mod (macro2, "osc2Volume", -50).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Stern Warning", "FX").osc (1, pulse, 65).unison (1, 1, 12, 30).lfo (1, sawUp, 1.6f).mod (lfo1, "osc1Pitch", 6.25f)
            .filter (1, ladderBand, 1400, 50, 30).amp (5, 300, 100, 220).reverb (12, 95, 4.5f, 45).level (-9).macro (1, "Rate")
            .mod (macro1, "lfo1Rate", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Synthetic Beacon", "FX").osc (1, analog, sinePos).osc (2, analog, sinePos, 75).pitch (2, 4, -15).lfoSync (1, steps, bars2, 1, 92)
            .stepPattern (1, { 0, 0, -100, 0, 0, 0, -100, -100, -100, 0, 0, 0, 0, 0, 0, 0 }).mod (lfo1, "ampLevel", 75)
            .lfoSync (2, steps, bars2, 1, 80)
            .stepPattern (2, { 100, -100, -100, 100, -100, -100, -100, 100, -100, -100, -100, -100, 100, 100, 100, -100 })
            .mod (lfo2, "osc2Volume", 30).filter (1, svfBand, 3700, 20, 0, 50).amp (3, 300, 100, 210).drive (crush, 20, 7000)
            .delay (sixteenthD, 25, 16, 4200, false).level (0).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Second")
            .mod (macro2, "osc2Volume", -50).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Tagged Uplink", "FX").osc (1, analog, sinePos).osc (2, analog, sinePos, 60).pitch (2, 3, -10).lfoSync (1, steps, half, 1, 92)
            .stepPattern (1, { 0, 0, 0, 0, 0, -100, 0, -100, 0, 0, -100, -100, 0, 0, -100, 0 }).mod (lfo1, "ampLevel", 80)
            .lfoSync (2, steps, bars2, 1, 90)
            .stepPattern (2, { -100, 100, 100, 100, -100, 100, -100, 100, -100, -100, 100, -100, -100, -100, 100, -100 })
            .mod (lfo2, "osc2Volume", 25).filter (1, svfBand, 1300, 15, 0, 50).amp (3, 300, 100, 70).drive (crush, 20, 6700)
            .delay (eighth, 50, 24, 3100).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Second")
            .mod (macro2, "osc2Volume", -50).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 35);
        add ("Tender Craft", "FX").osc (1, analog, sinePos).osc (2, analog, sinePos, 50).pitch (2, 5, 3).lfo (1, sine, 12, 0)
            .mod (lfo1, "osc1Pitch", 4.167f).mod (lfo1, "osc2Fine", -50).lfo (2, sine, 0.41f, 0).mod (lfo2, "lfo1Rate", 16)
            .filter (1, ladder12, 4000).amp (440, 400, 100, 600).phaser (55, 0.33f, 60).reverb (30, 70, 3.5f, 30).level (-16).macro (1, "Wobble")
            .mod (macro1, "lfo1Rate", 20).macro (2, "Second").mod (macro2, "osc2Volume", -50).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Tender Flyby", "FX").set ("osc1On", 0).noise (100, 0, 40).filter (1, ladder24, 570, 35).env (2, 2300, 2300, 0, 2300, -10)
            .mod (env2, "filter1Cutoff", 45).lfo (1, sawDown, 0.22f, 2).mod (lfo1, "ampPan", 30).amp (2300, 2300, 0, 2300, 30)
            .reverb (20, 85, 3, 40).level (-6).macro (1, "Height").mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance")
            .mod (macro2, "filter1Resonance", 20).phaser (0, 0.2f, 55).macro (3, "Phase").mod (macro3, "fxPhaserMix", 50).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Tender Sweep", "FX").set ("osc1On", 0).noise (95, 1, 40).filter (1, ladderBand, 370, 55).env (2, 2200, 2200, 0, 2200, 20)
            .mod (env2, "filter1Cutoff", 20).lfo (1, sawUp, 0.23f, 2).mod (lfo1, "ampPan", 25).amp (2200, 2200, 0, 2200, 40).reverb (35, 75, 6, 70)
            .level (-3).macro (1, "Height").mod (macro1, "filter1Cutoff", 15).macro (2, "Resonance").mod (macro2, "filter1Resonance", 20)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Colour").mod (macro4, "noiseColour", 40);
        add ("Thin Ray", "FX").osc (1, pulse, 10).warp (1, wSync, 50).env (2, 0, 380, 0, 200).mod (env2, "osc1Pitch", -25)
            .mod (env2, "osc1WarpAmount", -60).filter (1, ladder12, 7900, 40).amp (0, 200, 0, 200).drive (hard, 30, 7400)
            .delay (sixteenth, 35, 24, 4300).reverb (24, 70, 5, 70).level (-19).macro (1, "Sync").mod (macro1, "osc1WarpAmount", -35)
            .macro (2, "Sweep").mod (macro2, "osc1Pitch", 25).macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 20).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 35);
        add ("Threadbare Blackout", "FX").osc (1, analog, 30).warp (1, wSync, 55).unison (1, 2, 8, 70).env (2, 0, 2800, 0, 2800)
            .mod (env2, "osc1WarpAmount", 40).mod (env2, "filter1Cutoff", 35).mod (env2, "lfo1Rate", 20).lfo (1, sine, 0.75f)
            .mod (lfo1, "ampLevel", 18).filter (1, svfLow, 440, 15).amp (0, 2800, 0, 900, -10).drive (tube, 35, 5800).reverb (13, 75, 7.5f, 35)
            .level (-28).macro (1, "Sync").mod (macro1, "osc1WarpAmount", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 15)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Wrecked Saucer", "FX").osc (1, analog, trianglePos).osc (2, analog, sinePos, 50).pitch (2, 7, 4).lfo (1, triangle, 17, 0)
            .mod (lfo1, "osc1Pitch", 2.083f).mod (lfo1, "osc2Fine", -45).lfo (2, sine, 0.32f, 0).mod (lfo2, "lfo1Rate", 16).filter (1, svfLow, 2500)
            .amp (400, 400, 100, 300).phaser (55, 0.33f, 45).reverb (30, 90, 3, 35).level (-14).macro (1, "Wobble").mod (macro1, "lfo1Rate", 20)
            .macro (2, "Second").mod (macro2, "osc2Volume", -50).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);

        // Glitch, by name.
        add ("Adrift Shard", "Glitch").osc (1, pulse, 15).env (2, 0, 60, 0, 40, 80).mod (env2, "osc1Position", -25).mod (env2, "filter1Cutoff", 10)
            .filter (1, svfBand, 3400, 20).amp (0, 130, 0, 120).delay (sixteenthT, 78, 40, 9700, false).drive (hard, 20, 3900).level (-9)
            .macro (1, "Repeats").mod (macro1, "fxDelayFeedback", 10).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Skip")
            .mod (macro3, "fxDelayMix", 30).reverb (0, 70, 3, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Ageless Abort", "Glitch").osc (1, sync, 33.33f).warp (1, wCrush, 55).lfoSync (1, randomShape, sixteenth).mod (lfo1, "osc1Pitch", 35)
            .lfoSync (2, randomShape, eighthT).mod (lfo2, "filter1Cutoff", 16).mod (lfo2, "osc1Position", 40).filter (1, ladder12, 2000, 25)
            .amp (2, 300, 100, 200).delay (sixteenth, 30, 18, 4800, false).level (-14).macro (1, "Range").mod (macro1, "osc1Pitch", 20)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Bleak Cursor", "Glitch").osc (1, sync, 50).unison (1, 2, 10, 70).subOsc (25).lfoSync (1, steps, quarter, 1, 30)
            .stepPattern (1, { 100, 60, 20, -20, -60, -100, -60, -20, -20, -60, -100, -60, -20, 20, 60, 100 }).mod (lfo1, "osc1Position", 50)
            .lfoSync (2, randomShape, thirtySecond).mod (lfo2, "filter1Cutoff", 22).filter (1, ladder24, 830, 60).amp (2, 300, 100, 100)
            .drive (tube, 35, 8200).delay (sixteenthT, 45, 12, 2800).level (-20).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20)
            .macro (2, "Scan").mod (macro2, "osc1Position", 30).macro (3, "Resonance").mod (macro3, "filter1Resonance", 25).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 30);
        add ("Circling Parity", "Glitch").osc (1, analog, sinePos, 15).osc (2, analog, trianglePos).warp (2, wFm, 65).lfoSync (1, steps, half)
            .stepPattern (1, { -25, 100, 0, 25, -25, 0, 25, 50, 50, 25, 25, -25, 25, 25, -100, 100 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, randomShape, sixteenth).mod (lfo2, "osc2WarpAmount", 20).mod (lfo2, "filter1Cutoff", 14).filter (1, svfBand, 3100, 50)
            .amp (2, 300, 100, 180).drive (hard, 20, 4200).delay (sixteenth, 40, 16, 2300, false).level (-16).macro (1, "Index")
            .mod (macro1, "osc2WarpAmount", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 18).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Clocked Exception", "Glitch").osc (1, sync, 66.67f).warp (1, wCrush, 40).lfoSync (1, randomShape, eighth).mod (lfo1, "osc1Pitch", 15)
            .lfoSync (2, randomShape, eighth).mod (lfo2, "filter1Cutoff", 12).mod (lfo2, "osc1Position", 40).filter (1, ladderBand, 2400, 40)
            .amp (2, 300, 100, 140).drive (hard, 25, 6600).delay (sixteenthT, 25, 18, 3100).level (-16).macro (1, "Range")
            .mod (macro1, "osc1Pitch", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).reverb (0, 70, 3, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Compiled Modem", "Glitch").osc (1, analog, sinePos, 15).osc (2, analog, sinePos).warp (2, wFm, 50).lfoSync (1, steps, half)
            .stepPattern (1, { -100, 100, 25, 0, 0, 0, -25, -25, -100, -100, 50, -100, -100, 25, 50, -100 }).mod (lfo1, "osc1Pitch", 35)
            .lfoSync (2, randomShape, sixteenth).mod (lfo2, "osc2WarpAmount", 30).mod (lfo2, "filter1Cutoff", 12).filter (1, ladder12, 2200, 50)
            .amp (2, 300, 100, 160).delay (sixteenth, 50, 22, 4100, false).level (-16).macro (1, "Index").mod (macro1, "osc2WarpAmount", 25)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 18).macro (3, "Ratio").mod (macro3, "osc1Pitch", 20).drive (crush, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Cornered Hitch", "Glitch").osc (1, pulse, 50).unison (1, 2, 12, 60).lfoSync (1, steps, half, 1, 88)
            .stepPattern (1, { 0, 0, -40, -100, -100, -40, -100, -100, 0, 0, -40, -100, -100, -40, -100, -100 }).mod (lfo1, "ampLevel", 83)
            .lfoSync (2, steps, quarter).stepPattern (2, { -100, 50, 50, 50, 50, -100, 100, 50, 100, -100, 100, -100, -50, 0, 0, 50 })
            .mod (lfo2, "osc1Position", 30).mod (lfo2, "filter1Cutoff", 11).filter (1, svfLow, 4400, 55).amp (2, 300, 100, 80).level (-16)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Cut").mod (macro2, "ampLevel", 14).drive (crush, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).reverb (0, 70, 3, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Corrupt Fragment", "Glitch").osc (1, sync, 55).noise (25, 3).env (2, 0, 40, 0, 40, 80).mod (env2, "osc1Position", 40)
            .mod (env2, "filter1Cutoff", 10).filter (1, svfHigh, 3800, 50).amp (0, 70, 0, 90).delay (thirtySecond, 84, 45, 8200)
            .drive (hard, 20, 4000).level (-2).macro (1, "Repeats").mod (macro1, "fxDelayFeedback", 10).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Creeping Raster", "Glitch").osc (1, sync, 50).unison (1, 2, 8, 70).lfoSync (1, steps, bar1, 1, 40)
            .stepPattern (1, { -100, -75, -50, -25, 0, 25, 50, 75, 75, 50, 25, 0, -25, -50, -75, -100 }).mod (lfo1, "osc1Position", 50)
            .lfoSync (2, randomShape, eighth).mod (lfo2, "filter1Cutoff", 22).filter (1, svfBand, 1500, 30).amp (2, 300, 100, 180)
            .drive (crush, 30, 4800).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Scan")
            .mod (macro2, "osc1Position", 30).delay (sixteenth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).reverb (0, 70, 3, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Derelict Trace", "Glitch").osc (1, sync, 50).unison (1, 2, 16, 70).lfoSync (1, steps, half, 1, 50)
            .stepPattern (1, { -100, 100, -60, 60, -20, 20, 0, 0, 0, 0, 20, -20, 60, -60, 100, -100 }).mod (lfo1, "osc1Position", 45)
            .lfoSync (2, randomShape, eighth).mod (lfo2, "filter1Cutoff", 20).filter (1, ladder12, 760, 55).amp (2, 300, 100, 60)
            .drive (hard, 30, 8200).level (-22).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Scan")
            .mod (macro2, "osc1Position", 30).delay (sixteenth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 25);
        add ("Fallen Remnant", "Glitch").osc (1, analog, 90).warp (1, wCrush, 30).noise (40, 3, 20).lfo (1, randomShape, 22, 0)
            .mod (lfo1, "osc1WarpAmount", 30).mod (lfo1, "noiseColour", 40).lfo (2, drift, 0.62f, 0).mod (lfo2, "filter1Cutoff", 8)
            .filter (1, svfHigh, 1800, 45).amp (200, 300, 100, 100).drive (crush, 45, 7000).level (-12).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 20).macro (2, "Rot").mod (macro2, "osc1WarpAmount", 30).reverb (0, 70, 3, 45).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Hash").mod (macro4, "noiseLevel", -40);
        add ("Fenced Error", "Glitch").osc (1, analog, 20).warp (1, wCrush, 60).lfoSync (1, randomShape, sixteenth).mod (lfo1, "osc1Pitch", 15)
            .lfoSync (2, randomShape, eighth).mod (lfo2, "filter1Cutoff", 22).mod (lfo2, "osc1Position", 10).filter (1, ladder12, 4600, 35)
            .amp (2, 300, 100, 120).delay (sixteenthT, 30, 20, 5300, false).level (-12).macro (1, "Range").mod (macro1, "osc1Pitch", 20)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 35);
        add ("Fugitive Protocol", "Glitch").osc (1, analog, sinePos, 1).osc (2, analog, sinePos).warp (2, wFm, 50).lfoSync (1, steps, bars2)
            .stepPattern (1, { 50, 100, 0, 0, -50, 25, 100, 50, 100, 25, -25, -100, 50, 25, -100, -50 }).mod (lfo1, "osc1Pitch", 20)
            .lfoSync (2, randomShape, thirtySecond).mod (lfo2, "osc2WarpAmount", 10).mod (lfo2, "filter1Cutoff", 14).filter (1, svfBand, 3200, 45)
            .amp (2, 300, 100, 100).drive (crush, 20, 3800).level (-12).macro (1, "Index").mod (macro1, "osc2WarpAmount", 25).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 18).macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).reverb (0, 70, 3, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Glitched Skip", "Glitch").osc (1, analog, trianglePos).env (2, 0, 80, 0, 40, 80).mod (env2, "osc1Position", -30)
            .mod (env2, "filter1Cutoff", 22).filter (1, svfLow, 790, 20).amp (0, 50, 0, 110, 70).delay (sixtyFourth, 74, 40).drive (crush, 15, 3000)
            .level (-10).macro (1, "Repeats").mod (macro1, "fxDelayFeedback", 10).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Grain").mod (macro4, "osc1Position", 30);
        add ("Glitched Trace", "Glitch").osc (1, sync, 50).unison (1, 3, 6, 70).subOsc (35).lfoSync (1, steps, bar1, 1, 40)
            .stepPattern (1, { 100, 60, 20, -20, -60, -100, -60, -20, 100, 60, 20, -20, -60, -100, -60, -20 }).mod (lfo1, "osc1Position", 50)
            .lfoSync (2, randomShape, thirtySecond).mod (lfo2, "filter1Cutoff", 18).filter (1, svfBand, 4000, 40).amp (2, 300, 100, 180)
            .delay (sixteenthT, 55, 22, 5200).level (-13).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Scan")
            .mod (macro2, "osc1Position", 30).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (crush, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Grey Jitter", "Glitch").osc (1, sync, 50).unison (1, 1, 10, 60).lfoSync (1, steps, half, 1, 88)
            .stepPattern (1, { 0, -40, -100, -100, -40, 0, 0, -100, 0, -40, -100, -100, -40, 0, 0, -100 }).mod (lfo1, "ampLevel", 83)
            .lfoSync (2, steps, half).stepPattern (2, { 0, 50, 0, 50, -50, 0, 0, 0, 100, -50, 50, 50, 50, -100, 0, 100 })
            .mod (lfo2, "osc1Position", 48).mod (lfo2, "filter1Cutoff", 6).filter (1, svfBand, 3600, 25).amp (2, 300, 100, 80).level (-9)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Cut").mod (macro2, "ampLevel", 14).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Jump").mod (macro4, "osc1Position", 30);
        add ("Heartless Hiccup", "Glitch").osc (1, metal, 50).unison (1, 1, 14, 60).lfoSync (1, steps, quarter, 1, 88)
            .stepPattern (1, { 0, -40, -100, 0, -100, -40, -100, -40, 0, -40, -100, 0, -100, -40, -100, -40 }).mod (lfo1, "ampLevel", 85)
            .lfoSync (2, steps, half).stepPattern (2, { -50, 0, -100, 0, 50, -50, 0, 100, 100, 0, -100, 100, 50, -100, -100, -50 })
            .mod (lfo2, "osc1Position", 48).mod (lfo2, "filter1Cutoff", 16).filter (1, ladder24, 3100, 50).amp (2, 300, 100, 200)
            .delay (thirtySecond, 50, 12, 4300).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Cut")
            .mod (macro2, "ampLevel", 14).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Jump")
            .mod (macro4, "osc1Position", 30);
        add ("Icy Corruption", "Glitch").osc (1, vowel, 90).warp (1, wCrush, 50).noise (55, 3, 20).lfo (1, randomShape, 4, 0)
            .mod (lfo1, "osc1WarpAmount", 40).mod (lfo1, "noiseColour", 40).lfo (2, randomShape, 1.1f, 0).mod (lfo2, "filter1Cutoff", 10)
            .filter (1, ladder12, 4900, 25).amp (200, 300, 100, 550).drive (crush, 45, 3000).reverb (20, 70, 4, 30).level (-11).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 20).macro (2, "Rot").mod (macro2, "osc1WarpAmount", 30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Crush").mod (macro4, "fxDriveAmount", 35);
        add ("Ivory Hitch", "Glitch").osc (1, vowel, 50).unison (1, 1, 12, 60).lfoSync (1, steps, bar1, 1, 76)
            .stepPattern (1, { 0, -100, -100, 0, -100, 0, 0, -100, 0, -100, -100, 0, -100, 0, 0, -100 }).mod (lfo1, "ampLevel", 83)
            .lfoSync (2, steps, bar1).stepPattern (2, { 50, -50, -100, 100, 0, 0, 50, 0, 100, 50, -50, 50, 0, -50, -100, 50 })
            .mod (lfo2, "osc1Position", 45).mod (lfo2, "filter1Cutoff", 15).filter (1, svfBand, 1600, 15).amp (2, 300, 100, 120)
            .delay (sixteenth, 50, 22, 4900).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Cut")
            .mod (macro2, "ampLevel", 14).macro (3, "Jump").mod (macro3, "osc1Position", 30).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Logic Pointer", "Glitch").osc (1, sync, 50).unison (1, 3, 6, 70).subOsc (30).lfoSync (1, steps, bar1, 1, 30)
            .stepPattern (1, { -100, 100, -60, 60, -20, 20, 0, 0, 0, 0, 20, -20, 60, -60, 100, -100 }).mod (lfo1, "osc1Position", 35)
            .lfoSync (2, randomShape, thirtySecond).mod (lfo2, "filter1Cutoff", 20).filter (1, ladder24, 2900, 55).amp (2, 300, 100, 60)
            .drive (crush, 20, 4600).level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Scan")
            .mod (macro2, "osc1Position", 30).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).reverb (0, 70, 3, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Looping Jitter", "Glitch").osc (1, pulse, 50).unison (1, 1, 6, 60).lfoSync (1, steps, half, 1, 70)
            .stepPattern (1, { 0, 0, -100, 0, -100, 0, -100, -40, 0, 0, -100, 0, -100, 0, -100, -40 }).mod (lfo1, "ampLevel", 80)
            .lfoSync (2, steps, bar1).stepPattern (2, { 50, -50, -50, 50, 0, 50, 50, -50, -100, 50, -50, 50, -100, -100, 0, -100 })
            .mod (lfo2, "osc1Position", 33).mod (lfo2, "filter1Cutoff", 13).filter (1, svfBand, 2900, 30).amp (2, 300, 100, 80)
            .drive (hard, 35, 8100).delay (sixteenthT, 40, 16, 3800).level (-19).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20)
            .macro (2, "Cut").mod (macro2, "ampLevel", 14).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Lurching Bitrot", "Glitch").osc (1, fm, 40).warp (1, wCrush, 55).noise (25, 3, -20).lfo (1, randomShape, 6.2f, 0)
            .mod (lfo1, "osc1WarpAmount", 30).mod (lfo1, "noiseColour", 35).lfo (2, randomShape, 1.1f, 0).mod (lfo2, "filter1Cutoff", 8)
            .filter (1, svfLow, 710, 50).amp (20, 300, 100, 250).drive (crush, 30, 8100).reverb (10, 90, 3, 70).level (-14).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 20).macro (2, "Rot").mod (macro2, "osc1WarpAmount", 30).macro (3, "Hash").mod (macro3, "noiseLevel", -40)
            .macro (4, "Crush").mod (macro4, "fxDriveAmount", 35);
        add ("Manganese Hiccup", "Glitch").osc (1, metal, 50).unison (1, 2, 14, 60).lfoSync (1, steps, bar1, 1, 76)
            .stepPattern (1, { 0, -100, -100, -100, 0, 0, 0, -100, 0, -100, -100, -100, 0, 0, 0, -100 }).mod (lfo1, "ampLevel", 82)
            .lfoSync (2, steps, quarter).stepPattern (2, { 100, 50, 100, -100, 50, -50, 0, 100, 0, 50, -100, 100, 50, 0, -100, -50 })
            .mod (lfo2, "osc1Position", 33).mod (lfo2, "filter1Cutoff", 14).filter (1, ladder12, 1600, 30).amp (2, 300, 100, 40).level (-6)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Cut").mod (macro2, "ampLevel", 14).reverb (0, 70, 3, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (crush, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Marching Cursor", "Glitch").osc (1, sync, 50).unison (1, 2, 16, 70).subOsc (40).lfoSync (1, steps, quarter, 1, 20)
            .stepPattern (1, { -100, -75, -50, -25, 0, 25, 50, 75, -100, -75, -50, -25, 0, 25, 50, 75 }).mod (lfo1, "osc1Position", 45)
            .lfoSync (2, randomShape, eighthT).mod (lfo2, "filter1Cutoff", 10).filter (1, ladderBand, 1400, 35).amp (2, 300, 100, 40)
            .drive (hard, 35, 3800).level (-22).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Scan")
            .mod (macro2, "osc1Position", 30).macro (3, "Resonance").mod (macro3, "filter1Resonance", 25).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Marching Dropout", "Glitch").osc (1, fm, 60).warp (1, wCrush, 60).noise (25, 3, 40).lfo (1, randomShape, 7.3f, 0)
            .mod (lfo1, "osc1WarpAmount", 30).mod (lfo1, "noiseColour", 40).lfo (2, randomShape, 1.9f, 0).mod (lfo2, "filter1Cutoff", 8)
            .filter (1, svfLow, 2600, 15).amp (200, 300, 100, 750).drive (crush, 30, 4300).level (-13).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 20).macro (2, "Rot").mod (macro2, "osc1WarpAmount", 30).macro (3, "Speed").mod (macro3, "lfo1Rate", 20)
            .reverb (0, 70, 3, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Masked Overflow", "Glitch").osc (1, analog, trianglePos).warp (1, wCrush, 50).lfoSync (1, randomShape, thirtySecond)
            .mod (lfo1, "osc1Pitch", 25).lfoSync (2, randomShape, eighthT).mod (lfo2, "filter1Cutoff", 18).mod (lfo2, "osc1Position", 40)
            .filter (1, svfBand, 2300, 35).amp (2, 300, 100, 60).drive (crush, 40, 9600).delay (sixteenth, 50, 10, 4100).level (-9)
            .macro (1, "Range").mod (macro1, "osc1Pitch", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Steps")
            .mod (macro3, "osc1WarpAmount", 30).reverb (0, 70, 3, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Overnight Slice", "Glitch").osc (1, metal, 50).env (2, 0, 50, 0, 40, 80).mod (env2, "osc1Position", -30)
            .mod (env2, "filter1Cutoff", 10).filter (1, ladder12, 1200, 40).amp (0, 150, 0, 70, 80).delay (sixteenthT, 76, 40, 9000, false)
            .drive (hard, 25, 3500).level (-9).macro (1, "Repeats").mod (macro1, "fxDelayFeedback", 10).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).macro (3, "Grain").mod (macro3, "osc1Position", 30).reverb (0, 70, 3, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Plain Corruption", "Glitch").osc (1, fm, 80).warp (1, wCrush, 35).noise (35, 3, -20).lfo (1, randomShape, 9.7f, 0)
            .mod (lfo1, "osc1WarpAmount", 40).mod (lfo1, "noiseColour", 15).lfo (2, drift, 1.4f, 0).mod (lfo2, "filter1Cutoff", 8)
            .filter (1, svfBand, 1000, 40).amp (20, 300, 100, 300).drive (crush, 25, 9700).level (-13).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 20).macro (2, "Rot").mod (macro2, "osc1WarpAmount", 30).macro (3, "Speed").mod (macro3, "lfo1Rate", 20)
            .reverb (0, 70, 3, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Quarantine Hiccup", "Glitch").osc (1, fm, 50).unison (1, 1, 6, 60).lfoSync (1, steps, half, 1, 82)
            .stepPattern (1, { 0, 0, -100, 0, -40, 0, 0, -40, -100, -100, -100, -100, -100, -100, -100, -100 }).mod (lfo1, "ampLevel", 73)
            .lfoSync (2, steps, half).stepPattern (2, { -50, 50, 0, -100, -50, -50, -50, 50, -50, -100, 100, -50, 50, 50, 0, 100 })
            .mod (lfo2, "osc1Position", 48).mod (lfo2, "filter1Cutoff", 6).filter (1, ladder24, 3000, 15).amp (2, 300, 100, 80)
            .drive (tube, 40, 6200).delay (sixteenth, 45, 12, 5400).level (-21).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20)
            .macro (2, "Cut").mod (macro2, "ampLevel", 14).macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).reverb (0, 70, 3, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Rancid Dropout", "Glitch").osc (1, additive, 80).warp (1, wCrush, 55).noise (50, 3, 20).lfo (1, randomShape, 8.2f, 0)
            .mod (lfo1, "osc1WarpAmount", 25).mod (lfo1, "noiseColour", 40).lfo (2, drift, 0.59f, 0).mod (lfo2, "filter1Cutoff", 8)
            .filter (1, svfLow, 830, 40).amp (200, 300, 100, 150).drive (crush, 45, 7100).level (-9).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 20).macro (2, "Rot").mod (macro2, "osc1WarpAmount", 30).macro (3, "Speed").mod (macro3, "lfo1Rate", 20)
            .macro (4, "Hash").mod (macro4, "noiseLevel", -40);
        add ("Relentless Hiccup", "Glitch").osc (1, vowel, 50).unison (1, 2, 14, 60).lfoSync (1, steps, bar1, 1, 82)
            .stepPattern (1, { 0, -100, -40, -100, -100, 0, 0, -100, 0, -100, -40, -100, -100, 0, 0, -100 }).mod (lfo1, "ampLevel", 75)
            .lfoSync (2, steps, bar1).stepPattern (2, { -100, 100, 0, 100, 0, 0, 100, 50, 100, 100, 50, -100, -50, -50, 100, 50 })
            .mod (lfo2, "osc1Position", 30).mod (lfo2, "filter1Cutoff", 6).filter (1, svfLow, 4800, 40).amp (2, 300, 100, 120)
            .delay (thirtySecond, 50, 20, 3300).level (-12).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Cut")
            .mod (macro2, "ampLevel", 14).drive (crush, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).reverb (0, 70, 3, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Restless Exception", "Glitch").osc (1, analog, 20).warp (1, wCrush, 55).lfoSync (1, randomShape, thirtySecond)
            .mod (lfo1, "osc1Pitch", 50).lfoSync (2, randomShape, eighth).mod (lfo2, "filter1Cutoff", 18).mod (lfo2, "osc1Position", 10)
            .filter (1, svfBand, 1000, 20).amp (2, 300, 100, 180).drive (hard, 30, 3700).delay (sixteenth, 45, 24, 6000).level (-21)
            .macro (1, "Range").mod (macro1, "osc1Pitch", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).reverb (0, 70, 3, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Solar Scratch", "Glitch").osc (1, analog, trianglePos).env (2, 0, 40, 0, 40, 80).mod (env2, "osc1Position", -20)
            .mod (env2, "filter1Cutoff", 10).filter (1, svfBand, 2000, 25).amp (0, 150, 0, 110, 70).delay (thirtySecond, 78, 45, 11000)
            .drive (hard, 30, 3500).level (4).macro (1, "Repeats").mod (macro1, "fxDelayFeedback", 10).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).macro (3, "Skip").mod (macro3, "fxDelayMix", 30).macro (4, "Grain").mod (macro4, "osc1Position", 30);
        add ("Southern Trace", "Glitch").osc (1, sync, 50).unison (1, 3, 6, 70).lfoSync (1, steps, quarter, 1, 20)
            .stepPattern (1, { -100, 100, -60, 60, -20, 20, 0, 0, 0, 0, 20, -20, 60, -60, 100, -100 }).mod (lfo1, "osc1Position", 45)
            .lfoSync (2, randomShape, thirtySecond).mod (lfo2, "filter1Cutoff", 12).filter (1, ladderBand, 2800, 25).amp (2, 300, 100, 200)
            .level (-10).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Scan").mod (macro2, "osc1Position", 30)
            .reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (crush, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Splintered Underrun", "Glitch").osc (1, analog, 40).warp (1, wCrush, 55).lfoSync (1, randomShape, thirtySecond)
            .mod (lfo1, "osc1Pitch", 45).lfoSync (2, randomShape, eighthT).mod (lfo2, "filter1Cutoff", 16).mod (lfo2, "osc1Position", 10)
            .filter (1, ladder12, 2500, 20).amp (2, 300, 100, 180).drive (hard, 45, 3800).delay (sixteenth, 30, 10, 2600).level (-24)
            .macro (1, "Range").mod (macro1, "osc1Pitch", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Steps")
            .mod (macro3, "osc1WarpAmount", 30).macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Sprawling Shard", "Glitch").osc (1, sync, 20).env (2, 0, 90, 0, 40, 80).mod (env2, "osc1Position", -30)
            .mod (env2, "filter1Cutoff", 16).filter (1, ladder12, 1500, 30).amp (0, 50, 0, 90).delay (thirtySecond, 84, 45, 4400, false).level (-8)
            .macro (1, "Repeats").mod (macro1, "fxDelayFeedback", 10).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).macro (3, "Skip")
            .mod (macro3, "fxDelayMix", 30).macro (4, "Grain").mod (macro4, "osc1Position", 30);
        add ("Squat Handshake", "Glitch").osc (1, analog, sinePos, 15).osc (2, analog, sinePos).warp (2, wFm, 40).lfoSync (1, steps, half)
            .stepPattern (1, { -100, -50, 0, -50, 100, -50, 25, -100, 0, -50, 100, -25, 0, 0, 50, 100 }).mod (lfo1, "osc1Pitch", 30)
            .lfoSync (2, randomShape, eighth).mod (lfo2, "osc2WarpAmount", 10).mod (lfo2, "filter1Cutoff", 12).filter (1, ladder12, 1900, 45)
            .amp (2, 300, 100, 100).drive (hard, 25, 8200).level (-23).macro (1, "Index").mod (macro1, "osc2WarpAmount", 25).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 18).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 30);
        add ("Stalking Bitrot", "Glitch").osc (1, vowel, 50).warp (1, wCrush, 40).noise (40, 3).lfo (1, randomShape, 5, 0)
            .mod (lfo1, "osc1WarpAmount", 20).mod (lfo1, "noiseColour", 15).lfo (2, drift, 2, 0).mod (lfo2, "filter1Cutoff", 12)
            .filter (1, svfBand, 2500, 25).amp (2, 300, 100, 100).drive (crush, 30, 7900).reverb (15, 95, 7.5f, 45).level (-11).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 20).macro (2, "Rot").mod (macro2, "osc1WarpAmount", 30).macro (3, "Speed").mod (macro3, "lfo1Rate", 20)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Stubby Remnant", "Glitch").osc (1, vowel, 80).warp (1, wCrush, 50).noise (30, 3, 40).lfo (1, randomShape, 4.8f, 0)
            .mod (lfo1, "osc1WarpAmount", 30).mod (lfo1, "noiseColour", 30).lfo (2, randomShape, 0.64f, 0).mod (lfo2, "filter1Cutoff", 16)
            .filter (1, svfHigh, 2900, 25).amp (200, 300, 100, 750).drive (crush, 45, 4600).level (-6).macro (1, "Cutoff")
            .mod (macro1, "filter1Cutoff", 20).macro (2, "Rot").mod (macro2, "osc1WarpAmount", 30).macro (3, "Hash").mod (macro3, "noiseLevel", -40)
            .macro (4, "Speed").mod (macro4, "lfo1Rate", 20);
        add ("Stuttering Protocol", "Glitch").osc (1, analog, sinePos, 1).osc (2, analog, trianglePos).warp (2, wFm, 25).lfoSync (1, steps, bar1)
            .stepPattern (1, { 100, 100, 50, 50, 25, 0, 25, -25, 100, -25, 0, 100, -25, 50, 0, 0 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, randomShape, eighth).mod (lfo2, "osc2WarpAmount", 20).mod (lfo2, "filter1Cutoff", 12).filter (1, svfBand, 1200, 30)
            .amp (2, 300, 100, 200).delay (sixteenthT, 55, 14, 5600).level (-13).macro (1, "Index").mod (macro1, "osc2WarpAmount", 25)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 18).reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35)
            .macro (4, "Resonance").mod (macro4, "filter1Resonance", 30);
        add ("Sublevel Timeout", "Glitch").osc (1, fm, 40).warp (1, wCrush, 65).lfoSync (1, randomShape, sixteenth).mod (lfo1, "osc1Pitch", 20)
            .lfoSync (2, randomShape, thirtySecond).mod (lfo2, "filter1Cutoff", 10).mod (lfo2, "osc1Position", 40).filter (1, ladderBand, 1800, 55)
            .amp (2, 300, 100, 120).delay (sixteenth, 25, 18, 5300).level (-13).macro (1, "Range").mod (macro1, "osc1Pitch", 20).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 20).macro (3, "Resonance").mod (macro3, "filter1Resonance", 30).reverb (0, 70, 3, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Synthetic Exception", "Glitch").osc (1, fm, 40).warp (1, wCrush, 60).lfoSync (1, randomShape, sixteenthT).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, randomShape, thirtySecond).mod (lfo2, "filter1Cutoff", 20).mod (lfo2, "osc1Position", 40).filter (1, ladderBand, 4000, 60)
            .amp (2, 300, 100, 180).drive (hard, 20, 4000).delay (eighthT, 35, 20, 6900, false).level (-12).macro (1, "Range")
            .mod (macro1, "osc1Pitch", 20).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).reverb (0, 70, 3, 45).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Steps").mod (macro4, "osc1WarpAmount", 30);
        add ("Terse Parity", "Glitch").osc (1, analog, sinePos, 15).osc (2, analog, trianglePos).warp (2, wFm, 60).lfoSync (1, steps, bar1)
            .stepPattern (1, { -100, -100, 100, 25, 0, 50, 0, 25, 0, -100, 50, 100, 0, -50, 25, 25 }).mod (lfo1, "osc1Pitch", 40)
            .lfoSync (2, randomShape, sixteenth).mod (lfo2, "osc2WarpAmount", 10).mod (lfo2, "filter1Cutoff", 12).filter (1, ladderBand, 1600, 25)
            .amp (2, 300, 100, 180).level (-13).macro (1, "Index").mod (macro1, "osc2WarpAmount", 25).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 18).drive (crush, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Ratio")
            .mod (macro4, "osc1Pitch", 20);
        add ("Terse Raster", "Glitch").osc (1, sync, 50).unison (1, 1, 8, 70).lfoSync (1, steps, half, 1, 40)
            .stepPattern (1, { 100, 60, 20, -20, -60, -100, -60, -20, 100, 60, 20, -20, -60, -100, -60, -20 }).mod (lfo1, "osc1Position", 50)
            .lfoSync (2, randomShape, eighthT).mod (lfo2, "filter1Cutoff", 18).filter (1, ladder24, 3500, 30).amp (2, 300, 100, 40).level (-11)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Scan").mod (macro2, "osc1Position", 30).drive (crush, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).delay (sixteenth, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Turning Carrier", "Glitch").osc (1, analog, sinePos, 1).osc (2, analog, sinePos).warp (2, wFm, 55).lfoSync (1, steps, bars2)
            .stepPattern (1, { 50, -100, 50, 0, 25, 25, 50, 0, -25, -25, -25, -25, 50, 25, 100, 25 }).mod (lfo1, "osc1Pitch", 30)
            .lfoSync (2, randomShape, sixteenth).mod (lfo2, "osc2WarpAmount", 25).mod (lfo2, "filter1Cutoff", 6).filter (1, svfBand, 1800, 50)
            .amp (2, 300, 100, 100).drive (hard, 30, 6100).delay (thirtySecond, 50, 10, 2100).level (-19).macro (1, "Index")
            .mod (macro1, "osc2WarpAmount", 25).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 18).macro (3, "Resonance")
            .mod (macro3, "filter1Resonance", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Vaulted Slice", "Glitch").osc (1, sync, 45).noise (35, 2).env (2, 0, 60, 0, 40, 80).mod (env2, "osc1Position", 20)
            .mod (env2, "filter1Cutoff", 14).filter (1, svfHigh, 700, 35).amp (0, 90, 0, 100, 80).delay (sixtyFourth, 74, 50, 4100).level (-5)
            .macro (1, "Repeats").mod (macro1, "fxDelayFeedback", 10).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20).reverb (0, 70, 3, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (crush, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Veiled Scratch", "Glitch").osc (1, metal, 45).env (2, 0, 90, 0, 40, 80).mod (env2, "osc1Position", -40)
            .mod (env2, "filter1Cutoff", 12).filter (1, svfLow, 810, 55).amp (0, 50, 0, 120, 80).delay (thirtySecond, 76, 40, 3400, false)
            .level (-5).macro (1, "Repeats").mod (macro1, "fxDelayFeedback", 10).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 20)
            .reverb (0, 70, 3, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (crush, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Voiceless Modem", "Glitch").osc (1, analog, sinePos, 15).osc (2, analog, trianglePos).warp (2, wFm, 40).lfoSync (1, steps, quarter)
            .stepPattern (1, { 50, 100, 25, 100, 50, -50, -50, -50, 100, 100, -100, -100, 50, -25, -25, 25 }).mod (lfo1, "osc1Pitch", 50)
            .lfoSync (2, randomShape, eighth).mod (lfo2, "osc2WarpAmount", 30).mod (lfo2, "filter1Cutoff", 14).filter (1, ladder12, 1800, 55)
            .amp (2, 300, 100, 80).level (-16).macro (1, "Index").mod (macro1, "osc2WarpAmount", 25).macro (2, "Cutoff")
            .mod (macro2, "filter1Cutoff", 18).macro (3, "Ratio").mod (macro3, "osc1Pitch", 20).reverb (0, 70, 3, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);

        // Game, by name.
        add ("Amber Unlock", "Game").osc (1, analog, sinePos).osc (2, analog, sinePos, 50).pitch (2, 12).lfo (1, steps, 1.33f, 2)
            .stepPattern (1, { 0, 0, 0, 0, 16.67f, 16.67f, 16.67f, 16.67f, 50, 50, 50, 50, 50, 50, 50, 50 }).mod (lfo1, "osc1Pitch", 50)
            .mod (lfo1, "osc2Pitch", 50).filter (1, svfLow, 4800, 5).amp (0, 300, 0, 300, 45).reverb (26, 85, 3, 30).level (-14).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).chorus (0, 0.5f, 50).macro (3, "Wide")
            .mod (macro3, "fxChorusMix", 45).delay (sixteenth, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Arctic Confirm", "Game").osc (1, analog, 15).osc (2, analog, sinePos, 40).pitch (2, 12).lfo (1, steps, 1.43f, 2)
            .stepPattern (1, { 0, 0, 0, 0, 16.67f, 16.67f, 16.67f, 16.67f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).filter (1, ladder12, 10000, 5).amp (0, 450, 0, 500, 40).reverb (27, 100, 9, 50)
            .level (-11).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).chorus (0, 0.5f, 50)
            .macro (3, "Wide").mod (macro3, "fxChorusMix", 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Asphalt Locked", "Game").osc (1, analog, sawPos).pitch (1, -24).osc (2, analog, squarePos, 60).pitch (2, -23).lfo (1, steps, 4, 2, 90)
            .stepPattern (1, { 0, 0, 0, -100, 0, 0, 0, -100, -100, -100, -100, -100, -100, -100, -100, -100 }).mod (lfo1, "ampLevel", 75)
            .filter (1, svfBand, 1300, 30).amp (0, 300, 100, 140).reverb (25, 40, 1.4f, 40).level (-8).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", 20).macro (2, "Speed").mod (macro2, "lfo1Rate", 12).drive (crush, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Bleached Checkpoint", "Game").osc (1, analog, trianglePos).osc (2, analog, sinePos, 55).pitch (2, 12).lfo (1, steps, 2.86f, 2)
            .stepPattern (1, { 0, 0, 0, 0, 16.67f, 16.67f, 16.67f, 16.67f, 37.5f, 37.5f, 37.5f, 37.5f, 37.5f, 37.5f, 37.5f, 37.5f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).filter (1, ladder12, 4000, 5).amp (5, 650, 0, 400, 40).reverb (29, 75, 7, 40)
            .level (-13).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15)
            .delay (sixteenth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Blunt Bound", "Game").osc (1, analog, trianglePos).pitch (1, -12).env (2, 120, 140, 100, 100, 20).mod (env2, "osc1Pitch", 35.417f)
            .filter (1, ladder12, 6600, 5).amp (0, 160, 0, 160).level (-9).macro (1, "Height").mod (macro1, "osc1Pitch", 14.583f).macro (2, "Tone")
            .mod (macro2, "filter1Cutoff", -30).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 45).reverb (0, 50, 1.2f, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Border Error", "Game").osc (1, pulse, 40).pitch (1, -19).lfo (1, steps, 3.75f, 2, 80)
            .stepPattern (1, { 0, 0, 0, -100, 0, 0, 0, -100, -100, -100, -100, -100, -100, -100, -100, -100 }).mod (lfo1, "ampLevel", 87)
            .filter (1, ladder12, 1300, 25).amp (0, 300, 100, 90).level (-11).macro (1, "Tone").mod (macro1, "filter1Cutoff", 20).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 12).drive (crush, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).reverb (0, 50, 1.2f, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Bound Hazard", "Game").osc (1, analog, squarePos).lfo (1, steps, 2.5f, 1, 90)
            .stepPattern (1, { 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100 }).mod (lfo1, "ampLevel", 83)
            .lfo (2, square, 2).mod (lfo2, "osc1Pitch", 25).filter (1, svfBand, 4200).amp (2, 300, 100, 120).delay (eighth, 25, 12, 2400)
            .level (-11).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).drive (crush, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Boxed Detonation", "Game").set ("osc1On", 0).noise (100, 3).env (2, 0, 1500, 0, 400, 60).mod (env2, "noiseColour", 40)
            .mod (env2, "filter1Cutoff", 20).filter (1, ladder12, 1000, 5).subOsc (55).amp (0, 1650, 0, 600, 60).level (-11).macro (1, "Size")
            .mod (macro1, "noiseColour", -35).macro (2, "Tone").mod (macro2, "filter1Cutoff", 20).drive (crush, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).reverb (0, 55, 1.6f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Brittle Accept", "Game").osc (1, analog, sinePos).osc (2, analog, sinePos, 65).pitch (2, 12).lfo (1, steps, 1.82f, 2)
            .stepPattern (1, { 0, 0, 0, 0, 16.67f, 16.67f, 16.67f, 16.67f, 50, 50, 50, 50, 50, 50, 50, 50 }).mod (lfo1, "osc1Pitch", 50)
            .mod (lfo1, "osc2Pitch", 50).filter (1, svfLow, 6400, 5).amp (2, 450, 0, 450, 60).reverb (12, 75, 6.5f, 30).level (-16)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).chorus (0, 0.5f, 50)
            .macro (3, "Wide").mod (macro3, "fxChorusMix", 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Buffered Blaster", "Game").osc (1, pulse, 53).pitch (1, -5).env (2, 0, 250, 0, 100, 90).mod (env2, "osc1Pitch", 64.583f)
            .noise (45, 3, 60).filter (1, ladder12, 7200, 25).amp (0, 140, 0, 200, 80).drive (crush, 30, 7900).level (-11).macro (1, "Fall")
            .mod (macro1, "osc1Pitch", 25).macro (2, "Tone").mod (macro2, "filter1Cutoff", -30).reverb (0, 50, 1.2f, 45).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 40).chorus (0, 0.5f, 50).macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Buffered Danger", "Game").osc (1, pulse, 53).lfo (1, steps, 1.25f, 1, 80)
            .stepPattern (1, { 0, 0, -100, -100, 0, 0, -100, -100, 0, 0, -100, -100, 0, 0, -100, -100 }).mod (lfo1, "ampLevel", 75)
            .lfo (2, square, 1.5f).mod (lfo2, "osc1Pitch", 10.417f).filter (1, svfLow, 4000).amp (2, 300, 100, 130).level (-13).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).delay (sixteenth, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).drive (crush, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Clammy Token", "Game").osc (1, pulse, 0).lfo (1, steps, 2.22f, 2)
            .stepPattern (1, { 0, 0, 0, 0, 20.83f, 20.83f, 20.83f, 20.83f, 20.83f, 20.83f, 20.83f, 20.83f, 20.83f, 20.83f, 20.83f, 20.83f })
            .mod (lfo1, "osc1Pitch", 50).filter (1, ladder12, 6600, 5).amp (0, 450, 0, 200, 70).level (-12).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).drive (crush, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Coded Danger", "Game").osc (1, pulse, 0).lfo (1, steps, 1, 1, 80)
            .stepPattern (1, { 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100 }).mod (lfo1, "ampLevel", 87)
            .lfo (2, square, 0.5f).mod (lfo2, "osc1Pitch", 14.583f).filter (1, svfLow, 7200, 25).amp (2, 300, 100, 140)
            .delay (eighth, 30, 10, 6300, false).level (-15).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 15).drive (crush, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).reverb (0, 50, 1.2f, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Corrupt Wreck", "Game").set ("osc1On", 0).noise (80, 3, 60).env (2, 0, 1450, 0, 400, 20).mod (env2, "noiseColour", 50)
            .mod (env2, "filter1Cutoff", 15).filter (1, ladder12, 3200).subOsc (50).amp (0, 350, 0, 700, 40).reverb (20, 45, 1.2f, 50).level (-9)
            .macro (1, "Size").mod (macro1, "noiseColour", -35).macro (2, "Tone").mod (macro2, "filter1Cutoff", 20).drive (crush, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Creeping Charge", "Game").osc (1, analog, sinePos).lfo (1, steps, 0.91f, 2)
            .stepPattern (1, { 0, 0, 4.17f, 4.17f, 8.33f, 8.33f, 12.5f, 12.5f, 16.67f, 16.67f, 20.83f, 20.83f, 25, 25, 29.17f, 29.17f })
            .mod (lfo1, "osc1Pitch", 50).filter (1, svfLow, 7400, 5).amp (0, 300, 100, 100).delay (sixteenth, 25, 24, 4500, false).level (-15)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).drive (crush, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Crystal Explosion", "Game").set ("osc1On", 0).noise (85, 3, 60).env (2, 0, 1350, 0, 400, 70).mod (env2, "noiseColour", 20)
            .mod (env2, "filter1Cutoff", 15).filter (1, ladder24, 1100, 20).subOsc (30).amp (0, 750, 0, 700, 70).level (-6).macro (1, "Size")
            .mod (macro1, "noiseColour", -35).macro (2, "Tone").mod (macro2, "filter1Cutoff", 20).drive (crush, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).reverb (0, 55, 1.6f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Crystal Menu", "Game").osc (1, analog, trianglePos).pitch (1, 5).env (2, 0, 45, 0, 40, 70).mod (env2, "osc1Fine", 40)
            .mod (WaveSynth::random, "osc1Fine", 8).filter (1, ladder12, 10000, 5).amp (2, 120, 0, 90).level (-9).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -30).macro (2, "Chirp").mod (macro2, "osc1Fine", 30).chorus (0, 0.5f, 50).macro (3, "Wide")
            .mod (macro3, "fxChorusMix", 45).reverb (0, 50, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Cursed Intruder", "Game").osc (1, pulse, 0).lfo (1, steps, 2, 1, 90)
            .stepPattern (1, { 0, 0, -100, -100, 0, 0, -100, -100, 0, 0, -100, -100, 0, 0, -100, -100 }).mod (lfo1, "ampLevel", 75)
            .lfo (2, square, 1.5f).mod (lfo2, "osc1Pitch", 2.083f).filter (1, svfBand, 9500).amp (2, 300, 100, 60).level (-12).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).chorus (0, 0.5f, 50).macro (3, "Wide")
            .mod (macro3, "fxChorusMix", 45).reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Curt Leap", "Game").osc (1, pulse, 0).pitch (1, -7).env (2, 150, 120, 0, 100, 0).mod (env2, "osc1Pitch", 25)
            .filter (1, ladder12, 12000, 5).amp (0, 140, 0, 100, 70).level (-11).macro (1, "Height").mod (macro1, "osc1Pitch", 14.583f)
            .macro (2, "Tone").mod (macro2, "filter1Cutoff", -30).delay (sixteenth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 35)
            .drive (crush, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Dawnless Locked", "Game").osc (1, analog, squarePos).pitch (1, -24).lfo (1, steps, 3.5f, 2, 80)
            .stepPattern (1, { 0, 0, 0, 0, 0, 0, -100, 0, 0, 0, 0, 0, 0, 0, -100, -100 }).mod (lfo1, "ampLevel", 83).filter (1, svfLow, 4200, 35)
            .amp (0, 300, 100, 130).reverb (20, 40, 0.6f, 40).level (-17).macro (1, "Tone").mod (macro1, "filter1Cutoff", 20).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 12).drive (crush, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Diamond Refill", "Game").osc (1, pulse, 53).lfo (1, steps, 2.86f, 2)
            .stepPattern (1, { 0, 0, 12.5f, 12.5f, 29.17f, 29.17f, 50, 50, 62.5f, 62.5f, 79.17f, 79.17f, 100, 100, 100, 100 })
            .mod (lfo1, "osc1Pitch", 50).filter (1, svfLow, 13000, 5).amp (0, 300, 100, 350).delay (sixteenthD, 45, 14, 2600, false)
            .reverb (25, 50, 0.8f, 70).level (-14).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 15).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 45).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 35);
        add ("Dry Turret", "Game").osc (1, pulse, 80).pitch (1, -5).env (2, 0, 250, 0, 100, 60).mod (env2, "osc1Pitch", 50).noise (50, 3, 60)
            .filter (1, svfLow, 12000, 30).amp (0, 230, 0, 50, 80).level (-9).macro (1, "Fall").mod (macro1, "osc1Pitch", 25).macro (2, "Tone")
            .mod (macro2, "filter1Cutoff", -30).reverb (0, 50, 1.2f, 45).macro (3, "Space").mod (macro3, "fxReverbMix", 40).drive (crush, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Eastern Boom", "Game").set ("osc1On", 0).noise (85, 3).env (2, 0, 250, 0, 400, 20).mod (env2, "noiseColour", 35)
            .mod (env2, "filter1Cutoff", 20).filter (1, svfLow, 520).amp (0, 950, 0, 450).level (-6).macro (1, "Size")
            .mod (macro1, "noiseColour", -35).macro (2, "Tone").mod (macro2, "filter1Cutoff", 20).drive (crush, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).reverb (0, 55, 1.6f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Encoded Explosion", "Game").set ("osc1On", 0).noise (95, 3).env (2, 0, 250, 0, 400).mod (env2, "noiseColour", 20)
            .mod (env2, "filter1Cutoff", 15).filter (1, svfLow, 800, 20).subOsc (35).amp (0, 1950, 0, 450, 40).reverb (15, 30, 0.6f, 70).level (-10)
            .macro (1, "Size").mod (macro1, "noiseColour", -35).macro (2, "Tone").mod (macro2, "filter1Cutoff", 20).drive (crush, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Exiled Overheat", "Game").osc (1, analog, squarePos).lfo (1, steps, 1, 1, 90)
            .stepPattern (1, { 0, 0, 0, -100, 0, 0, 0, -100, -100, -100, -100, -100, -100, -100, -100, -100 }).mod (lfo1, "ampLevel", 83)
            .lfo (2, square, 2).mod (lfo2, "osc1Pitch", 6.25f).filter (1, ladder12, 4100, 5).amp (2, 300, 100, 60).delay (sixteenth, 55, 14, 3500)
            .level (-12).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).drive (crush, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).chorus (0, 0.5f, 50).macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Fallen Gem", "Game").osc (1, pulse, 80).lfo (1, steps, 2.5f, 2)
            .stepPattern (1, { 0, 0, 0, 0, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f })
            .mod (lfo1, "osc1Pitch", 50).filter (1, svfLow, 6100, 5).amp (0, 700, 0, 200, 60).level (-10).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).delay (sixteenth, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).chorus (0, 0.5f, 50).macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Flat Blaster", "Game").osc (1, pulse, 53).pitch (1, 7).env (2, 0, 170, 0, 100, 80).mod (env2, "osc1Pitch", 50)
            .filter (1, svfBand, 3500).amp (0, 100, 0, 80, 80).level (-9).macro (1, "Fall").mod (macro1, "osc1Pitch", 25).macro (2, "Tone")
            .mod (macro2, "filter1Cutoff", -30).delay (sixteenth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 35)
            .reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Flickering Victory", "Game").osc (1, pulse, 53).lfo (1, steps, 0.43f, 2)
            .stepPattern (1, { 0, 0, 0, 0, 0, 0, 16.67f, 16.67f, 29.17f, 29.17f, 29.17f, 29.17f, 50, 50, 50, 50 }).mod (lfo1, "osc1Pitch", 50)
            .filter (1, svfLow, 12000, 5).amp (0, 300, 100, 250).delay (sixteenth, 40, 16, 2500).level (-14).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 12).chorus (0, 0.5f, 50).macro (3, "Wide")
            .mod (macro3, "fxChorusMix", 45).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Flooded Triumph", "Game").osc (1, pulse, 80).osc (2, pulse, 80, 50).pitch (2, -12, 8).lfo (1, steps, 0.48f, 2)
            .stepPattern (1, { 0, 8.33f, 16.67f, 20.83f, 29.17f, 29.17f, 29.17f, 29.17f, 37.5f, 37.5f, 45.83f, 45.83f, 50, 50, 50, 50 })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).filter (1, ladder12, 4400, 5).amp (0, 300, 100, 400)
            .delay (sixteenth, 55, 8, 2900, false).reverb (25, 50, 1.2f, 40).level (-12).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30)
            .macro (2, "Speed").mod (macro2, "lfo1Rate", 12).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 45)
            .drive (crush, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Fluorescent Reward", "Game").osc (1, pulse, 80).lfo (1, steps, 0.5f, 2)
            .stepPattern (1, { 50, 50, 29.17f, 29.17f, 37.5f, 37.5f, 20.83f, 20.83f, 29.17f, 29.17f, 16.67f, 16.67f, 0, 0, 0, 0 })
            .mod (lfo1, "osc1Pitch", 50).filter (1, svfLow, 4000, 5).amp (0, 300, 100, 150).delay (eighth, 35, 18, 5900).reverb (15, 35, 0.8f, 50)
            .level (-13).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 12).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).chorus (0, 0.5f, 50).macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Frosted Cannon", "Game").osc (1, pulse, 0).pitch (1, 7).env (2, 0, 70, 0, 100, 60).mod (env2, "osc1Pitch", 50)
            .filter (1, svfBand, 6700, 30).amp (0, 190, 0, 90, 70).level (-11).macro (1, "Fall").mod (macro1, "osc1Pitch", 25).macro (2, "Tone")
            .mod (macro2, "filter1Cutoff", -30).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 45).reverb (0, 50, 1.2f, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Frozen Forfeit", "Game").osc (1, analog, trianglePos).lfo (1, steps, 0.61f, 2)
            .stepPattern (1, { 50, 45.83f, 41.67f, 37.5f, 33.33f, 29.17f, 25, 20.83f, 16.67f, 12.5f, 8.33f, 4.17f, 0, -4.17f, -8.33f, -50 })
            .mod (lfo1, "osc1Pitch", 50).filter (1, ladder12, 5800, 5).amp (0, 900, 0, 200, 60).delay (sixteenth, 40, 16, 7100, false).level (-9)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).reverb (0, 50, 1.2f, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 40).chorus (0, 0.5f, 50).macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Gleaming Hazard", "Game").osc (1, analog, sinePos).lfo (1, steps, 0.5f, 1, 85)
            .stepPattern (1, { 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100, 0, -100 }).mod (lfo1, "ampLevel", 87)
            .filter (1, svfLow, 5400).amp (2, 300, 100, 130).level (-13).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 15).drive (crush, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).chorus (0, 0.5f, 50)
            .macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Gleaming Hop", "Game").osc (1, analog, trianglePos).pitch (1, -12).env (2, 290, 160, 100, 100, -20).mod (env2, "osc1Pitch", 14.583f)
            .filter (1, svfLow, 14000, 5).amp (0, 360, 0, 60, 70).level (-11).macro (1, "Height").mod (macro1, "osc1Pitch", 14.583f)
            .macro (2, "Tone").mod (macro2, "filter1Cutoff", -30).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 45)
            .reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Gutted Select", "Game").osc (1, analog, 15).pitch (1, 12).env (2, 0, 50, 0, 40, 70).mod (env2, "osc1Fine", 10)
            .mod (WaveSynth::random, "osc1Fine", 7).filter (1, ladder12, 8400, 5).amp (5, 70, 0, 40, 70).reverb (20, 40, 0.6f, 60).level (-9)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Chirp").mod (macro2, "osc1Fine", 30).delay (sixteenth, 40, 0, 4500)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 35).chorus (0, 0.5f, 50).macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Halogen Pew", "Game").osc (1, pulse, 0).pitch (1, 7).env (2, 0, 180, 0, 100, 60).mod (env2, "osc1Pitch", 50)
            .filter (1, svfBand, 3500, 5).amp (0, 370, 0, 100, 70).level (-14).macro (1, "Fall").mod (macro1, "osc1Pitch", 25).macro (2, "Tone")
            .mod (macro2, "filter1Cutoff", -30).delay (sixteenth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).chorus (0, 0.5f, 50)
            .macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Halogen Triumph", "Game").osc (1, pulse, 0).osc (2, pulse, 53, 65).pitch (2, -12, 8).lfo (1, steps, 1.11f, 2)
            .stepPattern (1, { 50, 50, 29.17f, 29.17f, 37.5f, 37.5f, 20.83f, 20.83f, 29.17f, 29.17f, 16.67f, 16.67f, 0, 0, 0, 0 })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).filter (1, ladder12, 9800, 5).amp (0, 300, 100, 400)
            .delay (sixteenth, 55, 18, 2000).level (-15).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 12).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 45).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 35);
        add ("Hexed Powerup", "Game").osc (1, pulse, 80).lfo (1, steps, 0.87f, 2)
            .stepPattern (1, { 0, 0, 20.83f, 20.83f, 29.17f, 29.17f, 50, 50, 70.83f, 70.83f, 79.17f, 79.17f, 100, 100, 100, 100 })
            .mod (lfo1, "osc1Pitch", 50).filter (1, ladder12, 6600, 5).amp (0, 300, 100, 100).delay (sixteenth, 40, 22, 4000, false).level (-12)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Inland Triumph", "Game").osc (1, analog, sinePos).osc (2, pulse, 0, 65).pitch (2, -12).lfo (1, steps, 0.42f, 2)
            .stepPattern (1, { 0, 0, 0, 0, 0, 0, 16.67f, 16.67f, 29.17f, 29.17f, 29.17f, 29.17f, 50, 50, 50, 50 }).mod (lfo1, "osc1Pitch", 50)
            .mod (lfo1, "osc2Pitch", 50).filter (1, ladder12, 5700, 5).amp (0, 300, 100, 400).delay (eighth, 45, 14, 2400, false).level (-16)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 12).chorus (0, 0.5f, 50)
            .macro (3, "Wide").mod (macro3, "fxChorusMix", 45).reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Looping Refill", "Game").osc (1, analog, sinePos).lfo (1, steps, 1.82f, 2)
            .stepPattern (1, { 0, 0, 8.33f, 8.33f, 12.5f, 12.5f, 20.83f, 20.83f, 29.17f, 29.17f, 33.33f, 33.33f, 41.67f, 41.67f, 50, 50 })
            .mod (lfo1, "osc1Pitch", 50).filter (1, ladder12, 5700, 5).amp (0, 300, 100, 200).delay (sixteenth, 35, 14, 7000).level (-12)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).reverb (0, 50, 1.2f, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 40).chorus (0, 0.5f, 50).macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Lucid Blaster", "Game").osc (1, analog, trianglePos).pitch (1, 7).env (2, 0, 280, 0, 100, 90).mod (env2, "osc1Pitch", 64.583f)
            .noise (30, 3, 60).filter (1, svfLow, 5300, 30).amp (0, 200, 0, 160, 60).level (-12).macro (1, "Fall").mod (macro1, "osc1Pitch", 25)
            .macro (2, "Tone").mod (macro2, "filter1Cutoff", -30).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 45)
            .delay (sixteenth, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Lucid Save", "Game").osc (1, analog, sinePos).osc (2, analog, sinePos, 65).pitch (2, 12).lfo (1, steps, 1.43f, 2)
            .stepPattern (1, { 0, 0, 0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50 }).mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50)
            .filter (1, ladder12, 4500, 5).amp (5, 500, 0, 200).reverb (22, 100, 3, 70).level (-14).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).delay (sixteenth, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).macro (4, "Octave").mod (macro4, "osc2Volume", -45);
        add ("Magnesium Cursor", "Game").osc (1, analog, trianglePos).pitch (1, 12).env (2, 0, 20, 0, 40, 70).mod (env2, "osc1Fine", -20)
            .mod (WaveSynth::random, "osc1Fine", 6).filter (1, svfLow, 7500, 5).amp (5, 110, 0, 60).level (-9).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -30).macro (2, "Chirp").mod (macro2, "osc1Fine", 30).delay (sixteenth, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).reverb (0, 50, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Magnesium Select", "Game").osc (1, analog, sinePos).pitch (1, 12).osc (2, analog, sinePos, 45).pitch (2, 19).env (2, 0, 70, 0, 40, 70)
            .mod (env2, "osc1Fine", 30).mod (WaveSynth::random, "osc1Fine", 5).filter (1, ladder12, 9200, 5).amp (0, 30, 0, 60, 40)
            .reverb (16, 55, 1, 50).level (-9).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Chirp").mod (macro2, "osc1Fine", 30)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 40).drive (tube, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Neon Dash", "Game").osc (1, analog, squarePos).pitch (1, -5).env (2, 70, 60, 100, 100, 0).mod (env2, "osc1Pitch", 35.417f)
            .filter (1, ladder12, 9600, 5).amp (0, 380, 0, 180, 30).delay (sixteenthD, 45, 18, 6000, false).level (-12).macro (1, "Height")
            .mod (macro1, "osc1Pitch", 14.583f).macro (2, "Tone").mod (macro2, "filter1Cutoff", -30).chorus (0, 0.5f, 50).macro (3, "Wide")
            .mod (macro3, "fxChorusMix", 45).drive (crush, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Nervous Invalid", "Game").osc (1, pulse, 65).pitch (1, -24).osc (2, analog, squarePos, 65).pitch (2, -23).lfo (1, steps, 1.75f, 2, 80)
            .stepPattern (1, { 0, 0, 0, -100, 0, 0, 0, -100, -100, -100, -100, -100, -100, -100, -100, -100 }).mod (lfo1, "ampLevel", 87)
            .filter (1, svfBand, 1700, 15).amp (0, 300, 100, 40).level (-9).macro (1, "Tone").mod (macro1, "filter1Cutoff", 20).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 12).drive (crush, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).reverb (0, 50, 1.2f, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Nervous Ruin", "Game").osc (1, pulse, 53).lfo (1, steps, 0.61f, 2)
            .stepPattern (1, { 0, -4.17f, 0, -8.33f, 0, -12.5f, 0, -20.83f, 0, -29.17f, -33.33f, -37.5f, -41.67f, -45.83f, -50, -50 })
            .mod (lfo1, "osc1Pitch", 50).filter (1, svfLow, 5300, 5).amp (0, 2100, 0, 350, 40).delay (sixteenth, 55, 16, 2100).level (-12)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).reverb (0, 50, 1.2f, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 40).chorus (0, 0.5f, 50).macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Nomad Ruin", "Game").osc (1, pulse, 80).lfo (1, steps, 0.59f, 2)
            .stepPattern (1, { 50, 45.83f, 41.67f, 37.5f, 33.33f, 29.17f, 25, 20.83f, 16.67f, 12.5f, 8.33f, 4.17f, 0, -4.17f, -8.33f, -50 })
            .mod (lfo1, "osc1Pitch", 50).filter (1, svfLow, 3200, 5).amp (0, 700, 60, 100).drive (crush, 30, 4700).level (-12).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .chorus (0, 0.5f, 50).macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Nuclear Turret", "Game").osc (1, analog, squarePos).pitch (1, 0).env (2, 0, 320, 0, 100, 40).mod (env2, "osc1Pitch", 64.583f)
            .filter (1, svfBand, 9800, 5).amp (0, 160, 0, 140, 80).delay (sixteenthD, 40, 10, 2400).level (-10).macro (1, "Fall")
            .mod (macro1, "osc1Pitch", 25).macro (2, "Tone").mod (macro2, "filter1Cutoff", -30).reverb (0, 50, 1.2f, 45).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 40).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Oiled Save", "Game").osc (1, analog, 15).osc (2, analog, sinePos, 55).pitch (2, 12).lfo (1, steps, 1.25f, 2)
            .stepPattern (1, { 0, 0, 0, 0, 12.5f, 12.5f, 12.5f, 12.5f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).filter (1, svfLow, 4900, 5).amp (0, 850, 0, 400, 40).reverb (13, 75, 3, 40)
            .level (-15).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Octave").mod (macro4, "osc2Volume", -45);
        add ("Overnight Defeat", "Game").osc (1, analog, squarePos).lfo (1, steps, 1.18f, 2)
            .stepPattern (1, { 29.17f, 25, 20.83f, 16.67f, 12.5f, 8.33f, 4.17f, 0, -4.17f, -8.33f, -12.5f, -16.67f, -20.83f, -25, -29.17f, -50 })
            .mod (lfo1, "osc1Pitch", 50).filter (1, svfLow, 5000, 5).amp (0, 1600, 0, 250, 60).delay (sixteenth, 55, 12, 2200)
            .drive (crush, 40, 4600).level (-13).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 15).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 45).reverb (0, 50, 1.2f, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Polished Key", "Game").osc (1, analog, sinePos).lfo (1, steps, 1.67f, 2)
            .stepPattern (1, { 0, 0, 37.5f, 37.5f, 37.5f, 37.5f, 37.5f, 37.5f, 37.5f, 37.5f, 37.5f, 37.5f, 37.5f, 37.5f, 37.5f, 37.5f })
            .mod (lfo1, "osc1Pitch", 50).filter (1, ladder12, 14000, 5).amp (0, 250, 0, 300, 70).reverb (10, 40, 0.6f, 70).level (-11)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).chorus (0, 0.5f, 50)
            .macro (3, "Wide").mod (macro3, "fxChorusMix", 45).delay (sixteenth, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Prowling Meltdown", "Game").osc (1, pulse, 80).lfo (1, steps, 1, 1, 80)
            .stepPattern (1, { 0, 0, -100, -100, 0, 0, -100, -100, 0, 0, -100, -100, 0, 0, -100, -100 }).mod (lfo1, "ampLevel", 87)
            .filter (1, svfBand, 5000, 25).amp (2, 300, 100, 140).delay (sixteenth, 55, 14, 4000).level (-9).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).reverb (0, 50, 1.2f, 45).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 40).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Prowling Success", "Game").osc (1, analog, 15).osc (2, analog, sinePos, 40).pitch (2, 12).lfo (1, steps, 2, 2)
            .stepPattern (1, { 0, 0, 0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50 }).mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50)
            .filter (1, ladder12, 6200, 5).amp (5, 800, 0, 250, 45).reverb (25, 85, 4.5f, 55).level (-12).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).macro (3, "Octave").mod (macro3, "osc2Volume", -45)
            .delay (sixteenth, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Pulsing Gem", "Game").osc (1, analog, sinePos).lfo (1, steps, 2, 2)
            .stepPattern (1, { 0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50 }).mod (lfo1, "osc1Pitch", 50)
            .filter (1, svfLow, 6000, 5).amp (0, 600, 0, 200, 60).level (-13).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30)
            .macro (2, "Speed").mod (macro2, "lfo1Rate", 15).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 45)
            .reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Quantised Blast", "Game").set ("osc1On", 0).noise (95, 3, 20).env (2, 0, 250, 0, 400, 60).mod (env2, "noiseColour", 45)
            .mod (env2, "filter1Cutoff", 10).filter (1, ladder12, 560, 30).subOsc (45).amp (0, 350, 0, 650, 70).level (-9).macro (1, "Size")
            .mod (macro1, "noiseColour", -35).macro (2, "Tone").mod (macro2, "filter1Cutoff", 20).drive (crush, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).reverb (0, 55, 1.6f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Quicksilver Leap", "Game").osc (1, analog, squarePos).pitch (1, 0).env (2, 110, 40, 100, 100, -40).mod (env2, "osc1Pitch", 25)
            .filter (1, ladder12, 9100, 5).amp (0, 240, 0, 160).level (-11).macro (1, "Height").mod (macro1, "osc1Pitch", 14.583f).macro (2, "Tone")
            .mod (macro2, "filter1Cutoff", -30).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 45).reverb (0, 50, 1.2f, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Railside Ring", "Game").osc (1, pulse, 80).lfo (1, steps, 2.5f, 2)
            .stepPattern (1, { 0, 0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50 }).mod (lfo1, "osc1Pitch", 50)
            .filter (1, svfLow, 13000, 5).amp (0, 450, 0, 300).delay (sixteenth, 40, 20, 5500).reverb (10, 55, 1, 70).level (-9).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).macro (3, "Space").mod (macro3, "fxReverbMix", 40)
            .chorus (0, 0.5f, 50).macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Ration Menu", "Game").osc (1, analog, sinePos).pitch (1, 0).osc (2, analog, sinePos, 60).pitch (2, 7).env (2, 0, 25, 0, 40, 70)
            .mod (env2, "osc1Fine", -10).mod (WaveSynth::random, "osc1Fine", 4).filter (1, svfLow, 10000, 5).amp (0, 40, 0, 30, 40)
            .reverb (20, 40, 1.2f, 50).level (-11).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Chirp")
            .mod (macro2, "osc1Fine", 30).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).chorus (0, 0.5f, 50)
            .macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Reckless Leap", "Game").osc (1, analog, sinePos).pitch (1, -5).env (2, 350, 160, 100, 100, -40).mod (env2, "osc1Pitch", 25)
            .filter (1, svfLow, 7000, 5).amp (0, 240, 0, 120, 30).level (-12).macro (1, "Height").mod (macro1, "osc1Pitch", 14.583f)
            .macro (2, "Tone").mod (macro2, "filter1Cutoff", -30).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 45)
            .drive (crush, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Red Encore", "Game").osc (1, pulse, 80).lfo (1, steps, 0.43f, 2)
            .stepPattern (1, { 29.17f, 29.17f, 29.17f, 50, 50, 50, 66.67f, 66.67f, 79.17f, 79.17f, 79.17f, 79.17f, 79.17f, 79.17f, 79.17f, 79.17f })
            .mod (lfo1, "osc1Pitch", 50).filter (1, ladder12, 4400, 5).amp (0, 300, 100, 350).delay (sixteenthD, 45, 10, 2200)
            .reverb (15, 30, 1, 70).level (-12).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 12)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 35).chorus (0, 0.5f, 50).macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Rolling Token", "Game").osc (1, analog, sinePos).lfo (1, steps, 3.33f, 2)
            .stepPattern (1, { 0, 0, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f, 29.17f })
            .mod (lfo1, "osc1Pitch", 50).filter (1, ladder12, 5200, 5).amp (0, 700, 0, 250, 70).delay (sixteenth, 55, 14, 2300, false).level (-11)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Savage Refuse", "Game").osc (1, analog, sawPos).pitch (1, -19).osc (2, analog, squarePos, 70).pitch (2, -18).lfo (1, steps, 2, 2, 85)
            .stepPattern (1, { 0, 0, 0, 0, 0, -100, -100, 0, 0, 0, 0, 0, 0, -100, -100, -100 }).mod (lfo1, "ampLevel", 75)
            .filter (1, svfBand, 4200, 25).amp (0, 300, 100, 70).drive (hard, 20, 7500).level (-16).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", 20).macro (2, "Speed").mod (macro2, "lfo1Rate", 12).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Scalding Locked", "Game").osc (1, pulse, 55).pitch (1, -19).osc (2, analog, squarePos, 65).pitch (2, -18).lfo (1, steps, 1.5f, 2, 75)
            .stepPattern (1, { 0, 0, 0, 0, 0, -100, -100, 0, 0, 0, 0, 0, 0, -100, -100, -100 }).mod (lfo1, "ampLevel", 75)
            .filter (1, ladder12, 4900, 15).amp (0, 300, 100, 70).drive (crush, 40, 3400).level (-14).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", 20).macro (2, "Speed").mod (macro2, "lfo1Rate", 12).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Scoured Blaster", "Game").osc (1, analog, squarePos).pitch (1, 7).env (2, 0, 110, 0, 100, 70).mod (env2, "osc1Pitch", 50)
            .filter (1, ladder12, 6400, 15).amp (0, 120, 0, 90, 60).delay (sixteenthD, 50, 18, 2500).level (-11).macro (1, "Fall")
            .mod (macro1, "osc1Pitch", 25).macro (2, "Tone").mod (macro2, "filter1Cutoff", -30).macro (3, "Echo").mod (macro3, "fxDelayMix", 35)
            .reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Shapeless Restore", "Game").osc (1, pulse, 53).lfo (1, steps, 0.8f, 2)
            .stepPattern (1, { 0, 0, 8.33f, 8.33f, 16.67f, 16.67f, 25, 25, 33.33f, 33.33f, 41.67f, 41.67f, 50, 50, 58.33f, 58.33f })
            .mod (lfo1, "osc1Pitch", 50).filter (1, ladder12, 8800, 5).amp (0, 300, 100, 150).delay (sixteenthD, 45, 14, 1800).level (-12)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).drive (crush, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).chorus (0, 0.5f, 50).macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Silver Encore", "Game").osc (1, analog, squarePos).lfo (1, steps, 0.53f, 2)
            .stepPattern (1, { 0, 8.33f, 16.67f, 20.83f, 29.17f, 29.17f, 29.17f, 29.17f, 37.5f, 37.5f, 45.83f, 45.83f, 50, 50, 50, 50 })
            .mod (lfo1, "osc1Pitch", 50).filter (1, ladder12, 4200, 5).amp (0, 300, 100, 300).delay (sixteenth, 45, 16, 5000, false)
            .reverb (10, 50, 1.6f, 60).level (-13).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 12).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 45).macro (4, "Echo")
            .mod (macro4, "fxDelayMix", 35);
        add ("Slanted Loss", "Game").osc (1, analog, trianglePos).lfo (1, steps, 0.69f, 2)
            .stepPattern (1, { 0, -4.17f, 0, -8.33f, 0, -12.5f, 0, -20.83f, 0, -29.17f, -33.33f, -37.5f, -41.67f, -45.83f, -50, -50 })
            .mod (lfo1, "osc1Pitch", 50).filter (1, svfLow, 4800, 5).amp (0, 1000, 60, 300, 30).drive (crush, 20, 6700).level (-12)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).chorus (0, 0.5f, 50)
            .macro (3, "Wide").mod (macro3, "fxChorusMix", 45).delay (sixteenth, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Solemn Hop", "Game").osc (1, pulse, 0).pitch (1, -12).env (2, 300, 40, 100, 100, -20).mod (env2, "osc1Pitch", 35.417f)
            .filter (1, svfLow, 9300, 5).amp (0, 180, 0, 60, 70).delay (sixteenthD, 25, 18, 5200, false).level (-13).macro (1, "Height")
            .mod (macro1, "osc1Pitch", 14.583f).macro (2, "Tone").mod (macro2, "filter1Cutoff", -30).chorus (0, 0.5f, 50).macro (3, "Wide")
            .mod (macro3, "fxChorusMix", 45).drive (crush, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Stainless Refuse", "Game").osc (1, analog, squarePos).pitch (1, -17).lfo (1, steps, 3.75f, 2, 85)
            .stepPattern (1, { 0, 0, 0, -100, 0, 0, 0, -100, -100, -100, -100, -100, -100, -100, -100, -100 }).mod (lfo1, "ampLevel", 87)
            .filter (1, ladder12, 2900).amp (0, 300, 100, 70).drive (hard, 40, 4700).reverb (25, 50, 1.6f, 40).level (-23).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", 20).macro (2, "Speed").mod (macro2, "lfo1Rate", 12).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Stocky Menu", "Game").osc (1, analog, trianglePos).pitch (1, 0).env (2, 0, 30, 0, 40, 70).mod (env2, "osc1Fine", -40)
            .mod (WaveSynth::random, "osc1Fine", 5).filter (1, svfLow, 5500, 5).amp (2, 110, 0, 70, 60).level (-9).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -30).macro (2, "Chirp").mod (macro2, "osc1Fine", 30).drive (tube, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).reverb (0, 50, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Stubby Boom", "Game").set ("osc1On", 0).noise (85, 3, 60).env (2, 0, 750, 0, 400).mod (env2, "noiseColour", 30)
            .mod (env2, "filter1Cutoff", 25).filter (1, ladder12, 3400, 30).amp (0, 1900, 0, 450, 40).level (-6).macro (1, "Size")
            .mod (macro1, "noiseColour", -35).macro (2, "Tone").mod (macro2, "filter1Cutoff", 20).drive (crush, 0, 9000).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).reverb (0, 55, 1.6f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Terse Toggle", "Game").osc (1, analog, trianglePos).pitch (1, 5).osc (2, analog, sinePos, 60).pitch (2, 9).env (2, 0, 65, 0, 40, 70)
            .mod (env2, "osc1Fine", -20).mod (WaveSynth::random, "osc1Fine", 6).filter (1, svfLow, 7400, 5).amp (2, 60, 0, 30, 60).level (-11)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Chirp").mod (macro2, "osc1Fine", 30).reverb (0, 50, 1, 45)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 40).chorus (0, 0.5f, 50).macro (4, "Wide").mod (macro4, "fxChorusMix", 45);
        add ("Tight Jump", "Game").osc (1, analog, trianglePos).pitch (1, 0).env (2, 300, 40, 100, 100, 40).mod (env2, "osc1Pitch", 50)
            .filter (1, ladder12, 9300, 5).amp (0, 280, 0, 140).level (-9).macro (1, "Height").mod (macro1, "osc1Pitch", 14.583f).macro (2, "Tone")
            .mod (macro2, "filter1Cutoff", -30).chorus (0, 0.5f, 50).macro (3, "Wide").mod (macro3, "fxChorusMix", 45).reverb (0, 50, 1.2f, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Tireless Ring", "Game").osc (1, pulse, 0).lfo (1, steps, 2.5f, 2)
            .stepPattern (1, { 0, 0, 16.67f, 16.67f, 16.67f, 16.67f, 16.67f, 16.67f, 16.67f, 16.67f, 16.67f, 16.67f, 16.67f, 16.67f, 16.67f, 16.67f })
            .mod (lfo1, "osc1Pitch", 50).filter (1, ladder12, 5000, 5).amp (0, 250, 0, 150).delay (sixteenthD, 55, 18, 2400)
            .reverb (20, 55, 1.6f, 60).level (-11).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 15).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Toxic Select", "Game").osc (1, analog, sinePos).pitch (1, 0).env (2, 0, 80, 0, 40, 70).mod (env2, "osc1Fine", -40)
            .mod (WaveSynth::random, "osc1Fine", 10).filter (1, ladder12, 5800, 5).amp (0, 40, 0, 100, 40).level (-9).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -30).macro (2, "Chirp").mod (macro2, "osc1Fine", 30).delay (sixteenth, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).reverb (0, 50, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Twitching Powerup", "Game").osc (1, analog, squarePos).lfo (1, steps, 1.67f, 2)
            .stepPattern (1, { 0, 4.17f, 8.33f, 12.5f, 16.67f, 20.83f, 25, 29.17f, 50, 54.17f, 58.33f, 62.5f, 66.67f, 70.83f, 75, 79.17f })
            .mod (lfo1, "osc1Pitch", 50).filter (1, ladder12, 11000, 5).amp (0, 300, 100, 350).delay (sixteenth, 30, 24, 4800)
            .reverb (15, 45, 1.4f, 60).level (-13).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 15).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Umber Boost", "Game").osc (1, analog, sinePos).lfo (1, steps, 0.87f, 2)
            .stepPattern (1, { 0, 0, 8.33f, 8.33f, 16.67f, 16.67f, 25, 25, 33.33f, 33.33f, 41.67f, 41.67f, 50, 50, 58.33f, 58.33f })
            .mod (lfo1, "osc1Pitch", 50).filter (1, svfLow, 7800, 5).amp (0, 300, 100, 200).delay (eighth, 55, 18, 4600).level (-14)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).drive (crush, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).reverb (0, 50, 1.2f, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Virtual Triumph", "Game").osc (1, analog, sinePos).osc (2, pulse, 53, 50).pitch (2, -12, 8).lfo (1, steps, 0.59f, 2)
            .stepPattern (1, { 0, 0, 12.5f, 12.5f, 29.17f, 29.17f, 41.67f, 41.67f, 50, 50, 50, 50, 62.5f, 62.5f, 50, 50 })
            .mod (lfo1, "osc1Pitch", 50).mod (lfo1, "osc2Pitch", 50).filter (1, svfLow, 10000, 5).amp (0, 300, 100, 200)
            .delay (sixteenthD, 25, 18, 2400, false).level (-15).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 12).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).drive (crush, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Warped Refill", "Game").osc (1, analog, trianglePos).lfo (1, steps, 1.18f, 2)
            .stepPattern (1, { 0, 0, 20.83f, 20.83f, 29.17f, 29.17f, 50, 50, 70.83f, 70.83f, 79.17f, 79.17f, 100, 100, 100, 100 })
            .mod (lfo1, "osc1Pitch", 50).filter (1, ladder12, 4100, 5).amp (0, 300, 100, 300).delay (sixteenthD, 35, 14, 5100).level (-12)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).chorus (0, 0.5f, 50)
            .macro (3, "Wide").mod (macro3, "fxChorusMix", 45).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("Weightless Fall", "Game").osc (1, analog, sinePos).lfo (1, steps, 1.82f, 2)
            .stepPattern (1, { 50, 41.67f, 33.33f, 29.17f, 20.83f, 12.5f, 8.33f, 0, -8.33f, -16.67f, -20.83f, -29.17f, -37.5f, -41.67f, -50, -50 })
            .mod (lfo1, "osc1Pitch", 50).filter (1, svfLow, 5600, 5).amp (0, 2200, 0, 200, 30).delay (sixteenthD, 30, 10, 6400).level (-13)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).chorus (0, 0.5f, 50)
            .macro (3, "Wide").mod (macro3, "fxChorusMix", 45).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);
        add ("White Confirm", "Game").osc (1, analog, trianglePos).osc (2, analog, sinePos, 55).pitch (2, 12).lfo (1, steps, 2.5f, 2)
            .stepPattern (1, { 0, 0, 0, 0, 29.17f, 29.17f, 29.17f, 29.17f, 50, 50, 50, 50, 50, 50, 50, 50 }).mod (lfo1, "osc1Pitch", 50)
            .mod (lfo1, "osc2Pitch", 50).filter (1, svfLow, 8400, 5).amp (0, 800, 0, 350).reverb (13, 85, 7, 30).level (-14).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).delay (sixteenth, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).macro (4, "Octave").mod (macro4, "osc2Volume", -45);
        add ("White Token", "Game").osc (1, analog, sinePos).lfo (1, steps, 1.25f, 2)
            .stepPattern (1, { 0, 0, 0, 0, 20.83f, 20.83f, 20.83f, 20.83f, 20.83f, 20.83f, 20.83f, 20.83f, 20.83f, 20.83f, 20.83f, 20.83f })
            .mod (lfo1, "osc1Pitch", 50).filter (1, ladder12, 12000, 5).amp (0, 450, 0, 100, 70).delay (eighth, 45, 10, 2200)
            .reverb (10, 40, 0.6f, 70).level (-11).macro (1, "Tone").mod (macro1, "filter1Cutoff", -30).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 15).macro (3, "Space").mod (macro3, "fxReverbMix", 40).chorus (0, 0.5f, 50).macro (4, "Wide")
            .mod (macro4, "fxChorusMix", 45);
        add ("Zone Loss", "Game").osc (1, pulse, 80).lfo (1, steps, 1, 2)
            .stepPattern (1, { 50, 45.83f, 41.67f, 37.5f, 33.33f, 29.17f, 25, 20.83f, 16.67f, 12.5f, 8.33f, 4.17f, 0, -4.17f, -8.33f, -50 })
            .mod (lfo1, "osc1Pitch", 50).filter (1, svfLow, 6700, 5).amp (0, 1700, 0, 100).drive (crush, 35, 7500).level (-12).macro (1, "Tone")
            .mod (macro1, "filter1Cutoff", -30).macro (2, "Speed").mod (macro2, "lfo1Rate", 15).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .delay (sixteenth, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 35);

        // Chip, by name.
        add ("Amber Fuzz", "Chip").set ("osc1On", 0).noise (80, 3, 60).env (2, 0, 40, 0, 40, 60).mod (env2, "noiseColour", 20).filter (1, off, 8000)
            .amp (0, 40, 0, 130, 70).level (-1).macro (1, "Pitch").mod (macro1, "noiseColour", 35).drive (crush, 0, 8000).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 55).delay (sixteenth, 30, 0, 8000).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Banished Rotate", "Chip").osc (1, pulse, 40).lfo (1, steps, 2)
            .stepPattern (1, { 30, 100, -100, 100, 30, 100, -100, 100, 30, 100, -100, 100, 30, 100, -100, 100 }).mod (lfo1, "osc1Position", 30)
            .osc (2, pulse, 0, 60).pitch (2, -12).filter (1, off, 8000).amp (0, 250, 100, 70).level (-17).macro (1, "Duty")
            .mod (macro1, "osc1Position", 30).macro (2, "Speed").mod (macro2, "lfo1Rate", 12).delay (eighth, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Binary Duet", "Chip").osc (1, pulse, 0).osc (2, pulse, 0, 70).pitch (2, 7, 14).pan (1, -30).pan (2, 30).lfo (1, triangle, 6.5f)
            .lfoFade (1, 300).mod (lfo1, "osc1Fine", 4).mod (lfo1, "osc2Fine", 8).filter (1, off, 8000).amp (0, 150, 80, 140)
            .delay (eighthD, 25, 24, 9100).level (-17).macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Detune")
            .mod (macro2, "osc2Fine", 20).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (crush, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Bleached Channel", "Chip").osc (1, pulse, 53).lfo (1, sine, 6.75f).lfoFade (1, 450).mod (lfo1, "osc1Fine", 17).filter (1, off, 8000)
            .amp (0, 550, 50, 120).mono().delay (eighth, 25, 24, 1900).level (-4).macro (1, "Duty").mod (macro1, "osc1Position", 30)
            .macro (2, "Octave").mod (macro2, "osc1Pitch", 25).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Bleached Echo", "Chip").osc (1, pulse, 0).osc (2, pulse, 0, 65).pitch (2, 0, 10).pan (1, -30).pan (2, 40).lfo (1, sine, 6)
            .lfoFade (1, 150).mod (lfo1, "osc1Fine", 8).mod (lfo1, "osc2Fine", 5).filter (1, off, 8000).amp (0, 550, 80, 80)
            .delay (eighthD, 25, 12, 5700).level (-17).macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Detune")
            .mod (macro2, "osc2Fine", 20).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (crush, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Bleached Shift", "Chip").osc (1, pulse, 40).lfo (1, steps, 2)
            .stepPattern (1, { -30, -100, -100, -30, -30, -100, -100, -30, -30, -100, -100, -30, -30, -100, -100, -30 })
            .mod (lfo1, "osc1Position", 30).osc (2, pulse, 53, 55).pitch (2, -12).filter (1, off, 8000).amp (0, 600, 100, 40)
            .delay (sixteenthD, 30, 14, 2400).level (-15).macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 12).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Boxed Step", "Chip").osc (1, analog, trianglePos).warp (1, wCrush, 62).pitch (1, -12).filter (1, off, 8000).amp (0, 350, 0, 80)
            .mono (0, false).level (-4).delay (eighth, 30, 0, 6000).macro (1, "Steps").mod (macro1, "osc1WarpAmount", -30).drive (crush, 0, 8000)
            .macro (2, "Grit").mod (macro2, "fxDriveAmount", 55).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Burnished Descent", "Chip").osc (1, pulse, 0).pitch (1, -12).env (2, 0, 160, 0, 100, 20).mod (env2, "osc1Pitch", 25)
            .filter (1, off, 8000).amp (0, 480, 0, 90, 70).level (-13).macro (1, "Fall").mod (macro1, "osc1Pitch", 25).drive (crush, 0, 8000)
            .macro (2, "Grit").mod (macro2, "fxDriveAmount", 55).delay (sixteenth, 30, 0, 8000).macro (3, "Echo").mod (macro3, "fxDelayMix", 35)
            .reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Caged Pair", "Chip").osc (1, pulse, 80).osc (2, pulse, 0).pitch (2, 12, 14).pan (1, -20).pan (2, 30).lfo (1, triangle, 7)
            .lfoFade (1, 500).mod (lfo1, "osc1Fine", 13).mod (lfo1, "osc2Fine", 6).filter (1, off, 8000).amp (0, 600, 80, 60)
            .delay (sixteenth, 30, 26, 9500, false).level (-18).macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Detune")
            .mod (macro2, "osc2Fine", 20).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (crush, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Close Tumble", "Chip").osc (1, pulse, 0).pitch (1, -12).env (2, 0, 430, 0, 100, 75).mod (env2, "osc1Pitch", 25).filter (1, off, 8000)
            .amp (0, 100, 0, 80).level (-13).macro (1, "Fall").mod (macro1, "osc1Pitch", 25).drive (crush, 0, 8000).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 55).delay (sixteenth, 30, 0, 8000).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Coastal Crash", "Chip").set ("osc1On", 0).noise (90, 3, -40).env (2, 0, 700, 0, 700, 30).mod (env2, "noiseColour", 25)
            .filter (1, off, 8000).amp (0, 700, 0, 50).level (-7).macro (1, "Pitch").mod (macro1, "noiseColour", 35).drive (crush, 0, 8000)
            .macro (2, "Grit").mod (macro2, "fxDriveAmount", 55).delay (sixteenth, 30, 0, 8000).macro (3, "Echo").mod (macro3, "fxDelayMix", 35)
            .reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Cornered Shift", "Chip").osc (1, pulse, 40).lfo (1, steps, 2)
            .stepPattern (1, { -100, -100, -30, 30, -100, -100, -30, 30, -100, -100, -30, 30, -100, -100, -30, 30 }).mod (lfo1, "osc1Position", 35)
            .osc (2, pulse, 0, 55).pitch (2, 7, 8).filter (1, off, 8000).amp (0, 750, 30, 140).delay (eighthD, 50, 18, 2900).level (-15)
            .macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Speed").mod (macro2, "lfo1Rate", 12).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Crimson Burst", "Chip").set ("osc1On", 0).noise (85, 3, 80).env (2, 0, 150, 0, 150, 40).mod (env2, "noiseColour", 25)
            .filter (1, off, 8000).amp (0, 150, 0, 70).level (-5).macro (1, "Pitch").mod (macro1, "noiseColour", 35).drive (crush, 0, 8000)
            .macro (2, "Grit").mod (macro2, "fxDriveAmount", 55).delay (sixteenth, 30, 0, 8000).macro (3, "Echo").mod (macro3, "fxDelayMix", 35)
            .reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Crooked Pulse", "Chip").osc (1, pulse, 80).lfo (1, triangle, 7.25f).lfoFade (1, 300).mod (lfo1, "osc1Fine", 10).filter (1, off, 8000)
            .amp (0, 400, 40, 140).mono (60, false).delay (eighthD, 45, 20, 4600, false).level (-4).macro (1, "Duty")
            .mod (macro1, "osc1Position", 30).macro (2, "Octave").mod (macro2, "osc1Pitch", 25).macro (3, "Echo").mod (macro3, "fxDelayMix", 35)
            .reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Diamond Ramp", "Chip").osc (1, analog, trianglePos).warp (1, wCrush, 60).pitch (1, -12).env (2, 0, 120, 0, 60, 80)
            .mod (env2, "osc1Pitch", 25).filter (1, off, 8000).amp (0, 800, 100, 120).mono().level (-4).delay (eighth, 30, 0, 6000)
            .macro (1, "Steps").mod (macro1, "osc1WarpAmount", -30).drive (crush, 0, 8000).macro (2, "Grit").mod (macro2, "fxDriveAmount", 55)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("District Descent", "Chip").osc (1, analog, trianglePos).warp (1, wCrush, 56).pitch (1, 0).env (2, 0, 400, 0, 100, 20)
            .mod (env2, "osc1Pitch", 75).filter (1, off, 8000).amp (0, 220, 0, 30, 40).level (-11).macro (1, "Fall").mod (macro1, "osc1Pitch", 25)
            .drive (crush, 0, 8000).macro (2, "Grit").mod (macro2, "fxDriveAmount", 55).delay (sixteenth, 30, 0, 8000).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Faceless Flicker", "Chip").osc (1, pulse, 0).lfo (1, steps, 2.5f)
            .stepPattern (1, { 0, 50, 0, 50, 0, 50, 0, 50, 0, 50, 0, 50, 0, 50, 0, 50 }).mod (lfo1, "osc1Pitch", 50).filter (1, off, 8000)
            .amp (0, 500, 0, 60).delay (sixteenthD, 55, 20, 4100, false).level (-13).macro (1, "Duty").mod (macro1, "osc1Position", 30)
            .macro (2, "Speed").mod (macro2, "lfo1Rate", 12).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).drive (crush, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Flat Static", "Chip").set ("osc1On", 0).noise (80, 3, 80).env (2, 0, 90, 0, 90, 70).mod (env2, "noiseColour", 20)
            .filter (1, off, 8000).amp (0, 90, 0, 70, 80).level (-3).macro (1, "Pitch").mod (macro1, "noiseColour", 35).drive (crush, 0, 8000)
            .macro (2, "Grit").mod (macro2, "fxDriveAmount", 55).delay (sixteenth, 30, 0, 8000).macro (3, "Echo").mod (macro3, "fxDelayMix", 35)
            .reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Fluorescent Ramp", "Chip").osc (1, analog, trianglePos).warp (1, wCrush, 58).pitch (1, 0).env (2, 0, 30, 0, 60, 80)
            .mod (env2, "osc1Pitch", 50).filter (1, off, 8000).amp (0, 300, 0, 110).mono (0, false).level (-4).delay (eighth, 30, 0, 6000)
            .macro (1, "Steps").mod (macro1, "osc1WarpAmount", -30).drive (crush, 0, 8000).macro (2, "Grit").mod (macro2, "fxDriveAmount", 55)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Forgotten Dive", "Chip").osc (1, pulse, 0).pitch (1, 0).env (2, 0, 350, 0, 100, 65).mod (env2, "osc1Pitch", 50).filter (1, off, 8000)
            .amp (0, 160, 0, 100, 70).level (-12).macro (1, "Fall").mod (macro1, "osc1Pitch", 25).drive (crush, 0, 8000).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 55).delay (sixteenth, 30, 0, 8000).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Fugitive Phase", "Chip").osc (1, pulse, 40).lfo (1, steps, 0.5f)
            .stepPattern (1, { 30, 30, 30, -100, 30, 30, 30, -100, 30, 30, 30, -100, 30, 30, 30, -100 }).mod (lfo1, "osc1Position", 30)
            .osc (2, pulse, 0, 60).pitch (2, 7, 8).filter (1, off, 8000).amp (0, 300, 60, 160).delay (eighthD, 30, 22, 3000, false).level (-14)
            .macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Speed").mod (macro2, "lfo1Rate", 12).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Gaunt Crash", "Chip").set ("osc1On", 0).noise (100, 3, 60).env (2, 0, 60, 0, 60).mod (env2, "noiseColour", 15).filter (1, off, 8000)
            .amp (0, 60, 0, 90, 40).level (-9).macro (1, "Pitch").mod (macro1, "noiseColour", 35).drive (crush, 0, 8000).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 55).delay (sixteenth, 30, 0, 8000).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Gaunt Noise", "Chip").set ("osc1On", 0).noise (80, 3, 40).env (2, 0, 400, 0, 400).mod (env2, "noiseColour", 15).filter (1, off, 8000)
            .amp (0, 400, 0, 90, 70).level (-6).macro (1, "Pitch").mod (macro1, "noiseColour", 35).drive (crush, 0, 8000).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 55).delay (sixteenth, 30, 0, 8000).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Glinting Duet", "Chip").osc (1, pulse, 80).osc (2, pulse, 0, 70).pitch (2, -12, 6).pan (1, -20).pan (2, 0).lfo (1, sine, 7)
            .lfoFade (1, 400).mod (lfo1, "osc1Fine", 7).mod (lfo1, "osc2Fine", 6).filter (1, off, 8000).amp (0, 500, 50, 70)
            .delay (eighthD, 20, 24, 4100).level (-13).macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Detune")
            .mod (macro2, "osc2Fine", 20).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (crush, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Guarded Triad", "Chip").osc (1, pulse, 80).lfo (1, steps, 5)
            .stepPattern (1, { 0, 16.67f, 37.5f, 50, 0, 16.67f, 37.5f, 50, 0, 16.67f, 37.5f, 50, 0, 16.67f, 37.5f, 50 }).mod (lfo1, "osc1Pitch", 50)
            .filter (1, off, 8000).amp (0, 900, 0, 80).delay (eighth, 50, 18, 2200, false).level (-12).macro (1, "Duty")
            .mod (macro1, "osc1Position", 30).macro (2, "Speed").mod (macro2, "lfo1Rate", 12).macro (3, "Echo").mod (macro3, "fxDelayMix", 35)
            .drive (crush, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Gutted Twin", "Chip").osc (1, pulse, 0).osc (2, pulse, 53).pitch (2, 0, 16).pan (1, -10).pan (2, 10).lfo (1, triangle, 7.5f)
            .lfoFade (1, 100).mod (lfo1, "osc1Fine", 7).mod (lfo1, "osc2Fine", 4).filter (1, off, 8000).amp (0, 450, 80, 50)
            .delay (sixteenth, 40, 12, 6900).level (-17).macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Detune")
            .mod (macro2, "osc2Fine", 20).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (crush, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Heavy Broken", "Chip").osc (1, pulse, 80).lfo (1, steps, 5)
            .stepPattern (1, { 0, 12.5f, 29.17f, 50, 0, 12.5f, 29.17f, 50, 0, 12.5f, 29.17f, 50, 0, 12.5f, 29.17f, 50 }).mod (lfo1, "osc1Pitch", 50)
            .filter (1, off, 8000).amp (0, 400, 60, 30).level (-12).macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 12).delay (eighth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).drive (crush, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Indexed Arpeggio", "Chip").osc (1, pulse, 0).lfo (1, steps, 1.5f)
            .stepPattern (1, { 0, 12.5f, 29.17f, 50, 0, 12.5f, 29.17f, 50, 0, 12.5f, 29.17f, 50, 0, 12.5f, 29.17f, 50 }).mod (lfo1, "osc1Pitch", 50)
            .filter (1, off, 8000).amp (0, 350, 0, 50).level (-13).macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 12).delay (eighth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).drive (crush, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Indexed Pair", "Chip").osc (1, pulse, 80).osc (2, pulse, 80, 65).pitch (2, 7, 8).pan (1, -10).pan (2, 0).lfo (1, sine, 7)
            .lfoFade (1, 400).mod (lfo1, "osc1Fine", 7).mod (lfo1, "osc2Fine", 11).filter (1, off, 8000).amp (0, 600, 60, 90)
            .delay (sixteenthD, 35, 20, 6600).level (-14).macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Detune")
            .mod (macro2, "osc2Fine", 20).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (crush, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Leaning Shimmer", "Chip").osc (1, pulse, 80).lfo (1, steps, 1.5f)
            .stepPattern (1, { 0, 16.67f, 37.5f, 50, 0, 16.67f, 37.5f, 50, 0, 16.67f, 37.5f, 50, 0, 16.67f, 37.5f, 50 }).mod (lfo1, "osc1Pitch", 50)
            .filter (1, off, 8000).amp (0, 400, 0, 170).delay (sixteenthD, 25, 22, 1900).level (-9).macro (1, "Duty")
            .mod (macro1, "osc1Position", 30).macro (2, "Speed").mod (macro2, "lfo1Rate", 12).macro (3, "Echo").mod (macro3, "fxDelayMix", 35)
            .drive (crush, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Lone Bass", "Chip").osc (1, analog, trianglePos).warp (1, wCrush, 54).pitch (1, 0).filter (1, off, 8000).amp (0, 300, 100, 40).mono()
            .level (-4).delay (eighth, 30, 0, 6000).macro (1, "Steps").mod (macro1, "osc1WarpAmount", -30).drive (crush, 0, 8000).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 55).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Mirror Shimmer", "Chip").osc (1, pulse, 80).lfo (1, steps, 1.5f)
            .stepPattern (1, { 0, 16.67f, 37.5f, 50, 0, 16.67f, 37.5f, 50, 0, 16.67f, 37.5f, 50, 0, 16.67f, 37.5f, 50 }).mod (lfo1, "osc1Pitch", 50)
            .filter (1, off, 8000).amp (0, 200, 0, 150).level (-9).macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 12).delay (eighth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).drive (crush, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Needle Plunge", "Chip").osc (1, pulse, 53).pitch (1, 0).env (2, 0, 320, 0, 100, 45).mod (env2, "osc1Pitch", 75).filter (1, off, 8000)
            .amp (0, 120, 0, 20, 80).level (-12).macro (1, "Fall").mod (macro1, "osc1Pitch", 25).drive (crush, 0, 8000).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 55).delay (sixteenth, 30, 0, 8000).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Platinum Square", "Chip").osc (1, pulse, 0).lfo (1, sine, 5.75f).lfoFade (1, 550).mod (lfo1, "osc1Fine", 18).filter (1, off, 8000)
            .amp (0, 200, 50, 100).mono (0, false).level (-4).macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Octave")
            .mod (macro2, "osc1Pitch", 25).delay (eighth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Restless Morph", "Chip").osc (1, pulse, 40).lfo (1, steps, 4)
            .stepPattern (1, { -100, 100, 100, -30, -100, 100, 100, -30, -100, 100, 100, -30, -100, 100, 100, -30 }).mod (lfo1, "osc1Position", 35)
            .osc (2, pulse, 0, 50).pitch (2, -12, 8).filter (1, off, 8000).amp (0, 300, 80, 40).delay (eighth, 35, 20, 5400).level (-14)
            .macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Speed").mod (macro2, "lfo1Rate", 12).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Ruined Pulse", "Chip").osc (1, pulse, 80).lfo (1, sine, 6.5f).lfoFade (1, 550).mod (lfo1, "osc1Fine", 9).filter (1, off, 8000)
            .amp (0, 350, 90, 60).mono (60, false).level (-4).macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Octave")
            .mod (macro2, "osc1Pitch", 25).delay (eighth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Scrambled Triangle", "Chip").osc (1, analog, trianglePos).warp (1, wCrush, 54).pitch (1, -12).env (2, 0, 40, 0, 60, 80)
            .mod (env2, "osc1Pitch", 50).filter (1, off, 8000).amp (0, 650, 0, 30).mono (0, false).level (-4).delay (eighth, 30, 0, 6000)
            .macro (1, "Steps").mod (macro1, "osc1WarpAmount", -30).drive (crush, 0, 8000).macro (2, "Grit").mod (macro2, "fxDriveAmount", 55)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Sealed Ramp", "Chip").osc (1, analog, trianglePos).warp (1, wCrush, 52).pitch (1, 0).filter (1, off, 8000).amp (0, 600, 0, 50)
            .mono (0, false).level (-3).delay (eighth, 30, 0, 6000).macro (1, "Steps").mod (macro1, "osc1WarpAmount", -30).drive (crush, 0, 8000)
            .macro (2, "Grit").mod (macro2, "fxDriveAmount", 55).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Sector Slide", "Chip").osc (1, analog, trianglePos).warp (1, wCrush, 60).pitch (1, 0).env (2, 0, 380, 0, 100, 80)
            .mod (env2, "osc1Pitch", 25).filter (1, off, 8000).amp (0, 200, 0, 140, 70).level (-11).macro (1, "Fall").mod (macro1, "osc1Pitch", 25)
            .drive (crush, 0, 8000).macro (2, "Grit").mod (macro2, "fxDriveAmount", 55).delay (sixteenth, 30, 0, 8000).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Serial Triad", "Chip").osc (1, pulse, 0).lfo (1, steps, 2.5f)
            .stepPattern (1, { 0, 16.67f, 37.5f, 50, 0, 16.67f, 37.5f, 50, 0, 16.67f, 37.5f, 50, 0, 16.67f, 37.5f, 50 }).mod (lfo1, "osc1Pitch", 50)
            .filter (1, off, 8000).amp (0, 200, 0, 120).delay (sixteenthD, 55, 18, 1900).level (-13).macro (1, "Duty")
            .mod (macro1, "osc1Position", 30).macro (2, "Speed").mod (macro2, "lfo1Rate", 12).macro (3, "Echo").mod (macro3, "fxDelayMix", 35)
            .drive (crush, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Short Fuzz", "Chip").set ("osc1On", 0).noise (80, 3, -20).env (2, 0, 60, 0, 60).mod (env2, "noiseColour", -40).filter (1, off, 8000)
            .amp (0, 60, 0, 70, 40).level (-1).macro (1, "Pitch").mod (macro1, "noiseColour", 35).drive (crush, 0, 8000).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 55).delay (sixteenth, 30, 0, 8000).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Slanted Sweep", "Chip").osc (1, pulse, 40).lfo (1, steps, 1.5f)
            .stepPattern (1, { 100, -100, -30, 100, 100, -100, -30, 100, 100, -100, -30, 100, 100, -100, -30, 100 }).mod (lfo1, "osc1Position", 40)
            .filter (1, off, 8000).amp (0, 450, 100, 140).level (-13).macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 12).delay (eighth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Sodden Hiss", "Chip").set ("osc1On", 0).noise (90, 3, 20).env (2, 0, 150, 0, 150).mod (env2, "noiseColour", -20).filter (1, off, 8000)
            .amp (0, 150, 0, 140, 70).level (-6).macro (1, "Pitch").mod (macro1, "noiseColour", 35).drive (crush, 0, 8000).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 55).delay (sixteenth, 30, 0, 8000).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Squat Shift", "Chip").osc (1, pulse, 40).lfo (1, steps, 1)
            .stepPattern (1, { -100, -30, -30, -100, -100, -30, -30, -100, -100, -30, -30, -100, -100, -30, -30, -100 })
            .mod (lfo1, "osc1Position", 30).filter (1, off, 8000).amp (0, 600, 30, 100).level (-12).macro (1, "Duty")
            .mod (macro1, "osc1Position", 30).macro (2, "Speed").mod (macro2, "lfo1Rate", 12).delay (eighth, 40, 0, 4500).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Stalking Arpeggio", "Chip").osc (1, pulse, 53).lfo (1, steps, 1.5f)
            .stepPattern (1, { 0, 50, 0, 50, 0, 50, 0, 50, 0, 50, 0, 50, 0, 50, 0, 50 }).mod (lfo1, "osc1Pitch", 50).filter (1, off, 8000)
            .amp (0, 450, 80, 180).level (-12).macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Speed").mod (macro2, "lfo1Rate", 12)
            .delay (eighth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).drive (crush, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Stark Voice", "Chip").osc (1, pulse, 53).lfo (1, triangle, 7.75f).lfoFade (1, 150).mod (lfo1, "osc1Fine", 10).filter (1, off, 8000)
            .amp (0, 150, 80, 100).mono (60).level (-4).macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Octave")
            .mod (macro2, "osc1Pitch", 25).delay (eighth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Stocky Dive", "Chip").osc (1, analog, trianglePos).warp (1, wCrush, 56).pitch (1, 0).env (2, 0, 140, 0, 100, 85)
            .mod (env2, "osc1Pitch", 50).filter (1, off, 8000).amp (0, 120, 0, 120, 80).level (-11).macro (1, "Fall").mod (macro1, "osc1Pitch", 25)
            .drive (crush, 0, 8000).macro (2, "Grit").mod (macro2, "fxDriveAmount", 55).delay (sixteenth, 30, 0, 8000).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Sullen Ramp", "Chip").osc (1, analog, trianglePos).warp (1, wCrush, 58).pitch (1, -12).env (2, 0, 110, 0, 60, 80)
            .mod (env2, "osc1Pitch", 14.583f).filter (1, off, 8000).amp (0, 200, 100, 20).mono().level (-4).delay (eighth, 30, 0, 6000)
            .macro (1, "Steps").mod (macro1, "osc1WarpAmount", -30).drive (crush, 0, 8000).macro (2, "Grit").mod (macro2, "fxDriveAmount", 55)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Surgical Mirror", "Chip").osc (1, pulse, 0).osc (2, pulse, 80, 70).pitch (2, 12, 14).pan (1, -10).pan (2, 20).lfo (1, triangle, 6.25f)
            .lfoFade (1, 400).mod (lfo1, "osc1Fine", 13).mod (lfo1, "osc2Fine", 13).filter (1, off, 8000).amp (0, 250, 90, 150)
            .delay (sixteenthD, 35, 16, 6600).level (-17).macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Detune")
            .mod (macro2, "osc2Fine", 20).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (crush, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Synthetic Mirror", "Chip").osc (1, pulse, 53).osc (2, pulse, 80).pitch (2, -12, 14).pan (1, -30).pan (2, 10).lfo (1, sine, 6.25f)
            .lfoFade (1, 300).mod (lfo1, "osc1Fine", 12).mod (lfo1, "osc2Fine", 7).filter (1, off, 8000).amp (0, 400, 80, 150)
            .delay (eighth, 35, 22, 7000, false).level (-16).macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Detune")
            .mod (macro2, "osc2Fine", 20).macro (3, "Echo").mod (macro3, "fxDelayMix", 30).drive (crush, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Thin Step", "Chip").osc (1, analog, trianglePos).warp (1, wCrush, 58).pitch (1, -12).env (2, 0, 100, 0, 60, 80)
            .mod (env2, "osc1Pitch", 14.583f).filter (1, off, 8000).amp (0, 250, 0, 100).mono (0, false).level (-3).delay (eighth, 30, 0, 6000)
            .macro (1, "Steps").mod (macro1, "osc1WarpAmount", -30).drive (crush, 0, 8000).macro (2, "Grit").mod (macro2, "fxDriveAmount", 55)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Ticking Shift", "Chip").osc (1, pulse, 40).lfo (1, steps, 2)
            .stepPattern (1, { 100, 30, -100, -100, 100, 30, -100, -100, 100, 30, -100, -100, 100, 30, -100, -100 }).mod (lfo1, "osc1Position", 30)
            .filter (1, off, 8000).amp (0, 650, 50, 30).level (-12).macro (1, "Duty").mod (macro1, "osc1Position", 30).macro (2, "Speed")
            .mod (macro2, "lfo1Rate", 12).delay (eighth, 40, 0, 4500).macro (3, "Echo").mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Tight Tumble", "Chip").osc (1, analog, trianglePos).warp (1, wCrush, 56).pitch (1, -5).env (2, 0, 300, 0, 100, 20)
            .mod (env2, "osc1Pitch", 75).filter (1, off, 8000).amp (0, 600, 0, 50, 40).level (-12).macro (1, "Fall").mod (macro1, "osc1Pitch", 25)
            .drive (crush, 0, 8000).macro (2, "Grit").mod (macro2, "fxDriveAmount", 55).delay (sixteenth, 30, 0, 8000).macro (3, "Echo")
            .mod (macro3, "fxDelayMix", 35).reverb (0, 40, 1, 45).macro (4, "Space").mod (macro4, "fxReverbMix", 35);

        // Texture, by name.
        add ("Acrid Works", "Texture").osc (1, pulse, 70, 50).pitch (1, -12).noise (50).filter (1, comb, 65, 55).filter (2, svfBand, 920, 40)
            .lfoSync (1, steps, bars2, 0, 80).stepPattern (1, { 0, -50, 100, 0, 50, -50, 50, 100, -50, 50, 50, 50, -100, 100, 50, -50 })
            .mod (lfo1, "filter2Cutoff", 12).lfo (2, drift, 0.2f, 0).mod (lfo2, "filter1Cutoff", 6).amp (600, 1000, 100, 1700)
            .drive (hard, 35, 2800).reverb (15, 75, 8, 30).level (-9).macro (1, "Cutoff").mod (macro1, "filter2Cutoff", 22).macro (2, "Ring")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Air").mod (macro3, "noiseLevel", -40).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Ashen Smoulder", "Texture").set ("osc1On", 0).noise (85, 2, 60).filter (1, svfLow, 500, 25).lfo (1, randomShape, 0.35f, 0)
            .mod (lfo1, "filter1Cutoff", 17).mod (lfo1, "ampLevel", 6).amp (1000, 1000, 100, 1500).drive (tube, 15, 3300).reverb (25, 95, 5, 40)
            .level (6).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Colour").mod (macro2, "noiseColour", -40).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Resonance").mod (macro4, "filter1Resonance", 35);
        add ("Boarded Feed", "Texture").set ("osc1On", 0).noise (90, 3).lfoSync (1, steps, bars2, 0, 20)
            .stepPattern (1, { -20, 20, 60, -100, 60, -20, 100, -60, 60, -60, 100, -60, -20, -100, -20, 100 }).mod (lfo1, "noiseColour", 20)
            .lfo (2, drift, 0.49f, 0).mod (lfo2, "filter1Cutoff", 5).filter (1, ladderBand, 1100, 25).amp (1300, 1000, 100, 2100)
            .delay (sixteenth, 50, 26, 7900).reverb (30, 90, 7.5f, 40).level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20)
            .macro (2, "Rate").mod (macro2, "noiseColour", 30).macro (3, "Resonance").mod (macro3, "filter1Resonance", 35).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Bound Cloud", "Texture").osc (1, metal, 75).unison (1, 8, 70, 100, 0, 70).pitch (1, 0).filter (1, svfBand, 4200, 45)
            .lfo (1, randomShape, 2, 0).mod (lfo1, "osc1UnisonDetune", 16).mod (lfo1, "ampLevel", 4).amp (400, 1000, 100, 2300)
            .reverb (25, 95, 4.5f, 45).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Scatter")
            .mod (macro2, "osc1UnisonDetune", 20).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Bruised Static", "Texture").osc (1, analog, sinePos, 1).pitch (1, 17, -10).osc (2, analog, sinePos).warp (2, wFm, 60).noise (25)
            .lfo (1, drift, 2.1f, 0).mod (lfo1, "osc2WarpAmount", 15).mod (lfo1, "osc2Fine", 40).lfo (2, drift, 0.069f, 0)
            .mod (lfo2, "filter1Cutoff", 5).filter (1, svfBand, 1400, 65).amp (200, 1000, 100, 1300).drive (hard, 35, 3800).reverb (25, 70, 7, 65)
            .level (-22).macro (1, "Tune").mod (macro1, "filter1Cutoff", 18).macro (2, "Static").mod (macro2, "noiseLevel", 35).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Motion").mod (macro4, "lfo1Rate", 25);
        add ("Cellar Plant", "Texture").osc (1, pulse, 70, 60).pitch (1, -12).noise (45, 2).filter (1, comb, 81, 60).filter (2, svfBand, 1400, 35)
            .lfoSync (1, steps, half, 0, 50).stepPattern (1, { -100, 100, 100, 0, 100, -50, 50, -100, -100, 50, 50, -50, -100, -100, 50, 50 })
            .mod (lfo1, "filter2Cutoff", 12).lfo (2, drift, 0.053f, 0).mod (lfo2, "filter1Cutoff", 6).amp (1500, 1000, 100, 2300)
            .drive (hard, 35, 3100).reverb (20, 90, 7.5f, 65).level (-9).macro (1, "Cutoff").mod (macro1, "filter2Cutoff", 22).macro (2, "Ring")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Air")
            .mod (macro4, "noiseLevel", -40);
        add ("Circling Log", "Texture").set ("osc1On", 0).noise (75, 3).lfoSync (1, steps, bars2, 0)
            .stepPattern (1, { 60, -20, -60, 60, 60, -20, -100, 60, 100, 60, 100, 20, -100, 20, -60, -60 }).mod (lfo1, "noiseColour", 35)
            .lfo (2, randomShape, 0.35f, 0).mod (lfo2, "filter1Cutoff", 14).filter (1, ladderBand, 2900, 55).amp (1000, 1000, 100, 1300)
            .delay (sixteenthT, 55, 12, 2700).reverb (30, 95, 5, 70).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20)
            .macro (2, "Rate").mod (macro2, "noiseColour", 30).drive (crush, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Covert Gale", "Texture").set ("osc1On", 0).noise (75, 0, -40).filter (1, ladder24, 1500, 76).lfo (1, drift, 0.097f, 0)
            .mod (lfo1, "filter1Cutoff", 14).lfo (2, sine, 0.35f, 0).mod (lfo2, "ampLevel", 16).mod (lfo2, "filter1Resonance", 11)
            .amp (1000, 1000, 100, 1300).reverb (25, 75, 9, 60).level (-3).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Howl")
            .mod (macro2, "filter1Resonance", 10).macro (3, "Gusts").mod (macro3, "lfo1Rate", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Digital Shortwave", "Texture").osc (1, analog, sinePos, 20).pitch (1, 12, -20).osc (2, analog, sinePos).warp (2, wFm, 45).noise (40)
            .lfo (1, randomShape, 4.8f, 0).mod (lfo1, "osc2WarpAmount", 40).mod (lfo1, "osc2Fine", 10).lfo (2, drift, 0.075f, 0)
            .mod (lfo2, "filter1Cutoff", 10).filter (1, ladderBand, 1400, 60).amp (900, 1000, 100, 1100).drive (crush, 20, 2600)
            .reverb (20, 80, 6, 30).level (-16).macro (1, "Tune").mod (macro1, "filter1Cutoff", 18).macro (2, "Static")
            .mod (macro2, "noiseLevel", 35).macro (3, "Motion").mod (macro3, "lfo1Rate", 25).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Dormant Kindling", "Texture").set ("osc1On", 0).noise (100, 2, 20).filter (1, ladderBand, 590, 50).lfo (1, drift, 0.54f, 0)
            .mod (lfo1, "filter1Cutoff", 9).mod (lfo1, "ampLevel", 10).amp (1300, 1000, 100, 1000).reverb (20, 75, 4, 50).level (2)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Colour").mod (macro2, "noiseColour", -40).macro (3, "Motion")
            .mod (macro3, "lfo1Rate", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Faint Flurry", "Texture").set ("osc1On", 0).noise (95, 1).filter (1, ladder24, 580, 61).lfo (1, drift, 0.5f, 0)
            .mod (lfo1, "filter1Cutoff", 8).lfo (2, drift, 0.4f, 0).mod (lfo2, "ampLevel", 12).mod (lfo2, "filter1Resonance", 8)
            .amp (200, 1000, 100, 900).reverb (30, 90, 4.5f, 30).level (0).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Howl")
            .mod (macro2, "filter1Resonance", 10).macro (3, "Gusts").mod (macro3, "lfo1Rate", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Flayed Carrier", "Texture").osc (1, analog, sinePos, 20).pitch (1, 23).osc (2, analog, trianglePos).warp (2, wFm, 30).noise (45)
            .lfo (1, randomShape, 1.8f, 0).mod (lfo1, "osc2WarpAmount", 30).mod (lfo1, "osc2Fine", 10).lfo (2, drift, 0.08f, 0)
            .mod (lfo2, "filter1Cutoff", 11).filter (1, svfBand, 2100, 40).amp (1400, 1000, 100, 1900).drive (hard, 25, 5700)
            .reverb (10, 75, 7.5f, 40).level (-19).macro (1, "Tune").mod (macro1, "filter1Cutoff", 18).macro (2, "Static")
            .mod (macro2, "noiseLevel", 35).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Formless Tinder", "Texture").set ("osc1On", 0).noise (95, 2, 60).filter (1, ladderBand, 770, 20).lfo (1, drift, 0.79f, 0)
            .mod (lfo1, "filter1Cutoff", 8).mod (lfo1, "ampLevel", 12).amp (800, 1000, 100, 2500).drive (crush, 15, 7800).reverb (25, 75, 7, 70)
            .level (5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Colour").mod (macro2, "noiseColour", -40)
            .macro (3, "Motion").mod (macro3, "lfo1Rate", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Frosted Embers", "Texture").set ("osc1On", 0).noise (85, 2, 40).filter (1, svfHigh, 4000, 15).lfo (1, randomShape, 5.3f, 0)
            .mod (lfo1, "filter1Cutoff", 16).mod (lfo1, "ampLevel", 12).amp (1100, 1000, 100, 2100).reverb (20, 80, 3.5f, 35).level (0)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Colour").mod (macro2, "noiseColour", -40).drive (crush, 0, 9000)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Motion").mod (macro4, "lfo1Rate", 25);
        add ("Fugitive Gale", "Texture").set ("osc1On", 0).noise (70, 1, -40).filter (1, ladderBand, 510, 82).lfo (1, drift, 0.39f, 0)
            .mod (lfo1, "filter1Cutoff", 14).lfo (2, sine, 0.19f, 0).mod (lfo2, "ampLevel", 8).mod (lfo2, "filter1Resonance", 9)
            .amp (600, 1000, 100, 1400).reverb (30, 100, 7.5f, 45).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20)
            .macro (2, "Howl").mod (macro2, "filter1Resonance", 10).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Gusts")
            .mod (macro4, "lfo1Rate", 25);
        add ("Galvanised Plant", "Texture").osc (1, pulse, 80, 70).pitch (1, 0).noise (40, 2).filter (1, comb, 310, 60)
            .filter (2, ladder24, 1400, 25).lfoSync (1, steps, bars2, 0, 70)
            .stepPattern (1, { 0, 100, -100, -100, 0, -50, -100, 0, -50, 0, -100, 100, 100, -50, 0, -100 }).mod (lfo1, "filter2Cutoff", 8)
            .lfo (2, drift, 0.13f, 0).mod (lfo2, "filter1Cutoff", 4).amp (900, 1000, 100, 1400).drive (tube, 35, 3800).reverb (25, 95, 7.5f, 40)
            .level (-13).macro (1, "Cutoff").mod (macro1, "filter2Cutoff", 22).macro (2, "Ring").mod (macro2, "filter1Resonance", 25)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Air").mod (macro4, "noiseLevel", -40);
        add ("Glinting Cloud", "Texture").osc (1, metal, 95).unison (1, 8, 30, 100, 0, 80).pitch (1, 0).filter (1, ladderHigh, 1000, 15)
            .lfo (1, drift, 6.2f, 0).mod (lfo1, "osc1UnisonDetune", 10).mod (lfo1, "ampLevel", 9).amp (300, 1000, 100, 2300).reverb (15, 90, 3, 45)
            .level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Scatter").mod (macro2, "osc1UnisonDetune", 20)
            .macro (3, "Motion").mod (macro3, "lfo1Rate", 25).macro (4, "Resonance").mod (macro4, "filter1Resonance", 35);
        add ("Hollowed Band", "Texture").osc (1, analog, sinePos, 20).pitch (1, 17, -20).osc (2, analog, trianglePos).warp (2, wFm, 20).noise (50)
            .lfo (1, drift, 2.1f, 0).mod (lfo1, "osc2WarpAmount", 40).mod (lfo1, "osc2Fine", 20).lfo (2, drift, 0.05f, 0)
            .mod (lfo2, "filter1Cutoff", 6).filter (1, ladderBand, 3300, 50).amp (900, 1000, 100, 2300).drive (tube, 30, 3000)
            .reverb (30, 75, 7.5f, 60).level (-17).macro (1, "Tune").mod (macro1, "filter1Cutoff", 18).macro (2, "Static")
            .mod (macro2, "noiseLevel", 35).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Motion").mod (macro4, "lfo1Rate", 25);
        add ("Iron Brood", "Texture").osc (1, metal, 70).unison (1, 8, 40, 100, 0, 80).pitch (1, 0).filter (1, ladderHigh, 1900, 45)
            .lfo (1, drift, 0.39f, 0).mod (lfo1, "osc1UnisonDetune", 14).mod (lfo1, "ampLevel", 7).amp (1100, 1000, 100, 1900)
            .reverb (40, 90, 8, 35).level (-1).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Scatter")
            .mod (macro2, "osc1UnisonDetune", 20).drive (hard, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 25);
        add ("Lawless Band", "Texture").osc (1, analog, sinePos, 20).pitch (1, 12, -30).osc (2, analog, sinePos).warp (2, wFm, 25).noise (55, 2)
            .lfo (1, randomShape, 1.7f, 0).mod (lfo1, "osc2WarpAmount", 35).mod (lfo1, "osc2Fine", 20).lfo (2, drift, 0.13f, 0)
            .mod (lfo2, "filter1Cutoff", 7).filter (1, ladderBand, 1800, 50).amp (1000, 1000, 100, 1500).reverb (20, 85, 4, 70).level (-17)
            .macro (1, "Tune").mod (macro1, "filter1Cutoff", 18).macro (2, "Static").mod (macro2, "noiseLevel", 35).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).drive (crush, 0, 9000).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Listing Squall", "Texture").set ("osc1On", 0).noise (85, 1, -40).filter (1, ladder24, 1300, 67).lfo (1, drift, 0.34f, 0)
            .mod (lfo1, "filter1Cutoff", 16).lfo (2, sine, 0.18f, 0).mod (lfo2, "ampLevel", 8).mod (lfo2, "filter1Resonance", 6)
            .amp (1400, 1000, 100, 1000).reverb (20, 70, 6.5f, 40).level (-7).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20)
            .macro (2, "Howl").mod (macro2, "filter1Resonance", 10).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Gusts").mod (macro4, "lfo1Rate", 25);
        add ("Lurching Works", "Texture").osc (1, pulse, 70, 70).pitch (1, -12).noise (45, 1).filter (1, comb, 79, 80)
            .filter (2, ladder24, 3400, 20).lfoSync (1, steps, bars2, 0, 80)
            .stepPattern (1, { 50, -100, 0, 0, 0, 50, 100, 100, -50, 0, 0, -50, 100, 50, 50, 50 }).mod (lfo1, "filter2Cutoff", 10)
            .lfo (2, drift, 0.062f, 0).mod (lfo2, "filter1Cutoff", 2).amp (1500, 1000, 100, 2200).drive (hard, 20, 9000).reverb (15, 70, 6.5f, 35)
            .level (-11).macro (1, "Cutoff").mod (macro1, "filter2Cutoff", 22).macro (2, "Ring").mod (macro2, "filter1Resonance", 25)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Air").mod (macro4, "noiseLevel", -40);
        add ("Magnesium Swarm", "Texture").osc (1, metal, 55).unison (1, 8, 50, 100, 0, 100).pitch (1, 12).filter (1, svfHigh, 1800, 25)
            .lfo (1, drift, 3.5f, 0).mod (lfo1, "osc1UnisonDetune", 18).mod (lfo1, "ampLevel", 4).amp (600, 1000, 100, 1900).reverb (20, 90, 5, 60)
            .level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Scatter").mod (macro2, "osc1UnisonDetune", 20)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Motion").mod (macro4, "lfo1Rate", 25);
        add ("Mangled Embers", "Texture").set ("osc1On", 0).noise (100, 2, 40).filter (1, svfBand, 590, 35).lfo (1, randomShape, 1.3f, 0)
            .mod (lfo1, "filter1Cutoff", 11).mod (lfo1, "ampLevel", 12).amp (1000, 1000, 100, 1100).drive (hard, 20, 3600).reverb (15, 90, 9, 50)
            .level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Colour").mod (macro2, "noiseColour", -40)
            .macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Mothballed Works", "Texture").osc (1, pulse, 75, 55).pitch (1, -12).noise (70, 1).filter (1, comb, 140, 80)
            .filter (2, ladder24, 1800, 45).lfoSync (1, steps, half, 0, 40)
            .stepPattern (1, { -50, -50, -100, 50, 0, 100, -50, 0, 0, 0, 0, 100, 100, -50, 50, 50 }).mod (lfo1, "filter2Cutoff", 14)
            .lfo (2, drift, 0.12f, 0).mod (lfo2, "filter1Cutoff", 7).amp (200, 1000, 100, 1100).drive (hard, 25, 2700).reverb (35, 80, 3, 40)
            .level (-11).macro (1, "Cutoff").mod (macro1, "filter2Cutoff", 22).macro (2, "Ring").mod (macro2, "filter1Resonance", 25)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Air").mod (macro4, "noiseLevel", -40);
        add ("Overloaded Mill", "Texture").osc (1, pulse, 85, 55).pitch (1, 0).noise (50).filter (1, comb, 170, 70).filter (2, ladder24, 1100, 25)
            .lfoSync (1, steps, half, 0, 40).stepPattern (1, { 0, -100, -50, 0, 100, 0, -50, 100, -50, 100, -50, 100, 100, 50, 0, 100 })
            .mod (lfo1, "filter2Cutoff", 16).lfo (2, drift, 0.13f, 0).mod (lfo2, "filter1Cutoff", 4).amp (1500, 1000, 100, 1200)
            .drive (hard, 35, 5300).reverb (30, 75, 7.5f, 30).level (-13).macro (1, "Cutoff").mod (macro1, "filter2Cutoff", 22).macro (2, "Ring")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Padded Longwave", "Texture").osc (1, analog, sinePos, 1).pitch (1, 7, 10).osc (2, analog, sinePos).warp (2, wFm, 30).noise (40, 2)
            .lfo (1, randomShape, 1.9f, 0).mod (lfo1, "osc2WarpAmount", 30).mod (lfo1, "osc2Fine", 10).lfo (2, drift, 0.46f, 0)
            .mod (lfo2, "filter1Cutoff", 6).filter (1, ladderBand, 1100, 50).amp (200, 1000, 100, 2200).reverb (20, 100, 3.5f, 50).level (-16)
            .macro (1, "Tune").mod (macro1, "filter1Cutoff", 18).macro (2, "Static").mod (macro2, "noiseLevel", 35).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Motion").mod (macro4, "lfo1Rate", 25);
        add ("Pallid Wind", "Texture").set ("osc1On", 0).noise (95, 0, -60).filter (1, ladderBand, 1300, 85).lfo (1, drift, 0.16f, 0)
            .mod (lfo1, "filter1Cutoff", 18).lfo (2, sine, 0.11f, 0).mod (lfo2, "ampLevel", 10).mod (lfo2, "filter1Resonance", 5)
            .amp (400, 1000, 100, 2300).reverb (20, 70, 9, 55).level (-6).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Howl")
            .mod (macro2, "filter1Resonance", 10).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Gusts")
            .mod (macro4, "lfo1Rate", 25);
        add ("Parsed Trace", "Texture").set ("osc1On", 0).noise (90, 3).lfoSync (1, steps, bar1, 0)
            .stepPattern (1, { 60, -20, -20, -20, -20, -100, 100, -60, 60, 20, -60, -100, -20, 100, -100, 60 }).mod (lfo1, "noiseColour", 30)
            .lfo (2, drift, 0.65f, 0).mod (lfo2, "filter1Cutoff", 11).filter (1, svfLow, 1000, 25).amp (800, 1000, 100, 1300).reverb (20, 80, 6, 30)
            .level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Rate").mod (macro2, "noiseColour", 30)
            .drive (crush, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Resonance").mod (macro4, "filter1Resonance", 35);
        add ("Patient Cinders", "Texture").set ("osc1On", 0).noise (95, 2, 40).filter (1, ladderBand, 760, 40).lfo (1, drift, 0.76f, 0)
            .mod (lfo1, "filter1Cutoff", 18).mod (lfo1, "ampLevel", 8).amp (400, 1000, 100, 1000).reverb (25, 100, 6.5f, 55).level (3)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Colour").mod (macro2, "noiseColour", -40).macro (3, "Motion")
            .mod (macro3, "lfo1Rate", 25).macro (4, "Resonance").mod (macro4, "filter1Resonance", 35);
        add ("Rabid Yard", "Texture").osc (1, pulse, 50, 55).pitch (1, 0).noise (65).filter (1, comb, 100, 55).filter (2, ladder24, 2100, 20)
            .lfoSync (1, steps, bar1, 0, 80).stepPattern (1, { -50, 50, 50, 100, -100, 0, 50, 100, -50, -100, 50, -100, -50, 0, -50, 0 })
            .mod (lfo1, "filter2Cutoff", 8).lfo (2, drift, 0.3f, 0).mod (lfo2, "filter1Cutoff", 2).amp (700, 1000, 100, 1000).drive (hard, 15, 6000)
            .reverb (30, 75, 3, 55).level (-9).macro (1, "Cutoff").mod (macro1, "filter2Cutoff", 22).macro (2, "Ring")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);
        add ("Remote Brood", "Texture").osc (1, metal, 65).unison (1, 8, 45, 100, 0, 100).pitch (1, 0).filter (1, svfHigh, 1100, 30)
            .lfo (1, drift, 1.9f, 0).mod (lfo1, "osc1UnisonDetune", 8).mod (lfo1, "ampLevel", 9).amp (1100, 1000, 100, 2000).reverb (40, 80, 8, 70)
            .level (-11).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Scatter").mod (macro2, "osc1UnisonDetune", 20)
            .macro (3, "Motion").mod (macro3, "lfo1Rate", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Sampled Soot", "Texture").set ("osc1On", 0).noise (80, 2, 20).filter (1, ladderBand, 1400, 40).lfo (1, randomShape, 2.9f, 0)
            .mod (lfo1, "filter1Cutoff", 17).mod (lfo1, "ampLevel", 4).amp (1100, 1000, 100, 1100).drive (crush, 20, 3700).reverb (10, 90, 3, 45)
            .level (2).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Colour").mod (macro2, "noiseColour", -40).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 55).macro (4, "Motion").mod (macro4, "lfo1Rate", 25);
        add ("Scarlet Dump", "Texture").set ("osc1On", 0).noise (85, 3).lfoSync (1, steps, quarter, 0)
            .stepPattern (1, { 20, 20, 20, -20, 60, -100, 100, 20, -60, -100, 100, 20, -100, 100, -60, 60 }).mod (lfo1, "noiseColour", 40)
            .lfo (2, randomShape, 0.48f, 0).mod (lfo2, "filter1Cutoff", 5).filter (1, ladderBand, 740, 40).amp (500, 1000, 100, 800)
            .delay (eighthT, 30, 26, 5600, false).reverb (30, 75, 7, 60).level (-10).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20)
            .macro (2, "Rate").mod (macro2, "noiseColour", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (crush, 0, 9000)
            .macro (4, "Grit").mod (macro4, "fxDriveAmount", 55);
        add ("Scarlet Readout", "Texture").set ("osc1On", 0).noise (70, 3).lfoSync (1, steps, quarter, 0)
            .stepPattern (1, { -100, -60, 20, 20, 100, 20, -20, -20, -60, 60, 20, 100, 60, -20, -100, -20 }).mod (lfo1, "noiseColour", 45)
            .lfo (2, randomShape, 0.41f, 0).mod (lfo2, "filter1Cutoff", 11).filter (1, svfHigh, 2000, 20).amp (1500, 1000, 100, 1000)
            .reverb (15, 100, 4.5f, 55).level (-4).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Rate")
            .mod (macro2, "noiseColour", 30).drive (crush, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 35);
        add ("Secret Cinders", "Texture").set ("osc1On", 0).noise (85, 2, -20).filter (1, svfHigh, 5800, 25).lfo (1, drift, 1.2f, 0)
            .mod (lfo1, "filter1Cutoff", 7).mod (lfo1, "ampLevel", 8).amp (200, 1000, 100, 1600).reverb (30, 80, 8.5f, 60).level (-1)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Colour").mod (macro2, "noiseColour", -40).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Motion").mod (macro4, "lfo1Rate", 25);
        add ("Shapeless Colony", "Texture").osc (1, metal, 80).unison (1, 8, 45, 100, 0, 70).pitch (1, 12).filter (1, svfHigh, 1200, 25)
            .lfo (1, randomShape, 2.8f, 0).mod (lfo1, "osc1UnisonDetune", 20).mod (lfo1, "ampLevel", 3).amp (1500, 1000, 100, 1000)
            .drive (tube, 15, 5200).reverb (25, 85, 3.5f, 55).level (-9).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Scatter")
            .mod (macro2, "osc1UnisonDetune", 20).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 35);
        add ("Tender Log", "Texture").set ("osc1On", 0).noise (75, 3).lfoSync (1, steps, half, 0)
            .stepPattern (1, { -20, 60, 100, 60, 20, 100, 20, 60, -60, -60, -20, 100, 100, 20, -20, -100 }).mod (lfo1, "noiseColour", 30)
            .lfo (2, drift, 1.6f, 0).mod (lfo2, "filter1Cutoff", 11).filter (1, svfBand, 780, 50).amp (1100, 1000, 100, 900)
            .delay (sixteenth, 40, 22, 7600).reverb (10, 100, 4.5f, 55).level (-4).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20)
            .macro (2, "Rate").mod (macro2, "noiseColour", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 35).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 35);
        add ("Threadbare Channel", "Texture").osc (1, analog, sinePos, 1).pitch (1, 17, -10).osc (2, analog, sinePos).warp (2, wFm, 25)
            .noise (40, 3).lfo (1, randomShape, 3.3f, 0).mod (lfo1, "osc2WarpAmount", 30).mod (lfo1, "osc2Fine", 40).lfo (2, drift, 0.065f, 0)
            .mod (lfo2, "filter1Cutoff", 9).filter (1, ladderBand, 1600, 40).amp (1500, 1000, 100, 2000).drive (crush, 15, 11000)
            .reverb (30, 90, 3, 65).level (-14).macro (1, "Tune").mod (macro1, "filter1Cutoff", 18).macro (2, "Static")
            .mod (macro2, "noiseLevel", 35).macro (3, "Motion").mod (macro3, "lfo1Rate", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Titanium Infestation", "Texture").osc (1, metal, 90).unison (1, 8, 25, 100, 0, 80).pitch (1, 0).filter (1, svfBand, 2400, 45)
            .lfo (1, randomShape, 0.38f, 0).mod (lfo1, "osc1UnisonDetune", 10).mod (lfo1, "ampLevel", 3).amp (1300, 1000, 100, 2200)
            .drive (tube, 15, 6200).reverb (25, 75, 5.5f, 60).level (-3).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Scatter")
            .mod (macro2, "osc1UnisonDetune", 20).macro (3, "Resonance").mod (macro3, "filter1Resonance", 35).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Trailing Gust", "Texture").set ("osc1On", 0).noise (90, 1, -20).filter (1, svfBand, 1600, 79).lfo (1, drift, 0.15f, 0)
            .mod (lfo1, "filter1Cutoff", 8).lfo (2, drift, 0.14f, 0).mod (lfo2, "ampLevel", 6).mod (lfo2, "filter1Resonance", 8)
            .amp (400, 1000, 100, 2400).reverb (45, 80, 8.5f, 40).level (5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Howl")
            .mod (macro2, "filter1Resonance", 10).drive (tube, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Gusts")
            .mod (macro4, "lfo1Rate", 25);
        add ("Twitching Log", "Texture").set ("osc1On", 0).noise (95, 3).lfoSync (1, steps, quarter, 0, 20)
            .stepPattern (1, { -60, 20, 60, 60, -60, 20, 60, -60, 100, 60, 100, -100, 20, -100, 20, -60 }).mod (lfo1, "noiseColour", 25)
            .lfo (2, drift, 0.74f, 0).mod (lfo2, "filter1Cutoff", 5).filter (1, ladderBand, 4900, 55).amp (1400, 1000, 100, 1500)
            .reverb (10, 95, 9, 40).level (-4).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Rate")
            .mod (macro2, "noiseColour", 30).drive (crush, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 35);
        add ("Vast Gale", "Texture").set ("osc1On", 0).noise (90, 0, -20).filter (1, ladder24, 1100, 67).lfo (1, drift, 0.42f, 0)
            .mod (lfo1, "filter1Cutoff", 16).lfo (2, sine, 0.23f, 0).mod (lfo2, "ampLevel", 14).mod (lfo2, "filter1Resonance", 9)
            .amp (200, 1000, 100, 1400).reverb (40, 90, 7.5f, 65).level (-4).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Howl")
            .mod (macro2, "filter1Resonance", 10).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (tube, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Wan Readout", "Texture").set ("osc1On", 0).noise (95, 3).lfoSync (1, steps, bars2, 0)
            .stepPattern (1, { 100, -20, 100, 20, -100, -100, -100, 20, 60, 20, -100, -20, 100, -100, 60, 60 }).mod (lfo1, "noiseColour", 25)
            .lfo (2, drift, 1.2f, 0).mod (lfo2, "filter1Cutoff", 12).filter (1, svfBand, 1900, 45).amp (1400, 1000, 100, 1300)
            .delay (thirtySecond, 30, 14, 2700, false).reverb (30, 70, 6, 65).level (-5).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20)
            .macro (2, "Rate").mod (macro2, "noiseColour", 30).drive (crush, 0, 9000).macro (3, "Grit").mod (macro3, "fxDriveAmount", 55)
            .macro (4, "Resonance").mod (macro4, "filter1Resonance", 35);
        add ("Warped Wind", "Texture").set ("osc1On", 0).noise (90, 1, -60).filter (1, ladderBand, 680, 61).lfo (1, drift, 0.33f, 0)
            .mod (lfo1, "filter1Cutoff", 8).lfo (2, sine, 0.077f, 0).mod (lfo2, "ampLevel", 16).mod (lfo2, "filter1Resonance", 10)
            .amp (900, 1000, 100, 1000).reverb (45, 70, 3, 60).level (-10).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 20).macro (2, "Howl")
            .mod (macro2, "filter1Resonance", 10).macro (3, "Gusts").mod (macro3, "lfo1Rate", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Wary Brood", "Texture").osc (1, metal, 85).unison (1, 8, 45, 100, 0, 70).pitch (1, 24).filter (1, svfBand, 3700, 40)
            .lfo (1, randomShape, 0.5f, 0).mod (lfo1, "osc1UnisonDetune", 20).mod (lfo1, "ampLevel", 5).amp (700, 1000, 100, 800)
            .reverb (15, 70, 5.5f, 55).level (-3).macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 18).macro (2, "Scatter")
            .mod (macro2, "osc1UnisonDetune", 20).macro (3, "Space").mod (macro3, "fxReverbMix", 35).drive (hard, 0, 9000).macro (4, "Grit")
            .mod (macro4, "fxDriveAmount", 55);
        add ("Waxen Shortwave", "Texture").osc (1, analog, sinePos, 20).pitch (1, 19, 20).osc (2, analog, sinePos).warp (2, wFm, 30).noise (40)
            .lfo (1, drift, 1.5f, 0).mod (lfo1, "osc2WarpAmount", 15).mod (lfo1, "osc2Fine", 20).lfo (2, drift, 0.1f, 0)
            .mod (lfo2, "filter1Cutoff", 9).filter (1, ladderBand, 3300, 45).amp (1400, 1000, 100, 1300).reverb (15, 85, 4, 45).level (-15)
            .macro (1, "Tune").mod (macro1, "filter1Cutoff", 18).macro (2, "Static").mod (macro2, "noiseLevel", 35).macro (3, "Motion")
            .mod (macro3, "lfo1Rate", 25).macro (4, "Space").mod (macro4, "fxReverbMix", 35);
        add ("Zinc Refinery", "Texture").osc (1, pulse, 50, 65).pitch (1, -12).noise (40).filter (1, comb, 190, 85).filter (2, ladder24, 1400, 40)
            .lfoSync (1, steps, bars2, 0, 30).stepPattern (1, { -50, -50, 100, -100, -100, 50, 100, 50, -100, -100, -100, 0, 100, -100, 50, -50 })
            .mod (lfo1, "filter2Cutoff", 10).lfo (2, drift, 0.082f, 0).mod (lfo2, "filter1Cutoff", 6).amp (1000, 1000, 100, 1200)
            .drive (crush, 35, 11000).reverb (25, 80, 7.5f, 35).level (-5).macro (1, "Cutoff").mod (macro1, "filter2Cutoff", 22).macro (2, "Ring")
            .mod (macro2, "filter1Resonance", 25).macro (3, "Air").mod (macro3, "noiseLevel", -40).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 35);

        // Vox, by name.
        add ("Boundless Chorus", "Vox").osc (1, analog, sawPos).unison (1, 8, 16, 100, 0, 20).route (1, 50).parallel().filter (1, svfBand, 450, 60)
            .filter (2, svfBand, 800, 75).lfo (1, sine, 5.75f).lfoFade (1, 400).mod (lfo1, "osc1Fine", 3).lfo (2, drift, 0.2f, 0)
            .mod (lfo2, "filter2Cutoff", 4).amp (900, 1000, 85, 1000).reverb (50, 75, 8, 55).level (-7).macro (1, "Vowel")
            .mod (macro1, "filter2Cutoff", -12).macro (2, "Mouth").mod (macro2, "filter1Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Cyan Throng", "Vox").osc (1, analog, sawPos).unison (1, 8, 12, 100, 0, 20).route (1, 50).parallel().filter (1, svfBand, 570, 65)
            .filter (2, svfBand, 840, 75).lfo (1, sine, 5).lfoFade (1, 500).mod (lfo1, "osc1Fine", 6).lfo (2, drift, 0.072f, 0)
            .mod (lfo2, "filter2Cutoff", 3).amp (250, 1000, 85, 600).reverb (50, 100, 5, 40).level (-6).macro (1, "Vowel")
            .mod (macro1, "filter2Cutoff", -12).macro (2, "Mouth").mod (macro2, "filter1Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Doomed Golem", "Vox").osc (1, analog, squarePos).route (1, 50).parallel().filter (1, svfBand, 270, 85).filter (2, svfBand, 2300, 75)
            .lfoSync (1, steps, half, 1, 84).stepPattern (1, { -100, 100, 0, -100, -100, -100, 50, 0, 100, 50, -100, -50, 100, -100, 100, -100 })
            .mod (lfo1, "filter2Cutoff", 10).mod (lfo1, "filter1Cutoff", -8).amp (5, 300, 95, 240).mono (30).drive (hard, 30, 8300).level (-9)
            .macro (1, "Vowel").mod (macro1, "filter2Cutoff", -12).macro (2, "Mouth").mod (macro2, "filter1Cutoff", 12).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Dreamless Retch", "Vox").osc (1, growl, 60).unison (1, 3, 8, 70).route (1, 50).parallel().filter (1, svfBand, 570, 65)
            .filter (2, svfBand, 840, 80).env (2, 80, 600, 40, 200, 60).mod (env2, "filter2Cutoff", 10).mod (env2, "osc1Position", 15)
            .amp (3, 400, 90, 400).mono (120).drive (hard, 50, 4100).reverb (15, 95, 4, 45).level (-15).macro (1, "Vowel")
            .mod (macro1, "filter2Cutoff", -12).macro (2, "Growl").mod (macro2, "osc1Position", 30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 35);
        add ("Exposed Verse", "Vox").osc (1, vowel, 50).unison (1, 6, 12, 100, 0, 30).lfoSync (1, steps, bars2, 1, 91)
            .stepPattern (1, { 20, -20, -20, -60, -100, -60, 60, -60, 20, -20, -20, -60, -100, -60, 60, -60 }).mod (lfo1, "osc1Position", 35)
            .filter (1, ladder12, 3500, 30).amp (40, 600, 90, 1100).reverb (40, 100, 8.5f, 45).level (-9).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Fevered Automaton", "Vox").osc (1, pulse, 40).route (1, 50).parallel().filter (1, svfBand, 300, 80).filter (2, svfBand, 2250, 75)
            .lfoSync (1, steps, bars2, 1, 88).stepPattern (1, { 0, -50, -50, 100, 50, -50, 50, -100, 100, 50, 50, 100, 50, 50, -100, 100 })
            .mod (lfo1, "filter2Cutoff", 10).mod (lfo1, "filter1Cutoff", -6).amp (5, 300, 95, 220).mono (30).drive (crush, 25, 5700).level (3)
            .macro (1, "Vowel").mod (macro1, "filter2Cutoff", -12).macro (2, "Mouth").mod (macro2, "filter1Cutoff", 12).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Flooded Rumour", "Vox").set ("osc1On", 0).noise (100).route (1, 50).parallel().filter (1, svfBand, 325, 55)
            .filter (2, svfBand, 700, 65).lfo (1, drift, 2.4f, 0).mod (lfo1, "filter2Cutoff", 7).mod (lfo1, "filter1Cutoff", -4)
            .lfo (2, drift, 2.3f, 0).mod (lfo2, "ampLevel", 6).amp (560, 800, 90, 1500).reverb (35, 80, 8, 40).level (4).macro (1, "Vowel")
            .mod (macro1, "filter2Cutoff", -12).macro (2, "Mouth").mod (macro2, "filter1Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Speech").mod (macro4, "lfo1Rate", 25);
        add ("Formless Snarl", "Vox").osc (1, growl, 65).unison (1, 3, 14, 70).route (1, 50).parallel().filter (1, svfBand, 270, 80)
            .filter (2, svfBand, 2300, 80).env (2, 0, 500, 20, 200, 60).mod (env2, "filter2Cutoff", 10).mod (env2, "osc1Position", 20)
            .amp (3, 400, 90, 200).mono (100).drive (hard, 35, 4600).reverb (30, 85, 9, 60).level (-3).macro (1, "Vowel")
            .mod (macro1, "filter2Cutoff", -12).macro (2, "Growl").mod (macro2, "osc1Position", 30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 35);
        add ("Furious Automaton", "Vox").osc (1, analog, squarePos).route (1, 50).parallel().filter (1, svfBand, 325, 75)
            .filter (2, svfBand, 700, 85).lfoSync (1, steps, half, 1, 76)
            .stepPattern (1, { -100, 100, -100, -100, 100, 100, -100, 0, 50, 50, -50, -50, 0, -100, 0, 50 }).mod (lfo1, "filter2Cutoff", 14)
            .mod (lfo1, "filter1Cutoff", -6).amp (5, 300, 95, 180).mono().drive (hard, 30, 7400).delay (eighthT, 45, 18, 3100).level (-7)
            .macro (1, "Vowel").mod (macro1, "filter2Cutoff", -12).macro (2, "Mouth").mod (macro2, "filter1Cutoff", 12).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Gentle Mantra", "Vox").osc (1, vowel, 50).unison (1, 4, 16, 100, 0, 10).osc (2, vowel, 40, 55).pitch (2, 12, 4)
            .lfoSync (1, steps, bars4, 1, 87).stepPattern (1, { 100, 60, -60, -60, 60, 20, 20, 60, 100, 60, -60, -60, 60, 20, 20, 60 })
            .mod (lfo1, "osc1Position", 45).filter (1, ladder12, 3700, 25).amp (250, 600, 100, 900).reverb (30, 70, 5, 45).level (-11)
            .macro (1, "Vowel").mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Latched Automaton", "Vox").osc (1, analog, sawPos).route (1, 50).parallel().filter (1, svfBand, 500, 60).filter (2, svfBand, 1500, 60)
            .lfoSync (1, steps, half, 1, 76).stepPattern (1, { 100, 100, 100, 50, -100, -50, 50, -100, 100, -50, 100, -100, 100, -50, 0, 100 })
            .mod (lfo1, "filter2Cutoff", 14).mod (lfo1, "filter1Cutoff", -4).amp (5, 300, 95, 200).mono().drive (crush, 35, 5700)
            .delay (quarterT, 25, 16, 7100).level (-1).macro (1, "Vowel").mod (macro1, "filter2Cutoff", -12).macro (2, "Mouth")
            .mod (macro2, "filter1Cutoff", 12).macro (3, "Grit").mod (macro3, "fxDriveAmount", 35).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Marching Liturgy", "Vox").osc (1, vowel, 50).unison (1, 2, 10, 100, 0, 10).lfoSync (1, steps, bar1, 1, 85)
            .stepPattern (1, { 100, 20, -100, 60, 60, -100, 20, -60, 100, 20, -100, 60, 60, -100, 20, -60 }).mod (lfo1, "osc1Position", 40)
            .filter (1, ladder12, 1900, 30).amp (120, 600, 90, 800).reverb (30, 85, 4.5f, 65).level (-13).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Misted Refrain", "Vox").osc (1, vowel, 50).unison (1, 4, 8, 100).osc (2, vowel, 40, 55).pitch (2, -12, 8)
            .lfoSync (1, steps, bar1, 1, 93).stepPattern (1, { -60, 60, 60, 60, 100, -20, 100, 100, -60, 60, 60, 60, 100, -20, 100, 100 })
            .mod (lfo1, "osc1Position", 45).filter (1, ladder12, 3700).amp (80, 600, 100, 1400).reverb (35, 85, 7.5f, 30).level (-12)
            .macro (1, "Vowel").mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Overnight Snarl", "Vox").osc (1, growl, 45).unison (1, 2, 10, 40).route (1, 50).parallel().filter (1, svfBand, 450, 75)
            .filter (2, svfBand, 800, 75).env (2, 30, 200, 30, 200, 60).mod (env2, "filter2Cutoff", 6).mod (env2, "osc1Position", 30)
            .amp (10, 400, 90, 300).mono (70).drive (hard, 25, 5800).reverb (10, 80, 4, 70).level (-7).macro (1, "Vowel")
            .mod (macro1, "filter2Cutoff", -12).macro (2, "Growl").mod (macro2, "osc1Position", 30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 35);
        add ("Patient Secret", "Vox").set ("osc1On", 0).noise (100, 0, -20).route (1, 50).parallel().filter (1, svfBand, 300, 65)
            .filter (2, svfBand, 2250, 40).lfo (1, randomShape, 3, 0).mod (lfo1, "filter2Cutoff", 6).mod (lfo1, "filter1Cutoff", -4)
            .lfo (2, drift, 1.1f, 0).mod (lfo2, "ampLevel", 13).amp (460, 800, 100, 800).reverb (45, 95, 7.5f, 30).level (-1).macro (1, "Vowel")
            .mod (macro1, "filter2Cutoff", -12).macro (2, "Mouth").mod (macro2, "filter1Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Speech").mod (macro4, "lfo1Rate", 25);
        add ("Placid Chorus", "Vox").osc (1, analog, 80).unison (1, 5, 22, 100, 0, 30).route (1, 50).parallel().filter (1, svfBand, 400, 80)
            .filter (2, svfBand, 1600, 70).lfo (1, sine, 5.75f).lfoFade (1, 900).mod (lfo1, "osc1Fine", 8).lfo (2, drift, 0.076f, 0)
            .mod (lfo2, "filter2Cutoff", 4).amp (900, 1000, 100, 1600).reverb (45, 80, 3.5f, 30).level (-5).macro (1, "Vowel")
            .mod (macro1, "filter2Cutoff", -12).macro (2, "Mouth").mod (macro2, "filter1Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Polar Crowd", "Vox").osc (1, analog, sawPos).unison (1, 8, 20, 100, 0, 20).route (1, 50).parallel().filter (1, svfBand, 300, 80)
            .filter (2, svfBand, 2250, 80).lfo (1, sine, 5).lfoFade (1, 400).mod (lfo1, "osc1Fine", 7).lfo (2, drift, 0.083f, 0)
            .mod (lfo2, "filter2Cutoff", 4).amp (750, 1000, 85, 1600).reverb (35, 90, 8, 35).level (-2).macro (1, "Vowel")
            .mod (macro1, "filter2Cutoff", -12).macro (2, "Mouth").mod (macro2, "filter1Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Racing Golem", "Vox").osc (1, analog, squarePos).route (1, 50).parallel().filter (1, svfBand, 570, 60).filter (2, svfBand, 840, 75)
            .lfoSync (1, steps, bar1, 1, 84).stepPattern (1, { -50, 100, 50, 100, -50, -50, 100, 100, 100, -100, 0, 50, -100, 50, -50, 0 })
            .mod (lfo1, "filter2Cutoff", 6).mod (lfo1, "filter1Cutoff", -3).amp (5, 300, 95, 220).mono (80).drive (crush, 35, 6400).level (-1)
            .macro (1, "Vowel").mod (macro1, "filter2Cutoff", -12).macro (2, "Mouth").mod (macro2, "filter1Cutoff", 12).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Restless Unit", "Vox").osc (1, analog, sawPos).route (1, 50).parallel().filter (1, svfBand, 325, 65).filter (2, svfBand, 700, 85)
            .lfoSync (1, steps, bar1, 1, 80).stepPattern (1, { 50, 0, -50, -100, -100, 50, -100, 100, 50, 0, -100, -50, -50, -100, -100, 0 })
            .mod (lfo1, "filter2Cutoff", 6).mod (lfo1, "filter1Cutoff", -5).amp (5, 300, 95, 100).mono (30).drive (hard, 50, 7100).level (-12)
            .macro (1, "Vowel").mod (macro1, "filter2Cutoff", -12).macro (2, "Mouth").mod (macro2, "filter1Cutoff", 12).macro (3, "Grit")
            .mod (macro3, "fxDriveAmount", 35).delay (eighthD, 40, 0, 4500).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
        add ("Savage Croak", "Vox").osc (1, growl, 20).unison (1, 3, 8, 30).route (1, 50).parallel().filter (1, svfBand, 800, 80)
            .filter (2, svfBand, 1150, 65).env (2, 30, 650, 40, 200, 60).mod (env2, "filter2Cutoff", 8).mod (env2, "osc1Position", 15)
            .amp (30, 400, 90, 300).mono (70).drive (tube, 40, 6200).reverb (25, 70, 5, 70).level (-9).macro (1, "Vowel")
            .mod (macro1, "filter2Cutoff", -12).macro (2, "Growl").mod (macro2, "osc1Position", 30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 35);
        add ("Scarred Verse", "Vox").osc (1, vowel, 50).unison (1, 6, 10, 100, 0, 20).osc (2, vowel, 30, 65).pitch (2, 7, 8)
            .lfoSync (1, steps, bar1, 1, 93).stepPattern (1, { -60, 100, 100, 60, -20, -20, -20, -60, -60, 100, 100, 60, -20, -20, -20, -60 })
            .mod (lfo1, "osc1Position", 50).filter (1, svfLow, 4700, 30).amp (30, 600, 95, 1400).reverb (40, 90, 8.5f, 55).level (-13)
            .macro (1, "Vowel").mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Seared Snarl", "Vox").osc (1, growl, 65).unison (1, 1, 6, 60).route (1, 50).parallel().filter (1, svfBand, 300, 80)
            .filter (2, svfBand, 2250, 55).env (2, 30, 400, 40, 200, 60).mod (env2, "filter2Cutoff", 12).mod (env2, "osc1Position", 25)
            .amp (30, 400, 90, 350).mono (80).drive (hard, 30, 5200).reverb (20, 85, 4.5f, 45).level (-1).macro (1, "Vowel")
            .mod (macro1, "filter2Cutoff", -12).macro (2, "Growl").mod (macro2, "osc1Position", 30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 35);
        add ("Sector Verse", "Vox").osc (1, vowel, 50).unison (1, 2, 16, 100).lfoSync (1, steps, bars2, 1, 91)
            .stepPattern (1, { 100, -20, 20, 20, -60, 20, -60, -20, 100, -20, 20, 20, -60, 20, -60, -20 }).mod (lfo1, "osc1Position", 45)
            .filter (1, svfLow, 3300, 25).amp (190, 600, 95, 800).reverb (35, 80, 7, 35).level (-12).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 30).macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 22).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Sleepless Hiss", "Vox").set ("osc1On", 0).noise (100, 1, 40).route (1, 50).parallel().filter (1, svfBand, 300, 55)
            .filter (2, svfBand, 2250, 45).lfo (1, randomShape, 3.1f, 0).mod (lfo1, "filter2Cutoff", 8).mod (lfo1, "filter1Cutoff", -2)
            .lfo (2, drift, 2.2f, 0).mod (lfo2, "ampLevel", 9).amp (480, 800, 90, 800).reverb (40, 100, 6.5f, 65).level (2).macro (1, "Vowel")
            .mod (macro1, "filter2Cutoff", -12).macro (2, "Mouth").mod (macro2, "filter1Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Speech").mod (macro4, "lfo1Rate", 25);
        add ("Violent Gurgle", "Vox").osc (1, growl, 50).unison (1, 2, 6, 30).route (1, 50).parallel().filter (1, svfBand, 300, 60)
            .filter (2, svfBand, 2250, 60).env (2, 0, 350, 50, 200, 60).mod (env2, "filter2Cutoff", 12).mod (env2, "osc1Position", 25)
            .amp (3, 400, 90, 200).mono (90).drive (tube, 50, 5800).reverb (30, 95, 3, 55).level (-7).macro (1, "Vowel")
            .mod (macro1, "filter2Cutoff", -12).macro (2, "Growl").mod (macro2, "osc1Position", 30).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 35);
        add ("Voiceless Mass", "Vox").osc (1, analog, sawPos).unison (1, 8, 20, 100, 0, 30).route (1, 50).parallel().filter (1, svfBand, 800, 65)
            .filter (2, svfBand, 1150, 65).lfo (1, sine, 6).lfoFade (1, 600).mod (lfo1, "osc1Fine", 8).lfo (2, drift, 0.062f, 0)
            .mod (lfo2, "filter2Cutoff", 4).amp (550, 1000, 85, 2000).reverb (35, 90, 3.5f, 45).level (-6).macro (1, "Vowel")
            .mod (macro1, "filter2Cutoff", -12).macro (2, "Mouth").mod (macro2, "filter1Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
        add ("Warm Hiss", "Vox").set ("osc1On", 0).noise (100, 0, 20).route (1, 50).parallel().filter (1, svfBand, 400, 50)
            .filter (2, svfBand, 1600, 65).lfo (1, randomShape, 0.5f, 0).mod (lfo1, "filter2Cutoff", 7).mod (lfo1, "filter1Cutoff", -5)
            .lfo (2, drift, 0.79f, 0).mod (lfo2, "ampLevel", 6).amp (220, 800, 80, 600).reverb (40, 75, 6, 35).level (4).macro (1, "Vowel")
            .mod (macro1, "filter2Cutoff", -12).macro (2, "Mouth").mod (macro2, "filter1Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Speech").mod (macro4, "lfo1Rate", 25);
        add ("Waxen Rumour", "Vox").set ("osc1On", 0).noise (100, 1).route (1, 50).parallel().filter (1, svfBand, 800, 55)
            .filter (2, svfBand, 1150, 60).lfo (1, randomShape, 0.55f, 0).mod (lfo1, "filter2Cutoff", 9).mod (lfo1, "filter1Cutoff", -3)
            .lfo (2, drift, 0.57f, 0).mod (lfo2, "ampLevel", 6).amp (360, 800, 90, 1200).reverb (30, 90, 9, 70).level (1).macro (1, "Vowel")
            .mod (macro1, "filter2Cutoff", -12).macro (2, "Mouth").mod (macro2, "filter1Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Speech").mod (macro4, "lfo1Rate", 25);
        add ("Woollen Assembly", "Vox").osc (1, analog, sawPos).unison (1, 4, 14, 100, 0, 30).route (1, 50).parallel().filter (1, svfBand, 270, 80)
            .filter (2, svfBand, 2300, 60).lfo (1, sine, 5.25f).lfoFade (1, 500).mod (lfo1, "osc1Fine", 3).lfo (2, drift, 0.34f, 0)
            .mod (lfo2, "filter2Cutoff", 4).amp (350, 1000, 90, 1800).reverb (30, 95, 7.5f, 35).level (-4).macro (1, "Vowel")
            .mod (macro1, "filter2Cutoff", -12).macro (2, "Mouth").mod (macro2, "filter1Cutoff", 12).macro (3, "Space")
            .mod (macro3, "fxReverbMix", 35).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 25);
    }

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
