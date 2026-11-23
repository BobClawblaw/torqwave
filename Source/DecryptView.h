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
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
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
    /** For tests: the chain's strip and its signal tile, a link's node on the scope's path, how far a
        link's sound is from the one before, whether the search made it (or it was picked to go on
        from), the link shown in the panel (-1 none), the one marked as heard (-1 the signal, -2 none)
        and as on the keys, what the panel says of the link shown, how many links were played as they
        landed, and a click on a link. addTestLinks appends copies of the last link, to try a long chain. */
    juce::Rectangle<float> stripBounds() const                    { return strip; }
    juce::Rectangle<float> signalTileBounds() const               { return signalTile; }
    juce::Point<float> chainNode (int link) const                 { return juce::isPositiveAndBelow (link, (int) chain.size()) ? chain[(size_t) link].node : juce::Point<float>(); }
    float chainMoved (int link) const                             { return juce::isPositiveAndBelow (link, (int) chain.size()) ? chain[(size_t) link].movedDb : 0.0f; }
    juce::uint32 chainAddress (int link) const                    { return juce::isPositiveAndBelow (link, (int) chain.size()) ? SignalConsole::addressOf (chain[(size_t) link].patch) : 0; }
    bool chainPicked (int link) const                             { return juce::isPositiveAndBelow (link, (int) chain.size()) && chain[(size_t) link].picked; }
    int selectedStep() const noexcept                             { return selectedLink; }
    int heardStep() const noexcept                                { return heardLink; }
    int keysStep() const noexcept                                 { return keysLink; }
    juce::String stepChange (int link) const;
    int linksPlayedAsTheyLanded() const noexcept                  { return autoPlays; }
    void addTestLinks (int count);
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
    /** For tests: the peak of the matched note, as the keys play the patch last put on them, in dB (Decrypt::auditionDb when levelled). */
    float lastKeysDb() const noexcept                             { return keysDb; }
    /** Take's name: after Take the panel asks for a name, the patch's "Node" name there to type
        over. Save (or Return) keeps the patch as one of the user's own presets under it, listed in
        the synth's Decrypted category (DecryptedPatches); a name one of the presets has asks before
        replacing it, and an empty name or a factory patch's is refused, with the reason. Not Now (or
        Escape) leaves the patch on the keys, unsaved. Nothing waits on the answer. */
    bool isNaming() const noexcept                                { return naming; }
    juce::String nameTyped() const                                { return nameBox.getText(); }
    void typeName (const juce::String& name)                      { nameBox.setText (name, false); }
    /** Save: true once it is saved; false when it asks first (a taken name) or refuses (see nameNote). */
    bool saveName (bool replace = false);
    void notNow();
    bool isAskingToReplace() const noexcept                       { return replacing; }
    const juce::String& nameNote() const noexcept                 { return namingNote; }
    juce::Rectangle<float> saveNameBounds() const                 { return saveChip; }
    juce::Rectangle<float> notNowBounds() const                   { return laterChip; }
    /** Told when a patch has been saved (the presets bar and the patch list then show it). */
    std::function<void()> onPatchSaved;

    /** For tests: how many sounds the page has played since the sound on it arrived, and what it says of a sound it refused. */
    int playsSinceSound() const noexcept                          { return plays; }
    const juce::String& refusal() const noexcept                  { return problem; }
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
    void drawStrip (juce::Graphics&);
    void drawPath (juce::Graphics&);
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

    /** The chain: how the sound came along, oldest first, from a rough first match to the nearest
        found. The search makes its links (Decrypt::Progress::chain: a link only when the nearest
        patch sounds different enough from the last link); each Again's links follow the last pass's,
        and a candidate picked to go on from becomes a link too. It is shown twice: as a strip of
        tiles across the bottom of the page (Play, the signal, then each link with its number and
        lock) and as a numbered path on the scope. A click on a tile or a node plays the link and
        shows it in the panel; a double-click goes back to it (on the keys, the later links dropped,
        Again goes on from it). Emptied by a new clip. */
    struct Link
    {
        SignalConsole::Patch patch;
        float lock = 0.0f, db = 0.0f;   // its lock, and its distance from the signal in dB
        Decrypt::Distance distance;
        SignalConsole::Signature signature;
        juce::String from;              // what it grew from
        float movedDb = 0.0f;           // how far its sound is from the link before's
        bool picked = false;            // a candidate chosen to go on from, not a link the search made
        std::array<float, SignalConsole::numParts> moved {};   // how far each part's settings moved from the link before, 0 to 1
        juce::Rectangle<float> at;      // its tile
        juce::Point<float> node;        // its place on the scope's path
    };

    std::vector<Link> chain;
    std::optional<Link> chosen;                      // the candidate taken from the search shown (or the link gone back to), for Again to go on from
    static Link linkOf (const Decrypt::Candidate&);
    juce::Rectangle<float> playChip, strip, signalTile;
    size_t passBase = 0;                             // links before this pass's
    int selectedLink = -1, heardLink = -2, keysLink = -1;   // heardLink: -1 the signal, -2 none
    int autoPlays = 0, plays = 0;                    // plays: sounds played since the sound on the page arrived
    juce::uint32 soundCount = 0;                     // counts the sounds given: a render made for an earlier one is not played
    float stripScroll = 0.0f;                        // how far the tiles are scrolled, when there are too many to shrink
    void hearPatch (const SignalConsole::Patch&);
    void hearLink (int link);
    int linkAt (juce::Point<float>) const;
    void setMoved (size_t link);
    /** Puts a patch on the synth with its level trimmed to the audition's (Decrypt::levelForKeys). */
    void putOnKeys (const SignalConsole::Patch&, const juce::String& name);
    juce::String levelNote;                          // said when Amp Level's range could not bring it to the audition's level
    float keysDb = 0.0f;
    void growChain();

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
    juce::TextEditor words, nameBox;
    bool naming = false, replacing = false;          // Take's name is being asked for; the name is taken, and Replace is asked
    juce::String namingNote;                         // why a name was refused, or where the patch was saved
    juce::Rectangle<float> saveChip, laterChip;
    void askName();
    Decrypt::Advisor::Name offer;
    juce::uint32 offerFor = 0;

    std::unique_ptr<juce::FileChooser> chooser;
    juce::ThreadPool renders { juce::ThreadPoolOptions{}.withThreadName ("Torqwave Synth decrypt hear").withNumberOfThreads (1) };
    std::shared_ptr<std::atomic<bool>> alive { std::make_shared<std::atomic<bool>> (true) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DecryptView)
};
