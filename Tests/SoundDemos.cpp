#include <JuceHeader.h>
#include <numeric>
#include <set>
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
#include "../Source/Spectrogram.h"
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

    /** Writes the clip as an MP3 (variable bit rate, quality 4 unless given) and notes its length, peak and size. A clip
        still sounding where it stops is faded over its last 30 ms, so the cut does not click: the
        only thing done to a render. */
    bool save (const juce::File& folder, juce::DynamicObject* entry, juce::AudioBuffer<float> audio, int vbrQuality = 4)
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
        mp3.vbrQuality = vbrQuality;
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
        std::vector<Hit> played;

        for (int r = 0; r < rounds; ++r)
            for (auto& n : pattern.notes)
                played.push_back ({ (r * round + n.startMs) * 0.001, n.lengthMs * 0.001, n.note, n.velocity });

        return play (instrument, played, rounds * round * 0.001 + tail, 125.0);
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

    /** How much of the synth a patch uses, counted off the synth with the patch loaded: a point for each
        oscillator on, each of those with unison voices, each with a warp, the sub, the noise, each filter,
        each effect on, each modulation routing in use and each named macro. */
    int partsInUse (BuiltInEffects::BasicSynthProcessor& synth, const WaveSynth::Patch& patch)
    {
        const Settings p (synth);
        int parts = 0;

        for (int o = 1; o <= WaveSynth::numOscillators; ++o)
        {
            const auto id = "osc" + juce::String (o);

            if (! p.on (id + "On"))
                continue;

            ++parts;
            parts += p.number (id + "Unison") > 1.5 ? 1 : 0;

            if (const auto warp = p.choice (id + "Warp"); warp.isNotEmpty() && warp != "Off")
                ++parts;
        }

        parts += p.number ("subLevel") > 0.5 ? 1 : 0;
        parts += p.number ("noiseLevel") > 0.5 ? 1 : 0;

        for (int f = 1; f <= 2; ++f)
            if (const auto type = p.choice ("filter" + juce::String (f) + "Type"); type.isNotEmpty() && type != "Off")
                ++parts;

        parts += p.on ("fxDriveOn") && p.number ("fxDriveAmount") > 0.5 ? 1 : 0;
        parts += p.on ("fxChorusOn") && p.number ("fxChorusMix") > 0.5 ? 1 : 0;
        parts += p.on ("fxPhaserOn") && p.number ("fxPhaserMix") > 0.5 ? 1 : 0;
        parts += p.on ("fxDelayOn") && p.number ("fxDelayMix") > 0.5 ? 1 : 0;
        parts += p.on ("fxReverbOn") && p.number ("fxReverbMix") > 0.5 ? 1 : 0;

        for (int m = 1; m <= WaveSynth::numSlots; ++m)
        {
            const auto id = "mod" + juce::String (m);
            parts += p.number (id + "Source") > 0.5 && p.number (id + "Destination") > 0.5
                     && std::abs (p.number (id + "Amount")) > 0.5 ? 1 : 0;
        }

        for (auto& name : patch.macroNames)
            parts += name.isNotEmpty() ? 1 : 0;

        return parts;
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
        juce::StringArray done, files;

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
            entry->setProperty ("parts", partsInUse (synth, *found));
            save (folder, entry, play (synth, *part, length));
            done.add (name);
            files.add (slug (name));
        }

        // Then the patches that use the most of the synth, 750 with those above: each category's share is
        // its share of the list (ten at least), and within a category the patches with the most parts in
        // use come first (partsInUse), the earlier in the list among equals. Each plays one of the
        // category's audition phrases.
        const int wanted = 750;
        std::vector<std::vector<const WaveSynth::Patch*>> lists;
        std::vector<int> share;
        int all = 0, given = 0;

        for (auto& category : WaveSynth::patchCategories())
        {
            if (category == "Init")
                continue;

            lists.emplace_back();

            for (auto& patch : patches)
                if (patch.category == category)
                    lists.back().push_back (&patch);

            all += (int) lists.back().size();
        }

        for (auto& list : lists)
        {
            share.push_back (juce::jmin ((int) list.size(), juce::jmax (10, juce::roundToInt ((double) wanted * (double) list.size() / all))));
            given += share.back();
        }

        // Rounding leaves a few over or under: taken from, or given to, the lists in turn, the longest first.
        std::vector<size_t> bySize (lists.size());
        std::iota (bySize.begin(), bySize.end(), (size_t) 0);
        std::stable_sort (bySize.begin(), bySize.end(), [&lists] (size_t x, size_t y) { return lists[x].size() > lists[y].size(); });

        for (int turn = 0; given != wanted && turn < 10000; ++turn)
        {
            const auto c = bySize[(size_t) turn % bySize.size()];
            const auto step = given > wanted ? -1 : 1;

            if (share[c] + step >= juce::jmin (10, (int) lists[c].size()) && share[c] + step <= (int) lists[c].size())
            {
                share[c] += step;
                given += step;
            }
        }

        for (size_t c = 0; c < lists.size(); ++c)
        {
            const auto& inCategory = lists[c];
            std::vector<std::pair<int, int>> scored;   // parts in use, place in the list

            for (int at = 0; at < (int) inCategory.size(); ++at)
            {
                BuiltInEffects::BasicSynthProcessor synth;

                if (synth.loadPatch (inCategory[(size_t) at]->name))
                    scored.emplace_back (partsInUse (synth, *inCategory[(size_t) at]), at);
            }

            std::stable_sort (scored.begin(), scored.end(), [] (const auto& x, const auto& y) { return x.first > y.first; });
            int number = 0, shown = 0;

            for (auto& patch : inCategory)
                shown += done.contains (patch->name) ? 1 : 0;

            for (int rank = 0; rank < (int) scored.size() && shown < share[c]; ++rank)
            {
                const auto at = scored[(size_t) rank].second;
                const auto& patch = *inCategory[(size_t) at];

                if (done.contains (patch.name))
                    continue;

                BuiltInEffects::BasicSynthProcessor synth;

                if (! synth.loadPatch (patch.name))
                {
                    std::cout << "FAILED no patch " << patch.name << std::endl;
                    continue;
                }

                auto file = slug (patch.name);

                while (files.contains (file))
                    file << "-2";

                files.add (file);
                const auto kind = InstrumentKeys::kindFor (synth);
                const auto& patterns = InstrumentKeys::patternsFor (kind);
                const auto& pattern = patterns[(size_t) number++ % patterns.size()];
                const auto longTail = kind == InstrumentKeys::Kind::chord || kind == InstrumentKeys::Kind::held || kind == InstrumentKeys::Kind::hit;
                auto* entry = index.add ("synth", patch.category, patch.name, "synth/" + file + ".mp3");
                entry->setProperty ("about", describe (synth, patch));
                entry->setProperty ("phrase", pattern.name);
                entry->setProperty ("place", at + 1);
                entry->setProperty ("of", (int) inCategory.size());
                entry->setProperty ("parts", scored[(size_t) rank].first);
                save (folder, entry, audition (synth, pattern, longTail ? 3.0 : 1.5));
                done.add (patch.name);
                ++shown;
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

    /** The Sequence Library's factory presets unpacked into a folder of this run's own, taken away after. */
    struct UnpackedPresets
    {
        UnpackedPresets()
        {
            SequenceLibrary::setFactoryFolder (presets);

            if (const auto error = SequenceLibrary::unpackFactoryPresets(); error.isNotEmpty())
                std::cout << "FAILED " << error << std::endl;
        }

        ~UnpackedPresets()
        {
            SequenceLibrary::setFactoryFolder (before);

            // The unpacked files are read-only: made writable again to be deleted.
            for (const auto& f : juce::RangedDirectoryIterator (base, true, "*", juce::File::findFilesAndDirectories))
                f.getFile().setReadOnly (false);

            base.deleteRecursively();
        }

        const juce::File before = SequenceLibrary::factoryFolder(), base = juce::File::createTempFile ("sounddemos-presets"), presets = base.getChildFile ("set");
    };

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

    /** Basic Drums' kits, each on the Sequence Library's factory grooves written for it (Drums > Full
        Grooves: the three with the most energy, the first by name among equals), as the library's
        preview plays them. A kit no factory groove uses plays two of the audition beats instead. */
    void writeDrums (const juce::File& folder, Index& index)
    {
        const UnpackedPresets unpacked;
        juce::AudioPluginFormatManager formats;
        formats.addFormat (BuiltInEffects::createFormat());
        const auto entries = SequenceLibrary (unpacked.presets).scan();
        const auto& patterns = InstrumentKeys::patternsFor (InstrumentKeys::Kind::kit);
        int number = 0;

        for (auto& kit : DrumMachine::kits())
        {
            std::vector<std::pair<const SequenceLibrary::Entry*, SequenceFile::Sequence>> grooves;

            for (auto& e : entries)
                if (! e.isKit && e.path.size() > 1 && e.path[0] == "Drums" && e.path[1] == "Full Grooves")
                {
                    juce::String error;

                    if (auto q = SequenceFile::load (e.file, error); q.has_value() && q->voice != nullptr
                                                                      && q->voice->getStringAttribute ("name") == "Basic Drums: " + kit.name)
                        grooves.emplace_back (&e, std::move (*q));
                }

            std::stable_sort (grooves.begin(), grooves.end(), [] (const auto& x, const auto& y)
            {
                return x.first->energy != y.first->energy ? x.first->energy > y.first->energy : x.first->name.compareNatural (y.first->name) < 0;
            });

            std::cout << kit.name << ": " << grooves.size() << " factory grooves" << std::endl;
            const auto shown = juce::jmin ((int) grooves.size(), 3);

            for (int g = 0; g < shown; ++g)
            {
                const auto& e = *grooves[(size_t) g].first;
                auto* entry = index.add ("drums", kit.name, e.name, "drums/" + slug (kit.name + " " + e.name) + ".mp3");
                entry->setProperty ("about", kit.description);
                entry->setProperty ("groove", e.description);
                entry->setProperty ("style", e.style);
                entry->setProperty ("bars", e.bars);
                entry->setProperty ("energy", e.energy);
                entry->setProperty ("of", (int) grooves.size());
                writeLoop (folder, entry, { grooves[(size_t) g].second }, formats);
            }

            if (shown == 0)
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

    /** The Sequence Library's factory presets: every kit, and sequences from every style (one of each role
        from the styles the library leads with; a bass line and a melody, arp or harmony part from the
        others; of each the one that does the most by measure, on a voice the style has not played yet). */
    void writeLibrary (const juce::File& folder, Index& index, bool kits, bool sequences)
    {
        const UnpackedPresets unpacked;
        const auto& presets = unpacked.presets;

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
            // What can be measured of how much a sequence does: its notes to a bar, its different pitches
            // (a drum part's different sounds), and its rhythm (the different gaps between one note's start
            // and the next, and the different note lengths).
            struct Found
            {
                const SequenceLibrary::Entry* entry = nullptr;
                SequenceFile::Sequence sequence;
                juce::String voice;
                double notesPerBar = 0.0;
                int pitches = 0, gaps = 0, lengths = 0;
                bool still = false;   // two notes a bar or fewer, or one pitch (one drum sound) only
                bool oneNoteSub = false;   // a Bass > Sub part on one pitch: the last thing to show
                double score = 0.0;
            };

            const auto measure = [] (const SequenceLibrary::Entry& e, SequenceFile::Sequence q)
            {
                Found f;
                f.entry = &e;
                f.voice = q.voice != nullptr ? q.voice->getStringAttribute ("name") : juce::String();
                std::set<int> pitches;
                std::set<juce::int64> gaps, lengths, starts;

                for (auto& n : q.notes)
                {
                    pitches.insert (n.pitch);
                    lengths.insert (n.length);
                    starts.insert (n.start);
                }

                for (auto at = starts.begin(); at != starts.end() && std::next (at) != starts.end(); ++at)
                    gaps.insert (*std::next (at) - *at);

                f.notesPerBar = q.bars() > 0.0 ? (double) q.notes.size() / q.bars() : 0.0;
                f.pitches = (int) pitches.size();
                f.gaps = (int) gaps.size();
                f.lengths = (int) lengths.size();
                f.still = f.notesPerBar <= 2.0 || f.pitches <= 1;
                f.oneNoteSub = f.pitches <= 1 && e.path.size() > 1 && e.path[0] == "Bass" && e.path[1] == "Sub";
                f.score = juce::jmin (16.0, f.notesPerBar) + 2.0 * juce::jmin (8, f.pitches) + 2.0 * juce::jmin (6, f.gaps) + juce::jmin (4, f.lengths);
                f.sequence = std::move (q);
                return f;
            };

            const std::vector<juce::StringArray> leadingSlots { { "Drums" }, { "Bass" }, { "Harmony" }, { "Pads" }, { "Arps" }, { "Melody" }, { "SFX" } },
                                                 otherSlots { { "Bass" }, { "Melody", "Arps", "Harmony" } };

            for (auto& style : SequenceLibrary::stylesIn (entries))
            {
                juce::StringArray voices;   // the voices this style's clips already play on
                juce::StringArray taken;    // the names of the sequences already chosen for it

                for (auto& slot : leading.contains (style) ? leadingSlots : otherSlots)
                {
                    const auto candidates = [&] (bool anyRole)
                    {
                        std::vector<Found> found;

                        for (auto& e : entries)
                            if (! e.isKit && e.style == style && e.path.size() > 0 && (anyRole || slot.contains (e.path[0])) && ! taken.contains (e.name))
                            {
                                juce::String error;

                                if (auto q = SequenceFile::load (e.file, error))
                                    found.push_back (measure (e, std::move (*q)));
                                else
                                    std::cout << "FAILED " << e.name << ": " << error << std::endl;
                            }

                        return found;
                    };

                    auto found = candidates (false);

                    // Where every one of the role stands still, the style's other roles are looked at too.
                    if (std::all_of (found.begin(), found.end(), [] (const Found& f) { return f.still; }))
                        found = candidates (true);

                    if (found.empty())
                        continue;

                    // One that moves before one that stands still, a voice the style has not played before one
                    // it has, then the highest score, the most energy, and the first by name.
                    const auto best = std::min_element (found.begin(), found.end(), [&voices] (const Found& x, const Found& y)
                    {
                        const auto xUsed = voices.contains (x.voice), yUsed = voices.contains (y.voice);

                        if (x.oneNoteSub != y.oneNoteSub)   return ! x.oneNoteSub;
                        if (x.still != y.still)   return ! x.still;
                        if (xUsed != yUsed)       return ! xUsed;
                        if (! juce::approximatelyEqual (x.score, y.score))   return x.score > y.score;
                        if (x.entry->energy != y.entry->energy)              return x.entry->energy > y.entry->energy;
                        return x.entry->name.compareNatural (y.entry->name) < 0;
                    });

                    voices.add (best->voice);
                    taken.add (best->entry->name);
                    auto* entry = index.add ("sequences", style, best->entry->name, fileFor ("sequences", *best->entry));
                    note (entry, *best->entry);
                    entry->setProperty ("voice", best->voice);
                    entry->setProperty ("notesPerBar", juce::roundToInt (best->notesPerBar * 10.0) / 10.0);
                    entry->setProperty ("pitches", best->pitches);
                    entry->setProperty ("of", (int) found.size());
                    entry->setProperty ("still", best->still);
                    entry->setProperty ("voiceAgain", voices.indexOf (best->voice) != voices.size() - 1);
                    writeLoop (folder, entry, { best->sequence }, formats);
                }
            }
        }
    }
    //==============================================================================
    // Before and after: damage of a known kind and amount put on a clip Torqwave rendered, then repaired
    // by the same code the Effects menu runs. The clean clip is kept, so each repair is measured against it.

    juce::AudioBuffer<float> bufferOf (const AudioSnapshot& s)
    {
        juce::AudioBuffer<float> out (s.getNumChannels(), (int) s.getNumFrames());
        s.read (out, 0, 0, out.getNumSamples());
        return out;
    }

    AudioEdits::Snapshot snapshotOf (const juce::AudioBuffer<float>& audio)   { return AudioEdits::fromBuffer (juce::AudioBuffer<float> (audio)); }

    double rmsDb (const juce::AudioBuffer<float>& a, int from, int frames)
    {
        auto sum = 0.0;

        for (int ch = 0; ch < a.getNumChannels(); ++ch)
            for (int i = from; i < from + frames; ++i)
                sum += (double) a.getSample (ch, i) * a.getSample (ch, i);

        return 10.0 * std::log10 (juce::jmax (1.0e-20, sum / juce::jmax (1, frames * a.getNumChannels())));
    }

    /** How far a clip is from the clean one, in dB against the clean one's level, once the clean one is
        scaled to fit it best (so a repair that changes the level is not counted as an error for that). */
    double differenceDb (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& clean, int from = 0)
    {
        auto ac = 0.0, cc = 0.0;
        const auto frames = juce::jmin (a.getNumSamples(), clean.getNumSamples());

        for (int ch = 0; ch < clean.getNumChannels(); ++ch)
            for (int i = from; i < frames; ++i)
            {
                ac += (double) a.getSample (ch, i) * clean.getSample (ch, i);
                cc += (double) clean.getSample (ch, i) * clean.getSample (ch, i);
            }

        const auto g = cc > 0.0 ? ac / cc : 1.0;
        auto error = 0.0;

        for (int ch = 0; ch < clean.getNumChannels(); ++ch)
            for (int i = from; i < frames; ++i)
            {
                const auto d = a.getSample (ch, i) - g * clean.getSample (ch, i);
                error += d * d;
            }

        return 10.0 * std::log10 (juce::jmax (1.0e-20, error) / juce::jmax (1.0e-20, g * g * cc));
    }

    /** The level between two frequencies, in dB: the mean power of Hann-windowed 4,096-point frames. */
    double bandDb (const juce::AudioBuffer<float>& a, double lowHz, double highHz)
    {
        constexpr int order = 12, size = 1 << order;
        juce::dsp::FFT fft (order);
        juce::dsp::WindowingFunction<float> window ((size_t) size, juce::dsp::WindowingFunction<float>::hann);
        std::vector<float> frame ((size_t) size * 2);
        auto sum = 0.0;
        int frames = 0;

        for (int ch = 0; ch < a.getNumChannels(); ++ch)
            for (int at = 0; at + size <= a.getNumSamples(); at += size / 2, ++frames)
            {
                std::fill (frame.begin(), frame.end(), 0.0f);
                std::copy (a.getReadPointer (ch, at), a.getReadPointer (ch, at) + size, frame.begin());
                window.multiplyWithWindowingTable (frame.data(), (size_t) size);
                fft.performFrequencyOnlyForwardTransform (frame.data());

                for (int bin = (int) (lowHz * size / rate); bin <= juce::jmin (size / 2, (int) (highHz * size / rate)); ++bin)
                    sum += (double) frame[(size_t) bin] * frame[(size_t) bin] / ((double) size * size);
            }

        return 10.0 * std::log10 (juce::jmax (1.0e-20, sum / juce::jmax (1, frames)));
    }

    /** The level of one frequency over the whole clip (the left channel), in dBFS: a sine of that level would read the same. */
    double toneDb (const juce::AudioBuffer<float>& a, double hz)
    {
        auto re = 0.0, im = 0.0;

        for (int i = 0; i < a.getNumSamples(); ++i)
        {
            const auto phase = juce::MathConstants<double>::twoPi * hz * i / rate;
            re += a.getSample (0, i) * std::cos (phase);
            im += a.getSample (0, i) * std::sin (phase);
        }

        return 20.0 * std::log10 (juce::jmax (1.0e-10, 2.0 * std::sqrt (re * re + im * im) / juce::jmax (1, a.getNumSamples())));
    }

    double tenth (double v)   { return std::round (v * 10.0) / 10.0; }

    struct Quality
    {
        juce::File folder;
        juce::Array<juce::var> demos;

        juce::DynamicObject* demo (const juce::String& id, const juce::String& title)
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("id", id);
            o->setProperty ("title", title);
            demos.add (juce::var (o));
            return o;
        }

        /** A clip of a demo: its MP3 (variable bit rate, quality 2) and its spectrogram, the left channel as the editor draws it. */
        void clip (juce::DynamicObject* demo, const juce::String& which, const juce::AudioBuffer<float>& audio)
        {
            const auto name = demo->getProperty ("id").toString() + "-" + which;
            juce::DynamicObject::Ptr entry = new juce::DynamicObject();
            entry->setProperty ("file", "audio/" + name + ".mp3");
            save (folder, entry.get(), audio, 2);

            const auto image = Spectrogram::render (*snapshotOf (audio), 0, rate, 0.0, audio.getNumSamples() / 900.0, 900, 220);
            const auto png = folder.getChildFile ("img").getChildFile (name + ".png");
            png.getParentDirectory().createDirectory();
            png.deleteFile();

            if (juce::FileOutputStream out (png); out.openedOk())
                juce::PNGImageFormat().writeImageToStream (image, out);

            demo->setProperty (which, entry->getProperty ("file"));
            demo->setProperty (which + "Picture", "img/" + name + ".png");
            demo->setProperty (which + "Bytes", (juce::int64) entry->getProperty ("bytes") + png.getSize());
        }
    };
}

void writeQualityDemos (const juce::File& folder, const juce::StringArray& also)
{
    folder.createDirectory();
    Quality q { folder, {} };
    juce::Random random (20261124);

    // The clips: a pad's chords under a bell's arpeggio (four bars at 120 BPM and two seconds to ring), and a factory kit's loop twice.
    juce::AudioBuffer<float> tonal, beatLoop;
    juce::String kitName;

    {
        BuiltInEffects::BasicSynthProcessor pad, bell;
        pad.loadPatch (juce::String ("Concrete Pad"));
        bell.loadPatch (juce::String ("Cold Bell"));
        tonal = play (pad, minorChords, 4.0 * bar + 2.0);
        const auto arp = play (bell, arpeggio, 4.0 * bar + 2.0);

        for (int ch = 0; ch < 2; ++ch)
            tonal.addFrom (ch, 0, arp, ch, 0, arp.getNumSamples());

        tonal.applyGain (0.6f);
    }

    {
        const UnpackedPresets unpacked;
        juce::AudioPluginFormatManager formats;
        formats.addFormat (BuiltInEffects::createFormat());

        for (auto& e : SequenceLibrary (unpacked.presets).scan())
            if (e.isKit && e.style == "Industrial/EBM" && (kitName.isEmpty() || e.name.compareNatural (kitName) < 0))
            {
                juce::String error;

                if (const auto kit = SequenceKit::load (e.file, error); kit.has_value() && ! kit->parts.empty())
                {
                    juce::AudioBuffer<float> pass;

                    if (SequencePreview::render (kit->parts, kit->parts.front().writtenTempo, kit->parts.front().beatsPerBar, rate, formats, pass).isEmpty())
                    {
                        kitName = e.name;
                        beatLoop.setSize (2, pass.getNumSamples() * 2);

                        for (int r = 0; r < 2; ++r)
                            for (int ch = 0; ch < 2; ++ch)
                                beatLoop.copyFrom (ch, r * pass.getNumSamples(), pass, ch, 0, pass.getNumSamples());
                    }
                }
            }
    }

    if (beatLoop.getNumSamples() == 0)
    {
        std::cout << "FAILED no kit to play" << std::endl;
        return;
    }

    const auto tonalSource = juce::String ("Torqwave Synth: Concrete Pad's chords under Cold Bell's arpeggio, four bars at 120 BPM");
    const auto beatSource = "The factory kit " + kitName + " (Industrial/EBM), its loop twice, as the Sequence Library plays it";

    // Clicks: 60 of them, one to three samples long, at up to nine tenths of full scale.
    if (also.contains ("clicks"))
    {
        auto* d = q.demo ("clicks", "Clicks");
        auto damaged = tonal;
        const int clicks = 60;
        std::vector<int> places;

        for (int i = 0; i < clicks; ++i)
        {
            const auto at = 4800 + random.nextInt (damaged.getNumSamples() - 9600);
            places.push_back (at);
            const auto length = 1 + random.nextInt (3);
            const auto value = (random.nextBool() ? 1.0f : -1.0f) * (0.4f + 0.5f * random.nextFloat());

            for (int ch = 0; ch < 2; ++ch)
                for (int k = 0; k < length; ++k)
                    damaged.setSample (ch, at + k, value);
        }

        const auto repaired = ClickRepair::repair (*snapshotOf (damaged), 0, damaged.getNumSamples(), rate, ClickRepair::Settings());
        const auto after = repaired.has_value() && repaired->snapshot != nullptr ? bufferOf (*repaired->snapshot) : damaged;
        d->setProperty ("source", tonalSource);
        d->setProperty ("damage", juce::String (clicks) + " clicks put in, each one to three samples long at 40 to 90% of full scale, in both channels");
        d->setProperty ("repair", "Effects > Noise Reduction and Repair > Click and Pop Repair at its defaults (threshold 18 dB, clicks up to 3 ms)");
        d->setProperty ("clicksAdded", clicks);
        d->setProperty ("clicksRepaired", repaired.has_value() ? repaired->repaired : 0);
        d->setProperty ("skipped", repaired.has_value() ? repaired->skipped : 0);

        // What is left where each click was: how far the repaired clip stands from the clean one there.
        int left = 0, gone = 0;   // still more than a tenth of full scale away; within a hundredth

        for (auto at : places)
        {
            auto worst = 0.0f;

            for (int ch = 0; ch < 2; ++ch)
                for (int k = 0; k < 3; ++k)
                    worst = juce::jmax (worst, std::abs (after.getSample (ch, at + k) - tonal.getSample (ch, at + k)));

            left += worst > 0.1f ? 1 : 0;
            gone += worst < 0.01f ? 1 : 0;
        }

        d->setProperty ("clicksLeft", left);
        d->setProperty ("clicksGone", gone);
        d->setProperty ("clipRms", tenth (rmsDb (tonal, 0, tonal.getNumSamples())));
        d->setProperty ("differenceBefore", tenth (differenceDb (damaged, tonal)));
        d->setProperty ("differenceAfter", tenth (differenceDb (after, tonal)));
        q.clip (d, "before", damaged);
        q.clip (d, "after", after);
    }

    // Hiss: white noise 38 dB under full scale, with a second of it alone first for the noise print.
    {
        auto* d = q.demo ("hiss", "Hiss");
        const auto lead = (int) rate;
        juce::AudioBuffer<float> clean (2, lead + tonal.getNumSamples());
        clean.clear();

        for (int ch = 0; ch < 2; ++ch)
            clean.copyFrom (ch, lead, tonal, ch, 0, tonal.getNumSamples());

        auto damaged = clean;
        const auto amplitude = juce::Decibels::decibelsToGain (-38.0f) * std::sqrt (3.0f);   // a uniform noise of that RMS

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < damaged.getNumSamples(); ++i)
                damaged.addSample (ch, i, amplitude * (2.0f * random.nextFloat() - 1.0f));

        const auto snapshot = snapshotOf (damaged);
        const auto print = NoiseReduction::capture (*snapshot, 0, lead - 2400, rate);
        d->setProperty ("source", tonalSource + ", after a second of the noise alone");
        d->setProperty ("damage", "white noise at -38 dBFS RMS over the whole clip");
        d->setProperty ("repair", "Effects > Noise Reduction and Repair > Noise Reduction: the print taken from the first second, then the whole clip reduced by 12 dB (the default) and by 24 dB");
        d->setProperty ("noiseBefore", tenth (rmsDb (damaged, 4800, lead - 9600)));
        d->setProperty ("differenceBefore", tenth (differenceDb (damaged, clean, lead)));
        q.clip (d, "before", damaged);

        if (print.has_value())
            for (auto reduction : { 12.0f, 24.0f })
            {
                NoiseReduction::Settings settings;
                settings.reductionDb = reduction;
                const auto reduced = NoiseReduction::reduce (*snapshot, 0, damaged.getNumSamples(), rate, *print, settings);
                const auto after = reduced != nullptr ? bufferOf (*reduced) : damaged;
                const auto which = juce::String (reduction > 12.0f ? "after24" : "after");
                d->setProperty (reduction > 12.0f ? "noiseAfter24" : "noiseAfter", tenth (rmsDb (after, 4800, lead - 9600)));
                d->setProperty (reduction > 12.0f ? "differenceAfter24" : "differenceAfter", tenth (differenceDb (after, clean, lead)));
                q.clip (d, which, after);
            }
    }

    // Hum: 50 Hz and five harmonics, the fundamental 34 dB under full scale and each harmonic 6 dB under the one before.
    {
        auto* d = q.demo ("hum", "Mains hum");
        auto damaged = tonal;

        for (int h = 1; h <= 6; ++h)
        {
            const auto amplitude = juce::Decibels::decibelsToGain (-34.0f - 6.0f * (float) (h - 1));

            for (int i = 0; i < damaged.getNumSamples(); ++i)
            {
                const auto v = amplitude * (float) std::sin (juce::MathConstants<double>::twoPi * 50.0 * h * i / rate + 0.7 * h);
                damaged.addSample (0, i, v);
                damaged.addSample (1, i, v);
            }
        }

        const auto removed = Restoration::removeHum (*snapshotOf (damaged), 0, damaged.getNumSamples(), rate, Restoration::Hum());
        const auto after = removed != nullptr ? bufferOf (*removed) : damaged;
        d->setProperty ("source", tonalSource);
        d->setProperty ("damage", "a 50 Hz hum with five harmonics: the fundamental at -34 dBFS, each harmonic 6 dB under the one before");
        d->setProperty ("repair", "Effects > Noise Reduction and Repair > Remove Hum at its defaults (50 Hz, 8 harmonics, Q 30)");

        for (auto hz : { 50, 100, 150 })
        {
            d->setProperty ("clean" + juce::String (hz), tenth (toneDb (tonal, hz)));
            d->setProperty ("before" + juce::String (hz), tenth (toneDb (damaged, hz)));
            d->setProperty ("after" + juce::String (hz), tenth (toneDb (after, hz)));
        }
        q.clip (d, "before", damaged);
        q.clip (d, "after", after);
    }

    // Clipping: the kit pushed 4 dB over full scale and cut off flat there (then everything 6 dB down, to leave room for the rebuilt peaks).
    if (also.contains ("clipping"))
    {
        auto* d = q.demo ("clipping", "Clipping");
        auto clean = beatLoop;
        clean.applyGain (1.0f / juce::jmax (1.0e-6f, clean.getMagnitude (0, clean.getNumSamples())) * juce::Decibels::decibelsToGain (4.0f));
        auto damaged = clean;
        int clippedSamples = 0;

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < damaged.getNumSamples(); ++i)
                if (std::abs (damaged.getSample (ch, i)) > 1.0f)
                {
                    damaged.setSample (ch, i, damaged.getSample (ch, i) > 0.0f ? 1.0f : -1.0f);
                    ++clippedSamples;
                }

        clean.applyGain (0.5f);
        damaged.applyGain (0.5f);
        const auto rebuilt = Restoration::declip (*snapshotOf (damaged), 0, damaged.getNumSamples(), rate, Restoration::Declip());
        const auto after = rebuilt.audio != nullptr ? bufferOf (*rebuilt.audio) : damaged;
        d->setProperty ("source", beatSource);
        d->setProperty ("damage", "the loop turned up until its peak stood 4 dB over full scale, and cut off flat there");
        d->setProperty ("repair", "Effects > Noise Reduction and Repair > Repair Clipping at its defaults (threshold 99% of the peak, the peak kept)");
        d->setProperty ("clippedSamples", clippedSamples);
        d->setProperty ("runsRebuilt", rebuilt.repaired);
        d->setProperty ("runsTooLong", rebuilt.tooLong);
        d->setProperty ("differenceBefore", tenth (differenceDb (damaged, clean)));
        d->setProperty ("differenceAfter", tenth (differenceDb (after, clean)));
        q.clip (d, "before", damaged);
        q.clip (d, "after", after);
    }

    // Missing highs: everything over 5 kHz filtered away (eight low-pass sections in a row), then rebuilt.
    {
        auto* d = q.demo ("highs", "Missing highs");
        auto damaged = beatLoop;

        for (int ch = 0; ch < 2; ++ch)
            for (int stage = 0; stage < 8; ++stage)
            {
                juce::IIRFilter filter;
                filter.setCoefficients (juce::IIRCoefficients::makeLowPass (rate, 5000.0));
                filter.processSamples (damaged.getWritePointer (ch), damaged.getNumSamples());
            }

        const auto snapshot = snapshotOf (damaged);
        const auto cutoff = SpectralRecovery::findCutoff (*snapshot, 0, damaged.getNumSamples(), rate);
        const auto recovered = SpectralRecovery::recover (*snapshot, 0, damaged.getNumSamples(), rate, SpectralRecovery::Settings());
        const auto after = recovered != nullptr ? bufferOf (*recovered) : damaged;
        d->setProperty ("source", beatSource);
        d->setProperty ("damage", "everything over 5 kHz filtered away (eight 12 dB low-pass sections in a row at 5 kHz)");
        d->setProperty ("repair", "Effects > Noise Reduction and Repair > Spectral Recovery at its defaults (the cutoff found from the audio, level -3 dB, rebuilt up to 16 kHz)");
        d->setProperty ("recovered", recovered != nullptr);
        d->setProperty ("cutoffFound", juce::roundToInt (cutoff));
        d->setProperty ("highsOriginal", tenth (bandDb (beatLoop, 6000.0, 16000.0)));
        d->setProperty ("highsBefore", tenth (bandDb (damaged, 6000.0, 16000.0)));
        d->setProperty ("highsAfter", tenth (bandDb (after, 6000.0, 16000.0)));
        q.clip (d, "original", beatLoop);
        q.clip (d, "before", damaged);
        q.clip (d, "after", after);
    }

    // Loudness: the loop as rendered, then through the Maximizer with its gain set so it measures -10 LUFS.
    {
        auto* d = q.demo ("loudness", "Loudness");
        const auto snapshot = snapshotOf (beatLoop);
        const auto before = Loudness::measure (beatLoop, rate);
        const auto target = -10.0;
        Maximizer::Settings settings;
        settings.ceilingDb = -1.0;
        settings.gainDb = juce::jlimit (0.0, 24.0, target - before.integrated);
        auto after = beatLoop;
        Loudness::Result measured = before;

        for (int round = 0; round < 6; ++round)
        {
            if (const auto made = Maximizer::apply (*snapshot, 0, beatLoop.getNumSamples(), rate, settings))
                after = bufferOf (*made);

            measured = Loudness::measure (after, rate);

            if (std::abs (measured.integrated - target) < 0.05)
                break;

            settings.gainDb = juce::jlimit (0.0, 24.0, settings.gainDb + target - measured.integrated);
        }

        d->setProperty ("source", beatSource);
        d->setProperty ("damage", "nothing: the loop as the instruments made it");
        d->setProperty ("repair", "Effects > Dynamics > Maximizer: Balanced, ceiling -1 dBTP, its gain set so the clip measures " + juce::String (target, 0) + " LUFS");
        d->setProperty ("gainDb", tenth (settings.gainDb));
        d->setProperty ("loudnessBefore", tenth (before.integrated));
        d->setProperty ("loudnessAfter", tenth (measured.integrated));
        d->setProperty ("truePeakBefore", std::round (before.truePeak * 100.0) / 100.0);
        d->setProperty ("truePeakAfter", std::round (measured.truePeak * 100.0) / 100.0);
        d->setProperty ("rangeBefore", tenth (before.range));
        d->setProperty ("rangeAfter", tenth (measured.range));
        q.clip (d, "before", beatLoop);
        q.clip (d, "after", after);
    }

    folder.getChildFile ("facts.json").replaceWithText (juce::JSON::toString (juce::var (q.demos)) + "\n");
    std::cout << juce::JSON::toString (juce::var (q.demos)) << std::endl;
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

    std::cout << index.entries.size() << " clips, " << juce::String ((double) bytes / 1048576.0, 1) << " MB" << std::endl;
}
