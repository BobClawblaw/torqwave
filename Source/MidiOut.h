#pragma once

#include <JuceHeader.h>
#include <bitset>
#include "Session.h"

/** MIDI out: note clips, controller lanes and live input played on hardware synths through
    MIDI output ports, and MIDI clock and transport for other gear to follow. */
namespace MidiOut
{
    /** Sends timed MIDI to output ports from a thread of its own, so the audio thread never
        waits on a port. The audio thread queues messages with the time they are due (in
        juce::Time::getMillisecondCounterHiRes's milliseconds); the thread sends each when its time
        comes, in the order queued where times are equal. It remembers the notes each port has
        on, so notesOff() can end them. */
    class Sender final : private juce::Thread
    {
    public:
        static constexpr int maxPorts = 16;

        Sender();
        ~Sender() override;

        /** Message thread: opens these ports (by device identifier) and closes the others. Returns
            the names of those that could not be opened. */
        juce::StringArray usePorts (const juce::StringArray& identifiers);

        /** Message thread, for tests: uses an output that is already open, as this identifier. */
        void usePort (const juce::String& identifier, std::unique_ptr<juce::MidiOutput>);

        /** Any thread that doesn't race usePorts: the port's place, or -1 when it isn't open. */
        int indexOf (const juce::String& identifier) const noexcept;

        /** Audio thread: queues a message of 1 to 3 bytes for a port, due at `ms`. False when
            the queue is full (the message is dropped). */
        bool send (int port, const juce::MidiMessage&, double ms) noexcept;

        /** Audio thread: every note the ports have on is ended now, and what was queued before
            is dropped (playback stopped). */
        bool notesOff() noexcept;

        /** Audio thread: the notes a port has on at `ms` are ended then (after what is due before). */
        bool endNotes (int port, double ms) noexcept;

        /** Messages sent so far (for tests). */
        int getSent() const noexcept             { return sent; }

    private:
        struct Event
        {
            double ms = 0.0;
            juce::int64 order = 0;
            int port = 0, size = 0;
            juce::uint8 bytes[3] {};
        };

        void run() override;
        bool queueMarker (int kind, int port, double ms) noexcept;
        void sendNow (const Event&);

        std::array<std::unique_ptr<juce::MidiOutput>, maxPorts> outputs;
        std::array<juce::String, maxPorts> identifiers;
        juce::CriticalSection portsLock;              // the message thread changing ports, against the sending thread

        juce::AbstractFifo fifo { 8192 };
        std::vector<Event> queued = std::vector<Event> (8192);
        std::vector<Event> pending;                   // the sending thread's, by time
        std::array<std::array<std::bitset<128>, 16>, maxPorts> held;   // the sending thread's
        std::atomic<int> sent { 0 };
        juce::int64 order = 0;                        // the audio thread's
    };

    /** Plays the session to MIDI out, on the audio thread, through a Sender: the note clips and
        controller lanes of each track whose output is a MIDI port, and MIDI clock and transport
        on the clock port. Each run of frames the mix plays (loops and the arrangement chain
        included) is given to run(), with where it falls in the device's block. */
    class Player
    {
    public:
        explicit Player (Sender& s) : sender (s)    { block.ensureSize (16384); }

        /** Message thread: the port that gets MIDI clock (24 a beat) with start, stop, continue
            and song position, or empty for none. */
        void setClockPort (const juce::String& identifier);
        juce::String getClockPort() const;

        /** Audio thread, before each block: when the block's first frame is heard (ms), the rate,
            whether the session plays, and whether its frames play at the device's rate (when they
            don't, MIDI out sends nothing). Playing stopping ends every note and sends Stop. */
        void beginBlock (double ms, double sampleRate, bool playing, bool atDeviceRate) noexcept;

        /** Audio thread: the session's frames [start, start + count) play from frame `offset`
            of this block. */
        void run (const SessionState&, juce::int64 start, int offset, int count) noexcept;

        /** Audio thread: MIDI played live into a track (frames of this block), sent on if the
            track's output is a MIDI port. */
        void playLive (const SessionState&, juce::uint32 trackId, const juce::MidiBuffer&) noexcept;

        /** The time a frame of this block is due, with the track's offset (ms). */
        double timeOf (int frame, double offsetMs = 0.0) const noexcept { return blockMs + 1000.0 * frame / rate + offsetMs; }

        /** Song position (in 1/16 notes) for a beat position. */
        static int songPosition (double beat) noexcept { return juce::jmax (0, (int) std::floor (beat * 4.0 + 1.0e-9)); }

    private:
        void stopped() noexcept;
        void clock (const SessionState&, juce::int64 start, int offset, int count, bool first, bool jumped) noexcept;

        Sender& sender;
        mutable juce::SpinLock clockLock;
        juce::String clockPort;                       // under clockLock
        double blockMs = 0.0, rate = 48000.0;
        bool playing = false, wasPlaying = false, active = false;
        juce::MidiBuffer block;                       // a track's MIDI for a run (room made up front)
        juce::int64 nextFrame = -1;                   // where the last run ended; anything else is a jump
        juce::int64 nextTick = 0;                     // the next clock tick, in 24ths of a beat
    };
}
