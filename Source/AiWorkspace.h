#pragma once

#include <JuceHeader.h>
#include "AudioDocument.h"
#include "Markers.h"
#include "PluginCatalogue.h"
#include "AiSession.h"
#include "AiSequence.h"

class AudioEngine;
class TrackEffects;

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
            std::map<juce::uint32, juce::PluginDescription> instruments;   // the instrument each instrument track plays, by track id
            SessionTools::PresetsOf instrumentPresets;                       // the presets saved for an instrument; none: no presets
            SessionTools::Chains effects;                                   // each track's chain in order, its instrument first, by track id
            Library library;                                                // the factory sounds, sequences and kits and the drum maps the tools reach; empty: none

            /** The Sequencer's sequence to write or review in place of a file or session: the
                sequence tools (AiSequence) and the plug-in look tools, with no audio. */
            std::optional<SequenceTools::Context> sequence;
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

        /** Working on the Sequencer's sequence; its tools (the written copy, the review and its suggestions). */
        bool isSequence() const noexcept                            { return sequenceTools != nullptr; }
        const SequenceTools* getSequenceTools() const noexcept      { return sequenceTools.get(); }

        /** The instruments the plan loads onto tracks once it is applied (not part of its undo step). */
        std::vector<SessionTools::InstrumentLoad> getInstrumentLoads() const
        {
            return sessionTools != nullptr ? sessionTools->getInstrumentLoads() : std::vector<SessionTools::InstrumentLoad>();
        }

        /** The voices that factory sequences and kits bring for the tracks they made, loaded once
            the plan is applied (not part of its undo step). */
        std::vector<SessionTools::VoiceLoad> getVoiceLoads() const
        {
            return sessionTools != nullptr ? sessionTools->getVoiceLoads() : std::vector<SessionTools::VoiceLoad>();
        }

        /** The track chains the plan changes, once it is applied (not part of its undo step). */
        std::vector<SessionTools::ChainChange> getChainChanges() const
        {
            return sessionTools != nullptr ? sessionTools->getChainChanges() : std::vector<SessionTools::ChainChange>();
        }

        /** The tracks the plan freezes or bounces once it is applied, each its own undo step. */
        std::vector<SessionTools::Render> getRenders() const
        {
            return sessionTools != nullptr ? sessionTools->getRenders() : std::vector<SessionTools::Render>();
        }

        /** The changes the plan makes to Torqwave Synths on tracks once it is applied (not part of its undo step). */
        std::vector<SessionTools::SynthChange> getSynthChanges() const
        {
            return sessionTools != nullptr ? sessionTools->getSynthChanges() : std::vector<SessionTools::SynthChange>();
        }

        /** Runs one tool call and returns its result for the model, as JSON text. Never throws:
            a bad call comes back as {"ok": false, "error": ...} for the model to correct. */
        juce::String run (const juce::String& tool, const juce::String& argumentsJson);

        /** Asked during long renders; true stops them. */
        std::function<bool()> shouldStop;

        //==============================================================================
        /** A UI panel / window action requested by Ask Torq (e.g. open Score / Notation, Mixer, Piano Roll). */
        struct PanelAction
        {
            juce::String panel;   // canonical panel ID ("score", "pianoRoll", "mixer", "soundLibrary", "trackInstrument", ...)
            bool show = true;
        };

        /** Panel/window actions to execute when the plan is applied (not part of the audio undo step). */
        const std::vector<PanelAction>& getPanelActions() const noexcept  { return panelActions; }

        /** Resolves a friendly panel name ("score", "notation", "piano roll", "mixer", etc.) to a canonical
            panel key + human title; returns empty strings if unknown. */
        static std::pair<juce::String, juce::String> resolvePanelName (const juce::String& name);

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
        juce::var openPanel (const juce::var&);

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
        std::vector<PanelAction> panelActions;
        std::unique_ptr<SessionTools> sessionTools;
        std::unique_ptr<SequenceTools> sequenceTools;
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

    /** A track's chain as it stands, for SessionTools: each plug-in by its slot, with its
        parameters read (on the message thread, as plug-ins expect). */
    std::vector<SessionTools::Effect> readChain (const TrackEffects&, juce::uint32 trackId);

    /** Makes a plan's chain change on the engine's live chain (message thread): the effects it
        leaves out are taken off, the new ones loaded, then all are put in its order (an
        instrument stays first), switched on or off and set. done gets the loads' errors once
        it is all made, perhaps before this returns. */
    void applyChainChange (AudioEngine&, const SessionTools::ChainChange&, std::function<void (const juce::String& errors)> done);

    /** The state a planned instrument load sets: its saved preset's (from the user's instrument
        presets), or its factory sound's; nothing with no preset. error says when one was named
        and couldn't be read. Message thread. */
    std::optional<juce::MemoryBlock> stateFor (const SessionTools::InstrumentLoad&, juce::String& error);

    /** Loads the voices a plan's factory sequences and kits bring onto their tracks, as a drop
        from the Sequence Library does (message thread, after the plan is applied). done gets the
        loads' errors, at once when there is nothing to load. */
    void loadVoices (AudioEngine&, const std::vector<SessionTools::VoiceLoad>&, std::function<void (const juce::String& errors)> done);

    /** Makes a planned change to the Torqwave Synth a track plays (Decrypt's patch, or a table from
        a clip), on the message thread; false when the track no longer plays it. */
    bool applySynthChange (AudioEngine&, const SessionTools::SynthChange&);
}
