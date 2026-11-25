#pragma once

#include "DrumMachine.h"
#include "SoundControls.h"
#include "EffectPresets.h"

/** Basic Drums' window. On the left its 64 pads, eight across: one for each of its 30 sounds,
    coloured by family, then the 34 user pads, which play samples and have a name, a colour and a
    note of their own (a right-click on one, or Pad in its panel's header, renames it, moves it to
    another note, colours it or empties it; a sound's pad can be renamed and coloured the same way
    while its type is Sample, its note staying its own). A pad is lit as it
    plays; a click plays it, at the velocity of the keys under the window, and selects it. On the right the
    selected sound: a picture of one hit (its waveform and its level falling, a line sweeping across
    as it plays), its six knobs in real units (tune in semitones, decay in ms, tone, level in dB,
    pan, drive), its effects (a filter's type, cutoff and resonance, a crush, and how much goes to
    the kit's reverb and delay) and, for the kick, its character. Along the top the kits and the
    user's own presets; the bus, the whole kit's drive, glue (with how far it turns the kit down)
    and level; and the space, the reverb's size and the delay's time and feedback. In Torqwave's
    theme, light or dark.

    A pad whose type is Sample shows, under a shorter picture, its layers: each one's waveform, name
    and velocity range, with buttons to add, remove and reorder them and to spread them across the
    velocities; and three more controls, where the sample starts, how far its pitch bends, and
    Reverse. Sounds dropped on a pad, or on the selected sound's panel, become its layers: files
    from Finder, or a sound dragged from inside Torqwave (SoundDrops). Files beside the kits opens
    and saves a kit with its samples and looks for samples that have moved. */
