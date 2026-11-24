#include <JuceHeader.h>
#include "../Source/Maximizer.h"
#include "../Source/BuiltInEffects.h"
#include "TestUtils.h"
#include "MaximizerTests.h"

namespace
{
    using TestUtils::check;

    constexpr double rate = 48000.0;

    juce::AudioBuffer<float> readAll (const AudioSnapshot& s)
    {
        juce::AudioBuffer<float> b (s.getNumChannels(), (int) s.getNumFrames());
        s.read (b, 0, 0, b.getNumSamples());
        return b;
    }

    juce::AudioBuffer<float> applied (const juce::AudioBuffer<float>& in, const Maximizer::Settings& s)
    {
        return readAll (*Maximizer::apply (*AudioEdits::fromBuffer (juce::AudioBuffer<float> (in)), 0, in.getNumSamples(), rate, s));
    }

    /** A second of hits: a 1 kHz tone at -14 dB with a 3 ms burst at full scale every 125 ms. */
    juce::AudioBuffer<float> hits()
    {
        juce::AudioBuffer<float> b (2, 48000);

        for (int i = 0; i < 48000; ++i)
        {
            const auto tone = 0.2 * std::sin (juce::MathConstants<double>::twoPi * 1000.0 * i / rate);
            const auto burst = i % 6000 < 144 ? 0.8 * std::sin (juce::MathConstants<double>::twoPi * 3000.0 * i / rate) : 0.0;
            b.setSample (0, i, (float) (tone + burst));
            b.setSample (1, i, (float) (tone + burst));
        }

        return b;
    }

    double peakOf (const juce::AudioBuffer<float>& b)
    {
        return (double) b.getMagnitude (0, b.getNumSamples());
    }

    double rmsDb (const juce::AudioBuffer<float>& b, int from, int to)
    {
        double sum = 0.0;

        for (int i = from; i < to; ++i)
            sum += (double) b.getSample (0, i) * b.getSample (0, i);

        return 10.0 * std::log10 (sum / (to - from));
    }
}

