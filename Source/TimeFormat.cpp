#include "TimeFormat.h"

namespace TimeFormat
{
    namespace
    {
        constexpr double fps23976 = 24000.0 / 1001.0;
        constexpr double fps2997 = 30000.0 / 1001.0;
        constexpr double epsilon = 1.0e-6;   // keeps a whole number of frames whole through the arithmetic

        std::atomic<Kind> current { Kind::minutesSeconds };

        juce::ChangeBroadcaster& changes()
        {
            static juce::ChangeBroadcaster b;
            return b;
        }

        int nominalFps (Kind kind)
        {
            switch (kind)
            {
                case Kind::smpte23976:
                case Kind::smpte24: return 24;
                case Kind::smpte25: return 25;
                case Kind::smpte2997Drop:
                case Kind::smpte2997:
                case Kind::smpte30: return 30;
                case Kind::minutesSeconds:
                case Kind::decimalSeconds:
                case Kind::samples:
                case Kind::count: return 0;
            }
        }

        /** The real frame of timecode a sample position falls in. */
        juce::int64 timecodeFrame (juce::int64 samples, double sampleRate, Kind kind)
        {
            return (juce::int64) std::floor ((double) juce::jmax ((juce::int64) 0, samples) / sampleRate * framesPerSecond (kind) + epsilon);
        }

        /** The first sample position in a real frame of timecode. */
        juce::int64 sampleOfFrame (juce::int64 frame, double sampleRate, Kind kind)
        {
            return (juce::int64) std::ceil ((double) frame * sampleRate / framesPerSecond (kind) - epsilon);
        }

        juce::String twoDigits (juce::int64 v)
        {
            return juce::String (v).paddedLeft ('0', 2);
        }

        /** hh:mm:ss:ff from the counted frames of a kind, hours left out when withHours is false and there are none. */
        juce::String timecode (juce::int64 counted, Kind kind, bool withHours)
        {
            const auto fps = nominalFps (kind);
            const auto hours = counted / (3600 * fps);
            const auto separator = isDropFrame (kind) ? ";" : ":";
            const auto rest = twoDigits ((counted / (60 * fps)) % 60) + ":" + twoDigits ((counted / fps) % 60) + separator + twoDigits (counted % fps);
            return withHours || hours > 0 ? twoDigits (hours) + ":" + rest : rest;
        }

        juce::String minutesSeconds (juce::int64 frames, double sampleRate)
        {
            const auto ms = (juce::int64) std::llround ((double) juce::jmax ((juce::int64) 0, frames) / sampleRate * 1000.0);
            const auto minutes = ms / 60000, hours = minutes / 60;
            const auto rest = juce::String::formatted ("%02d.%03d", (int) ((ms / 1000) % 60), (int) (ms % 1000));
            return hours > 0 ? juce::String::formatted ("%d:%02d:", (int) hours, (int) (minutes % 60)) + rest
                             : juce::String ((int) minutes) + ":" + rest;
        }

        int decimalsFor (double secondsPerTick)
        {
            return secondsPerTick >= 1.0 ? 0 : secondsPerTick >= 0.1 ? 1 : secondsPerTick >= 0.01 ? 2 : 3;
        }

        std::optional<juce::int64> wholeNumber (const juce::String& part)
        {
            const auto t = part.trim();

            if (t.isEmpty() || ! t.containsOnly ("0123456789"))
                return std::nullopt;

            return t.getLargeIntValue();
        }
    }

    juce::StringArray names()
    {
        return { "Minutes and Seconds", "Decimal Seconds", "Samples", "SMPTE 23.976 fps", "SMPTE 24 fps", "SMPTE 25 fps", "SMPTE 29.97 fps Drop", "SMPTE 29.97 fps", "SMPTE 30 fps" };
    }

    juce::String key (Kind kind)
    {
        static const juce::StringArray keys { "minutesSeconds", "decimalSeconds", "samples", "smpte23976", "smpte24", "smpte25", "smpte2997drop", "smpte2997", "smpte30" };
        return keys[juce::jlimit (0, keys.size() - 1, (int) kind)];
    }

    Kind fromKey (const juce::String& k)
    {
        for (int i = 0; i < (int) Kind::count; ++i)
            if (key ((Kind) i) == k)
                return (Kind) i;

        return Kind::minutesSeconds;
    }

    double framesPerSecond (Kind kind)
    {
        switch (kind)
        {
            case Kind::smpte23976: return fps23976;
            case Kind::smpte24: return 24.0;
            case Kind::smpte25: return 25.0;
            case Kind::smpte2997Drop:
            case Kind::smpte2997: return fps2997;
            case Kind::smpte30: return 30.0;
            case Kind::minutesSeconds:
            case Kind::decimalSeconds:
            case Kind::samples:
            case Kind::count: return 0.0;
        }
    }

    bool isDropFrame (Kind kind)   { return kind == Kind::smpte2997Drop; }
    bool isSmpte (Kind kind)       { return framesPerSecond (kind) > 0.0; }

    Kind get()                     { return current.load(); }

