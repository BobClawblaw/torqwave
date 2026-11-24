#include <JuceHeader.h>
#include "../Source/AudioEdits.h"
#include "../Source/AudioFileIO.h"
#include "../Source/Home.h"
#include "../Source/MultitrackView.h"
#include "../Source/Resume.h"
#include "TestUtils.h"
#include "ResumeTests.h"

namespace
{
    using TestUtils::check;

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
        check (asked.size() == 2 && fewer.size() == 2 && some.missing.size() == 3,
               "left off: reading that is stopped opens what was read and names the rest");
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

    work.deleteRecursively();
}
