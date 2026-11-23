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
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Torqwave").getChildFile ("Synth Decrypted.txt");
}

void Store::read()
{
    listed.clear();

    for (auto& line : juce::StringArray::fromLines (list.loadFileAsString()))
        if (const auto name = storedAs (presets, line.trim()); name.isNotEmpty())
            listed.addIfNotAlreadyThere (name);

    listed.sort (true);
}

void Store::write() const
{
    list.getParentDirectory().createDirectory();
    list.replaceWithText (listed.joinIntoString ("\n") + (listed.isEmpty() ? "" : "\n"));
}

juce::StringArray Store::names() const
{
    // Those still among the presets: one deleted from the presets bar is no longer here.
    const auto stored = presets.getNames (key);
    juce::StringArray still;

    for (auto& name : listed)
        if (stored.contains (name, true))
            still.add (name);

    return still;
}

bool Store::contains (const juce::String& name) const
{
    return names().contains (name, true);
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

bool Store::save (Synth& synth, const juce::String& name, bool replace)
{
    const auto n = name.trim();

    if (n.isEmpty() || isFactory (n) || (isTaken (n) && ! replace))
        return false;

    // A name that differs only in case replaces the one there.
    if (const auto old = storedAs (presets, n); old.isNotEmpty() && old != n)
    {
        presets.remove (key, old);
        listed.removeString (old);
    }

    const auto was = synth.getPatchName();
    synth.setPatchName (n);

    if (! presets.save (key, n, synth.getPresetValues(), synth.getPresetTexts()))
    {
        synth.setPatchName (was);
        return false;
    }

    listed.addIfNotAlreadyThere (n);
    listed.sort (true);
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
    const auto n = to.trim();

    if (old.isEmpty() || ! listed.contains (old) || whyNot (n, old).isNotEmpty())
        return false;

    if (n == old)
        return true;

    const auto values = presets.load (key, old);

    if (! values.has_value())
        return false;

    auto texts = presets.loadTexts (key, old);
    texts["patch"] = n;

    // Under the new name first, then the old one removed (a change of case alone: the old one first, the store matching names by case ignored).
    if (n.equalsIgnoreCase (old))
        presets.remove (key, old);

    if (! presets.save (key, n, *values, texts))
        return false;

    if (! n.equalsIgnoreCase (old))
        presets.remove (key, old);

    listed.removeString (old);
    listed.addIfNotAlreadyThere (n);
    listed.sort (true);
    write();
    return true;
}

bool Store::remove (const juce::String& name)
{
    const auto old = storedAs (presets, name);

    if (old.isEmpty() || ! listed.contains (old))
        return false;

    presets.remove (key, old);
    listed.removeString (old);
    write();
    return true;
}

juce::String Store::step (Synth& synth, int by) const
{
    const auto all = names();

    if (all.isEmpty() || by == 0)
        return {};

    const auto count = all.size();
    const auto at = all.indexOf (synth.getPatchName(), true);
    const auto next = at < 0 ? (by > 0 ? 0 : count - 1) : (((at + by) % count) + count) % count;
    return load (synth, all[next]) ? all[next] : juce::String();
}
}
