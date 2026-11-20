#pragma once

#include <JuceHeader.h>
#include "Session.h"

/** The piano roll: the selected note clip's notes on a grid of pitch (a keyboard down the left,
    middle C marked) against time (the clip's length, a line at each repeat), edited by hand.

    A click on empty grid puts a note there (step entry): at the grid line at or before the mouse
    when snapping, as long as one grid step, at the velocity the slider shows. Dragging a note
    moves it in pitch and time (the time snapped), dragging its right edge changes its length;
    the selected notes move together; dragging the first repeat line sets the loop length. A
    click selects a note (Shift adds or takes it away), a
    drag on empty grid selects a box of them, Option-click removes one, Delete and Backspace
    remove the selected, Cmd-A selects all; the arrow keys move the selected notes a semitone
    or a grid step (Shift: an octave, or a quarter step). The velocity slider sets the selected
    notes'. A controller lane (the Lane choice: the mod wheel, the pitch bend or any CC) shows
    under the notes as a line over the timeline: a click puts a point there (snapped), dragging
    moves it, Option-click removes it; the lane's value is sent with the notes. The grid divides
    the beat at the clip's start by its choice, or is off. The time
    across is the notes' whole timeline: the part before the clip's start (its offset, after a
    trim or split) and the part past the pattern are washed out, as their notes don't play.

    It opens on the pattern's first repeat (the whole timeline when it doesn't repeat). Cmd-scroll
    or a pinch zooms the time around the mouse, the -, + and Fit buttons zoom it or show the whole
    timeline again, and the scroll bar under the notes (or a sideways or Shift-scroll) moves along
    it; Option-scroll makes the rows taller or shorter, and scrolling moves up and down the keys.

    The keyboard down the left plays: a click sounds its key until the mouse is let go, and a drag
    along it plays the keys it passes. A note sounds too while it is put in, clicked or dragged
    (at its new pitch as it moves).

    The Drums button shows the clip as a drum grid instead: a row for each sound of the track's
    drum map (General MIDI's unless the track has its own; notes outside the map get rows of
    their own at the bottom), a column for each grid step. A click on an empty step puts a note
    there, as long as a step, and a drag up or down from it sets its velocity; a click on a set
    step takes its notes away, a drag sets their velocity. The names down the left play their
    sound, and right-clicking one edits the map. Swing (on a grid of 1/2 or 1/4 beat) moves each
    pair's second line later, in both views, for the grid lines, snapping and the steps. Channel
    sets the MIDI channel the clip's notes and lanes go out on. Each change is handed back through `apply` as one undoable
    step; what sounds goes to `audition`. readClip gives what to show; the editor reads it ten
    times a second and redraws when it changed. */
