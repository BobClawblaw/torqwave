#include "DecryptPatches.h"
#include "WaveSynth.h"

namespace DecryptedPatches
{
namespace
{
    const juce::String key = BuiltInEffects::basicSynthId;

    std::unique_ptr<Store>& sharedOne()
    {
        static std::unique_ptr<Store> s;
        return s;
    }

    bool isFactory (const juce::String& name)
    {
        for (auto& p : WaveSynth::factoryPatches())
            if (p.name.equalsIgnoreCase (name))
                return true;

        return false;
    }

    bool isCategory (const juce::String& name)
    {
        return name.isNotEmpty() && WaveSynth::patchCategories().contains (name);
    }

    /** The name as the store has it (it matches ignoring case), or nothing. */
    juce::String storedAs (const EffectPresets::Store& presets, const juce::String& name)
    {
        for (auto& n : presets.getNames (key))
            if (n.equalsIgnoreCase (name))
                return n;

        return {};
    }
}

Store::Store (EffectPresets::Store& p, juce::File f) : presets (p), list (std::move (f))
{
    read();
}

Store& Store::shared()
{
    if (sharedOne() == nullptr)
        sharedOne() = std::make_unique<Store> (EffectPresets::userStore(), defaultList());

    return *sharedOne();
}

void Store::use (EffectPresets::Store* p, const juce::File& f)
{
    sharedOne() = p != nullptr ? std::make_unique<Store> (*p, f) : nullptr;
}

juce::File Store::defaultList()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Torqwave").getChildFile ("Synth Decrypted.xml");
}

void Store::read()
{
    listed.clear();

    if (const auto xml = juce::parseXML (list); xml != nullptr && xml->hasTagName ("DECRYPTED"))
        for (auto* e : xml->getChildWithTagNameIterator ("PATCH"))
        {
            Entry entry;
            entry.name = storedAs (presets, e->getStringAttribute ("name").trim());
            entry.category = isCategory (e->getStringAttribute ("category")) ? e->getStringAttribute ("category") : juce::String();
            entry.source = e->getStringAttribute ("source");
            entry.lock = (float) e->getDoubleAttribute ("lock");
            entry.taken = juce::Time (e->getStringAttribute ("taken").getLargeIntValue());

            if (entry.name.isNotEmpty() && indexOf (entry.name) < 0)
                listed.push_back (std::move (entry));
        }

    std::sort (listed.begin(), listed.end(), [] (const Entry& a, const Entry& b) { return a.name.compareIgnoreCase (b.name) < 0; });
}

void Store::write() const
{
    juce::XmlElement xml ("DECRYPTED");

    for (auto& entry : listed)
    {
        auto* e = xml.createNewChildElement ("PATCH");
        e->setAttribute ("name", entry.name);
        e->setAttribute ("category", entry.category);
        e->setAttribute ("source", entry.source);
        e->setAttribute ("lock", (double) entry.lock);
        e->setAttribute ("taken", juce::String (entry.taken.toMilliseconds()));
    }

    list.getParentDirectory().createDirectory();
    xml.writeTo (list);
}

int Store::indexOf (const juce::String& name) const
{
    for (size_t i = 0; i < listed.size(); ++i)
        if (listed[i].name.equalsIgnoreCase (name))
            return (int) i;

    return -1;
}

std::vector<Entry> Store::entries() const
{
    // Those still among the presets: one deleted from the presets bar is no longer here.
    const auto stored = presets.getNames (key);
    std::vector<Entry> still;

    for (auto& entry : listed)
        if (stored.contains (entry.name, true))
            still.push_back (entry);

    return still;
}

juce::StringArray Store::names() const
{
    juce::StringArray all;

    for (auto& entry : entries())
        all.add (entry.name);

    return all;
}

bool Store::contains (const juce::String& name) const
{
    return names().contains (name, true);
}

std::optional<Entry> Store::entry (const juce::String& name) const
{
    for (auto& e : entries())
        if (e.name.equalsIgnoreCase (name))
            return e;

    return std::nullopt;
}

std::vector<Entry> Store::inFolderOrder() const
{
    const auto all = entries();
    std::vector<Entry> ordered;

    for (auto& category : WaveSynth::patchCategories())
        for (auto& e : all)
            if (e.category == category)
                ordered.push_back (e);

    for (auto& e : all)
        if (e.category.isEmpty())
            ordered.push_back (e);

    return ordered;
}

