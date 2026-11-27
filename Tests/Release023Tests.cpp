#include "Release023Tests.h"
#include "TestUtils.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/CreativeEffects.h"
#include "../Source/MidiEffects.h"
#include "../Source/AudioToMidi.h"
#include "../Source/SfzImport.h"
#include "../Source/TrackModulators.h"
#include "../Source/TrackEffects.h"
#include "../Source/ClapHost.h"
#include "../Source/WaveSynth.h"
#include "../Source/Sampler.h"
#include "../Source/DrumMachine.h"
#include "../Source/Decrypt.h"
#include "../Source/NoteTools.h"
#include "../Source/NoteEditor.h"
#include "../Source/Session.h"
#include <cmath>
#include <cstring>

namespace
{
    using TestUtils::check;
    constexpr double kRate = 48000.0;

    bool allFinite (const juce::AudioBuffer<float>& buf)
    {
        for (int c = 0; c < buf.getNumChannels(); ++c)
            for (int i = 0; i < buf.getNumSamples(); ++i)
                if (! std::isfinite (buf.getSample (c, i)))
                    return false;
        return true;
    }

    float peakOf (const juce::AudioBuffer<float>& buf, int ch = 0)
    {
        return buf.getMagnitude (ch, 0, buf.getNumSamples());
    }

    //==========================================================================
    // Minimal in-process CLAP 1.2.2 test plugin (stereo gain + MIDI note counter + state)
    struct MockClapPlugin
    {
        clap_plugin_t plugin {};
        double gain = 0.5;
        int notesReceived = 0;
        bool active = false;
    };

    const char* const mockClapFeatures[] = { CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_STEREO, nullptr };

    const clap_plugin_descriptor_t mockClapDesc = {
        CLAP_VERSION,
        "org.torqwave.test.clap-gain",
        "Torqwave Mock CLAP",
        "Torqwave",
        "", "", "", "1.0.0",
        "Mock CLAP plugin for host verification",
        mockClapFeatures
    };

    const clap_plugin_params_t mockClapParams = {
        [] (const clap_plugin_t*) -> uint32_t { return 1; },
        [] (const clap_plugin_t*, uint32_t idx, clap_param_info_t* info) -> bool
        {
            if (idx != 0 || info == nullptr) return false;
            std::memset (info, 0, sizeof (*info));
            info->id = 42;
            info->flags = CLAP_PARAM_IS_AUTOMATABLE;
            std::strncpy (info->name, "Gain", CLAP_NAME_SIZE - 1);
            info->min_value = 0.0;
            info->max_value = 2.0;
            info->default_value = 0.5;
            return true;
        },
        [] (const clap_plugin_t* p, clap_id id, double* out) -> bool
        {
            if (id != 42 || out == nullptr) return false;
            *out = static_cast<MockClapPlugin*> (p->plugin_data)->gain;
            return true;
        },
        [] (const clap_plugin_t*, clap_id, double value, char* out, uint32_t sz) -> bool
        {
            std::snprintf (out, sz, "%.2f", value);
            return true;
        },
        [] (const clap_plugin_t*, clap_id, const char* txt, double* out) -> bool
        {
            *out = std::atof (txt);
            return true;
        },
        [] (const clap_plugin_t*, const clap_input_events_t*, const clap_output_events_t*) {}
    };

    const clap_plugin_state_t mockClapState = {
        [] (const clap_plugin_t* p, const clap_ostream_t* stream) -> bool
        {
            auto* self = static_cast<MockClapPlugin*> (p->plugin_data);
            return stream->write (stream, &self->gain, sizeof (double)) == (int64_t) sizeof (double);
        },
        [] (const clap_plugin_t* p, const clap_istream_t* stream) -> bool
        {
            auto* self = static_cast<MockClapPlugin*> (p->plugin_data);
            double v = 0.5;
            if (stream->read (stream, &v, sizeof (double)) != (int64_t) sizeof (double))
                return false;
            self->gain = v;
            return true;
        }
    };

    const clap_plugin_factory_t mockClapFactory = {
        [] (const clap_plugin_factory_t*) -> uint32_t { return 1; },
        [] (const clap_plugin_factory_t*, uint32_t idx) -> const clap_plugin_descriptor_t* { return idx == 0 ? &mockClapDesc : nullptr; },
        [] (const clap_plugin_factory_t*, const clap_host_t*, const char* id) -> const clap_plugin_t*
        {
            if (id == nullptr || std::strcmp (id, mockClapDesc.id) != 0)
                return nullptr;
            auto* self = new MockClapPlugin();
            self->plugin.desc = &mockClapDesc;
            self->plugin.plugin_data = self;
            self->plugin.init = [] (const clap_plugin_t*) -> bool { return true; };
            self->plugin.destroy = [] (const clap_plugin_t* p) { delete static_cast<MockClapPlugin*> (p->plugin_data); };
            self->plugin.activate = [] (const clap_plugin_t* p, double, uint32_t, uint32_t) -> bool { static_cast<MockClapPlugin*> (p->plugin_data)->active = true; return true; };
            self->plugin.deactivate = [] (const clap_plugin_t* p) { static_cast<MockClapPlugin*> (p->plugin_data)->active = false; };
            self->plugin.start_processing = [] (const clap_plugin_t*) -> bool { return true; };
            self->plugin.stop_processing = [] (const clap_plugin_t*) {};
            self->plugin.reset = [] (const clap_plugin_t*) {};
            self->plugin.process = [] (const clap_plugin_t* p, const clap_process_t* proc) -> clap_process_status
            {
                auto* inst = static_cast<MockClapPlugin*> (p->plugin_data);
                if (proc->in_events != nullptr)
                {
                    const auto nEv = proc->in_events->size (proc->in_events);
                    for (uint32_t e = 0; e < nEv; ++e)
                    {
                        const auto* hdr = proc->in_events->get (proc->in_events, e);
                        if (hdr->type == CLAP_EVENT_PARAM_VALUE)
                        {
                            const auto* pv = reinterpret_cast<const clap_event_param_value_t*> (hdr);
                            if (pv->param_id == 42) inst->gain = pv->value;
                        }
                        else if (hdr->type == CLAP_EVENT_NOTE_ON)
                            ++inst->notesReceived;
                    }
                }
                const auto frames = proc->frames_count;
                for (uint32_t c = 0; c < 2; ++c)
                    for (uint32_t i = 0; i < frames; ++i)
                        proc->audio_outputs[0].data32[c][i] = proc->audio_inputs[0].data32[c][i] * (float) inst->gain;
                return CLAP_PROCESS_CONTINUE;
            };
            self->plugin.get_extension = [] (const clap_plugin_t*, const char* eid) -> const void*
            {
                if (std::strcmp (eid, CLAP_EXT_PARAMS) == 0) return &mockClapParams;
                if (std::strcmp (eid, CLAP_EXT_STATE) == 0)  return &mockClapState;
                return nullptr;
            };
            self->plugin.on_main_thread = [] (const clap_plugin_t*) {};
            return &self->plugin;
        }
    };

    const clap_plugin_entry_t mockClapEntry = {
        CLAP_VERSION,
        [] (const char*) -> bool { return true; },
        [] () {},
        [] (const char* fid) -> const void* { return std::strcmp (fid, CLAP_PLUGIN_FACTORY_ID) == 0 ? &mockClapFactory : nullptr; }
    };

