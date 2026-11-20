#include "ControlSurface.h"
#include "AudioEngine.h"
#include "MultitrackView.h"

namespace Mackie
{
    namespace
    {
        // The mixer fader's range and taper (MixerView: -60 to +12 dB, -12 at the middle).
        constexpr float minDb = -60.0f, maxDb = 12.0f, midDb = -12.0f;

        double skew() noexcept
        {
            return std::log (0.5) / std::log ((double) (midDb - minDb) / (double) (maxDb - minDb));
        }
    }

    float positionOfDb (float db) noexcept
    {
        const auto proportion = (double) (juce::jlimit (minDb, maxDb, db) - minDb) / (double) (maxDb - minDb);
        return (float) std::pow (proportion, skew());
    }

    float dbOfPosition (float position) noexcept
    {
        const auto p = juce::jlimit (0.0, 1.0, (double) position);
        return p <= 0.0 ? minDb : (float) (minDb + (maxDb - minDb) * std::exp (std::log (p) / skew()));
    }

    juce::MidiMessage fader (int strip, float position)
    {
        const auto value = juce::jlimit (0, 16383, (int) std::lround (juce::jlimit (0.0f, 1.0f, position) * 16383.0f));
        return juce::MidiMessage::pitchWheel (juce::jlimit (0, master, strip) + 1, value);
    }

    std::optional<std::pair<int, float>> faderMoved (const juce::MidiMessage& m)
    {
        if (! m.isPitchWheel() || m.getChannel() > master + 1)
            return std::nullopt;

        return std::pair<int, float> { m.getChannel() - 1, (float) m.getPitchWheelValue() / 16383.0f };
    }

    juce::MidiMessage led (int note, bool on)
    {
        return juce::MidiMessage::noteOn (1, juce::jlimit (0, 127, note), (juce::uint8) (on ? 127 : 0));
    }

    std::optional<std::pair<int, bool>> button (const juce::MidiMessage& m)
    {
        if (m.isNoteOn (true) && m.getChannel() == 1)
            return std::pair<int, bool> { m.getNoteNumber(), m.getVelocity() > 0 };

        if (m.isNoteOff() && m.getChannel() == 1)
            return std::pair<int, bool> { m.getNoteNumber(), false };

        return std::nullopt;
    }

    std::optional<std::pair<int, int>> vpotTurned (const juce::MidiMessage& m)
    {
        if (! m.isController() || m.getControllerNumber() < 0x10 || m.getControllerNumber() >= 0x10 + strips)
            return std::nullopt;

        const auto v = m.getControllerValue();
        const auto ticks = v & 0x3f;
        return std::pair<int, int> { m.getControllerNumber() - 0x10, (v & 0x40) != 0 ? -ticks : ticks };
    }

    juce::MidiMessage vpotRing (int strip, float pan)
    {
        const auto place = 1 + (int) std::lround ((juce::jlimit (-1.0f, 1.0f, pan) + 1.0f) * 5.0f);   // 1 to 11, 6 the centre
        return juce::MidiMessage::controllerEvent (1, 0x30 + juce::jlimit (0, strips - 1, strip), (place == 6 ? 0x40 : 0) | place);
    }

    juce::MidiMessage scribble (int place, const juce::String& text)
    {
        std::vector<juce::uint8> data { 0x00, 0x00, 0x66, 0x14, 0x12, (juce::uint8) juce::jlimit (0, 111, place) };

        for (auto c : text)
            data.push_back ((juce::uint8) (c >= 0x20 && c < 0x7f ? c : '?'));

        return juce::MidiMessage::createSysExMessage (data.data(), (int) data.size());
    }

    juce::String stripText (const juce::String& text)
    {
        juce::String out;

        for (auto c : text)
            out += (c >= 0x20 && c < 0x7f) ? juce::String::charToString (c) : juce::String ("?");

        return out.substring (0, stripChars).paddedRight (' ', stripChars);
    }
}

