#include <JuceHeader.h>
#include "../Source/SequenceLibraryView.h"
#include "../Source/SequencerView.h"
#include "../Source/AudioEngine.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/TrackEffects.h"
#include "TestUtils.h"
#include "SequenceTests.h"

namespace
{
    using TestUtils::check;

    constexpr double rate = 48000.0;

    SessionState sessionAt (double tempo, std::vector<Metronome::TempoChange> changes = {})
    {
        SessionState s;
        s.sampleRate = rate;
        s.tempo = tempo;
        s.beatsPerBar = 4;
        s.tempoChanges = std::move (changes);
        return s;
    }

    /** A two-bar pattern at bar 2 (beat 4) of a session: notes on beats and sixteenths, one held
        over a beat, a mod wheel lane, a loop of a bar, and the notes' timeline starting half a
        beat before the clip. */
    juce::uint32 addPattern (SessionState& s, juce::uint32 trackId)
    {
        const auto map = s.tempoMap();
        const auto position = map.beatFrame (4);
        const auto origin = map.frameAtBeatPosition (3.5);
        const auto at = [&] (double beatsFromOrigin) { return map.frameAtBeatPosition (3.5 + beatsFromOrigin) - origin; };

        NoteClip c;
        c.name = "Groove";
        c.position = position;
        c.offset = position - origin;
        c.length = map.beatFrame (12) - position;
        c.loopLength = at (4.0);
        c.channel = 3;

        for (int i = 0; i < 8; ++i)
        {
            const auto start = 0.5 + 0.25 * i;
            c.notes.push_back ({ 36 + i, at (start), at (start + 0.25) - at (start), 0.5f + 0.05f * (float) i });
        }

        c.notes.push_back ({ 60, at (2.5), at (4.0) - at (2.5), 1.0f });

        ControllerLane mod;
        mod.controller = 1;
        mod.envelope.points = { { at (0.5), 0.0f }, { at (2.5), 1.0f } };
        c.lanes.push_back (mod);
        return SessionEdits::addNoteClip (s, trackId, c);
    }

    /** How far, in frames, the clip's notes are from the beats the sequence gives them, the worst of them. */
    juce::int64 worstOffBeat (const SessionState& s, const NoteClip& c, const SequenceFile::Sequence& q)
    {
        const auto map = s.tempoMap();
        const auto originBeat = map.beatPosition (c.position) - (double) q.offset / SequenceFile::ticksPerBeat;
        juce::int64 worst = c.notes.size() == q.notes.size() ? 0 : 1000000;

        for (size_t i = 0; i < c.notes.size() && i < q.notes.size(); ++i)
        {
            const auto origin = c.getOrigin();
            const auto start = map.frameAtBeatPosition (originBeat + (double) q.notes[i].start / SequenceFile::ticksPerBeat);
            const auto end = map.frameAtBeatPosition (originBeat + (double) (q.notes[i].start + q.notes[i].length) / SequenceFile::ticksPerBeat);
            worst = juce::jmax (worst, std::abs (origin + c.notes[i].start - start), std::abs (origin + c.notes[i].start + c.notes[i].length - end));
        }

        return worst;
    }

    const NoteClip& noteClip (const SessionState& s, juce::uint32 id)
    {
        const auto w = s.findNoteClip (id);
        return s.tracks[(size_t) w->first].noteClips[(size_t) w->second];
    }

    std::unique_ptr<juce::XmlElement> fakeEffects()
    {
        auto effects = std::make_unique<juce::XmlElement> ("EFFECTS");
        auto* slot = effects->createNewChildElement ("SLOT");
        slot->setAttribute ("id", 1);
        auto* plugin = slot->createNewChildElement ("PLUGIN");
        plugin->setAttribute ("name", "Test Synth");
        plugin->setAttribute ("isInstrument", true);
        return effects;
    }
}

