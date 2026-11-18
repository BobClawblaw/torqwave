#pragma once

#include <JuceHeader.h>
#include <bitset>
#include "Session.h"

/** MIDI coming in live (from the MIDI inputs the audio settings turn on, the on-screen keyboard,
    and the piano roll's keys and notes), played by the instrument of the track being worked on and
    recorded into a note clip; and Standard MIDI Files read into note clips. */
namespace MidiIn
{
    /** A note played live, in session frames: on, and off (or the capture's end). */
    struct CapturedNote
    {
        int pitch = 60;
        juce::int64 on = 0, off = 0;
        float velocity = 0.8f;
    };

    /** A controller moved live, in session frames: a CC (0 to 127) or the pitch bend
        (ControllerLane::pitchBend), its value from 0 to 1. */
    struct CapturedController
    {
        int controller = 1;
        juce::int64 frame = 0;
        float value = 0.0f;
    };

    /** What a capture kept: the notes and the controller moves. */
    struct Capture
    {
        std::vector<CapturedNote> notes;
        std::vector<CapturedController> controllers;
    };

    /** The live messages: given from any thread with their time (a device callback, the keyboard
        on the message thread), taken block by block on the audio thread in time order, each at
        the frame its time falls in. A capture keeps the note-ons and -offs of the blocks taken
        while it is on, at their session frames, for a clip. */
    class Live
    {
    public:
        Live();

        /** The device sample rate the blocks are at. Call before any block (the audio device opening). */
        void prepare (double sampleRate);

        /** A message from a device (timestamped by it) or the keyboard (now). */
        void addMessage (const juce::MidiMessage&);

        /** The track whose instrument plays the messages (0: none). The blocks taken go on to it
            once no note is held, so a note's off reaches the instrument its on did. */
        void setTrack (juce::uint32 trackId) noexcept        { track = trackId; }
        juce::uint32 getTrack() const noexcept                { return track; }

        /** The track the last block taken was for: the one set, or while a note was held the one before. */
        juce::uint32 getPlayingTrack() const noexcept         { return playing; }

        /** Audio thread: this block's messages, at frames from 0. True when there were any. */
        bool takeBlock (juce::MidiBuffer& into, int numFrames);

        /** Audio thread: frames the instrument should keep running for after a message (a release
            ringing on), counted down by each block taken: more than 0 while something may sound. */
        juce::int64 getSoundingFrames() const noexcept        { return sounding; }

        /** Keeps the instruments running for the tail from now, as a message would: for sound an
            instrument makes on its own (its window's keys clicked) while the session is stopped. */
        void keepSounding() noexcept                          { sounding = (juce::int64) (tailSeconds * rate); }

        /** Starts keeping the notes and controller moves taken from now on; stopCapture gives
            them, notes still held ended at `endFrame`, and forgets them. A block's messages are
            kept when it is played at a session frame (captureBlock); blocks taken while stopped
            are not. */
        void startCapture();
        bool isCapturing() const noexcept                     { return capturing; }
        Capture stopCapture (juce::int64 endFrame);

        /** Audio thread, while capturing: the notes of a block taken, played from this session frame. */
        void captureBlock (const juce::MidiBuffer&, juce::int64 blockFrame);

        /** How long an instrument is kept running after the last message, in seconds. */
        static constexpr double tailSeconds = 10.0;

    private:
        juce::MidiMessageCollector collector;
        double rate = 48000.0;
        std::atomic<juce::uint32> track { 0 }, playing { 0 };
        std::bitset<128> sounded;                         // the audio thread's: pitches taken on and not yet off
        std::atomic<bool> capturing { false };
        std::atomic<juce::int64> sounding { 0 };

        juce::SpinLock captureLock;
        std::vector<CapturedNote> captured;              // under the lock: notes ended
        std::vector<CapturedController> controllers;     // under the lock
        std::array<juce::int64, 128> held;               // under the lock: each pitch's on frame, or -1
        std::array<float, 128> heldVelocity;
    };

    /** A clip of a capture: at the first note's frame (or `from`, when earlier and not before
        0), as long as the last note's end or controller move, each note at its frames from there
        and each controller's moves as a lane. No notes give a clip of no notes and no length
        (nothing to add). */
    NoteClip clipOf (const Capture&, juce::int64 from);

    /** A Standard MIDI File's tracks as note clips, one for each track with notes, each named by
        the track's name (or "Track n"), its notes at frames of the session rate from the file's
        start (through the file's tempo map), at position 0, as long as its last note. An error
        for a file that can't be read, or has no notes. */
    struct Imported
    {
        std::vector<NoteClip> clips;
        juce::String error;
    };

    Imported importFile (const juce::File&, double sampleRate);
    Imported importFile (juce::InputStream&, double sampleRate);

    /** The on-screen keyboard: five octaves, played by the mouse or the computer's keys (as the
        JUCE keyboard has them), its messages given to the live input. */
    class KeyboardPanel final : public juce::Component,
                                private juce::MidiKeyboardState::Listener
    {
    public:
        explicit KeyboardPanel (Live&);
        ~KeyboardPanel() override;

        void setStatus (const juce::String&);

        /** The keys' state: what the mouse and the computer's keys press, and what notes played
            into it (noteOn, noteOff) are sent on to the live input. */
        juce::MidiKeyboardState& getState() noexcept   { return state; }

        void resized() override;
        void paint (juce::Graphics&) override;

    private:
        void handleNoteOn (juce::MidiKeyboardState*, int channel, int note, float velocity) override;
        void handleNoteOff (juce::MidiKeyboardState*, int channel, int note, float velocity) override;

        Live& live;
        juce::MidiKeyboardState state;
        juce::MidiKeyboardComponent keyboard { state, juce::MidiKeyboardComponent::horizontalKeyboard };
        juce::Label status;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyboardPanel)
    };
}
