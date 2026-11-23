#pragma once

#include <JuceHeader.h>

class AudioEngine;

/** Torqwave Sampler, the built-in sampler. */
void runSamplerTests (AudioEngine&);

/** Writes its window, drawn offscreen at twice its size in the dark and light themes, playing keys and playing slices, into the folder. */
void saveSamplerWindow (const juce::File& folder);
