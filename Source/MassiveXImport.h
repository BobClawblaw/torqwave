#pragma once

#include <JuceHeader.h>
#include "WaveSynth.h"

/** Clean-room Native Instruments Massive X (.nksf) preset importer for Torqwave Synth (WaveSynth).

    Reads standard unencrypted RIFF/NIKS containers (NISI metadata, PLID plug-in ID, and
    zlib-compressed MessagePack PCHK parameter trees) and translates oscillators, warp modes,
    auxiliary PM oscillators, noise, filters, envelopes, LFOs/Performers (onto WaveSynth's
    16-node MSEG curves), modulation matrix routes, macros, voice/unison settings, and effects
    onto Torqwave Synth's own built-in DSP engine.

    Zero Native Instruments audio assets, wavetables, or presets are bundled or redistributed.
    If the user has a legally installed local Massive X wavetable folder on their machine,
    local .mxwt files are read in place into WaveSynth's Sampled wavetable slots; otherwise
    Torqwave maps oscillators onto its own built-in wavetables. */
namespace MassiveXImport
{
    struct PresetInfo
    {
        juce::String name;
        juce::String author;
        juce::String bank;
        juce::String category;
        juce::String comment;
        juce::StringArray types;
        juce::StringArray characters;
    };

    struct ImportResult
    {
        bool ok = false;
        juce::String error;
        PresetInfo info;
        EffectPresets::Values values;
        std::array<juce::String, WaveSynth::numMacros> macroNames;
        std::array<std::shared_ptr<const WaveTables::Sampled>, WaveSynth::numOscillators> sampledTables;
        int oscsMapped = 0;
        int modsMapped = 0;
        int macrosMapped = 0;
        int fxMapped = 0;
        int localWavetablesLoaded = 0;
        juce::String summary;
    };

    /** Returns the user's local Massive X Wavetables folder if installed on this machine,
        or an empty File otherwise. Never downloads or copies anything. */
    juce::File defaultLocalWavetablesFolder();

    /** Quick header check: returns true if the file is an RIFF NIKS container for Massive X. */
    bool isMassiveXPreset (const juce::File& file);

    /** Parses and translates a Massive X .nksf file from disk into a WaveSynth patch.
        If localWavetablesFolder is empty, defaultLocalWavetablesFolder() is checked in place. */
    ImportResult importFile (const juce::File& nksfFile, const juce::File& localWavetablesFolder = {});

    /** Parses and translates a Massive X .nksf buffer from memory (bounds-checked; safe on malformed input). */
    ImportResult importMemory (const void* data, size_t size, const juce::File& localWavetablesFolder = {});

    /** Applies an imported Massive X patch to a live Torqwave Synth processor (message thread). */
    bool applyToSynth (BuiltInEffects::BasicSynthProcessor& synth, const ImportResult& imported);
}
