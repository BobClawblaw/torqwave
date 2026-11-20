#include "Sequencer.h"

namespace
{
    bool same (const SequenceFile::Sequence& a, const SequenceFile::Sequence& b)
    {
        return SequenceFile::toXml (a)->isEquivalentTo (SequenceFile::toXml (b).get(), false);
    }

    constexpr int maxSteps = 200;
}

//==============================================================================
SequencerDocument::SequencerDocument()
{
    history.push_back ({ SequenceFile::Sequence(), {} });
}

void SequencerDocument::open (SequenceFile::Sequence q, const juce::File& f)
{
    history.clear();
    history.push_back ({ std::move (q), {} });
    position = savedPosition = 0;
    voiceChanged = false;
    file = f;
}

void SequencerDocument::apply (SequenceFile::Sequence q, const juce::String& description)
{
    if (same (q, get()))
        return;

    history.resize ((size_t) position + 1);
    history.push_back ({ std::move (q), description });
    ++position;

    if (savedPosition >= position)
        savedPosition = -1;   // the saved step was undone and is gone

    while ((int) history.size() > maxSteps)
    {
        history.erase (history.begin());
        --position;
        savedPosition = savedPosition > 0 ? savedPosition - 1 : -1;
    }
}

juce::String SequencerDocument::saveTo (const juce::File& target, std::shared_ptr<const juce::XmlElement> voice)
{
    auto q = get();

    if (voice != nullptr)
        q.voice = voice;

    if (const auto error = SequenceFile::save (q, target); error.isNotEmpty())
        return error;

    if (voice != nullptr)
        keepVoice (voice);

    savedPosition = position;
    voiceChanged = false;
    file = target;
    return {};
}

void SequencerDocument::keepVoice (std::shared_ptr<const juce::XmlElement> voice)
{
    const auto old = get().voice;

    if (voice == old)
        return;

    for (auto& step : history)
        if (step.first.voice == old)
            step.first.voice = voice;
}

SequencerDocument::Built SequencerDocument::sessionFor (const SequenceFile::Sequence& q, double tempo, double sampleRate)
{
    Built b;
    b.state.sampleRate = sampleRate;
    b.state.tempo = tempo;
    b.state.beatsPerBar = q.beatsPerBar;
    const auto placed = SequenceFile::place (b.state, q, -1, 0, false);
    b.trackId = placed.trackId;
    b.clipId = placed.clipId;
    b.effects = placed.effects;
    return b;
}

SessionState SequencerDocument::withSequence (const Built& b, const SequenceFile::Sequence& q, double tempo)
{
    auto s = b.state;
    s.tempo = tempo;
    s.beatsPerBar = q.beatsPerBar;
    auto clip = SequenceFile::toClip (q, s, 0);
    clip.id = b.clipId;

    for (auto& t : s.tracks)
        if (t.id == b.trackId)
            t.noteClips = { std::move (clip) };

    return s;
}

std::optional<SessionState> SequencerDocument::heldOver (const SessionState& state, juce::uint32 clipId)
{
    const auto found = state.findNoteClip (clipId);

    if (! found.has_value())
        return std::nullopt;

    auto s = state;
    auto& c = s.tracks[(size_t) found->first].noteClips[(size_t) found->second];
    const auto period = c.loopLength > 0 ? c.loopLength : c.length;

    if (c.offset != 0 || period <= 0 || c.length % period != 0)
        return std::nullopt;

    c.loopLength = period;
    c.length = juce::jmax (c.length, (juce::int64) (4.0 * 3600.0 * s.sampleRate));
    c.length -= c.length % period;
    return s;
}

SequenceFile::Sequence SequencerDocument::withClip (const SequenceFile::Sequence& q, const SessionState& s, juce::uint32 clipId)
{
    const auto read = SequenceFile::fromClip (s, clipId, nullptr);

    if (! read.has_value())
        return q;

    auto out = q;
    out.channel = read->channel;
    out.offset = read->offset;
    out.length = read->length;
    out.loopLength = read->loopLength;
    out.notes = read->notes;
    out.lanes = read->lanes;
    return out;
}

SequenceFile::Sequence SequencerDocument::variationOf (const SequenceFile::Sequence& q)
{
    auto v = q;
    v.name = q.name + " Variation";
    v.variationOf = q.name;
    return v;
}

SequenceFile::Sequence SequencerDocument::blank (const juce::String& name, int bars, int beatsPerBar, double tempo)
{
    SequenceFile::Sequence q;
    q.name = name;
    q.role = "Inbox";
    q.beatsPerBar = juce::jlimit (1, 16, beatsPerBar);
    q.writtenTempo = tempo;
    q.length = (juce::int64) juce::jmax (1, bars) * q.beatsPerBar * SequenceFile::ticksPerBeat;
    return q;
}

