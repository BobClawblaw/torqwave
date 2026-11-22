#pragma once

#include <JuceHeader.h>

/** The controls Torqwave's instrument windows are built from, in the app's theme: a rotary knob
    with its value arc, rings for the modulation reaching it and a readout in real units; an LED
    toggle; a stepped selector; a value readout; a draggable modulation source; and the panel a
    section sits on. Each can follow a hosted parameter (its gestures reported, so automation
    records them) or stand alone with callbacks. They poll their parameter when refresh() is called
    (the window's timer), never from the audio thread. Everything is drawn as paths, so it stays
    sharp on any display scale. */
namespace SoundControls
{
    //==============================================================================
    /** Text for a plain value in its unit, as a sound designer reads it: "440 Hz", "1.20 kHz",
        "-6.0 dB", "-inf dB", "12 ms", "1.50 s", "35 %", "+7 st", "-12 ct"; other units as the
        number and the unit. */
    juce::String formatValue (float plain, const juce::String& unit);

    /** Where a vertical drag of `pixels` (up is positive) takes a normalised value that started at
        `start`: the whole range in `pixelsForRange` points, a tenth as fast with `fine`, kept in
        0 to 1. */
    float dragged (float start, float pixels, bool fine, float pixelsForRange = 250.0f) noexcept;

    /** The colour for a modulation source, by its index in the instrument's list of sources; the
        same in every theme (they mark meaning, like the track colours). */
    juce::Colour sourceColour (int index);

    /** The text a drag of a modulation source carries ("mod:<index>"), and back (-1 if it isn't one). */
    juce::String modDragText (int sourceIndex);
    int sourceFromDragText (const juce::var& description);

    //==============================================================================
    /** A link to one hosted parameter: reads its value and sets it the way a host expects, a
        change wrapped in a gesture. Stand-alone (no parameter) it holds its own value. */
    class ParameterLink
    {
    public:
        ParameterLink() = default;
        explicit ParameterLink (juce::RangedAudioParameter* p) : parameter (p) {}

        juce::RangedAudioParameter* get() const noexcept             { return parameter; }
        float getNormalised() const;
        float getDefaultNormalised() const;
        float getPlain() const;
        float toPlain (float normalised) const;
        juce::String getUnit() const;

        void begin();
        void set (float normalised);
        void end();

        /** Stand-alone: its value, default and the range it maps to. */
        void setStandalone (juce::NormalisableRange<float> range, float defaultPlain, const juce::String& unit);

    private:
        juce::RangedAudioParameter* parameter = nullptr;
        juce::NormalisableRange<float> ownRange { 0.0f, 1.0f };
        float own = 0.0f, ownDefault = 0.0f;
        juce::String ownUnit;
        bool inGesture = false;
    };

    //==============================================================================
    /** One modulation reaching a knob: its source's colour, its depth as a share of the knob's
        range (-1 to 1) and the slot it lives in, for the callbacks. */
    struct ModRing
    {
        juce::Colour colour;
        float depth = 0.0f;
        int slot = -1;
    };

    /** A rotary knob over 270 degrees. Drag up or right to turn it (Shift for fine), use the wheel,
        double-click (or Alt-click) to go back to the default; right-click for typing a value and
        removing modulation. The value arc runs from the bottom of the range, or from the middle
        for a bipolar knob. Modulation shows as coloured arcs around it, one a slot, from the value
        to where the slot's full depth takes it, and a dot where the modulation has it now; drag on
        the rings to change the top one's depth. Drop a modulation source on it to route that source
        here. Its name sits under it, its value in its place while it is turned or hovered. */
    class Knob : public juce::Component,
                 public juce::DragAndDropTarget,
                 public juce::SettableTooltipClient
    {
    public:
        enum class Size { small, normal, large };

        Knob (const juce::String& name, Size = Size::normal);

        void attach (juce::RangedAudioParameter*);
        ParameterLink& link() noexcept                               { return parameter; }

        void setBipolar (bool b)                                     { bipolar = b; repaint(); }
        void setColour (juce::Colour c)                              { tint = c; repaint(); }
        void setFormatter (std::function<juce::String (float)> f)    { formatter = std::move (f); repaint(); }
        void setSize (Size s)                                        { size = s; repaint(); }

        /** The modulation reaching it, and where the modulation has it now (normalised, or a
            negative number for none sounding). */
        void setRings (std::vector<ModRing>);
        void setLiveValue (float normalised);
        const std::vector<ModRing>& getRings() const noexcept        { return rings; }

        /** A source dropped on it (its index); a ring's depth changed by dragging (slot, depth);
            a ring removed from its menu (slot). */
        std::function<void (int)> onModulationDropped;
        std::function<void (int, float)> onRingDepthChanged;
        std::function<void (int)> onRingRemoved;
        std::function<void()> onValueChanged;

        /** Reads the parameter again, repainting if it moved. */
        void refresh();

        float getNormalised() const                                  { return parameter.getNormalised(); }
        void setNormalised (float);
        void resetToDefault();
        juce::String getValueText() const;
        const juce::String& getLabel() const noexcept                { return label; }
        void setLabel (const juce::String& l)                        { if (l != label) { label = l; setTitle (l); repaint(); } }

        /** Whether the readout shows the value (turning or hovered) rather than the name. */
        bool isShowingValue() const noexcept                         { return dragging || hovered; }

        /** The dial's square inside the bounds (the label below it). */
        juce::Rectangle<float> getDialBounds() const;

        void paint (juce::Graphics&) override;
        void mouseEnter (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

        bool isInterestedInDragSource (const SourceDetails&) override;
        void itemDragEnter (const SourceDetails&) override;
        void itemDragExit (const SourceDetails&) override;
        void itemDropped (const SourceDetails&) override;

    private:
        void showMenu();
        void typeValue();
        bool onRingBand (juce::Point<float>) const;

        juce::String label;
        Size size;
        ParameterLink parameter;
        std::function<juce::String (float)> formatter;
        std::vector<ModRing> rings;
        juce::Colour tint;
        bool bipolar = false, dragging = false, hovered = false, dropHover = false, ringDrag = false;
        float shown = -1.0f, live = -1.0f, dragStart = 0.0f, ringStart = 0.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Knob)
    };

