#pragma once

#include <JuceHeader.h>
#include "Sampler.h"

/** Clean-room reader for Akai MPC program files (.xpm — Drum and Keygroup programs).
    MPC .xpm files are standard XML (<MPCVObject><Program type="Drum"|"Keygroup">).
    Reads pad/keygroup instruments, multi-velocity layers, sample paths (resolved relative to
    the .xpm folder or its Samples subfolder, never copied), tuning, level, pan, start/end,
    reverse, mute/choke groups, filter, and volume envelope. */
namespace MpcKit
{
    constexpr size_t mostFileBytes = (size_t) 16 << 20;
    constexpr int mostInstruments = 128, mostLayers = 16;

    struct Layer
    {
        juce::String sampleName;       // <SampleName> (often without .wav)
        juce::String sampleFile;       // <SampleFile> (optional explicit file name)
        int lowVelocity = 0, highVelocity = 127;
        float volume = 1.0f;           // 0..1
        float pan = 0.5f;              // 0..1 (0.5 = center)
        float tuneCoarse = 0.0f;       // semitones
        float tuneFine = 0.0f;         // cents (-100..100)
        int rootNote = 60;
        int sliceStart = 0, sliceEnd = 0;
        bool reverse = false;
        bool loop = false;
        int loopStart = 0, loopEnd = 0;
    };

    struct Instrument
    {
        int number = 0;                // 1..128 (MPC pad / keygroup index)
        juce::String name;
        int lowNote = 36, highNote = 36;
        int muteGroup = 0;             // 0 = none, 1..32
        float volume = 0.707946f;      // linear gain (~0.708 = 0 dB in MPC)
        float pan = 0.5f;              // 0..1 (0.5 = center)
        float tuneCoarse = 0.0f;
        float tuneFine = 0.0f;
        int filterType = 0;            // 0 off, 1 LP, 2 BP, 3 HP
        float cutoff = 1.0f;           // 0..1
        float resonance = 0.0f;        // 0..1
        bool hasEnvelope = false;
        float attack = 0.0f, hold = 0.0f, decay = 0.5f, sustain = 1.0f, release = 0.05f;
        std::vector<Layer> layers;
    };

    struct Program
    {
        juce::String name;
        bool isKeygroup = false;       // false = Drum program, true = Keygroup program
        std::vector<Instrument> instruments;
    };

    /** Parses an MPC .xpm XML string. Returns an error message or empty on success. */
    juce::String parse (const juce::String& xmlText, Program&);

    /** Loads and parses an MPC .xpm file from disk. */
    juce::String read (const juce::File&, Program&);

    /** Resolves a Layer's sample file safely relative to the .xpm's folder (and sibling/child Samples
        folders, or `library` if provided). Rejects path traversal. */
    juce::File resolve (const Layer&, const juce::File& xpmFile, const juce::File& library = {});

    /** Converts an MPC Keygroup (or Drum) program into Torqwave Sampler zones. */
    struct SamplerResult
    {
        std::vector<Sampler::Zone> zones;
        int missingSamples = 0;
        juce::String error;
        bool ok() const noexcept { return error.isEmpty() && ! zones.empty(); }
    };

    SamplerResult toSamplerZones (const Program&, const juce::File& xpmFile, const juce::File& library = {});

    /** Writes a clean MPC .xpm XML document for unit/round-trip tests. */
    juce::String writeXml (const Program&);
}
