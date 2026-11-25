#include "DrumPadEffectsView.h"
#include "Theme.h"

namespace
{
    using DrumMachine::Fx;
    using DrumMachine::PadEffects;

    constexpr int tabsHeight = 22, headingWidth = 56;
    constexpr double pictureRate = 32000.0, binSeconds = 0.0005;
    constexpr float eqTopDb = 20.0f;
    constexpr double eqLowHz = 20.0, eqHighHz = 20000.0;

    juce::String signedText (float v, int places, const juce::String& unit)
    {
        return (v > 0.0f ? "+" : "") + juce::String (v, places) + unit;
    }

    juce::String textOf (Fx f, float v)
    {
        if (f == Fx::envCurve)
            return signedText (std::round (v), 0, {});

        if (f == Fx::transAttack || f == Fx::transSustain)
            return signedText (std::round (v), 0, " %");

        if (f == Fx::eqLowGain || f == Fx::eqBell1Gain || f == Fx::eqBell2Gain || f == Fx::eqHighGain || f == Fx::compGain)
            return signedText (v, 1, " dB");

        if (f == Fx::compThreshold)
            return juce::String (v, 1) + " dB";

        if (f == Fx::compRatio)
            return juce::String (v, 1) + ":1";

        if (f == Fx::eqBell1Width || f == Fx::eqBell2Width)
            return juce::String (v, 1) + " oct";

        return SoundControls::formatValue (v, DrumMachine::fxInfo (f).unit);
    }

    /** The EQ's bands: each one's frequency, gain and (a bell's) width. */
    struct Band { Fx freq, gain; std::optional<Fx> width; const char* name; };

    const std::array<Band, 4>& bands()
    {
        static const std::array<Band, 4> all {{ { Fx::eqLowFreq, Fx::eqLowGain, {}, "1 Low" },
                                                { Fx::eqBell1Freq, Fx::eqBell1Gain, Fx::eqBell1Width, "2 Bell" },
                                                { Fx::eqBell2Freq, Fx::eqBell2Gain, Fx::eqBell2Width, "3 Bell" },
                                                { Fx::eqHighFreq, Fx::eqHighGain, {}, "4 High" } }};
        return all;
    }

    Fx switchOf (int page)
    {
        return DrumMachine::effectSwitches[(size_t) juce::jlimit (0, DrumMachine::numEffects - 1, page - 1)];
    }

    /** How many points a curve across a width gets: one for each pixel of the display. */
    int pointsAcross (float width, float scale)   { return juce::jmax (2, (int) std::ceil (width * juce::jmax (1.0f, scale))); }

