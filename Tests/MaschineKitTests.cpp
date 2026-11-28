#include "MaschineKitTests.h"
#include "../Source/MaschineKit.h"
#include "../Source/DrumKitImport.h"
#include "TestUtils.h"
#include <iostream>

namespace
{
    using TestUtils::check;

    MaschineKit::Kit syntheticKit()
    {
        MaschineKit::Kit kit;
        kit.name = "Synthetic Industrial";
        kit.isGroup = true;

        // Pad 0: Clean-room Kick Drum Synth + Saturator + 4-band EQ + Metaverb
        {
            MaschineKit::Sound s;
            s.index = 0;
            s.name = "Kick Synth";
            s.level = 0.72f;
            s.pan = 0.0f;

            MaschineKit::Module kick;
            kick.kind = MaschineKit::ModuleKind::kick;
            kick.intParams = { 1 }; // Tronic -> fmKick
            kick.floatParams = { 0.55f, 0.48f, 0.70f, 0.60f };
            s.modules.push_back (kick);

            MaschineKit::Module sat;
            sat.kind = MaschineKit::ModuleKind::saturator;
            sat.intParams = { 2 }; // Tube
            sat.floatParams = { 0.45f, 0.50f, 0.80f };
            s.modules.push_back (sat);

            MaschineKit::Module eq;
            eq.kind = MaschineKit::ModuleKind::eq;
            eq.floatParams = { 0.30f, 0.25f, 0.45f, -0.20f, 0.65f, 0.15f, 0.70f, 0.30f, 0.50f, 0.50f };
            s.modules.push_back (eq);

            MaschineKit::Module verb;
            verb.kind = MaschineKit::ModuleKind::metaverb;
            verb.floatParams = { 0.30f, 0.55f };
            s.modules.push_back (verb);

            kit.sounds.push_back (std::move (s));
        }

        // Pad 1: Multi-zone Sampler (2 velocity layers) + TransientDesigner + LoFi + BeatDelay
        {
            MaschineKit::Sound s;
            s.index = 1;
            s.name = "Snare Layered";
            s.level = 0.68f;
            s.pan = -0.15f;

            MaschineKit::Module samp;
            samp.kind = MaschineKit::ModuleKind::sampler;
            samp.samplerFilterMode = 1; // LP
            samp.samplerCutoff = 0.75f;
            samp.samplerResonance = 0.20f;
            samp.samplerPitch = 0.50f;

            MaschineKit::Zone z1;
            z1.relativePath = "Samples/Drums/Snare Soft.wav";
            z1.lowVelocity = 1;
            z1.highVelocity = 70;
            z1.endFrame = 4799;
            z1.totalFrames = 4800;
            samp.zones.push_back (z1);

            MaschineKit::Zone z2;
            z2.relativePath = "Samples/Drums/Snare Hard.wav";
            z2.lowVelocity = 71;
            z2.highVelocity = 127;
            z2.endFrame = 4799;
            z2.totalFrames = 4800;
            samp.zones.push_back (z2);
            s.modules.push_back (samp);

            MaschineKit::Module trans;
            trans.kind = MaschineKit::ModuleKind::transientDesigner;
            trans.floatParams = { 0.0f, 0.35f, -0.25f };
            s.modules.push_back (trans);

            MaschineKit::Module lofi;
            lofi.kind = MaschineKit::ModuleKind::lofi;
            lofi.floatParams = { 0.30f, 1.0f, 0.20f, 0.80f };
            s.modules.push_back (lofi);

            MaschineKit::Module delay;
            delay.kind = MaschineKit::ModuleKind::beatDelay;
            delay.intParams = { 2, 1 };
            delay.floatParams = { 0.25f, 0.45f, 0.50f, 0.25f, 0.35f };
            s.modules.push_back (delay);

            kit.sounds.push_back (std::move (s));
        }

        // Pad 2 & Pad 3: Closed + Open Hi-Hat synths (should auto-choke in group 1)
        {
            MaschineKit::Sound ch;
            ch.index = 2;
            ch.name = "ClosedHH";
            MaschineKit::Module hh;
            hh.kind = MaschineKit::ModuleKind::hihat;
            hh.intParams = { 1 }; // Circuit -> fmHat
            hh.floatParams = { 0.50f, 0.20f, 0.60f, 0.20f };
            ch.modules.push_back (hh);
            kit.sounds.push_back (std::move (ch));

            MaschineKit::Sound oh;
            oh.index = 3;
            oh.name = "OpenHH";
            MaschineKit::Module hh2;
            hh2.kind = MaschineKit::ModuleKind::hihat;
            hh2.intParams = { 2 }; // Memory -> noiseHat
            hh2.floatParams = { 0.50f, 0.75f, 0.65f, 0.20f };
            oh.modules.push_back (hh2);
            kit.sounds.push_back (std::move (oh));
        }

        // Group FX: Compressor -> Kit Bus Glue
        {
            MaschineKit::Module grpComp;
            grpComp.kind = MaschineKit::ModuleKind::compressor;
            grpComp.floatParams = { 0.40f, 0.60f };
            kit.groupFx.push_back (grpComp);
        }

        return kit;
    }

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
}

