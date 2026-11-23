#pragma once

#include "BuiltInEffects.h"
#include "Instrument.h"
#include "Markers.h"

struct DrumSound;

/** The engine behind Torqwave Sampler: samples played across the keys in zones, each at the played
    note's pitch by resampling from the sample's own rate, through an amp envelope, a filter with
    an envelope of its own and an LFO. It knows nothing of plug-ins, so the window can render with
    it and the tests can play it bare.

    A sample is read through a windowed sinc (48 points at the sample's own pitch or below, more
    above it, where the band is narrowed to what the output can hold), so playing up doesn't
    alias. At its own pitch and rate a sample plays as it is, frame for frame. Nothing stops dead: a note taken for a new one, and All Sound Off, fade over 4 ms; a
    note rises over a quarter of a millisecond at least; a sample's end is faded over half a
    millisecond. A voice's controls are taken afresh every 16 samples counted from its note's
    start, so a part renders the same at any block size and the same twice. Allocates nothing
    after prepare. */
namespace Sampler
{
    constexpr int numVoices = 32, numFadingVoices = 8, maxZones = 128;
    constexpr int controlInterval = 16;
    constexpr double fadeSeconds = 0.004, minimumAttackSeconds = 0.00025, endFadeSeconds = 0.0005;

    /** A sound in memory: its audio (one or two channels) at its own rate. Never changed once made. */
    struct Sample
    {
        juce::AudioBuffer<float> audio;
        double rate = 48000.0;
        juce::String name;
        juce::File file;               // where it was read from; none for one made in memory
        Markers markers;               // its file's markers and regions, in its frames

        // What a WAV file's sampler chunk says, when it has one: the key it was recorded at (below
        // 0, none), how many cents to tune it by, and its loop (0 none, 1 forward, 2 forward and back).
        int fileRoot = -1;
        float fileTune = 0.0f;
        int fileLoop = 0;
        juce::int64 fileLoopStart = 0, fileLoopEnd = 0;

        juce::int64 bytes() const noexcept         { return (juce::int64) audio.getNumChannels() * audio.getNumSamples() * (juce::int64) sizeof (float); }

        int getNumFrames() const noexcept          { return audio.getNumSamples(); }
    };

    using SamplePtr = std::shared_ptr<const Sample>;

    /** A sample from audio in memory: its first two channels. */
    SamplePtr makeSample (const juce::AudioBuffer<float>&, double rate, const juce::String& name, const Markers& = {});

    /** A sample read whole from a file (any type Torqwave opens): its first two channels. Nothing,
        with the reason in error, for a file that can't be read or holds no audio. */
    SamplePtr loadSample (const juce::File&, juce::AudioFormatManager&, juce::String& error);

    //==============================================================================
    // Samples are held whole in memory. The limits: one sample's length, and what one sampler holds.
    constexpr double longestSampleSeconds = 600.0;
    constexpr juce::int64 mostBytes = (juce::int64) 1 << 30;

    /** A file's sample from the store every sampler shares, read (as loadSample does) when no
        sampler holds it or the file has changed since: two samplers on one file hold it once.
        Nothing, with the reason, for a file that can't be read or is longer than longestSampleSeconds. Any thread. */
    SamplePtr sharedSample (const juce::File&, juce::String& error);

    /** The folder sounds that are not files (an editor selection, an unsaved file) are written into
        to be sampled: Music > Torqwave > Samples. */
    juce::File samplesFolder();

    /** Writes audio as a 32-bit float WAV with its markers into a folder (samplesFolder() by
        default) under a name not yet used there, and gives the file; none, with the reason, if it can't. */
    juce::File writeSample (const juce::AudioBuffer<float>&, double rate, const juce::String& name, const Markers&, juce::String& error,
                            juce::File folder = {});

    /** The folder of the session last opened (none before any): where a sample named relative to
        its session is looked for first. Any thread. */
    void setSessionFolder (const juce::File&);
    juce::File getSessionFolder();

    /** While one lives, a sampler's state names each sample relative to this folder as well as by
        its full path: what saving a session wraps round the writing of its tracks' effects. */
    class SavingInto
    {
    public:
        explicit SavingInto (const juce::File& sessionFolder);
        ~SavingInto();

    private:
        juce::File before;
    };

