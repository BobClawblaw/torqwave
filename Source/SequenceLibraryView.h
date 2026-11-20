#pragma once

#include "SequenceLibrary.h"
#include "SequencePreview.h"

/** The Sequence Library panel: the library's roles, sub-roles and the user's folders as a tree,
    each sequence and kit with its bars, key and rating, and a search box that shows the ones
    matching every word. Below it the order in each folder (name, bars, rating, newest) and two
    filters: favourites only, and those that fit the session's key. A click plays a sequence (a
    kit: all its parts) through its instrument at the session's tempo (from the session's next bar
    while it plays), and moving through the list with the arrow keys plays each in turn; a second
    click or Stop stops it. A sequence or kit is dragged into the multitrack view (or to the
    Finder); a double-click puts it in the session at the cursor. 0 to 5 rate the selected ones and
    F marks them as favourites. Right-click for Export MIDI File, Rating, Favourite, Make Kit from
    the selected sequences, Show in Finder, Rename and Move to Trash; right-click a folder to make
    a folder, a sequence or a kit in it, import MIDI files into it or save it as a pack. Import...
    brings in MIDI files or opens a pack. Every folder shows, empty ones too. New... starts a new
    sequence. */
class SequenceLibraryView final : public juce::Component,
                                  public juce::DragAndDropContainer,
                                  private juce::Timer
{
public:
    explicit SequenceLibraryView (juce::File folder = SequenceLibrary::defaultFolder());
    ~SequenceLibraryView() override;

    /** Asked by a double-click: put this sequence in the session at the cursor. */
    std::function<void (const juce::File&)> onUse;

    /** Asked by Edit in Sequencer on a sequence's menu: open it in the Sequencer window. */
    std::function<void (const juce::File&)> onEdit;

    /** The session's key ("A minor", or empty), for Fits the Session. */
    std::function<juce::String()> getSessionKey;

    /** The instruments a MIDI file's sequences can be given, for Import MIDI Files. */
    std::function<juce::Array<juce::PluginDescription>()> getInstruments;

    /** Asked by New: start a new sequence, in this folder ("Drums > Kick", "Pads", or empty for
        the Inbox): the folder selected in the tree, or the one holding the selected sequence, or
        the folder right-clicked for New Sequence Here. */
    std::function<void (const juce::String& folder)> onNew;

    /** The folder of what is selected, as onNew gives it ("Drums > Kick > Mine"), or empty. */
    juce::String selectedFolder() const;

    /** Right-click on a folder: New Sequence Here, New Folder, Show in Finder, and for the user's
        own folders Rename and Move to Trash. */
    void folderMenu (const juce::StringArray& path);

    /** Moves the sequence files into the folder at path (copy: copies them, as from the Finder
        or with Option), then shows the first where it went. Errors are shown. */
    void dropInto (const juce::StringArray& path, const juce::StringArray& files, bool copy);

    /** Opens the folders down to a sequence and selects it (without playing it). */
    void select (const juce::File&);

    /** The player for clicks (not owned); none leaves clicks silent. */
    void setPreview (SequencePreview*);

    /** Scans the folder again (also done every few seconds while shown, when it changed). */
    void refresh();

    /** What a drag of this sequence describes, and the file a description names (or none). */
    static juce::var dragDescriptionFor (const juce::File&);
    static juce::File fileOfDrag (const juce::var& description);

    /** For tests: the sequences shown, in order. */
    std::vector<juce::File> shownFiles() const;
    juce::StringArray shownFolders() const;   // every folder in the tree, as "Drums > Kick"

    void setSearch (const juce::String&);
    void setSort (SequenceLibrary::Sort);
    void setFavouritesOnly (bool);
    void setFitsSession (bool);

    /** Shows the library in another folder (the one chosen in Settings). */
    void setFolder (const juce::File&);
    const juce::File& getFolder() const noexcept   { return library.getFolder(); }

    /** The sequences and kits selected, in the tree's order. */
    juce::Array<juce::File> selectedFiles() const;

    /** Rates the selected sequences and kits (0 clears it), or marks them all as favourites (or,
        when every one is, unmarks them). Errors are shown. */
    void rateSelected (int rating);
    void toggleFavouriteSelected();

    bool keyPressed (const juce::KeyPress&) override;

    void resized() override;
    void paint (juce::Graphics&) override;

    bool shouldDropFilesWhenDraggedExternally (const juce::DragAndDropTarget::SourceDetails&, juce::StringArray& files, bool& canMoveFiles) override;

    /** The Settings row for the library's folder: where it is, Choose... to pick another, and
        Standard to go back to ~/Music/Torqwave/Sequences. `onChange` gets the folder chosen
        (none for the standard one). */
    class FolderSetting final : public juce::Component
    {
    public:
        explicit FolderSetting (std::function<void (const juce::File&)> onChange);
        void resized() override;

    private:
        void show();

        std::function<void (const juce::File&)> changed;
        juce::Label where;
        juce::TextButton choose { "Choose..." }, standard { "Standard" };
        std::unique_ptr<juce::FileChooser> chooser;
    };

    /** Asks for a name, then saves these sequences as a kit in Kits and selects it. */
    void askToMakeKit (const juce::Array<juce::File>& sequences, const juce::String& suggestedName);

    /** Asks where, then writes the sequence (or each part of a kit) as a Standard MIDI File. */
    void askToExportMidi (const juce::File&);

    /** Asks for MIDI files, then for the folder and the instrument, and imports them (`folder`:
        the one offered first). */
    void askToImportMidi (const juce::String& folder);

    /** Asks where, then zips the folder at path into a pack. */
    void askToSavePack (const juce::StringArray& path);

    /** Asks for a pack, or opens this one, into the library. */
    void askToOpenPack();
    void openPack (const juce::File&);

private:
    class FolderItem;
    class SequenceItem;

    void rebuild();
    void showImportMenu();
    void clicked (const juce::File&);
    void selected (const juce::File&);
    void previewChanged();
    bool isPlaying (const juce::File&) const;
    void timerCallback() override;
    void visibilityChanged() override;
    juce::int64 folderStamp() const;

    SequenceLibrary library;
    std::vector<SequenceLibrary::Entry> entries;
    std::vector<juce::StringArray> folderPaths;
    juce::int64 stamp = 0;
    juce::TextEditor search;
    juce::TextButton newButton { "New..." }, folderButton { "Folder" }, stopButton { "Stop" };
    juce::ComboBox sortBox;
    juce::TextButton favouritesButton { "Favourites" }, fitsButton { "Fits the Session" }, importButton { "Import..." };
    SequenceLibrary::Sort sortBy = SequenceLibrary::Sort::name;
    juce::String shownKey;                    // the session's key the list was filtered by
    std::unique_ptr<juce::FileChooser> chooser;
    SequencePreview* preview = nullptr;
    juce::String countText;
    juce::File startedBySelecting;   // a click's selection started it, so the click doesn't stop it
    juce::TreeView tree;
    std::unique_ptr<FolderItem> root;
    std::unique_ptr<juce::XmlElement> openness;   // the folders opened by hand
    bool shownNarrowed = false;               // a search or filter chose what the tree shows
    juce::Label summary;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceLibraryView)
};
