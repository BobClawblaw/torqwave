#include "IdeaBoardView.h"
#include "AudioEngine.h"
#include "SequenceLibrary.h"
#include "SequenceLibraryView.h"
#include "Theme.h"
#include "TrackTemplate.h"

IdeaBoardView::IdeaBoardView (AudioEngine& e) : engine (e)
{
    player.setFreeRunning (true);
    player.getEffects().prepare (rate());
    devicePlayer.setSource (&player);
    engine.deviceManager.addAudioCallback (&devicePlayer);
    base.sampleRate = rate();

    for (auto* b : { &newButton, &openButton, &addPartButton, &addSceneButton, &stopButton, &sendButton })
        addAndMakeVisible (b);

    addAndMakeVisible (sessionTempoButton);
    addAndMakeVisible (tempoSlider);
    addAndMakeVisible (title);
    addAndMakeVisible (where);
    title.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    where.setJustificationType (juce::Justification::centredRight);

    newButton.setTooltip ("A new, empty board in the Boards folder");
    openButton.setTooltip ("Opens another board");
    addPartButton.setTooltip ("Adds a part: a row for one instrument's sequences");
    addSceneButton.setTooltip ("Adds a scene: a column of sequences that play together");
    stopButton.setTooltip ("Stops everything at once");
    sendButton.setTooltip ("Lays the scenes out in the session from the cursor's bar: a section for each, a track for each part, the sequences as clips");
    sessionTempoButton.setTooltip ("Plays at the session's tempo where its playhead is, instead of the board's own");
    tempoSlider.setTooltip ("The board's own tempo (set it while the board is stopped)");

    tempoSlider.setRange (20.0, 400.0, 0.1);
    tempoSlider.setTextValueSuffix (" BPM");
    tempoSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 80, 24);
    tempoSlider.onValueChange = [this]
    {
        board.tempo = tempoSlider.getValue();
        changed();
    };
    sessionTempoButton.onClick = [this]
    {
        board.sessionTempo = sessionTempoButton.getToggleState();
        changed();
    };

    newButton.onClick = [this] { askForNewBoard(); };
    openButton.onClick = [this] { showOpenChooser(); };
    addPartButton.onClick = [this]
    {
        if (board.addPart ("Part " + juce::String (board.parts.size() + 1)))
            changed();
    };
    addSceneButton.onClick = [this]
    {
        if (board.addScene ("Scene " + juce::String (board.scenes.size() + 1)))
            changed();
    };
    stopButton.onClick = [this] { stopAll(); };
    sendButton.onClick = [this]
    {
        if (onSendToSession != nullptr)
            onSendToSession (board);
    };

    openLatest();
    startTimerHz (15);
}

IdeaBoardView::~IdeaBoardView()
{
    stopTimer();

    for (auto& [id, alive] : loading)
        *alive = false;

    engine.deviceManager.removeAudioCallback (&devicePlayer);
    devicePlayer.setSource (nullptr);

    if (onPluginRemoved != nullptr)
        for (auto* p : player.getEffects().getAllPlugins())
            onPluginRemoved (p);
}

double IdeaBoardView::rate() const
{
    const auto r = engine.getOutputSampleRate();
    return r > 0.0 ? r : engine.session.getSampleRate();
}

//==============================================================================
void IdeaBoardView::openLatest()
{
    juce::File latest;

    for (auto& f : IdeaBoard::defaultFolder().findChildFiles (juce::File::findFiles, false, juce::String ("*") + IdeaBoard::extension))
        if (latest == juce::File() || f.getLastModificationTime() > latest.getLastModificationTime())
            latest = f;

    juce::String error;

    if (latest != juce::File())
        if (const auto b = IdeaBoard::load (latest, SequenceLibrary{}.getFolder(), error))
        {
            openBoard (*b, latest);
            return;
        }

    // A first board, written once something is put on it.
    openBoard (IdeaBoard(), IdeaBoard::defaultFolder().getChildFile (juce::String ("Board") + IdeaBoard::extension));
}

void IdeaBoardView::openBoard (const IdeaBoard& b, const juce::File& f)
{
    stopAll();

    for (auto it = trackOf.begin(); it != trackOf.end();)
    {
        const auto cell = it->first;
        ++it;
        removeTrack (cell);
    }

    board = b;
    file = f;
    tempoSlider.setValue (board.tempo, juce::dontSendNotification);
    sessionTempoButton.setToggleState (board.sessionTempo, juce::dontSendNotification);
    syncTracks();
    refreshPlaying();
    repaint();
}

