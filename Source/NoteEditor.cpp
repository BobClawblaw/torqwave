#include "NoteEditor.h"
#include "Instrument.h"
#include "Theme.h"

namespace
{
    constexpr int divisions[] = { 1, 2, 4, 8, 16, 32 };
    constexpr int swings[] = { 50, 54, 58, 62, 66, 70, 75 };   // percent

    bool isBlackKey (int pitch)
    {
        const auto n = pitch % 12;
        return n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
    }
}

NoteEditor::NoteEditor()
{
    setWantsKeyboardFocus (true);

    addAndMakeVisible (gridLabel);
    gridLabel.setText ("Grid", juce::dontSendNotification);
    Theme::follow (gridLabel, juce::Label::textColourId, Theme::dimText);
    addAndMakeVisible (gridBox);

    for (int i = 0; i < (int) std::size (divisions); ++i)
        gridBox.addItem (divisions[i] == 1 ? juce::String ("Beat") : "1/" + juce::String (divisions[i]) + " beat", i + 1);

    gridBox.addItem ("Off", (int) std::size (divisions) + 1);
    gridBox.setSelectedId (3, juce::dontSendNotification);
    gridBox.onChange = [this]
    {
        const auto id = gridBox.getSelectedId();
        gridDivision = juce::isPositiveAndBelow (id - 1, (int) std::size (divisions)) ? divisions[id - 1] : 0;
        repaint();
    };

    addAndMakeVisible (swingBox);
    swingBox.setTooltip ("Swing: each pair's second grid line moved later (on a grid of 1/2 or 1/4 beat)");

    for (auto percent : swings)
        swingBox.addItem (percent == 50 ? juce::String ("Straight") : "Swing " + juce::String (percent) + " %", percent);

    swingBox.setSelectedId (50, juce::dontSendNotification);
    swingBox.onChange = [this]
    {
        swing = juce::jlimit (0.5, NoteQuantize::maxSwing, swingBox.getSelectedId() / 100.0);
        repaint();
    };

    addAndMakeVisible (channelBox);
    channelBox.setTooltip ("The MIDI channel the clip's notes and lanes go out on");

    for (int c = 1; c <= 16; ++c)
        channelBox.addItem ("Ch " + juce::String (c), c);

    channelBox.setSelectedId (1, juce::dontSendNotification);
    channelBox.onChange = [this] { setChannel (channelBox.getSelectedId()); };

    addAndMakeVisible (drumsButton);
    drumsButton.setClickingTogglesState (false);
    drumsButton.setTooltip ("Show the track's note clips as a drum grid: a row for each sound, a column for each step");
    drumsButton.onClick = [this]
    {
        if (shown.has_value() && showDrums != nullptr)
        {
            showDrums (! shown->drums);
            refresh();
            resized();
        }
    };

    addAndMakeVisible (laneLabel);
    laneLabel.setText ("Lane", juce::dontSendNotification);
    Theme::follow (laneLabel, juce::Label::textColourId, Theme::dimText);
    addAndMakeVisible (laneBox);
    laneBox.addItem ("None", 1);

    for (auto c : { 1, ControllerLane::pitchBend, 2, 7, 10, 11, 64, 71, 74 })
        laneBox.addItem (ControllerLane::nameOf (c), c + 2);

    laneBox.addSeparator();

    for (int c = 0; c < 128; ++c)
        if (c != 1 && c != 2 && c != 7 && c != 10 && c != 11 && c != 64 && c != 71 && c != 74)
            laneBox.addItem (ControllerLane::nameOf (c), c + 2);

    laneBox.setSelectedId (1, juce::dontSendNotification);
    laneBox.onChange = [this]
    {
        laneController = laneBox.getSelectedId() >= 2 ? laneBox.getSelectedId() - 2 : -1;
        resized();
        repaint();
    };

    addAndMakeVisible (velocityLabel);
    velocityLabel.setText ("Velocity", juce::dontSendNotification);
    Theme::follow (velocityLabel, juce::Label::textColourId, Theme::dimText);
    addAndMakeVisible (velocitySlider);
    velocitySlider.setSliderStyle (juce::Slider::LinearHorizontal);
    velocitySlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 40, 20);
    velocitySlider.setRange (1.0, 127.0, 1.0);
    velocitySlider.setValue (100.0, juce::dontSendNotification);
    velocitySlider.onValueChange = [this]
    {
        if (! shown.has_value() || selected.empty())
            return;

        for (auto i : selected)
            if (juce::isPositiveAndBelow (i, (int) shown->clip.notes.size()))
                shown->clip.notes[(size_t) i].velocity = getVelocity();

        repaint();
    };
    velocitySlider.onDragEnd = [this]
    {
        if (shown.has_value() && ! selected.empty())
            commit ("Note Velocity");
    };

    for (auto* b : { &zoomOutButton, &zoomInButton, &fitButton })
        addAndMakeVisible (*b);

    zoomOutButton.setTooltip ("Zoom out (Cmd-scroll or pinch zooms around the mouse)");
    zoomInButton.setTooltip ("Zoom in (Cmd-scroll or pinch zooms around the mouse)");
    fitButton.setTooltip ("Show the whole timeline");
    zoomOutButton.onClick = [this] { zoomAround ((float) rollArea().getCentreX(), 0.5); };
    zoomInButton.onClick = [this] { zoomAround ((float) rollArea().getCentreX(), 2.0); };
    fitButton.onClick = [this] { fitTime(); };

    addAndMakeVisible (timeScroll);
    timeScroll.setAutoHide (false);
    timeScroll.addListener (this);

    addAndMakeVisible (status);
    status.setFont (juce::Font (juce::FontOptions (12.0f)));
    Theme::follow (status, juce::Label::textColourId, Theme::dimText);

    refreshControls();
    startTimer (100);
}

NoteEditor::~NoteEditor()
{
    stopTimer();
    silence();
}

void NoteEditor::sound (int pitch, float velocity)
{
    if (pitch == heard)
        return;

    silence();

    if (audition != nullptr)
    {
        audition (pitch, juce::jmax (0.01f, velocity));
        heard = pitch;
    }
}

void NoteEditor::silence()
{
    if (heard >= 0 && audition != nullptr)
        audition (heard, 0.0f);

    heard = -1;
}

void NoteEditor::timerCallback()
{
    if (drag == Drag::none)
        refresh();
}

void NoteEditor::refresh()
{
    auto next = readClip != nullptr ? readClip() : std::nullopt;

    if (next == shown)
        return;

    const auto other = ! next.has_value() || ! shown.has_value() || next->clip.id != shown->clip.id;
    shown = std::move (next);

    if (shown.has_value())
        for (auto it = selected.begin(); it != selected.end();)
            it = *it < (int) shown->clip.notes.size() ? std::next (it) : selected.erase (it);
    else
        selected.clear();

    if (other)
    {
        selected.clear();
        firstRow = 0;
        scrollToNotes();
        showPattern();
    }
    else
    {
        setView (viewStart, zoom);   // the timeline's length may have changed
    }

    refreshControls();
    repaint();
}

