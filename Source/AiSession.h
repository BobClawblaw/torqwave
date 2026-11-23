#pragma once

#include <JuceHeader.h>
#include "Session.h"
#include "AiLibrary.h"

namespace SignalConsole { struct Patch; }
namespace WaveTables { struct Sampled; }

/** Experimental: the tools a language model works a multitrack session with, on a working copy
    of its state. Look tools read the copy; change tools edit it through SessionEdits, as the
    multitrack view's own commands do, and add a step to the plan. Nothing reaches the session
    until the plan is applied (by the caller, as one undo step: Session::apply).

    This first set covers the session's layout and mix: tracks, buses and folders, their volume,
    pan, mute, solo, outputs and sends; clips' places, trims, splits, gains, fades and own effects; markers
    and regions; the tempo; note clips with their notes; a track's volume and pan automation;
    and an instrument track's instrument.
    An instrument lives outside the state (in the track's chain, TrackEffects), so choosing one
    is planned as a load for the caller to make once the plan is applied (getInstrumentLoads).
    A track's chain of effects is planned the same way: added, removed, reordered, switched off
    and set (getChainChanges), for the caller to make once the plan is applied, outside its undo
    step. The chain the plan starts from is given as it stands (Chains), with each plug-in's
    parameters read beforehand, as the tools run away from the message thread. Automation of an
    effect's or instrument's parameters is in the state, so in the undo step. Bouncing and
    freezing render through the plug-ins, so they too are planned (getRenders), each its own
    undo step after the plan's; unfreezing is a change to the state.

    Since 0.20.0 the tools also reach what ships with Torqwave and what the user saved (Library):
    the built-in instruments' factory sounds, by name, category and a search word; the factory
    sequences and kits, found by their facets and put into the copy by the code a drop from the
    Sequence Library runs (SequenceFile::place, SequenceKit::place), the voices they bring being
    loaded by the caller once the plan is applied (getVoiceLoads), outside its undo step; and
    the saved drum maps, a track's map and a clip's notes moved to it or back, all in the state. */
namespace Ai
{
    class SessionTools
    {
    public:
        /** An instrument plug-in by its id or name, or nothing with the reason in error. */
        using FindInstrument = std::function<std::optional<juce::PluginDescription> (const juce::String&, juce::String& error)>;

        /** The presets saved for an instrument (InstrumentPresets). */
        using PresetsOf = std::function<juce::StringArray (const juce::PluginDescription&)>;

        /** A plug-in's parameter as the plan starts: its ID (TrackEffects::parametersById), its
            name, its value (0 to 1) and how that reads, how its lowest and highest values read,
            and for one with few steps what each step reads (in order, from 0 to 1). */
        struct Parameter
        {
            juce::String id, name, text, lowest, highest;
            float value = 0.0f;
            juce::StringArray choices;
        };

        /** A plug-in in a track's chain as the plan starts, by its slot's id (TrackEffects). */
        struct Effect
        {
            juce::uint32 slotId = 0;
            juce::PluginDescription plugin;
            bool instrument = false, bypassed = false;
            std::vector<Parameter> parameters;
        };

        /** Each track's chain in order, its instrument first, by track id. */
        using Chains = std::map<juce::uint32, std::vector<Effect>>;

        /** steps: the plan, which change tools add their lines to. instruments: the instrument on
            each instrument track that has one, by track id. */
        SessionTools (SessionState, juce::String sessionName, juce::StringArray& steps,
                      std::map<juce::uint32, juce::PluginDescription> instruments = {}, FindInstrument findInstrument = nullptr,
                      PresetsOf presetsOf = nullptr, Chains chains = {}, FindInstrument findEffect = nullptr);

        /** The factory sounds, sequences and kits and the drum maps the tools can reach; without
            one they know of none. */
        void setLibrary (Library l)                         { library = std::move (l); }

        /** The tools, as entries for the Chat Completions "tools" array. */
        static juce::Array<juce::var> toolDefinitions();

        /** What the model is told about sessions, added to the system message. */
        static juce::String instructions();

        /** Runs the tool when it is one of these and returns its result; nothing when it isn't.
            A bad call comes back as {"ok": false, "error": ...} for the model to correct. */
        std::optional<juce::var> run (const juce::String& tool, const juce::var& args);

        const SessionState& getState() const noexcept     { return state; }
        bool changed() const noexcept                       { return edited; }

