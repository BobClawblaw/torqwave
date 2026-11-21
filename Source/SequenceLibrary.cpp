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
    const auto text = (e.name + " " + e.key + " " + e.tags.joinIntoString (" ") + " " + e.path.joinIntoString (" ")).toLowerCase();
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
    return { "Name", "Bars", "Rating", "Newest" };
}

void SequenceLibrary::sort (std::vector<Entry>& entries, Sort by)
{
    std::stable_sort (entries.begin(), entries.end(), [by] (const Entry& a, const Entry& b)
    {
        if (const auto pa = a.path.joinIntoString ("/"), pb = b.path.joinIntoString ("/"); pa != pb)
            return pa.compareNatural (pb) < 0;

        switch (by)
        {
            case Sort::bars:   if (! juce::exactlyEqual (a.bars, b.bars)) return a.bars < b.bars; break;
            case Sort::rating: if (a.rating != b.rating) return a.rating > b.rating; break;
            case Sort::newest: if (a.added != b.added) return a.added > b.added; break;
            case Sort::name:   break;
        }

        return a.name.compareNatural (b.name) < 0;
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
