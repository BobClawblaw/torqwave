#include <JuceHeader.h>
#include "../Source/BatteryKit.h"
#include "../Source/DrumKitImport.h"
#include "TestUtils.h"
#include "BatteryKitTests.h"

namespace
{
    using TestUtils::check;

    /** A kit of made-up cells: plain ones, layers split by velocity, a crossfade, a stack, cells
        that share a voice group, names outside ASCII and a path this cannot read. */
    BatteryKit::Kit madeUpKit()
    {
        BatteryKit::Kit kit;
        kit.programVersion = 0xa3;
        kit.voiceGroups = { { "Hats", 1, -1 }, { {}, 1, 0 }, { {}, 1, 0 } };

        const auto path = [&kit] (int up, const juce::StringArray& parts)
        {
            kit.paths.push_back ({ up, parts });
            return (int) kit.paths.size() - 1;
        };

        const auto layer = [] (int file, int low, int high)
        {
            BatteryKit::Layer l;
            l.path = file;
            l.lowVelocity = low;
            l.highVelocity = high;
            l.frames = 44100;
            return l;
        };

        const auto cell = [&kit] (int index, const juce::String& name, int key)
        {
            BatteryKit::Cell c;
            c.index = index;
            c.name = name;
            c.lowKey = c.highKey = key;
            c.colour = 16 + index % 16;
            kit.cells.push_back (c);
            return &kit.cells.back();
        };

        kit.cells.reserve (16);

        auto* low = cell (0, "Low Drum", 36);
        low->layers = { layer (path (1, { "Sounds", "Drums", "low drum.wav" }), 1, 127) };
        low->layers[0].lowKey = low->layers[0].highKey = 36;
        low->gain = 0.5f;
        low->pan = -0.25f;
        low->tune = 1.5f;
        low->envelopeRead = low->envelopeOn = true;
        low->attack = 2.5f;
        low->hold = 8.0f;
        low->decay = 1250.0f;

        auto* split = cell (1, "Split", 38);
        split->layers = { layer (path (1, { "Sounds", "soft.wav" }), 1, 63), layer (path (1, { "Sounds", "hard.wav" }), 64, 127) };
        split->layers[1].gain = 0.8f;
        split->layers[1].tune = 1.25f;
        split->layers[1].start = 120;
        split->layers[1].endBack = 4000;
        split->envelopeRead = true;      // there, and switched off
        split->attack = 1.0f;
        split->decay = 750.0f;

        auto* faded = cell (2, "Faded", 40);
        faded->layers = { layer (0, 1, 80), layer (1, 50, 127) };
        faded->layers[0].fadeHigh = 30;
        faded->layers[1].fadeLow = 30;

        auto* stack = cell (5, juce::String::fromUTF8 ("St\xc3\xa4pel \xf0\x9f\xa5\x81"), 41);
        stack->layers = { layer (0, 1, 127), layer (1, 1, 127), layer (2, 1, 127) };
        stack->reverse = true;

        auto* closed = cell (17, "Closed", 42);
        closed->layers = { layer (path (3, { "Elsewhere", "closed.wav" }), 1, 127) };
        closed->voiceGroup = 0;

        auto* open = cell (18, "Open", 46);
        open->layers = { layer (path (0, { "open.wav" }), 1, 127) };
        open->voiceGroup = 0;

        auto* wide = cell (40, "Wide", 60);
        wide->highKey = 72;
        wide->keyTracking = true;
        wide->layers = { layer (0, 1, 127) };
        wide->layers[0].lowKey = 60;
        wide->layers[0].highKey = 72;
        wide->layers[0].rootKey = 64;
        wide->voiceGroup = 2;

        auto* unreadable = cell (127, "No Path", 50);
        unreadable->layers = { layer (path (0, {}), 1, 127) };

        cell (64, "Named and Empty", 51);
        return kit;
    }

    bool same (const BatteryKit::Layer& a, const BatteryKit::Layer& b)
    {
        return a.path == b.path && a.lowVelocity == b.lowVelocity && a.highVelocity == b.highVelocity && a.fadeLow == b.fadeLow && a.fadeHigh == b.fadeHigh
            && a.lowKey == b.lowKey && a.highKey == b.highKey && a.rootKey == b.rootKey && juce::exactlyEqual (a.gain, b.gain) && juce::exactlyEqual (a.pan, b.pan)
            && juce::exactlyEqual (a.tune, b.tune) && a.start == b.start && a.endBack == b.endBack && a.frames == b.frames;
    }

    bool same (BatteryKit::Kit a, BatteryKit::Kit b)
    {
        const auto byPlace = [] (const BatteryKit::Cell& x, const BatteryKit::Cell& y) { return x.index < y.index; };
        std::sort (a.cells.begin(), a.cells.end(), byPlace);
        std::sort (b.cells.begin(), b.cells.end(), byPlace);

        if (a.cells.size() != b.cells.size() || a.paths != b.paths || a.voiceGroups.size() != b.voiceGroups.size() || a.voiceGroupsRead != b.voiceGroupsRead)
            return false;

        for (size_t i = 0; i < a.voiceGroups.size(); ++i)
            if (a.voiceGroups[i].name != b.voiceGroups[i].name || a.voiceGroups[i].voices != b.voiceGroups[i].voices || a.voiceGroups[i].exclusiveGroup != b.voiceGroups[i].exclusiveGroup)
                return false;

        for (size_t i = 0; i < a.cells.size(); ++i)
        {
            const auto& x = a.cells[i];
            const auto& y = b.cells[i];

            if (x.index != y.index || x.name != y.name || ! juce::exactlyEqual (x.gain, y.gain) || ! juce::exactlyEqual (x.pan, y.pan) || ! juce::exactlyEqual (x.tune, y.tune)
                || x.keyTracking != y.keyTracking || x.reverse != y.reverse || x.voiceGroup != y.voiceGroup || x.colour != y.colour || x.lowKey != y.lowKey
                || x.highKey != y.highKey || x.layers.size() != y.layers.size() || x.envelopeRead != y.envelopeRead || x.envelopeOn != y.envelopeOn
                || ! juce::exactlyEqual (x.attack, y.attack) || ! juce::exactlyEqual (x.hold, y.hold) || ! juce::exactlyEqual (x.decay, y.decay))
                return false;

            for (size_t j = 0; j < x.layers.size(); ++j)
                if (! same (x.layers[j], y.layers[j]))
                    return false;
        }

        return true;
    }

