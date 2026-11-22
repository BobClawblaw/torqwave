#include "SwarmGuitarEditor.h"
#include "Theme.h"

using BuiltInEffects::SwarmGuitarProcessor;
using SoundControls::Knob;

namespace
{
    float plain (const SwarmGuitarProcessor& g, const char* id)
    {
        if (auto* p = g.parameterFor (id))
            return p->convertFrom0to1 (p->getValue());

        return 0.0f;
    }

    juce::String noteName (int note)
    {
        return juce::MidiMessage::getMidiNoteName (juce::jlimit (0, 127, note), true, true, 4);
    }

    juce::String place (int semitones)
    {
        switch (semitones)
        {
            case 0:     return "note";
            case 12:    return "oct up";
            case -12:   return "oct down";
            case 7:     return "fifth";
            case -5:    return "fifth down";
            default:    return (semitones > 0 ? "+" : "") + juce::String (semitones) + " st";
        }
    }
}

//==============================================================================
/** The guitar: its strings from the lowest at the bottom, the nut on the left and the bridge on the right. */
class SwarmGuitarEditor::Strings final : public juce::Component
{
public:
    explicit Strings (SwarmGuitarProcessor& g) : guitar (g) {}

    int shown() const       { return juce::jlimit (1, SwarmGuitarProcessor::maxStrings, juce::roundToInt (plain (guitar, "strings"))); }

    juce::String label (int string) const
    {
        const auto offset = SwarmGuitarProcessor::octaveOf (guitar.tuningParameter()->getIndex(), string, shown());
        const auto note = guitar.getLastNote();
        return note >= 0 ? noteName (note + offset) : place (offset);
    }

    void fix (float amount)
    {
        fixed = amount;
        repaint();
    }

