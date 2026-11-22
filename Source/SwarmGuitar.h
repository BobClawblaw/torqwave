#pragma once

#include <JuceHeader.h>
#include "BuiltInEffects.h"

namespace BuiltInEffects
{
    constexpr const char* swarmGuitarId = "torqwave:swarm-guitar";

    juce::PluginDescription swarmGuitarDescription();

    /** A guitar with every string tuned to the played note, through a fuzz and a cabinet: the
        wall of unison strings beating against each other that a guitar tuned to one note makes.

        Each note is a guitar of 1 to 12 modelled strings (a plucked delay line each, with a loop
        filter for its brightness and decay) at the note's pitch, or with its low strings an
        octave down and its top string an octave up, each a few cents off the others. A strum
        plucks them one after another; Strum Rate strums again and again while the key is down,
        down and up strokes in turn. Layers plays 1 to 4 guitars at once, each tuned and timed a
        little apart, through its own amp, spread across the stereo field: the strings of one
        guitar distort together, as on one amp, and the guitars stay apart, as takes do.

        The amp is a fuzz (two stages, the first lopsided), a crunch (one stage) or none, then a
        cabinet (a low cut, body, presence and the Tone low-pass). The stages are anti-aliased by
        their antiderivatives, sample by sample, so a part renders the same at any block size and
        the same twice: the noise that plucks a string is worked out from which pluck it is.

        Nothing stops dead: a note let go is damped over Release, a stolen note and All Sound Off
        fade over 20 ms in decibels (so the fade stays gradual through the fuzz), and a string
        plucked again keeps ringing under the new pluck. Six notes at once, the oldest taken over.
        Pitch bend (2 semitones), volume (7), expression (11) and the sustain pedal (64) are
        answered. Its parameters are hosted. No audio input; stereo out. */
    class SwarmGuitarProcessor final : public BuiltInEffect
    {
    public:
        static constexpr int maxStrings = 12, maxLayers = 4, maxNotes = 6, numVoices = maxNotes + 4;

        SwarmGuitarProcessor();

        void prepareToPlay (double sampleRate, int maximumBlockSize) override;
        void reset() override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
        double getTailLengthSeconds() const override;

        /** How many notes are sounding, fading ones among them. */
        int getNumSounding() const;

        /** The semitones string (0 the lowest) of a guitar of strings sits from the played note, without its detune. */
        static int octaveOf (int tuning, int string, int strings);

    private:
        struct String
        {
            std::vector<float> line;
            int write = 0;
            double delay = 100.0;          // samples back the loop reads
            float lowpass = 0.0f, coefficient = 0.5f, loss = 1.0f;
            double hz = 110.0;
            int pluckIn = -1;              // samples until its next pluck; -1 none
            float pluckLevel = 0.0f;
            int excite = -1, period = 0, lag = 0;   // the pluck going in: where it is, the burst's length, the pick's comb
            float exciteLevel = 0.0f, exciteLowpass = 0.0f, exciteCoefficient = 1.0f;
            juce::uint32 seed = 0, plucks = 0;
        };

        struct Voice
        {
            int note = -1;
            bool down = false, held = false, dying = false;
            float velocity = 0.0f, gain = 1.0f;
            juce::uint32 age = 0;
            juce::int64 sinceTouch = 0;    // samples since it was last plucked or let go
            double strumAt = 0.0;          // when the next repeated strum is due, in samples since the note began
            juce::int64 played = 0;        // samples since the note began
            bool upstroke = false;
            std::array<std::array<String, maxStrings>, maxLayers> strings;
        };

        struct Biquad
        {
            float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f, z1 = 0.0f, z2 = 0.0f;

            float process (float x) noexcept
            {
                const auto y = b0 * x + z1;
                z1 = b1 * x - a1 * y + z2;
                z2 = b2 * x - a2 * y;
                return y;
            }

            void clear() noexcept      { z1 = z2 = 0.0f; }
        };

        struct Amp
        {
            double before1 = 0.0, before2 = 0.0;      // each stage's input a sample ago
            float tight = 0.0f, between = 0.0f;       // one-pole low cuts: before the fuzz, and between the stages
            Biquad lowCut, body, presence, tone1, tone2;
        };

        void noteOn (int note, float velocity);
        void noteOff (int note);
        void allOff (bool fade);
        void controller (const juce::MidiMessage&);
        void strum (Voice&, int startingIn, bool repeated);
        void tune (Voice&);
        void free (Voice&);
        void setAmps();
        void renderStrings (int from, int count);
        float tick (String&) noexcept;
        static double shape (double x, double& before, double bias) noexcept;

        int stringCount() const;
        int layerCount() const;

        juce::AudioParameterFloat *strings = nullptr, *detune = nullptr, *strumTime = nullptr, *strumRate = nullptr, *layers = nullptr,
                                  *width = nullptr, *pick = nullptr, *brightness = nullptr, *sustain = nullptr, *release = nullptr,
                                  *drive = nullptr, *tone = nullptr, *level = nullptr;
        juce::AudioParameterChoice *tuning = nullptr, *amp = nullptr;

        std::array<Voice, numVoices> voices;
        std::array<Amp, maxLayers> amps;
        std::array<std::vector<float>, maxLayers> buses;
        juce::uint32 counter = 0;
        double rate = 48000.0;
        int mask = 0;
        float bend = 0.0f, volume = 1.0f, expression = 1.0f;
        float lastGain = -1.0f;        // the output gain the last block ended on; below 0, none yet
        bool pedal = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SwarmGuitarProcessor)
    };
}
