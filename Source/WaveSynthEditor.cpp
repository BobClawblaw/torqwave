#include "WaveSynthEditor.h"
#include "MassiveXImport.h"
#include "SignalConsoleView.h"
#include "DecryptView.h"
#include "SoundDrops.h"
#include "WaveSynthFavourites.h"
#include "DecryptPatches.h"
#include "DecryptedView.h"
#include "Theme.h"

namespace
{
    using namespace SoundControls;
    using Synth = BuiltInEffects::BasicSynthProcessor;

    juce::Font bold (float size)    { return juce::FontOptions (size, juce::Font::bold); }
    juce::Font plain (float size)   { return juce::FontOptions (size, juce::Font::plain); }

    template <typename T> T* param (Synth& s, const juce::String& id)
    {
        auto* p = dynamic_cast<T*> (s.parameter (id));
        jassert (p != nullptr);
        return p;
    }

    float plainValue (Synth& s, const juce::String& id)
    {
        auto* p = s.parameter (id);
        return p != nullptr ? p->convertFrom0to1 (p->getValue()) : 0.0f;
    }

    /** A sample of a table's frame as the oscillator's warp plays it, for drawing. */
    float warpedSample (const WaveTables::Level& level, float position, double phase, int warp, float amount)
    {
        using W = WaveSynth::Warp;
        auto p = phase;

        switch ((W) warp)
        {
            case W::sync:    p = p * (1.0 + amount * 7.0); p -= std::floor (p); break;
            case W::bend:    { const auto q = (float) p * 2.0f - 1.0f, k = amount * 6.0f; p = (q * (1.0f + k) / (1.0f + k * std::abs (q))) * 0.5f + 0.5f; break; }
            case W::squeeze: { const auto w = 0.5f - 0.47f * amount; const auto f = (float) p; p = f < w ? f * 0.5f / w : 0.5f + (f - w) * 0.5f / (1.0f - w); break; }
            case W::mirror:  { const auto f = (float) p; const auto m = f < 0.5f ? f * 2.0f : (1.0f - f) * 2.0f; p = f + amount * (m - f); break; }
            case W::fm:      p += amount * 0.25 * std::sin (juce::MathConstants<double>::twoPi * 2.0 * p); p -= std::floor (p); break;
            case W::off: case W::crush: case W::fold: break;
        }

        auto s = WaveTables::read (level, position, p);

        if ((W) warp == W::crush && amount > 0.0f)
        {
            const auto steps = std::exp2 (8.0f - amount * 7.0f);
            s = std::round (s * steps) / steps;
        }
        else if ((W) warp == W::fold && amount > 0.0f)
        {
            const auto f = s * (1.0f + amount * 5.0f) * 0.25f + 0.25f;
            s = 4.0f * std::abs (f - std::floor (f + 0.5f)) - 1.0f;
        }

        return s;
    }

    //==========================================================================
    /** An oscillator's wavetable: its frames drawn one behind another in depth, the frame it plays
        in front with a glow, moving as its Position is modulated. A string shows as a pluck dying away. */
    class WavetableView final : public juce::Component,
                                public juce::FileDragAndDropTarget,
                                public juce::DragAndDropTarget,
                                public juce::SettableTooltipClient
    {
    public:
        WavetableView (Synth& s, int index)
            : synth (s), osc (index), positionDestination (WaveSynth::destinationIds().indexOf ("osc" + juce::String (index + 1) + "Position"))
        {
            setTooltip ("Drop a sound here to make a table from it: Position then plays through its tone, start to end. Double-click to choose a file");
        }

        /** Makes a Sampled table from an audio file (its first 30 seconds, channels mixed) and gives it
            to this oscillator; false if the file can't be read or is under a tenth of a second. */
        bool loadSound (const juce::File& file)
        {
            juce::AudioFormatManager formats;
            formats.registerBasicFormats();
            const std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));

            if (reader == nullptr || reader->lengthInSamples <= 0)
                return false;

