#include <JuceHeader.h>
#include "../Source/SequenceLibraryView.h"
#include "../Source/SequencerView.h"
#include "../Source/AudioEngine.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/WaveSynth.h"
#include "../Source/DrumMachine.h"
#include "../Source/NoteTools.h"
#include "../Source/Instrument.h"
#include "TestUtils.h"
#include "PresetTests.h"

namespace
{
    using TestUtils::check;

    struct Found
    {
        juce::File file;
        std::unique_ptr<juce::XmlElement> xml;
    };

    /** Every preset file under a folder, read. */
    std::vector<Found> readAll (const juce::File& folder)
    {
        std::vector<Found> all;

        for (const auto& f : juce::RangedDirectoryIterator (folder, true, "*.tqseq;*.tqkit", juce::File::findFiles))
            all.push_back ({ f.getFile(), juce::XmlDocument::parse (f.getFile()) });

        std::sort (all.begin(), all.end(), [] (auto& a, auto& b) { return a.file.getFullPathName() < b.file.getFullPathName(); });
        return all;
    }

    /** The sequences in a file: itself, or a kit's parts. */
    std::vector<const juce::XmlElement*> sequencesOf (const juce::XmlElement& xml)
    {
        if (xml.hasTagName ("TORQWAVE_SEQUENCE"))
            return { &xml };

        std::vector<const juce::XmlElement*> parts;

        for (auto* e : xml.getChildWithTagNameIterator ("TORQWAVE_SEQUENCE"))
            parts.push_back (e);

        return parts;
    }

    bool playsOnDrums (const SequenceFile::Sequence& q)
    {
        return q.voice != nullptr && q.voice->toString().contains (BuiltInEffects::basicDrumsId);
    }

    /** The rules every preset keeps (docs/plans/PLAN-0.19.3.md, step 6). What breaks one goes into `problems`. */
    void checkRules (const SequenceFile::Sequence& q, const juce::XmlElement& xml, juce::StringArray& problems)
    {
        const auto drums = playsOnDrums (q);
        const auto chromatic = q.tags.contains ("Chromatic");
        const std::set<int> mapped { 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 56, 57, 59, 69, 70 };
        std::set<int> scale { 9, 11, 0, 2, 4, 5, 7 };   // A natural minor

        if (q.tags.contains ("Phrygian"))
            scale.insert (10);

        if (q.tags.contains ("Harmonic minor"))
            scale.insert (8);

        const auto say = [&] (const juce::String& what) { problems.add (q.name + ": " + what); };
        const auto bars = q.bars();

        if (! (juce::exactlyEqual (bars, 1.0) || juce::exactlyEqual (bars, 2.0) || juce::exactlyEqual (bars, 4.0) || juce::exactlyEqual (bars, 8.0)))
            say ("its length is not 1, 2, 4 or 8 bars");

        if (q.notes.empty())
            say ("no notes");

        if (q.note.trim().isEmpty() || xml.getStringAttribute ("style").isEmpty())
            say ("no description or style");

        if (q.name.trim().isEmpty() || q.role.isEmpty() || q.subRole.isEmpty() || q.voice == nullptr)
            say ("no name, role, sub-role or voice");

        if (! drums && NoteTools::parseKey (q.key) != NoteTools::parseKey ("A minor"))
            say ("not in A minor");

        for (auto& n : q.notes)
        {
            if (n.start % (SequenceFile::ticksPerBeat / 4) != 0)
                say ("a note off the 16th grid at " + juce::String (n.start));

            if (n.start + n.length > q.length + 20 || n.length <= 0 || n.pitch < 0 || n.pitch > 127 || n.velocity <= 0.0f || n.velocity > 1.0f)
                say ("a note out of range at " + juce::String (n.start));

            if (drums && mapped.count (n.pitch) == 0)
                say ("a drum note not on the map: " + juce::String (n.pitch));

            if (! drums && ! chromatic && scale.count (n.pitch % 12) == 0)
                say ("a note outside the key: " + juce::String (n.pitch));
        }

        // Bass lines are mono; a slide overlaps the next note by exactly 20 ticks.
        if (q.role == "Bass")
        {
            auto notes = q.notes;
            std::sort (notes.begin(), notes.end(), [] (auto& a, auto& b) { return a.start < b.start; });

            for (size_t i = 1; i < notes.size(); ++i)
                if (const auto overlap = notes[i - 1].start + notes[i - 1].length - notes[i].start; overlap > 0 && overlap != 20)
                    say ("not mono at " + juce::String (notes[i].start));
        }
    }

