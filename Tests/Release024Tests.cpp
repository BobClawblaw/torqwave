#include "Release024Tests.h"
#include "../Source/MpcKit.h"
#include "../Source/Sf2Import.h"
#include "../Source/SamplerEditor.h"
#include "../Source/DrumKitImport.h"
#include "../Source/CreativeEffects.h"
#include "../Source/Notation.h"
#include "../Source/NoteEditor.h"
#include "../Source/MidiInput.h"
#include "../Source/AiAssistant.h"
#include "../Source/Theme.h"
#include "../Source/MassiveXImport.h"
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

        // Multi-preset SF2 bank listing & SamplerEditor preset switching
        std::vector<int16_t> pcm2 (4410);
        for (size_t i = 0; i < pcm2.size(); ++i)
            pcm2[i] = (int16_t) std::lround (18000.0 * std::sin (2.0 * juce::MathConstants<double>::pi * 880.0 * (double) i / 44100.0));

        const auto bankBytes = Sf2Import::writeBank ({
            { "Acid Bass", 0, 0, 48, 24, 60, pcm },
            { "Ice Bell",  0, 1, 72, 60, 96, pcm2 }
        }, 44100);
        const auto bankFile = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("torqwave-sf2-bank.sf2");
        bankFile.replaceWithData (bankBytes.getData(), bankBytes.getSize());

        juce::String listErr;
        const auto presets = Sf2Import::listPresets (bankFile, listErr);
        check (listErr.isEmpty() && presets.size() == 2 && presets[0].name == "Acid Bass" && presets[1].name == "Ice Bell",
               "0.24 sf2 bank: lists all presets in multi-preset SoundFont 2 bank");

        SamplerEditor editor (sampler);
        const bool loaded0 = editor.loadFile (bankFile);
        const bool switched1 = editor.loadSf2Preset (1);
        check (loaded0 && editor.getNumSf2Presets() == 2 && switched1
                   && sampler.getZones().size() == 1 && sampler.getZones()[0].root == 72,
               "0.24 sf2 bank: SamplerEditor switches cleanly to preset #2 in multi-preset SF2 bank");
        bankFile.deleteFile();
    }

    void testMaschinePatternDrop()
    {
        MaschineKit::Pattern pat;
        pat.name = "Main Groove";
        pat.bpm = 128.0;
        pat.notes.push_back ({ 0, 0.0, 0.25, 120, 60 });
        pat.notes.push_back ({ 1, 1.0, 0.25, 110, 60 });
        pat.notes.push_back ({ 0, 2.0, 0.25, 120, 60 });

        const std::map<int, int> noteMap { { 0, 36 }, { 1, 38 } };
        const auto midi = MaschineKit::buildMidiFile (pat, noteMap);

        juce::MemoryOutputStream mos;
        midi.writeTo (mos);
        juce::MemoryInputStream mis (mos.getData(), mos.getDataSize(), false);
        const auto imported = MidiIn::importFile (mis, 48000.0);
        check (imported.error.isEmpty() && ! imported.clips.empty() && imported.clips[0].notes.size() == 3
                   && imported.clips[0].notes[0].pitch == 36 && imported.clips[0].notes[1].pitch == 38,
               "0.24 maschine pattern drop: converts extracted pattern directly into NoteClip with mapped pad notes");
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

        // 3. Vocoder: verify 24-band modulator envelope shapes internal saw chord carrier (and stays silent on silent modulator)
        {
            BuiltInEffects::VocoderProcessor voc;
            voc.prepareToPlay (48000.0, 512);
            juce::MidiBuffer empty;

            juce::AudioBuffer<float> silent (2, 2048);
            silent.clear();
            voc.processBlock (silent, empty);
            const float silentPeak = silent.getMagnitude (0, 2048);

            juce::AudioBuffer<float> mod (2, 4096);
            for (int i = 0; i < 4096; ++i)
            {
                const float s = 0.5f * std::sin (2.0f * juce::MathConstants<float>::pi * 600.0f * (float) i / 48000.0f);
                mod.setSample (0, i, s);
                mod.setSample (1, i, s);
            }
            voc.processBlock (mod, empty);
            const float activePeak = mod.getMagnitude (1024, 3072);
            check (silentPeak < 1.0e-5f && std::isfinite (activePeak) && activePeak > 0.03f && activePeak <= 1.2f,
                   "0.24 vocoder: 24-band filterbank gates cleanly on silence and vocodes active modulator (peak " + juce::String (activePeak, 3) + ")");
        }

        // 4. Spectral Freeze: verify frozen STFT spectrum sustains audio indefinitely after input falls silent
        {
            BuiltInEffects::SpectralFreezeProcessor sf;
            sf.prepareToPlay (48000.0, 512);
            if (auto* mixP = dynamic_cast<juce::AudioParameterFloat*> (sf.getParameters()[4]))
                BuiltInEffects::setExactly (*mixP, 100.0f);

            juce::MidiBuffer empty;
            juce::AudioBuffer<float> feed (2, 4096);
            for (int i = 0; i < 4096; ++i)
            {
                const float s = 0.5f * std::sin (2.0f * juce::MathConstants<float>::pi * 440.0f * (float) i / 48000.0f);
                feed.setSample (0, i, s);
                feed.setSample (1, i, s);
            }
            sf.processBlock (feed, empty);

            // Engage Freeze and feed pure silence for 12,288 samples (256 ms)
            if (auto* frz = dynamic_cast<juce::AudioParameterBool*> (sf.getParameters()[0]))
                *frz = true;

            juce::AudioBuffer<float> tail (2, 12288);
            tail.clear();
            sf.processBlock (tail, empty);
            const float frozenPeak = tail.getMagnitude (8192, 4096);
            check (std::isfinite (frozenPeak) && frozenPeak > 0.05f && frozenPeak < 1.5f,
                   "0.24 spectral freeze: holds frozen STFT drone cleanly across pure silence (peak " + juce::String (frozenPeak, 3) + ")");
        }
    }

    void testNotationAndMusicXml()
    {
        const auto cMaj = NoteTools::Scale { 0, "Major" };
        const auto fMaj = NoteTools::Scale { 5, "Major" };
        const auto spC4 = Notation::spell (60, false);
        const auto spBb4 = Notation::spell (70, Notation::prefersFlats (fMaj));
        check (spC4.step == 'C' && spC4.octave == 4 && spC4.diatonic == 28 && spC4.accidental == 0
                   && spBb4.step == 'B' && spBb4.octave == 4 && spBb4.accidental == -1
                   && Notation::keyFifths (fMaj) == -1,
               "0.24 notation: spells MIDI pitches onto diatonic staff positions with key-aware sharps/flats");

        const auto gs = Notation::GrandStaff::fit ({ 0.0f, 0.0f, 800.0f, 300.0f });
        check (gs.diatonicAtY (gs.yForDiatonic (28)) == 28 && gs.diatonicAtY (gs.yForDiatonic (34)) == 34
                   && gs.diatonicAtY (gs.yForDiatonic (20)) == 20,
               "0.24 notation: GrandStaff yForDiatonic <-> diatonicAtY round-trips treble, middle C, and bass");

        NoteClip clip;
        clip.name = "Score Test";
        clip.length = 96000; // 4 beats at 120 BPM, 48 kHz
        clip.notes.push_back ({ 60, 0, 24000, 0.8f });      // C4 quarter (treble)
        clip.notes.push_back ({ 64, 0, 24000, 0.8f });      // E4 chord tone
        clip.notes.push_back ({ 48, 24000, 48000, 0.75f }); // C3 half (bass)

        const auto xml = Notation::toMusicXml (clip, 48000.0, 0.5, 4, cMaj, "Score Test");
        check (xml.contains ("<score-partwise version=\"3.1\">")
                   && xml.contains ("<staves>2</staves>")
                   && xml.contains ("<chord/>")
                   && xml.contains ("<step>C</step><octave>4</octave>")
                   && xml.contains ("<staff>2</staff>")
                   && xml.contains ("<type>half</type>"),
               "0.24 notation: exports valid MusicXML 3.1 with grand staff, chords, durations, and rests");

        // Verify NoteEditor toggles Score view cleanly and renders without issues
        NoteEditor ed;
        ed.setSize (800, 320);
        NoteEditor::Shown s;
        s.clip = clip;
        s.sampleRate = 48000.0;
        s.beatSeconds = 0.5;
        s.beatsPerBar = 4;
        ed.readClip = [&] { return std::optional<NoteEditor::Shown> (s); };
        ed.refresh();
        ed.setShowScore (true);
        juce::Image img (juce::Image::ARGB, 800, 320, true);
        juce::Graphics g (img);
        ed.paintEntireComponent (g, true);
        const auto tmpXml = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("torqwave-score-test.musicxml");
        const auto expErr = ed.exportMusicXml (tmpXml);
        check (ed.showingScore() && expErr.isEmpty() && tmpXml.existsAsFile() && tmpXml.getSize() > 400,
               "0.24 notation: NoteEditor renders grand-staff Score view and exports .musicxml cleanly");
        tmpXml.deleteFile();

        // Dedicated Score / Notation window mode: stays in grand-staff mode even with no clip selected or on a drum track
        NoteEditor scoreWin;
        scoreWin.setSize (800, 320);
        scoreWin.setScoreOnlyMode (true);
        juce::Image emptyImg (juce::Image::ARGB, 800, 320, true);
        juce::Graphics emptyG (emptyImg);
        scoreWin.paintEntireComponent (emptyG, true);
        NoteEditor::Shown drumShown = s;
        drumShown.drums = true;
        scoreWin.readClip = [&] { return std::optional<NoteEditor::Shown> (drumShown); };
        scoreWin.refresh();
        check (scoreWin.isScoreOnlyMode() && scoreWin.showingScore() && ! scoreWin.showingDrums(),
               "0.24 notation: dedicated Score / Notation window renders empty grand staff and stays in score mode");
    }

    void testZeroTelemetryGuarantee()
    {
        // Verify default AI settings are strictly off with empty server URL and hearClips disabled
        const Ai::Settings defaults;
        check (! defaults.enabled && ! defaults.hearClips && defaults.serverUrl.isEmpty() && ! defaults.isOn(),
               "0.24 zero telemetry: optional Ask Torq defaults to disabled, no server URL, and hearClips off");
    }

    void testMassiveXImport()
    {
        // Build a synthetic, self-contained RIFF/NIKS + MessagePack + RFC-1950 zlib .nksf in memory
        auto mpStr = [] (std::vector<uint8_t>& b, const std::string& s)
        {
            if (s.size() <= 31) { b.push_back ((uint8_t) (0xa0 | s.size())); }
            else if (s.size() <= 255) { b.push_back (0xd9); b.push_back ((uint8_t) s.size()); }
            else { b.push_back (0xda); b.push_back ((uint8_t) (s.size() >> 8)); b.push_back ((uint8_t) (s.size() & 0xff)); }
            b.insert (b.end(), s.begin(), s.end());
        };
        auto mpMap = [] (std::vector<uint8_t>& b, size_t n)
        {
            if (n <= 15) b.push_back ((uint8_t) (0x80 | n));
            else { b.push_back (0xde); b.push_back ((uint8_t) (n >> 8)); b.push_back ((uint8_t) (n & 0xff)); }
        };
        auto mpDouble = [] (std::vector<uint8_t>& b, double v)
        {
            b.push_back (0xcb);
            uint64_t bits = 0;
            std::memcpy (&bits, &v, 8);
            for (int i = 7; i >= 0; --i) b.push_back ((uint8_t) ((bits >> (i * 8)) & 0xff));
        };
        auto mpInt32 = [] (std::vector<uint8_t>& b, int32_t v)
        {
            b.push_back (0xd2);
            const uint32_t u = (uint32_t) v;
            b.push_back ((uint8_t) ((u >> 24) & 0xff)); b.push_back ((uint8_t) ((u >> 16) & 0xff));
            b.push_back ((uint8_t) ((u >> 8) & 0xff));  b.push_back ((uint8_t) (u & 0xff));
        };
        auto mpBool = [] (std::vector<uint8_t>& b, bool v) { b.push_back (v ? 0xc3 : 0xc2); };

        // Minimal RFC-1950 zlib wrapper using uncompressed deflate stored blocks + Adler-32
        auto wrapZlibStored = [] (const std::vector<uint8_t>& src)
        {
            std::vector<uint8_t> out { 0x78, 0x01 };
            size_t pos = 0;
            while (pos < src.size() || pos == 0)
            {
                const size_t chunk = std::min<size_t> (65535, src.size() - pos);
                const bool last = (pos + chunk >= src.size());
                out.push_back (last ? 0x01 : 0x00);
                const uint16_t len = (uint16_t) chunk;
                const uint16_t nlen = (uint16_t) ~len;
                out.push_back ((uint8_t) (len & 0xff));  out.push_back ((uint8_t) (len >> 8));
                out.push_back ((uint8_t) (nlen & 0xff)); out.push_back ((uint8_t) (nlen >> 8));
                out.insert (out.end(), src.begin() + (ptrdiff_t) pos, src.begin() + (ptrdiff_t) (pos + chunk));
                pos += chunk;
                if (last) break;
            }
            uint32_t s1 = 1, s2 = 0;
            for (auto c : src) { s1 = (s1 + c) % 65521; s2 = (s2 + s1) % 65521; }
            const uint32_t adler = (s2 << 16) | s1;
            out.push_back ((uint8_t) ((adler >> 24) & 0xff)); out.push_back ((uint8_t) ((adler >> 16) & 0xff));
            out.push_back ((uint8_t) ((adler >> 8) & 0xff));  out.push_back ((uint8_t) (adler & 0xff));
            return out;
        };

        auto buildNksf = [&] (uint32_t vstMagic, const std::string& pluginName)
        {
            std::vector<uint8_t> nisi { 1, 0, 0, 0 };
            mpMap (nisi, 3);
            mpStr (nisi, "name");   mpStr (nisi, "Synthetic Acid Bass");
            mpStr (nisi, "author"); mpStr (nisi, "Clean Room Test");
            mpStr (nisi, "characters"); nisi.push_back (0x92); mpStr (nisi, "Acid"); mpStr (nisi, "Bass");

            std::vector<uint8_t> plid { 1, 0, 0, 0 };
            mpMap (plid, 2);
            mpStr (plid, "VST.magic"); plid.push_back (0xce);
            plid.push_back ((uint8_t) ((vstMagic >> 24) & 0xff)); plid.push_back ((uint8_t) ((vstMagic >> 16) & 0xff));
            plid.push_back ((uint8_t) ((vstMagic >> 8) & 0xff));  plid.push_back ((uint8_t) (vstMagic & 0xff));
            mpStr (plid, "pluginName"); mpStr (plid, pluginName);

            std::vector<uint8_t> tree;
            const std::string u1 = "root/engine/unit1/";
            mpStr (tree, "strings"); tree.push_back (8);
            mpMap (tree, 8);
            mpStr (tree, u1 + "audioPluginSlots/MainOsc1Slot/audioPlugins/Wavetable/Mode/selectedEntryName"); mpStr (tree, "Bend");
            mpStr (tree, u1 + "audioPluginSlots/FilterSlot/selectedPlugin"); mpStr (tree, "BlueMonarkPlugin");
            mpStr (tree, u1 + "audioPluginSlots/FilterSlot/audioPlugins/BlueMonarkPlugin/Type/selectedEntryName"); mpStr (tree, "LP4");
            mpStr (tree, u1 + "modulationPluginSlots/Mod2Slot/selectedPlugin"); mpStr (tree, "ModEnvelope");
            mpStr (tree, u1 + "audioPluginSlots/FilterSlot/audioPlugins/BlueMonarkPlugin/Cutoff/ModTarget1/modulationTargetNodeState/modulationSource"); mpStr (tree, "engine/unit1/Mod2Slot/ModEnvelope/Out");
            mpStr (tree, "root/engine/global/macros/macro1/macroName/value"); mpStr (tree, "Cutoff");
            mpStr (tree, "root/engine/global/macros/macro1/macroAssignment/oneToOneMappingPath"); mpStr (tree, "engine/unit1/FilterSlot/BlueMonarkPlugin/Cutoff/Knob");
            mpStr (tree, u1 + "audioPluginSlots/UnitFX1Slot/selectedPlugin"); mpStr (tree, "EcoReverb");

            mpStr (tree, "doubles"); tree.push_back (9);
            mpMap (tree, 9);
            mpStr (tree, u1 + "audioPluginSlots/MainOsc1Slot/audioPlugins/Wavetable/Level/Knob/parameterState/normalizedValue"); mpDouble (tree, 0.9);
            mpStr (tree, u1 + "audioPluginSlots/MainOsc1Slot/audioPlugins/Wavetable/Pos/Knob/parameterState/normalizedValue"); mpDouble (tree, 0.45);
            mpStr (tree, u1 + "audioPluginSlots/FilterSlot/audioPlugins/BlueMonarkPlugin/Cutoff/Knob/parameterState/normalizedValue"); mpDouble (tree, 0.35);
            mpStr (tree, u1 + "audioPluginSlots/FilterSlot/audioPlugins/BlueMonarkPlugin/Res/Knob/parameterState/normalizedValue"); mpDouble (tree, 0.55);
            mpStr (tree, u1 + "audioPluginSlots/FilterSlot/audioPlugins/BlueMonarkPlugin/Cutoff/ModTarget1/modulationAmountState/normalizedValue"); mpDouble (tree, 0.85);
            mpStr (tree, u1 + "modulationPluginSlots/Mod1AmpEnvSlot/modulationPlugins/AmpEnvelope/Attack/Knob/parameterState/normalizedValue"); mpDouble (tree, 0.02);
            mpStr (tree, u1 + "modulationPluginSlots/Mod1AmpEnvSlot/modulationPlugins/AmpEnvelope/Decay/Knob/parameterState/normalizedValue"); mpDouble (tree, 0.30);
            mpStr (tree, u1 + "modulationPluginSlots/Mod1AmpEnvSlot/modulationPlugins/AmpEnvelope/Sustain/Knob/parameterState/normalizedValue"); mpDouble (tree, 0.50);
            mpStr (tree, u1 + "audioPluginSlots/UnitFX1Slot/audioPlugins/EcoReverb/DW/Knob/parameterState/normalizedValue"); mpDouble (tree, 0.30);

            mpStr (tree, "ints"); tree.push_back (2);
            mpMap (tree, 2);
            mpStr (tree, u1 + "audioPluginSlots/MainOsc1Slot/audioPlugins/Wavetable/Pitch/TuningKnob/tuningState/rawValue"); mpInt32 (tree, -12000);
            mpStr (tree, u1 + "genericAdapters/VoicePage/UnisonVoices/UnisonVoices_1/indexState/value"); mpInt32 (tree, 2);

            mpStr (tree, "bools"); tree.push_back (1);
            mpMap (tree, 1);
            mpStr (tree, u1 + "audioPluginSlots/FilterSlot/audioPlugins/BlueMonarkPlugin/Cutoff/ModTarget1/modulationTargetNodeState/isConnected"); mpBool (tree, true);

            const auto z = wrapZlibStored (tree);
            std::vector<uint8_t> pchk (20, 0);
            pchk[0] = 1; pchk[4] = 2; pchk[8] = 2;
            pchk.insert (pchk.end(), z.begin(), z.end());

            std::vector<uint8_t> riff { 'R', 'I', 'F', 'F', 0, 0, 0, 0, 'N', 'I', 'K', 'S' };
            auto appendChunk = [&] (const char id[4], const std::vector<uint8_t>& body)
            {
                riff.insert (riff.end(), id, id + 4);
                const uint32_t sz = (uint32_t) body.size();
                riff.push_back ((uint8_t) (sz & 0xff)); riff.push_back ((uint8_t) ((sz >> 8) & 0xff));
                riff.push_back ((uint8_t) ((sz >> 16) & 0xff)); riff.push_back ((uint8_t) ((sz >> 24) & 0xff));
                riff.insert (riff.end(), body.begin(), body.end());
                if (sz & 1u) riff.push_back (0);
            };
            appendChunk ("NISI", nisi);
            appendChunk ("PLID", plid);
            appendChunk ("PCHK", pchk);
            const uint32_t rsz = (uint32_t) (riff.size() - 8);
            riff[4] = (uint8_t) (rsz & 0xff); riff[5] = (uint8_t) ((rsz >> 8) & 0xff);
            riff[6] = (uint8_t) ((rsz >> 16) & 0xff); riff[7] = (uint8_t) ((rsz >> 24) & 0xff);
            return riff;
        };

        const auto valid = buildNksf (0x4e692448u, "Massive X");
        const auto res = MassiveXImport::importMemory (valid.data(), valid.size(), juce::File());
        check (res.ok && res.info.name == "Synthetic Acid Bass" && res.info.author == "Clean Room Test"
                   && res.oscsMapped >= 1 && res.modsMapped >= 2 && res.macrosMapped == 1 && res.fxMapped >= 1
                   && res.macroNames[0] == "Cutoff"
                   && std::abs (res.values.at ("osc1Pitch") - (-12.0)) < 0.01
                   && std::abs (res.values.at ("osc1Unison") - 3.0) < 0.01
                   && res.values.at ("fxReverbOn") > 0.5,
               "0.24 massive x: clean-room synthetic .nksf imports osc/pitch/unison/filter/mods/macros/FX cleanly");

        // Reject other instruments' .nksf files cleanly
        const auto wrongPlugin = buildNksf (0x12345678u, "Other Synth");
        check (! MassiveXImport::importMemory (wrongPlugin.data(), wrongPlugin.size()).ok,
               "0.24 massive x: rejects non-Massive-X .nksf presets cleanly");

        // Truncated fuzz safety
        bool fuzzSafe = true;
        for (size_t cut = 0; cut < valid.size(); cut += std::max<size_t> (1, valid.size() / 24))
        {
            const auto r = MassiveXImport::importMemory (valid.data(), cut);
            if (r.ok) { fuzzSafe = false; break; }
        }
        check (fuzzSafe, "0.24 massive x: truncated .nksf slices are rejected without crashing");

        // Apply onto live WaveSynth and render finite non-silent audio
        BuiltInEffects::BasicSynthProcessor synth;
        synth.prepareToPlay (48000.0, 512);
        const bool applied = MassiveXImport::applyToSynth (synth, res);
        juce::AudioBuffer<float> buf (2, 2048);
        buf.clear();
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 48, 0.85f), 0);
        synth.processBlock (buf, midi);
        float peak = 0.0f;
        bool finite = true;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < buf.getNumSamples(); ++i)
            {
                const float v = buf.getSample (ch, i);
                if (! std::isfinite (v)) finite = false;
                peak = std::max (peak, std::abs (v));
            }
        check (applied && finite && peak > 0.01f && synth.getPatchName() == "Synthetic Acid Bass",
               "0.24 massive x: applied patch renders clean finite audio in Torqwave Synth (peak " + juce::String (peak, 3) + ")");
    }
}