    void strokeCurve (juce::Graphics& g, const juce::Path& curve, juce::Colour c, float thickness)
    {
        g.setColour (c);
        g.strokePath (curve, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    /** A hit's level in bins of half a millisecond: each the loudest of the 12 ms from it on, then
        evened out over the same 12 ms, so the line neither ripples with a low sound's cycles nor steps. */
    std::vector<float> levelOf (const juce::AudioBuffer<float>& hit)
    {
        const auto per = juce::jmax (1, (int) (binSeconds * pictureRate));
        const auto n = hit.getNumSamples() / per;
        std::vector<float> peaks ((size_t) n, 0.0f);

        for (int i = 0; i < n * per; ++i)
            peaks[(size_t) (i / per)] = juce::jmax (peaks[(size_t) (i / per)], 0.5f * std::abs (hit.getSample (0, i) + hit.getSample (1, i)));

        constexpr int hold = 24;
        std::vector<float> held ((size_t) n, 0.0f), even ((size_t) n, 0.0f);

        for (int i = 0; i < n; ++i)
            for (int k = 0; k < hold && i + k < n; ++k)
                held[(size_t) i] = juce::jmax (held[(size_t) i], peaks[(size_t) (i + k)]);

        for (int i = 0; i < n; ++i)
        {
            auto sum = 0.0f;
            auto count = 0;

            for (int k = -hold / 2; k <= hold / 2; ++k)
                if (juce::isPositiveAndBelow (i + k, n))
                {
                    sum += held[(size_t) (i + k)];
                    ++count;
                }

            even[(size_t) i] = sum / (float) juce::jmax (1, count);
        }

        return even;
    }
}

//==============================================================================
DrumPadEffectsView::DrumPadEffectsView (BuiltInEffects::BasicDrumsProcessor& d) : drums (d)
{
    addChildComponent (onSwitch);
    addChildComponent (selector);
    onSwitch.setTooltip ("Switches this effect on for this pad; off, the pad plays as if it were not there");
    onSwitch.onToggle = [this] (bool on)
    {
        if (page != filterPage)
        {
            drums.setEffect (pad, switchOf (page), on ? 1.0f : 0.0f);
            load();
        }
    };

    selector.onChange = [this] (int index)
    {
        if (page == satPage)
            drums.setEffect (pad, Fx::satType, (float) index);
        else if (page == eqPage)
            band = index;

        load();
        layoutPage();
    };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        addChildComponent (knobs[i]);
        addChildComponent (readouts[i]);
        knobs[i].onValueChanged = [this, i]
        {
            if (i < shown.size())
            {
                drums.setEffect (pad, shown[i].fx, knobs[i].link().getPlain());
                fx = drums.getEffects (pad);
                seen = drums.getEffectsVersion();
                readouts[i].setText (knobs[i].getValueText());
                transientStale = true;
                repaint();
            }
        };
    }
}

juce::StringArray DrumPadEffectsView::pageNames()   { return { "FILTER", "ENV", "TRANSIENT", "SATURATE", "EQ", "COMP" }; }

void DrumPadEffectsView::show (int p, juce::Colour c)
{
    pad = p;
    colour = c;
    onSwitch.setColour (c);
    selector.setColour (c);

    for (auto& k : knobs)
        k.setColour (c);

    meterDb = 0.0f;
    transientStale = true;
    load();
}

void DrumPadEffectsView::setPage (int p)
{
    p = juce::jlimit (0, (int) numPages - 1, p);

    if (p == page)
        return;

    page = p;
    transientStale = true;
    load();
    layoutPage();

    if (onPageChanged != nullptr)
        onPageChanged();
}

void DrumPadEffectsView::setFilterLit (bool lit)
{
    if (lit != filterLit)
    {
        filterLit = lit;
        repaint();
    }
}

bool DrumPadEffectsView::isLit (int p) const
{
    return p == filterPage ? filterLit : fx.on (switchOf (p));
}

std::vector<DrumPadEffectsView::KnobSpec> DrumPadEffectsView::specs() const
{
    switch (page)
    {
        case envPage:
            return { { Fx::envAttack, "Attack", "How long the hit takes to come up to its level", false },
                     { Fx::envHold, "Hold", "How long it stays there", false },
                     { Fx::envDecay, "Decay", "How long it then takes to fall to nothing; the hit ends there, or sooner if the sound's own Decay ends it", false },
                     { Fx::envCurve, "Curve", "The shape of the fall: 0 a straight line; above, it drops fast and lingers; below, it stays up and drops at the end", true } };
        case transPage:
            return { { Fx::transAttack, "Attack", "The start of the hit: louder (snappier) or softer, by up to 12 dB", true },
                     { Fx::transSustain, "Sustain", "What follows the start: its body and tail brought up or down, by up to 12 dB", true } };
        case satPage:
            return { { Fx::satAmount, "Amount", "How hard the curve is driven", false },
                     { Fx::satMix, "Mix", "How much of the saturated sound against the sound as it came in", false } };
        case eqPage:
        {
            const auto& b = bands()[(size_t) band];
            std::vector<KnobSpec> all { { b.freq, "Freq", band == 0 ? "Where the low shelf turns" : band == 3 ? "Where the high shelf turns" : "The bell's centre", false },
                                        { b.gain, "Gain", "How far this band lifts or cuts; at 0 dB it is not there", true } };

            if (b.width.has_value())
                all.push_back ({ *b.width, "Width", "How wide the bell is, in octaves between the points of half its gain", false });

            return all;
        }
        case compPage:
            return { { Fx::compThreshold, "Thresh", "The level above which the hit is turned down", false },
                     { Fx::compRatio, "Ratio", "How hard: at 4:1, 4 dB over the threshold comes out as 1", false },
                     { Fx::compAttack, "Attack", "How long it takes to turn down: longer lets more of the hit's start through", false },
                     { Fx::compRelease, "Release", "How long it takes to let go", false },
                     { Fx::compGain, "Gain", "Level added after it, to make up what it took", false } };
        default:
            break;
    }

    return {};
}

void DrumPadEffectsView::load()
{
    fx = drums.getEffects (pad);
    seen = drums.getEffectsVersion();
    shown = specs();
    const auto own = page != filterPage;
    onSwitch.setVisible (own);

    if (own)
        onSwitch.setOn (fx.on (switchOf (page)), false);

    selector.setVisible (page == satPage || page == eqPage);

    if (page == satPage)
    {
        selector.setChoices (DrumMachine::saturationNames());
        selector.setIndex (juce::roundToInt (fx[Fx::satType]), false);
        selector.setTooltip ("The saturator's curve: Tape, soft and even; Tube, lopsided, with even harmonics; Clip, flat at full scale; Fold, the top folded back over");
    }
    else if (page == eqPage)
    {
        juce::StringArray names;

        for (auto& b : bands())
            names.add (b.name);

        selector.setChoices (names);
        selector.setIndex (band, false);
        selector.setTooltip ("The band the knobs set: the low shelf, the two bells, the high shelf (or click its point in the picture)");
    }

    const auto dim = own && ! fx.on (switchOf (page));

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const auto used = i < shown.size();
        knobs[i].setVisible (used);
        readouts[i].setVisible (used);

        if (! used)
            continue;

        const auto& info = DrumMachine::fxInfo (shown[i].fx);
        knobs[i].setLabel (shown[i].label);
        knobs[i].setTooltip (shown[i].tip);
        knobs[i].setBipolar (shown[i].bipolar);
        knobs[i].link().setStandalone ({ info.low, info.high, 0.0f, info.skew }, info.start, info.unit);
        knobs[i].link().set (juce::NormalisableRange<float> (info.low, info.high, 0.0f, info.skew).convertTo0to1 (fx[shown[i].fx]));
        knobs[i].setFormatter ([f = shown[i].fx] (float v) { return textOf (f, v); });
        knobs[i].refresh();
        knobs[i].setAlpha (dim ? 0.45f : 1.0f);
        readouts[i].setAlpha (dim ? 0.45f : 1.0f);
        readouts[i].setText (knobs[i].getValueText());
    }

