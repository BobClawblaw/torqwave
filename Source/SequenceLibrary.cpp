#include "SequenceLibrary.h"
#include "SequenceMidi.h"
#include "NoteTools.h"
#include "FactoryPresetData.h"

namespace
{
    juce::File& chosenFolder()
    {
        static juce::File chosen;
        return chosen;
    }

    juce::File& chosenSearchesFile()
    {
        static juce::File chosen;
        return chosen;
    }

    bool& transposeSetting()
    {
        static bool on = false;
        return on;
    }

    juce::File& chosenFactoryFolder()
    {
        static juce::File chosen;
        return chosen;
    }

    /** A factory preset's place in factoryRatingsFile: its path below the presets' folder. */
    juce::String factoryKey (const juce::File& file)
    {
        return file.getRelativePathFrom (SequenceLibrary::factoryFolder()).replaceCharacter ('\\', '/');
    }

    std::unique_ptr<juce::XmlElement> factoryRatings()
    {
        auto xml = juce::XmlDocument::parse (SequenceLibrary::factoryRatingsFile());
        return xml != nullptr && xml->hasTagName ("TORQWAVE_PRESET_RATINGS") ? std::move (xml) : std::make_unique<juce::XmlElement> ("TORQWAVE_PRESET_RATINGS");
    }

    /** Changes a factory preset's line in factoryRatingsFile (dropping it when it says nothing). */
    juce::String rewriteFactory (const juce::File& file, const std::function<void (juce::XmlElement&)>& change)
    {
        if (! file.existsAsFile())
            return file.getFileName() + " is not there.";

        auto xml = factoryRatings();
        const auto key = factoryKey (file);
        auto* line = xml->getChildByAttribute ("path", key);

        if (line == nullptr)
        {
            line = xml->createNewChildElement ("PRESET");
            line->setAttribute ("path", key);
        }

        change (*line);

        if (! line->hasAttribute ("rating") && ! line->hasAttribute ("favourite"))
            xml->removeChildElement (line, true);

        SequenceLibrary::factoryRatingsFile().getParentDirectory().createDirectory();
        return xml->writeTo (SequenceLibrary::factoryRatingsFile()) ? juce::String() : juce::String ("The presets' ratings could not be saved.");
    }

    bool isLibraryXml (const juce::XmlElement& xml)
    {
        return xml.hasTagName ("TORQWAVE_SEQUENCE") || xml.hasTagName ("TORQWAVE_KIT");
    }

    /** A file saved before sequences kept the time they were added keeps its file's creation
        time from now on, so rewriting it doesn't make it the newest. */
    void keepAdded (juce::XmlElement& xml, const juce::File& file)
    {
        if (xml.getStringAttribute ("added").getLargeIntValue() <= 0)
            xml.setAttribute ("added", juce::String (file.getCreationTime().toMilliseconds()));
    }

    /** Reads a sequence or kit, changes its XML, and writes it to `target` (its own file when none). */
    juce::String rewrite (const juce::File& file, const std::function<void (juce::XmlElement&)>& change, const juce::File& target = {})
    {
        const auto xml = juce::XmlDocument::parse (file);

        if (xml == nullptr || ! isLibraryXml (*xml))
            return file.getFileName() + " is not a sequence or a kit.";

        keepAdded (*xml, file);
        change (*xml);
        return SequenceFile::writeXml (*xml, target != juce::File() ? target : file);
    }
}

const std::vector<SequenceLibrary::Role>& SequenceLibrary::roles()
{
    static const std::vector<Role> all {
        { "Drums",   { "Full Grooves", "Kick", "Snare & Clap", "Hats & Tops", "Percussion", "Fills", "Breaks" } },
        { "Bass",    { "Sub", "Synth Bass", "Arp Bass", "Picked & Played" } },
        { "Harmony", { "Progressions", "Keys & Comping", "Stabs", "Strums" } },
        { "Pads",    { "Sustained", "Evolving", "Swells", "Drones" } },
        { "Arps",    { "Up & Down", "16th Sequences", "Plucks", "Gated" } },
        { "Melody",  { "Leads", "Hooks", "Counter-lines", "Bells & Mallets" } },
        { "SFX",     { "Risers", "Downlifters", "Impacts", "Sweeps", "Transitions", "Hits" } },
        { "Kits",    {} },
        { "Inbox",   {} },
    };

    return all;
}

juce::File SequenceLibrary::defaultFolder()
{
    return chosenFolder() != juce::File() ? chosenFolder() : standardFolder();
}

juce::File SequenceLibrary::standardFolder()
{
    return juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Torqwave").getChildFile ("Sequences");
}

