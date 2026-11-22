#include "SoundControls.h"
#include "Theme.h"

namespace SoundControls
{
namespace
{
    constexpr float startAngle = juce::MathConstants<float>::pi * 1.25f;   // seven o'clock, as juce measures from twelve
    constexpr float sweep = juce::MathConstants<float>::pi * 1.5f;         // 270 degrees

    float angleFor (float normalised) noexcept     { return -startAngle + juce::jlimit (0.0f, 1.0f, normalised) * sweep; }

    juce::Font labelFont (float size)              { return juce::FontOptions (size, juce::Font::plain); }
    juce::Font boldFont (float size)               { return juce::FontOptions (size, juce::Font::bold); }

    /** An arc between two angles around a centre. */
    juce::Path arc (juce::Point<float> c, float r, float from, float to)
    {
        juce::Path p;
        p.addCentredArc (c.x, c.y, r, r, 0.0f, juce::jmin (from, to), juce::jmax (from, to), true);
        return p;
    }

    juce::Colour accentOr (juce::Colour c)         { return c.isTransparent() ? Theme::accent : c; }
}

//==============================================================================
juce::String formatValue (float v, const juce::String& unit)
{
    const auto u = unit.trim();
    const auto signedNumber = [] (float x, int decimals) { return (x > 0.0f ? "+" : "") + juce::String (x, decimals); };

    if (u == "Hz" || u == "kHz")
    {
        const auto hz = u == "kHz" ? v * 1000.0f : v;
        if (hz >= 10000.0f) return juce::String (hz / 1000.0f, 1) + " kHz";
        if (hz >= 1000.0f)  return juce::String (hz / 1000.0f, 2) + " kHz";
        if (hz >= 100.0f)   return juce::String (juce::roundToInt (hz)) + " Hz";
        if (hz >= 10.0f)    return juce::String (hz, 1) + " Hz";
        return juce::String (hz, 2) + " Hz";
    }

    if (u == "dB")
        return v <= -96.0f ? juce::String ("-inf dB") : juce::String (v, 1) + " dB";

    if (u == "ms" || u == "s")
    {
        const auto ms = u == "s" ? v * 1000.0f : v;
        if (ms >= 10000.0f) return juce::String (ms / 1000.0f, 1) + " s";
        if (ms >= 1000.0f)  return juce::String (ms / 1000.0f, 2) + " s";
        if (ms >= 100.0f)   return juce::String (juce::roundToInt (ms)) + " ms";
        if (ms >= 10.0f)    return juce::String (ms, 1) + " ms";
        return juce::String (ms, 2) + " ms";
    }

    if (u == "%")
        return juce::String (juce::roundToInt (v)) + " %";

    if (u == "+%")   // bipolar share
        return signedNumber ((float) juce::roundToInt (v), 0) + " %";

    if (u == "st")
        return std::abs (v - std::round (v)) < 0.005f ? signedNumber (std::round (v), 0) + " st" : signedNumber (v, 2) + " st";

    if (u == "ct")
        return signedNumber ((float) juce::roundToInt (v), 0) + " ct";

    if (u == "x")
        return juce::String (v, 2) + "x";

    const auto number = std::abs (v) >= 100.0f ? juce::String (juce::roundToInt (v)) : juce::String (v, 2);
    return u.isEmpty() ? number : number + " " + u;
}

float dragged (float start, float pixels, bool fine, float pixelsForRange) noexcept
{
    return juce::jlimit (0.0f, 1.0f, start + pixels / pixelsForRange * (fine ? 0.1f : 1.0f));
}

juce::Colour sourceColour (int index)
{
    // Warm for envelopes, cool for LFOs, bright for performance sources, violet for macros: told
    // apart in both light and dark themes.
    static const juce::uint32 colours[] = {
        0xffff9f43, 0xffff6b6b, 0xfff8c94a, 0xffff7eb6,   // envelopes
        0xff4dd0e1, 0xff5c9dff,                           // LFOs
        0xff7bd88f, 0xffb4e05a, 0xff52d6b0, 0xffe0e060,   // velocity, wheel, aftertouch, key
        0xffb388ff, 0xffd17bff, 0xff8f8cff, 0xffe28cff,   // macros
        0xffc0c0c0, 0xff9ad0ff                            // others
    };
    constexpr auto n = (int) (sizeof (colours) / sizeof (colours[0]));
    return juce::Colour (colours[((index % n) + n) % n]);
}

juce::String modDragText (int sourceIndex)          { return "mod:" + juce::String (sourceIndex); }

int sourceFromDragText (const juce::var& description)
{
    const auto s = description.toString();
    return s.startsWith ("mod:") && s.substring (4).containsOnly ("0123456789") && s.length() > 4 ? s.substring (4).getIntValue() : -1;
}

//==============================================================================
float ParameterLink::getNormalised() const
{
    return parameter != nullptr ? parameter->getValue() : ownRange.convertTo0to1 (own);
}

float ParameterLink::getDefaultNormalised() const
{
    return parameter != nullptr ? parameter->getDefaultValue() : ownRange.convertTo0to1 (ownDefault);
}

float ParameterLink::getPlain() const
{
    return toPlain (getNormalised());
}

float ParameterLink::toPlain (float normalised) const
{
    return parameter != nullptr ? parameter->convertFrom0to1 (normalised) : ownRange.convertFrom0to1 (normalised);
}

juce::String ParameterLink::getUnit() const
{
    return parameter != nullptr ? parameter->getLabel() : ownUnit;
}

void ParameterLink::begin()
{
    if (parameter != nullptr && ! inGesture)
        parameter->beginChangeGesture();

    inGesture = true;
}

void ParameterLink::set (float normalised)
{
    normalised = juce::jlimit (0.0f, 1.0f, normalised);

    if (parameter == nullptr)
    {
        own = ownRange.convertFrom0to1 (normalised);
        return;
    }

    if (! juce::exactlyEqual (parameter->getValue(), normalised))
    {
        const auto wrap = ! inGesture;

        if (wrap)
            parameter->beginChangeGesture();

        parameter->setValueNotifyingHost (normalised);

        if (wrap)
            parameter->endChangeGesture();
    }
}

void ParameterLink::end()
{
    if (parameter != nullptr && inGesture)
        parameter->endChangeGesture();

    inGesture = false;
}

void ParameterLink::setStandalone (juce::NormalisableRange<float> range, float defaultPlain, const juce::String& unit)
{
    parameter = nullptr;
    ownRange = range;
    own = ownDefault = range.snapToLegalValue (defaultPlain);
    ownUnit = unit;
}

//==============================================================================
Knob::Knob (const juce::String& name, Size s)
    : label (name), size (s)
{
    setRepaintsOnMouseActivity (false);
    setWantsKeyboardFocus (false);
    setTitle (name);
}

void Knob::attach (juce::RangedAudioParameter* p)
{
    parameter = ParameterLink (p);
    shown = -1.0f;
    refresh();
}

void Knob::setRings (std::vector<ModRing> r)
{
    const auto same = r.size() == rings.size()
                      && std::equal (r.begin(), r.end(), rings.begin(), [] (const ModRing& a, const ModRing& b)
                                     { return a.colour == b.colour && juce::exactlyEqual (a.depth, b.depth) && a.slot == b.slot; });

    if (! same)
    {
        rings = std::move (r);
        repaint();
    }
}

void Knob::setLiveValue (float normalised)
{
    // Moved by less than a pixel's worth, it isn't redrawn.
    if (std::abs (normalised - live) > 0.002f || (normalised < 0.0f) != (live < 0.0f))
    {
        live = normalised;
        repaint();
    }
}

void Knob::refresh()
{
    if (const auto now = parameter.getNormalised(); ! juce::exactlyEqual (now, shown))
    {
        shown = now;
        repaint();
    }
}

void Knob::setNormalised (float v)
{
    parameter.set (v);
    refresh();

    if (onValueChanged != nullptr)
        onValueChanged();
}

void Knob::resetToDefault()
{
    parameter.begin();
    setNormalised (parameter.getDefaultNormalised());
    parameter.end();
}

juce::String Knob::getValueText() const
{
    const auto plain = parameter.getPlain();
    return formatter != nullptr ? formatter (plain) : formatValue (plain, parameter.getUnit());
}

juce::Rectangle<float> Knob::getDialBounds() const
{
    auto r = getLocalBounds().toFloat();
    const auto labelHeight = size == Size::small ? 13.0f : 15.0f;
    r.removeFromBottom (labelHeight);
    const auto side = juce::jmax (8.0f, juce::jmin (r.getWidth(), r.getHeight()));
    return r.withSizeKeepingCentre (side, side);
}

bool Knob::onRingBand (juce::Point<float> p) const
{
    if (rings.empty())
        return false;

    const auto dial = getDialBounds();
    const auto d = p.getDistanceFrom (dial.getCentre());
    return d > dial.getWidth() * 0.5f - 3.0f && d < dial.getWidth() * 0.5f + 4.0f;
}

void Knob::paint (juce::Graphics& g)
{
    const auto dial = getDialBounds();
    const auto c = dial.getCentre();
    const auto outer = dial.getWidth() * 0.5f;
    const auto ringWidth = size == Size::small ? 2.0f : 2.5f;
    const auto trackRadius = outer - ringWidth * 0.5f - (rings.empty() ? 0.0f : ringWidth + 1.5f);
    const auto trackWidth = size == Size::large ? 4.0f : size == Size::normal ? 3.0f : 2.5f;
    const auto bodyRadius = trackRadius - trackWidth - 2.5f;
    const auto value = parameter.getNormalised();
    const auto colour = accentOr (tint);
    const auto light = Theme::current().light;
    const auto enabled = isEnabled();

    // The track, the whole sweep (a well in a dark theme, a grey groove in a light one), then the value along it.
    const auto groove = light ? Theme::line.withAlpha (0.9f) : Theme::inset;
    g.setColour (groove);
    g.strokePath (arc (c, trackRadius, -startAngle, -startAngle + sweep), juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const auto from = angleFor (bipolar ? 0.5f : 0.0f), to = angleFor (value);

    if (std::abs (to - from) > 0.001f)
    {
        const auto valueColour = enabled ? colour : Theme::faintText;
        g.setColour (valueColour.withAlpha (0.25f));
        g.strokePath (arc (c, trackRadius, from, to), juce::PathStrokeType (trackWidth + 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (valueColour);
        g.strokePath (arc (c, trackRadius, from, to), juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // The modulation: an arc a slot outside the track, from the value to its full depth.
    if (! rings.empty())
    {
        const auto ringRadius = outer - ringWidth * 0.5f;
        g.setColour (groove.withAlpha (0.8f));
        g.strokePath (arc (c, ringRadius, -startAngle, -startAngle + sweep), juce::PathStrokeType (ringWidth));
        const auto count = (float) rings.size();

        for (size_t i = 0; i < rings.size(); ++i)
        {
            const auto& ring = rings[i];
            const auto target = juce::jlimit (0.0f, 1.0f, value + ring.depth);
            // Several rings share the band, each a slice of its width, so all show.
            const auto w = ringWidth / juce::jmax (1.0f, count * 0.6f);
            const auto r = ringRadius - ringWidth * 0.5f + w * 0.5f + (count > 1 ? (ringWidth - w) * (float) i / (count - 1.0f) : 0.0f);
            g.setColour (ring.colour.withAlpha (enabled ? 0.95f : 0.4f));
            g.strokePath (arc (c, r, angleFor (value), angleFor (target)), juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
        }

        if (live >= 0.0f)
        {
            const auto a = angleFor (live);
            const auto p = c.getPointOnCircumference (ringRadius, a);
            g.setColour (rings.front().colour.brighter (0.4f));
            g.fillEllipse (juce::Rectangle<float> (ringWidth + 2.5f, ringWidth + 2.5f).withCentre (p));
        }
    }

    // The body: a raised disc lit from above, its edge picked out.
    const auto body = juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (c);
    {
        const auto top = light ? Theme::raised.brighter (0.25f) : Theme::hover.brighter (0.12f);
        const auto bottom = light ? Theme::raised.darker (0.12f) : Theme::raised.darker (0.35f);
        g.setColour (juce::Colours::black.withAlpha (light ? 0.12f : 0.45f));
        g.fillEllipse (body.translated (0.0f, 1.5f).expanded (0.5f));
        g.setGradientFill (juce::ColourGradient (top, c.x, body.getY(), bottom, c.x, body.getBottom(), false));
        g.fillEllipse (body);
        g.setColour (light ? juce::Colours::white.withAlpha (0.7f) : juce::Colours::white.withAlpha (0.08f));
        g.drawEllipse (body.reduced (0.75f), 1.0f);
        g.setColour (dropHover ? colour : Theme::lineStrong.withAlpha (0.6f));
        g.drawEllipse (body, dropHover ? 2.0f : 0.75f);
    }

    // The pointer: a line from near the centre to the body's edge.
    {
        const auto a = angleFor (value);
        const auto inner = c.getPointOnCircumference (bodyRadius * 0.35f, a);
        const auto tip = c.getPointOnCircumference (bodyRadius - 2.0f, a);
        g.setColour (enabled ? Theme::textStrong.withAlpha (0.9f) : Theme::faintText);
        g.drawLine ({ inner, tip }, size == Size::small ? 1.6f : 2.0f);
    }

    // A dropped source would land here: a soft halo while one hovers.
    if (dropHover)
    {
        g.setColour (colour.withAlpha (0.18f));
        g.fillEllipse (dial.expanded (2.0f));
    }

    // The name, or the value while it is turned or hovered.
    auto text = getLocalBounds().toFloat().removeFromBottom (size == Size::small ? 13.0f : 15.0f);
    const auto showValue = isShowingValue();
    g.setColour (showValue ? Theme::accentLight : (enabled ? Theme::dimText : Theme::faintText));
    g.setFont (showValue ? boldFont (size == Size::small ? 10.0f : 11.0f) : labelFont (size == Size::small ? 10.0f : 11.0f));
    g.drawFittedText (showValue ? getValueText() : label, text.toNearestInt(), juce::Justification::centred, 1, 0.8f);
}

void Knob::mouseEnter (const juce::MouseEvent&)
{
    hovered = true;
    repaint();
}

void Knob::mouseExit (const juce::MouseEvent&)
{
    hovered = false;
    repaint();
}

void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (! isEnabled())
        return;

    if (e.mods.isPopupMenu())
    {
        showMenu();
        return;
    }

    if (e.mods.isAltDown())
    {
        resetToDefault();
        return;
    }

    ringDrag = onRingBand (e.position) && onRingDepthChanged != nullptr;

    if (ringDrag)
    {
        ringStart = rings.front().depth;
        return;
    }

    dragging = true;
    dragStart = parameter.getNormalised();
    parameter.begin();
    repaint();
}

void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (! isEnabled())
        return;

    // Up and right both turn it up, so a knob in a row can be dragged either way.
    const auto pixels = (float) (e.getDistanceFromDragStartX() - e.getDistanceFromDragStartY());

    if (ringDrag)
    {
        const auto depth = juce::jlimit (-1.0f, 1.0f, ringStart + pixels / 250.0f * (e.mods.isShiftDown() ? 0.1f : 1.0f));
        rings.front().depth = depth;
        onRingDepthChanged (rings.front().slot, depth);
        repaint();
        return;
    }

    if (! dragging)
        return;

    // Shift turns fine from where it is, so pressing it mid-drag doesn't jump.
    static constexpr float range = 250.0f;
    setNormalised (dragged (dragStart, pixels, e.mods.isShiftDown(), range));
}

void Knob::mouseUp (const juce::MouseEvent&)
{
    if (dragging)
        parameter.end();

    dragging = ringDrag = false;
    repaint();
}

void Knob::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (isEnabled() && ! e.mods.isPopupMenu())
        resetToDefault();
}

void Knob::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (! isEnabled())
        return;

    const auto delta = (std::abs (w.deltaY) > std::abs (w.deltaX) ? w.deltaY : -w.deltaX) * (w.isReversed ? -1.0f : 1.0f);
    parameter.begin();
    setNormalised (parameter.getNormalised() + delta * (e.mods.isShiftDown() ? 0.02f : 0.15f));
    parameter.end();
}

void Knob::showMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader (label + ": " + getValueText());
    menu.addItem (1, "Type a Value...");
    menu.addItem (2, "Reset to Default");

    for (size_t i = 0; i < rings.size(); ++i)
    {
        if (i == 0)
            menu.addSeparator();

        menu.addColouredItem (100 + (int) i, "Remove Modulation " + juce::String (rings[i].slot + 1)
                                                + " (" + formatValue (rings[i].depth * 100.0f, "+%") + ")",
                              rings[i].colour, onRingRemoved != nullptr);
    }

    juce::Component::SafePointer<Knob> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int chosen)
    {
        if (safe == nullptr || chosen == 0)
            return;

        if (chosen == 1)
            safe->typeValue();
        else if (chosen == 2)
            safe->resetToDefault();
        else if (chosen >= 100 && safe->onRingRemoved != nullptr)
            safe->onRingRemoved (safe->rings[(size_t) (chosen - 100)].slot);
    });
}

