#include "FramePlayer.h"
#include "Ffmpeg.h"

namespace
{
    double now()   { return juce::Time::getMillisecondCounterHiRes(); }

    /** Reads exactly size bytes, or fewer at the end of the stream. */
    int readFully (juce::ChildProcess& process, void* dest, int size)
    {
        auto* bytes = static_cast<char*> (dest);
        int got = 0;

        while (got < size)
        {
            const auto n = process.readProcessOutput (bytes + got, size - got);

            if (n <= 0)
                break;

            got += n;
        }

        return got;
    }
}

FramePlayer::FramePlayer (bool) : juce::Thread ("Video frames")
{
    setOpaque (true);
}

FramePlayer::~FramePlayer()
{
    closeVideo();
}

juce::Result FramePlayer::load (const juce::File& f)
{
    closeVideo();
    ffmpeg = Ffmpeg::find();

    if (ffmpeg == juce::File())
        return juce::Result::fail ("showing it needs ffmpeg, which isn't installed");

    const auto picture = Ffmpeg::probePicture (ffmpeg, f);

    if (picture.width <= 0 || picture.height <= 0 || picture.duration <= 0.0)
        return juce::Result::fail ("ffmpeg found no picture in it");

    file = f;
    nativeWidth = picture.width;
    nativeHeight = picture.height;
    duration = picture.duration;
    frameRate = picture.frameRate > 0.0 && picture.frameRate <= 240.0 ? picture.frameRate : 25.0;

    // Decoded at most 1280 wide, in even sizes (as ffmpeg's scaler wants for most picture formats).
    const auto scale = juce::jmin (1.0, 1280.0 / nativeWidth);
    width = juce::jmax (2, (int) std::lround (nativeWidth * scale / 2.0) * 2);
    height = juce::jmax (2, (int) std::lround (nativeHeight * scale / 2.0) * 2);

    playing = false;
    clockPosition = 0.0;
    clockStart = -1.0;
    requestedPosition = 0.0;
    ++request;
    open = true;
    startThread();
    wake.signal();
    return juce::Result::ok();
}

void FramePlayer::closeVideo()
{
    signalThreadShouldExit();
    wake.signal();
    stopThread (5000);
    cancelPendingUpdate();
    open = false;
    playing = false;

    const std::scoped_lock lock (frameLock);
    frame = {};
    shown = {};
    error.clear();
}

void FramePlayer::play()
{
    if (! open || playing)
        return;

    requestedPosition = clockPosition.load();
    clockStart = -1.0;
    playing = true;
    ++request;
    wake.signal();
}

void FramePlayer::stop()
{
    if (! playing)
        return;

    clockPosition = getPlayPosition();
    clockStart = -1.0;
    playing = false;
    requestedPosition = clockPosition.load();
    ++request;
    wake.signal();
}

void FramePlayer::setPlayPosition (double seconds)
{
    if (! open)
        return;

    seconds = juce::jlimit (0.0, duration, seconds);
    clockPosition = seconds;
    clockStart = -1.0;   // playing, the clock starts again with the first frame from here
    requestedPosition = seconds;
    ++request;
    wake.signal();
}

double FramePlayer::getPlayPosition() const
{
    const auto start = clockStart.load();

    if (! playing || start < 0.0)
        return clockPosition;

    return juce::jmin (duration, clockPosition + (now() - start) / 1000.0);
}

juce::StringArray FramePlayer::command (double from, bool oneFrame) const
{
    juce::StringArray args { ffmpeg.getFullPathName(), "-hide_banner", "-nostdin", "-loglevel", "error",
                             "-ss", juce::String (from, 3), "-i", file.getFullPathName(), "-map", "0:v:0", "-an", "-sn", "-dn" };

    auto filter = "scale=" + juce::String (width) + ":" + juce::String (height);

    if (oneFrame)
        args.addArray ({ "-frames:v", "1" });
    else
        filter << ",fps=" << juce::String (frameRate, 6);   // a frame every 1/frameRate seconds, whatever the file's timing

    args.addArray ({ "-vf", filter, "-pix_fmt", "bgra", "-f", "rawvideo", "pipe:1" });
    return args;
}