//==============================================================================
namespace ExternalControl
{
    void move (AudioEngine& engine, MultitrackView* w, juce::uint32 trackId, AutomationParameter p, float value)
    {
        auto& player = engine.getPlayer();
        const auto playing = player.isPlaying() && player.getMode() == DocumentPlayerProcessor::Mode::session;

        if (w != nullptr)
        {
            if (w->isAutomationPassUnderWay (trackId, p))
            {
                w->continueAutomationPass (trackId, p, player.getPositionFrames(), value);
                return;
            }

            if (w->isWritingAutomation (trackId) && playing)
            {
                w->startAutomationPass (trackId, p, player.getPositionFrames(), value);
                return;
            }
        }

        auto next = *engine.session.getState();

        for (auto& t : next.tracks)
            if (t.id == trackId)
            {
                if (t.follows (p))
                    return;

                const auto before = t;

                if (p == AutomationParameter::volume)
                    t.gainDb = value;
                else
                    t.pan = juce::jlimit (-1.0f, 1.0f, value);

                SessionEdits::followGroup (next, before);
                engine.session.apply (std::move (next), p == AutomationParameter::volume ? "Track Volume" : "Track Pan", true);
                return;
            }
    }

    void release (MultitrackView* w, juce::uint32 trackId, AutomationParameter p)
    {
        if (w != nullptr)
            w->releaseAutomationPass (trackId, p);
    }

    void setSwitch (AudioEngine& engine, juce::uint32 trackId, bool solo, bool on)
    {
        auto next = *engine.session.getState();

        for (auto& t : next.tracks)
            if (t.id == trackId)
            {
                if ((solo ? t.solo : t.mute) == on)
                    return;

                const auto before = t;
                (solo ? t.solo : t.mute) = on;
                SessionEdits::followGroup (next, before);
                engine.session.apply (std::move (next), solo ? (on ? "Solo" : "Unsolo") : (on ? "Mute" : "Unmute"));
                return;
            }
    }
}

//==============================================================================
ControlSurface::ControlSurface (AudioEngine& e, std::function<MultitrackView*()> w)
    : engine (e), writer (std::move (w))
{
    forget();
}

ControlSurface::~ControlSurface()
{
    close();
}

juce::String ControlSurface::open (const juce::String& inputId, const juce::String& outputId)
{
    close();

    if (inputId.isEmpty() && outputId.isEmpty())
        return {};

    engine.setControlSurfaceInput (inputId);
    auto in = inputId.isNotEmpty() ? juce::MidiInput::openDevice (inputId, this) : nullptr;
    auto out = outputId.isNotEmpty() ? juce::MidiOutput::openDevice (outputId) : nullptr;

    if ((inputId.isNotEmpty() && in == nullptr) || (outputId.isNotEmpty() && out == nullptr))
        return "The control surface's MIDI " + juce::String (in == nullptr && inputId.isNotEmpty() ? "input" : "output") + " could not be opened.";

    use (std::move (in), std::move (out));
    return {};
}

void ControlSurface::use (std::unique_ptr<juce::MidiInput> in, std::unique_ptr<juce::MidiOutput> out)
{
    close();
    input = std::move (in);
    output = std::move (out);

    if (input != nullptr)
        input->start();

    forget();
    refresh (true);
    startTimerHz (25);
}

void ControlSurface::close()
{
    stopTimer();

    if (input != nullptr)
    {
        input->stop();
        engine.setControlSurfaceInput ({});
    }

    input.reset();

    // Dark and blank, the faders down, as it is left.
    if (output != nullptr)
    {
        for (int i = 0; i < Mackie::strips; ++i)
            send (Mackie::fader (i, 0.0f));

        for (int note = 0; note < 128; ++note)
            if (shown.leds[(size_t) note] > 0)
                send (Mackie::led (note, false));

        send (Mackie::scribble (0, juce::String::repeatedString (" ", 112)));
    }

    output.reset();
    forget();
}

void ControlSurface::forget()
{
    shown.fader.fill (-1);
    shown.ring.fill (-1);
    shown.leds.fill (-1);
    shown.top = shown.bottom = "\x01";   // nothing the strips could show
}

void ControlSurface::send (const juce::MidiMessage& m)
{
    if (output != nullptr)
        output->sendMessageNow (m);
}

juce::uint32 ControlSurface::trackOn (int strip) const
{
    const auto state = engine.session.getState();
    const auto place = bankStart + strip;
    return juce::isPositiveAndBelow (strip, Mackie::strips) && juce::isPositiveAndBelow (place, (int) state->tracks.size())
               ? state->tracks[(size_t) place].id : 0;
}

void ControlSurface::moveBank (int by)
{
    const auto count = (int) engine.session.getState()->tracks.size();
    bankStart = juce::jlimit (0, juce::jmax (0, count - 1), bankStart + by);
    refresh();
}

