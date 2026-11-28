#pragma once

#include "BuiltInEffects.h"
#include "DrumPadEffects.h"
#include "Sampler.h"

/** The drum machine behind Basic Drums: synthesised sounds on the General MIDI drum map (35 to 81),
    each with its own tune, decay, tone, level, pan and drive, a kit-wide drive, glue compressor and
    level, and the kits made from them. Twenty-one of the sounds come in two types: Classic, the
    sound as it has always been, and Modern, a second model of it with more to it. Every sound has
    one more type, Sample: the pad plays recordings given to it, in layers by velocity, through the
    same settings. The engine knows nothing of plug-ins, so the window can
    render a sound with it to draw, and the tests can play it bare. */
namespace DrumMachine
{
    /** The sounds. Notes that are two of one instrument (the toms' pairs, high and low bongo, mute
        and open triangle) share a sound's settings, each note still ringing on its own voice. */
    enum class Sound { kick, rim, snare, clap, closedHat, pedalHat, openHat, lowTom, midTom, highTom, crash, ride, cowbell, shaker,
                       china, rideBell, tambourine, splash, vibraslap, bongo, muteConga, openConga, timbale, agogo, whistle, guiro,
                       claves, woodBlock, cuica, triangle, count };

    constexpr int numSounds = (int) Sound::count;
    constexpr int firstNote = 35, lastNote = 81, numNotes = lastNote - firstNote + 1;

    /** A note's sound, or none for a note outside 35 to 81. */
    std::optional<Sound> soundFor (int note);

    /** The note a sound's pad plays (36 for the kick, 42 for the closed hat). */
    int padNote (Sound);

    /** Its name ("Closed Hat") and its parameters' prefix ("closedHat"). */
    juce::String nameOf (Sound);
    juce::String idOf (Sound);

    /** Notes that cut each other off as one instrument does: the hats (42, 44, 46), the triangle
        (80, 81) and the cuica (78, 79). 0 for a note in no group. A note struck again always cuts its
        own last hit. */
    int chokeGroup (int note);

    /** One sound's settings, in the units its knobs show. */
    struct Settings
    {
        float tune = 0.0f;      // semitones, -24 to 24
        float decay = 300.0f;   // ms until it has fallen 60 dB, 5 to 5,000
        float tone = 0.0f;      // -100 (dark) to 100 (bright)
        float level = 0.0f;     // dB, -48 to 6
        float pan = 0.0f;       // -100 (left) to 100 (right)
        float drive = 0.0f;     // %, 0 to 100

        // Its effects: a resonant filter and a bitcrusher on it alone, then how much goes to the
        // kit's reverb and delay (after its level and pan).
        int filter = 0;              // 0 off, 1 low-pass, 2 band-pass, 3 high-pass
        float cutoff = 2000.0f;      // Hz, 20 to 20,000
        float resonance = 20.0f;     // %, 0 to 100
        float crush = 0.0f;          // %, 0 (clean) to 100 (4 bits, a twentieth of the rate)
        float reverb = 0.0f;         // %, 0 to 100, sent
        float delay = 0.0f;          // %, 0 to 100, sent

        // What plays: 0 Classic, 1 Modern (for the sounds that have one, hasTypes), 2 Sample (the pad's layers).
        int type = 0;

        // For a Sample alone: where in the sample it starts, whether it plays backwards, and how far
        // off its pitch it starts (it comes to pitch over about a tenth of a second).
        float start = 0.0f;          // %, 0 to 95 of the sample's length
        bool reverse = false;
        float bend = 0.0f;           // semitones, -24 to 24

        // Its own envelope, transient shaper, saturator, EQ and compressor, each off to start with
        // (DrumPadEffects.h). They come after the filter and the crusher, before its level and pan.
        PadEffects effects {};

        // The choke group it is in, of the user's eight: 0 none, 1 to numChokeGroups. A hit of a
        // pad in a group fades every other hit sounding in that group (beside the groups built
        // in: the hats, the triangle, the cuica).
        int choke = 0;

        // The source it plays in place of its own sound or samples: 0 for none, 1 to numSources.
        int source = 0;