            const auto length = (int) juce::jmin (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 30.0));
            juce::AudioBuffer<float> audio ((int) reader->numChannels, length);
            reader->read (&audio, 0, length, 0, true, true);
            return loadAudio (audio, reader->sampleRate, file.getFileNameWithoutExtension());
        }

        /** The same from audio in memory (a sound dragged from the Files panel). */
        bool loadAudio (juce::AudioBuffer<float> audio, double sampleRate, const juce::String& name)
        {
            const auto length = audio.getNumSamples();

            if (length <= 0 || audio.getNumChannels() <= 0)
                return false;

            for (int ch = 1; ch < audio.getNumChannels(); ++ch)
                audio.addFrom (0, 0, audio, ch, 0, length);

            audio.applyGain (0, 0, length, 1.0f / (float) audio.getNumChannels());
            const auto table = WaveTables::sampledFromAudio (audio.getReadPointer (0), length, sampleRate, 0.0, name);

            if (table == nullptr)
                return false;

            synth.setSampledTable (osc, table);
            auto* choice = param<juce::AudioParameterChoice> (synth, "osc" + juce::String (osc + 1) + "Table");
            choice->beginChangeGesture();
            choice->setValueNotifyingHost (choice->convertTo0to1 ((float) WaveTables::Table::sampled));
            choice->endChangeGesture();
            update();
            return true;
        }

        bool isInterestedInDragSource (const SourceDetails& d) override   { return SoundDrops::accepts (d.description); }
        void itemDragEnter (const SourceDetails&) override                { over = true; repaint(); }
        void itemDragExit (const SourceDetails&) override                 { over = false; repaint(); }

        void itemDropped (const SourceDetails& d) override
        {
            over = false;

            if (auto sound = SoundDrops::take (d.description, 30.0))
                loadAudio (std::move (sound->audio), sound->sampleRate, sound->name);

            repaint();
        }

        bool isInterestedInFileDrag (const juce::StringArray& files) override
        {
            return files.size() == 1 && juce::File (files[0]).hasFileExtension ("wav;aif;aiff;flac;ogg;mp3;m4a;caf");
        }

        void fileDragEnter (const juce::StringArray&, int, int) override   { over = true; repaint(); }
        void fileDragExit (const juce::StringArray&) override               { over = false; repaint(); }

        void filesDropped (const juce::StringArray& files, int, int) override
        {
            over = false;
            loadSound (juce::File (files[0]));
            repaint();
        }

        void mouseDoubleClick (const juce::MouseEvent&) override
        {
            chooser = std::make_unique<juce::FileChooser> ("Choose a sound to make a table from", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3;*.m4a;*.caf");
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [safe = juce::Component::SafePointer<WavetableView> (this)] (const juce::FileChooser& c)
                                  {
                                      if (safe != nullptr && c.getResult().existsAsFile())
                                          safe->loadSound (c.getResult());
                                  });
        }

        void update()
        {
            const auto n = "osc" + juce::String (osc + 1);
            State now;
            now.table = param<juce::AudioParameterChoice> (synth, n + "Table")->getIndex();
            now.warp = param<juce::AudioParameterChoice> (synth, n + "Warp")->getIndex();
            now.amount = plainValue (synth, n + "WarpAmount") * 0.01f;
            now.on = param<juce::AudioParameterBool> (synth, n + "On")->get();
            now.position = plainValue (synth, n + "Position") * 0.01f;
            const auto live = synth.getReadings().destination[(size_t) positionDestination].load (std::memory_order_relaxed);
            now.live = live >= 0.0f ? live : now.position;
            now.live = std::round (now.live * 200.0f) / 200.0f;
            now.own = now.table == (int) WaveTables::Table::sampled ? synth.getSampledTable (osc).get() : nullptr;

            if (now != state)
            {
                state = now;
                repaint();
            }
        }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat();
            drawDisplayWell (g, r, 8, 4);
            const auto colour = state.on ? Theme::accent : Theme::faintText;
            auto area = r.reduced (10.0f, 12.0f);

            if (state.table == (int) WaveTables::Table::string)
            {
                // A plucked string dying away: its brightness the position, its length the decay.
                juce::Path p;
                const auto decay = 1.5f + 8.0f * (1.0f - state.amount);
                const auto bright = state.live;

                for (int i = 0; i <= 200; ++i)
                {
                    const auto t = (float) i / 200.0f;
                    const auto env = std::exp (-decay * t);
                    const auto x = 18.0f * juce::MathConstants<float>::twoPi * t;
                    const auto s = env * (std::sin (x) + bright * 0.5f * std::sin (2.0f * x + 0.5f) * env + bright * 0.3f * std::sin (3.0f * x) * env * env);
                    const juce::Point<float> pt { area.getX() + t * area.getWidth(), area.getCentreY() - s * area.getHeight() * 0.42f };
                    i == 0 ? p.startNewSubPath (pt) : p.lineTo (pt);
                }

                drawGlowingCurve (g, p, colour, area, false);
                label (g, r, "Plucked string", juce::String (juce::roundToInt (state.live * 100.0f)) + " % bright");
                return;
            }

            // A sampled table is the synth's own; without one, the sawtooth it plays instead.
            const auto own = state.table == (int) WaveTables::Table::sampled ? synth.getSampledTable (osc) : nullptr;
            const auto& level = (own != nullptr ? own->bank : WaveTables::get ((WaveTables::Table) state.table)).levels[3];
            constexpr int layers = 9;
            const auto depthX = area.getWidth() * 0.16f, depthY = area.getHeight() * 0.26f;
            const auto front = area.withTrimmedRight (depthX).withTrimmedTop (depthY);

            // The frames behind, faint, the far ones fainter; the one played highlighted among them.
            for (int k = layers - 1; k >= 0; --k)
            {
                const auto pos = (float) k / (float) (layers - 1);
                const auto offset = juce::Point<float> (depthX * pos, -depthY * pos);
                const auto near = std::abs (pos - state.live) < 0.5f / (float) (layers - 1);
                const auto path = framePath (level, pos, front.translated (offset.x, offset.y), 64, 0.82f);
                g.setColour (colour.withAlpha (near ? 0.55f : 0.10f + 0.1f * (1.0f - pos)));
                g.strokePath (path, juce::PathStrokeType (near ? 1.2f : 0.8f));
            }

            // The frame played, at its place in the depth, glowing.
            const auto offset = juce::Point<float> (depthX * state.live, -depthY * state.live);
            const auto lit = framePath (level, state.live, front.translated (offset.x, offset.y), 160, 0.82f, true);
            drawGlowingCurve (g, lit, colour, front.translated (offset.x, offset.y), false);
            label (g, r, own != nullptr ? own->name : state.table == (int) WaveTables::Table::sampled ? juce::String ("Drop a sound here") : WaveTables::names()[state.table],
                   juce::String (juce::roundToInt (state.live * 100.0f)) + " %");

            if (over)
            {
                g.setColour (Theme::accent.withAlpha (0.8f));
                g.drawRoundedRectangle (r.reduced (1.5f), 6.0f, 1.5f);
            }
        }

    private:
        struct State
        {
            int table = -1, warp = 0;
            float amount = 0.0f, position = 0.0f, live = 0.0f;
            bool on = true;
            const void* own = nullptr;   // the sampled table it shows
            bool operator!= (const State& o) const { return own != o.own || table != o.table || warp != o.warp || ! juce::exactlyEqual (amount, o.amount) || ! juce::exactlyEqual (live, o.live) || on != o.on || ! juce::exactlyEqual (position, o.position); }
        };

        juce::Path framePath (const WaveTables::Level& level, float position, juce::Rectangle<float> box, int points, float height, bool warped = false) const
        {
            juce::Path p;

            for (int i = 0; i <= points; ++i)
            {
                const auto phase = (double) i / (double) points;
                const auto s = warped ? warpedSample (level, position, phase, state.warp, state.amount) : WaveTables::read (level, position, phase);
                const juce::Point<float> pt { box.getX() + (float) phase * box.getWidth(), box.getCentreY() - s * box.getHeight() * 0.5f * height };
                i == 0 ? p.startNewSubPath (pt) : p.lineTo (pt);
            }

            return p;
        }

        static void label (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& left, const juce::String& right)
        {
            auto strip = r.reduced (7.0f, 4.0f).removeFromTop (13.0f);
            g.setFont (bold (9.5f));
            g.setColour (Theme::dimText);
            g.drawText (left.toUpperCase(), strip, juce::Justification::centredLeft);
            g.setColour (Theme::accentLight);
            g.drawText (right, strip, juce::Justification::centredRight);
        }

        Synth& synth;
        int osc, positionDestination;
        bool over = false;   // a sound is being dragged over it
        std::unique_ptr<juce::FileChooser> chooser;
        State state;
    };

    //==========================================================================
    /** A filter's response from its type, cutoff (as modulated now) and resonance. */
    class FilterView final : public juce::Component
    {
    public:
        FilterView (Synth& s, int index) : synth (s), filter (index) {}

        void update()
        {
            const auto n = "filter" + juce::String (filter + 1);
            const auto type = param<juce::AudioParameterChoice> (synth, n + "Type")->getIndex();
            auto* cutoff = synth.parameter (n + "Cutoff");
            auto* res = synth.parameter (n + "Resonance");
            const auto base = filter == 0 ? 30 : 33;
            const auto liveCut = synth.getReadings().destination[(size_t) base].load (std::memory_order_relaxed);
            const auto liveRes = synth.getReadings().destination[(size_t) base + 1].load (std::memory_order_relaxed);
            const auto hz = cutoff->convertFrom0to1 (liveCut >= 0.0f ? liveCut : cutoff->getValue());
            const auto q = liveRes >= 0.0f ? liveRes : res->getValue();
            const auto rounded = std::round (std::log2 (hz) * 60.0f) / 60.0f;

            if (type != shownType || ! juce::exactlyEqual (rounded, shownHz) || std::abs (q - shownRes) > 0.004f)
            {
                shownType = type;
                shownHz = rounded;
                shownRes = q;
                repaint();
            }
        }

        /** The magnitude at f (Hz) as the analogue model the filter follows has it. */
        static float magnitude (int type, float f, float fc, float res)
        {
            using C = std::complex<float>;
            const C s (0.0f, f / fc);
            using T = WaveSynth::FilterType;

            switch ((T) type)
            {
                case T::ladder24: case T::ladder12: case T::ladderHigh: case T::ladderBand:
                {
                    const auto k = res * 4.1f;
                    const auto p = C (1.0f) + s;
                    const auto p2 = p * p, p4 = p2 * p2;
                    const auto u = C (1.0f) / (C (1.0f) + k / p4);
                    const auto boost = (type == (int) T::ladder24 || type == (int) T::ladder12) ? 1.0f + 0.6f * k : 1.0f;
                    const auto y1 = u / p, y2 = u / p2, y3 = y2 / p, y4 = u / p4;
                    C y;
                    if (type == (int) T::ladder24) y = y4;
                    else if (type == (int) T::ladder12) y = y2;
                    else if (type == (int) T::ladderHigh) y = u - 4.0f * y1 + 6.0f * y2 - 4.0f * y3 + y4;
                    else y = 4.0f * (y2 - 2.0f * y3 + y4);
                    return std::abs (y) * boost;
                }
                case T::svfLow: case T::svfBand: case T::svfHigh: case T::svfNotch:
                {
                    const auto k = 2.0f - 1.97f * res;
                    const auto d = s * s + k * s + C (1.0f);
                    if (type == (int) T::svfLow) return std::abs (C (1.0f) / d);
                    if (type == (int) T::svfBand) return std::abs (k * s / d);
                    if (type == (int) T::svfHigh) return std::abs (s * s / d);
                    return std::abs ((s * s + C (1.0f)) / d);
                }
                case T::comb:
                {
                    const auto fb = res * 0.97f;
                    const auto w = juce::MathConstants<float>::twoPi * f / fc;
                    return std::abs (C (1.0f) / (C (1.0f) - fb * std::exp (C (0.0f, -w)))) * (1.0f - 0.5f * fb);
                }
                case T::off: break;
            }

            return 1.0f;
        }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat();
            drawDisplayWell (g, r, 10, 4);
            const auto area = r.reduced (4.0f, 8.0f);
            const auto fc = std::exp2 (shownHz);
            const auto off = shownType == (int) WaveSynth::FilterType::off;
            constexpr float topDb = 24.0f, bottomDb = -36.0f;
            const auto xFor = [&area] (float hz) { return area.getX() + std::log (hz / 20.0f) / std::log (1000.0f) * area.getWidth(); };
            const auto yFor = [&area] (float db) { return area.getY() + (topDb - juce::jlimit (bottomDb, topDb, db)) / (topDb - bottomDb) * area.getHeight(); };

            // 0 dB, faintly.
            g.setColour (Theme::overlay.withAlpha (0.08f));
            g.drawHorizontalLine ((int) yFor (0.0f), area.getX(), area.getRight());
            juce::Path p;

            for (int i = 0; i <= 160; ++i)
            {
                const auto hz = 20.0f * std::pow (1000.0f, (float) i / 160.0f);
                const auto db = off ? 0.0f : juce::Decibels::gainToDecibels (magnitude (shownType, hz, fc, shownRes), -60.0f);
                const juce::Point<float> pt { xFor (hz), yFor (db) };
                i == 0 ? p.startNewSubPath (pt) : p.lineTo (pt);
            }

            drawGlowingCurve (g, p, off ? Theme::faintText : Theme::accent, area);

            if (! off && shownType != (int) WaveSynth::FilterType::comb)
            {
                g.setColour (Theme::accentLight.withAlpha (0.5f));
                g.drawVerticalLine ((int) xFor (fc), area.getY(), area.getBottom());
            }

            g.setFont (bold (9.5f));
            g.setColour (Theme::accentLight);
            g.drawText (off ? juce::String ("OFF") : formatValue (fc, "Hz"), r.reduced (7.0f, 4.0f).removeFromTop (13.0f), juce::Justification::centredRight);
        }

    private:
        Synth& synth;
        int filter;
        int shownType = -1;
        float shownHz = 0.0f, shownRes = 0.0f;
    };

    //==========================================================================
    /** An envelope's shape with three points to drag: the attack's peak (its time), the decay's
        end (its time, and the sustain level up and down) and the release's end. A dot rides the
        newest note's level. */
    class EnvelopeView final : public juce::Component
    {
    public:
        EnvelopeView (Synth& s) : synth (s) {}

        void setEnvelope (int e)
        {
            index = e;
            shown = {};
            update();
        }

        void update()
        {
            const auto n = "env" + juce::String (index + 1);
            std::array<float, 6> now { plainValue (synth, n + "Attack"), plainValue (synth, n + "Decay"), plainValue (synth, n + "Sustain"),
                                       plainValue (synth, n + "Release"), plainValue (synth, n + "Curve"),
                                       std::round (synth.getReadings().envelope[(size_t) index].load() * 100.0f) / 100.0f };

            if (now != shown)
            {
                shown = now;
                repaint();
            }
        }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat();
            drawDisplayWell (g, r, 8, 4);
            const auto pts = points();
            const auto k = shown[4] * 0.01f * 6.0f;
            const auto curveAt = [] (float t, float kk) { return std::abs (kk) < 0.01f ? t : (1.0f - std::exp (-kk * t)) / (1.0f - std::exp (-kk)); };
            juce::Path p;
            p.startNewSubPath (pts[0]);
            const auto segment = [&p, &curveAt] (juce::Point<float> a, juce::Point<float> b, float kk)
            {
                for (int i = 1; i <= 40; ++i)
                {
                    const auto t = (float) i / 40.0f;
                    p.lineTo (a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * curveAt (t, kk));
                }
            };
            segment (pts[0], pts[1], k * 0.5f);
            segment (pts[1], pts[2], k);
            p.lineTo (pts[3]);
            segment (pts[3], pts[4], k);
            const auto colour = SoundControls::sourceColour (index);
            drawGlowingCurve (g, p, colour, inner());

            // The handles.
            for (int i : { 1, 2, 4 })
            {
                const auto hot = i == dragging || i == hovering;
                g.setColour (Theme::inset);
                g.fillEllipse (juce::Rectangle<float> (11.0f, 11.0f).withCentre (pts[(size_t) i]));
                g.setColour (colour.withAlpha (hot ? 1.0f : 0.8f));
                g.drawEllipse (juce::Rectangle<float> (hot ? 11.0f : 9.0f, hot ? 11.0f : 9.0f).withCentre (pts[(size_t) i]), 2.0f);
            }

            // The newest note's level.
            if (shown[5] > 0.001f)
            {
                const auto y = inner().getBottom() - shown[5] * inner().getHeight();
                g.setColour (colour.withAlpha (0.35f));
                g.drawHorizontalLine ((int) y, inner().getX(), inner().getRight());
            }

            g.setFont (bold (9.5f));
            g.setColour (Theme::dimText);
            g.drawText (index == 0 ? "AMP ENVELOPE" : "ENVELOPE " + juce::String (index + 1), r.reduced (7.0f, 4.0f).removeFromTop (13.0f), juce::Justification::centredLeft);
        }

        void mouseMove (const juce::MouseEvent& e) override
        {
            const auto h = handleAt (e.position);
            if (h != hovering) { hovering = h; repaint(); }
            setMouseCursor (h >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
        }

        void mouseExit (const juce::MouseEvent&) override { hovering = -1; repaint(); }

        void mouseDown (const juce::MouseEvent& e) override
        {
            dragging = handleAt (e.position);

            for (auto* p : touched())
                p->beginChangeGesture();
        }

        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (dragging < 0)
                return;

            const auto n = "env" + juce::String (index + 1);
            const auto box = inner();
            const auto segmentWidth = box.getWidth() * 0.27f;
            const auto pts = points();
            const auto setTime = [&] (const juce::String& id, float px)
            {
                auto* p = synth.parameter (id);
                // The handle's distance along its segment is the knob's position (the same skew).
                p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, px / segmentWidth));
            };

            if (dragging == 1)
                setTime (n + "Attack", e.position.x - box.getX());
            else if (dragging == 2)
            {
                setTime (n + "Decay", e.position.x - pts[1].x);
                auto* s = synth.parameter (n + "Sustain");
                s->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, (box.getBottom() - e.position.y) / box.getHeight()));
            }
            else if (dragging == 4)
                setTime (n + "Release", e.position.x - pts[3].x);

            update();
        }

        void mouseUp (const juce::MouseEvent&) override
        {
            for (auto* p : touched())
                p->endChangeGesture();

            dragging = -1;
            repaint();
        }

    private:
        std::vector<juce::RangedAudioParameter*> touched()
        {
            const auto n = "env" + juce::String (index + 1);

            switch (dragging)
            {
                case 1: return { synth.parameter (n + "Attack") };
                case 2: return { synth.parameter (n + "Decay"), synth.parameter (n + "Sustain") };
                case 4: return { synth.parameter (n + "Release") };
                default: return {};
            }
        }

        juce::Rectangle<float> inner() const   { return getLocalBounds().toFloat().reduced (12.0f, 10.0f).withTrimmedTop (10.0f); }

        /** Start, peak, decay's end, sustain's end, release's end. */
        std::array<juce::Point<float>, 5> points() const
        {
            const auto box = inner();
            const auto w = box.getWidth() * 0.27f;   // each time segment's most
            const auto n = "env" + juce::String (index + 1);
            const auto norm = [&] (const char* f) { return synth.parameter (n + f)->getValue(); };
            const auto a = norm ("Attack") * w, d = norm ("Decay") * w, rel = norm ("Release") * w;
            const auto sustainY = box.getBottom() - shown[2] * 0.01f * box.getHeight();
            const auto x0 = box.getX();
            const auto holdEnd = box.getRight() - w;
            return { juce::Point<float> { x0, box.getBottom() }, { x0 + a, box.getY() }, { x0 + a + d, sustainY },
                     { juce::jmax (x0 + a + d + 4.0f, holdEnd), sustainY }, { juce::jmax (x0 + a + d + 4.0f, holdEnd) + rel, box.getBottom() } };
        }

        int handleAt (juce::Point<float> p) const
        {
            const auto pts = points();

            for (int i : { 1, 2, 4 })
                if (pts[(size_t) i].getDistanceFrom (p) < 9.0f)
                    return i;

            return -1;
        }

        Synth& synth;
        int index = 0, dragging = -1, hovering = -1;
        std::array<float, 6> shown {};
    };

    //==========================================================================
    /** An LFO's cycle drawn; with the Steps shape, its sixteen steps drawn as bars to click and
        drag. A line marks its value now. */
    class LfoView final : public juce::Component
    {
    public:
        LfoView (Synth& s) : synth (s) {}

        void setLfo (int l)
        {
            index = l;
            shownShape = -1;
            update();
        }

        void update()
        {
            const auto n = "lfo" + juce::String (index + 1);
            const auto shape = param<juce::AudioParameterChoice> (synth, n + "Shape")->getIndex();
            std::array<float, WaveSynth::numSteps> steps {};

            for (int i = 0; i < WaveSynth::numSteps; ++i)
                steps[(size_t) i] = plainValue (synth, n + "Step" + juce::String (i + 1));

            const auto live = std::round (synth.getReadings().lfo[(size_t) index].load() * 50.0f) / 50.0f;

            if (shape != shownShape || steps != shownSteps || ! juce::exactlyEqual (live, shownLive))
            {
                shownShape = shape;
                shownSteps = steps;
                shownLive = live;
                repaint();
            }
        }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat();
            drawDisplayWell (g, r, 16, 4);
            const auto box = inner();
            const auto colour = SoundControls::sourceColour (WaveSynth::lfoSource (index) - 1);
            g.setColour (Theme::overlay.withAlpha (0.1f));
            g.drawHorizontalLine ((int) box.getCentreY(), box.getX(), box.getRight());

            if (shownShape == 7)
            {
                const auto w = box.getWidth() / WaveSynth::numSteps;

                for (int i = 0; i < WaveSynth::numSteps; ++i)
                {
                    const auto v = shownSteps[(size_t) i] * 0.01f;
                    const auto top = box.getCentreY() - v * box.getHeight() * 0.5f;
                    auto bar = juce::Rectangle<float>::leftTopRightBottom (box.getX() + w * (float) i + 1.5f, juce::jmin (top, box.getCentreY()),
                                                                          box.getX() + w * (float) (i + 1) - 1.5f, juce::jmax (top, box.getCentreY()));
                    g.setColour (colour.withAlpha (0.25f));
                    g.fillRoundedRectangle (bar, 2.0f);
                    g.setColour (colour);
                    g.fillRect (bar.getX(), top - 1.0f, bar.getWidth(), 2.0f);
                }
            }
            else
            {
                juce::Path p;

                for (int i = 0; i <= 200; ++i)
                {
                    const auto t = (float) i / 200.0f;
                    const juce::Point<float> pt { box.getX() + t * box.getWidth(), box.getCentreY() - shapeAt (t) * box.getHeight() * 0.5f };
                    i == 0 ? p.startNewSubPath (pt) : p.lineTo (pt);
                }

                drawGlowingCurve (g, p, colour, box, false);

                if (shownShape == 8)
                {
                    const auto w = box.getWidth() / WaveSynth::numSteps;
                    for (int i = 0; i < WaveSynth::numSteps; ++i)
                    {
                        const auto v = shownSteps[(size_t) i] * 0.01f;
                        const juce::Point<float> node { box.getX() + w * ((float) i + 0.5f), box.getCentreY() - v * box.getHeight() * 0.5f };
                        g.setColour (colour);
                        g.fillEllipse (juce::Rectangle<float> (5.5f, 5.5f).withCentre (node));
                    }
                }
            }

            g.setColour (colour.withAlpha (0.7f));
            const auto y = box.getCentreY() - shownLive * box.getHeight() * 0.5f;
            g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ box.getRight() + 5.0f, y }));
            g.setFont (bold (9.5f));
            g.setColour (Theme::dimText);
            g.drawText ("LFO " + juce::String (index + 1) + "  " + WaveSynth::lfoShapeNames()[juce::jmax (0, shownShape)].toUpperCase()
                            + (shownShape == 7 ? "  (draw the steps)" : shownShape == 8 ? "  (drag curve nodes)" : ""),
                        r.reduced (7.0f, 4.0f).removeFromTop (13.0f), juce::Justification::centredLeft);
        }

        void mouseDown (const juce::MouseEvent& e) override    { drawStep (e.position, true); }
        void mouseDrag (const juce::MouseEvent& e) override    { drawStep (e.position, false); }
        void mouseUp (const juce::MouseEvent&) override
        {
            if (gestureStep >= 0)
                synth.parameter (stepId (gestureStep))->endChangeGesture();
            gestureStep = -1;
        }

    private:
        juce::String stepId (int i) const   { return "lfo" + juce::String (index + 1) + "Step" + juce::String (i + 1); }

        void drawStep (juce::Point<float> p, bool)
        {
            if (shownShape != 7 && shownShape != 8)
                return;

            const auto box = inner();
            const auto i = juce::jlimit (0, WaveSynth::numSteps - 1, (int) ((p.x - box.getX()) / box.getWidth() * WaveSynth::numSteps));

            if (i != gestureStep)
            {
                if (gestureStep >= 0)
                    synth.parameter (stepId (gestureStep))->endChangeGesture();
                gestureStep = i;
                synth.parameter (stepId (i))->beginChangeGesture();
            }

            const auto v = juce::jlimit (-1.0f, 1.0f, (box.getCentreY() - p.y) / (box.getHeight() * 0.5f));
            auto* param = synth.parameter (stepId (i));
            param->setValueNotifyingHost (param->convertTo0to1 (v * 100.0f));
            update();
        }

        float shapeAt (float t) const
        {
            const auto tau = juce::MathConstants<float>::twoPi;
            const auto hash = [] (int c) { auto x = (juce::uint32) c * 2654435761u + 11u; x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; return (float) (x >> 8) / 8388608.0f - 1.0f; };

            switch (shownShape)
            {
                case 0: return std::sin (tau * t);
                case 1: return t < 0.25f ? 4.0f * t : t < 0.75f ? 2.0f - 4.0f * t : 4.0f * t - 4.0f;
                case 2: return 2.0f * t - 1.0f;
                case 3: return 1.0f - 2.0f * t;
                case 4: return t < 0.5f ? 1.0f : -1.0f;
                case 5: return hash ((int) (t * 8.0f));
                case 6: { const auto x = t * 8.0f; const auto c = (int) x; const auto f = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi * (x - (float) c)); return hash (c) + (hash (c + 1) - hash (c)) * f; }
                case 8:
                {
                    const auto pos = t * (float) WaveSynth::numSteps;
                    const auto i1 = juce::jlimit (0, WaveSynth::numSteps - 1, (int) std::floor (pos));
                    const auto f = pos - std::floor (pos);
                    const auto p0 = shownSteps[(size_t) ((i1 + WaveSynth::numSteps - 1) % WaveSynth::numSteps)] * 0.01f;
                    const auto p1 = shownSteps[(size_t) i1] * 0.01f;
                    const auto p2 = shownSteps[(size_t) ((i1 + 1) % WaveSynth::numSteps)] * 0.01f;
                    const auto p3 = shownSteps[(size_t) ((i1 + 2) % WaveSynth::numSteps)] * 0.01f;
                    const auto f2 = f * f, f3 = f2 * f;
                    return juce::jlimit (-1.0f, 1.0f, 0.5f * ((2.0f * p1) + (-p0 + p2) * f
                                                              + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * f2
                                                              + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * f3));
                }
                default: return 0.0f;
            }
        }

        juce::Rectangle<float> inner() const   { return getLocalBounds().toFloat().reduced (12.0f, 10.0f).withTrimmedTop (10.0f).withTrimmedRight (6.0f); }

        Synth& synth;
        int index = 0, shownShape = -1, gestureStep = -1;
        std::array<float, WaveSynth::numSteps> shownSteps {};
        float shownLive = 0.0f;
    };

    //==========================================================================
    /** A modulation source that is also a tab: click to show it, drag onto a knob to route it. */
    class SourceTab final : public ModSourceChip
    {
    public:
        SourceTab (int source, const juce::String& text) : ModSourceChip (source, text)
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
            setTooltip ("Click to edit " + text + "; drag it onto a knob to modulate that knob");
        }

        std::function<void()> onClick;

        void mouseUp (const juce::MouseEvent& e) override
        {
            if (! e.mouseWasDraggedSinceMouseDown() && onClick != nullptr)
                onClick();
        }
    };

    /** A tab beside the sources' that opens a page and is no source itself. */
    class PlainTab final : public juce::Component,
                           public juce::SettableTooltipClient
    {
    public:
        explicit PlainTab (const juce::String& t) : text (t)   { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

        std::function<void()> onClick;
        void setHighlighted (bool h)                            { highlighted = h; repaint(); }

        void paint (juce::Graphics& g) override
        {
            const auto r = getLocalBounds().toFloat().reduced (0.5f);
            g.setColour (Theme::accent.withAlpha (hovered || highlighted ? 0.3f : 0.12f));
            g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
            g.setColour (Theme::accent.withAlpha (hovered || highlighted ? 1.0f : 0.6f));
            g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
            g.setColour (highlighted ? Theme::textStrong : Theme::dimText);
            g.setFont (bold (10.5f));
            g.drawText (text, r, juce::Justification::centred);
        }

        void mouseEnter (const juce::MouseEvent&) override    { hovered = true; repaint(); }
        void mouseExit (const juce::MouseEvent&) override     { hovered = false; repaint(); }
        void mouseUp (const juce::MouseEvent& e) override      { if (! e.mouseWasDraggedSinceMouseDown() && onClick != nullptr) onClick(); }

    private:
        juce::String text;
        bool hovered = false, highlighted = false;
    };

    /** Two or more page buttons drawn as a segmented switch. */
    class PageSwitch final : public juce::Component,
                             public juce::SettableTooltipClient
    {
    public:
        explicit PageSwitch (juce::StringArray names) : pages (std::move (names)) {}

        std::function<void (int)> onChange;
        int chosen = 0;

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (0.5f);
            g.setColour (Theme::inset);
            g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
            const auto w = r.getWidth() / (float) pages.size();

            for (int i = 0; i < pages.size(); ++i)
            {
                auto cell = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight()).reduced (2.0f);

                if (i == chosen)
                {
                    g.setColour (Theme::accentDeep);
                    g.fillRoundedRectangle (cell, cell.getHeight() * 0.5f);
                }

                g.setColour (i == chosen ? Theme::textStrong : Theme::dimText);
                g.setFont (bold (10.5f).withExtraKerningFactor (0.08f));
                g.drawText (pages[i].toUpperCase(), cell, juce::Justification::centred);
            }
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            const auto i = juce::jlimit (0, pages.size() - 1, (int) (e.position.x / ((float) getWidth() / (float) pages.size())));

            if (i != chosen)
            {
                chosen = i;
                repaint();
                if (onChange != nullptr) onChange (i);
            }
        }

    private:
        juce::StringArray pages;
    };

    //==========================================================================
    /** The OSCILLATORS strip: all nine at a glance, a cell each with its switch, its table, its wave at
        Position and its level. The panels below show three at a time; the page switch, or a click on a
        cell, chooses which three. A click on a cell's dot switches that oscillator on or off. */
    class OscillatorStrip final : public Panel
    {
    public:
        explicit OscillatorStrip (Synth& s) : Panel ("Oscillators"), pages ({ "1-3", "4-6", "7-9" })
        {
            setInterceptsMouseClicks (false, true);
            pages.setTooltip ("Which three oscillators the panels below show");
            pages.onChange = [this] (int p) { if (onPage != nullptr) onPage (p); };
            addAndMakeVisible (pages);

            for (int o = 0; o < WaveSynth::numOscillators; ++o)
            {
                cells[(size_t) o] = std::make_unique<Cell> (s, o);
                cells[(size_t) o]->onChoose = [this, o] { if (onPage != nullptr) onPage (o / WaveSynth::numFirstOscillators); };
                addAndMakeVisible (*cells[(size_t) o]);
            }
        }

        std::function<void (int)> onPage;

        void setPage (int page)
        {
            pages.chosen = page;
            pages.repaint();

            for (int o = 0; o < WaveSynth::numOscillators; ++o)
                cells[(size_t) o]->setShown (o / WaveSynth::numFirstOscillators == page);
        }

        void update()
        {
            for (auto& c : cells)
                c->update();
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced (8, 5);
            r.setLeft (getHeaderBounds().getX());
            pages.setBounds (r.removeFromLeft (150).removeFromTop (22));
            r.removeFromLeft (14);
            const auto w = r.getWidth() / WaveSynth::numOscillators;

            for (auto& c : cells)
                c->setBounds (r.removeFromLeft (w).reduced (3, 0));
        }

    private:
        class Cell final : public juce::Component,
                           public juce::SettableTooltipClient
        {
        public:
            Cell (Synth& s, int index) : synth (s), osc (index), id ("osc" + juce::String (index + 1))
            {
                setMouseCursor (juce::MouseCursor::PointingHandCursor);
                setTooltip ("Oscillator " + juce::String (index + 1) + ": click to show it below; click the dot to switch it on or off");
            }

            std::function<void()> onChoose;

            void setShown (bool s)        { if (s != shown) { shown = s; repaint(); } }

            void update()
            {
                State now;
                now.on = param<juce::AudioParameterBool> (synth, id + "On")->get();
                now.table = param<juce::AudioParameterChoice> (synth, id + "Table")->getIndex();
                now.position = std::round (plainValue (synth, id + "Position") * 0.01f * 100.0f) / 100.0f;
                now.level = std::round (synth.parameter (id + "Volume")->getValue() * 100.0f) / 100.0f;
                auto table = now.table == (int) WaveTables::Table::sampled ? synth.getSampledTable (osc) : nullptr;
                now.own = table.get();

                if (now != state)
                {
                    state = now;
                    held = std::move (table);
                    repaint();
                }
            }

            void paint (juce::Graphics& g) override
            {
                auto r = getLocalBounds().toFloat().reduced (0.5f);
                g.setColour (Theme::inset);
                g.fillRoundedRectangle (r, Theme::radius);
                g.setColour (shown ? Theme::accent : Theme::line);
                g.drawRoundedRectangle (r, Theme::radius, shown ? 1.5f : 1.0f);

                const auto colour = state.on ? Theme::accent : Theme::faintText;
                auto inner = r.reduced (6.0f, 3.0f);
                auto bar = inner.removeFromRight (4.0f);
                inner.removeFromRight (4.0f);
                auto top = inner.removeFromTop (13.0f);

                // The dot, the number and the table.
                const auto dot = top.removeFromLeft (9.0f).withSizeKeepingCentre (7.0f, 7.0f);
                g.setColour (colour);
                state.on ? g.fillEllipse (dot) : g.drawEllipse (dot, 1.2f);
                top.removeFromLeft (4.0f);
                g.setColour (state.on ? Theme::textStrong : Theme::dimText);
                g.setFont (bold (10.5f));
                g.drawText (juce::String (osc + 1), top.removeFromLeft (10.0f), juce::Justification::centredLeft);
                g.setFont (plain (9.5f));
                g.drawFittedText (state.on ? tableName() : juce::String ("Off"), top.toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);

                // The wave at Position.
                juce::Path p;
                const auto& level = (held != nullptr ? held->bank : WaveTables::get ((WaveTables::Table) state.table)).levels[3];
                constexpr int points = 48;

                for (int i = 0; i <= points; ++i)
                {
                    const auto phase = (double) i / (double) points;
                    const auto t = (float) phase;
                    const auto v = state.table == (int) WaveTables::Table::string ? std::exp (-3.0f * t) * std::sin (6.0f * juce::MathConstants<float>::twoPi * t)
                                                                                 : WaveTables::read (level, state.position, phase);
                    const juce::Point<float> pt { inner.getX() + t * inner.getWidth(), inner.getCentreY() - v * inner.getHeight() * 0.42f };
                    i == 0 ? p.startNewSubPath (pt) : p.lineTo (pt);
                }

                g.setColour (state.on ? colour : colour.withAlpha (0.5f));
                g.strokePath (p, juce::PathStrokeType (1.2f));

                // The level.
                g.setColour (Theme::surface);
                g.fillRoundedRectangle (bar, 1.5f);

                if (state.on)
                {
                    g.setColour (Theme::accent);
                    g.fillRoundedRectangle (bar.withTrimmedTop (bar.getHeight() * (1.0f - state.level)), 1.5f);
                }
            }

            void mouseUp (const juce::MouseEvent& e) override
            {
                if (e.mouseWasDraggedSinceMouseDown())
                    return;

                if (e.position.x < 20.0f && e.position.y < 20.0f)
                {
                    auto* on = synth.parameter (id + "On");
                    on->beginChangeGesture();
                    on->setValueNotifyingHost (on->getValue() > 0.5f ? 0.0f : 1.0f);
                    on->endChangeGesture();
                    update();
                }

                if (onChoose != nullptr)
                    onChoose();
            }

        private:
            struct State
            {
                bool on = false;
                int table = 0;
                float position = 0.0f, level = 0.0f;
                const void* own = nullptr;
                bool operator!= (const State& o) const { return on != o.on || table != o.table || ! juce::exactlyEqual (position, o.position) || ! juce::exactlyEqual (level, o.level) || own != o.own; }
            };

            juce::String tableName() const
            {
                if (state.table == (int) WaveTables::Table::sampled)
                    return held != nullptr ? held->name : juce::String ("Sampled");

                return WaveTables::names()[state.table];
            }

            Synth& synth;
            int osc;
            juce::String id;
            bool shown = false;
            State state;
            std::shared_ptr<const WaveTables::Sampled> held;   // the Sampled table drawn, kept while it is
        };

        PageSwitch pages;
        std::array<std::unique_ptr<Cell>, WaveSynth::numOscillators> cells;
    };

    /** The patch browser: the category and name, arrows for the one before and after, a heart to mark
        the patch as a favourite, and a click for the whole list: the favourites first, then by
        category. A patch chosen from the favourites puts the arrows on the favourites alone. */
    class PatchBrowser final : public juce::Component,
                               public juce::SettableTooltipClient
    {
    public:
        explicit PatchBrowser (Synth& s) : synth (s)    { setComponentID ("patchBrowser"); }

        /** Where the heart is. */
        juce::Rectangle<float> heartBounds() const
        {
            return { (float) getWidth() - 24.0f - 26.0f, 0.0f, 24.0f, (float) getHeight() };
        }

        /** Marks the patch it is on as a favourite or unmarks it (a factory patch only). */
        void toggleFavourite()
        {
            if (currentIndex() >= 0)
            {
                WaveSynth::Favourites::shared().toggle (synth.getPatchName());
                repaint();
            }
        }

        juce::String getTooltip() override
        {
            const auto x = (float) getMouseXYRelative().x;

            if (x < 24.0f)                       return synth.isBrowsingFavourites() ? "The favourite before this one" : synth.isBrowsingDecrypted() ? "The Decrypted patch before this one" : synth.isBrowsingComplex() ? "The Complex patch before this one" : "The patch before this one";
            if (x > (float) getWidth() - 24.0f)  return synth.isBrowsingFavourites() ? "The next favourite" : synth.isBrowsingDecrypted() ? "The next Decrypted patch" : synth.isBrowsingComplex() ? "The next Complex patch" : "The next patch";
            if (heartBounds().contains (getMouseXYRelative().toFloat()))
                return WaveSynth::Favourites::shared().contains (synth.getPatchName()) ? "A favourite: click to unmark it" : "Mark this patch as a favourite";

            return "Click for all the patches: your favourites first, then the ones you took from Decrypt, then by category. Right-click one of your own to rename or delete it";
        }

        std::function<void()> onLoaded;
        std::function<void()> onOwnChanged;     // one of the user's own patches was renamed or deleted

        /** Whether the synth is on one of the patches taken from Decrypt. */
        bool onDecrypted() const
        {
            return currentIndex() < 0 && DecryptedPatches::Store::shared().contains (synth.getPatchName());
        }

        /** Renames the Decrypted patch `from`; false for a name that is empty or taken. The synth's patch takes the name if it is on it. */
        bool rename (const juce::String& from, const juce::String& to)
        {
            const auto onIt = synth.getPatchName().equalsIgnoreCase (from);

            if (! DecryptedPatches::Store::shared().rename (from, to))
                return false;

            if (onIt)
                synth.setPatchName (to.trim());

            repaint();
            if (onOwnChanged != nullptr) onOwnChanged();
            return true;
        }

        /** Deletes a Decrypted patch; the synth keeps the sound it is on. */
        bool remove (const juce::String& name)
        {
            if (! DecryptedPatches::Store::shared().remove (name))
                return false;

            repaint();
            if (onOwnChanged != nullptr) onOwnChanged();
            return true;
        }

        void askRename()
        {
            const auto from = synth.getPatchName();
            auto* box = new juce::AlertWindow ("Rename Patch", "A new name for " + from.quoted() + ".", juce::MessageBoxIconType::NoIcon, this);
            box->addTextEditor ("name", from, "Name");
            box->addButton ("Rename", 1, juce::KeyPress (juce::KeyPress::returnKey));
            box->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
            box->enterModalState (true, juce::ModalCallbackFunction::create ([safe = juce::Component::SafePointer<PatchBrowser> (this), box, from] (int result)
            {
                if (safe == nullptr || result != 1)
                    return;

                const auto to = box->getTextEditorContents ("name").trim();

                if (const auto why = DecryptedPatches::Store::shared().whyNot (to, from); why.isNotEmpty() || ! safe->rename (from, to))
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Rename Patch",
                                                            why.isNotEmpty() ? why + " " + from.quoted() + " keeps its name." : from.quoted() + " couldn't be renamed.");
            }), true);
        }

        void askDelete()
        {
            const auto name = synth.getPatchName();
            juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon, "Delete Patch", "Delete " + name.quoted() + " from your Decrypted patches? The synth keeps the sound it is on.",
                                                "Delete", "Cancel", this,
                                                juce::ModalCallbackFunction::create ([safe = juce::Component::SafePointer<PatchBrowser> (this), name] (int result)
                                                {
                                                    if (safe != nullptr && result == 1)
                                                        safe->remove (name);
                                                }));
        }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (0.5f);
            g.setColour (Theme::inset);
            g.fillRoundedRectangle (r, 6.0f);
            g.setColour (hover == 0 ? Theme::accent.withAlpha (0.7f) : Theme::line);
            g.drawRoundedRectangle (r, 6.0f, 1.0f);
            const auto arrow = [&] (juce::Rectangle<float> a, bool right, bool hot)
            {
                juce::Path p;
                const auto c = a.getCentre();
                if (right) p.addTriangle (c.x - 3.0f, c.y - 5.0f, c.x - 3.0f, c.y + 5.0f, c.x + 4.0f, c.y);
                else p.addTriangle (c.x + 3.0f, c.y - 5.0f, c.x + 3.0f, c.y + 5.0f, c.x - 4.0f, c.y);
                g.setColour (hot ? Theme::accent : Theme::dimText);
                g.fillPath (p);
            };
            arrow (r.removeFromLeft (24.0f), false, hover == -1);
            arrow (r.removeFromRight (24.0f), true, hover == 1);
            const auto index = currentIndex();
            const auto favourite = index >= 0 && WaveSynth::Favourites::shared().contains (synth.getPatchName());
            const auto category = favourite && synth.isBrowsingFavourites() ? juce::String ("Favourites")
                                : index >= 0 ? WaveSynth::factoryPatches()[(size_t) index].category
                                : onDecrypted() ? juce::String ("Decrypted")
                                : complexIndex() >= 0 ? WaveSynth::complexPatches()[(size_t) complexIndex()].category : juce::String ("Patch");

            // The heart: filled when the patch is a favourite, an outline when it could be one.
            if (index >= 0)
            {
                const auto box = heartBounds().withSizeKeepingCentre (14.0f, 13.0f);
                juce::Path heart;
                heart.startNewSubPath (box.getCentreX(), box.getBottom());
                heart.cubicTo (box.getX() - 2.0f, box.getY() + box.getHeight() * 0.55f, box.getX() + 1.0f, box.getY() - 3.0f, box.getCentreX(), box.getY() + box.getHeight() * 0.3f);
                heart.cubicTo (box.getRight() - 1.0f, box.getY() - 3.0f, box.getRight() + 2.0f, box.getY() + box.getHeight() * 0.55f, box.getCentreX(), box.getBottom());
                heart.closeSubPath();

                if (favourite)
                {
                    g.setColour (Theme::accent);
                    g.fillPath (heart);
                }
                else
                {
                    g.setColour (hover == 3 ? Theme::accent : Theme::dimText.withAlpha (0.7f));
                    g.strokePath (heart, juce::PathStrokeType (1.3f));
                }

                r.removeFromRight (26.0f);
            }

            g.setFont (bold (9.0f).withExtraKerningFactor (0.1f));
            g.setColour (Theme::accentLight);
            g.drawText (category.toUpperCase(), r.removeFromLeft (juce::jmin (92.0f, r.getWidth() * 0.32f)), juce::Justification::centredLeft);
            g.setFont (bold (14.0f));
            g.setColour (Theme::textStrong);
            g.drawFittedText (synth.getPatchName(), r.toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
        }

        void mouseMove (const juce::MouseEvent& e) override
        {
            const auto h = e.position.x < 24.0f ? -1 : e.position.x > (float) getWidth() - 24.0f ? 1 : heartBounds().contains (e.position) ? 3 : 0;
            if (h != hover) { hover = h; repaint(); }
        }

        void mouseExit (const juce::MouseEvent&) override { hover = 2; repaint(); }

        void mouseDown (const juce::MouseEvent& e) override
        {
            const auto& all = WaveSynth::factoryPatches();

            // The arrows: through what the arrow keys go through too (the synth's own list).
            if (e.position.x < 24.0f || e.position.x > (float) getWidth() - 24.0f)
            {
                synth.stepPatch (e.position.x < 24.0f ? -1 : 1);
                repaint();
                if (onLoaded != nullptr) onLoaded();
                return;
            }

            if (heartBounds().contains (e.position) && currentIndex() >= 0)
            {
                toggleFavourite();
                return;
            }

            constexpr int decryptedIds = 2000000, renameId = 2900001, deleteId = 2900002, importMassiveXId = 2900003, complexIds = 3000000;

            // A right-click on one of the user's own: its name and whether it stays.
            if (e.mods.isPopupMenu())
            {
                if (! onDecrypted())
                    return;

                juce::PopupMenu own;
                own.addItem (renameId, "Rename " + synth.getPatchName().quoted() + "...");
                own.addItem (deleteId, "Delete " + synth.getPatchName().quoted() + "...");
                own.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe = juce::Component::SafePointer<PatchBrowser> (this)] (int chosen)
                {
                    if (safe != nullptr && chosen == renameId)  safe->askRename();
                    if (safe != nullptr && chosen == deleteId)  safe->askDelete();
                });
                return;
            }

            // The favourites first, by name; then the patches taken from Decrypt; then a sub-menu for each category, a large category's a list of pages by name.
            juce::PopupMenu menu;
            const auto current = synth.getPatchName();
            constexpr int favouriteIds = 1000000;

            {
                auto ticked = false;
                const auto favourites = WaveSynthEditor::favouritesMenu (current, synth.isBrowsingFavourites(), favouriteIds + 1, ticked);
                menu.addSubMenu ("Favourites", favourites, true, nullptr, ticked);
                auto decryptedTicked = false;
                const auto decrypted = WaveSynthEditor::decryptedMenu (onDecrypted() ? current : juce::String(), decryptedIds + 1, renameId, deleteId, decryptedTicked);
                menu.addSubMenu ("Decrypted", decrypted, true, nullptr, decryptedTicked);

                // The Complex patches: four oscillators or more each, a sub-menu for each category.
                juce::PopupMenu complexMenu;
                auto complexTicked = false;
                const auto& complex = WaveSynth::complexPatches();

                for (auto& c : WaveSynth::patchCategories())
                {
                    juce::PopupMenu category;
                    auto ticked = false;

                    for (size_t i = 0; i < complex.size(); ++i)
                        if (complex[i].category == c)
                        {
                            ticked = ticked || complex[i].name == current;
                            category.addItem (complexIds + 1 + (int) i, complex[i].name, true, complex[i].name == current);
                        }

                    if (category.getNumItems() > 0)
                        complexMenu.addSubMenu (c, category, true, nullptr, ticked);

                    complexTicked = complexTicked || ticked;
                }

                menu.addSubMenu ("Complex", complexMenu, true, nullptr, complexTicked);
                menu.addSeparator();
                menu.addItem (importMassiveXId, "Import Massive X Patch (.nksf)...");
                menu.addSeparator();
            }

            for (auto& c : WaveSynth::patchCategories())
            {
                const auto pages = WaveSynthEditor::menuPages (c);
                juce::PopupMenu category;
                auto holdsCurrent = false;

                for (auto& page : pages)
                {
                    juce::PopupMenu items;
                    auto ticked = false;

                    for (auto i : page.patches)
                    {
                        const auto& name = all[(size_t) i].name;
                        ticked = ticked || name == current;
                        (pages.size() == 1 ? category : items).addItem (i + 1, name, true, name == current);
                    }

                    holdsCurrent = holdsCurrent || ticked;

                    if (pages.size() > 1)
                        category.addSubMenu (page.label, items, true, nullptr, ticked);
                }

                if (! pages.empty())
                    menu.addSubMenu (c, category, true, nullptr, holdsCurrent);
            }

            // On one of the user's Decrypted patches, the menu opens on it.
            auto openOn = 0;

            if (onDecrypted())
            {
                const auto order = DecryptedPatches::Store::shared().inFolderOrder();

                for (size_t i = 0; i < order.size(); ++i)
                    if (order[i].name.equalsIgnoreCase (current))
                        openOn = decryptedIds + 1 + (int) i;
            }
            juce::Component::SafePointer<PatchBrowser> safe (this);
            menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withInitiallySelectedItem (openOn), [safe] (int chosen)
            {
                if (safe == nullptr || chosen <= 0)
                    return;

                if (chosen == renameId)     { safe->askRename(); return; }
                if (chosen == deleteId)     { safe->askDelete(); return; }
                if (chosen == importMassiveXId) { safe->askImportMassiveX(); return; }

                // A Complex patch: the arrows then go through those.
                if (chosen > complexIds)
                {
                    const auto& complex = WaveSynth::complexPatches();

                    if (juce::isPositiveAndBelow (chosen - complexIds - 1, (int) complex.size())
                        && safe->synth.loadPatch (complex[(size_t) (chosen - complexIds - 1)].name))
                    {
                        safe->synth.setBrowsingComplex (true);
                        safe->repaint();
                        if (safe->onLoaded != nullptr) safe->onLoaded();
                    }

                    return;
                }

                // One of the user's Decrypted patches: the arrows then go through those.
                if (chosen > decryptedIds)
                {
                    const auto order = DecryptedPatches::Store::shared().inFolderOrder();

                    if (juce::isPositiveAndBelow (chosen - decryptedIds - 1, (int) order.size())
                        && DecryptedPatches::Store::shared().load (safe->synth, order[(size_t) (chosen - decryptedIds - 1)].name))
                    {
                        safe->synth.setBrowsingDecrypted (true);
                        safe->repaint();
                        if (safe->onLoaded != nullptr) safe->onLoaded();
                    }

                    return;
                }

                safe->synth.setBrowsingDecrypted (false);
                safe->synth.setBrowsingComplex (false);

                // A patch chosen from the favourites puts the arrows on the favourites; one from a category, on every patch.
                safe->synth.setBrowsingFavourites (chosen > favouriteIds);
                safe->load (chosen > favouriteIds ? chosen - favouriteIds - 1 : chosen - 1);
            });
        }

        void load (int index)
        {
            synth.loadPatch (index);
            repaint();
            if (onLoaded != nullptr) onLoaded();
        }

        void askImportMassiveX()
        {
            const auto startDir = juce::File ("/Users/Shared/Massive X Factory Library/Presets").isDirectory()
                                ? juce::File ("/Users/Shared/Massive X Factory Library/Presets")
                                : juce::File::getSpecialLocation (juce::File::userHomeDirectory);
            importChooser = std::make_unique<juce::FileChooser> ("Import Massive X Patch (.nksf)", startDir, "*.nksf");
            juce::Component::SafePointer<PatchBrowser> safe (this);
            importChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                        [safe] (const juce::FileChooser& chooser)
            {
                if (safe == nullptr) return;
                const auto chosen = chooser.getResult();
                if (! chosen.existsAsFile()) return;
                if (auto* ed = safe->findParentComponentOfClass<WaveSynthEditor>())
                    ed->importMassiveXFile (chosen, true);
            });
        }

    private:
        std::unique_ptr<juce::FileChooser> importChooser;
        int currentIndex() const
        {
            const auto& all = WaveSynth::factoryPatches();

            for (size_t i = 0; i < all.size(); ++i)
                if (all[i].name == synth.getPatchName())
                    return (int) i;

            return -1;
        }

        /** The Complex patch the synth is on, by name; -1 on any other. */
        int complexIndex() const
        {
            const auto& complex = WaveSynth::complexPatches();

            for (size_t i = 0; i < complex.size(); ++i)
                if (complex[i].name == synth.getPatchName())
                    return (int) i;

            return -1;
        }

        Synth& synth;
        int hover = 2;
    };
}

