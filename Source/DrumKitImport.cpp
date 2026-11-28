#include "DrumKitImport.h"

namespace DrumKitImport
{
namespace
{
    using DrumMachine::Sound;

    juce::String counted (int n, const juce::String& one, const juce::String& many)
    {
        return juce::String (n) + " " + (n == 1 ? one : many);
    }

    /** A Battery cell's colour: its sixteen, in the order of its colour menu, as the cell itself is
        drawn. A kit keeps a colour as a number whose low four bits choose among them (0 to 15 and
        16 to 31 are the same sixteen); any other number is no colour. */
    juce::Colour batteryColour (int index)
    {
        static const juce::uint32 colours[16] { 0xfff2403d, 0xffd95f36, 0xffd6801e, 0xffa69129, 0xffabab33, 0xff78a633, 0xff56b856, 0xff2bad82,
                                                0xff2aa8a8, 0xff26acd1, 0xff3b7fed, 0xff816eff, 0xffb163ff, 0xffca54e8, 0xffcc56c2, 0xffe83a6e };

        return juce::isPositiveAndBelow (index, 32) ? juce::Colour (colours[index & 15]) : juce::Colour();
    }

    float decibels (float gain)
    {
        return gain > 0.0f ? juce::jlimit (-48.0f, 6.0f, 20.0f * std::log10 (gain)) : -48.0f;
    }

    float semitones (float ratio)
    {
        return ratio > 0.0f ? juce::jlimit (-24.0f, 24.0f, 12.0f * std::log2 (ratio)) : 0.0f;
    }

    /** A value all of a cell's layers share, or none. */
    template <typename Get>
    std::optional<float> shared (const std::vector<const BatteryKit::Layer*>& layers, Get get)
    {
        if (layers.empty())
            return std::nullopt;

        const auto first = get (*layers.front());

        for (auto* l : layers)
            if (std::abs (get (*l) - first) > 1.0e-4f)
                return std::nullopt;

        return first;
    }

    /** The cells of a kit that cut each other off, as sets: a cell's index to its set's number, from
        1 in the order the sets' first cells stand. In a set are the cells of a voice group of one
        voice, and the cells of every group that shares an exclusive group. A cell alone in its set
        has none (a hit always cuts its own last one). */
    std::map<int, int> chokeSets (const BatteryKit::Kit& kit)
    {
        std::map<juce::String, std::vector<int>> members;

        for (auto& c : kit.cells)
        {
            if (c.layers.empty() || ! juce::isPositiveAndBelow (c.voiceGroup, (int) kit.voiceGroups.size()))
                continue;

            const auto& g = kit.voiceGroups[(size_t) c.voiceGroup];

            if (g.exclusiveGroup >= 0)
                members["x" + juce::String (g.exclusiveGroup)].push_back (c.index);
            else if (g.voices == 1)
                members["g" + juce::String (c.voiceGroup)].push_back (c.index);
        }

        std::vector<std::vector<int>> sets;

        for (auto& [key, cells] : members)
            if (cells.size() > 1)
                sets.push_back (cells);

        for (auto& cells : sets)
            std::sort (cells.begin(), cells.end());

        std::sort (sets.begin(), sets.end());
        std::map<int, int> setOf;

        for (size_t i = 0; i < sets.size(); ++i)
            for (auto cell : sets[i])
                setOf[cell] = (int) i + 1;

        return setOf;
    }
}

//==============================================================================
Plan fromBattery (const BatteryKit::Kit& kit, const juce::File& kitFile, const juce::File& library)
{
    Plan plan;
    plan.name = kitFile.getFileNameWithoutExtension();
    const auto folder = kitFile.getParentDirectory();

    auto cells = kit.cells;
    std::stable_sort (cells.begin(), cells.end(), [] (const BatteryKit::Cell& a, const BatteryKit::Cell& b) { return a.index < b.index; });

    int samples = 0, faded = 0, stacked = 0, thinned = 0, wide = 0, ownValues = 0, ownStarts = 0, ends = 0, loops = 0, otherTypes = 0, unreadable = 0, emptied = 0, enveloped = 0, choked = 0;
    const auto setOf = chokeSets (kit);
    std::set<int> chokeSetsKept, chokeSetsDropped;

    for (auto& c : cells)
    {
        if (c.layers.empty())
            continue;

        struct Range
        {
            const BatteryKit::Layer* layer;
            int low, high;
        };

        std::vector<Range> ranges;

        for (auto& l : c.layers)
            ranges.push_back ({ &l, l.lowVelocity, l.highVelocity });

        // Softest first; of two on one range, the one that fades in from below is the harder.
        std::stable_sort (ranges.begin(), ranges.end(), [] (const Range& a, const Range& b)
        {
            return std::make_tuple (a.low, a.high, a.layer->fadeLow > 0) < std::make_tuple (b.low, b.high, b.layer->fadeLow > 0);
        });

        bool cellFaded = false, cellStacked = false;

        for (size_t i = 1; i < ranges.size(); ++i)
        {
            auto& a = ranges[i - 1];
            auto& b = ranges[i];

            if (b.low > a.high)
                continue;

            const auto fades = a.layer->fadeHigh > 0 || b.layer->fadeLow > 0;

            if (! fades && a.low == b.low && a.high == b.high)
            {
                cellStacked = true;
            }
            else if (fades || b.low == a.high)
            {
                // A crossfade becomes a split in its middle; ranges that share one velocity give it to the softer.
                const auto middle = (b.low + juce::jmin (a.high, b.high)) / 2;
                a.high = juce::jmax (a.low, middle);
                b.low = juce::jmin (b.high, a.high + 1);
                cellFaded = cellFaded || fades;
            }
            else
            {
                cellStacked = true;
            }
        }

        Pad pad;
        std::vector<const BatteryKit::Layer*> kept;
        int note = 127;
        bool cellWide = false;

        for (auto& r : ranges)
        {
            const auto& l = *r.layer;
            note = juce::jmin (note, l.lowKey);
            cellWide = cellWide || l.highKey > l.lowKey;

            if (! juce::isPositiveAndBelow (l.path, (int) kit.paths.size()) || kit.paths[(size_t) l.path].parts.isEmpty())
            {
                ++unreadable;
                continue;
            }

            const auto& path = kit.paths[(size_t) l.path];
            const juce::File named (juce::File::getSeparatorString() + path.parts[path.parts.size() - 1]);

            if (! named.hasFileExtension (soundFileTypes))
            {
                ++(named.hasFileExtension ("rx2;rex;rcy") ? loops : otherTypes);
                continue;
            }

            auto file = BatteryKit::resolve (path, folder);

            if (file == juce::File())
            {
                ++unreadable;
                continue;
            }

            if (! file.existsAsFile() && library != juce::File())
                if (const auto moved = BatteryKit::resolveUnder (path, library); moved != juce::File())
                    file = moved;

            if (! file.existsAsFile())
                plan.missing.addIfNotAlreadyThere (file.getFullPathName());

            pad.layers.push_back ({ file, r.low, r.high });
            kept.push_back (&l);
        }

        if (pad.layers.empty())
        {
            ++emptied;
            continue;
        }

        // More layers than a pad has: the ones spread most evenly through them, each reaching down to the one before.
        if ((int) pad.layers.size() > DrumMachine::maxLayers)
        {
            std::vector<PadLayer> spread;
            std::vector<const BatteryKit::Layer*> spreadKept;
            const auto n = pad.layers.size();

            for (int i = 0; i < DrumMachine::maxLayers; ++i)
            {
                const auto at = (size_t) std::lround ((double) i * (double) (n - 1) / (double) (DrumMachine::maxLayers - 1));
                spread.push_back (pad.layers[at]);
                spreadKept.push_back (kept[at]);
            }

            if (! cellStacked)
                for (size_t i = 1; i < spread.size(); ++i)
                    if (spread[i].lowVelocity > spread[i - 1].highVelocity + 1)
                        spread[i].lowVelocity = spread[i - 1].highVelocity + 1;

            pad.layers = std::move (spread);
            kept = std::move (spreadKept);
            ++thinned;
        }

        const auto gain = shared (kept, [] (const BatteryKit::Layer& l) { return l.gain; });
        const auto tune = shared (kept, [] (const BatteryKit::Layer& l) { return l.tune; });
        const auto pan = shared (kept, [] (const BatteryKit::Layer& l) { return l.pan; });
        const auto start = shared (kept, [] (const BatteryKit::Layer& l) { return l.frames > 0 ? 100.0f * (float) l.start / (float) l.frames : 0.0f; });

        pad.name = c.name.isNotEmpty() ? c.name : pad.layers.front().file.getFileNameWithoutExtension();
        pad.note = note;
        pad.level = decibels (c.gain * gain.value_or (1.0f));
        pad.pan = juce::jlimit (-100.0f, 100.0f, 100.0f * (c.pan + pan.value_or (0.0f)));
        pad.tune = semitones (c.tune * tune.value_or (1.0f));
        pad.start = juce::jlimit (0.0f, 95.0f, start.value_or (kept.front()->frames > 0 ? 100.0f * (float) kept.front()->start / (float) kept.front()->frames : 0.0f));
        pad.reverse = c.reverse;
        pad.colour = batteryColour (c.colour);
        pad.envelope = c.envelopeRead && c.envelopeOn;
        pad.attack = c.attack;
        pad.hold = c.hold;
        pad.decay = c.decay;
        enveloped += pad.envelope ? 1 : 0;

        if (const auto set = setOf.find (c.index); set != setOf.end())
        {
            if (set->second <= DrumMachine::numChokeGroups)
            {
                pad.choke = set->second;
                chokeSetsKept.insert (set->second);
                ++choked;
            }
            else
            {
                chokeSetsDropped.insert (set->second);
            }
        }

        // A cell over several keys follows them in pitch: on its lowest it is that far below its root.
        if (cellWide && c.keyTracking)
            pad.tune = juce::jlimit (-24.0f, 24.0f, pad.tune + (float) (note - kept.front()->rootKey));

        samples += (int) pad.layers.size();
        faded += cellFaded ? 1 : 0;
        stacked += cellStacked ? 1 : 0;
        wide += cellWide ? 1 : 0;
        ownValues += ! gain.has_value() || ! tune.has_value() || ! pan.has_value() ? 1 : 0;
        ownStarts += ! start.has_value() ? 1 : 0;
        ends += (int) std::count_if (kept.begin(), kept.end(), [] (const BatteryKit::Layer* l) { return l->endBack > 0; });
        plan.pads.push_back (std::move (pad));
    }

    auto& report = plan.report;
    report.add (counted ((int) plan.pads.size(), "cell has", "cells have") + " samples, " + juce::String (samples) + " in all.");

    if (faded > 0)
        report.add (counted (faded, "cell fades", "cells fade") + " between layers by velocity. Here a layer has a range of its own, so each fade is a split at its middle.");

    if (stacked > 0)
        report.add (counted (stacked, "cell has", "cells have") + " several samples on one range of velocities. Here those take turns, one a hit.");

    if (thinned > 0)
        report.add (counted (thinned, "cell has", "cells have") + " more than " + juce::String (DrumMachine::maxLayers) + " layers, the most a pad has: "
                    + juce::String (DrumMachine::maxLayers) + " spread evenly through them were kept.");

    if (wide > 0)
        report.add (counted (wide, "cell plays", "cells play") + " over several keys. Each is on its lowest note here, at the pitch it has there.");

    if (ownValues > 0)
        report.add (counted (ownValues, "cell has", "cells have") + " layers with a level, tune or pan of their own, which a layer here has not: the cell's own were kept.");

    if (ownStarts > 0)
        report.add (counted (ownStarts, "cell has", "cells have") + " layers that start at different places in their samples: the pad starts where the softest does.");

    if (ends > 0)
        report.add (counted (ends, "sample ends", "samples end") + " before its file does in the kit. Here it plays to its end, or as long as the pad's Decay.");

    if (enveloped > 0)
        report.add (counted (enveloped, "cell has", "cells have") + " a volume envelope on. Its attack, hold and decay are on the pad's ENV page; a sustain and a release are not brought over.");

    if (choked > 0)
        report.add (counted (choked, "cell cuts", "cells cut") + " others off in the kit (hi-hats, usually), in " + counted ((int) chokeSetsKept.size(), "set", "sets")
                    + ". Each set is a choke group here, from group 1.");

    if (! chokeSetsDropped.empty())
        report.add ("The kit has " + counted ((int) chokeSetsDropped.size(), "more set", "more sets") + " of cells that cut each other off than the "
                    + juce::String (DrumMachine::numChokeGroups) + " choke groups here: those cells ring.");

    if (kit.voiceGroupsRead && kit.kitVoices == 1)
        report.add ("The kit as a whole is set to one voice in Battery, where each hit cuts the last. That is not brought over: here only the pads in a choke group cut each other.");

    if (! kit.voiceGroupsRead)
        report.add ("The kit's voice groups are laid out in a way this does not read, so which cells cut each other off is not known.");

    if (loops > 0)
        report.add (counted (loops, "REX loop was", "REX loops were") + " left out: a pad plays a sound file.");

    if (otherTypes > 0)
        report.add (counted (otherTypes, "sample is", "samples are") + " of a type this does not play, and were left out.");

    if (unreadable > 0)
        report.add (counted (unreadable, "sample is", "samples are") + " named in a way this does not read, and were left out.");

    if (emptied > 0)
        report.add (counted (emptied, "cell was", "cells were") + " left with no sample, and has no pad.");

    report.add ("Battery's effects, pitch envelopes, modulation, time-stretching, articulations and buses are not brought over.");
    return plan;
}

//==============================================================================
namespace
{
    /** A file's name as words: lower case, split where a capital follows a small letter and where
        a letter follows a digit, everything else that is not a letter, a digit or a sharp a space.
        With digitsApart, a digit after a letter starts a word too ("bd2" is "bd" and "2"); without,
        it stays ("v2", "c3"). */
    juce::StringArray wordsOf (const juce::String& name, bool digitsApart)
    {
        juce::String spaced;
        juce::juce_wchar last = 0;

        for (auto c : name)
        {
            const auto letter = juce::CharacterFunctions::isLetter (c), digit = juce::CharacterFunctions::isDigit (c);
            const auto lastLetter = juce::CharacterFunctions::isLetter (last), lastDigit = juce::CharacterFunctions::isDigit (last);

            if ((letter && juce::CharacterFunctions::isUpperCase (c) && lastLetter && juce::CharacterFunctions::isLowerCase (last)) || (letter && lastDigit) || (digitsApart && digit && lastLetter))
                spaced << ' ';

            spaced << (letter || digit || c == '#' ? juce::CharacterFunctions::toLowerCase (c) : (juce::juce_wchar) ' ');
            last = c;
        }

        juce::StringArray words;
        words.addTokens (spaced, " ", {});
        words.removeEmptyStrings();
        return words;
    }

