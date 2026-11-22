#pragma once

#include <JuceHeader.h>
#include "Session.h"

/** Drum maps saved by name ("Battery: Dark Kit"): a kit's sounds, the key each is on and the
    General MIDI drum each stands for, a file each in a folder, for any session's tracks to use. */
namespace DrumMaps
{
    constexpr const char* extension = ".tqdrums";

    class Store
    {
    public:
        explicit Store (juce::File folder);

        /** The saved maps' names, in order. */
        juce::StringArray names() const;

        /** A map by its name; empty if there is none, or its file isn't one. */
        std::vector<DrumSound> load (const juce::String& name) const;

        /** Saves a map under a name (characters a file can't have become spaces), replacing one of
            that name. An empty string, or the reason it couldn't be saved. */
        juce::String save (const juce::String& name, const std::vector<DrumSound>&) const;

        bool remove (const juce::String& name) const;

        /** The name a map is filed under: trimmed, without characters a file name can't have. */
        static juce::String cleaned (const juce::String& name);

    private:
        juce::File fileFor (const juce::String& name) const;

        juce::File folder;
    };

    /** The user's own, in Torqwave's application data. */
    Store& userStore();
}
