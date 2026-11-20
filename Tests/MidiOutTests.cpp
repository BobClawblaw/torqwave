#include <JuceHeader.h>
#include "../Source/MidiOut.h"
#include "../Source/TrackEffects.h"
#include "../Source/SessionMixer.h"
#include "TestPlugins.h"
#include "TestUtils.h"
#include "MidiOutTests.h"

namespace
{
    using TestUtils::check;
    using namespace TestPlugins;

    constexpr double rate = 48000.0;

    /** What a MIDI input heard, with when each message came (ms). */
    struct Heard final : public juce::MidiInputCallback
    {
        void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& m) override
        {
            const juce::ScopedLock sl (lock);
            got.emplace_back (m, juce::Time::getMillisecondCounterHiRes());
        }

        /** Waits up to `seconds` for this many messages; then what came. */
        std::vector<std::pair<juce::MidiMessage, double>> take (size_t count, double seconds = 3.0)
        {
            const auto until = juce::Time::getMillisecondCounterHiRes() + seconds * 1000.0;

            for (;;)
            {
                {
                    const juce::ScopedLock sl (lock);

                    if (got.size() >= count || juce::Time::getMillisecondCounterHiRes() > until)
                    {
                        auto result = std::move (got);
                        got.clear();
                        return result;
                    }
                }

                juce::Thread::sleep (5);
            }
        }

