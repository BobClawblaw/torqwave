// Levels are checked against what the mixer itself computes.
#pragma clang diagnostic ignored "-Wfloat-equal"

#include <JuceHeader.h>
#include "../Source/AudioEdits.h"
#include "../Source/AudioEngine.h"
#include "../Source/Bounce.h"
#include "../Source/MixerView.h"
#include "../Source/MultitrackView.h"
#include "../Source/Session.h"
#include "../Source/SessionMixer.h"
#include "TestUtils.h"
#include "AutomationModeTests.h"

namespace
{
    using TestUtils::check;

    constexpr double rate = 48000.0;
    constexpr juce::int64 back = 4800;   // MultitrackView::returnSeconds at 48 kHz

    AudioDocument::Snapshot level (int frames, float value)
    {
        juce::AudioBuffer<float> b (1, frames);
        juce::FloatVectorOperations::fill (b.getWritePointer (0), value, frames);
        return AudioEdits::fromBuffer (std::move (b));
    }

    juce::AudioBuffer<float> mix (const SessionState& s, juce::int64 start = 0, int frames = 4096)
    {
        SessionMixer::Buffers buffers;
        juce::AudioBuffer<float> out (2, frames);
        out.clear();
        SessionMixer::render (s, start, out, 0, frames, buffers);
        return out;
    }

    float maxDifference (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        auto most = 0.0f;

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < a.getNumSamples(); ++i)
                most = juce::jmax (most, std::abs (a.getSample (ch, i) - b.getSample (ch, i)));