    /** A click in a rendered preset: a jump between two samples that stands alone. Its measure is the
        step as a ratio to the largest step in the 30 ms either side of it (the millisecond around it left
        out), for steps of `floor` or more that are also three times the steps just before and after them.
        A voice cut off, or a wave changed between two samples, puts one such step there with nothing like
        it around; a bright sound's own edges, a drum's noise, a crushed sound's stairs and a fast attack
        (several samples rising alike) have their like beside them. `starts` are sample positions whose
        next 3 ms are left out: a drum's hit begins with a jump, which is the stick. */
    struct Click
    {
        float ratio = 0.0f, step = 0.0f;
        double seconds = 0.0;
    };

    Click worstClick (const juce::AudioBuffer<float>& b, double rate, float floor, const std::vector<int>& starts = {})
    {
        Click worst;
        const auto size = juce::jmax (1, (int) (rate / 1000.0));
        const auto blocks = b.getNumSamples() / size;

        for (int ch = 0; ch < b.getNumChannels(); ++ch)
        {
            const auto* x = b.getReadPointer (ch);
            std::vector<float> largest ((size_t) blocks, 0.0f);
            std::vector<int> where ((size_t) blocks, 0);

            for (int i = 1; i < blocks * size; ++i)
                if (const auto step = std::abs (x[i] - x[i - 1]); step > largest[(size_t) (i / size)])
                {
                    largest[(size_t) (i / size)] = step;
                    where[(size_t) (i / size)] = i;
                }

            for (int at = 0; at < blocks; ++at)
            {
                const auto i = where[(size_t) at];

                if (largest[(size_t) at] < floor || i < 2 || i + 1 >= b.getNumSamples()
                    || largest[(size_t) at] < 3.0f * juce::jmax (std::abs (x[i - 1] - x[i - 2]), std::abs (x[i + 1] - x[i])))
                    continue;

                if (std::any_of (starts.begin(), starts.end(), [&] (int start) { return i >= start && i < start + (int) (0.003 * rate); }))
                    continue;

                auto around = 0.0f;

                for (int other = juce::jmax (0, at - 31); other < juce::jmin (blocks, at + 32); ++other)
                    if (std::abs (other - at) > 1)
                        around = juce::jmax (around, largest[(size_t) other]);

                if (const auto ratio = largest[(size_t) at] / juce::jmax (around, 1.0e-6f); ratio > worst.ratio)
                    worst = { ratio, largest[(size_t) at], (double) where[(size_t) at] / rate };
            }
        }

        return worst;
    }

    /** The instrument settings a voice carries, loaded into a fresh Torqwave Synth. */
    std::unique_ptr<BuiltInEffects::BasicSynthProcessor> synthFrom (const juce::XmlElement& voice)
    {
        auto* slot = voice.getChildByName ("EFFECTS") != nullptr ? voice.getChildByName ("EFFECTS")->getChildByName ("SLOT") : nullptr;
        auto* state = slot != nullptr ? slot->getChildByName ("STATE") : nullptr;
        juce::MemoryBlock block;

        if (state == nullptr || ! block.fromBase64Encoding (state->getAllSubText()))
            return nullptr;

        auto synth = std::make_unique<BuiltInEffects::BasicSynthProcessor>();
        synth->setStateInformation (block.getData(), (int) block.getSize());
        return synth;
    }
}

