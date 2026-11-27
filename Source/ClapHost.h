#pragma once

#include <JuceHeader.h>
#include <clap/clap.h>

/** CLAP (CLever Audio Plugin) 1.2 host format for Torqwave: discovers, scans (including out of
    process in ScannerWorker), loads, processes audio/MIDI, automates parameters, saves/restores
    state, and opens editors for native .clap plug-ins on macOS, Windows and Linux. */
namespace ClapHost
{
    constexpr const char* formatName = "CLAP";

    /** The format to register on AudioPluginFormatManager alongside VST3, AU, LV2 and Built-In. */
    std::unique_ptr<juce::AudioPluginFormat> createFormat();

    /** Default search paths for .clap plug-ins on this operating system (plus CLAP_PATH). */
    juce::FileSearchPath defaultSearchPaths();

    /** Creates a hosted CLAP instance directly from an in-process entry point (used by tests to
        verify the full CLAP host lifecycle without needing third-party .clap files installed). */
    std::unique_ptr<juce::AudioPluginInstance> createFromEntry (const clap_plugin_entry_t* entry,
                                                                const juce::String& pluginId,
                                                                double sampleRate, int maxBlockSize,
                                                                juce::String& error);
}
