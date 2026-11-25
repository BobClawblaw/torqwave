#include "Decrypt.h"
#include "WaveTables.h"
#include <set>

namespace Decrypt
{
namespace
{
    constexpr int block = 512;
    constexpr int firstRoundsLength = 48000;   // the factory patches are first compared over a second at most
    constexpr int keptBest = 12, parents = 4, perParent = 12, sweptPerRound = 3;
    constexpr int shownMost = 8, otherLines = 6, perOtherLine = 6;   // shown: the nearest and up to seven others; six other lines grown, six variations each a round
    constexpr float floorDb = -70.0f, loudnessFloorDb = -60.0f;
    constexpr float maskedDb = 40.0f;   // a band this far under its frame's loudest (in either sound) is masked: unheard

    juce::uint32 mix (juce::uint32 x) noexcept
    {
        x ^= x >> 16; x *= 0x7feb352du;
        x ^= x >> 15; x *= 0x846ca68bu;
        x ^= x >> 16;
        return x;
    }

    float toDb (double power, double floor = 1.0e-20)    { return (float) (10.0 * std::log10 (juce::jmax (floor, power))); }

    /** The triangular mel filters for one FFT size: for each band, the first bin and the weights from it. */
    struct MelBank
    {
        std::array<int, Measures::bands> first {};
        std::array<std::vector<float>, Measures::bands> weights;
    };

    const MelBank& melBank (int sizeIndex)
    {
        static const std::array<MelBank, Measures::sizes> banks = []
        {
            std::array<MelBank, Measures::sizes> made;
            const auto toMel = [] (double hz) { return 2595.0 * std::log10 (1.0 + hz / 700.0); };
            const auto toHz = [] (double mel) { return 700.0 * (std::pow (10.0, mel / 2595.0) - 1.0); };
            const auto low = toMel (40.0), high = toMel (16000.0);

            for (int s = 0; s < Measures::sizes; ++s)
            {
                const auto size = 512 << s;
                const auto binHz = rate / size;

                for (int b = 0; b < Measures::bands; ++b)
                {
                    const auto from = toHz (low + (high - low) * b / (Measures::bands + 1));
                    const auto middle = toHz (low + (high - low) * (b + 1) / (Measures::bands + 1));
                    const auto to = toHz (low + (high - low) * (b + 2) / (Measures::bands + 1));
                    auto& bank = made[(size_t) s];
                    // At least the one bin nearest its middle, so a narrow low band at a small size is not empty.
                    const auto firstBin = juce::jmin ((int) std::ceil (from / binHz), (int) std::lround (middle / binHz));
                    const auto lastBin = juce::jmax ((int) std::floor (to / binHz), (int) std::lround (middle / binHz));
                    bank.first[(size_t) b] = firstBin;

                    for (int k = firstBin; k <= lastBin && k < size / 2; ++k)
                    {
                        const auto hz = k * binHz;
                        const auto w = hz <= middle ? (hz - from) / juce::jmax (1.0e-9, middle - from) : (to - hz) / juce::jmax (1.0e-9, to - middle);
                        bank.weights[(size_t) b].push_back ((float) juce::jlimit (0.0, 1.0, w));
                    }

                    if (std::all_of (bank.weights[(size_t) b].begin(), bank.weights[(size_t) b].end(), [] (float w) { return w <= 0.0f; }) && ! bank.weights[(size_t) b].empty())
                        bank.weights[(size_t) b][0] = 1.0f;
                }
            }

            return made;
        }();

        return banks[(size_t) sizeIndex];
    }

    /** A render's channels as one, its peak at 1 (silence left as it is). */
    std::vector<float> monoOf (const juce::AudioBuffer<float>& audio)
    {
        const auto n = audio.getNumSamples();
        std::vector<float> mono ((size_t) n, 0.0f);
        const auto channels = audio.getNumChannels();

        for (int ch = 0; ch < channels; ++ch)
            juce::FloatVectorOperations::addWithMultiply (mono.data(), audio.getReadPointer (ch), 1.0f / (float) channels, n);

        float peak = 0.0f;

        for (auto x : mono)
            peak = juce::jmax (peak, std::abs (x));

        if (peak > 1.0e-6f)
            juce::FloatVectorOperations::multiply (mono.data(), 1.0f / peak, n);

        return mono;
    }

    int sampledIndex()   { return (int) WaveTables::Table::sampled; }

    bool slotSource (const EffectPresets::Values& v, int source)
    {
        for (int s = 1; s <= WaveSynth::numSlots; ++s)
        {
            const auto n = juce::String (s);
            const auto src = v.find ("mod" + n + "Source");
            const auto dst = v.find ("mod" + n + "Destination");

            if (src != v.end() && dst != v.end() && (int) src->second == source && (int) dst->second > 0)
                return true;
        }

        return false;
    }

    /** A patch whose oscillator 1 plays the clip's own table, swept from its start to its end over the
        note by an envelope nothing else uses; alone, the other oscillators, sub and noise off too. */
    Patch withClipTable (Patch p, std::shared_ptr<const WaveTables::Sampled> table, bool alone, double seconds)
    {
        auto& v = p.values;
        p.tables[0] = std::move (table);
        v["osc1On"] = 1.0;
        v["osc1Table"] = sampledIndex();
        v["osc1Warp"] = 0.0;
        v["osc1Pitch"] = 0.0;
        v["osc1Fine"] = 0.0;
        v["osc1EnvOn"] = 0.0;

        if (alone)
        {
            v["osc2On"] = 0.0;
            v["osc3On"] = 0.0;
            v["subLevel"] = 0.0;
            v["noiseLevel"] = 0.0;
        }

        // The sweep: an envelope that falls from full to nothing over the note, against Position at 100 and an
        // amount of -100, so Position runs from the table's first frame (0) to its last (100) as the note plays.
        const auto destination = WaveSynth::destinationIds().indexOf ("osc1Position") + 1;
        int envelope = 0, slot = 0;

        for (int e = WaveSynth::numEnvelopes; e >= 3 && envelope == 0; --e)
            if (! slotSource (v, WaveSynth::env1 + e - 1))
                envelope = e;

        for (int s = 1; s <= WaveSynth::numSlots && slot == 0; ++s)
        {
            const auto n = juce::String (s);
            if ((int) v["mod" + n + "Source"] == 0 || (int) v["mod" + n + "Destination"] == 0)
                slot = s;
        }

        if (envelope == 0 || slot == 0 || destination <= 0)
        {
            v["osc1Position"] = 0.0;
            return p;
        }

        const auto e = "env" + juce::String (envelope);
        v[e + "Attack"] = 0.0;
        v[e + "Decay"] = juce::jlimit (10.0, 10000.0, seconds * 1000.0);
        v[e + "Sustain"] = 0.0;
        v[e + "Release"] = juce::jlimit (10.0, 10000.0, seconds * 1000.0);
        const auto n = juce::String (slot);
        v["mod" + n + "Source"] = WaveSynth::env1 + envelope - 1;
        v["mod" + n + "Destination"] = destination;
        v["mod" + n + "Amount"] = -100.0;
        v["osc1Position"] = 100.0;
        return p;
    }

    std::unique_ptr<Synth> makeSynth()
    {
        auto s = std::make_unique<Synth>();
        s->setPlayConfigDetails (0, 2, rate, block);
        s->setNonRealtime (true);
        return s;
    }

    /** The first `length` samples of a target, with its measures: the factory patches' first round. */
    Target shortened (const Target& t, int length)
    {
        if (t.audio.getNumSamples() <= length)
            return t;

        Target s;
        s.audio.setSize (1, length);
        s.audio.copyFrom (0, 0, t.audio, 0, 0, length);
        s.pitchHz = t.pitchHz;
        s.note = t.note;
        s.chord = t.chord;
        s.held = juce::jmin (t.held, length);
        s.measures = measure (s.audio.getReadPointer (0), length);
        return s;
    }
}

//==============================================================================
Measures measure (const float* mono, int length)
{
    Measures m;
    m.frames = juce::jmax (1, length / Measures::hop);
    const auto frames = m.frames;

    for (int s = 0; s < Measures::sizes; ++s)
    {
        const auto order = 9 + s, size = 1 << order;
        juce::dsp::FFT fft (order);
        std::vector<float> work ((size_t) size * 2), window ((size_t) size);
        const auto& bank = melBank (s);
        auto& mel = m.mel[(size_t) s];
        mel.assign ((size_t) frames * Measures::bands, 0.0f);
        auto loudest = -1000.0f;

        for (int i = 0; i < size; ++i)
            window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) size);

