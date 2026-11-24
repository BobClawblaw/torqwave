#include <JuceHeader.h>
#include "../Source/AudioEdits.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/ClickRepair.h"
#include "../Source/DrumMachine.h"
#include "../Source/EffectPresets.h"
#include "../Source/InstrumentKeys.h"
#include "../Source/LossyFiles.h"
#include "../Source/Loudness.h"
#include "../Source/Maximizer.h"
#include "../Source/NoiseReduction.h"
#include "../Source/Restoration.h"
#include "../Source/Sampler.h"
#include "../Source/SequenceLibrary.h"
#include "../Source/SequencePreview.h"
#include "../Source/SpectralRecovery.h"
#include "../Source/SwarmGuitar.h"
#include "../Source/WaveSynth.h"
#include "SoundDemos.h"

namespace
{
    constexpr double rate = 48000.0, bpm = 120.0, beat = 60.0 / bpm, bar = beat * 4.0;

    struct Note { double at, length; int pitch; float velocity; };
    using Part = std::vector<Note>;

    /** The session a demo plays in: at its tempo, playing, from the start. */
    struct Session final : juce::AudioPlayHead
    {
        explicit Session (double tempoToUse) : tempo (tempoToUse) {}

        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo p;
            p.setBpm (tempo);
            p.setIsPlaying (true);
            return p;
        }

