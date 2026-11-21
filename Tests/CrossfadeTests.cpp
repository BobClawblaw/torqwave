#include <JuceHeader.h>
#include "../Source/AudioEngine.h"
#include "../Source/AudioFileIO.h"
#include "../Source/CrossfadePanel.h"
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

namespace
{
    // The view takes the session's changes from a change message: let it arrive.
    void settle()
    {
        TestUtils::runUntil ([] { return false; }, 50);
    }

    juce::MouseEvent mouse (juce::Component& c, juce::Point<int> at, juce::Point<int> down, juce::ModifierKeys mods = {})
    {
        const auto now = juce::Time::getCurrentTime();
        return juce::MouseEvent (juce::Desktop::getInstance().getMainMouseSource(), at.toFloat(), mods, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, &c, &c,
                                 now, down.toFloat(), now, 1, at != down);
    }

    void dragOn (juce::Component& c, juce::Point<int> from, juce::Point<int> to)
    {
        c.mouseDown (mouse (c, from, from));
        c.mouseDrag (mouse (c, to, from));
        c.mouseUp (mouse (c, to, from));
        settle();
    }

    bool runItem (const juce::PopupMenu& menu, const juce::String& name)
    {
        for (juce::PopupMenu::MenuItemIterator i (menu, true); i.next();)
            if (i.getItem().text == name && i.getItem().action != nullptr)
            {
                i.getItem().action();
                settle();
                return true;
            }

        return false;
    }

    /** Crossfades on the lane: the area over the overlap, its edges dragged for the length and its
        middle for the join, the pointer over each, and its menu. */
    void testLane (AudioEngine& engine)
    {
        auto h = halves (noise (48000, 12));
        SessionEdits::crossfadeClips (h.s, h.out, h.in, 4800);
        engine.session.reset (h.s, {});
        MultitrackView view (engine);
        view.setBounds (0, 0, 1400, 500);
        view.setSnapping (false);
        view.zoomToRange ({ 18000, 30000 });
        const auto now = [&] { return *SessionEdits::crossfadeBetween (*engine.session.getState(), h.out, h.in); };
        const auto b = view.crossfadeBounds (now());
        const auto pixels = (double) b.getWidth() / 4800.0;   // per frame
        check (b.getWidth() > 40 && b.getHeight() > 20, "crossfade lane: a 100 ms crossfade shows across its overlap");

        view.mouseMove (mouse (view, { b.getRight(), b.getCentreY() }, { b.getRight(), b.getCentreY() }));
        const auto edgeCursor = view.getMouseCursor() == juce::MouseCursor (juce::MouseCursor::LeftRightResizeCursor);
        view.mouseMove (mouse (view, b.getCentre(), b.getCentre()));
        check (edgeCursor && view.getMouseCursor() == juce::MouseCursor (juce::MouseCursor::DraggingHandCursor),
               "crossfade lane: the pointer is a resize arrow over its edges and a hand over its middle");

        // The right edge 20 pixels further: both edges move out about the join.
        dragOn (view, { b.getRight(), b.getCentreY() }, { b.getRight() + 20, b.getCentreY() });
        const auto longer = now();
        const auto wider = view.crossfadeBounds (longer);
        check (longer.join() == 24000 && longer.length() > 4800 && std::abs (wider.getRight() - (b.getRight() + 20)) <= 1
                   && std::abs (wider.getX() - (b.getX() - 20)) <= 1 && engine.session.getUndoDescription() == "Crossfade Length",
               "crossfade lane: an edge dragged out lengthens it about its join, both edges moving, as the Crossfade Length step");

        // Its middle 30 pixels to the right: the join moves, the length stays.
        const auto c = wider.getCentre();
        dragOn (view, c, c.translated (30, 0));
        const auto moved = now();
        check (moved.length() == longer.length() && std::abs ((double) (moved.join() - 24000) * pixels - 30.0) <= 1.0
                   && engine.session.getUndoDescription() == "Move Crossfade",
               "crossfade lane: its middle dragged moves the join by as much, the length kept, as the Move Crossfade step");

        // An edge dragged onto the join: as short as the pixel there allows, never none.
        const auto still = view.crossfadeBounds (moved);
        const auto joinX = juce::roundToInt (still.getX() + (double) moved.length() / 2.0 * pixels);
        dragOn (view, { still.getRight(), still.getCentreY() }, { joinX, still.getCentreY() });
        const auto shortest = SessionEdits::crossfadeBetween (*engine.session.getState(), h.out, h.in);
        check (shortest.has_value() && shortest->length() >= 2 && (double) shortest->length() * pixels <= 3.0,
               "crossfade lane: an edge dragged onto the join leaves a crossfade as short as a pixel or two, never none");
        dragOn (view, { joinX, still.getCentreY() }, { still.getRight(), still.getCentreY() });

        // The menu: equal power, one side's shape, removing it.
        check (runItem (view.crossfadeMenu (now()), "Equal Power")
                   && engine.session.getState()->tracks[0].clips[0].fadeOutShape == FadeShape::equalPower
                   && engine.session.getState()->tracks[0].clips[1].fadeInShape == FadeShape::equalPower,
               "crossfade lane: its menu's Equal Power gives both sides that shape");
        auto menu = view.crossfadeMenu (now());
        juce::PopupMenu::MenuItemIterator items (menu);
        auto ticked = false;

        while (items.next())
            ticked = ticked || (items.getItem().text == "Equal Power" && items.getItem().isTicked);

        check (ticked && runItem (view.crossfadeMenu (now()), "Linear") && engine.session.getState()->tracks[0].clips[0].fadeOutShape == FadeShape::linear
                   && engine.session.getState()->tracks[0].clips[1].fadeInShape == FadeShape::equalPower,
               "crossfade lane: the menu ticks the pair in use, and its Out Side submenu changes that side alone");
        view.createComponentSnapshot (view.getLocalBounds());   // paints the crossfade's area
        check (runItem (view.crossfadeMenu (now()), "Remove Crossfade") && SessionEdits::crossfadesOn (*engine.session.getState(), 0).empty()
                   && engine.session.getUndoDescription() == "Remove Crossfade",
               "crossfade lane: Remove Crossfade leaves the clips meeting at its join");

        // Locked clips: the area is no handle, and a drag there moves nothing.
        engine.session.undo();
        auto locked = *engine.session.getState();
        locked.tracks[0].clips[0].locked = locked.tracks[0].clips[1].locked = true;
        engine.session.apply (locked, "Lock");
        const auto before = now();
        const auto l = view.crossfadeBounds (before);
        dragOn (view, { l.getRight(), l.getCentreY() }, { l.getRight() + 25, l.getCentreY() });
        check (now().start == before.start && now().end == before.end, "crossfade lane: with its clips locked, dragging its edge changes nothing");
        engine.session.reset (SessionState(), {});
    }
}