    juce::String readBack (const juce::MemoryBlock& file, BatteryKit::Kit& kit)
    {
        return BatteryKit::read (file.getData(), file.getSize(), kit);
    }

    void put32 (juce::MemoryBlock& block, size_t at, juce::uint32 v)
    {
        for (int i = 0; i < 4; ++i)
            static_cast<juce::uint8*> (block.getData())[at + (size_t) i] = (juce::uint8) (v >> (8 * i));
    }

    void unpacking()
    {
        // Text, long runs, a pattern that comes round after 300 bytes and one after 9,000: literal
        // runs, short and long back-references, near and far ones.
        juce::MemoryBlock plain;
        juce::Random random (11);

        for (int i = 0; i < 300; ++i)
            plain.append ("\x01", 1), static_cast<char*> (plain.getData())[plain.getSize() - 1] = (char) random.nextInt (256);

        plain.append (plain.getData(), 300);
        plain.setSize (plain.getSize() + 2000, true);

        for (int i = 0; i < 6400; ++i)
        {
            const auto byte = (char) random.nextInt (256);
            plain.append (&byte, 1);
        }

        plain.append (plain.getData(), 600);
        plain.append ("abcabcabc", 9);

        for (auto runs : { true, false })
        {
            const auto packed = BatteryKit::pack (plain.getData(), plain.getSize(), runs);
            juce::MemoryBlock back;
            const auto ok = BatteryKit::unpack (packed.getData(), packed.getSize(), back, plain.getSize());
            check (ok && back == plain, juce::String ("Bytes packed ") + (runs ? "with back-references" : "as they are") + " unpack to what they were ("
                                            + juce::String ((int) plain.getSize()) + " bytes from " + juce::String ((int) packed.getSize()) + ")");

            if (runs)
            {
                check (packed.getSize() < plain.getSize() - 2500, "The packer found the runs and the repeats, near and far");

                juce::MemoryBlock other;
                check (! BatteryKit::unpack (packed.getData(), packed.getSize(), other, plain.getSize() - 1)
                           && ! BatteryKit::unpack (packed.getData(), packed.getSize(), other, plain.getSize() + 1),
                       "A stream that makes more or fewer bytes than it should is refused");

                int refused = 0, cuts = 0;

                for (size_t cut = 0; cut < packed.getSize(); cut += 7, ++cuts)
                    refused += BatteryKit::unpack (packed.getData(), cut, other, plain.getSize()) ? 0 : 1;

                check (refused == cuts, "A stream cut short anywhere is refused (" + juce::String (cuts) + " cuts)");
            }
        }

        // A reference to before the start, by hand: three bytes, then nine from 300 back.
        const juce::uint8 before[] { 0x22, 1, 2, 3, 0xe1, 0, 0x2b };
        juce::MemoryBlock out;
        check (! BatteryKit::unpack (before, sizeof (before), out, 12), "A back-reference to before the start is refused");

        const juce::uint8 overlapping[] { 0x20, 7, 0xe0, 4, 0 };
        check (BatteryKit::unpack (overlapping, sizeof (overlapping), out, 14) && std::all_of ((const juce::uint8*) out.begin(), (const juce::uint8*) out.end(), [] (juce::uint8 b) { return b == 7; }),
               "A back-reference that overlaps what it writes repeats the byte");

        check (! BatteryKit::unpack (overlapping, sizeof (overlapping), out, BatteryKit::mostUnpackedBytes + 1), "More than the most a kit unpacks to is refused before any is made");
    }

    void roundTrips()
    {
        const auto kit = madeUpKit();

        struct Layout
        {
            const char* name;
            BatteryKit::WriteOptions options;
        };

        BatteryKit::WriteOptions packedOld, newer, cellFile, small, newest;
        newest.programVersion = 0xb2;
        newest.zoneVersion = 0x9a;
        newest.pathsVersion = 3;
        newer.programVersion = 0xab;
        newer.zoneVersion = 0x9a;
        cellFile.packed = false;
        small.numCells = 128;
        small.programVersion = 0xaf;

        for (auto& layout : { Layout { "a packed kit", packedOld }, Layout { "a newer kit, its zones with more in them", newer },
                              Layout { "a file whose block is not packed, as a cell file's", cellFile }, Layout { "the newest program seen in a library", small },
                              Layout { "a kit as Battery saves one now: the path table that starts with the kit file", newest } })
        {
            BatteryKit::Kit back;
            const auto file = BatteryKit::write (kit, layout.options);
            const auto error = readBack (file, back);
            check (error.isEmpty() && same (kit, back) && back.programVersion == layout.options.programVersion,
                   juce::String ("Read back as written: ") + layout.name + (error.isNotEmpty() ? " (" + error + ")" : juce::String()));
        }

        BatteryKit::Kit back;
        readBack (BatteryKit::write (kit), back);
        check (back.cells.size() == 9 && back.cells[3].name == kit.cells[3].name && back.cells[3].name.length() == 8, "A name outside ASCII, a pair of units included, is read whole");
    }

