// Exact values are what the session holds.
#pragma clang diagnostic ignored "-Wfloat-equal"

#include <JuceHeader.h>
#include "../Source/AudioEngine.h"
#include "../Source/ControlSurface.h"
#include "../Source/MultitrackView.h"
#include "../Source/Session.h"
#include "TestUtils.h"
#include "ControlSurfaceTests.h"

namespace
{
    using TestUtils::check;
    using TestUtils::runUntil;

    constexpr double rate = 48000.0;

    bool near (float a, float b, float within = 0.01f)
    {
        return std::abs (a - b) <= within;
    }

    std::vector<juce::uint8> bytes (const juce::MidiMessage& m)
    {
        return { m.getRawData(), m.getRawData() + m.getRawDataSize() };
    }

    bool sameBytes (const juce::MidiMessage& a, const juce::MidiMessage& b)
    {
        return bytes (a) == bytes (b);
    }

    /** What a MIDI input heard. */
    struct Heard final : public juce::MidiInputCallback
    {
        void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& m) override
        {
            const juce::ScopedLock sl (lock);
            got.push_back (m);
        }

        /** Pumps the message loop until a message like this one came (and takes what came), or a second passes. */
        bool waitFor (const juce::MidiMessage& wanted, int ms = 1000)
        {
            return runUntil ([&]
            {
                const juce::ScopedLock sl (lock);
                return std::any_of (got.begin(), got.end(), [&] (auto& m) { return sameBytes (m, wanted); });
            }, ms);
        }

        /** Pumps the loop a while; true when nothing like this came. */
        bool nothingLike (std::function<bool (const juce::MidiMessage&)> like, int ms = 200)
        {
            runUntil ([] { return false; }, ms);
            const juce::ScopedLock sl (lock);
            return std::none_of (got.begin(), got.end(), like);
        }

        void clear()
        {
            const juce::ScopedLock sl (lock);
            got.clear();
        }