void NoteEditor::refreshControls()
{
    const auto on = shown.has_value();
    gridBox.setEnabled (on);
    velocitySlider.setEnabled (on);
    swingBox.setEnabled (on);
    channelBox.setEnabled (on);
    drumsButton.setEnabled (on);
    drumsButton.setToggleState (showingDrums(), juce::dontSendNotification);

    if (on)
        channelBox.setSelectedId (juce::jlimit (1, 16, shown->clip.channel), juce::dontSendNotification);

    if (! on)
    {
        status.setText ("Select a note clip in the multitrack (Add Notes... in a track's menu makes one).", juce::dontSendNotification);
        return;
    }

    const auto& c = shown->clip;
    juce::String text = shown->trackName + ": " + c.name + ", " + juce::String ((int) c.notes.size()) + (c.notes.size() == 1 ? " note" : " notes");

    if (c.loopLength > 0 && c.loopLength < c.length)
        text << ", repeating every " << juce::String ((double) c.loopLength / shown->sampleRate, 2) << " s";

    if (! selected.empty())
        text << "; " << juce::String ((int) selected.size()) << " selected";

    if (! shown->hasInstrument)
        text << ". The track has no instrument: right-click its name and choose one under Instrument.";

    status.setText (text, juce::dontSendNotification);
}

void NoteEditor::setGridDivision (int division)
{
    const auto found = std::find (std::begin (divisions), std::end (divisions), division);
    gridBox.setSelectedId (found == std::end (divisions) ? (int) std::size (divisions) + 1 : (int) (found - std::begin (divisions)) + 1, juce::sendNotificationSync);
}

juce::int64 NoteEditor::gridStepFrames() const noexcept
{
    if (! shown.has_value() || gridDivision <= 0)
        return 0;

    return juce::jmax ((juce::int64) 1, (juce::int64) std::llround (shown->beatSeconds * shown->sampleRate / gridDivision));
}

void NoteEditor::setVelocity (float velocity)
{
    velocitySlider.setValue (juce::jlimit (1.0, 127.0, (double) std::round (velocity * 127.0f)), juce::sendNotificationSync);

    if (shown.has_value() && ! selected.empty())
        commit ("Note Velocity");
}

juce::int64 NoteEditor::snapped (juce::int64 frame) const
{
    const auto step = gridStepFrames();

    if (step <= 0)
        return frame;

    // The line at or before the frame: swing only moves lines later.
    auto k = frame / step;

    while (k > 0 && lineFrame (k) > frame)
        --k;

    return lineFrame (k);
}

void NoteEditor::setSwing (double s)
{
    const auto percent = (int) std::lround (juce::jlimit (0.5, NoteQuantize::maxSwing, s) * 100.0);
    auto best = swings[0];

    for (auto p : swings)
        if (std::abs (p - percent) < std::abs (best - percent))
            best = p;

    swingBox.setSelectedId (best, juce::sendNotificationSync);
}

juce::int64 NoteEditor::straightStep() const noexcept
{
    if (! shown.has_value())
        return 0;

    const auto division = gridDivision > 0 ? gridDivision : 4;
    return juce::jmax ((juce::int64) 1, (juce::int64) std::llround (shown->beatSeconds * shown->sampleRate / division));
}

juce::int64 NoteEditor::lineFrame (juce::int64 k) const
{
    if (! shown.has_value())
        return 0;

    NoteQuantize q;
    q.division = drumDivision();
    q.swing = swing;
    return (juce::int64) std::llround (q.lineBeat (k) * shown->beatSeconds * shown->sampleRate);
}

void NoteEditor::setChannel (int channel)
{
    channel = juce::jlimit (1, 16, channel);
    channelBox.setSelectedId (channel, juce::dontSendNotification);

    if (! shown.has_value() || shown->clip.channel == channel)
        return;

    shown->clip.channel = channel;
    commit ("MIDI Channel");
}

int NoteEditor::leftWidth() const noexcept
{
    return showingDrums() ? drumNamesWidth : keyboardWidth;
}

//==============================================================================
std::vector<DrumSound> NoteEditor::drumRows() const
{
    if (! shown.has_value())
        return {};

    auto rows = shown->drumMap;
    std::set<int> others;

    for (auto& n : shown->clip.notes)
        if (std::none_of (rows.begin(), rows.end(), [&] (const DrumSound& d) { return d.pitch == n.pitch; }))
            others.insert (n.pitch);

    for (auto pitch : others)
        rows.push_back ({ pitch, DrumSound::nameIn ({}, pitch) });

    return rows;
}

int NoteEditor::stepCount() const
{
    if (! shown.has_value())
        return 0;

    const auto whole = wholeLength();
    int k = 0;

    while (lineFrame (k) < whole)
        ++k;

    return k;
}

juce::Rectangle<float> NoteEditor::cellBounds (int row, int step) const
{
    const auto area = rollArea();
    const auto x0 = xOf (lineFrame (step)), x1 = xOf (juce::jmin (wholeLength(), lineFrame (step + 1)));
    return { x0, (float) area.getY() + (float) ((row - firstRow) * drumRowHeight), juce::jmax (2.0f, x1 - x0), (float) drumRowHeight };
}

std::pair<int, int> NoteEditor::cellAt (juce::Point<float> p) const
{
    if (! showingDrums() || ! rollArea().toFloat().contains (p))
        return { -1, -1 };

    const auto row = firstRow + (int) std::floor ((p.y - (float) rollArea().getY()) / (float) drumRowHeight);

    if (! juce::isPositiveAndBelow (row, (int) drumRows().size()))
        return { -1, -1 };

    const auto frame = frameAt (p.x);
    const auto steps = stepCount();

    for (int k = 0; k < steps; ++k)
        if (frame < lineFrame (k + 1))
            return { row, k };

    return { -1, -1 };
}

int NoteEditor::noteInCell (int row, int step) const
{
    const auto rows = drumRows();

    if (! shown.has_value() || ! juce::isPositiveAndBelow (row, (int) rows.size()))
        return -1;

    const auto from = lineFrame (step), to = lineFrame (step + 1);

    for (size_t i = 0; i < shown->clip.notes.size(); ++i)
    {
        const auto& n = shown->clip.notes[i];

        if (n.pitch == rows[(size_t) row].pitch && n.start >= from && n.start < to)
            return (int) i;
    }

    return -1;
}

void NoteEditor::setDrumMap (const std::vector<DrumSound>& map, const juce::String& description)
{
    if (! shown.has_value())
        return;

    shown->drumMap = map.empty() ? DrumSound::generalMidi() : map;

    if (applyDrumMap != nullptr)
        applyDrumMap (map == DrumSound::generalMidi() ? std::vector<DrumSound>() : map, description);

    repaint();
}

