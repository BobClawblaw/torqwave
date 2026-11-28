#include "NoteEditor.h"
#include "Notation.h"
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
            if (! shown->drums)
                showScore = false;
            showDrums (! shown->drums);
            refresh();
            resized();
        }
    };

    addAndMakeVisible (scoreButton);
    scoreButton.setClickingTogglesState (false);
    scoreButton.setTooltip ("Show the clip as standard grand-staff music notation (treble + bass staves) with direct note editing and MusicXML export");
    scoreButton.onClick = [this] { setShowScore (! showingScore()); };

    addChildComponent (exportMusicXmlButton);
    exportMusicXmlButton.setTooltip ("Export the current note clip as a standard MusicXML 3.1 (.musicxml) grand-staff score");
    exportMusicXmlButton.onClick = [this] { exportMusicXmlDialog(); };

    addAndMakeVisible (toolsButton);
    toolsButton.setTooltip ("Writing tools: chords from Roman numerals, arpeggiate, humanize, fit to the key, scale lock");
    toolsButton.onClick = [this] { showToolsMenu(); };

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

        // Typed, stepped or scrolled: kept at once. Dragged: kept when the drag ends (the clip
        // isn't read again meanwhile, which would undo it).
        if (velocityDragging)
            repaint();
        else
            commit ("Note Velocity");
    };
    velocitySlider.onDragStart = [this] { velocityDragging = true; };
    velocitySlider.onDragEnd = [this]
    {
        velocityDragging = false;

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
    startTimerHz (30);
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
    // The playhead every tick (30 a second), the clip every third.
    std::optional<juce::int64> now;

    if (readPlayhead != nullptr && shown.has_value())
        if (const auto frame = readPlayhead())
            now = timelineFrame (shown->clip, *frame);

    if (now != playhead)
    {
        playhead = now;
        repaint();
    }

    // Learning a drum's key: the next note from a MIDI input.
    if (learningRow >= 0 && readNoteIn != nullptr)
        if (const auto [count, pitch] = readNoteIn(); count != notesInSeen)
            learn (pitch);

    if (++ticks % 3 == 0 && drag == Drag::none && ! velocityDragging)
        refresh();
}

std::optional<juce::int64> NoteEditor::timelineFrame (const NoteClip& clip, juce::int64 frame)
{
    if (frame < clip.position || frame >= clip.getEnd())
        return std::nullopt;

    return (frame - clip.getOrigin()) % juce::jmax ((juce::int64) 1, clip.getPeriod());
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
    drumsButton.setEnabled (on && ! scoreOnly);
    scoreButton.setEnabled (on && ! scoreOnly);
    exportMusicXmlButton.setEnabled (on && ! shown->clip.notes.empty());
    toolsButton.setEnabled (on);
    drumsButton.setToggleState (showingDrums(), juce::dontSendNotification);
    scoreButton.setToggleState (showingScore(), juce::dontSendNotification);

    if (on)
        channelBox.setSelectedId (juce::jlimit (1, 16, shown->clip.channel), juce::dontSendNotification);

    if (! on)
    {
        status.setText (scoreOnly
                            ? "Select a note clip in Multitrack (right-click a track -> Add Notes...) to view and edit it in Score / Notation."
                            : "Select a note clip in the multitrack (Add Notes... in a track's menu makes one).",
                        juce::dontSendNotification);
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
    const auto value = juce::jlimit (1.0, 127.0, (double) std::round (velocity * 127.0f));

    // The slider keeps a change itself (onValueChange); at its value already, the chosen notes still take it.
    if (! juce::exactlyEqual (velocitySlider.getValue(), value))
        velocitySlider.setValue (value, juce::sendNotificationSync);
    else if (velocitySlider.onValueChange != nullptr)
        velocitySlider.onValueChange();
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
    if (showingDrums()) return drumNamesWidth;
    if (showingScore()) return scoreClefWidth;
    return keyboardWidth;
}

void NoteEditor::setShowScore (bool on)
{
    if (scoreOnly)
        on = true;
    if (on && showingDrums() && showDrums != nullptr)
    {
        showDrums (false);
        refresh();
    }
    showScore = on;
    refreshControls();
    resized();
    repaint();
}

void NoteEditor::setScoreOnlyMode (bool on)
{
    scoreOnly = on;
    showScore = on;
    drumsButton.setVisible (! on);
    scoreButton.setVisible (! on);
    exportMusicXmlButton.setVisible (on);
    refreshControls();
    resized();
    repaint();
}

juce::String NoteEditor::exportMusicXml (const juce::File& file) const
{
    if (! shown.has_value())
        return "No note clip is selected.";

    return Notation::exportMusicXml (shown->clip, shown->sampleRate, shown->beatSeconds,
                                     shown->beatsPerBar, getKey(), file, shown->clip.name);
}

void NoteEditor::exportMusicXmlDialog()
{
    if (! shown.has_value())
        return;

    const auto suggested = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                               .getChildFile ((shown->clip.name.isNotEmpty() ? shown->clip.name : "Score") + ".musicxml");
    exportChooser = std::make_unique<juce::FileChooser> ("Export MusicXML Score", suggested, "*.musicxml;*.xml");
    exportChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                                [safe = juce::Component::SafePointer<NoteEditor> (this)] (const juce::FileChooser& c)
    {
        if (safe == nullptr || c.getResult() == juce::File())
            return;

        const auto out = c.getResult().withFileExtension (".musicxml");
        const auto err = safe->exportMusicXml (out);
        safe->status.setText (err.isNotEmpty() ? err : ("Exported MusicXML to " + out.getFileName() + "."), juce::dontSendNotification);
    });
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
    menu.addItem (9, learningRow == row ? "Stop Learning" : "Learn Its Note from MIDI", readNoteIn != nullptr);
    menu.addSeparator();

    // A kit laid out its own way: the clip's General MIDI notes moved to its keys, or back.
    const auto toKit = notesToRemap (true), back = notesToRemap (false);
    menu.addItem (7, toKit > 0 ? "Remap Notes to This Map (" + juce::String (toKit) + ")" : juce::String ("Remap Notes to This Map"), toKit > 0);
    menu.addItem (8, back > 0 ? "Remap Notes to General MIDI (" + juce::String (back) + ")" : juce::String ("Remap Notes to General MIDI"), back > 0);
    menu.addSeparator();

    const auto saved = drumMaps().names();
    juce::PopupMenu maps, remove;

    for (int i = 0; i < saved.size(); ++i)
    {
        maps.addItem (100 + i, saved[i]);
        remove.addItem (1000 + i, saved[i]);
    }

    menu.addSubMenu ("Maps", maps, ! saved.isEmpty());
    menu.addItem (10, "Save Map As...");
    menu.addSubMenu ("Delete a Saved Map", remove, ! saved.isEmpty());
    menu.addItem (6, "General MIDI Map", ! (shown->drumMap == DrumSound::generalMidi()));

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(),
                        [safe = juce::Component::SafePointer<NoteEditor> (this), row, inMap, saved] (int choice)
    {
        if (safe == nullptr || ! safe->shown.has_value())
            return;

        auto map = safe->shown->drumMap;

        if (choice >= 1000 && choice - 1000 < saved.size())
        {
            safe->drumMaps().remove (saved[choice - 1000]);
            return;
        }

        if (choice >= 100 && choice - 100 < saved.size())
        {
            if (const auto loaded = safe->drumMaps().load (saved[choice - 100]); ! loaded.empty())
                safe->setDrumMap (loaded, "Drum Map " + saved[choice - 100]);

            return;
        }

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
            case 7: safe->remapNotes (true); break;
            case 8: safe->remapNotes (false); break;
            case 9:
                if (safe->learningRow == row)
                    safe->learningRow = -1;
                else
                    safe->startLearning (row);

                safe->repaint();
                break;
            case 10: safe->saveDrumMapAs(); break;
            default: break;
        }
    });
}

