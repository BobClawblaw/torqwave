#pragma once

#include <JuceHeader.h>
#include "Session.h"

/** Experimental: the tools a language model works a multitrack session with, on a working copy
    of its state. Look tools read the copy; change tools edit it through SessionEdits, as the
    multitrack view's own commands do, and add a step to the plan. Nothing reaches the session
    until the plan is applied (by the caller, as one undo step: Session::apply).

    This first set covers the session's layout and mix: tracks, buses and folders, their volume,
    pan, mute, solo, outputs and sends; clips' places, trims, splits, gains and fades; markers
    and regions; and the tempo. A track's effects, notes in note clips and automation are not
    yet in it: they live outside the state, or need their own checks first. */
namespace Ai
{
    class SessionTools
    {
    public:
        /** steps: the plan, which change tools add their lines to. */
        SessionTools (SessionState, juce::String sessionName, juce::StringArray& steps);

        /** The tools, as entries for the Chat Completions "tools" array. */
        static juce::Array<juce::var> toolDefinitions();

        /** What the model is told about sessions, added to the system message. */
        static juce::String instructions();

        /** Runs the tool when it is one of these and returns its result; nothing when it isn't.
            A bad call comes back as {"ok": false, "error": ...} for the model to correct. */
        std::optional<juce::var> run (const juce::String& tool, const juce::var& args);

        const SessionState& getState() const noexcept     { return state; }
        bool changed() const noexcept                       { return edited; }

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
        bool edited = false;
    };
}
