#include "WaveSynthPatchBuilder.h"

// The Complex patches: four oscillators or more in each, made for a techno and electronic-industrial
// floor, the categories that are not club sounds included. Each is built from roles, named above it:
// weight (a sine or sub, mono, in the middle), body (the tone you would call the sound), bite (a harder
// layer at the start of a note), width (a detuned unison layer kept out of the low end), air (above
// 6 kHz), grit (crushed, folded or metal, low in the mix), movement (layers that come and go by their
// own envelopes, LFOs or steps), strike (the first few milliseconds only) and modulators (oscillators
// never heard, only driving another's FM through FM From, sometimes two deep).
//
// Width and air go to filter 2: a high-pass beside filter 1 (parallel), or, in series with filter 2
// off, past filter 1 untouched. Weight stays at one unison voice in the middle, so below 120 Hz the
// two channels match. No patch asks for more than 48 unison voices a note, so eight notes sound at once.

namespace WaveSynthPatchBuilder
{
    void addComplex (std::vector<Builder>& b)
    {
        const auto add = [&b] (const char* name, const char* category) -> Builder& { b.emplace_back (name, category); return b.back(); };

        //======================================================================
        // Bass

        // Nine. Weight 1; body 2, whose FM comes from 5, which takes 6's metal (two deep, neither heard);
        // body 3 an octave up, squeezed; bite 4, a sync sweep only at the start; strike 7; width 8 and
        // air 9 high-passed beside the low-pass. A step pattern moves 2's FM and 8's level each 16th.
        add ("Rivet Engine", "Bass").osc (1, analog, sinePos, 70)
            .osc (2, analog, 50, 60).fm (2, 5, 6).modulator (5, analog, sinePos).pitch (5, 12).fm (5, 6, 20).modulator (6, metal, 60).pitch (6, 7)
            .osc (3, pulse, 30, 30).pitch (3, 12).warp (3, wSqueeze, 30)
            .osc (4, sync, 40, 45).pitch (4, 12).warp (4, wSync, 20).levelEnv (4, 0, 0, 70, 0)
            .osc (7, fm, 85, 35).pitch (7, 24).levelEnv (7, 0, 0, 18, 0).route (7, 100)
            .osc (8, analog, sawPos, 40).pitch (8, 12).unison (8, 3, 18, 100).route (8, 100)
            .osc (9, additive, 90, 25).pitch (9, 24).route (9, 100)
            .parallel().filter (1, ladder24, 650, 35, 30, 30).filter (2, svfHigh, 300, 10)
            .env (2, 0, 180, 0, 80, 70).mod (env2, "filter1Cutoff", 30).mod (env2, "osc4WarpAmount", 40)
            .env (3, 0, 90, 0, 50, 70).mod (env3, "osc2WarpAmount", 60)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { 0, 60, -40, 100, 0, -80, 40, 60, 0, 100, -40, 20, 0, -60, 80, -100 })
            .mod (lfo1, "osc2WarpAmount", 15).mod (lfo1, "osc8Volume", 15)
            .mod (velocity, "osc2WarpAmount", 30).mod (velocity, "osc7Volume", 30)
            .amp (1, 160, 40, 60).mono (0.0f, true).drive (tube, 25, 6000).level (-5.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Clang").mod (macro2, "osc2WarpAmount", 40)
            .macro (3, "Width").mod (macro3, "osc8Volume", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        // Five. Weight 1; body 2, a mirrored pulse a triplet LFO pulls about; grit 3, a growl an octave
        // up; strike 4, a bell (a sine two octaves up FM'd by 5 at 3.5 times its pitch, never heard)
        // that rings for a moment on every note, past the low-pass.
        add ("Hollow Gantry", "Bass").osc (1, analog, sinePos, 75)
            .osc (2, pulse, 50, 55).warp (2, wMirror, 20)
            .osc (3, growl, 40, 28).pitch (3, 12)
            .osc (4, analog, sinePos, 70).pitch (4, 24).fm (4, 5, 35).levelEnv (4, 0, 0, 800, 0).route (4, 100)
            .modulator (5, analog, sinePos).pitch (5, 21, 69)
            .filter (1, ladder24, 500, 30, 25, 30)
            .lfoSync (1, triangle, eighthT, 0).mod (lfo1, "osc2WarpAmount", 45).mod (lfo1, "filter1Cutoff", 15)
            .env (2, 0, 220, 0, 90, 70).mod (env2, "filter1Cutoff", 30)
            .mod (velocity, "osc2WarpAmount", 30).mod (velocity, "filter1Cutoff", 20)
            .amp (1, 200, 50, 70).mono (0.0f, true).drive (hard, 20, 5000).level (-10.3)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Bell").mod (macro2, "osc4Volume", 40)
            .macro (3, "Growl").mod (macro3, "osc3Volume", 50).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        //======================================================================
        // Sub

        // Five. The sub is one clean sine, past the filter; everything else is a voice above it. Weight
        // 1; 2 a growl an octave up FM'd by 5 (unheard) a fifth above it, gated on the off-beat sixteenths
        // by a step pattern, so the note talks between the kicks; 3 a vowel a twelfth up that the half-bar
        // LFO opens and the mod wheel brings forward; strike 4. A hard note growls harder.
        add ("Basalt Floor", "Sub").osc (1, analog, sinePos, 85).route (1, 100)
            .osc (2, growl, 30, 45).pitch (2, 12).fm (2, 5, 15).modulator (5, analog, sinePos).pitch (5, 19)
            .osc (3, vowel, 30, 18).pitch (3, 19)
            .osc (4, metal, 60, 25).pitch (4, 24).levelEnv (4, 0, 0, 18, 0)
            .filter (1, svfBand, 700, 25, 0, 60)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { -100, 60, -100, 100, -100, 40, -100, 100, -100, 60, -100, 100, -100, 20, 100, 100 })
            .mod (lfo1, "osc2Volume", 45).mod (lfo1, "osc2WarpAmount", 20)
            .lfoSync (2, triangle, bars2, 0).mod (lfo2, "osc3Position", 60).mod (lfo2, "filter1Cutoff", 20)
            .mod (velocity, "osc2WarpAmount", 30).mod (modWheel, "osc3Volume", 50)
            .env (3, 0, 45, 0, 30, 70).mod (env3, "osc1Pitch", 4.17f)
            .amp (2, 300, 85, 90).mono (0.0f, true).drive (foldDrive, 0, 6000).level (-6.1)
            .macro (1, "Talk").mod (macro1, "osc2Volume", 40).macro (2, "Vowel").mod (macro2, "osc3Volume", 50)
            .macro (3, "Grind").mod (macro3, "fxDriveAmount", 60).macro (4, "Snarl").mod (macro4, "osc2WarpAmount", 40);

        // Nine, a sub that breathes against the kick. Weight 1, a clean sine past the filter; above it 2,
        // a saw an octave up FM'd by 6 (unheard) that swells through each beat and is gone on the next
        // one, as if ducked; 3 an FM table a twelfth up that does the opposite, so the top swaps on every
        // beat; bite 4, a sync the envelope sweeps on each note; strike 5; 7 (unheard) moves 3's FM over
        // four bars; 8 a vowel the mod wheel opens; grit 9, a crushed square that only a hard note brings.
        add ("Pressure Vault", "Sub").osc (1, analog, sinePos, 80).route (1, 100)
            .osc (2, analog, sawPos, 30).pitch (2, 12).fm (2, 6, 20).modulator (6, analog, sinePos).pitch (6, 12)
            .osc (3, fm, 40, 22).pitch (3, 19).fm (3, 7, 25).modulator (7, analog, sinePos).pitch (7, 31)
            .osc (4, sync, 40, 20).pitch (4, 12).warp (4, wSync, 20)
            .osc (5, pulse, 15, 20).pitch (5, 24).levelEnv (5, 0, 0, 12, 0)
            .osc (8, vowel, 40, 0).pitch (8, 24)
            .osc (9, analog, squarePos, 0).pitch (9, 12).warp (9, wCrush, 70)
            .filter (1, ladder12, 1400, 20, 0, 50)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { -100, -50, 20, 100, -100, -50, 20, 100, -100, -50, 20, 100, -100, -60, 40, 100 })
            .mod (lfo1, "osc2Volume", 40).mod (lfo1, "osc3Volume", -30)
            .lfoSync (2, sine, bars4, 0).mod (lfo2, "osc3WarpAmount", 35).mod (lfo2, "osc3Position", 30).mod (lfo2, "filter1Cutoff", 15)
            .env (2, 0, 160, 0, 60, 70).mod (env2, "osc4WarpAmount", 45)
            .mod (velocity, "osc9Volume", 30).mod (velocity, "osc4Volume", 20).mod (modWheel, "osc8Volume", 45)
            .mod (velocity, "osc3WarpAmount", 30).mod (velocity, "osc5Volume", 30)
            .amp (2, 300, 85, 90).mono (0.0f, true).drive (foldDrive, 0, 6000).level (-4.3)
            .macro (1, "Breath").mod (macro1, "osc2Volume", 40).macro (2, "Bite").mod (macro2, "osc4WarpAmount", 40)
            .macro (3, "Grind").mod (macro3, "fxDriveAmount", 60).macro (4, "Vowel").mod (macro4, "osc8Volume", 50);

        //======================================================================
        // Reese

