#include "Release024Tests.h"
#include "../Source/MpcKit.h"
#include "../Source/Sf2Import.h"
#include "../Source/DrumKitImport.h"
#include "../Source/CreativeEffects.h"
#include "TestUtils.h"
#include <iostream>

namespace
{
    using TestUtils::check;

    void writeSineWav (const juce::File& file, double hz = 220.0, int frames = 4800)
    {
        file.getParentDirectory().createDirectory();
        juce::WavAudioFormat wav;
        if (auto out = file.createOutputStream())
        {
            std::unique_ptr<juce::OutputStream> os (out.release());
            if (auto writer = wav.createWriterFor (os, juce::AudioFormatWriterOptions().withSampleRate (48000.0).withNumChannels (1).withBitsPerSample (16)))
            {
                juce::AudioBuffer<float> buf (1, frames);
                for (int i = 0; i < frames; ++i)
                    buf.setSample (0, i, 0.5f * std::sin (2.0f * juce::MathConstants<float>::pi * (float) hz * (float) i / 48000.0f));
                writer->writeFromAudioSampleBuffer (buf, 0, frames);
            }
        }
    }

    void testMpcKit()
    {
        MpcKit::Program prog;
        prog.name = "MPC Industrial Test";
        prog.isKeygroup = false;

        MpcKit::Instrument kick;
        kick.number = 1;
        kick.lowNote = kick.highNote = 36;
        kick.muteGroup = 0;
        kick.filterType = 1;
        kick.cutoff = 0.75f;
        kick.resonance = 0.25f;
        kick.hasEnvelope = true;
        kick.attack = 0.01f;
        kick.decay = 0.35f;
        kick.layers.push_back ({ "MPC Kick Soft", "", 1, 70, 1.0f, 0.5f, 0.0f, 0.0f, 36, 0, 4800, false, false, 0, 0 });
        kick.layers.push_back ({ "MPC Kick Hard", "", 71, 127, 1.0f, 0.5f, 0.0f, 0.0f, 36, 0, 4800, false, false, 0, 0 });
        prog.instruments.push_back (kick);

        MpcKit::Instrument hat;
        hat.number = 2;
        hat.lowNote = hat.highNote = 42;
        hat.muteGroup = 1;
        hat.layers.push_back ({ "MPC Hat", "", 1, 127, 0.9f, 0.6f, 2.0f, 0.0f, 42, 0, 4800, true, false, 0, 0 });
        prog.instruments.push_back (hat);

        const auto xml = MpcKit::writeXml (prog);
        MpcKit::Program back;
        check (MpcKit::parse (xml, back).isEmpty() && back.name == "MPC Industrial Test"
                   && back.instruments.size() == 2 && back.instruments[0].layers.size() == 2
                   && back.instruments[1].muteGroup == 1 && back.instruments[1].layers[0].reverse,
               "0.24 mpc kit: Drum .xpm round-trips cleanly with multi-velocity layers, mute group, filter and reverse");

        MpcKit::Program bad;
        check (MpcKit::parse ("<NotMpc/>", bad).isNotEmpty() && MpcKit::parse ("", bad).isNotEmpty(),
               "0.24 mpc kit: invalid XML is rejected cleanly");

        const auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("torqwave-mpc-test");
        tmp.deleteRecursively();
        const auto xpm = tmp.getChildFile ("MPC Industrial Test.xpm");
        xpm.getParentDirectory().createDirectory();
        xpm.replaceWithText (xml);
        writeSineWav (tmp.getChildFile ("MPC Kick Soft.wav"), 90.0);
        writeSineWav (tmp.getChildFile ("MPC Kick Hard.wav"), 120.0);
        writeSineWav (tmp.getChildFile ("Samples/MPC Hat.wav"), 6000.0);

        auto plan = DrumKitImport::fromMpc (back, xpm);
        DrumKitImport::place (plan);
        check (plan.missing.isEmpty() && plan.pads.size() == 2,
               "0.24 mpc kit: resolves samples beside .xpm and in Samples/ subfolder");

        BuiltInEffects::BasicDrumsProcessor drums;
        drums.prepareToPlay (48000.0, 256);
        check (DrumKitImport::apply (drums, plan).isEmpty()
                   && drums.getType (DrumMachine::Sound::kick) == DrumMachine::sampleType
                   && drums.filterParameter (DrumMachine::Sound::kick)->getIndex() == 1
                   && drums.getEffects ((int) DrumMachine::Sound::kick).on (DrumMachine::envOn)
                   && drums.getChoke ((int) DrumMachine::Sound::closedHat) == 1,
               "0.24 mpc kit: applies onto Basic Drums with filter, volume envelope, and choke group");

        // Keygroup -> Torqwave Sampler
        prog.isKeygroup = true;
        prog.instruments[0].lowNote = 36;
        prog.instruments[0].highNote = 60;
        const auto kgXpm = tmp.getChildFile ("MPC Keygroup.xpm");
        kgXpm.replaceWithText (MpcKit::writeXml (prog));

        BuiltInEffects::SamplerProcessor sampler;
        sampler.prepareToPlay (48000.0, 256);
        check (sampler.loadMpcKeygroup (kgXpm).isEmpty() && sampler.getZones().size() == 3,
               "0.24 mpc keygroup: loads directly into Torqwave Sampler zones");

        tmp.deleteRecursively();
    }