void runMaschineKitTests()
{
    std::cout << "Maschine kit import:" << std::endl;

    // 1. Round-trip synthetic kit write/read
    const auto original = syntheticKit();
    const auto bytes = MaschineKit::write (original);
    MaschineKit::Kit parsed;
    const auto err = MaschineKit::read (bytes.getData(), bytes.getSize(), parsed);
    check (err.isEmpty() && parsed.isGroup && parsed.sounds.size() >= 4,
           "maschine kit: synthetic .mxgrp writes and parses back cleanly (" + juce::String ((int) parsed.sounds.size()) + " sounds)");

    if (parsed.sounds.size() >= 4)
    {
        check (parsed.sounds[0].name == "Kick Synth" && std::abs (parsed.sounds[0].level - 0.72f) < 0.01f
                   && parsed.sounds[0].modules.size() == 4
                   && parsed.sounds[0].modules[0].kind == MaschineKit::ModuleKind::kick
                   && parsed.sounds[0].modules[1].kind == MaschineKit::ModuleKind::saturator
                   && parsed.sounds[0].modules[2].kind == MaschineKit::ModuleKind::eq
                   && parsed.sounds[0].modules[3].kind == MaschineKit::ModuleKind::metaverb,
               "maschine kit: pad 0 recovers name, mixer level, Drum Synth Kick, Saturator, EQ and Metaverb");

        check (parsed.sounds[1].name == "Snare Layered" && std::abs (parsed.sounds[1].pan - (-0.15f)) < 0.01f
                   && ! parsed.sounds[1].modules.empty()
                   && parsed.sounds[1].modules[0].kind == MaschineKit::ModuleKind::sampler
                   && parsed.sounds[1].modules[0].zones.size() == 2
                   && parsed.sounds[1].modules[0].zones[0].highVelocity == 70
                   && parsed.sounds[1].modules[0].zones[1].lowVelocity == 71
                   && parsed.sounds[1].modules[0].samplerFilterMode == 1,
               "maschine kit: pad 1 recovers 2 velocity-layered Sampler zones and filter mode");

        check (! parsed.groupFx.empty() && parsed.groupFx[0].kind == MaschineKit::ModuleKind::compressor,
               "maschine kit: group-level Compressor FX recovered after pad 16");
    }

    // 2. Untrusted input safety: non-Maschine data, truncation, oversized
    {
        MaschineKit::Kit bad;
        const char text[] = "Not a Maschine file at all";
        check (MaschineKit::read (text, sizeof (text), bad).contains ("not a Maschine"),
               "maschine kit: non-Maschine data is rejected cleanly");
        check (MaschineKit::read (nullptr, 0, bad).contains ("not a Maschine"),
               "maschine kit: empty buffer is rejected cleanly");

        int crashed = 0;
        for (size_t cut = 0; cut < bytes.getSize(); cut += std::max<size_t> (1, bytes.getSize() / 32))
        {
            MaschineKit::Kit trunc;
            MaschineKit::read (bytes.getData(), cut, trunc);
            ++crashed;
        }
        check (crashed > 10, "maschine kit: " + juce::String (crashed) + " truncated slices parsed/rejected without crashing");

        check (! MaschineKit::isSafeRelativePath ("../escape.wav")
                   && ! MaschineKit::isSafeRelativePath ("/etc/passwd")
                   && ! MaschineKit::isSafeRelativePath ("C:/Windows/win.ini")
                   && MaschineKit::isSafeRelativePath ("Samples/Drums/Kick/Kick 1.wav"),
               "maschine kit: path traversal and rooted relative paths are refused");
    }

    // 3. End-to-end clean-room plan + BasicDrumsProcessor apply + audio render verification
    {
        const auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("torqwave-maschine-test");
        tmp.deleteRecursively();
        const auto kitFile = tmp.getChildFile ("Groups/Kits/Synthetic Industrial.mxgrp");
        writeSineWav (tmp.getChildFile ("Samples/Drums/Snare Soft.wav"), 280.0);
        writeSineWav (tmp.getChildFile ("Samples/Drums/Snare Hard.wav"), 440.0);

        auto plan = DrumKitImport::fromMaschine (parsed, kitFile);
        DrumKitImport::place (plan);

        check (plan.missing.isEmpty() && plan.pads.size() == 4,
               "maschine import: all 4 pads planned and both sample layers resolved automatically from parent library root");

        BuiltInEffects::BasicDrumsProcessor drums;
        drums.prepareToPlay (48000.0, 256);
        const auto applyErr = DrumKitImport::apply (drums, plan);
        check (applyErr.isEmpty(), "maschine import: plan applies cleanly onto Basic Drums (" + applyErr + ")");

        const auto kickPad = (int) DrumMachine::Sound::kick;
        const auto snarePad = (int) DrumMachine::Sound::snare;
        const auto chPad = (int) DrumMachine::Sound::closedHat;
        const auto ohPad = (int) DrumMachine::Sound::openHat;
        const auto kickFx = drums.getEffects (kickPad);
        const auto snareFx = drums.getEffects (snarePad);

        check (drums.getSource (kickPad) == DrumMachine::sourceWithId ("fmKick")
                   && kickFx.on (DrumMachine::satOn)
                   && kickFx.on (DrumMachine::eqOn)
                   && drums.parameterOf (DrumMachine::Sound::kick, "reverb")->get() > 5.0f,
               "maschine import: clean-room Kick Synth -> fmKick with Pad Saturator, 4-Band EQ and Reverb send enabled");

        check (drums.getType (DrumMachine::Sound::snare) == DrumMachine::sampleType
                   && drums.filterParameter (DrumMachine::Sound::snare)->getIndex() == 1
                   && snareFx.on (DrumMachine::transOn)
                   && drums.parameterOf (DrumMachine::Sound::snare, "crush")->get() > 5.0f
                   && drums.parameterOf (DrumMachine::Sound::snare, "delay")->get() > 5.0f,
               "maschine import: layered Sample Snare carries Filter, Transient Shaper, Bitcrusher and Delay send");

        check (drums.getChoke (chPad) == 1 && drums.getChoke (ohPad) == 1 && drums.glueParameter()->get() > 10.0f,
               "maschine import: Closed/Open Hi-Hats auto-choke in group 1 and Group Compressor maps to Kit Glue");

        // Render a kick + snare hit and confirm non-silent, finite audio!
        juce::AudioBuffer<float> audio (2, 1024);
        audio.clear();
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (10, 36, (juce::uint8) 110), 0);
        midi.addEvent (juce::MidiMessage::noteOn (10, 38, (juce::uint8) 100), 128);
        drums.processBlock (audio, midi);
        const float peak = audio.getMagnitude (0, 1024);
        check (std::isfinite (peak) && peak > 0.02f,
               "maschine import: imported kit (clean-room synth kick + layered sample snare + pad FX) renders clean audio (peak "
                   + juce::String (peak, 3) + ")");

        tmp.deleteRecursively();
    }
}