        juce::CriticalSection lock;
        std::vector<std::pair<juce::MidiMessage, double>> got;
    };

    bool sameBytes (const juce::MidiMessage& a, const juce::MidiMessage& b)
    {
        return a.getRawDataSize() == b.getRawDataSize() && std::memcmp (a.getRawData(), b.getRawData(), (size_t) a.getRawDataSize()) == 0;
    }

    SessionState external (const juce::String& port)
    {
        SessionState s;
        s.sampleRate = rate;
        auto& t = s.addTrack();
        t.isInstrument = true;
        SessionEdits::setMidiOut (s, t.id, port, "Test Port", 0, 0.0);
        NoteClip c;
        c.length = 48000;
        c.channel = 3;
        c.notes = { { 60, 0, 4800, 1.0f }, { 64, 9600, 4800, 0.5f }, { 67, 19200, 9600, 0.8f } };
        SessionEdits::addNoteClip (s, t.id, c);
        return s;
    }

    void testPort()
    {
       #if JUCE_MAC || JUCE_LINUX
        const juce::String name ("Torqwave Test Port");
        auto output = juce::MidiOutput::createNewDevice (name);
        Heard heard;
        std::unique_ptr<juce::MidiInput> input;

        for (int attempt = 0; attempt < 40 && input == nullptr && output != nullptr; ++attempt)
        {
            for (auto& d : juce::MidiInput::getAvailableDevices())
                if (d.name == name)
                    input = juce::MidiInput::openDevice (d.identifier, &heard);

            if (input == nullptr)
                juce::MessageManager::getInstance()->runDispatchLoopUntil (50);   // CoreMIDI lists the new port from the run loop
        }

        check (output != nullptr && input != nullptr, "MIDI out: a virtual port is made, and an input opened on it");

        if (output == nullptr || input == nullptr)
            return;

        input->start();
        MidiOut::Sender sender;
        sender.usePort ("virtual", std::move (output));
        MidiOut::Player player (sender);
        const auto s = external ("virtual");

        // A second of the clip: the messages it plays, each when it should leave.
        juce::MidiBuffer want;
        s.tracks[0].noteClips[0].addMidi (0, 48000, want);
        std::vector<std::pair<juce::MidiMessage, double>> expected;

        for (const auto meta : want)
            expected.emplace_back (meta.getMessage(), 1000.0 * meta.samplePosition / rate);

        const auto start = juce::Time::getMillisecondCounterHiRes() + 100.0;
        player.beginBlock (start, rate, true, true);
        player.run (s, 0, 0, 24000);
        player.run (s, 24000, 24000, 24000);
        const auto got = heard.take (expected.size());
        auto same = got.size() == expected.size() && expected.size() == 6;
        double worst = 0.0;

        for (size_t i = 0; same && i < got.size(); ++i)
        {
            same = sameBytes (got[i].first, expected[i].first) && got[i].first.getChannel() == 3;
            worst = juce::jmax (worst, std::abs ((got[i].second - start) - expected[i].second));
        }

        check (same && worst < 5.0, "MIDI out: a virtual port receives exactly the clip's messages on its channel, each within "
                                    + juce::String (worst, 2) + " ms of its time (under 5)");

        // All on one channel, 50 ms later.
        auto moved = s;
        SessionEdits::setMidiOut (moved, moved.tracks[0].id, "virtual", "Test Port", 10, 50.0);
        player.beginBlock (0.0, rate, false, true);
        heard.take (100, 0.3);   // the stop's note offs
        const auto later = juce::Time::getMillisecondCounterHiRes() + 100.0;
        player.beginBlock (later, rate, true, true);
        player.run (moved, 0, 0, 4800);
        const auto shifted = heard.take (1);
        check (shifted.size() == 1 && shifted[0].first.isNoteOn() && shifted[0].first.getChannel() == 10 && shifted[0].first.getNoteNumber() == 60
                   && std::abs (shifted[0].second - (later + 50.0)) < 5.0,
               "MIDI out: a track's channel puts every message on it, and its offset sends them that much later ("
                   + juce::String (shifted.empty() ? 0.0 : shifted[0].second - later, 1) + " ms)");

        // Playback stopping with the note still on: its off goes out at once.
        player.beginBlock (juce::Time::getMillisecondCounterHiRes(), rate, false, true);
        const auto ended = heard.take (1);
        check (ended.size() == 1 && ended[0].first.isNoteOff() && ended[0].first.getNoteNumber() == 60 && ended[0].first.getChannel() == 10,
               "MIDI out: stopping ends a note left on");

        // A jump (the loop going round) with a note on ends it there, then plays on.
        auto held = s;
        held.tracks[0].noteClips[0].notes = { { 72, 0, 40000, 1.0f } };
        const auto loopAt = juce::Time::getMillisecondCounterHiRes() + 80.0;
        player.beginBlock (loopAt, rate, true, true);
        player.run (held, 0, 0, 2400);
        player.run (held, 0, 2400, 2400);
        const auto round = heard.take (3);
        check (round.size() == 3 && round[0].first.isNoteOn() && round[1].first.isNoteOff() && round[2].first.isNoteOn()
                   && round[1].first.getNoteNumber() == 72 && round[1].second >= round[0].second + 45.0,
               "MIDI out: a jump back ends the notes left on where it happens, before playing on");
        player.beginBlock (juce::Time::getMillisecondCounterHiRes(), rate, false, true);
        heard.take (1, 0.3);

        // Played live into the track, stopped: straight out.
        juce::MidiBuffer live;
        live.addEvent (juce::MidiMessage::noteOn (1, 50, (juce::uint8) 90), 0);
        player.beginBlock (juce::Time::getMillisecondCounterHiRes(), rate, false, true);
        player.playLive (s, s.tracks[0].id, live);
        const auto played = heard.take (1);
        check (played.size() == 1 && played[0].first.isNoteOn() && played[0].first.getNoteNumber() == 50,
               "MIDI out: a note played live into the track goes out of its port while stopped");
        juce::MidiBuffer off;
        off.addEvent (juce::MidiMessage::noteOff (1, 50), 0);
        player.playLive (s, s.tracks[0].id, off);
        heard.take (1, 0.5);

        // A resampled session (not at the device's rate): nothing goes out.
        const auto sentBefore = sender.getSent();
        player.beginBlock (juce::Time::getMillisecondCounterHiRes(), rate, true, false);
        player.run (s, 0, 0, 24000);
        juce::Thread::sleep (100);
        check (sender.getSent() == sentBefore, "MIDI out: nothing is sent while the session plays at another rate than the device");
        player.beginBlock (juce::Time::getMillisecondCounterHiRes(), rate, false, false);

        // MIDI clock: a beat at 120 from the top is Song Position 0, Start, then 24 clocks, one every 1/48 s.
        SessionState plain;
        plain.sampleRate = rate;
        player.setClockPort ("virtual");
        const auto clockAt = juce::Time::getMillisecondCounterHiRes() + 100.0;
        player.beginBlock (clockAt, rate, true, true);
        player.run (plain, 0, 0, 24000);
        const auto beat = heard.take (26);
        auto clocks = 0;
        double clockWorst = 0.0;

        for (size_t i = 2; i < beat.size(); ++i)
            if (beat[i].first.isMidiClock())
            {
                clockWorst = juce::jmax (clockWorst, std::abs ((beat[i].second - clockAt) - (double) clocks * 500.0 / 24.0));
                ++clocks;
            }

        check (beat.size() == 26 && beat[0].first.isSongPositionPointer() && beat[0].first.getSongPositionPointerMidiBeat() == 0
                   && beat[1].first.isMidiStart() && clocks == 24 && clockWorst < 5.0,
               "MIDI clock: playing from the top sends Song Position 0 and Start, then 24 clocks a beat, each within "
                   + juce::String (clockWorst, 2) + " ms");

        // A jump to beat 4 (bar 2): Stop, Song Position 16 (sixteenths), Continue; then stopping sends Stop.
        player.run (plain, 96000, 0, 1000);
        const auto jumped = heard.take (4);
        player.beginBlock (juce::Time::getMillisecondCounterHiRes(), rate, false, true);
        const auto stopped = heard.take (1);
        check (jumped.size() == 4 && jumped[0].first.isMidiStop() && jumped[1].first.isSongPositionPointer()
                   && jumped[1].first.getSongPositionPointerMidiBeat() == 16 && jumped[2].first.isMidiContinue() && jumped[3].first.isMidiClock()
                   && stopped.size() == 1 && stopped[0].first.isMidiStop(),
               "MIDI clock: a jump sends Stop, its Song Position and Continue; stopping sends Stop");

        input->stop();
       #endif
    }

    /** Two instrument tracks: the first plays a chord plug-in, the second a click instrument
        taking the first's MIDI. Returns the notes the click instrument got. */
    std::vector<std::pair<int, juce::int64>> routed (bool sourceFirst, bool told)
    {
        SessionState s;
        s.sampleRate = rate;
        auto& a = s.addTrack();
        a.isInstrument = true;
        const auto source = a.id;
        auto& b = s.addTrack();
        b.isInstrument = true;
        const auto target = b.id;
        NoteClip c;
        c.length = 512;
        c.notes = { { 60, 100, 50, 1.0f }, { 62, 300, 50, 1.0f } };
        SessionEdits::addNoteClip (s, source, c);
        SessionEdits::setMidiFrom (s, target, source);

        if (! sourceFirst)
            std::swap (s.tracks[0], s.tracks[1]);

        TrackEffects effects;
        effects.prepare (rate);
        effects.add (source, std::make_unique<ChordInstrument>());
        auto click = std::make_unique<ClickInstrument> (0);
        auto* got = click.get();
        effects.add (target, std::move (click));

        if (told)
            effects.setMidiSources ({ source });

        juce::AudioBuffer<float> out (2, 512);
        SessionMixer::Buffers buffers;
        buffers.reserve (128);

        for (int p = 0; p < 512; p += 128)
            SessionMixer::render (s, p, out, p, 128, buffers, &effects);

        return got->notes;
    }

    void testPluginMidi()
    {
        using Notes = std::vector<std::pair<int, juce::int64>>;
        const auto inTime = routed (true, true);
        const auto late = routed (false, true);
        const auto none = routed (true, false);
        check (inTime == Notes { { 60, 100 }, { 67, 100 }, { 62, 300 }, { 69, 300 } },
               "plug-in MIDI: a chord plug-in's notes (each and its fifth) reach the track taking its MIDI, at their frames");
        check (late == Notes { { 60, 128 }, { 67, 128 }, { 62, 384 }, { 69, 384 } } && none.empty(),
               "plug-in MIDI: with the taking track above its source, they come at the start of its next run; untold, nothing comes");

        // In the session file, and dropped with the track it came from.
        SessionState s;
        s.sampleRate = rate;
        s.addTrack().isInstrument = true;
        s.addTrack().isInstrument = true;
        const auto first = s.tracks[0].id, second = s.tracks[1].id;
        const auto set = SessionEdits::setMidiFrom (s, second, first);
        const auto self = SessionEdits::setMidiFrom (s, first, first);
        SessionEdits::setMidiOut (s, first, "port-id", "Synth Port", 5, -12.5);
        juce::AudioFormatManager formats;
        const auto file = juce::File::createTempFile ("tqsession");
        const auto error = SessionFile::save (s, file);
        const auto loaded = SessionFile::load (file, formats);
        file.deleteFile();
        const auto& got = loaded.state;
        auto removed = s;
        SessionEdits::removeTrack (removed, 0);
        check (set && ! self && error.isEmpty() && loaded.error.isEmpty() && got.tracks.size() == 2 && got.tracks[1].midiFromTrackId == got.tracks[0].id
                   && got.tracks[0].midiOutPort == "port-id" && got.tracks[0].midiOutName == "Synth Port" && got.tracks[0].midiOutChannel == 5
                   && juce::exactlyEqual (got.tracks[0].midiOutOffsetMs, -12.5) && got.tracks[0].playsMidiOut() && ! got.tracks[1].playsMidiOut()
                   && removed.tracks.size() == 1 && removed.tracks[0].midiFromTrackId == 0,
               "MIDI out: a track's port, channel and offset, and the track it takes MIDI from, come back from a session file; "
               "removing the source drops the link, and a track can't take its own");
    }
}

void runMidiOutTests()
{
    std::cout << "MIDI out:" << std::endl;
    testPluginMidi();
    testPort();
}
