#include "Resume.h"

namespace Resume
{
namespace
{
    constexpr int formatVersion = 1;

    juce::File noteIn (const juce::File& folder)   { return folder.getChildFile ("Left Off.xml"); }
    juce::File copyBeingWritten (const juce::File& folder)   { return folder.getChildFile ("Session Being Written.tqsession"); }
    juce::File markIn (const juce::File& folder)   { return folder.getChildFile ("Reopening"); }

    juce::String int64Text (juce::int64 v)         { return juce::String (v); }

    void writePlace (juce::XmlElement& place, const SessionPlace& p)
    {
        place.setAttribute ("cursor", int64Text (p.cursor));
        place.setAttribute ("selectionStart", int64Text (p.selection.getStart()));
        place.setAttribute ("selectionEnd", int64Text (p.selection.getEnd()));
        place.setAttribute ("viewStart", p.viewStart);
        place.setAttribute ("framesPerPixel", p.framesPerPixel);
        place.setAttribute ("scrollY", p.scrollY);
        place.setAttribute ("selectedTrack", p.selectedTrack);
    }

    SessionPlace readPlace (const juce::XmlElement* place)
    {
        SessionPlace p;

        if (place == nullptr)
            return p;

        const auto start = place->getStringAttribute ("selectionStart").getLargeIntValue();
        p.cursor = juce::jmax ((juce::int64) 0, place->getStringAttribute ("cursor").getLargeIntValue());
        p.selection = { start, juce::jmax (start, place->getStringAttribute ("selectionEnd").getLargeIntValue()) };
        p.viewStart = juce::jmax (0.0, place->getDoubleAttribute ("viewStart"));
        p.framesPerPixel = juce::jmax (0.0, place->getDoubleAttribute ("framesPerPixel"));
        p.scrollY = juce::jmax (0, place->getIntAttribute ("scrollY"));
        p.selectedTrack = place->getIntAttribute ("selectedTrack", -1);
        return p;
    }

    /** A project named in the note: the file when it is there; otherwise it is said, and given to keep. */
    juce::File readProject (const juce::XmlElement& e, juce::StringArray& missing, juce::File& waiting)
    {
        const auto path = e.getStringAttribute ("project");

        if (! juce::File::isAbsolutePath (path))
            return {};

        if (juce::File (path).existsAsFile())
            return juce::File (path);

        missing.add (path);
        waiting = juce::File (path);
        return {};
    }

