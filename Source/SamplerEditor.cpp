#include "SamplerEditor.h"
#include "AudioFileIO.h"
#include "SoundDrops.h"
#include "Theme.h"

using BuiltInEffects::SamplerProcessor;
using SoundControls::Knob;

namespace
{
    const juce::NormalisableRange<float> rootRange { 0.0f, 127.0f, 1.0f }, tuneRange { -100.0f, 100.0f }, zoneLevelRange { -48.0f, 12.0f },
                                         zonePanRange { -100.0f, 100.0f }, crossfadeRange { 0.0f, 500.0f, 0.0f, 0.4f }, sensitivityRange { 0.0f, 100.0f };

    constexpr int blockFrames = 256;        // the waveform's peaks are kept for every so many frames
    constexpr int keysHeight = 20;          // the strip of keys under the zones
    constexpr int padColumns = 16;
    constexpr juce::int64 shortestPart = 16, shortestLoop = 4;

    void put (Knob& knob, const juce::NormalisableRange<float>& range, float plain)
    {
        knob.setNormalised (range.convertTo0to1 (range.snapToLegalValue (juce::jlimit (range.start, range.end, plain))));
    }

    bool isBlack (int key)
    {
        const auto in = key % 12;
        return in == 1 || in == 3 || in == 6 || in == 8 || in == 10;
    }

    juce::String lengthText (const Sampler::Sample& sample)
    {
        const auto seconds = (double) sample.getNumFrames() / sample.rate;
        return seconds < 10.0 ? juce::String (seconds, 2) + " s" : juce::String (seconds, 1) + " s";
    }
}

//==============================================================================
SamplerEditor::Wave::Wave (SamplerEditor& e) : editor (e)
{
    setWantsKeyboardFocus (false);
}

void SamplerEditor::Wave::setView (juce::Range<double> frames)
{
    const auto all = shown != nullptr ? (double) shown->getNumFrames() : 1.0;
    const auto length = juce::jlimit (juce::jmin (16.0, all), all, frames.getLength());
    const auto start = juce::jlimit (0.0, all - length, frames.getStart());
    view = { start, start + length };
    repaint();
}

void SamplerEditor::Wave::showAll()
{
    view = { 0.0, shown != nullptr ? (double) shown->getNumFrames() : 1.0 };
    repaint();
}

float SamplerEditor::Wave::xOf (juce::int64 frame) const
{
    return view.getLength() > 0.0 ? (float) (((double) frame - view.getStart()) / view.getLength() * (double) getWidth()) : 0.0f;
}

juce::int64 SamplerEditor::Wave::frameAt (float x) const
{
    return (juce::int64) std::llround (view.getStart() + (double) x / juce::jmax (1.0, (double) getWidth()) * view.getLength());
}

void SamplerEditor::Wave::setSample (const Sampler::SamplePtr& sample)
{
    if (sample == shown)
        return;

    shown = sample;
    lows.clear();
    highs.clear();

    if (sample != nullptr)
    {
        const auto frames = sample->getNumFrames();
        const auto channels = sample->audio.getNumChannels();
        lows.assign ((size_t) ((frames + blockFrames - 1) / blockFrames), 0.0f);
        highs = lows;

        for (int at = 0; at < frames; at += blockFrames)
        {
            auto low = 1.0e9f, high = -1.0e9f;

            for (int c = 0; c < channels; ++c)
            {
                const auto range = juce::FloatVectorOperations::findMinAndMax (sample->audio.getReadPointer (c, at), juce::jmin (blockFrames, frames - at));
                low = juce::jmin (low, range.getStart());
                high = juce::jmax (high, range.getEnd());
            }

            lows[(size_t) (at / blockFrames)] = low;
            highs[(size_t) (at / blockFrames)] = high;
        }
    }

    showAll();
}

SamplerEditor::Wave::Handle SamplerEditor::Wave::handleAt (juce::Point<float> p, int& cut) const
{
    cut = 0;
    const auto& zones = editor.getZones();
    const auto selected = editor.getSelected();
    const auto near = [&] (juce::int64 frame, float within = 5.0f)     { return std::abs (p.x - xOf (frame)) <= within; };

    if (shown == nullptr || ! juce::isPositiveAndBelow (selected, (int) zones.size()))
        return Handle::none;

    if (editor.showsSlices())
    {
        for (int i = 1; i < (int) zones.size(); ++i)
            if (zones[(size_t) i].sample == shown && near (zones[(size_t) i].span().getStart()))
            {
                cut = i;
                return Handle::cut;
            }

        return Handle::none;
    }

    const auto& z = zones[(size_t) selected];
    const auto span = z.span();

    if (z.loop != 0 && ! z.oneShot)
    {
        const auto loopEnd = z.loopEnd < 0 ? span.getEnd() : z.loopEnd;

        // The crossfade's handle sits along the top, as far before the loop's end as the fade is long.
        if (z.loop == 1 && p.y < 16.0f && near (loopEnd - (juce::int64) std::llround (z.crossfade * 0.001 * shown->rate), 6.0f))
            return Handle::crossfade;

        if (near (z.loopStart))
            return Handle::loopStart;

        if (near (loopEnd))
            return Handle::loopEnd;
    }

    if (near (span.getStart()))
        return Handle::start;

    if (near (span.getEnd()))
        return Handle::end;

    return Handle::none;
}

void SamplerEditor::Wave::dragTo (Handle handle, int cut, juce::int64 frame)
{
    if (shown == nullptr || handle == Handle::none)
        return;

    const auto frames = (juce::int64) shown->getNumFrames();
    const auto rate = shown->rate;

    if (handle == Handle::cut)
    {
        auto zones = editor.getZones();

        if (cut < 1 || cut >= (int) zones.size())
            return;

        auto& before = zones[(size_t) (cut - 1)];
        auto& after = zones[(size_t) cut];
        const auto at = juce::jlimit (before.span().getStart() + shortestPart, after.span().getEnd() - shortestPart, frame);
        before.end = after.start = at;
        editor.setZones (std::move (zones));
        return;
    }

    editor.changeSelected ([&] (Sampler::Zone& z)
    {
        const auto span = z.span();
        const auto loopEnd = z.loopEnd < 0 ? span.getEnd() : z.loopEnd;

        switch (handle)
        {
            case Handle::start:         z.start = juce::jlimit ((juce::int64) 0, span.getEnd() - shortestPart, frame); break;
            case Handle::end:           z.end = juce::jlimit (span.getStart() + shortestPart, frames, frame); break;
            case Handle::loopStart:     z.loopStart = juce::jlimit (span.getStart(), loopEnd - shortestLoop, frame); break;
            case Handle::loopEnd:       z.loopEnd = juce::jlimit (z.loopStart + shortestLoop, span.getEnd(), frame); break;
            case Handle::crossfade:     z.crossfade = juce::jlimit (0.0f, crossfadeRange.end, (float) ((double) (loopEnd - frame) * 1000.0 / rate)); break;
            case Handle::none:
            case Handle::cut:           break;
        }
    });
}

bool SamplerEditor::Wave::addCut (juce::int64 frame)
{
    auto zones = editor.getZones();

    if (! editor.showsSlices() || (int) zones.size() >= Sampler::maxSlices)
        return false;

    for (size_t i = 0; i < zones.size(); ++i)
    {
        const auto span = zones[i].span();

        if (frame < span.getStart() + shortestPart || frame > span.getEnd() - shortestPart)
            continue;

        auto second = zones[i];
        second.id = 0;
        second.start = frame;
        second.end = span.getEnd();
        zones[i].end = frame;
        zones.insert (zones.begin() + (int) i + 1, second);

        // The slices keep their order on the keys, and those without a name of their own are numbered again.
        for (size_t k = 0; k < zones.size(); ++k)
        {
            zones[k].root = zones[k].lowKey = zones[k].highKey = Sampler::firstSliceKey + (int) k;

            if (zones[k].name.startsWith ("Slice ") || k == i + 1)
                zones[k].name = "Slice " + juce::String (k + 1);
        }

        editor.setZones (std::move (zones));
        return true;
    }

    return false;
}

