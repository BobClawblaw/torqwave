#include "SequenceMidi.h"
#include "NoteTools.h"

namespace SequenceMidi
{
namespace
{
    using SequenceFile::ticksPerBeat;

    /** Sharps (above 0) or flats (below) of a major or minor key's signature; nothing for other modes. */
    std::optional<std::pair<int, bool>> signatureOf (const juce::String& key)
    {
        const auto scale = NoteTools::parseKey (key);

        if (! scale.has_value() || (scale->mode != "major" && scale->mode != "minor"))
            return std::nullopt;

        const auto minor = scale->mode == "minor";
        const auto major = (scale->root + (minor ? 3 : 0)) % 12;   // a minor key's relative major
        auto sharps = (major * 7) % 12;                             // steps round the circle of fifths

        if (sharps > 6)
            sharps -= 12;

        return std::pair<int, bool> { sharps, minor };
    }

    juce::String keyOfSignature (int sharps, bool minor)
    {
        const auto major = ((sharps * 7) % 12 + 12) % 12;
        auto scale = NoteTools::parseKey (minor ? "C minor" : "C major");
        scale->root = minor ? (major + 9) % 12 : major;
        return scale->name();
    }

    /** The passes of a sequence's notes: calls `each` with every pass's start, in ticks from the
        sequence's start, while it is before the end. */
    void forEachPass (const SequenceFile::Sequence& q, const std::function<void (juce::int64 shift)>& each)
    {
        const auto period = q.loopLength > 0 ? q.loopLength : q.offset + q.length;

        if (q.length <= 0 || period <= 0)
            return;

        for (juce::int64 pass = 0; pass * period - q.offset < q.length; ++pass)
            each (pass * period - q.offset);
    }

    juce::int64 periodOf (const SequenceFile::Sequence& q)
    {
        return q.loopLength > 0 ? q.loopLength : q.offset + q.length;
    }
}

std::vector<SequenceFile::Note> playedNotes (const SequenceFile::Sequence& q)
{
    std::vector<SequenceFile::Note> out;
    const auto period = periodOf (q);

    forEachPass (q, [&] (juce::int64 shift)
    {
        for (auto& n : q.notes)
        {
            const auto start = n.start + shift;

            if (n.start >= period || start < 0 || start >= q.length)
                continue;

            auto played = n;
            played.start = start;
            played.length = juce::jmin (start + n.length, q.length) - start;
            out.push_back (played);
        }
    });

    std::stable_sort (out.begin(), out.end(), [] (auto& a, auto& b) { return a.start != b.start ? a.start < b.start : a.pitch < b.pitch; });
    return out;
}

juce::MidiFile toMidiFile (const std::vector<SequenceFile::Sequence>& sequences)
{
    juce::MidiFile file;
    file.setTicksPerQuarterNote (ticksPerBeat);
    juce::int64 end = 0;

    for (auto& q : sequences)
        end = juce::jmax (end, q.length);

    juce::MidiMessageSequence conductor;

    if (! sequences.empty())
    {
        const auto& first = sequences.front();
        conductor.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::lround (60000000.0 / juce::jlimit (20.0, 400.0, first.writtenTempo))), 0.0);
        conductor.addEvent (juce::MidiMessage::timeSignatureMetaEvent (first.beatsPerBar, 4), 0.0);

        if (const auto signature = signatureOf (first.key))
            conductor.addEvent (juce::MidiMessage::keySignatureMetaEvent (signature->first, signature->second), 0.0);
    }

    conductor.addEvent (juce::MidiMessage::endOfTrack(), (double) end);
    file.addTrack (conductor);

    for (auto& q : sequences)
    {
        // Each tick's note-offs before its note-ons, so a note taking over from another of the
        // same pitch is read back as two notes.
        struct Event
        {
            juce::int64 tick;
            int order;   // 0 a controller, 1 a note-off, 2 a note-on
            juce::MidiMessage message;
        };

        std::vector<Event> events;

        for (auto& n : playedNotes (q))
        {
            const auto velocity = (juce::uint8) juce::jlimit (1, 127, (int) std::lround (n.velocity * 127.0f));
            events.push_back ({ n.start, 2, juce::MidiMessage::noteOn (q.channel, n.pitch, velocity) });
            events.push_back ({ n.start + n.length, 1, juce::MidiMessage::noteOff (q.channel, n.pitch) });
        }

        const auto period = periodOf (q);

        for (auto& lane : q.lanes)
            forEachPass (q, [&] (juce::int64 shift)
            {
                for (auto& p : lane.points)
                    if (const auto tick = p.tick + shift; p.tick < period && tick >= 0 && tick < q.length)
                        events.push_back ({ tick, 0, NoteClip::laneMessage (lane.controller, p.value, q.channel) });
            });

        std::stable_sort (events.begin(), events.end(), [] (auto& a, auto& b) { return a.tick != b.tick ? a.tick < b.tick : a.order < b.order; });

        juce::MidiMessageSequence track;
        track.addEvent (juce::MidiMessage::textMetaEvent (3, q.name), 0.0);

        for (auto& e : events)
            track.addEvent (e.message, (double) e.tick);

        track.addEvent (juce::MidiMessage::endOfTrack(), (double) q.length);
        file.addTrack (track);
    }

    return file;
}

