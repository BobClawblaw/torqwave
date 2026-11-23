#include "SessionOverview.h"
#include "Theme.h"
#include "TrackColours.h"

SessionOverview::SessionOverview (AudioEngine& e, MultitrackView& v) : engine (e), view (v)
{
    engine.session.addChangeListener (this);
}

SessionOverview::~SessionOverview()
{
    engine.session.removeChangeListener (this);
}

double SessionOverview::getTotalFrames() const
{
    const auto s = engine.session.getState();
    return juce::jmax ((double) s->getLength(), s->sampleRate * 60.0, view.getVisibleFrames().getEnd());
}

void SessionOverview::paintContent (juce::Graphics& g)
{
    const auto s = engine.session.getState();
    std::vector<int> shown;

    for (int t = 0; t < (int) s->tracks.size(); ++t)
        if (! s->tracks[(size_t) t].hidden)
            shown.push_back (t);

    // One band per shown track, at most four pixels each, filling the strip when there are many.
    const auto band = shown.empty() ? 0.0f : juce::jmin (4.0f, (float) (getHeight() - 4) / (float) shown.size());

    for (size_t row = 0; row < shown.size(); ++row)
    {
        const auto t = shown[row];
        const auto& track = s->tracks[(size_t) t];

        for (auto& c : track.clips)
        {
            g.setColour (c.muted ? Theme::faintText : TrackColours::of (c, track, t));
            const auto x0 = xFor ((double) c.position), x1 = xFor ((double) c.getEnd());
            g.fillRect (juce::Rectangle<float> (x0, 2.0f + band * (float) row, juce::jmax (1.0f, x1 - x0), juce::jmax (1.0f, band - 1.0f)));
        }
    }
}

void SessionOverview::changeListenerCallback (juce::ChangeBroadcaster*)
{
    repaint();
}
