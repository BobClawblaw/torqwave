#include "Metronome.h"

namespace Metronome
{
namespace
{
    double beatLength (double tempo, double rate)
    {
        return 60.0 * rate / juce::jlimit (1.0, 1000.0, tempo);
    }
}

juce::int64 beatFrame (juce::int64 beat, double tempo, double rate)
{
    return (juce::int64) std::llround ((double) beat * beatLength (tempo, rate));
}

juce::int64 beatAt (juce::int64 frame, double tempo, double rate)
{
    auto beat = (juce::int64) std::floor ((double) frame / beatLength (tempo, rate));

    // The rounding of beat starts can put the frame on either side of the estimate.
    while (beatFrame (beat + 1, tempo, rate) <= frame)
        ++beat;

    while (beatFrame (beat, tempo, rate) > frame)
        --beat;

    return beat;
}

juce::int64 barFrames (double tempo, int beatsPerBar, double rate)
{
    return beatFrame (juce::jmax (1, beatsPerBar), tempo, rate);
}

namespace
{
    juce::int64 floorDiv (juce::int64 a, juce::int64 b)
    {
        return a >= 0 ? a / b : -((-a + b - 1) / b);
    }
}

template <typename Before>
TempoMap::Segment TempoMap::find (Before before) const
{
    Segment s { 0, 0, 0.0, beatLength (startTempo, rate), juce::jmax (1, startBeats) };

    for (auto& c : changes)
    {
        if (c.bar <= s.bar)
            continue;

        const auto beats = (c.bar - s.bar) * s.beatsPerBar;
        const Segment next { c.bar, s.beat + beats, s.frame + (double) beats * s.beatLength, beatLength (c.tempo, rate), juce::jmax (1, c.beatsPerBar) };

        if (before (next))
            break;

        s = next;
    }

    return s;
}

juce::int64 TempoMap::beatFrame (juce::int64 beat) const
{
    const auto s = find ([beat] (const Segment& next) { return beat < next.beat; });
    return (juce::int64) std::llround (s.frame + (double) (beat - s.beat) * s.beatLength);
}

juce::int64 TempoMap::beatAt (juce::int64 frame) const
{
    const auto s = find ([frame] (const Segment& next) { return frame < startOf (next); });
    auto beat = s.beat + (juce::int64) std::floor (((double) frame - s.frame) / s.beatLength);

    // The rounding of beat starts can put the frame on either side of the estimate.
    while (beatFrame (beat + 1) <= frame)
        ++beat;

    while (beatFrame (beat) > frame)
        --beat;

    return beat;
}

double TempoMap::beatPosition (juce::int64 frame) const
{
    const auto beat = beatAt (frame);
    const auto start = beatFrame (beat), next = beatFrame (beat + 1);
    return (double) beat + (next > start ? (double) (frame - start) / (double) (next - start) : 0.0);
}

juce::int64 TempoMap::frameAtBeatPosition (double beats) const
{
    const auto beat = (juce::int64) std::floor (beats);
    const auto start = beatFrame (beat), next = beatFrame (beat + 1);
    return start + (juce::int64) std::llround ((beats - (double) beat) * (double) (next - start));
}

std::pair<juce::int64, int> TempoMap::barAndBeat (juce::int64 beat) const
{
    const auto s = find ([beat] (const Segment& next) { return beat < next.beat; });
    const auto bars = floorDiv (beat - s.beat, s.beatsPerBar);
    return { s.bar + bars, (int) (beat - s.beat - bars * s.beatsPerBar) };
}

juce::int64 TempoMap::barStartBeat (juce::int64 bar) const
{
    const auto s = find ([bar] (const Segment& next) { return bar < next.bar; });
    return s.beat + (bar - s.bar) * s.beatsPerBar;
}

double TempoMap::tempoAt (juce::int64 frame) const
{
    const auto s = find ([frame] (const Segment& next) { return frame < startOf (next); });
    return 60.0 * rate / s.beatLength;
}

int TempoMap::beatsPerBarAt (juce::int64 frame) const
{
    return find ([frame] (const Segment& next) { return frame < startOf (next); }).beatsPerBar;
}

double TempoMap::shortestBeat() const
{
    auto shortest = beatLength (startTempo, rate);

    for (auto& c : changes)
        if (c.bar > 0)
            shortest = juce::jmin (shortest, beatLength (c.tempo, rate));

    return shortest;
}

double TempoMap::shortestBar() const
{
    auto shortest = beatLength (startTempo, rate) * juce::jmax (1, startBeats);

    for (auto& c : changes)
        if (c.bar > 0)
            shortest = juce::jmin (shortest, beatLength (c.tempo, rate) * juce::jmax (1, c.beatsPerBar));

    return shortest;
}

std::pair<juce::int64, juce::int64> TempoMap::linesAround (juce::int64 frame, Grid grid) const
{
    if (grid.bars <= 0 && grid.division > 1)
    {
        const auto div = (double) grid.division;
        auto k = (juce::int64) std::floor (beatPosition (frame) * div);

        while (k > 0 && frameAtBeatPosition ((double) k / div) > frame)   // rounding: the line at or before
            --k;

        return { frameAtBeatPosition ((double) k / div), frameAtBeatPosition ((double) (k + 1) / div) };
    }

    if (grid.bars <= 0)
    {
        const auto beat = beatAt (frame);
        return { beatFrame (beat), beatFrame (beat + 1) };
    }

    const auto below = floorDiv (barAt (frame), grid.bars) * grid.bars;
    return { barFrame (below), barFrame (below + grid.bars) };
}

void TempoMap::forEachLine (juce::int64 first, juce::int64 last, Grid grid,
                            const std::function<void (juce::int64, juce::int64)>& line) const
{
    if (grid.bars <= 0 && grid.division > 1)
    {
        const auto div = (double) grid.division;

        for (auto k = (juce::int64) std::floor (beatPosition (juce::jmax ((juce::int64) 0, first)) * div); ; ++k)
        {
            const auto frame = frameAtBeatPosition ((double) k / div);

            if (frame > last)
                break;

            if (frame >= first)
                line (frame, floorDiv (k, (juce::int64) grid.division));
        }

        return;
    }

    if (grid.bars <= 0)
    {
        for (auto beat = beatAt (first); beatFrame (beat) <= last; ++beat)
            if (beatFrame (beat) >= first)
                line (beatFrame (beat), beat);

        return;
    }

    for (auto bar = floorDiv (barAt (first), grid.bars) * grid.bars; barFrame (bar) <= last; bar += grid.bars)
        if (barFrame (bar) >= first)
            line (barFrame (bar), barStartBeat (bar));
}

juce::String positionText (const TempoMap& map, juce::int64 frame)
{
    const auto position = map.beatPosition (juce::jmax ((juce::int64) 0, frame));
    const auto beat = (juce::int64) std::floor (position);
    const auto ticks = juce::jlimit (0, 479, (int) std::floor ((position - (double) beat) * 480.0 + 1.0e-6));
    const auto [bar, inBar] = map.barAndBeat (beat);
    return juce::String (bar + 1) + "." + juce::String (inBar + 1) + "." + juce::String (ticks / 120 + 1) + "." + juce::String (ticks % 120);
}

std::optional<juce::int64> parsePosition (const TempoMap& map, const juce::String& text)
{
    auto parts = juce::StringArray::fromTokens (text.trim(), ".", {});
    parts.trim();

    if (parts.isEmpty() || parts.size() > 4)
        return std::nullopt;

    int values[4] { 0, 1, 1, 0 };

    for (int i = 0; i < parts.size(); ++i)
    {
        if (parts[i].isEmpty() || ! parts[i].containsOnly ("0123456789") || parts[i].length() > 7)
            return std::nullopt;

        values[i] = parts[i].getIntValue();
    }

    const auto [bar, beat, sixteenth, ticks] = values;

    if (bar < 1 || beat < 1 || sixteenth < 1 || sixteenth > 4 || ticks > 119)
        return std::nullopt;

    const auto barStart = map.barStartBeat (bar - 1);

    if (beat > map.beatsPerBarAt (map.beatFrame (barStart)))
        return std::nullopt;

    return map.frameAtBeatPosition ((double) (barStart + beat - 1) + (double) ((sixteenth - 1) * 120 + ticks) / 480.0);
}

float sample (juce::int64 offset, bool accent, double rate)
{
    const auto length = (juce::int64) std::llround (clickSeconds * rate);

    if (offset < 0 || offset >= length)
        return 0.0f;

    const auto frequency = accent ? 1760.0 : 1320.0;
    const auto level = accent ? 0.5 : 0.35;
    const auto fade = 1.0 - (double) offset / (double) length;
    return (float) (level * fade * fade * std::sin (juce::MathConstants<double>::twoPi * frequency * (double) offset / rate));
}
}
