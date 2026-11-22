#include <JuceHeader.h>
#include "../Source/BuiltInEffects.h"
#include "../Source/WaveSynth.h"
#include "../Source/WaveSynthEditor.h"
#include "../Source/WaveSynthPatchBuilder.h"
#include "ClickCheck.h"
#include "TestUtils.h"
#include "WaveSynthBankTests.h"

// The factory patches as a set. What is measured here is also what Scripts/make_patch_bank.py
// chose the second bank's patches by (TorqwaveTests --patch-measure), so the two agree.

namespace
{
    using TestUtils::check;
    using Synth = BuiltInEffects::BasicSynthProcessor;
    constexpr double rate = 48000.0;

    // The limits the second bank's patches are held to.
    constexpr float silentDb = -30.0f;        // a note in the category's register peaks above this
    constexpr float clickFloor = 0.03f;       // a click is a lone step of this or more (Tests/ClickCheck.h)...
    constexpr float clickRatio = 4.0f;        // ...this many times any step in the 30 ms either side
    constexpr float macroDb = 1.0f;           // a macro moves some band or stretch of the sound by this much
    constexpr float nearCopy = 3.0f;          // two patches of a category differ by this much (see distance)

    //==========================================================================
    /** The note a category's patches are played at to be measured: about where each is used. */
    int noteFor (const juce::String& category)
    {
        static const std::map<juce::String, int> notes {
            { "Sub", 33 }, { "Bass", 36 }, { "Reese", 36 }, { "Acid", 36 }, { "Drone", 36 }, { "Impact", 36 }, { "Cinematic", 36 },
            { "Guitar", 40 }, { "Brass", 55 }, { "Lead", 60 }, { "Pluck", 60 }, { "Keys", 60 }, { "Organ", 60 }, { "Strings", 60 },
            { "FX", 60 }, { "Perc", 60 }, { "Chip", 60 }, { "Bell", 72 }, { "Game", 72 } };
        const auto found = notes.find (category);
        return found != notes.end() ? found->second : 48;
    }

    struct Event
    {
        int frame;
        juce::MidiMessage message;
    };

    /** Plays events through the synth from silence; offs before ons at a frame. */
    juce::AudioBuffer<float> play (Synth& s, std::vector<Event> events, int length, int blockSize = 512)
    {
        s.setPlayConfigDetails (0, 2, rate, blockSize);
        s.setNonRealtime (true);
        s.prepareToPlay (rate, blockSize);
        std::stable_sort (events.begin(), events.end(), [] (const Event& a, const Event& b)
        {
            return a.frame < b.frame || (a.frame == b.frame && a.message.isNoteOff() && ! b.message.isNoteOff());
        });
        juce::AudioBuffer<float> out (2, length), b (2, blockSize);
        size_t next = 0;

        for (int at = 0; at < length; at += blockSize)
        {
            const auto n = juce::jmin (blockSize, length - at);
            juce::MidiBuffer midi;

            while (next < events.size() && events[next].frame < at + n)
            {
                midi.addEvent (events[next].message, juce::jmax (0, events[next].frame - at));
                ++next;
            }

            b.setSize (2, n, false, false, true);
            b.clear();
            s.processBlock (b, midi);

            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, at, b, ch, 0, n);
        }

