#include <JuceHeader.h>
#include "../Source/AiSession.h"
#include "../Source/AudioEdits.h"
#include "../Source/AudioEngine.h"
#include "../Source/AudioFileIO.h"
#include "../Source/FitTempoPanel.h"
#include "../Source/MultitrackView.h"
#include "../Source/Picture.h"
#include "TestUtils.h"
#include "PictureTests.h"

namespace
{
    using TestUtils::check;
    using Kind = TimeFormat::Kind;

    constexpr double rate = 48000.0;
    const std::vector<Kind> timecodeKinds { Kind::smpte23976, Kind::smpte24, Kind::smpte25, Kind::smpte2997Drop, Kind::smpte2997, Kind::smpte30 };

    juce::String nameOf (Kind k)       { return TimeFormat::names()[(int) k]; }

    juce::int64 counted (const juce::String& text, Kind k)
    {
        return TimeFormat::parseCounted (text, k).value_or (-1);
    }

    /** Runs a program to the end; true when it exits cleanly. */
    bool runTool (const juce::StringArray& command)
    {
        juce::ChildProcess p;
        return p.start (command) && p.readAllProcessOutput().length() >= 0 && p.getExitCode() == 0;
    }

    juce::MouseEvent clickAt (juce::Component& c, juce::Point<float> at)
    {
        const auto now = juce::Time::getCurrentTime();
        return { juce::Desktop::getInstance().getMainMouseSource(), at, juce::ModifierKeys (juce::ModifierKeys::leftButtonModifier),
                 juce::MouseInputSource::defaultPressure, juce::MouseInputSource::defaultOrientation, juce::MouseInputSource::defaultRotation,
                 juce::MouseInputSource::defaultTiltX, juce::MouseInputSource::defaultTiltY, &c, &c, now, at, now, 1, false };
    }

