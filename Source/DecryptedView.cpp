#include "DecryptedView.h"
#include "SignalConsoleView.h"
#include "Theme.h"

using namespace SignalConsoleDrawing;

namespace
{
    constexpr float rowHeight = 28.0f;

    juce::Colour signal()       { return Theme::warn; }

    /** Where the columns start, as parts of the list's width: name, category, lock, recording, date. */
    constexpr std::array<float, 6> columnAt { 0.0f, 0.34f, 0.52f, 0.62f, 0.84f, 1.0f };
    const char* const columnNames[] = { "NAME", "CATEGORY", "LOCK", "FROM", "TAKEN" };

    juce::String dateOf (juce::Time t)
    {
        return t.toMilliseconds() <= 0 ? juce::String() : t.formatted ("%Y-%m-%d %H:%M");
    }
}

DecryptedView::DecryptedView (Synth& s) : synth (s)
{
    setWantsKeyboardFocus (true);
    filter.setFont (mono (12.0f));
    filter.setTextToShowWhenEmpty ("filter: a name, a category, a recording", Theme::faintText);
    filter.onTextChange = [this] { refresh(); };
    filter.onEscapeKey = [this] { filter.setText ({}, true); grabKeyboardFocus(); };
    filter.onReturnKey = [this] { grabKeyboardFocus(); };
    addAndMakeVisible (filter);
    refresh();
}

void DecryptedView::visibilityChanged()
{
    if (isVisible())
        refresh();
}

void DecryptedView::setFilter (const juce::String& words)
{
    filter.setText (words, false);
    refresh();
}

void DecryptedView::refresh()
{
    rows = DecryptedPatches::Store::shared().entries();

    // Filtered: every word typed is in the name, the category or the recording.
    const auto words = juce::StringArray::fromTokens (filter.getText().trim(), " ", {});

    rows.erase (std::remove_if (rows.begin(), rows.end(), [&words] (const DecryptedPatches::Entry& e)
    {
        const auto all = e.name + " " + e.category + " " + e.source;

        for (auto& w : words)
            if (w.isNotEmpty() && ! all.containsIgnoreCase (w))
                return true;

        return false;
    }), rows.end());

    std::stable_sort (rows.begin(), rows.end(), [this] (const DecryptedPatches::Entry& a, const DecryptedPatches::Entry& b)
    {
        const auto order = [this, &a, &b]
        {
            switch (sorted)
            {
                case Column::category:  return a.category.isEmpty() != b.category.isEmpty() ? (a.category.isEmpty() ? 1 : -1) : a.category.compareIgnoreCase (b.category);
                case Column::lock:      return a.lock < b.lock ? -1 : a.lock > b.lock ? 1 : 0;
                case Column::source:    return a.source.compareIgnoreCase (b.source);
                case Column::date:      return a.taken < b.taken ? -1 : a.taken > b.taken ? 1 : 0;
                case Column::name:      break;
            }

            return a.name.compareIgnoreCase (b.name);
        }();

        return ascending ? order < 0 : order > 0;
    });

    // What is selected is still there, or is not selected.
    for (int i = selection.size(); --i >= 0;)
        if (std::none_of (rows.begin(), rows.end(), [this, i] (const DecryptedPatches::Entry& e) { return e.name == selection[i]; }))
            selection.remove (i);

    layoutRows();
    repaint();
}

void DecryptedView::sortBy (Column c)
{
    ascending = c == sorted ? ! ascending : c != Column::date && c != Column::lock;   // the newest and the nearest first
    sorted = c;
    refresh();
}

void DecryptedView::resized()
{
    const auto b = getLocalBounds().toFloat();
    filter.setBounds (juce::Rectangle<float> (b.getRight() - 24.0f - 300.0f, 10.0f, 300.0f, 24.0f).toNearestInt());
    headings = { b.getX() + 24.0f, 50.0f, b.getWidth() - 48.0f, 24.0f };
    list = { headings.getX(), headings.getBottom() + 4.0f, headings.getWidth(), b.getBottom() - 56.0f - headings.getBottom() - 4.0f };
    const auto y = b.getBottom() - 42.0f;
    categoryChip = { list.getX(), y, 130.0f, 26.0f };
    renameChip = { categoryChip.getRight() + 8.0f, y, 100.0f, 26.0f };
    deleteChip = { renameChip.getRight() + 8.0f, y, 100.0f, 26.0f };
    layoutRows();
}