void runPresetTests (AudioEngine& engine)
{
    const auto before = SequenceLibrary::factoryFolder();
    const auto base = juce::File::createTempFile ("presets");
    const auto presets = base.getChildFile ("Factory Presets").getChildFile ("set");
    const auto mine = base.getChildFile ("Mine");
    SequenceLibrary::setFactoryFolder (presets);
    SequenceLibrary::setChosenFolder (mine);

    // Unpacked from the app: every file read-only, once; another set beside it is replaced.
    const auto older = presets.getSiblingFile ("older");
    older.getChildFile ("Bass").createDirectory();
    const auto unpackError = SequenceLibrary::unpackFactoryPresets();
    const auto all = readAll (presets);
    const auto readOnly = std::all_of (all.begin(), all.end(), [] (auto& f) { return ! f.file.hasWriteAccess(); });
    const auto stampBefore = presets.getChildFile (".complete").getLastModificationTime();
    const auto again = SequenceLibrary::unpackFactoryPresets();
    int sequences = 0, kits = 0;

    for (auto& f : all)
        (f.file.hasFileExtension (SequenceFile::kitExtension) ? kits : sequences) += 1;

    check (unpackError.isEmpty() && again.isEmpty() && sequences >= 1000 && kits >= 100 && readOnly && ! older.exists()
               && presets.getChildFile (".complete").getLastModificationTime() == stampBefore,
           "presets: the app's presets unpack into a folder of their own (" + juce::String (sequences) + " sequences, " + juce::String (kits)
               + " kits), read-only, replacing an earlier set, and only once");

    // The rules every preset keeps.
    {
        juce::StringArray problems, names;
        std::map<juce::String, int> perRole;
        int checked = 0;

        for (auto& f : all)
        {
            if (f.xml == nullptr)
            {
                problems.add (f.file.getFileName() + ": not XML");
                continue;
            }

            if (f.xml->hasTagName ("TORQWAVE_SEQUENCE"))
            {
                ++perRole[f.xml->getStringAttribute ("role")];

                if (f.file.getParentDirectory().getRelativePathFrom (presets) != f.xml->getStringAttribute ("role") + "/" + f.xml->getStringAttribute ("subRole"))
                    problems.add (f.file.getFileName() + ": not in its role's and sub-role's folder");
            }

            names.add (f.xml->getStringAttribute ("name"));

            for (auto* x : sequencesOf (*f.xml))
                if (const auto q = SequenceFile::fromXml (*x))
                {
                    checkRules (*q, *x, problems);
                    ++checked;
                }
                else
                {
                    problems.add (f.file.getFileName() + ": not a sequence");
                }

            // A kit's parts agree on the key and the tempo.
            if (f.xml->hasTagName ("TORQWAVE_KIT"))
            {
                std::set<juce::String> tempos;
                const auto kitKey = NoteTools::parseKey (f.xml->getStringAttribute ("key"));

                for (auto* x : sequencesOf (*f.xml))
                {
                    tempos.insert (x->getStringAttribute ("writtenTempo"));

                    if (x->getStringAttribute ("key").isNotEmpty() && NoteTools::parseKey (x->getStringAttribute ("key")) != kitKey)
                        problems.add (f.file.getFileName() + ": a part in another key");
                }

                if (tempos.size() != 1 || sequencesOf (*f.xml).size() < 6)
                    problems.add (f.file.getFileName() + ": parts at different tempos, or fewer than six");
            }
        }

        auto unique = names;
        unique.removeDuplicates (true);
        const auto everyRole = std::all_of (SequenceLibrary::roles().begin(), SequenceLibrary::roles().end(), [&perRole] (auto& r)
        {
            return r.subRoles.isEmpty() || perRole[r.name] >= 20;
        });

        check (problems.isEmpty() && unique.size() == names.size() && everyRole,
               "presets: every role has at least 20 (Drums " + juce::String (perRole["Drums"]) + ", Bass " + juce::String (perRole["Bass"]) + "); the "
                   + juce::String (checked) + " sequences in them are on the 16th grid in 1, 2, 4 or 8 bars, drums on the General MIDI map, bass mono but for "
                   "20-tick slides, the rest in A minor (or the Phrygian, harmonic minor or chromatic notes they are tagged with), each with a description and "
                   "a style, in its role's folder; names are unique; each kit's six parts share its key and tempo"
                   + (problems.isEmpty() ? juce::String() : " (" + problems.joinIntoString ("; ").substring (0, 600) + ")"));
    }

    // Running the script again writes the same files.
    {
        const juce::File source (TORQWAVE_SOURCE_DIR);
        juce::ChildProcess python;
        const auto started = python.start (juce::StringArray { "python3", source.getChildFile ("Scripts/make_presets.py").getFullPathName(), "--check" });
        const auto said = started ? python.readAllProcessOutput().trim() : juce::String();

        // The list of patches and kits the script picks voices from is the instruments' own.
        {
            juce::String now;

            for (auto& patch : WaveSynth::factoryPatches())
                now << "patch|" << patch.category << "|" << patch.name << "\n";

            for (auto& kit : DrumMachine::kits())
                now << "kit|" << kit.name << "|" << kit.description << "\n";

            check (source.getChildFile ("Scripts/voice_names.txt").loadFileAsString() == now,
                   "presets: Scripts/voice_names.txt lists the synth's factory patches and the drum kits as they are (TorqwaveTests --voice-names writes it)");
        }

        if (started)
            check (said == "same" && python.getExitCode() == 0,
                   "presets: Scripts/make_presets.py run again writes Data/Presets byte for byte (" + said + ")");
        else
            std::cout << "SKIP  presets: python3 isn't here to run Scripts/make_presets.py again" << std::endl;
    }

    // A voice names a factory patch or kit, and the instrument loads it as it is now: an acid line on one of
    // the synth's Acid patches, a drum groove on one of Basic Drums' kits, channel 10.
    {
        juce::String error;
        const auto acid = SequenceFile::load (presets.getChildFile ("Bass/Arp Bass/Acid Root Jumper.tqseq"), error);
        const auto synth = acid.has_value() && acid->voice != nullptr ? synthFrom (*acid->voice) : nullptr;
        const auto groove = SequenceFile::load (presets.getChildFile ("Drums/Full Grooves/Peak Hour Driver.tqseq"), error);
        const auto patch = synth != nullptr ? synth->getPatchName() : juce::String();
        juce::String category;

        for (auto& p : WaveSynth::factoryPatches())
            if (p.name == patch)
                category = p.category;

        juce::String kit;

        if (groove.has_value() && groove->voice != nullptr)
            if (auto* slot = groove->voice->getChildByName ("EFFECTS") != nullptr ? groove->voice->getChildByName ("EFFECTS")->getChildByName ("SLOT") : nullptr)
                if (auto* state = slot->getChildByName ("STATE"))
                {
                    juce::MemoryBlock block;
                    block.fromBase64Encoding (state->getAllSubText());
                    BuiltInEffects::BasicDrumsProcessor drums;
                    drums.setStateInformation (block.getData(), (int) block.getSize());
                    kit = drums.getKitName();
                }

        check (category == "Acid" && acid->voice->getStringAttribute ("name") == "Torqwave Synth: " + patch && SequenceFile::instrumentOf (*acid) == "Torqwave Synth"
                   && DrumMachine::kitIndex (kit) >= 0 && groove->voice->getStringAttribute ("name") == "Basic Drums: " + kit
                   && SequenceFile::instrumentOf (*groove) == "Basic Drums" && groove->channel == 10,
               "presets: a preset's voice names a factory patch or kit that the instrument loads (the acid line on " + patch + ", an Acid patch; the groove on the "
                   + kit + " kit, channel 10)");
    }

    // Every preset sounds on its own voice, neither silent nor clipping: its notes at its written tempo,
    // through its instrument at the settings it carries.
    {
        juce::StringArray problems;
        float loudest = 0.0f, quietest = 1.0f;
        juce::String loudestName, quietestName;
        juce::StringArray loudKits;
        float loudestKit = 0.0f;
        juce::String loudestKitName;
        juce::StringArray clicks;
        Click worstOfAll;
        juce::String worstName;

        for (auto& f : all)
        {
            // A kit's parts added up as they start together, each at its track gain.
            juce::AudioBuffer<float> together;

            for (auto* x : f.xml != nullptr ? sequencesOf (*f.xml) : std::vector<const juce::XmlElement*> {})
            {
                const auto q = SequenceFile::fromXml (*x);
                auto* slot = q.has_value() && q->voice != nullptr && q->voice->getChildByName ("EFFECTS") != nullptr
                                 ? q->voice->getChildByName ("EFFECTS")->getChildByName ("SLOT") : nullptr;
                juce::PluginDescription description;

                if (slot == nullptr || slot->getChildByName ("PLUGIN") == nullptr || ! description.loadFromXml (*slot->getChildByName ("PLUGIN")))
                {
                    problems.add ((q.has_value() ? q->name : f.file.getFileName()) + ": no instrument");
                    continue;
                }

                juce::MemoryBlock state;

                if (auto* s = slot->getChildByName ("STATE"))
                    state.fromBase64Encoding (s->getAllSubText());

                // The voice names a factory patch or kit there is: its label says which, and its state asks for it.
                {
                    const auto label = q->voice->getStringAttribute ("name");
                    const auto named = label.fromFirstOccurrenceOf (": ", false, false);
                    const auto text = state.getSize() > 8 ? juce::String::fromUTF8 ((const char*) state.getData() + 8, (int) state.getSize() - 8) : juce::String();
                    auto known = false;

                    if (label.startsWith ("Basic Drums: "))
                        known = DrumMachine::kitIndex (named) >= 0 && text.contains ("factoryKit=\"" + named + "\"");
                    else if (label.startsWith ("Torqwave Synth: "))
                        known = text.contains ("factoryPatch=\"" + named + "\"")
                                    && std::any_of (WaveSynth::factoryPatches().begin(), WaveSynth::factoryPatches().end(), [&] (const WaveSynth::Patch& p) { return p.name == named; });

                    if (! known)
                        problems.add (q->name + ": its voice " + label.quoted() + " names no factory patch or kit");
                }

                const auto secondsPerTick = 60.0 / (q->writtenTempo * SequenceFile::ticksPerBeat);
                Instrument::Phrase phrase;
                phrase.lengthSeconds = (double) q->length * secondsPerTick + 1.0;

                for (auto& n : q->notes)
                    phrase.notes.push_back ({ n.pitch, (double) n.start * secondsPerTick, (double) n.length * secondsPerTick, n.velocity });

                juce::AudioBuffer<float> out;
                const auto error = Instrument::render (engine.pluginFormatManager, description, state, phrase, 48000.0, 2, out);
                const auto peak = out.getMagnitude (0, 0, out.getNumSamples());
                const auto gain = juce::Decibels::decibelsToGain ((float) q->voice->getDoubleAttribute ("gainDb"));

                // For Scripts/make_presets.py --calibrate: each preset's peak without its track gain.
                if (const auto levels = juce::SystemStats::getEnvironmentVariable ("TQ_PRESET_LEVELS", {}); levels.isNotEmpty() && error.isEmpty())
                    juce::File (levels).appendText (q->name + "|" + juce::String (juce::Decibels::gainToDecibels (peak, -200.0f), 2) + "\n");

                // A part in a kit sits 6 dB lower than it does alone, since six play together.
                const auto least = f.file.hasFileExtension (SequenceFile::kitExtension) ? -30.0f : -24.0f;

                if (error.isNotEmpty() || peak * gain < juce::Decibels::decibelsToGain (least) || peak * gain > juce::Decibels::decibelsToGain (-1.0f))
                    problems.add (q->name + ": " + (error.isNotEmpty() ? error : "peak " + juce::String (juce::Decibels::gainToDecibels (peak * gain), 1) + " dB"));

                // No clicks: no jump in one sample standing alone, four times anything in the 30 ms either side.
                if (error.isEmpty())
                {
                    // A drum's hit starts with a jump of its own; what is checked there is everything after it.
                    std::vector<int> hits;

                    if (description.fileOrIdentifier == BuiltInEffects::basicDrumsId)
                        for (auto& n : phrase.notes)
                            hits.push_back ((int) std::floor (n.start * 48000.0));

                    const auto click = worstClick (out, 48000.0, 0.03f / juce::jmax (gain, 1.0e-6f), hits);

                    if (click.ratio > worstOfAll.ratio)
                        worstOfAll = click, worstName = q->name;

                    if (click.ratio > 4.0f)
                        clicks.add (q->name + " (" + q->voice->getStringAttribute ("name") + ") " + juce::String (click.step * gain, 3) + " at " + juce::String (click.seconds, 3)
                                        + " s, " + juce::String (click.ratio, 1) + " times its surroundings");
                }

                if (peak * gain > loudest)
                    loudest = peak * gain, loudestName = q->name;

                if (peak * gain < quietest && ! f.file.hasFileExtension (SequenceFile::kitExtension))
                    quietest = peak * gain, quietestName = q->name;

                if (f.file.hasFileExtension (SequenceFile::kitExtension) && error.isEmpty())
                {
                    if (out.getNumSamples() > together.getNumSamples())
                        together.setSize (2, out.getNumSamples(), true, true, false);

                    for (int ch = 0; ch < 2; ++ch)
                        together.addFrom (ch, 0, out, juce::jmin (ch, out.getNumChannels() - 1), 0, out.getNumSamples(), gain);
                }
            }

            if (together.getNumSamples() > 0)
            {
                const auto kitPeak = together.getMagnitude (0, together.getNumSamples());

                if (kitPeak > loudestKit)
                    loudestKit = kitPeak, loudestKitName = f.file.getFileNameWithoutExtension();

                if (kitPeak > juce::Decibels::decibelsToGain (-1.0f))
                    loudKits.add (f.file.getFileNameWithoutExtension() + " " + juce::String (juce::Decibels::gainToDecibels (kitPeak), 1) + " dB");
            }
        }

        check (problems.isEmpty(),
               "presets: every one names a factory patch or kit and sounds on it (its notes at its tempo), peaking between -24 and -1 dB, a kit's parts 6 dB lower (the loudest " + loudestName + " at "
                   + juce::String (juce::Decibels::gainToDecibels (loudest), 1) + " dB, the quietest " + quietestName + " at "
                   + juce::String (juce::Decibels::gainToDecibels (quietest), 1) + " dB)"
                   + (problems.isEmpty() ? juce::String() : " (" + problems.joinIntoString ("; ").substring (0, 800) + ")"));

        check (loudKits.isEmpty(),
               "presets: a kit's parts played together stay under the master's top, peaking below -1 dB (the loudest, " + loudestKitName + ", at "
                   + juce::String (juce::Decibels::gainToDecibels (loudestKit), 1) + " dB)"
                   + (loudKits.isEmpty() ? juce::String() : " (" + juce::String (loudKits.size()) + ": " + loudKits.joinIntoString ("; ").substring (0, 600) + ")"));

        if (const auto list = juce::SystemStats::getEnvironmentVariable ("TQ_PRESET_CLICKS", {}); list.isNotEmpty())
            juce::File (list).replaceWithText (clicks.joinIntoString ("\n") + "\n");

        check (clicks.isEmpty(),
               "presets: none clicks on its voice: no jump in one sample of 0.03 or more (three times the steps beside it) stands alone, four times the largest step in the 30 ms either side, a drum hit's first 3 ms left out (the nearest, "
                   + worstName + ", " + juce::String (worstOfAll.ratio, 1) + " times)"
                   + (clicks.isEmpty() ? juce::String() : " (" + juce::String (clicks.size()) + ": " + clicks.joinIntoString ("; ").substring (0, 800) + ")"));
    }

    // In the library, under Presets above the user's own; the user's folder untouched.
    {
        SequenceLibraryView view (mine);
        const auto folders = view.shownFolders();
        const auto shown = view.shownFiles();
        const auto shownPresets = std::count_if (shown.begin(), shown.end(), [] (auto& f) { return SequenceLibrary::isFactory (f); });
        int ownFiles = 0;

        for (const auto& f : juce::RangedDirectoryIterator (mine, true, "*.tqseq;*.tqkit", juce::File::findFiles))
            ownFiles += f.getFile().exists() ? 1 : 0;

        check (folders[0] == "Presets" && folders[1] == "Presets > Drums" && folders[2] == "Presets > Drums > Full Grooves"
                   && folders.contains ("Presets > Kits") && folders.contains ("Drums > Kick") && (int) shownPresets == (int) all.size() && ownFiles == 0,
               "presets: the library shows Presets first, by role and sub-role in their order (" + folders.joinIntoString (", ").substring (0, 120)
                   + "...), every preset there, the user's own folders after it, and nothing written to the user's folder");

        view.setSearch ("presets acid");
        const auto found = view.shownFiles();
        const auto allAcid = ! found.empty() && std::all_of (found.begin(), found.end(), [] (auto& f) { return SequenceLibrary::isFactory (f); });
        view.setSearch ({});

        // Rated and marked in a file beside them; the preset itself as it was.
        const auto groove = presets.getChildFile ("Drums/Full Grooves/Rumble Roller.tqseq");
        const auto text = groove.loadFileAsString();
        const auto rated = SequenceLibrary::setRating (groove, 4);
        const auto liked = SequenceLibrary::setFavourite (groove, true);
        view.refresh();
        view.setFavouritesOnly (true);
        const auto favourites = view.shownFiles();
        view.setFavouritesOnly (false);
        const auto entries = SequenceLibrary (presets).scan();
        const auto entry = std::find_if (entries.begin(), entries.end(), [&groove] (auto& e) { return e.file == groove; });
        check (allAcid && rated.isEmpty() && liked.isEmpty() && groove.loadFileAsString() == text && SequenceLibrary::factoryRatingsFile().existsAsFile()
                   && entry != entries.end() && entry->rating == 4 && entry->favourite && entry->factory
                   && favourites.size() == 1 && favourites[0] == groove,
               "presets: found by search (\"presets acid\" finds only presets); rated and marked as a favourite beside the presets, the preset's file unchanged, "
               "and Favourites shows it");

        // Copy to My Library: the user's copy in the same role and sub-role, with its notes.
        juce::String error;
        const auto bass = presets.getChildFile ("Bass/Synth Bass/Belgian Body Bass.tqseq");
        view.copyToLibrary ({ bass });
        const auto copy = mine.getChildFile ("Bass/Synth Bass/Belgian Body Bass.tqseq");
        const auto original = SequenceFile::load (bass, error);
        const auto copied = SequenceFile::load (copy, error);
        check (copy.existsAsFile() && copy.hasWriteAccess() && original.has_value() && copied.has_value() && copied->notes == original->notes
                   && copied->role == "Bass" && copied->subRole == "Synth Bass" && bass.existsAsFile(),
               "presets: Copy to My Library puts a copy, with its notes, in the user's Bass > Synth Bass, and the preset stays");
        copy.deleteFile();
    }

    // Edit in Sequencer: a new copy, saved in the user's library, the preset untouched.
    {
        const auto lead = presets.getChildFile ("Melody/Hooks/Alarm Line.tqseq");
        const auto text = lead.loadFileAsString();
        SequencerView view (engine);
        view.openFile (lead);
        const auto target = view.getDocument().getFile();
        const auto newCopy = target == mine.getChildFile ("Melody/Hooks/Alarm Line.tqseq") && ! target.exists() && view.getDocument().isDirty()
                             && view.getDocument().get().name == "Alarm Line";
        const auto saved = view.save();
        juce::String error;
        const auto back = SequenceFile::load (target, error);
        const auto original = SequenceFile::load (lead, error);
        juce::Array<juce::File> files;
        const auto menu = SequencerView::presetsMenu (files);
        check (newCopy && saved.isEmpty() && back.has_value() && original.has_value() && back->notes == original->notes && lead.loadFileAsString() == text
                   && ! view.getDocument().isDirty() && files.size() == sequences && menu.getNumItems() == 7,
               "presets: Edit in Sequencer opens a preset as a new copy, not yet saved, that Save writes to the user's Melody > Hooks with its notes, the preset untouched; "
               "the Presets button lists all " + juce::String (files.size()) + " by their seven roles");
        target.deleteFile();

        // Opened while it had no room (as at start-up, from the Finder), then laid out, and opened
        // after a sequence pitched far above it: each time all its notes are in view.
        const auto inView = [] (SequencerView& v, const juce::File& f)
        {
            juce::String e;
            const auto q = SequenceFile::load (f, e);

            if (! q.has_value())
                return false;

            const auto low = v.getEditor().getLowPitch(), high = v.getEditor().getHighPitch();
            auto lowest = 127, highest = 0;

            for (auto& n : q->notes)
            {
                lowest = juce::jmin (lowest, n.pitch);
                highest = juce::jmax (highest, n.pitch);
            }

            return lowest >= low && highest <= high;
        };

        const auto bass = presets.getChildFile ("Bass/Arp Bass/Acid Slide Rider.tqseq");
        SequencerView fresh (engine);
        fresh.openFile (bass);
        fresh.setSize (1200, 700);
        const auto bassLow = fresh.getEditor().getLowPitch();
        const juce::TemporaryFile users (".tqseq");   // the user's own, so it opens unchanged and the next opens without asking to save
        lead.copyFileTo (users.getFile());
        SequencerView laidOut (engine);
        laidOut.setSize (1200, 700);
        laidOut.openFile (users.getFile());
        const auto leadShown = inView (laidOut, lead) && ! laidOut.getDocument().isDirty();
        laidOut.openFile (bass);
        check (leadShown && inView (laidOut, bass) && inView (fresh, bass) && laidOut.getDocument().get().name == "Acid Slide Rider",
               "presets: a sequence opened before the Sequencer is laid out, and one opened after another far above it, show their notes "
               "(the bass from " + juce::MidiMessage::getMidiNoteName (bassLow, true, true, 4) + ")");
    }

    // Dropped: on an instrument track the notes play on its instrument; on empty space a new track plays the preview voice.
    {
        juce::String error;
        const auto q = SequenceFile::load (presets.getChildFile ("Bass/Arp Bass/Acid Slide Rider.tqseq"), error);
        SessionState s;
        s.sampleRate = 48000.0;
        s.addTrack();
        SessionEdits::setTrackKind (s, s.tracks[0].id, true);
        const auto onTrack = q.has_value() ? SequenceFile::place (s, *q, 0, 0, true) : SequenceFile::Placed {};
        const auto tracksBefore = s.tracks.size();
        const auto onEmpty = q.has_value() ? SequenceFile::place (s, *q, -1, 0, false) : SequenceFile::Placed {};
        check (onTrack.clipId != 0 && onTrack.trackId == s.tracks[0].id && onTrack.effects == nullptr
                   && onEmpty.clipId != 0 && s.tracks.size() == tracksBefore + 1 && onEmpty.effects != nullptr
                   && onEmpty.effects->toString().contains (BuiltInEffects::basicSynthId),
               "presets: dropped on an instrument track a preset's notes play on that track's instrument; on empty space it makes a track playing its Torqwave Synth voice");
    }

    SequenceLibrary::setChosenFolder ({});
    SequenceLibrary::setFactoryFolder (before);
    base.deleteRecursively();
}