        // Multi-out bus routing: 0 = Main mix, 1..numAuxBuses = dedicated stereo aux output bus.
        int outputBus = 0;

        bool operator== (const Settings& o) const
        {
            return juce::exactlyEqual (tune, o.tune) && juce::exactlyEqual (decay, o.decay) && juce::exactlyEqual (tone, o.tone)
                && juce::exactlyEqual (level, o.level) && juce::exactlyEqual (pan, o.pan) && juce::exactlyEqual (drive, o.drive)
                && filter == o.filter && juce::exactlyEqual (cutoff, o.cutoff) && juce::exactlyEqual (resonance, o.resonance)
                && juce::exactlyEqual (crush, o.crush) && juce::exactlyEqual (reverb, o.reverb) && juce::exactlyEqual (delay, o.delay)
                && type == o.type && juce::exactlyEqual (start, o.start) && reverse == o.reverse && juce::exactlyEqual (bend, o.bend)
                && effects == o.effects && choke == o.choke && source == o.source && outputBus == o.outputBus;
        }
    };

    constexpr int classicType = 0, modernType = 1, sampleType = 2;
    constexpr int numChokeGroups = 8;
    constexpr int numAuxBuses = 8;
    juce::StringArray outputBusNames();

    //==============================================================================
    // Sources: synthesized sounds beside the 30, which belong to no note. Any pad can play one in
    // place of its own sound or its samples, through the same settings and effects: a user pad,
    // or a sound's pad (a Rumble Kick on the kick's notes). They are numbered from 1; a pad's
    // source 0 is none. The order here is the order they are listed in and never changes; a state
    // keeps a source by its id.
    constexpr int numSources = 24;

    struct SourceInfo
    {
        const char* id;
        const char* name;
        const char* family;     // "Kicks", "Snares and Claps", "Hats and Cymbals", "Metal", "Industrial", "Low"
        float decayMs;          // its decay as it starts
        float trimDb;
    };

    const SourceInfo& sourceInfo (int source);          // 1 to numSources
    int sourceWithId (const juce::String& id);          // 0 when there is none of that id

    //==============================================================================
    // Pads. The 30 sounds are the first 30 pads, in the order of Sound. After them come 34 user
    // pads, 64 in all: each plays samples only, on a note of its own choosing, which may be one a
    // sound has (then both play: a layered kick) or one outside 35 to 81. Until given another, user
    // pad 1 is on note 82, the next on 83, and so on up to 115.
    constexpr int numUserPads = 34, numPads = numSounds + numUserPads, firstUserNote = 82;

    /** A user pad's settings as it starts: a Sample, its decay at the top, so a sample plays whole. */
    inline Settings defaultUserSettings()
    {
        Settings s;
        s.decay = 5000.0f;
        s.type = sampleType;
        return s;
    }

    struct UserPad
    {
        Settings settings = defaultUserSettings();
        int note = firstUserNote;      // 0 to 127

        bool operator== (const UserPad& o) const   { return settings == o.settings && note == o.note; }
    };

    /** A sound's types ("Classic", "Modern"), and whether it has the second. Classic is the model
        every sound had before there were two, unchanged: a hit's level follows how hard it is
        struck and nothing else. A Modern sound is also duller, and where it suits shorter or less
        bent in pitch, the softer it is struck. */
    juce::StringArray typeNames();
    bool hasTypes (Sound);

    /** The types a sound's selector offers, in order: "Classic", "Modern" where it has one, and
        "Sample"; the type (classicType, modernType, sampleType) at a place in that list, and a
        type's place in it. */
    juce::StringArray typeNamesFor (Sound);
    int typeAt (Sound, int index);
    int typeIndexFor (Sound, int type);

    //==============================================================================
    // Samples on a pad. A pad whose type is Sample plays its layers: recordings, each for a range
    // of velocities. Layers whose ranges both hold a hit's velocity take turns, in their order,
    // one for each hit of the sound (round robin), so a repeated hit is not one recording twice.
    constexpr int maxLayers = 16;

