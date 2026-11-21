#pragma once

#include "SequenceLibrary.h"

/** Sequence Properties: a sequence's (or kit's) style, energy, character, key, description and
    tags, and for Add to Sequence Library its name and folder. Style and key take a choice or
    what is typed ("Techno/Dark", "F# minor"); a kit has no energy or character of its own. */
class SequencePropertiesDialog final : public juce::Component
{
public:
    struct Options
    {
        juce::String title { "Sequence Properties" }, intro, okText { "OK" };
        bool isKit = false;
        bool askName = false;             // a name and a folder too
        juce::String name, folder;
        juce::StringArray folders;
        juce::StringArray styles;         // offered; empty: those in the presets and the library
    };

    struct Result
    {
        SequenceLibrary::Properties properties;
        juce::String name, folder;
    };

    SequencePropertiesDialog (Options, const SequenceLibrary::Properties&);

    /** What is filled in now (a style, key and description trimmed; "Techno › Dark" read back as "Techno/Dark"). */
    Result result() const;

    /** For tests: the editors and choices, by name ("style", "key", "energy", "description",
        "tags", "name", "folder", and each pair's first pole, "Dark" ...). */
    juce::Component* find (const juce::String& name) const;

    /** Opens it in a window over `parent`; `onOk` gets what was filled in. */
    static void show (juce::Component* parent, Options, const SequenceLibrary::Properties&, std::function<void (const Result&)> onOk);

    void resized() override;
    void paint (juce::Graphics&) override;

    std::function<void (const Result&)> onOk;

private:
    void addRow (const juce::String& label, juce::Component&);

    Options options;
    juce::Label intro;
    juce::TextEditor name, description, tags;
    juce::ComboBox folder, style, key, energy;
    juce::OwnedArray<juce::ComboBox> character;   // one per pair: none, its first pole, its second
    juce::OwnedArray<juce::Label> labels;
    std::vector<juce::Component*> rows;
    juce::TextButton ok, cancel { "Cancel" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequencePropertiesDialog)
};
