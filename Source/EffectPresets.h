#pragma once

#include <JuceHeader.h>

/** Named presets for effects. A preset is a set of named values (each a setting's plain value: Hz,
    dB, ms, a percentage, a choice's index, 1 or 0 for on or off), kept under the effect's key in a
    file of its own. An effect's dialog and its live built-in effect use the same key and the same
    names for their settings (the live effect's parameter IDs), so a preset saved in one loads in
    the other. Values round-trip exactly. */
namespace EffectPresets
{
    using Values = std::map<juce::String, double>;

    class Store
    {
    public:
        /** Presets kept as one file an effect in this folder (made when the first is saved). */
        explicit Store (juce::File folder);

        /** The effect's presets' names, sorted, ignoring case. */
        juce::StringArray getNames (const juce::String& effectKey) const;

        std::optional<Values> load (const juce::String& effectKey, const juce::String& name) const;

        /** Saves (or replaces) the named preset; false if the file can't be written or the name is empty. */
        bool save (const juce::String& effectKey, const juce::String& name, const Values&);

        /** Removes the named preset; false if there was none or the file can't be written. */
        bool remove (const juce::String& effectKey, const juce::String& name);

        juce::File getFolder() const                    { return folder; }

    private:
        juce::File fileFor (const juce::String& effectKey) const;
        std::unique_ptr<juce::XmlElement> read (const juce::String& effectKey) const;

        juce::File folder;
    };

    /** The user's presets, in the app's data folder (Application Support/Torqwave/Presets). */
    Store& userStore();

    /** A preset that comes with Torqwave: a starting point for a common job. It can't be
        changed or deleted; one of the user's own may have the same name. */
    struct Factory
    {
        juce::String name;
        Values values;
        juce::String category = {};    // a sub-menu of the list to show it in; none, the list itself
    };

    /** The effect's factory presets, in the order they are listed (none for an effect without any). */
    const std::vector<Factory>& factory (const juce::String& effectKey);

    /** Every effect key with factory presets. */
    juce::StringArray factoryKeys();

    /** A row for a dialog or window: the effect's presets in a menu, the factory ones first, then
        the user's (choosing one loads it through `apply`), Save... (asks for a name and saves
        `current()` as one of the user's) and Delete (asks, then removes the user's preset chosen).
        Two arrows before the menu step to the previous and the next preset in the menu's order
        (through the factory ones' sub-menus in turn, then the user's), round from the last to the
        first; so do the left and right arrow keys while the row has the keyboard, and the mouse
        wheel over the menu. */
    class Bar final : public juce::Component
    {
    public:
        Bar (Store&, juce::String effectKey, std::function<Values()> current, std::function<void (const Values&)> apply);
        ~Bar() override;

        /** Reads the names again (another window may have saved one). */
        void refresh();

        /** Loads the preset `by` places on in the menu's order (1 the next, -1 the previous), round
            the ends; with none chosen, the first (or the last, going back). False with no presets. */
        bool step (int by);

        /** The presets' names in the order the arrows go through them. */
        juce::StringArray getOrder() const;

        /** Called after a preset is loaded, from the menu or by a step. */
        std::function<void()> onChosen;

        /** Shows a toggle after the arrows (an instrument's window: "play each sound as it is
            chosen"), set to `on`; `changed` is told when it is clicked. */
        void showAudition (bool on, std::function<void (bool)> changed);
        void setAudition (bool on)                  { audition.setToggleState (on, juce::dontSendNotification); }

        /** The chosen preset's name; empty with none chosen. */
        juce::String getChosen() const              { return list.getSelectedId() > 0 ? list.getText() : juce::String(); }

        void resized() override;
        bool keyPressed (const juce::KeyPress&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    private:
        std::vector<int> orderedIds() const;
        void saveAs();
        void removeChosen();

        static constexpr int userBase = 1000;   // the user's presets' item IDs come after this; factory ones from 1

        Store& store;
        juce::String key;
        std::function<Values()> currentValues;
        std::function<void (const Values&)> applyValues;
        juce::ComboBox list;
        juce::TextButton save { "Save..." }, remove { "Delete" };
        juce::TextButton audition { juce::String::fromUTF8 ("\xe2\x99\xaa") };
        std::function<void (bool)> auditionChanged;
        juce::TextButton previous { juce::String::fromUTF8 ("\xe2\x80\xb9") }, next { juce::String::fromUTF8 ("\xe2\x80\xba") };
        float wheel = 0.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Bar)
    };
}
