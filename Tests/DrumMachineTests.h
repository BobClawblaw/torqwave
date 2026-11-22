#pragma once

class AudioEngine;

/** Basic Drums: every sound, its settings, no clicks, the choke groups, stereo, the kits and old sessions.
    With levels, prints every note's peak and loudness instead. */
void runDrumMachineTests (AudioEngine&, bool levels = false);

/** Saves the window in the Dark and Light themes as drums-Dark.png and drums-Light.png in folder. */
void saveDrumMachineWindow (const juce::File& folder);

/** Renders a four-bar groove in each kit to "Basic Drums - <kit>.wav" in folder. */
void saveDrumMachineDemos (const juce::File& folder);