    //==============================================================================
    void testTimecode()
    {
        // Counted frames to text and back, through a whole day at every rate (drop frame's
        // counted numbers from real frames, so none of the dropped ones is asked for).
        int trips = 0, tried = 0;

        for (auto k : timecodeKinds)
        {
            const auto day = (juce::int64) 24 * 3600 * TimeFormat::countedFps (k);
            const auto realDay = TimeFormat::isDropFrame (k) ? TimeFormat::fromDropFrameCount (day) : day;

            for (juce::int64 real = 0; real < realDay; real += 997)
            {
                const auto c = TimeFormat::isDropFrame (k) ? TimeFormat::dropFrameCount (real) : real;
                ++tried;
                trips += TimeFormat::parseCounted (TimeFormat::countedText (c, k), k) == c ? 1 : 0;
            }
        }

        check (tried > 10000 && trips == tried, "timecode: frames counted at each rate (23.976, 24, 25, 29.97 drop and non-drop, 30) read back from their text, through a day ("
                                                    + juce::String (trips) + " of " + juce::String (tried) + ")");
        check (! TimeFormat::parseCounted ("00:01:00;00", Kind::smpte2997Drop).has_value() && TimeFormat::parseCounted ("00:10:00;00", Kind::smpte2997Drop).has_value()
                   && ! TimeFormat::parseCounted ("00:00:00:24", Kind::smpte23976).has_value() && counted ("01:00:00:00", Kind::smpte23976) == 86400,
               "timecode: dropped frame numbers and frames past the rate's count are refused; 23.976 counts 24 a second");

        // Session times with a start: each time's text names the first sample of the frame it is in.
        juce::Random random (19);
        int sessionTrips = 0, sessionTried = 0;
        juce::String firstMiss;

        for (auto k : timecodeKinds)
            for (auto startText : { "00:00:00:00", "01:00:00:00", "23:59:58:00" })
            {
                const TimeFormat::Timecode tc { k, counted (startText, k) };

                for (int i = 0; i < 400; ++i)
                {
                    const auto frame = (juce::int64) (random.nextDouble() * rate * 7200.0);
                    const auto text = TimeFormat::format (frame, rate, tc);
                    const auto back = TimeFormat::parse (text, rate, tc);
                    const auto ok = back.has_value() && *back <= frame && TimeFormat::format (*back, rate, tc) == text
                                    && (*back == 0 || TimeFormat::format (*back - 1, rate, tc) != text);
                    ++sessionTried;
                    sessionTrips += ok ? 1 : 0;

                    if (! ok && firstMiss.isEmpty())
                        firstMiss = "; " + nameOf (k) + " from " + startText + " at " + juce::String (frame) + " missed";
                }
            }

        check (sessionTrips == sessionTried, "timecode: a session time's timecode, from starts of 00:00:00:00, 01:00:00:00 and 23:59:58:00, reads back to the first sample of its frame at every rate ("
                                                 + juce::String (sessionTrips) + " of " + juce::String (sessionTried) + firstMiss + ")");

        const TimeFormat::Timecode hourDrop { Kind::smpte2997Drop, counted ("01:00:00;00", Kind::smpte2997Drop) };
        const TimeFormat::Timecode late25 { Kind::smpte25, counted ("23:59:58:00", Kind::smpte25) };
        const auto hour = (juce::int64) (3600.0 * rate);
        check (TimeFormat::format (hour, rate, hourDrop) == "02:00:00;00" && TimeFormat::format (60 * (juce::int64) rate, rate, hourDrop) == "01:00:59;28"
                   && TimeFormat::format (hour, rate, TimeFormat::Timecode { Kind::smpte23976, 0 }) == "00:59:56:09"
                   && TimeFormat::format (3 * (juce::int64) rate, rate, late25) == "00:00:01:00"
                   && TimeFormat::format (hour, rate, hourDrop.withoutStart()) == "01:00:00;00"
                   && TimeFormat::parse ("00:00:01:00", rate, late25) == 3 * (juce::int64) rate,
               "timecode: an hour from 01:00:00;00 at 29.97 drop frame is 02:00:00;00; 23.976 runs 3.6 s behind the clock in an hour; times run on through midnight");
        check (TimeFormat::rulerLabel (10.0, 1.0, rate, TimeFormat::Timecode { Kind::smpte25, 90000 }) == "01:00:10:00"
                   && TimeFormat::rulerLabel (10.0, 1.0, rate, TimeFormat::Timecode { Kind::smpte25, 0 }) == "00:10:00"
                   && TimeFormat::format (480000, rate, TimeFormat::Timecode()) == TimeFormat::format (480000, rate),
               "timecode: the ruler counts from the start; a session without a timecode shows the app's format");

        check (TimeFormat::kindForFrameRate (23.98, false) == Kind::smpte23976 && TimeFormat::kindForFrameRate (24.0, false) == Kind::smpte24
                   && TimeFormat::kindForFrameRate (25.0, true) == Kind::smpte25 && TimeFormat::kindForFrameRate (29.97, true) == Kind::smpte2997Drop
                   && TimeFormat::kindForFrameRate (29.97, false) == Kind::smpte2997 && TimeFormat::kindForFrameRate (59.94, true) == Kind::smpte2997Drop
                   && TimeFormat::kindForFrameRate (50.0, false) == Kind::smpte25 && TimeFormat::kindForFrameRate (30.0, false) == Kind::smpte30
                   && ! TimeFormat::kindForFrameRate (12.0, false).has_value() && ! TimeFormat::kindForFrameRate (27.0, false).has_value(),
               "timecode: a picture's frame rate gives the rate timecode counts it at (23.98, 29.97 drop, 59.94 and 50 included); 12 and 27 fps give none");
        check (TimeFormat::fromKey (TimeFormat::key (Kind::smpte23976)) == Kind::smpte23976 && TimeFormat::names()[(int) Kind::smpte23976] == "SMPTE 23.976 fps"
                   && juce::approximatelyEqual (TimeFormat::framesPerSecond (Kind::smpte23976), 24000.0 / 1001.0),
               "timecode: 23.976 is a time format of its own, kept in the settings by its key");
    }