    //==========================================================================
    void testCreativeEffects()
    {
        auto fmt = BuiltInEffects::createFormat();
        auto make = [&] (const char* id)
        {
            juce::PluginDescription found;
            for (auto& d : BuiltInEffects::getDescriptions())
                if (d.fileOrIdentifier == id)
                    found = d;
            juce::String err;
            auto p = fmt->createInstanceFromDescription (found, kRate, 512, err);
            if (p != nullptr)
            {
                p->setPlayConfigDetails (2, 2, kRate, 512);
                p->prepareToPlay (kRate, 512);
            }
            return p;
        };

        // Delay: produces wet echo tail after an impulse
        {
            auto p = make (BuiltInEffects::delayId);
            check (p != nullptr, "0.23 creative fx: Delay instantiates");
            juce::AudioBuffer<float> buf (2, 512);
            juce::MidiBuffer midi;
            buf.clear();
            buf.setSample (0, 0, 1.0f);
            buf.setSample (1, 0, 1.0f);
            p->processBlock (buf, midi);
            float tailPeak = 0.0f;
            for (int b = 0; b < 50; ++b)
            {
                buf.clear();
                p->processBlock (buf, midi);
                tailPeak = juce::jmax (tailPeak, peakOf (buf, 0), peakOf (buf, 1));
            }
            check (allFinite (buf) && tailPeak > 0.02f, "0.23 creative fx: Delay produces finite echo tail");
        }

        // Distortion: 4x oversampling reports Oversampler4::latency and saturates signal
        {
            auto p = make (BuiltInEffects::distortionId);
            check (p != nullptr && p->getLatencySamples() == Oversampler4::latency, "0.23 creative fx: Distortion 4x reports oversampler latency");
            juce::AudioBuffer<float> buf (2, 512);
            juce::MidiBuffer midi;
            for (int i = 0; i < 512; ++i)
            {
                const auto s = 0.6f * std::sin (juce::MathConstants<float>::twoPi * 440.0f * (float) i / (float) kRate);
                buf.setSample (0, i, s);
                buf.setSample (1, i, s);
            }
            p->processBlock (buf, midi);
            check (allFinite (buf) && peakOf (buf) > 0.2f, "0.23 creative fx: Distortion processes finite audio");
        }

        // Auto-Filter, Volume Shaper, Noise Gate, Tremolo & Auto-Pan
        for (auto id : { BuiltInEffects::autoFilterId, BuiltInEffects::volumeShaperId, BuiltInEffects::noiseGateId, BuiltInEffects::tremoloId })
        {
            auto p = make (id);
            check (p != nullptr, juce::String ("0.23 creative fx: instantiates ") + id);
            juce::AudioBuffer<float> buf (2, 512);
            juce::MidiBuffer midi;
            for (int i = 0; i < 512; ++i)
            {
                const auto s = 0.5f * std::sin (juce::MathConstants<float>::twoPi * 220.0f * (float) i / (float) kRate);
                buf.setSample (0, i, s);
                buf.setSample (1, i, s);
            }
            p->processBlock (buf, midi);
            juce::MemoryBlock state;
            p->getStateInformation (state);
            p->setStateInformation (state.getData(), (int) state.getSize());
            check (allFinite (buf) && state.getSize() > 0, juce::String ("0.23 creative fx: finite output and state round-trip for ") + id);
        }

        // Tremolo 180 deg auto-pan separates L and R levels
        {
            auto p = make (BuiltInEffects::tremoloId);
            juce::AudioBuffer<float> buf (2, 2048);
            juce::MidiBuffer midi;
            for (int i = 0; i < 2048; ++i) { buf.setSample (0, i, 0.5f); buf.setSample (1, i, 0.5f); }
            p->processBlock (buf, midi);
            float diff = 0.0f;
            for (int i = 0; i < 2048; ++i) diff = juce::jmax (diff, std::abs (buf.getSample (0, i) - buf.getSample (1, i)));
            check (diff > 0.05f, "0.23 creative fx: Tremolo & Auto-Pan creates stereo panning difference at 180 deg");
        }
    }

    //==========================================================================
    void testSynth023Upgrades()
    {
        // MSEG Curve LFO shape (index 8) + Built-in Arpeggiator + Patch Scenes / Evolve
        BuiltInEffects::BasicSynthProcessor synth;
        synth.setPlayConfigDetails (0, 2, kRate, 512);
        synth.prepareToPlay (kRate, 512);

        // Capture Scene A (dark cutoff) and Scene B (bright cutoff)
        auto* cut = synth.parameter ("filter1Cutoff");
        cut->setValueNotifyingHost (cut->convertTo0to1 (200.0f));
        synth.captureScene (0);
        cut->setValueNotifyingHost (cut->convertTo0to1 (14000.0f));
        synth.captureScene (1);
        check (synth.hasScene (0) && synth.hasScene (1) && synth.getSceneCount() == 2, "0.23 synth: captures 2 morph scenes");

        // State round-trip preserves scenes
        juce::MemoryBlock state;
        synth.getStateInformation (state);
        BuiltInEffects::BasicSynthProcessor restored;
        restored.setPlayConfigDetails (0, 2, kRate, 512);
        restored.prepareToPlay (kRate, 512);
        restored.setStateInformation (state.getData(), (int) state.getSize());
        check (restored.hasScene (0) && restored.hasScene (1), "0.23 synth: scene snapshots round-trip in state");

        // Morphing evolve 0% vs 100% changes sound clearly
        auto renderWithEvolve = [&] (float ev)
        {
            restored.reset();
            auto* evP = restored.parameter ("evolve");
            evP->setValueNotifyingHost (evP->convertTo0to1 (ev));
            juce::AudioBuffer<float> buf (2, 4096);
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);
            restored.processBlock (buf, midi);
            return buf;
        };
        const auto dark = renderWithEvolve (0.0f);
        const auto bright = renderWithEvolve (100.0f);
        float diff = 0.0f;
        for (int i = 0; i < 4096; ++i) diff = juce::jmax (diff, std::abs (dark.getSample (0, i) - bright.getSample (0, i)));
        check (allFinite (dark) && allFinite (bright) && diff > 0.05f, "0.23 synth: Evolve morphs audibly between scenes");