void runMaximizerTests()
{
    std::cout << "Maximizer:" << std::endl;

    // The ceiling: a loud sine driven 12 dB comes out with no sample over the ceiling and its true
    // peak at it or a hair under (the second stage's margin), for every character; so does one that
    // starts loud at once, which the first stage alone lets over through its first look-ahead.
    {
        juce::AudioBuffer<float> in (2, 48000), abrupt (2, 48000);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 48000; ++i)
            {
                const auto v = 0.9 * std::sin (juce::MathConstants<double>::twoPi * 997.0 * i / rate + ch);
                in.setSample (ch, i, (float) (v * juce::jmin (1.0, i / 2400.0)));
                abrupt.setSample (ch, i, (float) v);
            }

        auto held = true, under = true;
        auto worstOver = -100.0, worstStart = -100.0;
        const auto top = Dynamics::ceilingGain (-1.0);

        for (auto c : { Maximizer::Character::transparent, Maximizer::Character::balanced, Maximizer::Character::loud })
        {
            Maximizer::Settings s;
            s.gainDb = 12.0;
            s.ceilingDb = -1.0;
            s.character = c;
            const auto out = applied (in, s);
            under = under && peakOf (out) <= top && peakOf (applied (abrupt, s)) <= top;
            const auto truePeak = Dynamics::truePeakDb (*AudioEdits::fromBuffer (juce::AudioBuffer<float> (out)), 0, 48000, rate);
            worstOver = juce::jmax (worstOver, truePeak + 1.0);
            worstStart = juce::jmax (worstStart, Dynamics::truePeakDb (*AudioEdits::fromBuffer (applied (abrupt, s)), 0, 48000, rate) + 1.0);
            held = held && truePeak + 1.0 <= 0.0 && truePeak + 1.0 > -0.05;
        }

        std::cout << "      ceiling -1 dBTP: the true peak from it by at most " << worstOver << " dB (a sound starting loud at once: " << worstStart << " dB)" << std::endl;
        check (under && held && worstStart <= 0.0,
               "maximizer: a 0.9 sine faded in and driven 12 dB into a -1 dBTP ceiling comes out with no sample over the ceiling and its true peak at "
               "the ceiling or under it by less than 0.05 dB, for Transparent, Balanced and Loud; one that starts loud at once is not over it either");
    }

    // Drums: bursts of noise with a low thump under them every 125 ms, the kind of sound whose
    // peaks between samples the moving gain and the clip remake (the first stage alone: 0.3 dB over).
    // Driven 4 to 16 dB, the true peak, as the loudness meter measures it, is not over the ceiling.
    {
        juce::AudioBuffer<float> in (2, 96000);
        juce::Random random (20);

        for (int i = 0; i < 96000; ++i)
        {
            const auto t = (i % 6000) / rate;
            const auto thump = 0.5 * std::exp (-t * 30.0) * std::sin (juce::MathConstants<double>::twoPi * 55.0 * t);
            const auto noise = 0.35 * std::exp (-t * 60.0);
            in.setSample (0, i, (float) (thump + noise * (random.nextDouble() * 2.0 - 1.0)));
            in.setSample (1, i, (float) (thump + noise * (random.nextDouble() * 2.0 - 1.0)));
        }

        auto worst = -100.0;
        auto under = true;

        for (auto c : { Maximizer::Character::transparent, Maximizer::Character::balanced, Maximizer::Character::loud })
            for (const auto gain : { 4.0, 8.0, 12.0, 16.0 })
                for (const auto emphasis : { 0.0, 100.0 })
                {
                    Maximizer::Settings s;
                    s.gainDb = gain;
                    s.ceilingDb = -1.0;
                    s.character = c;
                    s.emphasisPercent = emphasis;
                    const auto out = applied (in, s);
                    under = under && peakOf (out) <= Dynamics::ceilingGain (-1.0);
                    worst = juce::jmax (worst, Dynamics::truePeakDb (*AudioEdits::fromBuffer (juce::AudioBuffer<float> (out)), 0, 96000, rate) + 1.0);
                }

        std::cout << "      noise bursts over a thump, driven 4 to 16 dB: the true peak from a -1 dBTP ceiling by at most " << worst << " dB" << std::endl;
        check (under && worst <= 0.0,
               "maximizer: two seconds of noise bursts over a thump, driven 4, 8, 12 and 16 dB into a -1 dBTP ceiling with and without Transient "
               "Emphasis, come out with no sample and no true peak over the ceiling, for every character");
    }

    // The second stage's detector reads what the meter reads: over a second of white noise, the
    // largest peak it gives is the true peak Dynamics measures.
    {
        juce::AudioBuffer<float> noise (1, 48000);
        juce::Random random (7);

        for (int i = 0; i < 48000; ++i)
            noise.setSample (0, i, (float) (0.5 * (random.nextDouble() * 2.0 - 1.0)));

        SampleRateConverter::PeakDetector detector;
        auto peak = 0.0;

        for (int i = 0; i < 48000 + SampleRateConverter::PeakDetector::latency(); ++i)
            peak = juce::jmax (peak, detector.tick (i < 48000 ? noise.getSample (0, i) : 0.0f));

        const auto metered = Dynamics::truePeakDb (*AudioEdits::fromBuffer (juce::AudioBuffer<float> (noise)), 0, 48000, rate);
        const auto apart = 20.0 * std::log10 (peak) - metered;
        std::cout << "      peak detector: " << apart << " dB from the meter's true peak on white noise" << std::endl;
        check (std::abs (apart) < 0.0001,
               "peak detector: over a second of white noise, a frame at a time, its largest peak is within 0.0001 dB of the true peak the meter "
               "measures (the same filter, at four times the rate)");
    }

    // A quiet sound with no gain passes bit for bit, in place (the latency taken off).
    {
        juce::AudioBuffer<float> in (2, 24000);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 24000; ++i)
                in.setSample (ch, i, (float) (0.1 * std::sin (juce::MathConstants<double>::twoPi * 440.0 * i / rate + ch)));

        Maximizer::Settings s;
        s.emphasisPercent = 0.0;
        const auto out = applied (in, s);
        auto same = true;

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 24000; ++i)
                same = same && juce::exactlyEqual (out.getSample (ch, i), in.getSample (ch, i));

        check (same, "maximizer: a quiet sound with no gain passes bit for bit, where it was");
    }

    // Character and emphasis on a second of hits over a tone: Loud comes out louder than Balanced
    // than Transparent (the gain recovers faster); with the emphasis up, the hits demand more of
    // the limiter, so the tone right after each hit comes out quieter.
    {
        const auto in = hits();
        Maximizer::Settings s;
        s.gainDb = 10.0;
        s.ceilingDb = -0.5;
        std::array<double, 3> loudness {};
        int k = 0;

        for (auto c : { Maximizer::Character::transparent, Maximizer::Character::balanced, Maximizer::Character::loud })
        {
            s.character = c;
            loudness[(size_t) k++] = rmsDb (applied (in, s), 6000, 48000);
        }

        s.character = Maximizer::Character::balanced;
        const auto plain = applied (in, s);
        s.emphasisPercent = 100.0;
        const auto punched = applied (in, s);
        auto bodyPlain = 0.0, bodyPunched = 0.0;

        for (int hit = 1; hit < 8; ++hit)   // 20 to 60 ms after each hit's start
        {
            bodyPlain += rmsDb (plain, hit * 6000 + 960, hit * 6000 + 2880);
            bodyPunched += rmsDb (punched, hit * 6000 + 960, hit * 6000 + 2880);
        }

        bodyPlain /= 7.0;
        bodyPunched /= 7.0;
        std::cout << "      hits over a tone: transparent " << loudness[0] << " dB RMS, balanced " << loudness[1] << ", loud " << loudness[2]
                  << "; the tone after a hit " << bodyPlain << " dB with no emphasis, " << bodyPunched << " with full" << std::endl;
        check (loudness[2] > loudness[1] && loudness[1] > loudness[0] && bodyPunched < bodyPlain - 0.5 && peakOf (punched) <= Dynamics::ceilingGain (-0.5),
               "maximizer: on hits over a tone, Loud comes out louder than Balanced, and Balanced than Transparent; with Transient Emphasis at 100 % the "
               "tone 20 to 60 ms after each hit comes out over 0.5 dB quieter than with none (the hit stands further over it), the ceiling still held");
    }

    // Live, in uneven blocks, late by the latency: the offline samples bit for bit; the latency
    // reported and following the character; state and presets; refused settings.
    {
        const auto in = hits();
        Maximizer::Settings s;
        s.gainDb = 8.0;
        s.ceilingDb = -1.0;
        s.character = Maximizer::Character::loud;
        s.emphasisPercent = 40.0;

        BuiltInEffects::MaximizerProcessor live;
        live.setSettings (s);
        live.setPlayConfigDetails (2, 2, rate, 512);
        live.prepareToPlay (rate, 3000);
        const auto latency = Maximizer::latencyFrames (s.character, rate);
        juce::AudioBuffer<float> played (2, 48000 + latency);
        played.clear();

        for (int ch = 0; ch < 2; ++ch)
            played.copyFrom (ch, 0, in, ch, 0, 48000);

        juce::MidiBuffer midi;
        const int sizes[] = { 1, 7, 512, 3000 };

        for (int pos = 0, k = 0; pos < played.getNumSamples(); ++k)
        {
            const auto n = juce::jmin (sizes[k % 4], played.getNumSamples() - pos);
            juce::AudioBuffer<float> block (played.getArrayOfWritePointers(), 2, pos, n);
            live.processBlock (block, midi);
            pos += n;
        }

        const auto offline = applied (in, live.getSettings());
        auto same = true;

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 48000; ++i)
                same = same && juce::exactlyEqual (played.getSample (ch, i + latency), offline.getSample (ch, i));

        const auto reportedLoud = live.getLatencySamples();
        auto wider = s;
        wider.character = Maximizer::Character::transparent;
        live.setSettings (wider);
        juce::AudioBuffer<float> one (2, 64);
        one.clear();
        live.processBlock (one, midi);
        const auto reportedTransparent = live.getLatencySamples();

        juce::MemoryBlock state;
        live.getStateInformation (state);
        BuiltInEffects::MaximizerProcessor copy, fromPreset;
        copy.setStateInformation (state.getData(), (int) state.getSize());
        fromPreset.setPresetValues (live.getPresetValues());
        const auto values = [] (const Maximizer::Settings& x) { return std::array<double, 4> { x.gainDb, x.ceilingDb, (double) x.character, x.emphasisPercent }; };
        const auto kept = values (copy.getSettings()) == values (live.getSettings()) && values (fromPreset.getSettings()) == values (live.getSettings());

        auto tooLoud = s;
        tooLoud.gainDb = 30.0;
        auto tooLow = s;
        tooLow.ceilingDb = -30.0;
        const auto snap = AudioEdits::fromBuffer (juce::AudioBuffer<float> (in));
        const auto refused = Maximizer::apply (*snap, 0, 48000, rate, tooLoud) == nullptr && Maximizer::apply (*snap, 0, 48000, rate, tooLow) == nullptr
                             && Maximizer::apply (*snap, 100, 100, rate, s) == nullptr;

        std::cout << "      live: latency " << reportedLoud << " frames as Loud, " << reportedTransparent << " as Transparent" << std::endl;
        check (same && reportedLoud == latency && reportedTransparent == Maximizer::latencyFrames (Maximizer::Character::transparent, rate) && kept && refused,
               "built-in maximizer: played in blocks of 1, 7, 512 and 3,000 frames it gives the offline samples bit for bit, late by its latency, which it "
               "reports and changes with the character (Loud 267 frames, Transparent 651 at 48 kHz); its state and presets restore every setting; a gain "
               "past 24 dB, a ceiling under -24 dB and an empty range are refused");
    }
}
