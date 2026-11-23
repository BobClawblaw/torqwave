#pragma once

#include <JuceHeader.h>

/** Writes Torqwave Sampler's factory content into a folder (Data/Samples in the repository): notes
    of Torqwave Synth's patches and StringSwarm's presets, and two-bar loops of Basic Drums' kits,
    as FLAC files, with Presets.xml naming the presets made of them (their settings and zones,
    loops found, the loops cut at their hits). Everything is rendered here from Torqwave's own
    instruments, so it can be made again and nothing in it is anyone else's. Prints what it wrote. */
void writeSamplerContent (const juce::File& folder);