//==============================================================================
juce::PopupMenu WaveSynthEditor::favouritesMenu (const juce::String& current, bool browsing, int firstId, bool& ticked)
{
    const auto& all = WaveSynth::factoryPatches();
    const auto order = WaveSynth::Favourites::shared().inTreeOrder();
    juce::PopupMenu favourites;
    ticked = false;

    // inTreeOrder keeps each category's favourites together, in the tree's order of categories.
    for (size_t from = 0; from < order.size();)
    {
        const auto& category = all[(size_t) order[from]].category;
        juce::PopupMenu items;
        auto holdsCurrent = false;
        auto to = from;

        for (; to < order.size() && all[(size_t) order[to]].category == category; ++to)
        {
            const auto& name = all[(size_t) order[to]].name;
            const auto isCurrent = browsing && name == current;
            holdsCurrent = holdsCurrent || isCurrent;
            items.addItem (firstId + order[to], name, true, isCurrent);
        }

        favourites.addSubMenu (category, items, true, nullptr, holdsCurrent);
        ticked = ticked || holdsCurrent;
        from = to;
    }

    if (order.empty())
        favourites.addItem (-1, "None yet: click the heart beside a patch's name", false);

    return favourites;
}

juce::PopupMenu WaveSynthEditor::decryptedMenu (const juce::String& current, int firstId, int renameId, int deleteId, bool& ticked)
{
    // In the folder's order (DecryptedPatches::Store::inFolderOrder): a sub-menu for each category that holds one, then those with no category.
    const auto order = DecryptedPatches::Store::shared().inFolderOrder();
    juce::PopupMenu decrypted;
    ticked = false;

    for (size_t from = 0; from < order.size();)
    {
        const auto& category = order[from].category;
        juce::PopupMenu items;
        auto holdsCurrent = false;
        auto to = from;

        for (; to < order.size() && order[to].category == category; ++to)
        {
            const auto isCurrent = current.isNotEmpty() && order[to].name.equalsIgnoreCase (current);
            holdsCurrent = holdsCurrent || isCurrent;
            (category.isEmpty() ? decrypted : items).addItem (firstId + (int) to, order[to].name, true, isCurrent);
        }

        if (category.isNotEmpty())
            decrypted.addSubMenu (category, items, true, nullptr, holdsCurrent);

        ticked = ticked || holdsCurrent;
        from = to;
    }

    if (order.empty())
        decrypted.addItem (-1, "None yet: Take a patch in Decrypt and give it a name", false);

    if (ticked)
    {
        decrypted.addSeparator();
        decrypted.addItem (renameId, "Rename " + current.quoted() + "...");
        decrypted.addItem (deleteId, "Delete " + current.quoted() + "...");
    }

    return decrypted;
}

