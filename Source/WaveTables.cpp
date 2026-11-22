#include "WaveTables.h"

namespace WaveTables
{
namespace
{
    constexpr int sourceOrder = 13;                  // each frame is drawn at 8,192 points
    constexpr int sourceSize = 1 << sourceOrder;
    constexpr double twoPi = juce::MathConstants<double>::twoPi;

    double frac (double x)                           { return x - std::floor (x); }
    double saw (double p)                            { return 1.0 - 2.0 * frac (p); }   // falling: its fundamental in phase with a sine
    double square (double p)                         { return frac (p) < 0.5 ? 1.0 : -1.0; }
    double triangle (double p)                       { const auto f = frac (p + 0.25); return f < 0.5 ? 4.0 * f - 1.0 : 3.0 - 4.0 * f; }

    /** One cycle of a table's frame at position t (0 to 1), sampled at sourceSize points. */
    void drawSource (Table table, double t, std::vector<double>& out)
    {
        out.assign (sourceSize, 0.0);

        for (int i = 0; i < sourceSize; ++i)
        {
            const auto p = (double) i / sourceSize;
            double s = 0.0;

            switch (table)
            {
                case Table::analog:
                {
                    // Sine, triangle, sawtooth and square, a third of the way apart, each blended into the next.
                    const auto x = t * 3.0;
                    const auto seg = juce::jmin (2, (int) x);
                    const auto mix = x - seg;
                    const double shapes[] = { std::sin (twoPi * p), triangle (p), saw (p), square (p) };
                    s = shapes[seg] * (1.0 - mix) + shapes[seg + 1] * mix;
                    break;
                }

                case Table::pulse:
                {
                    // A pulse from square (50 %) down to a thin 3 % needle.
                    const auto width = 0.5 - 0.47 * t;
                    s = (frac (p) < width ? 1.0 : -1.0) + (2.0 * width - 1.0);   // centred
                    break;
                }

                case Table::vowel:
                    break;   // made in the frequency domain below

                case Table::fm:
                {
                    // Two operators, the modulator at twice then three times the carrier, its index rising to 9.
                    const auto index = 9.0 * t * t;
                    const auto ratio = t < 0.5 ? 1.0 : 2.0;
                    s = std::sin (twoPi * p + index * std::sin (twoPi * ratio * p));
                    break;
                }

                case Table::metal:
                    break;   // frequency domain

                case Table::sub:
                {
                    // A sine saturating into a rounded square.
                    const auto drive = 1.0 + 12.0 * t * t;
                    s = std::tanh (drive * std::sin (twoPi * p)) / std::tanh (drive);
                    break;
                }

                case Table::additive:
                    break;   // frequency domain

                case Table::sync:
                {
                    // A sawtooth hard-synced to the cycle, its own rate swept from 1 to 9 times.
                    const auto ratio = 1.0 + 8.0 * t;
                    s = saw (p * ratio);
                    break;
                }

                case Table::growl:
                {
                    // A sine folded and driven into growling, buzzing shapes.
                    const auto drive = 1.0 + 7.0 * t;
                    const auto x = drive * (std::sin (twoPi * p) + 0.45 * t * std::sin (twoPi * 2.0 * p + 1.3));
                    s = std::sin (x) * 0.6 + std::tanh (x) * 0.4;
                    break;
                }

                case Table::string:
                    s = saw (p);   // drawn only
                    break;

                case Table::sampled:
                    s = saw (p);   // what a synth without a table of its own plays
                    break;

                case Table::count:
                    break;
            }

            out[(size_t) i] = s;
        }
    }

