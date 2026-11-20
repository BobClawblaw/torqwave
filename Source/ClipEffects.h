#pragma once

#include "Session.h"
#include <set>

/** Clip effects (SessionClip::Effects): a pitch shift and a chain of effects on one clip alone,
    rendered in the background and played from that render.

    A clip's chain is the text of a TORQWAVE_RACK element (AudioEngine::createRackXml()'s form):
    a SLOT a plug-in, each with its description, its state and whether it is bypassed. The
    functions here read and change that text; SessionEdits::setClipEffects puts it on a clip. */
namespace ClipEffects
{
    /** What a clip's effects are rendered from: the audio it plays (its active take where and as
        long as it plays it, or each take with its comp), the pitch shift, the chain and the rate.
        Clips with the same key render the same; empty for a clip without effects. */
    juce::String key (const SessionState&, const SessionClip&);

    /** True when a clip has effects but no render of what it plays now. */
    bool isStale (const SessionState&, const SessionClip&);

    /** Drops every render that no longer matches its clip, and every render of a clip without
        effects. Session does this to each state it is given. */
    void dropStale (SessionState&);

    /** The clip's audio as it plays (before its fades and gain), shifted and through its chain,
        as long as the clip. Null when progress stops it, there is no audio, or the chain can't run
        (error says why). Plug-ins are made here: call from a background thread, as for
        OfflineRender::processThroughRack. */
    AudioDocument::Snapshot render (const SessionState&, const SessionClip&, juce::AudioPluginFormatManager&, juce::String& error,
                                    const std::function<bool (float)>& progress = {});

    /** Every stale clip in the state rendered now, on this thread (for a test): a background
        thread, as for render(). The errors, one a line; a clip whose chain failed is left dry. */
    juce::String renderAll (SessionState&, juce::AudioPluginFormatManager&);

    //==============================================================================
    /** A slot of a chain, as shown. */
    struct Slot
    {
        juce::String name;
        juce::PluginDescription plugin;
        bool bypassed = false;
    };

    std::vector<Slot> slotsOf (const juce::String& chain);

    /** The chain with a plug-in added at its end, with this state (empty: its defaults). */
    juce::String withSlot (const juce::String& chain, const juce::PluginDescription&, const juce::MemoryBlock& state = {});

    /** The chain with a slot taken out, bypassed or not, given a new state, or moved; the chain as
        it was for an index out of range. */
    juce::String withoutSlot (const juce::String& chain, int index);
    juce::String withBypass (const juce::String& chain, int index, bool bypassed);
    juce::String withState (const juce::String& chain, int index, const juce::MemoryBlock& state);
    juce::String withMove (const juce::String& chain, int from, int to);

    /** A slot's saved state (empty when it has none). */
    juce::MemoryBlock stateOf (const juce::String& chain, int index);

    /** What a clip's effects are, in a few words: "-5 st, Reverb, Compressor"; empty for none. */
    juce::String describe (const SessionClip::Effects&);

    //==============================================================================
    /** Keeps rendering the session's clips with effects as they go stale, on a thread of its own,
        and puts each render into the session in place (not an undo step) once it is done. Recent
        renders are kept, so an undo is in place at once. Message thread, but the work. */
    class Renderer final : private juce::ChangeListener,
                           private juce::Thread
    {
    public:
        Renderer (Session&, juce::AudioPluginFormatManager&);
        ~Renderer() override;

        /** True when no clip waits for its render (a clip whose chain couldn't run is not waited for). */
        bool isUpToDate() const;

        /** Calls back (message thread) once no clip waits for its render: at once if none does.
            For what must hear every clip as it plays (a mixdown, a bounce). */
        void whenDone (std::function<void()>);

        /** Told on the message thread when a clip's chain can't run (a plug-in missing, say): the
            clip's name and why. The clip plays dry. Told once for each chain and audio. */
        std::function<void (const juce::String& clipName, const juce::String& error)> onFailed;

    private:
        void changeListenerCallback (juce::ChangeBroadcaster*) override;
        void run() override;
        void look();

        Session& session;
        juce::AudioPluginFormatManager& formats;

        struct Job
        {
            std::shared_ptr<const SessionState> state;
            juce::uint32 clipId = 0;
            juce::String key;
        };

        struct Kept
        {
            juce::String key;
            AudioDocument::Snapshot render;
            std::shared_ptr<const SessionState> from;   // keeps the audio it was rendered from (and so its key's addresses) in use
        };

        juce::CriticalSection lock;
        std::vector<Job> jobs;                         // under the lock: waiting
        std::set<juce::String> wanted;                 // under the lock: every stale clip's key now
        juce::String working;                          // under the lock: the key being rendered
        std::vector<Kept> kept;                        // message thread: newest last
        std::set<juce::String> failed;                 // message thread: keys whose chain couldn't run
        std::vector<std::function<void()>> waiting;    // message thread: whenDone's
        juce::WaitableEvent work;

        JUCE_DECLARE_WEAK_REFERENCEABLE (Renderer)
    };
}
