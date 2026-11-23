#pragma once

#include <juce_core/juce_core.h>

/** Decrypt on recordings that are not the synth's own: each file given (a note, read as the window
    reads a dropped file), a few of the system's alert sounds where there are any, and a factory patch
    played through its own effects (which Decrypt matches dry). For each: the target, the factory
    stage's best, the search until settled with its lock after each round, the rounds and time, and
    where the best still differs. For finding out why a search settles where it does. */
int runDecryptReal (const juce::StringArray& files);