    void testSf2Import()
    {
        std::vector<int16_t> pcm (4410);
        for (size_t i = 0; i < pcm.size(); ++i)
            pcm[i] = (int16_t) std::lround (20000.0 * std::sin (2.0 * juce::MathConstants<double>::pi * 440.0 * (double) i / 44100.0));

        const auto sf2 = Sf2Import::writeMinimal ("Test Lead", pcm, 44100, 69, 48, 84, 1, 127, true, 400, 4000);
        const auto res = Sf2Import::parse (sf2.getData(), sf2.getSize(), 0);

        check (res.ok() && res.presetName == "Test Lead" && res.zones.size() == 1
                   && res.zones[0].lowKey == 48 && res.zones[0].highKey == 84 && res.zones[0].root == 69
                   && res.zones[0].loop == 1
                   && res.zones[0].loopStart == 400 && res.zones[0].loopEnd == 4000,
               "0.24 sf2 import: RIFF sfbk parses preset, key range, root key, loop points, and PCM audio");

        const auto bad = Sf2Import::parse ("not sf2", 7);
        check (! bad.ok() && bad.error.isNotEmpty(), "0.24 sf2 import: non-SF2 data is rejected cleanly");

        // Load into SamplerProcessor and render a note!
        const auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("torqwave-sf2-test.sf2");
        tmp.replaceWithData (sf2.getData(), sf2.getSize());

        BuiltInEffects::SamplerProcessor sampler;
        sampler.prepareToPlay (48000.0, 256);
        check (sampler.loadSf2 (tmp).isEmpty() && sampler.getZones().size() == 1,
               "0.24 sf2 import: loads directly into Torqwave Sampler");

        juce::AudioBuffer<float> audio (2, 1024);
        audio.clear();
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 69, (juce::uint8) 100), 0);
        sampler.processBlock (audio, midi);
        const float peak = audio.getMagnitude (0, 1024);
        check (std::isfinite (peak) && peak > 0.05f,
               "0.24 sf2 import: loaded SoundFont zone renders clean audio (peak " + juce::String (peak, 3) + ")");
        tmp.deleteFile();
    }

    void testCreativeFx024()
    {
        // 1. Frequency Shifter: verify Hilbert single-sideband frequency shift (+200 Hz moves a 400 Hz tone toward 600 Hz!)
        {
            BuiltInEffects::FrequencyShifterProcessor fs;
            fs.prepareToPlay (48000.0, 512);
            if (auto* p = dynamic_cast<juce::AudioParameterFloat*> (fs.getParameters()[1]))
                BuiltInEffects::setExactly (*p, 200.0f);

            const int N = 4096;
            juce::AudioBuffer<float> buf (2, N);
            for (int i = 0; i < N; ++i)
            {
                const float s = 0.5f * std::sin (2.0f * juce::MathConstants<float>::pi * 400.0f * (float) i / 48000.0f);
                buf.setSample (0, i, s);
                buf.setSample (1, i, s);
            }

            juce::MidiBuffer empty;
            fs.processBlock (buf, empty);

            // Correlate second half against 600 Hz (upper sideband) vs 200 Hz (suppressed lower sideband)
            double c600 = 0.0, s600 = 0.0, c200 = 0.0, s200 = 0.0;
            for (int i = 2048; i < N; ++i)
            {
                const double x = buf.getSample (0, i);
                const double t = (double) i / 48000.0;
                c600 += x * std::cos (2.0 * juce::MathConstants<double>::pi * 600.0 * t);
                s600 += x * std::sin (2.0 * juce::MathConstants<double>::pi * 600.0 * t);
                c200 += x * std::cos (2.0 * juce::MathConstants<double>::pi * 200.0 * t);
                s200 += x * std::sin (2.0 * juce::MathConstants<double>::pi * 200.0 * t);
            }
            const double mag600 = std::hypot (c600, s600);
            const double mag200 = std::hypot (c200, s200);
            check (std::isfinite (mag600) && mag600 > mag200 * 10.0 && mag600 > 100.0,
                   "0.24 frequency shifter: Hilbert SSB shifts 400 Hz + 200 Hz to 600 Hz with >20 dB image rejection (600Hz="
                       + juce::String (mag600, 1) + ", 200Hz=" + juce::String (mag200, 1) + ")");
        }

        // 2. Shimmer Delay: verify pitch-shifted wet tail rings after an impulse
        {
            BuiltInEffects::ShimmerDelayProcessor sd;
            sd.prepareToPlay (48000.0, 512);
            if (auto* syncP = dynamic_cast<juce::AudioParameterBool*> (sd.getParameters()[0]))
                *syncP = false;
            if (auto* timeP = dynamic_cast<juce::AudioParameterFloat*> (sd.getParameters()[2]))
                BuiltInEffects::setExactly (*timeP, 40.0f);

            juce::AudioBuffer<float> buf (2, 8192);
            buf.clear();
            for (int i = 0; i < 480; ++i)
            {
                const float s = 0.6f * std::sin (2.0f * juce::MathConstants<float>::pi * 440.0f * (float) i / 48000.0f);
                buf.setSample (0, i, s);
                buf.setSample (1, i, s);
            }

            juce::MidiBuffer empty;
            sd.processBlock (buf, empty);
            const float tail = buf.getMagnitude (2400, 4000);
            check (std::isfinite (tail) && tail > 0.02f && tail < 2.0f,
                   "0.24 shimmer delay: produces finite pitch-shifted wet echo tail (peak " + juce::String (tail, 3) + ")");
        }
    }
}

void runRelease024Tests()
{
    std::cout << "Release 0.24.0 features:" << std::endl;
    testMpcKit();
    testSf2Import();
    testCreativeFx024();
}
