#pragma once

#include "SequenceFile.h"

/** Sequences as Standard MIDI Files (.mid), at the sequence's own resolution (960 ticks a beat),
    so a note keeps its ticks exactly through a file and back. */
namespace SequenceMidi
{
    /** The notes as the sequence plays them, in ticks from its start: each pass of its notes
        (every loop length, or once), the notes starting inside it, cut at its end. */
    std::vector<SequenceFile::Note> playedNotes (const SequenceFile::Sequence&);

    /** A type 1 file at 960 ticks a beat: a first track with the first sequence's written tempo,
        beats in a bar and key (when it is major or minor), then a track for each sequence,
        named for it, with its notes as they play (playedNotes) and its controller lanes, on its
        channel. */
    juce::MidiFile toMidiFile (const std::vector<SequenceFile::Sequence>&);

    /** Writes toMidiFile's file. Returns an error, or empty. */
    juce::String exportFile (const std::vector<SequenceFile::Sequence>&, const juce::File&);

    struct Imported
    {
        std::vector<SequenceFile::Sequence> sequences;
        juce::String error;
    };

    /** A file's tracks as sequences, one for each track with notes: named for the track (or, with
        none, for the file, numbered when there are several), its notes and controllers in ticks
        from the file's start, as long as whole bars to its last note's end, at the file's first
        tempo, time signature and key, on its notes' channel. An error for a file that can't be
        read or has no notes. */
    Imported read (const juce::MidiFile&, const juce::String& fileName);
    Imported importFile (const juce::File&);
}