    struct Layer
    {
        Sampler::SamplePtr sample;     // none when its file can't be read
        juce::File file;               // where it is from, kept when it can't be read so it can be found again
        int lowVelocity = 1, highVelocity = 127;

        bool plays (int velocity) const noexcept   { return sample != nullptr && velocity >= lowVelocity && velocity <= highVelocity; }
    };

    /** Every pad's layers, as the hits read them: never changed once made. */
    using PadLayers = std::array<std::vector<Layer>, (size_t) numPads>;

    struct Samples
    {
        PadLayers pads;                // the 30 sounds' in the order of Sound, then the user pads'
        juce::uint32 generation = 0;
    };

    using SamplesPtr = std::shared_ptr<const Samples>;

    /** The longest a sample on a pad plays for, in seconds: with its decay at this (the knob's top)
        or longer than the sample, a sample plays to its own end. */
    constexpr double longestDecaySeconds = 5.0;

    /** The kick's three characters. */
    enum class KickCharacter { boom, punch, hard };
    juce::StringArray kickCharacterNames();

    /** The filter's types ("Off", "Low-pass", "Band-pass", "High-pass") and the delay's times
        ("1/16" to "1/4 dotted"), with each time's length in beats. */
    juce::StringArray filterNames();
    juce::StringArray delayTimeNames();
    double delayBeats (int index);

    /** Everything a kit sets. */
    struct KitSettings
    {
        KitSettings()
        {
            for (int i = 0; i < numUserPads; ++i)
                user[(size_t) i].note = firstUserNote + i;
        }

        std::array<Settings, (size_t) numSounds> sounds {};
        std::array<UserPad, (size_t) numUserPads> user {};
        KickCharacter kick = KickCharacter::punch;
        float drive = 0.0f;     // %, 0 to 100: the whole kit through a soft clipper
        float glue = 0.0f;      // %, 0 to 100: a bus compressor, threshold and ratio together
        float level = -10.0f;   // dB, -48 to 6 (Standard's leaves a full kit struck at once under 0 dB)
        float reverbSize = 50.0f;   // %, 0 (a small room) to 100 (a hall)
        int delayTime = 2;          // an index into delayTimeNames(), 1/8 to start with
        float feedback = 35.0f;     // %, 0 to 100: how much of each echo comes round again

        bool operator== (const KitSettings& o) const
        {
            return sounds == o.sounds && user == o.user && kick == o.kick && juce::exactlyEqual (drive, o.drive) && juce::exactlyEqual (glue, o.glue) && juce::exactlyEqual (level, o.level)
                && juce::exactlyEqual (reverbSize, o.reverbSize) && delayTime == o.delayTime && juce::exactlyEqual (feedback, o.feedback);
        }
    };

    /** A sound's settings as the engine starts (the Standard kit's). */
    Settings defaultsFor (Sound);
    KitSettings defaultKit();

    /** The factory kits, Standard (the defaults) first; the kits made of Modern sounds come after
        the seven that were there before them. */
    struct Kit
    {
        juce::String name, description;
        KitSettings settings;
    };

    const std::vector<Kit>& kits();
    int kitIndex (const juce::String& name);   // -1 if there is none of that name

    /** The synthesis. A voice for each note, so a tom fill or two congas ring over each other; a
        note struck again, or choked, fades its last hit out over 3 ms instead of stopping it dead,
        and All Sound Off fades every hit the same way. Every hit rises from silence over a fraction
        of a millisecond and falls to nothing at its end, so nothing steps. Sample by sample
        throughout, so a pattern renders the same at any block size; its noise is seeded per note,
        so the same each time. Allocates nothing after prepare. */
    class Engine
    {
    public:
        Engine();
        ~Engine();

        void prepare (double sampleRate);
        void reset();

        /** The settings the next hits use (each hit keeps the ones it started with); the kit-wide
            drive, glue and level glide to theirs over 20 ms. */
        void setSettings (const KitSettings&);
        const KitSettings& getSettings() const noexcept   { return settings; }

        /** The pads' layers the next hits of a Sample pad play; a hit already sounding goes on with
            the sample it started with, which whoever set it must keep until oldestSamplesInUse()
            has passed its generation. */
        void setSamples (SamplesPtr);
        juce::uint32 oldestSamplesInUse() const noexcept;