    /** For the tables made from harmonics: each harmonic's amplitude at position t (sine phase). */
    bool harmonicsFor (Table table, double t, std::vector<double>& amp)
    {
        amp.assign (maxHarmonics, 0.0);

        if (table == Table::vowel)
        {
            // Formants for a voice on a low note, moving through the vowels A, E, I, O, U and back to A.
            struct Vowel { double f1, f2, f3; };
            static const Vowel vowels[] = { { 800, 1150, 2900 }, { 400, 1600, 2700 }, { 300, 2250, 3000 }, { 450, 800, 2830 }, { 325, 700, 2530 }, { 800, 1150, 2900 } };
            const auto x = t * 5.0;
            const auto seg = juce::jmin (4, (int) x);
            const auto mix = x - seg;
            const auto lerp = [mix] (double a, double b) { return a + (b - a) * mix; };
            const Vowel v { lerp (vowels[seg].f1, vowels[seg + 1].f1), lerp (vowels[seg].f2, vowels[seg + 1].f2), lerp (vowels[seg].f3, vowels[seg + 1].f3) };
            constexpr double fundamental = 98.0;   // the formants sit at these harmonics, so they move with the note as a table's do

            for (int h = 1; h <= maxHarmonics; ++h)
            {
                const auto f = h * fundamental;
                const auto peak = [f] (double centre, double width, double gain) { const auto d = (f - centre) / width; return gain * std::exp (-0.5 * d * d); };
                amp[(size_t) h - 1] = (peak (v.f1, 90.0, 1.0) + peak (v.f2, 120.0, 0.6) + peak (v.f3, 160.0, 0.3)) / std::sqrt ((double) h) + 0.02 / h;
            }

            return true;
        }

        if (table == Table::metal)
        {
            // Bell and plate partials, stretched as the position rises, each on its nearest harmonic of a low fundamental.
            static const double ratios[] = { 1.0, 2.76, 5.40, 8.93, 13.34, 18.64, 24.81, 4.17, 6.71, 11.2 };
            const auto stretch = 1.0 + 3.0 * t;

            for (int i = 0; i < (int) std::size (ratios); ++i)
            {
                const auto h = (int) std::lround (ratios[i] * (i == 0 ? 1.0 : stretch) * 4.0);   // a fundamental two octaves under the note keeps them inharmonic-sounding
                if (h >= 1 && h <= maxHarmonics)
                    amp[(size_t) h - 1] += 1.0 / (1.0 + 0.35 * i);
            }

            // Ring-modulated sidebands as it goes on: clang.
            for (int k = 1; k < 7; ++k)
            {
                const auto h = (int) std::lround (4.0 * (1.0 + t * 9.0) * k);
                for (auto side : { h - 4, h + 4 })
                    if (side >= 1 && side <= maxHarmonics)
                        amp[(size_t) side - 1] += 0.5 * t / k;
            }

            return true;
        }

        if (table == Table::additive)
        {
            // Harmonics added one at a time (up to 48), odd ones a little stronger, like drawbars pulled out.
            const auto count = 1.0 + t * 47.0;

            for (int h = 1; h <= 48; ++h)
            {
                const auto in = juce::jlimit (0.0, 1.0, count - (h - 1));
                amp[(size_t) h - 1] = in * (h % 2 == 1 ? 1.0 : 0.7) / std::pow ((double) h, 0.8);
            }

            return true;
        }

        return false;
    }

    struct Builder
    {
        juce::dsp::FFT sourceFft { sourceOrder };
        std::vector<std::unique_ptr<juce::dsp::FFT>> inverse;   // by order
        std::vector<double> drawn, amps;
        std::vector<float> work;

        Builder()
        {
            for (int order = 0; order <= sourceOrder; ++order)
                inverse.push_back (std::make_unique<juce::dsp::FFT> (order));
        }

        /** Each harmonic's complex value, 1 to maxHarmonics (index h - 1). */
        void spectrum (Table table, double t, std::vector<std::complex<float>>& bins)
        {
            bins.assign (maxHarmonics, {});

            if (harmonicsFor (table, t, amps))
            {
                // Sines in phase, each harmonic's (sin = -i/2 in the complex bin, scaled by the FFT's size below).
                for (int h = 0; h < maxHarmonics; ++h)
                    bins[(size_t) h] = std::complex<float> (0.0f, (float) -amps[(size_t) h]);
                return;
            }

            drawSource (table, t, drawn);
            std::vector<std::complex<float>> in ((size_t) sourceSize), out ((size_t) sourceSize);

            for (int i = 0; i < sourceSize; ++i)
                in[(size_t) i] = { (float) drawn[(size_t) i], 0.0f };

            sourceFft.perform (in.data(), out.data(), false);

            for (int h = 0; h < maxHarmonics; ++h)
                bins[(size_t) h] = out[(size_t) h + 1] * (2.0f / (float) sourceSize);
        }

