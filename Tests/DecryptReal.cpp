#include <JuceHeader.h>
#include "DecryptReal.h"
#include "../Source/Decrypt.h"
#include "TestUtils.h"
#include "../Source/WaveTables.h"
#include "../Source/WaveSynth.h"

namespace
{
    using namespace Decrypt;

    struct Recording
    {
        juce::String name;
        juce::AudioBuffer<float> audio;
        double sampleRate = 48000.0;
    };

    std::optional<Recording> read (const juce::File& file)
    {
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        const std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));

        if (reader == nullptr || reader->lengthInSamples <= 0)
            return std::nullopt;

        Recording r;
        r.name = file.getFileNameWithoutExtension();
        r.sampleRate = reader->sampleRate;
        const auto length = (int) juce::jmin (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 10.0));
        r.audio.setSize ((int) juce::jmin (2u, reader->numChannels), length);
        reader->read (&r.audio, 0, length, 0, true, r.audio.getNumChannels() > 1);
        return r;
    }


    float distanceOf (const Target& target, const std::vector<float>& mono)
    {
        return distance (target.measures, measure (mono.data(), (int) mono.size())).total;
    }

    /** How strict the measure is on this recording: the recording against itself shifted 5 ms,
        with noise 40 dB under its peak, and through a gentle low-pass at 8 kHz. */
    juce::String strictness (const Target& t)
    {
        const auto n = t.audio.getNumSamples();
        const auto* x = t.audio.getReadPointer (0);
        std::vector<float> shifted ((size_t) n, 0.0f), noisy ((size_t) n), dull ((size_t) n);
        juce::Random random (7);
        float lp = 0.0f;
        const auto a = (float) std::exp (-juce::MathConstants<double>::twoPi * 8000.0 / rate);

        for (int i = 0; i < n; ++i)
        {
            if (i >= 240) shifted[(size_t) i] = x[i - 240];
            noisy[(size_t) i] = x[i] + 0.5f * 0.01f * (random.nextFloat() * 2.0f - 1.0f);
            lp = (1.0f - a) * x[i] + a * lp;
            dull[(size_t) i] = lp;
        }

        // Changes anyone hears: a low-pass at 2 kHz, a high-pass at 300 Hz, a 60 ms fade-in, the tail cut short.
        std::vector<float> dark ((size_t) n), thin ((size_t) n), slow ((size_t) n), cut ((size_t) n);
        const auto a2 = (float) std::exp (-juce::MathConstants<double>::twoPi * 2000.0 / rate);
        const auto a3 = (float) std::exp (-juce::MathConstants<double>::twoPi * 300.0 / rate);
        float l2 = 0.0f, l3 = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            l2 = (1.0f - a2) * x[i] + a2 * l2;
            dark[(size_t) i] = l2;
            l3 = (1.0f - a3) * x[i] + a3 * l3;
            thin[(size_t) i] = x[i] - l3;
            slow[(size_t) i] = x[i] * juce::jmin (1.0f, (float) i / (0.06f * (float) rate));
            const auto from = (float) n * 0.4f;
            cut[(size_t) i] = x[i] * ((float) i < from ? 1.0f : std::exp (-((float) i - from) / (0.03f * (float) rate)));
        }

        return juce::String::formatted ("unheard: 5 ms later %.2f, noise at -40 dB %.2f, low-pass 8 kHz %.2f; heard: low-pass 2 kHz %.2f, high-pass 300 Hz %.2f, 60 ms fade-in %.2f, tail cut at 40 %% %.2f",
                                        distanceOf (t, shifted), distanceOf (t, noisy), distanceOf (t, dull),
                                        distanceOf (t, dark), distanceOf (t, thin), distanceOf (t, slow), distanceOf (t, cut));
    }

    /** The clip's own table alone on oscillator 1, scanned over the note, filters off, the amp
        envelope fitted to the loudness curve: the most direct start a recording allows. */
    Patch resynth (const Patch& from, const Target& t)
    {
        auto p = from;
        auto& v = p.values;
        p.tables[0] = WaveTables::sampledFromAudio (t.audio.getReadPointer (0), t.audio.getNumSamples(), rate, t.pitchHz, "Probe");
        v["osc1On"] = 1.0; v["osc1Table"] = (double) WaveTables::Table::sampled; v["osc1Warp"] = 0.0;
        v["osc1Pitch"] = 0.0; v["osc1Fine"] = 0.0; v["osc1EnvOn"] = 0.0; v["osc1Level"] = 100.0;
        v["osc2On"] = 0.0; v["osc3On"] = 0.0; v["subLevel"] = 0.0; v["noiseLevel"] = 0.0;
        v["filter1Type"] = 0.0; v["filter2Type"] = 0.0;

        for (int s = 1; s <= WaveSynth::numSlots; ++s)
            v["mod" + juce::String (s) + "Destination"] = 0.0;

        const auto& m = t.measures;
        int peak = 0;
        for (int f = 0; f < m.frames; ++f)
            if (m.loudness[(size_t) f] > m.loudness[(size_t) peak]) peak = f;
        const auto heldFrame = juce::jlimit (0, m.frames - 1, t.held / Measures::hop);
        v["env1Attack"] = juce::jmax (1.0, peak * 10.0);
        v["env1Decay"] = juce::jmax (10.0, (heldFrame - peak) * 10.0);
        v["env1Sustain"] = 100.0 * std::pow (10.0, m.loudness[(size_t) heldFrame] / 20.0);
        v["env1Release"] = juce::jmax (10.0, (m.frames - heldFrame) * 10.0 * 0.5);
        v["env1Curve"] = 0.0;
        const auto seconds = (double) t.audio.getNumSamples() / rate;
        v["env4Attack"] = 0.0; v["env4Decay"] = seconds * 1000.0; v["env4Sustain"] = 0.0; v["env4Release"] = seconds * 1000.0;
        v["mod1Source"] = (double) WaveSynth::env4;
        v["mod1Destination"] = (double) (WaveSynth::destinationIds().indexOf ("osc1Position") + 1);
        v["mod1Amount"] = -100.0;
        v["osc1Position"] = 100.0;
        return p;
    }

    /** A factory patch through its own effects (reverb, delay, drive...), as a recording of it would be. */
    Recording wet (const juce::String& patch, int note, double hold, double length)
    {
        Synth synth;
        synth.setPlayConfigDetails (0, 2, 48000.0, 512);
        synth.setNonRealtime (true);
        synth.loadPatch (patch);
        synth.prepareToPlay (48000.0, 512);
        const auto total = (int) (length * 48000.0), off = (int) (hold * 48000.0);
        Recording r;
        r.name = patch + " (wet)";
        r.audio.setSize (2, total);
        juce::AudioBuffer<float> part (2, 512);

        for (int at = 0; at < total; at += 512)
        {
            const auto n = juce::jmin (512, total - at);
            juce::MidiBuffer midi;
            if (at == 0) midi.addEvent (juce::MidiMessage::noteOn (1, note, 0.8f), 0);
            if (off >= at && off < at + n) midi.addEvent (juce::MidiMessage::noteOff (1, note), off - at);
            part.setSize (2, n, false, false, true);
            part.clear();
            synth.processBlock (part, midi);
            for (int ch = 0; ch < 2; ++ch) r.audio.copyFrom (ch, at, part, ch, 0, n);
        }

        return r;
    }
}

