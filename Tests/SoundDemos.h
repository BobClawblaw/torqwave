#pragma once

#include <JuceHeader.h>

/** Writes the Sounds pages' demos (docs/sounds/audio) into the folder as MP3s, rendered by Torqwave's
    own instruments with nothing added: Torqwave Synth patches from every category, StringSwarm
    presets, Basic Drums' kits, Torqwave Sampler's factory presets, and the Sequence Library's factory
    kits and sequences, and Basic Drums' synthesized sources, each in a folder of its group (synth,
    swarm, drums, sources, sampler, kits, sequences), with index.json listing every clip (what it is, what it plays, its length, peak and
    size), which docs/sounds/build.py makes the pages from. `only` names the groups to render (the
    others' lines in index.json are kept); empty, all of them.
    Run with `TorqwaveTests --sound-demos <folder> [--only synth,kits]`. */
void writeSoundDemos (const juce::File& folder, const juce::StringArray& only = {});

/** Writes the Quality page's before-and-after clips (docs/quality) into the folder: damage of a known kind
    and amount (hiss, mains hum, missing highs) put on clips Torqwave rendered, then repaired by the
    code the Effects menu runs at its defaults, and a loop through the Maximizer to a loudness; each
    as MP3s in audio/ with its spectrogram in img/, and facts.json with what was done and what was
    measured against the clean clip. `also` adds the two the page leaves out ("clicks": Click Repair
    on 60 clicks put in; "clipping": Declip on a loop cut off 4 dB over), to measure them.
    Run with `TorqwaveTests --quality-demos <folder> [--also clicks,clipping]`. */
void writeQualityDemos (const juce::File& folder, const juce::StringArray& also = {});

/** Renders each sequence (.tqseq) in a folder once through its voice at its written tempo, with two bars
    to ring out, to "<Style> - <Name>.wav" in another, and prints each one's peak and its level and
    brightness in its first and second half: for hearing presets before and after a change.
    Run with `TorqwaveTests --sequence-clips <folder of sequences> --into <folder>`. */
void writeSequenceClips (const juce::File& from, const juce::File& into, bool exact = false);

/** Writes clips of the Complex patches into the folder, a folder for each category: each patch alone
    in its category's phrase (four bars at 132 BPM) and the same over a four-to-the-floor kick and
    off-beat hats from Basic Drums (`-club`), with index.json listing what each plays (the
    oscillators on and those only modulating, the unison voices a note asks for, its loudness and
    the level check's measure). Run with `TorqwaveTests --complex-clips <folder>`. */
void writeComplexClips (const juce::File& folder);