void NoteEditor::drumMapMenu (int row)
{
    const auto rows = drumRows();

    if (! shown.has_value() || ! juce::isPositiveAndBelow (row, (int) rows.size()))
        return;

    const auto inMap = row < (int) shown->drumMap.size();
    juce::PopupMenu menu;
    menu.addSectionHeader (rows[(size_t) row].name + " (" + DrumSound::nameIn ({}, rows[(size_t) row].pitch) + ")");
    menu.addItem (1, inMap ? "Edit Sound..." : "Add to the Map...");
    menu.addItem (2, "Add Sound...");
    menu.addItem (3, "Remove from the Map", inMap);
    menu.addItem (4, "Move Up", inMap && row > 0);
    menu.addItem (5, "Move Down", inMap && row + 1 < (int) shown->drumMap.size());
    menu.addSeparator();
    menu.addItem (6, "General MIDI Map", ! (shown->drumMap == DrumSound::generalMidi()));

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(),
                        [safe = juce::Component::SafePointer<NoteEditor> (this), row, inMap] (int choice)
    {
        if (safe == nullptr || ! safe->shown.has_value())
            return;

        auto map = safe->shown->drumMap;

        switch (choice)
        {
            case 1: safe->editDrumSound (row, ! inMap); break;
            case 2: safe->editDrumSound (row, true); break;
            case 3:
                if (juce::isPositiveAndBelow (row, (int) map.size()))
                {
                    map.erase (map.begin() + row);
                    safe->setDrumMap (map, "Remove Drum Sound");
                }
                break;
            case 4:
            case 5:
                if (const auto other = choice == 4 ? row - 1 : row + 1; juce::isPositiveAndBelow (row, (int) map.size()) && juce::isPositiveAndBelow (other, (int) map.size()))
                {
                    std::swap (map[(size_t) row], map[(size_t) other]);
                    safe->setDrumMap (map, "Move Drum Sound");
                }
                break;
            case 6: safe->setDrumMap ({}, "General MIDI Drum Map"); break;
            default: break;
        }
    });
}

void NoteEditor::editDrumSound (int row, bool adding)
{
    const auto rows = drumRows();

    if (! shown.has_value() || ! juce::isPositiveAndBelow (row, (int) rows.size()))
        return;

    const auto& was = rows[(size_t) row];
    const auto extra = row >= (int) shown->drumMap.size();   // a note outside the map: added as it is
    auto* window = new juce::AlertWindow (adding ? "Add a Drum Sound" : "Edit the Drum Sound", "Its name, and the note (0 to 127) it plays.", juce::MessageBoxIconType::NoIcon);
    window->addTextEditor ("name", adding && ! extra ? juce::String() : was.name, "Name");
    window->addTextEditor ("note", juce::String (adding && ! extra ? juce::jmin (127, was.pitch + 1) : was.pitch), "Note");
    window->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    window->enterModalState (true, juce::ModalCallbackFunction::create (
        [safe = juce::Component::SafePointer<NoteEditor> (this), window, row, adding, extra] (int result)
    {
        if (result != 1 || safe == nullptr || ! safe->shown.has_value())
            return;

        const auto name = window->getTextEditorContents ("name").trim();
        const auto pitch = window->getTextEditorContents ("note").trim().getIntValue();

        if (! juce::isPositiveAndBelow (pitch, 128))
            return;

        auto map = safe->shown->drumMap;
        const DrumSound sound { pitch, name.isNotEmpty() ? name : DrumSound::nameIn ({}, pitch) };

        if (adding)
            map.insert (map.begin() + (extra ? (int) map.size() : juce::jmin (row + 1, (int) map.size())), sound);
        else if (juce::isPositiveAndBelow (row, (int) map.size()))
            map[(size_t) row] = sound;

        safe->setDrumMap (map, adding ? "Add Drum Sound" : "Edit Drum Sound");
    }), true);
}

//==============================================================================
juce::Rectangle<int> NoteEditor::rollArea() const
{
    return getLocalBounds().withTrimmedTop (topBar).withTrimmedLeft (leftWidth()).withTrimmedBottom ((laneController >= 0 ? laneHeight : 0) + scrollHeight);
}

juce::int64 NoteEditor::visibleLength() const noexcept
{
    return juce::jmax ((juce::int64) 1, (juce::int64) std::llround ((double) wholeLength() / zoom));
}

double NoteEditor::maxZoom() const noexcept
{
    // Down to a twentieth of a second across the roll.
    const auto shortest = shown.has_value() ? shown->sampleRate * 0.05 : 2400.0;
    return juce::jmax (1.0, (double) wholeLength() / shortest);
}

void NoteEditor::setView (juce::int64 start, double z)
{
    zoom = juce::jlimit (1.0, maxZoom(), z);
    viewStart = juce::jlimit ((juce::int64) 0, juce::jmax ((juce::int64) 0, wholeLength() - visibleLength()), start);
    updateScroll();
    repaint();
}

void NoteEditor::zoomAround (float x, double factor)
{
    const auto area = rollArea();
    const auto along = (double) juce::jlimit (0.0f, 1.0f, (x - (float) area.getX()) / (float) juce::jmax (1, area.getWidth() - 1));
    const auto frame = (double) viewStart + along * (double) visibleLength();
    const auto z = juce::jlimit (1.0, maxZoom(), zoom * factor);
    const auto visible = (double) wholeLength() / z;
    setView ((juce::int64) std::llround (frame - along * visible), z);
}

void NoteEditor::showPattern()
{
    if (! shown.has_value())
        return;

    const auto period = shown->clip.getPeriod();

    if (period > 0 && period < wholeLength())
        setView (0, (double) wholeLength() / (double) period);
    else
        fitTime();
}

void NoteEditor::setRowHeight (int pixels)
{
    const auto area = rollArea();
    const auto middle = pitchAt ((float) area.getCentreY());
    rowPixels = juce::jlimit (minRowHeight, maxRowHeight, pixels);
    const auto rows = juce::jmax (1, rollArea().getHeight() / rowPixels);
    lowPitch = juce::jlimit (0, juce::jmax (0, 128 - rows), middle - rows / 2);
    repaint();
}

void NoteEditor::updateScroll()
{
    timeScroll.setRangeLimits (0.0, (double) wholeLength(), juce::dontSendNotification);
    timeScroll.setCurrentRange ((double) viewStart, (double) visibleLength(), juce::dontSendNotification);
}

void NoteEditor::scrollBarMoved (juce::ScrollBar*, double newRangeStart)
{
    setView ((juce::int64) std::llround (newRangeStart), zoom);
}

