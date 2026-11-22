#pragma once

#include "WaveSynth.h"

// What Torqwave Synth's factory patches are written with: a patch is the settings it changes from
// the defaults, set through a Builder by name (the hand-made patches in WaveSynthPatches.cpp, the
// bank in WaveSynthPatchBank.cpp).

namespace WaveSynthPatchBuilder
{
    using namespace WaveSynth;

    enum T { analog, pulse, vowel, fm, metal, sub, additive, sync, growl, string };
    enum W { wOff, wSync, wBend, wSqueeze, wMirror, wFm, wCrush, wFold };
    enum F { off, ladder24, ladder12, ladderHigh, ladderBand, svfLow, svfBand, svfHigh, svfNotch, comb };
    enum D { soft, hard, tube, fuzz, foldDrive, crush };
    enum Shape { sine, triangle, sawUp, sawDown, square, randomShape, drift, steps };
    enum Div { bars8, bars4, bars2, bar1, half, halfT, quarterD, quarter, quarterT, eighthD, eighth, eighthT, sixteenthD, sixteenth, sixteenthT, thirtySecond, sixtyFourth };

    // Analog's positions: sine, triangle, saw, square.
    constexpr float sinePos = 0.0f, trianglePos = 33.33f, sawPos = 66.67f, squarePos = 100.0f;

    struct Builder
    {
        Builder (const char* name, const char* category)
        {
            patch.name = name;
            patch.category = category;
        }

        Builder& set (const juce::String& id, double v)       { patch.values[id] = v; return *this; }

        static juce::String o (int n, const char* field)       { return "osc" + juce::String (n) + field; }

        Builder& osc (int n, int table, float position, float level = 80.0f)
        {
            return set (o (n, "On"), 1).set (o (n, "Table"), table).set (o (n, "Position"), position).set (o (n, "Volume"), level);
        }

        Builder& pitch (int n, float semitones, float cents = 0.0f)    { return set (o (n, "Pitch"), semitones).set (o (n, "Fine"), cents); }
        Builder& warp (int n, int type, float amount)                   { return set (o (n, "Warp"), type).set (o (n, "WarpAmount"), amount); }
        Builder& pan (int n, float p)                                   { return set (o (n, "Pan"), p); }
        Builder& route (int n, float r)                                 { return set (o (n, "Route"), r); }

        Builder& unison (int n, int voices, float detune, float spread = 80.0f, float strum = 0.0f, float drift = 0.0f)
        {
            return set (o (n, "Unison"), voices).set (o (n, "UnisonDetune"), detune).set (o (n, "UnisonSpread"), spread)
                .set (o (n, "UnisonStrum"), strum).set (o (n, "UnisonDrift"), drift);
        }

        Builder& subOsc (float level, int octave = 0, int shape = 0, bool direct = true)
        {
            return set ("subLevel", level).set ("subOctave", octave).set ("subShape", shape).set ("subDirect", direct ? 1 : 0);
        }

        Builder& noise (float level, int type = 0, float colour = 0.0f)    { return set ("noiseLevel", level).set ("noiseType", type).set ("noiseColour", colour); }

        Builder& filter (int n, int type, float hz, float res = 10.0f, float drive = 0.0f, float key = 0.0f)
        {
            const auto f = "filter" + juce::String (n);
            return set (f + "Type", type).set (f + "Cutoff", hz).set (f + "Resonance", res).set (f + "Drive", drive).set (f + "KeyTrack", key);
        }

        Builder& parallel()                                              { return set ("filterRouting", 1); }

        Builder& env (int n, float a, float d, float s, float r, float curve = 50.0f)
        {
            const auto e = "env" + juce::String (n);
            return set (e + "Attack", a).set (e + "Decay", d).set (e + "Sustain", s).set (e + "Release", r).set (e + "Curve", curve);
        }

        Builder& amp (float a, float d, float s, float r, float curve = 50.0f)    { return env (1, a, d, s, r, curve); }

        Builder& level (float db, float velocity = 70.0f)               { return set ("ampLevel", db).set ("ampVelocity", velocity); }
        Builder& mono (float glideMs = 0.0f, bool legato = true)         { return set ("voiceMode", legato ? 2 : 1).set ("glideTime", glideMs); }
        Builder& glideAlways()                                           { return set ("glideMode", 1); }
        Builder& bend (float st)                                         { return set ("bendRange", st); }

