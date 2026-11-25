#include "DrumMachineEditor.h"
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

    constexpr const char* soundFileTypes = "wav;wave;bwf;aif;aiff;aifc;flac;ogg;opus;mp3;m4a;caf";

    juce::String panText (float v)
    {
        const auto r = juce::roundToInt (v);
        return r == 0 ? juce::String ("C") : (r < 0 ? "L " : "R ") + juce::String (std::abs (r));
    }
}

//==============================================================================
/** A pad: its sound's name and notes on a card in its family's colour, lit as it plays. */
class DrumMachineEditor::Pad final : public juce::Component
{
public:
    Pad (Sound s, std::function<void (Sound, float)> clicked)
        : sound (s), name (DrumMachine::nameOf (s)), notes (notesOf (s)), onClick (std::move (clicked))
    {
        setRepaintsOnMouseActivity (true);
        setTitle (name);
        setDescription ("Plays " + name + "; click higher for louder");
    }

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
        const auto colour = colourOf (sound);
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        constexpr auto corner = 6.0f;
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
        g.fillRoundedRectangle (r.getX() + 8.0f, r.getY() + 7.0f, 14.0f, 3.0f, 1.5f);

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

        auto text = r.reduced (8.0f, 6.0f);
        text.removeFromTop (8.0f);
        g.setColour (selected || glow > 0.3f ? Theme::textStrong : Theme::text);
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.drawFittedText (name, text.removeFromTop (text.getHeight() * 0.6f).toNearestInt(), juce::Justification::bottomLeft, 2, 0.9f);
        g.setColour (Theme::dimText);
        g.setFont (juce::FontOptions (10.5f));
        g.drawText (notes, text, juce::Justification::bottomLeft);

        // A pad that plays samples: three bars of a waveform in its corner.
        if (sampled)
        {
            g.setColour (colour.withAlpha (0.9f));
            const float heights[] { 4.0f, 9.0f, 6.0f, 3.0f };

            for (int i = 0; i < 4; ++i)
                g.fillRect (r.getRight() - 22.0f + 4.0f * (float) i, r.getY() + 12.0f - heights[i] * 0.5f, 2.0f, heights[i]);
        }

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
            return;

        onClick (sound, (float) e.y / (float) juce::jmax (1, getHeight()));
    }

    void mouseUp (const juce::MouseEvent&) override   { repaint(); }

