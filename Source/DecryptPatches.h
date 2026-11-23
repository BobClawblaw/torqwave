#pragma once

#include "WaveSynth.h"
#include "EffectPresets.h"

/** The patches taken from Decrypt and named: Torqwave Synth's "Decrypted" category.

    Each is one of the user's own Torqwave Synth presets, in the same store the presets bar saves
    to (so it is in the bar's menu too, and holds the same things: every setting, the Sampled
    tables, the macros' names). What makes it a Decrypted patch is its name on a list kept beside
    the favourites, a line each, in Torqwave's application data: the same in every session. A name
    on the list with no preset behind it (deleted from the bar) is dropped when the list is read. */
namespace DecryptedPatches
{
    using Synth = BuiltInEffects::BasicSynthProcessor;

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

        /** The Decrypted patches' names, sorted, case ignored. */
        juce::StringArray names() const;
        bool contains (const juce::String& name) const;

        /** Why a name can't be given to a patch: it is empty, or one of the user's presets or a
            factory patch has it (`except`, a patch being renamed, may keep its own). Empty: it can. */
        juce::String whyNot (const juce::String& name, const juce::String& except = {}) const;
        /** Whether one of the user's Torqwave Synth presets has the name (case ignored). */
        bool isTaken (const juce::String& name) const;

        /** Saves the synth's patch under the name, as one of the user's presets and a Decrypted patch;
            the synth's patch takes the name. Refused (false) for an empty name or a factory patch's,
            and for a name one of the user's presets has unless `replace`. */
        bool save (Synth&, const juce::String& name, bool replace);
        /** Puts a Decrypted patch on the synth: its settings, tables, macro names and name. */
        bool load (Synth&, const juce::String& name) const;
        /** Renames one; refused for an empty or taken name. */
        bool rename (const juce::String& from, const juce::String& to);
        /** Deletes one: the preset too. */
        bool remove (const juce::String& name);
        /** Loads the patch after (by 1) or before (by -1) the one the synth is on, round the ends;
            from a patch that isn't one of them, the first (or last). Its name; empty with none. */
        juce::String step (Synth&, int by) const;

    private:
        void read();
        void write() const;

        EffectPresets::Store& presets;
        juce::File list;
        juce::StringArray listed;
    };
}