void DecryptedView::layoutRows()
{
    scroll = juce::jlimit (0.0f, juce::jmax (0.0f, (float) rows.size() * rowHeight - list.getHeight()), scroll);
}

juce::Rectangle<float> DecryptedView::rowBounds (int index) const
{
    if (! juce::isPositiveAndBelow (index, (int) rows.size()))
        return {};

    const juce::Rectangle<float> r (list.getX(), list.getY() + (float) index * rowHeight - scroll, list.getWidth(), rowHeight);
    return r.getY() >= list.getY() - 0.5f && r.getBottom() <= list.getBottom() + 0.5f ? r : juce::Rectangle<float>();
}

juce::Rectangle<float> DecryptedView::headingBounds (Column c) const
{
    const auto i = (size_t) c;
    return { headings.getX() + headings.getWidth() * columnAt[i], headings.getY(), headings.getWidth() * (columnAt[i + 1] - columnAt[i]), headings.getHeight() };
}

int DecryptedView::rowAt (juce::Point<float> p) const
{
    for (int i = 0; i < (int) rows.size(); ++i)
        if (rowBounds (i).contains (p))
            return i;

    return -1;
}

void DecryptedView::keepInView (int index)
{
    const auto top = (float) index * rowHeight;
    scroll = juce::jlimit (juce::jmax (0.0f, top + rowHeight - list.getHeight()), top, scroll);
    layoutRows();
}

//==============================================================================
void DecryptedView::select (int index, bool toggle, bool range)
{
    if (! juce::isPositiveAndBelow (index, (int) rows.size()))
        return;

    const auto& name = rows[(size_t) index].name;

    if (range && anchor.isNotEmpty())
    {
        auto from = index;

        for (int i = 0; i < (int) rows.size(); ++i)
            if (rows[(size_t) i].name == anchor)
                from = i;

        selection.clear();

        for (int i = juce::jmin (from, index); i <= juce::jmax (from, index); ++i)
            selection.add (rows[(size_t) i].name);
    }
    else if (toggle)
    {
        if (selection.contains (name))
            selection.removeString (name);
        else
            selection.add (name);

        anchor = name;
    }
    else
    {
        selection = juce::StringArray { name };
        anchor = name;
    }

    keepInView (index);
    repaint();
}

bool DecryptedView::hear (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) rows.size()) || ! DecryptedPatches::Store::shared().load (synth, rows[(size_t) index].name))
        return false;

    // The arrows at the top then go through the Decrypted patches too.
    synth.setBrowsingDecrypted (true);

    if (onLoaded != nullptr)
        onLoaded();

    repaint();
    return true;
}

bool DecryptedView::open (int index)
{
    if (! hear (index))
        return false;

    if (onOpen != nullptr)
        onOpen();

    return true;
}

void DecryptedView::step (int by)
{
    if (rows.empty())
        return;

    auto at = -1;

    for (int i = 0; i < (int) rows.size(); ++i)
        if (selection.contains (rows[(size_t) i].name))
            at = by > 0 ? i : (at < 0 ? i : at);

    const auto next = at < 0 ? (by > 0 ? 0 : (int) rows.size() - 1) : juce::jlimit (0, (int) rows.size() - 1, at + by);
    select (next);
    hear (next);
}

int DecryptedView::categoriseSelected (const juce::String& category)
{
    const auto changed = DecryptedPatches::Store::shared().setCategory (selection, category);

    if (changed > 0)
    {
        refresh();
        if (onChanged != nullptr) onChanged();
    }

    return changed;
}

bool DecryptedView::renameSelected (const juce::String& to)
{
    if (selection.size() != 1)
        return false;

    const auto from = selection[0];
    const auto onIt = synth.getPatchName().equalsIgnoreCase (from);

    if (! DecryptedPatches::Store::shared().rename (from, to))
        return false;

    if (onIt)
        synth.setPatchName (to.trim());

    selection = juce::StringArray { to.trim() };
    anchor = to.trim();
    refresh();
    if (onChanged != nullptr) onChanged();
    return true;
}

