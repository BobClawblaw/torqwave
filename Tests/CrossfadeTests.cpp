#include <JuceHeader.h>
#include "../Source/AudioEngine.h"
#include "../Source/AudioFileIO.h"
#include "../Source/MultitrackView.h"
#include "../Source/SessionMixer.h"
#include "TestUtils.h"
#include "CrossfadeTests.h"

namespace
{
    using TestUtils::check;

    constexpr double rate = 48000.0;

    AudioDocument::Snapshot snapshotOf (juce::AudioBuffer<float> b)
    {
        const auto frames = b.getNumSamples(), channels = b.getNumChannels();
        std::vector<AudioSnapshot::Segment> segments { { std::make_shared<const AudioBlock> (std::move (b)), 0, frames } };
        return std::make_shared<const AudioSnapshot> (channels, std::move (segments));
    }

    AudioDocument::Snapshot noise (int frames, int seed)
    {
        juce::AudioBuffer<float> b (1, frames);
        juce::Random random (seed);

        for (int i = 0; i < frames; ++i)
            b.setSample (0, i, random.nextFloat() - 0.5f);

        return snapshotOf (std::move (b));
    }

    AudioDocument::Snapshot steady (int frames, float value)
    {
        juce::AudioBuffer<float> b (1, frames);

        for (int i = 0; i < frames; ++i)
            b.setSample (0, i, value);

        return snapshotOf (std::move (b));
    }

    // A clip playing `length` frames of `audio` from `offset`, at `position`.
    SessionClip clipOf (SessionState& s, AudioDocument::Snapshot audio, juce::int64 position, juce::int64 offset, juce::int64 length)
    {
        SessionClip c;
        c.id = s.nextId++;
        c.name = "clip";
        c.audio = std::move (audio);
        c.position = position;
        c.offset = offset;
        c.length = length;
        return c;
    }

    // The left channel of the session over [start, start + frames), the mono clips at the centre
    // brought back to their own level.
    std::vector<float> played (const SessionState& s, juce::int64 start, int frames)
    {
        juce::AudioBuffer<float> out (2, frames);
        SessionMixer::Buffers scratch;
        SessionMixer::render (s, start, out, 0, frames, scratch);
        std::vector<float> left ((size_t) frames);

        for (int i = 0; i < frames; ++i)
            left[(size_t) i] = out.getSample (0, i) / std::sqrt (0.5f);

        return left;
    }

    float worstAgainst (const std::vector<float>& heard, const AudioSnapshot& audio, juce::int64 from)
    {
        juce::AudioBuffer<float> b (1, (int) heard.size());
        audio.read (b, 0, from, b.getNumSamples());
        auto worst = 0.0f;

        for (size_t i = 0; i < heard.size(); ++i)
            worst = juce::jmax (worst, std::abs (heard[i] - b.getSample (0, (int) i)));

        return worst;
    }

    double rmsDb (const std::vector<float>& v, size_t from, size_t to)
    {
        double sum = 0.0;

        for (auto i = from; i < to; ++i)
            sum += (double) v[i] * v[i];

        return 10.0 * std::log10 (sum / (double) (to - from));
    }

    const SessionClip& clip (const SessionState& s, juce::uint32 id)
    {
        const auto at = s.findClip (id);
        return s.tracks[(size_t) at->first].clips[(size_t) at->second];
    }

    /** A file's first half and second half as two clips that meet at 24,000, with 24,000 frames of
        handle each: the out clip can reach to the file's end, the in clip back to its start. */
    struct Halves
    {
        SessionState s;
        AudioDocument::Snapshot audio;
        juce::uint32 out = 0, in = 0;
    };

    Halves halves (AudioDocument::Snapshot audio)
    {
        Halves h;
        h.s.sampleRate = rate;
        h.audio = audio;
        auto& t = h.s.addTrack();
        t.clips.push_back (clipOf (h.s, audio, 0, 0, 24000));
        t.clips.push_back (clipOf (h.s, audio, 24000, 24000, 24000));
        h.out = t.clips[0].id;
        h.in = t.clips[1].id;
        return h;
    }

