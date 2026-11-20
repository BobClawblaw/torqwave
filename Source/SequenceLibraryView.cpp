#include "SequenceLibraryView.h"
#include "Theme.h"

namespace
{
    const juce::String dragPrefix { "tqseq:" };

    juce::String detailsOf (const SequenceLibrary::Entry& e)
    {
        juce::StringArray parts;
        const auto bars = e.bars;

        if (bars > 0.0)
            parts.add (juce::exactlyEqual (bars, std::round (bars)) ? juce::String ((int) bars) + (juce::exactlyEqual (bars, 1.0) ? " bar" : " bars")
                                                                    : juce::String (bars, 2) + " bars");

        if (e.key.isNotEmpty())
            parts.add (e.key);

        if (! e.hasInstrument)
            parts.add ("no instrument");

        return parts.joinIntoString (" · ");
    }
}

//==============================================================================
class SequenceLibraryView::SequenceItem final : public juce::TreeViewItem
{
public:
    SequenceItem (SequenceLibraryView& v, SequenceLibrary::Entry e) : view (v), entry (std::move (e)) {}

    bool mightContainSubItems() override              { return false; }
    juce::String getUniqueName() const override       { return entry.file.getFileName(); }
    int getItemHeight() const override                { return 22; }
    juce::var getDragSourceDescription() override     { return dragDescriptionFor (entry.file); }
    juce::String getTooltip() override                { return entry.tags.joinIntoString (", "); }

    void paintItem (juce::Graphics& g, int width, int height) override
    {
        if (isSelected())
            g.fillAll (Theme::selection);

        auto r = juce::Rectangle<int> (0, 0, width, height).reduced (4, 0);
        const auto details = detailsOf (entry);
        g.setFont (juce::FontOptions (12.0f));
        const auto detailWidth = juce::jmin (r.getWidth() / 2, juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), details) + 8);
        g.setColour (Theme::dimText);
        g.drawText (details, r.removeFromRight (detailWidth), juce::Justification::centredRight, true);
        g.setColour (isSelected() ? Theme::textStrong : Theme::text);
        g.setFont (juce::FontOptions (13.0f));
        g.drawText ((view.isPlaying (entry.file) ? juce::String::fromUTF8 ("\xe2\x96\xb6 ") : juce::String()) + entry.name, r, juce::Justification::centredLeft, true);
    }

    void itemDoubleClicked (const juce::MouseEvent&) override
    {
        if (view.onUse != nullptr)
            view.onUse (entry.file);
    }

    void itemSelectionChanged (bool isNowSelected) override
    {
        if (isNowSelected)
            view.selected (entry.file);
    }

    void itemClicked (const juce::MouseEvent& e) override
    {
        if (! e.mods.isPopupMenu())
        {
            view.clicked (entry.file);
            return;
        }

        juce::PopupMenu menu;
        menu.addItem (4, "Edit in Sequencer", view.onEdit != nullptr);
        menu.addSeparator();
        menu.addItem (1, "Show in Finder");
        menu.addItem (2, "Rename...");
        menu.addItem (3, "Move to Trash");
        const auto file = entry.file;
        juce::Component::SafePointer<SequenceLibraryView> safe (&view);

        menu.showMenuAsync (juce::PopupMenu::Options(), [safe, file] (int result)
        {
            if (safe == nullptr)
                return;

            if (result == 4 && safe->onEdit != nullptr)
                safe->onEdit (file);
            else if (result == 1)
                file.revealToUser();
            else if (result == 3 && file.moveToTrash())
                safe->refresh();
            else if (result == 2)
            {
                auto* box = new juce::AlertWindow ("Rename Sequence", {}, juce::MessageBoxIconType::NoIcon, safe.getComponent());
                box->addTextEditor ("name", file.getFileNameWithoutExtension(), "Name");
                box->addButton ("Rename", 1, juce::KeyPress (juce::KeyPress::returnKey));
                box->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
                box->enterModalState (true, juce::ModalCallbackFunction::create ([safe, file, box] (int chosen)
                {
                    const auto name = juce::File::createLegalFileName (box->getTextEditorContents ("name").trim());

                    if (chosen != 1 || safe == nullptr || name.isEmpty())
                        return;

                    const auto target = file.getSiblingFile (name + SequenceFile::extension);
                    juce::String error;

                    if (target.exists() && target != file)
                    {
                        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Rename Sequence", "There is already a sequence called " + name + " there.");
                        return;
                    }

                    if (auto q = SequenceFile::load (file, error))
                    {
                        q->name = name;
                        error = SequenceFile::save (*q, target);

                        if (error.isEmpty() && target != file)
                            file.deleteFile();
                    }

                    if (error.isNotEmpty())
                        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Rename Sequence", error);

                    safe->refresh();
                }), true);
            }
        });
    }

    const SequenceLibrary::Entry& getEntry() const noexcept { return entry; }

