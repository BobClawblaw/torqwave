#include "CrossfadePanel.h"
#include "SessionMixer.h"
#include "Theme.h"

namespace
{
    int indexOf (FadeShape shape)
    {
        for (int i = 0; i < (int) FadeShapes::all.size(); ++i)
            if (FadeShapes::all[(size_t) i] == shape)
                return i;

        return 0;
    }

    FadeShape shapeAt (double index)
    {
        return FadeShapes::all[(size_t) juce::jlimit (0, (int) FadeShapes::all.size() - 1, juce::roundToInt (index))];
    }

    const SessionClip* clipIn (const SessionState& s, juce::uint32 id)
    {
        const auto at = s.findClip (id);
        return at.has_value() ? &s.tracks[(size_t) at->first].clips[(size_t) at->second] : nullptr;
    }
}

CrossfadePanel::CrossfadePanel (AudioEngine& e, std::vector<SessionEdits::Crossfade> chosen, EffectPresets::Store& store)
    : engine (e),
      before (e.session.getState()),
      openedAt (e.session.getPosition()),
      presets (store, presetKey, [this] { return currentValues(); }, [this] (const EffectPresets::Values& v) { applyValues (v); })
{
    for (const auto& x : chosen)
        pairs.emplace_back (x.out, x.in);

    for (auto* l : { &lengthLabel, &outLabel, &inLabel, &preLabel, &postLabel })
        addAndMakeVisible (*l);

    addAndMakeVisible (summary);
    summary.setColour (juce::Label::textColourId, Theme::dimText);

    length.setRange (0.1, 10000.0, 0.1);
    length.setSkewFactorFromMidPoint (100.0);
    length.setTextValueSuffix (" ms");
    length.onValueChange = [this] { if (! refreshing) setLengthMs (length.getValue()); };
    addAndMakeVisible (length);

    for (auto* roll : { &preRoll, &postRoll })
    {
        roll->setRange (0.0, 5000.0, 10.0);
        roll->setValue (1000.0, juce::dontSendNotification);
        roll->setTextValueSuffix (" ms");
        addAndMakeVisible (*roll);
    }

    for (auto* box : { &outShape, &inShape })
    {
        for (int i = 0; i < (int) FadeShapes::all.size(); ++i)
            box->addItem (FadeShapes::displayName (FadeShapes::all[(size_t) i]), i + 1);

        addAndMakeVisible (*box);
    }

    outShape.onChange = [this] { if (! refreshing) setOutShape (shapeAt (outShape.getSelectedId() - 1)); };
    inShape.onChange = [this] { if (! refreshing) setInShape (shapeAt (inShape.getSelectedId() - 1)); };

    symmetricButton.setTooltip ("Setting one side's shape sets the other's too");
    addAndMakeVisible (symmetricButton);
    equalGainButton.setTooltip ("S-curve both sides: steady level for a sound crossfading into more of itself");
    equalGainButton.onClick = [this] { setShapes (SessionEdits::equalGainShape, SessionEdits::equalGainShape); };
    equalPowerButton.setTooltip ("Equal power both sides: steady level for two different sounds");
    equalPowerButton.onClick = [this] { setShapes (SessionEdits::equalPowerShape, SessionEdits::equalPowerShape); };
    playOutButton.onClick = [this] { audition (Side::out); };
    playInButton.onClick = [this] { audition (Side::in); };
    playBothButton.onClick = [this] { audition (Side::both); };
    doneButton.onClick = [this] { done(); };
    cancelButton.onClick = [this] { cancel(); };

    for (auto* b : { &equalGainButton, &equalPowerButton, &playOutButton, &playInButton, &playBothButton, &doneButton, &cancelButton })
        addAndMakeVisible (*b);

    addAndMakeVisible (presets);
    engine.session.addChangeListener (this);
    refresh();
    setSize (width, height);
}

CrossfadePanel::~CrossfadePanel()
{
    engine.session.removeChangeListener (this);
}

std::vector<SessionEdits::Crossfade> CrossfadePanel::getCrossfades() const
{
    const auto state = engine.session.getState();
    std::vector<SessionEdits::Crossfade> found;

    for (auto [out, in] : pairs)
        if (const auto x = SessionEdits::crossfadeBetween (*state, out, in))
            found.push_back (*x);

    return found;
}

bool CrossfadePanel::edit (const std::function<bool (SessionState&, const SessionEdits::Crossfade&)>& change)
{
    auto next = *engine.session.getState();
    auto any = false;

    for (const auto& x : getCrossfades())
        any = change (next, x) || any;

    if (! any)
        return false;

    // The panel's changes are one step: each after the first takes the place of the one before.
    const auto merge = changed && engine.session.getPosition() == openedAt + 1 && engine.session.getUndoDescription() == "Edit Crossfade";
    engine.session.apply (std::move (next), "Edit Crossfade", merge);
    changed = true;
    refresh();
    return true;
}