void runRelease024Tests()
{
    std::cout << "Release 0.24.0 features:" << std::endl;
    testMpcKit();
    testSf2Import();
    testMaschinePatternDrop();
    testCreativeFx024();
    testNotationAndMusicXml();
    testZeroTelemetryGuarantee();
    testMassiveXImport();
}

int runMassiveXFiles (const juce::StringArray& files)
{
    int passed = 0, failed = 0, totalOsc = 0, totalMods = 0, totalMacros = 0, totalFx = 0, totalLocalWt = 0;
    const auto wtDir = MassiveXImport::defaultLocalWavetablesFolder();

    for (const auto& path : files)
    {
        const juce::File f (path);
        const auto res = MassiveXImport::importFile (f, wtDir);
        if (! res.ok)
        {
            ++failed;
            if (failed <= 25)
                std::cout << "FAIL  " << f.getFileName() << ": " << res.error << std::endl;
            continue;
        }
        ++passed;
        totalOsc += res.oscsMapped;
        totalMods += res.modsMapped;
        totalMacros += res.macrosMapped;
        totalFx += res.fxMapped;
        totalLocalWt += res.localWavetablesLoaded;
    }

    std::cout << "Massive X files verified: " << passed << "/" << files.size() << " passed (" << failed << " failed), "
              << totalOsc << " oscs, " << totalMods << " mod routes, " << totalMacros << " macros, "
              << totalFx << " FX, " << totalLocalWt << " local .mxwt wavetables loaded in place." << std::endl;
    return failed == 0 ? 0 : 1;
}