    void set (Kind kind)
    {
        if (kind == get() || kind == Kind::count)
            return;

        current.store (kind);
        changes().sendChangeMessage();
    }

    juce::ChangeBroadcaster& broadcaster()   { return changes(); }

    juce::int64 dropFrameCount (juce::int64 frames)
    {
        // 17,982 real frames in ten minutes; 18 frames dropped in them, 2 at each minute but the
        // first.
        const auto tenMinutes = frames / 17982, into = frames % 17982;
        return frames + 18 * tenMinutes + (into >= 2 ? 2 * ((into - 2) / 1798) : 0);
    }

    juce::int64 fromDropFrameCount (juce::int64 counted)
    {
        const auto minutes = counted / 1800;
        return counted - 2 * (minutes - minutes / 10);
    }

    juce::String format (juce::int64 frames, double sampleRate, Kind kind)
    {
        if (sampleRate <= 0.0)
            return "-";

        frames = juce::jmax ((juce::int64) 0, frames);

        switch (kind)
        {
            case Kind::decimalSeconds: return juce::String ((double) frames / sampleRate, 3);
            case Kind::samples:        return juce::String (frames);
            case Kind::minutesSeconds:
            case Kind::count:          return minutesSeconds (frames, sampleRate);
            case Kind::smpte23976:
            case Kind::smpte24:
            case Kind::smpte25:
            case Kind::smpte2997Drop:
            case Kind::smpte2997:
            case Kind::smpte30:        break;
        }

        const auto frame = timecodeFrame (frames, sampleRate, kind);
        return timecode (isDropFrame (kind) ? dropFrameCount (frame) : frame, kind, true);
    }

    std::optional<juce::int64> parse (const juce::String& typed, double sampleRate, Kind kind)
    {
        const auto text = typed.trim();

        if (text.isEmpty() || sampleRate <= 0.0)
            return std::nullopt;

        if (kind == Kind::samples)
            return wholeNumber (text);

        if (! isSmpte (kind))
        {
            // Seconds, m:ss or h:mm:ss, with a fraction on the last part.
            const auto parts = juce::StringArray::fromTokens (text.endsWithIgnoreCase ("s") ? text.dropLastCharacters (1) : text, ":", {});

            if (parts.size() < 1 || parts.size() > 3)
                return std::nullopt;

            double total = 0.0;

            for (int i = 0; i < parts.size(); ++i)
            {
                const auto t = parts[i].trim();

                if (t.isEmpty() || ! t.containsOnly ("0123456789.") || t.indexOfChar ('.') != t.lastIndexOfChar ('.')
                    || (i < parts.size() - 1 && t.containsChar ('.')) || (i > 0 && t.getDoubleValue() >= 60.0))
                    return std::nullopt;

                total = total * 60.0 + t.getDoubleValue();
            }

            return (juce::int64) std::llround (total * sampleRate);
        }

        const auto counted = parseCounted (text, kind);

        if (! counted.has_value())
            return std::nullopt;

        return sampleOfFrame (isDropFrame (kind) ? fromDropFrameCount (*counted) : *counted, sampleRate, kind);
    }

    int countedFps (Kind kind)   { return nominalFps (kind); }

    juce::String countedText (juce::int64 counted, Kind kind)
    {
        return isSmpte (kind) ? timecode (juce::jmax ((juce::int64) 0, counted), kind, true) : juce::String();
    }

    std::optional<juce::int64> parseCounted (const juce::String& text, Kind kind)
    {
        // Timecode: hh:mm:ss:ff, or mm:ss:ff; the frames may follow a semicolon.
        const auto parts = juce::StringArray::fromTokens (text.trim(), ":;", {});

        if (! isSmpte (kind) || parts.size() < 3 || parts.size() > 4)
            return std::nullopt;

        std::array<juce::int64, 4> values { 0, 0, 0, 0 };   // hours, minutes, seconds, frames

        for (int i = 0; i < parts.size(); ++i)
        {
            const auto v = wholeNumber (parts[i]);

            if (! v.has_value())
                return std::nullopt;

            values[(size_t) (i + 4 - parts.size())] = *v;
        }

        const auto fps = nominalFps (kind);

        if (values[1] >= 60 || values[2] >= 60 || values[3] >= fps)
            return std::nullopt;

        // 00 and 01 do not exist at the start of a minute that is not a tenth.
        if (isDropFrame (kind) && values[2] == 0 && values[3] < 2 && values[1] % 10 != 0)
            return std::nullopt;

        return ((values[0] * 60 + values[1]) * 60 + values[2]) * fps + values[3];
    }

    double rulerTick (double secondsPerPixel, double sampleRate, Kind kind, double minPixels)
    {
        std::vector<double> candidates;

        if (kind == Kind::samples && sampleRate > 0.0)
        {
            for (double step = 1.0; step <= 1.0e9; step *= 10.0)
                for (auto m : { 1.0, 2.0, 5.0 })
                    candidates.push_back (step * m / sampleRate);
        }
        else
        {
            if (isSmpte (kind))
                for (auto f : { 1.0, 2.0, 5.0, 10.0 })
                    candidates.push_back (f / framesPerSecond (kind));
            else
                candidates.insert (candidates.end(), { 0.001, 0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5 });

            candidates.insert (candidates.end(), { 1.0, 2.0, 5.0, 10.0, 15.0, 30.0, 60.0, 120.0, 300.0, 600.0, 1800.0, 3600.0 });
        }

        for (auto c : candidates)
            if (c / secondsPerPixel >= minPixels)
                return c;

        return candidates.back();
    }

