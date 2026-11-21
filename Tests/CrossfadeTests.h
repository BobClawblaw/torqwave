#pragma once

class AudioEngine;

/** Crossfades in a session: finding them, their length, join and shapes, removing them, crossfading
    clips that touch or overlap, the level across them, and session files. */
void runCrossfadeTests (AudioEngine&);
