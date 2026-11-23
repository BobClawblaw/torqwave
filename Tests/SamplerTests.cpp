#include <JuceHeader.h>
#include "../Source/AudioEngine.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/Sampler.h"
#include "TestUtils.h"
#include "SamplerTests.h"
#include "ClickCheck.h"

namespace
{
    using TestUtils::check;
    using Processor = BuiltInEffects::SamplerProcessor;

    constexpr double rate = 48000.0;
    constexpr double twoPi = juce::MathConstants<double>::twoPi;

    struct Event { double when; juce::MidiMessage message; };

    Event on (double when, int note, float velocity = 1.0f)   { return { when, juce::MidiMessage::noteOn (1, note, velocity) }; }
    Event off (double when, int note)                         { return { when, juce::MidiMessage::noteOff (1, note) }; }
    Event cc (double when, int number, int value)             { return { when, juce::MidiMessage::controllerEvent (1, number, value) }; }
    Event bend (double when, int value)                       { return { when, juce::MidiMessage::pitchWheel (1, value) }; }

    /** Plays the events for seconds in blocks of blockSize, from a fresh start. */
    juce::AudioBuffer<float> play (Processor& sampler, const std::vector<Event>& events, double seconds, int blockSize = 512)
    {
        sampler.prepareToPlay (rate, blockSize);
        const auto total = (int) (seconds * rate);
        juce::AudioBuffer<float> out (2, total), block (2, blockSize);

        for (int at = 0; at < total; at += blockSize)
        {
            const auto n = juce::jmin (blockSize, total - at);
            juce::MidiBuffer midi;

            for (auto& e : events)
                if (const auto sample = (int) (e.when * rate); sample >= at && sample < at + n)
                    midi.addEvent (e.message, sample - at);

            block.setSize (2, n, false, false, true);
            sampler.processBlock (block, midi);

            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, at, block, ch, 0, n);
        }

