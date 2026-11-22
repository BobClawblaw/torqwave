#include "EffectPresets.h"
#include "PreviousName.h"

#include <sstream>

namespace EffectPresets
{
namespace
{
    /** 17 significant digits, which reads back as the same double. */
    juce::String exactly (double v)
    {
        std::ostringstream s;
        s.imbue (std::locale::classic());
        s.precision (17);
        s << v;
        return s.str();
    }

    double parse (const juce::String& text)
    {
        std::istringstream s (text.toStdString());
        s.imbue (std::locale::classic());
        double v = 0.0;
        s >> v;
        return v;
    }
}

Store::Store (juce::File f) : folder (std::move (f)) {}

juce::File Store::fileFor (const juce::String& effectKey) const
{
    // "torqwave:compressor" keeps its presets in compressor.xml.
    return folder.getChildFile (juce::File::createLegalFileName (effectKey.fromLastOccurrenceOf (":", false, false)) + ".xml");
}

std::unique_ptr<juce::XmlElement> Store::read (const juce::String& effectKey) const
{
    auto xml = juce::XmlDocument::parse (fileFor (effectKey));

    if (xml == nullptr || ! PreviousName::hasTag (*xml, "TORQWAVE_PRESETS"))
        return std::make_unique<juce::XmlElement> ("TORQWAVE_PRESETS");

    // Presets saved by SpectraBlade name their effect "spectrablade:compressor".
    if (const auto effect = xml->getStringAttribute ("effect");
        effect != effectKey && effect != "spectrablade:" + effectKey.fromFirstOccurrenceOf (":", false, false))
        return std::make_unique<juce::XmlElement> ("TORQWAVE_PRESETS");

    xml->setTagName ("TORQWAVE_PRESETS");
    xml->setAttribute ("effect", effectKey);
    return xml;
}

juce::StringArray Store::getNames (const juce::String& effectKey) const
{
    juce::StringArray names;
    const auto xml = read (effectKey);

    for (auto* p : xml->getChildWithTagNameIterator ("PRESET"))
        names.addIfNotAlreadyThere (p->getStringAttribute ("name"));

    names.removeEmptyStrings();
    names.sortNatural();
    return names;
}

std::optional<Values> Store::load (const juce::String& effectKey, const juce::String& name) const
{
    const auto xml = read (effectKey);

    for (auto* p : xml->getChildWithTagNameIterator ("PRESET"))
    {
        if (p->getStringAttribute ("name") != name)
            continue;

        Values v;

        for (auto* value : p->getChildWithTagNameIterator ("VALUE"))
            v[value->getStringAttribute ("id")] = parse (value->getStringAttribute ("value"));

        return v;
    }

    return std::nullopt;
}

bool Store::save (const juce::String& effectKey, const juce::String& name, const Values& values)
{
    if (name.trim().isEmpty())
        return false;

    auto xml = read (effectKey);
    xml->setAttribute ("effect", effectKey);

    if (auto* old = xml->getChildByAttribute ("name", name))
        xml->removeChildElement (old, true);

    auto* p = xml->createNewChildElement ("PRESET");
    p->setAttribute ("name", name);

    for (auto& [id, v] : values)
    {
        auto* e = p->createNewChildElement ("VALUE");
        e->setAttribute ("id", id);
        e->setAttribute ("value", exactly (v));
    }

    return folder.createDirectory().wasOk() && xml->writeTo (fileFor (effectKey));
}

bool Store::remove (const juce::String& effectKey, const juce::String& name)
{
    auto xml = read (effectKey);
    auto* old = xml->getChildByAttribute ("name", name);

    if (old == nullptr)
        return false;

    xml->removeChildElement (old, true);
    return xml->writeTo (fileFor (effectKey));
}

Store& userStore()
{
    static Store store (juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                            .getChildFile ("Torqwave").getChildFile ("Presets"));
    return store;
}

//==============================================================================
Bar::Bar (Store& s, juce::String effectKey, std::function<Values()> current, std::function<void (const Values&)> apply)
    : store (s), key (std::move (effectKey)), currentValues (std::move (current)), applyValues (std::move (apply))
{
    list.setTextWhenNothingSelected ("Presets");
    list.setTextWhenNoChoicesAvailable ("No presets saved");
    list.onChange = [this]
    {
        const auto id = list.getSelectedId();
        remove.setEnabled (id > userBase);

        if (id > userBase)
        {
            if (const auto v = store.load (key, list.getText()))
                applyValues (*v);
        }
        else if (id > 0)
        {
            const auto& made = factory (key);

            if (juce::isPositiveAndNotGreaterThan (id, (int) made.size()))
                applyValues (made[(size_t) id - 1].values);
        }

        if (id > 0 && onChosen != nullptr)
            onChosen();
    };
    save.onClick = [this] { saveAs(); };
    remove.onClick = [this] { removeChosen(); };
    addAndMakeVisible (list);
    addAndMakeVisible (save);
    addAndMakeVisible (remove);
    previous.onClick = [this] { step (-1); };
    next.onClick = [this] { step (1); };
    previous.setTooltip ("Previous preset (left arrow key)");
    next.setTooltip ("Next preset (right arrow key)");
    previous.setWantsKeyboardFocus (false);
    next.setWantsKeyboardFocus (false);
    addAndMakeVisible (previous);
    addAndMakeVisible (next);
    audition.setClickingTogglesState (true);
    audition.setWantsKeyboardFocus (false);
    audition.setTooltip ("Audition: with this on, each sound plays as you choose it (the arrows, the list, the left and right keys). Loop and Stop are beside the keys at the bottom of the window.");
    audition.onClick = [this]
    {
        if (auditionChanged != nullptr)
            auditionChanged (audition.getToggleState());
    };
    addChildComponent (audition);
    list.addMouseListener (this, false);
    setWantsKeyboardFocus (true);
    refresh();
    setSize (320, 28);
}

Bar::~Bar() = default;

void Bar::refresh()
{
    const auto chosenId = list.getSelectedId();
    const auto chosen = list.getText();
    list.clear (juce::dontSendNotification);
    const auto& made = factory (key);
    const auto names = store.getNames (key);

    if (! made.empty())
    {
        list.addSectionHeading ("Factory");

        // Those with a category in a sub-menu each, in the order the categories first come; the rest in the list.
        juce::StringArray categories;

        for (auto& f : made)
            if (f.category.isNotEmpty())
                categories.addIfNotAlreadyThere (f.category);

        for (auto& c : categories)
        {
            juce::PopupMenu sub;

            for (size_t i = 0; i < made.size(); ++i)
                if (made[i].category == c)
                    sub.addItem ((int) i + 1, made[i].name);

            list.getRootMenu()->addSubMenu (c, sub);
        }

        for (size_t i = 0; i < made.size(); ++i)
            if (made[i].category.isEmpty())
                list.addItem (made[i].name, (int) i + 1);
    }

    if (! names.isEmpty())
    {
        if (! made.empty())
            list.addSectionHeading ("Yours");

        for (int i = 0; i < names.size(); ++i)
            list.addItem (names[i], userBase + i + 1);
    }

    // The same one chosen again: a factory preset by its place, one of the user's by its name.
    if (chosenId > 0 && chosenId <= userBase && chosenId <= (int) made.size())
        list.setSelectedId (chosenId, juce::dontSendNotification);
    else if (names.contains (chosen))
        list.setSelectedId (userBase + names.indexOf (chosen) + 1, juce::dontSendNotification);

    remove.setEnabled (list.getSelectedId() > userBase);
    const auto any = ! made.empty() || ! names.isEmpty();
    previous.setEnabled (any);
    next.setEnabled (any);
}

void Bar::showAudition (bool on, std::function<void (bool)> changed)
{
    auditionChanged = std::move (changed);
    audition.setToggleState (on, juce::dontSendNotification);
    audition.setVisible (true);
    resized();
}

std::vector<int> Bar::orderedIds() const
{
    // As the menu shows them: each category's in turn, then the factory ones without one, then the user's.
    std::vector<int> ids;
    const auto& made = factory (key);
    juce::StringArray categories;

    for (auto& f : made)
        if (f.category.isNotEmpty())
            categories.addIfNotAlreadyThere (f.category);

    for (auto& c : categories)
        for (size_t i = 0; i < made.size(); ++i)
            if (made[i].category == c)
                ids.push_back ((int) i + 1);

    for (size_t i = 0; i < made.size(); ++i)
        if (made[i].category.isEmpty())
            ids.push_back ((int) i + 1);

    for (int i = 0; i < store.getNames (key).size(); ++i)
        ids.push_back (userBase + i + 1);

    return ids;
}

juce::StringArray Bar::getOrder() const
{
    juce::StringArray order;
    const auto& made = factory (key);
    const auto names = store.getNames (key);

    for (auto id : orderedIds())
        order.add (id > userBase ? names[id - userBase - 1] : made[(size_t) id - 1].name);

    return order;
}

bool Bar::step (int by)
{
    const auto ids = orderedIds();

    if (ids.empty() || by == 0)
        return false;

    const auto count = (int) ids.size();
    const auto found = std::find (ids.begin(), ids.end(), list.getSelectedId());
    const auto at = found != ids.end() ? (int) (found - ids.begin()) + by : (by > 0 ? 0 : count - 1);
    list.setSelectedId (ids[(size_t) (((at % count) + count) % count)], juce::sendNotificationSync);
    return true;
}

bool Bar::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::leftKey || k == juce::KeyPress::rightKey)
        return step (k == juce::KeyPress::rightKey ? 1 : -1);

    return false;
}