    /** In a track's effects (as TrackEffects::createXml gives them), copies every sample its
        samplers name into `folder` (under its own name, numbered when another file has it), and
        names the copies in their states, relative to sessionFolder too. The copies made are added
        to `copies`, files that aren't there to `missing`. An error, or empty. */
    juce::String collectFiles (juce::XmlElement& effects, const juce::File& sessionFolder, const juce::File& folder,
                               std::vector<std::pair<juce::File, juce::File>>& copies, juce::StringArray& missing);

    /** A sound's pitch as a zone needs it: the key it is nearest, and the cents to tune it by so
        that key plays it in tune. Measured over half a second after the first 20 ms of the part
        (all of a shorter one), as Decrypt measures a note. Not found for a sound with no clear
        pitch between 30 Hz and 2 kHz: then the root is C4 (60). */
    struct Pitch
    {
        bool found = false;
        double hz = 0.0;
        int root = 60;
        float tune = 0.0f;
    };

    Pitch findPitch (const Sample&, juce::Range<juce::int64> part = {});

    /** A loop suggested for a part of a sample (all of it, for an empty part). For a sound with a
        pitch: in the held part of the note, a start and an end a whole number of its cycles apart
        where the sound either side of the two is most alike, both on a rising zero crossing, with
        a short crossfade. For one without: the later part of the sound under a long crossfade.
        Not found for a part shorter than a fifth of a second. */
    struct LoopFound
    {
        bool found = false;
        juce::int64 start = 0, end = 0;
        float crossfade = 0.0f;        // ms
    };

    LoopFound findLoop (const Sample&, juce::Range<juce::int64> part = {}, double hz = 0.0);

    /** Where a sample plays: the keys and velocities that play it, and the note it was recorded at. */
    struct Zone
    {
        SamplePtr sample;
        juce::File file;               // where its sample is from: kept when the file can't be read, so it can be found again
        juce::uint32 id = 0;           // its own, kept through edits (0: given one when it is set)
        juce::String name;             // a slice's: its marker's or region's name, or "Slice 3"
        int root = 60;                 // the key that plays it at its own pitch
        float tune = 0.0f;             // cents, -100 to 100
        int lowKey = 0, highKey = 127;
        int lowVelocity = 1, highVelocity = 127;
        juce::int64 start = 0, end = -1;   // the part of the sample played, in its frames; an end below 0 is the sample's
        float level = 0.0f;            // dB, -48 to 12
        float pan = 0.0f;              // -100 (left) to 100 (right)

        bool reverse = false;          // the part played backwards
        bool oneShot = false;          // plays to its end whatever the note's length (and never loops)
        int loop = 0;                  // 0 none, 1 forward, 2 forward and back
        juce::int64 loopStart = 0, loopEnd = -1;   // in the sample's frames; an end below 0 is the part's
        float crossfade = 0.0f;        // ms of the loop's end crossfaded into what comes before its start (forward loops)
        bool loopInRelease = true;     // the loop goes on while the note dies away; off, a note let go plays on to the end
        int choke = 0;                 // a group whose zones cut each other off; 0, none

        /** The part played, held to the sample: [first, last). Empty with no sample. */
        juce::Range<juce::int64> span() const;

        bool plays (int key, int velocity) const noexcept
        {
            return sample != nullptr && key >= lowKey && key <= highKey && velocity >= lowVelocity && velocity <= highVelocity;
        }
    };

    /** The zones a sampler plays, as the voices read them: never changed once made. */
    struct Program
    {
        /** A zone as a voice reads it. A voice counts frames along the way it plays (from 0 at the
            part's start, or at its end when reversed), and the loop is in those. */
        struct Prepared
        {
            juce::int64 first = 0, last = 0, length = 0;   // the part, in the sample's frames
            bool reverse = false, loopsInRelease = true;
            int loop = 0;                                  // as the zone's, 0 when it can't loop
            juce::int64 loopStart = 0, loopEnd = 0, period = 0, fade = 0;
            juce::AudioBuffer<float> turn;                 // the last `fade` frames of a forward loop, crossfaded
        };

        std::vector<Zone> zones;
        std::vector<Prepared> prepared;                    // one for each zone
        juce::uint32 generation = 0;
    };

    using ProgramPtr = std::shared_ptr<const Program>;

    /** The zones made ready to play (the first maxZones of them): each loop held to its part, and
        a forward loop's crossfade worked out, with the same equal-power fade as Crossfade Loop,
        into a buffer of its own, so the sample is never changed. */
    ProgramPtr makeProgram (std::vector<Zone>, juce::uint32 generation = 0);

