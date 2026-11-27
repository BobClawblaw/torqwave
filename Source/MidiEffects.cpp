#include "MidiEffects.h"

namespace BuiltInEffects
{
namespace
{
    juce::PluginDescription describeMidiFx (const char* name, const char* id, int uid)
    {
        juce::PluginDescription d;
        d.name = d.descriptiveName = name;
        d.pluginFormatName = formatName;
        d.category = "MIDI FX";
        d.manufacturerName = "Torqwave";
        d.version = "1";
        d.fileOrIdentifier = id;
        d.uniqueId = d.deprecatedUid = uid;
        d.isInstrument = false;
        d.numInputChannels = d.numOutputChannels = 2;
        return d;
    }

    const juce::StringArray& midiDivisions()
    {
        static const juce::StringArray n { "1/2", "1/4 D", "1/4", "1/4 T", "1/8 D", "1/8", "1/8 T", "1/16 D", "1/16", "1/16 T", "1/32" };
        return n;
    }

    double midiDivisionBeats (int idx) noexcept
    {
        static constexpr double b[] = { 2.0, 1.5, 1.0, 2.0 / 3.0, 0.75, 0.5, 1.0 / 3.0, 0.375, 0.25, 1.0 / 6.0, 0.125 };
        return b[juce::jlimit (0, (int) std::size (b) - 1, idx)];
    }

    const juce::StringArray& rootNames()
    {
        static const juce::StringArray n { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
        return n;
    }

    double readBpm (juce::AudioPlayHead* head) noexcept
    {
        if (head != nullptr)
            if (const auto pos = head->getPosition())
                if (const auto b = pos->getBpm(); b.hasValue() && *b > 10.0 && *b < 400.0)
                    return *b;
        return 120.0;
    }
}

juce::PluginDescription midiArpDescription()   { return describeMidiFx ("MIDI Arpeggiator", midiArpId, 0x54514d41); }
juce::PluginDescription midiChordDescription() { return describeMidiFx ("Chord Trigger", midiChordId, 0x54514d43); }
juce::PluginDescription midiScaleDescription() { return describeMidiFx ("Scale Lock", midiScaleId, 0x54514d53); }
juce::PluginDescription midiEchoDescription()  { return describeMidiFx ("Note Echo", midiEchoId, 0x54514d45); }

//==============================================================================
// MidiArpProcessor
//==============================================================================
MidiArpProcessor::MidiArpProcessor()
    : BuiltInEffect (midiArpDescription(), "TORQWAVE_MIDI_ARP")
{
    mode         = addChoice ("mode", "Mode", { "Up", "Down", "Up-Down", "Down-Up", "As Played", "Random" }, 0);
    division     = addChoice ("division", "Rate", midiDivisions(), 8); // 1/16
    octaves      = addFloat  ("octaves", "Octaves", { 1.0f, 4.0f, 1.0f }, 1.0f, "oct");
    gate         = addFloat  ("gate", "Gate", { 5.0f, 100.0f }, 75.0f, "%");
    swing        = addFloat  ("swing", "Swing", { 50.0f, 75.0f }, 50.0f, "%");
    velocityMode = addChoice ("velocityMode", "Velocity", { "As Played", "Accent", "Fixed 100" }, 0);
    latch        = addBool   ("latch", "Latch", false);
}

void MidiArpProcessor::prepareToPlay (double sampleRate, int)
{
    rate = juce::jmax (8000.0, sampleRate);
    reset();
}

void MidiArpProcessor::reset()
{
    held.clear();
    latched.clear();
    sequence.clear();
    stepIndex = 0;
    samplesUntilStep = 0.0;
    activePitch = -1;
    samplesUntilOff = 0;
    rng = 0x71c3a509u;
}

void MidiArpProcessor::buildOrder()
{
    const auto& src = (latch->get() && held.empty()) ? latched : held;
    sequence.clear();
    if (src.empty())
        return;

    auto base = src;
    const auto m = mode->getIndex();
    if (m != 4) // anything except "As Played" sorts by pitch first
        std::sort (base.begin(), base.end(), [] (const HeldNote& a, const HeldNote& b) { return a.pitch < b.pitch; });

    const auto numOct = juce::jlimit (1, 4, juce::roundToInt (octaves->get()));
    for (int o = 0; o < numOct; ++o)
        for (const auto& n : base)
            if (n.pitch + 12 * o <= 127)
                sequence.push_back ({ n.pitch + 12 * o, n.velocity, n.channel });

    if (m == 1 || m == 3) // Down or Down-Up
        std::reverse (sequence.begin(), sequence.end());

    if ((m == 2 || m == 3) && sequence.size() > 2) // Up-Down / Down-Up ping-pong
    {
        const auto sz = sequence.size();
        for (size_t i = sz - 2; i >= 1; --i)
            sequence.push_back (sequence[i]);
    }
}

void MidiArpProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    const auto numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    const auto bpm = readBpm (getPlayHead());
    const auto baseStepSamples = juce::jmax (64.0, midiDivisionBeats (division->getIndex()) * (60.0 / bpm) * rate);
    const auto sw = juce::jlimit (0.5, 0.75, (double) swing->get() * 0.01);
    const auto gateFrac = juce::jlimit (0.05, 0.99, (double) gate->get() * 0.01);
    const auto m = mode->getIndex();
    const auto vMode = velocityMode->getIndex();
    const auto isLatch = latch->get();

