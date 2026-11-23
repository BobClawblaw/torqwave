#include <JuceHeader.h>
#include "../Source/BuiltInEffects.h"
#include "../Source/DrumMachine.h"
#include "../Source/Sampler.h"
#include "../Source/SwarmGuitar.h"
#include "../Source/WaveSynth.h"
#include "SamplerContent.h"

namespace
{
    constexpr double rate = 48000.0;

    /** An instrument to sample: a preset made of notes of one of Torqwave's own sounds. */
    struct Played
    {
        const char* name;
        const char* category;
        const char* source;            // a Torqwave Synth patch, or a StringSwarm preset
        bool swarm;
        std::vector<int> notes;
        double seconds;
        bool loop, stereo;
        float attack, release;         // ms
    };

    const std::vector<Played>& played()
    {
        static const std::vector<Played> all {
            { "Hammer Tine",     "Keys",    "Hammer Tine",           false, { 36, 48, 60, 72 }, 3.0, false, false, 0.0f,  350.0f },
            { "Concrete Pad",    "Pad",     "Concrete Pad",          false, { 36, 48, 60, 72 }, 4.0, true,  true,  60.0f, 900.0f },
            { "Iron Cello",      "Strings", "Iron Cello",            false, { 36, 48, 60 },     3.5, true,  false, 20.0f, 350.0f },
            { "Cold Bell",       "Bell",    "Cold Bell",             false, { 60, 72, 84 },     4.0, false, false, 0.0f,  800.0f },
            { "Warehouse Sub",   "Bass",    "Warehouse Sub",         false, { 28, 36, 43 },     2.5, true,  false, 0.0f,  120.0f },
            { "Dark Kalimba",    "Pluck",   "Dark Kalimba",          false, { 48, 60, 72 },     2.5, false, false, 0.0f,  300.0f },
            { "One Note Guitar", "Guitar",  "Every String One Note", true,  { 40, 52 },         5.0, false, false, 0.0f,  300.0f } };
        return all;
    }

    /** A drum loop to slice: two bars of a Basic Drums kit. Steps are sixteenths, 0 to 31. */
    struct Hit { int step, note; float velocity; };

    struct Loop
    {
        const char* name;
        const char* kit;
        double bpm;
        std::vector<Hit> hits;
    };

    std::vector<Hit> every (int from, int by, int note, float velocity)
    {
        std::vector<Hit> hits;

        for (int s = from; s < 32; s += by)
            hits.push_back ({ s, note, velocity });

        return hits;
    }

    std::vector<Hit> joined (std::initializer_list<std::vector<Hit>> parts)
    {
        std::vector<Hit> all;

        for (auto& p : parts)
            all.insert (all.end(), p.begin(), p.end());

        std::stable_sort (all.begin(), all.end(), [] (const Hit& a, const Hit& b) { return a.step < b.step; });
        return all;
    }

    const std::vector<Loop>& loops()
    {
        static const std::vector<Loop> all {
            { "Dark Techno Break", "Dark Techno", 130.0,
              joined ({ every (0, 4, 36, 1.0f), every (2, 4, 46, 0.7f), every (4, 8, 39, 0.9f), { { 7, 37, 0.6f }, { 23, 37, 0.6f }, { 29, 37, 0.5f }, { 30, 42, 0.6f }, { 31, 42, 0.8f } } }) },
            { "Industrial Break", "Industrial", 125.0,
              joined ({ { { 0, 36, 1.0f }, { 3, 36, 0.8f }, { 6, 36, 0.9f }, { 8, 36, 1.0f }, { 11, 36, 0.8f }, { 14, 36, 0.9f }, { 16, 36, 1.0f }, { 19, 36, 0.8f }, { 22, 36, 0.9f },
                          { 24, 36, 1.0f }, { 27, 36, 0.8f } },
                        every (4, 8, 38, 1.0f), every (2, 4, 42, 0.6f), { { 0, 49, 0.8f }, { 28, 45, 0.9f }, { 29, 45, 0.8f }, { 30, 41, 0.9f }, { 31, 41, 1.0f } } }) },
            { "EBM Machine Break", "EBM Machine", 120.0,
              joined ({ every (0, 4, 36, 1.0f), { { 14, 36, 0.8f }, { 30, 36, 0.8f } }, every (4, 8, 38, 1.0f), every (1, 4, 42, 0.5f), every (3, 4, 42, 0.7f),
                        { { 2, 56, 0.6f }, { 10, 56, 0.6f }, { 18, 56, 0.6f }, { 26, 56, 0.6f } } }) },
            { "Breaks Break", "Breaks", 135.0,
              joined ({ { { 0, 36, 1.0f }, { 10, 36, 0.9f }, { 16, 36, 1.0f }, { 19, 36, 0.7f }, { 26, 36, 0.9f } },
                        every (4, 8, 38, 1.0f), { { 7, 38, 0.4f }, { 9, 38, 0.4f }, { 23, 38, 0.4f }, { 25, 38, 0.5f } }, every (2, 4, 42, 0.7f), { { 14, 46, 0.8f }, { 30, 46, 0.8f } } }) } };
        return all;
    }

