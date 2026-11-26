#include <JuceHeader.h>
#include <thread>
#include "../Source/BuiltInEffects.h"
#include "../Source/Decrypt.h"
#include "../Source/Loudness.h"
#include "../Source/WaveSynth.h"
#include "ClickCheck.h"
#include "TestUtils.h"
#include "WaveSynthTests.h"

// The Complex patches held to what a club needs of them (docs/plans/PLAN-SYNTH-COMPLEX.md): four
// oscillators or more and nine in each category's own, the low end mono, the level of the others, the
// weight and the cut where they belong, no clicks, eight notes at once, macros that do something, and
// far enough from each other and from the factory patches to be sounds of their own.

namespace
{
    using TestUtils::check;
    using Synth = BuiltInEffects::BasicSynthProcessor;
    constexpr double rate = 48000.0;

    struct Event { int frame; juce::MidiMessage message; };

    juce::AudioBuffer<float> render (Synth& s, std::vector<Event> events, double seconds, int blockSize = 512)
    {
        s.setPlayConfigDetails (0, 2, rate, blockSize);
        s.setNonRealtime (true);
        s.prepareToPlay (rate, blockSize);
        std::stable_sort (events.begin(), events.end(), [] (const Event& a, const Event& b)
        {
            return a.frame < b.frame || (a.frame == b.frame && a.message.isNoteOff() && ! b.message.isNoteOff());
        });

        const auto length = (int) (seconds * rate);
        juce::AudioBuffer<float> out (2, length), b (2, blockSize);
        size_t next = 0;

        for (int at = 0; at < length; at += blockSize)
        {
            const auto n = juce::jmin (blockSize, length - at);
            juce::MidiBuffer midi;

            for (; next < events.size() && events[next].frame < at + n; ++next)
                midi.addEvent (events[next].message, juce::jmax (0, events[next].frame - at));

            b.setSize (2, n, false, false, true);
            b.clear();
            s.processBlock (b, midi);

            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, at, b, ch, 0, n);
        }

