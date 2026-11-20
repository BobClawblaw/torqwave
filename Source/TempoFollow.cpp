#include "TempoFollow.h"
#include "AudioEdits.h"
#include "PhaseVocoder.h"
#include "TimeStretch.h"

namespace TempoFollow
{
namespace
{
    /** Beats an original frame lasts at the clip's own tempo. */
    double beatsPerFrame (const SessionClip::TempoFollow& f, double rate)
    {
        return f.bpm / 60.0 / rate;
    }

    /** Where the tempo changes, as beats on the map, with the tempo from there: the start's first. */
    std::vector<std::pair<double, double>> tempoSegments (const SessionState& s)
    {
        const auto map = s.tempoMap();
        std::vector<std::pair<double, double>> segments { { 0.0, s.tempo } };

        for (auto& c : s.tempoChanges)
            segments.emplace_back ((double) map.barStartBeat (c.bar), c.tempo);

        return segments;
    }

    /** Beats from the anchor where the original's last frame ends. */
    double lastBeat (const SessionState& s, const SessionClip& c)
    {
        return beatOf (c.tempoFollow, (double) c.tempoFollow.original->getNumFrames(), s.sampleRate);
    }

    /** A stretched frame (from the anchor) to a frame of the original, and back, on the map. */
    double toOriginal (const SessionState& s, const SessionClip& c, double anchor, double stretched)
    {
        const auto map = s.tempoMap();
        const auto anchorFrame = map.frameAtBeatPosition (anchor);
        return frameOf (c.tempoFollow, map.beatPosition (anchorFrame + (juce::int64) std::llround (stretched)) - anchor, s.sampleRate);
    }