void IdeaBoardView::changed()
{
    if (file != juce::File())
        board.save (file, SequenceLibrary{}.getFolder());

    syncTracks();
    refreshPlaying();
    repaint();
}

void IdeaBoardView::removeTrack (IdeaBoard::Cell cell)
{
    const auto it = trackOf.find (cell);

    if (it == trackOf.end())
        return;

    const auto id = it->second;

    if (const auto alive = loading.find (id); alive != loading.end())
    {
        *alive->second = false;
        loading.erase (alive);
    }

    auto& effects = player.getEffects();

    while (effects.getNumSlots (id) > 0)
    {
        if (onPluginRemoved != nullptr)
            onPluginRemoved (effects.getPlugin (id, effects.getNumSlots (id) - 1));

        auto removed = effects.remove (id, effects.getNumSlots (id) - 1);
    }

    base.tracks.erase (std::remove_if (base.tracks.begin(), base.tracks.end(), [id] (const SessionTrack& t) { return t.id == id; }), base.tracks.end());
    trackOf.erase (it);
    sequences.erase (cell);
    loadedFrom.erase (cell);   // a launch of the cell stays: a new sequence in it plays on its new track
}

void IdeaBoardView::syncTracks()
{
    // The tempo: the session's where its playhead is, or the board's own; kept while playing.
    if (! player.isPlaying())
    {
        const auto session = engine.session.getState();   // held: the map refers to its tempo changes
        const auto map = session->tempoMap();
        const auto frame = engine.getPlayer().getPositionFrames();
        base.tempo = board.sessionTempo ? map.tempoAt (frame) : board.tempo;
        base.beatsPerBar = board.sessionTempo ? map.beatsPerBarAt (frame) : 4;
    }

    tempoSlider.setEnabled (! board.sessionTempo && ! player.isPlaying());

    // Cells gone or changed lose their tracks; new ones get theirs, with their voices loading.
    for (auto it = trackOf.begin(); it != trackOf.end();)
    {
        const auto cell = it->first;
        ++it;
        const auto f = board.fileAt (cell.first, cell.second);

        if (f == juce::File() || loadedFrom[cell] != f.getFullPathName() + "|" + juce::String (f.getLastModificationTime().toMilliseconds()))
            removeTrack (cell);
    }

    for (auto& [cell, f] : board.cells)
    {
        if (trackOf.count (cell) > 0)
            continue;

        juce::String error;
        auto q = SequenceFile::load (f, error);

        if (! q.has_value())
            continue;

        juce::uint32 id = 0;
        std::shared_ptr<juce::XmlElement> effects;

        if (q->voice != nullptr)
        {
            const auto added = TrackTemplate::add (base, *q->voice);
            id = added.trackId;
            effects = added.effects;
        }

        if (id == 0)
            id = base.addInstrumentTrack().id;

        trackOf[cell] = id;
        sequences[cell] = *q;
        loadedFrom[cell] = f.getFullPathName() + "|" + juce::String (f.getLastModificationTime().toMilliseconds());

        if (effects != nullptr)
        {
            auto alive = std::make_shared<bool> (true);
            loading[id] = alive;
            engine.loadEffectsInto (player.getEffects(), rate(), { { id, effects } },
                                    [safe = juce::WeakReference<IdeaBoardView> (this), id] (const juce::String&)
                                    {
                                        if (auto* self = safe.get())
                                            self->loading.erase (id);
                                    },
                                    alive);
        }
    }
}

void IdeaBoardView::refreshPlaying()
{
    player.setState (std::make_shared<const SessionState> (launcher.stateFor (base, trackOf, sequences)), 0);
}

//==============================================================================
void IdeaBoardView::setCell (int part, int scene, const juce::File& sequence)
{
    board.set (part, scene, sequence);
    changed();
}

void IdeaBoardView::clickCell (int part, int scene)
{
    if (board.fileAt (part, scene) == juce::File())
    {
        chooseSequence ({ part, scene });
        return;
    }

    const auto map = base.tempoMap();
    const auto position = player.getPosition();
    const auto state = launcher.stateOf ({ part, scene }, position);

    if (player.isPlaying() && (state == IdeaBoardLauncher::State::playing || state == IdeaBoardLauncher::State::queued))
    {
        launcher.stopPart (part, map, position);
        refreshPlaying();
        return;
    }

    if (! player.isPlaying())
        syncTracks();   // the tempo as it is now

    const auto starts = launcher.launch ({ part, scene }, base.tempoMap(), position, player.isPlaying());
    refreshPlaying();

    if (starts)
        player.play();
}

