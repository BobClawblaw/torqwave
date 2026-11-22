#pragma once

class AudioEngine;

/** Torqwave Synth's sound doesn't click where notes start, end, are stolen or silenced. */
void runSynthClickTests (AudioEngine&);
