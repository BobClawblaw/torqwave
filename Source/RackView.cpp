#include "RackView.h"
#include "Platform.h"
#include "Theme.h"
#include "PluginMenu.h"

//==============================================================================
class RackView::SlotRow final : public juce::Component
{
public:
    SlotRow (RackView& o, int slotIndex) : owner (o), index (slotIndex)
    {
        auto* plugin = owner.pluginAt (index);
        const auto description = plugin != nullptr ? plugin->getPluginDescription() : juce::PluginDescription();

        name.setText (juce::String (index + 1) + ". " + description.name, juce::dontSendNotification);
        name.setTooltip (description.manufacturerName + " - " + description.pluginFormatName);
        format.setText (description.pluginFormatName, juce::dontSendNotification);
        Theme::follow (format, juce::Label::textColourId, Theme::dimText);

        bypass.setToggleState (! owner.isBypassed (index), juce::dontSendNotification);
        bypass.setTooltip ("On/off (bypass)");
        bypass.onClick = [this] { owner.setBypassed (index, ! bypass.getToggleState()); };

        edit.onClick   = [this] { owner.edit (index); };
        up.onClick     = [this] { owner.move (index, index - 1); };
        down.onClick   = [this] { owner.move (index, index + 1); };
        remove.onClick = [this] { owner.remove (index); };

        up.setEnabled (index > 0);
        down.setEnabled (index < owner.numSlots() - 1);

        for (auto* c : std::initializer_list<juce::Component*> { &bypass, &name, &format, &edit, &up, &down, &remove })
            addAndMakeVisible (c);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (Theme::raised);
        g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 4.0f);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (6, 4);
        bypass.setBounds (r.removeFromLeft (28));
        remove.setBounds (r.removeFromRight (28));
        down.setBounds (r.removeFromRight (28));
        up.setBounds (r.removeFromRight (28));
        edit.setBounds (r.removeFromRight (48).reduced (2, 0));
        format.setBounds (r.removeFromRight (44));
        name.setBounds (r);
    }

private:
    RackView& owner;
    const int index;
    juce::ToggleButton bypass;
    juce::Label name, format;
    juce::TextButton edit { "Edit" }, up { juce::String::fromUTF8 ("\xe2\x96\xb2") },
                     down { juce::String::fromUTF8 ("\xe2\x96\xbc") }, remove { "x" };
};

//==============================================================================
RackView::RackView (AudioEngine& e) : engine (e)
{
    title.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    addAndMakeVisible (title);
    addAndMakeVisible (addButton);
    addButton.onClick = [this] { showAddMenu(); };
    addAndMakeVisible (applyButton);
    applyButton.setTooltip ("Render the rack's effects into the selection, or the whole file (Cmd-R)");
    applyButton.onClick = [this] { if (onApply != nullptr) onApply(); };

    engine.addChangeListener (this);
    engine.session.addChangeListener (this);
    rebuildRows();
}

//==============================================================================
juce::uint32 RackView::masterId() const
{
    const auto state = engine.session.getState();
    const auto* master = masterBus ? state->findMaster() : nullptr;
    return master != nullptr ? master->id : 0u;
}

int RackView::numSlots() const
{
    const auto id = masterId();
    return id != 0 ? engine.getTrackEffects().getNumSlots (id) : engine.getNumRackSlots();
}

juce::AudioPluginInstance* RackView::pluginAt (int i) const
{
    const auto id = masterId();
    return id != 0 ? engine.getTrackEffects().getPlugin (id, i) : engine.getRackPlugin (i);
}

bool RackView::isBypassed (int i) const
{
    const auto id = masterId();
    return id != 0 ? engine.getTrackEffects().isBypassed (id, i) : engine.isRackSlotBypassed (i);
}

void RackView::setBypassed (int i, bool bypassed)
{
    if (const auto id = masterId(); id != 0)
        engine.setTrackPluginBypassed (id, i, bypassed);
    else
        engine.setRackSlotBypassed (i, bypassed);
}

void RackView::move (int from, int to)
{
    if (const auto id = masterId(); id != 0)
        engine.moveTrackPlugin (id, from, to);
    else
        engine.moveRackSlot (from, to);
}

void RackView::remove (int i)
{
    if (const auto id = masterId(); id != 0)
        engine.removeTrackPlugin (id, i);
    else
        engine.removeRackSlot (i);
}

void RackView::edit (int i)
{
    if (masterId() != 0)
    {
        if (auto* plugin = pluginAt (i); plugin != nullptr && onEditPlugin != nullptr)
            onEditPlugin (*plugin);
    }
    else if (onEditSlot != nullptr)
    {
        onEditSlot (i);
    }
}

