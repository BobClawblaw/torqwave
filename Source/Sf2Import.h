#pragma once

#include "Sampler.h"

/** Clean-room SoundFont 2 (.sf2) RIFF sfbk importer for Torqwave Sampler.
    Parses the standard SoundFont 2.01 specification (LIST INFO, LIST sdta -> smpl 16-bit PCM,
    LIST pdta -> phdr/pbag/pgen/inst/ibag/igen/shdr) with strict bounds checking and zero external
    dependencies. Extracts presets, key/velocity zones, root pitch, tuning, pan, attenuation,
    loop points, and PCM sample audio directly into Sampler::Zone records in memory. */
namespace Sf2Import
{
    constexpr size_t mostFileBytes = (size_t) 256 << 20;
    constexpr int mostZones = 256;

    struct PresetSummary
    {
        int bank = 0;
        int program = 0;
        juce::String name;
    };

    struct Result
    {
        juce::String bankName;
        juce::String presetName;
        std::vector<PresetSummary> presets;
        std::vector<Sampler::Zone> zones;
        juce::String error;

        bool ok() const noexcept { return error.isEmpty() && ! zones.empty(); }
    };

    /** Parses an SF2 file from memory and loads the chosen preset index (default 0 = first preset). */
    Result parse (const void* data, size_t size, int presetIndex = 0);

    /** Loads an SF2 file from disk into Sampler zones (default presetIndex = 0). */
    Result loadFile (const juce::File& sf2File, int presetIndex = 0);

    /** Lists all presets in an SF2 file without decoding PCM audio. */
    std::vector<PresetSummary> listPresets (const juce::File& sf2File, juce::String& error);

    /** Builds a minimal valid SoundFont 2 (.sf2) binary in memory for unit/round-trip/fuzz tests. */
    juce::MemoryBlock writeMinimal (const juce::String& presetName, const std::vector<int16_t>& pcm,
                                    int sampleRate = 44100, int rootKey = 60, int lowKey = 0, int highKey = 127,
                                    int lowVel = 1, int highVel = 127, bool looped = false,
                                    int loopStart = 0, int loopEnd = 0);

    /** Builds a multi-preset valid SoundFont 2 (.sf2) bank in memory for unit tests. */
    struct BankPresetSpec
    {
        juce::String name;
        int bank = 0, program = 0;
        int rootKey = 60, lowKey = 0, highKey = 127;
        std::vector<int16_t> pcm;
    };
    juce::MemoryBlock writeBank (const std::vector<BankPresetSpec>& presets, int sampleRate = 44100);
}
