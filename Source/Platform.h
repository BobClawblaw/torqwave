#pragma once

#include <JuceHeader.h>

/** Words that differ between the systems Torqwave runs on, for text the user reads. */
namespace Platform
{
    /** The system's file browser: "Finder" on macOS, "Explorer" on Windows. */
    inline juce::String fileManager()
    {
       #if JUCE_MAC
        return "Finder";
       #elif JUCE_WINDOWS
        return "Explorer";
       #else
        return "File Manager";
       #endif
    }

    /** The menu item that shows a file where it is kept. */
    inline juce::String showInFileManager()     { return "Show in " + fileManager(); }

    /** The system, as a name in a sentence. */
    inline juce::String systemName()
    {
       #if JUCE_MAC
        return "macOS";
       #elif JUCE_WINDOWS
        return "Windows";
       #else
        return "the system";
       #endif
    }

    /** The plug-in formats Torqwave hosts here, as a phrase: "VST3, Audio Unit and LV2" on the Mac,
        "VST3 and LV2" on Linux, "VST3" on Windows. */
    inline juce::String pluginFormats()
    {
       #if JUCE_MAC
        return "VST3, Audio Unit and LV2";
       #elif JUCE_WINDOWS
        return "VST3";
       #else
        return "VST3 and LV2";
       #endif
    }

    /** Text naming keys the Mac's way ("Cmd-K", "Option-drag"), with this system's names for them:
        on Windows and Linux the command key is Ctrl and Option is Alt. */
    inline juce::String keys (const juce::String& text)
    {
       #if JUCE_MAC
        return text;
       #else
        return text.replace ("Cmd", "Ctrl").replace ("Option", "Alt");
       #endif
    }
}
