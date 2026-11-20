#pragma clang diagnostic ignored "-Wfloat-equal"

#include <JuceHeader.h>
#include "../Source/AudioEdits.h"
#include "../Source/AudioEngine.h"
#include "../Source/TrackEffects.h"
#include "../Source/Session.h"
#include "TestUtils.h"
#include "ArrangerTests.h"

namespace
{
    using TestUtils::check;

    constexpr double rate = 48000.0;
    constexpr juce::int64 bar120 = 96000, bar100 = 115200;   // a 4/4 bar at 120 and at 100 beats a minute

    AudioDocument::Snapshot level (juce::int64 frames, float value)
    {
        juce::AudioBuffer<float> b (1, (int) frames);
        juce::FloatVectorOperations::fill (b.getWritePointer (0), value, (int) frames);
        return AudioEdits::fromBuffer (std::move (b));
    }

    /** Intro (bars 0-1) and Verse (bars 2-3) at 120, Chorus (bars 4-5) at 100 from bar 4; a clip
        filling each section, a volume point and a marker in the chorus. */
    struct Song
    {
        SessionState s;
        juce::uint32 intro = 0, verse = 0, chorus = 0;
        juce::uint32 introClip = 0, verseClip = 0, chorusClip = 0;

        Song()
        {
            s.sampleRate = rate;
            s.addTrack();
            SessionEdits::setTempoChange (s, { 4, 100.0, 4 });
            introClip = SessionEdits::addClip (s, 0, {}, level (2 * bar120, 0.1f), 0);
            verseClip = SessionEdits::addClip (s, 0, {}, level (2 * bar120, 0.2f), 2 * bar120);
            chorusClip = SessionEdits::addClip (s, 0, {}, level (2 * bar100, 0.3f), 4 * bar120);
            intro = SessionEdits::addSection (s, 0, 2 * bar120, "Intro");
            verse = SessionEdits::addSection (s, 2 * bar120, 4 * bar120, "Verse");
            chorus = SessionEdits::addSection (s, 4 * bar120, 4 * bar120 + 2 * bar100, "Chorus");
            s.tracks[0].volumeEnvelope.add (0, 0.0f);
            s.tracks[0].volumeEnvelope.add (4 * bar120, 0.0f);
            s.tracks[0].volumeEnvelope.add (4 * bar120 + 1000, -6.0f);
            s.tracks[0].volumeEnvelope.add (4 * bar120 + 5000, 0.0f);
            s.markers.push_back ({ 4 * bar120 + 2000, 0, "Hook" });
        }

        juce::int64 clipStart (juce::uint32 id) const
        {
            const auto w = s.findClip (id);
            return w.has_value() ? s.tracks[(size_t) w->first].clips[(size_t) w->second].position : -1;
        }

        juce::String order() const
        {
            juce::StringArray names;

            for (auto& sec : s.sections)
                names.add (sec.name + "@" + juce::String (sec.start));

            return names.joinIntoString (" ");
        }

        juce::String tempo() const
        {
            juce::StringArray changes;
            changes.add (juce::String (s.tempo, 0));

            for (auto& c : s.tempoChanges)
                changes.add (juce::String (c.bar) + ":" + juce::String (c.tempo, 0));

            return changes.joinIntoString (" ");
        }
    };

    void testSections()
    {
        Song song;
        auto& s = song.s;
        check (s.sections.size() == 3 && SessionEdits::addSection (s, bar120, 3 * bar120) == 0 && SessionEdits::addSection (s, 10 * bar120, 11 * bar120) != 0
                   && s.sections.back().name == "Section 4",
               "arranger: a section overlapping another is refused; one without a name is numbered");

        check (SessionEdits::renameSection (s, song.verse, "Verse 1") && ! SessionEdits::renameSection (s, song.verse, "  ")
                   && SessionEdits::setSectionColour (s, song.verse, 2) && ! SessionEdits::setSectionColour (s, song.verse, 2)
                   && s.findSection (song.verse)->name == "Verse 1",
               "arranger: sections are renamed and coloured");

        check (! SessionEdits::resizeSection (s, song.intro, 0, 3 * bar120) && SessionEdits::resizeSection (s, song.intro, 0, bar120)
                   && song.clipStart (song.verseClip) == 2 * bar120,
               "arranger: resizing a section moves its edges only, never over another");

        check (SessionEdits::removeSection (s, s.sections.back().id) && s.sections.size() == 3 && song.clipStart (song.chorusClip) == 4 * bar120,
               "arranger: removing a section leaves what was in it");
    }

