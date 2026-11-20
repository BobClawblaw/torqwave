#include "MidiOut.h"

namespace MidiOut
{
    namespace
    {
        // Queued events that aren't messages: every note off now and what is queued dropped
        // (playback stopped), or the notes on at that time ended (a jump in playback).
        constexpr int stopAll = -1, endHeld = -2;
    }

    Sender::Sender() : juce::Thread ("Torqwave MIDI out")
    {
        pending.reserve (8192);

        for (auto& port : held)
            for (auto& channel : port)
                channel.reset();

        startThread (juce::Thread::Priority::highest);
    }

    Sender::~Sender()
    {
        stopThread (2000);
    }

    juce::StringArray Sender::usePorts (const juce::StringArray& wanted)
    {
        juce::StringArray failed;
        const juce::ScopedLock sl (portsLock);

        for (int i = 0; i < maxPorts; ++i)
            if (identifiers[(size_t) i].isNotEmpty() && ! wanted.contains (identifiers[(size_t) i]))
            {
                outputs[(size_t) i].reset();
                identifiers[(size_t) i].clear();
            }

        for (auto& id : wanted)
        {
            if (id.isEmpty() || indexOf (id) >= 0)
                continue;

            const auto free = std::find_if (identifiers.begin(), identifiers.end(), [] (auto& s) { return s.isEmpty(); });
            auto output = free != identifiers.end() ? juce::MidiOutput::openDevice (id) : nullptr;

            if (output == nullptr)
            {
                failed.add (id);
                continue;
            }

            const auto i = (size_t) (free - identifiers.begin());
            outputs[i] = std::move (output);
            identifiers[i] = id;

            for (auto& channel : held[i])
                channel.reset();
        }

        return failed;
    }

    void Sender::usePort (const juce::String& identifier, std::unique_ptr<juce::MidiOutput> output)
    {
        const juce::ScopedLock sl (portsLock);
        const auto free = std::find_if (identifiers.begin(), identifiers.end(), [] (auto& s) { return s.isEmpty(); });

        if (free == identifiers.end() || output == nullptr)
            return;

        const auto i = (size_t) (free - identifiers.begin());
        outputs[i] = std::move (output);
        identifiers[i] = identifier;
    }

    int Sender::indexOf (const juce::String& identifier) const noexcept
    {
        if (identifier.isEmpty())
            return -1;

        for (int i = 0; i < maxPorts; ++i)
            if (identifiers[(size_t) i] == identifier)
                return i;

        return -1;
    }

    bool Sender::send (int port, const juce::MidiMessage& m, double ms) noexcept
    {
        const auto size = m.getRawDataSize();

        if (! juce::isPositiveAndBelow (port, maxPorts) || size < 1 || size > 3)
            return false;

        const auto write = fifo.write (1);

        if (write.blockSize1 + write.blockSize2 == 0)
            return false;

        auto& e = queued[(size_t) (write.blockSize1 > 0 ? write.startIndex1 : write.startIndex2)];
        e.ms = ms;
        e.order = order++;
        e.port = port;
        e.size = size;
        std::memcpy (e.bytes, m.getRawData(), (size_t) size);
        return true;
    }

    bool Sender::queueMarker (int kind, int port, double ms) noexcept
    {
        const auto write = fifo.write (1);

        if (write.blockSize1 + write.blockSize2 == 0)
            return false;

        auto& e = queued[(size_t) (write.blockSize1 > 0 ? write.startIndex1 : write.startIndex2)];
        e.ms = ms;
        e.order = order++;
        e.port = port;
        e.size = kind;
        return true;
    }

    bool Sender::endNotes (int port, double ms) noexcept
    {
        return juce::isPositiveAndBelow (port, maxPorts) && queueMarker (endHeld, port, ms);
    }

    bool Sender::notesOff() noexcept
    {
        return queueMarker (stopAll, 0, 0.0);
    }

    void Sender::run()
    {
        while (! threadShouldExit())
        {
            // What the audio thread queued, in order: messages wait in `pending` until due.
            const auto read = fifo.read (fifo.getNumReady());

            const auto take = [this] (int start, int count)
            {
                for (int i = start; i < start + count; ++i)
                {
                    auto e = queued[(size_t) i];

                    if (e.size == stopAll)
                    {
                        pending.clear();
                        e.ms = 0.0;
                        sendNow (e);
                    }
                    else
                    {
                        pending.push_back (e);
                    }
                }
            };

            take (read.startIndex1, read.blockSize1);
            take (read.startIndex2, read.blockSize2);

            std::stable_sort (pending.begin(), pending.end(), [] (const Event& a, const Event& b)
            {
                return a.ms < b.ms || (! (b.ms < a.ms) && a.order < b.order);
            });

            const auto now = juce::Time::getMillisecondCounterHiRes();
            auto due = pending.begin();

            while (due != pending.end() && due->ms <= now)
                sendNow (*due++);

            pending.erase (pending.begin(), due);

            // Every millisecond while a port is open (messages are queued ahead by the output
            // latency); otherwise rarely.
            bool anyOpen = false;

            {
                const juce::ScopedLock sl (portsLock);
                anyOpen = std::any_of (outputs.begin(), outputs.end(), [] (auto& o) { return o != nullptr; });
            }

            wait (anyOpen || ! pending.empty() ? 1 : 50);
        }
    }