    void versions()
    {
        const auto kit = madeUpKit();
        BatteryKit::Kit back;

        const auto refusal = [&] (BatteryKit::WriteOptions options) { return readBack (BatteryKit::write (kit, options), back); };

        BatteryKit::WriteOptions o;
        o.programVersion = 0xb3;
        check (refusal (o).contains ("program record is version 179") && back.cells.empty(), "A program record newer than any seen is refused by name, and nothing comes over");
        o.programVersion = 0xa2;
        check (refusal (o).contains ("program record is version 162"), "An older one too");

        o = {};
        o.cellVersion = 0x96;
        check (refusal (o).contains ("cell record is version 150"), "An unknown cell record is refused by name");

        o = {};
        o.zoneVersion = 0x99;
        check (refusal (o).contains ("zone record is version 153"), "An unknown zone record is refused by name");

        o = {};
        o.pathsVersion = 4;
        check (refusal (o).contains ("sample path record is version 4"), "An unknown path table is refused by name");

        o = {};
        o.voiceGroupVersion = 0x61;
        const auto error = refusal (o);
        check (error.isEmpty() && ! back.voiceGroupsRead && back.voiceGroups.empty() && back.cells.size() == 9
                   && std::all_of (back.cells.begin(), back.cells.end(), [] (const BatteryKit::Cell& c) { return c.voiceGroup == -1; }),
               "Unknown voice groups: the cells still come over, without them, and the kit says so");

        const char text[] = "Not a kit at all, only some text that is long enough to be looked at as one.";
        check (BatteryKit::read (text, sizeof (text), back).contains ("not a Battery 4 kit"), "A file that is something else is called that");
        check (BatteryKit::read (nullptr, 0, back).contains ("not a Battery 4 kit"), "So is no file at all");
    }

    void paths()
    {
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("torqwave-kit-paths-" + juce::String (juce::Random::getSystemRandom().nextInt()));
        const auto kits = folder.getChildFile ("Library").getChildFile ("Kits");
        const auto sample = folder.getChildFile ("Library").getChildFile ("Sounds").getChildFile ("Drums").getChildFile ("low drum.wav");
        kits.createDirectory();
        sample.create();

        const BatteryKit::Path path { 1, { "Sounds", "Drums", "low drum.wav" } };
        check (BatteryKit::resolve (path, kits) == sample, "A path is followed from the kit's folder: up, then down by name");
        check (BatteryKit::resolve ({ 0, {} }, kits) == juce::File(), "A path with no parts points nowhere");

        for (auto* bad : { "..", ".", "", "a/b", "a\\b" })
            check (BatteryKit::resolve ({ 0, { "Sounds", bad, "x.wav" } }, kits) == juce::File() && BatteryKit::resolveUnder ({ 0, { bad, "x.wav" } }, folder) == juce::File(),
                   "A path with the part \"" + juce::String (bad) + "\" in it points nowhere");

        const auto moved = folder.getChildFile ("Moved");
        moved.getChildFile ("Drums").createDirectory();
        const auto movedSample = moved.getChildFile ("Drums").getChildFile ("low drum.wav");
        movedSample.create();
        check (BatteryKit::resolveUnder (path, moved) == movedSample, "Under another folder, the longest end of the path that is there is the sample");
        check (BatteryKit::resolveUnder (path, moved.getChildFile ("Drums")) == movedSample, "Down to its name alone");
        check (BatteryKit::resolveUnder (path, kits) == juce::File(), "And nothing where it is not");

        folder.deleteRecursively();
    }

    /** Reads a file that may be anything, and says whether what came back is fit to use. */
    bool sound (const juce::MemoryBlock& file, bool& refused)
    {
        BatteryKit::Kit kit;
        const auto error = readBack (file, kit);
        refused = error.isNotEmpty();

        if (refused)
            return kit.cells.empty() && kit.paths.empty();

        for (auto& c : kit.cells)
        {
            if (! juce::isPositiveAndBelow (c.index, BatteryKit::mostCells) || c.voiceGroup < -1 || c.voiceGroup >= (int) kit.voiceGroups.size()
                || ! std::isfinite (c.gain) || ! std::isfinite (c.pan) || ! std::isfinite (c.tune) || c.tune <= 0.0f)
                return false;

            for (auto& l : c.layers)
                if (l.path < -1 || l.path >= (int) kit.paths.size() || l.lowVelocity < 1 || l.highVelocity > 127 || l.lowVelocity > l.highVelocity
                    || l.lowKey < 0 || l.highKey > 127 || l.lowKey > l.highKey || l.start < 0 || l.endBack < 0 || l.frames < 0 || l.start + l.endBack > l.frames
                    || ! std::isfinite (l.gain) || ! std::isfinite (l.tune) || l.tune <= 0.0f)
                    return false;
        }

        return true;
    }

