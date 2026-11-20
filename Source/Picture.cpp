#include "Picture.h"
#include "AudioFileIO.h"
#include "SampleRateConverter.h"

namespace Picture
{
namespace
{
    /** The real frames in a day of timecode at a rate, and between real and counted frames. */
    juce::int64 realOf (juce::int64 counted, TimeFormat::Kind kind)
    {
        return TimeFormat::isDropFrame (kind) ? TimeFormat::fromDropFrameCount (counted) : counted;
    }

    juce::int64 countedOf (juce::int64 real, TimeFormat::Kind kind)
    {
        return TimeFormat::isDropFrame (kind) ? TimeFormat::dropFrameCount (real) : real;
    }

    /** How near a tempo's beats (or bars) of `period` seconds come to the hits: how many land
        within the tolerance, and the farthest one's distance. */
    struct Score
    {
        int within = 0;
        double worst = 0.0;

        bool beats (const Score& o) const noexcept    { return within > o.within || (within == o.within && worst < o.worst); }
    };

    Score score (const std::vector<double>& hits, double period, double tolerance)
    {
        Score s;

        for (auto h : hits)
        {
            const auto off = std::abs (h - std::round (h / period) * period);
            s.within += off <= tolerance ? 1 : 0;
            s.worst = juce::jmax (s.worst, off);
        }

        return s;
    }

    /** The period near `period` that brings the farthest of the hits caught within the tolerance
        nearest its beat, each kept on the beat it is on: the largest of their distances is convex
        in the period, so a ternary search finds its least. */
    double refine (const std::vector<double>& hits, double period, double tolerance, double spread)
    {
        std::vector<std::pair<double, double>> caught;   // the hit and its beat count

        for (auto h : hits)
            if (const auto n = std::round (h / period); std::abs (h - n * period) <= tolerance && n > 0.0)
                caught.emplace_back (h, n);

        if (caught.empty())
            return period;

        const auto worst = [&caught] (double p)
        {
            auto w = 0.0;

            for (auto& [h, n] : caught)
                w = juce::jmax (w, std::abs (h - n * p));

            return w;
        };

        auto lo = period * (1.0 - spread), hi = period * (1.0 + spread);

        for (int i = 0; i < 200; ++i)
        {
            const auto a = lo + (hi - lo) / 3.0, b = hi - (hi - lo) / 3.0;

            if (worst (a) < worst (b))
                hi = b;
            else
                lo = a;
        }

        return 0.5 * (lo + hi);
    }

    /** One tempo, on beats or bars. */
    Fit fitOne (const std::vector<juce::int64>& hitFrames, double sampleRate, int beatsPerBar, LandOn landOn,
                double minTempo, double maxTempo, juce::int64 tolerance)
    {
        std::vector<double> hits;

        for (auto f : hitFrames)
            hits.push_back ((double) f / sampleRate);

        const auto unit = landOn == LandOn::beats ? 1.0 : (double) beatsPerBar;   // beats a mark
        const auto toleranceSeconds = (double) tolerance / sampleRate;
        const auto last = juce::jmax (toleranceSeconds, *std::max_element (hits.begin(), hits.end()));

        // Tempos a step apart that moves the last hit's beat by a quarter of the tolerance, so
        // none that catches it is passed over.
        const auto ratio = 1.0 + juce::jmax (1.0e-7, 0.25 * toleranceSeconds / last);
        const auto steps = juce::jmin (4000000, (int) std::ceil (std::log (maxTempo / minTempo) / std::log (ratio)) + 1);
        auto bestTempo = minTempo;
        Score best;
        best.within = -1;

        for (int i = 0; i < steps; ++i)
        {
            const auto tempo = juce::jmin (maxTempo, minTempo * std::pow (ratio, (double) i));

            if (const auto s = score (hits, 60.0 * unit / tempo, toleranceSeconds); s.beats (best))
            {
                best = s;
                bestTempo = tempo;
            }
        }

        const auto refined = juce::jlimit (minTempo, maxTempo, 60.0 * unit / refine (hits, 60.0 * unit / bestTempo, toleranceSeconds, 2.0 * (ratio - 1.0)));

        Fit fit;
        fit.beatsPerBar = beatsPerBar;
        fit.tempo = score (hits, 60.0 * unit / refined, toleranceSeconds).beats (best) ? refined : bestTempo;
        return fit;
    }