    //==============================================================================
    void testSessionFile (AudioEngine& engine)
    {
        juce::AudioFormatManager formats;
        AudioFileIO::registerFormats (formats);
        juce::TemporaryFile folderTemp;
        const auto folder = folderTemp.getFile();
        folder.createDirectory();

        SessionState s;
        s.sampleRate = rate;
        s.addTrack();
        SessionEdits::setTimecode (s, { Kind::smpte2997Drop, counted ("01:00:00;00", Kind::smpte2997Drop) });
        SessionEdits::addHitMarker (s, 96000);
        SessionEdits::addHitMarker (s, 48000);
        const auto file = folder.getChildFile ("picture.tqsession");
        const auto saved = SessionFile::save (s, file);
        const auto loaded = SessionFile::load (file, formats);
        check (saved.isEmpty() && loaded.error.isEmpty() && loaded.state.timecode == s.timecode && file.loadFileAsString().contains ("01:00:00;00")
                   && loaded.state.markers.size() == 2 && loaded.state.markers[0].name == "Hit 02" && loaded.state.markers[1].name == "Hit 01",
               "session file: the timecode (29.97 drop frame from 01:00:00;00) and hit markers come back");

        SessionState plain;
        plain.sampleRate = rate;
        plain.addTrack();
        const auto older = folder.getChildFile ("older.tqsession");
        SessionFile::save (plain, older);
        const auto old = SessionFile::load (older, formats);
        check (! older.loadFileAsString().contains ("TIMECODE") && old.error.isEmpty() && ! old.state.timecode.isSet()
                   && TimeFormat::format (480000, rate, old.state.timecode) == TimeFormat::format (480000, rate),
               "session file: a session without a timecode writes none and opens showing the app's time format, as before");

        // The timecode is an undo step.
        engine.session.reset (plain, {});
        auto next = *engine.session.getState();
        const auto set = SessionEdits::setTimecode (next, { Kind::smpte25, 90000 });
        const auto again = SessionEdits::setTimecode (next, { Kind::smpte25, 90000 });
        engine.session.apply (std::move (next), "Session Timecode");
        const auto applied = engine.session.getState()->timecode == TimeFormat::Timecode { Kind::smpte25, 90000 };
        engine.session.undo();
        check (set && ! again && applied && ! engine.session.getState()->timecode.isSet(), "session timecode: set as one undo step, undone back to none");
    }