    void testCrossfadeClips()
    {
        auto h = halves (noise (48000, 1));
        auto& s = h.s;
        const auto untouched = played (s, 0, 48000);
        check (SessionEdits::crossfadesOn (s, 0).empty(), "crossfades: two clips that only touch make none");

        check (SessionEdits::crossfadeClips (s, h.in, h.out, 480).isEmpty(), "crossfades: two clips that touch are crossfaded (chosen in either order)");
        const auto made = SessionEdits::crossfadesOn (s, 0);
        check (made.size() == 1 && made[0].out == h.out && made[0].in == h.in && made[0].start == 23760 && made[0].end == 24240 && made[0].join() == 24000,
               "crossfades: 480 frames centred where they touched, 23,760 to 24,240, the earlier clip going out");
        check (clip (s, h.out).getEnd() == 24240 && clip (s, h.out).fadeOut == 480 && clip (s, h.in).position == 23760 && clip (s, h.in).offset == 23760
                   && clip (s, h.in).length == 24240 && clip (s, h.in).fadeIn == 480,
               "crossfades: the out clip reaches into its audio after its end, the in clip into its audio before its start, staying in time");
        check (clip (s, h.out).fadeOutShape == FadeShape::sCurve && clip (s, h.in).fadeInShape == FadeShape::sCurve,
               "crossfades: Crossfade Selected Clips gives both sides an S-curve (equal gain)");
        check (SessionEdits::crossfadeAt (s, 0, 23760).has_value() && SessionEdits::crossfadeAt (s, 0, 24239).has_value()
                   && ! SessionEdits::crossfadeAt (s, 0, 23759).has_value() && ! SessionEdits::crossfadeAt (s, 0, 24240).has_value(),
               "crossfades: found at the frames of their overlap and not beside it");
        check (SessionEdits::crossfadesIn (s, 0, 23761).size() == 1 && SessionEdits::crossfadesIn (s, 24240, 48000).empty(),
               "crossfades: found in a time range they overlap");

        // A file crossfading into more of itself, equal gain: what it played before.
        for (auto [outShape, inShape, name] : { std::tuple (FadeShape::sCurve, FadeShape::sCurve, "S-curve"),
                                                std::tuple (FadeShape::linear, FadeShape::linear, "linear"),
                                                std::tuple (FadeShape::fast, FadeShape::slow, "fast start out, slow start in") })
        {
            SessionEdits::setCrossfadeShapes (s, h.out, h.in, outShape, inShape);
            const auto worst = worstAgainst (played (s, 0, 48000), *h.audio, 0);
            std::cout << "    a file crossfaded into itself, " << name << ": worst difference " << worst << std::endl;
            check (worst < 1.0e-6f, juce::String ("crossfades: a file crossfaded into more of itself plays as it did, ") + name + " both sides");
        }

        // Removing it leaves a butt join, where the file plays bit for bit.
        check (SessionEdits::removeCrossfade (s, h.out, h.in) && SessionEdits::crossfadesOn (s, 0).empty()
                   && clip (s, h.out).getEnd() == 24000 && clip (s, h.in).position == 24000 && clip (s, h.out).fadeOut == 0 && clip (s, h.in).fadeIn == 0,
               "crossfades: removed, the clips meet at its join with no fades");
        check (played (s, 0, 48000) == untouched, "crossfades: after removing one, the clips play bit for bit as before it");

        // Overlapping clips meet at the middle of their overlap.
        {
            auto o = halves (noise (48000, 2));
            auto& t = o.s.tracks[0];
            SessionEdits::trimClip (o.s, o.out, 0, 25000);
            SessionEdits::trimClip (o.s, o.in, 23000, 48000);
            t.clips[0].fadeOut = 300;
            check (SessionEdits::crossfadeClips (o.s, o.out, o.in, 960).isEmpty() && SessionEdits::crossfadesOn (o.s, 0).size() == 1
                       && SessionEdits::crossfadesOn (o.s, 0)[0].start == 23520 && SessionEdits::crossfadesOn (o.s, 0)[0].end == 24480,
                   "crossfades: overlapping clips are crossfaded centred on the middle of their overlap");
        }

        // Refused, each with a reason.
        {
            SessionState r;
            r.sampleRate = rate;
            auto& t = r.addTrack();
            const auto a = noise (24000, 3), b = noise (24000, 4);
            t.clips.push_back (clipOf (r, a, 0, 0, 24000));
            t.clips.push_back (clipOf (r, b, 24000, 0, 24000));
            t.clips.push_back (clipOf (r, a, 60000, 0, 24000));
            t.clips.push_back (clipOf (r, a, 62000, 0, 10000));
            auto& u = r.addTrack();
            u.clips.push_back (clipOf (r, a, 24000, 0, 24000));
            const auto ids = [&] (int track, int i) { return r.tracks[(size_t) track].clips[(size_t) i].id; };
            const auto before = r.tracks[0].clips[0].length;

            const auto noHandles = SessionEdits::crossfadeClips (r, ids (0, 0), ids (0, 1), 480);
            check (noHandles.contains ("no audio beyond") && SessionEdits::crossfadesOn (r, 0).empty() && r.tracks[0].clips[0].length == before,
                   "crossfades: two whole files that touch have no audio past where they meet, and are left as they were: \"" + noHandles + "\"");
            check (SessionEdits::crossfadeClips (r, ids (0, 1), ids (0, 2), 480).contains ("touch or overlap"),
                   "crossfades: clips with a gap between them are refused");
            check (SessionEdits::crossfadeClips (r, ids (0, 2), ids (0, 3), 480).contains ("whole of the other"),
                   "crossfades: a clip lying inside another is refused");
            check (SessionEdits::crossfadeClips (r, ids (0, 0), ids (1, 0), 480).contains ("one track"),
                   "crossfades: clips on two tracks are refused");
        }
    }