std::vector<WaveSynthEditor::MenuPage> WaveSynthEditor::menuPages (const juce::String& category)
{
    const auto& all = WaveSynth::factoryPatches();
    std::vector<int> in;

    for (size_t i = 0; i < all.size(); ++i)
        if (all[i].category == category)
            in.push_back ((int) i);

    if (in.empty())
        return {};

    if ((int) in.size() <= mostInAPage)
        return { { {}, in } };

    std::sort (in.begin(), in.end(), [&all] (int a, int b) { return all[(size_t) a].name.compareIgnoreCase (all[(size_t) b].name) < 0; });
    const auto count = ((int) in.size() + mostInAPage - 1) / mostInAPage;
    std::vector<MenuPage> pages;

    for (int p = 0; p < count; ++p)
    {
        const auto from = (int) in.size() * p / count, to = (int) in.size() * (p + 1) / count;
        const auto& first = all[(size_t) in[(size_t) from]].name;
        const auto& last = all[(size_t) in[(size_t) to - 1]].name;
        pages.push_back ({ first.substring (0, 2) + " - " + last.substring (0, 2), std::vector<int> (in.begin() + from, in.begin() + to) });
    }

    return pages;
}

//==============================================================================
struct WaveSynthEditor::Content final : public juce::Component
{
    explicit Content (Synth& s)
        : synth (s),
          presets (EffectPresets::userStore(), s.getPresetKey(), [&s] { return s.getPresetValues(); },
                   [&s] (const EffectPresets::Values& v) { s.setPresetValues (v); }),
          browser (s), modPages ({ "Slots 1-8", "Slots 9-16", "Slots 17-24", "Effects" }), viewPages ({ "Knobs", "Decrypt", "Decrypted" }),
          envelopeView (s), lfoView (s)
    {
        addAndMakeVisible (browser);
        addAndMakeVisible (presets);
        presets.carryTexts ([&s] { return s.getPresetTexts(); }, [&s] (const EffectPresets::Texts& t) { s.setPresetTexts (t); });

        // The oscillators: all nine, three at a time in the panels, the strip above choosing which.
        addAndMakeVisible (strip);
        strip.onPage = [this] (int page) { showOscillatorPage (page); };

        for (int o = 0; o < WaveSynth::numOscillators; ++o)
        {
            const auto n = "osc" + juce::String (o + 1);
            auto& osc = oscillators[(size_t) o];
            osc.panel = std::make_unique<Panel> ("Oscillator " + juce::String (o + 1));
            addAndMakeVisible (*osc.panel);
            osc.on = toggle (*osc.panel, n + "On", "On");
            osc.table = selector (*osc.panel, n + "Table");
            osc.view = std::make_unique<WavetableView> (synth, o);
            osc.panel->addAndMakeVisible (*osc.view);
            osc.position = knob (*osc.panel, n + "Position", "Position", Knob::Size::large);
            osc.warpAmount = knob (*osc.panel, n + "WarpAmount", "Warp");
            osc.warp = selector (*osc.panel, n + "Warp");
            osc.fmFrom = selector (*osc.panel, n + "FmFrom");
            osc.fmFrom->setTooltip ("FM From: the oscillator the FM warp takes. Before: the one before it (Osc 1 takes Osc 3), as it always was. "
                                    "One named here is worked out even when off or silent, to drive this FM, and is never heard for it");
            osc.random = toggle (*osc.panel, n + "RandomPhase", "Rnd");
            osc.random->setTooltip ("Each note's unison voices start at random phases (on), or all at Phase (off)");

            for (auto [id, label] : { std::pair { "Pitch", "Pitch" }, { "Fine", "Fine" }, { "Volume", "Level" }, { "Pan", "Pan" }, { "Route", "F1 > F2" }, { "Phase", "Phase" },
                                      { "Unison", "Unison" }, { "UnisonDetune", "Detune" }, { "UnisonSpread", "Spread" }, { "UnisonStrum", "Strum" }, { "UnisonDrift", "Drift" } })
                osc.knobs.push_back (knob (*osc.panel, n + id, label, Knob::Size::small));

            osc.knobs[0]->setBipolar (true);
            osc.knobs[1]->setBipolar (true);
            osc.knobs[3]->setBipolar (true);
            osc.knobs[6]->setFormatter ([] (float v) { const auto count = juce::roundToInt (v); return juce::String (count) + (count == 1 ? " voice" : " voices"); });
            osc.knobs[4]->setTooltip ("Which filter this oscillator goes to: Filter 1 at the left, Filter 2 at the right");
            osc.knobs[9]->setTooltip ("Starts the unison voices apart, like a pick across strings");
            osc.knobs[10]->setTooltip ("Each unison voice wanders slowly in pitch on its own");
        }

        // Sub and noise.
        sourcesPanel = std::make_unique<Panel> ("Sub + Noise");
        addAndMakeVisible (*sourcesPanel);
        subLevel = knob (*sourcesPanel, "subLevel", "Sub");
        subOctave = selector (*sourcesPanel, "subOctave");
        subShape = selector (*sourcesPanel, "subShape");
        subDirect = toggle (*sourcesPanel, "subDirect", "Direct");
        subDirect->setTooltip ("The sub skips the filters, staying clean and centred");
        noiseLevel = knob (*sourcesPanel, "noiseLevel", "Noise");
        noiseColour = knob (*sourcesPanel, "noiseColour", "Colour");
        noiseColour->setBipolar (true);
        noiseType = selector (*sourcesPanel, "noiseType");

        // The filters, and how they are joined.
        for (int f = 0; f < 2; ++f)
        {
            const auto n = "filter" + juce::String (f + 1);
            auto& fl = filters[(size_t) f];
            fl.panel = std::make_unique<Panel> ("Filter " + juce::String (f + 1));
            addAndMakeVisible (*fl.panel);
            fl.type = selector (*fl.panel, n + "Type");
            fl.view = std::make_unique<FilterView> (synth, f);
            fl.panel->addAndMakeVisible (*fl.view);
            fl.cutoff = knob (*fl.panel, n + "Cutoff", "Cutoff", Knob::Size::large);
            fl.resonance = knob (*fl.panel, n + "Resonance", "Res");
            fl.drive = knob (*fl.panel, n + "Drive", "Drive");
            fl.key = knob (*fl.panel, n + "KeyTrack", "Key");
        }

        routingPanel = std::make_unique<Panel> ("Route");
        addAndMakeVisible (*routingPanel);
        routing = selector (*routingPanel, "filterRouting");

        // Amp and voice.
        ampPanel = std::make_unique<Panel> ("Amp + Voice");
        addAndMakeVisible (*ampPanel);
        ampLevel = knob (*ampPanel, "ampLevel", "Level", Knob::Size::large);
        ampPan = knob (*ampPanel, "ampPan", "Pan");
        ampPan->setBipolar (true);
        velocity = knob (*ampPanel, "ampVelocity", "Velocity");
        glide = knob (*ampPanel, "glideTime", "Glide");
        bendRange = knob (*ampPanel, "bendRange", "Bend");
        voiceMode = selector (*ampPanel, "voiceMode");
        glideMode = selector (*ampPanel, "glideMode");

        // The modulators: envelopes and LFOs behind tabs that are also their sources.
        modulatorPanel = std::make_unique<Panel> ("Envelopes + LFOs");
        addAndMakeVisible (*modulatorPanel);
        const juce::StringArray tabNames { "Amp", "Env 2", "Env 3", "Env 4", "LFO 1", "LFO 2", "LFO 3", "LFO 4" };

        for (int i = 0; i < oscLevelsPage; ++i)
        {
            const auto source = i < WaveSynth::numEnvelopes ? WaveSynth::env1 + i : WaveSynth::lfoSource (i - WaveSynth::numEnvelopes);
            auto tab = std::make_unique<SourceTab> (source, tabNames[i]);
            tab->onClick = [this, i] { showModulator (i); };
            modulatorPanel->addAndMakeVisible (*tab);
            tabs.push_back (std::move (tab));
        }

        levelsTab = std::make_unique<PlainTab> ("Osc Levels");
        levelsTab->setTooltip ("Each oscillator's own level envelope: attack, hold, decay and sustain on its level, so layers come and go by themselves");
        levelsTab->onClick = [this] { showModulator (oscLevelsPage); };
        modulatorPanel->addAndMakeVisible (*levelsTab);

        for (int o = 0; o < WaveSynth::numOscillators; ++o)
        {
            const auto n = "osc" + juce::String (o + 1) + "Env";
            LevelControls c;
            c.on = toggle (*modulatorPanel, n + "On", "Osc " + juce::String (o + 1));
            c.on->setTooltip ("Oscillator " + juce::String (o + 1) + "'s level envelope. Off: its level is the Level knob's, as before");

            for (auto [id, label] : { std::pair { "Attack", "Attack" }, { "Hold", "Hold" }, { "Decay", "Decay" }, { "Sustain", "Sustain" } })
                c.knobs.push_back (knob (*modulatorPanel, n + id, label, Knob::Size::small));

            levels.push_back (c);
        }

        modulatorPanel->addAndMakeVisible (envelopeView);
        modulatorPanel->addAndMakeVisible (lfoView);

        for (int e = 0; e < WaveSynth::numEnvelopes; ++e)
        {
            const auto n = "env" + juce::String (e + 1);
            std::vector<Knob*> k;
            for (auto [id, label] : { std::pair { "Attack", "Attack" }, { "Decay", "Decay" }, { "Sustain", "Sustain" }, { "Release", "Release" }, { "Curve", "Curve" } })
                k.push_back (knob (*modulatorPanel, n + id, label, Knob::Size::small));
            k[4]->setBipolar (true);
            k[4]->setColour (sourceColour (e));
            envelopeKnobs.push_back (k);
        }

        for (int l = 0; l < WaveSynth::numLfos; ++l)
        {
            const auto n = "lfo" + juce::String (l + 1);
            LfoControls c;
            c.shape = selector (*modulatorPanel, n + "Shape");
            c.mode = selector (*modulatorPanel, n + "Mode");
            c.division = selector (*modulatorPanel, n + "Division");
            c.sync = toggle (*modulatorPanel, n + "Sync", "Sync");
            c.rate = knob (*modulatorPanel, n + "Rate", "Rate", Knob::Size::small);
            c.phase = knob (*modulatorPanel, n + "Phase", "Phase", Knob::Size::small);
            c.fade = knob (*modulatorPanel, n + "Fade", "Fade In", Knob::Size::small);
            c.smooth = knob (*modulatorPanel, n + "Smooth", "Smooth", Knob::Size::small);
            for (auto* k : { c.rate, c.phase, c.fade, c.smooth }) k->setColour (sourceColour (WaveSynth::lfoSource (l) - 1));
            lfos.push_back (c);
        }

        // The performance sources, for dragging.
        for (auto [source, name] : { std::pair { (int) WaveSynth::velocity, "Velocity" }, { (int) WaveSynth::modWheel, "Wheel" }, { (int) WaveSynth::aftertouch, "Pressure" },
                                     { (int) WaveSynth::key, "Key" }, { (int) WaveSynth::random, "Random" } })
        {
            auto chip = std::make_unique<ModSourceChip> (source, name);
            modulatorPanel->addAndMakeVisible (*chip);
            performanceChips.push_back (std::move (chip));
        }

        // The macros.
        macroPanel = std::make_unique<Panel> ("Macros");
        addAndMakeVisible (*macroPanel);

        for (int m = 0; m < WaveSynth::numMacros; ++m)
        {
            macroKnobs.push_back (knob (*macroPanel, "macro" + juce::String (m + 1), "Macro " + juce::String (m + 1)));
            macroKnobs.back()->setColour (sourceColour (WaveSynth::macro1 - 1 + m));
            auto chip = std::make_unique<ModSourceChip> (WaveSynth::macro1 + m, juce::String (m + 1));
            macroPanel->addAndMakeVisible (*chip);
            macroChips.push_back (std::move (chip));
        }

        // The slots and the effects, a page each.
        rightPanel = std::make_unique<Panel> ("");
        addAndMakeVisible (*rightPanel);
        rightPanel->addAndMakeVisible (modPages);
        modPages.onChange = [this] (int i) { showRightPage (i); };

        for (int slot = 0; slot < WaveSynth::numSlots; ++slot)
        {
            const auto n = "mod" + juce::String (slot + 1);
            SlotControls c;
            c.source = selector (*rightPanel, n + "Source");
            c.destination = selector (*rightPanel, n + "Destination");
            c.destination->setSections (WaveSynth::destinationSections());
            c.amount = knob (*rightPanel, n + "Amount", "", Knob::Size::small);
            c.amount->setBipolar (true);
            slots.push_back (c);
        }

        const auto fx = [this] (const juce::String& title, const juce::String& onId)
        {
            FxModule m;
            m.panel = std::make_unique<Panel> (title);
            rightPanel->addAndMakeVisible (*m.panel);
            m.on = toggle (*m.panel, onId, "");
            return m;
        };

        effects.push_back (fx ("Drive", "fxDriveOn"));
        effects.back().selector = selector (*effects.back().panel, "fxDriveType");
        for (auto [id, label] : { std::pair { "fxDriveAmount", "Amount" }, { "fxDriveTone", "Tone" }, { "fxDriveMix", "Mix" } })
            effects.back().knobs.push_back (knob (*effects.back().panel, id, label, Knob::Size::small));

        effects.push_back (fx ("Chorus", "fxChorusOn"));
        for (auto [id, label] : { std::pair { "fxChorusRate", "Rate" }, { "fxChorusDepth", "Depth" }, { "fxChorusMix", "Mix" } })
            effects.back().knobs.push_back (knob (*effects.back().panel, id, label, Knob::Size::small));

        effects.push_back (fx ("Phaser", "fxPhaserOn"));
        for (auto [id, label] : { std::pair { "fxPhaserRate", "Rate" }, { "fxPhaserFeedback", "Feedback" }, { "fxPhaserMix", "Mix" } })
            effects.back().knobs.push_back (knob (*effects.back().panel, id, label, Knob::Size::small));

        effects.push_back (fx ("Delay", "fxDelayOn"));
        effects.back().selector = selector (*effects.back().panel, "fxDelayDivision");
        effects.back().extra = toggle (*effects.back().panel, "fxDelayPingPong", "Ping-Pong");
        for (auto [id, label] : { std::pair { "fxDelayFeedback", "Feedback" }, { "fxDelayTone", "Tone" }, { "fxDelayMix", "Mix" } })
            effects.back().knobs.push_back (knob (*effects.back().panel, id, label, Knob::Size::small));

        effects.push_back (fx ("Reverb", "fxReverbOn"));
        for (auto [id, label] : { std::pair { "fxReverbSize", "Size" }, { "fxReverbDecay", "Decay" }, { "fxReverbDamping", "Damping" }, { "fxReverbMix", "Mix" } })
            effects.back().knobs.push_back (knob (*effects.back().panel, id, label, Knob::Size::small));

        browser.onLoaded = [this] { refresh(); };
        browser.onOwnChanged = [this] { presets.refresh(); decryptedView->refresh(); };
        // Decrypt takes the place of the knobs when its page is chosen. Trace's scope is not offered in the
        // window (it was withdrawn from 0.20.0); it stays for its tests.
        consoleView = std::make_unique<SignalConsoleView> (synth);
        addChildComponent (*consoleView);
        decryptView = std::make_unique<DecryptView> (synth);
        decryptView->onPatchSaved = [this] { presets.refresh(); browser.repaint(); decryptedView->refresh(); };
        decryptedView = std::make_unique<DecryptedView> (synth);
        addChildComponent (*decryptedView);
        decryptedView->onOpen = [this] { showView (0); };
        decryptedView->onLoaded = [this] { refresh(); browser.repaint(); };
        decryptedView->onChanged = [this] { presets.refresh(); browser.repaint(); };
        addChildComponent (*decryptView);
        addAndMakeVisible (viewPages);
        viewPages.setTooltip ("Knobs: the synth's controls. Decrypt: drop a recording of one note and find a patch that plays like it. Decrypted: the patches you took from Decrypt and named");
        viewPages.onChange = [this] (int i) { showView (i == 1 ? 2 : i == 2 ? 3 : 0); };

        showOscillatorPage (0);
        showModulator (0);
        showRightPage (0);
        setSize (designWidth, designHeight);
        refresh();
    }

