#include "ClipProperties.h"
#include "Theme.h"
#include "Metronome.h"

namespace
{
    juce::String secondsText (juce::int64 frames, double rate)
    {
        return juce::String ((double) frames / rate, 3);
    }

    /** Seconds typed into a field, as frames; nothing for text that isn't a number. */
    std::optional<juce::int64> framesFrom (const juce::String& text, double rate)
    {
        const auto trimmed = text.trim();

        if (trimmed.isEmpty() || ! trimmed.containsOnly ("0123456789.-+"))
            return std::nullopt;

        return (juce::int64) std::llround (trimmed.getDoubleValue() * rate);
    }
}

ClipProperties::ClipProperties (AudioEngine& e, std::function<juce::uint32()> toShow, std::function<juce::uint32()> notesToShow)
    : engine (e), clipToShow (std::move (toShow)), noteClipToShow (std::move (notesToShow))
{
    const juce::StringArray names { "Name", "Start (s)", "Length (s)", "Gain (dB)", "Fade in (s)", "Fade out (s)", "Note" };

    for (size_t i = 0; i < rows.size(); ++i)
    {
        auto& row = rows[i];
        row.label.setText (names[(int) i], juce::dontSendNotification);
        row.label.setJustificationType (juce::Justification::centredRight);
        const auto field = (Field) i;
        row.editor.onReturnKey = [this, field] { apply (field); };
        row.editor.onFocusLost = [this, field] { apply (field); };
        row.editor.onEscapeKey = [this] { refresh(); };
        addAndMakeVisible (row.label);
        addAndMakeVisible (row.editor);
    }

    Theme::follow (info, juce::Label::textColourId, Theme::dimText);
    addAndMakeVisible (info);
    engine.session.addChangeListener (this);
    refresh();
    startTimerHz (8);
}

ClipProperties::~ClipProperties()
{
    engine.session.removeChangeListener (this);
}

juce::TextEditor& ClipProperties::getEditor (Field f)
{
    return rows[(size_t) f].editor;
}

void ClipProperties::apply (Field f)
{
    const auto text = getEditor (f).getText();
    auto next = *engine.session.getState();
    const auto rate = next.sampleRate;
    const auto frames = framesFrom (text, rate);
    auto changed = false;

    // Counting bars, a start is a place in bars and a length (or a note clip's loop) is bars long
    // from the clip's start; the fades stay in seconds.
    const auto bars = next.barsAndBeats;
    const auto map = next.tempoMap();
    const auto startFrom = [&] { return bars ? Metronome::parsePosition (map, text) : frames; };
    const auto lengthFrom = [&] (juce::int64 start) { return bars ? Metronome::parseLength (map, start, text) : frames; };
    juce::String description;

    // A note clip: its name, start, length and loop length (0 plays the notes once).
    if (const auto notes = next.findNoteClip (shownNoteClip); shownNoteClip != 0 && notes.has_value())
    {
        const auto c = next.tracks[(size_t) notes->first].noteClips[(size_t) notes->second];

        switch (f)
        {
            case Field::name:
                if (text.trim().isNotEmpty() && text.trim() != c.name)
                {
                    auto renamed = c;
                    renamed.name = text.trim();
                    changed = SessionEdits::replaceNoteClip (next, c.id, renamed);
                    description = "Rename Notes";
                }
                break;
            case Field::start:
                if (const auto to = startFrom(); to.has_value() && *to >= 0 && *to != c.position)
                {
                    changed = SessionEdits::moveNoteClip (next, c.id, notes->first, *to);
                    SessionEdits::carryAutomation (next, *engine.session.getState(), { c.id });
                }

                description = "Move Notes";
                break;
            case Field::length:
                if (const auto length = lengthFrom (c.position); length.has_value() && *length > 0 && *length != c.length)
                    changed = SessionEdits::trimNoteClip (next, c.id, c.position, c.position + *length);

                description = "Trim Notes";
                break;
            case Field::gain:   // the loop's row
                if (const auto loop = lengthFrom (c.position); loop.has_value() && *loop >= 0 && *loop != c.loopLength)
                {
                    auto looped = c;
                    looped.loopLength = *loop;
                    changed = SessionEdits::replaceNoteClip (next, c.id, looped);
                }

                description = "Loop Notes";
                break;
            case Field::fadeIn:
            case Field::fadeOut:
            case Field::note:
                break;
        }

        if (changed)
            engine.session.apply (std::move (next), description);
        else
            refresh();

        return;
    }

    const auto where = next.findClip (shownClip);

    if (! where.has_value())
        return;

    const auto c = next.tracks[(size_t) where->first].clips[(size_t) where->second];

    switch (f)
    {
        case Field::name:
            if (text.trim().isNotEmpty() && text.trim() != c.name)
            {
                next.tracks[(size_t) where->first].clips[(size_t) where->second].name = text.trim();
                changed = true;
                description = "Rename Clip";
            }
            break;
        case Field::start:
            if (const auto to = startFrom(); to.has_value() && *to >= 0 && *to != c.position)
            {
                changed = SessionEdits::moveClip (next, c.id, where->first, *to);
                SessionEdits::carryAutomation (next, *engine.session.getState(), { c.id });
            }

            description = "Move Clip";
            break;
        case Field::length:
            if (const auto length = lengthFrom (c.position); length.has_value() && *length > 0 && *length != c.length)
                changed = SessionEdits::trimClip (next, c.id, c.position, c.position + *length);

            description = "Trim Clip";
            break;
        case Field::gain:
        {
            const auto db = text.trim();

            if (db.isNotEmpty() && db.containsOnly ("0123456789.-+") && ! juce::exactlyEqual (db.getFloatValue(), c.gainDb))
                changed = SessionEdits::setClipGain (next, c.id, db.getFloatValue());

            description = "Clip Gain";
            break;
        }
        case Field::fadeIn:
            changed = frames.has_value() && *frames >= 0 && *frames != c.fadeIn && SessionEdits::setClipFades (next, c.id, *frames, c.fadeOut);
            description = "Clip Fade In";
            break;
        case Field::fadeOut:
            changed = frames.has_value() && *frames >= 0 && *frames != c.fadeOut && SessionEdits::setClipFades (next, c.id, c.fadeIn, *frames);
            description = "Clip Fade Out";
            break;
        case Field::note:
            // Any text, or none to clear it; locked clips take notes too.
            changed = text.trim() != c.note;
            next.tracks[(size_t) where->first].clips[(size_t) where->second].note = text.trim();
            description = "Clip Note";
            break;
    }

    if (changed)
        engine.session.apply (std::move (next), description);
    else
        refresh();   // shows the setting again in place of what couldn't apply
}

