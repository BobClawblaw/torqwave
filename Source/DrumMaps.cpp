#include "DrumMaps.h"

namespace DrumMaps
{
Store::Store (juce::File f) : folder (std::move (f)) {}

juce::String Store::cleaned (const juce::String& name)
{
    return juce::File::createLegalFileName (name.replace (":", " -").replaceCharacters ("/\\", "  ")).trim().substring (0, 80).trim();
}

juce::File Store::fileFor (const juce::String& name) const
{
    return folder.getChildFile (cleaned (name) + extension);
}

juce::StringArray Store::names() const
{
    juce::StringArray found;

    for (auto& f : folder.findChildFiles (juce::File::findFiles, false, juce::String ("*") + extension))
        found.add (f.getFileNameWithoutExtension());

    found.sortNatural();
    return found;
}

std::vector<DrumSound> Store::load (const juce::String& name) const
{
    if (const auto xml = juce::XmlDocument::parse (fileFor (name)); xml != nullptr && xml->hasTagName ("DRUMMAP"))
        return DrumSound::read (*xml);

    return {};
}

juce::String Store::save (const juce::String& name, const std::vector<DrumSound>& map) const
{
    if (cleaned (name).isEmpty())
        return "The map needs a name.";

    if (! folder.createDirectory())
        return "Couldn't make the folder " + folder.getFullPathName() + ".";

    juce::XmlElement xml ("DRUMMAP");
    xml.setAttribute ("name", cleaned (name));
    DrumSound::write (map, xml);

    const auto file = fileFor (name);
    juce::TemporaryFile temp (file);

    if (! xml.writeTo (temp.getFile()) || ! temp.overwriteTargetFileWithTemporary())
        return "Couldn't write " + file.getFullPathName() + ".";

    return {};
}

bool Store::remove (const juce::String& name) const
{
    const auto file = fileFor (name);
    return file.existsAsFile() && file.deleteFile();
}

Store& userStore()
{
    static Store store (juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Torqwave").getChildFile ("Drum Maps"));
    return store;
}
}