    repaint();
}

void DrumPadEffectsView::refresh()
{
    if (drums.getEffectsVersion() != seen)
    {
        transientStale = true;
        load();
    }

    if (page == compPage)
    {
        if (const auto now = fx.on (Fx::compOn) ? drums.getEngine().padReductionDb (pad) : 0.0f; std::abs (now - meterDb) > 0.05f)
        {
            meterDb = now;
            repaint();
        }
    }

    if (page == transPage && transientStale)
        renderTransient();
}

void DrumPadEffectsView::soundChanged (double seconds)
{
    hitSeconds = juce::jlimit (0.05, 2.0, seconds);
    transientStale = true;
}

void DrumPadEffectsView::renderTransient()
{
    transientStale = false;
    auto kit = drums.currentSettings();
    auto& settings = pad < DrumMachine::numSounds ? kit.sounds[(size_t) pad] : kit.user[(size_t) (pad - DrumMachine::numSounds)].settings;
    after = levelOf (DrumMachine::Engine::renderPad (kit, pad, pictureRate, hitSeconds, drums.getSamples()));
    settings.effects[Fx::transOn] = 0.0f;
    before = levelOf (DrumMachine::Engine::renderPad (kit, pad, pictureRate, hitSeconds, drums.getSamples()));
    repaint();
}

//==============================================================================
juce::Rectangle<int> DrumPadEffectsView::tabBounds (int p) const
{
    // Each as wide as its name needs, with room for its dot.
    auto row = getLocalBounds().removeFromTop (tabsHeight).withTrimmedLeft (headingWidth);
    const auto names = pageNames();
    auto ahead = 0, all = 0;

    for (int i = 0; i < numPages; ++i)
    {
        const auto weight = names[i].length() + 5;
        ahead += i < p ? weight : 0;
        all += weight;
    }

    const auto unit = (float) row.getWidth() / (float) all;
    return juce::Rectangle<float> ((float) row.getX() + unit * (float) ahead, (float) row.getY(), unit * (float) (names[p].length() + 5), (float) row.getHeight())
               .reduced (1.5f, 1.0f).toNearestInt();
}

