#pragma once

class AudioEngine;

/** Basic Synth's sound doesn't click where notes start, end, are stolen or silenced. */
void runSynthClickTests (AudioEngine&);
