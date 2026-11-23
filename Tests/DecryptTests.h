#pragma once

#include <juce_core/juce_core.h>

/** Decrypt: a clip is trimmed, levelled and its pitch found; the same sound measures as no distance;
    ten factory patches of different kinds, rendered and decrypted without their own names to start
    from, come back near the original; the same clip and seed give the same result on any number of
    threads; a search can be stopped, changes no synth but its own, and leaves a synth that is
    playing alongside it without a late block. */
void runDecryptTests();
/** Only the window's checks, quicker to repeat. */
void runDecryptWindowTests();
/** Only the check that a second sound is heard, with the synth's audio running (about three minutes). */
void runDecryptHeardTests();
/** Only the check that a model that is slow or hung never makes the page wait (part of --decrypt-heard). */
void runDecryptModelTests();
/** Saves pictures of the synth's window on Decrypt: empty, a search ended, a step chosen, a chain of 20. */
void saveDecryptWindow (const juce::File& folder);

/** The ten patches of the search test decrypted twice, alone and helped by the model at that
    OpenAI-compatible server (Ask Torq's DecryptAdvisor, told each patch's category in words), with
    each lock, time and what the model proposed. Returns 0 when every search finished. */
int runDecryptEval (const juce::String& url, const juce::String& model, const juce::String& key);

/** `--decrypt-file <sound>`: decrypts a sound file as the window does (until it comes no closer)
    and prints each round's best lock and distance; for looking at how a real recording goes. */
int runDecryptFile (const juce::String& path);

/** The search test's two sets, simple and complex, each in 16 rounds and until settled, with
    distances, the key each was heard at, rounds and times. */
int runDecryptMeasure();