juce::Rectangle<int> DrumPadEffectsView::getPageBounds() const
{
    return getLocalBounds().withTrimmedTop (tabsHeight + 6);
}

juce::Rectangle<float> DrumPadEffectsView::pictureBounds() const
{
    const auto area = getPageBounds();
    const auto width = page == envPage ? 190 : page == transPage ? 220 : page == satPage ? 150 : page == eqPage ? 200 : 150;
    return area.withWidth (juce::jmin (width, area.getWidth() / 2)).toFloat();
}

juce::Rectangle<float> DrumPadEffectsView::plotBounds() const
{
    return pictureBounds().reduced (6.0f, 6.0f);
}

void DrumPadEffectsView::resized()   { layoutPage(); }

void DrumPadEffectsView::layoutPage()
{
    if (page == filterPage)
        return;

    const auto picture = pictureBounds().toNearestInt();
    onSwitch.setBounds (picture.getRight() - 50, picture.getY() + 4, 46, 18);
    auto rest = getPageBounds().withTrimmedLeft (picture.getWidth() + 8);

    if (selector.isVisible())
    {
        const auto w = page == eqPage ? 82 : 104;
        selector.setBounds (rest.removeFromLeft (w).withSizeKeepingCentre (w, 26).translated (0, -8));
        rest.removeFromLeft (4);
    }

    const auto count = (int) juce::jmax ((size_t) 1, shown.size());
    const auto w = juce::jmin (76, rest.getWidth() / count);
    rest = rest.withSizeKeepingCentre (w * count, rest.getHeight());

    for (size_t i = 0; i < shown.size(); ++i)
    {
        auto cell = rest.removeFromLeft (w);
        readouts[i].setBounds (cell.removeFromBottom (20).withSizeKeepingCentre (juce::jmin (72, w - 2), 18));
        knobs[i].setBounds (cell.reduced (2, 0));
    }
}

//==============================================================================
void DrumPadEffectsView::paint (juce::Graphics& g)
{
    // The heading, then the tabs: the one shown outlined in the pad's colour, a dot lit on each that is on.
    g.setColour (Theme::dimText);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("EFFECTS", getLocalBounds().removeFromTop (tabsHeight).withWidth (headingWidth), juce::Justification::centredLeft);
    const auto names = pageNames();

    for (int p = 0; p < numPages; ++p)
    {
        const auto r = tabBounds (p).toFloat();
        const auto current = p == page;
        g.setColour (current ? Theme::raised : Theme::inset);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (current ? colour : Theme::line);
        g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, current ? 1.4f : 1.0f);

        const auto dot = juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ r.getX() + 9.0f, r.getCentreY() });

        if (isLit (p))
        {
            g.setColour (colour.withAlpha (0.3f));
            g.fillEllipse (dot.expanded (2.0f));
            g.setColour (colour);
            g.fillEllipse (dot);
        }
        else
        {
            g.setColour (Theme::dimText.withAlpha (0.7f));
            g.drawEllipse (dot.reduced (0.5f), 1.0f);
        }

        g.setColour (current ? Theme::textStrong : Theme::dimText);
        g.setFont (juce::FontOptions (9.5f, current ? juce::Font::bold : juce::Font::plain));
        g.drawFittedText (names[p], r.withTrimmedLeft (16.0f).withTrimmedRight (4.0f).toNearestInt(), juce::Justification::centred, 1, 0.8f);
    }

    if (page == filterPage)
        return;

    const auto well = pictureBounds();
    const auto scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    SoundControls::drawDisplayWell (g, well, page == eqPage ? 6 : 4, page == eqPage ? 4 : 2);
    const auto plot = plotBounds();
    juce::Graphics::ScopedSaveState state (g);
    g.reduceClipRegion (well.toNearestInt());

    if (! fx.on (switchOf (page)))
        g.setOpacity (0.5f);

    switch (page)
    {
        case envPage:    paintEnvelope (g, plot, scale); break;
        case transPage:  paintTransient (g, plot, scale); break;
        case satPage:    paintSaturation (g, plot, scale); break;
        case eqPage:     paintEq (g, plot, scale); break;
        case compPage:   paintCompressor (g, plot, scale); break;
        default:         break;
    }
}

