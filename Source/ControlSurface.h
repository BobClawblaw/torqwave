#pragma once

#include <JuceHeader.h>
#include "Session.h"

class AudioEngine;
class MultitrackView;

/** The Mackie Control protocol, as a Mackie Control Universal (and surfaces in its MCU mode)
    speaks it: eight channel strips and a master fader. */
namespace Mackie
{
    // Buttons and their LEDs: a note, 127 pressed (or lit) and 0 let go (or dark); a strip's add its place (0 to 7).
    constexpr int recordArm = 0x00, solo = 0x08, mute = 0x10, select = 0x18, vpotPush = 0x20;
    constexpr int bankLeft = 0x2e, bankRight = 0x2f, channelLeft = 0x30, channelRight = 0x31;
    constexpr int rewind = 0x5b, forward = 0x5c, stop = 0x5d, play = 0x5e, record = 0x5f;
    constexpr int faderTouch = 0x68;      // to 0x70, the master's
    constexpr int strips = 8, master = 8;
    constexpr int stripChars = 7;         // a scribble strip's characters on each of its two lines

    /** The on-screen fader's taper (-60 to +12 dB, -12 at the middle) as a fader position, 0 to 1, and back. */
    float positionOfDb (float db) noexcept;
    float dbOfPosition (float position) noexcept;

    /** A fader moved to a position (strip 0 to 7, or master): a pitch bend on its channel, 14 bits. */
    juce::MidiMessage fader (int strip, float position);

    /** A fader the device moved: its strip and position, or nothing for another message. */
    std::optional<std::pair<int, float>> faderMoved (const juce::MidiMessage&);

    /** A button's LED, lit or dark. */
    juce::MidiMessage led (int note, bool on);

    /** A button the device pressed or let go: its note and whether pressed, or nothing. */
    std::optional<std::pair<int, bool>> button (const juce::MidiMessage&);

    /** A V-pot turned: its strip and the ticks (positive clockwise), or nothing. */
    std::optional<std::pair<int, int>> vpotTurned (const juce::MidiMessage&);

    /** A V-pot's LED ring showing a pan (-1 left to +1 right) as a dot, with the centre LED lit at centre. */
    juce::MidiMessage vpotRing (int strip, float pan);

    /** Text for the scribble strips: from a place (0 to 55 the top line, 56 to 111 the bottom one), ASCII only. */
    juce::MidiMessage scribble (int place, const juce::String& text);

    /** A strip's text, cut or padded to stripChars, ASCII only. */
    juce::String stripText (const juce::String&);
}

/** A control moved from outside the app (a control surface, a MIDI-learned knob): as the
    mixer's own controls, it writes into the track's envelope while the track writes automation
    and the session plays, does nothing to a control that follows its envelope otherwise, and
    else changes the setting (with the track's group) as an undo step merged while it moves. */
namespace ExternalControl
{
    void move (AudioEngine&, MultitrackView* writer, juce::uint32 trackId, AutomationParameter, float value);
    void release (MultitrackView* writer, juce::uint32 trackId, AutomationParameter);

    /** Mute or solo turned on or off, as an undo step (with the track's group). */
    void setSwitch (AudioEngine&, juce::uint32 trackId, bool solo, bool on);
}

//==============================================================================
/** A Mackie Control surface on a MIDI input and output: eight faders in banks over the
    session's tracks and buses (in the mixer's order), V-pots for pan, mute, solo, select and
    record arm, the transport, the scribble strips (each track's name, and its volume or pan as
    it moves), and motorised faders that follow automation as it plays. Messages from the device
    are handled on the message thread; what it shows is sent as it changes, 25 times a second. */
