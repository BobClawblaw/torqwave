#include "FitTempoPanel.h"
#include "Theme.h"

namespace
{
    constexpr int controlHeight = 28, headerHeight = 18, summaryHeight = 44, shownRows = 12;

    juce::String bpmText (double bpm)
    {
        return juce::String (bpm, 2).trimCharactersAtEnd ("0").trimCharactersAtEnd (".");
    }
}

FitTempoPanel::FitTempoPanel (const SessionState& s, juce::Range<juce::int64> range)
    : sampleRate (juce::jmax (1.0, s.sampleRate)), beatsPerBar (s.beatsPerBar), timecode (s.timecode),
      tolerance ((juce::int64) std::llround (sampleRate / (s.timecode.isSet() ? TimeFormat::framesPerSecond (s.timecode.rate) : 25.0)))
{
    for (auto& m : s.markers)
        if (! m.isRegion())
            rows.push_back ({ m, range.isEmpty() || range.contains (m.start), std::nullopt });

    const auto addLabel = [this] (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setColour (juce::Label::textColourId, Theme::text);
        addAndMakeVisible (label);
    };

    addLabel (landOnLabel, "Land on");
    addLabel (tempoLabel, "Tempo from");
    addLabel (toLabel, "to");
    addLabel (bpmLabel, "BPM");
    addLabel (summary, {});
    summary.setJustificationType (juce::Justification::topLeft);

    landOn.addItemList ({ "Beats (one tempo)", "Bars (one tempo)", "Bar lines (a tempo map)" }, 1);
    landOn.setSelectedItemIndex (0, juce::dontSendNotification);
    landOn.onChange = [this] { refit(); };
    addAndMakeVisible (landOn);

    const auto tempo = s.tempo;
    for (auto [editor, value] : { std::pair<juce::TextEditor*, double> { &lowest, juce::jmax (20.0, std::floor (tempo * 0.8)) },
                                  std::pair<juce::TextEditor*, double> { &highest, juce::jmin (400.0, std::ceil (tempo * 1.25)) } })
    {
        editor->setInputRestrictions (7, "0123456789.");
        editor->setText (bpmText (value), false);
        editor->onTextChange = [this] { refit(); };
        addAndMakeVisible (*editor);
    }

    setSize (width, 2 * controlHeight + 8 + headerHeight + juce::jlimit (3, shownRows, (int) rows.size()) * rowHeight + summaryHeight);
    refit();
}

juce::String FitTempoPanel::getTimeText (int row) const
{
    return TimeFormat::format (rows[(size_t) row].marker.start, sampleRate, timecode);
}

juce::String FitTempoPanel::getMissText (int row) const
{
    const auto& r = rows[(size_t) row];

    if (! r.chosen || ! r.miss.has_value())
        return {};

    const auto ms = 1000.0 * (double) *r.miss / sampleRate;
    const auto frames = (double) *r.miss / (double) tolerance;
    return (ms >= 0.0 ? "+" : "") + juce::String (ms, 1) + " ms, " + juce::String (std::abs (frames), 2) + " fr";
}

void FitTempoPanel::setLandOn (Picture::LandOn l)
{
    landOn.setSelectedItemIndex ((int) l, juce::sendNotificationSync);   // refits
}

void FitTempoPanel::setTempoRange (double low, double high)
{
    lowest.setText (bpmText (low), false);
    highest.setText (bpmText (high), false);
    refit();
}

void FitTempoPanel::refit()
{
    std::vector<juce::int64> hits;

    for (auto& r : rows)
        if (r.chosen)
            hits.push_back (r.marker.start);

    const auto mode = (Picture::LandOn) juce::jmax (0, landOn.getSelectedItemIndex());
    const auto low = lowest.getText().getDoubleValue(), high = highest.getText().getDoubleValue();
    fit = Picture::fitTempo (hits, sampleRate, beatsPerBar, mode, low, high, tolerance);

    size_t k = 0;

    for (auto& r : rows)
        r.miss = r.chosen && fit.isValid() ? std::optional<juce::int64> (fit.misses[k++]) : std::nullopt;

    juce::String text;

    if (hits.empty())
        text = "Choose the markers that should land on the beat.";
    else if (low < 20.0 || high > 400.0 || low > high)
        text = "The tempos to look between run from 20 to 400 BPM, the lower first.";
    else if (! fit.isValid())
        text = "No tempo fits these hits.";
    else
    {
        auto worst = (juce::int64) 0;

        for (auto m : fit.misses)
            worst = juce::jmax (worst, std::abs (m));

        text = mode == Picture::LandOn::barLines
                   ? "A tempo map from " + bpmText (fit.tempo) + " BPM, with " + juce::String ((int) fit.changes.size()) + " changes"
                   : bpmText (fit.tempo) + " BPM";
        text << ": " << fit.within << " of " << (int) hits.size() << " hits within a frame; the farthest "
             << juce::String (1000.0 * (double) worst / sampleRate, 1) << " ms off.";

        if (mode != Picture::LandOn::barLines)
            text << " The tempo's changes, if any, are replaced.";
    }

    summary.setText (text, juce::dontSendNotification);
    repaint();
}

