#include "DrumMachineEditor.h"
#include "DrumKitImport.h"
#include "InstrumentKeys.h"
#include "SoundDrops.h"
#include "Theme.h"

using DrumMachine::Sound;

namespace
{
    constexpr int windowWidth = 1100, windowHeight = 720;
    constexpr double pictureRate = 32000.0;   // the picture's hit is rendered at this rate: plenty to draw, quick to make

    juce::String notesOf (Sound s)
    {
        juce::StringArray notes;

        for (int n = DrumMachine::firstNote; n <= DrumMachine::lastNote; ++n)
            if (DrumMachine::soundFor (n) == s)
                notes.add (juce::String (n));

        return notes.joinIntoString (" \xc2\xb7 ");
    }

    /** What chokes a sound, for the line under its knobs. */
    juce::String chokesOf (Sound s)
    {
        if (s == Sound::closedHat || s == Sound::pedalHat || s == Sound::openHat)
            return "Hats choke each other";

        if (s == Sound::triangle)
            return "Mute (80) and open (81) choke each other";

        if (s == Sound::cuica)
            return "Mute (78) and open (79) choke each other";

        return "Chokes nothing; a hit struck again fades the last";
    }

    // Middle C is C4, as on the keys under the window and everywhere else in Torqwave.
    juce::String noteName (int note)   { return juce::MidiMessage::getMidiNoteName (note, true, true, 4); }

    constexpr const char* soundFileTypes = "wav;wave;bwf;aif;aiff;aifc;flac;ogg;opus;mp3;m4a;caf";

    juce::String panText (float v)
    {
        const auto r = juce::roundToInt (v);
        return r == 0 ? juce::String ("C") : (r < 0 ? "L " : "R ") + juce::String (std::abs (r));
    }
}

//==============================================================================
/** A pad: its name and notes on a card in its colour (a sound's family's, a user pad's own), lit as
    it plays. A user pad with no samples is drawn fainter. */