        for (int f = 0; f < frames; ++f)
        {
            const auto start = f * Measures::hop + Measures::hop / 2 - size / 2;
            std::fill (work.begin(), work.end(), 0.0f);

            for (int i = 0; i < size; ++i)
                if (const auto at = start + i; at >= 0 && at < length)
                    work[(size_t) i] = mono[at] * window[(size_t) i];

            fft.performFrequencyOnlyForwardTransform (work.data());

            for (int b = 0; b < Measures::bands; ++b)
            {
                double energy = 0.0;
                const auto& w = bank.weights[(size_t) b];

                for (size_t k = 0; k < w.size(); ++k)
                {
                    const auto a = (double) work[(size_t) bank.first[(size_t) b] + k];
                    energy += w[k] * a * a;
                }

                const auto db = toDb (energy);
                mel[(size_t) (f * Measures::bands + b)] = db;
                loudest = juce::jmax (loudest, db);
            }
        }

        for (auto& db : mel)
            db = juce::jmax (floorDb, db - loudest);
    }

    // Loudness and brightness, a frame each.
    m.loudness.resize ((size_t) frames);
    m.brightness.resize ((size_t) frames);
    auto loudest = -1000.0f;

    for (int f = 0; f < frames; ++f)
    {
        double sum = 0.0;
        const auto from = f * Measures::hop, to = juce::jmin (length, from + Measures::hop);

        for (int i = from; i < to; ++i)
            sum += (double) mono[i] * mono[i];

        m.loudness[(size_t) f] = toDb (sum / juce::jmax (1, to - from));
        loudest = juce::jmax (loudest, m.loudness[(size_t) f]);

        double weight = 0.0, total = 0.0;

        for (int b = 0; b < Measures::bands; ++b)
        {
            const auto e = std::pow (10.0, m.mel[1][(size_t) (f * Measures::bands + b)] / 10.0);
            weight += e * b;
            total += e;
        }

        m.brightness[(size_t) f] = total > 0.0 ? (float) (weight / total / (Measures::bands - 1)) : 0.0f;
    }

    for (auto& db : m.loudness)
        db = juce::jmax (loudnessFloorDb, db - loudest);

    return m;
}

Distance distance (const Measures& t, const Measures& c)
{
    Distance d;
    const auto frames = juce::jmax (t.frames, c.frames);

    // Each frame counts as much as the louder of the two is loud in it.
    const auto loud = [] (const Measures& m, int f) { return f < m.frames ? m.loudness[(size_t) f] : loudnessFloorDb; };
    const auto bandOf = [] (const Measures& m, int s, int f, int b) { return f < m.frames ? m.mel[(size_t) s][(size_t) (f * Measures::bands + b)] : floorDb; };
    std::vector<double> weight ((size_t) frames);
    double weights = 0.0;

    for (int f = 0; f < frames; ++f)
    {
        weight[(size_t) f] = std::pow (10.0, juce::jmax (loud (t, f), loud (c, f)) / 20.0);
        weights += weight[(size_t) f];
    }

    double tone = 0.0, movement = 0.0, brightness = 0.0, loudness = 0.0;

    for (int s = 0; s < Measures::sizes; ++s)
    {
        // A band far under the frame's loudest (in the louder of the two) is masked: it counts as at that floor.
        // Without it, faint noise 40 dB down measured farther than a low-pass at 2 kHz: the search spent its
        // rounds on detail nobody hears.
        const auto bandsOf = [&] (const Measures& m, int f, std::array<float, Measures::bands>& out)
        {
            for (int b = 0; b < Measures::bands; ++b)
                out[(size_t) b] = bandOf (m, s, f, b);
        };
        const auto top = [] (const std::array<float, Measures::bands>& a) { return *std::max_element (a.begin(), a.end()); };
        const auto apart = [] (const std::array<float, Measures::bands>& a, const std::array<float, Measures::bands>& b, float floor)
        {
            double sum = 0.0;
            for (int i = 0; i < Measures::bands; ++i)
                sum += std::abs (juce::jmax (floor, a[(size_t) i]) - juce::jmax (floor, b[(size_t) i]));
            return sum / Measures::bands;
        };
        std::array<float, Measures::bands> tb {}, cb {};

        for (int f = 0; f < frames; ++f)
        {
            double flux = 0.0;

            for (int b = 0; b < Measures::bands; ++b)
                if (f > 0)
                    flux += std::abs (std::abs (bandOf (t, s, f, b) - bandOf (t, s, f - 1, b)) - std::abs (bandOf (c, s, f, b) - bandOf (c, s, f - 1, b)));

            bandsOf (t, f, tb);
            bandsOf (c, f, cb);
            const auto here = apart (tb, cb, juce::jmax (top (tb), top (cb)) - maskedDb);
            tone += weight[(size_t) f] * here;
            movement += weight[(size_t) f] * flux / Measures::bands;
        }
    }

    for (int f = 0; f < frames; ++f)
    {
        loudness += std::abs (loud (t, f) - loud (c, f));
        const auto bt = f < t.frames ? t.brightness[(size_t) f] : 0.0f, bc = f < c.frames ? c.brightness[(size_t) f] : 0.0f;
        brightness += weight[(size_t) f] * std::abs (bt - bc) * 100.0;   // a band's width is 2.5 % of the range: about 2.5 dB a band
    }

    const auto norm = juce::jmax (1.0e-9, weights);
    d.tone = (float) (tone / Measures::sizes / norm);
    d.movement = (float) (movement / Measures::sizes / norm);
    d.brightness = (float) (brightness / norm);
    d.loudness = (float) (loudness / frames);
    d.total = 0.7f * d.tone + 0.3f * d.loudness;
    return d;
}

float lockOf (float distanceDb)
{
    return juce::jlimit (0.0f, 100.0f, 100.0f * (1.0f - distanceDb / 12.0f));
}

//==============================================================================
std::vector<int> findChord (const float* mono, int length)
{
    constexpr double chordLeast = 0.5;   // a further key weighs half the first's or more
    // The sound's spectrum at 1.5 Hz a bin, its windows' powers averaged: what stays in place through
    // the sound stands, what moves or is noise spreads.
    constexpr int order = 15, size = 1 << order;

    if (length < (int) (0.25 * rate))
        return {};

    juce::dsp::FFT fft (order);
    juce::dsp::WindowingFunction<float> window ((size_t) size, juce::dsp::WindowingFunction<float>::hann);
    std::vector<float> work ((size_t) size * 2), power ((size_t) size / 2, 0.0f);

    for (int at = 0; at == 0 || at + size <= length; at += size / 4)
    {
        std::fill (work.begin(), work.end(), 0.0f);
        std::copy (mono + at, mono + juce::jmin (length, at + size), work.begin());
        window.multiplyWithWindowingTable (work.data(), (size_t) size);
        fft.performFrequencyOnlyForwardTransform (work.data());

        for (int i = 0; i < size / 2; ++i)
            power[(size_t) i] += work[(size_t) i] * work[(size_t) i];
    }

    const auto hzPerBin = rate / size;

    const auto energy = [&] (double fromHz, double toHz)
    {
        const auto a = juce::jlimit (1, size / 2 - 1, (int) std::lround (fromHz / hzPerBin));
        const auto b = juce::jlimit (a, size / 2 - 1, (int) std::lround (toHz / hzPerBin));
        auto sum = 0.0;

        for (int i = a; i <= b; ++i)
            sum += power[(size_t) i];

        return sum / (b - a + 1);
    };

    const auto cents = [] (double c) { return std::pow (2.0, c / 1200.0); };

    // A partial stands at a frequency when what is within 20 cents of it is 5 dB or more over what
    // lies 30 to 50 cents to either side, and within 30 dB of the strongest: its amplitude, or 0.
    auto strongest = 0.0;

    for (int i = (int) (100.0 / hzPerBin); i < (int) (5000.0 / hzPerBin); ++i)
        strongest = juce::jmax (strongest, (double) power[(size_t) i]);

    if (strongest <= 0.0)
        return {};

    const auto standing = [&] (double hz)
    {
        if (hz < 100.0 || hz > 5000.0)
            return 0.0;

        auto top = 0.0;

        for (int i = (int) std::floor (hz * cents (-20.0) / hzPerBin); i <= (int) std::ceil (hz * cents (20.0) / hzPerBin); ++i)
            top = juce::jmax (top, (double) power[(size_t) juce::jlimit (1, size / 2 - 1, i)]);

        const auto beside = 0.5 * (energy (hz * cents (-50.0), hz * cents (-30.0)) + energy (hz * cents (30.0), hz * cents (50.0)));
        return top >= beside * 3.16 && top >= strongest * 0.001 ? std::sqrt (top) : 0.0;
    };

    // Noise has no notes: its spectrum is near flat from bin to bin, a sound of notes is mostly gaps.
    {
        auto logs = 0.0, sum = 0.0;
        const auto from = (int) (100.0 / hzPerBin), to = (int) (5000.0 / hzPerBin);

        for (int i = from; i < to; ++i)
        {
            logs += std::log ((double) power[(size_t) i] + 1.0e-30);
            sum += power[(size_t) i];
        }

        if (std::exp (logs / (to - from)) > 0.05 * sum / (to - from))
            return {};
    }

    // Each key's harmonics that stand, the first sixteen, their amplitudes compressed (a square root)
    // so one loud partial doesn't speak for a key alone. A key's weight is those amplitudes summed,
    // each weighed by (f + 27) / (h f + 320): low harmonics count most, and a low key's many count
    // for less each (Klapuri's weights). The heaviest key is taken, what it accounts for is taken
    // off the partials, and the next is weighed on what is left, six keys at most.
    constexpr int lowest = 36, highest = 96, harmonics = 16;
    const auto hzOf = [] (int key) { return 440.0 * std::pow (2.0, (key - 69) / 12.0); };
    std::vector<std::array<double, harmonics>> left ((size_t) (highest - lowest + 1));

    for (int key = lowest; key <= highest; ++key)
        for (int h = 1; h <= harmonics; ++h)
            left[(size_t) (key - lowest)][(size_t) h - 1] = std::sqrt (standing (hzOf (key) * h));

    const auto gain = [&] (int key, int h) { return (hzOf (key) + 27.0) / (h * hzOf (key) + 320.0); };

    const auto weigh = [&] (int key)
    {
        auto weight = 0.0;
        auto stood = 0;

        for (int h = 1; h <= harmonics; ++h)
            if (const auto a = left[(size_t) (key - lowest)][(size_t) h - 1]; a > 0.0)
            {
                weight += a * gain (key, h);
                ++stood;
            }

        return stood >= 3 ? weight : 0.0;
    };

    std::vector<int> chord;
    auto first = 0.0;

    while (chord.size() < 6)
    {
        auto best = -1;
        auto bestWeight = 0.0;

        for (int key = lowest; key <= highest; ++key)
            if (const auto w = weigh (key); w > bestWeight)
            {
                best = key;
                bestWeight = w;
            }

        if (best < 0 || bestWeight < first * chordLeast)
            break;

        if (first <= 0.0)
            first = bestWeight;

        chord.push_back (best);

        // What it accounts for comes off every key's partial at that frequency.
        for (int h = 1; h <= harmonics; ++h)
        {
            const auto hz = hzOf (best) * h;
            const auto share = gain (best, h) / gain (best, 1);

            for (int key = lowest; key <= highest; ++key)
                for (int k = 1; k <= harmonics; ++k)
                    if (std::abs (1200.0 * std::log2 (hzOf (key) * k / hz)) < 25.0)
                    {
                        auto& a = left[(size_t) (key - lowest)][(size_t) k - 1];
                        a = key == best ? 0.0 : juce::jmax (0.0, a * (1.0 - share));
                    }
        }
    }

    std::sort (chord.begin(), chord.end());
    return chord;
}