void ClipProperties::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refresh();
}

void ClipProperties::timerCallback()
{
    if ((clipToShow != nullptr && clipToShow() != shownClip) || (noteClipToShow != nullptr && noteClipToShow() != shownNoteClip))
        refresh();
}

void ClipProperties::refresh()
{
    shownClip = clipToShow != nullptr ? clipToShow() : 0;
    shownNoteClip = noteClipToShow != nullptr ? noteClipToShow() : 0;
    const auto state = engine.session.getState();
    const auto notes = state->findNoteClip (shownNoteClip);
    auto where = state->findClip (shownClip);

    // A note clip takes the panel over an audio clip; its rows are the first four.
    if (notes.has_value())
    {
        shownClip = 0;
        where.reset();
    }
    else
    {
        shownNoteClip = 0;
    }

    // Counting bars (Bars and Beats), the start, length and loop are in bars, beats, sixteenths and ticks.
    const auto bars = state->barsAndBeats;
    const auto map = state->tempoMap();
    const auto startText = [&] (juce::int64 frame) { return bars ? Metronome::positionText (map, frame) : secondsText (frame, state->sampleRate); };
    const auto lengthText = [&] (juce::int64 start, juce::int64 length) { return bars ? Metronome::lengthText (map, start, length) : secondsText (length, state->sampleRate); };
    const char* const audioNames[] = { "Name", bars ? "Start" : "Start (s)", bars ? "Length" : "Length (s)", "Gain (dB)", "Fade in (s)", "Fade out (s)", "Note" };
    const char* const noteNames[] = { "Name", bars ? "Start" : "Start (s)", bars ? "Length" : "Length (s)", bars ? "Loop" : "Loop (s)" };
    constexpr size_t noteRows = 4;

    for (size_t i = 0; i < rows.size(); ++i)
    {
        const auto inUse = ! notes.has_value() || i < noteRows;
        rows[i].label.setText (notes.has_value() && i < noteRows ? noteNames[i] : audioNames[i], juce::dontSendNotification);
        rows[i].label.setVisible (inUse);
        rows[i].editor.setVisible (inUse);
    }

    if (notes.has_value())
    {
        const auto& c = state->tracks[(size_t) notes->first].noteClips[(size_t) notes->second];
        const auto rate = state->sampleRate;
        const std::array<juce::String, noteRows> values { c.name, startText (c.position), lengthText (c.position, c.length), lengthText (c.position, c.loopLength) };

        for (size_t i = 0; i < rows.size(); ++i)
        {
            rows[i].editor.setEnabled (i < noteRows);

            if (i >= noteRows)
                rows[i].editor.setText ({}, false);
            else if (! rows[i].editor.hasKeyboardFocus (true))
                rows[i].editor.setText (values[i], false);
        }

        info.setText (juce::String ((int) c.notes.size()) + (c.notes.size() == 1 ? " note, " : " notes, ") + juce::String ((int) c.lanes.size()) + (c.lanes.size() == 1 ? " lane" : " lanes")
                          + (c.loopLength > 0 ? ", repeating every " + secondsText (c.loopLength, rate) + " s" : juce::String (", played once")) + (c.muted ? ", muted" : ""),
                      juce::dontSendNotification);

        if (asLine)
            resized();

        return;
    }

    if (! where.has_value())
    {
        shownClip = 0;
        info.setText ("No clip selected", juce::dontSendNotification);

        for (auto& row : rows)
        {
            row.editor.setText ({}, false);
            row.editor.setEnabled (false);
        }

        if (asLine)
            resized();

        return;
    }

    const auto& c = state->tracks[(size_t) where->first].clips[(size_t) where->second];
    const auto rate = state->sampleRate;
    const std::array<juce::String, 7> values { c.name, startText (c.position), lengthText (c.position, c.length),
                                               juce::String (c.gainDb, 1), secondsText (c.fadeIn, rate), secondsText (c.fadeOut, rate), c.note };

    for (size_t i = 0; i < rows.size(); ++i)
    {
        rows[i].editor.setEnabled (true);

        // Not while it is being typed into.
        if (! rows[i].editor.hasKeyboardFocus (true))
            rows[i].editor.setText (values[i], false);
    }

    info.setText (c.source.getFileName() + (c.takes.size() > 1 ? ", take " + juce::String (c.activeTake + 1) + " of " + juce::String ((int) c.takes.size()) : juce::String()),
                  juce::dontSendNotification);

    if (asLine)
        resized();
}