private:
    SequenceLibraryView& view;
    SequenceLibrary::Entry entry;
};

//==============================================================================
class SequenceLibraryView::FolderItem final : public juce::TreeViewItem
{
public:
    FolderItem (SequenceLibraryView& v, juce::String n) : view (v), name (std::move (n)) {}

    bool mightContainSubItems() override              { return true; }
    juce::String getUniqueName() const override       { return name; }
    int getItemHeight() const override                { return 22; }

    void paintItem (juce::Graphics& g, int width, int height) override
    {
        g.setColour (Theme::textStrong);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (name + (count > 0 ? "  (" + juce::String (count) + ")" : juce::String()), 4, 0, width - 8, height, juce::Justification::centredLeft, true);
    }

    void itemClicked (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
            view.folderMenu (path());
        else
            setOpen (! isOpen());
    }

    // Sequences dragged here from the panel move here (with Option, a copy); .tqseq files from the
    // Finder are copied here.
    bool isInterestedInDragSource (const juce::DragAndDropTarget::SourceDetails& details) override
    {
        return ! path().isEmpty() && fileOfDrag (details.description).existsAsFile();
    }

    void itemDropped (const juce::DragAndDropTarget::SourceDetails& details, int) override
    {
        view.dropInto (path(), { fileOfDrag (details.description).getFullPathName() },
                       juce::ModifierKeys::currentModifiers.isAltDown());
    }

    bool isInterestedInFileDrag (const juce::StringArray& files) override
    {
        return ! path().isEmpty() && std::any_of (files.begin(), files.end(), [] (auto& f) { return f.endsWithIgnoreCase (SequenceFile::extension); });
    }

    void filesDropped (const juce::StringArray& files, int) override
    {
        view.dropInto (path(), files, true);
    }

    /** The folders from the library down to this one (empty for the library itself). */
    juce::StringArray path() const
    {
        juce::StringArray names;

        for (const juce::TreeViewItem* item = this; item != nullptr && item->getParentItem() != nullptr; item = item->getParentItem())
            names.insert (0, item->getUniqueName());

        return names;
    }

    /** The child folder with this name, made when missing. */
    FolderItem& folder (const juce::String& child)
    {
        for (int i = 0; i < getNumSubItems(); ++i)
            if (auto* f = dynamic_cast<FolderItem*> (getSubItem (i)); f != nullptr && f->name == child)
                return *f;

        auto* f = new FolderItem (view, child);
        addSubItem (f);
        return *f;
    }

    SequenceLibraryView& view;
    const juce::String name;
    int count = 0;   // sequences in it and below
};

//==============================================================================
SequenceLibraryView::SequenceLibraryView (juce::File folder) : library (std::move (folder))
{
    library.makeFolders();

    addAndMakeVisible (search);
    search.setTextToShowWhenEmpty ("Search names, keys, tags and folders", Theme::dimText);
    search.onTextChange = [this] { rebuild(); };

    addAndMakeVisible (newButton);
    newButton.setTooltip ("Start a new sequence: an instrument track with an empty clip to write the notes in, then save it here");
    newButton.onClick = [this] { if (onNew != nullptr) onNew (selectedFolder()); };

    addAndMakeVisible (folderButton);
    folderButton.setTooltip ("Show the library's folder in the Finder: " + library.getFolder().getFullPathName());
    folderButton.onClick = [this] { library.getFolder().revealToUser(); };

    addAndMakeVisible (stopButton);
    stopButton.setTooltip ("Stop the sequence playing");
    stopButton.onClick = [this] { if (preview != nullptr) preview->stop(); };
    stopButton.setEnabled (false);

    addAndMakeVisible (tree);
    tree.setRootItemVisible (false);
    tree.setDefaultOpenness (false);
    Theme::follow (tree, juce::TreeView::backgroundColourId, Theme::surface);

    addAndMakeVisible (summary);
    Theme::follow (summary, juce::Label::textColourId, Theme::dimText);
    summary.setFont (juce::FontOptions (12.0f));

    refresh();
}

