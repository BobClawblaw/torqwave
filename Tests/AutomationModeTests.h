#pragma once

#include <JuceHeader.h>

class AudioEngine;

/** Automation modes (Off, Read, Touch, Latch and Write) over scripted passes, several tracks
    writing at once, the mixer's controls writing, and VCA faders in the mix and the file. */
void runAutomationModeTests (AudioEngine&);
