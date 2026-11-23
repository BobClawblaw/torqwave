#include "Sampler.h"
#include "AudioFileIO.h"
#include "FadeShape.h"
#include "PhaseVocoder.h"
#include "WaveTables.h"
#include "Session.h"

namespace Sampler
{
namespace
{
    //==============================================================================
    // The windowed sinc a sample is read through. A bank for each twelfth of an octave a sample is
    // played above its own rate, up to two octaves: each narrows the band to what the output can
    // hold and is as much longer. A bank holds the points for 256 places between two frames (and
    // the next frame's, so a place between two of those is a blend of neighbouring rows).
    constexpr int phases = 256, pointsEachSide = 24, banksPerOctave = 12, numBanks = 2 * banksPerOctave + 1;
    constexpr int mostPoints = 2 * pointsEachSide * 4;   // the longest bank's: two octaves up

    struct Bank
    {
        int half = pointsEachSide;       // points either side of the place read
        std::vector<float> points;       // (phases + 1) rows of 2 * half
    };

    double bessel0 (double x)
    {
        double sum = 1.0, term = 1.0;

        for (int k = 1; k < 64; ++k)
        {
            term *= (x * 0.5 / k) * (x * 0.5 / k);
            sum += term;

            if (term < sum * 1.0e-17)
                break;
        }

        return sum;
    }

    const std::array<Bank, (size_t) numBanks>& banks()
    {
        static const auto made = []
        {
            std::array<Bank, (size_t) numBanks> all;
            constexpr double beta = 8.4;   // a Kaiser window 85 dB down
            const auto norm = bessel0 (beta);

            for (int b = 0; b < numBanks; ++b)
            {
                auto& bank = all[(size_t) b];
                const auto band = std::pow (2.0, -(double) b / banksPerOctave);   // of the sample's own
                bank.half = (int) std::ceil (pointsEachSide / band);
                const auto count = 2 * bank.half;
                const auto cutoff = 0.93 * band;
                bank.points.resize ((size_t) ((phases + 1) * count));

                for (int p = 0; p <= phases; ++p)
                {
                    auto* row = bank.points.data() + (size_t) (p * count);
                    double sum = 0.0;
                    std::vector<double> exact ((size_t) count);

                    for (int j = 0; j < count; ++j)
                    {
                        const auto t = (double) (j - bank.half + 1) - (double) p / phases;
                        const auto across = t / bank.half;
                        const auto window = std::abs (across) >= 1.0 ? 0.0 : bessel0 (beta * std::sqrt (1.0 - across * across)) / norm;
                        const auto x = juce::MathConstants<double>::pi * cutoff * t;
                        exact[(size_t) j] = (std::abs (x) < 1.0e-9 ? 1.0 : std::sin (x) / x) * cutoff * window;
                        sum += exact[(size_t) j];
                    }

                    // Each row adds up to 1, so a steady level stays exactly as it is.
                    for (int j = 0; j < count; ++j)
                        row[j] = (float) (exact[(size_t) j] / sum);
                }
            }

            return all;
        }();

        return made;
    }

    int bankFor (double ratio)
    {
        return ratio <= 1.0 ? 0 : juce::jlimit (0, numBanks - 1, (int) std::ceil (banksPerOctave * std::log2 (ratio) - 1.0e-9));
    }

    /** The windowed sum of `count` frames starting at x, for a place `fraction` of the way from
        the frame in the middle to the next. */
    inline float readThrough (const Bank& bank, const float* x, double fraction) noexcept
    {
        const auto count = 2 * bank.half;
        const auto place = fraction * phases;
        const auto row = (int) place;
        const auto blend = (float) (place - row);
        const auto* k0 = bank.points.data() + (size_t) (row * count);
        const auto* k1 = k0 + count;
        auto sum = 0.0f;

        for (int j = 0; j < count; ++j)
            sum += x[j] * (k0[j] + blend * (k1[j] - k0[j]));

        return sum;
    }

    float gainOf (float db)                  { return juce::Decibels::decibelsToGain (db, -200.0f); }

    /** Left and right gains for a place from -1 (left) to 1 (right): equal power, 1 each in the middle. */
    std::pair<float, float> panned (float place)
    {
        const auto angle = (juce::jlimit (-1.0f, 1.0f, place) + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        return { std::cos (angle) * juce::MathConstants<float>::sqrt2, std::sin (angle) * juce::MathConstants<float>::sqrt2 };
    }

    /** What a level falling to 60 dB down over `ms` is multiplied by each sample. */
    float fallOver (float ms, double rate)
    {
        return (float) std::exp (-6.907755 / juce::jmax (1.0, (double) ms * 0.001 * rate));
    }

    juce::uint32 hashed (juce::uint32 x)
    {
        x ^= x >> 16;
        x *= 0x7feb352du;
        x ^= x >> 15;
        x *= 0x846ca68bu;
        x ^= x >> 16;
        return x;
    }

    /** An envelope: a straight rise, then falling toward the sustain level, then toward nothing. */
    struct Envelope
    {
        enum class Stage { rising, falling, releasing, over };

        Stage stage = Stage::over;
        float level = 0.0f;

        void start() noexcept              { stage = Stage::rising; }

        void release() noexcept
        {
            if (stage != Stage::over)
                stage = Stage::releasing;
        }

        void tick (float rise, float fall, float sustain, float let) noexcept
        {
            switch (stage)
            {
                case Stage::rising:
                    level += rise;

                    if (level >= 1.0f)
                    {
                        level = 1.0f;
                        stage = Stage::falling;
                    }
                    break;

                case Stage::falling:
                    level = sustain + (level - sustain) * fall;
                    break;

                case Stage::releasing:
                    level *= let;

                    if (level < 1.0e-4f)
                    {
                        level = 0.0f;
                        stage = Stage::over;
                    }
                    break;

                case Stage::over:
                    break;
            }
        }
    };

    /** One stage of the filter: a state-variable filter, for a channel. */
    struct Stage
    {
        float ic1 = 0.0f, ic2 = 0.0f;

        /** Its low-pass, band-pass and high-pass outputs for x. */
        inline void run (float x, float g, float k, float& low, float& band, float& high) noexcept
        {
            const auto a1 = 1.0f / (1.0f + g * (g + k));
            const auto a2 = g * a1;
            const auto v3 = x - ic2;
            const auto v1 = a1 * ic1 + a2 * v3;
            const auto v2 = ic2 + a2 * ic1 + g * a2 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;
            low = v2;
            band = v1;
            high = x - k * v1 - v2;
        }
    };
}

//==============================================================================
juce::Range<juce::int64> Zone::span() const
{
    if (sample == nullptr)
        return {};

    const auto frames = (juce::int64) sample->getNumFrames();
    const auto first = juce::jlimit ((juce::int64) 0, frames, start);
    const auto last = end < 0 ? frames : juce::jlimit (first, frames, end);
    return { first, last };
}

juce::StringArray filterNames()             { return { "Off", "Low-pass 12", "Low-pass 24", "Band-pass", "High-pass" }; }
juce::StringArray lfoShapeNames()           { return { "Sine", "Triangle", "Saw", "Square", "Random" }; }
juce::StringArray lfoSyncNames()            { return { "Off", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }; }

double lfoSyncBeats (int index)
{
    static constexpr double beats[] = { 0.0, 8.0, 4.0, 2.0, 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };
    return beats[juce::jlimit (0, (int) std::size (beats) - 1, index)];
}

SamplePtr makeSample (const juce::AudioBuffer<float>& audio, double rate, const juce::String& name, const Markers& markers)
{
    if (audio.getNumChannels() < 1 || audio.getNumSamples() < 1 || rate <= 0.0)
        return {};

    auto made = std::make_shared<Sample>();
    const auto channels = juce::jmin (2, audio.getNumChannels());
    made->audio.setSize (channels, audio.getNumSamples());

    for (int c = 0; c < channels; ++c)
        made->audio.copyFrom (c, 0, audio, c, 0, audio.getNumSamples());

    made->rate = rate;
    made->name = name;
    made->markers = markers;
    return made;
}

SamplePtr loadSample (const juce::File& file, juce::AudioFormatManager& formats, juce::String& error)
{
    if (! file.existsAsFile())
    {
        error = file.getFileName() + " was not found.";
        return {};
    }

    const auto loaded = AudioFileIO::load (file, formats);

    if (loaded.snapshot == nullptr || loaded.error.isNotEmpty())
    {
        error = loaded.error.isNotEmpty() ? loaded.error : file.getFileName() + " could not be read.";
        return {};
    }

    const auto frames = loaded.snapshot->getNumFrames();

    if (frames < 1 || frames > std::numeric_limits<int>::max() || loaded.snapshot->getNumChannels() < 1)
    {
        error = file.getFileName() + " holds no audio.";
        return {};
    }

    auto made = std::make_shared<Sample>();
    made->audio.setSize (juce::jmin (2, loaded.snapshot->getNumChannels()), (int) frames);
    loaded.snapshot->read (made->audio, 0, 0, (int) frames);
    made->rate = loaded.sampleRate;
    made->name = file.getFileNameWithoutExtension();
    made->file = file;
    made->markers = loaded.markers;

    // A WAV file's sampler chunk: the key it was recorded at and its first loop.
    if (file.hasFileExtension ("wav;wave;bwf"))
        if (std::unique_ptr<juce::AudioFormatReader> reader { formats.createReaderFor (file) })
        {
            const auto& values = reader->metadataValues;

            if (values.containsKey ("MidiUnityNote"))
            {
                made->fileRoot = juce::jlimit (0, 127, values["MidiUnityNote"].getIntValue());
                made->fileTune = -100.0f * (float) ((double) (juce::uint32) values["MidiPitchFraction"].getLargeIntValue() / 4294967296.0);

                if (values["NumSampleLoops"].getIntValue() > 0)
                {
                    const auto type = values["Loop0Type"].getIntValue();
                    const auto start = values["Loop0Start"].getLargeIntValue(), end = values["Loop0End"].getLargeIntValue() + 1;   // its end is the last frame played

                    if (start >= 0 && end > start && end <= frames)
                    {
                        made->fileLoop = type == 1 ? 2 : 1;
                        made->fileLoopStart = start;
                        made->fileLoopEnd = end;
                    }
                }
            }
        }
    return made;
}

ProgramPtr makeProgram (std::vector<Zone> zones, juce::uint32 generation)
{
    if ((int) zones.size() > maxZones)
        zones.resize ((size_t) maxZones);

    auto made = std::make_shared<Program>();
    made->generation = generation;
    made->prepared.resize (zones.size());

    for (size_t i = 0; i < zones.size(); ++i)
    {
        const auto& z = zones[i];
        auto& p = made->prepared[i];
        const auto span = z.span();
        p.first = span.getStart();
        p.last = span.getEnd();
        p.length = span.getLength();
        p.reverse = z.reverse;
        p.loopsInRelease = z.loopInRelease;

        if (z.sample == nullptr || z.oneShot || z.loop == 0)
            continue;

        // The loop, held to the part, in frames along the way the part plays.
        const auto from = juce::jlimit (p.first, p.last, z.loopStart);
        const auto to = z.loopEnd < 0 ? p.last : juce::jlimit (from, p.last, z.loopEnd);
        p.loopStart = z.reverse ? p.last - to : from - p.first;
        p.loopEnd = z.reverse ? p.last - from : to - p.first;
        const auto length = p.loopEnd - p.loopStart;

        if (length < 4)
            continue;

        p.loop = z.loop;
        p.period = z.loop == 2 ? 2 * (length - 1) : length;

        if (z.loop != 1)
            continue;

        // A forward loop's turn: its last frames crossfaded into the frames before its start, as
        // Crossfade Loop does, so going from its end back to its start carries on as the sound did.
        p.fade = juce::jlimit ((juce::int64) 0, juce::jmin (p.loopStart, length / 2), (juce::int64) std::llround (z.crossfade * 0.001 * z.sample->rate));

        if (p.fade <= 0)
            continue;

        const auto& audio = z.sample->audio;
        p.turn.setSize (audio.getNumChannels(), (int) p.fade);

        for (int ch = 0; ch < audio.getNumChannels(); ++ch)
        {
            const auto* x = audio.getReadPointer (ch);
            auto at = [&] (juce::int64 along)    { return x[z.reverse ? p.last - 1 - along : p.first + along]; };

            for (juce::int64 n = 0; n < p.fade; ++n)
            {
                const auto in = FadeShapes::level (FadeShape::equalPower, n + 1, p.fade);
                const auto out = FadeShapes::level (FadeShape::equalPower, p.fade - 1 - n, p.fade);
                p.turn.setSample (ch, (int) n, (float) (at (p.loopEnd - p.fade + n) * out + at (p.loopStart - p.fade + n) * in));
            }
        }
    }

    made->zones = std::move (zones);
    return made;
}

//==============================================================================
namespace
{
    struct Store
    {
        struct Held
        {
            std::weak_ptr<const Sample> sample;
            juce::Time changed;
            juce::int64 size = 0;
        };