bool SamplerEditor::Wave::removeCut (int cut)
{
    auto zones = editor.getZones();

    if (! editor.showsSlices() || cut < 1 || cut >= (int) zones.size())
        return false;

    zones[(size_t) (cut - 1)].end = zones[(size_t) cut].span().getEnd();
    zones.erase (zones.begin() + cut);

    for (size_t k = 0; k < zones.size(); ++k)
    {
        zones[k].root = zones[k].lowKey = zones[k].highKey = Sampler::firstSliceKey + (int) k;

        if (zones[k].name.startsWith ("Slice "))
            zones[k].name = "Slice " + juce::String (k + 1);
    }

    editor.select (juce::jmin (editor.getSelected(), (int) zones.size() - 1));
    editor.setZones (std::move (zones));
    return true;
}

void SamplerEditor::Wave::zoom (double by, float aroundX)
{
    if (shown == nullptr)
        return;

    const auto at = view.getStart() + (double) aroundX / juce::jmax (1.0, (double) getWidth()) * view.getLength();
    const auto length = view.getLength() * by;
    const auto share = (double) aroundX / juce::jmax (1.0, (double) getWidth());
    setView ({ at - share * length, at - share * length + length });
}

void SamplerEditor::Wave::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat();
    SoundControls::drawDisplayWell (g, area, 8, 2);

    const auto& zones = editor.getZones();
    const auto selected = editor.getSelected();
    const auto sample = juce::isPositiveAndBelow (selected, (int) zones.size()) ? zones[(size_t) selected].sample : nullptr;
    setSample (sample);

    if (shown == nullptr)
    {
        g.setColour (Theme::dimText);
        g.setFont (juce::FontOptions (14.0f));
        const auto missing = juce::isPositiveAndBelow (selected, (int) zones.size()) && zones[(size_t) selected].file != juce::File();
        g.drawText (missing ? zones[(size_t) selected].file.getFileName() + " is missing. Click Locate... to find it."
                            : juce::String ("Drop a sound here, or double-click to choose a file"),
                    area, juce::Justification::centred);
        return;
    }

    // The waveform: its lowest and highest in each pixel.
    const auto frames = shown->getNumFrames();
    const auto middle = area.getCentreY(), half = area.getHeight() * 0.46f;
    const auto perPixel = view.getLength() / juce::jmax (1.0, (double) getWidth());
    juce::Path shape;
    std::vector<float> tops ((size_t) getWidth()), bottoms ((size_t) getWidth());

    for (int x = 0; x < getWidth(); ++x)
    {
        const auto from = juce::jlimit (0, frames - 1, (int) (view.getStart() + x * perPixel));
        const auto to = juce::jlimit (from + 1, frames, (int) std::ceil (view.getStart() + (x + 1) * perPixel));
        auto low = 1.0e9f, high = -1.0e9f;

        if (perPixel >= 2.0 * blockFrames)
        {
            for (int b = from / blockFrames; b <= (to - 1) / blockFrames && b < (int) lows.size(); ++b)
            {
                low = juce::jmin (low, lows[(size_t) b]);
                high = juce::jmax (high, highs[(size_t) b]);
            }
        }
        else
        {
            for (int c = 0; c < shown->audio.getNumChannels(); ++c)
            {
                const auto range = juce::FloatVectorOperations::findMinAndMax (shown->audio.getReadPointer (c, from), to - from);
                low = juce::jmin (low, range.getStart());
                high = juce::jmax (high, range.getEnd());
            }
        }

        tops[(size_t) x] = middle - juce::jlimit (-1.0f, 1.0f, high) * half;
        bottoms[(size_t) x] = middle - juce::jlimit (-1.0f, 1.0f, low) * half;
    }

    if (! tops.empty())
    {
        shape.startNewSubPath (0.0f, tops[0]);

        for (size_t x = 1; x < tops.size(); ++x)
            shape.lineTo ((float) x, tops[x]);

        for (size_t x = bottoms.size(); x-- > 0;)
            shape.lineTo ((float) x, juce::jmax (bottoms[x], tops[x] + 1.0f));

        shape.closeSubPath();
    }

    const auto line = [&] (juce::int64 frame, juce::Colour colour, float thickness = 1.5f)
    {
        g.setColour (colour);
        g.fillRect (xOf (frame) - thickness * 0.5f, area.getY(), thickness, area.getHeight());
    };

    if (editor.showsSlices())
    {
        g.setColour (Theme::waveform);
        g.fillPath (shape);

        // The selected slice lit, a cut between each slice and the next, and each slice's number.
        if (juce::isPositiveAndBelow (selected, (int) zones.size()))
        {
            const auto span = zones[(size_t) selected].span();
            g.setColour (Theme::accent.withAlpha (0.18f));
            g.fillRect (juce::Rectangle<float>::leftTopRightBottom (xOf (span.getStart()), area.getY(), xOf (span.getEnd()), area.getBottom()));
        }

        g.setFont (juce::FontOptions (10.0f));

        for (size_t i = 0; i < zones.size(); ++i)
        {
            const auto start = zones[i].span().getStart();

            if (i > 0)
                line (start, Theme::accentLight, 1.0f);

            g.setColour (Theme::dimText);
            g.drawText (juce::String (i + 1), juce::Rectangle<float> (xOf (start) + 3.0f, area.getY() + 2.0f, 30.0f, 12.0f), juce::Justification::centredLeft);
        }
    }
    else
    {
        const auto& z = zones[(size_t) selected];
        const auto span = z.span();

        // What lies outside the part played is dimmed.
        g.setColour (Theme::waveform.withAlpha (0.35f));
        g.fillPath (shape);
        {
            const juce::Graphics::ScopedSaveState state (g);
            g.reduceClipRegion (juce::Rectangle<float>::leftTopRightBottom (xOf (span.getStart()), area.getY(), xOf (span.getEnd()), area.getBottom()).toNearestInt());
            g.setColour (Theme::waveform);
            g.fillPath (shape);
        }

        if (z.loop != 0 && ! z.oneShot)
        {
            const auto loopEnd = z.loopEnd < 0 ? span.getEnd() : z.loopEnd;
            const auto fade = (juce::int64) std::llround (z.crossfade * 0.001 * shown->rate);
            g.setColour (Theme::accent.withAlpha (0.16f));
            g.fillRect (juce::Rectangle<float>::leftTopRightBottom (xOf (z.loopStart), area.getY(), xOf (loopEnd), area.getBottom()));

            if (z.loop == 1)
            {
                // The crossfade: a wedge rising to the loop's end, and its handle along the top.
                juce::Path wedge;
                wedge.addTriangle (xOf (loopEnd - fade), area.getBottom(), xOf (loopEnd), area.getBottom(), xOf (loopEnd), area.getY());
                g.setColour (Theme::accentLight.withAlpha (0.22f));
                g.fillPath (wedge);
                g.setColour (Theme::accentLight);
                g.fillEllipse (xOf (loopEnd - fade) - 4.0f, area.getY() + 4.0f, 8.0f, 8.0f);
            }

            line (z.loopStart, Theme::accentLight);
            line (loopEnd, Theme::accentLight);
            g.setFont (juce::FontOptions (10.0f));
            g.setColour (Theme::accentLight);
            g.drawText (z.loop == 2 ? "LOOP <>" : "LOOP", juce::Rectangle<float> (xOf (z.loopStart) + 4.0f, area.getBottom() - 14.0f, 60.0f, 12.0f), juce::Justification::centredLeft);
        }

        line (span.getStart(), Theme::textStrong);
        line (span.getEnd(), Theme::textStrong);
    }

    // Where each voice is playing.
    for (const auto frame : playingFrames())
        line (frame, Theme::cursor, 1.0f);
}