void DrumPadEffectsView::paintEnvelope (juce::Graphics& g, juce::Rectangle<float> plot, float scale) const
{
    // The whole envelope and a tenth more, so its end shows.
    const auto seconds = DrumMachine::envelopeSeconds (fx) * 1.1;
    const auto n = pointsAcross (plot.getWidth(), scale);
    juce::Path curve;

    for (int i = 0; i < n; ++i)
    {
        const auto share = (double) i / (double) (n - 1);
        const auto x = plot.getX() + plot.getWidth() * (float) share;
        const auto y = plot.getBottom() - plot.getHeight() * DrumMachine::envelopeGain (fx, share * seconds);

        if (i == 0)
            curve.startNewSubPath (x, y);
        else
            curve.lineTo (x, y);
    }

    SoundControls::drawGlowingCurve (g, curve, colour, plot);
}

void DrumPadEffectsView::paintTransient (juce::Graphics& g, juce::Rectangle<float> plot, float scale) const
{
    if (before.size() < 2 || after.size() < 2)
        return;

    // Both to the louder one's peak, over the part of the hit that sounds.
    auto top = 1.0e-6f;
    size_t last = 1;

    for (auto* all : { &before, &after })
        for (size_t i = 0; i < all->size(); ++i)
        {
            top = juce::jmax (top, (*all)[i]);

            if ((*all)[i] > 1.0e-4f)
                last = juce::jmax (last, i);
        }

    const auto length = (float) juce::jmin (before.size() - 1, last + last / 20 + 2);
    const auto n = pointsAcross (plot.getWidth(), scale);
    const auto pathOf = [&] (const std::vector<float>& level)
    {
        juce::Path curve;

        for (int i = 0; i < n; ++i)
        {
            // Between two of its bins, a straight line.
            const auto at = length * (float) i / (float) (n - 1);
            const auto i0 = juce::jmin (level.size() - 1, (size_t) at), i1 = juce::jmin (level.size() - 1, i0 + 1);
            const auto v = level[i0] + (at - (float) i0) * (level[i1] - level[i0]);
            const auto x = plot.getX() + plot.getWidth() * (float) i / (float) (n - 1);
            const auto y = plot.getBottom() - plot.getHeight() * 0.94f * v / top;

            if (i == 0)
                curve.startNewSubPath (x, y);
            else
                curve.lineTo (x, y);
        }

        return curve;
    };

    strokeCurve (g, pathOf (before), Theme::dimText.withAlpha (0.8f), 1.2f);
    SoundControls::drawGlowingCurve (g, pathOf (after), colour, plot, false);
    g.setColour (Theme::dimText);
    g.setFont (juce::FontOptions (9.5f));
    g.drawText ("grey: before", plot.withTrimmedRight (52.0f).translated (0.0f, -3.0f), juce::Justification::topRight);
}

void DrumPadEffectsView::paintSaturation (juce::Graphics& g, juce::Rectangle<float> plot, float scale) const
{
    // Output against input, both from -1 to 1 across a square; the straight line is the sound unchanged.
    const auto side = juce::jmin (plot.getWidth(), plot.getHeight());
    const auto square = plot.withSizeKeepingCentre (side, side);
    g.setColour (Theme::dimText.withAlpha (0.5f));
    g.drawLine (square.getX(), square.getBottom(), square.getRight(), square.getY(), 1.0f);
    const auto n = pointsAcross (side, scale);
    juce::Path curve;

    for (int i = 0; i < n; ++i)
    {
        const auto in = -1.0f + 2.0f * (float) i / (float) (n - 1);
        const auto out = juce::jlimit (-1.0f, 1.0f, DrumMachine::saturated (fx, in));
        const auto x = square.getX() + side * (float) i / (float) (n - 1), y = square.getCentreY() - 0.5f * side * out;

        if (i == 0)
            curve.startNewSubPath (x, y);
        else
            curve.lineTo (x, y);
    }

    SoundControls::drawGlowingCurve (g, curve, colour, plot, false);
}

juce::Point<float> DrumPadEffectsView::pointOf (int b, juce::Rectangle<float> plot) const
{
    const auto& which = bands()[(size_t) b];
    const auto x = plot.getX() + plot.getWidth() * (float) (std::log (fx[which.freq] / eqLowHz) / std::log (eqHighHz / eqLowHz));
    return { x, plot.getCentreY() - plot.getHeight() * 0.5f * fx[which.gain] / eqTopDb };
}

