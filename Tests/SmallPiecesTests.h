#pragma once

class AudioEngine;

/** 0.19's smaller pieces: mixer scenes, channel-strip presets applied to a track, and hardware
    inserts pinged and compensated through a simulated loopback. */
void runSmallPiecesTests (AudioEngine&);