SequenceLibraryView::~SequenceLibraryView()
{
    setPreview (nullptr);
    tree.setRootItem (nullptr);
}

void SequenceLibraryView::setPreview (SequencePreview* p)
{
    if (preview != nullptr)
        preview->onChange = nullptr;

    preview = p;

    if (preview != nullptr)
        preview->onChange = [this] { previewChanged(); };

    previewChanged();
}

bool SequenceLibraryView::isPlaying (const juce::File& file) const
{
    return preview != nullptr && preview->getState() != SequencePreview::State::stopped && preview->getFile() == file;
}

void SequenceLibraryView::clicked (const juce::File& file)
{
    if (preview == nullptr)
        return;

    // The click that selected it has already started it.
    if (std::exchange (startedBySelecting, juce::File()) == file)
        return;

    // A second click on the one playing stops it.
    if (isPlaying (file))
        preview->stop();
    else
        preview->play (file);
}

void SequenceLibraryView::selected (const juce::File& file)
{
    // Moving through the list while one plays plays each in turn.
    if (preview != nullptr && preview->getState() != SequencePreview::State::stopped && preview->getFile() != file)
    {
        preview->play (file);
        startedBySelecting = file;
    }
}

void SequenceLibraryView::previewChanged()
{
    const auto said = preview != nullptr ? preview->describe() : juce::String();
    summary.setText (said.isNotEmpty() ? said : countText, juce::dontSendNotification);
    stopButton.setEnabled (preview != nullptr && preview->getState() != SequencePreview::State::stopped);
    tree.repaint();
}

juce::var SequenceLibraryView::dragDescriptionFor (const juce::File& file)
{
    return dragPrefix + file.getFullPathName();
}

juce::File SequenceLibraryView::fileOfDrag (const juce::var& description)
{
    const auto text = description.toString();
    return text.startsWith (dragPrefix) ? juce::File (text.substring (dragPrefix.length())) : juce::File();
}

bool SequenceLibraryView::shouldDropFilesWhenDraggedExternally (const juce::DragAndDropTarget::SourceDetails& details, juce::StringArray& files, bool& canMoveFiles)
{
    const auto file = fileOfDrag (details.description);

    if (! file.existsAsFile())
        return false;

    files.add (file.getFullPathName());
    canMoveFiles = false;
    return true;
}

juce::int64 SequenceLibraryView::folderStamp() const
{
    juce::int64 latest = 0, count = 0;

    for (const auto& found : juce::RangedDirectoryIterator (library.getFolder(), true, "*", juce::File::findFilesAndDirectories))
    {
        latest = juce::jmax (latest, found.getModificationTime().toMilliseconds());
        ++count;
    }

    return latest * 1000 + count;
}

void SequenceLibraryView::refresh()
{
    stamp = folderStamp();
    entries = library.scan();
    folderPaths = library.folders();
    rebuild();
}

void SequenceLibraryView::setSearch (const juce::String& text)
{
    search.setText (text, false);
    rebuild();
}