    bool has (const juce::StringArray& words, std::initializer_list<const char*> any)
    {
        for (auto* w : any)
            if (words.contains (w))
                return true;

        return false;
    }

    /** The instrument a name speaks of, or none. */
    std::optional<Sound> soundNamed (const juce::StringArray& w)
    {
        const auto hat = has (w, { "hat", "hats", "hihat", "hihats", "hh" });
        const auto tom = has (w, { "tom", "toms" });
        const auto conga = has (w, { "conga", "congas" });

        if (has (w, { "ohh", "openhat", "openhh" }) || (hat && has (w, { "open", "opn", "op" })))     return Sound::openHat;
        if (has (w, { "phh", "pedalhat" }) || (hat && has (w, { "pedal", "foot" })))                  return Sound::pedalHat;
        if (hat || has (w, { "chh", "closedhat", "closedhh" }))                                       return Sound::closedHat;
        if (has (w, { "kick", "kicks", "kik", "kck", "bd", "bassdrum" }) || (w.contains ("bass") && w.contains ("drum")))   return Sound::kick;
        if (has (w, { "rim", "rimshot", "sidestick" }) || (w.contains ("side") && w.contains ("stick")))                    return Sound::rim;
        if (has (w, { "snare", "snares", "snr", "sd", "sn" }))                                        return Sound::snare;
        if (has (w, { "clap", "claps", "handclap", "cp" }))                                           return Sound::clap;
        if ((tom && has (w, { "low", "lo", "floor" })) || has (w, { "lowtom", "floortom", "lt" }))    return Sound::lowTom;
        if ((tom && has (w, { "high", "hi" })) || has (w, { "hightom", "hitom", "ht" }))              return Sound::highTom;
        if (tom || has (w, { "midtom", "mt" }))                                                       return Sound::midTom;
        if (has (w, { "crash", "cymbal", "cymbals" }))                                                return Sound::crash;
        if (w.contains ("ride"))                                                                      return w.contains ("bell") ? Sound::rideBell : Sound::ride;
        if (w.contains ("china"))                                                                     return Sound::china;
        if (w.contains ("splash"))                                                                    return Sound::splash;
        if (has (w, { "cowbell", "cb" }) || (w.contains ("cow") && w.contains ("bell")))              return Sound::cowbell;
        if (has (w, { "shaker", "shakers", "maraca", "maracas", "cabasa" }))                          return Sound::shaker;
        if (has (w, { "tambourine", "tamb", "tambo" }))                                               return Sound::tambourine;
        if (has (w, { "vibraslap", "vibra" }))                                                        return Sound::vibraslap;
        if (has (w, { "bongo", "bongos" }))                                                           return Sound::bongo;
        if (conga)                                                                                    return has (w, { "mute", "muted", "slap" }) ? Sound::muteConga : Sound::openConga;
        if (has (w, { "timbale", "timbales" }))                                                       return Sound::timbale;
        if (w.contains ("agogo"))                                                                     return Sound::agogo;
        if (w.contains ("whistle"))                                                                   return Sound::whistle;
        if (w.contains ("guiro"))                                                                     return Sound::guiro;
        if (has (w, { "clave", "claves" }))                                                           return Sound::claves;
        if (w.contains ("woodblock") || (w.contains ("wood") && w.contains ("block")))                return Sound::woodBlock;
        if (w.contains ("cuica"))                                                                     return Sound::cuica;
        if (w.contains ("triangle"))                                                                  return Sound::triangle;
        return std::nullopt;
    }

    /** A whole number a word is, or -1. */
    int numberOf (const juce::String& word)
    {
        return word.isNotEmpty() && word.length() <= 3 && word.containsOnly ("0123456789") ? word.getIntValue() : -1;
    }

    /** The note a word names ("c2", "f#3", "bb1"; middle C is c4), or -1. */
    int noteNamed (const juce::String& word)
    {
        static const int steps[] { 9, 11, 0, 2, 4, 5, 7 };   // a to g

        if (word.length() < 2 || word.length() > 3 || word[0] < 'a' || word[0] > 'g' || ! juce::CharacterFunctions::isDigit (word.getLastCharacter()))
            return -1;

        auto step = steps[word[0] - 'a'];

        if (word.length() == 3)
        {
            if (word[1] == '#')        ++step;
            else if (word[1] == 'b')   --step;
            else                       return -1;
        }

        const auto note = (word.getLastCharacter() - '0' + 1) * 12 + step;
        return juce::isPositiveAndBelow (note, 128) ? note : -1;
    }

