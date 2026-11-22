#pragma once

#include "BuiltInEffects.h"

/** The drum machine behind Basic Drums: synthesised sounds on the General MIDI drum map (35 to 81),
    each with its own tune, decay, tone, level, pan and drive, a kit-wide drive, glue compressor and
    level, and the kits made from them. The engine knows nothing of plug-ins, so the window can
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

        bool operator== (const Settings& o) const
        {
            return juce::exactlyEqual (tune, o.tune) && juce::exactlyEqual (decay, o.decay) && juce::exactlyEqual (tone, o.tone)
                && juce::exactlyEqual (level, o.level) && juce::exactlyEqual (pan, o.pan) && juce::exactlyEqual (drive, o.drive)
                && filter == o.filter && juce::exactlyEqual (cutoff, o.cutoff) && juce::exactlyEqual (resonance, o.resonance)
                && juce::exactlyEqual (crush, o.crush) && juce::exactlyEqual (reverb, o.reverb) && juce::exactlyEqual (delay, o.delay);
        }
    };

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
        std::array<Settings, (size_t) numSounds> sounds {};
        KickCharacter kick = KickCharacter::punch;
        float drive = 0.0f;     // %, 0 to 100: the whole kit through a soft clipper
        float glue = 0.0f;      // %, 0 to 100: a bus compressor, threshold and ratio together
        float level = -10.0f;   // dB, -48 to 6 (Standard's leaves a full kit struck at once under 0 dB)
        float reverbSize = 50.0f;   // %, 0 (a small room) to 100 (a hall)
        int delayTime = 2;          // an index into delayTimeNames(), 1/8 to start with
        float feedback = 35.0f;     // %, 0 to 100: how much of each echo comes round again

        bool operator== (const KitSettings& o) const
        {
            return sounds == o.sounds && kick == o.kick && juce::exactlyEqual (drive, o.drive) && juce::exactlyEqual (glue, o.glue) && juce::exactlyEqual (level, o.level)
                && juce::exactlyEqual (reverbSize, o.reverbSize) && delayTime == o.delayTime && juce::exactlyEqual (feedback, o.feedback);
        }
    };

    /** A sound's settings as the engine starts (the Standard kit's). */
    Settings defaultsFor (Sound);
    KitSettings defaultKit();

    /** The factory kits, Standard (the defaults) first. */
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

        void noteOn (int note, float velocity);

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

        /** How far the glue is turning the kit down, in dB (0 or less). Any thread. */
        float getReductionDb() const noexcept                       { return reductionDb.load (std::memory_order_relaxed); }

        /** One hit of a sound (its pad's note unless note is given) at full velocity alone,
            through nothing else: what the window draws. */
        static juce::AudioBuffer<float> renderOne (const KitSettings&, Sound, double sampleRate, double seconds, int note = -1);

        struct Hit;

    private:
        void start (Hit&, int note, float velocity);
        void cut (int note);
        void renderBlock (float* left, float* right, int count);
        void renderEffects (float* left, float* right, int count);
        void clearEffects();
        void setReverbSize();
        float delayTarget() const;

        double rate = 48000.0;
        KitSettings settings;
        std::vector<Hit> hits, fading;   // one each per note
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

        std::array<std::atomic<juce::uint32>, (size_t) numSounds> hitCount {};
        std::array<std::atomic<float>, (size_t) numSounds> lastVelocity {};
        std::atomic<float> reductionDb { 0.0f };
    };
}

namespace BuiltInEffects
{
    juce::PluginDescription basicDrumsDescription();

    /** Basic Drums, the drum machine built in (DrumMachine), as an instrument: each sound's tune,
        decay, tone, level, pan and drive, the kick's character and the kit's drive, glue and level
        are hosted parameters, so automation and sessions find them. Stereo out. A kit sets them all
        at once; the state keeps the kit's name. Sessions saved before 0.19.3's step 10 (a kick,
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

        const DrumMachine::Engine& getEngine() const noexcept      { return engine; }

        /** A parameter of a sound, by its suffix: "tune", "decay", "tone", "level", "pan", "drive",
            "cutoff", "resonance", "crush", "reverb" or "delay"; and its filter's type. */
        juce::AudioParameterFloat* parameterOf (Sound, const juce::String& what) const;
        juce::AudioParameterChoice* filterParameter (Sound s) const noexcept { return sounds[(size_t) s].filter; }
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
            juce::AudioParameterChoice* filter = nullptr;
            juce::AudioParameterFloat *cutoff = nullptr, *resonance = nullptr, *crush = nullptr, *reverb = nullptr, *delay = nullptr;
        };

        std::array<SoundParameters, (size_t) DrumMachine::numSounds> sounds;
        juce::AudioParameterChoice* kickCharacter = nullptr;
        juce::AudioParameterFloat *kitDrive = nullptr, *glue = nullptr, *level = nullptr, *reverbSize = nullptr, *feedback = nullptr;
        juce::AudioParameterChoice* delayTime = nullptr;
        DrumMachine::Engine engine;
        float volume = 1.0f;   // CC 7, squared as MIDI's volume usually is
        juce::String kitName { "Standard" };

        // Pads clicked in the window, waiting for the audio thread.
        juce::SpinLock auditionLock;
        std::vector<std::pair<int, float>> auditions;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BasicDrumsProcessor)
    };
}