        /** A note: its sound, if it is one of 35 to 81, and every user pad on that note. */
        void noteOn (int note, float velocity);

        /** One pad alone, as a click on it plays it: a sound on its pad's note, or a user pad
            without whatever else is on its note. */
        void padOn (int pad, float velocity);

        /** The tempo the delay's times follow (120 until told). */
        void setTempo (double bpm);

        /** Fades every hit out over 3 ms, and the reverb and delay with them. */
        void allSoundOff();

        /** Writes count samples of the kit (replacing what is there). right may be null for mono:
            the two channels mixed. */
        void render (float* left, float* right, int count);

        /** Renders count samples of the sounds alone, before the kit's drive, glue and level, added to
            what is there; with send buffers, what each sends to the reverb and delay added to those. */
        void renderHits (float* left, float* right, int count, float* toReverbL = nullptr, float* toReverbR = nullptr,
                         float* toDelayL = nullptr, float* toDelayR = nullptr);

        int getNumSounding() const;

        /** For lights: a count of each sound's hits, and the last one's velocity. Any thread. */
        juce::uint32 hitsOf (Sound s) const noexcept               { return hitCount[(size_t) s].load (std::memory_order_relaxed); }
        float lastVelocityOf (Sound s) const noexcept              { return lastVelocity[(size_t) s].load (std::memory_order_relaxed); }
        juce::uint32 hitsOfPad (int pad) const noexcept            { return hitCount[(size_t) pad].load (std::memory_order_relaxed); }
        float lastVelocityOfPad (int pad) const noexcept           { return lastVelocity[(size_t) pad].load (std::memory_order_relaxed); }

        /** The layer a pad's last hit played, counted from 0 in the pad's list; -1 when it played
            none (no layer takes that velocity, or the pad does not play samples). */
        int lastLayerOfPad (int pad) const noexcept                { return lastLayer[(size_t) pad].load (std::memory_order_relaxed) - 1; }

        /** The furthest a pad's compressor has turned its last hit down so far, in dB (0 or less;
            0 with the compressor off). Any thread. */
        float padReductionDb (int pad) const noexcept              { return padReduction[(size_t) pad].load (std::memory_order_relaxed); }

        /** How far the glue is turning the kit down, in dB (0 or less). Any thread. */
        float getReductionDb() const noexcept                       { return reductionDb.load (std::memory_order_relaxed); }

        /** Renders main stereo mix plus up to numAuxBuses separate stereo pad buses (pads with
            outputBus > 0 route to auxL/auxR[outputBus - 1] instead of main). */
        void renderMultiOut (float* mainL, float* mainR,
                             std::array<float*, numAuxBuses> auxL, std::array<float*, numAuxBuses> auxR,
                             int count);

        /** One hit of a sound (its pad's note unless note is given) at full velocity alone,
            through nothing else: what the window draws. */
        static juce::AudioBuffer<float> renderOne (const KitSettings&, Sound, double sampleRate, double seconds, int note = -1, SamplesPtr = {});

        /** The same for any pad, a user pad among them. */
        static juce::AudioBuffer<float> renderPad (const KitSettings&, int pad, double sampleRate, double seconds, SamplesPtr = {});

        struct Hit;

    private:
        void start (Hit&, int pad, int note, float velocity);
        void cut (int note);
        void cutUser (int userPad);
        void silence (int pad, int note);
        void begin (int note, float velocity);
        void beginUser (int userPad, float velocity);
        void renderBlock (float* left, float* right, int count);
        void renderEffects (float* left, float* right, int count);
        void clearEffects();
        void setReverbSize();
        float delayTarget() const;

        double rate = 48000.0;
        KitSettings settings;
        SamplesPtr samples;
        std::array<juce::uint32, (size_t) numPads> turns {};     // each pad's hits on its samples so far: whose turn it is
        std::vector<Hit> hits, fading;   // one each per note
        std::vector<Hit> userHits, userFading;   // and one each per user pad
        int fadeLength = 144;

