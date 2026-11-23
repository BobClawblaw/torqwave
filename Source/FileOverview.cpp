#include "FileOverview.h"
#include "Theme.h"

FileOverview::FileOverview (AudioEngine& e, WaveformView& v) : engine (e), view (v)
{
    engine.document.addChangeListener (this);
    hadAudio = engine.document.hasAudio();
}

FileOverview::~FileOverview()
{
    engine.document.removeChangeListener (this);
}

double FileOverview::getTotalFrames() const
{
    return juce::jmax (1.0, (double) engine.document.getNumFrames());
}

double FileOverview::getCursorFrame() const
{
    auto& player = engine.getPlayer();
    return (double) (player.isPlaying() ? player.getHeardFrame() : view.getCursor());
}

void FileOverview::paintContent (juce::Graphics& g)
{
    const auto version = engine.document.getVersion();
    const auto resets = engine.document.getResetCount();

    if (! peaksValid || version != peaksVersion || resets != peaksResetCount || peaks.getWidth() != juce::jmax (1, getWidth()))
    {
        peaksValid = true;
        peaksVersion = version;
        peaksResetCount = resets;

        // A software image: Direct2D's, on Windows, holds what is drawn until the Graphics goes.
        peaks = juce::Image (juce::Image::ARGB, juce::jmax (1, getWidth()), juce::jmax (1, getHeight()), true, juce::SoftwareImageType());
        juce::Graphics pg (peaks);
        const auto snapshot = engine.document.getSnapshot();
        const auto frames = snapshot->getNumFrames();
        const auto channels = snapshot->getNumChannels();
        const auto mid = (float) getHeight() * 0.5f, half = (float) getHeight() * 0.5f - 3.0f;
        pg.setColour (Theme::waveform.withAlpha (0.75f));

        for (int x = 0; frames > 0 && x < peaks.getWidth(); ++x)
        {
            const auto a = (juce::int64) ((double) x / peaks.getWidth() * (double) frames);
            const auto b = juce::jmin (frames, juce::jmax (a + 1, (juce::int64) ((double) (x + 1) / peaks.getWidth() * (double) frames)));

            if (a >= frames)
                break;

            juce::Range<float> peak;

            for (int ch = 0; ch < channels; ++ch)
                peak = ch == 0 ? snapshot->getPeak (ch, a, b) : peak.getUnionWith (snapshot->getPeak (ch, a, b));

            const auto y1 = mid - juce::jlimit (-1.0f, 1.0f, peak.getEnd()) * half;
            const auto y2 = mid - juce::jlimit (-1.0f, 1.0f, peak.getStart()) * half;
            pg.drawVerticalLine (x, y1, juce::jmax (y1 + 1.0f, y2));
        }
    }

    g.setColour (Theme::canvas);
    g.fillRect (getLocalBounds().reduced (0, 1));
    g.drawImageAt (peaks, 0, 0);

    // The selection, faintly, so a far-off one can be found.
    if (const auto sel = view.getSelection(); ! sel.isEmpty())
    {
        const auto x0 = xFor ((double) sel.getStart());
        g.setColour (Theme::accent.withAlpha (0.25f));
        g.fillRect (juce::Rectangle<float> (x0, 1.0f, juce::jmax (1.0f, xFor ((double) sel.getEnd()) - x0), (float) getHeight() - 2.0f));
    }
}

void FileOverview::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (const auto has = engine.document.hasAudio(); has != hadAudio)
    {
        hadAudio = has;

        if (onHasAudioChanged != nullptr)
            onHasAudioChanged();
    }

    repaint();
}