void ClipProperties::setLine (bool on)
{
    asLine = on;
    refresh();
    resized();
    repaint();
}

void ClipProperties::paint (juce::Graphics& g)
{
    g.fillAll (asLine ? Theme::base : Theme::surface);

    if (asLine)
    {
        g.setColour (Theme::line);
        g.fillRect (0, getHeight() - 1, getWidth(), 1);
    }
}

void ClipProperties::resized()
{
    if (asLine)
    {
        // Label and field after label and field, then what the clip plays; with nothing selected,
        // just that, in the middle.
        auto r = getLocalBounds().reduced (6, 4);
        const auto any = rows[0].editor.isEnabled();
        info.setJustificationType (any ? juce::Justification::centredLeft : juce::Justification::centred);

        for (size_t i = 0; i < rows.size(); ++i)
        {
            auto& row = rows[i];
            const auto shown = any && row.label.isVisible();
            const auto font = row.label.getFont();
            const auto labelWidth = juce::GlyphArrangement::getStringWidthInt (font, row.label.getText()) + 8;
            const auto editorWidth = i == (size_t) Field::name ? 130 : i == (size_t) Field::note ? 150 : 64;

            if (! shown || r.getWidth() < labelWidth + editorWidth)
            {
                row.label.setBounds ({});
                row.editor.setBounds ({});
                continue;
            }

            row.label.setBounds (r.removeFromLeft (labelWidth));
            row.editor.setBounds (r.removeFromLeft (editorWidth));
            r.removeFromLeft (10);
        }

        info.setBounds (any ? r : getLocalBounds().reduced (6, 4));
        return;
    }

    info.setJustificationType (juce::Justification::centredLeft);
    auto r = getLocalBounds().reduced (10);
    info.setBounds (r.removeFromTop (22));
    r.removeFromTop (6);

    for (auto& row : rows)
    {
        auto line = r.removeFromTop (24);
        row.label.setBounds (line.removeFromLeft (90));
        line.removeFromLeft (8);
        row.editor.setBounds (line);
        r.removeFromTop (6);
    }
}