void SequenceLibrary::setChosenFolder (const juce::File& folder)
{
    chosenFolder() = folder;
}

juce::File SequenceLibrary::factoryFolder()
{
    if (chosenFactoryFolder() != juce::File())
        return chosenFactoryFolder();

    // One folder for each set of presets shipped, named for its contents.
    static const auto set = []
    {
        juce::uint64 h = 14695981039346656037ull;   // FNV-1a over the zip

        for (int i = 0; i < FactoryPresetData::factorypresets_zipSize; ++i)
            h = (h ^ (juce::uint8) FactoryPresetData::factorypresets_zip[i]) * 1099511628211ull;

        return juce::String::toHexString ((juce::int64) h);
    }();
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Torqwave").getChildFile ("Factory Presets").getChildFile (set);
}

void SequenceLibrary::setFactoryFolder (const juce::File& f)
{
    chosenFactoryFolder() = f;
}

bool SequenceLibrary::isFactory (const juce::File& f)
{
    return f.isAChildOf (factoryFolder());
}

juce::File SequenceLibrary::factoryRatingsFile()
{
    return factoryFolder().getSiblingFile ("Ratings.xml");
}

juce::String SequenceLibrary::unpackPresets (const void* zip, size_t size, const juce::File& into)
{
    const auto done = into.getChildFile (".complete");

    if (done.existsAsFile())
        return {};

    // Another set beside it is the one an earlier version shipped.
    for (const auto& other : into.getParentDirectory().findChildFiles (juce::File::findDirectories, false))
        if (other != into)
            other.deleteRecursively();

    into.deleteRecursively();

    if (! into.createDirectory())
        return "The presets' folder could not be made: " + into.getFullPathName();

    juce::MemoryInputStream stream (zip, size, false);
    juce::ZipFile archive (stream);

    if (archive.getNumEntries() == 0 && size > 0)
        return "The presets the app ships could not be read.";

    if (const auto result = archive.uncompressTo (into, true); result.failed())
        return "The presets could not be unpacked: " + result.getErrorMessage();

    for (const auto& found : juce::RangedDirectoryIterator (into, true, "*", juce::File::findFiles))
        found.getFile().setReadOnly (true);

    return done.create() ? juce::String() : juce::String ("The presets' folder could not be written.");
}

juce::String SequenceLibrary::unpackFactoryPresets()
{
    return unpackPresets (FactoryPresetData::factorypresets_zip, (size_t) FactoryPresetData::factorypresets_zipSize, factoryFolder());
}

SequenceLibrary::SequenceLibrary (juce::File f) : folder (std::move (f)) {}

bool SequenceLibrary::makeFolders() const
{
    if (! folder.createDirectory())
        return false;

    for (auto& role : roles())
    {
        folder.getChildFile (role.name).createDirectory();

        for (auto& sub : role.subRoles)
            folder.getChildFile (role.name).getChildFile (sub).createDirectory();
    }

    return true;
}

juce::File SequenceLibrary::folderFor (const juce::String& role, const juce::String& subRole) const
{
    for (auto& r : roles())
        if (r.name == role)
            return subRole.isNotEmpty() ? folder.getChildFile (r.name).getChildFile (juce::File::createLegalFileName (subRole))
                                        : folder.getChildFile (r.name);

    return folder.getChildFile ("Inbox");
}

juce::File SequenceLibrary::newFileFor (const juce::String& role, const juce::String& subRole, const juce::String& name) const
{
    return newFileIn (folderFor (role, subRole), name);
}

juce::File SequenceLibrary::newFileIn (const juce::File& dir, const juce::String& name, const char* extension)
{
    const auto base = juce::File::createLegalFileName (name.trim().isNotEmpty() ? name.trim() : juce::String ("Sequence"));
    auto file = dir.getChildFile (base + extension);

    for (int n = 2; file.exists(); ++n)
        file = dir.getChildFile (base + " " + juce::String (n) + extension);

    return file;
}

