#pragma once

#include "SequenceFile.h"

class AudioEngine;

/** Kits (.tqkit): sequences meant to play together (a groove's kick, snare and hats, or a bass
    with its chords), each a part kept whole in the kit's file with its notes and voice, so a kit
    is one file to move or share. Dropped into a session, a kit makes a track for each part under
    a new folder track named for it. */
namespace SequenceKit
{
    inline constexpr const char* extension = SequenceFile::kitExtension;

    struct Kit
    {
        juce::String name, key, note;                  // key: the parts' key when they share one, or empty
        juce::String style;                            // as a sequence's
        juce::StringArray tags;
        int rating = 0;                                // as a sequence's
        bool favourite = false;
        juce::int64 added = 0;
        std::vector<SequenceFile::Sequence> parts;

        /** The longest part's bars. */
        double bars() const noexcept;
    };

    std::unique_ptr<juce::XmlElement> toXml (const Kit&);

    /** Null when the XML is not a kit. Parts that are not sequences are left out. */
    std::optional<Kit> fromXml (const juce::XmlElement&);

    /** Writes it safely, stamped as SequenceFile::save stamps a sequence. Returns an error, or empty. */
    juce::String save (const Kit&, const juce::File&);

    /** Reads one, or gives an error. */
    std::optional<Kit> load (const juce::File&, juce::String& error);

    /** A kit named `name` of these sequence files, in their order, its key the one their keys
        agree on ("Em" and "E minor" agree; parts with no key are left out of it), or none when
        they don't. Nothing, with an error, when a file is not a sequence or there are none. */
    std::optional<Kit> fromFiles (const juce::String& name, const juce::Array<juce::File>&, juce::String& error);

    /** Where a dropped kit went: its folder track, and each part's track and clip. */
    struct Placed
    {
        juce::uint32 folderId = 0;
        std::vector<SequenceFile::Placed> parts;
    };

    /** Puts a kit into a session at `position`: a folder track named for it at the end of the
        session, and each part on a new track of its own (from its voice, named for the part, as
        SequenceFile::place makes one) in that folder, every part from the same position. */
    Placed place (SessionState&, const Kit&, juce::int64 position);

    /** place() into the engine's session as one undo step ("Add Kit name"), then each part's
        instrument and effects loaded onto its track; `done` is called with the errors of that
        load (empty when every one loaded), or at once when no part has a voice. */
    Placed addToSession (AudioEngine&, const Kit&, juce::int64 position, std::function<void (const juce::String& errors)> done);
}
