#pragma once

class AudioEngine;

/** Basic Drums: every sound, its settings, no clicks, the choke groups, stereo, the kits and old sessions.
    With levels, prints every note's peak and loudness instead. */
void runDrumMachineTests (AudioEngine&, bool levels = false);
