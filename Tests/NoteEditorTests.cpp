#include <JuceHeader.h>
#include "../Source/NoteEditor.h"
#include "../Source/Theme.h"
#include "TestUtils.h"
#include "NoteEditorTests.h"

namespace
{
    using TestUtils::check;

    constexpr double rate = 48000.0;

    juce::MouseEvent eventAt (juce::Component& c, juce::Point<float> at, juce::Point<float> down, bool dragged = false,
                              juce::ModifierKeys mods = juce::ModifierKeys::leftButtonModifier)
    {
        const auto now = juce::Time::getCurrentTime();
        return juce::MouseEvent (juce::Desktop::getInstance().getMainMouseSource(), at, mods,
                                 juce::MouseInputSource::defaultPressure, juce::MouseInputSource::defaultOrientation, juce::MouseInputSource::defaultRotation,
                                 juce::MouseInputSource::defaultTiltX, juce::MouseInputSource::defaultTiltY, &c, &c, now, down, now, 1, dragged);
    }

    /** What the editor is given and hands back: the clip it shows, and the steps it applied. */
    struct Rig
    {
        NoteEditor::Shown shown;
        std::vector<std::pair<NoteClip, juce::String>> applied;
        NoteEditor editor;

        Rig()
        {
            shown.clip.id = 7;
            shown.clip.name = "Riff";
            shown.clip.length = (juce::int64) (4.0 * rate);   // four seconds: eight beats at 120
            shown.sampleRate = rate;
            shown.beatSeconds = 0.5;
            shown.trackName = "Synth";
            editor.readClip = [this] { return std::optional<NoteEditor::Shown> (shown); };
            editor.apply = [this] (const NoteClip& c, const juce::String& d)
            {
                shown.clip = c;
                applied.emplace_back (c, d);
            };
            editor.setBounds (0, 0, 900, 400);
            editor.refresh();
        }

        /** Where a note at this pitch and frame starts: its left edge, the middle of its row. */
        juce::Point<float> at (int pitch, juce::int64 frame) const
        {
            const auto r = editor.noteBounds ({ pitch, frame, 1, 1.0f });
            return { r.getX(), r.getCentreY() };
        }

        void click (juce::Point<float> p, juce::ModifierKeys mods = juce::ModifierKeys::leftButtonModifier)
        {
            editor.mouseDown (eventAt (editor, p, p, false, mods));
            editor.mouseUp (eventAt (editor, p, p, false, mods));
        }

        void drag (juce::Point<float> from, juce::Point<float> to, juce::ModifierKeys mods = juce::ModifierKeys::leftButtonModifier)
        {
            editor.mouseDown (eventAt (editor, from, from, false, mods));
            editor.mouseDrag (eventAt (editor, (from + to) / 2.0f, from, true, mods));
            editor.mouseDrag (eventAt (editor, to, from, true, mods));
            editor.mouseUp (eventAt (editor, to, from, true, mods));
        }

        const std::vector<ClipNote>& notes() const  { return shown.clip.notes; }
        juce::String last() const                   { return applied.empty() ? juce::String() : applied.back().second; }
    };
}

