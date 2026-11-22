#include <JuceHeader.h>
#include "../Source/AudioEdits.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/EffectPresets.h"
#include "../Source/LossyFiles.h"
#include "../Source/SwarmGuitar.h"
#include "../Source/WaveSynth.h"
#include "SoundDemos.h"

namespace
{
    constexpr double rate = 48000.0, bpm = 120.0, beat = 60.0 / bpm, sixteenth = beat / 4.0, bar = beat * 4.0;

    struct Note { double at, length; int pitch; float velocity; };
    using Part = std::vector<Note>;

    /** The session the demos play in: 120 BPM, playing, from the start. */
    struct Session final : juce::AudioPlayHead
    {
        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo p;
            p.setBpm (bpm);
            p.setIsPlaying (true);
            return p;
        }
    };

    /** Plays the part through the instrument (its times in beats), for its length and a tail, in blocks of 512. */
    juce::AudioBuffer<float> play (juce::AudioProcessor& instrument, const Part& part, double seconds)
    {
        Session session;
        instrument.setPlayHead (&session);
        instrument.setPlayConfigDetails (0, 2, rate, 512);
        instrument.setNonRealtime (true);
        instrument.prepareToPlay (rate, 512);

        struct Event { int frame; juce::MidiMessage message; };
        std::vector<Event> events;

        for (auto& n : part)
        {
            events.push_back ({ (int) std::lround (n.at * beat * rate), juce::MidiMessage::noteOn (1, n.pitch, n.velocity) });
            events.push_back ({ (int) std::lround ((n.at + n.length) * beat * rate) - 32, juce::MidiMessage::noteOff (1, n.pitch) });
        }

        std::stable_sort (events.begin(), events.end(), [] (const Event& a, const Event& b)
        {
            return a.frame < b.frame || (a.frame == b.frame && a.message.isNoteOff() && ! b.message.isNoteOff());
        });

        const auto total = (int) (seconds * rate);
        juce::AudioBuffer<float> out (2, total), block (2, 512);
        size_t next = 0;

        for (int at = 0; at < total; at += 512)
        {
            const auto n = juce::jmin (512, total - at);
            juce::MidiBuffer midi;

            for (; next < events.size() && events[next].frame < at + n; ++next)
                midi.addEvent (events[next].message, juce::jmax (0, events[next].frame - at));

            block.setSize (2, n, false, false, true);
            block.clear();
            instrument.processBlock (block, midi);

            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, at, block, ch, 0, n);
        }

        instrument.setPlayHead (nullptr);
        return out;
    }

    /** A note every step of a pattern: its pitches in turn, the first of each beat accented. */
    Part steps (const std::vector<int>& pitches, double step, int count, double hold = 0.8)
    {
        Part p;

        for (int i = 0; i < count; ++i)
            p.push_back ({ i * step, step * hold, pitches[(size_t) i % pitches.size()], std::fmod (i * step, 1.0) < 1.0e-6 ? 1.0f : 0.78f });

        return p;
    }

    Part chords (const std::vector<std::vector<int>>& each, double length)
    {
        Part p;

        for (size_t i = 0; i < each.size(); ++i)
            for (auto pitch : each[i])
                p.push_back ({ (double) i * length, length * 0.95, pitch, 0.9f });

        return p;
    }

    Part line (const std::vector<std::pair<int, double>>& notes, bool legato)
    {
        Part p;
        double at = 0.0;

        for (auto [pitch, length] : notes)
        {
            p.push_back ({ at, legato ? length + 0.05 : length * 0.9, pitch, 0.9f });
            at += length;
        }

        return p;
    }

    // Four bars at 120 BPM, in beats.
    const Part bassLine = [] { Part p; const int notes[] = { 34, 34, 46, 34, 34, 37, 34, 39 }; for (int i = 0; i < 64; ++i) p.push_back ({ i * 0.25, 0.2, notes[i % 8] - (i >= 32 && i < 48 ? 2 : 0), i % 4 == 0 ? 1.0f : 0.78f }); return p; }();
    const Part sequence = steps ({ 45, 45, 57, 45, 48, 45, 52, 43 }, 0.25, 64);
    const Part arpeggio = steps ({ 64, 67, 71, 76, 71, 67, 62, 66, 69, 74, 69, 66 }, 0.5, 30, 0.9);
    const Part minorChords = chords ({ { 52, 55, 59, 64 }, { 50, 53, 57, 62 }, { 48, 52, 55, 60 }, { 47, 50, 54, 59 } }, 4.0);
    const Part stabs = [] { Part p; const double at[] = { 0, 0.75, 1.5, 2.5, 3.0 }; for (int b = 0; b < 4; ++b) for (auto t : at) p.push_back ({ b * 4.0 + t, 0.3, b == 2 ? 50 : 52, 0.9f }); return p; }();   // the patches make the chord
    const Part melody = line ({ { 64, 1.5 }, { 67, 0.5 }, { 71, 2.0 }, { 69, 1.0 }, { 67, 1.0 }, { 64, 2.0 }, { 62, 1.0 }, { 64, 1.0 }, { 67, 2.0 }, { 64, 4.0 } }, true);
    const Part power = line ({ { 40, 2.0 }, { 40, 1.0 }, { 43, 1.0 }, { 45, 2.0 }, { 43, 1.0 }, { 38, 1.0 }, { 40, 2.0 }, { 40, 1.0 }, { 43, 1.0 }, { 47, 2.0 }, { 45, 2.0 } }, false);
    const Part chug = [] { Part p; for (int i = 0; i < 64; ++i) p.push_back ({ i * 0.25, 0.18, i % 16 >= 12 ? (i % 32 >= 16 ? 38 : 43) : 40, i % 4 == 0 ? 1.0f : 0.72f }); return p; }();
    const Part held = { { 0.0, 7.5, 43, 0.95f }, { 8.0, 7.5, 45, 0.95f } };
    const Part one = { { 0.0, 15.5, 45, 0.95f } };
    const Part hits = { { 0.0, 1.0, 36, 1.0f }, { 6.0, 1.0, 33, 1.0f }, { 10.0, 1.0, 36, 0.8f } };

    void save (const juce::File& folder, const juce::String& name, const juce::AudioBuffer<float>& audio)
    {
        const auto file = folder.getChildFile (name + ".mp3");
        LossyFiles::Settings mp3;
        mp3.variable = true;
        mp3.vbrQuality = 3;
        const auto error = LossyFiles::write (*AudioEdits::fromBuffer (juce::AudioBuffer<float> (audio)), rate, file, mp3);
        const auto peak = juce::Decibels::gainToDecibels (audio.getMagnitude (0, audio.getNumSamples()), -100.0f);
        std::cout << (error.isEmpty() ? file.getFileName() + " (peak " + juce::String (peak, 1) + " dB)" : "FAILED " + file.getFileName() + ": " + error) << std::endl;
    }
}