        Builder& lfo (int n, int shape, float hz, int mode = 1, float smooth = 0.0f)
        {
            const auto l = "lfo" + juce::String (n);
            return set (l + "Shape", shape).set (l + "Sync", 0).set (l + "Rate", hz).set (l + "Mode", mode).set (l + "Smooth", smooth);
        }

        Builder& lfoSync (int n, int shape, int division, int mode = 1, float smooth = 0.0f)
        {
            const auto l = "lfo" + juce::String (n);
            return set (l + "Shape", shape).set (l + "Sync", 1).set (l + "Division", division).set (l + "Mode", mode).set (l + "Smooth", smooth);
        }

        Builder& lfoFade (int n, float ms)                               { return set ("lfo" + juce::String (n) + "Fade", ms); }

        Builder& stepPattern (int n, std::initializer_list<float> s)
        {
            int i = 1;
            for (auto v : s)
                set ("lfo" + juce::String (n) + "Step" + juce::String (i++), v);
            return *this;
        }

        Builder& mod (int source, const char* destination, float amount)
        {
            jassert (slot < numSlots && destinationIds().contains (destination));
            const auto m = "mod" + juce::String (++slot);
            return set (m + "Source", source).set (m + "Destination", destinationIds().indexOf (destination) + 1).set (m + "Amount", amount);
        }

        Builder& macro (int n, const char* name, float value = 0.0f)
        {
            patch.macroNames[(size_t) n - 1] = name;
            return set ("macro" + juce::String (n), value);
        }

        Builder& drive (int type, float amount, float tone = 12000.0f, float mix = 100.0f)
        {
            return set ("fxDriveOn", 1).set ("fxDriveType", type).set ("fxDriveAmount", amount).set ("fxDriveTone", tone).set ("fxDriveMix", mix);
        }

        Builder& chorus (float mix = 40.0f, float rate = 0.6f, float depth = 50.0f)
        {
            return set ("fxChorusOn", 1).set ("fxChorusMix", mix).set ("fxChorusRate", rate).set ("fxChorusDepth", depth);
        }

        Builder& phaser (float mix = 50.0f, float rate = 0.3f, float feedback = 50.0f)
        {
            return set ("fxPhaserOn", 1).set ("fxPhaserMix", mix).set ("fxPhaserRate", rate).set ("fxPhaserFeedback", feedback);
        }

        Builder& delay (int division, float feedback = 35.0f, float mix = 25.0f, float tone = 5000.0f, bool pingPong = true)
        {
            return set ("fxDelayOn", 1).set ("fxDelayDivision", division).set ("fxDelayFeedback", feedback).set ("fxDelayMix", mix)
                .set ("fxDelayTone", tone).set ("fxDelayPingPong", pingPong ? 1 : 0);
        }

        Builder& reverb (float mix = 25.0f, float size = 60.0f, float decay = 2.5f, float damping = 45.0f)
        {
            return set ("fxReverbOn", 1).set ("fxReverbMix", mix).set ("fxReverbSize", size).set ("fxReverbDecay", decay).set ("fxReverbDamping", damping);
        }

        // The four macros most patches share: brightness, grit, movement and space.
        Builder& standardMacros (float cutoffAmount = 30.0f)
        {
            macro (1, "Cutoff").mod (macro1, "filter1Cutoff", cutoffAmount);
            macro (2, "Grit");
            if (patch.values.count ("fxDriveOn") == 0) drive (tube, 0.0f, 12000.0f, 100.0f);
            mod (macro2, "fxDriveAmount", 70.0f);
            macro (3, "Space").mod (macro3, "fxReverbMix", 45.0f);
            if (patch.values.count ("fxReverbOn") == 0) reverb (0.0f, 70.0f, 3.0f);
            return *this;
        }

        Patch patch;
        int slot = 0;
    };

    /** Adds the patch bank's patches to the list (WaveSynthPatchBank.cpp). */
    void addBank (std::vector<Builder>&);
}