std::vector<juce::StringArray> SequenceLibrary::folders() const
{
    std::vector<juce::StringArray> fixed, own;

    for (auto& role : roles())
    {
        fixed.push_back ({ role.name });

        for (auto& sub : role.subRoles)
            fixed.push_back ({ role.name, sub });
    }

    for (const auto& found : juce::RangedDirectoryIterator (folder, true, "*", juce::File::findDirectories))
    {
        juce::StringArray path;
        path.addTokens (found.getFile().getRelativePathFrom (folder), juce::File::getSeparatorString(), {});
        path.removeString (".");
        path.removeEmptyStrings();

        const auto underRole = ! path.isEmpty() && std::any_of (roles().begin(), roles().end(), [&] (auto& r) { return r.name == path[0]; });

        if (underRole && ! isFixed (path))
            own.push_back (path);
    }

    // Parents before children, by name: sorted as joined paths, with the separator lowest.
    std::sort (own.begin(), own.end(), [] (const juce::StringArray& a, const juce::StringArray& b)
    {
        for (int i = 0; i < juce::jmin (a.size(), b.size()); ++i)
            if (a[i] != b[i])
                return a[i].compareNatural (b[i]) < 0;

        return a.size() < b.size();
    });

    fixed.insert (fixed.end(), own.begin(), own.end());
    return fixed;
}

juce::StringArray SequenceLibrary::folderNames() const
{
    juce::StringArray names { "Inbox" };

    for (auto& path : folders())
        if (path[0] != "Inbox" || path.size() > 1)
            names.add (nameOf (path));

    // A folder of the user's goes after its role's fixed ones; keep each under its parent.
    juce::StringArray ordered;

    std::function<void (const juce::String&)> addWithChildren = [&] (const juce::String& n)
    {
        ordered.add (n);

        for (auto& child : names)
            if (! ordered.contains (child) && pathOf (child).size() == pathOf (n).size() + 1 && child.startsWith (n + " > "))
                addWithChildren (child);
    };

    for (auto& n : names)
        if (pathOf (n).size() == 1)
            addWithChildren (n);

    return ordered;
}

juce::StringArray SequenceLibrary::pathOf (const juce::String& folderName)
{
    juce::StringArray path;
    path.addTokens (folderName, ">", {});
    path.trim();
    path.removeEmptyStrings();
    return path;
}

juce::String SequenceLibrary::nameOf (const juce::StringArray& path)
{
    return path.joinIntoString (" > ");
}

juce::File SequenceLibrary::folderAt (const juce::StringArray& path) const
{
    if (path.isEmpty() || ! std::any_of (roles().begin(), roles().end(), [&] (auto& r) { return r.name == path[0]; }))
        return folder.getChildFile ("Inbox");

    auto dir = folder.getChildFile (path[0]);

    for (int i = 1; i < path.size(); ++i)
        dir = dir.getChildFile (juce::File::createLegalFileName (path[i]));

    return dir;
}

bool SequenceLibrary::isFixed (const juce::StringArray& path)
{
    for (auto& role : roles())
        if (path.size() >= 1 && role.name == path[0])
            return path.size() == 1 || (path.size() == 2 && role.subRoles.contains (path[1]));

    return false;
}

juce::File SequenceLibrary::moveInto (const juce::File& sequence, const juce::StringArray& path, bool copy, juce::String& error) const
{
    const auto dir = folderAt (path);

    if (sequence.getParentDirectory() == dir && ! copy)
        return sequence;

    const auto isKit = sequence.hasFileExtension (SequenceFile::kitExtension);
    const auto xml = juce::XmlDocument::parse (sequence);

    if (xml == nullptr || ! xml->hasTagName (isKit ? "TORQWAVE_KIT" : "TORQWAVE_SEQUENCE"))
    {
        error = sequence.getFileName() + (isKit ? " is not a kit." : " is not a sequence.");
        return {};
    }

    if (! dir.isDirectory() && ! dir.createDirectory())
    {
        error = "The folder " + dir.getFileName() + " could not be made.";
        return {};
    }

    const auto target = newFileIn (dir, sequence.getFileNameWithoutExtension(), isKit ? SequenceFile::kitExtension : SequenceFile::extension);

    error = rewrite (sequence, [&path, isKit] (juce::XmlElement& x)
    {
        if (isKit)
            return;

        SequenceFile::Sequence q;
        setRoleFrom (q, path);
        x.setAttribute ("role", q.role);
        x.setAttribute ("subRole", q.subRole);
    }, target);

    if (error.isNotEmpty())
        return {};

    if (! copy && ! sequence.deleteFile())
    {
        target.deleteFile();
        error = sequence.getFileName() + " could not be moved.";
        return {};
    }

    return target;
}

void SequenceLibrary::setRoleFrom (SequenceFile::Sequence& q, const juce::StringArray& path)
{
    const auto known = ! path.isEmpty() && std::any_of (roles().begin(), roles().end(), [&] (auto& r) { return r.name == path[0]; });
    q.role = known ? path[0] : juce::String ("Inbox");
    q.subRole = known && path.size() > 1 ? path[1] : juce::String();
}