    /** A mark that makes a file one of a pad's layers: "v3" or "vel3" (by velocity), "rr2" (taking turns). */
    bool layerMark (const juce::String& word, bool& byVelocity, int& number)
    {
        for (auto* prefix : { "vel", "v", "rr" })
        {
            if (word.startsWith (prefix) && numberOf (word.substring ((int) std::strlen (prefix))) >= 0)
            {
                byVelocity = prefix[0] == 'v';
                number = numberOf (word.substring ((int) std::strlen (prefix)));
                return true;
            }
        }

        return false;
    }
}

Plan fromFolder (const juce::File& folder)
{
    Plan plan;
    plan.name = folder.getFileName();

    juce::Array<juce::File> files;
    int looked = 0;

    for (const auto& entry : juce::RangedDirectoryIterator (folder, true, "*", juce::File::findFiles | juce::File::ignoreHiddenFiles))
    {
        if (++looked > mostFolderFiles * 4 || files.size() >= mostFolderFiles)
            break;

        if (entry.getFile().hasFileExtension (soundFileTypes))
            files.add (entry.getFile());
    }

    const auto tooMany = files.size() >= mostFolderFiles;
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b) { return a.getFullPathName().compareNatural (b.getFullPathName()) < 0; });

    // In a folder whose files are numbered from below 35, a number is a count and not a note.
    std::map<juce::String, bool> numbered;

    for (auto& f : files)
    {
        const auto words = wordsOf (f.getFileNameWithoutExtension(), true);

        for (auto n : { words.isEmpty() ? -1 : numberOf (words[0]), words.isEmpty() ? -1 : numberOf (words[words.size() - 1]) })
            if (n >= 0 && n < DrumMachine::firstNote)
                numbered[f.getParentDirectory().getFullPathName()] = true;
    }

    // Files alike but for a layer's mark are one pad.
    struct Group
    {
        juce::String name;
        juce::StringArray words;
        juce::File folder;
        struct Member { juce::File file; int velocity, turn; };
        std::vector<Member> members;
    };

    std::vector<Group> groups;
    std::map<juce::String, size_t> groupOf;

    for (auto& f : files)
    {
        auto words = wordsOf (f.getFileNameWithoutExtension(), false);
        int velocity = -1, turn = -1;

        for (int i = words.size(); --i >= 0;)
        {
            bool byVelocity = false;
            int number = 0;

            if (words.size() > 1 && layerMark (words[i], byVelocity, number))
            {
                (byVelocity ? velocity : turn) = number;
                words.remove (i);
            }
        }

        const auto marked = velocity >= 0 || turn >= 0;
        const auto key = f.getParentDirectory().getFullPathName() + "\n" + (marked ? words.joinIntoString (" ") : f.getFileName());
        const auto [at, isNew] = groupOf.try_emplace (key, groups.size());

        if (isNew)
        {
            // Its name is the file's, without the mark where the mark stands as a word of its own.
            juce::StringArray named;
            named.addTokens (f.getFileNameWithoutExtension(), " _-.", {});
            named.removeEmptyStrings();
            const auto before = named.size();

            for (int i = named.size(); --i >= 0;)
            {
                bool byVelocity = false;
                int number = 0;

                if (named.size() > 1 && layerMark (named[i].toLowerCase(), byVelocity, number))
                    named.remove (i);
            }

            groups.push_back ({ ! marked ? f.getFileNameWithoutExtension() : named.size() < before ? named.joinIntoString (" ") : words.joinIntoString (" "),
                                words, f.getParentDirectory(), {} });
        }

        groups[at->second].members.push_back ({ f, velocity, turn });
    }

    // The folder's own files before its folders'; a plain name before one with a number in it or more
    // words, so that of "Kick", "Kick Sub" and "BD 2" it is "Kick" that gets the kick's pad.
    const auto depthOf = [&folder] (const juce::File& f) { return f.getRelativePathFrom (folder).retainCharacters ("/\\").length(); };

    std::stable_sort (groups.begin(), groups.end(), [&depthOf] (const Group& a, const Group& b)
    {
        const auto key = [&depthOf] (const Group& g)
        {
            return std::make_tuple (depthOf (g.folder), g.folder.getFullPathName(), g.name.containsAnyOf ("0123456789"), g.words.size());
        };

        return key (a) < key (b);
    });

    std::set<int> notesTaken;
    std::vector<size_t> unplaced;
    int byNumber = 0, byInstrument = 0, byNoteName = 0, layered = 0, thinned = 0;

    for (auto& g : groups)
    {
        Pad pad;
        pad.name = g.name;

        // Its layers: by velocity where the files say, each velocity's files taking turns.
        std::stable_sort (g.members.begin(), g.members.end(), [] (const Group::Member& a, const Group::Member& b) { return std::tie (a.velocity, a.turn) < std::tie (b.velocity, b.turn); });
        std::vector<int> velocities;

        for (auto& m : g.members)
            if (m.velocity >= 0 && (velocities.empty() || velocities.back() != m.velocity))
                velocities.push_back (m.velocity);

        for (auto& m : g.members)
        {
            PadLayer l { m.file, 1, 127 };

            if (m.velocity >= 0 && velocities.size() > 1)
            {
                const auto step = (int) (std::find (velocities.begin(), velocities.end(), m.velocity) - velocities.begin()), steps = (int) velocities.size();
                l.lowVelocity = 1 + 127 * step / steps;
                l.highVelocity = 127 * (step + 1) / steps;
            }

            pad.layers.push_back (l);
        }

        if ((int) pad.layers.size() > DrumMachine::maxLayers)
        {
            pad.layers.resize ((size_t) DrumMachine::maxLayers);
            ++thinned;
        }

        layered += pad.layers.size() > 1 ? 1 : 0;

        // Its note. Marks and note names are read with a digit kept on its letter, the rest with it apart.
        const auto& whole = g.words;
        const auto w = wordsOf (whole.joinIntoString (" "), true);
        const auto isCount = numbered[g.folder.getFullPathName()];
        const auto first = w.isEmpty() ? -1 : numberOf (w[0]), last = w.isEmpty() ? -1 : numberOf (w[w.size() - 1]);
        const auto inRange = [] (int n) { return n >= DrumMachine::firstNote && n <= DrumMachine::lastNote; };

        if (! isCount && (inRange (first) || inRange (last)))
        {
            pad.note = inRange (first) ? first : last;
            ++byNumber;
        }
        else if (const auto sound = soundNamed (w); sound.has_value() && notesTaken.count (DrumMachine::padNote (*sound)) == 0)
        {
            pad.note = DrumMachine::padNote (*sound);
            ++byInstrument;
        }
        else if (const auto named = whole.isEmpty() ? -1 : juce::jmax (noteNamed (whole[0]), noteNamed (whole[whole.size() - 1])); named >= 0 && ! sound.has_value())
        {
            pad.note = named;
            ++byNoteName;
        }
        else
        {
            pad.note = -1;
            unplaced.push_back (plan.pads.size());
        }

        if (pad.note >= 0)
            notesTaken.insert (pad.note);

        plan.pads.push_back (std::move (pad));
    }

    // The rest get notes of their own: from 82 up, then whatever is free below.
    int next = DrumMachine::firstUserNote;

    for (auto i : unplaced)
    {
        while (next < 128 && notesTaken.count (next) > 0)
            ++next;

        if (next >= 128)
            for (next = 0; next < 128 && notesTaken.count (next) > 0;)
                ++next;

        plan.pads[i].note = juce::jmin (127, next);
        notesTaken.insert (plan.pads[i].note);
    }

    auto& report = plan.report;
    report.add (counted ((int) plan.pads.size(), "pad", "pads") + " from " + counted (files.size(), "sound file", "sound files") + ": " + juce::String (byNumber)
                + " by a drum note's number in the name, " + juce::String (byInstrument) + " by an instrument's name, " + juce::String (byNoteName) + " by a note's name, and "
                + juce::String ((int) unplaced.size()) + " on notes of their own, from " + juce::String (DrumMachine::firstUserNote) + " up.");

    if (layered > 0)
        report.add (counted (layered, "pad has", "pads have") + " several layers, from files alike but for a mark: v1, v2 split the velocities, rr1, rr2 take turns.");

    if (thinned > 0)
        report.add (counted (thinned, "pad was", "pads were") + " given more than " + juce::String (DrumMachine::maxLayers) + " files, the most layers a pad has: the first "
                    + juce::String (DrumMachine::maxLayers) + " were kept.");

    if (tooMany)
        report.add ("The folder has more sound files than an import looks at: the first " + juce::String (mostFolderFiles) + " by name were used.");

    report.add ("Check the pads by ear: names are a guess at what a file is. A pad's note, name and layers can be changed from its menu and its panel.");
    return plan;
}

