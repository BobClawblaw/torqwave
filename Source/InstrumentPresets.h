#pragma once

#include <JuceHeader.h>

/** Named presets for instrument plug-ins: the plug-in's whole saved state (getStateInformation),
    so whatever it keeps there comes back, such as the kit a drum sampler has loaded. Many
    instruments don't show the host their own presets (Battery's kits, say, are only in its own
    browser); a preset saved here is how Torqwave, and Ask Torq, can choose one again.

    Kept as one file a preset, in a folder for each plug-in and format (an AU's state and its
    VST3's are not interchangeable). */
namespace InstrumentPresets
{
    inline constexpr const char* extension = ".tqinstrument";

    class Store
    {
    public:
        explicit Store (juce::File folder);

        /** The plug-in's presets' names, sorted, ignoring case. */
        juce::StringArray getNames (const juce::PluginDescription&) const;

        /** The saved state of the named preset (matched ignoring case), or nothing. */
        std::optional<juce::MemoryBlock> load (const juce::PluginDescription&, const juce::String& name) const;

        /** Saves (or replaces) the named preset; false if the name is empty or the file can't be written. */
        bool save (const juce::PluginDescription&, const juce::String& name, const juce::MemoryBlock& state);

        /** Removes the named preset; false if there was none or it can't be removed. */
        bool remove (const juce::PluginDescription&, const juce::String& name);

        juce::File getFolder() const    { return folder; }

    private:
        juce::File folderFor (const juce::PluginDescription&) const;
        juce::File fileFor (const juce::PluginDescription&, const juce::String& name) const;

        juce::File folder;
    };

    /** The user's instrument presets (in the app's data folder, beside the effects' Presets). */
    Store& userStore();
}
