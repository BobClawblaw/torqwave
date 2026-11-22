#pragma once

#include <JuceHeader.h>

/** Torqwave Synth's factory patches as a set: their settings against the list kept in
    Scripts/patch_settings.txt, and for the second bank's patches that each sounds in its register,
    does not click when struck again, released or taken for another note, has four macros that are
    heard, and is not a near-copy of another patch in its category. */
void runWaveSynthBankTests();

/** `--patch-settings` prints the list of patches and their settings' hashes (Scripts/patch_settings.txt);
    `--patch-measure <out> [--patches <file>] [--part k/n] [--wav <folder>]` writes each patch's
    measurements, for Scripts/make_patch_bank.py (and its held note as a WAV file). True when the arguments asked for one of them (`result` is the exit code). */
bool runPatchTool (const juce::StringArray& args, int& result);