    //==============================================================================
    void testCuts (const juce::File& ffmpeg)
    {
        juce::TemporaryFile folderTemp;
        const auto folder = folderTemp.getFile();
        folder.createDirectory();

        // Three shots at 25 fps: a moving test picture for 2 s, colour bars for 1.6 s, then
        // another moving test picture: cuts at 2.0 and 3.6 s. The file's own timecode starts at 01:00:00:00.
        const auto video = folder.getChildFile ("Shots.mov");
        runTool ({ ffmpeg.getFullPathName(), "-nostdin", "-v", "error", "-y",
                   "-f", "lavfi", "-i", "testsrc2=size=320x240:rate=25:duration=2",
                   "-f", "lavfi", "-i", "smptebars=size=320x240:rate=25:duration=1.6",
                   "-f", "lavfi", "-i", "testsrc=size=320x240:rate=25:duration=2",
                   "-filter_complex", "[0:v][1:v][2:v]concat=n=3:v=1:a=0,format=yuv420p[v]", "-map", "[v]",
                   "-c:v", "mpeg4", "-q:v", "3", "-timecode", "01:00:00:00", video.getFullPathName() });

        if (! video.existsAsFile())
        {
            check (false, "cuts: ffmpeg made a test video");
            return;
        }

        std::vector<double> times;
        const auto start = juce::Time::getMillisecondCounterHiRes();
        const auto error = Ffmpeg::findSceneChanges (ffmpeg, video, Picture::cutThreshold, times);
        const auto ms = juce::Time::getMillisecondCounterHiRes() - start;
        juce::StringArray found;

        for (auto t : times)
            found.add (juce::String (t, 3));

        check (error.isEmpty() && times.size() == 2 && std::abs (times[0] - 2.0) < 0.5 / 25.0 && std::abs (times[1] - 3.6) < 0.5 / 25.0,
               "cuts: ffmpeg's scene detection finds the two cuts of a generated video, at 2.0 and 3.6 s, and no cut in the moving shots (found "
                   + found.joinIntoString (", ") + " s, in " + juce::String (juce::roundToInt (ms)) + " ms)");

        SessionState s;
        s.sampleRate = rate;
        s.video = video;
        s.videoStart = 1.5;
        const auto added = SessionEdits::addCutMarkers (s, times, 25.0);
        const auto repeated = SessionEdits::addCutMarkers (s, times, 25.0);
        check (added == 2 && repeated == 0 && s.markers.size() == 2 && s.markers[0].name == "Cut 01" && s.markers[1].name == "Cut 02"
                   && std::abs (s.markers[0].start - 168000) <= 960 && std::abs (s.markers[1].start - 244800) <= 960,
               "cuts: Find Cuts' markers go where the video starts in the session plus each cut, named Cut 01 on; finding them again adds none");

        // The video's frame rate and timecode give the session's.
        const auto picture = Ffmpeg::probePicture (ffmpeg, video);
        const auto atZero = Picture::timecodeOf (picture, 0.0);
        const auto later = Picture::timecodeOf (picture, 2.0);
        check (picture.timecode == "01:00:00:00" && atZero.has_value() && *atZero == TimeFormat::Timecode { Kind::smpte25, 90000 }
                   && later.has_value() && TimeFormat::format (2 * (juce::int64) rate, rate, *later) == "01:00:00:00",
               "video timecode: a 25 fps video starting at 01:00:00:00 gives the session 25 fps, its first frame showing 01:00:00:00 where it starts in the session (at 0 and at 2 s)");

        const auto ntsc = folder.getChildFile ("Drop.mov");
        runTool ({ ffmpeg.getFullPathName(), "-nostdin", "-v", "error", "-y", "-f", "lavfi", "-i", "testsrc2=size=160x120:rate=30000/1001:duration=1",
                   "-c:v", "mpeg4", "-timecode", "00:00:59;28", ntsc.getFullPathName() });
        const auto dropped = Picture::timecodeOf (Ffmpeg::probePicture (ffmpeg, ntsc), 0.0);
        const auto plain = folder.getChildFile ("Film.mov");
        runTool ({ ffmpeg.getFullPathName(), "-nostdin", "-v", "error", "-y", "-f", "lavfi", "-i", "testsrc2=size=160x120:rate=24000/1001:duration=1",
                   "-c:v", "mpeg4", plain.getFullPathName() });
        const auto film = Picture::timecodeOf (Ffmpeg::probePicture (ffmpeg, plain), 0.0, 1234);
        check (dropped.has_value() && dropped->rate == Kind::smpte2997Drop && TimeFormat::countedText (dropped->start, Kind::smpte2997Drop) == "00:00:59;28"
                   && TimeFormat::format ((juce::int64) std::ceil (2.0 * 1001.0 / 30000.0 * rate), rate, *dropped) == "00:01:00;02"
                   && film.has_value() && film->rate == Kind::smpte23976 && film->start == 1234,
               "video timecode: a 29.97 video with drop-frame timecode gives drop frame, counting over the minute's dropped numbers; a 23.976 video with none keeps the start");
    }

    //==============================================================================
    /** Frames of the given beats at a tempo, each moved by its jitter in ms. */
    std::vector<juce::int64> hitsAt (double bpm, const std::vector<double>& beats, const std::vector<double>& jitterMs)
    {
        std::vector<juce::int64> hits;

        for (size_t i = 0; i < beats.size(); ++i)
            hits.push_back ((juce::int64) std::llround ((beats[i] * 60.0 / bpm + jitterMs[i] / 1000.0) * rate));

        return hits;
    }

    juce::int64 worstOf (const std::vector<juce::int64>& misses)
    {
        juce::int64 worst = 0;

        for (auto m : misses)
            worst = juce::jmax (worst, std::abs (m));

        return worst;
    }