void runNoteEditorTests()
{
    std::cout << "Piano roll:" << std::endl;

    Rig rig;
    auto& e = rig.editor;
    const auto step = e.gridStepFrames();
    check (e.getShown().has_value() && e.getShown()->clip.id == 7 && e.getSelectedNotes().empty() && e.getGridDivision() == 4 && step == 6000
               && e.pitchAt (rig.at (64, 0).y) == 64 && std::abs (e.frameAt (rig.at (64, 30000).x) - 30000) < 300,
           "piano roll: shows the clip it is given on a 1/4-beat grid (6,000 frames at 120), its rows and columns reading back a pitch and a frame");

    // Step entry: a click on empty grid puts a grid-long note at the grid line before the mouse.
    rig.click (rig.at (64, 7000));
    check (rig.notes().size() == 1 && rig.notes()[0].pitch == 64 && rig.notes()[0].start == 6000 && rig.notes()[0].length == 6000
               && std::abs (rig.notes()[0].velocity - 100.0f / 127.0f) < 1.0e-6f && rig.last() == "Add Note" && rig.applied.size() == 1
               && e.getSelectedNotes() == std::set<int> { 0 },
           "piano roll: a click on empty grid adds a note at the grid line before it, one step long, at the slider's velocity, selected (one undo step)");

    // Dragging the note moves it in pitch and time, snapped.
    rig.drag (rig.at (64, 8000), rig.at (66, 8000 + 12500));
    check (rig.notes().size() == 1 && rig.notes()[0].pitch == 66 && rig.notes()[0].start == 18000 && rig.notes()[0].length == 6000
               && rig.last() == "Move Notes" && rig.applied.size() == 2,
           "piano roll: dragging a note moves it two semitones up and two grid steps on (snapped), one undo step");

    // Dragging its right edge changes its length (snapped to the grid line after the mouse).
    const auto r = e.noteBounds (rig.notes()[0]);
    rig.drag ({ r.getRight() - 2.0f, r.getCentreY() }, { r.getRight() - 2.0f + (e.noteBounds ({ 66, 12000, 1, 1.0f }).getRight() - e.noteBounds ({ 66, 0, 1, 1.0f }).getRight()), r.getCentreY() });
    check (rig.notes().size() == 1 && rig.notes()[0].start == 18000 && rig.notes()[0].length == 18000 && rig.last() == "Note Length",
           "piano roll: dragging a note's right edge makes it three steps long");

    // A second note, then Shift-click adds it to the selection and the velocity slider sets both.
    rig.click (rig.at (60, 36500));
    rig.click (rig.at (66, 20000), juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::shiftModifier);
    e.setVelocity (0.5f);
    check (rig.notes().size() == 2 && e.getSelectedNotes() == std::set<int> { 0, 1 } && rig.last() == "Note Velocity"
               && std::abs (rig.notes()[0].velocity - 64.0f / 127.0f) < 1.0e-6f && std::abs (rig.notes()[1].velocity - 64.0f / 127.0f) < 1.0e-6f,
           "piano roll: Shift-click adds a note to the selection, and the velocity slider sets every selected note");

    // The arrow keys move the selection: up a semitone, right a grid step, Shift-up an octave.
    e.keyPressed (juce::KeyPress (juce::KeyPress::upKey));
    e.keyPressed (juce::KeyPress (juce::KeyPress::rightKey));
    e.keyPressed (juce::KeyPress (juce::KeyPress::upKey, juce::ModifierKeys::shiftModifier, 0));
    check (rig.notes()[0].pitch == 79 && rig.notes()[0].start == 24000 && rig.notes()[1].pitch == 73 && rig.notes()[1].start == 42000
               && rig.last() == "Move Notes",
           "piano roll: the arrow keys move the selected notes a semitone and a grid step, Shift an octave");

    // Option-click removes a note; Delete removes the selected ones; Cmd-A selects all.
    rig.click (rig.at (73, 44000), juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::altModifier);
    const auto afterOption = rig.notes().size();
    e.keyPressed (juce::KeyPress ('a', juce::ModifierKeys::commandModifier, 0));
    const auto all = e.getSelectedNotes().size();
    e.keyPressed (juce::KeyPress (juce::KeyPress::deleteKey));
    check (afterOption == 1 && all == 1 && rig.notes().empty() && rig.last() == "Remove Note" && e.getSelectedNotes().empty(),
           "piano roll: Option-click removes a note, Cmd-A selects all, Delete removes the selected");

    // A box dragged with Shift over empty grid selects the notes it touches; a click on empty grid
    // past the clip's end adds nothing; with the grid off a note takes the last length.
    rig.click (rig.at (60, 6500));
    rig.click (rig.at (62, 30500));
    rig.drag (rig.at (58, 0), rig.at (61, 12000), juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::shiftModifier);
    const auto boxed = e.getSelectedNotes();
    const auto before = rig.applied.size();
    e.mouseDown (eventAt (e, { (float) e.getWidth() - 1.0f, rig.at (60, 0).y }, { (float) e.getWidth() - 1.0f, rig.at (60, 0).y }));
    e.mouseUp (eventAt (e, { (float) e.getWidth() - 1.0f, rig.at (60, 0).y }, { (float) e.getWidth() - 1.0f, rig.at (60, 0).y }));
    e.setGridDivision (0);
    rig.click (rig.at (65, 50123));
    check (boxed == std::set<int> { 0 } && rig.applied.size() == before + 1 && rig.notes().size() == 3 && e.gridStepFrames() == 0
               && std::abs (rig.notes()[2].start - 50123) < 300 && rig.notes()[2].length == 18000,
           "piano roll: a Shift-drag selects the notes in its box, a click past the clip's end adds nothing, and with the grid off a "
           "note goes where the mouse is at the length last dragged to");

    // A trimmed clip shows its whole timeline, the offset washed; dragging the first repeat line sets the loop length.
    rig.shown.clip.notes.clear();
    rig.shown.clip.offset = 48000;
    rig.shown.clip.loopLength = 48000;
    e.refresh();
    e.setGridDivision (4);
    const auto whole = e.wholeLength();
    const auto loopX = e.noteBounds ({ 60, 48000, 1, 1.0f }).getX();
    rig.drag ({ loopX, rig.at (60, 0).y }, { e.noteBounds ({ 60, 96500, 1, 1.0f }).getX(), rig.at (60, 0).y });
    check (whole == 240000 && rig.notes().empty() && rig.shown.clip.loopLength == 96000 && rig.last() == "Loop Length",
           "piano roll: the timeline is the clip's offset and length, and dragging the first repeat line sets the loop length (snapped), one undo step");

    // A lane shown under the notes: a click puts a point there (snapped, the value from the
    // height), a drag moves it, Option-click removes it; each one undo step.
    e.setLane (1);
    const auto strip = e.laneArea();
    const auto laneX = [&e] (juce::int64 frame) { return e.noteBounds ({ 60, frame, 1, 1.0f }).getX(); };
    rig.click ({ laneX (6500), e.yForLaneValue (0.75f) });
    const auto* lane = rig.shown.clip.findLane (1);
    const auto added = lane != nullptr && lane->envelope.points.size() == 1 && lane->envelope.points[0].frame == 6000
                       && std::abs (lane->envelope.points[0].value - 0.75f) < 0.02f && rig.last() == "Lane Point";
    rig.drag ({ laneX (6000), e.yForLaneValue (0.75f) }, { laneX (12500), e.yForLaneValue (0.25f) });
    lane = rig.shown.clip.findLane (1);
    const auto moved = lane != nullptr && lane->envelope.points.size() == 1 && lane->envelope.points[0].frame == 12000
                       && std::abs (lane->envelope.points[0].value - 0.25f) < 0.02f;
    rig.click ({ laneX (12000), e.yForLaneValue (0.25f) }, juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::altModifier);
    lane = rig.shown.clip.findLane (1);
    check (! strip.isEmpty() && e.rollArea().getBottom() <= strip.getY() && added && moved && lane != nullptr && lane->envelope.isEmpty()
               && rig.last() == "Remove Lane Point" && e.getLane() == 1,
           "piano roll: a lane under the notes takes a point where it is clicked (snapped), moves it by dragging and drops it with Option-click, each an undo step");
    e.setLane (-1);
    check (e.laneArea().isEmpty() && e.getLane() == -1, "piano roll: with no lane chosen the notes have the whole height");

    // Another clip shown drops the selection; nothing shown disables the controls.
    rig.shown.clip.id = 8;
    e.refresh();
    const auto dropped = e.getSelectedNotes().empty() && e.getShown().has_value() && e.getShown()->clip.id == 8;
    e.readClip = [] { return std::optional<NoteEditor::Shown>(); };
    e.refresh();
    check (dropped && ! e.getShown().has_value(), "piano roll: a different clip clears the selection, and with none selected it shows nothing");

    // What sounds: the keys down the left, a note put in, clicked or dragged, each ended when the mouse is let go.
    {
        Rig sound;
        std::vector<std::pair<int, float>> heard;
        sound.editor.audition = [&heard] (int pitch, float velocity) { heard.emplace_back (pitch, velocity); };

        // Every note on is ended, by the off of its own pitch, before the next starts.
        const auto paired = [&heard]
        {
            if (heard.size() % 2 != 0)
                return false;

            for (size_t i = 0; i < heard.size(); i += 2)
                if (heard[i].second <= 0.0f || heard[i + 1].first != heard[i].first || heard[i + 1].second != 0.0f)
                    return false;

            return true;
        };

        const auto slider = 100.0f / 127.0f;
        const juce::Point<float> key60 { 10.0f, sound.at (60, 0).y }, key62 { 10.0f, sound.at (62, 0).y };
        sound.click (key60);
        const auto keyClick = heard;
        heard.clear();
        sound.drag (key60, key62);
        const auto glide = heard;
        const auto glidePaired = paired();
        heard.clear();
        check (keyClick.size() == 2 && keyClick[0].first == 60 && std::abs (keyClick[0].second - slider) < 1.0e-6f && keyClick[1] == std::pair<int, float> (60, 0.0f)
                   && glidePaired && glide.front().first == 60 && glide[glide.size() - 2].first == 62 && sound.applied.empty(),
               "piano roll: a key clicked sounds at the slider's velocity until let go, and a drag along the keys plays each it passes, "
               "ending each before the next, without touching the clip");

        sound.click (sound.at (64, 7000));
        const auto putIn = heard;
        heard.clear();
        sound.drag (sound.at (64, 8000), sound.at (66, 8000));
        const auto movedPaired = paired();
        check (putIn.size() == 2 && putIn[0].first == 64 && putIn[1] == std::pair<int, float> (64, 0.0f)
                   && movedPaired && heard.front().first == 64 && heard[heard.size() - 2].first == 66 && sound.notes().size() == 1 && sound.notes()[0].pitch == 66,
               "piano roll: a note put in sounds while the mouse is down, and one dragged sounds at each pitch it moves to");
    }

    // Zoom: a clip that repeats opens on its pattern; Cmd-scroll and the buttons zoom around a
    // point, the scroll bar and a sideways scroll move along, Fit shows it all, and notes are
    // put in where the zoomed grid shows them.
    {
        NoteEditor::Shown looped;
        looped.clip.id = 9;
        looped.clip.length = (juce::int64) (18.0 * rate);   // nine bars at 120
        looped.clip.loopLength = (juce::int64) (2.0 * rate); // a bar's pattern
        looped.sampleRate = rate;
        looped.beatSeconds = 0.5;
        Rig z;
        z.shown = looped;
        z.editor.refresh();
        const auto area = z.editor.rollArea();
        const auto opened = z.editor.getViewStart() == 0 && z.editor.visibleLength() == looped.clip.loopLength
                            && std::abs (z.editor.frameAt ((float) area.getRight() - 1.0f) - looped.clip.loopLength) <= 1;

        // Cmd-scroll in keeps the frame under the mouse where it is.
        const auto x = (float) area.getX() + 300.0f;
        const auto under = z.editor.frameAt (x);
        juce::MouseWheelDetails wheel {};
        wheel.deltaY = 0.5f;
        z.editor.mouseWheelMove (eventAt (z.editor, { x, (float) area.getCentreY() }, { x, (float) area.getCentreY() }, false, juce::ModifierKeys::commandModifier), wheel);
        const auto zoomedIn = z.editor.getZoom() > 9.0 * 1.9 && std::abs (z.editor.frameAt (x) - under) <= (juce::int64) (z.editor.visibleLength() / area.getWidth() + 1);

        // A note clicked in at the zoomed grid lands on its line.
        const auto line = z.editor.lineFrame (z.editor.frameAt (x) / z.editor.gridStepFrames() + 1);
        z.click (z.at (60, line) + juce::Point<float> (2.0f, 0.0f));
        const auto placed = z.notes().size() == 1 && z.notes()[0].start == line;

        // Sideways: the view moves along, and stops at the ends.
        const auto startedAt = z.editor.getViewStart();
        wheel.deltaY = 0.0f;
        wheel.deltaX = -0.5f;
        z.editor.mouseWheelMove (eventAt (z.editor, { x, (float) area.getCentreY() }, { x, (float) area.getCentreY() }), wheel);
        const auto movedAlong = z.editor.getViewStart() > startedAt;
        z.editor.setView (std::numeric_limits<juce::int64>::max() / 2, z.editor.getZoom());
        const auto atEnd = z.editor.getViewStart() + z.editor.visibleLength() == z.editor.wholeLength();

        // The - button halves it, Fit shows all; zoom stays within its limits.
        const auto was = z.editor.getZoom();
        z.editor.zoomAround ((float) area.getCentreX(), 0.5);
        const auto halved = std::abs (z.editor.getZoom() - was * 0.5) < 1e-9;
        z.editor.fitTime();
        const auto fitted = z.editor.getViewStart() == 0 && z.editor.visibleLength() == z.editor.wholeLength();
        z.editor.zoomAround (0.0f, 1.0e9);
        const auto limited = z.editor.visibleLength() >= (juce::int64) (0.05 * rate) - 1;

        // Option-scroll makes the rows taller.
        wheel.deltaX = 0.0f;
        wheel.deltaY = 0.5f;
        z.editor.mouseWheelMove (eventAt (z.editor, { x, (float) area.getCentreY() }, { x, (float) area.getCentreY() }, false, juce::ModifierKeys::altModifier), wheel);
        const auto taller = z.editor.getRowHeight() > NoteEditor::rowHeight
                            && std::abs (z.editor.noteBounds ({ 60, 0, 1, 1.0f }).getHeight() - (float) (z.editor.getRowHeight() - 2)) < 0.01f;

        check (opened && zoomedIn && placed && movedAlong && atEnd && halved && fitted && limited && taller,
               "piano roll: a repeating clip opens on its pattern; Cmd-scroll zooms around the mouse, a note clicked in lands on the zoomed grid's line, "
               "a sideways scroll moves along and stops at the end, - halves the zoom, Fit shows all, and Option-scroll makes the rows taller");
    }

    // Drawn in every theme without an unpainted spot.
    auto painted = true;

    for (const auto& name : Theme::themeNames())
    {
        Theme::apply (Theme::make (name, "Blue"));
        Rig look;
        look.shown.clip.loopLength = 96000;
        look.shown.clip.notes = { { 60, 0, 12000, 0.9f }, { 64, 12000, 12000, 0.6f }, { 67, 24000, 24000, 0.3f } };
        look.editor.refresh();
        const auto image = look.editor.createComponentSnapshot (look.editor.getLocalBounds());
        painted = painted && image.isValid() && image.getWidth() == 900;
    }

    check (painted, "piano roll: drawn in every theme");
}