    juce::MidiBuffer out;

    auto it = midi.cbegin();
    const auto end = midi.cend();

    for (int i = 0; i < numSamples; ++i)
    {
        bool listChanged = false;

        while (it != end && (*it).samplePosition <= i)
        {
            const auto msg = (*it).getMessage();
            ++it;

            if (msg.isNoteOn())
            {
                const bool wasEmpty = held.empty();
                if (wasEmpty && isLatch)
                    latched.clear();

                const auto p = msg.getNoteNumber();
                held.erase (std::remove_if (held.begin(), held.end(), [p] (const HeldNote& h) { return h.pitch == p; }), held.end());
                held.push_back ({ p, msg.getFloatVelocity(), msg.getChannel() });
                if (isLatch)
                {
                    latched.erase (std::remove_if (latched.begin(), latched.end(), [p] (const HeldNote& h) { return h.pitch == p; }), latched.end());
                    latched.push_back ({ p, msg.getFloatVelocity(), msg.getChannel() });
                }

                if (wasEmpty && sequence.empty())
                {
                    stepIndex = 0;
                    samplesUntilStep = 0.0;
                }
                listChanged = true;
            }
            else if (msg.isNoteOff())
            {
                const auto p = msg.getNoteNumber();
                held.erase (std::remove_if (held.begin(), held.end(), [p] (const HeldNote& h) { return h.pitch == p; }), held.end());
                listChanged = true;
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                held.clear();
                latched.clear();
                sequence.clear();
                if (activePitch >= 0)
                {
                    out.addEvent (juce::MidiMessage::noteOff (activeChannel, activePitch), i);
                    activePitch = -1;
                }
                out.addEvent (msg, i);
            }
            else
            {
                out.addEvent (msg, i); // pass CCs, pitch bend, sustain, etc.
            }
        }

        if (listChanged)
        {
            buildOrder();
            if (sequence.empty() && activePitch >= 0)
            {
                out.addEvent (juce::MidiMessage::noteOff (activeChannel, activePitch), i);
                activePitch = -1;
                samplesUntilOff = 0;
            }
        }

        if (activePitch >= 0 && samplesUntilOff > 0)
        {
            if (--samplesUntilOff == 0)
            {
                out.addEvent (juce::MidiMessage::noteOff (activeChannel, activePitch), i);
                activePitch = -1;
            }
        }

        if (! sequence.empty())
        {
            if (samplesUntilStep <= 0.0)
            {
                if (activePitch >= 0)
                {
                    out.addEvent (juce::MidiMessage::noteOff (activeChannel, activePitch), i);
                    activePitch = -1;
                }

                int pick = 0;
                if (m == 5) // Random
                {
                    rng = rng * 1664525u + 1013904223u;
                    pick = (int) ((rng >> 8) % (juce::uint32) sequence.size());
                }
                else
                {
                    pick = stepIndex % (int) sequence.size();
                }

                const auto& n = sequence[(size_t) pick];
                float vel = n.velocity;
                if (vMode == 1)
                    vel = (stepIndex % 4 == 0) ? 1.0f : juce::jmax (0.35f, n.velocity * 0.72f);
                else if (vMode == 2)
                    vel = 100.0f / 127.0f;

                out.addEvent (juce::MidiMessage::noteOn (n.channel, n.pitch, juce::jlimit (0.01f, 1.0f, vel)), i);
                activePitch = n.pitch;
                activeChannel = n.channel;

                const double stepLen = (stepIndex % 2 == 0) ? (baseStepSamples * 2.0 * sw) : (baseStepSamples * 2.0 * (1.0 - sw));
                samplesUntilStep += juce::jmax (32.0, stepLen);
                samplesUntilOff = juce::jmax (16, (int) std::lround (stepLen * gateFrac));
                ++stepIndex;
            }
            samplesUntilStep -= 1.0;
        }
    }

