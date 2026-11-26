#pragma once

class AudioEngine;

/** Torqwave Synth's engine, patches and window: levels, aliasing, unison width, the filters against
    their settings, every modulation route, block-size invariance, old sessions and CPU. */
void runWaveSynthTests (AudioEngine&);

/** The Complex patches (WaveSynthComplexTests.cpp): four oscillators or more, nine in some of each
    category, the low end mono, level, weight and cut, no clicks, eight notes at once, macros that move
    the sound, and the distance from each other and from the factory patches. Run by --synth, or alone
    with `TorqwaveTests --complex`. */
void runComplexPatchTests();

namespace juce { class File; }

/** Writes a line for each factory patch, `name|hash`, the hash of every sample of a held minor chord
    (two seconds and its tail) rendered on a new synth: to compare a build's patches with another's,
    sample for sample, on the same machine. Run with `TorqwaveTests --patch-hashes <file>`. */
void writePatchHashes (const juce::File&);

/** Saves pictures of the synth's window into the folder, in the dark theme at 2x: a patch as it
    opens, the oscillators' level envelopes with slots 9 to 16, oscillators 4 to 6, an oscillator playing
    a Sampled table, and the guide's picture (guide-synth.png: five oscillators on, a note held). Run with `TorqwaveTests --synth-window <folder>`. */
void saveSynthWindow (const juce::File& folder);

/** Prints `name|category|dB` for each factory patch: the peak of a held minor chord (C3, E flat 3, G3
    for two seconds and its tail), the measure the level check uses. Run with `TorqwaveTests --patch-levels`. */
void printPatchLevels();
/** Writes hashes of renders the factory patches don't reach (every warp, Sampled tables, level envelopes,
    odd blocks, saved states) into a folder, to compare two builds sample for sample. */
void writeSynthReference (const juce::File& folder);