juce::String SequenceLibrary::makeFolder (const juce::StringArray& parent, const juce::String& name) const
{
    const auto legal = juce::File::createLegalFileName (name.trim());

    if (legal.isEmpty())
        return "A folder needs a name.";

    const auto dir = folderAt (parent).getChildFile (legal);

    if (dir.exists())
        return "There is already a folder called " + legal + " there.";

    return dir.createDirectory() ? juce::String() : "The folder " + legal + " could not be made.";
}

std::vector<SequenceLibrary::Entry> SequenceLibrary::scan() const
{
    std::vector<Entry> entries;
    const auto wildcard = juce::String ("*") + SequenceFile::extension + ";*" + SequenceFile::kitExtension;
    const auto factory = folder == factoryFolder() || isFactory (folder);
    const auto ratings = factory ? factoryRatings() : nullptr;

    for (const auto& found : juce::RangedDirectoryIterator (folder, true, wildcard, juce::File::findFiles))
    {
        const auto file = found.getFile();
        const auto xml = juce::XmlDocument::parse (file);

        if (xml == nullptr)
            continue;

        Entry e;
        e.file = file;

        if (const auto q = SequenceFile::fromXml (*xml))
        {
            e.name = q->name;
            e.key = q->key;
            e.tags = q->tags;
            e.bars = q->bars();
            e.hasInstrument = SequenceFile::instrumentOf (*q).isNotEmpty();
            e.rating = q->rating;
            e.favourite = q->favourite;
            e.added = q->added;
            e.style = q->style;
            e.description = q->note;
            e.energy = q->energy;
            e.character = q->character;
            e.tempo = q->writtenTempo;
        }
        else if (const auto kit = SequenceKit::fromXml (*xml))
        {
            e.isKit = true;
            e.name = kit->name;
            e.key = kit->key;
            e.tags = kit->tags;
            e.bars = kit->bars();
            e.parts = (int) kit->parts.size();
            e.hasInstrument = std::any_of (kit->parts.begin(), kit->parts.end(), [] (auto& p) { return SequenceFile::instrumentOf (p).isNotEmpty(); });
            e.rating = kit->rating;
            e.favourite = kit->favourite;
            e.added = kit->added;
            e.style = kit->style;
            e.description = kit->note;
            e.tempo = kit->parts.empty() ? 0.0 : kit->parts.front().writtenTempo;
        }
        else
        {
            continue;
        }

        if (e.name.isEmpty())
            e.name = file.getFileNameWithoutExtension();

        if (e.added <= 0)
            e.added = file.getCreationTime().toMilliseconds();

        if (factory)
        {
            e.factory = true;

            if (auto* line = ratings->getChildByAttribute ("path", factoryKey (file)))
            {
                e.rating = juce::jlimit (0, 5, line->getIntAttribute ("rating"));
                e.favourite = line->getBoolAttribute ("favourite");
            }
        }

        e.path.addTokens (file.getParentDirectory().getRelativePathFrom (folder), juce::File::getSeparatorString(), {});
        e.path.removeString (".");
        e.path.removeEmptyStrings();
        entries.push_back (std::move (e));
    }

    sort (entries, Sort::name);
    return entries;
}

bool SequenceLibrary::matches (const Entry& e, const juce::String& search)
{
    const auto text = (e.name + " " + e.key + " " + e.tags.joinIntoString (" ") + " " + e.path.joinIntoString (" ") + " "
                       + e.style.replaceCharacter ('/', ' ') + " " + e.description).toLowerCase();
    juce::StringArray words;
    words.addTokens (search.toLowerCase(), " ", "\"");
    words.removeEmptyStrings();

    for (auto& w : words)
        if (! text.contains (w))
            return false;

    return true;
}

juce::StringArray SequenceLibrary::sortNames()
{
    return { "Name", "Bars", "Rating", "Newest", "Tempo", "Key", "Energy" };
}