class NoteEditor final : public juce::Component,
                         private juce::Timer,
                         private juce::ScrollBar::Listener
{
public:
    struct Shown
    {
        NoteClip clip;
        double sampleRate = 48000.0;
        double beatSeconds = 0.5;          // the session's beat at the clip's start
        juce::String trackName;
        bool hasInstrument = true;         // the track can play it
        bool drums = false;                // shown as a drum grid
        std::vector<DrumSound> drumMap = DrumSound::generalMidi();   // the track's sounds, top to bottom

        bool operator== (const Shown& o) const noexcept
        {
            return clip == o.clip && juce::exactlyEqual (sampleRate, o.sampleRate) && juce::exactlyEqual (beatSeconds, o.beatSeconds)
                && trackName == o.trackName && hasInstrument == o.hasInstrument && drums == o.drums && drumMap == o.drumMap;
        }
    };

    std::function<std::optional<Shown>()> readClip;
    std::function<void (const NoteClip&, const juce::String& description)> apply;
    std::function<void (int pitch, float velocity)> audition;   // a velocity of 0 ends the note
    std::function<void (bool drums)> showDrums;                  // the track's clips shown as a drum grid, or not
    std::function<void (const std::vector<DrumSound>&, const juce::String& description)> applyDrumMap;   // an empty map: General MIDI's

    NoteEditor();
    ~NoteEditor() override;

    /** Reads the clip again now (as the timer does). */
    void refresh();

    /** The clip as shown, edits under way included. */
    const std::optional<Shown>& getShown() const noexcept          { return shown; }
    const std::set<int>& getSelectedNotes() const noexcept         { return selected; }   // indices into the clip's notes

    /** Grid steps per beat: 1, 2, 4, 8, 16 or 32, or 0 for no grid. */
    void setGridDivision (int division);
    int getGridDivision() const noexcept                           { return gridDivision; }
    juce::int64 gridStepFrames() const noexcept;

    /** Swing on the grid, from 0.5 (straight) to NoteQuantize::maxSwing; it acts on a grid of 2 or 4 steps a beat. */
    void setSwing (double swing);
    double getSwing() const noexcept                               { return swing; }

    /** The grid's line k: its frame on the notes' timeline, swung. A drum grid's steps are its lines (a beat's quarters with the grid off). */
    juce::int64 lineFrame (juce::int64 k) const;

    /** The MIDI channel of the clip (1 to 16), set as one undoable step. */
    void setChannel (int channel);

    void setVelocity (float velocity);                             // for new notes, and the selected ones
    float getVelocity() const noexcept                             { return (float) velocitySlider.getValue() / 127.0f; }

    /** The note under a point (-1: none), and the point of a note's grid cell: pitch and frame. */
    int noteAt (juce::Point<float>) const;
    int pitchAt (float y) const;
    juce::int64 frameAt (float x) const;                           // frames of the notes' timeline, not snapped
    juce::int64 wholeLength() const noexcept;                      // the timeline: the clip's offset and length
    juce::Rectangle<float> noteBounds (const ClipNote&) const;
    juce::Rectangle<int> rollArea() const;

    /** The time shown: zoom is how many times the whole timeline the roll's width would hold
        (1: all of it, up to maxZoom), from viewStart, a frame of the notes' timeline. zoomAround
        keeps the frame under x where it is; fitTime shows all of it, showPattern the first repeat. */
    void zoomAround (float x, double factor);
    void setView (juce::int64 start, double zoom);
    void fitTime()                                                 { setView (0, 1.0); }
    void showPattern();
    double getZoom() const noexcept                                { return zoom; }
    double maxZoom() const noexcept;
    juce::int64 getViewStart() const noexcept                      { return viewStart; }
    juce::int64 visibleLength() const noexcept;

    /** The rows' height in pixels, from minRowHeight to maxRowHeight (rowHeight to begin with). */
    void setRowHeight (int pixels);
    int getRowHeight() const noexcept                              { return rowPixels; }

    /** The drum grid: whether it is shown, its rows (the map's sounds, then the clip's other
        notes), its steps, a cell's bounds, the cell under a point ({-1, -1}: none) and the note in
        a cell (-1: none). */
    bool showingDrums() const noexcept                             { return shown.has_value() && shown->drums; }
    std::vector<DrumSound> drumRows() const;
    int stepCount() const;
    juce::Rectangle<float> cellBounds (int row, int step) const;
    std::pair<int, int> cellAt (juce::Point<float>) const;
    int noteInCell (int row, int step) const;

    /** Changes the track's drum map as one undoable step (an empty map: General MIDI's). */
    void setDrumMap (const std::vector<DrumSound>&, const juce::String& description);

    void selectAll();
    void removeSelected();

    /** The controller lane shown under the notes (ControllerLane::pitchBend, a CC, or -1 for none). */
    void setLane (int controller);
    int getLane() const noexcept                                   { return laneController; }
    juce::Rectangle<int> laneArea() const;                         // empty without a lane
    int lanePointAt (juce::Point<float>) const;                    // a point of the shown lane, or -1
    float laneValueAt (float y) const;
    float yForLaneValue (float value) const;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMagnify (const juce::MouseEvent&, float scaleFactor) override;
    bool keyPressed (const juce::KeyPress&) override;

    static constexpr int keyboardWidth = 44, rowHeight = 12, topBar = 30, laneHeight = 72;
    static constexpr int drumNamesWidth = 130, drumRowHeight = 18;
    static constexpr int minRowHeight = 6, maxRowHeight = 32, scrollHeight = 10;

private:
    enum class Drag { none, move, resize, box, loop, lanePoint, keys, velocity, nothing };

    void timerCallback() override;
    void scrollBarMoved (juce::ScrollBar*, double newRangeStart) override;
    void commit (const juce::String& description);
    juce::int64 snapped (juce::int64 frame) const;
    int leftWidth() const noexcept;                // the keyboard's, or the drum names'
    juce::int64 straightStep() const noexcept;     // a step's frames without swing (the drum grid's with the grid off too)
    int drumDivision() const noexcept              { return gridDivision > 0 ? gridDivision : 4; }
    void paintRoll (juce::Graphics&, juce::Rectangle<int> area);
    void paintDrums (juce::Graphics&, juce::Rectangle<int> area);
    void drumMouseDown (const juce::MouseEvent&);
    void drumMapMenu (int row);
    void editDrumSound (int row, bool adding);
    float yOf (int pitch) const;
    float xOf (juce::int64 frame) const;
    void scrollToNotes();
    void moveSelected (int semitones, juce::int64 frames);
    void refreshControls();
    void sound (int pitch, float velocity);   // the note heard now, ending the one before
    void silence();

    std::optional<Shown> shown, base;      // what is shown, and as it was when a drag began
    std::set<int> selected;
    int gridDivision = 4;
    double swing = 0.5;
    int firstRow = 0;                      // the drum grid's top row shown
    bool addedNow = false;                 // the drum step under the mouse was set by this click
    int lowPitch = 48;                     // the bottom row shown
    juce::int64 lastLength = 0;            // the length new notes take without a grid
    double zoom = 1.0;                     // see zoomAround
    juce::int64 viewStart = 0;
    int rowPixels = rowHeight;

    Drag drag = Drag::none;
    int dragNote = -1;
    juce::Point<float> dragStart;
    juce::int64 dragFrame = 0;
    int dragPitch = 0;
    juce::Rectangle<float> box;
    bool dragged = false;
    int heard = -1;                        // the pitch sounding through `audition`, or -1

    juce::ComboBox gridBox, laneBox, swingBox, channelBox;
    juce::TextButton drumsButton { "Drums" };
    juce::TextButton zoomOutButton { "-" }, zoomInButton { "+" }, fitButton { "Fit" };
    juce::ScrollBar timeScroll { false };
    void updateScroll();
    juce::Label laneLabel;
    int laneController = -1;
    int dragPoint = -1;
    juce::Slider velocitySlider;
    juce::Label gridLabel, velocityLabel, status;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteEditor)
};
