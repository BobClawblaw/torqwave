#include <JuceHeader.h>
#include "../Source/AudioEngine.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/Sampler.h"
#include "../Source/SamplerEditor.h"
#include "../Source/SamplerTrack.h"
#include "../Source/AiLibrary.h"
#include "../Source/EffectPresets.h"
#include "SamplerContent.h"
#include "../Source/SoundDrops.h"
#include "../Source/Theme.h"
#include "../Source/InstrumentKeys.h"
#include "../Source/DrumMachine.h"
#include "../Source/Session.h"
#include "../Source/SessionCollect.h"
#include "../Source/WaveSynth.h"
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
                   "sampler: a key below C4 plays the low zone alone; above it a soft note plays the soft zone and a hard one the hard zone (velocity 79 the one, 80 the other), each with the layer over them, "
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

        // The same away from the sample's own pitch, where every frame is read between the sample's: loops forward,
        // crossfaded, forward and back and reversed, a fifth up and a fourth down.
        {
            plain (sampler);
            auto z = zoneOf (sine (220.0, 1.0), 57);
            z.loop = 1;
            z.loopStart = 9600;
            z.loopEnd = 12000;
            auto worst = 0.0f;
            auto offPitch = 0.0;
            auto sounding = true;

            for (int kind = 0; kind < 4; ++kind)
            {
                auto zone = z;
                zone.loopEnd = kind == 1 ? 12100 : 12000;
                zone.crossfade = kind == 1 ? 20.0f : 0.0f;
                zone.loop = kind == 2 ? 2 : 1;
                zone.reverse = kind == 3;
                sampler.setZones ({ zone });

                for (const auto key : { 64, 52 })
                {
                    const auto out = play (sampler, { on (0.0, key) }, 5.0);
                    const auto hz = 220.0 * std::pow (2.0, (key - 57) / 12.0);
                    worst = juce::jmax (worst, ClickCheck::worst (out, rate, 0.01f).ratio);
                    sounding = sounding && rms (out, 4.0, 0.9) > 0.2f;

                    if (kind == 0)
                        offPitch = juce::jmax (offPitch, std::abs (centsOff (pitchOf (out, (int) (3.0 * rate), (int) (0.5 * rate), hz), hz)));
                }
            }

            check (worst < 4.0f && sounding && offPitch < 0.5,
                   "sampler: played a fifth above and a fourth below its pitch, a loop goes round without a step forward, crossfaded, forward and back and reversed (the worst jump "
                       + juce::String (worst, 2) + " times its surroundings), still sounding after five seconds and within " + juce::String (offPitch, 2) + " cents");
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

    /** Two bars of drums at 120 BPM from Basic Drums' engine, with the times its hits start at. */
    juce::AudioBuffer<float> drumLoop (std::vector<double>& times)
    {
        DrumMachine::Engine drums;
        drums.prepare (rate);
        const auto step = 0.125;
        juce::AudioBuffer<float> out (2, (int) (4.0 * rate));
        out.clear();
        int at = 0;

        for (int i = 0; i < 32; ++i)
        {
            const auto frame = (int) std::lround (i * step * rate);
            drums.render (out.getWritePointer (0, at), out.getWritePointer (1, at), frame - at);
            at = frame;
            const auto inBar = i % 16;
            auto any = false;

            if (inBar % 4 == 0)                     { drums.noteOn (36, 1.0f); any = true; }
            if (inBar == 4 || inBar == 12)          { drums.noteOn (38, 0.9f); any = true; }
            if (inBar % 2 == 0)                     { drums.noteOn (42, 0.7f); any = true; }
            if (inBar == 7 || inBar == 15)          { drums.noteOn (36, 0.6f); any = true; }

            if (any)
                times.push_back (i * step);
        }

        drums.render (out.getWritePointer (0, at), out.getWritePointer (1, at), out.getNumSamples() - at);
        return out;
    }

    /** Slices: a sample cut at its markers, its regions, its hits or into equal parts, each piece on a key. */
    void testSlices (Processor& sampler)
    {
        // A tone that changes pitch every fifth of a second, with a marker at each change but the first.
        juce::AudioBuffer<float> steps (1, (int) (1.8 * rate));
        Markers markers;

        for (int k = 0; k < 9; ++k)
        {
            for (int i = 0; i < 9600; ++i)
                steps.setSample (0, k * 9600 + i, 0.5f * (float) std::sin (twoPi * (300.0 + 100.0 * k) * i / rate));

            if (k > 0)
                markers.push_back ({ (juce::int64) k * 9600, 0, k % 2 == 0 ? juce::String ("Hit ") + juce::String (k) : juce::String(), -1 });
        }

        markers.push_back ({ 4800, 9600, "Across", -1 });
        markers.push_back ({ 40000, 20000, "Late", -1 });
        markers.push_back ({ 70000, 5000, {}, -1 });
        MarkerEdits::sort (markers);
        const auto sample = Sampler::makeSample (steps, rate, "Steps", markers);

        {
            Sampler::Slicing how;
            how.source = Sampler::SliceSource::markers;
            const auto slices = Sampler::findSlices (*sample, {}, how);
            auto cutRight = slices.size() == 9;
            juce::StringArray names;

            for (size_t i = 0; cutRight && i < slices.size(); ++i)
            {
                cutRight = slices[i].start == (juce::int64) i * 9600 && slices[i].end == (juce::int64) (i + 1) * 9600;
                names.add (slices[i].name);
            }

            plain (sampler);
            const auto sliced = sampler.slice (sample, {}, {}, how);
            const auto zones = sampler.getZones();
            auto played = sliced && zones.size() == 9 && sampler.getSlicing().on;

            for (int k = 0; played && k < 9; ++k)
            {
                const auto out = play (sampler, { on (0.0, 36 + k), off (0.05, 36 + k) }, 0.4);
                played = zones[(size_t) k].lowKey == 36 + k && zones[(size_t) k].highKey == 36 + k && zones[(size_t) k].oneShot
                         && toneAt (out, 0.02, 0.15, 300.0 + 100.0 * k) > 0.45 && peak (out, 0.21, 0.15) == 0.0f;
            }

            const auto above = play (sampler, { on (0.0, 45) }, 0.3);
            how.source = Sampler::SliceSource::regions;
            const auto regions = Sampler::findSlices (*sample, {}, how);
            const auto inPart = Sampler::findSlices (*sample, { 30000, 80000 }, how);
            how.source = Sampler::SliceSource::markers;
            const auto partOnly = Sampler::findSlices (*sample, { 20000, 50000 }, how);

            check (cutRight && names.joinIntoString ("|") == "Start|Slice 2|Hit 2|Slice 4|Hit 4|Slice 6|Hit 6|Slice 8|Hit 8" && played && above.getMagnitude (0, above.getNumSamples()) == 0.0f
                       && regions.size() == 3 && regions[0].name == "Across" && regions[0].start == 4800 && regions[0].end == 14400 && regions[2].name == "Slice 3"
                       && inPart.size() == 2 && inPart[0].name == "Late"
                       && partOnly.size() == 4 && partOnly[0].start == 20000 && partOnly[0].name == "Start" && partOnly[3].end == 50000,
                   "sampler: a sample with eight markers is cut into the nine pieces between them, named after the markers (" + names.joinIntoString (", ")
                       + "), each a one-shot on its own key from C2 playing its piece and nothing after it; its three regions give three slices; a part of it is cut at the markers and regions in that part alone");
        }

        // Equal parts, and no more than 64 of anything.
        {
            Sampler::Slicing how;
            how.source = Sampler::SliceSource::equalParts;
            how.parts = 16;
            const auto sixteen = Sampler::findSlices (*sample, {}, how);
            auto equal = sixteen.size() == 16;

            for (size_t i = 0; equal && i < sixteen.size(); ++i)
                equal = sixteen[i].end - sixteen[i].start == 5400 && sixteen[i].start == (juce::int64) i * 5400;

            how.parts = 500;
            Markers lots;

            for (int i = 0; i < 100; ++i)
                lots.push_back ({ (juce::int64) i * 800, 0, {}, -1 });

            const auto crowded = Sampler::makeSample (steps, rate, "Crowded", lots);
            auto byMarkers = how;
            byMarkers.source = Sampler::SliceSource::markers;

            check (equal && Sampler::findSlices (*sample, {}, how).size() == 64 && Sampler::findSlices (*crowded, {}, byMarkers).size() == 64
                       && Sampler::zonesFromSlices (crowded, {}, Sampler::findSlices (*crowded, {}, byMarkers), false).back().highKey == 99,
                   "sampler: sixteen equal parts of 86,400 frames are 5,400 frames each; 500 parts asked for, or a hundred markers, give 64 slices, the last on D#7");
        }

        // Hits: a drum loop from Basic Drums cut where its hits start.
        std::vector<double> times;
        const auto loop = drumLoop (times);
        const auto drums = Sampler::makeSample (loop, rate, "Loop");

        {
            Sampler::Slicing how;
            how.source = Sampler::SliceSource::hits;
            how.sensitivity = 100.0f;
            const auto slices = Sampler::findSlices (*drums, {}, how);
            int found = 0, extra = 0;
            auto worstMs = 0.0;

            for (auto& slice : slices)
            {
                auto nearest = 1.0e9;

                for (const auto t : times)
                    if (std::abs ((double) slice.start / rate - t) < std::abs (nearest))
                        nearest = (double) slice.start / rate - t;

                if (std::abs (nearest) < 0.015)
                {
                    ++found;
                    worstMs = juce::jmax (worstMs, std::abs (nearest) * 1000.0);
                }
                else
                {
                    ++extra;
                }
            }

            how.sensitivity = 40.0f;
            const auto fewer = Sampler::findSlices (*drums, {}, how).size();
            how.sensitivity = 0.0f;
            const auto fewest = Sampler::findSlices (*drums, {}, how).size();
            auto covers = ! slices.empty() && slices.front().start == 0 && slices.back().end == loop.getNumSamples();

            for (size_t i = 1; covers && i < slices.size(); ++i)
                covers = slices[i].start == slices[i - 1].end;

            check (found == (int) times.size() && extra == 0 && worstMs < 6.0 && covers && fewer < slices.size() && fewest >= 1 && fewest < fewer,
                   "sampler: two bars of Basic Drums with " + juce::String (times.size()) + " hits are cut at " + juce::String (found) + " of them and nowhere else, each cut within "
                       + juce::String (worstMs, 1) + " ms of its hit, the slices covering the loop end to end; at 40 % sensitivity there are " + juce::String (fewer)
                       + " slices and at 0 % " + juce::String (fewest));

            // Notes from Slices: the loop again, from the notes.
            how.sensitivity = 100.0f;
            plain (sampler);
            sampler.slice (drums, {}, {}, how);
            const auto zones = sampler.getZones();
            const auto notes = Sampler::notesFromSlices (zones);
            std::vector<Event> part;
            std::vector<int> cuts;

            for (auto& note : notes)
            {
                part.push_back (on (note.start + 1.0e-7, note.pitch, note.velocity));
                part.push_back (off (note.start + note.hold * 0.5, note.pitch));
                cuts.push_back ((int) std::lround (note.start * rate));
            }

            cuts.push_back (loop.getNumSamples());   // the last slice's end is faded as the others are
            std::sort (part.begin(), part.end(), [] (const Event& a, const Event& b) { return a.when < b.when; });
            const auto rebuilt = play (sampler, part, 4.0);
            auto apart = 0.0f;
            double difference = 0.0, whole = 0.0;

            for (int i = 0; i < loop.getNumSamples(); ++i)
            {
                const auto d = rebuilt.getSample (0, i) - loop.getSample (0, i);
                difference += (double) d * d;
                whole += (double) loop.getSample (0, i) * loop.getSample (0, i);

                if (std::none_of (cuts.begin(), cuts.end(), [i] (int cut) { return std::abs (i - cut) < 72; }))
                    apart = juce::jmax (apart, std::abs (d));
            }

            const auto overall = 10.0 * std::log10 (difference / whole);
            auto inOrder = notes.size() == zones.size();

            for (size_t i = 0; inOrder && i < notes.size(); ++i)
                inOrder = notes[i].pitch == 36 + (int) i && std::abs (notes[i].start - (double) zones[i].start / rate) < 1.0e-9
                          && std::abs (notes[i].hold - (double) (zones[i].end - zones[i].start) / rate) < 1.0e-9;

            check (inOrder && apart < 1.0e-6f && overall < -30.0,
                   "sampler: Notes from Slices gives a note for each slice, at its time in the loop and as long; played back they are the loop again, within "
                       + juce::String (db (apart), 1) + " dB away from the cuts (1.5 ms either side of each) and " + juce::String (overall, 1) + " dB different over all");
        }

        // The drum map names the slices, and slices that cut each other do.
        {
            Sampler::Slicing how;
            how.source = Sampler::SliceSource::markers;
            how.cutEachOther = true;
            plain (sampler);
            sampler.slice (sample, {}, {}, how);
            const auto map = Sampler::drumMapFor (sampler.getZones());
            const auto out = play (sampler, { on (0.0, 36), on (0.05, 37) }, 0.3);
            const auto cut = toneAt (out, 0.06, 0.1, 300.0) < 0.01 && toneAt (out, 0.06, 0.1, 400.0) > 0.4;

            juce::MemoryBlock state;
            sampler.getStateInformation (state);
            Processor other;
            other.setStateInformation (state.getData(), (int) state.getSize());
            const auto kept = other.getSlicing() == sampler.getSlicing() && other.getSlicing().on && other.getSlicing().cutEachOther
                              && other.getSlicing().source == Sampler::SliceSource::markers && other.getZones().size() == 9 && other.getZones()[2].name == "Hit 2";

            check (map.size() == 9 && map[0].pitch == 36 && map[0].name == "Start" && map[2].pitch == 38 && map[2].name == "Hit 2" && map[8].pitch == 44 && map[0].standsFor == -1
                       && cut && ClickCheck::worst (out, rate, 0.01f, { 0, (int) (0.05 * rate) }).ratio < 4.0f && kept,
                   "sampler: its drum map has a row for each slice, named after it, on its key; with Cut Each Other a slice stops the last without a click; how it was sliced, and the slices' names, come back from its state (a sample made in memory, with no file, doesn't)");
        }
    }

    /** A track's effects as a session keeps them, holding one sampler with this state. */
    std::unique_ptr<juce::XmlElement> effectsOf (Processor& sampler)
    {
        auto xml = std::make_unique<juce::XmlElement> ("EFFECTS");
        auto* slot = xml->createNewChildElement ("SLOT");
        slot->setAttribute ("bypassed", false);
        slot->setAttribute ("id", 1);
        slot->addChildElement (sampler.getPluginDescription().createXml().release());
        juce::MemoryBlock state;
        sampler.getStateInformation (state);
        slot->createNewChildElement ("STATE")->addTextElement (state.toBase64Encoding());
        return xml;
    }

    /** The sampler's state out of a track's effects as a session gives them back. */
    juce::MemoryBlock stateIn (const juce::XmlElement& effects)
    {
        juce::MemoryBlock state;

        if (auto* slot = effects.getChildByName ("SLOT"))
            if (auto* text = slot->getChildByName ("STATE"))
                state.fromBase64Encoding (text->getAllSubText());

        return state;
    }

    /** A harmonic tone: partials falling off as 1/n. */
    juce::AudioBuffer<float> harmonic (double hz, double seconds, int partials = 8)
    {
        juce::AudioBuffer<float> b (1, (int) (seconds * rate));

        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            auto x = 0.0;

            for (int n = 1; n <= partials && hz * n < 0.45 * rate; ++n)
                x += std::sin (twoPi * hz * n * i / rate) / n;

            b.setSample (0, i, (float) (0.3 * x));
        }

        return b;
    }

    /** Files: the sampler chunk, pitch and loops found, the shared store, the limits, sessions moved and copied, missing files. */
    void testFiles (AudioEngine& engine, Processor& sampler)
    {
        const auto folder = juce::File::createTempFile ("samplerfiles");
        folder.createDirectory();

        // A WAV's sampler chunk gives the root and the loop.
        {
            const auto file = folder.getChildFile ("Chunk.wav");
            std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream> (file);
            const auto tone = sine (220.0, 1.0);
            auto writer = juce::WavAudioFormat().createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (rate).withNumChannels (1).withBitsPerSample (24)
                              .withMetadataValues ({ { "MidiUnityNote", "57" }, { "MidiPitchFraction", "0" }, { "NumSampleLoops", "1" }, { "Loop0Type", "0" },
                                                     { "Loop0Start", "9600" }, { "Loop0End", "11999" } }));
            if (writer != nullptr)
                writer->writeFromAudioSampleBuffer (tone, 0, tone.getNumSamples());

            writer.reset();
            juce::String error;
            const auto chunked = Sampler::sharedSample (file, error);
            const auto bare = Sampler::sharedSample (writeWav (folder.getChildFile ("Bare.wav"), tone, rate), error);

            check (chunked != nullptr && chunked->fileRoot == 57 && chunked->fileLoop == 1 && chunked->fileLoopStart == 9600 && chunked->fileLoopEnd == 12000
                       && bare != nullptr && bare->fileRoot == -1 && bare->fileLoop == 0,
                   "sampler: a WAV file's sampler chunk gives its root note (A2) and its loop (frames 9,600 to 12,000, the chunk's last frame being the last played); a WAV without one gives neither");
        }

        // The pitch found: the key and the tuning, for tones in and between the keys; none for noise.
        {
            auto worst = 0.0;
            juce::StringArray readings;

            for (const auto hz : { 41.2, 55.0, 110.0, 222.0, 440.0, 452.0, 880.0, 1760.0 })
            {
                const auto pitch = Sampler::findPitch (*Sampler::makeSample (harmonic (hz, 1.0), rate, "tone"));
                const auto wanted = 69.0 + 12.0 * std::log2 (hz / 440.0);
                const auto cents = pitch.found ? 100.0 * ((double) pitch.root - pitch.tune * 0.01 - wanted) : 1.0e6;
                worst = juce::jmax (worst, std::abs (cents));
                readings.add (Instrument::pitchName (pitch.root) + (pitch.tune >= 0.0f ? " +" : " ") + juce::String (pitch.tune, 1));
            }

            const auto none = Sampler::findPitch (*Sampler::makeSample (noise (1.0), rate, "noise"));
            const auto flat = Sampler::findPitch (*Sampler::makeSample (harmonic (452.0, 1.0), rate, "flat"));

            check (worst < 3.0 && ! none.found && none.root == 60 && juce::exactlyEqual (none.tune, 0.0f) && flat.root == 69 && flat.tune < -40.0f && flat.tune > -52.0f,
                   "sampler: the pitch of harmonic tones from 41 Hz to 1,760 Hz is found within " + juce::String (worst, 2) + " cents (" + readings.joinIntoString (", ")
                       + "); a tone 47 cents above A4 gets A4 and " + juce::String (flat.tune, 1) + " cents of tuning; noise has no pitch and gets C4");
        }

        // Find Loop: on a held note of Torqwave Synth, on two detuned tones beating, and on noise.
        {
            BuiltInEffects::BasicSynthProcessor synth;
            synth.prepareToPlay (rate, 512);
            juce::AudioBuffer<float> note (2, (int) (3.0 * rate)), block (2, 512);
            note.clear();

            for (int at = 0; at + 512 <= note.getNumSamples(); at += 512)
            {
                juce::MidiBuffer midi;

                if (at == 0)
                    midi.addEvent (juce::MidiMessage::noteOn (1, 45, 0.8f), 0);

                synth.processBlock (block, midi);
                note.copyFrom (0, at, block, 0, 0, 512);
                note.copyFrom (1, at, block, 1, 0, 512);
            }

            juce::AudioBuffer<float> beating (1, (int) (4.0 * rate));

            for (int i = 0; i < beating.getNumSamples(); ++i)
                beating.setSample (0, i, 0.25f * (float) (std::sin (twoPi * 110.0 * i / rate) + 0.5 * std::sin (twoPi * 220.0 * i / rate)
                                                          + std::sin (twoPi * 110.7 * i / rate) + 0.5 * std::sin (twoPi * 221.4 * i / rate)));

            juce::StringArray report;
            auto good = true;

            auto tryLoop = [&] (const juce::AudioBuffer<float>& audio, const juce::String& what, bool pitched)
            {
                const auto sample = Sampler::makeSample (audio, rate, what);
                const auto pitch = Sampler::findPitch (*sample);
                const auto loop = Sampler::findLoop (*sample, {}, pitch.hz);
                auto z = zoneOf (audio, pitch.root, what);
                z.sample = sample;
                z.tune = 0.0f;
                z.lowKey = 0;
                z.loop = 1;
                z.loopStart = loop.start;
                z.loopEnd = loop.end;
                z.crossfade = loop.crossfade;
                plain (sampler);
                sampler.setZones ({ z });
                const auto out = play (sampler, { on (0.0, pitch.root) }, 12.0);
                const auto click = ClickCheck::worst (out, rate, 0.005f);
                auto quietest = 1.0e9f, loudest = 0.0f;

                for (auto t = 3.0; t < 11.5; t += 0.25)
                {
                    quietest = juce::jmin (quietest, rms (out, t, 0.25));
                    loudest = juce::jmax (loudest, rms (out, t, 0.25));
                }

                const auto seconds = (double) (loop.end - loop.start) / rate;
                good = good && loop.found && pitch.found == pitched && click.ratio < 4.0f && quietest > 0.01f && seconds > 0.3 && loop.end <= audio.getNumSamples();
                report.add (what + ": a loop of " + juce::String (seconds, 2) + " s with a " + juce::String (loop.crossfade, 0) + " ms crossfade, the worst jump over twelve seconds "
                            + juce::String (click.ratio, 2) + " times its surroundings, the level moving " + juce::String (db (loudest / quietest), 1) + " dB");
            };

            tryLoop (note, "a three-second note of Torqwave Synth", true);
            tryLoop (beating, "two tones 0.7 Hz apart", true);
            tryLoop (noise (3.0, 21), "noise", false);

            const auto tooShort = Sampler::findLoop (*Sampler::makeSample (sine (220.0, 0.1), rate, "short"), {}, 220.0);

            check (good && ! tooShort.found, "sampler: Find Loop on " + report.joinIntoString ("; ") + "; a tenth of a second is too short to loop");
        }

        // One store: two samplers on a file hold it once, and a file that changes is read again.
        // The limits: a sample's length, and what a sampler holds.
        {
            const auto file = writeWav (folder.getChildFile ("Shared.wav"), sine (330.0, 1.0), rate);
            juce::String error;
            const auto first = Sampler::sharedSample (file, error);
            const auto second = Sampler::sharedSample (file, error);

            Sampler::Zone z;
            z.sample = first;
            z.file = file;
            sampler.setZones ({ z });
            juce::MemoryBlock state;
            sampler.getStateInformation (state);
            Processor other;
            other.setStateInformation (state.getData(), (int) state.getSize());
            const auto once = other.getZones().size() == 1 && other.getZones()[0].sample == first && first == second;

            juce::Thread::sleep (1100);    // so its modification time moves on
            writeWav (file, sine (550.0, 2.0), rate);
            const auto changed = Sampler::sharedSample (file, error);

            // Eleven minutes at 8 kHz: too long.
            juce::AudioBuffer<float> hours (1, 8000 * 660);
            hours.clear();
            juce::String tooLong;
            const auto refused = Sampler::sharedSample (writeWav (folder.getChildFile ("Long.wav"), hours, 8000.0), tooLong);

            // What a sampler may hold, made small for the test: 1 MB holds one of these and not two.
            const auto a = writeWav (folder.getChildFile ("A.wav"), sine (220.0, 3.0), rate), b = writeWav (folder.getChildFile ("B.wav"), sine (330.0, 3.0), rate);
            Sampler::Zone za, zb;
            za.sample = Sampler::sharedSample (a, error);
            za.file = a;
            zb.sample = Sampler::sharedSample (b, error);
            zb.file = b;
            sampler.setZones ({ za, zb });
            sampler.getStateInformation (state);
            Processor small;
            small.setMostBytes (1 << 20);
            small.setStateInformation (state.getData(), (int) state.getSize());
            const auto room = small.roomFor (zb.sample);

            check (once && changed != nullptr && changed != first && changed->getNumFrames() == 96000
                       && refused == nullptr && tooLong.contains ("11 minutes") && tooLong.contains ("up to 10 minutes")
                       && sampler.bytesHeld() == 2 * 144000 * 4 && small.getZones()[0].sample != nullptr && small.getZones()[1].sample == nullptr
                       && small.getMissing().size() == 1 && small.getMissing()[0].contains ("B.wav") && room.contains ("held in memory") && small.roomFor (za.sample).isEmpty(),
                   "sampler: two samplers on one file hold one copy of it, and a file that has changed is read again; an eleven-minute file is refused (" + tooLong
                       + "); a sampler allowed 1 MB keeps the first of two 576 KB samples and leaves out the second, saying why (" + small.getMissing().joinIntoString ("; ") + ")");
        }

        // A session with a sampler, saved, moved to another folder with its audio, and opened there.
        {
            const auto home = folder.getChildFile ("Song"), moved = folder.getChildFile ("Song Moved");
            home.getChildFile ("Sounds").createDirectory();
            const auto file = writeWav (home.getChildFile ("Sounds").getChildFile ("Lead.wav"), sine (440.0, 1.0), rate);
            juce::String error;
            Sampler::Zone z;
            z.sample = Sampler::sharedSample (file, error);
            z.file = file;
            z.root = 69;
            plain (sampler);
            sampler.setZones ({ z });
            const auto before = play (sampler, { on (0.0, 69) }, 0.5);

            SessionState session;
            session.sampleRate = rate;
            const auto trackId = session.addInstrumentTrack().id;
            const auto saved = SessionFile::save (session, home.getChildFile ("Song.tqsession"), [&] (juce::uint32 id) { return id == trackId ? effectsOf (sampler) : nullptr; });
            const auto movedOk = home.moveFileTo (moved);

            auto loaded = SessionFile::load (moved.getChildFile ("Song.tqsession"), engine.formatManager);
            Processor there;
            auto restored = false;

            if (loaded.effects.size() == 1)
            {
                const auto state = stateIn (*loaded.effects.begin()->second);
                there.setStateInformation (state.getData(), (int) state.getSize());
                restored = true;
            }

            const auto zones = there.getZones();
            const auto after = restored ? play (there, { on (0.0, 69) }, 0.5) : juce::AudioBuffer<float>();

            check (saved.isEmpty() && movedOk && restored && zones.size() == 1 && zones[0].sample != nullptr
                       && zones[0].file == moved.getChildFile ("Sounds").getChildFile ("Lead.wav") && there.getMissing().isEmpty() && same (before, after),
                   "sampler: a session with a sampler, its folder moved with its audio, opens with the sample found beside it and plays the same");

            // Save a Copy with Audio takes the samples with it.
            const auto copy = folder.getChildFile ("Copy").getChildFile ("Copy.tqsession");
            copy.getParentDirectory().createDirectory();
            juce::StringArray missing;
            const auto copied = SessionCollect::save (session, copy, [&] (juce::uint32 id) { return id == trackId ? effectsOf (there) : nullptr; }, missing);
            const auto sampleCopy = copy.getParentDirectory().getChildFile ("Audio").getChildFile ("Samples").getChildFile ("Lead.wav");
            const auto wasCopied = sampleCopy.existsAsFile();
            moved.deleteRecursively();

            loaded = SessionFile::load (copy, engine.formatManager);
            Processor fromCopy;

            if (loaded.effects.size() == 1)
            {
                const auto state = stateIn (*loaded.effects.begin()->second);
                fromCopy.setStateInformation (state.getData(), (int) state.getSize());
            }

            const auto copyZones = fromCopy.getZones();

            check (copied.isEmpty() && missing.isEmpty() && wasCopied && copyZones.size() == 1 && copyZones[0].sample != nullptr && copyZones[0].file == sampleCopy
                       && same (before, play (fromCopy, { on (0.0, 69) }, 0.5)),
                   "sampler: Save a Copy with Audio copies the sampler's sample into the copy's Audio/Samples folder, and the copy plays the same with the original deleted");

            // A sample that has gone: reported, its zone silent, and found again where it is now.
            juce::MemoryBlock state;
            fromCopy.getStateInformation (state);
            const auto elsewhere = folder.getChildFile ("Elsewhere");
            elsewhere.createDirectory();
            sampleCopy.moveFileTo (elsewhere.getChildFile ("Lead.wav"));
            Processor lost;
            lost.setStateInformation (state.getData(), (int) state.getSize());
            const auto reported = lost.getMissing().size() == 1 && lost.getZones()[0].sample == nullptr;
            const auto silent = play (lost, { on (0.0, 69) }, 0.3);
            const auto notThere = lost.locate (folder);
            const auto stillMissing = lost.getMissing().size();
            const auto found = lost.locate (elsewhere);

            check (reported && silent.getMagnitude (0, silent.getNumSamples()) == 0.0f && notThere == 0 && stillMissing == 1 && found == 1 && lost.getMissing().isEmpty()
                       && lost.getZones()[0].file == elsewhere.getChildFile ("Lead.wav") && same (before, play (lost, { on (0.0, 69) }, 0.5)),
                   "sampler: a sample moved away is reported and its zone silent; looked for in the wrong folder it stays missing, and in the right one it comes back and plays the same");
        }

        // A sound that is not a file, written to be sampled: a float WAV with its markers, under a name not yet used.
        {
            Markers markers { { 4800, 0, "One", -1 }, { 9600, 2400, "Two", -1 } };
            juce::String error;
            const auto tone = sine (300.0, 0.5, rate, 2);
            const auto first = Sampler::writeSample (tone, rate, "From: Selection", markers, error, folder.getChildFile ("Samples"));
            const auto second = Sampler::writeSample (tone, rate, "From: Selection", markers, error, folder.getChildFile ("Samples"));
            const auto back = Sampler::sharedSample (first, error);
            auto exact = back != nullptr && back->getNumFrames() == tone.getNumSamples() && back->audio.getNumChannels() == 2;

            for (int i = 0; exact && i < tone.getNumSamples(); ++i)
                exact = juce::exactlyEqual (back->audio.getSample (0, i), tone.getSample (0, i));

            check (error.isEmpty() && first.existsAsFile() && second.existsAsFile() && first != second && second.getFileName().contains (" 2.wav") && exact
                       && back->markers.size() == 2 && back->markers[0].name == "One" && back->markers[1].isRegion() && back->markers[1].length == 2400,
                   "sampler: a sound written to be sampled is a WAV with every sample exact and its marker and region, named " + first.getFileName() + " and, the second time, " + second.getFileName());
        }

        sampler.setZones ({});
        folder.deleteRecursively();
    }

    void runBlock (Processor& sampler)
    {
        juce::AudioBuffer<float> block (2, 512);
        juce::MidiBuffer midi;
        sampler.processBlock (block, midi);
    }

    float rootRangeOf (int key)      { return (float) key / 127.0f; }

    /** The window: its knobs, the waveform's handles, the key map, slices and their cuts. */
    void testWindow (Processor& sampler)
    {
        const auto folder = juce::File::createTempFile ("samplerwindow");
        folder.createDirectory();
        const auto toneFile = writeWav (folder.getChildFile ("Held Note.wav"), harmonic (110.0, 2.0), rate);
        const auto otherFile = writeWav (folder.getChildFile ("Other.wav"), harmonic (440.0, 1.0), rate);
        std::vector<double> times;
        const auto loopFile = writeWav (folder.getChildFile ("Break.wav"), drumLoop (times), rate);

        defaults (sampler);
        sampler.setZones ({});
        sampler.setSlicing ({});
        sampler.prepareToPlay (rate, 512);
        std::unique_ptr<juce::AudioProcessorEditor> made (sampler.createEditorAndMakeActive());
        auto* window = dynamic_cast<SamplerEditor*> (made.get());

        if (window == nullptr)
        {
            check (false, "sampler window: it has one of its own");
            return;
        }

        const auto text = [&] (const char* name)
        {
            auto* k = window->knob (name);
            return k != nullptr ? k->getValueText() : juce::String ("?");
        };

        // Empty, then with a sound: a zone over every key, its pitch found.
        const auto sized = window->getWidth() == SamplerEditor::windowWidth && window->getHeight() == SamplerEditor::windowHeight;
        const auto empty = window->getZones().empty() && window->getSelected() == -1 && window->getStatus() == "No sample yet";
        const auto loaded = window->loadFile (toneFile);
        const auto first = window->getZones().size() == 1 && window->getSelected() == 0 && window->getZones()[0].root == 45 && window->getZones()[0].lowKey == 0
                           && window->getZones()[0].highKey == 127 && window->getStatus().contains ("Held Note") && window->getStatus().contains ("root A2") && text ("Root") == "A2";
        const auto named = window->getStatus().upToFirstOccurrenceOf ("  |", false, false);
        const auto refused = ! window->loadFile (folder.getChildFile ("Nothing.wav")) && window->getStatus().contains ("was not found") && window->getZones().size() == 1;

        check (sized && empty && loaded && first && refused,
               "sampler window: 940 by 746; empty it says so; a sound given to it becomes a zone over every key with its root found (A2), named in its status ("
                   + named + "); a file that isn't there is refused with the reason and nothing changes");

        // Its knobs read the settings in their units, and turn them.
        {
            const auto readings = text ("Cutoff") + ", " + text ("Level") + ", " + text ("Attack") + ", " + text ("Release") + ", " + text ("Velocity") + ", " + text ("Rate")
                                  + ", " + text ("Bend") + ", " + text ("Transpose") + ", " + text ("Crossfade") + ", " + text ("Tune");
            auto* cutoff = window->knob ("Cutoff");
            cutoff->setNormalised (0.5f);
            const auto turned = sampler.parameter ("cutoff")->getValue();
            set (sampler, "cutoff", 20000.0f);
            window->refreshNow();
            int found = 0;

            for (const auto name : { "Root", "Tune", "Zone Level", "Zone Pan", "Crossfade", "Sensitivity", "Attack", "Decay", "Sustain", "Release", "Velocity", "Cutoff", "Reso", "Key", "Env",
                                     "F Attack", "F Decay", "F Sustain", "F Release", "Vel", "Rate", "Fade In", "To Pitch", "To Filter", "To Level", "To Pan", "Level", "Pan",
                                     "Transpose", "Fine", "Bend", "Glide", "Wheel" })
                found += window->knob (name) != nullptr ? 1 : 0;

            check (readings == "20.0 kHz, -6.0 dB, 0.00 ms, 100.0 ms, 80 %, 5.00 Hz, 2 st, 0 st, 0.00 ms, 0 ct" && std::abs (turned - 0.5f) < 1.0e-6f && found == 33
                       && text ("Cutoff") == "20.0 kHz",
                   "sampler window: its 33 knobs are there, reading as it comes " + readings + "; a knob turned sets its parameter, and follows it back");
        }

        // The zone's knobs and switches set the zone.
        {
            window->knob ("Root")->setNormalised (rootRangeOf (57));
            window->knob ("Zone Level")->setNormalised (0.5f);
            const auto z = sampler.getZones()[0];
            check (z.root == 57 && std::abs (z.level - (-18.0f)) < 0.01f && text ("Root") == "A3",
                   "sampler window: the Root knob sets the zone's root (A3) and Zone Level its level");
        }

        // The waveform's handles: the part's ends, the loop's, and the crossfade.
        {
            auto& wave = window->getWave();
            using Handle = SamplerEditor::Wave::Handle;
            wave.setBounds (wave.getBounds());
            int cut = 0;
            const auto whole = wave.getView().getStart() == 0.0 && wave.getView().getLength() == 96000.0 && std::abs (wave.xOf (48000) - (float) wave.getWidth() * 0.5f) < 1.0f
                               && std::abs ((double) wave.frameAt ((float) wave.getWidth() * 0.25f) - 24000.0) < 200.0;
            const auto atStart = wave.handleAt ({ wave.xOf (0) + 2.0f, 60.0f }, cut) == Handle::start, atEnd = wave.handleAt ({ wave.xOf (96000) - 2.0f, 60.0f }, cut) == Handle::end;
            const auto nowhere = wave.handleAt ({ wave.xOf (48000), 60.0f }, cut) == Handle::none;

            wave.dragTo (Handle::start, 0, 4800);
            wave.dragTo (Handle::end, 0, 90000);
            auto z = sampler.getZones()[0];
            const auto ends = z.start == 4800 && z.end == 90000;
            wave.dragTo (Handle::start, 0, 95000);       // not past the end
            const auto held = sampler.getZones()[0].start == 90000 - 16;
            wave.dragTo (Handle::start, 0, 4800);

            window->findLoop();
            z = sampler.getZones()[0];
            const auto found = z.loop == 1 && z.loopStart > 4800 && z.loopEnd > z.loopStart && z.loopEnd <= 90000 && z.crossfade > 0.0f;
            const auto onLoop = wave.handleAt ({ wave.xOf (z.loopStart), 60.0f }, cut) == Handle::loopStart && wave.handleAt ({ wave.xOf (z.loopEnd), 60.0f }, cut) == Handle::loopEnd
                                && wave.handleAt ({ wave.xOf (z.loopEnd - (juce::int64) (z.crossfade * 0.001 * rate)), 8.0f }, cut) == Handle::crossfade;

            wave.dragTo (Handle::loopStart, 0, 30000);
            wave.dragTo (Handle::loopEnd, 0, 60000);
            wave.dragTo (Handle::crossfade, 0, 60000 - 2400);
            z = sampler.getZones()[0];
            const auto moved = z.loopStart == 30000 && z.loopEnd == 60000 && std::abs (z.crossfade - 50.0f) < 0.01f && text ("Crossfade") == "50.0 ms";
            wave.dragTo (Handle::loopStart, 0, 70000);   // not past the loop's end
            const auto ordered = sampler.getZones()[0].loopStart == 60000 - 4;

            // Zoomed in, the handles are where the frames are.
            wave.setView ({ 20000.0, 40000.0 });
            const auto zoomed = std::abs (wave.xOf (30000) - (float) wave.getWidth() * 0.5f) < 1.0f && wave.frameAt (0.0f) == 20000;
            wave.showAll();

            check (whole && atStart && atEnd && nowhere && ends && held && found && onLoop && moved && ordered && zoomed,
                   "sampler window: the waveform shows the whole sample; its start and end handles are at the part's ends and drag them (not past each other); Find Loop sets a loop with a crossfade, "
                   "whose handles sit at the loop's ends and, along the top, where the crossfade begins; dragged, they move the loop (kept in order) and set the crossfade (50 ms for 2,400 frames); zoomed in, frames are where they are drawn");
        }

        // The key map: a zone's edges set its keys and velocities; a sound on a key is a new zone there.
        {
            auto& keys = window->getKeyMap();
            using Edge = SamplerEditor::KeyMap::Edge;
            const auto all = keys.zoneBounds (0);
            const auto across = std::abs (all.getX()) < 0.5f && std::abs (all.getRight() - (float) keys.getWidth()) < 0.5f && keys.keyAt (0.0f) == 0 && keys.keyAt ((float) keys.getWidth() - 1.0f) == 127
                                && keys.keyAt ((float) keys.getWidth() * 60.5f / 128.0f) == 60;
            keys.dragEdge (0, Edge::high, 71);
            keys.dragEdge (0, Edge::low, 36);
            keys.dragEdge (0, Edge::top, 100);
            keys.dragEdge (0, Edge::bottom, 20);
            auto z = sampler.getZones()[0];
            const auto edges = z.lowKey == 36 && z.highKey == 71 && z.lowVelocity == 20 && z.highVelocity == 100;
            keys.dragEdge (0, Edge::low, 90);
            const auto ordered = sampler.getZones()[0].lowKey == 71;
            keys.dragEdge (0, Edge::low, 36);
            const auto drawn = keys.zoneBounds (0);
            const auto placed = std::abs (drawn.getX() - (float) keys.getWidth() * 36.0f / 128.0f) < 0.5f && std::abs (drawn.getRight() - (float) keys.getWidth() * 72.0f / 128.0f) < 0.5f
                                && drawn.getY() > all.getY() && drawn.getBottom() < all.getBottom();

            const auto added = window->loadFile (otherFile, 72);
            const auto zones = sampler.getZones();
            const auto second = added && zones.size() == 2 && zones[1].lowKey == 72 && zones[1].highKey == 83 && zones[1].root == 69 && window->getSelected() == 1
                                && keys.zoneAt (keys.zoneBounds (0).getCentre()) == 0 && keys.zoneAt (keys.zoneBounds (1).getCentre()) == 1 && keys.zoneAt ({ 2.0f, 5.0f }) == -1;
            window->select (1);
            window->removeZone();
            const auto removed = sampler.getZones().size() == 1 && window->getSelected() == 0;

            check (across && edges && ordered && placed && second && removed,
                   "sampler window: a zone over every key is drawn across the whole key map; its edges dragged set its keys (C2 to B4) and velocities (20 to 100), kept in order, and it is drawn there; "
                   "a sound given at C5 is a second zone over the octave from C5 with its own root (A4), selected; Remove Zone takes it away");
        }

        // Find Pitch, and what a preset carries: the zones with their files.
        {
            window->changeSelected ([] (Sampler::Zone& z) { z.root = 12; z.tune = 30.0f; });
            window->findPitch();
            const auto z = sampler.getZones()[0];
            const auto texts = sampler.getPresetTexts();
            Processor other;
            other.setPresetTexts (texts);
            const auto back = other.getZones();
            other.setPresetTexts ({});

            check (z.root == 45 && std::abs (z.tune) < 3.0f && back.size() == 1 && back[0].sample != nullptr && back[0].file == toneFile && back[0].loopStart == z.loopStart
                       && back[0].lowKey == 36 && other.getZones().size() == 1,
                   "sampler window: Find Pitch puts back the root (A2) and tuning; what a preset carries beside its values brings the zones back in another sampler, their samples read from their files, "
                   "and a preset without zones leaves them as they are");
        }

        // Slices: the sample cut at its hits, a pad each; cuts dragged, added and taken away.
        {
            window->select (0);
            window->changeSelected ([] (Sampler::Zone& z) { z.lowKey = 0; z.highKey = 127; });
            const auto swapped = window->loadFile (loopFile);        // in place of the selected zone's sample
            const auto one = sampler.getZones().size() == 1 && sampler.getZones()[0].file == loopFile;
            window->setSlices (true);
            auto zones = sampler.getZones();
            const auto sliced = swapped && one && window->showsSlices() && zones.size() == times.size() && zones[0].lowKey == 36 && zones[0].oneShot;

            auto& wave = window->getWave();
            auto& keys = window->getKeyMap();
            using Handle = SamplerEditor::Wave::Handle;
            int cut = 0;
            const auto onCut = wave.handleAt ({ wave.xOf (zones[3].start), 50.0f }, cut) == Handle::cut && cut == 3;
            const auto was = zones[3].start;
            wave.dragTo (Handle::cut, 3, was + 300);
            zones = sampler.getZones();
            const auto dragged = zones[3].start == was + 300 && zones[2].end == was + 300;
            wave.dragTo (Handle::cut, 3, zones[4].end + 5000);          // not past the next slice's end
            zones = sampler.getZones();
            const auto held = zones[3].start == zones[3].end - 16 && zones[2].end == zones[3].start;
            wave.dragTo (Handle::cut, 3, was);

            const auto count = (int) sampler.getZones().size();
            const auto middle = (sampler.getZones()[5].start + sampler.getZones()[5].end) / 2;
            const auto added = wave.addCut (middle);
            zones = sampler.getZones();
            const auto split = added && (int) zones.size() == count + 1 && zones[5].end == middle && zones[6].start == middle && zones[6].lowKey == 42 && zones[7].lowKey == 43
                               && zones[6].name == "Slice 7";
            const auto taken = wave.removeCut (6);
            zones = sampler.getZones();
            const auto joined = taken && (int) zones.size() == count && zones[5].end > middle && zones[6].lowKey == 42 && ! wave.removeCut (0) && ! wave.removeCut (99);

            // A pad lights while its slice plays.
            const auto pads = keys.padBounds (0).getWidth() > 20.0f && keys.padBounds (count - 1).getBottom() <= (float) keys.getHeight() && keys.zoneAt (keys.padBounds (2).getCentre()) == 2;
            sampler.play (38, 1.0f);
            runBlock (sampler);
            const auto lit = keys.isLit (2) && ! keys.isLit (1);
            sampler.reset();

            for (int i = 0; i < 4; ++i)
                runBlock (sampler);

            const auto dark = ! keys.isLit (2);

            // Cut again in equal parts, and back to one zone across the keys.
            auto how = sampler.getSlicing();
            how.source = Sampler::SliceSource::equalParts;
            how.parts = 8;
            sampler.setSlicing (how);
            window->sliceAgain();
            const auto eight = sampler.getZones().size() == 8 && sampler.getZones()[7].end == 192000;
            window->setSlices (false);
            const auto whole = ! window->showsSlices() && sampler.getZones().size() == 1 && sampler.getZones()[0].lowKey == 0 && sampler.getZones()[0].highKey == 127 && ! sampler.getZones()[0].oneShot;

            check (sliced && onCut && dragged && held && split && joined && pads && lit && dark && eight && whole,
                   "sampler window: Slices cuts the selected zone's sample at its hits (" + juce::String (times.size()) + " slices from C2, a pad each); a cut is where its slice starts and drags it, "
                   "not past the next slice; a cut added makes two slices of one and moves the keys after it up, and taken away joins them again; a pad lights while its slice plays; "
                   "Slice Again in eight equal parts gives eight; back to Keys, the sample is one zone over every key");
        }

        // With everything an instrument's window adds to the presets row, the preset menu keeps room for a name.
        {
            int menuWidth = 0;

            if (auto* row = InstrumentKeys::presetsRow (*window))
            {
                row->showAudition (true, [] (bool) {});
                row->showAuditionLoop (false, [] (bool) {}, [] {});
                row->showPatterns ([] { return juce::StringArray { "Low Riff", "Octave Run" }; }, [] { return 0; }, [] (int) {});
                window->resized();

                for (auto* c : row->getChildren())
                    if (auto* menu = dynamic_cast<juce::ComboBox*> (c))
                        menuWidth = menu->getWidth();
            }

            check (menuWidth >= 180, "sampler window: with Audition, Loop, Stop and the pattern menu showing, the preset menu is " + juce::String (menuWidth) + " px wide, room for a name");
        }

        // The line on the waveform is where the voice reads, through a loop forward and back too: the sample is a
        // ramp, so what is heard says which frame was read.
        {
            constexpr int frames = 24000, blockSize = 256;
            juce::AudioBuffer<float> ramp (1, frames);

            for (int i = 0; i < frames; ++i)
                ramp.setSample (0, i, 0.1f + 0.8f * (float) i / (float) frames);

            const auto frameHeard = [] (float heard, float gain)    { return ((double) (heard / gain) - 0.1) / 0.8 * (double) frames; };

            // How far the line is from the frame last heard, at its worst over the blocks, and how often it turned.
            const auto follow = [&] (bool reverse, int& turns, int& lines)
            {
                plain (sampler);
                sampler.setSlicing ({});
                auto z = zoneOf (ramp, 60);
                z.loop = 2;
                z.loopStart = 6000;
                z.loopEnd = 9000;
                z.reverse = reverse;
                sampler.setZones ({ z });
                sampler.prepareToPlay (rate, blockSize);
                window->refreshNow();
                window->select (0);

                juce::AudioBuffer<float> block (2, blockSize);
                double worst = 0.0;
                float gain = 0.0f;
                juce::int64 last = -1;
                int direction = 0;
                turns = 0;
                lines = 0;

                for (int n = 0; n < 110; ++n)
                {
                    juce::MidiBuffer midi;

                    if (n == 0)
                        midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);

                    block.clear();
                    sampler.processBlock (block, midi);

                    if (n == 0)     // the frame it read at the block's 200th sample is known: 200 along the way it plays
                        gain = block.getSample (0, 200) / ramp.getSample (0, reverse ? frames - 1 - 200 : 200);

                    const auto drawn = window->getWave().playingFrames();
                    lines = juce::jmax (lines, (int) drawn.size());

                    if (drawn.size() != 1)
                        return 1.0e9;

                    worst = juce::jmax (worst, std::abs ((double) drawn[0] - frameHeard (block.getSample (0, blockSize - 1), gain)));

                    if (last >= 0 && drawn[0] != last)
                    {
                        const auto now = drawn[0] > last ? 1 : -1;
                        turns += direction != 0 && now != direction ? 1 : 0;
                        direction = now;
                    }

                    last = drawn[0];
                }

                juce::MidiBuffer end;
                end.addEvent (juce::MidiMessage::allSoundOff (1), 0);
                block.clear();
                sampler.processBlock (block, end);
                return worst;
            };

            int turns = 0, lines = 0, turnsReversed = 0, linesReversed = 0;
            const auto off = follow (false, turns, lines);
            const auto offReversed = follow (true, turnsReversed, linesReversed);

            check (off <= 2.0 && offReversed <= 2.0 && turns >= 4 && turnsReversed >= 4 && lines == 1 && linesReversed == 1,
                   "sampler window: through a loop forward and back the playing line stays on the frame heard, within " + juce::String (off, 1) + " frames over "
                       + juce::String (turns) + " turns, and within " + juce::String (offReversed, 1) + " over " + juce::String (turnsReversed) + " on a reversed zone");
        }

        sampler.editorBeingDeleted (made.get());   // as its host tells it
        made.reset();
        sampler.setZones ({});
        sampler.setSlicing ({});
        folder.deleteRecursively();
    }

    /** Getting sound in: several files at once, a sound from the Files panel, a clip and a selection as sampler tracks. */
    void testGettingSoundIn (AudioEngine& engine, Processor& sampler)
    {
        const auto folder = juce::File::createTempFile ("samplersounds");
        folder.createDirectory();

        // Several sounds at once: around their own notes when each has a pitch, else a key each.
        {
            const auto low = writeWav (folder.getChildFile ("Low.wav"), harmonic (110.0, 1.0), rate), mid = writeWav (folder.getChildFile ("Mid.wav"), harmonic (220.0, 1.0), rate),
                       high = writeWav (folder.getChildFile ("High.wav"), harmonic (440.0, 1.0), rate), hiss = writeWav (folder.getChildFile ("Hiss.wav"), noise (0.5), rate);
            defaults (sampler);
            sampler.setZones ({});
            sampler.setSlicing ({});
            std::unique_ptr<juce::AudioProcessorEditor> made (sampler.createEditorAndMakeActive());
            auto* window = dynamic_cast<SamplerEditor*> (made.get());
            const auto pitched = window != nullptr && window->loadFiles ({ high, low, mid });
            auto zones = sampler.getZones();
            const auto around = pitched && zones.size() == 3 && zones[0].root == 45 && zones[0].lowKey == 0 && zones[0].highKey == 51 && zones[1].root == 57 && zones[1].lowKey == 52
                                && zones[1].highKey == 63 && zones[2].root == 69 && zones[2].lowKey == 64 && zones[2].highKey == 127 && ! zones[0].oneShot;

            sampler.setZones ({});
            const auto mixed = window != nullptr && window->loadFiles ({ low, hiss, mid }, 48);
            zones = sampler.getZones();
            const auto keys = mixed && zones.size() == 3 && zones[0].lowKey == 48 && zones[0].highKey == 48 && zones[0].root == 48 && zones[1].lowKey == 49 && zones[2].lowKey == 50
                              && zones[0].oneShot && zones[2].oneShot;

            // An open file from the Files panel: a saved one is sampled where it is, one with changes is written first.
            sampler.setZones ({});
            const auto samples = folder.getChildFile ("Samples");
            auto asked = 0.0;
            SoundDrops::isSound = [] (const juce::var& description) { return description.toString().startsWith ("openfile:"); };
            SoundDrops::soundOf = [&] (const juce::var& description, double maxSeconds) -> std::optional<SoundDrops::Sound>
            {
                asked = maxSeconds;
                SoundDrops::Sound sound;
                sound.audio = harmonic (220.0, 1.0);
                sound.sampleRate = rate;
                sound.name = "Open File";

                if (description.toString() == "openfile:0")
                    sound.file = mid;

                return sound;
            };

            auto savedOne = false, interested = false;

            if (window != nullptr)
            {
                const juce::DragAndDropTarget::SourceDetails saved ("openfile:0", window, { 100, 100 });
                interested = window->isInterestedInDragSource (saved) && ! window->isInterestedInDragSource ({ "dockPanel:3", window, { 100, 100 } });
                window->itemDropped (saved);
                savedOne = sampler.getZones().size() == 1 && sampler.getZones()[0].file == mid && sampler.getZones()[0].root == 57;
            }

            SoundDrops::isSound = nullptr;
            SoundDrops::soundOf = nullptr;
            sampler.editorBeingDeleted (made.get());
            made.reset();

            check (around && keys && interested && savedOne && juce::exactlyEqual (asked, Sampler::longestSampleSeconds),
                   "sampler: three pitched sounds given together become zones around their own notes (A2 up to D#3, A3 from E3 to D#4, A4 from E4), the keys between two shared; with one unpitched among them "
                   "each goes on a key of its own from the key they land on, as one-shots; an open file dragged from the Files panel that is saved as it stands is sampled where it is");
        }

        // A clip as a sampler track, across the keys and sliced.
        {
            std::vector<double> times;
            const auto loop = drumLoop (times);
            const auto loopFile = writeWav (folder.getChildFile ("Break.wav"), loop, rate);
            const auto toneFile = writeWav (folder.getChildFile ("Tone.wav"), harmonic (220.0 * rate / 44100.0, 2.0), 44100.0);   // 220 Hz once it is read at 44.1 kHz

            SessionState session;
            session.sampleRate = rate;
            auto& track = session.addTrack();
            SessionClip tone;
            tone.id = 501;
            tone.name = "Tone";
            tone.source = toneFile;
            tone.position = 96000;
            tone.offset = 24000;          // half a second in, at the session's 48 kHz
            tone.length = 48000;
            track.clips.push_back (tone);
            SessionClip beat;
            beat.id = 502;
            beat.name = "Break";
            beat.source = loopFile;
            beat.position = 240000;
            beat.offset = 0;
            beat.length = loop.getNumSamples();
            track.clips.push_back (beat);
            SessionClip none;
            none.id = 503;
            none.length = 100;
            track.clips.push_back (none);

            juce::String error, noFile, noClip;
            const auto sound = SamplerTrack::soundOf (session, 501, error);
            const auto refused = ! SamplerTrack::soundOf (session, 503, noFile).has_value() && noFile.contains ("no file") && ! SamplerTrack::soundOf (session, 999, noClip).has_value();
            const auto part = sound.has_value() && sound->file == toneFile && sound->part == juce::Range<juce::int64> (22050, 66150) && sound->name == "Tone";

            auto made = sound.has_value() ? SamplerTrack::make (*sound, false) : SamplerTrack::Made();
            auto next = session;
            const auto placed = SamplerTrack::place (next, made, 0);
            Processor fromClip;

            if (made.effects != nullptr)
            {
                const auto state = stateIn (*made.effects);
                fromClip.setStateInformation (state.getData(), (int) state.getSize());
            }

            const auto zones = fromClip.getZones();
            const auto keysTrack = made.error.isEmpty() && placed.trackId != 0 && placed.clipId == 0 && next.tracks.size() == 2 && next.tracks[1].isInstrument && next.tracks[1].name == "Tone"
                                   && ! next.tracks[1].drumGrid && zones.size() == 1 && zones[0].start == 22050 && zones[0].end == 66150 && zones[0].root == 57 && zones[0].lowKey == 0
                                   && zones[0].highKey == 127 && ! fromClip.getSlicing().on;
            const auto played = play (fromClip, { on (0.0, 57) }, 1.5);
            const auto cents = centsOff (pitchOf (played, (int) (0.3 * rate), (int) (0.4 * rate), 220.0), 220.0);

            check (part && refused && keysTrack && std::abs (cents) < 3.0 && peak (played, 1.01, 0.4) == 0.0f,
                   "sampler: a clip playing the second half-second to a second and a half of a 44.1 kHz file in a 48 kHz session is that part of the file in the file's own frames (22,050 to 66,150); "
                   "New Sampler Track from Clip makes an instrument track named after it whose sampler plays that part across the keys, its root found (A3, " + juce::String (cents, 1)
                       + " cents off at that key); a clip with no file is refused with the reason");

            // Sliced: the notes play the clip as it was, where it was, and the clip is muted.
            error.clear();
            const auto beatSound = SamplerTrack::soundOf (session, 502, error);
            made = beatSound.has_value() ? SamplerTrack::make (*beatSound, true) : SamplerTrack::Made();
            next = session;
            const auto sliced = SamplerTrack::place (next, made, 240000, 502);
            Processor slicer;

            if (made.effects != nullptr)
            {
                const auto state = stateIn (*made.effects);
                slicer.setStateInformation (state.getData(), (int) state.getSize());
            }

            const auto& newTrack = next.tracks.back();
            const auto structure = made.error.isEmpty() && sliced.trackId == newTrack.id && sliced.clipId != 0 && newTrack.isInstrument && newTrack.drumGrid
                                   && newTrack.drumMap.size() == times.size() && newTrack.drumMap[0].pitch == 36 && newTrack.noteClips.size() == 1
                                   && newTrack.noteClips[0].position == 240000 && newTrack.noteClips[0].length == loop.getNumSamples() && newTrack.noteClips[0].notes.size() == times.size()
                                   && next.tracks[0].clips[1].muted && ! next.tracks[0].clips[0].muted && ! session.tracks[0].clips[1].muted && slicer.getSlicing().on;

            // The note clip's notes through the sampler, against the clip's audio.
            std::vector<Event> part2;
            std::vector<int> cuts;

            if (structure)
                for (auto& note : newTrack.noteClips[0].notes)
                {
                    part2.push_back (on (((double) note.start + 0.01) / rate, note.pitch, note.velocity));
                    cuts.push_back ((int) note.start);
                }

            cuts.push_back (loop.getNumSamples());
            const auto rebuilt = play (slicer, part2, 4.0);
            auto apart = 0.0f;

            for (int i = 0; i < loop.getNumSamples(); ++i)
                if (std::none_of (cuts.begin(), cuts.end(), [i] (int cut) { return std::abs (i - cut) < 72; }))
                    apart = juce::jmax (apart, std::abs (rebuilt.getSample (0, i) - loop.getSample (0, i)), std::abs (rebuilt.getSample (1, i) - loop.getSample (1, i)));

            check (structure && apart < 1.0e-6f,
                   "sampler: Slice to Sampler Track makes a track showing the drum grid with a row for each of the " + juce::String (times.size())
                       + " slices and a note clip where the audio clip is and as long, and mutes the audio clip; the note clip's notes through the sampler are the clip's audio again, within "
                       + juce::String (db (apart), 1) + " dB away from the cuts (1.5 ms either side of each)");

            // A selection that is not a file as it stands: written with its markers, then sampled; sliced, it is cut at them.
            Markers markers { { 24000, 0, "Two", -1 }, { 48000, 0, "Three", -1 } };
            const auto selection = SamplerTrack::written (harmonic (330.0, 1.5), rate, "Untitled selection", markers, error, folder.getChildFile ("Samples"));
            const auto fromSelection = selection.has_value() ? SamplerTrack::make (*selection, true) : SamplerTrack::Made();

            check (selection.has_value() && selection->file.existsAsFile() && selection->file.getParentDirectory() == folder.getChildFile ("Samples") && fromSelection.error.isEmpty()
                       && fromSelection.notes.size() == 3 && fromSelection.drumMap.size() == 3 && fromSelection.drumMap[1].name == "Two"
                       && std::abs (fromSelection.notes[1].start - 0.5) < 1.0e-9 && std::abs (fromSelection.seconds - 1.5) < 1.0e-9,
                   "sampler: a selection that is not a file as it stands is written into the Samples folder with its markers, and sliced it is cut at them (Start, Two, Three)");
        }

        juce::ignoreUnused (engine);
        sampler.setZones ({});
        sampler.setSlicing ({});
        folder.deleteRecursively();
    }

    /** The factory content: every preset loads its samples and plays, loops don't click, slices sound, and the files are the ones the tool writes. */
    void testFactory (Processor& sampler)
    {
        const auto runs = Sampler::factoryFolder();      // the test run's own
        const auto folder = juce::File::createTempFile ("samplerfactory");
        const auto unpackError = Sampler::unpackFactorySamples (folder);
        Sampler::setFactoryFolder (folder);
        const auto presets = BuiltInEffects::samplerPresets();
        juce::StringArray names, problems;
        int zonesInAll = 0, loops = 0, slices = 0, kits = 0;
        auto quietest = 0.0, loudestDb = -200.0, worstLoop = 0.0;
        juce::int64 bytes = 0;

        for (const auto& found : juce::RangedDirectoryIterator (folder, true, "*.flac", juce::File::findFiles))
            bytes += found.getFile().getSize();

        for (auto& preset : presets)
        {
            names.add (preset.name);
            defaults (sampler);
            sampler.setZones ({});
            sampler.setSlicing ({});
            sampler.setPresetValues (preset.values);
            sampler.setPresetTexts (preset.texts);
            const auto zones = sampler.getZones();
            zonesInAll += (int) zones.size();

            if (zones.empty() || ! sampler.getMissing().isEmpty())
                problems.add (preset.name + ": " + (zones.empty() ? juce::String ("no zones") : sampler.getMissing().joinIntoString (", ")));

            if (sampler.getSlicing().on)
            {
                ++kits;
                slices += (int) zones.size();

                // Every slice sounds on its own key, and all of them one after another don't clip.
                for (auto& z : zones)
                {
                    const auto out = play (sampler, { on (0.0, z.lowKey) }, 0.3);

                    if (peak (out, 0.0, 0.3) < 0.003f)
                        problems.add (preset.name + ": " + z.name + " is silent");

                    loudestDb = juce::jmax (loudestDb, db (juce::jmax (peak (out, 0.0, 0.3, 0), peak (out, 0.0, 0.3, 1))));
                }

                continue;
            }

            for (auto& z : zones)
            {
                if (z.sample == nullptr)
                    continue;

                // Its root, held for six seconds (a looped zone goes round several times), then let go.
                const auto out = play (sampler, { on (0.0, z.root), off (6.0, z.root) }, 7.5);
                const auto level = db (juce::jmax (peak (out, 0.0, 6.0, 0), peak (out, 0.0, 6.0, 1)));
                loudestDb = juce::jmax (loudestDb, level);
                quietest = juce::jmin (quietest, level);

                if (level < -30.0 || level > -0.1)
                    problems.add (preset.name + ": " + z.name + " peaks at " + juce::String (level, 1) + " dB");

                // In tune with its root: on its note, in whatever octave the sound it was sampled from plays
                // (a sub sounds an octave under its key, a kalimba an octave over). Bells, whose partials
                // are not a note's, are left out.
                if (const auto found = Sampler::findPitch (*z.sample); found.found && preset.category != "Bell"
                                                                         && ((found.root - z.root) % 12 != 0 || std::abs (found.tune) > 8.0f))
                    problems.add (preset.name + ": " + z.name + " is not on its root's note (" + Instrument::pitchName (found.root) + ", " + juce::String (found.tune, 1) + " cents)");

                if (z.loop != 0)
                {
                    ++loops;
                    const auto click = ClickCheck::worst (out, rate, 0.004f, { 0 });
                    worstLoop = juce::jmax (worstLoop, (double) click.ratio);

                    if (click.ratio >= 4.0f)
                        problems.add (preset.name + ": " + z.name + "'s loop steps (" + juce::String (click.ratio, 1) + " times its surroundings at " + juce::String (click.seconds, 2) + " s)");

                    if (rms (out, 5.0, 0.9) < 0.003f)
                        problems.add (preset.name + ": " + z.name + " has died away by five seconds though it loops");
                }
            }
        }

        check (unpackError.isEmpty() && presets.size() == 11 && problems.isEmpty() && loops >= 9 && kits == 4,
               "sampler factory: the " + juce::String (presets.size()) + " presets (" + names.joinIntoString (", ") + ") hold " + juce::String (zonesInAll) + " zones over 26 samples, "
                   + juce::String ((double) bytes / (1024.0 * 1024.0), 1) + " MB; every zone's sample is there and plays, its root peaking between " + juce::String (quietest, 1) + " and "
                   + juce::String (loudestDb, 1) + " dB; the " + juce::String (loops) + " loops go round without a step (the worst jump " + juce::String (worstLoop, 2)
                   + " times its surroundings); the four sliced kits' " + juce::String (slices) + " slices each sound" + (problems.isEmpty() ? juce::String() : ". PROBLEMS: " + problems.joinIntoString ("; ")));

        // The files the app ships are the ones the tool writes.
        {
            const auto again = juce::File::createTempFile ("samplercontent");
            std::ostringstream quiet;
            auto* was = std::cout.rdbuf (quiet.rdbuf());
            writeSamplerContent (again);
            std::cout.rdbuf (was);
            Sampler::setFactoryFolder (folder);
            int compared = 0, different = 0;

            for (const auto& found : juce::RangedDirectoryIterator (again, true, "*", juce::File::findFiles))
            {
                const auto shipped = folder.getChildFile (found.getFile().getRelativePathFrom (again));
                ++compared;
                different += shipped.existsAsFile() && shipped.hasIdenticalContentTo (found.getFile()) ? 0 : 1;
            }

            check (compared == 27 && different == 0 && ! quiet.str().empty() && juce::String (quiet.str()).contains ("26 samples") && ! juce::String (quiet.str()).contains ("FAILED"),
                   "sampler factory: the tool that renders the content from Torqwave's own instruments writes 26 samples and the manifest again, and all " + juce::String (compared)
                       + " are byte for byte the files the app ships (" + juce::String (different) + " different)");
            again.deleteRecursively();
        }

        // A factory preset's samples are named by their place among the factory samples, so its state works wherever they are unpacked.
        {
            defaults (sampler);
            sampler.setPresetValues (presets[2].values);
            sampler.setPresetTexts (presets[2].texts);
            juce::MemoryBlock state;
            sampler.getStateInformation (state);
            const auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), (int) state.getSize());
            const auto named = xml != nullptr && xml->getChildByName ("ZONES") != nullptr && xml->getChildByName ("ZONES")->getChildByName ("ZONE") != nullptr
                                   ? xml->getChildByName ("ZONES")->getChildByName ("ZONE")->getStringAttribute ("file") : juce::String();
            const auto before = play (sampler, { on (0.0, 48), off (1.0, 48) }, 1.5);

            const auto elsewhere = juce::File::createTempFile ("samplerfactory2");
            Sampler::unpackFactorySamples (elsewhere);
            Sampler::setFactoryFolder (elsewhere);
            Processor other;
            other.setStateInformation (state.getData(), (int) state.getSize());
            const auto there = ! other.getZones().empty() && other.getZones()[0].sample != nullptr && other.getZones()[0].file.isAChildOf (elsewhere) && other.getMissing().isEmpty();
            const auto same2 = same (before, play (other, { on (0.0, 48), off (1.0, 48) }, 1.5));
            Sampler::setFactoryFolder (folder);

            // Ask Torq's list of factory sounds, and the presets row stepping onto one.
            const auto listed = Ai::FactorySounds::of (BuiltInEffects::samplerDescription());
            const auto asState = Ai::FactorySounds::stateOf (BuiltInEffects::samplerDescription(), "Cold Bell");
            const auto noSuch = Ai::FactorySounds::stateOf (BuiltInEffects::samplerDescription(), "No Such Sound");
            Processor loaded;
            const auto byName = loaded.loadFactory ("Dark Techno Break") && loaded.getSlicing().on && loaded.getZones().size() > 8 && ! loaded.loadFactory ("No Such Sound");

            Processor stepped;
            std::unique_ptr<juce::AudioProcessorEditor> made (stepped.createEditorAndMakeActive());
            auto steppedOn = false;

            if (auto* row = made != nullptr ? InstrumentKeys::presetsRow (*made) : nullptr)
            {
                row->step (1);
                steppedOn = ! stepped.getZones().empty() && stepped.getZones()[0].sample != nullptr;
            }

            stepped.editorBeingDeleted (made.get());
            made.reset();

            check (named.startsWith ("factory:Strings/") && there && same2 && listed.size() == 11 && listed[0].category == "Keys" && asState.has_value() && ! noSuch.has_value() && byName && steppedOn
                       && InstrumentKeys::kindFor (loaded) == InstrumentKeys::Kind::kit,
                   "sampler factory: a factory preset's state names its samples by their place among the factory samples (" + named + ") and plays the same with them unpacked somewhere else; "
                   "Ask Torq is given the eleven by name and category and a state for one; a preset is loaded by name; the presets row's next arrow puts one on the keys");
            elsewhere.deleteRecursively();
        }

        Sampler::setFactoryFolder (runs);
        sampler.setZones ({});
        sampler.setSlicing ({});
        defaults (sampler);

        folder.deleteRecursively();
    }
}