juce::Rectangle<int> NoteEditor::laneArea() const
{
    if (laneController < 0)
        return {};

    return getLocalBounds().withTrimmedLeft (leftWidth()).removeFromBottom (laneHeight);
}

void NoteEditor::setLane (int controller)
{
    laneBox.setSelectedId (controller >= 0 && controller <= ControllerLane::pitchBend ? controller + 2 : 1, juce::sendNotificationSync);
}

float NoteEditor::yForLaneValue (float value) const
{
    const auto area = laneArea().toFloat().reduced (0.0f, 6.0f);
    return area.getBottom() - juce::jlimit (0.0f, 1.0f, value) * area.getHeight();
}

float NoteEditor::laneValueAt (float y) const
{
    const auto area = laneArea().toFloat().reduced (0.0f, 6.0f);
    return area.getHeight() > 0.0f ? juce::jlimit (0.0f, 1.0f, (area.getBottom() - y) / area.getHeight()) : 0.0f;
}

int NoteEditor::lanePointAt (juce::Point<float> p) const
{
    if (! shown.has_value() || laneController < 0 || ! laneArea().toFloat().contains (p))
        return -1;

    const auto* lane = shown->clip.findLane (laneController);

    if (lane == nullptr)
        return -1;

    for (auto i = (int) lane->envelope.points.size() - 1; i >= 0; --i)
    {
        const auto& point = lane->envelope.points[(size_t) i];

        if (juce::Point<float> (xOf (point.frame), yForLaneValue (point.value)).getDistanceFrom (p) <= 6.0f)
            return i;
    }

    return -1;
}

float NoteEditor::yOf (int pitch) const
{
    return (float) rollArea().getBottom() - (float) (pitch - lowPitch + 1) * (float) rowPixels;
}

int NoteEditor::pitchAt (float y) const
{
    return juce::jlimit (0, 127, lowPitch + (int) std::floor (((float) rollArea().getBottom() - y) / (float) rowPixels));
}

juce::int64 NoteEditor::wholeLength() const noexcept
{
    return shown.has_value() ? juce::jmax ((juce::int64) 1, shown->clip.offset + shown->clip.length) : (juce::int64) 1;
}

float NoteEditor::xOf (juce::int64 frame) const
{
    const auto area = rollArea();
    return (float) area.getX() + (float) ((double) (frame - viewStart) * (double) (area.getWidth() - 1) / (double) visibleLength());
}

juce::int64 NoteEditor::frameAt (float x) const
{
    const auto area = rollArea();
    const auto length = wholeLength();
    return juce::jlimit ((juce::int64) 0, length, viewStart + (juce::int64) std::llround ((x - (float) area.getX()) * (double) visibleLength() / (double) juce::jmax (1, area.getWidth() - 1)));
}

juce::Rectangle<float> NoteEditor::noteBounds (const ClipNote& n) const
{
    const auto x0 = xOf (n.start), x1 = xOf (n.start + n.length);
    return { x0, yOf (n.pitch) + 1.0f, juce::jmax (3.0f, x1 - x0), (float) rowPixels - 2.0f };
}

int NoteEditor::noteAt (juce::Point<float> p) const
{
    if (! shown.has_value() || ! rollArea().toFloat().contains (p))
        return -1;

    for (auto i = (int) shown->clip.notes.size() - 1; i >= 0; --i)
        if (noteBounds (shown->clip.notes[(size_t) i]).contains (p))
            return i;

    return -1;
}

void NoteEditor::scrollToNotes()
{
    if (! shown.has_value())
        return;

    const auto rows = juce::jmax (1, rollArea().getHeight() / rowPixels);
    int low = 128, high = -1;

    for (auto& n : shown->clip.notes)
    {
        low = juce::jmin (low, n.pitch);
        high = juce::jmax (high, n.pitch);
    }

    if (high < 0)
    {
        low = 60;
        high = 60;
    }

    lowPitch = juce::jlimit (0, juce::jmax (0, 128 - rows), (low + high) / 2 - rows / 2);
}

//==============================================================================
void NoteEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::base);
    const auto area = rollArea();
    const auto keys = juce::Rectangle<int> (0, area.getY(), keyboardWidth, area.getHeight());
    g.setColour (Theme::canvas);
    g.fillRect (area);

    if (! shown.has_value())
    {
        g.setColour (Theme::surface);
        g.fillRect (keys);
        return;
    }

    if (showingDrums())
        paintDrums (g, area);
    else
        paintRoll (g, area);

    g.setColour (Theme::line);
    g.drawRect (area);

    const auto& c = shown->clip;

    // The lane: its line over the timeline (the rest value, dashed, without points), its points.
    if (laneController >= 0)
    {
        const auto strip = laneArea();
        g.setColour (Theme::inset);
        g.fillRect (strip);
        g.setColour (Theme::line);
        g.drawRect (strip);
        g.setFont (11.0f);
        g.setColour (Theme::dimText);
        g.drawText (ControllerLane::nameOf (laneController), strip.reduced (6, 2), juce::Justification::topLeft);

        const auto* lane = c.findLane (laneController);
        const auto rest = laneController == ControllerLane::pitchBend ? 0.5f : 0.0f;

        if (lane == nullptr || lane->envelope.isEmpty())
        {
            const float dashes[] = { 4.0f, 4.0f };
            g.setColour (Theme::dimText);
            g.drawDashedLine ({ (float) strip.getX(), yForLaneValue (rest), (float) strip.getRight(), yForLaneValue (rest) }, dashes, 2, 1.0f);
        }
        else
        {
            juce::Path line;
            const auto left = (float) strip.getX(), right = (float) strip.getRight();

            for (auto x = left; ; x = juce::jmin (right, x + 2.0f))
            {
                const auto y = yForLaneValue (lane->envelope.valueAt (frameAt (x)));

                if (line.isEmpty())
                    line.startNewSubPath (x, y);
                else
                    line.lineTo (x, y);

                if (x >= right)
                    break;
            }

            g.setColour (Theme::accentLight);
            g.strokePath (line, juce::PathStrokeType (1.5f));
            const juce::Graphics::ScopedSaveState onlyShown (g);
            g.reduceClipRegion (strip);

            for (auto& point : lane->envelope.points)
                g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ xOf (point.frame), yForLaneValue (point.value) }));
        }
    }
}