int runDecryptReal (const juce::StringArray& filesAndFlags)
{
    auto files = filesAndFlags;
    juce::File wavDir;
    const auto strictOnly = files.contains ("strict");
    files.removeString ("strict");

    for (auto& f : juce::StringArray (files))
        if (f.startsWith ("wav=")) { wavDir = juce::File (f.fromFirstOccurrenceOf ("=", false, false)); files.removeString (f); }

    std::vector<Recording> recordings;

    for (auto& path : files)
        if (auto r = read (juce::File (path)))
            recordings.push_back (std::move (*r));
        else
            std::printf ("%s couldn't be read\n", juce::File (path).getFileName().toRawUTF8());

    for (auto* sound : { "Submarine", "Bottle", "Basso" })
        if (auto r = read (juce::File ("/System/Library/Sounds").getChildFile (juce::String (sound) + ".aiff")))
            recordings.push_back (std::move (*r));

    recordings.push_back (wet ("Concrete Pad", 57, 1.2, 2.5));
    recordings.push_back (wet ("Wailing Fuzz Lead", 57, 1.0, 2.0));
    double totalLock = 0.0;

    for (auto& r : recordings)
    {
        const auto target = prepare (r.audio, r.sampleRate);

        if (target.problem.isNotEmpty())
        {
            std::printf ("%s: %s\n", r.name.toRawUTF8(), target.problem.toRawUTF8());
            continue;
        }

        if (strictOnly)
        {
            std::printf ("%s: %s\n", r.name.toRawUTF8(), strictness (target).toRawUTF8());
            continue;
        }

        // The factory stage alone, then the whole search until settled, its best after every round.
        Options first;
        first.rounds = 0;
        Search factoryOnly (target, first);
        const auto stage = factoryOnly.runNow();

        Options options;
        options.untilSettled = true;
        Search search (target, options);
        juce::StringArray rounds;
        int lastRound = -1;
        search.onChange = [&]
        {
            const auto p = search.getProgress();

            if (p.round != lastRound && ! p.best.empty())
            {
                lastRound = p.round;
                rounds.add (juce::String (p.best.front().distance.total, 2));
            }
        };
        search.start();
        TestUtils::runUntil ([&search] { return ! search.isRunning(); }, 10 * 60 * 1000);
        TestUtils::runUntil ([] { return false; }, 50);   // the last change, delivered
        const auto end = search.getProgress();

        if (end.best.empty() || stage.best.empty())
            continue;

        const auto& best = end.best.front();
        Synth renderer;

        // The chain, as the search made it: each link's lock, and how far its sound is from the link before's.
        {
            juce::StringArray links;

            for (auto& l : end.chain)
                links.add (juce::String (l.candidate.lock, 1) + " %" + (links.isEmpty() ? juce::String() : " (" + juce::String (l.movedDb, 1) + " dB on)"));

            std::printf ("%s's chain, %d links: %s\n", r.name.toRawUTF8(), (int) end.chain.size(), links.joinIntoString (", ").toRawUTF8());
        }

        // The chain as the window plays it: the signal, then each link, a breath apart.
        if (wavDir != juce::File())
        {
            std::vector<Patch> chain;
            std::vector<float> chainDb;

            for (auto& l : end.chain)
                chain.push_back (l.candidate.patch), chainDb.push_back (l.candidate.distance.total);

            const auto gap = (int) (0.4 * rate), length = target.audio.getNumSamples();
            juce::AudioBuffer<float> out (1, (int) (chain.size() + 1) * (length + gap));
            out.clear();
            out.copyFrom (0, 0, target.audio, 0, 0, length);
            juce::StringArray steps;

            for (size_t i = 0; i < chain.size(); ++i)
            {
                const auto audio = render (renderer, chain[i], target);
                auto mono = juce::AudioBuffer<float> (1, length);
                mono.clear();
                for (int ch = 0; ch < audio.getNumChannels(); ++ch)
                    mono.addFrom (0, 0, audio, ch, 0, juce::jmin (length, audio.getNumSamples()), 1.0f / (float) audio.getNumChannels());
                const auto peak = mono.getMagnitude (0, length);
                if (peak > 1.0e-6f) mono.applyGain (0.5f / peak);
                out.copyFrom (0, (int) (i + 1) * (length + gap), mono, 0, 0, length);
                steps.add (juce::String (chainDb[i], 2));
            }

            wavDir.createDirectory();
            const auto file = wavDir.getChildFile (r.name.replaceCharacters (" ()", "___") + "-chain.wav");
            file.deleteFile();
            juce::WavAudioFormat wavFormat;
            std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream> (file);
            if (auto writer = wavFormat.createWriterFor (stream, juce::AudioFormatWriterOptions{}.withSampleRate (rate).withNumChannels (1).withBitsPerSample (24)))
                writer->writeFromAudioSampleBuffer (out, 0, out.getNumSamples());
            std::printf ("  chain written: the signal, then steps at %s dB\n", steps.joinIntoString (", ").toRawUTF8());
        }
        {
            const auto start = resynth (best.patch, target);
            const auto startDb = distance (target.measures, measureRender (render (renderer, start, target))).total;
            Options carry;
            carry.untilSettled = true;
            carry.resume = { start };
            Search on (target, carry);
            const auto got = on.runNow();
            std::printf ("  strictness: %s\n  the clip's table, filters off, envelope fitted: %.2f dB at the start, %.2f dB settled (%d rounds)\n",
                         strictness (target).toRawUTF8(), startDb, got.best.empty() ? 0.0f : got.best.front().distance.total, got.round);
        }
        const auto difference = describeDifference (target.measures, measureRender (render (renderer, best.patch, target)));
        totalLock += best.lock;
        std::printf ("%s: %s\n  factory stage %.2f dB (%.1f %%, %s); until settled %.2f dB (%.1f %%: tone %.2f, loudness %.2f), %d rounds, %.1f s, from %s\n"
                     "  by round (dB): %s\n  still differs: %s\n",
                     r.name.toRawUTF8(), describe (target).toRawUTF8(), stage.best.front().distance.total, stage.best.front().lock,
                     stage.best.front().from.toRawUTF8(), best.distance.total, best.lock, best.distance.tone, best.distance.loudness, end.round, end.seconds,
                     best.from.toRawUTF8(), rounds.joinIntoString (" ").toRawUTF8(), difference.toRawUTF8());
    }

    std::printf ("DECRYPT-REAL %d recordings, mean lock %.1f %%\n", (int) recordings.size(), recordings.empty() ? 0.0 : totalLock / (double) recordings.size());
    return 0;
}
