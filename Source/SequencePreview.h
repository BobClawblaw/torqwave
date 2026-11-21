#pragma once

#include "SequenceKit.h"

class AudioEngine;

/** Hearing a sequence before it goes into the session: it is rendered through its voice (its
    instrument and effects, as a drop would make them) at the session's tempo where it plays, on a
    thread of its own, then played in a loop beside whatever else plays. While the session plays,
    it starts on the session's next bar, so it is heard in time with it. A kit plays every part
    together. Renders are kept (a few), so going back to a sequence plays it at once. */
class SequencePreview final : private juce::AsyncUpdater
{
public:
    explicit SequencePreview (AudioEngine&);
    ~SequencePreview() override;

    /** Plays this sequence or kit (rendering it first if it hasn't been at this tempo). */
    void play (const juce::File&);
    void stop();

    enum class State { stopped, rendering, playing };
    State getState() const noexcept            { return state; }
    juce::File getFile() const                  { return current; }

    /** What a preview plays on: each sequence's own voice (the built-in voices for presets), the
        selected track's instrument (with its effects, as set now), or a plug-in chosen for each
        role (a role with none: its own voice). A kit plays each part on its role's plug-in; on the
        selected track's instrument it keeps its own voices, being several parts. */
    struct PlayOn
    {
        enum class Kind { own, selectedTrack, perRole };

        Kind kind = Kind::own;
        std::map<juce::String, juce::PluginDescription> forRole;

        bool operator== (const PlayOn&) const;
        std::unique_ptr<juce::XmlElement> toXml() const;
        static PlayOn fromXml (const juce::XmlElement*);   // null: its own voice

        /** Kept in the app's settings under this key. */
        static constexpr const char* settingsKey = "sequencePreviewOn";
        void save (juce::PropertySet&) const;
        static PlayOn load (juce::PropertySet&);
    };

    void setPlayOn (const PlayOn&);
    const PlayOn& getPlayOn() const noexcept   { return playOn; }

    /** The selected track's voice (its instrument and effects, as SequenceFile::voiceOf gives
        them), or null when no instrument track is selected. */
    std::function<std::unique_ptr<juce::XmlElement>()> getSelectedTrackVoice;

    /** The parts with the voices they play on (a file's own, or a kit's parts), and what that
        is called ("Basic Drums", or empty for their own). */
    static std::vector<SequenceFile::Sequence> voiced (std::vector<SequenceFile::Sequence> parts, bool kit, const PlayOn&,
                                                       std::unique_ptr<juce::XmlElement> selectedTrackVoice, juce::String& playsOn);

    /** What the panel says: "Rendering Groove...", "Groove at 128 BPM", "Groove at 128 BPM on Basic Drums", or why it is silent. */
    juce::String describe() const;

    /** Called on the message thread when the state, the file or the description changed. */
    std::function<void()> onChange;

    /** The sequence through its voice at a steady tempo, one pass of it as it sounds looping: the
        pass after one before it, so what rings past the end comes in at the start. Empty with an
        error when its voice has no instrument or the instrument can't load. Call off the message
        thread: plug-ins are made as offline rendering makes them. */
    static juce::String render (const SequenceFile::Sequence&, double tempo, int beatsPerBar, double sampleRate,
                                juce::AudioPluginFormatManager&, juce::AudioBuffer<float>& out);

    /** The same for a kit's parts played together, each through its own voice, a pass as long as
        the longest; parts with no instrument are left out. An error when none has one. */
    static juce::String render (const std::vector<SequenceFile::Sequence>& parts, double tempo, int beatsPerBar, double sampleRate,
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
    juce::String error, playingAt, playsOn;
    PlayOn playOn;
    std::vector<std::pair<juce::String, std::shared_ptr<const juce::AudioBuffer<float>>>> cache;   // newest last

    juce::CriticalSection pendingLock;
    std::vector<std::function<void()>> pending;   // results to take on the message thread

    JUCE_DECLARE_WEAK_REFERENCEABLE (SequencePreview)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequencePreview)
};