void RackView::setMasterBus (bool on)
{
    masterBus = on;
    title.setText (on ? "Master Bus Effects" : "Effects Rack", juce::dontSendNotification);
    title.setTooltip (on ? "The Master track's effects: the session's whole mix goes through them, after the tracks and buses and before the "
                           "master volume, and Mix Down writes through them. They are saved with the session and their parameters can be "
                           "automated on the Master track. Each track's own effects are on its FX button."
                         : juce::String());
    applyButton.setVisible (! on);
    rebuildRows();
}

RackView::~RackView()
{
    engine.session.removeChangeListener (this);
    engine.removeChangeListener (this);
}

void RackView::paint (juce::Graphics& g)
{
    g.fillAll (Theme::surface);

    if (rows.isEmpty())
    {
        g.setColour (Theme::dimText);
        g.drawFittedText (masterBus ? juce::String ("No effects on the master bus.\nWhat you add here plays on the whole mix, after the tracks and buses, and is saved with the session.")
                                    : "No effects.\nScan your plug-ins, then add " + Platform::pluginFormats() + " effects here.",
                          getLocalBounds().reduced (16).withTrimmedTop (80), juce::Justification::centredTop, 4);
    }
}

void RackView::resized()
{
    auto r = getLocalBounds().reduced (8);
    auto header = r.removeFromTop (28);
    addButton.setBounds (header.removeFromRight (110));
    header.removeFromRight (6);
    if (applyButton.isVisible())
        applyButton.setBounds (header.removeFromRight (64));

    title.setBounds (header);
    r.removeFromTop (8);

    for (auto* row : rows)
    {
        row->setBounds (r.removeFromTop (36));
        r.removeFromTop (4);
    }
}

void RackView::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // The engine also speaks for device changes. Rows are rebuilt only when the rack itself changed,
    // since a row replaced between a button's press and release loses the click.
    std::vector<std::pair<const void*, bool>> now;

    for (int i = 0; i < numSlots(); ++i)
        now.emplace_back (pluginAt (i), isBypassed (i));

    if (now != shown)
        rebuildRows();
}

void RackView::rebuildRows()
{
    rows.clear();
    shown.clear();

    for (int i = 0; i < numSlots(); ++i)
        shown.emplace_back (pluginAt (i), isBypassed (i));

    for (int i = 0; i < numSlots(); ++i)
        addAndMakeVisible (rows.add (new SlotRow (*this, i)));

    applyButton.setEnabled (! rows.isEmpty());
    resized();
    repaint();
}

void RackView::showAddMenu()
{
    // Effects only: instruments need MIDI and belong on tracks, which come in a later milestone.
    juce::Array<juce::PluginDescription> effects;

    for (auto& type : engine.knownPlugins.getTypes())
        if (! type.isInstrument)
            effects.add (type);

    if (effects.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "No effects found",
                                                "Use Plug-ins... to scan for " + Platform::pluginFormats() + " effects first.");
        return;
    }

    juce::PopupMenu menu;
    PluginMenu::add (menu, effects);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (addButton),
        [this, effects] (int result)
        {
            const auto chosen = PluginMenu::chosen (effects, result);

            if (! juce::isPositiveAndBelow (chosen, effects.size()))
                return;

            addEffect (effects[chosen]);
        });
}

void RackView::addEffect (const juce::PluginDescription& description)
{
    const auto name = description.name;

    if (const auto id = masterId(); id != 0)
    {
        engine.addTrackPlugin (id, description, [this, name, id, safe = juce::Component::SafePointer<RackView> (this)] (const juce::String& error)
        {
            if (error.isNotEmpty())
            {
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Could not load " + name, error);
                return;
            }

            const auto slots = engine.getTrackEffects().getNumSlots (id);

            if (safe != nullptr && onEditPlugin != nullptr && slots > 0)
                if (auto* plugin = engine.getTrackEffects().getPlugin (id, slots - 1))
                    onEditPlugin (*plugin);
        });
        return;
    }

    engine.addPlugin (description, [this, name, safe = juce::Component::SafePointer<RackView> (this)] (const juce::String& error)
    {
        if (error.isNotEmpty())
        {
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Could not load " + name, error);
            return;
        }

        if (safe != nullptr && onEditSlot != nullptr && engine.getNumRackSlots() > 0)
            onEditSlot (engine.getNumRackSlots() - 1);
    });
}
