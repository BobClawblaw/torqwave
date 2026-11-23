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

    /** Decrypts a sound file (its first four seconds after the note starts), or a clip. False, with
        the reason shown, when it can't be matched. */
    bool decryptFile (const juce::File&);
    bool decrypt (const juce::AudioBuffer<float>&, double sampleRate, const juce::String& name);

    /** Ends the search, giving the synth back the patch it had before. */
    void stop();
    /** Keeps the candidate shown (the best when the pointer is on none) and ends the search. */
    void take();

    /** For tests: the search (null before a clip), whether anything moves, where a candidate is drawn
        (-1 the middle), and the places of the controls. */
    const Decrypt::Search* getSearch() const noexcept             { return search.get(); }
    void setMotion (bool allowed)                                 { motion = allowed; }
    bool isMoving() const noexcept                                { return motion && search != nullptr && search->isRunning(); }
    juce::Point<float> candidateCentre (int index) const;
    juce::Rectangle<float> stopBounds() const                     { return stopChip; }
    juce::Rectangle<float> takeBounds() const                     { return takeChip; }
    juce::Rectangle<float> chooseBounds() const                   { return chooseChip; }
    juce::Rectangle<float> tablesBounds() const                   { return tablesChip; }
    bool factoryTablesOnly() const noexcept                       { return tablesOnly; }
    /** The name Ask Torq's model offered for the patch taken, waiting to be used or refused. */
    const Decrypt::Advisor::Name& offeredName() const noexcept    { return offer; }
    void useOfferedName (bool use);

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

    Decrypt::Synth& synth;
    std::unique_ptr<Decrypt::Search> search;
    Decrypt::Progress progress;
    SignalConsole::Patch before;                     // the synth's patch when the search began
    bool restored = true;                            // Stop or Take has settled what the synth holds
    juce::String problem, clipName, ended;

    juce::Point<float> centre;
    float radius = 100.0f;
    juce::Rectangle<float> panel, stopChip, takeChip, chooseChip, tablesChip, useChip, refuseChip;
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