namespace
{
    /** The crossfade editor: its changes reach every crossfade it holds as one undo step, Cancel
        puts them back, presets, Symmetric and the matched pairs, what it says about the level, its
        auditions, and how it is opened. */
    void testEditor (AudioEngine& engine)
    {
        // Two joins on one track: three parts of one recording.
        auto h = halves (noise (96000, 13));
        auto& t = h.s.tracks[0];
        t.clips[1].length = 48000;
        t.clips.push_back (clipOf (h.s, h.audio, 72000, 72000, 24000));
        const auto third = t.clips[2].id;
        SessionEdits::crossfadeClips (h.s, h.out, h.in, 480);
        SessionEdits::crossfadeClips (h.s, h.in, third, 480);
        engine.session.reset (h.s, {});
        const auto both = SessionEdits::crossfadesOn (*engine.session.getState(), 0);
        const auto folder = juce::File::createTempFile ("tqxfpresets");
        EffectPresets::Store store (folder);
        const auto history = [&] { return engine.session.getHistory().size(); };

        {
            CrossfadePanel panel (engine, both, store);
            const auto start = history();
            const auto lengths = [&] { std::vector<juce::int64> v; for (auto& x : panel.getCrossfades()) v.push_back (x.length()); return v; };

            check (panel.setLengthMs (50.0) && lengths() == std::vector<juce::int64> { 2400, 2400 } && engine.session.getUndoDescription() == "Edit Crossfade",
                   "crossfade editor: a length reaches both crossfades it holds, as an Edit Crossfade step");
            panel.setLengthMs (100.0);
            panel.setSymmetric (true);
            panel.setOutShape (FadeShape::linear);
            const auto state = engine.session.getState();
            check (lengths() == std::vector<juce::int64> { 4800, 4800 } && history() == start + 1
                       && clip (*state, h.in).fadeInShape == FadeShape::linear && clip (*state, h.in).fadeOutShape == FadeShape::linear
                       && clip (*state, third).fadeInShape == FadeShape::linear,
                   "crossfade editor: with Symmetric, the out side's shape sets the in side too; every change stays one undo step");

            panel.setSymmetric (false);
            panel.setInShape (FadeShape::slow);
            check (clip (*engine.session.getState(), h.out).fadeOutShape == FadeShape::linear && clip (*engine.session.getState(), h.in).fadeInShape == FadeShape::slow,
                   "crossfade editor: without Symmetric, one side alone");

            // What it says about the level at the middle.
            panel.setShapes (SessionEdits::equalGainShape, SessionEdits::equalGainShape);
            const auto gain = panel.middleLevelsDb();
            panel.setShapes (SessionEdits::equalPowerShape, SessionEdits::equalPowerShape);
            const auto power = panel.middleLevelsDb();
            std::cout << "    at the middle, S-curve: " << gain.first << " dB in gain, " << gain.second << " in power; equal power: "
                      << power.first << " in gain, " << power.second << " in power" << std::endl;
            check (std::abs (gain.first) < 0.01f && std::abs (gain.second + 3.01f) < 0.05f && std::abs (power.second) < 0.01f && std::abs (power.first - 3.01f) < 0.05f
                       && panel.getSummary().contains ("in gain"),
                   "crossfade editor: S-curve both sides is 0 dB in gain at the middle and -3 in power; equal power the other way round");

            // Presets: a factory one, one of the user's.
            check (panel.applyValues (EffectPresets::factory (CrossfadePanel::presetKey)[2].values) && lengths() == std::vector<juce::int64> { 9600, 9600 }
                       && clip (*engine.session.getState(), h.out).fadeOutShape == FadeShape::equalPower,
                   "crossfade editor: the Music factory preset makes them 200 ms, equal power");
            panel.setLengthMs (25.0);
            store.save (CrossfadePanel::presetKey, "Mine", panel.currentValues());
            panel.setLengthMs (300.0);
            check (panel.applyValues (*store.load (CrossfadePanel::presetKey, "Mine")) && lengths() == std::vector<juce::int64> { 1200, 1200 },
                   "crossfade editor: a preset saved from it gives the same crossfade back");

            // Auditions: what they render is what the session plays there, of those clips alone.
            panel.setPreRollMs (100.0);
            panel.setPostRollMs (50.0);
            const auto bothSides = panel.renderAudition (CrossfadePanel::Side::both);
            const auto first = panel.getCrossfades().front();
            juce::AudioBuffer<float> played (2, bothSides.getNumSamples());
            SessionMixer::Buffers scratch;
            auto onlyTwo = *engine.session.getState();
            onlyTwo.tracks[0].clips.erase (std::remove_if (onlyTwo.tracks[0].clips.begin(), onlyTwo.tracks[0].clips.end(),
                                                           [&] (const SessionClip& c) { return c.id == third; }), onlyTwo.tracks[0].clips.end());
            SessionMixer::render (onlyTwo, first.start - 4800, played, 0, played.getNumSamples(), scratch);
            auto worst = 0.0f;

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < played.getNumSamples(); ++i)
                    worst = juce::jmax (worst, std::abs (played.getSample (ch, i) - bothSides.getSample (ch, i)));

            const auto outOnly = panel.renderAudition (CrossfadePanel::Side::out), inOnly = panel.renderAudition (CrossfadePanel::Side::in);
            auto summed = 0.0f;

            for (int i = 0; i < outOnly.getNumSamples(); ++i)
                summed = juce::jmax (summed, std::abs (outOnly.getSample (0, i) + inOnly.getSample (0, i) - bothSides.getSample (0, i)));

            check (bothSides.getNumSamples() == 4800 + 1200 + 2400 && worst == 0.0f && summed < 1.0e-6f
                       && outOnly.getSample (0, 4800 + 1200 + 100) == 0.0f && inOnly.getSample (0, 100) == 0.0f,
                   "crossfade editor: Play Crossfade renders the pre-roll, the crossfade and the post-roll as the session plays them, and Play Out and Play In each side alone");
            check (panel.audition (CrossfadePanel::Side::both), "crossfade editor: an audition goes to the player");
            engine.auditionFile ({});

            panel.setBounds (0, 0, CrossfadePanel::width, CrossfadePanel::height);
            panel.createComponentSnapshot (panel.getLocalBounds());   // paints the waves and curves

            // Cancel: everything back as it was when it opened, in one undo.
            auto closed = false;
            panel.onClose = [&] { closed = true; };
            panel.cancel();
            const auto back = SessionEdits::crossfadesOn (*engine.session.getState(), 0);
            check (closed && history() == start + 1 && engine.session.getPosition() == start - 1 && back.size() == 2 && back[0].length() == 480
                       && clip (*engine.session.getState(), h.out).fadeOutShape == FadeShape::sCurve,
                   "crossfade editor: Cancel puts the crossfades back as they were and closes it");
        }

