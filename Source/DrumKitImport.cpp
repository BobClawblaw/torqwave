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

    /** How many of a kit's cells Battery lets cut each other off: the ones in a voice group of one
        voice that another cell is in too, and the ones whose groups share an exclusive group. */
    int chokedCells (const BatteryKit::Kit& kit)
    {
        std::map<int, int> inGroup, inExclusive;

        const auto groupOf = [&kit] (const BatteryKit::Cell& c) -> const BatteryKit::VoiceGroup*
        {
            return ! c.layers.empty() && juce::isPositiveAndBelow (c.voiceGroup, (int) kit.voiceGroups.size()) ? &kit.voiceGroups[(size_t) c.voiceGroup] : nullptr;
        };

        for (auto& c : kit.cells)
        {
            if (auto* g = groupOf (c))
            {
                ++inGroup[c.voiceGroup];

                if (g->exclusiveGroup >= 0)
                    ++inExclusive[g->exclusiveGroup];
            }
        }

        int choked = 0;

        for (auto& c : kit.cells)
            if (auto* g = groupOf (c))
                if ((g->voices == 1 && inGroup[c.voiceGroup] > 1) || (g->exclusiveGroup >= 0 && inExclusive[g->exclusiveGroup] > 1))
                    ++choked;

        return choked;
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

    int samples = 0, faded = 0, stacked = 0, thinned = 0, wide = 0, ownValues = 0, ownStarts = 0, ends = 0, loops = 0, otherTypes = 0, unreadable = 0, emptied = 0;

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

    if (const auto choked = kit.voiceGroupsRead ? chokedCells (kit) : 0; choked > 0)
        report.add (counted (choked, "cell cuts", "cells cut") + " others off in the kit (hi-hats, usually). Pads here cut each other off by their notes alone: the hats on 42, 44 and 46 do, the rest ring.");

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

    report.add ("Battery's effects, envelopes, modulation, time-stretching, articulations and buses are not brought over.");
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
        return notThere > 0 ? "None of " + plan.name + "'s samples are where the kit says, so nothing was changed."
                            : "None of " + plan.name + "'s samples could be read, so nothing was changed. " + firstReason;

    auto settings = DrumMachine::defaultKit();

    for (auto& p : plan.pads)
    {
        if (! juce::isPositiveAndBelow (p.pad, DrumMachine::numPads) || layers[(size_t) p.pad].empty())
            continue;

        auto s = DrumMachine::defaultUserSettings();
        s.tune = juce::jlimit (-24.0f, 24.0f, p.tune);
        s.level = juce::jlimit (-48.0f, 6.0f, p.level);
        s.pan = juce::jlimit (-100.0f, 100.0f, p.pan);
        s.start = juce::jlimit (0.0f, 95.0f, p.start);
        s.reverse = p.reverse;

        if (p.pad < DrumMachine::numSounds)
            settings.sounds[(size_t) p.pad] = s;
        else
            settings.user[(size_t) (p.pad - DrumMachine::numSounds)].settings = s;
    }

    drums.applyKit ({ plan.name, {}, settings });

    for (auto& p : plan.pads)
    {
        if (! juce::isPositiveAndBelow (p.pad, DrumMachine::numPads) || layers[(size_t) p.pad].empty())
            continue;

        drums.setPadName (p.pad, p.name);
        drums.setPadColour (p.pad, p.colour);
        drums.setPadNote (p.pad, p.note);     // a user pad's: applying a kit puts every pad back on its own
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
bool isBatteryKit (const juce::File& file)
{
    return file.hasFileExtension ("nbkt;nbcl");
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
