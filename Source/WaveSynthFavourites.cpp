#include "WaveSynthFavourites.h"
#include "WaveSynth.h"

namespace WaveSynth
{
namespace
{
    int indexOfPatch (const juce::String& name)
    {
        const auto& all = factoryPatches();

        for (size_t i = 0; i < all.size(); ++i)
            if (all[i].name == name)
                return (int) i;

        return -1;
    }

    std::unique_ptr<Favourites>& sharedOnes()
    {
        static std::unique_ptr<Favourites> f;
        return f;
    }
}

Favourites::Favourites (juce::File f) : file (std::move (f))
{
    read();
}

Favourites& Favourites::shared()
{
    if (sharedOnes() == nullptr)
        sharedOnes() = std::make_unique<Favourites> (defaultFile());

    return *sharedOnes();
}

juce::File Favourites::defaultFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Torqwave").getChildFile ("Synth Favourites.txt");
}

void Favourites::useFile (const juce::File& f)
{
    sharedOnes() = std::make_unique<Favourites> (f);
}

void Favourites::read()
{
    marked.clear();

    for (auto& line : juce::StringArray::fromLines (file.loadFileAsString()))
        if (const auto name = line.trim(); name.isNotEmpty() && indexOfPatch (name) >= 0)
            marked.addIfNotAlreadyThere (name);

    marked.sort (true);
}

void Favourites::write() const
{
    file.getParentDirectory().createDirectory();
    file.replaceWithText (marked.joinIntoString ("\n") + (marked.isEmpty() ? "" : "\n"));
}

bool Favourites::contains (const juce::String& name) const
{
    return marked.contains (name);
}

void Favourites::set (const juce::String& name, bool favourite)
{
    if (favourite == contains (name) || (favourite && indexOfPatch (name) < 0))
        return;

    if (favourite)
    {
        marked.add (name);
        marked.sort (true);
    }
    else
        marked.removeString (name);

    write();
}

bool Favourites::toggle (const juce::String& name)
{
    set (name, ! contains (name));
    return contains (name);
}

std::vector<int> Favourites::indices() const
{
    std::vector<int> found;

    for (auto& name : marked)
        if (const auto i = indexOfPatch (name); i >= 0)
            found.push_back (i);

    return found;
}
}