void Knob::typeValue()
{
    auto* window = new juce::AlertWindow (label, "Type a value" + (parameter.getUnit().isNotEmpty() ? " in " + parameter.getUnit() : juce::String()) + ".",
                                          juce::MessageBoxIconType::NoIcon, this);
    window->addTextEditor ("value", juce::String (parameter.getPlain(), 3));
    window->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Knob> safe (this);
    window->enterModalState (true, juce::ModalCallbackFunction::create ([safe, window] (int result)
    {
        if (safe == nullptr || result != 1)
            return;

        const auto typed = window->getTextEditorContents ("value").retainCharacters ("0123456789.-+eE");

        if (typed.isEmpty())
            return;

        auto plain = typed.getFloatValue();
        auto* p = safe->parameter.get();
        const auto norm = p != nullptr ? p->convertTo0to1 (p->getNormalisableRange().getRange().clipValue (plain)) : juce::jlimit (0.0f, 1.0f, plain);
        safe->parameter.begin();
        safe->setNormalised (norm);
        safe->parameter.end();
    }), true);
}

bool Knob::isInterestedInDragSource (const SourceDetails& d)
{
    return onModulationDropped != nullptr && sourceFromDragText (d.description) >= 0;
}

void Knob::itemDragEnter (const SourceDetails&)
{
    dropHover = true;
    repaint();
}