    midi.swapWith (out);
}

//==============================================================================
// MidiChordProcessor
//==============================================================================
MidiChordProcessor::MidiChordProcessor()
    : BuiltInEffect (midiChordDescription(), "TORQWAVE_MIDI_CHORD")
{
    chordType = addChoice ("chordType", "Chord", {
        "Minor Triad", "Major Triad", "Minor 7", "Major 7", "Dominant 7", "Minor 9", "Major 9",
        "Sus2", "Sus4", "Power 5th", "Octave", "Diatonic Triad", "Diatonic 7th", "Custom"
    }, 2); // Minor 7
    scaleRoot = addChoice ("scaleRoot", "Scale Root", rootNames(), 0);
    scaleMode = addChoice ("scaleMode", "Scale Mode", NoteTools::modes(), 1); // minor
    inversion = addFloat  ("inversion", "Inversion", { 0.0f, 3.0f, 1.0f }, 0.0f);
    strumMs   = addFloat  ("strumMs", "Strum", { 0.0f, 80.0f }, 0.0f, "ms");
    custom1   = addFloat  ("custom1", "Custom Note 2", { -24.0f, 24.0f, 1.0f }, 3.0f, "st");
    custom2   = addFloat  ("custom2", "Custom Note 3", { -24.0f, 24.0f, 1.0f }, 7.0f, "st");
    custom3   = addFloat  ("custom3", "Custom Note 4", { -24.0f, 24.0f, 1.0f }, 10.0f, "st");
}

void MidiChordProcessor::prepareToPlay (double sampleRate, int)
{
    rate = juce::jmax (8000.0, sampleRate);
    reset();
}

void MidiChordProcessor::reset()
{
    for (auto& v : soundingForKey) v.clear();
    pending.clear();
}

std::vector<int> MidiChordProcessor::chordPitchesFor (int rootPitch) const
{
    std::vector<int> pitches;
    const auto type = chordType->getIndex();

    if (type == 11 || type == 12) // Diatonic Triad / 7th
    {
        const auto sc = NoteTools::parseKey (rootNames()[scaleRoot->getIndex()] + " " + NoteTools::modes()[scaleMode->getIndex()]).value_or (NoteTools::Scale {});
        const auto base = sc.snap (rootPitch);
        pitches.push_back (base);
        pitches.push_back (sc.step (base, 2));
        pitches.push_back (sc.step (base, 4));
        if (type == 12)
            pitches.push_back (sc.step (base, 6));
    }
    else if (type == 13) // Custom
    {
        pitches = { rootPitch, rootPitch + juce::roundToInt (custom1->get()), rootPitch + juce::roundToInt (custom2->get()), rootPitch + juce::roundToInt (custom3->get()) };
    }
    else
    {
        static const std::vector<std::vector<int>> intervals {
            { 0, 3, 7 },          // Minor Triad
            { 0, 4, 7 },          // Major Triad
            { 0, 3, 7, 10 },      // Minor 7
            { 0, 4, 7, 11 },      // Major 7
            { 0, 4, 7, 10 },      // Dominant 7
            { 0, 3, 7, 10, 14 },  // Minor 9
            { 0, 4, 7, 11, 14 },  // Major 9
            { 0, 2, 7 },          // Sus2
            { 0, 5, 7 },          // Sus4
            { 0, 7 },             // Power 5th
            { 0, 12 },            // Octave
        };
        for (int iv : intervals[(size_t) juce::jlimit (0, (int) intervals.size() - 1, type)])
            pitches.push_back (rootPitch + iv);
    }

    const auto inv = juce::jlimit (0, 3, juce::roundToInt (inversion->get()));
    for (int k = 0; k < inv && ! pitches.empty(); ++k)
    {
        const auto lowest = pitches.front();
        pitches.erase (pitches.begin());
        pitches.push_back (lowest + 12);
    }

    std::vector<int> valid;
    for (int p : pitches)
        if (p >= 0 && p <= 127 && std::find (valid.begin(), valid.end(), p) == valid.end())
            valid.push_back (p);
    return valid;
}

void MidiChordProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    const auto numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    const auto strumSamp = (int) std::lround (strumMs->get() * 0.001 * rate);
    juce::MidiBuffer out;