void IdeaBoardView::clickScene (int scene)
{
    if (! player.isPlaying())
        syncTracks();

    const auto starts = launcher.launchScene (scene, board, base.tempoMap(), player.getPosition(), player.isPlaying());
    refreshPlaying();

    if (starts)
        player.play();
}

void IdeaBoardView::clickPart (int part)
{
    launcher.stopPart (part, base.tempoMap(), player.getPosition());
    refreshPlaying();
}

void IdeaBoardView::stopAll()
{
    player.stop();
    launcher.clear();
    refreshPlaying();
    syncTracks();
}

//==============================================================================
void IdeaBoardView::timerCallback()
{
    const auto position = player.getPosition();
    const auto before = launcher.getLaunched().size();
    launcher.tidy (position);

    if (launcher.getLaunched().size() != before)
        refreshPlaying();

    if (! player.isPlaying() && board.sessionTempo)
    {
        const auto old = base.tempo;
        syncTracks();

        if (! juce::exactlyEqual (old, base.tempo))
            refreshPlaying();
    }

    const auto shownTempo = juce::String (base.tempo, juce::exactlyEqual (base.tempo, std::round (base.tempo)) ? 0 : 1) + " BPM";
    title.setText (board.name, juce::dontSendNotification);
    where.setText (player.isPlaying() ? Metronome::positionText (base.tempoMap(), position) + "   " + shownTempo : "Stopped   " + shownTempo,
                   juce::dontSendNotification);
    tempoSlider.setEnabled (! board.sessionTempo && ! player.isPlaying());
    repaint (gridArea());
}

juce::Rectangle<int> IdeaBoardView::gridArea() const
{
    return getLocalBounds().reduced (4).withTrimmedTop (2 * rowHeight + 4);
}

juce::Rectangle<int> IdeaBoardView::cellBounds (int part, int scene) const
{
    const auto area = gridArea();
    const auto scenes = juce::jmax (1, board.scenes.size()), parts = juce::jmax (1, board.parts.size());
    const auto w = (area.getWidth() - partWidth) / scenes;
    const auto h = juce::jmin (64, (area.getHeight() - headerHeight) / parts);
    return { area.getX() + partWidth + scene * w, area.getY() + headerHeight + part * h, w, h };
}

juce::String IdeaBoardView::lengthText (double bars)
{
    const auto whole = juce::exactlyEqual (bars, std::round (bars));
    return juce::String (bars, whole ? 0 : 2) + (juce::exactlyEqual (bars, 1.0) ? " bar" : " bars");
}

IdeaBoard::Cell IdeaBoardView::cellAt (juce::Point<int> p) const
{
    for (int part = -1; part < board.parts.size(); ++part)
        for (int scene = -1; scene < board.scenes.size(); ++scene)
        {
            auto r = cellBounds (juce::jmax (0, part), juce::jmax (0, scene));

            if (part < 0)
                r = r.withY (gridArea().getY()).withHeight (headerHeight);

            if (scene < 0)
                r = r.withX (gridArea().getX()).withWidth (partWidth);

            if (r.contains (p))
                return { part, scene };
        }

    return { -2, -2 };
}

void IdeaBoardView::paint (juce::Graphics& g)
{
    g.fillAll (Theme::surface);
    const auto position = player.getPosition();

    for (int k = 0; k < board.scenes.size(); ++k)
    {
        const auto r = cellBounds (0, k).withY (gridArea().getY()).withHeight (headerHeight).reduced (2);
        g.setColour (Theme::raised);
        g.fillRoundedRectangle (r.toFloat(), 3.0f);
        g.setColour (Theme::text);
        g.setFont (12.0f);
        g.drawText (juce::String::fromUTF8 ("\xe2\x96\xb6 ") + board.scenes[k], r.reduced (6, 0), juce::Justification::centredLeft, true);
    }

    for (int p = 0; p < board.parts.size(); ++p)
    {
        const auto r = cellBounds (p, 0).withX (gridArea().getX()).withWidth (partWidth).reduced (2);
        g.setColour (Theme::raised);
        g.fillRoundedRectangle (r.toFloat(), 3.0f);
        g.setColour (Theme::text);
        g.setFont (12.0f);
        g.drawText (board.parts[p], r.reduced (6, 0), juce::Justification::centredLeft, true);

        for (int k = 0; k < board.scenes.size(); ++k)
        {
            const auto cell = cellBounds (p, k).reduced (2);
            const auto f = board.fileAt (p, k);
            const auto state = launcher.stateOf ({ p, k }, position);
            const auto playing = state == IdeaBoardLauncher::State::playing || state == IdeaBoardLauncher::State::stopping;

            g.setColour (f == juce::File() ? Theme::inset : playing ? Theme::accent.withAlpha (state == IdeaBoardLauncher::State::stopping ? 0.45f : 0.8f)
                                                                    : Theme::lane);
            g.fillRoundedRectangle (cell.toFloat(), 3.0f);

            if (state == IdeaBoardLauncher::State::queued || dropTarget == IdeaBoard::Cell { p, k })
            {
                g.setColour (Theme::accent);
                g.drawRoundedRectangle (cell.toFloat().reduced (1.0f), 3.0f, 2.0f);
            }

            g.setColour (f == juce::File() ? Theme::dimText : playing ? Theme::textStrong : Theme::text);
            g.setFont (12.0f);
            const auto bars = sequences.count ({ p, k }) > 0 ? sequences.at ({ p, k }).bars() : 0.0;
            const auto text = f == juce::File() ? juce::String ("Drop a sequence")
                                                : f.getFileNameWithoutExtension() + (bars > 0.0 ? "  " + lengthText (bars) : juce::String());
            g.drawFittedText (text + (state == IdeaBoardLauncher::State::queued ? "\n(next bar)" : juce::String()), cell.reduced (6, 2), juce::Justification::centredLeft, 2);
        }
    }
}