        juce::CriticalSection lock;
        std::map<juce::String, Held> held;
        juce::AudioFormatManager formats;
        juce::File sessionFolder, savingInto;

        Store()     { AudioFileIO::registerFormats (formats); }
    };

    Store& store()
    {
        static Store s;
        return s;
    }
}

SamplePtr sharedSample (const juce::File& file, juce::String& error)
{
    auto& s = store();
    const juce::ScopedLock sl (s.lock);
    const auto key = file.getFullPathName();

    if (const auto found = s.held.find (key); found != s.held.end())
    {
        if (auto sample = found->second.sample.lock(); sample != nullptr && found->second.changed == file.getLastModificationTime() && found->second.size == file.getSize())
            return sample;

        s.held.erase (found);
    }

    // Its length, before any of it is read.
    if (file.existsAsFile())
        if (std::unique_ptr<juce::AudioFormatReader> reader { s.formats.createReaderFor (file) })
            if (reader->sampleRate > 0.0 && (double) reader->lengthInSamples / reader->sampleRate > longestSampleSeconds)
            {
                error = file.getFileName() + " is " + juce::String (juce::roundToInt ((double) reader->lengthInSamples / reader->sampleRate / 60.0))
                        + " minutes long. A sample can be up to " + juce::String (juce::roundToInt (longestSampleSeconds / 60.0)) + " minutes: samples are held in memory, not played from disk.";
                return {};
            }

    auto sample = loadSample (file, s.formats, error);

    if (sample != nullptr && (double) sample->getNumFrames() / sample->rate > longestSampleSeconds)
    {
        error = file.getFileName() + " is longer than " + juce::String (juce::roundToInt (longestSampleSeconds / 60.0)) + " minutes, the most a sample can be.";
        return {};
    }

    if (sample != nullptr)
        s.held[key] = { sample, file.getLastModificationTime(), file.getSize() };

    // Samples nobody holds any more are forgotten.
    for (auto it = s.held.begin(); it != s.held.end();)
        it = it->second.sample.expired() ? s.held.erase (it) : std::next (it);

    return sample;
}

juce::File samplesFolder()
{
    return juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Torqwave").getChildFile ("Samples");
}

juce::File writeSample (const juce::AudioBuffer<float>& audio, double rate, const juce::String& name, const Markers& markers, juce::String& error, juce::File folder)
{
    if (folder == juce::File())
        folder = samplesFolder();

    if (audio.getNumChannels() < 1 || audio.getNumSamples() < 1 || rate <= 0.0)
    {
        error = "There is no audio to sample.";
        return {};
    }

    if (! folder.createDirectory())
    {
        error = "Could not make the folder " + folder.getFullPathName();
        return {};
    }

    const auto base = juce::File::createLegalFileName (name.trim()).isNotEmpty() ? juce::File::createLegalFileName (name.trim()) : juce::String ("Sample");
    auto file = folder.getChildFile (base + ".wav");

    for (int n = 2; file.exists(); ++n)
        file = folder.getChildFile (base + " " + juce::String (n) + ".wav");

    std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream> (file);
    const auto options = juce::AudioFormatWriterOptions().withSampleRate (rate).withNumChannels (audio.getNumChannels()).withBitsPerSample (32)
                             .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint)
                             .withMetadataValues (MarkerEdits::toMetadata (markers));
    auto writer = juce::WavAudioFormat().createWriterFor (stream, options);

    if (writer == nullptr || ! writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples()))
    {
        writer.reset();
        file.deleteFile();
        error = "Could not write " + file.getFullPathName();
        return {};
    }

    return file;
}

void setSessionFolder (const juce::File& folder)
{
    const juce::ScopedLock sl (store().lock);
    store().sessionFolder = folder;
}

juce::File getSessionFolder()
{
    const juce::ScopedLock sl (store().lock);
    return store().sessionFolder;
}

SavingInto::SavingInto (const juce::File& sessionFolder)
{
    const juce::ScopedLock sl (store().lock);
    before = store().savingInto;
    store().savingInto = sessionFolder;
}

SavingInto::~SavingInto()
{
    const juce::ScopedLock sl (store().lock);
    store().savingInto = before;
}

juce::String collectFiles (juce::XmlElement& effects, const juce::File& sessionFolder, const juce::File& folder,
                           std::vector<std::pair<juce::File, juce::File>>& copies, juce::StringArray& missing)
{
    for (auto* slot : effects.getChildWithTagNameIterator ("SLOT"))
    {
        const auto* plugin = slot->getChildByName ("PLUGIN");
        auto* stateXml = slot->getChildByName ("STATE");

        if (plugin == nullptr || stateXml == nullptr || plugin->getStringAttribute ("file") != BuiltInEffects::samplerId)
            continue;

        juce::MemoryBlock block;

        if (! block.fromBase64Encoding (stateXml->getAllSubText()))
            continue;

        auto state = juce::AudioProcessor::getXmlFromBinary (block.getData(), (int) block.getSize());
        auto* zones = state != nullptr ? state->getChildByName ("ZONES") : nullptr;

        if (zones == nullptr)
            continue;

        for (auto* zone : zones->getChildWithTagNameIterator ("ZONE"))
        {
            const auto path = zone->getStringAttribute ("file");

            if (path.isEmpty())
                continue;

            const juce::File from (path);

            if (! from.existsAsFile())
            {
                missing.addIfNotAlreadyThere (path);
                continue;
            }

            juce::File to;

            for (auto& [was, now] : copies)
                if (was == from)
                    to = now;

            if (to == juce::File())
            {
                if (from.isAChildOf (folder))
                {
                    to = from;
                }
                else
                {
                    auto taken = [&] (const juce::File& f)
                    {
                        return f.exists() || std::any_of (copies.begin(), copies.end(), [&f] (const auto& c) { return c.second == f; });
                    };

                    to = folder.getChildFile (from.getFileName());

                    for (int n = 2; taken (to); ++n)
                        to = folder.getChildFile (from.getFileNameWithoutExtension() + " " + juce::String (n) + from.getFileExtension());

                    if (! folder.createDirectory())
                        return "Could not make the folder " + folder.getFullPathName();

                    if (! from.copyFileTo (to))
                        return "Could not copy " + from.getFullPathName() + " to " + to.getFullPathName();
                }

                copies.emplace_back (from, to);
            }

            zone->setAttribute ("file", to.getFullPathName());
            zone->setAttribute ("relative", to.getRelativePathFrom (sessionFolder));
        }

        juce::MemoryBlock rewritten;
        juce::AudioProcessor::copyXmlToBinary (*state, rewritten);
        stateXml->deleteAllTextElements();
        stateXml->addTextElement (rewritten.toBase64Encoding());
    }

    return {};
}

