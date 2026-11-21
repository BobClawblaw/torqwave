#include <JuceHeader.h>
#include "../Source/SequenceLibraryView.h"
#include "../Source/SequencePropertiesDialog.h"
#include "../Source/SequenceKit.h"
#include "TestUtils.h"
#include "LibraryFacetTests.h"

namespace
{
    using TestUtils::check;
    using Entry = SequenceLibrary::Entry;
    using Filters = SequenceLibrary::Filters;

    Entry entry (juce::StringArray path, const juce::String& name, const juce::String& style, const juce::String& key, int energy,
                 juce::StringArray character, double bars, double tempo, int rating, juce::int64 added, bool factory, bool favourite)
    {
        Entry e;
        e.path = path;
        e.name = name;
        e.style = style;
        e.key = key;
        e.energy = energy;
        e.character = character;
        e.bars = bars;
        e.tempo = tempo;
        e.rating = rating;
        e.added = added;
        e.factory = factory;
        e.favourite = favourite;
        e.file = juce::File ("/tmp/" + name + ".tqseq");
        return e;
    }

    /** A fixed set: two of the user's, two presets. */
    std::vector<Entry> fixedSet()
    {
        return { entry ({ "Bass" }, "Alpha", "Techno/Dark", "A minor", 4, { "Dark", "Dense" }, 1, 130, 3, 300, false, false),
                 entry ({ "Presets", "Bass" }, "Bravo", "Techno/Peak", "C minor", 5, { "Bright" }, 2, 128, 0, 200, true, true),
                 entry ({ "Drums", "Kick" }, "Charlie", "Industrial/EBM", "", 2, {}, 4, 120, 0, 100, false, false),
                 entry ({ "Presets", "Lead" }, "Delta", "", "A minor", 0, {}, 8, 140, 0, 0, true, false) };
    }

    juce::String passing (const std::vector<Entry>& set, const Filters& f, const juce::String& sessionKey = {})
    {
        juce::StringArray names;

        for (auto& e : set)
            if (SequenceLibrary::passes (e, f, sessionKey))
                names.add (e.name);

        return names.joinIntoString (",");
    }

    juce::String sorted (std::vector<Entry> set, SequenceLibrary::Sort by, int nearEnergy = 0)
    {
        for (auto& e : set)
            e.path = { "Same" };

        SequenceLibrary::sort (set, by, nearEnergy);
        juce::StringArray names;

        for (auto& e : set)
            names.add (e.name);

        return names.joinIntoString (",");
    }

    SequenceFile::Sequence melodic()
    {
        SequenceFile::Sequence q;
        q.name = "Line";
        q.role = "Bass";
        q.key = "A minor";
        q.length = 4 * 960;
        q.notes = { { 57, 0, 480, 0.8f }, { 60, 480, 480, 0.8f }, { 64, 960, 480, 0.8f } };
        return q;
    }

    std::vector<int> pitches (const SequenceFile::Sequence& q)
    {
        std::vector<int> p;

        for (auto& n : q.notes)
            p.push_back (n.pitch);

        return p;
    }
}

