#pragma once

#include "FileChecks.h"
#include "Project.h"
#include "SessionPlace.h"

/** What was open, kept so the next start can continue from it (Settings > Start, "Continue where
    you left off"): the files open in the editor, each with its place, which one was shown, the
    effects rack beside the editor, the multitrack session by its file with where the view was in
    it, the project they belonged to, and whether the editor or the session was in front.

    It is a note of what was open, kept in the app's data folder; no audio is written. So it
    brings back files and a session as they are on disk: a file never saved, or changes not saved
    at quit, are not in it (quitting asks about those first). Undo histories aren't kept either. */
namespace Resume
{
    /** How often the main window notes what is open while it runs, so that a crash leaves a note
        no older than this. */
    constexpr int intervalSeconds = 30;

    struct State
    {
        Project::State open;            // no histories
        SessionPlace sessionPlace;
        juce::File project;             // the project they were opened from or saved as, or none

        /** Nothing to come back to: no file and no session. */
        bool isEmpty() const            { return open.files.empty() && open.session == juce::File(); }
    };

    /** What is open now. A file never saved is left out, and so is one opened from a session clip
        (it belongs to the clip). activePlace: where the editor is in the file shown. */
    State gather (const OpenFiles&, const FilePlace& activePlace, const AudioEngine&, bool multitrack,
                  const SessionPlace&, const juce::File& project);

    std::unique_ptr<juce::XmlElement> toXml (const State&);

    /** The note in folder, when there is one that can be read. Looks at no file but the note. */
    bool isKept (const juce::File& folder);

    /** Writes the note into folder (made if need be), replacing the last. Returns an error, or empty. */
    juce::String write (const State&, const juce::File& folder);
    void clear (const juce::File& folder);

    struct Loaded
    {
        bool found = false;                        // there was a note
        State state;                               // without what is missing
        juce::StringArray missing;                 // files, a session or a project no longer there
        std::vector<AudioFileIO::Loaded> audio;    // each of state.open.files, read
    };

    /** Reads the note and every file it names that is still there, on the calling thread (any
        thread: a file on a volume that doesn't answer holds up only this). keepGoing (if given) is
        asked before each file; false stops, and what is left is not read. */
    Loaded load (const juce::File& folder, const BackgroundLoad::Loader& = BackgroundLoad::readIfThere,
                 const std::function<bool()>& keepGoing = {});

    /** Opens what was loaded in place of every open file (ask about changes first), each with its
        video and place, the one that was shown in front. Gives its place, and adds what couldn't be
        read to loaded.missing. The session, rack and project are left to the caller. */
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

        /** Takes the state as it is now. Returns an error from writing, or empty. */
        juce::String note (const State&);

    private:
        juce::File folder;
        juce::String last;
        bool mayClear = false;
    };

    /** A mark that a restore is under way, so that a start can tell the last one didn't finish
        (the app closed while reopening) and not try the same again by itself. */
    void markStarted (const juce::File& folder);
    void markFinished (const juce::File& folder);
    bool wasInterrupted (const juce::File& folder);
}