        /** A level's frame from the spectrum: harmonics up to `keep`, at `size` points. */
        void synthesise (const std::vector<std::complex<float>>& bins, int keep, int size, float* dest)
        {
            const auto order = juce::roundToInt (std::log2 ((double) size));
            std::vector<std::complex<float>> spec ((size_t) size), time ((size_t) size);

            for (int h = 1; h <= keep && h < size / 2; ++h)
            {
                spec[(size_t) h] = bins[(size_t) h - 1] * 0.5f;
                spec[(size_t) (size - h)] = std::conj (bins[(size_t) h - 1]) * 0.5f;
            }

            inverse[(size_t) order]->perform (spec.data(), time.data(), true);

            // JUCE's inverse divides by the size; undo it, so a harmonic of amplitude a peaks at a.
            for (int i = 0; i < size; ++i)
                dest[i] = time[(size_t) i].real() * (float) size;

            dest[size] = dest[0];
        }
    };

    std::array<Bank, (size_t) Table::count>& banks()
    {
        static std::array<Bank, (size_t) Table::count> all;
        return all;
    }

    void build()
    {
        Builder builder;
        std::vector<std::complex<float>> bins;

        for (int ti = 0; ti < (int) Table::count; ++ti)
        {
            auto& bank = banks()[(size_t) ti];

            for (int L = 0; L < numLevels; ++L)
            {
                auto& level = bank.levels[(size_t) L];
                level.harmonics = maxHarmonics >> L;
                // At least 1024 points, so a top level is read 64 times oversampled or more: its
                // linear interpolation's images stay near -80 dB.
                level.size = L <= 1 ? 2048 : juce::jmax (1024, level.harmonics * 4);
                level.samples.assign ((size_t) numFrames * (size_t) (level.size + 1), 0.0f);
            }

            for (int f = 0; f < numFrames; ++f)
            {
                const auto t = (double) f / (numFrames - 1);
                builder.spectrum ((Table) ti, t, bins);

                // Each frame peaks at 1 with all its harmonics; every octave's copy shares that gain,
                // so a note sounds as loud whichever level plays it.
                std::vector<float> full (2049);
                builder.synthesise (bins, maxHarmonics, 2048, full.data());
                auto peak = 0.0f;

                for (int i = 0; i < 2048; ++i)
                    peak = juce::jmax (peak, std::abs (full[(size_t) i]));

                const auto gain = peak > 1.0e-6f ? 1.0f / peak : 0.0f;

                for (int L = 0; L < numLevels; ++L)
                {
                    auto& level = bank.levels[(size_t) L];
                    auto* dest = level.samples.data() + (size_t) f * (size_t) (level.size + 1);
                    builder.synthesise (bins, level.harmonics, level.size, dest);
                    juce::FloatVectorOperations::multiply (dest, gain, level.size + 1);
                }
            }
        }
    }
}

const juce::StringArray& names()
{
    static const juce::StringArray n { "Analog", "Pulse", "Vowel", "FM", "Metal", "Sub", "Additive", "Sync", "Growl", "String", "Sampled" };
    return n;
}

const Bank& get (Table t)
{
    static std::once_flag once;
    std::call_once (once, build);
    return banks()[(size_t) juce::jlimit (0, (int) Table::count - 1, (int) t)];
}

std::vector<float> drawFrame (Table table, float position, int points)
{
    return drawFrame (get (table), position, points);
}

std::vector<float> drawFrame (const Bank& bank, float position, int points)
{
    const auto& level = bank.levels[2];   // 256 harmonics: plenty for the eye
    std::vector<float> out ((size_t) juce::jmax (2, points));

    for (int i = 0; i < (int) out.size(); ++i)
        out[(size_t) i] = read (level, position, (double) i / (double) out.size());

    return out;
}

//==============================================================================
namespace
{
    constexpr float sampledRangeDb = 96.0f;

