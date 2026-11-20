// Exact values are what the session holds.
#pragma clang diagnostic ignored "-Wfloat-equal"

#include <JuceHeader.h>
#include "../Source/AudioEdits.h"
#include "../Source/Session.h"
#include "../Source/SessionMixer.h"
#include "../Source/TempoFollow.h"
#include "TestUtils.h"
#include "TempoFollowTests.h"

namespace
{
    using TestUtils::check;

    constexpr double rate = 48000.0;

    /** A click (a 1 ms burst at 2 kHz, then silence) on each of `beats` beats at `bpm`, the beats'
        frames moved by `late` where given; stereo. */
    juce::AudioBuffer<float> clicks (double bpm, int beats, std::map<int, int> late = {})
    {
        const auto beat = 60.0 * rate / bpm;
        juce::AudioBuffer<float> b (2, (int) std::ceil (beat * beats));
        b.clear();

        for (int k = 0; k < beats; ++k)
        {
            const auto at = (int) std::llround (beat * k) + (late.count (k) > 0 ? late[k] : 0);

            for (int i = 0; i < 48 && at + i < b.getNumSamples(); ++i)
                for (int ch = 0; ch < 2; ++ch)
                    b.setSample (ch, at + i, 0.9f * (float) std::sin (juce::MathConstants<double>::twoPi * 2000.0 * i / rate));
        }

        return b;
    }

    /** Where each click starts in the audio: the first frame over half the loudest, then nothing
        for 200 ms (a stretched click's tail rings on below it). */
    std::vector<juce::int64> clicksIn (const juce::AudioBuffer<float>& b, juce::int64 startFrame = 0)
    {
        const auto peak = b.getMagnitude (0, 0, b.getNumSamples());
        std::vector<juce::int64> found;
        int next = 0;

        for (int i = 0; i < b.getNumSamples(); ++i)
            if (i >= next && std::abs (b.getSample (0, i)) > 0.5f * peak)
            {
                found.push_back (startFrame + i);
                next = i + 9600;
            }

        return found;
    }

    /** The clips' audio played from frame 0 to `frames`, through the mixer, in blocks of `block`. */
    juce::AudioBuffer<float> play (const SessionState& s, int frames, int block = 512)
    {
        SessionMixer::Buffers buffers;
        buffers.reserve (block);
        juce::AudioBuffer<float> out (2, frames);

        for (int at = 0; at < frames; at += block)
            SessionMixer::render (s, at, out, at, juce::jmin (block, frames - at), buffers);

        return out;
    }

    /** The worst distance (ms) of the clicks from the frames given, or a large number when they
        don't pair up. */
    double worst (const std::vector<juce::int64>& got, const std::vector<juce::int64>& want)
    {
        if (got.size() != want.size())
            return 1.0e9;

        double w = 0.0;

        for (size_t i = 0; i < got.size(); ++i)
            w = juce::jmax (w, std::abs ((double) (got[i] - want[i])) * 1000.0 / rate);

        return w;
    }

    SessionState session (double tempo, juce::AudioBuffer<float> audio, const juce::File& file = juce::File ("/takes/clicks.wav"))
    {
        SessionState s;
        s.sampleRate = rate;
        s.tempo = tempo;
        s.addTrack();
        SessionEdits::addClip (s, 0, file, AudioEdits::fromBuffer (std::move (audio)), 0);
        return s;
    }

    const SessionClip& only (const SessionState& s)
    {
        return s.tracks[0].clips[0];
    }

    std::vector<juce::int64> beatsFrom (const SessionState& s, juce::int64 first, int count)
    {
        std::vector<juce::int64> v;
        const auto map = s.tempoMap();

        for (int k = 0; k < count; ++k)
            v.push_back (map.beatFrame (first + k));

        return v;
    }

