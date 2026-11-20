#pragma once

#include "SequenceFile.h"

/** The idea board (.tqboard): a grid of sequences, parts down (rows: drums, bass, pads...) and
    scenes across (columns: verse, chorus...), to launch on the bar and hear what works together,
    then lay out in the session as arranger sections. A cell names a sequence file; the board
    holds no notes of its own. */
class IdeaBoard
{
public:
    static constexpr int maxParts = 8, maxScenes = 8;
    static constexpr const char* extension = ".tqboard";

    juce::String name { "Board" };
    juce::StringArray parts { "Part 1", "Part 2", "Part 3", "Part 4" };
    juce::StringArray scenes { "Scene 1", "Scene 2", "Scene 3", "Scene 4" };
    double tempo = 120.0;                 // its own tempo, when not the session's
    bool sessionTempo = true;

    using Cell = std::pair<int, int>;     // part, scene
    std::map<Cell, juce::File> cells;

    juce::File fileAt (int part, int scene) const;
    void set (int part, int scene, const juce::File&);   // an empty file clears it

    /** Adds a part or scene at the end (false at the most), or removes one, moving the cells after it. */
    bool addPart (const juce::String&);
    bool addScene (const juce::String&);
    void removePart (int);
    void removeScene (int);

    /** The folder boards are kept in: Boards beside the Sequence Library's folder. */
    static juce::File defaultFolder();

    /** The board as XML; cells under `library` are written relative to it, so a library moved
        with its boards still opens. */
    std::unique_ptr<juce::XmlElement> toXml (const juce::File& library) const;
    static std::optional<IdeaBoard> fromXml (const juce::XmlElement&, const juce::File& library);
    juce::String save (const juce::File&, const juce::File& library) const;
    static std::optional<IdeaBoard> load (const juce::File&, const juce::File& library, juce::String& error);

    /** A sequence as it repeats on the board and in a section: from its start (notes before an
        offset left out) with its own loop, or the whole of it repeated. */
    static SequenceFile::Sequence asLoop (SequenceFile::Sequence);

    /** What Send to Session made. */
    struct Sent
    {
        int tracks = 0, clips = 0, sections = 0;
        std::map<juce::uint32, std::shared_ptr<juce::XmlElement>> effects;   // the new tracks' chains to load
        juce::StringArray missing;                                           // cells whose file couldn't be read
    };

    /** Lays the scenes out one after another as arranger sections from the bar at or after `at`
        (empty scenes left out), each as many whole bars as its longest sequence, named for the
        scene. Each part becomes a track made from its first sequence's voice (another track
        beside it for a sequence with another voice), named for the part, with each sequence as a
        clip filling its scene's section, repeating. */
    static Sent sendToSession (const IdeaBoard&, SessionState&, juce::int64 at,
                               const std::function<std::optional<SequenceFile::Sequence> (const juce::File&)>& read);
};

//==============================================================================
/** The board's launches, kept on the message thread: which cell of each part plays from which
    frame, on the board's own clock (frames from when it started), each starting and stopping on
    a bar line. */
class IdeaBoardLauncher
{
public:
    struct Launch
    {
        juce::int64 start = 0, stop = -1;   // stop: -1 until it is stopped
    };

    /** The first bar line at least `margin` frames after `position` (the bar line itself when on one and margin is 0). */
    static juce::int64 nextBar (const Metronome::TempoMap&, juce::int64 position, juce::int64 margin);

    /** Launches a cell: at once from 0 when nothing plays (returns true: the clock starts),
        otherwise from the next bar, the part's other cell stopping there. */
    bool launch (IdeaBoard::Cell, const Metronome::TempoMap&, juce::int64 position, bool playing);

    /** Stops a part's cell on the next bar. */
    void stopPart (int part, const Metronome::TempoMap&, juce::int64 position);

    /** Launches a scene: each part's cell in it, and the parts with none there stop. */
    bool launchScene (int scene, const IdeaBoard&, const Metronome::TempoMap&, juce::int64 position, bool playing);

    void clear()                                             { launched.clear(); }

    /** Forgets launches that have ended by `position`. */
    void tidy (juce::int64 position);

    enum class State { idle, queued, playing, stopping };
    State stateOf (IdeaBoard::Cell, juce::int64 position) const;

    const std::map<IdeaBoard::Cell, Launch>& getLaunched() const noexcept { return launched; }

    /** The board's session with the launched cells playing: on each cell's track (from `tracks`)
        its sequence (from `sequences`, as IdeaBoard::asLoop has it) as a clip from its launch,
        repeating until it stops (or for hours). Cells with no track or sequence are left out. */
    SessionState stateFor (const SessionState& base, const std::map<IdeaBoard::Cell, juce::uint32>& tracks,
                           const std::map<IdeaBoard::Cell, SequenceFile::Sequence>& sequences) const;

    /** The frames a launch is ahead of the playhead at least, so it isn't late for the audio being made. */
    static constexpr juce::int64 margin = 4096;

private:
    std::map<IdeaBoard::Cell, Launch> launched;
};