void NoteEditor::paintRoll (juce::Graphics& g, juce::Rectangle<int> area)
{
    const auto keys = juce::Rectangle<int> (0, area.getY(), keyboardWidth, area.getHeight());
    const auto& c = shown->clip;
    const auto rows = area.getHeight() / rowPixels + 1;

    // Rows: the black keys' rows darker, a line at each C.
    for (int row = 0; row < rows; ++row)
    {
        const auto pitch = lowPitch + row;

        if (pitch > 127)
            break;

        const auto y = yOf (pitch);
        const juce::Rectangle<float> r ((float) area.getX(), y, (float) area.getWidth(), (float) rowPixels);
        g.setColour (isBlackKey (pitch) ? Theme::laneAlternate : Theme::lane);
        g.fillRect (r.getIntersection (area.toFloat()));

        if (pitch % 12 == 0)
        {
            g.setColour (Theme::lineStrong);
            g.drawHorizontalLine ((int) (y + (float) rowPixels), (float) area.getX(), (float) area.getRight());
        }

        // The keyboard: white keys light, black keys dark, each C named.
        const juce::Rectangle<float> key (0.0f, y, (float) keyboardWidth - 1.0f, (float) rowPixels);
        g.setColour (isBlackKey (pitch) ? Theme::inset : Theme::raised);
        g.fillRect (key.getIntersection (keys.toFloat()));

        if (pitch % 12 == 0 && key.getY() >= (float) area.getY())
        {
            g.setColour (Theme::text);
            g.setFont (10.0f);
            g.drawText (Instrument::pitchName (pitch), key.toNearestInt().withTrimmedLeft (3), juce::Justification::centredLeft);
        }
    }

    g.setColour (Theme::line);
    g.drawVerticalLine (keyboardWidth - 1, (float) area.getY(), (float) area.getBottom());

    // Past here, only what is in the time shown.
    const juce::Graphics::ScopedSaveState onlyShown (g);
    g.reduceClipRegion (area);

    // The grid: a line each step, stronger each beat; the repeats of the pattern.
    const auto step = gridStepFrames();
    const auto whole = wholeLength();
    const auto period = juce::jmax ((juce::int64) 1, c.getPeriod());

    if (step > 0 && xOf (step) - xOf (0) >= 3.0f)
        for (juce::int64 k = 0, f = 0; f <= whole; f = lineFrame (++k))
        {
            g.setColour (k % gridDivision == 0 ? Theme::lineStrong : Theme::line);
            g.drawVerticalLine ((int) xOf (f), (float) area.getY(), (float) area.getBottom());
        }

    if (period < whole)
        for (auto f = period; f < whole; f += period)
        {
            g.setColour (Theme::accent.withAlpha (f == period ? 0.9f : 0.6f));
            g.drawVerticalLine ((int) xOf (f), (float) area.getY(), (float) area.getBottom());
        }

    // Before the clip's start, and past the pattern, the notes don't play: washed out.
    g.setColour (Theme::overlay.withAlpha (0.25f));

    if (c.offset > 0)
        g.fillRect (juce::Rectangle<float> (xOf (0), (float) area.getY(), xOf (c.offset) - xOf (0), (float) area.getHeight()));

    if (period < whole)
        g.fillRect (juce::Rectangle<float> (xOf (period), (float) area.getY(), xOf (whole) - xOf (period), (float) area.getHeight()));

    if (c.offset > 0)
    {
        g.setColour (Theme::cursor);
        g.drawVerticalLine ((int) xOf (c.offset), (float) area.getY(), (float) area.getBottom());
    }

    // The notes: velocity as their depth of colour, the selected outlined.
    for (size_t i = 0; i < c.notes.size(); ++i)
    {
        const auto& n = c.notes[i];
        const auto r = noteBounds (n);

        if (r.getBottom() < (float) area.getY() || r.getY() > (float) area.getBottom())
            continue;

        const auto playing = n.start < period && n.start >= c.offset && shown->hasInstrument;
        g.setColour ((playing ? Theme::accent : Theme::dimText).withAlpha (0.45f + 0.55f * juce::jlimit (0.0f, 1.0f, n.velocity)));
        g.fillRoundedRectangle (r.getIntersection (area.toFloat()), 2.0f);

        if (selected.count ((int) i) > 0)
        {
            g.setColour (Theme::textStrong);
            g.drawRoundedRectangle (r.getIntersection (area.toFloat()), 2.0f, 1.5f);
        }
    }

    if (drag == Drag::box)
    {
        g.setColour (Theme::selection.withAlpha (0.2f));
        g.fillRect (box);
        g.setColour (Theme::selection);
        g.drawRect (box, 1.0f);
    }
}

void NoteEditor::paintDrums (juce::Graphics& g, juce::Rectangle<int> area)
{
    const auto& c = shown->clip;
    const auto rows = drumRows();
    const auto steps = stepCount();
    const auto names = juce::Rectangle<int> (0, area.getY(), drumNamesWidth, area.getHeight());
    const auto period = juce::jmax ((juce::int64) 1, c.getPeriod());
    const auto whole = wholeLength();
    g.setColour (Theme::surface);
    g.fillRect (names);

    // Rows: alternate shades, each sound named down the left (notes outside the map dimmed).
    for (int row = firstRow; row < (int) rows.size(); ++row)
    {
        const auto r = juce::Rectangle<float> ((float) area.getX(), (float) area.getY() + (float) ((row - firstRow) * drumRowHeight),
                                               (float) area.getWidth(), (float) drumRowHeight);

        if (r.getY() >= (float) area.getBottom())
            break;

        g.setColour (row % 2 == 0 ? Theme::lane : Theme::laneAlternate);
        g.fillRect (r.getIntersection (area.toFloat()));
        g.setColour (Theme::line);
        g.drawHorizontalLine ((int) r.getBottom(), 0.0f, (float) area.getRight());

        const auto name = juce::Rectangle<int> (4, (int) r.getY(), drumNamesWidth - 8, drumRowHeight).getIntersection (names);
        g.setFont (11.0f);
        g.setColour (row < (int) shown->drumMap.size() ? Theme::text : Theme::dimText);
        g.drawText (rows[(size_t) row].name, name, juce::Justification::centredLeft, true);
    }

    g.setColour (Theme::line);
    g.drawVerticalLine (drumNamesWidth - 1, (float) area.getY(), (float) area.getBottom());

    // Past here, only what is in the time shown.
    const juce::Graphics::ScopedSaveState onlyShown (g);
    g.reduceClipRegion (area);

    // The steps: a line at each, stronger at each beat; the pattern's repeats.
    for (int k = 0; k <= steps; ++k)
    {
        g.setColour (k % drumDivision() == 0 ? Theme::lineStrong : Theme::line);
        g.drawVerticalLine ((int) xOf (juce::jmin (whole, lineFrame (k))), (float) area.getY(), (float) area.getBottom());
    }

    if (period < whole)
        for (auto f = period; f < whole; f += period)
        {
            g.setColour (Theme::accent.withAlpha (f == period ? 0.9f : 0.6f));
            g.drawVerticalLine ((int) xOf (f), (float) area.getY(), (float) area.getBottom());
        }

    // The set steps: velocity as depth of colour and as a bar's height, the selected outlined.
    for (size_t i = 0; i < c.notes.size(); ++i)
    {
        const auto& n = c.notes[i];
        const auto row = (int) (std::find_if (rows.begin(), rows.end(), [&] (const DrumSound& d) { return d.pitch == n.pitch; }) - rows.begin());
        int step = 0;

        while (step + 1 < steps && lineFrame (step + 1) <= n.start)
            ++step;

        const auto cell = cellBounds (row, step).reduced (1.5f);

        if (row < firstRow || cell.getY() >= (float) area.getBottom())
            continue;

        const auto playing = n.start < period && n.start >= c.offset && shown->hasInstrument;
        const auto v = juce::jlimit (0.0f, 1.0f, n.velocity);
        g.setColour ((playing ? Theme::accent : Theme::dimText).withAlpha (0.25f + 0.35f * v));
        g.fillRoundedRectangle (cell, 2.0f);
        g.setColour (playing ? Theme::accent : Theme::dimText);
        g.fillRect (cell.withTrimmedTop (cell.getHeight() * (1.0f - v)).reduced (2.0f, 0.0f).withTrimmedBottom (1.0f));

        if (selected.count ((int) i) > 0)
        {
            g.setColour (Theme::textStrong);
            g.drawRoundedRectangle (cell, 2.0f, 1.5f);
        }
    }

    // Before the clip's start, and past the pattern, nothing plays: washed out.
    g.setColour (Theme::overlay.withAlpha (0.25f));

    if (c.offset > 0)
        g.fillRect (juce::Rectangle<float> (xOf (0), (float) area.getY(), xOf (c.offset) - xOf (0), (float) area.getHeight()));

    if (period < whole)
        g.fillRect (juce::Rectangle<float> (xOf (period), (float) area.getY(), xOf (whole) - xOf (period), (float) area.getHeight()));
}

