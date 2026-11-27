#pragma once

#include "AudioDocument.h"
#include "NoteTools.h"
#include "Session.h"

/** Audio to MIDI extraction: turns a monophonic melody/bass recording into MIDI notes using
    PitchCorrection's YIN pitch tracker, or turns a drum loop / percussion recording into General MIDI
    (Basic Drums) drum notes and a reusable timing groove template using multi-band transient analysis. */
namespace AudioToMidi
{
    struct MelodyOptions
    {
        double minNoteMs = 45.0;          // notes shorter than this are dropped
        float minLevelDb = -48.0f;        // notes quieter than this are dropped
        std::optional<NoteTools::Scale> snapScale; // optional scale quantization
        double quantizeBeatSeconds = 0.0; // > 0 to snap starts to grid
        int quantizeDivision = 4;         // grid lines per beat (4 = 1/16)
    };

    struct DrumOptions
    {
        float sensitivity = 65.0f;        // 0..100: higher detects softer hits
        double minGapMs = 45.0;           // minimum spacing between hits
        bool classifyBands = true;        // true: Kick (36) / Snare (38) / Hat (42/46); false: all on singleNote
        int singleNote = 36;              // used when classifyBands == false
    };

    struct Result
    {
        std::vector<ClipNote> notes;
        /** Normalized fractional beat offsets of detected hits for groove quantization (empty if bpm <= 0). */
        std::vector<double> grooveOffsets;
        juce::String summary;
    };

    /** Extracts a monophonic melody or bassline from [start, end) of `audio` into ClipNotes
        (frames relative to `start`). */
    Result extractMelody (const AudioSnapshot& audio, juce::int64 start, juce::int64 end, double sampleRate,
                          const MelodyOptions& options = {}, const std::function<bool (float)>& progress = {});

    /** Extracts drum hits from [start, end) of `audio` into General MIDI / Basic Drums ClipNotes
        (Kick 36, Snare 38, Closed Hat 42, Open Hat 46), frames relative to `start`. */
    Result extractDrums (const AudioSnapshot& audio, juce::int64 start, juce::int64 end, double sampleRate,
                         const DrumOptions& options = {}, double bpm = 0.0,
                         const std::function<bool (float)>& progress = {});
}
