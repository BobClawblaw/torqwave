#include "SequencerView.h"
#include "AudioEngine.h"
#include "Instrument.h"
#include "PluginMenu.h"
#include "SequenceLibrary.h"

namespace
{
    constexpr double minTempo = 20.0, maxTempo = 400.0;
}

SequencerView::SequencerView (AudioEngine& e) : engine (e)
{
    player.getEffects().prepare (rate());
    devicePlayer.setSource (&player);
    engine.deviceManager.addAudioCallback (&devicePlayer);

    for (auto* b : { &newButton, &openButton, &saveButton, &saveAsButton, &duplicateButton, &variationButton, &undoButton, &redoButton, &playButton,
                     &instrumentButton, &editInstrumentButton, &addEffectButton })
        addAndMakeVisible (b);

    for (auto* b : { &loopButton, &clickButton, &sessionTempoButton })
        addAndMakeVisible (b);

    addChildComponent (askButton);
    askButton.setTooltip ("Opens Ask Torq on this sequence: to write it, or to review it and suggest changes to apply");

    for (auto* l : { &title, &where, &voiceTitle, &instrumentName, &voiceNote })
        addAndMakeVisible (l);

    addAndMakeVisible (tempoSlider);
    addAndMakeVisible (lengthSlider);
    lengthSlider.setTooltip ("The sequence's length in bars, by the beat (an undo step)");
    lengthSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 80, 24);
    lengthSlider.textFromValueFunction = [] (double bars) { return juce::String (bars, juce::exactlyEqual (bars, std::round (bars)) ? 0 : 2) + (juce::exactlyEqual (bars, 1.0) ? " bar" : " bars"); };
    lengthSlider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue(); };
    lengthSlider.onValueChange = [this] { setLength (lengthSlider.getValue()); };
    addAndMakeVisible (editor);

    title.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    voiceTitle.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    voiceNote.setJustificationType (juce::Justification::topLeft);
    voiceNote.setFont (juce::FontOptions (12.0f));
    where.setJustificationType (juce::Justification::centredRight);

    newButton.setTooltip ("A new sequence in a folder of the library, with an instrument to write it for");
    openButton.setTooltip ("Opens a sequence from the library (or double-click one there, or Edit in Sequencer on its menu)");
    saveButton.setTooltip ("Writes the sequence to its file, with its voice as it is now");
    saveAsButton.setTooltip ("Writes it under another name or in another folder, and goes on with that file");
    duplicateButton.setTooltip ("Saves a copy of what is here beside it and opens the copy");
    variationButton.setTooltip ("Saves a copy named as a variation of it (the file says which sequence it is a variation of) and opens the copy");
    loopButton.setTooltip ("Plays the sequence round and round; off, it plays once");
    clickButton.setTooltip ("A click on every beat while it plays");
    sessionTempoButton.setTooltip ("Plays at the session's tempo at its playhead, instead of the window's own");
    tempoSlider.setTooltip ("The window's own tempo: the notes stay on their beats at any tempo");

    loopButton.setToggleState (true, juce::dontSendNotification);
    tempoSlider.setRange (minTempo, maxTempo, 0.1);
    tempoSlider.setTextValueSuffix (" BPM");
    tempoSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 80, 24);
    tempoSlider.setValue (tempo, juce::dontSendNotification);
    tempoSlider.onValueChange = [this] { setTempo (tempoSlider.getValue()); };

    newButton.onClick = [this] { askForNew(); };
    openButton.onClick = [this] { showOpenChooser(); };
    saveButton.onClick = [this]
    {
        if (doc.getFile() == juce::File())
            askToSaveAs();
        else if (const auto error = save(); error.isNotEmpty())
            juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::WarningIcon).withTitle ("Save").withMessage (error).withButton ("OK"), nullptr);
    };
    saveAsButton.onClick = [this] { askToSaveAs(); };
    duplicateButton.onClick = [this] { duplicate (false); };
    variationButton.onClick = [this] { askForVariation(); };
    undoButton.onClick = [this] { undo(); };
    redoButton.onClick = [this] { redo(); };
    playButton.onClick = [this]
    {
        if (player.isPlaying())
            player.stop();
        else
            player.play();

        refreshControls();
    };
    loopButton.onClick = [this] { player.setLooping (loopButton.getToggleState()); };
    clickButton.onClick = [this] { player.setClick (clickButton.getToggleState()); };
    sessionTempoButton.onClick = [this]
    {
        followSession = sessionTempoButton.getToggleState();

        if (! followSession)
            setTempo (tempoSlider.getValue());

        timerCallback();
    };
    instrumentButton.onClick = [this] { chooseInstrument(); };
    editInstrumentButton.onClick = [this]
    {
        if (auto* p = player.getEffects().getInstrument (built.trackId); p != nullptr && onEditPlugin != nullptr)
            onEditPlugin (*p);
    };
    addEffectButton.onClick = [this] { addEffect(); };

    editor.readClip = [this]() -> std::optional<NoteEditor::Shown>
    {
        if (shown == nullptr || built.clipId == 0)
            return std::nullopt;

        const auto found = shown->findNoteClip (built.clipId);

        if (! found.has_value())
            return std::nullopt;

        NoteEditor::Shown s;
        s.clip = shown->tracks[(size_t) found->first].noteClips[(size_t) found->second];
        s.sampleRate = shown->sampleRate;
        s.beatSeconds = 60.0 / tempo;
        s.trackName = doc.get().name;
        s.hasInstrument = player.getEffects().getInstrument (built.trackId) != nullptr;
        s.drums = drums;
        s.beatsPerBar = doc.get().beatsPerBar;
        s.key = doc.get().key;
        return s;
    };
    editor.readPlayhead = [this]() -> std::optional<juce::int64>
    {
        return player.isPlaying() ? std::optional<juce::int64> (player.getPosition()) : std::nullopt;
    };
    editor.apply = [this] (const NoteClip& clip, const juce::String& description)
    {
        if (shown == nullptr)
            return;

        auto s = *shown;

        if (SessionEdits::replaceNoteClip (s, clip.id, clip))
        {
            doc.apply (SequencerDocument::withClip (doc.get(), s, built.clipId), description);
            refreshState();
        }
    };
    editor.onKeyChosen = [this] (const juce::String& key)
    {
        auto q = doc.get();
        q.key = key;
        doc.apply (q, "Key " + key);
        refreshControls();
    };
    editor.audition = [this] (int pitch, float velocity) { player.audition (pitch, velocity); };
    editor.showDrums = [this] (bool on)
    {
        drums = on;
        editor.refresh();
    };

    // An empty sequence to start with, as New would make one.
    doc.open (SequencerDocument::blank ("New Sequence", 2, 4, tempo), {});
    opened();
    setWantsKeyboardFocus (true);
    startTimerHz (10);
}