void Knob::itemDragExit (const SourceDetails&)
{
    dropHover = false;
    repaint();
}

void Knob::itemDropped (const SourceDetails& d)
{
    dropHover = false;
    repaint();

    if (const auto source = sourceFromDragText (d.description); source >= 0 && onModulationDropped != nullptr)
        onModulationDropped (source);
}

//==============================================================================
LedToggle::LedToggle (const juce::String& t) : text (t)
{
    setTitle (t);
}

void LedToggle::attach (juce::RangedAudioParameter* p)
{
    parameter = ParameterLink (p);
    refresh();
}

void LedToggle::setOn (bool b, bool notify)
{
    if (parameter.get() != nullptr && notify)
    {
        parameter.begin();
        parameter.set (b ? 1.0f : 0.0f);
        parameter.end();
    }

    if (b != on)
    {
        on = b;
        repaint();
    }

    if (notify && onToggle != nullptr)
        onToggle (on);
}

void LedToggle::refresh()
{
    if (parameter.get() != nullptr)
        if (const auto now = parameter.getNormalised() >= 0.5f; now != on)
        {
            on = now;
            repaint();
        }
}

void LedToggle::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    const auto colour = accentOr (tint);
    const auto radius = juce::jmin (r.getHeight() * 0.5f, 7.0f);
    g.setColour (on ? colour.withAlpha (0.16f) : (hovered ? Theme::hover : Theme::raised));
    g.fillRoundedRectangle (r, radius);
    g.setColour (on ? colour.withAlpha (0.6f) : Theme::line);
    g.drawRoundedRectangle (r, radius, 1.0f);

    // The LED: lit with a glow, or a dark lens.
    const auto d = juce::jmin (7.0f, r.getHeight() - 8.0f);
    const auto led = juce::Rectangle<float> (d, d).withCentre ({ r.getX() + radius + 2.0f, r.getCentreY() });

    if (on)
    {
        g.setColour (colour.withAlpha (0.35f));
        g.fillEllipse (led.expanded (2.5f));
        g.setColour (colour);
        g.fillEllipse (led);
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.fillEllipse (led.reduced (d * 0.3f).translated (-d * 0.12f, -d * 0.12f));
    }
    else
    {
        g.setColour (Theme::inset);
        g.fillEllipse (led);
        g.setColour (Theme::lineStrong.withAlpha (0.7f));
        g.drawEllipse (led, 0.75f);
    }

    if (text.isNotEmpty())
    {
        g.setColour (on ? Theme::textStrong : Theme::dimText);
        g.setFont (boldFont (10.5f));
        g.drawFittedText (text, r.withTrimmedLeft (led.getRight() - r.getX() + 4.0f).withTrimmedRight (6.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
    }
}

void LedToggle::mouseDown (const juce::MouseEvent&)
{
    if (isEnabled())
        setOn (! on);
}

//==============================================================================
StepSelector::StepSelector()
{
    setRepaintsOnMouseActivity (true);
}

void StepSelector::attach (juce::AudioParameterChoice* p)
{
    parameter = p;

    if (p != nullptr)
        choices = p->choices;

    refresh();
}

void StepSelector::setChoices (const juce::StringArray& c)
{
    choices = c;
    index = juce::jlimit (0, juce::jmax (0, choices.size() - 1), index);
    repaint();
}

void StepSelector::setIndex (int i, bool notify)
{
    if (choices.isEmpty())
        return;

    i = juce::jlimit (0, choices.size() - 1, i);

    if (parameter != nullptr && notify)
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 ((float) i));
        parameter->endChangeGesture();
    }

    if (i != index)
    {
        index = i;
        repaint();
    }

    if (notify && onChange != nullptr)
        onChange (index);
}

