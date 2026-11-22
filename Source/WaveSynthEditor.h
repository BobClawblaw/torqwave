#pragma once

#include "WaveSynth.h"
#include "SoundControls.h"

class SignalConsoleView;

/** Torqwave Synth's window: the three oscillators with their wavetables drawn at the position they
    play, the sub and noise; the two filters with their response curves; the amp and voice; the
    envelopes (drag their points) and LFOs (draw their steps), each a modulation source to drag
    onto any knob; the oscillators' level envelopes; the modulation slots; the macros; the effects;
    a sound dropped on an oscillator made into its table; and a browser for the factory
    patches beside the user's own presets. Laid out at one size and scaled as the window is
    resized, so it stays sharp at any size and on any display. */
class WaveSynthEditor final : public juce::AudioProcessorEditor,
                              public juce::DragAndDropContainer,
                              private juce::Timer
{
public:
    explicit WaveSynthEditor (BuiltInEffects::BasicSynthProcessor&);
    ~WaveSynthEditor() override;

    static constexpr int designWidth = 1240, designHeight = 760;

    void paint (juce::Graphics&) override;
    void resized() override;

    /** The patch browser's menu: a category's patches as pages. A category of up to `mostInAPage`
        patches is one page with no label, in the browser's order (hand-made first); a larger one is
        sorted by name and cut into pages of about equal size, each labelled by the letters its names
        run from and to ("Ab - Co"). The numbers are indices into WaveSynth::factoryPatches(). */
    struct MenuPage
    {
        juce::String label;
        std::vector<int> patches;
    };

    static constexpr int mostInAPage = 40;
    static std::vector<MenuPage> menuPages (const juce::String& category);

    /** The patch browser's Favourites sub-menu: a sub-menu for each category that holds a favourite,
        in the tree's order, each listing its favourites as the tree does (the order the arrows go
        through them). An item's ID is `firstId` plus the patch's index in the factory list; the
        patch named `current` is ticked, with its category, when the favourites are being browsed.
        `ticked` says whether any is. */
    static juce::PopupMenu favouritesMenu (const juce::String& current, bool browsing, int firstId, bool& ticked);

    /** Reads every control's parameter, the modulation and the displays again. */
    void refresh();

    /** The knob for a parameter (nullptr if it has none), for tests. */
    SoundControls::Knob* knobFor (const juce::String& id) const;

    /** Shows a section of the lower half: 0 to 3 the envelopes, 4 and 5 the LFOs, 6 the oscillators'
        level envelopes; and on the right modulation slots 1 to 8 (0), 9 to 16 (1) or the effects (2). */
    void showModulator (int index);
    void showRightPage (int index);

    /** The heart in the patch browser: marks the patch it is on as a favourite, or unmarks it.
        Where the heart is, in the window. */
    void toggleFavourite();
    juce::Rectangle<int> getFavouriteBounds() const;

    /** Shows the knobs (0) or the signal console's scope (1). Nothing in the window offers the scope
        yet (it is to be Decrypt's screen): this is for tests and for Decrypt when it is here. */
    void showView (int index);
    SignalConsoleView* getConsoleView() const;

    struct Content;

private:
    void timerCallback() override                                 { refresh(); }

    std::unique_ptr<Content> content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveSynthEditor)
};