        // Built-in Arpeggiator + MSEG Curve shape
        synth.clearScenes();
        synth.loadPatch ("Init");
        synth.parameter ("lfo1Shape")->setValueNotifyingHost (synth.parameter ("lfo1Shape")->convertTo0to1 (8.0f)); // Curve
        synth.parameter ("arpOn")->setValueNotifyingHost (1.0f);
        synth.reset();
        juce::AudioBuffer<float> arpBuf (2, 12000);
        juce::MidiBuffer arpMidi;
        arpMidi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);
        arpMidi.addEvent (juce::MidiMessage::noteOn (1, 64, 0.9f), 0);
        arpMidi.addEvent (juce::MidiMessage::noteOn (1, 67, 0.9f), 0);
        synth.processBlock (arpBuf, arpMidi);
        check (allFinite (arpBuf) && peakOf (arpBuf) > 0.02f, "0.23 synth: built-in arpeggiator and MSEG Curve LFO render cleanly");
    }

    //==========================================================================
    void testMidiEffectsAndSequencer()
    {
        // Chord Trigger turns 1 note into 4 notes (Minor 7)
        {
            BuiltInEffects::MidiChordProcessor chord;
            chord.prepareToPlay (kRate, 512);
            juce::AudioBuffer<float> dummy (2, 512);
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
            chord.processBlock (dummy, midi);
            int noteOns = 0;
            for (const auto m : midi) if (m.getMessage().isNoteOn()) ++noteOns;
            check (noteOns == 4, "0.23 midi fx: Chord Trigger expands single note to 4-note Minor 7 chord");
        }

        // Scale Lock snaps out-of-scale C# (61) into C minor (60 or 62)
        {
            BuiltInEffects::MidiScaleProcessor scale;
            scale.prepareToPlay (kRate, 512);
            juce::AudioBuffer<float> dummy (2, 512);
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 61, 0.8f), 0);
            scale.processBlock (dummy, midi);
            int outNote = -1;
            for (const auto m : midi) if (m.getMessage().isNoteOn()) outNote = m.getMessage().getNoteNumber();
            check (outNote == 60 || outNote == 62, "0.23 midi fx: Scale Lock quantizes out-of-scale note into C minor");
        }

        // MIDI Arpeggiator + Note Echo
        {
            BuiltInEffects::MidiArpProcessor arp;
            arp.prepareToPlay (kRate, 512);
            juce::AudioBuffer<float> dummy (2, 8192);
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
            midi.addEvent (juce::MidiMessage::noteOn (1, 67, 0.8f), 0);
            arp.processBlock (dummy, midi);
            int ons = 0;
            for (const auto m : midi) if (m.getMessage().isNoteOn()) ++ons;
            check (ons >= 2, "0.23 midi fx: MIDI Arpeggiator generates rhythmic notes");

            BuiltInEffects::MidiEchoProcessor echo;
            echo.prepareToPlay (kRate, 512);
            juce::MidiBuffer em;
            em.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);
            juce::AudioBuffer<float> eb (2, 512);
            int totalOns = 0;
            for (int b = 0; b < 80; ++b)
            {
                echo.processBlock (eb, em);
                for (const auto m : em) if (m.getMessage().isNoteOn()) ++totalOns;
                em.clear();
            }
            check (totalOns >= 3, "0.23 midi fx: Note Echo schedules decaying MIDI repeats");
        }

        // TrackEffects Live MIDI FX -> Instrument pipeline on one track
        {
            TrackEffects fx;
            fx.prepare (kRate);
            fx.add (1, std::make_unique<BuiltInEffects::MidiChordProcessor>());
            fx.add (1, std::make_unique<BuiltInEffects::BasicSynthProcessor>());

            SessionTrack track;
            track.id = 1;
            track.isInstrument = true;
            NoteClip clip;
            clip.position = 0;
            clip.length = 24000;
            clip.notes.push_back ({ 60, 0, 12000, 0.9f, 1.0f, 1, 0 });
            track.noteClips.push_back (clip);

            juce::AudioBuffer<float> buf (2, 2048);
            buf.clear();
            fx.process (track, 0, buf, 2048, {});
            check (allFinite (buf) && peakOf (buf) > 0.01f, "0.23 midi fx: TrackEffects chains MIDI FX into Instrument on same track");
        }

        // ClipNote ratchets & microTiming + Groove templates
        {
            NoteClip nc;
            nc.position = 0;
            nc.length = 48000;
            nc.notes.push_back ({ 60, 1000, 4000, 0.85f, 1.0f, 4, 240 }); // 4x ratchet, +240 frame offset
            juce::MidiBuffer midi;
            nc.addMidi (0, 8000, midi);
            int ons = 0;
            int firstPos = -1;
            for (const auto m : midi)
                if (m.getMessage().isNoteOn())
                {
                    if (firstPos < 0) firstPos = m.samplePosition;
                    ++ons;
                }
            check (ons == 4 && firstPos == 1240, "0.23 sequencer: ClipNote ratchets (4x) and microTiming (+240) render accurately");

            auto grooves = NoteTools::groovePresets();
            check (grooves.size() >= 6, "0.23 sequencer: built-in groove templates available");
            std::vector<ClipNote> gn { { 60, 0, 2000, 0.8f }, { 60, 6000, 2000, 0.8f } }; // beat = 24000, 16th = 6000
            NoteTools::applyGroove (gn, {}, grooves[3], 24000.0, 1.0f, 48000); // MPC 62%
            check (gn[0].start == 0 && gn[1].start > 6000, "0.23 sequencer: MPC groove shifts off-beat 16th later");
        }
    }

    //==========================================================================
    void testAudioToMidi()
    {
        // Melody extraction: 0.4 s A4 (440 Hz) then 0.4 s C5 (523.25 Hz)
        {
            const int len = (int) (0.9 * kRate);
            juce::AudioBuffer<float> buf (1, len);
            buf.clear();
            for (int i = 0; i < (int) (0.4 * kRate); ++i)
                buf.setSample (0, i + 2000, 0.6f * std::sin (juce::MathConstants<float>::twoPi * 440.0f * (float) i / (float) kRate));
            for (int i = 0; i < (int) (0.4 * kRate); ++i)
                buf.setSample (0, i + (int) (0.45 * kRate), 0.6f * std::sin (juce::MathConstants<float>::twoPi * 523.251f * (float) i / (float) kRate));

            auto blk = std::make_shared<AudioBlock> (std::move (buf));
            AudioSnapshot snap (1, { { blk, 0, len } });
            const auto res = AudioToMidi::extractMelody (snap, 0, snap.getNumFrames(), kRate);
            bool foundA4 = false, foundC5 = false;
            for (const auto& n : res.notes)
            {
                if (n.pitch == 69) foundA4 = true;
                if (n.pitch == 72) foundC5 = true;
            }
            check (foundA4 && foundC5, "0.23 audio-to-midi: extracts A4 (69) and C5 (72) melody notes");
        }

        // Drum extraction: synthetic Kick (60 Hz) + Hat (high-passed noise)
        {
            const int len = (int) (1.0 * kRate);
            juce::AudioBuffer<float> buf (1, len);
            buf.clear();
            // Kick at 0.1 s
            for (int i = 0; i < 4800; ++i)
            {
                const auto env = std::exp (-8.0f * (float) i / 4800.0f);
                buf.setSample (0, 4800 + i, 0.9f * env * std::sin (juce::MathConstants<float>::twoPi * 60.0f * (float) i / (float) kRate));
            }
            // High-frequency hat burst at 0.5 s
            juce::uint32 rng = 12345u;
            float prev = 0.0f;
            for (int i = 0; i < 2400; ++i)
            {
                rng = rng * 1664525u + 1013904223u;
                const auto w = ((float) (rng >> 8) / 8388608.0f) - 1.0f;
                const auto hp = w - prev;
                prev = w;
                const auto env = std::exp (-10.0f * (float) i / 2400.0f);
                buf.setSample (0, 24000 + i, 0.6f * env * hp);
            }

            auto blk = std::make_shared<AudioBlock> (std::move (buf));
            AudioSnapshot snap (1, { { blk, 0, len } });
            const auto res = AudioToMidi::extractDrums (snap, 0, snap.getNumFrames(), kRate, {}, 120.0);
            bool foundKick = false, foundHat = false;
            for (const auto& n : res.notes)
            {
                if (n.pitch == 36) foundKick = true;
                if (n.pitch == 42 || n.pitch == 46) foundHat = true;
            }
            check (foundKick && foundHat && ! res.grooveOffsets.empty(), "0.23 audio-to-midi: detects Kick (36), Hat (42/46), and groove offsets");
        }
    }

    //==========================================================================
    void testSamplerAndDrumMultiOut()
    {
        // Write two tiny WAV files to temp folder and load via SFZ (with round robin!)
        const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("tq_sfz_test");
        dir.deleteRecursively();
        dir.createDirectory();

        auto writeWav = [&] (const juce::String& name, float hz)
        {
            const auto f = dir.getChildFile (name);
            juce::AudioBuffer<float> b (1, 4800);
            for (int i = 0; i < 4800; ++i)
                b.setSample (0, i, 0.5f * std::sin (juce::MathConstants<float>::twoPi * hz * (float) i / (float) kRate));
            juce::WavAudioFormat wav;
            if (auto out = f.createOutputStream())
                if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (out.get(), kRate, 1, 16, {}, 0)))
                {
                    out.release();
                    w->writeFromAudioSampleBuffer (b, 0, 4800);
                }
            return f;
        };

        writeWav ("rr1.wav", 440.0f);
        writeWav ("rr2.wav", 880.0f);

        const auto sfzFile = dir.getChildFile ("test.sfz");
        sfzFile.replaceWithText (
            "<global>\n"
            "ampeg_attack=0.005 ampeg_release=0.2\n"
            "<group> lokey=c3 hikey=c5 pitch_keycenter=c4 seq_length=2\n"
            "<region> sample=rr1.wav seq_position=1\n"
            "<region> sample=rr2.wav seq_position=2\n"
        );

        BuiltInEffects::SamplerProcessor sampler;
        sampler.setPlayConfigDetails (0, 2, kRate, 512);
        sampler.prepareToPlay (kRate, 512);
        const auto err = sampler.loadSfz (sfzFile);
        check (err.isEmpty() && sampler.getZones().size() == 2, "0.23 sampler: SFZ importer loads regions with round-robin group");

        // Round-robin alternates between rr1 (440 Hz) and rr2 (880 Hz) on consecutive hits
        juce::AudioBuffer<float> hit1 (2, 2048), hit2 (2, 2048);
        juce::MidiBuffer m1, m2;
        m1.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);
        m1.addEvent (juce::MidiMessage::noteOff (1, 60), 1024);
        sampler.processBlock (hit1, m1);
        m2.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);
        m2.addEvent (juce::MidiMessage::noteOff (1, 60), 1024);
        sampler.processBlock (hit2, m2);
        float rrDiff = 0.0f;
        for (int i = 0; i < 1024; ++i) rrDiff = juce::jmax (rrDiff, std::abs (hit1.getSample (0, i) - hit2.getSample (0, i)));
        check (rrDiff > 0.1f, "0.23 sampler: round-robin cycles between zones on repeated hits");

        // Signalsmith Warp sample test
        const auto warped = Sampler::warpSample (*sampler.getZones()[0].sample, 0.5); // half speed -> 2x length
        check (warped != nullptr && warped->getNumFrames() > 8000 && allFinite (warped->audio), "0.23 sampler: tempo-synced warpSample stretches audio cleanly");
        dir.deleteRecursively();

        // Basic Drums Multi-Out routing
        {
            DrumMachine::Engine dm;
            dm.prepare (kRate);
            DrumMachine::KitSettings kit;
            kit.sounds[(size_t) DrumMachine::Sound::kick].outputBus = 1;  // Aux 1
            kit.sounds[(size_t) DrumMachine::Sound::snare].outputBus = 0; // Main
            dm.setSettings (kit);

            dm.noteOn (36, 1.0f); // Kick -> Aux 1
            std::vector<float> mainL (2048, 0.0f), mainR (2048, 0.0f);
            std::array<std::vector<float>, DrumMachine::numAuxBuses> auxBufL, auxBufR;
            std::array<float*, DrumMachine::numAuxBuses> auxPtrL {}, auxPtrR {};
            for (int b = 0; b < DrumMachine::numAuxBuses; ++b)
            {
                auxBufL[(size_t) b].assign (2048, 0.0f);
                auxBufR[(size_t) b].assign (2048, 0.0f);
                auxPtrL[(size_t) b] = auxBufL[(size_t) b].data();
                auxPtrR[(size_t) b] = auxBufR[(size_t) b].data();
            }
            dm.renderMultiOut (mainL.data(), mainR.data(), auxPtrL, auxPtrR, 2048);

            float mainPk = 0.0f, aux1Pk = 0.0f;
            for (int i = 0; i < 2048; ++i)
            {
                mainPk = juce::jmax (mainPk, std::abs (mainL[(size_t) i]));
                aux1Pk = juce::jmax (aux1Pk, std::abs (auxBufL[0][(size_t) i]));
            }
            check (aux1Pk > 0.1f && mainPk < 0.01f, "0.23 drums: multi-out routes Kick (outputBus=1) to Aux 1, leaving Main clear");

            BuiltInEffects::BasicDrumsProcessor proc;
            proc.setOutputBus (0, 3);
            juce::MemoryBlock state;
            proc.getStateInformation (state);
            BuiltInEffects::BasicDrumsProcessor proc2;
            proc2.setStateInformation (state.getData(), (int) state.getSize());
            check (proc2.getOutputBus (0) == 3, "0.23 drums: pad multi-out bus assignment persists in state");
        }
    }

    //==========================================================================
    void testTrackModulatorsAndClap()
    {
        // Track Modulator LFO modulates AutoFilter cutoff across blocks
        {
            TrackEffects fx;
            fx.prepare (kRate);
            auto af = std::make_unique<BuiltInEffects::AutoFilterProcessor>();
            auto* afRaw = af.get();
            const auto slotId = fx.add (1, std::move (af));

            SessionTrack track;
            track.id = 1;
            TrackModulator mod;
            mod.id = 1;
            mod.kind = TrackModulator::Kind::lfo;
            mod.targetSlotId = slotId;
            mod.targetParameterId = "cutoff";
            mod.sync = false;
            mod.rateHz = 4.0;
            mod.amount = 0.35f;
            track.modulators.push_back (mod);

            // XML round-trip
            const auto xml = mod.toXml();
            const auto parsed = TrackModulator::fromXml (*xml);
            check (parsed.has_value() && *parsed == mod, "0.23 modulators: TrackModulator XML round-trip");

            const auto startVal = afRaw->getParameters()[1]->getValue();
            juce::AudioBuffer<float> buf (2, 4096);
            buf.clear();
            fx.process (track, 0, buf, 4096, {});
            const auto endVal = afRaw->getParameters()[1]->getValue();
            check (std::abs (endVal - startVal) > 0.01f, "0.23 modulators: track LFO modulates plug-in parameter during processing");
        }

        // CLAP 1.2.2 host lifecycle with in-process mock plugin
        {
            juce::String err;
            auto inst = ClapHost::createFromEntry (&mockClapEntry, mockClapDesc.id, kRate, 512, err);
            check (inst != nullptr && err.isEmpty(), "0.23 clap: hosts CLAP 1.2.2 plug-in from entry");
            if (inst != nullptr)
            {
                inst->prepareToPlay (kRate, 512);
                check (inst->getParameters().size() == 1, "0.23 clap: exposes CLAP_EXT_PARAMS as JUCE parameters");

                // Set gain param to 1.5 (normalised 0.75 of [0, 2])
                inst->getParameters()[0]->setValue (0.75f);
                juce::AudioBuffer<float> buf (2, 128);
                for (int i = 0; i < 128; ++i) { buf.setSample (0, i, 0.4f); buf.setSample (1, i, -0.2f); }
                juce::MidiBuffer midi;
                midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
                inst->processBlock (buf, midi);
                check (std::abs (buf.getSample (0, 64) - 0.6f) < 1.0e-4f, "0.23 clap: processes audio with updated CLAP parameter value");

                // State save/restore round-trip
                juce::MemoryBlock st;
                inst->getStateInformation (st);
                inst->getParameters()[0]->setValue (0.1f);
                inst->setStateInformation (st.getData(), (int) st.getSize());
                check (std::abs (inst->getParameters()[0]->getValue() - 0.75f) < 1.0e-4f, "0.23 clap: CLAP_EXT_STATE save/restore round-trip");
            }
        }
    }
}