void StepSelector::step (int by)
{
    if (choices.isEmpty())
        return;

    const auto n = choices.size();
    setIndex (((index + by) % n + n) % n);
}

void StepSelector::refresh()
{
    if (parameter != nullptr && parameter->getIndex() != index)
    {
        index = parameter->getIndex();
        repaint();
    }
}

int StepSelector::partAt (juce::Point<float> p) const
{
    const auto arrow = juce::jmin (16.0f, (float) getHeight());

    if (p.x < arrow)
        return -1;

    if (p.x > (float) getWidth() - arrow)
        return 1;

    return 0;
}

void StepSelector::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    const auto colour = accentOr (tint);
    g.setColour (Theme::inset);
    g.fillRoundedRectangle (r, Theme::radius);
    g.setColour (hoverPart == 0 && isMouseOver() ? colour.withAlpha (0.6f) : Theme::line);
    g.drawRoundedRectangle (r, Theme::radius, 1.0f);

    const auto arrow = juce::jmin (16.0f, r.getHeight());
    const auto drawArrow = [&] (juce::Rectangle<float> a, bool right, bool hot)
    {
        juce::Path p;
        const auto s = juce::jmin (a.getWidth(), a.getHeight()) * 0.22f;
        const auto cc = a.getCentre();
        if (right) p.addTriangle (cc.x - s * 0.6f, cc.y - s, cc.x - s * 0.6f, cc.y + s, cc.x + s * 0.8f, cc.y);
        else       p.addTriangle (cc.x + s * 0.6f, cc.y - s, cc.x + s * 0.6f, cc.y + s, cc.x - s * 0.8f, cc.y);
        g.setColour (hot ? colour : Theme::dimText);
        g.fillPath (p);
    };

    drawArrow (r.removeFromLeft (arrow), false, hoverPart == -1);
    drawArrow (r.removeFromRight (arrow), true, hoverPart == 1);
    g.setColour (isEnabled() ? Theme::textStrong : Theme::faintText);
    g.setFont (boldFont (11.0f));
    g.drawFittedText (choices.isEmpty() ? juce::String() : choices[index], r.toNearestInt(), juce::Justification::centred, 1, 0.75f);
}

