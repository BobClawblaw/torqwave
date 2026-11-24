#pragma once

#include <JuceHeader.h>

/** The home screen the empty editor shows: buttons to start, and the recent files, sessions and
    projects as cards with a picture of each. An audio file's picture is its waveform; a session's
    (or a project's that was in its session) is its tracks with their clips; any other project's
    is the waveform of the file it showed. Pictures are drawn in the theme's colours from what is
    read, so they follow the theme. */
namespace Home
{
    enum class Kind { audio, session, project };
    Kind kindOf (const juce::File&);

    /** A session drawn small: its tracks (buses and folders left out), each with its clips. */
    struct Arrangement
    {
        struct Clip
        {
            double start = 0.0, end = 0.0;   // seconds
            int colour = -1;                 // a TrackColours palette index, or -1 for the track's
            bool notes = false;              // a note clip
        };

        struct Lane
        {
            int colour = -1;                 // the track's palette index, or -1 for its automatic one
            std::vector<Clip> clips;
        };

        std::vector<Lane> lanes;
        double length = 0.0;                 // seconds, to the end of the last clip
    };

    /** A .tqsession file's tracks and clips, read without its audio; nothing when it can't be read. */
    std::optional<Arrangement> arrangementOf (const juce::File& session);

    /** What a project shows on its card: its session when it was in the session or had no files,
        else the file it showed. */
    struct ProjectFace
    {
        juce::File session, audio;
    };

    ProjectFace faceOf (const juce::File& project);

    /** The pictures of recent items, made off the message thread and kept on disk (in the app's
        data folder, by each file's path, size and time) so a launch draws them at once. */
    class Pictures final : private juce::Thread,
                           private juce::AsyncUpdater
    {
    public:
        /** cache: the folder the pictures are kept in. */
        explicit Pictures (juce::File cache);
        ~Pictures() override;

        /** An audio file's waveform as `columns` pairs of the lowest and highest sample (all its
            channels together, -1 to 1), or nothing yet: it is then made, and onChange called on
            the message thread when it is ready. A file that can't be read has none. */
        std::shared_ptr<const std::vector<float>> waveform (const juce::File&);

        /** A session's or a project's arrangement, the same way (a project with no session in front
            has none). */
        std::shared_ptr<const Arrangement> arrangement (const juce::File&);

        std::function<void()> onChange;

        static constexpr int columns = 160;

        /** Makes a waveform now, on the calling thread (for the tests and the worker). */
        static std::vector<float> readWaveform (const juce::File&, juce::AudioFormatManager&);

    private:
        struct Entry
        {
            juce::String stamp;   // the file's size and modification time when it was made
            std::shared_ptr<const std::vector<float>> waveform;
            std::shared_ptr<const Arrangement> arrangement;
            bool failed = false;
        };

        juce::File cache;
        juce::CriticalSection lock;
        std::map<juce::String, Entry> entries;   // by full path
        juce::StringArray queue;
        juce::AudioFormatManager formats;

        static juce::String stampOf (const juce::File&);
        juce::File cacheFile (const juce::File&) const;
        const Entry* find (const juce::File&);
        void run() override;
        void handleAsyncUpdate() override;
    };

    /** What the app shows when it starts: the home screen (the default), the file that was open
        when it quit, or everything that was open, as it was left (see Resume). Kept in the settings
        under startKey as 0, 1 or 2. */
    enum class Start { homeScreen, lastFile, leftOff };
    inline constexpr const char* startKey = "onStart";
    Start startWith (const juce::PropertySet& settings);

    /** The file to reopen at start: the one kept under lastFileKey when the settings ask for the
        last file, otherwise none. */
    juce::File fileToReopen (const juce::PropertySet& settings, const juce::String& lastFileKey);

    /** Draws a card's picture: a waveform, an arrangement, or a plain mark for its kind while
        there is none yet. */
    void drawPicture (juce::Graphics&, juce::Rectangle<float> area, const juce::File&, Pictures&);
}