void DrumPadEffectsView::paintEq (juce::Graphics& g, juce::Rectangle<float> plot, float scale) const
{
    g.setColour (Theme::dimText.withAlpha (0.5f));
    g.drawHorizontalLine (juce::roundToInt (plot.getCentreY()), plot.getX(), plot.getRight());
    const auto n = pointsAcross (plot.getWidth(), scale);
    juce::Path curve;

    for (int i = 0; i < n; ++i)
    {
        const auto share = (double) i / (double) (n - 1);
        const auto db = juce::jlimit (-eqTopDb, eqTopDb, DrumMachine::eqGainDb (fx, eqLowHz * std::pow (eqHighHz / eqLowHz, share), 48000.0));
        const auto x = plot.getX() + plot.getWidth() * (float) share, y = plot.getCentreY() - plot.getHeight() * 0.5f * db / eqTopDb;

        if (i == 0)
            curve.startNewSubPath (x, y);
        else
            curve.lineTo (x, y);
    }

    SoundControls::drawGlowingCurve (g, curve, colour, plot, false);

    // A point for each band, numbered; the one the knobs set is filled.
    g.setFont (juce::FontOptions (8.5f, juce::Font::bold));

    for (int b = 0; b < 4; ++b)
    {
        const auto dot = juce::Rectangle<float> (11.0f, 11.0f).withCentre (pointOf (b, plot));
        g.setColour (b == band ? colour : Theme::inset);
        g.fillEllipse (dot);
        g.setColour (colour);
        g.drawEllipse (dot, 1.2f);
        g.setColour (b == band ? Theme::inset : colour);
        g.drawText (juce::String (b + 1), dot, juce::Justification::centred);
    }

    g.setColour (Theme::dimText);
    g.setFont (juce::FontOptions (8.5f));

    for (auto [hz, text] : std::initializer_list<std::pair<double, const char*>> { { 100.0, "100" }, { 1000.0, "1k" }, { 10000.0, "10k" } })
    {
        const auto x = plot.getX() + plot.getWidth() * (float) (std::log (hz / eqLowHz) / std::log (eqHighHz / eqLowHz));
        g.drawText (text, juce::Rectangle<float> (x - 14.0f, plot.getBottom() - 7.0f, 28.0f, 10.0f), juce::Justification::centred);
    }
}

void DrumPadEffectsView::paintCompressor (juce::Graphics& g, juce::Rectangle<float> plot, float scale) const
{
    // On the left, output against input from -60 to 0 dB (the straight line: nothing done); on the
    // right, how far the pad's last hit was turned down.
    const auto side = juce::jmin (plot.getHeight(), plot.getWidth() * 0.56f);
    auto rest = plot;
    const auto square = rest.removeFromLeft (side).withSizeKeepingCentre (side, side);
    g.setColour (Theme::dimText.withAlpha (0.5f));
    g.drawLine (square.getX(), square.getBottom(), square.getRight(), square.getY(), 1.0f);
    const auto n = pointsAcross (side, scale);
    juce::Path curve;
    auto flat = fx;
    flat[Fx::compGain] = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const auto in = -60.0f + 60.0f * (float) i / (float) (n - 1);
        const auto out = juce::jlimit (-60.0f, 0.0f, DrumMachine::compressedDb (flat, in));
        const auto x = square.getX() + side * (float) i / (float) (n - 1), y = square.getY() - side * out / 60.0f;

        if (i == 0)
            curve.startNewSubPath (x, y);
        else
            curve.lineTo (x, y);
    }

    SoundControls::drawGlowingCurve (g, curve, colour, plot, false);

    rest.removeFromLeft (8.0f);
    rest.removeFromTop (22.0f);   // the switch is above it
    g.setColour (Theme::dimText);
    g.setFont (juce::FontOptions (9.5f));
    g.drawText ("GR", rest.removeFromTop (11.0f), juce::Justification::centredLeft);
    g.setColour (Theme::accentLight);
    g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::bold));
    g.drawText (getMeterText(), rest.removeFromTop (14.0f), juce::Justification::centredLeft);
    auto bar = rest.removeFromTop (9.0f).reduced (0.0f, 2.0f);
    g.setColour (Theme::raised);
    g.fillRoundedRectangle (bar, 2.5f);

    if (const auto share = juce::jlimit (0.0f, 1.0f, -meterDb / 24.0f); share > 0.0f)
    {
        g.setColour (colour);
        g.fillRoundedRectangle (bar.withWidth (juce::jmax (5.0f, bar.getWidth() * share)), 2.5f);
    }
}

