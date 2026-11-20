#include <JuceHeader.h>
#include "../Source/NoteEditor.h"
#include "../Source/MidiInput.h"
#include "../Source/Session.h"
#include "TestUtils.h"
#include "DrumGridTests.h"

namespace
{
    using TestUtils::check;

    constexpr double rate = 48000.0;
    constexpr juce::int64 beat = 24000;   // 120 bpm at 48 kHz

    juce::MouseEvent eventAt (juce::Component& c, juce::Point<float> at, juce::Point<float> down, bool dragged = false,
                              juce::ModifierKeys mods = juce::ModifierKeys::leftButtonModifier)
    {
        const auto now = juce::Time::getCurrentTime();
        return juce::MouseEvent (juce::Desktop::getInstance().getMainMouseSource(), at, mods,
                                 juce::MouseInputSource::defaultPressure, juce::MouseInputSource::defaultOrientation, juce::MouseInputSource::defaultRotation,
                                 juce::MouseInputSource::defaultTiltX, juce::MouseInputSource::defaultTiltY, &c, &c, now, down, now, 1, dragged);
    }

    /** A piano roll showing a one-bar clip as a drum grid, and the steps it hands back. */
    struct Rig
    {
        NoteEditor::Shown shown;
        std::vector<std::pair<NoteClip, juce::String>> applied;
        std::vector<std::pair<std::vector<DrumSound>, juce::String>> maps;
        NoteEditor editor;

        Rig()
        {
            shown.clip.id = 3;
            shown.clip.name = "Beat";
            shown.clip.length = 4 * beat;
            shown.sampleRate = rate;
            shown.beatSeconds = 0.5;
            shown.trackName = "Drums";
            shown.drums = true;
            editor.readClip = [this] { return std::optional<NoteEditor::Shown> (shown); };
            editor.apply = [this] (const NoteClip& c, const juce::String& d)
            {
                shown.clip = c;
                applied.emplace_back (c, d);
            };
            editor.showDrums = [this] (bool drums) { shown.drums = drums; };
            editor.applyDrumMap = [this] (const std::vector<DrumSound>& map, const juce::String& d)
            {
                shown.drumMap = map.empty() ? DrumSound::generalMidi() : map;
                maps.emplace_back (map, d);
            };
            editor.setBounds (0, 0, 1000, 600);
            editor.refresh();
        }

        int rowOf (int pitch) const
        {
            const auto rows = editor.drumRows();

            for (size_t i = 0; i < rows.size(); ++i)
                if (rows[i].pitch == pitch)
                    return (int) i;

            return -1;
        }

        juce::Point<float> cell (int pitch, int step) const   { return editor.cellBounds (rowOf (pitch), step).getCentre(); }

        void click (juce::Point<float> p, juce::ModifierKeys mods = juce::ModifierKeys::leftButtonModifier)
        {
            editor.mouseDown (eventAt (editor, p, p, false, mods));
            editor.mouseUp (eventAt (editor, p, p, false, mods));
        }

        void drag (juce::Point<float> from, juce::Point<float> to)
        {
            editor.mouseDown (eventAt (editor, from, from));
            editor.mouseDrag (eventAt (editor, (from + to) / 2.0f, from, true));
            editor.mouseDrag (eventAt (editor, to, from, true));
            editor.mouseUp (eventAt (editor, to, from, true));
        }

        std::vector<ClipNote> sorted() const
        {
            auto notes = shown.clip.notes;
            std::sort (notes.begin(), notes.end(), [] (const ClipNote& a, const ClipNote& b) { return a.start != b.start ? a.start < b.start : a.pitch < b.pitch; });
            return notes;
        }

        juce::String last() const   { return applied.empty() ? juce::String() : applied.back().second; }
    };