    void hostile()
    {
        const auto kit = madeUpKit();
        BatteryKit::WriteOptions plainOptions;
        plainOptions.packed = false;

        // Cut short at every length (a packed kit by steps: each try unpacks what is left), the
        // packed one with the newer path table.
        for (auto packed : { false, true })
        {
            auto options = plainOptions;
            options.packed = packed;
            options.pathsVersion = packed ? 3 : 2;
            const auto whole = BatteryKit::write (kit, options);
            int tried = 0, bad = 0, taken = 0;

            for (size_t length = 0; length < whole.getSize(); length += packed ? 1 : 97)
            {
                bool refused = false;
                bad += sound (juce::MemoryBlock (whole.getData(), length), refused) ? 0 : 1;
                taken += refused ? 0 : 1;
                ++tried;
            }

            check (bad == 0 && taken == 0, juce::String (packed ? "A packed kit" : "A kit not packed") + " cut short at any of " + juce::String (tried) + " lengths is refused, and nothing comes over");
        }

        // Bytes changed at random: refused, or read into something every part of which is in range.
        for (auto packed : { false, true })
        {
            auto options = plainOptions;
            options.packed = packed;
            const auto whole = BatteryKit::write (kit, options);
            juce::Random random (packed ? 5 : 6);
            int bad = 0, refusals = 0;
            constexpr int tries = 4000;

            for (int i = 0; i < tries; ++i)
            {
                auto file = whole;
                auto* bytes = static_cast<juce::uint8*> (file.getData());

                for (int changes = 1 + random.nextInt (4); changes > 0; --changes)
                {
                    const auto at = (size_t) random.nextInt ((int) file.getSize());

                    switch (random.nextInt (4))
                    {
                        case 0:   bytes[at] = (juce::uint8) random.nextInt (256); break;
                        case 1:   bytes[at] = 0xff; break;
                        case 2:   bytes[at] ^= (juce::uint8) (1 << random.nextInt (8)); break;
                        default:  if (at + 4 <= file.getSize()) put32 (file, at, random.nextBool() ? 0xffffffffu : 0x7fffffffu); break;
                    }
                }

                bool refused = false;
                bad += sound (file, refused) ? 0 : 1;
                refusals += refused ? 1 : 0;
            }

            check (bad == 0, juce::String (packed ? "A packed kit" : "A kit not packed") + " with bytes changed at random, " + juce::String (tries) + " times: refused ("
                                 + juce::String (refusals) + ") or read with every value in range");
        }

        // Counts that promise more than is there.
        {
            BatteryKit::Kit one;
            one.cells.push_back ({});
            one.cells[0].name = "One";
            BatteryKit::WriteOptions o = plainOptions;
            o.numCells = 1;
            auto file = BatteryKit::write (one, o);
            const auto* bytes = static_cast<const juce::uint8*> (file.getData());
            int counts = 0, bad = 0;

            // Every four bytes that read as a small number are made a huge one in turn.
            for (size_t at = 0; at + 4 <= file.getSize(); ++at)
            {
                if (bytes[at] > 2 || bytes[at + 1] != 0 || bytes[at + 2] != 0 || bytes[at + 3] != 0)
                    continue;

                for (auto huge : { 0xffffffffu, 0x40000000u, 0x00100000u })
                {
                    auto changed = file;
                    put32 (changed, at, huge);
                    bool refused = false;
                    bad += sound (changed, refused) ? 0 : 1;
                    ++counts;
                }
            }

            check (bad == 0, "A count or a length made huge, each of " + juce::String (counts) + " in turn, is refused or read in range, and takes no memory to match");
        }

        // More cells, zones or path parts than a kit may have.
        {
            BatteryKit::Kit many;
            BatteryKit::WriteOptions o = plainOptions;
            o.numCells = BatteryKit::mostCells;
            BatteryKit::Kit back;
            check (readBack (BatteryKit::write (many, o), back).isEmpty(), "The most cells a kit may have are read");

            BatteryKit::Kit deep;
            BatteryKit::Path p;

            for (int i = 0; i < BatteryKit::mostPathParts + 1; ++i)
                p.parts.add ("x");

            deep.paths.push_back (p);
            check (readBack (BatteryKit::write (deep, plainOptions), back).contains ("damaged"), "A path of more parts than any has is refused");

            const juce::MemoryBlock big (BatteryKit::mostFileBytes + 1, true);
            check (readBack (big, back).contains ("larger than any Battery kit"), "A file larger than any kit is refused before it is looked at");
        }
    }

    //==============================================================================
    // A kit onto the pads.

    /** A folder with a library in it: Kits (where the kit is said to be) and Sounds, with a short
        sound file for every name given. Deleted when it goes. */
    struct Library
    {
        Library()
        {
            folder = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("torqwave-kit-import-" + juce::String (juce::Random::getSystemRandom().nextInt()));
            kit = folder.getChildFile ("Library").getChildFile ("Kits").getChildFile ("Made Up.nbkt");
            kit.getParentDirectory().createDirectory();
        }

        ~Library()   { folder.deleteRecursively(); }

        void add (const juce::String& name, const juce::File& into = {})
        {
            const auto file = (into != juce::File() ? into : folder.getChildFile ("Library").getChildFile ("Sounds")).getChildFile (name);
            file.getParentDirectory().createDirectory();
            juce::AudioBuffer<float> audio (1, 4800);

            for (int i = 0; i < audio.getNumSamples(); ++i)
                audio.setSample (0, i, 0.5f * (float) std::sin (0.05 * i) * (1.0f - (float) i / 4800.0f));

            std::unique_ptr<juce::OutputStream> stream (new juce::FileOutputStream (file));

            if (auto writer = juce::WavAudioFormat().createWriterFor (stream, juce::AudioFormatWriterOptions{}.withSampleRate (48000.0).withNumChannels (1).withBitsPerSample (16)))
                writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
        }

