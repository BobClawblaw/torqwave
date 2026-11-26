#pragma once

#include "BuiltInEffects.h"
#include "WaveTables.h"

/** The built-in synth's engine: nine wavetable oscillators (each with Position, a warp, and up to
    eight unison voices with detune, stereo spread, strum and drift, and a level envelope of its
    own to switch on), a sub and a noise source, two
    filters in series or parallel (a 24 dB ladder and a state-variable filter among their types),
    four envelopes, two LFOs, sixteen modulation slots, four macros, and drive, chorus, phaser, delay
    and reverb after the voices. Sixteen voices, poly, mono or legato with glide.

    Nothing it does steps the sound: a note struck again keeps its waves' phases and its filters'
    state and starts its envelopes from where they are; a voice taken for a new note, and All
    Sound Off (sent on a stop, a loop or a seek), fade out over 4 ms; the amp's attack is never
    shorter than 1.5 ms. Its controls are taken afresh every 16 samples of the song's own clock and
    ramped between, so it renders the same at any block size. */
namespace WaveSynth
{
    constexpr int numOscillators = 9, numEnvelopes = 4, numLfos = 2, numSlots = 16, numMacros = 4, numSteps = 16;
    /** The oscillators it had before it had nine: their parameters and destinations come first, those of 4 to 9
        after everything else, so every earlier session, preset and patch keeps its places. Decrypt and the
        signal console's Trace move these three only. */
    constexpr int numFirstOscillators = 3;
    /** The modulation destinations before there were nine oscillators (a slot's menu held these and None). */
    constexpr int numFirstDestinations = 46;
    /** The destinations: 9 for each oscillator, 19 more (sub, noise, filters, amp, LFO rates, effects). */
    constexpr int numDestinations = numFirstDestinations + 9 * (numOscillators - numFirstOscillators);
    constexpr int maxUnison = 8, numVoices = 16, numFadingVoices = 8;
    constexpr int controlInterval = 16;
    /** The unison voices that may sound at once, every note's oscillators counted (each on, its unison):
        three oscillators in 8-voice unison on all sixteen voices, the most a patch could ask for before
        there were nine. A patch that asks for more plays fewer notes at once, the oldest fading out to
        make room as when all sixteen are taken, so what one costs never grows past that. */
    constexpr int unisonBudget = 3 * maxUnison * numVoices;
    constexpr double fadeSeconds = 0.004, minimumAttackSeconds = 0.0015;
    /** A held sound (its amp sustains) struck again while it sounds (mono, or a ringing key played again):
        its amp, and what the envelopes do to the filters, rise from where they are no faster than this, as
        an amp or a filter jumping in a millisecond over a sounding note clicks. Pitch, wave, FM, warp and the
        oscillators' levels are left as they were, and so is a one-shot (no sustain), a fresh note and a note
        struck after silence. */
    constexpr double retriggerSeconds = 0.004;
    /** How long CC 74 (Cutoff) and the macros take to follow a move while notes sound: a controller's steps
        would otherwise step the filters, and a comb jumps and crackles. With no note sounding they are taken
        at once. */
    constexpr double controlSmoothingSeconds = 0.03;
    /** A legato slide brings the new note's velocity in over the glide, and at least this long, so what
        velocity moves glides rather than jumping. */
    constexpr double velocityGlideSeconds = 0.02;

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
    /** An oscillator's FM From choices: "Before" (the one before it: 1 takes 3, 2 takes 1, 3 takes 2, as
        it always has, and each of 4 to 9 the one before it), then Osc 1 to Osc 9. */
    const juce::StringArray& fmFromNames();
    /** The oscillator (1 to 9) a parameter ID belongs to, or 0. */
    int oscillatorOf (const juce::String& parameterId);
    /** True for a parameter added with oscillators 4 to 9 (theirs, and each oscillator's FM From). */
    bool isAddedWithNine (const juce::String& parameterId);
    /** Whether a saved state (as a session keeps it: base64) is Torqwave Synth's from before it had nine
        oscillators. An automation lane on a slot's Destination then holds values normalised over the 47
        choices the menu had (None and 46 destinations), which destinationFromBeforeNine puts back on the
        same destinations among the 101 there are now. */
    bool stateIsBeforeNine (const juce::String& base64State);
    float destinationFromBeforeNine (float normalised);
    bool isSlotDestination (const juce::String& parameterId);

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
    /** The Complex patches: a list of their own after the factory patches (whose indices they leave as
        they are), four oscillators or more in each, by category in browser order. */
    const std::vector<Patch>& complexPatches();
    const juce::StringArray& patchCategories();
    /** The name a factory patch has now: a patch renamed since a session, preset or favourite was
        saved is found under its new name; any other name is given back as it is. */
    juce::String patchNamedNow (const juce::String& name);