//==============================================================================
Target prepare (const juce::AudioBuffer<float>& clip, double sampleRate)
{
    Target t;
    const auto channels = clip.getNumChannels(), length = clip.getNumSamples();

    if (channels < 1 || length < 1 || sampleRate <= 0.0)
    {
        t.problem = "The clip is empty.";
        return t;
    }

    // One channel at 48 kHz.
    std::vector<float> mono ((size_t) length, 0.0f);

    for (int ch = 0; ch < channels; ++ch)
        juce::FloatVectorOperations::addWithMultiply (mono.data(), clip.getReadPointer (ch), 1.0f / (float) channels, length);

    if (std::abs (sampleRate - rate) > 0.5)
    {
        const auto ratio = sampleRate / rate;
        std::vector<float> resampled ((size_t) (length / ratio));
        juce::LagrangeInterpolator interpolator;
        interpolator.process (ratio, mono.data(), resampled.data(), (int) resampled.size());
        mono = std::move (resampled);
    }

    float peak = 0.0f;

    for (auto x : mono)
        peak = juce::jmax (peak, std::abs (x));

    if (peak < 1.0e-4f)
    {
        t.problem = "The clip is silent.";
        return t;
    }

    // The note: from 2 ms before it passes -30 dB of its peak to 10 ms after it last passes -60 dB, four seconds at most.
    const auto n = (int) mono.size();
    int start = 0, end = n;

    while (start < n && std::abs (mono[(size_t) start]) < peak * 0.0316f)
        ++start;

    while (end > start && std::abs (mono[(size_t) end - 1]) < peak * 0.001f)
        --end;

    start = juce::jmax (0, start - 96);
    const auto fadesInside = end + 480 < n && end + 480 <= start + (int) (longest * rate);   // silent before the clip (and the four seconds) end
    end = juce::jmin (n, end + 480, start + (int) (longest * rate));

    if (end - start < (int) (0.1 * rate))
    {
        t.problem = "The note is too short: a tenth of a second at least.";
        return t;
    }

    t.audio.setSize (1, end - start);
    t.audio.copyFrom (0, 0, mono.data() + start, end - start, 0.5f / peak);
    const auto* x = t.audio.getReadPointer (0);
    const auto samples = t.audio.getNumSamples();

    // Its pitch, from its first half second after the attack.
    const auto from = juce::jmin (samples - 1, (int) (0.02 * rate));
    t.pitchHz = WaveTables::findPitch (x + from, juce::jmin (samples - from, (int) (0.5 * rate)), rate);
    t.note = t.pitchHz > 0.0 ? juce::jlimit (12, 108, (int) std::lround (69.0 + 12.0 * std::log2 (t.pitchHz / 440.0))) : 60;


    // Several notes at once: keys of three or more different names found in it (one note's own
    // harmonics give its octaves and its fifth at most). It is rendered on all of them.
    if (auto chord = findChord (x, samples); chord.size() >= 3)
    {
        std::set<int> names;

        for (auto key : chord)
            names.insert (key % 12);

        if (names.size() >= 3)
        {
            t.note = chord.front();
            t.chord = std::move (chord);
        }
    }

    t.measures = measure (x, samples);
    t.signature = SignalConsole::Renderer::measure (t.audio, rate);

    // A note that dies away by itself (a stab, a pluck, a hit) is matched with the key held all the
    // way and a quarter of a second of silence after it: the patch has to fall silent as the sound
    // does with the key still down, and any sound it makes after the source has gone counts against
    // it. Held for only the note's loud part, a patch that sustains and lets go in time matched as
    // well, and on the keys went on for as long as a key was held.
    //
    // A note is taken as held and let go, and matched as before (the key down until it last falls
    // 12 dB under its loudest 10 ms), when after its loudest moment its level, smoothed over 90 ms:
    // stays within 3 dB for a fifth of a second (a pad, an organ); comes back up 3 dB after falling
    // (a beating reese, a gate, a tremolo); or, 0.4 s or more into the note, starts falling three
    // times as fast as it was, at 100 dB a second or more (a pluck let go). So does a note still
    // sounding where the clip ends.
    const auto frames = t.measures.frames;
    std::vector<float> smooth ((size_t) frames, -100.0f);

    for (int f = 0; f < frames; ++f)
        for (int k = juce::jmax (0, f - 4); k <= juce::jmin (frames - 1, f + 4); ++k)
            smooth[(size_t) f] = juce::jmax (smooth[(size_t) f], t.measures.loudness[(size_t) k]);

    auto holds = false;
    const auto loudest = (int) (std::max_element (smooth.begin(), smooth.end()) - smooth.begin());
    auto lowest = frames > 0 ? smooth[(size_t) loudest] : 0.0f;

    for (int f = loudest; f < frames && ! holds; ++f)
    {
        const auto now = smooth[(size_t) f];

        if (f + steadyFrames <= frames)
        {
            const auto [low, high] = std::minmax_element (smooth.begin() + f, smooth.begin() + f + steadyFrames);
            holds = *low > -24.0f && *high - *low < 3.0f;
        }

        lowest = juce::jmin (lowest, now);
        holds = holds || (lowest > -40.0f && now > lowest + 3.0f);

        if (f >= 40 && f - 15 >= loudest && f + 8 < frames && now > -20.0f)
        {
            const auto before = (now - smooth[(size_t) f - 15]) / 0.15f, after = (smooth[(size_t) f + 8] - now) / 0.08f;   // dB a second
            holds = holds || (after <= -100.0f && after <= 3.0f * before);
        }
    }

    t.decays = fadesInside && ! holds;

    if (t.decays)
    {
        const auto tail = (int) (silenceAfter * rate);
        t.audio.setSize (1, samples + tail, true, true);
        t.audio.clear (0, samples, tail);
        t.held = samples + tail;   // never let go inside the render
        t.measures = measure (t.audio.getReadPointer (0), samples + tail);
        return t;
    }

    int last = 0;

    for (int f = 0; f < t.measures.frames; ++f)
        if (t.measures.loudness[(size_t) f] >= -12.0f)
            last = f;

    t.held = juce::jlimit (Measures::hop * 5, samples, (last + 1) * Measures::hop);
    return t;
}