class DrumMachineEditor::Pad final : public juce::Component,
                                    public juce::SettableTooltipClient
{
public:
    Pad (int p, std::function<void (int)> clicked, std::function<void (int)> menu)
        : pad (p), onClick (std::move (clicked)), onMenu (std::move (menu))
    {
        setRepaintsOnMouseActivity (true);
        setTooltip ("Click to play it, at the velocity the keys below play at (C and V change it)");
    }

    void describe (const juce::String& newName, const juce::String& newNotes, juce::Colour c, bool isEmpty)
    {
        if (newName == name && newNotes == notes && c == colour && isEmpty == empty)
            return;

        name = newName;
        notes = newNotes;
        colour = c;
        empty = isEmpty;
        setTitle (name);
        setDescription ("Plays " + name + " at the keys' velocity");
        repaint();
    }

    juce::String getText() const   { return name + " / " + notes; }

    void setSelected (bool s)
    {
        if (s != selected)
        {
            selected = s;
            repaint();
        }
    }

    /** Lights it at a hit's velocity; the light falls away over about a third of a second. */
    void hit (float velocity)
    {
        glow = juce::jmax (glow, 0.35f + 0.65f * velocity);
        repaint();
    }

    void fade()
    {
        if (glow > 0.0f)
        {
            glow = glow < 0.02f ? 0.0f : glow * 0.86f;
            repaint();
        }
    }

    float getGlow() const noexcept   { return glow; }

    /** Whether it plays samples (a mark in its corner), and whether a drag is over it. */
    void setSampled (bool s)
    {
        if (s != sampled)
        {
            sampled = s;
            repaint();
        }
    }

    void setDropping (bool d)
    {
        if (d != dropping)
        {
            dropping = d;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const auto light = Theme::current().light;
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        constexpr auto corner = 6.0f;

        // An empty user pad: the whole card fainter.
        if (empty && ! selected && ! dropping)
            g.beginTransparencyLayer (0.55f);

        const auto over = isMouseOver (true), down = isMouseButtonDown();

        // The card: raised, tinted by its family, brighter under the mouse, pressed in when down.
        const auto base = Theme::raised.interpolatedWith (colour, light ? 0.10f : 0.14f);
        const auto top = (over ? base.brighter (light ? 0.04f : 0.12f) : base).brighter (light ? 0.02f : 0.05f);
        const auto bottom = base.darker (light ? 0.06f : 0.25f);
        g.setColour (juce::Colours::black.withAlpha (light ? 0.10f : 0.35f));
        g.fillRoundedRectangle (r.translated (0.0f, down ? 0.5f : 1.5f), corner);
        g.setGradientFill (juce::ColourGradient (down ? bottom : top, 0.0f, r.getY(), down ? top : bottom, 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, corner);

        // The light: a glow from the bottom in its colour, as bright as the hit was hard.
        if (glow > 0.0f)
        {
            g.setGradientFill (juce::ColourGradient (colour.withAlpha (0.75f * glow), r.getCentreX(), r.getBottom(),
                                                     colour.withAlpha (0.08f * glow), r.getCentreX(), r.getY(), false));
            g.fillRoundedRectangle (r, corner);
            g.setColour (colour.withAlpha (0.9f * glow));
            g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.5f);
        }

        // The family's mark along the top, and the edge: the accent when selected.
        g.setColour (colour.withAlpha (light ? 0.85f : 0.9f));
        g.fillRoundedRectangle (r.getX() + 7.0f, r.getY() + 6.0f, 12.0f, 3.0f, 1.5f);

        if (selected)
        {
            g.setColour (Theme::accent.withAlpha (0.25f));
            g.drawRoundedRectangle (r.expanded (1.0f), corner + 1.0f, 3.0f);
            g.setColour (Theme::accent);
            g.drawRoundedRectangle (r, corner, 1.5f);
        }
        else
        {
            g.setColour (Theme::line.withAlpha (over ? 1.0f : 0.7f));
            g.drawRoundedRectangle (r, corner, 1.0f);
        }

        auto text = r.reduced (6.0f, 4.0f);
        text.removeFromTop (7.0f);
        g.setColour (selected || glow > 0.3f ? Theme::textStrong : Theme::text);
        g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
        g.drawFittedText (name, text.removeFromTop (text.getHeight() * 0.64f).toNearestInt(), juce::Justification::bottomLeft, 2, 0.8f);
        g.setColour (Theme::dimText);
        g.setFont (juce::FontOptions (9.5f));
        g.drawText (notes, text, juce::Justification::bottomLeft);

        // A pad that plays samples: three bars of a waveform in its corner.
        if (sampled)
        {
            g.setColour (colour.withAlpha (0.9f));
            const float heights[] { 4.0f, 9.0f, 6.0f, 3.0f };

            for (int i = 0; i < 4; ++i)
                g.fillRect (r.getRight() - 20.0f + 4.0f * (float) i, r.getY() + 10.0f - heights[i] * 0.5f, 2.0f, heights[i]);
        }

        if (empty && ! selected && ! dropping)
            g.endTransparencyLayer();

        if (dropping)
        {
            g.setColour (Theme::accent.withAlpha (0.22f));
            g.fillRoundedRectangle (r, corner);
            g.setColour (Theme::accent);
            g.drawRoundedRectangle (r, corner, 2.0f);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        repaint();

        if (e.mods.isPopupMenu())
        {
            onMenu (pad);
            return;
        }

        onClick (pad);
    }

    void mouseUp (const juce::MouseEvent&) override   { repaint(); }

private:
    int pad;
    juce::String name, notes;
    juce::Colour colour;
    std::function<void (int)> onClick;
    std::function<void (int)> onMenu;
    bool selected = false, sampled = false, dropping = false, empty = false;
    float glow = 0.0f;
};

//==============================================================================
/** One hit of the selected sound: its waveform, its level in dB falling to -60 dB, where its decay
    setting ends, and a line sweeping across as it plays. */
class DrumMachineEditor::Picture final : public juce::Component
{
public:
    void set (const juce::AudioBuffer<float>& hit, double sampleRate, float decayMs, juce::Colour c)
    {
        colour = c;
        decaySeconds = decayMs * 0.001;
        const auto mid = [&hit] (int i) { return 0.5f * (hit.getSample (0, i) + hit.getSample (1, i)); };

        // How long it lasts: to its last sample above -90 dB.
        auto last = hit.getNumSamples() - 1;

        while (last > 0 && std::abs (mid (last)) < 3.0e-5f)
            --last;

        seconds = juce::jmax (0.02, (last + 1) / sampleRate * 1.04);
        const auto shown = juce::jmin (hit.getNumSamples(), (int) std::ceil (seconds * sampleRate));
        peak = 0.0f;
        samples.resize ((size_t) shown);

        for (int i = 0; i < shown; ++i)
        {
            samples[(size_t) i] = mid (i);
            peak = juce::jmax (peak, std::abs (samples[(size_t) i]));
        }

        lows.clear();   // made again for the width it is drawn at
        repaint();
    }

    /** Each column's lowest, highest and loudest sample, a column for each pixel of the display. */
    void build (int columns)
    {
        const auto shown = (int) samples.size();
        lows.assign ((size_t) columns, 0.0f);
        highs.assign ((size_t) columns, 0.0f);
        levels.assign ((size_t) columns, 0.0f);
        std::vector<bool> reached ((size_t) columns, false);

        for (int i = 0; i < shown; ++i)
        {
            const auto col = (size_t) juce::jmin (columns - 1, (int) ((double) i / shown * columns));
            const auto v = samples[(size_t) i];
            lows[col] = reached[col] ? juce::jmin (lows[col], v) : v;
            highs[col] = reached[col] ? juce::jmax (highs[col], v) : v;
            levels[col] = juce::jmax (levels[col], std::abs (v));
            reached[col] = true;
        }

        // With more columns than samples, a column no sample fell in runs between its neighbours.
        for (int col = 1; col < columns; ++col)
            if (! reached[(size_t) col])
            {
                lows[(size_t) col] = lows[(size_t) col - 1];
                highs[(size_t) col] = highs[(size_t) col - 1];
                levels[(size_t) col] = levels[(size_t) col - 1];
            }

        // The level as a peak meter would hold it: each column the loudest of the next 20 ms, so a
        // low sound's single cycles don't ripple the line; then evened out over the same 20 ms, so
        // the line falls as a curve and not in steps.
        const auto hold = juce::jmax (1, (int) std::ceil (0.02 / seconds * columns));
        auto held = levels;

        for (int col = 0; col < columns; ++col)
            for (int k = 1; k < hold && col + k < columns; ++k)
                held[(size_t) col] = juce::jmax (held[(size_t) col], levels[(size_t) (col + k)]);

        std::vector<double> sums ((size_t) columns + 1, 0.0);

        for (int col = 0; col < columns; ++col)
            sums[(size_t) col + 1] = sums[(size_t) col] + held[(size_t) col];

        for (int col = 0; col < columns; ++col)
        {
            const auto from = juce::jmax (0, col - hold / 2), to = juce::jmin (columns, col + hold / 2 + 1);
            levels[(size_t) col] = (float) ((sums[(size_t) to] - sums[(size_t) from]) / (double) (to - from));
        }
    }

    double getSeconds() const noexcept   { return seconds; }

    /** A hit just played: sweep across it. */
    void sweep()
    {
        sweepStart = juce::Time::getMillisecondCounterHiRes();
        repaint();
    }

    bool isSweeping() const
    {
        return sweepStart > 0.0 && (juce::Time::getMillisecondCounterHiRes() - sweepStart) * 0.001 < seconds + 0.05;
    }

    void paint (juce::Graphics& g) override
    {
        const auto area = getLocalBounds().toFloat();
        SoundControls::drawDisplayWell (g, area, 8, 4);

        if (samples.empty() || peak <= 0.0f)
            return;

        auto plot = area.reduced (6.0f, 8.0f);
        plot.removeFromBottom (14.0f);   // the time scale

        if (const auto columns = juce::jmax (2, (int) std::ceil (plot.getWidth() * juce::jmax (1.0f, g.getInternalContext().getPhysicalPixelScaleFactor())));
            columns != (int) lows.size())
            build (columns);

        const auto n = (int) lows.size();
        const auto xOf = [&plot, n] (int col) { return plot.getX() + plot.getWidth() * (float) col / (float) (n - 1); };

        // The waveform, around the middle, scaled to its own peak.
        juce::Path wave;
        const auto centre = plot.getCentreY(), half = plot.getHeight() * 0.46f;
        wave.startNewSubPath (xOf (0), centre - highs[0] / peak * half);

        for (int c = 1; c < n; ++c)
            wave.lineTo (xOf (c), centre - highs[(size_t) c] / peak * half);

        for (int c = n; --c >= 0;)
            wave.lineTo (xOf (c), centre - lows[(size_t) c] / peak * half);

        wave.closeSubPath();
        g.setColour (colour.withAlpha (0.45f));
        g.fillPath (wave);
        g.setColour (colour.withAlpha (0.85f));
        g.strokePath (wave, juce::PathStrokeType (0.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Its level in dB from its peak (the top) to 60 dB down (the bottom).
        juce::Path fall;

        for (int c = 0; c < n; ++c)
        {
            const auto db = juce::jlimit (-60.0f, 0.0f, juce::Decibels::gainToDecibels (levels[(size_t) c] / peak, -60.0f));
            const auto y = plot.getY() + plot.getHeight() * (-db / 60.0f);

            if (c == 0)
                fall.startNewSubPath (xOf (c), y);
            else
                fall.lineTo (xOf (c), y);
        }

        SoundControls::drawGlowingCurve (g, fall, Theme::accentLight, plot, false);

        // Where the decay setting says it has fallen 60 dB.
        if (decaySeconds < seconds)
        {
            const auto x = plot.getX() + plot.getWidth() * (float) (decaySeconds / seconds);
            juce::Path dash;
            const float lengths[] { 3.0f, 3.0f };
            juce::Path line;
            line.startNewSubPath (x, plot.getY());
            line.lineTo (x, plot.getBottom());
            juce::PathStrokeType (1.0f).createDashedStroke (dash, line, lengths, 2);
            g.setColour (Theme::dimText.withAlpha (0.8f));
            g.fillPath (dash);
        }

        // The scale: a few round times along the bottom, the peak in a corner.
        g.setFont (juce::FontOptions (10.0f));
        g.setColour (Theme::dimText);
        const auto step = niceStep (seconds / 5.0);

        for (double t = 0.0; t <= seconds; t += step)
        {
            const auto x = plot.getX() + plot.getWidth() * (float) (t / seconds);
            g.fillRect (x, plot.getBottom() + 2.0f, 1.0f, 3.0f);
            const auto text = t == 0.0 ? juce::String ("0") : t < 1.0 ? juce::String (juce::roundToInt (t * 1000.0)) + " ms" : juce::String (t, step < 1.0 ? 1 : 0) + " s";
            g.drawText (text, juce::Rectangle<float> (x - 30.0f, plot.getBottom() + 4.0f, 60.0f, 12.0f), t == 0.0 ? juce::Justification::centredLeft : juce::Justification::centred);
        }

        g.drawText ("peak " + juce::String (juce::Decibels::gainToDecibels (peak), 1) + " dB",
                    area.reduced (8.0f, 6.0f).removeFromTop (12.0f), juce::Justification::topRight);

        // The sweep.
        if (isSweeping())
        {
            const auto t = (juce::Time::getMillisecondCounterHiRes() - sweepStart) * 0.001;
            const auto x = plot.getX() + plot.getWidth() * (float) juce::jmin (1.0, t / seconds);
            g.setGradientFill (juce::ColourGradient (Theme::accent.withAlpha (0.0f), x - 24.0f, 0.0f, Theme::accent.withAlpha (0.25f), x, 0.0f, false));
            g.fillRect (juce::Rectangle<float> (x - 24.0f, plot.getY(), 24.0f, plot.getHeight()));
            g.setColour (Theme::accent);
            g.fillRect (x, plot.getY(), 1.5f, plot.getHeight());
        }
    }

private:
    static double niceStep (double rough)
    {
        for (auto s : { 0.005, 0.01, 0.02, 0.025, 0.05, 0.1, 0.2, 0.25, 0.5, 1.0, 2.0 })
            if (s >= rough)
                return s;

        return 5.0;
    }

    std::vector<float> samples, lows, highs, levels;
    juce::Colour colour;
    double seconds = 0.0, decaySeconds = 0.0, sweepStart = 0.0;
    float peak = 0.0f;
};

//==============================================================================
/** How far the glue turns the kit down: a short bar filling from the right, 12 dB full. */
class DrumMachineEditor::GlueMeter final : public juce::Component,
                                           public juce::SettableTooltipClient
{
public:
    void setDb (float db)
    {
        if (std::abs (db - shown) > 0.05f)
        {
            shown = db;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        auto bar = r.removeFromBottom (6.0f);
        g.setColour (Theme::inset);
        g.fillRoundedRectangle (bar, 3.0f);
        const auto share = juce::jlimit (0.0f, 1.0f, -shown / 12.0f);

        if (share > 0.0f)
        {
            g.setColour (Theme::meterMid);
            g.fillRoundedRectangle (bar.removeFromRight (juce::jmax (6.0f, bar.getWidth() * share)), 3.0f);
        }

        g.setColour (Theme::dimText);
        g.setFont (juce::FontOptions (10.0f));
        g.drawText ("GR " + juce::String (juce::jmin (0.0f, shown), 1) + " dB", r, juce::Justification::centredLeft);
    }

private:
    float shown = 0.0f;
};

//==============================================================================
/** A Sample pad's layers: a row each, with its waveform, its name and the velocities that play it.
    A click selects a row; dragging its low or high velocity up or down changes it, and a double
    click on either gives the row every velocity again. The buttons along the top add sounds,
    remove the selected row, move it up or down (round robin goes in this order), and spread the
    rows evenly across the velocities, the first the softest. The wheel scrolls. */
class DrumMachineEditor::Layers final : public juce::Component
{
public:
    Layers (BuiltInEffects::BasicDrumsProcessor& d, std::function<void()> addSounds) : drums (d)
    {
        for (auto* b : { &add, &remove, &up, &down, &spread })
        {
            addAndMakeVisible (*b);
            b->setWantsKeyboardFocus (false);
        }

        add.setTooltip ("Add sounds to this pad as layers (or drop them on the pad)");
        remove.setTooltip ("Remove the selected layer");
        up.setTooltip ("Move the selected layer up: layers that share velocities take turns in this order");
        down.setTooltip ("Move the selected layer down");
        spread.setTooltip ("Spread the layers evenly across the velocities, the first the softest, so each plays for its own range");
        add.onClick = std::move (addSounds);
        remove.onClick = [this] { press ("remove"); };
        up.onClick = [this] { press ("up"); };
        down.onClick = [this] { press ("down"); };
        spread.onClick = [this] { press ("spread"); };
    }

    void show (int p, juce::Colour c)
    {
        if (p != pad)
            forgetHit();

        pad = p;
        colour = c;
        reload();
    }

    /** The pad's last hit, from wherever it came: its velocity (1 to 127) and the layer it played
        (-1 for none). The header says both, and the layer's row lights and stays marked. */
    void played (int velocity, int row)
    {
        hitVelocity = velocity;
        hitRow = row;
        light = 1.0f;

        // The row that played is brought into view.
        if (juce::isPositiveAndBelow (row, (int) rows.size()))
            scroll = juce::jlimit (juce::jmax (0, row - visibleRows() + 1), row, scroll);

        repaint();
    }

    /** The light falls away over about half a second; the mark stays. */
    void fade()
    {
        if (light > 0.0f)
        {
            light = light < 0.02f ? 0.0f : light * 0.9f;
            repaint();
        }
    }

    void forgetHit()
    {
        hitVelocity = 0;
        hitRow = -1;
        light = 0.0f;
    }

    int getLitRow() const noexcept        { return hitRow; }

    /** The header's words about the last hit: "hit 52: layer 2", "hit 20: no layer", or nothing before one. */
    juce::String hitText() const
    {
        if (hitVelocity <= 0)
            return {};

        return "hit " + juce::String (hitVelocity) + (hitRow >= 0 ? ": layer " + juce::String (hitRow + 1) : juce::String (": no layer"));
    }

    void reload()
    {
        rows = drums.getLayers (pad);
        selectedRow = rows.empty() ? -1 : juce::jlimit (0, (int) rows.size() - 1, selectedRow);
        shapes.clear();

        // Each sample's shape: the lowest and highest of its two channels' mean in each of 72 columns.
        for (auto& l : rows)
        {
            std::vector<std::pair<float, float>> shape;

            if (l.sample != nullptr && l.sample->getNumFrames() > 0)
            {
                constexpr int columns = 72;
                shape.assign ((size_t) columns, { 0.0f, 0.0f });
                const auto& audio = l.sample->audio;
                const auto n = audio.getNumSamples();
                const auto* a = audio.getReadPointer (0);
                const auto* b = audio.getNumChannels() > 1 ? audio.getReadPointer (1) : a;
                auto peak = 1.0e-6f;

                for (int i = 0; i < n; ++i)
                {
                    const auto v = 0.5f * (a[i] + b[i]);
                    auto& [low, high] = shape[(size_t) juce::jmin (columns - 1, (int) ((juce::int64) i * columns / n))];
                    low = juce::jmin (low, v);
                    high = juce::jmax (high, v);
                    peak = juce::jmax (peak, std::abs (v));
                }

                for (auto& [low, high] : shape)
                {
                    low /= peak;
                    high /= peak;
                }
            }

            shapes.push_back (std::move (shape));
        }

        const auto any = ! rows.empty();
        remove.setEnabled (any);
        up.setEnabled (selectedRow > 0);
        down.setEnabled (any && selectedRow < (int) rows.size() - 1);
        spread.setEnabled (rows.size() > 1);
        scroll = juce::jlimit (0, juce::jmax (0, (int) rows.size() - visibleRows()), scroll);
        repaint();
    }

    int getNumRows() const noexcept       { return (int) rows.size(); }

    juce::String rowText (int row) const
    {
        if (! juce::isPositiveAndBelow (row, (int) rows.size()))
            return {};

        const auto& l = rows[(size_t) row];
        return nameOfLayer (l) + "  " + juce::String (l.lowVelocity) + "-" + juce::String (l.highVelocity);
    }

    void select (int row)
    {
        selectedRow = rows.empty() ? -1 : juce::jlimit (0, (int) rows.size() - 1, row);
        reload();
    }

    /** What a button does, by name: "remove", "up", "down", "spread". */
    void press (const juce::String& which)
    {
        auto next = rows;
        const auto at = selectedRow;

        // The rows are about to change places or ranges: the last hit's mark would point at the wrong one.
        if (which == "remove" || which == "up" || which == "down")
            hitRow = -1;

        if (which == "remove" && juce::isPositiveAndBelow (at, (int) next.size()))
        {
            next.erase (next.begin() + at);
        }
        else if (which == "up" && at > 0 && at < (int) next.size())
        {
            std::swap (next[(size_t) at], next[(size_t) at - 1]);
            --selectedRow;
        }
        else if (which == "down" && at >= 0 && at + 1 < (int) next.size())
        {
            std::swap (next[(size_t) at], next[(size_t) at + 1]);
            ++selectedRow;
        }
        else if (which == "spread" && next.size() > 1)
        {
            const auto n = (int) next.size();

            for (int i = 0; i < n; ++i)
            {
                next[(size_t) i].lowVelocity = 1 + 127 * i / n;
                next[(size_t) i].highVelocity = i == n - 1 ? 127 : 127 * (i + 1) / n;
            }
        }
        else
        {
            return;
        }

        drums.setLayers (pad, std::move (next));
        reload();
    }

    void resized() override
    {
        auto top = getLocalBounds().reduced (4, 2).removeFromTop (headerHeight);
        spread.setBounds (top.removeFromRight (58).reduced (0, 1));
        top.removeFromRight (4);
        down.setBounds (top.removeFromRight (30).reduced (0, 1));
        top.removeFromRight (2);
        up.setBounds (top.removeFromRight (30).reduced (0, 1));
        top.removeFromRight (4);
        remove.setBounds (top.removeFromRight (26).reduced (0, 1));
        add.setBounds (top.removeFromRight (48).reduced (1, 1));
    }

    void paint (juce::Graphics& g) override
    {
        const auto area = getLocalBounds().toFloat();
        g.setColour (Theme::inset);
        g.fillRoundedRectangle (area, 5.0f);
        g.setColour (Theme::line.withAlpha (0.8f));
        g.drawRoundedRectangle (area.reduced (0.5f), 5.0f, 1.0f);

        auto header = getLocalBounds().reduced (8, 2).removeFromTop (headerHeight);
        g.setColour (Theme::dimText);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText ("LAYERS", header.removeFromLeft (50), juce::Justification::centredLeft);
        g.setFont (juce::FontOptions (10.5f));
        g.drawText (juce::String ((int) rows.size()) + " of " + juce::String (DrumMachine::maxLayers), header.removeFromLeft (48), juce::Justification::centredLeft);

        // The last hit: its velocity and the layer it played, so a layer that does not sound can be told from a velocity that missed it.
        if (const auto hit = hitText(); hit.isNotEmpty())
        {
            g.setColour (hitRow >= 0 ? Theme::accentLight.interpolatedWith (Theme::textStrong, light * 0.6f) : Theme::warn);
            g.drawText (hit, header.withRight (add.getX() - 4), juce::Justification::centredLeft);
        }

        auto list = listBounds();

        if (rows.empty())
        {
            g.setColour (Theme::dimText);
            g.setFont (juce::FontOptions (11.5f));
            g.drawText ("Drop sounds here or on the pad: each becomes a layer", list, juce::Justification::centred);
            return;
        }

        g.saveState();
        g.reduceClipRegion (list);

        for (int i = scroll; i < (int) rows.size() && i < scroll + visibleRows() + 1; ++i)
        {
            auto row = rowBounds (i);
            const auto& l = rows[(size_t) i];

            if (i == selectedRow)
            {
                g.setColour (colour.withAlpha (0.16f));
                g.fillRoundedRectangle (row.toFloat().reduced (1.0f, 0.5f), 3.0f);
                g.setColour (colour.withAlpha (0.8f));
                g.fillRect (row.getX() + 1, row.getY() + 3, 2, row.getHeight() - 6);
            }

            // The layer the last hit played: lit at the hit, and outlined after it.
            if (i == hitRow)
            {
                g.setColour (Theme::accentLight.withAlpha (0.10f + 0.35f * light));
                g.fillRoundedRectangle (row.toFloat().reduced (1.0f, 0.5f), 3.0f);
                g.setColour (Theme::accentLight.withAlpha (0.55f + 0.45f * light));
                g.drawRoundedRectangle (row.toFloat().reduced (1.5f, 1.0f), 3.0f, 1.0f);
            }

            g.setFont (juce::FontOptions (10.5f));
            g.setColour (Theme::dimText);
            g.drawText (juce::String (i + 1), row.removeFromLeft (22), juce::Justification::centred);

            // Its waveform.
            auto wave = row.removeFromLeft (waveWidth).reduced (0, 3).toFloat();
            const auto& shape = shapes[(size_t) i];
            g.setColour (Theme::base.withAlpha (0.6f));
            g.fillRoundedRectangle (wave, 2.0f);

            if (! shape.empty())
            {
                g.setColour (colour.withAlpha (0.9f));
                const auto mid = wave.getCentreY(), half = wave.getHeight() * 0.5f - 1.0f;
                const auto w = wave.getWidth() / (float) shape.size();

                for (size_t c = 0; c < shape.size(); ++c)
                {
                    const auto top = mid - shape[c].second * half, bottom = mid - shape[c].first * half;
                    g.fillRect (wave.getX() + w * (float) c, top, juce::jmax (1.0f, w), juce::jmax (1.0f, bottom - top));
                }
            }

            row.removeFromLeft (8);
            const auto high = row.removeFromRight (velocityWidth), dash = row.removeFromRight (10), low = row.removeFromRight (velocityWidth);

            for (auto [cell, value] : { std::pair { low, l.lowVelocity }, std::pair { high, l.highVelocity } })
            {
                g.setColour (Theme::base.withAlpha (0.7f));
                g.fillRoundedRectangle (cell.toFloat().reduced (1.0f, 3.0f), 3.0f);
                g.setColour (Theme::accentLight);
                g.drawText (juce::String (value), cell, juce::Justification::centred);
            }

            g.setColour (Theme::dimText);
            g.drawText ("-", dash, juce::Justification::centred);
            row.removeFromRight (6);
            g.setColour (l.sample == nullptr ? Theme::warn : (i == selectedRow ? Theme::textStrong : Theme::text));
            g.setFont (juce::FontOptions (11.5f));
            g.drawText (nameOfLayer (l) + (l.sample == nullptr ? " (missing)" : ""), row, juce::Justification::centredLeft);
        }

        g.restoreState();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        dragging = 0;
        const auto row = rowAt (e.getPosition());

        if (row < 0)
            return;

        selectedRow = row;
        const auto bounds = rowBounds (row);

        // Its two velocities are dragged; the rest of the row only selects it.
        if (e.x >= bounds.getRight() - velocityWidth)
            dragging = 2;
        else if (e.x >= bounds.getRight() - 2 * velocityWidth - 10 && e.x < bounds.getRight() - velocityWidth - 10)
            dragging = 1;

        dragFrom = dragging == 1 ? rows[(size_t) row].lowVelocity : rows[(size_t) row].highVelocity;
        reload();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging == 0 || ! juce::isPositiveAndBelow (selectedRow, (int) rows.size()))
            return;

        auto next = rows;
        auto& l = next[(size_t) selectedRow];
        const auto value = juce::jlimit (1, 127, dragFrom - e.getDistanceFromDragStartY() / 2);

        if (dragging == 1)
            l.lowVelocity = juce::jmin (value, l.highVelocity);
        else
            l.highVelocity = juce::jmax (value, l.lowVelocity);

        if (l.lowVelocity == rows[(size_t) selectedRow].lowVelocity && l.highVelocity == rows[(size_t) selectedRow].highVelocity)
            return;

        drums.setLayers (pad, std::move (next));
        reload();
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        const auto row = rowAt (e.getPosition());

        if (row < 0 || e.x < rowBounds (row).getRight() - 2 * velocityWidth - 10)
            return;

        auto next = rows;
        next[(size_t) row].lowVelocity = 1;
        next[(size_t) row].highVelocity = 127;
        drums.setLayers (pad, std::move (next));
        reload();
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        const auto most = juce::jmax (0, (int) rows.size() - visibleRows());
        const auto next = juce::jlimit (0, most, scroll - (wheel.deltaY > 0.0f ? 1 : wheel.deltaY < 0.0f ? -1 : 0));

        if (next != scroll)
        {
            scroll = next;
            repaint();
        }
    }

private:
    static constexpr int headerHeight = 22, rowHeight = 22, waveWidth = 84, velocityWidth = 34;

    static juce::String nameOfLayer (const DrumMachine::Layer& l)
    {
        return l.file != juce::File() ? l.file.getFileNameWithoutExtension() : l.sample != nullptr ? l.sample->name : juce::String ("Sample");
    }

    juce::Rectangle<int> listBounds() const         { return getLocalBounds().reduced (4, 2).withTrimmedTop (headerHeight + 2); }
    int visibleRows() const                         { return juce::jmax (1, listBounds().getHeight() / rowHeight); }
    juce::Rectangle<int> rowBounds (int row) const  { const auto list = listBounds(); return { list.getX(), list.getY() + (row - scroll) * rowHeight, list.getWidth(), rowHeight }; }

    int rowAt (juce::Point<int> p) const
    {
        const auto list = listBounds();

        if (! list.contains (p))
            return -1;

        const auto row = scroll + (p.y - list.getY()) / rowHeight;
        return row < (int) rows.size() ? row : -1;
    }

    BuiltInEffects::BasicDrumsProcessor& drums;
    int pad = 0;
    juce::Colour colour;
    std::vector<DrumMachine::Layer> rows;
    std::vector<std::vector<std::pair<float, float>>> shapes;
    juce::TextButton add { "Add..." }, remove { "-" }, up { juce::String::fromUTF8 ("\xe2\x96\xb2") }, down { juce::String::fromUTF8 ("\xe2\x96\xbc") }, spread { "Spread" };
    int selectedRow = -1, scroll = 0, dragging = 0, dragFrom = 0;
    int hitVelocity = 0, hitRow = -1;
    float light = 0.0f;
};

//==============================================================================
DrumMachineEditor::DrumMachineEditor (BuiltInEffects::BasicDrumsProcessor& p)
    : AudioProcessorEditor (p), drums (p),
      presets (EffectPresets::userStore(), p.getPresetKey(), [&p] { return p.getPresetValues(); },
               [&p] (const EffectPresets::Values& v) { p.setPresetValues (v); })
{
    setOpaque (true);
    presets.carryTexts ([&p] { return p.getPresetTexts(); }, [&p] (const EffectPresets::Texts& t) { p.setPresetTexts (t); });

    for (auto* panel : { &padPanel, &soundPanel, &kitPanel, &busPanel, &spacePanel })
        addAndMakeVisible (*panel);

    // The sounds' pads in their order, then the user pads.
    for (size_t place = 0; place < (size_t) DrumMachine::numPads; ++place)
    {
        pads.push_back (std::make_unique<Pad> (padAtPlace (place), [this] (int pad) { clickPad (pad); },
                                               [this] (int pad) { selectPad (pad); showPadMenu (pad); }));
        padPanel.addAndMakeVisible (*pads.back());
    }

    describePads();
    padMenu.setTooltip ("This pad: its choke group; its name and colour while it plays samples; a user pad can also move to another note or be emptied (a right-click on the pad does the same)");
    padMenu.setWantsKeyboardFocus (false);
    padMenu.onClick = [this] { showPadMenu (selectedPad); };
    soundPanel.addAndMakeVisible (padMenu);

    picture = std::make_unique<Picture>();
    soundPanel.addAndMakeVisible (*picture);
    layers = std::make_unique<Layers> (drums, [this] { chooseSamples(); });
    soundPanel.addChildComponent (*layers);

    // Under everything else of the panel, so the filter's controls sit on its first page.
    effects = std::make_unique<DrumPadEffectsView> (drums);
    effects->onPageChanged = [this] { showEffectsPage(); resized(); };
    soundPanel.addAndMakeVisible (*effects);

    for (auto* k : soundKnobs())
        soundPanel.addAndMakeVisible (*k);

    for (auto& r : readouts)
        soundPanel.addAndMakeVisible (r);

    tune.setBipolar (true);
    tone.setBipolar (true);
    pan.setBipolar (true);
    tone.setFormatter ([] (float v) { return SoundControls::formatValue (v, "+%"); });
    pan.setFormatter (panText);
    tune.setTooltip ("Pitch in semitones");
    decay.setTooltip ("How long the sound takes to fall 60 dB");
    tone.setTooltip ("Darker or brighter: the click, the noise's filter, the strike, the top end");
    level.setTooltip ("The sound's level in the kit");
    pan.setTooltip ("Where it sits, left to right");
    drive.setTooltip ("Saturation on this sound alone");
    cutoff.setTooltip ("Where the filter cuts, or the middle of its band");
    resonance.setTooltip ("How far the filter rings at its cutoff");
    crush.setTooltip ("Fewer bits and a lower sample rate: from a little grit to a broken toy");
    reverb.setTooltip ("How much of the sound goes to the kit's reverb");
    delay.setTooltip ("How much of the sound goes to the kit's delay");
    start.setTooltip ("Where in the sample a hit starts, as a share of its length");
    bend.setBipolar (true);
    bend.setTooltip ("A pitch envelope: how many semitones off its pitch a hit starts, coming to pitch over about a tenth of a second");
    reverse.setTooltip ("Plays the sample backwards");
    soundPanel.addChildComponent (reverse);
    files.setTooltip ("Kit files: open a kit saved with its samples, save this kit with copies of its samples in one folder, find samples that have moved");
    files.setWantsKeyboardFocus (false);
    files.onClick = [this] { showFilesMenu(); };
    kitPanel.addAndMakeVisible (files);

    filter.setChoices (DrumMachine::filterNames());
    filter.setTooltip ("A filter on this sound alone: off, low-pass, band-pass or high-pass");
    filter.onChange = [this] (int) { refreshFilterKnobs(); };
    soundPanel.addAndMakeVisible (filter);

    character.setChoices (DrumMachine::kickCharacterNames());
    character.attach (drums.kickCharacterParameter());
    character.setTooltip ("The kick's character: Boom (long and round), Punch (909-style), Hard (an overdriven industrial kick)");
    soundPanel.addChildComponent (character);

    type.setTooltip ("What the pad plays. A sound's own types: Classic, as it has always been; Modern, a second model with more to it, duller the softer it is struck. "
                     "Sample plays the sounds given to the pad. Or one of the sources: 24 more synthesized sounds, which any pad can play in place of its own");
    type.onChange = [this] (int index)
    {
        // Past the pad's own choices, a source; among them, no source and (for a sound) its type.
        const auto first = firstSourceIndex();

        if (index >= first)
        {
            drums.setSource (selectedPad, index - first + 1);
        }
        else
        {
            drums.setSource (selectedPad, 0);

            if (selectedPad < DrumMachine::numSounds)
            {
                auto* choice = drums.typeParameter (selected);
                choice->beginChangeGesture();
                choice->setValueNotifyingHost (choice->convertTo0to1 ((float) index));
                choice->endChangeGesture();
            }
        }

        showSelected();
    };
    soundPanel.addAndMakeVisible (type);

    juce::StringArray kitNames;

    for (auto& kit : DrumMachine::kits())
        kitNames.add (kit.name);

    kits.setChoices (kitNames);
    kits.setIndex (juce::jmax (0, DrumMachine::kitIndex (drums.getKitName())), false);
    kits.onChange = [this] (int i)
    {
        drums.applyKit (DrumMachine::kits()[(size_t) i]);
        showSelected();
    };
    kits.setTooltip ("The factory kits: every sound's settings, its effects, and the bus and space at once");
    kitPanel.addAndMakeVisible (kits);
    kitPanel.addAndMakeVisible (presets);

    kitDrive.attach (drums.kitDriveParameter());
    glue.attach (drums.glueParameter());
    kitLevel.attach (drums.levelParameter());
    kitDrive.setTooltip ("The whole kit through a soft clipper");
    glue.setTooltip ("A bus compressor that holds the kit together: more turns it down further, harder");
    kitLevel.setTooltip ("The kit's output level");

    for (auto* k : { &kitDrive, &glue, &kitLevel })
        busPanel.addAndMakeVisible (*k);

    glueMeter = std::make_unique<GlueMeter>();
    glueMeter->setTooltip ("How far the glue is turning the kit down");
    busPanel.addAndMakeVisible (*glueMeter);

    reverbSize.attach (drums.reverbSizeParameter());
    feedback.attach (drums.feedbackParameter());
    reverbSize.setTooltip ("The reverb's room, from small to a hall");
    feedback.setTooltip ("How much of each echo comes round again");
    delayTime.setChoices (DrumMachine::delayTimeNames());
    delayTime.attach (drums.delayTimeParameter());
    delayTime.setTooltip ("The delay's time in beats of the song's tempo; each echo goes left, then right");

    for (juce::Component* c : { (juce::Component*) &reverbSize, (juce::Component*) &delayTime, (juce::Component*) &feedback })
        spacePanel.addAndMakeVisible (*c);

    setSize (windowWidth, windowHeight);
    select (Sound::kick);
    startTimerHz (30);
}

DrumMachineEditor::~DrumMachineEditor()
{
    stopTimer();
}

const std::array<Sound, (size_t) DrumMachine::numSounds>& DrumMachineEditor::padOrder()
{
    static const std::array<Sound, (size_t) DrumMachine::numSounds> order {{
        Sound::kick, Sound::snare, Sound::clap, Sound::rim, Sound::closedHat, Sound::openHat,
        Sound::pedalHat, Sound::lowTom, Sound::midTom, Sound::highTom, Sound::crash, Sound::ride,
        Sound::rideBell, Sound::china, Sound::splash, Sound::cowbell, Sound::tambourine, Sound::shaker,
        Sound::bongo, Sound::openConga, Sound::muteConga, Sound::timbale, Sound::agogo, Sound::claves,
        Sound::woodBlock, Sound::triangle, Sound::cuica, Sound::whistle, Sound::guiro, Sound::vibraslap,
    }};
    return order;
}

juce::Colour DrumMachineEditor::colourOf (Sound s)
{
    // Like the track colours, the same in every theme: they mean the family.
    switch (s)
    {
        case Sound::kick:                                                     return juce::Colour (0xffe8604c);
        case Sound::snare: case Sound::clap: case Sound::rim:                 return juce::Colour (0xffe9a23b);
        case Sound::closedHat: case Sound::pedalHat: case Sound::openHat:     return juce::Colour (0xff2fb8a6);
        case Sound::lowTom: case Sound::midTom: case Sound::highTom:          return juce::Colour (0xffa678e0);
        case Sound::crash: case Sound::ride: case Sound::rideBell:
        case Sound::china: case Sound::splash:                                return juce::Colour (0xff4f9be8);
        case Sound::cowbell: case Sound::tambourine: case Sound::shaker: case Sound::vibraslap:
        case Sound::bongo: case Sound::muteConga: case Sound::openConga: case Sound::timbale:
        case Sound::agogo: case Sound::whistle: case Sound::guiro: case Sound::claves:
        case Sound::woodBlock: case Sound::cuica: case Sound::triangle:       return juce::Colour (0xff7cc35a);
        case Sound::count:                                                    break;
    }

    return Theme::accent;
}

int DrumMachineEditor::padAtPlace (size_t place) const
{
    return place < padOrder().size() ? (int) padOrder()[place] : (int) place;
}

juce::Colour DrumMachineEditor::colourOfPad (int pad) const
{
    // The colour given to it (a sound's pad shows its own only while it plays samples); else its
    // family's, or for a user pad a quiet slate.
    const auto own = drums.getPadColour (pad);

    if (! own.isTransparent())
        return own;

    return pad < DrumMachine::numSounds ? colourOf ((Sound) pad) : juce::Colour (0xff8f9bb3);
}

void DrumMachineEditor::describePads()
{
    const auto held = drums.getSamples();

    for (size_t place = 0; place < pads.size(); ++place)
    {
        const auto pad = padAtPlace (place);
        const auto user = pad >= DrumMachine::numSounds;
        const auto note = drums.getPadNote (pad);
        pads[place]->describe (drums.getPadName (pad), user ? juce::String (note) + " " + noteName (note) : notesOf ((Sound) pad), colourOfPad (pad),
                               user && drums.getSource (pad) == 0 && (held == nullptr || held->pads[(size_t) pad].empty()));
    }
}

juce::String DrumMachineEditor::getPadText (int pad) const
{
    for (size_t place = 0; place < pads.size(); ++place)
        if (padAtPlace (place) == pad)
            return pads[place]->getText();

    return {};
}

void DrumMachineEditor::clickPad (int pad)
{
    drums.auditionPad (pad, (float) InstrumentKeys::velocityFor (*this) / 127.0f);
    selectPad (pad);
}

juce::Rectangle<int> DrumMachineEditor::padBounds (int pad) const
{
    for (size_t place = 0; place < pads.size(); ++place)
        if (padAtPlace (place) == pad)
            return pads[place]->getBounds() + padPanel.getPosition();

    return {};
}

void DrumMachineEditor::selectPad (int pad)
{
    selectedPad = juce::jlimit (0, DrumMachine::numPads - 1, pad);

    if (selectedPad < DrumMachine::numSounds)
        selected = (Sound) selectedPad;

    for (size_t place = 0; place < pads.size(); ++place)
        pads[place]->setSelected (padAtPlace (place) == selectedPad);

    showSelected();
}

void DrumMachineEditor::showSelected()
{
    const auto user = selectedPad >= DrumMachine::numSounds;
    const auto colour = colourOfPad (selectedPad);
    soundPanel.setTitle (drums.getPadName (selectedPad));
    soundPanel.setAccent (colour);

    tune.attach (drums.parameterOf (selectedPad, "tune"));
    decay.attach (drums.parameterOf (selectedPad, "decay"));
    tone.attach (drums.parameterOf (selectedPad, "tone"));
    level.attach (drums.parameterOf (selectedPad, "level"));
    pan.attach (drums.parameterOf (selectedPad, "pan"));
    drive.attach (drums.parameterOf (selectedPad, "drive"));
    start.attach (drums.parameterOf (selectedPad, "start"));
    bend.attach (drums.parameterOf (selectedPad, "bend"));
    reverse.attach (drums.reverseParameter (selectedPad));
    reverse.setColour (colour);
    cutoff.attach (drums.parameterOf (selectedPad, "cutoff"));
    resonance.attach (drums.parameterOf (selectedPad, "resonance"));
    crush.attach (drums.parameterOf (selectedPad, "crush"));
    reverb.attach (drums.parameterOf (selectedPad, "reverb"));
    delay.attach (drums.parameterOf (selectedPad, "delay"));
    filter.attach (drums.filterParameter (selectedPad));
    filter.setColour (colour);

    for (auto* k : soundKnobs())
        k->setColour (colour);

    refreshFilterKnobs();
    effects->show (selectedPad, colour);
    showEffectsPage();

    character.setVisible (! user && selected == Sound::kick);

    // A sound's type (a user pad has none: it plays samples, and has a menu of its own there), and
    // for a Sample its layers and its own three controls.
    {
        // In its menu the sources come under their families.
        std::vector<std::pair<int, juce::String>> sections;
        juce::String family;

        for (int s = 1; s <= DrumMachine::numSources; ++s)
            if (const auto& info = DrumMachine::sourceInfo (s); family != info.family)
                sections.emplace_back (firstSourceIndex() + s - 1, family = info.family);

        type.setChoices (typeChoices());
        type.setSections (std::move (sections));
        type.setIndex (typeIndexNow(), false);
        type.setColour (colour);
    }
    shownSample = drums.playsSamples (selectedPad);
    samplesSeen = drums.getSamplesVersion();
    padsSeen = drums.getPadsVersion();
    describePads();
    layers->show (selectedPad, colour);
    layers->setVisible (shownSample);
    reverse.setVisible (shownSample);

    for (size_t i : { (size_t) 11, (size_t) 12 })
    {
        soundKnobs()[i]->setVisible (shownSample);
        readouts[i].setVisible (shownSample);
    }

    for (auto* k : { &level, &pan, &drive })
        k->setSize (shownSample ? SoundControls::Knob::Size::small : SoundControls::Knob::Size::normal);

    markSampledPads();
    refreshReadouts();
    refreshPicture (true);
    resized();
    repaint();
}

void DrumMachineEditor::refreshFilterKnobs()
{
    // Cutoff and resonance do nothing with the filter off, so they dim.
    const auto on = filter.getIndex() > 0;

    for (auto* k : { &cutoff, &resonance })
        k->setAlpha (on ? 1.0f : 0.4f);

    for (size_t i : { (size_t) 6, (size_t) 7 })
        readouts[i].setAlpha (on ? 1.0f : 0.4f);

    effects->setFilterLit (on);
}

void DrumMachineEditor::showEffectsPage()
{
    // The filter, the crusher and the sends are the first page's; on the others they give way.
    const auto first = effects->getPage() == DrumPadEffectsView::filterPage;
    filter.setVisible (first);

    for (size_t i = 6; i < 11; ++i)
    {
        soundKnobs()[i]->setVisible (first);
        readouts[i].setVisible (first);
    }
}

void DrumMachineEditor::refreshReadouts()
{
    const auto knobs = soundKnobs();

    for (size_t i = 0; i < knobs.size(); ++i)
        if (const auto text = knobs[i]->getValueText(); text != readouts[i].getText())
            readouts[i].setText (text);
}

void DrumMachineEditor::refreshPicture (bool force)
{
    const auto now = drums.currentSettings();
    const auto user = selectedPad >= DrumMachine::numSounds;
    const auto settingsOf = [this, user] (const DrumMachine::KitSettings& k) -> const DrumMachine::Settings&
    {
        return user ? k.user[(size_t) (selectedPad - DrumMachine::numSounds)].settings : k.sounds[(size_t) selectedPad];
    };

    if (! force && hasPicture && settingsOf (pictured) == settingsOf (now) && (selectedPad != (int) Sound::kick || pictured.kick == now.kick))
        return;

    pictured = now;
    hasPicture = true;

    // As long as its decay says, or for a sample that plays whole, as long as its longest layer.
    auto guess = juce::jlimit (0.05, 6.5, settingsOf (now).decay * 0.00125 + 0.06);

    if (drums.playsSamples (selectedPad))
    {
        auto longest = 0.05;

        for (auto& l : drums.getLayers (selectedPad))
            if (l.sample != nullptr)
                longest = juce::jmax (longest, (double) l.sample->getNumFrames() / l.sample->rate);

        guess = juce::jmin (guess, longest * 4.0 + 0.1);
    }

    const auto hit = DrumMachine::Engine::renderPad (now, selectedPad, pictureRate, guess, drums.getSamples());
    picture->set (hit, pictureRate, settingsOf (now).decay, colourOfPad (selectedPad));
    pictureSeconds = picture->getSeconds();
    effects->soundChanged (pictureSeconds);
}

juce::String DrumMachineEditor::getKitText() const
{
    return drums.getKitName() + (drums.kitModified() ? " (edited)" : "");
}

juce::String DrumMachineEditor::getTypeText() const
{
    return type.getText();
}

int DrumMachineEditor::firstSourceIndex() const
{
    return selectedPad >= DrumMachine::numSounds ? 1 : DrumMachine::typeNamesFor (selected).size();
}

int DrumMachineEditor::typeIndexNow() const
{
    if (const auto source = drums.getSource (selectedPad); source > 0)
        return firstSourceIndex() + source - 1;

    return selectedPad >= DrumMachine::numSounds ? 0 : drums.typeParameter (selected)->getIndex();
}

juce::StringArray DrumMachineEditor::typeChoices() const
{
    // The pad's own choices (a sound's types; for a user pad, Sample), then the sources.
    auto choices = selectedPad >= DrumMachine::numSounds ? juce::StringArray { "Sample" } : DrumMachine::typeNamesFor (selected);

    for (int s = 1; s <= DrumMachine::numSources; ++s)
        choices.add (DrumMachine::sourceInfo (s).name);

    return choices;
}

bool DrumMachineEditor::chooseType (const juce::String& name)
{
    const auto index = typeChoices().indexOf (name);

    if (index < 0)
        return false;

    type.setIndex (index);
    return true;
}

int DrumMachineEditor::getNumLayersShown() const            { return layers->getNumRows(); }
juce::String DrumMachineEditor::getLayerText (int row) const { return layers->rowText (row); }
bool DrumMachineEditor::showsLayers() const                 { return layers->isVisible(); }
void DrumMachineEditor::selectLayer (int row)               { layers->select (row); }
void DrumMachineEditor::pressLayerButton (const juce::String& which)    { layers->press (which); }
int DrumMachineEditor::getLitLayer() const                  { return layers->getLitRow(); }
juce::String DrumMachineEditor::getHitText() const          { return layers->hitText(); }

void DrumMachineEditor::say (const juce::String& text)
{
    message = text;
    messageUntil = juce::Time::getMillisecondCounterHiRes() + 9000.0;
    repaint();
}

void DrumMachineEditor::markSampledPads()
{
    const auto held = drums.getSamples();

    // A sound's pad when its type is Sample; a user pad once it has a layer.
    for (size_t place = 0; place < pads.size(); ++place)
    {
        const auto pad = padAtPlace (place);
        pads[place]->setSampled (pad < DrumMachine::numSounds ? drums.playsSamples (pad) : held != nullptr && ! held->pads[(size_t) pad].empty());
    }
}

//==============================================================================
std::optional<int> DrumMachineEditor::padAt (juce::Point<int> p) const
{
    for (size_t place = 0; place < pads.size(); ++place)
        if ((pads[place]->getBounds() + padPanel.getPosition()).contains (p))
            return padAtPlace (place);

    if (soundPanel.getBounds().contains (p))
        return selectedPad;

    return {};
}

void DrumMachineEditor::setDropTarget (std::optional<int> s)
{
    if (s == dropTarget)
        return;

    dropTarget = s;

    for (size_t place = 0; place < pads.size(); ++place)
        pads[place]->setDropping (s.has_value() && padAtPlace (place) == *s);

    repaint();
}

void DrumMachineEditor::changeUserPad (int pad, const juce::String& what, int value)
{
    const auto user = pad >= DrumMachine::numSounds;

    if (what == "clear" && user)
        drums.clearPad (pad);
    else if (what == "clear")
    {
        // A sound's pad: its own name and colour back.
        drums.setPadName (pad, {});
        drums.setPadColour (pad, {});
    }
    else if (what == "note" && user)
        drums.setPadNote (pad, value);
    else if (what == "colour")
        drums.setPadColour (pad, value == 0 ? juce::Colour() : juce::Colour ((juce::uint32) value));
    else if (what == "choke")
        drums.setChoke (pad, value);

    showSelected();
}

juce::String DrumMachineEditor::getChokeText() const
{
    // The group built in that a sound is in, then the user's group with who else is in it.
    const auto user = selectedPad >= DrumMachine::numSounds;
    const auto group = drums.getChoke (selectedPad);
    juce::String builtIn;

    if (! user && (selected == Sound::closedHat || selected == Sound::pedalHat || selected == Sound::openHat || selected == Sound::triangle || selected == Sound::cuica))
        builtIn = chokesOf (selected);

    if (group == 0)
        return builtIn.isNotEmpty() ? builtIn : user ? juce::String ("Plays with whatever else is on its note; a hit struck again fades the last") : chokesOf (selected);

    juce::StringArray others;

    for (int pad = 0; pad < DrumMachine::numPads; ++pad)
        if (pad != selectedPad && drums.getChoke (pad) == group)
            others.add (drums.getPadName (pad));

    const auto more = others.size() - 2;

    if (more > 0)
    {
        others.removeRange (2, more);
        others.add (juce::String (more) + " more");
    }

    return (builtIn.isNotEmpty() ? builtIn + "; " : juce::String()) + "Choke group " + juce::String (group)
         + (others.isEmpty() ? juce::String (", alone in it") : " with " + others.joinIntoString (", "));
}

void DrumMachineEditor::showPadMenu (int pad)
{
    const auto user = pad >= DrumMachine::numSounds;

    // A sound's pad is renamed and coloured while it plays samples (its note is its own); any pad can be in a choke group.
    const auto named = user || drums.playsSamples (pad) || drums.getSource (pad) > 0;
    juce::PopupMenu chokeMenu;
    const auto group = drums.getChoke (pad);
    chokeMenu.addItem (3000, "None", true, group == 0);

    for (int g = 1; g <= DrumMachine::numChokeGroups; ++g)
    {
        juce::StringArray in;

        for (int other = 0; other < DrumMachine::numPads; ++other)
            if (other != pad && drums.getChoke (other) == g)
                in.add (drums.getPadName (other));

        chokeMenu.addItem (3000 + g, "Group " + juce::String (g) + (in.isEmpty() ? juce::String() : "   with " + in.joinIntoString (", ")), true, group == g);
    }

    // The notes by octave, each with the sound or pads already on it; and a few colours.
    juce::PopupMenu notes;
    const auto now = drums.getPadNote (pad);

    for (int octave = 0; octave < 11; ++octave)
    {
        juce::PopupMenu sub;
        auto holdsNow = false;

        for (int note = octave * 12; note < juce::jmin (128, octave * 12 + 12); ++note)
        {
            juce::StringArray on;

            if (const auto sound = DrumMachine::soundFor (note))
                on.add (DrumMachine::nameOf (*sound));

            for (int other = DrumMachine::numSounds; other < DrumMachine::numPads; ++other)
                if (other != pad && drums.getPadNote (other) == note && ! drums.getLayers (other).empty())
                    on.add (drums.getPadName (other));

            sub.addItem (1000 + note, juce::String (note) + "  " + noteName (note) + (on.isEmpty() ? juce::String() : "   with " + on.joinIntoString (", ")), true, note == now);
            holdsNow = holdsNow || note == now;
        }

        notes.addSubMenu (noteName (octave * 12) + " to " + noteName (juce::jmin (127, octave * 12 + 11)), sub, true, nullptr, holdsNow);
    }

    static const std::pair<const char*, juce::uint32> colours[] { { "Red", 0xffe8604c }, { "Orange", 0xffe9a23b }, { "Yellow", 0xffd9c545 }, { "Green", 0xff7cc35a },
                                                                  { "Teal", 0xff2fb8a6 }, { "Blue", 0xff4f9be8 }, { "Purple", 0xffa678e0 }, { "Pink", 0xffe07ab0 } };
    juce::PopupMenu colourMenu;
    const auto own = drums.getPadColour (pad);
    colourMenu.addItem (2000, "None", true, own.isTransparent());

    for (int i = 0; i < (int) std::size (colours); ++i)
        colourMenu.addItem (2001 + i, colours[i].first, true, own == juce::Colour (colours[i].second));

    juce::PopupMenu menu;
    menu.addSectionHeader (drums.getPadName (pad));
    menu.addItem (1, "Rename...", named);

    if (user)
        menu.addSubMenu ("Note (" + juce::String (now) + " " + noteName (now) + ")", notes);

    menu.addSubMenu ("Colour", colourMenu, named);
    menu.addSubMenu (group == 0 ? juce::String ("Choke Group") : "Choke Group (" + juce::String (group) + ")", chokeMenu);
    menu.addSeparator();

    if (user)
        menu.addItem (2, "Empty This Pad");
    else if (named)
        menu.addItem (2, "Use " + DrumMachine::nameOf ((Sound) pad) + "'s Own Name and Colour", drums.hasOwnLook (pad));
    else
        menu.addItem (2, "A name and a colour of its own: set its type to Sample", false);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (padBounds (pad) + getScreenPosition()),
                        [safe = juce::Component::SafePointer<DrumMachineEditor> (this), pad] (int chosen)
    {
        if (safe == nullptr || chosen == 0)
            return;

        if (chosen == 1)
        {
            auto* box = new juce::AlertWindow ("Rename " + safe->drums.getPadName (pad), {}, juce::MessageBoxIconType::NoIcon, safe.getComponent());
            box->addTextEditor ("name", safe->drums.getPadName (pad));
            box->addButton ("Rename", 1, juce::KeyPress (juce::KeyPress::returnKey));
            box->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
            box->enterModalState (true, juce::ModalCallbackFunction::create ([safe, box, pad] (int result)
            {
                if (safe != nullptr && result == 1)
                {
                    safe->drums.setPadName (pad, box->getTextEditorContents ("name"));
                    safe->showSelected();
                }
            }), true);
        }
        else if (chosen == 2)
            safe->changeUserPad (pad, "clear");
        else if (chosen >= 3000)
            safe->changeUserPad (pad, "choke", chosen - 3000);
        else if (chosen >= 2000)
            safe->changeUserPad (pad, "colour", chosen == 2000 ? 0 : (int) colours[chosen - 2001].second);
        else if (chosen >= 1000)
            safe->changeUserPad (pad, "note", chosen - 1000);
    });
}

bool DrumMachineEditor::addFiles (int s, const juce::Array<juce::File>& sounds)
{
    const auto before = drums.getLayers (s).size();
    const auto error = drums.addFiles (s, sounds);
    selectPad (s);

    if (error.isNotEmpty())
        say (error);

    return drums.getLayers (s).size() > before;
}

bool DrumMachineEditor::isInterestedInFileDrag (const juce::StringArray& dragged)
{
    for (auto& f : dragged)
        if (juce::File (f).hasFileExtension (soundFileTypes) || DrumKitImport::isBatteryKit (juce::File (f)) || juce::File (f).isDirectory())
            return true;

    return false;
}

void DrumMachineEditor::fileDragMove (const juce::StringArray&, int x, int y)      { setDropTarget (padAt ({ x, y })); }
void DrumMachineEditor::fileDragExit (const juce::StringArray&)                    { setDropTarget ({}); }

void DrumMachineEditor::filesDropped (const juce::StringArray& dropped, int x, int y)
{
    // A Battery kit or a folder of sounds dropped anywhere on the window is imported, in place of the kit here.
    if (const juce::File one (dropped[0]); dropped.size() == 1 && (DrumKitImport::isBatteryKit (one) || one.isDirectory()))
    {
        setDropTarget ({});
        importKit (one, one.isDirectory());
        return;
    }

    const auto to = padAt ({ x, y });
    setDropTarget ({});

    if (! to.has_value())
    {
        say ("Drop sounds on a pad, or on the selected sound's panel.");
        return;
    }

    juce::Array<juce::File> sounds;

    for (auto& f : dropped)
        if (juce::File (f).hasFileExtension (soundFileTypes))
            sounds.add (juce::File (f));

    addFiles (*to, sounds);
}

bool DrumMachineEditor::isInterestedInDragSource (const SourceDetails& details)    { return SoundDrops::accepts (details.description); }
void DrumMachineEditor::itemDragMove (const SourceDetails& details)                { setDropTarget (padAt (details.localPosition)); }
void DrumMachineEditor::itemDragExit (const SourceDetails&)                        { setDropTarget ({}); }

void DrumMachineEditor::itemDropped (const SourceDetails& details)
{
    const auto to = padAt (details.localPosition);
    setDropTarget ({});

    if (! to.has_value())
    {
        say ("Drop sounds on a pad, or on the selected sound's panel.");
        return;
    }

    const auto sound = SoundDrops::take (details.description, Sampler::longestSampleSeconds);

    if (! sound.has_value())
        return;

    // A saved file as it stands is sampled where it is; anything else is written to the Samples folder first.
    auto file = sound->file;

    if (file == juce::File())
    {
        juce::String error;
        file = Sampler::writeSample (sound->audio, sound->sampleRate, sound->name, sound->markers, error);

        if (file == juce::File())
        {
            say (error);
            return;
        }
    }

    addFiles (*to, { file });
}

void DrumMachineEditor::chooseSamples()
{
    chooser = std::make_unique<juce::FileChooser> ("Choose sounds for " + drums.getPadName (selectedPad), juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3;*.m4a;*.caf");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems,
                          [safe = juce::Component::SafePointer<DrumMachineEditor> (this), to = selectedPad] (const juce::FileChooser& c)
                          {
                              if (safe != nullptr && ! c.getResults().isEmpty())
                                  safe->addFiles (to, c.getResults());
                          });
}

void DrumMachineEditor::importKit (const juce::File& from, bool folder)
{
    const auto done = [safe = juce::Component::SafePointer<DrumMachineEditor> (this)] (const juce::String& said)
    {
        if (safe != nullptr)
        {
            safe->showSelected();
            safe->say (said);
        }
    };

    if (folder)
        DrumKitImport::importFolder (*this, drums, chooser, done, from);
    else
        DrumKitImport::importBatteryKit (*this, drums, chooser, done, from);
}

void DrumMachineEditor::showFilesMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "Open Kit File...");
    menu.addItem (2, "Save Kit with Samples...");
    menu.addSeparator();
    menu.addItem (4, "Import Battery Kit...");
    menu.addItem (5, "Import Folder of Sounds...");
    menu.addSeparator();
    menu.addItem (3, "Find Missing Samples...", ! drums.getMissing().isEmpty());

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (files), [safe = juce::Component::SafePointer<DrumMachineEditor> (this)] (int chosen)
    {
        if (safe == nullptr || chosen == 0)
            return;

        auto& self = *safe;
        using Flags = juce::FileBrowserComponent;

        if (chosen == 1)
        {
            self.chooser = std::make_unique<juce::FileChooser> ("Open a Basic Drums kit", juce::File(), juce::String ("*") + BuiltInEffects::BasicDrumsProcessor::kitFileExtension);
            self.chooser->launchAsync (Flags::openMode | Flags::canSelectFiles, [safe] (const juce::FileChooser& c)
            {
                if (safe == nullptr || c.getResult() == juce::File())
                    return;

                const auto error = safe->drums.loadKitFile (c.getResult());
                safe->showSelected();
                const auto missing = safe->drums.getMissing();
                safe->say (error.isNotEmpty() ? error : missing.isEmpty() ? "Opened " + c.getResult().getFileNameWithoutExtension() + "."
                                                                           : juce::String (missing.size()) + " of its samples could not be found: " + missing[0]);
            });
        }
        else if (chosen == 2)
        {
            self.chooser = std::make_unique<juce::FileChooser> ("Save the kit and its samples into a new folder", juce::File(), juce::String());
            self.chooser->launchAsync (Flags::saveMode | Flags::canSelectFiles | Flags::warnAboutOverwriting, [safe] (const juce::FileChooser& c)
            {
                if (safe == nullptr || c.getResult() == juce::File())
                    return;

                juce::StringArray missing;
                const auto folder = c.getResult().withFileExtension ({});
                const auto error = safe->drums.saveKitWithSamples (folder, missing);
                safe->say (error.isNotEmpty() ? error : missing.isEmpty() ? "Saved " + folder.getFileName() + " with its samples."
                                                                           : "Saved " + folder.getFileName() + ", without " + juce::String (missing.size()) + " samples that are not where the kit says.");
            });
        }
        else if (chosen == 4 || chosen == 5)
        {
            self.importKit ({}, chosen == 5);
        }
        else if (chosen == 3)
        {
            self.chooser = std::make_unique<juce::FileChooser> ("Find a missing sample, or the folder they are in", juce::File(), "*");
            self.chooser->launchAsync (Flags::openMode | Flags::canSelectFiles | Flags::canSelectDirectories, [safe] (const juce::FileChooser& c)
            {
                if (safe == nullptr || c.getResult() == juce::File())
                    return;

                const auto found = safe->drums.locate (c.getResult());
                safe->showSelected();
                safe->say (found == 0 ? juce::String ("None of the missing samples are there.") : juce::String (found) + (found == 1 ? " layer has" : " layers have") + " its sample back.");
            });
        }
    });
}

