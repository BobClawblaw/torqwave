#pragma once

#include "SequenceKit.h"

/** The Sequence Library: a folder of sequences (.tqseq) and kits (.tqkit), its first two levels
    the fixed roles and their sub-roles (Drums > Hats & Tops), and below them the user's own
    folders. The files are plain, so they can be moved, copied and shared in the Finder; where a
    file is says its role. A sequence's rating, favourite mark and the time it was added are kept
    in its own file, so they go wherever the file goes. */
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

    /** The folder chosen in Settings (setChosenFolder), or with none the standard one. */
    static juce::File defaultFolder();

    /** ~/Music/Torqwave/Sequences. */
    static juce::File standardFolder();

    /** The folder defaultFolder gives from now on (none: the standard one), as Settings sets it. */
    static void setChosenFolder (const juce::File&);

    /** Factory presets: sequences and kits that ship inside the app, read-only, shown in the
        library under Presets. They are unpacked from the app into a folder of their own, one for
        each set of presets the app has shipped, so a later version replaces them without
        touching the user's library. Their ratings and favourite marks are kept beside them, in
        factoryRatingsFile, not in their files. */
    static constexpr const char* factoryName = "Presets";
    static juce::File factoryFolder();
    static void setFactoryFolder (const juce::File&);   // for tests; none goes back to the app's own
    static bool isFactory (const juce::File&);
    static juce::File factoryRatingsFile();

    /** Unpacks a zip of presets into `into` when they aren't there yet, their files made
        read-only, and removes any other set beside it. Returns an error, or empty. */
    static juce::String unpackPresets (const void* zip, size_t size, const juce::File& into);

    /** Unpacks the presets the app ships into factoryFolder when needed. Returns an error, or empty. */
    static juce::String unpackFactoryPresets();

    explicit SequenceLibrary (juce::File folder = defaultFolder());

    const juce::File& getFolder() const noexcept   { return folder; }

    /** Makes the role and sub-role folders that are missing. False if the library's folder can't be made. */
    bool makeFolders() const;

    /** The folder for a role and sub-role (the role's own folder with no sub-role, Inbox for an unknown role). */
    juce::File folderFor (const juce::String& role, const juce::String& subRole) const;

    /** A file for a new sequence (or with that extension, a kit) of this name there, not one
        already used ("Name 2" and on). */
    juce::File newFileFor (const juce::String& role, const juce::String& subRole, const juce::String& name) const;
    static juce::File newFileIn (const juce::File& dir, const juce::String& name, const char* extension = SequenceFile::extension);

    /** Every folder, as the folders from the library down to it: each role, then its sub-roles in
        their order, then the user's folders on disk (by name) below them, each after its parent.
        Folders not under a role are left out. */
    std::vector<juce::StringArray> folders() const;

    /** The folders as the dialogs list them, "Drums > Hats & Tops > My Hats", the Inbox first. */
    juce::StringArray folderNames() const;

    /** A folder's path from such a name, and back. */
    static juce::StringArray pathOf (const juce::String& folderName);
    static juce::String nameOf (const juce::StringArray& path);

    /** The folder at a path (the Inbox when its first part is not a role). */
    juce::File folderAt (const juce::StringArray& path) const;

    /** Sets a sequence's role and sub-role to those of the folder at a path (the Inbox when it is not under a role). */
    static void setRoleFrom (SequenceFile::Sequence&, const juce::StringArray& path);

    /** Whether a path is one of the fixed roles or sub-roles, which can't be renamed or removed. */
    static bool isFixed (const juce::StringArray& path);

    /** Moves (or with copy, copies) a sequence file into the folder at `path`, its role and
        sub-role set to that folder's, under a name not used there ("Name 2" and on); a kit goes
        as it is. Everything else in the file is kept as it was. Returns the new file, or none
        with an error. A file already there stays as it is. */
    juce::File moveInto (const juce::File& sequence, const juce::StringArray& path, bool copy, juce::String& error) const;

    /** Makes a folder of this name in the folder at `parent`. Returns an error, or empty. */
    juce::String makeFolder (const juce::StringArray& parent, const juce::String& name) const;

    struct Entry
    {
        juce::File file;
        juce::String name, key;
        juce::StringArray path;    // the folders from the library down to it: role, sub-role, the user's
        juce::StringArray tags;
        double bars = 0.0;
        bool hasInstrument = false;   // its voice starts with an instrument plug-in (a kit: any part's)
        bool isKit = false;
        int parts = 0;                // a kit's
        int rating = 0;               // 0 to 5
        bool favourite = false;
        juce::int64 added = 0;        // ms since 1970
        bool factory = false;         // a factory preset
        juce::String style;           // "Techno/Dark", or empty
        juce::String description;     // its note: one line on what it does
        int energy = 0;               // 1 to 5, or 0
        juce::StringArray character;  // poles of the character pairs
        double tempo = 0.0;           // as written (a kit: its first part's)
    };

    /** Every sequence and kit under the folder, sorted by path and then name. Files that are
        not sequences or kits are skipped. */
    std::vector<Entry> scan() const;

    /** Whether an entry matches a search: every word in its name, key, tags, folders, style or description. */
    static bool matches (const Entry&, const juce::String& search);

    /** How the panel orders the sequences in each folder. */
    enum class Sort { name, bars, rating, newest, tempo, key, energy };
    static juce::StringArray sortNames();   // in Sort's order: "Name", "Bars", "Rating", "Newest", "Tempo", "Key", "Energy"

    /** Orders entries by path, then within a folder by name (A to Z), bars (fewest first),
        rating (highest first), newest (the latest added first), tempo (slowest first), key (by
        its root from C, then its scale; none last) or energy (lowest first; none last), then by
        name. With `nearEnergy` (1 to 5), the energies nearest it come first instead. */
    static void sort (std::vector<Entry>&, Sort, int nearEnergy = 0);

    /** How the panel groups them: in their folders (by role), by style, by key or by energy. */
    enum class Group { role, style, key, energy };
    static juce::StringArray groupNames();   // in Group's order: "Role", "Style", "Key", "Energy"

    /** The folders an entry shows in when grouped: by role its own folders; otherwise the
        style ("Techno › Dark", "No Style"), key ("A minor", "No Key") or energy ("Energy 4",
        "No Energy") and then its role. */
    static juce::StringArray groupPath (const Entry&, Group);

    /** What the panel narrows the library to. Empty: everything. */
    struct Filters
    {
        enum class Source { both, presets, mine };

        juce::String search, style, key, role;   // a style "Techno/Dark" or a genre "Techno"; a key as written; a role
        int energy = 0;                          // 1 to 5, or any
        int bars = 0;                            // 1, 2, 4, 8, or any
        juce::StringArray character;             // every one of these poles
        Source source = Source::both;
        bool favourites = false, fitsSession = false;
        int nearEnergy = 0;                      // Similar: the energies nearest this first

        bool isEmpty() const;
        bool operator== (const Filters&) const;
        std::unique_ptr<juce::XmlElement> toXml() const;
        static Filters fromXml (const juce::XmlElement&);

        /** What it narrows to, in words ("Techno › Dark · Energy 4 · Presets"), or empty. */
        juce::String describe() const;
    };

    /** Whether an entry passes the filters (with the session's key for Fits the Session). */
    static bool passes (const Entry&, const Filters&, const juce::String& sessionKey);

    /** The styles the entries carry, sorted ("Industrial/EBM", "Techno/Dark"). */
    static juce::StringArray stylesIn (const std::vector<Entry>&);

    /** The keys they carry, as each names itself, from C ("A minor"). */
    static juce::StringArray keysIn (const std::vector<Entry>&);

    /** Similar: the same role, style and key, nearest energy first. */
    static Filters similarTo (const Entry&);

    /** Saved searches, by name, in the order saved, kept in searchesFile. */
    static juce::File searchesFile();
    static void setSearchesFile (const juce::File&);   // for tests; none goes back to the app's own
    static std::vector<std::pair<juce::String, Filters>> savedSearches();
    static juce::String saveSearch (const juce::String& name, const Filters&);   // replaces one of that name; an error, or empty
    static juce::String removeSearch (const juce::String& name);

    /** Whether a drop moves a sequence into the session's key (SequenceFile::transposedTo); off by default. */
    static bool transposesDrops();
    static void setTransposesDrops (bool);

    /** What Sequence Properties sets on a sequence or kit (a kit's energy and character are left alone). */
    struct Properties
    {
        juce::String style, key, description;
        int energy = 0;
        juce::StringArray character, tags;
    };

    static Properties propertiesOf (const Entry&);

    /** Writes them into the file, everything else as it was. Refused for a factory preset. Returns an error, or empty. */
    static juce::String setProperties (const juce::File&, const Properties&);

    /** Whether an entry fits a session in this key: its key is the same (the same root and
        scale, however written: "Am" is "A minor"), or it has no key (drums, effects). Everything
        fits a session with no key. */
    static bool fitsKey (const Entry&, const juce::String& sessionKey);

    /** Sets a sequence's or kit's rating (0 to 5) or favourite mark in its file, everything else
        in it as it was (a factory preset's in factoryRatingsFile). Returns an error, or empty. */
    static juce::String setRating (const juce::File&, int rating);
    static juce::String setFavourite (const juce::File&, bool favourite);

    /** Renames a sequence or kit: its name inside and its file's. Returns the new file, or none
        with an error (a clash with another file there, or one that can't be written). */
    static juce::File rename (const juce::File&, const juce::String& name, juce::String& error);

    /** Saves a kit of these sequence files, named `name`, in the Kits folder under a name not
        used there. Returns the kit's file, or none with an error. */
    juce::File saveKit (const juce::String& name, const juce::Array<juce::File>& sequences, juce::String& error) const;

    /** Every sequence (not kit) file in a folder and the folders below it, by path and name. */
    juce::Array<juce::File> sequencesIn (const juce::StringArray& path) const;

    /** Imports Standard MIDI Files into the folder at `path`: each track with notes becomes a
        sequence there (SequenceMidi::read), playing on `voice` (null: on the instrument of the
        track it is dropped on), its role and sub-role the folder's. Returns the files made;
        files that can't be read add to `errors`. */
    juce::Array<juce::File> importMidi (const juce::Array<juce::File>& files, const juce::StringArray& path,
                                        std::shared_ptr<const juce::XmlElement> voice, juce::StringArray& errors) const;

    /** Packs: a folder's sequences and kits (and those in the folders below it) zipped into one
        file to share, each under its path from the library ("Drums/Kick/Thump.tqseq").
        Returns an error, or empty; `count` is how many went in. */
    static constexpr const char* packExtension = ".tqpack";
    juce::String savePack (const juce::StringArray& path, const juce::File& pack, int& count) const;

    /** Opens a pack (or any zip of sequences) into the library: each sequence into the folder of
        its role and sub-role (the Inbox with none or one not known), each kit into Kits, under its
        own file name, numbered when one of that name is there ("Name 2"). Files in it that are
        neither are skipped. Returns the files made; what couldn't be read adds to `errors`. */
    juce::Array<juce::File> openPack (const juce::File& pack, juce::StringArray& errors) const;

private:
    juce::File folder;
};