//==============================================================================
Pitch findPitch (const Sample& sample, juce::Range<juce::int64> part)
{
    const auto frames = (juce::int64) sample.getNumFrames();
    part = part.isEmpty() ? juce::Range<juce::int64> (0, frames) : part.getIntersectionWith ({ 0, frames });
    const auto skip = (juce::int64) (0.02 * sample.rate), most = (juce::int64) (0.5 * sample.rate);
    const auto from = part.getLength() > skip + most / 4 ? part.getStart() + skip : part.getStart();
    const auto count = (int) juce::jmin (most, part.getEnd() - from);
    Pitch pitch;

    if (count < 256)
        return pitch;

    std::vector<float> mono ((size_t) count, 0.0f);

    for (int c = 0; c < sample.audio.getNumChannels(); ++c)
        juce::FloatVectorOperations::add (mono.data(), sample.audio.getReadPointer (c, (int) from), count);

    pitch.hz = WaveTables::findPitch (mono.data(), count, sample.rate);

    if (pitch.hz <= 0.0)
        return pitch;

    const auto note = 69.0 + 12.0 * std::log2 (pitch.hz / 440.0);
    pitch.found = true;
    pitch.root = juce::jlimit (0, 127, juce::roundToInt (note));
    pitch.tune = (float) juce::jlimit (-100.0, 100.0, 100.0 * ((double) pitch.root - note));
    return pitch;
}

LoopFound findLoop (const Sample& sample, juce::Range<juce::int64> part, double hz)
{
    const auto frames = (juce::int64) sample.getNumFrames();
    part = part.isEmpty() ? juce::Range<juce::int64> (0, frames) : part.getIntersectionWith ({ 0, frames });
    LoopFound loop;

    if ((double) part.getLength() < 0.2 * sample.rate)
        return loop;

    const auto first = (int) part.getStart(), length = (int) part.getLength();
    std::vector<float> x ((size_t) length, 0.0f);

    for (int c = 0; c < sample.audio.getNumChannels(); ++c)
        juce::FloatVectorOperations::add (x.data(), sample.audio.getReadPointer (c, first), length);

    const auto period = hz > 0.0 ? sample.rate / hz : 0.0;

    if (period < 2.0 || period * 8.0 > length)
    {
        // No pitch to line up: the later part of the sound, its end fading into what comes before it.
        loop.found = true;
        loop.start = first + (juce::int64) (0.4 * length);
        loop.end = first + (juce::int64) (0.95 * length);
        loop.crossfade = (float) juce::jmin (250.0, 1000.0 * 0.25 * (double) (loop.end - loop.start) / sample.rate);
        return loop;
    }

    // The held part: from a little after the loudest moment (and no earlier than a quarter of the way in)
    // to a little before the end.
    const auto window = juce::jmax (64, (int) (0.02 * sample.rate));
    auto loudestAt = 0;
    auto loudest = 0.0;

    for (int at = 0; at + window <= length; at += window)
    {
        auto energy = 0.0;

        for (int i = 0; i < window; ++i)
            energy += (double) x[(size_t) (at + i)] * x[(size_t) (at + i)];

        if (energy > loudest)
        {
            loudest = energy;
            loudestAt = at;
        }
    }

    const auto reach = (int) std::ceil (2.0 * period);       // how far either side of the two places is compared
    const auto from = juce::jmax (length / 4, loudestAt + (int) (0.05 * sample.rate), reach + 1);
    const auto to = length - juce::jmax (reach + 1, length / 20);

    if (to - from < (int) (4.0 * period))
        return loop;

    // How unlike the sound around two places is: the difference's energy against the sound's.
    auto unlike = [&] (int a, int b)
    {
        auto difference = 0.0, energy = 1.0e-12;

        for (int i = -reach; i < reach; ++i)
        {
            const auto p = x[(size_t) (a + i)], q = x[(size_t) (b + i)];
            difference += (double) (p - q) * (p - q);
            energy += (double) p * p + (double) q * q;
        }

        return difference / energy;
    };

    const auto mostCycles = (int) ((to - from) / period);
    auto best = 1.0e30;
    int bestStart = from, bestEnd = from + (int) std::lround (mostCycles * period);

    for (int step = 0; step < 16; ++step)
    {
        const auto start = from + (int) ((to - from) * 0.5 * step / 16.0);
        const auto cycles = (int) ((to - start) / period);

        for (int n = cycles; n >= juce::jmax (2, cycles / 2); n -= juce::jmax (1, cycles / 24))
        {
            const auto centre = start + (int) std::lround (n * period);
            const auto slack = juce::jmax (1, (int) (period / 8.0));

            for (int end = centre - slack; end <= centre + slack; ++end)
            {
                if (end + reach >= length)
                    continue;

                // A longer loop is worth a little unlikeness: the sound moves less often.
                const auto score = (unlike (start, end) + 1.0e-6) * (1.0 + 0.5 * (1.0 - (double) (end - start) / (double) (to - from)));

                if (score < best)
                {
                    best = score;
                    bestStart = start;
                    bestEnd = end;
                }
            }
        }
    }

    // Both to a rising zero crossing, the start's nearest and the end's nearest to as far on.
    auto rising = [&] (int near, int within)
    {
        for (int d = 0; d <= within; ++d)
            for (const auto at : { near - d, near + d })
                if (at > 0 && at < length && x[(size_t) (at - 1)] <= 0.0f && x[(size_t) at] > 0.0f)
                    return at;

        return near;
    };

    const auto start = rising (bestStart, (int) period);
    const auto end = rising (bestEnd + (start - bestStart), (int) (period / 4.0));
    loop.found = end - start >= 4;
    loop.start = first + start;
    loop.end = first + end;
    loop.crossfade = (float) juce::jmin (20.0, 1000.0 * (double) start / sample.rate, 1000.0 * 0.25 * (double) (end - start) / sample.rate);
    return loop;
}

//==============================================================================
juce::StringArray sliceSourceNames()        { return { "Markers", "Regions", "Hits", "Equal Parts" }; }

std::vector<Slice> findSlices (const Sample& sample, juce::Range<juce::int64> part, const Slicing& how)
{
    const auto frames = (juce::int64) sample.getNumFrames();
    part = part.isEmpty() ? juce::Range<juce::int64> (0, frames) : part.getIntersectionWith ({ 0, frames });
    std::vector<Slice> slices;

    if (part.isEmpty())
        return slices;

    // Pieces from cuts: each from one cut to the next, the last to the part's end.
    auto between = [&] (const std::vector<std::pair<juce::int64, juce::String>>& cuts)
    {
        for (size_t i = 0; i < cuts.size() && (int) slices.size() < maxSlices; ++i)
        {
            const auto end = i + 1 < cuts.size() ? cuts[i + 1].first : part.getEnd();

            if (end > cuts[i].first)
                slices.push_back ({ cuts[i].first, end, cuts[i].second });
        }
    };

    switch (how.source)
    {
        case SliceSource::markers:
        {
            std::vector<std::pair<juce::int64, juce::String>> cuts;

            for (auto& m : sample.markers)
                if (! m.isRegion() && m.start >= part.getStart() && m.start < part.getEnd())
                    cuts.push_back ({ m.start, m.name });

            if (cuts.empty())
                break;

            if (cuts.front().first > part.getStart())
                cuts.insert (cuts.begin(), { part.getStart(), "Start" });

            for (size_t i = 0; i < cuts.size(); ++i)
                if (cuts[i].second.isEmpty())
                    cuts[i].second = "Slice " + juce::String (i + 1);

            between (cuts);
            break;
        }

        case SliceSource::regions:
        {
            for (auto& m : sample.markers)
                if (m.isRegion() && m.start >= part.getStart() && m.getEnd() <= part.getEnd() && (int) slices.size() < maxSlices)
                    slices.push_back ({ m.start, m.getEnd(), m.name.isNotEmpty() ? m.name : "Slice " + juce::String (slices.size() + 1) });

            break;
        }

        case SliceSource::hits:
        {
            const auto& audio = sample.audio;
            const auto channels = audio.getNumChannels();
            const auto from = (int) part.getStart(), to = (int) part.getEnd();

            auto sum = [&] (int i)
            {
                auto x = 0.0f;

                for (int c = 0; c < channels; ++c)
                    x += audio.getSample (c, i);

                return x;
            };

            // Each attack with how loud the sound gets in the 30 ms after it.
            struct Hit { int frame; float loud; };
            std::vector<Hit> hits;
            const auto after = (int) (0.03 * sample.rate), apart = (int) (0.02 * sample.rate);
            auto loudest = 0.0f;

            for (const auto at : PhaseVocoder::onsets (audio, from, to, sample.rate))
            {
                if (at < from + apart || at >= to || (! hits.empty() && at < hits.back().frame + apart))
                    continue;

                auto loud = 0.0f;

                for (int i = at; i < juce::jmin (to, at + after); ++i)
                    loud = juce::jmax (loud, std::abs (sum (i)));

                hits.push_back ({ at, loud });
                loudest = juce::jmax (loudest, loud);
            }

            // The sensitivity: at 100 % every attack counts, at 50 % those within 12 dB of the loudest, at 0 % that one alone.
            const auto less = 1.0f - juce::jlimit (0.0f, 100.0f, how.sensitivity) * 0.01f;
            const auto floor = loudest * less * less;
            hits.erase (std::remove_if (hits.begin(), hits.end(), [floor] (const Hit& h) { return h.loud < floor; }), hits.end());

            if ((int) hits.size() > maxSlices - 1)
            {
                auto byLoudness = hits;
                std::sort (byLoudness.begin(), byLoudness.end(), [] (const Hit& a, const Hit& b) { return a.loud > b.loud; });
                const auto least = byLoudness[(size_t) (maxSlices - 2)].loud;
                hits.erase (std::remove_if (hits.begin(), hits.end(), [least] (const Hit& h) { return h.loud < least; }), hits.end());

                if ((int) hits.size() > maxSlices - 1)
                    hits.resize ((size_t) (maxSlices - 1));
            }

            // Each cut at the nearest zero crossing of the channels' sum within 2 ms, so a slice starts from nothing.
            const auto reach = (int) (0.002 * sample.rate);
            std::vector<std::pair<juce::int64, juce::String>> cuts { { part.getStart(), {} } };

            for (auto& h : hits)
            {
                auto cut = h.frame;

                for (int d = 0; d <= reach; ++d)
                {
                    auto crosses = [&] (int i)      { return i > from && i < to && (sum (i - 1) <= 0.0f) != (sum (i) <= 0.0f); };

                    if (crosses (h.frame - d))
                    {
                        cut = h.frame - d;
                        break;
                    }

                    if (crosses (h.frame + d))
                    {
                        cut = h.frame + d;
                        break;
                    }
                }

                if (cut > cuts.back().first)
                    cuts.push_back ({ cut, {} });
            }

            for (size_t i = 0; i < cuts.size(); ++i)
                cuts[i].second = "Slice " + juce::String (i + 1);

            between (cuts);
            break;
        }

        case SliceSource::equalParts:
        {
            const auto parts = juce::jlimit (1, maxSlices, how.parts);

            for (int i = 0; i < parts; ++i)
            {
                const auto start = part.getStart() + part.getLength() * i / parts, end = part.getStart() + part.getLength() * (i + 1) / parts;

                if (end > start)
                    slices.push_back ({ start, end, "Slice " + juce::String (i + 1) });
            }

            break;
        }
    }

    return slices;
}

