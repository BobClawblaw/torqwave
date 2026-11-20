#pragma once

#include "Session.h"
#include "Ffmpeg.h"

/** Working to picture: the timecode a video gives a session, the markers Find Cuts and Mark Hit
    add, the tempo that puts chosen hits on the beat (Fit Tempo to Hits), and the sync point a
    sound is spotted by when it is dropped on a marker. */
namespace Picture
{
    /** The timecode a video gives a session: its rate from the picture's frame rate (drop frame
        when the video's own timecode is counted so, with a semicolon), and the start that has the
        session show the video's own timecode on its first frame, where the video starts in the
        session; keepStart where the video has no timecode. Nothing when the frame rate isn't one
        that timecode counts. */
    std::optional<TimeFormat::Timecode> timecodeOf (const Ffmpeg::Picture&, double videoStart, juce::int64 keepStart = 0);

    /** How strongly the picture must change for Find Cuts to call it a cut (ffmpeg's scene score). */
    constexpr double cutThreshold = 0.3;

    /** What the hits are put on: the beats of one tempo, its bars, or the bar lines of a tempo
        map, each hit on the first beat of a bar. */
    enum class LandOn { beats, bars, barLines };

    /** A tempo, or a tempo map, for some hits, and how far off each lands. */
    struct Fit
    {
        double tempo = 0.0;                            // from the start; 0 when nothing was fitted
        int beatsPerBar = 4;                           // from the start
        std::vector<Metronome::TempoChange> changes;   // barLines only
        std::vector<juce::int64> misses;               // frames each hit lands after (+) or before (-) its beat or bar, in the hits' order
        int within = 0;                                // how many land within the tolerance

        bool isValid() const noexcept                  { return tempo > 0.0; }
    };

    /** The tempo that puts the hits (session frames, sorted) closest to its beats or bars:
        - beats or bars: the one tempo from minTempo to maxTempo, from the session's start, that
          lands the most hits within tolerance frames of a beat (or a bar's first beat), the
          farthest one as near as it can be among those;
        - barLines: a tempo map with each hit on a bar line, the beats between two hits as near
          the tempo that fits best on beats as a whole number of beats allows, any beats left over
          from whole bars of beatsPerBar going in a shorter bar before the hit (the first bar is
          such a pickup when the first hit comes before a whole bar).
        Tempos stay from 20 to 400; a hit closer to the one before than that allows is left where
        it lands. Nothing (tempo 0) without hits or a range in 20 to 400. */
    Fit fitTempo (const std::vector<juce::int64>& hits, double sampleRate, int beatsPerBar, LandOn,
                  double minTempo, double maxTempo, juce::int64 tolerance);

    /** How far each hit lands from the nearest beat (or bar line) of a tempo map, signed. */
    std::vector<juce::int64> missesOf (const Metronome::TempoMap&, const std::vector<juce::int64>& hits, LandOn);

    /** The frame of a sound that is put on a marker when it is spotted there: its cue point named
        "Sync" (any case, or holding the word), else its first marker, else its loudest sample. */
    juce::int64 syncPoint (const Markers& fileMarkers, const AudioSnapshot&);

    /** Spots sound files on a marker: each is read (and converted to the session's rate) and
        added with its sync point at the marker, the first on the track (or a new one for -1) and
        the rest on new tracks. Returns a line for each file that couldn't be read or placed;
        `added` counts the clips added. */
    juce::StringArray spotFiles (SessionState&, const juce::StringArray& files, int track, juce::int64 marker,
                                 juce::AudioFormatManager&, int& added);

    /** A marker name not used yet in a numbered series: "Cut 01", "Hit 02"... */
    juce::String nextName (const Markers&, const juce::String& base);
}

namespace SessionEdits
{
    /** Adds a point marker, "Cut 01" on, at each time (seconds into the video, from where it
        starts in the session), unless a point marker is already within half a frame of it (of the
        video's frame rate, or 25 fps when that is 0), so finding cuts again adds none twice.
        Returns how many were added. */
    int addCutMarkers (SessionState&, const std::vector<double>& videoSeconds, double frameRate);

    /** Adds a point marker named "Hit 01" on, at a frame. */
    void addHitMarker (SessionState&, juce::int64 frame);

    /** Sets the session's timecode (a rate that isn't SMPTE: none). False when it is so already. */
    bool setTimecode (SessionState&, TimeFormat::Timecode);

    /** Gives the session a fitted tempo (one tempo with no changes, or its tempo map), keeping
        the clips on musical tracks on their bars and beats. False for no fit or no change. */
    bool applyTempoFit (SessionState&, const Picture::Fit&);

    /** Adds a clip of a sound (as addClip does: on the track, or a new one for -1) placed so that
        its frame `sync` plays at `marker`; when that would start it before the session's start, it
        starts there with its head left out. Returns its id, or 0 when nothing of it would be left. */
    juce::uint32 spotClip (SessionState&, int track, const juce::File& source, AudioDocument::Snapshot, juce::int64 marker, juce::int64 sync);
}
