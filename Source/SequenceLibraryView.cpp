#include "SequenceLibraryView.h"
#include "SequenceMidi.h"
#include "NoteTools.h"
#include "Instrument.h"
#include "Theme.h"

namespace
{
    const juce::String dragPrefix { "tqseq:" };

    juce::String detailsOf (const SequenceLibrary::Entry& e)
    {
        juce::StringArray parts;
        const auto bars = e.bars;

        if (e.isKit)
            parts.add ("kit of " + juce::String (e.parts) + (e.parts == 1 ? " part" : " parts"));

        if (bars > 0.0)
            parts.add (juce::exactlyEqual (bars, std::round (bars)) ? juce::String ((int) bars) + (juce::exactlyEqual (bars, 1.0) ? " bar" : " bars")
                                                                    : juce::String (bars, 2) + " bars");

        if (e.key.isNotEmpty())
            parts.add (e.key);

        if (! e.hasInstrument)
            parts.add ("no instrument");

        if (e.rating > 0)
            parts.add (juce::String::repeatedString (juce::String::fromUTF8 ("\xe2\x98\x85"), e.rating));

        if (e.favourite)
            parts.add (juce::String::fromUTF8 ("\xe2\x99\xa5"));

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

        // A right-click on a selected one keeps the selection, so Make Kit can take them all.
        const auto selection = view.selectedFiles();
        juce::Array<juce::File> sequences;

        for (auto& f : selection)
            if (f.hasFileExtension (SequenceFile::extension))
                sequences.add (f);

        juce::PopupMenu menu, rating;

        for (int stars = 0; stars <= 5; ++stars)
            rating.addItem (20 + stars, stars == 0 ? juce::String ("No Rating") : juce::String::repeatedString (juce::String::fromUTF8 ("\xe2\x98\x85"), stars),
                            true, entry.rating == stars);

        menu.addItem (4, "Edit in Sequencer", view.onEdit != nullptr && ! entry.isKit);
        menu.addItem (5, "Export MIDI File...");
        menu.addItem (6, "Make Kit from Selected...", sequences.size() >= 2);
        menu.addSeparator();
        menu.addSubMenu ("Rating", rating);
        menu.addItem (7, "Favourite", true, entry.favourite);
        menu.addSeparator();

        if (entry.factory)
            menu.addItem (8, "Copy to My Library");

        menu.addItem (1, "Show in Finder");
        menu.addItem (2, "Rename...", ! entry.factory);
        menu.addItem (3, "Move to Trash", ! entry.factory);
        const auto file = entry.file;
        const auto favourite = entry.favourite;
        juce::Component::SafePointer<SequenceLibraryView> safe (&view);

        menu.showMenuAsync (juce::PopupMenu::Options(), [safe, file, favourite, selection, sequences] (int result)
        {
            if (safe == nullptr)
                return;

            const auto onSelection = selection.contains (file) ? selection : juce::Array<juce::File> { file };
            juce::String error;

            if (result == 4 && safe->onEdit != nullptr)
                safe->onEdit (file);
            else if (result == 5)
                safe->askToExportMidi (file);
            else if (result == 6)
                safe->askToMakeKit (sequences, "Kit");
            else if (result == 8)
                safe->copyToLibrary (onSelection);
            else if (result >= 20 && result <= 25)
            {
                for (auto& f : onSelection)
                    if (error.isEmpty())
                        error = SequenceLibrary::setRating (f, result - 20);
            }
            else if (result == 7)
            {
                for (auto& f : onSelection)
                    if (error.isEmpty())
                        error = SequenceLibrary::setFavourite (f, ! favourite);
            }
            else if (result == 1)
                file.revealToUser();
            else if (result == 3 && file.moveToTrash())
                safe->refresh();
            else if (result == 2)
            {
                auto* box = new juce::AlertWindow (file.hasFileExtension (SequenceFile::kitExtension) ? "Rename Kit" : "Rename Sequence", {},
                                                   juce::MessageBoxIconType::NoIcon, safe.getComponent());
                box->addTextEditor ("name", file.getFileNameWithoutExtension(), "Name");
                box->addButton ("Rename", 1, juce::KeyPress (juce::KeyPress::returnKey));
                box->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
                box->enterModalState (true, juce::ModalCallbackFunction::create ([safe, file, box] (int chosen)
                {
                    if (chosen != 1 || safe == nullptr || box->getTextEditorContents ("name").trim().isEmpty())
                        return;

                    juce::String renameError;
                    const auto renamed = SequenceLibrary::rename (file, box->getTextEditorContents ("name"), renameError);

                    if (renameError.isNotEmpty())
                        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Rename", renameError);

                    safe->refresh();

                    if (renamed != juce::File())
                        safe->select (renamed);
                }), true);
            }

            if (result == 7 || (result >= 20 && result <= 25))
            {
                if (error.isNotEmpty())
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, result == 7 ? "Favourite" : "Rating", error);

                safe->refresh();
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
        return ! path().isEmpty() && ! isFactoryPath (path()) && fileOfDrag (details.description).existsAsFile();
    }

    void itemDropped (const juce::DragAndDropTarget::SourceDetails& details, int) override
    {
        view.dropInto (path(), { fileOfDrag (details.description).getFullPathName() },
                       juce::ModifierKeys::currentModifiers.isAltDown());
    }

    bool isInterestedInFileDrag (const juce::StringArray& files) override
    {
        return ! path().isEmpty() && ! isFactoryPath (path()) && std::any_of (files.begin(), files.end(), [] (auto& f)
        {
            return SequenceFile::isSequenceOrKit (juce::File (f)) || juce::File (f).hasFileExtension (SequenceLibrary::packExtension);
        });
    }

    // A pack dropped here opens into the library, each sequence by its own role.
    void filesDropped (const juce::StringArray& files, int) override
    {
        for (auto& f : files)
            if (juce::File (f).hasFileExtension (SequenceLibrary::packExtension))
                view.openPack (juce::File (f));

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

    addAndMakeVisible (sortBox);
    sortBox.addItemList (SequenceLibrary::sortNames(), 1);
    sortBox.setSelectedItemIndex (0, juce::dontSendNotification);
    sortBox.setTooltip ("The order in each folder: by name, by length (fewest bars first), by rating (highest first), or the newest first");
    sortBox.onChange = [this] { setSort ((SequenceLibrary::Sort) juce::jmax (0, sortBox.getSelectedItemIndex())); };

    addAndMakeVisible (favouritesButton);
    favouritesButton.setClickingTogglesState (true);
    favouritesButton.setTooltip ("Show only the sequences and kits marked as favourites (F, or Favourite on the right-click menu)");
    favouritesButton.onClick = [this] { rebuild(); };

    addAndMakeVisible (fitsButton);
    fitsButton.setClickingTogglesState (true);
    fitsButton.setTooltip ("Show only the sequences in the session's key (set in Multitrack > Tempo...), and those with no key, such as drums");
    fitsButton.onClick = [this] { rebuild(); };

    addAndMakeVisible (importButton);
    importButton.setTooltip ("Import MIDI files into the library as sequences, or open a pack of sequences someone shared");
    importButton.onClick = [this] { showImportMenu(); };

    addAndMakeVisible (stopButton);
    stopButton.setTooltip ("Stop the sequence playing");
    stopButton.onClick = [this] { if (preview != nullptr) preview->stop(); };
    stopButton.setEnabled (false);

    addAndMakeVisible (tree);
    tree.setMultiSelectEnabled (true);
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

void SequenceLibraryView::setSort (SequenceLibrary::Sort s)
{
    sortBy = s;
    sortBox.setSelectedItemIndex ((int) s, juce::dontSendNotification);
    rebuild();
}

void SequenceLibraryView::setFavouritesOnly (bool on)
{
    favouritesButton.setToggleState (on, juce::dontSendNotification);
    rebuild();
}

void SequenceLibraryView::setFitsSession (bool on)
{
    fitsButton.setToggleState (on, juce::dontSendNotification);
    rebuild();
}

void SequenceLibraryView::setFolder (const juce::File& folder)
{
    if (folder == library.getFolder())
        return;

    if (preview != nullptr)
        preview->stop();

    library = SequenceLibrary (folder);
    library.makeFolders();
    openness.reset();
    folderButton.setTooltip ("Show the library's folder in the Finder: " + library.getFolder().getFullPathName());
    refresh();
}

juce::Array<juce::File> SequenceLibraryView::selectedFiles() const
{
    juce::Array<juce::File> files;

    for (int i = 0; i < tree.getNumSelectedItems(); ++i)
        if (auto* s = dynamic_cast<SequenceItem*> (tree.getSelectedItem (i)))
            files.add (s->getEntry().file);

    return files;
}

void SequenceLibraryView::rateSelected (int rating)
{
    juce::String error;

    for (auto& f : selectedFiles())
        if (error.isEmpty())
            error = SequenceLibrary::setRating (f, rating);

    refresh();

    if (error.isNotEmpty())
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Rating", error);
}

void SequenceLibraryView::toggleFavouriteSelected()
{
    const auto files = selectedFiles();
    const auto all = std::all_of (files.begin(), files.end(), [this] (const juce::File& f)
    {
        return std::any_of (entries.begin(), entries.end(), [&f] (auto& e) { return e.file == f && e.favourite; });
    });
    juce::String error;

    for (auto& f : files)
        if (error.isEmpty())
            error = SequenceLibrary::setFavourite (f, ! all);

    refresh();

    if (error.isNotEmpty())
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Favourite", error);
}

bool SequenceLibraryView::keyPressed (const juce::KeyPress& key)
{
    if (selectedFiles().isEmpty() || key.getModifiers().isAnyModifierKeyDown())
        return false;

    const auto code = key.getKeyCode();

    if (code >= '0' && code <= '5')
    {
        rateSelected (code - '0');
        return true;
    }

    if (code == 'f' || code == 'F')
    {
        toggleFavouriteSelected();
        return true;
    }

    return false;
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

    // The factory presets, under Presets.
    for (auto e : SequenceLibrary (SequenceLibrary::factoryFolder()).scan())
    {
        e.path.insert (0, SequenceLibrary::factoryName);
        entries.push_back (std::move (e));
    }

    rebuild();
}

void SequenceLibraryView::setSearch (const juce::String& text)
{
    search.setText (text, false);
    rebuild();
}

void SequenceLibraryView::rebuild()
{
    const auto wasSelected = selectedFiles();

    // The folders opened by hand are kept, not those a search or filter opened.
    if (root != nullptr && ! shownNarrowed)
        openness = root->getOpennessState();

    tree.setRootItem (nullptr);
    root = std::make_unique<FolderItem> (*this, "Sequences");
    const auto text = search.getText().trim();
    const auto favouritesOnly = favouritesButton.getToggleState();
    const auto fits = fitsButton.getToggleState();
    const auto narrowed = text.isNotEmpty() || favouritesOnly || fits;
    shownKey = getSessionKey != nullptr ? getSessionKey().trim() : juce::String();

    const auto passes = [&] (const SequenceLibrary::Entry& e)
    {
        return ! ((text.isNotEmpty() && ! SequenceLibrary::matches (e, text)) || (favouritesOnly && ! e.favourite)
                  || (fits && ! SequenceLibrary::fitsKey (e, shownKey)));
    };

    // Presets first, its roles and sub-roles in their order (those holding presets that are shown).
    if (std::any_of (entries.begin(), entries.end(), [&] (auto& e) { return e.factory && passes (e); }))
    {
        auto& presets = root->folder (SequenceLibrary::factoryName);

        for (auto& role : SequenceLibrary::roles())
            for (auto& sub : juce::StringArray (role.subRoles).isEmpty() ? juce::StringArray { "" } : role.subRoles)
                for (auto& e : entries)
                    if (e.factory && passes (e) && e.path[1] == role.name && (sub.isEmpty() || e.path[2] == sub))
                    {
                        auto& r = presets.folder (role.name);

                        if (sub.isNotEmpty())
                            r.folder (sub);

                        break;
                    }
    }

    // Every folder, empty ones too: the roles and their sub-roles in their order, then the user's
    // own below them. A search or filter shows the roles and the folders holding what it finds.
    for (auto& path : narrowed ? std::vector<juce::StringArray> {} : folderPaths)
    {
        auto* at = root.get();

        for (auto& part : path)
            at = &at->folder (part);
    }

    for (auto& role : SequenceLibrary::roles())
        root->folder (role.name);

    shownNarrowed = narrowed;
    int shown = 0;
    auto ordered = entries;
    SequenceLibrary::sort (ordered, sortBy);

    for (auto& e : ordered)
    {
        if (! passes (e))
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

    if (! narrowed && openness != nullptr)
        root->restoreOpennessState (*openness);

    // With a search or filter, every folder holding a match is open.
    if (narrowed)
    {
        std::function<void (juce::TreeViewItem&)> openAll = [&] (juce::TreeViewItem& item)
        {
            item.setOpen (true);

            for (int i = 0; i < item.getNumSubItems(); ++i)
                openAll (*item.getSubItem (i));
        };

        openAll (*root);
    }

    // What was selected stays selected (a rating or a rescan doesn't lose it).
    std::function<void (juce::TreeViewItem&)> reselect = [&] (juce::TreeViewItem& item)
    {
        for (int i = 0; i < item.getNumSubItems(); ++i)
        {
            auto* sub = item.getSubItem (i);

            if (auto* s = dynamic_cast<SequenceItem*> (sub); s != nullptr && wasSelected.contains (s->getEntry().file))
                sub->setSelected (true, false, juce::dontSendNotification);

            reselect (*sub);
        }
    };

    reselect (*root);

    const auto noun = [] (int n) { return juce::String (n) + (n == 1 ? " sequence" : " sequences"); };
    countText = ! narrowed ? noun (shown) : juce::String (shown) + " of " + juce::String ((int) entries.size()) + " match";

    if (fits && ! NoteTools::parseKey (shownKey).has_value())
        countText << " (the session has no key)";
    else if (fits)
        countText << " in " << shownKey;

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
    else if (fitsButton.getToggleState() && getSessionKey != nullptr && getSessionKey().trim() != shownKey)
        rebuild();
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
    r.removeFromTop (4);
    auto filters = r.removeFromTop (24);
    importButton.setBounds (filters.removeFromRight (70));
    filters.removeFromRight (6);
    sortBox.setBounds (filters.removeFromLeft (juce::jmin (90, filters.getWidth() / 4)));
    filters.removeFromLeft (4);
    const auto half = (filters.getWidth() - 4) / 2;
    favouritesButton.setBounds (filters.removeFromLeft (half));
    filters.removeFromLeft (4);
    fitsButton.setBounds (filters);
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
        return SequenceLibrary::nameOf (ownPathFor (folder->path()));

    return {};
}

bool SequenceLibraryView::isFactoryPath (const juce::StringArray& path)
{
    return path[0] == SequenceLibrary::factoryName;
}

juce::StringArray SequenceLibraryView::ownPathFor (const juce::StringArray& path)
{
    if (! isFactoryPath (path))
        return path;

    auto own = path;
    own.remove (0);
    return own;
}

void SequenceLibraryView::copyToLibrary (const juce::Array<juce::File>& files)
{
    juce::StringArray errors;
    juce::File first;

    for (auto& file : files)
    {
        const auto entry = std::find_if (entries.begin(), entries.end(), [&file] (auto& e) { return e.file == file; });

        if (entry == entries.end())
            continue;

        juce::String error;
        const auto copied = library.moveInto (file, ownPathFor (entry->path), true, error);

        if (error.isNotEmpty())
            errors.add (file.getFileName() + ": " + error);
        else if (first == juce::File())
            first = copied;
    }

    refresh();

    if (first != juce::File())
        select (first);

    if (! errors.isEmpty())
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Copy to My Library", errors.joinIntoString ("\n"));
}

void SequenceLibraryView::folderMenu (const juce::StringArray& path)
{
    if (path.isEmpty())
        return;

    // Presets' folders: new sequences and copies go to the user's folder of the same role.
    if (isFactoryPath (path))
    {
        juce::Array<juce::File> presets;

        for (auto& e : entries)
            if (e.factory && e.path.joinIntoString ("/").startsWith (path.joinIntoString ("/")) && ! e.isKit)
                presets.add (e.file);

        juce::PopupMenu menu;
        menu.addItem (1, "New Sequence Here...", onNew != nullptr && path.size() > 1);
        menu.addItem (2, "Copy to My Library", ! presets.isEmpty());
        menu.addItem (3, "Save as Kit...", ! presets.isEmpty() && presets.size() <= 16);
        const juce::Component::SafePointer<SequenceLibraryView> safe (this);
        menu.showMenuAsync (juce::PopupMenu::Options(), [safe, path, presets] (int result)
        {
            if (safe == nullptr)
                return;

            if (result == 1 && safe->onNew != nullptr)
                safe->onNew (SequenceLibrary::nameOf (ownPathFor (path)));
            else if (result == 2)
                safe->copyToLibrary (presets);
            else if (result == 3)
                safe->askToMakeKit (presets, path[path.size() - 1]);
        });

        return;
    }

    const auto fixed = SequenceLibrary::isFixed (path);
    const auto dir = library.folderAt (path);
    juce::PopupMenu menu;
    const auto sequences = library.sequencesIn (path);
    menu.addItem (1, "New Sequence Here...", onNew != nullptr);
    menu.addItem (2, "New Folder...");
    menu.addSeparator();
    menu.addItem (6, "Save as Kit...", ! sequences.isEmpty());
    menu.addItem (7, "Import MIDI Files Here...");
    menu.addItem (8, "Save as Pack...");
    menu.addSeparator();
    menu.addItem (3, "Show in Finder");
    menu.addItem (4, "Rename...", ! fixed);
    menu.addItem (5, "Move to Trash", ! fixed);

    const juce::Component::SafePointer<SequenceLibraryView> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options(), [safe, path, dir, sequences] (int result)
    {
        if (safe == nullptr || result == 0)
            return;

        if (result == 6)
            safe->askToMakeKit (sequences, path[path.size() - 1]);
        else if (result == 7)
            safe->askToImportMidi (SequenceLibrary::nameOf (path));
        else if (result == 8)
            safe->askToSavePack (path);
        else if (result == 1)
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

        if (! SequenceFile::isSequenceOrKit (file))
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

void SequenceLibraryView::askToMakeKit (const juce::Array<juce::File>& sequences, const juce::String& suggestedName)
{
    if (sequences.isEmpty())
        return;

    auto* box = new juce::AlertWindow ("Make Kit", "Saves the " + juce::String (sequences.size()) + (sequences.size() == 1 ? " sequence" : " sequences")
                                                       + " as a kit in Kits: one file holding every part with its voice. Dropped into a session, "
                                                         "it makes a track for each part under a folder track named for the kit.",
                                       juce::MessageBoxIconType::NoIcon, this);
    box->addTextEditor ("name", suggestedName, "Name");
    box->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    box->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    box->enterModalState (true, juce::ModalCallbackFunction::create ([safe = juce::Component::SafePointer<SequenceLibraryView> (this), box, sequences] (int chosen)
    {
        if (chosen != 1 || safe == nullptr)
            return;

        juce::String error;
        const auto kit = safe->library.saveKit (box->getTextEditorContents ("name"), sequences, error);
        safe->refresh();

        if (error.isNotEmpty())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Make Kit", error);
        else
            safe->select (kit);
    }), true);
}

void SequenceLibraryView::askToExportMidi (const juce::File& file)
{
    const auto start = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile (file.getFileNameWithoutExtension() + ".mid");
    chooser = std::make_unique<juce::FileChooser> ("Export MIDI File", start, "*.mid");
    const auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting;

    chooser->launchAsync (flags, [file] (const juce::FileChooser& fc)
    {
        const auto target = fc.getResult();

        if (target == juce::File())
            return;

        juce::String error;
        std::vector<SequenceFile::Sequence> parts;

        if (file.hasFileExtension (SequenceKit::extension))
        {
            if (auto kit = SequenceKit::load (file, error))
                parts = std::move (kit->parts);
        }
        else if (auto q = SequenceFile::load (file, error))
        {
            parts.push_back (std::move (*q));
        }

        if (error.isEmpty())
            error = SequenceMidi::exportFile (parts, target.withFileExtension (".mid"));

        if (error.isNotEmpty())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Export MIDI File", error);
    });
}

void SequenceLibraryView::askToImportMidi (const juce::String& folder)
{
    chooser = std::make_unique<juce::FileChooser> ("Import MIDI Files", juce::File::getSpecialLocation (juce::File::userHomeDirectory), "*.mid;*.midi");
    const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems;

    chooser->launchAsync (flags, [safe = juce::Component::SafePointer<SequenceLibraryView> (this), folder] (const juce::FileChooser& fc)
    {
        const auto files = fc.getResults();

        if (safe == nullptr || files.isEmpty())
            return;

        auto& self = *safe;
        self.library.makeFolders();
        const auto folders = self.library.folderNames();
        const auto instruments = self.getInstruments != nullptr ? self.getInstruments() : juce::Array<juce::PluginDescription>();
        juce::StringArray voices { "None: play on the instrument of the track it is dropped on" };

        for (auto& d : instruments)
            voices.add (Instrument::title (d));

        auto* box = new juce::AlertWindow ("Import MIDI Files",
                                           "Each track with notes in " + (files.size() == 1 ? files[0].getFileName() : juce::String (files.size()) + " files")
                                               + " becomes a sequence, with the file's tempo, time signature and key, as long as whole bars.",
                                           juce::MessageBoxIconType::NoIcon, safe.getComponent());
        box->addComboBox ("folder", folders, "Folder");
        box->getComboBoxComponent ("folder")->setSelectedItemIndex (juce::jmax (0, folders.indexOf (folder)));
        box->addComboBox ("voice", voices, "Instrument");
        box->getComboBoxComponent ("voice")->setSelectedItemIndex (0);
        box->addButton ("Import", 1, juce::KeyPress (juce::KeyPress::returnKey));
        box->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        box->enterModalState (true, juce::ModalCallbackFunction::create ([safe, box, files, folders, instruments] (int chosen)
        {
            if (chosen != 1 || safe == nullptr)
                return;

            const auto path = SequenceLibrary::pathOf (folders[box->getComboBoxComponent ("folder")->getSelectedItemIndex()]);
            const auto voiceIndex = box->getComboBoxComponent ("voice")->getSelectedItemIndex() - 1;
            std::shared_ptr<const juce::XmlElement> voice;

            if (juce::isPositiveAndBelow (voiceIndex, instruments.size()))
                voice = SequenceFile::voiceFor (instruments[voiceIndex]);

            juce::Array<juce::File> fileArray;

            for (auto& f : files)
                fileArray.add (f);

            juce::StringArray errors;
            const auto made = safe->library.importMidi (fileArray, path, voice, errors);
            safe->refresh();

            if (! made.isEmpty())
                safe->select (made[0]);

            if (! errors.isEmpty())
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Import MIDI Files",
                                                        "Imported " + juce::String (made.size()) + (made.size() == 1 ? " sequence." : " sequences.")
                                                            + "\n\n" + errors.joinIntoString ("\n"));
        }), true);
    });
}

void SequenceLibraryView::askToSavePack (const juce::StringArray& path)
{
    const auto start = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                           .getChildFile (juce::File::createLegalFileName (path.joinIntoString (" ")) + SequenceLibrary::packExtension);
    chooser = std::make_unique<juce::FileChooser> ("Save as Pack", start, juce::String ("*") + SequenceLibrary::packExtension);
    const auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting;

    chooser->launchAsync (flags, [safe = juce::Component::SafePointer<SequenceLibraryView> (this), path] (const juce::FileChooser& fc)
    {
        const auto target = fc.getResult();

        if (safe == nullptr || target == juce::File())
            return;

        int count = 0;

        const auto pack = target.withFileExtension (SequenceLibrary::packExtension);

        if (const auto error = safe->library.savePack (path, pack, count); error.isNotEmpty())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Save as Pack", error);
        else
            safe->summary.setText ("Packed " + juce::String (count) + (count == 1 ? " file" : " files") + " into " + pack.getFileName() + ".", juce::dontSendNotification);
    });
}

void SequenceLibraryView::askToOpenPack()
{
    chooser = std::make_unique<juce::FileChooser> ("Open Pack", juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                                                   juce::String ("*") + SequenceLibrary::packExtension + ";*.zip");
    const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync (flags, [safe = juce::Component::SafePointer<SequenceLibraryView> (this)] (const juce::FileChooser& fc)
    {
        if (safe != nullptr && fc.getResult() != juce::File())
            safe->openPack (fc.getResult());
    });
}

void SequenceLibraryView::openPack (const juce::File& pack)
{
    juce::StringArray errors;
    const auto made = library.openPack (pack, errors);
    refresh();

    if (! made.isEmpty())
        select (made[0]);

    const auto said = "Opened " + juce::String (made.size()) + (made.size() == 1 ? " file" : " files") + " from " + pack.getFileName() + " into the library.";

    if (! errors.isEmpty() || made.isEmpty())
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Open Pack", said + (errors.isEmpty() ? juce::String() : "\n\n" + errors.joinIntoString ("\n")));
    else
        summary.setText (said, juce::dontSendNotification);
}

void SequenceLibraryView::showImportMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "MIDI Files...");
    menu.addItem (2, "Pack...");
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (importButton), [safe = juce::Component::SafePointer<SequenceLibraryView> (this)] (int result)
    {
        if (safe == nullptr)
            return;

        if (result == 1)
            safe->askToImportMidi (safe->selectedFolder());
        else if (result == 2)
            safe->askToOpenPack();
    });
}

//==============================================================================
SequenceLibraryView::FolderSetting::FolderSetting (std::function<void (const juce::File&)> onChange) : changed (std::move (onChange))
{
    addAndMakeVisible (where);
    where.setMinimumHorizontalScale (0.6f);
    Theme::follow (where, juce::Label::textColourId, Theme::text);

    addAndMakeVisible (choose);
    choose.setTooltip ("Choose the folder the Sequence Library keeps its sequences and kits in");
    choose.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Sequence Library Folder", SequenceLibrary::defaultFolder());
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                              [safe = juce::Component::SafePointer<FolderSetting> (this)] (const juce::FileChooser& fc)
        {
            if (safe != nullptr && fc.getResult().isDirectory())
            {
                safe->changed (fc.getResult());
                safe->show();
            }
        });
    };

    addAndMakeVisible (standard);
    standard.setTooltip ("Go back to the standard folder: " + SequenceLibrary::standardFolder().getFullPathName());
    standard.onClick = [this]
    {
        changed ({});
        show();
    };

    show();
}

void SequenceLibraryView::FolderSetting::show()
{
    const auto folder = SequenceLibrary::defaultFolder();
    where.setText (folder.getFullPathName(), juce::dontSendNotification);
    where.setTooltip (folder.getFullPathName());
    standard.setEnabled (folder != SequenceLibrary::standardFolder());
}

void SequenceLibraryView::FolderSetting::resized()
{
    auto r = getLocalBounds();
    standard.setBounds (r.removeFromRight (80).reduced (0, 2));
    r.removeFromRight (6);
    choose.setBounds (r.removeFromRight (80).reduced (0, 2));
    r.removeFromRight (6);
    where.setBounds (r);
}