    double toStretched (const Metronome::TempoMap& map, const SessionClip& c, double anchor, double original, double rate)
    {
        return (double) (map.frameAtBeatPosition (anchor + beatOf (c.tempoFollow, original, rate)) - map.frameAtBeatPosition (anchor));
    }
}

double beatOf (const SessionClip::TempoFollow& f, double frame, double rate)
{
    // From the first frame (beat 0) through each marker; past the last at the clip's tempo.
    double fromFrame = 0.0, fromBeat = 0.0;

    for (auto& w : f.warps)
    {
        if (frame <= (double) w.frame)
            return (double) w.frame > fromFrame ? fromBeat + (frame - fromFrame) * (w.beat - fromBeat) / ((double) w.frame - fromFrame) : w.beat;

        fromFrame = (double) w.frame;
        fromBeat = w.beat;
    }

    return fromBeat + (frame - fromFrame) * beatsPerFrame (f, rate);
}

double frameOf (const SessionClip::TempoFollow& f, double beat, double rate)
{
    double fromFrame = 0.0, fromBeat = 0.0;

    for (auto& w : f.warps)
    {
        if (beat <= w.beat)
            return w.beat > fromBeat ? fromFrame + (beat - fromBeat) * ((double) w.frame - fromFrame) / (w.beat - fromBeat) : (double) w.frame;

        fromFrame = (double) w.frame;
        fromBeat = w.beat;
    }

    const auto k = beatsPerFrame (f, rate);
    return k > 0.0 ? fromFrame + (beat - fromBeat) / k : fromFrame;
}

double anchorBeat (const SessionState& s, const SessionClip& c)
{
    return s.tempoMap().beatPosition (c.position - c.offset);
}

Span spanOf (const SessionState& s, const SessionClip& c)
{
    Span span;

    if (! c.tempoFollow.follows())
    {
        span.start = (double) c.offset;
        span.end = (double) (c.offset + c.length);

        for (auto& p : c.gainEnvelope.points)
            span.gainFrames.push_back ((double) p.frame);

        return span;
    }

    const auto anchor = anchorBeat (s, c);
    span.start = toOriginal (s, c, anchor, (double) c.offset);
    span.end = toOriginal (s, c, anchor, (double) (c.offset + c.length));

    for (auto& p : c.gainEnvelope.points)
        span.gainFrames.push_back (toOriginal (s, c, anchor, (double) p.frame));

    return span;
}

void place (const SessionState& s, SessionClip& c, const Span& span, double startBeat)
{
    const auto map = s.tempoMap();
    const auto rate = s.sampleRate;
    const auto anchor = startBeat - beatOf (c.tempoFollow, span.start, rate);
    const auto anchorFrame = map.frameAtBeatPosition (anchor);
    c.position = juce::jmax ((juce::int64) 0, map.frameAtBeatPosition (startBeat));
    c.offset = c.position - anchorFrame;
    const auto end = anchorFrame + (juce::int64) std::llround (toStretched (map, c, anchor, span.end, rate));
    c.length = juce::jmax ((juce::int64) 1, end - c.position);
    c.fadeIn = juce::jmin (c.fadeIn, c.length);
    c.fadeOut = juce::jmin (c.fadeOut, c.length - c.fadeIn);

    for (size_t i = 0; i < c.gainEnvelope.points.size() && i < span.gainFrames.size(); ++i)
        c.gainEnvelope.points[i].frame = (juce::int64) std::llround (toStretched (map, c, anchor, span.gainFrames[i], rate));
}

juce::String key (const SessionState& s, const SessionClip& c)
{
    if (! c.tempoFollow.follows())
        return {};

    // The map from the anchor to the original's end: the tempo there and each change after it,
    // as beats from the anchor. Where the clip sits otherwise doesn't change what it plays.
    const auto anchor = anchorBeat (s, c);
    const auto last = anchor + lastBeat (s, c);
    juce::String k;
    k << juce::String::toHexString ((juce::pointer_sized_int) c.tempoFollow.original.get()) << " " << juce::String (c.tempoFollow.bpm, 6)
      << " @" << juce::String (s.sampleRate, 1) << " w";

    for (auto& w : c.tempoFollow.warps)
        k << " " << w.frame << ":" << juce::String (w.beat, 6);

    k << " t";
    const auto segments = tempoSegments (s);

    for (size_t i = 0; i < segments.size(); ++i)
    {
        const auto next = i + 1 < segments.size() ? segments[i + 1].first : std::numeric_limits<double>::max();

        if (next > anchor && segments[i].first < last)
            k << " " << juce::String (juce::jmax (0.0, segments[i].first - anchor), 6) << "=" << juce::String (segments[i].second, 6);
    }

    return k;
}

bool isStale (const SessionState& s, const SessionClip& c)
{
    return c.tempoFollow.follows() && c.tempoFollow.renderedFor != key (s, c);
}

AudioDocument::Snapshot render (const SessionState& s, const SessionClip& c, const std::function<bool (float)>& progress)
{
    if (! c.tempoFollow.follows())
        return nullptr;

    const auto& original = *c.tempoFollow.original;
    const auto length = original.getNumFrames();

    if (length <= 0 || length > std::numeric_limits<int>::max())
        return nullptr;

    const auto rate = s.sampleRate;
    const auto map = s.tempoMap();
    const auto anchor = anchorBeat (s, c);
    const auto last = lastBeat (s, c);

    // The map's points: the ends, each marker, and each frame of the original where the tempo changes.
    std::vector<double> frames { 0.0, (double) length };

    for (auto& w : c.tempoFollow.warps)
        if (w.frame > 0 && w.frame < length)
            frames.push_back ((double) w.frame);

    for (auto& [beat, tempo] : tempoSegments (s))
        if (beat > anchor && beat < anchor + last)
            frames.push_back (frameOf (c.tempoFollow, beat - anchor, rate));

    std::sort (frames.begin(), frames.end());
    std::vector<TimeStretch::MapPoint> points;

    for (const auto f : frames)
    {
        const auto out = toStretched (map, c, anchor, f, rate);

        if (points.empty() || (f > points.back().in + 0.5 && out > points.back().out + 0.5))
            points.push_back ({ f, out });
    }

    points.back().in = (double) length;

    juce::AudioBuffer<float> in (original.getNumChannels(), (int) length);
    original.read (in, 0, 0, (int) length);
    auto out = TimeStretch::stretchAlong (in, points, rate, progress);

    if (out.getNumSamples() == 0)
        return nullptr;

    return AudioEdits::fromBuffer (std::move (out));
}

bool renderAll (SessionState& s, const std::function<bool (float)>& progress)
{
    for (auto& t : s.tracks)
        for (auto& c : t.clips)
            if (isStale (s, c))
            {
                auto audio = render (s, c, progress);

                if (audio == nullptr)
                    return false;

                c.audio = std::move (audio);
                c.tempoFollow.renderedFor = key (s, c);
            }

    return true;
}

std::optional<double> tempoFromName (const juce::String& name)
{
    // A number of two or three digits standing alone, "bpm" after it or not; with "bpm" first.
    const auto lower = name.toLowerCase();
    std::optional<double> plain;

    for (int i = 0; i < lower.length();)
    {
        if (! juce::CharacterFunctions::isDigit (lower[i]))
        {
            ++i;
            continue;
        }

        auto end = i;

        while (end < lower.length() && (juce::CharacterFunctions::isDigit (lower[end]) || lower[end] == '.'))
            ++end;

        const auto before = i > 0 ? lower[i - 1] : (juce::juce_wchar) ' ';
        const auto after = lower.substring (end).trimStart();
        const auto value = lower.substring (i, end).getDoubleValue();
        const auto alone = ! juce::CharacterFunctions::isLetter (before) && (end >= lower.length() || ! juce::CharacterFunctions::isLetter (lower[end]));
        i = end;

        if (value < 40.0 || value > 300.0)
            continue;

        if (after.startsWith ("bpm"))
            return value;

        if (alone && ! plain.has_value())
            plain = value;
    }

    return plain;
}

std::vector<juce::int64> transients (const AudioSnapshot& audio, juce::int64 from, juce::int64 to, double rate)
{
    from = juce::jmax ((juce::int64) 0, from);
    to = juce::jmin (audio.getNumFrames(), to);

    if (to - from < 64 || to - from > std::numeric_limits<int>::max())
        return {};

    // Mono: the first two channels together.
    juce::AudioBuffer<float> part (juce::jmin (2, audio.getNumChannels()), (int) (to - from));
    audio.read (part, 0, from, part.getNumSamples());
    juce::AudioBuffer<float> mono (1, part.getNumSamples());
    mono.copyFrom (0, 0, part, 0, 0, part.getNumSamples());

    if (part.getNumChannels() > 1)
    {
        mono.addFrom (0, 0, part, 1, 0, part.getNumSamples());
        mono.applyGain (0.5f);
    }

    std::vector<juce::int64> found;

    for (const auto at : PhaseVocoder::onsets (mono, 0, mono.getNumSamples(), rate))
        found.push_back (from + at);

    return found;
}

std::optional<double> detectTempo (const AudioSnapshot& audio, juce::int64 from, juce::int64 to, double rate)
{
    const auto attacks = transients (audio, from, to, rate);

    if (attacks.size() < 4)
        return std::nullopt;

    // Each tempo from 70 to 180 scored by how many attacks fall on its grid (any phase), each
    // counted by how close it is: the best, then tuned to the attacks' own spacing.
    double best = 0.0, bestScore = 0.0;

    for (double bpm = 70.0; bpm <= 180.0; bpm += 0.1)
    {
        const auto beat = 60.0 * rate / bpm;
        const auto half = beat / 2.0;
        double sumC = 0.0, sumS = 0.0;

        for (const auto a : attacks)
        {
            // Attacks on eighths count too, half as much.
            const auto phase = juce::MathConstants<double>::twoPi * (double) (a - attacks.front()) / half;
            sumC += std::cos (phase);
            sumS += std::sin (phase);
            const auto onBeat = juce::MathConstants<double>::twoPi * (double) (a - attacks.front()) / beat;
            sumC += std::cos (onBeat);
            sumS += std::sin (onBeat);
        }

        const auto score = std::sqrt (sumC * sumC + sumS * sumS);

        if (score > bestScore * 1.0001)
        {
            bestScore = score;
            best = bpm;
        }
    }

    // Too little agreement: no steady beat.
    if (best <= 0.0 || bestScore < 0.5 * 2.0 * (double) attacks.size())
        return std::nullopt;

    return std::round (best * 10.0) / 10.0;
}

//==============================================================================
Renderer::Renderer (Session& s) : juce::Thread ("Tempo follow"), session (s)
{
    session.addChangeListener (this);
    startThread (juce::Thread::Priority::low);
    look();
}

Renderer::~Renderer()
{
    session.removeChangeListener (this);
    signalThreadShouldExit();
    work.signal();
    stopThread (10000);
}

bool Renderer::isBusy() const
{
    return busy;
}

void Renderer::whenDone (std::function<void()> then)
{
    if (then == nullptr)
        return;

    waiting.push_back (std::move (then));
    look();
}

bool Renderer::isUpToDate() const
{
    const auto state = session.getState();

    for (auto& t : state->tracks)
        for (auto& c : t.clips)
            if (isStale (*state, c))
                return false;

    return true;
}

void Renderer::changeListenerCallback (juce::ChangeBroadcaster*)
{
    look();
}

void Renderer::look()
{
    const auto state = session.getState();
    std::optional<SessionState> next;
    std::vector<Job> wanting;
    std::set<juce::String> keys;

    for (size_t t = 0; t < state->tracks.size(); ++t)
        for (size_t i = 0; i < state->tracks[t].clips.size(); ++i)
        {
            const auto& c = state->tracks[t].clips[i];

            if (! isStale (*state, c))
                continue;

            const auto k = key (*state, c);
            const auto found = std::find_if (kept.begin(), kept.end(), [&k] (auto& e) { return e.first == k; });

            if (found == kept.end())
            {
                if (keys.insert (k).second)
                    wanting.push_back ({ state, c.id, k });

                continue;
            }

            if (! next.has_value())
                next = *state;

            auto& clip = next->tracks[t].clips[i];
            clip.audio = found->second;
            clip.tempoFollow.renderedFor = k;
        }

    {
        const juce::ScopedLock sl (lock);

        // The one being stretched goes on if it is still wanted.
        wanting.erase (std::remove_if (wanting.begin(), wanting.end(), [this] (auto& j) { return j.key == working; }), wanting.end());
        jobs = std::move (wanting);
        wanted = keys;
    }

    setBusy (! keys.empty());

    if (! keys.empty())
        work.signal();

    // In place of the state as it is: not a step of its own, and nothing new to save.
    if (next.has_value())
        session.refresh (std::move (*next));

    // Everything stretched: what was waiting for it goes ahead (once each).
    if (! waiting.empty() && isUpToDate())
    {
        auto now = std::move (waiting);
        waiting.clear();

        for (auto& then : now)
            then();
    }
}

void Renderer::setBusy (bool now)
{
    if (busy == now)
        return;

    busy = now;

    if (onBusyChanged != nullptr)
        onBusyChanged();
}

void Renderer::run()
{
    while (! threadShouldExit())
    {
        work.wait (-1);

        for (;;)
        {
            Job job;

            {
                const juce::ScopedLock sl (lock);

                if (jobs.empty())
                {
                    working = {};
                    break;
                }

                job = jobs.front();
                jobs.erase (jobs.begin());
                working = job.key;
            }

            const auto where = job.state->findClip (job.clipId);

            if (! where.has_value())
                continue;

            const auto& clip = job.state->tracks[(size_t) where->first].clips[(size_t) where->second];

            // Stops when no clip wants this any more.
            auto audio = render (*job.state, clip, [this, &job] (float)
            {
                const juce::ScopedLock sl (lock);
                return ! threadShouldExit() && wanted.count (job.key) > 0;
            });

            if (threadShouldExit())
                return;

            if (audio == nullptr)
                continue;

            juce::MessageManager::callAsync ([safe = juce::WeakReference<Renderer> (this), k = job.key, audio]
            {
                if (auto* r = safe.get())
                {
                    r->kept.emplace_back (k, audio);

                    if (r->kept.size() > 32)
                        r->kept.erase (r->kept.begin());

                    r->look();
                }
            });
        }
    }
}
}