        return out;
    }

    void set (Processor& sampler, const juce::String& id, float plain)
    {
        if (auto* p = sampler.parameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (plain));
        else
            jassertfalse;
    }

    void defaults (Processor& sampler)
    {
        for (auto* p : sampler.getParameters())
            p->setValueNotifyingHost (p->getDefaultValue());
    }

    /** As it comes, but at full level whatever the velocity, so a render reads as the sample does. */
    void plain (Processor& sampler)
    {
        defaults (sampler);
        set (sampler, "level", 0.0f);
        set (sampler, "velocity", 0.0f);
    }

    bool same (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples())
            return false;

        for (int c = 0; c < a.getNumChannels(); ++c)
            if (std::memcmp (a.getReadPointer (c), b.getReadPointer (c), sizeof (float) * (size_t) a.getNumSamples()) != 0)
                return false;

        return true;
    }

    juce::AudioBuffer<float> sine (double hz, double seconds, double atRate = rate, int channels = 1, float level = 0.5f)
    {
        juce::AudioBuffer<float> b (channels, (int) (seconds * atRate));

        for (int c = 0; c < channels; ++c)
            for (int i = 0; i < b.getNumSamples(); ++i)
                b.setSample (c, i, level * (float) std::sin (twoPi * hz * i / atRate));

        return b;
    }

    juce::AudioBuffer<float> steady (double seconds, float level = 0.5f)
    {
        juce::AudioBuffer<float> b (1, (int) (seconds * rate));

        for (int i = 0; i < b.getNumSamples(); ++i)
            b.setSample (0, i, level);

        return b;
    }

    juce::AudioBuffer<float> noise (double seconds, juce::int64 seed = 7)
    {
        juce::Random random (seed);
        juce::AudioBuffer<float> b (1, (int) (seconds * rate));

        for (int i = 0; i < b.getNumSamples(); ++i)
            b.setSample (0, i, random.nextFloat() - 0.5f);

        return b;
    }

    /** One zone over every key, the sample at `root`. */
    void oneZone (Processor& sampler, const juce::AudioBuffer<float>& audio, int root, double atRate = rate)
    {
        Sampler::Zone z;
        z.sample = Sampler::makeSample (audio, atRate, "test");
        z.root = root;
        sampler.setZones ({ z });
    }

    /** The pitch of channel 0 over a stretch, from its autocorrelation's peak nearest the period expected. */
    double pitchOf (const juce::AudioBuffer<float>& audio, int from, int length, double expectedHz)
    {
        const auto* x = audio.getReadPointer (0) + from;
        const auto around = rate / expectedHz;

        auto correlation = [&] (int lag)
        {
            double sum = 0.0;

            for (int i = 0; i + lag < length; ++i)
                sum += (double) x[i] * x[i + lag];

            return sum;
        };

        int best = (int) around;
        double most = -1.0e30;

        for (int lag = (int) (around * 0.94); lag <= (int) (around * 1.06) + 1; ++lag)
            if (const auto c = correlation (lag); c > most)
            {
                most = c;
                best = lag;
            }

        const auto before = correlation (best - 1), after = correlation (best + 1);
        const auto shift = 0.5 * (before - after) / (before - 2.0 * most + after);
        return rate / (best + shift);
    }

    double centsOff (double hz, double wanted)     { return 1200.0 * std::log2 (hz / wanted); }

    float peak (const juce::AudioBuffer<float>& audio, double fromSeconds, double seconds, int channel = 0)
    {
        return audio.getMagnitude (channel, (int) (fromSeconds * rate), juce::jmax (1, (int) (seconds * rate)));
    }

    float rms (const juce::AudioBuffer<float>& audio, double fromSeconds, double seconds, int channel = 0)
    {
        return audio.getRMSLevel (channel, (int) (fromSeconds * rate), juce::jmax (1, (int) (seconds * rate)));
    }

    double db (double gain)                         { return 20.0 * std::log10 (juce::jmax (1.0e-12, gain)); }

    /** What is left of y once the tone at `cycles` a sample is taken out of it, against the tone, in dB. */
    double residueDb (const std::vector<double>& y, double cycles)
    {
        double cc = 0.0, ss = 0.0, cs = 0.0, yc = 0.0, ys = 0.0;

        for (size_t n = 0; n < y.size(); ++n)
        {
            const auto c = std::cos (twoPi * cycles * (double) n), s = std::sin (twoPi * cycles * (double) n);
            cc += c * c;
            ss += s * s;
            cs += c * s;
            yc += y[n] * c;
            ys += y[n] * s;
        }

        const auto det = cc * ss - cs * cs;
        const auto a = (yc * ss - ys * cs) / det, b = (ys * cc - yc * cs) / det;
        double tone = 0.0, rest = 0.0;

        for (size_t n = 0; n < y.size(); ++n)
        {
            const auto fit = a * std::cos (twoPi * cycles * (double) n) + b * std::sin (twoPi * cycles * (double) n);
            tone += fit * fit;
            rest += (y[n] - fit) * (y[n] - fit);
        }

        return 10.0 * std::log10 (juce::jmax (1.0e-30, rest) / juce::jmax (1.0e-30, tone));
    }

    /** The energy of channel 0 above a frequency against all of it, in dB, over a stretch. */
    double highsDb (const juce::AudioBuffer<float>& audio, double fromSeconds, double seconds, double aboveHz)
    {
        constexpr int order = 12, size = 1 << order;
        juce::dsp::FFT fft (order);
        juce::dsp::WindowingFunction<float> window ((size_t) size, juce::dsp::WindowingFunction<float>::hann);
        std::vector<float> data ((size_t) size * 2);
        double high = 0.0, all = 0.0;
        const auto first = (int) (fromSeconds * rate), last = (int) ((fromSeconds + seconds) * rate) - size;

        for (int at = first; at <= last; at += size / 2)
        {
            std::fill (data.begin(), data.end(), 0.0f);
            std::copy_n (audio.getReadPointer (0) + at, size, data.begin());
            window.multiplyWithWindowingTable (data.data(), (size_t) size);
            fft.performFrequencyOnlyForwardTransform (data.data());

            for (int bin = 1; bin < size / 2; ++bin)
            {
                const auto power = (double) data[(size_t) bin] * data[(size_t) bin];
                all += power;

                if (bin * rate / size >= aboveHz)
                    high += power;
            }
        }

        return 10.0 * std::log10 (juce::jmax (1.0e-30, high) / juce::jmax (1.0e-30, all));
    }

    juce::File writeWav (const juce::File& file, const juce::AudioBuffer<float>& audio, double atRate)
    {
        file.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream> (file);
        auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (atRate).withNumChannels (audio.getNumChannels()).withBitsPerSample (32)
                                                                                    .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint));
        if (writer != nullptr)
            writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());

        return file;
    }

    //==============================================================================
    /** The windowed sinc a sample is read through: what it leaves beside a tone, and what it lets through that it shouldn't. */
    void testInterpolation()
    {
        constexpr int length = 1 << 17, taken = 8192;
        double worstRest = -1000.0, worstLeak = -1000.0, worstGain = 0.0;
        juce::String restAt, leakAt;

        for (int semitones = -24; semitones <= 24; ++semitones)
        {
            // A little off each semitone, so the places read fall everywhere between the frames.
            const auto ratio = std::pow (2.0, (semitones + 0.137) / 12.0);
            const auto holds = juce::jmin (1.0, 1.0 / ratio);   // the part of the sample's band the output can hold

            for (const auto part : { 0.02, 0.1, 0.25, 0.45, 0.6, 0.7 })
            {
                const auto cycles = 0.5 * part * holds;         // of the sample's rate
                std::vector<float> source ((size_t) length);

                for (int i = 0; i < length; ++i)
                    source[(size_t) i] = (float) std::sin (twoPi * cycles * i + 0.3);

                std::vector<double> y ((size_t) taken);
                double sum = 0.0;

                for (int n = 0; n < taken; ++n)
                {
                    y[(size_t) n] = Sampler::interpolate (source.data(), length, 400.0 + n * ratio, ratio);
                    sum += y[(size_t) n] * y[(size_t) n];
                }

                if (const auto rest = residueDb (y, cycles * ratio); rest > worstRest)
                {
                    worstRest = rest;
                    restAt = juce::String (semitones) + " semitones, " + juce::String (part * 100.0, 0) + " % of the band";
                }

                worstGain = juce::jmax (worstGain, std::abs (db (std::sqrt (2.0 * sum / taken))));
            }

            // Played up, what the output can't hold is taken out rather than folded back.
            if (semitones > 0)
                for (const auto part : { 1.1, 1.3, 1.7 })
                {
                    const auto cycles = 0.5 * part * holds;

                    if (cycles >= 0.5)
                        continue;

                    std::vector<float> source ((size_t) length);

                    for (int i = 0; i < length; ++i)
                        source[(size_t) i] = (float) std::sin (twoPi * cycles * i + 0.3);

                    double sum = 0.0;

                    for (int n = 0; n < taken; ++n)
                    {
                        const auto y = (double) Sampler::interpolate (source.data(), length, 400.0 + n * ratio, ratio);
                        sum += y * y;
                    }

                    if (const auto leak = db (std::sqrt (2.0 * sum / taken)); leak > worstLeak)
                    {
                        worstLeak = leak;
                        leakAt = juce::String (semitones) + " semitones, " + juce::String (part * 100.0, 0) + " % of what the output holds";
                    }
                }
        }

        check (worstRest < -80.0 && worstLeak < -80.0 && worstGain < 0.1,
               "sampler: a tone read through the windowed sinc from two octaves down to two octaves up, anywhere in the lower 70 % of the band, leaves "
                   + juce::String (worstRest, 1) + " dB beside it at worst (" + restAt + ") and keeps its level within " + juce::String (worstGain, 3)
                   + " dB; played up, a tone the output can't hold comes through at " + juce::String (worstLeak, 1) + " dB at worst (" + leakAt + ")");
    }

    /** How strong the tone at hz is in channel 0 over a stretch: its amplitude, by least squares. */
    double toneAt (const juce::AudioBuffer<float>& audio, double fromSeconds, double seconds, double hz)
    {
        const auto* x = audio.getReadPointer (0) + (int) (fromSeconds * rate);
        const auto n = (int) (seconds * rate);
        double c = 0.0, s = 0.0;

        for (int i = 0; i < n; ++i)
        {
            c += x[i] * std::cos (twoPi * hz * i / rate);
            s += x[i] * std::sin (twoPi * hz * i / rate);
        }

        return 2.0 * std::sqrt (c * c + s * s) / n;
    }

    Sampler::Zone zoneOf (const juce::AudioBuffer<float>& audio, int root, const juce::String& name = "test")
    {
        Sampler::Zone z;
        z.sample = Sampler::makeSample (audio, rate, name);
        z.root = root;
        return z;
    }

    /** Zones, layers, loops, reverse, one-shots and choke groups. */
    void testZones (Processor& sampler)
    {
        // Each key and velocity plays the zones that cover it, and only those; zones that overlap play together.
        {
            plain (sampler);
            auto low = zoneOf (sine (200.0, 1.0), 48);
            low.highKey = 59;
            auto soft = zoneOf (sine (500.0, 1.0), 72);
            soft.lowKey = 60;
            soft.highVelocity = 79;
            auto hard = zoneOf (sine (800.0, 1.0), 72);
            hard.lowKey = 60;
            hard.lowVelocity = 80;
            auto layer = zoneOf (sine (1300.0, 1.0), 72);
            layer.lowKey = 60;
            layer.level = -6.0f;
            sampler.setZones ({ low, soft, hard, layer });

            auto heard = [&] (int key, float velocity)
            {
                const auto out = play (sampler, { on (0.0, key, velocity) }, 0.5);
                juce::String which;

                for (const auto hz : { 200.0, 500.0, 800.0, 1300.0 })
                    which << (toneAt (out, 0.1, 0.3, hz) > 0.05 ? "1" : "0");

                return which;
            };

            const auto lowKey = heard (48, 1.0f), softKey = heard (72, 0.5f), hardKey = heard (72, 1.0f), edge = heard (72, 79.0f / 127.0f), over = heard (72, 80.0f / 127.0f);
            const auto layered = play (sampler, { on (0.0, 72, 1.0f) }, 0.5);
            const auto layerDb = db (toneAt (layered, 0.1, 0.3, 1300.0) / toneAt (layered, 0.1, 0.3, 800.0));

            check (lowKey == "1000" && softKey == "0101" && hardKey == "0011" && edge == "0101" && over == "0011" && std::abs (layerDb + 6.0) < 0.1 && sampler.getNumSounding() == 2,
                   "sampler: a key below C3 plays the low zone alone; above it a soft note plays the soft zone and a hard one the hard zone (velocity 79 the one, 80 the other), each with the layer over them, "
                       + juce::String (-layerDb, 1) + " dB down as its level says");
        }

        // A loop: the sound goes on for as long as the key is down, with no step where it turns.
        {
            plain (sampler);
            set (sampler, "release", 20.0f);
            const auto tone = sine (220.0, 1.0);          // 2,400 frames are eleven of its cycles
            auto z = zoneOf (tone, 57);
            sampler.setZones ({ z });
            const auto once = play (sampler, { on (0.0, 57) }, 2.0);

            z.loop = 1;
            z.loopStart = 9600;
            z.loopEnd = 12000;
            sampler.setZones ({ z });
            const auto looped = play (sampler, { on (0.0, 57), off (10.0, 57) }, 10.5);
            auto apart = 0.0f;

            for (int i = 1000; i < (int) (10.0 * rate); ++i)
                apart = juce::jmax (apart, std::abs (looped.getSample (0, i) - 0.5f * (float) std::sin (twoPi * 220.0 * i / rate)));

            // A loop that doesn't meet itself: a step at the turn, which the crossfade takes away.
            z.loopEnd = 12100;
            sampler.setZones ({ z });
            const auto stepped = ClickCheck::worst (play (sampler, { on (0.0, 57) }, 3.0), rate, 0.01f);
            z.crossfade = 20.0f;
            sampler.setZones ({ z });
            const auto faded = play (sampler, { on (0.0, 57) }, 10.0);
            const auto crossfaded = ClickCheck::worst (faded, rate, 0.01f);

            // Forward and back: it turns round at each end.
            z.crossfade = 0.0f;
            z.loop = 2;
            sampler.setZones ({ z });
            const auto backAndForth = play (sampler, { on (0.0, 57) }, 10.0);
            const auto turned = ClickCheck::worst (backAndForth, rate, 0.01f);

            check (peak (once, 1.01, 0.9) == 0.0f && apart < 1.0e-3f && peak (looped, 10.2, 0.25) == 0.0f
                       && stepped.ratio > 4.0f && crossfaded.ratio < 4.0f && rms (faded, 9.0, 0.9) > 0.2f
                       && turned.ratio < 4.0f && rms (backAndForth, 9.0, 0.9) > 0.2f,
                   "sampler: a one-second tone ends after its second; looped over eleven of its cycles it is the tone for ten seconds, within " + juce::String (db (apart), 1)
                       + " dB, and ends when let go; a loop that doesn't meet itself steps at the turn (" + juce::String (stepped.ratio, 1) + " times its surroundings), and with a 20 ms crossfade doesn't ("
                       + juce::String (crossfaded.ratio, 2) + "); forward and back turns without a step (" + juce::String (turned.ratio, 2) + ")");
        }

        // The crossfade is the one Crossfade Loop makes: the loop's end fading out as what comes before its start fades in.
        {
            auto z = zoneOf (noise (1.0, 3), 60);
            z.loop = 1;
            z.loopStart = 20000;
            z.loopEnd = 30000;
            z.crossfade = 10.0f;
            const auto program = Sampler::makeProgram ({ z });
            const auto& p = program->prepared[0];
            const auto* x = z.sample->audio.getReadPointer (0);
            auto worst = 0.0f;

            for (int n = 0; n < (int) p.fade; ++n)
            {
                const auto in = std::sin (0.5 * juce::MathConstants<double>::pi * (n + 1) / (double) p.fade);
                const auto out = std::cos (0.5 * juce::MathConstants<double>::pi * (n + 1) / (double) p.fade);
                worst = juce::jmax (worst, std::abs (p.turn.getSample (0, n) - (float) (x[30000 - 480 + n] * out + x[20000 - 480 + n] * in)));
            }

            z.crossfade = 500.0f;
            const auto longest = Sampler::makeProgram ({ z })->prepared[0].fade;
            z.loopStart = 100;
            const auto held = Sampler::makeProgram ({ z })->prepared[0].fade;

            check (p.fade == 480 && worst < 1.0e-6f && juce::exactlyEqual (p.turn.getSample (0, 479), x[19999]) && longest == 5000 && held == 100,
                   "sampler: a 10 ms crossfade is 480 frames of the loop's end fading out, equal power, under the frames before its start fading in, ending on the frame before the start; "
                   "it is held to half the loop and to the audio there is before the start");
        }

        // Reversed, a sample is the sample backwards.
        {
            plain (sampler);
            auto z = zoneOf (noise (0.5, 5), 60);
            sampler.setZones ({ z });
            const auto forwards = play (sampler, { on (0.0, 60) }, 0.6);
            z.reverse = true;
            sampler.setZones ({ z });
            const auto backwards = play (sampler, { on (0.0, 60) }, 0.6);
            const auto frames = (int) (0.5 * rate);
            auto apart = 0.0f;

            for (int i = 200; i < frames - 200; ++i)
                apart = juce::jmax (apart, std::abs (backwards.getSample (0, i) - forwards.getSample (0, frames - 1 - i)));

            // And its loop is the same stretch of the sample, played the other way.
            auto tone = zoneOf (sine (220.0, 1.0), 57);
            tone.reverse = true;
            tone.loop = 1;
            tone.loopStart = 9600;
            tone.loopEnd = 12000;
            sampler.setZones ({ tone });
            const auto looped = play (sampler, { on (0.0, 57) }, 5.0);

            check (apart < 1.0e-4f && ClickCheck::worst (looped, rate, 0.01f).ratio < 4.0f && rms (looped, 4.0, 0.9) > 0.3f,
                   "sampler: half a second of noise reversed is the forward render backwards, within " + juce::String (db (apart), 1) + " dB; a reversed tone loops over the same eleven cycles without a step");
        }

        // One-shot plays to its end whatever the note's length; held, a note ends when let go. A loop can end with the note.
        {
            plain (sampler);
            set (sampler, "release", 30.0f);
            auto z = zoneOf (sine (220.0, 1.0), 57);
            sampler.setZones ({ z });
            const auto held = play (sampler, { on (0.0, 57), off (0.1, 57) }, 1.5);
            z.oneShot = true;
            z.loop = 1;            // a one-shot never loops
            z.loopStart = 9600;
            z.loopEnd = 12000;
            sampler.setZones ({ z });
            const auto shot = play (sampler, { on (0.0, 57), off (0.1, 57), { 0.2, juce::MidiMessage::allNotesOff (1) } }, 1.5);

            z.oneShot = false;
            z.loopInRelease = false;
            sampler.setZones ({ z });
            set (sampler, "release", 8000.0f);
            const auto leaves = play (sampler, { on (0.0, 57), off (2.0, 57) }, 4.0);
            z.loopInRelease = true;
            sampler.setZones ({ z });
            const auto stays = play (sampler, { on (0.0, 57), off (2.0, 57) }, 4.0);

            check (peak (held, 0.3, 1.0) == 0.0f && rms (shot, 0.8, 0.15) > 0.3f && peak (shot, 1.01, 0.4) == 0.0f
                       && rms (leaves, 2.3, 0.3) > 0.05f && peak (leaves, 2.85, 1.0) == 0.0f && ClickCheck::worst (leaves, rate, 0.01f).ratio < 4.0f
                       && rms (stays, 3.5, 0.4) > 0.01f,
                   "sampler: a held note ends when let go; a one-shot plays its whole second through note-off and All Notes Off, and doesn't loop; "
                   "with the loop set to end with the note, a note let go plays on from the loop to the sample's end, and otherwise the loop goes on under the release");
        }

        // A choke group: one zone cuts the others of its group off, and leaves the rest.
        {
            plain (sampler);
            auto open = zoneOf (sine (700.0, 2.0), 46);
            open.lowKey = open.highKey = 46;
            open.oneShot = true;
            open.choke = 1;
            auto closed = zoneOf (sine (1100.0, 0.2), 42);
            closed.lowKey = closed.highKey = 42;
            closed.oneShot = true;
            closed.choke = 1;
            auto other = zoneOf (sine (300.0, 2.0), 36);
            other.lowKey = other.highKey = 36;
            other.oneShot = true;
            sampler.setZones ({ open, closed, other });
            const auto out = play (sampler, { on (0.0, 46), on (0.0, 36), on (0.5, 42) }, 1.5);

            check (toneAt (out, 0.2, 0.25, 700.0) > 0.4 && toneAt (out, 0.51, 0.3, 700.0) < 0.002 && toneAt (out, 0.8, 0.5, 300.0) > 0.4
                       && ClickCheck::worst (out, rate, 0.01f, { 0, (int) (0.5 * rate) }).ratio < 4.0f,
                   "sampler: an open hat's zone is cut off by the closed hat's of the same group, fading in 4 ms without a click, while a zone outside the group plays on");
        }

        // A loop's points and crossfade changed while its note sounds.
        {
            plain (sampler);
            juce::AudioBuffer<float> rich (1, (int) rate);

            for (int i = 0; i < rich.getNumSamples(); ++i)
                rich.setSample (0, i, 0.3f * (float) (std::sin (twoPi * 110.0 * i / rate) + 0.5 * std::sin (twoPi * 220.0 * i / rate) + 0.3 * std::sin (twoPi * 330.0 * i / rate)));

            auto z = zoneOf (rich, 45);
            z.loop = 1;
            z.loopStart = 9600;
            z.loopEnd = 9600 + 4900;      // not a whole number of its cycles: only the crossfade makes the turn smooth
            z.crossfade = 5.0f;
            sampler.setZones ({ z });
            z = sampler.getZones()[0];
            sampler.prepareToPlay (rate, 256);
            juce::AudioBuffer<float> out (2, 560 * 256), block (2, 256);   // just under three seconds, in whole blocks

            for (int at = 0, step = 0; at + 256 <= out.getNumSamples(); at += 256, ++step)
            {
                juce::MidiBuffer midi;

                if (at == 0)
                    midi.addEvent (juce::MidiMessage::noteOn (1, 45, 1.0f), 0);

                if (step % 40 == 39)
                {
                    z.crossfade = 5.0f + (float) (step % 7) * 6.0f;
                    sampler.setZones ({ z });
                }

                sampler.processBlock (block, midi);
                out.copyFrom (0, at, block, 0, 0, 256);
                out.copyFrom (1, at, block, 1, 0, 256);
            }

            const auto click = ClickCheck::worst (out, rate, 0.01f);
            check (click.ratio < 4.0f && sampler.getNumSounding() == 1 && rms (out, 2.5, 0.4) > 0.1f,
                   "sampler: a loop's crossfade changed fourteen times under a held note leaves it sounding, the worst jump " + juce::String (click.ratio, 2) + " times its surroundings");
        }

        // The same twice and at any block size, with loops, a reversed zone, a one-shot and a choke group.
        {
            plain (sampler);
            set (sampler, "level", -12.0f);
            set (sampler, "release", 200.0f);
            set (sampler, "lfoPitch", 20.0f);
            auto a = zoneOf (sine (220.0, 1.0), 57);
            a.highKey = 59;
            a.loop = 1;
            a.loopStart = 9000;
            a.loopEnd = 12345;
            a.crossfade = 15.0f;
            auto b = zoneOf (noise (0.5, 9), 64);
            b.lowKey = 60;
            b.highKey = 71;
            b.reverse = true;
            b.loop = 2;
            b.loopStart = 4000;
            b.loopEnd = 9000;
            auto c = zoneOf (sine (500.0, 0.7), 76);
            c.lowKey = 72;
            c.highKey = 80;
            c.oneShot = true;
            c.choke = 2;
            auto d = zoneOf (sine (900.0, 0.3), 84);
            d.lowKey = 81;
            d.choke = 2;
            d.loopInRelease = false;
            d.loop = 1;
            d.loopStart = 2000;
            d.loopEnd = 6000;
            sampler.setZones ({ a, b, c, d });

            std::vector<Event> part;

            for (int i = 0; i < 30; ++i)
            {
                const auto key = 48 + (i * 11) % 40;
                part.push_back (on (0.07 * i, key, 0.5f + 0.01f * (float) i));
                part.push_back (off (0.07 * i + 0.3, key));
            }

            std::sort (part.begin(), part.end(), [] (const Event& x, const Event& y) { return x.when < y.when; });
            const auto first = play (sampler, part, 3.5, 512);
            auto alike = same (first, play (sampler, part, 3.5, 512));

            for (const auto size : { 1, 7, 64, 333, 4096 })
                alike = alike && same (first, play (sampler, part, 3.5, size));

            check (alike && first.getMagnitude (0, first.getNumSamples()) > 0.05f,
                   "sampler: a part over a crossfaded loop, a reversed zone looping forward and back, a one-shot and a choke group renders the same twice and in blocks of 1, 7, 64, 333, 512 and 4,096 samples, bit for bit");
        }

        // As many as 128 zones, and every zone setting through the state.
        {
            const auto folder = juce::File::createTempFile ("samplerzones");
            folder.createDirectory();
            const auto file = writeWav (folder.getChildFile ("Zone.wav"), sine (330.0, 0.5), rate);
            juce::AudioFormatManager formats;
            formats.registerBasicFormats();
            juce::String error;
            const auto sample = Sampler::loadSample (file, formats, error);
            std::vector<Sampler::Zone> many;

            for (int i = 0; i < 130; ++i)
            {
                Sampler::Zone z;
                z.sample = sample;
                z.file = file;
                z.root = z.lowKey = z.highKey = i % 128;
                z.reverse = i % 2 == 1;
                z.oneShot = i % 3 == 1;
                z.loop = i % 3;
                z.loopStart = 100 + i;
                z.loopEnd = 20000 - i;
                z.crossfade = (float) i * 0.5f;
                z.loopInRelease = i % 4 != 0;
                z.choke = i % 5;
                many.push_back (z);
            }

            sampler.setZones (many);
            const auto kept = sampler.getZones();
            juce::MemoryBlock state;
            sampler.getStateInformation (state);
            Processor other;
            other.setStateInformation (state.getData(), (int) state.getSize());
            const auto back = other.getZones();
            auto alike = kept.size() == back.size();

            for (size_t i = 0; alike && i < kept.size(); ++i)
                alike = back[i].sample != nullptr && back[i].id == kept[i].id && back[i].root == kept[i].root && back[i].reverse == kept[i].reverse && back[i].oneShot == kept[i].oneShot
                        && back[i].loop == kept[i].loop && back[i].loopStart == kept[i].loopStart && back[i].loopEnd == kept[i].loopEnd
                        && juce::exactlyEqual (back[i].crossfade, kept[i].crossfade) && back[i].loopInRelease == kept[i].loopInRelease && back[i].choke == kept[i].choke;

            check (error.isEmpty() && kept.size() == 128 && alike,
                   "sampler: of 130 zones given it keeps 128; each comes back from its state with its reverse, one-shot, loop, loop points, crossfade, release and choke settings");
            folder.deleteRecursively();
        }
    }
}

