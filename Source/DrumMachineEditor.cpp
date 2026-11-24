#include "DrumMachineEditor.h"
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
    bool selected = false;
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
DrumMachineEditor::DrumMachineEditor (BuiltInEffects::BasicDrumsProcessor& p)
    : AudioProcessorEditor (p), drums (p),
      presets (EffectPresets::userStore(), p.getPresetKey(), [&p] { return p.getPresetValues(); },
               [&p] (const EffectPresets::Values& v) { p.setPresetValues (v); })
{
    setOpaque (true);

    for (auto* panel : { &padPanel, &soundPanel, &kitPanel, &busPanel, &spacePanel })
        addAndMakeVisible (*panel);

    for (auto s : padOrder())
    {
        pads.push_back (std::make_unique<Pad> (s, [this] (Sound sound, float proportionDown) { clickPad (sound, proportionDown); }));
        padPanel.addAndMakeVisible (*pads.back());
    }

    picture = std::make_unique<Picture>();
    soundPanel.addAndMakeVisible (*picture);

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

    filter.setChoices (DrumMachine::filterNames());
    filter.setTooltip ("A filter on this sound alone: off, low-pass, band-pass or high-pass");
    filter.onChange = [this] (int) { refreshFilterKnobs(); };
    soundPanel.addAndMakeVisible (filter);

    character.setChoices (DrumMachine::kickCharacterNames());
    character.attach (drums.kickCharacterParameter());
    character.setTooltip ("The kick's character: Boom (long and round), Punch (909-style), Hard (an overdriven industrial kick)");
    soundPanel.addChildComponent (character);

    type.setChoices (DrumMachine::typeNames());
    type.setTooltip ("The sound's type: Classic, as it has always been, or Modern, a second model with more to it, duller the softer it is struck");
    type.onChange = [this] (int) { refreshPicture (true); };
    soundPanel.addChildComponent (type);

    juce::StringArray kitNames;

    for (auto& kit : DrumMachine::kits())
        kitNames.add (kit.name);

    kits.setChoices (kitNames);
    kits.setIndex (juce::jmax (0, DrumMachine::kitIndex (drums.getKitName())), false);
    kits.onChange = [this] (int i)
    {
        drums.applyKit (DrumMachine::kits()[(size_t) i]);
        refreshPicture (true);
        repaint();
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

    // Its type, where it has two.
    if (auto* p = drums.typeParameter (selected))
    {
        type.attach (p);
        type.setColour (colour);
    }

    type.setVisible (drums.typeParameter (selected) != nullptr);
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
    const auto hit = DrumMachine::Engine::renderOne (now, selected, pictureRate, guess);
    picture->set (hit, pictureRate, now.sounds[(size_t) selected].decay, colourOf (selected));
    pictureSeconds = picture->getSeconds();
}

juce::String DrumMachineEditor::getKitText() const
{
    return drums.getKitName() + (drums.kitModified() ? " (edited)" : "");
}

juce::String DrumMachineEditor::getTypeText() const
{
    return type.isVisible() ? DrumMachine::typeNames()[type.getIndex()] : juce::String();
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

        auto c = soundPanel.getContentBounds();
        c.removeFromBottom (20);   // the line about notes and chokes, painted by the panel's owner below
        picture->setBounds (c.removeFromTop (170));
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

        const auto sw = c.getWidth() / 3;

        for (size_t i = 3; i < 6; ++i)
            place (i, c.removeFromLeft (sw).reduced (0, 4), 18);
    }
}

void DrumMachineEditor::paintOverChildren (juce::Graphics& g)
{
    // Under the knobs: the sound's notes and what chokes it.
    auto line = soundPanel.getBounds().reduced (16, 8).removeFromBottom (16);
    g.setColour (Theme::dimText);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("Notes " + notesOf (selected) + "    " + chokesOf (selected), line, juce::Justification::centredLeft);

    // A rule and a heading over its effects.
    g.setColour (Theme::dimText.withAlpha (0.35f));
    g.drawHorizontalLine (fxLine.getCentreY(), (float) fxLine.getX() + 64.0f, (float) fxLine.getRight());
    g.setColour (Theme::dimText);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("EFFECTS", fxLine.withWidth (60), juce::Justification::centredLeft);
}