void SequenceLibrary::sort (std::vector<Entry>& entries, Sort by, int nearEnergy)
{
    // A key's place: its root from C, then its scale; none after every key.
    const auto keyOrder = [] (const Entry& e)
    {
        const auto k = NoteTools::parseKey (e.key);
        return k.has_value() ? k->root * 100 + NoteTools::modes().indexOf (k->mode) : 100000;
    };

    std::stable_sort (entries.begin(), entries.end(), [by, nearEnergy, keyOrder] (const Entry& a, const Entry& b)
    {
        if (const auto pa = a.path.joinIntoString ("/"), pb = b.path.joinIntoString ("/"); pa != pb)
            return pa.compareNatural (pb) < 0;

        if (nearEnergy > 0)
        {
            const auto da = a.energy > 0 ? std::abs (a.energy - nearEnergy) : 10, db = b.energy > 0 ? std::abs (b.energy - nearEnergy) : 10;

            if (da != db)
                return da < db;
        }

        switch (by)
        {
            case Sort::bars:   if (! juce::exactlyEqual (a.bars, b.bars)) return a.bars < b.bars; break;
            case Sort::rating: if (a.rating != b.rating) return a.rating > b.rating; break;
            case Sort::newest: if (a.added != b.added) return a.added > b.added; break;
            case Sort::tempo:  if (! juce::exactlyEqual (a.tempo, b.tempo)) return a.tempo < b.tempo; break;
            case Sort::key:    if (keyOrder (a) != keyOrder (b)) return keyOrder (a) < keyOrder (b); break;
            case Sort::energy: if (a.energy != b.energy) return (a.energy > 0 ? a.energy : 10) < (b.energy > 0 ? b.energy : 10); break;
            case Sort::name:   break;
        }

        return a.name.compareNatural (b.name) < 0;
    });
}

juce::StringArray SequenceLibrary::groupNames()
{
    return { "Role", "Style", "Key", "Energy" };
}

juce::StringArray SequenceLibrary::groupPath (const Entry& e, Group by)
{
    if (by == Group::role)
        return e.path;

    // Under Presets the role is the second folder.
    const auto role = e.path[0] == factoryName ? e.path[1] : e.path[0];
    juce::String group;

    switch (by)
    {
        case Group::style:  group = e.style.isNotEmpty() ? SequenceFile::styleName (e.style) : juce::String ("No Style"); break;
        case Group::key:    group = NoteTools::parseKey (e.key).has_value() ? NoteTools::parseKey (e.key)->name() : juce::String ("No Key"); break;
        case Group::energy: group = e.energy > 0 ? "Energy " + juce::String (e.energy) : juce::String ("No Energy"); break;
        case Group::role:   break;
    }

    return { group, role };
}

bool SequenceLibrary::Filters::isEmpty() const
{
    return *this == Filters {};
}

bool SequenceLibrary::Filters::operator== (const Filters& o) const
{
    return search == o.search && style == o.style && key == o.key && role == o.role && energy == o.energy && bars == o.bars
        && character == o.character && source == o.source && favourites == o.favourites && fitsSession == o.fitsSession && nearEnergy == o.nearEnergy;
}

std::unique_ptr<juce::XmlElement> SequenceLibrary::Filters::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement> ("FILTERS");
    xml->setAttribute ("search", search);
    xml->setAttribute ("style", style);
    xml->setAttribute ("key", key);
    xml->setAttribute ("role", role);
    xml->setAttribute ("energy", energy);
    xml->setAttribute ("bars", bars);
    xml->setAttribute ("character", character.joinIntoString (","));
    xml->setAttribute ("source", (int) source);
    xml->setAttribute ("favourites", favourites);
    xml->setAttribute ("fitsSession", fitsSession);
    xml->setAttribute ("nearEnergy", nearEnergy);
    return xml;
}

SequenceLibrary::Filters SequenceLibrary::Filters::fromXml (const juce::XmlElement& xml)
{
    Filters f;
    f.search = xml.getStringAttribute ("search");
    f.style = xml.getStringAttribute ("style");
    f.key = xml.getStringAttribute ("key");
    f.role = xml.getStringAttribute ("role");
    f.energy = juce::jlimit (0, 5, xml.getIntAttribute ("energy"));
    f.bars = juce::jlimit (0, 64, xml.getIntAttribute ("bars"));
    f.character.addTokens (xml.getStringAttribute ("character"), ",", {});
    f.character.removeEmptyStrings();
    f.source = (Source) juce::jlimit (0, 2, xml.getIntAttribute ("source"));
    f.favourites = xml.getBoolAttribute ("favourites");
    f.fitsSession = xml.getBoolAttribute ("fitsSession");
    f.nearEnergy = juce::jlimit (0, 5, xml.getIntAttribute ("nearEnergy"));
    return f;
}

juce::String SequenceLibrary::Filters::describe() const
{
    juce::StringArray parts;

    if (search.isNotEmpty())
        parts.add ("\"" + search + "\"");

    if (role.isNotEmpty())
        parts.add (role);

    if (style.isNotEmpty())
        parts.add (SequenceFile::styleName (style));

    if (key.isNotEmpty())
        parts.add (key);

    if (energy > 0)
        parts.add ("Energy " + juce::String (energy));

    if (nearEnergy > 0)
        parts.add ("nearest energy " + juce::String (nearEnergy));

    if (bars > 0)
        parts.add (juce::String (bars) + (bars == 1 ? " bar" : " bars"));

    parts.addArray (character);

    if (source != Source::both)
        parts.add (source == Source::presets ? "Presets" : "Mine");

    if (favourites)
        parts.add ("Favourites");

    if (fitsSession)
        parts.add ("Fits the Session");

    return parts.joinIntoString (juce::String::fromUTF8 (" \xc2\xb7 "));
}