    void testFitTempo (AudioEngine& engine)
    {
        const juce::int64 frame = 1920;   // a frame at 25 fps, at 48 kHz
        const auto ms = [] (juce::int64 frames) { return juce::String (1000.0 * (double) frames / rate, 1) + " ms"; };

        // Hits on beats 3, 8, 13, 21, 30 and 44 of 97.3 BPM, each a few ms off as tapped.
        const auto beatHits = hitsAt (97.3, { 3, 8, 13, 21, 30, 44 }, { 4, -6, 2, -3, 5, -1 });
        const auto onBeats = Picture::fitTempo (beatHits, rate, 4, Picture::LandOn::beats, 90.0, 110.0, frame);
        check (onBeats.isValid() && onBeats.within == 6 && worstOf (onBeats.misses) < frame && std::abs (onBeats.tempo - 97.3) < 0.05 && onBeats.changes.empty(),
               "fit tempo: six hits tapped on beats of 97.3 BPM, looked for from 90 to 110, give " + juce::String (onBeats.tempo, 3) + " BPM, every hit within a frame (the farthest "
                   + ms (worstOf (onBeats.misses)) + " off)");

        // On bars of 4/4 at 123.7.
        const auto barHits = hitsAt (123.7, { 8, 20, 36, 48, 64 }, { -3, 2, 6, -4, 1 });
        const auto onBars = Picture::fitTempo (barHits, rate, 4, Picture::LandOn::bars, 110.0, 130.0, frame);
        juce::int64 offBar = 0;
        const auto barMap = Metronome::TempoMap (onBars.tempo, 4, onBars.changes, rate);

        for (auto h : barHits)
            offBar = juce::jmax (offBar, std::abs (h - barMap.barFrame (std::llround (barMap.beatPosition (h) / 4.0))));

        check (onBars.isValid() && onBars.within == 5 && offBar < frame && std::abs (onBars.tempo - 123.7) < 0.05,
               "fit tempo: hits on bar lines of 123.7 BPM in 4/4 give " + juce::String (onBars.tempo, 3) + " BPM, each within a frame of a bar's first beat (the farthest " + ms (offBar) + " off)");

        // Hits anywhere: a tempo map puts each on a bar line, the beats between as near the fitted tempo as whole beats allow.
        const std::vector<juce::int64> loose { 65760, 235200, 336960, 600000, 628800, 1003200 };   // 1.37, 4.9, 7.02, 12.5, 13.1 and 20.9 s
        const auto map = Picture::fitTempo (loose, rate, 4, Picture::LandOn::barLines, 100.0, 140.0, frame);
        auto lowest = map.tempo, highest = map.tempo;

        for (auto& c : map.changes)
        {
            lowest = juce::jmin (lowest, c.tempo);
            highest = juce::jmax (highest, c.tempo);
        }

        check (map.isValid() && worstOf (map.misses) <= 1 && map.within == 6 && lowest >= 20.0 && highest <= 400.0 && ! map.changes.empty(),
               "fit tempo: a tempo map puts six hits at odd times on bar lines, each within " + juce::String (worstOf (map.misses)) + " samples, with "
                   + juce::String ((int) map.changes.size()) + " changes from " + juce::String (lowest, 1) + " to " + juce::String (highest, 1) + " BPM");

        // Applied to the session: one undo step; its tempo map has each hit on a bar line, and a
        // musical track's clip keeps its beat (beat 12, bar 4 before the fit).
        SessionState s;
        s.sampleRate = rate;
        s.addTrack();
        const auto clip = SessionEdits::addClip (s, 0, juce::File ("/tmp/loop.wav"), AudioEdits::fromBuffer (juce::AudioBuffer<float> (1, 1000)), s.tempoMap().barFrame (3));
        SessionEdits::setTrackMusical (s, s.tracks[0].id, true);
        engine.session.reset (s, {});
        auto next = *engine.session.getState();
        const auto applied = SessionEdits::applyTempoFit (next, map);
        engine.session.apply (std::move (next), "Fit Tempo to Hits");
        const auto now = engine.session.getState();
        const auto tempoMap = now->tempoMap();
        juce::int64 offLine = 0;

        for (auto h : loose)
            offLine = juce::jmax (offLine, std::abs (h - tempoMap.barFrame (tempoMap.barAt (h + 1))));

        const auto where = now->findClip (clip);
        const auto keptBar = where.has_value() && now->tracks[(size_t) where->first].clips[(size_t) where->second].position == tempoMap.beatFrame (12);
        engine.session.undo();
        check (applied && offLine <= 1 && keptBar && engine.session.getState()->tempoChanges.empty() && juce::exactlyEqual (engine.session.getState()->tempo, 120.0),
               "fit tempo: applied, the session's tempo map has every hit on a bar line (within " + juce::String (offLine) + " samples), a musical clip stays on its beat, and undo puts 120 BPM back");

        // Ask Torq: fit_tempo_to_hits on named markers, leaving out a stray one.
        SessionState cue;
        cue.sampleRate = rate;
        cue.tempo = 100.0;

        for (auto h : beatHits)
            SessionEdits::addHitMarker (cue, h);

        cue.markers.push_back ({ 700000, 0, "Stray", -1 });
        MarkerEdits::sort (cue.markers);
        juce::StringArray steps;
        Ai::SessionTools tools (cue, "Cue", steps);
        const auto named = tools.run ("fit_tempo_to_hits", juce::JSON::parse (R"({"markers": ["Hit 01", "Hit 02", "Hit 03", "Hit 04", "Hit 05", "Hit 06"], "land_on": "beats", "min_bpm": 90, "max_bpm": 110})"));
        const auto unknown = tools.run ("fit_tempo_to_hits", juce::JSON::parse (R"({"markers": ["Hit 09"]})"));
        const auto fitted = tools.getState().tempo;
        check (named.has_value() && (bool) (*named)["ok"] && (int) (*named)["within_a_frame"] == 6 && (*named)["hits"].size() == 6 && std::abs (fitted - 97.3) < 0.05
                   && unknown.has_value() && ! (bool) (*unknown)["ok"] && steps.size() == 1,
               "fit tempo: Ask Torq's fit_tempo_to_hits fits the named markers (" + juce::String (fitted, 3) + " BPM) and refuses a marker that isn't there");

        check (! Picture::fitTempo ({}, rate, 4, Picture::LandOn::beats, 90.0, 110.0, frame).isValid()
                   && ! Picture::fitTempo (beatHits, rate, 4, Picture::LandOn::beats, 10.0, 110.0, frame).isValid()
                   && ! Picture::fitTempo (beatHits, rate, 4, Picture::LandOn::beats, 120.0, 110.0, frame).isValid(),
               "fit tempo: nothing without hits, or with tempos outside 20 to 400 or the wrong way round");
    }

