#pragma once

#include <JuceHeader.h>

/** A block of text for a dialog, laid out at a set width in the dialog's message font, whose last
    line is a value that copies itself to the clipboard when clicked, with either button, and reads
    "Copied" for a moment after. */
class CopyLine final : public juce::Component,
                       private juce::Timer
{
public:
    CopyLine (juce::String textAbove, juce::String valueToCopy, int width)
        : text (std::move (textAbove)), value (std::move (valueToCopy))
    {
        setSize (width, 10);
        layOut();
    }

    /** What the last line reads now: the value, or "Copied" just after a click. */
    juce::String getShownText() const                 { return copied ? "Copied" : value; }
    bool isShowingCopied() const noexcept             { return copied; }

    /** Where the value is drawn (the last line), in the component's coordinates. */
    juce::Rectangle<int> getValueArea() const         { return { 0, valueTop, getWidth(), getHeight() - valueTop }; }

    void paint (juce::Graphics& g) override
    {
        above.draw (g, getLocalBounds().withHeight (valueTop).toFloat());
        line.draw (g, getValueArea().toFloat());
    }

    void parentHierarchyChanged() override            { layOut(); }
    void lookAndFeelChanged() override                { layOut(); }

    void mouseMove (const juce::MouseEvent& e) override
    {
        setMouseCursor (getValueArea().contains (e.getPosition()) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown() || ! getValueArea().contains (e.getPosition()))
            return;

        juce::SystemClipboard::copyTextToClipboard (value);
        copied = true;
        layOut();
        repaint();
        startTimer (1500);
    }

private:
    void layOut()
    {
        const auto font = getLookAndFeel().getAlertWindowMessageFont();
        const auto colour = findColour (juce::AlertWindow::textColourId);
        const auto width = (float) getWidth();

        juce::AttributedString a;
        a.setJustification (juce::Justification::topLeft);
        a.append (text, font, colour);
        above = juce::TextLayout();
        above.createLayout (a, width);

        juce::AttributedString v;
        v.setJustification (juce::Justification::topLeft);
        v.append (getShownText(), font, colour);
        line = juce::TextLayout();
        line.createLayout (v, width);

        valueTop = (int) std::ceil (above.getHeight()) + (int) std::ceil (font.getHeight());
        const auto height = valueTop + (int) std::ceil (line.getHeight()) + 2;

        if (height != getHeight())
            setSize (getWidth(), height);
    }

    void timerCallback() override
    {
        stopTimer();
        copied = false;
        layOut();
        repaint();
    }

    juce::String text, value;
    juce::TextLayout above, line;
    int valueTop = 0;
    bool copied = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CopyLine)
};