        // The kit's bus: drive, glue and level, smoothed.
        juce::SmoothedValue<float> driveAmount, glueAmount, outputGain;
        float glueEnvelope = 0.0f, glueAttack = 0.0f, glueRelease = 0.0f;

        // The sends' effects: a reverb, and a ping-pong delay timed to the tempo.
        static constexpr int chunk = 256;
        std::array<float, chunk> sendReverbL {}, sendReverbR {}, sendDelayL {}, sendDelayR {};
        juce::Reverb reverb;
        float reverbSizeSet = -1.0f;
        std::vector<float> delayL, delayR;
        int delayWrite = 0;
        double tempo = 120.0;
        float delaySamples = 0.0f, delayGlide = 0.0f, delayToneK = 0.0f, delayToneL = 0.0f, delayToneR = 0.0f;
        int effectsFade = 0;    // samples left of the effects fading after All Sound Off
        juce::SmoothedValue<float> feedbackAmount;

        std::array<std::atomic<juce::uint32>, (size_t) numPads> hitCount {};
        std::array<std::atomic<float>, (size_t) numPads> lastVelocity {};
        std::array<std::atomic<int>, (size_t) numPads> lastLayer {};   // one more than the layer; 0 for none
        std::array<std::atomic<float>, (size_t) numPads> padReduction {};
        std::atomic<float> reductionDb { 0.0f };
    };
}

namespace BuiltInEffects
{
    juce::PluginDescription basicDrumsDescription();

    /** Basic Drums, the drum machine built in (DrumMachine), as an instrument: each sound's tune,
        decay, tone, level, pan and drive, its type, the kick's character and the
        kit's drive, glue and level are hosted parameters, so automation and sessions find them. A
        session saved before there were types has none, and plays every sound Classic, as it did. A
        pad's samples (its layers) are kept in the state, each named by its file. Stereo out. A kit sets them all
        at once; the state keeps the kit's name. Sessions saved before 0.20.0's step 10 (a kick,
        snare, hat and tom tune and decay in percent) load into the nearest settings: the same
        tunings, their decays scaled from the new sounds' own. MIDI: notes, CC 7 volume, CC 121 and
        All Sound Off, which fades rather than cuts. */
    class BasicDrumsProcessor final : public BuiltInEffect
    {
    public:
        BasicDrumsProcessor();

        void prepareToPlay (double sampleRate, int maximumBlockSize) override;
        void reset() override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
        double getTailLengthSeconds() const override              { return 10.0; }
        juce::AudioProcessorEditor* createEditor() override;

        using Sound = DrumMachine::Sound;
        static std::optional<Sound> soundFor (int note)            { return DrumMachine::soundFor (note); }

        /** How many hits are sounding. */
        int getNumSounding() const                                 { return engine.getNumSounding(); }

        /** The settings its parameters hold now. */
        DrumMachine::KitSettings currentSettings() const;

        /** Sets every parameter to a kit's (as host changes) and takes its name. Message thread. */
        void applyKit (const DrumMachine::Kit&);
        void applySettings (const DrumMachine::KitSettings&);

        /** The kit last chosen ("Standard" to start with); whether the parameters still hold it exactly. */
        juce::String getKitName() const                            { return kitName; }
        bool kitModified() const;

        /** Plays a note now, as a pad does: added to the next block. Any thread. */
        void audition (int note, float velocity);

        /** Plays one pad alone (0 to 63), a user pad without whatever else is on its note. Any thread. */
        void auditionPad (int pad, float velocity);

        //==============================================================================
        // The 64 pads: the 30 sounds first (a sound's pad is its place in DrumMachine::Sound), then
        // the 34 user pads, which play samples only. What follows takes any of them by its number.

        /** A pad's name: a sound's own, or a user pad's ("Pad 31" until it is given one); its
            colour (none set: transparent, and the window picks one); and its note. A sound's pad
            can be given a name and a colour too, which show while its type is Sample (its note is
            its own and stays). Names, colours and notes are kept in the state and in presets. Message thread. */
        juce::String getPadName (int pad) const;
        void setPadName (int pad, const juce::String&);
        juce::Colour getPadColour (int pad) const;
        void setPadColour (int pad, juce::Colour);
        int getPadNote (int pad) const;
        void setPadNote (int pad, int note);