    // Emit any pending strummed notes falling in this block
    auto keep = pending.begin();
    for (auto& pn : pending)
    {
        if (pn.delaySamples < numSamples)
            out.addEvent (pn.message, juce::jmax (0, pn.delaySamples));
        else
        {
            pn.delaySamples -= numSamples;
            *keep++ = pn;
        }
    }
    pending.erase (keep, pending.end());

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const auto pos = juce::jlimit (0, numSamples - 1, meta.samplePosition);

        if (msg.isNoteOn())
        {
            const auto key = juce::jlimit (0, 127, msg.getNoteNumber());
            // Release any previously held chord for this key
            for (int oldP : soundingForKey[(size_t) key])
                out.addEvent (juce::MidiMessage::noteOff (msg.getChannel(), oldP), pos);

            const auto chord = chordPitchesFor (key);
            soundingForKey[(size_t) key] = chord;

            for (size_t idx = 0; idx < chord.size(); ++idx)
            {
                const auto delay = pos + (int) idx * strumSamp;
                const auto noteMsg = juce::MidiMessage::noteOn (msg.getChannel(), chord[idx], msg.getFloatVelocity());
                if (delay < numSamples)
                    out.addEvent (noteMsg, delay);
                else
                    pending.push_back ({ delay - numSamples, noteMsg });
            }
        }
        else if (msg.isNoteOff())
        {
            const auto key = juce::jlimit (0, 127, msg.getNoteNumber());
            for (int p : soundingForKey[(size_t) key])
                out.addEvent (juce::MidiMessage::noteOff (msg.getChannel(), p), pos);
            soundingForKey[(size_t) key].clear();
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            reset();
            out.addEvent (msg, pos);
        }
        else
        {
            out.addEvent (msg, pos);
        }
    }

    midi.swapWith (out);
}

//==============================================================================
// MidiScaleProcessor
//==============================================================================
MidiScaleProcessor::MidiScaleProcessor()
    : BuiltInEffect (midiScaleDescription(), "TORQWAVE_MIDI_SCALE")
{
    root      = addChoice ("root", "Root", rootNames(), 0);
    mode      = addChoice ("mode", "Scale", NoteTools::modes(), 1); // minor
    action    = addChoice ("action", "Out of Scale", { "Snap Nearest", "Snap Down", "Snap Up", "Filter Out" }, 0);
    transpose = addFloat  ("transpose", "Transpose", { -24.0f, 24.0f, 1.0f }, 0.0f, "st");
    activeMapping.fill (-1);
}

void MidiScaleProcessor::prepareToPlay (double, int) { reset(); }
void MidiScaleProcessor::reset() { activeMapping.fill (-1); }

int MidiScaleProcessor::mapPitch (int pitch) const noexcept
{
    const auto sc = NoteTools::parseKey (rootNames()[root->getIndex()] + " " + NoteTools::modes()[mode->getIndex()]).value_or (NoteTools::Scale {});
    const auto shifted = juce::jlimit (0, 127, pitch + juce::roundToInt (transpose->get()));
    if (sc.contains (shifted))
        return shifted;

    const auto act = action->getIndex();
    if (act == 3)
        return -1; // Filter out
    if (act == 1) // Snap down
    {
        for (int d = 1; d < 12; ++d)
            if (shifted - d >= 0 && sc.contains (shifted - d))
                return shifted - d;
    }
    else if (act == 2) // Snap up
    {
        for (int d = 1; d < 12; ++d)
            if (shifted + d <= 127 && sc.contains (shifted + d))
                return shifted + d;
    }
    return sc.snap (shifted);
}