void NoteEditor::resized()
{
    auto bar = getLocalBounds().removeFromTop (topBar).reduced (6, 4);
    drumsButton.setBounds (bar.removeFromLeft (56));
    bar.removeFromLeft (8);
    gridLabel.setBounds (bar.removeFromLeft (34));
    gridBox.setBounds (bar.removeFromLeft (96));
    bar.removeFromLeft (4);
    swingBox.setBounds (bar.removeFromLeft (108));
    bar.removeFromLeft (12);
    velocityLabel.setBounds (bar.removeFromLeft (56));
    velocitySlider.setBounds (bar.removeFromLeft (juce::jmin (160, bar.getWidth())));
    bar.removeFromLeft (12);
    channelBox.setBounds (bar.removeFromLeft (juce::jmin (70, bar.getWidth())));
    bar.removeFromLeft (12);
    laneLabel.setBounds (bar.removeFromLeft (34));
    laneBox.setBounds (bar.removeFromLeft (juce::jmin (150, juce::jmax (60, bar.getWidth() / 3))));
    bar.removeFromLeft (12);
    fitButton.setBounds (bar.removeFromRight (juce::jmin (40, bar.getWidth())));
    bar.removeFromRight (2);
    zoomInButton.setBounds (bar.removeFromRight (juce::jmin (26, bar.getWidth())));
    bar.removeFromRight (2);
    zoomOutButton.setBounds (bar.removeFromRight (juce::jmin (26, bar.getWidth())));
    bar.removeFromRight (8);
    status.setBounds (bar);

    const auto roll = rollArea();
    timeScroll.setBounds (roll.getX(), roll.getBottom(), roll.getWidth(), scrollHeight);
    setView (viewStart, zoom);
}

//==============================================================================
void NoteEditor::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    drag = Drag::none;
    dragged = false;

    if (! shown.has_value())
        return;

    // The lane: a point grabbed, removed with Option, or a new one put where the mouse is.
    if (laneController >= 0 && laneArea().contains (e.getPosition()))
    {
        base = shown;
        const auto hit = lanePointAt (e.position);
        auto* lane = const_cast<ControllerLane*> (shown->clip.findLane (laneController));

        if (hit >= 0 && lane != nullptr && e.mods.isAltDown())
        {
            lane->envelope.remove (hit);
            commit ("Remove Lane Point");
            drag = Drag::nothing;
            return;
        }

        if (hit >= 0)
        {
            dragPoint = hit;
            drag = Drag::lanePoint;
            return;
        }

        if (e.mods.isPopupMenu())
        {
            drag = Drag::nothing;
            return;
        }

        if (lane == nullptr)
        {
            shown->clip.lanes.push_back ({ laneController, {} });
            lane = &shown->clip.lanes.back();
        }

        dragPoint = lane->envelope.add (snapped (frameAt (e.position.x)), laneValueAt (e.position.y));
        drag = Drag::lanePoint;
        commit ("Lane Point");
        base = shown;
        return;
    }

    if (showingDrums())
    {
        drumMouseDown (e);
        return;
    }

    // The keyboard: the key under the mouse sounds.
    if (const auto roll = rollArea(); juce::Rectangle<int> (0, roll.getY(), keyboardWidth, roll.getHeight()).contains (e.getPosition()))
    {
        drag = Drag::keys;
        sound (pitchAt (e.position.y), getVelocity());
        return;
    }

    if (! rollArea().contains (e.getPosition()))
        return;

    const auto p = e.position;
    const auto hit = noteAt (p);
    base = shown;
    dragStart = p;

    if (hit >= 0)
    {
        if (e.mods.isAltDown())
        {
            shown->clip.notes.erase (shown->clip.notes.begin() + hit);
            selected.clear();
            commit ("Remove Note");
            drag = Drag::nothing;
            return;
        }

        if (e.mods.isShiftDown())
        {
            if (selected.count (hit) > 0)
                selected.erase (hit);
            else
                selected.insert (hit);

            drag = Drag::nothing;
            refreshControls();
            repaint();
            return;
        }

        if (selected.count (hit) == 0)
            selected = { hit };

        const auto& n = shown->clip.notes[(size_t) hit];
        const auto r = noteBounds (n);
        drag = p.x >= r.getRight() - juce::jmin (6.0f, r.getWidth() / 3.0f) ? Drag::resize : Drag::move;
        dragNote = hit;
        dragFrame = frameAt (p.x);
        dragPitch = pitchAt (p.y);
        sound (n.pitch, n.velocity);
        velocitySlider.setValue (std::round (n.velocity * 127.0f), juce::dontSendNotification);
        refreshControls();
        repaint();
        return;
    }

    if (e.mods.isAltDown() || e.mods.isPopupMenu())
    {
        drag = Drag::nothing;
        return;
    }

    // The first repeat line: dragging it sets the loop length.
    if (shown->clip.loopLength > 0 && shown->clip.loopLength < wholeLength() && std::abs (p.x - xOf (shown->clip.loopLength)) <= 4.0f)
    {
        drag = Drag::loop;
        return;
    }

    // Empty grid: a note here, or with Shift (or a drag) a box of notes.
    if (e.mods.isShiftDown())
    {
        drag = Drag::box;
        box = { p, p };
        return;
    }

    const auto step = gridStepFrames();
    ClipNote n;
    n.pitch = pitchAt (p.y);
    n.start = snapped (frameAt (p.x));
    n.length = step > 0 ? step : lastLength > 0 ? lastLength : juce::jmax ((juce::int64) 1, (juce::int64) std::llround (0.25 * shown->sampleRate));
    n.velocity = getVelocity();

    if (n.start >= wholeLength())
        return;

    n.length = juce::jmin (n.length, wholeLength() - n.start);
    shown->clip.notes.push_back (n);
    selected = { (int) shown->clip.notes.size() - 1 };
    dragNote = (int) shown->clip.notes.size() - 1;
    dragFrame = n.start;
    dragPitch = n.pitch;
    sound (n.pitch, n.velocity);
    drag = Drag::resize;   // dragging on sets its length
    base = shown;
    commit ("Add Note");
    base = shown;
}

