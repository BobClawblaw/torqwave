#pragma once

#include "Decrypt.h"

/** Decrypt's screen in Torqwave Synth's window: the console's scope read as lock.

    Before a clip, the scope is empty and says what Decrypt is for: drop one note on it (or choose
    Decrypt a Sound...) and it finds a patch that plays like it. While it searches, the signal's
    signature sits in the middle with the best candidate's laid over it (where the two agree, the
    spokes light up); the rings are 75, 50 and 25 % lock, and each of the best twelve sits as far
    out as its lock is low, in the sector of the part it differs in most from the best, drifting
    inward as its lock rises. The panel at the right shows the candidate under the pointer (the
    best when there is none): its lock as a large figure, a bar for each part as the search
    settles it, what Ask Torq's model proposed and whether it was kept, and Stop and Take.

    Click a candidate to hear it, the middle to hear the signal; double-click a candidate to put it
    on the keys. Stop ends the search and gives the synth back the patch it had; Take keeps the
    candidate shown. Every figure on the screen is the search's own; the sweep turns only while it
    searches, and not at all when the system asks for less motion. */
class DecryptView final : public juce::Component,
                          public juce::FileDragAndDropTarget,
                          public juce::DragAndDropTarget,
                          public juce::SettableTooltipClient,
                          private juce::Timer
{
public:
    explicit DecryptView (Decrypt::Synth&);
    ~DecryptView() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    juce::String getTooltip() override;

    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void fileDragEnter (const juce::StringArray&, int, int) override;
    void fileDragExit (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;

    // A sound dragged from inside Torqwave: an open file from the Files panel.
    bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDragEnter (const SourceDetails&) override;
    void itemDragExit (const SourceDetails&) override;
    void itemDropped (const SourceDetails&) override;

    /** Decrypts a sound file (its first four seconds after the note starts), or a clip. False, with
        the reason shown, when it can't be matched. */
    bool decryptFile (const juce::File&);
    bool decrypt (const juce::AudioBuffer<float>&, double sampleRate, const juce::String& name);

    /** Carries the search on for another set of rounds, in smaller steps and with other variations
        than the last: from the candidate chosen (the one taken, or put on the keys by a
        double-click), so the next twelve are all nearer versions of that one; with none chosen,
        from the best twelve. Nothing happens with no search, or one still running. True if it started. */
    bool again();
    bool canGoAgain() const;
    juce::Rectangle<float> againBounds() const                    { return againChip; }
    int getPasses() const noexcept                                { return passes; }
    int getChainLength() const noexcept                           { return (int) chain.size(); }
    /** Plays the signal and then every step in turn; a second call stops it. */
    void playChain();
    bool isPlayingChain() const noexcept                          { return chainPlayer.isTimerRunning(); }
    /** Goes back to a step: on the keys, the later steps dropped, Again to go on from it. */
    void goBackTo (int link);
    juce::Rectangle<float> playChainBounds() const                { return playChip; }
    juce::Rectangle<float> chainBounds (int link) const           { return juce::isPositiveAndBelow (link, (int) chain.size()) ? chain[(size_t) link].at : juce::Rectangle<float>(); }
    float chainLock (int link) const                              { return juce::isPositiveAndBelow (link, (int) chain.size()) ? chain[(size_t) link].lock : 0.0f; }
    void setHovered (int candidate)                               { hovered = candidate; }    // for tests: the pointer over a candidate

    /** Ends the search, giving the synth back the patch it had before. */
    void stop();
    /** Keeps the candidate shown (the best when the pointer is on none) and ends the search. */
    void take();

    /** For tests: the search (null before a clip), whether anything moves, where a candidate is drawn
        (-1 the middle), and the places of the controls. */
    const Decrypt::Search* getSearch() const noexcept             { return search.get(); }
    void setMotion (bool allowed)                                 { motion = allowed; }
    bool isMoving() const noexcept                                { return motion && search != nullptr && search->isRunning(); }
    /** The search has ended and the view holds its last progress (it takes each on the message thread, after the search). */
    bool hasEnded() const noexcept                                { return search != nullptr && progress.finished; }
    juce::Point<float> candidateCentre (int index) const;
    juce::Rectangle<float> stopBounds() const                     { return stopChip; }
    juce::Rectangle<float> takeBounds() const                     { return takeChip; }
    juce::Rectangle<float> chooseBounds() const                   { return chooseChip; }
    juce::Rectangle<float> tablesBounds() const                   { return tablesChip; }
    bool factoryTablesOnly() const noexcept                       { return tablesOnly; }
    /** The name Ask Torq's model offered for the patch taken, waiting to be used or refused. */
    const Decrypt::Advisor::Name& offeredName() const noexcept    { return offer; }
    void useOfferedName (bool use);

    /** For tests, the panel's text where it is crowded: its heading and the status beside it (or under
        it, when the two would meet), the chain's hint and the room it has, and the candidates' lock
        labels on the scope (an empty one left out, so as not to print over a neighbour's). */
    juce::Rectangle<float> headingBounds() const                  { return topLine().headingAt; }
    juce::Rectangle<float> statusBounds() const                   { return topLine().statusAt; }
    float chainHintWidth() const;
    float panelTextWidth() const noexcept                         { return panel.getWidth() - 32.0f; }
    std::vector<juce::Rectangle<float>> lockLabelBounds() const   { return lockLabels(); }

private:
    void timerCallback() override;
    void layout();
    int candidateAt (juce::Point<float>) const;
    void hear (int candidate);
    void finish (const juce::String& how);
    void drawScope (juce::Graphics&);
    void drawPanel (juce::Graphics&);
    void drawControls (juce::Graphics&);
    const Decrypt::Candidate* shown() const;

    /** The heading and the status, measured: the status at the right of the heading's line, or on
        a line of its own under it when the two would meet. */
    struct TopLine
    {
        juce::String heading, status;
        juce::Rectangle<float> headingAt, statusAt;
    };

    TopLine topLine() const;
    juce::String chainHint() const;
    /** Each candidate's lock label, empty where it would print over one already placed: the candidate
        shown first, then the one under the pointer, then the rest best first. */
    std::vector<juce::Rectangle<float>> lockLabels() const;

    Decrypt::Synth& synth;
    std::unique_ptr<Decrypt::Search> search;
    Decrypt::Progress progress;
    SignalConsole::Patch before;                     // the synth's patch when the search began
    bool restored = true;                            // Stop or Take has settled what the synth holds
    juce::String problem, clipName, ended;

    juce::Point<float> centre;
    float radius = 100.0f;
    juce::Rectangle<float> panel, stopChip, takeChip, againChip, chooseChip, tablesChip, useChip, refuseChip;
    int passes = 0;                                  // searches run on this clip: 1 the first, more for each Again
    std::optional<SignalConsole::Patch> chosen;      // the candidate taken from the search shown, for Again to go on from
    float chosenLock = 0.0f, chosenDb = 0.0f;

    /** The chain: the best candidate each time the search came a quarter of a dB nearer the signal
        (and whatever an Again went on from), oldest first: how the sound has come along. A click
        plays a step, Play plays them all in turn after the signal, and a double-click goes back to
        one: it is put on the keys, the steps after it are dropped, and Again goes on from it.
        Emptied by a new clip. */
    struct Link
    {
        SignalConsole::Patch patch;
        float lock = 0.0f, db = 0.0f;   // its lock, and its distance from the signal in dB
        juce::Rectangle<float> at;
    };

    std::vector<Link> chain;
    juce::Rectangle<float> playChip;
    void hearPatch (const SignalConsole::Patch&);
    void growChain();
    void addLink (const SignalConsole::Patch&, float lock, float db);

    /** Where this pass began (its first best): how far it has come is shown against it. */
    float passFromDb = -1.0f, passFromLock = 0.0f;

    /** Plays the signal, then each step of the chain, one after another. */
    struct ChainPlayer final : public juce::Timer
    {
        explicit ChainPlayer (DecryptView& v) : view (v) {}
        void timerCallback() override;
        DecryptView& view;
        int next = -1;      // -1 the signal, then the chain's steps
    };

    ChainPlayer chainPlayer { *this };
    void begin (Decrypt::Target, std::vector<SignalConsole::Patch> resume);
    std::vector<juce::Point<float>> places;          // the twelve, where they are drawn
    std::vector<float> shownRadius;                  // their distance from the middle as drawn (drifting toward their lock's)
    std::vector<int> sectorOf;
    int hovered = -2, onKeys = -1;                   // -2 nothing under the pointer, -1 the middle
    float sweep = 0.0f, shownLock = 0.0f;
    bool motion = true, dragOver = false, tablesOnly = false;
    juce::TextEditor words;
    Decrypt::Advisor::Name offer;
    juce::uint32 offerFor = 0;

    std::unique_ptr<juce::FileChooser> chooser;
    juce::ThreadPool renders { juce::ThreadPoolOptions{}.withThreadName ("Torqwave Synth decrypt hear").withNumberOfThreads (1) };
    std::shared_ptr<std::atomic<bool>> alive { std::make_shared<std::atomic<bool>> (true) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DecryptView)
};