void runRelease023Tests()
{
    testCreativeEffects();
    testSynth023Upgrades();
    testMidiEffectsAndSequencer();
    testAudioToMidi();
    testSamplerAndDrumMultiOut();
    testTrackModulatorsAndClap();
}

namespace
{
    struct DemoPlayHead final : juce::AudioPlayHead
    {
        double bpm = 128.0;
        double sampleRate = 48000.0;
        juce::int64 samplePos = 0;

        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo p;
            p.setBpm (bpm);
            p.setIsPlaying (true);
            p.setTimeInSamples (samplePos);
            p.setTimeInSeconds ((double) samplePos / sampleRate);
            p.setPpqPosition (((double) samplePos / sampleRate) * (bpm / 60.0));
            return p;
        }
    };

    void setById (juce::AudioProcessor& proc, const juce::String& id, float plainOrNorm, bool isNorm = false)
    {
        for (auto* p : proc.getParameters())
        {
            if (auto* wp = dynamic_cast<juce::AudioProcessorParameterWithID*> (p); wp != nullptr && wp->paramID == id)
            {
                if (isNorm)
                    wp->setValue (juce::jlimit (0.0f, 1.0f, plainOrNorm));
                else if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
                    rp->setValue (rp->convertTo0to1 (plainOrNorm));
                return;
            }
        }
    }

    void writeDemoWav (const juce::File& file, juce::AudioBuffer<float>& buf, double sr)
    {
        // Clean peak levelling to -1.0 dBFS (~0.891) so every audition clip plays loud & clear without clipping
        float peak = 0.0f;
        for (int c = 0; c < buf.getNumChannels(); ++c)
            peak = juce::jmax (peak, buf.getMagnitude (c, 0, buf.getNumSamples()));
        if (peak > 1.0e-4f)
            buf.applyGain (0.891f / peak);

        // Short 10ms fade-out at end to avoid any tail cut click
        const auto fade = juce::jmin (480, buf.getNumSamples() / 8);
        for (int c = 0; c < buf.getNumChannels(); ++c)
            buf.applyGainRamp (c, buf.getNumSamples() - fade, fade, 1.0f, 0.0f);

        file.deleteFile();
        juce::WavAudioFormat wav;
        if (std::unique_ptr<juce::OutputStream> out = file.createOutputStream())
        {
            const auto opts = juce::AudioFormatWriterOptions()
                .withSampleRate (sr)
                .withNumChannels (buf.getNumChannels())
                .withBitsPerSample (24);
            if (auto writer = wav.createWriterFor (out, opts))
                writer->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples());
        }
        std::cout << "  wrote " << file.getFileName() << " (" << juce::String ((double) buf.getNumSamples() / sr, 1) << "s)" << std::endl;
    }

    void mixInto (juce::AudioBuffer<float>& dst, const juce::AudioBuffer<float>& src, int dstStart, float gain = 1.0f)
    {
        const auto n = juce::jmin (src.getNumSamples(), dst.getNumSamples() - dstStart);
        if (n <= 0) return;
        for (int c = 0; c < juce::jmin (dst.getNumChannels(), src.getNumChannels()); ++c)
            dst.addFrom (c, dstStart, src, c, 0, n, gain);
    }

    juce::AudioBuffer<float> renderKickBed (double bpm, int bars, double sr)
    {
        BuiltInEffects::BasicDrumsProcessor drums;
        drums.setPlayConfigDetails (0, 2, sr, 512);
        drums.prepareToPlay (sr, 512);
        const auto beatSamp = (int) std::llround (sr * 60.0 / bpm);
        const auto total = beatSamp * 4 * bars;
        juce::AudioBuffer<float> out (2, total);
        out.clear();
        for (int pos = 0; pos < total; pos += 512)
        {
            const auto n = juce::jmin (512, total - pos);
            juce::AudioBuffer<float> blk (2, n);
            blk.clear();
            juce::MidiBuffer m;
            for (int beat = 0; beat < 4 * bars; ++beat)
            {
                const auto s = beat * beatSamp;
                if (s >= pos && s < pos + n)
                    m.addEvent (juce::MidiMessage::noteOn (10, 36, 0.95f), s - pos);
                const auto offHat = s + beatSamp / 2;
                if (offHat >= pos && offHat < pos + n)
                    m.addEvent (juce::MidiMessage::noteOn (10, 42, 0.55f), offHat - pos);
            }
            drums.processBlock (blk, m);
            mixInto (out, blk, pos, 0.85f);
        }
        return out;
    }
}