juce::String exportFile (const std::vector<SequenceFile::Sequence>& sequences, const juce::File& target)
{
    if (sequences.empty())
        return "There is nothing to export.";

    const auto midi = toMidiFile (sequences);
    juce::TemporaryFile temp (target);

    {
        juce::FileOutputStream out (temp.getFile());

        if (! out.openedOk() || ! midi.writeTo (out, 1))
            return "Couldn't write " + target.getFullPathName() + ".";
    }

    if (! temp.overwriteTargetFileWithTemporary())
        return "Couldn't write " + target.getFullPathName() + ".";

    return {};
}

Imported read (const juce::MidiFile& original, const juce::String& fileName)
{
    Imported out;
    auto midi = original;
    const auto format = midi.getTimeFormat();

    // The first tempo, time signature and key anywhere in the file.
    double tempo = 120.0;
    int beatsPerBar = 4;
    juce::String key;
    bool hasTempo = false, hasSignature = false, hasKey = false;

    for (int t = 0; t < midi.getNumTracks(); ++t)
        for (auto* event : *midi.getTrack (t))
        {
            const auto& m = event->message;

            if (m.isTempoMetaEvent() && ! hasTempo && m.getTempoSecondsPerQuarterNote() > 0.0)
            {
                tempo = juce::jlimit (20.0, 400.0, 60.0 / m.getTempoSecondsPerQuarterNote());
                hasTempo = true;
            }
            else if (m.isTimeSignatureMetaEvent() && ! hasSignature)
            {
                int numerator = 4, denominator = 4;
                m.getTimeSignatureInfo (numerator, denominator);
                beatsPerBar = juce::jlimit (1, 32, denominator > 0 ? (int) std::lround (numerator * 4.0 / denominator) : 4);
                hasSignature = true;
            }
            else if (m.isKeySignatureMetaEvent() && ! hasKey)
            {
                key = keyOfSignature (m.getKeySignatureNumberOfSharpsOrFlats(), ! m.isKeySignatureMajorKey());
                hasKey = true;
            }
        }

    // Ticks a beat, or for a file timed in SMPTE frames, through seconds at its first tempo.
    if (format <= 0)
        midi.convertTimestampTicksToSeconds();

    const auto tickOf = [format, tempo] (double stamp)
    {
        return format > 0 ? (juce::int64) std::llround (stamp * ticksPerBeat / format)
                          : (juce::int64) std::llround (stamp * tempo / 60.0 * ticksPerBeat);
    };

    int withNotes = 0;

    for (int t = 0; t < midi.getNumTracks(); ++t)
        for (auto* event : *midi.getTrack (t))
            if (event->message.isNoteOn())
            {
                ++withNotes;
                break;
            }

    int number = 0;

    for (int t = 0; t < midi.getNumTracks(); ++t)
    {
        auto track = *midi.getTrack (t);
        track.updateMatchedPairs();
        SequenceFile::Sequence q;
        q.writtenTempo = tempo;
        q.beatsPerBar = beatsPerBar;
        q.key = key;
        juce::String trackName;
        juce::int64 last = 0;
        int channel = 0;
        std::map<int, SequenceFile::Lane> lanes;

        for (auto* event : track)
        {
            const auto& m = event->message;
            const auto tick = juce::jmax ((juce::int64) 0, tickOf (m.getTimeStamp()));

            if (m.isTrackNameEvent() && trackName.isEmpty())
                trackName = m.getTextFromTextMetaEvent().trim();

            if (m.isController() || m.isPitchWheel())
            {
                const auto controller = m.isPitchWheel() ? ControllerLane::pitchBend : m.getControllerNumber();
                auto& lane = lanes[controller];
                lane.controller = controller;
                lane.points.push_back ({ tick, m.isPitchWheel() ? (float) m.getPitchWheelValue() / 16383.0f : (float) m.getControllerValue() / 127.0f });
                last = juce::jmax (last, tick);
            }

            if (! m.isNoteOn() || event->noteOffObject == nullptr)
                continue;

            const auto off = juce::jmax (tick + 1, tickOf (event->noteOffObject->message.getTimeStamp()));
            q.notes.push_back ({ m.getNoteNumber(), tick, off - tick, m.getFloatVelocity() });
            last = juce::jmax (last, off);

            if (channel == 0)
                channel = m.getChannel();
        }

        if (q.notes.empty())
            continue;

        ++number;
        const auto base = juce::File::createLegalFileName (fileName).upToLastOccurrenceOf (".", false, false);
        q.name = trackName.isNotEmpty() ? trackName : withNotes > 1 ? base + " " + juce::String (number) : base;
        q.channel = juce::jlimit (1, 16, channel);

        const auto bar = (juce::int64) ticksPerBeat * beatsPerBar;
        q.length = juce::jmax ((juce::int64) 1, (last + bar - 1) / bar) * bar;

        for (auto& [controller, lane] : lanes)
            q.lanes.push_back (std::move (lane));

        std::stable_sort (q.notes.begin(), q.notes.end(), [] (auto& a, auto& b) { return a.start < b.start; });
        out.sequences.push_back (std::move (q));
    }

    if (out.sequences.empty())
        out.error = "no notes in it.";

    return out;
}

Imported importFile (const juce::File& file)
{
    juce::FileInputStream in (file);
    juce::MidiFile midi;

    if (! in.openedOk())
        return { {}, "Could not read " + file.getFileName() + "." };

    if (! midi.readFrom (in, true))
        return { {}, file.getFileName() + " is not a Standard MIDI File." };

    auto imported = read (midi, file.getFileName());

    if (imported.error.isNotEmpty())
        imported.error = file.getFileName() + ": " + imported.error;

    return imported;
}
}
