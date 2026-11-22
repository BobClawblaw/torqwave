#include <JuceHeader.h>
#include "../Source/SoundControls.h"
#include "../Source/Theme.h"
#include "TestUtils.h"
#include "SoundControlsTests.h"

namespace
{
    using TestUtils::check;

    /** A processor holding a few parameters, counting the gestures the host is told of. */
    struct Holder final : public juce::AudioProcessor,
                          private juce::AudioProcessorListener
    {
        Holder()
        {
            addParameter (cutoff = new juce::AudioParameterFloat (juce::ParameterID { "cutoff", 1 }, "Cutoff", juce::NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.25f), 1000.0f,
                                                                  juce::AudioParameterFloatAttributes().withLabel ("Hz")));
            addParameter (on = new juce::AudioParameterBool (juce::ParameterID { "on", 1 }, "On", false));
            addParameter (shape = new juce::AudioParameterChoice (juce::ParameterID { "shape", 1 }, "Shape", juce::StringArray { "Sine", "Saw", "Square" }, 0));
            addListener (this);
        }

        ~Holder() override                                         { removeListener (this); }

        void audioProcessorParameterChanged (juce::AudioProcessor*, int, float) override        { ++changes; }
        void audioProcessorChanged (juce::AudioProcessor*, const ChangeDetails&) override        {}
        void audioProcessorParameterChangeGestureBegin (juce::AudioProcessor*, int) override    { ++begins; }
        void audioProcessorParameterChangeGestureEnd (juce::AudioProcessor*, int) override      { ++ends; }

        const juce::String getName() const override                { return "Holder"; }
        void prepareToPlay (double, int) override                  {}
        void releaseResources() override                           {}
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
        double getTailLengthSeconds() const override               { return 0.0; }
        bool acceptsMidi() const override                          { return false; }
        bool producesMidi() const override                         { return false; }
        juce::AudioProcessorEditor* createEditor() override        { return nullptr; }
        bool hasEditor() const override                            { return false; }
        int getNumPrograms() override                              { return 1; }
        int getCurrentProgram() override                           { return 0; }
        void setCurrentProgram (int) override                      {}
        const juce::String getProgramName (int) override           { return {}; }
        void changeProgramName (int, const juce::String&) override {}
        void getStateInformation (juce::MemoryBlock&) override     {}
        void setStateInformation (const void*, int) override       {}

        juce::AudioParameterFloat* cutoff = nullptr;
        juce::AudioParameterBool* on = nullptr;
        juce::AudioParameterChoice* shape = nullptr;
        int begins = 0, ends = 0, changes = 0;
    };

    /** Whether a component draws something other than its background colour. */
    bool drawsSomething (juce::Component& c, juce::Colour background)
    {
        juce::Image image (juce::Image::ARGB, c.getWidth() * 2, c.getHeight() * 2, true);
        {
            juce::Graphics g (image);
            g.fillAll (background);
            g.addTransform (juce::AffineTransform::scale (2.0f));
            c.paintEntireComponent (g, false);
        }

        int different = 0;

        for (int y = 0; y < image.getHeight(); y += 2)
            for (int x = 0; x < image.getWidth(); x += 2)
                different += image.getPixelAt (x, y) != background ? 1 : 0;

        return different > image.getWidth() * image.getHeight() / 200;
    }
}