void NoteEditor::drumMouseDown (const juce::MouseEvent& e)
{
    const auto roll = rollArea();
    const auto rows = drumRows();

    // The names: a click sounds the row's drum, a right-click edits the map.
    if (juce::Rectangle<int> (0, roll.getY(), drumNamesWidth, roll.getHeight()).contains (e.getPosition()))
    {
        const auto row = firstRow + (int) std::floor ((e.position.y - (float) roll.getY()) / (float) drumRowHeight);

        if (! juce::isPositiveAndBelow (row, (int) rows.size()))
            return;

        if (e.mods.isPopupMenu())
        {
            drag = Drag::nothing;
            drumMapMenu (row);
            return;
        }

        drag = Drag::keys;
        sound (rows[(size_t) row].pitch, getVelocity());
        return;
    }

    const auto [row, step] = cellAt (e.position);

    if (row < 0 || e.mods.isPopupMenu())
    {
        drag = Drag::nothing;
        return;
    }

    auto hit = noteInCell (row, step);
    addedNow = false;

    if (hit >= 0 && e.mods.isAltDown())
    {
        shown->clip.notes.erase (shown->clip.notes.begin() + hit);
        selected.clear();
        commit ("Remove Note");
        drag = Drag::nothing;
        return;
    }

    if (hit < 0)
    {
        ClipNote n;
        n.pitch = rows[(size_t) row].pitch;
        n.start = lineFrame (step);
        n.length = juce::jmax ((juce::int64) 1, juce::jmin (straightStep(), wholeLength() - n.start));
        n.velocity = getVelocity();
        shown->clip.notes.push_back (n);
        hit = (int) shown->clip.notes.size() - 1;
        addedNow = true;
        commit ("Add Note");
    }

    const auto& n = shown->clip.notes[(size_t) hit];
    selected = { hit };
    dragNote = hit;
    dragStart = e.position;
    base = shown;
    drag = Drag::velocity;
    sound (n.pitch, n.velocity);
    velocitySlider.setValue (std::round (n.velocity * 127.0f), juce::dontSendNotification);
    refreshControls();
    repaint();
}

void NoteEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (drag == Drag::keys && showingDrums())
    {
        const auto roll = rollArea();
        const auto rows = drumRows();
        const auto row = firstRow + (int) std::floor ((juce::jlimit ((float) roll.getY(), (float) roll.getBottom() - 1.0f, e.position.y) - (float) roll.getY()) / (float) drumRowHeight);

        if (juce::isPositiveAndBelow (row, (int) rows.size()))
            sound (rows[(size_t) row].pitch, getVelocity());

        return;
    }

    if (drag == Drag::keys)
    {
        const auto roll = rollArea();
        sound (pitchAt (juce::jlimit ((float) roll.getY(), (float) roll.getBottom() - 1.0f, e.position.y)), getVelocity());
        return;
    }

    if (! shown.has_value() || ! base.has_value() || drag == Drag::none || drag == Drag::nothing)
        return;

    const auto p = e.position;

    // A drum step's velocity: up louder, down softer, 100 pixels from silent to full.
    if (drag == Drag::velocity)
    {
        if (! juce::isPositiveAndBelow (dragNote, (int) shown->clip.notes.size()) || ! juce::isPositiveAndBelow (dragNote, (int) base->clip.notes.size()))
            return;

        if (! dragged && std::abs (p.y - dragStart.y) < 3.0f)
            return;

        dragged = true;
        const auto v = juce::jlimit (1.0f / 127.0f, 1.0f, base->clip.notes[(size_t) dragNote].velocity + (dragStart.y - p.y) / 100.0f);
        shown->clip.notes[(size_t) dragNote].velocity = v;
        velocitySlider.setValue (std::round (v * 127.0f), juce::dontSendNotification);
        repaint();
        return;
    }

    if (drag == Drag::loop)
    {
        shown->clip.loopLength = juce::jlimit ((juce::int64) 1, wholeLength(), snapped (frameAt (p.x)));
        dragged = true;
        repaint();
        return;
    }

    if (drag == Drag::lanePoint)
    {
        if (auto* lane = const_cast<ControllerLane*> (shown->clip.findLane (laneController)))
        {
            dragPoint = lane->envelope.move (dragPoint, snapped (frameAt (p.x)), laneValueAt (p.y));
            dragged = true;
            repaint();
        }

        return;
    }

    if (drag == Drag::box)
    {
        box = juce::Rectangle<float>::leftTopRightBottom (juce::jmin (dragStart.x, p.x), juce::jmin (dragStart.y, p.y), juce::jmax (dragStart.x, p.x), juce::jmax (dragStart.y, p.y));
        selected.clear();

        for (size_t i = 0; i < shown->clip.notes.size(); ++i)
            if (noteBounds (shown->clip.notes[i]).intersects (box))
                selected.insert ((int) i);

        dragged = true;
        refreshControls();
        repaint();
        return;
    }

    if (! juce::isPositiveAndBelow (dragNote, (int) base->clip.notes.size()))
        return;

    dragged = true;
    shown->clip.notes = base->clip.notes;
    const auto length = wholeLength();

    if (drag == Drag::resize)
    {
        // The dragged note's end goes to the mouse (snapped); the other selected notes' lengths change by as much.
        const auto& was = base->clip.notes[(size_t) dragNote];
        const auto end = juce::jmax (was.start + 1, juce::jmin (length, snapped (frameAt (p.x) + (gridStepFrames() > 0 ? gridStepFrames() : 0))));
        const auto delta = (end - was.start) - was.length;

        for (auto i : selected)
            if (juce::isPositiveAndBelow (i, (int) shown->clip.notes.size()))
            {
                auto& n = shown->clip.notes[(size_t) i];
                n.length = juce::jlimit ((juce::int64) 1, length - n.start, n.length + delta);
            }

        lastLength = shown->clip.notes[(size_t) dragNote].length;
    }
    else
    {
        const auto semitones = pitchAt (p.y) - dragPitch;
        auto frames = snapped (frameAt (p.x)) - snapped (dragFrame);

        // Nothing moves before the clip's start or past its end.
        for (auto i : selected)
            if (juce::isPositiveAndBelow (i, (int) base->clip.notes.size()))
            {
                const auto& n = base->clip.notes[(size_t) i];
                frames = juce::jmax (frames, -n.start);
                frames = juce::jmin (frames, length - 1 - n.start);
            }

        for (auto i : selected)
            if (juce::isPositiveAndBelow (i, (int) shown->clip.notes.size()))
            {
                auto& n = shown->clip.notes[(size_t) i];
                n.pitch = juce::jlimit (0, 127, n.pitch + semitones);
                n.start += frames;
                n.length = juce::jmin (n.length, length - n.start);
            }

        const auto& moved = shown->clip.notes[(size_t) dragNote];
        sound (moved.pitch, moved.velocity);
    }

    repaint();
}

