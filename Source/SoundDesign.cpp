#include "SoundDesign.h"
#include "AudioEdits.h"
#include "AudioFileIO.h"
#include "SoundCategories.h"
#include "SoundMetadata.h"
#include "TempoFollow.h"
#include <set>

namespace SoundDesign
{
namespace
{
    const SessionClip* findClip (const SessionState& s, juce::uint32 clipId)
    {
        const auto where = s.findClip (clipId);
        return where.has_value() ? &s.tracks[(size_t) where->first].clips[(size_t) where->second] : nullptr;
    }
}

ClipEdit editClip (const SessionState& s, juce::uint32 clipId)
{
    ClipEdit edit;
    edit.clipId = clipId;
    const auto* c = findClip (s, clipId);

    if (c == nullptr || c->audio == nullptr)
    {
        edit.error = "The clip is no longer in the session.";
        return edit;
    }

    edit.clipName = c->name;
    const auto where = s.findClip (clipId);

    if (c->locked || s.tracks[(size_t) where->first].isFrozen())
    {
        edit.error = c->locked ? "The clip is locked. Unlock it to edit it in place."
                               : "The clip's track is frozen. Unfreeze it to edit the clip in place.";
        return edit;
    }

    if (c->tempoFollow.follows())
    {
        // The part of the original it plays, whole frames round it.
        const auto& original = c->tempoFollow.original;
        const auto span = TempoFollow::spanOf (s, *c);
        const auto start = juce::jlimit ((juce::int64) 0, original->getNumFrames(), (juce::int64) std::floor (span.start));
        const auto end = juce::jlimit (start, original->getNumFrames(), (juce::int64) std::ceil (span.end));

        if (end <= start || end - start > std::numeric_limits<int>::max())
        {
            edit.error = "The clip plays none of its audio.";
            return edit;
        }

        juce::AudioBuffer<float> part (original->getNumChannels(), (int) (end - start));
        original->read (part, 0, start, part.getNumSamples());
        edit.audio = AudioEdits::fromBuffer (std::move (part));
        edit.original = true;
        edit.start = start;
        edit.from = original;
        return edit;
    }

    edit.audio = c->comp.empty() || c->takes.empty() ? SessionEdits::playedPart (*c, c->audio) : SessionEdits::compAudio (*c);

    if (edit.audio == nullptr)
        edit.error = "The clip plays none of its audio.";

    return edit;
}

AudioDocument::Snapshot toSave (const ClipEdit& edit, const AudioDocument::Snapshot& edited, juce::String& error)
{
    error = {};

    if (edited == nullptr || edited->getNumFrames() <= 0)
    {
        error = "The edited audio is empty.";
        return nullptr;
    }

    if (! edit.original)
        return edited;

    if (edit.audio == nullptr || edit.from == nullptr || edited->getNumFrames() != edit.audio->getNumFrames()
        || edit.from->getNumFrames() > std::numeric_limits<int>::max())
    {
        error = "This clip follows the tempo, so its edit must stay as long as it was ("
                + juce::String (edit.audio != nullptr ? edit.audio->getNumFrames() : 0) + " frames; it is now "
                + juce::String (edited->getNumFrames()) + "): its warp markers and place are frames of its audio. "
                  "Edit it without changing its length, or play the clip at its own speed first (Tempo > Play at Its Own Speed).";
        return nullptr;
    }

    // The whole original with the part replaced; channels the edit added or dropped follow it.
    const auto channels = edited->getNumChannels();
    juce::AudioBuffer<float> whole (channels, (int) edit.from->getNumFrames()), part (channels, (int) edited->getNumFrames());
    whole.clear();
    edit.from->read (whole, 0, 0, whole.getNumSamples());
    edited->read (part, 0, 0, part.getNumSamples());

    for (int ch = 0; ch < channels; ++ch)
        whole.copyFrom (ch, (int) edit.start, part, ch, 0, part.getNumSamples());

    return AudioEdits::fromBuffer (std::move (whole));
}

bool putBack (SessionState& s, const ClipEdit& edit, const juce::File& file, AudioDocument::Snapshot saved, juce::String& error)
{
    error = {};
    const auto* c = findClip (s, edit.clipId);

    if (c == nullptr)
    {
        error = "The clip is no longer in the session.";
        return false;
    }

    if (edit.original)
    {
        if (! c->tempoFollow.follows() || c->tempoFollow.original != edit.from)
        {
            error = "The clip's audio changed in the session after it was opened here, so the edit can't go back into it.";
            return false;
        }

        if (! SessionEdits::replaceOriginal (s, edit.clipId, file, std::move (saved)))
        {
            error = "The clip is locked, or its track is frozen.";
            return false;
        }

        return true;
    }

    if (c->tempoFollow.follows())
    {
        error = "The clip follows the tempo now: open it again to edit it in place.";
        return false;
    }

    if (! SessionEdits::addEditedTake (s, edit.clipId, file, std::move (saved)))
    {
        error = "The clip is locked, or its track is frozen.";
        return false;
    }

    return true;
}

//==============================================================================
AudioDocument::Snapshot clipSound (const SessionClip& c)
{
    const auto& rendered = c.effects.rendered;
    auto played = rendered != nullptr && rendered->getNumFrames() == c.length ? rendered
                : c.comp.empty() || c.takes.empty()                           ? SessionEdits::playedPart (c, c.audio)
                                                                              : SessionEdits::compAudio (c);

    if (played == nullptr || c.length <= 0)
        return nullptr;

    // As the mixer reads it: the fades, then the gain line at the clip's audio frames, then the gain.
    juce::AudioBuffer<float> audio (played->getNumChannels(), (int) c.length);
    played->read (audio, 0, 0, audio.getNumSamples());
    const auto count = audio.getNumSamples();
    std::vector<float> line ((size_t) count, 0.0f);

    if (! c.gainEnvelope.isEmpty())
    {
        c.gainEnvelope.fill (c.offset, line.data(), count);

        for (auto& g : line)
            g = juce::Decibels::decibelsToGain (g, -150.0f);
    }

    const auto gain = juce::Decibels::decibelsToGain (c.gainDb, -150.0f);

    for (int ch = 0; ch < audio.getNumChannels(); ++ch)
    {
        auto* d = audio.getWritePointer (ch);

        for (int i = 0; i < count; ++i)
        {
            if (c.fadeIn > 0 || c.fadeOut > 0)
                d[i] *= FadeShapes::clipLevel (c.fadeInShape, c.fadeIn, c.fadeOutShape, c.fadeOut, i, c.length);

            if (! c.gainEnvelope.isEmpty())
                d[i] *= line[(size_t) i];
        }

        audio.applyGain (ch, 0, count, gain);
    }

    return AudioEdits::fromBuffer (std::move (audio));
}

SessionState combineSource (const SessionState& s, const std::vector<juce::uint32>& clipIds)
{
    const std::set<juce::uint32> wanted (clipIds.begin(), clipIds.end());
    auto alone = s;
    alone.tracks.clear();
    alone.surround = alone.sevenOne = false;
    alone.playArrangement = false;

    for (auto& t : s.tracks)
    {
        if (! t.takesAudioClips() && ! t.isFrozen())
            continue;

        auto copy = t;
        copy.clips.clear();

        for (auto& c : t.clips)
            if (wanted.count (c.id) > 0)
                copy.clips.push_back (c);

        if (copy.clips.empty())
            continue;

        copy.noteClips.clear();
        copy.frozenAudio = nullptr;    // its clips, as they are, rather than what it was frozen to
        copy.frozenFile = juce::File();
        copy.sends.clear();
        copy.mute = copy.solo = false;
        copy.outputBusId = 0;
        copy.deviceOutput = -1;
        copy.folderId = 0;
        copy.vcaId = 0;
        alone.tracks.push_back (std::move (copy));
    }

    SessionEdits::addKeys (s, alone);
    return alone;
}

juce::Range<juce::int64> combineRange (const SessionState& s, const std::vector<juce::uint32>& clipIds)
{
    std::optional<juce::Range<juce::int64>> range;

    for (auto id : clipIds)
        if (const auto* c = findClip (s, id))
            range = range.has_value() ? range->getUnionWith ({ c->position, c->getEnd() }) : juce::Range<juce::int64> (c->position, c->getEnd());

    return range.value_or (juce::Range<juce::int64>());
}

juce::uint32 landCombined (SessionState& s, const std::vector<juce::uint32>& clipIds, const juce::File& file, AudioDocument::Snapshot audio,
                           juce::int64 position, const juce::String& name)
{
    auto last = -1;

    for (auto id : clipIds)
        if (const auto where = s.findClip (id))
        {
            last = juce::jmax (last, where->first);
            s.tracks[(size_t) where->first].clips[(size_t) where->second].muted = true;
        }

    if (last < 0)
        return 0;

    auto& added = s.addTrack();
    added.name = name;
    const auto trackId = added.id;
    const auto clipId = SessionEdits::addClip (s, (int) s.tracks.size() - 1, file, std::move (audio), position);

    if (const auto where = s.findClip (clipId))
        s.tracks[(size_t) where->first].clips[(size_t) where->second].name = name;

    // From the end, where addTrack put it, to just after the last of the clips' tracks.
    std::rotate (s.tracks.begin() + last + 1, s.tracks.end() - 1, s.tracks.end());
    jassert (s.tracks[(size_t) last + 1].id == trackId);
    juce::ignoreUnused (trackId);
    return clipId;
}

//==============================================================================
Filed addToLibrary (SoundLibrary::Index& index, const AudioSnapshot& audio, double sampleRate, const juce::File& folder, const juce::String& name,
                    const juce::String& catId, const juce::StringArray& tags, juce::AudioFormatManager& formats)
{
    Filed filed;
    const auto legal = juce::File::createLegalFileName (name.trim());

    if (! index.isOpen())
        filed.error = "The Sound Library's index can't be opened: " + index.getError();
    else if (legal.isEmpty())
        filed.error = "The sound needs a name.";
    else if (catId.isNotEmpty() && Ucs::byCatId (catId) == nullptr)
        filed.error = catId + " is not a UCS category.";
    else if (audio.getNumFrames() <= 0)
        filed.error = "The sound is empty.";
    else if (! folder.createDirectory())
        filed.error = "Could not make the folder " + folder.getFullPathName();

    if (filed.error.isNotEmpty())
        return filed;

    // In a watched folder, so a rescan keeps it.
    const auto folders = index.getFolders();
    const auto watched = std::any_of (folders.begin(), folders.end(), [&folder] (auto& f) { return folder == f || folder.isAChildOf (f); });

    if (! watched && ! index.addFolder (folder))
    {
        filed.error = index.getError();
        return filed;
    }

    filed.file = folder.getNonexistentChildFile (legal, ".wav", false);

    if (const auto error = AudioFileIO::save (audio, sampleRate, filed.file, 24, formats); error.isNotEmpty())
    {
        filed.error = error;
        filed.file = juce::File();
        return filed;
    }

    // The category and tags in the file too, where other apps read them.
    const auto* entry = Ucs::byCatId (catId);

    if (entry != nullptr || ! tags.isEmpty())
        if (const auto error = SoundMetadata::write (filed.file, { entry != nullptr ? entry->catId : juce::String(), tags }); error.isNotEmpty())
        {
            filed.error = "The sound was written, but its category and tags couldn't go into the file: " + error;
            return filed;
        }

    const auto sound = index.addFile (filed.file, formats);

    if (! sound.has_value())
    {
        filed.error = "The sound was written, but the Sound Library couldn't read it: " + index.getError();
        return filed;
    }

    filed.id = sound->id;
    const std::vector<juce::int64> ids { filed.id };

    if (entry != nullptr)
        index.setCategory (ids, entry->catId);

    for (auto& tag : tags)
        index.addTag (ids, tag);

    return filed;
}

juce::StringArray tagsFrom (const juce::String& line)
{
    juce::StringArray tags;

    for (auto& part : juce::StringArray::fromTokens (line, ",", "\""))
        if (const auto tag = part.trim(); tag.isNotEmpty() && ! tags.contains (tag, true))
            tags.add (tag);

    return tags;
}
}