        juce::File folder, kit;
    };

    struct KitMaker
    {
        BatteryKit::Kit kit;

        int path (const juce::String& name)
        {
            kit.paths.push_back ({ 1, { "Sounds", name } });
            return (int) kit.paths.size() - 1;
        }

        BatteryKit::Cell& cell (const juce::String& name, int key)
        {
            BatteryKit::Cell c;
            c.index = (int) kit.cells.size();
            c.name = name;
            c.lowKey = c.highKey = key;
            kit.cells.push_back (c);
            return kit.cells.back();
        }

        static BatteryKit::Layer layer (int file, int low, int high, int key)
        {
            BatteryKit::Layer l;
            l.path = file;
            l.lowVelocity = low;
            l.highVelocity = high;
            l.lowKey = l.highKey = l.rootKey = key;
            l.frames = 4800;
            return l;
        }
    };

    const DrumKitImport::Pad* padNamed (const DrumKitImport::Plan& plan, const juce::String& name)
    {
        for (auto& p : plan.pads)
            if (p.name == name)
                return &p;

        return nullptr;
    }

    bool reports (const DrumKitImport::Plan& plan, const juce::String& text)
    {
        for (auto& line : plan.report)
            if (line.contains (text))
                return true;

        return false;
    }

    bool near (float a, float b, float within = 0.01f)   { return std::abs (a - b) <= within; }

