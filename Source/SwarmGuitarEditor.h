#pragma once

#include "SwarmGuitar.h"
#include "SoundControls.h"
#include "EffectPresets.h"

/** Swarm Guitar's window. The guitar's strings are drawn across the top left, each with the note
    it is tuned to (once a note has been played; before that, its place from the played note) and
    how many cents off it sits, swinging as it rings, with a mark where the pick is. Under them the
    knobs for the strings; beside them the amp, with the curve it bends the signal through; below,
    the strum, and the guitars across the stereo field. Knobs throughout, in real units, in
    Torqwave's theme, light or dark. */
class SwarmGuitarEditor final : public juce::AudioProcessorEditor,
                                private juce::Timer
{
public:
    explicit SwarmGuitarEditor (BuiltInEffects::SwarmGuitarProcessor&);
    ~SwarmGuitarEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int windowWidth = 900, windowHeight = 540;

    /** What the window shows, for the tests: a knob by its name, how many strings are drawn, a
        string's label (0 the lowest), and how many guitars the stereo picture has. */
    SoundControls::Knob* knob (const juce::String& name);
    int getStringsShown() const;
    juce::String getStringLabel (int string) const;
    int getGuitarsShown() const;

    /** Reads the instrument again now, as its timer does. */
    void refreshNow()                                      { timerCallback(); }

    /** Draws every string ringing this hard (0 to 1), for a picture of the window with no audio playing. */
    void showRinging (float level);

    class Strings;
    class AmpCurve;
    class Field;

private:
    void timerCallback() override;

    BuiltInEffects::SwarmGuitarProcessor& guitar;

    SoundControls::Panel stringsPanel { "Strings" }, ampPanel { "Amp" }, strumPanel { "Strum" }, layersPanel { "Layers" };
    std::unique_ptr<Strings> strings;
    std::unique_ptr<AmpCurve> curve;
    std::unique_ptr<Field> field;

    SoundControls::Knob count { "Strings" }, detune { "Detune" }, pick { "Pick" }, brightness { "Bright" };
    SoundControls::StepSelector tuning, amp;
    SoundControls::Knob drive { "Drive", SoundControls::Knob::Size::large }, tone { "Tone" }, level { "Level" };
    SoundControls::Knob strum { "Strum", SoundControls::Knob::Size::large }, strumRate { "Rate", SoundControls::Knob::Size::large },
                        sustain { "Sustain", SoundControls::Knob::Size::large }, release { "Release", SoundControls::Knob::Size::large };
    SoundControls::Knob layers { "Guitars" }, width { "Width" };
    EffectPresets::Bar presets;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SwarmGuitarEditor)
};