    /** A note held for `seconds` on an instrument, from a fresh start. */
    juce::AudioBuffer<float> hold (juce::AudioProcessor& instrument, int note, double seconds)
    {
        instrument.prepareToPlay (rate, 512);
        const auto total = (int) (seconds * rate);
        juce::AudioBuffer<float> out (2, total), block (2, 512);

        for (int at = 0; at < total; at += 512)
        {
            const auto n = juce::jmin (512, total - at);
            juce::MidiBuffer midi;

            if (at == 0)
                midi.addEvent (juce::MidiMessage::noteOn (1, note, 0.8f), 0);

            block.setSize (2, n, false, false, true);
            instrument.processBlock (block, midi);
            out.copyFrom (0, at, block, 0, 0, n);
            out.copyFrom (1, at, block, 1, 0, n);
        }

        return out;
    }

    /** Two bars of a kit. */
    juce::AudioBuffer<float> beat (const Loop& loop)
    {
        DrumMachine::Engine drums;
        drums.prepare (rate);
        drums.setTempo (loop.bpm);

        if (const auto index = DrumMachine::kitIndex (loop.kit); index >= 0)
            drums.setSettings (DrumMachine::kits()[(size_t) index].settings);

        const auto step = 60.0 / loop.bpm / 4.0;
        juce::AudioBuffer<float> out (2, (int) std::lround (32 * step * rate));
        out.clear();
        int at = 0;

        for (auto& hit : loop.hits)
        {
            const auto frame = juce::jmin (out.getNumSamples(), (int) std::lround (hit.step * step * rate));

            if (frame > at)
            {
                drums.render (out.getWritePointer (0, at), out.getWritePointer (1, at), frame - at);
                at = frame;
            }

            drums.noteOn (hit.note, hit.velocity);
        }

        drums.render (out.getWritePointer (0, at), out.getWritePointer (1, at), out.getNumSamples() - at);
        return out;
    }

    /** One channel (the two mixed) or both, scaled, its last 10 ms faded out. */
    juce::AudioBuffer<float> finished (const juce::AudioBuffer<float>& audio, bool stereo, float gain)
    {
        juce::AudioBuffer<float> out (stereo ? 2 : 1, audio.getNumSamples());

        if (stereo)
        {
            out.makeCopyOf (audio);
            out.applyGain (gain);
        }
        else
        {
            out.copyFrom (0, 0, audio, 0, 0, audio.getNumSamples());
            out.addFrom (0, 0, audio, 1, 0, audio.getNumSamples());
            out.applyGain (0.5f * gain);
        }

        const auto fade = juce::jmin (out.getNumSamples(), (int) (0.01 * rate));
        out.applyGainRamp (out.getNumSamples() - fade, fade, 1.0f, 0.0f);
        return out;
    }

    juce::int64 writeFlac (const juce::File& file, const juce::AudioBuffer<float>& audio)
    {
        file.getParentDirectory().createDirectory();
        file.deleteFile();
        std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream> (file);
        auto writer = juce::FlacAudioFormat().createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (rate).withNumChannels (audio.getNumChannels())
                                                                           .withBitsPerSample (24).withQualityOptionIndex (8));
        if (writer == nullptr || ! writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples()))
        {
            std::cout << "FAILED writing " << file.getFileName() << std::endl;
            return 0;
        }

        writer.reset();
        return file.getSize();
    }

    void set (BuiltInEffects::SamplerProcessor& sampler, const char* id, float plain)
    {
        if (auto* p = sampler.parameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (plain));
    }

    /** A preset's entry in the manifest: its settings and its zones as the sampler holds them. */
    void addPreset (juce::XmlElement& manifest, const juce::String& name, const juce::String& category, BuiltInEffects::SamplerProcessor& sampler)
    {
        auto* preset = manifest.createNewChildElement ("PRESET");
        preset->setAttribute ("name", name);
        preset->setAttribute ("category", category);
        auto* values = preset->createNewChildElement ("VALUES");

        for (auto& [id, value] : sampler.getPresetValues())
            values->setAttribute (id, value);

        if (auto zones = juce::XmlDocument::parse (sampler.getPresetTexts()["zones"]))
            preset->addChildElement (zones.release());
    }
}

