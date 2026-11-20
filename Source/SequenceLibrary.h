#pragma once

#include "SequenceFile.h"

/** The Sequence Library: a folder of sequences (.tqseq), its first two levels the fixed roles and
    their sub-roles (Drums > Hats & Tops), and below them the user's own folders. The files are
    plain, so they can be moved, copied and shared in the Finder; where a file is says its role. */
class SequenceLibrary
{
public:
    struct Role
    {
        juce::String name;
        juce::StringArray subRoles;
    };

    /** The roles, in the order shown: Drums, Bass, Harmony, Pads, Arps, Melody, SFX, Kits, Inbox. */
    static const std::vector<Role>& roles();

    /** ~/Music/Torqwave/Sequences. */
    static juce::File defaultFolder();

    explicit SequenceLibrary (juce::File folder = defaultFolder());

    const juce::File& getFolder() const noexcept   { return folder; }

    /** Makes the role and sub-role folders that are missing. False if the library's folder can't be made. */
    bool makeFolders() const;

    /** The folder for a role and sub-role (the role's own folder with no sub-role, Inbox for an unknown role). */
    juce::File folderFor (const juce::String& role, const juce::String& subRole) const;

    /** A file for a new sequence of this name there, not one already used ("Name 2" and on). */
    juce::File newFileFor (const juce::String& role, const juce::String& subRole, const juce::String& name) const;

    struct Entry
    {
        juce::File file;
        juce::String name, key;
        juce::StringArray path;    // the folders from the library down to it: role, sub-role, the user's
        juce::StringArray tags;
        double bars = 0.0;
        bool hasInstrument = false;   // its voice starts with an instrument plug-in
    };

    /** Every sequence under the folder, sorted by path and then name. Files that are not
        sequences are skipped. */
    std::vector<Entry> scan() const;

    /** Whether an entry matches a search: every word in its name, key, tags or folders. */
    static bool matches (const Entry&, const juce::String& search);

private:
    juce::File folder;
};
