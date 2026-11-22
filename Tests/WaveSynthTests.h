#pragma once

class AudioEngine;

/** Basic Synth's engine, patches and window: levels, aliasing, unison width, the filters against
    their settings, every modulation route, block-size invariance, old sessions and CPU. */
void runWaveSynthTests (AudioEngine&);