void place (Plan& plan)
{
    std::array<bool, (size_t) DrumMachine::numPads> taken {};
    std::vector<size_t> waiting;
    int nextUser = DrumMachine::numSounds;

    for (size_t i = 0; i < plan.pads.size(); ++i)
    {
        auto& p = plan.pads[i];
        p.pad = -1;

        // Its note's own sound, when the note is that sound's pad and nothing has it yet.
        if (const auto sound = DrumMachine::soundFor (p.note); sound.has_value() && DrumMachine::padNote (*sound) == p.note && ! taken[(size_t) *sound])
            p.pad = (int) *sound;
        else if (nextUser < DrumMachine::numPads)
            p.pad = nextUser++;
        else
            waiting.push_back (i);

        if (p.pad >= 0)
            taken[(size_t) p.pad] = true;
    }

    int moved = 0, dropped = 0;

    for (auto i : waiting)
    {
        const auto free = std::find (taken.begin(), taken.begin() + DrumMachine::numSounds, false);

        if (free == taken.begin() + DrumMachine::numSounds)
        {
            ++dropped;
            continue;
        }

        *free = true;
        plan.pads[i].pad = (int) (free - taken.begin());
        ++moved;
    }

    if (moved > 0)
        plan.report.add (counted (moved, "cell is", "cells are") + " on a pad whose note is fixed, and so on another note than in the kit: the "
                         + juce::String (DrumMachine::numUserPads) + " pads that can take any note were full.");

    if (dropped > 0)
    {
        plan.report.add ("The kit has " + juce::String ((int) plan.pads.size()) + " cells with samples. The first " + juce::String (DrumMachine::numPads) + " are on pads; "
                         + counted (dropped, "was", "were") + " left out.");
        plan.pads.erase (std::remove_if (plan.pads.begin(), plan.pads.end(), [] (const Pad& p) { return p.pad < 0; }), plan.pads.end());
    }
}

juce::String apply (BuiltInEffects::BasicDrumsProcessor& drums, Plan& plan)
{
    const auto placed = std::count_if (plan.pads.begin(), plan.pads.end(), [] (const Pad& p) { return juce::isPositiveAndBelow (p.pad, DrumMachine::numPads); });

    if (placed == 0)
        return plan.name + " has nothing in it to put on a pad.";

    // Read every sample first, so that a kit none of whose samples can be read changes nothing.
    std::array<std::vector<DrumMachine::Layer>, (size_t) DrumMachine::numPads> layers;
    std::set<const Sampler::Sample*> held;
    juce::int64 bytes = 0;
    int read = 0, notThere = 0, notRead = 0, noRoom = 0;
    juce::String firstReason;

    for (auto& p : plan.pads)
    {
        if (! juce::isPositiveAndBelow (p.pad, DrumMachine::numPads))
            continue;

        for (auto& l : p.layers)
        {
            if ((int) layers[(size_t) p.pad].size() >= DrumMachine::maxLayers)
                break;

            if (! l.file.existsAsFile())
            {
                ++notThere;
                continue;
            }

            juce::String why;
            const auto sample = Sampler::sharedSample (l.file, why);

            if (sample == nullptr)
            {
                ++notRead;

                if (firstReason.isEmpty())
                    firstReason = why.isNotEmpty() ? why : l.file.getFileName() + " could not be read.";

                continue;
            }

            if (held.count (sample.get()) == 0 && bytes + sample->bytes() > Sampler::mostBytes)
            {
                ++noRoom;
                continue;
            }

            if (held.insert (sample.get()).second)
                bytes += sample->bytes();

            DrumMachine::Layer layer;
            layer.sample = sample;
            layer.file = l.file;
            layer.lowVelocity = l.lowVelocity;
            layer.highVelocity = l.highVelocity;
            layers[(size_t) p.pad].push_back (std::move (layer));
            ++read;
        }
    }

    if (read == 0)
    {
        const auto synthCount = std::count_if (plan.pads.begin(), plan.pads.end(), [] (const Pad& p)
        {
            return juce::isPositiveAndBelow (p.pad, DrumMachine::numPads) && p.synthesised;
        });

        if (synthCount == 0)
            return notThere > 0 ? "None of " + plan.name + "'s samples are where the kit says, so nothing was changed."
                                : "None of " + plan.name + "'s samples could be read, so nothing was changed. " + firstReason;
    }

    auto settings = DrumMachine::defaultKit();
    settings.drive = juce::jlimit (0.0f, 100.0f, plan.kitDrive);
    settings.glue = juce::jlimit (0.0f, 100.0f, plan.kitGlue);
    settings.reverbSize = juce::jlimit (0.0f, 100.0f, plan.reverbSize);
    settings.delayTime = juce::jlimit (0, DrumMachine::delayTimeNames().size() - 1, plan.delayTime);
    settings.feedback = juce::jlimit (0.0f, 100.0f, plan.feedback);

    for (auto& p : plan.pads)
    {
        const bool hasSamples = juce::isPositiveAndBelow (p.pad, DrumMachine::numPads) && ! layers[(size_t) p.pad].empty();
        const bool hasSynth = juce::isPositiveAndBelow (p.pad, DrumMachine::numPads) && p.synthesised && ! hasSamples;

        if (! hasSamples && ! hasSynth)
            continue;

        auto s = hasSamples ? DrumMachine::defaultUserSettings()
                            : (p.pad < DrumMachine::numSounds ? settings.sounds[(size_t) p.pad] : DrumMachine::defaultUserSettings());
        s.tune = juce::jlimit (-24.0f, 24.0f, p.tune);
        s.level = juce::jlimit (-48.0f, 6.0f, p.level);
        s.pan = juce::jlimit (-100.0f, 100.0f, p.pan);
        s.choke = juce::jlimit (0, DrumMachine::numChokeGroups, p.choke);

        if (hasSamples)
        {
            s.type = DrumMachine::sampleType;
            s.source = 0;
            s.start = juce::jlimit (0.0f, 95.0f, p.start);
            s.reverse = p.reverse;
        }
        else
        {
            s.type = p.pad < DrumMachine::numSounds ? p.synthType : DrumMachine::classicType;
            s.source = juce::jlimit (0, DrumMachine::numSources, p.source);
            s.decay = juce::jlimit (5.0f, 5000.0f, p.synthDecay);
            s.tone = juce::jlimit (-100.0f, 100.0f, p.tone);
            s.drive = juce::jlimit (0.0f, 100.0f, p.drive);
        }

        s.filter = juce::jlimit (0, 3, p.filter);
        s.cutoff = juce::jlimit (20.0f, 20000.0f, p.cutoff);
        s.resonance = juce::jlimit (0.0f, 100.0f, p.resonance);
        s.crush = juce::jlimit (0.0f, 100.0f, p.crush);
        s.reverb = juce::jlimit (0.0f, 100.0f, p.reverb);
        s.delay = juce::jlimit (0.0f, 100.0f, p.delay);
        s.effects = p.effects;

        if (p.envelope)
        {
            s.effects[DrumMachine::envOn] = 1.0f;
            s.effects[DrumMachine::envAttack] = p.attack;
            s.effects[DrumMachine::envHold] = p.hold;
            s.effects[DrumMachine::envDecay] = p.decay;
        }

        s.effects.limit();

        if (p.pad < DrumMachine::numSounds)
            settings.sounds[(size_t) p.pad] = s;
        else
            settings.user[(size_t) (p.pad - DrumMachine::numSounds)].settings = s;
    }

    drums.applyKit ({ plan.name, {}, settings });

    for (auto& p : plan.pads)
    {
        const bool hasSamples = juce::isPositiveAndBelow (p.pad, DrumMachine::numPads) && ! layers[(size_t) p.pad].empty();
        const bool hasSynth = juce::isPositiveAndBelow (p.pad, DrumMachine::numPads) && p.synthesised && ! hasSamples;

        if (! hasSamples && ! hasSynth)
            continue;

        drums.setPadName (p.pad, p.name);
        drums.setPadColour (p.pad, p.colour);
        drums.setPadNote (p.pad, p.note);     // a user pad's: applying a kit puts every pad back on its own

        if (hasSamples)
            drums.setLayers (p.pad, std::move (layers[(size_t) p.pad]));
    }

    if (notThere > 0)
        plan.report.add (counted (notThere, "sample is", "samples are") + " not where the kit says, and " + (notThere == 1 ? "its layer was" : "their layers were") + " left out.");

    if (notRead > 0)
        plan.report.add (counted (notRead, "sample", "samples") + " could not be read. " + firstReason);

    if (noRoom > 0)
        plan.report.add (counted (noRoom, "sample was", "samples were") + " left out for room: one kit holds " + juce::File::descriptionOfSizeInBytes (Sampler::mostBytes)
                         + " of samples, in memory.");

    return {};
}

//==============================================================================
namespace
{
    /** Maschine's 0..1 level fader (0.75 = 0 dB, 1.0 = +6..10 dB, 0 = -inf) mapped cleanly to dB. */
    float maschineLevelDb (float norm)
    {
        if (norm <= 1.0e-4f)
            return -48.0f;

        if (norm <= 0.75f)
            return juce::jlimit (-48.0f, 0.0f, 20.0f * std::log10 (norm / 0.75f));

        return juce::jlimit (0.0f, 6.0f, (norm - 0.75f) * 24.0f);
    }

    float logFreq (float norm, float minHz, float maxHz)
    {
        const auto t = juce::jlimit (0.0f, 1.0f, norm);
        return minHz * std::pow (maxHz / minHz, t);
    }