        return out;
    }

    std::vector<Event> notes (std::initializer_list<int> keys, double from, double seconds, float velocity)
    {
        std::vector<Event> e;

        for (auto k : keys)
        {
            e.push_back ({ (int) (from * rate), juce::MidiMessage::noteOn (1, k, velocity) });
            e.push_back ({ (int) ((from + seconds) * rate), juce::MidiMessage::noteOff (1, k) });
        }

        return e;
    }

    /** The key a category's patches are measured at: where its phrase in the clips plays. */
    int keyFor (const juce::String& category)
    {
        if (category == "Bass" || category == "Sub" || category == "Reese" || category == "Impact")                 return 33;
        if (category == "Acid" || category == "Guitar" || category == "Drone" || category == "Texture"
            || category == "Cinematic" || category == "Sequence")                                                    return 45;
        if (category == "Drum")                                                                                       return 50;
        if (category == "Lead" || category == "Pluck" || category == "Bell" || category == "Chip" || category == "Vox") return 69;
        return 57;
    }

    bool isLow (const juce::String& c)    { return c == "Bass" || c == "Sub" || c == "Reese" || c == "Acid"; }
    bool isHook (const juce::String& c)   { return c == "Lead" || c == "Stab" || c == "Pluck" || c == "Arp" || c == "Sequence" || c == "Acid"; }

    /** The oscillators a patch plays and those it runs only for another's FM, and the unison a note asks for. */
    struct Used { int on = 0, modulators = 0, unison = 0; };

    Used usedBy (const WaveSynth::Patch& patch)
    {
        const auto value = [&patch] (const juce::String& id, double otherwise)
        {
            const auto found = patch.values.find (id);
            return found != patch.values.end() ? found->second : otherwise;
        };
        const auto field = [] (int o, const char* f) { return "osc" + juce::String (o) + f; };
        std::array<int, WaveSynth::numOscillators + 1> role {};   // 0 unused, 1 on, 2 modulator

        for (int o = 1; o <= WaveSynth::numOscillators; ++o)
            role[(size_t) o] = value (field (o, "On"), o == 1 ? 1.0 : 0.0) > 0.5 ? 1 : 0;

        for (bool grown = true; grown;)
        {
            grown = false;

            for (int o = 1; o <= WaveSynth::numOscillators; ++o)
                if (role[(size_t) o] != 0 && (int) value (field (o, "Warp"), 0.0) == (int) WaveSynth::Warp::fm)
                    if (const auto from = (int) value (field (o, "FmFrom"), 0.0); from > 0 && role[(size_t) from] == 0)
                    {
                        role[(size_t) from] = 2;
                        grown = true;
                    }
        }

        Used u;

        for (int o = 1; o <= WaveSynth::numOscillators; ++o)
            if (role[(size_t) o] != 0)
            {
                (role[(size_t) o] == 1 ? u.on : u.modulators) += 1;
                u.unison += (int) value (field (o, "Unison"), 1.0);
            }

        return u;
    }

    //==========================================================================
    // Filters for the measures: an RBJ biquad, two in a row.
    std::vector<float> pass (const std::vector<float>& x, double hz, bool high)
    {
        const auto w = juce::MathConstants<double>::twoPi * hz / rate, c = std::cos (w), a = std::sin (w) / (2.0 * 0.7071);
        const auto b0 = high ? (1 + c) / 2 : (1 - c) / 2, b1 = high ? -(1 + c) : 1 - c, a0 = 1 + a, a1 = -2 * c, a2 = 1 - a;
        auto y = x;

        for (int twice = 0; twice < 2; ++twice)
        {
            double x1 = 0, x2 = 0, y1 = 0, y2 = 0;

            for (auto& v : y)
            {
                const auto in = (double) v, out = (b0 * in + b1 * x1 + b0 * x2 - a1 * y1 - a2 * y2) / a0;
                x2 = x1; x1 = in; y2 = y1; y1 = out;
                v = (float) out;
            }
        }

        return y;
    }

    double meanSquare (const std::vector<float>& v, size_t from = 0, size_t to = SIZE_MAX)
    {
        to = juce::jmin (to, v.size());
        double sum = 0;

        for (auto i = from; i < to; ++i)
            sum += (double) v[i] * v[i];

        return to > from ? sum / (double) (to - from) : 0.0;
    }

    double db (double ratio)    { return 10.0 * std::log10 (juce::jmax (1.0e-20, ratio)); }

    /** What one note says about a patch. */
    struct Note
    {
        double lowSideDb = -200.0;   // the side below 120 Hz against all the note's power
        double below200 = 0.0;       // % of its power below 200 Hz
        double cut = 0.0;            // % of its first 150 ms from 1 to 5 kHz
        double dcDb = -200.0;        // its mean against its RMS
    };

    Note measureNote (const juce::AudioBuffer<float>& out)
    {
        const auto n = (size_t) out.getNumSamples();
        std::vector<float> mid (n), side (n);

        for (size_t i = 0; i < n; ++i)
        {
            mid[i] = 0.5f * (out.getSample (0, (int) i) + out.getSample (1, (int) i));
            side[i] = 0.5f * (out.getSample (0, (int) i) - out.getSample (1, (int) i));
        }

        const auto all = meanSquare (mid) + meanSquare (side);
        Note note;
        note.lowSideDb = db (meanSquare (pass (side, 120.0, false)) / all);
        note.below200 = 100.0 * (meanSquare (pass (mid, 200.0, false)) + meanSquare (pass (side, 200.0, false))) / juce::jmax (1.0e-20, all);

        const auto start = (size_t) Decrypt::Measures::hop * 0, early = (size_t) (0.15 * rate);
        const auto band = pass (pass (mid, 1000.0, true), 5000.0, false);
        note.cut = 100.0 * meanSquare (band, start, early) / juce::jmax (1.0e-20, meanSquare (mid, start, early));

        double sum = 0;
        for (auto v : mid) sum += v;
        note.dcDb = db (((sum / (double) n) * (sum / (double) n)) / juce::jmax (1.0e-20, meanSquare (mid)));
        return note;
    }

    /** A render's spectrum 10 ms at a time: 40 bands from 40 Hz to 16 kHz, each frame's shape (in dB
        under its loudest band, down to -40) and its level in dB. */
    struct Frames
    {
        static constexpr int bands = 40, hop = 480;
        std::vector<std::array<float, bands>> shape;
        std::vector<float> level;
    };

    Frames framesOf (const juce::AudioBuffer<float>& b)
    {
        constexpr int order = 11, size = 1 << order;
        juce::dsp::FFT fft (order);
        std::vector<float> window ((size_t) size), data ((size_t) (2 * size));

        for (int i = 0; i < size; ++i)
            window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) size);

        std::array<int, Frames::bands + 1> edges {};
        for (int k = 0; k <= Frames::bands; ++k)
            edges[(size_t) k] = juce::jlimit (1, size / 2 - 1, (int) std::round (40.0 * std::pow (400.0, (double) k / Frames::bands) * size / rate));

        Frames f;

        for (int at = 0; at + size <= b.getNumSamples(); at += Frames::hop)
        {
            std::fill (data.begin(), data.end(), 0.0f);
            for (int i = 0; i < size; ++i)
                data[(size_t) i] = 0.5f * (b.getSample (0, at + i) + b.getSample (1, at + i)) * window[(size_t) i];

            fft.performFrequencyOnlyForwardTransform (data.data());
            std::array<float, Frames::bands> shape {};
            double total = 0.0;

            for (int k = 0; k < Frames::bands; ++k)
            {
                double e = 0.0;
                for (int i = edges[(size_t) k]; i < juce::jmax (edges[(size_t) k] + 1, edges[(size_t) k + 1]); ++i)
                    e += (double) data[(size_t) i] * data[(size_t) i];

                total += e;
                shape[(size_t) k] = (float) (10.0 * std::log10 (e + 1.0e-20));
            }

            const auto top = *std::max_element (shape.begin(), shape.end());
            for (auto& v : shape) v = juce::jmax (-40.0f, v - top);
            f.shape.push_back (shape);
            f.level.push_back ((float) (10.0 * std::log10 (total + 1.0e-20)));
        }

        return f;
    }

    /** How a held note moves, from frame `from` to `to`: fast, its shape across a sixteenth at 132
        BPM (dB a band, on average); slow, its first bar's average shape against its last bar's; level,
        its level across a sixteenth (dB). Frames 40 dB under the loudest are left out. */
    struct Movement { double fast = 0.0, slow = 0.0, level = 0.0; };

    Movement movementOf (const Frames& f, int from, int to)
    {
        constexpr int sixteenth = 11, bar = 182;
        to = juce::jmin (to, (int) f.shape.size());
        Movement m;

        if (to - from < 2 * bar)
            return m;

        const auto loudest = *std::max_element (f.level.begin() + from, f.level.begin() + to);
        const auto apart = [&f] (int a, int b)
        {
            double sum = 0.0;
            for (int k = 0; k < Frames::bands; ++k) sum += std::abs (f.shape[(size_t) a][(size_t) k] - f.shape[(size_t) b][(size_t) k]);
            return sum / Frames::bands;
        };

        double fast = 0.0, level = 0.0;
        int counted = 0;

        for (int i = from; i + sixteenth < to; ++i)
            if (f.level[(size_t) i] > loudest - 40.0f || f.level[(size_t) (i + sixteenth)] > loudest - 40.0f)
            {
                fast += apart (i, i + sixteenth);
                level += std::abs (juce::jmax (loudest - 40.0f, f.level[(size_t) i]) - juce::jmax (loudest - 40.0f, f.level[(size_t) (i + sixteenth)]));
                ++counted;
            }

        std::array<double, Frames::bands> first {}, last {};
        for (int i = 0; i < bar; ++i)
            for (int k = 0; k < Frames::bands; ++k)
            {
                first[(size_t) k] += f.shape[(size_t) (from + i)][(size_t) k] / (double) bar;
                last[(size_t) k] += f.shape[(size_t) (to - bar + i)][(size_t) k] / (double) bar;
            }

        for (int k = 0; k < Frames::bands; ++k)
            m.slow += std::abs (first[(size_t) k] - last[(size_t) k]) / Frames::bands;

        m.fast = counted > 0 ? fast / counted : 0.0;
        m.level = counted > 0 ? level / counted : 0.0;
        return m;
    }

    /** The worst burst in a render: above 3 kHz, a millisecond louder by this many dB than anything
        from 2 to 30 ms before it and from 5 to 30 ms after it, and no more than 50 dB under the
        render's loudest millisecond there; a click the ear hears that no single-sample step shows. */
    struct Burst { double db = 0.0, seconds = 0.0; };

    Burst burstOf (const juce::AudioBuffer<float>& out)
    {
        const auto n = (size_t) out.getNumSamples();
        std::vector<float> mid (n);
        for (size_t i = 0; i < n; ++i)
            mid[i] = 0.5f * (out.getSample (0, (int) i) + out.getSample (1, (int) i));

        const auto high = pass (mid, 3000.0, true);
        std::vector<float> env (n, 0.0f);
        double sum = 0.0;

        for (size_t i = 0; i < n; ++i)
        {
            sum += (double) high[i] * high[i];
            if (i >= 48) sum -= (double) high[i - 48] * high[i - 48];
            env[i] = (float) std::sqrt (juce::jmax (0.0, sum) / 48.0);
        }

        const auto loudest = *std::max_element (env.begin(), env.end());
        Burst worst;

        for (size_t i = 1440; i + 1440 < n; i += 12)
        {
            if (env[i] < loudest * 0.00316f)
                continue;

            float around = 1.0e-9f;
            for (size_t j = i - 1440; j < i - 96; j += 12) around = juce::jmax (around, env[j]);
            for (size_t j = i + 240; j < i + 1440; j += 12) around = juce::jmax (around, env[j]);
            if (const auto db = 20.0 * std::log10 ((double) env[i] / around); db > worst.db)
                worst = { db, (double) i / rate };
        }

        return worst;
    }
}

