#include "SequencePropertiesDialog.h"
#include "NoteTools.h"
#include "Theme.h"

namespace
{
    const juce::String noChoice { juce::String::fromUTF8 ("\xe2\x80\x94") };

    // "Techno › Dark" or "Techno > Dark" as written back: "Techno/Dark".
    juce::String styleFrom (const juce::String& shown)
    {
        auto s = shown.replace (juce::String::fromUTF8 (" \xe2\x80\xba "), "/").replace (" > ", "/").trim();
        return s == noChoice ? juce::String() : s;
    }

    juce::StringArray keyChoices()
    {
        static const char* roots[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
        juce::StringArray keys;

        for (auto* mode : { "minor", "major" })
            for (auto* root : roots)
                keys.add (juce::String (root) + " " + mode);

        return keys;
    }
}

SequencePropertiesDialog::SequencePropertiesDialog (Options o, const SequenceLibrary::Properties& p) : options (std::move (o))
{
    ok.setButtonText (options.okText);

    if (options.intro.isNotEmpty())
    {
        intro.setText (options.intro, juce::dontSendNotification);
        intro.setJustificationType (juce::Justification::topLeft);
        intro.setFont (juce::FontOptions (13.0f));
        addAndMakeVisible (intro);
    }

    if (options.askName)
    {
        name.setText (options.name, false);
        addRow ("Name", name);
        folder.addItemList (options.folders, 1);
        folder.setSelectedItemIndex (juce::jmax (0, options.folders.indexOf (options.folder)), juce::dontSendNotification);
        addRow ("Folder", folder);
    }

    auto styles = options.styles;

    if (styles.isEmpty())
    {
        auto entries = SequenceLibrary (SequenceLibrary::factoryFolder()).scan();
        auto mine = SequenceLibrary{}.scan();
        entries.insert (entries.end(), mine.begin(), mine.end());
        styles = SequenceLibrary::stylesIn (entries);
    }

    style.setEditableText (true);
    style.addItem (noChoice, 1);

    for (int i = 0; i < styles.size(); ++i)
        style.addItem (SequenceFile::styleName (styles[i]), i + 2);

    style.setText (p.style.isNotEmpty() ? SequenceFile::styleName (p.style) : noChoice, juce::dontSendNotification);
    style.setTooltip ("A genre and a style, such as Floor/Dark Floor or Machine/Machine Pulse: pick one or type your own");
    addRow ("Style", style);

    key.setEditableText (true);
    key.addItem (noChoice, 1);
    key.addItemList (keyChoices(), 2);
    key.setText (p.key.isNotEmpty() ? p.key : noChoice, juce::dontSendNotification);
    key.setTooltip ("Its key, such as A minor or F# phrygian; none for drums");
    addRow ("Key", key);

    if (! options.isKit)
    {
        energy.addItem (noChoice, 1);

        for (int e = 1; e <= 5; ++e)
            energy.addItem (juce::String (e) + (e == 1 ? " (intro tool)" : e == 5 ? " (peak)" : ""), e + 1);

        energy.setSelectedId (juce::jlimit (0, 5, p.energy) + 1, juce::dontSendNotification);
        energy.setTooltip ("How much it drives, from 1 (an intro tool) to 5 (peak time)");
        addRow ("Energy", energy);

        for (auto& [first, second] : SequenceFile::characterPairs())
        {
            auto* box = character.add (new juce::ComboBox (first));
            box->addItem (noChoice, 1);
            box->addItem (first, 2);
            box->addItem (second, 3);
            box->setSelectedId (p.character.contains (first) ? 2 : p.character.contains (second) ? 3 : 1, juce::dontSendNotification);
            addRow (first + " or " + second, *box);
        }
    }

    description.setText (p.description, false);
    description.setTextToShowWhenEmpty ("One line on what it does", Theme::dimText);
    addRow ("Description", description);

    tags.setText (p.tags.joinIntoString (", "), false);
    tags.setTextToShowWhenEmpty ("Separated by commas", Theme::dimText);
    addRow ("Tags", tags);

    addAndMakeVisible (ok);
    addAndMakeVisible (cancel);
    ok.onClick = [this]
    {
        if (onOk != nullptr)
            onOk (result());

        if (auto* w = findParentComponentOfClass<juce::DialogWindow>())
            w->exitModalState (1);
    };
    cancel.onClick = [this]
    {
        if (auto* w = findParentComponentOfClass<juce::DialogWindow>())
            w->exitModalState (0);
    };

    setSize (460, (options.intro.isNotEmpty() ? 56 : 0) + (int) rows.size() * 30 + 56);
}

void SequencePropertiesDialog::addRow (const juce::String& text, juce::Component& editor)
{
    auto* label = labels.add (new juce::Label ({}, text));
    label->setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (label);
    addAndMakeVisible (editor);
    rows.push_back (&editor);
}

SequencePropertiesDialog::Result SequencePropertiesDialog::result() const
{
    Result r;
    r.name = name.getText().trim();
    r.folder = options.folders[folder.getSelectedItemIndex()];
    r.properties.style = styleFrom (style.getText());
    r.properties.key = key.getText().trim() == noChoice ? juce::String() : key.getText().trim();
    r.properties.description = description.getText().trim();
    r.properties.energy = options.isKit ? 0 : juce::jmax (0, energy.getSelectedId() - 1);
    r.properties.tags.addTokens (tags.getText(), ",", {});
    r.properties.tags.trim();
    r.properties.tags.removeEmptyStrings();

    for (int i = 0; i < character.size(); ++i)
        if (character[i]->getSelectedId() > 1)
            r.properties.character.add (character[i]->getText());

    return r;
}

juce::Component* SequencePropertiesDialog::find (const juce::String& what) const
{
    if (what == "name") return const_cast<juce::TextEditor*> (&name);
    if (what == "folder") return const_cast<juce::ComboBox*> (&folder);
    if (what == "style") return const_cast<juce::ComboBox*> (&style);
    if (what == "key") return const_cast<juce::ComboBox*> (&key);
    if (what == "energy") return const_cast<juce::ComboBox*> (&energy);
    if (what == "description") return const_cast<juce::TextEditor*> (&description);
    if (what == "tags") return const_cast<juce::TextEditor*> (&tags);

    for (auto* box : character)
        if (box->getName() == what)
            return box;

    return nullptr;
}

void SequencePropertiesDialog::show (juce::Component* parent, Options o, const SequenceLibrary::Properties& p, std::function<void (const Result&)> onOk)
{
    auto title = o.title;
    auto content = std::make_unique<SequencePropertiesDialog> (std::move (o), p);
    content->onOk = std::move (onOk);

    juce::DialogWindow::LaunchOptions launch;
    launch.dialogTitle = title;
    launch.content.setOwned (content.release());
    launch.componentToCentreAround = parent;
    launch.dialogBackgroundColour = Theme::surface;
    launch.escapeKeyTriggersCloseButton = true;
    launch.useNativeTitleBar = true;
    launch.resizable = false;
    launch.launchAsync();
}

void SequencePropertiesDialog::paint (juce::Graphics& g)
{
    g.fillAll (Theme::surface);
}

void SequencePropertiesDialog::resized()
{
    auto r = getLocalBounds().reduced (12);

    if (options.intro.isNotEmpty())
        intro.setBounds (r.removeFromTop (50));

    for (size_t i = 0; i < rows.size(); ++i)
    {
        auto row = r.removeFromTop (30).reduced (0, 3);
        labels[(int) i]->setBounds (row.removeFromLeft (130));
        row.removeFromLeft (6);
        rows[i]->setBounds (row);
    }

    auto buttons = r.removeFromBottom (28);
    ok.setBounds (buttons.removeFromRight (90));
    buttons.removeFromRight (8);
    cancel.setBounds (buttons.removeFromRight (90));
}