std::vector<juce::int64> SamplerEditor::Wave::playingFrames() const
{
    std::vector<juce::int64> frames;
    const auto& zones = editor.getZones();
    const auto selected = editor.getSelected();

    for (auto& reading : editor.getSampler().getEngine().getReadings())
    {
        const auto position = reading.position.load (std::memory_order_relaxed);
        const auto id = reading.zone.load (std::memory_order_relaxed);

        if (position < 0.0f)
            continue;

        for (size_t i = 0; i < zones.size(); ++i)
            if (zones[i].id == id && zones[i].sample == shown && (editor.showsSlices() || (int) i == selected))
            {
                const auto span = zones[i].span();
                frames.push_back (zones[i].reverse ? span.getEnd() - 1 - (juce::int64) position : span.getStart() + (juce::int64) position);
            }
    }

    return frames;
}

void SamplerEditor::Wave::mouseMove (const juce::MouseEvent& e)
{
    int cut = 0;
    setMouseCursor (handleAt (e.position, cut) != Handle::none ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
}

void SamplerEditor::Wave::mouseDown (const juce::MouseEvent& e)
{
    dragging = handleAt (e.position, draggingCut);

    if (editor.showsSlices())
    {
        if (dragging == Handle::cut && e.mods.isAltDown())
        {
            removeCut (draggingCut);
            dragging = Handle::none;
            return;
        }

        // A click in a slice selects it.
        if (dragging == Handle::none)
        {
            const auto frame = frameAt (e.position.x);
            const auto& zones = editor.getZones();

            for (size_t i = 0; i < zones.size(); ++i)
                if (zones[i].span().contains (frame))
                    editor.select ((int) i);
        }
    }
}

void SamplerEditor::Wave::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging != Handle::none)
        dragTo (dragging, draggingCut, frameAt (e.position.x));
}

void SamplerEditor::Wave::mouseUp (const juce::MouseEvent&)
{
    dragging = Handle::none;
}

void SamplerEditor::Wave::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (shown == nullptr)
        editor.chooseFile();
    else if (editor.showsSlices())
        addCut (frameAt (e.position.x));
}

void SamplerEditor::Wave::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& details)
{
    if (e.mods.isCommandDown() || e.mods.isCtrlDown())
    {
        zoom (std::pow (2.0, -(double) details.deltaY * 2.0), e.position.x);
        return;
    }

    const auto by = std::abs (details.deltaX) > std::abs (details.deltaY) ? details.deltaX : details.deltaY;
    setView (view - (double) by * view.getLength() * 0.5);
}

void SamplerEditor::Wave::mouseMagnify (const juce::MouseEvent& e, float scale)
{
    zoom (1.0 / juce::jmax (0.1, (double) scale), e.position.x);
}

//==============================================================================
SamplerEditor::KeyMap::KeyMap (SamplerEditor& e) : editor (e) {}

juce::Rectangle<float> SamplerEditor::KeyMap::keysArea() const
{
    return getLocalBounds().toFloat().removeFromBottom ((float) keysHeight);
}

juce::Rectangle<float> SamplerEditor::KeyMap::zonesArea() const
{
    return getLocalBounds().toFloat().withTrimmedBottom ((float) keysHeight + 2.0f);
}

int SamplerEditor::KeyMap::keyAt (float x) const
{
    return juce::jlimit (0, 127, (int) (x / ((float) getWidth() / 128.0f)));
}

int SamplerEditor::KeyMap::velocityAt (float y) const
{
    const auto area = zonesArea();
    return juce::jlimit (1, 127, 127 - (int) ((y - area.getY()) / (area.getHeight() / 127.0f)));
}

juce::Rectangle<float> SamplerEditor::KeyMap::zoneBounds (int zone) const
{
    const auto& zones = editor.getZones();

    if (! juce::isPositiveAndBelow (zone, (int) zones.size()))
        return {};

    const auto& z = zones[(size_t) zone];
    const auto area = zonesArea();
    const auto key = (float) getWidth() / 128.0f, step = area.getHeight() / 127.0f;
    return juce::Rectangle<float>::leftTopRightBottom ((float) z.lowKey * key, area.getY() + (float) (127 - z.highVelocity) * step,
                                                       (float) (z.highKey + 1) * key, area.getY() + (float) (128 - z.lowVelocity) * step);
}

juce::Rectangle<float> SamplerEditor::KeyMap::padBounds (int slice) const
{
    const auto count = (int) editor.getZones().size();

    if (! juce::isPositiveAndBelow (slice, count))
        return {};

    const auto rows = juce::jmax (1, (count + padColumns - 1) / padColumns);
    const auto w = (float) getWidth() / (float) padColumns, h = (float) getHeight() / (float) rows;
    return juce::Rectangle<float> ((float) (slice % padColumns) * w, (float) (slice / padColumns) * h, w, h).reduced (1.5f);
}

int SamplerEditor::KeyMap::zoneAt (juce::Point<float> p) const
{
    const auto count = (int) editor.getZones().size();

    if (editor.showsSlices())
    {
        for (int i = 0; i < count; ++i)
            if (padBounds (i).contains (p))
                return i;

        return -1;
    }

    if (zoneBounds (editor.getSelected()).expanded (3.0f).contains (p))
        return editor.getSelected();

    for (int i = count; --i >= 0;)
        if (zoneBounds (i).expanded (3.0f).contains (p))
            return i;

    return -1;
}

void SamplerEditor::KeyMap::dragEdge (int zone, Edge edge, int value)
{
    auto zones = editor.getZones();

    if (! juce::isPositiveAndBelow (zone, (int) zones.size()))
        return;

    auto& z = zones[(size_t) zone];

    switch (edge)
    {
        case Edge::low:     z.lowKey = juce::jlimit (0, z.highKey, value); break;
        case Edge::high:    z.highKey = juce::jlimit (z.lowKey, 127, value); break;
        case Edge::top:     z.highVelocity = juce::jlimit (z.lowVelocity, 127, value); break;
        case Edge::bottom:  z.lowVelocity = juce::jlimit (1, z.highVelocity, value); break;
        case Edge::none:    return;
    }

    editor.setZones (std::move (zones));
}

bool SamplerEditor::KeyMap::isLit (int slice) const
{
    const auto& zones = editor.getZones();

    if (! juce::isPositiveAndBelow (slice, (int) zones.size()))
        return false;

    for (auto& reading : editor.getSampler().getEngine().getReadings())
        if (reading.position.load (std::memory_order_relaxed) >= 0.0f && reading.zone.load (std::memory_order_relaxed) == zones[(size_t) slice].id)
            return true;

    return false;
}

void SamplerEditor::KeyMap::paint (juce::Graphics& g)
{
    const auto& zones = editor.getZones();
    const auto selected = editor.getSelected();
    g.setFont (juce::FontOptions (10.0f));

    if (editor.showsSlices())
    {
        for (int i = 0; i < (int) zones.size(); ++i)
        {
            const auto pad = padBounds (i);
            const auto lit = isLit (i);
            g.setColour (lit ? Theme::accent : i == selected ? Theme::accentDeep : Theme::raised);
            g.fillRoundedRectangle (pad, 3.0f);
            g.setColour (i == selected ? Theme::accentLight : Theme::line);
            g.drawRoundedRectangle (pad, 3.0f, 1.0f);
            g.setColour (lit || i == selected ? Theme::textStrong : Theme::text);
            g.drawFittedText (zones[(size_t) i].name.isNotEmpty() ? zones[(size_t) i].name : juce::String (i + 1), pad.reduced (3.0f, 1.0f).toNearestInt(),
                              juce::Justification::centred, 2, 0.8f);
        }

        return;
    }

    // The keys, C's marked, and the zones over them: across their keys, up their velocities.
    const auto keys = keysArea();
    const auto key = (float) getWidth() / 128.0f;
    g.setColour (Theme::inset);
    g.fillRect (zonesArea());

    for (int k = 0; k < 128; ++k)
    {
        const auto r = juce::Rectangle<float> ((float) k * key, keys.getY(), key, keys.getHeight());
        g.setColour (isBlack (k) ? Theme::inset : Theme::text.withAlpha (0.85f));
        g.fillRect (r.reduced (0.5f, 0.0f));

        if (k % 12 == 0)
        {
            g.setColour (Theme::dimText);
            g.drawText ("C" + juce::String (k / 12 - 1), juce::Rectangle<float> (r.getX() + 1.0f, zonesArea().getBottom() - 11.0f, 30.0f, 10.0f), juce::Justification::centredLeft);
        }
    }

    for (int i = 0; i < (int) zones.size(); ++i)
    {
        const auto r = zoneBounds (i);
        const auto missing = zones[(size_t) i].sample == nullptr;
        g.setColour ((missing ? Theme::danger : Theme::accent).withAlpha (i == selected ? 0.55f : 0.25f));
        g.fillRect (r);
        g.setColour (i == selected ? Theme::accentLight : Theme::accent.withAlpha (0.7f));
        g.drawRect (r, i == selected ? 1.5f : 1.0f);

        // Its root, on the keys.
        g.setColour (i == selected ? Theme::accentLight : Theme::accent);
        g.fillEllipse (((float) zones[(size_t) i].root + 0.5f) * key - 3.0f, keys.getCentreY() - 3.0f, 6.0f, 6.0f);
    }
}

