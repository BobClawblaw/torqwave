#pragma once

#include "MultitrackView.h"
#include "TimeViewStrip.h"

/** A strip above the lanes showing the whole session in miniature: every shown track's clips
    in their colours, the cursor, and a frame around the part the lanes show. Clicking or
    dragging in it scrolls the lanes to centre there, keeping the zoom; dragging an edge of the
    frame zooms (TimeViewStrip). */
class SessionOverview final : public TimeViewStrip,
                              private juce::ChangeListener
{
public:
    SessionOverview (AudioEngine&, MultitrackView&);
    ~SessionOverview() override;

    /** Session frames across the strip: from 0 to the session's end (at least a minute, and as
        far as the lanes show). */
    double getTotalFrames() const override;

private:
    juce::Range<double> getShownFrames() const override    { return view.getVisibleFrames(); }
    void showFrames (juce::Range<double> r) override        { view.showFrames (r); }
    void showAll() override                                 { view.zoomToFit(); }
    double getCursorFrame() const override                  { return (double) view.getCursor(); }
    bool isMoving() const override                          { return engine.getPlayer().isPlaying(); }
    void paintContent (juce::Graphics&) override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    AudioEngine& engine;
    MultitrackView& view;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SessionOverview)
};