    /** The levels of a sampled table's bank, from its strengths. */
    void buildSampledBank (Sampled& t)
    {
        Builder builder;
        std::vector<std::complex<float>> bins;

        for (int L = 0; L < numLevels; ++L)
        {
            auto& level = t.bank.levels[(size_t) L];
            level.harmonics = maxHarmonics >> L;
            level.size = L <= 1 ? 2048 : juce::jmax (1024, level.harmonics * 4);
            level.frames = t.frames;
            level.samples.assign ((size_t) t.frames * (size_t) (level.size + 1), 0.0f);
        }

        for (int f = 0; f < t.frames; ++f)
        {
            bins.assign (maxHarmonics, {});

            for (int h = 0; h < Sampled::harmonics; ++h)
            {
                const auto step = t.strengths[(size_t) f * Sampled::harmonics + (size_t) h];

                if (step == 0)
                    continue;

                const auto amplitude = std::pow (10.0f, ((float) step / 65535.0f - 1.0f) * sampledRangeDb / 20.0f);
                // Schroeder's phases: harmonic k at pi k (k - 1) / N, which spreads the peaks out.
                const auto phase = juce::MathConstants<double>::pi * (double) h * (double) (h + 1) / (double) Sampled::harmonics;
                bins[(size_t) h] = std::polar (amplitude, (float) std::fmod (phase, twoPi));
            }

            std::vector<float> full (2049);
            builder.synthesise (bins, maxHarmonics, 2048, full.data());
            auto peak = 0.0f;

            for (int i = 0; i < 2048; ++i)
                peak = juce::jmax (peak, std::abs (full[(size_t) i]));

            const auto gain = peak > 1.0e-6f ? 1.0f / peak : 0.0f;

            for (int L = 0; L < numLevels; ++L)
            {
                auto& level = t.bank.levels[(size_t) L];
                auto* dest = level.samples.data() + (size_t) f * (size_t) (level.size + 1);
                builder.synthesise (bins, level.harmonics, level.size, dest);
                juce::FloatVectorOperations::multiply (dest, gain, level.size + 1);
            }
        }
    }