    juce::String rulerLabel (double seconds, double secondsPerTick, double sampleRate, Kind kind)
    {
        seconds = juce::jmax (0.0, seconds);

        switch (kind)
        {
            case Kind::decimalSeconds: return juce::String (seconds, decimalsFor (secondsPerTick));
            case Kind::samples:        return juce::String ((juce::int64) std::llround (seconds * sampleRate));
            case Kind::minutesSeconds:
            case Kind::count:
            {
                const auto mins = (int) (seconds / 60.0);
                const auto secs = seconds - mins * 60.0;
                return juce::String (mins) + ":" + (secs < 10.0 ? "0" : "") + juce::String (secs, decimalsFor (secondsPerTick));
            }
            case Kind::smpte23976:
            case Kind::smpte24:
            case Kind::smpte25:
            case Kind::smpte2997Drop:
            case Kind::smpte2997:
            case Kind::smpte30:        break;
        }

        const auto frame = (juce::int64) std::floor (seconds * framesPerSecond (kind) + epsilon);
        return timecode (isDropFrame (kind) ? dropFrameCount (frame) : frame, kind, false);
    }

    //==============================================================================
    namespace
    {
        /** Real frames from counted ones, and back, at a kind's rate. */
        juce::int64 realOf (juce::int64 counted, Kind kind)   { return isDropFrame (kind) ? fromDropFrameCount (counted) : counted; }
        juce::int64 countedOf (juce::int64 real, Kind kind)   { return isDropFrame (kind) ? dropFrameCount (real) : real; }

        /** The real frames in a day of timecode. */
        juce::int64 realPerDay (Kind kind)                    { return realOf ((juce::int64) 24 * 3600 * nominalFps (kind), kind); }

        /** The timecode of the real frame `frame` frames on from the start, through midnight. */
        juce::String fromStart (juce::int64 frame, const Timecode& tc, bool withHours)
        {
            const auto day = realPerDay (tc.rate);
            const auto real = ((realOf (juce::jmax ((juce::int64) 0, tc.start), tc.rate) + frame) % day + day) % day;
            return timecode (countedOf (real, tc.rate), tc.rate, withHours);
        }
    }

    juce::String format (juce::int64 frames, double sampleRate, const Timecode& tc)
    {
        if (! tc.isSet())
            return format (frames, sampleRate);

        if (sampleRate <= 0.0)
            return "-";

        return fromStart (timecodeFrame (frames, sampleRate, tc.rate), tc, true);
    }

    std::optional<juce::int64> parse (const juce::String& text, double sampleRate, const Timecode& tc)
    {
        if (! tc.isSet())
            return parse (text, sampleRate);

        const auto counted = parseCounted (text, tc.rate);

        if (! counted.has_value() || sampleRate <= 0.0)
            return std::nullopt;

        const auto day = realPerDay (tc.rate);
        const auto real = ((realOf (*counted, tc.rate) - realOf (juce::jmax ((juce::int64) 0, tc.start), tc.rate)) % day + day) % day;
        return sampleOfFrame (real, sampleRate, tc.rate);
    }

    double rulerTick (double secondsPerPixel, double sampleRate, const Timecode& tc, double minPixels)
    {
        return rulerTick (secondsPerPixel, sampleRate, tc.isSet() ? tc.rate : get(), minPixels);
    }

    juce::String rulerLabel (double seconds, double secondsPerTick, double sampleRate, const Timecode& tc)
    {
        if (! tc.isSet())
            return rulerLabel (seconds, secondsPerTick, sampleRate);

        return fromStart ((juce::int64) std::floor (juce::jmax (0.0, seconds) * framesPerSecond (tc.rate) + epsilon), tc, false);
    }

    std::optional<Kind> kindForFrameRate (double fps, bool dropFrame)
    {
        const auto ntsc = dropFrame ? Kind::smpte2997Drop : Kind::smpte2997;
        const std::array<std::pair<double, Kind>, 8> rates { { { fps23976, Kind::smpte23976 }, { 24.0, Kind::smpte24 }, { 25.0, Kind::smpte25 },
                                                               { fps2997, ntsc }, { 30.0, Kind::smpte30 },
                                                               { 50.0, Kind::smpte25 }, { 2.0 * fps2997, ntsc }, { 60.0, Kind::smpte30 } } };
        std::optional<Kind> nearest;
        auto best = 0.005;   // within half a percent

        for (auto& [rate, kind] : rates)
            if (const auto off = std::abs (fps / rate - 1.0); off < best)
            {
                best = off;
                nearest = kind;
            }

        return nearest;
    }
}
