#include "SequenceFile.h"
#include "BuiltInEffects.h"
#include "NoteTools.h"
#include "TrackTemplate.h"

namespace SequenceFile
{
namespace
{
    juce::int64 ticksOf (double beats)
    {
        return (juce::int64) std::llround (beats * (double) ticksPerBeat);
    }
}

std::unique_ptr<juce::XmlElement> voiceOf (const SessionState& s, juce::uint32 trackId, std::unique_ptr<juce::XmlElement> effects)
{
    auto xml = TrackTemplate::create (s, trackId, std::move (effects));

    if (xml == nullptr)
        return nullptr;

    // What belongs to a session stays there: its buses, outputs, inputs and automation.
    for (auto* attribute : { "output", "deviceOutput", "inputFirst", "inputChannels", "sampleRate" })
        xml->removeAttribute (attribute);

    for (int i = xml->getNumChildElements(); --i >= 0;)
        if (! xml->getChildElement (i)->hasTagName ("EFFECTS"))
            xml->removeChildElement (xml->getChildElement (i), true);

    return xml;
}

std::optional<Sequence> fromClip (const SessionState& s, juce::uint32 clipId, std::unique_ptr<juce::XmlElement> effects)
{
    const auto where = s.findNoteClip (clipId);

    if (! where.has_value())
        return std::nullopt;

    const auto& track = s.tracks[(size_t) where->first];
    const auto& clip = track.noteClips[(size_t) where->second];
    const auto map = s.tempoMap();
    const auto origin = clip.getOrigin();
    const auto originBeat = map.beatPosition (origin);
    const auto tickAt = [&] (juce::int64 timelineFrame) { return ticksOf (map.beatPosition (origin + timelineFrame) - originBeat); };

    Sequence q;
    q.name = clip.name.isNotEmpty() ? clip.name : track.name;
    q.channel = clip.channel;
    q.beatsPerBar = map.beatsPerBarAt (clip.position);
    q.writtenTempo = map.tempoAt (clip.position);
    q.offset = tickAt (clip.offset);
    q.length = tickAt (clip.offset + clip.length) - q.offset;
    q.loopLength = clip.loopLength > 0 ? tickAt (clip.loopLength) : 0;

    for (auto& n : clip.notes)
    {
        const auto start = tickAt (n.start);
        q.notes.push_back ({ n.pitch, start, juce::jmax ((juce::int64) 1, tickAt (n.start + n.length) - start), n.velocity, n.probability });
    }

    for (auto& lane : clip.lanes)
    {
        Lane l { lane.controller, {} };

        for (auto& p : lane.envelope.points)
            l.points.push_back ({ tickAt (p.frame), p.value });

        q.lanes.push_back (std::move (l));
    }

    if (auto voice = voiceOf (s, track.id, std::move (effects)))
        q.voice = std::shared_ptr<const juce::XmlElement> (voice.release());

    return q;
}

NoteClip toClip (const Sequence& q, const SessionState& s, juce::int64 position)
{
    const auto map = s.tempoMap();
    const auto startBeat = map.beatPosition (position);
    const auto originBeat = startBeat - (double) q.offset / ticksPerBeat;
    const auto origin = map.frameAtBeatPosition (originBeat);
    const auto frameOf = [&] (juce::int64 tick) { return map.frameAtBeatPosition (originBeat + (double) tick / ticksPerBeat) - origin; };

    NoteClip c;
    c.name = q.name;
    c.channel = q.channel;
    c.position = position;
    c.offset = position - origin;
    c.length = juce::jmax ((juce::int64) 1, map.frameAtBeatPosition (startBeat + (double) q.length / ticksPerBeat) - position);
    c.loopLength = q.loopLength > 0 ? frameOf (q.loopLength) : 0;

    for (auto& n : q.notes)
    {
        const auto start = frameOf (n.start);
        c.notes.push_back ({ n.pitch, start, juce::jmax ((juce::int64) 1, frameOf (n.start + n.length) - start), n.velocity, n.probability });
    }

    for (auto& l : q.lanes)
    {
        ControllerLane lane;
        lane.controller = l.controller;

        for (auto& p : l.points)
            lane.envelope.points.push_back ({ frameOf (p.tick), p.value });

        c.lanes.push_back (std::move (lane));
    }

    c.mergeDuplicateNotes();
    return c;
}

std::unique_ptr<juce::XmlElement> voiceFor (const juce::PluginDescription& d)
{
    auto xml = std::make_unique<juce::XmlElement> ("TRACKTEMPLATE");
    xml->setAttribute ("name", d.name);
    xml->setAttribute ("bus", false);
    xml->setAttribute ("instrument", true);
    xml->setAttribute ("musical", true);
    xml->setAttribute ("gainDb", 0.0);
    xml->setAttribute ("pan", 0.0);
    xml->setAttribute ("colour", -1);
    auto* slot = xml->createNewChildElement ("EFFECTS")->createNewChildElement ("SLOT");
    slot->setAttribute ("bypassed", false);
    slot->setAttribute ("id", 0);
    slot->addChildElement (d.createXml().release());
    return xml;
}

juce::String instrumentOf (const Sequence& q)
{
    if (q.voice == nullptr)
        return {};

    if (auto* effects = q.voice->getChildByName ("EFFECTS"))
        if (auto* slot = effects->getChildByName ("SLOT"))
            if (auto* plugin = slot->getChildByName ("PLUGIN"); plugin != nullptr && plugin->getBoolAttribute ("isInstrument"))
            {
                // A built-in instrument by the name it has now (Basic Synth is Torqwave Synth since 0.19.3).
                if (juce::PluginDescription d; d.loadFromXml (*plugin))
                    for (auto& builtIn : BuiltInEffects::getDescriptions())
                        if (builtIn.fileOrIdentifier == d.fileOrIdentifier && builtIn.pluginFormatName == d.pluginFormatName)
                            return builtIn.name;

                return plugin->getStringAttribute ("name");
            }

    return {};
}

std::unique_ptr<juce::XmlElement> toXml (const Sequence& q)
{
    auto xml = std::make_unique<juce::XmlElement> ("TORQWAVE_SEQUENCE");
    xml->setAttribute ("version", 1);
    xml->setAttribute ("name", q.name);
    xml->setAttribute ("role", q.role);
    xml->setAttribute ("subRole", q.subRole);
    xml->setAttribute ("key", q.key);
    xml->setAttribute ("tags", q.tags.joinIntoString (","));
    xml->setAttribute ("note", q.note);

    if (q.style.isNotEmpty())
        xml->setAttribute ("style", q.style);

    if (q.energy > 0)
        xml->setAttribute ("energy", q.energy);

    if (! q.character.isEmpty())
        xml->setAttribute ("character", q.character.joinIntoString (","));

    if (q.rating > 0)
        xml->setAttribute ("rating", q.rating);

    if (q.favourite)
        xml->setAttribute ("favourite", true);

    if (q.added > 0)
        xml->setAttribute ("added", juce::String (q.added));

    if (q.variationOf.isNotEmpty())
        xml->setAttribute ("variationOf", q.variationOf);

    xml->setAttribute ("ticksPerBeat", ticksPerBeat);
    xml->setAttribute ("writtenTempo", q.writtenTempo);
    xml->setAttribute ("beatsPerBar", q.beatsPerBar);
    xml->setAttribute ("channel", q.channel);
    xml->setAttribute ("offset", juce::String (q.offset));
    xml->setAttribute ("length", juce::String (q.length));
    xml->setAttribute ("loopLength", juce::String (q.loopLength));

    auto* notes = xml->createNewChildElement ("NOTES");

    for (auto& n : q.notes)
    {
        auto* e = notes->createNewChildElement ("N");
        e->setAttribute ("p", n.pitch);
        e->setAttribute ("s", juce::String (n.start));
        e->setAttribute ("l", juce::String (n.length));
        e->setAttribute ("v", juce::String (n.velocity, 6));

        if (n.probability < 1.0f)
            e->setAttribute ("pr", juce::String (n.probability, 6));
    }

    for (auto& l : q.lanes)
    {
        auto* lane = xml->createNewChildElement ("LANE");
        lane->setAttribute ("controller", l.controller);

        for (auto& p : l.points)
        {
            auto* e = lane->createNewChildElement ("P");
            e->setAttribute ("t", juce::String (p.tick));
            e->setAttribute ("v", juce::String (p.value, 6));
        }
    }

    if (q.voice != nullptr)
        xml->createNewChildElement ("VOICE")->addChildElement (new juce::XmlElement (*q.voice));

    return xml;
}

std::optional<Sequence> fromXml (const juce::XmlElement& xml)
{
    if (! xml.hasTagName ("TORQWAVE_SEQUENCE"))
        return std::nullopt;

    // Written at another resolution, ticks are brought to ours.
    const auto theirs = juce::jmax (1, xml.getIntAttribute ("ticksPerBeat", ticksPerBeat));
    const auto tick = [theirs] (const juce::String& text)
    {
        const auto t = text.getLargeIntValue();
        return theirs == ticksPerBeat ? t : (juce::int64) std::llround ((double) t * ticksPerBeat / theirs);
    };

    Sequence q;
    q.name = xml.getStringAttribute ("name");
    q.role = xml.getStringAttribute ("role");
    q.subRole = xml.getStringAttribute ("subRole");
    q.key = xml.getStringAttribute ("key");
    q.tags.addTokens (xml.getStringAttribute ("tags"), ",", {});
    q.tags.trim();
    q.tags.removeEmptyStrings();
    q.note = xml.getStringAttribute ("note");
    q.style = xml.getStringAttribute ("style").trim();
    q.energy = juce::jlimit (0, 5, xml.getIntAttribute ("energy"));
    q.character.addTokens (xml.getStringAttribute ("character"), ",", {});
    q.character.trim();
    q.character.removeEmptyStrings();
    q.variationOf = xml.getStringAttribute ("variationOf");
    q.rating = juce::jlimit (0, 5, xml.getIntAttribute ("rating"));
    q.favourite = xml.getBoolAttribute ("favourite");
    q.added = juce::jmax ((juce::int64) 0, xml.getStringAttribute ("added").getLargeIntValue());
    q.writtenTempo = xml.getDoubleAttribute ("writtenTempo", 120.0);
    q.beatsPerBar = juce::jlimit (1, 32, xml.getIntAttribute ("beatsPerBar", 4));
    q.channel = juce::jlimit (1, 16, xml.getIntAttribute ("channel", 1));
    q.offset = juce::jmax ((juce::int64) 0, tick (xml.getStringAttribute ("offset")));
    q.length = juce::jmax ((juce::int64) 0, tick (xml.getStringAttribute ("length")));
    q.loopLength = juce::jmax ((juce::int64) 0, tick (xml.getStringAttribute ("loopLength")));

    if (auto* notes = xml.getChildByName ("NOTES"))
        for (auto* e : notes->getChildWithTagNameIterator ("N"))
            q.notes.push_back ({ juce::jlimit (0, 127, e->getIntAttribute ("p", 60)), juce::jmax ((juce::int64) 0, tick (e->getStringAttribute ("s"))),
                                 juce::jmax ((juce::int64) 1, tick (e->getStringAttribute ("l"))),
                                 juce::jlimit (0.0f, 1.0f, (float) e->getDoubleAttribute ("v", 0.8)),
                                 juce::jlimit (0.0f, 1.0f, (float) e->getDoubleAttribute ("pr", 1.0)) });

    for (auto* laneXml : xml.getChildWithTagNameIterator ("LANE"))
    {
        Lane l { juce::jlimit (0, ControllerLane::pitchBend, laneXml->getIntAttribute ("controller", 1)), {} };

        for (auto* e : laneXml->getChildWithTagNameIterator ("P"))
            l.points.push_back ({ juce::jmax ((juce::int64) 0, tick (e->getStringAttribute ("t"))), juce::jlimit (0.0f, 1.0f, (float) e->getDoubleAttribute ("v")) });

        q.lanes.push_back (std::move (l));
    }

    if (auto* voice = xml.getChildByName ("VOICE"))
        if (auto* t = voice->getChildByName ("TRACKTEMPLATE"))
            q.voice = std::make_shared<const juce::XmlElement> (*t);

    return q;
}

juce::String writeXml (const juce::XmlElement& xml, const juce::File& file)
{
    if (! file.getParentDirectory().createDirectory())
        return "Couldn't make the folder " + file.getParentDirectory().getFullPathName() + ".";

    juce::TemporaryFile temp (file);

    if (! xml.writeTo (temp.getFile()) || ! temp.overwriteTargetFileWithTemporary())
        return "Couldn't write " + file.getFullPathName() + ".";

    return {};
}

juce::String save (const Sequence& q, const juce::File& file)
{
    auto xml = toXml (q);

    // A new file (a new sequence, a duplicate, a variation) is added now; one rewritten keeps its time.
    if (q.added <= 0 || ! file.existsAsFile())
        xml->setAttribute ("added", juce::String (juce::Time::currentTimeMillis()));

    return writeXml (*xml, file);
}

std::optional<Sequence> load (const juce::File& file, juce::String& error)
{
    const auto xml = juce::XmlDocument::parse (file);
    auto q = xml != nullptr ? fromXml (*xml) : std::nullopt;

    if (! q.has_value())
        error = file.getFileName() + " is not a sequence.";
    else if (q->added <= 0)
        q->added = file.getCreationTime().toMilliseconds();

    return q;
}

Placed place (SessionState& s, const Sequence& q, int trackIndex, juce::int64 position, bool onTrack)
{
    Placed placed;
    position = juce::jmax ((juce::int64) 0, position);

    if (onTrack && juce::isPositiveAndBelow (trackIndex, (int) s.tracks.size()) && s.tracks[(size_t) trackIndex].isInstrument)
    {
        placed.trackId = s.tracks[(size_t) trackIndex].id;
    }
    else
    {
        if (q.voice != nullptr)
        {
            const auto added = TrackTemplate::add (s, *q.voice);
            placed.trackId = added.trackId;
            placed.effects = added.effects;
        }

        if (placed.trackId == 0)
            placed.trackId = s.addInstrumentTrack().id;

        for (auto& t : s.tracks)
            if (t.id == placed.trackId && q.name.isNotEmpty())
                t.name = q.name;
    }

    auto clip = toClip (q, s, position);

    // A drum part landing on a track whose kit isn't laid out as General MIDI is goes to the kit's keys.
    if (onTrack && (q.role == "Drums" || q.channel == 10))
        for (auto& t : s.tracks)
            if (t.id == placed.trackId && DrumSound::remaps (t.drumMap))
            {
                for (auto& n : clip.notes)
                    n.pitch = DrumSound::toKit (t.drumMap, n.pitch);

                clip.mergeDuplicateNotes();
            }

    placed.clipId = SessionEdits::addNoteClip (s, placed.trackId, std::move (clip));
    return placed;
}

Placed blank (SessionState& s, const juce::String& name, int bars, juce::int64 position)
{
    Placed placed;
    auto& track = s.addInstrumentTrack();
    track.name = name;
    placed.trackId = track.id;

    const auto session = s;   // the map refers to its tempo changes
    const auto map = session.tempoMap();
    const auto bar = map.barAt (juce::jmax ((juce::int64) 0, position));

    NoteClip clip;
    clip.name = name;
    clip.position = map.barFrame (bar);
    clip.length = map.barFrame (bar + juce::jmax (1, bars)) - clip.position;
    placed.clipId = SessionEdits::addNoteClip (s, placed.trackId, std::move (clip));
    return placed;
}

const std::vector<std::pair<juce::String, juce::String>>& characterPairs()
{
    static const std::vector<std::pair<juce::String, juce::String>> pairs {
        { "Dark", "Bright" }, { "Sparse", "Dense" }, { "Straight", "Swung" }, { "Clean", "Distorted" }
    };

    return pairs;
}

std::pair<juce::String, juce::String> splitStyle (const juce::String& style)
{
    const auto at = style.indexOfChar ('/');
    return at < 0 ? std::pair { juce::String(), style.trim() } : std::pair { style.substring (0, at).trim(), style.substring (at + 1).trim() };
}

juce::String styleName (const juce::String& style)
{
    const auto [genre, name] = splitStyle (style);
    return genre.isEmpty() ? name : genre + juce::String::fromUTF8 (" \xe2\x80\xba ") + name;
}

Sequence transposedTo (const Sequence& q, const juce::String& sessionKey)
{
    const auto own = NoteTools::parseKey (q.key);
    const auto session = NoteTools::parseKey (sessionKey);

    if (q.channel == 10 || q.role == "Drums" || ! own.has_value() || ! session.has_value())
        return q;

    auto shift = ((session->root - own->root) % 12 + 12) % 12;

    if (shift > 5)
        shift -= 12;

    if (shift == 0)
        return q;

    auto moved = q;

    for (auto& n : moved.notes)
        n.pitch = juce::jlimit (0, 127, n.pitch + shift);

    auto key = *own;
    key.root = session->root;
    moved.key = key.name();
    return moved;
}
}