    /** What the window reads as the synth plays: each destination's modulated value (normalised;
        negative when nothing sounds), each LFO's output, and envelope 1's level in the newest voice. */
    struct Readings
    {
        // Nothing has sounded yet: the window shows the knobs' own values.
        Readings()                                                   { for (auto& d : destination) d.store (-1.0f); }

        std::array<std::atomic<float>, numDestinations> destination {};
        std::array<std::atomic<float>, numLfos> lfo {};
        std::array<std::atomic<float>, numEnvelopes> envelope {};
        std::atomic<int> sounding { 0 };
    };
}

namespace SignalConsole { class Console; }

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
        /** Its automation menus' groups: each oscillator, then sub and noise, the filters, amp and voice,
            the envelopes, the LFOs, the modulation slots, the macros and the effects. */
        juce::StringArray parameterGroups() const override;
        juce::String parameterGroupOf (const juce::String& parameterId) const override;

        /** How many notes are sounding (not counting voices fading out). */
        int getNumSounding() const;

        /** A parameter by its ID (nullptr if there is none). */
        juce::RangedAudioParameter* parameter (const juce::String& id) const;

        /** Loads a factory patch (every parameter to its default, then the patch's), by index or name; by
            name, a Complex patch too. */
        void loadPatch (int index);
        bool loadPatch (const juce::String& name);

        /** What the next and previous arrows go through (the window's, and the arrow keys'): every
            factory patch in the browser's order, or, while the favourites are being browsed and
            there are some, the favourites in the tree's order (by category, then as the tree lists
            them). The factory list's indices. */
        std::vector<int> browseList() const;
        void setBrowsingFavourites (bool favourites) noexcept        { browsingFavourites = favourites; browsingDecrypted = browsingDecrypted && ! favourites; browsingComplex = browsingComplex && ! favourites; }
        bool isBrowsingFavourites() const noexcept                   { return browsingFavourites; }
        /** While on, and there are some, the arrows go through the patches taken from Decrypt and
            named (DecryptedPatches), by name; stepPatch then returns -1, the patch being no factory one. */
        void setBrowsingDecrypted (bool decrypted) noexcept          { browsingDecrypted = decrypted; browsingFavourites = browsingFavourites && ! decrypted; browsingComplex = browsingComplex && ! decrypted; }
        bool isBrowsingDecrypted() const noexcept                    { return browsingDecrypted; }
        /** While on, the arrows go through the Complex patches (complexPatches(), by category), by
            name; stepPatch then returns -1, the patch being no factory one. */
        void setBrowsingComplex (bool complex) noexcept              { browsingComplex = complex; browsingFavourites = browsingFavourites && ! complex; browsingDecrypted = browsingDecrypted && ! complex; }
        bool isBrowsingComplex() const noexcept                      { return browsingComplex; }
        /** Loads the patch after (by 1) or before (by -1) the one it is on in that list, round its
            ends; from a sound that isn't in the list, the first (or last). Its index, or -1 if
            the list is empty. */
        int stepPatch (int by);
        const juce::String& getPatchName() const noexcept            { return patchName; }
        void setPatchName (const juce::String& name)                  { patchName = name; }
        const juce::String& getMacroName (int i) const               { return macroNames[(size_t) i]; }
        void setMacroName (int i, const juce::String& n)              { macroNames[(size_t) i] = n; }

        /** Routes a source to a destination parameter in the first free slot (or the slot already
            joining them); the slot's index, or -1 when all sixteen are taken. */
        int addModulation (int source, const juce::String& destinationId, float amount = 0.25f);
        void removeModulation (int slot);
        void setModulationAmount (int slot, float amount);

        /** The slots reaching a parameter: (slot, source, amount -1 to 1). */
        std::vector<std::tuple<int, int, float>> modulationOf (const juce::String& id) const;

        WaveSynth::Readings& getReadings() noexcept                  { return readings; }

        /** The table an oscillator (0 to 8) plays when its Table is "Sampled": one made from a sound
            (WaveTables::sampledFromAudio). Nullptr takes it away, and the oscillator plays a
            sawtooth. Kept in the state, so a session or a patch with one plays anywhere; a factory
            patch has none. Set from the message thread; the voices take it up at their next block. */
        void setSampledTable (int oscillator, std::shared_ptr<const WaveTables::Sampled>);
        std::shared_ptr<const WaveTables::Sampled> getSampledTable (int oscillator) const;

        /** What a preset carries besides its values: the patch's name ("patch"), the macros' names
            ("macro1Name" to "macro4Name", those named) and each Sampled table ("osc1Sampled" to
            "osc9Sampled", as the state keeps it). Applying a preset's texts sets all of them, a
            name or table it lacks to none; empty texts (a preset saved before 0.20.0, or by an
            effect's dialog) leave them as they are. */
        EffectPresets::Texts getPresetTexts() const;
        void setPresetTexts (const EffectPresets::Texts&);

        /** Its signal console (Trace: the sounds within reach of its patch), made when first asked
            for, on the message thread. */
        SignalConsole::Console& getConsole();

        /** The old (before 0.20.0) settings turned into the new engine's, every parameter given. */
        static EffectPresets::Values translateOldSettings (const EffectPresets::Values& old);

        /** Settings applied as a preset: old ones are translated first. */
        void setPresetValues (const EffectPresets::Values&) override;

        struct Engine;

    protected:
        void addToState (juce::XmlElement&) const override;
        void readFromState (const juce::XmlElement&) override;

    private:
        std::unique_ptr<juce::XmlElement> sampledElement (int oscillator) const;
        void setAllToDefault();
        /** Oscillators 4 to 9 (with their Sampled tables) and every FM From to their defaults: for a state
            or a whole patch saved before they were added. */
        void resetAddedWithNine();

        std::unique_ptr<Engine> engine;
        std::map<juce::String, juce::RangedAudioParameter*> byId;
        juce::String patchName { "Init" };
        std::array<juce::String, WaveSynth::numMacros> macroNames;
        WaveSynth::Readings readings;

        // The sampled tables: swapped under the lock, which the audio thread only tries. One taken
        // away is kept until the voices have moved on from it.
        std::array<std::shared_ptr<const WaveTables::Sampled>, WaveSynth::numOscillators> sampled;
        std::vector<std::pair<juce::uint32, std::shared_ptr<const WaveTables::Sampled>>> retired;
        juce::SpinLock sampledLock;
        std::atomic<juce::uint32> sampledGeneration { 0 }, sampledSeen { 0 };

        bool browsingFavourites = false, browsingDecrypted = false, browsingComplex = false;
        std::unique_ptr<SignalConsole::Console> console;
        std::atomic<SignalConsole::Console*> consoleForAudio { nullptr };
        double preparedRate = 0.0;
        int preparedBlock = 512;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BasicSynthProcessor)
    };
}
