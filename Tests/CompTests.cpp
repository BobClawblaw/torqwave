#pragma clang diagnostic ignored "-Wfloat-equal"

#include <JuceHeader.h>
#include "../Source/AudioEdits.h"
#include "../Source/AudioFileIO.h"
#include "../Source/Session.h"
#include "../Source/SessionMixer.h"
#include "TestUtils.h"
#include "CompTests.h"

namespace
{
    using TestUtils::check;

    constexpr double rate = 48000.0;

    AudioDocument::Snapshot noise (int channels, int frames, int seed)
    {
        juce::AudioBuffer<float> b (channels, frames);
        juce::Random random (seed);

        for (int ch = 0; ch < channels; ++ch)
            for (int i = 0; i < frames; ++i)
                b.setSample (ch, i, random.nextFloat() - 0.5f);

        return AudioEdits::fromBuffer (std::move (b));
    }

    AudioDocument::Snapshot level (int channels, int frames, float value)
    {
        juce::AudioBuffer<float> b (channels, frames);

        for (int ch = 0; ch < channels; ++ch)
            juce::FloatVectorOperations::fill (b.getWritePointer (ch), value, frames);

        return AudioEdits::fromBuffer (std::move (b));
    }

    /** A track with one clip of three takes of noise, each its own length and shift: the clip
        starts at session frame 1000, plays 30000 frames from frame 2000 of take 0. */
    SessionState threeTakes (juce::uint32& id)
    {
        SessionState s;
        s.sampleRate = rate;
        auto& t = s.addTrack();
        SessionClip c;
        c.id = id = s.nextId++;
        c.takes = { { {}, noise (2, 40000, 1), 0 }, { {}, noise (2, 45000, 2), 3000 }, { {}, noise (2, 38000, 3), -1500 } };
        c.audio = c.takes[0].audio;
        c.position = 1000;
        c.offset = 2000;
        c.length = 30000;
        t.clips.push_back (c);
        return s;
    }

    juce::AudioBuffer<float> play (const SessionState& s, juce::int64 start, int frames, int blockSize = 256)
    {
        juce::AudioBuffer<float> out (2, frames);
        out.clear();
        SessionMixer::Buffers buffers;
        buffers.reserve (512);

        for (int p = 0; p < frames; p += blockSize)
            SessionMixer::render (s, start + p, out, p, juce::jmin (blockSize, frames - p), buffers, nullptr);

        return out;
    }

