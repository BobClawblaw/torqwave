#pragma once

#include <JuceHeader.h>

/** A thin strip over a whole timeline (a session, or a file in the editor), with a frame around the
    part its view shows. Drag inside the frame to scroll the view, drag either edge of the frame to
    zoom (the other edge stays), click elsewhere to centre the view there (and keep dragging to move
    it), double-click to fit the whole timeline. It follows the view's own scrolling and zoom.
    A subclass says what the timeline is, draws what is on it, and moves its view. */
class TimeViewStrip : public juce::Component,
                      private juce::Timer
{
public:
    TimeViewStrip();
    ~TimeViewStrip() override;

    static constexpr int height = 36;

    /** Timeline frames across the strip, from 0 to this. */
    virtual double getTotalFrames() const = 0;

    juce::int64 frameAt (float x) const;
    float xFor (double frame) const;

    /** The frame around the shown part, in strip coordinates. */
    juce::Rectangle<float> getViewFrame() const;

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    /** How near an edge of the frame (in pixels) a press grabs it to zoom. */
    static constexpr float edgeReach = 5.0f;

protected:
    /** The timeline frames the view shows, from its left edge to its right. */
    virtual juce::Range<double> getShownFrames() const = 0;
    /** Makes the view show these frames (scrolling and zooming as it must; it may limit them). */
    virtual void showFrames (juce::Range<double>) = 0;
    /** Fits the whole timeline in the view. */
    virtual void showAll() = 0;
    /** Where the line across the strip goes: the cursor, or the playhead while playing. */
    virtual double getCursorFrame() const = 0;
    /** Whether the strip should follow the view at the timer's pace (while playing, say). */
    virtual bool isMoving() const     { return false; }
    /** Draws what is on the timeline, under the frame and the cursor. */
    virtual void paintContent (juce::Graphics&) = 0;

private:
    void timerCallback() override;

    enum class Grab { none, move, start, end };
    Grab grabAt (float x) const;

    Grab grab = Grab::none;
    juce::Range<double> shownAtGrab;
    double totalAtGrab = 1.0;
    double grabOffset = 0.0;     // frames from the shown start to where the frame was grabbed
    juce::Range<double> lastShown;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeViewStrip)
};
