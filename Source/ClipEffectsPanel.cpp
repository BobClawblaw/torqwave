#include "ClipEffectsPanel.h"
#include "PluginMenu.h"
#include "Theme.h"

/** A slot's plug-in in a window of its own, made for it from the slot's state. */
class ClipEffectsPanel::EffectWindow final : public juce::DocumentWindow
{
public:
    EffectWindow (std::unique_ptr<juce::AudioPluginInstance> p, std::function<void()> onClose)
        : DocumentWindow (p->getName(), Theme::surface, DocumentWindow::closeButton), plugin (std::move (p)), closed (std::move (onClose))
    {
        juce::AudioProcessorEditor* editor = plugin->hasEditor() ? plugin->createEditorAndMakeActive() : nullptr;

        if (editor == nullptr)
            editor = new juce::GenericAudioProcessorEditor (*plugin);

        setContentOwned (editor, true);
        setResizable (editor->isResizable(), false);
        setUsingNativeTitleBar (true);
        centreWithSize (getWidth(), getHeight());
        setVisible (true);
    }

    ~EffectWindow() override
    {
        // The editor goes before the plug-in, which is told it has gone.
        if (auto* editor = dynamic_cast<juce::AudioProcessorEditor*> (getContentComponent()))
            plugin->editorBeingDeleted (editor);

        clearContentComponent();
    }

    void closeButtonPressed() override   { closed(); }

    juce::MemoryBlock state() const
    {
        juce::MemoryBlock block;
        plugin->getStateInformation (block);
        return block;
    }

private:
    std::unique_ptr<juce::AudioPluginInstance> plugin;
    std::function<void()> closed;
};

//==============================================================================
ClipEffectsPanel::ClipEffectsPanel (AudioEngine& e, juce::uint32 id) : engine (e), clipId (id)
{
    title.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    addAndMakeVisible (title);
    addAndMakeVisible (pitchLabel);
    pitch.setRange (-SessionClip::Effects::maxSemitones, SessionClip::Effects::maxSemitones, 0.01);
    pitch.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 22);
    pitch.setDoubleClickReturnValue (true, 0.0);
    pitch.setTooltip ("The clip's pitch, keeping its length: up to an octave either way. Double-click for none.");
    pitch.onDragEnd = [this] { setSemitones (pitch.getValue()); };
    pitch.onValueChange = [this] { if (! pitch.isMouseButtonDown()) setSemitones (pitch.getValue()); };
    addAndMakeVisible (pitch);
    add.onClick = [this] { showAddMenu(); };
    addAndMakeVisible (add);
    Theme::follow (status, juce::Label::textColourId, Theme::dimText);
    addAndMakeVisible (status);

    engine.session.addChangeListener (this);
    refresh();
    setSize (460, 220);
}

ClipEffectsPanel::~ClipEffectsPanel()
{
    storeWindowState();
    window = nullptr;
    engine.session.removeChangeListener (this);
}

const SessionClip* ClipEffectsPanel::clipIn (const SessionState& s) const
{
    const auto where = s.findClip (clipId);
    return where.has_value() ? &s.tracks[(size_t) where->first].clips[(size_t) where->second] : nullptr;
}

const SessionClip* ClipEffectsPanel::clip() const
{
    return state != nullptr ? clipIn (*state) : nullptr;
}

bool ClipEffectsPanel::apply (const juce::String& description, double semitones, const juce::String& chain, bool merge)
{
    auto next = *engine.session.getState();

    if (! SessionEdits::setClipEffects (next, clipId, semitones, chain))
        return false;

    engine.session.apply (std::move (next), description, merge);
    return true;
}

bool ClipEffectsPanel::setSemitones (double semitones)
{
    const auto now = engine.session.getState();   // as it is, not as last shown
    const auto* c = clipIn (*now);
    return c != nullptr && apply ("Clip Pitch", semitones, c->effects.chain, true);
}

bool ClipEffectsPanel::addEffect (const juce::PluginDescription& plugin)
{
    const auto now = engine.session.getState();   // as it is, not as last shown
    const auto* c = clipIn (*now);
    return c != nullptr && apply ("Add Clip Effect", c->effects.semitones, ClipEffects::withSlot (c->effects.chain, plugin));
}