    /** Clean-room mapping from a Maschine internal Drum Synth module onto Basic Drums' own
        built-in synthesized drum sources (Sound + 24 dedicated industrial/electronic sources). */
    void mapMaschineDrumSynth (const MaschineKit::Module& m, const juce::String& padName, Pad& pad, int& preferredNote)
    {
        pad.synthesised = true;
        const int engine = m.intParams.empty() ? 0 : m.intParams[0];
        const auto fp = [&] (size_t idx, float fallback)
        {
            return idx < m.floatParams.size() ? m.floatParams[idx] : fallback;
        };

        const float tuneNorm = fp (0, 0.5f);
        const float decayNorm = fp (1, 0.45f);
        const float toneNorm = fp (2, 0.5f);
        const float driveNorm = fp (3, 0.25f);

        pad.tune = juce::jlimit (-24.0f, 24.0f, (tuneNorm - 0.5f) * 24.0f);
        pad.tone = juce::jlimit (-100.0f, 100.0f, (toneNorm - 0.5f) * 160.0f);
        pad.drive = juce::jlimit (0.0f, 100.0f, driveNorm * 65.0f);

        const auto lower = padName.toLowerCase();

        if (m.kind == MaschineKit::ModuleKind::kick)
        {
            preferredNote = DrumMachine::padNote (Sound::kick);
            pad.synthDecay = logFreq (decayNorm, 80.0f, 1800.0f);
            if (engine == 0)      pad.source = DrumMachine::sourceWithId ("subKick");
            else if (engine == 1) pad.source = DrumMachine::sourceWithId ("fmKick");
            else if (engine == 2) pad.source = DrumMachine::sourceWithId ("distortedKick");
            else if (engine == 3) { pad.source = 0; pad.synthType = DrumMachine::modernType; }
            else if (engine == 4) pad.source = DrumMachine::sourceWithId ("clickKick");
            else                  pad.source = DrumMachine::sourceWithId ("rumbleKick");
        }
        else if (m.kind == MaschineKit::ModuleKind::snare)
        {
            preferredNote = lower.contains ("clap") ? DrumMachine::padNote (Sound::clap) : DrumMachine::padNote (Sound::snare);
            pad.synthDecay = logFreq (decayNorm, 60.0f, 900.0f);
            if (lower.contains ("clap") || engine == 4) pad.source = DrumMachine::sourceWithId ("clapStack");
            else if (engine == 0)                       pad.source = DrumMachine::sourceWithId ("fmSnare");
            else if (engine == 1)                       pad.source = DrumMachine::sourceWithId ("noiseSnare");
            else if (engine == 2)                       { pad.source = 0; pad.synthType = DrumMachine::modernType; }
            else if (engine == 3)                       pad.source = DrumMachine::sourceWithId ("metalSnare");
            else                                        pad.source = DrumMachine::sourceWithId ("snap");
        }
        else if (m.kind == MaschineKit::ModuleKind::hihat)
        {
            const bool isOpen = lower.contains ("open") || lower.contains ("oh") || decayNorm > 0.55f;
            preferredNote = DrumMachine::padNote (isOpen ? Sound::openHat : Sound::closedHat);
            pad.synthDecay = isOpen ? logFreq (decayNorm, 180.0f, 1600.0f) : logFreq (decayNorm, 25.0f, 220.0f);
            if (engine == 1)      pad.source = DrumMachine::sourceWithId ("fmHat");
            else if (engine == 2) pad.source = DrumMachine::sourceWithId ("noiseHat");
            else                  { pad.source = 0; pad.synthType = DrumMachine::modernType; }
        }
        else if (m.kind == MaschineKit::ModuleKind::tom)
        {
            preferredNote = DrumMachine::padNote (tuneNorm < 0.35f ? Sound::lowTom : tuneNorm > 0.65f ? Sound::highTom : Sound::midTom);
            pad.synthDecay = logFreq (decayNorm, 120.0f, 1200.0f);
            pad.source = DrumMachine::sourceWithId ("fmTom");
        }
        else if (m.kind == MaschineKit::ModuleKind::cymbal)
        {
            const bool isRide = lower.contains ("ride");
            preferredNote = DrumMachine::padNote (isRide ? Sound::ride : Sound::crash);
            pad.synthDecay = logFreq (decayNorm, 400.0f, 3500.0f);
            pad.source = DrumMachine::sourceWithId (isRide ? "darkRide" : "trashCrash");
        }
        else if (m.kind == MaschineKit::ModuleKind::bassSynth)
        {
            preferredNote = -1;
            pad.synthDecay = logFreq (decayNorm, 250.0f, 2400.0f);
            pad.source = DrumMachine::sourceWithId ("subDrop");
        }
        else
        {
            preferredNote = -1;
            pad.synthDecay = logFreq (decayNorm, 60.0f, 1400.0f);
            if (engine == 0)      pad.source = DrumMachine::sourceWithId ("fmBell");
            else if (engine == 1) pad.source = DrumMachine::sourceWithId ("anvil");
            else if (engine == 2) pad.source = DrumMachine::sourceWithId ("noiseBurst");
            else                  pad.source = DrumMachine::sourceWithId ("pipe");
        }
    }

    /** Clean-room mapping from a Maschine insert FX module onto Basic Drums' built-in per-pad
        DSP effects (EQ, Filter, Saturator, Transient Shaper, Compressor, Bitcrusher, Reverb/Delay).
        Returns true if mapped into Basic Drums' DSP, false if unsupported and noted in report. */
    bool mapMaschinePadFx (const MaschineKit::Module& m, Pad& pad, Plan& plan)
    {
        if (! m.active)
            return true;

        const auto fp = [&] (size_t idx, float fallback)
        {
            return idx < m.floatParams.size() ? m.floatParams[idx] : fallback;
        };
        const auto ip = [&] (size_t idx, int fallback)
        {
            return idx < m.intParams.size() ? m.intParams[idx] : fallback;
        };

        if (m.kind == MaschineKit::ModuleKind::eq)
        {
            auto& fx = pad.effects;
            fx[DrumMachine::eqOn] = 1.0f;
            fx[DrumMachine::eqLowFreq] = logFreq (fp (0, 0.31f), 30.0f, 800.0f);
            fx[DrumMachine::eqLowGain] = juce::jlimit (-18.0f, 18.0f, fp (1, 0.0f) * 18.0f);
            fx[DrumMachine::eqBell1Freq] = logFreq (fp (2, 0.46f), 80.0f, 6000.0f);
            fx[DrumMachine::eqBell1Gain] = juce::jlimit (-18.0f, 18.0f, fp (3, 0.0f) * 18.0f);
            fx[DrumMachine::eqBell1Width] = juce::jlimit (0.25f, 3.0f, 0.4f + fp (8, 0.5f) * 2.0f);
            fx[DrumMachine::eqBell2Freq] = logFreq (fp (4, 0.65f), 250.0f, 14000.0f);
            fx[DrumMachine::eqBell2Gain] = juce::jlimit (-18.0f, 18.0f, fp (5, 0.0f) * 18.0f);
            fx[DrumMachine::eqBell2Width] = juce::jlimit (0.25f, 3.0f, 0.4f + fp (9, 0.5f) * 2.0f);
            fx[DrumMachine::eqHighFreq] = logFreq (fp (6, 0.66f), 1000.0f, 16000.0f);
            fx[DrumMachine::eqHighGain] = juce::jlimit (-18.0f, 18.0f, fp (7, 0.0f) * 18.0f);
            fx.limit();
            return true;
        }
        if (m.kind == MaschineKit::ModuleKind::filter)
        {
            const int mode = ip (0, 0);
            pad.filter = mode == 1 ? 2 : mode == 2 ? 3 : 1; // 1 LP, 2 BP, 3 HP
            pad.cutoff = logFreq (fp (0, 0.5f), 40.0f, 18000.0f);
            pad.resonance = juce::jlimit (0.0f, 90.0f, fp (2, fp (1, 0.2f)) * 85.0f);
            return true;
        }
        if (m.kind == MaschineKit::ModuleKind::saturator)
        {
            auto& fx = pad.effects;
            fx[DrumMachine::satOn] = 1.0f;
            const int mode = ip (0, 0); // 0 Classic -> Clip(2), 1 Tape -> Tape(0), 2 Tube -> Tube(1)
            fx[DrumMachine::satType] = (float) (mode == 1 ? 0 : mode == 2 ? 1 : 2);
            fx[DrumMachine::satAmount] = juce::jlimit (5.0f, 100.0f, fp (0, 0.35f) * 100.0f);
            fx[DrumMachine::satMix] = juce::jlimit (15.0f, 100.0f, fp (2, 0.75f) * 100.0f);
            fx.limit();
            return true;
        }
        if (m.kind == MaschineKit::ModuleKind::distortion || m.kind == MaschineKit::ModuleKind::cabinet)
        {
            auto& fx = pad.effects;
            if (! fx.on (DrumMachine::satOn))
            {
                fx[DrumMachine::satOn] = 1.0f;
                fx[DrumMachine::satType] = (float) (m.kind == MaschineKit::ModuleKind::cabinet ? 1 : 3); // Tube or Fold
                fx[DrumMachine::satAmount] = juce::jlimit (10.0f, 100.0f, fp (0, 0.5f) * 80.0f);
                fx[DrumMachine::satMix] = juce::jlimit (20.0f, 100.0f, fp (6, 0.65f) * 100.0f);
                fx.limit();
            }
            else
            {
                pad.drive = juce::jlimit (pad.drive, 100.0f, pad.drive + fp (0, 0.3f) * 40.0f);
            }
            return true;
        }
        if (m.kind == MaschineKit::ModuleKind::transientDesigner)
        {
            auto& fx = pad.effects;
            fx[DrumMachine::transOn] = 1.0f;
            fx[DrumMachine::transAttack] = juce::jlimit (-100.0f, 100.0f, fp (1, 0.0f) * 100.0f);
            fx[DrumMachine::transSustain] = juce::jlimit (-100.0f, 100.0f, fp (2, 0.0f) * 100.0f);
            fx.limit();
            return true;
        }
        if (m.kind == MaschineKit::ModuleKind::compressor || m.kind == MaschineKit::ModuleKind::maximizer
            || m.kind == MaschineKit::ModuleKind::limiter)
        {
            auto& fx = pad.effects;
            if (! fx.on (DrumMachine::compOn) || m.kind == MaschineKit::ModuleKind::compressor)
            {
                fx[DrumMachine::compOn] = 1.0f;
                if (m.kind == MaschineKit::ModuleKind::compressor)
                {
                    fx[DrumMachine::compThreshold] = juce::jlimit (-48.0f, -2.0f, -40.0f + fp (0, 0.5f) * 38.0f);
                    fx[DrumMachine::compRatio] = juce::jlimit (1.5f, 16.0f, 1.5f + fp (1, 0.4f) * 12.0f);
                    fx[DrumMachine::compAttack] = logFreq (fp (2, 0.35f), 0.5f, 60.0f);
                    fx[DrumMachine::compRelease] = logFreq (fp (3, 0.35f), 20.0f, 600.0f);
                    fx[DrumMachine::compGain] = juce::jlimit (0.0f, 12.0f, juce::jmax (0.0f, fp (4, 0.5f) - 0.5f) * 16.0f);
                }
                else
                {
                    fx[DrumMachine::compThreshold] = juce::jlimit (-24.0f, -3.0f, -6.0f - fp (0, 0.25f) * 16.0f);
                    fx[DrumMachine::compRatio] = 12.0f;
                    fx[DrumMachine::compAttack] = 1.0f;
                    fx[DrumMachine::compRelease] = 80.0f;
                    fx[DrumMachine::compGain] = juce::jlimit (0.0f, 8.0f, fp (0, 0.25f) * 6.0f);
                }
                fx.limit();
            }
            return true;
        }
        if (m.kind == MaschineKit::ModuleKind::lofi)
        {
            const float bits = fp (0, 0.0f);
            const float sr = fp (2, 0.0f);
            const float mix = fp (3, 1.0f);
            pad.crush = juce::jmax (pad.crush, juce::jlimit (0.0f, 100.0f, juce::jmax (bits, sr) * mix * 100.0f));
            return true;
        }
        if (m.kind == MaschineKit::ModuleKind::metaverb || m.kind == MaschineKit::ModuleKind::reverb
            || m.kind == MaschineKit::ModuleKind::plateReverb || m.kind == MaschineKit::ModuleKind::iceverb)
        {
            const float mix = fp (0, 0.25f);
            const float room = fp (1, 0.5f);
            pad.reverb = juce::jmax (pad.reverb, juce::jlimit (0.0f, 100.0f, mix * 75.0f));
            plan.reverbSize = juce::jlimit (15.0f, 95.0f, room * 100.0f);
            return true;
        }
        if (m.kind == MaschineKit::ModuleKind::beatDelay || m.kind == MaschineKit::ModuleKind::reflex
            || m.kind == MaschineKit::ModuleKind::grainDelay)
        {
            const float wet = m.kind == MaschineKit::ModuleKind::beatDelay ? fp (4, fp (0, 0.25f)) : fp (0, 0.25f);
            const float fb = fp (1, 0.35f);
            pad.delay = juce::jmax (pad.delay, juce::jlimit (0.0f, 100.0f, wet * 70.0f));
            plan.feedback = juce::jlimit (10.0f, 75.0f, fb * 70.0f);
            return true;
        }
        return false;
    }