    void testGrid()
    {
        Rig rig;
        auto& e = rig.editor;
        const auto rows = e.drumRows();
        check (e.showingDrums() && rows == DrumSound::generalMidi() && rows.size() == 47 && rows.front().pitch == 35 && rows[1].name == "Bass Drum"
                   && rows[3].name == "Acoustic Snare" && rows[7].name == "Closed Hi-Hat" && rows.back().pitch == 81 && e.stepCount() == 16
                   && e.lineFrame (1) == 6000 && e.lineFrame (4) == beat,
               "drum grid: a row for each General MIDI drum sound (35 to 81), a step for each 1/16 of the one-bar clip");

        // Four on the floor, the snare on two and four, eighth-note hats: the notes that pattern means.
        e.setVelocity (100.0f / 127.0f);

        for (int s : { 0, 4, 8, 12 })
            rig.click (rig.cell (36, s));

        for (int s : { 4, 12 })
            rig.click (rig.cell (38, s));

        for (int s = 0; s < 16; s += 2)
            rig.click (rig.cell (42, s));

        std::vector<ClipNote> want;

        for (int s : { 0, 4, 8, 12 })
            want.push_back ({ 36, s * 6000, 6000, 100.0f / 127.0f });

        for (int s : { 4, 12 })
            want.push_back ({ 38, s * 6000, 6000, 100.0f / 127.0f });

        for (int s = 0; s < 16; s += 2)
            want.push_back ({ 42, s * 6000, 6000, 100.0f / 127.0f });

        std::sort (want.begin(), want.end(), [] (const ClipNote& a, const ClipNote& b) { return a.start != b.start ? a.start < b.start : a.pitch < b.pitch; });
        check (rig.sorted() == want && rig.applied.size() == 14 && rig.last() == "Add Note"
                   && e.noteInCell (rig.rowOf (36), 4) >= 0 && e.noteInCell (rig.rowOf (36), 5) < 0,
               "drum grid: clicked steps put in exactly the notes of the pattern, each a step long at the slider's velocity, one undo step each");

        // A click on a set step takes it away; Option-click too; a drag sets its velocity.
        rig.click (rig.cell (36, 8));
        const auto afterClick = rig.shown.clip.notes.size();
        rig.click (rig.cell (38, 12), juce::ModifierKeys (juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::altModifier));
        const auto afterOption = rig.shown.clip.notes.size();
        const auto from = rig.cell (42, 2);
        rig.drag (from, from.translated (0.0f, -20.0f));
        const auto louder = e.noteInCell (rig.rowOf (42), 2);
        const auto v = louder >= 0 ? rig.shown.clip.notes[(size_t) louder].velocity : 0.0f;
        check (afterClick == 13 && afterOption == 12 && rig.shown.clip.notes.size() == 12 && rig.last() == "Note Velocity"
                   && std::abs (v - (100.0f / 127.0f + 0.2f)) < 1.0e-4f && rig.shown.clip.notes[(size_t) louder].start == 12000,
               "drum grid: clicking a set step removes it, Option-click too, and dragging up from it raises its velocity (20 px, +0.2)");

        // A new step dragged down at once: put in, then softer, as two steps.
        const auto steps = rig.applied.size();
        const auto kickFrom = rig.cell (36, 10);
        rig.drag (kickFrom, kickFrom.translated (0.0f, 30.0f));
        const auto soft = e.noteInCell (rig.rowOf (36), 10);   // put in at the slider's velocity, which follows the last note clicked
        check (soft >= 0 && rig.applied.size() == steps + 2 && rig.applied[steps].second == "Add Note" && rig.last() == "Note Velocity"
                   && std::abs (rig.shown.clip.notes[(size_t) soft].velocity - (rig.applied[steps].first.notes[(size_t) soft].velocity - 0.3f)) < 1.0e-4f,
               "drum grid: a drag from an empty step puts a note in and sets its velocity");

        // Notes outside the map get rows of their own, named by pitch.
        rig.shown.clip.notes.push_back ({ 90, 0, 6000, 0.5f });
        e.refresh();
        const auto withExtra = e.drumRows();
        check (withExtra.size() == 48 && withExtra.back().pitch == 90 && withExtra.back().name == "F#6" && e.noteInCell (47, 0) >= 0,
               "drum grid: a note outside the map shows on a row of its own at the bottom, named by its pitch");

        // The toggle goes back to the piano roll; the map is the track's.
        e.showDrums (false);
        e.refresh();
        const auto piano = ! e.showingDrums() && e.rollArea().getX() == NoteEditor::keyboardWidth;
        e.showDrums (true);
        e.refresh();
        check (piano && e.showingDrums() && e.rollArea().getX() == NoteEditor::drumNamesWidth,
               "drum grid: the Drums toggle switches between the drum grid and the piano roll");

        std::vector<DrumSound> kit { { 36, "Kick" }, { 38, "Snare" }, { 42, "Hat" } };
        e.setDrumMap (kit, "Edit Drum Sound");
        e.refresh();
        const auto own = e.drumRows();
        e.setDrumMap (DrumSound::generalMidi(), "General MIDI Drum Map");
        check (own.size() == 4 && own[0].name == "Kick" && own[2].name == "Hat" && own[3].pitch == 90 && rig.maps.size() == 2
                   && rig.maps[0].first == kit && rig.maps[1].first.empty() && rig.maps[1].second == "General MIDI Drum Map",
               "drum grid: the map is the track's own (its rows in its order), and General MIDI's is stored as no map");

        // Drawn without trouble.
        const auto image = e.createComponentSnapshot (e.getLocalBounds());
        check (image.isValid() && image.getWidth() == 1000, "drum grid: drawn");
    }