    /** A tempo map with each hit on a bar line, near `tempo`. */
    Fit fitMap (const std::vector<juce::int64>& hits, double sampleRate, int beatsPerBar, double tempo)
    {
        Fit fit;
        std::vector<Metronome::TempoChange> marks;   // from bar 0
        juce::int64 bar = 0, from = 0;   // the bar and frame of the last hit placed (a hit at 0 is on bar 0 already)

        for (auto hit : hits)
        {
            if (hit <= from)
                continue;

            // A whole number of beats from the last hit (or the start) to this one, at a tempo near the fitted one.
            const auto seconds = (double) (hit - from) / sampleRate;
            auto beats = juce::jmax ((juce::int64) 1, (juce::int64) std::llround (seconds * tempo / 60.0));

            while ((double) beats * 60.0 / seconds < 20.0)
                ++beats;

            const auto segmentTempo = (double) beats * 60.0 / seconds;

            if (segmentTempo > 400.0)
                continue;   // too close to the last to be a bar later: left where it lands

            const auto whole = beats / beatsPerBar;
            const auto rest = (int) (beats % beatsPerBar);
            marks.push_back ({ bar, segmentTempo, whole > 0 ? beatsPerBar : rest });

            if (whole > 0 && rest > 0)
                marks.push_back ({ bar + whole, segmentTempo, rest });

            bar += whole + (rest > 0 ? 1 : 0);
            from = hit;
        }

        if (marks.empty())
            return fit;

        // After the last hit, the last tempo in whole bars.
        marks.push_back ({ bar, marks.back().tempo, beatsPerBar });

        fit.tempo = marks.front().tempo;
        fit.beatsPerBar = marks.front().beatsPerBar;
        auto current = marks.front();

        for (size_t i = 1; i < marks.size(); ++i)
            if (! juce::exactlyEqual (marks[i].tempo, current.tempo) || marks[i].beatsPerBar != current.beatsPerBar)
            {
                fit.changes.push_back (marks[i]);
                current = marks[i];
            }

        return fit;
    }
}

std::optional<TimeFormat::Timecode> timecodeOf (const Ffmpeg::Picture& picture, double videoStart, juce::int64 keepStart)
{
    const auto kind = TimeFormat::kindForFrameRate (picture.frameRate, picture.timecode.containsChar (';'));

    if (! kind.has_value())
        return std::nullopt;

    TimeFormat::Timecode tc { *kind, keepStart };

    if (const auto own = TimeFormat::parseCounted (picture.timecode, *kind))
    {
        // The session's start shows the video's own timecode less the frames before the video starts.
        const auto day = realOf ((juce::int64) 24 * 3600 * TimeFormat::countedFps (*kind), *kind);
        const auto lead = (juce::int64) std::llround (juce::jmax (0.0, videoStart) * TimeFormat::framesPerSecond (*kind));
        tc.start = countedOf (((realOf (*own, *kind) - lead) % day + day) % day, *kind);
    }

    return tc;
}

Fit fitTempo (const std::vector<juce::int64>& hits, double sampleRate, int beatsPerBar, LandOn landOn,
              double minTempo, double maxTempo, juce::int64 tolerance)
{
    if (hits.empty() || sampleRate <= 0.0 || beatsPerBar < 1 || minTempo < 20.0 || maxTempo > 400.0 || minTempo > maxTempo)
        return {};

    auto fit = fitOne (hits, sampleRate, beatsPerBar, landOn == LandOn::bars ? LandOn::bars : LandOn::beats, minTempo, maxTempo, juce::jmax ((juce::int64) 1, tolerance));

    if (landOn == LandOn::barLines)
        fit = fitMap (hits, sampleRate, beatsPerBar, fit.tempo);

    if (! fit.isValid())
        return {};

    fit.misses = missesOf ({ fit.tempo, fit.beatsPerBar, fit.changes, sampleRate }, hits, landOn);
    fit.within = (int) std::count_if (fit.misses.begin(), fit.misses.end(), [tolerance] (juce::int64 m) { return std::abs (m) <= tolerance; });
    return fit;
}

std::vector<juce::int64> missesOf (const Metronome::TempoMap& map, const std::vector<juce::int64>& hits, LandOn landOn)
{
    std::vector<juce::int64> misses;

    for (auto hit : hits)
    {
        juce::int64 before = 0, after = 0;

        if (landOn == LandOn::beats)
        {
            const auto beat = map.beatAt (hit);
            before = map.beatFrame (beat);
            after = map.beatFrame (beat + 1);
        }
        else
        {
            const auto bar = map.barAt (hit);
            before = map.barFrame (bar);
            after = map.barFrame (bar + 1);
        }

        misses.push_back (hit - before <= after - hit ? hit - before : hit - after);
    }

    return misses;
}

juce::int64 syncPoint (const Markers& fileMarkers, const AudioSnapshot& audio)
{
    const auto end = juce::jmax ((juce::int64) 0, audio.getNumFrames() - 1);

    for (auto& m : fileMarkers)
        if (m.name.containsIgnoreCase ("sync"))
            return juce::jlimit ((juce::int64) 0, end, m.start);

    if (! fileMarkers.empty())
    {
        const auto first = std::min_element (fileMarkers.begin(), fileMarkers.end(), [] (auto& a, auto& b) { return a.start < b.start; });
        return juce::jlimit ((juce::int64) 0, end, first->start);
    }

    // The loudest sample, on any channel: the hit of an impact, the middle of a pass-by.
    constexpr int block = 65536;
    juce::AudioBuffer<float> buffer (juce::jmax (1, audio.getNumChannels()), block);
    juce::int64 loudest = 0;
    auto peak = -1.0f;

    for (juce::int64 start = 0; start < audio.getNumFrames(); start += block)
    {
        const auto n = (int) juce::jmin ((juce::int64) block, audio.getNumFrames() - start);
        audio.read (buffer, 0, start, n);

        for (int ch = 0; ch < audio.getNumChannels(); ++ch)
        {
            const auto* data = buffer.getReadPointer (ch);

            for (int i = 0; i < n; ++i)
                if (std::abs (data[i]) > peak)
                {
                    peak = std::abs (data[i]);
                    loudest = start + i;
                }
        }
    }

    return loudest;
}

juce::StringArray spotFiles (SessionState& s, const juce::StringArray& files, int track, juce::int64 marker,
                             juce::AudioFormatManager& formats, int& added)
{
    juce::StringArray errors;
    added = 0;

    for (auto& path : files)
    {
        const juce::File file (path);
        auto loaded = AudioFileIO::load (file, formats);

        if (loaded.error.isNotEmpty())
        {
            errors.add (file.getFileName() + ": " + loaded.error);
            continue;
        }

        auto sync = syncPoint (loaded.markers, *loaded.snapshot);

        if (! juce::exactlyEqual (loaded.sampleRate, s.sampleRate))
        {
            loaded.snapshot = SampleRateConverter::convert (*loaded.snapshot, loaded.sampleRate, s.sampleRate);
            sync = (juce::int64) std::llround ((double) sync * s.sampleRate / loaded.sampleRate);
        }

        if (SessionEdits::spotClip (s, added == 0 ? track : -1, file, loaded.snapshot, marker, sync) == 0)
            errors.add (file.getFileName() + ": its sync point is further into it than the marker is into the session, past its end.");
        else
            ++added;
    }

    return errors;
}

juce::String nextName (const Markers& markers, const juce::String& base)
{
    int highest = 0;

    for (auto& m : markers)
        if (m.name.startsWith (base + " ") && m.name.substring (base.length() + 1).containsOnly ("0123456789"))
            highest = juce::jmax (highest, m.name.substring (base.length() + 1).getIntValue());

    return base + " " + juce::String (highest + 1).paddedLeft ('0', 2);
}
}