void writeSoundDemos (const juce::File& folder)
{
    folder.createDirectory();
    const auto length = 4.0 * bar + 2.0;

    // Torqwave Synth: a patch from each kind of sound, on the part that suits it.
    const std::vector<std::pair<const char*, const Part*>> patches {
        { "Target Lock Bass", &bassLine }, { "EBM Sequence Bass", &bassLine }, { "Warehouse Sub", &bassLine }, { "Abyss Reese", &bassLine },
        { "Acid Corrosion", &bassLine }, { "Siren Burn", &melody }, { "Mercury Lead", &melody }, { "Minor Stab", &stabs },
        { "Glass Shard", &arpeggio }, { "Wailing Fuzz Lead", &melody }, { "Body Riff Power Chord", &power }, { "Swarm Drone", &held },
        { "Concrete Pad", &minorChords }, { "Reactor Drone", &one }, { "Cold Bell", &arpeggio }, { "Gated Machine", &minorChords },
        { "Tension Riser", &one }, { "Low Boom", &hits } };

    for (auto& [name, part] : patches)
    {
        BuiltInEffects::BasicSynthProcessor synth;

        if (synth.loadPatch (juce::String (name)))
            save (folder, juce::String ("Torqwave Synth - ") + name, play (synth, *part, length));
        else
            std::cout << "FAILED no patch " << name << std::endl;
    }

    // Swarm Guitar: its strings and its swarm, from the factory presets.
    const std::vector<std::pair<const char*, const Part*>> presets {
        { "Tremolo Swarm Wall", &held }, { "Every String One Note", &held }, { "Low Octave Roar", &power }, { "Palm Muted Chug", &chug },
        { "Crunch Riff", &power }, { "Fifths Riff", &power }, { "Tremolo Strum", &minorChords }, { "Clean Shimmer", &arpeggio },
        { "Long Low Drone", &one }, { "Eight Voice Swarm Pulse", &sequence }, { "Racing Swarm Sixteenths", &sequence },
        { "Swarm Pulse Engine", &held }, { "Fuzzed Swarm Motor", &held }, { "Converging Swarm", &one }, { "Opening Swarm", &one },
        { "Bruised Swarm", &minorChords }, { "Gliding Swarm", &melody }, { "Smeared Swarm Lead", &melody }, { "Swarm Bass", &bassLine },
        { "Swarm Opening Out", &one }, { "Swarm Through Fuzz", &melody }, { "Wailing Swarm", &melody } };

    const auto& factory = EffectPresets::factory (BuiltInEffects::swarmGuitarId);

    for (auto& [name, part] : presets)
    {
        const auto found = std::find_if (factory.begin(), factory.end(), [&] (const EffectPresets::Factory& f) { return f.name == name; });

        if (found == factory.end())
        {
            std::cout << "FAILED no preset " << name << std::endl;
            continue;
        }

        BuiltInEffects::SwarmGuitarProcessor guitar;
        guitar.setPresetValues (found->values);
        save (folder, juce::String ("Swarm Guitar - ") + name, play (guitar, *part, length));
    }
}
