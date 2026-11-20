#pragma once

#include "IdeaBoard.h"
#include "Sequencer.h"

class AudioEngine;

/** The Idea Board panel: a grid of sequences, parts down and scenes across. A click on a cell
    launches it on the next bar (at once when nothing plays), in place of its part's other cell;
    a click on a playing cell stops it on the next bar. A scene's name launches the scene, a
    part's name stops the part. Drop sequences on cells from the Sequence Library or the Finder.
    It plays through each sequence's own voice, beside the session, at the session's tempo or
    its own. Send to Session lays the scenes out in the session. The board is saved as it
    changes, in its file in the Boards folder. */
class IdeaBoardView final : public juce::Component,
                            public juce::DragAndDropTarget,
                            public juce::FileDragAndDropTarget,
                            private juce::Timer
{
public:
    explicit IdeaBoardView (AudioEngine&);
    ~IdeaBoardView() override;

    std::function<void (const IdeaBoard&)> onSendToSession;
    std::function<void (const juce::File&)> onEdit;                 // a cell's sequence, to open in the Sequencer
    std::function<void (juce::AudioProcessor*)> onPluginRemoved;   // its window to close

    const IdeaBoard& getBoard() const noexcept             { return board; }
    const juce::File& getFile() const noexcept             { return file; }
    SequencerPlayer& getPlayer() noexcept                  { return player; }
    const IdeaBoardLauncher& getLauncher() const noexcept  { return launcher; }

    /** For tests and the menus: what the clicks do. */
    void openBoard (const IdeaBoard&, const juce::File&);
    void setCell (int part, int scene, const juce::File& sequence);
    void clickCell (int part, int scene);
    void clickScene (int scene);
    void clickPart (int part);
    void stopAll();

    /** The cell under a point ({ -1, -1 } for none), and a cell's bounds. */
    IdeaBoard::Cell cellAt (juce::Point<int>) const;
    juce::Rectangle<int> cellBounds (int part, int scene) const;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDropped (const SourceDetails&) override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int x, int y) override;

private:
    void timerCallback() override;
    void changed();                      // the board changed: saved, its tracks brought up to date
    void refreshPlaying();
    void syncTracks();
    void removeTrack (IdeaBoard::Cell);
    void cellMenu (IdeaBoard::Cell);
    void headerMenu (bool scene, int index);
    void chooseSequence (IdeaBoard::Cell);
    void askForNewBoard();
    void showOpenChooser();
    void openLatest();
    double rate() const;
    juce::Rectangle<int> gridArea() const;

    AudioEngine& engine;
    IdeaBoard board;
    juce::File file;
    IdeaBoardLauncher launcher;
    SequencerPlayer player;
    juce::AudioSourcePlayer devicePlayer;

    SessionState base;                                              // a track for each cell, no clips
    std::map<IdeaBoard::Cell, juce::uint32> trackOf;
    std::map<IdeaBoard::Cell, SequenceFile::Sequence> sequences;
    std::map<IdeaBoard::Cell, juce::String> loadedFrom;             // each cell's file and its time, as loaded
    std::map<juce::uint32, std::shared_ptr<bool>> loading;          // each track's voice loading: false once it goes

    juce::TextButton newButton { "New..." }, openButton { "Open..." }, addPartButton { "+ Part" }, addSceneButton { "+ Scene" },
                     stopButton { "Stop All" }, sendButton { "Send to Session" };
    juce::ToggleButton sessionTempoButton { "Session tempo" };
    juce::Slider tempoSlider { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft };
    juce::Label title, where;
    std::unique_ptr<juce::FileChooser> chooser;
    IdeaBoard::Cell dropTarget { -1, -1 };

    static constexpr int rowHeight = 30, headerHeight = 28, partWidth = 120;

    JUCE_DECLARE_WEAK_REFERENCEABLE (IdeaBoardView)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IdeaBoardView)
};