void FramePlayer::decode (double from, bool oneFrame)
{
    const auto asked = request.load();
    const auto changed = [this, asked] { return threadShouldExit() || request.load() != asked; };
    const auto frameBytes = width * height * 4;
    juce::HeapBlock<char> buffer ((size_t) frameBytes);

    // One frame past the last has nothing there: the last second's frames are read and the last kept.
    for (auto attempt = 0; attempt < 2; ++attempt)
    {
        juce::ChildProcess process;

        if (! process.start (command (from, oneFrame && attempt == 0), juce::ChildProcess::wantStdOut))
        {
            const std::scoped_lock lock (frameLock);
            error = "ffmpeg could not be started";
            triggerAsyncUpdate();
            return;
        }

        int frames = 0;
        juce::Image last;

        while (! changed())
        {
            if (readFully (process, buffer, frameBytes) < frameBytes)
                break;

            juce::Image image (juce::Image::ARGB, width, height, false);
            {
                const juce::Image::BitmapData data (image, juce::Image::BitmapData::writeOnly);

                for (int y = 0; y < height; ++y)
                    std::memcpy (data.getLinePointer (y), buffer + (size_t) y * (size_t) width * 4, (size_t) width * 4);
            }

            if (oneFrame)
            {
                last = image;
                ++frames;
                continue;
            }

            // Playing: the clock starts with the first frame, and each later one waits for its time.
            if (frames == 0)
            {
                clockPosition = from;
                clockStart = now();
            }
            else
            {
                const auto due = clockStart.load() + frames * 1000.0 / frameRate;

                while (! changed() && now() < due)
                    wake.wait (juce::jmax (1, (int) (due - now())));

                if (changed())
                    break;
            }

            {
                const std::scoped_lock lock (frameLock);
                frame = image;
            }

            triggerAsyncUpdate();
            ++frames;
        }

        process.kill();

        if (changed())
            return;

        if (oneFrame)
        {
            if (last.isValid())
            {
                {
                    const std::scoped_lock lock (frameLock);
                    frame = last;
                }

                triggerAsyncUpdate();
                return;
            }

            from = juce::jmax (0.0, duration - 1.0);   // nothing at `from`: the end's frames instead
            continue;
        }

        // Played to the end: it stops on its last frame, as the other systems' players do.
        if (! changed())
        {
            clockPosition = frames > 0 ? juce::jmin (duration, from + frames / frameRate) : from;
            clockStart = -1.0;
            playing = false;
        }

        return;
    }
}

void FramePlayer::run()
{
    auto done = -1;

    while (! threadShouldExit())
    {
        const auto asked = request.load();

        if (asked == done)
        {
            wake.wait (200);
            continue;
        }

        done = asked;
        decode (requestedPosition.load(), ! playing.load());
    }
}

void FramePlayer::handleAsyncUpdate()
{
    juce::String failure;
    {
        const std::scoped_lock lock (frameLock);

        if (frame.isValid())
            shown = std::exchange (frame, {});

        failure = std::exchange (error, {});
    }

    if (failure.isNotEmpty() && onErrorOccurred != nullptr)
        onErrorOccurred (failure);

    repaint();
}

void FramePlayer::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    if (shown.isValid())
    {
        // The picture's own shape (the decoded size can round it), fitted in the middle.
        const auto area = juce::Rectangle<float> (0.0f, 0.0f, (float) nativeWidth, (float) nativeHeight)
                              .transformedBy (juce::RectanglePlacement (juce::RectanglePlacement::centred)
                                                  .getTransformToFit ({ 0.0f, 0.0f, (float) nativeWidth, (float) nativeHeight },
                                                                      getLocalBounds().toFloat()));
        g.setImageResamplingQuality (juce::Graphics::mediumResamplingQuality);
        g.drawImage (shown, area);
    }
}
