#pragma once

#include <JuceHeader.h>
#include "Markers.h"

/** Sounds dragged from inside Torqwave (an open file from the Files panel) onto an instrument's
    window, which knows nothing of the app's documents: the app says whether a drag is a sound and
    hands over its audio. */
namespace SoundDrops
{
    struct Sound
    {
        juce::AudioBuffer<float> audio;
        double sampleRate = 48000.0;
        juce::String name;

        // For a taker that wants the file itself (the sampler): the file the sound is, when it is a
        // saved file as it stands (none for one with changes or never saved), and its markers.
        juce::File file;
        Markers markers;
    };

    /** Whether a drag's description is a sound the app can hand over. Unset: nothing is. */
    extern std::function<bool (const juce::var& description)> isSound;

    /** The first `maxSeconds` of the sound a drag describes; nullopt if it has none. */
    extern std::function<std::optional<Sound> (const juce::var& description, double maxSeconds)> soundOf;

    inline bool accepts (const juce::var& description)      { return isSound != nullptr && isSound (description); }

    inline std::optional<Sound> take (const juce::var& description, double maxSeconds)
    {
        return soundOf != nullptr ? soundOf (description, maxSeconds) : std::nullopt;
    }
}