bool CrossfadePanel::setLengthMs (double ms)
{
    const auto frames = (juce::int64) std::llround (ms * engine.session.getState()->sampleRate / 1000.0);
    return edit ([frames] (SessionState& n, const SessionEdits::Crossfade& x) { return SessionEdits::setCrossfadeLength (n, x.out, x.in, frames); });
}

bool CrossfadePanel::setShapes (FadeShape out, FadeShape in)
{
    return edit ([out, in] (SessionState& n, const SessionEdits::Crossfade& x) { return SessionEdits::setCrossfadeShapes (n, x.out, x.in, out, in); });
}

bool CrossfadePanel::setOutShape (FadeShape shape)
{
    return setShapes (shape, isSymmetric() ? shape : shapesNow().second);
}

bool CrossfadePanel::setInShape (FadeShape shape)
{
    return setShapes (isSymmetric() ? shape : shapesNow().first, shape);
}

void CrossfadePanel::setSymmetric (bool on)
{
    symmetricButton.setToggleState (on, juce::dontSendNotification);
}

std::pair<FadeShape, FadeShape> CrossfadePanel::shapesNow() const
{
    const auto state = engine.session.getState();
    const auto found = getCrossfades();

    if (found.empty())
        return { SessionEdits::equalGainShape, SessionEdits::equalGainShape };

    return { clipIn (*state, found.front().out)->fadeOutShape, clipIn (*state, found.front().in)->fadeInShape };
}

EffectPresets::Values CrossfadePanel::currentValues() const
{
    const auto found = getCrossfades();
    const auto [out, in] = shapesNow();
    const auto ms = found.empty() ? 10.0 : (double) found.front().length() * 1000.0 / engine.session.getState()->sampleRate;
    return { { "ms", ms }, { "out", (double) indexOf (out) }, { "in", (double) indexOf (in) } };
}

bool CrossfadePanel::applyValues (const EffectPresets::Values& v)
{
    const auto get = [&v] (const char* name, double otherwise) { const auto f = v.find (name); return f != v.end() ? f->second : otherwise; };
    const auto now = currentValues();
    const auto rate = engine.session.getState()->sampleRate;
    const auto frames = (juce::int64) std::llround (get ("ms", now.at ("ms")) * rate / 1000.0);
    const auto out = shapeAt (get ("out", now.at ("out"))), in = shapeAt (get ("in", now.at ("in")));

    return edit ([frames, out, in] (SessionState& n, const SessionEdits::Crossfade& x)
    {
        const auto longer = SessionEdits::setCrossfadeLength (n, x.out, x.in, frames);
        return SessionEdits::setCrossfadeShapes (n, x.out, x.in, out, in) || longer;
    });
}

void CrossfadePanel::done()
{
    engine.auditionFile ({});

    if (onClose != nullptr)
        onClose();
}

void CrossfadePanel::cancel()
{
    engine.auditionFile ({});

    if (changed && engine.session.getPosition() == openedAt + 1 && engine.session.getUndoDescription() == "Edit Crossfade")
        engine.session.undo();

    changed = false;

    if (onClose != nullptr)
        onClose();
}

juce::AudioBuffer<float> CrossfadePanel::renderAudition (Side side) const
{
    const auto found = getCrossfades();

    if (found.empty())
        return {};

    const auto& x = found.front();
    auto s = *engine.session.getState();

    // The two clips alone (or one of them) on their track, as it plays them, nothing else heard.
    for (int t = 0; t < (int) s.tracks.size(); ++t)
    {
        auto& track = s.tracks[(size_t) t];
        track.solo = false;

        if (t != x.track)
        {
            track.clips.clear();
            track.noteClips.clear();
            continue;
        }

        track.mute = false;
        auto& clips = track.clips;
        clips.erase (std::remove_if (clips.begin(), clips.end(), [&] (const SessionClip& c)
        {
            return ! ((c.id == x.out && side != Side::in) || (c.id == x.in && side != Side::out));
        }), clips.end());
    }

    const auto rate = s.sampleRate;
    const auto from = juce::jmax ((juce::int64) 0, x.start - (juce::int64) std::llround (preRoll.getValue() * rate / 1000.0));
    const auto to = x.end + (juce::int64) std::llround (postRoll.getValue() * rate / 1000.0);
    juce::AudioBuffer<float> out (2, (int) (to - from));
    SessionMixer::Buffers scratch;
    SessionMixer::render (s, from, out, 0, out.getNumSamples(), scratch);
    return out;
}

