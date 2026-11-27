#pragma once

#include "BuiltInEffects.h"
#include "NoteTools.h"

namespace BuiltInEffects
{
    constexpr const char* midiArpId   = "torqwave:midi-arp";
    constexpr const char* midiChordId = "torqwave:midi-chord";
    constexpr const char* midiScaleId = "torqwave:midi-scale";
    constexpr const char* midiEchoId  = "torqwave:midi-echo";

    juce::PluginDescription midiArpDescription();
    juce::PluginDescription midiChordDescription();
    juce::PluginDescription midiScaleDescription();
    juce::PluginDescription midiEchoDescription();

    /** True for built-in or third-party MIDI effects that transform MIDI before an instrument. */
    inline bool isMidiEffectPlugin (const juce::AudioPluginInstance& p)
    {
        return ! p.getPluginDescription().isInstrument && p.acceptsMidi() && p.producesMidi();
    }

    /** Real-time tempo-synced MIDI arpeggiator (Up, Down, Up-Down, Down-Up, As Played, Random). */
    class MidiArpProcessor final : public BuiltInEffect
    {
    public:
        MidiArpProcessor();

        bool acceptsMidi() const override  { return true; }
        bool producesMidi() const override { return true; }
        bool isMidiEffect() const override { return true; }

        void prepareToPlay (double sampleRate, int maximumBlockSize) override;
        void reset() override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    private:
        struct HeldNote { int pitch = 60; float velocity = 0.8f; int channel = 1; };

        void buildOrder();

        juce::AudioParameterChoice *mode = nullptr, *division = nullptr, *velocityMode = nullptr;
        juce::AudioParameterFloat *octaves = nullptr, *gate = nullptr, *swing = nullptr;
        juce::AudioParameterBool* latch = nullptr;

        double rate = 48000.0;
        std::vector<HeldNote> held;
        std::vector<HeldNote> latched;
        std::vector<HeldNote> sequence;
        int stepIndex = 0;
        double samplesUntilStep = 0.0;
        int activePitch = -1, activeChannel = 1;
        int samplesUntilOff = 0;
        juce::uint32 rng = 0x71c3a509u;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiArpProcessor)
    };

    /** Real-time MIDI chord trigger (triads, 7ths, 9ths, sus, diatonic scale chords, inversions, strum). */
    class MidiChordProcessor final : public BuiltInEffect
    {
    public:
        MidiChordProcessor();

        bool acceptsMidi() const override  { return true; }
        bool producesMidi() const override { return true; }
        bool isMidiEffect() const override { return true; }

        void prepareToPlay (double sampleRate, int maximumBlockSize) override;
        void reset() override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    private:
        struct PendingNote { int delaySamples = 0; juce::MidiMessage message; };
        std::vector<int> chordPitchesFor (int rootPitch) const;

        juce::AudioParameterChoice *chordType = nullptr, *scaleRoot = nullptr, *scaleMode = nullptr;
        juce::AudioParameterFloat *inversion = nullptr, *strumMs = nullptr;
        juce::AudioParameterFloat *custom1 = nullptr, *custom2 = nullptr, *custom3 = nullptr;

        double rate = 48000.0;
        std::array<std::vector<int>, 128> soundingForKey;
        std::vector<PendingNote> pending;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiChordProcessor)
    };

    /** Real-time MIDI scale quantizer / filter with transpose and stuck-note-safe note-off tracking. */
    class MidiScaleProcessor final : public BuiltInEffect
    {
    public:
        MidiScaleProcessor();

        bool acceptsMidi() const override  { return true; }
        bool producesMidi() const override { return true; }
        bool isMidiEffect() const override { return true; }

        void prepareToPlay (double sampleRate, int maximumBlockSize) override;
        void reset() override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    private:
        int mapPitch (int pitch) const noexcept;

        juce::AudioParameterChoice *root = nullptr, *mode = nullptr, *action = nullptr;
        juce::AudioParameterFloat* transpose = nullptr;

        std::array<int, 128> activeMapping {}; // -1: not held / filtered

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiScaleProcessor)
    };

    /** Real-time tempo-synced MIDI note echo with repeat count, velocity decay, and pitch shift per echo. */
    class MidiEchoProcessor final : public BuiltInEffect
    {
    public:
        MidiEchoProcessor();

        bool acceptsMidi() const override  { return true; }
        bool producesMidi() const override { return true; }
        bool isMidiEffect() const override { return true; }

        void prepareToPlay (double sampleRate, int maximumBlockSize) override;
        void reset() override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    private:
        struct Scheduled { juce::int64 sampleTime = 0; juce::MidiMessage message; };

        juce::AudioParameterBool* sync = nullptr;
        juce::AudioParameterChoice* division = nullptr;
        juce::AudioParameterFloat *timeMs = nullptr, *repeats = nullptr, *decay = nullptr, *pitchShift = nullptr, *gate = nullptr;

        double rate = 48000.0;
        juce::int64 sampleClock = 0;
        std::vector<Scheduled> queue;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiEchoProcessor)
    };
}
