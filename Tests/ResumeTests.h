#pragma once

#include <JuceHeader.h>

void runResumeTests();

/** Makes something to continue from, for trying the app by hand: three audio files and a session
    in folder/Files, and in folder/Left Off the note of them open (the second file shown, the
    session in front, each with a place). Copied into the app's data folder, with Settings > Start
    on "Continue where you left off", the app starts with them. Returns whether it was made. */
bool makeResumeDemo (const juce::File& folder);
