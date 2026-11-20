#pragma once

#include "BuiltInEffects.h"

/** Hardware inserts: an effect slot that plays its audio out of a pair of the interface's
    outputs, through outboard gear, and back in from a pair of its inputs.

    The Port is the device's side. Each device cycle it takes the interface's inputs before
    anything is processed (beginCycle) and puts what the inserts sent on the outputs after
    (endCycle). An insert running in that cycle, on the device's thread and at the device's rate,
    counts its frames on the device's clock: the frame it sends at goes out at that device frame,
    and what it plays is what came in `guard` frames earlier than the frame it is at. So a frame
    sent comes back exactly roundTrip + guard of the insert's own frames later, however its
    frames are split into blocks, where roundTrip is the device frames from an output to the
    input it comes back on: through the interface, its converters and the outboard. The insert
    reports that as its latency, and delay compensation lines it up with everything else.

    The guard lets an insert run a few frames ahead of the device in its cycle (the player's
    resampler reads 3 ahead). When an insert falls out of step with the device (it was not run
    for a while, or a jump primed it with many frames at once), it starts again at the cycle's
    first frame and plays silence for its latency, until what it sends from then on comes back.

    A ping measures the round trip: the Port sends one click (half scale) out of a pair and finds
    the loudest frame coming back on another within maxRoundTrip frames, while the inserts on
    those pairs are silent. It needs nothing to play, only the device running.

    Elsewhere (an offline render, a bounce, a device at another rate than the insert's, or no
    device) an insert can't reach the hardware: it passes its audio through, delayed by its
    latency so it stays in line, and says so (isLive). Nothing here allocates or locks on the
    audio thread. */
namespace HardwareInsert
{
    constexpr const char* identifier = "torqwave:hardware-insert";

    /** Interface pairs an insert can use: from 1/2 to 31/32, by their first channel (0, 2, ...). */
    constexpr int maxChannels = 32;
    constexpr int numPairs = maxChannels / 2;

    class Port
    {
    public:
        static constexpr int ringSize = 8192;             // frames a ring holds; a device cycle must be shorter than ringSize / 2
        static constexpr int guard = 64;
        static constexpr int maxInserts = 16;
        static constexpr int maxRoundTrip = ringSize - 2 * guard;
        static constexpr float pingLevel = 0.5f;
        static constexpr float pingThreshold = 0.001f;    // -60 dB: anything quieter is not the click

        Port();

        /** The engine's port. */
        static Port& shared();

        //==============================================================================
        // The device's side.

        /** Before the device's first cycle (any thread, nothing running): its rate, the active
            channels its cycles' buffers are, by absolute channel, and the round trip it reports
            (its input and output latency), for an estimate before a ping. */
        void deviceStarting (double rate, const juce::BigInteger& activeInputs, const juce::BigInteger& activeOutputs, int reportedRoundTrip);
        void deviceStopped();

        /** Audio thread, each cycle: the inputs, before anything is processed; then the outputs,
            to add the inserts' sends (and a ping's click) to, after. */
        void beginCycle (const float* const* inputs, int numInputs, int numFrames);
        void endCycle (float* const* outputs, int numOutputs, int numFrames);

        /** An audio device callback that runs `inner` (the engine's graph player) inside each
            cycle: beginCycle, inner, endCycle; and tells the Port when the device starts and stops. */
        std::unique_ptr<juce::AudioIODeviceCallback> wrap (juce::AudioIODeviceCallback& inner);

        double getDeviceRate() const noexcept               { return deviceRate.load(); }
        int getReportedRoundTrip() const noexcept           { return reported.load(); }

        //==============================================================================
        // A ping, from the message thread.

        enum class PingState { idle, waiting, done, failed };

        /** Starts a ping from this output pair to this input pair (first channels); false while
            one is going or with no device running, or the pairs not on in it. */
        bool startPing (int outputFirst, int inputFirst);
        PingState getPingState() const noexcept             { return pingState.load(); }

        /** The round trip the last ping found (device frames), or -1. */
        int getPingResult() const noexcept                  { return pingResult.load(); }

        /** Back to idle after a done or failed ping has been read. */
        void clearPing();

        //==============================================================================
        // The inserts' side (audio thread).

        /** A place for an insert's sends, taken for good by `owner` (null: none free). */
        int acquire (const void* owner);
        void release (int slot);

        /** True on the device's thread inside a cycle at this rate: an insert can play live. */
        bool isLive (double rate) const noexcept;

        /** The cycle running: its count and its first device frame. */
        juce::uint32 getCycle() const noexcept              { return cycle.load(); }
        juce::int64 getCycleStart() const noexcept          { return cycleStart.load(); }

        /** Adds a frame to send out of a pair at a device frame (the slot's ring). */
        void send (int slot, int outputFirst, juce::int64 frame, float left, float right) noexcept;

