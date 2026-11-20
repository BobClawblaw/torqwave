#pragma once

class AudioEngine;

/** How many tracks the mix keeps up with, plain and with effects. Run by hand (--stress); not in the suite. */
void runStressTests (AudioEngine&);
