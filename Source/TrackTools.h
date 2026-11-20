#pragma once

#include <JuceHeader.h>
#include "MultitrackView.h"
#include "AudioEngine.h"

/** The toolbar under the multitrack view. On the left, the tools, the one in hand lit: Select,
    Draw (drag along an instrument track's lane to make a note clip), Split (click a clip to cut
    it there), Stretch (drag a clip's edge), Glue (click a note clip to join it with the next),
    Range (drag across the lanes to select a time range) and Mute (click a clip to silence it or
    let it be heard again); then what the tool in hand does, in a line. On the right, the Snap switch and the grid
    snapping uses in bars and beats (Auto, Bar, 1/4 ... 1/16 Triplet), the loop's start and end as Cubase's locators
    (in bars, beats, sixteenths and ticks while counting bars, else in the time format; type a new one to move it),
    the session's tempo (a click opens Tempo...), Tap (tap tempo) and the Bars switch, lit while the ruler counts bars
    and beats. Each button runs
    its command, so it does what the menu item and keys do. */
class TrackTools final : public juce::Component,
                         private juce::Timer
{
public:
    struct Commands
    {
        int select = 0, draw = 0, split = 0, stretch = 0, glue = 0, range = 0, mute = 0;
        int tempo = 0, barsAndBeats = 0, snapping = 0, tapTempo = 0;
    };

    TrackTools (juce::ApplicationCommandManager&, MultitrackView&, AudioEngine&, Commands);
    ~TrackTools() override;

    /** Runs a button's command; by default through the command manager. */
    std::function<void (int commandId)> run;

    juce::Button& getToolButton (MultitrackView::Tool);
    juce::TextButton& getTempoButton()                  { return tempoButton; }
    juce::TextButton& getBarsButton()                   { return barsButton; }
    juce::TextButton& getSnapButton()                   { return snapButton; }
    juce::ComboBox& getGridBox()                        { return gridBox; }
    juce::TextButton& getTapButton()                    { return tapButton; }
    juce::Label& getLoopStartField()                    { return loopStartField; }
    juce::Label& getLoopEndField()                      { return loopEndField; }

    /** A frame as the locators show it: bars ("5.1.1.0") while counting bars, else the time format. */
    juce::String locatorText (juce::int64 frame) const;

    /** A locator typed in the same form, as a frame; nothing when it isn't one. */
    std::optional<juce::int64> parseLocator (const juce::String&) const;

    /** Called when a grid size is chosen from the box (after the view has it), to keep it. */
    std::function<void (MultitrackView::GridSize)> onGridSize;

    /** What the tool in hand does, as shown beside the tools. */
    static juce::String hintFor (MultitrackView::Tool);

    /** The tempo as shown, e.g. "120 bpm, 4/4" or "98.5 bpm, 3/4". */
    static juce::String tempoText (double tempo, int beatsPerBar);

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int height = 24;

private:
    class ToolButton;

    void timerCallback() override;
    void update();

    juce::ApplicationCommandManager& commands;
    MultitrackView& view;
    AudioEngine& engine;
    Commands ids;
    juce::OwnedArray<ToolButton> tools;
    void locatorTyped (bool isStart);

    juce::TextButton tempoButton, barsButton { "Bars" }, snapButton { "Snap" }, tapButton { "Tap" };
    juce::ComboBox gridBox;
    juce::Label loopStartField, loopEndField;
    MultitrackView::Tool shownTool = MultitrackView::Tool::select;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackTools)
};
