#include "IdeaBoard.h"
#include "SequenceLibrary.h"

juce::File IdeaBoard::fileAt (int part, int scene) const
{
    const auto it = cells.find ({ part, scene });
    return it != cells.end() ? it->second : juce::File();
}

void IdeaBoard::set (int part, int scene, const juce::File& file)
{
    if (file == juce::File())
        cells.erase ({ part, scene });
    else if (juce::isPositiveAndBelow (part, parts.size()) && juce::isPositiveAndBelow (scene, scenes.size()))
        cells[{ part, scene }] = file;
}

bool IdeaBoard::addPart (const juce::String& n)
{
    if (parts.size() >= maxParts)
        return false;

    parts.add (n);
    return true;
}

bool IdeaBoard::addScene (const juce::String& n)
{
    if (scenes.size() >= maxScenes)
        return false;

    scenes.add (n);
    return true;
}

void IdeaBoard::removePart (int index)
{
    if (! juce::isPositiveAndBelow (index, parts.size()))
        return;

    parts.remove (index);
    std::map<Cell, juce::File> kept;

    for (auto& [cell, file] : cells)
        if (cell.first != index)
            kept[{ cell.first > index ? cell.first - 1 : cell.first, cell.second }] = file;

    cells = std::move (kept);
}

void IdeaBoard::removeScene (int index)
{
    if (! juce::isPositiveAndBelow (index, scenes.size()))
        return;

    scenes.remove (index);
    std::map<Cell, juce::File> kept;

    for (auto& [cell, file] : cells)
        if (cell.second != index)
            kept[{ cell.first, cell.second > index ? cell.second - 1 : cell.second }] = file;

    cells = std::move (kept);
}

juce::File IdeaBoard::defaultFolder()
{
    return SequenceLibrary::defaultFolder().getSiblingFile ("Boards");
}

std::unique_ptr<juce::XmlElement> IdeaBoard::toXml (const juce::File& library) const
{
    auto xml = std::make_unique<juce::XmlElement> ("TORQWAVE_BOARD");
    xml->setAttribute ("version", 1);
    xml->setAttribute ("name", name);
    xml->setAttribute ("tempo", tempo);
    xml->setAttribute ("sessionTempo", sessionTempo);

    for (auto& p : parts)
        xml->createNewChildElement ("PART")->setAttribute ("name", p);

    for (auto& sc : scenes)
        xml->createNewChildElement ("SCENE")->setAttribute ("name", sc);

    for (auto& [cell, file] : cells)
    {
        auto* e = xml->createNewChildElement ("CELL");
        e->setAttribute ("part", cell.first);
        e->setAttribute ("scene", cell.second);
        e->setAttribute ("file", file.isAChildOf (library) ? file.getRelativePathFrom (library).replaceCharacter ('\\', '/') : file.getFullPathName());
        e->setAttribute ("relative", file.isAChildOf (library));
    }

    return xml;
}

std::optional<IdeaBoard> IdeaBoard::fromXml (const juce::XmlElement& xml, const juce::File& library)
{
    if (! xml.hasTagName ("TORQWAVE_BOARD"))
        return std::nullopt;

    IdeaBoard b;
    b.name = xml.getStringAttribute ("name", "Board");
    b.tempo = juce::jlimit (20.0, 400.0, xml.getDoubleAttribute ("tempo", 120.0));
    b.sessionTempo = xml.getBoolAttribute ("sessionTempo", true);
    b.parts.clear();
    b.scenes.clear();

    for (auto* e : xml.getChildWithTagNameIterator ("PART"))
        if (b.parts.size() < maxParts)
            b.parts.add (e->getStringAttribute ("name"));

    for (auto* e : xml.getChildWithTagNameIterator ("SCENE"))
        if (b.scenes.size() < maxScenes)
            b.scenes.add (e->getStringAttribute ("name"));

    for (auto* e : xml.getChildWithTagNameIterator ("CELL"))
    {
        const auto path = e->getStringAttribute ("file");
        const auto file = e->getBoolAttribute ("relative") ? library.getChildFile (path) : juce::File::isAbsolutePath (path) ? juce::File (path) : juce::File();
        b.set (e->getIntAttribute ("part", -1), e->getIntAttribute ("scene", -1), file);
    }

    return b;
}