namespace SessionEdits
{
int addCutMarkers (SessionState& s, const std::vector<double>& videoSeconds, double frameRate)
{
    const auto halfFrame = 0.5 * s.sampleRate / (frameRate > 0.0 ? frameRate : 25.0);
    int added = 0;

    for (auto t : videoSeconds)
    {
        const auto frame = (juce::int64) std::llround ((s.videoStart + t) * s.sampleRate);
        const auto taken = std::any_of (s.markers.begin(), s.markers.end(),
                                        [frame, halfFrame] (const Marker& m) { return ! m.isRegion() && std::abs ((double) (m.start - frame)) <= halfFrame; });

        if (t < 0.0 || taken)
            continue;

        s.markers.push_back ({ frame, 0, Picture::nextName (s.markers, "Cut"), -1 });
        ++added;
    }

    MarkerEdits::sort (s.markers);
    return added;
}

void addHitMarker (SessionState& s, juce::int64 frame)
{
    s.markers.push_back ({ juce::jmax ((juce::int64) 0, frame), 0, Picture::nextName (s.markers, "Hit"), -1 });
    MarkerEdits::sort (s.markers);
}

bool setTimecode (SessionState& s, TimeFormat::Timecode tc)
{
    if (! tc.isSet())
        tc = {};

    tc.start = juce::jmax ((juce::int64) 0, tc.start);

    if (tc == s.timecode)
        return false;

    s.timecode = tc;
    return true;
}

bool applyTempoFit (SessionState& s, const Picture::Fit& fit)
{
    return fit.isValid() && setTempoMap (s, fit.tempo, fit.beatsPerBar, fit.changes);
}

juce::uint32 spotClip (SessionState& s, int track, const juce::File& source, AudioDocument::Snapshot audio, juce::int64 marker, juce::int64 sync)
{
    if (audio == nullptr)
        return 0;

    const auto position = marker - sync;
    const auto head = juce::jmax ((juce::int64) 0, -position);

    if (head >= audio->getNumFrames())
        return 0;

    const auto id = addClip (s, track, source, std::move (audio), position + head);

    if (head > 0)
        if (const auto where = s.findClip (id))
        {
            auto& clip = s.tracks[(size_t) where->first].clips[(size_t) where->second];
            clip.offset += head;
            clip.length -= head;
        }

    return id;
}
}
