#pragma once

#include "AudioEngine.h"
#include "EffectPresets.h"

/** The crossfade editor (Edit Crossfade..., or a double-click on a crossfade): one crossfade, or
    several chosen at once, with both clips' sound across it and the two curves over it, and the
    level they add up to (in gain and in power) as a line, so a dip or a bump shows.

    Its length (ms), each side's shape, Symmetric (setting one side sets the other), Equal Gain and
    Equal Power, and presets (the factory ones and the user's) change every crossfade it holds at
    once, as they are made: together one undo step ("Edit Crossfade"); Cancel puts them back.

    Play Out, Play In and Play Crossfade play the outgoing clip alone, the incoming one alone, or
    both, from the pre-roll before the crossfade to the post-roll after it (of the first crossfade),
    as the clips play on their track: their gain and fades, the track's volume and pan, before
    any effects. */
class CrossfadePanel final : public juce::Component,
                             private juce::ChangeListener
{
public:
    CrossfadePanel (AudioEngine&, std::vector<SessionEdits::Crossfade>, EffectPresets::Store& = EffectPresets::userStore());
    ~CrossfadePanel() override;

    /** Called when Done or Cancel closes it (Cancel has put the crossfades back first). */
    std::function<void()> onClose;

    /** The crossfades it holds, as they are now (fewer if one has gone). */
    std::vector<SessionEdits::Crossfade> getCrossfades() const;

    /** The changes, to every crossfade it holds; false when nothing changed. */
    bool setLengthMs (double);
    bool setOutShape (FadeShape);   // with Symmetric, the in side too
    bool setInShape (FadeShape);    // with Symmetric, the out side too
    bool setShapes (FadeShape out, FadeShape in);
    void setSymmetric (bool);
    bool isSymmetric() const noexcept               { return symmetricButton.getToggleState(); }

    /** A preset's values: "ms", and "out" and "in" as places in FadeShapes::all. */
    EffectPresets::Values currentValues() const;
    bool applyValues (const EffectPresets::Values&);

    static constexpr const char* presetKey = "torqwave:crossfade";

    /** Done keeps the changes; Cancel puts every crossfade back as it was when the panel opened. */
    void done();
    void cancel();

    /** What the audition buttons play: the out clip alone, the in clip alone, or both. */
    enum class Side { out, in, both };

    /** The sound an audition plays, at the session's rate: the pre-roll, the first crossfade and
        the post-roll, stereo. Empty without a crossfade. */
    juce::AudioBuffer<float> renderAudition (Side) const;
    bool audition (Side);
    void setPreRollMs (double ms)                   { preRoll.setValue (ms); }
    void setPostRollMs (double ms)                  { postRoll.setValue (ms); }

    /** The level at the middle of the first crossfade, both sides added, in gain and in power (dB):
        0 dB in gain is steady for a sound crossfading into more of itself, 0 dB in power for two
        different sounds. */
    std::pair<float, float> middleLevelsDb() const;
    juce::String getSummary() const                 { return summary.getText(); }

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int width = 680, height = 412;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    bool edit (const std::function<bool (SessionState&, const SessionEdits::Crossfade&)>&);
    void refresh();
    juce::Rectangle<int> curveArea() const;
    std::pair<FadeShape, FadeShape> shapesNow() const;

    AudioEngine& engine;
    std::vector<std::pair<juce::uint32, juce::uint32>> pairs;   // each crossfade's out and in clip
    Session::State before;                                     // the session as the panel opened it
    int openedAt = 0;                                          // its undo position then
    bool changed = false;
    bool refreshing = false;

    juce::Label lengthLabel { {}, "Length (ms)" }, outLabel { {}, "Out side" }, inLabel { {}, "In side" },
                preLabel { {}, "Pre-roll (ms)" }, postLabel { {}, "Post-roll (ms)" }, summary;
    juce::Slider length { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight },
                 preRoll { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight },
                 postRoll { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::ComboBox outShape, inShape;
    juce::ToggleButton symmetricButton { "Symmetric" };
    juce::TextButton equalGainButton { "Equal Gain" }, equalPowerButton { "Equal Power" },
                     playOutButton { "Play Out" }, playInButton { "Play In" }, playBothButton { "Play Crossfade" },
                     doneButton { "Done" }, cancelButton { "Cancel" };
    EffectPresets::Bar presets;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrossfadePanel)
};