SequencerView::~SequencerView()
{
    stopTimer();

    if (loadAlive != nullptr)
        *loadAlive = false;

    engine.deviceManager.removeAudioCallback (&devicePlayer);
    devicePlayer.setSource (nullptr);

    if (onPluginRemoved != nullptr)
        for (auto* p : player.getEffects().getAllPlugins())
            onPluginRemoved (p);
}

double SequencerView::rate() const
{
    const auto r = engine.getOutputSampleRate();
    return r > 0.0 ? r : engine.session.getSampleRate();
}

juce::StringArray SequencerView::folderOf (const juce::File& file) const
{
    const auto library = SequenceLibrary{}.getFolder();

    if (file == juce::File() || ! file.isAChildOf (library))
        return { "Inbox" };

    return juce::StringArray::fromTokens (file.getParentDirectory().getRelativePathFrom (library), "/\\", {});
}

//==============================================================================
void SequencerView::whenSaved (std::function<void()> then)
{
    if (! doc.isDirty())
    {
        then();
        return;
    }

    const auto options = juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::QuestionIcon).withTitle ("Sequencer")
                             .withMessage ("Save the changes to " + doc.get().name + " first?")
                             .withButton ("Save").withButton ("Don't Save").withButton ("Cancel").withAssociatedComponent (this);

    juce::AlertWindow::showAsync (options, [safe = juce::WeakReference<SequencerView> (this), then] (int result)
    {
        auto* self = safe.get();

        if (self == nullptr || result == 0)
            return;

        if (result == 1)
        {
            const auto error = self->doc.getFile() == juce::File() ? juce::String ("It has no file yet: use Save As first.") : self->save();

            if (error.isNotEmpty())
            {
                juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::WarningIcon).withTitle ("Save").withMessage (error).withButton ("OK"), nullptr);
                return;
            }
        }

        then();
    });
}