    /** Clean-room palette for Maschine's 16 pads so imported kits remain visually clear. */
    juce::Colour maschinePadColour (int index)
    {
        static const juce::uint32 colours[16] {
            0xffe8483c, 0xffd96932, 0xffd48f24, 0xffb8a629,
            0xff7cb038, 0xff3cb568, 0xff2bb096, 0xff28a8c4,
            0xff3b82e8, 0xff6e6ef0, 0xff9d5ef0, 0xffc652de,
            0xffd94ca6, 0xffe6446c, 0xffd97852, 0xff60a5c8
        };
        return juce::Colour (colours[(size_t) (index & 15)]);
    }
}

Plan fromMaschine (const MaschineKit::Kit& kit, const juce::File& kitFile, const juce::File& library)
{
    Plan plan;
    plan.name = kit.name.isNotEmpty() ? kit.name : kitFile.getFileNameWithoutExtension();

    int samplePads = 0, synthPads = 0, mappedFx = 0, pluginHosts = 0;
    std::set<juce::String> mappedFxNames, unmappedFxNames;
    std::set<int> notesTaken;
    std::vector<size_t> unplacedNotes;
    int closedHatPadIdx = -1, openHatPadIdx = -1;

    for (auto& s : kit.sounds)
    {
        if (s.modules.empty())
            continue;

        Pad pad;
        pad.name = s.name.isNotEmpty() ? s.name : ("Pad " + juce::String (s.index + 1));
        pad.level = maschineLevelDb (s.level);
        pad.pan = juce::jlimit (-100.0f, 100.0f, s.pan * 100.0f);
        pad.colour = maschinePadColour (s.index);

        int preferredNote = -1;
        bool hasInstrument = false;
        bool hasPluginHost = false;

        for (auto& m : s.modules)
        {
            if (m.kind == MaschineKit::ModuleKind::sampler || m.kind == MaschineKit::ModuleKind::audioModule)
            {
                for (auto& z : m.zones)
                {
                    if (! MaschineKit::isSafeRelativePath (z.relativePath))
                        continue;

                    const auto resolved = MaschineKit::resolve (z, kitFile, library);

                    if (resolved == juce::File())
                        continue;

                    if (! resolved.existsAsFile())
                        plan.missing.addIfNotAlreadyThere (resolved.getFullPathName());

                    const int lowV = juce::jlimit (1, 127, z.lowVelocity <= 0 ? 1 : z.lowVelocity);
                    const int highV = juce::jlimit (lowV, 127, z.highVelocity <= 0 ? 127 : z.highVelocity);
                    pad.layers.push_back ({ resolved, lowV, highV });
                }

                if (! m.zones.empty())
                {
                    const auto& z0 = m.zones.front();
                    pad.level = juce::jlimit (-48.0f, 6.0f, pad.level + maschineLevelDb (z0.gain));
                    pad.pan = juce::jlimit (-100.0f, 100.0f, pad.pan + z0.pan * 100.0f);
                    pad.tune = juce::jlimit (-24.0f, 24.0f, z0.tune + (m.samplerPitch - 0.5f) * 24.0f);

                    if (z0.totalFrames > 0 && z0.startFrame > 0)
                        pad.start = juce::jlimit (0.0f, 95.0f, 100.0f * (float) z0.startFrame / (float) z0.totalFrames);

                    if (m.kind == MaschineKit::ModuleKind::sampler && m.samplerFilterMode > 0)
                    {
                        pad.filter = juce::jlimit (1, 3, m.samplerFilterMode);
                        pad.cutoff = logFreq (m.samplerCutoff, 40.0f, 18000.0f);
                        pad.resonance = juce::jlimit (0.0f, 90.0f, m.samplerResonance * 85.0f);
                    }

                    hasInstrument = true;
                }
            }
            else if (m.kind == MaschineKit::ModuleKind::kick || m.kind == MaschineKit::ModuleKind::snare
                     || m.kind == MaschineKit::ModuleKind::hihat || m.kind == MaschineKit::ModuleKind::tom
                     || m.kind == MaschineKit::ModuleKind::perc || m.kind == MaschineKit::ModuleKind::cymbal
                     || m.kind == MaschineKit::ModuleKind::bassSynth)
            {
                if (! hasInstrument)
                {
                    mapMaschineDrumSynth (m, pad.name, pad, preferredNote);
                    hasInstrument = true;
                }
            }
            else if (m.kind == MaschineKit::ModuleKind::pluginHost)
            {
                hasPluginHost = true;
                ++pluginHosts;
            }
            else if (m.kind != MaschineKit::ModuleKind::unknown)
            {
                if (mapMaschinePadFx (m, pad, plan))
                {
                    ++mappedFx;
                    mappedFxNames.insert (MaschineKit::kindName (m.kind));
                }
                else
                {
                    unmappedFxNames.insert (MaschineKit::kindName (m.kind));
                }
            }
        }

        if (! hasInstrument)
        {
            if (hasPluginHost)
                continue;

            continue;
        }

        if (! pad.layers.empty())
            ++samplePads;
        else if (pad.synthesised)
            ++synthPads;

        // Smart GM drum note placement by name or Drum Synth kind
        if (preferredNote < 0)
        {
            const auto words = wordsOf (pad.name, true);
            if (const auto named = soundNamed (words); named.has_value())
                preferredNote = DrumMachine::padNote (*named);
        }

        if (preferredNote >= 0 && notesTaken.count (preferredNote) == 0)
        {
            pad.note = preferredNote;
            notesTaken.insert (preferredNote);
        }
        else
        {
            pad.note = -1;
            unplacedNotes.push_back (plan.pads.size());
        }

        if (pad.note == DrumMachine::padNote (Sound::closedHat))
            closedHatPadIdx = (int) plan.pads.size();
        else if (pad.note == DrumMachine::padNote (Sound::openHat))
            openHatPadIdx = (int) plan.pads.size();

        plan.pads.push_back (std::move (pad));
    }

    // Assign notes to remaining pads (prefer 82+ user pad notes)
    int nextNote = DrumMachine::firstUserNote;

    for (auto i : unplacedNotes)
    {
        while (nextNote < 128 && notesTaken.count (nextNote) > 0)
            ++nextNote;

        if (nextNote >= 128)
            for (nextNote = 36; nextNote < 128 && notesTaken.count (nextNote) > 0;)
                ++nextNote;

        plan.pads[i].note = juce::jlimit (0, 127, nextNote);
        notesTaken.insert (plan.pads[i].note);
    }

    // Group-level FX -> Kit Bus Drive & Glue
    for (auto& m : kit.groupFx)
    {
        if (! m.active)
            continue;

        if (m.kind == MaschineKit::ModuleKind::saturator || m.kind == MaschineKit::ModuleKind::distortion)
        {
            const float amt = m.floatParams.empty() ? 0.3f : m.floatParams[0];
            plan.kitDrive = juce::jmax (plan.kitDrive, juce::jlimit (0.0f, 65.0f, amt * 55.0f));
            ++mappedFx;
            mappedFxNames.insert ("Group " + MaschineKit::kindName (m.kind));
        }
        else if (m.kind == MaschineKit::ModuleKind::compressor || m.kind == MaschineKit::ModuleKind::maximizer
                 || m.kind == MaschineKit::ModuleKind::limiter)
        {
            const float amt = m.floatParams.empty() ? 0.35f : m.floatParams[0];
            plan.kitGlue = juce::jmax (plan.kitGlue, juce::jlimit (0.0f, 75.0f, 20.0f + amt * 50.0f));
            ++mappedFx;
            mappedFxNames.insert ("Group " + MaschineKit::kindName (m.kind));
        }
        else if (m.kind != MaschineKit::ModuleKind::unknown)
        {
            unmappedFxNames.insert ("Group " + MaschineKit::kindName (m.kind));
        }
    }

    // Automatic hat choke group if both closed and open hats exist
    if (closedHatPadIdx >= 0 && openHatPadIdx >= 0)
    {
        plan.pads[(size_t) closedHatPadIdx].choke = 1;
        plan.pads[(size_t) openHatPadIdx].choke = 1;
    }

    // Honest, transparent import report
    auto& report = plan.report;
    report.add (counted ((int) plan.pads.size(), "pad", "pads") + " imported from " + plan.name
                + " (" + counted (samplePads, "sample pad", "sample pads")
                + (synthPads > 0 ? ", " + counted (synthPads, "clean-room synth pad", "clean-room synth pads") : juce::String()) + ").");

    if (synthPads > 0)
        report.add (counted (synthPads, "Maschine Drum Synth pad was", "Maschine Drum Synth pads were")
                    + " mapped clean-room onto Basic Drums' own synthesized sources (tune, decay, tone and drive carried over).");

    if (mappedFx > 0)
    {
        juce::StringArray names;
        for (auto& n : mappedFxNames)
            names.add (n);
        report.add (counted (mappedFx, "effect was", "effects were")
                    + " translated clean-room onto Basic Drums' pad & bus DSP (" + names.joinIntoString (", ") + ").");
    }

    if (! unmappedFxNames.empty())
    {
        juce::StringArray names;
        for (auto& n : unmappedFxNames)
            names.add (n);
        report.add ("Left out (no direct per-pad equivalent in Basic Drums): " + names.joinIntoString (", ") + ".");
    }

    if (pluginHosts > 0)
        report.add (counted (pluginHosts, "pad hosted a third-party plug-in", "pads hosted third-party plug-ins")
                    + " (such as Massive or Monark) and was left out.");

    if (! kit.patterns.empty())
        report.add (counted ((int) kit.patterns.front().notes.size(), "embedded pattern note was", "embedded pattern notes were")
                    + " found in this kit (use Files > Export Maschine Pattern to MIDI... to save as .mid).");

    return plan;
}