        // Done keeps the changes; a second panel's changes are a step of their own.
        {
            CrossfadePanel panel (engine, { both[0] }, store);
            panel.setLengthMs (20.0);
            panel.done();
        }
        {
            CrossfadePanel panel (engine, { both[0] }, store);
            panel.setLengthMs (30.0);
            panel.done();
        }

        engine.session.undo();
        check (SessionEdits::crossfadesOn (*engine.session.getState(), 0)[0].length() == 960
                   && SessionEdits::crossfadesOn (*engine.session.getState(), 0)[1].length() == 480,
               "crossfade editor: Done keeps the change to the one crossfade it held, and two openings are two undo steps");
        folder.deleteRecursively();

        // Opening it: a double-click on a crossfade, Edit Crossfade... in its menu (with the other
        // selected crossfades), and the selection.
        MultitrackView view (engine);
        view.setBounds (0, 0, 1400, 500);
        view.setSnapping (false);
        view.zoomToRange ({ 18000, 30000 });
        settle();
        std::vector<SessionEdits::Crossfade> opened;
        juce::uint32 openedInPlace = 0;
        view.onEditCrossfades = [&] (std::vector<SessionEdits::Crossfade> c) { opened = std::move (c); };
        view.onEditClip = [&] (juce::uint32 id) { openedInPlace = id; };
        const auto now = SessionEdits::crossfadesOn (*engine.session.getState(), 0);
        const auto b = view.crossfadeBounds (now[0]);
        view.mouseDoubleClick (mouse (view, b.getCentre(), b.getCentre()));
        view.mouseDoubleClick (mouse (view, { b.getX() - 60, b.getCentreY() }, { b.getX() - 60, b.getCentreY() }));
        check (opened.size() == 1 && opened[0].out == h.out && openedInPlace == h.out,
               "crossfade editor: a double-click on a crossfade opens it, elsewhere on a clip the clip in place");