    void testLengthAndJoin()
    {
        auto h = halves (noise (48000, 5));
        auto& s = h.s;
        SessionEdits::crossfadeClips (s, h.out, h.in, 480);

        check (SessionEdits::setCrossfadeLength (s, h.out, h.in, 2000) && SessionEdits::crossfadeBetween (s, h.out, h.in)->start == 23000
                   && SessionEdits::crossfadeBetween (s, h.out, h.in)->end == 25000,
               "crossfades: lengthened to 2,000 frames about its join, 23,000 to 25,000");
        check (! SessionEdits::setCrossfadeLength (s, h.out, h.in, 2000) && ! SessionEdits::setCrossfadeLength (s, h.out, h.in, 1),
               "crossfades: the same length again, or under 2 frames, is no change");
        check (SessionEdits::setCrossfadeLength (s, h.out, h.in, 101) && SessionEdits::crossfadeBetween (s, h.out, h.in)->join() == 24000
                   && SessionEdits::crossfadeBetween (s, h.out, h.in)->length() == 101,
               "crossfades: an odd length keeps the join");

        check (SessionEdits::moveCrossfade (s, h.out, h.in, 30000) && SessionEdits::crossfadeBetween (s, h.out, h.in)->join() == 30000
                   && SessionEdits::crossfadeBetween (s, h.out, h.in)->length() == 101 && clip (s, h.in).getEnd() == 48000 && clip (s, h.out).position == 0,
               "crossfades: the join moved to 30,000, the length kept and the clips' outer ends where they were");
        check (worstAgainst (played (s, 0, 48000), *h.audio, 0) < 1.0e-6f, "crossfades: moved along one file, it still plays as the file");

        // The out clip's audio ends at 48,000 and the in clip must keep a frame: as far as they allow.
        check (SessionEdits::moveCrossfade (s, h.out, h.in, 60000) && SessionEdits::crossfadeBetween (s, h.out, h.in)->end == 47999,
               "crossfades: moved past where the clips allow, it stops a frame before the in clip's end");
        SessionEdits::moveCrossfade (s, h.out, h.in, 24000);

        // Limited by the shorter handle, on both sides alike.
        {
            SessionState l;
            l.sampleRate = rate;
            auto& t = l.addTrack();
            const auto a = noise (24100, 6), b = noise (30000, 7);
            t.clips.push_back (clipOf (l, a, 0, 0, 24000));          // 100 frames of handle after its end
            t.clips.push_back (clipOf (l, b, 24000, 5000, 24000));   // 5,000 before its start
            const auto out = t.clips[0].id, in = t.clips[1].id;
            check (SessionEdits::crossfadeClips (l, out, in, 4800).isEmpty() && SessionEdits::crossfadeBetween (l, out, in)->start == 23900
                       && SessionEdits::crossfadeBetween (l, out, in)->end == 24100,
                   "crossfades: asked for 4,800 frames with 100 of handle on one side, 200 frames centred on the join");
            check (! SessionEdits::setCrossfadeLength (l, out, in, 9600), "crossfades: no longer than the handles allow (no change)");
        }

        // Locked clips are left alone.
        s.tracks[0].clips[1].locked = true;
        check (! SessionEdits::setCrossfadeLength (s, h.out, h.in, 960) && ! SessionEdits::moveCrossfade (s, h.out, h.in, 25000)
                   && ! SessionEdits::setCrossfadeShapes (s, h.out, h.in, FadeShape::linear, FadeShape::linear) && ! SessionEdits::removeCrossfade (s, h.out, h.in),
               "crossfades: with a locked clip, nothing changes");
        s.tracks[0].clips[1].locked = false;

        // A clip's own fade in and fade out stay out of the crossfade's way.
        s.tracks[0].clips[0].fadeIn = 23950;
        check (SessionEdits::setCrossfadeLength (s, h.out, h.in, 2000) && SessionEdits::crossfadeBetween (s, h.out, h.in)->start == 23950
                   && clip (s, h.out).fadeIn == 23950,
               "crossfades: lengthened up to the out clip's own fade in, which is kept");
    }