Plan fromMpc (const MpcKit::Program& prog, const juce::File& xpmFile, const juce::File& library)
{
    Plan plan;
    plan.name = prog.name.isNotEmpty() ? prog.name : xpmFile.getFileNameWithoutExtension();

    int filtered = 0, enveloped = 0, choked = 0;

    for (auto& inst : prog.instruments)
    {
        if (inst.layers.empty())
            continue;

        Pad pad;
        pad.name = inst.name;
        pad.note = juce::jlimit (0, 127, inst.lowNote);
        pad.colour = maschinePadColour (inst.number > 0 ? inst.number - 1 : 0);

        const float baseGain = juce::jmax (1.0e-4f, inst.volume / 0.707946f);
        pad.level = juce::jlimit (-48.0f, 6.0f, 20.0f * std::log10 (baseGain));
        pad.pan = juce::jlimit (-100.0f, 100.0f, (inst.pan - 0.5f) * 200.0f);
        pad.tune = juce::jlimit (-24.0f, 24.0f, inst.tuneCoarse + inst.tuneFine * 0.01f);

        if (inst.muteGroup > 0 && inst.muteGroup <= DrumMachine::numChokeGroups)
        {
            pad.choke = inst.muteGroup;
            ++choked;
        }

        if (inst.filterType > 0)
        {
            pad.filter = juce::jlimit (1, 3, inst.filterType);
            pad.cutoff = logFreq (inst.cutoff, 40.0f, 18000.0f);
            pad.resonance = juce::jlimit (0.0f, 90.0f, inst.resonance * 85.0f);
            ++filtered;
        }

        if (inst.hasEnvelope)
        {
            pad.envelope = true;
            pad.attack = juce::jlimit (0.0f, 500.0f, inst.attack * 500.0f);
            pad.hold = juce::jlimit (0.0f, 2000.0f, inst.hold * 2000.0f);
            pad.decay = juce::jlimit (5.0f, 5000.0f, 5.0f + inst.decay * 4995.0f);
            ++enveloped;
        }

        for (auto& lay : inst.layers)
        {
            const auto resolved = MpcKit::resolve (lay, xpmFile, library);

            if (resolved == juce::File())
                continue;

            if (! resolved.existsAsFile())
                plan.missing.addIfNotAlreadyThere (resolved.getFullPathName());

            const int loV = juce::jlimit (1, 127, lay.lowVelocity <= 0 ? 1 : lay.lowVelocity);
            const int hiV = juce::jlimit (loV, 127, lay.highVelocity <= 0 ? 127 : lay.highVelocity);
            pad.layers.push_back ({ resolved, loV, hiV });
        }

        if (! pad.layers.empty())
        {
            const auto& l0 = inst.layers.front();
            pad.reverse = l0.reverse;
            pad.tune = juce::jlimit (-24.0f, 24.0f, pad.tune + l0.tuneCoarse + l0.tuneFine * 0.01f);
            if (l0.sliceEnd > l0.sliceStart && l0.sliceStart > 0)
                pad.start = juce::jlimit (0.0f, 95.0f, 100.0f * (float) l0.sliceStart / (float) l0.sliceEnd);
            plan.pads.push_back (std::move (pad));
        }
    }

    auto& report = plan.report;
    report.add (counted ((int) plan.pads.size(), "pad", "pads") + " imported from Akai MPC program " + plan.name + ".");
    if (choked > 0)
        report.add (counted (choked, "pad uses", "pads use") + " MPC mute/choke groups mapped to Basic Drums choke groups.");
    if (filtered > 0 || enveloped > 0)
        report.add ("Carried over " + counted (filtered, "pad filter", "pad filters") + " and " + counted (enveloped, "volume envelope", "volume envelopes") + ".");

    return plan;
}

//==============================================================================
bool isBatteryKit (const juce::File& file)
{
    return file.hasFileExtension ("nbkt;nbcl");
}

bool isMaschineKit (const juce::File& file)
{
    return file.hasFileExtension ("mxgrp;mxsnd");
}

bool isMpcKit (const juce::File& file)
{
    return file.hasFileExtension ("xpm");
}

namespace
{
    using Say = std::function<void (const juce::String&)>;