    void planning()
    {
        Library library;

        for (auto* name : { "a.wav", "b.wav", "c.wav" })
            library.add (name);

        KitMaker m;
        m.kit.voiceGroups = { { "Hats", 1, -1 } };
        const auto a = m.path ("a.wav"), b = m.path ("b.wav"), c = m.path ("c.wav"), gone = m.path ("gone.wav"), loop = m.path ("loop.rx2");
        m.kit.paths.push_back ({ 0, {} });
        const auto unnamed = (int) m.kit.paths.size() - 1;

        auto& plainCell = m.cell ("Plain", 36);
        plainCell.layers = { KitMaker::layer (a, 1, 127, 36) };
        plainCell.gain = 0.5f;
        plainCell.pan = -0.25f;
        plainCell.tune = 1.5f;
        plainCell.reverse = true;
        plainCell.layers[0].start = 1200;
        plainCell.layers[0].endBack = 100;
        plainCell.envelopeRead = plainCell.envelopeOn = true;
        plainCell.attack = 2.5f;
        plainCell.hold = 8.0f;
        plainCell.decay = 9000.0f;

        auto& splitCell = m.cell ("Split", 38);
        splitCell.layers = { KitMaker::layer (b, 64, 127, 38), KitMaker::layer (a, 1, 63, 38) };
        splitCell.envelopeRead = true;
        splitCell.decay = 100.0f;

        auto& fade = m.cell ("Fade", 40);
        fade.layers = { KitMaker::layer (a, 1, 80, 40), KitMaker::layer (b, 50, 127, 40) };
        fade.layers[0].fadeHigh = 30;
        fade.layers[1].fadeLow = 30;

        auto& whole = m.cell ("Fade Whole", 41);
        whole.layers = { KitMaker::layer (b, 1, 127, 41), KitMaker::layer (a, 1, 127, 41) };
        whole.layers[0].fadeLow = 127;
        whole.layers[1].fadeHigh = 127;

        m.cell ("Stack", 43).layers = { KitMaker::layer (a, 1, 127, 43), KitMaker::layer (b, 1, 127, 43), KitMaker::layer (c, 1, 127, 43) };
        m.cell ("Edge", 45).layers = { KitMaker::layer (a, 1, 60, 45), KitMaker::layer (b, 60, 127, 45) };

        auto& wide = m.cell ("Wide", 60);
        wide.keyTracking = true;
        wide.layers = { KitMaker::layer (a, 1, 127, 60) };
        wide.layers[0].highKey = 72;
        wide.layers[0].rootKey = 64;

        auto& own = m.cell ("Own", 47);
        own.layers = { KitMaker::layer (a, 1, 63, 47), KitMaker::layer (b, 64, 127, 47) };
        own.layers[1].gain = 0.5f;

        auto& closed = m.cell ("Closed", 42);
        closed.layers = { KitMaker::layer (a, 1, 127, 42) };
        closed.voiceGroup = 0;
        auto& open = m.cell ("Open", 46);
        open.layers = { KitMaker::layer (b, 1, 127, 46) };
        open.voiceGroup = 0;

        m.cell ("Partly Gone", 48).layers = { KitMaker::layer (a, 1, 63, 48), KitMaker::layer (gone, 64, 127, 48) };
        m.cell ("Loop", 49).layers = { KitMaker::layer (loop, 1, 127, 49) };
        m.cell ("Unnamed", 50).layers = { KitMaker::layer (unnamed, 1, 127, 50) };
        m.cell ("", 51).layers = { KitMaker::layer (c, 1, 127, 51) };
        m.cell ("No Layers", 52);

        auto& many = m.cell ("Many", 53);

        for (int i = 0; i < 20; ++i)
            many.layers.push_back (KitMaker::layer (i % 2 == 0 ? a : b, 1 + i * 6, i == 19 ? 127 : 6 + i * 6, 53));

        // Through the file and back, so the plan is of what the reader gives.
        BatteryKit::Kit kit;
        const auto file = BatteryKit::write (m.kit);
        check (BatteryKit::read (file.getData(), file.getSize(), kit).isEmpty(), "The made-up kit is written and read");
        auto plan = DrumKitImport::fromBattery (kit, library.kit);

        check (plan.name == "Made Up" && plan.pads.size() == 13, "The plan is named after the kit file and has a pad for each cell with a sample it can play ("
                                                                    + juce::String ((int) plan.pads.size()) + ")");

        if (const auto* p = padNamed (plan, "Plain"))
            check (p->note == 36 && near (p->level, -6.02f) && near (p->pan, -25.0f) && near (p->tune, 7.02f) && p->reverse && near (p->start, 25.0f) && p->layers.size() == 1
                       && p->layers[0].file == library.folder.getChildFile ("Library/Sounds/a.wav"),
                   "A cell's note, level (half is -6 dB), pan, tune (a ratio of 1.5 is 7.02 semitones), reverse and start come over, its sample found from the kit's folder");
        else
            check (false, "Plain has a pad");

        const auto ranges = [&plan] (const juce::String& name)
        {
            juce::String s;

            if (const auto* p = padNamed (plan, name))
                for (auto& l : p->layers)
                    s << l.file.getFileNameWithoutExtension() << " " << l.lowVelocity << "-" << l.highVelocity << " ";

            return s.trim();
        };

        check (ranges ("Split") == "a 1-63 b 64-127", "Layers split by velocity keep their ranges, softest first");
        check (ranges ("Fade") == "a 1-65 b 66-127" && ranges ("Fade Whole") == "a 1-64 b 65-127" && reports (plan, "2 cells fade"),
               "A crossfade is split at its middle, one over the whole range too, and the report counts them");
        check (ranges ("Stack") == "a 1-127 b 1-127 c 1-127" && reports (plan, "1 cell has several samples on one range"), "A stack keeps its layers on one range, to take turns, and is reported");
        check (ranges ("Edge") == "a 1-60 b 61-127", "Ranges that share one velocity give it to the softer");

        if (const auto* p = padNamed (plan, "Wide"))
            check (p->note == 60 && near (p->tune, -4.0f) && reports (plan, "1 cell plays over several keys"), "A cell over several keys is on its lowest, as far below its root as that is");
        else
            check (false, "Wide has a pad");

        check (reports (plan, "1 cell has layers with a level, tune or pan of their own") && near (padNamed (plan, "Own")->level, 0.0f), "A layer's own level is not kept, and that is said");
        check (reports (plan, "2 cells cut others off"), "Cells that share a one-voice group are reported as chokes not brought over");
        check (reports (plan, "1 sample ends before its file does"), "An end marker is reported");
        check (plan.missing.size() == 1 && plan.missing[0].endsWith ("gone.wav") && ranges ("Partly Gone") == "a 1-63 gone 64-127", "A sample that is not there is listed as missing, its layer still planned");
        check (padNamed (plan, "Loop") == nullptr && reports (plan, "1 REX loop was left out"), "A REX loop is left out and reported");
        check (padNamed (plan, "Unnamed") == nullptr && reports (plan, "1 sample is named in a way this does not read") && reports (plan, "2 cells were left with no sample"),
               "A sample with no path is left out, and the cells left empty are counted");
        check (padNamed (plan, "c") != nullptr, "A cell with no name takes its sample's");
        check (padNamed (plan, "No Layers") == nullptr, "A cell with a name and no layers has no pad");

        if (const auto* p = padNamed (plan, "Many"))
        {
            bool covers = p->layers.size() == 16 && p->layers.front().lowVelocity == 1 && p->layers.back().highVelocity == 127;

            for (size_t i = 1; covers && i < p->layers.size(); ++i)
                covers = p->layers[i].lowVelocity == p->layers[i - 1].highVelocity + 1;

            check (covers && reports (plan, "1 cell has more than 16 layers"), "Twenty layers become sixteen that still cover every velocity, and that is said");
        }
        else
        {
            check (false, "Many has a pad");
        }

        check (reports (plan, "effects, pitch envelopes, modulation"), "The report always says what an import never brings");
        check (padNamed (plan, "Plain")->envelope && near (padNamed (plan, "Plain")->hold, 8.0f) && ! padNamed (plan, "Split")->envelope && reports (plan, "1 cell has a volume envelope on"),
               "A cell's volume envelope is planned when it is on, not when it is there and off, and the report counts it");

        // The library moved: its samples are found under the folder given.
        const auto moved = library.folder.getChildFile ("Moved Library");
        library.add ("gone.wav", moved.getChildFile ("Sounds"));
        const auto again = DrumKitImport::fromBattery (kit, library.kit, moved);
        check (again.missing.isEmpty() && padNamed (again, "Partly Gone")->layers[1].file == moved.getChildFile ("Sounds").getChildFile ("gone.wav")
                   && padNamed (again, "Plain")->layers[0].file == library.folder.getChildFile ("Library/Sounds/a.wav"),
               "With the library's new folder given, a missing sample is found under it, and the ones in place stay");

        // Places.
        DrumKitImport::place (plan);
        const auto placeOf = [&plan] (const juce::String& name) { const auto* p = padNamed (plan, name); return p != nullptr ? p->pad : -99; };
        check (placeOf ("Plain") == (int) DrumMachine::Sound::kick && placeOf ("Split") == (int) DrumMachine::Sound::snare && placeOf ("Closed") == (int) DrumMachine::Sound::closedHat
                   && placeOf ("Open") == (int) DrumMachine::Sound::openHat,
               "A cell on a note that is a sound's own takes that sound's pad");
        check (placeOf ("Fade") == DrumMachine::numSounds && placeOf ("Stack") == DrumMachine::numSounds + 1, "One on another note takes the next user pad");

        // On the pads.
        BuiltInEffects::BasicDrumsProcessor drums;
        const auto error = DrumKitImport::apply (drums, plan);
        const auto kick = (int) DrumMachine::Sound::kick;
        check (error.isEmpty() && drums.getKitName() == "Made Up", "The plan is put on the pads, and the kit takes its name");
        check (drums.playsSamples (kick) && drums.getPadName (kick) == "Plain" && drums.getLayers (kick).size() == 1 && drums.getLayers (kick)[0].sample != nullptr
                   && near (drums.parameterOf (kick, "level")->get(), -6.02f) && near (drums.parameterOf (kick, "pan")->get(), -25.0f) && near (drums.parameterOf (kick, "tune")->get(), 7.02f)
                   && near (drums.parameterOf (kick, "start")->get(), 25.0f) && drums.reverseParameter (kick)->get() && near (drums.parameterOf (kick, "decay")->get(), 5000.0f),
               "The kick's pad plays the cell's sample with its level, pan, tune, start and reverse, its decay long enough to play whole");

        const auto kickEffects = drums.getEffects (kick), snareEffects = drums.getEffects ((int) DrumMachine::Sound::snare);
        check (kickEffects.on (DrumMachine::envOn) && near (kickEffects[DrumMachine::envAttack], 2.5f) && near (kickEffects[DrumMachine::envHold], 8.0f)
                   && near (kickEffects[DrumMachine::envDecay], 5000.0f) && ! snareEffects.on (DrumMachine::envOn),
               "The envelope is on the pad's ENV page: attack and hold as in the kit, a decay past the knob's top held at it; a pad whose cell had it off has it off");

        const auto fadePad = placeOf ("Fade");
        check (drums.getPadName (fadePad) == "Fade" && drums.getPadNote (fadePad) == 40 && drums.getLayers (fadePad).size() == 2 && drums.getLayers (fadePad)[1].lowVelocity == 66,
               "A user pad has the cell's name, note and layers");
        check (drums.getLayers (placeOf ("Partly Gone")).size() == 1 && reports (plan, "1 sample is not where the kit says"), "A layer whose sample is not there is left out, and the report says so");
        check (! drums.playsSamples ((int) DrumMachine::Sound::clap) && near (drums.parameterOf (DrumMachine::Sound::clap, "decay")->get(), DrumMachine::defaultsFor (DrumMachine::Sound::clap).decay),
               "A sound no cell took is the Standard kit's");

        const auto hit = DrumMachine::Engine::renderPad (drums.currentSettings(), fadePad, 48000.0, 0.2, drums.getSamples());
        check (hit.getMagnitude (0, hit.getNumSamples()) > 0.01f, "An imported pad sounds");

        // Nothing readable: nothing changes.
        KitMaker none;
        none.cell ("Nowhere", 36).layers = { KitMaker::layer (none.path ("nowhere.wav"), 1, 127, 36) };
        auto nonePlan = DrumKitImport::fromBattery (none.kit, library.kit);
        DrumKitImport::place (nonePlan);
        check (DrumKitImport::apply (drums, nonePlan).contains ("nothing was changed") && drums.getKitName() == "Made Up" && drums.getPadName (kick) == "Plain",
               "A kit none of whose samples are there changes nothing, and says so");
    }