namespace
{
    /** A patch played on the target's key, held for as long as the target's is, at a velocity. */
    juce::AudioBuffer<float> play (Synth& synth, const Patch& patch, const Target& target, float velocity)
    {
        SignalConsole::apply (synth, patch);
        synth.prepareToPlay (rate, block);
        const auto length = target.audio.getNumSamples();
        juce::AudioBuffer<float> out (2, length), part (2, block);

        for (int at = 0; at < length; at += block)
        {
            const auto count = juce::jmin (block, length - at);
            juce::MidiBuffer midi;

            for (auto key : target.chord.empty() ? std::vector<int> { target.note } : target.chord)
            {
                if (at == 0)
                    midi.addEvent (juce::MidiMessage::noteOn (1, key, velocity), 0);

                if (target.held >= at && target.held < at + count)
                    midi.addEvent (juce::MidiMessage::noteOff (1, key), target.held - at);
            }

            part.setSize (2, count, false, false, true);
            part.clear();
            synth.processBlock (part, midi);

            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, at, part, ch, 0, count);
        }

        return out;
    }

    float peakDb (const juce::AudioBuffer<float>& audio)
    {
        return juce::Decibels::gainToDecibels (audio.getMagnitude (0, audio.getNumSamples()), -100.0f);
    }
}

juce::AudioBuffer<float> render (Synth& synth, const Patch& patch, const Target& target)
{
    auto dry = patch;

    for (auto* effect : { "fxDriveOn", "fxChorusOn", "fxPhaserOn", "fxDelayOn", "fxReverbOn" })
        dry.values[effect] = 0.0;

    return play (synth, dry, target, 0.8f);
}

Levelled levelForKeys (const Patch& patch, const Target& target)
{
    // Played as the keys play it (its effects on, velocity 100), its Amp Level moved by what its peak
    // misses the audition's by. The effects after the amp (drive above all) need not answer in step,
    // so it is measured again after each move, and the next move takes how far the last one got it:
    // six moves at most.
    Synth player;
    player.setPlayConfigDetails (0, 2, rate, block);
    player.setNonRealtime (true);
    const auto* knob = player.parameter ("ampLevel");
    const auto range = knob != nullptr ? knob->getNormalisableRange() : juce::NormalisableRange<float> (-40.0f, 6.0f);
    const auto velocity = 100.0f / 127.0f;

    Levelled l;
    l.patch = patch;
    const auto it = patch.values.find ("ampLevel");
    auto level = it != patch.values.end() ? (float) it->second : (knob != nullptr ? range.convertFrom0to1 (knob->getDefaultValue()) : -9.0f);
    l.beforeDb = peakDb (play (player, patch, target, velocity));
    l.afterDb = l.beforeDb;
    auto perDb = 1.0f;   // how many dB the peak moves for a dB of Amp Level: 1 until it is measured (drive and a limiter give less)

    for (int move = 0; move < 6 && std::abs (l.afterDb - auditionDb) > 0.25f && l.afterDb > -99.0f; ++move)
    {
        const auto next = juce::jlimit (range.start, range.end, level + (auditionDb - l.afterDb) / perDb);

        if (std::abs (next - level) < 0.01f)
            break;   // the knob is at the end of its range

        const auto was = l.afterDb;
        l.patch.values["ampLevel"] = next;
        l.afterDb = peakDb (play (player, l.patch, target, velocity));
        l.patch.values["ampLevel"] = SignalConsole::capture (player).values["ampLevel"];   // as the knob holds it, so the patch reads back the same
        perDb = juce::jlimit (0.2f, 2.0f, (l.afterDb - was) / (next - level));
        level = next;
    }

    l.ranOut = std::abs (l.afterDb - auditionDb) > 1.0f;
    return l;
}

Measures measureRender (const juce::AudioBuffer<float>& audio)
{
    const auto mono = monoOf (audio);
    return measure (mono.data(), (int) mono.size());
}

//==============================================================================
namespace
{
    juce::CriticalSection& makerLock()      { static juce::CriticalSection l; return l; }
    AdvisorMaker& maker()                   { static AdvisorMaker m; return m; }
}

void setAdvisorMaker (AdvisorMaker m)
{
    const juce::ScopedLock sl (makerLock());
    maker() = std::move (m);
}

std::shared_ptr<Advisor> makeAdvisor()
{
    AdvisorMaker m;
    {
        const juce::ScopedLock sl (makerLock());
        m = maker();
    }

    return m != nullptr ? m() : nullptr;
}

//==============================================================================
juce::String noteName (int note)
{
    static const juce::StringArray names { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return names[((note % 12) + 12) % 12] + juce::String (note / 12 - 1);
}

juce::String chordName (const Target& t)
{
    juce::StringArray names;

    for (auto key : t.chord)
        names.add (noteName (key));

    return names.joinIntoString (" ");
}

namespace
{
    /** The frame the target's note is loudest in, and the first after it that falls 20 dB under that. */
    std::pair<int, int> shapeOf (const Measures& m)
    {
        int peak = 0, fall = m.frames;

        for (int f = 0; f < m.frames; ++f)
            if (m.loudness[(size_t) f] > m.loudness[(size_t) peak])
                peak = f;

        for (int f = peak; f < m.frames; ++f)
            if (m.loudness[(size_t) f] < -20.0f)
            {
                fall = f;
                break;
            }

        return { peak, fall };
    }

    float meanOf (const std::vector<float>& v)
    {
        return v.empty() ? 0.0f : std::accumulate (v.begin(), v.end(), 0.0f) / (float) v.size();
    }
}

juce::String describe (const Target& t)
{
    const auto& m = t.measures;
    const auto [peak, fall] = shapeOf (m);
    const auto brightness = meanOf (m.brightness);
    juce::String s;
    s << (! t.chord.empty() ? "a chord (" + chordName (t) + ")"
          : t.pitchHz > 0.0 ? noteName (t.note) + " (" + juce::String (t.pitchHz, 1) + " Hz)" : juce::String ("no clear pitch (matched at C4)"))
      << ", " << juce::String (t.audio.getNumSamples() / rate, 2) << " s"
      << ", loudest at " << peak * 10 << " ms"
      << ", " << (fall >= m.frames ? juce::String ("held to the end") : "20 dB down by " + juce::String (fall * 10) + " ms")
      << ", " << (brightness < 0.25f ? "dark" : brightness < 0.45f ? "warm" : brightness < 0.6f ? "bright" : "very bright")
      << " (spectral weight at " << juce::roundToInt (brightness * 100.0f) << " % of 40 Hz to 16 kHz on a mel scale)";
    return s;
}

juce::String describeDifference (const Measures& t, const Measures& c)
{
    static const juce::StringArray bandNames { "lows", "low mids", "high mids", "highs" };
    static const juce::StringArray sectionNames { "attack", "body", "tail" };
    const auto [peak, fall] = shapeOf (t);
    const auto sectionOf = [attackEnd = juce::jmax (peak, 5), tailStart = fall] (int f) { return f <= attackEnd ? 0 : f < tailStart ? 1 : 2; };

    struct Cell { double sum = 0.0; int count = 0; };
    std::array<std::array<Cell, 4>, 3> cells {};
    std::array<Cell, 3> loud {};
    const auto frames = juce::jmax (t.frames, c.frames);

    for (int f = 0; f < frames; ++f)
    {
        const auto section = sectionOf (f);
        const auto lt = f < t.frames ? t.loudness[(size_t) f] : -60.0f, lc = f < c.frames ? c.loudness[(size_t) f] : -60.0f;

        if (lt > -50.0f || lc > -50.0f)
        {
            loud[(size_t) section].sum += lc - lt;
            ++loud[(size_t) section].count;
        }

        for (int b = 0; b < Measures::bands; ++b)
        {
            const auto vt = f < t.frames ? t.mel[1][(size_t) (f * Measures::bands + b)] : -70.0f;
            const auto vc = f < c.frames ? c.mel[1][(size_t) (f * Measures::bands + b)] : -70.0f;

            if (vt > -50.0f || vc > -50.0f)
            {
                auto& cell = cells[(size_t) section][(size_t) (b * 4 / Measures::bands)];
                cell.sum += vc - vt;
                ++cell.count;
            }
        }
    }

    struct Line { float db; juce::String text; };
    std::vector<Line> lines;

    for (int s = 0; s < 3; ++s)
    {
        if (loud[(size_t) s].count > 0)
        {
            const auto db = (float) (loud[(size_t) s].sum / loud[(size_t) s].count);
            lines.push_back ({ std::abs (db) * 1.5f, "loudness in the " + sectionNames[s] + ": the candidate is " + juce::String (std::abs (db), 1) + " dB " + (db > 0 ? "louder" : "quieter") });
        }

        for (int b = 0; b < 4; ++b)
            if (const auto& cell = cells[(size_t) s][(size_t) b]; cell.count > 0)
            {
                const auto db = (float) (cell.sum / cell.count);
                lines.push_back ({ std::abs (db), bandNames[b] + " in the " + sectionNames[s] + ": the candidate is " + juce::String (std::abs (db), 1) + " dB " + (db > 0 ? "stronger" : "weaker") });
            }
    }

    std::stable_sort (lines.begin(), lines.end(), [] (const Line& a, const Line& b) { return a.db > b.db; });
    juce::StringArray out;

    for (size_t i = 0; i < lines.size() && i < 5; ++i)
        if (lines[i].db >= 1.5f)
            out.add (lines[i].text);

    return out.isEmpty() ? juce::String ("no part differs by more than 1.5 dB") : out.joinIntoString ("; ");
}

juce::String describeSettings (Synth& synth, const Patch& patch)
{
    juce::StringArray lines;

    for (auto& [id, value] : patch.values)
    {
        const auto part = SignalConsole::partOf (id);
        auto* p = synth.parameter (id);

        if (p == nullptr || part == SignalConsole::effects || (part < 0 && id != "ampLevel"))
            continue;

        const auto normal = p->convertTo0to1 ((float) value);

        if (std::abs (normal - p->getDefaultValue()) < 0.001f && ! id.endsWith ("On"))
            continue;

        lines.add (id + " (" + p->getName (64) + "): " + (p->getText (normal, 64) + " " + p->getLabel()).trim()
                   + (dynamic_cast<juce::AudioParameterChoice*> (p) != nullptr ? " [" + juce::String ((int) std::lround (value)) + "]" : juce::String()));
    }

    lines.add ("The choices, by index: ");

    for (auto* id : { "osc1Table", "osc1Warp", "filter1Type", "noiseType", "subShape", "subOctave" })
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (synth.parameter (id)))
        {
            juce::StringArray choices;

            for (int i = 0; i < c->choices.size(); ++i)
                choices.add (juce::String (i) + " " + c->choices[i]);

            lines.add (juce::String (id) + (juce::String (id).startsWith ("osc1") ? " (and osc2, osc3)" : juce::String (id).startsWith ("filter1") ? " (and filter2)" : juce::String())
                       + ": " + choices.joinIntoString (", "));
        }

    return lines.joinIntoString ("\n");
}

