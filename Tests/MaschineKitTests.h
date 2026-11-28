#pragma once

#include <JuceHeader.h>

/** Unit, round-trip, path-safety, truncation-fuzz, and clean-room synth/FX mapping tests
    for MaschineKit (.mxgrp / .mxsnd) import into Basic Drums. */
void runMaschineKitTests();

/** Reads every real .mxgrp / .mxsnd file listed, verifies parsing and sample resolution, and prints a summary. */
int runMaschineKitFiles (const juce::StringArray& files);
