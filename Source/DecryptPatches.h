#pragma once

#include "WaveSynth.h"
#include "EffectPresets.h"

/** The patches taken from Decrypt and named: Torqwave Synth's Decrypted page and the "Decrypted"
    folder of its patch list.

    Each is one of the user's own Torqwave Synth presets, in the same store the presets bar saves
    to (so it is in the bar's menu too, and holds the same things: every setting with the level
    trim, the Sampled tables, the macros' names). What makes it a Decrypted patch is its entry in
    a list kept beside the favourites in Torqwave's application data, the same in every session:
    its name, the category the user gave it (one of the synth's factory categories, or none), the
    lock it was taken at, the name of the recording it came from, and when. A name on the list
    with no preset behind it (deleted from the bar) is dropped. */
namespace DecryptedPatches
{
    using Synth = BuiltInEffects::BasicSynthProcessor;

    /** What the list keeps of a patch, beside the preset itself. */
    struct Entry
    {
        juce::String name;
        juce::String category;      // one of WaveSynth::patchCategories(), or empty: none
        juce::String source;        // the recording's name, as Decrypt's page showed it
        float lock = 0.0f;          // the lock it was taken at, 0 to 100
        juce::Time taken;
    };

    class Store
    {
    public:
        Store (EffectPresets::Store& presets, juce::File list);

        /** The user's own: their presets, and the list in the application data folder. */
        static Store& shared();
        /** Points the shared store at other presets and another list: for tests, which put it back
            with nullptr when they are done. */
        static void use (EffectPresets::Store* presets, const juce::File& list);
        static juce::File defaultList();

        /** The Decrypted patches, sorted by name, case ignored; and their names alone. */
        std::vector<Entry> entries() const;
        juce::StringArray names() const;
        bool contains (const juce::String& name) const;
        std::optional<Entry> entry (const juce::String& name) const;

        /** The order the patch list's Decrypted folder lists them in, and its arrows step through
            them: by category as the synth lists its categories, each by name, then those with none. */
        std::vector<Entry> inFolderOrder() const;

        /** Why a name can't be given to a patch: it is empty, or one of the user's presets or a
            factory patch has it (`except`, a patch being renamed, may keep its own). Empty: it can. */
        juce::String whyNot (const juce::String& name, const juce::String& except = {}) const;
        /** Whether one of the user's Torqwave Synth presets has the name (case ignored). */
        bool isTaken (const juce::String& name) const;

        /** Saves the synth's patch under the name, as one of the user's presets and a Decrypted patch
            with these facts (`facts.name` is not used); the synth's patch takes the name. Refused
            (false) for an empty name or a factory patch's, and for a name one of the user's presets
            has unless `replace`. */
        bool save (Synth&, const juce::String& name, bool replace, Entry facts = {});
        /** Puts a Decrypted patch on the synth: its settings, tables, macro names and name. */
        bool load (Synth&, const juce::String& name) const;
        /** Renames one; refused for an empty or taken name. */
        bool rename (const juce::String& from, const juce::String& to);
        /** Gives patches a category (empty: none); one that is no factory category is refused. How many were changed. */
        int setCategory (const juce::StringArray& names, const juce::String& category);
        /** Deletes one: the preset too. */
        bool remove (const juce::String& name);
        /** Loads the patch after (by 1) or before (by -1) the one the synth is on in the folder's
            order, round the ends; from a patch that isn't one of them, the first (or last). Its
            name; empty with none. */
        juce::String step (Synth&, int by) const;

    private:
        void read();
        void write() const;
        int indexOf (const juce::String& name) const;

        EffectPresets::Store& presets;
        juce::File list;
        std::vector<Entry> listed;      // sorted by name, case ignored
    };
}