juce::String DrumMachineEditor::getKnobText (const juce::String& what) const
{
    for (auto* k : const_cast<DrumMachineEditor*> (this)->soundKnobs())
        if (k->getLabel().equalsIgnoreCase (what))
            return k->getValueText();

    if (what.equalsIgnoreCase ("Filter"))
        return filter.getText();

    return {};
}

void DrumMachineEditor::showHits()
{
    // The pads light at each new hit; the picture sweeps when its own sound plays, and the layers
    // say the hit's velocity and light the one it played.
    const auto& engine = drums.getEngine();

    for (size_t i = 0; i < pads.size(); ++i)
    {
        const auto pad = padAtPlace (i);
        const auto count = engine.hitsOfPad (pad);

        if (count != seenHits[i])
        {
            seenHits[i] = count;
            const auto velocity = engine.lastVelocityOfPad (pad);
            pads[i]->hit (velocity);

            if (pad == selectedPad)
            {
                picture->sweep();

                if (shownSample)
                        layers->played (juce::jlimit (1, 127, juce::roundToInt (velocity * 127.0f)), engine.lastLayerOfPad (pad));
            }
        }
        else
        {
            pads[i]->fade();
        }
    }

    layers->fade();
}

void DrumMachineEditor::timerCallback()
{
    for (auto* k : soundKnobs())
        k->refresh();

    for (auto* k : { &kitDrive, &glue, &kitLevel, &reverbSize, &feedback })
        k->refresh();

    refreshReadouts();

    character.refresh();
    delayTime.refresh();
    reverse.refresh();

    // The type and the layers may have changed from outside the window (a preset, a kit, the host).
    if (typeIndexNow() != type.getIndex()
            || drums.playsSamples (selectedPad) != shownSample || drums.getSamplesVersion() != samplesSeen || drums.getPadsVersion() != padsSeen)
        showSelected();

    markSampledPads();

    if (message.isNotEmpty() && juce::Time::getMillisecondCounterHiRes() > messageUntil)
    {
        message.clear();
        repaint();
    }

    if (const auto was = filter.getIndex(); (filter.refresh(), filter.getIndex() != was))
        refreshFilterKnobs();

    showHits();
    effects->refresh();

    if (picture->isSweeping())
        picture->repaint();

    refreshPicture (false);

    if (const auto index = DrumMachine::kitIndex (drums.getKitName()); index >= 0 && index != kits.getIndex())
        kits.setIndex (index, false);

    if (const auto modified = drums.kitModified(); modified != shownModified)
    {
        shownModified = modified;
        kitPanel.repaint();
        repaint();
    }

    glueMeter->setDb (drums.getEngine().getReductionDb());
}

void DrumMachineEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::base);

    // "edited" beside the kit's name when a knob has moved since it was chosen.
    if (shownModified)
    {
        g.setColour (Theme::dimText);
        g.setFont (juce::FontOptions (11.0f, juce::Font::italic));
        g.drawText ("edited", kits.getBounds().translated (kitPanel.getX(), kitPanel.getY()).withX (kitPanel.getX() + kits.getRight() + 6).withWidth (50),
                    juce::Justification::centredLeft);
    }
}

void DrumMachineEditor::resized()
{
    auto r = getLocalBounds().reduced (12);

    // Along the top: the kit (its selector over the user's presets), the bus (drive, glue and
    // level) and the space (the reverb's size, the delay's time and feedback).
    auto top = r.removeFromTop (100);
    spacePanel.setBounds (top.removeFromRight (300));
    top.removeFromRight (10);
    busPanel.setBounds (top.removeFromRight (300));
    top.removeFromRight (10);
    kitPanel.setBounds (top);
    {
        auto c = kitPanel.getContentBounds();
        auto first = c.removeFromTop (c.getHeight() / 2).withSizeKeepingCentre (c.getWidth(), 28);
        files.setBounds (first.removeFromRight (54).reduced (0, 2));
        kits.setBounds (first.removeFromLeft (juce::jmin (260, first.getWidth() - 64)));
        presets.setBounds (c.withSizeKeepingCentre (c.getWidth(), 28));
    }
    {
        auto c = busPanel.getContentBounds();
        glueMeter->setBounds (c.removeFromRight (70).withSizeKeepingCentre (70, 24));
        const auto w = c.getWidth() / 3;
        kitDrive.setBounds (c.removeFromLeft (w));
        glue.setBounds (c.removeFromLeft (w));
        kitLevel.setBounds (c);
    }
    {
        auto c = spacePanel.getContentBounds();
        reverbSize.setBounds (c.removeFromLeft (76));
        feedback.setBounds (c.removeFromRight (76));
        delayTime.setBounds (c.withSizeKeepingCentre (c.getWidth() - 8, 28));
    }

    r.removeFromTop (10);

    // The pads on the left, six across.
    padPanel.setBounds (r.removeFromLeft (600));
    {
        const auto area = padPanel.getContentBounds().reduced (2, 2);
        constexpr int columns = 8, rows = 8;
        const auto w = area.getWidth() / columns, h = area.getHeight() / rows;

        for (size_t i = 0; i < pads.size(); ++i)
            pads[i]->setBounds (area.getX() + (int) (i % columns) * w, area.getY() + (int) (i / columns) * h, w, h);
    }

    r.removeFromLeft (10);

    // The selected sound: its picture, its knobs, what chokes it.
    soundPanel.setBounds (r);
    {
        auto header = soundPanel.getHeaderBounds();
        if (character.isVisible())
        {
            character.setBounds (header.removeFromRight (140).reduced (0, 2));
            header.removeFromRight (6);
        }

        type.setBounds (header.removeFromRight (140).reduced (0, 2));

        if (padMenu.isVisible())
        {
            header.removeFromRight (6);
            padMenu.setBounds (header.removeFromRight (50).reduced (0, 2));
        }

        if (shownSample)
        {
            header.removeFromRight (6);
            reverse.setBounds (header.removeFromRight (84).reduced (0, 2));
        }

        auto c = soundPanel.getContentBounds();
        c.removeFromBottom (20);   // the line about notes and chokes, painted by the panel's owner below

        // A Sample's picture is shorter, to make room for its layers under it.
        if (shownSample)
        {
            picture->setBounds (c.removeFromTop (76));
            c.removeFromTop (4);
            layers->setBounds (c.removeFromTop (90));
        }
        else
        {
            picture->setBounds (c.removeFromTop (170));
        }

        c.removeFromTop (8);
        // Each knob with its value in a readout under it, in real units.
        const auto knobs = soundKnobs();
        const auto place = [this, &knobs] (size_t i, juce::Rectangle<int> cell, int inset)
        {
            readouts[i].setBounds (cell.removeFromBottom (20).withSizeKeepingCentre (juce::jmin (76, cell.getWidth() - 2), 18));
            knobs[i]->setBounds (cell.reduced (inset, 0));
        };

        // Below the sound's own six, its effects, a page at a time. The first page is laid out
        // here: the filter's type, then five small knobs.
        effects->setBounds (c.removeFromBottom (118));
        c.removeFromBottom (2);
        auto fx = effects->getPageBounds().translated (effects->getX(), effects->getY());
        filter.setBounds (fx.removeFromLeft (112).withSizeKeepingCentre (112, 26).translated (0, -8));
        fx.removeFromLeft (4);
        const auto fw = fx.getWidth() / 5;

        for (size_t i = 6; i < 11; ++i)
            place (i, (i < 10 ? fx.removeFromLeft (fw) : fx), 2);

        auto big = c.removeFromTop (c.getHeight() * 11 / 20);
        const auto bw = big.getWidth() / 3;

        for (size_t i = 0; i < 3; ++i)
            place (i, big.removeFromLeft (bw).reduced (0, 2), 8);

        // Level, pan and drive; for a Sample, where it starts and its pitch envelope beside them.
        const auto across = shownSample ? 5 : 3;
        const auto sw = c.getWidth() / across;

        for (size_t i : { (size_t) 3, (size_t) 4, (size_t) 5, (size_t) 11, (size_t) 12 })
            if (i < 6 || shownSample)
                place (i, c.removeFromLeft (sw).reduced (0, 4), shownSample ? 6 : 18);
    }
}