void saveRelease024Pictures (const juce::File& folder)
{
    folder.createDirectory();
    Theme::apply (Theme::make ("Dark", "Blue"));
    Theme::LookAndFeel laf;

    constexpr double sr = 48000.0;
    constexpr double beatSec = 0.5; // 120 BPM
    constexpr juce::int64 B = (juce::int64) (sr * beatSec); // 1 beat = 24000 frames

    NoteClip clip;
    clip.id = 42;
    clip.name = "Nocturne in D";
    clip.length = 16 * B; // 4 bars of 4/4
    clip.channel = 1;

    auto add = [&] (int pitch, double startBeat, double lenBeats, float vel = 0.82f)
    {
        clip.notes.push_back ({ pitch, (juce::int64) std::llround (startBeat * (double) B),
                                (juce::int64) std::llround (lenBeats * (double) B), vel });
    };

    // Bar 1: Dmaj7
    add (38, 0.0, 2.0, 0.72f); // D2
    add (45, 0.0, 2.0, 0.70f); // A2
    add (50, 2.0, 2.0, 0.72f); // D3
    add (57, 2.0, 2.0, 0.70f); // A3
    add (66, 0.0, 1.5, 0.86f); // F#4 dotted quarter
    add (69, 1.5, 0.5, 0.80f); // A4 eighth
    add (73, 2.0, 1.0, 0.88f); // C#5 quarter
    add (71, 3.0, 1.0, 0.84f); // B4 quarter

    // Bar 2: Bm7
    add (47, 4.0, 2.0, 0.74f); // B2
    add (54, 4.0, 2.0, 0.70f); // F#3
    add (50, 6.0, 1.0, 0.72f); // D3
    add (45, 7.0, 1.0, 0.70f); // A2
    add (69, 4.0, 1.0, 0.86f); // A4
    add (66, 5.0, 0.5, 0.80f); // F#4
    add (64, 5.5, 0.5, 0.78f); // E4
    add (62, 6.0, 2.0, 0.84f); // D4 half
    add (60, 6.0, 2.0, 0.76f); // C4 (natural accidental on middle C ledger line)

    // Bar 3: Gmaj7 -> A7
    add (43, 8.0, 2.0, 0.76f);  // G2
    add (50, 8.0, 2.0, 0.72f);  // D3
    add (45, 10.0, 2.0, 0.76f); // A2
    add (52, 10.0, 2.0, 0.72f); // E3
    add (71, 8.0, 2.0, 0.88f);  // B4 chord
    add (74, 8.0, 2.0, 0.88f);  // D5 chord
    add (78, 8.0, 2.0, 0.90f);  // F#5 chord
    add (79, 10.0, 1.0, 0.92f); // G5
    add (76, 11.0, 0.5, 0.84f); // E5
    add (73, 11.5, 0.5, 0.82f); // C#5

    // Bar 4: Dmaj9 resolution
    add (38, 12.0, 4.0, 0.78f); // D2 whole
    add (45, 12.0, 4.0, 0.74f); // A2 whole
    add (54, 12.0, 4.0, 0.72f); // F#3 whole
    add (66, 12.0, 4.0, 0.84f); // F#4 whole
    add (74, 12.0, 4.0, 0.90f); // D5 whole
    add (81, 12.0, 4.0, 0.92f); // A5 whole (ledger line above treble)

    NoteEditor::Shown shown;
    shown.clip = clip;
    shown.sampleRate = sr;
    shown.beatSeconds = beatSec;
    shown.beatsPerBar = 4;
    shown.trackName = "Grand Piano";
    shown.key = "D major";
    shown.hasInstrument = true;

    NoteEditor ed;
    ed.setLookAndFeel (&laf);
    ed.setScoreOnlyMode (true);
    ed.setGridDivision (4);
    ed.readClip = [&] { return std::optional<NoteEditor::Shown> (shown); };
    ed.readPlayhead = [&] { return std::optional<juce::int64> (6 * B); };
    ed.setSize (1120, 460);
    ed.refresh();
    ed.fitTime();

    const auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 2.0f);
    auto outFile = folder.getChildFile ("guide-score.png");
    outFile.deleteFile();
    juce::FileOutputStream out (outFile);
    juce::PNGImageFormat().writeImageToStream (img, out);
    ed.setLookAndFeel (nullptr);
    std::cout << outFile.getFullPathName() << " " << img.getWidth() << " x " << img.getHeight() << std::endl;
}