void SamplerEditor::KeyMap::mouseDown (const juce::MouseEvent& e)
{
    dragging = Edge::none;
    draggingZone = -1;

    if (editor.showsSlices())
    {
        if (const auto pad = zoneAt (e.position); pad >= 0)
        {
            editor.select (pad);
            const auto bounds = padBounds (pad);
            playing = editor.getZones()[(size_t) pad].lowKey;
            editor.getSampler().play (playing, juce::jlimit (0.2f, 1.0f, 1.0f - (e.position.y - bounds.getY()) / bounds.getHeight() * 0.8f));
        }

        return;
    }

    if (keysArea().contains (e.position))
    {
        playing = keyAt (e.position.x);
        editor.getSampler().play (playing, 0.8f);
        return;
    }

    if (const auto zone = zoneAt (e.position); zone >= 0)
    {
        editor.select (zone);
        draggingZone = zone;
        const auto r = zoneBounds (zone);
        dragging = std::abs (e.position.x - r.getX()) <= 4.0f ? Edge::low
                 : std::abs (e.position.x - r.getRight()) <= 4.0f ? Edge::high
                 : std::abs (e.position.y - r.getY()) <= 4.0f ? Edge::top
                 : std::abs (e.position.y - r.getBottom()) <= 4.0f ? Edge::bottom : Edge::none;
    }
}

void SamplerEditor::KeyMap::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging == Edge::low || dragging == Edge::high)
        dragEdge (draggingZone, dragging, keyAt (e.position.x));
    else if (dragging == Edge::top || dragging == Edge::bottom)
        dragEdge (draggingZone, dragging, velocityAt (e.position.y));
}

void SamplerEditor::KeyMap::mouseUp (const juce::MouseEvent&)
{
    if (playing >= 0)
        editor.getSampler().stop (playing);

    playing = -1;
    dragging = Edge::none;
}