    //==============================================================================
    void testOnTheGrid()
    {
        // A click track at 100 BPM in a session at 120.
        auto s = session (120.0, clicks (100.0, 8));
        const auto id = only (s).id;
        const auto before = only (s).length;
        const auto set = SessionEdits::setClipTempo (s, id, 100.0);
        const auto stale = TempoFollow::isStale (s, only (s));
        const auto rendered = TempoFollow::renderAll (s);
        const auto& c = only (s);
        const auto heard = clicksIn (play (s, (int) c.length + 4800));
        const auto miss = worst (heard, beatsFrom (s, 0, 8));
        check (set && stale && rendered && ! TempoFollow::isStale (s, c) && s.tracks[0].musical && c.length == std::llround ((double) before * 100.0 / 120.0)
                   && c.audio->getNumFrames() == c.length && miss < 1.0,
               "tempo follow: a 100 BPM click track in a 120 BPM session lands on the session's beats, each within "
                   + juce::String (miss, 3) + " ms (under 1), and is as many beats long");

        // Played in blocks of any size, the same samples: playback and mixdowns read the stretched audio alike.
        const auto a = play (s, (int) c.length, 512), b = play (s, (int) c.length, 333);
        auto same = true;

        for (int ch = 0; ch < 2; ++ch)
            same = same && std::memcmp (a.getReadPointer (ch), b.getReadPointer (ch), sizeof (float) * (size_t) a.getNumSamples()) == 0;

        check (same, "tempo follow: playback reads the stretched audio as any clip, sample for sample at any block size");

        // The session's tempo changed: it keeps its beats, and is stretched again.
        SessionEdits::setTempo (s, 90.0, 4);
        const auto staleAgain = TempoFollow::isStale (s, only (s));
        TempoFollow::renderAll (s);
        const auto slower = worst (clicksIn (play (s, (int) only (s).length + 4800)), beatsFrom (s, 0, 8));
        check (staleAgain && slower < 1.0 && std::abs ((double) only (s).length - 8.0 * 60.0 * rate / 90.0) < 2.0,
               "tempo follow: the session at 90 BPM, it is stretched again, eight beats long, its clicks within " + juce::String (slower, 3) + " ms");

        // Back at its own speed: the original, the same part of it.
        const auto original = only (s).tempoFollow.original;
        SessionEdits::setClipTempo (s, id, 0.0);
        check (only (s).audio == original && only (s).offset == 0 && only (s).length == original->getNumFrames() && ! only (s).tempoFollow.follows(),
               "tempo follow: turned off, the clip plays its original at its own speed, all of it as before");
    }

    void testTempoChange()
    {
        // 120 BPM for the first bar, 90 from the second: the clicks follow both.
        auto s = session (120.0, clicks (100.0, 12));
        SessionEdits::setTempoChange (s, { 1, 90.0, 4 });
        SessionEdits::setClipTempo (s, only (s).id, 100.0);
        TempoFollow::renderAll (s);
        const auto miss = worst (clicksIn (play (s, (int) only (s).length + 4800)), beatsFrom (s, 0, 12));
        check (miss < 1.0, "tempo follow: through a tempo change inside the clip (120, then 90 from bar 2), every click within "
                               + juce::String (miss, 3) + " ms of its beat");

        // Moved by a bar it plays the same part, from its new beat (bar 2's tempo throughout, so stretched anew).
        const auto bar = s.tempoMap().barFrame (1);
        auto moved = s;
        SessionEdits::moveClip (moved, only (moved).id, 0, bar);
        TempoFollow::renderAll (moved);
        const auto later = worst (clicksIn (play (moved, (int) (bar + only (moved).length + 4800))), beatsFrom (moved, 4, 12));
        check (later < 1.0, "tempo follow: moved to bar 2, it is stretched for the tempo there, every click within " + juce::String (later, 3) + " ms");
    }