void runSequenceTests (AudioEngine& engine)
{
    // In one session and back: every note, lane, the loop and the offset come back exactly.
    {
        auto s = sessionAt (120.0);
        const auto trackId = s.addInstrumentTrack().id;
        const auto id = addPattern (s, trackId);
        const auto& clip = noteClip (s, id);
        const auto q = SequenceFile::fromClip (s, id, nullptr);
        const auto xml = q.has_value() ? SequenceFile::toXml (*q) : nullptr;
        const auto back = xml != nullptr ? SequenceFile::fromXml (*xml) : std::nullopt;
        auto again = back.has_value() ? SequenceFile::toClip (*back, s, clip.position) : NoteClip {};

        check (q.has_value() && back.has_value() && back->notes == q->notes && back->lanes == q->lanes && back->offset == 480 && back->length == 8 * 960
                   && back->loopLength == 4 * 960 && back->notes[0].start == 480 && back->notes[0].length == 240 && back->channel == 3 && back->name == "Groove"
                   && juce::exactlyEqual (back->writtenTempo, 120.0) && juce::exactlyEqual (back->bars(), 2.0),
               "sequence: a note clip is read in beats (its timeline half a beat before it, 480 ticks; two bars, 7680 ticks; a loop of a bar) and comes back from its XML the same");
        check (again.notes == clip.notes && again.lanes == clip.lanes && again.length == clip.length && again.loopLength == clip.loopLength
                   && again.offset == clip.offset && again.position == clip.position,
               "sequence: put back where it came from, it is the clip it was, frame for frame");
    }

    // Written at 90 BPM, dropped into a session at 140 and one that changes tempo inside it: the same beats.
    {
        auto slow = sessionAt (90.0);
        const auto trackId = slow.addInstrumentTrack().id;
        const auto q = SequenceFile::fromClip (slow, addPattern (slow, trackId), nullptr);

        auto fast = sessionAt (140.0);
        const auto placed = SequenceFile::place (fast, *q, -1, fast.tempoMap().beatFrame (8), false);
        const auto& atFast = noteClip (fast, placed.clipId);
        const auto fastLength = fast.tempoMap().beatPosition (atFast.getEnd()) - fast.tempoMap().beatPosition (atFast.position);

        auto changing = sessionAt (120.0, { { 3, 90.0, 4 } });   // 90 from the third bar, inside the clip
        const auto placed2 = SequenceFile::place (changing, *q, -1, changing.tempoMap().beatFrame (6), false);
        const auto& atChanging = noteClip (changing, placed2.clipId);
        const auto changingLength = changing.tempoMap().beatPosition (atChanging.getEnd()) - changing.tempoMap().beatPosition (atChanging.position);

        check (worstOffBeat (fast, atFast, *q) <= 1 && std::abs (fastLength - 8.0) < 1.0e-6 && atFast.position == fast.tempoMap().beatFrame (8),
               "sequence: written at 90 BPM and dropped at bar 3 of a 140 BPM session, every note starts and ends on its beat (within a frame), and it is 8 beats long");
        check (worstOffBeat (changing, atChanging, *q) <= 1 && std::abs (changingLength - 8.0) < 1.0e-6,
               "sequence: dropped across a tempo change (120, then 90 from bar 3), every note is still on its beat, and it is 8 beats long");
    }

    // The voice: the track's instrument and effects, without its sends, output or automation.
    {
        auto s = sessionAt (120.0);
        auto& bus = s.addBus();
        bus.name = "Verb";
        const auto busId = bus.id;
        auto& t = s.addInstrumentTrack();
        t.name = "Pad Track";
        t.gainDb = -4.0f;
        t.sends.push_back ({ busId, -6.0f, false });
        t.outputBusId = busId;
        t.volumeEnvelope.points = { { 0, -10.0f }, { 48000, 0.0f } };
        const auto trackId = t.id;
        const auto q = SequenceFile::fromClip (s, addPattern (s, trackId), fakeEffects());
        const auto* voice = q.has_value() ? q->voice.get() : nullptr;

        check (voice != nullptr && voice->getChildByName ("EFFECTS") != nullptr && voice->getChildByName ("SEND") == nullptr
                   && ! voice->hasAttribute ("output") && voice->getNumChildElements() == 1 && voice->getBoolAttribute ("instrument")
                   && juce::exactlyEqual ((float) voice->getDoubleAttribute ("gainDb"), -4.0f),
               "sequence: its voice keeps the track's instrument and effects and its volume, and leaves out its sends, output and automation");

        // Into another session: a new instrument track from the voice, named for the sequence, its effects to load.
        auto other = sessionAt (100.0);
        other.addTrack();
        const auto placed = SequenceFile::place (other, *q, 0, 0, false);
        const auto* made = other.findTrack (placed.trackId);
        check (placed.clipId != 0 && made != nullptr && made->isInstrument && made->name == "Groove" && made->sends.empty() && other.tracks.size() == 2
                   && placed.effects != nullptr && placed.effects->getChildByName ("SLOT") != nullptr && ! made->isAutomated(),
               "sequence: dropped on empty space it makes an instrument track from its voice, named for it, with its effects to load and no sends or automation");

        // On an instrument track: the notes only, played by that track's instrument.
        auto withInstrument = sessionAt (100.0);
        const auto keysId = withInstrument.addInstrumentTrack().id;
        const auto onTrack = SequenceFile::place (withInstrument, *q, 0, 4800, true);
        check (onTrack.trackId == keysId && onTrack.effects == nullptr && withInstrument.tracks.size() == 1 && withInstrument.tracks[0].noteClips.size() == 1
                   && noteClip (withInstrument, onTrack.clipId).position == 4800,
               "sequence: dropped on an instrument track it adds its notes there and keeps the track's instrument");

        // On an audio track it can't go there: a new track instead.
        auto withAudio = sessionAt (100.0);
        withAudio.addTrack();
        const auto onAudio = SequenceFile::place (withAudio, *q, 0, 0, true);
        check (withAudio.tracks.size() == 2 && onAudio.trackId == withAudio.tracks[1].id && onAudio.clipId != 0,
               "sequence: dropped on an audio track it goes on a new instrument track");
    }

    // New Sequence: an instrument track with an empty clip of whole bars, from the cursor's bar.
    {
        auto s = sessionAt (120.0);
        s.addTrack();
        const auto map = s.tempoMap();
        const auto placed = SequenceFile::blank (s, "Idea", 4, map.barFrame (2) + 1000);
        const auto* track = s.findTrack (placed.trackId);
        const auto& clip = noteClip (s, placed.clipId);
        check (track != nullptr && track->isInstrument && track->name == "Idea" && s.tracks.size() == 2 && s.tracks[1].id == placed.trackId
                   && clip.name == "Idea" && clip.notes.empty() && clip.position == map.barFrame (2) && clip.length == map.barFrame (6) - map.barFrame (2)
                   && placed.effects == nullptr,
               "sequence: New Sequence makes an instrument track with an empty clip four bars long from the start of the cursor's bar");

        auto changing = sessionAt (120.0, { { 1, 60.0, 4 } });   // 60 from the second bar (bars count from 0)
        const auto two = SequenceFile::blank (changing, "Slow", 2, 0);
        const auto& both = noteClip (changing, two.clipId);
        check (both.position == 0 && both.length == Metronome::barFrames (120.0, 4, changing.sampleRate) + Metronome::barFrames (60.0, 4, changing.sampleRate),
               "sequence: its length follows the tempo map (a bar at 120 BPM, then one at 60)");
    }

    // Read at another resolution, ticks come to ours.
    {
        juce::XmlElement xml ("TORQWAVE_SEQUENCE");
        xml.setAttribute ("ticksPerBeat", 480);
        xml.setAttribute ("length", "1920");
        auto* n = xml.createNewChildElement ("NOTES")->createNewChildElement ("N");
        n->setAttribute ("p", 64);
        n->setAttribute ("s", "240");
        n->setAttribute ("l", "120");
        const auto q = SequenceFile::fromXml (xml);
        const auto notOne = SequenceFile::fromXml (juce::XmlElement ("TORQWAVE_SESSION"));
        check (q.has_value() && q->length == 3840 && q->notes.size() == 1 && q->notes[0].start == 480 && q->notes[0].length == 240 && ! notOne.has_value(),
               "sequence: one written at 480 ticks a beat is read at 960 (half a beat stays half a beat); other XML is not a sequence");
    }

    // The library: its roles' folders, saving, the scan and the search.
    {
        const auto folder = juce::File::createTempFile ("sequences");
        SequenceLibrary library (folder);
        const auto made = library.makeFolders();
        const auto roles = SequenceLibrary::roles();
        auto allThere = made;

        for (auto& r : roles)
            for (auto& sub : r.subRoles)
                allThere = allThere && folder.getChildFile (r.name).getChildFile (sub).isDirectory();

        auto s = sessionAt (120.0);
        const auto trackId = s.addInstrumentTrack().id;
        auto q = *SequenceFile::fromClip (s, addPattern (s, trackId), nullptr);
        q.key = "A minor";
        q.tags = { "four on the floor", "house" };
        const auto first = library.newFileFor ("Drums", "Hats & Tops", "Groove");
        const auto saved = SequenceFile::save (q, first);
        const auto second = library.newFileFor ("Drums", "Hats & Tops", "Groove");
        SequenceFile::save (q, second);
        const auto mine = folder.getChildFile ("Drums").getChildFile ("Hats & Tops").getChildFile ("My Hats");
        SequenceFile::save (q, mine.getChildFile ("Groove" + juce::String (SequenceFile::extension)));
        folder.getChildFile ("Inbox").getChildFile (juce::String ("broken") + SequenceFile::extension).replaceWithText ("not a sequence");
        const auto unknown = library.folderFor ("Polka", {});

        const auto entries = library.scan();
        juce::String error;
        const auto loaded = SequenceFile::load (first, error);

        check (allThere && saved.isEmpty() && first.getFileName() == "Groove.tqseq" && second.getFileName() == "Groove 2.tqseq"
                   && unknown == folder.getChildFile ("Inbox"),
               "sequence library: makes a folder for every role and sub-role (" + juce::String ((int) roles.size()) + " roles), saves a sequence where its role says, "
               "names a second one Groove 2, and files an unknown role in the Inbox");
        check (entries.size() == 3 && entries[0].path == juce::StringArray { "Drums", "Hats & Tops" } && entries[2].path == juce::StringArray { "Drums", "Hats & Tops", "My Hats" }
                   && entries[0].key == "A minor" && juce::exactlyEqual (entries[0].bars, 2.0) && ! entries[0].hasInstrument,
               "sequence library: the scan finds the three sequences with their folders (the user's own below the sub-role), key and bars, sees that their voice has no instrument, and skips a file that is not one");
        check (loaded.has_value() && loaded->notes == q.notes && loaded->tags == q.tags
                   && SequenceLibrary::matches (entries[0], "house hats") && SequenceLibrary::matches (entries[0], "A MINOR") && ! SequenceLibrary::matches (entries[0], "pads"),
               "sequence library: a saved sequence loads as it was; search matches every word in the name, key, tags and folders, in any case");

        // The panel: every sequence in the tree, a search narrowing it, a drag naming the file.
        {
            SequenceLibraryView view (folder);
            const auto all = view.shownFiles().size();
            view.setSearch ("my hats");
            const auto narrowed = view.shownFiles();
            view.setSearch ({});
            const auto drag = SequenceLibraryView::dragDescriptionFor (first);
            check (all == 3 && narrowed.size() == 1 && narrowed[0] == mine.getChildFile ("Groove" + juce::String (SequenceFile::extension))
                       && view.shownFiles().size() == 3 && SequenceLibraryView::fileOfDrag (drag) == first && SequenceLibraryView::fileOfDrag (juce::var ("files:2")) == juce::File(),
                   "sequence library panel: shows the three sequences, a search for \"my hats\" leaves the one in that folder, cleared all three again; a drag names its file");
        }

        // Moving and copying between folders: the role follows the folder, a clash gets a new name.
        {
            juce::String moveError, copyError, sameError;
            const auto inbox = folder.getChildFile ("Inbox").getChildFile ("Groove" + juce::String (SequenceFile::extension));
            SequenceFile::save (q, inbox);
            const auto moved = library.moveInto (inbox, { "Drums", "Hats & Tops" }, false, moveError);
            const auto movedBack = SequenceFile::load (moved, moveError);
            const auto copied = library.moveInto (moved, { "Pads" }, true, copyError);
            const auto same = library.moveInto (copied, { "Pads" }, false, sameError);
            const auto copyLoaded = SequenceFile::load (copied, copyError);
            check (moveError.isEmpty() && ! inbox.exists() && moved.getFileName() == "Groove 3.tqseq" && moved.getParentDirectory() == folder.getChildFile ("Drums").getChildFile ("Hats & Tops")
                       && movedBack.has_value() && movedBack->role == "Drums" && movedBack->subRole == "Hats & Tops" && movedBack->notes == q.notes
                       && copyError.isEmpty() && moved.existsAsFile() && copied.getParentDirectory() == folder.getChildFile ("Pads")
                       && copyLoaded.has_value() && copyLoaded->role == "Pads" && copyLoaded->subRole.isEmpty()
                       && same == copied && sameError.isEmpty(),
                   "sequence library: a sequence moved to another folder leaves the old one, takes that folder's role and sub-role and a free name (Groove 3), "
                   "and keeps its notes; copied, the original stays; moved where it is, it stays as it is");
            moved.deleteFile();
            copied.deleteFile();
        }

        // Folders: every fixed one shows, empty or not, with the user's own under their parents.
        {
            const auto nightFolder = library.makeFolder ({ "Pads", "Evolving" }, "Night");
            const auto again = library.makeFolder ({ "Pads", "Evolving" }, "Night");
            const auto names = library.folderNames();
            SequenceLibraryView view (folder);
            const auto shown = view.shownFolders();
            int fixedCount = 0;

            for (auto& r : roles)
                fixedCount += 1 + r.subRoles.size();

            check (nightFolder.isEmpty() && again.isNotEmpty() && library.folderAt ({ "Pads", "Evolving", "Night" }).isDirectory()
                       && names[0] == "Inbox" && names.contains ("Drums > Kick") && names.contains ("Drums > Hats & Tops > My Hats")
                       && names.indexOf ("Pads > Evolving > Night") == names.indexOf ("Pads > Evolving") + 1
                       && names.size() == fixedCount + 2,
                   "sequence library: lists every role and sub-role and the user's folders right under their parents; a new folder is made once, then refused");
            check (shown.contains ("Bass > Sub") && shown.contains ("SFX > Risers") && shown.contains ("Pads > Evolving > Night")
                       && shown.contains ("Drums > Hats & Tops > My Hats") && shown.size() == fixedCount + 2,
                   "sequence library panel: shows every sub-role folder, the empty ones too, and the user's own");
            check (SequenceLibrary::isFixed ({ "Drums", "Kick" }) && SequenceLibrary::isFixed ({ "Kits" }) && ! SequenceLibrary::isFixed ({ "Pads", "Evolving", "Night" })
                       && SequenceLibrary::pathOf ("Drums > Hats & Tops > My Hats") == juce::StringArray { "Drums", "Hats & Tops", "My Hats" }
                       && library.folderAt ({ "Polka" }) == folder.getChildFile ("Inbox"),
                   "sequence library: the fixed folders can't be renamed or removed, the user's can; a folder's name gives its path; an unknown role is the Inbox");
        }

        folder.deleteRecursively();
    }

    // Preview: the next bar, a render through the voice at a tempo, and the player.
    {
        const auto changing = sessionAt (120.0, { { 2, 60.0, 3 } });   // 4/4 at 120, then 3/4 at 60
        const auto map = changing.tempoMap();   // it refers to the session's changes, so the session is kept
        juce::int64 threeFour = -1;   // the first bar after the change: 3 beats at 60, 144000 frames

        for (juce::int64 bar = 0; bar < 8 && threeFour < 0; ++bar)
            if (map.barFrame (bar + 1) - map.barFrame (bar) == 144000)
                threeFour = bar;

        check (SequencePreview::framesToNextBar (map, 0) == 0 && SequencePreview::framesToNextBar (map, 1000) == 95000 && threeFour > 0
                   && SequencePreview::framesToNextBar (map, map.barFrame (threeFour) + 10) == 144000 - 10
                   && SequencePreview::framesToNextBar (map, map.barFrame (threeFour)) == 0,
               "sequence preview: from a frame to the next bar's start, 0 on a bar line, through a change to 3/4 at 60 (a bar of 144000 frames)");

        juce::PluginDescription synth;

        for (auto& d : BuiltInEffects::getDescriptions())
            if (d.fileOrIdentifier == BuiltInEffects::basicSynthId)
                synth = d;

        auto s = sessionAt (120.0);
        const auto trackId = s.addInstrumentTrack().id;
        const auto clipId = addPattern (s, trackId);
        TrackEffects effects;
        effects.prepare (rate);
        juce::String error;
        effects.add (trackId, engine.pluginFormatManager.createPluginInstance (synth, rate, 512, error));
        const auto q = SequenceFile::fromClip (s, clipId, effects.createXml (trackId));
        const auto silent = SequenceFile::fromClip (s, clipId, nullptr);

        juce::AudioBuffer<float> at120, at60, none;
        const auto e120 = SequencePreview::render (*q, 120.0, 4, rate, engine.pluginFormatManager, at120);
        const auto e60 = SequencePreview::render (*q, 60.0, 4, rate, engine.pluginFormatManager, at60);
        const auto eNone = SequencePreview::render (*silent, 120.0, 4, rate, engine.pluginFormatManager, none);
        const auto level = [] (const juce::AudioBuffer<float>& b, int from, int n) { return b.getNumSamples() >= from + n ? b.getRMSLevel (0, from, n) : 0.0f; };

        check (SequenceFile::instrumentOf (*q) == synth.name && e120.isEmpty() && at120.getNumChannels() == 2 && at120.getNumSamples() == 2 * 96000
                   && e60.isEmpty() && at60.getNumSamples() == 4 * 96000 && level (at120, 0, 4800) > 0.001f && level (at60, 0, 9600) > 0.001f
                   && eNone.contains ("no instrument") && none.getNumSamples() == 0,
               "sequence preview: two bars through the basic synth render as 192000 frames at 120 BPM and 384000 at 60, sounding from the start; "
               "with no instrument it says so (" + eNone + ")");

        // The player: rendered on its thread, then looped; the same again plays from the cache at once.
        const auto folder = juce::File::createTempFile ("preview");
        const auto file = folder.getChildFile (juce::String ("Groove") + SequenceFile::extension);
        const auto bare = folder.getChildFile (juce::String ("Bare") + SequenceFile::extension);
        SequenceFile::save (*q, file);
        SequenceFile::save (*silent, bare);

        SequencePreview preview (engine);
        preview.play (file);
        const auto rendering = preview.getState() == SequencePreview::State::rendering && preview.describe() == "Rendering Groove...";
        TestUtils::runUntil ([&] { return preview.getState() == SequencePreview::State::playing; }, 20000);
        const auto playing = preview.getState() == SequencePreview::State::playing && preview.describe().startsWith ("Groove at ");

        juce::AudioBuffer<float> pulled (2, 4800);
        preview.getSource().getNextAudioBlock ({ &pulled, 0, pulled.getNumSamples() });
        const auto heard = pulled.getRMSLevel (0, 0, pulled.getNumSamples()) > 0.001f;

        preview.stop();
        preview.getSource().getNextAudioBlock ({ &pulled, 0, pulled.getNumSamples() });
        const auto stopped = juce::exactlyEqual (pulled.getMagnitude (0, pulled.getNumSamples()), 0.0f);

        preview.play (file);
        const auto cached = preview.getState() == SequencePreview::State::playing;

        preview.play (bare);
        TestUtils::runUntil ([&] { return preview.getState() == SequencePreview::State::stopped; }, 20000);
        const auto refused = preview.getState() == SequencePreview::State::stopped && preview.describe().contains ("no instrument");

        check (rendering && playing && heard && stopped && cached && refused,
               "sequence preview: a click renders it on its own thread (\"Rendering Groove...\"), then it plays and is heard; Stop silences it; played again it starts at once "
               "from what was rendered; one with no instrument stops and says why");

        juce::MessageManager::getInstance()->runDispatchLoopUntil (50);   // plug-ins handed to the message thread go
        folder.deleteRecursively();
    }

    // The Sequencer window.
    {
        auto s = sessionAt (120.0);
        const auto trackId = s.addInstrumentTrack().id;
        const auto q = *SequenceFile::fromClip (s, addPattern (s, trackId), fakeEffects());

        // An edit at 128 BPM (23.4375 frames a tick), saved and read back, is exact in ticks.
        SequencerDocument doc;
        doc.open (q, {});
        const auto built = SequencerDocument::sessionFor (q, 128.0, rate);
        auto expected = q;
        expected.notes.push_back ({ 72, 3360, 240, 0.7f });

        auto edited = built.state;
        auto clip = SequenceFile::toClip (expected, edited, 0);
        clip.id = built.clipId;
        SessionEdits::replaceNoteClip (edited, built.clipId, clip);
        doc.apply (SequencerDocument::withClip (doc.get(), edited, built.clipId), "Add Note");

        const auto folder = juce::File::createTempFile ("sequencer");
        const auto file = folder.getChildFile (juce::String ("Groove") + SequenceFile::extension);
        const auto dirty = doc.isDirty();
        const auto saveError = doc.saveTo (file, nullptr);
        juce::String error;
        const auto back = SequenceFile::load (file, error);
        const auto same = [] (const SequenceFile::Sequence& a, const SequenceFile::Sequence& b)
        {
            return SequenceFile::toXml (a)->isEquivalentTo (SequenceFile::toXml (b).get(), false);
        };

        const auto exact = back.has_value() && same (*back, expected) && back->voice != nullptr && back->voice->isEquivalentTo (q.voice.get(), false);
        doc.undo();
        const auto undone = same (doc.get(), q) && doc.isDirty();
        doc.redo();
        const auto redone = same (doc.get(), expected) && ! doc.isDirty();

        check (dirty && saveError.isEmpty() && exact && undone && redone,
               "sequencer: a note added at 128 BPM, saved and opened again, is exactly the sequence with that note in ticks, every other note, lane and its voice as they were; "
               "undo and redo step through the edit, and the saved step is not dirty");

        // Make Variation: a copy beside it that names its original.
        SequencerView view (engine);
        view.openFile (file);
        const auto opened = view.getDocument().getFile() == file && view.getDocument().get().name == "Groove";
        const auto variationError = view.duplicate (true);
        const auto variationFile = folder.getChildFile (juce::String ("Groove Variation") + SequenceFile::extension);
        const auto variation = SequenceFile::load (variationFile, error);
        const auto goesOn = view.getDocument().getFile() == variationFile;
        const auto copyError = view.duplicate (false);
        const auto copy = SequenceFile::load (folder.getChildFile (juce::String ("Groove Variation 2") + SequenceFile::extension), error);

        check (opened && variationError.isEmpty() && variation.has_value() && variation->variationOf == "Groove" && variation->name == "Groove Variation"
                   && variation->notes == expected.notes && variation->voice != nullptr && goesOn
                   && copyError.isEmpty() && copy.has_value() && copy->variationOf == "Groove" && copy->name == "Groove Variation",
               "sequencer: Make Variation saves \"Groove Variation\" beside it, naming Groove as its original, with its notes and voice, and goes on with it; "
               "Duplicate of that saves \"Groove Variation 2\", still a variation of Groove");

        // The window's tempo: a note on beat 2 sounds on beat 2 at 90 and at 140 BPM, the click on each beat.
        juce::PluginDescription synth;

        for (auto& d : BuiltInEffects::getDescriptions())
            if (d.fileOrIdentifier == BuiltInEffects::basicSynthId)
                synth = d;

        SequenceFile::Sequence beat;
        beat.name = "Beat";
        beat.length = 4 * SequenceFile::ticksPerBeat;
        beat.notes.push_back ({ 60, 2 * SequenceFile::ticksPerBeat, 240, 1.0f });

        const auto firstSound = [&] (double tempo, bool click, const SequenceFile::Sequence& sequence, juce::int64 frames, juce::int64& position)
        {
            SequencerPlayer player;
            player.getEffects().prepare (rate);
            const auto b = SequencerDocument::sessionFor (sequence, tempo, rate);
            juce::String createError;
            player.getEffects().add (b.trackId, engine.pluginFormatManager.createPluginInstance (synth, rate, 512, createError));
            player.setState (std::make_shared<const SessionState> (SequencerDocument::withSequence (b, sequence, tempo)), b.trackId);
            player.prepareToPlay (512, rate);
            player.setClick (click);
            player.play();

            juce::AudioBuffer<float> heard (2, (int) frames), block (2, 512);
            heard.clear();

            for (int at = 0; at < (int) frames; at += 512)
            {
                const auto n = juce::jmin (512, (int) frames - at);
                player.getNextAudioBlock ({ &block, 0, n });
                heard.copyFrom (0, at, block, 0, 0, n);
            }

            position = player.getPosition();
            juce::MessageManager::getInstance()->runDispatchLoopUntil (20);

            for (int i = 0; i < heard.getNumSamples(); ++i)
                if (std::abs (heard.getSample (0, i)) > 1.0e-4f)
                    return (juce::int64) i;

            return (juce::int64) -1;
        };

        juce::int64 position = 0;
        const auto on90 = firstSound (90.0, false, beat, 96000, position);
        const auto on140 = firstSound (140.0, false, beat, 96000, position);
        const auto beat90 = (juce::int64) std::llround (2 * 60.0 / 90.0 * rate), beat140 = (juce::int64) std::llround (2 * 60.0 / 140.0 * rate);
        const auto bar140 = (juce::int64) std::llround (4 * 60.0 / 140.0 * rate);
        const auto looped = position == 96000 - bar140;

        auto clickOnly = beat;
        clickOnly.notes.clear();
        juce::int64 unused = 0;
        const auto clickAt = firstSound (90.0, true, clickOnly, 1000, unused);

        check (on90 >= beat90 && on90 < beat90 + 32 && on140 >= beat140 && on140 < beat140 + 32 && looped && clickAt >= 0 && clickAt <= 1,
               "sequencer: a note on beat 2 is first heard at frame " + juce::String (on90) + " at 90 BPM (beat 2 is " + juce::String (beat90) + ") and "
               + juce::String (on140) + " at 140 (" + juce::String (beat140) + "); looping, it is back at the start after its bar; the click sounds from the first beat (its sine starts at 0, so is heard from frame 1)");

        juce::MessageManager::getInstance()->runDispatchLoopUntil (50);
        folder.deleteRecursively();
    }

    // The writing tools (step 4).
    {
        using namespace NoteTools;
        constexpr juce::int64 beat = 24000;   // 120 BPM at 48 kHz

        // Keys and scales.
        const auto aMinor = parseKey ("A minor"), bFlat = parseKey ("Bb major"), fDorian = parseKey ("F# dorian"), am = parseKey ("Am"), c = parseKey ("C");
        const auto cMajor = *c;
        check (aMinor && aMinor->root == 9 && aMinor->mode == "minor" && bFlat && bFlat->root == 10 && fDorian && fDorian->root == 6 && fDorian->mode == "dorian"
                   && am && *am == *aMinor && c->name() == "C major" && ! parseKey ("H minor") && ! parseKey ("C weird") && ! parseKey ("")
                   && cMajor.snap (61) == 60 && cMajor.snap (66) == 65 && cMajor.step (64, 1) == 65 && cMajor.step (60, -1) == 59 && cMajor.step (72, 7) == 84
                   && cMajor.step (61, 1) == 62 && cMajor.contains (71) && ! cMajor.contains (70),
               "note tools: keys are read (A minor, Bb major, F# dorian, Am, C) and others refused; in C major 61 snaps to 60 (the lower on a tie), 66 to 65, "
               "a step up from E is F, down from C is B, seven steps up an octave");

        const auto line = [] (std::initializer_list<int> pitches)
        {
            std::vector<ClipNote> notes;
            juce::int64 t = 0;

            for (auto p : pitches)
                notes.push_back ({ p, (t += 1000) - 1000, 1000, 0.8f });

            return notes;
        };
        check (guessKey (line ({ 57, 60, 64, 67, 71, 62, 65 })).name() == "A minor" && guessKey (line ({ 60, 57, 64, 67, 71, 62, 65 })).name() == "C major"
                   && guessKey (line ({ 62, 64, 66, 67, 69, 71, 73 })).name() == "D major" && guessKey ({}).name() == "C major",
               "note tools: a key is guessed from the notes: the white keys starting on A are A minor, starting on C C major; D E F# G A B C# is D major");

        // Chords from Roman numerals.
        juce::String error;
        const auto pitchesOf = [] (const std::vector<Chord>& chords)
        {
            std::vector<std::vector<int>> out;

            for (auto& ch : chords)
                out.push_back (ch.pitches);

            return out;
        };
        const auto minorRun = pitchesOf (parseProgression (juce::String::fromUTF8 ("i\xe2\x80\x93VI\xe2\x80\x93III\xe2\x80\x93VII"), *aMinor, 3, error));
        const auto jazz = pitchesOf (parseProgression (juce::String::fromUTF8 ("ii7 V7 Imaj7 vii\xc2\xb0 bVII Vsus4 vii\xc3\xb8" "7 viidim7 vii\xc2\xb0" "7"), cMajor, 4, error));
        juce::String bad1, bad2, bad3;
        const auto none1 = parseProgression ("I VIII", cMajor, 4, bad1).empty();
        const auto none2 = parseProgression ("Vx", cMajor, 4, bad2).empty();
        const auto none3 = parseProgression ("I IV", *parseKey ("C major pentatonic"), 4, bad3).empty();
        const std::vector<std::vector<int>> wantMinor { { 57, 60, 64 }, { 65, 69, 72 }, { 60, 64, 67 }, { 67, 71, 74 } };
        const std::vector<std::vector<int>> wantJazz { { 62, 65, 69, 72 }, { 67, 71, 74, 77 }, { 60, 64, 67, 71 }, { 71, 74, 77 }, { 70, 74, 77 }, { 67, 72, 74 }, { 71, 74, 77, 81 }, { 71, 74, 77, 80 }, { 71, 74, 77, 80 } };
        const auto written = chordNotes (parseProgression ("I IV", cMajor, 4, error), 500, 4 * beat, 0.7f);

        check (minorRun == wantMinor && jazz == wantJazz && none1 && bad1.contains ("VIII") && none2 && bad2.contains ("x") && none3 && bad3.contains ("seven")
                   && written.size() == 6 && written[0].start == 500 && written[3].start == 500 + 4 * beat && written[5].pitch == 72 && written[0].length == 4 * beat,
               "note tools: i-VI-III-VII (en dashes) in A minor from octave 3 is Am F C G in root position; ii7 V7 Imaj7 vii-diminished bVII Vsus4 vii-half-diminished-7 viidim7 vii-diminished-7 in C are "
               "Dm7 G7 Cmaj7 Bdim Bb Gsus4 Bm7b5 Bdim7 Bdim7; VIII, an unknown suffix and a pentatonic key are refused with why; chords are written a bar each from the start");

        // The arpeggiator.
        const std::vector<ClipNote> chord { { 64, 0, 4 * beat, 0.6f }, { 60, 10, 4 * beat, 0.8f }, { 67, 0, 4 * beat, 0.7f } };
        const auto pitches = [] (const std::vector<ClipNote>& notes)
        {
            std::vector<int> p;

            for (auto& n : notes)
                p.push_back (n.pitch);

            return p;
        };
        Arp arp;
        arp.step = beat / 4;
        const auto up = arpeggiate (chord, arp);
        arp.order = ArpOrder::down;
        const auto down = arpeggiate (chord, arp);
        arp.order = ArpOrder::upDown;
        const auto upDown = arpeggiate (chord, arp);
        arp.order = ArpOrder::up;
        arp.octaves = 2;
        const auto twoOctaves = arpeggiate (chord, arp);
        arp.order = ArpOrder::random;
        arp.seed = 7;
        const auto random1 = arpeggiate (chord, arp), random2 = arpeggiate (chord, arp);
        arp.seed = 8;
        const auto random3 = arpeggiate (chord, arp);
        auto noRepeats = true;

        for (size_t i = 1; i < random1.size(); ++i)
            noRepeats = noRepeats && random1[i].pitch != random1[i - 1].pitch;

        const auto firstEight = [&] (const std::vector<ClipNote>& n) { auto p = pitches (n); p.resize (juce::jmin ((size_t) 8, p.size())); return p; };

        check (up.size() == 16 && firstEight (up) == std::vector<int> { 60, 64, 67, 60, 64, 67, 60, 64 } && up[1].start == beat / 4 && up[0].length == 5400
                   && std::abs (up[0].velocity - 0.7f) < 1.0e-6f && up.back().start == 15 * beat / 4
                   && firstEight (down) == std::vector<int> { 67, 64, 60, 67, 64, 60, 67, 64 }
                   && firstEight (upDown) == std::vector<int> { 60, 64, 67, 64, 60, 64, 67, 64 }
                   && firstEight (twoOctaves) == std::vector<int> { 60, 64, 67, 72, 76, 79, 60, 64 }
                   && random1 == random2 && random1 != random3 && noRepeats && random1.size() == 16,
               "note tools: C E G held four beats, arpeggiated at a quarter beat, is 16 notes 0.9 of a step long at the chord's mean velocity: up C E G C..., down G E C..., "
               "up and down C E G E C..., over two octaves C E G C' E' G'; random is the same for a seed, another for another seed, never one pitch twice running");

        // Humanize and fit to the key.
        auto loose = line ({ 60, 62, 64, 65, 67, 69, 71, 72 });

        for (auto& n : loose)
            n.start += 5000;

        const auto before = loose;
        auto again = loose;
        Humanize h { 480, 0.2f, 11 };
        humanize (loose, { 1, 2, 3, 4, 5, 6 }, h, 100000);
        humanize (again, { 1, 2, 3, 4, 5, 6 }, h, 100000);
        auto withinAmounts = loose[0] == before[0] && loose[7] == before[7];
        auto anyMoved = false;

        for (size_t i = 1; i < 7; ++i)
        {
            withinAmounts = withinAmounts && std::abs (loose[i].start - before[i].start) <= 480
                         && std::abs (loose[i].velocity - before[i].velocity) <= 0.2f * before[i].velocity + 1.0e-6f && loose[i].pitch == before[i].pitch;
            anyMoved = anyMoved || loose[i].start != before[i].start;
        }

        auto offKey = line ({ 61, 63, 66, 68, 70 });
        fitToScale (offKey, {}, cMajor);

        check (withinAmounts && anyMoved && loose == again && pitches (offKey) == std::vector<int> { 60, 62, 65, 67, 69 },
               "note tools: humanize moves the chosen notes up to 10 ms and changes their velocity up to 20 %, the others untouched, the same for a seed; "
               "fit to the key moves C# D# F# G# A# to C D F G A");

        // Vary: velocity, pitch within the key, density.
        const auto part = line ({ 60, 64, 67, 72 });   // four notes, a thousand frames apart
        Vary v;
        v.scale = cMajor;
        v.step = 1000;
        v.seed = 3;
        v.velocity = 0.25f;
        const auto softer = vary (part, v, 8000);
        v.velocity = 0.0f;
        v.pitch = 1.0f;
        const auto moved = vary (part, v, 8000);
        v.pitch = 0.0f;
        v.density = 1.0f;
        const auto denser = vary (part, v, 8000);
        v.density = -1.0f;
        const auto thinnest = vary (part, v, 8000);
        v.density = -0.5f;
        const auto thinner = vary (part, v, 8000), thinnerAgain = vary (part, v, 8000);

        auto velocityOk = softer.size() == 4, pitchOk = moved.size() == 4;

        for (size_t i = 0; i < 4 && velocityOk && pitchOk; ++i)
        {
            velocityOk = std::abs (softer[i].velocity - part[i].velocity) <= 0.25f * part[i].velocity + 1.0e-6f && softer[i].pitch == part[i].pitch;
            pitchOk = (moved[i].pitch == cMajor.step (part[i].pitch, 1) || moved[i].pitch == cMajor.step (part[i].pitch, -1)) && moved[i].start == part[i].start;
        }

        check (velocityOk && pitchOk && denser.size() == 8 && denser[4].start == 4000 && denser[4].pitch == 72 && juce::exactlyEqual (denser[4].velocity, 0.8f * 0.6f)
                   && thinnest.size() == 1 && thinner.size() < 4 && ! thinner.empty() && thinner == thinnerAgain,
               "note tools: a variation changes velocities up to its share, moves every note a step up or down C major at a chance of 1, "
               "fills each of 4 empty steps with a softer copy of the note before at a density of +1, keeps one note at -1, and is the same for a seed");

        // Probability: a fixed draw for each pass, the same in any blocks, about the chance asked.
        NoteClip loop;
        loop.id = 7;
        loop.length = 400 * beat;
        loop.loopLength = beat;
        loop.notes = { { 60, 0, 1000, 0.8f, 0.5f }, { 64, beat / 2, 1000, 0.8f, 1.0f }, { 67, beat / 4, 1000, 0.8f, 0.0f } };

        const auto ons = [&loop] (int block)
        {
            std::map<int, std::vector<int>> byPitch;

            for (juce::int64 from = 0; from < loop.length; from += block)
            {
                juce::MidiBuffer midi;
                const auto n = (int) juce::jmin ((juce::int64) block, loop.length - from);
                loop.addMidi (from, n, midi);

                for (const auto m : midi)
                    if (m.getMessage().isNoteOn())
                        byPitch[m.getMessage().getNoteNumber()].push_back ((int) (from + m.samplePosition));
            }

            return byPitch;
        };

        const auto whole = ons (4096), odd = ons (333);
        const auto halves = whole.count (60) > 0 ? (int) whole.at (60).size() : 0;

        // Saved and read back: in a session and in a sequence.
        SessionState saved = sessionAt (120.0);
        const auto track = saved.addInstrumentTrack().id;
        NoteClip small;
        small.length = 4 * beat;
        small.notes = { { 60, 0, 1000, 0.8f, 0.25f }, { 62, 1000, 1000, 0.8f, 1.0f } };
        const auto smallId = SessionEdits::addNoteClip (saved, track, small);
        const auto folder = juce::File::createTempFile ("probability");
        folder.createDirectory();
        const auto sessionFile = folder.getChildFile ("p.tqsession");
        SessionFile::save (saved, sessionFile);
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        const auto loaded = SessionFile::load (sessionFile, formats);
        const NoteClip* back = nullptr;

        for (auto& t : loaded.state.tracks)
            for (auto& nc : t.noteClips)
                back = &nc;

        const auto sequence = SequenceFile::fromXml (*SequenceFile::toXml (*SequenceFile::fromClip (saved, smallId, nullptr)));

        check (whole == odd && halves > 160 && halves < 240 && whole.at (64).size() == 400 && whole.count (67) == 0
                   && back != nullptr && back->notes.size() == 2 && juce::exactlyEqual (back->notes[0].probability, 0.25f) && juce::exactlyEqual (back->notes[1].probability, 1.0f)
                   && sequence.has_value() && juce::exactlyEqual (sequence->notes[0].probability, 0.25f) && juce::exactlyEqual (sequence->notes[1].probability, 1.0f),
               "note probability: over 400 passes a note at 50 % plays " + juce::String (halves) + " times, on the same passes in blocks of 4096 and of 333 frames; "
               "one at 100 % plays all 400, one at 0 % none; a session file and a sequence keep 25 % and 100 %");

        // The Piano Roll's tools, through its callbacks.
        NoteClip edited;
        edited.id = 3;
        edited.length = 8 * beat;
        NoteEditor editor;
        editor.readClip = [&edited]() -> std::optional<NoteEditor::Shown>
        {
            NoteEditor::Shown shown;
            shown.clip = edited;
            shown.sampleRate = rate;
            shown.beatSeconds = 0.5;
            shown.key = "A minor";
            return shown;
        };
        juce::StringArray steps;
        editor.apply = [&edited, &steps] (const NoteClip& clip, const juce::String& description)
        {
            edited = clip;
            steps.add (description);
        };
        editor.refresh();
        const auto chordError = editor.writeChords ("i iv", 4.0, 3, true);
        editor.refresh();
        const auto chordPitches = pitches (edited.notes);
        editor.selectAll();
        editor.setProbability (0.5f);
        editor.refresh();
        const auto allHalf = std::all_of (edited.notes.begin(), edited.notes.end(), [] (const ClipNote& n) { return juce::exactlyEqual (n.probability, 0.5f); });
        editor.setScaleLock (true);
        editor.selectAll();
        editor.arpeggiate (arp = Arp { ArpOrder::up, 1, beat / 2, 0.5, 1 });
        editor.refresh();

        check (chordError.isEmpty() && chordPitches == std::vector<int> { 57, 60, 64, 62, 65, 69 } && edited.notes.size() == 16 && allHalf
                   && edited.notes[0].pitch == 57 && edited.notes[8].pitch == 62 && edited.notes[1].start == beat / 2
                   && steps == juce::StringArray { "Chords", "Note Probability", "Arpeggiate" } && editor.getKey().name() == "A minor",
               "piano roll tools: i iv in the part's key, A minor, writes Am and Dm a bar each; Probability sets every selected note's; Arpeggiate plays each chord "
               "at a half beat (8 notes each); each is one undo step");

        // Make Variation with changes, in the Sequencer.
        SequencerView view (engine);
        SequenceFile::Sequence groove;
        groove.name = "Hats";
        groove.length = 4 * SequenceFile::ticksPerBeat;
        groove.notes = { { 42, 0, 240, 0.8f }, { 42, 1920, 240, 0.8f } };
        const auto grooveFile = folder.getChildFile (juce::String ("Hats") + SequenceFile::extension);
        SequenceFile::save (groove, grooveFile);
        view.openFile (grooveFile);
        Vary fill;
        fill.density = 1.0f;
        fill.step = (juce::int64) std::llround (60.0 / view.getTempo() / 4.0 * view.getPlayer().getEffects().getSampleRate());
        fill.scale = cMajor;
        const auto fillError = view.duplicate (true, &fill);
        const auto filled = SequenceFile::load (folder.getChildFile (juce::String ("Hats Variation") + SequenceFile::extension), error);
        auto onSixteenths = filled.has_value();

        if (filled.has_value())
            for (auto& n : filled->notes)
                onSixteenths = onSixteenths && n.start % 240 == 0 && n.pitch == 42;

        check (fillError.isEmpty() && filled.has_value() && filled->notes.size() == 16 && onSixteenths && filled->variationOf == "Hats",
               "sequencer: Make Variation at a density of +1 fills every empty sixteenth of a two-hit bar of hats (16 notes, each on a sixteenth), naming Hats as its original");

        juce::MessageManager::getInstance()->runDispatchLoopUntil (50);
        folder.deleteRecursively();
    }
}