void saveSamplerWindow (const juce::File& folder)
{
    const auto sounds = juce::File::createTempFile ("samplerpictures");
    sounds.createDirectory();
    std::vector<double> times;
    const auto loopFile = writeWav (sounds.getChildFile ("Break.wav"), drumLoop (times), rate);

    // A held note with its attack and a slow fall, looped.
    auto note = harmonic (110.0, 3.0);

    for (int i = 0; i < note.getNumSamples(); ++i)
        note.setSample (0, i, note.getSample (0, i) * (float) (juce::jmin (1.0, i / 300.0) * (0.55 + 0.45 * std::exp (-i / (0.6 * rate)))));

    const auto noteFile = writeWav (sounds.getChildFile ("Held Note.wav"), note, rate);

    for (const auto slices : { false, true })
    {
        Processor sampler;
        sampler.prepareToPlay (rate, 512);
        std::unique_ptr<juce::AudioProcessorEditor> made (sampler.createEditorAndMakeActive());
        auto* window = dynamic_cast<SamplerEditor*> (made.get());

        if (window == nullptr)
            return;

        if (slices)
        {
            window->loadFile (loopFile);
            window->setSlices (true);
            window->select (4);
            sampler.play (40, 1.0f);
        }
        else
        {
            window->loadFile (noteFile);
            window->findLoop();
            window->getKeyMap().dragEdge (0, SamplerEditor::KeyMap::Edge::low, 24);
            window->getKeyMap().dragEdge (0, SamplerEditor::KeyMap::Edge::high, 84);
            sampler.play (45, 1.0f);
        }

        for (int i = 0; i < (slices ? 4 : 60); ++i)
            runBlock (sampler);

        window->refreshNow();

        for (auto theme : { "Dark", "Light" })
        {
            Theme::apply (Theme::make (theme, "Blue"));
            const auto image = made->createComponentSnapshot (made->getLocalBounds(), true, 2.0f);
            auto file = folder.getChildFile (juce::String ("sampler-") + (slices ? "slices-" : "keys-") + theme + ".png");
            file.deleteFile();
            juce::FileOutputStream out (file);
            juce::PNGImageFormat().writeImageToStream (image, out);
            std::cout << file.getFullPathName() << std::endl;
        }

        sampler.editorBeingDeleted (made.get());
        made.reset();
    }

    sounds.deleteRecursively();
}

