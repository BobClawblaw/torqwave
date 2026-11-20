#include "TrackInspector.h"
#include "Theme.h"

namespace
{
    const SessionTrack* findTrack (const SessionState& s, juce::uint32 id)
    {
        for (auto& t : s.tracks)
            if (t.id == id)
                return &t;

        return nullptr;
    }

    juce::String kindOf (const SessionTrack& t)
    {
        return t.isFolder ? "Folder" : t.isVca ? "VCA fader" : t.isMaster ? "Master" : t.isBus ? "Bus" : t.isInstrument ? "Instrument track" : "Audio track";
    }
}

TrackInspector::TrackInspector (AudioEngine& e, std::function<juce::uint32()> toShow)
    : engine (e), trackToShow (std::move (toShow))
{
    empty.setJustificationType (juce::Justification::centred);
    Theme::follow (empty, juce::Label::textColourId, Theme::dimText);
    Theme::follow (kind, juce::Label::textColourId, Theme::dimText);
    Theme::follow (noInserts, juce::Label::textColourId, Theme::dimText);
    Theme::follow (noSends, juce::Label::textColourId, Theme::dimText);
    addAndMakeVisible (empty);

    name.setTooltip ("The track's name; type a new one and press Return");
    name.onReturnKey = [this]
    {
        if (const auto typed = name.getText().trim(); typed.isNotEmpty())
            edit ("Rename Track", false, [typed] (SessionTrack& t) { t.name = typed; });

        refresh();
    };
    name.onEscapeKey = [this] { name.giveAwayKeyboardFocus(); refresh(); };
    name.onFocusLost = [this] { name.onReturnKey(); };

    volume.setSliderStyle (juce::Slider::LinearBar);
    volume.setRange (-60.0, 12.0, 0.1);
    volume.setSkewFactorFromMidPoint (-12.0);
    volume.setDoubleClickReturnValue (true, 0.0);
    volume.setTextValueSuffix (" dB");
    volume.onValueChange = [this] { edit ("Track Volume", true, [db = (float) volume.getValue()] (SessionTrack& t) { t.gainDb = db; }); };

    pan.setSliderStyle (juce::Slider::LinearBar);
    pan.setRange (-100.0, 100.0, 1.0);
    pan.setDoubleClickReturnValue (true, 0.0);
    pan.onValueChange = [this] { edit ("Track Pan", true, [p = (float) pan.getValue() / 100.0f] (SessionTrack& t) { t.pan = p; }); };

    for (auto* s : { &volume, &pan })
        Theme::follow (*s, juce::Slider::trackColourId, Theme::lineStrong);

    mute.onClick = [this] { edit ("Mute Track", false, [on = mute.getToggleState()] (SessionTrack& t) { t.mute = on; }); };
    solo.onClick = [this] { edit ("Solo Track", false, [on = solo.getToggleState()] (SessionTrack& t) { t.solo = on; }); };

    output.setTooltip ("Where the track plays: the master, or a bus");
    output.onChange = [this]
    {
        const auto index = output.getSelectedItemIndex();

        if (! juce::isPositiveAndBelow (index, (int) outputIds.size()))
            return;

        auto next = *engine.session.getState();

        if (SessionEdits::setTrackOutput (next, shownTrack, outputIds[(size_t) index]))
            engine.session.apply (std::move (next), "Track Output");

        refresh();
    };

    instrument.setTooltip ("Open the instrument's window");
    instrument.onClick = [this]
    {
        if (auto* plugin = engine.getTrackEffects().getInstrument (shownTrack); plugin != nullptr && showEditor)
            showEditor (*plugin);
    };

    for (auto* c : std::initializer_list<juce::Component*> { &kind, &name, &volumeLabel, &volume, &panLabel, &pan, &mute, &solo, &outputLabel, &output,
                                                             &instrumentLabel, &instrument, &insertsLabel, &noInserts, &sendsLabel, &noSends })
        addChildComponent (c);

    engine.session.addChangeListener (this);
    refresh();
    startTimerHz (6);
}

TrackInspector::~TrackInspector()
{
    engine.session.removeChangeListener (this);
}

void TrackInspector::edit (const juce::String& description, bool merge, const std::function<void (SessionTrack&)>& change)
{
    auto next = *engine.session.getState();

    for (auto& t : next.tracks)
    {
        if (t.id == shownTrack)
        {
            const auto before = t;
            change (t);

            // Only a real change: a control set to what the session holds makes no step.
            if (t.name == before.name && juce::exactlyEqual (t.gainDb, before.gainDb) && juce::exactlyEqual (t.pan, before.pan)
                && t.mute == before.mute && t.solo == before.solo)
                return;

            SessionEdits::followGroup (next, before);
            engine.session.apply (std::move (next), description, merge);
            return;
        }
    }
}

