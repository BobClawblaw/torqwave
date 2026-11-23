#include "SamplerTrack.h"

namespace SamplerTrack
{
std::optional<Sound> soundOf (const SessionState& s, juce::uint32 clipId, juce::String& error)
{
    const auto where = s.findClip (clipId);

    if (! where.has_value())
    {
        error = "There is no such clip.";
        return std::nullopt;
    }

    const auto& clip = s.tracks[(size_t) where->first].clips[(size_t) where->second];

    if (clip.source == juce::File())
    {
        error = (clip.name.isNotEmpty() ? clip.name : juce::String ("The clip")) + " has no file to sample.";
        return std::nullopt;
    }

    const auto sample = Sampler::sharedSample (clip.source, error);

    if (sample == nullptr)
        return std::nullopt;

    // The clip's frames are the session's; the sampler reads the file at its own rate.
    const auto ratio = s.sampleRate > 0.0 ? sample->rate / s.sampleRate : 1.0;
    const auto frames = (juce::int64) sample->getNumFrames();
    const auto start = juce::jlimit ((juce::int64) 0, frames, (juce::int64) std::llround ((double) clip.offset * ratio));
    const auto end = juce::jlimit (start, frames, (juce::int64) std::llround ((double) (clip.offset + clip.length) * ratio));

    if (end <= start)
    {
        error = (clip.name.isNotEmpty() ? clip.name : juce::String ("The clip")) + " plays nothing of its file.";
        return std::nullopt;
    }

    return Sound { clip.source, { start, end }, clip.name.isNotEmpty() ? clip.name : clip.source.getFileNameWithoutExtension() };
}

std::optional<Sound> written (const juce::AudioBuffer<float>& audio, double rate, const juce::String& name, const Markers& markers, juce::String& error,
                              const juce::File& folder)
{
    const auto file = Sampler::writeSample (audio, rate, name, markers, error, folder);

    if (file == juce::File())
        return std::nullopt;

    return Sound { file, {}, name };
}

Made make (const Sound& sound, bool slices)
{
    Made made;
    made.name = sound.name.isNotEmpty() ? sound.name : sound.file.getFileNameWithoutExtension();
    const auto sample = Sampler::sharedSample (sound.file, made.error);

    if (sample == nullptr)
        return made;

    const auto frames = (juce::int64) sample->getNumFrames();
    const auto part = sound.part.isEmpty() ? juce::Range<juce::int64> (0, frames) : sound.part.getIntersectionWith ({ 0, frames });
    const auto whole = part.getStart() == 0 && part.getEnd() == frames;
    made.seconds = (double) part.getLength() / sample->rate;

    BuiltInEffects::SamplerProcessor sampler;

    if (const auto room = sampler.roomFor (sample); room.isNotEmpty())
    {
        made.error = room;
        return made;
    }

    if (slices)
    {
        Sampler::Slicing how;
        how.source = Sampler::SliceSource::markers;

        if (Sampler::findSlices (*sample, part, how).size() < 2)
            how.source = Sampler::SliceSource::hits;

        if (! sampler.slice (sample, sound.file, part, how))
        {
            made.error = made.name + " is too short to slice.";
            return made;
        }

        // At full level, so the notes play the sound as loud as it was.
        if (auto* level = sampler.parameter ("level"))
            level->setValueNotifyingHost (level->convertTo0to1 (0.0f));

        made.notes = Sampler::notesFromSlices (sampler.getZones());
        made.drumMap = Sampler::drumMapFor (sampler.getZones());
    }
    else
    {
        Sampler::Zone z;
        z.sample = sample;
        z.file = sound.file;
        z.name = made.name;
        z.start = part.getStart();
        z.end = whole ? -1 : part.getEnd();

        if (whole && sample->fileRoot >= 0)
        {
            z.root = sample->fileRoot;
            z.tune = sample->fileTune;

            if (sample->fileLoop != 0)
            {
                z.loop = sample->fileLoop;
                z.loopStart = sample->fileLoopStart;
                z.loopEnd = sample->fileLoopEnd;
            }
        }
        else
        {
            const auto pitch = Sampler::findPitch (*sample, part);
            z.root = pitch.root;
            z.tune = pitch.tune;
        }

        sampler.setZones ({ z });
    }

    made.effects = std::make_shared<juce::XmlElement> ("EFFECTS");
    auto* slot = made.effects->createNewChildElement ("SLOT");
    slot->setAttribute ("bypassed", false);
    slot->addChildElement (sampler.getPluginDescription().createXml().release());
    juce::MemoryBlock state;
    sampler.getStateInformation (state);
    slot->createNewChildElement ("STATE")->addTextElement (state.toBase64Encoding());
    return made;
}

Placed place (SessionState& s, const Made& made, juce::int64 position, juce::uint32 from)
{
    Placed placed;

    if (made.effects == nullptr)
        return placed;

    auto& track = s.addInstrumentTrack();
    track.name = made.name;
    placed.trackId = track.id;

    if (made.notes.empty())
        return placed;

    track.drumGrid = true;
    track.drumMap = made.drumMap;

    NoteClip clip;
    clip.name = made.name;
    clip.position = juce::jmax ((juce::int64) 0, position);
    clip.length = juce::jmax ((juce::int64) 1, (juce::int64) std::llround (made.seconds * s.sampleRate));

    for (auto& note : made.notes)
        clip.notes.push_back ({ note.pitch, (juce::int64) std::llround (note.start * s.sampleRate),
                                juce::jmax ((juce::int64) 1, (juce::int64) std::llround (note.hold * s.sampleRate)), note.velocity, 1.0f });

    placed.clipId = SessionEdits::addNoteClip (s, placed.trackId, std::move (clip));

    // The clip it came from stays where it is, muted, so the session sounds as it did.
    if (from != 0)
        if (const auto where = s.findClip (from))
            s.tracks[(size_t) where->first].clips[(size_t) where->second].muted = true;

    return placed;
}
}
