#pragma once

#include <JuceHeader.h>

/** Reading a Native Instruments Maschine 2/3 Group kit file (.mxgrp) or Sound file (.mxsnd)
    the user owns, so Basic Drums can load its pads, sample zones, clean-room mapped drum synth
    sounds, and clean-room mapped per-pad effects.

    The format is not documented; this is a clean-room reader written from inspecting the container
    layout. A kit wraps an uncompressed Boost portable binary archive ("serialization::archive")
    inside an NI hsin/DSIN container. Each Sound carries its mixer settings (level, pan), name,
    and a chain of Slots (Sampler, AudioModule, internal Drum Synths, and internal Effects).

    The file is untrusted input. Every length, count and offset is checked before use, sizes are
    capped, and sample paths are validated against directory traversal. Nothing here reads audio
    samples or copies files: it only describes where the kit points and what settings it carries. */
namespace MaschineKit
{
    constexpr size_t mostFileBytes = (size_t) 32 << 20;
    constexpr int mostPads = 64, mostZones = 256, mostModules = 32, mostParams = 128, mostNameLength = 1024;

    enum class ModuleKind
    {
        unknown = 0,
        sampler,
        audioModule,
        // Internal Maschine drum / bass synths (mapped to Basic Drums clean-room synthesized sources)
        kick,
        snare,
        hihat,
        tom,
        perc,
        cymbal,
        bassSynth,
        // Internal Maschine effects (mapped to Basic Drums clean-room per-pad & bus FX)
        eq,
        filter,
        saturator,
        distortion,        // MullhollandDrive
        compressor,
        maximizer,
        limiter,
        transientDesigner,
        lofi,
        beatDelay,
        reflex,
        grainDelay,
        metaverb,
        reverb,
        plateReverb,
        iceverb,
        cabinet,
        gate,
        chorus,
        flanger,
        phaser,
        freqShifter,
        fm,
        resochord,
        grainStretch,
        pluginHost
    };

    /** A sample reference inside a Sampler or AudioModule slot. */
    struct Zone
    {
        juce::String relativePath;     // e.g. "Samples/Drums/Kick/Kick VirtualLife.wav"
        juce::String basePath;         // library path stored by the kit author (optional hint)
        juce::String bankName;         // expansion / library display name
        int startFrame = 0;
        int endFrame = 0;
        int totalFrames = 0;
        int rootKey = 60;
        int lowKey = 0, highKey = 127;
        int lowVelocity = 0, highVelocity = 127;
        float gain = 0.75f;            // Maschine 0..1 gain scale (0.75 = 0 dB)
        float pan = 0.0f;              // -1..+1
        float tune = 0.0f;             // semitones
    };

    /** One module inside a Sound's (or Group's) Slot chain. */
    struct Module
    {
        ModuleKind kind = ModuleKind::unknown;
        juce::String typeName;         // e.g. "Sampler", "Kick", "EQ", "Saturator"
        bool active = true;
        std::vector<int> intParams;
        std::vector<float> floatParams;
        std::vector<Zone> zones;       // populated for Sampler / AudioModule

        // Sampler engine extras (read when kind == sampler)
        int samplerFilterMode = 0;     // 0 off, 1 LP, 2 BP, 3 HP
        float samplerCutoff = 0.5f;    // 0..1
        float samplerResonance = 0.0f; // 0..1
        float samplerPitch = 0.5f;     // 0..1 (0.5 = 0 st)
    };

    /** One Sound (pad) inside a Group (.mxgrp) or standalone Sound (.mxsnd). */
    struct Sound
    {
        int index = 0;                 // 0..15 in a Maschine Group
        juce::String name;
        float level = 0.75f;           // Maschine 0..1 gain scale (0.75 = 0 dB)
        float pan = 0.0f;              // -1 (left) to +1 (right)
        std::vector<Module> modules;   // Slot 0 (instrument) followed by insert FX slots
    };

    /** One MIDI note event extracted from a Maschine Group pattern (at 960 PPQN). */
    struct PatternNote
    {
        int soundIndex = 0;            // 0..15 pad index inside the Group
        double startBeat = 0.0;        // in quarter-note beats from pattern start
        double durationBeats = 0.25;   // in quarter-note beats
        int velocity = 100;            // 1..127
        int pitch = 60;                // original Maschine note pitch (60 = default pad trigger)
    };

    /** A pattern embedded inside a Maschine Group (.mxgrp). */
    struct Pattern
    {
        juce::String name;
        double lengthBeats = 16.0;     // e.g. 16.0 = 4 bars
        double bpm = 0.0;              // optional tempo hint parsed from pattern name (e.g. "2-126 Bpm")
        std::vector<PatternNote> notes;
    };

    struct Kit
    {
        juce::String name;             // from metadata or file name
        bool isGroup = true;           // true for .mxgrp, false for .mxsnd
        std::vector<Sound> sounds;     // up to 16 pads for a group, 1 for a sound
        std::vector<Module> groupFx;   // group-level insert FX (after pad 16)
        std::vector<Pattern> patterns; // MIDI patterns embedded inside the .mxgrp
    };

    /** Builds a standard Type-0 MIDI file (960 PPQN) for a Pattern, mapping each soundIndex
        through `soundToNote` (falling back to 36 + soundIndex). */
    juce::MidiFile buildMidiFile (const Pattern&, const std::map<int, int>& soundToNote, double fallbackBpm = 120.0);

    /** Writes a Pattern to a standard .mid file on disk. Returns an error message or empty on success. */
    juce::String exportPatternToMidi (const Pattern&, const std::map<int, int>& soundToNote, const juce::File& outFile, double fallbackBpm = 120.0);

    /** Reads a .mxgrp or .mxsnd from memory. Returns an error message to show, or empty on success. */
    juce::String read (const void* data, size_t size, Kit&);

    /** Reads a .mxgrp or .mxsnd file from disk. Returns an error message, or empty on success. */
    juce::String read (const juce::File&, Kit&);

    /** Resolves a Zone's sample path safely without copying files.
        Walks up from kitFile's folder (matching standard NI Expansion / Library layouts),
        then checks basePath + relativePath if present, then searches under `library` if given.
        Returns an empty juce::File if invalid (e.g. path traversal) or not found. */
    juce::File resolve (const Zone&, const juce::File& kitFile, const juce::File& library = {});

    /** Returns true if a relative path has only safe path components (no "..", no root/drive prefix). */
    bool isSafeRelativePath (const juce::String& relativePath);

    /** Human-readable name for a ModuleKind (for import reports). */
    juce::String kindName (ModuleKind);

    /** Builds a valid minimal .mxgrp or .mxsnd binary container in memory for round-trip and fuzz tests. */
    juce::MemoryBlock write (const Kit&);
}
