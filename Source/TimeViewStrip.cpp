#include "TimeViewStrip.h"
#include "Theme.h"

TimeViewStrip::TimeViewStrip()
{
    startTimerHz (15);
}

TimeViewStrip::~TimeViewStrip() = default;

juce::int64 TimeViewStrip::frameAt (float x) const
{
    return (juce::int64) std::llround ((double) x / juce::jmax (1, getWidth()) * getTotalFrames());
}

float TimeViewStrip::xFor (double frame) const
{
    return (float) (frame / juce::jmax (1.0, getTotalFrames()) * getWidth());
}

juce::Rectangle<float> TimeViewStrip::getViewFrame() const
{
    const auto shown = getShownFrames();
    const auto x0 = xFor (shown.getStart());
    return { x0, 0.5f, juce::jmax (3.0f, xFor (shown.getEnd()) - x0), (float) getHeight() - 1.0f };
}

void TimeViewStrip::paint (juce::Graphics& g)
{
    g.fillAll (Theme::base);
    paintContent (g);

    const auto frame = getViewFrame();
    g.setColour (Theme::overlay.withAlpha (grab == Grab::none ? 0.08f : 0.14f));
    g.fillRect (frame);
    g.setColour (Theme::overlay.withAlpha (0.6f));
    g.drawRect (frame, 1.0f);

    // The edges that zoom, marked while one is held.
    if (grab == Grab::start || grab == Grab::end)
    {
        g.setColour (Theme::accent);
        const auto x = grab == Grab::start ? frame.getX() : frame.getRight() - 2.0f;
        g.fillRect (juce::Rectangle<float> (x, frame.getY(), 2.0f, frame.getHeight()));
    }

    g.setColour (juce::Colour (0xfff2cc60));
    g.drawVerticalLine ((int) xFor (getCursorFrame()), 0.0f, (float) getHeight());
}

TimeViewStrip::Grab TimeViewStrip::grabAt (float x) const
{
    const auto frame = getViewFrame();
    const auto toStart = std::abs (x - frame.getX()), toEnd = std::abs (x - frame.getRight());

    // In a narrow frame, inside is for moving: its edges grab from just outside.
    const auto narrow = frame.getWidth() < edgeReach * 3.0f;
    const auto inside = x >= frame.getX() && x <= frame.getRight();

    if (! (narrow && inside))
    {
        if (toEnd <= edgeReach && toEnd <= toStart)
            return Grab::end;

        if (toStart <= edgeReach)
            return Grab::start;
    }

    return inside ? Grab::move : Grab::none;
}

void TimeViewStrip::mouseMove (const juce::MouseEvent& e)
{
    switch (grabAt (e.position.x))
    {
        case Grab::start:
        case Grab::end:   setMouseCursor (juce::MouseCursor::LeftRightResizeCursor); break;
        case Grab::move:  setMouseCursor (juce::MouseCursor::DraggingHandCursor); break;
        case Grab::none:  setMouseCursor (juce::MouseCursor::NormalCursor); break;
    }
}

void TimeViewStrip::mouseDown (const juce::MouseEvent& e)
{
    totalAtGrab = getTotalFrames();
    shownAtGrab = getShownFrames();
    grab = grabAt (e.position.x);

    if (grab == Grab::none)
    {
        // Elsewhere: centre the view there, keeping the zoom, and hold the frame by its middle.
        grab = Grab::move;
        grabOffset = shownAtGrab.getLength() / 2.0;
        showFrames (shownAtGrab.movedToStartAt ((double) frameAt (e.position.x) - grabOffset));
        shownAtGrab = getShownFrames();
        grabOffset = (double) frameAt (e.position.x) - shownAtGrab.getStart();
    }
    else
    {
        grabOffset = (double) frameAt (e.position.x) - shownAtGrab.getStart();
    }

    repaint();
}

void TimeViewStrip::mouseDrag (const juce::MouseEvent& e)
{
    // Frames from where the mouse is, across the timeline as it was when the drag began (the
    // timeline may grow as the view goes past its end).
    const auto at = juce::jlimit (0.0, totalAtGrab, (double) e.position.x / juce::jmax (1, getWidth()) * totalAtGrab);
    const auto minLength = juce::jmax (16.0, totalAtGrab / 1.0e6);

    switch (grab)
    {
        case Grab::move:
            showFrames (shownAtGrab.movedToStartAt (at - grabOffset));
            break;

        case Grab::start:
            showFrames ({ juce::jmin (at, shownAtGrab.getEnd() - minLength), shownAtGrab.getEnd() });
            break;

        case Grab::end:
            showFrames ({ shownAtGrab.getStart(), juce::jmax (at, shownAtGrab.getStart() + minLength) });
            break;

        case Grab::none:
            break;
    }

    repaint();
}

void TimeViewStrip::mouseUp (const juce::MouseEvent& e)
{
    grab = Grab::none;
    mouseMove (e);
    repaint();
}

void TimeViewStrip::mouseDoubleClick (const juce::MouseEvent&)
{
    showAll();
    repaint();
}

void TimeViewStrip::timerCallback()
{
    // Follows the view's scrolling and zoom, and the playhead while playing.
    if (const auto shown = getShownFrames(); shown != lastShown || isMoving())
    {
        lastShown = shown;
        repaint();
    }
}
