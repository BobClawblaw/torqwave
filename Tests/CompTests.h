#pragma once

#include <JuceHeader.h>

/** Comping a clip's takes: playback against each take read by hand, crossfaded joins, the comp
    edits, the comp kept through moves, trims, splits, take changes and processing, a session
    file and undo, and Flatten Comp. */
void runCompTests (juce::AudioFormatManager&);