void Bar::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& d)
{
    // A notch of a wheel, or a third of a line's worth of a trackpad's glide, is one preset.
    wheel += d.deltaY;

    while (std::abs (wheel) >= 0.3f)
    {
        step (wheel > 0.0f ? -1 : 1);
        wheel -= wheel > 0.0f ? 0.3f : -0.3f;
    }
}

void Bar::saveAs()
{
    auto* box = new juce::AlertWindow ("Save Preset", "A name for these settings (an existing name is replaced).", juce::MessageBoxIconType::NoIcon, this);
    box->addTextEditor ("name", list.getText(), "Name");
    box->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    box->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    box->enterModalState (true, juce::ModalCallbackFunction::create ([safe = juce::Component::SafePointer<Bar> (this), box] (int result)
    {
        const auto name = box->getTextEditorContents ("name").trim();

        if (safe == nullptr || result != 1 || name.isEmpty())
            return;

        if (! safe->store.save (safe->key, name, safe->currentValues()))
        {
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Save Preset",
                                                    "The preset couldn't be written to " + safe->store.getFolder().getFullPathName() + ".");
            return;
        }

        safe->list.setSelectedId (0, juce::dontSendNotification);
        safe->refresh();
        safe->list.setSelectedId (userBase + safe->store.getNames (safe->key).indexOf (name) + 1, juce::dontSendNotification);
        safe->remove.setEnabled (true);
    }), true);
}