//==============================================================================
class Search::Runner final : public juce::Thread
{
public:
    explicit Runner (Search& s) : juce::Thread ("Torqwave Synth decrypt"), search (s)    {}
    ~Runner() override                                                                 { stopThread (-1); }

    void run() override
    {
        search.run ([this] { return threadShouldExit() || search.stopping.load(); });
    }

private:
    Search& search;
};

/** The advisor's calls, one at a time on a thread of their own, so the search never waits for one.
    An answer later than the advisor's time limit is dropped, and the call cancelled. */
/** The model, asked on a thread of its own that nothing ever waits for. A model can take a minute over
    an answer, or never give one, and may not give up when told to: so the thread is let go, not
    joined. When the helper goes (the search ended, or was replaced by a new sound's), the thread is
    told to stop and the advisor to give up, and whatever it is still doing ends in its own time,
    holding only what it shares with the helper (the advisor, a copy of the clip). */
class Search::Helper final
{
public:
    explicit Helper (std::shared_ptr<Advisor> a) : state (std::make_shared<State>())
    {
        state->advisor = std::move (a);
        juce::Thread::launch (juce::Thread::Priority::low, [s = state] { run (*s); });
    }

    ~Helper()
    {
        state->leave.store (true);
        state->advisor->cancel();
        state->wake.signal();
    }

    /** Asks, unless a call is already out: false then. */
    bool ask (bool forStart, Advisor::Brief brief)
    {
        const juce::ScopedLock sl (state->lock);

        if (state->busy)
            return false;

        // The clip is the search's: the call may outlast the search, so it gets a copy.
        std::shared_ptr<const juce::AudioBuffer<float>> clip;

        if (brief.clip != nullptr)
        {
            clip = std::make_shared<const juce::AudioBuffer<float>> (*brief.clip);
            brief.clip = clip.get();
        }

        state->busy = true;
        state->job = { forStart, std::move (brief), std::move (clip), juce::Time::getMillisecondCounterHiRes(), ++state->generation };
        state->wake.signal();
        return true;
    }

    /** Drops a call that has run past its time. */
    void expire()
    {
        bool late;
        {
            const juce::ScopedLock sl (state->lock);
            late = state->busy && juce::Time::getMillisecondCounterHiRes() - state->job.askedAt > state->advisor->timeLimitMs;

            if (late)
                ++state->generation;   // its answer, if it comes, is for nobody
        }

        if (late)
            state->advisor->cancel();
    }

    /** True while a call is out (an expired one until its advisor has given up). */
    bool isBusy()
    {
        const juce::ScopedLock sl (state->lock);
        return state->busy;
    }

    std::optional<Advisor::Start> takeStart()
    {
        const juce::ScopedLock sl (state->lock);
        return std::exchange (state->startAnswer, std::nullopt);
    }

    /** Changes proposed, with the lock of the candidate they were proposed for. */
    std::optional<std::pair<std::vector<Advisor::Change>, float>> takeStructure()
    {
        const juce::ScopedLock sl (state->lock);
        return std::exchange (state->structureAnswer, std::nullopt);
    }

private:
    struct Job
    {
        bool forStart = false;
        Advisor::Brief brief;
        std::shared_ptr<const juce::AudioBuffer<float>> clip;   // what brief.clip points at
        double askedAt = 0.0;
        juce::uint32 generation = 0;
        bool taken = false;
    };

    struct State
    {
        std::shared_ptr<Advisor> advisor;
        juce::CriticalSection lock;
        juce::WaitableEvent wake;
        std::atomic<bool> leave { false };
        Job job;
        bool busy = false;
        juce::uint32 generation = 0;
        std::optional<Advisor::Start> startAnswer;
        std::optional<std::pair<std::vector<Advisor::Change>, float>> structureAnswer;
    };

    static void run (State& s)
    {
        while (! s.leave.load())
        {
            s.wake.wait (100);
            Job now;
            {
                const juce::ScopedLock sl (s.lock);

                if (! s.busy || s.job.taken)
                    continue;

                s.job.taken = true;
                now = s.job;
            }

            std::optional<Advisor::Start> start;
            std::vector<Advisor::Change> changes;

            if (now.forStart)
                start = s.advisor->start (now.brief);
            else
                changes = s.advisor->structure (now.brief);

            const juce::ScopedLock sl (s.lock);

            if (now.generation == s.generation && juce::Time::getMillisecondCounterHiRes() - now.askedAt <= s.advisor->timeLimitMs)
            {
                if (now.forStart)
                    s.startAnswer = std::move (start);
                else if (! changes.empty())
                    s.structureAnswer = std::make_pair (std::move (changes), now.brief.lock);
            }

            s.busy = false;
        }
    }

    std::shared_ptr<State> state;
};

namespace
{
    /** Renders and scores its share of a round's candidates, on a synth of its own. */
    class Worker final : public juce::Thread
    {
    public:
        Worker (std::vector<Candidate>& j, std::atomic<size_t>& n, const Target& t, std::atomic<int>& r, const std::function<bool()>& stop, bool keep)
            : juce::Thread ("Torqwave Synth decrypt worker"), jobs (j), next (n), target (t), rendered (r), shouldStop (stop), keepMeasures (keep)
        {
        }

        void run() override
        {
            const auto synth = makeSynth();

            for (auto at = next++; at < jobs.size() && ! shouldStop(); at = next++)
            {
                auto& job = jobs[at];
                const auto audio = render (*synth, job.patch, target);
                const auto mono = monoOf (audio);
                const auto silent = std::all_of (mono.begin(), mono.end(), [] (float x) { return std::abs (x) < 1.0e-6f; });

                if (silent)
                    job.distance.total = 1000.0f;
                else
                {
                    auto measures = std::make_shared<Measures> (measure (mono.data(), (int) mono.size()));
                    job.distance = distance (target.measures, *measures);

                    if (keepMeasures)
                        job.measures = std::move (measures);
                }

                job.lock = lockOf (job.distance.total);
                job.address = SignalConsole::addressOf (job.patch);
                ++rendered;
            }
        }

    private:
        std::vector<Candidate>& jobs;
        std::atomic<size_t>& next;
        const Target& target;
        std::atomic<int>& rendered;
        const std::function<bool()>& shouldStop;
        const bool keepMeasures;
    };

    /** The best `count`, one for each address, by distance and then by the order they were made in. */
    std::vector<Candidate> bestOf (std::vector<Candidate> all, size_t count)
    {
        std::vector<size_t> order (all.size());
        std::iota (order.begin(), order.end(), size_t { 0 });
        std::stable_sort (order.begin(), order.end(), [&all] (size_t a, size_t b) { return all[a].distance.total < all[b].distance.total; });
        std::vector<Candidate> best;
        std::set<juce::uint32> seen;

        for (auto i : order)
        {
            if (best.size() >= count)
                break;

            if (all[i].distance.total >= 1000.0f || ! seen.insert (all[i].address).second)
                continue;

            best.push_back (std::move (all[i]));
        }

        return best;
    }

