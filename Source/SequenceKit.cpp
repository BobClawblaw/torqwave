#include "SequenceKit.h"
#include "AudioEngine.h"
#include "NoteTools.h"

namespace SequenceKit
{
double Kit::bars() const noexcept
{
    double most = 0.0;

    for (auto& p : parts)
        most = juce::jmax (most, p.bars());

    return most;
}

std::unique_ptr<juce::XmlElement> toXml (const Kit& kit)
{
    auto xml = std::make_unique<juce::XmlElement> ("TORQWAVE_KIT");
    xml->setAttribute ("version", 1);
    xml->setAttribute ("name", kit.name);
    xml->setAttribute ("key", kit.key);
    xml->setAttribute ("tags", kit.tags.joinIntoString (","));
    xml->setAttribute ("note", kit.note);

    if (kit.rating > 0)
        xml->setAttribute ("rating", kit.rating);

    if (kit.favourite)
        xml->setAttribute ("favourite", true);

    if (kit.added > 0)
        xml->setAttribute ("added", juce::String (kit.added));

    for (auto& part : kit.parts)
        xml->addChildElement (SequenceFile::toXml (part).release());

    return xml;
}

std::optional<Kit> fromXml (const juce::XmlElement& xml)
{
    if (! xml.hasTagName ("TORQWAVE_KIT"))
        return std::nullopt;

    Kit kit;
    kit.name = xml.getStringAttribute ("name");
    kit.key = xml.getStringAttribute ("key");
    kit.tags.addTokens (xml.getStringAttribute ("tags"), ",", {});
    kit.tags.trim();
    kit.tags.removeEmptyStrings();
    kit.note = xml.getStringAttribute ("note");
    kit.rating = juce::jlimit (0, 5, xml.getIntAttribute ("rating"));
    kit.favourite = xml.getBoolAttribute ("favourite");
    kit.added = juce::jmax ((juce::int64) 0, xml.getStringAttribute ("added").getLargeIntValue());

    for (auto* e : xml.getChildWithTagNameIterator ("TORQWAVE_SEQUENCE"))
        if (auto part = SequenceFile::fromXml (*e))
            kit.parts.push_back (std::move (*part));

    return kit;
}

juce::String save (const Kit& kit, const juce::File& file)
{
    auto xml = toXml (kit);

    if (kit.added <= 0 || ! file.existsAsFile())
        xml->setAttribute ("added", juce::String (juce::Time::currentTimeMillis()));

    return SequenceFile::writeXml (*xml, file);
}

std::optional<Kit> load (const juce::File& file, juce::String& error)
{
    const auto xml = juce::XmlDocument::parse (file);
    auto kit = xml != nullptr ? fromXml (*xml) : std::nullopt;

    if (! kit.has_value())
        error = file.getFileName() + " is not a kit.";
    else if (kit->added <= 0)
        kit->added = file.getCreationTime().toMilliseconds();

    return kit;
}

std::optional<Kit> fromFiles (const juce::String& name, const juce::Array<juce::File>& files, juce::String& error)
{
    Kit kit;
    kit.name = name.trim().isNotEmpty() ? name.trim() : juce::String ("Kit");
    juce::StringArray keys;

    for (auto& f : files)
    {
        auto q = SequenceFile::load (f, error);

        if (! q.has_value())
            return std::nullopt;

        // A part's own rating and time belong to it in the library, not to the kit.
        q->rating = 0;
        q->favourite = false;
        q->added = 0;

        // Keys compared as keys: "Em" is "E minor".
        if (const auto scale = NoteTools::parseKey (q->key))
            keys.addIfNotAlreadyThere (scale->name());

        for (auto& tag : q->tags)
            kit.tags.addIfNotAlreadyThere (tag);

        kit.parts.push_back (std::move (*q));
    }

    if (kit.parts.empty())
    {
        error = "A kit needs at least one sequence.";
        return std::nullopt;
    }

    kit.key = keys.size() == 1 ? keys[0] : juce::String();
    return kit;
}

Placed place (SessionState& s, const Kit& kit, juce::int64 position)
{
    Placed placed;
    auto& folder = s.addFolder();

    if (kit.name.isNotEmpty())
        folder.name = kit.name;

    placed.folderId = folder.id;

    for (auto& part : kit.parts)
    {
        auto p = SequenceFile::place (s, part, -1, position, false);

        if (p.clipId == 0)
            continue;

        SessionEdits::setTrackFolder (s, p.trackId, placed.folderId);
        placed.parts.push_back (std::move (p));
    }

    return placed;
}

Placed addToSession (AudioEngine& engine, const Kit& kit, juce::int64 position, std::function<void (const juce::String& errors)> done)
{
    auto next = *engine.session.getState();
    auto placed = place (next, kit, position);

    if (placed.parts.empty())
    {
        if (done != nullptr)
            done (kit.name + " has no parts to add.");

        return {};
    }

    engine.session.apply (std::move (next), "Add Kit " + kit.name);

    std::map<juce::uint32, std::shared_ptr<juce::XmlElement>> chains;

    for (auto& p : placed.parts)
        if (p.effects != nullptr)
            chains[p.trackId] = p.effects;

    if (chains.empty())
    {
        if (done != nullptr)
            done ({});

        return placed;
    }

    engine.session.markEffectsChanged();
    engine.loadTrackEffects (std::move (chains), std::move (done));
    return placed;
}
}