void StepSelector::mouseMove (const juce::MouseEvent& e)
{
    if (const auto part = partAt (e.position); part != hoverPart)
    {
        hoverPart = part;
        repaint();
    }
}

void StepSelector::mouseDown (const juce::MouseEvent& e)
{
    if (! isEnabled())
        return;

    if (const auto part = partAt (e.position); part != 0 && ! e.mods.isPopupMenu())
        step (part);
    else
        showMenu();
}

void StepSelector::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    wheelAccumulator += w.deltaY * (w.isReversed ? -1.0f : 1.0f);

    if (std::abs (wheelAccumulator) >= 0.1f)
    {
        step (wheelAccumulator > 0.0f ? -1 : 1);
        wheelAccumulator = 0.0f;
    }
}

void StepSelector::showMenu()
{
    juce::PopupMenu menu;
    size_t nextSection = 0;

    for (int i = 0; i < choices.size(); ++i)
    {
        while (nextSection < sections.size() && sections[nextSection].first == i)
            menu.addSectionHeader (sections[nextSection++].second);

        menu.addItem (i + 1, choices[i], true, i == index);
    }

    juce::Component::SafePointer<StepSelector> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()), [safe] (int chosen)
    {
        if (safe != nullptr && chosen > 0)
            safe->setIndex (chosen - 1);
    });
}

