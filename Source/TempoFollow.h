#pragma once

#include "Session.h"

/** Audio clips that follow the tempo (SessionClip::TempoFollow).

    A following clip has its own tempo (beats a minute) and, optionally, warp markers that pin
    frames of its original audio to beats. Together they say on which beat each frame of the
    original plays, counted from the beat where its first frame plays (the anchor): straight
    between the first frame (beat 0) and the markers, and at the clip's tempo past the last. The
    session's tempo map then says when each of those beats plays, so the clip's audio is the
    original stretched along that map (TimeStretch::stretchAlong), its frame 0 at the anchor.

    The clip's offset and length are frames of that stretched audio, as for any clip; its anchor
    is where the stretched audio's first frame would play, `position - offset`. Edits that change
    how it is stretched (its tempo, its markers, the tempo map) keep where it starts, in beats, and
    which part of the original it plays, and leave it to be stretched again (Renderer). */
namespace TempoFollow
{
    using Warp = SessionClip::WarpMarker;

    /** Beats from the anchor on which a frame of the original plays (fractional frames between). */
    double beatOf (const SessionClip::TempoFollow&, double originalFrame, double sampleRate);

    /** The frame of the original that plays a beat from the anchor. */
    double frameOf (const SessionClip::TempoFollow&, double beat, double sampleRate);

    /** The beat (on the session's map) where the clip's original starts: its anchor. */
    double anchorBeat (const SessionState&, const SessionClip&);

    /** The part of the original a clip plays, as frames of the original (identity for a clip that
        doesn't follow), and its gain envelope's points likewise, under the session's map now. */
    struct Span
    {
        double start = 0.0, end = 0.0;
        std::vector<double> gainFrames;
    };

    Span spanOf (const SessionState&, const SessionClip&);

    /** Puts a following clip (its tempo and markers as they now are) so the span starts on
        `startBeat` of the session's map: its position, offset, length and gain envelope's frames.
        Its audio is left as it was, to be stretched again. */
    void place (const SessionState&, SessionClip&, const Span&, double startBeat);

    /** What a following clip's audio is stretched for: its original, tempo, markers and the
        tempo map over it, from its anchor. Clips with the same key have the same stretched audio. */
    juce::String key (const SessionState&, const SessionClip&);

    /** True when a following clip's audio isn't stretched for what it is now. */
    bool isStale (const SessionState&, const SessionClip&);

    /** The clip's original stretched along the session's map from its anchor; null when progress
        stops it, or the clip doesn't follow. */
    AudioDocument::Snapshot render (const SessionState&, const SessionClip&, const std::function<bool (float)>& progress = {});

    /** Every stale clip in the state stretched now, on this thread (for a mixdown, or a test).
        False when progress stopped it. */
    bool renderAll (SessionState&, const std::function<bool (float)>& progress = {});

    /** A tempo in a file's name, "loop 120bpm.wav", "drums_92_BPM" or "beat-128-Am", between 40
        and 300; else nothing. */
    std::optional<double> tempoFromName (const juce::String&);

    /** The frames of the attacks in [from, to) of the audio (its first channels, mixed). */
    std::vector<juce::int64> transients (const AudioSnapshot&, juce::int64 from, juce::int64 to, double sampleRate);

    /** A tempo, 70 to 180 beats a minute, that the attacks in the audio keep to (from the gaps
        between them); nothing when there are too few, or they keep to none. */
    std::optional<double> detectTempo (const AudioSnapshot&, juce::int64 from, juce::int64 to, double sampleRate);

    /** Keeps re-stretching the session's following clips as they go stale, on a thread of its own,
        and puts each into the session in place (not an undo step) once it is done: what was stretched
        recently is kept, so an undo is in place at once. Message thread, but the work. */
    class Renderer final : private juce::ChangeListener,
                           private juce::Thread
    {
    public:
        explicit Renderer (Session&);
        ~Renderer() override;

        /** True while clips wait to be stretched. */
        bool isBusy() const;

        /** Told on the message thread when it starts or finishes stretching. */
        std::function<void()> onBusyChanged;

        /** True when no clip waits to be stretched. */
        bool isUpToDate() const;

        /** Calls back (message thread) once no clip waits to be stretched: at once if none does.
            For what must hear every clip as it should (a mixdown, a bounce). */
        void whenDone (std::function<void()>);

    private:
        void changeListenerCallback (juce::ChangeBroadcaster*) override;
        void run() override;
        void look();                       // finds stale clips: from the kept ones at once, else for the thread
        void setBusy (bool);

        Session& session;

        struct Job
        {
            std::shared_ptr<const SessionState> state;   // as it was when the clip was found stale
            juce::uint32 clipId = 0;
            juce::String key;
        };

        juce::CriticalSection lock;
        std::vector<Job> jobs;                         // under the lock: waiting
        std::set<juce::String> wanted;                 // under the lock: every stale clip's key now
        juce::String working;                          // under the lock: the key being stretched
        std::vector<std::pair<juce::String, AudioDocument::Snapshot>> kept;   // message thread: newest last
        bool busy = false;
        std::vector<std::function<void()>> waiting;   // message thread: whenDone's
        juce::WaitableEvent work;

        JUCE_DECLARE_WEAK_REFERENCEABLE (Renderer)
    };
}
