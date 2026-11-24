#include "Resume.h"

namespace Resume
{
namespace
{
    constexpr int formatVersion = 1;

    juce::File noteIn (const juce::File& folder)   { return folder.getChildFile ("Left Off.xml"); }
    juce::File markIn (const juce::File& folder)   { return folder.getChildFile ("Reopening"); }

    juce::String int64Text (juce::int64 v)         { return juce::String (v); }

    std::unique_ptr<juce::XmlElement> parse (const juce::File& folder)
    {
        auto xml = juce::XmlDocument::parse (noteIn (folder));

        if (xml == nullptr || ! xml->hasTagName ("TORQWAVE_LEFT_OFF") || xml->getIntAttribute ("version", 0) > formatVersion)
            return nullptr;

        return xml;
    }
}

State gather (const OpenFiles& files, const FilePlace& activePlace, const AudioEngine& engine, bool multitrack,
              const SessionPlace& sessionPlace, const juce::File& project)
{
    State s;
    s.open.multitrack = multitrack;
    s.open.session = engine.session.getFile();
    s.open.rack = engine.createRackXml();
    s.sessionPlace = sessionPlace;
    s.project = project;

    for (int i = 0; i < files.size(); ++i)
    {
        const auto& doc = files.documentAt (i);

        if (doc.getFile() == juce::File() || files.linkAt (i) != nullptr)
            continue;

        if (i == files.getActiveIndex())
            s.open.active = (int) s.open.files.size();

        s.open.files.push_back ({ doc.getFile(), doc.getVideo(), i == files.getActiveIndex() ? activePlace : files.placeAt (i), nullptr });
    }

    s.open.active = juce::jlimit (0, juce::jmax (0, (int) s.open.files.size() - 1), s.open.active);
    return s;
}

std::unique_ptr<juce::XmlElement> toXml (const State& state)
{
    auto xml = std::make_unique<juce::XmlElement> ("TORQWAVE_LEFT_OFF");
    xml->setAttribute ("version", formatVersion);
    xml->setAttribute ("project", state.project.getFullPathName());

    // Every path in full: the note is not beside the files.
    xml->addChildElement (Project::toXml (state.open, {}).release());

    auto* place = xml->createNewChildElement ("SESSION_PLACE");
    place->setAttribute ("cursor", int64Text (state.sessionPlace.cursor));
    place->setAttribute ("selectionStart", int64Text (state.sessionPlace.selection.getStart()));
    place->setAttribute ("selectionEnd", int64Text (state.sessionPlace.selection.getEnd()));
    place->setAttribute ("viewStart", state.sessionPlace.viewStart);
    place->setAttribute ("framesPerPixel", state.sessionPlace.framesPerPixel);
    place->setAttribute ("scrollY", state.sessionPlace.scrollY);
    place->setAttribute ("selectedTrack", state.sessionPlace.selectedTrack);
    return xml;
}

bool isKept (const juce::File& folder)
{
    return parse (folder) != nullptr;
}

juce::String write (const State& state, const juce::File& folder)
{
    if (! folder.createDirectory())
        return "Couldn't make " + folder.getFullPathName() + ".";

    return toXml (state)->writeTo (noteIn (folder)) ? juce::String() : "Couldn't write " + noteIn (folder).getFullPathName() + ".";
}

void clear (const juce::File& folder)
{
    noteIn (folder).deleteFile();
}

Loaded load (const juce::File& folder, const BackgroundLoad::Loader& loader, const std::function<bool()>& keepGoing)
{
    Loaded out;
    const auto xml = parse (folder);

    if (xml == nullptr)
        return out;

    out.found = true;

    if (auto* open = xml->getChildByName ("TORQWAVE_PROJECT"))
    {
        auto read = Project::fromXml (*open, {});
        out.state.open = std::move (read.state);
        out.missing = std::move (read.missing);
    }

    if (const auto path = xml->getStringAttribute ("project"); juce::File::isAbsolutePath (path))
    {
        if (juce::File (path).existsAsFile())
            out.state.project = juce::File (path);
        else
            out.missing.add (path);
    }

    if (auto* place = xml->getChildByName ("SESSION_PLACE"))
    {
        auto& p = out.state.sessionPlace;
        const auto start = place->getStringAttribute ("selectionStart").getLargeIntValue();
        p.cursor = juce::jmax ((juce::int64) 0, place->getStringAttribute ("cursor").getLargeIntValue());
        p.selection = { start, juce::jmax (start, place->getStringAttribute ("selectionEnd").getLargeIntValue()) };
        p.viewStart = juce::jmax (0.0, place->getDoubleAttribute ("viewStart"));
        p.framesPerPixel = juce::jmax (0.0, place->getDoubleAttribute ("framesPerPixel"));
        p.scrollY = juce::jmax (0, place->getIntAttribute ("scrollY"));
        p.selectedTrack = place->getIntAttribute ("selectedTrack", -1);
    }

    for (auto& f : out.state.open.files)
    {
        if (keepGoing != nullptr && ! keepGoing())
        {
            out.audio.emplace_back();
            out.audio.back().error = "Stopped before it was read.";
            continue;
        }

        out.audio.push_back (loader (f.file));

        if (out.audio.back().snapshot == nullptr && out.audio.back().error.isEmpty())
            out.audio.back().error = "It is no longer there.";
    }

    return out;
}

FilePlace restoreFiles (Loaded& loaded, OpenFiles& files, AudioEngine& engine)
{
    auto restored = Project::restoreFiles (loaded.state.open, files, engine, &loaded.audio);
    loaded.missing.addArray (restored.missing);
    return restored.place;
}

//==============================================================================
Keeper::Keeper (juce::File f) : folder (std::move (f))
{
}

juce::String Keeper::note (const State& state)
{
    if (state.isEmpty())
    {
        if (mayClear && last != "none")
        {
            clear (folder);
            last = "none";
        }

        return {};
    }

    // Something is open: what was kept before is replaced, and from here on tracks what is open.
    mayClear = true;
    const auto text = toXml (state)->toString();

    if (text == last)
        return {};

    if (const auto error = write (state, folder); error.isNotEmpty())
        return error;

    last = text;
    return {};
}

//==============================================================================
void markStarted (const juce::File& folder)
{
    folder.createDirectory();
    markIn (folder).replaceWithText ("Torqwave was reopening what was open.");
}

void markFinished (const juce::File& folder)
{
    markIn (folder).deleteFile();
}

bool wasInterrupted (const juce::File& folder)
{
    return markIn (folder).existsAsFile();
}
}