bool SequenceLibrary::passes (const Entry& e, const Filters& f, const juce::String& sessionKey)
{
    const auto role = e.path[0] == factoryName ? e.path[1] : e.path[0];

    if (f.search.trim().isNotEmpty() && ! matches (e, f.search.trim()))
        return false;

    if (f.role.isNotEmpty() && role != f.role)
        return false;

    // A style, or a whole genre.
    if (f.style.isNotEmpty() && e.style != f.style && ! (f.style.indexOfChar ('/') < 0 && SequenceFile::splitStyle (e.style).first == f.style))
        return false;

    if (f.key.isNotEmpty() && (! NoteTools::parseKey (e.key).has_value() || NoteTools::parseKey (e.key) != NoteTools::parseKey (f.key)))
        return false;

    if (f.energy > 0 && e.energy != f.energy)
        return false;

    if (f.bars > 0 && ! juce::exactlyEqual (e.bars, (double) f.bars))
        return false;

    for (auto& c : f.character)
        if (! e.character.contains (c))
            return false;

    if ((f.source == Filters::Source::presets && ! e.factory) || (f.source == Filters::Source::mine && e.factory))
        return false;

    if (f.favourites && ! e.favourite)
        return false;

    return ! f.fitsSession || fitsKey (e, sessionKey);
}

juce::StringArray SequenceLibrary::stylesIn (const std::vector<Entry>& entries)
{
    juce::StringArray styles;

    for (auto& e : entries)
        if (e.style.isNotEmpty())
            styles.addIfNotAlreadyThere (e.style);

    styles.sortNatural();
    return styles;
}

juce::StringArray SequenceLibrary::keysIn (const std::vector<Entry>& entries)
{
    std::vector<NoteTools::Scale> keys;

    for (auto& e : entries)
        if (const auto k = NoteTools::parseKey (e.key); k.has_value() && std::find (keys.begin(), keys.end(), *k) == keys.end())
            keys.push_back (*k);

    std::sort (keys.begin(), keys.end(), [] (auto& a, auto& b)
    {
        return a.root != b.root ? a.root < b.root : NoteTools::modes().indexOf (a.mode) < NoteTools::modes().indexOf (b.mode);
    });

    juce::StringArray names;

    for (auto& k : keys)
        names.add (k.name());

    return names;
}

SequenceLibrary::Filters SequenceLibrary::similarTo (const Entry& e)
{
    Filters f;
    f.role = e.path[0] == factoryName ? e.path[1] : e.path[0];
    f.style = e.style;
    f.key = e.key;
    f.nearEnergy = e.energy;
    return f;
}

juce::File SequenceLibrary::searchesFile()
{
    if (chosenSearchesFile() != juce::File())
        return chosenSearchesFile();

    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Torqwave").getChildFile ("Sequence Searches.xml");
}

void SequenceLibrary::setSearchesFile (const juce::File& f)
{
    chosenSearchesFile() = f;
}

std::vector<std::pair<juce::String, SequenceLibrary::Filters>> SequenceLibrary::savedSearches()
{
    std::vector<std::pair<juce::String, Filters>> all;

    if (const auto xml = juce::XmlDocument::parse (searchesFile()); xml != nullptr && xml->hasTagName ("TORQWAVE_SEQUENCE_SEARCHES"))
        for (auto* e : xml->getChildWithTagNameIterator ("SEARCH"))
            if (auto* filters = e->getChildByName ("FILTERS"); filters != nullptr && e->getStringAttribute ("name").isNotEmpty())
                all.push_back ({ e->getStringAttribute ("name"), Filters::fromXml (*filters) });

    return all;
}

namespace
{
    juce::String writeSearches (const std::vector<std::pair<juce::String, SequenceLibrary::Filters>>& all)
    {
        juce::XmlElement xml ("TORQWAVE_SEQUENCE_SEARCHES");

        for (auto& [name, filters] : all)
        {
            auto* e = xml.createNewChildElement ("SEARCH");
            e->setAttribute ("name", name);
            e->addChildElement (filters.toXml().release());
        }

        SequenceLibrary::searchesFile().getParentDirectory().createDirectory();
        return xml.writeTo (SequenceLibrary::searchesFile()) ? juce::String() : juce::String ("The saved searches could not be written.");
    }
}