    //==============================================================================
    // Slices: one sample cut into pieces, each on a key of its own from C2 (36) up.
    constexpr int maxSlices = 64, firstSliceKey = 36;

    enum class SliceSource { markers, regions, hits, equalParts };
    juce::StringArray sliceSourceNames();

    /** How a sampler playing slices was cut, kept so its window can cut again. */
    struct Slicing
    {
        bool on = false;               // slices rather than zones across the keys
        SliceSource source = SliceSource::hits;
        float sensitivity = 70.0f;     // %, for hits: 100 keeps every attack found, less only the louder ones
        int parts = 16;                // for equal parts: 2 to 64
        bool cutEachOther = false;     // each slice stops the last

        bool operator== (const Slicing& o) const noexcept
        {
            return on == o.on && source == o.source && juce::exactlyEqual (sensitivity, o.sensitivity) && parts == o.parts && cutEachOther == o.cutEachOther;
        }
    };

    struct Slice
    {
        juce::int64 start = 0, end = 0;    // in the sample's frames
        juce::String name;
    };

    /** The pieces a part of a sample is cut into, in order, at most maxSlices:
        - markers: from each point marker in the part to the next (or the part's end), named after
          it, with a piece named "Start" from the part's start to the first when that is later;
        - regions: each region that lies in the part, named after it;
        - hits: from the part's start and from each attack found in it (those too soft for the
          sensitivity left out, and when there are too many, the softest), each cut moved to the
          nearest zero crossing within 2 ms, named "Slice 1" on;
        - equal parts: that many pieces of one length. */
    std::vector<Slice> findSlices (const Sample&, juce::Range<juce::int64> part, const Slicing&);

    /** A zone for each slice, on the keys from firstSliceKey: one-shot, at its own pitch on its
        key, all in one choke group when they cut each other. */
    std::vector<Zone> zonesFromSlices (const SamplePtr&, const juce::File&, const std::vector<Slice>&, bool cutEachOther);

    /** The notes that play slices in order at the times they have in the sample (from the
        earliest's start), each held for its slice's length: the sample as it was, as notes. */
    std::vector<Instrument::Note> notesFromSlices (const std::vector<Zone>&);

    /** A drum map with a row for each zone that sits on one key, named after it, lowest key first. */
    std::vector<DrumSound> drumMapFor (const std::vector<Zone>&);

    enum class FilterType { off, lowPass12, lowPass24, bandPass, highPass };
    juce::StringArray filterNames();

    enum class LfoShape { sine, triangle, saw, square, random };
    juce::StringArray lfoShapeNames();

    /** The LFO's rates in note values ("Off" first: its rate is in Hz), and each one's length in beats. */
    juce::StringArray lfoSyncNames();
    double lfoSyncBeats (int index);

    /** Everything that isn't a zone's, in the units the knobs show. */
    struct Settings
    {
        float level = -6.0f;           // dB, -48 to 6
        float pan = 0.0f;              // -100 to 100
        float transpose = 0.0f;        // semitones, -24 to 24
        float fine = 0.0f;             // cents, -100 to 100
        float bendRange = 2.0f;        // semitones, 0 to 24
        bool mono = false;             // one note at a time
        float glide = 0.0f;            // ms, 0 to 3,000: each note slides from the last
        float velocity = 80.0f;        // %, how much velocity sets the level

        float attack = 0.0f, decay = 300.0f, sustain = 100.0f, release = 100.0f;   // ms, ms, %, ms

        FilterType filter = FilterType::off;
        float cutoff = 20000.0f;       // Hz, 20 to 20,000
        float resonance = 0.0f;        // %, 0 to 100
        float keyTrack = 0.0f;         // %, how far the cutoff follows the key (around C4)
        float filterEnvelope = 0.0f;   // %, -100 to 100: 100 opens it eight octaves
        float filterAttack = 0.0f, filterDecay = 300.0f, filterSustain = 0.0f, filterRelease = 100.0f;
        float velocityToCutoff = 0.0f; // %, 100 opens it four octaves at full velocity