bool CrossfadePanel::audition (Side side)
{
    const auto sound = renderAudition (side);

    if (sound.getNumSamples() == 0)
        return false;

    // Played as a file is auditioned: written to memory as a WAV and read back.
    juce::MemoryBlock block;
    {
        std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::MemoryOutputStream> (block, false);
        juce::WavAudioFormat wav;
        auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (engine.session.getState()->sampleRate)
                                                       .withNumChannels (2).withBitsPerSample (32)
                                                       .withSampleFormat (juce::AudioFormatWriterOptions::SampleFormat::floatingPoint));

        if (writer == nullptr || ! writer->writeFromAudioSampleBuffer (sound, 0, sound.getNumSamples()))
            return false;
    }

    std::unique_ptr<juce::AudioFormatReader> reader (juce::WavAudioFormat().createReaderFor (new juce::MemoryInputStream (block, true), true));

    if (reader == nullptr)
        return false;

    engine.getPlayer().audition (std::move (reader));
    return true;
}

std::pair<float, float> CrossfadePanel::middleLevelsDb() const
{
    const auto [out, in] = shapesNow();
    const auto found = getCrossfades();
    const auto n = found.empty() ? (juce::int64) 1000 : found.front().length();
    const auto k = n / 2;
    // As the mixer plays them: the in side one step up from k, the out side down from n - 1 - k.
    const auto g = FadeShapes::level (out, n - 1 - k, n), h = FadeShapes::level (in, k + 1, n);
    return { juce::Decibels::gainToDecibels (g + h, -100.0f), juce::Decibels::gainToDecibels (std::sqrt (g * g + h * h), -100.0f) };
}

void CrossfadePanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refresh();
}

void CrossfadePanel::refresh()
{
    const juce::ScopedValueSetter<bool> quiet (refreshing, true);
    const auto found = getCrossfades();
    const auto [out, in] = shapesNow();
    length.setValue (currentValues().at ("ms"), juce::dontSendNotification);
    outShape.setSelectedId (indexOf (out) + 1, juce::dontSendNotification);
    inShape.setSelectedId (indexOf (in) + 1, juce::dontSendNotification);

    const auto [gain, power] = middleLevelsDb();
    const auto db = [] (float v) { return (v >= 0.0f ? "+" : "") + juce::String (v, 1) + " dB"; };
    summary.setText (found.empty() ? juce::String ("The crossfade is gone.")
                                   : (found.size() > 1 ? juce::String ((int) found.size()) + " crossfades. " : juce::String())
                                         + "At the middle: " + db (gain) + " in gain (steady for one sound), " + db (power) + " in power (steady for two).",
                     juce::dontSendNotification);

    for (auto* c : std::initializer_list<juce::Component*> { &length, &outShape, &inShape, &equalGainButton, &equalPowerButton, &playOutButton, &playInButton, &playBothButton })
        c->setEnabled (! found.empty());

    repaint();
}

juce::Rectangle<int> CrossfadePanel::curveArea() const
{
    return getLocalBounds().reduced (12).removeFromTop (200);
}

void CrossfadePanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::surface);
    const auto area = curveArea().toFloat();
    g.setColour (Theme::inset);
    g.fillRect (area);

    const auto found = getCrossfades();

    if (found.empty())
        return;

    const auto state = engine.session.getState();
    const auto& x = found.front();
    const auto* out = clipIn (*state, x.out);
    const auto* in = clipIn (*state, x.in);

    // Half the crossfade's length shown either side of it.
    const auto margin = juce::jmax ((juce::int64) 1, x.length() / 2);
    const auto from = x.start - margin, to = x.end + margin;
    const auto xOf = [&] (juce::int64 f) { return area.getX() + (float) (f - from) / (float) (to - from) * area.getWidth(); };
    const auto columns = juce::jmax (1, (int) area.getWidth());

    // Each clip's sound as it plays, before its fades: its peak in each column, the out clip above the middle, the in clip below.
    const auto drawWave = [&] (const SessionClip& c, juce::Colour colour, bool above)
    {
        if (c.audio == nullptr)
            return;

        juce::AudioBuffer<float> column (c.audio->getNumChannels(), 1);
        g.setColour (colour);

        for (int i = 0; i < columns; ++i)
        {
            const auto f0 = from + (to - from) * i / columns, f1 = juce::jmax (f0 + 1, from + (to - from) * (i + 1) / columns);
            auto peak = 0.0f;

            for (auto f = juce::jmax (f0, c.position); f < juce::jmin (f1, c.getEnd()); f += juce::jmax ((juce::int64) 1, (f1 - f0) / 16))
            {
                c.audio->read (column, 0, c.offset + f - c.position, 1);

                for (int ch = 0; ch < column.getNumChannels(); ++ch)
                    peak = juce::jmax (peak, std::abs (column.getSample (ch, 0)));
            }

            const auto h = juce::jmin (1.0f, peak) * area.getHeight() * 0.5f;
            const auto px = area.getX() + (float) i;
            g.drawVerticalLine ((int) px, above ? area.getCentreY() - h : area.getCentreY(), above ? area.getCentreY() : area.getCentreY() + h);
        }
    };

    drawWave (*out, Theme::waveform.withAlpha (0.45f), true);
    drawWave (*in, Theme::accent.withAlpha (0.45f), false);

    // The crossfade itself, the two curves over it, and what they add up to.
    const auto x0 = xOf (x.start), x1 = xOf (x.end);
    g.setColour (Theme::accent.withAlpha (0.10f));
    g.fillRect (juce::Rectangle<float>::leftTopRightBottom (x0, area.getY(), x1, area.getBottom()));

    const auto n = x.length();
    const auto yOf = [&] (float level) { return area.getBottom() - level * area.getHeight() * 0.8f - area.getHeight() * 0.05f; };
    juce::Path outCurve, inCurve, sumCurve;
    const auto steps = juce::jmax (2, (int) (x1 - x0));

    for (int i = 0; i <= steps; ++i)
    {
        const auto k = juce::jlimit ((juce::int64) 0, n - 1, (juce::int64) ((double) i / steps * (double) (n - 1)));
        const auto gOut = FadeShapes::level (out->fadeOutShape, n - 1 - k, n), gIn = FadeShapes::level (in->fadeInShape, k + 1, n);
        const auto px = x0 + (x1 - x0) * (float) i / (float) steps;
        const auto add = [&] (juce::Path& p, float y) { if (i == 0) p.startNewSubPath (px, y); else p.lineTo (px, y); };
        add (outCurve, yOf (gOut));
        add (inCurve, yOf (gIn));
        add (sumCurve, yOf (juce::jmin (1.25f, gOut + gIn)));
    }

    g.setColour (Theme::text);
    g.strokePath (outCurve, juce::PathStrokeType (1.5f));
    g.setColour (Theme::accent);
    g.strokePath (inCurve, juce::PathStrokeType (1.5f));
    g.setColour (Theme::warn.withAlpha (0.8f));
    const float dashes[] { 4.0f, 3.0f };
    juce::Path dashed;
    juce::PathStrokeType (1.0f).createDashedStroke (dashed, sumCurve, dashes, 2);
    g.fillPath (dashed);

    g.setColour (Theme::dimText);
    g.setFont (11.0f);
    g.drawText ("Out: " + out->name, area.reduced (6, 4), juce::Justification::topLeft, true);
    g.drawText ("In: " + in->name, area.reduced (6, 4), juce::Justification::bottomRight, true);
    g.drawText ("Sum (gain)", area.reduced (6, 4), juce::Justification::topRight, true);
}

void CrossfadePanel::resized()
{
    auto r = getLocalBounds().reduced (12);
    r.removeFromTop (200 + 6);
    summary.setBounds (r.removeFromTop (22));
    r.removeFromTop (6);

    const auto row = [&r] { auto line = r.removeFromTop (26); r.removeFromTop (6); return line; };
    auto line = row();
    lengthLabel.setBounds (line.removeFromLeft (100));
    length.setBounds (line);

    line = row();
    outLabel.setBounds (line.removeFromLeft (100));
    outShape.setBounds (line.removeFromLeft (150));
    line.removeFromLeft (20);
    inLabel.setBounds (line.removeFromLeft (70));
    inShape.setBounds (line.removeFromLeft (150));
    line.removeFromLeft (12);
    symmetricButton.setBounds (line);

    line = row();
    line.removeFromLeft (100);
    equalGainButton.setBounds (line.removeFromLeft (110));
    line.removeFromLeft (8);
    equalPowerButton.setBounds (line.removeFromLeft (110));
    line.removeFromLeft (20);
    presets.setBounds (line);

    line = row();
    preLabel.setBounds (line.removeFromLeft (100));
    preRoll.setBounds (line.removeFromLeft (220));
    line.removeFromLeft (16);
    postLabel.setBounds (line.removeFromLeft (100));
    postRoll.setBounds (line);

    line = row();
    line.removeFromLeft (100);
    playOutButton.setBounds (line.removeFromLeft (100));
    line.removeFromLeft (8);
    playInButton.setBounds (line.removeFromLeft (100));
    line.removeFromLeft (8);
    playBothButton.setBounds (line.removeFromLeft (130));
    cancelButton.setBounds (line.removeFromRight (90));
    line.removeFromRight (8);
    doneButton.setBounds (line.removeFromRight (90));
}