void SequencerView::openFile (const juce::File& file)
{
    whenSaved ([safe = juce::WeakReference<SequencerView> (this), file]
    {
        auto* self = safe.get();

        if (self == nullptr)
            return;

        juce::String error;
        auto q = SequenceFile::load (file, error);

        if (! q.has_value())
        {
            juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::WarningIcon).withTitle ("Open Sequence").withMessage (error).withButton ("OK"), nullptr);
            return;
        }

        self->doc.open (std::move (*q), file);
        self->opened();
    });
}

void SequencerView::opened()
{
    player.stop();
    const auto& q = doc.get();

    if (! followSession)
    {
        tempo = juce::jlimit (minTempo, maxTempo, q.writtenTempo);
        tempoSlider.setValue (tempo, juce::dontSendNotification);
    }

    built = SequencerDocument::sessionFor (q, tempo, rate());
    drums = q.role == "Drums";
    loadVoice();
    refreshState();
    editor.showPattern();
}

void SequencerView::loadVoice()
{
    if (loadAlive != nullptr)
        *loadAlive = false;   // a voice still loading for the sequence before goes no further

    loadAlive = std::make_shared<bool> (true);

    if (onPluginRemoved != nullptr)
        for (auto* p : player.getEffects().getAllPlugins())
            onPluginRemoved (p);

    player.getEffects().clear();
    built.effects = SequencerDocument::sessionFor (doc.get(), tempo, rate()).effects;
    loading = built.effects != nullptr;
    voiceIncomplete = false;
    loadError.clear();

    if (loading)
        engine.loadEffectsInto (player.getEffects(), rate(), { { built.trackId, built.effects } },
                                [safe = juce::WeakReference<SequencerView> (this), alive = loadAlive] (const juce::String& error)
                                {
                                    auto* self = safe.get();

                                    if (self == nullptr || ! *alive)
                                        return;

                                    self->loading = false;
                                    self->voiceIncomplete = error.isNotEmpty();
                                    self->loadError = error;
                                    self->rebuildVoice();
                                    self->editor.refresh();
                                },
                                loadAlive);

    rebuildVoice();
}

void SequencerView::refreshState()
{
    shown = std::make_shared<const SessionState> (SequencerDocument::withSequence (built, doc.get(), tempo));
    player.setState (shown, built.trackId);
    editor.refresh();
    refreshControls();
}

void SequencerView::refreshControls()
{
    const auto& q = doc.get();
    title.setText (q.name + (doc.isDirty() ? " *" : "") + (q.variationOf.isNotEmpty() ? "   (a variation of " + q.variationOf + ")" : juce::String()),
                   juce::dontSendNotification);
    undoButton.setEnabled (doc.canUndo());
    redoButton.setEnabled (doc.canRedo());
    undoButton.setTooltip (doc.canUndo() ? "Undo " + doc.getUndoDescription() : juce::String());
    redoButton.setTooltip (doc.canRedo() ? "Redo " + doc.getRedoDescription() : juce::String());
    playButton.setButtonText (player.isPlaying() ? "Stop" : "Play");
    tempoSlider.setEnabled (! followSession);

    const auto beat = 1.0 / juce::jmax (1, q.beatsPerBar);
    lengthSlider.setRange (beat, 256.0, beat);
    lengthSlider.setValue (q.bars(), juce::dontSendNotification);
}

void SequencerView::setLength (double bars)
{
    auto q = doc.get();
    const auto beats = juce::jmax ((juce::int64) 1, (juce::int64) std::llround (bars * juce::jmax (1, q.beatsPerBar)));
    const auto length = beats * SequenceFile::ticksPerBeat;

    if (length == q.length)
        return;

    if (q.loopLength > 0 && (q.loopLength == q.length || q.loopLength > length))
        q.loopLength = length;

    q.length = length;
    doc.apply (q, "Length " + lengthSlider.getTextFromValue ((double) beats / juce::jmax (1, q.beatsPerBar)));
    refreshState();
}