void DrumMachineEditor::paintOverChildren (juce::Graphics& g)
{
    // Under the knobs: the sound's notes and what chokes it.
    auto line = soundPanel.getBounds().reduced (16, 8).removeFromBottom (16);
    g.setColour (Theme::dimText);
    g.setFont (juce::FontOptions (11.0f));
    const auto missing = drums.getMissing();

    if (message.isNotEmpty())
    {
        g.setColour (Theme::accentLight);
        g.drawText (message, line, juce::Justification::centredLeft);
    }
    else if (! missing.isEmpty())
    {
        g.setColour (Theme::warn);
        g.drawText (juce::String (missing.size()) + (missing.size() == 1 ? " sample is" : " samples are") + " missing: Files > Find Missing Samples", line, juce::Justification::centredLeft);
    }
    else
    {
        if (selectedPad < DrumMachine::numSounds)
            g.drawText ("Notes " + notesOf (selected) + "    " + getChokeText(), line, juce::Justification::centredLeft);
        else
            g.drawText ("Note " + juce::String (drums.getPadNote (selectedPad)) + " " + noteName (drums.getPadNote (selectedPad))
                            + "    " + getChokeText(), line, juce::Justification::centredLeft);
    }

    // A drag over the selected sound's panel: its edge lit, as a pad's is.
    if (dropTarget.has_value() && soundPanel.getBounds().contains (getMouseXYRelative()))
    {
        g.setColour (Theme::accent);
        g.drawRoundedRectangle (soundPanel.getBounds().toFloat().reduced (1.0f), 6.0f, 2.0f);
    }

}