void ControlSurface::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& m)
{
    {
        const juce::ScopedLock sl (incomingLock);
        incoming.push_back (m);
    }

    triggerAsyncUpdate();
}

void ControlSurface::handleAsyncUpdate()
{
    std::vector<juce::MidiMessage> now;

    {
        const juce::ScopedLock sl (incomingLock);
        now.swap (incoming);
    }

    for (auto& m : now)
        handle (m);
}

void ControlSurface::handle (const juce::MidiMessage& m)
{
    auto* w = writer != nullptr ? writer() : nullptr;

    if (const auto moved = Mackie::faderMoved (m))
    {
        if (const auto id = trackOn (moved->first); id != 0 && moved->first < Mackie::strips)
        {
            const auto db = Mackie::dbOfPosition (moved->second);
            ExternalControl::move (engine, w, id, AutomationParameter::volume, db);
            shown.fader[(size_t) moved->first] = (int) std::lround (moved->second * 16383.0f);   // where the device's fader is
        }

        return;
    }

    if (const auto turned = Mackie::vpotTurned (m))
    {
        const auto strip = turned->first;

        if (const auto id = trackOn (strip); id != 0)
            if (const auto* t = engine.session.getState()->findTrack (id))
            {
                const auto frame = engine.getPlayer().getPositionFrames();
                ExternalControl::move (engine, w, id, AutomationParameter::pan, t->valueAt (AutomationParameter::pan, frame) + 0.02f * (float) turned->second);
                panTurned[(size_t) strip] = id;
                panTurnedAt[(size_t) strip] = juce::Time::getMillisecondCounterHiRes();
            }

        return;
    }

    const auto pressed = Mackie::button (m);

    if (! pressed.has_value())
        return;

    const auto [note, down] = *pressed;

    // A fader held or let go: while held, the app doesn't move it; let go, its pass ends.
    if (note >= Mackie::faderTouch && note < Mackie::faderTouch + Mackie::strips)
    {
        const auto strip = note - Mackie::faderTouch;
        touched[(size_t) strip] = down;

        if (! down)
            if (const auto id = trackOn (strip); id != 0)
                ExternalControl::release (w, id, AutomationParameter::volume);

        return;
    }

    if (! down)
        return;

    const auto strip = note & 7;
    const auto id = trackOn (strip);
    const auto* t = id != 0 ? engine.session.getState()->findTrack (id) : nullptr;

    if (note < Mackie::vpotPush + Mackie::strips && t != nullptr)
    {
        if (note >= Mackie::vpotPush)
            ExternalControl::move (engine, w, id, AutomationParameter::pan, 0.0f);   // a V-pot pushed: centred
        else if (note >= Mackie::select)
        {
            if (onSelect != nullptr)
                onSelect (id);
        }
        else if (note >= Mackie::mute)
            ExternalControl::setSwitch (engine, id, false, ! t->mute);
        else if (note >= Mackie::solo)
            ExternalControl::setSwitch (engine, id, true, ! t->solo);
        else if (! t->isBus && ! t->isControl() && isArmed != nullptr && setArmed != nullptr)
            setArmed (id, ! isArmed (id));

        refresh();
        return;
    }

    switch (note)
    {
        case Mackie::bankLeft:     moveBank (-Mackie::strips); break;
        case Mackie::bankRight:    moveBank (Mackie::strips); break;
        case Mackie::channelLeft:  moveBank (-1); break;
        case Mackie::channelRight: moveBank (1); break;
        case Mackie::play:         if (onTransport != nullptr) onTransport (Transport::play); break;
        case Mackie::stop:         if (onTransport != nullptr) onTransport (Transport::stop); break;
        case Mackie::record:       if (onTransport != nullptr) onTransport (Transport::record); break;
        case Mackie::rewind:       if (onTransport != nullptr) onTransport (Transport::rewind); break;
        case Mackie::forward:      if (onTransport != nullptr) onTransport (Transport::forward); break;
        default: break;
    }
}

void ControlSurface::timerCallback()
{
    // A V-pot at rest half a second: its pan pass ends.
    const auto now = juce::Time::getMillisecondCounterHiRes();

    for (int i = 0; i < Mackie::strips; ++i)
        if (panTurned[(size_t) i] != 0 && now - panTurnedAt[(size_t) i] > 500.0)
        {
            ExternalControl::release (writer != nullptr ? writer() : nullptr, panTurned[(size_t) i], AutomationParameter::pan);
            panTurned[(size_t) i] = 0;
        }

    refresh();
}

