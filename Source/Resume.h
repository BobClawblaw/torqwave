#pragma once

#include "FileChecks.h"
#include "Project.h"
#include "SessionPlace.h"

/** What was open, kept so the next start can continue from it (Settings > Start, "Continue where
    you left off"): the files open in the editor, each with its place, which one was shown, the
    effects rack beside the editor, the multitrack session by its file with where the view was in
    it, the project they belonged to, and whether the editor or the session was in front.

    It is a note of what was open, kept in the app's data folder; no audio is written. So it
    brings back files as they are on disk: a file never saved, or changes to a file not saved at
    quit, are not in it (quitting asks about those first). Undo histories aren't kept either.

    A session with changes not saved (one never saved among them) is kept whole beside the note,
    as Save Session would write it (tracks, clips, instruments and effects with their settings,
    tempo; its audio by the files' names, none of it copied), and comes back as it was: still not
    saved, under the file it had or none. */
namespace Resume
{
    /** How often the main window notes what is open while it runs, so that a crash leaves a note
        no older than this. */
    constexpr int intervalSeconds = 30;

    /** What was open and could not be reopened (moved, deleted, unreadable, or on a drive that isn't
        connected): kept in the note, each with its place, and tried again at every start until it
        is back, or is forgotten. */
    struct Waiting
    {
        std::vector<Project::OpenFile> files;
        juce::File session;
        SessionPlace sessionPlace;
        juce::File project;

        bool isEmpty() const            { return files.empty() && session == juce::File() && project == juce::File(); }
    };

    struct State
    {
        Project::State open;            // no histories
        SessionPlace sessionPlace;
        juce::File project;             // the project they were opened from or saved as, or none
        Waiting waiting;

        /** The session has changes not saved, so it is kept as a copy (sessionCopy()): open.session
            is then the file it would be saved to (which need not be there), or none. */
        bool sessionCopy = false;

        /** Nothing to come back to: no file and no session, open or waiting. */
        bool isEmpty() const            { return open.files.empty() && open.session == juce::File() && ! sessionCopy && waiting.isEmpty(); }
    };

    /** Where the copy of a session with unsaved changes is kept in folder. */
    juce::File sessionCopy (const juce::File& folder);

    /** Writes the session to the file given, as SessionFile::save does. Returns an error, or empty. */
    using SessionWriter = std::function<juce::String (const juce::File& target)>;

    /** What is open now, and of `waiting` what still is: a waiting file that has been opened since
        is no longer waiting, and a waiting session or project gives way to one that is open (a
        session with unsaved changes counts as open, saved or not). A file never saved is left out,
        and so is one opened from a session clip (it belongs to the clip).
        activePlace: where the editor is in the file shown. */
    State gather (const OpenFiles&, const FilePlace& activePlace, const AudioEngine&, bool multitrack,
                  const SessionPlace&, const juce::File& project, const Waiting& waiting = {});

    std::unique_ptr<juce::XmlElement> toXml (const State&);

    /** The note in folder, when there is one that can be read. Looks at no file but the note. */
    bool isKept (const juce::File& folder);

    /** Writes the note into folder (made if need be), replacing the last. Returns an error, or
        empty. The session's copy is not written here (Keeper does that). */
    juce::String write (const State&, const juce::File& folder);

    /** Removes the note and the session's copy. */
    void clear (const juce::File& folder);

    struct Loaded
    {
        bool found = false;                        // there was a note
        State state;                               // open: what can be opened; waiting: what can't, to keep
        juce::StringArray missing;                 // what can't be opened, to say
        std::vector<AudioFileIO::Loaded> audio;    // each of state.open.files, read
    };

    /** Reads the note and every file it names that is still there, on the calling thread (any
        thread: a file on a volume that doesn't answer holds up only this). What was waiting and is
        back joins the files to open, after the others; a waiting session or project is taken when
        the note has none of its own. What can't be read is moved to state.waiting. A session
        noted as kept in a copy is state.sessionCopy when the copy is there (the copy itself is
        read by whoever opens it); when it is gone that is said, and the session's own file is
        opened if it has one. keepGoing (if given) is asked before each file; false stops, and
        what is left is not read. */
    Loaded load (const juce::File& folder, const BackgroundLoad::Loader& = BackgroundLoad::readIfThere,
                 const std::function<bool()>& keepGoing = {});

    /** Opens what was loaded in place of every open file (ask about changes first), each with its
        video and place, the one that was shown in front. Gives its place. The session, rack and
        project are left to the caller. */
    FilePlace restoreFiles (Loaded&, OpenFiles&, AudioEngine&);

    /** Decides when the note is written while the app runs. A state the same as the last one
        written is not written again. An empty state (nothing open) removes the note only once what
        it held was brought back or replaced in this run: opening the app and closing it again
        leaves what was kept on offer. */
    class Keeper final
    {
    public:
        explicit Keeper (juce::File folder);

        /** What was kept has been brought back: from now on nothing open means nothing to keep. */
        void restored()                             { mayClear = true; }

        /** What the note holds is still from an earlier run: nothing has been brought back or
            opened in this one. */
        bool keepsEarlier() const                   { return ! mayClear; }

        /** Takes the state as it is now. Returns an error from writing, or empty.

            A state with sessionCopy has the session written first, by writeSession, beside the
            note: when sessionChanged says it has changed since the last call, or no copy has been
            written in this run. It is written under another name and then moved into place, so a
            copy is never half written. A state without it removes the copy once the note no
            longer speaks of it. */
        juce::String note (const State&, const SessionWriter& writeSession = {}, bool sessionChanged = true);

    private:
        juce::File folder;
        juce::String last;
        bool mayClear = false;
        bool copyWritten = false;
    };

    /** A mark that a restore is under way, so that a start can tell the last one didn't finish
        (the app closed while reopening) and not try the same again by itself. */
    void markStarted (const juce::File& folder);
    void markFinished (const juce::File& folder);
    bool wasInterrupted (const juce::File& folder);
}
