#pragma once

#include <JuceHeader.h>

/** Reading Battery kit files: a kit built in memory comes back as it was written, in every layout
    this reads; the unpacker's every branch; unknown versions refused by name; cut, corrupted and
    hostile files refused without reading outside them; and a kit planned, placed and put on the
    pads. No real kit is used or kept. */
void runBatteryKitTests();

/** Reads kit files where they are (the names of files, one a line, in a text file given on the
    command line) and prints counts only: how many read, their cells, layers and samples found.
    For checking the reader against kits the user owns; nothing of them is kept. */
int runBatteryKitFiles (const juce::StringArray& files);