void runLibraryFacetTests()
{
    const auto set = fixedSet();

    // Each filter.
    {
        Filters style, genre, energy, bars, key, character, presets, mine, favourites, fits, role, search, both;
        style.style = "Techno/Dark";
        genre.style = "Techno";
        energy.energy = 5;
        bars.bars = 4;
        key.key = "Am";
        character.character = { "Dark" };
        presets.source = Filters::Source::presets;
        mine.source = Filters::Source::mine;
        favourites.favourites = true;
        fits.fitsSession = true;
        role.role = "Bass";
        search.search = "ebm";
        both.style = "Techno";
        both.source = Filters::Source::presets;

        check (passing (set, {}) == "Alpha,Bravo,Charlie,Delta" && passing (set, style) == "Alpha" && passing (set, genre) == "Alpha,Bravo"
                   && passing (set, energy) == "Bravo" && passing (set, bars) == "Charlie" && passing (set, key) == "Alpha,Delta"
                   && passing (set, character) == "Alpha" && passing (set, presets) == "Bravo,Delta" && passing (set, mine) == "Alpha,Charlie"
                   && passing (set, favourites) == "Bravo" && passing (set, fits, "A minor") == "Alpha,Charlie,Delta"
                   && passing (set, role) == "Alpha,Bravo" && passing (set, search) == "Charlie" && passing (set, both) == "Bravo",
               "Facets: each filter (style, a whole genre, energy, bars, key, character, presets or mine, favourites, the session's key, role, search) narrows to what it should, and two together to both");
    }

    // Each sort.
    {
        using S = SequenceLibrary::Sort;
        check (sorted (set, S::name) == "Alpha,Bravo,Charlie,Delta" && sorted (set, S::bars) == "Alpha,Bravo,Charlie,Delta"
                   && sorted (set, S::rating) == "Alpha,Bravo,Charlie,Delta" && sorted (set, S::newest) == "Alpha,Bravo,Charlie,Delta"
                   && sorted (set, S::tempo) == "Charlie,Bravo,Alpha,Delta" && sorted (set, S::key) == "Bravo,Alpha,Delta,Charlie"
                   && sorted (set, S::energy) == "Charlie,Alpha,Bravo,Delta" && sorted (set, S::name, 2) == "Charlie,Alpha,Bravo,Delta"
                   && SequenceLibrary::sortNames().size() == 7,
               "Facets: sorts by name, bars, rating, newest, tempo (slowest first), key (from C, none last) and energy (none last), and nearest an energy first: " + sorted (set, S::key));
    }

    // Each group.
    {
        using G = SequenceLibrary::Group;
        const auto path = [&set] (int i, G g) { return SequenceLibrary::groupPath (set[(size_t) i], g).joinIntoString (" | "); };
        const auto styleOf = SequenceFile::styleName ("Techno/Dark");

        check (path (0, G::role) == "Bass" && path (1, G::role) == "Presets | Bass"
                   && path (0, G::style) == styleOf + " | Bass" && path (2, G::style) == SequenceFile::styleName ("Industrial/EBM") + " | Drums"
                   && path (3, G::style) == "No Style | Lead" && path (1, G::key) == "C minor | Bass" && path (2, G::key) == "No Key | Drums"
                   && path (1, G::energy) == "Energy 5 | Bass" && path (3, G::energy) == "No Energy | Lead"
                   && styleOf == juce::String::fromUTF8 ("Techno \xe2\x80\xba Dark"),
               "Facets: grouped by role (its folders, Presets as given), style, key or energy, each then by role: " + path (0, G::style));
    }

    // Filters keep through XML; Similar.
    {
        Filters f;
        f.search = "acid";
        f.style = "Techno/Dark";
        f.key = "F# minor";
        f.role = "Bass";
        f.energy = 3;
        f.bars = 2;
        f.character = { "Dark", "Swung" };
        f.source = Filters::Source::mine;
        f.favourites = f.fitsSession = true;
        f.nearEnergy = 4;
        const auto back = Filters::fromXml (*f.toXml());
        const auto similar = SequenceLibrary::similarTo (set[1]);

        check (back == f && ! (back == Filters {}) && Filters {}.isEmpty() && ! f.isEmpty() && f.describe().contains ("Energy 3") && Filters {}.describe().isEmpty()
                   && similar.role == "Bass" && similar.style == "Techno/Peak" && similar.key == "C minor" && similar.nearEnergy == 5
                   && passing (set, similar) == "Bravo",
               "Facets: a filter set comes back from its XML, says what it narrows to, and Similar is the same role, style and key, nearest energy first");
    }

    const auto base = juce::File::createTempFile ("facets");
    const auto before = SequenceLibrary::searchesFile();
    const auto beforeFactory = SequenceLibrary::factoryFolder();
    SequenceLibrary::setSearchesFile (base.getChildFile ("Searches.xml"));
    SequenceLibrary::setFactoryFolder (base.getChildFile ("No Presets"));

    // Saved searches.
    {
        Filters dark, peak;
        dark.style = "Techno/Dark";
        peak.energy = 5;
        const auto e1 = SequenceLibrary::saveSearch ("Dark", dark);
        const auto e2 = SequenceLibrary::saveSearch ("Peak", peak);
        peak.energy = 4;
        const auto e3 = SequenceLibrary::saveSearch ("Peak", peak);
        const auto unnamed = SequenceLibrary::saveSearch ("  ", peak);
        auto saved = SequenceLibrary::savedSearches();
        const auto kept = saved.size() == 2 && saved[0].first == "Dark" && saved[0].second == dark && saved[1].first == "Peak" && saved[1].second.energy == 4;
        SequenceLibrary::removeSearch ("Dark");
        saved = SequenceLibrary::savedSearches();

        check (e1.isEmpty() && e2.isEmpty() && e3.isEmpty() && unnamed.isNotEmpty() && kept && saved.size() == 1 && saved[0].first == "Peak",
               "Facets: saved searches come back from their file in the order saved, one saved again under its name is replaced, and one removed goes");
        SequenceLibrary::removeSearch ("Peak");
    }

    // Older files: none of these, and written back as they were. Newer ones keep theirs.
    {
        auto q = melodic();
        const auto older = SequenceFile::toXml (q);
        const auto read = SequenceFile::fromXml (*older);
        auto withFacets = q;
        withFacets.style = "Industrial/EBM";
        withFacets.energy = 3;
        withFacets.character = { "Dark", "Distorted" };
        withFacets.note = "16ths between the kicks";
        const auto xml = SequenceFile::toXml (withFacets);
        const auto back = SequenceFile::fromXml (*xml);

        check (read.has_value() && read->style.isEmpty() && read->energy == 0 && read->character.isEmpty()
                   && SequenceFile::toXml (*read)->isEquivalentTo (older.get(), false) && ! older->hasAttribute ("style") && ! older->hasAttribute ("energy")
                   && ! older->hasAttribute ("character") && back.has_value() && back->style == "Industrial/EBM" && back->energy == 3
                   && back->character == juce::StringArray { "Dark", "Distorted" } && back->note == withFacets.note,
               "Facets: an older sequence reads with no style, energy or character and is written back the same; a newer one keeps all of them");
    }

    // Sequence Properties: written into the user's files (a kit: no energy or character), read by the scan, refused for presets.
    {
        const auto mine = base.getChildFile ("Mine");
        SequenceLibrary library (mine);
        library.makeFolders();
        auto q = melodic();
        const auto file = library.folderAt ({ "Bass" }).getChildFile ("Line.tqseq");
        SequenceFile::save (q, file);
        SequenceKit::Kit kit;
        kit.name = "Pair";
        kit.parts = { q };
        const auto kitFile = library.folderAt ({ "Kits" }).getChildFile ("Pair.tqkit");
        kitFile.getParentDirectory().createDirectory();
        SequenceKit::save (kit, kitFile);

        SequenceLibrary::Properties p;
        p.style = "Techno/Dark";
        p.key = "D minor";
        p.description = "Offbeat 16ths";
        p.energy = 4;
        p.character = { "Dark", "Sparse" };
        p.tags = { "Rolling", "Acid" };
        const auto e1 = SequenceLibrary::setProperties (file, p);
        const auto e2 = SequenceLibrary::setProperties (kitFile, p);
        const auto factoryFile = SequenceLibrary::factoryFolder().getChildFile ("Bass").getChildFile ("X.tqseq");
        factoryFile.getParentDirectory().createDirectory();
        SequenceFile::save (q, factoryFile);
        const auto refused = SequenceLibrary::setProperties (factoryFile, p);

        juce::String error;
        const auto loaded = SequenceFile::load (file, error);
        const auto loadedKit = SequenceKit::load (kitFile, error);
        const auto entries = library.scan();
        const auto found = std::find_if (entries.begin(), entries.end(), [&file] (auto& e) { return e.file == file; });
        const auto foundKit = std::find_if (entries.begin(), entries.end(), [&kitFile] (auto& e) { return e.file == kitFile; });

        check (e1.isEmpty() && e2.isEmpty() && refused.isNotEmpty() && loaded.has_value() && loaded->style == p.style && loaded->key == p.key
                   && loaded->note == p.description && loaded->energy == 4 && loaded->character == p.character && loaded->tags == p.tags
                   && pitches (*loaded) == pitches (q) && loadedKit.has_value() && loadedKit->style == p.style && loadedKit->note == p.description
                   && found != entries.end() && found->style == p.style && found->energy == 4 && found->description == p.description
                   && found->character == p.character && juce::exactlyEqual (found->tempo, 120.0)
                   && foundKit != entries.end() && foundKit->style == p.style && foundKit->energy == 0
                   && SequenceLibrary::propertiesOf (*found).description == p.description,
               "Facets: Sequence Properties writes style, key, description, energy, character and tags into a sequence (its notes as they were), style and description into a kit, the scan reads them, and a preset is refused");

        // The dialog: what is shown is what comes back; a style picked as shown is written as stored.
        SequencePropertiesDialog::Options options;
        options.styles = { "Techno/Dark", "Industrial/EBM" };
        SequencePropertiesDialog dialog (options, p);
        const auto same = dialog.result().properties;
        auto* styleBox = dynamic_cast<juce::ComboBox*> (dialog.find ("style"));
        auto* energyBox = dynamic_cast<juce::ComboBox*> (dialog.find ("energy"));
        auto* darkBox = dynamic_cast<juce::ComboBox*> (dialog.find ("Dark"));
        auto* description = dynamic_cast<juce::TextEditor*> (dialog.find ("description"));
        juce::String changedStyle;
        int changedEnergy = -1;
        juce::StringArray changedCharacter;
        juce::String changedDescription;

        if (styleBox != nullptr && energyBox != nullptr && darkBox != nullptr && description != nullptr)
        {
            styleBox->setText (SequenceFile::styleName ("Industrial/EBM"), juce::dontSendNotification);
            energyBox->setSelectedId (1, juce::dontSendNotification);
            darkBox->setSelectedId (3, juce::dontSendNotification);
            description->setText ("  Gated  ", false);
            const auto changed = dialog.result().properties;
            changedStyle = changed.style;
            changedEnergy = changed.energy;
            changedCharacter = changed.character;
            changedDescription = changed.description;
        }

        check (same.style == p.style && same.key == p.key && same.description == p.description && same.energy == p.energy && same.character == p.character
                   && same.tags == p.tags && changedStyle == "Industrial/EBM" && changedEnergy == 0 && changedCharacter == juce::StringArray { "Bright", "Sparse" }
                   && changedDescription == "Gated",
               "Facets: the Sequence Properties dialog gives back what it was opened with, and what is changed in it: " + changedStyle + " " + changedCharacter.joinIntoString (","));

        // The panel: grouped by style, narrowed by the Filters menu's choices, Similar, and saved searches shown above the folders.
        auto other = melodic();
        other.name = "Other";
        other.style = "Techno/Dark";
        other.key = "D minor";
        other.energy = 2;
        SequenceFile::save (other, library.folderAt ({ "Bass" }).getChildFile ("Other.tqseq"));
        auto pad = melodic();
        pad.name = "Wash";
        pad.role = "Pads";
        pad.style = "Ambient/Drone";
        pad.energy = 1;
        SequenceFile::save (pad, library.folderAt ({ "Pads" }).getChildFile ("Wash.tqseq"));

        SequenceLibraryView view (mine);
        view.setGroup (SequenceLibrary::Group::style);
        const auto folders = view.shownFolders();
        Filters dark;
        dark.style = "Techno/Dark";
        dark.energy = 4;
        view.setFilters (dark);
        const auto narrowed = view.shownFiles();
        view.showSimilar (library.folderAt ({ "Bass" }).getChildFile ("Other.tqseq"));
        const auto similar = view.shownFiles();
        view.setFilters ({});
        view.setGroup (SequenceLibrary::Group::role);
        const auto all = view.shownFiles().size();
        SequenceLibrary::saveSearch ("Ambient", [] { Filters f; f.style = "Ambient"; return f; }());
        view.refresh();
        const auto searches = view.shownSearches();
        view.applySearch ("Ambient");
        const auto ambient = view.shownFiles();
        const auto applied = view.getFilters();
        SequenceLibrary::removeSearch ("Ambient");

        check (folders.contains (SequenceFile::styleName ("Techno/Dark")) && folders.contains (SequenceFile::styleName ("Techno/Dark") + " > Bass")
                   && folders.contains (SequenceFile::styleName ("Ambient/Drone") + " > Pads") && ! folders.contains ("Presets")
                   && narrowed.size() == 1 && narrowed[0] == file
                   && similar.size() == 2 && similar[0].getFileName() == "Other.tqseq" && similar[1] == file
                   && all == 5 && searches == juce::StringArray { "Ambient" } && ambient.size() == 1 && ambient[0].getFileName() == "Wash.tqseq"
                   && applied.style == "Ambient",
               "Facets: the panel groups by style then role, narrows by the Filters menu's choices, shows Similar nearest energy first, and a saved search above the folders shows its sequences: "
                   + folders.joinIntoString (", "));
    }

    // Transposing a drop: every note by the nearer interval between the keys, the key renamed; drums and keyless ones as they are.
    {
        const auto q = melodic();
        const auto up = SequenceFile::transposedTo (q, "C minor");
        const auto down = SequenceFile::transposedTo (q, "E minor");
        const auto same = SequenceFile::transposedTo (q, "A minor");
        const auto noSession = SequenceFile::transposedTo (q, "");
        auto drums = q;
        drums.channel = 10;
        auto drumRole = q;
        drumRole.role = "Drums";
        auto keyless = q;
        keyless.key = {};
        auto phrygian = q;
        phrygian.key = "A phrygian";
        const auto movedPhrygian = SequenceFile::transposedTo (phrygian, "D minor");
        SequenceKit::Kit kit;
        kit.parts = { q, drums };

        for (auto& part : kit.parts)
            part = SequenceFile::transposedTo (part, "C minor");

        check (pitches (up) == std::vector<int> { 60, 63, 67 } && up.key == "C minor" && pitches (down) == std::vector<int> { 52, 55, 59 } && down.key == "E minor"
                   && pitches (same) == pitches (q) && pitches (noSession) == pitches (q) && noSession.key == "A minor"
                   && pitches (SequenceFile::transposedTo (drums, "C minor")) == pitches (q) && pitches (SequenceFile::transposedTo (drumRole, "C minor")) == pitches (q)
                   && pitches (SequenceFile::transposedTo (keyless, "C minor")) == pitches (q)
                   && pitches (movedPhrygian) == std::vector<int> { 62, 65, 69 } && movedPhrygian.key == "D phrygian"
                   && pitches (kit.parts[0]) == std::vector<int> { 60, 63, 67 } && pitches (kit.parts[1]) == pitches (q)
                   && ! SequenceLibrary::transposesDrops(),
               "Facets: a drop moved into the session's key moves every note the nearer way and renames the key in its own scale; drums, keyless ones and a session with no key are left alone; off by default: "
                   + up.key + ", " + down.key + ", " + movedPhrygian.key);
    }

    SequenceLibrary::setSearchesFile (before);
    SequenceLibrary::setFactoryFolder (beforeFactory);
    base.deleteRecursively();
}