    //==========================================================================
    Knob* knob (juce::Component& parent, const juce::String& id, const juce::String& label, Knob::Size size = Knob::Size::normal)
    {
        auto k = std::make_unique<Knob> (label, size);
        k->attach (synth.parameter (id));
        auto* raw = k.get();
        const auto destination = WaveSynth::destinationIds().indexOf (id);

        if (destination >= 0)
        {
            raw->onModulationDropped = [this, id] (int source) { synth.addModulation (source, id); refresh(); };
            raw->onRingDepthChanged = [this] (int slot, float depth) { synth.setModulationAmount (slot, depth); };
            raw->onRingRemoved = [this] (int slot) { synth.removeModulation (slot); refresh(); };
            raw->setTooltip (WaveSynth::destinationNames()[destination] + ": drag a source here to modulate it");
            destinationKnobs.push_back ({ raw, destination });
        }

        knobsById[id] = raw;
        parent.addAndMakeVisible (*raw);
        allKnobs.push_back (std::move (k));
        return raw;
    }

    StepSelector* selector (juce::Component& parent, const juce::String& id)
    {
        auto s = std::make_unique<StepSelector>();
        s->attach (dynamic_cast<juce::AudioParameterChoice*> (synth.parameter (id)));
        s->setTooltip (synth.parameter (id)->getName (64));
        auto* raw = s.get();
        parent.addAndMakeVisible (*raw);
        allSelectors.push_back (std::move (s));
        return raw;
    }