//==============================================================================
void ValueReadout::setText (const juce::String& t)
{
    if (t != text)
    {
        text = t;
        repaint();
    }
}

void ValueReadout::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (Theme::inset);
    g.fillRoundedRectangle (r, Theme::radius);
    g.setColour (Theme::line);
    g.drawRoundedRectangle (r, Theme::radius, 1.0f);
    g.setColour (Theme::accentLight);
    g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.5f, juce::Font::bold));
    g.drawFittedText (text, r.reduced (4.0f, 0.0f).toNearestInt(), juce::Justification::centred, 1, 0.75f);
}

//==============================================================================
ModSourceChip::ModSourceChip (int sourceIndex, const juce::String& t)
    : source (sourceIndex), text (t)
{
    setTooltip ("Drag " + t + " onto a knob to modulate it");
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
}

void ModSourceChip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    const auto colour = sourceColour (source);
    g.setColour (colour.withAlpha (hovered || highlighted ? 0.3f : 0.16f));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (colour.withAlpha (hovered || highlighted ? 1.0f : 0.7f));
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);

    // A cross of arrows: this can be dragged.
    const auto icon = r.removeFromLeft (r.getHeight()).reduced (r.getHeight() * 0.22f);
    const auto cc = icon.getCentre();
    const auto s = icon.getWidth() * 0.5f;
    juce::Path p;
    p.startNewSubPath (cc.x - s, cc.y); p.lineTo (cc.x + s, cc.y);
    p.startNewSubPath (cc.x, cc.y - s); p.lineTo (cc.x, cc.y + s);
    const auto head = s * 0.35f;
    for (auto [dx, dy] : { std::pair { 1.0f, 0.0f }, { -1.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, -1.0f } })
    {
        const juce::Point<float> tip { cc.x + dx * s, cc.y + dy * s };
        p.startNewSubPath (tip.x - dx * head - dy * head, tip.y - dy * head - dx * head);
        p.lineTo (tip);
        p.lineTo (tip.x - dx * head + dy * head, tip.y - dy * head + dx * head);
    }
    g.setColour (colour);
    g.strokePath (p, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (Theme::textStrong);
    g.setFont (boldFont (10.5f));
    g.drawFittedText (text, r.withTrimmedRight (6.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
}

void ModSourceChip::mouseDrag (const juce::MouseEvent& e)
{
    if (e.getDistanceFromDragStart() < 3)
        return;

    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this); container != nullptr && ! container->isDragAndDropActive())
    {
        // The drag image: the chip itself, a little see-through.
        auto image = createComponentSnapshot (getLocalBounds(), true, 2.0f);
        image.multiplyAllAlphas (0.85f);
        container->startDragging (modDragText (source), this, juce::ScaledImage (image, 2.0), true);
    }
}