void IdeaBoardView::resized()
{
    auto area = getLocalBounds().reduced (4);
    const auto place = [] (juce::Rectangle<int>& row, juce::Component& c, int width) { c.setBounds (row.removeFromLeft (width).reduced (2)); };

    auto top = area.removeFromTop (rowHeight);
    place (top, newButton, 60);
    place (top, openButton, 70);
    top.removeFromLeft (8);
    place (top, addPartButton, 66);
    place (top, addSceneButton, 72);
    top.removeFromLeft (8);
    place (top, sendButton, 124);
    title.setBounds (top);

    auto transport = area.removeFromTop (rowHeight);
    place (transport, stopButton, 76);
    place (transport, tempoSlider, 170);
    place (transport, sessionTempoButton, 124);
    where.setBounds (transport);
}

//==============================================================================
void IdeaBoardView::mouseDown (const juce::MouseEvent& e)
{
    const auto [part, scene] = cellAt (e.getPosition());

    if (part < -1)
        return;

    if (part == -1 && scene == -1)
        return;

    if (part == -1)
    {
        e.mods.isPopupMenu() ? headerMenu (true, scene) : clickScene (scene);
        return;
    }

    if (scene == -1)
    {
        e.mods.isPopupMenu() ? headerMenu (false, part) : clickPart (part);
        return;
    }

    if (e.mods.isPopupMenu())
        cellMenu ({ part, scene });
    else
        clickCell (part, scene);
}

void IdeaBoardView::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto cell = cellAt (e.getPosition());

    if (cell.first >= 0 && cell.second >= 0 && board.fileAt (cell.first, cell.second) != juce::File() && onEdit != nullptr)
        onEdit (board.fileAt (cell.first, cell.second));
}

void IdeaBoardView::cellMenu (IdeaBoard::Cell cell)
{
    const auto has = board.fileAt (cell.first, cell.second) != juce::File();
    juce::PopupMenu menu;
    menu.addItem (1, "Choose Sequence...");
    menu.addItem (2, "Edit in Sequencer", has && onEdit != nullptr);
    menu.addItem (3, "Clear", has);

    menu.showMenuAsync (juce::PopupMenu::Options().withMousePosition(), [safe = juce::WeakReference<IdeaBoardView> (this), cell] (int result)
    {
        auto* self = safe.get();

        if (self == nullptr)
            return;

        if (result == 1)
            self->chooseSequence (cell);
        else if (result == 2)
            self->onEdit (self->board.fileAt (cell.first, cell.second));
        else if (result == 3)
            self->setCell (cell.first, cell.second, {});
    });
}

void IdeaBoardView::headerMenu (bool scene, int index)
{
    const auto& names = scene ? board.scenes : board.parts;
    juce::PopupMenu menu;
    menu.addItem (1, scene ? "Launch" : "Stop");
    menu.addItem (2, "Rename...");
    menu.addItem (3, scene ? "Remove Scene" : "Remove Part", names.size() > 1);

    menu.showMenuAsync (juce::PopupMenu::Options().withMousePosition(), [safe = juce::WeakReference<IdeaBoardView> (this), scene, index] (int result)
    {
        auto* self = safe.get();

        if (self == nullptr)
            return;

        if (result == 1)
            scene ? self->clickScene (index) : self->clickPart (index);
        else if (result == 3)
        {
            self->stopAll();
            scene ? self->board.removeScene (index) : self->board.removePart (index);
            self->changed();
        }
        else if (result == 2)
        {
            auto& list = scene ? self->board.scenes : self->board.parts;
            auto* dialog = new juce::AlertWindow (scene ? "Rename Scene" : "Rename Part", {}, juce::MessageBoxIconType::NoIcon, self);
            dialog->addTextEditor ("name", list[index], "Name");
            dialog->addButton ("Rename", 1, juce::KeyPress (juce::KeyPress::returnKey));
            dialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
            dialog->enterModalState (true, juce::ModalCallbackFunction::create ([safe, scene, index, dialog] (int chosen)
            {
                auto* view = safe.get();
                const auto name = dialog->getTextEditorContents ("name").trim();

                if (chosen != 1 || view == nullptr || name.isEmpty())
                    return;

                (scene ? view->board.scenes : view->board.parts).set (index, name);
                view->changed();
            }), true);
        }
    });
}