std::vector<Zone> zonesFromSlices (const SamplePtr& sample, const juce::File& file, const std::vector<Slice>& slices, bool cutEachOther)
{
    std::vector<Zone> zones;

    for (size_t i = 0; i < slices.size() && (int) i < maxSlices; ++i)
    {
        Zone z;
        z.sample = sample;
        z.file = file;
        z.name = slices[i].name;
        z.root = z.lowKey = z.highKey = firstSliceKey + (int) i;
        z.start = slices[i].start;
        z.end = slices[i].end;
        z.oneShot = true;
        z.choke = cutEachOther ? 1 : 0;
        zones.push_back (std::move (z));
    }

    return zones;
}

std::vector<Instrument::Note> notesFromSlices (const std::vector<Zone>& zones)
{
    std::vector<Instrument::Note> notes;
    auto first = std::numeric_limits<juce::int64>::max();

    for (auto& z : zones)
        if (z.sample != nullptr)
            first = juce::jmin (first, z.span().getStart());

    for (auto& z : zones)
        if (z.sample != nullptr && ! z.span().isEmpty())
            notes.push_back ({ z.lowKey, (double) (z.span().getStart() - first) / z.sample->rate, (double) z.span().getLength() / z.sample->rate, 1.0f });

    std::sort (notes.begin(), notes.end(), [] (const Instrument::Note& a, const Instrument::Note& b) { return a.start < b.start; });
    return notes;
}

std::vector<DrumSound> drumMapFor (const std::vector<Zone>& zones)
{
    std::vector<DrumSound> map;

    for (auto& z : zones)
        if (z.lowKey == z.highKey)
        {
            DrumSound sound;
            sound.pitch = z.lowKey;
            sound.name = z.name.isNotEmpty() ? z.name : Instrument::pitchName (z.lowKey);
            map.push_back (std::move (sound));
        }

    std::sort (map.begin(), map.end(), [] (const DrumSound& a, const DrumSound& b) { return a.pitch < b.pitch; });
    return map;
}

float interpolate (const float* samples, int numSamples, double position, double ratio)
{
    const auto& bank = banks()[(size_t) bankFor (ratio)];
    const auto count = 2 * bank.half;
    const auto index = (juce::int64) std::floor (position);
    std::array<float, (size_t) mostPoints> window;

    for (int j = 0; j < count; ++j)
    {
        const auto at = index - bank.half + 1 + j;
        window[(size_t) j] = at >= 0 && at < numSamples ? samples[at] : 0.0f;
    }

    return readThrough (bank, window.data(), position - (double) index);
}

//==============================================================================
struct Engine::Voice
{
    bool active = false, down = false, held = false, dying = false;
    int note = -1, index = 0;
    float velocity = 0.0f;
    juce::uint32 age = 0, seed = 0;

    ProgramPtr program;
    const Zone* zone = nullptr;
    const Program::Prepared* view = nullptr;
    bool wrapped = false;                // its loop has gone round at least once

    // Zones changed under it in a way that moves what it reads (a loop's points or crossfade, the
    // part's ends): it goes over to them at a place both read alike, so nothing steps.
    ProgramPtr nextProgram;
    int nextZone = -1;
    bool leaving = false;                // let go, it plays on past its loop to the end

    double position = 0.0, ratio = 1.0;  // frames along the way it plays
    int bank = 0;
    juce::int64 played = 0;
    float glide = 0.0f;                  // semitones still to slide
    float endFade = 1.0f;                // how much of the level a frame before the end gives

    Envelope amp, filterEnvelope;
    float fadeGain = 1.0f;

    std::array<std::array<Stage, 2>, 2> stages;   // two stages, a channel each
    float g = 0.1f, gStep = 0.0f, k = 1.414f;
    bool filterSet = false;

    double lfoPhase = 0.0;
    float left = 0.0f, right = 0.0f, leftStep = 0.0f, rightStep = 0.0f;
    bool gainsSet = false;
};

Engine::Engine()
{
    voices.resize ((size_t) (numVoices + numFadingVoices));

    for (size_t i = 0; i < voices.size(); ++i)
        voices[i].index = (int) i;
}

Engine::~Engine() = default;

void Engine::prepare (double sampleRate)
{
    rate = sampleRate > 0.0 ? sampleRate : 48000.0;
    banks();
    scratchL.assign (4096, 0.0f);
    scratchR.assign (4096, 0.0f);
    reset();
}

void Engine::reset()
{
    for (auto& v : voices)
    {
        v.active = false;
        v.program = nullptr;
        v.nextProgram = nullptr;
        v.zone = nullptr;
    }

    for (auto& r : readings)
        r.position.store (-1.0f, std::memory_order_relaxed);

    bend = wheel = panning = 0.0f;
    volume = expression = 1.0f;
    pedal = false;
    lastNote = -1.0f;
    gainsSet = false;
    counter = 0;
}

void Engine::setSettings (const Settings& s)      { settings = s; }
void Engine::setTempo (double bpm)                { tempo = bpm > 0.0 ? bpm : 120.0; }

void Engine::setProgram (ProgramPtr next)
{
    for (auto& v : voices)
    {
        if (! v.active || v.zone == nullptr)
            continue;

        int same = -1;

        if (next != nullptr && next->prepared.size() == next->zones.size())
            for (size_t i = 0; i < next->zones.size(); ++i)
                if (next->zones[i].id == v.zone->id && next->zones[i].sample == v.zone->sample)
                    same = (int) i;

        const auto* to = same >= 0 ? &next->prepared[(size_t) same] : nullptr;
        const auto& from = *v.view;

        if (to != nullptr && to->reverse == from.reverse && to->first == from.first && to->last == from.last && to->loop == from.loop
            && to->loopStart == from.loopStart && to->loopEnd == from.loopEnd && to->fade == from.fade)
        {
            // What it reads is as it was: the zone's other settings are taken up at once.
            v.program = next;
            v.zone = &next->zones[(size_t) same];
            v.view = to;
            v.nextProgram = nullptr;
            v.nextZone = -1;
        }
        else if (to != nullptr && to->reverse == from.reverse)
        {
            v.nextProgram = next;
            v.nextZone = same;
        }
        else
        {
            fade (v);
        }
    }

    program = std::move (next);
}

juce::uint32 Engine::oldestGenerationInUse() const noexcept
{
    auto oldest = program != nullptr ? program->generation : std::numeric_limits<juce::uint32>::max();

    for (auto& v : voices)
        if (v.active && v.program != nullptr)
            oldest = juce::jmin (oldest, v.program->generation);

    return oldest;
}

int Engine::getNumSounding() const
{
    int count = 0;

    for (auto& v : voices)
        count += v.active && ! v.dying ? 1 : 0;

    return count;
}

Engine::Voice* Engine::take()
{
    // More than it plays at once: the oldest gives way, fading.
    int sounding = 0;
    Voice* oldest = nullptr;

    for (auto& v : voices)
        if (v.active && ! v.dying)
        {
            ++sounding;

            if (oldest == nullptr || v.age < oldest->age)
                oldest = &v;
        }

    if (sounding >= numVoices && oldest != nullptr)
        fade (*oldest);

    for (auto& v : voices)
        if (! v.active)
            return &v;

    // Every voice busy, the spare ones fading: the one nearest silence is taken.
    Voice* quietest = nullptr;

    for (auto& v : voices)
        if (v.dying && (quietest == nullptr || v.fadeGain < quietest->fadeGain))
            quietest = &v;

    return quietest;
}

void Engine::start (Voice& v, const Zone& zone, int note, float velocity)
{
    const auto index = v.index;
    v = Voice();
    v.index = index;
    v.active = v.down = true;
    v.note = note;
    v.velocity = velocity;
    v.age = ++counter;
    v.seed = hashed (counter * 2654435761u + (juce::uint32) note);
    v.program = program;
    v.zone = &zone;
    v.view = &program->prepared[(size_t) (&zone - program->zones.data())];
    v.glide = settings.glide > 0.0f && lastNote >= 0.0f ? lastNote - (float) note : 0.0f;
    v.amp.start();
    v.filterEnvelope.start();
}

void Engine::letGo (Voice& v)
{
    v.down = v.held = false;
    v.amp.release();
    v.filterEnvelope.release();
}

void Engine::fade (Voice& v)
{
    v.dying = true;
    v.down = v.held = false;
}