//==============================================================================
Panel::Panel (const juce::String& t) : title (t)
{
    setInterceptsMouseClicks (false, true);
}

juce::Rectangle<int> Panel::getHeaderBounds() const
{
    auto r = getLocalBounds().removeFromTop (titleHeight).reduced (8, 3);
    const auto titleWidth = juce::GlyphArrangement::getStringWidthInt (boldFont (10.5f), title.toUpperCase()) + 22;
    return r.withTrimmedLeft (titleWidth);
}

juce::Rectangle<int> Panel::getContentBounds() const
{
    return getLocalBounds().withTrimmedTop (titleHeight).reduced (8, 6);
}

void Panel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    const auto light = Theme::current().light;
    constexpr auto corner = 7.0f;

    // A card raised off the window: a soft shadow, a gentle top-lit fill and a hairline edge.
    g.setColour (juce::Colours::black.withAlpha (light ? 0.06f : 0.3f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.5f), corner);
    g.setGradientFill (juce::ColourGradient (Theme::surface.brighter (light ? 0.0f : 0.04f), 0.0f, r.getY(),
                                             Theme::surface.darker (light ? 0.03f : 0.12f), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, corner);
    g.setColour (Theme::line);
    g.drawRoundedRectangle (r, corner, 1.0f);

    // The title strip: a hairline under it, an accent tick and the title spaced out in capitals.
    auto strip = r.removeFromTop ((float) titleHeight);
    g.setColour (Theme::line.withAlpha (0.6f));
    g.fillRect (strip.getX() + 8.0f, strip.getBottom() - 1.0f, strip.getWidth() - 16.0f, 1.0f);
    const auto colour = accentOr (accentColour).withMultipliedAlpha (dimmed ? 0.4f : 1.0f);
    g.setColour (colour);
    g.fillRoundedRectangle (strip.getX() + 9.0f, strip.getCentreY() - 5.0f, 3.0f, 10.0f, 1.5f);
    g.setColour (dimmed ? Theme::faintText : Theme::text);
    juce::GlyphArrangement glyphs;
    auto font = boldFont (10.5f).withExtraKerningFactor (0.12f);
    glyphs.addLineOfText (font, title.toUpperCase(), strip.getX() + 17.0f, strip.getCentreY() + font.getAscent() * 0.5f - 1.0f);
    glyphs.draw (g);
}