    std::unique_ptr<juce::XmlElement> parse (const juce::File& folder)
    {
        auto xml = juce::XmlDocument::parse (noteIn (folder));

        if (xml == nullptr || ! xml->hasTagName ("TORQWAVE_LEFT_OFF") || xml->getIntAttribute ("version", 0) > formatVersion)
            return nullptr;

        return xml;
    }
}

State gather (const OpenFiles& files, const FilePlace& activePlace, const AudioEngine& engine, bool multitrack,
              const SessionPlace& sessionPlace, const juce::File& project, const Waiting& waiting)
{
    State s;
    s.open.multitrack = multitrack;
    s.open.session = engine.session.getFile();
    s.sessionCopy = engine.session.isDirty();
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

    for (auto& f : waiting.files)
        if (files.indexOf (f.file) < 0)
            s.waiting.files.push_back (f);

    if (s.open.session == juce::File() && ! s.sessionCopy)
    {
        s.waiting.session = waiting.session;
        s.waiting.sessionPlace = waiting.sessionPlace;
    }

    if (project == juce::File())
        s.waiting.project = waiting.project;

    return s;
}

std::unique_ptr<juce::XmlElement> toXml (const State& state)
{
    auto xml = std::make_unique<juce::XmlElement> ("TORQWAVE_LEFT_OFF");
    xml->setAttribute ("version", formatVersion);
    xml->setAttribute ("project", state.project.getFullPathName());

    // Every path in full: the note is not beside the files.
    if (state.sessionCopy)
    {
        // The session is in its copy. Its own file is only where it would be saved, so it is
        // named apart from the files to open, which have to be there.
        auto open = state.open;
        open.session = juce::File();
        xml->setAttribute ("sessionCopy", true);
        xml->setAttribute ("sessionFile", state.open.session.getFullPathName());
        xml->addChildElement (Project::toXml (open, {}).release());
    }
    else
    {
        xml->addChildElement (Project::toXml (state.open, {}).release());
    }

    writePlace (*xml->createNewChildElement ("SESSION_PLACE"), state.sessionPlace);

    if (! state.waiting.isEmpty())
    {
        Project::State kept;
        kept.files = state.waiting.files;
        kept.session = state.waiting.session;
        auto* waiting = xml->createNewChildElement ("WAITING");
        waiting->setAttribute ("project", state.waiting.project.getFullPathName());
        waiting->addChildElement (Project::toXml (kept, {}).release());
        writePlace (*waiting->createNewChildElement ("SESSION_PLACE"), state.waiting.sessionPlace);
    }

    return xml;
}

juce::File sessionCopy (const juce::File& folder)
{
    return folder.getChildFile ("Unsaved Session.tqsession");
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
    sessionCopy (folder).deleteFile();
    copyBeingWritten (folder).deleteFile();
}

Loaded load (const juce::File& folder, const BackgroundLoad::Loader& loader, const std::function<bool()>& keepGoing)
{
    Loaded out;
    const auto xml = parse (folder);

    if (xml == nullptr)
        return out;

    out.found = true;

    auto& state = out.state;

    if (auto* open = xml->getChildByName ("TORQWAVE_PROJECT"))
    {
        auto read = Project::fromXml (*open, {});
        state.open = std::move (read.state);
        out.missing = std::move (read.missing);
        state.waiting.files = std::move (read.missingFiles);
        state.waiting.session = read.missingSession;
    }

    // A session kept in a copy: the copy is the session, whether or not its own file is there.
    if (xml->getBoolAttribute ("sessionCopy"))
    {
        const auto path = xml->getStringAttribute ("sessionFile");
        const auto own = juce::File::isAbsolutePath (path) ? juce::File (path) : juce::File();

        if (sessionCopy (folder).existsAsFile())
        {
            state.sessionCopy = true;
            state.open.session = own;
        }
        else if (own.existsAsFile())
        {
            out.missing.add ("The changes to " + own.getFileName() + " that were not saved (their copy is no longer there; it was opened as last saved)");
            state.open.session = own;
        }
        else if (own != juce::File())
        {
            out.missing.add (own.getFullPathName());
            state.waiting.session = own;
        }
        else
        {
            out.missing.add ("The session that was never saved (its copy is no longer there)");
        }
    }

    state.sessionPlace = readPlace (xml->getChildByName ("SESSION_PLACE"));
    state.project = readProject (*xml, out.missing, state.waiting.project);

    // A session that is waiting keeps the place noted for it.
    if (state.waiting.session != juce::File())
        state.waiting.sessionPlace = state.sessionPlace;

    // What was waiting: back, it is opened after the others; still away, it goes on waiting.
    if (auto* waiting = xml->getChildByName ("WAITING"))
    {
        if (auto* kept = waiting->getChildByName ("TORQWAVE_PROJECT"))
        {
            auto read = Project::fromXml (*kept, {});
            out.missing.addArray (read.missing);

            for (auto& f : read.state.files)
                state.open.files.push_back (f);

            for (auto& f : read.missingFiles)
                state.waiting.files.push_back (f);

            if (state.open.session == juce::File() && state.waiting.session == juce::File() && ! state.sessionCopy)
            {
                const auto place = readPlace (waiting->getChildByName ("SESSION_PLACE"));

                if (read.state.session != juce::File())
                {
                    state.open.session = read.state.session;
                    state.sessionPlace = place;
                }
                else if (read.missingSession != juce::File())
                {
                    state.waiting.session = read.missingSession;
                    state.waiting.sessionPlace = place;
                }
            }
        }

        if (state.project == juce::File() && state.waiting.project == juce::File())
            state.project = readProject (*waiting, out.missing, state.waiting.project);
    }

    // Read each; one that can't be read is said, and waits with the others.
    std::vector<Project::OpenFile> readable;
    auto active = 0;

    for (size_t i = 0; i < state.open.files.size(); ++i)
    {
        const auto& f = state.open.files[i];
        AudioFileIO::Loaded loaded;

        if (keepGoing != nullptr && ! keepGoing())
            loaded.error = "Stopped before it was read.";
        else
            loaded = loader (f.file);

        if (loaded.snapshot == nullptr)
        {
            out.missing.add (f.file.getFileName() + ": " + (loaded.error.isNotEmpty() ? loaded.error : juce::String ("It is no longer there.")));
            state.waiting.files.push_back (f);
            continue;
        }

        // The one shown, or the one before it when that can't be read.
        if ((int) i <= state.open.active)
            active = (int) readable.size();

        readable.push_back (f);
        out.audio.push_back (std::move (loaded));
    }

    state.open.files = std::move (readable);
    state.open.active = active;
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

juce::String Keeper::note (const State& state, const SessionWriter& writeSession, bool sessionChanged)
{
    if (state.isEmpty())
    {
        if (mayClear && last != "none")
        {
            clear (folder);
            last = "none";
            copyWritten = false;
        }

        return {};
    }

    // Something is open: what was kept before is replaced, and from here on tracks what is open.
    mayClear = true;

    // The session first, so the note never speaks of a copy that isn't there.
    if (state.sessionCopy && (sessionChanged || ! copyWritten))
    {
        if (writeSession == nullptr)
            return "The session's unsaved changes could not be kept: nothing was given to write them.";

        if (! folder.createDirectory())
            return "Couldn't make " + folder.getFullPathName() + ".";

        const auto part = copyBeingWritten (folder);

        if (const auto error = writeSession (part); error.isNotEmpty())
        {
            part.deleteFile();
            return error;
        }

        if (! part.moveFileTo (sessionCopy (folder)))
        {
            part.deleteFile();
            return "Couldn't write " + sessionCopy (folder).getFullPathName() + ".";
        }

        copyWritten = true;
    }

    const auto text = toXml (state)->toString();

    if (text != last)
    {
        if (const auto error = write (state, folder); error.isNotEmpty())
            return error;

        last = text;
    }

    // A session saved or closed since: its copy goes once the note no longer speaks of it.
    if (! state.sessionCopy)
    {
        sessionCopy (folder).deleteFile();
        copyWritten = false;
    }

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
