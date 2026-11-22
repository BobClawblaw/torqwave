#pragma once

#include <JuceHeader.h>
#include "SequenceLibrary.h"

/** Experimental: what Ask Torq's session tools can reach besides the session itself: the sounds
    the built-in instruments ship with, the factory sequences and kits, and the saved drum maps.
    The tools are handed these as functions (Library), so the tests can give them their own. */
namespace Ai
{
    /** A sound an instrument ships with: one of Torqwave Synth's factory patches, Basic Drums'
        kits or StringSwarm's presets. */
    struct FactorySound
    {
        juce::String name, category, about;   // category and about: empty with none
    };

    namespace FactorySounds
    {
        /** The sounds a built-in instrument ships with, in its browser's order; none for any other plug-in. */
        std::vector<FactorySound> of (const juce::PluginDescription&);

        /** The instrument's whole state (as getStateInformation gives it) with the named factory
            sound loaded over its defaults, or nothing when it has no such sound. Message thread. */
        std::optional<juce::MemoryBlock> stateOf (const juce::PluginDescription&, const juce::String& name);
    }

    /** The names nearest to a word that matched none of them, nearest first, at most `most`: those
        sharing more of its words come first, then those fewer letters away (the edit distance,
        ignoring case), then in the order given. The same answer every time. */
    juce::StringArray nearestNames (const juce::StringArray& names, const juce::String& wanted, int most = 5);

    /** Whether every word of a search is somewhere in the text, ignoring case. An empty search matches. */
    bool hasWords (const juce::String& text, const juce::String& search);

    struct Library
    {
        /** An instrument's factory sounds; none: no instrument has any. */
        std::function<std::vector<FactorySound> (const juce::PluginDescription&)> factorySounds;

        /** The factory sequences and kits (SequenceLibrary::scan of the factory folder), read when
            a tool first asks; none: there are none. */
        std::function<std::vector<SequenceLibrary::Entry>()> sequencePresets;

        /** Whether a sequence dropped into a session is moved into its key (SequenceLibrary::transposesDrops). */
        bool transposeDrops = false;

        /** The saved drum maps' names, and one by its name (DrumMaps::Store); none: no maps. */
        std::function<juce::StringArray()> drumMapNames;
        std::function<std::vector<DrumSound> (const juce::String&)> drumMap;

        /** The app's own: the built-in instruments' sounds, the factory presets' folder, the
            user's drum maps and the drop setting as it is now. */
        static Library standard();
    };
}