    void placing()
    {
        DrumKitImport::Plan plan;

        // Two on the kick's note, then seventy on notes no sound's pad has.
        for (int i = 0; i < 72; ++i)
        {
            DrumKitImport::Pad p;
            p.name = "Cell " + juce::String (i + 1);
            p.note = i < 2 ? 36 : 90;
            p.layers.push_back ({});
            plan.pads.push_back (p);
        }

        DrumKitImport::place (plan);
        std::set<int> places;

        for (auto& p : plan.pads)
            places.insert (p.pad);

        check (plan.pads.size() == 64 && places.size() == 64 && *places.begin() == 0 && *places.rbegin() == 63, "Seventy-two cells: sixty-four are placed, each on a pad of its own");
        check (plan.pads[0].pad == (int) DrumMachine::Sound::kick && plan.pads[1].pad == DrumMachine::numSounds, "The second cell on a sound's note takes a user pad, on that note");
        check (reports (plan, "29 cells are on a pad whose note is fixed") && reports (plan, "8 were left out"), "The cells that changed note and the ones left out are counted in the report");
    }

    void folders()
    {
        Library library;
        const auto folder = library.folder.getChildFile ("Loose Sounds");

        for (auto* name : { "Kick.wav", "Kick Sub.wav", "Snare v1.wav", "Snare v2.wav", "Snare v3.wav", "ClosedHH rr1.wav", "ClosedHH rr2.wav", "OpenHat.wav", "hi tom.wav",
                            "BD9.wav", "Thud D7.wav", "Whatever.wav" })
            library.add (name, folder);

        library.add ("36 thump.wav", folder.getChildFile ("By Note"));
        library.add ("crack 039.wav", folder.getChildFile ("By Note"));
        library.add ("01 one.wav", folder.getChildFile ("Counted"));
        library.add ("40 forty.wav", folder.getChildFile ("Counted"));
        folder.getChildFile ("notes.txt").replaceWithText ("not a sound");

        auto plan = DrumKitImport::fromFolder (folder);
        const auto noteOf = [&plan] (const juce::String& name) { const auto* p = padNamed (plan, name); return p != nullptr ? p->note : -99; };

        const auto ranges = [&plan] (const juce::String& name)
        {
            juce::String s;

            if (const auto* p = padNamed (plan, name))
                for (auto& l : p->layers)
                    s << l.file.getFileNameWithoutExtension() << " " << l.lowVelocity << "-" << l.highVelocity << ", ";

            return s.trim();
        };

        check (plan.name == "Loose Sounds" && plan.pads.size() == 13, "A folder's sound files, the ones in its folders too, become pads, files alike but for a mark one pad ("
                                                                         + juce::String ((int) plan.pads.size()) + ")");
        check (noteOf ("Kick") == 36 && noteOf ("OpenHat") == 46 && noteOf ("hi tom") == 48 && noteOf ("ClosedHH") == 42 && noteOf ("Snare") == 38,
               "An instrument's name, or its short form, puts a file on that sound's note");
        check (noteOf ("Whatever") == 82 && noteOf ("Kick Sub") == 83 && noteOf ("BD9") == 84, "A file with no name this knows gets a note of its own, from 82 up, and so does another file of an instrument whose pad the plainest name has taken");
        check (noteOf ("01 one") == 85 && noteOf ("40 forty") == 86, "Where a folder's files are numbered from below 35, the numbers are a count, not notes");
        check (noteOf ("36 thump") == 36 && noteOf ("crack 039") == 39, "A drum note's number at the front or the end of a name is its note");
        check (noteOf ("Thud D7") == 98, "A note's name is its note, middle C being C4");
        check (ranges ("Snare") == "Snare v1 1-42, Snare v2 43-84, Snare v3 85-127,", "Files marked v1, v2, v3 are one pad's layers, the velocities split between them");
        check (ranges ("ClosedHH") == "ClosedHH rr1 1-127, ClosedHH rr2 1-127,", "Files marked rr1, rr2 are layers that take turns");
        check (reports (plan, "13 pads from 16 sound files: 2 by a drum note's number in the name, 5 by an instrument's name, 1 by a note's name, and 5 on notes of their own")
                   && reports (plan, "2 pads have several layers"),
               "The report counts how each pad was chosen");

        DrumKitImport::place (plan);
        BuiltInEffects::BasicDrumsProcessor drums;
        const auto error = DrumKitImport::apply (drums, plan);
        const auto snare = (int) DrumMachine::Sound::snare, kick = (int) DrumMachine::Sound::kick;
        check (error.isEmpty() && drums.getKitName() == "Loose Sounds" && drums.playsSamples (kick) && drums.getLayers (kick).size() == 1 && drums.getLayers (snare).size() == 3
                   && drums.getLayers (snare)[1].lowVelocity == 43 && drums.getPadName (kick) == "Kick",
               "The folder's plan is put on the pads: the kick's and the snare's play the files");

        const auto* thump = padNamed (plan, "36 thump");
        check (thump != nullptr && thump->pad >= DrumMachine::numSounds && drums.getPadNote (thump->pad) == 36, "A file numbered for a note a pad already has takes a user pad on that note");

        const auto empty = library.folder.getChildFile ("Nothing Here");
        empty.createDirectory();
        check (DrumKitImport::fromFolder (empty).pads.empty(), "A folder with no sound files gives no pads");
    }
}