//==============================================================================
SequencerPlayer::SequencerPlayer()
{
    buffers.interfaceChannel = -1;
    buffers.reserve (TrackEffects::maxBlock);
    mix.setSize (2, TrackEffects::maxBlock);
    liveBlock.ensureSize (1024);
    collector.reset (rate);
}

SequencerPlayer::~SequencerPlayer() = default;

void SequencerPlayer::setState (std::shared_ptr<const SessionState> s, juce::uint32 track)
{
    juce::int64 last = 0;

    if (s != nullptr)
        if (const auto* t = s->findTrack (track))
            for (auto& c : t->noteClips)
                last = juce::jmax (last, c.position + c.length);

    std::shared_ptr<const SessionState> old;
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        old = std::exchange (state, std::move (s));
        trackId = track;
        end = last;
    }
}

void SequencerPlayer::play()
{
    position = 0;
    playing = true;
}

void SequencerPlayer::stop()
{
    playing = false;
    position = 0;
}

void SequencerPlayer::audition (int pitch, float velocity)
{
    auto m = velocity > 0.0f ? juce::MidiMessage::noteOn (1, pitch, velocity) : juce::MidiMessage::noteOff (1, pitch);
    m.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
    collector.addMessageToQueue (m);
}

void SequencerPlayer::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    rate = sampleRate > 0.0 ? sampleRate : 48000.0;
    collector.reset (rate);
    const auto frames = juce::jmax (TrackEffects::maxBlock, samplesPerBlockExpected);
    buffers.reserve (frames);
    mix.setSize (2, frames);
}

void SequencerPlayer::renderRun (const SessionState& s, juce::int64 start, int destStart, int numFrames, const SessionMixer::Live* live)
{
    // In pieces no longer than the effects take at once; live notes go with the first.
    for (int done = 0; done < numFrames;)
    {
        const auto n = juce::jmin (numFrames - done, TrackEffects::maxBlock);
        effects.setLiveMidi (trackId, done == 0 && destStart == 0 ? &liveBlock : nullptr);
        SessionMixer::render (s, start + done, mix, destStart + done, n, buffers, &effects, live);
        done += n;
    }
}

void SequencerPlayer::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();
    liveBlock.clear();
    collector.removeNextBlockOfMessages (liveBlock, info.numSamples);

    const juce::SpinLock::ScopedTryLockType sl (lock);

    if (! sl.isLocked() || state == nullptr || info.numSamples > mix.getNumSamples() || info.buffer->getNumChannels() == 0)
        return;

    const auto& s = *state;
    const auto now = playing.load();

    if (! liveBlock.isEmpty())
        idleFrames = (juce::int64) (3.0 * rate);

    // Stopping: the instruments' notes end. Starting from the top: they start afresh, the
    // first block's notes played (endNotes would leave them out).
    if (now != std::exchange (wasPlaying, now))
    {
        if (now)
            for (auto& t : s.tracks)
                effects.reset (t.id);
        else
            effects.endNotes();
    }

    effects.setClipNotes (now);
    mix.clear (0, info.numSamples);

    if (now)
    {
        auto at = position.load();
        const auto free = freeRunning.load();

        for (int done = 0; done < info.numSamples;)
        {
            if (! free && (end <= 0 || at >= end))
            {
                if (! looping.load() || end <= 0)
                {
                    playing = false;
                    at = 0;
                    break;
                }

                at = 0;

                // The notes held over the end get their offs, then the pass plays from its start.
                for (auto& t : s.tracks)
                    effects.endHeldNotes (t.id);
            }

            const auto n = free ? info.numSamples - done : (int) juce::jmin ((juce::int64) (info.numSamples - done), end - at);
            renderRun (s, at, done, n, nullptr);

            if (click.load())
            {
                const auto map = s.tempoMap();
                auto* l = mix.getWritePointer (0, done);
                auto* r = mix.getWritePointer (1, done);

                for (int i = 0; i < n; ++i)
                {
                    const auto beat = map.beatAt (at + i);
                    const auto c = 0.5f * Metronome::sample (at + i - map.beatFrame (beat), map.barAndBeat (beat).second == 0, s.sampleRate);
                    l[i] += c;
                    r[i] += c;
                }
            }

            at += n;
            done += n;
        }

        position = at;
    }
    else if (idleFrames > 0)
    {
        static const SessionMixer::Live silent { {}, 0, nullptr, false };
        renderRun (s, position.load(), 0, info.numSamples, &silent);
        idleFrames -= info.numSamples;
    }
    else
    {
        return;
    }

    for (int ch = 0; ch < juce::jmin (2, info.buffer->getNumChannels()); ++ch)
        info.buffer->addFrom (ch, info.startSample, mix, ch, 0, info.numSamples);
}
