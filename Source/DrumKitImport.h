#pragma once

#include "DrumMachine.h"
#include "BatteryKit.h"

/** Bringing a kit made elsewhere onto Basic Drums' pads: a Battery 4 kit file the user owns, read
    where it is. An import is first a plan (which pad gets what, and a report of everything that
    was approximated or left out), which nothing has touched yet; then the plan is put on the pads.
    Samples are read from where the kit points and are never copied. */
namespace DrumKitImport
{
    /** The files a pad can play. */
    constexpr const char* soundFileTypes = "wav;wave;bwf;aif;aiff;aifc;flac;ogg;opus;mp3;m4a;caf";

    struct PadLayer
    {
        juce::File file;
        int lowVelocity = 1, highVelocity = 127;
    };

    /** What one pad is to be. */
    struct Pad
    {
        int pad = -1;                  // which of the 64, once place() has said; -1 before
        juce::String name;
        int note = 36;                 // the note it was on; a sound's pad plays its own
        juce::Colour colour;           // transparent: none given
        float tune = 0.0f;             // semitones
        float level = 0.0f;            // dB
        float pan = 0.0f;              // -100 to 100
        float start = 0.0f;            // %
        bool reverse = false;
        std::vector<PadLayer> layers;
    };

    struct Plan
    {
        juce::String name;
        std::vector<Pad> pads;         // in the kit's order, every one with a place
        juce::StringArray report;      // what was approximated or left out, a line each
        juce::StringArray missing;     // samples that are not where the kit says
    };

    /** A Battery kit as a plan. Samples are looked for where the kit points from its own folder,
        then under `library` if one is given (a library that has moved). */
    Plan fromBattery (const BatteryKit::Kit&, const juce::File& kitFile, const juce::File& library = {});

    /** Gives every pad of a plan its place. A pad on a note one of the 30 sounds has, with that
        sound's pad still free, takes it; others take the user pads in order, keeping their notes;
        when those are full the rest take the sounds' pads left, and so change note. Pads past the
        64th are dropped. Each is said in the report. */
    void place (Plan&);

    /** Puts a plan on the pads, in place of the kit there: its samples read, the sounds no pad of
        the plan took left as the Standard kit's. Adds what could not be read to the report.
        Message thread. An error (nothing changed), or empty. */
    juce::String apply (BuiltInEffects::BasicDrumsProcessor&, Plan&);

    //==============================================================================
    /** The whole of an import from the window: asks for a kit file (or takes the one given), asks
        for the library's folder if samples are not where the kit says, puts the kit on the pads
        and shows the report. `done` is called with a line to say once the pads have changed. */
    void importBatteryKit (juce::Component& window, BuiltInEffects::BasicDrumsProcessor&, std::unique_ptr<juce::FileChooser>&,
                           std::function<void (const juce::String&)> done, const juce::File& kitFile = {});

    /** Whether a file is one importBatteryKit takes. */
    bool isBatteryKit (const juce::File&);
}
