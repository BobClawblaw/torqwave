#pragma once

#include <JuceHeader.h>

/** Tests for 0.24.0 features: Akai MPC (.xpm) Drum & Keygroup import, SoundFont 2 (.sf2) import,
    Frequency Shifter (Hilbert SSB + Ring Mod), Shimmer Delay (granular pitch-shifted feedback),
    Vocoder, Spectral Freeze, Grand-Staff Score / Notation window, and Zero Telemetry invariant. */
void runRelease024Tests();

/** Renders @2x retina guide screenshots for 0.24.0 (guide-score.png) into <folder>. */
void saveRelease024Pictures (const juce::File& folder);