void runSamplerTests (AudioEngine& engine)
{
    juce::PluginDescription description;

    for (auto& d : BuiltInEffects::getDescriptions())
        if (d.fileOrIdentifier == BuiltInEffects::samplerId)
            description = d;

    juce::String error;
    auto instance = engine.pluginFormatManager.createPluginInstance (description, rate, 512, error);
    auto* sampler = dynamic_cast<Processor*> (instance.get());

    check (sampler != nullptr && error.isEmpty() && description.isInstrument && description.numInputChannels == 0 && description.name == "Torqwave Sampler"
               && sampler->acceptsMidi() && sampler->getTotalNumInputChannels() == 0 && sampler->getTotalNumOutputChannels() == 2
               && sampler->getParameters().size() == 31,
           "sampler: made from its description, an instrument with no audio input, a stereo output and 31 parameters");

    if (sampler == nullptr)
        return;

    testInterpolation();

    // With no zones, and before any are set, it is silent.
    {
        plain (*sampler);
        sampler->setZones ({});
        const auto out = play (*sampler, { on (0.0, 60), off (0.5, 60) }, 1.0);
        check (out.getMagnitude (0, out.getNumSamples()) == 0.0f && sampler->getNumSounding() == 0, "sampler: with no sample it is silent");
    }

    // At its root a sample plays as it is; an octave up it is twice the pitch and half as long.
    {
        plain (*sampler);
        const auto tone = sine (440.0, 1.0);
        oneZone (*sampler, tone, 69);
        const auto atRoot = play (*sampler, { on (0.0, 69) }, 1.5);
        auto apart = 0.0f;

        for (int i = (int) (0.01 * rate); i < (int) (0.99 * rate); ++i)
            apart = juce::jmax (apart, std::abs (atRoot.getSample (0, i) - tone.getSample (0, i)));

        const auto up = play (*sampler, { on (0.0, 81) }, 1.5);
        const auto down = play (*sampler, { on (0.0, 57) }, 2.5);
        const auto upCents = centsOff (pitchOf (up, (int) (0.1 * rate), (int) (0.3 * rate), 880.0), 880.0);
        const auto downCents = centsOff (pitchOf (down, (int) (0.1 * rate), (int) (0.3 * rate), 220.0), 220.0);
        auto bothSides = true;

        for (int i = 0; i < atRoot.getNumSamples(); ++i)
            bothSides = bothSides && juce::exactlyEqual (atRoot.getSample (0, i), atRoot.getSample (1, i));

        check (apart < 1.0e-4f && peak (atRoot, 1.001, 0.4) == 0.0f && bothSides
                   && std::abs (upCents) < 0.5 && peak (up, 0.45, 0.04) > 0.4f && peak (up, 0.501, 0.9) == 0.0f
                   && std::abs (downCents) < 0.5 && peak (down, 1.9, 0.09) > 0.4f && peak (down, 2.001, 0.4) == 0.0f,
               "sampler: a 440 Hz sample played at its root is the sample to within " + juce::String (db (apart), 1) + " dB, the same left and right, and ends where it does; an octave up it is "
                   + juce::String (upCents, 2) + " cents from 880 Hz and half a second long, an octave down " + juce::String (downCents, 2) + " cents from 220 Hz and two seconds long");
    }

    // A sample at another rate plays at its own pitch, and a stereo one keeps its sides.
    {
        plain (*sampler);
        oneZone (*sampler, sine (440.0, 1.0, 44100.0), 69, 44100.0);
        const auto other = play (*sampler, { on (0.0, 69) }, 1.5);
        const auto cents = centsOff (pitchOf (other, (int) (0.1 * rate), (int) (0.3 * rate), 440.0), 440.0);

        juce::AudioBuffer<float> two (2, (int) rate);
        two.copyFrom (0, 0, sine (300.0, 1.0), 0, 0, (int) rate);
        two.clear (1, 0, (int) rate);
        oneZone (*sampler, two, 60);
        const auto sides = play (*sampler, { on (0.0, 60) }, 1.0);

        check (std::abs (cents) < 0.5 && peak (other, 0.9, 0.09) > 0.4f && peak (other, 1.001, 0.4) == 0.0f
                   && peak (sides, 0.1, 0.5, 0) > 0.4f && peak (sides, 0.0, 1.0, 1) == 0.0f,
               "sampler: a 44.1 kHz sample in a 48 kHz session plays " + juce::String (cents, 2) + " cents from its pitch and for its own second; a stereo sample's left stays on the left");
    }

    // The amp envelope, read from a steady sample: the rise, the fall to the sustain level, and the release.
    {
        plain (*sampler);
        oneZone (*sampler, steady (4.0), 60);
        set (*sampler, "attack", 100.0f);
        set (*sampler, "decay", 400.0f);
        set (*sampler, "sustain", 50.0f);
        set (*sampler, "release", 200.0f);
        const auto out = play (*sampler, { on (0.0, 60), off (2.0, 60) }, 3.0);
        const auto at = [&] (double seconds)     { return std::abs (out.getSample (0, (int) (seconds * rate))); };
        const auto half = at (0.05), top = at (0.1001), settled = at (1.5);
        const auto fallen = db ((at (0.1 + 0.4) - 0.25) / 0.25);     // how far above the sustain level it still is, the decay time on
        const auto released = db (at (2.2) / at (2.0));

        set (*sampler, "attack", 0.0f);
        const auto fast = play (*sampler, { on (0.0, 60) }, 0.1);
        auto reached = 0;

        while (reached < fast.getNumSamples() && std::abs (fast.getSample (0, reached)) < 0.499f)
            ++reached;

        check (std::abs (half - 0.25f) < 0.005f && std::abs (top - 0.5f) < 0.005f && std::abs (settled - 0.25f) < 0.001f
                   && std::abs (fallen + 60.0) < 1.0 && std::abs (released + 60.0) < 1.0 && peak (out, 2.35, 0.6) == 0.0f
                   && reached >= 10 && reached <= 13 && ClickCheck::worst (fast, rate, 0.02f).ratio < 4.0f,
               "sampler: with a 100 ms attack a steady sample is half way up at 50 ms and full at 100 ms; it has fallen " + juce::String (-fallen, 1)
                   + " dB of the way to a 50 % sustain 400 ms later and holds there; let go, it is " + juce::String (-released, 1)
                   + " dB down after its 200 ms release and then silent; with no attack it rises over " + juce::String (reached) + " samples, a quarter of a millisecond");
    }

    // Velocity: how much it sets the level.
    {
        plain (*sampler);
        oneZone (*sampler, steady (1.0), 60);
        const auto none = peak (play (*sampler, { on (0.0, 60, 0.5f) }, 0.5), 0.2, 0.1);
        set (*sampler, "velocity", 100.0f);
        const auto full = peak (play (*sampler, { on (0.0, 60, 0.5f) }, 0.5), 0.2, 0.1);
        const auto hard = peak (play (*sampler, { on (0.0, 60, 1.0f) }, 0.5), 0.2, 0.1);

        check (std::abs (none - 0.5f) < 0.001f && std::abs (hard - 0.5f) < 0.001f && std::abs (db (full / hard) + 12.0) < 0.3,
               "sampler: with Velocity at 0 % a soft note is as loud as a hard one; at 100 % a note at half velocity is " + juce::String (-db (full / hard), 1) + " dB under one at full");
    }

    // The filter and its envelope, on noise.
    {
        plain (*sampler);
        oneZone (*sampler, noise (3.0), 60);
        const auto open = highsDb (play (*sampler, { on (0.0, 60) }, 2.0), 0.2, 1.5, 4000.0);
        set (*sampler, "filter", 1.0f);
        set (*sampler, "cutoff", 1000.0f);
        const auto low12 = highsDb (play (*sampler, { on (0.0, 60) }, 2.0), 0.2, 1.5, 4000.0);
        set (*sampler, "filter", 2.0f);
        const auto low24 = highsDb (play (*sampler, { on (0.0, 60) }, 2.0), 0.2, 1.5, 4000.0);
        set (*sampler, "filter", 4.0f);
        set (*sampler, "cutoff", 4000.0f);
        const auto high = highsDb (play (*sampler, { on (0.0, 60) }, 2.0), 0.2, 1.5, 4000.0);

        // An envelope that opens the filter four octaves and lets it close again.
        set (*sampler, "filter", 2.0f);
        set (*sampler, "cutoff", 500.0f);
        set (*sampler, "filterEnv", 50.0f);
        set (*sampler, "filterDecay", 300.0f);
        const auto swept = play (*sampler, { on (0.0, 60) }, 2.0);
        const auto early = highsDb (swept, 0.0, 0.1, 2000.0), late = highsDb (swept, 1.0, 0.5, 2000.0);

        // Key tracking: at 100 % the cutoff is an octave higher an octave up.
        set (*sampler, "filterEnv", 0.0f);
        const auto untracked = highsDb (play (*sampler, { on (0.0, 72) }, 2.0), 0.2, 1.5, 1000.0);
        set (*sampler, "keyTrack", 100.0f);
        const auto tracked = highsDb (play (*sampler, { on (0.0, 72) }, 2.0), 0.2, 1.5, 1000.0);

        check (open > -2.0 && low12 < open - 15.0 && low24 < low12 - 10.0 && high > -1.0 && early > late + 15.0 && tracked > untracked + 6.0
                   && ClickCheck::worst (swept, rate, 0.05f).ratio < 4.0f,
               "sampler: noise through the low-pass at 1 kHz has " + juce::String (open - low12, 1) + " dB less above 4 kHz at 12 dB an octave and "
                   + juce::String (open - low24, 1) + " dB less at 24; the high-pass at 4 kHz keeps what is above it; the filter's envelope leaves "
                   + juce::String (early - late, 1) + " dB more above 2 kHz at the start of a note than a second in; key tracking opens it "
                   + juce::String (tracked - untracked, 1) + " dB further above 1 kHz an octave up");
    }

    // The LFO on the level: its rate in Hz, and in note values at the tempo.
    {
        Sampler::Engine bare;
        bare.prepare (rate);
        Sampler::Zone z;
        z.sample = Sampler::makeSample (steady (5.0), rate, "steady");
        z.id = 1;
        const auto program = Sampler::makeProgram ({ z });
        bare.setProgram (program);

        auto dips = [&] (Sampler::Settings s, double bpm)
        {
            bare.reset();
            bare.setProgram (program);
            bare.setSettings (s);
            bare.setTempo (bpm);
            bare.noteOn (60, 1.0f);
            std::vector<float> left ((size_t) (4.0 * rate), 0.0f), right (left.size(), 0.0f);
            bare.render (left.data(), right.data(), (int) left.size());
            int count = 0;
            const auto middle = 0.5f * (*std::max_element (left.begin() + 100, left.end()) + *std::min_element (left.begin() + 100, left.end()));

            for (size_t i = 101; i < left.size(); ++i)
                count += left[i - 1] >= middle && left[i] < middle ? 1 : 0;

            return count;
        };

        Sampler::Settings s;
        s.velocity = 0.0f;
        s.lfoLevel = 100.0f;
        s.lfoRate = 5.0f;
        const auto free = dips (s, 120.0);
        s.lfoSync = 4;   // 1/4
        const auto at120 = dips (s, 120.0), at90 = dips (s, 90.0);
        s.lfoSync = 7;   // 1/16
        const auto sixteenths = dips (s, 120.0);

        check (free == 20 && at120 == 8 && at90 == 6 && sixteenths == 32,
               "sampler: the LFO at 5 Hz dips the level " + juce::String (free) + " times in four seconds; at 1/4 it dips " + juce::String (at120)
                   + " times at 120 BPM and " + juce::String (at90) + " at 90, and at 1/16 " + juce::String (sixteenths) + " times at 120");
    }

    // Vibrato from the LFO and from the mod wheel, and the LFO's fade in.
    {
        plain (*sampler);
        oneZone (*sampler, sine (440.0, 3.0), 69);

        auto wander = [&] (const juce::AudioBuffer<float>& out, double from)
        {
            double low = 1.0e9, high = 0.0;

            for (auto t = from; t < from + 0.4; t += 0.01)
            {
                const auto hz = pitchOf (out, (int) (t * rate), (int) (0.02 * rate), 440.0);
                low = juce::jmin (low, hz);
                high = juce::jmax (high, hz);
            }

            return centsOff (high, low);
        };

        const auto still = wander (play (*sampler, { on (0.0, 69) }, 2.0), 1.0);
        const auto wheeled = wander (play (*sampler, { cc (0.0, 1, 127), on (0.0, 69) }, 2.0), 1.0);
        set (*sampler, "lfoPitch", 50.0f);
        set (*sampler, "lfoFade", 1000.0f);
        const auto faded = play (*sampler, { on (0.0, 69) }, 2.0);
        const auto early = wander (faded, 0.02), later = wander (faded, 1.2);

        // (The pitch is read over 20 ms at a time, which is good to a few cents.)
        check (still < 5.0 && wheeled > 70.0 && wheeled < 110.0 && later > 70.0 && later < 110.0 && early < 0.5 * later,
               "sampler: a held note is steady (within " + juce::String (still, 1) + " cents as measured); the mod wheel at its top adds " + juce::String (juce::roundToInt (wheeled))
                   + " cents of vibrato from end to end, as Wheel Vibrato's 50 either way; the LFO at 50 cents gives " + juce::String (juce::roundToInt (later))
                   + " once its one-second fade is over and " + juce::String (juce::roundToInt (early)) + " at the start");
    }

    // The controllers: bend, volume, expression, pan and the sustain pedal.
    {
        plain (*sampler);
        oneZone (*sampler, sine (440.0, 3.0), 69);
        const auto bent = play (*sampler, { bend (0.0, 16383), on (0.0, 69) }, 1.0);
        const auto bentCents = centsOff (pitchOf (bent, (int) (0.3 * rate), (int) (0.3 * rate), 493.88), 440.0);
        set (*sampler, "bendRange", 12.0f);
        const auto far = play (*sampler, { bend (0.0, 0), on (0.0, 69) }, 1.0);
        const auto farCents = centsOff (pitchOf (far, (int) (0.3 * rate), (int) (0.3 * rate), 220.0), 440.0);
        set (*sampler, "bendRange", 2.0f);

        const auto full = rms (play (*sampler, { on (0.0, 69) }, 1.0), 0.3, 0.5);
        const auto quiet = rms (play (*sampler, { cc (0.0, 7, 64), on (0.0, 69) }, 1.0), 0.3, 0.5);
        const auto soft = rms (play (*sampler, { cc (0.0, 11, 32), on (0.0, 69) }, 1.0), 0.3, 0.5);
        const auto left = play (*sampler, { cc (0.0, 10, 0), on (0.0, 69) }, 1.0);
        const auto wanted7 = db ((64.0 / 127.0) * (64.0 / 127.0)), wanted11 = db ((32.0 / 127.0) * (32.0 / 127.0));

        // The pedal holds a note let go, until it comes up.
        set (*sampler, "release", 50.0f);
        const auto pedalled = play (*sampler, { cc (0.0, 64, 127), on (0.0, 69), off (0.5, 69), cc (1.5, 64, 0) }, 2.5);
        const auto unpedalled = play (*sampler, { on (0.0, 69), off (0.5, 69) }, 2.5);

        // A volume change in the middle of a note is reached smoothly.
        const auto stepped = play (*sampler, { on (0.0, 69), cc (0.5, 7, 20), cc (0.8, 7, 127) }, 1.2);

        check (std::abs (bentCents - 200.0) < 1.0 && std::abs (farCents + 1200.0) < 1.0
                   && std::abs (db (quiet / full) - wanted7) < 0.1 && std::abs (db (soft / full) - wanted11) < 0.1
                   && rms (left, 0.3, 0.5, 1) < 1.0e-6f && std::abs (db (rms (left, 0.3, 0.5, 0) / full) - 3.01) < 0.05
                   && rms (pedalled, 1.0, 0.4) > 0.3f && rms (pedalled, 1.7, 0.5) == 0.0f && rms (unpedalled, 0.7, 0.5) == 0.0f
                   && ClickCheck::worst (stepped, rate, 0.01f).ratio < 4.0f,
               "sampler: the bend wheel at its top raises a note " + juce::String (bentCents, 1) + " cents with a range of 2 semitones, and at its bottom lowers it "
                   + juce::String (-farCents, 1) + " with a range of 12; volume at 64 is " + juce::String (db (quiet / full), 1) + " dB and expression at 32 "
                   + juce::String (db (soft / full), 1) + " dB; pan hard left leaves the right silent; the pedal holds a note let go until it comes up; a volume change mid-note makes no click");
    }

    // Glide slides each note from the last, and Mono plays one at a time.
    {
        plain (*sampler);
        oneZone (*sampler, sine (440.0, 4.0), 69);
        set (*sampler, "glide", 300.0f);
        const auto slid = play (*sampler, { on (0.0, 57), off (0.5, 57), on (0.5, 69) }, 2.0);
        const auto soon = centsOff (pitchOf (slid, (int) (0.52 * rate), (int) (0.02 * rate), 300.0), 440.0);
        const auto there = centsOff (pitchOf (slid, (int) (1.5 * rate), (int) (0.2 * rate), 440.0), 440.0);

        set (*sampler, "glide", 0.0f);
        set (*sampler, "voices", 1.0f);
        const auto mono = play (*sampler, { on (0.0, 57), on (0.5, 69) }, 1.0);
        const auto soundingMono = sampler->getNumSounding();
        set (*sampler, "voices", 0.0f);
        play (*sampler, { on (0.0, 57), on (0.5, 69) }, 1.0);
        const auto soundingPoly = sampler->getNumSounding();

        check (soon < -700.0 && soon > -1200.0 && std::abs (there) < 2.0 && soundingMono == 1 && soundingPoly == 2
                   && ClickCheck::worst (mono, rate, 0.02f, { (int) (0.5 * rate) }).ratio < 4.0f,
               "sampler: with a 300 ms glide a note an octave above the last starts " + juce::String (juce::roundToInt (-soon)) + " cents under its pitch and is within "
                   + juce::String (std::abs (there), 1) + " cents a second later; Mono leaves one note sounding where Poly leaves two, the first fading under the second without a click");
    }

    // More notes than voices: the oldest gives way without a click, and All Sound Off fades in 4 ms.
    {
        plain (*sampler);
        set (*sampler, "level", -24.0f);
        oneZone (*sampler, sine (220.0, 6.0), 57);
        std::vector<Event> many;
        std::vector<int> starts;

        for (int i = 0; i < 40; ++i)
        {
            many.push_back (on (0.05 * i, 40 + i));
            starts.push_back ((int) (0.05 * i * rate));
        }

        const auto crowded = play (*sampler, many, 2.5);
        const auto sounding = sampler->getNumSounding();
        const auto crowdedClick = ClickCheck::worst (crowded, rate, 0.005f, starts);

        many.push_back ({ 2.6, juce::MidiMessage::allSoundOff (1) });
        const auto stopped = play (*sampler, many, 3.0);
        const auto stopClick = ClickCheck::worst (stopped, rate, 0.005f, starts);
        auto silentFrom = stopped.getNumSamples();

        while (silentFrom > 0 && std::abs (stopped.getSample (0, silentFrom - 1)) == 0.0f && std::abs (stopped.getSample (1, silentFrom - 1)) == 0.0f)
            --silentFrom;

        const auto fadeMs = 1000.0 * (silentFrom - (int) (2.6 * rate)) / rate;

        check (sounding == Sampler::numVoices && crowdedClick.ratio < 4.0f && stopClick.ratio < 4.0f && fadeMs > 3.0 && fadeMs <= 4.1 && sampler->getNumSounding() == 0,
               "sampler: forty notes held leave " + juce::String (sounding) + " sounding, the oldest having given way (worst jump " + juce::String (crowdedClick.ratio, 2)
                   + " times its surroundings); All Sound Off is silent " + juce::String (fadeMs, 2) + " ms later (worst jump " + juce::String (stopClick.ratio, 2) + ")");
    }

    // A key struck again while it is down doesn't pile up, and a sample cut short at its end doesn't click.
    {
        plain (*sampler);
        oneZone (*sampler, sine (220.0, 6.0), 57);
        std::vector<Event> again;

        for (int i = 0; i < 20; ++i)
            again.push_back (on (0.1 * i, 57));

        play (*sampler, again, 2.2);
        const auto piled = sampler->getNumSounding();

        // A sample that ends at full level: its end is faded.
        Sampler::Zone z;
        z.sample = Sampler::makeSample (sine (220.0, 1.0), rate, "cut");
        z.root = 57;
        z.end = (juce::int64) (0.5 * rate) + 54;    // a quarter of a cycle in: the top of the wave
        sampler->setZones ({ z });
        const auto cut = play (*sampler, { on (0.0, 57) }, 1.0);
        const auto cutClick = ClickCheck::worst (cut, rate, 0.01f);
        z.start = 54;
        sampler->setZones ({ z });
        const auto begun = play (*sampler, { on (0.0, 57) }, 1.0);
        const auto beginClick = ClickCheck::worst (begun, rate, 0.01f);

        check (piled == 1 && cutClick.ratio < 4.0f && beginClick.ratio < 4.0f && peak (cut, 0.52, 0.4) == 0.0f,
               "sampler: a key struck twenty times while down leaves one note sounding; a sample that ends, or starts, at the top of its wave is faded (worst jumps "
                   + juce::String (cutClick.ratio, 2) + " and " + juce::String (beginClick.ratio, 2) + " times their surroundings)");
    }

    // The same twice and at any block size: a busy part with the filter, its envelope, the LFO, bends and the pedal.
    {
        plain (*sampler);
        set (*sampler, "level", -12.0f);
        set (*sampler, "velocity", 80.0f);
        juce::AudioBuffer<float> two (2, (int) (2.0 * rate));
        two.copyFrom (0, 0, sine (196.0, 2.0), 0, 0, two.getNumSamples());
        two.copyFrom (1, 0, noise (2.0, 11), 0, 0, two.getNumSamples());
        oneZone (*sampler, two, 55);
        set (*sampler, "filter", 2.0f);
        set (*sampler, "cutoff", 900.0f);
        set (*sampler, "resonance", 60.0f);
        set (*sampler, "filterEnv", 40.0f);
        set (*sampler, "filterDecay", 250.0f);
        set (*sampler, "keyTrack", 50.0f);
        set (*sampler, "lfoShape", 4.0f);
        set (*sampler, "lfoRate", 7.3f);
        set (*sampler, "lfoPitch", 30.0f);
        set (*sampler, "lfoFilter", 30.0f);
        set (*sampler, "lfoLevel", 40.0f);
        set (*sampler, "lfoPan", 60.0f);
        set (*sampler, "glide", 80.0f);
        set (*sampler, "attack", 3.0f);
        set (*sampler, "release", 300.0f);

        std::vector<Event> part;

        for (int i = 0; i < 24; ++i)
        {
            const auto when = 0.0817 * i;
            part.push_back (on (when, 43 + (i * 7) % 29, 0.4f + 0.025f * (float) i));
            part.push_back (off (when + 0.21, 43 + (i * 7) % 29));
            part.push_back (bend (when + 0.03, 8192 + (i % 5 - 2) * 1500));
        }

        part.push_back (cc (0.4, 64, 127));
        part.push_back (cc (1.1, 64, 0));
        part.push_back (cc (0.7, 1, 90));
        part.push_back (cc (0.9, 7, 100));
        part.push_back (cc (1.3, 10, 30));
        std::sort (part.begin(), part.end(), [] (const Event& a, const Event& b) { return a.when < b.when; });

        const auto first = play (*sampler, part, 3.0, 512);
        auto alike = same (first, play (*sampler, part, 3.0, 512));
        juce::StringArray sizes;

        for (const auto size : { 1, 7, 64, 333, 4096 })
        {
            alike = alike && same (first, play (*sampler, part, 3.0, size));
            sizes.add (juce::String (size));
        }

        check (alike && first.getMagnitude (0, first.getNumSamples()) > 0.05f && first.getMagnitude (0, first.getNumSamples()) < 1.0f,
               "sampler: a busy part (the filter and its envelope, a random LFO on pitch, filter, level and pan, glide, bends, the pedal, the wheel, volume and pan) "
               "renders the same twice and in blocks of " + sizes.joinIntoString (", ") + " and 512 samples, bit for bit");
    }

    // Its state: the settings and the zones, each sample named by its file, come back exactly; a file that has gone is reported.
    {
        const auto folder = juce::File::createTempFile ("samplerstate");
        folder.createDirectory();
        const auto file = writeWav (folder.getChildFile ("Tone.wav"), sine (330.0, 1.0, 44100.0, 2), 44100.0);

        defaults (*sampler);
        juce::String loadError;
        Sampler::Zone a;
        a.sample = Sampler::loadSample (file, engine.formatManager, loadError);
        a.file = file;
        a.root = 64;
        a.tune = 12.5f;
        a.lowKey = 40;
        a.highKey = 70;
        a.lowVelocity = 20;
        a.highVelocity = 110;
        a.start = 100;
        a.end = 30000;
        a.level = -3.5f;
        a.pan = 25.0f;
        auto b = a;
        b.root = 76;
        b.lowKey = 71;
        b.highKey = 127;
        b.start = 0;
        b.end = -1;
        sampler->setZones ({ a, b });
        set (*sampler, "cutoff", 1234.5f);
        set (*sampler, "filter", 3.0f);
        set (*sampler, "lfoSync", 5.0f);
        set (*sampler, "release", 777.0f);

        juce::MemoryBlock state;
        sampler->getStateInformation (state);
        const auto before = play (*sampler, { on (0.0, 60, 0.7f), on (0.2, 80, 0.7f), off (0.6, 60) }, 1.0);

        Processor other;
        other.setStateInformation (state.getData(), (int) state.getSize());
        const auto zones = other.getZones();
        const auto after = play (other, { on (0.0, 60, 0.7f), on (0.2, 80, 0.7f), off (0.6, 60) }, 1.0);
        juce::MemoryBlock again;
        other.getStateInformation (again);

        const auto kept = zones.size() == 2 && zones[0].sample != nullptr && zones[0].sample == zones[1].sample && zones[0].file == file
                          && zones[0].root == 64 && juce::exactlyEqual (zones[0].tune, 12.5f) && zones[0].lowKey == 40 && zones[0].highKey == 70
                          && zones[0].lowVelocity == 20 && zones[0].highVelocity == 110 && zones[0].start == 100 && zones[0].end == 30000
                          && juce::exactlyEqual (zones[0].level, -3.5f) && juce::exactlyEqual (zones[0].pan, 25.0f)
                          && zones[1].root == 76 && zones[1].lowKey == 71 && zones[1].end == -1 && zones[0].id != zones[1].id && zones[0].id != 0;

        check (loadError.isEmpty() && kept && other.getPresetValues() == sampler->getPresetValues() && again == state && same (before, after)
                   && before.getMagnitude (0, before.getNumSamples()) > 0.05f && other.getMissing().isEmpty(),
               "sampler: its state brings back every setting and both zones (root, tuning, keys, velocities, start, end, level and pan), their sample read once from its file, and plays the same, bit for bit");

        file.deleteFile();
        Processor lost;
        lost.setStateInformation (state.getData(), (int) state.getSize());
        const auto lostZones = lost.getZones();
        const auto silent = play (lost, { on (0.0, 60) }, 0.5);
        juce::MemoryBlock lostState;
        lost.getStateInformation (lostState);

        check (lostZones.size() == 2 && lostZones[0].sample == nullptr && lostZones[0].file == file && lost.getMissing().size() == 1
                   && lost.getMissing()[0].contains ("Tone.wav") && silent.getMagnitude (0, silent.getNumSamples()) == 0.0f && lostState == state,
               "sampler: with its file gone, a zone is kept with its settings and its file's name, plays nothing, is reported once (" + lost.getMissing().joinIntoString ("; ")
                   + ") and is saved again as it was");

        folder.deleteRecursively();
    }

    // Zones swapped while a note sounds: the note goes on when its zone is still there, and fades when it isn't.
    {
        plain (*sampler);
        oneZone (*sampler, sine (220.0, 4.0), 57);
        sampler->prepareToPlay (rate, 512);
        juce::AudioBuffer<float> out (2, (int) (1.5 * rate)), block (2, 512);
        int at = 0;

        auto run = [&] (int until, const juce::MidiMessage* message = nullptr)
        {
            while (at < until)
            {
                const auto n = juce::jmin (512, until - at);
                juce::MidiBuffer midi;

                if (message != nullptr)
                {
                    midi.addEvent (*message, 0);
                    message = nullptr;
                }

                block.setSize (2, n, false, false, true);
                sampler->processBlock (block, midi);
                out.copyFrom (0, at, block, 0, 0, n);
                out.copyFrom (1, at, block, 1, 0, n);
                at += n;
            }
        };

        const auto note = juce::MidiMessage::noteOn (1, 57, 1.0f);
        run ((int) (0.5 * rate), &note);
        auto zones = sampler->getZones();
        zones[0].level = -6.0f;
        sampler->setZones (zones);
        run ((int) (1.0 * rate));
        const auto goesOn = sampler->getNumSounding() == 1;
        const auto quieter = db (rms (out, 0.7, 0.2) / rms (out, 0.2, 0.2));
        sampler->setZones ({});
        run ((int) (1.5 * rate));

        check (goesOn && std::abs (quieter + 6.0) < 0.1 && sampler->getNumSounding() == 0 && rms (out, 1.1, 0.3) == 0.0f
                   && ClickCheck::worst (out, rate, 0.01f).ratio < 4.0f,
               "sampler: a zone's level changed while its note sounds is taken up by the note (" + juce::String (quieter, 1)
                   + " dB) without a click; the zones taken away, the note fades out");
    }

    testZones (*sampler);

    defaults (*sampler);
    sampler->setZones ({});
}