juce::String SequenceLibrary::saveSearch (const juce::String& name, const Filters& filters)
{
    if (name.trim().isEmpty())
        return "A saved search needs a name.";

    auto all = savedSearches();
    const auto same = std::find_if (all.begin(), all.end(), [&name] (auto& s) { return s.first == name.trim(); });

    if (same != all.end())
        same->second = filters;
    else
        all.push_back ({ name.trim(), filters });

    return writeSearches (all);
}

juce::String SequenceLibrary::removeSearch (const juce::String& name)
{
    auto all = savedSearches();
    all.erase (std::remove_if (all.begin(), all.end(), [&name] (auto& s) { return s.first == name; }), all.end());
    return writeSearches (all);
}

bool SequenceLibrary::transposesDrops()
{
    return transposeSetting();
}

void SequenceLibrary::setTransposesDrops (bool on)
{
    transposeSetting() = on;
}

SequenceLibrary::Properties SequenceLibrary::propertiesOf (const Entry& e)
{
    return { e.style, e.key, e.description, e.energy, e.character, e.tags };
}

juce::String SequenceLibrary::setProperties (const juce::File& file, const Properties& p)
{
    if (isFactory (file))
        return "A preset can't be changed: copy it to your library first.";

    return rewrite (file, [&p] (juce::XmlElement& x)
    {
        const auto set = [&x] (const char* name, const juce::String& value)
        {
            if (value.trim().isNotEmpty())
                x.setAttribute (name, value.trim());
            else
                x.removeAttribute (name);
        };

        set ("style", p.style);
        x.setAttribute ("key", p.key.trim());
        x.setAttribute ("note", p.description.trim());
        x.setAttribute ("tags", p.tags.joinIntoString (","));

        if (x.hasTagName ("TORQWAVE_SEQUENCE"))
        {
            if (p.energy > 0)
                x.setAttribute ("energy", juce::jlimit (1, 5, p.energy));
            else
                x.removeAttribute ("energy");

            set ("character", p.character.joinIntoString (","));
        }
    });
}

bool SequenceLibrary::fitsKey (const Entry& e, const juce::String& sessionKey)
{
    const auto session = NoteTools::parseKey (sessionKey);

    if (! session.has_value() || e.key.trim().isEmpty())
        return true;

    const auto own = NoteTools::parseKey (e.key);
    return own.has_value() && *own == *session;
}

juce::String SequenceLibrary::setRating (const juce::File& file, int rating)
{
    const auto change = [rating] (juce::XmlElement& x)
    {
        if (rating > 0)
            x.setAttribute ("rating", juce::jlimit (1, 5, rating));
        else
            x.removeAttribute ("rating");
    };

    return isFactory (file) ? rewriteFactory (file, change) : rewrite (file, change);
}

juce::String SequenceLibrary::setFavourite (const juce::File& file, bool favourite)
{
    const auto change = [favourite] (juce::XmlElement& x)
    {
        if (favourite)
            x.setAttribute ("favourite", true);
        else
            x.removeAttribute ("favourite");
    };

    return isFactory (file) ? rewriteFactory (file, change) : rewrite (file, change);
}

juce::File SequenceLibrary::rename (const juce::File& file, const juce::String& name, juce::String& error)
{
    const auto legal = juce::File::createLegalFileName (name.trim());

    if (legal.isEmpty())
    {
        error = "It needs a name.";
        return {};
    }

    const auto target = file.getSiblingFile (legal + file.getFileExtension());

    if (target.exists() && target != file)
    {
        error = "There is already one called " + legal + " there.";
        return {};
    }

    if (error = rewrite (file, [&legal] (juce::XmlElement& x) { x.setAttribute ("name", legal); }, target); error.isNotEmpty())
        return {};

    if (target != file)
        file.deleteFile();

    return target;
}

juce::File SequenceLibrary::saveKit (const juce::String& name, const juce::Array<juce::File>& sequences, juce::String& error) const
{
    const auto kit = SequenceKit::fromFiles (name, sequences, error);

    if (! kit.has_value())
        return {};

    const auto file = newFileIn (folderFor ("Kits", {}), kit->name, SequenceKit::extension);

    if (error = SequenceKit::save (*kit, file); error.isNotEmpty())
        return {};

    return file;
}