void Engine::noteOn (int note, float velocity)
{
    if (program == nullptr || program->prepared.size() != program->zones.size())
        return;

    const auto key = juce::jlimit (0, 127, note);
    const auto struck = juce::jlimit (1, 127, juce::roundToInt (velocity * 127.0f));

    // A key struck again while it is still down, and in Mono every other note, gives way.
    for (auto& v : voices)
        if (v.active && ! v.dying && (settings.mono || (v.note == key && (v.down || v.held))))
            fade (v);

    for (auto& zone : program->zones)
        if (zone.plays (key, struck) && ! zone.span().isEmpty())
        {
            // A zone of a choke group cuts off the others of it, as a closed hat does an open one.
            if (zone.choke != 0)
                for (auto& v : voices)
                    if (v.active && ! v.dying && v.zone->choke == zone.choke)
                        fade (v);

            if (auto* v = take())
                start (*v, zone, key, juce::jlimit (0.0f, 1.0f, velocity));
        }

    lastNote = (float) key;
}

void Engine::noteOff (int note)
{
    for (auto& v : voices)
        if (v.active && v.down && v.note == note && ! v.zone->oneShot)
        {
            if (pedal)
            {
                v.down = false;
                v.held = true;
            }
            else
            {
                letGo (v);
            }
        }
}

void Engine::allNotesOff()
{
    for (auto& v : voices)
        if (v.active && (v.down || v.held) && ! v.zone->oneShot)
            letGo (v);
}

void Engine::allSoundOff()
{
    for (auto& v : voices)
        if (v.active)
            fade (v);
}

void Engine::pitchBend (float place)             { bend = juce::jlimit (-1.0f, 1.0f, place); }

void Engine::controller (int number, int value)
{
    const auto amount = (float) juce::jlimit (0, 127, value) / 127.0f;

    switch (number)
    {
        case 1:     wheel = amount; break;
        case 7:     volume = amount * amount; break;
        case 10:    panning = juce::jlimit (-1.0f, 1.0f, ((float) value - 64.0f) / 63.0f); break;
        case 11:    expression = amount * amount; break;

        case 64:
            pedal = value >= 64;

            if (! pedal)
                for (auto& v : voices)
                    if (v.active && v.held)
                        letGo (v);
            break;

        case 121:
            bend = wheel = panning = 0.0f;
            volume = expression = 1.0f;
            controller (64, 0);
            break;

        default:
            break;
    }
}

void Engine::steer (Voice& v)
{
    const auto& s = settings;
    const auto& zone = *v.zone;
    const auto seconds = (double) v.played / rate;

    // The LFO where it has reached, and how far in it has come.
    const auto beats = lfoSyncBeats (s.lfoSync);
    const auto hz = beats > 0.0 ? tempo / (60.0 * beats) : (double) s.lfoRate;
    const auto phase = v.lfoPhase - std::floor (v.lfoPhase);
    auto lfo = 0.0f;

    switch (s.lfoShape)
    {
        case LfoShape::sine:        lfo = (float) std::sin (juce::MathConstants<double>::twoPi * phase); break;
        case LfoShape::triangle:    lfo = (float) (phase < 0.25 ? 4.0 * phase : phase < 0.75 ? 2.0 - 4.0 * phase : 4.0 * phase - 4.0); break;
        case LfoShape::saw:         lfo = (float) (2.0 * phase - 1.0); break;
        case LfoShape::square:      lfo = phase < 0.5 ? 1.0f : -1.0f; break;
        case LfoShape::random:      lfo = (float) (hashed (v.seed + (juce::uint32) std::floor (v.lfoPhase)) & 0xffffu) / 32767.5f - 1.0f; break;
    }

    if (s.lfoFade > 0.0f)
        lfo *= (float) juce::jmin (1.0, seconds / (s.lfoFade * 0.001));

    v.lfoPhase += hz * controlInterval / rate;

    // Its pitch: the key against the zone's root, the tunings, the bend, the glide and the vibrato.
    if (v.glide != 0.0f)
    {
        v.glide *= (float) std::exp (-3.0 * controlInterval / juce::jmax (1.0, (double) s.glide * 0.001 * rate));

        if (std::abs (v.glide) < 0.001f)
            v.glide = 0.0f;
    }

    const auto semitones = (double) (v.note - zone.root) + s.transpose + v.glide + bend * s.bendRange
                           + 0.01 * (zone.tune + s.fine + lfo * s.lfoPitch + wheel * s.wheel * (float) std::sin (juce::MathConstants<double>::twoPi * phase));
    v.ratio = zone.sample->rate / rate * std::pow (2.0, semitones / 12.0);
    v.bank = bankFor (v.ratio);
    v.endFade = (float) (1.0 / juce::jmax (1.0, endFadeSeconds * rate * v.ratio));

    // The filter: where its cutoff is now, reached over this interval.
    if (s.filter != FilterType::off)
    {
        const auto octaves = s.keyTrack * 0.01f * (float) (v.note - 60) / 12.0f
                             + v.filterEnvelope.level * s.filterEnvelope * 0.08f
                             + v.velocity * s.velocityToCutoff * 0.04f
                             + lfo * s.lfoFilter * 0.04f;
        const auto cutoff = juce::jlimit (20.0, 0.45 * rate, (double) s.cutoff * std::pow (2.0, (double) octaves));
        const auto g = (float) std::tan (juce::MathConstants<double>::pi * cutoff / rate);
        v.k = 1.414f * std::pow (0.035f, s.resonance * 0.01f);

        if (! v.filterSet)
        {
            v.g = g;
            v.filterSet = true;
        }

        v.gStep = (g - v.g) / (float) controlInterval;
    }

    // Its level and place: the zone's, the velocity and the LFO.
    const auto sense = s.velocity * 0.01f;
    const auto level = gainOf (zone.level) * (1.0f - sense + sense * v.velocity * v.velocity)
                       * (1.0f - s.lfoLevel * 0.01f * (0.5f - 0.5f * lfo));
    const auto [l, r] = panned (zone.pan * 0.01f + lfo * s.lfoPan * 0.01f);

    if (! v.gainsSet)
    {
        v.left = level * l;
        v.right = level * r;
        v.gainsSet = true;
    }

    v.leftStep = (level * l - v.left) / (float) controlInterval;
    v.rightStep = (level * r - v.right) / (float) controlInterval;
}

void Engine::renderVoice (Voice& v, float* left, float* right, int count)
{
    const auto& s = settings;

    if (v.nextProgram != nullptr)
    {
        // Over to the changed zone where both read the sample as it lies: clear of either loop's
        // turn and crossfade, the same frame of the sample under it.
        const auto& from = *v.view;
        const auto& to = v.nextProgram->prepared[(size_t) v.nextZone];
        const auto moved = v.position + (double) (from.reverse ? to.last - from.last : from.first - to.first);
        const auto half = banks()[(size_t) v.bank].half;

        auto lies = [&] (const Program::Prepared& p, double position)
        {
            const auto first = (juce::int64) position - half + 1, last = first + 2 * half;
            const auto loops = p.loop != 0 && ! v.leaving;
            return position >= 0.0 && first >= (loops && v.wrapped ? p.loopStart : 0) && last <= (loops ? p.loopEnd - p.fade : p.length);
        };

        if (lies (from, v.position) && lies (to, moved))
        {
            v.position = moved;
            v.program = std::move (v.nextProgram);
            v.zone = &v.program->zones[(size_t) v.nextZone];
            v.view = &to;
            v.nextProgram = nullptr;
            v.nextZone = -1;
        }
    }
    const auto& audio = v.zone->sample->audio;
    const auto& view = *v.view;
    const auto stereo = audio.getNumChannels() > 1;
    const auto* sourceL = audio.getReadPointer (0);
    const auto* sourceR = audio.getReadPointer (stereo ? 1 : 0);
    const auto* turnL = view.fade > 0 ? view.turn.getReadPointer (0) : nullptr;
    const auto* turnR = view.fade > 0 ? view.turn.getReadPointer (view.turn.getNumChannels() > 1 ? 1 : 0) : nullptr;
    const auto loopLength = view.loopEnd - view.loopStart;

    const auto rise = (float) (1.0 / juce::jmax (1.0, juce::jmax (minimumAttackSeconds, (double) s.attack * 0.001) * rate));
    const auto fall = fallOver (s.decay, rate), let = fallOver (s.release, rate);
    const auto sustain = s.sustain * 0.01f;
    const auto filterRise = (float) (1.0 / juce::jmax (1.0, (double) s.filterAttack * 0.001 * rate));
    const auto filterFall = fallOver (s.filterDecay, rate), filterLet = fallOver (s.filterRelease, rate);
    const auto filterSustain = s.filterSustain * 0.01f;
    const auto fadeStep = (float) (1.0 / (fadeSeconds * rate));
    const auto type = s.filter;
    std::array<float, (size_t) mostPoints> windowL, windowR;

    for (int i = 0; i < count; ++i)
    {
        if (v.played % controlInterval == 0)
            steer (v);

        // The sample at its place, through the bank for its speed. Frames outside the part played are silence.
        const auto& bank = banks()[(size_t) v.bank];
        const auto points = 2 * bank.half;
        const auto index = (juce::int64) v.position;
        const auto fraction = v.position - (double) index;
        const auto from = index - bank.half + 1;
        auto looping = view.loop != 0 && ! v.leaving;
        float xl, xr;

        // Let go with a loop that ends with the note: once clear of the loop's turn (and on its way
        // forward), it plays on to the end as if there were no loop.
        if (looping && ! view.loopsInRelease && ! v.down && ! v.held
            && from >= view.loopStart && from + points <= view.loopEnd - view.fade
            && (view.loop == 1 || ! v.wrapped || v.position < (double) view.loopEnd))
        {
            v.leaving = true;
            looping = false;
        }

        const auto lies = ! view.reverse && from >= (looping && v.wrapped ? view.loopStart : 0)
                          && from + points <= (looping ? view.loopEnd - view.fade : view.length);

        // The frame `at` along the way it plays: through the loop, its crossfade and a reversed part.
        auto frameAt = [&] (juce::int64 at, float& l, float& r)
        {
            if (looping && (at >= view.loopEnd || (v.wrapped && at < view.loopStart)))
            {
                // Past the loop's end, or before its start once it has gone round: where the loop puts it.
                auto along = (at - view.loopStart) % view.period;

                if (along < 0)
                    along += view.period;

                at = view.loopStart + (along < loopLength ? along : view.period - along);
            }

            if (looping && view.fade > 0 && at >= view.loopEnd - view.fade && at < view.loopEnd)
            {
                l = turnL[at - (view.loopEnd - view.fade)];
                r = turnR[at - (view.loopEnd - view.fade)];
            }
            else if (at >= 0 && at < view.length)
            {
                const auto frame = view.reverse ? view.last - 1 - at : view.first + at;
                l = sourceL[frame];
                r = sourceR[frame];
            }
            else
            {
                l = r = 0.0f;
            }
        };

        if (juce::exactlyEqual (v.ratio, 1.0) && juce::exactlyEqual (fraction, 0.0))
        {
            // At its own pitch and rate, on a frame: the sample itself.
            frameAt (index, xl, xr);
        }
        else if (lies)
        {
            // Nothing in reach but the sample as it lies.
            xl = readThrough (bank, sourceL + view.first + from, fraction);
            xr = stereo ? readThrough (bank, sourceR + view.first + from, fraction) : xl;
        }
        else
        {
            for (int j = 0; j < points; ++j)
                frameAt (from + j, windowL[(size_t) j], windowR[(size_t) j]);

            xl = readThrough (bank, windowL.data(), fraction);
            xr = stereo ? readThrough (bank, windowR.data(), fraction) : xl;
        }

        const auto goesRound = looping;
        const auto ending = goesRound ? 1.0f : juce::jlimit (0.0f, 1.0f, (float) ((double) view.length - v.position) * v.endFade);
        v.position += v.ratio;

        if (goesRound)
        {
            const auto turnsAt = view.loop == 1 ? view.loopEnd : view.loopStart + view.period;

            while (v.position >= (double) turnsAt)
            {
                v.position -= (double) view.period;
                v.wrapped = true;
            }
        }

        v.amp.tick (rise, fall, sustain, let);
        v.filterEnvelope.tick (filterRise, filterFall, filterSustain, filterLet);

        if (type != FilterType::off)
        {
            v.g += v.gStep;

            for (int c = 0; c < 2; ++c)
            {
                auto& x = c == 0 ? xl : xr;
                float low, band, high;
                v.stages[0][(size_t) c].run (x, v.g, v.k, low, band, high);

                if (type == FilterType::lowPass24)
                {
                    float second, unused1, unused2;
                    v.stages[1][(size_t) c].run (low, v.g, 1.414f, second, unused1, unused2);
                    x = second;
                }
                else
                {
                    x = type == FilterType::bandPass ? band : type == FilterType::highPass ? high : low;
                }
            }
        }

        v.left += v.leftStep;
        v.right += v.rightStep;

        const auto gain = v.amp.level * ending * v.fadeGain;
        left[i] += xl * gain * v.left;
        right[i] += xr * gain * v.right;

        ++v.played;

        if (v.dying)
            v.fadeGain -= fadeStep;

        if (v.fadeGain <= 0.0f || v.amp.stage == Envelope::Stage::over || (! goesRound && v.position >= (double) view.length))
        {
            v.view = nullptr;
            v.nextProgram = nullptr;
            v.active = false;
            v.program = nullptr;
            v.zone = nullptr;
            break;
        }
    }
}

