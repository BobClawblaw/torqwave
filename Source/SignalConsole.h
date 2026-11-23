#pragma once

#include "WaveSynth.h"

/** The signal console's workings, without its look: Trace, a map of the sounds within reach of a
    Torqwave Synth patch.

    A patch is a node. Round the node held are twelve more, each the patch with some of its settings
    moved: mostly in one part of it (its oscillators, filter, envelopes, modulation or effects), by
    an amount Reach sets. A firewall keeps a part as it is in every node. A bridge to a second patch
    lays the twelve along the line between the two instead. Each node is levelled to the one held
    (rendered for a second and its amp level set so it peaks where the held node does), measured
    (the signature the look draws, and what Ask Torq reads) and given an address made from its
    settings. Every node is an ordinary patch.

    Nothing here is random from one call to the next: the same node, settings and seed give the
    same twelve. */
namespace SignalConsole
{
    using Synth = BuiltInEffects::BasicSynthProcessor;

    enum Part { oscillators, filter, envelopes, modulation, effects, numParts };
    const juce::StringArray& partNames();

    constexpr int numNodes = 12;

    /** A patch as the console holds it: every parameter's plain value under its ID (a choice's index,
        1 or 0 for a switch), the oscillators' Sampled tables, the macros' names and the patch's. */
    struct Patch
    {
        EffectPresets::Values values;
        std::array<std::shared_ptr<const WaveTables::Sampled>, WaveSynth::numOscillators> tables;
        std::array<juce::String, WaveSynth::numMacros> macroNames;
        juce::String name;
    };

    Patch capture (const Synth&);
    void apply (Synth&, const Patch&);

    /** A patch's address: the same settings and tables always give the same one. As text, four hex digits. */
    juce::uint32 addressOf (const Patch&);
    juce::String addressText (juce::uint32 address);

    /** What a second of a patch sounds like, measured: its spectrum in 48 bands from 40 Hz to 16 kHz
        (each 0 to 1 over the 48 dB below its strongest), its peak, where its weight sits (0 dark
        to 1 bright), how like noise it is (0 a tone to 1 noise), how wide (0 mono to 1), and its
        loudness through the second in eight steps (each 0 to 1 of its loudest). */
    struct Signature
    {
        static constexpr int bands = 48, steps = 8;
        std::array<float, bands> band {};
        std::array<float, steps> loudness {};
        float peak = 0.0f, brightness = 0.0f, noisiness = 0.0f, width = 0.0f;
    };

    /** A setting a node moved, for showing: its name, how far in its own units ("-40 ms") and how
        far as a share of its range. */
    struct Change
    {
        juce::String id, name, text;
        float amount = 0.0f;
    };

    struct Node
    {
        Patch patch;
        int part = oscillators;        // the part that changed most
        float distance = 0.0f;         // 0 (the node held) to 1 (as far as Reach goes)
        juce::uint32 address = 0;
        Signature signature;
        std::vector<Change> changes;   // the settings that moved most, furthest first (up to four)
    };

    struct Settings
    {
        float reach = 0.35f;                       // 0 (every node the one held) to 1
        juce::uint32 seed = 1;
        std::array<bool, numParts> firewall {};
        std::shared_ptr<const Patch> bridge;       // a second patch to lay the nodes toward
    };

    /** The part a parameter belongs to, or -1 for one the console never moves (the amp's level, which
        levelling sets, and how the keys are played: pan, velocity, voice mode, glide, bend). */
    int partOf (const juce::String& parameterId);

    /** Renders patches on a synth of its own and measures them. Not for the audio thread; one
        renderer is for one thread at a time. */
    class Renderer
    {
    public:
        Renderer();
        ~Renderer();

        /** A second of the patch: middle C held for 0.6 s at velocity 0.8, and its tail. */
        juce::AudioBuffer<float> render (const Patch&);
        Signature measure (const Patch&);
        static Signature measure (const juce::AudioBuffer<float>&, double sampleRate);

        /** The synth it renders on, holding the last patch rendered: for reading a parameter's range. */
        Synth& synth() noexcept                     { return *scratch; }

    private:
        std::unique_ptr<Synth> scratch;
    };

    /** The twelve nodes round a patch. */
    std::vector<Node> nodesAround (const Patch& held, const Settings&, Renderer&);

    /** Up to `count` patches moved from `held` as the twelve nodes are (by the reach map, firewalled
        parts kept), at reach from a third of `reach` to all of it, without rendering or levelling them:
        for a search that scores them itself (Decrypt). rangesFrom gives the parameters' ranges. The
        same arguments give the same patches; a patch nothing could move in is left out. */
    std::vector<Patch> variations (const Patch& held, Synth& rangesFrom, float reach, juce::uint32 seed,
                                   const std::array<bool, numParts>& firewall, int count);