void SequenceLibraryView::rebuild()
{
    // The folders opened by hand are kept, not those a search opened.
    if (root != nullptr && shownSearch.isEmpty())
        openness = root->getOpennessState();

    tree.setRootItem (nullptr);
    root = std::make_unique<FolderItem> (*this, "Sequences");
    const auto text = search.getText().trim();

    // Every folder, empty ones too: the roles and their sub-roles in their order, then the user's
    // own below them. A search shows the roles and the folders holding what it finds.
    for (auto& path : text.isEmpty() ? folderPaths : std::vector<juce::StringArray> {})
    {
        auto* at = root.get();

        for (auto& part : path)
            at = &at->folder (part);
    }

    for (auto& role : SequenceLibrary::roles())
        root->folder (role.name);

    shownSearch = text;
    int shown = 0;

    for (auto& e : entries)
    {
        if (text.isNotEmpty() && ! SequenceLibrary::matches (e, text))
            continue;

        auto* at = root.get();
        ++at->count;

        for (auto& part : e.path)
        {
            at = &at->folder (part);
            ++at->count;
        }

        at->addSubItem (new SequenceItem (*this, e));
        ++shown;
    }

    tree.setRootItem (root.get());

    if (text.isEmpty() && openness != nullptr)
        root->restoreOpennessState (*openness);

    // With a search, every folder holding a match is open.
    if (text.isNotEmpty())
    {
        std::function<void (juce::TreeViewItem&)> openAll = [&] (juce::TreeViewItem& item)
        {
            item.setOpen (true);

            for (int i = 0; i < item.getNumSubItems(); ++i)
                openAll (*item.getSubItem (i));
        };

        openAll (*root);
    }

    countText = text.isEmpty() ? juce::String (shown) + (shown == 1 ? " sequence" : " sequences")
                               : juce::String (shown) + " of " + juce::String ((int) entries.size()) + " match";
    previewChanged();
}

juce::StringArray SequenceLibraryView::shownFolders() const
{
    juce::StringArray names;

    std::function<void (const juce::TreeViewItem&)> walk = [&] (const juce::TreeViewItem& item)
    {
        for (int i = 0; i < item.getNumSubItems(); ++i)
            if (auto* f = dynamic_cast<const FolderItem*> (item.getSubItem (i)))
            {
                names.add (SequenceLibrary::nameOf (f->path()));
                walk (*f);
            }
    };

    if (root != nullptr)
        walk (*root);

    return names;
}

std::vector<juce::File> SequenceLibraryView::shownFiles() const
{
    std::vector<juce::File> files;

    std::function<void (juce::TreeViewItem*)> walk = [&] (juce::TreeViewItem* item)
    {
        if (auto* s = dynamic_cast<SequenceItem*> (item))
            files.push_back (s->getEntry().file);

        for (int i = 0; i < item->getNumSubItems(); ++i)
            walk (item->getSubItem (i));
    };

    if (root != nullptr)
        walk (root.get());

    return files;
}

void SequenceLibraryView::timerCallback()
{
    if (folderStamp() != stamp)
        refresh();
}

void SequenceLibraryView::visibilityChanged()
{
    if (isShowing())
    {
        refresh();
        startTimer (3000);
    }
    else
    {
        stopTimer();

        if (preview != nullptr)
            preview->stop();
    }
}

void SequenceLibraryView::paint (juce::Graphics& g)
{
    g.fillAll (Theme::surface);
}

void SequenceLibraryView::resized()
{
    auto r = getLocalBounds().reduced (6);
    auto top = r.removeFromTop (26);
    folderButton.setBounds (top.removeFromRight (64));
    top.removeFromRight (4);
    newButton.setBounds (top.removeFromRight (56));
    top.removeFromRight (6);
    search.setBounds (top);
    r.removeFromTop (6);
    auto bottom = r.removeFromBottom (24);
    stopButton.setBounds (bottom.removeFromRight (56).reduced (0, 1));
    summary.setBounds (bottom);
    tree.setBounds (r);
}

juce::String SequenceLibraryView::selectedFolder() const
{
    auto* item = tree.getSelectedItem (0);

    if (item != nullptr && dynamic_cast<FolderItem*> (item) == nullptr)
        item = item->getParentItem();

    if (auto* folder = dynamic_cast<FolderItem*> (item))
        return SequenceLibrary::nameOf (folder->path());

    return {};
}