juce::Array<juce::File> SequenceLibrary::sequencesIn (const juce::StringArray& path) const
{
    const auto dir = folderAt (path);
    juce::Array<juce::File> files;

    for (const auto& found : juce::RangedDirectoryIterator (dir, true, juce::String ("*") + SequenceFile::extension, juce::File::findFiles))
        files.add (found.getFile());

    std::sort (files.begin(), files.end(), [&dir] (const juce::File& a, const juce::File& b)
    {
        return a.getRelativePathFrom (dir).compareNatural (b.getRelativePathFrom (dir)) < 0;
    });

    return files;
}

juce::Array<juce::File> SequenceLibrary::importMidi (const juce::Array<juce::File>& files, const juce::StringArray& path,
                                                     std::shared_ptr<const juce::XmlElement> voice, juce::StringArray& errors) const
{
    juce::Array<juce::File> made;
    const auto dir = folderAt (path);

    for (auto& f : files)
    {
        auto imported = SequenceMidi::importFile (f);

        if (imported.error.isNotEmpty())
        {
            errors.add (imported.error);
            continue;
        }

        for (auto& q : imported.sequences)
        {
            q.voice = voice;
            setRoleFrom (q, path);
            const auto target = newFileIn (dir, q.name);

            if (const auto error = SequenceFile::save (q, target); error.isNotEmpty())
                errors.add (error);
            else
                made.add (target);
        }
    }

    return made;
}

juce::String SequenceLibrary::savePack (const juce::StringArray& path, const juce::File& pack, int& count) const
{
    const auto dir = folderAt (path);
    const auto wildcard = juce::String ("*") + SequenceFile::extension + ";*" + SequenceFile::kitExtension;
    juce::ZipFile::Builder zip;
    count = 0;

    for (const auto& found : juce::RangedDirectoryIterator (dir, true, wildcard, juce::File::findFiles))
    {
        const auto stored = found.getFile().getRelativePathFrom (folder).replaceCharacter ('\\', '/');
        zip.addFile (found.getFile(), 9, stored);
        ++count;
    }

    if (count == 0)
        return "There are no sequences or kits in " + nameOf (path) + " to pack.";

    juce::TemporaryFile temp (pack);

    {
        juce::FileOutputStream out (temp.getFile());

        if (! out.openedOk() || ! zip.writeToStream (out, nullptr))
            return "Couldn't write " + pack.getFullPathName() + ".";
    }

    if (! temp.overwriteTargetFileWithTemporary())
        return "Couldn't write " + pack.getFullPathName() + ".";

    return {};
}

juce::Array<juce::File> SequenceLibrary::openPack (const juce::File& pack, juce::StringArray& errors) const
{
    juce::Array<juce::File> made;
    juce::ZipFile zip (pack);

    if (zip.getNumEntries() == 0)
    {
        errors.add (pack.getFileName() + " is not a pack.");
        return made;
    }

    constexpr juce::int64 largest = 64 * 1024 * 1024;   // no sequence is near this; anything bigger is not one

    for (int i = 0; i < zip.getNumEntries(); ++i)
    {
        const auto* entry = zip.getEntry (i);
        const auto stored = entry->filename.replaceCharacter ('\\', '/');
        const auto fileName = stored.fromLastOccurrenceOf ("/", false, false);
        const auto isKit = fileName.endsWithIgnoreCase (SequenceFile::kitExtension);

        if (entry->isSymbolicLink || ! (isKit || fileName.endsWithIgnoreCase (SequenceFile::extension)))
            continue;

        if (entry->uncompressedSize > largest)
        {
            errors.add (stored + " is too large to be a sequence.");
            continue;
        }

        std::unique_ptr<juce::InputStream> in (zip.createStreamForEntry (i));
        const auto xml = in != nullptr ? juce::XmlDocument::parse (in->readEntireStreamAsString()) : nullptr;
        const auto q = xml != nullptr && ! isKit ? SequenceFile::fromXml (*xml) : std::nullopt;

        if (xml == nullptr || (isKit ? ! xml->hasTagName ("TORQWAVE_KIT") : ! q.has_value()))
        {
            errors.add (stored + (isKit ? " is not a kit." : " is not a sequence."));
            continue;
        }

        const auto dir = isKit ? folderFor ("Kits", {}) : folderFor (q->role, q->role == "Inbox" ? juce::String() : q->subRole);
        const auto base = fileName.upToLastOccurrenceOf (".", false, false);
        const auto target = newFileIn (dir, base, isKit ? SequenceFile::kitExtension : SequenceFile::extension);

        if (const auto error = SequenceFile::writeXml (*xml, target); error.isNotEmpty())
            errors.add (error);
        else
            made.add (target);
    }

    return made;
}