class ControlSurface final : public juce::MidiInputCallback,
                             private juce::AsyncUpdater,
                             private juce::Timer
{
public:
    enum class Transport { play, stop, record, rewind, forward };

    ControlSurface (AudioEngine&, std::function<MultitrackView*()> automationWriter);
    ~ControlSurface() override;

    /** The app's hands: the transport, selecting and arming a track. */
    std::function<void (Transport)> onTransport;
    std::function<void (juce::uint32 trackId)> onSelect;
    std::function<juce::uint32()> selectedTrack;
    std::function<bool (juce::uint32 trackId)> isArmed;
    std::function<void (juce::uint32 trackId, bool armed)> setArmed;

    /** Opens the device's ports by identifier (empty: closes them). Returns an error, or empty. */
    juce::String open (const juce::String& inputId, const juce::String& outputId);

    /** Uses ports already open (a test's virtual ones): the input must call this surface. */
    void use (std::unique_ptr<juce::MidiInput>, std::unique_ptr<juce::MidiOutput>);
    void close();
    bool isOpen() const noexcept                       { return output != nullptr || input != nullptr; }

    /** A message from the device, on the message thread. */
    void handle (const juce::MidiMessage&);

    /** Sends what the device should show now, where it changed (all of it with `everything`). */
    void refresh (bool everything = false);

    /** The first track or bus on strip 1 (a place in the session's list), moved by the bank and channel buttons. */
    int getBankStart() const noexcept                  { return bankStart; }

    /** The track or bus on a strip (0 to 7), or 0. */
    juce::uint32 trackOn (int strip) const;

    void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage&) override;

private:
    void handleAsyncUpdate() override;
    void timerCallback() override;
    void send (const juce::MidiMessage&);
    void moveBank (int by);

    AudioEngine& engine;
    std::function<MultitrackView*()> writer;
    std::unique_ptr<juce::MidiInput> input;
    std::unique_ptr<juce::MidiOutput> output;

    juce::CriticalSection incomingLock;
    std::vector<juce::MidiMessage> incoming;           // from the device's thread, for the message thread

    int bankStart = 0;
    std::array<bool, Mackie::strips> touched {};       // faders held: not moved by the app
    std::array<juce::uint32, Mackie::strips> panTurned {};   // the track whose pan a V-pot moved, until it rests
    std::array<double, Mackie::strips> panTurnedAt {};

    // What the device was last sent, so only changes go.
    struct Shown
    {
        std::array<int, Mackie::strips> fader;
        std::array<int, Mackie::strips> ring;
        std::array<int, 128> leds;
        juce::String top, bottom;
    };

    Shown shown;
    void forget();
};

//==============================================================================
/** MIDI learn: any controller's knob, fader or button (a CC on a channel) moves a control in the
    session. Learning waits for the next CC to come in; bindings are kept in the session (each
    learned an undo step). CCs that are bound, or being learned, don't reach the instruments.
    A volume or pan moved by a knob writes automation as the mixer's controls do; its pass ends
    half a second after the knob last moved. */
class MidiLearn final : private juce::AsyncUpdater,
                        private juce::Timer
{
public:
    MidiLearn (AudioEngine&, std::function<MultitrackView*()> automationWriter);
    ~MidiLearn() override;

    /** The transport, for bindings to it. */
    std::function<void (ControlSurface::Transport)> onTransport;

    /** Told when a binding is learned (message thread). */
    std::function<void (const MidiBinding&)> onLearned;

    /** The next CC to come in moves this target; learning another, or cancel(), stops it. */
    void learn (const MidiBinding::Target&);
    void cancel();
    std::optional<MidiBinding::Target> getLearning() const;

    /** Any thread: a message from a MIDI input. True when it was taken (a bound CC, or the one learned). */
    bool offer (const juce::MidiMessage&);

    /** Message thread: acts on a CC as if it had come in (the queue's, and tests'). */
    void handle (int channel, int controller, int value);

    static constexpr double passRestSeconds = 0.5;

private:
    void handleAsyncUpdate() override;
    void timerCallback() override;
    void refreshBound();

    AudioEngine& engine;
    std::function<MultitrackView*()> writer;
    mutable juce::SpinLock lock;
    std::optional<MidiBinding::Target> learning;       // under the lock
    std::vector<MidiBinding> bound;                    // under the lock: the session's, as last seen
    juce::uint32 boundVersion = 0;                     // the session version they are from (message thread)
    std::vector<std::array<int, 3>> queued;            // under the lock: channel, controller, value
    std::map<std::pair<juce::uint32, int>, double> moving;   // message thread: a track's volume (0) or pan (1), and when it last moved
};
