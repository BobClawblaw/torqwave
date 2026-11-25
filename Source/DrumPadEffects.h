#pragma once

#include <JuceHeader.h>

/** The effects each of Basic Drums' pads has for itself, beside its filter, crusher and sends: a
    volume envelope, a transient shaper, a saturator, a four-band EQ and a compressor, in that
    order. Each is off until switched on, and a pad with all five off is played exactly as it was
    before there were any. They belong to a hit: its settings are the pad's when it is struck, and
    everything they remember starts with it, so a pad's effects sound the same whatever else plays,
    render the same at any block size, and a change to one is heard from the next hit on. */
namespace DrumMachine
{
    /** A pad's values for its effects, in the order they are kept. */
    enum Fx
    {
        envOn, envAttack, envHold, envDecay, envCurve,
        transOn, transAttack, transSustain,
        satOn, satType, satAmount, satMix,
        eqOn, eqLowFreq, eqLowGain, eqBell1Freq, eqBell1Gain, eqBell1Width, eqBell2Freq, eqBell2Gain, eqBell2Width, eqHighFreq, eqHighGain,
        compOn, compThreshold, compRatio, compAttack, compRelease, compGain,
        numFx
    };

    /** One of them: the name it is saved under, how it reads, its range and where it starts, its
        unit ("" for a switch or a choice), and the skew a knob turns it with (1: evenly). */
    struct FxInfo
    {
        const char* id;
        const char* name;
        float low, high, start;
        const char* unit;
        float skew;
    };

    const FxInfo& fxInfo (int fx);

    /** The saturator's curves ("Tape", "Tube", "Clip", "Fold"). */
    juce::StringArray saturationNames();

    /** The five effects, in the order a hit goes through them, each with its switch. */
    constexpr int numEffects = 5;
    constexpr std::array<Fx, numEffects> effectSwitches { envOn, transOn, satOn, eqOn, compOn };
    juce::StringArray effectNames();

    struct PadEffects
    {
        PadEffects();

        std::array<float, numFx> values;

        float operator[] (Fx f) const noexcept       { return values[(size_t) f]; }
        float& operator[] (Fx f) noexcept            { return values[(size_t) f]; }
        bool on (Fx f) const noexcept                { return values[(size_t) f] >= 0.5f; }
        bool anyOn() const noexcept;

        /** Every value put inside its range (a switch 0 or 1, the saturator's curve a whole number). */
        void limit();

        bool operator== (const PadEffects& o) const noexcept;
        bool operator!= (const PadEffects& o) const noexcept   { return ! (*this == o); }
    };

    //==============================================================================
    // What the pictures draw and the tests measure against.

    /** The envelope's gain (0 to 1) a time after the hit, and how long it lasts in all. */
    float envelopeGain (const PadEffects&, double seconds);
    double envelopeSeconds (const PadEffects&);

    /** The saturator's output for an input (both as samples, -1 to 1 and beyond), mix and all. */
    float saturated (const PadEffects&, float in);

    /** The EQ's gain in dB at a frequency, of the bands that are not at 0 dB. */
    float eqGainDb (const PadEffects&, double hz, double sampleRate);

    /** The compressor's output level for a steady input level, both in dB, its gain included. */
    float compressedDb (const PadEffects&, float inDb);

    //==============================================================================
    /** The effects of one hit: set from the pad's values when it starts, then run on each of its
        samples. Copied with its hit. Nothing here allocates. */
    struct HitEffects
    {
        /** Takes the pad's values. False when every effect is off: then run() must not be called. */
        bool start (const PadEffects&, double sampleRate);

        /** A sample through the effects that are on, in order. age counts the hit's samples from 0. */
        void run (float& l, float& r, juce::int64 age) noexcept;

        /** The envelope's length in samples; 0 when it is off (the hit's own length stands). */
        juce::int64 envelopeLength = 0;

        /** The furthest the compressor has turned this hit down so far, in dB (0 or less). */
        float reductionDb = 0.0f;

    private:
        struct Biquad
        {
            double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
            double l1 = 0.0, l2 = 0.0, r1 = 0.0, r2 = 0.0;
        };

        bool env = false, trans = false, sat = false, eq = false, comp = false;

        // The envelope: its three parts in samples, and the power its fall is raised to.
        double envA = 0.0, envH = 0.0, envD = 1.0, envPower = 1.0;

        // The transient shaper: three followers of the level, and how far the attack and the
        // sustain each move the gain (in octaves, for an octave between two followers).
        float tFast = 0.0f, tSlow = 0.0f, tHeld = 0.0f;
        float tFastK = 0.0f, tSlowK = 0.0f, tFallK = 0.0f, tHeldK = 0.0f, tAttack = 0.0f, tSustain = 0.0f;

        // The saturator.
        int satCurve = 0;
        float satDrive = 1.0f, satNorm = 1.0f, satWet = 1.0f;
        float dcK = 0.0f, dcL = 0.0f, dcR = 0.0f;

        // The EQ's bands that do something.
        std::array<Biquad, 4> bands;
        int numBands = 0;

        // The compressor.
        float cPeak = 0.0f, cReduction = 0.0f, cAttack = 0.0f, cRelease = 0.0f, cThreshold = 0.0f, cSlope = 0.0f, cMakeUp = 1.0f;
    };
}