    void testFitTempoPanel()
    {
        // Five hits on beats of 97.3 BPM and one stray marker, in a session at 25 fps timecode.
        SessionState s;
        s.sampleRate = rate;
        s.tempo = 100.0;
        s.timecode = { Kind::smpte25, 90000 };
        const auto hits = hitsAt (97.3, { 4, 9, 17, 26, 40 }, { 3, -2, 5, -4, 2 });

        for (auto h : hits)
            SessionEdits::addHitMarker (s, h);

        s.markers.push_back ({ 1000000, 0, "Marker 01", -1 });
        s.markers.push_back ({ 400000, 48000, "Region 01", -1 });
        MarkerEdits::sort (s.markers);

        FitTempoPanel panel (s, {});
        panel.setSize (panel.getWidth(), panel.getHeight());
        const auto stray = panel.getNumRows() == 6 ? 4 : -1;   // in time order, the stray marker comes after four hits
        const auto allChosen = panel.getNumRows() == 6 && panel.isChosen (0) && panel.isChosen (5) && panel.getName (4) == "Marker 01";
        const auto withStray = panel.getFit().within;
        const auto shownTime = panel.getTimeText (0);

        if (stray >= 0)
            panel.mouseDown (clickAt (panel, panel.getRowArea (stray).getCentre().toFloat()));

        const auto without = panel.getFit();
        check (allChosen && ! panel.isChosen (4) && without.within == 5 && withStray < 6 && std::abs (without.tempo - 97.3) < 0.05
                   && panel.getMissText (4).isEmpty() && panel.getMissText (0).endsWith (" fr") && panel.getSummary().contains ("5 of 5 hits within a frame")
                   && shownTime.startsWith ("01:00:"),
               "fit tempo panel: every point marker listed and chosen, shown in the session's timecode; clicking the stray one leaves it out, and the five hits fit at "
                   + juce::String (without.tempo, 3) + " BPM (" + panel.getSummary() + ")");

        panel.setLandOn (Picture::LandOn::barLines);
        check (panel.getFit().isValid() && worstOf (panel.getFit().misses) <= 1 && panel.getSummary().startsWith ("A tempo map"),
               "fit tempo panel: Bar lines makes a tempo map with each chosen hit on a bar line");

        panel.setTempoRange (200.0, 100.0);
        check (! panel.getFit().isValid() && panel.getSummary().contains ("lower first"), "fit tempo panel: a range the wrong way round fits nothing and says so");

        FitTempoPanel ranged (s, { hits[1] - 10, hits[3] + 10 });
        check (! ranged.isChosen (0) && ranged.isChosen (1) && ranged.isChosen (2) && ranged.isChosen (3) && ! ranged.isChosen (4) && ! ranged.isChosen (5),
               "fit tempo panel: with a time range, the markers in it are the ones chosen to start with");
    }