    void testWarps()
    {
        // The fourth click played 30 ms late: a marker pins it to its beat, and markers on the
        // clicks either side keep the rest on theirs (between markers the audio is stretched in
        // proportion; past the last it plays at its own tempo).
        auto s = session (120.0, clicks (120.0, 8, { { 3, 1440 } }));
        const auto id = only (s).id;
        SessionEdits::setClipTempo (s, id, 120.0);
        const auto lateFrame = (juce::int64) std::llround (3 * 24000.0) + 1440;
        const auto bad = SessionEdits::setWarpMarkers (s, id, { { 0, 1.0 } }) || SessionEdits::setWarpMarkers (s, id, { { 1000, 2.0 }, { 900, 3.0 } });
        const auto set = SessionEdits::setWarpMarkers (s, id, { { 2 * 24000, 2.0 }, { lateFrame, 3.0 }, { 4 * 24000, 4.0 } });
        TempoFollow::renderAll (s);
        const auto heard = clicksIn (play (s, (int) only (s).length + 4800));
        const auto miss = worst (heard, beatsFrom (s, 0, 8));
        check (! bad && set && miss < 1.0 && heard.size() == 8 && std::abs ((double) (heard[3] - 3 * 24000)) * 1000.0 / rate < 1.0,
               "tempo follow: a warp marker pins a late click to its beat, and the clicks either side stay on theirs, each within "
                   + juce::String (miss, 3) + " ms; markers out of order are refused");

        // Markers in the stretched clip's span change its length, and keep the part it plays.
        const auto span = TempoFollow::spanOf (s, only (s));
        check (std::abs (span.start) < 0.5 && std::abs (span.end - (double) only (s).tempoFollow.original->getNumFrames()) < 2.0,
               "tempo follow: with markers, the clip still plays all of its original");
    }

    void testFile()
    {
        const auto dir = juce::File::createTempFile ("tqtempo");
        dir.createDirectory();
        const auto wav = dir.getChildFile ("loop 100bpm.wav");
        const auto audio = clicks (100.0, 8, { { 5, 960 } });

        {
            juce::WavAudioFormat format;
            std::unique_ptr<juce::OutputStream> out (wav.createOutputStream());
            auto writer = format.createWriterFor (out, juce::AudioFormatWriterOptions().withSampleRate (rate).withNumChannels (2).withBitsPerSample (32)
                                                           .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint));
            writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
        }

        auto s = session (120.0, audio, wav);
        const auto id = only (s).id;
        SessionEdits::setClipTempo (s, id, 100.0);
        SessionEdits::setWarpMarkers (s, id, { { (juce::int64) std::llround (5 * 28800.0) + 960, 5.0 } });
        SessionEdits::moveClip (s, id, 0, 24000);
        TempoFollow::renderAll (s);

        const auto file = dir.getChildFile ("session.tqs");
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        const auto error = SessionFile::save (s, file);
        auto loaded = SessionFile::load (file, formats);
        const auto& c = loaded.state.tracks.empty() || loaded.state.tracks[0].clips.empty() ? only (s) : loaded.state.tracks[0].clips[0];
        const auto came = error.isEmpty() && loaded.error.isEmpty() && c.tempoFollow.bpm == 100.0 && c.tempoFollow.warps == only (s).tempoFollow.warps
                          && c.position == only (s).position && c.offset == only (s).offset && c.length == only (s).length && TempoFollow::isStale (loaded.state, c);
        TempoFollow::renderAll (loaded.state);
        const auto& again = loaded.state.tracks[0].clips[0];
        auto same = again.audio->getNumFrames() == only (s).audio->getNumFrames();
        juce::AudioBuffer<float> x (2, (int) again.audio->getNumFrames()), y (2, (int) again.audio->getNumFrames());
        again.audio->read (x, 0, 0, x.getNumSamples());
        only (s).audio->read (y, 0, 0, y.getNumSamples());

        for (int ch = 0; ch < 2 && same; ++ch)
            same = std::memcmp (x.getReadPointer (ch), y.getReadPointer (ch), sizeof (float) * (size_t) x.getNumSamples()) == 0;

