#pragma once

// Factory sounds renamed in 0.22.0, named for what they play rather than a scene: old name first.
// A session, preset, favourite or setting saved before then names the old one, and finds the new.

#include <JuceHeader.h>
#include <utility>

namespace FactoryRenames
{
    /** Basic Drums' kits. */
    inline constexpr std::pair<const char*, const char*> drumKits[] = {
        { "Dark Techno", "Low Rumble" },
        { "EBM Machine", "Iron Shutter" },
        { "Body Music", "Pressure Room" },
    };

    /** The patterns an instrument plays when a sound is chosen (Audition). */
    inline constexpr std::pair<const char*, const char*> auditionPatterns[] = {
        { "EBM Gallop", "Iron Gallop" },
        { "EBM Riff", "Signal Riff" },
        { "EBM", "Machine March" },
    };

    /** The sampler's factory samples, by their place among them. */
    inline constexpr std::pair<const char*, const char*> samplerFiles[] = {
        { "Sliced Kits/Dark Techno Break 130 BPM.flac", "Sliced Kits/Low Rumble Break 130 BPM.flac" },
        { "Sliced Kits/EBM Machine Break 120 BPM.flac", "Sliced Kits/Iron Shutter Break 120 BPM.flac" },
    };

    /** The sampler's factory presets. */
    inline constexpr std::pair<const char*, const char*> samplerPresets[] = {
        { "Dark Techno Break", "Low Rumble Break" },
        { "EBM Machine Break", "Iron Shutter Break" },
    };

    /** A name as it is now: the new one for an old one in `renames`, any other as it is. */
    template <size_t n>
    juce::String now (const std::pair<const char*, const char*> (&renames)[n], const juce::String& name)
    {
        for (auto& [before, after] : renames)
            if (name == before)
                return after;

        return name;
    }
}