    /** Whether a and b agree exactly over [from, to). */
    bool same (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b, int from, int to)
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int i = from; i < to; ++i)
                if (a.getSample (ch, i) != b.getSample (ch, i))
                    return false;

        return true;
    }

    const SessionClip& clipOf (const SessionState& s, juce::uint32 id)
    {
        const auto w = s.findClip (id);
        return s.tracks[(size_t) w->first].clips[(size_t) w->second];
    }

    void testPlayback()
    {
        juce::uint32 id = 0;
        const auto plain = threeTakes (id);

        // Take 1 over session frames 8000 to 15000, take 2 from 20000 on, cut hard.
        auto s = plain;
        check (SessionEdits::compTake (s, id, 1, 8000, 15000) && SessionEdits::compTake (s, id, 2, 20000, 99999)
                   && SessionEdits::setCompFade (s, id, 0),
               "comping: takes are comped over ranges");
        const auto& c = clipOf (s, id);
        check (c.comp.size() == 4 && c.comp[1].take == 1 && c.comp[2].take == 0 && c.comp[3].take == 2
                   && c.comp[1].from == c.takeZeroFrame (8000) && c.comp[3].from == c.takeZeroFrame (20000),
               "comping: the comp holds each range's take from its start, in take-0 frames");

        // Each part against the clip playing that take throughout.
        const auto got = play (s, 0, 34000);
        auto heard = [&] (int take)
        {
            auto t = plain;
            SessionEdits::playTakeThroughout (t, id, take);
            return play (t, 0, 34000);
        };

        const auto take0 = heard (0), take1 = heard (1), take2 = heard (2);
        check (same (got, take0, 0, 8000) && same (got, take1, 8000, 15000) && same (got, take0, 15000, 20000)
                   && same (got, take2, 20000, 34000),
               "comping: a comp of three takes plays each take sample for sample over its part, each at its shift");

        // With blocks of other sizes, and a block starting mid-join.
        check (same (play (s, 0, 34000, 97), got, 0, 34000) && same (play (s, 0, 34000, 1), got, 0, 34000),
               "comping: playback doesn't depend on the block size");

        // Crossfaded: the same away from the joins, and inside them the equal-power mix.
        SessionEdits::setCompFade (s, id, 480);
        const auto faded = play (s, 0, 34000);
        check (same (faded, take0, 0, 8000 - 240) && same (faded, take1, 8000 + 240, 15000 - 240)
                   && same (faded, take2, 20000 + 240, 34000),
               "comping: away from the joins a crossfaded comp plays as a hard-cut one");

        auto worst = 0.0f;

        for (int i = 8000 - 240; i < 8000 + 240; ++i)
        {
            const auto x = ((double) (i - (8000 - 240)) + 0.5) / 480.0;
            const auto out = (float) std::cos (x * juce::MathConstants<double>::halfPi), in = (float) std::sin (x * juce::MathConstants<double>::halfPi);

            for (int ch = 0; ch < 2; ++ch)
                worst = juce::jmax (worst, std::abs (faded.getSample (ch, i) - (take0.getSample (ch, i) * out + take1.getSample (ch, i) * in)));
        }

        check (worst < 1.0e-6f, "comping: across a join the outgoing take fades out and the incoming in, with equal power ("
                                    + juce::String (worst) + ")");
    }

    void testJoinLevels()
    {
        // Two takes at constant, unrelated levels: the join is cos/sin, centred on it.
        SessionClip c;
        c.takes = { { {}, level (1, 4000, 0.5f), 0 }, { {}, level (1, 4000, -0.25f), 0 } };
        c.audio = c.takes[0].audio;
        c.length = 4000;
        c.comp = { { 0, 0 }, { 2000, 1 } };
        c.compFade = 100;
        juce::AudioBuffer<float> out (1, 4000), other (1, 4000);
        Comp::read (c, 0, 4000, out, other);

        const auto first = out.getSample (0, 1950), last = out.getSample (0, 2049);
        const auto g0 = std::cos (0.5 / 100.0 * juce::MathConstants<double>::halfPi), g1 = std::sin (0.5 / 100.0 * juce::MathConstants<double>::halfPi);
        check (out.getSample (0, 1949) == 0.5f && out.getSample (0, 2050) == -0.25f
                   && std::abs (first - (float) (0.5 * g0 - 0.25 * g1)) < 1.0e-6f && std::abs (last - (float) (0.5 * g1 - 0.25 * g0)) < 1.0e-6f,
               "comping: a 100-frame join runs from 50 frames before it to 50 after");

        // Short segments: no two crossfades meet, and a segment shorter than the fade still sounds.
        c.comp = { { 0, 0 }, { 2000, 1 }, { 2010, 0 } };
        Comp::read (c, 0, 4000, out, other);
        check (out.getSample (0, 1994) == 0.5f && out.getSample (0, 2015) == 0.5f && out.getSample (0, 2004) < 0.5f
                   && out.getSample (0, 2005) < 0.5f,
               "comping: joins closer than the fade shorten their crossfades so they don't meet");
    }

    void testEdits()
    {
        juce::uint32 id = 0;
        auto s = threeTakes (id);
        const auto clip = [&] () -> const SessionClip& { return clipOf (s, id); };

        check (! SessionEdits::compTake (s, id, 0, 5000, 6000) && clip().comp.empty(),
               "comping: the active take over a range is no change");
        check (! SessionEdits::compTake (s, id, 3, 5000, 6000) && ! SessionEdits::compTake (s, id, 1, 40000, 50000),
               "comping: a take that isn't there, or a range outside the clip, is refused");

        SessionEdits::compTake (s, id, 1, 5000, 9000);
        check (clip().compFade == 480, "comping: the first comp crossfades its joins over 10 ms");
        SessionEdits::compTake (s, id, 2, 6000, 7000);
        check (clip().comp.size() == 5 && clip().takeAt (clip().takeZeroFrame (6500)) == 2 && clip().takeAt (clip().takeZeroFrame (7500)) == 1,
               "comping: a range inside another take's splits it");

        SessionEdits::compTake (s, id, 0, 4000, 10000);
        check (clip().comp.empty(), "comping: comping the active take back over every other part ends the comp");

        SessionEdits::compTake (s, id, 1, 0, 40000);
        check (clip().comp.size() == 1 && clip().takeAt (0) == 1 && clip().activeTake == 0,
               "comping: one take over the whole clip is a comp of one");

        check (SessionEdits::playTakeThroughout (s, id, 2) && clip().comp.empty() && clip().activeTake == 2,
               "comping: playing a take throughout ends the comp and puts it in use");
        check (! SessionEdits::playTakeThroughout (s, id, 2), "comping: the take in use already, with no comp, is no change");

        // A locked clip.
        SessionEdits::compTake (s, id, 1, 5000, 9000);
        SessionEdits::setClipLocked (s, id, true);
        check (! SessionEdits::compTake (s, id, 0, 5000, 9000) && ! SessionEdits::playTakeThroughout (s, id, 1)
                   && ! SessionEdits::setCompFade (s, id, 0),
               "comping: a locked clip's comp can't be changed");
    }

    void testNormalize()
    {
        // Take 1 is twice as loud as take 0: normalizing a comp with a little of take 1 goes by
        // take 1's peak, not the active take's.
        SessionState s;
        s.sampleRate = rate;
        auto& t = s.addTrack();
        SessionClip c;
        c.id = s.nextId++;
        c.takes = { { {}, level (1, 10000, 0.25f), 0 }, { {}, level (1, 10000, 0.5f), 0 } };
        c.audio = c.takes[0].audio;
        c.length = 10000;
        t.clips.push_back (c);

        SessionEdits::compTake (s, c.id, 1, 4000, 4100);
        SessionEdits::setCompFade (s, c.id, 0);   // crossfaded, like levels would sum to more than either
        SessionEdits::normalizeClip (s, c.id, 0.0f);
        check (std::abs (clipOf (s, c.id).gainDb - 6.0206f) < 0.001f, "comping: Normalize goes by the loudest part the comp plays ("
                                                                         + juce::String (clipOf (s, c.id).gainDb, 3) + " dB)");
    }

    void testKeptThroughEdits()
    {
        juce::uint32 id = 0;
        auto s = threeTakes (id);
        SessionEdits::compTake (s, id, 1, 8000, 15000);
        SessionEdits::compTake (s, id, 2, 20000, 99999);
        const auto before = play (s, 0, 34000);

        // Moved by 1234 frames: the same sound, later.
        auto moved = s;
        SessionEdits::moveClip (moved, id, 0, 1000 + 1234);
        const auto after = play (moved, 1234, 34000);
        check (same (before, after, 0, 34000), "comping: a moved clip plays its comp as it did");

        // Trimmed at both ends, and with another take chosen: each frame plays as it did.
        auto trimmed = s;
        SessionEdits::trimClip (trimmed, id, 5000, 25000);
        SessionEdits::chooseTake (trimmed, id, 1);
        const auto cut = play (trimmed, 0, 34000);
        check (same (before, cut, 5000, 25000), "comping: a trimmed clip, with another take chosen, plays its comp as it did");

        // Split inside a crossfaded join and either side: both parts play as the whole did, but
        // for the clip ends at the split.
        auto split = s;
        SessionEdits::splitClip (split, id, 12000);
        SessionEdits::splitClip (split, id, 8100);
        const auto parts = play (split, 0, 34000);
        check (split.tracks[0].clips.size() == 3 && same (before, parts, 0, 34000),
               "comping: split clips each play their part of the comp");

        // Reversed with its takes: the comp turns round with them.
        auto reversed = s;
        const auto& c = clipOf (reversed, id);
        std::vector<SessionClip::Take> takes;

        for (int k = 0; k < 3; ++k)
        {
            const auto played = SessionEdits::takeAsPlayed (c, k);
            const auto audio = k == c.activeTake ? SessionEdits::playedPart (c, c.audio) : played.audio;
            juce::AudioBuffer<float> b (2, (int) c.length);
            audio->read (b, 0, 0, (int) c.length);
            b.reverse (0, (int) c.length);
            takes.push_back ({ {}, AudioEdits::fromBuffer (std::move (b)), 0 });
        }

        const auto active = takes[(size_t) c.activeTake].audio;
        SessionEdits::reverseClip (reversed, id, {}, active, takes);
        const auto back = play (reversed, 0, 34000);
        auto mirrored = true;

        // Away from the joins (whose crossfades are mirrored too), frame i plays what frame
        // 30999 - (i - 1000) did.
        for (int i = 1000; i < 31000 && mirrored; ++i)
        {
            const auto j = 31000 - 1 - (i - 1000);

            if (std::abs (j - 8000) < 241 || std::abs (j - 15000) < 241 || std::abs (j - 20000) < 241)
                continue;

            mirrored = back.getSample (0, i) == before.getSample (0, j) && back.getSample (1, i) == before.getSample (1, j);
        }

        check (mirrored && clipOf (reversed, id).comp.size() == 4, "comping: a reversed clip plays its comp back to front");
    }

    void testFlatten()
    {
        juce::uint32 id = 0;
        auto s = threeTakes (id);
        SessionEdits::compTake (s, id, 1, 8000, 15000);
        SessionEdits::compTake (s, id, 2, 20000, 99999);
        SessionEdits::setClipFades (s, id, 300, 700);
        SessionEdits::setClipGainEnvelope (s, id, Envelope { { { 2000, -6.0f }, { 20000, 3.0f } } });
        const auto before = play (s, 0, 34000);

        const auto audio = SessionEdits::compAudio (clipOf (s, id));
        check (audio != nullptr && audio->getNumFrames() == 30000, "comping: the comp renders as long as the clip");
        check (SessionEdits::flattenComp (s, id, {}, audio), "comping: Flatten Comp makes it a take");

        const auto& c = clipOf (s, id);
        check (c.comp.empty() && c.takes.size() == 4 && c.activeTake == 3 && c.audio == audio && c.offset == 0,
               "comping: the flattened take is a new take, in use, the others kept");
        check (same (play (s, 0, 34000), before, 0, 34000), "comping: a flattened comp plays exactly what the comp did, fades and gain line too");
        check (! SessionEdits::flattenComp (s, id, {}, audio), "comping: with no comp there is nothing to flatten");

        // The other takes still line up: take 1 throughout plays as it did.
        juce::uint32 id2 = 0;
        auto fresh = threeTakes (id2);
        SessionEdits::setClipFades (fresh, id2, 300, 700);
        SessionEdits::playTakeThroughout (fresh, id2, 1);
        SessionEdits::playTakeThroughout (s, id, 1);
        auto flat = s;
        SessionEdits::setClipGainEnvelope (flat, id, {});
        check (same (play (flat, 0, 34000), play (fresh, 0, 34000), 0, 34000),
               "comping: after a flatten, the other takes play as they did");
    }

    void testFileAndUndo (juce::AudioFormatManager& formats)
    {
        const auto folder = juce::File::createTempFile ("comp");
        folder.createDirectory();
        const auto wav = folder.getChildFile ("Track 1.wav");
        const auto audio = noise (2, 14000, 7);
        AudioFileIO::save (*audio, rate, wav, 32, formats);

        SessionState s;
        s.sampleRate = rate;
        s.addTrack();
        const auto id = SessionEdits::recordTakes (s, 0, wav, audio, 1000, 4000, { 0, 4000, 8000 });
        SessionEdits::compTake (s, id, 0, 2000, 2500);
        SessionEdits::compTake (s, id, 1, 3000, 4000);
        SessionEdits::setCompFade (s, id, 64);

        const auto session = folder.getChildFile ("Comp.tqsession");
        const auto error = SessionFile::save (s, session);
        const auto loaded = SessionFile::load (session, formats);
        const auto ok = error.isEmpty() && loaded.error.isEmpty() && loaded.state.tracks.size() == 1 && loaded.state.tracks[0].clips.size() == 1;
        check (ok && loaded.state.tracks[0].clips[0].comp == clipOf (s, id).comp && loaded.state.tracks[0].clips[0].compFade == 64,
               "comping: a comp and its crossfade come back from a session file");
        check (ok && same (play (loaded.state, 0, 6000), play (s, 0, 6000), 0, 6000), "comping: the loaded comp plays as it did");

        // Undo and redo, each a step.
        Session undoable;
        auto start = s;
        SessionEdits::playTakeThroughout (start, id, 2);
        undoable.reset (start, {});
        auto next = *undoable.getState();
        SessionEdits::compTake (next, id, 0, 2000, 3000);
        undoable.apply (std::move (next), "Comp Take");
        undoable.undo();
        const auto undone = undoable.getState()->tracks[0].clips[0].comp.empty();
        undoable.redo();
        check (undone && undoable.getState()->tracks[0].clips[0].comp.size() == 3, "comping: comping a range undoes and redoes as one step");

        folder.deleteRecursively();
    }
}

void runCompTests (juce::AudioFormatManager& formats)
{
    testPlayback();
    testJoinLevels();
    testEdits();
    testNormalize();
    testKeptThroughEdits();
    testFlatten();
    testFileAndUndo (formats);
}