int DecryptedView::deleteSelected()
{
    int deleted = 0;

    for (auto& name : selection)
        deleted += DecryptedPatches::Store::shared().remove (name) ? 1 : 0;

    selection.clear();
    refresh();

    if (deleted > 0 && onChanged != nullptr)
        onChanged();

    return deleted;
}

//==============================================================================
void DecryptedView::askRename()
{
    if (selection.size() != 1)
        return;

    const auto from = selection[0];
    auto* box = new juce::AlertWindow ("Rename Patch", "A new name for " + from.quoted() + ".", juce::MessageBoxIconType::NoIcon, this);
    box->addTextEditor ("name", from, "Name");
    box->addButton ("Rename", 1, juce::KeyPress (juce::KeyPress::returnKey));
    box->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    box->enterModalState (true, juce::ModalCallbackFunction::create ([safe = juce::Component::SafePointer<DecryptedView> (this), box, from] (int result)
    {
        if (safe == nullptr || result != 1)
            return;

        const auto to = box->getTextEditorContents ("name").trim();

        if (const auto why = DecryptedPatches::Store::shared().whyNot (to, from); why.isNotEmpty() || ! safe->renameSelected (to))
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Rename Patch",
                                                    why.isNotEmpty() ? why + " " + from.quoted() + " keeps its name." : from.quoted() + " couldn't be renamed.");
    }), true);
}

void DecryptedView::askDelete()
{
    if (selection.isEmpty())
        return;

    const auto what = selection.size() == 1 ? selection[0].quoted() : juce::String (selection.size()) + " patches";
    juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon, "Delete", "Delete " + what + " from your Decrypted patches? The synth keeps the sound it is on.",
                                        "Delete", "Cancel", this,
                                        juce::ModalCallbackFunction::create ([safe = juce::Component::SafePointer<DecryptedView> (this)] (int result)
                                        {
                                            if (safe != nullptr && result == 1)
                                                safe->deleteSelected();
                                        }));
}

void DecryptedView::showCategoryMenu (juce::Rectangle<int> at)
{
    if (selection.isEmpty())
        return;

    // Ticked: the category every row selected has.
    juce::String shared;
    auto mixed = false;

    for (int i = 0; i < selection.size(); ++i)
        if (const auto e = DecryptedPatches::Store::shared().entry (selection[i]))
        {
            mixed = mixed || (i > 0 && e->category != shared);
            shared = e->category;
        }

    juce::PopupMenu menu;
    menu.addItem (1, "No category", true, ! mixed && shared.isEmpty());
    menu.addSeparator();
    const auto& all = WaveSynth::patchCategories();

    for (int i = 0; i < all.size(); ++i)
        menu.addItem (2 + i, all[i], true, ! mixed && all[i] == shared);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (at)),
                        [safe = juce::Component::SafePointer<DecryptedView> (this)] (int chosen)
                        {
                            if (safe != nullptr && chosen > 0)
                                safe->categoriseSelected (chosen == 1 ? juce::String() : WaveSynth::patchCategories()[chosen - 2]);
                        });
}

//==============================================================================
void DecryptedView::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    const auto p = e.position;

    for (auto c : { Column::name, Column::category, Column::lock, Column::source, Column::date })
        if (headingBounds (c).contains (p))
        {
            sortBy (c);
            return;
        }

    if (categoryChip.contains (p))      { showCategoryMenu (categoryChip.toNearestInt()); return; }
    if (renameChip.contains (p))        { askRename(); return; }
    if (deleteChip.contains (p))        { askDelete(); return; }

    const auto at = rowAt (p);

    if (at < 0)
        return;

    if (e.mods.isPopupMenu())
    {
        // A right-click: the row joins what is selected (or is it), and the same three things are offered.
        if (! selection.contains (rows[(size_t) at].name))
            select (at);

        juce::PopupMenu menu;
        menu.addItem (1, "Category...");
        menu.addItem (2, "Rename...", selection.size() == 1);
        menu.addItem (3, selection.size() == 1 ? "Delete..." : "Delete " + juce::String (selection.size()) + " Patches...");
        const auto where = juce::Rectangle<int> (juce::roundToInt (p.x), juce::roundToInt (p.y), 1, 1);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (where)),
                            [safe = juce::Component::SafePointer<DecryptedView> (this), where] (int chosen)
                            {
                                if (safe == nullptr)    return;
                                if (chosen == 1)        safe->showCategoryMenu (where);
                                if (chosen == 2)        safe->askRename();
                                if (chosen == 3)        safe->askDelete();
                            });
        return;
    }

    const auto toggle = e.mods.isCommandDown(), range = e.mods.isShiftDown();
    select (at, toggle, range);

    // One row, clicked plainly: on the keys.
    if (! toggle && ! range)
        hear (at);
}