void MidiScaleProcessor::processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer& midi)
{
    juce::MidiBuffer out;

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const auto pos = meta.samplePosition;

        if (msg.isNoteOn())
        {
            const auto key = juce::jlimit (0, 127, msg.getNoteNumber());
            if (activeMapping[(size_t) key] >= 0)
                out.addEvent (juce::MidiMessage::noteOff (msg.getChannel(), activeMapping[(size_t) key]), pos);

            const auto mapped = mapPitch (key);
            activeMapping[(size_t) key] = mapped;
            if (mapped >= 0)
                out.addEvent (juce::MidiMessage::noteOn (msg.getChannel(), mapped, msg.getFloatVelocity()), pos);
        }
        else if (msg.isNoteOff())
        {
            const auto key = juce::jlimit (0, 127, msg.getNoteNumber());
            const auto mapped = activeMapping[(size_t) key];
            activeMapping[(size_t) key] = -1;
            if (mapped >= 0)
                out.addEvent (juce::MidiMessage::noteOff (msg.getChannel(), mapped), pos);
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            activeMapping.fill (-1);
            out.addEvent (msg, pos);
        }
        else
        {
            out.addEvent (msg, pos);
        }
    }

    midi.swapWith (out);
}

//==============================================================================
// MidiEchoProcessor
//==============================================================================
MidiEchoProcessor::MidiEchoProcessor()
    : BuiltInEffect (midiEchoDescription(), "TORQWAVE_MIDI_ECHO")
{
    sync       = addBool   ("sync", "Tempo Sync", true);
    division   = addChoice ("division", "Division", midiDivisions(), 4); // 1/8 D
    timeMs     = addFloat  ("timeMs", "Time", { 20.0f, 2000.0f, 0.0f, 0.4f }, 375.0f, "ms");
    repeats    = addFloat  ("repeats", "Repeats", { 1.0f, 8.0f, 1.0f }, 3.0f);
    decay      = addFloat  ("decay", "Velocity Decay", { 10.0f, 95.0f }, 68.0f, "%");
    pitchShift = addFloat  ("pitchShift", "Pitch Shift", { -12.0f, 12.0f, 1.0f }, 0.0f, "st");
    gate       = addFloat  ("gate", "Echo Gate", { 10.0f, 100.0f }, 60.0f, "%");
}

void MidiEchoProcessor::prepareToPlay (double sampleRate, int)
{
    rate = juce::jmax (8000.0, sampleRate);
    reset();
}

void MidiEchoProcessor::reset()
{
    sampleClock = 0;
    queue.clear();
}

void MidiEchoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    const auto numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    const auto bpm = readBpm (getPlayHead());
    const auto stepSamples = (juce::int64) std::llround (juce::jmax (0.02,
        sync->get() ? (midiDivisionBeats (division->getIndex()) * (60.0 / bpm)) : ((double) timeMs->get() * 0.001)) * rate);
    const auto numRep = juce::jlimit (1, 8, juce::roundToInt (repeats->get()));
    const auto velDecay = decay->get() * 0.01f;
    const auto shift = juce::roundToInt (pitchShift->get());
    const auto noteLen = juce::jmax ((juce::int64) 64, (juce::int64) std::llround ((double) stepSamples * (double) gate->get() * 0.01));

    juce::MidiBuffer out;

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const auto pos = juce::jlimit (0, numSamples - 1, meta.samplePosition);
        out.addEvent (msg, pos);

        if (msg.isNoteOn())
        {
            float vel = msg.getFloatVelocity();
            int pitch = msg.getNoteNumber();
            for (int r = 1; r <= numRep; ++r)
            {
                vel *= velDecay;
                pitch += shift;
                if (vel < 0.02f || pitch < 0 || pitch > 127)
                    break;
                const auto onTime = sampleClock + pos + (juce::int64) r * stepSamples;
                queue.push_back ({ onTime, juce::MidiMessage::noteOn (msg.getChannel(), pitch, vel) });
                queue.push_back ({ onTime + noteLen, juce::MidiMessage::noteOff (msg.getChannel(), pitch) });
            }
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            queue.clear();
        }
    }

    const auto blockEnd = sampleClock + numSamples;
    auto keep = queue.begin();
    for (auto& ev : queue)
    {
        if (ev.sampleTime < blockEnd)
            out.addEvent (ev.message, (int) juce::jlimit ((juce::int64) 0, (juce::int64) (numSamples - 1), ev.sampleTime - sampleClock));
        else
            *keep++ = ev;
    }
    queue.erase (keep, queue.end());

    sampleClock = blockEnd;
    midi.swapWith (out);
}
}