void NoteEditor::mouseUp (const juce::MouseEvent&)
{
    silence();
    const auto was = std::exchange (drag, Drag::none);

    if (was == Drag::box)
    {
        refreshControls();
        repaint();
        return;
    }

    // A click on a set drum step (not one this click set) takes it away; a drag set its velocity.
    if (was == Drag::velocity && shown.has_value() && juce::isPositiveAndBelow (dragNote, (int) shown->clip.notes.size()))
    {
        if (dragged)
            commit ("Note Velocity");
        else if (! addedNow)
        {
            shown->clip.notes.erase (shown->clip.notes.begin() + dragNote);
            selected.clear();
            commit ("Remove Note");
        }

        base.reset();
        refreshControls();
        repaint();
        return;
    }

    if ((was == Drag::move || was == Drag::resize || was == Drag::loop || was == Drag::lanePoint) && dragged && shown.has_value() && base.has_value() && ! (shown->clip == base->clip))
        commit (was == Drag::move ? "Move Notes" : was == Drag::loop ? "Loop Length" : was == Drag::lanePoint ? "Lane Point" : "Note Length");

    base.reset();
    refreshControls();
    repaint();
}

void NoteEditor::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isCommandDown())
    {
        zoomAround (e.position.x, std::pow (2.0, (double) wheel.deltaY * 2.0));
        return;
    }

    const auto sideways = e.mods.isShiftDown() ? wheel.deltaY : wheel.deltaX;

    if (std::abs (sideways) > 0.0f && (e.mods.isShiftDown() || std::abs (wheel.deltaX) > std::abs (wheel.deltaY)))
    {
        setView (viewStart - (juce::int64) std::llround ((double) sideways * (double) visibleLength() * 0.5), zoom);
        return;
    }

    if (e.mods.isAltDown() && ! showingDrums())
    {
        setRowHeight (rowPixels + (int) std::round (wheel.deltaY * 16.0f));
        return;
    }

    if (showingDrums())
    {
        const auto visible = juce::jmax (1, rollArea().getHeight() / drumRowHeight);
        firstRow = juce::jlimit (0, juce::jmax (0, (int) drumRows().size() - visible), firstRow - (int) std::round (wheel.deltaY * 12.0f));
        repaint();
        return;
    }

    const auto rows = juce::jmax (1, rollArea().getHeight() / rowPixels);
    lowPitch = juce::jlimit (0, juce::jmax (0, 128 - rows), lowPitch + (int) std::round (wheel.deltaY * 12.0f));
    repaint();
}

void NoteEditor::mouseMagnify (const juce::MouseEvent& e, float scaleFactor)
{
    if (scaleFactor > 0.0f)
        zoomAround (e.position.x, (double) scaleFactor);
}

bool NoteEditor::keyPressed (const juce::KeyPress& key)
{
    if (! shown.has_value())
        return false;

    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        removeSelected();
        return true;
    }

    if (key == juce::KeyPress ('a', juce::ModifierKeys::commandModifier, 0))
    {
        selectAll();
        return true;
    }

    const auto step = gridStepFrames() > 0 ? gridStepFrames() : juce::jmax ((juce::int64) 1, (juce::int64) std::llround (0.05 * shown->sampleRate));
    const auto shift = key.getModifiers().isShiftDown();

    if (key.getKeyCode() == juce::KeyPress::upKey)         moveSelected (shift ? 12 : 1, 0);
    else if (key.getKeyCode() == juce::KeyPress::downKey)  moveSelected (shift ? -12 : -1, 0);
    else if (key.getKeyCode() == juce::KeyPress::leftKey)  moveSelected (0, shift ? -juce::jmax ((juce::int64) 1, step / 4) : -step);
    else if (key.getKeyCode() == juce::KeyPress::rightKey) moveSelected (0, shift ? juce::jmax ((juce::int64) 1, step / 4) : step);
    else
        return false;

    return true;
}

void NoteEditor::moveSelected (int semitones, juce::int64 frames)
{
    if (! shown.has_value() || selected.empty())
        return;

    const auto length = wholeLength();

    for (auto i : selected)
        if (juce::isPositiveAndBelow (i, (int) shown->clip.notes.size()))
        {
            const auto& n = shown->clip.notes[(size_t) i];
            frames = juce::jmax (frames, -n.start);
            frames = juce::jmin (frames, length - 1 - n.start);
        }

    for (auto i : selected)
        if (juce::isPositiveAndBelow (i, (int) shown->clip.notes.size()))
        {
            auto& n = shown->clip.notes[(size_t) i];
            n.pitch = juce::jlimit (0, 127, n.pitch + semitones);
            n.start += frames;
            n.length = juce::jmin (n.length, length - n.start);
        }

    commit ("Move Notes");
}

void NoteEditor::selectAll()
{
    if (! shown.has_value())
        return;

    selected.clear();

    for (size_t i = 0; i < shown->clip.notes.size(); ++i)
        selected.insert ((int) i);

    refreshControls();
    repaint();
}

void NoteEditor::removeSelected()
{
    if (! shown.has_value() || selected.empty())
        return;

    std::vector<ClipNote> kept;

    for (size_t i = 0; i < shown->clip.notes.size(); ++i)
        if (selected.count ((int) i) == 0)
            kept.push_back (shown->clip.notes[i]);

    const auto count = (int) shown->clip.notes.size() - (int) kept.size();
    shown->clip.notes = std::move (kept);
    selected.clear();
    commit (count > 1 ? "Remove " + juce::String (count) + " Notes" : juce::String ("Remove Note"));
}

void NoteEditor::commit (const juce::String& description)
{
    if (shown.has_value() && apply != nullptr)
        apply (shown->clip, description);

    refreshControls();
    repaint();
}
