#include "AiLibrary.h"
#include "BuiltInEffects.h"
#include "DrumMachine.h"
#include "DrumMaps.h"
#include "SwarmGuitar.h"
#include "WaveSynth.h"

namespace Ai
{
namespace
{
    bool isBuiltIn (const juce::PluginDescription& d, const char* id)
    {
        return d.fileOrIdentifier == id && d.pluginFormatName == BuiltInEffects::formatName;
    }

    juce::StringArray wordsOf (const juce::String& text)
    {
        juce::StringArray words;
        words.addTokens (text.toLowerCase(), " \t,;:/()-_", {});
        words.removeEmptyStrings();
        return words;
    }

    /** How many letters must be changed, added or taken out to turn one string into the other. */
    int editDistance (const juce::String& from, const juce::String& to)
    {
        const auto a = from.toUTF32(), b = to.toUTF32();
        const auto n = from.length(), m = to.length();
        std::vector<int> row ((size_t) m + 1);

        for (int j = 0; j <= m; ++j)
            row[(size_t) j] = j;

        for (int i = 1; i <= n; ++i)
        {
            auto diagonal = row[0];
            row[0] = i;

            for (int j = 1; j <= m; ++j)
            {
                const auto above = row[(size_t) j];
                row[(size_t) j] = juce::jmin (above + 1, row[(size_t) j - 1] + 1, diagonal + (a[i - 1] == b[j - 1] ? 0 : 1));
                diagonal = above;
            }
        }

        return row[(size_t) m];
    }
}

//==============================================================================
std::vector<FactorySound> FactorySounds::of (const juce::PluginDescription& d)
{
    std::vector<FactorySound> sounds;

    if (isBuiltIn (d, BuiltInEffects::basicSynthId))
    {
        for (auto& p : WaveSynth::factoryPatches())
            sounds.push_back ({ p.name, p.category, {} });
    }
    else if (isBuiltIn (d, BuiltInEffects::basicDrumsId))
    {
        for (auto& k : DrumMachine::kits())
            sounds.push_back ({ k.name, {}, k.description });
    }
    else if (isBuiltIn (d, BuiltInEffects::swarmGuitarId))
    {
        for (auto& p : EffectPresets::factory (BuiltInEffects::swarmGuitarId))
            sounds.push_back ({ p.name, p.category, {} });
    }

    return sounds;
}

std::optional<juce::MemoryBlock> FactorySounds::stateOf (const juce::PluginDescription& d, const juce::String& name)
{
    juce::MemoryBlock state;

    if (isBuiltIn (d, BuiltInEffects::basicSynthId))
    {
        BuiltInEffects::BasicSynthProcessor synth;

        if (! synth.loadPatch (name))
            return std::nullopt;

        synth.getStateInformation (state);
        return state;
    }

    if (isBuiltIn (d, BuiltInEffects::basicDrumsId))
    {
        const auto index = DrumMachine::kitIndex (name);

        if (index < 0)
            return std::nullopt;

        BuiltInEffects::BasicDrumsProcessor drums;
        drums.applyKit (DrumMachine::kits()[(size_t) index]);
        drums.getStateInformation (state);
        return state;
    }

    if (isBuiltIn (d, BuiltInEffects::swarmGuitarId))
        for (auto& p : EffectPresets::factory (BuiltInEffects::swarmGuitarId))
            if (p.name == name)
            {
                BuiltInEffects::SwarmGuitarProcessor guitar;
                guitar.setPresetValues (p.values);
                guitar.getStateInformation (state);
                return state;
            }

    return std::nullopt;
}

//==============================================================================
juce::StringArray nearestNames (const juce::StringArray& names, const juce::String& wanted, int most)
{
    struct Scored
    {
        int shared = 0, distance = 0, index = 0;
    };

    const auto words = wordsOf (wanted);
    const auto lower = wanted.trim().toLowerCase();
    std::vector<Scored> scored;

    for (int i = 0; i < names.size(); ++i)
    {
        const auto name = names[i].toLowerCase();
        Scored s;
        s.index = i;
        s.distance = editDistance (name, lower);

        for (auto& w : words)
            s.shared += name.contains (w) ? 1 : 0;

        scored.push_back (s);
    }

    std::sort (scored.begin(), scored.end(), [] (const Scored& a, const Scored& b)
    {
        return a.shared != b.shared ? a.shared > b.shared : a.distance != b.distance ? a.distance < b.distance : a.index < b.index;
    });

    juce::StringArray nearest;

    for (auto& s : scored)
        if (nearest.size() < most)
            nearest.addIfNotAlreadyThere (names[s.index]);

    return nearest;
}

bool hasWords (const juce::String& text, const juce::String& search)
{
    const auto lower = text.toLowerCase();
    juce::StringArray words;
    words.addTokens (search.toLowerCase(), " ", {});
    words.removeEmptyStrings();

    for (auto& w : words)
        if (! lower.contains (w))
            return false;

    return true;
}

//==============================================================================
Library Library::standard()
{
    Library l;
    l.factorySounds = [] (const juce::PluginDescription& d) { return FactorySounds::of (d); };
    l.sequencePresets = [] { return SequenceLibrary (SequenceLibrary::factoryFolder()).scan(); };
    l.transposeDrops = SequenceLibrary::transposesDrops();
    l.drumMapNames = [] { return DrumMaps::userStore().names(); };
    l.drumMap = [] (const juce::String& name) { return DrumMaps::userStore().load (name); };
    return l;
}
}