        double tempo;
    };

    /** A note in seconds. */
    struct Hit { double at, length; int pitch; float velocity; };

    /** Plays the notes through the instrument in a session at the tempo, for so many seconds, in blocks of 512. */
    juce::AudioBuffer<float> play (juce::AudioProcessor& instrument, const std::vector<Hit>& hits, double seconds, double tempo)
    {
        Session session (tempo);
        instrument.setPlayHead (&session);
        instrument.setPlayConfigDetails (0, 2, rate, 512);
        instrument.setNonRealtime (true);
        instrument.prepareToPlay (rate, 512);

        struct Event { int frame; juce::MidiMessage message; };
        std::vector<Event> events;

        for (auto& n : hits)
        {
            events.push_back ({ (int) std::lround (n.at * rate), juce::MidiMessage::noteOn (1, n.pitch, n.velocity) });
            events.push_back ({ (int) std::lround ((n.at + n.length) * rate) - 32, juce::MidiMessage::noteOff (1, n.pitch) });
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

    /** A part written in beats, at 120 BPM. */
    juce::AudioBuffer<float> play (juce::AudioProcessor& instrument, const Part& part, double seconds)
    {
        std::vector<Hit> hits;

        for (auto& n : part)
            hits.push_back ({ n.at * beat, n.length * beat, n.pitch, n.velocity });

        return play (instrument, hits, seconds, bpm);
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

    //==============================================================================
    // What was written: a line for each clip in index.json, which the Sounds pages are made from.
    struct Index
    {
        juce::Array<juce::var> entries;

        juce::DynamicObject* add (const juce::String& group, const juce::String& category, const juce::String& name, const juce::String& file)
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("group", group);
            o->setProperty ("category", category);
            o->setProperty ("name", name);
            o->setProperty ("file", file);
            entries.add (juce::var (o));
            return o;
        }
    };

    /** A name as a file's: lower case, a hyphen for anything but letters and digits. */
    juce::String slug (const juce::String& name)
    {
        juce::String out;

        for (auto c : name.toLowerCase())
            if (juce::CharacterFunctions::isLetterOrDigit (c) && c < 128)
                out << juce::String::charToString (c);
            else if (! out.endsWithChar ('-') && out.isNotEmpty())
                out << "-";

        return out.trimCharactersAtEnd ("-");
    }

    double peakDb (const juce::AudioBuffer<float>& audio)
    {
        return juce::Decibels::gainToDecibels (audio.getMagnitude (0, audio.getNumSamples()), -100.0f);
    }

    /** Writes the clip as an MP3 (variable bit rate, quality 4) and notes its length, peak and size. A clip
        still sounding where it stops is faded over its last 30 ms, so the cut does not click: the
        only thing done to a render. */
    bool save (const juce::File& folder, juce::DynamicObject* entry, juce::AudioBuffer<float> audio)
    {
        const auto file = folder.getChildFile (entry->getProperty ("file").toString());
        file.getParentDirectory().createDirectory();
        const auto frames = audio.getNumSamples();
        const auto fade = juce::jmin (frames, (int) (0.03 * rate));
        const auto faded = frames > 0 && audio.getMagnitude (frames - fade, fade) > 0.001f;

        if (faded)
            audio.applyGainRamp (frames - fade, fade, 1.0f, 0.0f);

        entry->setProperty ("seconds", juce::roundToInt (frames / rate * 10.0) / 10.0);
        entry->setProperty ("peakDb", juce::roundToInt (peakDb (audio) * 10.0) / 10.0);
        entry->setProperty ("faded", faded);

        LossyFiles::Settings mp3;
        mp3.variable = true;
        mp3.vbrQuality = 4;
        const auto error = LossyFiles::write (*AudioEdits::fromBuffer (std::move (audio)), rate, file, mp3);
        entry->setProperty ("bytes", file.getSize());
        std::cout << (error.isEmpty() ? entry->getProperty ("file").toString() + " (peak " + entry->getProperty ("peakDb").toString() + " dB)"
                                      : "FAILED " + file.getFileName() + ": " + error) << std::endl;
        return error.isEmpty();
    }

    //==============================================================================
    /** A sound on one of Torqwave's own audition phrases (written at 125 BPM), as Audition plays it:
        the phrase round twice when it is short, and time after it for the sound to ring out. */
    juce::AudioBuffer<float> audition (juce::AudioProcessor& instrument, const InstrumentKeys::Pattern& pattern, double tail)
    {
        auto end = 0.0;

        for (auto& n : pattern.notes)
            end = juce::jmax (end, n.startMs + n.lengthMs);

        const auto beatMs = 480.0;
        const auto round = pattern.lengthMs > 0.0 ? pattern.lengthMs : juce::jmax (2.0, std::ceil (end / beatMs)) * beatMs;
        const auto rounds = round < 3500.0 ? 2 : 1;
        std::vector<Hit> hits;

        for (int r = 0; r < rounds; ++r)
            for (auto& n : pattern.notes)
                hits.push_back ({ (r * round + n.startMs) * 0.001, n.lengthMs * 0.001, n.note, n.velocity });

        return play (instrument, hits, rounds * round * 0.001 + tail, 125.0);
    }

    /** An instrument's settings, read by parameter ID. */
    struct Settings
    {
        explicit Settings (juce::AudioProcessor& p)
        {
            for (auto* parameter : p.getParameters())
                if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
                    byId[ranged->getParameterID()] = ranged;
        }

        double number (const juce::String& id) const
        {
            const auto found = byId.find (id);
            return found != byId.end() ? (double) found->second->convertFrom0to1 (found->second->getValue()) : 0.0;
        }

        juce::String choice (const juce::String& id) const
        {
            const auto found = byId.find (id);

            if (found != byId.end())
                if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (found->second))
                    return c->getCurrentChoiceName();

            return {};
        }

        bool on (const juce::String& id) const   { return number (id) > 0.5; }

        std::map<juce::String, juce::RangedAudioParameter*> byId;
    };

    juce::String hertz (double hz)
    {
        return hz >= 1000.0 ? juce::String (hz / 1000.0, hz >= 10000.0 ? 0 : 1) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
    }

    /** What a synth patch is made of, read off the synth with the patch loaded: its oscillators, filters,
        effects and macros. */
    juce::String describe (BuiltInEffects::BasicSynthProcessor& synth, const WaveSynth::Patch& patch)
    {
        const Settings p (synth);
        juce::StringArray sources, filters, effects, macros;

        for (int o = 1; o <= 3; ++o)
        {
            const auto id = "osc" + juce::String (o);

            if (! p.on (id + "On"))
                continue;

            auto text = p.choice (id + "Table");

            if (text == "Analog")
            {
                const char* waves[] = { "sine", "triangle", "saw", "square" };
                text << " " << waves[juce::jlimit (0, 3, juce::roundToInt (p.number (id + "Position") / 33.33))];
            }

            if (const auto voices = juce::roundToInt (p.number (id + "Unison")); voices > 1)
                text << " x" << voices;

            if (const auto pitch = juce::roundToInt (p.number (id + "Pitch")); pitch != 0)
                text << (pitch == 12 ? " an octave up" : pitch == -12 ? " an octave down" : pitch == 24 ? " two octaves up" : pitch == -24 ? " two octaves down"
                                     : juce::String (pitch > 0 ? " +" : " ") + juce::String (pitch) + " st");

            if (const auto warp = p.choice (id + "Warp"); warp.isNotEmpty() && warp != "Off")
                text << " (" << warp << " warp)";

            sources.add (text);
        }

        if (p.number ("subLevel") > 0.5)
            sources.add ("a sub");

        if (p.number ("noiseLevel") > 0.5)
            sources.add (p.choice ("noiseType").toLowerCase() + " noise");

        for (int f = 1; f <= 2; ++f)
        {
            const auto id = "filter" + juce::String (f);

            if (const auto type = p.choice (id + "Type"); type.isNotEmpty() && type != "Off")
                filters.add (type + " at " + hertz (p.number (id + "Cutoff")));
        }

        if (p.on ("fxDriveOn") && p.number ("fxDriveAmount") > 0.5)   effects.add (p.choice ("fxDriveType").toLowerCase() + " drive");
        if (p.on ("fxChorusOn") && p.number ("fxChorusMix") > 0.5)    effects.add ("chorus");
        if (p.on ("fxPhaserOn") && p.number ("fxPhaserMix") > 0.5)    effects.add ("phaser");
        if (p.on ("fxDelayOn") && p.number ("fxDelayMix") > 0.5)      effects.add (p.choice ("fxDelayDivision") + " delay");
        if (p.on ("fxReverbOn") && p.number ("fxReverbMix") > 0.5)    effects.add ("reverb");

        for (auto& name : patch.macroNames)
            if (name.isNotEmpty())
                macros.add (name);

        juce::String text;
        text << sources.joinIntoString (", ") << ".";

        if (! filters.isEmpty())    text << " " << (filters.size() > 1 ? "Filters: " : "Filter: ") << filters.joinIntoString (" and ") << ".";
        if (const auto mode = p.choice ("voiceMode"); mode.isNotEmpty() && mode != "Poly")   text << " " << mode << ".";
        if (! effects.isEmpty())    text << " Effects: " << effects.joinIntoString (", ") << ".";
        if (! macros.isEmpty())     text << " Macros: " << macros.joinIntoString (", ") << ".";

        return text;
    }

    /** A StringSwarm preset in a line, from its values. */
    juce::String describe (BuiltInEffects::SwarmGuitarProcessor& guitar)
    {
        const Settings p (guitar);
        const auto swarm = p.choice ("source") == "Swarm";
        juce::String text;
        text << juce::roundToInt (p.number ("strings")) << (swarm ? " oscillators" : " strings") << ", " << p.choice ("tuning") << " tuning";

        if (const auto layers = juce::roundToInt (p.number ("layers")); layers > 1 && ! swarm)
            text << ", " << layers << " guitars";

        text << "; " << (p.choice ("amp") == "None" ? juce::String ("no amp") : p.choice ("amp") + " amp");

        if (const auto sync = p.choice ("strumSync"); sync.isNotEmpty() && sync != "Off")
            text << "; strummed in " << sync << " with the tempo";

        return text + ".";
    }

    /** `count` of n places in a list: the first, then spread evenly over the rest. */
    std::vector<int> spread (int n, int count)
    {
        std::vector<int> picks;

        for (int k = 0; k < juce::jmin (n, count); ++k)
        {
            const auto at = k == 0 ? 0 : n <= count ? k : 1 + (int) ((juce::int64) (n - 1) * (k - 1) / (count - 1));

            if (std::find (picks.begin(), picks.end(), at) == picks.end())
                picks.push_back (at);
        }

        return picks;
    }

    //==============================================================================
    void writeSynth (const juce::File& folder, Index& index)
    {
        const auto length = 4.0 * bar + 2.0;

        // Chosen by ear for the first Sounds page, each on a part that suits it.
        const std::vector<std::pair<const char*, const Part*>> featured {
            { "Target Lock Bass", &bassLine }, { "EBM Sequence Bass", &bassLine }, { "Warehouse Sub", &bassLine }, { "Abyss Reese", &bassLine },
            { "Acid Corrosion", &bassLine }, { "Siren Burn", &melody }, { "Mercury Lead", &melody }, { "Minor Stab", &stabs },
            { "Glass Shard", &arpeggio }, { "Wailing Fuzz Lead", &melody }, { "Body Riff Power Chord", &power }, { "Swarm Drone", &held },
            { "Concrete Pad", &minorChords }, { "Reactor Drone", &one }, { "Cold Bell", &arpeggio }, { "Gated Machine", &minorChords },
            { "Tension Riser", &one }, { "Low Boom", &hits } };

        const auto& patches = WaveSynth::factoryPatches();
        juce::StringArray done;

        for (auto& [name, part] : featured)
        {
            BuiltInEffects::BasicSynthProcessor synth;
            const auto found = std::find_if (patches.begin(), patches.end(), [&] (const WaveSynth::Patch& p) { return p.name == name; });

            if (found == patches.end() || ! synth.loadPatch (juce::String (name)))
            {
                std::cout << "FAILED no patch " << name << std::endl;
                continue;
            }

            auto* entry = index.add ("synth", found->category, name, "synth/" + slug (name) + ".mp3");
            entry->setProperty ("featured", true);
            entry->setProperty ("about", describe (synth, *found));
            entry->setProperty ("phrase", "four bars at 120 BPM");
            save (folder, entry, play (synth, *part, length));
            done.add (name);
        }

        // Every category: five patches by their place in the list (the first, then spread evenly through
        // the rest), each on one of the category's audition phrases.
        for (auto& category : WaveSynth::patchCategories())
        {
            if (category == "Init")
                continue;

            std::vector<const WaveSynth::Patch*> inCategory;

            for (auto& patch : patches)
                if (patch.category == category)
                    inCategory.push_back (&patch);

            int number = 0;

            for (auto at : spread ((int) inCategory.size(), 5))
            {
                const auto& patch = *inCategory[(size_t) at];

                if (done.contains (patch.name))
                    continue;

                BuiltInEffects::BasicSynthProcessor synth;

                if (! synth.loadPatch (patch.name))
                {
                    std::cout << "FAILED no patch " << patch.name << std::endl;
                    continue;
                }

                const auto kind = InstrumentKeys::kindFor (synth);
                const auto& patterns = InstrumentKeys::patternsFor (kind);
                const auto& pattern = patterns[(size_t) number++ % patterns.size()];
                const auto longTail = kind == InstrumentKeys::Kind::chord || kind == InstrumentKeys::Kind::held || kind == InstrumentKeys::Kind::hit;
                auto* entry = index.add ("synth", category, patch.name, "synth/" + slug (patch.name) + ".mp3");
                entry->setProperty ("about", describe (synth, patch));
                entry->setProperty ("phrase", pattern.name);
                entry->setProperty ("place", at + 1);
                entry->setProperty ("of", (int) inCategory.size());
                save (folder, entry, audition (synth, pattern, longTail ? 3.0 : 1.5));
                done.add (patch.name);
            }
        }
    }

    void writeSwarm (const juce::File& folder, Index& index)
    {
        const auto length = 4.0 * bar + 2.0;

        const std::vector<std::pair<const char*, const Part*>> featured {
            { "Tremolo Swarm Wall", &held }, { "Every String One Note", &held }, { "Low Octave Roar", &power }, { "Palm Muted Chug", &chug },
            { "Crunch Riff", &power }, { "Fifths Riff", &power }, { "Tremolo Strum", &minorChords }, { "Clean Shimmer", &arpeggio },
            { "Long Low Drone", &one }, { "Eight Voice Swarm Pulse", &sequence }, { "Racing Swarm Sixteenths", &sequence },
            { "Swarm Pulse Engine", &held }, { "Fuzzed Swarm Motor", &held }, { "Converging Swarm", &one }, { "Opening Swarm", &one },
            { "Bruised Swarm", &minorChords }, { "Gliding Swarm", &melody }, { "Smeared Swarm Lead", &melody }, { "Swarm Bass", &bassLine },
            { "Swarm Opening Out", &one }, { "Swarm Through Fuzz", &melody }, { "Wailing Swarm", &melody } };

        const auto& factory = EffectPresets::factory (BuiltInEffects::swarmGuitarId);
        juce::StringArray done, categories;

        for (auto& [name, part] : featured)
        {
            const auto found = std::find_if (factory.begin(), factory.end(), [&] (const EffectPresets::Factory& f) { return f.name == name; });

            if (found == factory.end())
            {
                std::cout << "FAILED no preset " << name << std::endl;
                continue;
            }

            BuiltInEffects::SwarmGuitarProcessor guitar;
            guitar.setPresetValues (found->values);
            auto* entry = index.add ("swarm", found->category, name, "swarm/" + slug (name) + ".mp3");
            entry->setProperty ("featured", true);
            entry->setProperty ("about", describe (guitar));
            entry->setProperty ("phrase", "four bars at 120 BPM");
            save (folder, entry, play (guitar, *part, length));
            done.add (name);
        }

        for (auto& preset : factory)
            categories.addIfNotAlreadyThere (preset.category);

        // Every category: three presets by their place in the list, on the strings' audition phrases.
        for (auto& category : categories)
        {
            std::vector<const EffectPresets::Factory*> inCategory;

            for (auto& preset : factory)
                if (preset.category == category)
                    inCategory.push_back (&preset);

            int number = 0;

            for (auto at : spread ((int) inCategory.size(), 3))
            {
                const auto& preset = *inCategory[(size_t) at];

                if (done.contains (preset.name))
                    continue;

                BuiltInEffects::SwarmGuitarProcessor guitar;
                guitar.setPresetValues (preset.values);
                const auto& patterns = InstrumentKeys::patternsFor (InstrumentKeys::Kind::strings);
                const auto& pattern = patterns[(size_t) number++ % patterns.size()];
                auto* entry = index.add ("swarm", category, preset.name, "swarm/" + slug (preset.name) + ".mp3");
                entry->setProperty ("about", describe (guitar));
                entry->setProperty ("phrase", pattern.name);
                entry->setProperty ("place", at + 1);
                entry->setProperty ("of", (int) inCategory.size());
                save (folder, entry, audition (guitar, pattern, 2.5));
                done.add (preset.name);
            }
        }
    }

    /** Every Basic Drums kit on two of the kits' audition beats. */
    void writeDrums (const juce::File& folder, Index& index)
    {
        const auto& patterns = InstrumentKeys::patternsFor (InstrumentKeys::Kind::kit);
        int number = 0;

        for (auto& kit : DrumMachine::kits())
        {
            for (auto which : { 0, 1 + number % juce::jmax (1, (int) patterns.size() - 1) })
            {
                const auto& pattern = patterns[(size_t) which % patterns.size()];
                BuiltInEffects::BasicDrumsProcessor drums;
                drums.applyKit (kit);
                auto* entry = index.add ("drums", kit.name, kit.name + ": " + pattern.name, "drums/" + slug (kit.name + " " + pattern.name) + ".mp3");
                entry->setProperty ("about", kit.description);
                entry->setProperty ("phrase", pattern.name);
                save (folder, entry, audition (drums, pattern, 1.5));
            }

            ++number;
        }
    }

    /** Torqwave Sampler's factory presets, each on the phrase Audition gives it. */
    void writeSampler (const juce::File& folder, Index& index)
    {
        for (auto& preset : BuiltInEffects::samplerPresets())
        {
            BuiltInEffects::SamplerProcessor sampler;

            if (! sampler.loadFactory (preset.name))
            {
                std::cout << "FAILED no sampler preset " << preset.name << std::endl;
                continue;
            }

            const auto kind = InstrumentKeys::kindFor (sampler);
            const auto& pattern = InstrumentKeys::patternsFor (kind).front();
            const auto zones = (int) sampler.getZones().size();
            auto* entry = index.add ("sampler", preset.category, preset.name, "sampler/" + slug (preset.name) + ".mp3");
            entry->setProperty ("about", sampler.getSlicing().on ? "A loop cut into " + juce::String (zones) + " slices, each on a key of its own."
                                                                 : juce::String (zones) + (zones == 1 ? " sample" : " samples") + " across the keys.");
            entry->setProperty ("phrase", pattern.name);
            save (folder, entry, audition (sampler, pattern, 2.5));
        }
    }

    /** A sequence's or a kit's loop, as the Sequence Library's preview renders it at its written tempo:
        round again until it is five seconds or more. */
    bool writeLoop (const juce::File& folder, juce::DynamicObject* entry, const std::vector<SequenceFile::Sequence>& parts, juce::AudioPluginFormatManager& formats)
    {
        const auto tempo = parts.front().writtenTempo;
        juce::AudioBuffer<float> pass;
        const auto error = SequencePreview::render (parts, tempo, parts.front().beatsPerBar, rate, formats, pass);

        if (error.isNotEmpty() || pass.getNumSamples() == 0)
        {
            std::cout << "FAILED " << entry->getProperty ("name").toString() << ": " << error << std::endl;
            return false;
        }

        const auto rounds = juce::jlimit (1, 4, (int) std::ceil (5.0 * rate / pass.getNumSamples()));
        juce::AudioBuffer<float> audio (pass.getNumChannels(), pass.getNumSamples() * rounds);

        for (int r = 0; r < rounds; ++r)
            for (int ch = 0; ch < pass.getNumChannels(); ++ch)
                audio.copyFrom (ch, r * pass.getNumSamples(), pass, ch, 0, pass.getNumSamples());

        juce::Array<juce::var> voices;

        for (auto& part : parts)
        {
            auto* v = new juce::DynamicObject();
            v->setProperty ("name", part.name);
            v->setProperty ("role", part.role);
            v->setProperty ("voice", part.voice != nullptr ? part.voice->getStringAttribute ("name") : juce::String());
            voices.add (juce::var (v));
        }

        entry->setProperty ("tempo", tempo);
        entry->setProperty ("rounds", rounds);
        entry->setProperty ("parts", voices);
        return save (folder, entry, std::move (audio));
    }

    /** The Sequence Library's factory presets: every kit, and sequences from every style (one of each role
        from the styles the library leads with, a bass line and a melody from the others; of each
        role the one with the most energy, the first by name among equals). */
    void writeLibrary (const juce::File& folder, Index& index, bool kits, bool sequences)
    {
        const auto before = SequenceLibrary::factoryFolder();
        const auto base = juce::File::createTempFile ("sounddemos-presets");
        const auto presets = base.getChildFile ("set");
        SequenceLibrary::setFactoryFolder (presets);

        if (const auto error = SequenceLibrary::unpackFactoryPresets(); error.isNotEmpty())
            std::cout << "FAILED " << error << std::endl;

        juce::AudioPluginFormatManager formats;
        formats.addFormat (BuiltInEffects::createFormat());
        auto entries = SequenceLibrary (presets).scan();
        juce::StringArray used;

        const auto fileFor = [&used] (const juce::String& group, const SequenceLibrary::Entry& e)
        {
            auto name = slug (e.style.fromFirstOccurrenceOf ("/", false, false) + " " + e.name);

            while (used.contains (group + name))
                name << "-2";

            used.add (group + name);
            return group + "/" + name + ".mp3";
        };

        const auto note = [] (juce::DynamicObject* entry, const SequenceLibrary::Entry& e)
        {
            entry->setProperty ("about", e.description);
            entry->setProperty ("key", e.key);
            entry->setProperty ("bars", e.bars);
            entry->setProperty ("energy", e.energy);
            entry->setProperty ("role", e.path.joinIntoString (" > "));
        };

        if (kits)
            for (auto& e : entries)
                if (e.isKit)
                {
                    juce::String error;

                    if (const auto kit = SequenceKit::load (e.file, error); kit.has_value() && ! kit->parts.empty())
                    {
                        auto* entry = index.add ("kits", e.style, e.name, fileFor ("kits", e));
                        note (entry, e);
                        writeLoop (folder, entry, kit->parts, formats);
                    }
                    else
                    {
                        std::cout << "FAILED " << e.name << ": " << error << std::endl;
                    }
                }

        if (sequences)
        {
            const juce::StringArray leading { "Techno/Dark", "Techno/Industrial", "Techno/Hard", "Techno/Acid", "Industrial/EBM", "Industrial/Electro-industrial",
                                              "Industrial/Dark electro", "Industrial/Aggrotech", "Industrial/Techno-body" };
            const juce::StringArray everyRole { "Drums", "Bass", "Harmony", "Pads", "Arps", "Melody", "SFX" }, twoRoles { "Bass", "Melody" };

            for (auto& style : SequenceLibrary::stylesIn (entries))
                for (auto& role : leading.contains (style) ? everyRole : twoRoles)
                {
                    const SequenceLibrary::Entry* best = nullptr;

                    for (auto& e : entries)
                        if (! e.isKit && e.style == style && e.path.size() > 0 && e.path[0] == role
                            && (best == nullptr || e.energy > best->energy || (e.energy == best->energy && e.name.compareNatural (best->name) < 0)))
                            best = &e;

                    if (best == nullptr)
                        continue;

                    juce::String error;

                    if (const auto q = SequenceFile::load (best->file, error))
                    {
                        auto* entry = index.add ("sequences", style, best->name, fileFor ("sequences", *best));
                        note (entry, *best);
                        writeLoop (folder, entry, { *q }, formats);
                    }
                    else
                    {
                        std::cout << "FAILED " << best->name << ": " << error << std::endl;
                    }
                }
        }

        SequenceLibrary::setFactoryFolder (before);

        // The unpacked files are read-only: made writable again to be deleted.
        for (const auto& f : juce::RangedDirectoryIterator (base, true, "*", juce::File::findFilesAndDirectories))
            f.getFile().setReadOnly (false);

        base.deleteRecursively();
    }
}