void SequencerView::rebuildVoice()
{
    auto& effects = player.getEffects();
    const auto* instrument = effects.getInstrument (built.trackId);
    instrumentName.setText (instrument != nullptr ? instrument->getName()
                                : loading ? "Loading " + SequenceFile::instrumentOf (doc.get()) + "..."
                                          : juce::String ("No instrument: choose one to hear it"),
                            juce::dontSendNotification);
    editInstrumentButton.setEnabled (instrument != nullptr);

    effectButtons.clear();
    removeButtons.clear();
    const auto first = instrument != nullptr ? 1 : 0;

    for (int i = first; i < effects.getNumSlots (built.trackId); ++i)
    {
        auto* plugin = effects.getPlugin (built.trackId, i);

        if (plugin == nullptr)
            continue;

        auto* edit = effectButtons.add (new juce::TextButton (plugin->getName()));
        edit->setTooltip ("Opens its window");
        edit->onClick = [this, slotId = effects.getSlotId (built.trackId, i)]
        {
            if (auto* p = player.getEffects().findSlot (built.trackId, slotId); p != nullptr && onEditPlugin != nullptr)
                onEditPlugin (*p);
        };

        auto* remove = removeButtons.add (new juce::TextButton (juce::String::fromUTF8 ("\xc3\x97")));
        remove->setTooltip ("Takes it out of the voice");
        remove->onClick = [this, slot = i]
        {
            juce::MessageManager::callAsync ([safe = juce::WeakReference<SequencerView> (this), slot]
            {
                auto* self = safe.get();

                if (self == nullptr)
                    return;

                if (auto* p = self->player.getEffects().getPlugin (self->built.trackId, slot); p != nullptr && self->onPluginRemoved != nullptr)
                    self->onPluginRemoved (p);

                auto removed = self->player.getEffects().remove (self->built.trackId, slot);
                self->doc.markVoiceChanged();
                self->voiceIncomplete = false;
                self->rebuildVoice();
                self->refreshControls();
            });
        };

        addAndMakeVisible (edit);
        addAndMakeVisible (remove);
    }

    voiceNote.setText (voiceIncomplete ? "Some of the voice didn't load (" + loadError.trim() + "). Saving keeps it as it was in the file until you change it."
                                       : "The instrument and effects saved with the sequence: a drop makes its track from them.",
                       juce::dontSendNotification);
    resized();
}

//==============================================================================
void SequencerView::chooseInstrument()
{
    const auto instruments = Instrument::known (engine.knownPlugins);
    std::optional<juce::PluginDescription> current;

    if (auto* p = player.getEffects().getInstrument (built.trackId))
        current = p->getPluginDescription();

    juce::PopupMenu menu;
    Instrument::addMenu (menu, instruments, [safe = juce::WeakReference<SequencerView> (this)] (const juce::PluginDescription& d)
    {
        if (auto* self = safe.get())
            self->setInstrument (d);
    }, current.has_value() ? &*current : nullptr);

    if (instruments.isEmpty())
        menu.addItem ("No instruments are known: scan with Plug-ins... first", false, false, nullptr);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (instrumentButton));
}

void SequencerView::setInstrument (const juce::PluginDescription& d)
{
    engine.createPlugin (d, rate(), [safe = juce::WeakReference<SequencerView> (this)] (std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error)
    {
        auto* view = safe.get();

        if (view == nullptr)
            return;

        if (instance == nullptr)
        {
            juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::WarningIcon).withTitle ("Instrument").withMessage (error).withButton ("OK"), nullptr);
            return;
        }

        auto& effects = view->player.getEffects();

        if (auto* old = effects.getInstrument (view->built.trackId))
        {
            if (view->onPluginRemoved != nullptr)
                view->onPluginRemoved (old);

            auto removed = effects.remove (view->built.trackId, 0);
        }

        effects.add (view->built.trackId, std::move (instance));
        view->doc.markVoiceChanged();
        view->voiceIncomplete = false;
        view->rebuildVoice();
        view->refreshControls();
        view->editor.refresh();

        // Its window, to hear and set it at once.
        if (auto* p = effects.getInstrument (view->built.trackId); p != nullptr && view->onEditPlugin != nullptr)
            view->onEditPlugin (*p);
    });
}