    void testSwing()
    {
        Rig rig;
        auto& e = rig.editor;
        e.setSwing (0.66);
        const auto pair = 2 * 6000;   // two 1/16 steps
        const auto second = (juce::int64) std::llround (0.66 * pair);
        check (juce::exactlyEqual (e.getSwing(), 0.66) && e.lineFrame (0) == 0 && e.lineFrame (1) == second && e.lineFrame (2) == pair
                   && e.lineFrame (3) == pair + second && e.lineFrame (4) == beat,
               "swing: at 66 %, each pair's second 1/16 line moves to 66 % of the pair (" + juce::String (second) + " frames), the first stays");

        rig.click (rig.cell (42, 1));
        rig.click (rig.cell (42, 3));
        const auto notes = rig.sorted();
        check (notes.size() == 2 && notes[0].start == second && notes[1].start == pair + second,
               "swing: drum steps on the off-beats go in at their swung frames");

        // The piano roll snaps to the swung lines, and swing is off on a grid that doesn't pair.
        e.showDrums (false);
        e.refresh();
        rig.shown.clip.notes.clear();
        e.refresh();
        const auto at = [&] (juce::int64 frame) { return juce::Point<float> (e.noteBounds ({ 60, frame, 1, 1.0f }).getX() + 1.0f, e.noteBounds ({ 60, frame, 1, 1.0f }).getCentreY()); };
        rig.click (at (second + 1000));
        rig.click (at (second - 1000));
        const auto snapped = rig.sorted();
        e.setGridDivision (8);
        const auto straight = e.lineFrame (1) == 3000 && e.lineFrame (3) == 9000;
        check (snapped.size() == 2 && snapped[0].start == 0 && snapped[1].start == second && straight,
               "swing: the piano roll snaps to the swung line at or before the mouse; a 1/8-beat grid stays straight");

        // Quantize Notes: swing, strength and lengths.
        SessionState s;
        s.sampleRate = rate;
        auto& instrument = s.addTrack();
        instrument.isInstrument = true;
        const auto track = instrument.id;
        NoteClip played;
        played.length = 4 * beat;
        played.notes = { { 36, 500, 5000, 1.0f }, { 42, 8000, 2000, 1.0f }, { 38, 23000, 7000, 1.0f } };
        const auto id = SessionEdits::addNoteClip (s, track, played);
        auto swung = s, half = s, ends = s;
        NoteQuantize q;
        q.swing = 0.66;
        SessionEdits::quantizeNotes (swung, { id }, q);
        q.swing = 0.5;
        q.strength = 0.5;
        SessionEdits::quantizeNotes (half, { id }, q);
        q.strength = 1.0;
        q.lengths = true;
        SessionEdits::quantizeNotes (ends, { id }, q);
        const auto& sw = swung.tracks[0].noteClips[0].notes;
        const auto& hf = half.tracks[0].noteClips[0].notes;
        const auto& en = ends.tracks[0].noteClips[0].notes;
        check (sw[0].start == 0 && sw[1].start == second && sw[2].start == beat && sw[0].length == 5000 && sw[1].length == 2000
                   && hf[0].start == 250 && hf[1].start == 7000 && hf[2].start == 23500
                   && en[0].start == 0 && en[0].length == 6000 && en[1].start == 6000 && en[1].length == 6000 && en[2].start == beat && en[2].length == 6000,
               "quantize: swing moves the off-beat to its swung line, strength 50 % moves halfway, and lengths puts the ends on lines too (one step at least)");
    }

