#pragma once

#include "Session.h"
#include "SoundLibrary.h"

/** Sound design in the session: editing a clip's audio in place in the editor, combining stacked
    clips into one sound, and filing a sound in the Sound Library. */
namespace SoundDesign
{
    /** What Edit in Place opens in the editor for a clip, linked to it.
        - A clip that plays at its own speed opens what it plays (its active take where and as
          long as it plays it, or its comp), before its effects, fades and gain. Saving makes that
          a new take, in use (SessionEdits::addEditedTake), so what it played stays a take away.
        - A clip that follows the tempo opens the part of its original it plays (the audio as it
          is in its file, unstretched). Saving puts the part back into a new original, which is
          stretched again (SessionEdits::replaceOriginal): the part must keep its length, since the
          clip's warp markers and place are frames of the original. */
    struct ClipEdit
    {
        juce::uint32 clipId = 0;
        juce::String clipName;
        AudioDocument::Snapshot audio;     // what opens; null when it can't (error says why)
        bool original = false;             // a part of a following clip's original
        juce::int64 start = 0;             // with original: the part's first frame in it
        AudioDocument::Snapshot from;      // with original: the original it is a part of
        juce::String error;
    };

    ClipEdit editClip (const SessionState&, juce::uint32 clipId);

    /** The audio to write to a file to save an edit (at the session's rate): the edited audio, or
        for an original's part the whole original with the part replaced. Null, with the reason in
        error, for no audio or a part whose length changed. */
    AudioDocument::Snapshot toSave (const ClipEdit&, const AudioDocument::Snapshot& edited, juce::String& error);

    /** Puts a saved edit (toSave()'s audio, written as file) into the clip; false, with the reason
        in error, when the clip is gone, locked, or (for an original) its original changed since. */
    bool putBack (SessionState&, const ClipEdit&, const juce::File&, AudioDocument::Snapshot saved, juce::String& error);

    //==============================================================================
    /** What a clip plays, as long as it is from its start: its audio through its effects (their
        render; dry while it isn't there), its fades, gain line and gain. Null with no audio. */
    AudioDocument::Snapshot clipSound (const SessionClip&);

    /** Combine to New Sound: the session cut to these clips (audio clips), each on a copy of its
        track as it is set (volume, pan, automation, effects, ducking), playing into a stereo mix:
        no other clips, no sends, buses, folders or VCA faders, nothing muted or soloed but the
        clips muted themselves, and the keys they listen to (SessionEdits::addKeys). Mixed over
        combineRange() it gives what the stack plays. */
    SessionState combineSource (const SessionState&, const std::vector<juce::uint32>& clipIds);

    /** From the first of the clips' starts to the last of their ends; empty for none. */
    juce::Range<juce::int64> combineRange (const SessionState&, const std::vector<juce::uint32>& clipIds);

    /** Puts a combined sound (written as file) into the session: one clip of it at `position`, on
        a new track after the last of the clips' tracks, named so; the clips are muted, so the
        session sounds as before. Returns the new clip's id, or 0 when none of the clips is left. */
    juce::uint32 landCombined (SessionState&, const std::vector<juce::uint32>& clipIds, const juce::File&, AudioDocument::Snapshot,
                               juce::int64 position, const juce::String& name);

    //==============================================================================
    /** A sound filed in the Sound Library: its file and its id in the index, or why not. */
    struct Filed
    {
        juce::File file;
        juce::int64 id = 0;
        juce::String error;
    };

    /** Add to Library: writes a sound as a 24-bit WAV file named `name` in `folder` (made when it
        isn't there, and watched when no watched folder holds it), writes its UCS category (a
        CatID; empty for none) and tags into the file (SoundMetadata), and puts it in the index
        with them, its category set by hand. */
    Filed addToLibrary (SoundLibrary::Index&, const AudioSnapshot&, double sampleRate, const juce::File& folder, const juce::String& name,
                        const juce::String& catId, const juce::StringArray& tags, juce::AudioFormatManager&);

    /** Tags typed as one line, split at commas, trimmed, empty ones and repeats (in any case) left out. */
    juce::StringArray tagsFrom (const juce::String&);
}