        return out;
    }

    int frames (double seconds)                    { return (int) std::lround (seconds * rate); }

    std::vector<Event> held (std::initializer_list<int> notes, double seconds, float velocity)
    {
        std::vector<Event> e;

        for (auto n : notes)
        {
            e.push_back ({ 0, juce::MidiMessage::noteOn (1, n, velocity) });
            e.push_back ({ frames (seconds), juce::MidiMessage::noteOff (1, n) });
        }

        return e;
    }

    /** A patch's settings on a synth: every parameter at its default, then the patch's. */
    void load (Synth& s, const WaveSynth::Patch& p)
    {
        for (auto* parameter : s.getParameters())
            parameter->setValueNotifyingHost (parameter->getDefaultValue());

        s.BuiltInEffects::BuiltInEffect::setPresetValues (p.values);
    }

    float peakDb (const juce::AudioBuffer<float>& b)
    {
        return juce::Decibels::gainToDecibels (juce::jmax (b.getMagnitude (0, 0, b.getNumSamples()), b.getMagnitude (1, 0, b.getNumSamples())), -100.0f);
    }

    //==========================================================================
    /** The phrase a patch is checked for clicks over: its note struck, struck again twice while it
        rings, and released; seventeen notes 10 ms apart, so the last takes the first one's voice, and
        one more; all released; a note released 1.3 ms into its attack. */
    juce::AudioBuffer<float> clickPhrase (Synth& s, int note)
    {
        std::vector<Event> e;
        e.push_back ({ frames (0.0), juce::MidiMessage::noteOn (1, note, 1.0f) });
        e.push_back ({ frames (0.25), juce::MidiMessage::noteOn (1, note, 1.0f) });
        e.push_back ({ frames (0.5), juce::MidiMessage::noteOn (1, note, 0.6f) });
        e.push_back ({ frames (0.75), juce::MidiMessage::noteOff (1, note) });

        for (int i = 0; i < 17; ++i)
            e.push_back ({ frames (0.85 + 0.01 * i), juce::MidiMessage::noteOn (1, note - 6 + i, 0.9f) });

        e.push_back ({ frames (1.1), juce::MidiMessage::noteOn (1, note - 7, 1.0f) });

        for (int i = -7; i < 11; ++i)
            e.push_back ({ frames (1.3), juce::MidiMessage::noteOff (1, note + i) });

        e.push_back ({ frames (1.7), juce::MidiMessage::noteOn (1, note, 1.0f) });
        e.push_back ({ frames (1.7013), juce::MidiMessage::noteOff (1, note) });
        return play (s, e, frames (2.2));
    }

    //==========================================================================
    // What a sound is told from another by: a note held for two seconds and left a second to die,
    // as 122 numbers in dB.
    //  - its level over 15 stretches of time (short ones at the start and at the release), against the loudest;
    //  - its spectrum in 24 bands from 25 Hz to 18 kHz, each against the whole, at the attack (the first
    //    150 ms), after it (to 600 ms), held (0.8 to 2 s) and released (2 to 2.6 s);
    //  - how much its level moves at ten rates from 1 to 24 Hz while it is held;
    //  - how wide it is (side against mid, held).
    // Two sounds' distance is the mean of their absolute differences (see distance). Measured on a saw
    // bass: its cutoff a third of an octave higher is 1.0 away, an octave higher 2.6, the same bass on a
    // square wave 3.8. In the first bank, half the patches have a neighbour within 3.0 in their category.
    constexpr int numBands = 24, numStretches = 15, numRates = 10;
    constexpr int numFeatures = numStretches + 4 * numBands + numRates + 1;
    using Features = std::array<float, numFeatures>;

    constexpr double featureHold = 2.0, featureLength = 3.0;
    constexpr float stretchFloor = -60.0f, bandFloor = -50.0f, otherFloor = -40.0f;   // where each kind of number is cut off

    float clipDb (double power, double reference, float lowest)
    {
        if (reference <= 0.0 || power <= 0.0)
            return lowest;

        return juce::jlimit (lowest, 0.0f, (float) (10.0 * std::log10 (power / reference)));
    }

    /** The power in each band between two times (both channels), and in the side signal. */
    void bandPowers (const juce::AudioBuffer<float>& b, double from, double to, std::array<double, numBands>& bands, double& total, double* sidePower = nullptr, double* midPower = nullptr)
    {
        constexpr int order = 12, size = 1 << order;
        thread_local juce::dsp::FFT fft (order);
        static const auto window = []
        {
            std::vector<float> w ((size_t) size);
            for (int i = 0; i < size; ++i)
                w[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) size);
            return w;
        }();
        static const auto bandOf = []
        {
            // Each bin's band: 24 bands of equal width in octaves from 25 Hz to 18 kHz (-1 outside).
            std::vector<int> of ((size_t) size / 2, -1);
            for (int bin = 1; bin < size / 2; ++bin)
            {
                const auto hz = bin * rate / size;
                const auto at = std::log (hz / 25.0) / std::log (18000.0 / 25.0) * numBands;
                // The lowest bands are narrower than a bin: a bin under 25 Hz counts in the first.
                of[(size_t) bin] = hz > 18000.0 ? -1 : juce::jlimit (0, numBands - 1, (int) std::floor (at));
            }
            return of;
        }();

        bands.fill (0.0);
        total = 0.0;
        if (sidePower != nullptr) *sidePower = 0.0;
        if (midPower != nullptr) *midPower = 0.0;
        std::vector<float> work ((size_t) size * 2);
        const auto first = frames (from), last = juce::jmin (frames (to), b.getNumSamples());

        for (int start = first; start < last; start += size / 2)
            for (int ch = 0; ch < 2; ++ch)
            {
                std::fill (work.begin(), work.end(), 0.0f);

                for (int i = 0; i < size && start + i < last; ++i)
                    work[(size_t) i] = b.getSample (ch, start + i) * window[(size_t) i];

                fft.performFrequencyOnlyForwardTransform (work.data(), true);

                for (int bin = 1; bin < size / 2; ++bin)
                    if (const auto band = bandOf[(size_t) bin]; band >= 0)
                    {
                        const auto power = (double) work[(size_t) bin] * (double) work[(size_t) bin];
                        bands[(size_t) band] += power;
                        total += power;
                    }
            }

        if (sidePower != nullptr && midPower != nullptr)
            for (int i = first; i < last; ++i)
            {
                const auto l = (double) b.getSample (0, i), r = (double) b.getSample (1, i);
                *sidePower += (l - r) * (l - r);
                *midPower += (l + r) * (l + r);
            }
    }

    double meanPower (const juce::AudioBuffer<float>& b, int from, int to)
    {
        double sum = 0.0;
        to = juce::jmin (to, b.getNumSamples());

        for (int ch = 0; ch < 2; ++ch)
            for (int i = from; i < to; ++i)
                sum += (double) b.getSample (ch, i) * (double) b.getSample (ch, i);

        return to > from ? sum / (2.0 * (to - from)) : 0.0;
    }

    /** The features of a note rendered with featureHold and featureLength. */
    Features featuresOf (const juce::AudioBuffer<float>& b)
    {
        Features f {};
        size_t at = 0;

        // Level over time.
        static const double bounds[numStretches + 1] = { 0.0, 0.01, 0.03, 0.06, 0.12, 0.25, 0.5, 0.8, 1.2, 1.6, 2.0, 2.03, 2.1, 2.25, 2.5, 3.0 };
        std::array<double, numStretches> level {};
        double loudest = 0.0;

        for (int i = 0; i < numStretches; ++i)
        {
            level[(size_t) i] = meanPower (b, frames (bounds[i]), frames (bounds[i + 1]));
            loudest = juce::jmax (loudest, level[(size_t) i]);
        }

        for (auto l : level)
            f[at++] = clipDb (l, loudest, stretchFloor);

        // The spectrum at four times; a window 50 dB under the loudest counts as silent.
        static const double windows[4][2] = { { 0.0, 0.15 }, { 0.15, 0.6 }, { 0.8, 2.0 }, { 2.0, 2.6 } };
        double side = 0.0, mid = 0.0;

        for (int w = 0; w < 4; ++w)
        {
            std::array<double, numBands> bands {};
            double total = 0.0;
            bandPowers (b, windows[w][0], windows[w][1], bands, total, w == 2 ? &side : nullptr, w == 2 ? &mid : nullptr);
            const auto sounding = meanPower (b, frames (windows[w][0]), frames (windows[w][1])) > loudest * 1.0e-5;

            for (auto p : bands)
                f[at++] = sounding ? clipDb (p, total, bandFloor) : bandFloor;
        }

        // How its level moves while held: the level every 4 ms from 0.15 to 2 s, at ten rates.
        {
            constexpr int hop = 192;
            std::vector<double> env;

            for (int i = frames (0.15); i + hop <= frames (2.0); i += hop)
                env.push_back (std::sqrt (meanPower (b, i, i + hop)));

            double mean = 0.0;
            for (auto e : env) mean += e;
            mean /= (double) env.size();
            static const double rates[numRates] = { 1.0, 1.5, 2.0, 3.0, 4.0, 6.0, 8.0, 12.0, 16.0, 24.0 };

            for (auto hz : rates)
            {
                double re = 0.0, im = 0.0;

                for (size_t i = 0; i < env.size(); ++i)
                {
                    const auto w = juce::MathConstants<double>::twoPi * hz * (double) i * hop / rate;
                    re += (env[i] - mean) * std::cos (w);
                    im += (env[i] - mean) * std::sin (w);
                }

                const auto depth = 2.0 * std::sqrt (re * re + im * im) / (double) env.size();
                f[at++] = mean > 1.0e-6 ? clipDb (depth * depth, mean * mean, otherFloor) : otherFloor;
            }
        }

        f[at++] = clipDb (side, mid, otherFloor);
        jassert (at == (size_t) numFeatures);
        return f;
    }

    float distance (const Features& a, const Features& b)
    {
        // The mean absolute difference over the numbers either sound has above its floor: silence in both
        // (a bass has nothing above 5 kHz, a drum nothing after half a second) is not a likeness. The level
        // over time is 15 numbers against the spectrum's 96, so it counts three times, and a short sound
        // and a long one of the same colour are told apart. The weights never add to less than 20.
        constexpr double stretchWeight = 3.0, leastWeight = 20.0;
        double sum = 0.0, weight = 0.0;

        for (size_t i = 0; i < a.size(); ++i)
        {
            const auto floor = i < (size_t) numStretches ? stretchFloor : i < (size_t) (numStretches + 4 * numBands) ? bandFloor : otherFloor;

            if (a[i] > floor + 0.005f || b[i] > floor + 0.005f)
            {
                const auto w = i < (size_t) numStretches ? stretchWeight : 1.0;
                sum += std::abs ((double) a[i] - (double) b[i]) * w;
                weight += w;
            }
        }

        return (float) (sum / juce::jmax (weight, leastWeight));
    }

    //==========================================================================
    /** What a macro changes: the sound's power in each eighth of a second, and in each band of its mid
        and of its side over the whole of a held note. The measure is the largest difference in dB between
        the macro at 0 and at 100, among the numbers within 60 dB of the largest of their kind. */
    std::vector<double> macroProfile (const juce::AudioBuffer<float>& b)
    {
        std::vector<double> v;
        constexpr int stretch = 6000;

        for (int i = 0; i + stretch <= b.getNumSamples(); i += stretch)
            v.push_back (meanPower (b, i, i + stretch));

        for (auto sign : { 1.0f, -1.0f })
        {
            juce::AudioBuffer<float> one (2, b.getNumSamples());

            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                const auto x = 0.5f * (b.getSample (0, i) + sign * b.getSample (1, i));
                one.setSample (0, i, x);
                one.setSample (1, i, x);
            }

            std::array<double, numBands> bands {};
            double total = 0.0;
            bandPowers (one, 0.0, (double) b.getNumSamples() / rate, bands, total);
            v.insert (v.end(), bands.begin(), bands.end());
        }

        return v;
    }

    float macroChange (Synth& s, const WaveSynth::Patch& p, int macro, int note)
    {
        const auto with = [&] (float value)
        {
            load (s, p);
            auto* parameter = s.parameter ("macro" + juce::String (macro + 1));
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
            return macroProfile (play (s, held ({ note }, 1.0, 0.8f), frames (1.5)));
        };
        const auto low = with (0.0f), high = with (100.0f);
        const auto levels = low.size() - 2 * (size_t) numBands;
        auto most = 0.0f;

        // The levels over time are one kind of number, the band powers (mid and side together) another.
        for (auto [from, to] : { std::pair { (size_t) 0, levels }, std::pair { levels, low.size() } })
        {
            double largest = 0.0;

            for (auto i = from; i < to; ++i)
                largest = juce::jmax (largest, low[i], high[i]);

            const auto floor = largest * 1.0e-6;

            for (auto i = from; i < to; ++i)
                if (low[i] > floor || high[i] > floor)
                    most = juce::jmax (most, (float) std::abs (10.0 * std::log10 (juce::jmax (floor, low[i]) / juce::jmax (floor, high[i]))));
        }

        return most;
    }

    /** True when each macro has a name and a slot that takes it somewhere. */
    bool macrosRouted (const WaveSynth::Patch& p)
    {
        std::array<bool, WaveSynth::numMacros> routed {};

        for (int slot = 1; slot <= WaveSynth::numSlots; ++slot)
        {
            const auto m = "mod" + juce::String (slot);
            const auto value = [&] (const char* field) { const auto v = p.values.find (m + field); return v != p.values.end() ? v->second : 0.0; };
            const auto source = (int) value ("Source");

            if (source >= WaveSynth::macro1 && source <= WaveSynth::macro4 && (int) value ("Destination") > 0 && ! juce::exactlyEqual (value ("Amount"), 0.0))
                routed[(size_t) (source - WaveSynth::macro1)] = true;
        }

        for (int i = 0; i < WaveSynth::numMacros; ++i)
            if (! routed[(size_t) i] || p.macroNames[(size_t) i].isEmpty())
                return false;

        return true;
    }

    //==========================================================================
    juce::uint64 fnv (juce::uint64 h, const void* data, size_t size)
    {
        const auto* bytes = static_cast<const juce::uint8*> (data);

        for (size_t i = 0; i < size; ++i)
            h = (h ^ bytes[i]) * 1099511628211ull;

        return h;
    }

    /** A patch's settings as one number: each setting's ID and value (as the float the synth takes,
        byte for byte, low byte first) and the macros' names. The same on every machine. */
    juce::String settingsHash (const WaveSynth::Patch& p)
    {
        auto h = 14695981039346656037ull;

        for (auto& [id, value] : p.values)
        {
            h = fnv (h, id.toRawUTF8(), id.getNumBytesAsUTF8());
            juce::uint32 bits;
            const auto f = (float) value;
            std::memcpy (&bits, &f, sizeof (bits));
            const juce::uint8 bytes[4] = { (juce::uint8) bits, (juce::uint8) (bits >> 8), (juce::uint8) (bits >> 16), (juce::uint8) (bits >> 24) };
            h = fnv (h, bytes, 4);
        }

        for (auto& name : p.macroNames)
        {
            h = fnv (h, name.toRawUTF8(), name.getNumBytesAsUTF8());
            h = fnv (h, "|", 1);
        }

        return juce::String::toHexString ((juce::int64) h).paddedLeft ('0', 16);
    }

    juce::String settingsList()
    {
        juce::String list;

        for (auto& p : WaveSynth::factoryPatches())
            list << p.category << "|" << p.name << "|" << settingsHash (p) << "\n";

        return list;
    }

    //==========================================================================
    /** True for a patch with an envelope that takes more than a second and a half to rise: a riser or a
        swell, louder long after the two seconds the level check holds its chord for. */
    bool slowToArrive (const WaveSynth::Patch& p)
    {
        for (int e = 1; e <= WaveSynth::numEnvelopes; ++e)
            if (const auto attack = p.values.find ("env" + juce::String (e) + "Attack"); attack != p.values.end() && attack->second > 1500.0)
                return true;

        return false;
    }

    struct Measured
    {
        float chordDb = 0.0f, noteDb = 0.0f, longDb = -100.0f;
        ClickCheck::Click click;
        std::array<float, WaveSynth::numMacros> macros {};
        Features features {};
        juce::uint64 chordHash = 0;
    };

    /** A patch's measurements. Each is made on a synth of its own, new for the purpose, so nothing
        left by another patch (or by the chord, which only the tool plays) reaches it: the test and the
        tool measure the same numbers whatever they measured before. */
    /** A hand-made patch on all three oscillators with a level envelope on one or more. */
    bool threeLayer (const WaveSynth::Patch& p)
    {
        const auto on = [&] (const char* id) { const auto f = p.values.find (id); return f != p.values.end() && f->second > 0.5; };
        return on ("osc2On") && on ("osc3On") && (on ("osc1EnvOn") || on ("osc2EnvOn") || on ("osc3EnvOn"));
    }

    /** `rest` adds the macros and the long chord; `click` the click phrase (the second bank's and the three-layer patches'). */
    Measured measure (const WaveSynth::Patch& p, bool chord, bool rest, bool click)
    {
        Measured m;
        const auto note = noteFor (p.category);

        if (chord)
        {
            Synth s;
            load (s, p);
            const auto out = play (s, held ({ 48, 51, 55 }, 2.0, 0.8f), frames (4.0));
            m.chordDb = peakDb (out);
            m.chordHash = fnv (fnv (14695981039346656037ull, out.getReadPointer (0), sizeof (float) * (size_t) out.getNumSamples()),
                               out.getReadPointer (1), sizeof (float) * (size_t) out.getNumSamples());
        }

        Synth s;
        load (s, p);
        const auto out = play (s, held ({ note }, featureHold, 0.8f), frames (featureLength));
        m.noteDb = peakDb (out);
        m.features = featuresOf (out);

        if (click)
        {
            load (s, p);
            m.click = ClickCheck::worst (clickPhrase (s, note), rate, clickFloor);
        }

        if (rest)
        {
            for (int i = 0; i < WaveSynth::numMacros; ++i)
                m.macros[(size_t) i] = macroChange (s, p, i, note);

            if (slowToArrive (p))
            {
                load (s, p);
                m.longDb = peakDb (play (s, held ({ 48, 51, 55 }, 10.0, 0.8f), frames (10.5)));
            }
        }

        return m;
    }

    /** Patches from a file of lines "name|category|macro;macro;macro;macro|id=value;id=value...". */
    std::vector<WaveSynth::Patch> patchesFrom (const juce::File& file)
    {
        std::vector<WaveSynth::Patch> all;

        for (auto& line : juce::StringArray::fromLines (file.loadFileAsString()))
        {
            const auto parts = juce::StringArray::fromTokens (line, "|", "");

            if (parts.size() < 4)
                continue;

            WaveSynth::Patch p;
            p.name = parts[0];
            p.category = parts[1];
            const auto macros = juce::StringArray::fromTokens (parts[2], ";", "");

            for (int i = 0; i < WaveSynth::numMacros && i < macros.size(); ++i)
                p.macroNames[(size_t) i] = macros[i];

            for (auto& setting : juce::StringArray::fromTokens (parts[3], ";", ""))
                p.values[setting.upToFirstOccurrenceOf ("=", false, false)] = (double) setting.fromFirstOccurrenceOf ("=", false, false).getFloatValue();

            all.push_back (std::move (p));
        }

        return all;
    }
}