//==============================================================================
void drawDisplayWell (juce::Graphics& g, juce::Rectangle<float> r, int columns, int rows)
{
    const auto light = Theme::current().light;
    g.setColour (Theme::inset);
    g.fillRoundedRectangle (r, 5.0f);

    g.setColour (Theme::overlay.withAlpha (light ? 0.06f : 0.045f));

    for (int i = 1; i < columns; ++i)
        g.fillRect (r.getX() + r.getWidth() * (float) i / (float) columns, r.getY() + 2.0f, 1.0f, r.getHeight() - 4.0f);

    for (int i = 1; i < rows; ++i)
        g.fillRect (r.getX() + 2.0f, r.getY() + r.getHeight() * (float) i / (float) rows, r.getWidth() - 4.0f, 1.0f);

    // An inner shadow along the top edge, so the well sits below the panel.
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (light ? 0.08f : 0.35f), 0.0f, r.getY(),
                                             juce::Colours::transparentBlack, 0.0f, r.getY() + 6.0f, false));
    g.fillRoundedRectangle (r.withHeight (6.0f), 5.0f);
    g.setColour (Theme::line.withAlpha (0.8f));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
}

void drawGlowingCurve (juce::Graphics& g, const juce::Path& curve, juce::Colour colour, juce::Rectangle<float> area, bool fill)
{
    if (fill)
    {
        auto under = curve;
        const auto bounds = curve.getBounds();
        under.lineTo (bounds.getRight(), area.getBottom());
        under.lineTo (bounds.getX(), area.getBottom());
        under.closeSubPath();
        g.setGradientFill (juce::ColourGradient (colour.withAlpha (0.28f), 0.0f, area.getY(), colour.withAlpha (0.02f), 0.0f, area.getBottom(), false));
        g.fillPath (under);
    }

    g.setColour (colour.withAlpha (0.22f));
    g.strokePath (curve, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (colour);
    g.strokePath (curve, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
}
