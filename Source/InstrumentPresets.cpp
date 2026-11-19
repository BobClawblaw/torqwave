#include "InstrumentPresets.h"

namespace InstrumentPresets
{
    Store::Store (juce::File f) : folder (std::move (f)) {}

    juce::File Store::folderFor (const juce::PluginDescription& d) const
    {
        return folder.getChildFile (juce::File::createLegalFileName (d.manufacturerName + " " + d.name + " (" + d.pluginFormatName + ")"));
    }

    juce::File Store::fileFor (const juce::PluginDescription& d, const juce::String& name) const
    {
        // A name differing only in case is the same preset.
        for (auto& f : folderFor (d).findChildFiles (juce::File::findFiles, false, juce::String ("*") + extension))
            if (auto xml = juce::XmlDocument::parse (f); xml != nullptr && xml->getStringAttribute ("name").equalsIgnoreCase (name.trim()))
                return f;

        return folderFor (d).getChildFile (juce::File::createLegalFileName (name.trim()) + extension);
    }

    juce::StringArray Store::getNames (const juce::PluginDescription& d) const
    {
        juce::StringArray names;

        for (auto& f : folderFor (d).findChildFiles (juce::File::findFiles, false, juce::String ("*") + extension))
            if (auto xml = juce::XmlDocument::parse (f); xml != nullptr && xml->hasTagName ("INSTRUMENTPRESET"))
                names.addIfNotAlreadyThere (xml->getStringAttribute ("name"), true);

        names.removeEmptyStrings();
        names.sortNatural();
        return names;
    }

    std::optional<juce::MemoryBlock> Store::load (const juce::PluginDescription& d, const juce::String& name) const
    {
        const auto file = fileFor (d, name);
        const auto xml = juce::XmlDocument::parse (file);

        if (xml == nullptr || ! xml->hasTagName ("INSTRUMENTPRESET") || ! xml->getStringAttribute ("name").equalsIgnoreCase (name.trim()))
            return std::nullopt;

        juce::MemoryBlock state;

        if (auto* s = xml->getChildByName ("STATE"); s == nullptr || ! state.fromBase64Encoding (s->getAllSubText()))
            return std::nullopt;

        return state;
    }

    bool Store::save (const juce::PluginDescription& d, const juce::String& name, const juce::MemoryBlock& state)
    {
        if (name.trim().isEmpty())
            return false;

        juce::XmlElement xml ("INSTRUMENTPRESET");
        xml.setAttribute ("name", name.trim());
        xml.setAttribute ("plugin", d.name);
        xml.setAttribute ("maker", d.manufacturerName);
        xml.setAttribute ("format", d.pluginFormatName);
        xml.createNewChildElement ("STATE")->addTextElement (state.toBase64Encoding());

        const auto file = fileFor (d, name);
        return file.getParentDirectory().createDirectory() && xml.writeTo (file);
    }

    bool Store::remove (const juce::PluginDescription& d, const juce::String& name)
    {
        const auto file = fileFor (d, name);
        return file.existsAsFile() && file.deleteFile();
    }

    Store& userStore()
    {
        static Store store (juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                .getChildFile ("Torqwave").getChildFile ("Instrument Presets"));
        return store;
    }
}