private:
    Sound sound;
    juce::String name, notes;
    std::function<void (Sound, float)> onClick;
    bool selected = false, sampled = false, dropping = false;
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

        for (int i = 0; i < shown; ++i)
            peak = juce::jmax (peak, std::abs (mid (i)));

        // Each column's lowest, highest and loudest sample, for 400 columns.
        constexpr int columns = 400;
        lows.assign (columns, 0.0f);
        highs.assign (columns, 0.0f);
        levels.assign (columns, 0.0f);

        for (int i = 0; i < shown; ++i)
        {
            const auto col = juce::jmin (columns - 1, (int) ((double) i / shown * columns));
            const auto v = mid (i);
            lows[(size_t) col] = juce::jmin (lows[(size_t) col], v);
            highs[(size_t) col] = juce::jmax (highs[(size_t) col], v);
            levels[(size_t) col] = juce::jmax (levels[(size_t) col], std::abs (v));
        }

        // The level as a peak meter would hold it: each column the loudest of the next 20 ms, so a
        // low sound's single cycles don't ripple the line.
        const auto hold = juce::jmax (1, (int) std::ceil (0.02 / seconds * columns));
        auto held = levels;

        for (int col = 0; col < columns; ++col)
            for (int k = 1; k < hold && col + k < columns; ++k)
                held[(size_t) col] = juce::jmax (held[(size_t) col], levels[(size_t) (col + k)]);

        levels = std::move (held);
        repaint();
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

        if (lows.empty() || peak <= 0.0f)
            return;

        auto plot = area.reduced (6.0f, 8.0f);
        plot.removeFromBottom (14.0f);   // the time scale
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
        g.strokePath (wave, juce::PathStrokeType (0.6f));

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

    std::vector<float> lows, highs, levels;
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

    void show (Sound s, juce::Colour c)
    {
        sound = s;
        colour = c;
        reload();
    }

    void reload()
    {
        rows = drums.getLayers (sound);
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

        drums.setLayers (sound, std::move (next));
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
        g.drawText (juce::String ((int) rows.size()) + " of " + juce::String (DrumMachine::maxLayers), header.removeFromLeft (60), juce::Justification::centredLeft);

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

        drums.setLayers (sound, std::move (next));
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
        drums.setLayers (sound, std::move (next));
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
    Sound sound = Sound::kick;
    juce::Colour colour;
    std::vector<DrumMachine::Layer> rows;
    std::vector<std::vector<std::pair<float, float>>> shapes;
    juce::TextButton add { "Add..." }, remove { "-" }, up { juce::String::fromUTF8 ("\xe2\x96\xb2") }, down { juce::String::fromUTF8 ("\xe2\x96\xbc") }, spread { "Spread" };
    int selectedRow = -1, scroll = 0, dragging = 0, dragFrom = 0;
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

    for (auto s : padOrder())
    {
        pads.push_back (std::make_unique<Pad> (s, [this] (Sound sound, float proportionDown) { clickPad (sound, proportionDown); }));
        padPanel.addAndMakeVisible (*pads.back());
    }

    picture = std::make_unique<Picture>();
    soundPanel.addAndMakeVisible (*picture);
    layers = std::make_unique<Layers> (drums, [this] { chooseSamples(); });
    soundPanel.addChildComponent (*layers);

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

    type.setTooltip ("The sound's type: Classic, as it has always been; Modern, a second model with more to it, duller the softer it is struck; or Sample, which plays the sounds given to the pad");
    type.onChange = [this] (int)
    {
        if ((drums.getType (selected) == DrumMachine::sampleType) != shownSample)
            showSelected();
        else
            refreshPicture (true);
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

float DrumMachineEditor::velocityAt (float proportionDown) noexcept
{
    return juce::jlimit (0.3f, 1.0f, 1.0f - 0.7f * juce::jlimit (0.0f, 1.0f, (proportionDown - 0.1f) / 0.85f));
}

void DrumMachineEditor::clickPad (Sound s, float proportionDown)
{
    drums.audition (DrumMachine::padNote (s), velocityAt (proportionDown));
    select (s);
}

juce::Rectangle<int> DrumMachineEditor::padBounds (Sound s) const
{
    const auto& order = padOrder();

    for (size_t i = 0; i < order.size(); ++i)
        if (order[i] == s)
            return pads[i]->getBounds() + padPanel.getPosition();

    return {};
}

void DrumMachineEditor::select (Sound s)
{
    selected = s;
    const auto& order = padOrder();

    for (size_t i = 0; i < order.size(); ++i)
        pads[i]->setSelected (order[i] == s);

    showSelected();
}

void DrumMachineEditor::showSelected()
{
    const auto colour = colourOf (selected);
    soundPanel.setTitle (DrumMachine::nameOf (selected));
    soundPanel.setAccent (colour);

    tune.attach (drums.parameterOf (selected, "tune"));
    decay.attach (drums.parameterOf (selected, "decay"));
    tone.attach (drums.parameterOf (selected, "tone"));
    level.attach (drums.parameterOf (selected, "level"));
    pan.attach (drums.parameterOf (selected, "pan"));
    drive.attach (drums.parameterOf (selected, "drive"));
    start.attach (drums.parameterOf (selected, "start"));
    bend.attach (drums.parameterOf (selected, "bend"));
    reverse.attach (drums.reverseParameter (selected));
    reverse.setColour (colour);
    cutoff.attach (drums.parameterOf (selected, "cutoff"));
    resonance.attach (drums.parameterOf (selected, "resonance"));
    crush.attach (drums.parameterOf (selected, "crush"));
    reverb.attach (drums.parameterOf (selected, "reverb"));
    delay.attach (drums.parameterOf (selected, "delay"));
    filter.attach (drums.filterParameter (selected));
    filter.setColour (colour);

    for (auto* k : soundKnobs())
        k->setColour (colour);

    refreshFilterKnobs();

    character.setVisible (selected == Sound::kick);

    // Its type, and for a Sample its layers and its own three controls.
    type.setChoices (DrumMachine::typeNamesFor (selected));
    type.attach (drums.typeParameter (selected));
    type.setColour (colour);
    shownSample = drums.getType (selected) == DrumMachine::sampleType;
    samplesSeen = drums.getSamplesVersion();
    layers->show (selected, colour);
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
    const auto& was = pictured.sounds[(size_t) selected];

    if (! force && hasPicture && was == now.sounds[(size_t) selected] && (selected != Sound::kick || pictured.kick == now.kick))
        return;

    pictured = now;
    hasPicture = true;
    const auto guess = juce::jlimit (0.05, 6.5, now.sounds[(size_t) selected].decay * 0.00125 + 0.06);
    const auto hit = DrumMachine::Engine::renderOne (now, selected, pictureRate, guess, -1, drums.getSamples());
    picture->set (hit, pictureRate, now.sounds[(size_t) selected].decay, colourOf (selected));
    pictureSeconds = picture->getSeconds();
}

juce::String DrumMachineEditor::getKitText() const
{
    return drums.getKitName() + (drums.kitModified() ? " (edited)" : "");
}

juce::String DrumMachineEditor::getTypeText() const
{
    return DrumMachine::typeNamesFor (selected)[type.getIndex()];
}

int DrumMachineEditor::getNumLayersShown() const            { return layers->getNumRows(); }
juce::String DrumMachineEditor::getLayerText (int row) const { return layers->rowText (row); }
bool DrumMachineEditor::showsLayers() const                 { return layers->isVisible(); }
void DrumMachineEditor::selectLayer (int row)               { layers->select (row); }
void DrumMachineEditor::pressLayerButton (const juce::String& which)    { layers->press (which); }

void DrumMachineEditor::say (const juce::String& text)
{
    message = text;
    messageUntil = juce::Time::getMillisecondCounterHiRes() + 9000.0;
    repaint();
}

void DrumMachineEditor::markSampledPads()
{
    const auto& order = padOrder();

    for (size_t i = 0; i < order.size(); ++i)
        pads[i]->setSampled (drums.getType (order[i]) == DrumMachine::sampleType);
}

//==============================================================================
std::optional<Sound> DrumMachineEditor::soundAt (juce::Point<int> p) const
{
    const auto& order = padOrder();

    for (size_t i = 0; i < order.size(); ++i)
        if ((pads[i]->getBounds() + padPanel.getPosition()).contains (p))
            return order[i];

    if (soundPanel.getBounds().contains (p))
        return selected;

    return {};
}

void DrumMachineEditor::setDropTarget (std::optional<Sound> s)
{
    if (s == dropTarget)
        return;

    dropTarget = s;
    const auto& order = padOrder();

    for (size_t i = 0; i < order.size(); ++i)
        pads[i]->setDropping (s.has_value() && order[i] == *s);

    repaint();
}

bool DrumMachineEditor::addFiles (Sound s, const juce::Array<juce::File>& sounds)
{
    const auto before = drums.getLayers (s).size();
    const auto error = drums.addFiles (s, sounds);
    select (s);

    if (error.isNotEmpty())
        say (error);

    return drums.getLayers (s).size() > before;
}

bool DrumMachineEditor::isInterestedInFileDrag (const juce::StringArray& dragged)
{
    for (auto& f : dragged)
        if (juce::File (f).hasFileExtension (soundFileTypes))
            return true;

    return false;
}

void DrumMachineEditor::fileDragMove (const juce::StringArray&, int x, int y)      { setDropTarget (soundAt ({ x, y })); }
void DrumMachineEditor::fileDragExit (const juce::StringArray&)                    { setDropTarget ({}); }

void DrumMachineEditor::filesDropped (const juce::StringArray& dropped, int x, int y)
{
    const auto to = soundAt ({ x, y });
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
void DrumMachineEditor::itemDragMove (const SourceDetails& details)                { setDropTarget (soundAt (details.localPosition)); }
void DrumMachineEditor::itemDragExit (const SourceDetails&)                        { setDropTarget ({}); }

void DrumMachineEditor::itemDropped (const SourceDetails& details)
{
    const auto to = soundAt (details.localPosition);
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
    chooser = std::make_unique<juce::FileChooser> ("Choose sounds for " + DrumMachine::nameOf (selected), juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3;*.m4a;*.caf");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems,
                          [safe = juce::Component::SafePointer<DrumMachineEditor> (this), to = selected] (const juce::FileChooser& c)
                          {
                              if (safe != nullptr && ! c.getResults().isEmpty())
                                  safe->addFiles (to, c.getResults());
                          });
}

void DrumMachineEditor::showFilesMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "Open Kit File...");
    menu.addItem (2, "Save Kit with Samples...");
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
    if (const auto was = type.getIndex(); (type.refresh(), type.getIndex() != was)
            || (drums.getType (selected) == DrumMachine::sampleType) != shownSample || drums.getSamplesVersion() != samplesSeen)
        showSelected();

    markSampledPads();

    if (message.isNotEmpty() && juce::Time::getMillisecondCounterHiRes() > messageUntil)
    {
        message.clear();
        repaint();
    }

    if (const auto was = filter.getIndex(); (filter.refresh(), filter.getIndex() != was))
        refreshFilterKnobs();

    // The pads light at each new hit; the picture sweeps when its own sound plays.
    const auto& engine = drums.getEngine();
    const auto& order = padOrder();

    for (size_t i = 0; i < order.size(); ++i)
    {
        const auto count = engine.hitsOf (order[i]);

        if (count != seenHits[i])
        {
            seenHits[i] = count;
            pads[i]->hit (engine.lastVelocityOf (order[i]));

            if (order[i] == selected)
                picture->sweep();
        }
        else
        {
            pads[i]->fade();
        }
    }

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
        constexpr int columns = 6, rows = 5;
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
            readouts[i].setBounds (cell.removeFromBottom (20).withSizeKeepingCentre (76, 18));
            knobs[i]->setBounds (cell.reduced (inset, 0));
        };

        // Below the sound's own six, its effects: the filter's type, then five small knobs.
        auto fx = c.removeFromBottom (112);
        c.removeFromBottom (4);
        fxLine = fx.removeFromTop (14).translated (soundPanel.getX(), soundPanel.getY());
        filter.setBounds (fx.removeFromTop (28).withSizeKeepingCentre (juce::jmin (200, fx.getWidth()), 26));
        const auto fw = fx.getWidth() / 5;

        for (size_t i = 6; i < 11; ++i)
            place (i, (i < 10 ? fx.removeFromLeft (fw) : fx), 4);

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
        g.drawText ("Notes " + notesOf (selected) + "    " + chokesOf (selected), line, juce::Justification::centredLeft);
    }

    // A drag over the selected sound's panel: its edge lit, as a pad's is.
    if (dropTarget.has_value() && soundPanel.getBounds().contains (getMouseXYRelative()))
    {
        g.setColour (Theme::accent);
        g.drawRoundedRectangle (soundPanel.getBounds().toFloat().reduced (1.0f), 6.0f, 2.0f);
    }

    // A rule and a heading over its effects.
    g.setColour (Theme::dimText.withAlpha (0.35f));
    g.drawHorizontalLine (fxLine.getCentreY(), (float) fxLine.getX() + 64.0f, (float) fxLine.getRight());
    g.setColour (Theme::dimText);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("EFFECTS", fxLine.withWidth (60), juce::Justification::centredLeft);
}