void Engine::render (float* left, float* right, int count)
{
    const auto smooth = (float) (1.0 - std::exp (-1.0 / (0.005 * rate)));

    for (int done = 0; done < count;)
    {
        const auto n = juce::jmin (count - done, (int) scratchL.size());
        std::fill_n (scratchL.data(), n, 0.0f);
        std::fill_n (scratchR.data(), n, 0.0f);

        for (auto& v : voices)
            if (v.active)
                renderVoice (v, scratchL.data(), scratchR.data(), n);

        // The output's level and place, reached over a few milliseconds, sample by sample.
        const auto [l, r] = panned (settings.pan * 0.01f + panning);
        const auto level = gainOf (settings.level) * volume * expression;

        if (! gainsSet)
        {
            gainL = level * l;
            gainR = level * r;
            gainsSet = true;
        }

        for (int i = 0; i < n; ++i)
        {
            gainL += smooth * (level * l - gainL);
            gainR += smooth * (level * r - gainR);
            const auto yl = scratchL[(size_t) i] * gainL, yr = scratchR[(size_t) i] * gainR;

            if (right != nullptr)
            {
                left[done + i] += yl;
                right[done + i] += yr;
            }
            else
            {
                left[done + i] += 0.5f * (yl + yr);
            }
        }

        done += n;
    }

    for (size_t i = 0; i < voices.size(); ++i)
    {
        const auto& v = voices[i];
        readings[i].position.store (v.active && ! v.dying ? (float) v.position : -1.0f, std::memory_order_relaxed);
        readings[i].zone.store (v.active && v.zone != nullptr ? v.zone->id : 0, std::memory_order_relaxed);
    }
}
}

//==============================================================================
namespace BuiltInEffects
{
using namespace Sampler;

SamplerProcessor::SamplerProcessor()
    : BuiltInEffect (samplerDescription(), "TORQWAVE_SAMPLER", false, true)
{
    const Settings s;

    auto keep = [this] (juce::RangedAudioParameter* p)
    {
        byId[p->paramID] = p;
    };

    auto times = [] (float low, float high)     { return juce::NormalisableRange<float> { low, high, 0.0f, 0.3f }; };

    keep (addFloat ("level", "Level", { -48.0f, 6.0f }, s.level, "dB"));
    keep (addFloat ("pan", "Pan", { -100.0f, 100.0f }, s.pan));
    keep (addFloat ("transpose", "Transpose", { -24.0f, 24.0f, 1.0f }, s.transpose, "st"));
    keep (addFloat ("fine", "Fine", { -100.0f, 100.0f }, s.fine, "ct"));
    keep (addFloat ("bendRange", "Bend Range", { 0.0f, 24.0f, 1.0f }, s.bendRange, "st"));
    keep (addChoice ("voices", "Voices", { "Poly", "Mono" }, 0));
    keep (addFloat ("glide", "Glide", times (0.0f, 3000.0f), s.glide, "ms"));
    keep (addFloat ("velocity", "Velocity", { 0.0f, 100.0f }, s.velocity, "%"));

    keep (addFloat ("attack", "Attack", times (0.0f, 10000.0f), s.attack, "ms"));
    keep (addFloat ("decay", "Decay", times (1.0f, 10000.0f), s.decay, "ms"));
    keep (addFloat ("sustain", "Sustain", { 0.0f, 100.0f }, s.sustain, "%"));
    keep (addFloat ("release", "Release", times (1.0f, 10000.0f), s.release, "ms"));

    keep (addChoice ("filter", "Filter", filterNames(), (int) s.filter));
    keep (addFloat ("cutoff", "Cutoff", { 20.0f, 20000.0f, 0.0f, 0.25f }, s.cutoff, "Hz"));
    keep (addFloat ("resonance", "Resonance", { 0.0f, 100.0f }, s.resonance, "%"));
    keep (addFloat ("keyTrack", "Key Tracking", { 0.0f, 100.0f }, s.keyTrack, "%"));
    keep (addFloat ("filterEnv", "Filter Envelope", { -100.0f, 100.0f }, s.filterEnvelope, "%"));
    keep (addFloat ("filterAttack", "Filter Attack", times (0.0f, 10000.0f), s.filterAttack, "ms"));
    keep (addFloat ("filterDecay", "Filter Decay", times (1.0f, 10000.0f), s.filterDecay, "ms"));
    keep (addFloat ("filterSustain", "Filter Sustain", { 0.0f, 100.0f }, s.filterSustain, "%"));
    keep (addFloat ("filterRelease", "Filter Release", times (1.0f, 10000.0f), s.filterRelease, "ms"));
    keep (addFloat ("velocityToCutoff", "Velocity to Cutoff", { 0.0f, 100.0f }, s.velocityToCutoff, "%"));

    keep (addFloat ("lfoRate", "LFO Rate", { 0.02f, 40.0f, 0.0f, 0.3f }, s.lfoRate, "Hz"));
    keep (addChoice ("lfoSync", "LFO Sync", lfoSyncNames(), s.lfoSync));
    keep (addChoice ("lfoShape", "LFO Shape", lfoShapeNames(), (int) s.lfoShape));
    keep (addFloat ("lfoFade", "LFO Fade In", times (0.0f, 5000.0f), s.lfoFade, "ms"));
    keep (addFloat ("lfoPitch", "LFO to Pitch", { 0.0f, 1200.0f, 0.0f, 0.3f }, s.lfoPitch, "ct"));
    keep (addFloat ("lfoFilter", "LFO to Filter", { -100.0f, 100.0f }, s.lfoFilter, "%"));
    keep (addFloat ("lfoLevel", "LFO to Level", { 0.0f, 100.0f }, s.lfoLevel, "%"));
    keep (addFloat ("lfoPan", "LFO to Pan", { 0.0f, 100.0f }, s.lfoPan, "%"));
    keep (addFloat ("wheel", "Wheel Vibrato", { 0.0f, 200.0f }, s.wheel, "ct"));
}

SamplerProcessor::~SamplerProcessor() = default;

juce::RangedAudioParameter* SamplerProcessor::parameter (const juce::String& id) const
{
    const auto found = byId.find (id);
    return found != byId.end() ? found->second : nullptr;
}

Settings SamplerProcessor::currentSettings() const
{
    auto value = [this] (const char* id)
    {
        auto* p = parameter (id);
        return p != nullptr ? p->convertFrom0to1 (p->getValue()) : 0.0f;
    };

    auto choice = [&] (const char* id)      { return juce::roundToInt (value (id)); };

    Settings s;
    s.level = value ("level");
    s.pan = value ("pan");
    s.transpose = value ("transpose");
    s.fine = value ("fine");
    s.bendRange = value ("bendRange");
    s.mono = choice ("voices") == 1;
    s.glide = value ("glide");
    s.velocity = value ("velocity");
    s.attack = value ("attack");
    s.decay = value ("decay");
    s.sustain = value ("sustain");
    s.release = value ("release");
    s.filter = (FilterType) choice ("filter");
    s.cutoff = value ("cutoff");
    s.resonance = value ("resonance");
    s.keyTrack = value ("keyTrack");
    s.filterEnvelope = value ("filterEnv");
    s.filterAttack = value ("filterAttack");
    s.filterDecay = value ("filterDecay");
    s.filterSustain = value ("filterSustain");
    s.filterRelease = value ("filterRelease");
    s.velocityToCutoff = value ("velocityToCutoff");
    s.lfoRate = value ("lfoRate");
    s.lfoSync = choice ("lfoSync");
    s.lfoShape = (LfoShape) choice ("lfoShape");
    s.lfoFade = value ("lfoFade");
    s.lfoPitch = value ("lfoPitch");
    s.lfoFilter = value ("lfoFilter");
    s.lfoLevel = value ("lfoLevel");
    s.lfoPan = value ("lfoPan");
    s.wheel = value ("wheel");
    return s;
}

void SamplerProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    engine.setSettings (currentSettings());