void runComplexPatchTests()
{
    const auto& complex = WaveSynth::complexPatches();
    const auto& factory = WaveSynth::factoryPatches();

    //==========================================================================
    // What the list holds.
    juce::StringArray factoryNames, names, fewer, tooMuchUnison;
    std::map<juce::String, int> nines, inCategory;

    for (auto& p : factory)
        factoryNames.add (p.name.toLowerCase());

    for (auto& p : complex)
    {
        const auto u = usedBy (p);

        if (u.on + u.modulators < 4)   fewer.add (p.name);
        if (u.unison > 48)             tooMuchUnison.add (p.name + " " + juce::String (u.unison));
        if (u.on + u.modulators == 9)  ++nines[p.category];

        ++inCategory[p.category];
        names.add (p.name.toLowerCase());
    }

    juce::StringArray clashes, noNine, missing;

    for (int i = 0; i < names.size(); ++i)
        if (names.indexOf (names[i]) != i || factoryNames.contains (names[i]))
            clashes.add (names[i]);

    for (auto& c : WaveSynth::patchCategories())
        if (c != "Init")
        {
            if (inCategory[c] == 0)   missing.add (c);
            if (nines[c] < 1)         noNine.add (c);
        }

    check (complex.size() >= 58 && missing.isEmpty() && fewer.isEmpty() && noNine.isEmpty(),
           "synth complex: " + juce::String ((int) complex.size()) + " patches in all 29 categories, each using four oscillators or more, and all nine in at least one a category"
               + (missing.isEmpty() ? juce::String() : " (none in " + missing.joinIntoString (", ") + ")")
               + (fewer.isEmpty() ? juce::String() : " (fewer than four: " + fewer.joinIntoString (", ") + ")")
               + (noNine.isEmpty() ? juce::String() : " (no nine in " + noNine.joinIntoString (", ") + ")"));
    check (tooMuchUnison.isEmpty(), "synth complex: no patch asks for more than 48 unison voices a note, so eight notes sound at once"
                                        + (tooMuchUnison.isEmpty() ? juce::String() : " (" + tooMuchUnison.joinIntoString ("; ") + ")"));
    check (clashes.isEmpty(), "synth complex: every name is its own, none a factory patch's" + (clashes.isEmpty() ? juce::String() : " (" + clashes.joinIntoString (", ") + ")"));

    //==========================================================================
    // The folder: browsed, the arrows go through the Complex patches in order and round the ends;
    // browsing the favourites takes them back to the factory list.
    {
        Synth s;
        s.loadPatch (complex.front().name);
        s.setBrowsingComplex (true);
        const auto forward = s.stepPatch (1) == -1 && s.getPatchName() == complex[1].name;
        s.stepPatch (-1);
        const auto round = s.stepPatch (-1) == -1 && s.getPatchName() == complex.back().name && s.stepPatch (1) == -1 && s.getPatchName() == complex.front().name;
        s.setBrowsingFavourites (true);
        const auto left = ! s.isBrowsingComplex();
        check (forward && round && left, "synth complex: browsed, the arrows step through the Complex patches and round their ends, and leave them for the favourites");
    }

    //==========================================================================
    // Each patch played: one note at its category's key, a chord for the level check, a struck and
    // restruck note for clicks, eight notes at once, each macro at full, a note at C3 to compare with
    // the factory patches, and its state saved and loaded back. Eight at a time.
    struct Result
    {
        Note note;
        double chordPeakDb = -200.0, momentary = -200.0, fullPeakDb = -200.0;
        Decrypt::Measures atC3, atKey;
        ClickCheck::Click click;
        int sounding = 0;
        std::array<float, WaveSynth::numMacros> macroMoves {};
        bool roundTrip = false, blockSizes = false;
        Movement movement;
        double velocityApart = 0.0;
        Burst burst, noteBurst;
    };

    std::vector<Result> results (complex.size());
    {
        std::atomic<size_t> next { 0 };
        std::vector<std::thread> threads;

        for (int t = 0; t < 8; ++t)
            threads.emplace_back ([&]
            {
                for (auto i = next++; i < complex.size(); i = next++)
                {
                    const auto& patch = complex[i];
                    auto& r = results[i];
                    const auto key = keyFor (patch.category);
                    const auto load = [&patch] (Synth& s) { s.loadPatch (patch.name); };

                    Synth one;
                    load (one);
                    const auto note = render (one, notes ({ key }, 0.0, 1.5, 0.9f), 3.5);
                    r.note = measureNote (note);
                    r.momentary = Loudness::measure (note, rate).maxMomentary;
                    r.atKey = Decrypt::measureRender (note);

                    Synth chord;
                    load (chord);
                    const auto c = render (chord, notes ({ 48, 51, 55 }, 0.0, 2.0, 0.8f), 4.0);
                    r.chordPeakDb = juce::Decibels::gainToDecibels (juce::jmax (c.getMagnitude (0, 0, c.getNumSamples()), c.getMagnitude (1, 0, c.getNumSamples())), -200.0f);

                    // Struck, struck again before it has gone, let go, and a steal: no step anywhere.
                    Synth struck;
                    load (struck);
                    auto events = notes ({ key }, 0.0, 0.4, 0.9f);
                    for (auto& e : notes ({ key }, 0.45, 0.6, 0.6f)) events.push_back (e);
                    for (auto& e : notes ({ key + 7 }, 0.6, 0.3, 1.0f)) events.push_back (e);
                    const auto s = render (struck, events, 2.5);
                    r.click = ClickCheck::worst (s, rate, 0.03f);

                    // Eight notes at once at full velocity: all sound, and none clips.
                    Synth eight;
                    load (eight);
                    const auto e8 = render (eight, notes ({ key, key + 3, key + 7, key + 10, key + 12, key + 15, key + 19, key + 24 }, 0.0, 1.0, 1.0f), 0.5);
                    r.sounding = eight.getNumSounding();
                    r.fullPeakDb = juce::Decibels::gainToDecibels (juce::jmax (e8.getMagnitude (0, 0, e8.getNumSamples()), e8.getMagnitude (1, 0, e8.getNumSamples())), -200.0f);

                    // Each macro at full against at rest: how far the sound moves, by Decrypt's measure.
                    for (int m = 0; m < WaveSynth::numMacros; ++m)
                    {
                        Synth moved;
                        load (moved);
                        auto* p = moved.parameter ("macro" + juce::String (m + 1));
                        p->setValueNotifyingHost (p->getValue() < 0.5f ? 1.0f : 0.0f);
                        r.macroMoves[(size_t) m] = Decrypt::distance (r.atKey, Decrypt::measureRender (render (moved, notes ({ key }, 0.0, 1.5, 0.9f), 3.5))).total;
                    }

                    // Held for four bars at 132 BPM: how it moves. Soft against hard: how far apart, past level.
                    Synth held;
                    load (held);
                    const auto h = render (held, notes ({ key }, 0.0, 7.27, 0.9f), 7.4);
                    r.movement = movementOf (framesOf (h), 25, 727 - 2048 / Frames::hop);

                    Synth soft, hard;
                    load (soft);
                    load (hard);
                    r.velocityApart = Decrypt::distance (Decrypt::measureRender (render (soft, notes ({ key }, 0.0, 1.5, 0.3f), 3.5)),
                                                         Decrypt::measureRender (render (hard, notes ({ key }, 0.0, 1.5, 1.0f), 3.5))).total;
                    r.burst = burstOf (s);
                    r.noteBurst = burstOf (note);

                    Synth atC3;
                    load (atC3);
                    r.atC3 = Decrypt::measureRender (render (atC3, notes ({ 48 }, 0.0, 1.5, 0.9f), 3.0));

                    // Saved and loaded back on a new synth, it plays the same; and the same in blocks of 512 and 37.
                    juce::MemoryBlock state;
                    one.getStateInformation (state);
                    Synth back;
                    back.setStateInformation (state.getData(), (int) state.getSize());
                    const auto again = render (back, notes ({ key }, 0.0, 1.5, 0.9f), 3.5);
                    Synth odd;
                    load (odd);
                    const auto inOdd = render (odd, notes ({ key }, 0.0, 1.5, 0.9f), 3.5, 37);
                    const auto same = [] (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
                    {
                        for (int ch = 0; ch < 2; ++ch)
                            if (std::memcmp (a.getReadPointer (ch), b.getReadPointer (ch), sizeof (float) * (size_t) a.getNumSamples()) != 0)
                                return false;
                        return true;
                    };
                    r.roundTrip = same (note, again);
                    r.blockSizes = same (note, inOdd);
                }
            });

        for (auto& t : threads)
            t.join();
    }

    // The factory patches at C3, measured once here, for the distance from each.
    std::vector<Decrypt::Measures> factoryAtC3 (factory.size());
    {
        std::atomic<size_t> next { 0 };
        std::vector<std::thread> threads;

        for (int t = 0; t < 8; ++t)
            threads.emplace_back ([&]
            {
                for (auto i = next++; i < factory.size(); i = next++)
                {
                    Synth s;
                    s.loadPatch ((int) i);
                    factoryAtC3[i] = Decrypt::measureRender (render (s, notes ({ 48 }, 0.0, 1.5, 0.9f), 3.0));
                }
            });

        for (auto& t : threads)
            t.join();
    }

    // The factory patches at each key a category is measured at, for the distance where each is played.
    std::map<int, std::vector<Decrypt::Measures>> factoryAtKey;
    for (auto& p : complex)
        factoryAtKey[keyFor (p.category)];

    for (auto& [key, measures] : factoryAtKey)
    {
        measures.resize (factory.size());
        std::atomic<size_t> next { 0 };
        std::vector<std::thread> threads;

        for (int t = 0; t < 8; ++t)
            threads.emplace_back ([&, k = key]
            {
                for (auto i = next++; i < factory.size(); i = next++)
                {
                    Synth s;
                    s.loadPatch ((int) i);
                    measures[i] = Decrypt::measureRender (render (s, notes ({ k }, 0.0, 1.5, 0.9f), 3.5));
                }
            });

        for (auto& t : threads)
            t.join();
    }

    for (size_t i = 0; i < complex.size(); ++i)
    {
        const auto& r = results[i];
        const auto& atKey = factoryAtKey[keyFor (complex[i].category)];
        std::vector<std::pair<float, juce::String>> near;

        for (size_t f = 0; f < factory.size(); ++f)
            near.push_back ({ Decrypt::distance (r.atKey, atKey[f]).total, factory[f].name });

        std::partial_sort (near.begin(), near.begin() + 3, near.end());
        std::cout << "CALIB|" << complex[i].name << "|" << complex[i].category << "|fast " << juce::String (r.movement.fast, 2) << "|slow " << juce::String (r.movement.slow, 2)
                  << "|level " << juce::String (r.movement.level, 2) << "|velocity " << juce::String (r.velocityApart, 2) << "|burst " << juce::String (r.burst.db, 1) << " at " << juce::String (r.burst.seconds, 3) << " note " << juce::String (r.noteBurst.db, 1) << " at " << juce::String (r.noteBurst.seconds, 3)
                  << "|atKey " << juce::String (near[0].first, 2) << " " << near[0].second << ", " << juce::String (near[1].first, 2) << " " << near[1].second
                  << ", " << juce::String (near[2].first, 2) << " " << near[2].second << std::endl;
    }

    // How far a factory patch is from its nearest other, every twelfth one against all the rest: its
    // tenth percentile is the least a Complex patch may be from any factory patch and from its sibling.
    float floorDb = 0.0f;
    {
        std::vector<float> nearestOther;

        for (size_t i = 0; i < factory.size(); i += 12)
        {
            float nearest = 1000.0f;

            for (size_t f = 0; f < factory.size(); ++f)
                if (f != i)
                    nearest = juce::jmin (nearest, Decrypt::distance (factoryAtC3[i], factoryAtC3[f]).total);

            nearestOther.push_back (nearest);
        }

        std::sort (nearestOther.begin(), nearestOther.end());
        floorDb = nearestOther[(size_t) (0.1 * (double) (nearestOther.size() - 1))];
    }

    //==========================================================================
    // The measures, each held to its bound; a patch that misses one is named with all its measures.
    std::map<juce::String, juce::StringArray> misses;

    for (size_t i = 0; i < complex.size(); ++i)
    {
        const auto& r = results[i];
        const auto& p = complex[i];
        const auto low = isLow (p.category) || p.category == "Impact";
        const auto mono = p.values.count ("voiceMode") != 0 && p.values.at ("voiceMode") > 0.5;
        float nearest = 1000.0f, sibling = 1000.0f;
        juce::String nearestName, siblingName;

        for (size_t f = 0; f < factory.size(); ++f)
            if (const auto d = Decrypt::distance (r.atC3, factoryAtC3[f]).total; d < nearest) { nearest = d; nearestName = factory[f].name; }

        for (size_t j = 0; j < complex.size(); ++j)
            if (j != i && complex[j].category == p.category)
                if (const auto d = Decrypt::distance (r.atC3, results[j].atC3).total; d < sibling) { sibling = d; siblingName = complex[j].name; }

        const auto miss = [&] (bool ok, const char* what) { if (! ok) misses[what].add (p.name); return ok; };
        bool ok = true;
        ok &= miss (r.note.lowSideDb <= -20.0, "the side below 120 Hz at -20 dB or under");
        ok &= miss (! low || r.note.below200 >= 40.0, "a low patch with 40 % or more of its power below 200 Hz");
        ok &= miss (! (p.category == "Lead" || p.category == "Stab") || r.note.below200 <= 25.0, "a lead or stab with 25 % or less below 200 Hz");
        ok &= miss (! isHook (p.category) || r.note.cut >= 3.0, "a hook with 3 % or more of its first 150 ms from 1 to 5 kHz");
        ok &= miss (r.note.dcDb <= -30.0, "no offset (mean at -30 dB under the RMS or less)");
        ok &= miss (r.chordPeakDb >= -24.0 && r.chordPeakDb <= -1.0, "a three-note chord peaking from -24 to -1 dBFS");
        ok &= miss (r.momentary >= -28.0 && r.momentary <= -9.0, "a note's loudest moment from -28 to -9 LUFS");
        ok &= miss (r.click.ratio <= 4.0f, "no clicks when struck, restruck or stolen");
        ok &= miss (mono ? r.sounding >= 1 : r.sounding == 8, "eight notes sounding at once (one in a mono patch)");
        ok &= miss (r.fullPeakDb <= 6.0, "eight notes at full velocity peaking at +6 dBFS or under");
        ok &= miss (*std::min_element (r.macroMoves.begin(), r.macroMoves.end()) >= 0.3f, "each macro moving the sound 0.3 dB or more");
        ok &= miss (nearest >= floorDb, "as far from every factory patch as a factory patch's tenth-percentile nearest");
        ok &= miss (sibling >= floorDb, "as far from the other patches of its category");
        ok &= miss (r.roundTrip && r.blockSizes, "the same after its state is loaded back, and in blocks of 37");

        if (! ok)
        {
            juce::StringArray moves;
            for (auto m : r.macroMoves) moves.add (juce::String (m, 1));

            std::cout << "  " << p.name << " (" << p.category << "): side " << juce::String (r.note.lowSideDb, 1) << " dB, below 200 " << juce::String (r.note.below200, 0)
                      << " %, cut " << juce::String (r.note.cut, 1) << " %, dc " << juce::String (r.note.dcDb, 0) << " dB, chord " << juce::String (r.chordPeakDb, 1)
                      << " dBFS, momentary " << juce::String (r.momentary, 1) << " LUFS, click " << juce::String (r.click.ratio, 1) << ", sounding " << r.sounding
                      << ", eight at full " << juce::String (r.fullPeakDb, 1) << " dBFS, macros " << moves.joinIntoString (" ") << ", factory " << juce::String (nearest, 2)
                      << " (" << nearestName << "), sibling " << juce::String (sibling, 2) << " (" << siblingName << ")" << std::endl;
        }
    }

    juce::StringArray failed;
    for (auto& [what, who] : misses)
        failed.add (juce::String (what) + ": " + who.joinIntoString (", "));

    check (failed.isEmpty(), "synth complex: all " + juce::String ((int) complex.size()) + " patches mono below 120 Hz, at level, the weight and cut where they belong, no clicks, "
                                 "eight notes, macros that move them, and at least " + juce::String (floorDb, 2) + " dB from every factory patch and each other"
                                 + (failed.isEmpty() ? juce::String() : " (" + failed.joinIntoString ("; ") + ")"));
}
