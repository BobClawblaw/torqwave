// How many tracks the session's mix keeps up with. Not part of the suite: run it by hand with
// TorqwaveTests --stress (a few minutes; nothing else heavy on the machine meanwhile).
#include <JuceHeader.h>
#include "../Source/AudioEngine.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/OfflineRender.h"
#include "../Source/TrackEffects.h"
#include "StressTests.h"

namespace
{
    constexpr double rate = 48000.0;
    constexpr int seconds = 10;
    constexpr int block = 512;

    AudioDocument::Snapshot noise (int frames, int seed)
    {
        juce::Random r (seed);
        juce::AudioBuffer<float> b (2, frames);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < frames; ++i)
                b.setSample (ch, i, r.nextFloat() * 0.2f - 0.1f);

        std::vector<AudioSnapshot::Segment> segments { { std::make_shared<const AudioBlock> (std::move (b)), 0, frames } };
        return std::make_shared<const AudioSnapshot> (2, std::move (segments));
    }

    /** n stereo tracks, each a clip the whole length, panned across the field. */
    SessionState session (int n, const AudioDocument::Snapshot& audio)
    {
        SessionState s;
        s.sampleRate = rate;

        for (int t = 0; t < n; ++t)
        {
            auto& track = s.addTrack();
            track.pan = n > 1 ? -1.0f + 2.0f * (float) t / (float) (n - 1) : 0.0f;
            SessionClip c;
            c.id = s.nextId++;
            c.audio = audio;
            c.length = audio->getNumFrames();
            track.clips.push_back (c);
        }

        return s;
    }

    juce::PluginDescription builtIn (const char* id)
    {
        for (auto& d : BuiltInEffects::getDescriptions())
            if (d.fileOrIdentifier == id)
                return d;

        return {};
    }

    /** Seconds of audio mixed per second of work (above 1 keeps up). With `effects`, each track
        gets a Parametric EQ and a Compressor. */
    double speed (AudioEngine& engine, int n, bool effects, const AudioDocument::Snapshot& audio)
    {
        const auto s = session (n, audio);
        TrackEffects inserts;
        inserts.prepare (rate);

        if (effects)
            for (auto& t : s.tracks)
                for (auto* id : { BuiltInEffects::parametricEqId, BuiltInEffects::compressorId })
                {
                    juce::String error;
                    inserts.add (t.id, engine.pluginFormatManager.createPluginInstance (builtIn (id), rate, block, error));
                }

        juce::AudioBuffer<float> mix;
        const auto start = juce::Time::getMillisecondCounterHiRes();
        OfflineRender::mixSession (s, { 0, (juce::int64) seconds * (juce::int64) rate }, mix, &inserts, {});
        const auto took = (juce::Time::getMillisecondCounterHiRes() - start) / 1000.0;
        return took > 0.0 ? seconds / took : 1.0e9;
    }
}

void runStressTests (AudioEngine& engine)
{
    // Audio from memory, so the disk is not measured: this is the mix and the plug-ins alone.
    const auto audio = noise ((int) rate * seconds, 1);

    std::cout << "Stress: tracks of stereo audio mixed for " << seconds << " s at 48 kHz, as fast as the machine goes.\n"
              << "Speed is seconds mixed per second of work; live playback wants it well above 1 (2 or more leaves room)." << std::endl;

    for (const auto effects : { false, true })
    {
        std::cout << (effects ? "\nWith a Parametric EQ and a Compressor on each track:" : "\nPlain tracks:") << std::endl;
        int lastAbove2 = 0;

        for (int n = 8; n <= 1024; n *= 2)
        {
            const auto x = speed (engine, n, effects, audio);
            std::cout << "  " << juce::String (n).paddedLeft (' ', 5) << " tracks: " << juce::String (x, 2) << "x" << std::endl;

            if (x >= 2.0)
                lastAbove2 = n;

            if (x < 1.0)
                break;
        }

        std::cout << "  Comfortable (2x or better) up to about " << lastAbove2 << " tracks." << std::endl;
    }

    std::cout << "\nNot measured: reading clips from disk, third-party plug-ins, and the device's block size;"
                 " the mix here runs on one thread as the audio thread does." << std::endl;
}
