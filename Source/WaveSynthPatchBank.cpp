#include "WaveSynthPatchBuilder.h"

// Basic Synth's patch bank: 970 more patches after the hand-made ones, in the same categories and
// two more (Perc and Vox). Each kind of sound (an acid line, a reese, a strummed guitar) was set up
// once and its settings drawn in the ranges that suit it, then every patch's level was set so a
// held chord peaks near -9 dB.

namespace WaveSynthPatchBuilder
{
    void addBank (std::vector<Builder>& b)
    {
        const auto add = [&b] (const char* name, const char* category) -> Builder& { b.emplace_back (name, category); return b.back(); };

        // Bass, by name.
        add ("Abandoned Piston", "Bass").osc (1, fm, 35).subOsc (40).env (2, 0, 120, 15, 120, 70).mod (env2, "osc1Position", 25)
            .filter (1, ladder24, 1700, 15).amp (1, 340, 70, 80).mono().mod (velocity, "osc1Position", 16).drive (tube, 20, 6000).level (-8)
            .standardMacros (25).macro (4, "Index").mod (macro4, "osc1Position", 50);
        add ("Abandoned Pump", "Bass").osc (1, sub, sinePos).subOsc (40, 1, 1).filter (1, ladder24, 750, 20).amp (3, 450, 95, 100).mono().level (-4)
            .macro (1, "Grit").mod (macro1, "filter1Drive", 60).macro (2, "Body").mod (macro2, "subLevel", 50).macro (3, "Tone")
            .mod (macro3, "filter1Cutoff", 30);
        add ("Acid Press", "Bass").osc (1, analog, squarePos).osc (2, analog, squarePos, 65).pitch (2, 12, -6).subOsc (40)
            .filter (1, ladder24, 310, 45, 40).env (2, 0, 100, 10, 160, 70).mod (env2, "filter1Cutoff", 30).amp (2, 320, 80, 80).mono (0, false)
            .drive (tube, 40, 8000).mod (velocity, "filter1Cutoff", 7).level (-13).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Ash Crank", "Bass").osc (1, analog, squarePos).osc (2, analog, sawPos, 45).pitch (2, 0, 7).subOsc (50).filter (1, ladder24, 230, 15, 45)
            .env (2, 0, 150, 5, 120, 65).mod (env2, "filter1Cutoff", 30).amp (2, 230, 80, 60).mono().drive (tube, 25, 4000)
            .mod (velocity, "filter1Cutoff", 13).level (-10).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Basalt Drive", "Bass").osc (1, analog, 20).subOsc (30, 1, 0).filter (1, ladder24, 500, 15).drive (tube, 35, 3000).amp (3, 800, 95, 150)
            .mono().level (-10).macro (1, "Grit").mod (macro1, "fxDriveAmount", 60).macro (2, "Body").mod (macro2, "subLevel", 50).macro (3, "Tone")
            .mod (macro3, "filter1Cutoff", 30);
        add ("Basalt Pulse", "Bass").osc (1, sub, sinePos).subOsc (30).filter (1, ladder24, 850, 5).amp (1, 650, 90, 110).mono().level (-3)
            .macro (1, "Grit").mod (macro1, "filter1Drive", 60).macro (2, "Body").mod (macro2, "subLevel", 50).macro (3, "Tone")
            .mod (macro3, "filter1Cutoff", 30);
        add ("Basement Pulse", "Bass").osc (1, analog, sawPos).unison (1, 2, 11, 45).filter (1, ladder24, 330, 55, 25).env (2, 0, 90, 0, 100, 75)
            .mod (env2, "filter1Cutoff", 30).amp (1, 220, 0, 60).mono (0, false).delay (eighthD, 30, 15, 2500).mod (velocity, "filter1Cutoff", 10)
            .level (-4).standardMacros (25);
        add ("Basement Pump", "Bass").osc (1, sub, 0).subOsc (45).filter (1, ladder24, 350, 15).drive (tube, 20, 3250).amp (3, 550, 85, 130).mono()
            .env (2, 0, 75, 0, 60, 90).mod (env2, "osc1Pitch", 21).level (-8).macro (1, "Grit").mod (macro1, "fxDriveAmount", 60).macro (2, "Body")
            .mod (macro2, "subLevel", 50).macro (3, "Tone").mod (macro3, "filter1Cutoff", 30);
        add ("Black Dredge", "Bass").osc (1, analog, squarePos).osc (2, analog, squarePos, 65).pitch (2, 12, 5).subOsc (45)
            .filter (1, ladder24, 430, 10, 30).env (2, 0, 100, 15, 150, 65).mod (env2, "filter1Cutoff", 25).amp (2, 370, 80, 50)
            .drive (soft, 30, 5500).mod (velocity, "filter1Cutoff", 15).level (-18).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Black Rumble", "Bass").osc (1, analog, sawPos).unison (1, 2, 8, 25).filter (1, ladder24, 240, 50, 20).env (2, 0, 140, 0, 100, 75)
            .mod (env2, "filter1Cutoff", 35).amp (1, 140, 0, 60).mono (0, false).mod (velocity, "filter1Cutoff", 16).level (-3).standardMacros (25);
        add ("Bleak Girder", "Bass").osc (1, fm, 45).subOsc (45).env (2, 0, 140, 15, 120, 70).mod (env2, "osc1Position", 40)
            .filter (1, svfLow, 2300, 15).amp (1, 210, 80, 80).mono().mod (velocity, "osc1Position", 15).level (-5).standardMacros (25)
            .macro (4, "Index").mod (macro4, "osc1Position", 50);
        add ("Bleak Mass", "Bass").osc (1, analog, sawPos).unison (1, 3, 7, 40).filter (1, ladder24, 260, 40, 35).env (2, 0, 170, 0, 100, 75)
            .mod (env2, "filter1Cutoff", 35).amp (1, 160, 0, 60).mono (0, false).delay (sixteenthD, 20, 15, 2500).mod (velocity, "filter1Cutoff", 12)
            .level (-2).standardMacros (25);
        add ("Bleak Press", "Bass").osc (1, analog, squarePos).osc (2, analog, squarePos, 45).pitch (2, 0).subOsc (40)
            .filter (1, ladder24, 420, 35, 40).env (2, 0, 120, 15, 140, 80).mod (env2, "filter1Cutoff", 30).amp (3, 300, 0, 90)
            .mod (velocity, "filter1Cutoff", 15).level (-10).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Bleak Thud", "Bass").osc (1, analog, squarePos).osc (2, analog, squarePos, 60).pitch (2, 0, 5).subOsc (30, 0, 1)
            .filter (1, ladder12, 330, 35, 50).env (2, 0, 290, 15, 150, 75).mod (env2, "filter1Cutoff", 35).amp (2, 350, 80, 50).mono (40)
            .drive (tube, 20, 6500).mod (velocity, "filter1Cutoff", 6).level (-8).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Bunker Drive", "Bass").osc (1, analog, squarePos).subOsc (35).filter (1, ladder24, 360, 30, 5).env (2, 0, 160, 5, 170, 65)
            .mod (env2, "filter1Cutoff", 35).amp (2, 250, 80, 100).mono (20).drive (tube, 35, 5000).mod (velocity, "filter1Cutoff", 13).level (-11)
            .standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Bunker Mass", "Bass").osc (1, analog, squarePos).osc (2, analog, sawPos, 65).pitch (2, 12).subOsc (40).filter (1, ladder12, 270, 45, 25)
            .env (2, 0, 290, 20, 140, 65).mod (env2, "filter1Cutoff", 25).amp (2, 360, 0, 100).mono (40, false).drive (soft, 35, 4500)
            .mod (velocity, "filter1Cutoff", 10).level (-16).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Burnt Dredge", "Bass").osc (1, pulse, 55).warp (1, wSqueeze, 30).subOsc (45).filter (1, ladder24, 700, 25, 55)
            .lfoSync (1, triangle, quarter, 1).mod (lfo1, "osc1Position", 35).mod (lfo1, "filter1Cutoff", 25).amp (1, 300, 85, 80).mono()
            .drive (fuzz, 45, 6000).level (-22).standardMacros (25).macro (4, "Warp").mod (macro4, "osc1WarpAmount", 50);
        add ("Burnt Grind", "Bass").osc (1, analog, squarePos).subOsc (45).filter (1, ladder12, 340, 50, 50).env (2, 0, 180, 5, 130, 80)
            .mod (env2, "filter1Cutoff", 25).amp (2, 350, 80, 60).mono (40).mod (velocity, "filter1Cutoff", 5).level (-6).standardMacros (25)
            .macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Carbon Bass", "Bass").osc (1, analog, squarePos).osc (2, analog, squarePos, 50).pitch (2, 0).subOsc (40)
            .filter (1, ladder24, 360, 50, 30).env (2, 0, 230, 15, 120, 80).mod (env2, "filter1Cutoff", 20).amp (1, 200, 80, 80).mono (40, false)
            .drive (soft, 35, 8000).mod (velocity, "filter1Cutoff", 9).level (-15).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Cellar Anvil", "Bass").osc (1, analog, sawPos).unison (1, 3, 6, 35).filter (1, ladder24, 370, 55, 30).env (2, 0, 100, 0, 100, 75)
            .mod (env2, "filter1Cutoff", 35).amp (1, 150, 0, 60).mono (0, false).mod (velocity, "filter1Cutoff", 17).level (-2).standardMacros (25);
        add ("Cellar Ballast", "Bass").osc (1, analog, 0).subOsc (45, 0, 1).filter (1, ladder24, 850, 10).amp (2, 800, 95, 130).mono()
            .env (2, 0, 45, 0, 60, 90).mod (env2, "osc1Pitch", 14).level (-5).macro (1, "Grit").mod (macro1, "filter1Drive", 60).macro (2, "Body")
            .mod (macro2, "subLevel", 50).macro (3, "Tone").mod (macro3, "filter1Cutoff", 30);
        add ("Cellar Dredge", "Bass").osc (1, growl, 60).warp (1, wBend, 15).subOsc (40).filter (1, ladder24, 1150, 30, 45)
            .env (2, 0, 250, 20, 150, 60).mod (env2, "filter1Cutoff", 30).amp (1, 300, 85, 80).mono().drive (foldDrive, 50, 5500).level (-18)
            .standardMacros (25).macro (4, "Warp").mod (macro4, "osc1WarpAmount", 50);
        add ("Cellar Drive", "Bass").osc (1, analog, 85).osc (2, analog, sawPos, 65).pitch (2, -12).subOsc (30).filter (1, ladder24, 580, 20, 10)
            .env (2, 0, 180, 15, 190, 70).mod (env2, "filter1Cutoff", 25).amp (2, 340, 90, 120).mod (velocity, "filter1Cutoff", 9).level (-11)
            .standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Cellar Engine", "Bass").osc (1, analog, sawPos).filter (1, ladder24, 190, 50, 35).env (2, 0, 130, 0, 100, 75)
            .mod (env2, "filter1Cutoff", 25).amp (1, 160, 0, 60).mono (0, false).mod (velocity, "filter1Cutoff", 10).level (-3).standardMacros (25);
        add ("Cellar Slab", "Bass").osc (1, analog, squarePos).subOsc (30).filter (1, ladder24, 540, 40, 40).env (2, 0, 290, 20, 120, 65)
            .mod (env2, "filter1Cutoff", 25).amp (2, 230, 0, 100).drive (tube, 15, 5000).mod (velocity, "filter1Cutoff", 6).level (-12)
            .standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Cellar Turbine", "Bass").osc (1, fm, 50).subOsc (45).env (2, 0, 250, 10, 120, 70).mod (env2, "osc1Position", 40)
            .filter (1, ladder24, 1100, 15).amp (1, 350, 80, 80).mono().mod (velocity, "osc1Position", 11).level (-3).standardMacros (25)
            .macro (4, "Index").mod (macro4, "osc1Position", 50);
        add ("Choked Bass", "Bass").osc (1, analog, sinePos).subOsc (25).filter (1, ladder24, 500, 5).amp (4, 550, 90, 100).mono().level (-2)
            .macro (1, "Grit").mod (macro1, "filter1Drive", 60).macro (2, "Body").mod (macro2, "subLevel", 50).macro (3, "Tone")
            .mod (macro3, "filter1Cutoff", 30);
        add ("Chrome Ballast", "Bass").osc (1, vowel, 65).warp (1, wBend, 25).subOsc (40).filter (1, ladder24, 750, 25, 30)
            .lfoSync (1, sawDown, eighth, 1).mod (lfo1, "osc1Position", 20).mod (lfo1, "filter1Cutoff", 15).amp (1, 300, 85, 80).mono()
            .drive (fuzz, 35, 4000).level (-19).standardMacros (25).macro (4, "Warp").mod (macro4, "osc1WarpAmount", 50);
        add ("Chrome Groan", "Bass").osc (1, fm, 20).subOsc (45).env (2, 0, 110, 10, 120, 70).mod (env2, "osc1Position", 20)
            .filter (1, svfLow, 1000, 10).amp (1, 380, 65, 80).mono (30).mod (velocity, "osc1Position", 13).level (-3).standardMacros (25)
            .macro (4, "Index").mod (macro4, "osc1Position", 50);
        add ("Cinder Thud", "Bass").osc (1, fm, 60).subOsc (45).env (2, 0, 150, 15, 120, 70).mod (env2, "osc1Position", 35)
            .filter (1, ladder24, 2000, 5).amp (1, 390, 85, 80).mono().mod (velocity, "osc1Position", 14).drive (hard, 25, 6000).level (-11)
            .standardMacros (25).macro (4, "Index").mod (macro4, "osc1Position", 50);
        add ("Cobalt Piston", "Bass").osc (1, growl, 35).warp (1, wSqueeze, 25).subOsc (35).filter (1, ladder24, 450, 20, 25)
            .lfoSync (1, sine, eighthT, 1).mod (lfo1, "osc1Position", 25).mod (lfo1, "filter1Cutoff", 25).amp (1, 300, 85, 80).mono()
            .drive (hard, 45, 4000).level (-15).standardMacros (25).macro (4, "Warp").mod (macro4, "osc1WarpAmount", 50);
        add ("Concrete Ballast", "Bass").osc (1, analog, sawPos).osc (2, analog, sawPos, 60).pitch (2, -12).subOsc (50, 0, 1)
            .filter (1, ladder24, 510, 35, 35).env (2, 0, 90, 5, 110, 60).mod (env2, "filter1Cutoff", 20).amp (3, 240, 80, 90).mono (40)
            .mod (velocity, "filter1Cutoff", 6).level (-4).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Corroded Mass", "Bass").osc (1, analog, 20).subOsc (45, 0, 1).filter (1, ladder24, 650, 0).amp (4, 750, 90, 120).mono().level (-3)
            .macro (1, "Grit").mod (macro1, "filter1Drive", 60).macro (2, "Body").mod (macro2, "subLevel", 50).macro (3, "Tone")
            .mod (macro3, "filter1Cutoff", 30);
        add ("Corroded Pulse", "Bass").osc (1, sync, 20).warp (1, wBend, 45).subOsc (40).filter (1, ladder24, 1150, 35, 45)
            .env (2, 0, 250, 20, 150, 60).mod (env2, "filter1Cutoff", 25).amp (1, 300, 85, 80).mono().drive (hard, 35, 6000).level (-13)
            .standardMacros (25).macro (4, "Warp").mod (macro4, "osc1WarpAmount", 50);
        add ("Dead Drive", "Bass").osc (1, analog, sawPos).unison (1, 2, 7, 25).filter (1, ladder24, 320, 55, 20).env (2, 0, 180, 0, 100, 75)
            .mod (env2, "filter1Cutoff", 30).amp (1, 160, 0, 60).mono (0, false).delay (sixteenthD, 20, 17, 2500).mod (velocity, "filter1Cutoff", 11)
            .level (-3).standardMacros (25);
        add ("Dead Floor", "Bass").osc (1, analog, 85).osc (2, analog, sawPos, 60).pitch (2, 12, 5).subOsc (40, 0, 1)
            .filter (1, ladder12, 480, 15, 15).env (2, 0, 260, 15, 80, 80).mod (env2, "filter1Cutoff", 35).amp (3, 380, 80, 70).mono()
            .drive (soft, 25, 5500).mod (velocity, "filter1Cutoff", 14).level (-10).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Dead Throb", "Bass").osc (1, sub, 0).subOsc (30).filter (1, ladder24, 900, 5).amp (3, 800, 85, 100).mono().level (-3).macro (1, "Grit")
            .mod (macro1, "filter1Drive", 60).macro (2, "Body").mod (macro2, "subLevel", 50).macro (3, "Tone").mod (macro3, "filter1Cutoff", 30);
        add ("Deep Bass", "Bass").osc (1, analog, sawPos).osc (2, analog, sawPos, 60).pitch (2, 0, -6).subOsc (30).filter (1, ladder24, 520, 10, 45)
            .env (2, 0, 240, 5, 140, 75).mod (env2, "filter1Cutoff", 35).amp (2, 270, 0, 110).mono (0, false).drive (tube, 30, 7500)
            .mod (velocity, "filter1Cutoff", 13).level (-8).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Deep Piston", "Bass").osc (1, analog, 85).osc (2, analog, sawPos, 50).pitch (2, 12, -6).subOsc (25).filter (1, ladder24, 350, 25, 45)
            .env (2, 0, 190, 5, 120, 80).mod (env2, "filter1Cutoff", 40).amp (3, 260, 90, 100).drive (hard, 25, 7500)
            .mod (velocity, "filter1Cutoff", 8).level (-19).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Derelict Engine", "Bass").osc (1, analog, sinePos).subOsc (45, 1, 0).filter (1, ladder24, 450, 20).drive (tube, 30, 2000)
            .amp (4, 700, 90, 120).mono().env (2, 0, 70, 0, 60, 90).mod (env2, "osc1Pitch", 22).level (-11).macro (1, "Grit")
            .mod (macro1, "fxDriveAmount", 60).macro (2, "Body").mod (macro2, "subLevel", 50).macro (3, "Tone").mod (macro3, "filter1Cutoff", 30);
        add ("Derelict Floor", "Bass").osc (1, analog, squarePos).osc (2, analog, squarePos, 60).pitch (2, -12, 5).subOsc (30, 0, 1)
            .filter (1, ladder24, 470, 40, 20).env (2, 0, 220, 5, 90, 70).mod (env2, "filter1Cutoff", 30).amp (2, 200, 80, 70)
            .mod (velocity, "filter1Cutoff", 12).level (-11).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Derelict Weight", "Bass").osc (1, analog, sawPos).unison (1, 2, 12, 35).filter (1, ladder24, 300, 40, 30).env (2, 0, 150, 0, 100, 75)
            .mod (env2, "filter1Cutoff", 30).amp (1, 180, 0, 60).mono (0, false).delay (sixteenthD, 25, 10, 2500).mod (velocity, "filter1Cutoff", 15)
            .level (-3).standardMacros (25);
        add ("Feral Anvil", "Bass").osc (1, analog, sawPos).osc (2, analog, squarePos, 60).pitch (2, -12, 5).subOsc (45)
            .filter (1, ladder12, 530, 20, 20).env (2, 0, 300, 15, 130, 70).mod (env2, "filter1Cutoff", 40).amp (2, 280, 80, 90).mono (0, false)
            .mod (velocity, "filter1Cutoff", 10).level (-4).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Feral Drive", "Bass").osc (1, analog, sawPos).unison (1, 2, 10, 30).filter (1, ladder24, 320, 35, 30).env (2, 0, 120, 0, 100, 75)
            .mod (env2, "filter1Cutoff", 30).amp (1, 180, 0, 60).mono (0, false).delay (eighthD, 20, 8, 2500).mod (velocity, "filter1Cutoff", 9)
            .level (-3).standardMacros (25);
        add ("Feral Hammer", "Bass").osc (1, growl, 30).warp (1, wFold, 20).subOsc (40).filter (1, ladder24, 900, 40, 60)
            .lfoSync (1, sine, sixteenth, 1).mod (lfo1, "osc1Position", 25).mod (lfo1, "filter1Cutoff", 15).amp (1, 300, 85, 80).mono()
            .drive (foldDrive, 45, 5000).level (-16).standardMacros (25).macro (4, "Warp").mod (macro4, "osc1WarpAmount", 50);
        add ("Foundry Engine", "Bass").osc (1, analog, sawPos).subOsc (30, 0, 1).filter (1, ladder24, 320, 10, 15).env (2, 0, 230, 10, 90, 80)
            .mod (env2, "filter1Cutoff", 35).amp (3, 290, 0, 100).mod (velocity, "filter1Cutoff", 13).level (-8).standardMacros (25).macro (4, "Body")
            .mod (macro4, "subLevel", 40);
        add ("Foundry Press", "Bass").osc (1, analog, 85).subOsc (30, 0, 1).filter (1, ladder12, 270, 25, 20).env (2, 0, 160, 15, 160, 70)
            .mod (env2, "filter1Cutoff", 30).amp (1, 260, 80, 100).mono().mod (velocity, "filter1Cutoff", 12).level (-5).standardMacros (25)
            .macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Foundry Pump", "Bass").osc (1, sub, sinePos).subOsc (45, 1, 1).filter (1, ladder24, 800, 10).amp (3, 300, 95, 110).mono()
            .env (2, 0, 70, 0, 60, 90).mod (env2, "osc1Pitch", 22).level (-5).macro (1, "Grit").mod (macro1, "filter1Drive", 60).macro (2, "Body")
            .mod (macro2, "subLevel", 50).macro (3, "Tone").mod (macro3, "filter1Cutoff", 30);
        add ("Foundry Slab", "Bass").osc (1, analog, sawPos).osc (2, analog, squarePos, 65).pitch (2, 0, -6).subOsc (45, 0, 1)
            .filter (1, ladder24, 240, 25, 30).env (2, 0, 220, 15, 180, 60).mod (env2, "filter1Cutoff", 25).amp (2, 160, 80, 100)
            .drive (tube, 35, 3000).mod (velocity, "filter1Cutoff", 8).level (-19).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Foundry Throb", "Bass").osc (1, sub, 0).subOsc (40, 1, 1).filter (1, ladder24, 350, 10).amp (3, 400, 95, 120).mono()
            .env (2, 0, 70, 0, 60, 90).mod (env2, "osc1Pitch", 8).level (-3).macro (1, "Grit").mod (macro1, "filter1Drive", 60).macro (2, "Body")
            .mod (macro2, "subLevel", 50).macro (3, "Tone").mod (macro3, "filter1Cutoff", 30);
        add ("Foundry Thud", "Bass").osc (1, fm, 20).subOsc (30).env (2, 0, 160, 10, 120, 70).mod (env2, "osc1Position", 45)
            .filter (1, ladder24, 1500, 25).amp (1, 320, 75, 80).mono (30).mod (velocity, "osc1Position", 15).drive (hard, 25, 6000).level (-11)
            .standardMacros (25).macro (4, "Index").mod (macro4, "osc1Position", 50);
        add ("Frozen Crank", "Bass").osc (1, analog, 85).osc (2, analog, squarePos, 60).pitch (2, -12, -6).subOsc (35)
            .filter (1, ladder24, 260, 25, 50).env (2, 0, 150, 15, 140, 60).mod (env2, "filter1Cutoff", 20).amp (1, 320, 90, 80).mono (40)
            .mod (velocity, "filter1Cutoff", 9).level (-2).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Frozen Turbine", "Bass").osc (1, analog, 85).osc (2, analog, squarePos, 60).pitch (2, 0, 5).subOsc (30)
            .filter (1, ladder24, 580, 15, 10).env (2, 0, 180, 15, 90, 75).mod (env2, "filter1Cutoff", 30).amp (2, 350, 80, 50).drive (hard, 15, 6000)
            .mod (velocity, "filter1Cutoff", 6).level (-16).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Ground Floor", "Bass").osc (1, sub, 0).subOsc (50, 1, 0).filter (1, ladder24, 700, 5).amp (2, 800, 85, 130).mono().level (-5)
            .macro (1, "Grit").mod (macro1, "filter1Drive", 60).macro (2, "Body").mod (macro2, "subLevel", 50).macro (3, "Tone")
            .mod (macro3, "filter1Cutoff", 30);
        add ("Heavy Piston", "Bass").osc (1, fm, 40).subOsc (35).env (2, 0, 290, 5, 120, 70).mod (env2, "osc1Position", 30)
            .filter (1, svfLow, 1300, 25).amp (1, 360, 85, 80).mono (30).mod (velocity, "osc1Position", 13).level (-5).standardMacros (25)
            .macro (4, "Index").mod (macro4, "osc1Position", 50);
        add ("Hollow Ballast", "Bass").osc (1, vowel, 40).warp (1, wSqueeze, 30).subOsc (35).filter (1, svfLow, 600, 35, 25)
            .env (2, 0, 230, 20, 150, 60).mod (env2, "filter1Cutoff", 30).amp (1, 300, 85, 80).mono().drive (foldDrive, 30, 5500).level (-13)
            .standardMacros (25).macro (4, "Warp").mod (macro4, "osc1WarpAmount", 50);
        add ("Industrial Weight", "Bass").osc (1, analog, squarePos).subOsc (40, 0, 1).filter (1, ladder24, 460, 25, 40).env (2, 0, 280, 10, 100, 65)
            .mod (env2, "filter1Cutoff", 25).amp (1, 280, 60, 60).mono().mod (velocity, "filter1Cutoff", 10).level (-3).standardMacros (25)
            .macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Iron Girder", "Bass").osc (1, analog, squarePos).osc (2, analog, sawPos, 45).pitch (2, -12).subOsc (35)
            .filter (1, ladder12, 410, 35, 35).env (2, 0, 210, 5, 170, 80).mod (env2, "filter1Cutoff", 25).amp (2, 240, 80, 110).mono (40)
            .mod (velocity, "filter1Cutoff", 8).level (-6).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Leaden Weight", "Bass").osc (1, analog, 85).subOsc (40, 0, 1).filter (1, ladder24, 410, 30, 5).env (2, 0, 150, 10, 200, 60)
            .mod (env2, "filter1Cutoff", 35).amp (2, 150, 60, 50).mod (velocity, "filter1Cutoff", 7).level (-10).standardMacros (25).macro (4, "Body")
            .mod (macro4, "subLevel", 40);
        add ("Low Girder", "Bass").osc (1, fm, 50).subOsc (30).env (2, 0, 190, 0, 120, 70).mod (env2, "osc1Position", 30).filter (1, svfLow, 1100, 20)
            .amp (1, 340, 75, 80).mono().mod (velocity, "osc1Position", 16).level (-3).standardMacros (25).macro (4, "Index")
            .mod (macro4, "osc1Position", 50);
        add ("Molten Anvil", "Bass").osc (1, analog, squarePos).osc (2, analog, sawPos, 45).pitch (2, -12).subOsc (35)
            .filter (1, ladder12, 490, 25, 20).env (2, 0, 140, 10, 150, 70).mod (env2, "filter1Cutoff", 25).amp (1, 380, 60, 100).mono (0, false)
            .mod (velocity, "filter1Cutoff", 5).level (-5).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Molten Grind", "Bass").osc (1, growl, 65).warp (1, wFold, 20).subOsc (40).filter (1, ladder24, 1000, 30, 50)
            .lfoSync (1, sawDown, sixteenth, 1).mod (lfo1, "osc1Position", 30).mod (lfo1, "filter1Cutoff", 20).amp (1, 300, 85, 80).mono()
            .drive (fuzz, 35, 4500).level (-20).standardMacros (25).macro (4, "Warp").mod (macro4, "osc1WarpAmount", 50);
        add ("Molten Rumble", "Bass").osc (1, pulse, 65).warp (1, wBend, 40).subOsc (30).filter (1, ladder24, 1100, 40, 50)
            .lfoSync (1, sine, eighth, 1).mod (lfo1, "osc1Position", 25).mod (lfo1, "filter1Cutoff", 15).amp (1, 300, 85, 80).mono()
            .drive (hard, 50, 7000).level (-15).standardMacros (25).macro (4, "Warp").mod (macro4, "osc1WarpAmount", 50);
        add ("Molten Turbine", "Bass").osc (1, fm, 15).subOsc (40).env (2, 0, 290, 15, 120, 70).mod (env2, "osc1Position", 40)
            .filter (1, svfLow, 1300, 15).amp (1, 230, 75, 80).mono().mod (velocity, "osc1Position", 14).level (-5).standardMacros (25)
            .macro (4, "Index").mod (macro4, "osc1Position", 50);
        add ("Mute Crank", "Bass").osc (1, analog, sawPos).osc (2, analog, sawPos, 65).pitch (2, 12, -6).subOsc (30).filter (1, ladder24, 480, 40, 25)
            .env (2, 0, 150, 10, 110, 85).mod (env2, "filter1Cutoff", 20).amp (2, 230, 0, 110).drive (hard, 15, 5500)
            .mod (velocity, "filter1Cutoff", 6).level (-13).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Mute Grind", "Bass").osc (1, pulse, 55).warp (1, wSqueeze, 25).subOsc (30).filter (1, svfLow, 900, 25, 50).env (2, 0, 250, 20, 150, 60)
            .mod (env2, "filter1Cutoff", 20).amp (1, 300, 85, 80).mono().drive (hard, 25, 3500).level (-10).standardMacros (25).macro (4, "Warp")
            .mod (macro4, "osc1WarpAmount", 50);
        add ("Mute Hammer", "Bass").osc (1, pulse, 65).warp (1, wSqueeze, 15).subOsc (45).filter (1, svfLow, 1050, 30, 35)
            .lfoSync (1, sawDown, eighthT, 1).mod (lfo1, "osc1Position", 30).mod (lfo1, "filter1Cutoff", 15).amp (1, 300, 85, 80).mono()
            .drive (hard, 35, 6000).level (-14).standardMacros (25).macro (4, "Warp").mod (macro4, "osc1WarpAmount", 50);
        add ("Nickel Groan", "Bass").osc (1, pulse, 65).warp (1, wFold, 15).subOsc (40).filter (1, svfLow, 600, 25, 50)
            .lfoSync (1, sawDown, eighth, 1).mod (lfo1, "osc1Position", 25).mod (lfo1, "filter1Cutoff", 20).amp (1, 300, 85, 80).mono()
            .drive (hard, 35, 5000).level (-13).standardMacros (25).macro (4, "Warp").mod (macro4, "osc1WarpAmount", 50);
        add ("Nickel Hammer", "Bass").osc (1, analog, 85).subOsc (40).filter (1, ladder24, 420, 15, 30).env (2, 0, 180, 0, 100, 85)
            .mod (env2, "filter1Cutoff", 30).amp (3, 230, 0, 90).drive (tube, 30, 7000).mod (velocity, "filter1Cutoff", 14).level (-15)
            .standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Night Mass", "Bass").osc (1, growl, 20).warp (1, wFold, 30).subOsc (35).filter (1, ladder24, 1000, 30, 50)
            .lfoSync (1, triangle, quarter, 1).mod (lfo1, "osc1Position", 35).mod (lfo1, "filter1Cutoff", 15).amp (1, 300, 85, 80).mono()
            .drive (foldDrive, 40, 7000).level (-15).standardMacros (25).macro (4, "Warp").mod (macro4, "osc1WarpAmount", 50);
        add ("Night Turbine", "Bass").osc (1, fm, 55).subOsc (45).env (2, 0, 290, 20, 120, 70).mod (env2, "osc1Position", 40)
            .filter (1, ladder24, 2400, 15).amp (1, 270, 70, 80).mono().mod (velocity, "osc1Position", 19).level (-2).standardMacros (25)
            .macro (4, "Index").mod (macro4, "osc1Position", 50);
        add ("Obsidian Crank", "Bass").osc (1, analog, sawPos).unison (1, 2, 7, 40).filter (1, ladder24, 370, 45, 40).env (2, 0, 120, 0, 100, 75)
            .mod (env2, "filter1Cutoff", 30).amp (1, 140, 0, 60).mono (0, false).delay (sixteenthD, 25, 14, 2500).mod (velocity, "filter1Cutoff", 11)
            .level (-3).standardMacros (25);
        add ("Obsidian Mass", "Bass").osc (1, analog, squarePos).osc (2, analog, sawPos, 65).pitch (2, 0, 5).subOsc (40)
            .filter (1, ladder24, 230, 20, 25).env (2, 0, 170, 10, 120, 70).mod (env2, "filter1Cutoff", 35).amp (1, 310, 0, 90).mono (20)
            .drive (soft, 15, 5000).mod (velocity, "filter1Cutoff", 14).level (-8).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Pressed Ballast", "Bass").osc (1, analog, sawPos).filter (1, ladder24, 210, 55, 30).env (2, 0, 100, 0, 100, 75)
            .mod (env2, "filter1Cutoff", 30).amp (1, 200, 0, 60).mono (0, false).mod (velocity, "filter1Cutoff", 18).level (-4).standardMacros (25);
        add ("Pylon Piston", "Bass").osc (1, analog, sawPos).filter (1, ladder24, 260, 45, 25).env (2, 0, 150, 0, 100, 75)
            .mod (env2, "filter1Cutoff", 25).amp (1, 130, 0, 60).mono (0, false).delay (eighthD, 30, 11, 2500).mod (velocity, "filter1Cutoff", 13)
            .level (-3).standardMacros (25);
        add ("Pylon Pump", "Bass").osc (1, analog, sawPos).unison (1, 3, 7, 25).filter (1, ladder24, 230, 50, 20).env (2, 0, 120, 0, 100, 75)
            .mod (env2, "filter1Cutoff", 35).amp (1, 130, 0, 60).mono (0, false).delay (eighthD, 25, 9, 2500).mod (velocity, "filter1Cutoff", 8)
            .level (-1).standardMacros (25);
        add ("Pylon Slab", "Bass").osc (1, analog, sawPos).unison (1, 2, 7, 40).filter (1, ladder24, 330, 50, 15).env (2, 0, 120, 0, 100, 75)
            .mod (env2, "filter1Cutoff", 35).amp (1, 130, 0, 60).mono (0, false).delay (eighthD, 25, 16, 2500).mod (velocity, "filter1Cutoff", 10)
            .level (-3).standardMacros (25);
        add ("Riveted Ballast", "Bass").osc (1, fm, 55).subOsc (25).env (2, 0, 230, 15, 120, 70).mod (env2, "osc1Position", 25)
            .filter (1, svfLow, 1300, 15).amp (1, 340, 70, 80).mono (30).mod (velocity, "osc1Position", 12).level (-3).standardMacros (25)
            .macro (4, "Index").mod (macro4, "osc1Position", 50);
        add ("Riveted Press", "Bass").osc (1, vowel, 20).warp (1, wFm, 35).subOsc (35).filter (1, svfLow, 700, 25, 20).env (2, 0, 180, 20, 150, 60)
            .mod (env2, "filter1Cutoff", 25).amp (1, 300, 85, 80).mono().drive (hard, 45, 6500).level (-15).standardMacros (25).macro (4, "Warp")
            .mod (macro4, "osc1WarpAmount", 50);
        add ("Riveted Thud", "Bass").osc (1, sub, sinePos).subOsc (25).filter (1, ladder24, 350, 15).drive (soft, 25, 3250).amp (2, 600, 90, 80)
            .mono().level (-8).macro (1, "Grit").mod (macro1, "fxDriveAmount", 60).macro (2, "Body").mod (macro2, "subLevel", 50).macro (3, "Tone")
            .mod (macro3, "filter1Cutoff", 30);
        add ("Ruined Anvil", "Bass").osc (1, fm, 60).subOsc (30).env (2, 0, 220, 15, 120, 70).mod (env2, "osc1Position", 40)
            .filter (1, ladder24, 1700, 30).amp (1, 350, 85, 80).mono().mod (velocity, "osc1Position", 10).level (-3).standardMacros (25)
            .macro (4, "Index").mod (macro4, "osc1Position", 50);
        add ("Ruined Ballast", "Bass").osc (1, analog, 85).osc (2, analog, squarePos, 65).pitch (2, 12, 5).subOsc (25)
            .filter (1, ladder24, 600, 45, 45).env (2, 0, 170, 0, 160, 75).mod (env2, "filter1Cutoff", 35).amp (2, 400, 90, 90).mono (40)
            .drive (soft, 35, 4500).mod (velocity, "filter1Cutoff", 13).level (-11).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Ruined Hammer", "Bass").osc (1, fm, 50).subOsc (30).env (2, 0, 130, 15, 120, 70).mod (env2, "osc1Position", 40)
            .filter (1, ladder24, 2300, 15).amp (1, 360, 70, 80).mono (30).mod (velocity, "osc1Position", 10).level (-2).standardMacros (25)
            .macro (4, "Index").mod (macro4, "osc1Position", 50);
        add ("Rust Drive", "Bass").osc (1, growl, 30).warp (1, wBend, 30).subOsc (35).filter (1, svfLow, 850, 35, 55)
            .lfoSync (1, sawDown, sixteenth, 1).mod (lfo1, "osc1Position", 15).mod (lfo1, "filter1Cutoff", 20).amp (1, 300, 85, 80).mono()
            .drive (foldDrive, 45, 5500).level (-16).standardMacros (25).macro (4, "Warp").mod (macro4, "osc1WarpAmount", 50);
        add ("Rust Pump", "Bass").osc (1, analog, squarePos).osc (2, analog, squarePos, 60).pitch (2, -12, 7).subOsc (30)
            .filter (1, ladder12, 250, 25, 25).env (2, 0, 230, 20, 130, 85).mod (env2, "filter1Cutoff", 30).amp (3, 270, 0, 110).mono (20)
            .drive (tube, 30, 5500).mod (velocity, "filter1Cutoff", 10).level (-11).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Scorched Anvil", "Bass").osc (1, fm, 20).subOsc (40).env (2, 0, 220, 10, 120, 70).mod (env2, "osc1Position", 25)
            .filter (1, ladder24, 900, 20).amp (1, 290, 75, 80).mono().mod (velocity, "osc1Position", 14).level (-1).standardMacros (25)
            .macro (4, "Index").mod (macro4, "osc1Position", 50);
        add ("Scorched Slab", "Bass").osc (1, analog, sawPos).unison (1, 2, 6, 40).filter (1, ladder24, 380, 40, 30).env (2, 0, 120, 0, 100, 75)
            .mod (env2, "filter1Cutoff", 35).amp (1, 160, 0, 60).mono (0, false).mod (velocity, "filter1Cutoff", 17).level (-3).standardMacros (25);
        add ("Scorched Sub", "Bass").osc (1, analog, sawPos).filter (1, ladder24, 280, 35, 20).env (2, 0, 170, 0, 100, 75)
            .mod (env2, "filter1Cutoff", 30).amp (1, 180, 0, 60).mono (0, false).delay (eighthD, 15, 10, 2500).mod (velocity, "filter1Cutoff", 13)
            .level (-3).standardMacros (25);
        add ("Severed Dredge", "Bass").osc (1, sync, 60).warp (1, wFm, 35).subOsc (30).filter (1, svfLow, 1050, 45, 45).lfoSync (1, sine, quarter, 1)
            .mod (lfo1, "osc1Position", 25).mod (lfo1, "filter1Cutoff", 10).amp (1, 300, 85, 80).mono().drive (fuzz, 45, 5500).level (-20)
            .standardMacros (25).macro (4, "Warp").mod (macro4, "osc1WarpAmount", 50);
        add ("Sewer Bass", "Bass").osc (1, analog, squarePos).osc (2, analog, sawPos, 50).pitch (2, -12, -6).subOsc (25)
            .filter (1, ladder12, 530, 45, 30).env (2, 0, 260, 15, 180, 60).mod (env2, "filter1Cutoff", 30).amp (2, 210, 60, 70)
            .mod (velocity, "filter1Cutoff", 6).level (-13).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Shadow Anvil", "Bass").osc (1, growl, 30).warp (1, wBend, 45).subOsc (45).filter (1, svfLow, 800, 40, 50).lfoSync (1, sine, eighthT, 1)
            .mod (lfo1, "osc1Position", 15).mod (lfo1, "filter1Cutoff", 10).amp (1, 300, 85, 80).mono().drive (foldDrive, 30, 5500).level (-13)
            .standardMacros (25).macro (4, "Warp").mod (macro4, "osc1WarpAmount", 50);
        add ("Sodium Drive", "Bass").osc (1, analog, sawPos).unison (1, 2, 8, 30).filter (1, ladder24, 270, 50, 30).env (2, 0, 100, 0, 100, 75)
            .mod (env2, "filter1Cutoff", 25).amp (1, 190, 0, 60).mono (0, false).delay (eighthD, 35, 10, 2500).mod (velocity, "filter1Cutoff", 9)
            .level (-3).standardMacros (25);
        add ("Sodium Girder", "Bass").osc (1, analog, sawPos).unison (1, 2, 11, 35).filter (1, ladder24, 340, 30, 40).env (2, 0, 150, 0, 100, 75)
            .mod (env2, "filter1Cutoff", 25).amp (1, 210, 0, 60).mono (0, false).mod (velocity, "filter1Cutoff", 14).level (-3).standardMacros (25);
        add ("Sodium Throb", "Bass").osc (1, fm, 45).subOsc (45).env (2, 0, 150, 15, 120, 70).mod (env2, "osc1Position", 25)
            .filter (1, svfLow, 1500, 15).amp (1, 240, 85, 80).mono (30).mod (velocity, "osc1Position", 11).level (-5).standardMacros (25)
            .macro (4, "Index").mod (macro4, "osc1Position", 50);
        add ("Static Grind", "Bass").osc (1, analog, sawPos).subOsc (35).filter (1, ladder24, 500, 10, 30).env (2, 0, 130, 10, 190, 75)
            .mod (env2, "filter1Cutoff", 30).amp (3, 390, 90, 110).drive (soft, 40, 5500).mod (velocity, "filter1Cutoff", 5).level (-19)
            .standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Steel Slab", "Bass").osc (1, analog, 0).subOsc (40, 0, 1).filter (1, ladder24, 700, 10).amp (3, 650, 95, 120).mono()
            .env (2, 0, 60, 0, 60, 90).mod (env2, "osc1Pitch", 18).level (-4).macro (1, "Grit").mod (macro1, "filter1Drive", 60).macro (2, "Body")
            .mod (macro2, "subLevel", 50).macro (3, "Tone").mod (macro3, "filter1Cutoff", 30);
        add ("Sullen Grind", "Bass").osc (1, analog, 85).subOsc (40, 0, 1).filter (1, ladder24, 290, 30, 25).env (2, 0, 280, 0, 200, 80)
            .mod (env2, "filter1Cutoff", 30).amp (3, 150, 90, 50).mono().drive (tube, 40, 3000).mod (velocity, "filter1Cutoff", 6).level (-12)
            .standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Sunken Pump", "Bass").osc (1, analog, sawPos).unison (1, 3, 10, 50).filter (1, ladder24, 390, 30, 25).env (2, 0, 130, 0, 100, 75)
            .mod (env2, "filter1Cutoff", 35).amp (1, 150, 0, 60).mono (0, false).delay (eighthD, 15, 17, 2500).mod (velocity, "filter1Cutoff", 8)
            .level (-2).standardMacros (25);
        add ("Tar Press", "Bass").osc (1, analog, sawPos).subOsc (30).filter (1, ladder12, 540, 40, 25).env (2, 0, 100, 15, 170, 75)
            .mod (env2, "filter1Cutoff", 30).amp (2, 160, 0, 110).drive (tube, 35, 7500).mod (velocity, "filter1Cutoff", 6).level (-17)
            .standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Tar Weight", "Bass").osc (1, sub, 10).subOsc (45, 0, 1).filter (1, ladder24, 350, 10).drive (soft, 35, 2250).amp (2, 700, 100, 100)
            .mono().env (2, 0, 85, 0, 60, 90).mod (env2, "osc1Pitch", 23).level (-11).macro (1, "Grit").mod (macro1, "fxDriveAmount", 60)
            .macro (2, "Body").mod (macro2, "subLevel", 50).macro (3, "Tone").mod (macro3, "filter1Cutoff", 30);
        add ("Toxic Mass", "Bass").osc (1, fm, 60).subOsc (45).env (2, 0, 140, 5, 120, 70).mod (env2, "osc1Position", 40)
            .filter (1, ladder24, 800, 15).amp (1, 310, 75, 80).mono().mod (velocity, "osc1Position", 15).level (-1).standardMacros (25)
            .macro (4, "Index").mod (macro4, "osc1Position", 50);
        add ("Toxic Press", "Bass").osc (1, analog, 85).osc (2, analog, squarePos, 55).pitch (2, 12, -6).subOsc (30).filter (1, ladder24, 200, 50, 5)
            .env (2, 0, 280, 15, 90, 70).mod (env2, "filter1Cutoff", 20).amp (2, 260, 0, 100).mono (20).drive (soft, 25, 3500)
            .mod (velocity, "filter1Cutoff", 8).level (-8).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Toxic Pulse", "Bass").osc (1, analog, squarePos).osc (2, analog, squarePos, 40).pitch (2, 12, 7).subOsc (30, 0, 1)
            .filter (1, ladder24, 230, 35, 40).env (2, 0, 190, 5, 100, 85).mod (env2, "filter1Cutoff", 20).amp (2, 190, 80, 80).mono (0, false)
            .mod (velocity, "filter1Cutoff", 7).level (-3).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Tunnel Sub", "Bass").osc (1, sub, 10).subOsc (45).filter (1, ladder24, 650, 20).drive (tube, 35, 3000).amp (1, 350, 90, 60).mono()
            .level (-12).macro (1, "Grit").mod (macro1, "fxDriveAmount", 60).macro (2, "Body").mod (macro2, "subLevel", 50).macro (3, "Tone")
            .mod (macro3, "filter1Cutoff", 30);
        add ("Tunnel Throb", "Bass").osc (1, analog, squarePos).osc (2, analog, sawPos, 40).pitch (2, -12, 7).subOsc (30)
            .filter (1, ladder24, 230, 45, 40).env (2, 0, 170, 15, 170, 80).mod (env2, "filter1Cutoff", 30).amp (1, 180, 0, 100).mono()
            .drive (tube, 20, 6000).mod (velocity, "filter1Cutoff", 11).level (-8).standardMacros (25).macro (4, "Body").mod (macro4, "subLevel", 40);
        add ("Vacant Bass", "Bass").osc (1, analog, 30).subOsc (35).filter (1, ladder24, 800, 5).drive (soft, 30, 2250).amp (2, 750, 80, 80).mono()
            .env (2, 0, 45, 0, 60, 90).mod (env2, "osc1Pitch", 20).level (-8).macro (1, "Grit").mod (macro1, "fxDriveAmount", 60).macro (2, "Body")
            .mod (macro2, "subLevel", 50).macro (3, "Tone").mod (macro3, "filter1Cutoff", 30);
        add ("Vacant Pump", "Bass").osc (1, pulse, 50).warp (1, wFold, 25).subOsc (45).filter (1, ladder24, 450, 30, 25).env (2, 0, 210, 20, 150, 60)
            .mod (env2, "filter1Cutoff", 25).amp (1, 300, 85, 80).mono().drive (foldDrive, 35, 7000).level (-12).standardMacros (25).macro (4, "Warp")
            .mod (macro4, "osc1WarpAmount", 50);
        add ("Void Floor", "Bass").osc (1, fm, 20).subOsc (35).env (2, 0, 130, 10, 120, 70).mod (env2, "osc1Position", 35)
            .filter (1, ladder24, 800, 15).amp (1, 300, 60, 80).mono().mod (velocity, "osc1Position", 17).level (-1).standardMacros (25)
            .macro (4, "Index").mod (macro4, "osc1Position", 50);
        add ("Void Press", "Bass").osc (1, sub, 10).subOsc (30, 1, 1).filter (1, ladder24, 750, 15).drive (tube, 45, 3250).amp (4, 400, 85, 110)
            .mono().env (2, 0, 80, 0, 60, 90).mod (env2, "osc1Pitch", 20).level (-14).macro (1, "Grit").mod (macro1, "fxDriveAmount", 60)
            .macro (2, "Body").mod (macro2, "subLevel", 50).macro (3, "Tone").mod (macro3, "filter1Cutoff", 30);
        add ("Zinc Ballast", "Bass").osc (1, fm, 30).subOsc (40).env (2, 0, 230, 5, 120, 70).mod (env2, "osc1Position", 30)
            .filter (1, ladder24, 900, 30).amp (1, 340, 80, 80).mono (30).mod (velocity, "osc1Position", 14).drive (hard, 25, 6000).level (-12)
            .standardMacros (25).macro (4, "Index").mod (macro4, "osc1Position", 50);

        // Reese, by name.
        add ("Acid Helix", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos, 70).pitch (2, -12, 7).unison (1, 4, 16, 45).unison (2, 1, 13, 65)
            .filter (1, svfLow, 500, 25, 25).lfo (1, drift, 0.08f, 0).mod (lfo1, "filter1Cutoff", 14).subOsc (35).mono().amp (9, 400, 90, 240)
            .level (-2).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Ash Swell", "Reese").osc (1, sync, 45).osc (2, analog, sawPos, 70).pitch (2, -12, 7).unison (1, 2, 20, 50).unison (2, 2, 16, 45)
            .filter (1, ladderBand, 1050, 20, 45).lfo (1, triangle, 0.1f, 0).mod (lfo1, "filter1Cutoff", 12).phaser (40, 0.27f, 45).subOsc (30).mono()
            .amp (3, 400, 90, 150).level (-4).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Ash Wire", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos).pitch (2, 0, 6).unison (1, 3, 12, 45).unison (2, 2, 15, 55)
            .filter (1, ladder24, 800, 35, 30).lfo (1, drift, 0.31f, 0).mod (lfo1, "filter1Cutoff", 12).subOsc (35).mono().amp (7, 400, 90, 170)
            .level (-4).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Basalt Rope", "Reese").osc (1, growl, 30).osc (2, analog, sawPos, 70).pitch (2, -12, 10).unison (1, 3, 20, 55).unison (2, 2, 18, 65)
            .filter (1, ladder24, 850, 40, 35).lfo (1, triangle, 0.11f, 0).mod (lfo1, "filter1Cutoff", 11).phaser (30, 0.13f, 60).subOsc (25).mono()
            .amp (7, 400, 90, 210).level (-5).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Bitter Current", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, 0, 15).unison (1, 2, 11, 55)
            .unison (2, 1, 14, 60).filter (1, svfLow, 800, 30, 35).lfo (1, triangle, 0.16f, 0).mod (lfo1, "filter1Cutoff", 14).phaser (30, 0.26f, 55)
            .subOsc (35).mono().amp (8, 400, 90, 180).level (-6).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Bitter Reese", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos, 65).pitch (2, 0, 13).unison (1, 2, 12, 45).unison (2, 1, 18, 60)
            .filter (1, svfLow, 1100, 35, 40).lfo (1, triangle, 0.28f, 0).mod (lfo1, "filter1Cutoff", 10).phaser (30, 0.26f, 40)
            .drive (fuzz, 25, 6000).subOsc (30).mono().amp (8, 400, 90, 140).level (-22).standardMacros (25).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 40);
        add ("Bitter Wire", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos, 65).pitch (2, -12, 15).unison (1, 4, 20, 70)
            .unison (2, 3, 12, 55).filter (1, ladderBand, 650, 35, 35).lfo (1, drift, 0.32f, 0).mod (lfo1, "filter1Cutoff", 11).subOsc (30).mono()
            .amp (9, 400, 90, 160).level (-5).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Burnt Undertow", "Reese").osc (1, growl, 35).osc (2, analog, sawPos, 65).pitch (2, 0, 7).unison (1, 4, 18, 50).unison (2, 1, 11, 70)
            .filter (1, ladder24, 700, 25, 25).lfo (1, drift, 0.28f, 0).mod (lfo1, "filter1Cutoff", 6).drive (hard, 30, 6000).subOsc (35).mono()
            .amp (7, 400, 90, 150).level (-12).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Chrome Cable", "Reese").osc (1, sync, 45).osc (2, analog, sawPos, 65).pitch (2, 0, 6).unison (1, 2, 12, 55).unison (2, 1, 16, 65)
            .filter (1, ladder24, 950, 30, 30).lfo (1, drift, 0.24f, 0).mod (lfo1, "filter1Cutoff", 6).phaser (45, 0.13f, 40).drive (tube, 35, 4000)
            .subOsc (35).mono().amp (9, 400, 90, 200).level (-10).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Cobalt Vortex", "Reese").osc (1, pulse, 55).osc (2, analog, sawPos, 75).pitch (2, -12, 16).unison (1, 2, 19, 45).unison (2, 2, 17, 55)
            .filter (1, ladderBand, 1050, 35, 25).lfo (1, drift, 0.15f, 0).mod (lfo1, "filter1Cutoff", 13).drive (hard, 20, 4500).subOsc (30).mono()
            .amp (4, 400, 90, 210).level (-12).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Cold Cable", "Reese").osc (1, pulse, 50).osc (2, analog, sawPos, 70).pitch (2, 0, 16).unison (1, 2, 20, 45).unison (2, 2, 15, 70)
            .filter (1, ladder24, 500, 35, 50).lfo (1, drift, 0.32f, 0).mod (lfo1, "filter1Cutoff", 11).phaser (40, 0.22f, 65).subOsc (25).mono()
            .amp (7, 400, 90, 170).level (-2).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Concrete Undertow", "Reese").osc (1, pulse, 45).osc (2, analog, sawPos, 60).pitch (2, 0, 9).unison (1, 2, 18, 70).unison (2, 1, 11, 55)
            .filter (1, svfLow, 650, 30, 35).lfo (1, drift, 0.2f, 0).mod (lfo1, "filter1Cutoff", 7).phaser (45, 0.09f, 65).drive (tube, 25, 6500)
            .subOsc (25).mono().amp (8, 400, 90, 160).level (-12).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Corroded Helix", "Reese").osc (1, pulse, 45).osc (2, analog, sawPos, 60).pitch (2, 0, 15).unison (1, 4, 20, 40).unison (2, 3, 11, 60)
            .filter (1, ladder24, 750, 35, 30).lfo (1, sine, 0.33f, 0).mod (lfo1, "filter1Cutoff", 13).phaser (25, 0.23f, 60).subOsc (35).mono()
            .amp (5, 400, 90, 170).level (-4).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Corroded Undertow", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos, 70).pitch (2, 0, 17).unison (1, 3, 22, 70)
            .unison (2, 1, 14, 60).filter (1, ladder24, 450, 30, 50).lfo (1, triangle, 0.12f, 0).mod (lfo1, "filter1Cutoff", 11)
            .drive (fuzz, 25, 4500).subOsc (40).mono().amp (3, 400, 90, 160).level (-19).standardMacros (25).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 40);
        add ("Derelict Current", "Reese").osc (1, sync, 45).osc (2, analog, sawPos, 65).pitch (2, 0, 13).unison (1, 4, 21, 55).unison (2, 1, 19, 40)
            .filter (1, ladder24, 1200, 35, 20).lfo (1, triangle, 0.15f, 0).mod (lfo1, "filter1Cutoff", 9).drive (fuzz, 35, 6500).subOsc (25).mono()
            .amp (8, 400, 90, 220).level (-21).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Drifting Rope", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos, 60).pitch (2, -12, 16).unison (1, 3, 12, 50)
            .unison (2, 1, 14, 45).filter (1, ladderBand, 750, 30, 35).lfo (1, sine, 0.23f, 0).mod (lfo1, "filter1Cutoff", 11).drive (hard, 35, 5500)
            .subOsc (30).mono().amp (9, 400, 90, 180).level (-13).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Feral Cable", "Reese").osc (1, growl, 35).osc (2, analog, sawPos, 70).pitch (2, 0, 6).unison (1, 4, 22, 50).unison (2, 2, 19, 50)
            .filter (1, ladder24, 950, 30, 50).lfo (1, triangle, 0.22f, 0).mod (lfo1, "filter1Cutoff", 13).phaser (35, 0.12f, 50)
            .drive (hard, 40, 6500).subOsc (30).mono().amp (4, 400, 90, 240).level (-16).standardMacros (25).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 40);
        add ("Heavy Rope", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, 0, 16).unison (1, 3, 22, 45).unison (2, 3, 12, 55)
            .filter (1, ladderBand, 600, 40, 35).lfo (1, drift, 0.36f, 0).mod (lfo1, "filter1Cutoff", 15).subOsc (30).mono().amp (8, 400, 90, 220)
            .level (-7).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Industrial Swell", "Reese").osc (1, growl, 50).osc (2, analog, sawPos, 70).pitch (2, 0, 8).unison (1, 3, 20, 40).unison (2, 3, 12, 70)
            .filter (1, svfLow, 750, 25, 25).lfo (1, triangle, 0.21f, 0).mod (lfo1, "filter1Cutoff", 10).phaser (45, 0.16f, 45).subOsc (30).mono()
            .amp (6, 400, 90, 220).level (-5).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Iron Wire", "Reese").osc (1, growl, 55).osc (2, analog, sawPos, 70).pitch (2, 0, 17).unison (1, 4, 18, 50).unison (2, 3, 13, 50)
            .filter (1, ladder24, 1050, 35, 20).lfo (1, drift, 0.12f, 0).mod (lfo1, "filter1Cutoff", 7).phaser (30, 0.08f, 70).subOsc (30).mono()
            .amp (9, 400, 90, 190).level (-4).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Nickel Wire", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos, 70).pitch (2, 0, 16).unison (1, 3, 16, 50).unison (2, 3, 15, 50)
            .filter (1, ladderBand, 450, 30, 30).lfo (1, triangle, 0.39f, 0).mod (lfo1, "filter1Cutoff", 14).phaser (35, 0.11f, 70).subOsc (40).mono()
            .amp (8, 400, 90, 200).level (-6).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Obsidian Tendril", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, 0, 13).unison (1, 4, 14, 50)
            .unison (2, 3, 13, 45).filter (1, ladder24, 700, 35, 45).lfo (1, drift, 0.09f, 0).mod (lfo1, "filter1Cutoff", 12).subOsc (25).mono()
            .amp (8, 400, 90, 160).level (-3).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Oily Vortex", "Reese").osc (1, pulse, 30).osc (2, analog, sawPos, 65).pitch (2, 0, 6).unison (1, 4, 15, 55).unison (2, 2, 16, 70)
            .filter (1, ladderBand, 600, 20, 35).lfo (1, sine, 0.3f, 0).mod (lfo1, "filter1Cutoff", 10).phaser (35, 0.1f, 45).drive (hard, 35, 5500)
            .subOsc (30).mono().amp (4, 400, 90, 250).level (-12).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Pressed Knot", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, -12, 6).unison (1, 4, 14, 50)
            .unison (2, 3, 17, 50).filter (1, ladder24, 1150, 20, 40).lfo (1, sine, 0.38f, 0).mod (lfo1, "filter1Cutoff", 7).phaser (40, 0.21f, 70)
            .drive (tube, 35, 5500).subOsc (25).mono().amp (4, 400, 90, 150).level (-14).standardMacros (25).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 40);
        add ("Ruined Wire", "Reese").osc (1, growl, 30).osc (2, analog, sawPos, 70).pitch (2, -12, 12).unison (1, 4, 14, 50).unison (2, 3, 12, 55)
            .filter (1, svfLow, 500, 20, 30).lfo (1, sine, 0.26f, 0).mod (lfo1, "filter1Cutoff", 10).subOsc (35).mono().amp (6, 400, 90, 230)
            .level (-5).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Rust Eel", "Reese").osc (1, pulse, 50).osc (2, analog, sawPos).pitch (2, 0, 11).unison (1, 3, 24, 50).unison (2, 1, 18, 60)
            .filter (1, ladderBand, 1200, 40, 30).lfo (1, triangle, 0.27f, 0).mod (lfo1, "filter1Cutoff", 11).subOsc (30).mono().amp (5, 400, 90, 180)
            .level (-6).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Rust Swell", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos, 65).pitch (2, 0, 7).unison (1, 2, 22, 60).unison (2, 1, 11, 55)
            .filter (1, svfLow, 1000, 30, 40).lfo (1, sine, 0.16f, 0).mod (lfo1, "filter1Cutoff", 9).phaser (35, 0.14f, 50).drive (hard, 35, 5500)
            .subOsc (35).mono().amp (9, 400, 90, 190).level (-15).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Scorched Vortex", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos, 65).pitch (2, 0, 13).unison (1, 3, 16, 45)
            .unison (2, 2, 19, 60).filter (1, ladder24, 600, 25, 30).lfo (1, drift, 0.22f, 0).mod (lfo1, "filter1Cutoff", 13).phaser (35, 0.2f, 60)
            .subOsc (30).mono().amp (10, 400, 90, 130).level (-2).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Seasick Knot", "Reese").osc (1, growl, 60).osc (2, analog, sawPos, 60).pitch (2, -12, 16).unison (1, 4, 21, 40).unison (2, 3, 19, 55)
            .filter (1, ladder24, 850, 40, 25).lfo (1, sine, 0.28f, 0).mod (lfo1, "filter1Cutoff", 14).phaser (35, 0.18f, 60).subOsc (25).mono()
            .amp (6, 400, 90, 210).level (-4).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Severed Eel", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos, 65).pitch (2, -12, 7).unison (1, 2, 14, 40).unison (2, 2, 11, 70)
            .filter (1, ladder24, 400, 35, 45).lfo (1, drift, 0.27f, 0).mod (lfo1, "filter1Cutoff", 9).phaser (40, 0.27f, 55).drive (hard, 35, 6000)
            .subOsc (40).mono().amp (7, 400, 90, 200).level (-13).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Severed Spiral", "Reese").osc (1, pulse, 40).osc (2, analog, sawPos, 65).pitch (2, 0, 12).unison (1, 4, 10, 50).unison (2, 3, 18, 50)
            .filter (1, ladder24, 900, 40, 25).lfo (1, triangle, 0.16f, 0).mod (lfo1, "filter1Cutoff", 10).subOsc (30).mono().amp (5, 400, 90, 150)
            .level (-6).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Shadow Drag", "Reese").osc (1, sync, 50).osc (2, analog, sawPos, 70).pitch (2, 0, 15).unison (1, 2, 24, 40).unison (2, 1, 19, 65)
            .filter (1, ladder24, 1100, 35, 35).lfo (1, sine, 0.2f, 0).mod (lfo1, "filter1Cutoff", 8).subOsc (25).mono().amp (5, 400, 90, 190)
            .level (-4).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Soot Drag", "Reese").osc (1, growl, 45).osc (2, analog, sawPos, 70).pitch (2, 0, 7).unison (1, 4, 21, 60).unison (2, 2, 19, 70)
            .filter (1, ladderBand, 1150, 30, 30).lfo (1, triangle, 0.36f, 0).mod (lfo1, "filter1Cutoff", 12).subOsc (30).mono().amp (8, 400, 90, 190)
            .level (-5).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Soot Tendril", "Reese").osc (1, sync, 30).osc (2, analog, sawPos, 65).pitch (2, 0, 17).unison (1, 2, 12, 45).unison (2, 2, 14, 65)
            .filter (1, ladder24, 1200, 40, 40).lfo (1, triangle, 0.3f, 0).mod (lfo1, "filter1Cutoff", 12).phaser (35, 0.26f, 50)
            .drive (hard, 20, 4500).subOsc (30).mono().amp (4, 400, 90, 140).level (-10).standardMacros (25).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 40);
        add ("Soot Undertow", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos, 65).pitch (2, 0, 10).unison (1, 2, 24, 45)
            .unison (2, 2, 17, 60).filter (1, svfLow, 1200, 30, 50).lfo (1, sine, 0.15f, 0).mod (lfo1, "filter1Cutoff", 9).phaser (40, 0.1f, 45)
            .subOsc (30).mono().amp (6, 400, 90, 160).level (-5).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Soot Vortex", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos, 65).pitch (2, -12, 12).unison (1, 3, 24, 45)
            .unison (2, 3, 18, 45).filter (1, svfLow, 850, 40, 35).lfo (1, triangle, 0.4f, 0).mod (lfo1, "filter1Cutoff", 13).phaser (35, 0.16f, 60)
            .drive (fuzz, 20, 5500).subOsc (40).mono().amp (9, 400, 90, 250).level (-21).standardMacros (25).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 40);
        add ("Static Coil", "Reese").osc (1, pulse, 45).osc (2, analog, sawPos, 75).pitch (2, 0, 11).unison (1, 3, 13, 45).unison (2, 3, 12, 65)
            .filter (1, svfLow, 550, 25, 30).lfo (1, drift, 0.17f, 0).mod (lfo1, "filter1Cutoff", 10).drive (hard, 20, 4500).subOsc (30).mono()
            .amp (5, 400, 90, 240).level (-10).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Static Tendril", "Reese").osc (1, growl, 55).osc (2, analog, sawPos, 70).pitch (2, 0, 11).unison (1, 3, 19, 50).unison (2, 3, 13, 45)
            .filter (1, svfLow, 450, 25, 25).lfo (1, triangle, 0.29f, 0).mod (lfo1, "filter1Cutoff", 7).phaser (30, 0.15f, 50).drive (tube, 30, 6500)
            .subOsc (30).mono().amp (4, 400, 90, 200).level (-10).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Steel Rope", "Reese").osc (1, pulse, 40).osc (2, analog, sawPos, 70).pitch (2, 0, 17).unison (1, 3, 17, 45).unison (2, 1, 15, 50)
            .filter (1, ladder24, 850, 30, 45).lfo (1, sine, 0.12f, 0).mod (lfo1, "filter1Cutoff", 6).drive (tube, 20, 6000).subOsc (30).mono()
            .amp (4, 400, 90, 180).level (-8).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Steel Vortex", "Reese").osc (1, sync, 50).osc (2, analog, sawPos, 70).pitch (2, 0, 9).unison (1, 3, 13, 50).unison (2, 2, 18, 65)
            .filter (1, ladderBand, 500, 30, 35).lfo (1, triangle, 0.31f, 0).mod (lfo1, "filter1Cutoff", 15).phaser (40, 0.16f, 70).subOsc (35).mono()
            .amp (7, 400, 90, 190).level (-7).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Sterile Cable", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos).pitch (2, -12, 17).unison (1, 2, 17, 60).unison (2, 1, 11, 70)
            .filter (1, ladder24, 750, 35, 50).lfo (1, triangle, 0.35f, 0).mod (lfo1, "filter1Cutoff", 8).phaser (35, 0.09f, 50)
            .drive (hard, 35, 6000).subOsc (30).mono().amp (8, 400, 90, 250).level (-14).standardMacros (25).macro (4, "Detune")
            .mod (macro4, "osc1UnisonDetune", 40);
        add ("Sterile Undertow", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, 0, 13).unison (1, 4, 13, 60)
            .unison (2, 2, 11, 45).filter (1, ladder24, 650, 25, 45).lfo (1, drift, 0.24f, 0).mod (lfo1, "filter1Cutoff", 8).drive (fuzz, 20, 4500)
            .subOsc (35).mono().amp (6, 400, 90, 130).level (-18).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Sullen Reese", "Reese").osc (1, pulse, 40).osc (2, analog, sawPos, 75).pitch (2, -12, 10).unison (1, 3, 10, 40).unison (2, 1, 20, 65)
            .filter (1, ladder24, 750, 25, 20).lfo (1, sine, 0.15f, 0).mod (lfo1, "filter1Cutoff", 12).phaser (30, 0.1f, 45).subOsc (35).mono()
            .amp (4, 400, 90, 130).level (-3).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Sunken Vortex", "Reese").osc (1, growl, 35).osc (2, analog, sawPos, 65).pitch (2, 0, 8).unison (1, 4, 10, 60).unison (2, 1, 12, 60)
            .filter (1, svfLow, 1000, 30, 40).lfo (1, sine, 0.33f, 0).mod (lfo1, "filter1Cutoff", 10).phaser (40, 0.16f, 65).drive (tube, 25, 5000)
            .subOsc (35).mono().amp (5, 400, 90, 140).level (-13).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Swaying Helix", "Reese").osc (1, growl, 60).osc (2, analog, sawPos, 70).pitch (2, -12, 16).unison (1, 2, 25, 65).unison (2, 1, 15, 60)
            .filter (1, ladderBand, 1200, 40, 25).lfo (1, drift, 0.35f, 0).mod (lfo1, "filter1Cutoff", 6).subOsc (35).mono().amp (7, 400, 90, 170)
            .level (-6).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Twisted Tide", "Reese").osc (1, pulse, 50).osc (2, analog, sawPos, 60).pitch (2, 0, 14).unison (1, 3, 14, 55).unison (2, 3, 17, 60)
            .filter (1, ladderBand, 600, 25, 20).lfo (1, sine, 0.19f, 0).mod (lfo1, "filter1Cutoff", 10).phaser (35, 0.12f, 55).subOsc (30).mono()
            .amp (5, 400, 90, 140).level (-1).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Unstable Wire", "Reese").osc (1, sync, 30).osc (2, analog, sawPos, 70).pitch (2, 0, 7).unison (1, 2, 17, 45).unison (2, 3, 14, 65)
            .filter (1, ladderBand, 750, 40, 45).lfo (1, sine, 0.25f, 0).mod (lfo1, "filter1Cutoff", 11).subOsc (30).mono().amp (4, 400, 90, 150)
            .level (-6).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Vacant Helix", "Reese").osc (1, pulse, 35).osc (2, analog, sawPos, 65).pitch (2, 0, 9).unison (1, 3, 16, 40).unison (2, 3, 15, 65)
            .filter (1, ladder24, 550, 20, 25).lfo (1, drift, 0.11f, 0).mod (lfo1, "filter1Cutoff", 6).subOsc (30).mono().amp (7, 400, 90, 170)
            .level (-3).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Void Wire", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, 0, 7).unison (1, 3, 19, 65).unison (2, 1, 12, 40)
            .filter (1, ladder24, 600, 30, 35).lfo (1, sine, 0.24f, 0).mod (lfo1, "filter1Cutoff", 8).phaser (35, 0.24f, 65).subOsc (30).mono()
            .amp (3, 400, 90, 120).level (-5).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);
        add ("Welded Tide", "Reese").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, 0, 7).unison (1, 2, 16, 40).unison (2, 3, 19, 50)
            .filter (1, ladderBand, 1050, 20, 45).lfo (1, drift, 0.38f, 0).mod (lfo1, "filter1Cutoff", 8).drive (fuzz, 25, 6000).subOsc (35).mono()
            .amp (3, 400, 90, 220).level (-20).standardMacros (25).macro (4, "Detune").mod (macro4, "osc1UnisonDetune", 40);

        // Acid, by name.
        add ("Abandoned Drip", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 280, 81, 40, 30).env (2, 0, 280, 10, 120, 70)
            .mod (env2, "filter1Cutoff", 35).mod (velocity, "filter1Cutoff", 16).mono (40).amp (1, 270, 75, 60).drive (hard, 30, 5500).level (-12)
            .standardMacros (34).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Basalt Sting", "Acid").osc (1, analog, squarePos).filter (1, ladder12, 440, 67, 15, 30).env (2, 0, 130, 0, 120, 85)
            .mod (env2, "filter1Cutoff", 37).mod (velocity, "filter1Cutoff", 11).mono (70).amp (1, 160, 70, 60).lfoSync (1, steps, sixteenth, 0)
            .stepPattern (1, { -100, 60, -100, 100, 40, -100, -100, 100, -100, 60, -100, 100, -100, -100, 80, 100 }).mod (lfo1, "filter1Cutoff", 14)
            .delay (eighthT, 50, 23, 4500).level (-8).standardMacros (32).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Bitter Squeal", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 320, 78, 30, 30).env (2, 0, 240, 0, 120, 75)
            .mod (env2, "filter1Cutoff", 38).mod (velocity, "filter1Cutoff", 13).mono (80).amp (1, 270, 70, 60).drive (hard, 40, 4500).level (-15)
            .standardMacros (32).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Bleach Bite", "Acid").osc (1, analog, squarePos).filter (1, ladder12, 540, 81, 30, 30).env (2, 0, 250, 0, 120, 70)
            .mod (env2, "filter1Cutoff", 34).mod (velocity, "filter1Cutoff", 13).mono (60).amp (1, 220, 65, 60).drive (soft, 25, 6500).level (-12)
            .standardMacros (32).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Bleak Sting", "Acid").osc (1, analog, squarePos).filter (1, ladder24, 390, 67, 15, 30).env (2, 0, 280, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 31).mod (velocity, "filter1Cutoff", 17).mono (50).amp (1, 240, 65, 60).drive (tube, 40, 7000)
            .delay (sixteenthD, 30, 24, 5000).level (-13).standardMacros (34).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Brutal Hiss", "Acid").osc (1, analog, squarePos).filter (1, ladder24, 570, 69, 45, 30).env (2, 0, 190, 0, 120, 75)
            .mod (env2, "filter1Cutoff", 35).mod (velocity, "filter1Cutoff", 14).mono (80).amp (1, 260, 80, 60).drive (tube, 50, 4500)
            .lfoSync (1, steps, sixteenth, 0)
            .stepPattern (1, { -100, 100, -100, 100, 100, -100, 100, -100, -100, 100, 100, 100, -100, 100, -100, -100 })
            .mod (lfo1, "filter1Cutoff", 13).level (-15).standardMacros (34).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Brutal Rain", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 270, 73, 45, 30).env (2, 0, 210, 10, 120, 80)
            .mod (env2, "filter1Cutoff", 32).mod (velocity, "filter1Cutoff", 18).mono (80).amp (1, 180, 65, 60).delay (eighthT, 40, 24, 3500)
            .level (-4).standardMacros (31).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Buried Leak", "Acid").osc (1, analog, 80).filter (1, ladder24, 580, 71, 30, 30).env (2, 0, 250, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 41).mod (velocity, "filter1Cutoff", 11).mono (70).amp (1, 170, 70, 60).drive (hard, 25, 6500)
            .lfoSync (1, steps, sixteenth, 0).stepPattern (1, { -50, 0, 100, 0, -100, 0, 100, 0, -100, 0, 100, 0, -100, 0, 50, 0 })
            .mod (lfo1, "filter1Cutoff", 13).level (-10).standardMacros (32).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Carbon Burn", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 570, 77, 10, 30).env (2, 0, 230, 0, 120, 75)
            .mod (env2, "filter1Cutoff", 37).mod (velocity, "filter1Cutoff", 16).mono (60).amp (1, 270, 65, 60).delay (sixteenthD, 30, 15, 4000)
            .level (-4).standardMacros (33).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Carbon Rain", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 270, 75, 15, 30).env (2, 0, 270, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 31).mod (velocity, "filter1Cutoff", 11).mono (70).amp (1, 180, 70, 60).drive (soft, 35, 6500)
            .lfoSync (1, steps, sixteenth, 0)
            .stepPattern (1, { -100, -100, 60, -100, -100, -100, 100, -100, -100, 60, -100, -100, 60, -100, 100, -100 })
            .mod (lfo1, "filter1Cutoff", 9).delay (eighthT, 45, 24, 3000).level (-13).standardMacros (30).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 20);
        add ("Carbon Spill", "Acid").osc (1, analog, 80).filter (1, ladder12, 420, 86, 25, 30).env (2, 0, 230, 10, 120, 85)
            .mod (env2, "filter1Cutoff", 44).mod (velocity, "filter1Cutoff", 14).mono (60).amp (1, 210, 70, 60).drive (tube, 50, 6000).level (-18)
            .standardMacros (30).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Choked Spill", "Acid").osc (1, analog, 80).filter (1, ladder12, 240, 67, 35, 30).env (2, 0, 190, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 36).mod (velocity, "filter1Cutoff", 11).mono (30).amp (1, 170, 70, 60).lfoSync (1, steps, sixteenth, 0)
            .stepPattern (1, { 0, 50, 0, -50, 0, 100, 0, -100, 0, 100, 0, -100, 0, 100, 0, -100 }).mod (lfo1, "filter1Cutoff", 14).level (-7)
            .standardMacros (33).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Choked Squeal", "Acid").osc (1, analog, 80).filter (1, ladder12, 510, 87, 35, 30).env (2, 0, 220, 10, 120, 65)
            .mod (env2, "filter1Cutoff", 41).mod (velocity, "filter1Cutoff", 13).mono (60).amp (1, 270, 70, 60).drive (soft, 40, 4000).level (-16)
            .standardMacros (35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Cinder Etch", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 550, 76, 35, 30).env (2, 0, 180, 0, 120, 70)
            .mod (env2, "filter1Cutoff", 43).mod (velocity, "filter1Cutoff", 12).mono (70).amp (1, 280, 60, 60).drive (tube, 40, 6500)
            .delay (eighthD, 45, 21, 4500).level (-13).standardMacros (32).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Cinder Vat", "Acid").osc (1, analog, squarePos).filter (1, ladder24, 270, 67, 15, 30).env (2, 0, 130, 10, 120, 70)
            .mod (env2, "filter1Cutoff", 31).mod (velocity, "filter1Cutoff", 17).mono (60).amp (1, 260, 70, 60).level (-5).standardMacros (32)
            .macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Concrete Bath", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 580, 80, 15, 30).env (2, 0, 180, 0, 120, 70)
            .mod (env2, "filter1Cutoff", 45).mod (velocity, "filter1Cutoff", 14).mono (70).amp (1, 280, 75, 60).lfoSync (1, steps, sixteenth, 0)
            .stepPattern (1, { 100, -100, -100, 80, 100, -100, 60, -100, 100, 40, -100, -100, 100, -100, 60, -100 }).mod (lfo1, "filter1Cutoff", 9)
            .level (-5).standardMacros (34).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Corrosive Burn", "Acid").osc (1, analog, squarePos).filter (1, ladder24, 500, 73, 15, 30).env (2, 0, 210, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 34).mod (velocity, "filter1Cutoff", 16).mono (60).amp (1, 240, 75, 60).lfoSync (1, steps, sixteenth, 0)
            .stepPattern (1, { -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100 })
            .mod (lfo1, "filter1Cutoff", 11).delay (sixteenthD, 40, 18, 3500).level (-5).standardMacros (33).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 20);
        add ("Corrosive Loop", "Acid").osc (1, analog, squarePos).filter (1, ladder24, 450, 87, 45, 30).env (2, 0, 300, 10, 120, 75)
            .mod (env2, "filter1Cutoff", 35).mod (velocity, "filter1Cutoff", 11).mono (40).amp (1, 280, 70, 60).drive (fuzz, 30, 6000)
            .delay (sixteenthD, 45, 20, 2500).level (-21).standardMacros (31).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Derelict Burn", "Acid").osc (1, analog, squarePos).filter (1, ladder24, 450, 79, 30, 30).env (2, 0, 160, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 32).mod (velocity, "filter1Cutoff", 10).mono (60).amp (1, 260, 70, 60).drive (soft, 45, 5000)
            .delay (sixteenthD, 45, 16, 3000).level (-16).standardMacros (33).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Derelict Drip", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 300, 72, 15, 30).env (2, 0, 220, 10, 120, 65)
            .mod (env2, "filter1Cutoff", 45).mod (velocity, "filter1Cutoff", 14).mono (80).amp (1, 260, 75, 60).drive (fuzz, 40, 5000).level (-21)
            .standardMacros (35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Derelict Line", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 370, 75, 35, 30).env (2, 0, 140, 10, 120, 85)
            .mod (env2, "filter1Cutoff", 33).mod (velocity, "filter1Cutoff", 17).mono (50).amp (1, 210, 75, 60).drive (soft, 35, 6500)
            .lfoSync (1, steps, sixteenth, 0).stepPattern (1, { 100, -100, 50, 100, -100, 50, -100, -100, 100, 25, -100, 100, -100, 50, -100, -100 })
            .mod (lfo1, "filter1Cutoff", 14).level (-13).standardMacros (30).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Etched Vat", "Acid").osc (1, analog, 80).filter (1, ladder24, 600, 72, 20, 30).env (2, 0, 160, 10, 120, 85)
            .mod (env2, "filter1Cutoff", 32).mod (velocity, "filter1Cutoff", 13).mono (60).amp (1, 190, 55, 60).drive (fuzz, 40, 4500)
            .lfoSync (1, steps, sixteenth, 0).stepPattern (1, { 100, 40, -100, -100, 100, -100, 60, -100, 100, -100, -100, 80, 100, -100, 60, -100 })
            .mod (lfo1, "filter1Cutoff", 9).delay (eighthD, 35, 14, 3500).level (-21).standardMacros (33).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 20);
        add ("Feral Bite", "Acid").osc (1, analog, 80).filter (1, ladder24, 360, 68, 20, 30).env (2, 0, 300, 0, 120, 75)
            .mod (env2, "filter1Cutoff", 43).mod (velocity, "filter1Cutoff", 11).mono (80).amp (1, 250, 55, 60).drive (fuzz, 35, 4000)
            .lfoSync (1, steps, sixteenth, 0).stepPattern (1, { -100, 100, -100, 50, -100, -100, 100, -100, 50, 100, -100, 50, -100, -100, 100, 25 })
            .mod (lfo1, "filter1Cutoff", 12).delay (eighthD, 45, 22, 4000).level (-21).standardMacros (34).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 20);
        add ("Feral Fizz", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 410, 66, 30, 30).env (2, 0, 310, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 31).mod (velocity, "filter1Cutoff", 11).mono (70).amp (1, 190, 55, 60).drive (tube, 30, 5000)
            .delay (eighthD, 50, 21, 4000).level (-10).standardMacros (32).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Grey Bath", "Acid").osc (1, analog, sawPos).filter (1, ladder12, 360, 81, 40, 30).env (2, 0, 300, 10, 120, 65)
            .mod (env2, "filter1Cutoff", 38).mod (velocity, "filter1Cutoff", 10).mono (90).amp (1, 180, 80, 60).delay (eighthD, 35, 19, 3500)
            .level (-7).standardMacros (30).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Grey Wail", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 450, 70, 40, 30).env (2, 0, 310, 0, 120, 75)
            .mod (env2, "filter1Cutoff", 35).mod (velocity, "filter1Cutoff", 15).mono (60).amp (1, 290, 65, 60).level (-4).standardMacros (30)
            .macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Hollow Bath", "Acid").osc (1, analog, sawPos).filter (1, ladder12, 310, 76, 30, 30).env (2, 0, 190, 0, 120, 75)
            .mod (env2, "filter1Cutoff", 38).mod (velocity, "filter1Cutoff", 16).mono (30).amp (1, 280, 50, 60).drive (hard, 25, 5500)
            .lfoSync (1, steps, sixteenth, 0)
            .stepPattern (1, { -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100 })
            .mod (lfo1, "filter1Cutoff", 8).level (-14).standardMacros (30).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Hollow Rain", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 260, 79, 30, 30).env (2, 0, 160, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 33).mod (velocity, "filter1Cutoff", 12).mono (90).amp (1, 280, 75, 60).level (-5).standardMacros (31)
            .macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Industrial Loop", "Acid").osc (1, analog, 80).filter (1, ladder24, 310, 68, 15, 30).env (2, 0, 280, 0, 120, 70)
            .mod (env2, "filter1Cutoff", 35).mod (velocity, "filter1Cutoff", 14).mono (90).amp (1, 150, 75, 60).delay (eighthT, 40, 15, 3500)
            .level (-5).standardMacros (35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Iron Etch", "Acid").osc (1, analog, sawPos).filter (1, ladder12, 430, 79, 45, 30).env (2, 0, 260, 0, 120, 65)
            .mod (env2, "filter1Cutoff", 38).mod (velocity, "filter1Cutoff", 11).mono (40).amp (1, 220, 80, 60).drive (soft, 20, 5000).level (-10)
            .standardMacros (34).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Iron Sting", "Acid").osc (1, analog, squarePos).filter (1, ladder24, 440, 66, 15, 30).env (2, 0, 270, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 32).mod (velocity, "filter1Cutoff", 11).mono (50).amp (1, 210, 75, 60).level (-5).standardMacros (33)
            .macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Leaden Bath", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 490, 83, 45, 30).env (2, 0, 250, 10, 120, 80)
            .mod (env2, "filter1Cutoff", 42).mod (velocity, "filter1Cutoff", 17).mono (60).amp (1, 220, 60, 60).drive (hard, 40, 5000)
            .delay (eighthT, 45, 16, 3000).level (-13).standardMacros (33).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Leaking Bite", "Acid").osc (1, analog, squarePos).filter (1, ladder12, 450, 70, 35, 30).env (2, 0, 270, 10, 120, 75)
            .mod (env2, "filter1Cutoff", 37).mod (velocity, "filter1Cutoff", 13).mono (70).amp (1, 210, 70, 60).drive (fuzz, 25, 6500)
            .delay (sixteenthD, 45, 20, 3000).level (-22).standardMacros (35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Leaking Burn", "Acid").osc (1, analog, 80).filter (1, ladder24, 220, 80, 25, 30).env (2, 0, 200, 0, 120, 65)
            .mod (env2, "filter1Cutoff", 32).mod (velocity, "filter1Cutoff", 13).mono (30).amp (1, 270, 60, 60).drive (soft, 25, 5000)
            .lfoSync (1, steps, sixteenth, 0).stepPattern (1, { 60, -100, 100, -100, -100, 80, 100, -100, 60, -100, 100, 40, -100, -100, 100, -100 })
            .mod (lfo1, "filter1Cutoff", 9).delay (sixteenthD, 50, 20, 3000).level (-10).standardMacros (30).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 20);
        add ("Leaking Smear", "Acid").osc (1, analog, 80).filter (1, ladder24, 530, 81, 30, 30).env (2, 0, 190, 10, 120, 80)
            .mod (env2, "filter1Cutoff", 33).mod (velocity, "filter1Cutoff", 17).mono (40).amp (1, 290, 60, 60).lfoSync (1, steps, sixteenth, 0)
            .stepPattern (1, { 0, -50, 0, 100, 0, -100, 0, 100, 0, -100, 0, 100, 0, -100, 0, 50 }).mod (lfo1, "filter1Cutoff", 14).level (-5)
            .standardMacros (34).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Nickel Sizzle", "Acid").osc (1, analog, squarePos).filter (1, ladder24, 210, 77, 45, 30).env (2, 0, 300, 10, 120, 80)
            .mod (env2, "filter1Cutoff", 36).mod (velocity, "filter1Cutoff", 17).mono (30).amp (1, 190, 55, 60).lfoSync (1, steps, sixteenth, 0)
            .stepPattern (1, { 60, -10, 20, -90, 100, -40, 30, -60, 80, -20, 0, -80, 100, -30, 40, -70 }).mod (lfo1, "filter1Cutoff", 9)
            .delay (sixteenthD, 30, 25, 3500).level (-4).standardMacros (35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Nickel Wail", "Acid").osc (1, analog, 80).filter (1, ladder24, 300, 82, 40, 30).env (2, 0, 260, 0, 120, 75)
            .mod (env2, "filter1Cutoff", 38).mod (velocity, "filter1Cutoff", 11).mono (30).amp (1, 200, 55, 60).drive (hard, 35, 5000)
            .delay (eighthD, 30, 18, 4000).level (-13).standardMacros (33).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Night Drip", "Acid").osc (1, analog, sawPos).filter (1, ladder12, 450, 69, 35, 30).env (2, 0, 180, 10, 120, 80)
            .mod (env2, "filter1Cutoff", 39).mod (velocity, "filter1Cutoff", 18).mono (60).amp (1, 210, 70, 60).level (-7).standardMacros (33)
            .macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Night Squeal", "Acid").osc (1, analog, sawPos).filter (1, ladder12, 400, 77, 10, 30).env (2, 0, 240, 10, 120, 80)
            .mod (env2, "filter1Cutoff", 36).mod (velocity, "filter1Cutoff", 18).mono (70).amp (1, 280, 55, 60).delay (eighthD, 45, 21, 3500)
            .level (-7).standardMacros (34).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Null Line", "Acid").osc (1, analog, squarePos).filter (1, ladder24, 320, 87, 15, 30).env (2, 0, 140, 0, 120, 70)
            .mod (env2, "filter1Cutoff", 35).mod (velocity, "filter1Cutoff", 17).mono (50).amp (1, 230, 65, 60).drive (hard, 45, 6000)
            .lfoSync (1, steps, sixteenth, 0).stepPattern (1, { -50, 0, 100, 0, -100, 0, 100, 0, -100, 0, 100, 0, -100, 0, 50, 0 })
            .mod (lfo1, "filter1Cutoff", 11).level (-18).standardMacros (35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Obsidian Spill", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 270, 71, 35, 30).env (2, 0, 280, 0, 120, 70)
            .mod (env2, "filter1Cutoff", 37).mod (velocity, "filter1Cutoff", 16).mono (40).amp (1, 250, 75, 60).drive (tube, 40, 6500)
            .delay (sixteenthD, 50, 17, 4000).level (-13).standardMacros (33).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Pressed Leak", "Acid").osc (1, analog, 80).filter (1, ladder24, 420, 74, 30, 30).env (2, 0, 210, 10, 120, 70)
            .mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 15).mono (60).amp (1, 240, 60, 60).drive (soft, 30, 5500)
            .delay (sixteenthD, 35, 23, 5000).level (-10).standardMacros (32).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Riveted Drip", "Acid").osc (1, analog, squarePos).filter (1, ladder24, 410, 82, 25, 30).env (2, 0, 240, 10, 120, 70)
            .mod (env2, "filter1Cutoff", 37).mod (velocity, "filter1Cutoff", 14).mono (30).amp (1, 300, 70, 60).drive (hard, 45, 6500)
            .delay (sixteenthD, 40, 19, 3000).level (-17).standardMacros (33).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Riveted Squelch", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 240, 77, 20, 30).env (2, 0, 140, 10, 120, 75)
            .mod (env2, "filter1Cutoff", 36).mod (velocity, "filter1Cutoff", 14).mono (60).amp (1, 270, 65, 60).delay (eighthD, 45, 18, 4000)
            .level (-5).standardMacros (31).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Rotten Gnaw", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 260, 66, 15, 30).env (2, 0, 140, 0, 120, 85)
            .mod (env2, "filter1Cutoff", 37).mod (velocity, "filter1Cutoff", 17).mono (50).amp (1, 240, 65, 60).drive (fuzz, 30, 6000)
            .delay (eighthT, 30, 22, 4000).level (-20).standardMacros (30).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Rust Hiss", "Acid").osc (1, analog, sawPos).filter (1, ladder12, 540, 80, 40, 30).env (2, 0, 260, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 44).mod (velocity, "filter1Cutoff", 11).mono (50).amp (1, 280, 75, 60).drive (soft, 45, 7000).level (-17)
            .standardMacros (33).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Scorched Hiss", "Acid").osc (1, analog, 80).filter (1, ladder12, 260, 70, 10, 30).env (2, 0, 230, 10, 120, 85)
            .mod (env2, "filter1Cutoff", 32).mod (velocity, "filter1Cutoff", 17).mono (40).amp (1, 270, 70, 60).delay (sixteenthD, 35, 22, 4500)
            .level (-7).standardMacros (32).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Severed Drip", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 560, 80, 15, 30).env (2, 0, 210, 0, 120, 85)
            .mod (env2, "filter1Cutoff", 35).mod (velocity, "filter1Cutoff", 12).mono (30).amp (1, 160, 50, 60).drive (soft, 35, 5500)
            .lfoSync (1, steps, sixteenth, 0).stepPattern (1, { -100, 50, 100, -100, 50, -100, -100, 100, 25, -100, 100, -100, 50, -100, -100, 100 })
            .mod (lfo1, "filter1Cutoff", 9).delay (eighthT, 30, 13, 4000).level (-12).standardMacros (31).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 20);
        add ("Severed Gnaw", "Acid").osc (1, analog, 80).filter (1, ladder24, 520, 75, 40, 30).env (2, 0, 310, 10, 120, 75)
            .mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 12).mono (60).amp (1, 210, 60, 60).level (-3).standardMacros (30)
            .macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Severed Squeal", "Acid").osc (1, analog, sawPos).filter (1, ladder12, 390, 83, 30, 30).env (2, 0, 260, 10, 120, 70)
            .mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 14).mono (60).amp (1, 240, 50, 60).drive (tube, 40, 6500).level (-16)
            .standardMacros (35).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Sodium Smear", "Acid").osc (1, analog, sawPos).filter (1, ladder12, 290, 86, 25, 30).env (2, 0, 170, 0, 120, 85)
            .mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 11).mono (80).amp (1, 190, 55, 60).lfoSync (1, steps, sixteenth, 0)
            .stepPattern (1, { -100, 100, -100, -100, 100, -100, -100, 100, -100, 100, -100, -100, 100, -100, 100, -100 })
            .mod (lfo1, "filter1Cutoff", 10).delay (sixteenthD, 45, 14, 3500).level (-8).standardMacros (30).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 20);
        add ("Soot Drip", "Acid").osc (1, analog, 80).filter (1, ladder24, 310, 81, 30, 30).env (2, 0, 160, 0, 120, 70)
            .mod (env2, "filter1Cutoff", 36).mod (velocity, "filter1Cutoff", 11).mono (50).amp (1, 150, 70, 60).level (-5).standardMacros (33)
            .macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Static Etch", "Acid").osc (1, analog, sawPos).filter (1, ladder12, 540, 80, 35, 30).env (2, 0, 300, 0, 120, 70)
            .mod (env2, "filter1Cutoff", 33).mod (velocity, "filter1Cutoff", 16).mono (40).amp (1, 250, 65, 60).drive (fuzz, 40, 5500).level (-21)
            .standardMacros (34).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Sterile Loop", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 590, 77, 15, 30).env (2, 0, 150, 0, 120, 75)
            .mod (env2, "filter1Cutoff", 38).mod (velocity, "filter1Cutoff", 12).mono (50).amp (1, 160, 60, 60).drive (soft, 35, 5500).level (-12)
            .standardMacros (31).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Sullen Bath", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 240, 73, 25, 30).env (2, 0, 130, 10, 120, 70)
            .mod (env2, "filter1Cutoff", 36).mod (velocity, "filter1Cutoff", 14).mono (40).amp (1, 190, 65, 60).drive (tube, 40, 6000)
            .delay (sixteenthD, 35, 20, 4500).level (-13).standardMacros (34).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Sullen Smear", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 250, 80, 25, 30).env (2, 0, 310, 10, 120, 75)
            .mod (env2, "filter1Cutoff", 36).mod (velocity, "filter1Cutoff", 16).mono (60).amp (1, 190, 65, 60).drive (fuzz, 25, 4000)
            .lfoSync (1, steps, sixteenth, 0)
            .stepPattern (1, { 100, -100, -100, 100, -100, -100, 100, -100, 100, -100, -100, 100, -100, 100, -100, -100 })
            .mod (lfo1, "filter1Cutoff", 12).delay (eighthD, 40, 23, 3000).level (-20).standardMacros (30).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 20);
        add ("Tar Sting", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 320, 78, 45, 30).env (2, 0, 150, 0, 120, 70)
            .mod (env2, "filter1Cutoff", 42).mod (velocity, "filter1Cutoff", 18).mono (80).amp (1, 220, 65, 60).drive (fuzz, 40, 5500).level (-21)
            .standardMacros (32).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);
        add ("Venom Bath", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 580, 86, 45, 30).env (2, 0, 250, 10, 120, 70)
            .mod (env2, "filter1Cutoff", 43).mod (velocity, "filter1Cutoff", 14).mono (70).amp (1, 280, 50, 60).drive (hard, 30, 4000)
            .lfoSync (1, steps, sixteenth, 0).stepPattern (1, { 70, 40, 10, -20, -50, -80, -100, 100, 70, 40, 10, -20, -50, -80, -100, 100 })
            .mod (lfo1, "filter1Cutoff", 9).delay (sixteenthD, 35, 15, 4000).level (-10).standardMacros (32).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 20);
        add ("Venom Sizzle", "Acid").osc (1, analog, squarePos).filter (1, ladder12, 490, 83, 30, 30).env (2, 0, 190, 10, 120, 85)
            .mod (env2, "filter1Cutoff", 39).mod (velocity, "filter1Cutoff", 10).mono (30).amp (1, 230, 60, 60).drive (soft, 30, 6500)
            .lfoSync (1, steps, sixteenth, 0)
            .stepPattern (1, { -100, 60, -100, -100, -100, 100, -100, -100, 60, -100, -100, 60, -100, 100, -100, -100 })
            .mod (lfo1, "filter1Cutoff", 12).delay (eighthT, 45, 21, 4000).level (-14).standardMacros (30).macro (4, "Resonance")
            .mod (macro4, "filter1Resonance", 20);
        add ("Void Squeal", "Acid").osc (1, analog, sawPos).filter (1, ladder24, 560, 79, 45, 30).env (2, 0, 190, 10, 120, 80)
            .mod (env2, "filter1Cutoff", 31).mod (velocity, "filter1Cutoff", 16).mono (80).amp (1, 270, 60, 60).delay (eighthD, 35, 24, 3000)
            .level (-4).standardMacros (32).macro (4, "Resonance").mod (macro4, "filter1Resonance", 20);

        // Lead, by name.
        add ("Abandoned Cutter", "Lead").osc (1, analog, 20).noise (14, 1).filter (1, ladderBand, 640, 75, 0, 100).lfo (1, sine, 4.7f, 1)
            .lfoFade (1, 850).mod (lfo1, "osc1Pitch", 0.5f).mono (130).amp (27, 300, 90, 300).reverb (30, 75, 4, 45).level (6).standardMacros (20);
        add ("Acid Signal", "Lead").osc (1, pulse, 50).unison (1, 4, 14, 65).filter (1, ladder12, 1900, 35).env (2, 0, 220, 30, 200, 60)
            .mod (env2, "filter1Cutoff", 20).lfo (1, sine, 5.5f, 1).lfoFade (1, 550).mod (lfo1, "osc1Pitch", 0.6f).mono (70).amp (8, 400, 80, 290)
            .drive (crush, 35, 5500).delay (quarterT, 35, 24, 4000).level (-5).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Bitter Arc", "Lead").osc (1, pulse, 30).unison (1, 2, 9, 45).filter (1, ladder24, 4700, 35).env (2, 0, 380, 35, 200, 60)
            .mod (env2, "filter1Cutoff", 19).lfo (1, sine, 5.8f, 1).lfoFade (1, 400).mod (lfo1, "osc1Pitch", 0.5f).mono (70).amp (2, 400, 80, 290)
            .drive (tube, 35, 6500).delay (eighth, 30, 19, 4000).level (-12).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Bitter Lead", "Lead").osc (1, analog, sawPos).unison (1, 4, 15, 60).osc (2, analog, squarePos, 55).pitch (2, 12, 5)
            .filter (1, ladder12, 2100, 15).env (2, 0, 280, 35, 200, 60).mod (env2, "filter1Cutoff", 18).lfo (1, sine, 5, 1).lfoFade (1, 350)
            .mod (lfo1, "osc1Pitch", 0.6f).mono (80).amp (4, 400, 80, 250).delay (quarterT, 45, 21, 4000).level (-6).standardMacros (25)
            .macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Black Blade", "Lead").osc (1, vowel, 35).lfoSync (1, sine, half, 1).mod (lfo1, "osc1Position", 25).filter (1, svfBand, 3000, 30)
            .drive (fuzz, 35).mono (90).amp (8, 400, 85, 200).level (-20).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Bleak Filament", "Lead").osc (1, vowel, 70).lfoSync (1, sine, quarter, 1).mod (lfo1, "osc1Position", 30).filter (1, ladder12, 2900, 40)
            .drive (hard, 40).mono (60).amp (7, 400, 85, 200).reverb (15, 70, 3, 45).level (-16).macro (1, "Vowel").mod (macro1, "osc1Position", 60)
            .macro (2, "Grit").mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Brutal Arc", "Lead").osc (1, analog, sawPos).unison (1, 1, 12, 55).filter (1, ladder24, 2800, 35).env (2, 0, 250, 40, 200, 60)
            .mod (env2, "filter1Cutoff", 17).lfo (1, sine, 5.8f, 1).lfoFade (1, 300).mod (lfo1, "osc1Pitch", 0.5f).mono (80).amp (3, 400, 80, 300)
            .delay (eighth, 35, 13, 4000).level (-3).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Brutal Cry", "Lead").osc (1, analog, 20).noise (6, 1).filter (1, ladderBand, 600, 70, 0, 100).lfo (1, sine, 5.5f, 1).lfoFade (1, 500)
            .mod (lfo1, "osc1Pitch", 0.7f).mono (100).amp (25, 300, 90, 300).reverb (35, 75, 4, 45).level (6).standardMacros (20);
        add ("Burning Spark", "Lead").osc (1, analog, trianglePos).noise (9, 1).filter (1, svfBand, 500, 65, 0, 100).lfo (1, sine, 4.4f, 1)
            .lfoFade (1, 650).mod (lfo1, "osc1Pitch", 0.6f).mono (120).amp (25, 300, 90, 300).reverb (35, 75, 4, 45).level (4).standardMacros (20);
        add ("Burnt Flare", "Lead").osc (1, pulse, 25).unison (1, 1, 9, 40).osc (2, pulse, squarePos, 55).pitch (2, -12, 4)
            .filter (1, svfLow, 4800, 20).env (2, 0, 280, 30, 200, 60).mod (env2, "filter1Cutoff", 16).lfo (1, sine, 5.7f, 1).lfoFade (1, 350)
            .mod (lfo1, "osc1Pitch", 0.4f).mono (80).amp (2, 400, 80, 190).drive (tube, 55, 8000).delay (eighthD, 30, 21, 4000).level (-18)
            .standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Carbon Cutter", "Lead").osc (1, analog, sinePos).noise (15, 1).filter (1, svfBand, 530, 70, 0, 100).lfo (1, sine, 4.7f, 1)
            .lfoFade (1, 800).mod (lfo1, "osc1Pitch", 0.8f).mono (90).amp (20, 300, 90, 300).reverb (25, 75, 4, 45).level (5).standardMacros (20);
        add ("Carbon Spark", "Lead").osc (1, vowel, 65).lfoSync (1, triangle, bar1, 1).mod (lfo1, "osc1Position", 20).filter (1, svfBand, 2800, 35)
            .drive (tube, 35).mono (50).amp (11, 400, 85, 200).level (-10).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Choked Beam", "Lead").osc (1, analog, sawPos).unison (1, 2, 12, 50).osc (2, pulse, sawPos, 50).pitch (2, 0, 4)
            .filter (1, ladder24, 2600, 20).env (2, 0, 300, 40, 200, 60).mod (env2, "filter1Cutoff", 16).lfo (1, sine, 5.8f, 1).lfoFade (1, 500)
            .mod (lfo1, "osc1Pitch", 0.5f).mono (80).amp (5, 400, 80, 270).delay (quarterT, 25, 12, 4000).level (-4).standardMacros (25)
            .macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Chrome Blade", "Lead").osc (1, vowel, 50).lfoSync (1, triangle, quarter, 1).mod (lfo1, "osc1Position", 25).filter (1, svfBand, 3900, 20)
            .drive (fuzz, 25).mono (50).amp (10, 400, 85, 200).reverb (15, 70, 3, 45).level (-17).macro (1, "Vowel").mod (macro1, "osc1Position", 60)
            .macro (2, "Grit").mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Cold Blade", "Lead").osc (1, growl, 35).unison (1, 4, 8, 55).filter (1, ladder12, 4700, 30).env (2, 0, 430, 30, 200, 60)
            .mod (env2, "filter1Cutoff", 13).lfo (1, sine, 4.7f, 1).lfoFade (1, 600).mod (lfo1, "osc1Pitch", 0.6f).mono (80).amp (7, 400, 80, 160)
            .delay (quarterT, 35, 23, 4000).level (-3).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Cold Shriek", "Lead").osc (1, analog, sawPos).unison (1, 2, 8, 60).osc (2, pulse, squarePos, 65).pitch (2, 12, 4)
            .filter (1, ladder24, 2500, 30).env (2, 0, 280, 20, 200, 60).mod (env2, "filter1Cutoff", 17).lfo (1, sine, 4.5f, 1).lfoFade (1, 350)
            .mod (lfo1, "osc1Pitch", 0.5f).mono (50).amp (6, 400, 80, 260).drive (crush, 35, 5000).delay (eighthD, 40, 20, 4000).level (-3)
            .standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Concrete Probe", "Lead").osc (1, additive, 15).lfoSync (1, sawUp, quarter, 1).mod (lfo1, "osc1Position", 30)
            .filter (1, svfLow, 3300, 35).drive (fuzz, 30).mono (70).amp (8, 400, 85, 200).level (-20).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 60).macro (2, "Grit").mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Concrete Saw", "Lead").osc (1, growl, 30).unison (1, 4, 15, 70).osc (2, analog, squarePos, 70).pitch (2, 12, 9)
            .filter (1, ladder12, 3100, 30).env (2, 0, 390, 30, 200, 60).mod (env2, "filter1Cutoff", 17).lfo (1, sine, 4.7f, 1).lfoFade (1, 650)
            .mod (lfo1, "osc1Pitch", 0.6f).mono (40).amp (3, 400, 80, 180).drive (crush, 25, 7000).delay (eighth, 30, 14, 4000).level (-7)
            .standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Concrete Voice", "Lead").osc (1, analog, sawPos).unison (1, 1, 14, 50).filter (1, svfLow, 2500, 35).env (2, 0, 470, 30, 200, 60)
            .mod (env2, "filter1Cutoff", 10).lfo (1, sine, 5.7f, 1).lfoFade (1, 650).mod (lfo1, "osc1Pitch", 0.6f).mono (40).amp (3, 400, 80, 240)
            .delay (eighth, 30, 15, 4000).level (-4).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Concrete Wire", "Lead").osc (1, sync, 50).unison (1, 1, 11, 50).filter (1, ladder12, 1800, 40).env (2, 0, 240, 35, 200, 60)
            .mod (env2, "filter1Cutoff", 15).lfo (1, sine, 5.7f, 1).lfoFade (1, 550).mod (lfo1, "osc1Pitch", 0.5f).mono (80).amp (6, 400, 80, 160)
            .drive (tube, 30, 6000).delay (eighthD, 45, 22, 4000).level (-10).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Dead Arc", "Lead").osc (1, additive, 35).lfoSync (1, sawUp, quarter, 1).mod (lfo1, "osc1Position", 25).filter (1, svfBand, 1900, 25)
            .drive (tube, 30).mono (90).amp (3, 400, 85, 200).reverb (25, 70, 3, 45).level (-10).macro (1, "Vowel").mod (macro1, "osc1Position", 60)
            .macro (2, "Grit").mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Dead Signal", "Lead").osc (1, analog, sawPos).unison (1, 3, 10, 65).osc (2, pulse, sawPos, 50).pitch (2, 12, 4)
            .filter (1, ladder24, 2100, 25).env (2, 0, 220, 30, 200, 60).mod (env2, "filter1Cutoff", 15).lfo (1, sine, 4.8f, 1).lfoFade (1, 350)
            .mod (lfo1, "osc1Pitch", 0.5f).mono (100).amp (7, 400, 80, 290).delay (eighth, 25, 22, 4000).level (-4).standardMacros (25)
            .macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Electric Blade", "Lead").osc (1, vowel, 65).lfoSync (1, sine, half, 1).mod (lfo1, "osc1Position", 30).filter (1, svfLow, 2000, 50)
            .drive (tube, 40).mono (70).amp (6, 400, 85, 200).level (-13).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Electric Torch", "Lead").osc (1, analog, sawPos).unison (1, 1, 9, 70).filter (1, ladder12, 4700, 20).env (2, 0, 420, 25, 200, 60)
            .mod (env2, "filter1Cutoff", 15).lfo (1, sine, 5.3f, 1).lfoFade (1, 400).mod (lfo1, "osc1Pitch", 0.5f).mono (60).amp (2, 400, 80, 150)
            .delay (eighth, 35, 18, 4000).level (-3).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Feral Torch", "Lead").osc (1, analog, sawPos).unison (1, 2, 8, 70).filter (1, svfLow, 3800, 35).env (2, 0, 380, 30, 200, 60)
            .mod (env2, "filter1Cutoff", 12).lfo (1, sine, 5.2f, 1).lfoFade (1, 350).mod (lfo1, "osc1Pitch", 0.5f).mono (70).amp (6, 400, 80, 240)
            .drive (tube, 25, 8000).delay (eighthD, 40, 15, 4000).level (-10).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Frozen Cry", "Lead").osc (1, analog, sawPos).unison (1, 1, 11, 55).osc (2, pulse, squarePos, 55).pitch (2, 12, 4)
            .filter (1, ladder12, 4800, 30).env (2, 0, 480, 25, 200, 60).mod (env2, "filter1Cutoff", 12).lfo (1, sine, 4.7f, 1).lfoFade (1, 550)
            .mod (lfo1, "osc1Pitch", 0.6f).mono (50).amp (3, 400, 80, 270).delay (quarterT, 30, 20, 4000).level (-5).standardMacros (25)
            .macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Frozen Cutter", "Lead").osc (1, sync, 60).unison (1, 1, 7, 45).filter (1, ladder12, 2300, 35).env (2, 0, 200, 25, 200, 60)
            .mod (env2, "filter1Cutoff", 17).lfo (1, sine, 4.8f, 1).lfoFade (1, 300).mod (lfo1, "osc1Pitch", 0.4f).mono (60).amp (6, 400, 80, 270)
            .drive (tube, 50, 7500).delay (quarterT, 30, 24, 4000).level (-16).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Frozen Probe", "Lead").osc (1, additive, 30).lfoSync (1, triangle, half, 1).mod (lfo1, "osc1Position", 35).filter (1, svfBand, 2100, 30)
            .drive (fuzz, 35).mono (60).amp (6, 400, 85, 200).reverb (25, 70, 3, 45).level (-21).macro (1, "Vowel").mod (macro1, "osc1Position", 60)
            .macro (2, "Grit").mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Grey Probe", "Lead").osc (1, analog, sinePos).noise (7, 1).filter (1, svfBand, 700, 65, 0, 100).lfo (1, sine, 4.6f, 1).lfoFade (1, 450)
            .mod (lfo1, "osc1Pitch", 0.5f).mono (120).amp (24, 300, 90, 300).reverb (25, 75, 4, 45).level (6).standardMacros (20);
        add ("Grey Siren", "Lead").osc (1, sync, 25).unison (1, 1, 13, 55).filter (1, ladder12, 4400, 30).env (2, 0, 280, 25, 200, 60)
            .mod (env2, "filter1Cutoff", 11).lfo (1, sine, 5.2f, 1).lfoFade (1, 600).mod (lfo1, "osc1Pitch", 0.6f).mono (70).amp (3, 400, 80, 270)
            .drive (fuzz, 25, 6000).delay (eighth, 40, 12, 4000).level (-17).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Heavy Alarm", "Lead").osc (1, analog, sawPos).unison (1, 3, 11, 50).filter (1, svfLow, 4800, 35).env (2, 0, 330, 35, 200, 60)
            .mod (env2, "filter1Cutoff", 10).lfo (1, sine, 5.2f, 1).lfoFade (1, 400).mod (lfo1, "osc1Pitch", 0.6f).mono (40).amp (5, 400, 80, 210)
            .drive (fuzz, 45, 6000).delay (eighth, 35, 23, 4000).level (-24).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Hollow Alarm", "Lead").osc (1, analog, trianglePos).noise (11, 1).filter (1, ladderBand, 800, 80, 0, 100).lfo (1, sine, 4.1f, 1)
            .lfoFade (1, 450).mod (lfo1, "osc1Pitch", 0.7f).mono (90).amp (24, 300, 90, 300).reverb (35, 75, 4, 45).level (3).standardMacros (20);
        add ("Howling Probe", "Lead").osc (1, sync, 60).unison (1, 2, 13, 55).osc (2, analog, squarePos, 65).pitch (2, 0, 3)
            .filter (1, ladder24, 4600, 25).env (2, 0, 280, 25, 200, 60).mod (env2, "filter1Cutoff", 16).lfo (1, sine, 4.6f, 1).lfoFade (1, 400)
            .mod (lfo1, "osc1Pitch", 0.4f).mono (90).amp (6, 400, 80, 280).drive (crush, 40, 6000).delay (eighth, 30, 21, 4000).level (-4)
            .standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Industrial Cry", "Lead").osc (1, analog, sinePos).noise (11, 0).filter (1, svfBand, 720, 75, 0, 100).lfo (1, sine, 4.3f, 1)
            .lfoFade (1, 600).mod (lfo1, "osc1Pitch", 0.4f).mono (140).amp (19, 300, 90, 300).reverb (25, 75, 4, 45).level (6).standardMacros (20);
        add ("Iron Lance", "Lead").osc (1, analog, 20).noise (14, 0).filter (1, svfBand, 780, 60, 0, 100).lfo (1, sine, 5.8f, 1).lfoFade (1, 850)
            .mod (lfo1, "osc1Pitch", 0.5f).mono (110).amp (20, 300, 90, 300).reverb (30, 75, 4, 45).level (6).standardMacros (20);
        add ("Leaden Signal", "Lead").osc (1, analog, sinePos).noise (10, 0).filter (1, svfBand, 390, 75, 0, 100).lfo (1, sine, 4.9f, 1)
            .lfoFade (1, 600).mod (lfo1, "osc1Pitch", 0.6f).mono (80).amp (14, 300, 90, 300).reverb (25, 75, 4, 45).level (2).standardMacros (20);
        add ("Live Beam", "Lead").osc (1, analog, 20).noise (15, 1).filter (1, svfBand, 680, 70, 0, 100).lfo (1, sine, 4.9f, 1).lfoFade (1, 750)
            .mod (lfo1, "osc1Pitch", 0.6f).mono (120).amp (29, 300, 90, 300).reverb (30, 75, 4, 45).level (6).standardMacros (20);
        add ("Live Needle", "Lead").osc (1, analog, sinePos).noise (11, 0).filter (1, svfBand, 330, 65, 0, 100).lfo (1, sine, 5.2f, 1)
            .lfoFade (1, 600).mod (lfo1, "osc1Pitch", 0.7f).mono (140).amp (19, 300, 90, 300).reverb (30, 75, 4, 45).level (-2).standardMacros (20);
        add ("Live Voice", "Lead").osc (1, fm, 35).lfoSync (1, sawUp, bar1, 1).mod (lfo1, "osc1Position", 35).filter (1, ladder12, 1800, 30)
            .drive (tube, 30).mono (50).amp (4, 400, 85, 200).level (-10).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Molten Filament", "Lead").osc (1, vowel, 75).lfoSync (1, sawUp, bar1, 1).mod (lfo1, "osc1Position", 20).filter (1, svfLow, 2900, 30)
            .drive (hard, 40).mono (60).amp (9, 400, 85, 200).level (-14).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Mute Filament", "Lead").osc (1, sync, 45).unison (1, 4, 6, 55).filter (1, svfLow, 4100, 25).env (2, 0, 420, 30, 200, 60)
            .mod (env2, "filter1Cutoff", 19).lfo (1, sine, 4.5f, 1).lfoFade (1, 350).mod (lfo1, "osc1Pitch", 0.6f).mono (80).amp (6, 400, 80, 300)
            .delay (eighthD, 40, 23, 4000).level (-4).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Mute Needle", "Lead").osc (1, growl, 65).unison (1, 4, 9, 55).osc (2, pulse, sawPos, 55).pitch (2, 12, 8).filter (1, ladder24, 3500, 30)
            .env (2, 0, 200, 25, 200, 60).mod (env2, "filter1Cutoff", 16).lfo (1, sine, 4.6f, 1).lfoFade (1, 700).mod (lfo1, "osc1Pitch", 0.5f)
            .mono (90).amp (3, 400, 80, 240).drive (tube, 30, 6000).delay (eighth, 40, 19, 4000).level (-10).standardMacros (25).macro (4, "Vibrato")
            .mod (macro4, "lfo1Rate", 15);
        add ("Mute Voice", "Lead").osc (1, analog, sinePos).noise (11, 0).filter (1, svfBand, 330, 55, 0, 100).lfo (1, sine, 4.6f, 1).lfoFade (1, 850)
            .mod (lfo1, "osc1Pitch", 0.8f).mono (150).amp (18, 300, 90, 300).reverb (25, 75, 4, 45).level (-3).standardMacros (20);
        add ("Night Laser", "Lead").osc (1, analog, trianglePos).noise (9, 1).filter (1, svfBand, 650, 70, 0, 100).lfo (1, sine, 5.7f, 1)
            .lfoFade (1, 550).mod (lfo1, "osc1Pitch", 0.5f).mono (140).amp (17, 300, 90, 300).reverb (35, 75, 4, 45).level (6).standardMacros (20);
        add ("Night Lead", "Lead").osc (1, fm, 30).lfoSync (1, sine, bar1, 1).mod (lfo1, "osc1Position", 25).filter (1, ladder12, 1500, 40)
            .drive (hard, 45).mono (50).amp (7, 400, 85, 200).level (-18).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Null Lead", "Lead").osc (1, analog, trianglePos).noise (8, 0).filter (1, svfBand, 510, 75, 0, 100).lfo (1, sine, 5.9f, 1)
            .lfoFade (1, 750).mod (lfo1, "osc1Pitch", 0.6f).mono (110).amp (17, 300, 90, 300).reverb (30, 75, 4, 45).level (6).standardMacros (20);
        add ("Obsidian Beam", "Lead").osc (1, additive, 20).lfoSync (1, triangle, quarter, 1).mod (lfo1, "osc1Position", 20)
            .filter (1, svfLow, 2600, 30).drive (hard, 30).mono (60).amp (5, 400, 85, 200).reverb (15, 70, 3, 45).level (-13).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 60).macro (2, "Grit").mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Oily Alarm", "Lead").osc (1, analog, sawPos).unison (1, 1, 11, 65).osc (2, pulse, sawPos, 50).pitch (2, 0, 10)
            .filter (1, svfLow, 3500, 20).env (2, 0, 330, 40, 200, 60).mod (env2, "filter1Cutoff", 17).lfo (1, sine, 4.8f, 1).lfoFade (1, 450)
            .mod (lfo1, "osc1Pitch", 0.4f).mono (70).amp (8, 400, 80, 290).drive (tube, 45, 7000).delay (eighthD, 30, 13, 4000).level (-16)
            .standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Piercing Flare", "Lead").osc (1, fm, 65).lfoSync (1, sine, quarter, 1).mod (lfo1, "osc1Position", 35).filter (1, ladder12, 3300, 45)
            .drive (hard, 45).mono (50).amp (5, 400, 85, 200).reverb (25, 70, 3, 45).level (-20).macro (1, "Vowel").mod (macro1, "osc1Position", 60)
            .macro (2, "Grit").mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Piercing Needle", "Lead").osc (1, additive, 30).lfoSync (1, sawUp, bar1, 1).mod (lfo1, "osc1Position", 20)
            .filter (1, ladder12, 2500, 35).drive (fuzz, 40).mono (90).amp (14, 400, 85, 200).reverb (25, 70, 3, 45).level (-23).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 60).macro (2, "Grit").mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Piercing Spark", "Lead").osc (1, analog, sawPos).unison (1, 4, 10, 50).filter (1, ladder12, 3900, 30).env (2, 0, 290, 35, 200, 60)
            .mod (env2, "filter1Cutoff", 18).lfo (1, sine, 5.7f, 1).lfoFade (1, 650).mod (lfo1, "osc1Pitch", 0.5f).mono (70).amp (3, 400, 80, 220)
            .drive (fuzz, 25, 6500).delay (quarterT, 40, 19, 4000).level (-20).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Riveted Signal", "Lead").osc (1, analog, sawPos).unison (1, 4, 14, 50).osc (2, analog, sawPos, 50).pitch (2, 12, 5)
            .filter (1, ladder24, 4800, 25).env (2, 0, 380, 30, 200, 60).mod (env2, "filter1Cutoff", 17).lfo (1, sine, 6, 1).lfoFade (1, 450)
            .mod (lfo1, "osc1Pitch", 0.5f).mono (100).amp (6, 400, 80, 180).delay (eighthD, 30, 25, 4000).level (-4).standardMacros (25)
            .macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Rotten Beam", "Lead").osc (1, vowel, 40).lfoSync (1, sawUp, quarter, 1).mod (lfo1, "osc1Position", 35).filter (1, ladder12, 1600, 45)
            .drive (fuzz, 40).mono (80).amp (14, 400, 85, 200).reverb (25, 70, 3, 45).level (-22).macro (1, "Vowel").mod (macro1, "osc1Position", 60)
            .macro (2, "Grit").mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Rotten Cutter", "Lead").osc (1, analog, 20).noise (11, 1).filter (1, svfBand, 350, 75, 0, 100).lfo (1, sine, 4.6f, 1).lfoFade (1, 400)
            .mod (lfo1, "osc1Pitch", 0.5f).mono (160).amp (17, 300, 90, 300).reverb (30, 75, 4, 45).level (1).standardMacros (20);
        add ("Rotten Lead", "Lead").osc (1, analog, sawPos).unison (1, 4, 16, 45).filter (1, svfLow, 2200, 20).env (2, 0, 290, 35, 200, 60)
            .mod (env2, "filter1Cutoff", 14).lfo (1, sine, 5.1f, 1).lfoFade (1, 400).mod (lfo1, "osc1Pitch", 0.6f).mono (30).amp (7, 400, 80, 190)
            .delay (eighthD, 40, 20, 4000).level (-4).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Rotten Probe", "Lead").osc (1, analog, sawPos).unison (1, 3, 7, 50).osc (2, analog, sawPos, 55).pitch (2, 0, 10)
            .filter (1, ladder24, 4400, 30).env (2, 0, 280, 40, 200, 60).mod (env2, "filter1Cutoff", 19).lfo (1, sine, 5.2f, 1).lfoFade (1, 350)
            .mod (lfo1, "osc1Pitch", 0.5f).mono (60).amp (5, 400, 80, 160).drive (tube, 40, 8000).delay (eighthD, 40, 17, 4000).level (-13)
            .standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Rotten Signal", "Lead").osc (1, sync, 35).unison (1, 2, 9, 50).filter (1, ladder12, 2900, 25).env (2, 0, 270, 40, 200, 60)
            .mod (env2, "filter1Cutoff", 15).lfo (1, sine, 5.9f, 1).lfoFade (1, 550).mod (lfo1, "osc1Pitch", 0.6f).mono (70).amp (5, 400, 80, 280)
            .delay (eighthD, 25, 23, 4000).level (-3).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Ruined Siren", "Lead").osc (1, analog, sawPos).unison (1, 2, 11, 60).osc (2, pulse, squarePos, 45).pitch (2, 0, 8)
            .filter (1, ladder24, 4100, 25).env (2, 0, 410, 40, 200, 60).mod (env2, "filter1Cutoff", 19).lfo (1, sine, 4.9f, 1).lfoFade (1, 600)
            .mod (lfo1, "osc1Pitch", 0.6f).mono (70).amp (8, 400, 80, 240).delay (quarterT, 45, 24, 4000).level (-4).standardMacros (25)
            .macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Scorched Lead", "Lead").osc (1, vowel, 40).lfoSync (1, sine, half, 1).mod (lfo1, "osc1Position", 30).filter (1, ladder12, 2500, 20)
            .drive (tube, 35).mono (80).amp (12, 400, 85, 200).level (-10).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Searing Beam", "Lead").osc (1, growl, 50).unison (1, 2, 13, 45).filter (1, svfLow, 2500, 40).env (2, 0, 320, 30, 200, 60)
            .mod (env2, "filter1Cutoff", 11).lfo (1, sine, 5.1f, 1).lfoFade (1, 500).mod (lfo1, "osc1Pitch", 0.5f).mono (60).amp (3, 400, 80, 220)
            .drive (fuzz, 40, 6500).delay (eighth, 40, 14, 4000).level (-23).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Searing Blade", "Lead").osc (1, fm, 40).lfoSync (1, sine, half, 1).mod (lfo1, "osc1Position", 40).filter (1, svfBand, 3100, 20)
            .drive (hard, 30).mono (80).amp (13, 400, 85, 200).reverb (25, 70, 3, 45).level (-10).macro (1, "Vowel").mod (macro1, "osc1Position", 60)
            .macro (2, "Grit").mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Severed Signal", "Lead").osc (1, additive, 50).lfoSync (1, sawUp, half, 1).mod (lfo1, "osc1Position", 30).filter (1, svfLow, 1600, 40)
            .drive (fuzz, 40).mono (50).amp (4, 400, 85, 200).level (-22).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Shadow Probe", "Lead").osc (1, analog, sawPos).unison (1, 1, 14, 50).osc (2, pulse, squarePos, 65).pitch (2, 12, 6)
            .filter (1, svfLow, 2000, 25).env (2, 0, 340, 30, 200, 60).mod (env2, "filter1Cutoff", 19).lfo (1, sine, 4.9f, 1).lfoFade (1, 650)
            .mod (lfo1, "osc1Pitch", 0.6f).mono (100).amp (3, 400, 80, 260).drive (hard, 35, 6000).delay (quarterT, 35, 16, 4000).level (-16)
            .standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Shrill Saw", "Lead").osc (1, growl, 60).unison (1, 4, 12, 55).osc (2, analog, squarePos, 60).pitch (2, 7, 8)
            .filter (1, svfLow, 3300, 40).env (2, 0, 480, 35, 200, 60).mod (env2, "filter1Cutoff", 15).lfo (1, sine, 4.6f, 1).lfoFade (1, 350)
            .mod (lfo1, "osc1Pitch", 0.6f).mono (90).amp (6, 400, 80, 260).delay (eighthD, 25, 19, 4000).level (-8).standardMacros (25)
            .macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Soot Filament", "Lead").osc (1, pulse, 55).unison (1, 1, 13, 50).osc (2, analog, squarePos, 65).pitch (2, -12, 4)
            .filter (1, ladder24, 4600, 35).env (2, 0, 240, 30, 200, 60).mod (env2, "filter1Cutoff", 15).lfo (1, sine, 5, 1).lfoFade (1, 650)
            .mod (lfo1, "osc1Pitch", 0.6f).mono (40).amp (5, 400, 80, 200).drive (crush, 30, 5500).delay (eighthD, 35, 19, 4000).level (-5)
            .standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Static Alarm", "Lead").osc (1, growl, 45).unison (1, 1, 11, 55).filter (1, ladder24, 4100, 25).env (2, 0, 280, 25, 200, 60)
            .mod (env2, "filter1Cutoff", 12).lfo (1, sine, 5.6f, 1).lfoFade (1, 300).mod (lfo1, "osc1Pitch", 0.5f).mono (40).amp (5, 400, 80, 300)
            .delay (eighth, 40, 16, 4000).level (0).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Sterile Probe", "Lead").osc (1, analog, sawPos).unison (1, 4, 12, 65).filter (1, svfLow, 4800, 30).env (2, 0, 460, 40, 200, 60)
            .mod (env2, "filter1Cutoff", 15).lfo (1, sine, 5.5f, 1).lfoFade (1, 650).mod (lfo1, "osc1Pitch", 0.6f).mono (40).amp (4, 400, 80, 170)
            .delay (quarterT, 40, 16, 4000).level (-5).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Sullen Beam", "Lead").osc (1, analog, trianglePos).noise (14, 0).filter (1, svfBand, 760, 75, 0, 100).lfo (1, sine, 4.1f, 1)
            .lfoFade (1, 650).mod (lfo1, "osc1Pitch", 0.6f).mono (140).amp (21, 300, 90, 300).reverb (40, 75, 4, 45).level (6).standardMacros (20);
        add ("Sullen Filament", "Lead").osc (1, analog, 20).noise (7, 1).filter (1, ladderBand, 270, 60, 0, 100).lfo (1, sine, 5.1f, 1)
            .lfoFade (1, 450).mod (lfo1, "osc1Pitch", 0.5f).mono (150).amp (14, 300, 90, 300).reverb (30, 75, 4, 45).level (-8).standardMacros (20);
        add ("Sullen Laser", "Lead").osc (1, analog, 20).noise (6, 0).filter (1, svfBand, 520, 65, 0, 100).lfo (1, sine, 5.8f, 1).lfoFade (1, 500)
            .mod (lfo1, "osc1Pitch", 0.7f).mono (90).amp (14, 300, 90, 300).reverb (25, 75, 4, 45).level (5).standardMacros (20);
        add ("Sullen Wail", "Lead").osc (1, sync, 50).unison (1, 4, 11, 65).filter (1, svfLow, 4200, 30).env (2, 0, 450, 35, 200, 60)
            .mod (env2, "filter1Cutoff", 12).lfo (1, sine, 5.8f, 1).lfoFade (1, 300).mod (lfo1, "osc1Pitch", 0.4f).mono (50).amp (8, 400, 80, 210)
            .delay (eighth, 25, 14, 4000).level (-4).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Sunken Wire", "Lead").osc (1, sync, 65).unison (1, 2, 11, 70).osc (2, pulse, sawPos, 45).pitch (2, 7, 7).filter (1, ladder24, 2500, 35)
            .env (2, 0, 390, 25, 200, 60).mod (env2, "filter1Cutoff", 11).lfo (1, sine, 5, 1).lfoFade (1, 650).mod (lfo1, "osc1Pitch", 0.7f).mono (50)
            .amp (7, 400, 80, 170).drive (fuzz, 35, 6000).delay (eighthD, 35, 25, 4000).level (-21).standardMacros (25).macro (4, "Vibrato")
            .mod (macro4, "lfo1Rate", 15);
        add ("Tar Flare", "Lead").osc (1, analog, sinePos).noise (13, 1).filter (1, svfBand, 450, 65, 0, 100).lfo (1, sine, 4.6f, 1).lfoFade (1, 450)
            .mod (lfo1, "osc1Pitch", 0.4f).mono (150).amp (24, 300, 90, 300).reverb (40, 75, 4, 45).level (1).standardMacros (20);
        add ("Toxic Filament", "Lead").osc (1, sync, 60).unison (1, 3, 6, 40).filter (1, ladder12, 4300, 25).env (2, 0, 280, 25, 200, 60)
            .mod (env2, "filter1Cutoff", 17).lfo (1, sine, 5, 1).lfoFade (1, 550).mod (lfo1, "osc1Pitch", 0.7f).mono (40).amp (8, 400, 80, 200)
            .delay (quarterT, 35, 22, 4000).level (-3).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Vacant Filament", "Lead").osc (1, additive, 20).lfoSync (1, sine, bar1, 1).mod (lfo1, "osc1Position", 35).filter (1, svfBand, 2700, 20)
            .drive (hard, 40).mono (70).amp (13, 400, 85, 200).level (-14).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);
        add ("Vacant Lead", "Lead").osc (1, sync, 45).unison (1, 2, 8, 40).filter (1, ladder24, 2700, 40).env (2, 0, 200, 40, 200, 60)
            .mod (env2, "filter1Cutoff", 16).lfo (1, sine, 5.2f, 1).lfoFade (1, 600).mod (lfo1, "osc1Pitch", 0.4f).mono (40).amp (7, 400, 80, 210)
            .drive (tube, 45, 7500).delay (quarterT, 40, 18, 4000).level (-13).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Void Filament", "Lead").osc (1, analog, sawPos).unison (1, 2, 6, 45).osc (2, analog, sawPos, 50).pitch (2, 0, 3)
            .filter (1, ladder24, 2800, 20).env (2, 0, 400, 35, 200, 60).mod (env2, "filter1Cutoff", 17).lfo (1, sine, 5.5f, 1).lfoFade (1, 450)
            .mod (lfo1, "osc1Pitch", 0.7f).mono (90).amp (2, 400, 80, 270).delay (quarterT, 35, 14, 4000).level (-4).standardMacros (25)
            .macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Void Lead", "Lead").osc (1, analog, sawPos).unison (1, 3, 9, 50).filter (1, svfLow, 3000, 25).env (2, 0, 300, 35, 200, 60)
            .mod (env2, "filter1Cutoff", 15).lfo (1, sine, 5.8f, 1).lfoFade (1, 600).mod (lfo1, "osc1Pitch", 0.4f).mono (60).amp (6, 400, 80, 170)
            .delay (eighthD, 40, 20, 4000).level (-5).standardMacros (25).macro (4, "Vibrato").mod (macro4, "lfo1Rate", 15);
        add ("Wailing Cutter", "Lead").osc (1, analog, sinePos).noise (11, 0).filter (1, ladderBand, 390, 80, 0, 100).lfo (1, sine, 5.6f, 1)
            .lfoFade (1, 700).mod (lfo1, "osc1Pitch", 0.8f).mono (110).amp (15, 300, 90, 300).reverb (30, 75, 4, 45).level (-4).standardMacros (20);
        add ("Welded Probe", "Lead").osc (1, vowel, 60).lfoSync (1, triangle, half, 1).mod (lfo1, "osc1Position", 30).filter (1, svfBand, 1900, 45)
            .drive (hard, 35).mono (60).amp (5, 400, 85, 200).level (-10).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Grit")
            .mod (macro2, "fxDriveAmount", 60).macro (3, "Open").mod (macro3, "filter1Cutoff", 30);

        // Stab, by name.
        add ("Abandoned Clamp", "Stab").osc (1, growl, 50).osc (2, growl, 50, 60).pitch (2, 5).osc (3, growl, 50, 55).pitch (3, 7)
            .unison (1, 3, 10, 65).filter (1, ladder24, 1900, 30).env (2, 0, 120, 0, 150, 80).mod (env2, "filter1Cutoff", 21).amp (0, 190, 0, 190)
            .delay (sixteenthD, 40, 35, 3750).reverb (25, 85, 3.5f, 45).level (-8).standardMacros (30);
        add ("Abandoned Crack", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, 7).osc (3, analog, sawPos, 55).pitch (3, 12)
            .filter (1, ladder24, 1800, 25).env (2, 0, 160, 0, 150, 80).mod (env2, "filter1Cutoff", 25).amp (0, 320, 0, 180).reverb (25, 65, 2.5f, 45)
            .drive (tube, 25).level (-18).standardMacros (30);
        add ("Abandoned Jab", "Stab").osc (1, pulse, 55).osc (2, pulse, 55, 75).pitch (2, 3).unison (1, 2, 7, 80).filter (1, ladder24, 1300, 35)
            .env (2, 0, 200, 0, 150, 80).mod (env2, "filter1Cutoff", 26).amp (0, 250, 0, 130).reverb (35, 85, 1.5f, 45).drive (hard, 25).level (-17)
            .standardMacros (30);
        add ("Acid Chop", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 65).pitch (2, 7).osc (3, analog, sawPos, 65).pitch (3, 10)
            .filter (1, ladder24, 1900, 40).env (2, 0, 240, 0, 150, 80).mod (env2, "filter1Cutoff", 27).amp (0, 190, 0, 240)
            .delay (eighthD, 55, 25, 2000).reverb (30, 75, 3.5f, 45).level (-10).standardMacros (30);
        add ("Acid Crack", "Stab").osc (1, metal, 30).osc (2, analog, squarePos, 65).filter (1, ladder24, 2400, 50).env (2, 0, 140, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 20).amp (0, 240, 0, 150).drive (crush, 35).reverb (35, 60, 1.8f, 45).level (-10).standardMacros (25);
        add ("Acid Rivet", "Stab").osc (1, pulse, 60).osc (2, pulse, 60, 70).pitch (2, 3).osc (3, pulse, 60, 70).pitch (3, 7)
            .filter (1, ladder24, 2200, 20).env (2, 0, 210, 0, 150, 80).mod (env2, "filter1Cutoff", 30).amp (0, 320, 0, 180)
            .delay (sixteenthD, 40, 25, 3250).reverb (35, 80, 3, 45).drive (tube, 30).level (-15).standardMacros (30);
        add ("Ash Knock", "Stab").osc (1, metal, 65).osc (2, analog, sawPos, 55).filter (1, svfBand, 1200, 40).env (2, 0, 100, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 20).amp (0, 250, 0, 150).drive (hard, 30).reverb (30, 60, 1.8f, 45).level (-10).standardMacros (25);
        add ("Ash Slam", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 70).pitch (2, 3).osc (3, analog, sawPos, 55).pitch (3, 7)
            .unison (1, 3, 11, 75).filter (1, ladder24, 2400, 30).env (2, 0, 140, 0, 150, 80).mod (env2, "filter1Cutoff", 29).amp (0, 380, 0, 230)
            .delay (sixteenthD, 40, 25, 3500).reverb (35, 80, 3, 45).level (-10).standardMacros (30);
        add ("Basalt Rivet", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 65).pitch (2, 7).osc (3, analog, sawPos, 55).pitch (3, 10)
            .unison (1, 2, 14, 60).filter (1, ladder24, 1100, 30).env (2, 0, 130, 0, 150, 80).mod (env2, "filter1Cutoff", 26).amp (0, 360, 0, 180)
            .delay (sixteenthD, 50, 30, 3750).reverb (20, 90, 2, 45).level (-10).standardMacros (30);
        add ("Brutal Stamp", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 70).pitch (2, 3).osc (3, analog, sawPos, 60).pitch (3, 7)
            .filter (1, ladder24, 1100, 20).env (2, 0, 220, 0, 150, 80).mod (env2, "filter1Cutoff", 21).amp (0, 200, 0, 140)
            .delay (sixteenthD, 50, 25, 2250).reverb (20, 80, 4, 45).drive (hard, 35).level (-19).standardMacros (30);
        add ("Burnt Slam", "Stab").osc (1, sync, 40).osc (2, sync, 40, 70).pitch (2, 5).filter (1, ladder24, 1000, 40).env (2, 0, 100, 0, 150, 80)
            .mod (env2, "filter1Cutoff", 28).amp (0, 210, 0, 250).delay (eighthD, 45, 20, 2500).reverb (20, 80, 2.5f, 45).level (-7)
            .standardMacros (30);
        add ("Choked Chop", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, 5).unison (1, 3, 13, 60)
            .filter (1, ladder24, 2500, 20).env (2, 0, 180, 0, 150, 80).mod (env2, "filter1Cutoff", 29).amp (0, 240, 0, 200)
            .delay (eighthD, 55, 20, 3000).reverb (25, 80, 3.5f, 45).level (-10).standardMacros (30);
        add ("Chrome Clamp", "Stab").osc (1, pulse, 45).osc (2, pulse, 45, 60).pitch (2, 7).osc (3, pulse, 45, 70).pitch (3, 15).unison (1, 2, 10, 80)
            .filter (1, ladder24, 800, 20).env (2, 0, 160, 0, 150, 80).mod (env2, "filter1Cutoff", 30).amp (0, 370, 0, 220)
            .delay (sixteenthD, 35, 35, 3250).reverb (30, 85, 3.5f, 45).level (-10).standardMacros (30);
        add ("Chrome Stamp", "Stab").osc (1, pulse, 65).osc (2, pulse, 65, 70).pitch (2, 5).osc (3, pulse, 65, 50).pitch (3, 7).unison (1, 2, 10, 75)
            .filter (1, ladder24, 1400, 25).env (2, 0, 210, 0, 150, 80).mod (env2, "filter1Cutoff", 26).amp (0, 380, 0, 230).reverb (30, 70, 3.5f, 45)
            .level (-10).standardMacros (30);
        add ("Cinder Bolt", "Stab").osc (1, pulse, 55).osc (2, pulse, 55, 65).pitch (2, 3).osc (3, pulse, 55, 50).pitch (3, 10).unison (1, 2, 7, 75)
            .filter (1, ladder24, 1400, 30).env (2, 0, 150, 0, 150, 80).mod (env2, "filter1Cutoff", 21).amp (0, 320, 0, 220).reverb (30, 90, 3.5f, 45)
            .level (-10).standardMacros (30);
        add ("Corroded Bolt", "Stab").osc (1, growl, 30).osc (2, growl, 30).pitch (2, 7).filter (1, ladder24, 2500, 25).env (2, 0, 130, 0, 150, 80)
            .mod (env2, "filter1Cutoff", 26).amp (0, 320, 0, 120).delay (eighthD, 45, 30, 2750).reverb (20, 60, 1.5f, 45).level (-12)
            .standardMacros (30);
        add ("Corroded Strike", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 65).pitch (2, 3).osc (3, analog, sawPos, 65).pitch (3, 7)
            .unison (1, 2, 9, 50).filter (1, ladder24, 1000, 35).env (2, 0, 130, 0, 150, 80).mod (env2, "filter1Cutoff", 23).amp (0, 290, 0, 120)
            .reverb (30, 65, 3, 45).drive (tube, 30).level (-15).standardMacros (30);
        add ("Derelict Chop", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, 7).osc (3, analog, sawPos, 50).pitch (3, 10)
            .unison (1, 2, 13, 75).filter (1, ladder24, 1600, 25).env (2, 0, 160, 0, 150, 80).mod (env2, "filter1Cutoff", 22).amp (0, 320, 0, 130)
            .reverb (20, 65, 2.5f, 45).drive (tube, 30).level (-18).standardMacros (30);
        add ("Derelict Chord", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 60).pitch (2, 3).osc (3, analog, sawPos, 65).pitch (3, 10)
            .filter (1, ladder24, 1000, 25).env (2, 0, 180, 0, 150, 80).mod (env2, "filter1Cutoff", 22).amp (0, 250, 0, 230)
            .delay (eighthD, 40, 30, 3750).reverb (30, 75, 2.5f, 45).drive (hard, 30).level (-20).standardMacros (30);
        add ("Derelict Snap", "Stab").osc (1, metal, 75).osc (2, analog, sawPos, 65).filter (1, comb, 1500, 40).env (2, 0, 160, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 25).amp (0, 210, 0, 150).drive (fuzz, 25).reverb (25, 60, 1.8f, 45).level (-23).standardMacros (25);
        add ("Dub Stab", "Stab").osc (1, sync, 50).osc (2, sync, 50, 60).pitch (2, 3).osc (3, sync, 50, 60).pitch (3, 7).unison (1, 3, 13, 70)
            .filter (1, ladder24, 2100, 15).env (2, 0, 180, 0, 150, 80).mod (env2, "filter1Cutoff", 25).amp (0, 390, 0, 200)
            .delay (sixteenthD, 55, 30, 2500).reverb (25, 90, 1.5f, 45).drive (hard, 25).level (-16).standardMacros (30);
        add ("Dub Stamp", "Stab").osc (1, growl, 60).osc (2, growl, 60, 75).pitch (2, 5).osc (3, growl, 60, 60).pitch (3, 7).unison (1, 2, 8, 55)
            .filter (1, ladder24, 1200, 25).env (2, 0, 180, 0, 150, 80).mod (env2, "filter1Cutoff", 25).amp (0, 370, 0, 210).reverb (25, 70, 3.5f, 45)
            .level (-10).standardMacros (30);
        add ("Frozen Knock", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, 5).osc (3, analog, sawPos, 70).pitch (3, 7)
            .filter (1, ladder24, 1800, 25).env (2, 0, 140, 0, 150, 80).mod (env2, "filter1Cutoff", 22).amp (0, 260, 0, 170).reverb (20, 75, 3.5f, 45)
            .level (-10).standardMacros (30);
        add ("Hard Slam", "Stab").osc (1, metal, 50).osc (2, analog, squarePos, 70).noise (35, 2).filter (1, comb, 1500, 30)
            .env (2, 0, 90, 0, 120, 80).mod (env2, "filter1Cutoff", 15).amp (0, 180, 0, 150).drive (hard, 20).reverb (30, 60, 1.8f, 45).level (-17)
            .standardMacros (25);
        add ("Hard Snap", "Stab").osc (1, sync, 60).osc (2, sync, 60, 70).pitch (2, 7).filter (1, ladder24, 1400, 20).env (2, 0, 170, 0, 150, 80)
            .mod (env2, "filter1Cutoff", 28).amp (0, 330, 0, 190).reverb (25, 85, 2.5f, 45).level (-10).standardMacros (30);
        add ("Hollow Slam", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos).pitch (2, 7).osc (3, analog, sawPos, 50).pitch (3, 10)
            .filter (1, ladder24, 1100, 30).env (2, 0, 180, 0, 150, 80).mod (env2, "filter1Cutoff", 23).amp (0, 400, 0, 150).reverb (30, 85, 3, 45)
            .level (-10).standardMacros (30);
        add ("Industrial Bolt", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 60).pitch (2, 5).osc (3, analog, sawPos, 65).pitch (3, 7)
            .unison (1, 2, 10, 55).filter (1, ladder24, 1500, 15).env (2, 0, 230, 0, 150, 80).mod (env2, "filter1Cutoff", 25).amp (0, 190, 0, 130)
            .delay (quarterD, 55, 20, 3000).reverb (30, 75, 1.5f, 45).drive (hard, 25).level (-16).standardMacros (30);
        add ("Leaden Shot", "Stab").osc (1, fm, 65).osc (2, analog, sawPos, 55).noise (15, 0).filter (1, ladder24, 1000, 45).env (2, 0, 110, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 15).amp (0, 280, 0, 150).drive (fuzz, 35).reverb (30, 60, 1.8f, 45).level (-23).standardMacros (25);
        add ("Molten Stab", "Stab").osc (1, metal, 65).osc (2, analog, sawPos, 70).noise (20, 0).filter (1, svfBand, 2300, 30)
            .env (2, 0, 120, 0, 120, 80).mod (env2, "filter1Cutoff", 20).amp (0, 170, 0, 150).drive (hard, 30).reverb (30, 60, 1.8f, 45).level (-14)
            .standardMacros (25);
        add ("Molten Stamp", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 70).pitch (2, 7).osc (3, analog, sawPos, 65).pitch (3, 15)
            .unison (1, 2, 8, 55).filter (1, ladder24, 1200, 20).env (2, 0, 240, 0, 150, 80).mod (env2, "filter1Cutoff", 25).amp (0, 170, 0, 150)
            .delay (eighthD, 40, 25, 3500).reverb (25, 65, 3.5f, 45).drive (tube, 25).level (-14).standardMacros (30);
        add ("Mute Clamp", "Stab").osc (1, metal, 40).osc (2, analog, sawPos, 50).noise (15, 0).filter (1, svfBand, 1900, 35)
            .env (2, 0, 150, 0, 120, 80).mod (env2, "filter1Cutoff", 20).amp (0, 210, 0, 150).drive (crush, 25).reverb (30, 60, 1.8f, 45).level (-5)
            .standardMacros (25);
        add ("Mute Spike", "Stab").osc (1, metal, 65).osc (2, analog, sawPos, 45).noise (15, 0).filter (1, ladder24, 600, 60)
            .env (2, 0, 150, 0, 120, 80).mod (env2, "filter1Cutoff", 30).amp (0, 200, 0, 150).drive (crush, 35).reverb (25, 60, 1.8f, 45).level (-3)
            .standardMacros (25);
        add ("Mute Stab", "Stab").osc (1, sync, 45).osc (2, sync, 45, 65).pitch (2, 3).unison (1, 2, 12, 70).filter (1, ladder24, 1100, 20)
            .env (2, 0, 150, 0, 150, 80).mod (env2, "filter1Cutoff", 21).amp (0, 170, 0, 230).reverb (20, 90, 2, 45).level (-8).standardMacros (30);
        add ("Nickel Chord", "Stab").osc (1, fm, 90).osc (2, analog, sawPos, 70).noise (25, 0).filter (1, svfBand, 1500, 35)
            .env (2, 0, 100, 0, 120, 80).mod (env2, "filter1Cutoff", 25).amp (0, 210, 0, 150).drive (hard, 30).reverb (30, 60, 1.8f, 45).level (-16)
            .standardMacros (25);
        add ("Nickel Slam", "Stab").osc (1, metal, 90).osc (2, analog, squarePos, 60).noise (15, 3).filter (1, svfBand, 1400, 40)
            .env (2, 0, 90, 0, 120, 80).mod (env2, "filter1Cutoff", 20).amp (0, 290, 0, 150).drive (crush, 35).reverb (35, 60, 1.8f, 45).level (-3)
            .standardMacros (25);
        add ("Night Chord", "Stab").osc (1, pulse, 50).osc (2, pulse, 50, 65).pitch (2, 3).filter (1, ladder24, 1800, 30).env (2, 0, 240, 0, 150, 80)
            .mod (env2, "filter1Cutoff", 23).amp (0, 370, 0, 220).delay (quarterD, 40, 25, 3750).reverb (25, 85, 2.5f, 45).drive (hard, 25)
            .level (-19).standardMacros (30);
        add ("Night Clamp", "Stab").osc (1, fm, 35).osc (2, analog, squarePos, 60).filter (1, svfBand, 2500, 35).env (2, 0, 130, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 20).amp (0, 180, 0, 150).drive (hard, 25).reverb (25, 60, 1.8f, 45).level (-5).standardMacros (25);
        add ("Null Bolt", "Stab").osc (1, fm, 70).osc (2, analog, squarePos, 70).noise (30, 2).filter (1, svfBand, 1800, 35)
            .env (2, 0, 170, 0, 120, 80).mod (env2, "filter1Cutoff", 20).amp (0, 230, 0, 150).drive (crush, 35).reverb (20, 60, 1.8f, 45).level (-7)
            .standardMacros (25);
        add ("Null Punch", "Stab").osc (1, metal, 60).osc (2, analog, squarePos, 65).noise (15, 2).filter (1, svfBand, 1000, 60)
            .env (2, 0, 150, 0, 120, 80).mod (env2, "filter1Cutoff", 20).amp (0, 250, 0, 150).drive (crush, 35).reverb (30, 60, 1.8f, 45).level (-2)
            .standardMacros (25);
        add ("Obsidian Blow", "Stab").osc (1, pulse, 65).osc (2, pulse, 65, 70).pitch (2, 3).osc (3, pulse, 65, 50).pitch (3, 7)
            .filter (1, ladder24, 1600, 35).env (2, 0, 180, 0, 150, 80).mod (env2, "filter1Cutoff", 26).amp (0, 350, 0, 160).reverb (20, 75, 2.5f, 45)
            .drive (hard, 30).level (-16).standardMacros (30);
        add ("Obsidian Hit", "Stab").osc (1, metal, 70).osc (2, analog, sawPos, 60).noise (25, 3).filter (1, svfBand, 500, 55)
            .env (2, 0, 140, 0, 120, 80).mod (env2, "filter1Cutoff", 30).amp (0, 300, 0, 150).drive (fuzz, 25).reverb (25, 60, 1.8f, 45).level (-18)
            .standardMacros (25);
        add ("Riveted Jab", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 70).pitch (2, 7).osc (3, analog, sawPos, 60).pitch (3, 10)
            .filter (1, ladder24, 1400, 25).env (2, 0, 230, 0, 150, 80).mod (env2, "filter1Cutoff", 27).amp (0, 390, 0, 190).reverb (30, 65, 3.5f, 45)
            .level (-12).standardMacros (30);
        add ("Rotten Crack", "Stab").osc (1, sync, 35).osc (2, sync, 35, 75).pitch (2, 3).osc (3, sync, 35, 50).pitch (3, 10).unison (1, 3, 12, 55)
            .filter (1, ladder24, 1900, 20).env (2, 0, 110, 0, 150, 80).mod (env2, "filter1Cutoff", 23).amp (0, 320, 0, 180)
            .delay (sixteenthD, 45, 20, 2750).reverb (20, 65, 2.5f, 45).level (-8).standardMacros (30);
        add ("Ruined Bolt", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, 3).osc (3, analog, sawPos, 60).pitch (3, 10)
            .unison (1, 2, 9, 70).filter (1, ladder24, 2000, 25).env (2, 0, 220, 0, 150, 80).mod (env2, "filter1Cutoff", 21).amp (0, 260, 0, 230)
            .delay (eighthD, 40, 25, 2500).reverb (30, 80, 3.5f, 45).level (-10).standardMacros (30);
        add ("Ruined Hit", "Stab").osc (1, metal, 45).osc (2, analog, squarePos, 55).noise (25, 2).filter (1, svfBand, 700, 65)
            .env (2, 0, 100, 0, 120, 80).mod (env2, "filter1Cutoff", 15).amp (0, 260, 0, 150).drive (fuzz, 40).reverb (25, 60, 1.8f, 45).level (-19)
            .standardMacros (25);
        add ("Ruined Slam", "Stab").osc (1, pulse, 40).osc (2, pulse, 40, 65).pitch (2, 7).filter (1, ladder24, 2400, 20).env (2, 0, 210, 0, 150, 80)
            .mod (env2, "filter1Cutoff", 22).amp (0, 260, 0, 150).reverb (25, 85, 2, 45).level (-10).standardMacros (30);
        add ("Rust Crack", "Stab").osc (1, sync, 65).osc (2, sync, 65, 75).pitch (2, 7).osc (3, sync, 65, 55).pitch (3, 15)
            .filter (1, ladder24, 1700, 35).env (2, 0, 240, 0, 150, 80).mod (env2, "filter1Cutoff", 23).amp (0, 250, 0, 200).reverb (25, 80, 3, 45)
            .level (-10).standardMacros (30);
        add ("Scorched Snap", "Stab").osc (1, metal, 45).osc (2, analog, squarePos, 70).filter (1, svfBand, 1300, 55).env (2, 0, 90, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 25).amp (0, 170, 0, 150).drive (hard, 35).reverb (20, 60, 1.8f, 45).level (-14).standardMacros (25);
        add ("Severed Rivet", "Stab").osc (1, fm, 40).osc (2, analog, sawPos, 40).noise (15, 0).filter (1, svfBand, 1500, 55)
            .env (2, 0, 180, 0, 120, 80).mod (env2, "filter1Cutoff", 20).amp (0, 250, 0, 150).drive (hard, 35).reverb (20, 60, 1.8f, 45).level (-2)
            .standardMacros (25);
        add ("Shadow Rivet", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos).pitch (2, 7).osc (3, analog, sawPos, 60).pitch (3, 12)
            .unison (1, 3, 11, 80).filter (1, ladder24, 1400, 30).env (2, 0, 140, 0, 150, 80).mod (env2, "filter1Cutoff", 26).amp (0, 290, 0, 170)
            .delay (quarterD, 45, 30, 3000).reverb (30, 90, 2.5f, 45).level (-10).standardMacros (30);
        add ("Sharp Chord", "Stab").osc (1, pulse, 65).osc (2, pulse, 65, 75).pitch (2, 3).filter (1, ladder24, 2100, 40).env (2, 0, 150, 0, 150, 80)
            .mod (env2, "filter1Cutoff", 22).amp (0, 220, 0, 220).delay (eighthD, 50, 20, 2250).reverb (30, 80, 3.5f, 45).drive (tube, 20).level (-13)
            .standardMacros (30);
        add ("Sharp Crack", "Stab").osc (1, pulse, 65).osc (2, pulse, 65, 65).pitch (2, 3).osc (3, pulse, 65, 65).pitch (3, 7).unison (1, 2, 9, 55)
            .filter (1, ladder24, 800, 25).env (2, 0, 130, 0, 150, 80).mod (env2, "filter1Cutoff", 25).amp (0, 380, 0, 240).reverb (35, 60, 3.5f, 45)
            .level (-10).standardMacros (30);
        add ("Short Knock", "Stab").osc (1, fm, 35).osc (2, analog, squarePos, 65).filter (1, ladder24, 2400, 55).env (2, 0, 160, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 20).amp (0, 240, 0, 150).drive (fuzz, 35).reverb (25, 60, 1.8f, 45).level (-23).standardMacros (25);
        add ("Short Slam", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 60).pitch (2, 7).osc (3, analog, sawPos, 70).pitch (3, 10)
            .filter (1, ladder24, 1400, 35).env (2, 0, 210, 0, 150, 80).mod (env2, "filter1Cutoff", 24).amp (0, 190, 0, 220)
            .delay (sixteenthD, 35, 25, 3500).reverb (30, 70, 2, 45).drive (tube, 20).level (-17).standardMacros (30);
        add ("Short Stab", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, 3).osc (3, analog, sawPos, 60).pitch (3, 7)
            .filter (1, ladder24, 1600, 35).env (2, 0, 190, 0, 150, 80).mod (env2, "filter1Cutoff", 23).amp (0, 240, 0, 220).reverb (30, 80, 2, 45)
            .level (-10).standardMacros (30);
        add ("Static Blow", "Stab").osc (1, metal, 80).osc (2, analog, sawPos, 60).filter (1, svfBand, 1000, 40).env (2, 0, 110, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 20).amp (0, 210, 0, 150).drive (hard, 30).reverb (25, 60, 1.8f, 45).level (-10).standardMacros (25);
        add ("Steel Rivet", "Stab").osc (1, pulse, 35).osc (2, pulse, 35, 75).pitch (2, 7).osc (3, pulse, 35, 55).pitch (3, 15)
            .filter (1, ladder24, 900, 30).env (2, 0, 130, 0, 150, 80).mod (env2, "filter1Cutoff", 30).amp (0, 400, 0, 180)
            .delay (sixteenthD, 50, 25, 3750).reverb (25, 60, 4, 45).level (-10).standardMacros (30);
        add ("Strobe Slam", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 70).pitch (2, 5).osc (3, analog, sawPos, 65).pitch (3, 7)
            .unison (1, 2, 7, 55).filter (1, ladder24, 2000, 35).env (2, 0, 170, 0, 150, 80).mod (env2, "filter1Cutoff", 30).amp (0, 310, 0, 200)
            .reverb (30, 60, 3.5f, 45).drive (hard, 25).level (-16).standardMacros (30);
        add ("Sullen Chord", "Stab").osc (1, pulse, 40).osc (2, pulse, 40).pitch (2, 7).osc (3, pulse, 40, 60).pitch (3, 15)
            .filter (1, ladder24, 1700, 20).env (2, 0, 160, 0, 150, 80).mod (env2, "filter1Cutoff", 28).amp (0, 170, 0, 200).reverb (25, 85, 2.5f, 45)
            .level (-10).standardMacros (30);
        add ("Tar Shot", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 70).pitch (2, 3).osc (3, analog, sawPos, 60).pitch (3, 10)
            .filter (1, ladder24, 2300, 15).env (2, 0, 130, 0, 150, 80).mod (env2, "filter1Cutoff", 22).amp (0, 270, 0, 150).reverb (25, 80, 2.5f, 45)
            .level (-10).standardMacros (30);
        add ("Toxic Chop", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 70).pitch (2, 5).osc (3, analog, sawPos, 70).pitch (3, 7)
            .filter (1, ladder24, 1700, 35).env (2, 0, 140, 0, 150, 80).mod (env2, "filter1Cutoff", 21).amp (0, 360, 0, 210)
            .delay (eighthD, 50, 20, 3250).reverb (25, 65, 3.5f, 45).level (-10).standardMacros (30);
        add ("Toxic Strike", "Stab").osc (1, fm, 30).osc (2, analog, squarePos, 60).filter (1, svfBand, 900, 45).env (2, 0, 160, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 20).amp (0, 220, 0, 150).drive (hard, 35).reverb (30, 60, 1.8f, 45).level (-12).standardMacros (25);
        add ("Vacant Blow", "Stab").osc (1, metal, 85).osc (2, analog, sawPos, 60).noise (25, 2).filter (1, svfBand, 800, 40)
            .env (2, 0, 140, 0, 120, 80).mod (env2, "filter1Cutoff", 25).amp (0, 230, 0, 150).drive (crush, 30).reverb (35, 60, 1.8f, 45).level (-1)
            .standardMacros (25);
        add ("Vacant Crack", "Stab").osc (1, fm, 85).osc (2, analog, squarePos, 50).noise (30, 2).filter (1, svfBand, 1200, 55)
            .env (2, 0, 120, 0, 120, 80).mod (env2, "filter1Cutoff", 25).amp (0, 230, 0, 150).drive (fuzz, 25).reverb (30, 60, 1.8f, 45).level (-22)
            .standardMacros (25);
        add ("Vacant Strike", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 65).pitch (2, 3).osc (3, analog, sawPos, 65).pitch (3, 7)
            .filter (1, ladder24, 1100, 25).env (2, 0, 190, 0, 150, 80).mod (env2, "filter1Cutoff", 24).amp (0, 180, 0, 150)
            .delay (eighthD, 40, 30, 3500).reverb (20, 90, 2, 45).level (-10).standardMacros (30);
        add ("Warehouse Bolt", "Stab").osc (1, analog, sawPos).osc (2, analog, sawPos, 75).pitch (2, 3).osc (3, analog, sawPos, 70).pitch (3, 7)
            .filter (1, ladder24, 1200, 30).env (2, 0, 230, 0, 150, 80).mod (env2, "filter1Cutoff", 29).amp (0, 280, 0, 250).reverb (35, 85, 3, 45)
            .level (-10).standardMacros (30);
        add ("Welded Punch", "Stab").osc (1, pulse, 55).osc (2, pulse, 55, 65).pitch (2, 5).osc (3, pulse, 55, 55).pitch (3, 7).unison (1, 3, 11, 55)
            .filter (1, ladder24, 1900, 40).env (2, 0, 120, 0, 150, 80).mod (env2, "filter1Cutoff", 26).amp (0, 200, 0, 200)
            .delay (quarterD, 50, 25, 2750).reverb (30, 60, 1.5f, 45).drive (tube, 25).level (-12).standardMacros (30);
        add ("Welded Slam", "Stab").osc (1, fm, 50).osc (2, analog, sawPos, 45).filter (1, comb, 900, 35).env (2, 0, 100, 0, 120, 80)
            .mod (env2, "filter1Cutoff", 15).amp (0, 150, 0, 150).drive (hard, 30).reverb (20, 60, 1.8f, 45).level (-16).standardMacros (25);
        add ("Zinc Bolt", "Stab").osc (1, sync, 60).osc (2, sync, 60, 60).pitch (2, 3).filter (1, ladder24, 1700, 35).env (2, 0, 230, 0, 150, 80)
            .mod (env2, "filter1Cutoff", 25).amp (0, 270, 0, 220).delay (sixteenthD, 60, 35, 3000).reverb (20, 70, 2, 45).level (-6)
            .standardMacros (30);
        add ("Zinc Punch", "Stab").osc (1, pulse, 45).osc (2, pulse, 45, 70).pitch (2, 7).unison (1, 3, 12, 70).filter (1, ladder24, 1900, 35)
            .env (2, 0, 160, 0, 150, 80).mod (env2, "filter1Cutoff", 29).amp (0, 240, 0, 140).delay (sixteenthD, 50, 25, 2000)
            .reverb (20, 85, 1.5f, 45).level (-10).standardMacros (30);

        // Pluck, by name.
        add ("Acid Shard", "Pluck").osc (1, analog, sawPos).osc (2, analog, sawPos, 50).pitch (2, 12).filter (1, ladder24, 450, 45, 0, 50)
            .env (2, 0, 160, 0, 150, 85).mod (env2, "filter1Cutoff", 43).mod (velocity, "filter1Cutoff", 9).amp (0, 530, 0, 310)
            .delay (eighthD, 45, 19, 3500).reverb (25, 60, 2.5f, 45).level (-10).standardMacros (30);
        add ("Basalt Wire", "Pluck").osc (1, analog, sawPos).filter (1, svfLow, 1000, 50, 0, 50).env (2, 0, 220, 0, 150, 85)
            .mod (env2, "filter1Cutoff", 34).mod (velocity, "filter1Cutoff", 10).amp (0, 370, 0, 380).delay (eighthD, 35, 24, 3000).level (-10)
            .standardMacros (30);
        add ("Black Pin", "Pluck").osc (1, analog, sawPos).osc (2, analog, squarePos, 55).pitch (2, 7).filter (1, svfLow, 1050, 50, 0, 50)
            .env (2, 0, 130, 0, 150, 85).mod (env2, "filter1Cutoff", 39).mod (velocity, "filter1Cutoff", 15).amp (0, 480, 0, 390)
            .delay (quarterT, 35, 25, 3000).level (-12).standardMacros (30);
        add ("Black Tick", "Pluck").osc (1, string, 55, 90).warp (1, wOff, 35).filter (1, svfLow, 5500, 20).amp (0, 1500, 0, 230)
            .delay (quarter, 35, 17, 4000).level (-8).macro (1, "Bright").mod (macro1, "osc1Position", 40).macro (2, "Decay")
            .mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45).mod (macro3, "fxReverbMix", 45);
        add ("Bleak Pluck", "Pluck").osc (1, additive, 65).osc (2, analog, squarePos, 60).pitch (2, 7).filter (1, ladder24, 600, 40, 0, 50)
            .env (2, 0, 240, 0, 150, 85).mod (env2, "filter1Cutoff", 41).mod (velocity, "filter1Cutoff", 11).amp (0, 320, 0, 400)
            .delay (eighthD, 50, 24, 5000).reverb (15, 60, 2.5f, 45).level (-10).standardMacros (30);
        add ("Brutal Barb", "Pluck").osc (1, analog, sawPos).osc (2, analog, squarePos, 45).pitch (2, 0).filter (1, ladder24, 600, 35, 0, 50)
            .env (2, 0, 200, 0, 150, 85).mod (env2, "filter1Cutoff", 35).mod (velocity, "filter1Cutoff", 9).amp (0, 400, 0, 330)
            .delay (eighthD, 45, 20, 5500).reverb (20, 60, 2.5f, 45).warp (1, wCrush, 30).level (-10).standardMacros (30);
        add ("Brutal Pebble", "Pluck").osc (1, string, 50, 90).warp (1, wOff, 40).unison (1, 2, 3, 60, 4, 5).filter (1, svfLow, 7750, 10)
            .amp (0, 2300, 0, 230).delay (eighthD, 35, 21, 4000).level (-10).macro (1, "Bright").mod (macro1, "osc1Position", 40).macro (2, "Decay")
            .mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45).mod (macro3, "fxReverbMix", 45);
        add ("Brutal Tack", "Pluck").osc (1, string, 40, 90).warp (1, wOff, 25).filter (1, svfLow, 5000, 20).amp (0, 1400, 0, 230)
            .delay (eighth, 35, 16, 4000).drive (fuzz, 15, 5000).level (-19).macro (1, "Bright").mod (macro1, "osc1Position", 40).macro (2, "Decay")
            .mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45).mod (macro3, "fxReverbMix", 45);
        add ("Burnt Wire", "Pluck").osc (1, fm, 35).filter (1, ladder24, 400, 50, 0, 50).env (2, 0, 240, 0, 150, 85).mod (env2, "filter1Cutoff", 32)
            .mod (velocity, "filter1Cutoff", 11).amp (0, 300, 0, 280).delay (eighthD, 40, 19, 3000).level (-6).standardMacros (30);
        add ("Choked Tack", "Pluck").osc (1, pulse, 50).osc (2, analog, sawPos, 50).pitch (2, 0).filter (1, ladder24, 650, 30, 0, 50)
            .env (2, 0, 180, 0, 150, 85).mod (env2, "filter1Cutoff", 30).mod (velocity, "filter1Cutoff", 12).amp (0, 440, 0, 360)
            .delay (quarterT, 40, 21, 3000).level (-10).standardMacros (30);
        add ("Chrome Barb", "Pluck").osc (1, additive, 65).osc (2, analog, squarePos, 40).pitch (2, 12).filter (1, ladder24, 850, 25, 0, 50)
            .env (2, 0, 130, 0, 150, 85).mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 11).amp (0, 400, 0, 210)
            .delay (sixteenthD, 45, 25, 4000).reverb (20, 60, 2.5f, 45).level (-7).standardMacros (30);
        add ("Cold Spring", "Pluck").osc (1, analog, sawPos).filter (1, ladder24, 400, 40, 0, 50).env (2, 0, 170, 0, 150, 85)
            .mod (env2, "filter1Cutoff", 44).mod (velocity, "filter1Cutoff", 14).amp (0, 530, 0, 360).delay (quarterT, 30, 23, 5000)
            .reverb (25, 60, 2.5f, 45).level (-10).standardMacros (30);
        add ("Concrete Hook", "Pluck").osc (1, analog, sawPos).osc (2, analog, squarePos, 50).pitch (2, 0).filter (1, svfLow, 350, 25, 0, 50)
            .env (2, 0, 170, 0, 150, 85).mod (env2, "filter1Cutoff", 36).mod (velocity, "filter1Cutoff", 11).amp (0, 570, 0, 340)
            .delay (sixteenthD, 40, 23, 4500).warp (1, wCrush, 45).level (-10).standardMacros (30);
        add ("Dead Pebble", "Pluck").osc (1, pulse, 10).filter (1, ladder24, 750, 45, 0, 50).env (2, 0, 150, 0, 150, 85)
            .mod (env2, "filter1Cutoff", 44).mod (velocity, "filter1Cutoff", 9).amp (0, 360, 0, 350).delay (eighthD, 45, 18, 5000)
            .reverb (25, 60, 2.5f, 45).warp (1, wCrush, 30).level (-10).standardMacros (30);
        add ("Dead Shard", "Pluck").osc (1, metal, 25).osc (2, analog, sinePos, 50).pitch (2, 7).filter (1, ladder24, 700, 50, 0, 50)
            .env (2, 0, 210, 0, 150, 85).mod (env2, "filter1Cutoff", 37).mod (velocity, "filter1Cutoff", 13).amp (0, 530, 0, 380)
            .delay (sixteenthD, 45, 18, 5000).level (-6).standardMacros (30);
        add ("Derelict Wire", "Pluck").osc (1, string, 80, 90).warp (1, wOff, 30).unison (1, 2, 8, 50, 7, 10).filter (1, ladder24, 3750, 5)
            .amp (0, 2000, 0, 260).delay (eighth, 40, 18, 4000).drive (crush, 15, 5000).level (-3).macro (1, "Bright")
            .mod (macro1, "osc1Position", 40).macro (2, "Decay").mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45)
            .mod (macro3, "fxReverbMix", 45);
        add ("Dry Pin", "Pluck").osc (1, analog, sawPos).osc (2, analog, squarePos, 60).pitch (2, 0).filter (1, svfLow, 1100, 25, 0, 50)
            .env (2, 0, 250, 0, 150, 85).mod (env2, "filter1Cutoff", 33).mod (velocity, "filter1Cutoff", 14).amp (0, 420, 0, 330)
            .delay (quarterT, 45, 27, 3500).reverb (20, 60, 2.5f, 45).level (-12).standardMacros (30);
        add ("Dry Shard", "Pluck").osc (1, pulse, 40).osc (2, analog, sawPos, 50).pitch (2, 0).filter (1, ladder24, 1100, 40, 0, 50)
            .env (2, 0, 170, 0, 150, 85).mod (env2, "filter1Cutoff", 34).mod (velocity, "filter1Cutoff", 14).amp (0, 270, 0, 340)
            .delay (sixteenthD, 35, 26, 4000).warp (1, wCrush, 35).level (-10).standardMacros (30);
        add ("Feral Pin", "Pluck").osc (1, string, 50, 90).warp (1, wOff, 40).unison (1, 2, 6, 40, 3, 5).filter (1, ladder24, 7000, 5)
            .amp (0, 1600, 0, 230).delay (eighthD, 35, 22, 4000).level (-8).macro (1, "Bright").mod (macro1, "osc1Position", 40).macro (2, "Decay")
            .mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45).mod (macro3, "fxReverbMix", 45);
        add ("Frozen Shard", "Pluck").osc (1, fm, 20).filter (1, svfLow, 850, 50, 0, 50).env (2, 0, 170, 0, 150, 85).mod (env2, "filter1Cutoff", 39)
            .mod (velocity, "filter1Cutoff", 9).amp (0, 330, 0, 390).delay (quarterT, 45, 30, 5500).reverb (15, 60, 2.5f, 45).level (-10)
            .standardMacros (30);
        add ("Frozen Tine", "Pluck").osc (1, pulse, 15).osc (2, analog, sawPos, 45).pitch (2, 19).filter (1, ladder24, 550, 50, 0, 50)
            .env (2, 0, 240, 0, 150, 85).mod (env2, "filter1Cutoff", 35).mod (velocity, "filter1Cutoff", 12).amp (0, 460, 0, 210)
            .delay (sixteenthD, 45, 25, 3000).reverb (15, 60, 2.5f, 45).warp (1, wMirror, 30).level (-10).standardMacros (30);
        add ("Grey Nail", "Pluck").osc (1, metal, 65).filter (1, ladder24, 700, 45, 0, 50).env (2, 0, 200, 0, 150, 85).mod (env2, "filter1Cutoff", 41)
            .mod (velocity, "filter1Cutoff", 14).amp (0, 340, 0, 360).delay (eighthT, 50, 20, 4000).reverb (30, 60, 2.5f, 45).level (-4)
            .standardMacros (30);
        add ("Grey Pick", "Pluck").osc (1, string, 45, 90).warp (1, wOff, 50).filter (1, ladder24, 5250, 15).amp (0, 2100, 0, 390)
            .delay (quarter, 30, 24, 4000).drive (tube, 20, 5000).level (-10).macro (1, "Bright").mod (macro1, "osc1Position", 40).macro (2, "Decay")
            .mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45).mod (macro3, "fxReverbMix", 45);
        add ("Grey Pluck", "Pluck").osc (1, fm, 30).osc (2, analog, sinePos, 50).pitch (2, 0).filter (1, svfLow, 850, 25, 0, 50)
            .env (2, 0, 140, 0, 150, 85).mod (env2, "filter1Cutoff", 33).mod (velocity, "filter1Cutoff", 11).amp (0, 690, 0, 360)
            .delay (eighthT, 40, 24, 5500).reverb (20, 60, 2.5f, 45).level (-10).standardMacros (30);
        add ("Heavy Nail", "Pluck").osc (1, string, 65, 90).warp (1, wOff, 20).unison (1, 2, 4, 70, 4, 5).filter (1, ladder24, 5750, 10)
            .amp (0, 2300, 0, 340).delay (eighthD, 35, 18, 4000).drive (tube, 30, 5000).level (-12).macro (1, "Bright")
            .mod (macro1, "osc1Position", 40).macro (2, "Decay").mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45)
            .mod (macro3, "fxReverbMix", 45);
        add ("Hollow Hook", "Pluck").osc (1, analog, sawPos).osc (2, analog, sawPos, 55).pitch (2, 19).filter (1, ladder24, 1000, 50, 0, 50)
            .env (2, 0, 240, 0, 150, 85).mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 15).amp (0, 670, 0, 210)
            .delay (sixteenthD, 40, 19, 3000).level (-10).standardMacros (30);
        add ("Industrial Drop", "Pluck").osc (1, additive, 40).filter (1, ladder24, 1150, 20, 0, 50).env (2, 0, 230, 0, 150, 85)
            .mod (env2, "filter1Cutoff", 35).mod (velocity, "filter1Cutoff", 11).amp (0, 600, 0, 310).delay (quarterT, 35, 20, 4500).level (-10)
            .standardMacros (30);
        add ("Industrial Spring", "Pluck").osc (1, string, 80, 90).warp (1, wOff, 45).filter (1, svfLow, 4500, 10).amp (0, 1500, 0, 230)
            .delay (eighthD, 35, 24, 4000).drive (fuzz, 40, 5000).level (-21).macro (1, "Bright").mod (macro1, "osc1Position", 40).macro (2, "Decay")
            .mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45).mod (macro3, "fxReverbMix", 45);
        add ("Oily Tack", "Pluck").osc (1, string, 50, 90).warp (1, wOff, 50).unison (1, 3, 8, 55, 6, 15).filter (1, svfLow, 4750, 10)
            .amp (0, 1900, 0, 300).delay (quarter, 40, 23, 4000).drive (crush, 35, 5000).level (-10).macro (1, "Bright")
            .mod (macro1, "osc1Position", 40).macro (2, "Decay").mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45)
            .mod (macro3, "fxReverbMix", 45);
        add ("Oily Tine", "Pluck").osc (1, pulse, 35).filter (1, ladder24, 400, 35, 0, 50).env (2, 0, 230, 0, 150, 85).mod (env2, "filter1Cutoff", 30)
            .mod (velocity, "filter1Cutoff", 12).amp (0, 670, 0, 300).delay (eighthD, 35, 21, 4500).reverb (30, 60, 2.5f, 45).warp (1, wCrush, 45)
            .level (-10).standardMacros (30);
        add ("Pinged Shard", "Pluck").osc (1, pulse, 40).filter (1, ladder24, 900, 40, 0, 50).env (2, 0, 190, 0, 150, 85)
            .mod (env2, "filter1Cutoff", 32).mod (velocity, "filter1Cutoff", 12).amp (0, 510, 0, 380).delay (eighthT, 35, 29, 4500)
            .reverb (25, 60, 2.5f, 45).level (-10).standardMacros (30);
        add ("Pressed Staple", "Pluck").osc (1, additive, 55).osc (2, analog, sawPos, 55).pitch (2, 7).filter (1, ladder24, 350, 30, 0, 50)
            .env (2, 0, 140, 0, 150, 85).mod (env2, "filter1Cutoff", 36).mod (velocity, "filter1Cutoff", 13).amp (0, 540, 0, 360)
            .delay (quarterT, 35, 29, 3000).reverb (25, 60, 2.5f, 45).level (-8).standardMacros (30);
        add ("Ruined Nail", "Pluck").osc (1, string, 40, 90).warp (1, wOff, 40).filter (1, ladder24, 5000, 10).amp (0, 1600, 0, 340)
            .delay (eighth, 30, 18, 4000).level (-5).macro (1, "Bright").mod (macro1, "osc1Position", 40).macro (2, "Decay")
            .mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45).mod (macro3, "fxReverbMix", 45);
        add ("Rust Pebble", "Pluck").osc (1, additive, 60).osc (2, analog, squarePos, 55).pitch (2, 19).filter (1, ladder24, 550, 45, 0, 50)
            .env (2, 0, 160, 0, 150, 85).mod (env2, "filter1Cutoff", 36).mod (velocity, "filter1Cutoff", 9).amp (0, 560, 0, 220)
            .delay (eighthD, 40, 19, 4000).reverb (30, 60, 2.5f, 45).level (-10).standardMacros (30);
        add ("Rust Pin", "Pluck").osc (1, analog, sawPos).filter (1, svfLow, 900, 35, 0, 50).env (2, 0, 120, 0, 150, 85)
            .mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 13).amp (0, 580, 0, 340).delay (sixteenthD, 40, 19, 4500)
            .reverb (15, 60, 2.5f, 45).level (-10).standardMacros (30);
        add ("Scorched Splinter", "Pluck").osc (1, fm, 55).osc (2, analog, sinePos, 60).pitch (2, 0).filter (1, ladder24, 800, 30, 0, 50)
            .env (2, 0, 160, 0, 150, 85).mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 8).amp (0, 590, 0, 390)
            .delay (quarterT, 40, 23, 3500).warp (1, wMirror, 30).level (-10).standardMacros (30);
        add ("Severed Spring", "Pluck").osc (1, additive, 55).filter (1, svfLow, 400, 20, 0, 50).env (2, 0, 250, 0, 150, 85)
            .mod (env2, "filter1Cutoff", 39).mod (velocity, "filter1Cutoff", 8).amp (0, 290, 0, 260).delay (eighthT, 30, 26, 4000).level (-8)
            .standardMacros (30);
        add ("Shadow Barb", "Pluck").osc (1, additive, 25).osc (2, analog, sinePos, 55).pitch (2, 0).filter (1, ladder24, 1150, 45, 0, 50)
            .env (2, 0, 230, 0, 150, 85).mod (env2, "filter1Cutoff", 34).mod (velocity, "filter1Cutoff", 11).amp (0, 630, 0, 240)
            .delay (quarterT, 45, 24, 5000).level (-10).standardMacros (30);
        add ("Shadow Hook", "Pluck").osc (1, analog, sawPos).filter (1, ladder24, 500, 35, 0, 50).env (2, 0, 220, 0, 150, 85)
            .mod (env2, "filter1Cutoff", 34).mod (velocity, "filter1Cutoff", 12).amp (0, 520, 0, 300).delay (quarterT, 30, 28, 3000).level (-10)
            .standardMacros (30);
        add ("Shadow Wire", "Pluck").osc (1, string, 80, 90).warp (1, wOff, 55).filter (1, svfLow, 5500, 15).amp (0, 1600, 0, 360)
            .delay (eighth, 25, 21, 4000).level (-10).macro (1, "Bright").mod (macro1, "osc1Position", 40).macro (2, "Decay")
            .mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45).mod (macro3, "fxReverbMix", 45);
        add ("Snapped Pebble", "Pluck").osc (1, fm, 55).osc (2, analog, sawPos, 55).pitch (2, 19).filter (1, svfLow, 900, 25, 0, 50)
            .env (2, 0, 210, 0, 150, 85).mod (env2, "filter1Cutoff", 38).mod (velocity, "filter1Cutoff", 13).amp (0, 400, 0, 340)
            .delay (quarterT, 35, 23, 4500).reverb (25, 60, 2.5f, 45).level (-12).standardMacros (30);
        add ("Sodium Pin", "Pluck").osc (1, pulse, 25).filter (1, svfLow, 650, 25, 0, 50).env (2, 0, 160, 0, 150, 85).mod (env2, "filter1Cutoff", 35)
            .mod (velocity, "filter1Cutoff", 14).amp (0, 410, 0, 360).delay (eighthD, 50, 28, 4500).level (-12).standardMacros (30);
        add ("Sodium Twang", "Pluck").osc (1, string, 40, 90).warp (1, wOff, 40).unison (1, 3, 4, 50, 5, 10).filter (1, ladder24, 8000, 15)
            .amp (0, 1500, 0, 390).delay (eighth, 30, 18, 4000).drive (fuzz, 30, 5000).level (-20).macro (1, "Bright")
            .mod (macro1, "osc1Position", 40).macro (2, "Decay").mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45)
            .mod (macro3, "fxReverbMix", 45);
        add ("Soot Pick", "Pluck").osc (1, pulse, 65).filter (1, ladder24, 800, 30, 0, 50).env (2, 0, 130, 0, 150, 85).mod (env2, "filter1Cutoff", 38)
            .mod (velocity, "filter1Cutoff", 11).amp (0, 260, 0, 360).delay (eighthT, 50, 25, 4500).level (-8).standardMacros (30);
        add ("Soot Wire", "Pluck").osc (1, string, 70, 90).warp (1, wOff, 35).filter (1, ladder24, 6250, 15).amp (0, 1500, 0, 280)
            .delay (eighthD, 35, 23, 4000).drive (tube, 20, 5000).level (-10).macro (1, "Bright").mod (macro1, "osc1Position", 40).macro (2, "Decay")
            .mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45).mod (macro3, "fxReverbMix", 45);
        add ("Static Tack", "Pluck").osc (1, analog, sawPos).osc (2, analog, sawPos, 50).pitch (2, 0).filter (1, ladder24, 950, 45, 0, 50)
            .env (2, 0, 200, 0, 150, 85).mod (env2, "filter1Cutoff", 44).mod (velocity, "filter1Cutoff", 13).amp (0, 390, 0, 330)
            .delay (quarterT, 40, 18, 5500).warp (1, wCrush, 45).level (-8).standardMacros (30);
        add ("Sterile Tack", "Pluck").osc (1, string, 65, 90).warp (1, wOff, 25).unison (1, 2, 5, 65, 6, 15).filter (1, svfLow, 3750, 15)
            .amp (0, 1800, 0, 220).delay (eighth, 25, 16, 4000).drive (tube, 20, 5000).level (-10).macro (1, "Bright")
            .mod (macro1, "osc1Position", 40).macro (2, "Decay").mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45)
            .mod (macro3, "fxReverbMix", 45);
        add ("Sullen Pin", "Pluck").osc (1, analog, sawPos).filter (1, ladder24, 700, 45, 0, 50).env (2, 0, 230, 0, 150, 85)
            .mod (env2, "filter1Cutoff", 34).mod (velocity, "filter1Cutoff", 13).amp (0, 370, 0, 240).delay (eighthT, 35, 22, 4500)
            .reverb (30, 60, 2.5f, 45).level (-10).standardMacros (30);
        add ("Tapped Staple", "Pluck").osc (1, additive, 50).osc (2, analog, squarePos, 40).pitch (2, 19).filter (1, ladder24, 550, 50, 0, 50)
            .env (2, 0, 240, 0, 150, 85).mod (env2, "filter1Cutoff", 32).mod (velocity, "filter1Cutoff", 15).amp (0, 430, 0, 290)
            .delay (sixteenthD, 40, 28, 6000).warp (1, wFold, 25).level (-10).standardMacros (30);
        add ("Tar Staple", "Pluck").osc (1, string, 45, 90).warp (1, wOff, 30).unison (1, 2, 7, 60, 5, 5).filter (1, ladder24, 3000, 10)
            .amp (0, 1200, 0, 240).delay (eighth, 30, 16, 4000).drive (crush, 25, 5000).level (-3).macro (1, "Bright")
            .mod (macro1, "osc1Position", 40).macro (2, "Decay").mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45)
            .mod (macro3, "fxReverbMix", 45);
        add ("Tar Tine", "Pluck").osc (1, fm, 60).osc (2, analog, sinePos, 55).pitch (2, 12).filter (1, ladder24, 1150, 25, 0, 50)
            .env (2, 0, 210, 0, 150, 85).mod (env2, "filter1Cutoff", 41).mod (velocity, "filter1Cutoff", 9).amp (0, 400, 0, 270)
            .delay (sixteenthD, 40, 29, 4500).reverb (25, 60, 2.5f, 45).level (-10).standardMacros (30);
        add ("Thin Pebble", "Pluck").osc (1, analog, sawPos).osc (2, analog, sinePos, 50).pitch (2, 12).filter (1, ladder24, 1100, 25, 0, 50)
            .env (2, 0, 200, 0, 150, 85).mod (env2, "filter1Cutoff", 34).mod (velocity, "filter1Cutoff", 10).amp (0, 640, 0, 360)
            .delay (eighthT, 35, 26, 5500).reverb (15, 60, 2.5f, 45).level (-8).standardMacros (30);
        add ("Thin Prong", "Pluck").osc (1, string, 60, 90).warp (1, wOff, 30).filter (1, ladder24, 8000, 10).amp (0, 1800, 0, 350)
            .delay (eighthD, 35, 23, 4000).level (-7).macro (1, "Bright").mod (macro1, "osc1Position", 40).macro (2, "Decay")
            .mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45).mod (macro3, "fxReverbMix", 45);
        add ("Toxic Pebble", "Pluck").osc (1, pulse, 35).filter (1, svfLow, 400, 35, 0, 50).env (2, 0, 140, 0, 150, 85)
            .mod (env2, "filter1Cutoff", 30).mod (velocity, "filter1Cutoff", 10).amp (0, 540, 0, 390).delay (eighthT, 45, 29, 3500)
            .reverb (15, 60, 2.5f, 45).level (-12).standardMacros (30);
        add ("Toxic Prong", "Pluck").osc (1, string, 60, 90).warp (1, wOff, 50).filter (1, ladder24, 4500, 10).amp (0, 2400, 0, 250)
            .delay (quarter, 30, 21, 4000).level (-5).macro (1, "Bright").mod (macro1, "osc1Position", 40).macro (2, "Decay")
            .mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45).mod (macro3, "fxReverbMix", 45);
        add ("Void Barb", "Pluck").osc (1, metal, 40).osc (2, analog, sinePos, 45).pitch (2, 12).filter (1, ladder24, 1000, 40, 0, 50)
            .env (2, 0, 250, 0, 150, 85).mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 9).amp (0, 490, 0, 360)
            .delay (eighthT, 35, 25, 5000).reverb (30, 60, 2.5f, 45).level (-5).standardMacros (30);
        add ("Void Shard", "Pluck").osc (1, string, 55, 90).warp (1, wOff, 50).unison (1, 2, 7, 65, 6, 10).filter (1, svfLow, 4500, 5)
            .amp (0, 1900, 0, 330).delay (quarter, 35, 18, 4000).level (-10).macro (1, "Bright").mod (macro1, "osc1Position", 40).macro (2, "Decay")
            .mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45).mod (macro3, "fxReverbMix", 45);
        add ("Welded Pebble", "Pluck").osc (1, string, 40, 90).warp (1, wOff, 50).unison (1, 2, 4, 65, 5, 5).filter (1, ladder24, 3250, 15)
            .amp (0, 1600, 0, 360).delay (eighthD, 30, 19, 4000).level (-4).macro (1, "Bright").mod (macro1, "osc1Position", 40).macro (2, "Decay")
            .mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45).mod (macro3, "fxReverbMix", 45);
        add ("Welded Wire", "Pluck").osc (1, string, 70, 90).warp (1, wOff, 35).unison (1, 2, 7, 55, 8, 10).filter (1, svfLow, 3500, 10)
            .amp (0, 1800, 0, 380).delay (eighth, 35, 21, 4000).level (-8).macro (1, "Bright").mod (macro1, "osc1Position", 40).macro (2, "Decay")
            .mod (macro2, "osc1WarpAmount", 40).macro (3, "Space").reverb (0, 70, 3, 45).mod (macro3, "fxReverbMix", 45);
        add ("Zinc Hook", "Pluck").osc (1, fm, 30).filter (1, svfLow, 1150, 50, 0, 50).env (2, 0, 260, 0, 150, 85).mod (env2, "filter1Cutoff", 31)
            .mod (velocity, "filter1Cutoff", 11).amp (0, 420, 0, 400).delay (eighthT, 45, 22, 3500).level (-10).standardMacros (30);

        // Guitar, by name.
        add ("Basalt Buzz", "Guitar").osc (1, string, 75, 90).warp (1, wOff, 80).unison (1, 3, 5, 65, 39, 55).subOsc (25)
            .filter (1, ladderBand, 4500, 25, 5).drive (crush, 45, 5750).amp (2, 2400, 90, 550).reverb (20, 80, 5, 45).delay (quarter, 25, 15, 3500)
            .level (-7).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30)
            .macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Basalt Swarm", "Guitar").osc (1, string, 75, 90).warp (1, wOff, 75).unison (1, 5, 13, 85, 45, 35).osc (2, string, 60, 65)
            .warp (2, wOff, 88).pitch (2, -12).unison (2, 4, 17, 80, 37, 30).filter (1, ladderBand, 4250, 25).drive (fuzz, 60, 3750)
            .amp (0, 1900, 40, 500).reverb (30, 60, 2, 45).level (-27).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Bitter Riff", "Guitar").osc (1, string, 70, 90).warp (1, wOff, 88).unison (1, 3, 12, 95, 42, 35).filter (1, ladder24, 2500, 30, 10)
            .drive (hard, 45, 4000).amp (100, 1000, 100, 2800).lfo (1, drift, 0.08f, 0).mod (lfo1, "filter1Cutoff", 7).reverb (30, 60, 5, 45)
            .delay (eighthD, 30, 10, 3500).level (-17).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Bitter Roar", "Guitar").osc (1, string, 65, 90).warp (1, wOff, 25).unison (1, 8, 6, 95, 18, 40).subOsc (20)
            .filter (1, ladderBand, 4500, 30, 30).drive (hard, 45, 4750).amp (2, 2300, 0, 450).reverb (20, 80, 4.5f, 45).level (-19)
            .macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Black Roar", "Guitar").osc (1, string, 75, 90).warp (1, wOff, 90).unison (1, 6, 5, 85, 15, 45).osc (2, string, 70, 60)
            .warp (2, wOff, 93).pitch (2, -12).unison (2, 2, 15, 95, 38, 45).subOsc (10).filter (1, ladderBand, 5250, 30, 10).drive (tube, 75, 5750)
            .amp (300, 1000, 100, 2100).lfo (1, drift, 0.15f, 0).mod (lfo1, "filter1Cutoff", 6).reverb (35, 65, 2, 45).level (-21).macro (1, "Swarm")
            .mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Black Strings", "Guitar").osc (1, string, 80, 90).warp (1, wOff, 80).unison (1, 6, 5, 65, 25, 40).osc (2, string, 70, 70)
            .warp (2, wOff, 90).pitch (2, 12).unison (2, 2, 8, 70, 25, 55).filter (1, ladderBand, 5000, 25).drive (tube, 50, 4750)
            .amp (1, 2100, 40, 500).reverb (40, 60, 4, 45).level (-17).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Buried Swarm", "Guitar").osc (1, string, 60, 90).warp (1, wOff, 78).unison (1, 5, 4, 95, 48, 55).subOsc (10)
            .filter (1, svfBand, 2750, 25).drive (fuzz, 70, 4750).amp (1, 1300, 90, 400).reverb (30, 75, 2.5f, 45).level (-29).macro (1, "Swarm")
            .mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Burnt Fret", "Guitar").osc (1, string, 80, 90).warp (1, wOff, 40).unison (1, 3, 11, 95, 31, 50).osc (2, string, 70, 60)
            .warp (2, wOff, 20).pitch (2, -5).unison (2, 3, 14, 90, 21, 45).filter (1, ladderBand, 2750, 15, 5).drive (tube, 70, 4250)
            .amp (1, 1400, 0, 550).reverb (40, 75, 4, 45).level (-20).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Chrome Amp", "Guitar").osc (1, string, 65, 90).warp (1, wOff, 82).unison (1, 5, 16, 75, 35, 30).filter (1, svfBand, 3250, 30, 15)
            .drive (fuzz, 70, 3750).amp (1, 1900, 90, 350).reverb (20, 65, 3, 45).level (-29).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25)
            .macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Chrome Buzz", "Guitar").osc (1, string, 75, 90).warp (1, wOff, 85).unison (1, 6, 5, 75, 47, 50).osc (2, string, 65, 55)
            .warp (2, wOff, 72).pitch (2, -12).unison (2, 2, 12, 80, 17, 20).filter (1, ladder24, 4750, 35, 5).drive (hard, 80, 4500)
            .amp (1, 2300, 40, 450).reverb (40, 80, 5, 45).delay (quarter, 25, 19, 3500).level (-30).macro (1, "Swarm")
            .mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Cobalt Cabinet", "Guitar").osc (1, string, 80, 90).warp (1, wOff, 78).unison (1, 6, 10, 85, 23, 15).osc (2, string, 65, 75)
            .warp (2, wOff, 90).pitch (2, -5).unison (2, 3, 7, 90, 33, 15).filter (1, ladder24, 6000, 15, 5).drive (tube, 65, 4750)
            .amp (1, 2000, 90, 500).reverb (45, 75, 3, 45).level (-23).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Cobalt Neck", "Guitar").osc (1, string, 85, 90).warp (1, wOff, 98).unison (1, 8, 15, 100, 24, 25).osc (2, string, 75, 75)
            .warp (2, wOff, 95).pitch (2, 7).unison (2, 3, 11, 95, 11, 25).filter (1, svfBand, 4750, 20, 25).drive (fuzz, 55, 4250)
            .amp (100, 1000, 100, 1900).lfo (1, drift, 0.13f, 0).mod (lfo1, "filter1Cutoff", 7).reverb (45, 65, 6, 45).level (-31).macro (1, "Swarm")
            .mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Dead Swarm", "Guitar").osc (1, string, 50, 90).warp (1, wOff, 82).unison (1, 6, 5, 100, 35, 25).osc (2, string, 40, 75)
            .warp (2, wOff, 82).pitch (2, -12).unison (2, 4, 16, 75, 39, 50).filter (1, ladder24, 3500, 25, 10).drive (fuzz, 65, 5500)
            .amp (1, 1600, 90, 500).reverb (40, 80, 4.5f, 45).delay (eighthD, 40, 11, 3500).level (-32).macro (1, "Swarm")
            .mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Detuned Strum", "Guitar").osc (1, string, 55, 90).warp (1, wOff, 98).unison (1, 4, 11, 80, 18, 40).osc (2, string, 35, 60)
            .warp (2, wOff, 92).pitch (2, 7).unison (2, 4, 11, 85, 38, 20).filter (1, ladderBand, 5750, 20, 25).drive (crush, 55, 4250)
            .amp (150, 1000, 100, 1600).lfo (1, drift, 0.08f, 0).mod (lfo1, "filter1Cutoff", 7).reverb (25, 85, 5.5f, 45).level (0)
            .macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Distorted Roar", "Guitar").osc (1, string, 60, 90).warp (1, wOff, 90).unison (1, 8, 15, 75, 29, 20).osc (2, string, 55, 75)
            .warp (2, wOff, 75).pitch (2, -5).unison (2, 6, 8, 75, 49, 55).filter (1, ladder24, 2750, 30, 15).drive (tube, 60, 5000)
            .amp (1, 1700, 90, 300).reverb (35, 80, 4.5f, 45).delay (eighthD, 25, 16, 3500).level (-22).macro (1, "Swarm")
            .mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Droning Neck", "Guitar").osc (1, string, 50, 90).warp (1, wOff, 97).unison (1, 4, 6, 75, 26, 40).subOsc (25)
            .filter (1, ladder24, 5500, 20, 25).drive (tube, 55, 6000).amp (200, 1000, 100, 3000).lfo (1, drift, 0.08f, 0)
            .mod (lfo1, "filter1Cutoff", 10).reverb (45, 70, 3, 45).level (-19).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25)
            .macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Feral Amp", "Guitar").osc (1, string, 55, 90).warp (1, wOff, 95).unison (1, 5, 12, 80, 12, 15).osc (2, string, 45, 75)
            .warp (2, wOff, 98).pitch (2, -12).unison (2, 2, 14, 70, 26, 25).filter (1, ladder24, 4500, 15, 15).drive (hard, 70, 5750)
            .amp (400, 1000, 100, 2000).lfo (1, drift, 0.12f, 0).mod (lfo1, "filter1Cutoff", 5).reverb (20, 75, 3.5f, 45).level (-23)
            .macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Fuzzed Cabinet", "Guitar").osc (1, string, 55, 90).warp (1, wOff, 40).unison (1, 4, 12, 85, 29, 40).subOsc (25)
            .filter (1, ladder24, 2500, 25, 10).drive (hard, 50, 5500).amp (0, 1700, 0, 550).reverb (35, 85, 2.5f, 45).level (-19).macro (1, "Swarm")
            .mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Hollow Choir", "Guitar").osc (1, string, 65, 90).warp (1, wOff, 100).unison (1, 3, 12, 100, 16, 15).filter (1, svfBand, 4500, 35, 5)
            .drive (crush, 75, 4000).amp (300, 1000, 100, 2700).lfo (1, drift, 0.12f, 0).mod (lfo1, "filter1Cutoff", 7).reverb (35, 75, 5.5f, 45)
            .level (3).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30)
            .macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Iron Fret", "Guitar").osc (1, string, 80, 90).warp (1, wOff, 92).unison (1, 8, 10, 80, 38, 45).subOsc (15)
            .filter (1, ladder24, 6000, 15, 15).drive (fuzz, 65, 4500).amp (300, 1000, 100, 1900).lfo (1, drift, 0.12f, 0)
            .mod (lfo1, "filter1Cutoff", 5).reverb (35, 90, 5, 45).level (-30).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25)
            .macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Leaden Choir", "Guitar").osc (1, string, 50, 90).warp (1, wOff, 95).unison (1, 8, 9, 75, 41, 25).osc (2, string, 45, 55)
            .warp (2, wOff, 92).pitch (2, -12).unison (2, 3, 13, 75, 23, 55).subOsc (15).filter (1, ladder24, 5000, 35, 5).drive (fuzz, 70, 3750)
            .amp (150, 1000, 100, 3000).lfo (1, drift, 0.1f, 0).mod (lfo1, "filter1Cutoff", 9).reverb (30, 70, 6, 45).level (-31).macro (1, "Swarm")
            .mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Leaden Roar", "Guitar").osc (1, string, 80, 90).warp (1, wOff, 25).unison (1, 8, 7, 80, 37, 45).osc (2, string, 70, 70).warp (2, wOff, 20)
            .pitch (2, 7).unison (2, 3, 12, 90, 13, 30).filter (1, ladder24, 3250, 10, 10).drive (crush, 55, 5500).amp (1, 1300, 0, 400)
            .reverb (20, 75, 3, 45).level (-4).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Leaden Strings", "Guitar").osc (1, string, 65, 90).warp (1, wOff, 93).unison (1, 5, 12, 90, 42, 20).osc (2, string, 50, 55)
            .warp (2, wOff, 87).pitch (2, -12).unison (2, 2, 16, 80, 18, 25).filter (1, ladder24, 3000, 30, 10).drive (hard, 70, 5500)
            .amp (200, 1000, 100, 2600).lfo (1, drift, 0.06f, 0).mod (lfo1, "filter1Cutoff", 9).reverb (45, 80, 2.5f, 45).level (-24)
            .macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Nickel Guitar", "Guitar").osc (1, string, 75, 90).warp (1, wOff, 78).unison (1, 8, 10, 90, 38, 40).filter (1, svfBand, 4500, 15, 10)
            .drive (crush, 60, 4750).amp (1, 1600, 90, 600).reverb (25, 70, 2.5f, 45).delay (quarter, 35, 10, 3500).level (-6).macro (1, "Swarm")
            .mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Null Choir", "Guitar").osc (1, string, 75, 90).warp (1, wOff, 88).unison (1, 6, 12, 95, 22, 45).osc (2, string, 60, 75)
            .warp (2, wOff, 90).pitch (2, -12).unison (2, 3, 15, 90, 30, 50).filter (1, svfBand, 4000, 10).drive (fuzz, 45, 5750)
            .amp (2, 1700, 90, 400).reverb (35, 60, 2, 45).level (-30).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Ruined Cabinet", "Guitar").osc (1, string, 60, 90).warp (1, wOff, 80).unison (1, 8, 11, 60, 39, 25).subOsc (20)
            .filter (1, ladder24, 4750, 25, 25).drive (crush, 75, 4250).amp (1, 1900, 90, 400).reverb (25, 75, 2.5f, 45).level (-8)
            .macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Rust Choir", "Guitar").osc (1, string, 80, 90).warp (1, wOff, 95).unison (1, 3, 8, 95, 39, 45).osc (2, string, 70, 60)
            .warp (2, wOff, 100).pitch (2, 7).unison (2, 4, 17, 75, 38, 35).subOsc (25).filter (1, svfBand, 3500, 20, 10).drive (tube, 65, 6000)
            .amp (250, 1000, 100, 2600).lfo (1, drift, 0.06f, 0).mod (lfo1, "filter1Cutoff", 5).reverb (45, 80, 3, 45).delay (quarter, 35, 18, 3500)
            .level (-19).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30)
            .macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Shadow Buzz", "Guitar").osc (1, string, 50, 90).warp (1, wOff, 95).unison (1, 6, 11, 85, 9, 20).osc (2, string, 40, 70)
            .warp (2, wOff, 92).pitch (2, 7).unison (2, 2, 13, 65, 31, 55).filter (1, ladder24, 3500, 30, 5).drive (crush, 60, 5000)
            .amp (150, 1000, 100, 1800).lfo (1, drift, 0.09f, 0).mod (lfo1, "filter1Cutoff", 10).reverb (20, 70, 2.5f, 45).level (-3)
            .macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Soot Storm", "Guitar").osc (1, string, 55, 90).warp (1, wOff, 85).unison (1, 5, 14, 80, 22, 35).osc (2, string, 40, 60)
            .warp (2, wOff, 90).pitch (2, -12).unison (2, 2, 17, 60, 31, 35).filter (1, ladderBand, 4500, 30).drive (fuzz, 60, 5750)
            .amp (1, 2200, 90, 300).reverb (25, 80, 2, 45).delay (eighthD, 35, 15, 3500).level (-28).macro (1, "Swarm")
            .mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Steel Buzz", "Guitar").osc (1, string, 60, 90).warp (1, wOff, 93).unison (1, 6, 10, 70, 42, 10).filter (1, ladderBand, 4750, 30, 10)
            .drive (hard, 80, 4250).amp (400, 1000, 100, 2900).lfo (1, drift, 0.07f, 0).mod (lfo1, "filter1Cutoff", 8).reverb (35, 75, 5, 45)
            .level (-21).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30)
            .macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Steel Guitar", "Guitar").osc (1, string, 80, 90).warp (1, wOff, 82).unison (1, 6, 7, 90, 19, 55).osc (2, string, 75, 60)
            .warp (2, wOff, 78).pitch (2, -12).unison (2, 3, 11, 70, 16, 40).filter (1, ladderBand, 2750, 25, 5).drive (tube, 65, 3750)
            .amp (2, 2000, 90, 550).reverb (25, 80, 4, 45).level (-22).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Sterile Wall", "Guitar").osc (1, string, 70, 90).warp (1, wOff, 40).unison (1, 8, 4, 75, 21, 45).osc (2, string, 50, 60)
            .warp (2, wOff, 40).pitch (2, 12).unison (2, 2, 9, 70, 15, 30).filter (1, ladderBand, 5500, 10).drive (crush, 50, 4750)
            .amp (0, 2100, 0, 500).reverb (30, 85, 2.5f, 45).level (-4).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz")
            .mod (macro2, "fxDriveAmount", 30).macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Sunken Amp", "Guitar").osc (1, string, 80, 90).warp (1, wOff, 92).unison (1, 5, 14, 100, 24, 15).subOsc (20)
            .filter (1, ladderBand, 4000, 35, 30).drive (hard, 50, 4250).amp (1, 1400, 40, 350).reverb (20, 90, 4.5f, 45).level (-23)
            .macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Sunken Guitar", "Guitar").osc (1, string, 70, 90).warp (1, wOff, 30).unison (1, 5, 7, 80, 9, 20).filter (1, ladder24, 2750, 10, 10)
            .drive (crush, 70, 5000).amp (2, 2200, 0, 450).reverb (35, 75, 4, 45).level (-4).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25)
            .macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Swarming Drone", "Guitar").osc (1, string, 50, 90).warp (1, wOff, 88).unison (1, 5, 11, 80, 18, 40).osc (2, string, 45, 70)
            .warp (2, wOff, 93).pitch (2, 7).unison (2, 4, 11, 75, 15, 40).subOsc (10).filter (1, ladderBand, 5750, 25, 5).drive (hard, 50, 3750)
            .amp (350, 1000, 100, 2600).lfo (1, drift, 0.14f, 0).mod (lfo1, "filter1Cutoff", 4).reverb (25, 75, 4.5f, 45).level (-4)
            .macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Unison Buzz", "Guitar").osc (1, string, 60, 90).warp (1, wOff, 98).unison (1, 4, 8, 75, 30, 15).osc (2, string, 50, 65)
            .warp (2, wOff, 90).pitch (2, 12).unison (2, 2, 9, 80, 11, 55).filter (1, svfBand, 5000, 10, 5).drive (crush, 60, 4250)
            .amp (350, 1000, 100, 2900).lfo (1, drift, 0.09f, 0).mod (lfo1, "filter1Cutoff", 7).reverb (20, 80, 4.5f, 45).level (3)
            .macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Void Amp", "Guitar").osc (1, string, 70, 90).warp (1, wOff, 25).unison (1, 4, 15, 90, 37, 15).subOsc (20)
            .filter (1, svfBand, 5750, 20, 25).drive (hard, 45, 5750).amp (2, 2000, 0, 500).reverb (30, 80, 4.5f, 45).level (-19).macro (1, "Swarm")
            .mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Void Storm", "Guitar").osc (1, string, 65, 90).warp (1, wOff, 10).unison (1, 8, 4, 95, 11, 25).filter (1, ladder24, 5500, 35, 20)
            .drive (fuzz, 55, 5250).amp (0, 1800, 0, 550).reverb (35, 60, 4, 45).level (-20).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25)
            .macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);
        add ("Void Swarm", "Guitar").osc (1, string, 50, 90).warp (1, wOff, 25).unison (1, 4, 4, 100, 14, 55).osc (2, string, 35, 70)
            .warp (2, wOff, 10).pitch (2, -12).unison (2, 4, 13, 85, 48, 45).filter (1, ladder24, 4750, 20, 5).drive (hard, 55, 5750)
            .amp (1, 1800, 0, 550).reverb (40, 80, 4, 45).delay (quarter, 35, 17, 3500).level (-20).macro (1, "Swarm")
            .mod (macro1, "osc1UnisonDetune", 25).macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright")
            .mod (macro3, "osc1Position", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 40);
        add ("Welded Storm", "Guitar").osc (1, string, 70, 90).warp (1, wOff, 92).unison (1, 4, 7, 65, 14, 30).subOsc (10)
            .filter (1, ladderBand, 4000, 30, 5).drive (tube, 80, 5500).amp (200, 1000, 100, 2900).lfo (1, drift, 0.07f, 0)
            .mod (lfo1, "filter1Cutoff", 6).reverb (15, 75, 4.5f, 45).level (-21).macro (1, "Swarm").mod (macro1, "osc1UnisonDetune", 25)
            .macro (2, "Fuzz").mod (macro2, "fxDriveAmount", 30).macro (3, "Bright").mod (macro3, "osc1Position", 30).macro (4, "Space")
            .mod (macro4, "fxReverbMix", 40);

        // Pad, by name.
        add ("Acid Embers", "Pad").osc (1, fm, 15).unison (1, 6, 13, 95).osc (2, vowel, 25, 60).pitch (2, 0, 9).unison (2, 3, 9, 90).noise (9, 1, -40)
            .filter (1, svfLow, 2300, 20).lfo (1, sine, 0.06f, 0).mod (lfo1, "osc1Position", 16).amp (1700, 1000, 95, 3400).reverb (40, 90, 5, 45)
            .level (-14).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Acid Field", "Pad").osc (1, analog, sawPos).unison (1, 6, 17, 90).filter (1, svfLow, 1200, 10).lfo (1, drift, 0.18f, 0)
            .mod (lfo1, "filter1Cutoff", 18).amp (1900, 1000, 85, 3500).reverb (35, 95, 4, 45).level (-9).standardMacros (25).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Acid Smoke", "Pad").osc (1, metal, 20).unison (1, 3, 11, 90).osc (2, additive, 55, 70).pitch (2, 7, 9).filter (1, ladder24, 2200, 25)
            .lfo (1, triangle, 0.13f, 0).mod (lfo1, "osc1Position", 13).amp (1200, 1000, 95, 1700).chorus (35, 0.6f, 50).phaser (40, 0.1f, 50)
            .reverb (35, 95, 7.5f, 45).level (-3).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Ash Haze", "Pad").osc (1, analog, sawPos).unison (1, 6, 21, 90).osc (2, pulse, 25, 40).pitch (2, -12, 5).unison (2, 2, 16, 90)
            .filter (1, ladder12, 2500, 20).lfo (1, triangle, 0.15f, 0).mod (lfo1, "filter1Cutoff", 20).amp (1100, 1000, 90, 2000)
            .chorus (35, 0.6f, 50).reverb (40, 90, 7, 45).level (-9).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Basalt Smog", "Pad").osc (1, pulse, 65).unison (1, 4, 16, 95).noise (7, 1, -40).filter (1, ladder12, 2000, 15).lfo (1, sine, 0.1f, 0)
            .mod (lfo1, "osc1Position", 22).amp (1600, 1000, 90, 2100).phaser (25, 0.05f, 50).reverb (45, 85, 5.5f, 45).level (-8)
            .standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Bitter Field", "Pad").osc (1, additive, 25).unison (1, 3, 13, 85).osc (2, metal, 15, 55).pitch (2, -12, 7)
            .filter (1, ladder12, 1200, 30).lfo (1, triangle, 0.13f, 0).mod (lfo1, "osc1Position", 16).amp (1800, 1000, 85, 3000)
            .chorus (40, 0.6f, 50).reverb (50, 95, 7, 45).level (-8).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Black Embers", "Pad").osc (1, analog, sawPos).unison (1, 6, 13, 85).osc (2, metal, 20, 65).pitch (2, -12, 6).unison (2, 4, 9, 90)
            .filter (1, svfLow, 1900, 10).lfo (1, triangle, 0.16f, 0).mod (lfo1, "filter1Cutoff", 9).amp (1500, 1000, 95, 1700).chorus (40, 0.6f, 50)
            .reverb (40, 90, 5.5f, 45).level (-10).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Black Horizon", "Pad").osc (1, vowel, 50).unison (1, 3, 18, 85).osc (2, metal, 55, 65).pitch (2, 7, 5).unison (2, 2, 17, 90)
            .filter (1, svfLow, 1200, 20).lfo (1, drift, 0.17f, 0).mod (lfo1, "osc1Position", 11).amp (1000, 1000, 90, 2000).reverb (35, 90, 7.5f, 45)
            .level (-12).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Black Ruins", "Pad").osc (1, analog, sawPos).unison (1, 3, 13, 90).noise (8, 1, -40).filter (1, svfLow, 1000, 20)
            .lfo (1, sine, 0.18f, 0).mod (lfo1, "filter1Cutoff", 12).amp (1700, 1000, 95, 2400).reverb (40, 100, 8, 45).level (-12)
            .standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Bleak Ceiling", "Pad").osc (1, metal, 40).unison (1, 6, 18, 80).osc (2, vowel, 35, 45).pitch (2, 0, 5).unison (2, 3, 14, 90)
            .filter (1, ladder24, 2400, 15).lfo (1, drift, 0.15f, 0).mod (lfo1, "osc1Position", 19).amp (1300, 1000, 95, 3500)
            .reverb (45, 95, 6.5f, 45).level (-6).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Bleak Smoke", "Pad").osc (1, metal, 50).unison (1, 6, 13, 90).filter (1, ladder24, 1700, 25).lfo (1, drift, 0.19f, 0)
            .mod (lfo1, "osc1Position", 13).amp (1400, 1000, 90, 1700).reverb (45, 95, 8, 45).level (-4).standardMacros (25).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Buried Ruins", "Pad").osc (1, analog, sawPos).unison (1, 6, 13, 95).osc (2, vowel, 50, 50).pitch (2, 0, 7).unison (2, 2, 9, 90)
            .filter (1, svfLow, 1400, 15).lfo (1, triangle, 0.19f, 0).mod (lfo1, "filter1Cutoff", 10).amp (1300, 1000, 95, 1900)
            .reverb (45, 95, 8.5f, 45).level (-14).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Burnt Dusk", "Pad").osc (1, vowel, 55).unison (1, 5, 19, 95).filter (1, ladder12, 1900, 10).lfo (1, sine, 0.1f, 0)
            .mod (lfo1, "osc1Position", 13).amp (1900, 1000, 90, 2700).chorus (40, 0.6f, 50).reverb (50, 90, 6, 45).level (-7).standardMacros (25)
            .macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Burnt Horizon", "Pad").osc (1, pulse, 40).unison (1, 5, 12, 80).osc (2, vowel, 25, 40).pitch (2, 12, 10).filter (1, svfLow, 2200, 25)
            .lfo (1, triangle, 0.21f, 0).mod (lfo1, "osc1Position", 22).amp (1300, 1000, 95, 2800).chorus (35, 0.6f, 50).reverb (35, 95, 6.5f, 45)
            .level (-12).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Carbon Horizon", "Pad").osc (1, metal, 65).unison (1, 4, 21, 80).osc (2, metal, 25, 60).pitch (2, 0, 5).filter (1, ladder12, 2300, 15)
            .lfo (1, drift, 0.22f, 0).mod (lfo1, "osc1Position", 18).amp (1200, 1000, 85, 2600).phaser (40, 0.06f, 50).reverb (50, 100, 5.5f, 45)
            .level (-5).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Carbon Sky", "Pad").osc (1, analog, sawPos).unison (1, 3, 10, 95).osc (2, analog, sawPos, 55).pitch (2, -12, 10)
            .filter (1, svfLow, 1000, 20).lfo (1, triangle, 0.09f, 0).mod (lfo1, "filter1Cutoff", 14).amp (1000, 1000, 95, 1900)
            .reverb (50, 95, 4, 45).level (-12).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Chrome Horizon", "Pad").osc (1, analog, sawPos).unison (1, 3, 11, 90).osc (2, vowel, 35, 55).pitch (2, 7, 11).unison (2, 2, 13, 90)
            .filter (1, ladder24, 1700, 10).lfo (1, triangle, 0.05f, 0).mod (lfo1, "filter1Cutoff", 21).amp (1300, 1000, 95, 1800)
            .phaser (30, 0.14f, 50).reverb (50, 85, 5, 45).level (-10).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Cinder Smog", "Pad").osc (1, growl, 55).unison (1, 3, 14, 100).osc (2, pulse, 20, 40).pitch (2, -12, 6).unison (2, 4, 10, 90)
            .noise (9, 1, -40).filter (1, ladder24, 1100, 25).lfo (1, drift, 0.08f, 0).mod (lfo1, "osc1Position", 12).amp (1300, 1000, 85, 3100)
            .phaser (25, 0.14f, 50).reverb (45, 80, 6, 45).level (-9).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Cobalt Atrium", "Pad").osc (1, additive, 10).unison (1, 3, 12, 100).osc (2, pulse, 60, 60).pitch (2, 0, 5).unison (2, 3, 10, 90)
            .filter (1, ladder24, 2200, 20).lfo (1, triangle, 0.09f, 0).mod (lfo1, "osc1Position", 21).amp (900, 1000, 95, 3400).chorus (35, 0.6f, 50)
            .phaser (30, 0.07f, 50).reverb (50, 90, 8.5f, 45).level (-9).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Cobalt Cloud", "Pad").osc (1, analog, sawPos).unison (1, 4, 19, 90).osc (2, analog, sawPos, 50).pitch (2, 12, 9)
            .filter (1, svfLow, 1600, 15).lfo (1, drift, 0.21f, 0).mod (lfo1, "filter1Cutoff", 12).amp (1000, 1000, 95, 1700).chorus (35, 0.6f, 50)
            .reverb (50, 90, 4.5f, 45).level (-10).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Cobalt Snow", "Pad").osc (1, analog, sawPos).unison (1, 6, 16, 85).osc (2, vowel, 45, 50).pitch (2, -12, 5).filter (1, svfLow, 700, 20)
            .lfo (1, drift, 0.08f, 0).mod (lfo1, "filter1Cutoff", 15).amp (600, 1000, 100, 1700).phaser (40, 0.14f, 50).reverb (45, 85, 5, 45)
            .level (-10).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Cold Sky", "Pad").osc (1, analog, sawPos).unison (1, 4, 18, 100).filter (1, ladder24, 2000, 20).lfo (1, triangle, 0.19f, 0)
            .mod (lfo1, "filter1Cutoff", 8).amp (1200, 1000, 85, 2300).reverb (50, 90, 8.5f, 45).level (-12).standardMacros (25).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Dead Ceiling", "Pad").osc (1, additive, 50).unison (1, 3, 18, 85).osc (2, metal, 35, 45).pitch (2, 0, 7).filter (1, ladder12, 700, 20)
            .lfo (1, sine, 0.06f, 0).mod (lfo1, "osc1Position", 10).amp (1200, 1000, 90, 2900).reverb (45, 85, 6, 45).level (-12).standardMacros (25)
            .macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Distant Horizon", "Pad").osc (1, pulse, 45).unison (1, 5, 12, 90).osc (2, additive, 30, 50).pitch (2, 7, 7).unison (2, 2, 11, 90)
            .filter (1, svfLow, 2200, 10).lfo (1, sine, 0.1f, 0).mod (lfo1, "osc1Position", 13).amp (1500, 1000, 90, 2700).reverb (40, 80, 4, 45)
            .level (-12).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Dusk Ceiling", "Pad").osc (1, additive, 60).unison (1, 4, 12, 80).osc (2, vowel, 10, 55).pitch (2, 7, 7).filter (1, ladder24, 2300, 20)
            .lfo (1, drift, 0.11f, 0).mod (lfo1, "osc1Position", 9).amp (1800, 1000, 100, 3300).reverb (55, 95, 5, 45).level (-10).standardMacros (25)
            .macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Dusk Haze", "Pad").osc (1, fm, 20).unison (1, 5, 11, 95).filter (1, svfLow, 1500, 25).lfo (1, sine, 0.12f, 0)
            .mod (lfo1, "osc1Position", 14).amp (1600, 1000, 90, 3200).chorus (25, 0.6f, 50).reverb (50, 85, 7.5f, 45).level (-12).standardMacros (25)
            .macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Endless Field", "Pad").osc (1, metal, 65).unison (1, 4, 16, 85).osc (2, additive, 15, 65).pitch (2, 7, 7).unison (2, 3, 15, 90)
            .filter (1, ladder24, 2200, 25).lfo (1, sine, 0.13f, 0).mod (lfo1, "osc1Position", 16).amp (700, 1000, 95, 2100).phaser (30, 0.13f, 50)
            .reverb (45, 95, 5.5f, 45).level (-8).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Endless Pad", "Pad").osc (1, fm, 45).unison (1, 5, 18, 85).osc (2, pulse, 50, 45).pitch (2, 12, 5).filter (1, ladder12, 1700, 30)
            .lfo (1, drift, 0.06f, 0).mod (lfo1, "osc1Position", 11).amp (1300, 1000, 95, 3300).reverb (55, 95, 7, 45).level (-12).standardMacros (25)
            .macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Fading Dusk", "Pad").osc (1, fm, 30).unison (1, 6, 22, 95).osc (2, pulse, 50, 40).pitch (2, 12, 10).unison (2, 2, 9, 90)
            .filter (1, svfLow, 1200, 25).lfo (1, drift, 0.09f, 0).mod (lfo1, "osc1Position", 16).amp (1400, 1000, 85, 3300).chorus (40, 0.6f, 50)
            .reverb (45, 80, 5.5f, 45).level (-12).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Fading Glow", "Pad").osc (1, metal, 20).unison (1, 4, 16, 85).osc (2, additive, 55, 65).pitch (2, 12, 7).filter (1, ladder12, 1900, 20)
            .lfo (1, drift, 0.09f, 0).mod (lfo1, "osc1Position", 8).amp (500, 1000, 90, 3100).phaser (40, 0.05f, 50).reverb (45, 95, 8.5f, 45)
            .level (-7).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Fading Veil", "Pad").osc (1, additive, 20).unison (1, 5, 12, 95).filter (1, svfLow, 1900, 15).lfo (1, triangle, 0.09f, 0)
            .mod (lfo1, "osc1Position", 16).amp (1600, 1000, 95, 3000).reverb (45, 100, 5.5f, 45).level (-12).standardMacros (25).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Feral Shroud", "Pad").osc (1, pulse, 35).unison (1, 5, 16, 90).osc (2, analog, sawPos, 40).pitch (2, 7, 8).noise (13, 1, -40)
            .filter (1, ladder12, 1900, 20).lfo (1, sine, 0.22f, 0).mod (lfo1, "osc1Position", 9).amp (900, 1000, 95, 1800).reverb (45, 80, 6, 45)
            .level (-12).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Feral Tundra", "Pad").osc (1, additive, 30).unison (1, 4, 13, 90).filter (1, svfLow, 2200, 15).lfo (1, triangle, 0.09f, 0)
            .mod (lfo1, "osc1Position", 19).amp (600, 1000, 95, 2900).reverb (50, 90, 8, 45).level (-12).standardMacros (25).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Grey Embers", "Pad").osc (1, analog, sawPos).unison (1, 3, 12, 90).noise (13, 1, -40).filter (1, ladder12, 2300, 25)
            .lfo (1, drift, 0.18f, 0).mod (lfo1, "filter1Cutoff", 20).amp (700, 1000, 90, 1800).chorus (35, 0.6f, 50).reverb (50, 80, 5.5f, 45)
            .level (-9).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Grey Pad", "Pad").osc (1, additive, 70).unison (1, 4, 14, 85).filter (1, svfLow, 1200, 10).lfo (1, drift, 0.1f, 0)
            .mod (lfo1, "osc1Position", 13).amp (1300, 1000, 90, 1700).phaser (35, 0.09f, 50).reverb (55, 80, 6.5f, 45).level (-8)
            .standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Hazy Dusk", "Pad").osc (1, growl, 10).unison (1, 5, 16, 95).osc (2, metal, 45, 55).pitch (2, 0, 4).noise (13, 1, -40)
            .filter (1, ladder24, 1800, 10).lfo (1, sine, 0.19f, 0).mod (lfo1, "osc1Position", 9).amp (1200, 1000, 95, 3300).reverb (45, 100, 8, 45)
            .level (-14).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Heavy Atrium", "Pad").osc (1, analog, sawPos).unison (1, 6, 19, 80).filter (1, svfLow, 1700, 25).lfo (1, triangle, 0.24f, 0)
            .mod (lfo1, "filter1Cutoff", 10).amp (1700, 1000, 95, 3300).chorus (30, 0.6f, 50).reverb (40, 85, 7, 45).level (-12).standardMacros (25)
            .macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Hollow Hall", "Pad").osc (1, growl, 50).unison (1, 4, 15, 90).osc (2, metal, 35, 70).pitch (2, 12, 10).unison (2, 3, 15, 90)
            .filter (1, ladder12, 800, 20).lfo (1, sine, 0.21f, 0).mod (lfo1, "osc1Position", 13).amp (1400, 1000, 85, 2000).reverb (50, 95, 6.5f, 45)
            .level (-12).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Hollow Sky", "Pad").osc (1, growl, 50).unison (1, 6, 19, 85).osc (2, analog, sawPos, 55).pitch (2, 7, 4).unison (2, 3, 17, 90)
            .filter (1, svfLow, 1700, 25).lfo (1, drift, 0.14f, 0).mod (lfo1, "osc1Position", 23).amp (1200, 1000, 90, 2300).phaser (30, 0.05f, 50)
            .reverb (55, 85, 7.5f, 45).level (-12).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Industrial Shroud", "Pad").osc (1, metal, 15).unison (1, 3, 11, 80).noise (14, 1, -40).filter (1, ladder24, 1900, 20)
            .lfo (1, triangle, 0.06f, 0).mod (lfo1, "osc1Position", 24).amp (900, 1000, 95, 3400).chorus (25, 0.6f, 50).phaser (35, 0.08f, 50)
            .reverb (55, 100, 8.5f, 45).level (0).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Industrial Sky", "Pad").osc (1, additive, 20).unison (1, 5, 15, 90).osc (2, analog, sawPos, 40).pitch (2, 12, 9).unison (2, 2, 12, 90)
            .filter (1, ladder24, 1000, 10).lfo (1, drift, 0.25f, 0).mod (lfo1, "osc1Position", 17).amp (1800, 1000, 95, 3300)
            .reverb (40, 80, 6.5f, 45).level (-12).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Iron Dusk", "Pad").osc (1, analog, sawPos).unison (1, 3, 16, 100).osc (2, metal, 30, 65).pitch (2, 12, 5).unison (2, 2, 9, 90)
            .filter (1, ladder24, 600, 30).lfo (1, triangle, 0.06f, 0).mod (lfo1, "filter1Cutoff", 15).amp (1300, 1000, 90, 3300)
            .reverb (45, 100, 5, 45).level (-10).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Iron Sky", "Pad").osc (1, growl, 35).unison (1, 3, 18, 90).noise (6, 1, -40).filter (1, ladder12, 2500, 25).lfo (1, triangle, 0.11f, 0)
            .mod (lfo1, "osc1Position", 23).amp (1900, 1000, 100, 2100).chorus (40, 0.6f, 50).reverb (40, 90, 5, 45).level (-8).standardMacros (25)
            .macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Iron Snow", "Pad").osc (1, metal, 55).unison (1, 5, 16, 100).osc (2, metal, 60, 55).pitch (2, 12, 5).filter (1, ladder24, 1200, 25)
            .lfo (1, triangle, 0.17f, 0).mod (lfo1, "osc1Position", 12).amp (1300, 1000, 85, 1800).reverb (55, 100, 4, 45).level (-4)
            .standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Iron Veil", "Pad").osc (1, additive, 55).unison (1, 3, 18, 85).noise (5, 1, -40).filter (1, ladder12, 1600, 10).lfo (1, drift, 0.21f, 0)
            .mod (lfo1, "osc1Position", 11).amp (1200, 1000, 90, 1900).reverb (50, 90, 6.5f, 45).level (-12).standardMacros (25).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Mournful Fog", "Pad").osc (1, vowel, 35).unison (1, 3, 21, 85).osc (2, vowel, 45, 60).pitch (2, 12, 7).filter (1, ladder24, 2500, 15)
            .lfo (1, triangle, 0.11f, 0).mod (lfo1, "osc1Position", 24).amp (1800, 1000, 90, 3200).reverb (50, 95, 7.5f, 45).level (-9)
            .standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Mournful Hall", "Pad").osc (1, analog, sawPos).unison (1, 6, 17, 95).noise (11, 1, -40).filter (1, ladder12, 1300, 30)
            .lfo (1, triangle, 0.13f, 0).mod (lfo1, "filter1Cutoff", 23).amp (1500, 1000, 90, 3000).chorus (25, 0.6f, 50).phaser (40, 0.14f, 50)
            .reverb (45, 95, 5.5f, 45).level (-6).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Mute Sky", "Pad").osc (1, analog, sawPos).unison (1, 5, 21, 80).osc (2, analog, sawPos, 45).pitch (2, 7, 5)
            .filter (1, ladder24, 1600, 25).lfo (1, triangle, 0.21f, 0).mod (lfo1, "filter1Cutoff", 15).amp (800, 1000, 90, 3100)
            .reverb (55, 90, 7.5f, 45).level (-12).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Nickel Sky", "Pad").osc (1, analog, sawPos).unison (1, 5, 18, 85).osc (2, additive, 50, 45).pitch (2, 7, 11)
            .filter (1, ladder24, 1500, 30).lfo (1, triangle, 0.17f, 0).mod (lfo1, "filter1Cutoff", 21).amp (800, 1000, 90, 1800)
            .phaser (30, 0.15f, 50).reverb (45, 90, 5.5f, 45).level (-10).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Nickel Tundra", "Pad").osc (1, analog, sawPos).unison (1, 5, 15, 100).osc (2, additive, 40, 50).pitch (2, 7, 7).unison (2, 4, 13, 90)
            .filter (1, svfLow, 700, 15).lfo (1, sine, 0.07f, 0).mod (lfo1, "filter1Cutoff", 19).amp (1200, 1000, 100, 3000).reverb (35, 95, 5.5f, 45)
            .level (-12).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Obsidian Sky", "Pad").osc (1, analog, sawPos).unison (1, 5, 17, 100).osc (2, vowel, 10, 55).pitch (2, 12, 8).noise (6, 1, -40)
            .filter (1, ladder24, 1300, 10).lfo (1, triangle, 0.07f, 0).mod (lfo1, "filter1Cutoff", 17).amp (900, 1000, 90, 2300)
            .reverb (45, 100, 7.5f, 45).level (-12).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Obsidian Veil", "Pad").osc (1, analog, sawPos).unison (1, 3, 19, 90).osc (2, additive, 30, 45).pitch (2, -12, 9).unison (2, 2, 12, 90)
            .filter (1, ladder24, 1600, 10).lfo (1, triangle, 0.18f, 0).mod (lfo1, "filter1Cutoff", 21).amp (1400, 1000, 90, 2100)
            .reverb (45, 85, 7, 45).level (-12).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Oily Dusk", "Pad").osc (1, additive, 45).unison (1, 6, 22, 90).noise (6, 1, -40).filter (1, svfLow, 1300, 25).lfo (1, drift, 0.22f, 0)
            .mod (lfo1, "osc1Position", 14).amp (1300, 1000, 100, 2500).reverb (45, 90, 5, 45).level (-12).standardMacros (25).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Pale Hall", "Pad").osc (1, additive, 25).unison (1, 6, 20, 80).osc (2, metal, 15, 40).pitch (2, 12, 8).noise (7, 1, -40)
            .filter (1, svfLow, 1000, 20).lfo (1, sine, 0.22f, 0).mod (lfo1, "osc1Position", 16).amp (700, 1000, 95, 1700).reverb (55, 100, 5, 45)
            .level (-12).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Pale Haze", "Pad").osc (1, fm, 45).unison (1, 4, 15, 80).filter (1, svfLow, 2100, 20).lfo (1, triangle, 0.15f, 0)
            .mod (lfo1, "osc1Position", 24).amp (1600, 1000, 95, 3200).phaser (30, 0.09f, 50).reverb (50, 95, 7, 45).level (-14).standardMacros (25)
            .macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Pressed Embers", "Pad").osc (1, pulse, 65).unison (1, 5, 15, 85).osc (2, pulse, 55, 60).pitch (2, -12, 12)
            .filter (1, ladder12, 2300, 10).lfo (1, sine, 0.16f, 0).mod (lfo1, "osc1Position", 24).amp (1900, 1000, 95, 2100).phaser (25, 0.11f, 50)
            .reverb (50, 95, 9, 45).level (-12).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Pressed Haze", "Pad").osc (1, analog, sawPos).unison (1, 5, 12, 90).osc (2, additive, 50, 60).pitch (2, 12, 5).noise (6, 1, -40)
            .filter (1, ladder12, 1000, 20).lfo (1, drift, 0.13f, 0).mod (lfo1, "filter1Cutoff", 15).amp (800, 1000, 85, 2400).chorus (25, 0.6f, 50)
            .reverb (45, 90, 6, 45).level (-9).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Riveted Snow", "Pad").osc (1, analog, sawPos).unison (1, 3, 12, 95).osc (2, pulse, 50, 65).pitch (2, 0, 10)
            .filter (1, ladder24, 1000, 20).lfo (1, sine, 0.07f, 0).mod (lfo1, "filter1Cutoff", 23).amp (600, 1000, 95, 3400).phaser (30, 0.14f, 50)
            .reverb (40, 90, 6, 45).level (-12).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Rotten Smog", "Pad").osc (1, growl, 15).unison (1, 5, 19, 90).filter (1, ladder24, 2000, 30).lfo (1, triangle, 0.22f, 0)
            .mod (lfo1, "osc1Position", 10).amp (600, 1000, 100, 2200).reverb (35, 95, 5, 45).level (-12).standardMacros (25).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Ruined Haze", "Pad").osc (1, vowel, 45).unison (1, 3, 14, 85).osc (2, metal, 50, 50).pitch (2, 0, 9).filter (1, ladder24, 2300, 20)
            .lfo (1, triangle, 0.14f, 0).mod (lfo1, "osc1Position", 16).amp (1300, 1000, 100, 2400).chorus (40, 0.6f, 50).phaser (30, 0.12f, 50)
            .reverb (40, 80, 9, 45).level (-3).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Ruined Horizon", "Pad").osc (1, analog, sawPos).unison (1, 5, 10, 95).filter (1, ladder12, 2200, 25).lfo (1, triangle, 0.13f, 0)
            .mod (lfo1, "filter1Cutoff", 15).amp (700, 1000, 95, 1700).phaser (40, 0.07f, 50).reverb (40, 85, 7, 45).level (-9).standardMacros (25)
            .macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Rust Dusk", "Pad").osc (1, analog, sawPos).unison (1, 6, 11, 85).osc (2, metal, 25, 65).pitch (2, -12, 9).filter (1, ladder12, 1000, 15)
            .lfo (1, sine, 0.16f, 0).mod (lfo1, "filter1Cutoff", 24).amp (1900, 1000, 90, 3100).reverb (50, 90, 7, 45).level (-12).standardMacros (25)
            .macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Rust Embers", "Pad").osc (1, fm, 35).unison (1, 3, 22, 85).osc (2, pulse, 45, 65).pitch (2, 0, 6).filter (1, ladder24, 2100, 20)
            .lfo (1, drift, 0.2f, 0).mod (lfo1, "osc1Position", 14).amp (1200, 1000, 95, 2800).chorus (40, 0.6f, 50).phaser (30, 0.08f, 50)
            .reverb (35, 90, 8.5f, 45).level (-10).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Rust Ruins", "Pad").osc (1, vowel, 70).unison (1, 5, 17, 90).filter (1, ladder24, 2300, 30).lfo (1, triangle, 0.25f, 0)
            .mod (lfo1, "osc1Position", 17).amp (1100, 1000, 100, 3200).reverb (50, 80, 6.5f, 45).level (-10).standardMacros (25).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Shadow Fog", "Pad").osc (1, metal, 45).unison (1, 5, 21, 95).osc (2, metal, 25, 40).pitch (2, 12, 6).filter (1, svfLow, 1400, 25)
            .lfo (1, drift, 0.09f, 0).mod (lfo1, "osc1Position", 23).amp (600, 1000, 90, 1900).reverb (45, 95, 8, 45).level (-4).standardMacros (25)
            .macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Shrouded Glow", "Pad").osc (1, fm, 15).unison (1, 3, 20, 85).osc (2, pulse, 55, 50).pitch (2, 0, 5).unison (2, 4, 14, 90)
            .noise (7, 1, -40).filter (1, ladder12, 2400, 15).lfo (1, drift, 0.17f, 0).mod (lfo1, "osc1Position", 16).amp (1200, 1000, 90, 2200)
            .reverb (35, 85, 6, 45).level (-14).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Silent Smoke", "Pad").osc (1, vowel, 55).unison (1, 4, 19, 95).osc (2, vowel, 60, 65).pitch (2, 0, 6).filter (1, svfLow, 900, 30)
            .lfo (1, sine, 0.15f, 0).mod (lfo1, "osc1Position", 13).amp (900, 1000, 90, 2500).chorus (40, 0.6f, 50).reverb (50, 95, 6, 45).level (-8)
            .standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Silent Snow", "Pad").osc (1, vowel, 25).unison (1, 3, 14, 90).osc (2, pulse, 25, 45).pitch (2, 7, 5).unison (2, 3, 17, 90)
            .filter (1, ladder24, 1200, 30).lfo (1, drift, 0.1f, 0).mod (lfo1, "osc1Position", 24).amp (800, 1000, 95, 2400).reverb (45, 95, 4.5f, 45)
            .level (-6).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Soot Glow", "Pad").osc (1, metal, 25).unison (1, 3, 11, 95).filter (1, ladder24, 1600, 20).lfo (1, drift, 0.21f, 0)
            .mod (lfo1, "osc1Position", 15).amp (800, 1000, 85, 2900).reverb (40, 85, 4.5f, 45).level (-3).standardMacros (25).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Steel Horizon", "Pad").osc (1, additive, 50).unison (1, 3, 15, 85).noise (7, 1, -40).filter (1, ladder24, 2000, 20)
            .lfo (1, triangle, 0.16f, 0).mod (lfo1, "osc1Position", 22).amp (900, 1000, 95, 1700).chorus (35, 0.6f, 50).phaser (25, 0.12f, 50)
            .reverb (40, 85, 8, 45).level (-4).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Sterile Dusk", "Pad").osc (1, analog, sawPos).unison (1, 5, 14, 85).osc (2, vowel, 15, 45).pitch (2, 12, 11).unison (2, 4, 18, 90)
            .filter (1, ladder12, 1300, 15).lfo (1, drift, 0.24f, 0).mod (lfo1, "filter1Cutoff", 12).amp (2000, 1000, 95, 3000).chorus (30, 0.6f, 50)
            .reverb (45, 95, 8, 45).level (-9).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Sunken Cathedral", "Pad").osc (1, growl, 25).unison (1, 6, 14, 95).osc (2, analog, sawPos, 60).pitch (2, 0, 7).unison (2, 3, 14, 90)
            .filter (1, svfLow, 800, 15).lfo (1, sine, 0.2f, 0).mod (lfo1, "osc1Position", 11).amp (1600, 1000, 100, 1700).chorus (35, 0.6f, 50)
            .reverb (45, 85, 5, 45).level (-12).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Sunken Cloud", "Pad").osc (1, metal, 55).unison (1, 4, 19, 85).filter (1, ladder12, 1000, 20).lfo (1, drift, 0.13f, 0)
            .mod (lfo1, "osc1Position", 14).amp (600, 1000, 95, 3000).reverb (45, 100, 5, 45).level (-7).standardMacros (25).macro (4, "Motion")
            .mod (macro4, "lfo1Rate", 30);
        add ("Tar Glow", "Pad").osc (1, additive, 25).unison (1, 4, 19, 100).osc (2, metal, 10, 45).pitch (2, 0, 12).filter (1, ladder12, 1600, 15)
            .lfo (1, sine, 0.21f, 0).mod (lfo1, "osc1Position", 23).amp (1800, 1000, 100, 3000).chorus (25, 0.6f, 50).reverb (50, 95, 8, 45)
            .level (-9).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Toxic Cloud", "Pad").osc (1, vowel, 60).unison (1, 6, 16, 95).noise (6, 1, -40).filter (1, ladder24, 1900, 25)
            .lfo (1, triangle, 0.19f, 0).mod (lfo1, "osc1Position", 21).amp (1100, 1000, 90, 2900).chorus (25, 0.6f, 50).reverb (40, 95, 9, 45)
            .level (-7).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Void Cloud", "Pad").osc (1, additive, 15).unison (1, 4, 16, 85).osc (2, analog, sawPos, 55).pitch (2, 0, 6)
            .filter (1, ladder24, 900, 15).lfo (1, drift, 0.09f, 0).mod (lfo1, "osc1Position", 8).amp (1500, 1000, 95, 1600).phaser (40, 0.1f, 50)
            .reverb (50, 85, 5.5f, 45).level (-10).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Welded Tundra", "Pad").osc (1, fm, 45).unison (1, 5, 12, 90).osc (2, pulse, 15, 40).pitch (2, 0, 10).unison (2, 4, 9, 90)
            .filter (1, svfLow, 1900, 20).lfo (1, sine, 0.21f, 0).mod (lfo1, "osc1Position", 11).amp (900, 1000, 85, 3400).reverb (40, 90, 7.5f, 45)
            .level (-15).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Winter Smog", "Pad").osc (1, analog, sawPos).unison (1, 5, 19, 90).osc (2, pulse, 30, 55).pitch (2, 7, 8).filter (1, ladder24, 2500, 15)
            .lfo (1, triangle, 0.1f, 0).mod (lfo1, "filter1Cutoff", 16).amp (1300, 1000, 95, 1800).phaser (30, 0.14f, 50).reverb (55, 100, 5, 45)
            .level (-12).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Winter Tundra", "Pad").osc (1, pulse, 25).unison (1, 3, 17, 80).osc (2, pulse, 45, 45).pitch (2, 12, 6).filter (1, ladder12, 1900, 20)
            .lfo (1, drift, 0.07f, 0).mod (lfo1, "osc1Position", 21).amp (1500, 1000, 90, 1800).phaser (35, 0.11f, 50).reverb (35, 80, 7, 45)
            .level (-14).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);
        add ("Zinc Haze", "Pad").osc (1, analog, sawPos).unison (1, 4, 22, 80).noise (11, 1, -40).filter (1, svfLow, 1200, 10)
            .lfo (1, triangle, 0.17f, 0).mod (lfo1, "filter1Cutoff", 25).amp (1600, 1000, 100, 1500).chorus (25, 0.6f, 50).phaser (40, 0.07f, 50)
            .reverb (40, 80, 5.5f, 45).level (-9).standardMacros (25).macro (4, "Motion").mod (macro4, "lfo1Rate", 30);

        // Drone, by name.
        add ("Abandoned Mains", "Drone").osc (1, additive, 35).unison (1, 1, 23, 100).osc (2, additive, 25, 50).pitch (2, 19, 6)
            .filter (1, ladder24, 800, 60, 40).lfo (1, drift, 0.05f, 0).mod (lfo1, "osc1Position", 20).lfo (2, sine, 0.04f, 0)
            .mod (lfo2, "filter1Cutoff", 10).amp (2200, 1000, 100, 3200).drive (hard, 20, 3500).reverb (30, 100, 7.5f, 45).level (-17)
            .standardMacros (20);
        add ("Abandoned Reactor", "Drone").osc (1, pulse, 20).unison (1, 6, 32, 100).filter (1, ladder24, 1100, 40, 30).lfo (1, drift, 0.1f, 0)
            .mod (lfo1, "osc1Position", 25).lfo (2, sine, 0.05f, 0).mod (lfo2, "filter1Cutoff", 7).amp (2000, 1000, 100, 3400).reverb (35, 100, 8, 45)
            .level (-12).standardMacros (20);
        add ("Acid Furnace", "Drone").osc (1, metal, 70).unison (1, 8, 35, 100).osc (2, metal, 20, 45).pitch (2, 12, 5).noise (15, 1)
            .filter (1, comb, 90, 55, 30).lfo (1, drift, 0.08f, 0).mod (lfo1, "osc1Position", 20).lfo (2, sine, 0.03f, 0)
            .mod (lfo2, "filter1Cutoff", 8).amp (2200, 1000, 100, 4300).drive (hard, 25, 3500).reverb (40, 100, 10, 45).level (-16)
            .standardMacros (20);
        add ("Acid Grid", "Drone").osc (1, analog, sawPos).unison (1, 4, 13, 100).noise (25, 1).filter (1, comb, 120, 35, 20).lfo (1, drift, 0.11f, 0)
            .mod (lfo1, "osc1Position", 25).lfo (2, sine, 0.04f, 0).mod (lfo2, "filter1Cutoff", 8).amp (2300, 1000, 100, 4300).drive (hard, 20, 3500)
            .reverb (35, 100, 8.5f, 45).level (-19).standardMacros (20);
        add ("Acid Reactor", "Drone").osc (1, sub, 60).unison (1, 4, 35, 100).filter (1, ladder24, 850, 55, 50).lfo (1, drift, 0.1f, 0)
            .mod (lfo1, "osc1Position", 25).lfo (2, sine, 0.04f, 0).mod (lfo2, "filter1Cutoff", 6).amp (2600, 1000, 100, 4200).drive (crush, 25, 3500)
            .reverb (50, 100, 6.5f, 45).level (-12).standardMacros (20);
        add ("Basalt Furnace", "Drone").osc (1, sub, 20).unison (1, 6, 13, 100).filter (1, comb, 125, 55, 30).lfo (1, drift, 0.08f, 0)
            .mod (lfo1, "osc1Position", 25).lfo (2, sine, 0.03f, 0).mod (lfo2, "filter1Cutoff", 11).amp (1700, 1000, 100, 3500).drive (hard, 20, 3500)
            .reverb (40, 100, 6, 45).level (-20).standardMacros (20);
        add ("Black Dynamo", "Drone").osc (1, additive, 25).unison (1, 4, 37, 100).filter (1, svfBand, 600, 35, 10).lfo (1, drift, 0.05f, 0)
            .mod (lfo1, "osc1Position", 35).lfo (2, sine, 0.06f, 0).mod (lfo2, "filter1Cutoff", 10).amp (1500, 1000, 100, 3200).drive (hard, 15, 3500)
            .reverb (45, 100, 7, 45).level (-9).standardMacros (20);
        add ("Black Reactor", "Drone").osc (1, sub, 55).unison (1, 4, 20, 100).filter (1, ladder24, 1000, 35).lfo (1, drift, 0.03f, 0)
            .mod (lfo1, "osc1Position", 20).lfo (2, sine, 0.03f, 0).mod (lfo2, "filter1Cutoff", 11).amp (2200, 1000, 100, 3500)
            .reverb (30, 100, 5.5f, 45).level (-12).standardMacros (20);
        add ("Black Substation", "Drone").osc (1, growl, 65).unison (1, 2, 16, 100).noise (20, 3).filter (1, svfBand, 900, 50, 10)
            .lfo (1, drift, 0.12f, 0).mod (lfo1, "osc1Position", 35).lfo (2, sine, 0.04f, 0).mod (lfo2, "filter1Cutoff", 6)
            .amp (2500, 1000, 100, 4900).reverb (50, 100, 5.5f, 45).level (-6).standardMacros (20);
        add ("Bleak Hum", "Drone").osc (1, sub, 45).unison (1, 4, 27, 100).filter (1, ladder24, 500, 55, 30).lfo (1, drift, 0.11f, 0)
            .mod (lfo1, "osc1Position", 15).lfo (2, sine, 0.07f, 0).mod (lfo2, "filter1Cutoff", 12).amp (1900, 1000, 100, 4300)
            .reverb (40, 100, 9.5f, 45).level (-12).standardMacros (20);
        add ("Brutal Silo", "Drone").osc (1, analog, sawPos).unison (1, 4, 12, 100).osc (2, additive, 35, 50).pitch (2, 12, 2)
            .filter (1, svfBand, 400, 40, 20).lfo (1, drift, 0.08f, 0).mod (lfo1, "osc1Position", 30).lfo (2, sine, 0.07f, 0)
            .mod (lfo2, "filter1Cutoff", 11).amp (2800, 1000, 100, 3800).drive (tube, 20, 3500).reverb (35, 100, 8.5f, 45).level (-10)
            .standardMacros (20);
        add ("Buried Dynamo", "Drone").osc (1, growl, 50).unison (1, 1, 25, 100).noise (20, 3).filter (1, ladder24, 900, 50, 40)
            .lfo (1, drift, 0.04f, 0).mod (lfo1, "osc1Position", 35).lfo (2, sine, 0.07f, 0).mod (lfo2, "filter1Cutoff", 8)
            .amp (1600, 1000, 100, 4400).drive (hard, 25, 3500).reverb (35, 100, 6.5f, 45).level (-19).standardMacros (20);
        add ("Buried Mill", "Drone").osc (1, sync, 40).unison (1, 1, 12, 100).osc (2, metal, 50, 50).pitch (2, 7, 4).noise (20, 1)
            .filter (1, svfBand, 500, 35, 20).lfo (1, drift, 0.08f, 0).mod (lfo1, "osc1Position", 15).lfo (2, sine, 0.06f, 0)
            .mod (lfo2, "filter1Cutoff", 10).amp (2000, 1000, 100, 4300).reverb (45, 100, 7.5f, 45).level (-12).standardMacros (20);
        add ("Carbon Mill", "Drone").osc (1, pulse, 60).unison (1, 1, 31, 100).filter (1, comb, 140, 65, 40).lfo (1, drift, 0.1f, 0)
            .mod (lfo1, "osc1Position", 20).lfo (2, sine, 0.05f, 0).mod (lfo2, "filter1Cutoff", 9).amp (2100, 1000, 100, 4800).drive (hard, 30, 3500)
            .reverb (35, 100, 7.5f, 45).level (-23).standardMacros (20);
        add ("Ceaseless Dynamo", "Drone").osc (1, sync, 25).unison (1, 8, 22, 100).noise (20, 3).filter (1, comb, 135, 80, 40).lfo (1, drift, 0.1f, 0)
            .mod (lfo1, "osc1Position", 25).lfo (2, sine, 0.05f, 0).mod (lfo2, "filter1Cutoff", 11).amp (1800, 1000, 100, 3700).drive (tube, 25, 3500)
            .reverb (45, 100, 6, 45).level (-20).standardMacros (20);
        add ("Choked Shaft", "Drone").osc (1, pulse, 25).unison (1, 2, 24, 100).filter (1, ladder24, 500, 40, 40).lfo (1, drift, 0.06f, 0)
            .mod (lfo1, "osc1Position", 20).lfo (2, sine, 0.07f, 0).mod (lfo2, "filter1Cutoff", 10).amp (2700, 1000, 100, 4700)
            .reverb (40, 100, 8, 45).level (-12).standardMacros (20);
        add ("Chrome Reactor", "Drone").osc (1, analog, sawPos).unison (1, 4, 27, 100).osc (2, sync, 50, 40).pitch (2, 19, 3)
            .filter (1, ladder24, 1100, 35, 40).lfo (1, drift, 0.08f, 0).mod (lfo1, "osc1Position", 30).lfo (2, sine, 0.06f, 0)
            .mod (lfo2, "filter1Cutoff", 8).amp (2200, 1000, 100, 4500).reverb (45, 100, 7.5f, 45).level (-9).standardMacros (20);
        add ("Cinder Bunker", "Drone").osc (1, sync, 65).unison (1, 1, 32, 100).osc (2, additive, 35, 45).pitch (2, -12, 6)
            .filter (1, ladder24, 1050, 40, 30).lfo (1, drift, 0.09f, 0).mod (lfo1, "osc1Position", 30).lfo (2, sine, 0.08f, 0)
            .mod (lfo2, "filter1Cutoff", 9).amp (2900, 1000, 100, 4900).reverb (45, 100, 11.5f, 45).level (-10).standardMacros (20);
        add ("Cobalt Bunker", "Drone").osc (1, pulse, 40).unison (1, 8, 13, 100).osc (2, sync, 40, 40).pitch (2, -12, 3).noise (15, 3)
            .filter (1, comb, 125, 45, 50).lfo (1, drift, 0.08f, 0).mod (lfo1, "osc1Position", 30).lfo (2, sine, 0.03f, 0)
            .mod (lfo2, "filter1Cutoff", 5).amp (2200, 1000, 100, 3200).reverb (45, 100, 11.5f, 45).level (-12).standardMacros (20);
        add ("Cold Dynamo", "Drone").osc (1, analog, sawPos).unison (1, 1, 28, 100).filter (1, svfBand, 550, 50, 10).lfo (1, drift, 0.04f, 0)
            .mod (lfo1, "osc1Position", 35).lfo (2, sine, 0.07f, 0).mod (lfo2, "filter1Cutoff", 7).amp (2600, 1000, 100, 4500)
            .reverb (45, 100, 9.5f, 45).level (3).standardMacros (20);
        add ("Concrete Silo", "Drone").osc (1, sub, 70).unison (1, 6, 13, 100).osc (2, analog, 65, 60).pitch (2, 7, 5)
            .filter (1, ladder24, 1050, 35, 10).lfo (1, drift, 0.04f, 0).mod (lfo1, "osc1Position", 25).lfo (2, sine, 0.06f, 0)
            .mod (lfo2, "filter1Cutoff", 5).amp (2300, 1000, 100, 4900).reverb (35, 100, 7, 45).level (-12).standardMacros (20);
        add ("Corroded Drone", "Drone").osc (1, pulse, 60).unison (1, 4, 27, 100).noise (25, 1).filter (1, ladder24, 450, 30, 40)
            .lfo (1, drift, 0.06f, 0).mod (lfo1, "osc1Position", 25).lfo (2, sine, 0.05f, 0).mod (lfo2, "filter1Cutoff", 6)
            .amp (3000, 1000, 100, 4300).drive (tube, 15, 3500).reverb (45, 100, 8, 45).level (-12).standardMacros (20);
        add ("Corroded Generator", "Drone").osc (1, pulse, 30).unison (1, 4, 34, 100).filter (1, svfBand, 500, 45, 50).lfo (1, drift, 0.04f, 0)
            .mod (lfo1, "osc1Position", 25).lfo (2, sine, 0.07f, 0).mod (lfo2, "filter1Cutoff", 10).amp (1900, 1000, 100, 4600)
            .reverb (50, 100, 6, 45).level (-9).standardMacros (20);
        add ("Dead Furnace", "Drone").osc (1, additive, 70).unison (1, 2, 11, 100).filter (1, comb, 115, 50, 30).lfo (1, drift, 0.1f, 0)
            .mod (lfo1, "osc1Position", 30).lfo (2, sine, 0.08f, 0).mod (lfo2, "filter1Cutoff", 7).amp (2800, 1000, 100, 3300).reverb (30, 100, 9, 45)
            .level (-10).standardMacros (20);
        add ("Endless Silo", "Drone").osc (1, growl, 45).unison (1, 2, 23, 100).filter (1, ladder24, 350, 50, 40).lfo (1, drift, 0.06f, 0)
            .mod (lfo1, "osc1Position", 30).lfo (2, sine, 0.03f, 0).mod (lfo2, "filter1Cutoff", 11).amp (2600, 1000, 100, 3900)
            .reverb (45, 100, 5.5f, 45).level (-12).standardMacros (20);
        add ("Feral Hull", "Drone").osc (1, sync, 50).unison (1, 1, 31, 100).filter (1, svfBand, 350, 55, 10).lfo (1, drift, 0.09f, 0)
            .mod (lfo1, "osc1Position", 30).lfo (2, sine, 0.07f, 0).mod (lfo2, "filter1Cutoff", 10).amp (1900, 1000, 100, 4400).drive (tube, 35, 3500)
            .reverb (45, 100, 6, 45).level (-17).standardMacros (20);
        add ("Grey Grid", "Drone").osc (1, additive, 35).unison (1, 2, 31, 100).osc (2, metal, 45, 50).pitch (2, 7, 6).filter (1, comb, 95, 45, 50)
            .lfo (1, drift, 0.06f, 0).mod (lfo1, "osc1Position", 20).lfo (2, sine, 0.07f, 0).mod (lfo2, "filter1Cutoff", 6)
            .amp (2500, 1000, 100, 3100).reverb (40, 100, 6.5f, 45).level (-12).standardMacros (20);
        add ("Industrial Mains", "Drone").osc (1, metal, 45).unison (1, 6, 22, 100).osc (2, analog, 30, 55).pitch (2, 12, 6)
            .filter (1, ladder24, 700, 40, 30).lfo (1, drift, 0.04f, 0).mod (lfo1, "osc1Position", 30).lfo (2, sine, 0.05f, 0)
            .mod (lfo2, "filter1Cutoff", 11).amp (2900, 1000, 100, 4600).drive (hard, 20, 3500).reverb (50, 100, 11, 45).level (-14)
            .standardMacros (20);
        add ("Infinite Plant", "Drone").osc (1, additive, 40).unison (1, 6, 19, 100).osc (2, additive, 35, 55).pitch (2, 12, 6).noise (10, 3)
            .filter (1, ladder24, 350, 50, 40).lfo (1, drift, 0.09f, 0).mod (lfo1, "osc1Position", 35).lfo (2, sine, 0.03f, 0)
            .mod (lfo2, "filter1Cutoff", 10).amp (1800, 1000, 100, 3700).reverb (50, 100, 6.5f, 45).level (-12).standardMacros (20);
        add ("Molten Core", "Drone").osc (1, analog, sawPos).unison (1, 2, 24, 100).filter (1, ladder24, 1100, 30, 50).lfo (1, drift, 0.06f, 0)
            .mod (lfo1, "osc1Position", 30).lfo (2, sine, 0.02f, 0).mod (lfo2, "filter1Cutoff", 6).amp (1700, 1000, 100, 4800)
            .reverb (30, 100, 7.5f, 45).level (-10).standardMacros (20);
        add ("Molten Vault", "Drone").osc (1, analog, sawPos).unison (1, 6, 26, 100).osc (2, additive, 45, 40).pitch (2, 12, 6)
            .filter (1, svfBand, 650, 40).lfo (1, drift, 0.1f, 0).mod (lfo1, "osc1Position", 30).lfo (2, sine, 0.03f, 0)
            .mod (lfo2, "filter1Cutoff", 6).amp (2300, 1000, 100, 4300).drive (hard, 35, 3500).reverb (35, 100, 9, 45).level (-14).standardMacros (20);
        add ("Mute Boiler", "Drone").osc (1, metal, 40).unison (1, 6, 38, 100).osc (2, sync, 65, 40).pitch (2, 12, 3)
            .filter (1, ladder24, 1050, 35, 50).lfo (1, drift, 0.05f, 0).mod (lfo1, "osc1Position", 15).lfo (2, sine, 0.05f, 0)
            .mod (lfo2, "filter1Cutoff", 8).amp (2600, 1000, 100, 3200).reverb (30, 100, 6.5f, 45).level (-5).standardMacros (20);
        add ("Night Transformer", "Drone").osc (1, sync, 55).unison (1, 6, 17, 100).osc (2, analog, 55, 45).pitch (2, 7, 5).noise (15, 1)
            .filter (1, ladder24, 600, 45, 30).lfo (1, drift, 0.07f, 0).mod (lfo1, "osc1Position", 35).lfo (2, sine, 0.06f, 0)
            .mod (lfo2, "filter1Cutoff", 9).amp (3000, 1000, 100, 4100).drive (crush, 20, 3500).reverb (35, 100, 5, 45).level (-9)
            .standardMacros (20);
        add ("Null Generator", "Drone").osc (1, sync, 25).unison (1, 1, 13, 100).noise (15, 3).filter (1, ladder24, 550, 45, 20)
            .lfo (1, drift, 0.08f, 0).mod (lfo1, "osc1Position", 25).lfo (2, sine, 0.05f, 0).mod (lfo2, "filter1Cutoff", 7)
            .amp (3000, 1000, 100, 4500).reverb (45, 100, 10.5f, 45).level (-10).standardMacros (20);
        add ("Oily Boiler", "Drone").osc (1, pulse, 60).unison (1, 8, 30, 100).osc (2, sync, 60, 45).pitch (2, 12, 2).noise (20, 3)
            .filter (1, svfBand, 1050, 40, 40).lfo (1, drift, 0.09f, 0).mod (lfo1, "osc1Position", 25).lfo (2, sine, 0.03f, 0)
            .mod (lfo2, "filter1Cutoff", 8).amp (2300, 1000, 100, 3800).drive (tube, 35, 3500).reverb (35, 100, 9.5f, 45).level (-15)
            .standardMacros (20);
        add ("Oily Chamber", "Drone").osc (1, sync, 25).unison (1, 4, 12, 100).osc (2, analog, 40, 45).pitch (2, 12, 5)
            .filter (1, svfBand, 1100, 45, 10).lfo (1, drift, 0.1f, 0).mod (lfo1, "osc1Position", 30).lfo (2, sine, 0.05f, 0)
            .mod (lfo2, "filter1Cutoff", 11).amp (2700, 1000, 100, 4000).drive (crush, 25, 3500).reverb (35, 100, 5, 45).level (-7)
            .standardMacros (20);
        add ("Riveted Core", "Drone").osc (1, growl, 55).unison (1, 2, 16, 100).osc (2, additive, 25, 40).pitch (2, 12, 3)
            .filter (1, comb, 90, 45, 20).lfo (1, drift, 0.08f, 0).mod (lfo1, "osc1Position", 25).lfo (2, sine, 0.05f, 0)
            .mod (lfo2, "filter1Cutoff", 8).amp (1800, 1000, 100, 3100).reverb (45, 100, 8.5f, 45).level (-12).standardMacros (20);
        add ("Riveted Furnace", "Drone").osc (1, analog, sawPos).unison (1, 2, 38, 100).noise (20, 1).filter (1, ladder24, 1150, 55, 40)
            .lfo (1, drift, 0.03f, 0).mod (lfo1, "osc1Position", 35).lfo (2, sine, 0.07f, 0).mod (lfo2, "filter1Cutoff", 10)
            .amp (2600, 1000, 100, 3700).drive (hard, 35, 3500).reverb (35, 100, 10, 45).level (-19).standardMacros (20);
        add ("Ruined Hull", "Drone").osc (1, pulse, 65).unison (1, 4, 27, 100).noise (10, 1).filter (1, svfBand, 1100, 45, 20)
            .lfo (1, drift, 0.12f, 0).mod (lfo1, "osc1Position", 20).lfo (2, sine, 0.08f, 0).mod (lfo2, "filter1Cutoff", 5)
            .amp (2400, 1000, 100, 3600).drive (tube, 30, 3500).reverb (30, 100, 9.5f, 45).level (-12).standardMacros (20);
        add ("Ruined Mill", "Drone").osc (1, analog, sawPos).unison (1, 4, 27, 100).osc (2, sync, 60, 40).pitch (2, 7, 4)
            .filter (1, comb, 125, 60, 10).lfo (1, drift, 0.03f, 0).mod (lfo1, "osc1Position", 20).lfo (2, sine, 0.08f, 0)
            .mod (lfo2, "filter1Cutoff", 6).amp (2500, 1000, 100, 3900).reverb (45, 100, 7.5f, 45).level (-12).standardMacros (20);
        add ("Rust Bunker", "Drone").osc (1, growl, 45).unison (1, 6, 35, 100).noise (20, 3).filter (1, ladder24, 900, 60, 30)
            .lfo (1, drift, 0.07f, 0).mod (lfo1, "osc1Position", 30).lfo (2, sine, 0.07f, 0).mod (lfo2, "filter1Cutoff", 6)
            .amp (1500, 1000, 100, 4500).drive (crush, 30, 3500).reverb (35, 100, 10, 45).level (-12).standardMacros (20);
        add ("Scorched Furnace", "Drone").osc (1, metal, 35).unison (1, 8, 35, 100).osc (2, metal, 30, 50).pitch (2, -12, 5)
            .filter (1, ladder24, 1100, 45, 10).lfo (1, drift, 0.04f, 0).mod (lfo1, "osc1Position", 35).lfo (2, sine, 0.03f, 0)
            .mod (lfo2, "filter1Cutoff", 11).amp (1700, 1000, 100, 4300).reverb (30, 100, 8, 45).level (-7).standardMacros (20);
        add ("Scorched Hull", "Drone").osc (1, additive, 30).unison (1, 8, 13, 100).filter (1, ladder24, 1150, 45, 20).lfo (1, drift, 0.1f, 0)
            .mod (lfo1, "osc1Position", 15).lfo (2, sine, 0.02f, 0).mod (lfo2, "filter1Cutoff", 11).amp (2100, 1000, 100, 4000).drive (tube, 25, 3500)
            .reverb (40, 100, 7, 45).level (-17).standardMacros (20);
        add ("Scorched Mains", "Drone").osc (1, metal, 60).unison (1, 4, 12, 100).osc (2, analog, 65, 50).pitch (2, 12, 3)
            .filter (1, ladder24, 1150, 50, 20).lfo (1, drift, 0.09f, 0).mod (lfo1, "osc1Position", 35).lfo (2, sine, 0.06f, 0)
            .mod (lfo2, "filter1Cutoff", 11).amp (2800, 1000, 100, 4600).reverb (45, 100, 7, 45).level (-7).standardMacros (20);
        add ("Shadow Furnace", "Drone").osc (1, sub, 55).unison (1, 4, 22, 100).filter (1, ladder24, 500, 50, 30).lfo (1, drift, 0.06f, 0)
            .mod (lfo1, "osc1Position", 15).lfo (2, sine, 0.04f, 0).mod (lfo2, "filter1Cutoff", 5).amp (1900, 1000, 100, 3600)
            .reverb (35, 100, 8.5f, 45).level (-12).standardMacros (20);
        add ("Sodium Transformer", "Drone").osc (1, additive, 20).unison (1, 1, 35, 100).osc (2, metal, 70, 50).pitch (2, -12, 3)
            .filter (1, ladder24, 750, 55, 10).lfo (1, drift, 0.03f, 0).mod (lfo1, "osc1Position", 35).lfo (2, sine, 0.03f, 0)
            .mod (lfo2, "filter1Cutoff", 12).amp (2400, 1000, 100, 4500).reverb (30, 100, 11, 45).level (-9).standardMacros (20);
        add ("Soot Shaft", "Drone").osc (1, growl, 60).unison (1, 6, 29, 100).osc (2, metal, 30, 45).pitch (2, 12, 2)
            .filter (1, ladder24, 550, 45, 20).lfo (1, drift, 0.07f, 0).mod (lfo1, "osc1Position", 20).lfo (2, sine, 0.03f, 0)
            .mod (lfo2, "filter1Cutoff", 10).amp (2300, 1000, 100, 3200).drive (crush, 30, 3500).reverb (45, 100, 6, 45).level (-12)
            .standardMacros (20);
        add ("Static Chamber", "Drone").osc (1, pulse, 20).unison (1, 6, 18, 100).filter (1, ladder24, 700, 40, 30).lfo (1, drift, 0.1f, 0)
            .mod (lfo1, "osc1Position", 25).lfo (2, sine, 0.03f, 0).mod (lfo2, "filter1Cutoff", 12).amp (2700, 1000, 100, 4700)
            .drive (crush, 30, 3500).reverb (50, 100, 11, 45).level (-12).standardMacros (20);
        add ("Static Hum", "Drone").osc (1, sync, 55).unison (1, 1, 16, 100).osc (2, additive, 35, 60).pitch (2, -12, 4)
            .filter (1, ladder24, 700, 30, 10).lfo (1, drift, 0.07f, 0).mod (lfo1, "osc1Position", 20).lfo (2, sine, 0.05f, 0)
            .mod (lfo2, "filter1Cutoff", 8).amp (1800, 1000, 100, 3400).reverb (35, 100, 9.5f, 45).level (-10).standardMacros (20);
        add ("Sterile Drone", "Drone").osc (1, metal, 45).unison (1, 6, 30, 100).osc (2, sync, 40, 60).pitch (2, 12, 4).filter (1, comb, 120, 70, 30)
            .lfo (1, drift, 0.06f, 0).mod (lfo1, "osc1Position", 20).lfo (2, sine, 0.04f, 0).mod (lfo2, "filter1Cutoff", 10)
            .amp (2400, 1000, 100, 3900).drive (hard, 20, 3500).reverb (30, 100, 8, 45).level (-16).standardMacros (20);
        add ("Sterile Hum", "Drone").osc (1, additive, 45).unison (1, 4, 37, 100).osc (2, sync, 35, 55).pitch (2, 7, 4).filter (1, svfBand, 850, 35)
            .lfo (1, drift, 0.04f, 0).mod (lfo1, "osc1Position", 30).lfo (2, sine, 0.06f, 0).mod (lfo2, "filter1Cutoff", 10)
            .amp (2200, 1000, 100, 3700).drive (hard, 25, 3500).reverb (35, 100, 5, 45).level (-17).standardMacros (20);
        add ("Subsonic Dynamo", "Drone").osc (1, sub, 55).unison (1, 2, 25, 100).filter (1, comb, 140, 45, 50).lfo (1, drift, 0.06f, 0)
            .mod (lfo1, "osc1Position", 30).lfo (2, sine, 0.04f, 0).mod (lfo2, "filter1Cutoff", 9).amp (2200, 1000, 100, 3600)
            .reverb (40, 100, 10.5f, 45).level (-12).standardMacros (20);
        add ("Sullen Mill", "Drone").osc (1, sub, 60).unison (1, 4, 30, 100).filter (1, comb, 115, 45, 50).lfo (1, drift, 0.07f, 0)
            .mod (lfo1, "osc1Position", 20).lfo (2, sine, 0.02f, 0).mod (lfo2, "filter1Cutoff", 12).amp (2200, 1000, 100, 4500).drive (hard, 25, 3500)
            .reverb (45, 100, 7.5f, 45).level (-19).standardMacros (20);
        add ("Sunken Reactor", "Drone").osc (1, sub, 50).unison (1, 1, 39, 100).osc (2, metal, 30, 55).pitch (2, 12, 4).noise (15, 1)
            .filter (1, ladder24, 300, 55, 10).lfo (1, drift, 0.05f, 0).mod (lfo1, "osc1Position", 15).lfo (2, sine, 0.04f, 0)
            .mod (lfo2, "filter1Cutoff", 10).amp (2200, 1000, 100, 3700).drive (tube, 30, 3500).reverb (45, 100, 5.5f, 45).level (-20)
            .standardMacros (20);
        add ("Tar Furnace", "Drone").osc (1, metal, 25).unison (1, 2, 24, 100).osc (2, sync, 40, 50).pitch (2, 19, 6).noise (20, 1)
            .filter (1, ladder24, 800, 45, 10).lfo (1, drift, 0.08f, 0).mod (lfo1, "osc1Position", 35).lfo (2, sine, 0.05f, 0)
            .mod (lfo2, "filter1Cutoff", 11).amp (2000, 1000, 100, 4700).reverb (45, 100, 6.5f, 45).level (-6).standardMacros (20);
        add ("Tar Silo", "Drone").osc (1, metal, 35).unison (1, 1, 24, 100).noise (10, 3).filter (1, ladder24, 700, 50, 10).lfo (1, drift, 0.12f, 0)
            .mod (lfo1, "osc1Position", 35).lfo (2, sine, 0.04f, 0).mod (lfo2, "filter1Cutoff", 12).amp (2900, 1000, 100, 4500)
            .drive (crush, 20, 3500).reverb (35, 100, 6, 45).level (-4).standardMacros (20);
        add ("Throbbing Boiler", "Drone").osc (1, additive, 40).unison (1, 8, 15, 100).noise (25, 3).filter (1, ladder24, 750, 55, 20)
            .lfo (1, drift, 0.07f, 0).mod (lfo1, "osc1Position", 35).lfo (2, sine, 0.03f, 0).mod (lfo2, "filter1Cutoff", 9)
            .amp (2300, 1000, 100, 4400).drive (hard, 20, 3500).reverb (35, 100, 12, 45).level (-17).standardMacros (20);
        add ("Vacant Hum", "Drone").osc (1, additive, 50).unison (1, 4, 12, 100).filter (1, svfBand, 900, 55, 50).lfo (1, drift, 0.1f, 0)
            .mod (lfo1, "osc1Position", 25).lfo (2, sine, 0.05f, 0).mod (lfo2, "filter1Cutoff", 10).amp (2100, 1000, 100, 4900).drive (hard, 30, 3500)
            .reverb (45, 100, 10.5f, 45).level (-17).standardMacros (20);
        add ("Welded Furnace", "Drone").osc (1, analog, sawPos).unison (1, 4, 17, 100).noise (20, 1).filter (1, ladder24, 500, 50)
            .lfo (1, drift, 0.1f, 0).mod (lfo1, "osc1Position", 25).lfo (2, sine, 0.06f, 0).mod (lfo2, "filter1Cutoff", 7).amp (2100, 1000, 100, 4600)
            .drive (crush, 25, 3500).reverb (35, 100, 10.5f, 45).level (-12).standardMacros (20);
        add ("Welded Plant", "Drone").osc (1, sub, 25).unison (1, 6, 22, 100).osc (2, additive, 20, 40).pitch (2, -12, 4)
            .filter (1, svfBand, 1150, 35, 40).lfo (1, drift, 0.11f, 0).mod (lfo1, "osc1Position", 15).lfo (2, sine, 0.06f, 0)
            .mod (lfo2, "filter1Cutoff", 11).amp (1600, 1000, 100, 4500).reverb (35, 100, 9, 45).level (-2).standardMacros (20);

        // Bell, by name.
        add ("Abandoned Chime", "Bell").osc (1, additive, 65).osc (2, analog, sinePos, 40).pitch (2, 19).env (2, 0, 1400, 0, 1500, 70)
            .mod (env2, "osc1Position", 35).filter (1, ladder12, 2700, 25).amp (0, 3100, 0, 2100, 70).delay (quarterD, 40, 24, 4000)
            .reverb (35, 95, 4, 45).level (-10).standardMacros (25);
        add ("Basalt Peal", "Bell").osc (1, fm, 90).env (2, 0, 1000, 0, 1500, 70).mod (env2, "osc1Position", 45).warp (1, wFm, 30)
            .filter (1, svfBand, 2900, 20).amp (0, 3700, 0, 2600, 70).delay (eighthD, 35, 20, 4000).reverb (35, 95, 6, 45).level (-12)
            .standardMacros (25);
        add ("Bitter Knell", "Bell").osc (1, fm, 85).osc (2, metal, sinePos, 40).pitch (2, 7).warp (1, wFm, 35).filter (1, svfHigh, 1100, 10)
            .amp (0, 3800, 0, 2000, 70).reverb (40, 80, 6.5f, 45).level (-10).standardMacros (25);
        add ("Bitter Tube", "Bell").osc (1, metal, 50).osc (2, metal, sinePos, 45).pitch (2, 12).env (2, 0, 1400, 0, 1500, 70)
            .mod (env2, "osc1Position", 45).filter (1, ladder12, 1200, 20).amp (0, 3200, 0, 2000, 70).delay (quarterD, 40, 26, 4000)
            .reverb (40, 95, 4, 45).level (-4).standardMacros (25);
        add ("Black Bowl", "Bell").osc (1, metal, 65).osc (2, metal, sinePos, 50).pitch (2, 19).filter (1, ladder12, 3000, 20)
            .amp (0, 1600, 0, 1700, 70).reverb (45, 90, 5.5f, 45).level (-3).standardMacros (25);
        add ("Bleak Clapper", "Bell").osc (1, metal, 65).env (2, 0, 1400, 0, 1500, 70).mod (env2, "osc1Position", 35).warp (1, wMirror, 20)
            .filter (1, svfBand, 1500, 25).amp (0, 1800, 0, 1400, 70).reverb (40, 75, 8, 45).level (-2).standardMacros (25);
        add ("Brutal Tine", "Bell").osc (1, additive, 85).osc (2, fm, 40, 45).pitch (2, 24).filter (1, ladder12, 500, 15).amp (0, 2900, 0, 2600, 70)
            .reverb (50, 80, 5.5f, 45).level (-7).standardMacros (25);
        add ("Buried Peal", "Bell").osc (1, additive, 55).osc (2, fm, sinePos, 55).pitch (2, 24).env (2, 0, 1100, 0, 1500, 70)
            .mod (env2, "osc1Position", 40).filter (1, ladder12, 2800, 20).amp (0, 3300, 0, 2300, 70).delay (quarterD, 40, 27, 4000)
            .reverb (45, 90, 4, 45).level (-12).standardMacros (25);
        add ("Cinder Carillon", "Bell").osc (1, fm, 45).osc (2, fm, 40, 60).pitch (2, 7).filter (1, svfHigh, 2600, 20).amp (0, 2000, 0, 2300, 70)
            .reverb (30, 100, 3, 45).level (6).standardMacros (25);
        add ("Cold Bar", "Bell").osc (1, metal, 25).filter (1, svfBand, 300, 15).amp (0, 1200, 0, 2200, 70).reverb (30, 80, 3.5f, 45).level (1)
            .standardMacros (25);
        add ("Corroded Mallet", "Bell").osc (1, metal, 65).warp (1, wFold, 25).filter (1, svfHigh, 2900, 20).amp (0, 3000, 0, 1800, 70)
            .reverb (45, 70, 8, 45).level (-10).standardMacros (25);
        add ("Corroded Plate", "Bell").osc (1, fm, 75).filter (1, svfBand, 700, 20).amp (0, 1500, 0, 1200, 70).delay (eighthD, 30, 26, 4000)
            .reverb (45, 100, 8, 45).level (-13).standardMacros (25);
        add ("Cracked Resonator", "Bell").osc (1, metal, 15).filter (1, svfBand, 2900, 15).amp (0, 3800, 0, 2000, 70).delay (eighthD, 35, 24, 4000)
            .reverb (35, 80, 7, 45).level (-10).standardMacros (25);
        add ("Dead Tube", "Bell").osc (1, additive, 70).osc (2, analog, 60, 55).pitch (2, 19).filter (1, svfHigh, 400, 10).amp (0, 3200, 0, 2800, 70)
            .delay (eighthD, 40, 28, 4000).reverb (35, 80, 7.5f, 45).level (-13).standardMacros (25);
        add ("Derelict Bell", "Bell").osc (1, fm, 15).osc (2, metal, sinePos, 45).pitch (2, 24).filter (1, ladder12, 2700, 20)
            .amp (0, 2900, 0, 2900, 70).reverb (50, 90, 5.5f, 45).level (-10).standardMacros (25);
        add ("Derelict Toll", "Bell").osc (1, metal, 30).osc (2, analog, 60, 45).pitch (2, 19).filter (1, svfHigh, 700, 25).amp (0, 1400, 0, 2700, 70)
            .delay (eighthD, 40, 21, 4000).reverb (40, 85, 3.5f, 45).level (-8).standardMacros (25);
        add ("Distant Resonator", "Bell").osc (1, additive, 80).osc (2, fm, 40, 40).pitch (2, 24).filter (1, svfHigh, 2200, 10)
            .amp (0, 1700, 0, 2000, 70).reverb (50, 85, 4, 45).level (-7).standardMacros (25);
        add ("Frozen Ring", "Bell").osc (1, additive, 20).osc (2, metal, 40, 50).pitch (2, 24).env (2, 0, 1200, 0, 1500, 70)
            .mod (env2, "osc1Position", 25).warp (1, wMirror, 15).filter (1, svfHigh, 1100, 10).amp (0, 2600, 0, 1100, 70).reverb (50, 90, 6, 45)
            .level (-8).standardMacros (25);
        add ("Grey Clang", "Bell").osc (1, additive, 70).osc (2, fm, 60, 50).pitch (2, 12).env (2, 0, 2000, 0, 1500, 70)
            .mod (env2, "osc1Position", 40).filter (1, svfBand, 1100, 20).amp (0, 2500, 0, 2200, 70).delay (eighthD, 35, 22, 4000)
            .reverb (35, 85, 7.5f, 45).level (-10).standardMacros (25);
        add ("Grey Peal", "Bell").osc (1, fm, 20).osc (2, fm, sinePos, 45).pitch (2, 24).filter (1, svfBand, 2100, 15).amp (0, 2900, 0, 2400, 70)
            .delay (eighthD, 30, 27, 4000).reverb (40, 75, 6, 45).level (0).standardMacros (25);
        add ("Heavy Bar", "Bell").osc (1, metal, 50).osc (2, metal, sinePos, 40).pitch (2, 12).filter (1, svfHigh, 2200, 15)
            .amp (0, 2400, 0, 1500, 70).reverb (45, 80, 6.5f, 45).level (-8).standardMacros (25);
        add ("Hollow Plate", "Bell").osc (1, fm, 80).env (2, 0, 1800, 0, 1500, 70).mod (env2, "osc1Position", 45).filter (1, svfHigh, 2400, 15)
            .amp (0, 3400, 0, 1800, 70).delay (quarterD, 40, 26, 4000).reverb (40, 90, 4.5f, 45).level (-10).standardMacros (25);
        add ("Hollow Tine", "Bell").osc (1, additive, 85).env (2, 0, 1200, 0, 1500, 70).mod (env2, "osc1Position", 25).warp (1, wFold, 40)
            .filter (1, ladder12, 900, 20).amp (0, 1500, 0, 1600, 70).reverb (40, 90, 5, 45).level (-10).standardMacros (25);
        add ("Industrial Peal", "Bell").osc (1, fm, 90).osc (2, analog, 60, 50).pitch (2, 7).warp (1, wFm, 35).filter (1, svfHigh, 500, 20)
            .amp (0, 3500, 0, 2300, 70).reverb (35, 85, 4.5f, 45).level (-13).standardMacros (25);
        add ("Iron Bar", "Bell").osc (1, metal, 90).osc (2, fm, sinePos, 40).pitch (2, 12).filter (1, ladder12, 2500, 10).amp (0, 2700, 0, 2200, 70)
            .reverb (40, 95, 7.5f, 45).level (-4).standardMacros (25);
        add ("Leaden Chime", "Bell").osc (1, fm, 40).osc (2, fm, 40, 50).pitch (2, 12).warp (1, wFold, 25).filter (1, ladder12, 2300, 20)
            .amp (0, 2200, 0, 2700, 70).reverb (30, 95, 5.5f, 45).level (-10).standardMacros (25);
        add ("Nickel Bell", "Bell").osc (1, additive, 15).osc (2, analog, 40, 55).pitch (2, 24).filter (1, svfBand, 900, 25)
            .amp (0, 2900, 0, 1000, 70).reverb (50, 95, 5.5f, 45).level (-12).standardMacros (25);
        add ("Night Bowl", "Bell").osc (1, additive, 85).osc (2, metal, 60, 40).pitch (2, 19).env (2, 0, 1200, 0, 1500, 70)
            .mod (env2, "osc1Position", 30).filter (1, svfHigh, 1300, 10).amp (0, 1700, 0, 1500, 70).delay (quarterD, 40, 22, 4000)
            .reverb (45, 80, 7, 45).level (-10).standardMacros (25);
        add ("Oily Gong", "Bell").osc (1, metal, 55).osc (2, fm, 60, 40).pitch (2, 7).env (2, 0, 1700, 0, 1500, 70).mod (env2, "osc1Position", 30)
            .filter (1, ladder12, 900, 20).amp (0, 3400, 0, 2400, 70).delay (quarterD, 40, 20, 4000).reverb (40, 80, 4.5f, 45).level (-4)
            .standardMacros (25);
        add ("Oily Toll", "Bell").osc (1, metal, 50).filter (1, svfHigh, 1700, 15).amp (0, 2100, 0, 2100, 70).delay (eighthD, 35, 21, 4000)
            .reverb (40, 75, 3, 45).level (-8).standardMacros (25);
        add ("Pressed Plate", "Bell").osc (1, fm, 25).env (2, 0, 1400, 0, 1500, 70).mod (env2, "osc1Position", 40).filter (1, ladder12, 2800, 20)
            .amp (0, 1700, 0, 1300, 70).delay (eighthD, 45, 26, 4000).reverb (45, 95, 6.5f, 45).level (-10).standardMacros (25);
        add ("Prison Mallet", "Bell").osc (1, metal, 15).osc (2, metal, 40, 50).pitch (2, 19).env (2, 0, 1900, 0, 1500, 70)
            .mod (env2, "osc1Position", 35).filter (1, svfBand, 2100, 10).amp (0, 1300, 0, 1100, 70).delay (eighthD, 45, 26, 4000)
            .reverb (45, 85, 4, 45).level (-5).standardMacros (25);
        add ("Rotten Chime", "Bell").osc (1, metal, 15).env (2, 0, 800, 0, 1500, 70).mod (env2, "osc1Position", 50).filter (1, ladder12, 400, 25)
            .amp (0, 3900, 0, 3000, 70).delay (eighthD, 45, 25, 4000).reverb (50, 95, 6, 45).level (5).standardMacros (25);
        add ("Ruined Knell", "Bell").osc (1, fm, 50).osc (2, fm, 40, 60).pitch (2, 12).env (2, 0, 1100, 0, 1500, 70).mod (env2, "osc1Position", 50)
            .warp (1, wFm, 30).filter (1, ladder12, 1300, 25).amp (0, 1500, 0, 1400, 70).delay (eighthD, 30, 19, 4000).reverb (35, 90, 7, 45)
            .level (-10).standardMacros (25);
        add ("Scorched Ring", "Bell").osc (1, fm, 25).env (2, 0, 800, 0, 1500, 70).mod (env2, "osc1Position", 40).filter (1, svfHigh, 1100, 20)
            .amp (0, 1300, 0, 2700, 70).delay (eighthD, 35, 21, 4000).reverb (45, 90, 6.5f, 45).level (-6).standardMacros (25);
        add ("Severed Clapper", "Bell").osc (1, additive, 25).osc (2, fm, sinePos, 40).pitch (2, 19).env (2, 0, 1400, 0, 1500, 70)
            .mod (env2, "osc1Position", 50).filter (1, svfBand, 1300, 15).amp (0, 2500, 0, 1400, 70).reverb (40, 95, 4.5f, 45).level (-12)
            .standardMacros (25);
        add ("Severed Knell", "Bell").osc (1, fm, 85).osc (2, analog, 60, 50).pitch (2, 7).env (2, 0, 900, 0, 1500, 70).mod (env2, "osc1Position", 35)
            .filter (1, ladder12, 2700, 15).amp (0, 2300, 0, 2300, 70).delay (eighthD, 40, 27, 4000).reverb (50, 75, 5.5f, 45).level (-12)
            .standardMacros (25);
        add ("Sodium Bowl", "Bell").osc (1, fm, 50).filter (1, svfBand, 2600, 20).amp (0, 3100, 0, 2900, 70).reverb (35, 90, 4.5f, 45).level (-5)
            .standardMacros (25);
        add ("Steel Bell", "Bell").osc (1, fm, 30).filter (1, ladder12, 400, 15).amp (0, 2000, 0, 2300, 70).delay (eighthD, 40, 23, 4000)
            .reverb (40, 85, 5, 45).level (-10).standardMacros (25);
        add ("Sterile Clang", "Bell").osc (1, metal, 70).osc (2, fm, sinePos, 45).pitch (2, 12).env (2, 0, 1700, 0, 1500, 70)
            .mod (env2, "osc1Position", 25).warp (1, wFm, 35).filter (1, ladder12, 1700, 15).amp (0, 3200, 0, 1500, 70).reverb (30, 75, 4, 45)
            .level (-5).standardMacros (25);
        add ("Sullen Chime", "Bell").osc (1, metal, 20).env (2, 0, 1500, 0, 1500, 70).mod (env2, "osc1Position", 30).filter (1, svfBand, 1500, 10)
            .amp (0, 1900, 0, 2100, 70).delay (eighthD, 40, 25, 4000).reverb (30, 95, 8, 45).level (-4).standardMacros (25);
        add ("Tar Bowl", "Bell").osc (1, metal, 30).filter (1, ladder12, 2400, 20).amp (0, 3100, 0, 2200, 70).delay (eighthD, 40, 22, 4000)
            .reverb (40, 90, 3.5f, 45).level (-2).standardMacros (25);
        add ("Tolling Anvil", "Bell").osc (1, additive, 75).osc (2, fm, 40, 60).pitch (2, 19).warp (1, wFold, 25).filter (1, svfHigh, 1500, 20)
            .amp (0, 1600, 0, 2700, 70).delay (eighthD, 40, 24, 4000).reverb (45, 75, 3.5f, 45).level (-5).standardMacros (25);
        add ("Toxic Chime", "Bell").osc (1, metal, 85).osc (2, analog, 60, 45).pitch (2, 24).filter (1, svfBand, 400, 20).amp (0, 1700, 0, 1300, 70)
            .reverb (40, 80, 7, 45).level (-3).standardMacros (25);
        add ("Toxic Resonator", "Bell").osc (1, fm, 70).env (2, 0, 1000, 0, 1500, 70).mod (env2, "osc1Position", 45).filter (1, ladder12, 400, 10)
            .amp (0, 2500, 0, 1200, 70).delay (eighthD, 35, 25, 4000).reverb (45, 80, 4, 45).level (-3).standardMacros (25);

        // Sequence, by name.
        add ("Abandoned Pattern", "Sequence").osc (1, pulse, 35).unison (1, 3, 12, 70).filter (1, svfBand, 1300, 45)
            .lfoSync (1, square, sixteenth, 1, 15).mod (lfo1, "ampLevel", 33).amp (2, 300, 90, 200).delay (sixteenthD, 25, 16, 4000).level (-23)
            .standardMacros (25);
        add ("Abandoned Sequence", "Sequence").osc (1, metal, 30).filter (1, svfBand, 2400, 25).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100 }).mod (lfo1, "ampLevel", 36)
            .mod (lfo1, "filter1Cutoff", 8).amp (2, 300, 90, 200).delay (eighthT, 35, 19, 4000).drive (crush, 30).level (-22).standardMacros (25);
        add ("Abandoned Shuttle", "Sequence").osc (1, sync, 60).filter (1, ladder24, 900, 25).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100 })
            .mod (lfo1, "osc1Position", 24).amp (2, 300, 90, 200).delay (eighthD, 30, 23, 4000).level (-5).standardMacros (25);
        add ("Abandoned Signal", "Sequence").osc (1, pulse, 40).filter (1, ladder24, 2500, 35).lfoSync (1, square, eighth, 1, 15)
            .mod (lfo1, "ampLevel", 34).amp (2, 300, 90, 200).delay (eighthT, 45, 20, 4000).drive (crush, 25).level (-28).standardMacros (25);
        add ("Basalt Metronome", "Sequence").osc (1, sync, 65).filter (1, svfBand, 2100, 30).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 20, -90, 100, -40, 30, -60, 80, -20, 0, -80, 100, -30, 40, -70, 60, -10 }).mod (lfo1, "osc1Position", 38)
            .amp (2, 300, 90, 200).delay (sixteenthD, 30, 16, 4000).drive (hard, 25).level (-18).standardMacros (25);
        add ("Bitter Assembly", "Sequence").osc (1, analog, sawPos).filter (1, ladder24, 2300, 30).lfoSync (1, steps, eighth, 1)
            .stepPattern (1, { 100, -100, 50, -100, -100, 100, -100, 50, 100, -100, 50, -100, -100, 100, 25, -100 }).mod (lfo1, "ampLevel", 37)
            .mod (lfo1, "filter1Cutoff", 11).amp (2, 300, 90, 200).delay (eighthT, 30, 22, 4000).level (-26).standardMacros (25);
        add ("Black Pattern", "Sequence").osc (1, sync, 20).unison (1, 2, 11, 70).filter (1, ladder12, 2100, 40)
            .lfoSync (1, sawDown, sixteenth, 1, 15).mod (lfo1, "ampLevel", 39).amp (2, 300, 90, 200).delay (eighthT, 40, 24, 4000).level (-26)
            .standardMacros (25);
        add ("Buried Relay", "Sequence").osc (1, sync, 25).filter (1, ladder24, 1300, 40).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 60, -100, 100, 40, -100, -100, 100, -100, 60, -100, 100, -100, -100, 80, 100, -100 }).mod (lfo1, "ampLevel", 36)
            .mod (lfo1, "filter1Cutoff", 13).amp (2, 300, 90, 200).delay (eighthD, 40, 22, 4000).drive (crush, 30).level (-27).standardMacros (25);
        add ("Burnt Shuttle", "Sequence").osc (1, analog, sawPos).filter (1, svfBand, 2200, 30).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, 60, -100, 100, -100, -100, -100, 60, -100, -100, -100, 100, -100, -100, 60, -100 })
            .mod (lfo1, "filter1Cutoff", 38).amp (2, 300, 90, 200).delay (sixteenthD, 30, 16, 4000).level (-12).standardMacros (25);
        add ("Carbon Loom", "Sequence").osc (1, metal, 60).filter (1, ladder24, 2200, 30).lfoSync (1, steps, sixteenthT, 1)
            .stepPattern (1, { -100, -100, 100, 100, 100, -100, 100, -100, -100, -100, 100, -100, 100, 100, -100, 100 }).mod (lfo1, "ampLevel", 36)
            .mod (lfo1, "filter1Cutoff", 10).amp (2, 300, 90, 200).delay (sixteenthD, 25, 19, 4000).level (-17).standardMacros (25);
        add ("Choked Press", "Sequence").osc (1, pulse, 40).filter (1, ladder12, 1400, 45).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, -100, 60, -100, 100, -100, -100, -100, 60, -100, -100, -100, 100, -100, -100, 60 }).mod (lfo1, "ampLevel", 37)
            .mod (lfo1, "filter1Cutoff", 14).amp (2, 300, 90, 200).delay (sixteenthD, 30, 23, 4000).drive (crush, 35).level (-30).standardMacros (25);
        add ("Choked Ratchet", "Sequence").osc (1, pulse, 35).filter (1, ladder12, 1600, 35).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 0, -100, 0, 100, 0, -100, 0, 50, 0, -50, 0, 100, 0, -100, 0, 100 }).mod (lfo1, "osc1Position", 31)
            .amp (2, 300, 90, 200).delay (eighthD, 35, 20, 4000).drive (crush, 30).level (-14).standardMacros (25);
        add ("Chopped Ratchet", "Sequence").osc (1, growl, 30).filter (1, ladder24, 2000, 40).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -60, 100, -60, 20, -60, 60, -20, -60, -100, 100, -60, 20, -60, 80, -60, 40 }).mod (lfo1, "ampLevel", 35)
            .mod (lfo1, "filter1Cutoff", 9).amp (2, 300, 90, 200).delay (eighthD, 30, 22, 4000).drive (hard, 25).level (-29).standardMacros (25);
        add ("Chrome Assembly", "Sequence").osc (1, pulse, 50).filter (1, ladder24, 2100, 25).lfoSync (1, square, sixteenth, 1, 15)
            .mod (lfo1, "ampLevel", 36).amp (2, 300, 90, 200).delay (eighthD, 35, 21, 4000).level (-27).standardMacros (25);
        add ("Cinder Circuit", "Sequence").osc (1, sync, 30).unison (1, 2, 9, 70).filter (1, ladder12, 2500, 40).lfoSync (1, square, eighth, 1, 15)
            .mod (lfo1, "ampLevel", 37).amp (2, 300, 90, 200).delay (sixteenthD, 35, 24, 4000).drive (crush, 30).level (-29).standardMacros (25);
        add ("Cinder Cog", "Sequence").osc (1, growl, 65).filter (1, ladder24, 1700, 50).lfoSync (1, square, sixteenth, 1, 15)
            .mod (lfo1, "ampLevel", 34).amp (2, 300, 90, 200).delay (eighthT, 35, 17, 4000).drive (hard, 15).level (-31).standardMacros (25);
        add ("Clocked Conveyor", "Sequence").osc (1, pulse, 25).unison (1, 2, 9, 70).filter (1, svfBand, 1400, 35).lfoSync (1, steps, sixteenthT, 1)
            .stepPattern (1, { 0, -100, 0, 50, 0, -50, 0, 100, 0, -100, 0, 100, 0, -100, 0, 100 }).mod (lfo1, "ampLevel", 38)
            .mod (lfo1, "filter1Cutoff", 11).amp (2, 300, 90, 200).delay (eighthD, 40, 17, 4000).drive (hard, 30).level (-27).standardMacros (25);
        add ("Clocked Relay", "Sequence").osc (1, sync, 30).filter (1, ladder12, 2400, 25).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -20, -50, -80, -100, 100, 70, 40, 10, -20, -50, -80, -100, 100, 70, 40, 10 }).mod (lfo1, "ampLevel", 34)
            .mod (lfo1, "filter1Cutoff", 12).amp (2, 300, 90, 200).delay (eighthD, 40, 18, 4000).level (-23).standardMacros (25);
        add ("Cobalt Metronome", "Sequence").osc (1, analog, sawPos).filter (1, ladder24, 1500, 30).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, 100, 100, -100, 100, -100, -100, 100, 100, 100, -100, 100, -100, -100, -100, 100 })
            .mod (lfo1, "filter1Cutoff", 24).amp (2, 300, 90, 200).delay (sixteenthD, 30, 17, 4000).level (-10).standardMacros (25);
        add ("Cold Circuit", "Sequence").osc (1, sync, 25).unison (1, 3, 13, 70).filter (1, ladder24, 2300, 40).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 100, -30, 40, -70, 60, -10, 20, -90, 100, -40, 30, -60, 80, -20, 0, -80 }).mod (lfo1, "ampLevel", 39)
            .mod (lfo1, "filter1Cutoff", 12).amp (2, 300, 90, 200).delay (eighthT, 30, 23, 4000).drive (crush, 20).level (-26).standardMacros (25);
        add ("Cold Line", "Sequence").osc (1, analog, sawPos).filter (1, ladder12, 1700, 25).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100 })
            .mod (lfo1, "filter1Cutoff", 32).amp (2, 300, 90, 200).delay (eighthD, 30, 21, 4000).drive (crush, 25).level (-12).standardMacros (25);
        add ("Concrete Press", "Sequence").osc (1, analog, sawPos).unison (1, 3, 13, 70).filter (1, ladder12, 2400, 30)
            .lfoSync (1, square, sixteenth, 1, 15).mod (lfo1, "ampLevel", 31).amp (2, 300, 90, 200).delay (eighthD, 30, 19, 4000).level (-26)
            .standardMacros (25);
        add ("Concrete Shuttle", "Sequence").osc (1, pulse, 55).unison (1, 2, 9, 70).filter (1, ladder24, 1800, 50).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, 100, -100, 60, -100, 100, -100, -100, 80, 100, -100, 60, -100, 100, 40, -100 }).mod (lfo1, "osc1Position", 24)
            .amp (2, 300, 90, 200).delay (eighthD, 45, 22, 4000).level (-12).standardMacros (25);
        add ("Corroded Circuit", "Sequence").osc (1, growl, 50).filter (1, svfBand, 2100, 50).lfoSync (1, sawDown, eighth, 1, 15)
            .mod (lfo1, "ampLevel", 32).amp (2, 300, 90, 200).delay (eighthD, 25, 20, 4000).drive (hard, 30).level (-24).standardMacros (25);
        add ("Dead Circuit", "Sequence").osc (1, sync, 70).filter (1, svfBand, 2200, 35).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 0, -100, 0, 100, 0, -100, 0, 100, 0, -100, 0, 50, 0, -50, 0, 100 }).mod (lfo1, "osc1Position", 36)
            .amp (2, 300, 90, 200).delay (sixteenthD, 40, 25, 4000).drive (crush, 20).level (-12).standardMacros (25);
        add ("Derelict Metronome", "Sequence").osc (1, analog, sawPos).filter (1, ladder12, 2100, 40).lfoSync (1, sawDown, sixteenth, 1, 15)
            .mod (lfo1, "ampLevel", 27).amp (2, 300, 90, 200).delay (eighthD, 35, 21, 4000).level (-22).standardMacros (25);
        add ("Derelict Sequence", "Sequence").osc (1, growl, 70).filter (1, svfBand, 1900, 35).lfoSync (1, steps, eighth, 1)
            .stepPattern (1, { 60, -20, -60, -100, 100, -60, 20, -60, 80, -60, 40, -60, 100, -60, 20, -60 }).mod (lfo1, "ampLevel", 31)
            .mod (lfo1, "filter1Cutoff", 9).amp (2, 300, 90, 200).delay (eighthT, 45, 24, 4000).level (-16).standardMacros (25);
        add ("Feral Conveyor", "Sequence").osc (1, pulse, 25).filter (1, ladder24, 2400, 30).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 100, -100, -100, 100, -100, 100, -100, -100, 100, -100, 100, -100, -100, 100, -100, -100 }).mod (lfo1, "ampLevel", 38)
            .mod (lfo1, "filter1Cutoff", 13).amp (2, 300, 90, 200).delay (eighthD, 25, 19, 4000).drive (hard, 20).level (-31).standardMacros (25);
        add ("Feral Relay", "Sequence").osc (1, analog, sawPos).filter (1, svfBand, 2400, 35).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, 100, -100, 100, -100, -100, 100, -100, -100, 100, -100, -100, 100, -100, 100, -100 }).mod (lfo1, "ampLevel", 36)
            .mod (lfo1, "filter1Cutoff", 12).amp (2, 300, 90, 200).delay (sixteenthD, 25, 16, 4000).drive (crush, 30).level (-28).standardMacros (25);
        add ("Feral Signal", "Sequence").osc (1, metal, 20).filter (1, ladder12, 1300, 30).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 100, -100, -100, -100, 60, -100, -100, -100, 100, -100, -100, 60, -100, -100, 60, -100 })
            .mod (lfo1, "osc1Position", 30).amp (2, 300, 90, 200).delay (eighthT, 35, 19, 4000).level (-6).standardMacros (25);
        add ("Frozen Counter", "Sequence").osc (1, growl, 20).filter (1, svfBand, 1500, 40).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100 })
            .mod (lfo1, "osc1Position", 23).amp (2, 300, 90, 200).delay (sixteenthD, 35, 24, 4000).level (-6).standardMacros (25);
        add ("Gated Metronome", "Sequence").osc (1, sync, 30).filter (1, ladder24, 2200, 35).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, -100, 100, -100, 100, 100, -100, 100, -100, -100, 100, 100, 100, -100, 100, -100 }).mod (lfo1, "ampLevel", 35)
            .mod (lfo1, "filter1Cutoff", 10).amp (2, 300, 90, 200).delay (eighthT, 35, 21, 4000).level (-24).standardMacros (25);
        add ("Grey Loom", "Sequence").osc (1, metal, 35).unison (1, 2, 11, 70).filter (1, svfBand, 2000, 45).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, 100, -100, -100, 100, -100, -100, 100, -100, -100, 100, -100, 100, -100, -100, 100 }).mod (lfo1, "ampLevel", 33)
            .mod (lfo1, "filter1Cutoff", 13).amp (2, 300, 90, 200).delay (eighthD, 35, 21, 4000).level (-20).standardMacros (25);
        add ("Grey Machine", "Sequence").osc (1, pulse, 70).unison (1, 2, 10, 70).filter (1, ladder24, 2100, 25).lfoSync (1, steps, eighth, 1)
            .stepPattern (1, { 50, 0, -50, 0, 100, 0, -100, 0, 100, 0, -100, 0, 100, 0, -100, 0 }).mod (lfo1, "ampLevel", 39)
            .mod (lfo1, "filter1Cutoff", 15).amp (2, 300, 90, 200).delay (eighthD, 35, 22, 4000).level (-27).standardMacros (25);
        add ("Heavy Gate", "Sequence").osc (1, analog, sawPos).filter (1, ladder24, 2200, 45).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, 50, -100, -100, 100, 25, -100, 100, -100, 50, -100, -100, 100, -100, 50, 100 }).mod (lfo1, "filter1Cutoff", 31)
            .amp (2, 300, 90, 200).delay (eighthD, 35, 23, 4000).level (-10).standardMacros (25);
        add ("Hollow Counter", "Sequence").osc (1, analog, sawPos).filter (1, ladder24, 1500, 35).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, -100, 100, -100, 100, -100, -100, 100, -100, -100, 100, -100, -100, 100, -100, 100 }).mod (lfo1, "ampLevel", 34)
            .mod (lfo1, "filter1Cutoff", 11).amp (2, 300, 90, 200).delay (sixteenthD, 35, 17, 4000).level (-25).standardMacros (25);
        add ("Hollow Loom", "Sequence").osc (1, pulse, 45).unison (1, 3, 13, 70).filter (1, svfBand, 2200, 25).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 100, -100, 50, -100, -100, 100, 25, -100, 100, -100, 50, -100, -100, 100, -100, 50 }).mod (lfo1, "ampLevel", 33)
            .mod (lfo1, "filter1Cutoff", 9).amp (2, 300, 90, 200).delay (eighthT, 25, 25, 4000).level (-20).standardMacros (25);
        add ("Iron Machine", "Sequence").osc (1, analog, sawPos).unison (1, 2, 9, 70).filter (1, ladder12, 1900, 45).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -10, 20, -90, 100, -40, 30, -60, 80, -20, 0, -80, 100, -30, 40, -70, 60 }).mod (lfo1, "ampLevel", 36)
            .mod (lfo1, "filter1Cutoff", 15).amp (2, 300, 90, 200).delay (sixteenthD, 30, 25, 4000).level (-27).standardMacros (25);
        add ("Leaden Assembly", "Sequence").osc (1, sync, 30).filter (1, ladder24, 900, 35).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, 100, -100, 100, -100, -100, 100, -100, 100, -100, -100, 100, -100, -100, 100, -100 })
            .mod (lfo1, "osc1Position", 24).amp (2, 300, 90, 200).delay (sixteenthD, 25, 20, 4000).level (-7).standardMacros (25);
        add ("Looping Pump", "Sequence").osc (1, metal, 25).filter (1, svfBand, 1600, 45).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, 100, 25, -100, 100, -100, 50, -100, -100, 100, -100, 50, 100, -100, 50, -100 }).mod (lfo1, "ampLevel", 37)
            .mod (lfo1, "filter1Cutoff", 11).amp (2, 300, 90, 200).delay (sixteenthD, 40, 17, 4000).level (-20).standardMacros (25);
        add ("Mechanical Conveyor", "Sequence").osc (1, analog, sawPos).unison (1, 2, 13, 70).filter (1, ladder12, 1300, 45)
            .lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, 100, -100, -100, 60, -100, -100, 60, -100, 100, -100, -100, -100, 60, -100, -100 })
            .mod (lfo1, "filter1Cutoff", 34).amp (2, 300, 90, 200).delay (eighthT, 30, 20, 4000).level (-12).standardMacros (25);
        add ("Molten Machine", "Sequence").osc (1, metal, 50).filter (1, ladder12, 2300, 45).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 100, -60, 20, -60, 60, -20, -60, -100, 100, -60, 20, -60, 80, -60, 40, -60 }).mod (lfo1, "osc1Position", 31)
            .amp (2, 300, 90, 200).delay (sixteenthD, 25, 21, 4000).level (-6).standardMacros (25);
        add ("Nickel Conveyor", "Sequence").osc (1, analog, sawPos).filter (1, ladder24, 1500, 25).lfoSync (1, square, eighth, 1, 15)
            .mod (lfo1, "ampLevel", 27).amp (2, 300, 90, 200).delay (eighthD, 30, 21, 4000).drive (crush, 15).level (-24).standardMacros (25);
        add ("Nickel Pattern", "Sequence").osc (1, analog, sawPos).filter (1, ladder24, 2200, 50).lfoSync (1, sawDown, eighth, 1, 15)
            .mod (lfo1, "ampLevel", 28).amp (2, 300, 90, 200).delay (sixteenthD, 40, 16, 4000).level (-21).standardMacros (25);
        add ("Nickel Sequence", "Sequence").osc (1, sync, 65).filter (1, svfBand, 800, 30).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 10, -20, -50, -80, -100, 100, 70, 40, 10, -20, -50, -80, -100, 100, 70, 40 }).mod (lfo1, "osc1Position", 23)
            .amp (2, 300, 90, 200).delay (eighthT, 35, 20, 4000).level (-10).standardMacros (25);
        add ("Night Clock", "Sequence").osc (1, analog, sawPos).unison (1, 2, 13, 70).filter (1, ladder12, 1600, 40).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, 100, 100, 100, -100, 100, -100, -100, -100, 100, -100, 100, 100, -100, 100, -100 })
            .mod (lfo1, "filter1Cutoff", 27).amp (2, 300, 90, 200).delay (eighthT, 35, 23, 4000).drive (hard, 15).level (-17).standardMacros (25);
        add ("Night Cog", "Sequence").osc (1, analog, sawPos).filter (1, svfBand, 2100, 40).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, -100, 100, -100, 50, 100, -100, 50, -100, -100, 100, 25, -100, 100, -100, 50 }).mod (lfo1, "filter1Cutoff", 33)
            .amp (2, 300, 90, 200).delay (eighthT, 30, 19, 4000).level (-12).standardMacros (25);
        add ("Night Gear", "Sequence").osc (1, analog, sawPos).unison (1, 2, 12, 70).filter (1, svfBand, 1100, 35).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, 100, -100, -100, 100, -100, 100, -100, -100, 100, -100, -100, 100, -100, -100, 100 }).mod (lfo1, "ampLevel", 32)
            .mod (lfo1, "filter1Cutoff", 10).amp (2, 300, 90, 200).delay (eighthD, 45, 22, 4000).level (-26).standardMacros (25);
        add ("Null Conveyor", "Sequence").osc (1, growl, 40).filter (1, svfBand, 1200, 45).lfoSync (1, sawDown, eighth, 1, 15)
            .mod (lfo1, "ampLevel", 36).amp (2, 300, 90, 200).delay (eighthD, 40, 19, 4000).level (-19).standardMacros (25);
        add ("Null Gear", "Sequence").osc (1, metal, 60).filter (1, ladder24, 1600, 45).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 0, -50, 0, 100, 0, -100, 0, 100, 0, -100, 0, 100, 0, -100, 0, 50 }).mod (lfo1, "osc1Position", 27)
            .amp (2, 300, 90, 200).delay (eighthD, 35, 20, 4000).level (-1).standardMacros (25);
        add ("Obsidian Assembly", "Sequence").osc (1, pulse, 45).unison (1, 2, 8, 70).filter (1, svfBand, 1000, 30).lfoSync (1, steps, eighth, 1)
            .stepPattern (1, { 100, -100, -100, -100, 100, -100, 100, 100, -100, 100, -100, -100, 100, 100, 100, -100 }).mod (lfo1, "ampLevel", 31)
            .mod (lfo1, "filter1Cutoff", 11).amp (2, 300, 90, 200).delay (eighthT, 30, 21, 4000).drive (crush, 35).level (-22).standardMacros (25);
        add ("Obsidian Pump", "Sequence").osc (1, growl, 50).filter (1, ladder12, 900, 45).lfoSync (1, steps, sixteenthT, 1)
            .stepPattern (1, { 100, 40, -100, -100, 100, -100, 60, -100, 100, -100, -100, 80, 100, -100, 60, -100 }).mod (lfo1, "ampLevel", 31)
            .mod (lfo1, "filter1Cutoff", 10).amp (2, 300, 90, 200).delay (eighthT, 45, 21, 4000).level (-25).standardMacros (25);
        add ("Oily Pattern", "Sequence").osc (1, growl, 55).filter (1, ladder12, 1700, 45).lfoSync (1, steps, sixteenthT, 1)
            .stepPattern (1, { -100, -100, 100, -100, 100, -100, -100, 100, -100, 100, -100, -100, 100, -100, -100, 100 }).mod (lfo1, "ampLevel", 32)
            .mod (lfo1, "filter1Cutoff", 15).amp (2, 300, 90, 200).delay (eighthT, 30, 23, 4000).level (-24).standardMacros (25);
        add ("Pressed Ratchet", "Sequence").osc (1, metal, 50).unison (1, 3, 13, 70).filter (1, ladder12, 1400, 30).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, 50, -100, -100, 100, -100, 50, 100, -100, 50, -100, -100, 100, 25, -100, 100 }).mod (lfo1, "osc1Position", 31)
            .amp (2, 300, 90, 200).delay (eighthT, 45, 16, 4000).level (-6).standardMacros (25);
        add ("Repeating Machine", "Sequence").osc (1, pulse, 45).filter (1, ladder12, 900, 50).lfoSync (1, steps, sixteenthT, 1)
            .stepPattern (1, { 20, -60, 60, -20, -60, -100, 100, -60, 20, -60, 80, -60, 40, -60, 100, -60 }).mod (lfo1, "ampLevel", 32)
            .mod (lfo1, "filter1Cutoff", 14).amp (2, 300, 90, 200).delay (sixteenthD, 35, 25, 4000).level (-26).standardMacros (25);
        add ("Repeating Signal", "Sequence").osc (1, analog, sawPos).unison (1, 3, 14, 70).filter (1, ladder24, 1400, 40)
            .lfoSync (1, sawDown, eighth, 1, 15).mod (lfo1, "ampLevel", 33).amp (2, 300, 90, 200).delay (eighthT, 40, 16, 4000).level (-23)
            .standardMacros (25);
        add ("Ruined Circuit", "Sequence").osc (1, growl, 60).filter (1, ladder12, 1500, 35).lfoSync (1, steps, eighth, 1)
            .stepPattern (1, { 50, -100, -100, 100, 25, -100, 100, -100, 50, -100, -100, 100, -100, 50, 100, -100 }).mod (lfo1, "ampLevel", 31)
            .mod (lfo1, "filter1Cutoff", 9).amp (2, 300, 90, 200).delay (eighthT, 45, 23, 4000).drive (crush, 35).level (-26).standardMacros (25);
        add ("Ruined Clock", "Sequence").osc (1, growl, 50).unison (1, 3, 8, 70).filter (1, ladder24, 2000, 40).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, 100, 70, 40, 10, -20, -50, -80, -100, 100, 70, 40, 10, -20, -50, -80 }).mod (lfo1, "osc1Position", 33)
            .amp (2, 300, 90, 200).delay (sixteenthD, 45, 17, 4000).drive (crush, 30).level (-12).standardMacros (25);
        add ("Ruined Shuttle", "Sequence").osc (1, pulse, 40).filter (1, svfBand, 2300, 45).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, 100, -100, -100, 100, -100, 100, -100, -100, 100, -100, 100, -100, -100, 100, -100 }).mod (lfo1, "ampLevel", 38)
            .mod (lfo1, "filter1Cutoff", 10).amp (2, 300, 90, 200).delay (eighthT, 35, 21, 4000).level (-25).standardMacros (25);
        add ("Scorched Assembly", "Sequence").osc (1, pulse, 25).filter (1, svfBand, 1200, 50).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 60, -100, -100, 60, -100, 100, -100, -100, -100, 60, -100, -100, -100, 100, -100, -100 }).mod (lfo1, "ampLevel", 39)
            .mod (lfo1, "filter1Cutoff", 14).amp (2, 300, 90, 200).delay (sixteenthD, 25, 23, 4000).level (-19).standardMacros (25);
        add ("Scorched Counter", "Sequence").osc (1, sync, 55).filter (1, svfBand, 1400, 30).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, 100, -100, 100, -100, -100, 100, -100, -100, 100, -100, -100, 100, -100, 100, -100 }).mod (lfo1, "ampLevel", 37)
            .mod (lfo1, "filter1Cutoff", 11).amp (2, 300, 90, 200).delay (eighthT, 35, 17, 4000).level (-27).standardMacros (25);
        add ("Scorched Machine", "Sequence").osc (1, analog, sawPos).filter (1, svfBand, 1200, 35).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100, 100, -100 })
            .mod (lfo1, "filter1Cutoff", 23).amp (2, 300, 90, 200).delay (sixteenthD, 40, 17, 4000).level (-12).standardMacros (25);
        add ("Shadow Pattern", "Sequence").osc (1, pulse, 25).unison (1, 2, 10, 70).filter (1, ladder12, 1100, 35).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -100, -100, 80, 100, -100, 60, -100, 100, 40, -100, -100, 100, -100, 60, -100, 100 }).mod (lfo1, "osc1Position", 36)
            .amp (2, 300, 90, 200).delay (eighthT, 45, 20, 4000).drive (hard, 25).level (-20).standardMacros (25);
        add ("Soot Assembly", "Sequence").osc (1, metal, 25).filter (1, svfBand, 900, 35).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { -80, -100, 100, 70, 40, 10, -20, -50, -80, -100, 100, 70, 40, 10, -20, -50 }).mod (lfo1, "osc1Position", 34)
            .amp (2, 300, 90, 200).delay (eighthT, 30, 18, 4000).drive (crush, 15).level (-6).standardMacros (25);
        add ("Soot Clock", "Sequence").osc (1, metal, 40).filter (1, svfBand, 1400, 35).lfoSync (1, steps, sixteenthT, 1)
            .stepPattern (1, { 100, -100, 100, 100, -100, 100, -100, -100, 100, 100, 100, -100, 100, -100, -100, -100 }).mod (lfo1, "ampLevel", 33)
            .mod (lfo1, "filter1Cutoff", 10).amp (2, 300, 90, 200).delay (sixteenthD, 40, 19, 4000).drive (hard, 20).level (-27).standardMacros (25);
        add ("Steel Line", "Sequence").osc (1, growl, 25).unison (1, 3, 9, 70).filter (1, ladder12, 1500, 45).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 100, -100, 100, -100, -100, 100, 100, 100, -100, 100, -100, -100, -100, 100, -100, 100 })
            .mod (lfo1, "osc1Position", 36).amp (2, 300, 90, 200).delay (sixteenthD, 35, 19, 4000).level (-14).standardMacros (25);
        add ("Steel Ratchet", "Sequence").osc (1, pulse, 50).unison (1, 2, 14, 70).filter (1, ladder12, 1000, 50).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 80, -60, 40, -60, 100, -60, 20, -60, 60, -20, -60, -100, 100, -60, 20, -60 }).mod (lfo1, "osc1Position", 33)
            .amp (2, 300, 90, 200).delay (sixteenthD, 45, 17, 4000).level (-14).standardMacros (25);
        add ("Sullen Metronome", "Sequence").osc (1, analog, sawPos).filter (1, ladder12, 1000, 40).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 0, -100, 0, 100, 0, -100, 0, 50, 0, -50, 0, 100, 0, -100, 0, 100 }).mod (lfo1, "filter1Cutoff", 25)
            .amp (2, 300, 90, 200).delay (eighthT, 35, 17, 4000).level (-14).standardMacros (25);
        add ("Sunken Loom", "Sequence").osc (1, growl, 25).filter (1, ladder24, 2300, 35).lfoSync (1, sawDown, eighth, 1, 15)
            .mod (lfo1, "ampLevel", 34).amp (2, 300, 90, 200).delay (eighthD, 30, 16, 4000).level (-23).standardMacros (25);
        add ("Sunken Ratchet", "Sequence").osc (1, sync, 35).filter (1, ladder24, 2400, 45).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 100, -100, -100, 100, 100, 100, -100, 100, -100, -100, -100, 100, -100, 100, 100, -100 }).mod (lfo1, "ampLevel", 31)
            .mod (lfo1, "filter1Cutoff", 12).amp (2, 300, 90, 200).delay (eighthT, 35, 23, 4000).level (-23).standardMacros (25);
        add ("Vacant Sequence", "Sequence").osc (1, analog, sawPos).filter (1, svfBand, 1200, 40).lfoSync (1, sawDown, eighth, 1, 15)
            .mod (lfo1, "ampLevel", 37).amp (2, 300, 90, 200).delay (sixteenthD, 25, 25, 4000).drive (hard, 30).level (-28).standardMacros (25);
        add ("Vacant Shuttle", "Sequence").osc (1, metal, 25).filter (1, ladder12, 1600, 35).lfoSync (1, square, eighth, 1, 15)
            .mod (lfo1, "ampLevel", 35).amp (2, 300, 90, 200).delay (eighthT, 45, 21, 4000).level (-18).standardMacros (25);
        add ("Void Clock", "Sequence").osc (1, sync, 30).filter (1, ladder24, 2400, 50).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 40, 10, -20, -50, -80, -100, 100, 70, 40, 10, -20, -50, -80, -100, 100, 70 }).mod (lfo1, "osc1Position", 20)
            .amp (2, 300, 90, 200).delay (sixteenthD, 25, 24, 4000).level (-10).standardMacros (25);
        add ("Void Metronome", "Sequence").osc (1, analog, sawPos).filter (1, svfBand, 2300, 45).lfoSync (1, steps, sixteenthT, 1)
            .stepPattern (1, { 0, -80, 100, -30, 40, -70, 60, -10, 20, -90, 100, -40, 30, -60, 80, -20 }).mod (lfo1, "ampLevel", 35)
            .mod (lfo1, "filter1Cutoff", 10).amp (2, 300, 90, 200).delay (sixteenthD, 30, 16, 4000).level (-23).standardMacros (25);
        add ("Void Pattern", "Sequence").osc (1, sync, 30).filter (1, ladder12, 1100, 50).lfoSync (1, steps, sixteenth, 1)
            .stepPattern (1, { 100, -100, 100, -100, -100, -100, 100, -100, 100, 100, -100, 100, -100, -100, 100, 100 })
            .mod (lfo1, "osc1Position", 34).amp (2, 300, 90, 200).delay (eighthD, 40, 24, 4000).level (-12).standardMacros (25);

        // Riser, by name.
        add ("Ash Countdown", "Riser").set ("osc1On", 0).noise (75, 1).filter (1, svfBand, 800, 25).env (2, 6000, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 40).amp (4500, 1000, 100, 600).lfo (1, sawUp, 5.5f, 1).mod (lfo1, "filter1Cutoff", 9)
            .delay (eighthD, 55, 21, 4000).reverb (40, 90, 6, 45).level (6).standardMacros (25);
        add ("Basalt Dive", "Riser").set ("osc1On", 0).noise (90, 0).filter (1, svfBand, 3500, 45).env (2, 0, 6000, 0, 6000, 60)
            .mod (env2, "filter1Cutoff", 45).amp (0, 6000, 0, 1900).delay (sixteenthD, 50, 25, 4000).reverb (45, 90, 5.5f, 45).level (-1)
            .standardMacros (25);
        add ("Basalt Riser", "Riser").set ("osc1On", 0).noise (60, 0).filter (1, ladder24, 600, 30).env (2, 6500, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 50).amp (3000, 1000, 100, 600).reverb (40, 90, 4.5f, 45).level (6).standardMacros (25);
        add ("Basalt Surge", "Riser").osc (1, metal, 45).unison (1, 6, 21, 100).filter (1, ladder24, 7500, 45).env (2, 0, 6000, 0, 6000, 60)
            .mod (env2, "filter1Cutoff", 35).mod (env2, "osc1Pitch", 35).amp (0, 6000, 0, 1800).lfo (1, sine, 2.5f, 1).mod (lfo1, "filter1Cutoff", 7)
            .reverb (50, 90, 6.5f, 45).level (-5).standardMacros (25);
        add ("Brutal Tension", "Riser").set ("osc1On", 0).noise (85, 1).filter (1, svfHigh, 6500, 45).env (2, 0, 6000, 0, 6000, 60)
            .mod (env2, "filter1Cutoff", 45).amp (0, 6000, 0, 1700).lfo (1, sine, 3, 1).mod (lfo1, "filter1Cutoff", 10).reverb (40, 90, 7, 45)
            .level (6).standardMacros (25);
        add ("Brutal Whoosh", "Riser").osc (1, sync, 55).unison (1, 6, 20, 100).filter (1, svfHigh, 3000, 30).env (2, 0, 5500, 0, 5500, 60)
            .mod (env2, "filter1Cutoff", 45).mod (env2, "osc1Pitch", 40).amp (0, 5500, 0, 1600).delay (eighthD, 45, 29, 4000).reverb (45, 90, 7, 45)
            .level (6).standardMacros (25);
        add ("Choked Dive", "Riser").osc (1, metal, 50).unison (1, 4, 16, 100).noise (50, 0).filter (1, svfBand, 5000, 35)
            .env (2, 0, 5000, 0, 5000, 60).mod (env2, "filter1Cutoff", 35).mod (env2, "osc1Pitch", 35).amp (0, 5000, 0, 2000).lfo (1, sawUp, 2.5f, 1)
            .mod (lfo1, "filter1Cutoff", 7).reverb (40, 90, 6, 45).level (2).standardMacros (25);
        add ("Chrome Tension", "Riser").osc (1, sync, 45).unison (1, 6, 32, 100).noise (40, 0).filter (1, svfBand, 400, 30)
            .env (2, 7000, 1000, 100, 500, -50).mod (env2, "filter1Cutoff", 60).mod (env2, "osc1Pitch", 20).amp (2000, 1000, 100, 900)
            .delay (sixteenthD, 45, 24, 4000).reverb (45, 90, 7.5f, 45).level (-12).standardMacros (25);
        add ("Climbing Tension", "Riser").osc (1, growl, 35).unison (1, 6, 24, 100).filter (1, ladder24, 4000, 40).env (2, 0, 3000, 0, 3000, 60)
            .mod (env2, "filter1Cutoff", 35).mod (env2, "osc1Pitch", 30).amp (0, 3000, 0, 1900).reverb (40, 90, 8, 45).level (-10).standardMacros (25);
        add ("Cobalt Surge", "Riser").osc (1, sync, 55).unison (1, 3, 33, 100).filter (1, svfHigh, 400, 35).env (2, 6500, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 45).mod (env2, "osc1Pitch", 15).amp (5000, 1000, 100, 600).reverb (50, 90, 5, 45).level (-10)
            .standardMacros (25);
        add ("Collapsing Alarm", "Riser").osc (1, growl, 50).unison (1, 3, 18, 100).noise (30, 0).filter (1, ladder24, 700, 45)
            .env (2, 3500, 1000, 100, 500, -50).mod (env2, "filter1Cutoff", 40).mod (env2, "osc1Pitch", 15).amp (4500, 1000, 100, 600)
            .lfo (1, sawUp, 7, 1).mod (lfo1, "filter1Cutoff", 11).delay (eighthD, 50, 26, 4000).reverb (50, 90, 6.5f, 45).level (-6)
            .standardMacros (25);
        add ("Collapsing Ignition", "Riser").osc (1, analog, sawPos).unison (1, 3, 21, 100).noise (50, 0).filter (1, svfBand, 6000, 35)
            .env (2, 0, 3500, 0, 3500, 60).mod (env2, "filter1Cutoff", 50).mod (env2, "osc1Pitch", 40).amp (0, 3500, 0, 2100)
            .reverb (45, 90, 6.5f, 45).level (3).standardMacros (25);
        add ("Collapsing Lift", "Riser").osc (1, metal, 40).unison (1, 6, 29, 100).filter (1, svfHigh, 5500, 40).env (2, 0, 3000, 0, 3000, 60)
            .mod (env2, "filter1Cutoff", 45).mod (env2, "osc1Pitch", 45).amp (0, 3000, 0, 2100).delay (sixteenthD, 50, 28, 4000)
            .reverb (40, 90, 7, 45).level (6).standardMacros (25);
        add ("Concrete Surge", "Riser").set ("osc1On", 0).noise (60, 1).filter (1, svfBand, 700, 35).env (2, 5000, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 50).amp (4500, 1000, 100, 800).delay (sixteenthD, 55, 30, 4000).reverb (45, 90, 7.5f, 45).level (6)
            .standardMacros (25);
        add ("Corroded Ignition", "Riser").osc (1, metal, 50).unison (1, 4, 19, 100).noise (50, 0).filter (1, svfHigh, 3500, 30)
            .env (2, 0, 6500, 0, 6500, 60).mod (env2, "filter1Cutoff", 40).mod (env2, "osc1Pitch", 35).amp (0, 6500, 0, 1900).reverb (40, 90, 7, 45)
            .level (5).standardMacros (25);
        add ("Corroded Siren", "Riser").osc (1, metal, 35).unison (1, 4, 20, 100).filter (1, svfBand, 600, 25).env (2, 7000, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 40).mod (env2, "osc1Pitch", 30).amp (3500, 1000, 100, 700).delay (eighthD, 50, 26, 4000)
            .reverb (45, 90, 6, 45).level (-2).standardMacros (25);
        add ("Derelict Ramp", "Riser").osc (1, sync, 30).unison (1, 3, 16, 100).noise (50, 0).filter (1, svfHigh, 4000, 45)
            .env (2, 0, 6000, 0, 6000, 60).mod (env2, "filter1Cutoff", 35).mod (env2, "osc1Pitch", 50).amp (0, 6000, 0, 2300).lfo (1, sawUp, 4.5f, 1)
            .mod (lfo1, "filter1Cutoff", 8).reverb (45, 90, 7, 45).level (3).standardMacros (25);
        add ("Derelict Tension", "Riser").osc (1, metal, 50).unison (1, 6, 15, 100).filter (1, svfBand, 500, 25).env (2, 4000, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 50).mod (env2, "osc1Pitch", 30).amp (5000, 1000, 100, 600).delay (eighthD, 50, 21, 4000)
            .reverb (50, 90, 4, 45).level (1).standardMacros (25);
        add ("Draining Ignition", "Riser").set ("osc1On", 0).noise (90, 0).filter (1, svfHigh, 900, 45).env (2, 3500, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 55).amp (3500, 1000, 100, 600).lfo (1, sine, 3, 1).mod (lfo1, "filter1Cutoff", 11).reverb (45, 90, 7, 45)
            .level (-7).standardMacros (25);
        add ("Draining Swell", "Riser").osc (1, sync, 60).unison (1, 4, 17, 100).noise (30, 0).filter (1, ladder24, 700, 30)
            .env (2, 6000, 1000, 100, 500, -50).mod (env2, "filter1Cutoff", 40).mod (env2, "osc1Pitch", 25).amp (2000, 1000, 100, 600)
            .lfo (1, sine, 6.5f, 1).mod (lfo1, "filter1Cutoff", 12).reverb (35, 90, 5.5f, 45).level (-10).standardMacros (25);
        add ("Draining Whoosh", "Riser").set ("osc1On", 0).noise (80, 0).filter (1, svfBand, 900, 45).env (2, 4000, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 45).amp (5000, 1000, 100, 900).reverb (45, 90, 7.5f, 45).level (5).standardMacros (25);
        add ("Falling Sweep", "Riser").set ("osc1On", 0).noise (90, 0).filter (1, ladder24, 900, 35).env (2, 6000, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 45).amp (3500, 1000, 100, 700).delay (sixteenthD, 45, 29, 4000).reverb (40, 90, 8, 45).level (4)
            .standardMacros (25);
        add ("Grey Whoosh", "Riser").osc (1, sync, 20).unison (1, 4, 33, 100).filter (1, svfHigh, 1000, 45).env (2, 4500, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 50).mod (env2, "osc1Pitch", 25).amp (3000, 1000, 100, 600).lfo (1, sawUp, 6, 1)
            .mod (lfo1, "filter1Cutoff", 11).reverb (45, 90, 7, 45).level (-7).standardMacros (25);
        add ("Hollow Ascent", "Riser").osc (1, analog, sawPos).unison (1, 4, 33, 100).noise (50, 0).filter (1, svfHigh, 600, 30)
            .env (2, 3500, 1000, 100, 500, -50).mod (env2, "filter1Cutoff", 50).mod (env2, "osc1Pitch", 25).amp (3500, 1000, 100, 800)
            .delay (eighthD, 50, 20, 4000).reverb (40, 90, 6.5f, 45).level (-6).standardMacros (25);
        add ("Hollow Siren", "Riser").set ("osc1On", 0).noise (80, 1).filter (1, svfHigh, 5000, 25).env (2, 0, 4500, 0, 4500, 60)
            .mod (env2, "filter1Cutoff", 40).amp (0, 4500, 0, 1600).reverb (40, 90, 7.5f, 45).level (6).standardMacros (25);
        add ("Iron Ascent", "Riser").osc (1, growl, 55).unison (1, 3, 33, 100).filter (1, svfHigh, 500, 40).env (2, 3000, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 55).mod (env2, "osc1Pitch", 25).amp (2000, 1000, 100, 500).delay (sixteenthD, 55, 24, 4000)
            .reverb (35, 90, 6, 45).level (-5).standardMacros (25);
        add ("Iron Countdown", "Riser").set ("osc1On", 0).noise (70, 0).filter (1, svfHigh, 800, 35).env (2, 4500, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 45).amp (4500, 1000, 100, 1000).reverb (50, 90, 6, 45).level (-1).standardMacros (25);
        add ("Leaden Riser", "Riser").set ("osc1On", 0).noise (65, 1).filter (1, svfHigh, 8000, 50).env (2, 0, 5500, 0, 5500, 60)
            .mod (env2, "filter1Cutoff", 40).amp (0, 5500, 0, 2000).lfo (1, sine, 7, 1).mod (lfo1, "filter1Cutoff", 12)
            .delay (sixteenthD, 45, 20, 4000).reverb (40, 90, 5, 45).level (6).standardMacros (25);
        add ("Molten Tension", "Riser").set ("osc1On", 0).noise (80, 1).filter (1, svfBand, 1300, 45).env (2, 7000, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 55).amp (2500, 1000, 100, 1000).reverb (45, 90, 5, 45).level (3).standardMacros (25);
        add ("Mute Wind", "Riser").set ("osc1On", 0).noise (85, 0).filter (1, svfBand, 1000, 35).env (2, 4000, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 60).amp (2000, 1000, 100, 900).lfo (1, sawUp, 5, 1).mod (lfo1, "filter1Cutoff", 11)
            .delay (sixteenthD, 40, 23, 4000).reverb (45, 90, 7.5f, 45).level (-4).standardMacros (25);
        add ("Night Ignition", "Riser").set ("osc1On", 0).noise (80, 0).filter (1, svfHigh, 4500, 30).env (2, 0, 6000, 0, 6000, 60)
            .mod (env2, "filter1Cutoff", 45).amp (0, 6000, 0, 1600).delay (sixteenthD, 50, 25, 4000).reverb (45, 90, 8, 45).level (1)
            .standardMacros (25);
        add ("Night Ramp", "Riser").osc (1, growl, 55).unison (1, 4, 28, 100).noise (45, 0).filter (1, ladder24, 1300, 40)
            .env (2, 5000, 1000, 100, 500, -50).mod (env2, "filter1Cutoff", 50).mod (env2, "osc1Pitch", 20).amp (4000, 1000, 100, 900)
            .reverb (45, 90, 5, 45).level (-6).standardMacros (25);
        add ("Null Ignition", "Riser").set ("osc1On", 0).noise (90, 1).filter (1, svfBand, 1000, 50).env (2, 5000, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 50).amp (4000, 1000, 100, 900).reverb (45, 90, 6.5f, 45).level (4).standardMacros (25);
        add ("Rising Swell", "Riser").osc (1, analog, sawPos).unison (1, 4, 15, 100).filter (1, ladder24, 3000, 25).env (2, 0, 7000, 0, 7000, 60)
            .mod (env2, "filter1Cutoff", 30).mod (env2, "osc1Pitch", 35).amp (0, 7000, 0, 2100).reverb (50, 90, 4.5f, 45).level (-10)
            .standardMacros (25);
        add ("Ruined Ascent", "Riser").osc (1, analog, sawPos).unison (1, 4, 16, 100).filter (1, svfBand, 1400, 45).env (2, 6500, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 60).mod (env2, "osc1Pitch", 35).amp (4500, 1000, 100, 500).lfo (1, sawUp, 6.5f, 1)
            .mod (lfo1, "filter1Cutoff", 12).delay (eighthD, 40, 28, 4000).reverb (45, 90, 8, 45).level (-4).standardMacros (25);
        add ("Ruined Tension", "Riser").osc (1, metal, 20).unison (1, 4, 34, 100).noise (40, 0).filter (1, svfHigh, 6000, 40)
            .env (2, 0, 4500, 0, 4500, 60).mod (env2, "filter1Cutoff", 50).mod (env2, "osc1Pitch", 30).amp (0, 4500, 0, 1800).lfo (1, sine, 7, 1)
            .mod (lfo1, "filter1Cutoff", 7).reverb (40, 90, 5, 45).level (6).standardMacros (25);
        add ("Ruined Wind", "Riser").set ("osc1On", 0).noise (90, 1).filter (1, svfBand, 4500, 40).env (2, 0, 5500, 0, 5500, 60)
            .mod (env2, "filter1Cutoff", 30).amp (0, 5500, 0, 1900).reverb (40, 90, 4.5f, 45).level (6).standardMacros (25);
        add ("Shadow Pressure", "Riser").set ("osc1On", 0).noise (90, 1).filter (1, ladder24, 1300, 40).env (2, 5000, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 50).amp (4500, 1000, 100, 900).lfo (1, sawUp, 4.5f, 1).mod (lfo1, "filter1Cutoff", 9)
            .delay (sixteenthD, 40, 23, 4000).reverb (50, 90, 4.5f, 45).level (-2).standardMacros (25);
        add ("Sinking Lift", "Riser").set ("osc1On", 0).noise (80, 0).filter (1, svfBand, 3000, 45).env (2, 0, 3500, 0, 3500, 60)
            .mod (env2, "filter1Cutoff", 40).amp (0, 3500, 0, 2500).delay (eighthD, 55, 27, 4000).reverb (35, 90, 5.5f, 45).level (1)
            .standardMacros (25);
        add ("Sinking Ramp", "Riser").osc (1, metal, 60).unison (1, 4, 19, 100).filter (1, svfHigh, 1400, 45).env (2, 6500, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 40).mod (env2, "osc1Pitch", 35).amp (4500, 1000, 100, 900).delay (sixteenthD, 50, 27, 4000)
            .reverb (40, 90, 7.5f, 45).level (-2).standardMacros (25);
        add ("Steel Alarm", "Riser").osc (1, analog, sawPos).unison (1, 6, 20, 100).noise (30, 0).filter (1, svfHigh, 1400, 45)
            .env (2, 4500, 1000, 100, 500, -50).mod (env2, "filter1Cutoff", 45).mod (env2, "osc1Pitch", 35).amp (4500, 1000, 100, 900)
            .reverb (35, 90, 7.5f, 45).level (-3).standardMacros (25);
        add ("Sunken Ascent", "Riser").set ("osc1On", 0).noise (65, 1).filter (1, ladder24, 900, 40).env (2, 5500, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 55).amp (3000, 1000, 100, 800).lfo (1, sine, 2.5f, 1).mod (lfo1, "filter1Cutoff", 6)
            .reverb (40, 90, 7.5f, 45).level (3).standardMacros (25);
        add ("Tar Tension", "Riser").set ("osc1On", 0).noise (80, 0).filter (1, svfBand, 1300, 25).env (2, 4500, 1000, 100, 500, -50)
            .mod (env2, "filter1Cutoff", 55).amp (3500, 1000, 100, 600).delay (sixteenthD, 50, 21, 4000).reverb (45, 90, 7.5f, 45).level (-1)
            .standardMacros (25);
        add ("Welded Lift", "Riser").osc (1, analog, sawPos).unison (1, 3, 27, 100).noise (55, 0).filter (1, svfHigh, 8000, 45)
            .env (2, 0, 5500, 0, 5500, 60).mod (env2, "filter1Cutoff", 40).mod (env2, "osc1Pitch", 45).amp (0, 5500, 0, 2300).reverb (35, 90, 6, 45)
            .level (6).standardMacros (25);
        add ("Welded Spin", "Riser").set ("osc1On", 0).noise (80, 1).filter (1, svfHigh, 6000, 30).env (2, 0, 4000, 0, 4000, 60)
            .mod (env2, "filter1Cutoff", 35).amp (0, 4000, 0, 2100).reverb (35, 90, 6, 45).level (6).standardMacros (25);

        // Impact, by name.
        add ("Abandoned Boom", "Impact").osc (1, fm, 10).noise (70, 0).env (2, 0, 300, 0, 400, 80).mod (env2, "osc1Pitch", 45)
            .filter (1, svfHigh, 1200, 20).amp (0, 1500, 0, 2600, 60).drive (fuzz, 45).reverb (50, 85, 5.5f, 70).level (-21).standardMacros (20);
        add ("Abandoned Shock", "Impact").osc (1, sub, 50).noise (45, 0).subOsc (45).env (2, 0, 750, 0, 400, 80).mod (env2, "osc1Pitch", 50)
            .filter (1, ladder24, 1400, 30).amp (0, 1900, 0, 900, 65).reverb (50, 95, 5.5f, 70).level (-14).standardMacros (20);
        add ("Abandoned Thump", "Impact").osc (1, analog, sinePos).noise (45, 1).subOsc (35).env (2, 0, 350, 0, 400, 75).mod (env2, "osc1Pitch", 35)
            .filter (1, ladder24, 1400, 35).amp (0, 1800, 0, 1300, 60).drive (fuzz, 50).reverb (55, 80, 3, 70).level (-27).standardMacros (20);
        add ("Bitter Smash", "Impact").osc (1, analog, sinePos).noise (45, 0).subOsc (35).env (2, 0, 550, 0, 400, 90).mod (env2, "osc1Pitch", 30)
            .filter (1, ladder24, 800, 30).amp (0, 1100, 0, 1900, 75).drive (hard, 40).reverb (55, 95, 6.5f, 70).level (-22).standardMacros (20);
        add ("Black Detonation", "Impact").osc (1, analog, sinePos).noise (65, 1).subOsc (45).env (2, 0, 750, 0, 400, 80).mod (env2, "osc1Pitch", 35)
            .filter (1, svfHigh, 600, 25).amp (0, 2200, 0, 2100, 75).drive (hard, 40).reverb (35, 95, 6, 70).level (-21).standardMacros (20);
        add ("Bleak Breach", "Impact").osc (1, analog, sinePos).noise (50, 3).subOsc (50).env (2, 0, 300, 0, 400, 90).mod (env2, "osc1Pitch", 25)
            .filter (1, comb, 90, 65).amp (0, 1900, 0, 2800, 70).drive (fuzz, 35).reverb (50, 75, 7.5f, 70).level (-26).standardMacros (20);
        add ("Bleak Wreck", "Impact").osc (1, metal, 30).noise (35, 0).env (2, 0, 750, 0, 400, 90).mod (env2, "osc1Pitch", 45)
            .filter (1, svfHigh, 2000, 30).amp (0, 2900, 0, 2900, 65).drive (crush, 45).reverb (35, 80, 7, 70).level (-6).standardMacros (20);
        add ("Blunt Strike", "Impact").osc (1, metal, 30).noise (30, 1).env (2, 0, 600, 0, 400, 85).mod (env2, "osc1Pitch", 45)
            .filter (1, ladder24, 1000, 25).amp (0, 2500, 0, 2600, 65).drive (hard, 30).reverb (40, 75, 5.5f, 70).level (-6).standardMacros (20);
        add ("Brutal Blast", "Impact").osc (1, fm, 10).noise (50, 0).env (2, 0, 650, 0, 400, 80).mod (env2, "osc1Pitch", 45)
            .filter (1, ladder24, 700, 30).amp (0, 1100, 0, 2300, 60).drive (crush, 40).reverb (40, 90, 8, 70).level (-10).standardMacros (20);
        add ("Brutal Detonation", "Impact").osc (1, analog, sinePos).noise (40, 2).subOsc (40).env (2, 0, 700, 0, 400, 90).mod (env2, "osc1Pitch", 35)
            .filter (1, ladder24, 1400, 25).amp (0, 900, 0, 3000, 65).drive (fuzz, 30).reverb (45, 90, 8, 70).level (-24).standardMacros (20);
        add ("Burnt Rupture", "Impact").osc (1, sub, 30).noise (65, 2).subOsc (40).env (2, 0, 650, 0, 400, 85).mod (env2, "osc1Pitch", 35)
            .filter (1, svfHigh, 500, 30).amp (0, 1400, 0, 1400, 70).drive (crush, 40).reverb (40, 100, 6.5f, 70).level (-13).standardMacros (20);
        add ("Burnt Thump", "Impact").osc (1, fm, 30).noise (65, 3).env (2, 0, 550, 0, 400, 75).mod (env2, "osc1Pitch", 40).filter (1, comb, 100, 55)
            .amp (0, 2200, 0, 1400, 75).reverb (55, 95, 2.5f, 70).level (-12).standardMacros (20);
        add ("Choked Boom", "Impact").osc (1, fm, 0).noise (50, 0).env (2, 0, 500, 0, 400, 85).mod (env2, "osc1Pitch", 45).filter (1, comb, 80, 70)
            .amp (0, 2600, 0, 2600, 65).reverb (30, 100, 5.5f, 70).level (-12).standardMacros (20);
        add ("Chrome Shock", "Impact").osc (1, sub, 50).noise (55, 0).subOsc (35).env (2, 0, 750, 0, 400, 90).mod (env2, "osc1Pitch", 40)
            .filter (1, comb, 105, 60).amp (0, 900, 0, 1400, 70).drive (hard, 40).reverb (40, 75, 4.5f, 70).level (-25).standardMacros (20);
        add ("Cinder Wreck", "Impact").osc (1, fm, 50).noise (60, 1).env (2, 0, 350, 0, 400, 90).mod (env2, "osc1Pitch", 40).filter (1, comb, 120, 65)
            .amp (0, 1400, 0, 800, 60).drive (fuzz, 50).reverb (30, 70, 5.5f, 70).level (-23).standardMacros (20);
        add ("Cold Wreck", "Impact").osc (1, fm, 20).noise (30, 0).env (2, 0, 450, 0, 400, 80).mod (env2, "osc1Pitch", 30)
            .filter (1, ladder24, 600, 25).amp (0, 1100, 0, 1200, 70).drive (crush, 45).reverb (40, 95, 4.5f, 70).level (-10).standardMacros (20);
        add ("Corroded Smash", "Impact").osc (1, fm, 10).noise (60, 1).env (2, 0, 350, 0, 400, 80).mod (env2, "osc1Pitch", 35)
            .filter (1, ladder24, 700, 25).amp (0, 2100, 0, 3000, 70).drive (crush, 40).reverb (30, 85, 4, 70).level (-12).standardMacros (20);
        add ("Crushing Impact", "Impact").osc (1, analog, sinePos).noise (30, 3).subOsc (30).env (2, 0, 700, 0, 400, 75).mod (env2, "osc1Pitch", 35)
            .filter (1, ladder24, 700, 30).amp (0, 2600, 0, 2100, 70).drive (crush, 40).reverb (45, 90, 5, 70).level (-12).standardMacros (20);
        add ("Derelict Quake", "Impact").osc (1, analog, sinePos).noise (40, 2).subOsc (40).env (2, 0, 400, 0, 400, 85).mod (env2, "osc1Pitch", 40)
            .filter (1, comb, 125, 65).amp (0, 2400, 0, 2400, 65).reverb (45, 80, 7, 70).level (-10).standardMacros (20);
        add ("Distant Crash", "Impact").osc (1, sub, 20).noise (30, 3).subOsc (50).env (2, 0, 800, 0, 400, 80).mod (env2, "osc1Pitch", 40)
            .filter (1, comb, 125, 55).amp (0, 2300, 0, 2500, 60).reverb (40, 75, 7, 70).level (-14).standardMacros (20);
        add ("Dropped Shock", "Impact").osc (1, sub, 70).noise (30, 3).subOsc (50).env (2, 0, 450, 0, 400, 80).mod (env2, "osc1Pitch", 25)
            .filter (1, svfHigh, 1800, 20).amp (0, 2300, 0, 2600, 65).reverb (45, 75, 9, 70).level (-8).standardMacros (20);
        add ("Feral Blast", "Impact").osc (1, sub, 30).noise (45, 2).subOsc (55).env (2, 0, 300, 0, 400, 85).mod (env2, "osc1Pitch", 30)
            .filter (1, ladder24, 1600, 15).amp (0, 1700, 0, 2500, 60).drive (crush, 30).reverb (30, 90, 5, 70).level (-14).standardMacros (20);
        add ("Fractured Rupture", "Impact").osc (1, sub, 60).noise (50, 1).subOsc (35).env (2, 0, 450, 0, 400, 85).mod (env2, "osc1Pitch", 30)
            .filter (1, ladder24, 1200, 15).amp (0, 2500, 0, 2200, 60).drive (fuzz, 40).reverb (40, 100, 7, 70).level (-29).standardMacros (20);
        add ("Frozen Concussion", "Impact").osc (1, analog, sinePos).noise (70, 3).subOsc (50).env (2, 0, 750, 0, 400, 75).mod (env2, "osc1Pitch", 35)
            .filter (1, ladder24, 1600, 25).amp (0, 900, 0, 2400, 75).drive (fuzz, 45).reverb (45, 85, 5, 70).level (-25).standardMacros (20);
        add ("Heavy Blast", "Impact").osc (1, fm, 0).noise (55, 0).env (2, 0, 200, 0, 400, 75).mod (env2, "osc1Pitch", 30).filter (1, comb, 115, 55)
            .amp (0, 2200, 0, 1000, 75).reverb (40, 100, 6, 70).level (-12).standardMacros (20);
        add ("Heavy Crash", "Impact").osc (1, metal, 50).noise (30, 0).env (2, 0, 550, 0, 400, 85).mod (env2, "osc1Pitch", 45)
            .filter (1, ladder24, 1000, 20).amp (0, 1000, 0, 2400, 75).reverb (50, 80, 5, 70).level (6).standardMacros (20);
        add ("Heavy Strike", "Impact").osc (1, sub, 40).noise (30, 3).subOsc (55).env (2, 0, 650, 0, 400, 80).mod (env2, "osc1Pitch", 30)
            .filter (1, ladder24, 1800, 20).amp (0, 2700, 0, 1500, 60).drive (fuzz, 45).reverb (45, 75, 2.5f, 70).level (-28).standardMacros (20);
        add ("Industrial Collapse", "Impact").osc (1, fm, 30).noise (40, 1).env (2, 0, 350, 0, 400, 85).mod (env2, "osc1Pitch", 30)
            .filter (1, comb, 80, 70).amp (0, 2600, 0, 1500, 70).reverb (40, 80, 6.5f, 70).level (-10).standardMacros (20);
        add ("Obsidian Collapse", "Impact").osc (1, analog, sinePos).noise (35, 0).subOsc (50).env (2, 0, 350, 0, 400, 90).mod (env2, "osc1Pitch", 35)
            .filter (1, svfHigh, 800, 15).amp (0, 1700, 0, 2100, 75).drive (crush, 35).reverb (40, 95, 7.5f, 70).level (-8).standardMacros (20);
        add ("Obsidian Concussion", "Impact").osc (1, fm, 60).noise (55, 1).env (2, 0, 650, 0, 400, 75).mod (env2, "osc1Pitch", 35)
            .filter (1, svfHigh, 1700, 20).amp (0, 1400, 0, 1300, 75).drive (hard, 45).reverb (45, 85, 4, 70).level (-22).standardMacros (20);
        add ("Obsidian Detonation", "Impact").osc (1, fm, 10).noise (30, 1).env (2, 0, 350, 0, 400, 75).mod (env2, "osc1Pitch", 30)
            .filter (1, ladder24, 1000, 20).amp (0, 1600, 0, 1800, 70).drive (hard, 40).reverb (45, 80, 6.5f, 70).level (-22).standardMacros (20);
        add ("Obsidian Quake", "Impact").osc (1, sub, 0).noise (65, 3).subOsc (35).env (2, 0, 700, 0, 400, 85).mod (env2, "osc1Pitch", 45)
            .filter (1, ladder24, 1800, 30).amp (0, 1500, 0, 1000, 60).reverb (45, 85, 5, 70).level (-10).standardMacros (20);
        add ("Oily Hit", "Impact").osc (1, sub, 50).noise (30, 3).subOsc (50).env (2, 0, 600, 0, 400, 80).mod (env2, "osc1Pitch", 25)
            .filter (1, comb, 85, 70).amp (0, 2000, 0, 1000, 70).reverb (50, 90, 6.5f, 70).level (-13).standardMacros (20);
        add ("Oily Wreck", "Impact").osc (1, sub, 60).noise (60, 2).subOsc (40).env (2, 0, 500, 0, 400, 80).mod (env2, "osc1Pitch", 35)
            .filter (1, svfHigh, 500, 35).amp (0, 1400, 0, 1200, 65).reverb (30, 80, 5.5f, 70).level (-14).standardMacros (20);
        add ("Pressed Strike", "Impact").osc (1, sub, 50).noise (40, 3).subOsc (35).env (2, 0, 350, 0, 400, 85).mod (env2, "osc1Pitch", 45)
            .filter (1, svfHigh, 1200, 25).amp (0, 800, 0, 2700, 60).drive (crush, 40).reverb (55, 95, 5.5f, 70).level (-10).standardMacros (20);
        add ("Riveted Blast", "Impact").osc (1, metal, 30).noise (50, 1).env (2, 0, 550, 0, 400, 80).mod (env2, "osc1Pitch", 30)
            .filter (1, comb, 115, 65).amp (0, 800, 0, 2000, 65).drive (crush, 30).reverb (45, 75, 5, 70).level (-5).standardMacros (20);
        add ("Rotten Breach", "Impact").osc (1, sub, 80).noise (35, 3).subOsc (60).env (2, 0, 400, 0, 400, 90).mod (env2, "osc1Pitch", 35)
            .filter (1, ladder24, 1100, 30).amp (0, 2000, 0, 1500, 70).reverb (50, 85, 5.5f, 70).level (-14).standardMacros (20);
        add ("Rotten Wreck", "Impact").osc (1, analog, sinePos).noise (40, 3).subOsc (40).env (2, 0, 300, 0, 400, 75).mod (env2, "osc1Pitch", 35)
            .filter (1, svfHigh, 1900, 30).amp (0, 900, 0, 1800, 60).drive (hard, 30).reverb (40, 95, 4, 70).level (-10).standardMacros (20);
        add ("Shadow Crash", "Impact").osc (1, analog, sinePos).noise (55, 2).subOsc (55).env (2, 0, 650, 0, 400, 75).mod (env2, "osc1Pitch", 45)
            .filter (1, comb, 110, 70).amp (0, 800, 0, 1700, 70).drive (hard, 40).reverb (35, 80, 3, 70).level (-23).standardMacros (20);
        add ("Shadow Wreck", "Impact").osc (1, analog, sinePos).noise (40, 1).subOsc (35).env (2, 0, 700, 0, 400, 80).mod (env2, "osc1Pitch", 40)
            .filter (1, comb, 125, 55).amp (0, 2900, 0, 1100, 70).reverb (30, 100, 7.5f, 70).level (-21).standardMacros (20);
        add ("Shattering Detonation", "Impact").osc (1, fm, 80).noise (40, 3).env (2, 0, 750, 0, 400, 80).mod (env2, "osc1Pitch", 40)
            .filter (1, comb, 145, 75).amp (0, 2500, 0, 2200, 60).reverb (35, 85, 5.5f, 70).level (-10).standardMacros (20);
        add ("Shattering Quake", "Impact").osc (1, fm, 20).noise (35, 1).env (2, 0, 450, 0, 400, 75).mod (env2, "osc1Pitch", 30)
            .filter (1, svfHigh, 1300, 35).amp (0, 2000, 0, 2500, 65).drive (hard, 30).reverb (30, 70, 5, 70).level (-4).standardMacros (20);
        add ("Shattering Wreck", "Impact").osc (1, metal, 50).noise (35, 2).env (2, 0, 300, 0, 400, 75).mod (env2, "osc1Pitch", 35)
            .filter (1, comb, 140, 70).amp (0, 2200, 0, 1700, 65).reverb (40, 85, 8, 70).level (-4).standardMacros (20);
        add ("Static Strike", "Impact").osc (1, fm, 10).noise (25, 2).env (2, 0, 650, 0, 400, 80).mod (env2, "osc1Pitch", 50)
            .filter (1, svfHigh, 1500, 20).amp (0, 1800, 0, 2900, 70).drive (hard, 30).reverb (40, 85, 7.5f, 70).level (-6).standardMacros (20);
        add ("Static Thump", "Impact").osc (1, fm, 30).noise (40, 0).env (2, 0, 650, 0, 400, 80).mod (env2, "osc1Pitch", 35)
            .filter (1, ladder24, 1100, 35).amp (0, 2500, 0, 2700, 75).reverb (40, 85, 8, 70).level (-12).standardMacros (20);
        add ("Steel Shock", "Impact").osc (1, sub, 20).noise (35, 3).subOsc (50).env (2, 0, 200, 0, 400, 80).mod (env2, "osc1Pitch", 45)
            .filter (1, ladder24, 1700, 30).amp (0, 2100, 0, 1500, 65).reverb (30, 80, 3.5f, 70).level (-13).standardMacros (20);
        add ("Steel Smash", "Impact").osc (1, analog, sinePos).noise (30, 2).subOsc (35).env (2, 0, 300, 0, 400, 85).mod (env2, "osc1Pitch", 35)
            .filter (1, ladder24, 1500, 25).amp (0, 1800, 0, 800, 75).reverb (35, 90, 8, 70).level (-10).standardMacros (20);
        add ("Sterile Shock", "Impact").osc (1, metal, 10).noise (40, 3).env (2, 0, 450, 0, 400, 90).mod (env2, "osc1Pitch", 30)
            .filter (1, ladder24, 2000, 35).amp (0, 1700, 0, 1500, 70).reverb (45, 90, 4, 70).level (-4).standardMacros (20);
        add ("Sudden Strike", "Impact").osc (1, metal, 30).noise (35, 3).env (2, 0, 650, 0, 400, 80).mod (env2, "osc1Pitch", 25)
            .filter (1, ladder24, 1200, 15).amp (0, 2500, 0, 1000, 75).drive (hard, 30).reverb (35, 90, 3, 70).level (-7).standardMacros (20);
        add ("Sunken Crash", "Impact").osc (1, analog, sinePos).noise (45, 0).subOsc (45).env (2, 0, 650, 0, 400, 90).mod (env2, "osc1Pitch", 45)
            .filter (1, svfHigh, 2000, 20).amp (0, 2200, 0, 2100, 70).drive (crush, 30).reverb (35, 80, 2.5f, 70).level (-5).standardMacros (20);
        add ("Tar Hammer", "Impact").osc (1, analog, sinePos).noise (60, 3).subOsc (35).env (2, 0, 600, 0, 400, 85).mod (env2, "osc1Pitch", 35)
            .filter (1, ladder24, 1500, 25).amp (0, 2900, 0, 1900, 70).reverb (45, 75, 4, 70).level (-10).standardMacros (20);
        add ("Tar Thump", "Impact").osc (1, analog, sinePos).noise (55, 1).subOsc (55).env (2, 0, 800, 0, 400, 90).mod (env2, "osc1Pitch", 50)
            .filter (1, ladder24, 700, 25).amp (0, 2200, 0, 2200, 70).drive (crush, 25).reverb (35, 75, 5, 70).level (-13).standardMacros (20);
        add ("Toxic Breach", "Impact").osc (1, fm, 20).noise (25, 0).env (2, 0, 550, 0, 400, 90).mod (env2, "osc1Pitch", 40)
            .filter (1, ladder24, 1600, 20).amp (0, 1500, 0, 1400, 65).reverb (45, 85, 8, 70).level (-10).standardMacros (20);
        add ("Welded Breach", "Impact").osc (1, sub, 0).noise (45, 2).subOsc (40).env (2, 0, 350, 0, 400, 85).mod (env2, "osc1Pitch", 35)
            .filter (1, ladder24, 1300, 20).amp (0, 2300, 0, 2100, 70).drive (hard, 35).reverb (45, 95, 6.5f, 70).level (-22).standardMacros (20);
        add ("Welded Thump", "Impact").osc (1, metal, 80).noise (30, 2).env (2, 0, 800, 0, 400, 75).mod (env2, "osc1Pitch", 45)
            .filter (1, ladder24, 800, 20).amp (0, 1100, 0, 1300, 70).reverb (40, 90, 4, 70).level (6).standardMacros (20);

        // Texture, by name.
        add ("Abandoned Rubble", "Texture").set ("osc1On", 0).noise (70, 2, -40).filter (1, ladderBand, 2700, 20).lfo (1, randomShape, 11.1f, 0, 20)
            .mod (lfo1, "filter1Cutoff", 18).amp (350, 1000, 100, 1000).drive (crush, 25).reverb (30, 90, 4.5f, 45).level (6).standardMacros (25);
        add ("Acid Transmission", "Texture").osc (1, metal, 70).warp (1, wMirror, 40).noise (20, 3).filter (1, ladderBand, 4000, 35)
            .lfo (1, randomShape, 11.1f, 0, 10).mod (lfo1, "filter1Cutoff", 10).mod (lfo1, "osc1Position", 35).amp (1300, 1000, 100, 1000)
            .delay (sixteenthT, 40, 20, 3500).reverb (40, 90, 3, 45).level (-3).standardMacros (25);
        add ("Ash Residue", "Texture").set ("osc1On", 0).noise (70, 3, 30).filter (1, svfHigh, 3400, 45).lfo (1, drift, 9.4f, 0, 30)
            .mod (lfo1, "filter1Cutoff", 24).amp (1400, 1000, 100, 1400).drive (crush, 20).delay (sixteenthT, 50, 25, 3500).reverb (45, 90, 5.5f, 45)
            .level (-1).standardMacros (25);
        add ("Bitter Exhaust", "Texture").set ("osc1On", 0).noise (80, 2, -40).filter (1, ladderBand, 1200, 40).lfo (1, randomShape, 5.5f, 0, 40)
            .mod (lfo1, "filter1Cutoff", 18).amp (400, 1000, 100, 600).delay (quarterD, 55, 30, 3500).reverb (35, 90, 5.5f, 45).level (3)
            .standardMacros (25);
        add ("Bitter Radio", "Texture").osc (1, metal, 55).warp (1, wCrush, 45).noise (25, 2).filter (1, ladderBand, 3700, 25)
            .lfo (1, randomShape, 10.8f, 0).mod (lfo1, "filter1Cutoff", 14).mod (lfo1, "osc1Position", 30).amp (900, 1000, 100, 1700)
            .reverb (45, 90, 5, 45).level (-7).standardMacros (25);
        add ("Bleak Interference", "Texture").osc (1, growl, 30).warp (1, wCrush, 55).filter (1, svfHigh, 3200, 40).lfo (1, drift, 10.1f, 0, 40)
            .mod (lfo1, "filter1Cutoff", 24).mod (lfo1, "osc1Position", 25).amp (100, 1000, 100, 1100).reverb (30, 90, 4.5f, 45).level (6)
            .standardMacros (25);
        add ("Brutal Gravel", "Texture").osc (1, vowel, 50).warp (1, wMirror, 50).noise (25, 3).filter (1, svfLow, 2800, 45)
            .lfo (1, drift, 6.1f, 0, 40).mod (lfo1, "filter1Cutoff", 21).mod (lfo1, "osc1Position", 40).amp (200, 1000, 100, 1200)
            .reverb (40, 90, 7, 45).level (-10).standardMacros (25);
        add ("Buried Radio", "Texture").osc (1, sync, 65).warp (1, wCrush, 35).filter (1, svfLow, 2600, 35).lfo (1, drift, 4.1f, 0, 40)
            .mod (lfo1, "filter1Cutoff", 19).mod (lfo1, "osc1Position", 25).amp (550, 1000, 100, 1800).drive (crush, 45).reverb (40, 90, 4.5f, 45)
            .level (-12).standardMacros (25);
        add ("Buzzing Fragments", "Texture").set ("osc1On", 0).noise (70, 0, 30).filter (1, svfLow, 3900, 35).lfo (1, drift, 2.6f, 0, 40)
            .mod (lfo1, "filter1Cutoff", 19).amp (700, 1000, 100, 1200).drive (crush, 30).delay (sixteenthD, 55, 35, 3500).reverb (25, 90, 6.5f, 45)
            .level (-2).standardMacros (25);
        add ("Choked Texture", "Texture").osc (1, metal, 30).warp (1, wFm, 35).filter (1, svfBand, 3800, 25).lfo (1, randomShape, 5.1f, 0)
            .mod (lfo1, "filter1Cutoff", 22).mod (lfo1, "osc1Position", 35).amp (450, 1000, 100, 1400).delay (sixteenthT, 45, 25, 3500)
            .reverb (45, 90, 7, 45).level (-7).standardMacros (25);
        add ("Chrome Rubble", "Texture").set ("osc1On", 0).noise (75, 1, 30).filter (1, ladderBand, 2800, 40).lfo (1, randomShape, 0.8f, 0, 10)
            .mod (lfo1, "filter1Cutoff", 25).amp (1300, 1000, 100, 2000).drive (crush, 40).delay (sixteenthT, 50, 25, 3500).reverb (35, 90, 4.5f, 45)
            .level (0).standardMacros (25);
        add ("Concrete Debris", "Texture").osc (1, growl, 35).warp (1, wFm, 45).noise (15, 3).filter (1, svfHigh, 3600, 25)
            .lfo (1, randomShape, 10.7f, 0, 50).mod (lfo1, "filter1Cutoff", 19).mod (lfo1, "osc1Position", 30).amp (550, 1000, 100, 900)
            .drive (crush, 30).reverb (45, 90, 5, 45).level (-3).standardMacros (25);
        add ("Corroded Dust", "Texture").set ("osc1On", 0).noise (90, 3).filter (1, svfHigh, 1400, 45).lfo (1, randomShape, 5.6f, 0)
            .mod (lfo1, "filter1Cutoff", 13).amp (900, 1000, 100, 1200).delay (sixteenthD, 40, 20, 3500).reverb (45, 90, 5.5f, 45).level (-6)
            .standardMacros (25);
        add ("Corrupted Dust", "Texture").set ("osc1On", 0).noise (75, 2).filter (1, ladderBand, 900, 35).lfo (1, randomShape, 11.8f, 0, 60)
            .mod (lfo1, "filter1Cutoff", 16).amp (350, 1000, 100, 2000).reverb (50, 90, 4.5f, 45).level (4).standardMacros (25);
        add ("Crackling Ventilation", "Texture").set ("osc1On", 0).noise (75, 1, -40).filter (1, ladderBand, 3600, 45)
            .lfo (1, randomShape, 8.3f, 0, 50).mod (lfo1, "filter1Cutoff", 21).amp (150, 1000, 100, 2000).reverb (25, 90, 3.5f, 45).level (1)
            .standardMacros (25);
        add ("Derelict Gravel", "Texture").osc (1, metal, 65).warp (1, wFm, 35).noise (20, 2).filter (1, svfBand, 1000, 50)
            .lfo (1, drift, 8.1f, 0, 20).mod (lfo1, "filter1Cutoff", 13).mod (lfo1, "osc1Position", 35).amp (450, 1000, 100, 1500).drive (crush, 30)
            .reverb (50, 90, 4, 45).level (-3).standardMacros (25);
        add ("Derelict Steam", "Texture").set ("osc1On", 0).noise (80, 0).filter (1, ladderBand, 4000, 20).lfo (1, drift, 0.6f, 0, 60)
            .mod (lfo1, "filter1Cutoff", 20).amp (250, 1000, 100, 700).reverb (45, 90, 5, 45).level (1).standardMacros (25);
        add ("Frozen Scrap", "Texture").set ("osc1On", 0).noise (85, 0).filter (1, svfHigh, 1500, 35).lfo (1, randomShape, 4.6f, 0)
            .mod (lfo1, "filter1Cutoff", 20).amp (250, 1000, 100, 600).drive (crush, 40).delay (sixteenthD, 45, 30, 3500).reverb (40, 90, 6, 45)
            .level (-10).standardMacros (25);
        add ("Frozen Signal", "Texture").osc (1, growl, 80).warp (1, wFm, 50).filter (1, svfHigh, 3000, 25).lfo (1, randomShape, 1.3f, 0, 10)
            .mod (lfo1, "filter1Cutoff", 24).mod (lfo1, "osc1Position", 25).amp (600, 1000, 100, 1500).delay (sixteenthT, 45, 20, 3500)
            .reverb (40, 90, 5, 45).level (-10).standardMacros (25);
        add ("Frozen Texture", "Texture").osc (1, fm, 80).warp (1, wCrush, 60).filter (1, svfHigh, 800, 40).lfo (1, randomShape, 0.4f, 0, 40)
            .mod (lfo1, "filter1Cutoff", 13).mod (lfo1, "osc1Position", 35).amp (500, 1000, 100, 1600).drive (crush, 20)
            .delay (sixteenthD, 40, 25, 3500).reverb (45, 90, 3.5f, 45).level (-14).standardMacros (25);
        add ("Grey Fragments", "Texture").osc (1, metal, 45).warp (1, wCrush, 45).noise (20, 3).filter (1, svfBand, 2000, 40)
            .lfo (1, randomShape, 11.2f, 0, 10).mod (lfo1, "filter1Cutoff", 18).mod (lfo1, "osc1Position", 30).amp (850, 1000, 100, 1200)
            .delay (sixteenthD, 45, 30, 3500).reverb (30, 90, 5, 45).level (-5).standardMacros (25);
        add ("Heavy Hive", "Texture").set ("osc1On", 0).noise (70, 1, 30).filter (1, ladderBand, 3000, 35).lfo (1, drift, 5.4f, 0, 20)
            .mod (lfo1, "filter1Cutoff", 22).amp (400, 1000, 100, 1200).drive (crush, 40).delay (quarterD, 45, 25, 3500).reverb (40, 90, 4.5f, 45)
            .level (1).standardMacros (25);
        add ("Heavy Transmission", "Texture").osc (1, fm, 80).warp (1, wFold, 50).filter (1, svfHigh, 3100, 40).lfo (1, randomShape, 11.1f, 0)
            .mod (lfo1, "filter1Cutoff", 14).mod (lfo1, "osc1Position", 20).amp (600, 1000, 100, 600).delay (sixteenthT, 50, 25, 3500)
            .reverb (50, 90, 3, 45).level (-12).standardMacros (25);
        add ("Hollow Fragments", "Texture").osc (1, fm, 30).warp (1, wFm, 55).noise (35, 3).filter (1, svfHigh, 2000, 35)
            .lfo (1, randomShape, 1.2f, 0, 20).mod (lfo1, "filter1Cutoff", 19).mod (lfo1, "osc1Position", 40).amp (400, 1000, 100, 800)
            .drive (crush, 50).reverb (30, 90, 4, 45).level (-1).standardMacros (25);
        add ("Industrial Swarm", "Texture").osc (1, sync, 80).warp (1, wMirror, 35).noise (25, 3).filter (1, svfLow, 3700, 30)
            .lfo (1, randomShape, 2.2f, 0, 40).mod (lfo1, "filter1Cutoff", 25).mod (lfo1, "osc1Position", 40).amp (850, 1000, 100, 600)
            .delay (quarterD, 40, 20, 3500).reverb (25, 90, 4.5f, 45).level (-13).standardMacros (25);
        add ("Iron Interference", "Texture").osc (1, metal, 60).warp (1, wMirror, 60).filter (1, svfHigh, 2200, 40).lfo (1, randomShape, 2.3f, 0, 20)
            .mod (lfo1, "filter1Cutoff", 11).mod (lfo1, "osc1Position", 30).amp (450, 1000, 100, 700).reverb (40, 90, 4.5f, 45).level (-5)
            .standardMacros (25);
        add ("Iron Scrap", "Texture").set ("osc1On", 0).noise (80, 2).filter (1, ladderBand, 2300, 30).lfo (1, randomShape, 10.4f, 0, 40)
            .mod (lfo1, "filter1Cutoff", 16).amp (1350, 1000, 100, 1900).reverb (40, 90, 4, 45).level (2).standardMacros (25);
        add ("Iron Ventilation", "Texture").osc (1, vowel, 40).warp (1, wMirror, 65).noise (35, 3).filter (1, ladderBand, 1100, 35)
            .lfo (1, randomShape, 6.3f, 0, 50).mod (lfo1, "filter1Cutoff", 22).mod (lfo1, "osc1Position", 50).amp (1350, 1000, 100, 1400)
            .drive (crush, 40).reverb (40, 90, 6.5f, 45).level (-7).standardMacros (25);
        add ("Leaden Exhaust", "Texture").osc (1, sync, 55).warp (1, wCrush, 50).filter (1, svfHigh, 3900, 50).lfo (1, randomShape, 9.7f, 0, 20)
            .mod (lfo1, "filter1Cutoff", 21).mod (lfo1, "osc1Position", 40).amp (1050, 1000, 100, 900).reverb (35, 90, 6, 45).level (-10)
            .standardMacros (25);
        add ("Leaden Rain", "Texture").osc (1, growl, 50).warp (1, wCrush, 30).noise (25, 3).filter (1, svfHigh, 2200, 35).lfo (1, drift, 3.1f, 0, 10)
            .mod (lfo1, "filter1Cutoff", 14).mod (lfo1, "osc1Position", 35).amp (450, 1000, 100, 800).delay (sixteenthD, 40, 20, 3500)
            .reverb (50, 90, 3.5f, 45).level (4).standardMacros (25);
        add ("Leaden Residue", "Texture").set ("osc1On", 0).noise (75, 1, -40).filter (1, svfLow, 700, 25).lfo (1, randomShape, 7.7f, 0, 50)
            .mod (lfo1, "filter1Cutoff", 19).amp (1200, 1000, 100, 1700).drive (crush, 25).reverb (25, 90, 6.5f, 45).level (-3).standardMacros (25);
        add ("Leaden Transmission", "Texture").osc (1, vowel, 75).warp (1, wFm, 35).noise (35, 3).filter (1, ladderBand, 2600, 25)
            .lfo (1, randomShape, 7.6f, 0, 20).mod (lfo1, "filter1Cutoff", 19).mod (lfo1, "osc1Position", 25).amp (450, 1000, 100, 900)
            .drive (crush, 35).delay (sixteenthT, 50, 25, 3500).reverb (30, 90, 5, 45).level (-10).standardMacros (25);
        add ("Mute Dust", "Texture").set ("osc1On", 0).noise (75, 2).filter (1, svfLow, 800, 30).lfo (1, randomShape, 6.4f, 0, 20)
            .mod (lfo1, "filter1Cutoff", 13).amp (500, 1000, 100, 1100).drive (crush, 45).delay (sixteenthT, 35, 25, 3500).reverb (45, 90, 4.5f, 45)
            .level (-1).standardMacros (25);
        add ("Mute Steam", "Texture").osc (1, vowel, 50).warp (1, wFold, 55).filter (1, svfHigh, 1400, 35).lfo (1, randomShape, 1.8f, 0, 10)
            .mod (lfo1, "filter1Cutoff", 12).mod (lfo1, "osc1Position", 25).amp (450, 1000, 100, 2000).delay (quarterD, 35, 35, 3500)
            .reverb (50, 90, 3, 45).level (-12).standardMacros (25);
        add ("Nickel Rain", "Texture").set ("osc1On", 0).noise (70, 0, 30).filter (1, ladderBand, 3900, 30).lfo (1, randomShape, 3.8f, 0, 20)
            .mod (lfo1, "filter1Cutoff", 23).amp (1000, 1000, 100, 1400).reverb (25, 90, 6.5f, 45).level (-2).standardMacros (25);
        add ("Night Scrap", "Texture").set ("osc1On", 0).noise (100, 3).filter (1, svfHigh, 1800, 50).lfo (1, drift, 2.7f, 0, 20)
            .mod (lfo1, "filter1Cutoff", 11).amp (1000, 1000, 100, 1200).reverb (35, 90, 5.5f, 45).level (-8).standardMacros (25);
        add ("Obsidian Rain", "Texture").osc (1, metal, 70).warp (1, wFold, 45).noise (25, 3).filter (1, svfBand, 1500, 40)
            .lfo (1, randomShape, 0.6f, 0, 30).mod (lfo1, "filter1Cutoff", 15).mod (lfo1, "osc1Position", 50).amp (850, 1000, 100, 1400)
            .drive (crush, 35).delay (quarterD, 50, 30, 3500).reverb (30, 90, 3.5f, 45).level (-10).standardMacros (25);
        add ("Pressed Interference", "Texture").osc (1, vowel, 75).warp (1, wMirror, 60).filter (1, ladderBand, 3700, 45)
            .lfo (1, randomShape, 3.1f, 0, 10).mod (lfo1, "filter1Cutoff", 22).mod (lfo1, "osc1Position", 35).amp (650, 1000, 100, 1200)
            .drive (crush, 25).reverb (45, 90, 4, 45).level (-7).standardMacros (25);
        add ("Pressed Rain", "Texture").set ("osc1On", 0).noise (75, 2, -40).filter (1, svfLow, 3900, 35).lfo (1, randomShape, 9.5f, 0, 40)
            .mod (lfo1, "filter1Cutoff", 18).amp (450, 1000, 100, 1300).delay (sixteenthD, 35, 25, 3500).reverb (30, 90, 6, 45).level (-1)
            .standardMacros (25);
        add ("Pressed Static", "Texture").osc (1, sync, 70).warp (1, wCrush, 60).filter (1, svfLow, 2200, 30).lfo (1, drift, 4.4f, 0)
            .mod (lfo1, "filter1Cutoff", 13).mod (lfo1, "osc1Position", 50).amp (550, 1000, 100, 1400).drive (crush, 30).reverb (30, 90, 6, 45)
            .level (-10).standardMacros (25);
        add ("Ruined Rain", "Texture").set ("osc1On", 0).noise (95, 2, 30).filter (1, svfBand, 3400, 25).lfo (1, drift, 10.4f, 0, 10)
            .mod (lfo1, "filter1Cutoff", 21).amp (1350, 1000, 100, 900).drive (crush, 35).delay (quarterD, 35, 25, 3500).reverb (35, 90, 7, 45)
            .level (-2).standardMacros (25);
        add ("Scorched Signal", "Texture").osc (1, growl, 40).warp (1, wFold, 60).filter (1, svfBand, 2200, 35).lfo (1, randomShape, 0.5f, 0, 60)
            .mod (lfo1, "filter1Cutoff", 10).mod (lfo1, "osc1Position", 50).amp (850, 1000, 100, 700).delay (sixteenthD, 50, 25, 3500)
            .reverb (50, 90, 5.5f, 45).level (-12).standardMacros (25);
        add ("Scorched Texture", "Texture").osc (1, growl, 45).warp (1, wCrush, 30).filter (1, svfHigh, 3100, 30).lfo (1, drift, 5.3f, 0, 20)
            .mod (lfo1, "filter1Cutoff", 17).mod (lfo1, "osc1Position", 40).amp (1450, 1000, 100, 800).drive (crush, 25).reverb (30, 90, 3.5f, 45)
            .level (6).standardMacros (25);
        add ("Scraping Fragments", "Texture").set ("osc1On", 0).noise (60, 3, 30).filter (1, ladderBand, 2000, 25).lfo (1, randomShape, 11.8f, 0)
            .mod (lfo1, "filter1Cutoff", 18).amp (500, 1000, 100, 1800).drive (crush, 20).reverb (45, 90, 7, 45).level (0).standardMacros (25);
        add ("Scraping Rubble", "Texture").set ("osc1On", 0).noise (95, 2, 30).filter (1, ladderBand, 3700, 30).lfo (1, randomShape, 3.2f, 0, 50)
            .mod (lfo1, "filter1Cutoff", 18).amp (650, 1000, 100, 1600).delay (sixteenthD, 40, 25, 3500).reverb (30, 90, 6.5f, 45).level (1)
            .standardMacros (25);
        add ("Severed Scrap", "Texture").set ("osc1On", 0).noise (90, 0, -40).filter (1, svfHigh, 1100, 45).lfo (1, randomShape, 3.4f, 0, 10)
            .mod (lfo1, "filter1Cutoff", 14).amp (250, 1000, 100, 900).drive (crush, 20).reverb (25, 90, 5.5f, 45).level (-6).standardMacros (25);
        add ("Soot Interference", "Texture").set ("osc1On", 0).noise (80, 0, -40).filter (1, svfLow, 2000, 35).lfo (1, randomShape, 5.3f, 0, 50)
            .mod (lfo1, "filter1Cutoff", 10).amp (1250, 1000, 100, 1100).drive (crush, 35).delay (sixteenthD, 45, 20, 3500).reverb (30, 90, 3.5f, 45)
            .level (0).standardMacros (25);
        add ("Static Texture", "Texture").set ("osc1On", 0).noise (85, 0).filter (1, svfHigh, 800, 40).lfo (1, randomShape, 11.9f, 0, 20)
            .mod (lfo1, "filter1Cutoff", 24).amp (400, 1000, 100, 1900).drive (crush, 40).reverb (50, 90, 3, 45).level (-10).standardMacros (25);
        add ("Steel Signal", "Texture").osc (1, vowel, 35).warp (1, wCrush, 40).noise (40, 2).filter (1, svfHigh, 2700, 25)
            .lfo (1, drift, 0.7f, 0, 20).mod (lfo1, "filter1Cutoff", 12).mod (lfo1, "osc1Position", 25).amp (200, 1000, 100, 1000).drive (crush, 30)
            .reverb (35, 90, 4, 45).level (-10).standardMacros (25);
        add ("Sterile Rubble", "Texture").set ("osc1On", 0).noise (80, 1).filter (1, ladderBand, 3300, 40).lfo (1, randomShape, 0.4f, 0, 40)
            .mod (lfo1, "filter1Cutoff", 20).amp (600, 1000, 100, 1500).drive (crush, 30).delay (quarterD, 45, 25, 3500).reverb (35, 90, 3, 45)
            .level (-2).standardMacros (25);
        add ("Sullen Swarm", "Texture").set ("osc1On", 0).noise (75, 1, 30).filter (1, ladderBand, 1300, 50).lfo (1, randomShape, 6, 0, 10)
            .mod (lfo1, "filter1Cutoff", 16).amp (500, 1000, 100, 900).drive (crush, 30).reverb (30, 90, 6, 45).level (0).standardMacros (25);
        add ("Sullen Transmission", "Texture").set ("osc1On", 0).noise (75, 3, 30).filter (1, ladderBand, 1100, 35).lfo (1, randomShape, 4.7f, 0, 50)
            .mod (lfo1, "filter1Cutoff", 12).amp (1350, 1000, 100, 1300).drive (crush, 40).reverb (45, 90, 6, 45).level (-5).standardMacros (25);
        add ("Ticking Signal", "Texture").osc (1, metal, 35).warp (1, wMirror, 50).noise (20, 3).filter (1, svfBand, 3900, 40)
            .lfo (1, drift, 10.1f, 0, 30).mod (lfo1, "filter1Cutoff", 11).mod (lfo1, "osc1Position", 40).amp (1050, 1000, 100, 1800).drive (crush, 35)
            .delay (sixteenthD, 50, 25, 3500).reverb (40, 90, 6.5f, 45).level (-7).standardMacros (25);
        add ("Ticking Wiring", "Texture").osc (1, metal, 30).warp (1, wMirror, 55).filter (1, svfLow, 1900, 30).lfo (1, drift, 6.9f, 0, 50)
            .mod (lfo1, "filter1Cutoff", 12).mod (lfo1, "osc1Position", 30).amp (1300, 1000, 100, 1400).reverb (30, 90, 6.5f, 45).level (-6)
            .standardMacros (25);
        add ("Toxic Static", "Texture").set ("osc1On", 0).noise (60, 3, 30).filter (1, svfHigh, 2000, 25).lfo (1, randomShape, 7.5f, 0, 60)
            .mod (lfo1, "filter1Cutoff", 23).amp (1200, 1000, 100, 1200).reverb (40, 90, 6, 45).level (0).standardMacros (25);
        add ("Void Interference", "Texture").set ("osc1On", 0).noise (90, 1, 30).filter (1, ladderBand, 2700, 35).lfo (1, drift, 2.4f, 0, 10)
            .mod (lfo1, "filter1Cutoff", 15).amp (600, 1000, 100, 1200).delay (sixteenthT, 50, 25, 3500).reverb (40, 90, 4.5f, 45).level (-2)
            .standardMacros (25);
        add ("Welded Ventilation", "Texture").osc (1, metal, 30).warp (1, wFold, 65).noise (15, 3).filter (1, ladderBand, 1900, 30)
            .lfo (1, drift, 9.4f, 0, 10).mod (lfo1, "filter1Cutoff", 12).mod (lfo1, "osc1Position", 45).amp (700, 1000, 100, 1300).drive (crush, 25)
            .delay (sixteenthD, 40, 30, 3500).reverb (40, 90, 5.5f, 45).level (-10).standardMacros (25);
        add ("Whirring Interference", "Texture").set ("osc1On", 0).noise (90, 0).filter (1, svfBand, 800, 25).lfo (1, randomShape, 4.8f, 0, 40)
            .mod (lfo1, "filter1Cutoff", 25).amp (1500, 1000, 100, 1400).delay (sixteenthT, 35, 25, 3500).reverb (45, 90, 6.5f, 45).level (-4)
            .standardMacros (25);
        add ("Whirring Wiring", "Texture").set ("osc1On", 0).noise (65, 1).filter (1, svfHigh, 2300, 50).lfo (1, randomShape, 2, 0, 20)
            .mod (lfo1, "filter1Cutoff", 13).amp (50, 1000, 100, 900).delay (sixteenthT, 45, 25, 3500).reverb (45, 90, 5.5f, 45).level (1)
            .standardMacros (25);
        add ("Zinc Rain", "Texture").set ("osc1On", 0).noise (80, 3, 30).filter (1, ladderBand, 2000, 45).lfo (1, drift, 11.2f, 0, 30)
            .mod (lfo1, "filter1Cutoff", 19).amp (400, 1000, 100, 1000).drive (crush, 30).reverb (50, 90, 3.5f, 45).level (-4).standardMacros (25);

        // Perc, by name.
        add ("Abandoned Clank", "Perc").osc (1, analog, trianglePos, 45).noise (90, 0).filter (1, svfBand, 2000, 30).env (2, 0, 40, 0, 40, 90)
            .mod (env2, "osc1Pitch", 16).amp (0, 260, 0, 170).reverb (20, 50, 1.2f, 45).level (-2).standardMacros (20);
        add ("Abandoned Tick", "Perc").set ("osc1On", 0).noise (100, 3).filter (1, svfHigh, 7000, 30).amp (0, 150, 0, 110).level (1)
            .standardMacros (20);
        add ("Acid Knock", "Perc").osc (1, metal, 80).filter (1, svfBand, 400, 70).noise (20, 2).amp (0, 210, 0, 310, 75).level (3)
            .standardMacros (20);
        add ("Basalt Clink", "Perc").osc (1, analog, trianglePos, 40).noise (70, 0).filter (1, svfBand, 2700, 30).env (2, 0, 40, 0, 40, 90)
            .mod (env2, "osc1Pitch", 11).amp (0, 180, 0, 110).reverb (30, 50, 1.2f, 45).level (2).standardMacros (20);
        add ("Bitter Snare", "Perc").osc (1, sub, 0).env (2, 0, 30, 0, 60, 90).mod (env2, "osc1Pitch", 30).amp (0, 350, 0, 80).drive (tube, 30, 4000)
            .filter (1, ladder24, 2100, 0).level (-18).standardMacros (20);
        add ("Brutal Tom", "Perc").osc (1, analog, trianglePos, 50).noise (70, 0).filter (1, svfBand, 2600, 20).env (2, 0, 40, 0, 40, 90)
            .mod (env2, "osc1Pitch", 14).amp (0, 180, 0, 220).reverb (35, 50, 1.2f, 45).level (2).standardMacros (20);
        add ("Burnt Drum", "Perc").osc (1, analog, trianglePos, 35).noise (80, 0).filter (1, svfBand, 2200, 20).env (2, 0, 40, 0, 40, 90)
            .mod (env2, "osc1Pitch", 18).amp (0, 140, 0, 130).reverb (25, 50, 1.2f, 45).level (0).standardMacros (20);
        add ("Carbon Zap", "Perc").osc (1, analog, sinePos).env (2, 0, 50, 0, 60, 90).mod (env2, "osc1Pitch", 25).amp (0, 230, 0, 80)
            .filter (1, ladder24, 2800, 0).level (-10).standardMacros (20);
        add ("Choked Pipe", "Perc").set ("osc1On", 0).noise (90, 3).filter (1, svfHigh, 6000, 35).amp (0, 190, 0, 140).level (1).standardMacros (20);
        add ("Chrome Can", "Perc").set ("osc1On", 0).noise (75, 0).filter (1, svfHigh, 5500, 20).amp (0, 90, 0, 150).level (-1).standardMacros (20);
        add ("Cinder Can", "Perc").osc (1, analog, trianglePos, 35).noise (70, 0).filter (1, svfBand, 2600, 35).env (2, 0, 40, 0, 40, 90)
            .mod (env2, "osc1Pitch", 14).amp (0, 190, 0, 210).reverb (30, 50, 1.2f, 45).level (2).standardMacros (20);
        add ("Clanging Hat", "Perc").osc (1, analog, trianglePos, 50).noise (80, 0).filter (1, svfBand, 2800, 25).env (2, 0, 40, 0, 40, 90)
            .mod (env2, "osc1Pitch", 20).amp (0, 200, 0, 230).reverb (30, 50, 1.2f, 45).level (0).standardMacros (20);
        add ("Cobalt Block", "Perc").osc (1, analog, 0).env (2, 0, 65, 0, 60, 90).mod (env2, "osc1Pitch", 20).amp (0, 250, 0, 80)
            .filter (1, ladder24, 1700, 0).level (-10).standardMacros (20);
        add ("Cold Kick", "Perc").osc (1, sub, sinePos).env (2, 0, 80, 0, 60, 90).mod (env2, "osc1Pitch", 35).amp (0, 300, 0, 80)
            .filter (1, ladder24, 2200, 0).level (-10).standardMacros (20);
        add ("Corroded Kick", "Perc").osc (1, analog, trianglePos, 35).noise (75, 0).filter (1, svfBand, 1600, 35).env (2, 0, 40, 0, 40, 90)
            .mod (env2, "osc1Pitch", 12).amp (0, 170, 0, 240).reverb (20, 50, 1.2f, 45).level (3).standardMacros (20);
        add ("Dead Hat", "Perc").osc (1, analog, sinePos).env (2, 0, 45, 0, 60, 90).mod (env2, "osc1Pitch", 40).amp (0, 200, 0, 80)
            .filter (1, ladder24, 1600, 0).level (-10).standardMacros (20);
        add ("Dry Rim", "Perc").osc (1, fm, 70).filter (1, comb, 700, 60).noise (20, 2).amp (0, 210, 0, 360, 75).drive (hard, 20).level (-17)
            .standardMacros (20);
        add ("Factory Clave", "Perc").osc (1, analog, 0).env (2, 0, 50, 0, 60, 90).mod (env2, "osc1Pitch", 30).amp (0, 270, 0, 80)
            .filter (1, ladder24, 1900, 0).level (-10).standardMacros (20);
        add ("Factory Tom", "Perc").osc (1, sub, sinePos).env (2, 0, 60, 0, 60, 90).mod (env2, "osc1Pitch", 25).amp (0, 210, 0, 80)
            .drive (tube, 45, 4000).filter (1, ladder24, 2700, 0).level (-21).standardMacros (20);
        add ("Feral Hat", "Perc").set ("osc1On", 0).noise (70, 3).filter (1, svfHigh, 8500, 20).amp (0, 50, 0, 110).level (6).standardMacros (20);
        add ("Grey Kick", "Perc").set ("osc1On", 0).noise (95, 3).filter (1, svfHigh, 8500, 35).amp (0, 190, 0, 60).level (2).standardMacros (20);
        add ("Grey Tom", "Perc").osc (1, analog, sinePos).env (2, 0, 55, 0, 60, 90).mod (env2, "osc1Pitch", 35).amp (0, 400, 0, 80)
            .drive (tube, 30, 4000).filter (1, ladder24, 1000, 0).level (-16).standardMacros (20);
        add ("Heavy Block", "Perc").osc (1, sub, sinePos).env (2, 0, 90, 0, 60, 90).mod (env2, "osc1Pitch", 25).amp (0, 220, 0, 80)
            .filter (1, ladder24, 1300, 0).level (-10).standardMacros (20);
        add ("Heavy Zap", "Perc").set ("osc1On", 0).noise (75, 3).filter (1, svfHigh, 5500, 20).amp (0, 110, 0, 70).level (5).standardMacros (20);
        add ("Hollow Kick", "Perc").osc (1, analog, 0).env (2, 0, 85, 0, 60, 90).mod (env2, "osc1Pitch", 25).amp (0, 390, 0, 80)
            .drive (tube, 50, 4000).filter (1, ladder24, 1800, 0).level (-21).standardMacros (20);
        add ("Hollow Tom", "Perc").set ("osc1On", 0).noise (85, 0).filter (1, svfHigh, 7000, 30).amp (0, 120, 0, 70).level (-3).standardMacros (20);
        add ("Hollow Zap", "Perc").osc (1, analog, 0).env (2, 0, 65, 0, 60, 90).mod (env2, "osc1Pitch", 25).amp (0, 320, 0, 80).drive (fuzz, 30, 4000)
            .filter (1, ladder24, 2800, 0).level (-23).standardMacros (20);
        add ("Iron Clank", "Perc").set ("osc1On", 0).noise (70, 3).filter (1, svfHigh, 6000, 35).amp (0, 50, 0, 50).level (6).standardMacros (20);
        add ("Iron Clap", "Perc").set ("osc1On", 0).noise (75, 0).filter (1, svfHigh, 7500, 30).amp (0, 160, 0, 140).level (-1).standardMacros (20);
        add ("Leaden Block", "Perc").osc (1, fm, 90).filter (1, svfBand, 2400, 70).noise (25, 2).amp (0, 600, 0, 320, 75).level (-10)
            .standardMacros (20);
        add ("Leaden Pipe", "Perc").set ("osc1On", 0).noise (90, 0).filter (1, svfHigh, 9000, 20).amp (0, 200, 0, 80).level (-3).standardMacros (20);
        add ("Mute Zap", "Perc").osc (1, analog, trianglePos, 50).noise (70, 0).filter (1, svfBand, 2800, 30).env (2, 0, 40, 0, 40, 90)
            .mod (env2, "osc1Pitch", 15).amp (0, 230, 0, 120).reverb (30, 50, 1.2f, 45).level (3).standardMacros (20);
        add ("Nickel Chain", "Perc").osc (1, sub, sinePos).env (2, 0, 35, 0, 60, 90).mod (env2, "osc1Pitch", 25).amp (0, 270, 0, 80)
            .filter (1, ladder24, 2200, 0).level (-10).standardMacros (20);
        add ("Nickel Pipe", "Perc").osc (1, sub, sinePos).env (2, 0, 85, 0, 60, 90).mod (env2, "osc1Pitch", 35).amp (0, 270, 0, 80)
            .filter (1, ladder24, 1400, 0).level (-10).standardMacros (20);
        add ("Night Block", "Perc").osc (1, sub, 0).env (2, 0, 75, 0, 60, 90).mod (env2, "osc1Pitch", 40).amp (0, 430, 0, 80)
            .filter (1, ladder24, 1100, 0).level (-10).standardMacros (20);
        add ("Null Knock", "Perc").osc (1, analog, trianglePos, 30).noise (85, 0).filter (1, svfBand, 2800, 35).env (2, 0, 40, 0, 40, 90)
            .mod (env2, "osc1Pitch", 19).amp (0, 200, 0, 180).reverb (20, 50, 1.2f, 45).level (-1).standardMacros (20);
        add ("Oily Block", "Perc").osc (1, analog, trianglePos, 50).noise (80, 0).filter (1, svfBand, 2600, 35).env (2, 0, 40, 0, 40, 90)
            .mod (env2, "osc1Pitch", 20).amp (0, 250, 0, 240).reverb (35, 50, 1.2f, 45).level (0).standardMacros (20);
        add ("Oily Pipe", "Perc").osc (1, fm, 30).filter (1, svfBand, 1500, 60).noise (25, 2).amp (0, 500, 0, 170, 75).level (6).standardMacros (20);
        add ("Pressed Clave", "Perc").osc (1, analog, trianglePos, 55).noise (80, 0).filter (1, svfBand, 2200, 30).env (2, 0, 40, 0, 40, 90)
            .mod (env2, "osc1Pitch", 14).amp (0, 290, 0, 100).reverb (15, 50, 1.2f, 45).level (1).standardMacros (20);
        add ("Pressed Clink", "Perc").osc (1, metal, 40).filter (1, svfBand, 600, 40).noise (35, 2).amp (0, 490, 0, 190, 75).drive (hard, 35)
            .level (-12).standardMacros (20);
        add ("Pressed Kick", "Perc").osc (1, sub, 0).env (2, 0, 40, 0, 60, 90).mod (env2, "osc1Pitch", 35).amp (0, 190, 0, 80).drive (fuzz, 30, 4000)
            .filter (1, ladder24, 900, 0).level (-22).standardMacros (20);
        add ("Scorched Bolt", "Perc").osc (1, analog, 0).env (2, 0, 35, 0, 60, 90).mod (env2, "osc1Pitch", 40).amp (0, 460, 0, 80)
            .filter (1, ladder24, 800, 0).level (-8).standardMacros (20);
        add ("Snapping Hat", "Perc").osc (1, analog, 0).env (2, 0, 80, 0, 60, 90).mod (env2, "osc1Pitch", 35).amp (0, 370, 0, 80)
            .filter (1, ladder24, 900, 0).level (-8).standardMacros (20);
        add ("Static Shaker", "Perc").set ("osc1On", 0).noise (90, 3).filter (1, svfHigh, 8000, 15).amp (0, 170, 0, 80).level (4)
            .standardMacros (20);
        add ("Steel Kick", "Perc").osc (1, sub, 0).env (2, 0, 50, 0, 60, 90).mod (env2, "osc1Pitch", 40).amp (0, 190, 0, 80)
            .filter (1, ladder24, 1200, 0).level (-10).standardMacros (20);
        add ("Sterile Rim", "Perc").osc (1, fm, 80).filter (1, svfBand, 1100, 75).noise (20, 2).amp (0, 460, 0, 130, 75).drive (crush, 25).level (-10)
            .standardMacros (20);
        add ("Sunken Hat", "Perc").osc (1, analog, sinePos).env (2, 0, 90, 0, 60, 90).mod (env2, "osc1Pitch", 25).amp (0, 480, 0, 80)
            .filter (1, ladder24, 2000, 0).level (-10).standardMacros (20);
        add ("Tight Bolt", "Perc").osc (1, fm, 90).filter (1, svfBand, 1800, 45).noise (15, 2).amp (0, 360, 0, 340, 75).drive (crush, 30).level (-13)
            .standardMacros (20);
        add ("Toxic Block", "Perc").osc (1, analog, trianglePos, 55).noise (80, 0).filter (1, svfBand, 2500, 35).env (2, 0, 40, 0, 40, 90)
            .mod (env2, "osc1Pitch", 14).amp (0, 250, 0, 150).reverb (35, 50, 1.2f, 45).level (1).standardMacros (20);
        add ("Vacant Hat", "Perc").osc (1, metal, 50).filter (1, svfBand, 1300, 40).noise (25, 2).amp (0, 370, 0, 300, 75).drive (crush, 30)
            .level (0).standardMacros (20);

        // Vox, by name.
        add ("Acid Breath", "Vox").osc (1, vowel, 40).unison (1, 3, 10, 100).osc (2, vowel, 45, 55).pitch (2, -12, 11).noise (15, 1)
            .lfo (1, drift, 0.11f, 0).mod (lfo1, "osc1Position", 40).mod (lfo1, "osc2Position", -30).filter (1, svfLow, 2400, 10)
            .amp (1400, 1000, 90, 1700).reverb (40, 90, 7, 45).level (-10).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Open")
            .mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Ash Wraith", "Vox").osc (1, vowel, 5).unison (1, 1, 11, 100).lfo (1, sine, 0.31f, 0).mod (lfo1, "osc1Position", 40)
            .filter (1, svfBand, 3100, 25).amp (550, 1000, 90, 2100).reverb (35, 80, 7, 45).level (-9).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 60).macro (2, "Open").mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Bitter Mass", "Vox").osc (1, vowel, 35).unison (1, 1, 14, 100).osc (2, vowel, 25, 65).pitch (2, 12, 7).lfo (1, triangle, 0.13f, 0)
            .mod (lfo1, "osc1Position", 35).mod (lfo1, "osc2Position", -30).filter (1, svfLow, 3200, 25).amp (1250, 1000, 90, 1900)
            .reverb (40, 90, 4.5f, 45).level (-12).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Open")
            .mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Bleak Ghost", "Vox").osc (1, vowel, 35).unison (1, 6, 11, 100).osc (2, vowel, 45, 65).pitch (2, 12, 12).noise (10, 1)
            .lfo (1, triangle, 0.17f, 0).mod (lfo1, "osc1Position", 45).mod (lfo1, "osc2Position", -25).filter (1, ladder12, 2900, 30)
            .amp (1250, 1000, 90, 2300).reverb (35, 90, 7, 45).level (-12).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Open")
            .mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Concrete Throat", "Vox").osc (1, vowel, 45).unison (1, 3, 9, 100).osc (2, vowel, 10, 50).pitch (2, 0, 6).noise (15, 1)
            .lfo (1, triangle, 0.26f, 0).mod (lfo1, "osc1Position", 20).mod (lfo1, "osc2Position", -35).filter (1, ladder12, 3700, 30)
            .amp (850, 1000, 90, 1200).drive (crush, 35).reverb (35, 100, 7, 45).level (-12).macro (1, "Vowel").mod (macro1, "osc1Position", 60)
            .macro (2, "Open").mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Concrete Vowel", "Vox").osc (1, vowel, 100).unison (1, 1, 15, 100).osc (2, vowel, 60, 65).pitch (2, -12, 10).lfo (1, triangle, 0.25f, 0)
            .mod (lfo1, "osc1Position", 35).mod (lfo1, "osc2Position", -30).filter (1, svfBand, 3600, 30).amp (400, 1000, 90, 1300).drive (tube, 25)
            .reverb (50, 85, 5, 45).level (-15).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Open")
            .mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Dead Sigh", "Vox").osc (1, vowel, 30).unison (1, 4, 12, 100).osc (2, vowel, 45, 60).pitch (2, 12, 10).noise (20, 1)
            .lfo (1, drift, 0.09f, 0).mod (lfo1, "osc1Position", 20).mod (lfo1, "osc2Position", -15).filter (1, ladder12, 2600, 15)
            .amp (1150, 1000, 90, 1300).drive (tube, 25).reverb (35, 85, 7, 45).level (-17).macro (1, "Vowel").mod (macro1, "osc1Position", 60)
            .macro (2, "Open").mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Dead Vowel", "Vox").osc (1, vowel, 55).unison (1, 6, 9, 100).osc (2, vowel, 45, 65).pitch (2, 0, 6).noise (10, 1)
            .lfo (1, drift, 0.32f, 0).mod (lfo1, "osc1Position", 35).mod (lfo1, "osc2Position", -15).filter (1, svfLow, 2800, 20)
            .amp (1450, 1000, 90, 1400).drive (crush, 20).reverb (35, 90, 3.5f, 45).level (-12).macro (1, "Vowel").mod (macro1, "osc1Position", 60)
            .macro (2, "Open").mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Frozen Mouth", "Vox").osc (1, vowel, 5).unison (1, 6, 17, 100).osc (2, vowel, 65, 50).pitch (2, 12, 10).lfo (1, drift, 0.18f, 0)
            .mod (lfo1, "osc1Position", 25).mod (lfo1, "osc2Position", -30).filter (1, svfBand, 1800, 20).amp (700, 1000, 90, 2400)
            .reverb (30, 100, 4.5f, 45).level (-12).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Open")
            .mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Frozen Prayer", "Vox").osc (1, vowel, 50).unison (1, 6, 15, 100).osc (2, vowel, 5, 60).pitch (2, 0, 8).lfo (1, sine, 0.37f, 0)
            .mod (lfo1, "osc1Position", 30).mod (lfo1, "osc2Position", -25).filter (1, svfBand, 2000, 10).amp (600, 1000, 90, 1200).drive (crush, 35)
            .reverb (35, 100, 6.5f, 45).level (-10).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Open")
            .mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Ghostly Choir", "Vox").osc (1, vowel, 80).unison (1, 3, 16, 100).lfo (1, triangle, 0.07f, 0).mod (lfo1, "osc1Position", 35)
            .filter (1, ladder12, 3400, 25).amp (300, 1000, 90, 1000).reverb (35, 90, 7, 45).level (-12).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 60).macro (2, "Open").mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Ghostly Mouth", "Vox").osc (1, vowel, 90).unison (1, 3, 9, 100).noise (5, 1).lfo (1, sine, 0.12f, 0).mod (lfo1, "osc1Position", 25)
            .filter (1, ladder12, 2700, 20).amp (400, 1000, 90, 1600).drive (tube, 20).reverb (30, 90, 6, 45).level (-16).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 60).macro (2, "Open").mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Ghostly Voice", "Vox").osc (1, vowel, 75).unison (1, 3, 9, 100).noise (15, 1).lfo (1, drift, 0.05f, 0).mod (lfo1, "osc1Position", 40)
            .filter (1, ladder12, 2300, 25).amp (500, 1000, 90, 1400).reverb (40, 85, 7.5f, 45).level (-12).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 60).macro (2, "Open").mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Hollow Mouth", "Vox").osc (1, vowel, 60).unison (1, 6, 17, 100).noise (15, 1).lfo (1, sine, 0.12f, 0).mod (lfo1, "osc1Position", 30)
            .filter (1, svfBand, 1500, 10).amp (350, 1000, 90, 1700).reverb (35, 95, 8, 45).level (-9).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 60).macro (2, "Open").mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Hollow Speech", "Vox").osc (1, vowel, 15).unison (1, 6, 9, 100).noise (10, 1).lfo (1, sine, 0.12f, 0).mod (lfo1, "osc1Position", 30)
            .filter (1, svfLow, 1800, 20).amp (850, 1000, 90, 2500).reverb (40, 95, 4, 45).level (-12).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 60).macro (2, "Open").mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Hollow Vowel", "Vox").osc (1, vowel, 15).unison (1, 3, 9, 100).osc (2, vowel, 95, 60).pitch (2, 12, 8).noise (15, 1)
            .lfo (1, sine, 0.37f, 0).mod (lfo1, "osc1Position", 35).mod (lfo1, "osc2Position", -30).filter (1, svfLow, 3100, 30)
            .amp (1100, 1000, 90, 1900).reverb (45, 85, 4, 45).level (-12).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Open")
            .mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Industrial Chant", "Vox").osc (1, vowel, 40).unison (1, 4, 12, 100).lfo (1, triangle, 0.38f, 0).mod (lfo1, "osc1Position", 40)
            .filter (1, svfLow, 2400, 30).amp (400, 1000, 90, 1700).reverb (55, 90, 5, 45).level (-12).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 60).macro (2, "Open").mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Industrial Vowel", "Vox").osc (1, vowel, 75).unison (1, 6, 17, 100).osc (2, vowel, 50, 65).pitch (2, 0, 10).lfo (1, triangle, 0.18f, 0)
            .mod (lfo1, "osc1Position", 30).mod (lfo1, "osc2Position", -20).filter (1, svfLow, 4000, 30).amp (550, 1000, 90, 2300)
            .reverb (40, 85, 4.5f, 45).level (-12).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Open")
            .mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Iron Choir", "Vox").osc (1, vowel, 85).unison (1, 6, 9, 100).osc (2, vowel, 35, 65).pitch (2, 7, 8).lfo (1, sine, 0.13f, 0)
            .mod (lfo1, "osc1Position", 30).mod (lfo1, "osc2Position", -20).filter (1, svfLow, 3100, 35).amp (1350, 1000, 90, 900)
            .reverb (50, 80, 4, 45).level (-12).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Open")
            .mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Moaning Murmur", "Vox").osc (1, vowel, 70).unison (1, 1, 18, 100).osc (2, vowel, 25, 50).pitch (2, 7, 10).lfo (1, sine, 0.37f, 0)
            .mod (lfo1, "osc1Position", 30).mod (lfo1, "osc2Position", -30).filter (1, ladder12, 2900, 30).amp (250, 1000, 90, 1800)
            .reverb (55, 95, 7, 45).level (-14).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Open")
            .mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Muttering Ghost", "Vox").osc (1, vowel, 10).unison (1, 6, 15, 100).lfo (1, triangle, 0.37f, 0).mod (lfo1, "osc1Position", 25)
            .filter (1, svfLow, 3500, 30).amp (300, 1000, 90, 2300).drive (tube, 35).reverb (30, 90, 4, 45).level (-20).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 60).macro (2, "Open").mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Nickel Mass", "Vox").osc (1, vowel, 95).unison (1, 3, 16, 100).osc (2, vowel, 95, 70).pitch (2, 7, 11).lfo (1, triangle, 0.08f, 0)
            .mod (lfo1, "osc1Position", 35).mod (lfo1, "osc2Position", -25).filter (1, ladder12, 3300, 30).amp (1350, 1000, 90, 1100).drive (tube, 35)
            .reverb (45, 95, 6, 45).level (-22).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Open")
            .mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Oily Throat", "Vox").osc (1, vowel, 60).unison (1, 6, 12, 100).noise (15, 1).lfo (1, drift, 0.16f, 0).mod (lfo1, "osc1Position", 20)
            .filter (1, svfBand, 2500, 30).amp (1200, 1000, 90, 1700).reverb (35, 100, 4, 45).level (-5).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 60).macro (2, "Open").mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Riveted Vox", "Vox").osc (1, vowel, 20).unison (1, 6, 9, 100).lfo (1, sine, 0.07f, 0).mod (lfo1, "osc1Position", 35)
            .filter (1, svfLow, 2700, 30).amp (950, 1000, 90, 2400).reverb (55, 80, 5.5f, 45).level (-12).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 60).macro (2, "Open").mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Robotic Breath", "Vox").osc (1, vowel, 55).unison (1, 1, 8, 100).lfo (1, triangle, 0.11f, 0).mod (lfo1, "osc1Position", 40)
            .filter (1, ladder12, 2000, 10).amp (1300, 1000, 90, 900).reverb (55, 85, 5, 45).level (-9).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 60).macro (2, "Open").mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Ruined Prayer", "Vox").osc (1, vowel, 35).unison (1, 1, 14, 100).osc (2, vowel, 85, 55).pitch (2, 12, 9).lfo (1, drift, 0.25f, 0)
            .mod (lfo1, "osc1Position", 30).mod (lfo1, "osc2Position", -30).filter (1, svfBand, 2600, 25).amp (500, 1000, 90, 2400)
            .reverb (50, 90, 7, 45).level (-12).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Open")
            .mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Scorched Hymn", "Vox").osc (1, vowel, 90).unison (1, 3, 12, 100).lfo (1, sine, 0.15f, 0).mod (lfo1, "osc1Position", 40)
            .filter (1, svfBand, 2300, 15).amp (450, 1000, 90, 2400).reverb (55, 85, 3.5f, 45).level (-12).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 60).macro (2, "Open").mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Sterile Throat", "Vox").osc (1, vowel, 60).unison (1, 6, 12, 100).osc (2, vowel, 25, 50).pitch (2, 0, 7).lfo (1, drift, 0.12f, 0)
            .mod (lfo1, "osc1Position", 40).mod (lfo1, "osc2Position", -30).filter (1, svfLow, 1500, 20).amp (800, 1000, 90, 1700)
            .reverb (45, 100, 3, 45).level (-12).macro (1, "Vowel").mod (macro1, "osc1Position", 60).macro (2, "Open")
            .mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Sunken Mass", "Vox").osc (1, vowel, 15).unison (1, 1, 16, 100).lfo (1, triangle, 0.35f, 0).mod (lfo1, "osc1Position", 45)
            .filter (1, ladder12, 1500, 15).amp (1150, 1000, 90, 2400).reverb (30, 85, 3.5f, 45).level (-10).macro (1, "Vowel")
            .mod (macro1, "osc1Position", 60).macro (2, "Open").mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
        add ("Vacant Psalm", "Vox").osc (1, vowel, 65).unison (1, 3, 12, 100).osc (2, vowel, 10, 65).pitch (2, 12, 6).noise (15, 1)
            .lfo (1, sine, 0.2f, 0).mod (lfo1, "osc1Position", 20).mod (lfo1, "osc2Position", -35).filter (1, ladder12, 1900, 20)
            .amp (800, 1000, 90, 2000).drive (crush, 20).reverb (45, 95, 4.5f, 45).level (-12).macro (1, "Vowel").mod (macro1, "osc1Position", 60)
            .macro (2, "Open").mod (macro2, "filter1Cutoff", 30).macro (3, "Space").mod (macro3, "fxReverbMix", 40);
    }
}