//==============================================================================
SamplerEditor::SamplerEditor (SamplerProcessor& p)
    : AudioProcessorEditor (p), sampler (p),
      presets (EffectPresets::userStore(), p.getPresetKey(), [&p] { return p.getPresetValues(); },
               [&p] (const EffectPresets::Values& v) { p.setPresetValues (v); })
{
    setOpaque (true);
    presets.carryTexts ([&p] { return p.getPresetTexts(); }, [&p] (const EffectPresets::Texts& t) { p.setPresetTexts (t); });

    for (auto* panel : { &wavePanel, &keysPanel, &zonePanel, &ampPanel, &filterPanel, &lfoPanel, &voicePanel })
        addAndMakeVisible (*panel);

    wave = std::make_unique<Wave> (*this);
    keyMap = std::make_unique<KeyMap> (*this);
    wavePanel.addAndMakeVisible (*wave);
    keysPanel.addAndMakeVisible (*keyMap);

    const auto attach = [this] (Knob& k, SoundControls::Panel& panel, const char* id, const juce::String& tip)
    {
        k.attach (sampler.parameter (id));
        k.setTooltip (tip);
        panel.addAndMakeVisible (k);
    };

    attach (attack, ampPanel, "attack", "How long a note takes to rise");
    attach (decay, ampPanel, "decay", "How long it takes to fall to the sustain level");
    attach (sustain, ampPanel, "sustain", "The level a held note stays at");
    attach (release, ampPanel, "release", "How long a note takes to die away when it is let go");
    attach (velocity, ampPanel, "velocity", "How much a note's velocity sets its level");
    attach (cutoff, filterPanel, "cutoff", "Where the filter sits");
    attach (resonance, filterPanel, "resonance", "How much the filter rings at its cutoff");
    attach (keyTrack, filterPanel, "keyTrack", "How far the cutoff follows the key, around C4");
    attach (filterEnv, filterPanel, "filterEnv", "How far the filter's envelope moves the cutoff: 100 % is eight octaves up");
    attach (filterAttack, filterPanel, "filterAttack", "The filter envelope's rise");
    attach (filterDecay, filterPanel, "filterDecay", "The filter envelope's fall to its sustain level");
    attach (filterSustain, filterPanel, "filterSustain", "The level the filter's envelope holds");
    attach (filterRelease, filterPanel, "filterRelease", "The filter envelope's fall when the note is let go");
    attach (velocityToCutoff, filterPanel, "velocityToCutoff", "How far a hard note opens the filter: 100 % is four octaves at full velocity");
    attach (lfoRate, lfoPanel, "lfoRate", "The LFO's rate, when Sync is Off");
    attach (lfoFade, lfoPanel, "lfoFade", "How long the LFO takes to come in after a note starts");
    attach (lfoPitch, lfoPanel, "lfoPitch", "How far the LFO moves the pitch either way");
    attach (lfoFilter, lfoPanel, "lfoFilter", "How far the LFO moves the cutoff: 100 % is four octaves either way");
    attach (lfoLevel, lfoPanel, "lfoLevel", "How far the LFO dips the level");
    attach (lfoPan, lfoPanel, "lfoPan", "How far the LFO moves the sound left and right");
    attach (level, voicePanel, "level", "The output level");
    attach (pan, voicePanel, "pan", "Left or right");
    attach (transpose, voicePanel, "transpose", "Every note this many semitones higher or lower");
    attach (fine, voicePanel, "fine", "Fine tuning, in cents");
    attach (bendRange, voicePanel, "bendRange", "How far the pitch bend wheel goes either way");
    attach (glide, voicePanel, "glide", "Each note slides from the last over about this time; at 0 it doesn't");
    attach (wheel, voicePanel, "wheel", "The vibrato the mod wheel adds at its top");

    const auto whole = [] (float v) { return juce::String (juce::roundToInt (v)); };
    transpose.setFormatter ([] (float v) { return (v > 0.0f ? "+" : "") + juce::String (juce::roundToInt (v)) + " st"; });
    bendRange.setFormatter ([whole] (float v) { return whole (v) + " st"; });

    for (auto* k : { &pan, &fine, &transpose, &filterEnv, &lfoFilter })
        k->setBipolar (true);

    const auto choice = [this] (SoundControls::StepSelector& s, SoundControls::Panel& panel, const char* id, const juce::String& tip)
    {
        s.attach (dynamic_cast<juce::AudioParameterChoice*> (sampler.parameter (id)));
        s.setTooltip (tip);
        panel.addAndMakeVisible (s);
    };

    choice (filterType, filterPanel, "filter", "Off, a low-pass at 12 or 24 dB an octave, a band-pass or a high-pass");
    choice (lfoShape, lfoPanel, "lfoShape", "The LFO's shape");
    choice (lfoSync, lfoPanel, "lfoSync", "Off: the Rate knob, in Hz. Otherwise the LFO's cycle in note values at the session's tempo");
    choice (voices, voicePanel, "voices", "Poly: up to 32 notes at once. Mono: one, each new note taking over from the last");

    // The selected zone's settings: they are the zone's, not hosted parameters.
    const auto zoneKnob = [this] (Knob& k, const juce::NormalisableRange<float>& range, float start, const juce::String& unit, const juce::String& tip,
                                  std::function<void (Sampler::Zone&, float)> set)
    {
        k.link().setStandalone (range, start, unit);
        k.setTooltip (tip);
        zonePanel.addAndMakeVisible (k);
        k.onValueChanged = [this, &k, apply = std::move (set)]
        {
            if (! showing)
                changeSelected ([&] (Sampler::Zone& z) { apply (z, k.link().getPlain()); });
        };
    };

    zoneKnob (root, rootRange, 60.0f, {}, "The key that plays the sample at its own pitch", [] (Sampler::Zone& z, float v) { z.root = juce::roundToInt (v); });
    zoneKnob (tune, tuneRange, 0.0f, "ct", "The zone's tuning, in cents", [] (Sampler::Zone& z, float v) { z.tune = v; });
    zoneKnob (zoneLevel, zoneLevelRange, 0.0f, "dB", "The zone's level", [] (Sampler::Zone& z, float v) { z.level = v; });
    zoneKnob (zonePan, zonePanRange, 0.0f, {}, "The zone's place, left or right", [] (Sampler::Zone& z, float v) { z.pan = v; });
    zoneKnob (crossfade, crossfadeRange, 0.0f, "ms", "How much of a forward loop's end is crossfaded into what comes before its start",
              [] (Sampler::Zone& z, float v) { z.crossfade = v; });
    root.setFormatter ([] (float v) { return Instrument::pitchName (juce::roundToInt (v)); });
    tune.setBipolar (true);
    zonePan.setBipolar (true);

    sensitivity.link().setStandalone (sensitivityRange, 70.0f, "%");
    sensitivity.setTooltip ("For slices cut at hits: at 100 % every attack found is a cut, lower only the louder ones. Slice Again cuts with it");
    zonePanel.addAndMakeVisible (sensitivity);
    sensitivity.onValueChanged = [this]
    {
        if (! showing)
        {
            auto how = sampler.getSlicing();
            how.sensitivity = sensitivity.link().getPlain();
            sampler.setSlicing (how);
        }
    };

    const auto toggle = [this] (SoundControls::LedToggle& t, const juce::String& tip, std::function<void (Sampler::Zone&, bool)> set)
    {
        t.setTooltip (tip);
        zonePanel.addAndMakeVisible (t);
        t.onToggle = [this, apply = std::move (set)] (bool on)
        {
            if (! showing)
                changeSelected ([&] (Sampler::Zone& z) { apply (z, on); });
        };
    };

    toggle (reverse, "Plays the part backwards", [] (Sampler::Zone& z, bool on) { z.reverse = on; });
    toggle (oneShot, "Plays to the end whatever the note's length, as a drum hit does; it doesn't loop", [] (Sampler::Zone& z, bool on) { z.oneShot = on; });
    toggle (loopInRelease, "On: the loop goes on while the note dies away. Off: a note let go plays on from the loop to the sample's end",
            [] (Sampler::Zone& z, bool on) { z.loopInRelease = on; });

    loopMode.setChoices ({ "No Loop", "Loop", "Loop Back and Forth" });
    loopMode.setTooltip ("Whether the zone loops between the loop handles while its note sounds, forward or forward and back");
    loopMode.onChange = [this] (int index)
    {
        if (! showing)
            changeSelected ([&] (Sampler::Zone& z)
            {
                // A loop switched on for the first time takes the last half of the part.
                if (z.loop == 0 && index != 0 && z.loopStart == 0 && z.loopEnd < 0)
                    z.loopStart = z.span().getStart() + z.span().getLength() / 2;

                z.loop = index;
            });
    };
    zonePanel.addAndMakeVisible (loopMode);

    choke.setChoices ({ "No Choke", "Choke 1", "Choke 2", "Choke 3", "Choke 4", "Choke 5", "Choke 6", "Choke 7", "Choke 8" });
    choke.setTooltip ("Zones of one choke group cut each other off, as a closed hat does an open one");
    choke.onChange = [this] (int index)
    {
        if (! showing)
            changeSelected ([&] (Sampler::Zone& z) { z.choke = index; });
    };
    zonePanel.addAndMakeVisible (choke);

    // Keys or slices, and how the slices are cut.
    mode.setChoices ({ "Keys", "Slices" });
    mode.setTooltip ("Keys: samples across the keys, in zones. Slices: one sample cut into pieces, each on its own key from C2");
    mode.onChange = [this] (int index)
    {
        if (! showing)
            setSlices (index == 1);
    };
    addAndMakeVisible (mode);

    sliceSource.setChoices (Sampler::sliceSourceNames());
    sliceSource.setTooltip ("Where the cuts come from: the file's markers, its regions, its hits, or equal parts. Slice Again cuts with it");
    sliceSource.onChange = [this] (int index)
    {
        if (! showing)
        {
            auto how = sampler.getSlicing();
            how.source = (Sampler::SliceSource) index;
            sampler.setSlicing (how);
            showZone();
        }
    };
    keysPanel.addAndMakeVisible (sliceSource);

    sliceParts.setChoices ({ "2 Parts", "4 Parts", "8 Parts", "16 Parts", "32 Parts", "64 Parts" });
    sliceParts.onChange = [this] (int index)
    {
        if (! showing)
        {
            auto how = sampler.getSlicing();
            how.parts = 2 << index;
            sampler.setSlicing (how);
        }
    };
    keysPanel.addAndMakeVisible (sliceParts);

    cutEachOther.setTooltip ("Each slice stops the last, as playing a break by hand needs");
    cutEachOther.onToggle = [this] (bool on)
    {
        if (showing)
            return;

        auto how = sampler.getSlicing();
        how.cutEachOther = on;
        sampler.setSlicing (how);
        auto changed = zoneList;

        for (auto& z : changed)
            z.choke = on ? 1 : 0;

        setZones (std::move (changed));
    };
    keysPanel.addAndMakeVisible (cutEachOther);

    open.onClick = [this] { chooseFile(); };
    sf2PresetButton.onClick = [this] { showSf2Presets(); };
    pitchButton.onClick = [this] { findPitch(); };
    loopButton.onClick = [this] { findLoop(); };
    locateButton.onClick = [this] { locate(); };
    removeButton.onClick = [this] { removeZone(); };
    sliceButton.onClick = [this] { sliceAgain(); };
    open.setTooltip ("Choose a sound, SFZ, SoundFont 2 (.sf2), or MPC (.xpm) program");
    sf2PresetButton.setTooltip ("Choose a preset from the loaded SoundFont 2 (.sf2) bank");
    pitchButton.setTooltip ("Measures the sample's pitch and sets the zone's root and tuning from it");
    loopButton.setTooltip ("Looks for a loop in the held part of the sound, and sets the loop and its crossfade: a suggestion, to change with the handles");
    locateButton.setTooltip ("Finds a missing sample: choose it where it is now");
    removeButton.setTooltip ("Takes the selected zone away");
    sliceButton.setTooltip ("Cuts the sample again from its start, as Source, Sensitivity and Parts say: cuts moved or added by hand are lost");

    wavePanel.addAndMakeVisible (open);
    wavePanel.addChildComponent (sf2PresetButton);
    wavePanel.addChildComponent (locateButton);
    statusLine.setJustificationType (juce::Justification::centredRight);
    statusLine.setFont (juce::FontOptions (12.0f));
    statusLine.setColour (juce::Label::textColourId, Theme::dimText);
    statusLine.setInterceptsMouseClicks (false, false);
    wavePanel.addAndMakeVisible (statusLine);

    for (auto* b : { &pitchButton, &loopButton, &removeButton, &sliceButton })
        zonePanel.addAndMakeVisible (*b);

    addAndMakeVisible (presets);
    setSize (windowWidth, windowHeight);
    readZones();
    startTimerHz (30);
}

SamplerEditor::~SamplerEditor()
{
    stopTimer();
}