    const juce::SpinLock::ScopedLockType lock (programLock);
    engine.setProgram (program);
    pending = nullptr;
    programChanged.store (false);
    oldestInUse.store (engine.oldestGenerationInUse());
}

void SamplerProcessor::reset()
{
    engine.allSoundOff();
}

double SamplerProcessor::getTailLengthSeconds() const
{
    if (auto* p = parameter ("release"))
        return 0.1 + 0.0014 * p->convertFrom0to1 (p->getValue());

    return 0.1;
}

void SamplerProcessor::retire()
{
    const auto oldest = oldestInUse.load();
    retired.erase (std::remove_if (retired.begin(), retired.end(), [oldest] (const ProgramPtr& p) { return p->generation < oldest; }), retired.end());
}

void SamplerProcessor::setZones (std::vector<Zone> zones)
{
    for (auto& z : zones)
    {
        if (z.id == 0)
            z.id = ++zoneIds;
        else
            zoneIds = juce::jmax (zoneIds, z.id);
    }

    auto next = makeProgram (std::move (zones), ++generations);

    const juce::SpinLock::ScopedLockType lock (programLock);

    if (program != nullptr)
        retired.push_back (program);

    retire();
    program = next;
    pending = std::move (next);
    programChanged.store (true);
    version.fetch_add (1);
}

std::vector<Zone> SamplerProcessor::getZones() const
{
    const juce::SpinLock::ScopedLockType lock (programLock);
    return program != nullptr ? program->zones : std::vector<Zone>();
}

void SamplerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    const auto n = buffer.getNumSamples();
    buffer.clear();

    if (programChanged.load())
    {
        const juce::SpinLock::ScopedTryLockType lock (programLock);

        if (lock.isLocked() && pending != nullptr)
        {
            engine.setProgram (std::move (pending));
            pending = nullptr;
            programChanged.store (false);
        }
    }

    if (auto* head = getPlayHead())
        if (const auto position = head->getPosition())
            if (const auto bpm = position->getBpm(); bpm.hasValue() && *bpm > 0.0)
                engine.setTempo (*bpm);

    engine.setSettings (currentSettings());

    {
        const juce::SpinLock::ScopedTryLockType lock (windowLock);

        if (lock.isLocked() && ! windowNotes.empty())
        {
            for (auto& [note, velocity] : windowNotes)
            {
                if (velocity >= 0.0f)
                    engine.noteOn (note, velocity);
                else
                    engine.noteOff (note);
            }

            windowNotes.clear();
        }
    }

    auto* left = buffer.getNumChannels() > 0 ? buffer.getWritePointer (0) : nullptr;
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    int done = 0;

    auto renderTo = [&] (int at)
    {
        if (left != nullptr && at > done)
            engine.render (left + done, right != nullptr ? right + done : nullptr, at - done);

        done = at;
    };

    for (const auto meta : midi)
    {
        renderTo (juce::jlimit (done, n, meta.samplePosition));
        const auto m = meta.getMessage();

        if (m.isNoteOn())
            engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())
            engine.noteOff (m.getNoteNumber());
        else if (m.isAllSoundOff())
            engine.allSoundOff();
        else if (m.isAllNotesOff())
            engine.allNotesOff();
        else if (m.isPitchWheel())
            engine.pitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.0f);
        else if (m.isController())
            engine.controller (m.getControllerNumber(), m.getControllerValue());
    }

    renderTo (n);
    midi.clear();
    oldestInUse.store (engine.oldestGenerationInUse());
}

bool SamplerProcessor::slice (const SamplePtr& sample, const juce::File& file, juce::Range<juce::int64> part, Slicing how)
{
    if (sample == nullptr)
        return false;

    const auto slices = findSlices (*sample, part, how);

    if (slices.empty())
        return false;

    how.on = true;
    slicing = how;
    setZones (zonesFromSlices (sample, file, slices, how.cutEachOther));
    return true;
}

void SamplerProcessor::play (int note, float velocity)
{
    const juce::SpinLock::ScopedLockType lock (windowLock);

    if (windowNotes.size() < 256)
        windowNotes.emplace_back (juce::jlimit (0, 127, note), juce::jlimit (0.0f, 1.0f, velocity));
}

void SamplerProcessor::stop (int note)
{
    const juce::SpinLock::ScopedLockType lock (windowLock);

    if (windowNotes.size() < 256)
        windowNotes.emplace_back (juce::jlimit (0, 127, note), -1.0f);
}

juce::String SamplerProcessor::addSample (const SamplePtr& sample, const juce::File& file, juce::uint32 replace, int atKey)
{
    if (sample == nullptr)
        return "There is no sound to add.";

    if (slicing.on)
    {
        // Slices take the place of what was there, so only the new sample has to fit.
        if (sample->bytes() > most)
            return sample->name + " takes " + juce::File::descriptionOfSizeInBytes (sample->bytes()) + ". One sampler can hold " + juce::File::descriptionOfSizeInBytes (most)
                   + " of samples: they are held in memory, not played from disk.";

        auto how = slicing;

        if (findSlices (*sample, {}, how).empty())
            how.source = SliceSource::hits;     // no markers or regions to cut at

        return slice (sample, file, {}, how) ? juce::String() : sample->name + " is too short to slice.";
    }

    auto zones = getZones();
    const auto old = std::find_if (zones.begin(), zones.end(), [replace] (const Zone& z) { return replace != 0 && z.id == replace; });

    if (old == zones.end() && (int) zones.size() >= maxZones)
        return "A sampler has at most " + juce::String (maxZones) + " zones.";

    // Replacing a zone's sample frees the old one when no other zone plays it.
    {
        std::set<const Sample*> kept;
        juce::int64 held = 0;

        for (auto& z : zones)
            if (z.sample != nullptr && &z != &*old && kept.insert (z.sample.get()).second)
                held += z.sample->bytes();

        if (kept.count (sample.get()) == 0 && held + sample->bytes() > most)
            return roomFor (sample).isNotEmpty() ? roomFor (sample)
                                                 : sample->name + " doesn't fit: one sampler can hold " + juce::File::descriptionOfSizeInBytes (most) + " of samples.";
    }

    Zone z;
    z.sample = sample;
    z.file = file;
    z.name = sample->name;

    if (sample->fileRoot >= 0)
    {
        z.root = sample->fileRoot;
        z.tune = sample->fileTune;
    }
    else
    {
        const auto pitch = findPitch (*sample);
        z.root = pitch.root;
        z.tune = pitch.tune;
    }

    if (sample->fileLoop != 0)
    {
        z.loop = sample->fileLoop;
        z.loopStart = sample->fileLoopStart;
        z.loopEnd = sample->fileLoopEnd;
    }

    if (old != zones.end())
    {
        z.id = old->id;
        z.lowKey = old->lowKey;
        z.highKey = old->highKey;
        z.lowVelocity = old->lowVelocity;
        z.highVelocity = old->highVelocity;
        z.level = old->level;
        z.pan = old->pan;
        z.choke = old->choke;
        *old = std::move (z);
    }
    else
    {
        if (! zones.empty() && atKey >= 0)
        {
            z.lowKey = juce::jlimit (0, 127, atKey);
            z.highKey = juce::jmin (127, z.lowKey + 11);
        }

        zones.push_back (std::move (z));
    }

    setZones (std::move (zones));
    return {};
}

juce::String SamplerProcessor::addSamples (const std::vector<std::pair<SamplePtr, juce::File>>& sounds, int fromKey)
{
    if (sounds.empty())
        return {};

    if (slicing.on || sounds.size() == 1)
        return addSample (sounds.front().first, sounds.front().second);

    std::vector<Zone> added;
    auto allPitched = true;

    for (auto& [sample, file] : sounds)
    {
        if (sample == nullptr)
            continue;

        Zone z;
        z.sample = sample;
        z.file = file;
        z.name = sample->name;

        if (sample->fileRoot >= 0)
        {
            z.root = sample->fileRoot;
            z.tune = sample->fileTune;
        }
        else
        {
            const auto pitch = findPitch (*sample);
            z.root = pitch.root;
            z.tune = pitch.tune;
            allPitched = allPitched && pitch.found;
        }

        added.push_back (std::move (z));
    }

    if (allPitched)
    {
        // Around their own notes: each from half way down to the one below to half way up to the one above.
        std::stable_sort (added.begin(), added.end(), [] (const Zone& a, const Zone& b) { return a.root < b.root; });

        for (size_t i = 0; i < added.size(); ++i)
        {
            added[i].lowKey = i == 0 ? 0 : added[i - 1].highKey + 1;
            added[i].highKey = i + 1 == added.size() ? 127 : juce::jmax (added[i].lowKey, (added[i].root + added[i + 1].root) / 2);
        }
    }
    else
    {
        for (size_t i = 0; i < added.size(); ++i)
        {
            added[i].root = added[i].lowKey = added[i].highKey = juce::jlimit (0, 127, fromKey + (int) i);
            added[i].tune = 0.0f;
            added[i].oneShot = true;
        }
    }

    auto zones = getZones();
    juce::String error;

    for (auto& z : added)
    {
        if ((int) zones.size() >= maxZones)
        {
            error = "A sampler has at most " + juce::String (maxZones) + " zones.";
            break;
        }

        std::set<const Sample*> held;
        juce::int64 bytes = 0;

        for (auto& other : zones)
            if (other.sample != nullptr && held.insert (other.sample.get()).second)
                bytes += other.sample->bytes();

        if (held.count (z.sample.get()) == 0 && bytes + z.sample->bytes() > most)
        {
            error = z.sample->name + " and what follows were left out: one sampler can hold " + juce::File::descriptionOfSizeInBytes (most) + " of samples.";
            break;
        }

        zones.push_back (z);
    }

    setZones (std::move (zones));
    return error;
}

