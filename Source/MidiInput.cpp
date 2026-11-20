#include "MidiInput.h"
#include "Theme.h"

namespace MidiIn
{
    Live::Live()
    {
        held.fill (-1);
        heldVelocity.fill (0.0f);
        remembered.resize (rememberedSize);
        collector.reset (rate);
    }

    void Live::prepare (double sampleRate)
    {
        rate = sampleRate;
        collector.reset (sampleRate);
        sounded.reset();
        playing = track.load();
    }

    void Live::addMessage (const juce::MidiMessage& m)
    {
        auto stamped = m;

        if (stamped.getTimeStamp() <= 0.0)
            stamped.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);

        collector.addMessageToQueue (stamped);
    }

    bool Live::takeBlock (juce::MidiBuffer& into, int numFrames)
    {
        // A new track takes over only between notes.
        if (sounded.none())
            playing = track.load();

        into.clear();
        collector.removeNextBlockOfMessages (into, numFrames);
        const auto any = ! into.isEmpty();

        for (const auto meta : into)
        {
            const auto m = meta.getMessage();

            if (m.isNoteOn())
                sounded.set ((size_t) m.getNoteNumber());
            else if (m.isNoteOff())
                sounded.reset ((size_t) m.getNoteNumber());
            else if (m.isAllNotesOff() || m.isAllSoundOff())
                sounded.reset();
        }

        if (any)
            sounding = (juce::int64) (tailSeconds * rate);
        else if (sounding > 0)
            sounding = juce::jmax ((juce::int64) 0, sounding.load() - numFrames);

        // A note whose off never came (a device unplugged) stops holding the track after the tail.
        if (sounding == 0)
            sounded.reset();

        return any;
    }

    void Live::startCapture()
    {
        const juce::SpinLock::ScopedLockType sl (captureLock);
        captured.clear();
        controllers.clear();
        held.fill (-1);
        capturing = true;
    }

    Capture Live::stopCapture (juce::int64 endFrame)
    {
        capturing = false;
        const juce::SpinLock::ScopedLockType sl (captureLock);

        for (int pitch = 0; pitch < 128; ++pitch)
            if (held[(size_t) pitch] >= 0)
            {
                if (endFrame > held[(size_t) pitch])
                    captured.push_back ({ pitch, held[(size_t) pitch], endFrame, heldVelocity[(size_t) pitch] });

                held[(size_t) pitch] = -1;
            }

        return { std::exchange (captured, {}), std::exchange (controllers, {}) };
    }

    namespace
    {
        /** A message into a capture at a frame: a note-on starts a note (ending one held at that
            pitch), a note-off or All Notes Off ends it, a controller or the pitch bend is a move. */
        void addToCapture (const juce::MidiMessage& m, juce::int64 frame, std::vector<CapturedNote>& notes,
                           std::vector<CapturedController>& controllers, std::array<juce::int64, 128>& held, std::array<float, 128>& velocity)
        {
            if (m.isNoteOn())
            {
                const auto pitch = (size_t) m.getNoteNumber();

                if (held[pitch] >= 0 && frame > held[pitch])
                    notes.push_back ({ (int) pitch, held[pitch], frame, velocity[pitch] });

                held[pitch] = frame;
                velocity[pitch] = m.getFloatVelocity();
            }
            else if (m.isController())
            {
                controllers.push_back ({ m.getControllerNumber(), frame, (float) m.getControllerValue() / 127.0f });
            }
            else if (m.isPitchWheel())
            {
                controllers.push_back ({ ControllerLane::pitchBend, frame, (float) m.getPitchWheelValue() / 16383.0f });
            }
            else if (m.isNoteOff() || m.isAllNotesOff())
            {
                for (int pitch = 0; pitch < 128; ++pitch)
                {
                    if (! (m.isAllNotesOff() || pitch == m.getNoteNumber()) || held[(size_t) pitch] < 0)
                        continue;

                    if (frame > held[(size_t) pitch])
                        notes.push_back ({ pitch, held[(size_t) pitch], frame, velocity[(size_t) pitch] });

                    held[(size_t) pitch] = -1;
                }
            }
        }
    }

    void Live::captureBlock (const juce::MidiBuffer& block, juce::int64 blockFrame)
    {
        if (! capturing || block.isEmpty())
            return;

        // The message thread holds the lock only briefly (start, stop); a block that finds it held is lost.
        const juce::SpinLock::ScopedTryLockType sl (captureLock);

        if (! sl.isLocked())
            return;

        for (const auto meta : block)
            addToCapture (meta.getMessage(), blockFrame + meta.samplePosition, captured, controllers, held, heldVelocity);
    }

    void Live::rememberBlock (const juce::MidiBuffer& block, int numFrames, juce::int64 sessionFrame)
    {
        const auto at = clock.load();
        clock = at + numFrames;

        if (block.isEmpty())
            return;

        const juce::SpinLock::ScopedTryLockType sl (captureLock);

        if (! sl.isLocked() || remembered.empty())
            return;

        for (const auto meta : block)
        {
            const auto m = meta.getMessage();

            if (! (m.isNoteOnOrOff() || m.isAllNotesOff() || m.isController() || m.isPitchWheel()) || m.getRawDataSize() > 3)
                continue;

            auto& r = remembered[rememberedNext];
            r.clock = at + meta.samplePosition;
            r.sessionFrame = sessionFrame >= 0 ? sessionFrame + meta.samplePosition : -1;
            std::memcpy (r.bytes, m.getRawData(), (size_t) m.getRawDataSize());
            rememberedNext = (rememberedNext + 1) % rememberedSize;
            rememberedCount = juce::jmin (rememberedSize, rememberedCount + 1);
        }
    }

    Live::Recent Live::takeRecent()
    {
        std::vector<Remembered> events;

        {
            const juce::SpinLock::ScopedLockType sl (captureLock);
            const auto oldest = clock.load() - (juce::int64) (retroSeconds * rate);

            for (size_t i = 0; i < rememberedCount; ++i)
            {
                const auto& r = remembered[(rememberedNext + rememberedSize - rememberedCount + i) % rememberedSize];

                if (r.clock >= oldest)
                    events.push_back (r);
            }

            rememberedCount = 0;
        }

        Recent recent;
        recent.rate = rate;

        if (events.empty())
            return recent;

        // Played while the session played, or else while it was stopped: the last message says which.
        recent.whilePlaying = events.back().sessionFrame >= 0;
        std::array<juce::int64, 128> on;
        std::array<float, 128> velocity {};
        on.fill (-1);
        juce::int64 first = -1, last = 0;

        for (auto& r : events)
        {
            if ((r.sessionFrame >= 0) != recent.whilePlaying)
                continue;

            if (first < 0)
                first = r.clock;

            const auto frame = recent.whilePlaying ? r.sessionFrame : r.clock - first;
            const juce::MidiMessage m (r.bytes, juce::MidiMessage::getMessageLengthFromFirstByte (r.bytes[0]), 0.0);
            addToCapture (m, frame, recent.capture.notes, recent.capture.controllers, on, velocity);
            last = juce::jmax (last, frame);
        }

        for (int pitch = 0; pitch < 128; ++pitch)
            if (on[(size_t) pitch] >= 0)
                recent.capture.notes.push_back ({ pitch, on[(size_t) pitch], juce::jmax (on[(size_t) pitch] + 1, last), velocity[(size_t) pitch] });

        std::sort (recent.capture.notes.begin(), recent.capture.notes.end(), [] (auto& a, auto& b) { return a.on < b.on; });
        return recent;
    }

    //==============================================================================
    NoteClip clipOf (const Capture& capture, juce::int64 from)
    {
        NoteClip clip;
        clip.name = "Recorded";
        const auto& notes = capture.notes;

        if (notes.empty())
            return clip;

        auto first = std::numeric_limits<juce::int64>::max();
        juce::int64 last = 0;

        for (auto& n : notes)
        {
            first = juce::jmin (first, n.on);
            last = juce::jmax (last, n.off);
        }

        for (auto& c : capture.controllers)
            last = juce::jmax (last, c.frame + 1);

        clip.position = juce::jmax ((juce::int64) 0, juce::jmin (first, from));
        clip.length = juce::jmax ((juce::int64) 1, last - clip.position);

        for (auto& n : notes)
            clip.notes.push_back ({ juce::jlimit (0, 127, n.pitch), n.on - clip.position, juce::jmax ((juce::int64) 1, n.off - n.on), juce::jlimit (0.0f, 1.0f, n.velocity) });

        std::sort (clip.notes.begin(), clip.notes.end(), [] (const ClipNote& a, const ClipNote& b) { return a.start < b.start; });

        // Each controller's moves as a lane, in time order (moves before the clip start at 0).
        for (auto& c : capture.controllers)
        {
            auto* lane = const_cast<ControllerLane*> (clip.findLane (c.controller));

            if (lane == nullptr)
            {
                clip.lanes.push_back ({ c.controller, {} });
                lane = &clip.lanes.back();
            }

            lane->envelope.add (juce::jmax ((juce::int64) 0, c.frame - clip.position), juce::jlimit (0.0f, 1.0f, c.value));
        }

        return clip;
    }

    NoteClip clipOfRecent (const Live::Recent& recent, juce::int64 cursor, double sessionRate)
    {
        auto capture = recent.capture;

        if (! recent.whilePlaying)
        {
            const auto scale = sessionRate / recent.rate;
            const auto at = [&] (juce::int64 frame) { return juce::jmax ((juce::int64) 0, cursor) + (juce::int64) std::llround ((double) frame * scale); };

            for (auto& n : capture.notes)
            {
                n.on = at (n.on);
                n.off = juce::jmax (n.on + 1, at (n.off));
            }

            for (auto& c : capture.controllers)
                c.frame = at (c.frame);
        }

        auto clip = clipOf (capture, std::numeric_limits<juce::int64>::max());
        clip.name = "Captured";
        return clip;
    }

    Imported importFile (const juce::File& file, double sampleRate)
    {
        juce::FileInputStream in (file);

        if (! in.openedOk())
            return { {}, "Could not read " + file.getFileName() + "." };

        auto imported = importFile (in, sampleRate);

        if (imported.error.isNotEmpty())
            imported.error = file.getFileName() + ": " + imported.error;

        return imported;
    }

    Imported importFile (juce::InputStream& in, double sampleRate)
    {
        juce::MidiFile midi;

        if (! midi.readFrom (in, true))
            return { {}, "not a Standard MIDI File." };

        midi.convertTimestampTicksToSeconds();
        Imported out;

        for (int t = 0; t < midi.getNumTracks(); ++t)
        {
            const auto* track = midi.getTrack (t);

            if (track == nullptr)
                continue;

            auto sequence = *track;
            sequence.updateMatchedPairs();
            NoteClip clip;
            juce::int64 last = 0;

            for (int i = 0; i < sequence.getNumEvents(); ++i)
            {
                const auto* event = sequence.getEventPointer (i);
                const auto& m = event->message;

                if (m.isTrackNameEvent() && clip.name.isEmpty())
                    clip.name = m.getTextFromTextMetaEvent().trim();

                if (! m.isNoteOn() || event->noteOffObject == nullptr)
                    continue;

                const auto on = (juce::int64) std::llround (m.getTimeStamp() * sampleRate);
                const auto off = juce::jmax (on + 1, (juce::int64) std::llround (event->noteOffObject->message.getTimeStamp() * sampleRate));
                clip.notes.push_back ({ m.getNoteNumber(), on, off - on, m.getFloatVelocity() });
                last = juce::jmax (last, off);
            }

            if (clip.notes.empty())
                continue;

            if (clip.name.isEmpty())
                clip.name = "Track " + juce::String (t + 1);

            clip.length = last;
            std::sort (clip.notes.begin(), clip.notes.end(), [] (const ClipNote& a, const ClipNote& b) { return a.start < b.start; });
            out.clips.push_back (std::move (clip));
        }

        if (out.clips.empty())
            out.error = "no notes in it.";

        return out;
    }

    //==============================================================================
    KeyboardPanel::KeyboardPanel (Live& l) : live (l)
    {
        addAndMakeVisible (keyboard);
        keyboard.setAvailableRange (36, 96);
        keyboard.setOctaveForMiddleC (4);
        state.addListener (this);
        addAndMakeVisible (status);
        status.setFont (juce::Font (juce::FontOptions (12.0f)));
        Theme::follow (status, juce::Label::textColourId, Theme::dimText);
        status.setText ("Plays the instrument of the track being worked on.", juce::dontSendNotification);
    }

    KeyboardPanel::~KeyboardPanel()
    {
        state.removeListener (this);
    }

    void KeyboardPanel::setStatus (const juce::String& text)
    {
        status.setText (text, juce::dontSendNotification);
    }

    void KeyboardPanel::resized()
    {
        auto area = getLocalBounds();
        status.setBounds (area.removeFromTop (22).reduced (6, 0));
        keyboard.setBounds (area.reduced (4));
    }

    void KeyboardPanel::paint (juce::Graphics& g)
    {
        g.fillAll (Theme::base);
    }

    void KeyboardPanel::handleNoteOn (juce::MidiKeyboardState*, int channel, int note, float velocity)
    {
        live.addMessage (juce::MidiMessage::noteOn (channel, note, velocity));
    }

    void KeyboardPanel::handleNoteOff (juce::MidiKeyboardState*, int channel, int note, float velocity)
    {
        live.addMessage (juce::MidiMessage::noteOff (channel, note, velocity));
    }
}
