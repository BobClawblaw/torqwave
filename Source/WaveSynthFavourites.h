#pragma once

#include <JuceHeader.h>

namespace WaveSynth
{
    /** The factory patches marked as favourites: kept by name, a line each, in a file in Torqwave's
        application data, so they are the same in every session and outlast an update. A name that
        is no longer a factory patch is dropped when the file is read, unless the patch was taken out
        as a near-copy of another: then the one it was a copy of is marked instead. */
    class Favourites
    {
    public:
        explicit Favourites (juce::File file);

        /** The user's own, in the application data folder. */
        static Favourites& shared();
        /** Points the shared favourites at another file (and reads it): for tests, which put it back
            to defaultFile() when they are done. */
        static void useFile (const juce::File&);
        static juce::File defaultFile();

        bool contains (const juce::String& patchName) const;
        /** Marks or unmarks a patch and writes the file; only a factory patch can be marked. */
        void set (const juce::String& patchName, bool favourite);
        bool toggle (const juce::String& patchName);

        /** The favourites by name, and where each is in the factory list (in that same order). */
        juce::StringArray names() const                               { return marked; }
        std::vector<int> indices() const;
        /** Where each is in the factory list, in the patch tree's order: by category as the tree
            lists them, then as the tree's arrows go through a category. What the favourites menu
            shows and its arrows step through. */
        std::vector<int> inTreeOrder() const;

        juce::File getFile() const                                    { return file; }

    private:
        void read();
        void write() const;

        juce::File file;
        juce::StringArray marked;   // sorted by name, case ignored
    };
}