std::vector<Knob*> SamplerEditor::allKnobs()
{
    return { &root, &tune, &zoneLevel, &zonePan, &crossfade, &sensitivity, &attack, &decay, &sustain, &release, &velocity,
             &cutoff, &resonance, &keyTrack, &filterEnv, &filterAttack, &filterDecay, &filterSustain, &filterRelease, &velocityToCutoff,
             &lfoRate, &lfoFade, &lfoPitch, &lfoFilter, &lfoLevel, &lfoPan, &level, &pan, &transpose, &fine, &bendRange, &glide, &wheel };
}

Knob* SamplerEditor::knob (const juce::String& name)
{
    for (auto* k : allKnobs())
        if (k->getLabel() == name)
            return k;

    return nullptr;
}

void SamplerEditor::select (int zone)
{
    const auto next = zoneList.empty() ? -1 : juce::jlimit (0, (int) zoneList.size() - 1, zone);

    if (next != selectedZone)
    {
        selectedZone = next;
        showZone();
    }
}

void SamplerEditor::setZones (std::vector<Sampler::Zone> changed)
{
    sampler.setZones (std::move (changed));
    readZones();
}

void SamplerEditor::changeSelected (const std::function<void (Sampler::Zone&)>& change)
{
    if (! juce::isPositiveAndBelow (selectedZone, (int) zoneList.size()))
        return;

    auto changed = zoneList;
    change (changed[(size_t) selectedZone]);
    setZones (std::move (changed));
}

void SamplerEditor::readZones()
{
    zoneList = sampler.getZones();
    seenVersion = sampler.getZonesVersion();
    selectedZone = zoneList.empty() ? -1 : juce::jlimit (0, (int) zoneList.size() - 1, selectedZone);
    showZone();
}

void SamplerEditor::showZone()
{
    const juce::ScopedValueSetter<bool> setting (showing, true);
    const auto how = sampler.getSlicing();
    const auto has = juce::isPositiveAndBelow (selectedZone, (int) zoneList.size());
    const auto slices = how.on;

    mode.setIndex (slices ? 1 : 0, false);
    sliceSource.setIndex ((int) how.source, false);
    sliceParts.setIndex (juce::jlimit (0, 5, (int) std::lround (std::log2 (juce::jmax (2, how.parts))) - 1), false);
    cutEachOther.setOn (how.cutEachOther, false);
    put (sensitivity, sensitivityRange, how.sensitivity);
    keysPanel.setTitle (slices ? "Slices" : "Keys");
    zonePanel.setTitle (slices ? "Slice" : "Zone");

    if (has)
    {
        const auto& z = zoneList[(size_t) selectedZone];
        put (root, rootRange, (float) z.root);
        put (tune, tuneRange, z.tune);
        put (zoneLevel, zoneLevelRange, z.level);
        put (zonePan, zonePanRange, z.pan);
        put (crossfade, crossfadeRange, z.crossfade);
        reverse.setOn (z.reverse, false);
        oneShot.setOn (z.oneShot, false);
        loopInRelease.setOn (z.loopInRelease, false);
        loopMode.setIndex (z.loop, false);
        choke.setIndex (juce::jlimit (0, 8, z.choke), false);

        if (z.sample != nullptr)
        {
            status = z.sample->name + "  |  " + juce::String (z.sample->rate / 1000.0, 1) + " kHz " + (z.sample->audio.getNumChannels() > 1 ? "stereo" : "mono")
                     + "  |  " + lengthText (*z.sample) + (slices ? "  |  slice " + juce::String (selectedZone + 1) + " of " + juce::String (zoneList.size()) + " on " + Instrument::pitchName (z.lowKey)
                                                                 : "  |  root " + Instrument::pitchName (z.root));
        }
        else
        {
            status = z.file != juce::File() ? z.file.getFileName() + " is missing" : juce::String ("No sample in this zone");
        }
    }
    else
    {
        status = "No sample yet";
    }

    // What the mode and the zone leave nothing to do with is dimmed and takes no clicks.
    const auto loops = has && zoneList[(size_t) selectedZone].loop != 0 && ! zoneList[(size_t) selectedZone].oneShot;
    const auto missing = has && zoneList[(size_t) selectedZone].sample == nullptr && zoneList[(size_t) selectedZone].file != juce::File();

    for (auto* c : std::initializer_list<juce::Component*> { &root, &tune, &zoneLevel, &zonePan, &reverse, &oneShot, &choke, &removeButton })
        c->setEnabled (has);

    for (auto* c : std::initializer_list<juce::Component*> { &loopMode, &loopButton })
        c->setEnabled (has && ! slices && ! zoneList[(size_t) selectedZone].oneShot);

    crossfade.setEnabled (loops && zoneList[(size_t) selectedZone].loop == 1);
    loopInRelease.setEnabled (loops);
    pitchButton.setEnabled (has && zoneList[(size_t) selectedZone].sample != nullptr);
    locateButton.setVisible (missing);
    removeButton.setVisible (! slices);
    sliceButton.setVisible (slices);
    sensitivity.setVisible (slices);
    sliceSource.setVisible (slices);
    sliceParts.setVisible (slices && how.source == Sampler::SliceSource::equalParts);
    cutEachOther.setVisible (slices);

    for (auto* k : allKnobs())
        k->setAlpha (k->isEnabled() ? 1.0f : 0.4f);

    for (auto* c : std::initializer_list<juce::Component*> { &reverse, &oneShot, &loopInRelease, &loopMode, &choke })
        c->setAlpha (c->isEnabled() ? 1.0f : 0.4f);

    statusLine.setText (status, juce::dontSendNotification);
    wave->setSample (has ? zoneList[(size_t) selectedZone].sample : nullptr);
    wave->repaint();
    keyMap->repaint();
    repaint();
}

void SamplerEditor::say (const juce::String& message)
{
    status = message;
    statusLine.setText (status, juce::dontSendNotification);
}

bool SamplerEditor::loadSf2Preset (int presetIndex)
{
    if (! sf2BankFile.existsAsFile() || ! juce::isPositiveAndBelow (presetIndex, (int) sf2Presets.size()))
        return false;

    const auto err = sampler.loadSf2 (sf2BankFile, presetIndex);
    if (err.isNotEmpty())
    {
        say (err);
        return false;
    }

    sf2PresetIndex = presetIndex;
    const auto& p = sf2Presets[(size_t) presetIndex];
    sf2PresetButton.setButtonText (p.name.isNotEmpty() ? p.name : ("Preset " + juce::String (presetIndex + 1)));
    readZones();
    select (0);
    wave->showAll();
    say ("Loaded " + p.name + " (" + juce::String ((int) zoneList.size()) + " zones) from " + sf2BankFile.getFileName() + ".");
    return true;
}

void SamplerEditor::showSf2Presets()
{
    if (sf2Presets.empty() || ! sf2BankFile.existsAsFile())
        return;

    juce::PopupMenu menu;
    const bool groupByBank = sf2Presets.size() > 30;

    if (groupByBank)
    {
        std::map<int, juce::PopupMenu> banks;
        for (int i = 0; i < (int) sf2Presets.size(); ++i)
        {
            const auto& p = sf2Presets[(size_t) i];
            const auto label = juce::String (p.program).paddedLeft ('0', 3) + "  " + p.name;
            banks[p.bank].addItem (i + 1, label, true, i == sf2PresetIndex);
        }
        for (auto& [bankNum, sub] : banks)
            menu.addSubMenu ("Bank " + juce::String (bankNum).paddedLeft ('0', 3), sub);
    }
    else
    {
        for (int i = 0; i < (int) sf2Presets.size(); ++i)
        {
            const auto& p = sf2Presets[(size_t) i];
            const auto label = juce::String (p.bank).paddedLeft ('0', 3) + ":" + juce::String (p.program).paddedLeft ('0', 3) + "  " + p.name;
            menu.addItem (i + 1, label, true, i == sf2PresetIndex);
        }
    }

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (sf2PresetButton),
                        [safe = juce::Component::SafePointer<SamplerEditor> (this)] (int chosen)
    {
        if (safe != nullptr && chosen > 0)
            safe->loadSf2Preset (chosen - 1);
    });
}