    //==============================================================================
    void testSpotting (AudioEngine& engine)
    {
        juce::AudioFormatManager formats;
        AudioFileIO::registerFormats (formats);
        juce::TemporaryFile folderTemp;
        const auto folder = folderTemp.getFile();
        folder.createDirectory();

        // A sound 1 s long, quiet, with a loud sample at `peak`.
        const auto sound = [] (int length, int peak)
        {
            juce::AudioBuffer<float> b (2, length);

            for (int i = 0; i < length; ++i)
                for (int ch = 0; ch < 2; ++ch)
                    b.setSample (ch, i, 0.1f * (float) std::sin (0.01 * i));

            b.setSample (1, peak, -0.9f);
            return AudioEdits::fromBuffer (std::move (b));
        };

        const auto withSync = folder.getChildFile ("Punch.wav");
        const auto withCue = folder.getChildFile ("Door.wav");
        const auto withNone = folder.getChildFile ("Whoosh.wav");
        const auto slower = folder.getChildFile ("Slam 44k.wav");
        AudioFileIO::save (*sound (48000, 30000), rate, withSync, 24, formats, Markers { { 3000, 0, "Start", -1 }, { 12000, 0, "Sync", -1 } });
        AudioFileIO::save (*sound (48000, 30000), rate, withCue, 24, formats, Markers { { 7000, 0, "Latch", -1 }, { 20000, 0, "Close", -1 } });
        AudioFileIO::save (*sound (48000, 25000), rate, withNone, 24, formats);
        AudioFileIO::save (*sound (44100, 30000), 44100.0, slower, 24, formats, Markers { { 22050, 0, "sync point", -1 } });

        const auto loaded = AudioFileIO::load (withNone, formats);
        check (Picture::syncPoint (AudioFileIO::load (withSync, formats).markers, *AudioFileIO::load (withSync, formats).snapshot) == 12000
                   && Picture::syncPoint (AudioFileIO::load (withCue, formats).markers, *AudioFileIO::load (withCue, formats).snapshot) == 7000
                   && Picture::syncPoint (loaded.markers, *loaded.snapshot) == 25000,
               "spotting: a sound's sync point is its cue named Sync, else its first cue, else its loudest sample");

        const juce::int64 marker = 96000;
        SessionState s;
        s.sampleRate = rate;
        s.addTrack();
        s.markers = { { marker, 0, "Hit 01", -1 }, { 2000, 0, "Early", -1 } };
        int added = 0;
        const auto errors = Picture::spotFiles (s, { withSync.getFullPathName(), withCue.getFullPathName(), withNone.getFullPathName(), slower.getFullPathName() },
                                                0, marker, formats, added);

        // Each clip plays its sound's sync sample at the marker's frame.
        const auto& tracks = s.tracks;
        juce::StringArray placed;
        auto synced = 0;
        const std::vector<juce::int64> syncs { 12000, 7000, 25000, 24000 };   // the 44.1 kHz one's at 0.5 s

        for (size_t t = 0; t < tracks.size() && t < syncs.size(); ++t)
        {
            if (tracks[t].clips.size() != 1)
                continue;

            const auto& c = tracks[t].clips[0];
            placed.add (juce::String (c.position));
            synced += c.offset + (marker - c.position) == syncs[t] ? 1 : 0;
        }

        // The peak sample sits exactly at the marker in the whoosh's clip.
        juce::AudioBuffer<float> at (2, 1);
        const auto& whoosh = tracks.size() > 2 && ! tracks[2].clips.empty() ? tracks[2].clips[0] : tracks[0].clips[0];
        whoosh.audio->read (at, 0, whoosh.takeZeroFrame (marker), 1);
        check (errors.isEmpty() && added == 4 && tracks.size() == 4 && synced == 4 && std::abs (at.getSample (1, 0) + 0.9f) < 1.0e-5f,
               "spotting: four sounds dropped on a marker each land with their sync point on it, a 44.1 kHz one converted (starts " + placed.joinIntoString (", ") + ")");

        // On a marker near the start the sound's head is left out.
        SessionState early;
        early.sampleRate = rate;
        early.addTrack();
        int addedEarly = 0;
        Picture::spotFiles (early, { withSync.getFullPathName() }, 0, 2000, formats, addedEarly);
        const auto& head = early.tracks[0].clips[0];
        check (addedEarly == 1 && head.position == 0 && head.offset == 10000 && head.length == 38000,
               "spotting: on a marker earlier than the sound's sync point, it starts at 0 with its head left out, the sync point still on the marker");

        // In the view: dropped on a marker's line, the files are spotted there; elsewhere, dropped as before.
        s.tracks.resize (1);
        s.tracks[0].clips.clear();
        engine.session.reset (s, {});
        MultitrackView view (engine);
        constexpr int timelineWidth = 1000;
        view.setSize (MultitrackView::headerWidth + timelineWidth + 12, MultitrackView::rulerHeight + 3 * MultitrackView::trackHeight + 12);
        view.zoomToRange ({ 0, 480000 });
        juce::MessageManager::getInstance()->runDispatchLoopUntil (30);
        const auto shown = view.getVisibleFrames();
        const auto xOf = [&] (double frame) { return (int) std::lround ((double) MultitrackView::headerWidth + (frame - shown.getStart()) / (shown.getLength() / timelineWidth)); };
        const auto bodyY = MultitrackView::rulerHeight + MultitrackView::trackHeight / 2;
        juce::int64 spottedAt = -1, droppedAt = -1;
        view.onFilesSpotted = [&spottedAt] (const juce::StringArray&, int, juce::int64 m) { spottedAt = m; };
        view.onFilesDropped = [&droppedAt] (const juce::StringArray&, int, juce::int64 f) { droppedAt = f; };
        view.setSnapping (false);
        view.filesDropped ({ withSync.getFullPathName() }, xOf ((double) marker) + 3, bodyY);
        const auto onMarker = spottedAt;
        view.filesDropped ({ withSync.getFullPathName() }, xOf ((double) marker) + 40, bodyY);
        check (onMarker == marker && droppedAt > marker + 4800 && spottedAt == marker,
               "spotting: in the view, files dropped 3 pixels from a marker's line are spotted on it; 40 pixels away they are dropped where they fall");
    }
}

void runPictureTests (AudioEngine& engine)
{
    testTimecode();
    testSessionFile (engine);
    testFitTempo (engine);
    testFitTempoPanel();
    testSpotting (engine);

    if (const auto ffmpeg = Ffmpeg::find(); ffmpeg != juce::File())
        testCuts (ffmpeg);
    else
        std::cout << "NOTE  cut detection tests skipped: no ffmpeg installed" << std::endl;
}
