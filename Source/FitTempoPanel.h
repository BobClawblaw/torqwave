#pragma once

#include "Picture.h"

/** Fit Tempo to Hits: the session's point markers in a list, each with a box to choose it as a
    hit and, once fitted, how far off it lands; what the hits land on (beats, bars, or the bar lines
    of a tempo map) and the tempos to look between. The fit is made again at each change, and a
    line under the list sums it up. Hits count as landing when they are within a frame (of the
    session's timecode, or of 25 fps without one). */
class FitTempoPanel final : public juce::Component
{
public:
    /** The markers chosen to start with: those in the range, or every one when it is empty. */
    FitTempoPanel (const SessionState&, juce::Range<juce::int64> range);

    int getNumRows() const noexcept                  { return (int) rows.size(); }
    bool isChosen (int row) const                    { return rows[(size_t) row].chosen; }
    juce::String getName (int row) const             { return rows[(size_t) row].marker.name; }

    /** The row's time, and how far its hit lands from its beat or bar ("+3.2 ms, 0.08 fr"; empty when it isn't chosen). */
    juce::String getTimeText (int row) const;
    juce::String getMissText (int row) const;

    void setLandOn (Picture::LandOn);
    void setTempoRange (double lowest, double highest);

    /** The fit for the chosen hits as things stand (invalid with none chosen). */
    const Picture::Fit& getFit() const noexcept      { return fit; }
    juce::String getSummary() const                  { return summary.getText(); }

    /** Where a row is in the panel, for clicking (the list scrolls with the wheel). */
    juce::Rectangle<int> getRowArea (int row) const;

    /** The frames of a frame of picture, the tolerance the hits are counted within. */
    juce::int64 getTolerance() const noexcept        { return tolerance; }

    static constexpr int rowHeight = 22, width = 460;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Row
    {
        Marker marker;
        bool chosen = false;
        std::optional<juce::int64> miss;
    };

    void refit();
    juce::Rectangle<int> listArea() const;

    std::vector<Row> rows;
    double sampleRate;
    int beatsPerBar;
    TimeFormat::Timecode timecode;
    juce::int64 tolerance;
    Picture::Fit fit;

    int firstRow = 0;   // scrolled to

    juce::Label landOnLabel, tempoLabel, toLabel, bpmLabel, summary;
    juce::ComboBox landOn;
    juce::TextEditor lowest, highest;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FitTempoPanel)
};
