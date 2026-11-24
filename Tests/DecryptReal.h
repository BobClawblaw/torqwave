#pragma once

#include <juce_core/juce_core.h>

/** Decrypt on recordings that are not the synth's own: each file given (a note, read as the window
    reads a dropped file), a few of the system's alert sounds where there are any, and a factory patch
    played through its own effects (which Decrypt matches dry). For each: the target, the factory
    stage's best, the search until settled with its lock after each round, the rounds and time, and
    where the best still differs. For finding out why a search settles where it does. */
int runDecryptReal (const juce::StringArray& files);

/** How different from each other the patches Decrypt shows are: on each file given (printed by
    number, never by name), four sounds with no pitch, the two sets of ten factory patches (their
    own names left out), some system sounds and a wet patch. For each: the lock, the time, how many
    candidates are shown and from how many factory patches, how far apart they are from each other
    by Decrypt's own measure (least, median, most, and how many have another within 1.5 dB), and
    the chain's links with how far each is from the one before. "quick" takes three of each set. */
int runDecryptVariety (const juce::StringArray& filesAndFlags);

/** A check, part of --decrypt-heard: on sounds with a pitch and without, the candidates Decrypt
    shows are far enough from each other to hear (Decrypt::shownApartDb, rendered and measured
    again) and come from several factory patches, and its chain's neighbouring links are
    Decrypt::linkApartDb or more apart, each clearly nearer the signal. */
void testDecryptVariety();

/** What Decrypt's chord finder makes of each file given (by number), of sixteen factory patches
    playing single notes and chords of two to five, and of sounds with no pitch. */
int runDecryptChords (const juce::StringArray& files);

/** A check, part of --decrypt-heard: a sound of several notes at once is matched as its chord (the
    keys found, one note and noise never taken for one, the search's best sounding the chord). */
void testDecryptChords();
