#pragma once

#include <JuceHeader.h>

/** Reading a Battery 4 kit file (.nbkt) the user owns, and a cell file (.nbcl), so Basic Drums can
    take its cells onto pads. The format is not documented; this is our own reader, written from
    looking at kit files. A kit is a container of nested chunks, each with its size in front. One
    chunk holds nearly everything, usually packed (a byte-oriented LZ77 stream in the layout FastLZ
    calls level 2). Unpacked it is a tree of numbered records: a program, its cells, a list of zones
    (one per sample layer, each naming its cell), the voice groups and a table of sample paths,
    which are relative to the kit file.

    The file is untrusted input. Every length, count and offset in it is checked against what is
    there before it is used, sizes are capped, and a record whose version this does not know is
    refused by name instead of being guessed at. Nothing here reads a sample: it only says where
    the kit points. */
namespace BatteryKit
{
    /** The most a kit file, and what it unpacks to, may be. The largest kit looked at was 131 KB and
        unpacked to a few megabytes. */
    constexpr size_t mostFileBytes = (size_t) 32 << 20, mostUnpackedBytes = (size_t) 128 << 20;
    constexpr int mostCells = 1024, mostZones = 65536, mostPaths = 65536, mostPathParts = 64, mostNameLength = 1024;

    /** A sample layer of a cell (the file calls it a zone). */
    struct Layer
    {
        int path = -1;                     // which of the kit's paths, or -1
        int lowVelocity = 1, highVelocity = 127;
        int fadeLow = 0, fadeHigh = 0;     // velocities over which it fades in from the bottom of its range, and out to the top
        int lowKey = 36, highKey = 36, rootKey = 36;
        float gain = 1.0f;                 // 1 is as recorded
        float pan = 0.0f;                  // -1 (left) to 1 (right)
        float tune = 1.0f;                 // a frequency ratio: 2 is an octave up
        int start = 0;                     // frames in from the sample's start
        int endBack = 0;                   // frames back from its end
        int frames = 0;                    // the sample's length as the kit knew it
    };

    struct Cell
    {
        int index = 0;                     // its place in the kit's matrix, row by row
        juce::String name;
        float gain = 1.0f, pan = 0.0f, tune = 1.0f;
        bool keyTracking = false, reverse = false;
        int voiceGroup = -1;               // which of the kit's voice groups (0 is Battery's group 1), or -1: the kit's own
        int colour = -1;                   // a place in Battery's palette, or -1
        int lowKey = -1, highKey = -1;     // the keys that play it, or -1 where the cell does not say

        // Its volume envelope, where the kit keeps one in a layout this reads: whether it is on, and
        // its attack, hold and decay in ms. (Its sustain and release are not read.)
        bool envelopeRead = false, envelopeOn = false;
        float attack = 0.0f, hold = 0.0f, decay = 0.0f;
        std::vector<Layer> layers;
    };

    /** Cells in one voice group share its voices: with one voice, each cuts the others off. Groups
        with the same exclusive group (0 or more) cut each other off too. A kit has 128 of them,
        and keeps only the ones changed from how a group starts: one voice, no name, no exclusive
        group. */
    constexpr int numVoiceGroups = 128;

    struct VoiceGroup
    {
        juce::String name;
        int voices = 1;
        int exclusiveGroup = -1;

        bool isDefault() const   { return name.isEmpty() && voices == 1 && exclusiveGroup == -1; }
    };

    /** A sample's place, relative to the kit file's folder: up some folders, then down by name. */
    struct Path
    {
        int up = 0;
        juce::StringArray parts;           // folders, then the file's name; empty when the kit names it in a way this does not read

        bool operator== (const Path& o) const   { return up == o.up && parts == o.parts; }
    };

    struct Kit
    {
        int programVersion = 0;
        std::vector<Cell> cells;           // only the cells with a name or a layer
        std::vector<VoiceGroup> voiceGroups;   // from Battery's group 1, up to the last one kept or used; the ones between as a group starts
        int kitVoices = 32;                // the voices of the kit's own group, which a cell in no other is in
        std::vector<Path> paths;
        bool voiceGroupsRead = true;       // false when that record was not one this knows: the cells came over without chokes
    };

    /** Reads a kit or a cell from memory. An error to show, or empty. */
    juce::String read (const void* data, size_t size, Kit&);

    /** Reads a kit or cell file. An error to show, or empty. */
    juce::String read (const juce::File&, Kit&);

    /** Where a path points, from the folder its kit is in; none for a path with no parts. */
    juce::File resolve (const Path&, const juce::File& kitFolder);

    /** The same sample looked for under another folder, for a library that has moved: the longest
        run of the path's last parts that names a file there. None if there is none. */
    juce::File resolveUnder (const Path&, const juce::File& folder);

    //==============================================================================
    // For the tests, which build kits in memory and never keep a real one.

    /** Unpacks the block's stream to exactly `size` bytes. False for a stream that is cut short,
        points back before its start or does not make that many bytes. */
    bool unpack (const void* packed, size_t packedSize, juce::MemoryBlock& out, size_t size);

    /** Packs bytes as a stream unpack() reads. With runs, repeated bytes become back-references
        (short, long, and far ones), so the unpacker's every branch is used. */
    juce::MemoryBlock pack (const void* data, size_t size, bool runs = true);

    struct WriteOptions
    {
        bool packed = true;                // a kit's block is packed, a cell file's is not
        int programVersion = 0xa3;
        int cellVersion = 0x95;
        int zoneVersion = 0x98;            // 0x98, or 0x9a with six more bytes after a layer's tune
        int voiceGroupVersion = 0x60;
        int pathsVersion = 2;              // 2, or 3: the kit file first, the zones naming samples from 1
        int numCells = 128;                // the matrix written, cells without a name or layer included
    };

    /** A kit file's bytes for a Kit, laid out as Battery lays one out, with every part this reader
        does not use left out or empty. read() of it gives the Kit back. */
    juce::MemoryBlock write (const Kit&, const WriteOptions& = {});
}