void SequencerView::addEffect()
{
    juce::Array<juce::PluginDescription> types;

    for (auto& type : engine.knownPlugins.getTypes())
        if (! type.isInstrument)
            types.add (type);

    juce::PopupMenu menu;
    PluginMenu::add (menu, types);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (addEffectButton),
                        [safe = juce::WeakReference<SequencerView> (this), types] (int result)
    {
        auto* self = safe.get();

        if (self == nullptr || result <= 0 || result > types.size())
            return;

        self->engine.createPlugin (types.getReference (result - 1), self->rate(),
                                   [safe] (std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error)
        {
            auto* view = safe.get();

            if (view == nullptr)
                return;

            if (instance == nullptr)
            {
                juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::WarningIcon).withTitle ("Effect").withMessage (error).withButton ("OK"), nullptr);
                return;
            }

            auto& added = *instance;
            view->player.getEffects().add (view->built.trackId, std::move (instance));
            view->doc.markVoiceChanged();
            view->voiceIncomplete = false;
            view->rebuildVoice();
            view->refreshControls();

            if (view->onEditPlugin != nullptr)
                view->onEditPlugin (added);
        });
    });
}

std::shared_ptr<const juce::XmlElement> SequencerView::currentVoice() const
{
    // Until the voice is all in, the file's stays as it was.
    if (loading || voiceIncomplete)
        return doc.get().voice;

    if (shown == nullptr)
        return nullptr;

    auto voice = SequenceFile::voiceOf (*shown, built.trackId, player.getEffects().createXml (built.trackId));
    return voice != nullptr ? std::shared_ptr<const juce::XmlElement> (voice.release()) : nullptr;
}

//==============================================================================
juce::String SequencerView::save()
{
    if (doc.getFile() == juce::File())
        return "It has no file yet: use Save As.";

    const auto error = doc.saveTo (doc.getFile(), currentVoice());

    if (error.isEmpty() && onSaved != nullptr)
        onSaved (doc.getFile());

    refreshControls();
    return error;
}

juce::String SequencerView::saveAs (const juce::String& name, const juce::StringArray& folder)
{
    SequenceLibrary library;
    library.makeFolders();
    auto q = doc.get();
    q.name = name.trim().isNotEmpty() ? name.trim() : q.name;
    SequenceLibrary::setRoleFrom (q, folder);
    doc.apply (q, "Save As " + q.name);

    const auto target = SequenceLibrary::newFileIn (library.folderAt (folder), q.name);
    const auto error = doc.saveTo (target, currentVoice());

    if (error.isEmpty() && onSaved != nullptr)
        onSaved (target);

    refreshState();
    return error;
}

juce::String SequencerView::duplicate (bool variation, const NoteTools::Vary* changes)
{
    SequenceLibrary library;
    auto q = variation ? SequencerDocument::variationOf (doc.get()) : doc.get();

    // Its notes varied: as a clip of the window's session, then back into ticks.
    if (changes != nullptr && shown != nullptr)
        if (const auto found = shown->findNoteClip (built.clipId))
        {
            auto s = *shown;
            auto clip = s.tracks[(size_t) found->first].noteClips[(size_t) found->second];
            clip.notes = NoteTools::vary (clip.notes, *changes, clip.offset + clip.length);
            SessionEdits::replaceNoteClip (s, clip.id, clip);
            const auto varied = SequencerDocument::withClip (q, s, built.clipId);
            q.notes = varied.notes;
        }

    q.voice = currentVoice();
    const auto dir = doc.getFile() != juce::File() ? doc.getFile().getParentDirectory() : library.folderAt (folderOf (doc.getFile()));
    const auto target = SequenceLibrary::newFileIn (dir, q.name);

    if (const auto error = SequenceFile::save (q, target); error.isNotEmpty())
        return error;

    // The copy is open now, its voice as loaded here.
    const auto keepIncomplete = voiceIncomplete;
    doc.open (q, target);
    voiceIncomplete = keepIncomplete;

    if (onSaved != nullptr)
        onSaved (target);

    refreshState();
    return {};
}

