#pragma once

#include <JuceHeader.h>

/** Writes the sound demos (docs/sounds/audio) into the folder: Torqwave Synth patches and Swarm Guitar
    presets playing short parts at 120 BPM, as MP3s, each named for what it plays. The Sounds page
    (docs/sounds/index.html) lists them. Run with `TorqwaveTests --sound-demos <folder>`. */
void writeSoundDemos (const juce::File& folder);