juce::String DrumPadEffectsView::getMeterText() const
{
    return juce::String (juce::jmin (0.0f, meterDb), 1) + " dB";
}

//==============================================================================
void DrumPadEffectsView::setBand (int b, double hz, float db)
{
    const auto& which = bands()[(size_t) b];
    auto all = drums.getEffects (pad);
    all[which.freq] = (float) hz;
    all[which.gain] = db;
    drums.setEffects (pad, all);
    load();
}

void DrumPadEffectsView::dragPoint (int b, double hz, float db)
{
    band = juce::jlimit (0, 3, b);
    setBand (band, hz, db);
    layoutPage();
}

void DrumPadEffectsView::mouseDown (const juce::MouseEvent& e)
{
    for (int p = 0; p < numPages; ++p)
        if (tabBounds (p).contains (e.getPosition()))
        {
            setPage (p);
            return;
        }

    dragging = -1;

    if (page != eqPage)
        return;

    // The nearest point within reach; the one already chosen first, where two sit together.
    const auto plot = plotBounds();
    auto nearest = 14.0f;

    for (int k = 0; k < 4; ++k)
    {
        const auto b = (band + k) % 4;

        if (const auto distance = pointOf (b, plot).getDistanceFrom (e.position); distance < nearest)
        {
            nearest = distance;
            dragging = b;
        }
    }

    if (dragging >= 0 && dragging != band)
    {
        band = dragging;
        load();
        layoutPage();
    }
}

void DrumPadEffectsView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging < 0 || page != eqPage)
        return;

    const auto plot = plotBounds();
    const auto share = juce::jlimit (0.0f, 1.0f, (e.position.x - plot.getX()) / plot.getWidth());
    const auto db = juce::jlimit (-18.0f, 18.0f, (plot.getCentreY() - e.position.y) / (plot.getHeight() * 0.5f) * eqTopDb);
    setBand (dragging, eqLowHz * std::pow (eqHighHz / eqLowHz, (double) share), std::abs (db) < 0.4f ? 0.0f : db);
}

void DrumPadEffectsView::mouseUp (const juce::MouseEvent&)   { dragging = -1; }

void DrumPadEffectsView::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (page != eqPage)
        return;

    const auto plot = plotBounds();

    for (int b = 0; b < 4; ++b)
        if (pointOf (b, plot).getDistanceFrom (e.position) < 10.0f)
        {
            band = b;
            setBand (b, fx[bands()[(size_t) b].freq], 0.0f);
            layoutPage();
            return;
        }
}

void DrumPadEffectsView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const auto& which = bands()[(size_t) band];

    if (page != eqPage || ! which.width.has_value() || ! pictureBounds().contains (e.position))
    {
        Component::mouseWheelMove (e, wheel);
        return;
    }

    drums.setEffect (pad, *which.width, fx[*which.width] * std::exp2 (-wheel.deltaY * 2.0f));
    load();
}

//==============================================================================
void DrumPadEffectsView::clickSwitch()   { onSwitch.setOn (! onSwitch.isOn()); }

juce::StringArray DrumPadEffectsView::getKnobLabels() const
{
    juce::StringArray labels;

    for (auto& s : shown)
        labels.add (s.label);

    return labels;
}

juce::String DrumPadEffectsView::getKnobText (const juce::String& label) const
{
    for (size_t i = 0; i < shown.size(); ++i)
        if (shown[i].label.equalsIgnoreCase (label))
            return readouts[i].getText();

    return {};
}

bool DrumPadEffectsView::turnKnob (const juce::String& label, float value)
{
    for (size_t i = 0; i < shown.size(); ++i)
        if (shown[i].label.equalsIgnoreCase (label))
        {
            const auto& info = DrumMachine::fxInfo (shown[i].fx);
            knobs[i].setNormalised (juce::NormalisableRange<float> (info.low, info.high, 0.0f, info.skew).convertTo0to1 (juce::jlimit (info.low, info.high, value)));
            return true;
        }

    return false;
}