juce::Rectangle<int> FitTempoPanel::listArea() const
{
    return getLocalBounds().withTrimmedTop (2 * controlHeight + 8 + headerHeight).withTrimmedBottom (summaryHeight);
}

juce::Rectangle<int> FitTempoPanel::getRowArea (int row) const
{
    return { 0, listArea().getY() + (row - firstRow) * rowHeight, getWidth(), rowHeight };
}

void FitTempoPanel::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    const auto most = juce::jmax (0, getNumRows() - listArea().getHeight() / rowHeight);
    firstRow = juce::jlimit (0, most, firstRow + (wheel.deltaY < 0.0f ? 1 : wheel.deltaY > 0.0f ? -1 : 0));
    repaint();
}

void FitTempoPanel::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < getNumRows(); ++i)
        if (getRowArea (i).contains (e.getPosition()) && listArea().contains (e.getPosition()))
        {
            rows[(size_t) i].chosen = ! rows[(size_t) i].chosen;
            refit();
            return;
        }
}

void FitTempoPanel::paint (juce::Graphics& g)
{
    const auto list = listArea();
    g.setColour (Theme::surface);
    g.fillRect (list.withTop (list.getY() - headerHeight));

    g.setFont (12.0f);
    g.setColour (Theme::dimText);
    const auto header = list.withHeight (headerHeight).translated (0, -headerHeight);
    g.drawText ("Hit", header.withTrimmedLeft (28).withWidth (150), juce::Justification::centredLeft);
    g.drawText ("At", header.withTrimmedLeft (180).withWidth (110), juce::Justification::centredLeft);
    g.drawText ("Lands", header.withTrimmedLeft (300).withTrimmedRight (6), juce::Justification::centredRight);

    if (rows.empty())
    {
        g.drawText ("The session has no markers: add some with Find Cuts, Mark Hit or Add Marker.", list.reduced (8, 0), juce::Justification::centred);
        return;
    }

    g.saveState();
    g.reduceClipRegion (list);

    for (int i = 0; i < getNumRows(); ++i)
    {
        const auto r = getRowArea (i);
        const auto& row = rows[(size_t) i];
        const auto box = juce::Rectangle<float> (8.0f, (float) r.getY() + 5.0f, 12.0f, 12.0f);
        g.setColour (Theme::lineStrong);
        g.drawRoundedRectangle (box, 2.0f, 1.0f);

        if (row.chosen)
        {
            g.setColour (Theme::accent);
            g.fillRoundedRectangle (box.reduced (2.5f), 1.5f);
        }

        g.setColour (row.chosen ? Theme::textStrong : Theme::dimText);
        g.drawText (row.marker.name, r.withTrimmedLeft (28).withWidth (150), juce::Justification::centredLeft, true);
        g.drawText (getTimeText (i), r.withTrimmedLeft (180).withWidth (110), juce::Justification::centredLeft, true);

        const auto within = row.miss.has_value() && std::abs (*row.miss) <= tolerance;
        g.setColour (within ? Theme::good : Theme::warn);
        g.drawText (getMissText (i), r.withTrimmedLeft (300).withTrimmedRight (6), juce::Justification::centredRight, true);
    }

    g.restoreState();
}

void FitTempoPanel::resized()
{
    auto r = getLocalBounds();
    auto first = r.removeFromTop (controlHeight).reduced (0, 2);
    landOnLabel.setBounds (first.removeFromLeft (90));
    landOn.setBounds (first.removeFromLeft (220));

    auto second = r.removeFromTop (controlHeight).reduced (0, 2);
    tempoLabel.setBounds (second.removeFromLeft (90));
    lowest.setBounds (second.removeFromLeft (70));
    toLabel.setBounds (second.removeFromLeft (30));
    highest.setBounds (second.removeFromLeft (70));
    bpmLabel.setBounds (second.removeFromLeft (50));

    summary.setBounds (getLocalBounds().removeFromBottom (summaryHeight));
}