void renderRelease023Demos (const juce::File& folder)
{
    folder.createDirectory();
    const double sr = 48000.0;
    std::cout << "Rendering Torqwave 0.23.0 audio demos into " << folder.getFullPathName() << std::endl;

    // 1. Synth Built-In Tempo-Synced Arpeggiator + 4-Scene Continuous Evolve Morphing
    {
        const double bpm = 128.0;
        DemoPlayHead ph; ph.bpm = bpm; ph.sampleRate = sr;
        BuiltInEffects::BasicSynthProcessor synth;
        synth.setPlayHead (&ph);
        synth.setPlayConfigDetails (0, 2, sr, 512);
        synth.prepareToPlay (sr, 512);

        // Capture 4 distinct Complex/factory scenes
        synth.loadPatch ("Velvet Roller");  synth.captureScene (0);
        synth.loadPatch ("Strobe Pluck");   synth.captureScene (1);
        synth.loadPatch ("Palazzo Chord");  synth.captureScene (2);
        synth.loadPatch ("Elysian Bloom");  synth.captureScene (3);
        synth.recallScene (0);

        setById (synth, "arpOn", 1.0f, true);
        setById (synth, "arpMode", 2.0f);     // Up-Down
        setById (synth, "arpDivision", 4.0f); // 1/16
        setById (synth, "arpOctaves", 2.0f);
        setById (synth, "arpGate", 72.0f);
        setById (synth, "arpSwing", 58.0f);

        const auto beatSamp = (int) std::llround (sr * 60.0 / bpm);
        const auto barSamp = beatSamp * 4;
        const int bars = 8;
        const auto total = barSamp * bars;
        juce::AudioBuffer<float> out (2, total);
        out.clear();

        // 4 chords (2 bars each): Am9 -> Fmaj9 -> Dm9 -> Em7
        const std::vector<std::vector<int>> chords = {
            { 45, 52, 55, 59, 60 },
            { 41, 48, 52, 55, 57 },
            { 38, 45, 48, 52, 53 },
            { 40, 47, 50, 55, 59 }
        };

        for (int pos = 0; pos < total; pos += 512)
        {
            const auto n = juce::jmin (512, total - pos);
            ph.samplePos = pos;
            // Smoothly morph Evolve from 0% -> 100% across the 8 bars
            setById (synth, "evolve", (float) pos / (float) total, true);

            juce::MidiBuffer m;
            for (int c = 0; c < 4; ++c)
            {
                const auto onAt = c * 2 * barSamp;
                const auto offAt = (c + 1) * 2 * barSamp - 64;
                if (onAt >= pos && onAt < pos + n)
                    for (int p : chords[(size_t) c])
                        m.addEvent (juce::MidiMessage::noteOn (1, p, 0.85f), onAt - pos);
                if (offAt >= pos && offAt < pos + n)
                    for (int p : chords[(size_t) c])
                        m.addEvent (juce::MidiMessage::noteOff (1, p), offAt - pos);
            }
            juce::AudioBuffer<float> blk (2, n);
            blk.clear();
            synth.processBlock (blk, m);
            mixInto (out, blk, pos);
        }
        writeDemoWav (folder.getChildFile ("01-synth-arp-and-4scene-evolve-morph.wav"), out, sr);
    }

    // 2. Live Creative FX: Tape/Ping-Pong Delay (with wet ducking) + Volume Shaper (Club Pump + Low-Band Split)
    {
        const double bpm = 132.0;
        DemoPlayHead ph; ph.bpm = bpm; ph.sampleRate = sr;
        BuiltInEffects::BasicSynthProcessor synth;
        synth.setPlayHead (&ph);
        synth.setPlayConfigDetails (0, 2, sr, 512);
        synth.prepareToPlay (sr, 512);
        synth.loadPatch ("Palazzo Chord");

        BuiltInEffects::DelayProcessor delay;
        delay.setPlayHead (&ph);
        delay.setPlayConfigDetails (2, 2, sr, 512);
        delay.prepareToPlay (sr, 512);
        setById (delay, "mode", 2.0f);       // Tape
        setById (delay, "sync", 1.0f, true);
        setById (delay, "division", 7.0f);   // 1/8 D
        setById (delay, "feedback", 58.0f);
        setById (delay, "wow", 32.0f);
        setById (delay, "drive", 28.0f);
        setById (delay, "ducking", 65.0f);
        setById (delay, "mix", 48.0f);

        BuiltInEffects::VolumeShaperProcessor pump;
        pump.setPlayHead (&ph);
        pump.setPlayConfigDetails (2, 2, sr, 512);
        pump.prepareToPlay (sr, 512);
        setById (pump, "shape", 0.0f);       // Club Pump
        setById (pump, "sync", 1.0f, true);
        setById (pump, "division", 5.0f);    // 1/4
        setById (pump, "depth", 88.0f);
        setById (pump, "splitHz", 160.0f);   // Keep sub punch intact while pumping upper spectrum

        const auto beatSamp = (int) std::llround (sr * 60.0 / bpm);
        const auto barSamp = beatSamp * 4;
        const int bars = 6; // 2 bars dry, 4 bars wet Delay + Club Pump + Kick
        const auto total = barSamp * bars;
        juce::AudioBuffer<float> out (2, total);
        out.clear();
        const auto kick = renderKickBed (bpm, bars, sr);

        for (int pos = 0; pos < total; pos += 512)
        {
            const auto n = juce::jmin (512, total - pos);
            ph.samplePos = pos;
            juce::MidiBuffer m;
            // Syncopated club stabs on beat 1 and beat 2.75 of every bar
            for (int bar = 0; bar < bars; ++bar)
            {
                const int root = (bar % 4 < 2) ? 45 : (bar % 4 == 2 ? 41 : 43);
                const int hits[2] = { bar * barSamp, bar * barSamp + (beatSamp * 11) / 4 };
                for (int h : hits)
                {
                    if (h >= pos && h < pos + n)
                        for (int iv : { 0, 3, 7, 10 })
                            m.addEvent (juce::MidiMessage::noteOn (1, root + iv, 0.88f), h - pos);
                    const int off = h + beatSamp / 2;
                    if (off >= pos && off < pos + n)
                        for (int iv : { 0, 3, 7, 10 })
                            m.addEvent (juce::MidiMessage::noteOff (1, root + iv), off - pos);
                }
            }
            juce::AudioBuffer<float> blk (2, n);
            blk.clear();
            synth.processBlock (blk, m);
            if (pos >= 2 * barSamp)
            {
                juce::MidiBuffer empty;
                delay.processBlock (blk, empty);
                pump.processBlock (blk, empty);
            }
            mixInto (out, blk, pos, 0.85f);
        }
        // Bring in the kick bed after the 2 dry bars
        for (int c = 0; c < 2; ++c)
            out.addFrom (c, 2 * barSamp, kick, c, 2 * barSamp, total - 2 * barSamp, 0.8f);

        writeDemoWav (folder.getChildFile ("02-creative-fx-tape-delay-and-volume-shaper.wav"), out, sr);
    }

    // 3. Live Creative FX: 4x-Oversampled Distortion (Clean -> Tube -> Wavefold -> Bitcrush) + Auto-Filter
    {
        const double bpm = 130.0;
        DemoPlayHead ph; ph.bpm = bpm; ph.sampleRate = sr;
        BuiltInEffects::BasicSynthProcessor synth;
        synth.setPlayHead (&ph);
        synth.setPlayConfigDetails (0, 2, sr, 512);
        synth.prepareToPlay (sr, 512);
        synth.loadPatch ("Velvet Roller");

        BuiltInEffects::DistortionProcessor dist;
        dist.setPlayConfigDetails (2, 2, sr, 512);
        dist.prepareToPlay (sr, 512);
        setById (dist, "oversample", 1.0f);  // 4x
        setById (dist, "driveDb", 20.0f);
        setById (dist, "bits", 6.0f);
        setById (dist, "downsample", 4.0f);

        BuiltInEffects::AutoFilterProcessor af;
        af.setPlayHead (&ph);
        af.setPlayConfigDetails (2, 2, sr, 512);
        af.prepareToPlay (sr, 512);
        setById (af, "filterType", 1.0f);    // LP24
        setById (af, "cutoff", 550.0f);
        setById (af, "resonance", 52.0f);
        setById (af, "sync", 1.0f, true);
        setById (af, "division", 3.0f);      // 1/2
        setById (af, "lfoDepth", 62.0f);
        setById (af, "envAmount", 35.0f);

        const auto stepSamp = (int) std::llround (sr * 60.0 / bpm / 4.0); // 16th
        const int bars = 8; // 2 bars clean, 2 Tube, 2 Wavefold, 2 Bitcrush + Auto-Filter
        const auto total = stepSamp * 16 * bars;
        juce::AudioBuffer<float> out (2, total);
        out.clear();
        const int riff[16] = { 33, -1, 33, 36, 33, -1, 40, -1, 33, 33, 45, -1, 43, -1, 40, 36 };

        for (int pos = 0; pos < total; pos += 512)
        {
            const auto n = juce::jmin (512, total - pos);
            ph.samplePos = pos;
            const int bar = pos / (stepSamp * 16);
            juce::MidiBuffer m;
            for (int step = 0; step < 16 * bars; ++step)
            {
                const int p = riff[step % 16];
                if (p < 0) continue;
                const int on = step * stepSamp;
                const int off = on + (stepSamp * 3) / 4;
                if (on >= pos && on < pos + n) m.addEvent (juce::MidiMessage::noteOn (1, p, 0.9f), on - pos);
                if (off >= pos && off < pos + n) m.addEvent (juce::MidiMessage::noteOff (1, p), off - pos);
            }
            juce::AudioBuffer<float> blk (2, n);
            blk.clear();
            synth.processBlock (blk, m);
            juce::MidiBuffer empty;
            if (bar >= 2 && bar < 4)      { setById (dist, "mode", 1.0f); setById (dist, "mix", 85.0f); dist.processBlock (blk, empty); }
            else if (bar >= 4 && bar < 6) { setById (dist, "mode", 4.0f); setById (dist, "mix", 70.0f); dist.processBlock (blk, empty); }
            else if (bar >= 6)            { setById (dist, "mode", 5.0f); setById (dist, "mix", 75.0f); dist.processBlock (blk, empty); }
            if (bar >= 2)
                af.processBlock (blk, empty);
            mixInto (out, blk, pos);
        }
        writeDemoWav (folder.getChildFile ("03-creative-fx-distortion-4x-and-autofilter.wav"), out, sr);
    }

    // 4. Live Creative FX: Tremolo & Auto-Pan (180 deg stereo pan on 9-osc Elysian Bloom pad)
    {
        const double bpm = 124.0;
        DemoPlayHead ph; ph.bpm = bpm; ph.sampleRate = sr;
        BuiltInEffects::BasicSynthProcessor synth;
        synth.setPlayHead (&ph);
        synth.setPlayConfigDetails (0, 2, sr, 512);
        synth.prepareToPlay (sr, 512);
        synth.loadPatch ("Elysian Bloom");

        BuiltInEffects::TremoloProcessor trem;
        trem.setPlayHead (&ph);
        trem.setPlayConfigDetails (2, 2, sr, 512);
        trem.prepareToPlay (sr, 512);
        setById (trem, "sync", 1.0f, true);
        setById (trem, "division", 6.0f);    // 1/8
        setById (trem, "depth", 85.0f);
        setById (trem, "stereoPhase", 180.0f);

        const auto barSamp = (int) std::llround (sr * 60.0 / bpm * 4.0);
        const auto total = barSamp * 4;
        juce::AudioBuffer<float> out (2, total);
        out.clear();
        for (int pos = 0; pos < total; pos += 512)
        {
            const auto n = juce::jmin (512, total - pos);
            ph.samplePos = pos;
            juce::MidiBuffer m;
            if (pos == 0)
                for (int p : { 45, 52, 55, 59, 64 }) m.addEvent (juce::MidiMessage::noteOn (1, p, 0.85f), 0);
            if (pos <= total - 4800 && pos + n > total - 4800)
                for (int p : { 45, 52, 55, 59, 64 }) m.addEvent (juce::MidiMessage::noteOff (1, p), (total - 4800) - pos);
            juce::AudioBuffer<float> blk (2, n);
            blk.clear();
            synth.processBlock (blk, m);
            juce::MidiBuffer empty;
            trem.processBlock (blk, empty);
            mixInto (out, blk, pos);
        }
        writeDemoWav (folder.getChildFile ("04-creative-fx-tremolo-autopan-pad.wav"), out, sr);
    }

    // 5. Live MIDI FX Chain: Scale Lock -> Chord Trigger (Min 9) -> MIDI Arp -> Note Echo (+12 st) -> Synth
    {
        const double bpm = 126.0;
        DemoPlayHead ph; ph.bpm = bpm; ph.sampleRate = sr;
        BuiltInEffects::MidiScaleProcessor scale;
        BuiltInEffects::MidiChordProcessor chord;
        BuiltInEffects::MidiArpProcessor arp;
        BuiltInEffects::MidiEchoProcessor echo;
        BuiltInEffects::BasicSynthProcessor synth;

        for (juce::AudioProcessor* p : std::initializer_list<juce::AudioProcessor*> { &scale, &chord, &arp, &echo, &synth })
        {
            p->setPlayHead (&ph);
            p->setPlayConfigDetails (0, 2, sr, 512);
            p->prepareToPlay (sr, 512);
        }
        synth.loadPatch ("Strobe Pluck");
        setById (scale, "root", 9.0f);       // A
        setById (scale, "mode", 1.0f);       // Minor
        setById (chord, "chordType", 6.0f);  // Min 9
        setById (arp, "mode", 2.0f);         // Up-Down
        setById (arp, "division", 8.0f);     // 1/16
        setById (arp, "octaves", 2.0f);
        setById (arp, "swing", 56.0f);
        setById (echo, "sync", 1.0f, true);
        setById (echo, "division", 7.0f);    // 1/8 D
        setById (echo, "repeats", 3.0f);
        setById (echo, "decay", 62.0f);
        setById (echo, "pitchShift", 12.0f); // Octave shimmer echoes

        const auto barSamp = (int) std::llround (sr * 60.0 / bpm * 4.0);
        const int roots[4] = { 45, 41, 38, 40 }; // Single root notes only!
        const auto total = barSamp * 4;
        juce::AudioBuffer<float> out (2, total);
        out.clear();

        for (int pos = 0; pos < total; pos += 512)
        {
            const auto n = juce::jmin (512, total - pos);
            ph.samplePos = pos;
            juce::MidiBuffer m;
            for (int b = 0; b < 4; ++b)
            {
                const int on = b * barSamp;
                const int off = (b + 1) * barSamp - 128;
                if (on >= pos && on < pos + n) m.addEvent (juce::MidiMessage::noteOn (1, roots[b], 0.85f), on - pos);
                if (off >= pos && off < pos + n) m.addEvent (juce::MidiMessage::noteOff (1, roots[b]), off - pos);
            }
            juce::AudioBuffer<float> blk (2, n);
            blk.clear();
            scale.processBlock (blk, m);
            chord.processBlock (blk, m);
            arp.processBlock (blk, m);
            echo.processBlock (blk, m);
            synth.processBlock (blk, m);
            mixInto (out, blk, pos);
        }
        writeDemoWav (folder.getChildFile ("05-midi-fx-chain-chord-arp-echo.wav"), out, sr);
    }

    // 6. Sequencer Ratchets (1x/2x/4x/8x rolls) + MPC 62% Groove Template on Basic Drums
    {
        const double bpm = 130.0;
        const auto stepSamp = (juce::int64) std::llround (sr * 60.0 / bpm / 4.0);
        const auto barSamp = stepSamp * 16;

        NoteClip clip;
        clip.id = 1;
        clip.position = 0;
        clip.length = barSamp * 4;
        clip.loopLength = barSamp * 2;
        for (int step = 0; step < 32; ++step)
        {
            const auto t = step * stepSamp;
            if (step % 4 == 0)
                clip.notes.push_back ({ 36, t, stepSamp, 0.95f }); // Kick
            if (step % 8 == 4)
                clip.notes.push_back ({ 38, t, stepSamp, 0.88f }); // Snare
            // Hi-hat with ratchets on fill steps
            ClipNote hat { 42, t, stepSamp, (step % 2 == 0) ? 0.72f : 0.45f };
            if (step == 14)      hat.ratchets = 2;
            else if (step == 15) hat.ratchets = 4;
            else if (step == 30) hat.ratchets = 4;
            else if (step == 31) hat.ratchets = 8;
            clip.notes.push_back (hat);
            // Snare roll ratchet at end of bar 2
            if (step == 29)
            {
                ClipNote roll { 38, t, stepSamp * 2, 0.78f };
                roll.ratchets = 4;
                clip.notes.push_back (roll);
            }
        }
        // Apply MPC 62% groove template!
        const auto grooves = NoteTools::groovePresets();
        NoteTools::applyGroove (clip.notes, {}, grooves[3], (double) (stepSamp * 4), 1.0f, clip.length);

        BuiltInEffects::BasicDrumsProcessor drums;
        drums.setPlayConfigDetails (0, 2, sr, 512);
        drums.prepareToPlay (sr, 512);
        const auto total = (int) clip.length;
        juce::AudioBuffer<float> out (2, total);
        out.clear();
        for (int pos = 0; pos < total; pos += 512)
        {
            const auto n = juce::jmin (512, total - pos);
            juce::MidiBuffer m;
            clip.addMidi (pos, n, m);
            juce::AudioBuffer<float> blk (2, n);
            blk.clear();
            drums.processBlock (blk, m);
            mixInto (out, blk, pos);
        }
        writeDemoWav (folder.getChildFile ("06-sequencer-ratchets-and-mpc-groove.wav"), out, sr);
    }

    // 7. Audio to MIDI: Original melody clip -> pause -> Extracted MIDI replayed by Torqwave Synth
    {
        const double bpm = 124.0;
        const auto beatSamp = (int) std::llround (sr * 60.0 / bpm);
        const int notes[8] = { 57, 60, 64, 67, 65, 64, 60, 57 }; // A minor arpeggiated phrase
        const auto phraseLen = beatSamp * 4;
        juce::AudioBuffer<float> orig (2, phraseLen);
        orig.clear();
        double phase = 0.0;
        for (int i = 0; i < 8; ++i)
        {
            const auto start = i * (beatSamp / 2);
            const auto len = (beatSamp * 4) / 10;
            const double freq = 440.0 * std::pow (2.0, (notes[i] - 69) / 12.0);
            for (int k = 0; k < len && start + k < phraseLen; ++k)
            {
                const float env = juce::jmin (1.0f, (float) k / 240.0f) * std::exp (-3.5f * (float) k / (float) len);
                const float s = 0.55f * env * (float) (std::sin (phase) + 0.25 * std::sin (phase * 2.0));
                phase += juce::MathConstants<double>::twoPi * freq / sr;
                orig.setSample (0, start + k, s);
                orig.setSample (1, start + k, s);
            }
        }
        auto snap = AudioEdits::fromBuffer (juce::AudioBuffer<float> (orig));
        const auto extracted = AudioToMidi::extractMelody (*snap, 0, snap->getNumFrames(), sr);

        // Replay extracted MIDI through Torqwave Synth ("Strobe Pluck")
        BuiltInEffects::BasicSynthProcessor synth;
        synth.setPlayConfigDetails (0, 2, sr, 512);
        synth.prepareToPlay (sr, 512);
        synth.loadPatch ("Strobe Pluck");

        NoteClip extractedClip;
        extractedClip.id = 2;
        extractedClip.position = 0;
        extractedClip.length = phraseLen + beatSamp;
        extractedClip.notes = extracted.notes;

        const auto gap = beatSamp / 2;
        const auto total = phraseLen + gap + (int) extractedClip.length;
        juce::AudioBuffer<float> out (2, total);
        out.clear();
        mixInto (out, orig, 0, 0.9f);
        for (int pos = 0; pos < (int) extractedClip.length; pos += 512)
        {
            const auto n = juce::jmin (512, (int) extractedClip.length - pos);
            juce::MidiBuffer m;
            extractedClip.addMidi (pos, n, m);
            juce::AudioBuffer<float> blk (2, n);
            blk.clear();
            synth.processBlock (blk, m);
            mixInto (out, blk, phraseLen + gap + pos, 1.0f);
        }
        writeDemoWav (folder.getChildFile ("07-audio-to-midi-melody-resynthesis.wav"), out, sr);
    }

    // 8. Track-Level Modulator: LFO sweeping AutoFilter cutoff live on Afterlife Anthem
    {
        const double bpm = 128.0;
        BuiltInEffects::BasicSynthProcessor synth;
        synth.setPlayConfigDetails (0, 2, sr, 512);
        synth.prepareToPlay (sr, 512);
        synth.loadPatch ("Afterlife Anthem");

        TrackEffects fx;
        fx.prepare (sr);
        auto af = std::make_unique<BuiltInEffects::AutoFilterProcessor>();
        setById (*af, "filterType", 1.0f);  // LP24
        setById (*af, "cutoff", 700.0f);
        setById (*af, "resonance", 48.0f);
        setById (*af, "lfoDepth", 0.0f);    // Internal LFO OFF — purely driven by Track Modulator!
        const auto afSlot = fx.add (1, std::move (af));

        SessionTrack track;
        track.id = 1;
        TrackModulator mod;
        mod.id = 1;
        mod.kind = TrackModulator::Kind::lfo;
        mod.targetSlotId = afSlot;
        mod.targetParameterId = "cutoff";
        mod.sync = false;
        mod.rateHz = bpm / 120.0; // Half-bar rhythmic sweep
        mod.amount = 0.42f;
        track.modulators.push_back (mod);

        const auto barSamp = (int) std::llround (sr * 60.0 / bpm * 4.0);
        const auto total = barSamp * 4;
        juce::AudioBuffer<float> out (2, total);
        out.clear();

        for (int pos = 0; pos < total; pos += 512)
        {
            const auto n = juce::jmin (512, total - pos);
            juce::MidiBuffer m;
            if (pos == 0)
                for (int p : { 45, 52, 57, 60, 64 }) m.addEvent (juce::MidiMessage::noteOn (1, p, 0.85f), 0);
            if (pos <= total - 4800 && pos + n > total - 4800)
                for (int p : { 45, 52, 57, 60, 64 }) m.addEvent (juce::MidiMessage::noteOff (1, p), (total - 4800) - pos);
            juce::AudioBuffer<float> blk (2, n);
            blk.clear();
            synth.processBlock (blk, m);
            fx.process (track, pos, blk, n, {});
            mixInto (out, blk, pos);
        }
        writeDemoWav (folder.getChildFile ("08-track-modulator-lfo-filter-sweep.wav"), out, sr);
    }

    std::cout << "Done! 8 demos written to " << folder.getFullPathName() << std::endl;
}
