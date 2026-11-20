#pragma once

#include "SequenceFile.h"
#include "SessionMixer.h"
#include "TrackEffects.h"

/** A sequence open in the Sequencer window: the sequence itself (in ticks), with undo steps of
    its own, and the file it is saved to. The window edits it as a note clip on a one-track
    session at the window's tempo (sessionFor); each edit comes back into ticks (withClip). The
    voice's plug-ins are outside the undo steps, as a session's effects are: changing them only
    marks it as changed. */
class SequencerDocument
{
public:
    SequencerDocument();

    /** Opens a sequence (or a new one, with no file yet): the undo steps start again. */
    void open (SequenceFile::Sequence, const juce::File& file);

    const SequenceFile::Sequence& get() const noexcept           { return history[(size_t) position].first; }
    const juce::File& getFile() const noexcept                   { return file; }

    /** An edit, as one undo step. Nothing happens when it is as it is now. */
    void apply (SequenceFile::Sequence, const juce::String& description);

    bool canUndo() const noexcept                                { return position > 0; }
    bool canRedo() const noexcept                                { return position + 1 < (int) history.size(); }
    juce::String getUndoDescription() const                      { return canUndo() ? history[(size_t) position].second : juce::String(); }
    juce::String getRedoDescription() const                      { return canRedo() ? history[(size_t) position + 1].second : juce::String(); }
    void undo()                                                  { if (canUndo()) --position; }
    void redo()                                                  { if (canRedo()) ++position; }

    bool isDirty() const noexcept                                { return position != savedPosition || voiceChanged; }
    void markVoiceChanged() noexcept                             { voiceChanged = true; }

    /** Writes it to the file (with this voice, when given), then it is saved there. Returns an error, or empty. */
    juce::String saveTo (const juce::File&, std::shared_ptr<const juce::XmlElement> voice);

    /** The sequence as a note clip from frame 0 of a session of its own at this tempo, on an
        instrument track made from its voice (named for it). */
    struct Built
    {
        SessionState state;
        juce::uint32 trackId = 0, clipId = 0;
        std::shared_ptr<juce::XmlElement> effects;   // the voice's chain to load, or null
    };

    static Built sessionFor (const SequenceFile::Sequence&, double tempo, double sampleRate);

    /** The same session with the sequence (an edited one) in place of the clip, its ids kept. */
    static SessionState withSequence (const Built&, const SequenceFile::Sequence&, double tempo);

    /** The sequence with the notes, lanes, lengths and channel of the clip as edited in such a
        session (read in ticks through its tempo); its name, folder, key, tags and voice kept. */
    static SequenceFile::Sequence withClip (const SequenceFile::Sequence&, const SessionState&, juce::uint32 clipId);

    /** A copy to save as a variation: named after it ("Name Variation"), linked to it by its name. */
    static SequenceFile::Sequence variationOf (const SequenceFile::Sequence&);

    /** A new, empty sequence of this many bars in a folder of the library. */
    static SequenceFile::Sequence blank (const juce::String& name, int bars, int beatsPerBar, double tempo);

private:
    std::vector<std::pair<SequenceFile::Sequence, juce::String>> history;
    int position = 0, savedPosition = 0;
    bool voiceChanged = false;
    juce::File file;
};

//==============================================================================
/** What the Sequencer window plays: its one-track session from frame 0 through the voice's own
    effects (made for it, beside the session's), in a loop or once, with a click on each beat,
    and the notes auditioned live through the instrument. Added to the audio device beside the
    session; it plays only while asked to, or for a few seconds after a note is auditioned. Set
    and asked on the message thread; the audio thread never waits for it. */
class SequencerPlayer final : public juce::AudioSource
{
public:
    SequencerPlayer();
    ~SequencerPlayer() override;

    /** The effects the session plays through: prepared and filled on the message thread. */
    TrackEffects& getEffects() noexcept                    { return effects; }
    const TrackEffects& getEffects() const noexcept        { return effects; }

    /** What to play: its track, and the end of its clip, where a loop goes back to the start. */
    void setState (std::shared_ptr<const SessionState>, juce::uint32 trackId);

    void play();
    void stop();
    bool isPlaying() const noexcept                        { return playing.load(); }
    juce::int64 getPosition() const noexcept               { return position.load(); }

    void setLooping (bool on) noexcept                     { looping = on; }
    void setClick (bool on) noexcept                       { click = on; }
    bool isLooping() const noexcept                        { return looping.load(); }
    bool isClicking() const noexcept                       { return click.load(); }

    /** A note played live on the instrument (velocity 0: let go). */
    void audition (int pitch, float velocity);

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override {}
    void getNextAudioBlock (const juce::AudioSourceChannelInfo&) override;

private:
    void renderRun (const SessionState&, juce::int64 start, int destStart, int numFrames, const SessionMixer::Live*);

    TrackEffects effects;
    juce::SpinLock lock;
    std::shared_ptr<const SessionState> state;
    juce::uint32 trackId = 0;
    juce::int64 end = 0;

    std::atomic<bool> playing { false }, looping { true }, click { false };
    std::atomic<juce::int64> position { 0 };
    bool wasPlaying = false;

    juce::MidiMessageCollector collector;
    juce::MidiBuffer liveBlock;
    juce::int64 idleFrames = 0;        // frames left to play the instrument stopped, after an audition
    double rate = 48000.0;
    SessionMixer::Buffers buffers;
    juce::AudioBuffer<float> mix;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequencerPlayer)
};