    void advance()
    {
        auto moving = false;

        for (int i = 0; i < SwarmGuitarProcessor::maxStrings; ++i)
        {
            // How hard it rings, on a scale the eye reads: quick up, slow down.
            const auto now = fixed >= 0.0f ? fixed : juce::jlimit (0.0f, 1.0f, std::sqrt (guitar.getStringLevel (i) * 1.5f));
            auto& shownLevel = levels[(size_t) i];
            shownLevel = juce::jmax (now, shownLevel * 0.9f);
            moving = moving || shownLevel > 0.004f;
            phases[(size_t) i] += 0.55f + 0.09f * (float) i;
        }

        const auto state = juce::String (shown()) + label (0) + label (shown() - 1) + juce::String (plain (guitar, "pick"), 1) + juce::String (plain (guitar, "detune"), 1);

        if (moving || state != last)
        {
            last = state;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const auto all = getLocalBounds().toFloat();
        SoundControls::drawDisplayWell (g, all, 0, 0);

        const auto n = shown();
        auto area = all.reduced (10.0f, 8.0f);
        const auto names = area.removeFromLeft (64.0f), cents = area.removeFromRight (50.0f);
        area.reduce (8.0f, 0.0f);
        const auto gap = area.getHeight() / (float) n;
        const auto tuningIndex = guitar.tuningParameter()->getIndex();
        const auto amount = plain (guitar, "detune");

        // The nut and the bridge, and where the pick is, measured from the bridge.
        g.setColour (Theme::lineStrong);
        g.fillRoundedRectangle (area.getX() - 3.0f, area.getY() + 2.0f, 3.0f, area.getHeight() - 4.0f, 1.5f);
        g.fillRoundedRectangle (area.getRight(), area.getY() + 2.0f, 3.0f, area.getHeight() - 4.0f, 1.5f);

        const auto pickX = area.getRight() - area.getWidth() * plain (guitar, "pick") * 0.01f;
        g.setColour (Theme::accentLight.withAlpha (0.55f));
        const float dashes[] = { 3.0f, 3.0f };
        g.drawDashedLine ({ pickX, area.getY(), pickX, area.getBottom() }, dashes, 2, 1.0f);
        juce::Path tip;
        tip.addTriangle (pickX - 5.0f, area.getY() - 5.0f, pickX + 5.0f, area.getY() - 5.0f, pickX, area.getY() + 3.0f);
        g.fillPath (tip);

        for (int i = 0; i < n; ++i)
        {
            // The lowest string at the bottom, as a guitar lies in a tab.
            const auto y = area.getBottom() - gap * ((float) i + 0.5f);
            const auto offset = SwarmGuitarProcessor::octaveOf (tuningIndex, i, n);
            const auto swing01 = levels[(size_t) i];
            const auto reach = swing01 * juce::jmin (gap * 0.46f, 15.0f);
            const auto thickness = offset < 0 ? 2.2f : offset > 0 ? 1.0f : 1.5f;
            const auto colour = offset < 0 ? Theme::accent : offset > 0 ? Theme::accentLight : Theme::accent.interpolatedWith (Theme::accentLight, 0.5f);

            // The room it swings in, then where it is now.
            juce::Path swing, line;
            constexpr int points = 48;

            for (int p = 0; p <= points; ++p)
            {
                const auto t = (float) p / (float) points;
                const auto x = area.getX() + area.getWidth() * t;
                const auto shape = std::sin (juce::MathConstants<float>::pi * t);
                const auto second = 0.35f * std::sin (juce::MathConstants<float>::twoPi * t);
                const auto now = reach * (shape * std::cos (phases[(size_t) i]) + second * std::cos (phases[(size_t) i] * 1.73f));

                if (p == 0)
                {
                    swing.startNewSubPath (x, y);
                    line.startNewSubPath (x, y);
                }
                else
                {
                    swing.lineTo (x, y - reach * shape);
                    line.lineTo (x, y + now);
                }
            }

            for (int p = points; p >= 0; --p)
            {
                const auto t = (float) p / (float) points;
                swing.lineTo (area.getX() + area.getWidth() * t, y + reach * std::sin (juce::MathConstants<float>::pi * t));
            }

            swing.closeSubPath();

            if (reach > 0.3f)
            {
                g.setColour (colour.withAlpha (0.16f));
                g.fillPath (swing);
            }

            g.setColour (colour.withAlpha (0.25f + 0.2f * swing01));
            g.strokePath (line, juce::PathStrokeType (thickness + 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (colour.interpolatedWith (Theme::textStrong, 0.25f * swing01));
            g.strokePath (line, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            const auto row = juce::Rectangle<float> (0.0f, y - 8.0f, 10.0f, 16.0f);
            g.setFont (juce::FontOptions (juce::jmin (12.0f, gap * 0.8f), juce::Font::bold));
            g.setColour (swing01 > 0.05f ? Theme::textStrong : Theme::text);
            g.drawText (label (i), row.withX (names.getX()).withWidth (names.getWidth()), juce::Justification::centredLeft);

            g.setFont (juce::FontOptions (juce::jmin (11.0f, gap * 0.75f)));
            g.setColour (Theme::dimText);
            g.drawText (SoundControls::formatValue (SwarmGuitarProcessor::centsOf (0, i, amount), "ct"), row.withX (cents.getX()).withWidth (cents.getWidth()),
                        juce::Justification::centredRight);
        }
    }

private:
    SwarmGuitarProcessor& guitar;
    std::array<float, SwarmGuitarProcessor::maxStrings> levels {}, phases {};
    float fixed = -1.0f;
    juce::String last;
};

//==============================================================================
/** What the amp does to a signal: out against in, at the Drive it is set to. */
class SwarmGuitarEditor::AmpCurve final : public juce::Component
{
public:
    explicit AmpCurve (SwarmGuitarProcessor& g) : guitar (g) {}

    void advance()
    {
        const auto state = juce::String (guitar.ampParameter()->getIndex()) + juce::String (plain (guitar, "drive"), 1) + juce::String (plain (guitar, "tone"), 0);

        if (state != last)
        {
            last = state;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const auto all = getLocalBounds().toFloat();
        SoundControls::drawDisplayWell (g, all, 4, 2);
        const auto area = all.reduced (8.0f, 8.0f);
        const auto kind = guitar.ampParameter()->getIndex();
        const auto d = plain (guitar, "drive") * 0.01f;

        // The same stages the instrument runs, without their filters; in on a scale that shows quiet signals too.
        const auto first = kind == 0 ? juce::Decibels::decibelsToGain (12.0f + 42.0f * d) : juce::Decibels::decibelsToGain (30.0f * d);
        const auto second = 2.0f + 4.0f * d;

        const auto shaped = [&] (float x)
        {
            if (kind == 2)
                return x;

            const auto bias = kind == 0 ? 0.35f : 0.1f;
            auto y = std::tanh (x * first + bias) - std::tanh (bias);

            if (kind == 0)
                y = std::tanh (y * second);

            return y;
        };

        juce::Path path;
        constexpr int points = 120;

        for (int p = 0; p <= points; ++p)
        {
            const auto t = 2.0f * (float) p / (float) points - 1.0f;
            const auto x = (t < 0.0f ? -1.0f : 1.0f) * std::pow (std::abs (t), 2.5f);   // quiet signals in the middle, spread out
            const auto point = juce::Point<float> (area.getCentreX() + t * area.getWidth() * 0.5f, area.getCentreY() - shaped (x) * area.getHeight() * 0.46f);

            if (p == 0)
                path.startNewSubPath (point);
            else
                path.lineTo (point);
        }

        SoundControls::drawGlowingCurve (g, path, Theme::accent, area, false);
    }

private:
    SwarmGuitarProcessor& guitar;
    juce::String last;
};

//==============================================================================
/** Where the guitars sit, left to right. */
class SwarmGuitarEditor::Field final : public juce::Component
{
public:
    explicit Field (SwarmGuitarProcessor& g) : guitar (g) {}

    int shown() const       { return juce::jlimit (1, SwarmGuitarProcessor::maxLayers, juce::roundToInt (plain (guitar, "layers"))); }

    void advance()
    {
        const auto state = juce::String (shown()) + juce::String (plain (guitar, "width"), 0);

        if (state != last)
        {
            last = state;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const auto all = getLocalBounds().toFloat();
        SoundControls::drawDisplayWell (g, all, 0, 0);
        const auto area = all.reduced (26.0f, 6.0f);
        const auto n = shown();
        const auto widthNow = plain (guitar, "width") * 0.01f;

        g.setColour (Theme::dimText);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText ("L", all.withWidth (22.0f), juce::Justification::centred);
        g.drawText ("R", all.withLeft (all.getRight() - 22.0f), juce::Justification::centred);
        g.setColour (Theme::line);
        g.fillRect (area.getX(), area.getCentreY() - 0.5f, area.getWidth(), 1.0f);
        g.fillRect (area.getCentreX() - 0.5f, area.getY() + 4.0f, 1.0f, area.getHeight() - 8.0f);

        for (int i = 0; i < n; ++i)
        {
            const auto where = n == 1 ? 0.0f : (2.0f * (float) i / (float) (n - 1) - 1.0f) * widthNow;
            const auto x = area.getCentreX() + where * area.getWidth() * 0.5f;
            g.setColour (Theme::accent.withAlpha (0.25f));
            g.fillEllipse (x - 9.0f, area.getCentreY() - 9.0f, 18.0f, 18.0f);
            g.setColour (Theme::accentLight);
            g.fillEllipse (x - 4.5f, area.getCentreY() - 4.5f, 9.0f, 9.0f);
        }
    }

private:
    SwarmGuitarProcessor& guitar;
    juce::String last;
};

//==============================================================================
SwarmGuitarEditor::SwarmGuitarEditor (SwarmGuitarProcessor& p)
    : AudioProcessorEditor (p), guitar (p),
      presets (EffectPresets::userStore(), p.getPresetKey(), [&p] { return p.getPresetValues(); },
               [&p] (const EffectPresets::Values& v) { p.setPresetValues (v); })
{
    setOpaque (true);

    for (auto* panel : { &stringsPanel, &ampPanel, &strumPanel, &layersPanel, &swarmPanel, &envelopePanel })
        addAndMakeVisible (*panel);

    strings = std::make_unique<Strings> (guitar);
    curve = std::make_unique<AmpCurve> (guitar);
    field = std::make_unique<Field> (guitar);
    stringsPanel.addAndMakeVisible (*strings);
    ampPanel.addAndMakeVisible (*curve);
    layersPanel.addAndMakeVisible (*field);

    const auto attach = [this] (Knob& k, SoundControls::Panel& panel, const char* id, const juce::String& tip)
    {
        k.attach (guitar.parameterFor (id));
        k.setTooltip (tip);
        panel.addAndMakeVisible (k);
    };

    attach (count, stringsPanel, "strings", "How many strings each guitar has");
    attach (detune, stringsPanel, "detune", "How far apart the strings sit: more is a thicker, slower-turning swarm");
    attach (pick, stringsPanel, "pick", "Where along the string it is picked, from the bridge: near it thin and bright, the middle hollow");
    attach (brightness, stringsPanel, "brightness", "How bright the strings are and how hard the pick sounds");
    attach (drive, ampPanel, "drive", "How hard the amp is driven");
    attach (tone, ampPanel, "tone", "The cabinet's top end");
    attach (level, ampPanel, "level", "The output level");
    attach (strum, strumPanel, "strum", "The time between one string and the next in a strum");
    attach (strumRate, strumPanel, "strumRate", "Strums again and again while the key is down, down and up strokes in turn; at 0, once");
    attach (sustain, strumPanel, "sustain", "How long a held note rings");
    attach (release, strumPanel, "release", "How quickly a note is damped when it is let go");
    attach (layers, layersPanel, "layers", "How many guitars play at once, each tuned and timed a little apart, through its own amp");
    attach (width, layersPanel, "width", "How far the guitars are spread left and right");
    attach (spread, swarmPanel, "spread", "How far apart the swarm's oscillators are fanned, evenly, around the note: a few cents beat, an octave is a cloud");
    attach (converge, swarmPanel, "converge", "The swarm closing to one pitch over this time; below 0, opening from one pitch to the full Spread");
    attach (glide, swarmPanel, "glide", "Each oscillator sliding from the last note to the new one, each at its own speed, so the swarm smears");
    attach (drift, swarmPanel, "drift", "Each oscillator wandering on its own slow course, up to 30 cents either way");
    attach (attack, envelopePanel, "attack", "How long a note takes to swell in");
    attach (decay, envelopePanel, "decay", "How long it takes to fall to the sustain level, and the filter to close");
    attach (hold, envelopePanel, "sustainLevel", "The level a held note stays at");
    attach (cutoff, envelopePanel, "cutoff", "Where the swarm's low-pass sits (it follows the note by half)");
    attach (resonance, envelopePanel, "resonance", "How much the low-pass rings at its cutoff");
    attach (filterEnv, envelopePanel, "filterEnv", "How far the envelope opens the low-pass on each note, and on each Strum Rate pulse");

    const auto whole = [] (float v) { return juce::String (juce::roundToInt (v)); };
    count.setFormatter (whole);
    layers.setFormatter (whole);
    strumRate.setFormatter ([] (float v) { return v < 0.05f ? juce::String ("Once") : juce::String (v, 1) + " /s"; });

    tuning.attach (guitar.tuningParameter());
    tuning.setTooltip ("Unison: every string on the note. Low Octave: the lowest third an octave down. Three Octaves: the top string an octave up as well. Fifths: root, fifth and octave");
    stringsPanel.addAndMakeVisible (tuning);

    source.attach (guitar.sourceParameter());
    source.setTooltip ("Strings: modelled strings, plucked. Swarm: a sawtooth oscillator for each string, fanned across Spread, through its own envelope and filter");
    stringsPanel.addAndMakeVisible (source);

    sync.attach (guitar.strumSyncParameter());
    sync.setTooltip ("Off: Strum Rate's times a second. Otherwise the repeats in note values at the session's tempo, from each note's start");
    strumPanel.addAndMakeVisible (sync);

    amp.attach (guitar.ampParameter());
    amp.setTooltip ("Fuzz (two stages), Crunch (one), or None: the strings alone, for your own amp after it");
    ampPanel.addAndMakeVisible (amp);

    addAndMakeVisible (presets);
    showSource (guitar.isSwarm());

    setSize (windowWidth, windowHeight);
    startTimerHz (30);
}

SwarmGuitarEditor::~SwarmGuitarEditor()
{
    stopTimer();
}

Knob* SwarmGuitarEditor::knob (const juce::String& name)
{
    for (auto* k : allKnobs())
        if (k->getLabel() == name)
            return k;

    return nullptr;
}

std::vector<Knob*> SwarmGuitarEditor::allKnobs()
{
    return { &count, &detune, &pick, &brightness, &drive, &tone, &level, &strum, &strumRate, &sustain, &release, &layers, &width,
             &spread, &converge, &glide, &drift, &attack, &decay, &hold, &cutoff, &resonance, &filterEnv };
}

void SwarmGuitarEditor::showSource (bool swarm)
{
    // What each plays: the swarm's knobs for the swarm, and the strings' own (the pick, how long they ring) for the strings.
    swarmShown = swarm;
    stringsPanel.setTitle (swarm ? "Oscillators" : "Strings");
    count.setLabel (swarm ? "Oscillators" : "Strings");
    count.setTooltip (swarm ? "How many oscillators each guitar's swarm has" : "How many strings each guitar has");
    strum.setTooltip (swarm ? "The time between one oscillator's entry and the next" : "The time between one string and the next in a strum");
    strumRate.setTooltip (swarm ? "Fires the envelope and filter again and again while the key is down; at 0, once"
                                : "Strums again and again while the key is down, down and up strokes in turn; at 0, once");

    for (auto* k : { &pick, &sustain })
    {
        k->setEnabled (! swarm);
        k->setAlpha (swarm ? 0.35f : 1.0f);
    }

    for (auto* panel : { &swarmPanel, &envelopePanel })
    {
        panel->setDimmed (! swarm);
        panel->setEnabled (swarm);
        panel->setAlpha (swarm ? 1.0f : 0.45f);
    }
}

int SwarmGuitarEditor::getStringsShown() const                        { return strings->shown(); }
juce::String SwarmGuitarEditor::getStringLabel (int string) const     { return strings->label (string); }
int SwarmGuitarEditor::getGuitarsShown() const                        { return field->shown(); }
void SwarmGuitarEditor::showRinging (float amount)                    { strings->fix (amount); strings->advance(); }

void SwarmGuitarEditor::timerCallback()
{
    for (auto* k : allKnobs())
        k->refresh();

    if (guitar.isSwarm() != swarmShown)
        showSource (guitar.isSwarm());

    source.refresh();
    sync.refresh();
    const auto synced = guitar.strumSyncParameter()->getIndex() > 0;
    strumRate.setAlpha (synced ? 0.35f : 1.0f);
    strumRate.setEnabled (! synced);
    tuning.refresh();
    amp.refresh();
    strings->advance();
    curve->advance();
    field->advance();
}

void SwarmGuitarEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::base);

    auto top = getLocalBounds().reduced (12).removeFromTop (30);
    g.setColour (Theme::accent);
    g.fillRoundedRectangle ((float) top.getX(), (float) top.getY() + 7.0f, 3.0f, 16.0f, 1.5f);
    g.setColour (Theme::textStrong);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText ("SWARM GUITAR", top.withTrimmedLeft (10), juce::Justification::centredLeft);
    g.setColour (Theme::dimText);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("every string on one note", top.withTrimmedLeft (140), juce::Justification::centredLeft);
}

void SwarmGuitarEditor::resized()
{
    auto r = getLocalBounds().reduced (12);

    auto top = r.removeFromTop (30);
    presets.setBounds (top.removeFromRight (360));
    r.removeFromTop (8);

    auto upper = r.removeFromTop (300);
    r.removeFromTop (10);

    // The guitar: its strings, then their knobs with the tuning between them.
    stringsPanel.setBounds (upper.removeFromLeft (580));
    upper.removeFromLeft (10);
    {
        source.setBounds (stringsPanel.getHeaderBounds().removeFromRight (140).reduced (0, 1));
        auto c = stringsPanel.getContentBounds();
        strings->setBounds (c.removeFromTop (170));
        c.removeFromTop (6);
        const auto w = 84;
        count.setBounds (c.removeFromLeft (w));
        tuning.setBounds (c.removeFromLeft (190).withSizeKeepingCentre (176, 28));
        const auto each = c.getWidth() / 3;
        detune.setBounds (c.removeFromLeft (each));
        pick.setBounds (c.removeFromLeft (each));
        brightness.setBounds (c);
    }

    // The amp: which, its curve, and its knobs.
    ampPanel.setBounds (upper);
    {
        amp.setBounds (ampPanel.getHeaderBounds().removeFromRight (140).reduced (0, 1));
        auto c = ampPanel.getContentBounds();
        curve->setBounds (c.removeFromTop (110));
        c.removeFromTop (6);
        drive.setBounds (c.removeFromLeft (c.getWidth() * 42 / 100));
        tone.setBounds (c.removeFromLeft (c.getWidth() / 2).reduced (0, 10));
        level.setBounds (c.reduced (0, 10));
    }

    // The swarm, and its envelope and filter, along the bottom.
    auto lower = r.removeFromBottom (140);
    r.removeFromBottom (10);
    swarmPanel.setBounds (lower.removeFromLeft (340));
    lower.removeFromLeft (10);
    envelopePanel.setBounds (lower);
    {
        auto c = swarmPanel.getContentBounds().reduced (6, 2);
        spread.setBounds (c.removeFromLeft (c.getWidth() * 34 / 100));
        const auto each = c.getWidth() / 3;
        converge.setBounds (c.removeFromLeft (each).reduced (0, 8));
        glide.setBounds (c.removeFromLeft (each).reduced (0, 8));
        drift.setBounds (c.reduced (0, 8));
    }
    {
        auto c = envelopePanel.getContentBounds().reduced (6, 2);
        const auto each = c.getWidth() / 6;
        cutoff.setBounds (c.removeFromLeft (each + 14));
        for (auto* k : { &resonance, &filterEnv, &attack, &decay })
            k->setBounds (c.removeFromLeft (each - 3).reduced (0, 8));
        hold.setBounds (c.reduced (0, 8));
    }

    // The strum, and the guitars across the field.
    strumPanel.setBounds (r.removeFromLeft (580));
    r.removeFromLeft (10);
    {
        sync.setBounds (strumPanel.getHeaderBounds().removeFromRight (120).reduced (0, 1));
        auto c = strumPanel.getContentBounds().reduced (10, 4);
        const auto each = c.getWidth() / 4;

        for (auto* k : { &strum, &strumRate, &sustain })
            k->setBounds (c.removeFromLeft (each));

        release.setBounds (c);
    }

    layersPanel.setBounds (r);
    {
        auto c = layersPanel.getContentBounds();
        field->setBounds (c.removeFromTop (38));
        c.removeFromTop (4);
        layers.setBounds (c.removeFromLeft (c.getWidth() / 2));
        width.setBounds (c);
    }
}