//==============================================================================
bool runPatchTool (const juce::StringArray& args, int& result)
{
    const auto valueOf = [&] (const char* flag) { const auto i = args.indexOf (flag); return i >= 0 ? args[i + 1] : juce::String(); };

    if (args.contains ("--patch-settings"))
    {
        std::cout << settingsList();
        result = 0;
        return true;
    }

    if (const auto out = valueOf ("--patch-measure"); out.isNotEmpty())
    {
        // Each line: name|category|chord dB|chord hash|note dB|click ratio|click step|click seconds|macro changes|features|long chord dB.
        const auto file = valueOf ("--patches");
        const auto patches = file.isNotEmpty() ? patchesFrom (juce::File::getCurrentWorkingDirectory().getChildFile (file)) : WaveSynth::factoryPatches();
        const auto part = valueOf ("--part");
        const auto k = part.isNotEmpty() ? part.upToFirstOccurrenceOf ("/", false, false).getIntValue() : 0;
        const auto n = part.isNotEmpty() ? juce::jmax (1, part.fromFirstOccurrenceOf ("/", false, false).getIntValue()) : 1;
        const auto wavs = valueOf ("--wav");   // a folder for each patch's held note as a WAV file, to look at
        juce::String text;

        for (size_t i = 0; i < patches.size(); ++i)
        {
            if ((int) (i % (size_t) n) != k)
                continue;

            const auto& p = patches[i];
            const auto m = measure (p, true, true, true);

            if (wavs.isNotEmpty())
            {
                Synth synth;
                load (synth, p);
                const auto heldNote = play (synth, held ({ noteFor (p.category) }, featureHold, 0.8f), frames (featureLength));
                const auto wav = juce::File::getCurrentWorkingDirectory().getChildFile (wavs).getChildFile (juce::File::createLegalFileName (p.name.replaceCharacter ('/', '-')) + ".wav");
                wav.deleteFile();
                std::unique_ptr<juce::OutputStream> stream = wav.createOutputStream();
                const auto options = juce::AudioFormatWriterOptions{}.withSampleRate (rate).withNumChannels (2).withBitsPerSample (24);

                if (stream != nullptr)
                    if (auto writer = juce::WavAudioFormat().createWriterFor (stream, options))
                        writer->writeFromAudioSampleBuffer (heldNote, 0, heldNote.getNumSamples());
            }

            text << p.name << "|" << p.category << "|" << juce::String (m.chordDb, 2) << "|" << juce::String::toHexString ((juce::int64) m.chordHash)
                 << "|" << juce::String (m.noteDb, 2) << "|" << juce::String (m.click.ratio, 2) << "|" << juce::String (m.click.step, 4) << "|" << juce::String (m.click.seconds, 3) << "|";

            for (auto c : m.macros)
                text << juce::String (c, 2) << " ";

            text << "|";

            for (auto f : m.features)
                text << juce::String (f, 2) << " ";

            text << "|" << juce::String (m.longDb, 2) << "\n";
        }

        result = juce::File::getCurrentWorkingDirectory().getChildFile (out).replaceWithText (text) ? 0 : 1;
        return true;
    }

    return false;
}

