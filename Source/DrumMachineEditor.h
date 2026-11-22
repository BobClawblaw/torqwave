#pragma once

#include "DrumMachine.h"
#include "SoundControls.h"
#include "EffectPresets.h"

/** Basic Drums' window. On the left a pad for each of its 30 sounds, coloured by family, lit as it
    plays; a click plays it (louder towards the pad's top) and selects it. On the right the
    selected sound: a picture of one hit (its waveform and its level falling, a line sweeping across
    as it plays), its six knobs in real units (tune in semitones, decay in ms, tone, level in dB,
    pan, drive), its effects (a filter's type, cutoff and resonance, a crush, and how much goes to
    the kit's reverb and delay) and, for the kick, its character. Along the top the kits and the
    user's own presets; the bus, the whole kit's drive, glue (with how far it turns the kit down)
    and level; and the space, the reverb's size and the delay's time and feedback. In Torqwave's
    theme, light or dark. */
class DrumMachineEditor final : public juce::AudioProcessorEditor,
                                private juce::Timer
{
public:
    explicit DrumMachineEditor (BuiltInEffects::BasicDrumsProcessor&);
    ~DrumMachineEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

    /** The sound whose settings are shown; selecting one shows its knobs and picture. */
    DrumMachine::Sound getSelected() const noexcept       { return selected; }
    void select (DrumMachine::Sound);

    /** The pads in the order shown, left to right and down; and where one is. */
    static const std::array<DrumMachine::Sound, (size_t) DrumMachine::numSounds>& padOrder();
    juce::Rectangle<int> padBounds (DrumMachine::Sound) const;

    /** What a click on a pad does: plays its sound at the velocity for the click's height
        (0 its top, 1 its bottom) and selects it. */
    void clickPad (DrumMachine::Sound, float proportionDown);

    /** The velocity a click at a height inside a pad plays: 1 at its top to about 0.3 at its bottom. */
    static float velocityAt (float proportionDown) noexcept;

    /** What the window shows, for the tests: the kit's name as shown, a knob's readout, and the
        length of the picture's hit in seconds. */
    juce::String getKitText() const;
    juce::String getKnobText (const juce::String& what) const;
    double getPictureSeconds() const noexcept              { return pictureSeconds; }

    /** Each family's colour: kick, snares and claps, hats, toms, cymbals, hand percussion. */
    static juce::Colour colourOf (DrumMachine::Sound);

    class Pad;
    class Picture;
    class GlueMeter;

private:
    void timerCallback() override;
    void refreshPicture (bool force);
    void refreshReadouts();
    void refreshFilterKnobs();
    void showSelected();

    BuiltInEffects::BasicDrumsProcessor& drums;
    DrumMachine::Sound selected = DrumMachine::Sound::kick;

    std::vector<std::unique_ptr<Pad>> pads;
    std::unique_ptr<Picture> picture;
    std::unique_ptr<GlueMeter> glueMeter;
    std::array<juce::uint32, (size_t) DrumMachine::numSounds> seenHits {};

    SoundControls::Panel padPanel { "Pads" }, soundPanel { "Kick" }, kitPanel { "Kit" }, busPanel { "Bus" }, spacePanel { "Space" };
    SoundControls::StepSelector kits, character, filter, delayTime;
    SoundControls::Knob tune { "Tune", SoundControls::Knob::Size::large }, decay { "Decay", SoundControls::Knob::Size::large },
                        tone { "Tone", SoundControls::Knob::Size::large }, level { "Level" }, pan { "Pan" }, drive { "Drive" };
    SoundControls::Knob cutoff { "Cutoff", SoundControls::Knob::Size::small }, resonance { "Reso", SoundControls::Knob::Size::small },
                        crush { "Crush", SoundControls::Knob::Size::small }, reverb { "Reverb", SoundControls::Knob::Size::small },
                        delay { "Delay", SoundControls::Knob::Size::small };
    std::array<SoundControls::ValueReadout, 11> readouts;
    SoundControls::Knob kitDrive { "Drive", SoundControls::Knob::Size::small }, glue { "Glue", SoundControls::Knob::Size::small },
                        kitLevel { "Level", SoundControls::Knob::Size::small }, reverbSize { "Size", SoundControls::Knob::Size::small },
                        feedback { "Feedback", SoundControls::Knob::Size::small };

    std::array<SoundControls::Knob*, 11> soundKnobs() noexcept
    {
        return { &tune, &decay, &tone, &level, &pan, &drive, &cutoff, &resonance, &crush, &reverb, &delay };
    }
    EffectPresets::Bar presets;

    DrumMachine::KitSettings pictured;
    bool hasPicture = false;
    double pictureSeconds = 0.0;
    bool shownModified = false;
    juce::Rectangle<int> fxLine;   // where the effects' heading is drawn

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumMachineEditor)
};
