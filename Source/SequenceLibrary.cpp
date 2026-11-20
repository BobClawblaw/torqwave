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
    const auto dir = folderFor (role, subRole);
    const auto base = juce::File::createLegalFileName (name.trim().isNotEmpty() ? name.trim() : juce::String ("Sequence"));
    auto file = dir.getChildFile (base + SequenceFile::extension);

    for (int n = 2; file.exists(); ++n)
        file = dir.getChildFile (base + " " + juce::String (n) + SequenceFile::extension);

    return file;
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
