#pragma once

#include "SequenceFile.h"

class AudioEngine;

/** Hearing a sequence before it goes into the session: it is rendered through its voice (its
    instrument and effects, as a drop would make them) at the session's tempo where it plays, on a
    thread of its own, then played in a loop beside whatever else plays. While the session plays,
    it starts on the session's next bar, so it is heard in time with it. Renders are kept (a few),
    so going back to a sequence plays it at once. */
class SequencePreview final : private juce::AsyncUpdater
{
public:
    explicit SequencePreview (AudioEngine&);
    ~SequencePreview() override;

    /** Plays this sequence (rendering it first if it hasn't been at this tempo). */
    void play (const juce::File&);
    void stop();

    enum class State { stopped, rendering, playing };
    State getState() const noexcept            { return state; }
    juce::File getFile() const                  { return current; }

    /** What the panel says: "Rendering Groove...", "Groove at 128 BPM", or why it is silent. */
    juce::String describe() const;

    /** Called on the message thread when the state, the file or the description changed. */
    std::function<void()> onChange;

    /** The sequence through its voice at a steady tempo, one pass of it as it sounds looping: the
        pass after one before it, so what rings past the end comes in at the start. Empty with an
        error when its voice has no instrument or the instrument can't load. Call off the message
        thread: plug-ins are made as offline rendering makes them. */
    static juce::String render (const SequenceFile::Sequence&, double tempo, int beatsPerBar, double sampleRate,
                                juce::AudioPluginFormatManager&, juce::AudioBuffer<float>& out);

    /** Frames from `frame` to the next bar's start in the map (0 on a bar's first frame). */
    static juce::int64 framesToNextBar (const Metronome::TempoMap&, juce::int64 frame);

    /** For tests: the player's source, pulled directly. */
    juce::AudioSource& getSource();

private:
    class Loop;
    struct Job;

    void handleAsyncUpdate() override;
    void rendered (const juce::File&, juce::String key, std::shared_ptr<const juce::AudioBuffer<float>>, juce::String error, double tempo);
    void start (std::shared_ptr<const juce::AudioBuffer<float>>);

    AudioEngine& engine;
    std::unique_ptr<Loop> loop;
    juce::AudioSourcePlayer player;
    juce::ThreadPool renderer { 1 };

    juce::File current;
    State state = State::stopped;
    juce::String error, playingAt;
    std::vector<std::pair<juce::String, std::shared_ptr<const juce::AudioBuffer<float>>>> cache;   // newest last

    juce::CriticalSection pendingLock;
    std::vector<std::function<void()>> pending;   // results to take on the message thread

    JUCE_DECLARE_WEAK_REFERENCEABLE (SequencePreview)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequencePreview)
};
