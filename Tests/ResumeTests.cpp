#include <JuceHeader.h>
#include "../Source/AudioEdits.h"
#include "../Source/AudioFileIO.h"
#include "../Source/DrumMachine.h"
#include "../Source/Home.h"
#include "../Source/MultitrackView.h"
#include "../Source/Resume.h"
#include "../Source/Sampler.h"
#include "TestUtils.h"
#include "ResumeTests.h"

namespace
{
    using TestUtils::check;
    using TestUtils::runUntil;

    AudioDocument::Snapshot tone (int frames, double hz, float level)
    {
        juce::AudioBuffer<float> b (2, frames);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < frames; ++i)
                b.setSample (ch, i, level * (float) std::sin (juce::MathConstants<double>::twoPi * hz * (i + 7 * ch) / 48000.0));

        return AudioEdits::fromBuffer (std::move (b));
    }

    bool identical (const AudioSnapshot& a, const AudioSnapshot& b)
    {
        if (a.getNumChannels() != b.getNumChannels() || a.getNumFrames() != b.getNumFrames())
            return false;

        juce::AudioBuffer<float> x (a.getNumChannels(), (int) a.getNumFrames()), y (b.getNumChannels(), (int) b.getNumFrames());
        a.read (x, 0, 0, x.getNumSamples());
        b.read (y, 0, 0, y.getNumSamples());

        for (int ch = 0; ch < x.getNumChannels(); ++ch)
            if (std::memcmp (x.getReadPointer (ch), y.getReadPointer (ch), sizeof (float) * (size_t) x.getNumSamples()) != 0)
                return false;

        return true;
    }

    bool samePlace (const FilePlace& a, const FilePlace& b)
    {
        return a.selection == b.selection && a.cursor == b.cursor && juce::exactlyEqual (a.viewStart, b.viewStart)
            && juce::exactlyEqual (a.framesPerPixel, b.framesPerPixel) && a.band == b.band;
    }

    bool samePlace (const SessionPlace& a, const SessionPlace& b)
    {
        return a.cursor == b.cursor && a.selection == b.selection && juce::exactlyEqual (a.viewStart, b.viewStart)
            && juce::exactlyEqual (a.framesPerPixel, b.framesPerPixel) && a.scrollY == b.scrollY && a.selectedTrack == b.selectedTrack;
    }

    /** Every file under a folder with its size and time, to see that nothing was written there. */
    juce::String listing (const juce::File& folder)
    {
        juce::StringArray lines;

        for (auto& f : folder.findChildFiles (juce::File::findFiles, true))
            lines.add (f.getRelativePathFrom (folder) + " " + juce::String (f.getSize()) + " " + juce::String (f.getLastModificationTime().toMilliseconds()));

        lines.sort (false);
        return lines.joinIntoString ("\n");
    }

    /** A view of the engine's session, the size the others' tests use, tall enough for two lanes of four. */
    void size (MultitrackView& view)
    {
        view.setSize (MultitrackView::headerWidth + 1000 + 12, MultitrackView::rulerHeight + 2 * MultitrackView::trackHeight + 12);
    }

    void testSetting()
    {
        juce::PropertySet settings;
        settings.setValue ("lastFile", juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Last Take.wav").getFullPathName());
        settings.setValue (Home::startKey, 2);
        check (Home::startWith (settings) == Home::Start::leftOff && Home::fileToReopen (settings, "lastFile") == juce::File(),
               "left off: the third Start choice is kept as 2, and the last file is not reopened by itself with it");
        settings.setValue (Home::startKey, 7);
        check (Home::startWith (settings) == Home::Start::homeScreen, "left off: a Start value that means nothing starts on the home screen");
    }

    /** Quits with three saved files (one with changes not saved), a file never saved and a saved
        session open, the second file and the session in front; starts again and finds it all back. */
    void testQuitAndStart (const juce::File& work)
    {
        const auto user = work.getChildFile ("User"), data = work.getChildFile ("Data/Left Off");
        user.createDirectory();
        const auto a = user.getChildFile ("Take A.wav"), b = user.getChildFile ("Take B.wav"), c = user.getChildFile ("Take C.flac"),
                   video = user.getChildFile ("Scene.mp4"), sessionFile = user.getChildFile ("Mix.tqsession"),
                   project = user.getChildFile ("Mix.tqproject");

        const FilePlace placeA { { 1000, 9000 }, 1000, 500.0, 12.5, { 200.0, 4000.0 }, {} };
        const FilePlace placeB { { 300, 300 }, 4321, 250.0, 3.0, {}, {} };
        const FilePlace placeC { { 10, 20 }, 15, 1.0, 0.5, {}, {} };
        SessionPlace sessionPlace;
        juce::String before;

        {
            AudioEngine engine;
            OpenFiles files (engine);
            AudioFileIO::save (*tone (48000, 440.0, 0.25f), 48000.0, a, 24, engine.formatManager);
            AudioFileIO::save (*tone (24000, 660.0, 0.5f), 48000.0, b, 16, engine.formatManager);
            AudioFileIO::save (*tone (36000, 880.0, 0.4f), 48000.0, c, 24, engine.formatManager);
            video.replaceWithText ("not really a video");
            project.replaceWithText ("not really a project");

            // The session: four tracks, clips on two, saved.
            SessionState session;
            session.sampleRate = 48000.0;
            SessionEdits::addClip (session, 0, a, AudioFileIO::load (a, engine.formatManager).snapshot, 4800);
            SessionEdits::addClip (session, 1, b, AudioFileIO::load (b, engine.formatManager).snapshot, 96000);
            session.addTrack();
            session.addTrack();
            const auto saved = SessionFile::save (session, sessionFile);
            engine.session.reset (std::move (session), sessionFile);

            // The files: A with its video, B changed and not saved, C, and one never saved; B shown.
            engine.adoptFile (a, AudioFileIO::load (a, engine.formatManager));
            engine.document.setVideo (video);
            files.prepareForAnother (placeA);
            engine.adoptFile (b, AudioFileIO::load (b, engine.formatManager));
            engine.document.apply (AudioEdits::gain (*engine.document.getSnapshot(), 0, 24000, -6.0f), "Gain");
            files.prepareForAnother (placeB);
            engine.adoptFile (c, AudioFileIO::load (c, engine.formatManager));
            files.prepareForAnother (placeC);
            AudioFileIO::Loaded generated;
            generated.snapshot = tone (12000, 1000.0, 0.3f);
            generated.sampleRate = 48000.0;
            generated.bitDepth = 32;
            engine.adoptFile ({}, std::move (generated));
            engine.document.apply (AudioEdits::gain (*engine.document.getSnapshot(), 0, 12000, -3.0f), "Gain");
            files.activate (1, {});   // B shown: the editor is asked for its place
            const auto unsaved = files.unsaved();

            // The session's view: zoomed in, scrolled along and down, a range, the third track selected.
            MultitrackView view (engine);
            size (view);
            view.setPlace ({ 123456, { 48000, 72000 }, 40000.0, 96.0, 60, 2 });
            sessionPlace = view.getPlace();
            const auto placed = sessionPlace.cursor == 123456 && sessionPlace.selection == juce::Range<juce::int64> (48000, 72000)
                             && juce::exactlyEqual (sessionPlace.viewStart, 40000.0) && juce::exactlyEqual (sessionPlace.framesPerPixel, 96.0)
                             && sessionPlace.scrollY == 60 && sessionPlace.selectedTrack == 2;
            check (saved.isEmpty() && files.size() == 4 && unsaved.size() == 2 && placed,
                   "left off: three saved files (one with unsaved changes), a file never saved and a saved session are open, the session's view placed");

            // Quit: what is open is noted, and nothing else is written anywhere.
            before = listing (user);
            Resume::Keeper keeper (data);
            const auto state = Resume::gather (files, placeB, engine, true, sessionPlace, project);
            const auto error = keeper.note (state);
            check (error.isEmpty() && Resume::isKept (data) && data.getNumberOfChildFiles (juce::File::findFilesAndDirectories) == 1
                       && listing (user) == before && state.open.files.size() == 3,
                   "left off: quitting writes one note in the app's data folder, with the three files that are on disk, and nothing in the user's folder");
        }

        // The next start, with nothing open.
        AudioEngine engine;
        OpenFiles files (engine);
        auto loaded = Resume::load (data);
        const auto read = loaded.found && loaded.missing.isEmpty() && loaded.state.open.files.size() == 3 && loaded.audio.size() == 3;
        const auto place = Resume::restoreFiles (loaded, files, engine);
        const auto onDisk = AudioFileIO::load (b, engine.formatManager).snapshot;

        check (read && files.size() == 3 && files.documentAt (0).getFile() == a && files.documentAt (1).getFile() == b
                   && files.documentAt (2).getFile() == c && loaded.missing.isEmpty(),
               "left off: the next start reopens the three files, in their order");
        check (files.getActiveIndex() == 1 && engine.document.getFile() == b && samePlace (place, placeB)
                   && samePlace (files.placeAt (0), placeA) && samePlace (files.placeAt (2), placeC),
               "left off: the file that was shown is shown, and each file has its selection, cursor, zoom and scroll");
        check (onDisk != nullptr && identical (*engine.document.getSnapshot(), *onDisk) && ! engine.document.isDirty() && files.unsaved().isEmpty(),
               "left off: a file whose changes were not saved at quit comes back as it is on disk, with nothing to save");
        check (files.documentAt (0).getVideo() == video && files.documentAt (2).getVideo() == juce::File(),
               "left off: a file's video is attached again");
        check (loaded.state.open.multitrack && loaded.state.open.session == sessionFile && loaded.state.project == project,
               "left off: the session's file, its being in front and the project are kept");

        // The session opened as the main window opens it, and its view put back.
        auto session = SessionFile::load (loaded.state.open.session, engine.formatManager);
        const auto tracks = session.state.tracks.size();
        engine.session.reset (std::move (session.state), loaded.state.open.session);
        MultitrackView view (engine);
        size (view);
        view.setPlace (loaded.state.sessionPlace);
        check (session.error.isEmpty() && tracks >= 4 && samePlace (view.getPlace(), sessionPlace) && view.getSelectedTrack() == 2
                   && view.getCursor() == 123456,
               "left off: the session's cursor, time range, zoom, scroll and selected track are back where they were");
        check (listing (user) == before, "left off: starting again writes nothing in the user's folder either");

        // A place with no zoom kept moves the cursor and leaves the view alone; a track that isn't there is not selected.
        view.setPlace ({ 500, {}, 0.0, 0.0, 0, 99 });
        check (view.getCursor() == 500 && juce::exactlyEqual (view.getPlace().framesPerPixel, 96.0) && view.getSelectedTrack() == 2,
               "left off: a session place without a zoom leaves the zoom, and a track that is no longer there is not selected");
    }

    /** What was open is partly gone, changed or unreadable at the next start. */
    void testMissingAndChanged (const juce::File& work)
    {
        const auto user = work.getChildFile ("User 2"), data = work.getChildFile ("Data 2/Left Off");
        user.createDirectory();
        const auto a = user.getChildFile ("One.wav"), b = user.getChildFile ("Two.wav"), c = user.getChildFile ("Three.wav"),
                   d = user.getChildFile ("Four.wav"), sessionFile = user.getChildFile ("Gone.tqsession"), project = user.getChildFile ("Gone.tqproject");
        const FilePlace placeA { { 40000, 47000 }, 46000, 30000.0, 20.0, {}, {} };

        {
            AudioEngine engine;
            OpenFiles files (engine);

            for (auto& f : { a, b, c, d })
                AudioFileIO::save (*tone (48000, 440.0, 0.25f), 48000.0, f, 24, engine.formatManager);

            SessionState session;
            session.sampleRate = 48000.0;
            session.addTrack();
            SessionFile::save (session, sessionFile);
            engine.session.reset (std::move (session), sessionFile);
            project.replaceWithText ("not really a project");

            engine.adoptFile (a, AudioFileIO::load (a, engine.formatManager));
            files.prepareForAnother (placeA);
            engine.adoptFile (b, AudioFileIO::load (b, engine.formatManager));
            files.prepareForAnother ({});
            engine.adoptFile (c, AudioFileIO::load (c, engine.formatManager));
            files.prepareForAnother ({});
            engine.adoptFile (d, AudioFileIO::load (d, engine.formatManager));   // D shown

            Resume::Keeper keeper (data);
            keeper.note (Resume::gather (files, {}, engine, true, {}, project));

            // Then: A is replaced by a shorter file, B is moved away, C is no longer audio, the
            // session and the project are deleted, and D (the one shown) is deleted too.
            AudioFileIO::save (*tone (12000, 220.0, 0.5f), 48000.0, a, 16, engine.formatManager);
            b.moveFileTo (user.getChildFile ("Elsewhere.wav"));
            c.replaceWithText ("this is no longer audio");
            d.deleteFile();
            sessionFile.deleteFile();
            project.deleteFile();
        }

        AudioEngine engine;
        OpenFiles files (engine);
        auto loaded = Resume::load (data);
        const auto place = Resume::restoreFiles (loaded, files, engine);
        const auto said = loaded.missing.joinIntoString ("\n");

        check (loaded.found && files.size() == 1 && engine.document.getFile() == a && engine.document.getNumFrames() == 12000,
               "left off: with files gone, moved or no longer readable, the start still reopens the one that is there, as it is on disk now");
        check (samePlace (place, placeA), "left off: a file that changed on disk keeps the place noted for it (the editor limits it to the file)");
        check (loaded.missing.size() == 5 && said.contains (b.getFullPathName()) && said.contains (d.getFullPathName())
                   && said.contains (sessionFile.getFullPathName()) && said.contains (project.getFullPathName()) && said.contains ("Three.wav: "),
               "left off: what could not be reopened is named: the moved and deleted files, the unreadable one with why, the session and the project");
        check (loaded.state.open.session == juce::File() && loaded.state.project == juce::File() && loaded.state.open.multitrack,
               "left off: a session and a project that are gone are left out, the view that was in front kept");

        // What couldn't be reopened waits in the note, each with its place, and is tried again.
        const auto& waiting = loaded.state.waiting;
        const auto waitingFor = [&waiting] (const juce::File& f)
        {
            return std::any_of (waiting.files.begin(), waiting.files.end(), [&f] (auto& w) { return w.file == f; });
        };
        check (waiting.files.size() == 3 && waitingFor (b) && waitingFor (c) && waitingFor (d) && waiting.session == sessionFile
                   && waiting.project == project,
               "left off: the three files, the session and the project that could not be reopened are kept waiting");

        // The run goes on with One.wav open; at quit the note holds it and what is waiting.
        Resume::Keeper keeper (data);
        keeper.restored();
        const auto quit = Resume::gather (files, placeA, engine, true, {}, {}, waiting);
        keeper.note (quit);
        check (quit.open.files.size() == 1 && quit.waiting.files.size() == 3 && quit.waiting.session == sessionFile && ! quit.isEmpty(),
               "left off: quitting notes what is open and what is still waiting");

        // Before the next start two.wav is moved back and the session is there again (the drive
        // connected); Three.wav is still not audio, Four.wav and the project still gone.
        user.getChildFile ("Elsewhere.wav").moveFileTo (b);
        {
            SessionState session;
            session.sampleRate = 48000.0;
            session.addTrack();
            SessionFile::save (session, sessionFile);
        }

        AudioEngine next;
        OpenFiles nextFiles (next);
        auto again = Resume::load (data);
        Resume::restoreFiles (again, nextFiles, next);
        const auto saidAgain = again.missing.joinIntoString ("\n");
        check (nextFiles.size() == 2 && nextFiles.documentAt (0).getFile() == a && nextFiles.documentAt (1).getFile() == b
                   && next.document.getFile() == a && again.state.open.session == sessionFile,
               "left off: at the next start what is back is reopened after the others, and the session with it");
        check (again.state.waiting.files.size() == 2 && again.state.waiting.session == juce::File() && again.state.waiting.project == project
                   && again.missing.size() == 3 && saidAgain.contains (d.getFullPathName()) && saidAgain.contains ("Three.wav: ")
                   && saidAgain.contains (project.getFullPathName()) && ! saidAgain.contains (b.getFullPathName()),
               "left off: what is still away goes on waiting and is named again; what came back is not");

        // A waiting file opened by hand meanwhile stops waiting; a session or project opened in place of a waiting one replaces it.
        next.session.reset (SessionState::newEmpty (48000.0), a.getSiblingFile ("Other.tqsession"));
        Resume::Waiting w;
        w.files.push_back ({ a, {}, {}, nullptr });
        w.files.push_back ({ d, {}, {}, nullptr });
        w.session = sessionFile;
        w.project = project;
        const auto merged = Resume::gather (nextFiles, {}, next, false, {}, a.getSiblingFile ("Other.tqproject"), w);
        check (merged.waiting.files.size() == 1 && merged.waiting.files[0].file == d && merged.waiting.session == juce::File()
                   && merged.waiting.project == juce::File(),
               "left off: a waiting file that is open again stops waiting, and an open session and project replace waiting ones");

        // Forgotten (an empty waiting list), with everything closed: nothing is left to come back to.
        nextFiles.closeAll();
        next.session.reset (SessionState::newEmpty (48000.0), {});
        const auto onlyWaiting = Resume::gather (nextFiles, {}, next, false, {}, {}, again.state.waiting);
        const auto forgotten = Resume::gather (nextFiles, {}, next, false, {}, {}, {});
        check (! onlyWaiting.isEmpty() && forgotten.isEmpty(),
               "left off: with everything closed, what is waiting is still kept for the next start, until it is forgotten");

        // A note that can't be read, or from a newer version, is no note; nothing is opened.
        data.getChildFile ("Left Off.xml").replaceWithText ("<TORQWAVE_LEFT_OFF version=\"999\"/>");
        const auto newer = ! Resume::isKept (data) && ! Resume::load (data).found;
        data.getChildFile ("Left Off.xml").replaceWithText ("not xml at all");
        const auto garbage = ! Resume::isKept (data) && ! Resume::load (data).found;
        Resume::clear (data);
        check (newer && garbage && ! Resume::isKept (data) && ! Resume::load (data).found,
               "left off: a note from a newer version, one that can't be read, or none at all, reopens nothing");
    }

    /** What the note leaves out, and when it is written. */
    void testKeeping (const juce::File& work)
    {
        const auto user = work.getChildFile ("User 3"), data = work.getChildFile ("Data 3/Left Off");
        user.createDirectory();
        const auto a = user.getChildFile ("Kept.wav"), b = user.getChildFile ("Other.wav");
        const auto note = data.getChildFile ("Left Off.xml");
        AudioEngine engine;
        OpenFiles files (engine);
        AudioFileIO::save (*tone (4800, 440.0, 0.25f), 48000.0, a, 24, engine.formatManager);
        AudioFileIO::save (*tone (4800, 550.0, 0.25f), 48000.0, b, 24, engine.formatManager);

        // Nothing open: nothing to keep, and no folder made for it.
        Resume::Keeper first (data);
        const auto nothing = Resume::gather (files, {}, engine, false, {}, {});
        check (nothing.isEmpty() && first.note (nothing).isEmpty() && ! data.exists() && first.keepsEarlier(),
               "left off: with nothing open there is nothing to keep, and no note is written");

        // A file never saved, and one opened from a session clip, are not in the note.
        AudioFileIO::Loaded generated;
        generated.snapshot = tone (2400, 1000.0, 0.3f);
        generated.sampleRate = 48000.0;
        generated.bitDepth = 32;
        engine.adoptFile ({}, std::move (generated));
        files.prepareForAnother ({});
        engine.adoptFile (b, AudioFileIO::load (b, engine.formatManager));
        auto link = std::make_shared<SoundDesign::ClipEdit>();
        link->clipId = 7;
        link->clipName = "Hit";
        files.setLink (files.getActiveIndex(), link);
        check (Resume::gather (files, {}, engine, false, {}, {}).isEmpty() && first.note (Resume::gather (files, {}, engine, false, {}, {})).isEmpty()
                   && ! note.existsAsFile(),
               "left off: a file never saved and a file opened from a session clip are not noted");

        files.prepareForAnother ({});
        engine.adoptFile (a, AudioFileIO::load (a, engine.formatManager));
        const auto one = Resume::gather (files, {}, engine, false, {}, {});
        const auto wrote = one.open.files.size() == 1 && one.open.files[0].file == a && first.note (one).isEmpty() && note.existsAsFile()
                        && ! first.keepsEarlier();

        // The same again is not written again (the note, taken away behind its back, stays away).
        note.deleteFile();
        const auto notAgain = first.note (one).isEmpty() && ! note.existsAsFile();
        auto moved = one;
        moved.open.files[0].place.cursor = 99;
        const auto changed = first.note (moved).isEmpty() && note.existsAsFile();
        check (wrote && notAgain && changed, "left off: the note is written when what is open changes, and not again while it is the same");

        // The next run: opening the app and closing it with nothing open leaves the note on offer.
        Resume::Keeper next (data);
        const auto stays = next.keepsEarlier() && next.note (nothing).isEmpty() && note.existsAsFile() && Resume::load (data).state.open.files.size() == 1;

        // Once it has been brought back, closing everything means there is nothing to come back to.
        next.restored();
        const auto cleared = ! next.keepsEarlier() && next.note (nothing).isEmpty() && ! note.existsAsFile();
        check (stays && cleared,
               "left off: a run with nothing opened keeps the earlier note; once it was brought back, closing everything removes it");

        // And a run that opens something else replaces it.
        first.note (one);
        Resume::Keeper third (data);
        auto other = one;
        other.open.files[0].file = b;
        third.note (other);
        const auto reread = Resume::load (data);
        check (! third.keepsEarlier() && reread.state.open.files.size() == 1 && reread.state.open.files[0].file == b,
               "left off: opening something else replaces what was kept");

        // The mark of a restore under way.
        const auto clean = ! Resume::wasInterrupted (data);
        Resume::markStarted (data);
        const auto marked = Resume::wasInterrupted (data);
        Resume::markFinished (data);
        check (clean && marked && ! Resume::wasInterrupted (data) && Resume::isKept (data),
               "left off: a restore that didn't finish can be told at the next start, and the note is still there to try again");
    }

    juce::PluginDescription builtInInstrument (const juce::String& name)
    {
        for (auto& d : BuiltInEffects::getDescriptions())
            if (d.isInstrument && d.name == name)
                return d;

        return {};
    }

    float value01 (const juce::AudioProcessorParameter& p)   { return p.getValue(); }

    /** Lets what is waiting for the message thread run (an edit told of, a plug-in made). */
    void pump()
    {
        runUntil ([] { return false; }, 120);
    }

    /** Adds an instrument track playing a built-in instrument, as Add Instrument Track does, and waits for it. */
    juce::uint32 addInstrumentTrack (AudioEngine& engine, const juce::String& instrument)
    {
        auto next = *engine.session.getState();
        auto& track = next.addInstrumentTrack();
        track.name = instrument;
        const auto id = track.id;
        engine.session.apply (std::move (next), "Add Instrument Track");
        auto done = false;
        engine.addTrackPlugin (id, builtInInstrument (instrument), [&] (const juce::String&) { done = true; });
        runUntil ([&] { return done; }, 10000);
        return id;
    }

    /** Opens a session file as the main window does: the state, then its tracks' effects. */
    bool openSession (AudioEngine& engine, const juce::File& file, const juce::File& as, bool unsaved)
    {
        auto loaded = SessionFile::load (file, engine.formatManager);

        if (loaded.error.isNotEmpty())
            return false;

        engine.clearTrackEffects();
        engine.session.reset (std::move (loaded.state), as);
        engine.session.ensureMaster();

        if (unsaved)
            engine.session.markUnsaved();

        auto done = false;
        engine.loadTrackEffects (std::move (loaded.effects), [&] (const juce::String&) { done = true; });
        const auto loading = engine.isLoadingTrackEffects();
        runUntil ([&] { return done; }, 10000);
        pump();
        return done && loading && ! engine.isLoadingTrackEffects();
    }

    Resume::SessionWriter writerFor (AudioEngine& engine)
    {
        return [&engine] (const juce::File& target)
        {
            return SessionFile::save (*engine.session.getState(), target, [&engine] (juce::uint32 id) { return engine.getTrackEffects().createXml (id); });
        };
    }

    /** Whether a session counts as changed (so quitting asks, and its changes are kept) after
        each thing done to an instrument on one of its tracks. */
    void testInstrumentEdits (const juce::File& work)
    {
        const auto user = work.getChildFile ("User 5");
        user.createDirectory();
        const auto sessionFile = user.getChildFile ("Beat.tqsession"), hit = user.getChildFile ("Hit.wav");
        AudioEngine engine;
        AudioFileIO::save (*tone (4800, 220.0, 0.5f), 48000.0, hit, 24, engine.formatManager);
        const auto save = [&]
        {
            const auto error = writerFor (engine) (sessionFile);
            engine.session.markSaved (sessionFile);
            pump();
            return error.isEmpty() && ! engine.session.isDirty();
        };

        engine.session.reset (SessionState::newEmpty (48000.0), {});
        engine.session.ensureMaster();
        const auto clean = ! engine.session.isDirty();
        const auto drumsId = addInstrumentTrack (engine, "Basic Drums");
        auto* drums = dynamic_cast<BuiltInEffects::BasicDrumsProcessor*> (engine.getTrackEffects().getInstrument (drumsId));
        check (clean && drums != nullptr && engine.session.isDirty() && engine.session.getFile() == juce::File(),
               "unsaved session: a new session is not changed; with only an instrument track added, never saved, it is (quitting asks)");

        if (drums == nullptr)
            return;

        // Saved. Then a knob turned in the instrument's window, as a slider does it.
        const auto saved = save();
        auto* tune = drums->parameterOf (DrumMachine::Sound::kick, "tune");
        const auto edits = engine.session.getEffectsEdits();
        tune->beginChangeGesture();
        tune->setValueNotifyingHost (value01 (*tune) > 0.5f ? 0.2f : 0.8f);
        tune->endChangeGesture();
        pump();
        check (saved && engine.session.isDirty() && engine.session.getEffectsEdits() != edits,
               "unsaved session: a knob turned in an instrument's window marks a saved session changed");

        // A kit chosen: values set with no gesture.
        const auto savedAgain = save();
        drums->applyKit (DrumMachine::kits()[2]);
        pump();
        check (savedAgain && engine.session.isDirty(), "unsaved session: a kit chosen in Basic Drums marks a saved session changed");

        // A value set from another thread (a MIDI controller reaches the instrument on the audio thread) is no edit.
        const auto savedThird = save();
        std::thread ([&] { tune->setValueNotifyingHost (0.5f); }).join();
        pump();
        check (savedThird && ! engine.session.isDirty(), "unsaved session: a value an instrument is given off the message thread is not a change to save");

        // A sample loaded into the Sampler: no parameter moves, its state does.
        const auto samplerId = addInstrumentTrack (engine, "Torqwave Sampler");
        auto* sampler = dynamic_cast<BuiltInEffects::SamplerProcessor*> (engine.getTrackEffects().getInstrument (samplerId));
        const auto savedFourth = sampler != nullptr && save();
        juce::String error;
        const auto sample = Sampler::sharedSample (hit, error);
        const auto added = sampler != nullptr && sample != nullptr && sampler->addSample (sample, hit).isEmpty();
        pump();
        check (savedFourth && added && engine.session.isDirty(), "unsaved session: a sample loaded into the Sampler marks a saved session changed");

        // Opening the saved session gives every instrument its state back, and that is not a change.
        const auto savedLast = save();
        const auto opened = openSession (engine, sessionFile, sessionFile, false);
        const auto state = engine.session.getState();
        auto* drumsBack = state->tracks.size() >= 2 ? dynamic_cast<BuiltInEffects::BasicDrumsProcessor*> (engine.getTrackEffects().getInstrument (state->tracks[0].id)) : nullptr;
        auto* samplerBack = state->tracks.size() >= 2 ? dynamic_cast<BuiltInEffects::SamplerProcessor*> (engine.getTrackEffects().getInstrument (state->tracks[1].id)) : nullptr;
        check (savedLast && opened && drumsBack != nullptr && samplerBack != nullptr && samplerBack->getZones().size() == 1
                   && drumsBack->getKitName() == DrumMachine::kits()[2].name && ! engine.session.isDirty(),
               "unsaved session: opening a session, its instruments given their saved state, leaves it not changed");
        engine.clearTrackEffects();
    }

    /** Quits with a session never saved (an audio clip, an instrument with its settings and notes,
        a tempo), starts again and has it back, still to be saved; and the same for a saved
        session's unsaved changes. */
    void testUnsavedSession (const juce::File& work)
    {
        const auto user = work.getChildFile ("User 6"), data = work.getChildFile ("Data 6/Left Off");
        user.createDirectory();
        const auto a = user.getChildFile ("Loop.wav"), sessionFile = user.getChildFile ("Song.tqsession");
        const auto note = data.getChildFile ("Left Off.xml"), copy = Resume::sessionCopy (data);
        const SessionPlace place { 4800, { 0, 9600 }, 0.0, 48.0, 0, 1 };
        NoteClip notes;
        notes.name = "Pattern";
        notes.position = 24000;
        notes.length = 96000;
        notes.notes = { { 36, 0, 6000, 0.9f, 1.0f }, { 38, 24000, 6000, 0.7f, 1.0f }, { 42, 36000, 3000, 0.5f, 0.5f } };
        float tuneLeft = 0.0f;
        juce::String userBefore;

        {
            AudioEngine engine;
            OpenFiles files (engine);
            AudioFileIO::save (*tone (48000, 110.0, 0.4f), 48000.0, a, 24, engine.formatManager);
            auto session = SessionState::newEmpty (48000.0);
            session.tempo = 97.0;
            SessionEdits::addClip (session, 0, a, AudioFileIO::load (a, engine.formatManager).snapshot, 12000);
            engine.session.reset (std::move (session), {});
            engine.session.ensureMaster();

            // Clean and never saved, there is nothing of it to keep.
            const auto nothing = Resume::gather (files, {}, engine, true, place, {});
            const auto drumsId = addInstrumentTrack (engine, "Basic Drums");
            auto* drums = dynamic_cast<BuiltInEffects::BasicDrumsProcessor*> (engine.getTrackEffects().getInstrument (drumsId));

            if (drums == nullptr)
            {
                check (false, "unsaved session: Basic Drums loads onto a track");
                return;
            }

            drums->applyKit (DrumMachine::kits()[3]);
            auto* tune = drums->parameterOf (DrumMachine::Sound::snare, "tune");
            tune->setValueNotifyingHost (value01 (*tune) > 0.5f ? 0.25f : 0.75f);
            tuneLeft = value01 (*tune);
            auto next = *engine.session.getState();
            SessionEdits::addNoteClip (next, drumsId, notes);
            engine.session.apply (std::move (next), "Add Notes");
            pump();
            userBefore = listing (user);

            // Quit: the note, and the session beside it.
            Resume::Keeper keeper (data);
            const auto state = Resume::gather (files, {}, engine, true, place, {});
            const auto error = keeper.note (state, writerFor (engine), true);
            check (nothing.isEmpty() && ! nothing.sessionCopy && state.sessionCopy && ! state.isEmpty() && state.open.session == juce::File()
                       && error.isEmpty() && note.existsAsFile() && copy.existsAsFile(),
                   "unsaved session: a session never saved, with work in it, is kept beside the note at quit");
            check (data.findChildFiles (juce::File::findFiles, true).size() == 2 && copy.getSize() < 200000 && listing (user) == userBefore,
                   "unsaved session: what is kept is the note and the session's file alone (" + juce::String (copy.getSize())
                       + " bytes: no audio), and nothing is written beside the user's files");

            // Not written again while nothing has changed; again when something has.
            copy.deleteFile();
            const auto notAgain = keeper.note (state, writerFor (engine), false).isEmpty() && ! copy.existsAsFile();
            const auto again = keeper.note (state, writerFor (engine), true).isEmpty() && copy.existsAsFile()
                            && ! data.getChildFile ("Session Being Written.tqsession").exists();
            check (notAgain && again, "unsaved session: the session is written again only when it is said to have changed");
            engine.clearTrackEffects();
        }

        // The next start.
        {
            AudioEngine engine;
            auto loaded = Resume::load (data);
            const auto found = loaded.found && loaded.state.sessionCopy && loaded.state.open.session == juce::File() && loaded.missing.isEmpty()
                            && loaded.state.waiting.isEmpty() && loaded.state.open.multitrack && samePlace (loaded.state.sessionPlace, place);
            const auto opened = found && openSession (engine, copy, loaded.state.open.session, true);
            const auto state = engine.session.getState();
            const auto shape = state->tracks.size() == 3 && state->tracks[0].clips.size() == 1 && state->tracks[0].clips[0].source == a
                            && state->tracks[0].clips[0].position == 12000 && state->tracks[1].isInstrument && state->tracks[1].noteClips.size() == 1
                            && state->tracks[2].isMaster && juce::exactlyEqual (state->tempo, 97.0);
            check (opened && shape && engine.session.isDirty() && engine.session.getFile() == juce::File(),
                   "unsaved session: at the next start it is back (its audio clip, its instrument track, its tempo), with no file and still to be saved");

            if (! (opened && shape))
                return;

            const auto& back = state->tracks[1].noteClips[0];
            auto* drums = dynamic_cast<BuiltInEffects::BasicDrumsProcessor*> (engine.getTrackEffects().getInstrument (state->tracks[1].id));
            check (back.name == notes.name && back.position == notes.position && back.length == notes.length && back.notes == notes.notes
                       && drums != nullptr && drums->getKitName() == DrumMachine::kits()[3].name
                       && juce::exactlyEqual (value01 (*drums->parameterOf (DrumMachine::Sound::snare, "tune")), tuneLeft),
                   "unsaved session: its notes are the same, and its instrument has the kit and the knob as they were left");

            // Saved now: the next note names the file, and the copy goes.
            OpenFiles files (engine);
            Resume::Keeper keeper (data);
            keeper.restored();
            const auto savedTo = writerFor (engine) (sessionFile).isEmpty();
            engine.session.markSaved (sessionFile);
            const auto saved = Resume::gather (files, {}, engine, true, place, {});
            check (savedTo && ! saved.sessionCopy && saved.open.session == sessionFile && keeper.note (saved, writerFor (engine), true).isEmpty()
                       && ! copy.exists() && Resume::load (data).state.open.session == sessionFile && ! Resume::load (data).state.sessionCopy,
                   "unsaved session: once the session is saved the note names its file and the copy is removed");

            // Changed again and not saved: the copy is the session, whether or not its file is still there.
            auto next = *engine.session.getState();
            next.tempo = 141.0;
            engine.session.apply (std::move (next), "Tempo");
            const auto changed = Resume::gather (files, {}, engine, true, place, {});
            const auto kept = changed.sessionCopy && changed.open.session == sessionFile && keeper.note (changed, writerFor (engine), true).isEmpty();
            const auto onDisk = SessionFile::load (sessionFile, engine.formatManager);
            sessionFile.moveFileTo (user.getChildFile ("Moved.tqsession"));
            const auto away = Resume::load (data);
            const auto inCopy = SessionFile::load (copy, engine.formatManager);
            check (kept && juce::exactlyEqual (onDisk.state.tempo, 97.0) && away.state.sessionCopy && away.state.open.session == sessionFile
                       && away.missing.isEmpty() && away.state.waiting.isEmpty() && juce::exactlyEqual (inCopy.state.tempo, 141.0),
                   "unsaved session: a saved session's unsaved changes are kept the same way, its file left as saved, and come back under its name even with the file gone");

            // The copy gone: said; the session's own file is opened when it is there, and waits when it is not.
            copy.deleteFile();
            const auto bothGone = Resume::load (data);
            user.getChildFile ("Moved.tqsession").moveFileTo (sessionFile);
            const auto fileOnly = Resume::load (data);
            check (! bothGone.state.sessionCopy && bothGone.state.open.session == juce::File() && bothGone.state.waiting.session == sessionFile
                       && bothGone.missing.size() == 1 && ! fileOnly.state.sessionCopy && fileOnly.state.open.session == sessionFile
                       && fileOnly.state.waiting.isEmpty() && fileOnly.missing.size() == 1,
                   "unsaved session: with the copy gone that is said, and the session's own file is opened as saved, or waits if it is gone too");

            // A session that was waiting gives way to one with unsaved work, and not to a new empty one.
            Resume::Waiting waiting;
            waiting.session = user.getChildFile ("Away.tqsession");
            const auto givesWay = Resume::gather (files, {}, engine, true, place, {}, waiting).waiting.session == juce::File();
            engine.clearTrackEffects();
            engine.session.reset (SessionState::newEmpty (48000.0), {});
            engine.session.ensureMaster();
            const auto empty = Resume::gather (files, {}, engine, true, place, {}, waiting);
            check (givesWay && ! empty.sessionCopy && empty.waiting.session == waiting.session,
                   "unsaved session: a session waiting to be found again gives way to unsaved work, and stays for a new empty session");

            // Everything closed after that: nothing to come back to, the copy included.
            keeper.note (changed, writerFor (engine), true);
            const auto there = copy.existsAsFile();
            keeper.note (Resume::gather (files, {}, engine, false, {}, {}), writerFor (engine), true);
            check (there && ! copy.exists() && ! note.exists(), "unsaved session: with nothing left open the note and the copy are both removed");
        }
    }

    /** The files are read once each, in order, by the loader given, and reading can be stopped. */
    void testLoading (const juce::File& work)
    {
        const auto user = work.getChildFile ("User 4"), data = work.getChildFile ("Data 4/Left Off");
        user.createDirectory();
        juce::Array<juce::File> made;
        Resume::State state;
        AudioEngine engine;

        for (int i = 0; i < 5; ++i)
        {
            made.add (user.getChildFile ("File " + juce::String (i + 1) + ".wav"));
            AudioFileIO::save (*tone (2400 * (i + 1), 440.0, 0.25f), 48000.0, made.getLast(), 16, engine.formatManager);
            state.open.files.push_back ({ made.getLast(), {}, {}, nullptr });
        }

        state.open.active = 3;
        Resume::write (state, data);
        juce::StringArray asked;
        const auto loader = [&asked] (const juce::File& f) { asked.add (f.getFileName()); return BackgroundLoad::readIfThere (f); };
        auto all = Resume::load (data, loader);
        OpenFiles files (engine);
        Resume::restoreFiles (all, files, engine);
        check (asked.joinIntoString (",") == "File 1.wav,File 2.wav,File 3.wav,File 4.wav,File 5.wav" && files.size() == 5
                   && files.getActiveIndex() == 3 && engine.document.getNumFrames() == 2400 * 4 && all.missing.isEmpty(),
               "left off: each file is read once, in order, off the thread that opens them, and the one that was shown is in front");

        // Stopped after two: those two open, the rest are said to be left out.
        asked.clear();
        auto count = 0;
        auto some = Resume::load (data, loader, [&count] { return ++count <= 2; });
        OpenFiles fewer (engine);
        Resume::restoreFiles (some, fewer, engine);
        check (asked.size() == 2 && fewer.size() == 2 && some.missing.size() == 3 && some.state.waiting.files.size() == 3,
               "left off: reading that is stopped opens what was read, and names the rest and keeps them waiting");
    }
}