void ControlSurface::refresh (bool everything)
{
    if (output == nullptr)
        return;

    if (everything)
        forget();

    const auto state = engine.session.getState();
    auto& player = engine.getPlayer();
    const auto frame = player.getPositionFrames();
    const auto selected = selectedTrack != nullptr ? selectedTrack() : 0;
    juce::String top, bottom;
    std::array<int, 128> leds;
    leds.fill (0);

    for (int i = 0; i < Mackie::strips; ++i)
    {
        const auto id = trackOn (i);
        const auto* t = id != 0 ? state->findTrack (id) : nullptr;
        const auto db = t != nullptr ? t->valueAt (AutomationParameter::volume, frame) : Mackie::dbOfPosition (0.0f);
        const auto pan = t != nullptr ? t->valueAt (AutomationParameter::pan, frame) : 0.0f;

        // Motorised faders follow the volume (its envelope as it plays), unless held.
        if (! touched[(size_t) i])
        {
            const auto value = (int) std::lround (Mackie::positionOfDb (t != nullptr ? db : -60.0f) * 16383.0f);

            if (value != shown.fader[(size_t) i])
            {
                send (Mackie::fader (i, (float) value / 16383.0f));
                shown.fader[(size_t) i] = value;
            }
        }

        const auto ring = t != nullptr ? (int) std::lround ((pan + 1.0f) * 5.0f) : -2;

        if (ring != shown.ring[(size_t) i])
        {
            send (t != nullptr ? Mackie::vpotRing (i, pan) : juce::MidiMessage::controllerEvent (1, 0x30 + i, 0));
            shown.ring[(size_t) i] = ring;
        }

        if (t != nullptr)
        {
            leds[(size_t) (Mackie::mute + i)] = t->mute ? 1 : 0;
            leds[(size_t) (Mackie::solo + i)] = t->solo ? 1 : 0;
            leds[(size_t) (Mackie::select + i)] = id == selected ? 1 : 0;
            leds[(size_t) (Mackie::recordArm + i)] = isArmed != nullptr && isArmed (id) ? 1 : 0;
        }

        // The top line names the tracks; the bottom shows the volume, or the pan while a V-pot turns it.
        top += Mackie::stripText (t != nullptr ? t->name : juce::String());

        juce::String value;

        if (t != nullptr && panTurned[(size_t) i] == id)
            value = std::abs (pan) < 0.005f ? juce::String ("C") : juce::String (std::lround (std::abs (pan) * 100.0f)) + (pan < 0.0f ? "L" : "R");
        else if (t != nullptr)
            value = db <= -60.0f ? juce::String ("-inf") : juce::String (db, 1);

        bottom += Mackie::stripText (value.paddedLeft (' ', 6));
    }

    const auto playing = player.isPlaying() && player.getMode() == DocumentPlayerProcessor::Mode::session;
    leds[(size_t) Mackie::play] = playing ? 1 : 0;
    leds[(size_t) Mackie::stop] = playing ? 0 : 1;

    for (int note = 0; note < 128; ++note)
        if (leds[(size_t) note] != shown.leds[(size_t) note] && (shown.leds[(size_t) note] >= 0 || leds[(size_t) note] > 0 || everything))
        {
            send (Mackie::led (note, leds[(size_t) note] > 0));
            shown.leds[(size_t) note] = leds[(size_t) note];
        }

    if (top != shown.top)
    {
        send (Mackie::scribble (0, top));
        shown.top = top;
    }

    if (bottom != shown.bottom)
    {
        send (Mackie::scribble (56, bottom));
        shown.bottom = bottom;
    }
}

//==============================================================================
MidiLearn::MidiLearn (AudioEngine& e, std::function<MultitrackView*()> w)
    : engine (e), writer (std::move (w))
{
    refreshBound();
    startTimer (100);
}

MidiLearn::~MidiLearn()
{
    stopTimer();
    cancelPendingUpdate();
}

void MidiLearn::learn (const MidiBinding::Target& target)
{
    const juce::SpinLock::ScopedLockType sl (lock);
    learning = target;
}

void MidiLearn::cancel()
{
    const juce::SpinLock::ScopedLockType sl (lock);
    learning.reset();
}

