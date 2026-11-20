#include "SequenceLibrary.h"

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
    return juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Torqwave").getChildFile ("Sequences");
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

juce::File SequenceLibrary::newFileIn (const juce::File& dir, const juce::String& name)
{
    const auto base = juce::File::createLegalFileName (name.trim().isNotEmpty() ? name.trim() : juce::String ("Sequence"));
    auto file = dir.getChildFile (base + SequenceFile::extension);

    for (int n = 2; file.exists(); ++n)
        file = dir.getChildFile (base + " " + juce::String (n) + SequenceFile::extension);

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

    auto q = SequenceFile::load (sequence, error);

    if (! q.has_value())
        return {};

    setRoleFrom (*q, path);

    if (! dir.isDirectory() && ! dir.createDirectory())
    {
        error = "The folder " + dir.getFileName() + " could not be made.";
        return {};
    }

    const auto target = newFileIn (dir, sequence.getFileNameWithoutExtension());

    if (error = SequenceFile::save (*q, target); error.isNotEmpty())
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

    for (const auto& found : juce::RangedDirectoryIterator (folder, true, juce::String ("*") + SequenceFile::extension, juce::File::findFiles))
    {
        const auto file = found.getFile();
        const auto xml = juce::XmlDocument::parse (file);
        const auto q = xml != nullptr ? SequenceFile::fromXml (*xml) : std::nullopt;

        if (! q.has_value())
            continue;

        Entry e;
        e.file = file;
        e.name = q->name.isNotEmpty() ? q->name : file.getFileNameWithoutExtension();
        e.key = q->key;
        e.tags = q->tags;
        e.bars = q->bars();
        e.hasInstrument = SequenceFile::instrumentOf (*q).isNotEmpty();
        e.path.addTokens (file.getParentDirectory().getRelativePathFrom (folder), juce::File::getSeparatorString(), {});
        e.path.removeString (".");
        e.path.removeEmptyStrings();
        entries.push_back (std::move (e));
    }

    std::sort (entries.begin(), entries.end(), [] (const Entry& a, const Entry& b)
    {
        const auto pa = a.path.joinIntoString ("/"), pb = b.path.joinIntoString ("/");
        return pa != pb ? pa.compareNatural (pb) < 0 : a.name.compareNatural (b.name) < 0;
    });

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
