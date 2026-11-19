#include "PluginCatalogue.h"

namespace
{
    void set (juce::var& object, const juce::Identifier& name, const juce::var& value)
    {
        if (auto* o = object.getDynamicObject())
            o->setProperty (name, value);
    }

    juce::var newObject()    { return juce::var (new juce::DynamicObject()); }
}

PluginCatalogue::PluginCatalogue (juce::File storeFile)
    : file (std::move (storeFile))
{
    load();
}

PluginCatalogue::~PluginCatalogue()
{
    saveIfNeeded();
}

juce::File PluginCatalogue::defaultFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Torqwave").getChildFile ("Plug-in Catalogue.json");
}

//==============================================================================
juce::var PluginCatalogue::parameterFacts (juce::AudioProcessorParameter& p, bool currentValue)
{
    auto entry = newObject();
    set (entry, "name", p.getName (100));

    if (currentValue)
        set (entry, "value", p.getCurrentValueAsText());

    if (const auto label = p.getLabel(); label.isNotEmpty())
        set (entry, "unit", label);

    if (const auto choices = p.getAllValueStrings(); ! choices.isEmpty() && choices.size() <= 64)
    {
        juce::Array<juce::var> c;

        for (auto& s : choices)
            c.add (s);

        set (entry, "choices", c);
    }
    else
    {
        set (entry, "lowest", p.getText (0.0f, 40));
        set (entry, "highest", p.getText (1.0f, 40));
    }

    if (! currentValue)
        set (entry, "default", p.getText (p.getDefaultValue(), 40));

    return entry;
}

juce::var PluginCatalogue::factsOf (juce::AudioPluginInstance& plugin)
{
    juce::Array<juce::var> list;
    const auto& parameters = plugin.getParameters();

    for (auto* p : parameters)
        if (list.size() < maxParameters)
            list.add (parameterFacts (*p, false));

    auto result = newObject();
    set (result, "parameters", list);
    set (result, "parameterCount", parameters.size());
    set (result, "inputs", plugin.getTotalNumInputChannels());
    set (result, "outputs", plugin.getTotalNumOutputChannels());
    set (result, "latency", plugin.getLatencySamples());
    return result;
}

//==============================================================================
juce::String PluginCatalogue::factsKey (const juce::PluginDescription& d)
{
    return d.createIdentifierString();
}

juce::String PluginCatalogue::understandingKey (const juce::PluginDescription& d)
{
    return (d.manufacturerName.trim() + "|" + d.name.trim()).toLowerCase();
}

void PluginCatalogue::setFacts (const juce::PluginDescription& d, const juce::var& known)
{
    auto entry = newObject();
    set (entry, "name", d.name);
    set (entry, "maker", d.manufacturerName);
    set (entry, "format", d.pluginFormatName);
    set (entry, "category", d.category);
    set (entry, "version", d.version);
    set (entry, "read", juce::Time::getCurrentTime().toISO8601 (true));

    if (auto* from = known.getDynamicObject())
        for (auto& p : from->getProperties())
            set (entry, p.name, p.value);

    {
        const juce::ScopedLock sl (lock);
        facts[factsKey (d)] = entry;
    }

    changed();
}

void PluginCatalogue::setFactsFailed (const juce::PluginDescription& d, const juce::String& reason)
{
    auto failed = newObject();
    set (failed, "error", reason);
    setFacts (d, failed);
}

bool PluginCatalogue::hasCurrentFacts (const juce::PluginDescription& d) const
{
    const juce::ScopedLock sl (lock);
    const auto f = facts.find (factsKey (d));
    return f != facts.end() && f->second["version"].toString() == d.version;
}

juce::var PluginCatalogue::getFacts (const juce::PluginDescription& d) const
{
    const juce::ScopedLock sl (lock);
    const auto f = facts.find (factsKey (d));
    return f != facts.end() ? f->second : juce::var();
}