        float lfoRate = 5.0f;          // Hz, 0.02 to 40
        int lfoSync = 0;               // an index into lfoSyncNames()
        LfoShape lfoShape = LfoShape::sine;
        float lfoFade = 0.0f;          // ms, 0 to 5,000: how long it takes to come in
        float lfoPitch = 0.0f;         // cents, 0 to 1,200
        float lfoFilter = 0.0f;        // %, -100 to 100: 100 is four octaves either way
        float lfoLevel = 0.0f;         // %, 0 to 100
        float lfoPan = 0.0f;           // %, 0 to 100
        float wheel = 50.0f;           // cents of vibrato the mod wheel adds at its top, 0 to 200
    };

    /** A sample's value part way between its frames, read through the windowed sinc: what a voice
        hears at `position` frames when it plays `ratio` frames of the sample a sample of output.
        For the tests and the window; the voices use the same tables. */
    float interpolate (const float* samples, int numSamples, double position, double ratio);

    class Engine
    {
    public:
        Engine();
        ~Engine();

        void prepare (double sampleRate);

        /** Silence at once, and every controller back where it starts. */
        void reset();

        void setSettings (const Settings&);
        const Settings& getSettings() const noexcept      { return settings; }

        /** The zones the next notes play. A note already sounding goes on with the zone of the same
            id and sample when the new zones have one, and fades out over 4 ms when they don't. */
        void setProgram (ProgramPtr);

        /** The oldest zones any voice still reads (their generation): older ones can be let go. */
        juce::uint32 oldestGenerationInUse() const noexcept;

        void noteOn (int note, float velocity);
        void noteOff (int note);
        void allNotesOff();
        void allSoundOff();
        void pitchBend (float semitonesOfRange);   // -1 to 1: the wheel's place
        void controller (int number, int value);

        /** The tempo the LFO's note values follow (120 until told). */
        void setTempo (double bpm);

        /** Adds count samples of what is sounding to left and right (right may be null: the two mixed). */
        void render (float* left, float* right, int count);

        /** How many notes are sounding, not counting voices fading out. */
        int getNumSounding() const;

        /** For the window: where each sounding voice is in its sample (frames; below 0, not
            sounding) and its zone's id. Any thread. */
        struct Reading { std::atomic<float> position { -1.0f }; std::atomic<juce::uint32> zone { 0 }; };
        const std::array<Reading, (size_t) numVoices + numFadingVoices>& getReadings() const noexcept    { return readings; }

        struct Voice;

    private:
        Voice* take();
        void start (Voice&, const Zone&, int note, float velocity);
        void letGo (Voice&);
        void fade (Voice&);
        void steer (Voice&);
        void renderVoice (Voice&, float* left, float* right, int count);

        double rate = 48000.0;
        Settings settings;
        ProgramPtr program;
        std::vector<Voice> voices;
        std::vector<float> scratchL, scratchR;
        juce::uint32 counter = 0;
        double tempo = 120.0;
        float bend = 0.0f, wheel = 0.0f, volume = 1.0f, expression = 1.0f, panning = 0.0f;
        bool pedal = false;
        float lastNote = -1.0f;        // the note the next glides from; below 0, none yet
        float gainL = 0.0f, gainR = 0.0f;   // the output's gains, smoothed sample by sample
        bool gainsSet = false;
        std::array<Reading, (size_t) numVoices + numFadingVoices> readings;
    };
}

namespace BuiltInEffects
{
    constexpr const char* samplerId = "torqwave:sampler";

    juce::PluginDescription samplerDescription();

    /** Torqwave Sampler, the sampler built in (Sampler::Engine), as an instrument: its settings are
        hosted parameters, so automation, presets and sessions find them, and its zones are kept in
        its state, each sample named by its file. Stereo out, no audio input. MIDI: notes with
        velocity, pitch bend, the mod wheel (vibrato), volume (7), pan (10), expression (11), the
        sustain pedal (64), All Notes Off and All Sound Off, which fades rather than cuts. */
    class SamplerProcessor final : public BuiltInEffect
    {
    public:
        SamplerProcessor();
        ~SamplerProcessor() override;

        void prepareToPlay (double sampleRate, int maximumBlockSize) override;
        void reset() override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
        double getTailLengthSeconds() const override;
        juce::AudioProcessorEditor* createEditor() override;

        /** A sound added, as a drop on its window adds one. Playing slices, it is cut as the slicing
            says (at its hits when it has no markers or regions to cut at) in place of what was
            there. Otherwise it becomes a zone: in place of the zone `replace` (an id) keeping that
            zone's keys, or a new zone over every key when it is the first, or over the octave from
            `atKey`. Its root and tuning come from its file's sampler chunk or are found, and its
            loop from the chunk. An error (no room for it), or empty. Message thread. */
        juce::String addSample (const Sampler::SamplePtr&, const juce::File&, juce::uint32 replace = 0, int atKey = -1);

