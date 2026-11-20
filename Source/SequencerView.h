#pragma once

#include "Sequencer.h"
#include "NoteEditor.h"

class AudioEngine;

/** The Sequencer window: one sequence from the library, open by itself to write and hear. The
    Piano Roll (or the drum grid) and the controller lanes edit the sequence file itself; it plays
    with its own transport (Play, Loop, Click) at its own tempo or the session's; its voice (the
    instrument and the effects it plays through) is down the right, to change and edit. New, Open,
    Save, Save As, Duplicate and Make Variation work on files in the Sequence Library; Undo and
    Redo (and Cmd-Z, Shift-Cmd-Z while it has the focus) step through its own edits. */
class SequencerView final : public juce::Component,
                            private juce::Timer
{
public:
    explicit SequencerView (AudioEngine&);
    ~SequencerView() override;

    /** Opens a sequence file, asking first about unsaved changes. */
    void openFile (const juce::File&);

    /** Asks about unsaved changes, then calls `then` unless the asking was cancelled. */
    void whenSaved (std::function<void()> then);

    /** Called when a file was written, so the library can show it. */
    std::function<void (const juce::File&)> onSaved;
    std::function<void (juce::AudioPluginInstance&)> onEditPlugin;
    std::function<void (juce::AudioProcessor*)> onPluginRemoved;   // its window to close

    const SequencerDocument& getDocument() const noexcept { return doc; }
    SequencerPlayer& getPlayer() noexcept                 { return player; }
    NoteEditor& getEditor() noexcept                      { return editor; }
    double getTempo() const noexcept                      { return tempo; }

    /** For tests and the menus: the actions as the buttons do them. */
    void newSequence (const juce::String& name, const juce::StringArray& folder, int bars, const juce::PluginDescription* instrument);
    juce::String save();
    juce::String saveAs (const juce::String& name, const juce::StringArray& folder);
    juce::String duplicate (bool variation);
    void setTempo (double bpm);
    void undo();
    void redo();

    /** The voice to save: the track's chain as loaded, or the one it was opened with while that is still loading or didn't all load. */
    std::shared_ptr<const juce::XmlElement> currentVoice() const;

    void resized() override;
    void paint (juce::Graphics&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    void timerCallback() override;
    void opened();                           // a sequence was opened: build its session and load its voice
    void refreshState();                     // an edit, undo or tempo: the session again from the sequence
    void refreshControls();
    void rebuildVoice();
    void chooseInstrument();
    void addEffect();
    void askForNew();
    void askToSaveAs();
    void showOpenChooser();
    juce::StringArray folderOf (const juce::File&) const;
    double rate() const;

    AudioEngine& engine;
    SequencerDocument doc;
    SequencerPlayer player;
    juce::AudioSourcePlayer devicePlayer;
    NoteEditor editor;

    SequencerDocument::Built built;
    std::shared_ptr<const SessionState> shown;
    std::shared_ptr<bool> loadAlive;
    bool loading = false, voiceIncomplete = false, drums = false, followSession = false;
    double tempo = 120.0;
    juce::String loadError;

    juce::TextButton newButton { "New..." }, openButton { "Open..." }, saveButton { "Save" }, saveAsButton { "Save As..." },
                     duplicateButton { "Duplicate" }, variationButton { "Make Variation" }, undoButton { "Undo" }, redoButton { "Redo" },
                     playButton { "Play" };
    juce::ToggleButton loopButton { "Loop" }, clickButton { "Click" }, sessionTempoButton { "Session tempo" };
    juce::Slider tempoSlider { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft };
    juce::Label title, where;

    // The voice, down the right.
    juce::Label voiceTitle { {}, "Voice" }, instrumentName;
    juce::TextButton instrumentButton { "Instrument..." }, editInstrumentButton { "Edit" }, addEffectButton { "+ Effect" };
    juce::OwnedArray<juce::TextButton> effectButtons, removeButtons;
    juce::Label voiceNote;

    std::unique_ptr<juce::FileChooser> chooser;

    static constexpr int voiceWidth = 230, rowHeight = 30;

    JUCE_DECLARE_WEAK_REFERENCEABLE (SequencerView)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequencerView)
};