        return most;
    }

    bool same (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < a.getNumSamples(); ++i)
                if (a.getSample (ch, i) != b.getSample (ch, i))
                    return false;

        return true;
    }

    /** Two tracks of a steady level, one at -6 dB, and a bus. */
    SessionState twoTracks()
    {
        SessionState s;
        s.sampleRate = rate;
        s.addTrack().gainDb = -6.0f;
        s.addTrack();
        s.addBus();
        SessionEdits::addClip (s, 0, juce::File ("/takes/a.wav"), level (9000, 0.5f), 0);
        SessionEdits::addClip (s, 1, juce::File ("/takes/b.wav"), level (9000, 0.25f), 0);
        return s;
    }

    void testModel (juce::AudioFormatManager& formats)
    {
        auto s = twoTracks();
        const auto a = s.tracks[0].id;
        SessionEdits::setEnvelope (s, a, AutomationParameter::volume, { { { 0, -40.0f }, { 9000, 0.0f } } });

        auto off = s;
        SessionEdits::setAutomationMode (off, a, AutomationMode::off);
        auto plain = s;
        SessionEdits::setEnvelope (plain, a, AutomationParameter::volume, {});
        check (same (mix (off), mix (plain)) && ! same (mix (s), mix (plain)) && off.tracks[0].valueAt (AutomationParameter::volume, 1000) == -6.0f
                   && ! off.tracks[0].follows (AutomationParameter::volume),
               "automation modes: in Off a track's envelopes don't play, and its value is its setting");

        // Modes are kept out of undo, as the loop is.
        Session session;
        session.reset (s, {});
        session.setAutomationMode (a, AutomationMode::latch);
        auto next = *session.getState();
        next.tracks[1].gainDb = -2.0f;
        session.apply (std::move (next), "Track Volume");
        session.undo();
        const auto keptByUndo = session.getState()->tracks[0].automationMode == AutomationMode::latch;
        session.apply (s, "Stale Copy");   // a state from before the mode changed
        check (keptByUndo && session.getState()->tracks[0].automationMode == AutomationMode::latch && session.getHistory().size() == 2,
               "automation modes: not undo steps; an undo, or a state applied from before, leaves each track's mode");

        session.setAutomationMode (0, AutomationMode::write);
        const auto all = std::all_of (session.getState()->tracks.begin(), session.getState()->tracks.end(),
                                      [] (auto& t) { return t.automationMode == AutomationMode::write; });

        // Saved with the session, and Read is the default.
        auto toSave = *session.getState();
        SessionEdits::setAutomationMode (toSave, toSave.tracks[1].id, AutomationMode::read);

        for (auto& t : toSave.tracks)
            t.clips.clear();   // no audio files to save

        const auto file = juce::File::createTempFile ("tqsession");
        const auto error = SessionFile::save (toSave, file);
        const auto loaded = SessionFile::load (file, formats);
        check (all && error.isEmpty() && loaded.state.tracks[0].automationMode == AutomationMode::write
                   && loaded.state.tracks[1].automationMode == AutomationMode::read && loaded.state.tracks[2].automationMode == AutomationMode::write,
               "automation modes: all tracks set at once, and each track's mode comes back from a session file");
        file.deleteFile();
    }

    /** The buttons in a component and its children whose text is this. */
    int buttonsSaying (juce::Component& c, const juce::String& text)
    {
        auto n = 0;

        if (auto* b = dynamic_cast<juce::TextButton*> (&c); b != nullptr && b->isVisible() && b->getButtonText() == text)
            ++n;

        for (auto* child : c.getChildren())
            n += buttonsSaying (*child, text);

        return n;
    }

    void testPasses (AudioEngine& engine)
    {
        auto initial = twoTracks();
        const auto a = initial.tracks[0].id, b = initial.tracks[1].id;
        engine.session.reset (initial, {});
        MultitrackView view (engine);
        const auto track = [&] (juce::uint32 id) { return *engine.session.getState()->findTrack (id); };
        const auto volume = [&] (juce::uint32 id, juce::int64 frame) { return track (id).valueAt (AutomationParameter::volume, frame); };

        // Off and Read: a control moved while playing writes nothing.
        view.setAutomationMode (a, AutomationMode::read);
        view.startAutomationPass (a, AutomationParameter::volume, 10000, -12.0f);
        check (! view.isAutomationPassUnderWay (a, AutomationParameter::volume) && track (a).volumeEnvelope.isEmpty(),
               "automation passes: in Read nothing is written");

        // Touch: written while held, then back to what was there.
        view.setSize (900, 400);
        view.setAutomationMode (a, AutomationMode::touch);
        view.setAutomationMode (b, AutomationMode::off);
        const auto shown = [&] { return buttonsSaying (view, "T") == 1 && buttonsSaying (view, "Off") == 1 && buttonsSaying (view, "A") == 1; };
        TestUtils::runUntil (shown, 2000);
        check (shown(),
               "automation passes: each track header's A button shows its mode (T, Off, and A for Read on the bus)");
        view.setAutomationMode (b, AutomationMode::read);

        view.startAutomationPass (a, AutomationParameter::volume, 10000, -12.0f);
        view.continueAutomationPass (a, AutomationParameter::volume, 20000, -18.0f);
        view.releaseAutomationPass (a, AutomationParameter::volume);
        view.followPlay (40000, std::nullopt);
        check (! view.isAutomationPassUnderWay (a, AutomationParameter::volume) && volume (a, 15000) == -15.0f && volume (a, 20000) == -18.0f
                   && volume (a, 20000 + back) == -6.0f && volume (a, 40000) == -6.0f,
               "automation passes: Touch writes while the control is held, and goes back to the setting when it is let go");
        view.finishAutomationPasses();
        engine.session.undo();

        // Latch: let go, it holds its last value until playback stops.
        view.setAutomationMode (a, AutomationMode::latch);
        view.startAutomationPass (a, AutomationParameter::volume, 10000, -12.0f);
        view.continueAutomationPass (a, AutomationParameter::volume, 20000, -18.0f);
        view.releaseAutomationPass (a, AutomationParameter::volume);
        view.followPlay (40000, std::nullopt);
        const auto stillLatched = view.isAutomationPassUnderWay (a, AutomationParameter::volume);
        view.finishAutomationPasses();
        check (stillLatched && volume (a, 30000) == -18.0f && volume (a, 40000) == -18.0f && volume (a, 40000 + back) == -6.0f
                   && engine.session.getUndoDescription() == "Write Volume Automation",
               "automation passes: Latch goes on writing the last value once let go, until playback stops");
        engine.session.undo();

        // Write: from play to stop, as the controls stand, moved or not; the pan too.
        view.setAutomationMode (a, AutomationMode::write);
        view.setAutomationMode (b, AutomationMode::read);
        SessionEdits::setEnvelope (initial, a, AutomationParameter::volume, { { { 0, -30.0f } } });
        engine.session.apply (initial, "Volume Envelope");
        view.followPlay (30000, 24000);   // playback began at 24000
        view.followPlay (48000, std::nullopt);
        view.startAutomationPass (a, AutomationParameter::volume, 48000, -10.0f);   // the fader moved, at the timer's frame
        view.followPlay (60000, std::nullopt);
        view.finishAutomationPasses();
        const auto written = track (a);
        check (written.valueAt (AutomationParameter::volume, 12000) == -30.0f && written.valueAt (AutomationParameter::volume, 24000) == -30.0f
                   && written.valueAt (AutomationParameter::volume, 47999) == -30.0f && written.valueAt (AutomationParameter::volume, 48000) == -10.0f
                   && written.valueAt (AutomationParameter::volume, 60000) == -10.0f && written.valueAt (AutomationParameter::volume, 60000 + back) == -30.0f
                   && written.panEnvelope.points.size() >= 2 && written.valueAt (AutomationParameter::pan, 40000) == 0.0f
                   && engine.session.getUndoDescription() == "Write Volume Automation",
               "automation passes: Write writes the volume and pan from where playback began to where it stopped, the fader's move in it");
        engine.session.undo();

        // Two tracks at once: each writes its own envelope.
        view.setAutomationMode (a, AutomationMode::touch);
        view.setAutomationMode (b, AutomationMode::latch);
        view.startAutomationPass (a, AutomationParameter::volume, 10000, -12.0f);
        view.startAutomationPass (b, AutomationParameter::pan, 15000, 0.5f);
        view.continueAutomationPass (a, AutomationParameter::volume, 20000, -18.0f);
        view.releaseAutomationPass (a, AutomationParameter::volume);
        view.releaseAutomationPass (b, AutomationParameter::pan);
        view.followPlay (30000, std::nullopt);
        view.finishAutomationPasses();
        const auto both = track (a).valueAt (AutomationParameter::volume, 20000) == -18.0f && track (b).valueAt (AutomationParameter::pan, 30000) == 0.5f
                          && track (b).valueAt (AutomationParameter::pan, 30000 + back) == 0.0f;
        engine.session.undo();
        engine.session.undo();
        check (both && track (a).volumeEnvelope.points.size() == 1 && track (b).panEnvelope.isEmpty(),
               "automation passes: two tracks write at once, each its own envelope and mode; each pass that starts is an undo step");

        view.setAutomationMode (0, AutomationMode::read);
    }

    void testMixerWriting (AudioEngine& engine)
    {
        auto initial = twoTracks();
        const auto a = initial.tracks[0].id;
        engine.session.reset (initial, {});
        MultitrackView view (engine);
        MixerView mixer (engine);
        mixer.setSize (600, 500);
        mixer.setAutomationWriter (&view);
        view.setAutomationMode (a, AutomationMode::touch);
        TestUtils::runUntil ([] { return false; }, 50);
        auto* fader = mixer.findFader (a);

        auto& player = engine.getPlayer();
        const auto mode = player.getMode();
        player.setMode (DocumentPlayerProcessor::Mode::session);
        player.play (4800, -1);
        const auto first = player.getPositionFrames();
        fader->setValue (-20.0, juce::sendNotificationSync);
        player.setPositionFrames (first + 9600);
        fader->setValue (-30.0, juce::sendNotificationSync);
        const auto underWay = view.isAutomationPassUnderWay (a, AutomationParameter::volume);
        fader->onDragEnd();
        player.pause();
        player.setMode (mode);
        const auto t = *engine.session.getState()->findTrack (a);
        check (underWay && ! view.isAutomationPassUnderWay (a, AutomationParameter::volume) && t.gainDb == -6.0f
                   && t.valueAt (AutomationParameter::volume, first + 9600) == -30.0f && t.volumeEnvelope.points.size() >= 3
                   && engine.session.getUndoDescription() == "Write Volume Automation",
               "automation passes: the mixer's fader writes the volume envelope while playing in Touch, and leaves the setting");

        mixer.setAutomationWriter (nullptr);
        view.setAutomationMode (0, AutomationMode::read);
    }

    void testVca (juce::AudioFormatManager& formats)
    {
        auto s = twoTracks();
        const auto a = s.tracks[0].id, b = s.tracks[1].id, bus = s.tracks[2].id;
        const auto vca = s.addVca().id;
        auto reference = s;
        reference.tracks[0].gainDb = -9.0f;

        check (SessionEdits::setTrackVca (s, a, vca) && ! SessionEdits::setTrackVca (s, vca, vca) && ! SessionEdits::setTrackVca (s, b, bus)
                   && s.tracks.back().name == "VCA 1" && ! s.tracks.back().takesClips(),
               "VCA faders: a track joins one; a VCA can't join one, and a bus isn't one");

        s.tracks.back().gainDb = -3.0f;
        check (same (mix (s), mix (reference)), "VCA faders: a member plays at its volume plus the VCA's, sample for sample (-6 dB and -3 dB as -9 dB)");

        // Its envelope: a ramp, as the member's own envelope shifted by the member's volume.
        SessionEdits::setEnvelope (s, vca, AutomationParameter::volume, { { { 0, -20.0f }, { 4096, 0.0f } } });
        auto ramp = reference;
        SessionEdits::setEnvelope (ramp, a, AutomationParameter::volume, { { { 0, -26.0f }, { 4096, -6.0f } } });
        const auto rampDifference = maxDifference (mix (s), mix (ramp));
        check (rampDifference < 1.0e-6f, "VCA faders: its envelope moves its members' levels as theirs would (largest difference "
                                             + juce::String (rampDifference, 9) + ")");

        // Off ignores it; a pan envelope isn't taken.
        SessionEdits::setAutomationMode (s, vca, AutomationMode::off);
        check (same (mix (s), mix (reference)) && ! SessionEdits::setEnvelope (s, vca, AutomationParameter::pan, { { { 0, 0.5f } } }),
               "VCA faders: in Off its envelope is ignored; it has no pan to automate");
        SessionEdits::setAutomationMode (s, vca, AutomationMode::read);
        SessionEdits::setEnvelope (s, vca, AutomationParameter::volume, {});

        // Mute and solo act on its members.
        auto muted = s;
        for (auto& t : muted.tracks) if (t.id == vca) t.mute = true;
        auto withoutA = reference;
        withoutA.tracks[0].mute = true;
        auto soloed = s;
        for (auto& t : soloed.tracks) if (t.id == vca) t.solo = true;
        auto onlyA = reference;
        onlyA.tracks[1].mute = true;
        check (same (mix (muted), mix (withoutA)) && same (mix (soloed), mix (onlyA)),
               "VCA faders: muting it mutes its members; soloing it solos them");

        // A bus joins one too.
        auto busMix = s;
        SessionEdits::setTrackVca (busMix, a, 0);
        SessionEdits::setSend (busMix, b, { bus, -6.0f, false });
        SessionEdits::setTrackVca (busMix, bus, vca);
        auto busReference = busMix;
        SessionEdits::setTrackVca (busReference, bus, 0);
        for (auto& t : busReference.tracks) if (t.id == bus) t.gainDb = -3.0f;
        check (same (mix (busMix), mix (busReference)), "VCA faders: a bus joins one, its volume and the VCA's added");

        // A bounce of a member alone plays it at the VCA fader's level.
        const auto alone = Bounce::isolate (s, a);
        check (alone.tracks.size() == 2 && alone.vcaOf (alone.tracks[0]) != nullptr && ! alone.tracks[1].mute && ! alone.tracks[1].solo,
               "VCA faders: a member bounced alone keeps its VCA fader, so its level");

        // Saved and loaded; deleting it lets its members go.
        for (auto& t : s.tracks)
            t.clips.clear();

        const auto file = juce::File::createTempFile ("tqsession");
        const auto error = SessionFile::save (s, file);
        const auto loaded = SessionFile::load (file, formats);
        const auto& got = loaded.state;
        const auto restored = error.isEmpty() && got.tracks.size() == 4 && got.tracks[3].isVca && got.tracks[3].gainDb == -3.0f
                          && got.tracks[0].vcaId == got.tracks[3].id && got.tracks[1].vcaId == 0;
        file.deleteFile();
        SessionEdits::removeTrack (s, 3);
        check (restored && s.tracks[0].vcaId == 0, "VCA faders: membership comes back from a session file; deleting the VCA lets its members go");
    }
}

void runAutomationModeTests (AudioEngine& engine)
{
    testModel (engine.formatManager);
    testPasses (engine);
    testMixerWriting (engine);
    testVca (engine.formatManager);
}
