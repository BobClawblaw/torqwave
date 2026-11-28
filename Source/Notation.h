#pragma once

#include <JuceHeader.h>
#include "Session.h"
#include "NoteTools.h"

/** Clean-room Western standard music notation helpers and MusicXML 3.1 exporter for NoteEditor:
    maps MIDI pitches to diatonic grand-staff positions (treble + bass staves, accidentals,
    key signatures, ledger lines), classifies note durations (whole through sixteenth + dotted),
    and exports standard score-partwise MusicXML 3.1 files. */
namespace Notation
{
    struct PitchSpelling
    {
        int diatonic = 28;     // C4 (middle C) = 28, D4 = 29, E4 = 30, ...
        int accidental = 0;    // -1 flat, 0 natural, +1 sharp
        char step = 'C';       // 'C'..'B'
        int octave = 4;        // MIDI octave (C4 = 60)
    };

    enum class DurationKind
    {
        whole,
        dottedHalf,
        half,
        dottedQuarter,
        quarter,
        eighth,
        sixteenth
    };

    /** Key signature fifths (-7 flats to +7 sharps) for a scale root and mode. */
    int keyFifths (const NoteTools::Scale& key) noexcept;

    /** Whether a key prefers flat spellings over sharp spellings. */
    bool prefersFlats (const NoteTools::Scale& key) noexcept;

    /** Spells a MIDI note (0..127) onto its diatonic step, accidental, letter, and octave. */
    PitchSpelling spell (int midiPitch, bool useFlats) noexcept;

    /** Converts a diatonic step back to its natural MIDI pitch (or key-altered pitch when in key). */
    int diatonicToMidi (int diatonic, const NoteTools::Scale& key) noexcept;

    /** Classifies a note length in beats into a standard rhythmic notation value. */
    DurationKind classifyDuration (double beats) noexcept;

    /** Grand-staff layout geometry derived from the visible editor area. */
    struct GrandStaff
    {
        float space = 8.0f;          // pixels between adjacent staff lines (half-space = one diatonic step)
        float trebleBottomY = 0.0f;  // E4 (diatonic 30)
        float bassTopY = 0.0f;       // A3 (diatonic 26)
        float middleCY = 0.0f;       // C4 (diatonic 28)

        static GrandStaff fit (juce::Rectangle<float> area) noexcept;

        /** Vertical pixel center for a diatonic step on the grand staff. */
        float yForDiatonic (int diatonic) const noexcept;

        /** Inverse mapping: diatonic step nearest to vertical coordinate y. */
        int diatonicAtY (float y) const noexcept;
    };

    /** Generates a standard MusicXML 3.1 (score-partwise) document for a NoteClip. */
    juce::String toMusicXml (const NoteClip& clip, double sampleRate, double beatSeconds,
                             int beatsPerBar, const NoteTools::Scale& key, const juce::String& title = {});

    /** Exports a NoteClip to a .musicxml file on disk. Returns an error message or empty string on success. */
    juce::String exportMusicXml (const NoteClip& clip, double sampleRate, double beatSeconds,
                                 int beatsPerBar, const NoteTools::Scale& key, const juce::File& file,
                                 const juce::String& title = {});
}
