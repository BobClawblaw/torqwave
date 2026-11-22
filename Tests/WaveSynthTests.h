#pragma once

class AudioEngine;

/** Torqwave Synth's engine, patches and window: levels, aliasing, unison width, the filters against
    their settings, every modulation route, block-size invariance, old sessions and CPU. */
void runWaveSynthTests (AudioEngine&);

namespace juce { class File; }

/** Writes a line for each factory patch, `name|hash`, the hash of every sample of a held minor chord
    (two seconds and its tail) rendered on a new synth: to compare a build's patches with another's,
    sample for sample, on the same machine. Run with `TorqwaveTests --patch-hashes <file>`. */
void writePatchHashes (const juce::File&);

/** Saves pictures of the synth's window into the folder, in the dark theme at 2x: a patch as it
    opens, the oscillators' level envelopes with slots 9 to 16, and an oscillator playing a Sampled
    table. Run with `TorqwaveTests --synth-window <folder>`. */
void saveSynthWindow (const juce::File& folder);
