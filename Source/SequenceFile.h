#pragma once

#include "Session.h"

/** A sequence (.tqseq): a part to keep in the Sequence Library and drop into any session. Its
    notes and controller lanes are in beats (ticks, ticksPerBeat to a beat), not frames, so it
    lands on the same beats at any tempo; its voice is a track template of the track it came
    from (its instrument and effects, volume, pan and colour, without sends, output, input or
    automation). Nothing in it is in frames. */
namespace SequenceFile
{
    inline constexpr const char* extension = ".tqseq";
    inline constexpr const char* kitExtension = ".tqkit";   // a kit: sequences to play together (SequenceKit.h)
    inline constexpr int ticksPerBeat = 960;

    /** A sequence or a kit: what the library holds and a drop puts into a session. */
    inline bool isSequenceOrKit (const juce::File& f)   { return f.hasFileExtension (extension) || f.hasFileExtension (kitExtension); }

    struct Note
    {
        int pitch = 60;
        juce::int64 start = 0, length = 0;   // ticks
        float velocity = 0.8f;
        float probability = 1.0f;            // as ClipNote's

        bool operator== (const Note& o) const noexcept
        {
            return pitch == o.pitch && start == o.start && length == o.length && juce::exactlyEqual (velocity, o.velocity)
                && juce::exactlyEqual (probability, o.probability);
        }
    };

    struct LanePoint
    {
        juce::int64 tick = 0;
        float value = 0.0f;

        bool operator== (const LanePoint& o) const noexcept { return tick == o.tick && juce::exactlyEqual (value, o.value); }
    };

    struct Lane
    {
        int controller = 1;   // as ControllerLane
        std::vector<LanePoint> points;

        bool operator== (const Lane& o) const noexcept { return controller == o.controller && points == o.points; }
    };

    /** As a NoteClip, in ticks: the notes' timeline starts `offset` ticks before the sequence does,
        it plays `length` ticks, and with a loop length the notes repeat every that many ticks. */
    struct Sequence
    {
        juce::String name, role, subRole, key, note;   // key: a pitch name and scale, such as "A minor", or empty; note: its one-line description
        juce::String style;                            // a genre and a style, "Techno/Dark", or empty
        int energy = 0;                                // 1 (an intro tool) to 5 (peak), or 0 for none
        juce::StringArray character;                   // poles of the character pairs it has: Dark or Bright, Sparse or Dense, Straight or Swung, Clean or Distorted
        juce::String variationOf;                      // the name of the sequence it is a variation of, or empty
        juce::StringArray tags;
        int rating = 0;                                // 0 (none) to 5, set in the library
        bool favourite = false;
        juce::int64 added = 0;                         // when its file was first written, in ms since 1970; 0 until then
        double writtenTempo = 120.0;                   // for reference: it plays at the session's
        int beatsPerBar = 4;
        int channel = 1;
        juce::int64 offset = 0, length = 0, loopLength = 0;
        std::vector<Note> notes;
        std::vector<Lane> lanes;
        std::shared_ptr<const juce::XmlElement> voice;   // a TRACKTEMPLATE, or null to play on any instrument

        double bars() const noexcept   { return beatsPerBar > 0 ? (double) length / (double) (ticksPerBeat * beatsPerBar) : 0.0; }
    };

    /** The note clip with this id as a sequence, read through the session's tempo map from where
        it starts, named for the clip, with its track's voice made from `effects` (the track's
        EFFECTS, as TrackEffects::createXml gives them, or null). Null when there is no such clip. */
    std::optional<Sequence> fromClip (const SessionState&, juce::uint32 clipId, std::unique_ptr<juce::XmlElement> effects);

    /** The sequence as a note clip starting at `position` in this session, through its tempo map
        (so on the same beats at its tempo, through tempo changes too). Its id is left 0. */
    NoteClip toClip (const Sequence&, const SessionState&, juce::int64 position);

    /** The voice for a track: its template, without its sends, output, input or envelopes. */
    std::unique_ptr<juce::XmlElement> voiceOf (const SessionState&, juce::uint32 trackId, std::unique_ptr<juce::XmlElement> effects);

    /** A voice of an instrument alone, at its default settings: what a part made without a track
        (an imported MIDI file) plays on. */
    std::unique_ptr<juce::XmlElement> voiceFor (const juce::PluginDescription&);

    /** The name of the instrument its voice plays it on, or empty with none. */
    juce::String instrumentOf (const Sequence&);

    std::unique_ptr<juce::XmlElement> toXml (const Sequence&);

    /** Null when the XML is not a sequence. */
    std::optional<Sequence> fromXml (const juce::XmlElement&);

    /** The character pairs, each as its two poles ("Dark", "Bright"). */
    const std::vector<std::pair<juce::String, juce::String>>& characterPairs();

    /** A style's parts: "Techno/Dark" is { "Techno", "Dark" }; one with no genre is { "", style }. */
    std::pair<juce::String, juce::String> splitStyle (const juce::String&);

    /** How a style shows: "Techno › Dark". */
    juce::String styleName (const juce::String&);

    /** The sequence moved into a session's key: every note by the interval between its key's root
        and the session's (the nearer way, -6 to +5 semitones), its key named for the new root in
        its own scale. Drums (on channel 10, or in the Drums role), a sequence with no key and a
        session with none are left as they are. */
    Sequence transposedTo (const Sequence&, const juce::String& sessionKey);

    /** Writes it safely (to a temporary file, then in place). A new file (no file there yet), or
        one with no `added` time, is stamped with the time now; one rewritten keeps its time.
        Returns an error, or empty. */
    juce::String save (const Sequence&, const juce::File&);

    /** Writes XML safely, as save does (a sequence, a kit). Returns an error, or empty. */
    juce::String writeXml (const juce::XmlElement&, const juce::File&);

    /** Reads one, or gives an error. One saved before sequences kept their `added` time takes the
        file's creation time. */
    std::optional<Sequence> load (const juce::File&, juce::String& error);

    /** Where a dropped sequence went. */
    struct Placed
    {
        juce::uint32 trackId = 0, clipId = 0;
        std::shared_ptr<juce::XmlElement> effects;   // the new track's chain to load, or null
    };

    /** Puts a sequence into a session at `position`: on the track at `trackIndex` when `onTrack`
        and it is an instrument track (its notes only, played by its instrument), else on a new
        instrument track made from the voice (or a plain one with none), named for the sequence,
        at the end of the session. clipId is 0 when nothing could be added. */
    Placed place (SessionState&, const Sequence&, int trackIndex, juce::int64 position, bool onTrack);

    /** A new sequence to write: an instrument track named `name` at the end of the session, with
        an empty note clip of that name `bars` bars long, starting on the bar at or before
        `position`. effects is left null: the caller loads the instrument. */
    Placed blank (SessionState&, const juce::String& name, int bars, juce::int64 position);
}
