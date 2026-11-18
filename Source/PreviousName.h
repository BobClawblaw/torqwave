#pragma once

#include <JuceHeader.h>

/** Torqwave was called SpectraBlade up to 0.15.10. What that name left behind still opens: its
    session, project and rack files, the built-in effects saved in them, and, copied across on the
    first launch, its settings, presets, Sound Library and recovery folder. */
namespace PreviousName
{
    inline constexpr const char* name = "SpectraBlade";
    inline constexpr const char* sessionExtension = "sbsession";
    inline constexpr const char* projectExtension = "sbproject";
    inline constexpr const char* rackExtension = "sbrack";

    /** True for an element with the given tag (e.g. TORQWAVE_SESSION) or SpectraBlade's for it
        (SPECTRABLADE_SESSION). */
    inline bool hasTag (const juce::XmlElement& xml, const juce::String& tag)
    {
        return xml.hasTagName (tag)
            || (tag.startsWith ("TORQWAVE_") && xml.hasTagName ("SPECTRABLADE_" + tag.fromFirstOccurrenceOf ("TORQWAVE_", false, false)));
    }

    /** The first child with the given tag, or else with SpectraBlade's tag for it. */
    inline juce::XmlElement* getChild (const juce::XmlElement& xml, const juce::String& tag)
    {
        for (auto* child : xml.getChildIterator())
            if (hasTag (*child, tag))
                return child;

        return nullptr;
    }

    /** A built-in effect as SpectraBlade described it ("spectrablade:compressor" in the
        "SpectraBlade" format), described as Torqwave's own. Other plug-ins are left as they are. */
    inline void upgrade (juce::PluginDescription& d)
    {
        if (d.pluginFormatName != name && ! d.fileOrIdentifier.startsWith ("spectrablade:"))
            return;

        d.pluginFormatName = "Torqwave";
        d.manufacturerName = "Torqwave";
        d.fileOrIdentifier = "torqwave:" + d.fileOrIdentifier.fromFirstOccurrenceOf (":", false, false);
    }

    /** SpectraBlade's settings, with its built-in effects, their IDs and its XML tags renamed.
        Paths are left alone: recordings and files saved under a SpectraBlade folder stay there. */
    inline juce::String upgradeSettings (const juce::String& text)
    {
        return text.replace ("=\"SpectraBlade\"", "=\"Torqwave\"")
                   .replace ("=&quot;SpectraBlade&quot;", "=&quot;Torqwave&quot;")
                   .replace ("spectrablade:", "torqwave:")
                   .replace ("SPECTRABLADE_", "TORQWAVE_");
    }

    /** On the first launch, copies SpectraBlade's settings folder (settings, crashed plug-ins,
        recovered sessions) and its data folder (presets, Sound Library) to Torqwave's, and renames
        the settings inside. The SpectraBlade folders are left in place. */
    inline void copyData (const juce::PropertiesFile::Options& options)
    {
        auto previous = options;
        previous.applicationName = previous.folderName = name;
        const auto oldSettings = previous.getDefaultFile();
        const auto newSettings = options.getDefaultFile();

        if (newSettings.exists() || ! oldSettings.existsAsFile())
            return;

        const auto data = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
        const std::pair<juce::File, juce::File> folders[] {
            { oldSettings.getParentDirectory(), newSettings.getParentDirectory() },
            { data.getChildFile (name), data.getChildFile (options.folderName) },  // the same folder on Windows
        };

        for (const auto& [from, to] : folders)
            if (from.isDirectory() && ! to.exists())
                from.copyDirectoryTo (to);

        if (const auto copied = newSettings.getSiblingFile (oldSettings.getFileName()); copied.existsAsFile())
            if (newSettings.replaceWithText (upgradeSettings (copied.loadFileAsString())))
                copied.deleteFile();
    }
}