    void testChannelsAndFile (juce::AudioFormatManager& formats)
    {
        Rig rig;
        rig.editor.setChannel (10);
        check (rig.shown.clip.channel == 10 && rig.last() == "MIDI Channel", "channels: the piano roll's channel choice sets the clip's, as an undo step");

        NoteClip c;
        c.length = beat;
        c.channel = 10;
        c.notes = { { 36, 0, 1000, 1.0f } };
        c.lanes.push_back ({ 7, {} });
        c.lanes[0].envelope.add (0, 0.5f);
        juce::MidiBuffer out;
        c.addMidi (0, (int) beat, out);
        auto allOnTen = ! out.isEmpty();

        for (const auto meta : out)
            allOnTen = allOnTen && meta.getMessage().getChannel() == 10;

        check (allOnTen && NoteClip::laneMessage (7, 0.5f, 10).getChannel() == 10 && NoteClip::laneMessage (ControllerLane::pitchBend, 0.5f, 3).getChannel() == 3,
               "channels: a clip's notes and lane values go out on its channel");

        SessionState s;
        s.sampleRate = rate;
        auto& t = s.addTrack();
        t.isInstrument = true;
        t.drumGrid = true;
        t.drumMap = { { 36, "Kick" }, { 40, "Rim" } };
        const auto trackId = t.id;
        const auto kit = t.drumMap;   // t moves when the next track is added
        SessionEdits::addNoteClip (s, trackId, c);
        auto& plain = s.addTrack();
        plain.isInstrument = true;

        const auto file = juce::File::createTempFile ("tqsession");
        const auto error = SessionFile::save (s, file);
        const auto loaded = SessionFile::load (file, formats);
        const auto& got = loaded.state;
        check (error.isEmpty() && loaded.error.isEmpty() && got.tracks.size() == 2 && got.tracks[0].drumGrid && got.tracks[0].drumMap == kit
                   && got.tracks[0].noteClips.size() == 1 && got.tracks[0].noteClips[0].channel == 10
                   && ! got.tracks[1].drumGrid && got.tracks[1].drumMap.empty() && got.tracks[1].drumSounds() == DrumSound::generalMidi(),
               "file: the drum grid, a track's drum map and a clip's channel come back from a session file (no map: General MIDI's)");
        file.deleteFile();
    }

    void testCapture()
    {
        // Played while stopped: kept with the device's clock, captured from the cursor at the session's rate.
        MidiIn::Live live;
        live.prepare (44100.0);
        const auto block = [&] (std::initializer_list<std::pair<int, juce::MidiMessage>> events, juce::int64 sessionFrame)
        {
            juce::MidiBuffer b;

            for (auto& [at, m] : events)
                b.addEvent (m, at);

            live.rememberBlock (b, 441, sessionFrame);
        };

        block ({ { 100, juce::MidiMessage::noteOn (1, 36, (juce::uint8) 100) } }, -1);
        block ({}, -1);
        block ({ { 100, juce::MidiMessage::noteOff (1, 36) }, { 100, juce::MidiMessage::noteOn (1, 38, (juce::uint8) 64) } }, -1);
        block ({ { 341, juce::MidiMessage::noteOff (1, 38) } }, -1);
        const auto recent = live.takeRecent();
        const auto clip = MidiIn::clipOfRecent (recent, 96000, rate);
        const auto again = live.takeRecent();
        auto notes = clip.notes;
        std::sort (notes.begin(), notes.end(), [] (const ClipNote& a, const ClipNote& b) { return a.start < b.start; });
        const auto toSession = [] (juce::int64 deviceFrames) { return (juce::int64) std::llround ((double) deviceFrames * rate / 44100.0); };
        check (! recent.whilePlaying && clip.name == "Captured" && clip.position == 96000 && notes.size() == 2
                   && notes[0].pitch == 36 && notes[0].start == 0 && notes[0].length == toSession (882)
                   && notes[1].pitch == 38 && notes[1].start == toSession (882) && notes[1].length == toSession (1323 + 341 - 100) - toSession (882)
                   && std::abs (notes[1].velocity - 64.0f / 127.0f) < 0.01f && again.capture.notes.empty(),
               "capture: notes played while stopped come back from the cursor at their times, brought to the session's rate, and are then forgotten");

        // Played while the session played: at their session frames.
        block ({ { 10, juce::MidiMessage::noteOn (1, 42, (juce::uint8) 90) } }, 48000);
        block ({ { 20, juce::MidiMessage::noteOff (1, 42) } }, 48441);
        const auto playing = live.takeRecent();
        const auto onTime = MidiIn::clipOfRecent (playing, 0, rate);
        check (playing.whilePlaying && onTime.notes.size() == 1 && onTime.getOrigin() + onTime.notes[0].start == 48010 && onTime.notes[0].length == 451,
               "capture: notes played while the session played land at the session frames they were played at");
    }
}

void runDrumGridTests (juce::AudioFormatManager& formats)
{
    std::cout << "Drum grid, swing, channels and Capture MIDI:" << std::endl;
    testGrid();
    testSwing();
    testChannelsAndFile (formats);
    testCapture();
}
