#pragma once

#include "DrumMachine.h"
#include "SoundControls.h"

/** The Effects part of a Basic Drums pad's panel: six pages chosen by tabs, in the order a hit
    goes through them. FILTER is the pad's filter, crusher and sends, whose controls belong to the
    window (it lays them out in getPageBounds()); ENV, TRANSIENT, SATURATE, EQ and COMP are the
    pad's own effects (DrumPadEffects.h), each with its switch, a picture and its knobs. A tab's dot
    is lit while that effect is on for the pad.

    The pictures: the envelope's shape; the pad's hit before the transient shaper (grey) and after;
    the saturator's curve, output against input; the EQ's curve from 20 Hz to 20 kHz with a point
    for each band, which can be dragged (the wheel sets a bell's width, a double click puts a band
    back to 0 dB); the compressor's curve, and how far it turned the pad's last hit down. Every
    curve is a path sampled once for each pixel of the display, stroked with round joints. */
class DrumPadEffectsView final : public juce::Component
{
public:
    enum Page { filterPage, envPage, transPage, satPage, eqPage, compPage, numPages };

    explicit DrumPadEffectsView (BuiltInEffects::BasicDrumsProcessor&);

    /** The pad whose effects are shown, and its colour. */
    void show (int pad, juce::Colour);

    /** The window's timer: takes in changes made elsewhere (a preset, a kit), the compressor's
        meter, and the transient page's picture when the sound has changed. */
    void refresh();

    /** The pad's sound changed (a knob of its own, its samples): `seconds` is how long its hit is drawn for. */
    void soundChanged (double seconds);

    int getPage() const noexcept                     { return page; }
    void setPage (int);
    std::function<void()> onPageChanged;

    /** Where a page's controls go, in this component; and whether the FILTER tab's dot is lit. */
    juce::Rectangle<int> getPageBounds() const;
    void setFilterLit (bool);

    static juce::StringArray pageNames();

    /** For the tests: whether a tab's dot is lit; the switch of the page shown; a knob of it by its
        label, as it reads and turned to a value; its selector; a drag of an EQ point; the meter's words. */
    bool isLit (int page) const;
    bool isOn() const                                { return onSwitch.isOn(); }
    void clickSwitch();
    juce::StringArray getKnobLabels() const;
    juce::String getKnobText (const juce::String& label) const;
    bool turnKnob (const juce::String& label, float value);
    juce::String getSelectorText() const             { return selector.isVisible() ? selector.getText() : juce::String(); }
    void stepSelector (int by)                       { selector.step (by); }
    void dragPoint (int band, double hz, float db);
    juce::String getMeterText() const;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    struct KnobSpec
    {
        DrumMachine::Fx fx;
        juce::String label, tip;
        bool bipolar;
    };

    std::vector<KnobSpec> specs() const;
    void load();
    void layoutPage();
    juce::Rectangle<int> tabBounds (int page) const;
    juce::Rectangle<float> pictureBounds() const;
    juce::Rectangle<float> plotBounds() const;
    void renderTransient();
    void paintEnvelope (juce::Graphics&, juce::Rectangle<float>, float scale) const;
    void paintTransient (juce::Graphics&, juce::Rectangle<float>, float scale) const;
    void paintSaturation (juce::Graphics&, juce::Rectangle<float>, float scale) const;
    void paintEq (juce::Graphics&, juce::Rectangle<float>, float scale) const;
    void paintCompressor (juce::Graphics&, juce::Rectangle<float>, float scale) const;
    juce::Point<float> pointOf (int band, juce::Rectangle<float> plot) const;
    void setBand (int band, double hz, float db);

    BuiltInEffects::BasicDrumsProcessor& drums;
    int pad = 0, page = filterPage, band = 1, dragging = -1;
    juce::Colour colour;
    bool filterLit = false;
    DrumMachine::PadEffects fx;
    juce::uint32 seen = 0;

    SoundControls::LedToggle onSwitch { "On" };
    SoundControls::StepSelector selector;
    std::array<SoundControls::Knob, 5> knobs { SoundControls::Knob { "", SoundControls::Knob::Size::small }, SoundControls::Knob { "", SoundControls::Knob::Size::small },
                                               SoundControls::Knob { "", SoundControls::Knob::Size::small }, SoundControls::Knob { "", SoundControls::Knob::Size::small },
                                               SoundControls::Knob { "", SoundControls::Knob::Size::small } };
    std::array<SoundControls::ValueReadout, 5> readouts;
    std::vector<KnobSpec> shown;

    // The transient page's picture: the hit's level every half millisecond, before and after.
    std::vector<float> before, after;
    double hitSeconds = 0.3;
    bool transientStale = true;
    float meterDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumPadEffectsView)
};