        /** A user pad back as it starts: no layers, its own name, colour, note and settings. */
        void clearPad (int pad);

        /** The source a pad plays in place of its own sound or samples (DrumMachine::sourceInfo): 0
            for none, or 1 to DrumMachine::numSources. Choosing one sets the pad's decay to the
            source's own; a pad with a source and no name of its own is called by the source's.
            Kept in the state, in kit files and in presets, by the source's id. Message thread to
            set; any thread to read. */
        int getSource (int pad) const;
        void setSource (int pad, int source);

        /** The choke group a pad is in: 0 for none, or 1 to DrumMachine::numChokeGroups. Pads in one
            group cut each other: a hit of one fades whatever of the others is sounding (and, for a
            sound with two notes, its other note). Kept in the state, in kit files and in presets;
            a factory kit has none. Message thread to set; any thread to read. */
        int getChoke (int pad) const;
        void setChoke (int pad, int group);

        /** A pad's own effects (DrumMachine::Fx): its envelope, transient shaper, saturator, EQ and
            compressor. They are kept in the state, in kit files and in presets, and are not hosted
            parameters: a hit takes them as they are when it is struck. Setting one puts it inside
            its range. Message thread to set; any thread to read. */
        float getEffect (int pad, DrumMachine::Fx) const;
        void setEffect (int pad, DrumMachine::Fx, float value);
        DrumMachine::PadEffects getEffects (int pad) const;
        void setEffects (int pad, const DrumMachine::PadEffects&);

        /** Counts up when any pad's effects change. */
        juce::uint32 getEffectsVersion() const noexcept             { return effectsVersion.load(); }

        /** The kit as the piano roll's drum grid lists it: General MIDI's 47 rows, a row of a
            sound's pad that plays samples under the name given to it, and a row for each user pad
            that has samples, under its name on its note (after the 47, lowest note first). A user
            pad on a note that has a row already is added to that row's name ("Bass Drum 1 + Kick
            Layer"), as are two user pads on one note. With no samples and no names it is General
            MIDI's map exactly. Message thread. */
        std::vector<DrumSound> drumSounds() const;

        /** Whether a pad has a name or a colour given to it (shown or not). */
        bool hasOwnLook (int pad) const;

        /** Counts up when a user pad's name, colour or note changes. */
        juce::uint32 getPadsVersion() const noexcept                { return padsVersion.load(); }

        juce::AudioParameterFloat* parameterOf (int pad, const juce::String& what) const;
        juce::AudioParameterChoice* filterParameter (int pad) const noexcept  { return of (pad).filter; }
        juce::AudioParameterBool* reverseParameter (int pad) const noexcept   { return of (pad).reverse; }

        /** Whether a pad plays samples now: a user pad, or a sound whose type is Sample, that has no source. */
        bool playsSamples (int pad) const;

        const DrumMachine::Engine& getEngine() const noexcept      { return engine; }

        /** A parameter of a sound, by its suffix: "tune", "decay", "tone", "level", "pan", "drive",
            "cutoff", "resonance", "crush", "reverb", "delay", and for a Sample "start" or "bend"; and its filter's type. */
        juce::AudioParameterFloat* parameterOf (Sound, const juce::String& what) const;
        juce::AudioParameterChoice* filterParameter (Sound s) const noexcept { return sounds[(size_t) s].filter; }

        /** A sound's type: a choice among DrumMachine::typeNamesFor (s). */
        juce::AudioParameterChoice* typeParameter (Sound s) const noexcept   { return sounds[(size_t) s].type; }

        /** A sound's type as the engine knows it (DrumMachine::classicType, modernType, sampleType);
            setting one the sound hasn't (Modern, for nine of them) sets Classic. */
        int getType (Sound) const;
        void setType (Sound, int type);

        /** A Sample's own settings: where it starts ("start"), how far off pitch ("bend"), and backwards or not. */
        juce::AudioParameterBool* reverseParameter (Sound s) const noexcept  { return sounds[(size_t) s].reverse; }