int trySamplerFolder (const juce::File& folder)
{
    juce::Array<juce::File> files;
    folder.findChildFiles (files, juce::File::findFiles, true, "*.wav;*.wave;*.bwf;*.aif;*.aiff;*.aifc;*.flac;*.ogg;*.opus;*.mp3;*.m4a;*.caf");
    files.sort();

    if (files.isEmpty())
    {
        std::cout << "No sounds in " << folder.getFullPathName() << std::endl;
        return 1;
    }

    Processor sampler;
    int failed = 0;

    // The level of a note at a zone's root, held for the sound's length (two seconds at most).
    const auto levelOf = [&sampler] (const Sampler::Zone& z)
    {
        const auto seconds = juce::jlimit (0.1, 2.0, z.sample != nullptr ? (double) z.sample->getNumFrames() / z.sample->rate : 0.1);
        const auto out = play (sampler, { on (0.0, z.root) }, seconds);
        return juce::Decibels::gainToDecibels (out.getMagnitude (0, out.getNumSamples()), -120.0f);
    };

    for (auto& f : files)
    {
        juce::String error;
        const auto sample = Sampler::sharedSample (f, error);

        if (sample == nullptr)
        {
            std::cout << "REFUSED  " << f.getFileName() << ": " << error << std::endl;
            continue;
        }

        // As the first sound of an empty sampler, then in place of another sound, as a drop on the waveform does.
        plain (sampler);
        sampler.setSlicing ({});
        sampler.setZones ({});
        const auto first = sampler.addSample (sample, f);
        const auto asFirst = first.isEmpty() && sampler.getZones().size() == 1 ? levelOf (sampler.getZones()[0]) : -120.0f;

        oneZone (sampler, sine (220.0, 1.0), 57);
        const auto second = sampler.addSample (sample, f, sampler.getZones()[0].id);
        const auto zones = sampler.getZones();
        const auto asSecond = second.isEmpty() && zones.size() == 1 && zones[0].sample == sample ? levelOf (zones[0]) : -120.0f;

        // Sliced at its hits: each slice's key sounds.
        Sampler::Slicing how;
        how.on = true;
        how.source = Sampler::SliceSource::hits;
        const auto sliced = sampler.slice (sample, f, {}, how);
        const auto slices = sliced ? (int) sampler.getZones().size() : 0;
        int silent = 0;

        for (auto& z : sampler.getZones())
            silent += levelOf (z) < -60.0f ? 1 : 0;

        const auto peak = juce::Decibels::gainToDecibels (sample->audio.getMagnitude (0, sample->audio.getNumSamples()), -120.0f);
        const auto ok = first.isEmpty() && second.isEmpty() && asFirst > peak - 12.0f && asSecond > peak - 12.0f && std::abs (asFirst - asSecond) < 0.1f;
        failed += ok ? 0 : 1;
        const auto& z = zones[0];

        std::cout << (ok ? "PASS  " : "FAIL  ") << f.getFileName() << ": " << juce::String ((double) sample->getNumFrames() / sample->rate, 2) << " s, "
                  << sample->audio.getNumChannels() << " ch at " << sample->rate << " Hz, peak " << juce::String (peak, 1) << " dB; root " << Instrument::pitchName (z.root)
                  << (sample->fileRoot >= 0 ? " (the file's)" : " (found)") << ", tune " << juce::String (z.tune, 1) << " ct, loop " << z.loop
                  << "; at its root " << juce::String (asFirst, 1) << " dB as the first sound, " << juce::String (asSecond, 1) << " dB in place of another; "
                  << slices << " slices at its hits" << (silent > 0 ? ", " + juce::String (silent) + " of them silent" : juce::String())
                  << (first.isNotEmpty() ? "; " + first : juce::String()) << (second.isNotEmpty() ? "; " + second : juce::String()) << std::endl;
    }

    std::cout << (failed == 0 ? "ALL PASSED" : juce::String (failed) + " FAILED") << std::endl;
    return failed == 0 ? 0 : 1;
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
    testSlices (*sampler);
    testFiles (engine, *sampler);
    testWindow (*sampler);
    testGettingSoundIn (engine, *sampler);
    testFactory (*sampler);

    defaults (*sampler);
    sampler->setZones ({});
}