    void testMove()
    {
        // The chorus before the verse: its bars, at 100, come with it, and the verse after it
        // plays at 120, then the session plays on at 100 as it did after the chorus.
        Song song;
        auto& s = song.s;
        check (SessionEdits::moveSection (s, song.chorus, 2 * bar120, false), "arranger: a section moves");
        check (song.order() == "Intro@0 Chorus@192000 Verse@422400" && s.findSection (song.chorus) != nullptr,
               "arranger: the moved section keeps its id, and the others close up and make room (" + song.order() + ")");
        check (song.clipStart (song.chorusClip) == 2 * bar120 && song.clipStart (song.verseClip) == 2 * bar120 + 2 * bar100
                   && song.clipStart (song.introClip) == 0,
               "arranger: the clips in it move with it, the others with the time");
        check (song.tempo() == "120 2:100 4:120 6:100" && s.tempoMap().barFrame (4) == 2 * bar120 + 2 * bar100,
               "arranger: its tempo change comes with it, and the bars after play at the tempo they had (" + song.tempo() + ")");
        const auto volume = [&s] (juce::int64 f) { return s.tracks[0].valueAt (AutomationParameter::volume, f); };
        check (juce::approximatelyEqual (volume (2 * bar120 + 1000), -6.0f) && juce::approximatelyEqual (volume (2 * bar120 + 5000), 0.0f)
                   && juce::approximatelyEqual (volume (2 * bar120 + 2 * bar100 + 1000), 0.0f)
                   && s.markers.size() == 1 && s.markers[0].start == 2 * bar120 + 2000,
               "arranger: its envelope points and markers come with it");

        check (! SessionEdits::moveSection (s, song.chorus, 2 * bar120 + 10, false) && ! SessionEdits::moveSection (s, song.chorus, 2 * bar120, false),
               "arranger: a section can't be moved into itself, or to where it is");

        // Moved later, past the verse: back where it was.
        SessionEdits::moveSection (s, song.chorus, 2 * bar120 + 2 * bar100 + 2 * bar120, false);
        check (song.order() == "Intro@0 Verse@192000 Chorus@384000" && song.tempo() == "120 4:100" && song.clipStart (song.chorusClip) == 4 * bar120,
               "arranger: moved on past the next section, it is back where it began (" + song.order() + ", " + song.tempo() + ")");

        // A locked clip stays where it is.
        Song locked;
        SessionEdits::setClipLocked (locked.s, locked.verseClip, true);
        SessionEdits::moveSection (locked.s, locked.verse, 0, false);
        check (locked.clipStart (locked.verseClip) == 2 * bar120 && locked.s.tracks[0].clips.size() == 3,
               "arranger: a locked clip in a moved section stays where it is, uncopied");
    }

    void testCopyDeleteInsert()
    {
        Song song;
        auto& s = song.s;
        check (SessionEdits::moveSection (s, song.verse, 4 * bar120, true), "arranger: a section is copied");
        check (song.order() == "Intro@0 Verse@192000 Verse@384000 Chorus@576000" && s.tracks[0].clips.size() == 4
                   && song.clipStart (song.chorusClip) == 6 * bar120 && song.tempo() == "120 6:100",
               "arranger: a copy after it doubles it, the chorus and its tempo moved on two bars (" + song.order() + ", " + song.tempo() + ")");

        Song gone;
        check (SessionEdits::deleteSection (gone.s, gone.verse) && gone.order() == "Intro@0 Chorus@192000"
                   && gone.clipStart (gone.chorusClip) == 2 * bar120 && gone.tempo() == "120 2:100" && gone.s.tracks[0].clips.size() == 2,
               "arranger: deleting a section closes its time; the chorus's tempo comes back with it (" + gone.tempo() + ")");

        Song first;
        SessionEdits::deleteSection (first.s, first.intro);
        SessionEdits::deleteSection (first.s, first.verse);
        check (first.tempo() == "100" && first.clipStart (first.chorusClip) == 0, "arranger: deleting from the start, the session's own tempo becomes the chorus's");

        Song more;
        check (SessionEdits::insertBarsBefore (more.s, more.chorus, 2) && more.clipStart (more.chorusClip) == 4 * bar120 + 2 * bar100
                   && more.order() == "Intro@0 Verse@192000 Chorus@614400" && more.tempo() == "120 4:100",
               "arranger: two bars put in before the chorus, at its tempo (" + more.order() + ", " + more.tempo() + ")");

        // Time edits that aren't whole bars leave the tempo alone.
        Song odd;
        SessionEdits::deleteTime (odd.s, 1000, 2000);
        SessionEdits::insertTime (odd.s, 5000, 777);
        check (odd.tempo() == "120 4:100", "arranger: time edits that aren't whole bars leave the tempo changes where they are");
    }