bool ClipEffectsPanel::removeEffect (int slot)
{
    const auto now = engine.session.getState();   // as it is, not as last shown
    const auto* c = clipIn (*now);

    if (c == nullptr || ! juce::isPositiveAndBelow (slot, (int) ClipEffects::slotsOf (c->effects.chain).size()))
        return false;

    if (slot == windowSlot)
    {
        window = nullptr;
        windowSlot = -1;
    }
    else if (slot < windowSlot)
    {
        --windowSlot;
    }

    return apply ("Remove Clip Effect", c->effects.semitones, ClipEffects::withoutSlot (c->effects.chain, slot));
}

bool ClipEffectsPanel::setBypassed (int slot, bool bypassed)
{
    const auto now = engine.session.getState();   // as it is, not as last shown
    const auto* c = clipIn (*now);
    return c != nullptr && apply (bypassed ? "Bypass Clip Effect" : "Clip Effect On", c->effects.semitones, ClipEffects::withBypass (c->effects.chain, slot, bypassed));
}

bool ClipEffectsPanel::moveEffect (int from, int to)
{
    const auto now = engine.session.getState();   // as it is, not as last shown
    const auto* c = clipIn (*now);

    if (c == nullptr || ! apply ("Move Clip Effect", c->effects.semitones, ClipEffects::withMove (c->effects.chain, from, to)))
        return false;

    if (windowSlot == from)
        windowSlot = to;
    else if (windowSlot >= juce::jmin (from, to) && windowSlot <= juce::jmax (from, to))
        windowSlot += from < to ? -1 : 1;

    return true;
}

void ClipEffectsPanel::editEffect (int slot)
{
    const auto now = engine.session.getState();   // as it is, not as last shown
    const auto* c = clipIn (*now);

    if (c == nullptr)
        return;

    const auto slots = ClipEffects::slotsOf (c->effects.chain);

    if (! juce::isPositiveAndBelow (slot, (int) slots.size()))
        return;

    if (slot == windowSlot && window != nullptr)
    {
        window->toFront (true);
        return;
    }

    storeWindowState();
    window = nullptr;
    const auto saved = ClipEffects::stateOf (c->effects.chain, slot);
    const auto name = slots[(size_t) slot].name;

    engine.createPlugin (slots[(size_t) slot].plugin, engine.session.getSampleRate(),
                         [safe = juce::Component::SafePointer<ClipEffectsPanel> (this), slot, saved, name]
                         (std::unique_ptr<juce::AudioPluginInstance> plugin, const juce::String& error)
    {
        if (safe == nullptr)
            return;

        if (plugin == nullptr)
        {
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Could not load " + name, error);
            return;
        }

        if (saved.getSize() > 0)
            plugin->setStateInformation (saved.getData(), (int) saved.getSize());

        safe->windowSlot = slot;
        safe->window = std::make_unique<EffectWindow> (std::move (plugin), [safe]
        {
            juce::MessageManager::callAsync ([safe]
            {
                if (safe != nullptr)
                {
                    safe->storeWindowState();
                    safe->window = nullptr;
                    safe->windowSlot = -1;
                }
            });
        });
        safe->windowState = safe->window->state();
        safe->startTimer (400);
    });
}

void ClipEffectsPanel::storeWindowState()
{
    const auto now = engine.session.getState();   // as it is, not as last shown
    const auto* c = clipIn (*now);

    if (window == nullptr || c == nullptr)
        return;

    // What changed in the window goes into the slot, merged into one step while it is worked on.
    const auto settings = window->state();

    if (settings == windowState)
        return;

    windowState = settings;
    apply ("Clip Effect Settings", c->effects.semitones, ClipEffects::withState (c->effects.chain, windowSlot, settings), true);
}

void ClipEffectsPanel::timerCallback()
{
    if (window == nullptr)
    {
        stopTimer();
        return;
    }

    storeWindowState();
}

void ClipEffectsPanel::showAddMenu()
{
    juce::Array<juce::PluginDescription> types;

    for (auto& type : engine.knownPlugins.getTypes())
        if (! type.isInstrument)
            types.add (type);

    juce::PopupMenu menu;
    PluginMenu::add (menu, types);

    if (types.isEmpty())
        menu.addItem (-1, "No effects found: scan for plug-ins in Plug-ins...", false);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (add),
                        [safe = juce::Component::SafePointer<ClipEffectsPanel> (this), types] (int result)
                        {
                            if (const auto index = PluginMenu::chosen (types, result); safe != nullptr && index >= 0)
                                safe->addEffect (types.getReference (index));
                        });
}

