#pragma once

#include <JuceHeader.h>
#include "AudioDocument.h"
#include "Markers.h"
#include "PluginCatalogue.h"
#include "AiSession.h"

/** Experimental: the tools a language model works Torqwave with, run on a working copy.

    The working copy starts as the file (its audio, markers and selection) and the effects rack.
    Look tools read it; change tools edit it and add a step to the plan. Nothing reaches the file
    or the rack until the plan is applied (by the caller, as one undo step), so a plan can be read
    first and thrown away. Each edit goes through the same code as its menu command: Sequences
    for the effects, AudioEdits for the rest, the rack's own render for plug-ins.

    Plug-ins are driven through their parameters and saved state, which every AU, VST3 and LV2
    plug-in exposes, so the built-in effects and third-party plug-ins are worked the same way.
    They are created on the thread that calls run(): call it from a background thread. */
namespace Ai
{
    class Workspace
    {
    public:
        struct Start
        {
            AudioDocument::Snapshot audio;
            double sampleRate = 48000.0;
            juce::Range<juce::int64> selection;
            Markers markers;
            juce::String fileName;
            juce::Array<juce::PluginDescription> plugins;      // the plug-ins Torqwave knows
            std::unique_ptr<juce::XmlElement> rack;           // AudioEngine::createRackXml(); none: empty
            juce::AudioPluginFormatManager* formats = nullptr; // none: no plug-in tools
            PluginCatalogue* catalogue = nullptr;              // what is known of the plug-ins; none: nothing

            /** A multitrack session to work on in place of a file: the session tools (AiSession)
                and the plug-in look tools, with no audio. */
            std::optional<SessionState> session;
            juce::String sessionName;
        };

        explicit Workspace (Start);
        ~Workspace();

        /** The tools for a file, as the Chat Completions "tools" array. */
        static juce::var toolDefinitions();

        /** What the model is told about Torqwave and a file (the system message). */
        static juce::String instructions();

        /** This workspace's tools and system message: the file's, or the session's. */
        juce::var tools() const;
        juce::String systemMessage() const;

        /** Working on a session rather than a file; its working copy, and whether it changed. */
        bool isSession() const noexcept                             { return sessionTools != nullptr; }
        const SessionState* getSession() const noexcept             { return sessionTools != nullptr ? &sessionTools->getState() : nullptr; }
        bool sessionChanged() const noexcept                        { return sessionTools != nullptr && sessionTools->changed(); }

        /** Runs one tool call and returns its result for the model, as JSON text. Never throws:
            a bad call comes back as {"ok": false, "error": ...} for the model to correct. */
        juce::String run (const juce::String& tool, const juce::String& argumentsJson);

        /** Asked during long renders; true stops them. */
        std::function<bool()> shouldStop;

        //==============================================================================
        /** The plan so far: one line for each change, in order. */
        const juce::StringArray& getSteps() const noexcept          { return steps; }

        AudioDocument::Snapshot getAudio() const                    { return audio; }
        const Markers& getMarkers() const noexcept                  { return markers; }
        juce::Range<juce::int64> getSelection() const noexcept      { return selection; }
        bool audioChanged() const noexcept                          { return audio != original; }
        bool markersChanged() const noexcept                        { return markersEdited; }

        /** The chain changed: the rack should become createRackXml(). */
        bool rackChanged() const noexcept                           { return chainEdited; }
        std::unique_ptr<juce::XmlElement> createRackXml() const;

        /** The last chain rendered into the audio, for the undo step (AudioDocument::setRackApplied). */
        std::shared_ptr<const juce::XmlElement> getRackRendered() const   { return rendered; }

        /** A time in seconds, as the model reads it ("1.250"). */
        juce::String secondsText (juce::int64 frame) const;

    private:
        struct Slot
        {
            std::unique_ptr<juce::AudioPluginInstance> plugin;
            bool bypassed = false;
        };

        juce::var dispatch (const juce::String& tool, const juce::var& args);

        // Look
        juce::var getState();
        juce::var measure (const juce::var&);
        juce::var spectrum (const juce::var&);
        juce::var find (const juce::var&);
        juce::var listMarkers();
        juce::var listPlugins (const juce::var&);
        juce::var describePlugin (const juce::var&);
        juce::var listChain();
        juce::var getParameters (const juce::var&);

        // Change
        juce::var select (const juce::var&);
        juce::var edit (const juce::var&);
        juce::var fade (const juce::var&);
        juce::var trimSilence (const juce::var&);
        juce::var cutOrCrop (const juce::var&, bool crop);
        juce::var silence (const juce::var&);
        juce::var insertSilence (const juce::var&);
        juce::var addMarker (const juce::var&);
        juce::var renameMarker (const juce::var&);
        juce::var deleteMarker (const juce::var&);
        juce::var addPlugin (const juce::var&);
        juce::var setParameters (const juce::var&);
        juce::var bypassPlugin (const juce::var&);
        juce::var movePlugin (const juce::var&);
        juce::var removePlugin (const juce::var&);
        juce::var applyChain (const juce::var&);

        /** [start, end) from the call's "start" and "end" in seconds, or the selection, or the whole
            file. Sets error for times outside the file or backwards. */
        juce::Range<juce::int64> rangeOf (const juce::var& args, juce::String& error) const;
        std::optional<juce::int64> frameOf (const juce::var& seconds) const;
        juce::String rangeText (juce::Range<juce::int64>) const;
        void change (AudioDocument::Snapshot next, const juce::String& step);

        juce::String loadChain();
        Slot* slotOf (const juce::var& args, juce::String& error);
        std::optional<juce::PluginDescription> findPlugin (const juce::String& nameOrId, juce::String& error, bool instrumentsToo = false) const;

        AudioDocument::Snapshot original, audio;
        double sampleRate;
        juce::Range<juce::int64> selection;
        Markers markers;
        juce::String fileName;
        juce::Array<juce::PluginDescription> plugins;
        std::unique_ptr<juce::XmlElement> rackXml;
        juce::AudioPluginFormatManager* formats;
        PluginCatalogue* catalogue;

        std::vector<Slot> chain;
        bool chainLoaded = false, chainEdited = false, markersEdited = false;
        std::shared_ptr<const juce::XmlElement> rendered;
        juce::StringArray steps;
        std::unique_ptr<SessionTools> sessionTools;
    };

    /** Plug-in parameters set from text, as a model writes them. Public for the tests. */
    namespace Parameters
    {
        /** The number in a value's text ("-6.0 dB" -6, "1.2 kHz" 1200, "4:1" 4), or nothing. */
        std::optional<double> numberIn (const juce::String&);

        /** Sets the parameter to the value written as the plug-in writes it, or a choice's name,
            or a number in its units. Tries the plug-in's own reading of the text, then, for a
            number it read wrongly, searches the range for it. Returns what the plug-in shows
            afterwards; error when the value couldn't be reached. */
        juce::String set (juce::AudioProcessorParameter&, const juce::String& value, juce::String& error);
    }
}