void SequencerView::newSequence (const juce::String& name, const juce::StringArray& folder, int bars, const juce::PluginDescription* instrument)
{
    SequenceLibrary library;
    library.makeFolders();
    auto q = SequencerDocument::blank (name.trim().isNotEmpty() ? name.trim() : juce::String ("New Sequence"), bars, 4, tempo);
    SequenceLibrary::setRoleFrom (q, folder);
    doc.open (q, SequenceLibrary::newFileIn (library.folderAt (folder), q.name));
    doc.markVoiceChanged();   // nothing is on disk yet
    opened();

    if (instrument == nullptr)
        return;

    engine.createPlugin (*instrument, rate(), [safe = juce::WeakReference<SequencerView> (this)] (std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String&)
    {
        auto* self = safe.get();

        if (self == nullptr || instance == nullptr)
            return;

        self->player.getEffects().add (self->built.trackId, std::move (instance));
        self->rebuildVoice();
        self->editor.refresh();
    });
}

void SequencerView::setTempo (double bpm)
{
    tempo = juce::jlimit (minTempo, maxTempo, bpm);

    if (! followSession)
        tempoSlider.setValue (tempo, juce::dontSendNotification);

    refreshState();
}

void SequencerView::undo()
{
    doc.keepVoice (currentVoice());
    const auto voice = doc.get().voice;
    doc.undo();

    if (doc.get().voice != voice)
        loadVoice();

    refreshState();
}

void SequencerView::redo()
{
    doc.keepVoice (currentVoice());
    const auto voice = doc.get().voice;
    doc.redo();

    if (doc.get().voice != voice)
        loadVoice();

    refreshState();
}

SequenceFile::Sequence SequencerView::currentSequence() const
{
    auto q = doc.get();
    q.voice = currentVoice();
    return q;
}

void SequencerView::applyEdit (const SequenceFile::Sequence& edited, const juce::String& description)
{
    const auto live = currentVoice();
    doc.keepVoice (live);
    const auto voice = doc.get().voice;
    auto q = edited;

    if (q.voice == live || (q.voice != nullptr && live != nullptr && q.voice->isEquivalentTo (live.get(), false)))
        q.voice = voice;

    doc.apply (std::move (q), description);

    if (doc.get().voice != voice)
        loadVoice();

    refreshState();
}

void SequencerView::showAskButton (const juce::String& name, std::function<void()> onClick)
{
    askButton.setButtonText (name);
    askButton.onClick = std::move (onClick);
    askButton.setVisible (true);
    resized();
}

//==============================================================================
void SequencerView::askForNew()
{
    SequenceLibrary{}.makeFolders();
    const auto folders = SequenceLibrary{}.folderNames();
    const auto instruments = Instrument::known (engine.knownPlugins);
    juce::StringArray names { "None" };

    for (auto& d : instruments)
        names.add (Instrument::title (d));

    const juce::StringArray lengths { "1 bar", "2 bars", "4 bars", "8 bars", "16 bars" };
    auto* box = new juce::AlertWindow ("New Sequence", "A new sequence in the Sequencer, saved in the folder chosen when you save it.",
                                       juce::MessageBoxIconType::NoIcon, this);
    box->addTextEditor ("name", "New Sequence", "Name");
    box->addComboBox ("folder", folders, "Folder");
    box->getComboBoxComponent ("folder")->setSelectedItemIndex (juce::jmax (0, folders.indexOf (SequenceLibrary::nameOf (folderOf (doc.getFile())))));
    box->addComboBox ("instrument", names, "Instrument");
    box->getComboBoxComponent ("instrument")->setSelectedItemIndex (names.size() > 1 ? 1 : 0);
    box->addComboBox ("bars", lengths, "Length");
    box->getComboBoxComponent ("bars")->setSelectedItemIndex (2);
    box->addButton ("Make", 1, juce::KeyPress (juce::KeyPress::returnKey));
    box->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    box->enterModalState (true, juce::ModalCallbackFunction::create ([safe = juce::WeakReference<SequencerView> (this), box, folders, instruments, lengths] (int result)
    {
        if (result != 1 || safe.get() == nullptr)
            return;

        const auto name = box->getTextEditorContents ("name");
        const auto folder = SequenceLibrary::pathOf (folders[box->getComboBoxComponent ("folder")->getSelectedItemIndex()]);
        const auto choice = box->getComboBoxComponent ("instrument")->getSelectedItemIndex();
        const auto bars = lengths[box->getComboBoxComponent ("bars")->getSelectedItemIndex()].getIntValue();
        const auto instrument = choice > 0 && choice <= instruments.size() ? std::optional<juce::PluginDescription> (instruments[choice - 1]) : std::nullopt;

        safe->whenSaved ([safe, name, folder, bars, instrument]
        {
            if (auto* self = safe.get())
                self->newSequence (name, folder, bars, instrument.has_value() ? &*instrument : nullptr);
        });
    }), true);
}