void runWaveSynthBankTests()
{
    const auto& all = WaveSynth::factoryPatches();
    const juce::File source (TORQWAVE_SOURCE_DIR);

    // Which patches are the second bank's.
    const auto secondSize = WaveSynthPatchBuilder::secondBankNames().size();
    std::vector<bool> second (all.size(), false);
    {
        const std::set<juce::String> names (WaveSynthPatchBuilder::secondBankNames().begin(), WaveSynthPatchBuilder::secondBankNames().end());

        for (size_t i = 0; i < all.size(); ++i)
            second[i] = names.count (all[i].name) > 0;
    }

    // Nothing changes by accident: the list of patches and their settings is the one kept in the source.
    check (source.getChildFile ("Scripts/patch_settings.txt").loadFileAsString() == settingsList(),
           "synth bank: Scripts/patch_settings.txt lists every factory patch with its category and its settings' hash as they are, in the browser's order "
           "(TorqwaveTests --patch-settings writes it; a line that differs is a patch that has changed)");

    // The second bank: how many, named apart from every other patch whatever the case, each with four macros that go somewhere.
    {
        std::set<juce::String> lower;
        std::map<juce::String, int> perCategory;
        juce::StringArray unrouted;
        int found = 0;

        for (size_t i = 0; i < all.size(); ++i)
        {
            const auto& p = all[i];
            lower.insert (p.name.toLowerCase());

            if (second[i])
            {
                ++found;
                ++perCategory[p.category];

                if (! macrosRouted (p))
                    unrouted.add (p.name);
            }
        }

        juce::StringArray counts;

        for (auto& c : WaveSynth::patchCategories())
            if (perCategory.count (c) > 0)
                counts.add (c + " " + juce::String (perCategory[c]));

        check (found == secondSize && found >= 1450 && found <= 1600 && lower.size() == all.size() && unrouted.isEmpty(),
               "synth bank: the second bank adds " + juce::String (found) + " patches (1,450 to 1,600) to make " + juce::String ((int) all.size())
                   + ", no two names alike whatever the case, each with four named macros routed to something (" + counts.joinIntoString (", ") + ")"
                   + (unrouted.isEmpty() ? juce::String() : " (not routed: " + unrouted.joinIntoString ("; ").substring (0, 400) + ")"));
    }

    // The browser's menu holds every patch once, and no page of it is longer than a screen.
    {
        std::vector<int> seen (all.size(), 0);
        int longest = 0, mostPages = 0;

        for (auto& c : WaveSynth::patchCategories())
        {
            const auto pages = WaveSynthEditor::menuPages (c);
            mostPages = juce::jmax (mostPages, (int) pages.size());

            for (auto& page : pages)
            {
                longest = juce::jmax (longest, (int) page.patches.size());

                for (auto i : page.patches)
                    ++seen[(size_t) i];
            }
        }

        check (std::all_of (seen.begin(), seen.end(), [] (int n) { return n == 1; }) && longest <= WaveSynthEditor::mostInAPage && mostPages <= 12,
               "synth bank: the patch menu lists every patch once, by category, no page longer than " + juce::String (WaveSynthEditor::mostInAPage)
                   + " (the longest " + juce::String (longest) + ", the most pages in a category " + juce::String (mostPages) + ")");
    }

    // Every patch's note is rendered once for its features; the second bank's are also played through
    // the click phrase and with each macro at both ends. Eight at a time.
    std::vector<Measured> measured (all.size());
    {
        std::atomic<size_t> next { 0 };
        std::vector<std::thread> threads;

        for (int i = 0; i < 8; ++i)
            threads.emplace_back ([&]
            {
                for (auto at = next++; at < all.size(); at = next++)
                    measured[at] = measure (all[at], false, second[at], second[at] || threeLayer (all[at]));
            });

        for (auto& t : threads)
            t.join();
    }

    juce::StringArray silent, clicking, deaf, overs;
    auto quietest = 0.0f, worstRatio = 0.0f, leastMacro = 1000.0f, loudestLong = -100.0f;
    juce::String quietestName, worstClickName, leastMacroName, loudestLongName;
    int slow = 0;

    for (size_t i = 0; i < all.size(); ++i)
    {
        if (! second[i])
            continue;

        const auto& m = measured[i];

        if (m.noteDb < quietest) { quietest = m.noteDb; quietestName = all[i].name; }
        if (m.click.ratio > worstRatio) { worstRatio = m.click.ratio; worstClickName = all[i].name; }

        if (m.noteDb < silentDb || ! std::isfinite (m.noteDb))
            silent.add (all[i].name + " " + juce::String (m.noteDb, 1));

        if (slowToArrive (all[i]))
        {
            ++slow;

            if (m.longDb > loudestLong) { loudestLong = m.longDb; loudestLongName = all[i].name; }

            if (m.longDb > -1.0f)
                overs.add (all[i].name + " " + juce::String (m.longDb, 1));
        }

        if (m.click.ratio > clickRatio)
            clicking.add (all[i].name + " " + juce::String (m.click.step, 3) + " at " + juce::String (m.click.seconds, 3) + " s, " + juce::String (m.click.ratio, 1) + " times");

        for (int k = 0; k < WaveSynth::numMacros; ++k)
        {
            if (m.macros[(size_t) k] < leastMacro) { leastMacro = m.macros[(size_t) k]; leastMacroName = all[i].name + "'s " + all[i].macroNames[(size_t) k]; }

            if (m.macros[(size_t) k] < macroDb)
                deaf.add (all[i].name + "'s " + all[i].macroNames[(size_t) k] + " " + juce::String (m.macros[(size_t) k], 2));
        }
    }

    // Four first-bank patches that failed these measures: Concrete Debris clicked, and three bells were near silent up at C6.
    {
        juce::StringArray still;
        auto seen = 0;

        for (size_t i = 0; i < all.size(); ++i)
        {
            const auto& name = all[i].name;

            if (name == "Concrete Debris" || name == "Rotten Chime" || name == "Toxic Chime" || name == "Oily Gong")
            {
                ++seen;

                if (measured[i].click.ratio > clickRatio || measured[i].noteDb < silentDb)
                    still.add (name + " " + juce::String (measured[i].noteDb, 1) + " dB, click " + juce::String (measured[i].click.ratio, 1));
            }
        }

        check (seen == 4 && still.isEmpty(), "synth bank: Concrete Debris no longer clicks on the click phrase, and Rotten Chime, Toxic Chime and Oily Gong sound in the bells' register (above "
                                                 + juce::String (silentDb, 0) + " dB)" + (still.isEmpty() ? juce::String() : " (" + still.joinIntoString ("; ") + ")"));
    }

    // The hand-made patches of three layers (every oscillator on, a level envelope on at least one): each sounds in
    // its register and passes the click phrase, as the second bank's patches must.
    {
        juce::StringArray failing;
        auto layered = 0;
        auto worst = 0.0f;
        juce::String worstName;

        for (size_t i = 0; i < all.size(); ++i)
        {
            if (second[i] || ! threeLayer (all[i]))
                continue;

            ++layered;

            if (measured[i].click.ratio > worst) { worst = measured[i].click.ratio; worstName = all[i].name; }

            if (measured[i].click.ratio > clickRatio || measured[i].noteDb < silentDb)
                failing.add (all[i].name + " " + juce::String (measured[i].noteDb, 1) + " dB, click " + juce::String (measured[i].click.ratio, 1));
        }

        check (layered >= 41 && failing.isEmpty(), "synth bank: each of the " + juce::String (layered) + " hand-made patches of three layers (all three oscillators, with level envelopes) sounds in its register and passes the click phrase ("
                                                       + (worstName.isEmpty() ? juce::String ("none jumps") : "the nearest " + worstName + ", " + juce::String (worst, 1) + " times") + ")" + (failing.isEmpty() ? juce::String() : " (" + failing.joinIntoString ("; ").substring (0, 600) + ")"));
    }

    const auto listed = [] (const juce::StringArray& names) { return names.isEmpty() ? juce::String() : " (" + juce::String (names.size()) + ": " + names.joinIntoString ("; ").substring (0, 600) + ")"; };

    check (silent.isEmpty(), "synth bank: each of the second bank's patches sounds in its category's register: a note held two seconds peaks above "
                                 + juce::String (silentDb, 0) + " dB (the quietest " + quietestName + " at " + juce::String (quietest, 1) + " dB)" + listed (silent));

    check (clicking.isEmpty(), "synth bank: none of the second bank's patches clicks when its note is struck again while it rings, released, taken for another note "
                                   "or released in its attack: no jump in one sample of " + juce::String (clickFloor) + " or more stands alone, " + juce::String (clickRatio, 0)
                                   + " times the largest step in the 30 ms either side (the nearest " + worstClickName + ", " + juce::String (worstRatio, 1) + " times)" + listed (clicking));

    check (deaf.isEmpty(), "synth bank: each macro of the second bank's patches is heard: from 0 to 100 it moves the level of some eighth of a second, or some band of the mid or the side, by "
                               + juce::String (macroDb, 0) + " dB or more (the least " + leastMacroName + ", " + juce::String (leastMacro, 1) + " dB)" + listed (deaf));

    check (overs.isEmpty(), "synth bank: the second bank's " + juce::String (slow) + " patches with an envelope that takes more than 1.5 s to rise stay under -1 dB with their chord held ten seconds (the loudest "
                                + loudestLongName + " at " + juce::String (loudestLong, 1) + " dB)" + listed (overs));

    // The same at any block size: the first second-bank patch of each category, a note held half a second.
    {
        juce::StringArray differing;
        juce::String last;
        int tried = 0;

        for (size_t i = 0; i < all.size(); ++i)
        {
            const auto& p = all[i];

            if (! second[i] || p.category == last)
                continue;

            last = p.category;
            ++tried;
            std::vector<juce::AudioBuffer<float>> renders;

            for (auto blockSize : { 512, 64, 37 })
            {
                Synth s;
                load (s, p);
                renders.push_back (play (s, held ({ noteFor (p.category) }, 0.5, 0.8f), frames (1.0), blockSize));
            }

            for (size_t r = 1; r < renders.size(); ++r)
                for (int ch = 0; ch < 2; ++ch)
                    if (std::memcmp (renders[0].getReadPointer (ch), renders[r].getReadPointer (ch), sizeof (float) * (size_t) renders[0].getNumSamples()) != 0)
                        differing.addIfNotAlreadyThere (p.name);
        }

        check (differing.isEmpty() && tried >= 20, "synth bank: the first second-bank patch of each of " + juce::String (tried)
                                                       + " categories renders bit for bit the same in blocks of 512, 64 and 37 samples" + listed (differing));
    }

    // No near-copies: every second-bank patch against every other patch of its category.
    {
        juce::StringArray copies;
        auto nearest = 1000.0f;
        juce::String nearestPair;

        for (size_t i = 0; i < all.size(); ++i)
            for (size_t j = i + 1; j < all.size() && all[j].category == all[i].category; ++j)
            {
                if (! second[i] && ! second[j])
                    continue;

                const auto d = distance (measured[i].features, measured[j].features);

                if (d < nearest) { nearest = d; nearestPair = all[i].name + " and " + all[j].name; }

                if (d < nearCopy)
                    copies.add (all[i].name + " and " + all[j].name + " " + juce::String (d, 2));
            }

        check (copies.isEmpty(), "synth bank: no patch of the second bank is a near-copy of another in its category, the first bank's and the hand-made ones included: "
                                     "a held note's level over time, spectrum at four times, movement and width (" + juce::String (numFeatures) + " numbers in dB, those silent in both left out) differ by "
                                     + juce::String (nearCopy, 1) + " dB or more on average (the nearest " + nearestPair + ", " + juce::String (nearest, 2) + " dB)" + listed (copies));
    }
}
