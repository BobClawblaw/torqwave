#pragma once

#include "Session.h"

/** The Piano Roll's writing tools, on a clip's notes (frames of its timeline): keys and scales,
    chords from Roman numerals, the arpeggiator, humanize and varying a part. Each is a plain
    function of the notes, so it can be checked note for note; the random ones take a seed and
    give the same notes for it every time. */
namespace NoteTools
{
    /** A key: a root (0 is C) and the scale's steps from it. */
    struct Scale
    {
        int root = 0;
        juce::String mode = "major";
        std::vector<int> steps { 0, 2, 4, 5, 7, 9, 11 };

        bool contains (int pitch) const noexcept;

        /** The nearest pitch in the scale (the lower one on a tie). */
        int snap (int pitch) const noexcept;

        /** The pitch `degrees` scale steps from this one (snapped to the scale first). */
        int step (int pitch, int degrees) const noexcept;

        /** The pitch of a degree (1 is the root) above the root in an octave (4: middle C's). */
        int degree (int number, int octave) const noexcept;

        juce::String name() const;   // "A minor"

        bool operator== (const Scale& o) const noexcept { return root == o.root && steps == o.steps; }
    };

    /** The modes known, as parseKey reads them: major, minor, harmonic minor, melodic minor,
        dorian, phrygian, lydian, mixolydian, locrian, major pentatonic, minor pentatonic, blues. */
    juce::StringArray modes();

    /** A key as written: "A minor", "Bb major", "F# dorian", "Am", "C". Nothing when it isn't one. */
    std::optional<Scale> parseKey (const juce::String&);

    /** The major or minor key holding the most of the notes (weighted by length), the first
        notes' pitch class winning a tie; C major with no notes. */
    Scale guessKey (const std::vector<ClipNote>&);

    /** A chord: its pitches from the lowest, and how it was written. */
    struct Chord
    {
        std::vector<int> pitches;
        juce::String symbol;
    };

    /** A progression in Roman numerals in a key ("i VI III VII", "I-V-vi-IV", "ii7 V7 Imaj7",
        "bVII", "vii°", "V/sus4"...), each chord built on its degree of the scale, its quality from
        the case (upper major, lower minor) unless marked (°, dim, ø, +, aug, sus2, sus4), with 7,
        maj7, °7 or ø7 adding the seventh. Its root is in `octave` (4: from middle C's), the chord
        above it. Empty with an error when a chord can't be read or the scale hasn't seven notes. */
    std::vector<Chord> parseProgression (const juce::String&, const Scale&, int octave, juce::String& error);

    /** The chords one after another from `start`, each `each` frames long. */
    std::vector<ClipNote> chordNotes (const std::vector<Chord>&, juce::int64 start, juce::int64 each, float velocity);

    enum class ArpOrder { up, down, upDown, random };
    juce::StringArray arpOrderNames();   // in ArpOrder's order

    struct Arp
    {
        ArpOrder order = ArpOrder::up;
        int octaves = 1;               // 1 to 4: the chord, then an octave up and on
        juce::int64 step = 0;          // frames between notes
        double gate = 0.9;             // each note's length, as a share of a step
        juce::uint32 seed = 1;         // for random
    };

    /** The notes as chords (notes starting within half a step of the first of a group), each
        played as an arpeggio from its start to its last note's end, a note each step (a last
        step only with half a step left), at the chord's mean velocity. A chord of one note is
        repeated. */
    std::vector<ClipNote> arpeggiate (const std::vector<ClipNote>&, const Arp&);

    struct Humanize
    {
        juce::int64 timing = 0;        // frames: each start moves up to this far either way
        float velocity = 0.0f;         // each velocity changes by up to this share of itself either way
        juce::uint32 seed = 1;
    };

    /** The notes in `which` (all when empty), moved and changed by random amounts up to these,
        none before frame 0 or past `length`. */
    void humanize (std::vector<ClipNote>&, const std::set<int>& which, const Humanize&, juce::int64 length);

    /** The chosen notes' pitches moved into the scale (each to its nearest). */
    void fitToScale (std::vector<ClipNote>&, const std::set<int>& which, const Scale&);

    struct Vary
    {
        float velocity = 0.0f;         // each velocity changes by up to this share either way
        float pitch = 0.0f;            // the chance a note moves a step up or down the scale
        float density = 0.0f;          // below 0: the chance each note is left out; above: the chance an empty step gets a note
        juce::int64 step = 0;          // the grid density works on, in frames
        Scale scale;
        juce::uint32 seed = 1;
    };

    /** A variation of the notes in a timeline of `length` frames: velocities, pitches within the
        scale and density changed by random amounts up to these. A note added in an empty step
        is a softer copy of the note before it (the first note when none is before it), a step
        long at most. At least one note is kept. */
    std::vector<ClipNote> vary (const std::vector<ClipNote>&, const Vary&, juce::int64 length);
}