void SequencerView::askForVariation()
{
    const juce::StringArray amounts { "None", "A little", "Some", "A lot" };
    const juce::StringArray densities { "Much thinner", "Thinner", "The same", "Denser", "Much denser" };
    auto* dialog = new juce::AlertWindow ("Make Variation",
                                          "Saves a copy named as a variation of " + doc.get().name + ", with its notes changed as chosen here "
                                          "(by chance: each time is different), and opens it. Notes move within " + editor.getKey().name() + ".",
                                          juce::MessageBoxIconType::NoIcon, this);
    dialog->addComboBox ("velocity", amounts, "Velocity");
    dialog->addComboBox ("pitch", amounts, "Notes in the key");
    dialog->addComboBox ("density", densities, "Density");
    dialog->getComboBoxComponent ("velocity")->setSelectedItemIndex (0);
    dialog->getComboBoxComponent ("pitch")->setSelectedItemIndex (0);
    dialog->getComboBoxComponent ("density")->setSelectedItemIndex (2);
    dialog->addButton ("Make", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    dialog->enterModalState (true, juce::ModalCallbackFunction::create ([safe = juce::WeakReference<SequencerView> (this), dialog] (int result)
    {
        auto* self = safe.get();

        if (result != 1 || self == nullptr)
            return;

        const float shares[] = { 0.0f, 0.1f, 0.25f, 0.5f };
        const float density[] = { -0.5f, -0.25f, 0.0f, 0.25f, 0.5f };
        NoteTools::Vary changes;
        changes.velocity = shares[juce::jlimit (0, 3, dialog->getComboBoxComponent ("velocity")->getSelectedItemIndex())];
        changes.pitch = shares[juce::jlimit (0, 3, dialog->getComboBoxComponent ("pitch")->getSelectedItemIndex())];
        changes.density = density[juce::jlimit (0, 4, dialog->getComboBoxComponent ("density")->getSelectedItemIndex())];
        changes.step = self->editor.gridStepFrames() > 0 ? self->editor.gridStepFrames()
                                                         : (juce::int64) std::llround (15.0 / self->tempo * self->rate());   // a quarter beat
        changes.scale = self->editor.getKey();
        changes.seed = (juce::uint32) juce::Random::getSystemRandom().nextInt();

        if (const auto error = self->duplicate (true, &changes); error.isNotEmpty())
            juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::WarningIcon).withTitle ("Make Variation").withMessage (error).withButton ("OK"), nullptr);
    }), true);
}

void SequencerView::askToSaveAs()
{
    SequenceLibrary{}.makeFolders();
    const auto folders = SequenceLibrary{}.folderNames();
    auto* box = new juce::AlertWindow ("Save Sequence As", "Saves it under this name in this folder, and goes on with that file.",
                                       juce::MessageBoxIconType::NoIcon, this);
    box->addTextEditor ("name", doc.get().name, "Name");
    box->addComboBox ("folder", folders, "Folder");
    box->getComboBoxComponent ("folder")->setSelectedItemIndex (juce::jmax (0, folders.indexOf (SequenceLibrary::nameOf (folderOf (doc.getFile())))));
    box->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    box->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    box->enterModalState (true, juce::ModalCallbackFunction::create ([safe = juce::WeakReference<SequencerView> (this), box, folders] (int result)
    {
        auto* self = safe.get();

        if (result != 1 || self == nullptr)
            return;

        const auto error = self->saveAs (box->getTextEditorContents ("name"), SequenceLibrary::pathOf (folders[box->getComboBoxComponent ("folder")->getSelectedItemIndex()]));

        if (error.isNotEmpty())
            juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::WarningIcon).withTitle ("Save As").withMessage (error).withButton ("OK"), nullptr);
    }), true);
}