    LedToggle* toggle (juce::Component& parent, const juce::String& id, const juce::String& text)
    {
        auto t = std::make_unique<LedToggle> (text);
        t->attach (synth.parameter (id));
        t->setTooltip (synth.parameter (id)->getName (64));
        auto* raw = t.get();
        parent.addAndMakeVisible (*raw);
        allToggles.push_back (std::move (t));
        return raw;
    }

    //==========================================================================
    void showModulator (int index)
    {
        modulator = juce::jlimit (0, oscLevelsPage, index);

        for (int i = 0; i < (int) tabs.size(); ++i)
            tabs[(size_t) i]->setHighlighted (i == modulator);

        levelsTab->setHighlighted (modulator == oscLevelsPage);
        const auto isEnvelope = modulator < 4, isLfo = modulator >= 4 && modulator < oscLevelsPage;
        envelopeView.setVisible (isEnvelope);
        lfoView.setVisible (isLfo);

        if (isEnvelope)
            envelopeView.setEnvelope (modulator);
        else if (isLfo)
            lfoView.setLfo (modulator - 4);

        for (int o = 0; o < (int) levels.size(); ++o)
        {
            const auto shown = modulator == oscLevelsPage && onPage (o);
            levels[(size_t) o].on->setVisible (shown);
            for (auto* k : levels[(size_t) o].knobs) k->setVisible (shown);
        }

        for (int e = 0; e < (int) envelopeKnobs.size(); ++e)
            for (auto* k : envelopeKnobs[(size_t) e])
                k->setVisible (e == modulator);

        for (int l = 0; l < (int) lfos.size(); ++l)
            for (juce::Component* c : lfos[(size_t) l].all())
                c->setVisible (l == modulator - 4);

        updateLfoVisibility();
        resized();
    }

    /** Oscillators 1 to 3 (0), 4 to 6 (1) or 7 to 9 (2) in the three panels, and in the Osc Levels page. */
    void showOscillatorPage (int page)
    {
        oscPage = juce::jlimit (0, WaveSynth::numOscillators / WaveSynth::numFirstOscillators - 1, page);
        strip.setPage (oscPage);

        if (strip.isVisible())
            for (int o = 0; o < WaveSynth::numOscillators; ++o)
                oscillators[(size_t) o].panel->setVisible (onPage (o));

        showModulator (modulator);
    }

    bool onPage (int oscillator) const    { return oscillator / WaveSynth::numFirstOscillators == oscPage; }

    /** The knobs (0), Trace's scope (1, for its tests), Decrypt (2) or the Decrypted patches (3) under the header. */
    void showView (int index)
    {
        const auto trace = index == 1, decrypt = index == 2, decrypted = index == 3;
        viewPages.chosen = decrypt ? 1 : decrypted ? 2 : 0;
        viewPages.repaint();

        for (auto* c : getChildren())
            if (c != &browser && c != &presets && c != &viewPages && c != consoleView.get() && c != decryptView.get() && c != decryptedView.get())
                c->setVisible (! trace && ! decrypt && ! decrypted);

        consoleView->setVisible (trace);
        decryptView->setVisible (decrypt);
        decryptedView->setVisible (decrypted);

        if (! trace && ! decrypt && ! decrypted)
        {
            // The pages within the knobs show what they showed.
            for (int o = 0; o < WaveSynth::numOscillators; ++o)
                oscillators[(size_t) o].panel->setVisible (onPage (o));

            showModulator (modulator);
            showRightPage (modPages.chosen);
        }
    }