void SequenceLibraryView::folderMenu (const juce::StringArray& path)
{
    if (path.isEmpty())
        return;

    const auto fixed = SequenceLibrary::isFixed (path);
    const auto dir = library.folderAt (path);
    juce::PopupMenu menu;
    menu.addItem (1, "New Sequence Here...", onNew != nullptr);
    menu.addItem (2, "New Folder...");
    menu.addSeparator();
    menu.addItem (3, "Show in Finder");
    menu.addItem (4, "Rename...", ! fixed);
    menu.addItem (5, "Move to Trash", ! fixed);

    const juce::Component::SafePointer<SequenceLibraryView> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options(), [safe, path, dir] (int result)
    {
        if (safe == nullptr || result == 0)
            return;

        if (result == 1)
        {
            if (safe->onNew != nullptr)
                safe->onNew (SequenceLibrary::nameOf (path));
        }
        else if (result == 3)
        {
            dir.revealToUser();
        }
        else if (result == 5)
        {
            if (dir.moveToTrash())
                safe->refresh();
        }
        else
        {
            const auto renaming = result == 4;
            auto* box = new juce::AlertWindow (renaming ? "Rename Folder" : "New Folder",
                                               renaming ? juce::String() : "In " + SequenceLibrary::nameOf (path) + ".",
                                               juce::MessageBoxIconType::NoIcon, safe.getComponent());
            box->addTextEditor ("name", renaming ? path[path.size() - 1] : juce::String ("New Folder"), "Name");
            box->addButton (renaming ? "Rename" : "Make", 1, juce::KeyPress (juce::KeyPress::returnKey));
            box->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
            box->enterModalState (true, juce::ModalCallbackFunction::create ([safe, path, dir, box, renaming] (int chosen)
            {
                if (chosen != 1 || safe == nullptr)
                    return;

                const auto name = box->getTextEditorContents ("name").trim();
                juce::String error;

                if (renaming)
                {
                    const auto legal = juce::File::createLegalFileName (name);
                    const auto target = dir.getSiblingFile (legal);

                    if (legal.isEmpty())
                        error = "A folder needs a name.";
                    else if (target.exists() && target != dir)
                        error = "There is already a folder called " + legal + " there.";
                    else if (target != dir && ! dir.moveFileTo (target))
                        error = "The folder could not be renamed.";
                }
                else
                {
                    error = safe->library.makeFolder (path, name);
                }

                if (error.isNotEmpty())
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, renaming ? "Rename Folder" : "New Folder", error);

                safe->refresh();
            }), true);
        }
    });
}

void SequenceLibraryView::dropInto (const juce::StringArray& path, const juce::StringArray& files, bool copy)
{
    juce::StringArray errors;
    juce::File first;

    for (auto& f : files)
    {
        const juce::File file (f);

        if (! file.hasFileExtension (SequenceFile::extension))
            continue;

        // From outside the library it is always a copy, so the Finder's file stays where it is.
        const auto inLibrary = file.isAChildOf (library.getFolder());

        if (preview != nullptr && preview->getFile() == file && ! copy && inLibrary)
            preview->stop();

        juce::String error;
        const auto moved = library.moveInto (file, path, copy || ! inLibrary, error);

        if (error.isNotEmpty())
            errors.add (file.getFileName() + ": " + error);
        else if (first == juce::File())
            first = moved;
    }

    refresh();

    if (first != juce::File())
        select (first);

    if (! errors.isEmpty())
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, copy ? "Copy Sequence" : "Move Sequence", errors.joinIntoString ("\n"));
}

void SequenceLibraryView::select (const juce::File& file)
{
    std::function<bool (juce::TreeViewItem&)> find = [&] (juce::TreeViewItem& item)
    {
        for (int i = 0; i < item.getNumSubItems(); ++i)
        {
            auto* sub = item.getSubItem (i);

            if (auto* s = dynamic_cast<SequenceItem*> (sub); s != nullptr && s->getEntry().file == file)
            {
                for (auto* p = sub->getParentItem(); p != nullptr; p = p->getParentItem())
                    p->setOpen (true);

                sub->setSelected (true, true, juce::dontSendNotification);
                tree.scrollToKeepItemVisible (sub);
                return true;
            }

            if (find (*sub))
                return true;
        }

        return false;
    };

    if (root != nullptr)
        find (*root);
}
