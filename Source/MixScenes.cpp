#include "Session.h"

const MixScene::Strip* MixScene::find (juce::uint32 trackId) const
{
    const auto found = std::find_if (strips.begin(), strips.end(), [trackId] (auto& s) { return s.trackId == trackId; });
    return found != strips.end() ? &*found : nullptr;
}

MixScene::Strip MixScene::stripOf (const SessionTrack& t)
{
    return { t.id, t.gainDb, t.pan, t.surroundPan, t.mute, t.solo, t.invertPolarity, t.sends };
}

const MixScene* SessionState::findScene (juce::uint32 id) const
{
    const auto found = std::find_if (scenes.begin(), scenes.end(), [id] (auto& sc) { return sc.id == id; });
    return found != scenes.end() ? &*found : nullptr;
}

namespace SessionEdits
{
namespace
{
    MixScene* findScene (SessionState& s, juce::uint32 id)
    {
        const auto found = std::find_if (s.scenes.begin(), s.scenes.end(), [id] (auto& sc) { return sc.id == id; });
        return found != s.scenes.end() ? &*found : nullptr;
    }

    std::vector<MixScene::Strip> mixOf (const SessionState& s)
    {
        std::vector<MixScene::Strip> strips;

        for (auto& t : s.tracks)
            strips.push_back (MixScene::stripOf (t));

        return strips;
    }
}

juce::uint32 storeScene (SessionState& s, const juce::String& name)
{
    MixScene scene;
    scene.id = s.nextId++;
    scene.name = name.trim().isNotEmpty() ? name.trim() : "Scene " + juce::String ((int) s.scenes.size() + 1);
    scene.strips = mixOf (s);
    s.scenes.push_back (std::move (scene));
    return s.scenes.back().id;
}

bool updateScene (SessionState& s, juce::uint32 id)
{
    auto* scene = findScene (s, id);

    if (scene == nullptr)
        return false;

    auto strips = mixOf (s);

    if (strips == scene->strips)
        return false;

    scene->strips = std::move (strips);
    return true;
}

bool recallScene (SessionState& s, juce::uint32 id)
{
    const auto* scene = s.findScene (id);

    if (scene == nullptr)
        return false;

    auto changed = false;

    for (auto& t : s.tracks)
    {
        const auto* strip = scene->find (t.id);

        if (strip == nullptr)
            continue;

        // Sends only to buses still here; a bus or folder has none.
        std::vector<SessionSend> sends;

        if (! t.isBus && ! t.isControl())
            for (auto& send : strip->sends)
                if (const auto* bus = s.findTrack (send.busId); bus != nullptr && bus->isBus)
                    sends.push_back (send);

        auto recalled = *strip;
        recalled.sends = std::move (sends);

        if (MixScene::stripOf (t) == recalled)
            continue;

        t.gainDb = recalled.gainDb;
        t.pan = recalled.pan;
        t.surroundPan = recalled.surroundPan;
        t.mute = recalled.mute;
        t.solo = recalled.solo;
        t.invertPolarity = recalled.invertPolarity;
        t.sends = std::move (recalled.sends);
        changed = true;
    }

    return changed;
}

bool renameScene (SessionState& s, juce::uint32 id, const juce::String& name)
{
    auto* scene = findScene (s, id);

    if (scene == nullptr || name.trim().isEmpty() || scene->name == name.trim())
        return false;

    scene->name = name.trim();
    return true;
}

bool removeScene (SessionState& s, juce::uint32 id)
{
    const auto before = s.scenes.size();
    s.scenes.erase (std::remove_if (s.scenes.begin(), s.scenes.end(), [id] (auto& sc) { return sc.id == id; }), s.scenes.end());
    return s.scenes.size() != before;
}
}