void writeSoundDemos (const juce::File& folder, const juce::StringArray& only)
{
    folder.createDirectory();
    const auto wanted = [&only] (const char* group) { return only.isEmpty() || only.contains (group); };
    const auto indexFile = folder.getChildFile ("index.json");
    Index index;

    // Rendering some of the groups keeps the others' lines.
    if (! only.isEmpty())
        if (const auto old = juce::JSON::parse (indexFile); old.isArray())
            for (auto& entry : *old.getArray())
                if (! only.contains (entry["group"].toString()))
                    index.entries.add (entry);

    if (wanted ("synth"))      writeSynth (folder, index);
    if (wanted ("swarm"))      writeSwarm (folder, index);
    if (wanted ("drums"))      writeDrums (folder, index);
    if (wanted ("sampler"))    writeSampler (folder, index);

    if (wanted ("kits") || wanted ("sequences"))
        writeLibrary (folder, index, wanted ("kits"), wanted ("sequences"));

    indexFile.replaceWithText (juce::JSON::toString (juce::var (index.entries)) + "\n");

    juce::int64 bytes = 0;

    for (auto& entry : index.entries)
        bytes += (juce::int64) entry["bytes"];

    std::cout << index.entries.size() << " clips, " << juce::String (bytes / 1048576.0, 1) << " MB" << std::endl;
}