        /** Drops what a slot had waiting to go out (the insert starting again). */
        void clearSends (int slot) noexcept;

        /** What came in on a pair at a device frame of this cycle or an earlier one (still in the
            ring); silence for a frame not yet in, or a pair not on. */
        std::pair<float, float> received (int inputFirst, juce::int64 frame) const noexcept;

        /** True while a ping uses this output or input pair (inserts on it play silence). */
        bool isPinging (int outputFirst, int inputFirst) const noexcept;

    private:
        struct Slot
        {
            std::atomic<const void*> owner { nullptr };
            std::atomic<int> outputFirst { -1 };
            std::array<std::vector<float>, 2> ring;
        };

        std::array<Slot, maxInserts> slots;
        std::array<std::vector<float>, maxChannels> inputs;   // rings of each input channel by device frame

        // The device: active channels, by absolute channel, as indices into a cycle's buffers (-1: off).
        std::array<int, maxChannels> inputIndex {}, outputIndex {};
        std::atomic<double> deviceRate { 0.0 };
        std::atomic<int> reported { 0 };

        // The cycle (the device's thread writes, inserts read).
        std::atomic<juce::uint32> cycle { 0 };
        std::atomic<juce::int64> cycleStart { 0 };
        std::atomic<juce::int64> cycleEnd { 0 };          // where the inputs written so far end
        std::atomic<bool> inCycle { false };
        std::atomic<juce::Thread::ThreadID> cycleThread { nullptr };
        juce::int64 nextFrame = 0;                         // device thread: the next cycle's first frame

        // A ping: asked for (message thread), then run by the device's thread.
        std::atomic<PingState> pingState { PingState::idle };
        std::atomic<int> pingOutput { -1 }, pingInput { -1 }, pingResult { -1 };
        std::atomic<bool> pingAsked { false };
        juce::int64 pingAt = -1;                           // device thread: the frame the click went out at
        float pingPeak = 0.0f;
        juce::int64 pingPeakAt = -1;

        static constexpr juce::int64 mask = ringSize - 1;

        JUCE_DECLARE_NON_COPYABLE (Port)
    };

    //==============================================================================
    /** The insert: a built-in effect ("Hardware Insert") with an output pair to send to and an
        input pair to return from (saved in its state, set from its window), a send level and a
        return level (automatable parameters, in dB), and the round trip a ping measured (saved,
        with the rate it was measured at). Its latency is that round trip plus the Port's guard
        or, until a ping, the device's reported round trip plus the guard ("estimated"). */
    class Processor final : public BuiltInEffects::BuiltInEffect
    {
    public:
        explicit Processor (Port& = Port::shared());
        ~Processor() override;

        static juce::PluginDescription description();

        /** The pairs, by first channel (0, 2, ... 30). Message thread. */
        void setPairs (int outputFirst, int inputFirst);
        int getOutputFirst() const noexcept                 { return outputFirst.load(); }
        int getInputFirst() const noexcept                  { return inputFirst.load(); }

        /** Pings the round trip through its pairs (Port::startPing). False when it can't start. */
        bool ping();

        /** Message thread: takes a finished ping's result, its latency then following it. Returns
            the ping's state as it was. */
        Port::PingState collectPing();

        /** The round trip measured (device frames), or -1 for none at this rate. */
        int getMeasuredRoundTrip() const noexcept           { return measured; }

        /** True when it played through the hardware in its last block. */
        bool isLive() const noexcept                        { return live.load(); }

        /** Called on the message thread when its pairs change (the engine turns them on). */
        static inline std::function<void()> onPairsChanged;

        juce::AudioProcessorEditor* createEditor() override;
        void prepareToPlay (double sampleRate, int maximumBlockSize) override;
        void reset() override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

        Port& getPort() noexcept                            { return port; }

    private:
        void addToState (juce::XmlElement&) const override;
        void readFromState (const juce::XmlElement&) override;
        void updateLatency();

        Port& port;
        juce::AudioParameterFloat* sendDb = nullptr;
        juce::AudioParameterFloat* returnDb = nullptr;
        std::atomic<int> outputFirst { 0 }, inputFirst { 0 };
        int measured = -1;                     // round trip, device frames
        double measuredRate = 0.0;
        double rate = 48000.0;
        bool pinging = false;                  // message thread: this insert asked for the ping running

        // Audio thread.
        int slot = -1;
        juce::int64 position = 0;              // the device frame it sends at next
        juce::uint32 lastCycle = 0;
        juce::int64 silentUntil = 0;           // after starting again: silent until its sends come back
        bool running = false;                  // it was live in its last block
        std::atomic<bool> live { false };
        juce::AudioBuffer<float> delay;        // passing through elsewhere: delayed by the latency
        int delayWrite = 0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Processor)
    };
}
