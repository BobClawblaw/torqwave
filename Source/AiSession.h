#pragma once

#include <JuceHeader.h>
#include "Session.h"

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
    Effects are added to the end of a track's chain the same way, once the plan is applied
    (getEffectLoads); their settings, order and automation are not yet in it. */
namespace Ai
{
    class SessionTools
    {
    public:
        /** An instrument plug-in by its id or name, or nothing with the reason in error. */
        using FindInstrument = std::function<std::optional<juce::PluginDescription> (const juce::String&, juce::String& error)>;

        /** The presets saved for an instrument (InstrumentPresets). */
        using PresetsOf = std::function<juce::StringArray (const juce::PluginDescription&)>;

        /** steps: the plan, which change tools add their lines to. instruments: the instrument on
            each instrument track that has one, by track id. */
        SessionTools (SessionState, juce::String sessionName, juce::StringArray& steps,
                      std::map<juce::uint32, juce::PluginDescription> instruments = {}, FindInstrument findInstrument = nullptr,
                      PresetsOf presetsOf = nullptr, std::map<juce::uint32, juce::StringArray> effects = {},
                      FindInstrument findEffect = nullptr);

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
            bool onlyPreset = false;
            juce::String step;          // its line in the plan, taken out again if a later call replaces it
        };

        /** The loads the plan makes, for tracks still in it (a track added and then removed in
            the same plan loads nothing). */
        std::vector<InstrumentLoad> getInstrumentLoads() const;

        /** An effect plug-in to add at the end of a track's chain once the plan is applied. */
        struct EffectLoad
        {
            juce::uint32 trackId = 0;
            juce::PluginDescription plugin;
        };

        /** The effects the plan adds, in order, for tracks still in it. */
        std::vector<EffectLoad> getEffectLoads() const;

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
        juce::var addTrackEffect (const juce::var&);

        /** The instrument a track plays, or will once the plan is applied; nothing for none. */
        std::optional<juce::PluginDescription> instrumentOf (juce::uint32 trackId) const;
        juce::StringArray presetsFor (const juce::PluginDescription&) const;

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
        std::map<juce::uint32, juce::StringArray> effects;
        FindInstrument findEffect;
        std::vector<EffectLoad> effectLoads;
        bool edited = false;
    };
}