    /** How far apart two candidates sound, by the measure they were scored with; 0 where one was never measured. */
    float apart (const Candidate& a, const Candidate& b)
    {
        return a.measures != nullptr && b.measures != nullptr ? distance (*a.measures, *b.measures).total : 0.0f;
    }

    /** A factory patch as a candidate. */
    Candidate factoryCandidate (Synth& synth, int index)
    {
        synth.loadPatch (index);
        Candidate c;
        c.patch = SignalConsole::capture (synth);
        c.from = WaveSynth::factoryPatches()[(size_t) index].name;
        return c;
    }

    /** A proposal's settings on a patch, each clamped to its parameter's range (the voice's and the
        macros' too, which the search itself doesn't move); IDs the synth doesn't have and the effects
        are left out. False when nothing it names is a setting that can change while matching. */
    bool applyChange (Patch& patch, const EffectPresets::Values& values, Synth& synth)
    {
        auto any = false;

        for (auto& [id, value] : values)
        {
            auto* p = synth.parameter (id);
            const auto part = SignalConsole::partOf (id);

            if (p == nullptr || part == SignalConsole::effects || WaveSynth::isAddedWithNine (id) || ! std::isfinite (value))
                continue;

            const auto range = p->getNormalisableRange();
            auto v = juce::jlimit ((double) range.start, (double) range.end, value);

            // A table can be Sampled only where there is one.
            if (id.endsWith ("Table") && (int) std::lround (v) == sampledIndex() && patch.tables[(size_t) juce::jlimit (0, 2, id.substring (3, 4).getIntValue() - 1)] == nullptr)
                continue;

            patch.values[id] = v;
            any = true;
        }

        return any;
    }
}

Search::Search (Target t, Options o) : target (std::move (t)), options (std::move (o))
{
    if (! options.factoryTablesOnly && target.problem.isEmpty())
        clipTable = WaveTables::sampledFromAudio (target.audio.getReadPointer (0), target.audio.getNumSamples(), rate, target.pitchHz, "Decrypted");

    progress.rounds = options.rounds;
}

Search::~Search()
{
    alive->store (false);
    stop();
    runner.reset();
}

void Search::start()
{
    if (runner != nullptr || target.problem.isNotEmpty())
        return;

    stopping = false;
    notify = true;
    started = juce::Time::getMillisecondCounterHiRes();
    runner = std::make_unique<Runner> (*this);
    runner->startThread (juce::Thread::Priority::low);
}

void Search::stop()
{
    stopping = true;

    if (options.advisor != nullptr)
        options.advisor->cancel();
}

bool Search::isRunning() const
{
    return runner != nullptr && runner->isThreadRunning();
}

Progress Search::getProgress() const
{
    const juce::ScopedLock sl (lock);
    auto p = progress;
    p.rendered = rendered.load();
    return p;
}

Progress Search::runNow (std::function<bool()> stopWhen)
{
    if (target.problem.isEmpty())
    {
        stopping = false;
        started = juce::Time::getMillisecondCounterHiRes();
        run ([this, &stopWhen] { return stopping.load() || (stopWhen != nullptr && stopWhen()); });
    }

    return getProgress();
}

std::vector<Candidate> Search::score (std::vector<Candidate> jobs, const Target& against, const std::function<bool()>& shouldStop, bool keepMeasures)
{
    std::atomic<size_t> next { 0 };
    const auto count = options.threads > 0 ? options.threads : juce::jmax (1, juce::SystemStats::getNumCpus() / 2);
    std::vector<std::unique_ptr<Worker>> workers;

    for (int i = 0; i < count; ++i)
    {
        workers.push_back (std::make_unique<Worker> (jobs, next, against, rendered, shouldStop, keepMeasures));
        workers.back()->startThread (juce::Thread::Priority::low);
    }

    for (auto& w : workers)
        w->waitForThreadToExit (-1);

    return jobs;
}

Advisor::Brief Search::briefFor (const std::vector<Candidate>& best, int round, Synth& synth) const
{
    Advisor::Brief b;
    b.target = describe (target);
    b.words = options.words;
    b.round = round;

    if (! best.empty())
    {
        const auto mono = monoOf (render (synth, best.front().patch, target));
        b.best = describeSettings (synth, best.front().patch);
        b.difference = describeDifference (target.measures, measure (mono.data(), (int) mono.size()));
        b.lock = best.front().lock;
    }

    return b;
}

void Search::growChain (const Candidate& best, bool last, Synth& synth)
{
    if (! chain.empty() && chain.back().candidate.address == best.address)
        return;

    // What the next link is heard against: the last link, or the link this search goes on from.
    if (chain.empty() && ! chainFrom.has_value() && options.chainFrom.has_value())
    {
        const auto m = measureRender (render (synth, *options.chainFrom, target));
        chainFrom = { m, distance (target.measures, m).total };
    }

    const auto* against = ! linkMeasures.empty() ? &linkMeasures.back() : chainFrom.has_value() ? &chainFrom->first : nullptr;
    const auto againstDb = ! chain.empty() ? chain.back().candidate.distance.total : chainFrom.has_value() ? chainFrom->second : 1000.0f;

    if (against != nullptr && best.distance.total >= againstDb)
        return;   // no nearer the signal than the last link

    const auto m = measureRender (render (synth, best.patch, target));
    const auto moved = against != nullptr ? distance (*against, m).total : 0.0f;

    if (against == nullptr || (moved >= linkApartDb && againstDb - best.distance.total >= linkNearerDb) || (last && chain.empty()))
    {
        // Far enough from the last link to hear and clearly nearer the signal (or the first link; or, where a
        // search carried on came nearer by less, its end, which takes the place of the link it went on from).
        chain.push_back ({ best, moved });
        linkMeasures.push_back (m);
    }
    else if (last)
    {
        // The end is always the nearest found. Too like the last link to be a link of its own, it takes that link's
        // place, and the place of any before it that it is as like: no two neighbours are nearer than linkApartDb.
        const auto from = [&] { return ! linkMeasures.empty() ? distance (linkMeasures.back(), m).total : chainFrom.has_value() ? distance (chainFrom->first, m).total : 0.0f; };

        do
        {
            chain.pop_back();
            linkMeasures.pop_back();
        }
        while (! chain.empty() && from() < linkApartDb);

        chain.push_back ({ best, from() });
        linkMeasures.push_back (m);
    }
}

void Search::noteLines (const std::vector<Candidate>& scored, const Candidate* nearest)
{
    // The nearest of each line. A line other than the nearest patch's takes a nearer one only while it
    // still sounds different from the nearest: every line comes nearer the signal by coming to sound
    // like it, and one that has come to sound like the nearest is no other answer.
    for (auto& c : scored)
    {
        if (c.distance.total >= 1000.0f || c.measures == nullptr)
            continue;

        const auto line = lines.find (c.from);

        if (line == lines.end())
            lines[c.from] = c;
        else if (c.distance.total < line->second.distance.total
                 && (nearest == nullptr || c.from == nearest->from || apart (c, *nearest) >= shownApartDb))
            line->second = c;
    }
}

std::vector<Candidate> Search::shownOf (const std::vector<Candidate>& working) const
{
    std::vector<Candidate> shown;

    if (working.empty())
        return shown;

    shown.push_back (working.front());
    std::vector<const Candidate*> rest;

    for (size_t i = 1; i < working.size(); ++i)
        rest.push_back (&working[i]);

    for (auto& line : lines)
        rest.push_back (&line.second);

    std::stable_sort (rest.begin(), rest.end(), [] (const Candidate* a, const Candidate* b) { return a->distance.total < b->distance.total; });

    for (auto* c : rest)
    {
        if (shown.size() >= (size_t) shownMost)
            break;

        if (c->distance.total < 1000.0f && c->measures != nullptr
            && std::all_of (shown.begin(), shown.end(), [c] (const Candidate& s) { return apart (s, *c) >= shownApartDb; }))
            shown.push_back (*c);
    }

    return shown;
}