    void finish (juce::Component::SafePointer<juce::Component> window, BuiltInEffects::BasicDrumsProcessor& drums, Plan plan, const Say& done)
    {
        place (plan);

        if (const auto error = apply (drums, plan); error.isNotEmpty())
        {
            done (error);
            return;
        }

        done ("Imported " + plan.name + ": " + counted ((int) plan.pads.size(), "pad", "pads") + ". The samples are read where they are, not copied.");
        juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::InfoIcon).withTitle ("Imported " + plan.name)
                                          .withMessage (plan.report.joinIntoString ("\n\n")).withButton ("OK").withAssociatedComponent (window.getComponent()),
                                      nullptr);
    }

    void importRead (juce::Component::SafePointer<juce::Component> window, BuiltInEffects::BasicDrumsProcessor& drums, std::unique_ptr<juce::FileChooser>& chooser,
                     const Say& done, const juce::File& file)
    {
        auto kit = std::make_shared<BatteryKit::Kit>();

        if (const auto error = BatteryKit::read (file, *kit); error.isNotEmpty())
        {
            done (file.getFileName() + ": " + error);
            return;
        }

        auto plan = fromBattery (*kit, file);

        if (plan.missing.isEmpty())
        {
            finish (window, drums, std::move (plan), done);
            return;
        }

        // Its samples are not where it says: the library has moved, or is on a drive that is not there.
        const auto options = juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::QuestionIcon).withTitle ("Import " + plan.name)
                                 .withMessage (counted (plan.missing.size(), "sample is", "samples are") + " not where the kit says. The first is looked for at:\n\n" + plan.missing[0]
                                               + "\n\nIf its library has moved, choose the folder the library is in now.")
                                 .withButton ("Choose Folder...").withButton ("Import Without Them").withButton ("Cancel").withAssociatedComponent (window.getComponent());

        juce::AlertWindow::showAsync (options, [window, &drums, &chooser, done, file, kit, plan] (int chosen) mutable
        {
            if (window == nullptr || chosen == 0)
                return;

            if (chosen == 2)
            {
                finish (window, drums, std::move (plan), done);
                return;
            }

            chooser = std::make_unique<juce::FileChooser> ("Choose the folder the kit's library is in", file.getParentDirectory(), juce::String());
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                                  [window, &drums, done, file, kit] (const juce::FileChooser& c)
            {
                if (window != nullptr && c.getResult() != juce::File())
                    finish (window, drums, fromBattery (*kit, file, c.getResult()), done);
            });
        });
    }
    void importMaschineRead (juce::Component::SafePointer<juce::Component> window, BuiltInEffects::BasicDrumsProcessor& drums, std::unique_ptr<juce::FileChooser>& chooser,
                             const Say& done, const juce::File& file)
    {
        auto kit = std::make_shared<MaschineKit::Kit>();

        if (const auto error = MaschineKit::read (file, *kit); error.isNotEmpty())
        {
            done (file.getFileName() + ": " + error);
            return;
        }

        auto plan = fromMaschine (*kit, file);

        if (plan.missing.isEmpty())
        {
            finish (window, drums, std::move (plan), done);
            return;
        }

        const auto options = juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::QuestionIcon).withTitle ("Import " + plan.name)
                                 .withMessage (counted (plan.missing.size(), "sample is", "samples are") + " not where the Maschine kit says. The first is looked for at:\n\n" + plan.missing[0]
                                               + "\n\nIf its library has moved, choose the folder the library is in now.")
                                 .withButton ("Choose Folder...").withButton ("Import Without Them").withButton ("Cancel").withAssociatedComponent (window.getComponent());

        juce::AlertWindow::showAsync (options, [window, &drums, &chooser, done, file, kit, plan] (int chosen) mutable
        {
            if (window == nullptr || chosen == 0)
                return;

            if (chosen == 2)
            {
                finish (window, drums, std::move (plan), done);
                return;
            }

            chooser = std::make_unique<juce::FileChooser> ("Choose the folder the Maschine library is in", file.getParentDirectory(), juce::String());
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                                  [window, &drums, done, file, kit] (const juce::FileChooser& c)
            {
                if (window != nullptr && c.getResult() != juce::File())
                    finish (window, drums, fromMaschine (*kit, file, c.getResult()), done);
            });
        });
    }
    void importMpcRead (juce::Component::SafePointer<juce::Component> window, BuiltInEffects::BasicDrumsProcessor& drums, std::unique_ptr<juce::FileChooser>& chooser,
                        const Say& done, const juce::File& file)
    {
        auto prog = std::make_shared<MpcKit::Program>();

        if (const auto error = MpcKit::read (file, *prog); error.isNotEmpty())
        {
            done (file.getFileName() + ": " + error);
            return;
        }

        auto plan = fromMpc (*prog, file);

        if (plan.missing.isEmpty())
        {
            finish (window, drums, std::move (plan), done);
            return;
        }

        const auto options = juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::QuestionIcon).withTitle ("Import " + plan.name)
                                 .withMessage (counted (plan.missing.size(), "sample is", "samples are") + " not beside " + file.getFileName() + ".\n\nChoose the folder its samples are in.")
                                 .withButton ("Choose Folder...").withButton ("Import Without Them").withButton ("Cancel").withAssociatedComponent (window.getComponent());

        juce::AlertWindow::showAsync (options, [window, &drums, &chooser, done, file, prog, plan] (int chosen) mutable
        {
            if (window == nullptr || chosen == 0)
                return;

            if (chosen == 2)
            {
                finish (window, drums, std::move (plan), done);
                return;
            }

            chooser = std::make_unique<juce::FileChooser> ("Choose the folder the MPC samples are in", file.getParentDirectory(), juce::String());
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                                  [window, &drums, done, file, prog] (const juce::FileChooser& c)
            {
                if (window != nullptr && c.getResult() != juce::File())
                    finish (window, drums, fromMpc (*prog, file, c.getResult()), done);
            });
        });
    }
}

void importMaschineKit (juce::Component& window, BuiltInEffects::BasicDrumsProcessor& drums, std::unique_ptr<juce::FileChooser>& chooser, Say done, const juce::File& kitFile)
{
    const juce::Component::SafePointer<juce::Component> safe (&window);

    if (kitFile != juce::File())
    {
        importMaschineRead (safe, drums, chooser, done, kitFile);
        return;
    }

    chooser = std::make_unique<juce::FileChooser> ("Import a Maschine kit or sound of yours", juce::File(), "*.mxgrp;*.mxsnd");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [safe, &drums, &chooser, done] (const juce::FileChooser& c)
    {
        if (safe != nullptr && c.getResult() != juce::File())
            importMaschineRead (safe, drums, chooser, done, c.getResult());
    });
}

void importMpcKit (juce::Component& window, BuiltInEffects::BasicDrumsProcessor& drums, std::unique_ptr<juce::FileChooser>& chooser, Say done, const juce::File& xpmFile)
{
    const juce::Component::SafePointer<juce::Component> safe (&window);

    if (xpmFile != juce::File())
    {
        importMpcRead (safe, drums, chooser, done, xpmFile);
        return;
    }

    chooser = std::make_unique<juce::FileChooser> ("Import an Akai MPC Drum program (.xpm)", juce::File(), "*.xpm");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [safe, &drums, &chooser, done] (const juce::FileChooser& c)
    {
        if (safe != nullptr && c.getResult() != juce::File())
            importMpcRead (safe, drums, chooser, done, c.getResult());
    });
}

void exportMaschineMidi (juce::Component& window, std::unique_ptr<juce::FileChooser>& chooser, Say done, const juce::File& kitFile)
{
    const juce::Component::SafePointer<juce::Component> safe (&window);

    auto handleFile = [safe, &chooser, done] (const juce::File& src)
    {
        auto kit = std::make_shared<MaschineKit::Kit>();

        if (const auto err = MaschineKit::read (src, *kit); err.isNotEmpty())
        {
            done (src.getFileName() + ": " + err);
            return;
        }

        if (kit->patterns.empty())
        {
            done (src.getFileName() + " has no embedded MIDI patterns.");
            return;
        }

        // Build soundIndex -> Basic Drums MIDI note map from the kit's plan
        auto plan = fromMaschine (*kit, src);
        std::map<int, int> soundToNote;
        for (size_t i = 0; i < plan.pads.size() && i < kit->sounds.size(); ++i)
            soundToNote[kit->sounds[i].index] = plan.pads[i].note;

        const auto suggested = src.getParentDirectory().getChildFile (src.getFileNameWithoutExtension() + ".mid");
        chooser = std::make_unique<juce::FileChooser> ("Export Maschine pattern as MIDI", suggested, "*.mid");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                              [safe, done, kit, soundToNote] (const juce::FileChooser& c)
        {
            if (safe == nullptr || c.getResult() == juce::File())
                return;

            const auto out = c.getResult().withFileExtension (".mid");
            const auto err = MaschineKit::exportPatternToMidi (kit->patterns.front(), soundToNote, out);
            done (err.isNotEmpty() ? err : "Exported " + counted ((int) kit->patterns.front().notes.size(), "note", "notes") + " to " + out.getFileName() + ".");
        });
    };

    if (kitFile != juce::File())
    {
        handleFile (kitFile);
        return;
    }

    chooser = std::make_unique<juce::FileChooser> ("Choose a Maschine kit (.mxgrp) to export its pattern to MIDI", juce::File(), "*.mxgrp");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [safe, handleFile] (const juce::FileChooser& c)
    {
        if (safe != nullptr && c.getResult() != juce::File())
            handleFile (c.getResult());
    });
}

void importFolder (juce::Component& window, BuiltInEffects::BasicDrumsProcessor& drums, std::unique_ptr<juce::FileChooser>& chooser, Say done, const juce::File& folder)
{
    const juce::Component::SafePointer<juce::Component> safe (&window);

    const auto import = [] (juce::Component::SafePointer<juce::Component> to, BuiltInEffects::BasicDrumsProcessor& into, const Say& say, const juce::File& from)
    {
        auto plan = fromFolder (from);

        if (plan.pads.empty())
            say (from.getFileName() + " has no sound files in it.");
        else
            finish (to, into, std::move (plan), say);
    };

    if (folder != juce::File())
    {
        import (safe, drums, done, folder);
        return;
    }

    chooser = std::make_unique<juce::FileChooser> ("Import a folder of sounds, laid on the pads by their names", juce::File(), juce::String());
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories, [safe, &drums, done, import] (const juce::FileChooser& c)
    {
        if (safe != nullptr && c.getResult().isDirectory())
            import (safe, drums, done, c.getResult());
    });
}

void importBatteryKit (juce::Component& window, BuiltInEffects::BasicDrumsProcessor& drums, std::unique_ptr<juce::FileChooser>& chooser, Say done, const juce::File& kitFile)
{
    const juce::Component::SafePointer<juce::Component> safe (&window);

    if (kitFile != juce::File())
    {
        importRead (safe, drums, chooser, done, kitFile);
        return;
    }

    chooser = std::make_unique<juce::FileChooser> ("Import a Battery kit of yours", juce::File(), "*.nbkt;*.nbcl");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [safe, &drums, &chooser, done] (const juce::FileChooser& c)
    {
        if (safe != nullptr && c.getResult() != juce::File())
            importRead (safe, drums, chooser, done, c.getResult());
    });
}
}
