#pragma once

class AudioEngine;
namespace juce { class File; }

/** Instrument tracks: note clips in the session (their MIDI, the edits, the file), an instrument
    first in a track's chain played by them through the mixer, its automation, and the offline
    chains that bounces and mixdowns load. */
void runInstrumentTrackTests (AudioEngine&);

/** Torqwave Sampler on a session's track alone (part of the above): what --sampler runs with the sampler's own tests. */
void runSamplerTrackTests (AudioEngine&);

/** Sessions with Torqwave Synth on their tracks, kept as fixtures in Tests/Fixtures/SynthSessions, each played
    as a bounce plays it; their renders' hashes go to hashes.txt in the folder, to compare builds sample for
    sample. False if a fixture is missing. With writeFixtures, writes them first (false if any is there). */
bool writeSessionReference (AudioEngine&, const juce::File& folder, bool writeFixtures);
