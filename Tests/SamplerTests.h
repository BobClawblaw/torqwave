#pragma once

#include <JuceHeader.h>

class AudioEngine;

/** Torqwave Sampler, the built-in sampler. */
void runSamplerTests (AudioEngine&);

/** Writes its window, drawn offscreen at twice its size in the dark and light themes, playing keys and playing slices, into the folder. */
void saveSamplerWindow (const juce::File& folder);

/** Tries every sound in a folder (and the folders in it) in the sampler: as the first sound, in place of another, and sliced at its hits,
    printing what the sampler makes of each. Returns 0 when each that loads plays at its root, as loud either way. */
int trySamplerFolder (const juce::File& folder);