bool Store::isTaken (const juce::String& name) const
{
    return storedAs (presets, name.trim()).isNotEmpty();
}

juce::String Store::whyNot (const juce::String& name, const juce::String& except) const
{
    const auto n = name.trim();

    if (n.isEmpty())
        return "A patch needs a name.";

    if (isFactory (n))
        return n.quoted() + " is a factory patch's name.";

    if (isTaken (n) && ! n.equalsIgnoreCase (except))
        return "There is a preset called " + n.quoted() + " already.";

    return {};
}

bool Store::save (Synth& synth, const juce::String& name, bool replace, Entry facts)
{
    const auto n = name.trim();

    if (n.isEmpty() || isFactory (n) || (isTaken (n) && ! replace))
        return false;

    // A name that differs only in case replaces the one there.
    if (const auto old = storedAs (presets, n); old.isNotEmpty() && old != n)
        presets.remove (key, old);

    const auto was = synth.getPatchName();
    synth.setPatchName (n);

    if (! presets.save (key, n, synth.getPresetValues(), synth.getPresetTexts()))
    {
        synth.setPatchName (was);
        return false;
    }

    if (const auto at = indexOf (n); at >= 0)
        listed.erase (listed.begin() + at);

    facts.name = n;
    facts.category = isCategory (facts.category) ? facts.category : juce::String();
    listed.push_back (std::move (facts));
    std::sort (listed.begin(), listed.end(), [] (const Entry& a, const Entry& b) { return a.name.compareIgnoreCase (b.name) < 0; });
    write();
    return true;
}

bool Store::load (Synth& synth, const juce::String& name) const
{
    const auto stored = storedAs (presets, name);
    const auto values = stored.isNotEmpty() ? presets.load (key, stored) : std::nullopt;

    if (! values.has_value())
        return false;

    synth.setPresetValues (*values);
    auto texts = presets.loadTexts (key, stored);
    texts["patch"] = stored;
    synth.setPresetTexts (texts);
    return true;
}

bool Store::rename (const juce::String& from, const juce::String& to)
{
    const auto old = storedAs (presets, from);
    const auto at = indexOf (old);
    const auto n = to.trim();

    if (old.isEmpty() || at < 0 || whyNot (n, old).isNotEmpty())
        return false;

    if (n == old)
        return true;

    const auto values = presets.load (key, old);

    if (! values.has_value())
        return false;

    auto texts = presets.loadTexts (key, old);
    texts["patch"] = n;

    // Under the new name, then the old one removed (a change of case alone: the old one first).
    if (n.equalsIgnoreCase (old))
        presets.remove (key, old);

    if (! presets.save (key, n, *values, texts))
        return false;

    if (! n.equalsIgnoreCase (old))
        presets.remove (key, old);

    listed[(size_t) at].name = n;
    std::sort (listed.begin(), listed.end(), [] (const Entry& a, const Entry& b) { return a.name.compareIgnoreCase (b.name) < 0; });
    write();
    return true;
}

int Store::setCategory (const juce::StringArray& names, const juce::String& category)
{
    if (category.isNotEmpty() && ! isCategory (category))
        return 0;

    int changed = 0;

    for (auto& name : names)
        if (const auto at = indexOf (name); at >= 0 && listed[(size_t) at].category != category)
        {
            listed[(size_t) at].category = category;
            ++changed;
        }

    if (changed > 0)
        write();

    return changed;
}

bool Store::remove (const juce::String& name)
{
    const auto old = storedAs (presets, name);
    const auto at = indexOf (old);

    if (old.isEmpty() || at < 0)
        return false;

    presets.remove (key, old);
    listed.erase (listed.begin() + at);
    write();
    return true;
}

juce::String Store::step (Synth& synth, int by) const
{
    const auto all = inFolderOrder();

    if (all.empty() || by == 0)
        return {};

    const auto count = (int) all.size();
    auto at = -1;

    for (int i = 0; i < count; ++i)
        if (all[(size_t) i].name.equalsIgnoreCase (synth.getPatchName()))
            at = i;

    const auto next = at < 0 ? (by > 0 ? 0 : count - 1) : (((at + by) % count) + count) % count;
    return load (synth, all[(size_t) next].name) ? all[(size_t) next].name : juce::String();
}
}
