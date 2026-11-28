#pragma once

#include "DrumMachine.h"
#include "BatteryKit.h"
#include "MaschineKit.h"
#include "MpcKit.h"

/** Bringing a kit made elsewhere onto Basic Drums' pads: a Battery 4 kit file (.nbkt/.nbcl),
    a Maschine 2/3 kit or sound (.mxgrp/.mxsnd) the user owns, or a folder of sound files laid on
    the pads by their names. An import is first a plan (which pad gets what, and a report of
    everything that was mapped, approximated or left out), which nothing has touched yet; then the
    plan is put on the pads. Samples are read from where they are and are never copied. */
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
        int choke = 0;                 // the choke group it is in, 1 to 8, or 0
        bool envelope = false;         // a volume envelope, for the pad's ENV page: on, with these in ms
        float attack = 0.0f, hold = 0.0f, decay = 0.0f;
        std::vector<PadLayer> layers;

        // Clean-room synthesized drum support (e.g. Maschine internal Drum Synths mapped onto Basic Drums)
        bool synthesised = false;      // true when the pad plays a synthesized drum (GM sound or source) rather than samples
        int source = 0;                // 0 for the pad's own GM sound, or 1..DrumMachine::numSources
        int synthType = DrumMachine::modernType;
        float synthDecay = 300.0f;     // ms (5..5000)
        float tone = 0.0f;             // -100..100
        float drive = 0.0f;            // 0..100

        // Clean-room per-pad FX translation (filter, bitcrusher, sends, and PadEffects)
        int filter = 0;                // 0 off, 1 LP, 2 BP, 3 HP
        float cutoff = 2000.0f;        // Hz
        float resonance = 20.0f;       // %
        float crush = 0.0f;            // %
        float reverb = 0.0f;           // %
        float delay = 0.0f;            // %
        DrumMachine::PadEffects effects {};
    };

    struct Plan
    {
        juce::String name;
        std::vector<Pad> pads;         // in the kit's order, every one with a place
        juce::StringArray report;      // what was mapped, approximated or left out, a line each
        juce::StringArray missing;     // samples that are not where the kit says

        // Optional kit-bus defaults translated from group FX
        float kitDrive = 0.0f;         // 0..100
        float kitGlue = 0.0f;          // 0..100
        float reverbSize = 50.0f;      // 0..100
        int delayTime = 2;             // index into DrumMachine::delayTimeNames()
        float feedback = 35.0f;        // 0..100
    };

    /** A Battery kit as a plan. Samples are looked for where the kit points from its own folder,
        then under `library` if one is given (a library that has moved). */
    Plan fromBattery (const BatteryKit::Kit&, const juce::File& kitFile, const juce::File& library = {});

    /** A Maschine 2/3 Group (.mxgrp) or Sound (.mxsnd) as a plan: sample zones, clean-room mapped
        synthesized drum sources (Kick, Snare, Hi-Hat, Tom, Perc, Cymbal), and clean-room mapped
        per-pad & kit-bus effects (EQ, Filter, Saturator, Transient Shaper, Compressor, Bitcrusher,
        Reverb/Delay sends). */
    Plan fromMaschine (const MaschineKit::Kit&, const juce::File& kitFile, const juce::File& library = {});

    /** An Akai MPC Drum (.xpm) program as a plan: pad layers, velocity ranges, tune, level, pan,
        start, reverse, choke/mute groups, filter, and volume envelope. */
    Plan fromMpc (const MpcKit::Program&, const juce::File& xpmFile, const juce::File& library = {});

    /** A folder of sound files as a plan (the folders inside it too), each file's pad chosen from
        its name:
        - a General MIDI drum note number at the front or the end ("36 Thud", "Thud 038"), unless
          the files' numbers go below 35, which makes them a count and not notes;
        - else an instrument's name or its usual short form ("Kick", "BD", "Snare", "ClosedHH",
          "Open Hat", "Low Tom", "Crash", "Cowbell" and so on): that sound's pad, the first time.
          Another file with the same instrument goes on a pad and a note of its own;
        - else a note's name at the front or the end ("Thud C2"), middle C being C4;
        - else a pad and a note of its own, from 82 up.
        Files alike but for a mark are one pad's layers: "v1", "v2" (or "vel1") split the velocities
        between them in that order, and "rr1", "rr2" take turns. */
    Plan fromFolder (const juce::File& folder);

    /** The most files a folder import looks at. */
    constexpr int mostFolderFiles = 2000;

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

    /** The same for a Maschine 2/3 Group (.mxgrp) or Sound (.mxsnd). */
    void importMaschineKit (juce::Component& window, BuiltInEffects::BasicDrumsProcessor&, std::unique_ptr<juce::FileChooser>&,
                            std::function<void (const juce::String&)> done, const juce::File& kitFile = {});

    /** The same for an Akai MPC Drum program (.xpm). */
    void importMpcKit (juce::Component& window, BuiltInEffects::BasicDrumsProcessor&, std::unique_ptr<juce::FileChooser>&,
                       std::function<void (const juce::String&)> done, const juce::File& xpmFile = {});

    /** Exports embedded MIDI patterns from a Maschine .mxgrp file to a chosen .mid file,
        mapped to Basic Drums' notes. */
    void exportMaschineMidi (juce::Component& window, std::unique_ptr<juce::FileChooser>&,
                             std::function<void (const juce::String&)> done, const juce::File& kitFile = {});

    /** The same for a folder of sounds: asks for one, or takes the one given. */
    void importFolder (juce::Component& window, BuiltInEffects::BasicDrumsProcessor&, std::unique_ptr<juce::FileChooser>&,
                       std::function<void (const juce::String&)> done, const juce::File& folder = {});

    /** Whether a file is one importBatteryKit takes. */
    bool isBatteryKit (const juce::File&);

    /** Whether a file is one importMaschineKit takes (.mxgrp or .mxsnd). */
    bool isMaschineKit (const juce::File&);

    /** Whether a file is one importMpcKit takes (.xpm). */
    bool isMpcKit (const juce::File&);
}