juce::String TrackInspector::layoutKey (const SessionState& s) const
{
    juce::String key;
    auto& effects = engine.getTrackEffects();

    for (int i = 0; i < effects.getNumSlots (shownTrack); ++i)
        if (auto* p = effects.getPlugin (shownTrack, i))
            key << p->getName() << (effects.isBypassed (shownTrack, i) ? "~" : "") << "|";

    key << "#";

    for (auto& t : s.tracks)
        if (t.isBus || t.id == shownTrack)
            key << (int) t.id << ":" << t.name << ";";

    if (const auto* t = findTrack (s, shownTrack))
        for (auto& send : t->sends)
            key << "s" << (int) send.busId << ";";

    return key;
}

void TrackInspector::refresh()
{
    const auto state = engine.session.getState();
    const auto wanted = trackToShow ? trackToShow() : 0;
    const auto* track = findTrack (*state, wanted);
    shownTrack = track != nullptr ? track->id : 0;

    for (auto* c : std::initializer_list<juce::Component*> { &kind, &name, &volumeLabel, &volume, &panLabel, &pan, &mute, &solo, &outputLabel, &output,
                                                             &insertsLabel, &sendsLabel })
        c->setVisible (track != nullptr);

    empty.setVisible (track == nullptr);

    if (track == nullptr)
    {
        instrumentLabel.setVisible (false);
        instrument.setVisible (false);
        noInserts.setVisible (false);
        noSends.setVisible (false);
        inserts.clear();
        sends.clear();
        shownLayout.clear();
        resized();
        return;
    }

    kind.setText (kindOf (*track), juce::dontSendNotification);

    if (! name.hasKeyboardFocus (true) && name.getText() != track->name)
        name.setText (track->name, false);

    if (! volume.isMouseButtonDown())
        volume.setValue (track->gainDb, juce::dontSendNotification);

    if (! pan.isMouseButtonDown())
        pan.setValue (std::round (track->pan * 100.0f), juce::dontSendNotification);

    mute.setToggleState (track->mute, juce::dontSendNotification);
    solo.setToggleState (track->solo, juce::dontSendNotification);

    // A folder has no mix of its own to show; a VCA fader only its volume.
    for (auto* c : std::initializer_list<juce::Component*> { &volumeLabel, &volume })
        c->setVisible (! track->isFolder);

    for (auto* c : std::initializer_list<juce::Component*> { &panLabel, &pan, &outputLabel, &output, &insertsLabel, &sendsLabel })
        c->setVisible (! track->isControl());

    auto& effects = engine.getTrackEffects();
    auto* plays = effects.getInstrument (track->id);
    const auto showInstrument = track->isInstrument && ! track->isFolder;
    instrumentLabel.setVisible (showInstrument);
    instrument.setVisible (showInstrument);
    instrument.setEnabled (plays != nullptr);
    instrument.setButtonText (plays != nullptr ? plays->getName() : juce::String ("None (choose one on the track)"));

    if (const auto key = layoutKey (*state); key != shownLayout)
    {
        shownLayout = key;

        // The output's choices: the master, then each bus but this one.
        output.clear (juce::dontSendNotification);
        outputIds.clear();
        output.addItem ("Master", 1);
        outputIds.push_back (0);

        for (auto& t : state->tracks)
        {
            if (t.takesBusInput() && t.id != track->id)
            {
                outputIds.push_back (t.id);
                output.addItem (t.name, (int) outputIds.size());
            }
        }

        // The inserts: every slot but the instrument, a click opening its window.
        inserts.clear();

        for (int i = 0; i < effects.getNumSlots (track->id); ++i)
        {
            auto* plugin = effects.getPlugin (track->id, i);

            if (plugin == nullptr || plugin == plays)
                continue;

            auto* b = inserts.add (new juce::TextButton (plugin->getName() + (effects.isBypassed (track->id, i) ? "  (bypassed)" : "")));
            b->setTooltip ("Open " + plugin->getName() + "'s window");
            b->onClick = [this, slot = effects.getSlotId (track->id, i)]
            {
                if (auto* p = engine.getTrackEffects().findSlot (shownTrack, slot); p != nullptr && showEditor)
                    showEditor (*p);
            };
            addAndMakeVisible (b);
        }

        // The sends: the bus, its level and whether it is taken before the volume.
        sends.clear();

        for (auto& send : track->sends)
        {
            auto row = std::make_unique<Send>();
            row->busId = send.busId;
            const auto* bus = findTrack (*state, send.busId);
            row->name.setText (bus != nullptr ? bus->name : juce::String ("Bus"), juce::dontSendNotification);
            row->level.setSliderStyle (juce::Slider::LinearBar);
            row->level.setRange (-60.0, 12.0, 0.1);
            row->level.setSkewFactorFromMidPoint (-12.0);
            row->level.setDoubleClickReturnValue (true, 0.0);
            row->level.setTextValueSuffix (" dB");
            Theme::follow (row->level, juce::Slider::trackColourId, Theme::lineStrong);
            row->pre.setTooltip ("Take the send before the track's volume (pre-fader)");
            auto* r = row.get();
            const auto change = [this, r] (bool merge)
            {
                auto next = *engine.session.getState();
                const auto* from = findTrack (next, shownTrack);
                const auto level = (float) r->level.getValue();
                const auto pre = r->pre.getToggleState();

                // Only a real change: a control set to what the session holds makes no step.
                if (from == nullptr || std::none_of (from->sends.begin(), from->sends.end(), [&] (const SessionSend& s)
                                                     { return s.busId == r->busId && (! juce::exactlyEqual (s.gainDb, level) || s.preFader != pre); }))
                    return;

                if (SessionEdits::setSend (next, shownTrack, { r->busId, level, pre }))
                    engine.session.apply (std::move (next), "Send Level", merge);
            };
            row->level.onValueChange = [change] { change (true); };
            row->pre.onClick = [change] { change (false); };

            for (auto* c : std::initializer_list<juce::Component*> { &row->name, &row->level, &row->pre })
                addAndMakeVisible (c);

            sends.push_back (std::move (row));
        }
    }

    for (size_t i = 0; i < outputIds.size(); ++i)
        if (outputIds[i] == track->outputBusId)
            output.setSelectedItemIndex ((int) i, juce::dontSendNotification);

    for (auto& row : sends)
    {
        for (auto& send : track->sends)
        {
            if (send.busId == row->busId)
            {
                if (! row->level.isMouseButtonDown())
                    row->level.setValue (send.gainDb, juce::dontSendNotification);

                row->pre.setToggleState (send.preFader, juce::dontSendNotification);
            }
        }
    }

    noInserts.setVisible (! track->isControl() && inserts.isEmpty());
    noSends.setVisible (! track->isControl() && sends.empty());
    resized();
}