void runBatteryKitTests()
{
    std::cout << "\n--- Battery kit files ---" << std::endl;
    unpacking();
    roundTrips();
    versions();
    paths();
    hostile();
    planning();
    placing();
    folders();
}

int runBatteryKitFiles (const juce::StringArray& files)
{
    int read = 0, refused = 0, cells = 0, layers = 0, found = 0, pads = 0, envelopes = 0, envelopesOn = 0;
    std::map<juce::String, int> errors, versions;

    for (auto& name : files)
    {
        const juce::File file (name);
        BatteryKit::Kit kit;

        if (const auto error = BatteryKit::read (file, kit); error.isNotEmpty())
        {
            ++refused;
            ++errors[error];
            continue;
        }

        ++read;
        ++versions["program " + juce::String::toHexString (kit.programVersion)];
        auto plan = DrumKitImport::fromBattery (kit, file);
        DrumKitImport::place (plan);
        pads += (int) plan.pads.size();

        for (auto& c : kit.cells)
        {
            cells += c.layers.empty() ? 0 : 1;
            envelopes += ! c.layers.empty() && c.envelopeRead ? 1 : 0;
            envelopesOn += ! c.layers.empty() && c.envelopeRead && c.envelopeOn ? 1 : 0;

            for (auto& l : c.layers)
            {
                ++layers;
                found += l.path >= 0 && BatteryKit::resolve (kit.paths[(size_t) l.path], file.getParentDirectory()).existsAsFile() ? 1 : 0;
            }
        }
    }

    std::cout << files.size() << " files: " << read << " read, " << refused << " refused; " << cells << " cells with samples, " << layers << " layers, "
              << found << " samples found; " << pads << " pads planned; " << envelopes << " volume envelopes read, " << envelopesOn << " of them on" << std::endl;

    for (auto& [what, n] : versions)
        std::cout << "  " << what << ": " << n << std::endl;

    for (auto& [what, n] : errors)
        std::cout << "  refused (" << n << "): " << what << std::endl;

    return refused == 0 ? 0 : 1;
}