void writeSamplerContent (const juce::File& folder)
{
    folder.createDirectory();

    // The samples are named relative to this folder in the manifest, as they are to the factory folder in the app.
    const auto before = Sampler::factoryFolder();
    Sampler::setFactoryFolder (folder);
    juce::XmlElement manifest ("SAMPLER_PRESETS");
    juce::int64 bytes = 0;
    int files = 0;

    for (auto& instrument : played())
    {
        // Its notes, rendered, then all scaled alike so the loudest peaks at -6 dB.
        std::vector<juce::AudioBuffer<float>> notes;
        auto loudest = 0.0f;

        for (const auto note : instrument.notes)
        {
            if (instrument.swarm)
            {
                BuiltInEffects::SwarmGuitarProcessor guitar;
                const auto& factory = EffectPresets::factory (BuiltInEffects::swarmGuitarId);
                const auto found = std::find_if (factory.begin(), factory.end(), [&] (const EffectPresets::Factory& f) { return f.name == instrument.source; });

                if (found == factory.end())
                {
                    std::cout << "FAILED no StringSwarm preset " << instrument.source << std::endl;
                    continue;
                }

                guitar.setPresetValues (found->values);
                notes.push_back (hold (guitar, note, instrument.seconds));
            }
            else
            {
                BuiltInEffects::BasicSynthProcessor synth;

                if (! synth.loadPatch (instrument.source))
                {
                    std::cout << "FAILED no Torqwave Synth patch " << instrument.source << std::endl;
                    continue;
                }

                notes.push_back (hold (synth, note, instrument.seconds));
            }

            loudest = juce::jmax (loudest, notes.back().getMagnitude (0, notes.back().getNumSamples()));
        }

        if (notes.size() != instrument.notes.size() || loudest <= 0.0f)
            continue;

        BuiltInEffects::SamplerProcessor sampler;
        std::vector<Sampler::Zone> zones;

        for (size_t i = 0; i < notes.size(); ++i)
        {
            const auto note = instrument.notes[i];
            const auto file = folder.getChildFile (instrument.category).getChildFile (juce::String (instrument.name) + " " + Instrument::pitchName (note) + ".flac");
            bytes += writeFlac (file, finished (notes[i], instrument.stereo, 0.5f / loudest));
            ++files;

            juce::String error;
            Sampler::Zone z;
            z.sample = Sampler::sharedSample (file, error);
            z.file = file;
            z.name = juce::String (instrument.name) + " " + Instrument::pitchName (note);
            z.root = note;
            z.lowKey = i == 0 ? 0 : (instrument.notes[i - 1] + note) / 2 + 1;
            z.highKey = i + 1 == notes.size() ? 127 : (note + instrument.notes[i + 1]) / 2;

            if (z.sample == nullptr)
            {
                std::cout << "FAILED reading " << file.getFileName() << ": " << error << std::endl;
                continue;
            }

            // The loop by the pitch the sound has (a sub sounds an octave under its key), or the key's when none is found.
            const auto pitch = Sampler::findPitch (*z.sample);

            if (instrument.loop)
                if (const auto loop = Sampler::findLoop (*z.sample, {}, pitch.found ? pitch.hz : 440.0 * std::pow (2.0, (note - 69) / 12.0)); loop.found)
                {
                    z.loop = 1;
                    z.loopStart = loop.start;
                    z.loopEnd = loop.end;
                    z.crossfade = loop.crossfade;
                }

            zones.push_back (std::move (z));
        }

        sampler.setZones (std::move (zones));
        set (sampler, "attack", instrument.attack);
        set (sampler, "release", instrument.release);
        addPreset (manifest, instrument.name, instrument.category, sampler);
    }

    for (auto& loop : loops())
    {
        const auto audio = beat (loop);
        const auto peak = audio.getMagnitude (0, audio.getNumSamples());

        if (peak <= 0.0f)
        {
            std::cout << "FAILED " << loop.name << " is silent" << std::endl;
            continue;
        }

        const auto file = folder.getChildFile ("Sliced Kits").getChildFile (juce::String (loop.name) + " " + juce::String (juce::roundToInt (loop.bpm)) + " BPM.flac");
        bytes += writeFlac (file, finished (audio, true, 0.7f / peak));
        ++files;

        juce::String error;
        const auto sample = Sampler::sharedSample (file, error);
        BuiltInEffects::SamplerProcessor sampler;
        Sampler::Slicing how;
        how.source = Sampler::SliceSource::hits;

        if (sample == nullptr || ! sampler.slice (sample, file, {}, how))
        {
            std::cout << "FAILED slicing " << loop.name << " " << error << std::endl;
            continue;
        }

        set (sampler, "level", -3.0f);
        addPreset (manifest, loop.name, "Sliced Kits", sampler);
    }

    const auto written = folder.getChildFile ("Presets.xml");
    written.deleteFile();
    manifest.writeTo (written);
    Sampler::setFactoryFolder (before);
    std::cout << files << " samples, " << juce::String ((double) bytes / (1024.0 * 1024.0), 2) << " MB, " << manifest.getNumChildElements() << " presets, in " << folder.getFullPathName() << std::endl;
}