//==============================================================================
std::optional<PluginCatalogue::Understanding> PluginCatalogue::getUnderstanding (const juce::PluginDescription& d) const
{
    const juce::ScopedLock sl (lock);
    const auto u = understandings.find (understandingKey (d));
    return u != understandings.end() ? std::optional<Understanding> (u->second) : std::nullopt;
}

void PluginCatalogue::setUnderstanding (const juce::PluginDescription& d, Understanding u)
{
    {
        const juce::ScopedLock sl (lock);

        if (u.text.trim().isEmpty())
            understandings.erase (understandingKey (d));
        else
            understandings[understandingKey (d)] = std::move (u);
    }

    changed();
}

juce::String PluginCatalogue::summaryOf (const juce::PluginDescription& d) const
{
    const auto u = getUnderstanding (d);

    if (! u)
        return {};

    // The first sentence: up to a full stop followed by a space or the end, within 300 characters.
    const auto text = u->text.trim().replaceCharacters ("\r\n", "  ");

    for (int i = 0; i < juce::jmin (text.length(), 300); ++i)
        if (text[i] == '.' && (i + 1 == text.length() || text[i + 1] == ' '))
            return text.substring (0, i + 1);

    return text.length() <= 300 ? text : text.substring (0, 300).trimEnd() + "...";
}

bool PluginCatalogue::needsUnderstanding (const juce::PluginDescription& d) const
{
    return ! getUnderstanding (d).has_value();
}

void PluginCatalogue::forget (const juce::PluginDescription& d)
{
    {
        const juce::ScopedLock sl (lock);
        understandings.erase (understandingKey (d));

        for (auto f = facts.begin(); f != facts.end();)
        {
            if (understandingKey (d) == (f->second["maker"].toString().trim() + "|" + f->second["name"].toString().trim()).toLowerCase())
                f = facts.erase (f);
            else
                ++f;
        }
    }

    changed();
}

int PluginCatalogue::getNumFacts() const
{
    const juce::ScopedLock sl (lock);
    return (int) std::count_if (facts.begin(), facts.end(), [] (auto& f) { return ! f.second.hasProperty ("error"); });
}

int PluginCatalogue::getNumUnderstood() const
{
    const juce::ScopedLock sl (lock);
    return (int) understandings.size();
}

//==============================================================================
void PluginCatalogue::load()
{
    const auto parsed = juce::JSON::parse (file);

    if (auto* f = parsed["facts"].getDynamicObject())
        for (auto& p : f->getProperties())
            facts[p.name.toString()] = p.value;

    if (auto* u = parsed["understanding"].getDynamicObject())
        for (auto& p : u->getProperties())
            understandings[p.name.toString()] = { p.value["text"].toString(), p.value["model"].toString(), p.value["version"].toString(),
                                                  juce::Time::fromISO8601 (p.value["when"].toString()) };
}

bool PluginCatalogue::saveIfNeeded()
{
    auto root = newObject();
    auto factsObject = newObject(), understandingObject = newObject();

    {
        const juce::ScopedLock sl (lock);

        if (! dirty)
            return true;

        dirty = false;

        for (auto& [key, value] : facts)
            set (factsObject, key, value);

        for (auto& [key, u] : understandings)
        {
            auto entry = newObject();
            set (entry, "text", u.text);
            set (entry, "model", u.model);
            set (entry, "version", u.version);
            set (entry, "when", u.when.toISO8601 (true));
            set (understandingObject, key, entry);
        }
    }

    set (root, "version", 1);
    set (root, "facts", factsObject);
    set (root, "understanding", understandingObject);

    file.getParentDirectory().createDirectory();
    juce::TemporaryFile temp (file);

    if (temp.getFile().replaceWithText (juce::JSON::toString (root, true)) && temp.overwriteTargetFileWithTemporary())
        return true;

    const juce::ScopedLock sl (lock);
    dirty = true;   // tried again next time
    return false;
}

void PluginCatalogue::changed()
{
    {
        const juce::ScopedLock sl (lock);
        dirty = true;
    }

    if (onChange != nullptr)
        onChange();
}