void Search::publish (std::vector<Candidate> merged, int round, const juce::String& stage, bool finished, bool stopped, Synth& synth, bool noCloser)
{
    // What is shown: the nearest, and others that sound different from it and from each other. Their
    // signatures, for drawing (each rendered again: eight renders at most).
    auto shown = shownOf (merged);

    for (auto& c : shown)
        if (c.signature.peak <= 0.0f)
            c.signature = SignalConsole::Renderer::measure (render (synth, c.patch, target), rate);

    // How closely the twelve agree on each part's settings.
    std::array<float, SignalConsole::numParts> settled {};

    if (merged.size() > 1)
        for (int part = 0; part < SignalConsole::effects; ++part)
        {
            double spread = 0.0;
            int count = 0;

            for (auto& [id, value] : merged.front().patch.values)
            {
                auto* p = synth.parameter (id);

                if (p == nullptr || SignalConsole::partOf (id) != part)
                    continue;

                auto low = 1.0f, high = 0.0f;

                for (auto& c : merged)
                {
                    const auto it = c.patch.values.find (id);
                    const auto x = p->convertTo0to1 ((float) (it != c.patch.values.end() ? it->second : value));
                    low = juce::jmin (low, x);
                    high = juce::jmax (high, x);
                }

                spread += high - low;
                ++count;
            }

            settled[(size_t) part] = count > 0 ? (float) juce::jlimit (0.0, 1.0, 1.0 - 3.0 * spread / count) : 1.0f;
        }

    if (! shown.empty() && ! stopped)
        growChain (shown.front(), finished, synth);

    {
        const juce::ScopedLock sl (lock);
        progress.chain = chain;
        progress.best = std::move (shown);
        progress.round = round;
        progress.stage = stage;
        progress.finished = finished;
        progress.stopped = stopped;
        progress.noCloser = noCloser;
        progress.settled = settled;
        progress.model = modelLog;
        progress.seconds = (juce::Time::getMillisecondCounterHiRes() - started) / 1000.0;
    }

    if (notify)
        juce::MessageManager::callAsync ([this, weak = std::weak_ptr<std::atomic<bool>> (alive)]
        {
            if (const auto a = weak.lock(); a != nullptr && a->load() && onChange != nullptr)
                onChange();
        });
}

