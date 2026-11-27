#pragma once

#include "Sampler.h"

/** Imports standard SFZ (.sfz) multisample instruments into Torqwave Sampler zones, supporting
    <control> (default_path), <global>, <group>, <region>, key/velocity ranges, pitch_keycenter,
    tune, transpose, volume, pan, offset/end, loop_mode/loop_start/loop_end, choke groups
    (group/off_by), round robin (seq_length/seq_position), and amp envelope opcodes. */
namespace SfzImport
{
    struct Result
    {
        std::vector<Sampler::Zone> zones;
        Sampler::Settings settings;
        bool hasEnvelope = false;
        int missingSamples = 0;
        juce::String error;

        bool ok() const noexcept { return error.isEmpty() && ! zones.empty(); }
    };

    /** Parses an SFZ note token (MIDI integer "60" or note name "c4", "f#2", "bb3") -> 0..127, or -1. */
    int parseNote (const juce::String& token) noexcept;

    /** Parses SFZ text with sample paths resolved relative to `baseFolder`. */
    Result parse (const juce::String& sfzText, const juce::File& baseFolder);

    /** Loads an SFZ file from disk into Sampler zones. */
    Result loadFile (const juce::File& sfzFile);
}