class DrumMachineEditor final : public juce::AudioProcessorEditor,
                                public juce::FileDragAndDropTarget,
                                public juce::DragAndDropTarget,
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
    void select (DrumMachine::Sound s)                      { selectPad ((int) s); }

    /** The same by pad (0 to 63: a sound's is its place in DrumMachine::Sound, the user pads follow). */
    int getSelectedPad() const noexcept                    { return selectedPad; }
    void selectPad (int pad);

    /** The sounds' pads in the order shown, left to right and down (the user pads come after them, in their own order); and where a pad is. */
    static const std::array<DrumMachine::Sound, (size_t) DrumMachine::numSounds>& padOrder();
    juce::Rectangle<int> padBounds (DrumMachine::Sound s) const    { return padBounds ((int) s); }
    juce::Rectangle<int> padBounds (int pad) const;

    /** What a click on a pad does: plays it alone and selects it. It plays at the velocity the
        window's keys play at (the strip under it, set with C and V; 100 with no strip), wherever
        on the pad the click is. */
    void clickPad (DrumMachine::Sound s)     { clickPad ((int) s); }
    void clickPad (int pad);

    /** A pad's card as it reads: its name, then its notes ("Pad 31 / 82"). */
    juce::String getPadText (int pad) const;

    /** What a pad's menu does, by name: "clear" (a user pad emptied; a sound's pad given its own
        name and colour back); "colour" takes the value, and for a user pad so does "note". */
    void changeUserPad (int pad, const juce::String& what, int value = 0);

    /** Takes in the hits since the last look, as the window's timer does: lights their pads, and
        for the selected pad says the hit's velocity and lights the layer it played. */
    void showHits();

    /** For the tests: the layer the selected pad's last hit played (-1 for none, or before a hit),
        and the layer list's words about that hit ("hit 52: layer 2", "hit 20: no layer"). */
    int getLitLayer() const;
    juce::String getHitText() const;

    /** What the window shows, for the tests: the kit's name as shown, a knob's readout, and the
        length of the picture's hit in seconds. */
    juce::String getKitText() const;
    juce::String getKnobText (const juce::String& what) const;

    /** The selected sound's type as its selector shows it ("Classic", "Modern", "Sample"). */
    juce::String getTypeText() const;

    /** Sounds dropped: on a pad they go to that pad, on the selected sound's panel to it. */
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragMove (const juce::StringArray&, int x, int y) override;
    void fileDragExit (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;
    bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDragMove (const SourceDetails&) override;
    void itemDragExit (const SourceDetails&) override;
    void itemDropped (const SourceDetails&) override;

    /** The pad a drop at a place in the window goes to: the pad there, the selected one over its panel, none elsewhere. */
    std::optional<int> padAt (juce::Point<int>) const;

    /** Gives files to a pad, as a drop does, and selects it. False when none of them became a layer. */
    bool addFiles (DrumMachine::Sound s, const juce::Array<juce::File>& sounds)   { return addFiles ((int) s, sounds); }
    bool addFiles (int pad, const juce::Array<juce::File>&);

    /** For the tests: how many layers the selected sound's list shows, a row as it reads
        ("Name  1-127"), whether the list is shown at all, and the line of news along the bottom. */
    int getNumLayersShown() const;
    juce::String getLayerText (int row) const;
    bool showsLayers() const;
    juce::String getMessage() const                        { return message; }

    /** The layer list's buttons, by what they do: "remove", "up", "down", "spread" on its selected row (selectLayer). */
    void selectLayer (int row);
    void pressLayerButton (const juce::String& which);
    double getPictureSeconds() const noexcept              { return pictureSeconds; }

    /** Each family's colour: kick, snares and claps, hats, toms, cymbals, hand percussion. */
    static juce::Colour colourOf (DrumMachine::Sound);

    class Pad;
    class Picture;
    class GlueMeter;
    class Layers;

private:
    void timerCallback() override;
    void refreshPicture (bool force);
    void refreshReadouts();
    void refreshFilterKnobs();
    void showSelected();
    void say (const juce::String&);
    void setDropTarget (std::optional<int>);
    void chooseSamples();
    void showFilesMenu();
    void showPadMenu (int pad);
    void markSampledPads();
    void describePads();
    juce::Colour colourOfPad (int pad) const;
    int padAtPlace (size_t place) const;

    BuiltInEffects::BasicDrumsProcessor& drums;
    DrumMachine::Sound selected = DrumMachine::Sound::kick;   // the selected pad's sound; with a user pad selected, the last sound that was
    int selectedPad = 0;

    std::vector<std::unique_ptr<Pad>> pads;
    std::unique_ptr<Picture> picture;
    std::unique_ptr<GlueMeter> glueMeter;
    std::unique_ptr<Layers> layers;
    std::array<juce::uint32, (size_t) DrumMachine::numPads> seenHits {};

    SoundControls::Panel padPanel { "Pads" }, soundPanel { "Kick" }, kitPanel { "Kit" }, busPanel { "Bus" }, spacePanel { "Space" };
    SoundControls::StepSelector kits, character, type, filter, delayTime;
    SoundControls::Knob tune { "Tune", SoundControls::Knob::Size::large }, decay { "Decay", SoundControls::Knob::Size::large },
                        tone { "Tone", SoundControls::Knob::Size::large }, level { "Level" }, pan { "Pan" }, drive { "Drive" };
    SoundControls::Knob cutoff { "Cutoff", SoundControls::Knob::Size::small }, resonance { "Reso", SoundControls::Knob::Size::small },
                        crush { "Crush", SoundControls::Knob::Size::small }, reverb { "Reverb", SoundControls::Knob::Size::small },
                        delay { "Delay", SoundControls::Knob::Size::small };
    SoundControls::Knob start { "Start" }, bend { "Bend" };   // a Sample's own
    SoundControls::LedToggle reverse { "Reverse" };
    juce::TextButton files { "Files" }, padMenu { "Pad" };
    std::unique_ptr<juce::FileChooser> chooser;
    std::array<SoundControls::ValueReadout, 13> readouts;
    SoundControls::Knob kitDrive { "Drive", SoundControls::Knob::Size::small }, glue { "Glue", SoundControls::Knob::Size::small },
                        kitLevel { "Level", SoundControls::Knob::Size::small }, reverbSize { "Size", SoundControls::Knob::Size::small },
                        feedback { "Feedback", SoundControls::Knob::Size::small };

    std::array<SoundControls::Knob*, 13> soundKnobs() noexcept
    {
        return { &tune, &decay, &tone, &level, &pan, &drive, &cutoff, &resonance, &crush, &reverb, &delay, &start, &bend };
    }
    EffectPresets::Bar presets;

    DrumMachine::KitSettings pictured;
    bool hasPicture = false;
    double pictureSeconds = 0.0;
    bool shownModified = false;
    bool shownSample = false;                  // whether the selected sound is laid out as a Sample
    juce::uint32 samplesSeen = 0, padsSeen = 0;   // the processor's counts of its layers' and its user pads' changes when last read
    std::optional<int> dropTarget;             // the pad a drag is over
    juce::String message;                      // news for the line along the bottom, until messageUntil
    double messageUntil = 0.0;
    juce::Rectangle<int> fxLine;   // where the effects' heading is drawn

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumMachineEditor)
};