    void testLevels()
    {
        // The gains themselves: a steady 1 on one side and silence on the other gives each side's.
        const auto sides = [] (FadeShape outShape, FadeShape inShape)
        {
            std::array<std::vector<float>, 2> gains;

            for (int side = 0; side < 2; ++side)
            {
                SessionState s;
                s.sampleRate = rate;
                auto& t = s.addTrack();
                const auto one = steady (10000, 1.0f), none = steady (10000, 0.0f);
                t.clips.push_back (clipOf (s, side == 0 ? one : none, 0, 0, 5000));
                t.clips.push_back (clipOf (s, side == 0 ? none : one, 5000, 5000, 5000));
                SessionEdits::crossfadeClips (s, t.clips[0].id, t.clips[1].id, 1000, outShape, inShape);
                gains[(size_t) side] = played (s, 4500, 1000);
            }

            return gains;
        };

        for (auto [shape, power, name] : { std::tuple (FadeShape::sCurve, false, "S-curve"), std::tuple (FadeShape::linear, false, "linear"),
                                           std::tuple (FadeShape::equalPower, true, "equal power") })
        {
            const auto g = sides (shape, shape);
            auto worst = 0.0f;

            for (size_t i = 0; i < 1000; ++i)
                worst = juce::jmax (worst, std::abs ((power ? g[0][i] * g[0][i] + g[1][i] * g[1][i] : g[0][i] + g[1][i]) - 1.0f));

            std::cout << "    " << name << " both sides: worst " << (power ? "power" : "gain") << " sum off 1 by " << worst << std::endl;
            check (worst < 1.0e-6f && g[0][0] > 0.99f && g[0][999] < 0.01f,
                   juce::String ("crossfades: ") + name + " both sides sums to 1 in " + (power ? "power" : "gain") + " at every frame");
        }

        // Two unrelated sounds, equal power: as loud across the crossfade as either side.
        SessionState s;
        s.sampleRate = rate;
        auto& t = s.addTrack();
        const auto a = noise (240000, 8), b = noise (240000, 9);
        t.clips.push_back (clipOf (s, a, 0, 0, 120000));
        t.clips.push_back (clipOf (s, b, 120000, 120000, 120000));
        SessionEdits::crossfadeClips (s, t.clips[0].id, t.clips[1].id, 96000, SessionEdits::equalPowerShape, SessionEdits::equalPowerShape);
        const auto heard = played (s, 0, 240000);
        const auto outside = (rmsDb (heard, 0, 72000) + rmsDb (heard, 168000, 240000)) / 2.0, across = rmsDb (heard, 72000, 168000);
        std::cout << "    two noises, equal power over 2 s: " << across - outside << " dB across the crossfade against either side" << std::endl;
        check (std::abs (across - outside) < 0.1, "crossfades: two unrelated noises, equal power, keep their level across the crossfade within 0.1 dB");
    }