    std::shared_ptr<const Sampled> finish (std::shared_ptr<Sampled> t)
    {
        get (Table::analog);   // the shared FFT sizes and tables exist before a sampled one is built
        buildSampledBank (*t);
        return t;
    }
}

std::shared_ptr<const Sampled> makeSampled (const std::vector<float>& amplitudes, int frames, double rootHz, const juce::String& name)
{
    if (frames < 1 || (int) amplitudes.size() < frames * Sampled::harmonics)
        return nullptr;

    auto t = std::make_shared<Sampled>();
    const auto single = frames == 1;
    t->frames = juce::jlimit (2, Sampled::maxFrames, frames);
    t->rootHz = rootHz;
    t->name = name;
    t->strengths.assign ((size_t) t->frames * Sampled::harmonics, 0);

    for (int f = 0; f < t->frames; ++f)
    {
        const auto* from = amplitudes.data() + (size_t) (single ? 0 : f) * Sampled::harmonics;
        auto most = 0.0f;

        for (int h = 0; h < Sampled::harmonics; ++h)
            most = juce::jmax (most, std::abs (from[h]));

        if (most <= 0.0f || ! std::isfinite (most))
            continue;

        for (int h = 0; h < Sampled::harmonics; ++h)
        {
            const auto db = juce::Decibels::gainToDecibels (std::abs (from[h]) / most, -200.0f);
            const auto step = juce::roundToInt ((db + sampledRangeDb) / sampledRangeDb * 65535.0f);
            t->strengths[(size_t) f * Sampled::harmonics + (size_t) h] = (juce::uint16) juce::jlimit (0, 65535, step);
        }
    }

    return finish (std::move (t));
}

juce::String Sampled::toText() const
{
    juce::MemoryBlock block (strengths.size() * 2);
    auto* bytes = static_cast<juce::uint8*> (block.getData());

    for (size_t i = 0; i < strengths.size(); ++i)
    {
        bytes[i * 2] = (juce::uint8) (strengths[i] & 0xff);
        bytes[i * 2 + 1] = (juce::uint8) (strengths[i] >> 8);
    }

    return block.toBase64Encoding();
}

std::shared_ptr<const Sampled> Sampled::fromText (const juce::String& text, int frames, double rootHz, const juce::String& name)
{
    juce::MemoryBlock block;

    if (frames < 2 || frames > maxFrames || ! block.fromBase64Encoding (text) || block.getSize() != (size_t) frames * harmonics * 2)
        return nullptr;

    auto t = std::make_shared<Sampled>();
    t->frames = frames;
    t->rootHz = rootHz;
    t->name = name;
    t->strengths.resize ((size_t) frames * harmonics);
    const auto* bytes = static_cast<const juce::uint8*> (block.getData());

    for (size_t i = 0; i < t->strengths.size(); ++i)
        t->strengths[i] = (juce::uint16) (bytes[i * 2] | (bytes[i * 2 + 1] << 8));

    return finish (std::move (t));
}

double findPitch (const float* samples, int numSamples, double sampleRate)
{
    // The middle of the sound, up to 8,192 samples of it.
    const auto length = juce::jmin (numSamples, 8192);
    const auto* x = samples + (numSamples - length) / 2;
    const auto shortest = juce::jmax (2, (int) (sampleRate / 2000.0)), longest = juce::jmin (length / 2, (int) (sampleRate / 30.0));

    if (longest <= shortest + 2)
        return 0.0;

    std::vector<double> r ((size_t) longest + 2, 0.0);
    auto best = 0.0;

    for (int lag = shortest - 1; lag <= longest + 1; ++lag)
    {
        double sum = 0.0, a = 0.0, b = 0.0;

        for (int i = 0; i + lag < length; ++i)
        {
            sum += (double) x[i] * x[i + lag];
            a += (double) x[i] * x[i];
            b += (double) x[i + lag] * x[i + lag];
        }

        r[(size_t) lag] = a > 0.0 && b > 0.0 ? sum / std::sqrt (a * b) : 0.0;
        best = juce::jmax (best, r[(size_t) lag]);
    }

    if (best < 0.6)
        return 0.0;

    // The shortest lag that is a peak nearly as high as the highest: the period, not a multiple of it.
    for (int lag = shortest; lag <= longest; ++lag)
        if (r[(size_t) lag] >= 0.9 * best && r[(size_t) lag] >= r[(size_t) lag - 1] && r[(size_t) lag] >= r[(size_t) lag + 1])
        {
            const auto a = r[(size_t) lag - 1], b = r[(size_t) lag], c = r[(size_t) lag + 1];
            const auto bend = a - 2.0 * b + c;
            const auto offset = std::abs (bend) > 1.0e-12 ? 0.5 * (a - c) / bend : 0.0;
            return sampleRate / ((double) lag + juce::jlimit (-0.5, 0.5, offset));
        }

    return 0.0;
}

std::shared_ptr<const Sampled> sampledFromAudio (const float* samples, int numSamples, double sampleRate, double rootHz, const juce::String& name)
{
    if (samples == nullptr || sampleRate <= 0.0 || numSamples < (int) (sampleRate * 0.1))
        return nullptr;

    if (rootHz <= 0.0)
        rootHz = findPitch (samples, numSamples, sampleRate);

    if (rootHz <= 0.0)
        rootHz = sampleRate / 2048.0;

    // A window two periods of the root long: a partial at a harmonic is read in full and its
    // neighbours not at all, and one between two harmonics is shared between them.
    constexpr int order = 14, size = 1 << order;
    const auto window = juce::jlimit (64, juce::jmin (size, numSamples), juce::roundToInt (2.0 * sampleRate / rootHz));
    const auto frames = juce::jlimit (2, Sampled::maxFrames, (numSamples - window) / juce::jmax (1, window / 4) + 1);
    juce::dsp::FFT fft (order);
    std::vector<std::complex<float>> in ((size_t) size), out ((size_t) size);
    std::vector<float> amplitudes ((size_t) frames * Sampled::harmonics, 0.0f);

    for (int f = 0; f < frames; ++f)
    {
        const auto start = (int) ((double) f / (double) (frames - 1) * (double) (numSamples - window));
        std::fill (in.begin(), in.end(), std::complex<float>());
        double weight = 0.0;

        for (int i = 0; i < window; ++i)
        {
            const auto w = 0.5 - 0.5 * std::cos (twoPi * (i + 0.5) / window);
            in[(size_t) i] = { (float) (samples[start + i] * w), 0.0f };
            weight += w;
        }

        fft.perform (in.data(), out.data(), false);

        for (int h = 0; h < Sampled::harmonics; ++h)
        {
            const auto bin = (double) (h + 1) * rootHz * size / sampleRate;

            if (bin >= size / 2 - 1)
                break;

            const auto i = (int) bin;
            const auto t = (float) (bin - i);
            const auto magnitude = std::abs (out[(size_t) i]) + t * (std::abs (out[(size_t) i + 1]) - std::abs (out[(size_t) i]));
            amplitudes[(size_t) f * Sampled::harmonics + (size_t) h] = (float) (2.0 * magnitude / weight);
        }
    }

    return makeSampled (amplitudes, frames, rootHz, name);
}
}
