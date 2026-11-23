#pragma once

#include "DecryptPatches.h"

/** Torqwave Synth's Decrypted page: every patch taken from Decrypt and named, a row each with its
    name, its category, the lock it was taken at, the recording it came from and the date.

    A click on a row puts the patch on the keys (the app plays it when Audition is on, as it does
    any sound chosen); a double-click or Return loads it and goes to the knobs; the up and down
    arrows step through the rows. Command- and Shift-click select several. The column headings
    sort, and the box at the top right filters by name, category or recording. Category gives the
    rows selected one of the synth's factory categories (or none), Rename renames the one selected
    (an empty or taken name is refused) and Delete deletes those selected, after asking; a
    right-click on a row offers the same. */
class DecryptedView final : public juce::Component,
                            public juce::SettableTooltipClient
{
public:
    using Synth = DecryptedPatches::Synth;

    explicit DecryptedView (Synth&);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void visibilityChanged() override;
    juce::String getTooltip() override;

    /** Reads the patches again (one was taken, renamed or deleted elsewhere). */
    void refresh();

    enum class Column { name, category, lock, source, date };
    /** Sorts by a column; by the one it is sorted by already, the other way round. */
    void sortBy (Column);
    Column sortedBy() const noexcept                { return sorted; }
    bool sortedAscending() const noexcept           { return ascending; }
    void setFilter (const juce::String&);

    /** The rows as they are listed (sorted and filtered). */
    int getNumRows() const noexcept                 { return (int) rows.size(); }
    const DecryptedPatches::Entry& row (int index) const    { return rows[(size_t) index]; }
    juce::Rectangle<float> rowBounds (int index) const;
    juce::Rectangle<float> headingBounds (Column) const;
    juce::Rectangle<float> categoryButton() const   { return categoryChip; }
    juce::Rectangle<float> renameButton() const     { return renameChip; }
    juce::Rectangle<float> deleteButton() const     { return deleteChip; }

    /** Selects a row: alone, or added to (or taken from) the selection, or with every row from the last one selected. */
    void select (int index, bool toggle = false, bool range = false);
    juce::StringArray selected() const              { return selection; }

    /** Puts a row's patch on the keys; and the same, then the knobs (onOpen). False if it couldn't be loaded. */
    bool hear (int index);
    bool open (int index);
    /** Steps the selection (and the keys) a row on or back, stopping at the ends. */
    void step (int by);

    /** The rows selected, without asking: a category (empty: none); a new name for the one selected
        (false, nothing changed, when it is empty or taken, or more than one is selected); deleted. */
    int categoriseSelected (const juce::String& category);
    bool renameSelected (const juce::String& to);
    int deleteSelected();

    std::function<void()> onOpen;       // a patch was opened: show the knobs
    std::function<void()> onLoaded;     // a patch was put on the keys: the window's knobs and name show it
    std::function<void()> onChanged;    // patches were renamed, deleted or given a category

private:
    void layoutRows();
    void askRename();
    void askDelete();
    void showCategoryMenu (juce::Rectangle<int> at);
    int rowAt (juce::Point<float>) const;
    void keepInView (int index);

    Synth& synth;
    std::vector<DecryptedPatches::Entry> rows;
    juce::StringArray selection;
    juce::String anchor;                // the row a Shift-click's range runs from
    Column sorted = Column::name;
    bool ascending = true;
    juce::TextEditor filter;
    juce::Rectangle<float> list, headings, categoryChip, renameChip, deleteChip;
    float scroll = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DecryptedView)
};