        // Nine. Weight 1; body 2 and 3, saws eleven cents apart in the middle (the beating survives
        // mono), 3 FM'd faintly by 8, a sine three cents off, so the beating has a second, slower one
        // inside it; width 4 an octave up, high-passed; 5 a growl pulled by 9, a triangle an octave
        // below; 6 a vowel the long LFO turns; grit 7.
        add ("Tidal Shear", "Reese").osc (1, analog, sinePos, 60)
            .osc (2, analog, sawPos, 55).pitch (2, 0, -11)
            .osc (3, analog, sawPos, 55).pitch (3, 0, 11).fm (3, 8, 4).modulator (8, analog, sinePos).pitch (8, 0, 3)
            .osc (4, analog, sawPos, 25).pitch (4, 12).unison (4, 4, 25, 100).route (4, 100)
            .osc (5, growl, 30, 40).fm (5, 9, 12).modulator (9, analog, trianglePos).pitch (9, -12)
            .osc (6, vowel, 50, 30).pitch (6, 12)
            .osc (7, metal, 40, 6).pitch (7, 24).warp (7, wFold, 30).route (7, 100)
            .parallel().filter (1, ladder24, 420, 30, 30, 20).filter (2, svfHigh, 260, 10)
            .lfoSync (1, sine, bars2, 0).mod (lfo1, "filter1Cutoff", 18).mod (lfo1, "osc5Position", 30)
            .lfoSync (2, triangle, bars4, 0).mod (lfo2, "osc6Position", 40).mod (lfo2, "osc3WarpAmount", 12)
            .env (2, 5, 600, 30, 300).mod (env2, "filter1Cutoff", 15)
            .mod (velocity, "osc5WarpAmount", 30).mod (velocity, "filter1Cutoff", 20)
            .amp (5, 400, 100, 180).mono (50).drive (fuzz, 25, 5000).level (-20.5)
            .macro (1, "Shear").mod (macro1, "osc3Fine", 10).macro (2, "Throat").mod (macro2, "osc6Volume", 30)
            .macro (3, "Width").mod (macro3, "osc4Volume", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        // Five. Weight 1; body 2 and 3, vowels nine cents apart whose mouths an LFO turns opposite ways,
        // so the beating speaks; width 4, a vowel an octave up in three voices, high-passed; bite 5, a
        // sync sweep on each note's start.
        add ("Smolder Choir", "Reese").osc (1, analog, sinePos, 60)
            .osc (2, vowel, 20, 50).pitch (2, 0, -9)
            .osc (3, vowel, 60, 50).pitch (3, 0, 9)
            .osc (4, vowel, 40, 35).pitch (4, 12).unison (4, 3, 20, 100).route (4, 100)
            .osc (5, sync, 30, 50).pitch (5, 12).warp (5, wSync, 25).levelEnv (5, 0, 0, 250, 0)
            .parallel().filter (1, ladder12, 700, 25, 20, 20).filter (2, svfHigh, 300, 10)
            .lfoSync (1, sine, bar1, 0).mod (lfo1, "osc2Position", 35).mod (lfo1, "osc3Position", -35)
            .lfo (2, drift, 0.2f, 0).mod (lfo2, "filter1Cutoff", 10).mod (lfo2, "osc4Position", 25)
            .mod (velocity, "osc5WarpAmount", 30).mod (velocity, "filter1Cutoff", 20)
            .amp (5, 400, 100, 200).mono (60).drive (tube, 30, 5000).level (-11.1)
            .macro (1, "Vowel").mod (macro1, "osc2Position", 30).macro (2, "Shear").mod (macro2, "osc3Fine", 10)
            .macro (3, "Width").mod (macro3, "osc4Volume", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        //======================================================================
        // Acid

        // Nine. Body 1, a saw FM'd by 4 (unheard), which is FM'd by 5's metal a fifth up (two deep), so
        // the accent opens a clang as well as the filter; weight 2 an octave down; 3 a squeezed pulse
        // an octave up; bite 6 a sync sweep; strike 7 past the filter; width 8; air 9 past the
        // filter. Steps move the resonance and the FM under the line; a slow LFO rides the cutoff.
        add ("Caustic Lattice", "Acid").osc (1, analog, sawPos, 65).fm (1, 4, 0)
            .modulator (4, analog, sinePos).pitch (4, 12).fm (4, 5, 25).modulator (5, metal, 30).pitch (5, 7)
            .osc (2, analog, squarePos, 35).pitch (2, -12)
            .osc (3, pulse, 30, 20).pitch (3, 12).warp (3, wSqueeze, 40)
            .osc (6, sync, 40, 30).pitch (6, 12).warp (6, wSync, 10).levelEnv (6, 0, 0, 80, 0)
            .osc (7, fm, 90, 25).pitch (7, 24).levelEnv (7, 0, 0, 20, 0).route (7, 100)
            .osc (8, analog, sawPos, 14).pitch (8, 12).unison (8, 3, 15, 100)
            .osc (9, additive, 90, 8).pitch (9, 24).route (9, 100)
            .filter (1, ladder24, 500, 72, 40, 40)
            .env (2, 0, 230, 0, 90, 75).mod (env2, "filter1Cutoff", 45).mod (env2, "osc1WarpAmount", 30).mod (env2, "osc6WarpAmount", 30)
            .mod (velocity, "filter1Cutoff", 15)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { 100, 0, -60, 40, 0, 80, -100, 20, 60, 0, -40, 100, -20, 40, 0, -80 })
            .mod (lfo1, "filter1Resonance", 12).mod (lfo1, "osc1WarpAmount", 15)
            .lfoSync (2, triangle, bars4, 0).mod (lfo2, "filter1Cutoff", 15)
            .amp (1, 200, 70, 60).mono (70).drive (fuzz, 30, 7000).delay (eighthD, 30, 12, 3000, false).level (-19.9)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Corrode").mod (macro2, "osc1WarpAmount", 40)
            .macro (3, "Res").mod (macro3, "filter1Resonance", 25).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        // Five. Body 1, a plucked string, so every note of the line is struck; 2 a saw FM'd by 5 (unheard,
        // the third harmonic), the FM stepping on the sixteenths so a held note squelches in rhythm;
        // weight 3 an octave down; 4 a vowel an octave up whose mouth the steps throw open and shut. The
        // filter climbs over two bars; a hard note opens it and drives the FM harder. Every note is
        // struck and slides from the last (struck, not legato: a slid legato note takes its new
        // velocity at once, and what velocity moves would jump).
        add ("Wet Circuit", "Acid").osc (1, string, 75, 55)
            .osc (2, analog, sawPos, 55).fm (2, 5, 0).modulator (5, analog, sinePos).pitch (5, 19)
            .osc (3, analog, squarePos, 45).pitch (3, -12)
            .osc (4, vowel, 10, 40).pitch (4, 12)
            .filter (1, ladder24, 650, 65, 30, 40)
            .env (2, 0, 180, 0, 80, 75).mod (env2, "filter1Cutoff", 40).mod (env2, "osc2WarpAmount", 25)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { 100, -40, 0, 80, -100, 60, -20, 100, 40, -100, 80, 0, -60, 100, -40, 60 })
            .mod (lfo1, "osc2WarpAmount", 25).mod (lfo1, "osc4Position", 45)
            .lfoSync (2, sawUp, bars2, 0).mod (lfo2, "filter1Cutoff", 20)
            .mod (velocity, "filter1Cutoff", 15).mod (velocity, "osc2WarpAmount", 20)
            .amp (3, 180, 60, 60).mono (55, false).glideAlways().drive (hard, 25, 6000).delay (sixteenthD, 25, 15, 3500, false).level (-8.7)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Talk").mod (macro2, "osc4Volume", 30)
            .macro (3, "Squelch").mod (macro3, "filter1Resonance", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        //======================================================================
        // Lead

        // Nine, no saws. Body 1, a vowel whose mouth a step pattern opens and shuts on the sixteenths, so
        // the line talks; 2 a folded triangle that takes over from it across four bars; bite 3, a sine an
        // octave up at the end of an FM chain, 7 (unheard) driving 6 (unheard) driving 3, the chain pulsing
        // with the steps and biting harder the harder the note; 4 a sync scream the mod wheel brings in;
        // strike 5, past the filter; air 8; 9 a growl an octave down under it all.
        add ("Spark Foreman", "Lead").osc (1, vowel, 20, 50)
            .osc (2, analog, trianglePos, 0).warp (2, wFold, 45)
            .osc (3, analog, sinePos, 32).pitch (3, 12).fm (3, 6, 30).modulator (6, analog, sinePos).pitch (6, 19)
            .fm (6, 7, 40).modulator (7, analog, sinePos).pitch (7, 5, 7)
            .osc (4, sync, 50, 0).pitch (4, 12).warp (4, wSync, 35)
            .osc (5, metal, 80, 28).pitch (5, 24).levelEnv (5, 0, 0, 30, 0).route (5, 100)
            .osc (8, additive, 90, 12).pitch (8, 24).unison (8, 2, 10, 100).route (8, 100)
            .osc (9, growl, 40, 18).pitch (9, -12)
            .filter (1, svfLow, 3800, 20, 0, 50)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { 100, -60, 20, -100, 60, -20, 100, -80, 40, -100, 80, 0, -60, 100, -40, 20 })
            .mod (lfo1, "osc1Position", 45).mod (lfo1, "osc3WarpAmount", 20)
            .lfoSync (2, triangle, bars4, 0).mod (lfo2, "osc1Volume", -35).mod (lfo2, "osc2Volume", 45)
            .mod (velocity, "osc3WarpAmount", 30).mod (velocity, "osc5Volume", 25)
            .mod (modWheel, "osc4Volume", 50).mod (modWheel, "osc4WarpAmount", 30)
            .env (2, 0, 300, 30, 200).mod (env2, "filter1Cutoff", 20)
            .amp (5, 300, 85, 250).mono (40).drive (tube, 20, 8000).delay (eighthD, 30, 18, 4000).level (-6.5)
            .macro (1, "Mouth").mod (macro1, "osc1Position", 40).macro (2, "Chain").mod (macro2, "osc6WarpAmount", 40)
            .macro (3, "Fold").mod (macro3, "osc2WarpAmount", 40).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        // Six. Body 1 and 2, vowels an octave apart whose mouths a half-bar LFO turns opposite ways;
        // 3 a sine FM'd by 4 (unheard, seven cents off it), the static in the voice, which swells with
        // each note; weight 5 a growl an octave down; strike 6.
        add ("Static Vespers", "Lead").osc (1, vowel, 30, 50)
            .osc (2, vowel, 70, 30).pitch (2, 12)
            .osc (3, analog, sinePos, 40).fm (3, 4, 15).modulator (4, analog, sinePos).pitch (4, 0, 7)
            .osc (5, growl, 40, 22).pitch (5, -12)
            .osc (6, pulse, 50, 25).pitch (6, 24).levelEnv (6, 0, 0, 22, 0)
            .filter (1, ladder12, 3000, 20, 0, 40)
            .lfo (1, sine, 5.0f, 1).lfoFade (1, 600).mod (lfo1, "osc1Fine", 5).mod (lfo1, "osc3Fine", 5)
            .lfoSync (2, triangle, half, 0).mod (lfo2, "osc1Position", 35).mod (lfo2, "osc2Position", -30)
            .env (2, 0, 500, 0, 200).mod (env2, "osc3WarpAmount", 25)
            .mod (velocity, "osc6Volume", 30).mod (velocity, "filter1Cutoff", 20)
            .amp (10, 400, 90, 300).mono (80).drive (tube, 0, 8000).chorus (25).delay (quarter, 30, 20, 4000).reverb (25, 70, 2.5f).level (2.5)
            .macro (1, "Vowel").mod (macro1, "osc1Position", 30).macro (2, "Choir").mod (macro2, "osc2Volume", 30)
            .macro (3, "Static").mod (macro3, "osc3WarpAmount", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        //======================================================================
        // Stab

        // Nine. Body 1 to 4, a minor seventh chord of saws on one key; weight 5, a pulse an octave down;
        // bite 6, a sine FM'd by 7 (unheard, two octaves up) for the first tenth of a second; width 8;
        // strike 9. A resonant low-pass snaps shut on each hit, into a dotted delay.
        add ("Bulkhead Chord", "Stab").osc (1, analog, sawPos, 40)
            .osc (2, analog, sawPos, 34).pitch (2, 3, -6)
            .osc (3, analog, sawPos, 34).pitch (3, 7, 5)
            .osc (4, analog, sawPos, 28).pitch (4, 10)
            .osc (5, pulse, 50, 30).pitch (5, -12)
            .osc (6, analog, sinePos, 25).pitch (6, 12).fm (6, 7, 30).levelEnv (6, 0, 0, 90, 0).modulator (7, analog, sinePos).pitch (7, 24)
            .osc (8, analog, sawPos, 15).pitch (8, 12).unison (8, 3, 20, 100)
            .osc (9, metal, 80, 15).pitch (9, 24).levelEnv (9, 0, 0, 40, 0)
            .filter (1, ladder24, 900, 45, 25, 40)
            .env (2, 0, 260, 0, 120, 70).mod (env2, "filter1Cutoff", 40).mod (velocity, "filter1Cutoff", 12)
            .lfoSync (1, sine, bars4, 0).mod (lfo1, "filter1Cutoff", 12)
            .amp (1, 350, 0, 220).delay (eighthD, 45, 28, 2500).reverb (25, 75, 2.5f).level (3.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Dub").mod (macro2, "fxDelayFeedback", 30)
            .macro (3, "Bite").mod (macro3, "osc6Volume", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        // Five. Body 1, metal; 2 an FM table a fifth up; 3 a growl at the note, the low end left to the kick; 4 a ring, a
        // sine an octave up FM'd by 5 (unheard) two and a half times its pitch, dying first. Fuzz into a
        // large room.
        add ("Rust Cathedral", "Stab").osc (1, metal, 40, 45).warp (1, wFold, 0)
            .osc (2, fm, 60, 30).pitch (2, 7)
            .osc (3, growl, 30, 30)
            .osc (4, analog, sinePos, 35).pitch (4, 12).fm (4, 5, 40).levelEnv (4, 0, 0, 250, 0).modulator (5, analog, sinePos).pitch (5, 15, 86)
            .filter (1, ladder12, 2500, 20, 0, 30)
            .env (2, 0, 180, 0, 100, 70).mod (env2, "filter1Cutoff", 30).mod (env2, "osc4WarpAmount", 30)
            .mod (velocity, "osc1WarpAmount", 30).mod (velocity, "filter1Cutoff", 20)
            .amp (1, 400, 0, 350).drive (fuzz, 35, 4500).reverb (30, 85, 3.0f).level (-10.9)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Ring").mod (macro2, "osc4Volume", 30)
            .macro (3, "Rust").mod (macro3, "osc1WarpAmount", 70).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        //======================================================================
        // Chord

        // Nine, taking turns on a held chord: strike 5, a sine FM'd by 6 (unheard), and 7, a plucked
        // string, at the start; body 1, three saws, throughout; a vowel 2 rising over a third of a
        // second; additive 3 over most of one; width 8 over a second and a half; weight 4; air 9 past
        // the filter, its position turning with the vowel's.
        add ("Sodium Choir", "Chord").osc (1, analog, sawPos, 30).unison (1, 3, 14, 70)
            .osc (2, vowel, 25, 25).levelEnv (2, 300, 0, 2000, 60)
            .osc (3, additive, 50, 18).pitch (3, 12).levelEnv (3, 900, 0, 3000, 30)
            .osc (4, pulse, 30, 22).pitch (4, -12)
            .osc (5, analog, sinePos, 18).pitch (5, 12).fm (5, 6, 20).levelEnv (5, 0, 0, 400, 0).modulator (6, analog, sinePos).pitch (6, 19)
            .osc (7, string, 60, 20).pitch (7, 12)
            .osc (8, analog, sawPos, 14).pitch (8, 12).unison (8, 3, 22, 100).levelEnv (8, 1500, 0, 0, 100)
            .osc (9, metal, 90, 6).pitch (9, 24).route (9, 100)
            .filter (1, ladder12, 3000, 15, 0, 30)
            .lfoSync (1, triangle, bars2, 0).mod (lfo1, "osc2Position", 30).mod (lfo1, "osc9Position", 40)
            .lfo (2, sine, 0.13f, 0).mod (lfo2, "filter1Cutoff", 10)
            .env (2, 0, 1200, 30, 600).mod (env2, "filter1Cutoff", 20)
            .mod (velocity, "filter1Cutoff", 20)
            .amp (8, 600, 85, 900).chorus (30).reverb (30, 80, 3.5f).level (4.6)
            .macro (1, "Dark").mod (macro1, "filter1Cutoff", -40).macro (2, "Voice").mod (macro2, "osc2Volume", 30)
            .macro (3, "Air").mod (macro3, "osc9Volume", 50).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        // Five. A chord that rewires itself each sixteenth: two step patterns hand the level between body
        // 1 (a pulse) and 2 (a saw), and between 3 (an FM table an octave up, FM'd by 5, unheard, a fifth
        // above it) and 4 (metal), on different steps.
        add ("Broken Relay", "Chord").osc (1, pulse, 40, 40)
            .osc (2, analog, sawPos, 35)
            .osc (3, fm, 50, 30).pitch (3, 12).fm (3, 5, 15).modulator (5, analog, sinePos).pitch (5, 19)
            .osc (4, metal, 30, 20).pitch (4, 12)
            .filter (1, ladder24, 1800, 30, 0, 30)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { 100, -100, 100, 100, -100, 100, -100, -100, 100, -100, 100, -100, 100, 100, -100, 100 })
            .mod (lfo1, "osc1Volume", 40).mod (lfo1, "osc2Volume", -35)
            .lfoSync (2, steps, bar1, 0).stepPattern (2, { -100, 100, 100, -100, 100, -100, -100, 100, -100, 100, -100, 100, 100, -100, 100, -100 })
            .mod (lfo2, "osc3Volume", 30).mod (lfo2, "osc4Volume", -20)
            .env (2, 0, 500, 30, 300).mod (env2, "filter1Cutoff", 20)
            .mod (velocity, "osc3WarpAmount", 30).mod (velocity, "filter1Cutoff", 20)
            .amp (3, 500, 80, 300).reverb (20, 60, 2.0f).level (-11.9)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Metal").mod (macro2, "osc4Volume", 60)
            .macro (3, "FM").mod (macro3, "osc3WarpAmount", 40).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        //======================================================================
        // Pluck

        // Nine. Body 1, a plucked string; 2 an FM pluck, a sine FM'd by 3 (unheard) an octave over it;
        // 4 a high tine FM'd by 5 (unheard) at an odd interval; weight 6, a thump an octave down;
        // strike 7; width 8, fading; air 9. A random step each sixteenth changes 2's FM, so no two
        // plucks are alike.
        add ("Needle Rain", "Pluck").osc (1, string, 80, 50)
            .osc (2, analog, sinePos, 34).pitch (2, 12).fm (2, 3, 30).levelEnv (2, 0, 0, 220, 0).modulator (3, analog, sinePos).pitch (3, 24)
            .osc (4, analog, sinePos, 22).pitch (4, 24).fm (4, 5, 20).levelEnv (4, 0, 0, 90, 0).modulator (5, metal, 50).pitch (5, 17)
            .osc (6, pulse, 20, 25).pitch (6, -12).levelEnv (6, 0, 0, 160, 0)
            .osc (7, fm, 95, 28).pitch (7, 24).levelEnv (7, 0, 0, 10, 0)
            .osc (8, analog, sawPos, 13).pitch (8, 12).unison (8, 3, 15, 100).levelEnv (8, 0, 0, 300, 0)
            .osc (9, additive, 95, 9).pitch (9, 24).levelEnv (9, 0, 0, 500, 0).route (9, 100)
            .filter (1, ladder12, 3200, 20, 0, 50)
            .env (2, 0, 250, 0, 150, 70).mod (env2, "filter1Cutoff", 30)
            .lfoSync (1, randomShape, sixteenth, 0).mod (lfo1, "osc2WarpAmount", 15)
            .mod (velocity, "osc7Volume", 30).mod (velocity, "filter1Cutoff", 20)
            .mod (velocity, "osc1Position", 40)
            .amp (1, 600, 0, 400).delay (eighthD, 40, 25, 4000).reverb (20, 60, 2.0f).level (3.1)
            .macro (1, "Echo").mod (macro1, "fxDelayMix", 40).macro (2, "Tine").mod (macro2, "osc4Volume", 30)
            .macro (3, "Thump").mod (macro3, "osc6Volume", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        // Four. Body 1, a plucked string; 2 metal an octave up, dying fast; 3 an FM table; strike 4.
        add ("Copper Tick", "Pluck").osc (1, string, 50, 59)
            .osc (2, metal, 20, 30).pitch (2, 12).levelEnv (2, 0, 0, 150, 0)
            .osc (3, fm, 40, 36).levelEnv (3, 0, 0, 300, 0)
            .osc (4, pulse, 60, 36).pitch (4, 24).levelEnv (4, 0, 0, 12, 0)
            .filter (1, ladder12, 2800, 25, 10, 50)
            .env (2, 0, 200, 0, 120, 70).mod (env2, "filter1Cutoff", 30)
            .lfoSync (1, triangle, bars2, 0).mod (lfo1, "osc3Position", 40)
            .mod (velocity, "osc4Volume", 30).mod (velocity, "filter1Cutoff", 20)
            .mod (velocity, "osc1Position", 40)
            .amp (1, 400, 0, 300).drive (hard, 0, 8000).delay (sixteenthD, 35, 20, 3500).level (3.8)
            .macro (1, "Echo").mod (macro1, "fxDelayMix", 40).macro (2, "Copper").mod (macro2, "osc2Volume", 30)
            .macro (3, "Tick").mod (macro3, "osc4Volume", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        //======================================================================
        // Arp

        // Nine. Body 1, a saw, and 2, a square an octave up; 3 the third harmonic, dying; 4 a sine an
        // octave up FM'd by 5 (unheard); 6 a vowel; strike 7; width 8; weight 9. A step pattern under
        // the arpeggio moves the vowel's mouth and the FM, so each note has its own colour.
        add ("Prism Gear", "Arp").osc (1, analog, sawPos, 42)
            .osc (2, analog, squarePos, 30).pitch (2, 12)
            .osc (3, analog, sinePos, 18).pitch (3, 19).levelEnv (3, 0, 0, 150, 20)
            .osc (4, analog, sinePos, 30).pitch (4, 12).fm (4, 5, 25).modulator (5, analog, sinePos).pitch (5, 24)
            .osc (6, vowel, 30, 24)
            .osc (7, metal, 60, 26).pitch (7, 24).levelEnv (7, 0, 0, 15, 0)
            .osc (8, analog, sawPos, 14).pitch (8, 12).unison (8, 3, 12, 100)
            .osc (9, analog, trianglePos, 26).pitch (9, -12)
            .filter (1, ladder24, 2200, 35, 0, 40)
            .env (2, 0, 160, 0, 100, 70).mod (env2, "filter1Cutoff", 30)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { -100, 40, 100, -20, 60, -60, 20, 100, -100, 60, -20, 80, 0, -80, 100, -40 })
            .mod (lfo1, "osc6Position", 50).mod (lfo1, "osc4WarpAmount", 20)
            .lfoSync (2, sine, bars4, 0).mod (lfo2, "filter1Cutoff", 15)
            .mod (velocity, "osc7Volume", 30).mod (velocity, "filter1Cutoff", 20)
            .amp (1, 220, 20, 150).delay (eighthD, 35, 20, 4000).level (4.3)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Facet").mod (macro2, "osc6Volume", 30)
            .macro (3, "FM").mod (macro3, "osc4WarpAmount", 35).macro (4, "Space").mod (macro4, "fxDelayMix", 30);

        // Five. Body 1, a squeezed pulse an LFO works; 2 an FM table; bite 3, a sync an octave up the
        // envelope sweeps; air 4, dying; weight 5.
        add ("Scatter Lamp", "Arp").osc (1, pulse, 50, 48).warp (1, wSqueeze, 30)
            .osc (2, fm, 70, 36)
            .osc (3, sync, 30, 30).pitch (3, 12).warp (3, wSync, 10)
            .osc (4, additive, 85, 14).pitch (4, 24).levelEnv (4, 0, 0, 120, 0)
            .osc (5, analog, sinePos, 30).pitch (5, -12)
            .filter (1, ladder24, 2600, 30, 0, 40)
            .env (2, 0, 140, 0, 90, 70).mod (env2, "filter1Cutoff", 25).mod (env2, "osc3WarpAmount", 40)
            .lfoSync (1, triangle, quarterT, 0).mod (lfo1, "osc1WarpAmount", 30)
            .mod (velocity, "osc3WarpAmount", 30).mod (velocity, "filter1Cutoff", 20)
            .amp (1, 180, 0, 120).delay (sixteenthD, 40, 22, 4500).level (4.7)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Squeeze").mod (macro2, "osc1WarpAmount", 40)
            .macro (3, "Sync").mod (macro3, "osc3WarpAmount", 40).macro (4, "Space").mod (macro4, "fxDelayMix", 30);

        //======================================================================
        // Keys

        // Nine, a broken electric piano. Body 1, a sine FM'd by 2 (unheard, at its pitch), brighter for
        // harder notes; tine 3 two octaves up FM'd by 4 (unheard, forty cents sharp); 5 a bark, a square
        // FM'd by 9's metal (unheard), which only a hard note brings; strike 6, the hammer; width 7, an
        // octave the tremolo throws side to side on the eighths; grit 8, crushed, stuttering on the
        // sixteenths as the note is held. All through a tube drive that breaks up harder over two bars.
        add ("Fused Tine", "Keys").osc (1, analog, sinePos, 45).fm (1, 2, 12).modulator (2, analog, sinePos)
            .osc (3, analog, sinePos, 32).pitch (3, 24).fm (3, 4, 30).levelEnv (3, 0, 0, 350, 10).modulator (4, analog, sinePos).pitch (4, 24, 40)
            .osc (5, analog, squarePos, 5).pitch (5, 12).fm (5, 9, 15).levelEnv (5, 0, 0, 400, 20).modulator (9, metal, 30).pitch (9, 12)
            .osc (6, pulse, 80, 20).pitch (6, 24).levelEnv (6, 0, 0, 8, 0)
            .osc (7, analog, sinePos, 18).pitch (7, 12).unison (7, 3, 10, 100)
            .osc (8, growl, 50, 12).warp (8, wCrush, 60)
            .filter (1, ladder12, 4000, 10, 0, 30)
            .env (2, 0, 600, 20, 400).mod (env2, "osc1WarpAmount", 25).mod (velocity, "osc1WarpAmount", 20).mod (velocity, "osc5Volume", 35)
            .lfoSync (1, square, eighth, 0, 40).mod (lfo1, "ampPan", 30).mod (lfo1, "osc7Volume", 15)
            .lfoSync (2, steps, bar1, 0).stepPattern (2, { 100, -100, 40, -100, 100, -60, -100, 80, 100, -100, 20, -100, 60, 100, -100, -40 })
            .mod (lfo2, "osc8Volume", 25).mod (lfo2, "osc3WarpAmount", 15)
            .amp (1, 1800, 30, 400, 70).drive (tube, 45, 5000, 80).chorus (25).reverb (15, 50, 1.5f).level (-10.1)
            .macro (1, "Tine").mod (macro1, "osc3Volume", 30).macro (2, "Bark").mod (macro2, "osc5Volume", 30)
            .macro (3, "Tremolo").mod (macro3, "osc7Volume", 30).macro (4, "Break").mod (macro4, "fxDriveAmount", 50);

        // Four. Body 1, an FM table whose position a dotted-eighth LFO rocks, so a held chord glints in
        // a cross-rhythm; 2 additive an octave up, dying slowly; 3 a vowel that melts open over two bars;
        // strike 4, harder and brighter for a hard note. Fuzz half in, through a phaser.
        add ("Glass Smelter", "Keys").osc (1, fm, 30, 40)
            .osc (2, additive, 40, 25).pitch (2, 12).levelEnv (2, 0, 0, 900, 20)
            .osc (3, vowel, 60, 20)
            .osc (4, pulse, 70, 20).pitch (4, 24).levelEnv (4, 0, 0, 12, 0)
            .filter (1, ladder12, 3500, 15, 0, 30)
            .env (2, 0, 700, 10, 400).mod (env2, "filter1Cutoff", 25).mod (env2, "osc1Position", 30)
            .lfoSync (1, triangle, eighthD, 0).mod (lfo1, "osc1Position", 35).mod (lfo1, "osc2Volume", 15)
            .lfoSync (2, triangle, bars2, 0).mod (lfo2, "osc3Position", 50).mod (lfo2, "osc3Volume", 15)
            .mod (velocity, "osc4Volume", 50).mod (velocity, "filter1Cutoff", 35).mod (velocity, "osc1Position", 30)
            .amp (1, 1500, 30, 400, 70).drive (fuzz, 30, 4500, 60).phaser (40, 0.25f, 50).level (-8.4)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", -40).macro (2, "Glass").mod (macro2, "osc2Volume", 30)
            .macro (3, "Vowel").mod (macro3, "osc3Position", 40).macro (4, "Melt").mod (macro4, "fxDriveAmount", 50);

        //======================================================================
        // Organ

        // Nine drawbars, each an oscillator: 1 the sixteen-foot, 2 the eight (FM'd by 9's saw, unheard,
        // a buzz), 3 the fifth, 4 the four-foot, 5 the twelfth, 6 the two-foot; strike 7, the key click;
        // 8 a percussion twelfth that dies. A fast LFO throws the four- and two-foot side to side, and
        // the stab goes through hard drive.
        add ("Rave Liturgy", "Organ").osc (1, analog, sinePos, 40).pitch (1, -12)
            .osc (2, analog, sinePos, 50).fm (2, 9, 15).modulator (9, analog, sawPos)
            .osc (3, analog, sinePos, 30).pitch (3, 7)
            .osc (4, analog, sinePos, 40).pitch (4, 12)
            .osc (5, analog, sinePos, 25).pitch (5, 19)
            .osc (6, analog, sinePos, 30).pitch (6, 24)
            .osc (7, pulse, 90, 30).pitch (7, 24).levelEnv (7, 0, 0, 6, 0)
            .osc (8, analog, sinePos, 30).pitch (8, 19).levelEnv (8, 0, 0, 200, 0)
            .filter (1, ladder12, 7000, 10, 0, 20)
            .lfo (1, sine, 6.5f, 0).mod (lfo1, "osc4Pan", 40).mod (lfo1, "osc6Pan", -40).mod (lfo1, "osc5Fine", 4)
            .mod (velocity, "osc7Volume", 30).mod (velocity, "filter1Cutoff", 20)
            .amp (1, 500, 40, 150).drive (hard, 50, 7000).chorus (30, 4.0f, 40).reverb (15, 50, 1.5f).level (-15.6)
            .macro (1, "Low").mod (macro1, "osc1Volume", 50).macro (2, "Perc").mod (macro2, "osc8Volume", 40)
            .macro (3, "Buzz").mod (macro3, "osc2WarpAmount", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        // Five, an organ in a furnace. 1 a folded square, the fold breathing on the half bar; 2 the octave;
        // 3 a pulse a twelfth up, chopped by a step pattern so the held chord pulses on the sixteenths;
        // 4 a choir an octave down whose mouth turns over four bars; strike 5, the key click, louder for a
        // hard note, which also folds the square harder.
        add ("Furnace Hymn", "Organ").osc (1, analog, squarePos, 40).warp (1, wFold, 10)
            .osc (2, analog, squarePos, 25).pitch (2, 12)
            .osc (3, pulse, 30, 18).pitch (3, 19)
            .osc (4, vowel, 50, 25).pitch (4, -12)
            .osc (5, analog, sinePos, 15).pitch (5, 24).levelEnv (5, 0, 0, 8, 0)
            .filter (1, ladder12, 3500, 15, 0, 20)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { 100, -100, 100, 40, -100, 100, -100, 60, 100, -100, 100, 20, -100, 100, 80, -100 })
            .mod (lfo1, "osc3Volume", 40).mod (lfo1, "osc1WarpAmount", 10)
            .lfoSync (2, triangle, bars4, 0).mod (lfo2, "osc4Position", 50).mod (lfo2, "osc1WarpAmount", 25)
            .mod (velocity, "osc5Volume", 30).mod (velocity, "osc1WarpAmount", 20)
            .amp (5, 300, 90, 300).drive (foldDrive, 30, 5000).reverb (35, 90, 4.0f).level (-13.5)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", 35).macro (2, "Fold").mod (macro2, "osc1WarpAmount", 40)
            .macro (3, "Choir").mod (macro3, "osc4Volume", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        //======================================================================
        // Guitar

        // Nine, a power chord of plucked strings on one key: 1 and 2 the root, eight cents apart; 3 and 4
        // the fifth, seven apart; 5 the octave; 6 a growl under them, FM'd by 8 (unheard), the pickup's
        // hum, chugging on the sixteenths as the chord rings (a step pattern on its level and FM), as if
        // palm-muted; strike 7, the pick, harder for a hard note; width 9, a saw octave that swells and
        // dies. Fuzz, a scooped middle whose notch sweeps over two bars; a hard note opens the filter.
        add ("Tendon Riff", "Guitar").osc (1, string, 55, 40)
            .osc (2, string, 60, 35).pitch (2, 0, 8)
            .osc (3, string, 55, 35).pitch (3, 7)
            .osc (4, string, 60, 30).pitch (4, 7, -7)
            .osc (5, string, 50, 28).pitch (5, 12)
            .osc (6, growl, 40, 30).fm (6, 8, 15).modulator (8, analog, sinePos).pitch (8, 12)
            .osc (7, fm, 95, 25).pitch (7, 24).levelEnv (7, 0, 0, 8, 0)
            .osc (9, analog, sawPos, 10).pitch (9, 12).unison (9, 3, 18, 100).levelEnv (9, 20, 0, 300, 50)
            .filter (1, ladder12, 3200, 15, 0, 20).filter (2, svfNotch, 900, 20)
            .env (2, 0, 200, 0, 100, 70).mod (env2, "filter1Cutoff", 20)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { 100, -100, 100, 100, -100, 100, -100, 100, 100, -100, 100, 100, -100, 100, 100, -100 })
            .mod (lfo1, "osc6Volume", 40).mod (lfo1, "osc6WarpAmount", 20)
            .lfoSync (2, triangle, bars2, 0).mod (lfo2, "filter2Cutoff", 30)
            .mod (velocity, "osc7Volume", 35).mod (velocity, "filter1Cutoff", 20)
            .amp (1, 1200, 60, 120).drive (fuzz, 55, 4000).level (-22.5)
            .macro (1, "Mute").mod (macro1, "filter1Cutoff", -30).macro (2, "Hum").mod (macro2, "osc6Volume", 30)
            .macro (3, "Chug").mod (macro3, "osc6WarpAmount", 40).macro (4, "Fuzz").mod (macro4, "fxDriveAmount", 40);

        // Four, a bowed slide. Body 1, a plucked string, and 2, a saw that rises under it as the pluck
        // fades; 3 a growl an octave up; 4 metal a twelfth up. Glides, fuzz, a quarter-note echo.
        add ("Wire Sermon", "Guitar").osc (1, string, 70, 40)
            .osc (2, analog, sawPos, 25).levelEnv (2, 200, 0, 0, 100)
            .osc (3, growl, 60, 18).pitch (3, 12)
            .osc (4, metal, 20, 10).pitch (4, 19)
            .filter (1, ladder12, 3000, 20, 0, 30)
            .lfo (1, sine, 5.0f, 1).lfoFade (1, 700).mod (lfo1, "osc2Fine", 8)
            .mod (velocity, "filter1Cutoff", 20)
            .amp (5, 800, 70, 400).mono (120).drive (fuzz, 50, 4000).delay (quarter, 35, 20, 3500).reverb (20, 70, 2.5f).level (-12.1)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", 35).macro (2, "Bow").mod (macro2, "osc2Volume", 30)
            .macro (3, "Howl").mod (macro3, "osc3Volume", 30).macro (4, "Fuzz").mod (macro4, "fxDriveAmount", 40);

        //======================================================================
        // Brass

        // Nine. Body 1, four saws, and 2, four pulses whose width an LFO turns, both rising into each
        // note from two semitones under; 3 three saws an octave down; 4 a square an octave up FM'd by 7
        // (unheard); weight 5; bite 6, a sync; 8 a vowel's growl; strike 9. Always glides.
        add ("Siren Carcass", "Brass").osc (1, analog, sawPos, 30).unison (1, 4, 30, 80)
            .osc (2, pulse, 50, 25).unison (2, 4, 30, 80)
            .osc (3, analog, sawPos, 25).pitch (3, -12).unison (3, 3, 20, 0)
            .osc (4, analog, squarePos, 12).pitch (4, 12).fm (4, 7, 25).modulator (7, analog, sinePos)
            .osc (5, analog, sinePos, 30).pitch (5, -12)
            .osc (6, sync, 40, 15).pitch (6, 12).warp (6, wSync, 30).levelEnv (6, 0, 0, 200, 30)
            .osc (8, vowel, 40, 12).pitch (8, 12)
            .osc (9, metal, 50, 15).pitch (9, 12).levelEnv (9, 0, 0, 25, 0)
            .filter (1, ladder12, 3500, 15, 0, 30)
            .env (3, 0, 220, 0, 100, 60).mod (env3, "osc1Pitch", -4.17f).mod (env3, "osc2Pitch", -4.17f).mod (env3, "osc3Pitch", -4.17f)
            .lfo (1, sine, 0.9f, 0).mod (lfo1, "osc2Position", 30)
            .mod (velocity, "osc6WarpAmount", 30).mod (velocity, "osc9Volume", 30)
            .mod (velocity, "osc5Position", 40)
            .amp (5, 500, 90, 300).mono (90).glideAlways().drive (tube, 25, 7000).reverb (15, 60, 2.0f).level (0.2)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Hoover").mod (macro2, "osc2Volume", 30)
            .macro (3, "Throat").mod (macro3, "osc8Volume", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        // Five, FM brass. 1 and 2, sines an octave apart, both FM'd by 4 (unheard) as much as the
        // envelope has risen and as hard as the note is played, so a soft note is a mellow horn and a
        // hard one blares; the FM flutters on the triplet eighths as a held note is tongued; weight 3, a
        // saw an octave down; 5 a breath at the start. A swell over two bars opens the filter.
        add ("Iron Fanfare", "Brass").osc (1, analog, sinePos, 45).fm (1, 4, 0)
            .osc (2, analog, sinePos, 30).pitch (2, 12).fm (2, 4, 0)
            .osc (3, analog, sawPos, 25).pitch (3, -12)
            .modulator (4, analog, sinePos)
            .osc (5, vowel, 80, 15).pitch (5, 24).levelEnv (5, 0, 0, 60, 0)
            .filter (1, ladder12, 2500, 15, 0, 30)
            .env (2, 60, 300, 60, 200).mod (env2, "osc1WarpAmount", 40).mod (env2, "osc2WarpAmount", 30)
            .mod (velocity, "osc1WarpAmount", 25).mod (velocity, "osc2WarpAmount", 20)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { 100, -60, 20, 100, -60, 20, 100, -60, 20, 100, -60, 20, 100, -100, 100, -100 })
            .mod (lfo1, "osc1WarpAmount", 15).mod (lfo1, "osc2WarpAmount", 15).mod (lfo1, "ampLevel", 6)
            .lfoSync (2, triangle, bars2, 0).mod (lfo2, "filter1Cutoff", 20).mod (lfo2, "osc3Volume", 20)
            .amp (40, 400, 85, 250).reverb (20, 70, 2.5f).level (-1.2)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Blare").mod (macro2, "osc1WarpAmount", 30)
            .macro (3, "Growl").mod (macro3, "osc3Volume", 50).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        //======================================================================
        // Strings

        // Nine, a detuned wall. Body 1 (four saws) and 2 (four an octave up); 3 three pulses; 4 a plucked
        // string, the bow's bite at each note; 5 a vowel, FM'd by 8 (unheard, five cents off an octave
        // up) and turned by a slow LFO; weight 6; air 7 past the filter; grit 9.
        add ("Riot Section", "Strings").osc (1, analog, sawPos, 25).unison (1, 4, 20, 100)
            .osc (2, analog, sawPos, 20).pitch (2, 12).unison (2, 4, 25, 100)
            .osc (3, pulse, 40, 18).unison (3, 3, 18, 80)
            .osc (4, string, 70, 18)
            .osc (5, vowel, 30, 14).fm (5, 8, 15).modulator (8, analog, sinePos).pitch (8, 12, 5)
            .osc (6, analog, sinePos, 18).pitch (6, -12)
            .osc (7, additive, 80, 6).pitch (7, 24).route (7, 100)
            .osc (9, metal, 50, 5).pitch (9, 12).warp (9, wCrush, 40)
            .filter (1, ladder12, 2800, 15, 0, 30)
            .env (2, 400, 1500, 60, 900).mod (env2, "filter1Cutoff", 20)
            .lfo (1, sine, 0.18f, 0).mod (lfo1, "osc1UnisonDetune", 15).mod (lfo1, "osc5Position", 30)
            .mod (velocity, "filter1Cutoff", 20)
            .amp (350, 800, 90, 900).chorus (30).reverb (35, 80, 3.0f).level (5.9)
            .macro (1, "Dark").mod (macro1, "filter1Cutoff", -40).macro (2, "Vowel").mod (macro2, "osc5Volume", 50)
            .macro (3, "Wall").mod (macro3, "osc2Volume", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        // Four, tremolo strings for a chase. Body 1 (three saws) and 2 (two an octave up), cut in triplet
        // sixteenths; 3 a vowel; 4 a plucked string under each note.
        add ("Taut Mire", "Strings").osc (1, analog, sawPos, 30).unison (1, 3, 15, 90)
            .osc (2, analog, sawPos, 20).pitch (2, 12).unison (2, 2, 12, 100)
            .osc (3, vowel, 50, 18)
            .osc (4, string, 60, 18)
            .filter (1, ladder12, 3000, 20, 0, 30)
            .lfoSync (1, square, sixteenthT, 0, 30).mod (lfo1, "osc1Volume", -30).mod (lfo1, "osc2Volume", -20)
            .mod (velocity, "filter1Cutoff", 20)
            .amp (60, 600, 90, 500).reverb (25, 70, 2.5f).level (-5.7)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Tremolo").mod (macro2, "osc1Volume", -30)
            .macro (3, "Vowel").mod (macro3, "osc3Volume", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        //======================================================================
        // Pad

        // Nine, a pad that corrodes over a held chord: body 1 (three saws) crushed more and more as a
        // six-second envelope rises; 2 a vowel that comes and goes; 3 additive; weight 4; 5 metal
        // arriving over three seconds; 6 a twelfth FM'd by 7's metal (unheard) as the envelope rises;
        // width 8; grit 9, folded, arriving last.
        add ("Corroding Halo", "Pad").osc (1, analog, sawPos, 25).unison (1, 3, 18, 90).warp (1, wCrush, 0)
            .osc (2, vowel, 30, 18).pitch (2, 12).levelEnv (2, 1500, 0, 4000, 40)
            .osc (3, additive, 60, 20)
            .osc (4, analog, sinePos, 20).pitch (4, -12)
            .osc (5, metal, 40, 10).pitch (5, 12).levelEnv (5, 3000, 0, 0, 100)
            .osc (6, analog, sinePos, 10).pitch (6, 19).fm (6, 7, 0).modulator (7, metal, 70).pitch (7, 12, 33)
            .osc (8, pulse, 50, 14).pitch (8, 12).unison (8, 3, 22, 100)
            .osc (9, growl, 50, 5).pitch (9, 24).warp (9, wFold, 40).levelEnv (9, 4000, 0, 0, 100)
            .filter (1, ladder12, 2000, 20, 0, 20)
            .env (3, 6000, 0, 100, 2000).mod (env3, "osc1WarpAmount", 35).mod (env3, "osc6WarpAmount", 30)
            .lfoSync (1, triangle, bars4, 0).mod (lfo1, "filter1Cutoff", 15).mod (lfo1, "osc2Position", 30)
            .lfo (2, drift, 0.1f, 0).mod (lfo2, "osc3Position", 30)
            .mod (velocity, "filter1Cutoff", 20)
            .amp (900, 1000, 100, 2500).reverb (22, 85, 5.0f).level (3.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Corrode").mod (macro2, "osc1WarpAmount", 80)
            .macro (3, "Metal").mod (macro3, "osc5Volume", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        // Five. Body 1 (three vowels) and 2 (two an octave up) whose mouths an eight-bar LFO turns opposite
        // ways; 3 a sine FM'd by 5 (unheard) a fifth above; air 4 additive a twelfth up. A phaser.
        add ("Ozone Nave", "Pad").osc (1, vowel, 20, 30).unison (1, 3, 15, 90)
            .osc (2, vowel, 80, 20).pitch (2, 12).unison (2, 2, 12, 100)
            .osc (3, analog, sinePos, 25).fm (3, 5, 15).modulator (5, analog, sinePos).pitch (5, 7)
            .osc (4, additive, 30, 10).pitch (4, 19)
            .filter (1, ladder12, 3000, 15, 0, 20)
            .lfoSync (1, sine, bars8, 0).mod (lfo1, "osc1Position", 40).mod (lfo1, "osc2Position", -40)
            .mod (velocity, "filter1Cutoff", 20)
            .amp (1200, 1000, 100, 3000).phaser (30, 0.15f, 40).reverb (30, 85, 5.0f).level (4)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Vowel").mod (macro2, "osc1Position", 30)
            .macro (3, "Glow").mod (macro3, "osc3WarpAmount", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        //======================================================================
        // Drone

        // Nine, a machine at prayer, its layers spread out so each is heard. Weight 1; 2 a saw FM'd by 3
        // (unheard) an octave down, the FM rising and falling over four bars, so the drone growls and
        // clears; 4 a vowel chanting on the eighths (a step pattern on its mouth); 5 metal a fifth up FM'd
        // by 6 (unheard), answering the vowel on the off-beats; 7 a growl that arrives in the third bar;
        // 8 air shimmering on the sixteenths; 9 a crushed pulse the mod wheel brings. The filter opens
        // over four bars with the FM.
        add ("Turbine Monastery", "Drone").osc (1, analog, sinePos, 38)
            .osc (2, analog, sawPos, 28).fm (2, 3, 15).modulator (3, analog, sinePos).pitch (3, -12)
            .osc (4, vowel, 40, 28).pitch (4, 12)
            .osc (5, metal, 30, 20).pitch (5, 19).fm (5, 6, 25).modulator (6, analog, sinePos).pitch (6, 26, -13)
            .osc (7, growl, 50, 24).pitch (7, -12).levelEnv (7, 3600, 0, 0, 100)
            .osc (8, additive, 70, 14).pitch (8, 24).unison (8, 3, 25, 100)
            .osc (9, pulse, 50, 0).pitch (9, 12).warp (9, wCrush, 70)
            .filter (1, svfLow, 2400, 25, 0, 20)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { 100, -100, 60, -100, 100, -100, 20, -100, 100, -100, 80, -100, 100, -60, 40, -100 })
            .mod (lfo1, "osc4Position", 45).mod (lfo1, "osc5Volume", -45).mod (lfo1, "osc8Volume", 25)
            .lfoSync (2, triangle, bars4, 0).mod (lfo2, "osc2WarpAmount", 40).mod (lfo2, "filter1Cutoff", 25).mod (lfo2, "osc5WarpAmount", 25)
            .mod (velocity, "osc2WarpAmount", 15).mod (modWheel, "osc9Volume", 40)
            .amp (300, 1000, 100, 3000).reverb (14, 85, 5.0f).level (0.3)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Turbine").mod (macro2, "osc2WarpAmount", 35)
            .macro (3, "Chant").mod (macro3, "osc4Volume", 35).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        // Four, mains hum under a pylon. Body 1, a square; 2 a pulse an octave up three cents off; 3 a
        // growl a twelfth up that arcs on and off on the sixteenths (a step pattern), the pylon
        // crackling; weight 4. A comb tuned to the note, swept over four bars, so the hum's colour turns;
        // a hard note arcs louder and pushes the comb.
        add ("Pylon Hum", "Drone").osc (1, analog, squarePos, 35)
            .osc (2, pulse, 80, 20).pitch (2, 12, 3)
            .osc (3, growl, 60, 10).pitch (3, 19)
            .osc (4, analog, sinePos, 30).pitch (4, -12)
            .filter (1, ladder12, 1400, 20, 0, 20).filter (2, comb, 110, 50, 0, 100)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { 100, -100, -100, 60, -100, 100, -100, -100, 100, 40, -100, -100, 100, -100, 80, -100 })
            .mod (lfo1, "osc3Volume", 30)
            .lfoSync (2, triangle, bars4, 0).mod (lfo2, "filter2Cutoff", 35).mod (lfo2, "filter1Cutoff", 15)
            .mod (velocity, "osc3Volume", 20).mod (velocity, "filter2Resonance", 20)
            .amp (1500, 1000, 100, 3000).reverb (25, 80, 4.0f).level (-1.1)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Comb").mod (macro2, "filter2Resonance", 30)
            .macro (3, "Buzz").mod (macro3, "osc3Volume", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 50);

        //======================================================================
        // Bell

        // Nine, a cast bell partial by partial: 1 the hum an octave down, 2 the prime, 3 the minor third
        // (tierce), 4 the fifth, 5 the nominal an octave up (struck harder by 9, unheard, at an odd
        // interval), 6 two and a half times, 7 the twelfth, 8 two octaves and a bit; each dies at its own
        // rate, the higher sooner, and the inner ones sit apart left and right.
        add ("Foundry Carillon", "Bell").osc (1, analog, sinePos, 25).pitch (1, -12).levelEnv (1, 0, 0, 4000, 0)
            .osc (2, analog, sinePos, 35).levelEnv (2, 0, 0, 3000, 0)
            .osc (3, analog, sinePos, 22).pitch (3, 3, 16).levelEnv (3, 0, 0, 2200, 0).pan (3, -30)
            .osc (4, analog, sinePos, 15).pitch (4, 7, 2).levelEnv (4, 0, 0, 1600, 0).pan (4, 30)
            .osc (5, analog, sinePos, 30).pitch (5, 12).fm (5, 9, 0).levelEnv (5, 0, 0, 2500, 0).modulator (9, analog, sinePos).pitch (9, 17)
            .osc (6, analog, sinePos, 24).pitch (6, 15, 86).levelEnv (6, 0, 0, 500, 0).pan (6, -40)
            .osc (7, analog, sinePos, 12).pitch (7, 19, 2).levelEnv (7, 0, 0, 350, 0).pan (7, 40)
            .osc (8, analog, sinePos, 20).pitch (8, 24, 80).levelEnv (8, 0, 0, 200, 0)
            .filter (1, ladder12, 9000, 0, 0, 0)
            .env (2, 0, 150, 0, 40, 70).mod (env2, "osc5WarpAmount", 55)
            .mod (velocity, "filter1Cutoff", 20)
            .mod (velocity, "osc2Position", 40)
            .amp (1, 4000, 0, 3000).reverb (30, 80, 3.0f).level (5.1)
            .macro (1, "Strike").mod (macro1, "osc5WarpAmount", 30).macro (2, "Hum").mod (macro2, "osc1Volume", 30)
            .macro (3, "Tierce").mod (macro3, "osc3Volume", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        // Five. Body 1, metal; 2 a sine an octave up FM'd by 4 (unheard) at an odd interval; air 3; weight 5.
        add ("Bitter Chime", "Bell").osc (1, metal, 50, 35)
            .osc (2, analog, sinePos, 30).pitch (2, 12).fm (2, 4, 35).modulator (4, analog, sinePos).pitch (4, 16)
            .osc (3, additive, 70, 15).pitch (3, 24).levelEnv (3, 0, 0, 600, 0)
            .osc (5, analog, sinePos, 20)
            .filter (1, ladder12, 7000, 10, 0, 20)
            .env (2, 0, 1500, 0, 800, 70).mod (env2, "osc2WarpAmount", 20)
            .mod (velocity, "filter1Cutoff", 20)
            .mod (velocity, "osc1Position", 40)
            .amp (1, 2500, 0, 2000).delay (quarterD, 30, 20, 5000).reverb (25, 70, 2.5f).level (5.3)
            .macro (1, "Dark").mod (macro1, "filter1Cutoff", -40).macro (2, "Bitter").mod (macro2, "osc2WarpAmount", 30)
            .macro (3, "Air").mod (macro3, "osc3Volume", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        //======================================================================
        // Sequence

        // Nine. Body 1, a saw; weight 2; 3 a sine an octave up FM'd by 4 (unheard); 5 metal; 6 a vowel;
        // bite 7, a sync; strike 8; width 9. Two step patterns, one a sixteenth a step and one an eighth,
        // move the metal, the mouth, the FM and the sync, so the line changes under its own repeats.
        add ("Conveyor Hex", "Sequence").osc (1, analog, sawPos, 52)
            .osc (2, pulse, 30, 45).pitch (2, -12)
            .osc (3, analog, sinePos, 37).pitch (3, 12).fm (3, 4, 20).modulator (4, analog, sinePos).pitch (4, 19)
            .osc (5, metal, 40, 27).pitch (5, 12)
            .osc (6, vowel, 60, 30)
            .osc (7, sync, 30, 27).pitch (7, 12).warp (7, wSync, 30)
            .osc (8, fm, 90, 30).pitch (8, 24).levelEnv (8, 0, 0, 12, 0)
            .osc (9, analog, sawPos, 18).pitch (9, 12).unison (9, 3, 12, 100)
            .filter (1, ladder24, 1600, 40, 0, 40)
            .env (2, 0, 140, 0, 80, 70).mod (env2, "filter1Cutoff", 30)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { 100, -100, 0, 60, -60, 100, -100, 20, 100, -40, -100, 60, 0, 100, -80, -20 })
            .mod (lfo1, "osc5Volume", 30).mod (lfo1, "osc6Position", 40).mod (lfo1, "osc3WarpAmount", 20)
            .lfoSync (2, steps, bars2, 0).stepPattern (2, { -100, 0, 100, 40, -40, 80, -80, 0, 60, -100, 100, -20, 20, -60, 40, 100 })
            .mod (lfo2, "osc7WarpAmount", 30).mod (lfo2, "filter1Cutoff", 15)
            .mod (velocity, "osc7WarpAmount", 30).mod (velocity, "osc8Volume", 10)
            .mod (velocity, "osc1Position", 40)
            .amp (1, 150, 0, 80).drive (hard, 0, 8000).delay (sixteenthD, 30, 15, 4000).level (0.4)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Metal").mod (macro2, "osc5Volume", 30)
            .macro (3, "Res").mod (macro3, "filter1Resonance", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        // Five. Body 1, a square; 2 an FM table an octave up; weight 3, a growl; strike 4; 5 additive an
        // octave up a dotted-quarter LFO turns.
        add ("Gauge Pulse", "Sequence").osc (1, analog, squarePos, 40)
            .osc (2, fm, 30, 55).pitch (2, 12)
            .osc (3, growl, 30, 15).pitch (3, -12)
            .osc (4, metal, 70, 50).pitch (4, 24).levelEnv (4, 0, 0, 60, 0).route (4, 100)
            .osc (5, additive, 70, 28).pitch (5, 12).route (5, 100)
            .filter (1, ladder24, 3500, 45, 10, 40)
            .env (2, 0, 120, 0, 60, 70).mod (env2, "filter1Cutoff", 70)
            .lfoSync (1, triangle, quarterD, 0).mod (lfo1, "osc5Position", 40).mod (lfo1, "osc2Position", 25)
            .mod (velocity, "filter1Cutoff", 20)
            .mod (velocity, "osc2Position", 40)
            .amp (1, 140, 0, 70).drive (hard, 20, 6000).level (-2.8)
            .macro (1, "Dark").mod (macro1, "filter1Cutoff", -40).macro (2, "Gauge").mod (macro2, "osc2Volume", 30)
            .macro (3, "Air").mod (macro3, "osc5Volume", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        //======================================================================
        // Riser

        // Nine, entering one at a time over four bars: body 1 (three saws) and weight 6 from the start;
        // 2 a pulse after a second and a half; bite 3, a sync, after three; 7 a sine FM'd by 8 (unheard)
        // harder as it rises; air 4 after four and a half; width 9 after five; metal 5 after six. A
        // seven-second envelope lifts the pitch six semitones, the filter, the noise and an LFO's speed.
        add ("Ascent Boiler", "Riser").osc (1, analog, sawPos, 30).unison (1, 3, 20, 80)
            .osc (2, pulse, 50, 22).pitch (2, 12).levelEnv (2, 1500, 10000, 0, 100)
            .osc (3, sync, 30, 20).pitch (3, 12).warp (3, wSync, 0).levelEnv (3, 3000, 10000, 0, 100)
            .osc (4, additive, 80, 12).pitch (4, 24).levelEnv (4, 4500, 10000, 0, 100)
            .osc (5, metal, 50, 10).pitch (5, 19).levelEnv (5, 6000, 10000, 0, 100)
            .osc (6, analog, sinePos, 25).pitch (6, -12)
            .osc (7, analog, sinePos, 15).pitch (7, 12).fm (7, 8, 0).modulator (8, analog, sinePos).pitch (8, 24)
            .osc (9, analog, sawPos, 8).pitch (9, 24).unison (9, 3, 30, 100).levelEnv (9, 5000, 10000, 0, 100)
            .noise (10, 0)
            .filter (1, ladder24, 400, 40, 10, 20)
            .env (3, 7000, 0, 100, 1500, 50).mod (env3, "osc1Pitch", 12.5f).mod (env3, "osc2Pitch", 12.5f).mod (env3, "filter1Cutoff", 45)
            .mod (env3, "noiseLevel", 30).mod (env3, "lfo1Rate", 40).mod (env3, "osc3WarpAmount", 50).mod (env3, "osc7WarpAmount", 40)
            .lfo (1, sine, 2.0f, 1).mod (lfo1, "filter1Cutoff", 12)
            .mod (velocity, "osc3WarpAmount", 30).mod (velocity, "filter1Cutoff", 20)
            .amp (50, 100, 100, 1500).reverb (30, 80, 3.0f).level (3.6)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30).macro (2, "Steam").mod (macro2, "noiseLevel", 30)
            .macro (3, "Weight").mod (macro3, "osc6Volume", 50).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        // Five. Body 1, a growl whose position climbs; 2 a vowel that opens; 3 a sine FM'd by 5 (unheard)
        // harder and harder; 4 metal arriving at four seconds. Pitch climbs four semitones; a phaser.
        add ("Lift Shaft", "Riser").osc (1, growl, 10, 44)
            .osc (2, vowel, 0, 25).pitch (2, 12)
            .osc (3, analog, sinePos, 19).pitch (3, 12).fm (3, 5, 0).modulator (5, analog, sinePos).pitch (5, 7)
            .osc (4, metal, 40, 10).pitch (4, 24).levelEnv (4, 4000, 10000, 0, 100)
            .filter (1, ladder24, 600, 30, 0, 20)
            .env (3, 7000, 0, 100, 1500, 50).mod (env3, "osc1Position", 70).mod (env3, "osc2Position", 80).mod (env3, "osc3WarpAmount", 50)
            .mod (env3, "osc1Pitch", 8.33f).mod (env3, "osc3Pitch", 8.33f).mod (env3, "filter1Cutoff", 40)
            .mod (velocity, "filter1Cutoff", 20)
            .amp (50, 100, 100, 1500).phaser (40, 0.4f, 60).reverb (30, 80, 3.0f).level (4.4)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 30).macro (2, "Vowel").mod (macro2, "osc2Volume", 30)
            .macro (3, "FM").mod (macro3, "osc3WarpAmount", 50).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        //======================================================================
        // Impact

        // Nine. Weight 1, falling an octave; 2 a sub tail an octave down; 3 metal FM'd by 9's metal
        // (unheard), groaning for seconds; strike 4; 5 a clang, a sine a fifth up FM'd by 6 (unheard) at
        // an odd interval; grit 7, a folded growl; width 8, four saws two octaves up, dying; and a burst of
        // noise. Dry, so the boom stays mono; the Space macro opens a hall.
        add ("Keel Rupture", "Impact").osc (1, analog, sinePos, 70)
            .osc (2, analog, sinePos, 40).pitch (2, -12).levelEnv (2, 0, 0, 2500, 0)
            .osc (3, metal, 40, 30).pitch (3, 12).fm (3, 9, 25).levelEnv (3, 0, 0, 2500, 0).modulator (9, metal, 80).pitch (9, 5)
            .osc (4, fm, 80, 40).pitch (4, 24).levelEnv (4, 0, 0, 25, 0)
            .osc (5, analog, sinePos, 35).pitch (5, 7).fm (5, 6, 40).levelEnv (5, 0, 0, 1500, 0).modulator (6, analog, sinePos).pitch (6, 18, 60)
            .osc (7, growl, 70, 20).warp (7, wFold, 40).levelEnv (7, 0, 0, 1200, 0)
            .osc (8, analog, sawPos, 15).pitch (8, 24).unison (8, 4, 40, 100).levelEnv (8, 0, 0, 800, 0)
            .noise (0, 0)
            .filter (1, ladder12, 6000, 10, 0, 0)
            .env (3, 0, 400, 0, 300, 70).mod (env3, "osc1Pitch", 25)
            .env (2, 0, 300, 0, 200, 70).mod (env2, "noiseLevel", 40)
            .mod (velocity, "osc3WarpAmount", 30).mod (velocity, "osc4Volume", 30)
            .mod (velocity, "osc1Position", 40)
            .amp (1, 3500, 0, 2500).drive (hard, 35, 7000).reverb (0, 90, 5.0f).level (-7.1)
            .macro (1, "Tail").mod (macro1, "osc2Volume", 50).macro (2, "Clang").mod (macro2, "osc5Volume", 30)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 40).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        // Four. Weight 1 falling slowly; 2 a sub an octave down; 3 crushed metal crumbling, a random step
        // each sixteenth changing how crushed; strike 4; a long burst of noise. Dry; Space opens a room.
        add ("Slab Collapse", "Impact").osc (1, analog, sinePos, 70)
            .osc (2, analog, sinePos, 40).pitch (2, -12).levelEnv (2, 0, 0, 1800, 0)
            .osc (3, metal, 60, 50).pitch (3, 12).warp (3, wCrush, 40).levelEnv (3, 0, 0, 1500, 0)
            .osc (4, pulse, 80, 35).pitch (4, 24).levelEnv (4, 0, 0, 15, 0)
            .noise (0, 1)
            .filter (1, ladder12, 5000, 10, 0, 0)
            .env (3, 0, 900, 0, 200, 60).mod (env3, "osc1Pitch", 20)
            .env (2, 0, 900, 0, 300, 70).mod (env2, "noiseLevel", 50)
            .lfoSync (1, randomShape, sixteenth, 0).mod (lfo1, "osc3WarpAmount", 40)
            .mod (velocity, "osc3WarpAmount", 30).mod (velocity, "osc4Volume", 30)
            .mod (velocity, "osc1Position", 40)
            .amp (1, 2500, 0, 2000).drive (tube, 30, 6000).reverb (0, 85, 4.0f).level (-4.5)
            .macro (1, "Tail").mod (macro1, "osc2Volume", 50).macro (2, "Metal").mod (macro2, "osc3Volume", 50)
            .macro (3, "Grit").mod (macro3, "fxDriveAmount", 40).macro (4, "Space").mod (macro4, "fxReverbMix", 50);

        //======================================================================
        // Cinematic

        // Nine, a braam. Body 1 (three saws an octave down, mono) and 2 (three at the note);
        // 3 a growl an octave down; weight 4; 5 metal shimmer arriving after a second; 6 a blare, a sine
        // an octave up FM'd by 7 (unheard) as the filter opens; 8 a vowel; width 9 arriving after two.
        add ("Black Meridian", "Cinematic").osc (1, analog, sawPos, 30).pitch (1, -12).unison (1, 3, 20, 0)
            .osc (2, analog, sawPos, 25).unison (2, 3, 18, 70)
            .osc (3, growl, 50, 25).pitch (3, -12)
            .osc (4, analog, sinePos, 35).pitch (4, -12)
            .osc (5, metal, 70, 8).pitch (5, 24).levelEnv (5, 1200, 0, 0, 100).route (5, 100)
            .osc (6, analog, sinePos, 15).pitch (6, 12).fm (6, 7, 0).modulator (7, analog, sinePos)
            .osc (8, vowel, 70, 12).pitch (8, 12)
            .osc (9, additive, 60, 8).pitch (9, 19).unison (9, 4, 35, 100).levelEnv (9, 2000, 0, 0, 100).route (9, 100)
            .filter (1, ladder24, 600, 30, 40, 20)
            .env (2, 300, 2500, 40, 2000).mod (env2, "filter1Cutoff", 35).mod (env2, "osc6WarpAmount", 25)
            .lfo (1, drift, 0.1f, 0).mod (lfo1, "osc3Position", 30)
            .mod (velocity, "filter1Cutoff", 20)
            .amp (150, 3000, 70, 3000).drive (tube, 40, 6000).reverb (30, 95, 6.0f).level (-6.9)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Blare").mod (macro2, "osc6WarpAmount", 30)
            .macro (3, "Shimmer").mod (macro3, "osc5Volume", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        // Five. 1 a bright string an octave up, plucked again on the dotted eighths by a step pattern on
        // its level, so the held note glitters in a cross-rhythm; air 2 arriving; weight 3; body 4 a
        // vowel that turns over four bars; 5 metal a twelfth up, cracking in only on a hard note. A
        // long room.
        add ("Glacier Wire", "Cinematic").osc (1, string, 90, 50).pitch (1, 12)
            .osc (2, additive, 70, 42).pitch (2, 24).levelEnv (2, 800, 0, 0, 100)
            .osc (3, analog, sinePos, 25).pitch (3, -12)
            .osc (4, vowel, 20, 56)
            .osc (5, metal, 30, 8).pitch (5, 19)
            .filter (1, ladder12, 5000, 15, 0, 20)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { 100, -100, -100, 100, -100, -100, 100, -100, -100, 100, -100, -100, 100, -100, 60, -100 })
            .mod (lfo1, "osc1Volume", 35).mod (lfo1, "osc5Position", 30)
            .lfoSync (2, triangle, bars4, 0).mod (lfo2, "osc4Position", 50).mod (lfo2, "filter1Cutoff", 20)
            .mod (velocity, "osc5Volume", 40).mod (velocity, "osc1Volume", 20)
            .amp (400, 2000, 80, 3000).reverb (15, 90, 6.0f).level (-0.2)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Ice").mod (macro2, "osc2Volume", 30)
            .macro (3, "Vowel").mod (macro3, "osc4Volume", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        //======================================================================
        // Perc

        // Nine. Body 1, a sine; 2 a sine an octave up FM'd by 3 (unheard) at an odd interval; 4 metal,
        // its pitch stepped; strike 5; weight 6, a knock FM'd by 8's metal (unheard); 7 a wooden string;
        // width 9. A step pattern changes the FM and the metal's pitch on every sixteenth.
        add ("Ratchet Idol", "Perc").osc (1, analog, sinePos, 50)
            .osc (2, analog, sinePos, 35).pitch (2, 12).fm (2, 3, 35).levelEnv (2, 0, 0, 120, 0).modulator (3, analog, sinePos).pitch (3, 17, 30)
            .osc (4, metal, 60, 25).pitch (4, 19).levelEnv (4, 0, 0, 80, 0)
            .osc (5, fm, 95, 30).pitch (5, 24).levelEnv (5, 0, 0, 15, 0)
            .osc (6, pulse, 20, 25).pitch (6, -12).fm (6, 8, 20).levelEnv (6, 0, 0, 60, 0).modulator (8, metal, 50).pitch (8, 5)
            .osc (7, string, 30, 25).pitch (7, 7)
            .osc (9, vowel, 50, 12).pitch (9, 12).unison (9, 2, 10, 100).levelEnv (9, 0, 0, 150, 0)
            .filter (1, ladder12, 6000, 10, 0, 30)
            .env (3, 0, 40, 0, 30, 70).mod (env3, "osc1Pitch", 6.25f)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { 100, -40, 20, -100, 60, 0, -60, 100, -20, 40, -100, 80, 0, -40, 100, -80 })
            .mod (lfo1, "osc2WarpAmount", 25).mod (lfo1, "osc4Pitch", 8.33f)
            .mod (velocity, "osc6WarpAmount", 30)
            .mod (velocity, "osc1Position", 40)
            .amp (0, 260, 0, 200).drive (hard, 20, 7000).level (-2.4)
            .macro (1, "Knock").mod (macro1, "osc6Volume", 60).macro (2, "Hollow").mod (macro2, "osc1Volume", -40)
            .macro (3, "Wood").mod (macro3, "osc7Volume", 60).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        // Four. Body 1, metal; 2 an FM table an octave up; weight 3 a body dying fast; strike 4, crushed.
        add ("Tin Oracle", "Perc").osc (1, metal, 70, 52)
            .osc (2, fm, 20, 37).pitch (2, 12)
            .osc (3, analog, sinePos, 52).pitch (3, -12).levelEnv (3, 0, 0, 100, 0)
            .osc (4, pulse, 50, 30).pitch (4, 24).warp (4, wCrush, 60).levelEnv (4, 0, 0, 20, 0)
            .filter (1, ladder12, 7000, 15, 0, 30)
            .env (3, 0, 30, 0, 20, 70).mod (env3, "osc3Pitch", 6.25f)
            .mod (velocity, "osc4WarpAmount", 30).mod (velocity, "osc4Volume", 15)
            .mod (velocity, "osc3Position", 40)
            .amp (0, 180, 0, 150).drive (hard, 0, 8000).delay (sixteenthT, 30, 15, 5000).level (2.5)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", 35).macro (2, "Tin").mod (macro2, "osc1Volume", 30)
            .macro (3, "Body").mod (macro3, "osc3Volume", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        //======================================================================
        // Drum

        // Nine, a snare you play as an instrument: tuned to the key, and a different drum at each
        // velocity. Body 1 and 2, sines at the drum's two modes, falling; a soft hit is little more than
        // these, a knock. A hard hit brings 3, the wires, a folded FM table; 4 a metal ring a twelfth up;
        // and the noise. Strike 5; 6 a rattle, a sine FM'd by 7 (unheard) at an odd interval; 8 a throat,
        // a vowel whose mouth closes as it falls; 9 a growl tail cut into a ratchet by a step pattern.
        add ("Breaker Snare", "Drum").osc (1, analog, sinePos, 40)
            .osc (2, analog, sinePos, 30).pitch (2, 9).levelEnv (2, 0, 0, 150, 0)
            .osc (3, fm, 100, 10).pitch (3, 24).warp (3, wFold, 80).levelEnv (3, 0, 0, 180, 0)
            .osc (4, metal, 70, 5).pitch (4, 19).levelEnv (4, 0, 0, 400, 0)
            .osc (5, pulse, 50, 30).pitch (5, 24).levelEnv (5, 0, 0, 8, 0)
            .osc (6, analog, sinePos, 18).pitch (6, 12).fm (6, 7, 50).levelEnv (6, 0, 0, 90, 0).modulator (7, analog, sinePos).pitch (7, 23, 40)
            .osc (8, vowel, 90, 20).pitch (8, 12).levelEnv (8, 0, 0, 250, 0)
            .osc (9, growl, 80, 22).warp (9, wCrush, 50).levelEnv (9, 0, 60, 500, 0)
            .noise (15, 0, 20)
            .filter (1, ladder12, 9000, 10, 0, 100)
            .env (3, 0, 50, 0, 30, 70).mod (env3, "osc1Pitch", 8.33f).mod (env3, "osc2Pitch", 8.33f)
            .env (4, 0, 220, 0, 100, 60).mod (env4, "osc8Position", 70)
            .lfoSync (1, steps, bar1, 2).stepPattern (1, { 100, -100, 100, -100, 60, -100, 40, -100, 20, -100, 0, -100, 0, -100, 0, -100 })
            .mod (lfo1, "osc9Volume", 40)
            .mod (velocity, "osc3Volume", 40).mod (velocity, "osc4Volume", 40).mod (velocity, "noiseLevel", 40)
            .amp (0, 600, 0, 300).drive (hard, 25, 8000).reverb (18, 40, 1.0f).level (-3.2, 40)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", 35).macro (2, "Ratchet").mod (macro2, "osc9Volume", 50)
            .macro (3, "Ring").mod (macro3, "osc4Volume", 60).macro (4, "Throat").mod (macro4, "osc8Volume", 50);

        // Four, a tom. Body 1 falling; 2 a fifth up; 3 metal; strike 4.
        add ("Clamp Tom", "Drum").osc (1, analog, sinePos, 50)
            .osc (2, analog, sinePos, 25).pitch (2, 7).levelEnv (2, 0, 0, 350, 0)
            .osc (3, metal, 40, 45).pitch (3, 12).levelEnv (3, 0, 0, 400, 0)
            .osc (4, fm, 90, 25).pitch (4, 24).levelEnv (4, 0, 0, 10, 0)
            .filter (1, ladder12, 6000, 10, 0, 0)
            .env (3, 0, 200, 0, 80, 70).mod (env3, "osc1Pitch", 20.8f).mod (env3, "osc2Pitch", 20.8f)
            .mod (velocity, "osc4Volume", 10).mod (velocity, "filter1Cutoff", 20)
            .amp (0, 500, 0, 400).drive (tube, 30, 6000).reverb (15, 40, 1.0f).level (-0.7)
            .macro (1, "Fifth").mod (macro1, "osc2Volume", 50).macro (2, "Clamp").mod (macro2, "osc3Volume", 60)
            .macro (3, "Stick").mod (macro3, "osc4Volume", 60).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        //======================================================================
        // FX

        // Nine, a zap. Body 1 and 2 sweep down two octaves in a little over half a second, 2 FM'd by 3
        // (unheard), which stays put, so the ratio slides; 4 a vowel FM'd by 9's metal (unheard) as its
        // mouth sweeps; bite 5, a sync; 6 metal ringing; weight 7 holding still; width 8. A triplet
        // square stutters the FM; a dotted echo.
        add ("Radar Tongue", "FX").osc (1, analog, sawPos, 36)
            .osc (2, analog, sinePos, 36).pitch (2, 12).fm (2, 3, 40).modulator (3, analog, sinePos).pitch (3, 7)
            .osc (4, vowel, 0, 24).fm (4, 9, 20).modulator (9, metal, 50).pitch (9, -5)
            .osc (5, sync, 30, 24).pitch (5, 12).warp (5, wSync, 0)
            .osc (6, metal, 60, 18).pitch (6, 24).levelEnv (6, 0, 0, 200, 0)
            .osc (7, analog, sinePos, 30).pitch (7, -12)
            .osc (8, pulse, 50, 14).pitch (8, 12).unison (8, 3, 25, 100)
            .filter (1, ladder12, 6000, 20, 0, 20)
            .env (3, 0, 600, 0, 300, 70).mod (env3, "osc1Pitch", 50).mod (env3, "osc2Pitch", 50).mod (env3, "osc5WarpAmount", 40).mod (env3, "osc4Position", 60)
            .lfoSync (1, square, sixteenthT, 0).mod (lfo1, "osc2WarpAmount", 20)
            .mod (velocity, "osc4WarpAmount", 30).mod (velocity, "filter1Cutoff", 20)
            .amp (1, 900, 0, 500).delay (eighthD, 50, 30, 4000).reverb (25, 70, 2.5f).level (4)
            .macro (1, "Dark").mod (macro1, "filter1Cutoff", -40).macro (2, "Stutter").mod (macro2, "osc2WarpAmount", 30)
            .macro (3, "Echo").mod (macro3, "fxDelayFeedback", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        // Four, a two-tone siren. Body 1 and 2, pulses a fifth apart, wobbling together; weight 3, a
        // growl; grit 4, crushed metal.
        add ("Klaxon Swamp", "FX").osc (1, pulse, 50, 62)
            .osc (2, pulse, 50, 44).pitch (2, 7)
            .osc (3, growl, 50, 44).pitch (3, -12)
            .osc (4, metal, 40, 18).pitch (4, 12).warp (4, wCrush, 50)
            .filter (1, ladder12, 4000, 20, 0, 20)
            .lfo (1, sine, 6.0f, 1).mod (lfo1, "osc1Pitch", 4.17f).mod (lfo1, "osc2Pitch", 4.17f)
            .lfo (2, triangle, 0.5f, 1).mod (lfo2, "filter1Cutoff", 20)
            .mod (velocity, "osc4WarpAmount", 30).mod (velocity, "filter1Cutoff", 20)
            .amp (5, 800, 70, 400).phaser (40, 0.5f, 60).delay (quarter, 35, 20, 3500).level (1.3)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", 35).macro (2, "Wail").mod (macro2, "lfo1Rate", 30)
            .macro (3, "Swamp").mod (macro3, "osc3Volume", 30).macro (4, "Grit").mod (macro4, "osc4Volume", 30);

        //======================================================================
        // Glitch

        // Nine. A thirty-second-note step pattern and a random sixteenth hand the sound between its
        // layers: body 1, a saw; 2 a sine FM'd by 3 (unheard); 4 a crushed pulse; 5 metal; 6 a vowel;
        // weight 7; 8 a sync; 9 a folded growl. A stuttering thirty-second echo.
        add ("Fault Mosaic", "Glitch").osc (1, analog, sawPos, 30)
            .osc (2, analog, sinePos, 25).pitch (2, 12).fm (2, 3, 20).modulator (3, analog, sinePos).pitch (3, 19)
            .osc (4, pulse, 50, 20).pitch (4, 12).warp (4, wCrush, 80)
            .osc (5, metal, 60, 12).pitch (5, 24)
            .osc (6, vowel, 50, 20)
            .osc (7, analog, sinePos, 25).pitch (7, -12)
            .osc (8, sync, 40, 15).pitch (8, 12).warp (8, wSync, 20)
            .osc (9, growl, 60, 15).warp (9, wFold, 60)
            .filter (1, ladder12, 5000, 15, 0, 20)
            .lfoSync (1, steps, half, 0).stepPattern (1, { 100, -100, -100, 100, 0, 100, -100, 60, -60, 100, 100, -100, 40, -100, 100, -40 })
            .mod (lfo1, "osc4Volume", 30).mod (lfo1, "osc6Position", 50).mod (lfo1, "osc2WarpAmount", 30)
            .lfoSync (2, randomShape, sixteenth, 0).mod (lfo2, "osc8WarpAmount", 40).mod (lfo2, "osc9Volume", -20).mod (lfo2, "filter1Cutoff", 20)
            .mod (velocity, "osc8WarpAmount", 30).mod (velocity, "filter1Cutoff", 20)
            .amp (0, 120, 60, 40).drive (crush, 30, 8000).delay (thirtySecond, 50, 20, 6000).level (1.6)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", 35).macro (2, "Fault").mod (macro2, "osc4Volume", 30)
            .macro (3, "Stutter").mod (macro3, "fxDelayMix", 30).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        // Four, a choir rotting in its bits. Body 1 and 2, crushed vowels an octave apart, a step pattern
        // changing how crushed; weight 3; strike 4.
        add ("Bitrot Canticle", "Glitch").osc (1, vowel, 30, 59).warp (1, wCrush, 40)
            .osc (2, vowel, 70, 42).pitch (2, 12).warp (2, wCrush, 40)
            .osc (3, pulse, 40, 42).pitch (3, -12)
            .osc (4, fm, 90, 34).pitch (4, 24).levelEnv (4, 0, 0, 15, 0)
            .filter (1, ladder12, 5000, 15, 0, 20)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { 100, -100, 40, -40, 100, 0, -100, 60, -60, 100, -20, -100, 80, -80, 100, -100 })
            .mod (lfo1, "osc1WarpAmount", 40).mod (lfo1, "osc2WarpAmount", -40).mod (lfo1, "osc1Position", 25)
            .mod (velocity, "osc1WarpAmount", 30).mod (velocity, "osc4Volume", 30)
            .amp (1, 300, 60, 100).delay (sixteenth, 40, 20, 5000).level (0)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", 35).macro (2, "Rot").mod (macro2, "osc1WarpAmount", 40)
            .macro (3, "Vowel").mod (macro3, "osc1Position", 30).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);

        //======================================================================
        // Game

        // Nine, a boss's theme. Body 1, a square whose octave a step pattern flips; weight 2; 3 a sine
        // an octave up FM'd by 4's square (unheard) a fifth up; 5 a vowel flipping with it; 6 metal; 7 a
        // folded growl an octave down; width 8; strike 9.
        add ("Boss Gate", "Game").osc (1, analog, squarePos, 30)
            .osc (2, pulse, 50, 30).pitch (2, -12)
            .osc (3, analog, sinePos, 20).pitch (3, 12).fm (3, 4, 30).modulator (4, analog, squarePos).pitch (4, 19)
            .osc (5, vowel, 70, 18)
            .osc (6, metal, 50, 10).pitch (6, 19).levelEnv (6, 0, 0, 150, 0)
            .osc (7, growl, 40, 20).pitch (7, -12).warp (7, wFold, 30)
            .osc (8, analog, sawPos, 12).pitch (8, 12).unison (8, 3, 15, 100)
            .osc (9, pulse, 80, 25).pitch (9, 24).levelEnv (9, 0, 0, 15, 0)
            .filter (1, ladder24, 3000, 25, 0, 30)
            .lfoSync (1, steps, quarter, 0).stepPattern (1, { 0, 0, 100, 100, 0, 0, 100, 100, 0, 0, 100, 100, 0, 0, 100, 100 })
            .mod (lfo1, "osc1Pitch", 25).mod (lfo1, "osc5Pitch", 25)
            .env (2, 0, 200, 0, 100, 70).mod (env2, "filter1Cutoff", 25)
            .mod (velocity, "osc7WarpAmount", 30).mod (velocity, "osc9Volume", 30)
            .amp (1, 300, 50, 150).drive (hard, 30, 6000).reverb (20, 60, 2.0f).level (-1.2)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Menace").mod (macro2, "osc7Volume", 30)
            .macro (3, "FM").mod (macro3, "osc3WarpAmount", 35).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        // Four, a machine with a dog's temper. 1 a sync an LFO growls through; body 2, a square; weight 3;
        // strike 4, crushed metal.
        add ("Servo Hound", "Game").osc (1, sync, 40, 30).pitch (1, 12).warp (1, wSync, 20)
            .osc (2, analog, squarePos, 30)
            .osc (3, analog, sinePos, 30).pitch (3, -12)
            .osc (4, metal, 70, 20).pitch (4, 24).warp (4, wCrush, 60).levelEnv (4, 0, 0, 20, 0)
            .filter (1, ladder24, 2500, 30, 0, 30)
            .lfoSync (1, sawDown, eighth, 1).mod (lfo1, "osc1WarpAmount", 40)
            .env (2, 0, 150, 0, 100, 70).mod (env2, "filter1Cutoff", 30)
            .mod (velocity, "osc1WarpAmount", 30).mod (velocity, "osc4Volume", 30)
            .amp (1, 250, 40, 100).drive (hard, 25, 6000).level (1.8)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Growl").mod (macro2, "osc1WarpAmount", 30)
            .macro (3, "Square").mod (macro3, "osc2Volume", 50).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        //======================================================================
        // Chip

        // Nine, a game chip with weight. Body 1 and 2, crushed pulses an octave apart, running a minor
        // arpeggio inside each note; weight 3, a crushed triangle an octave down; 4 the noise channel's
        // hit; 5 a twelfth, dying; 6 a sine an octave up FM'd by 7's square (unheard); width 8; strike 9.
        add ("Silicon Brawler", "Chip").osc (1, pulse, 50, 39).warp (1, wCrush, 30)
            .osc (2, pulse, 80, 25).pitch (2, 12).warp (2, wCrush, 30)
            .osc (3, analog, trianglePos, 39).pitch (3, -12).warp (3, wCrush, 50)
            .osc (4, fm, 100, 25).pitch (4, 24).warp (4, wCrush, 90).levelEnv (4, 0, 0, 25, 0)
            .osc (5, analog, squarePos, 11).pitch (5, 19).levelEnv (5, 0, 0, 80, 0)
            .osc (6, analog, sinePos, 17).pitch (6, 12).fm (6, 7, 30).modulator (7, analog, squarePos).pitch (7, 12)
            .osc (8, pulse, 30, 11).pitch (8, 12).unison (8, 2, 6, 100)
            .osc (9, metal, 80, 17).pitch (9, 24).warp (9, wCrush, 70).levelEnv (9, 0, 0, 10, 0)
            .filter (1, ladder12, 8000, 10, 0, 20)
            .lfoSync (1, steps, quarter, 0).stepPattern (1, { 0, 25, 58.33f, 0, 25, 58.33f, 0, 25, 58.33f, 0, 25, 58.33f, 0, 25, 58.33f, 100 })
            .mod (lfo1, "osc1Pitch", 25).mod (lfo1, "osc2Pitch", 25)
            .mod (velocity, "osc1WarpAmount", 30).mod (velocity, "osc4Volume", 30)
            .mod (velocity, "osc3Position", 40)
            .amp (1, 200, 60, 60).drive (crush, 20, 9000).delay (eighthD, 30, 15, 5000).level (4.8)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", 35).macro (2, "Duty").mod (macro2, "osc1Position", 40)
            .macro (3, "FM").mod (macro3, "osc6WarpAmount", 50).macro (4, "Grit").mod (macro4, "fxDriveAmount", 50);

        // Four, a chip line that plays itself. Body 1, a crushed pulse whose width steps every sixteenth;
        // weight 2, a crushed triangle an octave down; 3 a pulse that arpeggiates up a fifth and an octave
        // inside the held note (a step pattern on its pitch); strike 4. The resonant filter's cutoff
        // walks over two bars; a hard note opens it more.
        add ("Sprite Acid", "Chip").osc (1, pulse, 30, 40).warp (1, wCrush, 40)
            .osc (2, analog, trianglePos, 30).pitch (2, -12).warp (2, wCrush, 60)
            .osc (3, pulse, 60, 28).pitch (3, 12).warp (3, wSqueeze, 30)
            .osc (4, fm, 100, 20).pitch (4, 24).warp (4, wCrush, 80).levelEnv (4, 0, 0, 15, 0)
            .filter (1, ladder24, 1300, 55, 20, 40)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { 0, 58.33f, 100, 58.33f, 0, 58.33f, 100, 100, 0, 58.33f, 100, 58.33f, 0, 100, 58.33f, 0 })
            .mod (lfo1, "osc3Pitch", 25).mod (lfo1, "osc1Position", 30)
            .lfoSync (2, sawUp, bars2, 0).mod (lfo2, "filter1Cutoff", 25).mod (lfo2, "osc3WarpAmount", 25)
            .env (2, 0, 180, 0, 80, 75).mod (env2, "filter1Cutoff", 30).mod (velocity, "filter1Cutoff", 20)
            .amp (1, 200, 60, 60).mono (30).level (3.7)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Bits").mod (macro2, "osc1WarpAmount", 40)
            .macro (3, "Arp").mod (macro3, "osc3Volume", 40).macro (4, "Res").mod (macro4, "filter1Resonance", 30);

        //======================================================================
        // Texture

        // Nine, ash in the wind. 1 three vowels an octave up, strummed and drifting; 2 two metals,
        // gusting on the sixteenths (a step pattern on their level); air 3; 4 a growl an octave down;
        // 5 a sine a fifth up FM'd by 6's metal (unheard); grit 7, crushed, which a hard note brings;
        // weight 8; 9 a folded FM table; crackle. The wind rises and falls over four bars, turning the
        // mouths, the fold and the filter.
        add ("Ash Weather", "Texture").osc (1, vowel, 20, 53).pitch (1, 12).unison (1, 3, 40, 90, 40, 60)
            .osc (2, metal, 60, 32).pitch (2, 24).unison (2, 2, 20, 100, 0, 80)
            .osc (3, additive, 80, 21).pitch (3, 24)
            .osc (4, growl, 50, 53).pitch (4, -12)
            .osc (5, analog, sinePos, 32).pitch (5, 7).fm (5, 6, 30).modulator (6, metal, 40).pitch (6, 11, 30)
            .osc (7, pulse, 50, 8).pitch (7, 19).warp (7, wCrush, 70)
            .osc (8, analog, sinePos, 53).pitch (8, -12)
            .osc (9, fm, 50, 27).pitch (9, 12).warp (9, wFold, 40)
            .noise (6, 2, 60)
            .filter (1, ladder12, 3000, 20, 0, 20)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { 60, -100, 100, -40, -100, 80, -100, 100, 20, -100, 100, -60, 100, -100, 40, 100 })
            .mod (lfo1, "osc2Volume", 35).mod (lfo1, "osc9WarpAmount", 20)
            .lfoSync (2, triangle, bars4, 0).mod (lfo2, "filter1Cutoff", 25).mod (lfo2, "osc1Position", 40).mod (lfo2, "osc5WarpAmount", 30)
            .mod (velocity, "osc7Volume", 30).mod (velocity, "osc9WarpAmount", 20)
            .amp (1500, 1000, 100, 3000).reverb (8, 90, 6.0f).level (-4)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Air").mod (macro2, "osc3Volume", 60)
            .macro (3, "Metal").mod (macro3, "osc2Volume", 60).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        // Four, a lung of tar breathing. Body 1 and 2, vowels at the ends of the table, a half-bar LFO
        // handing the breath between them; 3 metal, rattling on the sixteenths (a step pattern) as if
        // the lung wheezes; weight 4, a growl that heaves over four bars; pink noise breathing with them.
        // A hard note wheezes harder.
        add ("Tarred Lung", "Texture").osc (1, vowel, 10, 25)
            .osc (2, vowel, 90, 20)
            .osc (3, metal, 30, 10).pitch (3, 12)
            .osc (4, growl, 40, 18).pitch (4, -12)
            .noise (15, 1)
            .filter (1, ladder12, 2500, 20, 0, 20)
            .lfoSync (1, sine, half, 0).mod (lfo1, "osc1Volume", 25).mod (lfo1, "osc2Volume", -25).mod (lfo1, "noiseLevel", 15)
            .lfoSync (2, steps, bar1, 0).stepPattern (2, { 100, 40, -100, 80, -60, 100, -100, 20, 100, -100, 60, -40, 100, -80, 40, -100 })
            .mod (lfo2, "osc3Volume", 25).mod (lfo2, "osc4WarpAmount", 15)
            .mod (velocity, "osc3Volume", 20).mod (velocity, "filter1Cutoff", 15)
            .amp (1000, 1000, 100, 2500).reverb (35, 80, 4.0f).level (3.9)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Breath").mod (macro2, "noiseLevel", 30)
            .macro (3, "Metal").mod (macro3, "osc3Volume", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        //======================================================================
        // Vox

        // Nine, a choir in one voice. 1 to 4 vowels at four mouths, two of them a few cents apart, 4 an
        // octave up; 5 a bass voice an octave down; 6 a chest buzz, a sine FM'd by 7 (unheard) at its own
        // pitch; width 8; strike 9, a consonant. A bar-long LFO turns two mouths opposite ways, and a
        // vibrato arrives late.
        add ("Throat Basilica", "Vox").osc (1, vowel, 10, 47)
            .osc (2, vowel, 35, 41).pitch (2, 0, 6)
            .osc (3, vowel, 60, 38).pitch (3, 0, -6)
            .osc (4, vowel, 85, 26).pitch (4, 12)
            .osc (5, vowel, 50, 38).pitch (5, -12)
            .osc (6, analog, sinePos, 28).fm (6, 7, 15).modulator (7, analog, sinePos)
            .osc (8, vowel, 40, 19).pitch (8, 12).unison (8, 3, 18, 100)
            .osc (9, fm, 95, 28).pitch (9, 24).levelEnv (9, 0, 0, 30, 0)
            .filter (1, ladder12, 5000, 15, 0, 20)
            .lfoSync (1, sine, bar1, 0).mod (lfo1, "osc1Position", 25).mod (lfo1, "osc2Position", -25)
            .lfo (2, sine, 5.0f, 1).lfoFade (2, 600).mod (lfo2, "osc1Fine", 4).mod (lfo2, "osc5Fine", 4)
            .mod (velocity, "osc9Volume", 30).mod (velocity, "filter1Cutoff", 20)
            .amp (60, 500, 90, 500).mono (50).chorus (20).reverb (35, 80, 3.0f).level (1.4)
            .macro (1, "Vowel").mod (macro1, "osc1Position", 30).macro (2, "Bass").mod (macro2, "osc5Volume", 30)
            .macro (3, "Chest").mod (macro3, "osc6WarpAmount", 30).macro (4, "Space").mod (macro4, "fxReverbMix", 30);

        // Four, a vocal chop. Body 1 and 2, vowels an octave apart whose mouths a step pattern changes
        // each sixteenth; weight 3; strike 4.
        add ("Larynx Pump", "Vox").osc (1, vowel, 30, 49)
            .osc (2, vowel, 70, 28).pitch (2, 12)
            .osc (3, growl, 40, 35).pitch (3, -12)
            .osc (4, pulse, 70, 28).pitch (4, 24).levelEnv (4, 0, 0, 15, 0)
            .filter (1, ladder12, 4500, 20, 0, 20)
            .lfoSync (1, steps, bar1, 0).stepPattern (1, { -100, 60, -20, 100, -60, 20, 100, -100, 40, -40, 100, -80, 0, 80, -100, 60 })
            .mod (lfo1, "osc1Position", 45).mod (lfo1, "osc2Position", 35)
            .mod (velocity, "osc4Volume", 30).mod (velocity, "filter1Cutoff", 20)
            .amp (1, 200, 60, 80).delay (sixteenthD, 35, 18, 4500).level (4.7)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 35).macro (2, "Mouth").mod (macro2, "osc1Position", 30)
            .macro (3, "Weight").mod (macro3, "osc3Volume", 30).macro (4, "Echo").mod (macro4, "fxDelayMix", 30);
    }
}

namespace WaveSynth
{
    const std::vector<Patch>& complexPatches()
    {
        static const auto all = []
        {
            std::vector<WaveSynthPatchBuilder::Builder> b;
            WaveSynthPatchBuilder::addComplex (b);
            std::stable_sort (b.begin(), b.end(), [] (const auto& x, const auto& y)
            {
                return patchCategories().indexOf (x.patch.category) < patchCategories().indexOf (y.patch.category);
            });

            std::vector<Patch> result;

            for (auto& p : b)
                result.push_back (std::move (p.patch));

            return result;
        }();
        return all;
    }
}