void DecryptedView::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (const auto at = rowAt (e.position); at >= 0)
        open (at);
}

void DecryptedView::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    scroll -= wheel.deltaY * 240.0f;
    layoutRows();
    repaint();
}

bool DecryptedView::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::downKey)     { step (1); return true; }
    if (key == juce::KeyPress::upKey)       { step (-1); return true; }

    if (key == juce::KeyPress::returnKey && selection.size() == 1)
    {
        for (int i = 0; i < (int) rows.size(); ++i)
            if (rows[(size_t) i].name == selection[0])
                open (i);

        return true;
    }

    if ((key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) && ! selection.isEmpty())
    {
        askDelete();
        return true;
    }

    return false;
}

juce::String DecryptedView::getTooltip()
{
    const auto p = getMouseXYRelative().toFloat();

    if (headings.contains (p))          return "Click a heading to sort by it; again for the other way round";
    if (categoryChip.contains (p))      return "Give the patches selected a category: they are then under it in the patch list's Decrypted folder";
    if (renameChip.contains (p))        return "Rename the patch selected";
    if (deleteChip.contains (p))        return "Delete the patches selected (it asks first)";
    if (rowAt (p) >= 0)                 return "Click to put it on the keys, double-click (or Return) to load it and go to the knobs; Command- and Shift-click select several";
    return {};
}

