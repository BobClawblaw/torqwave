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
    Recording wet (const juce::String& patch, int note, double hold, double length, bool effects = true)
    {
        Synth synth;
        synth.setPlayConfigDetails (0, 2, 48000.0, 512);
        synth.setNonRealtime (true);
        synth.loadPatch (patch);

        if (! effects)
            for (auto* id : { "fxDriveOn", "fxChorusOn", "fxPhaserOn", "fxDelayOn", "fxReverbOn" })
                if (auto* fx = synth.parameter (id))
                    fx->setValueNotifyingHost (0.0f);

        synth.prepareToPlay (48000.0, 512);
        const auto total = (int) (length * 48000.0), off = (int) (hold * 48000.0);
        Recording r;
        r.name = patch + (effects ? " (wet)" : "");
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

namespace
{
    /** Sounds with no pitch to find: a noise burst dying away, noise swept down through a band, a
        cluster of partials that are no one note's (a struck sheet of metal), and three claps of noise. */
    std::vector<Recording> unpitched()
    {
        std::vector<Recording> all;
        const auto make = [&all] (const char* name, double seconds, const std::function<float (int, juce::Random&)>& sample)
        {
            Recording r;
            r.name = name;
            r.audio.setSize (1, (int) (seconds * 48000.0));
            juce::Random random (11);

            for (int i = 0; i < r.audio.getNumSamples(); ++i)
                r.audio.setSample (0, i, sample (i, random));

            all.push_back (std::move (r));
        };

        make ("noise burst", 1.2, [] (int i, juce::Random& r) { return (r.nextFloat() * 2.0f - 1.0f) * 0.7f * std::exp (-(float) i / 9600.0f); });

        {
            float low = 0.0f, band = 0.0f;
            make ("noise sweep", 3.0, [&] (int i, juce::Random& r)
            {
                const auto t = (float) i / 144000.0f;
                const auto f = 2.0f * std::sin (juce::MathConstants<float>::pi * (6000.0f * std::pow (0.05f, t)) / 48000.0f);
                const auto high = (r.nextFloat() * 2.0f - 1.0f) - low - 0.4f * band;
                band += f * high;
                low += f * band;
                return band * 0.5f * juce::jmin (1.0f, (float) i / 2400.0f) * juce::jmin (1.0f, (float) (144000 - i) / 9600.0f);
            });
        }

        make ("metal sheet", 2.5, [] (int i, juce::Random&)
        {
            static const float partials[] { 211.0f, 347.0f, 523.7f, 811.3f, 1289.0f, 1873.1f, 2741.0f, 3907.0f };
            const auto t = (float) i / 48000.0f;
            auto x = 0.0f;

            for (int k = 0; k < 8; ++k)
                x += std::sin (juce::MathConstants<float>::twoPi * partials[k] * t) * std::exp (-t * (1.2f + 0.5f * (float) k)) / (1.0f + 0.4f * (float) k);

            return x * 0.25f;
        });

        make ("claps", 0.8, [] (int i, juce::Random& r)
        {
            auto e = 0.0f;

            for (auto at : { 0, 720, 1500 })
                if (i >= at)
                    e = juce::jmax (e, std::exp (-(float) (i - at) / (at == 1500 ? 4800.0f : 400.0f)));

            return (r.nextFloat() * 2.0f - 1.0f) * 0.6f * e;
        });

        return all;
    }

    float medianOf (std::vector<float> v)
    {
        if (v.empty())
            return 0.0f;

        std::sort (v.begin(), v.end());
        return v[v.size() / 2];
    }
}

int runDecryptVariety (const juce::StringArray& filesAndFlags)
{
    struct Group
    {
        juce::String name;
        std::vector<Recording> sounds;
        std::vector<juce::String> leaveOut;
    };

    std::vector<Group> groups;
    auto files = filesAndFlags;
    const auto quick = files.contains ("quick");
    files.removeString ("quick");

    {
        Group local { "local files", {}, {} };
        int n = 0;

        for (auto& path : files)
            if (auto r = read (juce::File (path)))
            {
                r->name = "file " + juce::String (++n);    // never by name
                local.sounds.push_back (std::move (*r));
                local.leaveOut.push_back ({});
            }

        if (! local.sounds.empty())
            groups.push_back (std::move (local));
    }

    {
        Group g { "unpitched", unpitched(), {} };
        g.leaveOut.resize (g.sounds.size());
        groups.push_back (std::move (g));
    }

    const std::pair<const char*, int> simple[] { { "Warehouse Sub", 45 }, { "Abyss Reese", 45 }, { "Acid Corrosion", 45 }, { "Bare Shiver", 33 }, { "Siren Burn", 57 },
                                                 { "Concrete Pad", 57 }, { "Black Pluck", 57 }, { "Cold Bell", 69 }, { "Dub Chord Stab", 57 }, { "Abrasive Bray", 57 } };
    const std::pair<const char*, int> complex[] { { "Hydraulic Maw", 45 }, { "Undertow Gnash", 45 }, { "Caustic Gnaw", 45 }, { "Bedrock Growl", 33 }, { "Voltage Scream", 57 },
                                                  { "Furnace Halo", 57 }, { "Rivet Snarl", 57 }, { "Foundry Knell", 69 }, { "Furnace Clamp", 57 }, { "Foundry Horns", 57 } };

    for (auto& [name, set] : { std::pair<const char*, const std::pair<const char*, int>*> { "simple set", simple }, { "complex set", complex } })
    {
        Group g { name, {}, {} };

        for (int i = 0; i < (quick ? 3 : 10); ++i)
        {
            g.sounds.push_back (wet (set[i].first, set[i].second, 0.8, 1.5, false));
            g.leaveOut.push_back (set[i].first);
        }

        groups.push_back (std::move (g));
    }

    {
        Group g { "system sounds and wet patches", {}, {} };

        for (auto* sound : { "Submarine", "Bottle", "Basso" })
            if (auto r = read (juce::File ("/System/Library/Sounds").getChildFile (juce::String (sound) + ".aiff")))
                g.sounds.push_back (std::move (*r));

        g.sounds.push_back (wet ("Concrete Pad", 57, 1.2, 2.5));
        g.leaveOut.resize (g.sounds.size());
        groups.push_back (std::move (g));
    }

    Synth renderer;

    for (auto& g : groups)
    {
        double lock = 0.0, seconds = 0.0, least = 0.0, middle = 0.0, families = 0.0, shownCount = 0.0, links = 0.0, leastLink = 0.0, copies = 0.0, altLock = 0.0;
        int n = 0, chained = 0;

        for (size_t k = 0; k < g.sounds.size(); ++k)
        {
            const auto& r = g.sounds[k];
            const auto target = prepare (r.audio, r.sampleRate);

            if (target.problem.isNotEmpty())
            {
                std::printf ("%s: %s\n", r.name.toRawUTF8(), target.problem.toRawUTF8());
                continue;
            }

            Options options;
            options.untilSettled = true;

            if (g.leaveOut[k].isNotEmpty())
                options.leaveOut.add (g.leaveOut[k]);

            Search search (target, options);
            const auto end = search.runNow();

            if (end.best.empty())
                continue;

            // Each shown candidate against every other, by the measure the search uses.
            std::vector<Measures> measures;

            for (auto& c : end.best)
                measures.push_back (measureRender (render (renderer, c.patch, target)));

            std::vector<float> apart, nearestOther (end.best.size(), 1000.0f);
            std::set<juce::String> from;
            juce::StringArray shown;

            for (size_t i = 0; i < end.best.size(); ++i)
            {
                from.insert (end.best[i].from);

                for (size_t j = i + 1; j < end.best.size(); ++j)
                {
                    const auto d = distance (measures[i], measures[j]).total;
                    apart.push_back (d);
                    nearestOther[i] = juce::jmin (nearestOther[i], d);
                    nearestOther[j] = juce::jmin (nearestOther[j], d);
                }
            }

            for (size_t i = 0; i < end.best.size(); ++i)
                shown.add (juce::String (juce::roundToInt (end.best[i].lock)) + "/" + juce::String (nearestOther[i] >= 1000.0f ? 0.0f : nearestOther[i], 1));

            const auto near = (int) std::count_if (nearestOther.begin(), nearestOther.end(), [] (float d) { return d < 1.5f; });
            juce::StringArray chain;
            auto linkLeast = 1000.0f;

            for (size_t i = 0; i < end.chain.size(); ++i)
            {
                chain.add (juce::String (juce::roundToInt (end.chain[i].candidate.lock)) + (i > 0 ? "(" + juce::String (end.chain[i].movedDb, 1) + ")" : juce::String()));

                if (i > 0)
                    linkLeast = juce::jmin (linkLeast, end.chain[i].movedDb);
            }

            const auto leastApart = apart.empty() ? 0.0f : *std::min_element (apart.begin(), apart.end());
            std::printf ("%s (%s, %.2f s): lock %.1f %%, %.1f s, %d rounds; shown %d from %d patches, apart least %.2f median %.2f most %.2f dB, %d within 1.5 dB of another\n"
                         "    shown lock/nearest other dB: %s\n    chain %d links, lock(dB from the last): %s\n",
                         r.name.toRawUTF8(), target.pitchHz > 0.0 ? noteName (target.note).toRawUTF8() : "no pitch", (double) target.audio.getNumSamples() / rate,
                         end.best.front().lock, end.seconds, end.round, (int) end.best.size(), (int) from.size(), leastApart, medianOf (apart),
                         apart.empty() ? 0.0f : *std::max_element (apart.begin(), apart.end()), near, shown.joinIntoString (" ").toRawUTF8(),
                         (int) end.chain.size(), chain.joinIntoString (" ").toRawUTF8());
            std::fflush (stdout);
            ++n;
            lock += end.best.front().lock;
            seconds += end.seconds;
            least += leastApart;
            middle += medianOf (apart);
            families += (double) from.size();
            shownCount += (double) end.best.size();
            links += (double) end.chain.size();
            copies += near;
            altLock += end.best.size() > 1 ? end.best[1].lock : end.best.front().lock;

            if (end.chain.size() > 1)
            {
                leastLink += linkLeast;
                ++chained;
            }
        }

        if (n > 0)
            std::printf ("VARIETY %s, %d sounds: mean lock %.1f %% (the second shown %.1f %%), %.1f s; shown %.1f from %.1f patches, apart least %.2f dB and median %.2f dB on average, "
                         "%.1f of them within 1.5 dB of another; chain %.1f links, the least between two links %.2f dB on average\n",
                         g.name.toRawUTF8(), n, lock / n, altLock / n, seconds / n, shownCount / n, families / n, least / n, middle / n, copies / n, links / n,
                         chained > 0 ? leastLink / chained : 0.0);
    }

    return 0;
}

void testDecryptVariety()
{
    // What Decrypt shows must be choices that sound different, on sounds with a pitch and without: the candidates
    // shown each shownApartDb or more from every other (rendered again here and measured, not as the search kept
    // them), from several factory patches; the chain's neighbours linkApartDb or more apart and each clearly nearer.
    std::vector<std::pair<Recording, juce::String>> sounds;

    for (auto& r : unpitched())
        if (r.name != "noise sweep")    // three seconds: left to --decrypt-variety
            sounds.push_back ({ r, {} });

    sounds.push_back ({ wet ("Abyss Reese", 45, 0.8, 1.5, false), "Abyss Reese" });
    sounds.push_back ({ wet ("Foundry Knell", 69, 0.8, 1.5, false), "Foundry Knell" });
    juce::StringArray faults, said;
    Synth renderer;
    auto leastShown = 1000.0f, leastLink = 1000.0f;
    int fewestShown = 1000, fewestFrom = 1000, noPitch = 0;

    for (auto& [r, leaveOut] : sounds)
    {
        const auto target = prepare (r.audio, r.sampleRate);
        Options options;
        options.untilSettled = true;

        if (leaveOut.isNotEmpty())
            options.leaveOut.add (leaveOut);

        Search search (target, options);
        const auto end = search.runNow();
        noPitch += target.pitchHz > 0.0 ? 0 : 1;
        std::vector<Measures> measures;
        std::set<juce::String> from;

        for (auto& c : end.best)
        {
            measures.push_back (measureRender (render (renderer, c.patch, target)));
            from.insert (c.from);
        }

        auto least = 1000.0f;

        for (size_t i = 0; i < measures.size(); ++i)
            for (size_t j = i + 1; j < measures.size(); ++j)
                least = juce::jmin (least, distance (measures[i], measures[j]).total);

        auto link = 1000.0f;
        auto nearer = true, sameLock = false;

        for (size_t i = 1; i < end.chain.size(); ++i)
        {
            link = juce::jmin (link, end.chain[i].movedDb);
            nearer = nearer && (i + 1 == end.chain.size() || end.chain[i - 1].candidate.distance.total - end.chain[i].candidate.distance.total >= linkNearerDb - 0.001f);
            sameLock = sameLock || juce::roundToInt (end.chain[i].candidate.lock) == juce::roundToInt (end.chain[i - 1].candidate.lock);
        }

        if (end.best.size() < 3)                faults.add (r.name + ": only " + juce::String ((int) end.best.size()) + " shown");
        if (from.size() < 3)                    faults.add (r.name + ": shown from only " + juce::String ((int) from.size()) + " factory patches");
        if (least < shownApartDb - 0.05f)       faults.add (r.name + ": two shown are " + juce::String (least, 2) + " dB apart");
        if (link < linkApartDb - 0.05f)         faults.add (r.name + ": two links are " + juce::String (link, 2) + " dB apart");
        if (! nearer)                           faults.add (r.name + ": a link is not " + juce::String (linkNearerDb, 1) + " dB nearer than the one before");
        if (sameLock)                           faults.add (r.name + ": two neighbouring links have the same lock");
        if (end.chain.empty())                  faults.add (r.name + ": no chain");

        leastShown = juce::jmin (leastShown, least);
        leastLink = juce::jmin (leastLink, link);
        fewestShown = juce::jmin (fewestShown, (int) end.best.size());
        fewestFrom = juce::jmin (fewestFrom, (int) from.size());
        said.add (r.name + ": lock " + juce::String (juce::roundToInt (end.best.empty() ? 0.0f : end.best.front().lock)) + " %, " + juce::String ((int) end.best.size()) + " shown, "
                  + juce::String ((int) end.chain.size()) + " links");
    }

    TestUtils::check (faults.isEmpty() && noPitch >= 2,
                      "decrypt: on " + juce::String ((int) sounds.size()) + " sounds (" + juce::String (noPitch) + " with no pitch to find) the candidates shown are choices that sound different: at least "
                      + juce::String (fewestShown) + " shown from at least " + juce::String (fewestFrom) + " factory patches, no two nearer than " + juce::String (leastShown, 2) + " dB to each other by Decrypt's measure ("
                      + juce::String (shownApartDb, 1) + " asked), and no two neighbouring links of a chain nearer than " + juce::String (leastLink >= 1000.0f ? 0.0f : leastLink, 2) + " dB (" + juce::String (linkApartDb, 1)
                      + " asked), each link clearly nearer the signal than the one before (" + said.joinIntoString ("; ") + ")"
                      + (faults.isEmpty() ? juce::String() : " (failed: " + faults.joinIntoString ("; ") + ")"));
}