        check (came && same, "tempo follow: a clip's tempo, markers and place come back from a session file, and it is stretched again to the same samples");
        dir.deleteRecursively();
    }

    void testDetection()
    {
        using TempoFollow::tempoFromName;
        check (tempoFromName ("loop 120bpm") == 120.0 && tempoFromName ("drums_92_BPM") == 92.0 && tempoFromName ("beat-128-Am") == 128.0
                   && tempoFromName ("Kick 808") == std::nullopt && tempoFromName ("take 3") == std::nullopt && tempoFromName ("bass 2024 mix 95") == 95.0
                   && tempoFromName ("140 BPM loop 2") == 140.0,
               "tempo follow: a tempo is read from a file's name (\"loop 120bpm\", \"drums_92_BPM\", \"beat-128-Am\"), and not from other numbers");

        const auto a = AudioEdits::fromBuffer (clicks (100.0, 16));
        const auto b = AudioEdits::fromBuffer (clicks (132.0, 16));
        juce::AudioBuffer<float> tone (2, 96000);

        for (int i = 0; i < tone.getNumSamples(); ++i)
            for (int ch = 0; ch < 2; ++ch)
                tone.setSample (ch, i, 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * 440.0 * i / rate));

        const auto steady = AudioEdits::fromBuffer (std::move (tone));
        const auto ta = TempoFollow::detectTempo (*a, 0, a->getNumFrames(), rate);
        const auto tb = TempoFollow::detectTempo (*b, 0, b->getNumFrames(), rate);
        check (ta.has_value() && std::abs (*ta - 100.0) <= 0.5 && tb.has_value() && std::abs (*tb - 132.0) <= 0.5
                   && ! TempoFollow::detectTempo (*steady, 0, steady->getNumFrames(), rate).has_value(),
               "tempo follow: a click track's tempo is detected (" + juce::String (ta.value_or (0.0), 1) + " for 100, "
                   + juce::String (tb.value_or (0.0), 1) + " for 132); a steady tone has none");

        // Slicing at the attacks: a clip for each click.
        auto s = session (120.0, clicks (120.0, 8));
        const auto attacks = TempoFollow::transients (*only (s).audio, 0, only (s).audio->getNumFrames(), rate);
        const auto made = SessionEdits::sliceClip (s, only (s).id, attacks);
        std::vector<juce::int64> starts;

        for (auto& c : s.tracks[0].clips)
            starts.push_back (c.position);

        std::sort (starts.begin(), starts.end());
        auto near = starts.size() == 8;

        for (size_t k = 0; near && k < starts.size(); ++k)
            near = std::abs ((double) (starts[k] - (juce::int64) (k * 24000))) < 0.005 * rate;

        check (made == 7 && near, "tempo follow: Slice at Transients makes a clip for each click, each starting within 5 ms of it");
    }

    void testRenderer()
    {
        Session live;
        auto s = session (120.0, clicks (100.0, 8));
        SessionEdits::setClipTempo (s, only (s).id, 100.0);
        live.reset (s, {});
        TempoFollow::Renderer renderer (live);
        auto doneCalls = 0;
        renderer.whenDone ([&doneCalls] { ++doneCalls; });
        const auto busy = renderer.isBusy();
        const auto finished = TestUtils::runUntil ([&] { return renderer.isUpToDate(); }, 20000);
        TestUtils::runUntil ([&] { return doneCalls > 0; }, 1000);
        const auto first = only (*live.getState()).audio;
        check (busy && finished && doneCalls == 1 && ! renderer.isBusy() && ! live.canUndo() && ! live.isDirty()
                   && first->getNumFrames() == only (*live.getState()).length,
               "tempo follow: the renderer stretches a stale clip on its own thread and puts it in place, not as an undo step or a change to save, then tells who waited");

        // A tempo edit: stretched again; undone, the earlier stretch is back at once.
        auto next = *live.getState();
        SessionEdits::setTempo (next, 140.0, 4);
        live.apply (next, "Tempo");
        const auto again = TestUtils::runUntil ([&] { return renderer.isUpToDate(); }, 20000);
        const auto second = only (*live.getState()).audio;
        live.undo();
        const auto back = TestUtils::runUntil ([&] { return renderer.isUpToDate(); }, 200);
        check (again && second != first && back && only (*live.getState()).audio == first && live.getUndoDescription().isEmpty(),
               "tempo follow: a tempo change is stretched again in the background; undone, the earlier stretch is back at once");

        // An audio edit on a following clip: the new audio plays at its own speed.
        auto edited = *live.getState();
        const auto& c = only (edited);
        juce::AudioBuffer<float> reversed (2, (int) c.length);
        reversed.clear();
        const auto reversedOk = SessionEdits::reverseClip (edited, c.id, juce::File ("/takes/rev.wav"), AudioEdits::fromBuffer (std::move (reversed)), {});
        check (reversedOk && ! only (edited).tempoFollow.follows(), "tempo follow: reversing a following clip keeps what was heard, at its own speed");
    }
}

void runTempoFollowTests()
{
    std::cout << "Tempo follow:" << std::endl;
    testOnTheGrid();
    testTempoChange();
    testWarps();
    testFile();
    testDetection();
    testRenderer();
}