//==============================================================================
void DecryptedView::paint (juce::Graphics& g)
{
    g.fillAll (Theme::base.darker (0.25f));
    g.setColour (Theme::overlay.withAlpha (0.06f));

    for (float x = 14.0f; x < (float) getWidth(); x += 28.0f)
        for (float y = 14.0f; y < (float) getHeight(); y += 28.0f)
            g.fillRect (x, y, 1.5f, 1.5f);

    text (g, "DECRYPTED", 24.0f, 22.0f, 12.0f, signal(), juce::Justification::centredLeft, true);
    const auto all = (int) DecryptedPatches::Store::shared().entries().size();
    text (g, all == 0 ? juce::String ("the patches you take from Decrypt and name are kept here")
                      : juce::String (all) + (all == 1 ? " patch" : " patches") + " taken from Decrypt" + ((int) rows.size() != all ? ", " + juce::String ((int) rows.size()) + " shown" : juce::String()),
          112.0f, 22.0f, 12.0f, Theme::dimText);

    // The headings: the one sorted by lit, with the way it runs.
    g.setColour (Theme::inset);
    g.fillRoundedRectangle (headings.getUnion (list), 8.0f);
    g.setColour (Theme::line);
    g.drawRoundedRectangle (headings.getUnion (list), 8.0f, 1.0f);
    g.fillRect (headings.getX(), headings.getBottom() + 1.5f, headings.getWidth(), 1.0f);

    for (auto c : { Column::name, Column::category, Column::lock, Column::source, Column::date })
    {
        const auto at = headingBounds (c);
        text (g, juce::String (columnNames[(size_t) c]) + (c == sorted ? (ascending ? "  ^" : "  v") : ""), at.getX() + 12.0f, at.getCentreY(), 10.0f,
              c == sorted ? signal() : Theme::dimText, juce::Justification::centredLeft, true);
    }

    if (rows.empty())
    {
        text (g, all == 0 ? "NOTHING TAKEN YET" : "NOTHING MATCHES", list.getCentreX(), list.getCentreY() - 12.0f, 12.0f, Theme::dimText, juce::Justification::centred, true);
        text (g, all == 0 ? "in Decrypt, press Take when a patch sounds right, and give it a name" : "clear the filter to see them all",
              list.getCentreX(), list.getCentreY() + 10.0f, 11.0f, Theme::faintText, juce::Justification::centred);
    }

    g.saveState();
    g.reduceClipRegion (list.toNearestInt());

    for (int i = 0; i < (int) rows.size(); ++i)
    {
        const juce::Rectangle<float> r (list.getX(), list.getY() + (float) i * rowHeight - scroll, list.getWidth(), rowHeight);

        if (r.getBottom() < list.getY() || r.getY() > list.getBottom())
            continue;

        const auto& e = rows[(size_t) i];
        const auto chosen = selection.contains (e.name);
        const auto onKeys = synth.getPatchName().equalsIgnoreCase (e.name);

        if (chosen)
        {
            g.setColour (Theme::accent.withAlpha (0.28f));
            g.fillRect (r.reduced (2.0f, 1.0f));
        }

        if (onKeys)
        {
            g.setColour (Theme::good);
            g.fillRect (r.getX() + 2.0f, r.getY() + 5.0f, 3.0f, r.getHeight() - 10.0f);
        }

        const auto x = [&r] (int column) { return r.getX() + r.getWidth() * columnAt[(size_t) column] + 12.0f; };
        const auto room = [&r] (int column) { return juce::jmax (4, (int) ((r.getWidth() * (columnAt[(size_t) column + 1] - columnAt[(size_t) column]) - 20.0f) / 7.3f)); };
        const auto y = r.getCentreY();
        text (g, e.name.substring (0, room (0)), x (0), y, 12.0f, chosen || onKeys ? Theme::textStrong : Theme::text, juce::Justification::centredLeft, onKeys);
        text (g, e.category.isEmpty() ? juce::String ("-") : e.category.substring (0, room (1)), x (1), y, 11.5f, e.category.isEmpty() ? Theme::faintText : Theme::accentLight);

        // The lock: its figure, and a bar as long.
        text (g, juce::String (juce::roundToInt (e.lock)) + "%", x (2), y - 3.0f, 11.0f, Theme::text);
        g.setColour (Theme::line);
        g.fillRect (x (2), r.getBottom() - 7.0f, 44.0f, 2.0f);
        g.setColour (signal().withAlpha (0.85f));
        g.fillRect (x (2), r.getBottom() - 7.0f, 44.0f * juce::jlimit (0.0f, 1.0f, e.lock / 100.0f), 2.0f);

        text (g, e.source.substring (0, room (3)), x (3), y, 11.0f, Theme::dimText);
        text (g, dateOf (e.taken), x (4), y, 11.0f, Theme::dimText);
        g.setColour (Theme::line.withAlpha (0.5f));
        g.fillRect (r.getX() + 8.0f, r.getBottom() - 0.5f, r.getWidth() - 16.0f, 0.5f);
    }

    g.restoreState();

    // What can be done with the rows selected.
    const auto some = ! selection.isEmpty(), one = selection.size() == 1;
    drawChip (g, categoryChip, "Category...", Theme::accent, false, some ? Theme::text : Theme::faintText);
    drawChip (g, renameChip, "Rename...", Theme::lineStrong, false, one ? Theme::text : Theme::faintText);
    drawChip (g, deleteChip, "Delete...", Theme::lineStrong, false, some ? Theme::text : Theme::faintText);
    // The hint has the rest of the row: the shared text helper's box is 400 wide, which cut it off.
    const auto hint = juce::Rectangle<float> (deleteChip.getRight() + 16.0f, deleteChip.getY(),
                                               juce::jmax (0.0f, list.getRight() - deleteChip.getRight() - 16.0f), deleteChip.getHeight());
    g.setColour (Theme::faintText);
    g.setFont (mono (10.5f));
    g.drawText (some ? juce::String (selection.size()) + " selected" : juce::String ("click a row to hear it on the keys; double-click to load it and go to the knobs"),
                hint, juce::Justification::centredLeft, true);
}