DrumMaps::Store& NoteEditor::drumMaps() const
{
    return mapStore != nullptr ? *mapStore : DrumMaps::userStore();
}

void NoteEditor::saveDrumMapAs()
{
    if (! shown.has_value())
        return;

    auto* window = new juce::AlertWindow ("Save the Drum Map", "A name for this kit's map, to use on other tracks and in other sessions.", juce::MessageBoxIconType::NoIcon);
    window->addTextEditor ("name", shown->trackName, "Name");
    window->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    window->enterModalState (true, juce::ModalCallbackFunction::create (
        [safe = juce::Component::SafePointer<NoteEditor> (this), window] (int result)
    {
        if (result != 1 || safe == nullptr || ! safe->shown.has_value())
            return;

        if (const auto error = safe->drumMaps().save (window->getTextEditorContents ("name"), safe->shown->drumMap); error.isNotEmpty())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Save the Drum Map", error);
    }), true);
}

int NoteEditor::notesToRemap (bool toKit) const
{
    if (! shown.has_value() || ! DrumSound::remaps (shown->drumMap))
        return 0;

    return (int) std::count_if (shown->clip.notes.begin(), shown->clip.notes.end(), [&] (const ClipNote& n)
    {
        return (toKit ? DrumSound::toKit (shown->drumMap, n.pitch) : DrumSound::toGeneralMidi (shown->drumMap, n.pitch)) != n.pitch;
    });
}

int NoteEditor::remapNotes (bool toKit)
{
    const auto moving = notesToRemap (toKit);

    if (moving == 0)
        return 0;

    // Every note from where it was: a kit key that is also a General MIDI note isn't moved twice.
    for (auto& n : shown->clip.notes)
        n.pitch = toKit ? DrumSound::toKit (shown->drumMap, n.pitch) : DrumSound::toGeneralMidi (shown->drumMap, n.pitch);

    selected.clear();
    commit (toKit ? "Remap Notes to the Drum Map" : "Remap Notes to General MIDI");
    return moving;
}

void NoteEditor::startLearning (int row)
{
    learningRow = row;
    notesInSeen = readNoteIn != nullptr ? readNoteIn().first : 0u;
}

bool NoteEditor::learn (int pitch)
{
    const auto row = learningRow;
    learningRow = -1;
    const auto rows = drumRows();

    if (! shown.has_value() || ! juce::isPositiveAndBelow (row, (int) rows.size()) || ! juce::isPositiveAndBelow (pitch, 128))
        return false;

    auto map = shown->drumMap;

    if (row >= (int) map.size())
        map.push_back (rows[(size_t) row]);   // a note outside the map joins it

    auto& sound = map[(size_t) juce::jmin (row, (int) map.size() - 1)];

    // A General MIDI sound given another key stands for the note it had, so patterns can follow it there.
    if (sound.standsFor < 0 && sound.pitch != pitch && sound.pitch >= 35 && sound.pitch <= 81)
        sound.standsFor = sound.pitch;

    sound.pitch = pitch;
    setDrumMap (map, "Learn Drum Note");
    return true;
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

    // The General MIDI drum it stands for: what Remap Notes moves here.
    juce::StringArray drums { "Its own note (nothing moved here)" };

    for (auto& d : DrumSound::generalMidi())
        drums.add (juce::String (d.pitch) + "  " + d.name);

    window->addComboBox ("standsFor", drums, "Stands for");
    const auto stoodFor = adding && ! extra ? -1 : was.standsFor;
    window->getComboBoxComponent ("standsFor")->setSelectedItemIndex (stoodFor >= 35 && stoodFor <= 81 ? stoodFor - 34 : 0);
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
        const auto chosen = window->getComboBoxComponent ("standsFor")->getSelectedItemIndex();
        auto standsFor = chosen > 0 ? 34 + chosen : -1;

        // A General MIDI sound given another key stands for the note it had, unless told otherwise.
        if (standsFor < 0 && ! adding && juce::isPositiveAndBelow (row, (int) map.size()) && map[(size_t) row].standsFor < 0
            && map[(size_t) row].pitch != pitch && map[(size_t) row].pitch >= 35 && map[(size_t) row].pitch <= 81)
            standsFor = map[(size_t) row].pitch;

        const DrumSound sound { pitch, name.isNotEmpty() ? name : DrumSound::nameIn ({}, pitch), standsFor };

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
    if (showingScore())
    {
        const auto gs = Notation::GrandStaff::fit (rollArea().toFloat());
        return Notation::diatonicToMidi (gs.diatonicAtY (y), getKey());
    }

    return juce::jlimit (0, 127, lowPitch + (int) std::floor (((float) rollArea().getBottom() - y) / (float) rowPixels));
}

juce::int64 NoteEditor::wholeLength() const noexcept
{
    return shown.has_value() ? juce::jmax ((juce::int64) 1, shown->clip.offset + shown->clip.length) : (juce::int64) 1;
}