void runSoundControlsTests()
{
    using namespace SoundControls;

    // Values read in their units, as a sound designer writes them.
    check (formatValue (440.0f, "Hz") == "440 Hz" && formatValue (1200.0f, "Hz") == "1.20 kHz" && formatValue (12500.0f, "Hz") == "12.5 kHz"
               && formatValue (55.5f, "Hz") == "55.5 Hz" && formatValue (-6.0f, "dB") == "-6.0 dB" && formatValue (-120.0f, "dB") == "-inf dB"
               && formatValue (12.0f, "ms") == "12.0 ms" && formatValue (250.0f, "ms") == "250 ms" && formatValue (1500.0f, "ms") == "1.50 s"
               && formatValue (35.2f, "%") == "35 %" && formatValue (7.0f, "st") == "+7 st" && formatValue (-12.0f, "ct") == "-12 ct"
               && formatValue (0.0f, "st") == "0 st" && formatValue (-40.0f, "+%") == "-40 %",
           "controls: values read in their units (Hz and kHz, dB and -inf, ms and s, %, semitones and cents signed)");

    // Dragging: 250 points for the whole range, a tenth as fast with Shift, held to the ends.
    check (juce::exactlyEqual (dragged (0.5f, 125.0f, false), 1.0f) && juce::exactlyEqual (dragged (0.5f, -500.0f, false), 0.0f)
               && std::abs (dragged (0.5f, 125.0f, true) - 0.55f) < 1e-6f && std::abs (dragged (0.2f, 25.0f, false) - 0.3f) < 1e-6f,
           "controls: a drag turns a knob its whole range in 250 points, a tenth as fast with Shift, and stops at the ends");

    check (sourceFromDragText (modDragText (7)) == 7 && sourceFromDragText (juce::var ("mod:")) == -1 && sourceFromDragText (juce::var ("file:3")) == -1
               && sourceColour (0) != sourceColour (4) && sourceColour (4) != sourceColour (10),
           "controls: a modulation source travels in a drag as its index, other drags are refused, and sources have their own colours");

    Holder holder;

    // A knob follows its parameter and sets it with a gesture the host sees; double-click's reset goes back to the default.
    {
        Knob knob ("Cutoff");
        knob.setBounds (0, 0, 56, 70);
        knob.attach (holder.cutoff);
        const auto before = holder.begins;
        knob.link().begin();
        knob.setNormalised (dragged (knob.getNormalised(), 50.0f, false));
        knob.link().end();
        const auto moved = holder.cutoff->get();
        knob.resetToDefault();
        const auto reset = holder.cutoff->get();
        *holder.cutoff = 5000.0f;
        knob.refresh();
        check (moved > 1000.0f && std::abs (reset - 1000.0f) < 0.5f && holder.begins == before + 2 && holder.ends == holder.begins
                   && knob.getValueText() == "5.00 kHz" && knob.getLabel() == "Cutoff",
               "controls: a knob turns its parameter inside a gesture, resets to its default, reads the parameter back and shows it in kHz");

        // A modulation source dropped on it is handed on; other drags are not taken.
        int dropped = -1;
        knob.onModulationDropped = [&dropped] (int s) { dropped = s; };
        juce::Component source;
        const juce::DragAndDropTarget::SourceDetails mod (modDragText (5), &source, {}), other (juce::var ("file:x"), &source, {});
        const auto interested = knob.isInterestedInDragSource (mod), notInterested = knob.isInterestedInDragSource (other);
        knob.itemDropped (mod);
        check (interested && ! notInterested && dropped == 5, "controls: a knob takes a modulation source dropped on it, and only that");

        // Rings: drawn, and kept when set again the same.
        knob.setRings ({ { sourceColour (4), 0.3f, 2 }, { sourceColour (0), -0.2f, 5 } });
        knob.setLiveValue (0.6f);
        check (knob.getRings().size() == 2 && knob.getRings()[1].slot == 5, "controls: a knob holds the modulation rings it is given");
    }

    // The toggle and the selector set their parameters, the selector wrapping round both ways.
    {
        LedToggle toggle ("On");
        toggle.attach (holder.on);
        toggle.setOn (true);
        const auto lit = holder.on->get() && toggle.isOn();
        *holder.on = false;
        toggle.refresh();

        StepSelector selector;
        selector.attach (holder.shape);
        selector.step (-1);
        const auto wrappedBack = holder.shape->getIndex();
        selector.step (1);
        selector.step (1);
        check (lit && ! toggle.isOn() && wrappedBack == 2 && holder.shape->getIndex() == 1 && selector.getText() == "Saw",
               "controls: the LED toggle sets and follows its parameter; the selector steps through the choices, wrapping round");
    }

    // Everything draws, in a dark and a light theme, at twice the scale.
    {
        const auto before = Theme::current();
        auto drawsAll = true;

        for (auto themeName : { "Dark", "Light", "High Contrast" })
        {
            Theme::apply (Theme::make (themeName, "Blue"));
            Knob small ("Res", Knob::Size::small), normal ("Cutoff"), large ("Macro", Knob::Size::large);
            small.setBounds (0, 0, 40, 52);
            normal.setBounds (0, 0, 56, 70);
            large.setBounds (0, 0, 80, 96);
            normal.attach (holder.cutoff);
            normal.setRings ({ { sourceColour (4), 0.3f, 0 } });
            LedToggle toggle ("Sync");
            toggle.setBounds (0, 0, 60, 20);
            toggle.setOn (true, false);
            StepSelector selector;
            selector.setChoices ({ "Ladder 24", "SVF" });
            selector.setBounds (0, 0, 100, 20);
            ValueReadout readout;
            readout.setText ("8 voices");
            readout.setBounds (0, 0, 70, 20);
            ModSourceChip chip (4, "LFO 1");
            chip.setBounds (0, 0, 70, 20);
            Panel panel ("Oscillator 1");
            panel.setBounds (0, 0, 200, 120);
            juce::Component* all[] = { &small, &normal, &large, &toggle, &selector, &readout, &chip, &panel };

            for (auto* c : all)
                drawsAll = drawsAll && drawsSomething (*c, Theme::base);
        }

        Theme::apply (before);
        check (drawsAll, "controls: knobs of each size, rings, the toggle, selector, readout, source chip and panel draw in Dark, Light and High Contrast at 2x");
    }
}