void Bar::removeChosen()
{
    const auto name = list.getText();

    if (list.getSelectedId() <= userBase || ! store.getNames (key).contains (name))
        return;

    juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon, "Delete Preset", "Delete the preset " + name.quoted() + "?",
                                        "Delete", "Cancel", this,
                                        juce::ModalCallbackFunction::create ([safe = juce::Component::SafePointer<Bar> (this), name] (int result)
    {
        if (safe == nullptr || result != 1)
            return;

        safe->store.remove (safe->key, name);
        safe->list.setSelectedId (0, juce::dontSendNotification);
        safe->refresh();
    }));
}

void Bar::resized()
{
    auto r = getLocalBounds();
    remove.setBounds (r.removeFromRight (64));
    r.removeFromRight (4);
    save.setBounds (r.removeFromRight (70));
    r.removeFromRight (6);
    previous.setBounds (r.removeFromLeft (24));
    r.removeFromLeft (2);
    next.setBounds (r.removeFromLeft (24));

    if (audition.isVisible())
    {
        // By name where there is room for it, by its note alone where there is not.
        const auto named = getWidth() >= 460;
        audition.setButtonText (named ? juce::String::fromUTF8 ("\xe2\x99\xaa Audition") : juce::String::fromUTF8 ("\xe2\x99\xaa"));
        r.removeFromLeft (2);
        audition.setBounds (r.removeFromLeft (named ? 84 : 26));
    }

    r.removeFromLeft (6);
    list.setBounds (r);
}
}