bool SamplerEditor::loadFile (const juce::File& file, int atKey)
{
    if (file.hasFileExtension ("sf2"))
    {
        juce::String listErr;
        auto listed = Sf2Import::listPresets (file, listErr);
        const auto err = sampler.loadSf2 (file, 0);
        if (err.isNotEmpty())
        {
            say (err);
            return false;
        }

        sf2BankFile = file;
        sf2Presets = std::move (listed);
        sf2PresetIndex = 0;
        const bool multi = sf2Presets.size() > 1;
        sf2PresetButton.setVisible (multi);
        if (multi)
            sf2PresetButton.setButtonText (sf2Presets[0].name.isNotEmpty() ? sf2Presets[0].name : "SF2 Preset");
        resized();
        readZones();
        select (0);
        wave->showAll();
        say ("Loaded " + file.getFileName() + " (" + juce::String ((int) zoneList.size()) + " zones"
             + (multi ? ", " + juce::String ((int) sf2Presets.size()) + " presets" : "") + ").");
        return true;
    }

    sf2BankFile = juce::File();
    sf2Presets.clear();
    sf2PresetButton.setVisible (false);
    resized();

    if (file.hasFileExtension ("sfz;xpm"))
    {
        const auto err = file.hasFileExtension ("sfz") ? sampler.loadSfz (file)
                                                       : sampler.loadMpcKeygroup (file);
        if (err.isNotEmpty())
        {
            say (err);
            return false;
        }

        readZones();
        select (0);
        wave->showAll();
        say ("Loaded " + file.getFileName() + " (" + juce::String ((int) zoneList.size()) + " zones).");
        return true;
    }

    juce::String error;
    const auto sample = Sampler::sharedSample (file, error);

    if (sample == nullptr)
    {
        say (error);
        return false;
    }

    // On the key map a sound becomes a new zone; anywhere else it is the selected zone's sample.
    const auto replace = atKey < 0 && juce::isPositiveAndBelow (selectedZone, (int) zoneList.size()) ? zoneList[(size_t) selectedZone].id : 0;
    error = sampler.addSample (sample, file, replace, atKey);

    if (error.isNotEmpty())
    {
        say (error);
        return false;
    }

    readZones();

    if (replace == 0)
        select (showsSlices() ? 0 : (int) zoneList.size() - 1);

    wave->showAll();
    return true;
}

bool SamplerEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;wave;bwf;aif;aiff;aifc;flac;ogg;opus;mp3;m4a;caf;sfz;sf2;xpm"))
            return true;

    return false;
}

void SamplerEditor::filesDropped (const juce::StringArray& files, int x, int y)
{
    if (files.size() == 1 && juce::File (files[0]).hasFileExtension ("sfz;sf2;xpm"))
    {
        loadFile (juce::File (files[0]));
        return;
    }

    // On the key map (playing keys, not slices) a sound lands as a new zone at the key under it.
    const auto onKeys = keyMap->getScreenBounds().contains (localPointToGlobal (juce::Point<int> (x, y))) && ! showsSlices();
    const auto key = onKeys ? keyMap->keyAt ((float) (localPointToGlobal (juce::Point<int> (x, y)).x - keyMap->getScreenX())) : -1;

    juce::Array<juce::File> sounds;

    for (auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;wave;bwf;aif;aiff;aifc;flac;ogg;opus;mp3;m4a;caf"))
            sounds.add (juce::File (f));

    if (sounds.size() == 1)
        loadFile (sounds[0], key);
    else if (sounds.size() > 1)
        loadFiles (sounds, key);
}

bool SamplerEditor::loadFiles (const juce::Array<juce::File>& files, int atKey)
{
    std::vector<std::pair<Sampler::SamplePtr, juce::File>> sounds;
    juce::String error;

    for (auto& f : files)
        if (auto sample = Sampler::sharedSample (f, error))
            sounds.emplace_back (sample, f);

    if (sounds.empty())
    {
        say (error);
        return false;
    }

    const auto refused = sampler.addSamples (sounds, atKey >= 0 ? atKey : Sampler::firstSliceKey);
    readZones();
    select ((int) zoneList.size() - 1);

    if (refused.isNotEmpty() || error.isNotEmpty())
        say (refused.isNotEmpty() ? refused : error);

    return refused.isEmpty();
}

bool SamplerEditor::isInterestedInDragSource (const SourceDetails& details)
{
    return SoundDrops::accepts (details.description);
}

void SamplerEditor::itemDropped (const SourceDetails& details)
{
    const auto sound = SoundDrops::take (details.description, Sampler::longestSampleSeconds);

    if (! sound.has_value())
        return;

    const auto onKeys = keyMap->getScreenBounds().contains (localPointToGlobal (details.localPosition)) && ! showsSlices();
    const auto key = onKeys ? keyMap->keyAt ((float) (localPointToGlobal (details.localPosition).x - keyMap->getScreenX())) : -1;

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

    loadFile (file, key);
}

void SamplerEditor::chooseFile()
{
    chooser = std::make_unique<juce::FileChooser> ("Choose a sound, SFZ, SF2 or MPC keygroup", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3;*.m4a;*.caf;*.sfz;*.sf2;*.xpm");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe = juce::Component::SafePointer<SamplerEditor> (this)] (const juce::FileChooser& c)
                          {
                              if (safe != nullptr && c.getResult() != juce::File())
                                  safe->loadFile (c.getResult());
                          });
}

void SamplerEditor::locate()
{
    if (! juce::isPositiveAndBelow (selectedZone, (int) zoneList.size()))
        return;

    const auto wanted = zoneList[(size_t) selectedZone].file;
    chooser = std::make_unique<juce::FileChooser> ("Find " + wanted.getFileName(), wanted.getParentDirectory(), "*" + wanted.getFileExtension());
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe = juce::Component::SafePointer<SamplerEditor> (this)] (const juce::FileChooser& c)
                          {
                              if (safe != nullptr && c.getResult() != juce::File())
                              {
                                  const auto found = safe->sampler.locate (c.getResult());
                                  safe->readZones();

                                  if (found == 0)
                                  {
                                      safe->say (c.getResult().getFileName() + " isn't one of the missing samples.");
                                  }
                              }
                          });
}

void SamplerEditor::findPitch()
{
    changeSelected ([] (Sampler::Zone& z)
    {
        if (z.sample == nullptr)
            return;

        const auto pitch = Sampler::findPitch (*z.sample, z.span());
        z.root = pitch.root;
        z.tune = pitch.tune;
    });

    if (juce::isPositiveAndBelow (selectedZone, (int) zoneList.size()) && zoneList[(size_t) selectedZone].sample != nullptr
        && ! Sampler::findPitch (*zoneList[(size_t) selectedZone].sample, zoneList[(size_t) selectedZone].span()).found)
    {
        say ("No clear pitch was found: the root is C4. Set it with the Root knob.");
    }
}

void SamplerEditor::findLoop()
{
    auto found = false;

    changeSelected ([&found] (Sampler::Zone& z)
    {
        if (z.sample == nullptr)
            return;

        const auto loop = Sampler::findLoop (*z.sample, z.span(), Sampler::findPitch (*z.sample, z.span()).hz);

        if (! loop.found)
            return;

        found = true;
        z.loop = 1;
        z.oneShot = false;
        z.loopStart = loop.start;
        z.loopEnd = loop.end;
        z.crossfade = loop.crossfade;
    });

    if (! found)
    {
        say ("The part is too short to loop.");
    }
}

void SamplerEditor::removeZone()
{
    if (! juce::isPositiveAndBelow (selectedZone, (int) zoneList.size()))
        return;

    auto changed = zoneList;
    changed.erase (changed.begin() + selectedZone);
    setZones (std::move (changed));
}

void SamplerEditor::sliceAgain()
{
    if (zoneList.empty() || zoneList[0].sample == nullptr)
        return;

    auto how = sampler.getSlicing();

    if (! sampler.slice (zoneList[0].sample, zoneList[0].file, {}, how))
    {
        say (how.source == Sampler::SliceSource::markers ? "The file has no markers to cut at."
               : how.source == Sampler::SliceSource::regions ? "The file has no regions to cut at." : "Nothing to cut.");
        return;
    }

    selectedZone = 0;
    readZones();
}