juce::String IdeaBoard::save (const juce::File& file, const juce::File& library) const
{
    if (! file.getParentDirectory().createDirectory())
        return "Couldn't make the folder " + file.getParentDirectory().getFullPathName() + ".";

    juce::TemporaryFile temp (file);

    if (! toXml (library)->writeTo (temp.getFile()) || ! temp.overwriteTargetFileWithTemporary())
        return "Couldn't write " + file.getFullPathName() + ".";

    return {};
}

std::optional<IdeaBoard> IdeaBoard::load (const juce::File& file, const juce::File& library, juce::String& error)
{
    const auto xml = juce::XmlDocument::parse (file);
    auto b = xml != nullptr ? fromXml (*xml, library) : std::nullopt;

    if (! b.has_value())
        error = file.getFileName() + " is not an idea board.";

    return b;
}

SequenceFile::Sequence IdeaBoard::asLoop (SequenceFile::Sequence q)
{
    if (q.loopLength > 0)
        return q;

    // From its start: what comes before it (an offset) never plays, so it is left out.
    if (q.offset > 0)
    {
        std::vector<SequenceFile::Note> notes;

        for (auto n : q.notes)
            if (n.start >= q.offset)
            {
                n.start -= q.offset;
                notes.push_back (n);
            }

        q.notes = std::move (notes);

        for (auto& lane : q.lanes)
            for (auto& p : lane.points)
                p.tick = juce::jmax ((juce::int64) 0, p.tick - q.offset);

        q.offset = 0;
    }

    q.loopLength = juce::jmax ((juce::int64) 1, q.length);
    return q;
}

IdeaBoard::Sent IdeaBoard::sendToSession (const IdeaBoard& board, SessionState& s, juce::int64 at,
                                          const std::function<std::optional<SequenceFile::Sequence> (const juce::File&)>& read)
{
    Sent sent;
    const auto timing = s;   // the map refers to its tempo changes, which nothing here changes
    const auto map = timing.tempoMap();
    auto bar = map.barAt (juce::jmax ((juce::int64) 0, at));

    if (map.barFrame (bar) < at)
        ++bar;

    // Each part's tracks so far, by voice.
    std::map<int, std::vector<std::pair<juce::String, juce::uint32>>> tracksOf;

    for (int k = 0; k < board.scenes.size(); ++k)
    {
        std::vector<std::pair<int, SequenceFile::Sequence>> here;

        for (int p = 0; p < board.parts.size(); ++p)
        {
            const auto file = board.fileAt (p, k);

            if (file == juce::File())
                continue;

            if (auto q = read (file))
                here.emplace_back (p, std::move (*q));
            else
                sent.missing.add (file.getFileNameWithoutExtension());
        }

        if (here.empty())
            continue;

        int bars = 1;

        for (auto& [p, q] : here)
            bars = juce::jmax (bars, (int) std::ceil (q.bars() - 1.0e-9));

        const auto start = map.barFrame (bar), end = map.barFrame (bar + bars);
        SessionEdits::addSection (s, start, end, board.scenes[k]);
        ++sent.sections;

        for (auto& [p, q] : here)
        {
            const auto loop = asLoop (q);
            const auto voice = q.voice != nullptr ? q.voice->toString (juce::XmlElement::TextFormat().singleLine()) : juce::String();
            auto& tracks = tracksOf[p];
            const auto existing = std::find_if (tracks.begin(), tracks.end(), [&] (auto& t) { return t.first == voice; });
            SequenceFile::Placed placed;

            if (existing != tracks.end())
            {
                int index = 0;

                while (index < (int) s.tracks.size() && s.tracks[(size_t) index].id != existing->second)
                    ++index;

                placed = SequenceFile::place (s, loop, index, start, true);
            }

            if (placed.clipId == 0 || existing == tracks.end() || placed.trackId != existing->second)
            {
                placed = SequenceFile::place (s, loop, -1, start, false);

                for (auto& t : s.tracks)
                    if (t.id == placed.trackId)
                        t.name = tracks.empty() ? board.parts[p] : board.parts[p] + " (" + q.name + ")";

                tracks.emplace_back (voice, placed.trackId);
                ++sent.tracks;

                if (placed.effects != nullptr)
                    sent.effects[placed.trackId] = placed.effects;
            }

            if (const auto where = s.findNoteClip (placed.clipId))
            {
                auto& clip = s.tracks[(size_t) where->first].noteClips[(size_t) where->second];
                clip.length = end - clip.position;
                ++sent.clips;
            }
        }

        bar += bars;
    }

    return sent;
}