    void testFilesAndView (AudioEngine& engine)
    {
        const auto dir = juce::File::createTempFile ("tqjoins");
        dir.createDirectory();
        const auto wav = dir.getChildFile ("audio/take.wav");
        wav.getParentDirectory().createDirectory();
        auto h = halves (noise (48000, 10));
        AudioFileIO::save (*h.audio, rate, wav, 32, engine.formatManager);

        for (auto& c : h.s.tracks[0].clips)
            c.source = wav;

        SessionEdits::crossfadeClips (h.s, h.out, h.in, 960, FadeShape::fast, FadeShape::slow);
        const auto file = dir.getChildFile ("song.tqsession");
        check (SessionFile::save (h.s, file).isEmpty(), "crossfades: a session with one saves");
        const auto text = file.loadFileAsString();
        const auto loaded = SessionFile::load (file, engine.formatManager);
        const auto back = SessionEdits::crossfadesOn (loaded.state, 0);
        check (loaded.error.isEmpty() && back.size() == 1 && back[0].start == 23520 && back[0].end == 24480
                   && clip (loaded.state, back[0].out).fadeOutShape == FadeShape::fast && clip (loaded.state, back[0].in).fadeInShape == FadeShape::slow,
               "crossfades: read back from the session file with their frames and shapes");
        check (! text.containsIgnoreCase ("crossfade"), "crossfades: the file holds only the clips' fades, nothing new");
        dir.deleteRecursively();

        // Crossfade Selected Clips in the multitrack view, as one undo step.
        auto twoJoins = halves (noise (48000, 11)).s;
        auto& t = twoJoins.tracks[0];
        SessionEdits::splitClip (twoJoins, t.clips[1].id, 36000);
        engine.session.reset (twoJoins, {});
        MultitrackView view (engine);
        view.selectAllClips();
        check (view.crossfadeSelectedClips (0.010).isEmpty() && SessionEdits::crossfadesOn (*engine.session.getState(), 0).size() == 2
                   && SessionEdits::crossfadesOn (*engine.session.getState(), 0)[1].length() == 480,
               "crossfades: Crossfade Selected Clips crossfades each join of three selected clips, 10 ms each");
        engine.session.undo();
        check (SessionEdits::crossfadesOn (*engine.session.getState(), 0).empty(), "crossfades: one undo takes both away");
        engine.session.reset (SessionState(), {});
        view.selectAllClips();
        check (view.crossfadeSelectedClips (0.010).contains ("Select two clips"), "crossfades: with nothing selected, it says what to select");
    }
}

void runCrossfadeTests (AudioEngine& engine)
{
    testCrossfadeClips();
    testLengthAndJoin();
    testLevels();
    testFilesAndView (engine);
}