void SamplerEditor::setSlices (bool on)
{
    auto how = sampler.getSlicing();

    if (how.on == on)
        return;

    how.on = on;
    const auto sample = ! zoneList.empty() ? zoneList[juce::isPositiveAndBelow (selectedZone, (int) zoneList.size()) ? (size_t) selectedZone : 0].sample : nullptr;
    const auto file = ! zoneList.empty() ? zoneList[juce::isPositiveAndBelow (selectedZone, (int) zoneList.size()) ? (size_t) selectedZone : 0].file : juce::File();
    sampler.setSlicing (how);

    if (sample != nullptr)
    {
        if (on)
        {
            // The selected zone's sample, cut: at its hits when it has no markers or regions to cut at.
            if (Sampler::findSlices (*sample, {}, how).empty())
                how.source = Sampler::SliceSource::hits;

            sampler.slice (sample, file, {}, how);
        }
        else
        {
            // The sample whole again, as one zone over the keys.
            sampler.setZones ({});
            sampler.addSample (sample, file);
        }
    }

    selectedZone = 0;
    readZones();
    wave->showAll();
}

void SamplerEditor::timerCallback()
{
    if (sampler.getZonesVersion() != seenVersion)
        readZones();

    for (auto* k : allKnobs())
        if (k->link().get() != nullptr)
            k->refresh();

    for (auto* s : { &filterType, &lfoShape, &lfoSync, &voices })
        s->refresh();

    // The play heads and the pads' lights, while anything sounds (and once more as it stops).
    wave->repaint();
    keyMap->repaint();
}

void SamplerEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::base);

    auto top = getLocalBounds().reduced (12).removeFromTop (30);
    const auto brandCol = Theme::brandOrange();
    g.setColour (brandCol);
    g.fillRoundedRectangle ((float) top.getX(), (float) top.getY() + 7.0f, 3.0f, 16.0f, 1.5f);
    const auto titleFont = juce::Font (juce::FontOptions (15.0f, juce::Font::bold));
    const float baseY = (float) top.getCentreY() + 5.0f;
    float x = (float) top.getX() + 10.0f;
    juce::GlyphArrangement gTorq, gWave, gRest;
    gTorq.addLineOfText (titleFont, "TORQ", x, baseY);
    const float torqW = gTorq.getBoundingBox (0, -1, true).getWidth();
    gWave.addLineOfText (titleFont, "WAVE", x + torqW + 1.5f, baseY);
    const float waveRight = gWave.getBoundingBox (0, -1, true).getRight();
    gRest.addLineOfText (titleFont, "SAMPLER", waveRight + 6.0f, baseY);
    g.setColour (Theme::textStrong);
    gTorq.draw (g);
    g.setColour (brandCol);
    gWave.draw (g);
    g.setColour (Theme::dimText);
    gRest.draw (g);

    // The status keeps to the theme's colour when the theme changes.
    statusLine.setColour (juce::Label::textColourId, Theme::dimText);
}

void SamplerEditor::resized()
{
    auto r = getLocalBounds().reduced (12);

    auto top = r.removeFromTop (30);
    mode.setBounds (top.withTrimmedLeft (176).removeFromLeft (120).reduced (0, 2));
    presets.setBounds (top.removeFromRight (juce::jmax (360, top.getWidth() - 310)));
    r.removeFromTop (8);

    wavePanel.setBounds (r.removeFromTop (200));
    r.removeFromTop (8);
    {
        auto header = wavePanel.getHeaderBounds();
        open.setBounds (header.removeFromRight (80).reduced (0, 2));
        header.removeFromRight (6);
        if (sf2PresetButton.isVisible())
        {
            sf2PresetButton.setBounds (header.removeFromRight (140).reduced (0, 2));
            header.removeFromRight (6);
        }
        locateButton.setBounds (header.removeFromRight (80).reduced (0, 2));
        header.removeFromRight (6);
        statusLine.setBounds (header);
        wave->setBounds (wavePanel.getContentBounds());
    }

    keysPanel.setBounds (r.removeFromTop (112));
    r.removeFromTop (8);
    {
        auto header = keysPanel.getHeaderBounds();
        cutEachOther.setBounds (header.removeFromRight (130).reduced (0, 2));
        header.removeFromRight (8);
        sliceParts.setBounds (header.removeFromRight (110).reduced (0, 1));
        header.removeFromRight (8);
        sliceSource.setBounds (header.removeFromRight (140).reduced (0, 1));
        keyMap->setBounds (keysPanel.getContentBounds());
    }

    zonePanel.setBounds (r.removeFromTop (128));
    r.removeFromTop (8);
    {
        auto c = zonePanel.getContentBounds();
        const auto w = 72;

        for (auto* k : { &root, &tune, &zoneLevel, &zonePan, &crossfade })
            k->setBounds (c.removeFromLeft (w));

        c.removeFromLeft (10);
        auto selectors = c.removeFromLeft (160);
        loopMode.setBounds (selectors.removeFromTop (selectors.getHeight() / 2).withSizeKeepingCentre (156, 28));
        choke.setBounds (selectors.withSizeKeepingCentre (156, 28));
        c.removeFromLeft (10);

        auto toggles = c.removeFromLeft (136);
        const auto each = toggles.getHeight() / 3;
        reverse.setBounds (toggles.removeFromTop (each).reduced (0, 3));
        oneShot.setBounds (toggles.removeFromTop (each).reduced (0, 3));
        loopInRelease.setBounds (toggles.reduced (0, 3));
        c.removeFromLeft (10);

        auto buttons = c.removeFromLeft (104);
        pitchButton.setBounds (buttons.removeFromTop (each).reduced (0, 3));
        loopButton.setBounds (buttons.removeFromTop (each).reduced (0, 3));
        removeButton.setBounds (buttons.reduced (0, 3));
        sliceButton.setBounds (removeButton.getBounds());
        c.removeFromLeft (10);
        sensitivity.setBounds (c.removeFromLeft (w));
    }

    const auto row = [] (juce::Rectangle<int> area, std::initializer_list<juce::Component*> parts)
    {
        const auto each = area.getWidth() / juce::jmax (1, (int) parts.size());

        for (auto* p : parts)
            p->setBounds (area.removeFromLeft (each));
    };

    auto third = r.removeFromTop (106);
    r.removeFromTop (8);
    ampPanel.setBounds (third.removeFromLeft (300));
    third.removeFromLeft (8);
    filterPanel.setBounds (third);
    row (ampPanel.getContentBounds(), { &attack, &decay, &sustain, &release, &velocity });
    filterType.setBounds (filterPanel.getHeaderBounds().removeFromRight (150).reduced (0, 1));
    row (filterPanel.getContentBounds(), { &cutoff, &resonance, &keyTrack, &velocityToCutoff, &filterEnv, &filterAttack, &filterDecay, &filterSustain, &filterRelease });

    auto fourth = r.removeFromTop (106);
    lfoPanel.setBounds (fourth.removeFromLeft (440));
    fourth.removeFromLeft (8);
    voicePanel.setBounds (fourth);
    {
        auto header = lfoPanel.getHeaderBounds();
        lfoSync.setBounds (header.removeFromRight (110).reduced (0, 1));
        header.removeFromRight (8);
        lfoShape.setBounds (header.removeFromRight (120).reduced (0, 1));
        row (lfoPanel.getContentBounds(), { &lfoRate, &lfoFade, &lfoPitch, &lfoFilter, &lfoLevel, &lfoPan });
        voices.setBounds (voicePanel.getHeaderBounds().removeFromRight (100).reduced (0, 1));
        row (voicePanel.getContentBounds(), { &level, &pan, &transpose, &fine, &bendRange, &glide, &wheel });
    }
}

//==============================================================================
juce::AudioProcessorEditor* SamplerProcessor::createEditor()
{
    return new SamplerEditor (*this);
}
