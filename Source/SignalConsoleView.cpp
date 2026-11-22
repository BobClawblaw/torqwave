#include "SignalConsoleView.h"
#include "Theme.h"

using namespace SignalConsole;

namespace
{
    constexpr float tau = juce::MathConstants<float>::twoPi;
    constexpr int hearMs = 600, jumpMs = 220;

    juce::Font mono (float size, bool bold = false)
    {
        return juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, bold ? juce::Font::bold : juce::Font::plain);
    }

    void text (juce::Graphics& g, const juce::String& s, float x, float y, float size, juce::Colour colour,
               juce::Justification where = juce::Justification::centredLeft, bool bold = false)
    {
        g.setColour (colour);
        g.setFont (mono (size, bold));
        const auto w = 400.0f, h = size + 6.0f;
        const auto left = where.testFlags (juce::Justification::left) ? x : where.testFlags (juce::Justification::right) ? x - w : x - w * 0.5f;
        g.drawText (s, juce::Rectangle<float> (left, y - h * 0.5f, w, h), where, false);
    }

    /** A sound's signature: a spoke for each band of its spectrum, longer where it is stronger, round a ring,
        the same on its left as on its right. */
    void drawSignature (juce::Graphics& g, juce::Point<float> c, float r, const Signature& s, juce::Colour colour, float alpha, bool glow)
    {
        juce::Path spokes;

        // The spectrum runs down both sides from the top: low at twelve o'clock, high at six.
        for (int b = 0; b < Signature::bands * 2; ++b)
        {
            const auto a = ((float) b + 0.5f) / (float) (Signature::bands * 2) * tau - tau * 0.25f;
            const auto band = b < Signature::bands ? b : Signature::bands * 2 - 1 - b;
            const auto reach = r * (0.5f + 0.5f * juce::jmax (0.06f, s.band[(size_t) band]));
            spokes.startNewSubPath (c.x + std::cos (a) * r * 0.5f, c.y + std::sin (a) * r * 0.5f);
            spokes.lineTo (c.x + std::cos (a) * reach, c.y + std::sin (a) * reach);
        }

        const auto width = r > 30.0f ? 2.0f : 1.3f;

        if (glow)
        {
            g.setColour (colour.withAlpha (alpha * 0.22f));
            g.strokePath (spokes, juce::PathStrokeType (width * 3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        g.setColour (colour.withAlpha (alpha));
        g.strokePath (spokes, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.drawEllipse (c.x - r * 0.44f, c.y - r * 0.44f, r * 0.88f, r * 0.88f, 1.0f);
    }

    /** A chip: a label in a rounded box, tinted when it is on. */
    void drawChip (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& label, juce::Colour colour, bool filled, juce::Colour labelColour)
    {
        if (filled)
        {
            g.setColour (colour.withAlpha (0.16f));
            g.fillRoundedRectangle (r, 4.0f);
        }

        g.setColour (colour);
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
        g.setColour (labelColour);
        g.setFont (mono (12.0f, true));
        g.drawText (label, r, juce::Justification::centred, false);
    }

    const juce::StringArray& sectorLabels()
    {
        static const juce::StringArray n { "OSCILLATORS", "FILTER", "ENVELOPES", "MODULATION", "EFFECTS" };
        return n;
    }

    const juce::StringArray& chipLabels()
    {
        static const juce::StringArray n { "OSC", "FILTER", "ENV", "MOD", "FX" };
        return n;
    }

    float sectorMiddle (int part)   { return (float) part / (float) numParts * tau - tau * 0.25f; }
}

//==============================================================================
SignalConsoleView::SignalConsoleView (Synth& s) : console (s.getConsole())
{
    motion = ! reduceMotion();
    console.onChange = [this]
    {
        layoutNodes();
        repaint();
    };
}

SignalConsoleView::~SignalConsoleView()
{
    console.onChange = nullptr;
}

void SignalConsoleView::visibilityChanged()
{
    if (isShowing())
    {
        // The patch as it is now is the node held; the twelve round it are made if they aren't there.
        if (! console.sync() && console.getNodes().empty() && ! console.isScanning())
            console.refresh();

        layoutNodes();
        startTimerHz (motion ? 30 : 4);
    }
    else
        stopTimer();
}

void SignalConsoleView::timerCallback()
{
    const auto now = juce::Time::getMillisecondCounter();

    if (now - lastSync > 500)
    {
        lastSync = now;
        console.sync();
    }

    const auto animating = (heard > -2 && now - heardAt < (juce::uint32) hearMs) || now - jumpedAt < (juce::uint32) jumpMs;
    moving = motion;

    if (motion)
    {
        sweep = std::fmod (sweep + 0.6f / 30.0f, tau);
        repaint();
    }
    else if (animating || console.isScanning())
        repaint();
}

//==============================================================================
void SignalConsoleView::resized()
{
    const auto b = getLocalBounds().toFloat();
    panel = b.withTrimmedLeft (b.getWidth() - 320.0f).reduced (4.0f, 8.0f);
    const auto scope = b.withTrimmedRight (328.0f);
    radius = juce::jmin ((scope.getWidth() - 110.0f) * 0.5f, (scope.getHeight() - 54.0f - 64.0f) * 0.5f);
    centre = { scope.getCentreX(), 34.0f + radius };

    // Under the scope: the firewalls, Reach and Rescan.
    const auto y = b.getBottom() - 30.0f;
    auto x = 24.0f;
    for (int part = 0; part < numParts; ++part)
    {
        const auto w = juce::GlyphArrangement::getStringWidth (mono (12.0f, true), chipLabels()[part]) + 24.0f;
        firewallChips[(size_t) part] = { x, y - 12.0f, w, 24.0f };
        x += w + 8.0f;
    }

    reachArea = { x + 28.0f, y - 20.0f, 150.0f, 40.0f };
    rescanChip = { reachArea.getRight() + 16.0f, y - 12.0f, 76.0f, 24.0f };
    layoutNodes();
}

void SignalConsoleView::layoutNodes()
{
    const auto& nodes = console.getNodes();
    places.assign (nodes.size(), {});
    const auto inner = radius * 0.31f, outer = radius * 0.95f;
    std::array<std::vector<int>, numParts> inPart;

    for (int i = 0; i < (int) nodes.size(); ++i)
        inPart[(size_t) juce::jlimit (0, (int) numParts - 1, nodes[(size_t) i].part)].push_back (i);

    std::vector<float> angle (nodes.size()), far (nodes.size());

    for (int part = 0; part < numParts; ++part)
    {
        const auto count = (int) inPart[(size_t) part].size();

        for (int k = 0; k < count; ++k)
        {
            const auto i = (size_t) inPart[(size_t) part][(size_t) k];
            angle[i] = sectorMiddle (part) + (((float) k + 0.5f) / (float) count - 0.5f) * 0.8f * tau / (float) numParts;
            far[i] = inner + (outer - inner) * juce::jlimit (0.0f, 1.0f, nodes[i].distance);
        }
    }

    // Nodes that would overlap: the further of the two moves out a little, a few times over.
    const auto apart = radius * 0.16f;

    for (int pass = 0; pass < 12; ++pass)
        for (size_t a = 0; a < nodes.size(); ++a)
            for (size_t b = a + 1; b < nodes.size(); ++b)
            {
                const juce::Point<float> pa { std::cos (angle[a]) * far[a], std::sin (angle[a]) * far[a] }, pb { std::cos (angle[b]) * far[b], std::sin (angle[b]) * far[b] };

                if (pa.getDistanceFrom (pb) < apart)
                {
                    auto& moved = nodes[a].distance >= nodes[b].distance ? far[a] : far[b];
                    moved = juce::jmin (outer, moved + apart * 0.25f);
                }
            }

    for (size_t i = 0; i < nodes.size(); ++i)
        places[i] = { centre.x + std::cos (angle[i]) * far[i], centre.y + std::sin (angle[i]) * far[i] };
}

juce::Point<float> SignalConsoleView::nodeCentre (int node) const
{
    return juce::isPositiveAndBelow (node, (int) places.size()) ? places[(size_t) node] : centre;
}

juce::Rectangle<float> SignalConsoleView::routeBounds (int entry) const
{
    for (size_t i = 0; i < routeRows.size(); ++i)
        if (routeEntries[i] == entry)
            return routeRows[i];

    return {};
}

int SignalConsoleView::nodeAt (juce::Point<float> p) const
{
    const auto reach = radius * 0.1f;

    for (int i = 0; i < (int) places.size(); ++i)
        if (p.getDistanceFrom (places[(size_t) i]) < reach)
            return i;

    return p.getDistanceFrom (centre) < radius * 0.2f ? -1 : -2;
}

//==============================================================================
void SignalConsoleView::paint (juce::Graphics& g)
{
    g.fillAll (Theme::base.darker (0.25f));

    // A grid of dots behind.
    g.setColour (Theme::overlay.withAlpha (0.06f));

    for (float x = 14.0f; x < (float) getWidth(); x += 28.0f)
        for (float y = 14.0f; y < (float) getHeight(); y += 28.0f)
            g.fillRect (x, y, 1.5f, 1.5f);

    drawScope (g);
    drawControls (g);
    drawPanel (g);
}

void SignalConsoleView::drawScope (juce::Graphics& g)
{
    const auto& nodes = console.getNodes();
    const auto& settings = console.getSettings();
    const auto k = radius / 236.0f;
    const auto accent = Theme::accent, wall = Theme::danger;
    const auto now = juce::Time::getMillisecondCounter();
    const auto inner = radius * 0.31f, outer = radius * 0.95f;

    // The rings: near to far.
    for (auto f : { 0.3f, 0.53f, 0.77f, 1.0f })
    {
        g.setColour (juce::exactlyEqual (f, 1.0f) ? Theme::lineStrong : Theme::line);
        g.drawEllipse (centre.x - radius * f, centre.y - radius * f, radius * f * 2.0f, radius * f * 2.0f, 1.0f);
    }

    text (g, "near", centre.x + 6.0f, centre.y - radius * 0.3f - 9.0f, 11.0f, Theme::dimText);
    text (g, "far", centre.x + 6.0f, centre.y - radius * 0.77f - 9.0f, 11.0f, Theme::dimText);

    // Reach: a ring that widens round the node held.
    {
        const auto r = inner + (outer - inner) * settings.reach;
        juce::Path ring;
        ring.addEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);
        const float dashes[] = { 3.0f, 6.0f };
        juce::Path dashed;
        juce::PathStrokeType (1.2f).createDashedStroke (dashed, ring, dashes, 2);
        g.setColour (accent.withAlpha (0.45f));
        g.fillPath (dashed);
    }

    // The sectors, their names, and the firewalls.
    for (int part = 0; part < numParts; ++part)
    {
        const auto from = sectorMiddle (part) - tau / (2.0f * (float) numParts), to = from + tau / (float) numParts;
        g.setColour (Theme::line);
        g.drawLine (centre.x + std::cos (from) * radius * 0.25f, centre.y + std::sin (from) * radius * 0.25f,
                    centre.x + std::cos (from) * radius, centre.y + std::sin (from) * radius, 1.0f);
        const auto held = settings.firewall[(size_t) part];
        const auto middle = sectorMiddle (part);
        text (g, sectorLabels()[part], centre.x + std::cos (middle) * (radius + 24.0f), centre.y + std::sin (middle) * (radius + 17.0f), 11.5f,
              held ? wall : Theme::dimText, juce::Justification::centred, true);

        if (held)
        {
            // juce's arcs measure from twelve o'clock, clockwise: a quarter turn on from these angles.
            juce::Path wedge;
            wedge.addPieSegment (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, from + tau * 0.25f, to + tau * 0.25f, 0.0f);
            juce::Graphics::ScopedSaveState state (g);
            g.reduceClipRegion (wedge);
            g.setColour (wall.withAlpha (0.22f));

            for (float x = centre.x - radius * 2.0f; x < centre.x + radius; x += 12.0f)
                g.drawLine (x, centre.y - radius, x + radius * 2.0f, centre.y + radius, 1.0f);

            juce::Path barrier;
            barrier.addCentredArc (centre.x, centre.y, radius * 0.27f, radius * 0.27f, 0.0f, from + tau * 0.25f, to + tau * 0.25f, true);
            g.setColour (wall);
            g.strokePath (barrier, juce::PathStrokeType (2.0f));
        }
    }

    // The sweep.
    {
        const auto at = motion ? sweep : -tau * 0.25f;
        juce::Path wedge;
        wedge.addPieSegment (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, at - 0.5f + tau * 0.25f, at + tau * 0.25f, 0.0f);
        g.setColour (accent.withAlpha (0.07f));
        g.fillPath (wedge);
        g.setColour (accent.withAlpha (0.6f));
        g.drawLine (centre.x, centre.y, centre.x + std::cos (at) * radius, centre.y + std::sin (at) * radius, 1.0f);
    }

    // The links out to the twelve, then the twelve.
    const auto shown = shownNode();
    const auto nodeRadius = 15.0f * k;

    for (int i = 0; i < (int) nodes.size() && i < (int) places.size(); ++i)
    {
        const auto p = places[(size_t) i];
        const auto toward = (p - centre) / juce::jmax (1.0f, p.getDistanceFrom (centre));
        g.setColour (accent.withAlpha (i == shown ? 0.5f : 0.14f));
        g.drawLine (juce::Line<float> (centre + toward * 46.0f * k, p - toward * (nodeRadius + 4.0f)), 1.0f);
    }

    for (int i = 0; i < (int) nodes.size() && i < (int) places.size(); ++i)
    {
        const auto p = places[(size_t) i];
        const auto on = i == shown;
        drawSignature (g, p, nodeRadius, nodes[(size_t) i].signature, on ? accent : accent.interpolatedWith (Theme::text, 0.55f), on ? 1.0f : 0.85f, on);

        if (on)
        {
            g.setColour (Theme::textStrong);
            g.drawEllipse (p.x - nodeRadius - 6.0f, p.y - nodeRadius - 6.0f, (nodeRadius + 6.0f) * 2.0f, (nodeRadius + 6.0f) * 2.0f, 1.5f);
        }

        // A scan line sweeps a node while it sounds.
        if (heard == i && now - heardAt < (juce::uint32) hearMs)
        {
            const auto t = (float) (now - heardAt) / (float) hearMs;
            juce::Path scan;
            scan.addCentredArc (p.x, p.y, nodeRadius + 10.0f, nodeRadius + 10.0f, 0.0f, t * tau * 1.5f, t * tau * 1.5f + 1.6f, true);
            g.setColour (accent);
            g.strokePath (scan, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        text (g, addressText (nodes[(size_t) i].address), p.x, p.y + nodeRadius + 12.0f, 10.5f, on ? Theme::textStrong : Theme::dimText, juce::Justification::centred, on);
    }

    // The node held, in the middle.
    drawSignature (g, centre, 40.0f * k, console.getHeld().signature, accent, 1.0f, true);

    if (heard == -1 && now - heardAt < (juce::uint32) hearMs)
    {
        const auto t = (float) (now - heardAt) / (float) hearMs;
        juce::Path scan;
        scan.addCentredArc (centre.x, centre.y, 48.0f * k, 48.0f * k, 0.0f, t * tau * 1.5f, t * tau * 1.5f + 1.6f, true);
        g.setColour (accent);
        g.strokePath (scan, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // A jump: a ring runs in along the link from where the node was.
    if (motion && now - jumpedAt < (juce::uint32) jumpMs)
    {
        const auto t = (float) (now - jumpedAt) / (float) jumpMs;
        const auto p = jumpedFrom + (centre - jumpedFrom) * (t * t * (3.0f - 2.0f * t));
        const auto r = nodeRadius + (40.0f * k - nodeRadius) * t;
        g.setColour (accent.withAlpha (1.0f - t * 0.6f));
        g.drawEllipse (p.x - r, p.y - r, r * 2.0f, r * 2.0f, 2.0f);
    }

    if (nodes.empty())
        text (g, console.isScanning() ? "SCANNING" : "", centre.x, centre.y + 62.0f * k, 11.0f, accent, juce::Justification::centred, true);
}

void SignalConsoleView::drawControls (juce::Graphics& g)
{
    const auto& settings = console.getSettings();

    for (int part = 0; part < numParts; ++part)
    {
        const auto held = settings.firewall[(size_t) part];
        drawChip (g, firewallChips[(size_t) part], chipLabels()[part], held ? Theme::danger : Theme::lineStrong, held, held ? Theme::danger : Theme::dimText);
    }

    text (g, "FIREWALLS", firewallChips[0].getX(), firewallChips[0].getY() - 12.0f, 10.0f, Theme::faintText, juce::Justification::centredLeft, true);

    // Reach: a small dial, its name and its value.
    const juce::Point<float> c { reachArea.getX() + 20.0f, reachArea.getCentreY() };
    const auto r = 15.0f, from = tau * 0.625f, to = from + tau * 0.75f * settings.reach;
    juce::Path track, value;
    track.addCentredArc (c.x, c.y, r, r, 0.0f, from, from + tau * 0.75f, true);
    value.addCentredArc (c.x, c.y, r, r, 0.0f, from, to, true);
    g.setColour (Theme::lineStrong);
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (Theme::accent);
    g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (Theme::textStrong);
    g.drawLine (c.x + std::sin (to) * r * 0.3f, c.y - std::cos (to) * r * 0.3f, c.x + std::sin (to) * r * 0.78f, c.y - std::cos (to) * r * 0.78f, 2.0f);
    text (g, "REACH", c.x + r + 10.0f, c.y - 8.0f, 11.0f, Theme::dimText);
    text (g, juce::String (settings.reach, 2), c.x + r + 10.0f, c.y + 8.0f, 13.0f, Theme::textStrong, juce::Justification::centredLeft, true);
    drawChip (g, rescanChip, "Rescan", Theme::lineStrong, false, Theme::dimText);
}

void SignalConsoleView::drawPanel (juce::Graphics& g)
{
    g.setColour (Theme::inset);
    g.fillRoundedRectangle (panel, 8.0f);
    g.setColour (Theme::line);
    g.drawRoundedRectangle (panel, 8.0f, 1.0f);

    const auto& nodes = console.getNodes();
    const auto& settings = console.getSettings();
    const auto shown = shownNode();
    const auto isNode = juce::isPositiveAndBelow (shown, (int) nodes.size());
    const auto& node = isNode ? nodes[(size_t) shown] : console.getHeld();
    const auto x = panel.getX() + 18.0f, right = panel.getRight() - 18.0f;
    auto y = panel.getY() + 24.0f;

    text (g, "NODE " + addressText (node.address), x, y, 12.0f, Theme::accent, juce::Justification::centredLeft, true);
    text (g, isNode ? juce::String (node.distance, 2) + " away" : juce::String ("held"), right, y, 12.0f, Theme::dimText, juce::Justification::centredRight);
    drawSignature (g, { x + 40.0f, y + 64.0f }, 34.0f, node.signature, Theme::accent, 1.0f, true);
    text (g, isNode ? partNames()[node.part] : juce::String ("The sound you hold"), x + 92.0f, y + 46.0f, 13.0f, Theme::textStrong, juce::Justification::centredLeft, true);
    text (g, isNode ? juce::String ("changed most") : console.getHeld().patch.name, x + 92.0f, y + 66.0f, 12.0f, Theme::dimText);

    juce::StringArray walls;

    for (int part = 0; part < numParts; ++part)
        if (settings.firewall[(size_t) part])
            walls.add (partNames()[part].toLowerCase());

    if (! walls.isEmpty())
        text (g, walls.joinIntoString (", ") + " held", x + 92.0f, y + 84.0f, 11.0f, Theme::danger);

    // What moved most, in its own units.
    y += 134.0f;

    for (int i = 0; i < 4; ++i)
    {
        if (isNode && i < (int) node.changes.size())
        {
            text (g, node.changes[(size_t) i].name, x, y, 12.0f, Theme::dimText);
            text (g, node.changes[(size_t) i].text, right, y, 12.0f, Theme::textStrong, juce::Justification::centredRight, true);
        }
        else if (! isNode && i == 0)
            text (g, "Click a node to hear it,", x, y, 12.0f, Theme::dimText);
        else if (! isNode && i == 1)
            text (g, "double-click to jump to it.", x, y, 12.0f, Theme::dimText);

        y += 26.0f;
    }

    hearChip = { x, y, 62.0f, 26.0f };
    jumpChip = { x + 72.0f, y, 62.0f, 26.0f };
    drawChip (g, hearChip, "Hear", Theme::lineStrong, false, Theme::text);
    drawChip (g, jumpChip, "Jump", isNode ? Theme::good : Theme::line, isNode, isNode ? Theme::good : Theme::faintText);

    // The route: where it has been, the newest last, as many as fit.
    y += 58.0f;
    text (g, "ROUTE", x, y, 11.0f, Theme::dimText, juce::Justification::centredLeft, true);
    y += 22.0f;
    const auto& route = console.getRoute();
    const auto rowHeight = 34.0f;
    const auto fit = juce::jmax (1, (int) ((panel.getBottom() - 12.0f - y) / rowHeight));
    const auto first = juce::jmax (0, (int) route.size() - fit);
    routeRows.clear();
    routeEntries.clear();

    for (int i = first; i < (int) route.size(); ++i)
    {
        const auto current = i == console.getRoutePosition();
        const juce::Rectangle<float> row { x - 6.0f, y - rowHeight * 0.5f + 4.0f, right - x + 12.0f, rowHeight - 2.0f };
        routeRows.push_back (row);
        routeEntries.push_back (i);

        if (current)
        {
            g.setColour (Theme::accent.withAlpha (0.12f));
            g.fillRoundedRectangle (row, 4.0f);
        }

        if (i > first)
        {
            g.setColour (Theme::lineStrong);
            g.drawLine (x + 12.0f, y - rowHeight + 18.0f, x + 12.0f, y - 10.0f, 1.0f);
        }

        drawSignature (g, { x + 12.0f, y + 4.0f }, 11.0f, route[(size_t) i].signature, current ? Theme::accent : Theme::dimText, current ? 1.0f : 0.75f, current);
        text (g, addressText (route[(size_t) i].address), x + 34.0f, y + 4.0f, 12.0f, current ? Theme::textStrong : Theme::dimText, juce::Justification::centredLeft, current);
        text (g, route[(size_t) i].patch.name, right, y + 4.0f, 11.0f, Theme::faintText, juce::Justification::centredRight);
        y += rowHeight;
    }
}

//==============================================================================
void SignalConsoleView::mouseMove (const juce::MouseEvent& e)
{
    const auto at = nodeAt (e.position);

    if (at != hovered)
    {
        hovered = at;
        repaint();
    }
}

void SignalConsoleView::mouseExit (const juce::MouseEvent&)
{
    if (hovered != -2)
    {
        hovered = -2;
        repaint();
    }
}

void SignalConsoleView::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position;
    const auto hearNode = [this] (int node)
    {
        console.hear (node);
        heard = node;
        heardAt = juce::Time::getMillisecondCounter();
    };

    if (reachArea.contains (p))
    {
        draggingReach = true;
        reachAtDragStart = console.getSettings().reach;
        return;
    }

    draggingReach = false;

    for (int part = 0; part < numParts; ++part)
        if (firewallChips[(size_t) part].contains (p))
        {
            console.setFirewall (part, ! console.getSettings().firewall[(size_t) part]);
            repaint();
            return;
        }

    if (rescanChip.contains (p))
    {
        console.rescan();
        repaint();
        return;
    }

    if (hearChip.contains (p))
    {
        hearNode (juce::jmax (-1, shownNode()));
        repaint();
        return;
    }

    if (jumpChip.contains (p))
    {
        if (const auto node = shownNode(); juce::isPositiveAndBelow (node, (int) console.getNodes().size()))
        {
            jumpedFrom = nodeCentre (node);
            jumpedAt = juce::Time::getMillisecondCounter();
            selected = -1;
            hovered = -2;
            console.jump (node);
        }

        return;
    }

    for (size_t i = 0; i < routeRows.size(); ++i)
        if (routeRows[i].contains (p))
        {
            selected = -1;
            console.goTo (routeEntries[i]);
            repaint();
            return;
        }

    if (const auto node = nodeAt (p); node > -2)
    {
        selected = node;
        hearNode (node);
        repaint();
    }
}

void SignalConsoleView::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingReach)
    {
        console.setReach (reachAtDragStart - (float) e.getDistanceFromDragStartY() / 160.0f);
        repaint();
    }
}

void SignalConsoleView::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (reachArea.contains (e.position))
    {
        console.setReach (0.35f);
        repaint();
        return;
    }

    if (const auto node = nodeAt (e.position); juce::isPositiveAndBelow (node, (int) console.getNodes().size()))
    {
        jumpedFrom = nodeCentre (node);
        jumpedAt = juce::Time::getMillisecondCounter();
        selected = -1;
        hovered = -2;
        console.jump (node);
    }
}

juce::String SignalConsoleView::getTooltip()
{
    const auto p = getMouseXYRelative().toFloat();

    for (int part = 0; part < numParts; ++part)
        if (firewallChips[(size_t) part].contains (p))
            return "Firewall: keep the " + partNames()[part].toLowerCase() + " as they are in every nearby sound";

    if (reachArea.contains (p))     return "Reach: how far the nearby sounds are from this one. Drag up or down; double-click for 0.35";
    if (rescanChip.contains (p))    return "Rescan: another twelve sounds round this one";
    if (hearChip.contains (p))      return "Hear this sound, at the last note you played";
    if (jumpChip.contains (p))      return "Jump: make this sound the synth's patch, and map the sounds round it";

    for (auto& row : routeRows)
        if (row.contains (p))
            return "Go back to this sound: its settings exactly";

    const auto node = nodeAt (p);

    if (node >= 0)                  return "Click to hear this sound; double-click to jump to it";
    if (node == -1)                 return "The sound you hold now: the synth's patch. Click to hear it";
    return {};
}