void SequencerView::showOpenChooser()
{
    chooser = std::make_unique<juce::FileChooser> ("Open Sequence", SequenceLibrary{}.getFolder(), juce::String ("*") + SequenceFile::extension);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe = juce::WeakReference<SequencerView> (this)] (const juce::FileChooser& c)
    {
        if (auto* self = safe.get(); self != nullptr && c.getResult() != juce::File())
            self->openFile (c.getResult());
    });
}

//==============================================================================
void SequencerView::timerCallback()
{
    if (followSession)
    {
        const auto session = engine.session.getState();   // held: the map refers to its tempo changes
        const auto t = session->tempoMap().tempoAt (engine.getPlayer().getPositionFrames());

        if (! juce::exactlyEqual (t, tempo))
            setTempo (t);
    }

    if (shown != nullptr)
    {
        const auto map = shown->tempoMap();
        where.setText (Metronome::positionText (map, player.getPosition()), juce::dontSendNotification);
    }

    if (playButton.getButtonText() != (player.isPlaying() ? "Stop" : "Play"))
        refreshControls();
}

bool SequencerView::keyPressed (const juce::KeyPress& key)
{
    const auto command = juce::ModifierKeys::commandModifier;

    if (key == juce::KeyPress ('z', command, 0))
    {
        undo();
        return true;
    }

    if (key == juce::KeyPress ('z', command | juce::ModifierKeys::shiftModifier, 0))
    {
        redo();
        return true;
    }

    if (key == juce::KeyPress::spaceKey)
    {
        playButton.triggerClick();
        return true;
    }

    return false;
}

void SequencerView::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.setColour (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId).contrasting (0.15f));
    g.drawVerticalLine (getWidth() - voiceWidth - 1, (float) (2 * rowHeight + 8), (float) getHeight());
}

void SequencerView::resized()
{
    auto area = getLocalBounds().reduced (4);
    const auto place = [] (juce::Rectangle<int>& row, juce::Component& c, int width)
    {
        c.setBounds (row.removeFromLeft (width).reduced (2));
    };

    auto files = area.removeFromTop (rowHeight);
    place (files, newButton, 64);
    place (files, openButton, 70);
    place (files, saveButton, 54);
    place (files, saveAsButton, 84);
    place (files, duplicateButton, 84);
    place (files, variationButton, 116);
    files.removeFromLeft (8);
    place (files, undoButton, 54);
    place (files, redoButton, 54);
    files.removeFromLeft (8);
    title.setBounds (files);

    auto transport = area.removeFromTop (rowHeight);
    place (transport, playButton, 60);
    place (transport, loopButton, 64);
    place (transport, clickButton, 64);
    place (transport, tempoSlider, 170);
    place (transport, lengthSlider, 150);
    place (transport, sessionTempoButton, 124);

    if (askButton.isVisible())
        askButton.setBounds (transport.removeFromRight (120).reduced (2));

    where.setBounds (transport);
    area.removeFromTop (4);

    auto voice = area.removeFromRight (voiceWidth).reduced (6, 0);
    voiceTitle.setBounds (voice.removeFromTop (24));
    instrumentName.setBounds (voice.removeFromTop (24));
    auto instrumentRow = voice.removeFromTop (rowHeight);
    place (instrumentRow, instrumentButton, 110);
    place (instrumentRow, editInstrumentButton, 54);
    voice.removeFromTop (6);

    for (int i = 0; i < effectButtons.size(); ++i)
    {
        auto row = voice.removeFromTop (rowHeight);
        removeButtons[i]->setBounds (row.removeFromRight (28).reduced (2));
        effectButtons[i]->setBounds (row.reduced (2));
    }

    addEffectButton.setBounds (voice.removeFromTop (rowHeight).removeFromLeft (100).reduced (2));
    voice.removeFromTop (6);
    voiceNote.setBounds (voice.removeFromTop (80));

    editor.setBounds (area.withTrimmedRight (4));
}