    //==============================================================================
    /** A switch drawn as a pill with an LED and its text, lit in the accent (or its own colour)
        when on. Follows a bool or choice parameter (on is anything but the first choice), or
        stands alone. */
    class LedToggle : public juce::Component,
                      public juce::SettableTooltipClient
    {
    public:
        explicit LedToggle (const juce::String& text);

        void attach (juce::RangedAudioParameter*);
        void setOn (bool, bool notify = true);
        bool isOn() const noexcept                                   { return on; }
        void setColour (juce::Colour c)                              { tint = c; repaint(); }
        void setText (const juce::String& t)                         { text = t; repaint(); }

        std::function<void (bool)> onToggle;

        void refresh();
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseEnter (const juce::MouseEvent&) override          { hovered = true; repaint(); }
        void mouseExit (const juce::MouseEvent&) override           { hovered = false; repaint(); }

    private:
        juce::String text;
        ParameterLink parameter;
        juce::Colour tint;
        bool on = false, hovered = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LedToggle)
    };

    //==============================================================================
    /** A choice shown as its text between two arrows: the arrows (or the wheel) step through the
        choices, wrapping round; a click on the text opens them all as a menu, in sections if
        given. Follows a choice parameter or stands alone. */
    class StepSelector : public juce::Component,
                         public juce::SettableTooltipClient
    {
    public:
        StepSelector();

        void attach (juce::AudioParameterChoice*);
        void setChoices (const juce::StringArray&);
        /** Menu sections: a heading before the choice at each index. */
        void setSections (std::vector<std::pair<int, juce::String>> s) { sections = std::move (s); }
        void setIndex (int, bool notify = true);
        int getIndex() const noexcept                                { return index; }
        juce::String getText() const                                 { return choices[index]; }
        void step (int by);
        void setColour (juce::Colour c)                              { tint = c; repaint(); }

        std::function<void (int)> onChange;

        void refresh();
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override           { hoverPart = 0; repaint(); }

    private:
        int partAt (juce::Point<float>) const;   // -1 left arrow, 1 right arrow, 0 the text
        void showMenu();

        juce::StringArray choices;
        std::vector<std::pair<int, juce::String>> sections;
        juce::AudioParameterChoice* parameter = nullptr;
        juce::Colour tint;
        int index = 0, hoverPart = 0;
        float wheelAccumulator = 0.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepSelector)
    };

    //==============================================================================
    /** A small inset well showing a value in the accent's light tone, like an instrument's LCD. */
    class ValueReadout : public juce::Component
    {
    public:
        ValueReadout() = default;
        void setText (const juce::String&);
        const juce::String& getText() const noexcept                 { return text; }
        void paint (juce::Graphics&) override;

    private:
        juce::String text;
    };

    //==============================================================================
    /** A modulation source that can be dragged onto a knob: a chip in the source's colour with a
        cross of arrows. Its window must be a juce::DragAndDropContainer. */
    class ModSourceChip : public juce::Component,
                          public juce::SettableTooltipClient
    {
    public:
        ModSourceChip (int sourceIndex, const juce::String& text);

        int getSourceIndex() const noexcept                          { return source; }
        void setHighlighted (bool h)                                 { highlighted = h; repaint(); }

        void paint (juce::Graphics&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseEnter (const juce::MouseEvent&) override          { hovered = true; repaint(); }
        void mouseExit (const juce::MouseEvent&) override           { hovered = false; repaint(); }

    private:
        int source;
        juce::String text;
        bool hovered = false, highlighted = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModSourceChip)
    };

    //==============================================================================
    /** A section of an instrument's window: a raised card with its title in small capitals along
        the top, a thin accent mark before it, and room on the right of the title strip for a
        few header controls (laid out by the owner). */
    class Panel : public juce::Component
    {
    public:
        explicit Panel (const juce::String& title);

        void setTitle (const juce::String& t)                        { title = t; repaint(); }
        void setAccent (juce::Colour c)                              { accentColour = c; repaint(); }
        void setDimmed (bool d)                                      { dimmed = d; repaint(); }

        static constexpr int titleHeight = 24;

        /** Inside the title strip, right of the title; and the area under it. */
        juce::Rectangle<int> getHeaderBounds() const;
        juce::Rectangle<int> getContentBounds() const;

        void paint (juce::Graphics&) override;

    private:
        juce::String title;
        juce::Colour accentColour;
        bool dimmed = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Panel)
    };

    /** Draws a display well (a wavetable, a curve): the inset colour, a soft inner edge and a faint
        grid of `columns` by `rows`. */
    void drawDisplayWell (juce::Graphics&, juce::Rectangle<float>, int columns = 4, int rows = 2);

    /** Strokes a curve the way the displays draw one: a soft glow under a crisp line, and a fill
        under it to the bottom of `area` fading out. */
    void drawGlowingCurve (juce::Graphics&, const juce::Path&, juce::Colour, juce::Rectangle<float> area, bool fill = true);
}