    void testChain()
    {
        Song song;
        auto& s = song.s;
        check (SessionEdits::setArrangement (s, { song.intro, song.chorus, 999, song.chorus, song.verse }) && s.arrangement.size() == 4,
               "arranger: the chain is set, unknown sections dropped");

        {
            auto moved = s;
            SessionEdits::setPlayArrangement (moved, true);
            SessionEdits::moveSection (moved, song.intro, s.findSection (song.chorus)->getEnd(), false);
            check (moved.arrangement == s.arrangement && moved.playArrangement, "arranger: a moved section keeps its place in the chain");

            SessionEdits::deleteSection (moved, song.chorus);
            check (moved.arrangement == std::vector<juce::uint32> { song.intro, song.verse } && moved.playArrangement,
                   "arranger: a deleted section leaves the chain");

            SessionEdits::deleteTime (moved, 0, moved.getLength());
            check (moved.arrangement.empty() && ! moved.playArrangement, "arranger: with every section gone, the chain is empty and doesn't play");
        }

        check (SessionEdits::flattenArrangement (s), "arranger: the chain is flattened");
        check (song.order() == "Intro@0 Chorus@192000 Chorus@422400 Verse@652800" && s.arrangement.size() == 4 && ! s.playArrangement
                   && s.tracks[0].clips.size() == 4,
               "arranger: flattened, the session plays the chain's sections in order (" + song.order() + ")");
        check (song.tempo() == "120 2:100 6:120 8:100", "arranger: each section brings its tempo (" + song.tempo() + ")");

        std::vector<juce::int64> starts;

        for (auto& c : s.tracks[0].clips)
            starts.push_back (c.position);

        std::sort (starts.begin(), starts.end());
        check (starts == std::vector<juce::int64> { 0, 192000, 422400, 652800 }, "arranger: and their clips");
    }

    void testChainPlayback()
    {
        // A over [0, 1000), B over [1000, 2000), each a clip of its own level; the chain B A B.
        SessionState s;
        s.sampleRate = rate;
        s.addTrack();
        SessionEdits::addClip (s, 0, {}, level (1000, 0.1f), 0);
        SessionEdits::addClip (s, 0, {}, level (1000, 0.2f), 1000);
        const auto a = SessionEdits::addSection (s, 0, 1000, "A"), b = SessionEdits::addSection (s, 1000, 2000, "B");

        const auto play = [] (const SessionState& state, juce::int64 from, int frames)
        {
            TrackEffects none;
            none.prepare (rate);
            Session session;
            session.reset (state, {});
            SessionSource source (session, none);
            constexpr int block = 256;
            source.prepareToPlay (block, rate);
            source.setNextReadPosition (from);
            juce::AudioBuffer<float> out (2, frames);
            out.clear();

            for (int done = 0; done < frames; done += block)
                source.getNextAudioBlock ({ &out, done, juce::jmin (block, frames - done) });

            return out;
        };

        const auto plain = play (s, 0, 4000);
        SessionEdits::setArrangement (s, { b, a, b });
        SessionEdits::setPlayArrangement (s, true);
        const auto chained = play (s, 1000, 4000);

        auto same = true;
        const auto expect = [&] (int at, int from, int count)
        {
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < count; ++i)
                    same = same && chained.getSample (ch, at + i) == plain.getSample (ch, from + i);
        };

        expect (0, 1000, 1000);
        expect (1000, 0, 1000);
        expect (2000, 1000, 1000);
        expect (3000, 2000, 1000);
        check (same && plain.getSample (0, 500) != plain.getSample (0, 1500),
               "arranger: playing the chain B A B plays B, A and B again, then on past B's end, jumping inside blocks");

        // From inside A, the chain's entry for A plays on into the second B, then on.
        const auto fromA = play (s, 500, 2000);
        auto same2 = true;

        for (int i = 0; i < 500; ++i)
            same2 = same2 && fromA.getSample (0, i) == plain.getSample (0, 500 + i);

        for (int i = 0; i < 1000; ++i)
            same2 = same2 && fromA.getSample (0, 500 + i) == plain.getSample (0, 1000 + i);

        check (same2, "arranger: starting inside a section, the chain plays on from its entry");
    }

    void testFile (juce::AudioFormatManager& formats)
    {
        Song song;
        SessionEdits::setArrangement (song.s, { song.verse, song.intro, song.verse });
        SessionEdits::setPlayArrangement (song.s, true);
        SessionEdits::setSectionColour (song.s, song.chorus, 3);
        song.s.tracks[0].clips.clear();   // no audio files to save

        const auto file = juce::File::createTempFile ("tqsession");
        const auto error = SessionFile::save (song.s, file);
        const auto loaded = SessionFile::load (file, formats);
        const auto& got = loaded.state;
        auto same = error.isEmpty() && loaded.error.isEmpty() && got.sections.size() == 3 && got.arrangement.size() == 3 && got.playArrangement;

        for (size_t i = 0; same && i < 3; ++i)
            same = got.sections[i].name == song.s.sections[i].name && got.sections[i].start == song.s.sections[i].start
                   && got.sections[i].length == song.s.sections[i].length && got.sections[i].colour == song.s.sections[i].colour;

        check (same && got.arrangement[0] == got.sections[1].id && got.arrangement[1] == got.sections[0].id && got.arrangement[2] == got.sections[1].id,
               "arranger: sections, the chain and playing it come back from a session file");
        file.deleteFile();
    }
}

void runArrangerTests (juce::AudioFormatManager& formats)
{
    testSections();
    testMove();
    testCopyDeleteInsert();
    testChain();
    testChainPlayback();
    testFile (formats);
}
