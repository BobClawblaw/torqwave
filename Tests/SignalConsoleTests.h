#pragma once

/** The signal console (Trace): the nodes within reach of a patch are the same for the same seed,
    keep a firewalled part, are the node held at no reach, all sound and none clips; a bridge lays
    them toward a second patch; addresses and signatures follow the settings. */
void runSignalConsoleTests();

namespace juce { class File; }

/** Saves pictures of the synth's window showing the Trace view (dark and light, at 2x) into the
    folder. Run with `TorqwaveTests --console-window <folder>`. */
void saveSignalConsoleWindow (const juce::File& folder);
