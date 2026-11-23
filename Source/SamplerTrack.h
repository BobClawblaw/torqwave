#pragma once

#include "Sampler.h"
#include "Session.h"

/** Making a sampler track from a sound that is already in Torqwave: the editor's selection, or a
    session's clip. What the menu commands and Ask Torq do, apart from the main window. */
namespace SamplerTrack
{
    /** A sound to sample: a file, and the part of it wanted, in the file's own frames (empty: all of it). */
    struct Sound
    {
        juce::File file;
        juce::Range<juce::int64> part;
        juce::String name;
    };

    /** A clip's sound: its file, and the part of it the clip plays, in the file's own frames (the
        session may hold the audio at another rate). Nothing, with the reason, for a clip that
        isn't there, has no file, or whose file can't be read. The clip's gain, fades, effects
        and tempo-following are not part of it: a sampler plays the file. */
    std::optional<Sound> soundOf (const SessionState&, juce::uint32 clipId, juce::String& error);

    /** Audio that is not a file as it stands (a selection of a file with changes, or one never
        saved), written into the Samples folder (or `folder`) to be sampled, with its markers. */
    std::optional<Sound> written (const juce::AudioBuffer<float>&, double rate, const juce::String& name, const Markers&, juce::String& error,
                                  const juce::File& folder = {});

    struct Made
    {
        juce::String error, name;
        std::shared_ptr<juce::XmlElement> effects;     // the new track's chain: Torqwave Sampler holding the sound
        std::vector<Instrument::Note> notes;           // sliced: the notes that play the slices as the sound was
        std::vector<DrumSound> drumMap;                // sliced: a row for each slice
        double seconds = 0.0;                          // the sound's length
    };

    /** A sampler holding the sound. Across the keys: one zone over every key, its root and tuning
        found from the part (or taken from the file's sampler chunk when the part is all of it,
        with its loop). Sliced: cut at the markers in the part when it has any, else at its hits,
        at full level, so the notes play the sound as it was. */
    Made make (const Sound&, bool slices);

    struct Placed
    {
        juce::uint32 trackId = 0, clipId = 0;
    };

    /** Adds an instrument track for it at the end of the session, named for the sound. Sliced, the
        track shows the drum grid with a row for each slice, and gets a note clip at `position`
        (a session frame) playing the slices as the sound was; when the sound came from a clip
        (`from`, or 0), that clip is muted, so the session sounds as before. The caller loads
        made.effects onto the track. */
    Placed place (SessionState&, const Made&, juce::int64 position, juce::uint32 from = 0);
}
