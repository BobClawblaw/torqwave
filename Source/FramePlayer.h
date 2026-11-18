#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <mutex>

/** The Video panel's picture on Linux, where JUCE has no VideoComponent: frames decoded by the user's
    ffmpeg (Ffmpeg::find) and drawn here. It has the part of juce::VideoComponent's interface the panel
    uses, so Video::Player is one or the other.

    Stopped, a seek decodes the one frame there. Playing, ffmpeg decodes from the play position into a
    pipe and each frame is shown at its time; the clock starts with the first frame, so (like the other
    systems' players) the picture starts a moment after play(), which Video::Follower learns. No sound:
    the panel plays the picture silent. */
class FramePlayer final : public juce::Component,
                          private juce::Thread,
                          private juce::AsyncUpdater
{
public:
    explicit FramePlayer (bool useNativeControls = false);
    ~FramePlayer() override;

    juce::Result load (const juce::File&);
    void closeVideo();
    bool isVideoOpen() const noexcept                  { return open; }

    double getVideoDuration() const noexcept           { return duration; }
    juce::Rectangle<int> getVideoNativeSize() const    { return { nativeWidth, nativeHeight }; }

    void play();
    void stop();
    bool isPlaying() const noexcept                    { return playing; }

    void setPlayPosition (double seconds);
    double getPlayPosition() const;

    /** The picture is silent: kept for the VideoComponent interface. */
    void setAudioVolume (float) noexcept               {}
    float getAudioVolume() const noexcept              { return 0.0f; }

    std::function<void (const juce::String&)> onErrorOccurred;

    void paint (juce::Graphics&) override;

private:
    void run() override;
    void handleAsyncUpdate() override;

    /** Decodes from `from`: one frame, or (playing) frames until told otherwise. */
    void decode (double from, bool oneFrame);
    juce::StringArray command (double from, bool oneFrame) const;

    juce::File ffmpeg, file;
    bool open = false;
    double duration = 0.0, frameRate = 25.0;
    int nativeWidth = 0, nativeHeight = 0, width = 0, height = 0;   // as decoded: at most 1280 wide

    // What the decoding thread is asked to do: a request number that changes with each.
    std::atomic<int> request { 0 };
    std::atomic<bool> playing { false };
    std::atomic<double> requestedPosition { 0.0 };

    // The play clock: the position at clockStart (ms), once the first frame is up; until then, at rest.
    std::atomic<double> clockPosition { 0.0 };
    std::atomic<double> clockStart { -1.0 };

    std::mutex frameLock;
    juce::Image frame, shown;
    juce::String error;

    juce::WaitableEvent wake;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FramePlayer)
};