juce::StringArray ClipEffectsPanel::describeRows() const
{
    juce::StringArray out;

    if (const auto* c = clip())
        for (auto& slot : ClipEffects::slotsOf (c->effects.chain))
            out.add (slot.bypassed ? slot.name + " (off)" : slot.name);

    return out;
}

juce::Button* ClipEffectsPanel::rowButton (int slot, int which)
{
    if (! juce::isPositiveAndBelow (slot, (int) rows.size()))
        return nullptr;

    auto& r = *rows[(size_t) slot];
    juce::Button* buttons[] { &r.edit, &r.on, &r.up, &r.down, &r.remove };
    return juce::isPositiveAndBelow (which, 5) ? buttons[which] : nullptr;
}

void ClipEffectsPanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refresh();
}

void ClipEffectsPanel::refresh()
{
    state = engine.session.getState();
    const auto* c = clip();

    if (c == nullptr)
    {
        title.setText ("The clip is no longer in the session.", juce::dontSendNotification);
        status.setText ({}, juce::dontSendNotification);
        rows.clear();
        pitch.setEnabled (false);
        add.setEnabled (false);
        return;
    }

    title.setText ("Clip Effects: " + c->name, juce::dontSendNotification);

    if (! pitch.isMouseButtonDown())
        pitch.setValue (c->effects.semitones, juce::dontSendNotification);

    pitch.setEnabled (! c->locked);
    add.setEnabled (! c->locked);
    status.setText (! c->effects.isOn()            ? juce::String ("No effects: the clip plays as it is.")
                    : ClipEffects::isStale (*state, *c) ? juce::String ("Rendering... (the clip plays without its effects until it is done)")
                                                    : juce::String ("Rendered: the clip plays with its effects."),
                    juce::dontSendNotification);

    const auto slots = ClipEffects::slotsOf (c->effects.chain);

    while (rows.size() > slots.size())
        rows.pop_back();

    while (rows.size() < slots.size())
    {
        auto row = std::make_unique<Row>();
        const auto index = (int) rows.size();
        row->edit.onClick = [this, index] { editEffect (index); };
        row->on.onClick = [this, index] { setBypassed (index, ! rows[(size_t) index]->on.getToggleState()); };
        row->up.onClick = [this, index] { moveEffect (index, index - 1); };
        row->down.onClick = [this, index] { moveEffect (index, index + 1); };
        row->remove.onClick = [this, index] { removeEffect (index); };

        for (auto* comp : std::initializer_list<juce::Component*> { &row->name, &row->edit, &row->on, &row->up, &row->down, &row->remove })
            addAndMakeVisible (comp);

        rows.push_back (std::move (row));
    }

    for (size_t i = 0; i < slots.size(); ++i)
    {
        auto& r = *rows[i];
        r.name.setText (slots[i].name, juce::dontSendNotification);
        r.on.setToggleState (! slots[i].bypassed, juce::dontSendNotification);
        r.up.setEnabled (i > 0 && ! c->locked);
        r.down.setEnabled (i + 1 < slots.size() && ! c->locked);
        r.remove.setEnabled (! c->locked);
        r.on.setEnabled (! c->locked);
    }

    const auto wanted = 150 + juce::jmax (1, (int) rows.size()) * rowHeight;

    if (getHeight() != wanted && getHeight() > 0)
        setSize (getWidth(), wanted);
    else
        resized();
}

void ClipEffectsPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::surface);
}

void ClipEffectsPanel::resized()
{
    auto r = getLocalBounds().reduced (12);
    title.setBounds (r.removeFromTop (24));
    r.removeFromTop (6);
    auto line = r.removeFromTop (26);
    pitchLabel.setBounds (line.removeFromLeft (130));
    pitch.setBounds (line);
    r.removeFromTop (8);

    for (auto& row : rows)
    {
        auto rr = r.removeFromTop (rowHeight).reduced (0, 2);
        row->remove.setBounds (rr.removeFromRight (68));
        rr.removeFromRight (4);
        row->down.setBounds (rr.removeFromRight (52));
        rr.removeFromRight (4);
        row->up.setBounds (rr.removeFromRight (40));
        rr.removeFromRight (4);
        row->on.setBounds (rr.removeFromRight (48));
        rr.removeFromRight (4);
        row->edit.setBounds (rr.removeFromRight (48));
        row->name.setBounds (rr);
    }

    r.removeFromTop (6);
    add.setBounds (r.removeFromTop (26).removeFromLeft (120));
    r.removeFromTop (6);
    status.setBounds (r.removeFromTop (22));
}