        /** An instrument to load onto a track once the plan is applied, with a saved preset or
            none; or, with onlyPreset, a preset for the instrument the track already plays. */
        struct InstrumentLoad
        {
            juce::uint32 trackId = 0;
            juce::PluginDescription plugin;
            juce::String preset;
            bool factory = false;       // the preset is one of the instrument's factory sounds (FactorySounds::stateOf), not one saved
            bool onlyPreset = false;
            juce::String step;          // its line in the plan, taken out again if a later call replaces it
        };

        /** The loads the plan makes, for tracks still in it (a track added and then removed in
            the same plan loads nothing). */
        std::vector<InstrumentLoad> getInstrumentLoads() const;

        /** The voice a factory sequence or a kit's part brought for the new track it made: the
            chain to load onto it (a track's EFFECTS, as SequenceFile::place gives it) once the
            plan is applied, as a drop loads it. */
        struct VoiceLoad
        {
            juce::uint32 trackId = 0;
            std::shared_ptr<juce::XmlElement> effects;
            juce::String instrument;    // the instrument's name, or empty when the chain has none
        };

        /** The voices the plan loads, for tracks still in it. */
        std::vector<VoiceLoad> getVoiceLoads() const;

        /** A parameter to set: to a value from 0 to 1, or else to the text given, as the plug-in reads it. */
        struct Setting
        {
            juce::String parameterId;
            std::optional<float> value;
            juce::String text;
        };

        /** A plug-in in a chain as the plan leaves it: one there now by its slot's id, or (slotId 0)
            one to add. */
        struct Slot
        {
            juce::uint32 slotId = 0;
            juce::PluginDescription plugin;
            bool instrument = false, bypassed = false;
            std::vector<Setting> settings;
        };

        /** A track's whole chain as the plan leaves it, in order: slots there now and left out go,
            new ones are added, and all are put in this order (the instrument stays first), then
            switched on or off and set. */
        struct ChainChange
        {
            juce::uint32 trackId = 0;
            std::vector<Slot> chain;
        };

        /** The chains the plan changes, for tracks still in it. */
        std::vector<ChainChange> getChainChanges() const;

        /** A track to freeze, or to bounce over a range (the whole track, from 0 to its last
            clip's end, when it is empty), once the plan is applied, in the plan's order. */
        struct Render
        {
            juce::uint32 trackId = 0;
            bool freeze = false;
            juce::Range<juce::int64> range;
        };

        std::vector<Render> getRenders() const;

        /** A change to the Torqwave Synth an instrument track already plays, made once the plan is
            applied (not part of its undo step), as a preset is loaded: the patch Decrypt found
            for a clip (patch), or one oscillator given a Sampled table made from a clip
            (oscillator, from 0, and table). */
        struct SynthChange
        {
            juce::uint32 trackId = 0;
            std::shared_ptr<const SignalConsole::Patch> patch;
            int oscillator = -1;
            std::shared_ptr<const WaveTables::Sampled> table;
            juce::String step;
        };

        /** The synth changes the plan makes, in order, for tracks still in it. */
        std::vector<SynthChange> getSynthChanges() const;

        /** Asked during a long search (Decrypt); true stops it. */
        std::function<bool()> shouldStop;

    private:
        juce::var getSession();
        juce::var listClips (const juce::var&);

        juce::var addTrack (const juce::var&);
        juce::var setTrack (const juce::var&);
        juce::var removeTrack (const juce::var&);
        juce::var moveTrack (const juce::var&);
        juce::var setFolder (const juce::var&);
        juce::var setOutput (const juce::var&);
        juce::var setSend (const juce::var&);
        juce::var moveClip (const juce::var&);
        juce::var trimClip (const juce::var&);
        juce::var splitClip (const juce::var&);
        juce::var compTakes (const juce::var&);
        juce::var setClipEffects (const juce::var&);
        juce::var addSection (const juce::var&);
        juce::var moveSection (const juce::var&);
        juce::var deleteSection (const juce::var&);
        juce::var changeSectionBars (const juce::var&);
        juce::var setArrangement (const juce::var&);
        std::optional<juce::uint32> sectionOf (const juce::var& which, juce::String& error) const;
        juce::var setClip (const juce::var&);
        juce::var removeClip (const juce::var&);
        juce::var addMarker (const juce::var&);
        juce::var setTempo (const juce::var&);
        juce::var fitTempoToHits (const juce::var&);
        juce::var setAutomation (const juce::var&);
        juce::var addNoteClip (const juce::var&);
        juce::var setInstrument (const juce::var&);
        juce::var listInstrumentPresets (const juce::var&);
        juce::var findSequencePresets (const juce::var&);
        juce::var addSequencePreset (const juce::var&);
        juce::var addKit (const juce::var&);
        juce::var listDrumMaps (const juce::var&);
        juce::var setDrumMap (const juce::var&);
        juce::var remapClipNotes (const juce::var&);
        juce::var addTrackEffect (const juce::var&);
        juce::var removeTrackEffect (const juce::var&);
        juce::var moveTrackEffect (const juce::var&);
        juce::var setTrackEffect (const juce::var&);
        juce::var listEffectParameters (const juce::var&);
        juce::var freezeTrack (const juce::var&);
        juce::var decryptClip (const juce::var&);
        juce::var tableFromClip (const juce::var&);