        view.selectAllClips();
        opened.clear();
        check (view.selectedCrossfades().size() == 2 && runItem (view.crossfadeMenu (now[0]), "Edit Crossfade...") && opened.size() == 2,
               "crossfade editor: Edit Crossfade... with the clips selected opens every selected crossfade");
        engine.session.reset (SessionState(), {});
    }
}

namespace
{
    // Clips laid over each other by hand, as in a session from use: two clips of one file on a track,
    // overlapping by 1.68 s with no fades.
    SessionState overlaid (juce::uint32& out, juce::uint32& in)
    {
        SessionState s;
        s.sampleRate = rate;
        const auto audio = noise (240000, 30);
        auto& t = s.addTrack();
        t.clips.push_back (clipOf (s, audio, 0, 16828, 52637));
        t.clips.push_back (clipOf (s, audio, 44563, 16828, 60619));
        out = t.clips[0].id;
        in = t.clips[1].id;
        return s;
    }

    void testOverlaid (AudioEngine& engine)
    {
        juce::uint32 out = 0, in = 0;
        const auto start = overlaid (out, in);
        const auto overlap = 52637 - 44563;

        // Crossfade Selected Clips keeps the whole overlap; clips that only touch still get 10 ms.
        engine.session.reset (start, {});
        MultitrackView view (engine);
        view.selectAllClips();
        check (view.crossfadeSelectedClips (0.010).isEmpty(), "overlaid clips: Crossfade Selected Clips crossfades them");
        auto made = SessionEdits::crossfadesOn (*engine.session.getState(), 0);
        check (made.size() == 1 && made[0].start == 44563 && made[0].end == 52637 && made[0].length() == overlap
                   && clip (*engine.session.getState(), out).getEnd() == 52637 && clip (*engine.session.getState(), in).position == 44563,
               "overlaid clips: the crossfade is the whole overlap, the clips where they were");

        // A double-click on the overlap makes it a crossfade and opens it in the editor.
        engine.session.reset (start, {});
        view.setBounds (0, 0, 1400, 500);
        view.setSnapping (false);
        view.zoomToRange ({ 30000, 70000 });
        settle();
        std::vector<SessionEdits::Crossfade> opened;
        juce::uint32 openedInPlace = 0;
        view.onEditCrossfades = [&] (std::vector<SessionEdits::Crossfade> c) { opened = std::move (c); };
        view.onEditClip = [&] (juce::uint32 id) { openedInPlace = id; };
        // Where frames lie on the lane, from the area a crossfade over them would take.
        const auto at = [&] (juce::int64 from, juce::int64 to) { return view.crossfadeBounds ({ out, in, 0, from, to }); };
        const auto mid = at (44563, 52637).getCentre();
        view.mouseDoubleClick (mouse (view, mid, mid));
        made = SessionEdits::crossfadesOn (*engine.session.getState(), 0);
        check (made.size() == 1 && made[0].length() == overlap && opened.size() == 1 && opened[0].out == out && opened[0].in == in && openedInPlace == 0
                   && engine.session.getUndoDescription() == "Crossfade Clips" && view.isClipSelected (out) && view.isClipSelected (in),
               "overlaid clips: a double-click on their overlap makes it a crossfade (one undo step), selects both and opens the editor, not the clip in place");

        // Then the same double-click opens the crossfade it now is; elsewhere on a clip, the clip in place.
        settle();
        opened.clear();
        view.mouseDoubleClick (mouse (view, mid, mid));
        const juce::Point<int> clear { at (35000, 35000).getX(), mid.y };
        view.mouseDoubleClick (mouse (view, clear, clear));
        check (opened.size() == 1 && SessionEdits::crossfadesOn (*engine.session.getState(), 0).size() == 1 && openedInPlace == out,
               "overlaid clips: once crossfaded, a double-click opens the crossfade; off the overlap, the clip in place");

        // From the menu's Crossfade Clips: the same, without the editor.
        engine.session.reset (start, {});
        opened.clear();
        check (view.crossfadeOverlap (out, in, false).isEmpty() && opened.empty()
                   && SessionEdits::crossfadesOn (*engine.session.getState(), 0)[0].length() == overlap,
               "overlaid clips: Crossfade Clips makes the whole overlap a crossfade without opening the editor");

        // No clicks: two clips of a 220 Hz sine laid over each other out of phase, as in the session
        // it was found in, click where one starts and where the other stops; crossfaded, the sound's
        // second difference stays within 1.5 times the sine's own (equal power on two copies of one
        // sound adds up to as much as 3 dB louder), mixed or layered.
        {
            juce::AudioBuffer<float> sine (1, 240000);

            for (int i = 0; i < sine.getNumSamples(); ++i)
                sine.setSample (0, i, 0.5f * std::sin (juce::MathConstants<float>::twoPi * 220.0f * (float) i / (float) rate));

            const auto audio = snapshotOf (std::move (sine));
            const auto worstStep = [] (const SessionState& s)
            {
                juce::AudioBuffer<float> b (2, 30000);
                SessionMixer::Buffers scratch;

                for (int at = 0; at < b.getNumSamples(); at += 512)
                    SessionMixer::render (s, 35000 + at, b, at, juce::jmin (512, b.getNumSamples() - at), scratch);

                auto worst = 0.0f;

                for (int i = 2; i < b.getNumSamples(); ++i)
                    worst = juce::jmax (worst, std::abs (b.getSample (0, i) - 2 * b.getSample (0, i - 1) + b.getSample (0, i - 2)));

                return worst / std::sqrt (0.5f);
            };
            const auto ownStep = 0.5f * std::pow (juce::MathConstants<float>::twoPi * 220.0f / (float) rate, 2.0f);
            auto laid = start;

            for (auto& c : laid.tracks[0].clips)
                c.audio = audio;

            laid.tracks[0].clips[1].offset = 16828 + 101;   // out of phase with the clip going out
            auto worstLaid = worstStep (laid), worstMade = 0.0f;

            for (auto layered : { false, true })
                for (auto shape : { SessionEdits::equalGainShape, SessionEdits::equalPowerShape })
                {
                    auto s = laid;
                    s.layerClips = layered;
                    SessionEdits::crossfadeClips (s, out, in, overlap, shape, shape);
                    worstMade = juce::jmax (worstMade, worstStep (s));
                }

            std::cout << "    sine laid over itself out of phase: worst step " << worstLaid / ownStep << "x the sine's own; crossfaded, " << worstMade / ownStep << "x" << std::endl;
            check (worstLaid > 20 * ownStep && worstMade < 1.5f * ownStep,
                   "overlaid clips: laid over each other they click; crossfaded (equal gain or power, mixed or layered) there is no click, in 512-frame blocks");
        }

        // One clip over the whole of the other is no crossfade; it says so.
        auto covered = start;
        covered.tracks[0].clips[1].length = 2000;
        engine.session.reset (covered, {});
        check (view.crossfadeOverlap (out, in, false).isNotEmpty() && SessionEdits::crossfadesOn (*engine.session.getState(), 0).empty(),
               "overlaid clips: one clip inside the other is refused with a reason");
        engine.session.reset (SessionState(), {});
    }
}

void runCrossfadeTests (AudioEngine& engine)
{
    testCrossfadeClips();
    testLengthAndJoin();
    testLevels();
    testFilesAndView (engine);
    testLane (engine);
    testEditor (engine);
    testOverlaid (engine);
}