        //==============================================================================
        // Samples on the pads. Message thread; the hits take them up at their next block.

        /** A pad's layers, in order. Setting them keeps the first DrumMachine::maxLayers. */
        std::vector<DrumMachine::Layer> getLayers (int pad) const;
        void setLayers (int pad, std::vector<DrumMachine::Layer>);
        std::vector<DrumMachine::Layer> getLayers (Sound s) const                  { return getLayers ((int) s); }
        void setLayers (Sound s, std::vector<DrumMachine::Layer> layers)           { setLayers ((int) s, std::move (layers)); }

        /** Sounds given to a pad, as a drop on it gives them: each becomes a layer over every
            velocity after the ones there (so they take turns), the pad's type becomes Sample, and
            its decay is made long enough for the longest of them to play whole. An error (no room
            for a layer or for its memory: what fitted is kept), or empty. */
        juce::String addSamples (int pad, const std::vector<std::pair<Sampler::SamplePtr, juce::File>>&);
        juce::String addSamples (Sound s, const std::vector<std::pair<Sampler::SamplePtr, juce::File>>& given)   { return addSamples ((int) s, given); }

        /** Files given to a pad: read through the store every sampler shares, then as addSamples. */
        juce::String addFiles (int pad, const juce::Array<juce::File>&);
        juce::String addFiles (Sound s, const juce::Array<juce::File>& files)      { return addFiles ((int) s, files); }

        /** Counts up each time any pad's layers are set, so a window knows to read them again. */
        juce::uint32 getSamplesVersion() const noexcept                { return samplesVersion.load(); }
        DrumMachine::SamplesPtr getSamples() const;

        /** Samples named in its state that could not be read when it was last restored, with why;
            the files its layers name, each once; and the memory its samples take, each counted once. */
        juce::StringArray getMissing() const                           { return missing; }
        juce::Array<juce::File> getFiles() const;
        juce::int64 bytesHeld() const;

        /** Looks for its missing samples, as Torqwave Sampler does: `found` is one of them where it
            is now, or a folder they may be in. How many layers got their sample back. */
        int locate (const juce::File& found);

        /** Writes the kit as it is now into a folder (made if need be): a kit file named after the
            folder, and beside it, in Samples, a copy of every sample its layers play, which the kit
            file names relative to itself, so the folder can be moved to another computer. Files that
            aren't there are added to `missing`. An error, or empty. */
        juce::String saveKitWithSamples (const juce::File& folder, juce::StringArray& missing);

        /** Loads a kit file saved that way: its settings, and its samples from beside it. An error, or empty. */
        juce::String loadKitFile (const juce::File&);
        static constexpr const char* kitFileExtension = ".tqdrums";

        /** What a preset carries besides its values: the pads' layers ("samples", as the state keeps
            them). Applying texts without any takes every pad's layers away: a preset is a whole kit. */
        EffectPresets::Texts getPresetTexts() const;
        void setPresetTexts (const EffectPresets::Texts&);
        void setPresetValues (const EffectPresets::Values&) override;

        /** Multi-out bus assignment for each pad: 0 = Main, 1..numAuxBuses = Aux 1..8. */
        int getOutputBus (int pad) const;
        void setOutputBus (int pad, int bus);

        /** Optional host callback: drops an imported MIDI pattern (such as a Maschine .mxgrp groove)
            directly onto this Basic Drums track at the session cursor. Returns true if placed. */
        std::function<bool (const juce::MidiFile&, const juce::String& clipName)> onDropPattern;

        /** For the tests: what it may hold, in place of Sampler::mostBytes. */
        void setMostBytes (juce::int64 bytes)                          { most = bytes; }
        juce::AudioParameterChoice* kickCharacterParameter() const noexcept   { return kickCharacter; }
        juce::AudioParameterFloat* kitDriveParameter() const noexcept         { return kitDrive; }
        juce::AudioParameterFloat* glueParameter() const noexcept             { return glue; }
        juce::AudioParameterFloat* levelParameter() const noexcept            { return level; }
        juce::AudioParameterFloat* reverbSizeParameter() const noexcept       { return reverbSize; }
        juce::AudioParameterChoice* delayTimeParameter() const noexcept       { return delayTime; }
        juce::AudioParameterFloat* feedbackParameter() const noexcept         { return feedback; }