        /** The audio clip a call names, its track, and its sound (up to `seconds` from its start, its
            channels at the session's rate); sets error when there is none. */
        const SessionClip* audioClipOf (const juce::var& which, juce::AudioBuffer<float>& audio, double seconds, juce::String& error) const;

        /** The instrument track a call names, when it plays Torqwave Synth already (not one the
            plan loads); sets error otherwise. */
        const SessionTrack* synthTrackOf (const juce::var& which, juce::String& error) const;
        juce::var bounceTrack (const juce::var&);

        /** A track's chain as planned so far: the start's, or the plan's once it has changed it. */
        struct PlannedSlot : Slot
        {
            const Effect* was = nullptr;   // the plug-in as the plan started; null for one added
            juce::String name() const      { return plugin.name; }
        };

        std::vector<PlannedSlot>& chainOf (juce::uint32 trackId);     // to change it
        std::vector<PlannedSlot> viewOf (juce::uint32 trackId) const; // to look at it

        /** The plug-in a call names in a track's chain: "instrument", its number among the
            effects (from 1, as get_session lists them) or its name; sets error when there is none. */
        std::optional<size_t> effectOf (juce::uint32 trackId, const juce::var& which, juce::String& error) const;

        /** A parameter of a plug-in there when the plan started, by its name or ID. */
        static const Parameter* parameterOf (const Effect&, const juce::String& which, juce::String& error);

        /** Refuses a chain change on a folder, VCA or frozen track: the reason, or empty. */
        juce::String chainRefusal (const SessionTrack&) const;
        juce::var storeScene (const juce::var&);
        juce::var recallScene (const juce::var&);
        std::optional<juce::uint32> sceneNamed (const juce::String&) const;

        /** The instrument a track plays, or will once the plan is applied; nothing for none. */
        std::optional<juce::PluginDescription> instrumentOf (juce::uint32 trackId) const;
        juce::StringArray presetsFor (const juce::PluginDescription&) const;
        std::vector<FactorySound> factorySoundsOf (const juce::PluginDescription&) const;

        /** The factory sequences and kits, read once. */
        const std::vector<SequenceLibrary::Entry>& sequencePresets();

        /** The factory sequence (or with kit, the kit) a call names by its name or its path from
            find_sequence_presets; sets error, with the nearest names, when there is none or
            several share the name. */
        const SequenceLibrary::Entry* presetOf (const juce::String& which, bool kit, juce::String& error);

        /** A bar's frame from a call's "bar" (from 1; bar 1 when left out), or nothing with the error. */
        std::optional<juce::int64> barFrameOf (const juce::var& bar, juce::String& error) const;

        /** The voice the plan brings for a track, or null. */
        const VoiceLoad* voiceOf (juce::uint32 trackId) const;

        /** The track a call names by its number (from 1, in get_session's order) or its name;
            sets error when there is none. */
        std::optional<int> trackOf (const juce::var& which, juce::String& error) const;
        std::optional<juce::int64> frameOf (const juce::var& seconds) const;
        juce::String seconds (juce::int64 frame) const;
        juce::String kindOf (const SessionTrack&) const;
        juce::String nameOf (juce::uint32 trackId) const;
        juce::var done (const juce::String& step);

        SessionState state;
        juce::String sessionName;
        juce::StringArray& steps;
        std::map<juce::uint32, juce::PluginDescription> instruments;
        FindInstrument findInstrument;
        PresetsOf presetsOf;
        std::vector<InstrumentLoad> loads;
        Library library;
        std::optional<std::vector<SequenceLibrary::Entry>> factoryPresets;   // the factory sequences and kits, once read
        std::vector<VoiceLoad> voices;
        Chains chains;
        FindInstrument findEffect;
        std::map<juce::uint32, std::vector<PlannedSlot>> planned;   // the chains the plan changes
        std::vector<Render> renders;
        std::vector<SynthChange> synthChanges;
        bool edited = false;
    };
}