        /** Several sounds added at once, as files dropped together are (playing slices, the first
            alone is cut). When each has a clear pitch they become zones around their own notes,
            the keys between two shared half and half; otherwise each goes on a key of its own from
            `fromKey` up, as a one-shot. The first error (no room), or empty. Message thread. */
        juce::String addSamples (const std::vector<std::pair<Sampler::SamplePtr, juce::File>>&, int fromKey = Sampler::firstSliceKey);

        /** Plays a note now, as a click in its window does, and lets it go: added to the next block. Any thread. */
        void play (int note, float velocity);
        void stop (int note);

        /** Counts up each time the zones are set, so a window knows to read them again. */
        juce::uint32 getZonesVersion() const noexcept                { return version.load(); }

        /** What a preset carries besides its values: the zones ("zones", as the state keeps them,
            each sample named by its file) and how it is sliced. Applying texts with no zones
            (a preset saved by a plain dialog) leaves them as they are. */
        EffectPresets::Texts getPresetTexts() const;
        void setPresetTexts (const EffectPresets::Texts&);

        /** The zones it plays. Set from the message thread; the voices take them up at their next
            block. A zone with an id of 0 is given one. */
        void setZones (std::vector<Sampler::Zone>);
        std::vector<Sampler::Zone> getZones() const;

        /** Cuts a part of a sample (all of it, for an empty part) into slices as the slicing says and
            plays them in place of its zones; the slicing is kept. False, with nothing changed, when
            there is nothing to cut. */
        bool slice (const Sampler::SamplePtr&, const juce::File&, juce::Range<juce::int64> part, Sampler::Slicing);

        /** Whether it plays slices, and how they were cut. Setting it alone cuts nothing. */
        Sampler::Slicing getSlicing() const                          { return slicing; }
        void setSlicing (const Sampler::Slicing& s)                  { slicing = s; }

        /** The settings its parameters hold now. */
        Sampler::Settings currentSettings() const;

        /** A parameter by its ID (nullptr if there is none). */
        juce::RangedAudioParameter* parameter (const juce::String& id) const;

        int getNumSounding() const                                   { return engine.getNumSounding(); }
        const Sampler::Engine& getEngine() const noexcept            { return engine; }

        /** Samples named in its state that could not be read when it was last restored, with why. */
        juce::StringArray getMissing() const                         { return missing; }

        /** The files its zones name, each once, those that are missing among them. */
        juce::Array<juce::File> getFiles() const;

        /** How much memory its samples take, each counted once. */
        juce::int64 bytesHeld() const;

        /** Why a sample can't be added (it would take the sampler past what it may hold), or empty. */
        juce::String roomFor (const Sampler::SamplePtr&) const;

        /** Looks for its missing samples: `found` is one of them where it is now, or a folder they
            may be in. A file found is tried, by name, for the others missing from its folder too.
            How many zones got their sample back. */
        int locate (const juce::File& found);

        /** For the tests: what it may hold, in place of Sampler::mostBytes. */
        void setMostBytes (juce::int64 bytes)                         { most = bytes; }

    protected:
        void addToState (juce::XmlElement&) const override;
        void readFromState (const juce::XmlElement&) override;

    private:
        void retire();
        void writeZones (juce::XmlElement& into) const;
        void readZones (const juce::XmlElement& from);

        // Notes from the window, waiting for the audio thread: a velocity below 0 lets the note go.
        juce::SpinLock windowLock;
        std::vector<std::pair<int, float>> windowNotes;
        std::atomic<juce::uint32> version { 0 };

        std::map<juce::String, juce::RangedAudioParameter*> byId;
        Sampler::Engine engine;

        // The zones: swapped under the lock, which the audio thread only tries. Ones taken away are
        // kept until the voices have moved on from them.
        Sampler::ProgramPtr program, pending;
        std::vector<Sampler::ProgramPtr> retired;
        juce::SpinLock programLock;
        std::atomic<bool> programChanged { false };
        std::atomic<juce::uint32> oldestInUse { 0 };
        juce::uint32 generations = 0, zoneIds = 0;
        juce::StringArray missing;
        Sampler::Slicing slicing;
        juce::int64 most = Sampler::mostBytes;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerProcessor)
    };
}