    private:
        void addToState (juce::XmlElement&) const override;
        void readFromState (const juce::XmlElement&) override;

        struct SoundParameters
        {
            juce::AudioParameterFloat *tune = nullptr, *decay = nullptr, *tone = nullptr, *level = nullptr, *pan = nullptr, *drive = nullptr;
            juce::AudioParameterChoice *filter = nullptr, *type = nullptr;
            juce::AudioParameterFloat *cutoff = nullptr, *resonance = nullptr, *crush = nullptr, *reverb = nullptr, *delay = nullptr;
            juce::AudioParameterFloat *start = nullptr, *bend = nullptr;
            juce::AudioParameterBool* reverse = nullptr;
        };

        void setSamples (DrumMachine::PadLayers);
        void clearSamples();
        void writeSamples (juce::XmlElement& into) const;
        void readSamples (const juce::XmlElement& from);
        void writePads (juce::XmlElement& into) const;
        void readPads (const juce::XmlElement& from);
        void resetPads();
        void writeEffects (juce::XmlElement& into) const;
        void readEffects (const juce::XmlElement& from);
        static void setUserDefaults (const SoundParameters&);
        static juce::String idOfPad (int pad);
        const SoundParameters& of (int pad) const noexcept;

        std::array<SoundParameters, (size_t) DrumMachine::numSounds> sounds;
        std::array<SoundParameters, (size_t) DrumMachine::numUserPads> users;   // the user pads': no type, they play samples

        // The user pads' names, colours (transparent: none chosen) and notes.
        std::array<juce::String, (size_t) DrumMachine::numUserPads> userNames;
        std::array<juce::Colour, (size_t) DrumMachine::numUserPads> userColours;
        std::array<juce::String, (size_t) DrumMachine::numSounds> soundNames;      // a sound's pad's, shown while it is a Sample
        std::array<juce::Colour, (size_t) DrumMachine::numSounds> soundColours;
        std::array<std::atomic<int>, (size_t) DrumMachine::numUserPads> userNotes;
        std::array<std::atomic<int>, (size_t) DrumMachine::numPads> chokes {};
        std::array<std::atomic<int>, (size_t) DrumMachine::numPads> sources {};
        std::array<std::atomic<int>, (size_t) DrumMachine::numPads> outputBuses {};
        std::atomic<juce::uint32> padsVersion { 0 };

        // Every pad's effects, a value at a time so the audio thread reads them without waiting.
        std::array<std::array<std::atomic<float>, (size_t) DrumMachine::numFx>, (size_t) DrumMachine::numPads> effects;
        std::atomic<juce::uint32> effectsVersion { 0 };
        juce::AudioParameterChoice* kickCharacter = nullptr;
        juce::AudioParameterFloat *kitDrive = nullptr, *glue = nullptr, *level = nullptr, *reverbSize = nullptr, *feedback = nullptr;
        juce::AudioParameterChoice* delayTime = nullptr;
        DrumMachine::Engine engine;
        float volume = 1.0f;   // CC 7, squared as MIDI's volume usually is
        juce::String kitName { "Standard" };

        // Pads clicked in the window, waiting for the audio thread.
        juce::SpinLock auditionLock;
        std::vector<std::pair<int, float>> auditions;

        // The pads' layers: swapped under the lock, which the audio thread only tries. Ones taken
        // away are kept until no hit plays from them.
        DrumMachine::SamplesPtr samples, pendingSamples;
        std::vector<DrumMachine::SamplesPtr> retiredSamples;
        juce::SpinLock samplesLock;
        std::atomic<bool> samplesChanged { false };
        std::atomic<juce::uint32> oldestSamplesInUse { 0 }, samplesVersion { 0 };
        juce::uint32 samplesGenerations = 0;
        juce::StringArray missing;
        juce::File kitFolder;                  // while a kit file is read: where its samples are beside
        juce::int64 most = Sampler::mostBytes;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BasicDrumsProcessor)
    };
}
