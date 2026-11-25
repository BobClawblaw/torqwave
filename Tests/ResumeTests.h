#pragma once

#include <JuceHeader.h>

void runResumeTests();

/** Makes something to continue from, for trying the app by hand: three audio files and a session
    in folder/Files, and in folder/Left Off the note of them open (the second file shown, the
    session in front, each with a place). Copied into the app's data folder, with Settings > Start
    on "Continue where you left off", the app starts with them. Returns whether it was made.

    With unsavedSession the session was never saved and has a drum track besides (Basic Drums with
    a kit chosen, a bar of notes, 97 beats a minute): the note says so, and the session is kept
    beside it. */
bool makeResumeDemo (const juce::File& folder, bool unsavedSession = false);
