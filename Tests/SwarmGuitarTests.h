#pragma once

#include <JuceHeader.h>

class AudioEngine;

/** StringSwarm, the built-in guitar with every string on one note. */
void runSwarmGuitarTests (AudioEngine&);

/** Writes its window, drawn offscreen at twice its size in the dark and light themes, into the folder. */
void saveSwarmGuitarWindow (const juce::File& folder);
