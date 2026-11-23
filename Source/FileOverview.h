#pragma once

#include "WaveformView.h"
#include "TimeViewStrip.h"

/** A strip above the editor's ruler showing the whole open file in miniature (its channels'
    peaks laid over each other), the cursor or playhead, and a frame around the part the editor
    shows: the same strip as the session's (TimeViewStrip), to scroll and zoom a long file. */
class FileOverview final : public TimeViewStrip,
                           private juce::ChangeListener
{
public:
    FileOverview (AudioEngine&, WaveformView&);
    ~FileOverview() override;

    /** The file's frames across the strip (at least one). */
    double getTotalFrames() const override;

    /** Called when a file with audio opens or the last one closes: the strip shows only over audio. */
    std::function<void()> onHasAudioChanged;

    void resized() override                                 { peaksValid = false; }
    void lookAndFeelChanged() override                      { peaksValid = false; repaint(); }

private:
    juce::Range<double> getShownFrames() const override    { return view.getVisibleFrames(); }
    void showFrames (juce::Range<double> r) override        { view.showFrames (r); }
    void showAll() override                                 { view.zoomToFit(); }
    double getCursorFrame() const override;
    bool isMoving() const override                          { return engine.getPlayer().isPlaying(); }
    void paintContent (juce::Graphics&) override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    AudioEngine& engine;
    WaveformView& view;
    juce::Image peaks;          // the file's peaks at the strip's size, drawn again when the file changes
    bool peaksValid = false;
    juce::uint32 peaksVersion = 0;
    int peaksResetCount = -1;
    bool hadAudio = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileOverview)
};