EffectPresets::Texts SamplerProcessor::getPresetTexts() const
{
    juce::XmlElement holder ("SAMPLER");
    writeZones (holder);
    return { { "zones", holder.getChildByName ("ZONES")->toString (juce::XmlElement::TextFormat().singleLine()) } };
}

void SamplerProcessor::setPresetTexts (const EffectPresets::Texts& texts)
{
    const auto found = texts.find ("zones");

    if (found == texts.end() || found->second.isEmpty())
        return;

    if (auto zones = juce::XmlDocument::parse (found->second))
    {
        juce::XmlElement holder ("SAMPLER");
        holder.addChildElement (zones.release());
        readZones (holder);
    }
}

void SamplerProcessor::addToState (juce::XmlElement& xml) const
{
    writeZones (xml);
}

void SamplerProcessor::readFromState (const juce::XmlElement& xml)
{
    readZones (xml);
}

void SamplerProcessor::writeZones (juce::XmlElement& xml) const
{
    auto* all = xml.createNewChildElement ("ZONES");
    const auto saving = [] { const juce::ScopedLock sl (store().lock); return store().savingInto; }();
    all->setAttribute ("slices", slicing.on ? 1 : 0);
    all->setAttribute ("sliceSource", (int) slicing.source);
    all->setAttribute ("sensitivity", (double) slicing.sensitivity);
    all->setAttribute ("parts", slicing.parts);
    all->setAttribute ("cutEachOther", slicing.cutEachOther ? 1 : 0);

    for (auto& z : getZones())
    {
        auto* e = all->createNewChildElement ("ZONE");
        e->setAttribute ("id", (int) z.id);
        e->setAttribute ("file", z.file.getFullPathName());

        // Saved into a session: relative to its folder too, as its clips' audio is, so the folder can be moved.
        if (saving != juce::File() && z.file != juce::File())
            e->setAttribute ("relative", z.file.getRelativePathFrom (saving));

        if (z.name.isNotEmpty())
            e->setAttribute ("name", z.name);

        e->setAttribute ("root", z.root);
        e->setAttribute ("tune", (double) z.tune);
        e->setAttribute ("lowKey", z.lowKey);
        e->setAttribute ("highKey", z.highKey);
        e->setAttribute ("lowVelocity", z.lowVelocity);
        e->setAttribute ("highVelocity", z.highVelocity);
        e->setAttribute ("start", juce::String (z.start));
        e->setAttribute ("end", juce::String (z.end));
        e->setAttribute ("level", (double) z.level);
        e->setAttribute ("pan", (double) z.pan);
        e->setAttribute ("reverse", z.reverse ? 1 : 0);
        e->setAttribute ("oneShot", z.oneShot ? 1 : 0);
        e->setAttribute ("loop", z.loop);
        e->setAttribute ("loopStart", juce::String (z.loopStart));
        e->setAttribute ("loopEnd", juce::String (z.loopEnd));
        e->setAttribute ("crossfade", (double) z.crossfade);
        e->setAttribute ("loopInRelease", z.loopInRelease ? 1 : 0);
        e->setAttribute ("choke", z.choke);
    }
}

void SamplerProcessor::readZones (const juce::XmlElement& xml)
{
    const auto* all = xml.getChildByName ("ZONES");

    if (all == nullptr)
        return;

    std::map<juce::String, SamplePtr> read;
    std::vector<Zone> zones;
    missing.clear();
    const auto sessionFolder = getSessionFolder();
    juce::int64 held = 0;

    slicing = {};
    slicing.on = all->getIntAttribute ("slices") != 0;
    slicing.source = (SliceSource) juce::jlimit (0, 3, all->getIntAttribute ("sliceSource", (int) slicing.source));
    slicing.sensitivity = (float) all->getDoubleAttribute ("sensitivity", slicing.sensitivity);
    slicing.parts = juce::jlimit (2, maxSlices, all->getIntAttribute ("parts", slicing.parts));
    slicing.cutEachOther = all->getIntAttribute ("cutEachOther") != 0;

    for (auto* e : all->getChildWithTagNameIterator ("ZONE"))
    {
        Zone z;
        z.id = (juce::uint32) e->getIntAttribute ("id");
        z.name = e->getStringAttribute ("name");
        z.root = juce::jlimit (0, 127, e->getIntAttribute ("root", 60));
        z.tune = (float) e->getDoubleAttribute ("tune");
        z.lowKey = juce::jlimit (0, 127, e->getIntAttribute ("lowKey", 0));
        z.highKey = juce::jlimit (0, 127, e->getIntAttribute ("highKey", 127));
        z.lowVelocity = juce::jlimit (1, 127, e->getIntAttribute ("lowVelocity", 1));
        z.highVelocity = juce::jlimit (1, 127, e->getIntAttribute ("highVelocity", 127));
        z.start = e->getStringAttribute ("start").getLargeIntValue();
        z.end = e->getStringAttribute ("end", "-1").getLargeIntValue();
        z.level = (float) e->getDoubleAttribute ("level");
        z.pan = (float) e->getDoubleAttribute ("pan");
        z.reverse = e->getIntAttribute ("reverse") != 0;
        z.oneShot = e->getIntAttribute ("oneShot") != 0;
        z.loop = juce::jlimit (0, 2, e->getIntAttribute ("loop"));
        z.loopStart = e->getStringAttribute ("loopStart").getLargeIntValue();
        z.loopEnd = e->getStringAttribute ("loopEnd", "-1").getLargeIntValue();
        z.crossfade = (float) e->getDoubleAttribute ("crossfade");
        z.loopInRelease = e->getIntAttribute ("loopInRelease", 1) != 0;
        z.choke = e->getIntAttribute ("choke");

        if (auto path = e->getStringAttribute ("file"); path.isNotEmpty())
        {
            // Beside its session first (where it is relative to the session's folder), then where it was.
            if (const auto relative = e->getStringAttribute ("relative"); relative.isNotEmpty() && sessionFolder != juce::File())
                if (const auto beside = sessionFolder.getChildFile (relative); beside.existsAsFile())
                    path = beside.getFullPathName();

            z.file = juce::File (path);

            if (const auto found = read.find (path); found != read.end())
            {
                z.sample = found->second;
            }
            else
            {
                juce::String error;
                z.sample = sharedSample (z.file, error);

                if (z.sample != nullptr && held + z.sample->bytes() > most)
                {
                    error = z.file.getFileName() + " was left out: with it the sampler would hold more than " + juce::File::descriptionOfSizeInBytes (most) + " of samples.";
                    z.sample = nullptr;
                }

                if (z.sample != nullptr)
                    held += z.sample->bytes();
                else
                    missing.add (error);

                read[path] = z.sample;
            }
        }

        zones.push_back (std::move (z));
    }

    setZones (std::move (zones));
}
}

namespace BuiltInEffects
{
juce::Array<juce::File> SamplerProcessor::getFiles() const
{
    juce::Array<juce::File> files;

    for (auto& z : getZones())
        if (z.file != juce::File())
            files.addIfNotAlreadyThere (z.file);

    return files;
}

juce::int64 SamplerProcessor::bytesHeld() const
{
    std::set<const Sampler::Sample*> counted;
    juce::int64 bytes = 0;

    for (auto& z : getZones())
        if (z.sample != nullptr && counted.insert (z.sample.get()).second)
            bytes += z.sample->bytes();

    return bytes;
}

juce::String SamplerProcessor::roomFor (const Sampler::SamplePtr& sample) const
{
    if (sample == nullptr)
        return {};

    for (auto& z : getZones())
        if (z.sample == sample)
            return {};

    if (bytesHeld() + sample->bytes() <= most)
        return {};

    return sample->name + " takes " + juce::File::descriptionOfSizeInBytes (sample->bytes()) + ", and the sampler holds " + juce::File::descriptionOfSizeInBytes (bytesHeld())
           + " already. One sampler can hold " + juce::File::descriptionOfSizeInBytes (most) + " of samples: they are held in memory, not played from disk.";
}

int SamplerProcessor::locate (const juce::File& found)
{
    auto zones = getZones();
    const auto folder = found.isDirectory() ? found : found.getParentDirectory();
    int back = 0;
    juce::StringArray still;

    for (auto& z : zones)
    {
        if (z.sample != nullptr || z.file == juce::File())
            continue;

        const auto candidate = ! found.isDirectory() && found.getFileName() == z.file.getFileName() ? found : folder.getChildFile (z.file.getFileName());
        juce::String error;
        auto sample = candidate.existsAsFile() ? Sampler::sharedSample (candidate, error) : nullptr;

        if (sample != nullptr && roomFor (sample).isEmpty())
        {
            z.sample = sample;
            z.file = candidate;
            ++back;
            setZones (zones);       // so the room left counts it
        }
        else
        {
            still.addIfNotAlreadyThere (error.isNotEmpty() ? error : z.file.getFileName() + " was not found.");
        }
    }

    missing = still;
    return back;
}
}