int runMaschineKitFiles (const juce::StringArray& files)
{
    int ok = 0, failed = 0, totalPads = 0, totalSamples = 0, totalMissing = 0, totalSynthPads = 0, totalFx = 0;

    for (auto& path : files)
    {
        const juce::File f (path);
        MaschineKit::Kit kit;
        const auto err = MaschineKit::read (f, kit);

        if (err.isNotEmpty())
        {
            std::cerr << "FAIL  " << f.getFileName() << ": " << err << std::endl;
            ++failed;
            continue;
        }

        auto plan = DrumKitImport::fromMaschine (kit, f);
        DrumKitImport::place (plan);
        ++ok;
        totalPads += (int) plan.pads.size();
        totalMissing += plan.missing.size();

        for (auto& p : plan.pads)
        {
            totalSamples += (int) p.layers.size();
            if (p.synthesised) ++totalSynthPads;
            if (p.filter > 0) ++totalFx;
            if (p.crush > 0.0f) ++totalFx;
            if (p.reverb > 0.0f) ++totalFx;
            if (p.delay > 0.0f) ++totalFx;
            if (p.effects.anyOn()) ++totalFx;
        }
    }

    std::cout << "Maschine files verified: " << ok << "/" << files.size()
              << " passed (" << failed << " failed), " << totalPads << " pads ("
              << totalSamples << " resolved samples, " << totalMissing << " missing, "
              << totalSynthPads << " clean-room synth pads, " << totalFx << " pads with FX)." << std::endl;
    return failed == 0 ? 0 : 1;
}