    void Sender::sendNow (const Event& e)
    {
        const juce::ScopedLock sl (portsLock);

        // Every note a port has on (all of them for stopAll) gets its own off.
        if (e.size == stopAll || e.size == endHeld)
        {
            for (int p = 0; p < maxPorts; ++p)
            {
                if (e.size == endHeld && p != e.port)
                    continue;

                for (int channel = 0; channel < 16; ++channel)
                    for (int pitch = 0; pitch < 128; ++pitch)
                        if (held[(size_t) p][(size_t) channel][(size_t) pitch])
                        {
                            if (auto* out = outputs[(size_t) p].get())
                                out->sendMessageNow (juce::MidiMessage::noteOff (channel + 1, pitch));

                            ++sent;
                        }

                for (auto& channel : held[(size_t) p])
                    channel.reset();
            }

            return;
        }

        const juce::MidiMessage m (e.bytes, e.size);
        auto& notes = held[(size_t) e.port][(size_t) juce::jlimit (0, 15, m.getChannel() - 1)];

        if (m.isNoteOn())
            notes.set ((size_t) m.getNoteNumber());
        else if (m.isNoteOff())
            notes.reset ((size_t) m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff())
            notes.reset();

        if (auto* out = outputs[(size_t) e.port].get())
            out->sendMessageNow (m);

        ++sent;
    }

    //==============================================================================
    void Player::setClockPort (const juce::String& identifier)
    {
        const juce::SpinLock::ScopedLockType sl (clockLock);
        clockPort = identifier;
    }

    juce::String Player::getClockPort() const
    {
        const juce::SpinLock::ScopedLockType sl (clockLock);
        return clockPort;
    }

    void Player::beginBlock (double ms, double sampleRate, bool isPlaying, bool atDeviceRate) noexcept
    {
        blockMs = ms;
        rate = juce::jmax (1.0, sampleRate);
        active = atDeviceRate;
        playing = isPlaying;

        if (wasPlaying && ! playing)
            stopped();

        if (! wasPlaying && playing)
            nextFrame = -1;

        wasPlaying = playing;
    }

    void Player::stopped() noexcept
    {
        sender.notesOff();

        const juce::SpinLock::ScopedTryLockType sl (clockLock);

        if (sl.isLocked())
            if (const auto port = sender.indexOf (clockPort); port >= 0)
                sender.send (port, juce::MidiMessage::midiStop(), timeOf (0));
    }

    void Player::run (const SessionState& state, juce::int64 start, int offset, int count) noexcept
    {
        if (! active || ! playing || count <= 0)
            return;

        const auto first = nextFrame < 0;
        const auto jumped = ! first && start != nextFrame;
        nextFrame = start + count;

        const auto soloing = state.anySolo();

        for (auto& t : state.tracks)
        {
            if (! t.playsMidiOut() || state.isMuted (t) || (soloing && ! state.isSoloed (t) && ! state.isSoloSafe (t)))
                continue;

            const auto port = sender.indexOf (t.midiOutPort);

            if (port < 0)
                continue;

            // A jump (a loop going round): the notes left on end where it happens.
            if (jumped)
                sender.endNotes (port, timeOf (offset, t.midiOutOffsetMs));

            block.clear();

            for (auto& clip : t.noteClips)
                if (! clip.muted)
                    clip.addMidi (start, count, block);

            for (const auto meta : block)
            {
                auto m = meta.getMessage();

                if (t.midiOutChannel > 0)
                    m.setChannel (t.midiOutChannel);

                sender.send (port, m, timeOf (offset + meta.samplePosition, t.midiOutOffsetMs));
            }
        }

        clock (state, start, offset, count, first, jumped);
    }

    void Player::clock (const SessionState& state, juce::int64 start, int offset, int count, bool first, bool jumped) noexcept
    {
        const juce::SpinLock::ScopedTryLockType sl (clockLock);

        if (! sl.isLocked())
            return;

        const auto port = sender.indexOf (clockPort);

        if (port < 0)
            return;

        const auto map = state.tempoMap();
        const auto from = map.beatPosition (start);

        // Where play starts or jumps to: its song position, then Start (from the top) or Continue.
        if (first || jumped)
        {
            const auto at = timeOf (offset);

            if (jumped)
                sender.send (port, juce::MidiMessage::midiStop(), at);

            const auto position = songPosition (from);
            sender.send (port, juce::MidiMessage::songPositionPointer (position), at);
            sender.send (port, position == 0 && ! jumped ? juce::MidiMessage::midiStart() : juce::MidiMessage::midiContinue(), at);
            nextTick = (juce::int64) std::ceil (from * 24.0 - 1.0e-9);
        }

        // A clock at each 24th of a beat in the run.
        const auto to = map.beatPosition (start + count);

        for (; (double) nextTick / 24.0 < to; ++nextTick)
        {
            const auto frame = map.frameAtBeatPosition ((double) nextTick / 24.0);
            sender.send (port, juce::MidiMessage::midiClock(), timeOf (offset + (int) juce::jlimit ((juce::int64) 0, (juce::int64) count - 1, frame - start)));
        }
    }

    void Player::playLive (const SessionState& state, juce::uint32 trackId, const juce::MidiBuffer& live) noexcept
    {
        if (live.isEmpty() || trackId == 0)
            return;

        const auto* t = state.findTrack (trackId);

        if (t == nullptr || ! t->playsMidiOut())
            return;

        const auto port = sender.indexOf (t->midiOutPort);

        if (port < 0)
            return;

        for (const auto meta : live)
        {
            auto m = meta.getMessage();

            if (t->midiOutChannel > 0)
                m.setChannel (t->midiOutChannel);

            sender.send (port, m, timeOf (meta.samplePosition, t->midiOutOffsetMs));
        }
    }
}
