#include <JuceHeader.h>
#include "../Source/AudioEdits.h"
#include "../Source/AudioEngine.h"
#include "../Source/FileOverview.h"
#include "TestUtils.h"
#include "TimeViewStripTests.h"

namespace
{
    using TestUtils::check;

    juce::MouseEvent mouse (juce::Component& c, float x, float downX, int clicks = 1)
    {
        const auto now = juce::Time::getCurrentTime();
        const juce::Point<float> at { x, 10.0f }, down { downX, 10.0f };
        return juce::MouseEvent (juce::Desktop::getInstance().getMainMouseSource(), at, juce::ModifierKeys::leftButtonModifier, 1.0f,
                                 0.0f, 0.0f, 0.0f, 0.0f, &c, &c, now, down, now, clicks, at != down);
    }

    void drag (juce::Component& c, float from, float to)
    {
        c.mouseDown (mouse (c, from, from));
        c.mouseDrag (mouse (c, to, from));
        c.mouseUp (mouse (c, to, from));
    }

    bool near (double a, double b, double within)    { return std::abs (a - b) <= within; }
}

void runTimeViewStripTests()
{
    // Five minutes of stereo at 8 kHz: a ramp up the left, silence on the right but for a burst.
    constexpr double rate = 8000.0;
    constexpr int frames = (int) (rate * 300.0);
    juce::AudioBuffer<float> b (2, frames);
    b.clear();

    for (int i = 0; i < frames; ++i)
        b.setSample (0, i, 0.8f * (float) ((i % 4000) - 2000) / 2000.0f * (i < frames / 2 ? 1.0f : 0.0f));

    AudioEngine engine;
    engine.document.reset (AudioEdits::fromBuffer (std::move (b)), rate, {}, 24);
    WaveformView view (engine);
    view.setBounds (0, 0, 1000, 400);
    FileOverview strip (engine, view);
    strip.setSize (500, FileOverview::height);
    const auto pxFrames = (double) frames / 500.0;   // file frames per strip pixel

    view.zoomToFit();
    check (juce::exactlyEqual (strip.getTotalFrames(), (double) frames) && near (strip.getViewFrame().getX(), 0.0, 0.01)
               && near (strip.getViewFrame().getRight(), 500.0, 0.5),
           "time-view strip: spans the file, and a fitted editor's frame covers the whole strip");

    view.zoomIn();
    view.zoomIn();
    auto shown = view.getVisibleFrames();
    check (near (strip.getViewFrame().getWidth(), 125.0, 0.5) && near (strip.getViewFrame().getX(), strip.xFor (shown.getStart()), 0.01),
           "time-view strip: follows the editor's Zoom + (the frame a quarter of the strip after two)");

    // Drag inside the frame: the editor scrolls by as much, keeping the zoom.
    const auto before = view.getVisibleFrames();
    const auto inside = strip.getViewFrame().getCentreX();
    drag (strip, inside, inside + 50.0f);
    shown = view.getVisibleFrames();
    check (near (shown.getStart(), before.getStart() + 50.0 * pxFrames, pxFrames) && near (shown.getLength(), before.getLength(), 1.0),
           "time-view strip: dragging the frame scrolls the editor, keeping the zoom");

    // Drag the right edge out 100 pixels: zooms out, the start stays.
    auto frame = strip.getViewFrame();
    drag (strip, frame.getRight() - 1.0f, frame.getRight() + 99.0f);
    const auto widened = view.getVisibleFrames();
    check (near (widened.getStart(), shown.getStart(), 1.0) && near (widened.getEnd(), shown.getEnd() + 100.0 * pxFrames, 2.0 * pxFrames),
           "time-view strip: dragging the frame's right edge zooms out, the left edge staying put");

    // Drag the left edge in: zooms in, the end stays.
    frame = strip.getViewFrame();
    drag (strip, frame.getX() + 1.0f, frame.getX() + 101.0f);
    const auto narrowed = view.getVisibleFrames();
    check (near (narrowed.getEnd(), widened.getEnd(), 2.0) && near (narrowed.getStart(), widened.getStart() + 100.0 * pxFrames, 2.0 * pxFrames),
           "time-view strip: dragging the frame's left edge zooms in, the right edge staying put");

    // An edge dragged past the other stops short of it: the view never turns inside out.
    frame = strip.getViewFrame();
    drag (strip, frame.getRight() - 1.0f, frame.getX() - 80.0f);
    check (view.getVisibleFrames().getLength() > 0.0 && near (view.getVisibleFrames().getStart(), narrowed.getStart(), 1.0),
           "time-view strip: an edge dragged past the other leaves a sliver, the far edge in place");

    // Click away from the frame: the editor centres there, keeping the zoom.
    view.zoomToFit();
    view.zoomIn();
    view.zoomIn();
    view.zoomIn();
    const auto span = view.getVisibleFrames().getLength();
    strip.mouseDown (mouse (strip, 400.0f, 400.0f));
    strip.mouseUp (mouse (strip, 400.0f, 400.0f));
    check (near ((view.getVisibleFrames().getStart() + view.getVisibleFrames().getLength() / 2.0), 400.0 * pxFrames, pxFrames) && near (view.getVisibleFrames().getLength(), span, 1.0),
           "time-view strip: clicking away from the frame centres the editor there");

    // The editor's own scrolling moves the frame (the scroll bar, the Hand tool).
    view.showFrames ({ 1000.0, 1000.0 + span });
    check (near (strip.getViewFrame().getX(), 1000.0 / pxFrames, 0.01), "time-view strip: follows the editor's scrolling");

    // Double-click fits the whole file.
    strip.mouseDoubleClick (mouse (strip, 10.0f, 10.0f, 2));
    check (near (view.getVisibleFrames().getStart(), 0.0, 0.5) && near (view.getVisibleFrames().getEnd(), (double) frames, pxFrames),
           "time-view strip: double-click fits the file in the editor");

    // The peaks: drawn over the loud first half, not over the silent second.
    juce::Image image (juce::Image::ARGB, 500, FileOverview::height, true, juce::SoftwareImageType());
    {
        juce::Graphics g (image);
        strip.paint (g);
    }
    const auto loud = image.getPixelAt (100, 12), quiet = image.getPixelAt (400, 12);
    check (loud != quiet, "time-view strip: the file's peaks are drawn where it is loud (" + loud.toDisplayString (true) + " there, "
                              + quiet.toDisplayString (true) + " in the silence)");
}
