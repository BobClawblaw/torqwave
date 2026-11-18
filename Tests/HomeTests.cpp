#include <JuceHeader.h>
#include "../Source/AudioFileIO.h"
#include "../Source/Home.h"
#include "TestUtils.h"
#include "HomeTests.h"

namespace
{
    using TestUtils::check;

    /** A WAV of a sine at 0.5 for the first half and 0.25 for the second. */
    void writeWav (const juce::File& f)
    {
        juce::AudioBuffer<float> b (2, 48000);

        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const auto level = i < 24000 ? 0.5f : 0.25f;
            const auto v = level * (float) std::sin (juce::MathConstants<double>::twoPi * 440.0 * i / 48000.0);
            b.setSample (0, i, v);
            b.setSample (1, i, -v);
        }

        juce::WavAudioFormat wav;
        f.deleteFile();
        std::unique_ptr<juce::OutputStream> stream (f.createOutputStream());
        auto w = wav.createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (48000.0).withNumChannels (2).withBitsPerSample (24));
        w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    }

    void testReading (const juce::File& dir)
    {
        // A session: a track with two clips (one coloured), a hidden track and a bus, which are left out.
        const auto session = dir.getChildFile ("song.tqsession");
        session.replaceWithText (R"(<?xml version="1.0"?>
<TORQWAVE_SESSION version="1" sampleRate="48000">
  <TRACK name="Voice" colour="3">
    <CLIP name="a" position="0" length="96000"/>
    <CLIP name="b" position="144000" length="48000" colour="5"/>
  </TRACK>
  <TRACK name="Lead" instrument="1"><NOTECLIP name="n" position="48000" length="48000"/></TRACK>
  <TRACK name="Hidden" hidden="1"><CLIP name="h" position="0" length="480000"/></TRACK>
  <TRACK name="Reverb" bus="1"/>
</TORQWAVE_SESSION>)");
        const auto a = Home::arrangementOf (session);
        check (a.has_value() && a->lanes.size() == 2 && a->lanes[0].colour == 3 && a->lanes[0].clips.size() == 2
                   && a->lanes[0].clips[1].start == 3.0 && a->lanes[0].clips[1].end == 4.0 && a->lanes[0].clips[1].colour == 5
                   && a->lanes[1].clips.size() == 1 && a->lanes[1].clips[0].notes && a->length == 4.0,
               "home: a session's picture is its shown tracks and their clips, in seconds, buses and hidden tracks left out");
        check (! Home::arrangementOf (dir.getChildFile ("missing.tqsession")).has_value(), "home: a session that can't be read has no picture");

        // Projects: in the session, its session; in the editor, the file it showed.
        const auto wav = dir.getChildFile ("take.wav");
        writeWav (wav);
        const auto inSession = dir.getChildFile ("a.tqproject"), inEditor = dir.getChildFile ("b.tqproject");
        inSession.replaceWithText (R"(<TORQWAVE_PROJECT version="1" active="0" multitrack="1"><FILES><FILE path="/nowhere/x.wav"/></FILES><SESSION path="" pathRelative="song.tqsession"/></TORQWAVE_PROJECT>)");
        inEditor.replaceWithText (R"(<TORQWAVE_PROJECT version="1" active="1" multitrack="0"><FILES><FILE path="/nowhere/x.wav"/><FILE path="" pathRelative="take.wav"/></FILES><SESSION path="" pathRelative="song.tqsession"/></TORQWAVE_PROJECT>)");
        check (Home::faceOf (inSession).session == session && Home::faceOf (inEditor).audio == wav && Home::faceOf (inEditor).session == juce::File(),
               "home: a project shows its session when it was in front, else the file it showed");

        // A waveform: the first half's columns reach about 0.5, the second's about 0.25, both signs (the right channel inverted).
        juce::AudioFormatManager formats;
        AudioFileIO::registerFormats (formats);
        const auto peaks = Home::Pictures::readWaveform (wav, formats);
        const auto at = [&peaks] (int column) { return std::pair<float, float> (peaks[(size_t) column * 2], peaks[(size_t) column * 2 + 1]); };
        check (peaks.size() == (size_t) Home::Pictures::columns * 2 && std::abs (at (10).second - 0.5f) < 0.01f && std::abs (at (10).first + 0.5f) < 0.01f
                   && std::abs (at (150).second - 0.25f) < 0.01f,
               "home: a file's waveform is each column's lowest and highest sample over its channels");
        check (Home::Pictures::readWaveform (session, formats).empty(), "home: a file that isn't audio has no waveform");
    }

    void testPictures (const juce::File& dir)
    {
        const auto wav = dir.getChildFile ("take.wav"), cache = dir.getChildFile ("cache");
        std::shared_ptr<const std::vector<float>> first, again;

        {
            Home::Pictures pictures (cache);
            auto changed = false;
            pictures.onChange = [&changed] { changed = true; };
            const auto none = pictures.waveform (wav) == nullptr;
            TestUtils::runUntil ([&] { return changed; }, 5000);
            first = pictures.waveform (wav);
            check (none && first != nullptr && first->size() == (size_t) Home::Pictures::columns * 2
                       && pictures.arrangement (dir.getChildFile ("a.tqproject")) == nullptr,
                   "home pictures: a waveform is made off the message thread and announced when ready");

            changed = false;
            pictures.arrangement (dir.getChildFile ("a.tqproject"));
            TestUtils::runUntil ([&] { return changed; }, 5000);
            const auto a = pictures.arrangement (dir.getChildFile ("a.tqproject"));
            check (a != nullptr && a->lanes.size() == 2, "home pictures: a project in its session gets the session's picture");
        }

        check (cache.getNumberOfChildFiles (juce::File::findFiles) == 1, "home pictures: the waveform is kept on disk");

        // Kept: a new instance reads it from the cache. The file is zeroed (same size and time), so
        // read again it would have no waveform at all.
        const auto time = wav.getLastModificationTime();
        wav.replaceWithData (juce::MemoryBlock ((size_t) wav.getSize(), true).getData(), (size_t) wav.getSize());
        wav.setLastModificationTime (time);
        {
            Home::Pictures pictures (cache);
            auto changed = false;
            pictures.onChange = [&changed] { changed = true; };
            pictures.waveform (wav);
            TestUtils::runUntil ([&] { return changed; }, 5000);
            again = pictures.waveform (wav);
        }

        check (again != nullptr && *again == *first, "home pictures: the next launch gets the same waveform from the disk");
    }
}

void runHomeTests()
{
    const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("torqwave-home-test");
    dir.deleteRecursively();
    dir.createDirectory();
    testReading (dir);
    testPictures (dir);
    dir.deleteRecursively();
}