juce::int64 NoteEditor::noteEndLimit() const noexcept
{
    if (! shown.has_value() || ! shown->holdOver)
        return wholeLength();

    return wholeLength() + juce::jmax ((juce::int64) 1, shown->clip.getPeriod());
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
    if (showingScore())
    {
        const auto gs = Notation::GrandStaff::fit (rollArea().toFloat());
        const auto sp = Notation::spell (n.pitch, Notation::prefersFlats (getKey()));
        const float hw = gs.space * 1.25f;
        const float hh = gs.space * 0.95f;
        const float cx = xOf (n.start) + hw * 0.75f;
        const float cy = gs.yForDiatonic (sp.diatonic);
        return { cx - hw * 0.5f, cy - hh * 0.5f, juce::jmax (hw, xOf (n.start + n.length) - xOf (n.start)), hh };
    }

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

int NoteEditor::getHighPitch() const
{
    return juce::jmin (127, lowPitch + juce::jmax (1, rollArea().getHeight() / rowPixels) - 1);
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
    placedRows = rows;
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
        if (showingScore())
        {
            const auto gutter = juce::Rectangle<int> (0, area.getY(), scoreClefWidth, area.getHeight());
            g.setColour (Theme::surface);
            g.fillRect (gutter);
            g.setColour (Theme::lineStrong);
            g.drawVerticalLine (scoreClefWidth - 1, (float) area.getY(), (float) area.getBottom());

            const auto gs = Notation::GrandStaff::fit (area.toFloat());
            const float staffLeft = 8.0f;
            const float staffRight = (float) area.getRight();
            g.setColour (Theme::lineStrong.withAlpha (0.45f));
            for (int d : { 30, 32, 34, 36, 38, 18, 20, 22, 24, 26 })
                g.drawHorizontalLine ((int) std::round (gs.yForDiatonic (d)), staffLeft, staffRight);

            const float topLineY = gs.yForDiatonic (38);
            const float botLineY = gs.yForDiatonic (18);
            g.setColour (Theme::dimText.withAlpha (0.75f));
            g.drawVerticalLine ((int) staffLeft, topLineY, botLineY);

            g.setFont (juce::FontOptions (gs.space * 2.1f, juce::Font::bold));
            g.drawText ("G", juce::Rectangle<float> (10.0f, gs.yForDiatonic (36) - gs.space * 0.8f, 20.0f, gs.space * 3.0f), juce::Justification::centred, false);
            g.drawText ("F", juce::Rectangle<float> (10.0f, gs.yForDiatonic (26) - gs.space * 0.5f, 20.0f, gs.space * 2.8f), juce::Justification::centred, false);
            g.fillEllipse (27.0f, gs.yForDiatonic (25) - 1.5f, 3.0f, 3.0f);
            g.fillEllipse (27.0f, gs.yForDiatonic (23) - 1.5f, 3.0f, 3.0f);

            const float tsX = (float) scoreClefWidth - 20.0f;
            g.setFont (juce::FontOptions (gs.space * 1.5f, juce::Font::bold));
            g.drawText ("4", juce::Rectangle<float> (tsX, gs.yForDiatonic (38), 16.0f, gs.space * 2.0f), juce::Justification::centred, false);
            g.drawText ("4", juce::Rectangle<float> (tsX, gs.yForDiatonic (34), 16.0f, gs.space * 2.0f), juce::Justification::centred, false);
            g.drawText ("4", juce::Rectangle<float> (tsX, gs.yForDiatonic (26), 16.0f, gs.space * 2.0f), juce::Justification::centred, false);
            g.drawText ("4", juce::Rectangle<float> (tsX, gs.yForDiatonic (22), 16.0f, gs.space * 2.0f), juce::Justification::centred, false);

            g.setColour (Theme::dimText);
            g.setFont (juce::FontOptions (13.0f));
            g.drawText ("Select a note clip in Multitrack to view and edit it in Grand-Staff Notation",
                        area.reduced (16), juce::Justification::centred, true);
            g.setColour (Theme::line);
            g.drawRect (area);
            return;
        }

        g.setColour (Theme::surface);
        g.fillRect (keys);
        return;
    }

    if (showingDrums())
        paintDrums (g, area);
    else if (showingScore())
        paintScore (g, area);
    else
        paintRoll (g, area);

    g.setColour (Theme::line);
    g.drawRect (area);

    const auto& c = shown->clip;

    if (playhead.has_value())
    {
        const auto x = xOf (*playhead);
        const auto bottom = laneController >= 0 ? laneArea().getBottom() : area.getBottom();

        if (x >= (float) area.getX() && x <= (float) area.getRight())
        {
            g.setColour (Theme::textStrong);   // as the multitrack view's playhead
            g.fillRect (juce::Rectangle<float> (x - 0.5f, (float) area.getY(), 1.5f, (float) (bottom - area.getY())));
        }
    }

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

void NoteEditor::paintBars (juce::Graphics& g, juce::Rectangle<int> area)
{
    // Each bar of the notes' timeline (from its start, in the clip's beats): a bold line and its number.
    const auto barFrames = shown->beatSeconds * shown->sampleRate * juce::jmax (1, shown->beatsPerBar);

    if (barFrames <= 0.0 || xOf ((juce::int64) barFrames) - xOf (0) < 12.0f)
        return;

    const auto whole = wholeLength();
    g.setFont (10.0f);

    for (int bar = 0; ; ++bar)
    {
        const auto f = (juce::int64) std::llround (bar * barFrames);

        if (f > whole)
            break;

        const auto x = xOf (f);

        if (x > (float) area.getRight())
            break;

        g.setColour (Theme::dimText);
        g.fillRect (juce::Rectangle<float> (x - 1.0f, (float) area.getY(), 2.0f, (float) area.getHeight()));

        if (f < whole && x >= (float) area.getX() - 30.0f)
        {
            const auto label = juce::Rectangle<float> (x + 3.0f, (float) area.getY() + 2.0f, 26.0f, 13.0f);
            g.setColour (Theme::canvas.withAlpha (0.8f));
            g.fillRoundedRectangle (label, 2.0f);
            g.setColour (Theme::text);
            g.drawText (juce::String (bar + 1), label, juce::Justification::centredLeft, false);
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

        if (scaleLock && ! getKey().contains (pitch))
        {
            g.setColour (Theme::overlay.withAlpha (0.3f));
            g.fillRect (r.getIntersection (area.toFloat()));
        }

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

    paintBars (g, area);

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

    // Ghost notes from other instrument tracks, drawn faintly behind the active clip's notes.
    if (showGhostNotes && ! shown->ghostNotes.empty())
    {
        g.setColour (Theme::dimText.withAlpha (0.22f));
        for (const auto& gn : shown->ghostNotes)
        {
            const auto gr = noteBounds (gn);
            if (gr.getBottom() < (float) area.getY() || gr.getY() > (float) area.getBottom())
                continue;
            g.fillRoundedRectangle (gr.getIntersection (area.toFloat()), 2.0f);
        }
    }

    // The notes: velocity as their depth of colour, the selected outlined.
    for (size_t i = 0; i < c.notes.size(); ++i)
    {
        const auto& n = c.notes[i];
        const auto r = noteBounds (n);

        if (r.getBottom() < (float) area.getY() || r.getY() > (float) area.getBottom())
            continue;

        const auto playing = n.start < period && n.start >= c.offset && shown->hasInstrument;

        // Held over the loop's end: what sounds into the next pass, faint at the start.
        if (shown->holdOver && n.start < period && n.start + n.length > period)
        {
            const auto carried = juce::jmin (period, n.start + n.length - period);
            g.setColour (Theme::accent.withAlpha (0.25f));
            g.fillRoundedRectangle (juce::Rectangle<float> (xOf (0), r.getY(), xOf (carried) - xOf (0), r.getHeight()).getIntersection (area.toFloat()), 2.0f);
        }

        g.setColour ((playing ? Theme::accent : Theme::dimText).withAlpha (0.45f + 0.55f * juce::jlimit (0.0f, 1.0f, n.velocity)));
        g.fillRoundedRectangle (r.getIntersection (area.toFloat()), 2.0f);

        if ((n.probability < 1.0f || n.ratchets > 1) && r.getWidth() >= 22.0f && r.getHeight() >= 9.0f)
        {
            juce::String tag;
            if (n.ratchets > 1) tag += juce::String (n.ratchets) + "x";
            if (n.probability < 1.0f) tag += (tag.isNotEmpty() ? " " : "") + juce::String (juce::roundToInt (n.probability * 100.0f)) + "%";
            g.setColour (Theme::textStrong);
            g.setFont (juce::jmin (11.0f, r.getHeight() - 1.0f));
            g.drawText (tag, r.toNearestInt(), juce::Justification::centred, false);
        }

        if (selected.count ((int) i) > 0)
        {
            g.setColour (Theme::textStrong);
            g.drawRoundedRectangle (r.getIntersection (area.toFloat()), 2.0f, 1.5f);

            // Its end's grip: drag it for the length (or Option-left and right).
            if (r.getWidth() >= 8.0f)
                g.fillRect (juce::Rectangle<float> (r.getRight() - 4.0f, r.getY() + 1.0f, 3.0f, r.getHeight() - 2.0f).getIntersection (area.toFloat()));
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

void NoteEditor::paintScore (juce::Graphics& g, juce::Rectangle<int> area)
{
    const auto& c = shown->clip;
    const auto gs = Notation::GrandStaff::fit (area.toFloat());
    const auto key = getKey();
    const int fifths = Notation::keyFifths (key);
    const bool flats = fifths < 0;
    const auto whole = wholeLength();
    const auto period = c.getPeriod();
    const double beatFramesD = juce::jmax (1.0, shown->beatSeconds * shown->sampleRate);

    // Left clef + key signature + time signature gutter
    const auto gutter = juce::Rectangle<int> (0, area.getY(), scoreClefWidth, area.getHeight());
    g.setColour (Theme::surface);
    g.fillRect (gutter);
    g.setColour (Theme::lineStrong);
    g.drawVerticalLine (scoreClefWidth - 1, (float) area.getY(), (float) area.getBottom());

    // Staff lines across both gutter and score canvas: Treble (E4..F5: 30,32,34,36,38) & Bass (G2..A3: 18,20,22,24,26)
    const float staffLeft = 8.0f;
    const float staffRight = (float) area.getRight();
    g.setColour (Theme::lineStrong.withAlpha (0.75f));
    for (int d : { 30, 32, 34, 36, 38, 18, 20, 22, 24, 26 })
    {
        const float y = gs.yForDiatonic (d);
        g.drawHorizontalLine ((int) std::round (y), staffLeft, staffRight);
    }

    // Grand staff brace + left vertical bar connecting F5 to G2
    const float topLineY = gs.yForDiatonic (38);
    const float botLineY = gs.yForDiatonic (18);
    g.setColour (Theme::text.withAlpha (0.85f));
    g.drawVerticalLine ((int) staffLeft, topLineY, botLineY);

    // Clef glyphs & labels (G treble, F bass)
    g.setColour (Theme::textStrong);
    g.setFont (juce::FontOptions (gs.space * 2.1f, juce::Font::bold));
    g.drawText ("G", juce::Rectangle<float> (10.0f, gs.yForDiatonic (36) - gs.space * 0.8f, 20.0f, gs.space * 3.0f), juce::Justification::centred, false);
    g.drawText ("F", juce::Rectangle<float> (10.0f, gs.yForDiatonic (26) - gs.space * 0.5f, 20.0f, gs.space * 2.8f), juce::Justification::centred, false);
    // Bass clef dots flanking F3 (diatonic 24)
    g.fillEllipse (27.0f, gs.yForDiatonic (25) - 1.5f, 3.0f, 3.0f);
    g.fillEllipse (27.0f, gs.yForDiatonic (23) - 1.5f, 3.0f, 3.0f);

    // Key signature accidentals in gutter
    static constexpr int trebleSharpD[7] = { 38, 35, 39, 36, 33, 37, 34 }; // F5 C5 G5 D5 A4 E5 B4
    static constexpr int trebleFlatD[7]  = { 34, 37, 33, 36, 32, 35, 31 }; // B4 E5 A4 D5 G4 C5 F4
    static constexpr int bassSharpD[7]   = { 24, 21, 25, 22, 19, 23, 20 }; // F3 C3 G3 D3 A2 E3 B2
    static constexpr int bassFlatD[7]    = { 20, 23, 19, 22, 18, 21, 17 }; // B2 E3 A2 D3 G2 C3 F2
    static constexpr int sharpSteps[7]   = { 3, 0, 4, 1, 5, 2, 6 };        // F C G D A E B
    static constexpr int flatSteps[7]    = { 6, 2, 5, 1, 4, 0, 3 };        // B E A D G C F
    int keyAcc[7] = {};
    if (fifths > 0)
        for (int i = 0; i < juce::jmin (7, fifths); ++i)
            keyAcc[sharpSteps[i]] = 1;
    else if (fifths < 0)
        for (int i = 0; i < juce::jmin (7, -fifths); ++i)
            keyAcc[flatSteps[i]] = -1;

    const int numAcc = juce::jmin (5, std::abs (fifths));
    g.setFont (juce::FontOptions (gs.space * 1.05f, juce::Font::bold));
    for (int k = 0; k < numAcc; ++k)
    {
        const float ax = 33.0f + (float) k * 8.0f;
        const juce::String sym = fifths > 0 ? "#" : "b";
        g.drawText (sym, juce::Rectangle<float> (ax, gs.yForDiatonic (fifths > 0 ? trebleSharpD[k] : trebleFlatD[k]) - gs.space * 0.7f, 14.0f, gs.space * 1.4f), juce::Justification::centred, false);
        g.drawText (sym, juce::Rectangle<float> (ax, gs.yForDiatonic (fifths > 0 ? bassSharpD[k] : bassFlatD[k]) - gs.space * 0.7f, 14.0f, gs.space * 1.4f), juce::Justification::centred, false);
    }

    // Time signature
    const float tsX = (float) scoreClefWidth - 20.0f;
    g.setFont (juce::FontOptions (gs.space * 1.5f, juce::Font::bold));
    g.drawText (juce::String (shown->beatsPerBar), juce::Rectangle<float> (tsX, gs.yForDiatonic (38), 16.0f, gs.space * 2.0f), juce::Justification::centred, false);
    g.drawText ("4", juce::Rectangle<float> (tsX, gs.yForDiatonic (34), 16.0f, gs.space * 2.0f), juce::Justification::centred, false);
    g.drawText (juce::String (shown->beatsPerBar), juce::Rectangle<float> (tsX, gs.yForDiatonic (26), 16.0f, gs.space * 2.0f), juce::Justification::centred, false);
    g.drawText ("4", juce::Rectangle<float> (tsX, gs.yForDiatonic (22), 16.0f, gs.space * 2.0f), juce::Justification::centred, false);

    // Subtle beat grid + bold barlines + loop lines
    {
        const juce::Graphics::ScopedSaveState clipScore (g);
        g.reduceClipRegion (area);

        if (gridDivision > 0)
        {
            const auto step = straightStep();
            if (step > 0 && xOf (step) - xOf (0) >= 8.0f)
            {
                for (juce::int64 k = 0; ; ++k)
                {
                    const auto f = lineFrame (k);
                    if (f > whole) break;
                    const float gx = xOf (f);
                    if (gx > (float) area.getRight()) break;
                    if (gx >= (float) area.getX())
                    {
                        g.setColour (k % gridDivision == 0 ? Theme::line.withAlpha (0.65f) : Theme::line.withAlpha (0.28f));
                        g.drawVerticalLine ((int) gx, topLineY, botLineY);
                    }
                }
            }
        }

        paintBars (g, area);

        if (period < whole)
            for (auto f = period; f < whole; f += period)
            {
                g.setColour (Theme::accent.withAlpha (f == period ? 0.9f : 0.6f));
                g.drawVerticalLine ((int) xOf (f), (float) area.getY(), (float) area.getBottom());
            }

        // Draw notes on grand staff
        const float headW = gs.space * 1.18f;
        const float headH = gs.space * 0.86f;
        const float stemH = gs.space * 3.1f;

        for (size_t i = 0; i < c.notes.size(); ++i)
        {
            const auto& n = c.notes[i];
            const float cx = xOf (n.start) + headW * 0.75f;
            if (cx + headW * 2.0f < (float) area.getX() || cx - headW * 2.0f > (float) area.getRight())
                continue;

            const auto sp = Notation::spell (n.pitch, flats);
            const float cy = gs.yForDiatonic (sp.diatonic);
            const double beats = (double) n.length / beatFramesD;
            const auto kind = Notation::classifyDuration (beats);
            const bool isSelected = selected.count ((int) i) > 0;
            const bool playing = n.start < period && n.start >= c.offset && shown->hasInstrument;

            const auto noteCol = isSelected ? Theme::accent : (playing ? Theme::textStrong : Theme::dimText);

            // Ledger lines (middle C = 28, above treble >= 40, below bass <= 16)
            g.setColour (Theme::lineStrong.withAlpha (0.9f));
            if (sp.diatonic == 28)
            {
                g.drawHorizontalLine ((int) std::round (cy), cx - headW * 0.78f, cx + headW * 0.78f);
            }
            else if (sp.diatonic >= 40)
            {
                for (int d = 40; d <= sp.diatonic; d += 2)
                    g.drawHorizontalLine ((int) std::round (gs.yForDiatonic (d)), cx - headW * 0.78f, cx + headW * 0.78f);
            }
            else if (sp.diatonic <= 16)
            {
                for (int d = 16; d >= sp.diatonic; d -= 2)
                    g.drawHorizontalLine ((int) std::round (gs.yForDiatonic (d)), cx - headW * 0.78f, cx + headW * 0.78f);
            }

            // Accidental left of notehead only when differing from the key signature
            const int expectedAcc = keyAcc[sp.diatonic % 7];
            if (sp.accidental != expectedAcc)
            {
                g.setColour (noteCol);
                if (sp.accidental == 0)
                {
                    // Draw a crisp natural sign (4 strokes, independent of glyph font support)
                    const float nx = cx - headW * 0.92f;
                    const float nw = headW * 0.28f;
                    const float nh = gs.space * 0.52f;
                    g.drawLine (nx - nw, cy - nh * 1.45f, nx - nw, cy + nh * 0.55f, 1.3f);
                    g.drawLine (nx + nw, cy - nh * 0.55f, nx + nw, cy + nh * 1.45f, 1.3f);
                    g.drawLine (nx - nw, cy - nh * 0.45f, nx + nw, cy - nh * 0.65f, 1.6f);
                    g.drawLine (nx - nw, cy + nh * 0.65f, nx + nw, cy + nh * 0.45f, 1.6f);
                }
                else
                {
                    g.setFont (juce::FontOptions (gs.space * 1.05f, juce::Font::bold));
                    g.drawText (sp.accidental > 0 ? "#" : "b",
                                juce::Rectangle<float> (cx - headW * 1.45f, cy - gs.space * 0.7f, headW * 0.92f, gs.space * 1.4f),
                                juce::Justification::centredRight, false);
                }
            }

            // Notehead (hollow for whole/half/dotted-half, filled for quarter/eighth/sixteenth)
            const bool hollow = (kind == Notation::DurationKind::whole || kind == Notation::DurationKind::half || kind == Notation::DurationKind::dottedHalf);
            const juce::Rectangle<float> headRect (cx - headW * 0.5f, cy - headH * 0.5f, headW, headH);
            g.setColour (noteCol);
            if (hollow)
            {
                g.setColour (Theme::canvas);
                g.fillEllipse (headRect);
                g.setColour (noteCol);
                g.drawEllipse (headRect, 1.8f);
            }
            else
            {
                g.fillEllipse (headRect);
            }

            // Augmentation dot
            if (kind == Notation::DurationKind::dottedHalf || kind == Notation::DurationKind::dottedQuarter)
                g.fillEllipse (cx + headW * 0.72f, cy - 1.8f, 3.4f, 3.4f);

            // Stem + flags (all except whole notes)
            if (kind != Notation::DurationKind::whole)
            {
                const bool treble = sp.diatonic >= 28;
                const bool stemUp = treble ? (sp.diatonic < 34) : (sp.diatonic < 22);
                const float stemX = stemUp ? (cx + headW * 0.45f) : (cx - headW * 0.45f);
                const float stemY2 = stemUp ? (cy - stemH) : (cy + stemH);
                g.drawLine (stemX, cy, stemX, stemY2, 1.5f);

                const int numFlags = (kind == Notation::DurationKind::sixteenth) ? 2
                                   : (kind == Notation::DurationKind::eighth)    ? 1 : 0;
                for (int fl = 0; fl < numFlags; ++fl)
                {
                    const float fy = stemUp ? (stemY2 + (float) fl * 4.5f) : (stemY2 - (float) fl * 4.5f);
                    const float dir = stemUp ? 1.0f : -1.0f;
                    juce::Path flag;
                    flag.startNewSubPath (stemX, fy);
                    flag.quadraticTo (stemX + 6.0f, fy + dir * 3.5f, stemX + 4.5f, fy + dir * 9.0f);
                    g.strokePath (flag, juce::PathStrokeType (1.5f));
                }
            }
        }

        // Box selection rectangle
        if (drag == Drag::box)
        {
            g.setColour (Theme::accent.withAlpha (0.18f));
            g.fillRect (box);
            g.setColour (Theme::accent);
            g.drawRect (box, 1.0f);
        }

        // Wash out offset & post-loop regions
        g.setColour (Theme::overlay.withAlpha (0.25f));
        if (c.offset > 0)
            g.fillRect (juce::Rectangle<float> (xOf (0), (float) area.getY(), xOf (c.offset) - xOf (0), (float) area.getHeight()));
        if (period < whole)
            g.fillRect (juce::Rectangle<float> (xOf (period), (float) area.getY(), xOf (whole) - xOf (period), (float) area.getHeight()));
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

    paintBars (g, area);

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
        const auto chance = 0.35f + 0.65f * juce::jlimit (0.0f, 1.0f, n.probability);   // a step that may not play is fainter
        g.setColour ((playing ? Theme::accent : Theme::dimText).withAlpha ((0.25f + 0.35f * v) * chance));
        g.fillRoundedRectangle (cell, 2.0f);
        g.setColour ((playing ? Theme::accent : Theme::dimText).withMultipliedAlpha (chance));
        g.fillRect (cell.withTrimmedTop (cell.getHeight() * (1.0f - v)).reduced (2.0f, 0.0f).withTrimmedBottom (1.0f));

        if (n.probability < 1.0f && cell.getWidth() >= 24.0f)
        {
            g.setColour (Theme::textStrong);
            g.setFont (10.0f);
            g.drawText (juce::String (juce::roundToInt (n.probability * 100.0f)) + "%", cell.toNearestInt(), juce::Justification::centredTop, false);
        }

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
    if (scoreOnly)
    {
        exportMusicXmlButton.setBounds (bar.removeFromLeft (86));
        bar.removeFromLeft (4);
    }
    else
    {
        drumsButton.setBounds (bar.removeFromLeft (56));
        bar.removeFromLeft (4);
        scoreButton.setBounds (bar.removeFromLeft (52));
        bar.removeFromLeft (4);
    }
    toolsButton.setBounds (bar.removeFromLeft (52));
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

    // Shown before it had room for a row (a window opening with a file): put its notes in the middle now.
    if (placedRows <= 1 && roll.getHeight() / rowPixels > 1)
        scrollToNotes();
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
    if (const auto roll = rollArea(); ! showingScore() && juce::Rectangle<int> (0, roll.getY(), keyboardWidth, roll.getHeight()).contains (e.getPosition()))
    {
        drag = Drag::keys;
        sound (pitchAt (e.position.y), getVelocity());
        return;
    }

    if (! rollArea().contains (e.getPosition()))
        return;

    const auto p = e.position;
    const auto past = noteAt (p.translated (-3.0f, 0.0f));
    const auto hit = noteAt (p) >= 0 ? noteAt (p) : past >= 0 && onNoteEnd (past, p) ? past : -1;
    base = shown;
    dragStart = p;

    if (hit >= 0)
    {
        if (e.mods.isPopupMenu())
        {
            if (selected.count (hit) == 0)
                selected = { hit };

            drag = Drag::nothing;
            refreshControls();
            repaint();
            noteMenu();
            return;
        }

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
        drag = onNoteEnd (hit, p) ? Drag::resize : Drag::move;
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

    // Empty grid: Shift-drag, or a drag, a box of notes; Cmd a note at once, dragging on for its
    // length; a click a note.
    if (e.mods.isShiftDown())
    {
        drag = Drag::box;
        box = { p, p };
        return;
    }

    if (e.mods.isCommandDown())
    {
        addNoteAt (p);
        return;
    }

    drag = Drag::pending;
}

void NoteEditor::addNoteAt (juce::Point<float> p)
{
    const auto step = gridStepFrames();
    ClipNote n;
    n.pitch = scaleLock ? getKey().snap (pitchAt (p.y)) : pitchAt (p.y);
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

bool NoteEditor::onNoteEnd (int note, juce::Point<float> p) const
{
    if (! shown.has_value() || ! juce::isPositiveAndBelow (note, (int) shown->clip.notes.size()))
        return false;

    // The last third of a short note, the last 10 pixels of a long one, and 3 pixels past its end.
    const auto r = noteBounds (shown->clip.notes[(size_t) note]);
    return p.y >= r.getY() && p.y < r.getBottom() && p.x >= r.getRight() - juce::jlimit (3.0f, 10.0f, r.getWidth() / 3.0f) && p.x <= r.getRight() + 3.0f;
}

void NoteEditor::mouseMove (const juce::MouseEvent& e)
{
    auto cursor = juce::MouseCursor::NormalCursor;

    if (shown.has_value() && ! showingDrums() && rollArea().contains (e.getPosition()))
    {
        const auto hit = noteAt (e.position);
        const auto near = hit >= 0 ? hit : noteAt (e.position.translated (-3.0f, 0.0f));

        if (near >= 0 && onNoteEnd (near, e.position))
            cursor = juce::MouseCursor::LeftRightResizeCursor;
    }

    setMouseCursor (cursor);
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

    if (row >= 0 && e.mods.isPopupMenu())
    {
        drag = Drag::nothing;

        if (const auto set = noteInCell (row, step); set >= 0)
        {
            if (selected.count (set) == 0)
                selected = { set };

            repaint();
            noteMenu();
        }

        return;
    }

    if (row < 0)
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

    // Dragged from empty grid: a box of notes.
    if (drag == Drag::pending)
    {
        if (p.getDistanceFrom (dragStart) < 4.0f)
            return;

        drag = Drag::box;
    }

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
        const auto limit = noteEndLimit();
        const auto end = juce::jmax (was.start + 1, juce::jmin (limit, snapped (frameAt (p.x) + (gridStepFrames() > 0 ? gridStepFrames() : 0))));
        const auto delta = (end - was.start) - was.length;

        for (auto i : selected)
            if (juce::isPositiveAndBelow (i, (int) shown->clip.notes.size()))
            {
                auto& n = shown->clip.notes[(size_t) i];
                n.length = juce::jlimit ((juce::int64) 1, juce::jmax ((juce::int64) 1, limit - n.start), n.length + delta);
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

                if (scaleLock && semitones != 0)
                    n.pitch = getKey().snap (n.pitch);

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

    // A click on empty grid: a note there.
    if (was == Drag::pending && shown.has_value())
    {
        addNoteAt (dragStart);
        drag = Drag::none;
        silence();
        base.reset();
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

    // A trackpad's small steps add up to whole rows.
    rowScroll += wheel.deltaY * 12.0f;
    const auto whole = (int) rowScroll;
    rowScroll -= (float) whole;
    const auto rows = juce::jmax (1, rollArea().getHeight() / rowPixels);
    lowPitch = juce::jlimit (0, juce::jmax (0, 128 - rows), lowPitch + whole);
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

    // Option with left and right: the chosen notes shorter or longer by a grid step (a quarter with Shift).
    if (key.getModifiers().isAltDown() && (key.getKeyCode() == juce::KeyPress::leftKey || key.getKeyCode() == juce::KeyPress::rightKey))
    {
        const auto by = shift ? juce::jmax ((juce::int64) 1, step / 4) : step;
        resizeSelected (key.getKeyCode() == juce::KeyPress::rightKey ? by : -by);
        return true;
    }

    if (key.getKeyCode() == juce::KeyPress::upKey)         moveSelected (shift ? 12 : 1, 0);
    else if (key.getKeyCode() == juce::KeyPress::downKey)  moveSelected (shift ? -12 : -1, 0);
    else if (key.getKeyCode() == juce::KeyPress::leftKey)  moveSelected (0, shift ? -juce::jmax ((juce::int64) 1, step / 4) : -step);
    else if (key.getKeyCode() == juce::KeyPress::rightKey) moveSelected (0, shift ? juce::jmax ((juce::int64) 1, step / 4) : step);
    else
        return false;

    return true;
}

void NoteEditor::resizeSelected (juce::int64 frames)
{
    if (! shown.has_value() || selected.empty())
        return;

    const auto limit = noteEndLimit();

    for (auto i : selected)
        if (juce::isPositiveAndBelow (i, (int) shown->clip.notes.size()))
        {
            auto& n = shown->clip.notes[(size_t) i];
            n.length = juce::jlimit ((juce::int64) 1, juce::jmax ((juce::int64) 1, limit - n.start), n.length + frames);
        }

    commit ("Note Length");
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

            // Locked to the key, a semitone up or down is a step of the scale.
            if (scaleLock && std::abs (semitones) == 1)
                n.pitch = getKey().step (n.pitch, semitones);
            else
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
    // Notes landing on one another (moved, snapped to a key, pasted) are one note: the selection
    // is kept by the notes' places, not their indices.
    if (shown.has_value())
    {
        std::set<std::pair<int, juce::int64>> chosen;

        for (auto i : selected)
            if (juce::isPositiveAndBelow (i, (int) shown->clip.notes.size()))
                chosen.insert ({ shown->clip.notes[(size_t) i].pitch, shown->clip.notes[(size_t) i].start });

        if (shown->clip.mergeDuplicateNotes() > 0)
        {
            selected.clear();

            for (int i = 0; i < (int) shown->clip.notes.size(); ++i)
                if (chosen.count ({ shown->clip.notes[(size_t) i].pitch, shown->clip.notes[(size_t) i].start }) > 0)
                    selected.insert (i);
        }
    }

    if (shown.has_value() && apply != nullptr)
        apply (shown->clip, description);

    refreshControls();
    repaint();
}

//==============================================================================
std::set<int> NoteEditor::chosenNotes() const
{
    if (! shown.has_value())
        return {};

    if (! selected.empty())
        return selected;

    std::set<int> all;

    for (int i = 0; i < (int) shown->clip.notes.size(); ++i)
        all.insert (i);

    return all;
}

juce::int64 NoteEditor::beatFrames() const noexcept
{
    return shown.has_value() ? juce::jmax ((juce::int64) 1, (juce::int64) std::llround (shown->beatSeconds * shown->sampleRate)) : 1;
}

NoteTools::Scale NoteEditor::getKey() const
{
    if (chosenKey.has_value())
        return *chosenKey;

    if (shown.has_value())
    {
        if (const auto k = NoteTools::parseKey (shown->key))
            return *k;

        return NoteTools::guessKey (shown->clip.notes);
    }

    return {};
}

void NoteEditor::setKey (const NoteTools::Scale& key)
{
    chosenKey = key;

    if (onKeyChosen != nullptr)
        onKeyChosen (key.name());

    repaint();
}

void NoteEditor::setScaleLock (bool on)
{
    scaleLock = on;
    repaint();
}

juce::String NoteEditor::writeChords (const juce::String& progression, double beatsEach, int octave, bool replace)
{
    if (! shown.has_value())
        return "There is no clip.";

    juce::String error;
    const auto chords = NoteTools::parseProgression (progression, getKey(), octave, error);

    if (chords.empty())
        return error;

    auto& c = shown->clip;
    const auto each = juce::jmax ((juce::int64) 1, (juce::int64) std::llround (beatsEach * (double) beatFrames()));
    auto notes = NoteTools::chordNotes (chords, c.offset, each, getVelocity());

    // Nothing past the timeline: the last chords are cut there, or left out.
    notes.erase (std::remove_if (notes.begin(), notes.end(), [&] (const ClipNote& n) { return n.start >= wholeLength(); }), notes.end());

    for (auto& n : notes)
        n.length = juce::jmin (n.length, wholeLength() - n.start);

    if (replace)
        c.notes.clear();

    selected.clear();

    for (auto& n : notes)
    {
        c.notes.push_back (n);
        selected.insert ((int) c.notes.size() - 1);
    }

    commit ("Chords");
    return {};
}

void NoteEditor::arpeggiate (const NoteTools::Arp& arp)
{
    if (! shown.has_value() || shown->clip.notes.empty())
        return;

    const auto chosen = chosenNotes();
    std::vector<ClipNote> chords, others;

    for (int i = 0; i < (int) shown->clip.notes.size(); ++i)
        (chosen.count (i) > 0 ? chords : others).push_back (shown->clip.notes[(size_t) i]);

    const auto arpeggio = NoteTools::arpeggiate (chords, arp);
    shown->clip.notes = others;
    selected.clear();

    for (auto& n : arpeggio)
    {
        shown->clip.notes.push_back (n);
        selected.insert ((int) shown->clip.notes.size() - 1);
    }

    commit ("Arpeggiate");
}

void NoteEditor::humanize (const NoteTools::Humanize& h)
{
    if (! shown.has_value())
        return;

    NoteTools::humanize (shown->clip.notes, selected, h, wholeLength());
    commit ("Humanize");
}

void NoteEditor::fitToScale()
{
    if (! shown.has_value())
        return;

    NoteTools::fitToScale (shown->clip.notes, selected, getKey());
    commit ("Fit to Scale");
}

void NoteEditor::setProbability (float probability)
{
    if (! shown.has_value() || selected.empty())
        return;

    for (auto i : selected)
        if (juce::isPositiveAndBelow (i, (int) shown->clip.notes.size()))
            shown->clip.notes[(size_t) i].probability = juce::jlimit (0.0f, 1.0f, probability);

    commit ("Note Probability");
}

void NoteEditor::setRatchets (int count)
{
    if (! shown.has_value() || selected.empty())
        return;

    for (auto i : selected)
        if (juce::isPositiveAndBelow (i, (int) shown->clip.notes.size()))
            shown->clip.notes[(size_t) i].ratchets = juce::jlimit (1, 8, count);

    commit ("Note Ratchets");
}

void NoteEditor::setMicroTiming (int offsetFrames)
{
    if (! shown.has_value() || selected.empty())
        return;

    for (auto i : selected)
        if (juce::isPositiveAndBelow (i, (int) shown->clip.notes.size()))
            shown->clip.notes[(size_t) i].microTiming = offsetFrames;

    commit ("Note Micro-Timing");
}

void NoteEditor::applyGroove (const NoteTools::GrooveTemplate& groove, float strength)
{
    if (! shown.has_value() || shown->clip.notes.empty())
        return;

    NoteTools::applyGroove (shown->clip.notes, selected, groove, (double) beatFrames(), strength, wholeLength());
    commit ("Apply Groove (" + groove.name + ")");
}

void NoteEditor::noteMenu()
{
    if (! shown.has_value() || selected.empty())
        return;

    const auto first = shown->clip.notes[(size_t) *selected.begin()];
    juce::PopupMenu chance, velocity, ratchets, micro;
    const int percents[] = { 100, 90, 75, 50, 33, 25, 10 };

    for (auto p : percents)
        chance.addItem (1000 + p, juce::String (p) + " %", true, juce::roundToInt (first.probability * 100.0f) == p);

    for (auto v : { 127, 110, 96, 80, 64, 48, 32 })
        velocity.addItem (2000 + v, juce::String (v), true, juce::roundToInt (first.velocity * 127.0f) == v);

    for (int r : { 1, 2, 3, 4, 6, 8 })
        ratchets.addItem (3000 + r, r == 1 ? "Off (1x)" : juce::String (r) + "x Ratchet", true, first.ratchets == r);

    const auto msToFrames = [sr = shown->sampleRate] (int ms) { return (int) std::lround (ms * 0.001 * sr); };
    const int offsetsMs[] = { -20, -10, -5, 0, 5, 10, 20 };
    for (int idx = 0; idx < (int) std::size (offsetsMs); ++idx)
    {
        const auto ms = offsetsMs[idx];
        const auto label = ms == 0 ? juce::String ("On Grid (0 ms)") : (ms > 0 ? ("+" + juce::String (ms) + " ms (late)") : (juce::String (ms) + " ms (early)"));
        micro.addItem (4000 + idx, label, true, std::abs (first.microTiming - msToFrames (ms)) <= 2);
    }

    juce::PopupMenu menu;
    menu.addSectionHeader (selected.size() == 1 ? juce::String ("Note") : juce::String ((int) selected.size()) + " notes");
    menu.addSubMenu ("Probability", chance);
    menu.addSubMenu ("Ratchets / Flams", ratchets);
    menu.addSubMenu ("Micro-Timing", micro);
    menu.addSubMenu ("Velocity", velocity);
    menu.addItem (3, "Delete");

    menu.showMenuAsync (juce::PopupMenu::Options().withMousePosition(), [safe = juce::Component::SafePointer<NoteEditor> (this), msToFrames] (int result)
    {
        if (safe == nullptr || result == 0)
            return;

        static constexpr int microMs[] = { -20, -10, -5, 0, 5, 10, 20 };
        if (result == 3)
            safe->removeSelected();
        else if (result >= 4000 && result < 4007)
            safe->setMicroTiming (msToFrames (microMs[result - 4000]));
        else if (result >= 3000)
            safe->setRatchets (result - 3000);
        else if (result >= 2000)
            safe->setVelocity ((float) (result - 2000) / 127.0f);   // an undo step of its own
        else if (result >= 1000)
            safe->setProbability ((float) (result - 1000) / 100.0f);
    });
}

void NoteEditor::showToolsMenu()
{
    if (! shown.has_value())
        return;

    const auto some = selected.empty() ? juce::String ("all the notes") : juce::String ("the selected notes");
    juce::PopupMenu menu, grooves;
    const auto presets = NoteTools::groovePresets();
    for (int g = 0; g < (int) presets.size(); ++g)
        grooves.addItem (100 + g, presets[(size_t) g].name);

    menu.addSectionHeader ("Key: " + getKey().name() + (chosenKey.has_value() || NoteTools::parseKey (shown->key).has_value() ? juce::String() : juce::String (" (guessed)")));
    menu.addItem (1, "Chords...");
    menu.addItem (2, "Arpeggiate " + some + "...", ! shown->clip.notes.empty());
    menu.addItem (3, "Humanize " + some + "...", ! shown->clip.notes.empty());
    menu.addSubMenu ("Apply Groove to " + some, grooves, ! shown->clip.notes.empty());
    menu.addItem (4, "Fit " + some + " to the Key", ! shown->clip.notes.empty());
    menu.addSeparator();
    menu.addItem (5, "Scale Lock", true, scaleLock);
    menu.addItem (7, "Show Ghost Notes", true, showGhostNotes);
    menu.addItem (6, "Key...");
    menu.addSeparator();
    menu.addItem (8, "Export MusicXML (.musicxml)...", ! shown->clip.notes.empty());

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (toolsButton), [safe = juce::Component::SafePointer<NoteEditor> (this), presets] (int result)
    {
        if (safe == nullptr)
            return;

        if (result >= 100 && result < 100 + (int) presets.size())
        {
            safe->applyGroove (presets[(size_t) (result - 100)], 1.0f);
            return;
        }

        switch (result)
        {
            case 1: safe->askForChords(); break;
            case 2: safe->askToArpeggiate(); break;
            case 3: safe->askToHumanize(); break;
            case 4: safe->fitToScale(); break;
            case 5: safe->setScaleLock (! safe->scaleLock); break;
            case 6: safe->askForKey(); break;
            case 7: safe->setShowGhostNotes (! safe->showGhostNotes); break;
            case 8: safe->exportMusicXmlDialog(); break;
            default: break;
        }
    });
}

void NoteEditor::askForKey()
{
    const auto key = getKey();
    const auto modes = NoteTools::modes();
    const juce::StringArray roots { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    auto* dialog = new juce::AlertWindow ("Key", "The key the writing tools and Scale Lock use.", juce::MessageBoxIconType::NoIcon, this);
    dialog->addComboBox ("root", roots, "Root");
    dialog->getComboBoxComponent ("root")->setSelectedItemIndex (key.root);
    dialog->addComboBox ("mode", modes, "Scale");
    dialog->getComboBoxComponent ("mode")->setSelectedItemIndex (juce::jmax (0, modes.indexOf (key.mode)));
    dialog->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    dialog->enterModalState (true, juce::ModalCallbackFunction::create ([safe = juce::Component::SafePointer<NoteEditor> (this), dialog, roots, modes] (int result)
    {
        if (result != 1 || safe == nullptr)
            return;

        if (const auto k = NoteTools::parseKey (roots[dialog->getComboBoxComponent ("root")->getSelectedItemIndex()] + " " + modes[dialog->getComboBoxComponent ("mode")->getSelectedItemIndex()]))
            safe->setKey (*k);
    }), true);
}

void NoteEditor::askForChords()
{
    const auto key = getKey();
    const auto minor = key.steps.size() > 2 && key.steps[2] == 3;
    const juce::StringArray lengths { "1 beat", "2 beats", "A bar", "2 bars" };
    const juce::StringArray octaves { "2", "3", "4", "5" };
    auto* dialog = new juce::AlertWindow ("Chords",
                                       "Roman numerals in " + key.name() + " (Key... in the Tools menu changes it): upper case major, lower case minor, "
                                       + juce::String::fromUTF8 ("with 7, maj7, dim or \xc2\xb0, \xc3\xb8 or m7b5, aug or +, sus2, sus4, and b or # before for a borrowed root (bVII). ")
                                       + "They are written from the clip's start, one after another.",
                                       juce::MessageBoxIconType::NoIcon, this);
    dialog->addTextEditor ("progression", minor ? "i VI III VII" : "I V vi IV", "Chords");
    dialog->addComboBox ("each", lengths, "Each chord");
    dialog->getComboBoxComponent ("each")->setSelectedItemIndex (2);
    dialog->addComboBox ("octave", octaves, "Root octave");
    dialog->getComboBoxComponent ("octave")->setSelectedItemIndex (1);
    dialog->addComboBox ("replace", { "In place of the notes", "Added to the notes" }, "Notes");
    dialog->getComboBoxComponent ("replace")->setSelectedItemIndex (shown.has_value() && ! shown->clip.notes.empty() ? 1 : 0);
    dialog->addButton ("Write", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    dialog->enterModalState (true, juce::ModalCallbackFunction::create ([safe = juce::Component::SafePointer<NoteEditor> (this), dialog, octaves] (int result)
    {
        if (result != 1 || safe == nullptr || ! safe->shown.has_value())
            return;

        const auto bar = (double) safe->shown->beatsPerBar;
        const double beats[] = { 1.0, 2.0, bar, 2.0 * bar };
        const auto each = beats[juce::jlimit (0, 3, dialog->getComboBoxComponent ("each")->getSelectedItemIndex())];
        const auto octave = octaves[dialog->getComboBoxComponent ("octave")->getSelectedItemIndex()].getIntValue();
        const auto error = safe->writeChords (dialog->getTextEditorContents ("progression"), each, octave, dialog->getComboBoxComponent ("replace")->getSelectedItemIndex() == 0);

        if (error.isNotEmpty())
            juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::WarningIcon).withTitle ("Chords").withMessage (error).withButton ("OK"), nullptr);
    }), true);
}

void NoteEditor::askToArpeggiate()
{
    const juce::StringArray rates { "1/2 beat", "1/3 beat", "1/4 beat", "1/6 beat", "1/8 beat" };
    const juce::StringArray gates { "Short (50 %)", "75 %", "90 %", "Legato (100 %)" };
    auto* dialog = new juce::AlertWindow ("Arpeggiate",
                                       "Plays each chord of " + juce::String (selected.empty() ? "the notes" : "the selected notes")
                                       + " (notes starting together) as an arpeggio over its length, in place of it.",
                                       juce::MessageBoxIconType::NoIcon, this);
    dialog->addComboBox ("order", NoteTools::arpOrderNames(), "Order");
    dialog->getComboBoxComponent ("order")->setSelectedItemIndex (0);
    dialog->addComboBox ("octaves", { "1 octave", "2 octaves", "3 octaves", "4 octaves" }, "Range");
    dialog->getComboBoxComponent ("octaves")->setSelectedItemIndex (0);
    dialog->addComboBox ("rate", rates, "Rate");
    dialog->getComboBoxComponent ("rate")->setSelectedItemIndex (2);
    dialog->addComboBox ("gate", gates, "Note length");
    dialog->getComboBoxComponent ("gate")->setSelectedItemIndex (2);
    dialog->addButton ("Arpeggiate", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    dialog->enterModalState (true, juce::ModalCallbackFunction::create ([safe = juce::Component::SafePointer<NoteEditor> (this), dialog] (int result)
    {
        if (result != 1 || safe == nullptr)
            return;

        const int perBeat[] = { 2, 3, 4, 6, 8 };
        const double gateShares[] = { 0.5, 0.75, 0.9, 1.0 };
        NoteTools::Arp arp;
        arp.order = (NoteTools::ArpOrder) juce::jlimit (0, 3, dialog->getComboBoxComponent ("order")->getSelectedItemIndex());
        arp.octaves = dialog->getComboBoxComponent ("octaves")->getSelectedItemIndex() + 1;
        arp.step = juce::jmax ((juce::int64) 1, safe->beatFrames() / perBeat[juce::jlimit (0, 4, dialog->getComboBoxComponent ("rate")->getSelectedItemIndex())]);
        arp.gate = gateShares[juce::jlimit (0, 3, dialog->getComboBoxComponent ("gate")->getSelectedItemIndex())];
        arp.seed = (juce::uint32) juce::Random::getSystemRandom().nextInt();
        safe->arpeggiate (arp);
    }), true);
}

void NoteEditor::askToHumanize()
{
    const juce::StringArray timings { "None", "5 ms", "10 ms", "20 ms", "40 ms" };
    const juce::StringArray velocities { "None", "5 %", "10 %", "20 %", "30 %" };
    auto* dialog = new juce::AlertWindow ("Humanize", "Moves each of " + juce::String (selected.empty() ? "the notes" : "the selected notes")
                                       + " and changes its velocity by a random amount, up to these.",
                                       juce::MessageBoxIconType::NoIcon, this);
    dialog->addComboBox ("timing", timings, "Timing");
    dialog->getComboBoxComponent ("timing")->setSelectedItemIndex (2);
    dialog->addComboBox ("velocity", velocities, "Velocity");
    dialog->getComboBoxComponent ("velocity")->setSelectedItemIndex (2);
    dialog->addButton ("Humanize", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    dialog->enterModalState (true, juce::ModalCallbackFunction::create ([safe = juce::Component::SafePointer<NoteEditor> (this), dialog] (int result)
    {
        if (result != 1 || safe == nullptr || ! safe->shown.has_value())
            return;

        const double ms[] = { 0.0, 5.0, 10.0, 20.0, 40.0 };
        const float shares[] = { 0.0f, 0.05f, 0.1f, 0.2f, 0.3f };
        NoteTools::Humanize h;
        h.timing = (juce::int64) std::llround (ms[juce::jlimit (0, 4, dialog->getComboBoxComponent ("timing")->getSelectedItemIndex())] * 0.001 * safe->shown->sampleRate);
        h.velocity = shares[juce::jlimit (0, 4, dialog->getComboBoxComponent ("velocity")->getSelectedItemIndex())];
        h.seed = (juce::uint32) juce::Random::getSystemRandom().nextInt();
        safe->humanize (h);
    }), true);
}