//==============================================================================
juce::int64 IdeaBoardLauncher::nextBar (const Metronome::TempoMap& map, juce::int64 position, juce::int64 within)
{
    const auto from = juce::jmax ((juce::int64) 0, position + within);
    const auto bar = map.barAt (from);
    const auto frame = map.barFrame (bar);
    return frame >= from ? frame : map.barFrame (bar + 1);
}

bool IdeaBoardLauncher::launch (IdeaBoard::Cell cell, const Metronome::TempoMap& map, juce::int64 position, bool playing)
{
    if (! playing)
    {
        launched.clear();
        launched[cell] = { 0, -1 };
        return true;
    }

    const auto at = nextBar (map, position, margin);

    for (auto it = launched.begin(); it != launched.end();)
    {
        if (it->first.first == cell.first && it->first != cell)
        {
            if (it->second.start >= at)
            {
                it = launched.erase (it);
                continue;
            }

            if (it->second.stop < 0 || it->second.stop > at)
                it->second.stop = at;
        }

        ++it;
    }

    launched[cell] = { at, -1 };
    return false;
}

void IdeaBoardLauncher::stopPart (int part, const Metronome::TempoMap& map, juce::int64 position)
{
    const auto at = nextBar (map, position, margin);

    for (auto it = launched.begin(); it != launched.end();)
    {
        if (it->first.first == part)
        {
            if (it->second.start >= at)
            {
                it = launched.erase (it);
                continue;
            }

            if (it->second.stop < 0 || it->second.stop > at)
                it->second.stop = at;
        }

        ++it;
    }
}

bool IdeaBoardLauncher::launchScene (int scene, const IdeaBoard& board, const Metronome::TempoMap& map, juce::int64 position, bool playing)
{
    if (! playing)
    {
        launched.clear();

        for (int p = 0; p < board.parts.size(); ++p)
            if (board.fileAt (p, scene) != juce::File())
                launched[{ p, scene }] = { 0, -1 };

        return ! launched.empty();
    }

    for (int p = 0; p < board.parts.size(); ++p)
        if (board.fileAt (p, scene) != juce::File())
            launch ({ p, scene }, map, position, true);
        else
            stopPart (p, map, position);

    return false;
}

void IdeaBoardLauncher::tidy (juce::int64 position)
{
    for (auto it = launched.begin(); it != launched.end();)
        it = it->second.stop >= 0 && it->second.stop <= position ? launched.erase (it) : std::next (it);
}

IdeaBoardLauncher::State IdeaBoardLauncher::stateOf (IdeaBoard::Cell cell, juce::int64 position) const
{
    const auto it = launched.find (cell);

    if (it == launched.end() || (it->second.stop >= 0 && it->second.stop <= position))
        return State::idle;

    if (it->second.start > position)
        return State::queued;

    return it->second.stop >= 0 ? State::stopping : State::playing;
}

SessionState IdeaBoardLauncher::stateFor (const SessionState& base, const std::map<IdeaBoard::Cell, juce::uint32>& tracks,
                                          const std::map<IdeaBoard::Cell, SequenceFile::Sequence>& sequences) const
{
    auto s = base;
    const auto hours = (juce::int64) (4.0 * 3600.0 * s.sampleRate);

    for (auto& t : s.tracks)
        t.noteClips.clear();

    for (auto& [cell, launch] : launched)
    {
        const auto track = tracks.find (cell);
        const auto sequence = sequences.find (cell);

        if (track == tracks.end() || sequence == sequences.end())
            continue;

        auto clip = SequenceFile::toClip (IdeaBoard::asLoop (sequence->second), s, launch.start);
        clip.length = (launch.stop >= 0 ? launch.stop : launch.start + hours) - launch.start;

        if (clip.length > 0)
            SessionEdits::addNoteClip (s, track->second, std::move (clip));
    }

    return s;
}