std::optional<MidiBinding::Target> MidiLearn::getLearning() const
{
    const juce::SpinLock::ScopedLockType sl (lock);
    return learning;
}

bool MidiLearn::offer (const juce::MidiMessage& m)
{
    if (! m.isController())
        return false;

    {
        const juce::SpinLock::ScopedLockType sl (lock);
        const auto channel = m.getChannel(), controller = m.getControllerNumber();
        const auto isBound = std::any_of (bound.begin(), bound.end(), [&] (auto& b) { return b.channel == channel && b.controller == controller; });

        if (! learning.has_value() && ! isBound)
            return false;

        if (queued.size() < 1024)
            queued.push_back ({ channel, controller, m.getControllerValue() });
    }

    triggerAsyncUpdate();
    return true;
}

void MidiLearn::handleAsyncUpdate()
{
    std::vector<std::array<int, 3>> now;

    {
        const juce::SpinLock::ScopedLockType sl (lock);
        now.swap (queued);
    }

    for (auto& [channel, controller, value] : now)
        handle (channel, controller, value);
}

void MidiLearn::handle (int channel, int controller, int value)
{
    // Learning: this controller now moves the target.
    if (const auto target = getLearning())
    {
        cancel();
        MidiBinding b;
        b.channel = channel;
        b.controller = controller;
        b.target = *target;
        auto next = *engine.session.getState();

        if (SessionEdits::bindMidi (next, b))
            engine.session.apply (std::move (next), "MIDI Learn");

        refreshBound();

        if (onLearned != nullptr)
            onLearned (b);

        return;
    }

    refreshBound();
    const auto state = engine.session.getState();
    const auto found = std::find_if (state->midiBindings.begin(), state->midiBindings.end(), [&] (auto& b)
    {
        return b.channel == channel && b.controller == controller;
    });

    if (found == state->midiBindings.end())
        return;

    const auto target = found->target;
    const auto pressed = value >= 64;
    auto* w = writer != nullptr ? writer() : nullptr;
    const auto* t = target.isTrack() ? state->findTrack (target.trackId) : nullptr;

    switch (target.kind)
    {
        case MidiBinding::Kind::volume:
        case MidiBinding::Kind::pan:
        {
            if (t == nullptr)
                return;

            const auto volume = target.kind == MidiBinding::Kind::volume;
            const auto v = volume ? Mackie::dbOfPosition ((float) value / 127.0f) : juce::jlimit (-1.0f, 1.0f, (float) (value - 64) / 63.0f);
            ExternalControl::move (engine, w, target.trackId, volume ? AutomationParameter::volume : AutomationParameter::pan, v);
            moving[{ target.trackId, volume ? 0 : 1 }] = juce::Time::getMillisecondCounterHiRes();
            break;
        }

        case MidiBinding::Kind::mute:
        case MidiBinding::Kind::solo:
            if (t != nullptr && pressed)
                ExternalControl::setSwitch (engine, target.trackId, target.kind == MidiBinding::Kind::solo,
                                            target.kind == MidiBinding::Kind::solo ? ! t->solo : ! t->mute);
            break;

        case MidiBinding::Kind::play:
        case MidiBinding::Kind::stop:
        case MidiBinding::Kind::record:
            if (pressed && onTransport != nullptr)
                onTransport (target.kind == MidiBinding::Kind::play ? ControlSurface::Transport::play
                           : target.kind == MidiBinding::Kind::stop ? ControlSurface::Transport::stop : ControlSurface::Transport::record);
            break;
    }
}

void MidiLearn::timerCallback()
{
    refreshBound();

    // A knob at rest: its volume or pan pass ends.
    const auto now = juce::Time::getMillisecondCounterHiRes();

    for (auto it = moving.begin(); it != moving.end();)
    {
        if (now - it->second > passRestSeconds * 1000.0)
        {
            ExternalControl::release (writer != nullptr ? writer() : nullptr, it->first.first,
                                      it->first.second == 0 ? AutomationParameter::volume : AutomationParameter::pan);
            it = moving.erase (it);
        }
        else
        {
            ++it;
        }
    }
}

void MidiLearn::refreshBound()
{
    if (const auto v = engine.session.getVersion(); v != boundVersion || boundVersion == 0)
    {
        auto now = engine.session.getState()->midiBindings;
        boundVersion = v;
        const juce::SpinLock::ScopedLockType sl (lock);
        bound.swap (now);
    }
}