void TrackInspector::paint (juce::Graphics& g)
{
    g.fillAll (Theme::surface);
}

void TrackInspector::resized()
{
    auto r = getLocalBounds().reduced (8);
    empty.setBounds (r);

    if (shownTrack == 0)
        return;

    constexpr int row = 24, gap = 4, labelWidth = 62;
    const auto take = [&r] (int height) { auto line = r.removeFromTop (height); r.removeFromTop (gap); return line; };
    const auto labelled = [&] (juce::Label& label, juce::Component& control)
    {
        if (! control.isVisible())
            return;

        auto line = take (row);
        label.setBounds (line.removeFromLeft (labelWidth));
        control.setBounds (line);
    };
    const auto heading = [&] (juce::Label& label)
    {
        if (! label.isVisible())
            return;

        r.removeFromTop (gap);
        label.setBounds (take (18));
    };

    name.setBounds (take (row));
    kind.setBounds (take (16));

    labelled (volumeLabel, volume);
    labelled (panLabel, pan);
    auto switches = take (row);
    mute.setBounds (switches.removeFromLeft (switches.getWidth() / 2));
    solo.setBounds (switches);
    labelled (outputLabel, output);

    if (instrument.isVisible())
    {
        heading (instrumentLabel);
        instrument.setBounds (take (row));
    }

    heading (insertsLabel);

    for (auto* b : inserts)
        b->setBounds (take (row));

    if (noInserts.isVisible())
        noInserts.setBounds (take (row));

    heading (sendsLabel);

    for (auto& s : sends)
    {
        auto line = take (row);
        s->name.setBounds (line.removeFromLeft (labelWidth));
        s->pre.setBounds (line.removeFromRight (48));
        s->level.setBounds (line.withTrimmedRight (4));
    }

    if (noSends.isVisible())
        noSends.setBounds (take (row));
}
