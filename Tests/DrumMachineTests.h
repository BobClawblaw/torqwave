#pragma once

class AudioEngine;

/** Basic Drums: every sound, its settings, no clicks, the choke groups, stereo, the kits and old sessions.
    With levels, prints every note's peak and loudness instead. */
void runDrumMachineTests (AudioEngine&, bool levels = false);

/** Saves the window in the Dark and Light themes as drums-Dark.png and drums-Light.png in folder. */
void saveDrumMachineWindow (const juce::File& folder);
