#pragma once

#include "BuiltInEffects.h"
#include "WaveTables.h"

/** The built-in synth's engine: three wavetable oscillators (each with Position, a warp, and up to
    eight unison voices with detune, stereo spread, strum and drift), a sub and a noise source, two
    filters in series or parallel (a 24 dB ladder and a state-variable filter among their types),
    four envelopes, two LFOs, eight modulation slots, four macros, and drive, chorus, phaser, delay
    and reverb after the voices. Sixteen voices, poly, mono or legato with glide.

    Nothing it does steps the sound: a note struck again keeps its waves' phases and its filters'
    state and starts its envelopes from where they are; a voice taken for a new note, and All
    Sound Off (sent on a stop, a loop or a seek), fade out over 4 ms; the amp's attack is never
    shorter than 1.5 ms. Its controls are taken afresh every 16 samples of the song's own clock and
    ramped between, so it renders the same at any block size. */
namespace WaveSynth
{
    constexpr int numOscillators = 3, numEnvelopes = 4, numLfos = 2, numSlots = 8, numMacros = 4, numSteps = 16;
    constexpr int maxUnison = 8, numVoices = 16, numFadingVoices = 8;
    constexpr int controlInterval = 16;
    constexpr double fadeSeconds = 0.004, minimumAttackSeconds = 0.0015;

    /** The modulation sources, in this order (index 0 is none). */
    enum Source { none, env1, env2, env3, env4, lfo1, lfo2, velocity, modWheel, aftertouch, key, macro1, macro2, macro3, macro4, random, numSources };
    const juce::StringArray& sourceNames();

    /** The parameters a slot can move, by ID, and their names in its menu, in menu order (added
        to at the end only, since sessions keep the index). */
    const juce::StringArray& destinationIds();
    const juce::StringArray& destinationNames();
    /** The menu's sections: a heading before the destination at each index. */
    const std::vector<std::pair<int, juce::String>>& destinationSections();

    enum class Warp { off, sync, bend, squeeze, mirror, fm, crush, fold };
    const juce::StringArray& warpNames();

    enum class FilterType { off, ladder24, ladder12, ladderHigh, ladderBand, svfLow, svfBand, svfHigh, svfNotch, comb };
    const juce::StringArray& filterNames();

    const juce::StringArray& lfoShapeNames();
    const juce::StringArray& divisionNames();
    /** A division's length in beats. */
    double divisionBeats (int index);

    /** A factory patch: its name, category and the parameter values it sets (the rest at their
        defaults), plus its macros' names. */
    struct Patch
    {
        juce::String name, category;
        EffectPresets::Values values;
        std::array<juce::String, numMacros> macroNames;
    };

    /** The factory patches, by category in browser order. */
    const std::vector<Patch>& factoryPatches();
    const juce::StringArray& patchCategories();

    /** What the window reads as the synth plays: each destination's modulated value (normalised;
        negative when nothing sounds), each LFO's output, and envelope 1's level in the newest voice. */
    struct Readings
    {
        // Nothing has sounded yet: the window shows the knobs' own values.
        Readings()                                                   { for (auto& d : destination) d.store (-1.0f); }

        std::array<std::atomic<float>, 64> destination {};
        std::array<std::atomic<float>, numLfos> lfo {};
        std::array<std::atomic<float>, numEnvelopes> envelope {};
        std::atomic<int> sounding { 0 };
    };
}

namespace BuiltInEffects
{
    /** The instrument built in, under the plug-in ID it had as Torqwave Synth: WaveSynth's engine
        (see above), stereo, with its own window. A session or preset saved before 0.20.0 loads
        its old settings as the nearest sound the new engine makes: its wave as the Analog table's
        position, oscillators 2 and 3 at their octave, detune and level, its envelope, its
        low-pass filter, filter envelope and velocity to filter as modulation slots, and glide as
        legato. It doesn't sound the same sample for sample. */
    class BasicSynthProcessor final : public BuiltInEffect
    {
    public:
        BasicSynthProcessor();
        ~BasicSynthProcessor() override;

        void prepareToPlay (double sampleRate, int maximumBlockSize) override;
        void reset() override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
        double getTailLengthSeconds() const override;
        juce::AudioProcessorEditor* createEditor() override;

        /** How many notes are sounding (not counting voices fading out). */
        int getNumSounding() const;

        /** A parameter by its ID (nullptr if there is none). */
        juce::RangedAudioParameter* parameter (const juce::String& id) const;

        /** Loads a factory patch (every parameter to its default, then the patch's), by index or name. */
        void loadPatch (int index);
        bool loadPatch (const juce::String& name);
        const juce::String& getPatchName() const noexcept            { return patchName; }
        const juce::String& getMacroName (int i) const               { return macroNames[(size_t) i]; }
        void setMacroName (int i, const juce::String& n)              { macroNames[(size_t) i] = n; }

        /** Routes a source to a destination parameter in the first free slot (or the slot already
            joining them); the slot's index, or -1 when all eight are taken. */
        int addModulation (int source, const juce::String& destinationId, float amount = 0.25f);
        void removeModulation (int slot);
        void setModulationAmount (int slot, float amount);

        /** The slots reaching a parameter: (slot, source, amount -1 to 1). */
        std::vector<std::tuple<int, int, float>> modulationOf (const juce::String& id) const;

        WaveSynth::Readings& getReadings() noexcept                  { return readings; }

        /** The old (before 0.20.0) settings turned into the new engine's, every parameter given. */
        static EffectPresets::Values translateOldSettings (const EffectPresets::Values& old);

        /** Settings applied as a preset: old ones are translated first. */
        void setPresetValues (const EffectPresets::Values&) override;

        struct Engine;

    protected:
        void addToState (juce::XmlElement&) const override;
        void readFromState (const juce::XmlElement&) override;

    private:
        void setAllToDefault();

        std::unique_ptr<Engine> engine;
        std::map<juce::String, juce::RangedAudioParameter*> byId;
        juce::String patchName { "Init" };
        std::array<juce::String, WaveSynth::numMacros> macroNames;
        WaveSynth::Readings readings;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BasicSynthProcessor)
    };
}
