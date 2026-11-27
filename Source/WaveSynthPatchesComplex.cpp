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

        //======================================================================
        // Musical Club Nine-Oscillator Collection (2 per category, all 9 oscillators)
        //======================================================================

        //----------------------------------------------------------------------
        // Bass

        // Nine. Velvet Roller: warm Afterlife/Bodzin rolling analog club bass.
        // 1 pure mono sub sine; 2 warm saturated sub; 3 analog saw body FM'd gently by 4 (1:2 woody);
        // 5 warm pulse octave; 6 Karplus plucked transient; 7 & 8 stereo high-passed detuned saws
        // pulsing on an 8th-note groove; 9 glassy additive sparkle opening with velocity.
        add ("Velvet Roller", "Bass").osc (1, analog, sinePos, 78)
            .osc (2, sub, 30, 56)
            .osc (3, analog, sawPos, 54).fm (3, 4, 10).modulator (4, analog, sinePos).pitch (4, 12)
            .osc (5, pulse, 35, 36).pitch (5, 12).warp (5, wBend, 20)
            .osc (6, string, 65, 34).pitch (6, 12).levelEnv (6, 0, 0, 55, 0)
            .osc (7, analog, sawPos, 28).pitch (7, 12, -9).pan (7, -55).route (7, 100)
            .osc (8, analog, sawPos, 28).pitch (8, 12, 9).pan (8, 55).route (8, 100)
            .osc (9, additive, 55, 16).pitch (9, 24).route (9, 100)
            .parallel().filter (1, ladder24, 480, 28, 22, 35).filter (2, svfHigh, 340, 12)
            .env (2, 0, 190, 8, 90, 68).mod (env2, "filter1Cutoff", 34).mod (env2, "osc3WarpAmount", 18)
            .lfoSync (1, triangle, eighth, 0, 20).mod (lfo1, "filter1Cutoff", 20).mod (lfo1, "osc7Volume", 32).mod (lfo1, "osc8Volume", 32)
            .lfoSync (2, sine, bars2, 0).mod (lfo2, "osc5Position", 35).mod (lfo2, "filter2Cutoff", 18)
            .mod (velocity, "filter1Cutoff", 24).mod (velocity, "osc9Volume", 36).mod (velocity, "osc3WarpAmount", 18)
            .amp (2, 220, 52, 85).mono (25.0f, true).drive (tube, 16, 6500).level (-5.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Wood").mod (macro2, "osc3WarpAmount", 38)
            .macro (3, "Width").mod (macro3, "osc7Volume", 42)
            .macro (4, "Warmth").mod (macro4, "fxDriveAmount", 48);

        // Nine. Midnight Garage: silky UKG / deep-house woody FM donk-bass.
        // 1 pure mono sub; 2 classic 1:2 FM carrier driven by 3 (unheard) + 4 (1:3 sheen, unheard);
        // 5 hollow square body; 6 warm octave triangle; 7 & 8 stereo high-passed vowel/additive
        // shimmer rocking on dotted 8ths; 9 crisp hammer transient.
        add ("Midnight Garage", "Bass").osc (1, analog, sinePos, 82)
            .osc (2, analog, sinePos, 66).fm (2, 3, 28).modulator (3, analog, sinePos).pitch (3, 12).fm (3, 4, 14).modulator (4, analog, sinePos).pitch (4, 19)
            .osc (5, analog, squarePos, 32).pitch (5, 0, 4)
            .osc (6, analog, trianglePos, 38).pitch (6, 12)
            .osc (7, vowel, 35, 22).pitch (7, 12).pan (7, -45).route (7, 100)
            .osc (8, additive, 35, 22).pitch (8, 19).pan (8, 45).route (8, 100)
            .osc (9, fm, 30, 32).pitch (9, 24).levelEnv (9, 0, 0, 28, 0).route (9, 100)
            .parallel().filter (1, ladder24, 620, 24, 16, 40).filter (2, svfHigh, 360, 14)
            .env (2, 0, 145, 0, 80, 75).mod (env2, "filter1Cutoff", 32).mod (env2, "osc2WarpAmount", 34)
            .lfoSync (1, sine, eighthD, 0, 15).mod (lfo1, "osc2WarpAmount", 20).mod (lfo1, "osc7Position", 40).mod (lfo1, "filter1Cutoff", 16)
            .lfoSync (2, triangle, bar1, 0).mod (lfo2, "osc8Position", 35)
            .mod (velocity, "osc2WarpAmount", 32).mod (velocity, "filter1Cutoff", 22).mod (velocity, "osc9Volume", 35)
            .amp (2, 260, 38, 90).mono (18.0f, true).drive (soft, 18, 7000).level (-7.5)
            .macro (1, "Donk").mod (macro1, "osc2WarpAmount", 42)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 36)
            .macro (3, "Shimmer").mod (macro3, "osc8Volume", 48)
            .macro (4, "Drive").mod (macro4, "fxDriveAmount", 50);

        //----------------------------------------------------------------------
        // Sub

        // Nine. Abyssal Pulse: seismic melodic-techno chest sub with hypnotic 1/4-note breathing halo.
        // 1 pure direct sine; 2 rounded saturated sub; 3 warm triangle; 4 octave-up sine FM'd by 5 (1:2);
        // 6 soft saw octave breathing on 1/4-note sidechain; 7 & 8 high-passed stereo fifth/octave glass
        // choir; 9 soft pluck transient.
        add ("Abyssal Pulse", "Sub").osc (1, analog, sinePos, 86).route (1, 100)
            .osc (2, sub, 28, 52)
            .osc (3, analog, trianglePos, 38)
            .osc (4, analog, sinePos, 34).pitch (4, 12).fm (4, 5, 16).modulator (5, analog, sinePos).pitch (5, 24)
            .osc (6, analog, sawPos, 24).pitch (6, 12)
            .osc (7, additive, 30, 16).pitch (7, 19).pan (7, -40)
            .osc (8, vowel, 45, 16).pitch (8, 24).pan (8, 40)
            .osc (9, string, 55, 22).pitch (9, 12).levelEnv (9, 0, 0, 35, 0)
            .filter (1, ladder24, 340, 22, 14, 35)
            .lfoSync (1, sawUp, quarter, 0, 30).mod (lfo1, "filter1Cutoff", 26).mod (lfo1, "osc6Volume", 38).mod (lfo1, "osc4WarpAmount", 24)
            .lfoSync (2, sine, bars2, 0).mod (lfo2, "osc7Position", 40).mod (lfo2, "osc8Position", 35)
            .env (2, 0, 65, 0, 40, 75).mod (env2, "osc1Pitch", 2.08f)
            .mod (velocity, "filter1Cutoff", 28).mod (velocity, "osc4WarpAmount", 28).mod (velocity, "osc6Volume", 30)
            .amp (3, 350, 88, 120).mono (20.0f, true).drive (soft, 12, 4000).level (-9)
            .macro (1, "Glow").mod (macro1, "filter1Cutoff", 40)
            .macro (2, "Pulse").mod (macro2, "osc6Volume", 48)
            .macro (3, "Halo").mod (macro3, "osc7Volume", 50)
            .macro (4, "Warmth").mod (macro4, "fxDriveAmount", 55);

        // Nine. Silk Undertow: liquid DnB / deep house rolling sub with velvety low-mid purr.
        // 1 pure direct sine; 2 warm sub-wave; 3 2:1 warm FM fundamental (3 driven by 4);
        // 5 soft octave pulse; 6 octave triangle; 7 & 8 stereo high-passed air twins blooming on velocity;
        // 9 subtle woody knock.
        add ("Silk Undertow", "Sub").osc (1, analog, sinePos, 84).route (1, 100)
            .osc (2, sub, 48, 46)
            .osc (3, analog, sinePos, 46).fm (3, 4, 12).modulator (4, analog, trianglePos).pitch (4, 12)
            .osc (5, pulse, 30, 26).pitch (5, 12)
            .osc (6, analog, trianglePos, 30).pitch (6, 19)
            .osc (7, analog, sawPos, 15).pitch (7, 24, -7).pan (7, -50)
            .osc (8, analog, sawPos, 15).pitch (8, 24, 7).pan (8, 50)
            .osc (9, fm, 22, 24).pitch (9, 12).levelEnv (9, 0, 0, 26, 0)
            .filter (1, ladder12, 460, 24, 12, 40)
            .lfoSync (1, triangle, eighth, 0, 20).mod (lfo1, "filter1Cutoff", 24).mod (lfo1, "osc5Position", 45).mod (lfo1, "osc3WarpAmount", 20)
            .lfoSync (2, sine, bars4, 0).mod (lfo2, "osc6Volume", 25).mod (lfo2, "filter1Resonance", 15)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc3WarpAmount", 28).mod (velocity, "osc7Volume", 35)
            .amp (4, 320, 90, 140).mono (45.0f, true).drive (tube, 10, 4500).level (-7.3)
            .macro (1, "Purr").mod (macro1, "osc3WarpAmount", 42)
            .macro (2, "Cutoff").mod (macro2, "filter1Cutoff", 38)
            .macro (3, "Air").mod (macro3, "osc8Volume", 50)
            .macro (4, "Saturate").mod (macro4, "fxDriveAmount", 50);

        //----------------------------------------------------------------------
        // Reese

        // Nine. Aurora Reese: lush Liquid DnB / melodic-club Reese with mono sub anchor and wide silk shimmer.
        // 1 pure mono sub; 2 & 3 beating mono saws (+/-8 ct); 4 warm pulse body; 5 FM shimmer (driven by 6);
        // 7 & 8 stereo high-passed unison saws + additive choir an octave up; 9 airy fifth shimmer.
        add ("Aurora Reese", "Reese").osc (1, analog, sinePos, 74)
            .osc (2, analog, sawPos, 50).pitch (2, 0, -8)
            .osc (3, analog, sawPos, 50).pitch (3, 0, 8)
            .osc (4, pulse, 40, 32)
            .osc (5, analog, sinePos, 36).pitch (5, 12).fm (5, 6, 18).modulator (6, analog, sinePos).pitch (6, 19)
            .osc (7, analog, sawPos, 28).pitch (7, 12).unison (7, 4, 18, 95, 0, 25).route (7, 100)
            .osc (8, additive, 50, 24).pitch (8, 12, 6).pan (8, -45).route (8, 100)
            .osc (9, vowel, 40, 22).pitch (9, 19).pan (9, 45).route (9, 100)
            .parallel().filter (1, ladder24, 360, 28, 22, 25).filter (2, svfHigh, 320, 14)
            .lfoSync (1, sine, eighth, 0, 15).mod (lfo1, "filter1Cutoff", 22).mod (lfo1, "osc5WarpAmount", 22).mod (lfo1, "osc8Position", 35)
            .lfoSync (2, triangle, bars2, 0).mod (lfo2, "filter2Cutoff", 24).mod (lfo2, "osc9Position", 40)
            .env (2, 15, 700, 35, 300).mod (env2, "filter1Cutoff", 22)
            .mod (velocity, "filter1Cutoff", 24).mod (velocity, "osc7Volume", 32).mod (velocity, "osc5WarpAmount", 24)
            .amp (10, 400, 95, 220).mono (65.0f, true).drive (tube, 18, 5500).level (-8.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Detune").mod (macro2, "osc3Fine", 14)
            .macro (3, "Aurora").mod (macro3, "osc7Volume", 42)
            .macro (4, "Warmth").mod (macro4, "fxDriveAmount", 45);

        // Nine. Solstice Drift: emotional progressive-house / deep-techno PWM Reese with silky parallel notch motion.
        // 1 pure sub; 2 & 3 opposite-moving mono PWM pulses (+/-7 ct); 4 warm triangle; 5 octave analog saw
        // FM'd gently by 6 (1:1); 7 & 8 panned stereo fifth/octave glass voices through parallel SVF Notch; 9 breathy top.
        add ("Solstice Drift", "Reese").osc (1, sub, 20, 74)
            .osc (2, pulse, 25, 48).pitch (2, 0, -7)
            .osc (3, pulse, 75, 48).pitch (3, 0, 7)
            .osc (4, analog, trianglePos, 36).pitch (4, 12)
            .osc (5, analog, sawPos, 34).pitch (5, 12).fm (5, 6, 12).modulator (6, analog, sinePos).pitch (6, 12)
            .osc (7, additive, 45, 24).pitch (7, 19).pan (7, -55).route (7, 100)
            .osc (8, vowel, 55, 24).pitch (8, 12).pan (8, 55).route (8, 100)
            .osc (9, analog, sawPos, 20).pitch (9, 24).unison (9, 3, 14, 90).route (9, 100)
            .parallel().filter (1, ladder12, 420, 26, 18, 25).filter (2, svfHigh, 360, 20)
            .lfoSync (1, triangle, quarter, 0, 15).mod (lfo1, "osc2Position", 42).mod (lfo1, "osc3Position", -42).mod (lfo1, "filter1Cutoff", 20)
            .lfoSync (2, sine, eighthD, 0).mod (lfo2, "filter2Cutoff", 30).mod (lfo2, "osc8Position", 35)
            .mod (velocity, "filter1Cutoff", 24).mod (velocity, "osc5WarpAmount", 28).mod (velocity, "osc9Volume", 35)
            .amp (12, 450, 95, 250).mono (75.0f, true).drive (soft, 20, 6000).level (-11)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "PWM").mod (macro2, "osc2Position", 36)
            .macro (3, "Shimmer").mod (macro3, "osc7Volume", 45)
            .macro (4, "Warmth").mod (macro4, "fxDriveAmount", 48);

        //----------------------------------------------------------------------
        // Acid

        // Nine. Liquid Acid: hypnotic melodic-techno / classic 303-plus liquid acid.
        // 1 crisp saw; 2 warm square; 3 mono sub octave; 4 1:2 FM squelch driven by 5 (unheard);
        // 6 silky sync overtone; 7 & 8 high-passed wide stereo harmonic twins; 9 crisp pluck bite.
        add ("Liquid Acid", "Acid").osc (1, analog, sawPos, 66)
            .osc (2, analog, squarePos, 42)
            .osc (3, analog, sinePos, 54).pitch (3, -12)
            .osc (4, analog, sawPos, 40).fm (4, 5, 10).modulator (5, analog, sinePos).pitch (5, 12)
            .osc (6, sync, 25, 28).pitch (6, 12).warp (6, wSync, 15)
            .osc (7, pulse, 35, 18).pitch (7, 12, -8).pan (7, -55).route (7, 100)
            .osc (8, pulse, 35, 18).pitch (8, 12, 8).pan (8, 55).route (8, 100)
            .osc (9, string, 75, 26).pitch (9, 12).levelEnv (9, 0, 0, 35, 0)
            .parallel().filter (1, ladder24, 520, 66, 26, 45).filter (2, svfHigh, 420, 18)
            .env (2, 0, 210, 0, 95, 76).mod (env2, "filter1Cutoff", 44).mod (env2, "osc6WarpAmount", 32).mod (env2, "osc4WarpAmount", 22)
            .lfoSync (1, steps, bar1, 0, 35).stepPattern (1, { 85, 20, -40, 65, -20, 90, -50, 30, 75, -30, 50, 95, -40, 25, 60, -25 })
            .mod (lfo1, "filter1Cutoff", 18).mod (lfo1, "osc6Position", 38)
            .lfoSync (2, triangle, bars2, 0).mod (lfo2, "filter1Resonance", 16)
            .mod (velocity, "filter1Cutoff", 24).mod (velocity, "osc4WarpAmount", 28)
            .amp (2, 220, 68, 70).mono (55.0f, true).drive (tube, 22, 7500).delay (eighthD, 34, 18, 3600, true).level (-7.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Res").mod (macro2, "filter1Resonance", 28)
            .macro (3, "Liquid").mod (macro3, "osc6WarpAmount", 45)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 40);

        // Nine. Neon Squelch: progressive-trance / electro singing formant-acid.
        // 1 warm saw; 2 mono sub octave; 3 liquid sync; 4 vowel formant octave singing on 8ths;
        // 5 FM bell-acid layer driven by 6 (1:3); 7 & 8 high-passed wide stereo sheen; 9 sharp pick.
        add ("Neon Squelch", "Acid").osc (1, analog, sawPos, 64)
            .osc (2, sub, 30, 52).pitch (2, -12)
            .osc (3, sync, 35, 38).warp (3, wSync, 18)
            .osc (4, vowel, 20, 38).pitch (4, 12)
            .osc (5, analog, sinePos, 30).pitch (5, 12).fm (5, 6, 16).modulator (6, analog, sinePos).pitch (6, 19)
            .osc (7, additive, 60, 18).pitch (7, 12, -10).pan (7, -50).route (7, 100)
            .osc (8, additive, 60, 18).pitch (8, 12, 10).pan (8, 50).route (8, 100)
            .osc (9, fm, 45, 24).pitch (9, 24).levelEnv (9, 0, 0, 24, 0).route (9, 100)
            .parallel().filter (1, ladder24, 580, 62, 24, 45).filter (2, svfHigh, 450, 16)
            .env (2, 0, 195, 0, 85, 74).mod (env2, "filter1Cutoff", 40).mod (env2, "osc3WarpAmount", 36)
            .lfoSync (1, sine, eighth, 0, 15).mod (lfo1, "osc4Position", 52).mod (lfo1, "filter1Cutoff", 18).mod (lfo1, "osc5WarpAmount", 22)
            .lfoSync (2, triangle, bars2, 0).mod (lfo2, "filter2Cutoff", 24)
            .mod (velocity, "filter1Cutoff", 22).mod (velocity, "osc3WarpAmount", 30)
            .amp (2, 210, 64, 65).mono (48.0f, true).drive (soft, 24, 8000).delay (sixteenthD, 30, 18, 4200, true).level (-8)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Formant").mod (macro2, "osc4Position", 45)
            .macro (3, "Squelch").mod (macro3, "filter1Resonance", 30)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 38);

        //----------------------------------------------------------------------
        // Lead

        // Nine. Afterlife Anthem: soaring melodic-techno / Bodzin emotional analog lead.
        // 1 warm detuned saw stack; 2 silky PWM pulse; 3 expressive 1:2 FM voice driven by 4 (unheard);
        // 5 fifth harmonic bloom (levelEnv); 6 Karplus plucked attack; 7 & 8 wide stereo high-passed
        // supersaw shimmer; 9 crystalline additive top air. Clean above 200Hz!
        add ("Afterlife Anthem", "Lead").osc (1, analog, sawPos, 56).unison (1, 3, 14, 70, 0, 15)
            .osc (2, pulse, 35, 44).pitch (2, 0, 5)
            .osc (3, analog, sinePos, 40).fm (3, 4, 16).modulator (4, analog, sinePos).pitch (4, 12)
            .osc (5, analog, trianglePos, 24).pitch (5, 7).levelEnv (5, 220, 0, 1200, 55)
            .osc (6, string, 78, 30).levelEnv (6, 0, 0, 55, 0)
            .osc (7, analog, sawPos, 28).pitch (7, 12, -10).pan (7, -55).route (7, 100)
            .osc (8, analog, sawPos, 28).pitch (8, 12, 10).pan (8, 55).route (8, 100)
            .osc (9, additive, 70, 18).pitch (9, 12).route (9, 100)
            .parallel().filter (1, ladder24, 2400, 22, 14, 55).filter (2, svfHigh, 520, 14)
            .env (2, 0, 320, 28, 220, 65).mod (env2, "filter1Cutoff", 24).mod (env2, "osc3WarpAmount", 20)
            .lfoSync (1, sine, eighth, 0, 15).mod (lfo1, "filter1Cutoff", 20).mod (lfo1, "osc2Position", 40).mod (lfo1, "osc9Position", 35)
            .lfo (2, sine, 5.2f, 1).lfoFade (2, 450).mod (lfo2, "osc1Fine", 6).mod (lfo2, "osc3Fine", 6)
            .mod (velocity, "filter1Cutoff", 24).mod (velocity, "osc3WarpAmount", 28).mod (velocity, "osc6Volume", 32)
            .amp (4, 350, 88, 260).mono (38.0f, true).drive (tube, 14, 9500).chorus (22, 0.5f, 40).delay (eighthD, 36, 24, 4800, true).reverb (24, 72, 3.0f, 40).level (-6)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 36)
            .macro (2, "Shimmer").mod (macro2, "osc7Volume", 42)
            .macro (3, "Fifth").mod (macro3, "osc5Volume", 48)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Prism Euphoria: progressive-house / trance crystal festival hook lead.
        // 1 bright additive body; 2 silky sync lead; 3 crystal 1:3 FM chime driven by 4 (unheard);
        // 5 warm triangle foundation; 6 & 7 lush panned octave saws; 8 vowel air; 9 plucked sparkle.
        add ("Prism Euphoria", "Lead").osc (1, additive, 52, 52).unison (1, 2, 10, 65)
            .osc (2, sync, 28, 46).warp (2, wSync, 14)
            .osc (3, analog, sinePos, 38).pitch (3, 12).fm (3, 4, 22).levelEnv (3, 0, 0, 280, 25).modulator (4, analog, sinePos).pitch (4, 19)
            .osc (5, analog, trianglePos, 40)
            .osc (6, analog, sawPos, 26).pitch (6, 12, -8).pan (6, -60).route (6, 100)
            .osc (7, analog, sawPos, 26).pitch (7, 12, 8).pan (7, 60).route (7, 100)
            .osc (8, vowel, 30, 22).pitch (8, 12).route (8, 100)
            .osc (9, fm, 55, 24).pitch (9, 12).levelEnv (9, 0, 0, 32, 0).route (9, 100)
            .parallel().filter (1, svfLow, 2800, 18, 10, 55).filter (2, svfHigh, 600, 14)
            .env (2, 0, 260, 20, 180, 65).mod (env2, "filter1Cutoff", 24).mod (env2, "osc2WarpAmount", 28)
            .lfoSync (1, triangle, eighthD, 0, 15).mod (lfo1, "osc1Position", 38).mod (lfo1, "osc8Position", 42).mod (lfo1, "filter1Cutoff", 18)
            .lfo (2, sine, 5.6f, 1).lfoFade (2, 380).mod (lfo2, "osc2Fine", 6)
            .mod (velocity, "osc2WarpAmount", 32).mod (velocity, "filter1Cutoff", 22).mod (velocity, "osc3WarpAmount", 26)
            .amp (3, 300, 86, 280).chorus (28, 0.7f, 45).delay (eighthD, 38, 26, 5500, true).reverb (26, 78, 3.4f, 35).level (-3)
            .macro (1, "Bright").mod (macro1, "filter1Cutoff", 36)
            .macro (2, "Sync").mod (macro2, "osc2WarpAmount", 42)
            .macro (3, "Crystal").mod (macro3, "osc3Volume", 46)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 40);

        //----------------------------------------------------------------------
        // Stab

        // Nine. Palazzo Chord: classic deep/melodic-house minor-9th chord stab on a single key!
        // 1 root saw; 2 m3 (+3); 3 5th (+7); 4 m7 (+10); 5 9th (+14) additive glass;
        // 6 woody 1:2 FM attack driven by 7 (unheard); 8 & 9 wide stereo high-passed octave shimmer.
        add ("Palazzo Chord", "Stab").osc (1, analog, sawPos, 46)
            .osc (2, analog, sawPos, 38).pitch (2, 3, -4)
            .osc (3, pulse, 35, 38).pitch (3, 7, 4)
            .osc (4, analog, sawPos, 32).pitch (4, 10, -5)
            .osc (5, additive, 45, 28).pitch (5, 14, 5)
            .osc (6, analog, sinePos, 34).pitch (6, 12).fm (6, 7, 26).levelEnv (6, 0, 0, 95, 0).modulator (7, analog, sinePos).pitch (7, 24)
            .osc (8, analog, sawPos, 20).pitch (8, 12, -9).pan (8, -60).route (8, 100)
            .osc (9, analog, sawPos, 20).pitch (9, 15, 9).pan (9, 60).route (9, 100)
            .parallel().filter (1, ladder24, 920, 38, 18, 45).filter (2, svfHigh, 520, 14)
            .env (2, 0, 230, 0, 120, 72).mod (env2, "filter1Cutoff", 40).mod (env2, "osc6WarpAmount", 26)
            .lfoSync (1, sine, eighth, 0, 15).mod (lfo1, "filter1Cutoff", 18).mod (lfo1, "osc5Position", 40).mod (lfo1, "filter2Cutoff", 22)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc6Volume", 36).mod (velocity, "osc8Volume", 30)
            .amp (2, 360, 0, 240).chorus (20, 0.6f, 40).delay (eighthD, 46, 28, 3200, true).reverb (24, 72, 2.6f, 45).level (0)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Dub").mod (macro2, "fxDelayFeedback", 34)
            .macro (3, "Sparkle").mod (macro3, "osc5Volume", 46)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Mirage Dub: Berlin / Basic Channel warm resonant dub-techno chord stab.
        // 1 root warm FM driven by 5 (1:2); 2 m3 (+3) soft pulse; 3 5th (+7) triangle; 4 m7 (+10) vowel;
        // 6 Karplus string pluck; 7 & 8 stereo SVF Band dub shimmer; 9 airy top harmonic.
        add ("Mirage Dub", "Stab").osc (1, analog, sinePos, 48).fm (1, 5, 20).modulator (5, analog, sinePos).pitch (5, 12)
            .osc (2, pulse, 40, 38).pitch (2, 3, -5)
            .osc (3, analog, trianglePos, 36).pitch (3, 7, 5)
            .osc (4, vowel, 35, 32).pitch (4, 10)
            .osc (6, string, 72, 30).pitch (6, 12).levelEnv (6, 0, 0, 75, 0)
            .osc (7, analog, sawPos, 24).pitch (7, 12, -7).pan (7, -55).route (7, 100)
            .osc (8, additive, 55, 24).pitch (8, 19, 7).pan (8, 55).route (8, 100)
            .osc (9, fm, 35, 20).pitch (9, 24).levelEnv (9, 0, 0, 50, 0).route (9, 100)
            .parallel().filter (1, ladder12, 1050, 44, 16, 40).filter (2, svfBand, 1400, 35, 0, 40)
            .env (2, 0, 280, 0, 150, 70).mod (env2, "filter1Cutoff", 38).mod (env2, "filter2Cutoff", 28)
            .lfoSync (1, triangle, eighthD, 0, 15).mod (lfo1, "filter1Cutoff", 20).mod (lfo1, "osc4Position", 42).mod (lfo1, "filter2Cutoff", 28)
            .mod (velocity, "filter1Cutoff", 25).mod (velocity, "osc1WarpAmount", 30).mod (velocity, "osc6Volume", 32)
            .amp (2, 420, 0, 320).delay (eighthD, 52, 32, 2800, true).reverb (28, 82, 3.4f, 40).level (1)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Dub").mod (macro2, "fxDelayFeedback", 32)
            .macro (3, "Shimmer").mod (macro3, "filter2Cutoff", 36)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Chord

        // Nine. Velvet Horizon: lush Rüfüs Du Sol / Anjunadeep polyphonic club chord synth.
        // 1 warm detuned saws; 2 silky PWM pulse; 3 warm triangle body; 4 1:2 FM warmth driven by 5;
        // 6 additive bloom entering at 280ms; 7 & 8 wide stereo high-passed octave saws pulsing in 1/4-note
        // sidechain; 9 soft airy vowel halo.
        add ("Velvet Horizon", "Chord").osc (1, analog, sawPos, 40).unison (1, 3, 12, 75, 0, 20)
            .osc (2, pulse, 38, 36).pitch (2, 0, 5)
            .osc (3, analog, trianglePos, 38).pitch (3, -12)
            .osc (4, analog, sinePos, 32).fm (4, 5, 14).modulator (5, analog, sinePos).pitch (5, 12)
            .osc (6, additive, 42, 26).pitch (6, 12).levelEnv (6, 280, 0, 1600, 65)
            .osc (7, analog, sawPos, 24).pitch (7, 12, -8).pan (7, -60).route (7, 100)
            .osc (8, analog, sawPos, 24).pitch (8, 12, 8).pan (8, 60).route (8, 100)
            .osc (9, vowel, 45, 20).pitch (9, 12).route (9, 100)
            .parallel().filter (1, ladder24, 1500, 18, 12, 35).filter (2, svfHigh, 420, 12)
            .lfoSync (1, sawUp, quarter, 0, 30).mod (lfo1, "filter1Cutoff", 24).mod (lfo1, "osc7Volume", 36).mod (lfo1, "osc8Volume", 36)
            .lfoSync (2, sine, bars2, 0).mod (lfo2, "osc2Position", 38).mod (lfo2, "osc9Position", 36)
            .env (2, 10, 850, 25, 450, 60).mod (env2, "filter1Cutoff", 22)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc4WarpAmount", 26).mod (velocity, "osc6Volume", 34)
            .amp (12, 700, 84, 750).chorus (28, 0.5f, 45).delay (eighthD, 32, 18, 4200, true).reverb (28, 78, 3.2f, 40).level (-7.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Bloom").mod (macro2, "osc6Volume", 46)
            .macro (3, "Pulse").mod (macro3, "osc7Volume", 44)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Glass Pavilion: ethereal Four Tet / Bonobo crystal FM & plucked-string poly chord.
        // 1 silky 1:2 FM Rhodes core driven by 2; 3 crystal 1:3 bell halo driven by 4; 5 Karplus string;
        // 6 warm sub-octave triangle; 7 & 8 stereo high-passed additive fifth/octave shimmer alternating on 8ths;
        // 9 glassy top air.
        add ("Glass Pavilion", "Chord").osc (1, analog, sinePos, 46).fm (1, 2, 16).modulator (2, analog, sinePos).pitch (2, 12)
            .osc (3, analog, sinePos, 32).pitch (3, 12).fm (3, 4, 22).levelEnv (3, 0, 0, 550, 30).modulator (4, analog, sinePos).pitch (4, 19)
            .osc (5, string, 68, 32).pitch (5, 12).unison (5, 2, 8, 60, 12)
            .osc (6, analog, trianglePos, 34).pitch (6, -12)
            .osc (7, additive, 52, 24).pitch (7, 7).pan (7, -55).route (7, 100)
            .osc (8, additive, 62, 24).pitch (8, 12).pan (8, 55).route (8, 100)
            .osc (9, fm, 32, 20).pitch (9, 24).route (9, 100)
            .parallel().filter (1, ladder12, 2400, 16, 8, 40).filter (2, svfBand, 1800, 26, 0, 45)
            .lfoSync (1, sine, eighth, 0, 15).mod (lfo1, "osc7Volume", 36).mod (lfo1, "osc8Volume", -32).mod (lfo1, "filter2Cutoff", 30)
            .lfoSync (2, triangle, bars2, 0).mod (lfo2, "filter1Cutoff", 18).mod (lfo2, "osc1WarpAmount", 14)
            .mod (velocity, "osc1WarpAmount", 28).mod (velocity, "osc3WarpAmount", 30).mod (velocity, "filter1Cutoff", 22)
            .amp (6, 650, 80, 900).chorus (24, 0.6f, 40).delay (eighthD, 38, 22, 5200, true).reverb (32, 82, 3.8f, 35).level (-7)
            .macro (1, "Glass").mod (macro1, "osc3WarpAmount", 40)
            .macro (2, "Warmth").mod (macro2, "filter1Cutoff", -35)
            .macro (3, "Shimmer").mod (macro3, "filter2Cutoff", 38)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Pluck (One-shot: strikeToTail >= 3.0)

        // Nine. Strobe Pluck: legendary Deadmau5 / Prydz progressive-house supersaw pluck!
        // 1 tight unison saws; 2 warm pulse body; 3 pure triangle anchor; 4 crisp 1:2 FM pluck driven by 5;
        // 6 Karplus wooden string transient; 7 & 8 wide stereo high-passed octave supersaw sparkle (fast decay);
        // 9 airy top chime (fast decay).
        add ("Strobe Pluck", "Pluck").osc (1, analog, sawPos, 46).unison (1, 3, 15, 70)
            .osc (2, pulse, 35, 34)
            .osc (3, analog, trianglePos, 38).pitch (3, -12)
            .osc (4, analog, sinePos, 36).pitch (4, 12).fm (4, 5, 32).levelEnv (4, 0, 0, 110, 0).modulator (5, analog, sinePos).pitch (5, 24)
            .osc (6, string, 82, 34).levelEnv (6, 0, 0, 90, 0)
            .osc (7, analog, sawPos, 28).pitch (7, 12, -10).pan (7, -60).levelEnv (7, 0, 0, 140, 0).route (7, 100)
            .osc (8, analog, sawPos, 28).pitch (8, 12, 10).pan (8, 60).levelEnv (8, 0, 0, 140, 0).route (8, 100)
            .osc (9, additive, 88, 22).pitch (9, 24).levelEnv (9, 0, 0, 70, 0).route (9, 100)
            .parallel().filter (1, ladder24, 460, 26, 14, 50).filter (2, svfHigh, 500, 12)
            .env (2, 0, 140, 0, 90, 76).mod (env2, "filter1Cutoff", 48).mod (env2, "osc4WarpAmount", 32)
            .lfoSync (1, sine, bars2, 0).mod (lfo1, "osc2Position", 35)
            .mod (velocity, "filter1Cutoff", 28).mod (velocity, "osc4Volume", 35).mod (velocity, "osc6Position", 35)
            .amp (2, 540, 0, 380).delay (eighthD, 38, 22, 2100, true).reverb (20, 72, 2.4f, 52).level (-2.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 40)
            .macro (2, "Snap").mod (macro2, "osc4Volume", 48)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 40)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Marimba Glass: organic tropical/melodic-house glassy FM-marimba pluck.
        // 1 woody 1:2 FM core driven by 2 (unheard); 3 bright 1:4 glass overtone driven by 4 (fast decay);
        // 5 Karplus plucked string; 6 warm sine body; 7 & 8 panned octave/twelfth sparkle (fast decay);
        // 9 crisp mallet click.
        add ("Marimba Glass", "Pluck").osc (1, analog, sinePos, 54).fm (1, 2, 0).modulator (2, analog, sinePos).pitch (2, 12)
            .osc (3, analog, sinePos, 46).pitch (3, 12).fm (3, 4, 48).levelEnv (3, 0, 20, 75, 0).modulator (4, analog, sinePos).pitch (4, 24)
            .osc (5, string, 85, 42).levelEnv (5, 0, 15, 85, 0)
            .osc (6, analog, sinePos, 54).pitch (6, -12).levelEnv (6, 0, 0, 700, 0)
            .osc (7, additive, 88, 32).pitch (7, 19).pan (7, -50).levelEnv (7, 0, 15, 65, 0).route (7, 100)
            .osc (8, analog, sawPos, 28).pitch (8, 12).pan (8, 50).levelEnv (8, 0, 15, 70, 0).route (8, 100)
            .osc (9, metal, 24, 30).pitch (9, 24).levelEnv (9, 0, 0, 35, 0).route (9, 100)
            .parallel().filter (1, ladder24, 380, 18, 8, 55).filter (2, svfHigh, 750, 12)
            .env (2, 0, 90, 0, 60, 82).mod (env2, "filter1Cutoff", 55).mod (env2, "osc1WarpAmount", 62)
            .mod (velocity, "osc1WarpAmount", 36).mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc3Volume", 38)
            .amp (1.5f, 700, 0, 420).delay (eighthD, 28, 12, 1000, true).reverb (14, 70, 2.2f, 80).level (-5.0)
            .macro (1, "Wood").mod (macro1, "osc1WarpAmount", 42)
            .macro (2, "Glass").mod (macro2, "osc3Volume", 48)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 40)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Arp

        // Nine. Starlight Pulse: sparkling Prydz / melodic-techno 16th-note club arp voice.
        // 1 crisp analog saw; 2 warm PWM pulse; 3 woody 1:2 FM sparkle driven by 4; 5 Karplus string pick;
        // 6 clean sub-octave body; 7 & 8 wide stereo high-passed octave shimmer; 9 airy top sparkle.
        add ("Starlight Pulse", "Arp").osc (1, analog, sawPos, 48)
            .osc (2, pulse, 32, 40).pitch (2, 0, 6)
            .osc (3, analog, sinePos, 34).pitch (3, 12).fm (3, 4, 24).levelEnv (3, 0, 0, 150, 0).modulator (4, analog, sinePos).pitch (4, 24)
            .osc (5, string, 78, 30).levelEnv (5, 0, 0, 75, 0)
            .osc (6, analog, trianglePos, 30).pitch (6, -12)
            .osc (7, analog, sawPos, 24).pitch (7, 12, -8).pan (7, -55).route (7, 100)
            .osc (8, analog, sawPos, 24).pitch (8, 12, 8).pan (8, 55).route (8, 100)
            .osc (9, additive, 82, 18).pitch (9, 24).route (9, 100)
            .parallel().filter (1, ladder24, 1100, 30, 14, 50).filter (2, svfHigh, 480, 14)
            .env (2, 0, 180, 8, 100, 72).mod (env2, "filter1Cutoff", 38)
            .lfoSync (1, triangle, eighth, 0, 15).mod (lfo1, "filter1Cutoff", 22).mod (lfo1, "osc2Position", 44).mod (lfo1, "osc9Position", 36)
            .lfoSync (2, sine, bars2, 0).mod (lfo2, "filter2Cutoff", 24)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc3WarpAmount", 30).mod (velocity, "osc5Volume", 32)
            .amp (2, 240, 28, 140).delay (eighthD, 40, 26, 4800, true).reverb (20, 68, 2.2f, 40).level (0.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Sparkle").mod (macro2, "osc3Volume", 46)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 40)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Emerald Cascade: hypnotic progressive-trance / synthwave glassy arp.
        // 1 crystalline additive drawbars; 2 silky sync shimmer; 3 1:3 FM glass driven by 4;
        // 5 warm triangle; 6 & 7 panned stereo fifth/octave twins crossfading on 8ths; 8 vowel air; 9 pick.
        add ("Emerald Cascade", "Arp").osc (1, additive, 48, 48)
            .osc (2, sync, 24, 36).warp (2, wSync, 14)
            .osc (3, analog, sinePos, 32).pitch (3, 12).fm (3, 4, 20).modulator (4, analog, sinePos).pitch (4, 19)
            .osc (5, analog, trianglePos, 34).pitch (5, -12)
            .osc (6, pulse, 40, 24).pitch (6, 7).pan (6, -55).route (6, 100)
            .osc (7, analog, sawPos, 24).pitch (7, 12).pan (7, 55).route (7, 100)
            .osc (8, vowel, 35, 20).pitch (8, 12).route (8, 100)
            .osc (9, fm, 48, 22).pitch (9, 24).levelEnv (9, 0, 0, 28, 0).route (9, 100)
            .parallel().filter (1, svfLow, 1500, 24, 10, 50).filter (2, svfHigh, 520, 16)
            .env (2, 0, 210, 12, 120, 70).mod (env2, "filter1Cutoff", 32).mod (env2, "osc2WarpAmount", 26)
            .lfoSync (1, sine, eighth, 0, 15).mod (lfo1, "osc6Volume", 36).mod (lfo1, "osc7Volume", -34).mod (lfo1, "filter1Cutoff", 20)
            .lfoSync (2, triangle, bar1, 0).mod (lfo2, "osc1Position", 38)
            .mod (velocity, "filter1Cutoff", 24).mod (velocity, "osc2WarpAmount", 30).mod (velocity, "osc3WarpAmount", 26)
            .amp (2, 260, 32, 160).phaser (24, 0.35f, 38).delay (sixteenthD, 38, 24, 5200, true).reverb (22, 72, 2.6f, 38).level (0)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Sync").mod (macro2, "osc2WarpAmount", 42)
            .macro (3, "Shimmer").mod (macro3, "osc6Volume", 46)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 38);

        //----------------------------------------------------------------------
        // Keys

        // Nine. Lagoon Rhodes: lush deep-house / neo-soul crystal electric piano with silky stereo tremolo.
        // 1 warm 1:1/1:2 FM tine body (1 driven by 2); 3 crystal bell tine (3 driven by 4, fast decay);
        // 5 pure warm triangle body; 6 sub-octave warmth; 7 & 8 stereo high-passed shimmering tine twins
        // panning/breathing in silky 8th-note tremolo; 9 felt hammer transient.
        add ("Lagoon Rhodes", "Keys").osc (1, analog, sinePos, 56).fm (1, 2, 18).modulator (2, analog, sinePos).pitch (2, 12)
            .osc (3, analog, sinePos, 32).pitch (3, 12).fm (3, 4, 28).levelEnv (3, 0, 0, 140, 0).modulator (4, analog, sinePos).pitch (4, 24)
            .osc (5, analog, trianglePos, 38)
            .osc (6, analog, sinePos, 28).pitch (6, -12)
            .osc (7, additive, 34, 24).pitch (7, 12, -5).pan (7, -60).route (7, 100)
            .osc (8, additive, 34, 24).pitch (8, 12, 5).pan (8, 60).route (8, 100)
            .osc (9, string, 55, 24).levelEnv (9, 0, 0, 30, 0)
            .parallel().filter (1, ladder12, 2200, 14, 10, 45).filter (2, svfHigh, 480, 12)
            .env (2, 0, 380, 10, 180, 68).mod (env2, "osc1WarpAmount", 22).mod (env2, "filter1Cutoff", 20)
            .lfoSync (1, sine, eighth, 0, 15).mod (lfo1, "osc7Volume", 42).mod (lfo1, "osc8Volume", -42).mod (lfo1, "filter1Cutoff", 16)
            .lfoSync (2, triangle, bars2, 0).mod (lfo2, "osc7Position", 35)
            .mod (velocity, "osc1WarpAmount", 32).mod (velocity, "osc3Volume", 38).mod (velocity, "filter1Cutoff", 24)
            .amp (2, 1100, 34, 380).chorus (30, 0.65f, 45).delay (eighthD, 28, 16, 4200, true).reverb (24, 70, 2.4f, 42).level (-7)
            .macro (1, "Bark").mod (macro1, "osc1WarpAmount", 40)
            .macro (2, "Tine").mod (macro2, "osc3Volume", 48)
            .macro (3, "Tremolo").mod (macro3, "osc7Volume", 46)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Ivory Cassette: nostalgic Bicep / lo-fi club warm felt-DX hybrid keys with organic drift.
        // 1 warm triangle/sine fundamental; 2 gentle additive wood; 3 warm 1:2 FM driven by 4;
        // 5 Karplus felt-key attack; 6 octave-down warmth; 7 & 8 stereo drifting unison halo; 9 airy top.
        add ("Ivory Cassette", "Keys").osc (1, analog, trianglePos, 54)
            .osc (2, additive, 26, 40)
            .osc (3, analog, sinePos, 36).fm (3, 4, 14).modulator (4, analog, trianglePos).pitch (4, 12)
            .osc (5, string, 48, 28).levelEnv (5, 0, 0, 48, 0)
            .osc (6, analog, sinePos, 30).pitch (6, -12)
            .osc (7, analog, sawPos, 22).pitch (7, 12).unison (7, 3, 14, 90, 0, 48).route (7, 100)
            .osc (8, vowel, 50, 20).pitch (8, 12).pan (8, -40).route (8, 100)
            .osc (9, fm, 24, 20).pitch (9, 19).pan (9, 40).route (9, 100)
            .parallel().filter (1, ladder12, 1100, 18, 12, 40).filter (2, svfHigh, 420, 14)
            .env (2, 0, 420, 15, 220, 66).mod (env2, "filter1Cutoff", 22)
            .lfoSync (1, triangle, eighth, 0, 20).mod (lfo1, "filter1Cutoff", 20).mod (lfo1, "osc2Position", 38).mod (lfo1, "osc8Position", 40)
            .lfo (2, drift, 0.8f, 0).mod (lfo2, "osc1Fine", 8).mod (lfo2, "osc2Fine", -8)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc3WarpAmount", 30).mod (velocity, "osc5Volume", 34)
            .amp (3, 950, 32, 420).drive (soft, 14, 5800).chorus (22, 0.45f, 45).reverb (24, 76, 2.8f, 48).level (-6)
            .macro (1, "Dark").mod (macro1, "filter1Cutoff", -46)
            .macro (2, "Flutter").mod (macro2, "osc7Volume", 48)
            .macro (3, "Warmth").mod (macro3, "fxDriveAmount", 46)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Organ

        // Nine. Ibiza Organ: iconic 90s/modern UK House & Garage percussive M1-style club organ!
        // 1 16' sub sine; 2 8' warm fundamental; 3 4' octave; 4 2-2/3' fifth harmonic;
        // 5 percussive key-click FM driven by 6 (unheard, fast 60ms decay);
        // 7 & 8 stereo high-passed Leslie rotary additive twins alternating on 8ths; 9 2' sparkle.
        add ("Ibiza Organ", "Organ").osc (1, analog, sinePos, 54).pitch (1, -12)
            .osc (2, analog, sinePos, 58)
            .osc (3, analog, sinePos, 42).pitch (3, 12)
            .osc (4, analog, sinePos, 34).pitch (4, 19)
            .osc (5, analog, sinePos, 40).pitch (5, 12).fm (5, 6, 30).levelEnv (5, 0, 0, 95, 0).modulator (6, analog, sinePos).pitch (6, 24)
            .osc (7, additive, 32, 28).pitch (7, 12, -5).pan (7, -60).route (7, 100)
            .osc (8, additive, 42, 28).pitch (8, 19, 5).pan (8, 60).route (8, 100)
            .osc (9, analog, trianglePos, 24).pitch (9, 24).levelEnv (9, 0, 0, 160, 25).route (9, 100)
            .parallel().filter (1, ladder12, 2400, 16, 14, 40).filter (2, svfHigh, 420, 14)
            .env (2, 0, 160, 20, 80, 72).mod (env2, "filter1Cutoff", 26)
            .lfoSync (1, sine, eighth, 0, 10).mod (lfo1, "osc7Volume", 42).mod (lfo1, "osc8Volume", -42).mod (lfo1, "filter1Cutoff", 18)
            .lfoSync (2, triangle, bar1, 0).mod (lfo2, "osc7Position", 30)
            .mod (velocity, "osc5Volume", 48).mod (velocity, "osc7Position", 48).mod (velocity, "osc8Volume", 44)
            .amp (2, 280, 65, 95).drive (tube, 16, 7500).chorus (14, 1.4f, 40).reverb (16, 58, 1.6f, 45).level (-11.5)
            .macro (1, "Click").mod (macro1, "osc5Volume", 48)
            .macro (2, "Rotary").mod (macro2, "osc7Volume", 45)
            .macro (3, "Dark").mod (macro3, "filter1Cutoff", -48)
            .macro (4, "Drive").mod (macro4, "fxDriveAmount", 50);

        // Nine. Cathedral Glass: soulful progressive/melodic-techno harmonic pipe-glass organ.
        // 1 16' sub triangle; 2 8' additive drawbars; 3 4' silky FM pipe driven by 4 (1:2);
        // 5 2-2/3' warm fifth; 6 breathy vowel choir; 7 & 8 stereo high-passed shimmering pipes; 9 airy 2' gleam.
        add ("Cathedral Glass", "Organ").osc (1, analog, trianglePos, 44).pitch (1, -12)
            .osc (2, additive, 38, 52)
            .osc (3, analog, sinePos, 40).pitch (3, 12).fm (3, 4, 16).modulator (4, analog, sinePos).pitch (4, 24)
            .osc (5, analog, sinePos, 30).pitch (5, 19)
            .osc (6, vowel, 40, 28).pitch (6, 12)
            .osc (7, additive, 58, 24).pitch (7, 12, -7).pan (7, -55).route (7, 100)
            .osc (8, additive, 58, 24).pitch (8, 19, 7).pan (8, 55).route (8, 100)
            .osc (9, analog, sinePos, 20).pitch (9, 24).route (9, 100)
            .parallel().filter (1, ladder12, 2200, 14, 10, 35).filter (2, svfHigh, 480, 14)
            .lfoSync (1, sine, quarter, 0, 15).mod (lfo1, "filter1Cutoff", 22).mod (lfo1, "osc2Position", 36).mod (lfo1, "osc6Position", 42)
            .lfoSync (2, triangle, eighth, 0, 15).mod (lfo2, "osc7Volume", 36).mod (lfo2, "osc8Volume", -34)
            .mod (velocity, "filter1Cutoff", 24).mod (velocity, "osc3WarpAmount", 30).mod (velocity, "osc2Position", 32)
            .amp (10, 400, 92, 420).chorus (10, 0.55f, 35).reverb (22, 84, 3.8f, 40).level (-7.5)
            .macro (1, "Drawbars").mod (macro1, "osc2Position", 44)
            .macro (2, "Choir").mod (macro2, "osc6Volume", 48)
            .macro (3, "Cutoff").mod (macro3, "filter1Cutoff", 36)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Guitar

        // Nine. Balearic Nylon: warm Ibiza sunset-house acoustic-hybrid plucked guitar.
        // 1, 2, 3 triple Karplus-Strong plucked strings (slight detune + strum); 4 warm woody 1:2 FM body
        // driven by 5 (unheard); 6 octave-up harmonic string; 7 & 8 stereo high-passed sunset shimmer
        // breathing on 8ths; 9 delicate finger-nail harmonic.
        add ("Balearic Nylon", "Guitar").osc (1, string, 64, 52).unison (1, 2, 6, 40, 8)
            .osc (2, string, 54, 42).pitch (2, 0, 5).pan (2, -30).route (2, 50)
            .osc (3, string, 72, 40).pitch (3, 12, -5).pan (3, 30).route (3, 50)
            .osc (4, analog, sinePos, 36).fm (4, 5, 16).levelEnv (4, 0, 0, 380, 20).modulator (5, analog, sinePos).pitch (5, 12)
            .osc (6, analog, trianglePos, 32).levelEnv (6, 15, 0, 600, 30)
            .osc (7, additive, 45, 22).pitch (7, 12, -8).pan (7, -55).route (7, 100)
            .osc (8, additive, 45, 22).pitch (8, 19, 8).pan (8, 55).route (8, 100)
            .osc (9, fm, 32, 22).pitch (9, 24).levelEnv (9, 0, 0, 35, 0).route (9, 100)
            .parallel().filter (1, ladder12, 2600, 14, 10, 45).filter (2, svfHigh, 450, 14)
            .env (2, 0, 260, 15, 160, 70).mod (env2, "filter1Cutoff", 24)
            .lfoSync (1, sine, eighth, 0, 15).mod (lfo1, "filter1Cutoff", 20).mod (lfo1, "osc7Volume", 38).mod (lfo1, "osc8Volume", -34)
            .mod (velocity, "osc1Position", 38).mod (velocity, "filter1Cutoff", 24).mod (velocity, "osc4WarpAmount", 28)
            .amp (2, 750, 28, 360).chorus (18, 0.5f, 38).delay (eighthD, 38, 24, 4400, true).reverb (24, 76, 3.0f, 40).level (1)
            .macro (1, "Mellow").mod (macro1, "filter1Cutoff", -48)
            .macro (2, "Body").mod (macro2, "osc4Volume", 46)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 40)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Chrome Shoegaze: dreamy M83 / Bicep chorus-drenched synth-guitar wall.
        // 1 & 2 dual Karplus plucked strings; 3 lush detuned saw sustain blooming at 120ms;
        // 4 fifth harmonic bloom; 5 warm FM shimmer driven by 6 (1:2); 7 & 8 wide stereo high-passed
        // octave saws; 9 ethereal vowel wash.
        add ("Chrome Shoegaze", "Guitar").osc (1, string, 74, 48).unison (1, 2, 10, 0, 10)
            .osc (2, string, 62, 36).pitch (2, 12).levelEnv (2, 0, 0, 450, 0)
            .osc (3, analog, sawPos, 38).unison (3, 3, 16, 0).levelEnv (3, 120, 0, 1200, 75)
            .osc (4, pulse, 40, 26).pitch (4, 7).levelEnv (4, 180, 0, 1400, 60)
            .osc (5, analog, sinePos, 30).pitch (5, 12).fm (5, 6, 20).modulator (6, analog, sinePos).pitch (6, 19)
            .osc (7, analog, sawPos, 28).pitch (7, 12, -10).pan (7, -60).route (7, 100)
            .osc (8, analog, sawPos, 28).pitch (8, 12, 10).pan (8, 60).route (8, 100)
            .osc (9, vowel, 45, 22).pitch (9, 12).route (9, 100)
            .parallel().filter (1, ladder12, 2200, 18, 14, 40).filter (2, svfHigh, 480, 16)
            .lfoSync (1, triangle, eighth, 0, 15).mod (lfo1, "filter1Cutoff", 22).mod (lfo1, "osc9Position", 44).mod (lfo1, "filter2Cutoff", 24)
            .lfoSync (2, sine, bars2, 0).mod (lfo2, "osc4Position", 38)
            .mod (velocity, "osc1Position", 35).mod (velocity, "filter1Cutoff", 24).mod (velocity, "osc5WarpAmount", 30)
            .amp (3, 550, 76, 550).drive (tube, 18, 7200).chorus (12, 0.7f, 40).reverb (22, 82, 3.4f, 42).level (-2)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Bloom").mod (macro2, "osc3Volume", 46)
            .macro (3, "Wash").mod (macro3, "fxChorusMix", 42)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Brass

        // Nine. Vangelis Gold: iconic Blade Runner / progressive-house warm analog poly-brass!
        // 1 & 2 lush detuned saws; 3 silky PWM pulse; 4 warm sub-octave saw; 5 expressive 1:1 brassy FM
        // driven by 6 (swells via Env 2!); 7 & 8 wide stereo high-passed unison brass shimmer; 9 bright air.
        add ("Vangelis Gold", "Brass").osc (1, analog, sawPos, 46).unison (1, 3, 14, 65, 0, 22)
            .osc (2, analog, sawPos, 40).pitch (2, 0, 7)
            .osc (3, pulse, 38, 34).pitch (3, 0, -6)
            .osc (4, analog, sawPos, 32).pitch (4, -12)
            .osc (5, analog, sinePos, 38).fm (5, 6, 12).modulator (6, analog, sinePos)
            .osc (7, analog, sawPos, 24).pitch (7, 12, -10).pan (7, -55).route (7, 100)
            .osc (8, analog, sawPos, 24).pitch (8, 12, 10).pan (8, 55).route (8, 100)
            .osc (9, additive, 62, 18).pitch (9, 12).route (9, 100)
            .parallel().filter (1, ladder24, 900, 22, 16, 40).filter (2, svfHigh, 450, 14)
            .env (2, 110, 650, 48, 380, 45).mod (env2, "filter1Cutoff", 34).mod (env2, "osc5WarpAmount", 28)
            .lfoSync (1, sine, eighth, 0, 15).mod (lfo1, "filter1Cutoff", 20).mod (lfo1, "osc3Position", 40).mod (lfo1, "osc9Position", 36)
            .lfo (2, sine, 5.0f, 1).lfoFade (2, 500).mod (lfo2, "osc1Fine", 6)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc5WarpAmount", 28).mod (velocity, "osc7Volume", 32)
            .amp (55, 500, 88, 480).drive (tube, 14, 8500).chorus (24, 0.45f, 40).reverb (32, 84, 4.2f, 38).level (-5)
            .macro (1, "Swell").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Brass").mod (macro2, "osc5WarpAmount", 42)
            .macro (3, "Shimmer").mod (macro3, "osc7Volume", 44)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Solaria Horns: punchy French-house / Nu-Disco funk synth-brass section.
        // 1 & 2 snappy saws; 3 bright additive brass harmonics; 4 sync horn bite (fast decay);
        // 5 warm octave-down body; 6 1:2 FM punch driven by 7; 8 & 9 stereo SVF Band horn shimmer.
        add ("Solaria Horns", "Brass").osc (1, analog, sawPos, 48).pitch (1, 0, -5)
            .osc (2, analog, sawPos, 44).pitch (2, 0, 5)
            .osc (3, additive, 54, 40)
            .osc (4, sync, 30, 42).warp (4, wSync, 18).levelEnv (4, 15, 0, 320, 40)
            .osc (5, pulse, 30, 30).pitch (5, -12)
            .osc (6, analog, sinePos, 32).pitch (6, 7).fm (6, 7, 18).modulator (7, analog, sinePos).pitch (7, 19)
            .osc (8, analog, sawPos, 24).pitch (8, 12, -8).pan (8, -55).route (8, 100)
            .osc (9, additive, 68, 22).pitch (9, 12, 8).pan (9, 55).route (9, 100)
            .parallel().filter (1, ladder24, 1300, 26, 18, 45).filter (2, svfBand, 1800, 28, 0, 45)
            .env (2, 18, 280, 28, 160, 62).mod (env2, "filter1Cutoff", 32).mod (env2, "osc4WarpAmount", 28)
            .lfoSync (1, triangle, eighth, 0, 15).mod (lfo1, "filter1Cutoff", 20).mod (lfo1, "filter2Cutoff", 28).mod (lfo1, "osc3Position", 35)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc4WarpAmount", 32).mod (velocity, "osc6WarpAmount", 26)
            .amp (12, 340, 76, 180).drive (tube, 18, 9000).reverb (20, 62, 1.8f, 45).level (-6.0)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Bite").mod (macro2, "osc4Volume", 50)
            .macro (3, "Fifth").mod (macro3, "osc6Volume", 46)
            .macro (4, "Room").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Strings

        // Nine. Solina Velvet: classic Solina / Anjuna silky analog string ensemble.
        // 1 & 2 lush detuned saws across root & octave; 3 & 4 warm PWM pulses; 5 Karplus bow rosin;
        // 6 subtle 1:1 FM warmth driven by 7; 8 & 9 wide stereo high-passed air choir.
        add ("Solina Velvet", "Strings").osc (1, analog, sawPos, 40).unison (1, 3, 14, 75, 0, 20)
            .osc (2, analog, sawPos, 34).pitch (2, 12).unison (2, 3, 16, 85, 0, 25).route (2, 100)
            .osc (3, pulse, 30, 36).pitch (3, 0, -6)
            .osc (4, pulse, 65, 32).pitch (4, 12, 6).route (4, 100)
            .osc (5, string, 58, 22).levelEnv (5, 0, 0, 140, 0)
            .osc (6, analog, sinePos, 32).pitch (6, -12).fm (6, 7, 10).modulator (7, analog, trianglePos).pitch (7, -12)
            .osc (8, vowel, 42, 22).pitch (8, 12).pan (8, -50).route (8, 100)
            .osc (9, additive, 72, 18).pitch (9, 24).pan (9, 50).route (9, 100)
            .parallel().filter (1, ladder12, 1300, 14, 8, 35).filter (2, svfHigh, 480, 14)
            .lfoSync (1, sine, quarter, 0, 15).mod (lfo1, "osc3Position", 44).mod (lfo1, "osc4Position", -44).mod (lfo1, "filter1Cutoff", 22)
            .lfoSync (2, triangle, eighth, 0, 15).mod (lfo2, "filter2Cutoff", 24).mod (lfo2, "osc8Position", 36)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc2Volume", 34).mod (velocity, "osc6WarpAmount", 26)
            .amp (180, 600, 92, 950).chorus (34, 0.75f, 52).phaser (26, 0.25f, 40).reverb (32, 86, 4.2f, 38).level (-6.5)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", 44)
            .macro (2, "Ensemble").mod (macro2, "fxChorusMix", 42)
            .macro (3, "Air").mod (macro3, "osc9Volume", 48)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Nordic Chamber: emotional Max Cooper / Olafur Arnalds intimate hybrid club strings.
        // 1 warm cello additive; 2 violin octave triangle/saw; 3 fifth harmonic shimmer; 4 warm 1:2 FM
        // chamber resonance driven by 5; 6 Karplus bow strike; 7 & 8 stereo drifting high-passed strings
        // with delicate 8th-note bow-tremolo shimmer; 9 glassy top air.
        add ("Nordic Chamber", "Strings").osc (1, additive, 32, 46).unison (1, 2, 10, 60, 0, 35)
            .osc (2, analog, 50, 40).pitch (2, 12).unison (2, 3, 12, 85, 0, 40).route (2, 100)
            .osc (3, analog, trianglePos, 28).pitch (3, 7).levelEnv (3, 240, 0, 1400, 60)
            .osc (4, analog, sinePos, 34).pitch (4, -12).fm (4, 5, 14).modulator (5, analog, sinePos)
            .osc (6, string, 62, 26).levelEnv (6, 0, 0, 120, 0)
            .osc (7, analog, sawPos, 22).pitch (7, 12, -7).pan (7, -55).route (7, 100)
            .osc (8, vowel, 55, 22).pitch (8, 12, 7).pan (8, 55).route (8, 100)
            .osc (9, additive, 76, 16).pitch (9, 24).route (9, 100)
            .parallel().filter (1, svfLow, 1200, 16, 8, 40).filter (2, svfHigh, 450, 14)
            .lfoSync (1, sine, eighth, 0, 15).mod (lfo1, "osc7Volume", 36).mod (lfo1, "osc8Volume", -32).mod (lfo1, "filter1Cutoff", 20)
            .lfoSync (2, triangle, bars2, 0).mod (lfo2, "osc1Position", 36).mod (lfo2, "osc8Position", 38)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc4WarpAmount", 28).mod (velocity, "osc2Volume", 34)
            .amp (140, 700, 90, 1100).chorus (24, 0.4f, 40).reverb (36, 88, 4.8f, 34).level (-12)
            .macro (1, "Bright").mod (macro1, "filter1Cutoff", 44)
            .macro (2, "Tremolo").mod (macro2, "osc7Volume", 46)
            .macro (3, "Cello").mod (macro3, "osc4Volume", 48)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Pad

        // Nine. Elysian Bloom: pure Afterlife / Anjunadeep euphoric club breakdown pad!
        // 1 warm analog sine/tri foundation; 2 lush detuned saws; 3 silky PWM pulse; 4 angelic vowel choir;
        // 5 crystal 1:2 FM shimmer driven by 6; 7 & 8 wide stereo high-passed additive/saw halo blooming in
        // and pulsing gently in 1/4-note club sidechain; 9 ethereal top air.
        add ("Elysian Bloom", "Pad").osc (1, analog, trianglePos, 40).pitch (1, -12)
            .osc (2, analog, sawPos, 36).unison (2, 3, 16, 0, 0, 22)
            .osc (3, pulse, 35, 32).pitch (3, 0, 6)
            .osc (4, vowel, 28, 30).pitch (4, 12).levelEnv (4, 420, 0, 2000, 75)
            .osc (5, analog, sinePos, 28).pitch (5, 12).fm (5, 6, 16).modulator (6, analog, sinePos).pitch (6, 24)
            .osc (7, analog, sawPos, 26).pitch (7, 12, -9).pan (7, -60).levelEnv (7, 650, 0, 2000, 80).route (7, 100)
            .osc (8, additive, 58, 26).pitch (8, 19, 9).pan (8, 60).levelEnv (8, 900, 0, 2000, 70).route (8, 100)
            .osc (9, additive, 82, 18).pitch (9, 24).route (9, 100)
            .parallel().filter (1, ladder12, 1300, 18, 10, 35).filter (2, svfHigh, 480, 14)
            .lfoSync (1, sawUp, quarter, 0, 35).mod (lfo1, "filter1Cutoff", 24).mod (lfo1, "osc7Volume", 36).mod (lfo1, "osc8Volume", 36)
            .lfoSync (2, sine, bars4, 0).mod (lfo2, "osc4Position", 48).mod (lfo2, "osc3Position", 40)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc5WarpAmount", 30).mod (velocity, "osc8Volume", 34)
            .amp (420, 1000, 92, 1800).chorus (12, 0.45f, 38).delay (eighthD, 32, 16, 4800, false).reverb (22, 90, 5.2f, 38).level (-12.5)
            .macro (1, "Bloom").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Choir").mod (macro2, "osc4Volume", 48)
            .macro (3, "Halo").mod (macro3, "osc8Volume", 48)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Astral Glass: ethereal Jon Hopkins / Four Tet crystalline harmonic pad with sweeping SVF Notch.
        // 1 warm 1:2 FM glass core driven by 2; 3 crystal 1:3 ninth halo driven by 4; 5 warm fifth sine;
        // 6 delicate Karplus harp shimmer; 7 & 8 stereo high-passed additive/vowel voices through parallel SVF Notch;
        // 9 warm fundamental bed.
        add ("Astral Glass", "Pad").osc (1, analog, sinePos, 44).fm (1, 2, 16).modulator (2, analog, sinePos).pitch (2, 12)
            .osc (3, analog, sinePos, 30).pitch (3, 14).fm (3, 4, 20).levelEnv (3, 500, 0, 2200, 65).modulator (4, analog, sinePos).pitch (4, 19)
            .osc (5, analog, trianglePos, 32).pitch (5, 7)
            .osc (6, string, 68, 26).pitch (6, 12).unison (6, 2, 10, 70, 20).route (6, 100)
            .osc (7, additive, 52, 26).pitch (7, 12, -6).pan (7, -55).route (7, 100)
            .osc (8, vowel, 42, 26).pitch (8, 12, 6).pan (8, 55).route (8, 100)
            .osc (9, analog, sinePos, 36).pitch (9, -12)
            .parallel().filter (1, svfLow, 1900, 16, 8, 40).filter (2, svfBand, 1500, 32, 0, 45)
            .lfoSync (1, sine, eighthD, 0, 15).mod (lfo1, "osc7Volume", 38).mod (lfo1, "osc8Volume", -34).mod (lfo1, "filter2Cutoff", 34)
            .lfoSync (2, triangle, bars4, 0).mod (lfo2, "filter1Cutoff", 22).mod (lfo2, "osc8Position", 44)
            .mod (velocity, "osc1WarpAmount", 30).mod (velocity, "filter1Cutoff", 24).mod (velocity, "osc3Volume", 36)
            .amp (360, 900, 90, 2000).chorus (12, 0.35f, 36).delay (eighthD, 36, 18, 5500, false).reverb (24, 92, 5.6f, 36).level (-13)
            .macro (1, "Glass").mod (macro1, "osc1WarpAmount", 42)
            .macro (2, "Sweep").mod (macro2, "filter2Cutoff", 40)
            .macro (3, "Ninth").mod (macro3, "osc3Volume", 48)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Drone

        // Nine. Obsidian Tide: deep hypnotic Berlin-club tonal drone locked to 1/8-note subtle pulse.
        // 1 pure mono sub; 2 & 3 warm beating mono fifths; 4 warm 1:2 FM tide driven by 5;
        // 6 slow-breathing vowel choir; 7 & 8 stereo high-passed additive/saw shimmer; 9 airy overtone.
        add ("Obsidian Tide", "Drone").osc (1, analog, sinePos, 64).pitch (1, -12)
            .osc (2, analog, sawPos, 42).pitch (2, 0, -6)
            .osc (3, analog, trianglePos, 38).pitch (3, 7, 6)
            .osc (4, analog, sinePos, 36).fm (4, 5, 16).modulator (5, analog, sinePos).pitch (5, 12)
            .osc (6, vowel, 30, 32).pitch (6, 12)
            .osc (7, additive, 48, 26).pitch (7, 12, -8).pan (7, -55).route (7, 100)
            .osc (8, analog, sawPos, 24).pitch (8, 19, 8).pan (8, 55).route (8, 100)
            .osc (9, fm, 30, 18).pitch (9, 24).route (9, 100)
            .parallel().filter (1, ladder24, 620, 28, 16, 30).filter (2, svfHigh, 380, 18)
            .lfoSync (1, sine, eighth, 0, 15).mod (lfo1, "filter1Cutoff", 24).mod (lfo1, "osc4WarpAmount", 24).mod (lfo1, "osc7Volume", 34)
            .lfoSync (2, triangle, bars4, 0).mod (lfo2, "osc6Position", 48).mod (lfo2, "filter2Cutoff", 28)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc4WarpAmount", 30).mod (velocity, "osc8Volume", 34)
            .amp (600, 1200, 95, 2200).drive (tube, 14, 5500).delay (quarter, 34, 16, 3200, false).reverb (14, 86, 4.8f, 44).level (-9.5)
            .macro (1, "Tide").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Choir").mod (macro2, "osc6Volume", 48)
            .macro (3, "Shimmer").mod (macro3, "osc7Volume", 46)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Celestial Hum: warm Eno / Max Cooper harmonic sunrise drone with stacked pure intervals.
        // 1 sub fundamental; 2 root additive; 3 fifth triangle; 4 octave 1:2 FM glow driven by 5;
        // 6 ninth harmonic bloom; 7 & 8 stereo SVF Band sweeping overtones; 9 drifting crystal top.
        add ("Celestial Hum", "Drone").osc (1, analog, sinePos, 56).pitch (1, -12)
            .osc (2, additive, 24, 46)
            .osc (3, analog, trianglePos, 36).pitch (3, 7)
            .osc (4, analog, sinePos, 34).pitch (4, 12).fm (4, 5, 14).modulator (5, analog, sinePos).pitch (5, 19)
            .osc (6, analog, sinePos, 24).pitch (6, 14).levelEnv (6, 800, 0, 2000, 80)
            .osc (7, additive, 56, 28).pitch (7, 12, -5).pan (7, -55).route (7, 100)
            .osc (8, vowel, 50, 26).pitch (8, 19, 5).pan (8, 55).route (8, 100)
            .osc (9, analog, trianglePos, 20).pitch (9, 24).unison (9, 2, 12, 80, 0, 45).route (9, 100)
            .parallel().filter (1, ladder12, 1100, 18, 8, 30).filter (2, svfBand, 1200, 34, 0, 35)
            .lfoSync (1, triangle, quarter, 0, 15).mod (lfo1, "filter2Cutoff", 34).mod (lfo1, "osc2Position", 36).mod (lfo1, "osc7Volume", 36)
            .lfoSync (2, sine, bars4, 0).mod (lfo2, "filter1Cutoff", 22).mod (lfo2, "osc8Position", 44)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc4WarpAmount", 30).mod (velocity, "osc6Volume", 36)
            .amp (750, 1200, 96, 2600).reverb (7, 90, 5.8f, 45).level (-6.0)
            .macro (1, "Glow").mod (macro1, "osc2Position", 55)
            .macro (2, "Overtones").mod (macro2, "filter2Cutoff", 40)
            .macro (3, "Ninth").mod (macro3, "osc6Volume", 48)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Bell (One-shot: strikeToTail >= 3.0)

        // Nine. Crystal Celesta: glistening Four Tet / Anjuna glass celesta bell with staggered partial decays.
        // 1 pure warm fundamental; 2 harmonic 1:2 FM glass driven by 3 (medium decay);
        // 4 crystal 1:3 FM sparkle driven by 5 (short decay); 6 delicate metal chime (18% pos, short decay);
        // 7 & 8 stereo high-passed panned twelfth/double-octave sparkle (fast decay); 9 mallet strike.
        add ("Crystal Celesta", "Bell").osc (1, analog, sinePos, 54).levelEnv (1, 0, 0, 2400, 0)
            .osc (2, analog, sinePos, 40).pitch (2, 12).fm (2, 3, 24).levelEnv (2, 0, 0, 620, 0).modulator (3, analog, sinePos).pitch (3, 24)
            .osc (4, analog, sinePos, 32).pitch (4, 19).fm (4, 5, 30).levelEnv (4, 0, 0, 180, 0).modulator (5, analog, sinePos).pitch (5, 24)
            .osc (6, metal, 16, 24).pitch (6, 12).levelEnv (6, 0, 0, 140, 0).route (6, 100)
            .osc (7, additive, 72, 22).pitch (7, 19).pan (7, -55).levelEnv (7, 0, 0, 150, 0).route (7, 100)
            .osc (8, additive, 82, 22).pitch (8, 24).pan (8, 55).levelEnv (8, 0, 0, 120, 0).route (8, 100)
            .osc (9, string, 85, 24).pitch (9, 12).levelEnv (9, 0, 0, 40, 0).route (9, 100)
            .parallel().filter (1, ladder12, 1600, 14, 6, 55).filter (2, svfHigh, 700, 12)
            .env (2, 0, 160, 0, 120, 75).mod (env2, "filter1Cutoff", 38).mod (env2, "osc2WarpAmount", 28)
            .mod (velocity, "osc2WarpAmount", 34).mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc6Volume", 36)
            .amp (1.5f, 2200, 0, 1200).delay (eighthD, 40, 24, 6000, true).reverb (32, 82, 3.8f, 32).level (-2.0)
            .macro (1, "Sparkle").mod (macro1, "osc4Volume", 48)
            .macro (2, "Chime").mod (macro2, "osc6Volume", 48)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 40)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Kyoto Chime: lush progressive-house wooden/glass temple bell with hovering warm fifth tail.
        // 1 warm sine hum (-12); 2 prime warm triangle; 3 fifth resonant FM driven by 4 (1:2);
        // 5 Karplus wooden mallet attack; 6 glassy octave FM driven by 7 (fast decay);
        // 8 & 9 stereo airy temple shimmer (short decay).
        add ("Kyoto Chime", "Bell").osc (1, analog, sinePos, 46).pitch (1, -12).levelEnv (1, 0, 0, 2600, 0)
            .osc (2, analog, trianglePos, 44).levelEnv (2, 0, 0, 1800, 0)
            .osc (3, analog, sinePos, 36).pitch (3, 7).fm (3, 4, 22).levelEnv (3, 0, 0, 480, 0).modulator (4, analog, sinePos).pitch (4, 19)
            .osc (5, string, 72, 32).levelEnv (5, 0, 0, 65, 0)
            .osc (6, analog, sinePos, 30).pitch (6, 12).fm (6, 7, 32).levelEnv (6, 0, 0, 130, 0).modulator (7, analog, sinePos).pitch (7, 24)
            .osc (8, vowel, 35, 22).pitch (8, 12).pan (8, -50).levelEnv (8, 0, 0, 220, 0).route (8, 100)
            .osc (9, additive, 75, 22).pitch (9, 24).pan (9, 50).levelEnv (9, 0, 0, 110, 0).route (9, 100)
            .parallel().filter (1, ladder12, 1300, 18, 6, 50).filter (2, svfHigh, 600, 14)
            .env (2, 0, 180, 0, 140, 74).mod (env2, "filter1Cutoff", 38).mod (env2, "osc3WarpAmount", 26)
            .mod (velocity, "osc3WarpAmount", 32).mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc5Volume", 36)
            .amp (1.5f, 2400, 0, 1400).chorus (18, 0.4f, 35).delay (quarter, 34, 20, 4800, true).reverb (34, 86, 4.4f, 36).level (-0.5)
            .macro (1, "Wood").mod (macro1, "osc5Volume", 48)
            .macro (2, "Fifth").mod (macro2, "osc3Volume", 48)
            .macro (3, "Shimmer").mod (macro3, "osc9Volume", 48)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Sequence

        // Nine. Berlin Pulse: classic hypnotic Tale Of Us / Bodzin melodic-techno rolling sequence synth.
        // 1 punchy analog saw; 2 warm PWM pulse; 3 pure mono sub octave; 4 silky 1:2 FM bite driven by 5;
        // 6 Karplus crisp attack; 7 & 8 stereo high-passed octave shimmer; 9 airy top harmonic.
        add ("Berlin Pulse", "Sequence").osc (1, analog, sawPos, 54)
            .osc (2, pulse, 34, 42).pitch (2, 0, 5)
            .osc (3, analog, sinePos, 50).pitch (3, -12)
            .osc (4, analog, sinePos, 36).pitch (4, 12).fm (4, 5, 22).modulator (5, analog, sinePos).pitch (5, 24)
            .osc (6, string, 76, 28).pitch (6, 12).levelEnv (6, 0, 0, 45, 0)
            .osc (7, analog, sawPos, 24).pitch (7, 12, -9).pan (7, -55).route (7, 100)
            .osc (8, analog, sawPos, 24).pitch (8, 12, 9).pan (8, 55).route (8, 100)
            .osc (9, additive, 72, 18).pitch (9, 24).route (9, 100)
            .parallel().filter (1, ladder24, 760, 32, 18, 45).filter (2, svfHigh, 420, 14)
            .env (2, 0, 170, 8, 85, 72).mod (env2, "filter1Cutoff", 38).mod (env2, "osc4WarpAmount", 24)
            .lfoSync (1, steps, bar1, 0, 35).stepPattern (1, { 90, 20, -30, 70, -40, 85, -20, 40, 80, -30, 55, 95, -40, 30, 65, -20 })
            .mod (lfo1, "filter1Cutoff", 20).mod (lfo1, "osc7Volume", 32).mod (lfo1, "osc8Volume", 32)
            .lfoSync (2, triangle, bars2, 0).mod (lfo2, "osc2Position", 40)
            .mod (velocity, "filter1Cutoff", 24).mod (velocity, "osc4WarpAmount", 28)
            .amp (2, 220, 42, 90).mono (18.0f, true).drive (tube, 18, 7200).delay (eighthD, 38, 22, 4200, true).reverb (16, 62, 1.8f, 45).level (-6.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Bite").mod (macro2, "osc4WarpAmount", 42)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 40)
            .macro (4, "Warmth").mod (macro4, "fxDriveAmount", 48);

        // Nine. Kinetix Arp: driving progressive-trance / electro sequence voice with syncopated harmonic groove.
        // 1 tight crisp pulse; 2 liquid sync sweep; 3 clean sub-octave triangle; 4 1:3 FM glass driven by 5;
        // 6 sparkling additive harmonic; 7 & 8 stereo panned fifth/octave twins; 9 crisp mallet attack.
        add ("Kinetix Arp", "Sequence").osc (1, pulse, 28, 50)
            .osc (2, sync, 26, 40).warp (2, wSync, 16)
            .osc (3, analog, trianglePos, 46).pitch (3, -12)
            .osc (4, analog, sinePos, 32).pitch (4, 12).fm (4, 5, 22).modulator (5, analog, sinePos).pitch (5, 19)
            .osc (6, additive, 52, 28).pitch (6, 12)
            .osc (7, analog, sawPos, 22).pitch (7, 7).pan (7, -55).route (7, 100)
            .osc (8, analog, sawPos, 22).pitch (8, 12).pan (8, 55).route (8, 100)
            .osc (9, fm, 42, 24).pitch (9, 24).levelEnv (9, 0, 0, 26, 0).route (9, 100)
            .parallel().filter (1, svfLow, 980, 30, 14, 50).filter (2, svfHigh, 450, 16)
            .env (2, 0, 190, 5, 90, 74).mod (env2, "filter1Cutoff", 36).mod (env2, "osc2WarpAmount", 30)
            .lfoSync (1, steps, bar1, 0, 30).stepPattern (1, { 80, -30, 60, 95, -40, 50, 85, -20, 70, -40, 90, 20, -30, 75, 45, -35 })
            .mod (lfo1, "osc2WarpAmount", 24).mod (lfo1, "osc6Position", 42).mod (lfo1, "filter1Cutoff", 18)
            .mod (velocity, "filter1Cutoff", 24).mod (velocity, "osc4WarpAmount", 28).mod (velocity, "osc2WarpAmount", 26)
            .amp (2, 240, 38, 100).drive (soft, 16, 8000).delay (sixteenthD, 36, 20, 4800, false).reverb (12, 66, 1.8f, 45).level (-7)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Sync").mod (macro2, "osc2WarpAmount", 42)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 40)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Riser

        // Nine. Euphoria Lift: anthemic melodic-techno / progressive-house 4-bar rising build-up!
        // 1 & 2 lush detuned saws climbing cleanly in pitch over 4 bars; 3 sub anchor; 4 additive shimmer
        // climbing; 5 1:2 FM tension driven by 6; 7 & 8 wide stereo octave saws pulsing in 8th-note gate;
        // 9 bright air harmonic + pink noise sweep.
        add ("Euphoria Lift", "Riser").osc (1, analog, sawPos, 44).unison (1, 3, 18, 0)
            .osc (2, analog, sawPos, 36).pitch (2, 12).levelEnv (2, 1200, 0, 0, 100)
            .osc (3, analog, sinePos, 42).pitch (3, -12)
            .osc (4, additive, 35, 34).pitch (4, 12).levelEnv (4, 1800, 0, 0, 100)
            .osc (5, analog, sinePos, 32).fm (5, 6, 18).modulator (6, analog, sinePos).pitch (6, 12)
            .osc (7, analog, sawPos, 28).pitch (7, 12, -10).pan (7, -60).route (7, 100)
            .osc (8, analog, sawPos, 28).pitch (8, 19, 10).pan (8, 60).route (8, 100)
            .osc (9, vowel, 25, 24).pitch (9, 12).levelEnv (9, 2200, 0, 0, 100).route (9, 100)
            .noise (14, 1, 30)
            .parallel().filter (1, ladder24, 420, 32, 14, 30).filter (2, svfHigh, 500, 18)
            .env (2, 5500, 0, 100, 500, 25).mod (env2, "filter1Cutoff", 48).mod (env2, "osc1Pitch", 25.0f).mod (env2, "osc2Pitch", 25.0f).mod (env2, "noiseLevel", 34)
            .lfoSync (1, sawDown, eighth, 0, 20).mod (lfo1, "osc7Volume", 42).mod (lfo1, "osc8Volume", 42).mod (lfo1, "filter2Cutoff", 26)
            .mod (velocity, "filter1Cutoff", 24).mod (velocity, "osc5WarpAmount", 32)
            .amp (40, 1000, 100, 900).chorus (10, 0.5f, 35).delay (eighthD, 34, 18, 5200, false).reverb (20, 86, 4.2f, 38).level (-4.5)
            .macro (1, "Lift").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Air").mod (macro2, "noiseLevel", 48)
            .macro (3, "Pulse").mod (macro3, "osc7Volume", 46)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Aurora Swell: hypnotic trance / deep-house ascending harmonic choir riser.
        // 1 & 2 layered vowel choir opening across formants; 3 warm fifth triangle; 4 crystal 1:3 FM
        // driven by 5 (swells); 6 sub bed; 7 & 8 stereo high-passed additive/sync flutter on 16ths; 9 air.
        add ("Aurora Swell", "Riser").osc (1, vowel, 15, 44).unison (1, 2, 12, 0)
            .osc (2, vowel, 55, 36).pitch (2, 12).levelEnv (2, 1000, 0, 0, 100)
            .osc (3, analog, trianglePos, 34).pitch (3, 7).levelEnv (3, 1600, 0, 0, 100)
            .osc (4, analog, sinePos, 32).pitch (4, 12).fm (4, 5, 16).modulator (5, analog, sinePos).pitch (5, 19)
            .osc (6, analog, sinePos, 38).pitch (6, -12)
            .osc (7, additive, 40, 28).pitch (7, 12).pan (7, -55).route (7, 100)
            .osc (8, sync, 25, 26).pitch (8, 12).pan (8, 55).warp (8, wSync, 15).route (8, 100)
            .osc (9, additive, 80, 20).pitch (9, 24).levelEnv (9, 2400, 0, 0, 100).route (9, 100)
            .parallel().filter (1, ladder12, 550, 24, 10, 35).filter (2, svfHigh, 520, 16)
            .env (2, 5200, 0, 100, 600, 20).mod (env2, "filter1Cutoff", 46).mod (env2, "osc1Position", 55).mod (env2, "osc4WarpAmount", 36).mod (env2, "osc8Pitch", 25.0f)
            .lfoSync (1, sine, sixteenth, 0, 15).mod (lfo1, "osc7Volume", 38).mod (lfo1, "osc8WarpAmount", 30).mod (lfo1, "filter2Cutoff", 24)
            .mod (velocity, "filter1Cutoff", 24).mod (velocity, "osc4WarpAmount", 30)
            .amp (60, 1000, 100, 1000).chorus (10, 0.6f, 35).delay (eighthD, 34, 18, 5000, false).reverb (22, 88, 4.6f, 38).level (-3)
            .macro (1, "Open").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Choir").mod (macro2, "osc1Position", 44)
            .macro (3, "Flutter").mod (macro3, "osc7Volume", 46)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Impact (One-shot: strikeToTail >= 3.0, below200 >= 40%)

        // Nine. Horizon Drop: massive clean melodic-techno drop impact with shimmering glass chord burst.
        // 1 huge pitch-dropping mono sub boom; 2 warm saturated sub tail; 3 warm fifth boom;
        // 4 crystal 1:2 FM chord chime driven by 5 (fast decay); 6 Karplus slam transient;
        // 7 & 8 stereo high-passed shimmering burst (fast decay); 9 airy top wash (fast decay).
        add ("Horizon Drop", "Impact").osc (1, analog, sinePos, 84).levelEnv (1, 0, 120, 1800, 0)
            .osc (2, sub, 38, 56).levelEnv (2, 0, 80, 1500, 0)
            .osc (3, analog, trianglePos, 38).pitch (3, 7).levelEnv (3, 0, 0, 480, 0)
            .osc (4, analog, sinePos, 40).pitch (4, 12).fm (4, 5, 34).levelEnv (4, 0, 0, 220, 0).modulator (5, analog, sinePos).pitch (5, 19)
            .osc (6, string, 80, 32).pitch (6, 12).levelEnv (6, 0, 0, 80, 0)
            .osc (7, analog, sawPos, 32).pitch (7, 12, -9).pan (7, -60).levelEnv (7, 0, 0, 260, 0).route (7, 100)
            .osc (8, additive, 82, 32).pitch (8, 19, 9).pan (8, 60).levelEnv (8, 0, 0, 280, 0).route (8, 100)
            .osc (9, vowel, 40, 22).pitch (9, 24).levelEnv (9, 0, 0, 220, 0).route (9, 100)
            .parallel().filter (1, ladder24, 280, 20, 18, 10).filter (2, svfHigh, 450, 12)
            .env (2, 0, 220, 0, 120, 76).mod (env2, "filter1Cutoff", 46).mod (env2, "osc4WarpAmount", 30)
            .env (3, 0, 260, 0, 120, 74).mod (env3, "osc1Pitch", 25.0f).mod (env3, "osc2Pitch", 25.0f)
            .mod (velocity, "filter1Cutoff", 30).mod (velocity, "osc4Volume", 42).mod (velocity, "osc7Volume", 42)
            .amp (2, 2200, 0, 950).drive (soft, 16, 5500).reverb (14, 82, 3.8f, 45).level (-6)
            .macro (1, "Boom").mod (macro1, "osc2Volume", 48)
            .macro (2, "Burst").mod (macro2, "osc4Volume", 55)
            .macro (3, "Shimmer").mod (macro3, "osc8Volume", 55)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Velvet Subdrop: deep progressive-house velvety pitch-drop boom with woody FM punch.
        // 1 pure mono sub sine; 2 warm triangle drop; 3 woody 1:2 FM punch driven by 4 (fast decay);
        // 5 warm octave body; 6 noise puff; 7 & 8 stereo high-passed air wash (fast decay); 9 crisp strike.
        add ("Velvet Subdrop", "Impact").osc (1, analog, sinePos, 84).levelEnv (1, 0, 100, 1600, 0)
            .osc (2, analog, trianglePos, 52).levelEnv (2, 0, 60, 1200, 0)
            .osc (3, analog, sinePos, 46).fm (3, 4, 30).levelEnv (3, 0, 0, 260, 0).modulator (4, analog, sinePos).pitch (4, 12)
            .osc (5, pulse, 35, 38).pitch (5, 12).levelEnv (5, 0, 0, 280, 0)
            .osc (6, additive, 70, 26).pitch (6, 24).levelEnv (6, 0, 0, 160, 0).route (6, 100)
            .osc (7, analog, sawPos, 26).pitch (7, 12, -8).pan (7, -55).levelEnv (7, 0, 0, 240, 0).route (7, 100)
            .osc (8, analog, sawPos, 26).pitch (8, 12, 8).pan (8, 55).levelEnv (8, 0, 0, 240, 0).route (8, 100)
            .osc (9, fm, 50, 26).pitch (9, 24).levelEnv (9, 0, 0, 60, 0).route (9, 100)
            .noise (14, 1, 20)
            .parallel().filter (1, ladder24, 240, 18, 16, 10).filter (2, svfHigh, 480, 12)
            .env (2, 0, 200, 0, 100, 76).mod (env2, "filter1Cutoff", 44).mod (env2, "noiseLevel", 34)
            .env (3, 0, 340, 0, 140, 72).mod (env3, "osc1Pitch", 20.8f).mod (env3, "osc2Pitch", 20.8f)
            .mod (velocity, "osc3Volume", 48).mod (velocity, "osc7Volume", 48).mod (velocity, "noiseLevel", 48)
            .amp (2, 1900, 0, 850).drive (tube, 14, 4800).reverb (12, 78, 3.2f, 48).level (-5.0)
            .macro (1, "Drive").mod (macro1, "fxDriveAmount", 65)
            .macro (2, "Shimmer").mod (macro2, "osc7Volume", 60)
            .macro (3, "Air").mod (macro3, "noiseLevel", 48)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Cinematic

        // Nine. Overture Glass: Zimmer meets Anyma widescreen club-intro brass/strings/glass hybrid.
        // 1 deep cello/sub anchor; 2 lush brassy saw unison; 3 warm fifth pulse; 4 glassy 1:2 FM shimmer
        // driven by 5; 6 soaring additive choir blooming at 500ms; 7 & 8 wide stereo pulsing octave shimmer;
        // 9 delicate plucked glass sparkle.
        add ("Overture Glass", "Cinematic").osc (1, sub, 28, 52).pitch (1, -12)
            .osc (2, analog, sawPos, 44).unison (2, 3, 16, 0, 0, 20)
            .osc (3, pulse, 38, 34).pitch (3, 7)
            .osc (4, analog, sinePos, 32).pitch (4, 12).fm (4, 5, 18).modulator (5, analog, sinePos).pitch (5, 24)
            .osc (6, additive, 52, 30).pitch (6, 12).levelEnv (6, 520, 0, 2000, 80)
            .osc (7, analog, sawPos, 28).pitch (7, 12, -9).pan (7, -60).route (7, 100)
            .osc (8, vowel, 42, 26).pitch (8, 19, 9).pan (8, 60).route (8, 100)
            .osc (9, string, 72, 22).pitch (9, 12).levelEnv (9, 0, 0, 180, 0).route (9, 100)
            .parallel().filter (1, ladder24, 900, 20, 14, 30).filter (2, svfHigh, 450, 14)
            .env (2, 380, 1200, 55, 700, 40).mod (env2, "filter1Cutoff", 30)
            .lfoSync (1, sine, eighth, 0, 15).mod (lfo1, "osc7Volume", 36).mod (lfo1, "osc8Volume", 36).mod (lfo1, "filter1Cutoff", 18)
            .lfoSync (2, triangle, bars4, 0).mod (lfo2, "osc6Position", 40).mod (lfo2, "osc8Position", 44)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc4WarpAmount", 30).mod (velocity, "osc6Volume", 34)
            .amp (260, 1000, 94, 1800).delay (eighthD, 34, 10, 4400, false).reverb (7, 90, 5.0f, 44).level (-8.0)
            .macro (1, "Swell").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Choir").mod (macro2, "osc6Volume", 48)
            .macro (3, "Pulse").mod (macro3, "osc7Volume", 46)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Solaris Brass: emotional sci-fi / melodic-techno widescreen brass & vowel swell.
        // 1 deep saw sub; 2 & 3 warm wide brassy pulses (+/-7 ct); 4 ethereal vowel choir; 5 1:1 brass FM
        // driven by 6; 7 & 8 stereo high-passed fifth/octave shimmer; 9 airy top drawbars.
        add ("Solaris Brass", "Cinematic").osc (1, analog, sawPos, 46).pitch (1, -12)
            .osc (2, pulse, 30, 44).pitch (2, 0, -7)
            .osc (3, pulse, 65, 44).pitch (3, 0, 7)
            .osc (4, vowel, 25, 34).pitch (4, 12)
            .osc (5, analog, sinePos, 36).fm (5, 6, 14).modulator (6, analog, sinePos)
            .osc (7, analog, sawPos, 28).pitch (7, 7, -8).pan (7, -55).route (7, 100)
            .osc (8, additive, 58, 28).pitch (8, 12, 8).pan (8, 55).route (8, 100)
            .osc (9, additive, 82, 18).pitch (9, 24).route (9, 100)
            .parallel().filter (1, ladder24, 780, 22, 16, 30).filter (2, svfHigh, 420, 14)
            .env (2, 240, 950, 50, 600, 42).mod (env2, "filter1Cutoff", 32).mod (env2, "osc5WarpAmount", 26)
            .lfoSync (1, triangle, quarter, 0, 15).mod (lfo1, "filter1Cutoff", 22).mod (lfo1, "osc4Position", 44).mod (lfo1, "osc2Position", 38)
            .lfoSync (2, sine, bars4, 0).mod (lfo2, "filter2Cutoff", 24)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc5WarpAmount", 30).mod (velocity, "osc7Volume", 34)
            .amp (200, 900, 92, 1600).drive (tube, 14, 7000).reverb (14, 90, 4.8f, 42).level (-7.5)
            .macro (1, "Swell").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Choir").mod (macro2, "osc4Volume", 48)
            .macro (3, "Brass").mod (macro3, "osc5WarpAmount", 42)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Perc (One-shot: strikeToTail >= 3.0)

        // Nine. Kalimba Wood: organic Afro-House / Deep-House melodic wooden kalimba-marimba perc.
        // 1 warm woody 1:2 FM tone driven by 2; 3 Karplus plucked tine; 4 hollow fifth body;
        // 5 resonant octave sine; 6 crisp wood mallet click (14ms); 7 & 8 stereo high-passed tine sparkle
        // (fast decay); 9 sub-octave thumb warmth.
        add ("Kalimba Wood", "Perc").osc (1, analog, sinePos, 56).fm (1, 2, 22).levelEnv (1, 0, 0, 340, 0).modulator (2, analog, sinePos).pitch (2, 12)
            .osc (3, string, 76, 42).levelEnv (3, 0, 0, 140, 0)
            .osc (4, analog, trianglePos, 32).pitch (4, 7).levelEnv (4, 0, 0, 180, 0)
            .osc (5, analog, sinePos, 34).pitch (5, 12).levelEnv (5, 0, 0, 110, 0)
            .osc (6, fm, 52, 28).pitch (6, 24).levelEnv (6, 0, 0, 16, 0).route (6, 100)
            .osc (7, additive, 68, 22).pitch (7, 19).pan (7, -50).levelEnv (7, 0, 0, 80, 0).route (7, 100)
            .osc (8, metal, 14, 20).pitch (8, 24).pan (8, 50).levelEnv (8, 0, 0, 65, 0).route (8, 100)
            .osc (9, analog, sinePos, 32).pitch (9, -12).levelEnv (9, 0, 0, 260, 0)
            .parallel().filter (1, ladder12, 1200, 18, 10, 55).filter (2, svfHigh, 650, 14)
            .env (2, 0, 110, 0, 70, 78).mod (env2, "filter1Cutoff", 40).mod (env2, "osc1WarpAmount", 30)
            .mod (velocity, "osc1WarpAmount", 34).mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc6Volume", 36)
            .amp (1.5f, 440, 0, 260).delay (eighthD, 34, 20, 4800, true).reverb (20, 62, 1.8f, 44).level (-2.5)
            .macro (1, "Wood").mod (macro1, "osc1WarpAmount", 42)
            .macro (2, "Tine").mod (macro2, "osc3Volume", 48)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 40)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Bongo Glass: playable tribal/tech-house pitched glassy conga-bongo.
        // 1 pitch-enveloped skin sine; 2 warm triangle shell; 3 hollow 1:3 FM shell driven by 4;
        // 5 fifth harmonic ring (short); 6 palm slap transient (14ms); 7 & 8 stereo rim/air snap (short);
        // 9 Karplus woody knock.
        add ("Bongo Glass", "Perc").osc (1, analog, sinePos, 62).levelEnv (1, 0, 0, 320, 0)
            .osc (2, analog, trianglePos, 40).levelEnv (2, 0, 0, 240, 0)
            .osc (3, analog, sinePos, 36).pitch (3, 7).fm (3, 4, 28).levelEnv (3, 0, 0, 110, 0).modulator (4, analog, sinePos).pitch (4, 19)
            .osc (5, analog, sinePos, 28).pitch (5, 12).levelEnv (5, 0, 0, 95, 0)
            .osc (6, pulse, 65, 28).pitch (6, 24).levelEnv (6, 0, 0, 14, 0).route (6, 100)
            .osc (7, additive, 75, 22).pitch (7, 19).pan (7, -45).levelEnv (7, 0, 0, 60, 0).route (7, 100)
            .osc (8, vowel, 35, 22).pitch (8, 12).pan (8, 45).levelEnv (8, 0, 0, 85, 0).route (8, 100)
            .osc (9, string, 72, 26).levelEnv (9, 0, 0, 55, 0)
            .parallel().filter (1, ladder12, 1050, 22, 12, 50).filter (2, svfHigh, 600, 14)
            .env (2, 0, 95, 0, 60, 78).mod (env2, "filter1Cutoff", 40)
            .env (3, 0, 70, 0, 40, 76).mod (env3, "osc1Pitch", 12.5f).mod (env3, "osc2Pitch", 12.5f)
            .mod (velocity, "osc3WarpAmount", 34).mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc6Volume", 36)
            .amp (1.5f, 380, 0, 220).drive (soft, 14, 7000).reverb (18, 56, 1.5f, 45).level (-1.5)
            .macro (1, "Pitch").mod (macro1, "osc3Volume", 48)
            .macro (2, "Wood").mod (macro2, "osc9Volume", 52)
            .macro (3, "Slap").mod (macro3, "osc6Volume", 48)
            .macro (4, "Room").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Drum (One-shot: strikeToTail >= 3.0)

        // Nine. Club Punch: tight, warm melodic-techno / electro pitched synth-tom/snare hybrid.
        // 1 snappy pitch-dropping sine body; 2 warm saturated chest; 3 tuned fifth body; 4 woody 1:2 FM
        // punch driven by 5 (fast decay); 6 crisp stick snap (12ms); 7 & 8 stereo high-passed airy brush
        // snap (110ms); 9 Karplus membrane transient + tight noise burst.
        add ("Club Punch", "Drum").osc (1, analog, sinePos, 66).levelEnv (1, 0, 0, 320, 0)
            .osc (2, sub, 40, 44).levelEnv (2, 0, 0, 240, 0)
            .osc (3, analog, trianglePos, 32).pitch (3, 7).levelEnv (3, 0, 0, 160, 0)
            .osc (4, analog, sinePos, 36).pitch (4, 12).fm (4, 5, 34).levelEnv (4, 0, 0, 70, 0).modulator (5, analog, sinePos).pitch (5, 24)
            .osc (6, pulse, 55, 28).pitch (6, 24).levelEnv (6, 0, 0, 12, 0).route (6, 100)
            .osc (7, additive, 82, 24).pitch (7, 19).pan (7, -45).levelEnv (7, 0, 0, 95, 0).route (7, 100)
            .osc (8, vowel, 60, 22).pitch (8, 12).pan (8, 45).levelEnv (8, 0, 0, 110, 0).route (8, 100)
            .osc (9, string, 80, 24).pitch (9, 12).levelEnv (9, 0, 0, 45, 0)
            .noise (20, 1, 25)
            .parallel().filter (1, ladder24, 900, 18, 16, 20).filter (2, svfHigh, 550, 14)
            .env (2, 0, 110, 0, 60, 78).mod (env2, "filter1Cutoff", 42).mod (env2, "noiseLevel", 38)
            .env (3, 0, 85, 0, 40, 76).mod (env3, "osc1Pitch", 16.67f).mod (env3, "osc2Pitch", 16.67f)
            .mod (velocity, "filter1Cutoff", 28).mod (velocity, "osc4WarpAmount", 34).mod (velocity, "noiseLevel", 36)
            .amp (1.5f, 440, 0, 260).drive (tube, 18, 7200).reverb (14, 48, 1.2f, 50).level (-3)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", 40)
            .macro (2, "Snap").mod (macro2, "noiseLevel", 48)
            .macro (3, "Punch").mod (macro3, "osc4Volume", 48)
            .macro (4, "Room").mod (macro4, "fxReverbMix", 38);

        // Nine. Electro Rim: crisp 808/909-meets-FM pitched electro rimshot / clap-tom.
        // 1 woody pitch-dropped sine; 2 hollow square resonance; 3 1:3 FM rim click driven by 4 (45ms);
        // 5 bright twelfth woody overtone (80ms); 6 crisp tick (10ms); 7 & 8 stereo high-passed clap air
        // (130ms); 9 Karplus woody rim knock.
        add ("Electro Rim", "Drum").osc (1, analog, sinePos, 56).levelEnv (1, 0, 0, 260, 0)
            .osc (2, analog, squarePos, 34).pitch (2, 12).levelEnv (2, 0, 0, 110, 0)
            .osc (3, analog, sinePos, 42).pitch (3, 12).fm (3, 4, 38).levelEnv (3, 0, 0, 48, 0).modulator (4, analog, sinePos).pitch (4, 19)
            .osc (5, analog, trianglePos, 30).pitch (5, 19).levelEnv (5, 0, 0, 85, 0)
            .osc (6, fm, 68, 28).pitch (6, 24).levelEnv (6, 0, 0, 12, 0).route (6, 100)
            .osc (7, additive, 86, 24).pitch (7, 19).pan (7, -50).levelEnv (7, 0, 0, 120, 0).route (7, 100)
            .osc (8, metal, 20, 20).pitch (8, 24).pan (8, 50).levelEnv (8, 0, 0, 90, 0).route (8, 100)
            .osc (9, string, 82, 28).pitch (9, 12).levelEnv (9, 0, 0, 40, 0)
            .noise (18, 0, 35)
            .parallel().filter (1, ladder12, 1300, 22, 14, 30).filter (2, svfHigh, 620, 16)
            .env (2, 0, 95, 0, 50, 80).mod (env2, "filter1Cutoff", 40).mod (env2, "noiseLevel", 36)
            .env (3, 0, 60, 0, 30, 78).mod (env3, "osc1Pitch", 12.5f)
            .mod (velocity, "osc3WarpAmount", 36).mod (velocity, "filter1Cutoff", 26).mod (velocity, "noiseLevel", 36)
            .amp (1.5f, 380, 0, 220).drive (soft, 18, 8200).reverb (16, 52, 1.4f, 48).level (-2.0)
            .macro (1, "Rim").mod (macro1, "osc3Volume", 48)
            .macro (2, "Snap").mod (macro2, "noiseLevel", 48)
            .macro (3, "Tone").mod (macro3, "filter1Cutoff", 38)
            .macro (4, "Room").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // FX

        // Nine. Comet Shimmer: gorgeous progressive-house crystalline cascading ear-candy sweep.
        // 1 warm sine foundation; 2 glassy 1:3 FM sparkle driven by 3 (sweeping down cleanly);
        // 4 airy vowel sweep; 5 fifth harmonic halo; 6 Karplus sparkle; 7 & 8 stereo high-passed
        // additive shimmer fluttering smoothly on 16ths; 9 ethereal top chime.
        add ("Comet Shimmer", "FX").osc (1, analog, sinePos, 46).pitch (1, -12)
            .osc (2, analog, sinePos, 40).pitch (2, 12).fm (2, 3, 28).modulator (3, analog, sinePos).pitch (3, 19)
            .osc (4, vowel, 15, 34).pitch (4, 12)
            .osc (5, analog, trianglePos, 32).pitch (5, 7)
            .osc (6, string, 82, 26).pitch (6, 12).levelEnv (6, 0, 0, 240, 0)
            .osc (7, additive, 62, 26).pitch (7, 12, -8).pan (7, -60).route (7, 100)
            .osc (8, additive, 76, 26).pitch (8, 24, 8).pan (8, 60).route (8, 100)
            .osc (9, fm, 55, 22).pitch (9, 19).route (9, 100)
            .parallel().filter (1, svfLow, 1300, 24, 8, 35).filter (2, svfBand, 2200, 30, 0, 40)
            .env (2, 0, 850, 15, 450, 64).mod (env2, "osc2Pitch", 25.0f).mod (env2, "osc4Position", 55).mod (env2, "filter1Cutoff", 28)
            .lfoSync (1, sine, sixteenth, 0, 15).mod (lfo1, "osc7Volume", 42).mod (lfo1, "osc8Volume", -38).mod (lfo1, "filter2Cutoff", 34).mod (lfo1, "filter1Cutoff", 22)
            .mod (velocity, "osc2WarpAmount", 32).mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc6Volume", 36)
            .amp (6, 1200, 45, 950).delay (eighthD, 42, 22, 5600, false).reverb (20, 84, 3.8f, 38).level (-2)
            .macro (1, "Sweep").mod (macro1, "filter1Cutoff", 45)
            .macro (2, "Flutter").mod (macro2, "osc7Volume", 46)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 40)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Liquid Sonar: hypnotic minimal/deep-house dub sonar ping with underwater resonant halo.
        // 1 pure resonant sine ping; 2 glassy 1:2 FM overtone driven by 3; 4 warm sub bed;
        // 5 fifth harmonic halo; 6 soft vowel underwater wash; 7 & 8 stereo SVF Band dub echoes
        // breathing on dotted 8ths; 9 delicate droplet click.
        add ("Liquid Sonar", "FX").osc (1, analog, sinePos, 54)
            .osc (2, analog, sinePos, 38).pitch (2, 12).fm (2, 3, 24).modulator (3, analog, sinePos).pitch (3, 24)
            .osc (4, analog, sinePos, 42).pitch (4, -12)
            .osc (5, analog, trianglePos, 30).pitch (5, 7)
            .osc (6, vowel, 45, 28).pitch (6, 12)
            .osc (7, additive, 50, 24).pitch (7, 12, -6).pan (7, -55).route (7, 100)
            .osc (8, additive, 68, 24).pitch (8, 19, 6).pan (8, 55).route (8, 100)
            .osc (9, string, 75, 24).pitch (9, 24).levelEnv (9, 0, 0, 50, 0).route (9, 100)
            .parallel().filter (1, ladder24, 1500, 38, 10, 40).filter (2, svfBand, 1400, 36, 0, 45)
            .env (2, 0, 450, 15, 300, 68).mod (env2, "filter1Cutoff", 30).mod (env2, "osc2WarpAmount", 28)
            .lfoSync (1, sine, eighthD, 0, 15).mod (lfo1, "filter2Cutoff", 34).mod (lfo1, "osc6Position", 44).mod (lfo1, "filter1Cutoff", 20)
            .mod (velocity, "osc2WarpAmount", 32).mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc9Volume", 36)
            .amp (4, 950, 50, 850).phaser (28, 0.3f, 45).delay (eighthD, 52, 32, 3800, true).reverb (34, 86, 4.2f, 38).level (0.5)
            .macro (1, "Ping").mod (macro1, "osc2WarpAmount", 42)
            .macro (2, "Depth").mod (macro2, "filter1Cutoff", 38)
            .macro (3, "Dub").mod (macro3, "fxDelayFeedback", 34)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Glitch

        // Nine. Crystal Stutter: musical Four Tet / Flume sparkling micro-rhythmic synth!
        // 1 warm sine/tri foundation; 2 glassy 1:2 FM pluck driven by 3; 4 Karplus harp sparkle;
        // 5 warm octave pulse; 6 musical fifth bell; 7 & 8 stereo high-passed additive/vowel micro-chops
        // dancing cleanly on a musical 16th groove (smooth=25!); 9 high droplet sparkle.
        add ("Crystal Stutter", "Glitch").osc (1, analog, trianglePos, 46)
            .osc (2, analog, sinePos, 40).pitch (2, 12).fm (2, 3, 24).modulator (3, analog, sinePos).pitch (3, 24)
            .osc (4, string, 80, 32).pitch (4, 12)
            .osc (5, pulse, 35, 30).pitch (5, -12)
            .osc (6, analog, sinePos, 28).pitch (6, 7)
            .osc (7, additive, 62, 28).pitch (7, 12).pan (7, -60).route (7, 100)
            .osc (8, vowel, 35, 28).pitch (8, 19).pan (8, 60).route (8, 100)
            .osc (9, fm, 52, 24).pitch (9, 24).route (9, 100)
            .parallel().filter (1, svfLow, 1400, 20, 8, 45).filter (2, svfHigh, 550, 16)
            .lfoSync (1, steps, bar1, 0, 25).stepPattern (1, { 90, -80, 70, 95, -90, 60, -80, 90, 40, -90, 85, -70, 95, -60, 80, -85 })
            .mod (lfo1, "osc7Volume", 42).mod (lfo1, "osc8Volume", -40).mod (lfo1, "osc2WarpAmount", 26).mod (lfo1, "filter1Cutoff", 18)
            .lfoSync (2, triangle, quarter, 0, 15).mod (lfo2, "osc8Position", 44).mod (lfo2, "osc9Position", 36)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc2WarpAmount", 30).mod (velocity, "osc4Position", 36)
            .amp (2, 380, 64, 180).delay (sixteenth, 42, 24, 5800, true).reverb (22, 70, 2.2f, 38).level (-8.0)
            .macro (1, "Tone").mod (macro1, "filter1Cutoff", 45)
            .macro (2, "Chop").mod (macro2, "osc7Volume", 48)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 40)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Prism Flicker: playable Overmono / UKG melodic micro-chop synth.
        // 1 warm Rhodes 1:2 FM core driven by 2; 3 crisp additive octave; 4 warm sub-octave sine;
        // 5 vowel chop layer; 6 silky sync overtone; 7 & 8 stereo high-passed panned harmonic flickers
        // trading cleanly across a syncopated 1/2-bar step groove; 9 glass pick.
        add ("Prism Flicker", "Glitch").osc (1, analog, sinePos, 50).fm (1, 2, 20).modulator (2, analog, sinePos).pitch (2, 12)
            .osc (3, additive, 45, 36).pitch (3, 12)
            .osc (4, analog, sinePos, 38).pitch (4, -12)
            .osc (5, vowel, 25, 32).pitch (5, 12)
            .osc (6, sync, 25, 26).warp (6, wSync, 14)
            .osc (7, pulse, 40, 26).pitch (7, 12, -7).pan (7, -55).route (7, 100)
            .osc (8, analog, sawPos, 26).pitch (8, 19, 7).pan (8, 55).route (8, 100)
            .osc (9, string, 78, 24).pitch (9, 12).levelEnv (9, 0, 0, 45, 0).route (9, 100)
            .parallel().filter (1, ladder12, 2400, 22, 10, 45).filter (2, svfHigh, 500, 16)
            .lfoSync (1, steps, half, 0, 25).stepPattern (1, { 95, -75, 40, 90, -85, 75, -90, 85, 60, -80, 95, -60, 70, -90, 85, -50 })
            .mod (lfo1, "osc5Position", 48).mod (lfo1, "osc7Volume", 40).mod (lfo1, "osc8Volume", -36).mod (lfo1, "osc6WarpAmount", 26)
            .lfoSync (2, sine, bar1, 0).mod (lfo2, "filter1Cutoff", 18)
            .mod (velocity, "osc1WarpAmount", 30).mod (velocity, "filter1Cutoff", 24).mod (velocity, "osc6WarpAmount", 28)
            .amp (2, 340, 62, 160).delay (sixteenthD, 40, 24, 5200, true).reverb (20, 68, 2.0f, 40).level (-4.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Flicker").mod (macro2, "osc7Volume", 46)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 40)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Game

        // Nine. Hyperlight Lead: lush Disasterpeace / synthwave playable adventure synth!
        // 1 warm singing pulse; 2 lush detuned saw; 3 glassy 1:2 FM bell attack driven by 4;
        // 5 warm octave-down triangle; 6 shimmering fifth harmonic; 7 & 8 stereo high-passed nostalgic
        // shimmer breathing on 8ths; 9 airy additive sparkle.
        add ("Hyperlight Lead", "Game").osc (1, pulse, 32, 48).unison (1, 2, 10, 60, 0, 18)
            .osc (2, analog, sawPos, 42).pitch (2, 0, 6)
            .osc (3, analog, sinePos, 34).pitch (3, 12).fm (3, 4, 24).levelEnv (3, 0, 0, 240, 20).modulator (4, analog, sinePos).pitch (4, 24)
            .osc (5, analog, trianglePos, 36).pitch (5, -12)
            .osc (6, analog, trianglePos, 24).pitch (6, 7)
            .osc (7, pulse, 60, 24).pitch (7, 12, -8).pan (7, -55).route (7, 100)
            .osc (8, analog, sawPos, 24).pitch (8, 12, 8).pan (8, 55).route (8, 100)
            .osc (9, additive, 76, 18).pitch (9, 24).route (9, 100)
            .parallel().filter (1, ladder24, 1900, 20, 12, 45).filter (2, svfHigh, 480, 14)
            .env (2, 0, 320, 22, 180, 68).mod (env2, "filter1Cutoff", 28)
            .lfoSync (1, sine, eighth, 0, 15).mod (lfo1, "osc1Position", 42).mod (lfo1, "filter1Cutoff", 20).mod (lfo1, "osc7Volume", 34)
            .lfo (2, sine, 5.4f, 1).lfoFade (2, 420).mod (lfo2, "osc1Fine", 6)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc3WarpAmount", 30).mod (velocity, "osc9Volume", 36)
            .amp (3, 420, 78, 320).chorus (28, 0.6f, 45).delay (eighthD, 38, 22, 4800, true).reverb (28, 78, 3.2f, 38).level (-2.5)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Bell").mod (macro2, "osc3Volume", 46)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 40)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Starfield Quest: vibrant modern electronic / arcade heroic poly-synth.
        // 1 bright additive chime; 2 warm analog brass saw; 3 1:3 FM sparkle driven by 4;
        // 5 Karplus crisp pluck attack; 6 clean sub-octave; 7 & 8 wide stereo high-passed fifth/octave
        // air pulsing on 8ths; 9 glassy top.
        add ("Starfield Quest", "Game").osc (1, additive, 36, 48)
            .osc (2, analog, sawPos, 42).unison (2, 2, 12, 0)
            .osc (3, analog, sinePos, 32).pitch (3, 12).fm (3, 4, 22).modulator (4, analog, sinePos).pitch (4, 19)
            .osc (5, string, 78, 30).levelEnv (5, 0, 0, 80, 0)
            .osc (6, analog, sinePos, 34).pitch (6, -12)
            .osc (7, analog, sawPos, 26).pitch (7, 7, -7).pan (7, -55).route (7, 100)
            .osc (8, pulse, 35, 26).pitch (8, 12, 7).pan (8, 55).route (8, 100)
            .osc (9, fm, 44, 20).pitch (9, 24).route (9, 100)
            .parallel().filter (1, svfLow, 1150, 18, 10, 45).filter (2, svfHigh, 500, 14)
            .env (2, 0, 280, 20, 160, 68).mod (env2, "filter1Cutoff", 28)
            .lfoSync (1, triangle, eighth, 0, 15).mod (lfo1, "osc1Position", 38).mod (lfo1, "osc7Volume", 36).mod (lfo1, "osc8Volume", -32)
            .lfoSync (2, sine, bars2, 0).mod (lfo2, "filter1Cutoff", 18)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc3WarpAmount", 30).mod (velocity, "osc5Volume", 34)
            .amp (3, 380, 76, 300).chorus (12, 0.55f, 36).delay (eighthD, 34, 20, 5200, false).reverb (18, 76, 2.6f, 40).level (-3.5)
            .macro (1, "Harmonics").mod (macro1, "osc1Position", 58)
            .macro (2, "Heroic").mod (macro2, "osc7Volume", 46)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 40)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Chip

        // Nine. Arcade Blossom: Porter Robinson / dream-pop lush chiptune-club poly!
        // 1 crisp 25% pulse; 2 silky triangle core; 3 glassy 1:2 FM sparkle driven by 4;
        // 5 clean sub-octave triangle; 6 octave pulse with subtle musical octave-blip shimmer;
        // 7 & 8 wide stereo detuned octave halo; 9 sparkling additive top.
        add ("Arcade Blossom", "Chip").osc (1, pulse, 45, 48)
            .osc (2, analog, trianglePos, 44)
            .osc (3, analog, sinePos, 32).pitch (3, 12).fm (3, 4, 22).modulator (4, analog, squarePos).pitch (4, 12)
            .osc (5, analog, trianglePos, 34).pitch (5, -12)
            .osc (6, pulse, 25, 28).pitch (6, 12)
            .osc (7, pulse, 55, 22).pitch (7, 12, -8).pan (7, -55).route (7, 100)
            .osc (8, analog, sawPos, 22).pitch (8, 12, 8).pan (8, 55).route (8, 100)
            .osc (9, additive, 78, 18).pitch (9, 24).route (9, 100)
            .parallel().filter (1, ladder12, 3200, 18, 8, 45).filter (2, svfHigh, 550, 14)
            .env (2, 0, 240, 15, 140, 70).mod (env2, "filter1Cutoff", 28)
            .lfoSync (1, steps, quarter, 0, 20).stepPattern (1, { 0, 0, 50, 50, 100, 100, 50, 50, 0, 0, 50, 50, 100, 100, 0, 0 })
            .mod (lfo1, "osc1Position", 44).mod (lfo1, "osc6Position", -40).mod (lfo1, "filter1Cutoff", 20)
            .lfoSync (2, sine, eighth, 0, 15).mod (lfo2, "osc7Volume", 34).mod (lfo2, "osc8Volume", -32)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc3WarpAmount", 30).mod (velocity, "osc9Volume", 36)
            .amp (2, 340, 72, 240).chorus (24, 0.6f, 40).delay (eighthD, 36, 22, 5600, true).reverb (24, 74, 2.6f, 36).level (-2)
            .macro (1, "Duty").mod (macro1, "osc1Position", 44)
            .macro (2, "Sparkle").mod (macro2, "osc3WarpAmount", 42)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 40)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Pixel Horizon: punchy synthwave / electro-house chiptune poly with NES triangle warmth.
        // 1 singing square/pulse lead; 2 warm NES triangle bass layer; 3 fifth chip pulse;
        // 4 1:2 FM arcade pluck driven by 5; 6 octave crisp pulse; 7 & 8 stereo high-passed shimmer;
        // 9 crisp mallet attack.
        add ("Pixel Horizon", "Chip").osc (1, pulse, 20, 50)
            .osc (2, analog, trianglePos, 46).pitch (2, -12)
            .osc (3, pulse, 65, 28).pitch (3, 7)
            .osc (4, analog, sinePos, 34).pitch (4, 12).fm (4, 5, 26).levelEnv (4, 0, 0, 180, 15).modulator (5, analog, squarePos).pitch (5, 24)
            .osc (6, analog, squarePos, 28).pitch (6, 12)
            .osc (7, additive, 58, 22).pitch (7, 12, -7).pan (7, -55).route (7, 100)
            .osc (8, pulse, 45, 22).pitch (8, 12, 7).pan (8, 55).route (8, 100)
            .osc (9, string, 82, 24).pitch (9, 12).levelEnv (9, 0, 0, 38, 0).route (9, 100)
            .parallel().filter (1, ladder24, 2400, 26, 12, 45).filter (2, svfHigh, 520, 14)
            .env (2, 0, 210, 12, 120, 72).mod (env2, "filter1Cutoff", 30)
            .lfoSync (1, steps, bar1, 0, 22).stepPattern (1, { 80, 20, -40, 80, 40, -60, 90, 20, 60, -40, 95, 30, -50, 75, 40, -30 })
            .mod (lfo1, "osc1Position", 46).mod (lfo1, "filter1Cutoff", 22).mod (lfo1, "osc6Position", -36)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc4WarpAmount", 30).mod (velocity, "osc9Volume", 36)
            .amp (2, 300, 68, 180).delay (sixteenthD, 36, 20, 5400, true).reverb (20, 66, 2.0f, 40).level (-2)
            .macro (1, "Cutoff").mod (macro1, "filter1Cutoff", 38)
            .macro (2, "Duty").mod (macro2, "osc1Position", 44)
            .macro (3, "Fifth").mod (macro3, "osc3Volume", 48)
            .macro (4, "Echo").mod (macro4, "fxDelayMix", 40);

        //----------------------------------------------------------------------
        // Texture

        // Nine. Velvet Mist: lush Anjunadeep / Bonobo warm organic atmospheric bed.
        // 1 warm sine foundation; 2 velvety detuned triangle choir; 3 distant vowel breath;
        // 4 crystal 1:2 FM mist driven by 5; 6 delicate Karplus harp sparkles; 7 & 8 stereo SVF Band
        // harmonic mist sweeping softly on 1/4-note & 4-bar arcs; 9 airy top + warm pink breeze.
        add ("Velvet Mist", "Texture").osc (1, analog, sinePos, 48).pitch (1, -12)
            .osc (2, analog, trianglePos, 42).unison (2, 3, 18, 0, 0, 35)
            .osc (3, vowel, 30, 34).pitch (3, 12)
            .osc (4, analog, sinePos, 30).pitch (4, 12).fm (4, 5, 16).modulator (5, analog, sinePos).pitch (5, 19)
            .osc (6, string, 65, 24).pitch (6, 12).unison (6, 2, 12, 70, 25).route (6, 100)
            .osc (7, additive, 48, 28).pitch (7, 7).pan (7, -55).route (7, 100)
            .osc (8, additive, 62, 28).pitch (8, 12).pan (8, 55).route (8, 100)
            .osc (9, vowel, 65, 20).pitch (9, 24).route (9, 100)
            .noise (12, 0, 75)
            .parallel().filter (1, svfLow, 1100, 16, 6, 35).filter (2, svfBand, 1400, 30, 0, 40)
            .lfoSync (1, sine, quarter, 0, 15).mod (lfo1, "filter2Cutoff", 36).mod (lfo1, "osc7Volume", 36).mod (lfo1, "osc8Volume", -34)
            .lfoSync (2, triangle, bars4, 0).mod (lfo2, "osc3Position", 48).mod (lfo2, "filter1Cutoff", 22).mod (lfo2, "noiseLevel", 20)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc4WarpAmount", 30).mod (velocity, "noiseLevel", 30)
            .amp (650, 1200, 94, 2400).delay (eighthD, 34, 12, 4800, false).reverb (7, 90, 5.4f, 42).level (-4.5)
            .macro (1, "Mist").mod (macro1, "filter1Cutoff", 46)
            .macro (2, "Breeze").mod (macro2, "noiseLevel", 48)
            .macro (3, "Shimmer").mod (macro3, "filter2Cutoff", 40)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Rainlight Halo: ethereal Max Cooper / Rival Consoles glistening harmonic rain texture.
        // 1 warm drifting analog sine bed; 2 soft fifth triangle; 3 delicate 1:3 FM droplets driven by 4;
        // 5 Karplus glass rain; 6 soft octave vowel; 7 & 8 stereo high-passed shimmering droplets dancing
        // smoothly on 16th-note groove; 9 crystal double-octave air.
        add ("Rainlight Halo", "Texture").osc (1, analog, sinePos, 46).pitch (1, -12)
            .osc (2, analog, trianglePos, 38).pitch (2, 7).unison (2, 2, 12, 0, 0, 42)
            .osc (3, analog, sinePos, 32).pitch (3, 12).fm (3, 4, 22).modulator (4, analog, sinePos).pitch (4, 19)
            .osc (5, string, 76, 26).pitch (5, 12).unison (5, 2, 10, 80, 18).route (5, 100)
            .osc (6, vowel, 45, 26).pitch (6, 12)
            .osc (7, additive, 68, 28).pitch (7, 19).pan (7, -60).route (7, 100)
            .osc (8, fm, 44, 28).pitch (8, 24).pan (8, 60).route (8, 100)
            .osc (9, additive, 85, 18).pitch (9, 24).route (9, 100)
            .parallel().filter (1, ladder12, 1050, 18, 6, 40).filter (2, svfHigh, 580, 16)
            .lfoSync (1, steps, bar1, 0, 28).stepPattern (1, { 80, -60, 40, 95, -75, 50, -80, 90, 20, -70, 85, -40, 95, -65, 60, -80 })
            .mod (lfo1, "osc7Volume", 40).mod (lfo1, "osc8Volume", -38).mod (lfo1, "osc3WarpAmount", 24)
            .lfoSync (2, sine, bars4, 0).mod (lfo2, "filter1Cutoff", 22).mod (lfo2, "osc6Position", 44)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc3WarpAmount", 30).mod (velocity, "osc7Volume", 34)
            .amp (520, 1000, 92, 2200).delay (eighthD, 36, 12, 5600, false).reverb (7, 90, 5.2f, 40).level (-13.0)
            .macro (1, "Glass").mod (macro1, "osc3WarpAmount", 55)
            .macro (2, "Rain").mod (macro2, "osc7Volume", 48)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 40)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        //----------------------------------------------------------------------
        // Vox

        // Nine. Angelic Chop: modern Fred again.. / Bicep lush playable vocal-chop club synth!
        // 1 & 2 smooth vowel formants across A-E-O; 3 octave vowel air; 4 warm chest triangle;
        // 5 silky 1:2 FM breath sheen driven by 6; 7 & 8 wide stereo high-passed vocal choir twins;
        // 9 crisp consonant attack.
        add ("Angelic Chop", "Vox").osc (1, vowel, 18, 52)
            .osc (2, vowel, 58, 42).pitch (2, 0, 6)
            .osc (3, vowel, 38, 32).pitch (3, 12)
            .osc (4, analog, trianglePos, 38).pitch (4, -12)
            .osc (5, analog, sinePos, 30).pitch (5, 12).fm (5, 6, 16).modulator (6, analog, sinePos).pitch (6, 24)
            .osc (7, vowel, 25, 26).pitch (7, 12, -8).pan (7, -55).route (7, 100)
            .osc (8, vowel, 75, 26).pitch (8, 12, 8).pan (8, 55).route (8, 100)
            .osc (9, string, 72, 22).pitch (9, 12).levelEnv (9, 0, 0, 28, 0).route (9, 100)
            .parallel().filter (1, ladder12, 2800, 16, 8, 40).filter (2, svfHigh, 520, 14)
            .env (2, 0, 280, 20, 160, 68).mod (env2, "filter1Cutoff", 24)
            .lfoSync (1, sine, eighth, 0, 15).mod (lfo1, "osc1Position", 44).mod (lfo1, "osc2Position", -40).mod (lfo1, "filter1Cutoff", 18)
            .lfo (2, sine, 5.2f, 1).lfoFade (2, 420).mod (lfo2, "osc1Fine", 5).mod (lfo2, "osc2Fine", 5)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc5WarpAmount", 30).mod (velocity, "osc9Volume", 36)
            .amp (8, 420, 82, 320).chorus (26, 0.6f, 44).delay (eighthD, 38, 24, 4800, true).reverb (30, 80, 3.4f, 38).level (-1)
            .macro (1, "Vowel").mod (macro1, "osc1Position", 44)
            .macro (2, "Choir").mod (macro2, "osc7Volume", 48)
            .macro (3, "Echo").mod (macro3, "fxDelayMix", 40)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);

        // Nine. Aurelia Choir: transcendent melodic-techno / trance cathedral vocal pad-lead.
        // 1, 2, 3 lush multi-voice vowel choir; 4 warm cello-octave support; 5 fifth harmonic halo;
        // 6 crystal 1:2 FM angelic glow driven by 7; 8 & 9 stereo high-passed ethereal octave choir & air.
        add ("Aurelia Choir", "Vox").osc (1, vowel, 22, 46).unison (1, 2, 10, 65, 0, 22)
            .osc (2, vowel, 52, 40).pitch (2, 0, -6)
            .osc (3, vowel, 78, 34).pitch (3, 12, 6)
            .osc (4, analog, sawPos, 32).pitch (4, -12)
            .osc (5, analog, trianglePos, 26).pitch (5, 7)
            .osc (6, analog, sinePos, 28).pitch (6, 12).fm (6, 7, 16).modulator (7, analog, sinePos).pitch (7, 24)
            .osc (8, vowel, 40, 26).pitch (8, 12, -9).pan (8, -60).route (8, 100)
            .osc (9, additive, 74, 20).pitch (9, 24, 9).pan (9, 60).route (9, 100)
            .parallel().filter (1, svfLow, 2400, 16, 6, 35).filter (2, svfHigh, 500, 14)
            .lfoSync (1, triangle, quarter, 0, 15).mod (lfo1, "osc1Position", 44).mod (lfo1, "osc2Position", -40).mod (lfo1, "filter1Cutoff", 22)
            .lfo (2, sine, 4.8f, 1).lfoFade (2, 550).mod (lfo2, "osc1Fine", 5).mod (lfo2, "osc3Fine", 5)
            .mod (velocity, "filter1Cutoff", 26).mod (velocity, "osc6WarpAmount", 30).mod (velocity, "osc9Volume", 36)
            .amp (120, 700, 92, 950).chorus (30, 0.45f, 46).delay (quarter, 32, 18, 4400, true).reverb (40, 90, 5.4f, 34).level (-3)
            .macro (1, "Formant").mod (macro1, "osc1Position", 44)
            .macro (2, "Halo").mod (macro2, "osc9Volume", 48)
            .macro (3, "Warmth").mod (macro3, "osc4Volume", 46)
            .macro (4, "Space").mod (macro4, "fxReverbMix", 38);
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