void IdeaBoardView::chooseSequence (IdeaBoard::Cell cell)
{
    chooser = std::make_unique<juce::FileChooser> ("Choose a Sequence", SequenceLibrary{}.getFolder(), juce::String ("*") + SequenceFile::extension);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe = juce::WeakReference<IdeaBoardView> (this), cell] (const juce::FileChooser& c)
    {
        if (auto* self = safe.get(); self != nullptr && c.getResult() != juce::File())
            self->setCell (cell.first, cell.second, c.getResult());
    });
}

void IdeaBoardView::askForNewBoard()
{
    auto* dialog = new juce::AlertWindow ("New Idea Board", "An empty board, kept in the Boards folder beside the Sequence Library.", juce::MessageBoxIconType::NoIcon, this);
    dialog->addTextEditor ("name", "Board", "Name");
    dialog->addButton ("Make", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    dialog->enterModalState (true, juce::ModalCallbackFunction::create ([safe = juce::WeakReference<IdeaBoardView> (this), dialog] (int result)
    {
        auto* self = safe.get();
        const auto name = dialog->getTextEditorContents ("name").trim();

        if (result != 1 || self == nullptr)
            return;

        IdeaBoard b;
        b.name = name.isNotEmpty() ? name : juce::String ("Board");
        auto target = IdeaBoard::defaultFolder().getChildFile (juce::File::createLegalFileName (b.name) + IdeaBoard::extension);

        for (int n = 2; target.exists(); ++n)
            target = IdeaBoard::defaultFolder().getChildFile (juce::File::createLegalFileName (b.name) + " " + juce::String (n) + IdeaBoard::extension);

        self->openBoard (b, target);
        b.save (target, SequenceLibrary{}.getFolder());
    }), true);
}

void IdeaBoardView::showOpenChooser()
{
    chooser = std::make_unique<juce::FileChooser> ("Open Idea Board", IdeaBoard::defaultFolder(), juce::String ("*") + IdeaBoard::extension);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe = juce::WeakReference<IdeaBoardView> (this)] (const juce::FileChooser& c)
    {
        auto* self = safe.get();

        if (self == nullptr || c.getResult() == juce::File())
            return;

        juce::String error;

        if (const auto b = IdeaBoard::load (c.getResult(), SequenceLibrary{}.getFolder(), error))
            self->openBoard (*b, c.getResult());
        else
            juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::WarningIcon).withTitle ("Open Idea Board").withMessage (error).withButton ("OK"), nullptr);
    });
}

//==============================================================================
bool IdeaBoardView::isInterestedInDragSource (const SourceDetails& details)
{
    return SequenceLibraryView::fileOfDrag (details.description).hasFileExtension (SequenceFile::extension);   // a sequence, not a kit
}

void IdeaBoardView::itemDropped (const SourceDetails& details)
{
    const auto cell = cellAt (details.localPosition.toInt());
    dropTarget = { -1, -1 };

    if (cell.first >= 0 && cell.second >= 0)
        setCell (cell.first, cell.second, SequenceLibraryView::fileOfDrag (details.description));
}

bool IdeaBoardView::isInterestedInFileDrag (const juce::StringArray& files)
{
    return std::any_of (files.begin(), files.end(), [] (const juce::String& f) { return f.endsWithIgnoreCase (SequenceFile::extension); });
}

void IdeaBoardView::filesDropped (const juce::StringArray& files, int x, int y)
{
    const auto cell = cellAt ({ x, y });

    if (cell.first < 0 || cell.second < 0)
        return;

    // Several files: along the part from the cell dropped on.
    int scene = cell.second;

    for (auto& f : files)
        if (f.endsWithIgnoreCase (SequenceFile::extension) && scene < board.scenes.size())
            board.set (cell.first, scene++, juce::File (f));

    changed();
}
