#pragma once

#include "SignalConsole.h"

/** Decrypt: a Torqwave Synth patch that plays a recorded note.

    A clip of one note (held or plucked, up to four seconds) is the signal. Its pitch is found, it is
    trimmed to the note and levelled, and it is measured: its spectrum over time on a mel scale at
    three window sizes, and its loudness through the note. A search then renders candidates through
    the synth itself at the clip's note and length and scores each against the clip by the same
    measures: first every factory patch, then variations of the best, round after round, each round
    a smaller step. The lock is how near a candidate is, 0 to 100 % (lockOf).

    Dry in this version: the candidates' effects are off, and a clip with reverb or delay on it is
    matched as if it were dry. One note: chords and riffs are not matched.

    Without an advisor, nothing here is random from one run to the next: the same clip, settings
    and seed give the same result, whatever the number of threads. */
namespace Decrypt
{
    using Synth = SignalConsole::Synth;
    using Patch = SignalConsole::Patch;

    constexpr double rate = 48000.0;
    constexpr double longest = 4.0;   // seconds of a clip that are matched
    constexpr double silenceAfter = 0.25;   // seconds of silence after a note that dies away by itself, which a match must keep
    constexpr int steadyFrames = 20;        // 10 ms frames a note holds a level for (within 3 dB) to count as held, not dying away (see prepare)

    /** A sound measured for matching: its mel spectrum over time at three window sizes (each band in
        dB under the loudest band of the whole sound, down to -70) and its loudness through it (each
        10 ms in dB under its loudest, down to -60). */
    struct Measures
    {
        static constexpr int sizes = 3, bands = 40;
        static constexpr int hop = 480;   // 10 ms at 48 kHz
        int frames = 0;
        std::array<std::vector<float>, sizes> mel;   // frames x bands, by frame
        std::vector<float> loudness;                 // dB, a frame each
        std::vector<float> brightness;               // 0 to 1, where each frame's weight sits
    };

    Measures measure (const float* mono, int length);

    /** How far apart two sounds are, in dB on average: the spectra (weighted to where either is
        loud, each moment's bands more than 40 dB under its loudest counted as at that floor, as
        unheard) and the loudness; 0 for the same sound. Also by part, for showing. */
    struct Distance
    {
        float total = 0.0f;
        float tone = 0.0f, brightness = 0.0f, loudness = 0.0f, movement = 0.0f;
    };

    Distance distance (const Measures& target, const Measures& candidate);

    /** A distance as a lock: 100 % for the same sound, falling to 0 at 12 dB. */
    float lockOf (float distanceDb);

    /** A clip made ready to be matched. */
    struct Target
    {
        juce::AudioBuffer<float> audio;     // one channel at 48 kHz, from just before the note starts, its peak at -6 dB
        double pitchHz = 0.0;               // 0: none found, matched at middle C
        int note = 60;                      // the key it is rendered on, the nearest to its pitch
        int held = 0;                       // samples the key is held in a render: the note's loud part, or all of it when it decays
        bool decays = false;                // it dies away by itself: matched with the key held, and silence after it
        Measures measures;
        SignalConsole::Signature signature;
        juce::String problem;               // why it can't be matched; empty when it can
    };

    /** A clip (any channels and rate) made a target. */
    Target prepare (const juce::AudioBuffer<float>& clip, double sampleRate);

    /** A patch played as a target is: its effects off, the target's key held for as long, as long. */
    juce::AudioBuffer<float> render (Synth&, const Patch&, const Target&);

    /** How loud the window auditions a candidate: its peak at -6 dB, as the target is levelled. */
    constexpr float auditionDb = -6.0f;

    /** A patch made ready for the keys. The search matches sounds at their peaks and never looks at a
        patch's own level, so a patch put on the synth as found plays at whatever level it happens to
        have. This one has its Amp Level trimmed so the target's note, played as the keys play it (its
        effects on, velocity 100), peaks where the audition does; ranOut when the knob's range could
        not bring it within a dB. */
    struct Levelled
    {
        Patch patch;
        float beforeDb = 0.0f, afterDb = 0.0f;    // the note's peak on the keys, before and after
        bool ranOut = false;
    };

    Levelled levelForKeys (const Patch&, const Target&);

    /** A render measured as the search measures it: its channels as one, at its peak. */
    Measures measureRender (const juce::AudioBuffer<float>&);

    /** A key's name, "A1". */
    juce::String noteName (int note);

    /** What a target is, in a line: "A1 (55.0 Hz), 1.82 s, attack 12 ms, ...". */
    juce::String describe (const Target&);

    /** Where a candidate differs from the target, by band (lows, low mids, high mids, highs) and by
        part of the note (attack, body, tail): the largest few, in dB. */
    juce::String describeDifference (const Measures& target, const Measures& candidate);

    /** A patch's settings that are not at their defaults, as the synth reads them, the effects left
        out; and the choices its tables, filters and shapes can take. */
    juce::String describeSettings (Synth& rangesFrom, const Patch&);

    //==========================================================================
    struct Candidate
    {
        Patch patch;
        Distance distance { 1000.0f };      // not scored yet (or silent): as far as can be
        float lock = 0.0f;
        juce::String from;                  // what it grew from: a factory patch's name, or "model: ..."
        juce::uint32 address = 0;
        SignalConsole::Signature signature; // of its render
    };

