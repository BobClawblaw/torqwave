#pragma once

#include <JuceHeader.h>
#include <map>
#include <optional>

/** What Torqwave knows about each plug-in, kept between runs in a JSON file:
    - facts, read from the plug-in itself: its parameters (name, unit, default, range or choices)
      and its channels, one entry for each plug-in in each format;
    - an understanding, written by a language model from those facts (or by the user): what the
      plug-in does, what it is for, which settings matter. One for each plug-in, whatever its
      format, so an AU and a VST3 of the same plug-in share it.

    Torqwave also ships descriptions, reviewed, of the plug-ins it has been learnt on
    (Data/plugin-knowledge.json): where this computer's catalogue has none of its own, the
    shipped one is used, so learning has only the rest to describe.

    Ask Torq reads it to choose a plug-in without loading each one to look. Safe to use from
    several threads. */
class PluginCatalogue
{
public:
    /** shipped: the descriptions Torqwave ships, as Data/plugin-knowledge.json holds them. */
    explicit PluginCatalogue (juce::File storeFile, const juce::var& shipped = {});
    ~PluginCatalogue();

    /** Where the app keeps it. */
    static juce::File defaultFile();

    //==============================================================================
    /** The facts of a plug-in that is open: { parameters: [...], parameterCount, inputs, outputs,
        latency, programs: [...], programCount }. At most parameterLimit's parameters and maxPrograms
        program (preset) names are listed; the counts say how many there are. */
    static juce::var factsOf (juce::AudioPluginInstance&);

    /** One parameter as Ask Torq is shown it: name, unit, its choices or its lowest and highest
        values as the plug-in writes them, and the default or the current value. */
    static juce::var parameterFacts (juce::AudioProcessorParameter&, bool currentValue);

    static constexpr int maxParameters = 400, maxPrograms = 128;

    /** How many of a plug-in's parameters Ask Torq is shown: all of Torqwave's own (its built-in format), whose
        names it knows, and maxParameters of any other maker's, to keep the replies short for small models. */
    static int parameterLimit (const juce::PluginDescription&);

    /** 2: the presets' names were added. */
    static constexpr int factsVersion = 2;

    /** Kept with the plug-in's name, maker, format and version; read again when the version changes. */
    void setFacts (const juce::PluginDescription&, const juce::var& facts);

    /** The plug-in couldn't be read (it crashed, hung or wouldn't open); not tried again until its version changes. */
    void setFactsFailed (const juce::PluginDescription&, const juce::String& reason);

    /** Facts, or a failure, recorded for this version of the plug-in by this version of the
        catalogue (factsVersion), so facts it learns to read later are read again. */
    bool hasCurrentFacts (const juce::PluginDescription&) const;

    /** The facts, or void; a failure has { error }. */
    juce::var getFacts (const juce::PluginDescription&) const;

    //==============================================================================
    struct Understanding
    {
        juce::String text;
        juce::String model;     // who wrote it: the model's name, or "you" when edited by hand
        juce::String version;   // the plug-in's version when it was written
        juce::Time when;
        bool shipped = false;   // Torqwave's own, reviewed; model says who wrote it
    };

    /** What the model is asked to begin an entry with when it doesn't know the plug-in. */
    static constexpr const char* notKnown = "Not known:";

    std::optional<Understanding> getUnderstanding (const juce::PluginDescription&) const;
    void setUnderstanding (const juce::PluginDescription&, Understanding);

    /** Its first sentence, for lists; empty when there is none. */
    juce::String summaryOf (const juce::PluginDescription&) const;

    /** Whether the model should write it: there is none yet. One is kept through the plug-in's
        updates (the AU and VST3 of a plug-in can carry different version numbers); forget()
        has it written again. */
    bool needsUnderstanding (const juce::PluginDescription&) const;

    /** Drops what is known of the plug-in: its facts in every format and the understanding
        written here. One Torqwave ships stays, and is used again. */
    void forget (const juce::PluginDescription&);

    int getNumFacts() const;
    int getNumUnderstood() const;   // written here
    int getNumShipped() const noexcept    { return (int) shipped.size(); }

    /** Writes the file if anything changed since it was last written; also done when the
        catalogue is deleted. */
    bool saveIfNeeded();

    /** Called on the thread that made the change. */
    std::function<void()> onChange;

private:
    static juce::String factsKey (const juce::PluginDescription&);
    static juce::String understandingKey (const juce::PluginDescription&);
    void load();
    void changed();

    const juce::File file;
    mutable juce::CriticalSection lock;
    std::map<juce::String, juce::var> facts;                 // by factsKey, guarded by lock
    std::map<juce::String, Understanding> understandings;    // by understandingKey, guarded by lock
    std::map<juce::String, Understanding> shipped;           // by understandingKey, set once
    bool dirty = false;                                      // guarded by lock

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginCatalogue)
};