    /** Like variations, but each moves only one to three settings of one part (each part in turn),
        chosen at random: small, sparse steps that reach a patch with many settings (sixteen
        modulation routes, step patterns) where moving all of a part at once rarely lands. */
    std::vector<Patch> fewMoves (const Patch& held, Synth& rangesFrom, float reach, juce::uint32 seed, const std::array<bool, numParts>& firewall, int count);

    /** Fitted steps: `settings` of the held patch's settings, from the `first` in an order that puts
        what shapes the sound most first (filter types, an off filter's too, cutoffs, levels, tables,
        pitches, the amp's envelope), each taken to `points` places across its whole range and a
        little either side of where it is (a choice to each of its others, a switch thrown, a pitch
        by fifths and octaves). `total` is how many settings there are to sweep, to go round them. */
    std::vector<Patch> sweeps (const Patch& held, Synth& rangesFrom, const std::array<bool, numParts>& firewall, int first, int settings, int points, int& total);

    /** A held node's own entry (distance 0), measured. */
    Node nodeOf (const Patch&, Renderer&);

    /** Whether the system asks for less motion (macOS's Reduce Motion, Windows' animations off). */
    bool reduceMotion();

    //==========================================================================
    /** A synth's console: the node it holds (the synth's patch as it is), the twelve round it, and
        the route here. Used from the message thread; the twelve are made on a thread of its own,
        since each is rendered twice, and onChange is called (on the message thread) when they
        arrive. Hearing a node plays it on a second synth of the console's, mixed into the
        synth's output, so the patch held is left alone. */
    class Console
    {
    public:
        explicit Console (Synth&);
        ~Console();

        const Settings& getSettings() const noexcept                { return settings; }
        void setReach (float);
        void setFirewall (int part, bool held);
        void setBridge (std::shared_ptr<const Patch>);
        /** Another twelve round the same node: the next seed. */
        void rescan();
        /** The twelve made again as they are set (the same seed): for when there are none yet. */
        void refresh()                                              { startScan(); }

        const Node& getHeld() const noexcept                        { return held; }
        /** The twelve; empty while the first are being made. */
        const std::vector<Node>& getNodes() const noexcept          { return nodes; }
        bool isScanning() const noexcept                            { return scanning; }

        /** Where it has been, oldest first, and which of them is held now. */
        const std::vector<Node>& getRoute() const noexcept          { return route; }
        int getRoutePosition() const noexcept                       { return position; }

        /** Plays a node for a moment (0 to 11, or -1 for the node held) at the last note played. */
        void hear (int node);
        /** Plays a sound once (one or two channels, at its own rate), mixed into the synth's output as
            a node heard is, the patch left alone: Decrypt's signal and its candidates. Another sound
            played cuts it. */
        void play (const juce::AudioBuffer<float>&, double sampleRate);

        /** Makes a node the synth's patch and the node held; the map is made again round it. */
        void jump (int node);
        /** Goes back (or forward) to a node on the route: its settings exactly. */
        void goTo (int routeIndex);
        /** Takes the synth's patch as the node held if it has changed since (a knob turned, a patch
            loaded): a new entry on the route. True if it had. */
        bool sync();

        /** Makes the twelve now, on this thread, and returns when they are in place: for tests and tools. */
        void scanNow();

        std::function<void()> onChange;

        // For the synth: its sample rate, and the audition added to its output (the audio thread).
        void prepare (double sampleRate, int blockSize);
        void addAudition (juce::AudioBuffer<float>&) noexcept;
        void notePlayed (int note) noexcept                         { lastNote.store (note, std::memory_order_relaxed); }

    private:
        class Scanner;
        void hold (const Patch&, bool addToRoute);
        void startScan();
        void install (std::vector<Node>, juce::uint32 scan);

        Synth& synth;
        Settings settings;
        Node held;
        std::vector<Node> nodes, route;
        int position = 0;
        bool scanning = false;
        juce::uint32 scanCount = 0;
        Renderer renderer;
        std::unique_ptr<Scanner> scanner;
        std::shared_ptr<std::atomic<bool>> alive { std::make_shared<std::atomic<bool>> (true) };

        // The audition.
        std::unique_ptr<Synth> player;
        juce::AudioBuffer<float> scratch;
        std::atomic<int> lastNote { 60 }, strike { -1 };
        int sounding = -1, remaining = 0, tail = 0;
        double rate = 48000.0;

        // Sounds played: three buffers, so the one being written is never the one playing or the one about to.
        std::array<juce::AudioBuffer<float>, 3> sounds;
        std::atomic<int> soundReady { -1 }, soundPlaying { -1 };
        int soundAt = 0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Console)
    };
}