    /** Help from a language model, optional at every point. Each call blocks, and is made on a
        thread of the search's own, never the search's: the search never waits for an answer, and
        one that comes after its time limit is dropped. Whatever it proposes is rendered and scored
        by the same measure as every other candidate and kept only if it scores better. */
    class Advisor
    {
    public:
        virtual ~Advisor() = default;

        /** What the model is told: the target, the best candidate so far and how they differ, the
            words the user typed (if any), and for start() the clip itself (to send only to a model
            that hears, and only when the user has allowed it). */
        struct Brief
        {
            juce::String target, best, difference, words;
            int round = 0;
            float lock = 0.0f;
            const juce::AudioBuffer<float>* clip = nullptr;   // 48 kHz, one channel; start() only
        };

        /** Where to start: factory categories and patches to add to the starting set, and parts to
            firewall (keep as the start has them) in the variations. */
        struct Start
        {
            juce::StringArray categories, patches;
            std::array<bool, SignalConsole::numParts> firewall {};
            juce::String heard;   // what a model that hears said the clip is, when it was sent
        };

        /** A change of structure to try on the best candidate: settings by ID, as plain values (a
            choice's index, 1 or 0 for a switch). */
        struct Change
        {
            juce::String what;
            EffectPresets::Values values;
        };

        /** What to call a patch it found, offered to the user to keep or refuse. */
        struct Name
        {
            juce::String name, category, description;
            std::array<juce::String, WaveSynth::numMacros> macros;
        };

        virtual Start start (const Brief&)                          { return {}; }
        /** Up to five changes of structure (which table, which filter, a layer an octave up...). */
        virtual std::vector<Change> structure (const Brief&)        { return {}; }
        /** A name for the patch taken (brief.best its settings); an empty name for none. */
        virtual Name name (const Brief&)                            { return {}; }
        /** Stops a call in progress, from any thread: it returns soon after, with nothing. */
        virtual void cancel()                                       {}

        int timeLimitMs = 30000;
    };

    /** Where the synth's window gets an advisor: set by the app when Ask Torq is on (and cleared
        when it is turned off); nothing when it is off or there is no app. */
    using AdvisorMaker = std::function<std::shared_ptr<Advisor>()>;
    void setAdvisorMaker (AdvisorMaker);
    std::shared_ptr<Advisor> makeAdvisor();

    struct Options
    {
        juce::uint32 seed = 1;
        bool factoryTablesOnly = false;     // never give oscillator 1 the clip's own Sampled table
        juce::StringArray leaveOut;         // factory patches not to start from (tests)
        int rounds = 16;
        /** Rounds go on past `rounds` for as long as they come closer: the search ends when `patience`
            rounds in a row have raised the best lock by less than a tenth of a point (0.012 dB), or at
            `maxRounds`. Off (as tests and tools have it): exactly `rounds`. */
        bool untilSettled = false;
        int patience = 6, maxRounds = 64;
        int threads = 0;                    // 0: about half the cores
        juce::String words;                 // what the user said it is ("a reese"), for the advisor
        std::shared_ptr<Advisor> advisor;   // none: the search alone
        /** A search carried on: these patches (an earlier search's best) are where it starts, in place
            of the factory patches, and its rounds take smaller steps. Empty: a search from the start. */
        std::vector<Patch> resume;
    };

    /** Where a search is. */
    struct Progress
    {
        std::vector<Candidate> best;        // the best twelve, best first
        int rendered = 0, round = 0, rounds = 0;
        double seconds = 0.0;
        bool finished = false, stopped = false;
        juce::String stage;                 // "Factory patches", "Round 3 of 12", "Done"
        bool noCloser = false;              // it ended because no round came closer (untilSettled), not at a set last round
        /** How far each part has settled, 0 to 1: how closely the best twelve agree on its settings
            (1 when they all have the same). The effects are off while matching: always 0. */
        std::array<float, SignalConsole::numParts> settled {};
        /** What the advisor proposed and what came of it, oldest first. Empty without one. */
        juce::StringArray model;
    };

    /** A search, on threads of its own. Started on the message thread; onChange is called there as
        the best twelve change and when it ends. Stopping (or destroying it) ends it within a
        render or two and leaves every synth as it was: a search changes no synth but its own. */
    class Search
    {
    public:
        Search (Target, Options);
        ~Search();

        void start();
        void stop();
        bool isRunning() const;
        Progress getProgress() const;
        const Target& getTarget() const noexcept          { return target; }

        /** Runs to its end on this thread, or until stopWhen says to: for tests and tools. onChange
            is not called. */
        Progress runNow (std::function<bool()> stopWhen = nullptr);

        std::function<void()> onChange;

    private:
        class Runner;
        class Helper;
        void run (const std::function<bool()>& shouldStop);
        std::vector<Candidate> score (std::vector<Candidate> jobs, const Target&, const std::function<bool()>& shouldStop);
        void publish (std::vector<Candidate> merged, int round, const juce::String& stage, bool finished, bool stopped, Synth&, bool noCloser = false);
        Advisor::Brief briefFor (const std::vector<Candidate>& best, int round, Synth&) const;

        const Target target;
        const Options options;
        std::shared_ptr<const WaveTables::Sampled> clipTable;
        mutable juce::CriticalSection lock;
        Progress progress;
        juce::StringArray modelLog;         // the search thread's; copied into progress
        std::atomic<int> rendered { 0 };
        std::atomic<bool> stopping { false }, notify { false };
        double started = 0.0;
        std::unique_ptr<Runner> runner;
        std::shared_ptr<std::atomic<bool>> alive { std::make_shared<std::atomic<bool>> (true) };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Search)
    };
}