void Search::run (const std::function<bool()>& shouldStop)
{
    const auto synth = makeSynth();
    const auto& factory = WaveSynth::factoryPatches();
    const auto startable = [this, &factory] (int i) { return ! options.leaveOut.contains (factory[(size_t) i].name); };
    std::unique_ptr<Helper> helper;
    std::array<bool, SignalConsole::numParts> modelFirewall {};
    auto haveModelFirewall = false;

    if (options.advisor != nullptr)
    {
        helper = std::make_unique<Helper> (options.advisor);
        auto brief = briefFor ({}, 0, *synth);
        brief.clip = &target.audio;
        helper->ask (true, std::move (brief));
    }

    // Where the advisor said to start: the patches and categories it named that the search didn't
    // already take, as candidates (twelve at most), and the parts it would hold.
    const auto startsFrom = [&] (const Advisor::Start& s, const std::vector<Candidate>& have)
    {
        std::vector<Candidate> added;
        std::set<juce::String> taken;

        for (auto& c : have)
            taken.insert (c.from);

        for (int i = 0; i < (int) factory.size() && added.size() < 12; ++i)
        {
            const auto& f = factory[(size_t) i];

            if (startable (i) && taken.count (f.name) == 0 && (s.patches.contains (f.name, true) || s.categories.contains (f.category, true)))
            {
                added.push_back (factoryCandidate (*synth, i));
                added.back().from = "model: " + f.name;
                taken.insert (f.name);
            }
        }

        juce::StringArray held;

        for (int part = 0; part < SignalConsole::effects; ++part)
            if (s.firewall[(size_t) part])
            {
                modelFirewall[(size_t) part] = haveModelFirewall = true;
                held.add (SignalConsole::partNames()[part].toLowerCase());
            }

        modelFirewall[SignalConsole::effects] = true;
        modelLog.add ("start: " + juce::String ((int) added.size()) + " patches added"
                      + (held.isEmpty() ? juce::String() : ", holding " + held.joinIntoString (" and "))
                      + (s.heard.isNotEmpty() ? "; heard: " + s.heard : juce::String()));
        return added;
    };

    std::vector<Candidate> pool, further;
    const auto resumed = ! options.resume.empty();

    if (resumed)
    {
        // A search carried on: where the last one ended.
        for (auto& patch : options.resume)
        {
            Candidate c;
            c.patch = patch;
            const auto at = (int) pool.size();
            c.from = at < options.resumeFrom.size() && options.resumeFrom[at].isNotEmpty() ? options.resumeFrom[at] : juce::String ("the last search");
            pool.push_back (std::move (c));
        }
    }
    else
    {
        // The factory patches, over the clip's first second.
        std::vector<Candidate> jobs;

        for (int i = 0; i < (int) factory.size(); ++i)
            if (startable (i))
                jobs.push_back (factoryCandidate (*synth, i));

        // A first look at one in 64 of them, a closer look at one in 8, then all: the best of each look
        // is shown as it is found, so the chain starts with rough matches and not with the nearest of
        // thousands. What is scored, and in what order it is ranked, is the same as scoring them at once.
        const auto first = shortened (target, firstRoundsLength);
        std::vector<Candidate> looks;   // the nearest of each look, over the whole note
        size_t looked = 0;
        std::vector<bool> done (jobs.size(), false);

        for (const auto stride : { size_t { 64 }, size_t { 8 }, size_t { 1 } })
        {
            std::vector<size_t> at;
            std::vector<Candidate> part;

            for (size_t i = 0; i < jobs.size(); ++i)
                if (i % stride == 0 && ! done[i])
                {
                    done[i] = true;
                    at.push_back (i);
                    part.push_back (std::move (jobs[i]));
                }

            part = score (std::move (part), first, shouldStop, false);   // thousands of them: their sounds are not kept

            for (size_t k = 0; k < at.size(); ++k)
                jobs[at[k]] = std::move (part[k]);

            looked += at.size();

            if (shouldStop())
            {
                publish (bestOf (std::move (jobs), keptBest), 0, "Stopped", true, true, *synth);
                return;
            }

            if (stride > 1 && looked >= keptBest)
            {
                auto look = bestOf (jobs, keptBest);

                for (auto& c : look)
                    c.signature = {};

                look = bestOf (score (std::move (look), target, shouldStop), keptBest);

                if (shouldStop())
                {
                    publish (std::move (look), 0, "Stopped", true, true, *synth);
                    return;
                }

                if (! look.empty())
                {
                    looks.push_back (look.front());
                    publish (std::move (look), 0, stride == 64 ? "A first look" : "A closer look", false, false, *synth);
                }
            }
        }

        // Other lines to choose from: the 48 nearest are often one kind of patch, as like each other as
        // they are like the signal, so every eighth of the next five hundred joins them, less near and
        // more different. They are scored over the whole note with the rest; the nearest of them that
        // sound different from the best are shown beside it and grown.
        {
            std::vector<size_t> order (jobs.size());
            std::iota (order.begin(), order.end(), size_t { 0 });
            std::stable_sort (order.begin(), order.end(), [&jobs] (size_t a, size_t b) { return jobs[a].distance.total < jobs[b].distance.total; });

            for (size_t at = 48 + 8; at < order.size() && further.size() < 64; at += 8)
                if (jobs[order[at]].distance.total < 1000.0f)
                    further.push_back (jobs[order[at]]);
        }

        // The 48 nearest over the whole note, with the clip's own table tried on the best six.
        pool = bestOf (std::move (jobs), 48);

        // The nearest of each look stays in, wherever its first second ranked it: the chain has shown it.
        for (auto& l : looks)
            if (std::none_of (pool.begin(), pool.end(), [&l] (const Candidate& c) { return c.address == l.address; }))
                pool.push_back (l);

        for (auto& c : pool)
            c.signature = {};
    }

    if (clipTable != nullptr && ! resumed)
        for (size_t i = 0; i < 6 && i < pool.size(); ++i)
            for (auto alone : { false, true })
            {
                Candidate c;
                c.patch = withClipTable (pool[i].patch, clipTable, alone, (double) target.audio.getNumSamples() / rate);
                c.from = pool[i].from;
                pool.push_back (std::move (c));
            }

    pool.insert (pool.end(), std::make_move_iterator (further.begin()), std::make_move_iterator (further.end()));
    auto startAnswered = false;

    if (helper != nullptr)
        if (auto s = helper->takeStart())
        {
            startAnswered = true;
            auto added = startsFrom (*s, pool);
            pool.insert (pool.end(), std::make_move_iterator (added.begin()), std::make_move_iterator (added.end()));
        }

    pool = score (std::move (pool), target, shouldStop);

    // The nearest of each line, as it starts.
    {
        std::vector<Candidate> byDistance = pool;
        std::stable_sort (byDistance.begin(), byDistance.end(), [] (const Candidate& a, const Candidate& b) { return a.distance.total < b.distance.total; });
        noteLines (byDistance, nullptr);
    }

    auto best = bestOf (std::move (pool), keptBest);
    publish (best, 0, resumed ? "The last search" : "Factory patches", false, false, *synth);

    if (helper != nullptr && ! shouldStop() && startAnswered)
        helper->ask (false, briefFor (best, 0, *synth));

    // Rounds: twelve variations of each of the best four, each round a smaller step.
    std::array<bool, SignalConsole::numParts> firewall {};
    firewall[SignalConsole::effects] = true;
    int lastRound = 0;

    // What the advisor has sent since it was last asked: a late start, or changes of structure to the best.
    const auto takeProposals = [&]
    {
        std::vector<Candidate> proposed;
        helper->expire();

        if (! startAnswered)
            if (auto s = helper->takeStart())
            {
                startAnswered = true;
                proposed = startsFrom (*s, best);
            }

        if (auto changes = helper->takeStructure(); changes && ! best.empty())
            for (size_t i = 0; i < changes->first.size() && i < 5; ++i)
            {
                auto& change = changes->first[i];
                Candidate c;
                c.patch = best.front().patch;
                c.from = "model: " + change.what;

                if (applyChange (c.patch, change.values, *synth))
                    proposed.push_back (std::move (c));
                else
                    modelLog.add ("ignored " + change.what.quoted() + ": it names no setting that can change");
            }

        return proposed;
    };

    // Each proposal is scored with a few variations of its own (a change of structure rarely lands
    // with its knobs right), and joins the candidates only if the best of them beats the best so far:
    // a proposal that doesn't come closer changes nothing.
    const auto tryProposals = [&] (std::vector<Candidate> proposed, std::vector<Candidate>& next, juce::uint32 seed)
    {
        const auto lockBefore = best.empty() ? 0.0f : best.front().lock;

        for (size_t i = 0; i < proposed.size() && ! shouldStop(); ++i)
        {
            std::vector<Candidate> family { proposed[i] };

            for (auto& v : SignalConsole::variations (proposed[i].patch, *synth, 0.3f, mix (seed + (juce::uint32) i * 0x27d4eb2du), firewall, perParent))
            {
                Candidate c;
                c.patch = std::move (v);
                c.from = proposed[i].from;
                family.push_back (std::move (c));
            }

            family = score (std::move (family), target, shouldStop);

            if (shouldStop())
                return;

            auto top = 0.0f;

            for (auto& c : family)
                top = juce::jmax (top, c.lock);

            const auto kept = top > lockBefore;
            modelLog.add ("tried " + proposed[i].from.fromFirstOccurrenceOf ("model: ", false, false).quoted() + ": "
                          + (kept ? "kept, lock " + juce::String (lockBefore, 0) + " to " + juce::String (top, 0) + " %"
                                  : "dropped, lock " + juce::String (top, 0) + " % against " + juce::String (lockBefore, 0) + " %"));

            if (kept)
                next.insert (next.end(), std::make_move_iterator (family.begin()), std::make_move_iterator (family.end()));
        }
    };

    // Fixed rounds, or (untilSettled) on past them while they come closer.
    int quietRounds = 0;
    auto noCloser = false;
    const auto goesOn = [&] (int round)
    {
        if (round <= options.rounds)
            return true;

        return options.untilSettled && round <= options.maxRounds && quietRounds < options.patience;
    };

    int sweepAt = 0;

    for (int round = 1; goesOn (round) && ! shouldStop(); ++round)
    {
        // Carried on, the steps are smaller from the first: it is refining, not looking for somewhere to start.
        const auto widest = resumed ? 0.25f : 0.45f, finest = resumed ? 0.06f : 0.15f;
        // Past its set rounds the steps go round the last quarter of the schedule again: fine, then finer.
        const auto place = round <= options.rounds ? (float) (round - 1) / (float) juce::jmax (1, options.rounds - 1)
                                                   : 0.75f + 0.25f * (float) ((round - options.rounds - 1) % 4) / 3.0f;
        const auto reach = widest * std::pow (finest / widest, place);
        const auto lockBeforeRound = best.empty() ? 0.0f : best.front().lock;
        std::vector<Candidate> next = best;
        const auto proposed = helper != nullptr ? takeProposals() : std::vector<Candidate>();

        for (size_t p = 0; p < (size_t) parents && p < best.size(); ++p)
        {
            const auto seed = mix (options.seed * 0x9e3779b9u + (juce::uint32) round * 0x85ebca6bu + (juce::uint32) p * 0xc2b2ae35u);

            for (auto& v : SignalConsole::variations (best[p].patch, *synth, reach, seed, firewall, perParent))
            {
                Candidate c;
                c.patch = std::move (v);
                c.from = best[p].from;
                next.push_back (std::move (c));
            }

            // For the best two, steps that move only one to three settings: a patch with many settings
            // (three layers, sixteen modulation routes, step patterns) is rarely improved by moving all
            // of a part at once.
            if (p < 2)
                for (auto& v : SignalConsole::fewMoves (best[p].patch, *synth, juce::jmin (1.0f, reach * 1.5f), seed ^ 0x46455721u, firewall, perParent))
                {
                    Candidate c;
                    c.patch = std::move (v);
                    c.from = best[p].from;
                    next.push_back (std::move (c));
                }

            // The parts the advisor would hold: as many variations again of the best, with them held.
            if (p == 0 && haveModelFirewall)
                for (auto& v : SignalConsole::variations (best[p].patch, *synth, reach, seed ^ 0x4d4f444cu, modelFirewall, perParent))
                {
                    Candidate c;
                    c.patch = std::move (v);
                    c.from = best[p].from;
                    next.push_back (std::move (c));
                }
        }

        // The other lines: the nearest few that sound different from the best and from each other
        // (those shown beside it) are grown too, half as many variations each, so that what is shown
        // beside the best are other answers come as near as they can, and not the best's own copies.
        {
            const auto others = shownOf (best);

            for (size_t o = 1; o < others.size() && o <= (size_t) otherLines; ++o)
            {
                const auto seed = mix (options.seed * 0x2545f491u + (juce::uint32) round * 0x85ebca6bu + (juce::uint32) o * 0x9e3779b9u);

                for (auto& v : SignalConsole::variations (others[o].patch, *synth, reach, seed, firewall, perOtherLine))
                {
                    Candidate c;
                    c.patch = std::move (v);
                    c.from = others[o].from;
                    next.push_back (std::move (c));
                }
            }
        }

        // Fitted steps on the best: a few of its settings each taken across its whole range, the
        // next few each round, round and round. A random step moves many settings a little; this
        // finds the cutoff, level or envelope time that a recording needs, however far it is.
        if (! best.empty())
        {
            int sweepable = 0;

            for (auto& v : SignalConsole::sweeps (best.front().patch, *synth, firewall, sweepAt, sweptPerRound, 9, sweepable))
            {
                Candidate c;
                c.patch = std::move (v);
                c.from = best.front().from;
                next.push_back (std::move (c));
            }

            sweepAt = sweepable > 0 ? (sweepAt + sweptPerRound) % sweepable : 0;
        }

        // Those already scored keep their scores; the new ones are scored now.
        std::vector<Candidate> fresh (next.begin() + (std::ptrdiff_t) best.size(), next.end());
        fresh = score (std::move (fresh), target, shouldStop);

        if (shouldStop())
            break;

        std::copy (std::make_move_iterator (fresh.begin()), std::make_move_iterator (fresh.end()), next.begin() + (std::ptrdiff_t) best.size());
        tryProposals (proposed, next, mix (options.seed + (juce::uint32) round * 0x165667b1u));

        if (shouldStop())
            break;

        noteLines (next, best.empty() ? nullptr : &best.front());
        best = bestOf (std::move (next), keptBest);
        lastRound = round;
        quietRounds = ! best.empty() && best.front().lock > lockBeforeRound + 0.1f ? 0 : quietRounds + 1;
        noCloser = options.untilSettled && quietRounds >= options.patience;

        if (helper != nullptr)
            helper->ask (! startAnswered, briefFor (best, round, *synth));

        publish (best, round, options.untilSettled ? "Round " + juce::String (round) + (quietRounds > 0 ? ", no closer for " + juce::String (quietRounds) : juce::String())
                                                   : "Round " + juce::String (round) + " of " + juce::String (options.rounds), false, false, *synth);
    }

    // A model answers in seconds, the search in about as long: when the rounds are done, up to three
    // more of its answers are waited for (no longer than its time limit in all) and tried on the best.
    if (helper != nullptr && ! shouldStop())
    {
        const auto until = juce::Time::getMillisecondCounterHiRes() + options.advisor->timeLimitMs;

        for (int answer = 0; answer < 3 && ! shouldStop() && juce::Time::getMillisecondCounterHiRes() < until; ++answer)
        {
            publish (best, lastRound, "Waiting for the model", false, false, *synth);

            while (! shouldStop() && helper->isBusy() && juce::Time::getMillisecondCounterHiRes() < until)
            {
                helper->expire();
                juce::Thread::sleep (20);
            }

            if (shouldStop() || helper->isBusy())
                break;

            const auto proposed = takeProposals();

            if (! proposed.empty())
            {
                std::vector<Candidate> next = best;
                tryProposals (proposed, next, mix (options.seed + (juce::uint32) (lastRound + 1 + answer) * 0x165667b1u));

                if (shouldStop())
                    break;

                noteLines (next, best.empty() ? nullptr : &best.front());
                best = bestOf (std::move (next), keptBest);
            }

            helper->ask (! startAnswered, briefFor (best, lastRound, *synth));
        }
    }

    helper.reset();
    const auto settledNow = noCloser && ! shouldStop();
    publish (best, lastRound, shouldStop() ? "Stopped" : settledNow ? "No closer found after " + juce::String (lastRound) + " rounds" : "Done", true, shouldStop(), *synth, settledNow);
}
}