void runResumeTests()
{
    const auto work = juce::File::getSpecialLocation (juce::File::tempDirectory)
                          .getChildFile ("torqwave-resume-test-" + juce::String::toHexString (juce::Random::getSystemRandom().nextInt64()));
    work.deleteRecursively();
    work.createDirectory();

    testSetting();
    testQuitAndStart (work);
    testMissingAndChanged (work);
    testKeeping (work);
    testLoading (work);
    testInstrumentEdits (work);
    testUnsavedSession (work);

    work.deleteRecursively();
}

bool makeResumeDemo (const juce::File& folder, bool unsavedSession)
{
    const auto user = folder.getChildFile ("Files");
    user.createDirectory();
    const auto a = user.getChildFile ("Take A.wav"), b = user.getChildFile ("Take B.wav"), c = user.getChildFile ("Take C.flac"),
               sessionFile = user.getChildFile ("Mix.tqsession");
    AudioEngine engine;
    OpenFiles files (engine);
    AudioFileIO::save (*tone (480000, 440.0, 0.25f), 48000.0, a, 24, engine.formatManager);
    AudioFileIO::save (*tone (240000, 660.0, 0.5f), 48000.0, b, 16, engine.formatManager);
    AudioFileIO::save (*tone (360000, 880.0, 0.4f), 48000.0, c, 24, engine.formatManager);

    SessionState session;
    session.sampleRate = 48000.0;
    SessionEdits::addClip (session, 0, a, AudioFileIO::load (a, engine.formatManager).snapshot, 48000);
    SessionEdits::addClip (session, 1, b, AudioFileIO::load (b, engine.formatManager).snapshot, 480000);
    SessionEdits::addClip (session, 2, c, AudioFileIO::load (c, engine.formatManager).snapshot, 240000);
    const auto error = unsavedSession ? juce::String() : SessionFile::save (session, sessionFile);
    engine.session.reset (std::move (session), unsavedSession ? juce::File() : sessionFile);

    if (unsavedSession)
    {
        // Never saved: a drum track with a kit chosen and a bar of notes, at 97 beats a minute.
        engine.session.ensureMaster();
        const auto drumsId = addInstrumentTrack (engine, "Basic Drums");

        if (auto* drums = dynamic_cast<BuiltInEffects::BasicDrumsProcessor*> (engine.getTrackEffects().getInstrument (drumsId)))
            drums->applyKit (DrumMachine::kits()[3]);

        NoteClip notes;
        notes.name = "Pattern";
        notes.position = 48000;
        notes.length = 192000;
        notes.notes = { { 36, 0, 6000, 0.9f, 1.0f }, { 38, 48000, 6000, 0.7f, 1.0f }, { 42, 72000, 3000, 0.5f, 1.0f }, { 36, 96000, 6000, 0.9f, 1.0f } };
        auto next = *engine.session.getState();
        next.tempo = 97.0;
        SessionEdits::addNoteClip (next, drumsId, notes);
        engine.session.apply (std::move (next), "Add Notes");
    }

    engine.adoptFile (a, AudioFileIO::load (a, engine.formatManager));
    files.prepareForAnother ({ { 48000, 96000 }, 48000, 24000.0, 120.0, {}, {} });
    engine.adoptFile (b, AudioFileIO::load (b, engine.formatManager));
    files.prepareForAnother ({ { 100000, 130000 }, 130000, 90000.0, 60.0, {}, {} });
    engine.adoptFile (c, AudioFileIO::load (c, engine.formatManager));
    files.activate (1, { { 200000, 200000 }, 200000, 150000.0, 150.0, {}, {} });

    const auto state = Resume::gather (files, { { 100000, 130000 }, 130000, 90000.0, 60.0, {}, {} }, engine, true,
                                       { 300000, { 240000, 360000 }, 200000.0, 400.0, 0, 1 }, {});

    if (! unsavedSession)
        return error.isEmpty() && Resume::write (state, folder.getChildFile ("Left Off")).isEmpty();

    Resume::Keeper keeper (folder.getChildFile ("Left Off"));
    const auto kept = state.sessionCopy && keeper.note (state, writerFor (engine), true).isEmpty();
    engine.clearTrackEffects();
    return kept;
}