    void showRightPage (int index)
    {
        modPages.chosen = index;
        modPages.repaint();

        for (int i = 0; i < (int) slots.size(); ++i)
            for (juce::Component* c : { (juce::Component*) slots[(size_t) i].source, (juce::Component*) slots[(size_t) i].destination, (juce::Component*) slots[(size_t) i].amount })
                c->setVisible (index == i / 8);

        for (auto& e : effects)
            e.panel->setVisible (index == effectsPage);

        rightPanel->setTitle (index == effectsPage ? "Effects" : "Modulation Slots");
        rightPanel->repaint();
    }

    void updateLfoVisibility()
    {
        for (int l = 0; l < (int) lfos.size(); ++l)
        {
            const auto shown = l == modulator - 4;
            const auto sync = lfos[(size_t) l].sync->isOn();
            lfos[(size_t) l].rate->setVisible (shown && ! sync);
            lfos[(size_t) l].division->setVisible (shown && sync);
        }
    }

    //==========================================================================
    void refresh()
    {
        for (auto& k : allKnobs) k->refresh();
        for (auto& s : allSelectors) s->refresh();
        for (auto& t : allToggles) t->refresh();

        // A level envelope that is off has its knobs faint.
        for (auto& l : levels)
            for (auto* k : l.knobs)
                k->setAlpha (l.on->isOn() ? 1.0f : 0.4f);

        // The rings: every slot's source, depth and slot on the knob it reaches, and where the newest note has it.
        std::vector<std::vector<ModRing>> rings ((size_t) WaveSynth::destinationIds().size());

        for (int s = 0; s < WaveSynth::numSlots; ++s)
        {
            const auto n = "mod" + juce::String (s + 1);
            const auto source = param<juce::AudioParameterChoice> (synth, n + "Source")->getIndex();
            const auto destination = param<juce::AudioParameterChoice> (synth, n + "Destination")->getIndex() - 1;

            if (source > 0 && destination >= 0)
                rings[(size_t) destination].push_back ({ sourceColour (source - 1), plainValue (synth, n + "Amount") * 0.01f, s });

            // A slot's knob takes its source's colour.
            slots[(size_t) s].amount->setColour (source > 0 ? sourceColour (source - 1) : juce::Colour());
        }

        for (auto& [k, d] : destinationKnobs)
        {
            k->setRings (rings[(size_t) d]);
            k->setLiveValue (rings[(size_t) d].empty() ? -1.0f : synth.getReadings().destination[(size_t) d].load (std::memory_order_relaxed));
        }

        // What a string's knobs mean, and which controls a choice leaves idle.
        for (int o = 0; o < WaveSynth::numOscillators; ++o)
        {
            auto& osc = oscillators[(size_t) o];
            const auto string = osc.table->getIndex() == (int) WaveTables::Table::string;

            if (const auto fm = osc.warp->getIndex() == (int) WaveSynth::Warp::fm && ! string; fm != osc.fmFrom->isVisible())
            {
                osc.fmFrom->setVisible (fm);
                layoutOscillator (osc);
            }

            osc.position->setLabel (string ? "Bright" : "Position");
            osc.warpAmount->setLabel (string ? "Decay" : "Warp");
            osc.warp->setEnabled (! string);
            osc.view->update();
            osc.panel->setDimmed (! osc.on->isOn());
        }

        for (auto& f : filters)
        {
            const auto on = f.type->getIndex() != 0;
            for (auto* k : { f.cutoff, f.resonance, f.drive, f.key }) k->setEnabled (on);
            f.view->update();
            f.panel->setDimmed (! on);
        }

        for (auto& e : effects)
            e.panel->setDimmed (! e.on->isOn());

        for (int m = 0; m < WaveSynth::numMacros; ++m)
        {
            const auto& name = synth.getMacroName (m);
            macroKnobs[(size_t) m]->setLabel (name.isNotEmpty() ? name : "Macro " + juce::String (m + 1));
        }

        updateLfoVisibility();
        envelopeView.update();
        lfoView.update();
        strip.update();
        browser.repaint();
    }

    //==========================================================================
    void paint (juce::Graphics& g) override
    {
        const auto light = Theme::current().light;
        g.setGradientFill (juce::ColourGradient (Theme::base.brighter (light ? 0.0f : 0.03f), 0.0f, 0.0f, Theme::base.darker (light ? 0.02f : 0.25f), 0.0f, (float) getHeight(), false));
        g.fillAll();

        // Brand mark: "TORQ" in white/strong text, "WAVE" in official Torqwave orange (#ff5c1f),
        // followed by "SYNTH", with the signature orange bar underneath.
        auto header = getLocalBounds().removeFromTop (headerHeight).reduced (16, 0).toFloat();
        const auto font = bold (17.0f).withExtraKerningFactor (0.22f);
        const auto brandCol = Theme::brandOrange();
        juce::GlyphArrangement torqGlyphs, waveGlyphs, synthGlyphs;
        torqGlyphs.addLineOfText (font, "TORQ", header.getX(), header.getCentreY() + 6.0f);
        const auto torqW = torqGlyphs.getBoundingBox (0, -1, true).getWidth();
        waveGlyphs.addLineOfText (font, "WAVE", header.getX() + torqW + 3.0f, header.getCentreY() + 6.0f);
        const auto waveRight = waveGlyphs.getBoundingBox (0, -1, true).getRight();
        const auto firstWidth = waveRight - header.getX();
        g.setColour (Theme::textStrong);
        torqGlyphs.draw (g);
        g.setColour (brandCol);
        waveGlyphs.draw (g);
        synthGlyphs.addLineOfText (font, "SYNTH", waveRight + 10.0f, header.getCentreY() + 6.0f);
        g.setColour (Theme::dimText);
        synthGlyphs.draw (g);
        g.setColour (brandCol);
        g.fillRoundedRectangle (header.getX(), header.getCentreY() + 11.0f, firstWidth, 2.0f, 1.0f);
        g.setFont (plain (10.0f));
        g.setColour (Theme::dimText);
        g.drawText ("WAVETABLE", juce::Rectangle<float> (header.getX(), header.getCentreY() - 21.0f, 140.0f, 12.0f), juce::Justification::centredLeft);

        // Voices sounding, on the right.
        const auto sounding = synth.getReadings().sounding.load();
        auto right = header.removeFromRight (90.0f);
        g.setColour (sounding > 0 ? Theme::accentLight : Theme::faintText);
        g.setFont (bold (10.0f));
        g.drawText (juce::String (sounding) + (sounding == 1 ? " VOICE" : " VOICES"), right, juce::Justification::centredRight);
    }

    void resized() override
    {
        constexpr int gap = 8, margin = 12;
        auto r = getLocalBounds().reduced (margin, 0);
        auto header = r.removeFromTop (headerHeight);
        header.removeFromLeft (210);
        header.removeFromRight (100);
        browser.setBounds (header.removeFromLeft (340).withSizeKeepingCentre (340, 32));   // a little narrower than it was, for the third page in the switch
        header.removeFromLeft (gap * 2);
        viewPages.setBounds (header.removeFromRight (216).withSizeKeepingCentre (216, 24));
        header.removeFromRight (gap * 2);
        presets.setBounds (header.withSizeKeepingCentre (header.getWidth(), 28));
        r.removeFromBottom (margin);
        consoleView->setBounds (r);
        decryptView->setBounds (r);
        decryptedView->setBounds (r);

        // The strip of all nine, then the oscillators of its page, and the sources.
        strip.setBounds (r.removeFromTop (stripHeight));
        r.removeFromTop (gap);
        auto row = r.removeFromTop (236);
        const auto sourcesWidth = 196;
        const auto oscWidth = (row.getWidth() - sourcesWidth - gap * 3) / 3;

        for (int place = 0; place < WaveSynth::numFirstOscillators; ++place)
        {
            const auto bounds = row.removeFromLeft (oscWidth);
            row.removeFromLeft (gap);

            for (int o = place; o < WaveSynth::numOscillators; o += WaveSynth::numFirstOscillators)
            {
                oscillators[(size_t) o].panel->setBounds (bounds);
                layoutOscillator (oscillators[(size_t) o]);
            }
        }

        sourcesPanel->setBounds (row);
        {
            auto c = sourcesPanel->getContentBounds();
            auto sub = c.removeFromTop (c.getHeight() / 2);
            auto noise = c;
            subLevel->setBounds (sub.removeFromLeft (70).withSizeKeepingCentre (64, 80));
            sub.removeFromLeft (4);
            subOctave->setBounds (sub.removeFromTop (22).reduced (0, 1));
            sub.removeFromTop (5);
            subShape->setBounds (sub.removeFromTop (22).reduced (0, 1));
            sub.removeFromTop (5);
            subDirect->setBounds (sub.removeFromTop (22).withWidth (70).reduced (0, 1));
            noise.removeFromTop (6);
            auto knobs = noise.removeFromLeft (70);
            noiseLevel->setBounds (knobs.removeFromTop (knobs.getHeight()).withSizeKeepingCentre (64, 80));
            noise.removeFromLeft (4);
            noiseType->setBounds (noise.removeFromTop (22).reduced (0, 1));
            noiseColour->setBounds (noise.withSizeKeepingCentre (56, 66));
        }

        r.removeFromTop (gap);

        // The filters, the route between them, the amp and voice.
        row = r.removeFromTop (196);
        const auto filterWidth = 366;

        for (int f = 0; f < 2; ++f)
        {
            auto& fl = filters[(size_t) f];
            fl.panel->setBounds (row.removeFromLeft (filterWidth));
            row.removeFromLeft (gap);

            if (f == 0)
            {
                routingPanel->setBounds (row.removeFromLeft (92));
                row.removeFromLeft (gap);
                auto c = routingPanel->getContentBounds();
                routing->setBounds (c.removeFromTop (22).withSizeKeepingCentre (c.getWidth(), 22));
            }

            fl.type->setBounds (fl.panel->getHeaderBounds().removeFromRight (124).reduced (0, 1));
            auto c = fl.panel->getContentBounds();
            fl.view->setBounds (c.removeFromLeft (190));
            c.removeFromLeft (6);
            auto left = c.removeFromLeft (c.getWidth() / 2 + 6);
            fl.cutoff->setBounds (left.withSizeKeepingCentre (left.getWidth(), 104));
            const auto h = c.getHeight() / 3;
            fl.resonance->setBounds (c.removeFromTop (h).withSizeKeepingCentre (54, h));
            fl.drive->setBounds (c.removeFromTop (h).withSizeKeepingCentre (54, h));
            fl.key->setBounds (c.withSizeKeepingCentre (54, h));
        }

        ampPanel->setBounds (row);
        {
            auto c = ampPanel->getContentBounds();
            auto top = c.removeFromTop (100);
            ampLevel->setBounds (top.removeFromLeft (86).withSizeKeepingCentre (80, 96));
            const auto w = top.getWidth() / 3;
            ampPan->setBounds (top.removeFromLeft (w).withSizeKeepingCentre (w, 70));
            velocity->setBounds (top.removeFromLeft (w).withSizeKeepingCentre (w, 70));
            bendRange->setBounds (top.withSizeKeepingCentre (w, 70));
            c.removeFromTop (6);
            auto bottom = c.removeFromTop (60);
            voiceMode->setBounds (bottom.removeFromLeft (100).removeFromTop (22));
            bottom.removeFromLeft (8);
            glideMode->setBounds (bottom.removeFromLeft (110).removeFromTop (22));
            glide->setBounds (bottom.withSizeKeepingCentre (60, 60));
        }

        r.removeFromTop (gap);

        // Envelopes and LFOs, the macros, the slots or effects.
        row = r;
        modulatorPanel->setBounds (row.removeFromLeft (520));
        row.removeFromLeft (gap);
        macroPanel->setBounds (row.removeFromLeft (196));
        row.removeFromLeft (gap);
        rightPanel->setBounds (row);
        layoutModulators();
        layoutMacros();
        layoutRight();
    }

    template <typename Oscillator>
    void layoutOscillator (const Oscillator& osc)
    {
        auto header = osc.panel->getHeaderBounds();
        header.removeFromLeft (10);   // clear of the title's last letter
        osc.on->setBounds (header.removeFromLeft (44).reduced (0, 1));
        osc.table->setBounds (header.removeFromRight (112).reduced (0, 1));
        auto c = osc.panel->getContentBounds();
        auto top = c.removeFromTop (98);
        osc.view->setBounds (top.removeFromLeft (top.getWidth() - 126));
        top.removeFromLeft (6);
        auto knobsArea = top.removeFromTop (76);
        osc.position->setBounds (knobsArea.removeFromLeft (64));
        osc.warpAmount->setBounds (knobsArea.withSizeKeepingCentre (56, 66));
        auto warpRow = top.removeFromBottom (20);

        if (osc.fmFrom->isVisible())
            osc.fmFrom->setBounds (warpRow.removeFromRight (warpRow.getWidth() / 2 - 2));

        osc.warp->setBounds (warpRow.withTrimmedRight (osc.fmFrom->isVisible() ? 4 : 0));
        c.removeFromTop (4);
        const auto cell = c.getWidth() / 6;
        auto first = c.removeFromTop (c.getHeight() / 2);
        auto second = c;

        for (int i = 0; i < 6; ++i)
            osc.knobs[(size_t) i]->setBounds (first.removeFromLeft (cell).withSizeKeepingCentre (cell, juce::jmin (first.getHeight(), 52)));

        for (int i = 6; i < 11; ++i)
            osc.knobs[(size_t) i]->setBounds (second.removeFromLeft (cell).withSizeKeepingCentre (cell, juce::jmin (second.getHeight(), 52)));

        osc.random->setBounds (second.withSizeKeepingCentre (juce::jmin (second.getWidth(), 48), 20));
    }

