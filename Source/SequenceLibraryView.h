#pragma once

#include "SequenceLibrary.h"
#include "SequencePreview.h"

/** The Sequence Library panel: the library's roles, sub-roles and the user's folders as a tree,
    each sequence with its bars and key, and a search box that shows the sequences matching
    every word. A click plays a sequence through its instrument at the session's tempo (from the
    session's next bar while it plays), and moving through the list with the arrow keys plays each
    in turn; a second click or Stop stops it. A sequence is dragged into the multitrack view (or to
    the Finder); a double-click puts it in the session at the cursor. Right-click for Show in
    Finder, Rename and Move to Trash. New... starts a new one. */
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
        the Inbox): the folder selected in the tree, or the one holding the selected sequence. */
    std::function<void (const juce::String& folder)> onNew;

    /** The role and sub-role of what is selected, as onNew gives them. */
    juce::String selectedFolder() const;

    /** The player for clicks (not owned); none leaves clicks silent. */
    void setPreview (SequencePreview*);

    /** Scans the folder again (also done every few seconds while shown, when it changed). */
    void refresh();

    /** What a drag of this sequence describes, and the file a description names (or none). */
    static juce::var dragDescriptionFor (const juce::File&);
    static juce::File fileOfDrag (const juce::var& description);

    /** For tests: the sequences shown, in order. */
    std::vector<juce::File> shownFiles() const;
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
