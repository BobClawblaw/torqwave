#pragma once

#include "SequenceLibrary.h"
#include "SequencePreview.h"

/** The Sequence Library panel: the library's roles, sub-roles and the user's folders as a tree,
    each sequence with its bars and key, and a search box that shows the sequences matching
    every word. A click plays a sequence through its instrument at the session's tempo (from the
    session's next bar while it plays), and moving through the list with the arrow keys plays each
    in turn; a second click or Stop stops it. A sequence is dragged into the multitrack view (or to
    the Finder); a double-click puts it in the session at the cursor. Right-click for Show in
    Finder, Rename and Move to Trash; right-click a folder to make a folder or a sequence in it.
    Every folder shows, empty ones too. New... starts a new sequence. */
class SequenceLibraryView final : public juce::Component,
                                  public juce::DragAndDropContainer,
                                  private juce::Timer
{
public:
    explicit SequenceLibraryView (juce::File folder = SequenceLibrary::defaultFolder());
    ~SequenceLibraryView() override;

    /** Asked by a double-click: put this sequence in the session at the cursor. */
    std::function<void (const juce::File&)> onUse;

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

    void resized() override;
    void paint (juce::Graphics&) override;

    bool shouldDropFilesWhenDraggedExternally (const juce::DragAndDropTarget::SourceDetails&, juce::StringArray& files, bool& canMoveFiles) override;

private:
    class FolderItem;
    class SequenceItem;

    void rebuild();
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
    SequencePreview* preview = nullptr;
    juce::String countText;
    juce::File startedBySelecting;   // a click's selection started it, so the click doesn't stop it
    juce::TreeView tree;
    std::unique_ptr<FolderItem> root;
    std::unique_ptr<juce::XmlElement> openness;   // the folders opened by hand
    juce::String shownSearch;
    juce::Label summary;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceLibraryView)
};