    void layoutModulators()
    {
        auto c = modulatorPanel->getContentBounds();
        auto tabRow = c.removeFromTop (22);
        levelsTab->setBounds (tabRow.removeFromRight (78).reduced (2, 0));
        const auto tabWidth = tabRow.getWidth() / (int) tabs.size();

        for (auto& t : tabs)
            t->setBounds (tabRow.removeFromLeft (tabWidth).reduced (2, 0));

        auto chips = c.removeFromBottom (22);
        const auto chipWidth = chips.getWidth() / (int) performanceChips.size();

        for (auto& chip : performanceChips)
            chip->setBounds (chips.removeFromLeft (chipWidth).reduced (2, 0));

        c.removeFromTop (6);
        c.removeFromBottom (6);

        if (modulator == oscLevelsPage)
        {
            // The level envelopes of the oscillators on the page: a row each, its switch and four knobs.
            const auto h = c.getHeight() / WaveSynth::numFirstOscillators;

            for (int o = 0; o < (int) levels.size(); ++o)
            {
                if (! onPage (o))
                    continue;

                auto& l = levels[(size_t) o];
                auto row = c.removeFromTop (h);
                l.on->setBounds (row.removeFromLeft (70).withSizeKeepingCentre (64, 20));
                const auto w = row.getWidth() / (int) l.knobs.size();

                for (auto* k : l.knobs)
                    k->setBounds (row.removeFromLeft (w).withSizeKeepingCentre (w, juce::jmin (h, 52)));
            }

            return;
        }

        auto display = c.removeFromLeft (296);
        envelopeView.setBounds (display);
        lfoView.setBounds (display);
        c.removeFromLeft (8);

        if (modulator < 4)
        {
            auto& k = envelopeKnobs[(size_t) modulator];
            const auto w = c.getWidth() / 3, h = c.getHeight() / 2;
            auto top = c.removeFromTop (h);
            for (int i = 0; i < 3; ++i) k[(size_t) i]->setBounds (top.removeFromLeft (w).withSizeKeepingCentre (w, juce::jmin (h, 56)));
            for (int i = 3; i < 5; ++i) k[(size_t) i]->setBounds (c.removeFromLeft (w).withSizeKeepingCentre (w, juce::jmin (h, 56)));
        }
        else
        {
            auto& l = lfos[(size_t) (modulator - 4)];
            auto top = c.removeFromTop (22);
            l.shape->setBounds (top.removeFromLeft (top.getWidth() / 2 - 2));
            top.removeFromLeft (4);
            l.mode->setBounds (top);
            c.removeFromTop (6);
            auto second = c.removeFromTop (22);
            l.sync->setBounds (second.removeFromLeft (56));
            second.removeFromLeft (4);
            l.division->setBounds (second);
            c.removeFromTop (4);
            const auto w = c.getWidth() / 4;

            for (auto* k : { l.rate, l.phase, l.fade, l.smooth })
                k->setBounds (c.removeFromLeft (w).withSizeKeepingCentre (w, juce::jmin (c.getHeight(), 58)));
        }
    }

    void layoutMacros()
    {
        auto c = macroPanel->getContentBounds();
        const auto w = c.getWidth() / 2, h = c.getHeight() / 2;

        for (int m = 0; m < WaveSynth::numMacros; ++m)
        {
            auto cell = juce::Rectangle<int> (c.getX() + (m % 2) * w, c.getY() + (m / 2) * h, w, h);
            macroChips[(size_t) m]->setBounds (cell.removeFromTop (18).withSizeKeepingCentre (38, 18));
            macroKnobs[(size_t) m]->setBounds (cell.reduced (2, 0));
        }
    }

    void layoutRight()
    {
        auto header = rightPanel->getHeaderBounds();
        modPages.setBounds (header.removeFromRight (316).reduced (0, 1));
        auto c = rightPanel->getContentBounds();

        // The slots, eight to a page: two columns of four, each a source, a destination and an amount.
        {
            auto area = c;
            const auto columnWidth = (area.getWidth() - 12) / 2;
            const auto rowHeight = area.getHeight() / 4;

            for (int s = 0; s < WaveSynth::numSlots; ++s)
            {
                auto cell = juce::Rectangle<int> (area.getX() + (s % 8 / 4) * (columnWidth + 12), area.getY() + (s % 4) * rowHeight, columnWidth, rowHeight);
                auto& slot = slots[(size_t) s];
                slot.amount->setBounds (cell.removeFromRight (44).withSizeKeepingCentre (44, juce::jmin (44, rowHeight)));
                auto lines = cell.withSizeKeepingCentre (cell.getWidth() - 4, 46);
                slot.source->setBounds (lines.removeFromTop (21));
                lines.removeFromTop (4);
                slot.destination->setBounds (lines.removeFromTop (21));
            }
        }

        // The effects, in the order they play.
        {
            auto area = c;
            const auto w = (area.getWidth() - 4 * 6) / 5;

            for (auto& e : effects)
            {
                e.panel->setBounds (area.removeFromLeft (w));
                area.removeFromLeft (6);
                e.on->setBounds (e.panel->getHeaderBounds().removeFromRight (26).withSizeKeepingCentre (24, 16));
                auto inner = e.panel->getContentBounds();

                if (e.selector != nullptr)
                {
                    e.selector->setBounds (inner.removeFromTop (20));
                    inner.removeFromTop (4);
                }

                if (e.extra != nullptr)
                {
                    e.extra->setBounds (inner.removeFromTop (20));
                    inner.removeFromTop (4);
                }

                const auto rows = ((int) e.knobs.size() + 1) / 2;
                const auto h = inner.getHeight() / juce::jmax (1, rows);

                for (size_t i = 0; i < e.knobs.size(); ++i)
                {
                    auto cell = juce::Rectangle<int> (inner.getX() + (int) (i % 2) * inner.getWidth() / 2, inner.getY() + (int) (i / 2) * h, inner.getWidth() / 2, h);

                    if (e.knobs.size() % 2 == 1 && i == e.knobs.size() - 1)
                        cell = juce::Rectangle<int> (inner.getX(), cell.getY(), inner.getWidth(), h);

                    e.knobs[i]->setBounds (cell.withSizeKeepingCentre (juce::jmin (cell.getWidth(), 52), juce::jmin (h, 56)));
                }
            }
        }
    }

    //==========================================================================
    static constexpr int headerHeight = 52, stripHeight = 40;

    Synth& synth;
    EffectPresets::Bar presets;
    PatchBrowser browser;
    PageSwitch modPages, viewPages;
    std::unique_ptr<SignalConsoleView> consoleView;
    std::unique_ptr<DecryptView> decryptView;
    std::unique_ptr<DecryptedView> decryptedView;
    EnvelopeView envelopeView;
    LfoView lfoView;

    std::vector<std::unique_ptr<Knob>> allKnobs;
    std::vector<std::unique_ptr<StepSelector>> allSelectors;
    std::vector<std::unique_ptr<LedToggle>> allToggles;
    std::vector<std::pair<Knob*, int>> destinationKnobs;
    std::map<juce::String, Knob*> knobsById;

    struct Osc
    {
        std::unique_ptr<Panel> panel;
        LedToggle *on = nullptr, *random = nullptr;
        StepSelector *table = nullptr, *warp = nullptr, *fmFrom = nullptr;
        std::unique_ptr<WavetableView> view;
        Knob *position = nullptr, *warpAmount = nullptr;
        std::vector<Knob*> knobs;
    };
    std::array<Osc, WaveSynth::numOscillators> oscillators;
    OscillatorStrip strip { synth };
    int oscPage = 0;

    std::unique_ptr<Panel> sourcesPanel, routingPanel, ampPanel, modulatorPanel, macroPanel, rightPanel;
    Knob *subLevel = nullptr, *noiseLevel = nullptr, *noiseColour = nullptr, *ampLevel = nullptr, *ampPan = nullptr, *velocity = nullptr, *glide = nullptr, *bendRange = nullptr;
    StepSelector *subOctave = nullptr, *subShape = nullptr, *noiseType = nullptr, *routing = nullptr, *voiceMode = nullptr, *glideMode = nullptr;
    LedToggle* subDirect = nullptr;

    struct Filter
    {
        std::unique_ptr<Panel> panel;
        StepSelector* type = nullptr;
        std::unique_ptr<FilterView> view;
        Knob *cutoff = nullptr, *resonance = nullptr, *drive = nullptr, *key = nullptr;
    };
    std::array<Filter, 2> filters;

    std::vector<std::unique_ptr<SourceTab>> tabs;
    std::vector<std::unique_ptr<ModSourceChip>> performanceChips, macroChips;
    std::vector<std::vector<Knob*>> envelopeKnobs;

    struct LfoControls
    {
        StepSelector *shape = nullptr, *mode = nullptr, *division = nullptr;
        LedToggle* sync = nullptr;
        Knob *rate = nullptr, *phase = nullptr, *fade = nullptr, *smooth = nullptr;
        std::vector<juce::Component*> all() const { return { shape, mode, division, sync, rate, phase, fade, smooth }; }
    };
    std::vector<LfoControls> lfos;
    std::unique_ptr<PlainTab> levelsTab;
    struct LevelControls { LedToggle* on = nullptr; std::vector<Knob*> knobs; };
    std::vector<LevelControls> levels;
    std::vector<Knob*> macroKnobs;

    struct SlotControls { StepSelector *source = nullptr, *destination = nullptr; Knob* amount = nullptr; };
    std::vector<SlotControls> slots;

    struct FxModule
    {
        std::unique_ptr<Panel> panel;
        LedToggle *on = nullptr, *extra = nullptr;
        StepSelector* selector = nullptr;
        std::vector<Knob*> knobs;
    };
    std::vector<FxModule> effects;
    int modulator = 0;
};

//==============================================================================
WaveSynthEditor::WaveSynthEditor (BuiltInEffects::BasicSynthProcessor& s)
    : AudioProcessorEditor (s), content (std::make_unique<Content> (s))
{
    addAndMakeVisible (*content);

    // Scaled as a whole: from 70 % to 160 %, the shape kept. It opens at full size where the
    // screen has room, smaller where it hasn't.
    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) designWidth / designHeight);
    setResizeLimits (designWidth * 7 / 10, designHeight * 7 / 10, designWidth * 16 / 10, designHeight * 16 / 10);
    auto scale = 1.0;

    if (const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        scale = juce::jlimit (0.7, 1.0, juce::jmin ((display->userBounds.getHeight() - 100) / (double) designHeight, (display->userBounds.getWidth() - 40) / (double) designWidth));

    setSize (juce::roundToInt (designWidth * scale), juce::roundToInt (designHeight * scale));
    startTimerHz (30);
}

WaveSynthEditor::~WaveSynthEditor()
{
    stopTimer();
}

void WaveSynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::base);
}

void WaveSynthEditor::resized()
{
    const auto scale = (float) getWidth() / (float) designWidth;
    content->setTransform (juce::AffineTransform::scale (scale));
    content->setBounds (0, 0, designWidth, designHeight);
}

void WaveSynthEditor::refresh()
{
    content->refresh();
    content->repaint (content->getLocalBounds().removeFromTop (52).removeFromRight (120));
}

SoundControls::Knob* WaveSynthEditor::knobFor (const juce::String& id) const
{
    const auto found = content->knobsById.find (id);
    return found != content->knobsById.end() ? found->second : nullptr;
}

void WaveSynthEditor::showModulator (int index)
{
    content->showModulator (index);
}

void WaveSynthEditor::showRightPage (int index)
{
    content->showRightPage (index);
}

void WaveSynthEditor::showOscillatorPage (int page)
{
    content->showOscillatorPage (page);
}

int WaveSynthEditor::getOscillatorPage() const
{
    return content->oscPage;
}

void WaveSynthEditor::toggleFavourite()
{
    content->browser.toggleFavourite();
}

juce::Rectangle<int> WaveSynthEditor::getFavouriteBounds() const
{
    return getLocalArea (&content->browser, content->browser.heartBounds().toNearestInt());
}

void WaveSynthEditor::showView (int index)
{
    content->showView (index);
}

SignalConsoleView* WaveSynthEditor::getConsoleView() const
{
    return content->consoleView.get();
}

void WaveSynthEditor::ownPatchesChanged()
{
    content->presets.refresh();
    content->browser.repaint();
    content->decryptedView->refresh();
}

bool WaveSynthEditor::renameDecrypted (const juce::String& from, const juce::String& to)    { return content->browser.rename (from, to); }
bool WaveSynthEditor::deleteDecrypted (const juce::String& name)                            { return content->browser.remove (name); }

DecryptedView* WaveSynthEditor::getDecryptedView() const
{
    return content->decryptedView.get();
}

DecryptView* WaveSynthEditor::getDecryptView() const
{
    return content->decryptView.get();
}

bool WaveSynthEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& path : files)
        if (juce::File (path).hasFileExtension (".nksf"))
            return true;
    return false;
}

void WaveSynthEditor::filesDropped (const juce::StringArray& files, int, int)
{
    for (const auto& path : files)
    {
        const juce::File f (path);
        if (f.hasFileExtension (".nksf"))
        {
            importMassiveXFile (f, true);
            return;
        }
    }
}

bool WaveSynthEditor::importMassiveXFile (const juce::File& file, bool showFeedback)
{
    const auto res = MassiveXImport::importFile (file);
    if (! res.ok)
    {
        if (showFeedback)
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                    "Import Massive X Patch", res.error);
        return false;
    }

    MassiveXImport::applyToSynth (content->synth, res);
    content->synth.setBrowsingFavourites (false);
    content->synth.setBrowsingDecrypted (false);
    content->synth.setBrowsingComplex (false);
    refresh();
    content->browser.repaint();

    if (showFeedback)
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon,
                                                "Imported Massive X Patch", res.summary);
    return true;
}