        juce::CriticalSection lock;
        std::vector<juce::MidiMessage> got;
    };

    /** An input opened on a virtual port made here, once CoreMIDI lists it. */
    std::unique_ptr<juce::MidiInput> openOn (const juce::String& name, juce::MidiInputCallback* callback)
    {
        for (int attempt = 0; attempt < 40; ++attempt)
        {
            for (auto& d : juce::MidiInput::getAvailableDevices())
                if (d.name == name)
                    return juce::MidiInput::openDevice (d.identifier, callback);

            juce::MessageManager::getInstance()->runDispatchLoopUntil (50);
        }

        return nullptr;
    }

    SessionState tenTracks()
    {
        SessionState s;
        s.sampleRate = rate;

        for (int i = 1; i <= 10; ++i)
        {
            auto& t = s.addTrack();
            t.name = "T" + juce::String (i);
        }

        s.tracks[0].gainDb = -6.0f;
        s.addBus().name = "Drum Bus";
        return s;
    }

    const SessionTrack& track (AudioEngine& engine, int index)
    {
        return engine.session.getState()->tracks[(size_t) index];
    }

    //==============================================================================
    void testProtocol()
    {
        auto roundTrip = true;

        for (float db = -59.0f; db <= 12.0f; db += 0.5f)
            roundTrip = roundTrip && near (Mackie::dbOfPosition (Mackie::positionOfDb (db)), db, 0.001f);

        check (roundTrip && near (Mackie::positionOfDb (-12.0f), 0.5f, 1.0e-6f) && Mackie::positionOfDb (12.0f) == 1.0f
                   && Mackie::positionOfDb (-60.0f) == 0.0f && Mackie::dbOfPosition (0.0f) == -60.0f && near (Mackie::dbOfPosition (1.0f), 12.0f, 1.0e-4f),
               "Mackie: a fader's position follows the mixer's taper (-60 to +12 dB, -12 at the middle), there and back");

        const auto fader = Mackie::fader (2, 0.5f);
        const auto moved = Mackie::faderMoved (Mackie::fader (5, 0.25f));
        check (bytes (fader) == std::vector<juce::uint8> { 0xe2, 0x00, 0x40 } && bytes (Mackie::fader (Mackie::master, 1.0f)) == std::vector<juce::uint8> { 0xe8, 0x7f, 0x7f }
                   && moved.has_value() && moved->first == 5 && near (moved->second, 0.25f, 0.0001f)
                   && ! Mackie::faderMoved (juce::MidiMessage::pitchWheel (10, 0)).has_value(),
               "Mackie: faders are 14-bit pitch bends on channels 1 to 8, the master on 9");

        const auto pressed = Mackie::button (juce::MidiMessage::noteOn (1, Mackie::mute + 3, (juce::uint8) 127));
        const auto letGo = Mackie::button (juce::MidiMessage::noteOn (1, Mackie::mute + 3, (juce::uint8) 0));
        check (bytes (Mackie::led (Mackie::mute + 3, true)) == std::vector<juce::uint8> { 0x90, 0x13, 0x7f }
                   && bytes (Mackie::led (Mackie::play, false)) == std::vector<juce::uint8> { 0x90, 0x5e, 0x00 }
                   && pressed == std::pair<int, bool> { 0x13, true } && letGo == std::pair<int, bool> { 0x13, false }
                   && ! Mackie::button (juce::MidiMessage::noteOn (2, 0x13, (juce::uint8) 127)).has_value(),
               "Mackie: buttons and their LEDs are notes on channel 1, 127 pressed or lit and 0 let go or dark");

        check (Mackie::vpotTurned (juce::MidiMessage::controllerEvent (1, 0x12, 0x43)) == std::pair<int, int> { 2, -3 }
                   && Mackie::vpotTurned (juce::MidiMessage::controllerEvent (1, 0x17, 0x05)) == std::pair<int, int> { 7, 5 }
                   && ! Mackie::vpotTurned (juce::MidiMessage::controllerEvent (1, 0x18, 0x01)).has_value()
                   && bytes (Mackie::vpotRing (1, 0.0f)) == std::vector<juce::uint8> { 0xb0, 0x31, 0x46 }
                   && bytes (Mackie::vpotRing (1, -1.0f)) == std::vector<juce::uint8> { 0xb0, 0x31, 0x01 }
                   && bytes (Mackie::vpotRing (7, 1.0f)) == std::vector<juce::uint8> { 0xb0, 0x37, 0x0b },
               "Mackie: V-pots turn as relative CCs 16 to 23 (64 set: counter-clockwise), and their rings show a dot, the centre lit at centre");

        check (bytes (Mackie::scribble (56, juce::String::fromUTF8 ("Ab\xc3\xa9"))) == std::vector<juce::uint8> { 0xf0, 0x00, 0x00, 0x66, 0x14, 0x12, 0x38, 'A', 'b', '?', 0xf7 }
                   && Mackie::stripText ("Drums long") == "Drums l" && Mackie::stripText ("Kick") == "Kick   ",
               "Mackie: the scribble strips take ASCII from a place by SysEx, seven characters a strip");
    }

    //==============================================================================
    void testSurface (AudioEngine& engine)
    {
       #if JUCE_MAC || JUCE_LINUX
        engine.session.reset (tenTracks(), {});
        MultitrackView view (engine);
        const auto ids = [&] { std::vector<juce::uint32> v; for (auto& t : engine.session.getState()->tracks) v.push_back (t.id); return v; }();

        // The device: what it sends comes into the surface's input; what the surface sends comes to `heard`.
        auto device = juce::MidiOutput::createNewDevice ("Torqwave Test Mackie");
        auto shows = juce::MidiOutput::createNewDevice ("Torqwave Test Mackie Shows");
        Heard heard;
        ControlSurface surface (engine, [&] { return &view; });
        auto surfaceIn = device != nullptr ? openOn ("Torqwave Test Mackie", &surface) : nullptr;
        auto heardIn = shows != nullptr ? openOn ("Torqwave Test Mackie Shows", &heard) : nullptr;
        check (surfaceIn != nullptr && heardIn != nullptr, "control surface: virtual ports for a simulated Mackie device, both ways");

        if (surfaceIn == nullptr || heardIn == nullptr)
            return;

        heardIn->start();
        std::vector<ControlSurface::Transport> transport;
        std::vector<juce::uint32> selected;
        std::set<juce::uint32> armed;
        surface.onTransport = [&] (auto t) { transport.push_back (t); };
        surface.onSelect = [&] (auto id) { selected.push_back (id); };
        surface.selectedTrack = [&] { return selected.empty() ? 0u : selected.back(); };
        surface.isArmed = [&] (auto id) { return armed.count (id) > 0; };
        surface.setArmed = [&] (auto id, bool on) { if (on) armed.insert (id); else armed.erase (id); };
        surface.use (std::move (surfaceIn), std::move (shows));

        // What it shows from the start.
        juce::String names;

        for (int i = 1; i <= 8; ++i)
            names += Mackie::stripText ("T" + juce::String (i));

        const auto shown = heard.waitFor (Mackie::scribble (0, names)) && heard.waitFor (Mackie::led (Mackie::stop, true));
        surface.refresh (true);
        const auto fader = heard.waitFor (Mackie::fader (0, (float) std::lround (Mackie::positionOfDb (-6.0f) * 16383.0f) / 16383.0f))
                           && heard.waitFor (Mackie::vpotRing (4, 0.0f))
                           && heard.waitFor (Mackie::scribble (56, Mackie::stripText ("  -6.0") + Mackie::stripText ("   0.0") + Mackie::stripText ("   0.0")
                                                                       + Mackie::stripText ("   0.0") + Mackie::stripText ("   0.0") + Mackie::stripText ("   0.0")
                                                                       + Mackie::stripText ("   0.0") + Mackie::stripText ("   0.0")));
        check (shown && fader, "control surface: opened, it shows the first eight tracks' names, volumes and pans, its faders at their volumes and Stop lit");

        // A fader moved on the device: the track's volume, an undo step merged while it moves.
        device->sendMessageNow (Mackie::fader (1, Mackie::positionOfDb (-20.0f)));
        device->sendMessageNow (Mackie::fader (1, Mackie::positionOfDb (-24.0f)));
        const auto volume = runUntil ([&] { return near (track (engine, 1).gainDb, -24.0f); }, 1000);
        const auto undo = engine.session.getUndoDescription();
        engine.session.undo();
        check (volume && undo == "Track Volume" && track (engine, 1).gainDb == 0.0f,
               "control surface: a fader moves its track's volume, one undo step while it moves");

        // Buttons: mute, solo, select, arm; their LEDs light.
        device->sendMessageNow (Mackie::led (Mackie::mute + 2, true));
        device->sendMessageNow (Mackie::led (Mackie::solo + 3, true));
        device->sendMessageNow (Mackie::led (Mackie::select + 4, true));
        device->sendMessageNow (Mackie::led (Mackie::recordArm + 5, true));
        const auto pressed = runUntil ([&] { return track (engine, 2).mute && track (engine, 3).solo && ! selected.empty() && armed.count (ids[5]) > 0; }, 1000);
        const auto lit = heard.waitFor (Mackie::led (Mackie::mute + 2, true)) && heard.waitFor (Mackie::led (Mackie::solo + 3, true))
                         && heard.waitFor (Mackie::led (Mackie::select + 4, true)) && heard.waitFor (Mackie::led (Mackie::recordArm + 5, true));
        device->sendMessageNow (Mackie::led (Mackie::mute + 2, true));
        const auto unmuted = runUntil ([&] { return ! track (engine, 2).mute; }, 1000) && heard.waitFor (Mackie::led (Mackie::mute + 2, false));
        check (pressed && lit && unmuted && selected.back() == ids[4] && engine.session.getUndoDescription() == "Unmute",
               "control surface: mute, solo, select and record arm act on their strip's track and light their LEDs; pressed again, mute turns off");

        // Banks: eight along, one back, and back to the start.
        device->sendMessageNow (Mackie::led (Mackie::bankRight, true));
        const auto banked = runUntil ([&] { return surface.getBankStart() == 8; }, 1000);
        const auto onStrips = surface.trackOn (0) == ids[8] && surface.trackOn (2) == ids[10] && surface.trackOn (3) == 0;
        const auto bankNames = heard.waitFor (Mackie::scribble (0, Mackie::stripText ("T9") + Mackie::stripText ("T10") + Mackie::stripText ("Drum Bus")
                                                                       + juce::String::repeatedString (" ", 35)));
        device->sendMessageNow (Mackie::led (Mackie::channelLeft, true));
        const auto channel = runUntil ([&] { return surface.getBankStart() == 7; }, 1000);
        device->sendMessageNow (Mackie::led (Mackie::bankLeft, true));
        const auto back = runUntil ([&] { return surface.getBankStart() == 0; }, 1000);
        check (banked && onStrips && bankNames && channel && back,
               "control surface: the bank buttons move the strips eight tracks along (the bus after them, empty strips blank), the channel buttons one");

        // The transport.
        for (auto note : { Mackie::play, Mackie::stop, Mackie::record, Mackie::rewind, Mackie::forward })
            device->sendMessageNow (Mackie::led (note, true));

        using T = ControlSurface::Transport;
        check (runUntil ([&] { return transport.size() == 5; }, 1000) && transport == std::vector<T> { T::play, T::stop, T::record, T::rewind, T::forward },
               "control surface: play, stop, record, rewind and forward reach the app's transport");

        // A V-pot turned: the pan, shown on the bottom line while it turns; pushed: centred.
        device->sendMessageNow (juce::MidiMessage::controllerEvent (1, 0x10 + 6, 0x05));
        const auto panned = runUntil ([&] { return near (track (engine, 6).pan, 0.1f, 1.0e-5f); }, 1000);
        surface.refresh();
        const auto ring = heard.waitFor (Mackie::vpotRing (6, 0.1f));
        const auto label = runUntil ([&]
        {
            const juce::ScopedLock sl (heard.lock);
            return std::any_of (heard.got.begin(), heard.got.end(), [] (auto& m) { return m.isSysEx() && bytes (m)[6] == 56
                                                                                         && juce::String ((const char*) m.getSysExData() + 6, 56).substring (42, 49) == "   10R "; });
        }, 1000);
        device->sendMessageNow (juce::MidiMessage::controllerEvent (1, 0x10 + 6, 0x43));
        const auto left = runUntil ([&] { return near (track (engine, 6).pan, 0.04f, 1.0e-5f); }, 1000);
        device->sendMessageNow (Mackie::led (Mackie::vpotPush + 6, true));
        const auto centred = runUntil ([&] { return track (engine, 6).pan == 0.0f; }, 1000);
        check (panned && ring && label && left && centred, "control surface: a V-pot moves its track's pan (shown on its ring and, as it turns, its strip), and pushed, centres it");

        // Motorised faders follow the volume envelope as it plays, but not while held.
        auto next = *engine.session.getState();
        next.tracks[0].volumeEnvelope.points = { { 0, -30.0f }, { 48000, -30.0f } };
        engine.session.apply (next, "Envelope");
        const auto at = [] (float db) { return Mackie::fader (0, (float) std::lround (Mackie::positionOfDb (db) * 16383.0f) / 16383.0f); };
        const auto follows = heard.waitFor (at (-30.0f));
        heard.clear();
        device->sendMessageNow (Mackie::led (Mackie::faderTouch, true));
        runUntil ([] { return false; }, 100);
        next = *engine.session.getState();
        next.tracks[0].volumeEnvelope.points = { { 0, -40.0f }, { 48000, -40.0f } };
        engine.session.apply (next, "Envelope");
        const auto held = heard.nothingLike ([] (auto& m) { return m.isPitchWheel() && m.getChannel() == 1; }, 300);
        device->sendMessageNow (Mackie::led (Mackie::faderTouch, false));
        const auto letGo = heard.waitFor (at (-40.0f));
        check (follows && held && letGo, "control surface: a motorised fader follows its track's envelope, and stays put while it is touched");

        // Writing automation from the device's fader as the session plays in Touch.
        const auto a = ids[1];
        view.setAutomationMode (a, AutomationMode::touch);
        auto& player = engine.getPlayer();
        const auto mode = player.getMode();
        player.setMode (DocumentPlayerProcessor::Mode::session);
        player.play (4800, -1);
        device->sendMessageNow (Mackie::led (Mackie::faderTouch + 1, true));
        device->sendMessageNow (Mackie::fader (1, Mackie::positionOfDb (-18.0f)));
        const auto writing = runUntil ([&] { return view.isAutomationPassUnderWay (a, AutomationParameter::volume); }, 1000);
        device->sendMessageNow (Mackie::led (Mackie::faderTouch + 1, false));
        const auto released = runUntil ([&] { return ! view.isAutomationPassUnderWay (a, AutomationParameter::volume); }, 1000);
        player.pause();
        player.setMode (mode);
        view.finishAutomationPasses();
        const auto& written = track (engine, 1);
        check (writing && released && written.gainDb == 0.0f && ! written.volumeEnvelope.isEmpty()
                   && engine.session.getUndoDescription() == "Write Volume Automation",
               "control surface: playing in Touch, a fader writes its track's volume envelope while held, and leaves the setting");
        view.setAutomationMode (0, AutomationMode::read);

        // Closed: the faders go down, the LEDs dark and the strips blank.
        heard.clear();
        surface.close();
        check (heard.waitFor (Mackie::scribble (0, juce::String::repeatedString (" ", 112))) && heard.waitFor (Mackie::fader (7, 0.0f))
                   && heard.waitFor (Mackie::led (Mackie::play, false)) && heard.nothingLike ([] (auto& m) { return m.isNoteOn (false) && m.getVelocity() > 0; }) && ! surface.isOpen(),
               "control surface: closed, it leaves the device's faders down, its LEDs dark and its strips blank");
        heardIn->stop();
       #else
        juce::ignoreUnused (engine);
       #endif
    }

    //==============================================================================
    void testLearn (AudioEngine& engine)
    {
        SessionState s;
        s.sampleRate = rate;
        s.addTrack().name = "A";
        s.addTrack().name = "B";
        engine.session.reset (s, {});
        const auto a = engine.session.getState()->tracks[0].id, b = engine.session.getState()->tracks[1].id;
        MultitrackView view (engine);
        MidiLearn learn (engine, [&] { return &view; });
        std::vector<MidiBinding> learned;
        std::vector<ControlSurface::Transport> transport;
        learn.onLearned = [&] (auto& binding) { learned.push_back (binding); };
        learn.onTransport = [&] (auto t) { transport.push_back (t); };
        const auto cc = [] (int channel, int controller, int value) { return juce::MidiMessage::controllerEvent (channel, controller, value); };
        const auto bindings = [&] { return engine.session.getState()->midiBindings; };

        const auto ignored = ! learn.offer (cc (2, 7, 100)) && ! learn.offer (juce::MidiMessage::noteOn (2, 60, (juce::uint8) 100));
        learn.learn ({ MidiBinding::Kind::volume, a });
        const auto taken = learn.offer (cc (2, 7, 100));
        const auto bound = runUntil ([&] { return bindings().size() == 1; }, 1000);
        check (ignored && taken && bound && bindings()[0] == MidiBinding { 2, 7, { MidiBinding::Kind::volume, a } } && learned.size() == 1
                   && ! learn.getLearning().has_value() && engine.session.getUndoDescription() == "MIDI Learn",
               "MIDI learn: nothing is taken until learning; then the next CC is bound to the control, an undo step");

        check (learn.offer (cc (2, 7, 64)) && runUntil ([&] { return near (track (engine, 0).gainDb, Mackie::dbOfPosition (64.0f / 127.0f), 1.0e-5f); }, 1000)
                   && ! learn.offer (cc (2, 8, 64)) && ! learn.offer (cc (3, 7, 64)),
               "MIDI learn: a bound CC moves the volume on the mixer's taper; other CCs and channels pass to the instruments");

        learn.learn ({ MidiBinding::Kind::pan, b });
        learn.handle (1, 10, 0);
        learn.handle (1, 10, 1);
        const auto hardLeft = track (engine, 1).pan == -1.0f;
        learn.handle (1, 10, 127);
        const auto hardRight = track (engine, 1).pan == 1.0f;
        learn.handle (1, 10, 64);
        check (hardLeft && hardRight && track (engine, 1).pan == 0.0f, "MIDI learn: a pan knob runs hard left to hard right, 64 the centre");

        learn.learn ({ MidiBinding::Kind::mute, a });
        learn.handle (1, 20, 127);   // learned
        learn.handle (1, 20, 127);
        const auto on = track (engine, 0).mute;
        learn.handle (1, 20, 0);
        const auto stays = track (engine, 0).mute;
        learn.handle (1, 20, 127);
        const auto off = ! track (engine, 0).mute;
        learn.handle (1, 20, 127);
        learn.learn ({ MidiBinding::Kind::solo, b });
        learn.handle (1, 21, 127);
        learn.handle (1, 21, 127);
        check (on && stays && off && track (engine, 0).mute && track (engine, 1).solo,
               "MIDI learn: a mute or solo button toggles at each press (64 and up), and does nothing let go");

        for (auto [controller, kind] : std::vector<std::pair<int, MidiBinding::Kind>> { { 30, MidiBinding::Kind::play }, { 31, MidiBinding::Kind::stop }, { 32, MidiBinding::Kind::record } })
        {
            learn.learn ({ kind, 0 });
            learn.handle (1, controller, 127);
            learn.handle (1, controller, 127);
            learn.handle (1, controller, 0);
        }

        using T = ControlSurface::Transport;
        check (transport == std::vector<T> { T::play, T::stop, T::record }, "MIDI learn: buttons can play, stop and record");

        // Learning a bound control again, or a bound CC for another control, replaces the old binding.
        const auto count = bindings().size();
        learn.learn ({ MidiBinding::Kind::volume, a });
        learn.handle (5, 1, 0);
        learn.learn ({ MidiBinding::Kind::pan, a });
        learn.handle (1, 10, 0);
        const auto now = bindings();
        const auto one = [&] (int channel, int controller) { return std::count_if (now.begin(), now.end(), [&] (auto& x) { return x.channel == channel && x.controller == controller; }); };
        check (now.size() == count && one (2, 7) == 0 && one (5, 1) == 1 && one (1, 10) == 1
                   && std::none_of (now.begin(), now.end(), [&] (auto& x) { return x.target == MidiBinding::Target { MidiBinding::Kind::pan, b }; }),
               "MIDI learn: each control has one CC and each CC one control; learning again replaces the old binding");

        // The session file.
        const auto file = juce::File::createTempFile ("tqsession");
        const auto error = SessionFile::save (*engine.session.getState(), file);
        const auto loaded = SessionFile::load (file, engine.formatManager);
        file.deleteFile();
        auto same = error.isEmpty() && loaded.error.isEmpty() && loaded.state.midiBindings.size() == now.size();

        for (size_t i = 0; same && i < now.size(); ++i)
        {
            const auto& x = now[i];
            const auto& y = loaded.state.midiBindings[i];
            const auto place = [] (const SessionState& st, juce::uint32 id) { for (size_t k = 0; k < st.tracks.size(); ++k) if (st.tracks[k].id == id) return (int) k; return -1; };
            same = x.channel == y.channel && x.controller == y.controller && x.target.kind == y.target.kind
                   && (x.target.isTrack() ? place (*engine.session.getState(), x.target.trackId) == place (loaded.state, y.target.trackId) : y.target.trackId == 0);
        }

        check (same, "MIDI learn: the bindings come back from a session file, each to its track");

        // Removing a track drops its bindings; unbinding drops a track's, or all.
        auto removed = *engine.session.getState();
        SessionEdits::removeTrack (removed, 0);
        auto unbound = *engine.session.getState();
        const auto ofB = SessionEdits::unbindMidi (unbound, b);
        auto none = unbound;
        SessionEdits::unbindMidi (none, 0);
        const auto hasTrack = [] (const SessionState& st, juce::uint32 id) { return std::any_of (st.midiBindings.begin(), st.midiBindings.end(), [&] (auto& x) { return x.target.trackId == id; }); };
        check (! hasTrack (removed, a) && hasTrack (removed, 0) && ofB == 1 && ! hasTrack (unbound, b) && hasTrack (unbound, a) && none.midiBindings.empty(),
               "MIDI learn: removing a track drops its bindings; unlearning drops a track's, or every one");

        // A knob writes automation while the session plays in Touch; its pass ends when it rests.
        view.setAutomationMode (a, AutomationMode::touch);
        auto& player = engine.getPlayer();
        const auto mode = player.getMode();
        player.setMode (DocumentPlayerProcessor::Mode::session);
        player.play (4800, -1);
        learn.handle (5, 1, 100);
        const auto writing = view.isAutomationPassUnderWay (a, AutomationParameter::volume);
        const auto start = juce::Time::getMillisecondCounterHiRes();
        const auto rested = runUntil ([&] { return ! view.isAutomationPassUnderWay (a, AutomationParameter::volume); }, 2000);
        const auto took = juce::Time::getMillisecondCounterHiRes() - start;
        player.pause();
        player.setMode (mode);
        view.finishAutomationPasses();
        check (writing && rested && took >= MidiLearn::passRestSeconds * 1000.0 && ! track (engine, 0).volumeEnvelope.isEmpty(),
               "MIDI learn: playing in Touch, a knob writes its track's envelope, and its pass ends once it rests (after "
                   + juce::String (took, 0) + " ms)");
        view.setAutomationMode (0, AutomationMode::read);
    }
}

void runControlSurfaceTests (AudioEngine& engine)
{
    std::cout << "Control surfaces:" << std::endl;
    testProtocol();
    testSurface (engine);
    testLearn (engine);
}
