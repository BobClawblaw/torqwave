#pragma once

#include <JuceHeader.h>
#include "Session.h"

/** Experimental: the tools a language model works a multitrack session with, on a working copy
    of its state. Look tools read the copy; change tools edit it through SessionEdits, as the
    multitrack view's own commands do, and add a step to the plan. Nothing reaches the session
    until the plan is applied (by the caller, as one undo step: Session::apply).

    This first set covers the session's layout and mix: tracks, buses and folders, their volume,
    pan, mute, solo, outputs and sends; clips' places, trims, splits, gains and fades; markers
    and regions; the tempo; note clips with their notes; and an instrument track's instrument.
    An instrument lives outside the state (in the track's chain, TrackEffects), so choosing one
    is planned as a load for the caller to make once the plan is applied (getInstrumentLoads).
    A track's other effects and automation are not yet in it. */
namespace Ai
{
    class SessionTools
    {
    public:
        /** An instrument plug-in by its id or name, or nothing with the reason in error. */
        using FindInstrument = std::function<std::optional<juce::PluginDescription> (const juce::String&, juce::String& error)>;

        /** steps: the plan, which change tools add their lines to. instruments: the name of the
            instrument on each instrument track that has one, by track id. */
        SessionTools (SessionState, juce::String sessionName, juce::StringArray& steps,
                      std::map<juce::uint32, juce::String> instruments = {}, FindInstrument findInstrument = nullptr);

        /** The tools, as entries for the Chat Completions "tools" array. */
        static juce::Array<juce::var> toolDefinitions();

        /** What the model is told about sessions, added to the system message. */
        static juce::String instructions();

        /** Runs the tool when it is one of these and returns its result; nothing when it isn't.
            A bad call comes back as {"ok": false, "error": ...} for the model to correct. */
        std::optional<juce::var> run (const juce::String& tool, const juce::var& args);

        const SessionState& getState() const noexcept     { return state; }
        bool changed() const noexcept                       { return edited; }

        /** An instrument to load onto a track once the plan is applied. */
        struct InstrumentLoad
        {
            juce::uint32 trackId = 0;
            juce::PluginDescription plugin;
        };

        /** The loads the plan makes, for tracks still in it (a track added and then removed in
            the same plan loads nothing). */
        std::vector<InstrumentLoad> getInstrumentLoads() const;

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
        juce::var setClip (const juce::var&);
        juce::var removeClip (const juce::var&);
        juce::var addMarker (const juce::var&);
        juce::var setTempo (const juce::var&);
        juce::var addNoteClip (const juce::var&);
        juce::var setInstrument (const juce::var&);

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
        std::map<juce::uint32, juce::String> instruments;
        FindInstrument findInstrument;
        std::vector<InstrumentLoad> loads;
        bool edited = false;
    };
}
