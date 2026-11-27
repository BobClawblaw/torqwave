#include <JuceHeader.h>
#include "../Source/AudioEngine.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/WaveSynth.h"
#include "../Source/WaveSynthEditor.h"
#include "../Source/WaveTables.h"
#include "../Source/SoundControls.h"
#include "../Source/Theme.h"
#include "TestUtils.h"
#include "WaveSynthTests.h"
#include "ClickCheck.h"
#include "../Source/WaveSynthPatchBuilder.h"
#include "../Source/MultitrackView.h"

namespace
{
    using TestUtils::check;
    using Synth = BuiltInEffects::BasicSynthProcessor;
    constexpr double rate = 48000.0;

    //==========================================================================
    void set (Synth& s, const juce::String& id, float plain)
    {
        auto* p = s.parameter (id);
        jassert (p != nullptr);

        if (auto* f = dynamic_cast<juce::AudioParameterFloat*> (p))
            BuiltInEffects::setExactly (*f, plain);
        else
            p->setValueNotifyingHost (p->convertTo0to1 (plain));
    }

    float plainOf (Synth& s, const juce::String& id)
    {
        auto* p = s.parameter (id);
        return p->convertFrom0to1 (p->getValue());
    }

    void defaults (Synth& s)
    {
        for (auto* p : s.getParameters())
            p->setValueNotifyingHost (p->getDefaultValue());
    }

    /** One sine from oscillator 1 at full level, no filter, the amp envelope as short as it goes and held. */
    void bare (Synth& s)
    {
        defaults (s);
        set (s, "osc1Position", 0.0f);
        set (s, "osc1Volume", 100.0f);
        set (s, "osc1RandomPhase", 0.0f);
        set (s, "filter1Type", 0.0f);
        set (s, "ampLevel", 0.0f);
        set (s, "ampVelocity", 100.0f);
        set (s, "env1Attack", 0.0f);
        set (s, "env1Decay", 0.0f);
        set (s, "env1Sustain", 100.0f);
        set (s, "env1Release", 0.0f);
    }

    /** A sine's amplitude in [from, from + n) of a channel. */
    double amplitude (const juce::AudioBuffer<float>& b, int channel, double frequency, int from, int n)
    {
        double re = 0.0, im = 0.0;

        for (int i = from; i < from + n; ++i)
        {
            const auto w = juce::MathConstants<double>::twoPi * frequency * i / rate;
            re += b.getSample (channel, i) * std::cos (w);
            im += b.getSample (channel, i) * std::sin (w);
        }

        return 2.0 * std::sqrt (re * re + im * im) / n;
    }

    struct Event
    {
        int frame;
        juce::MidiMessage message;
    };

    /** Plays events through the synth for a length in blocks of a size; offs before ons at a frame. */
    juce::AudioBuffer<float> play (Synth& s, std::vector<Event> events, int length, int blockSize = 512, std::function<void (int)> afterBlock = {})
    {
        s.setPlayConfigDetails (0, 2, rate, blockSize);
        s.setNonRealtime (true);
        s.prepareToPlay (rate, blockSize);
        std::stable_sort (events.begin(), events.end(), [] (const Event& a, const Event& b)
        {
            return a.frame < b.frame || (a.frame == b.frame && a.message.isNoteOff() && ! b.message.isNoteOff());
        });
        juce::AudioBuffer<float> out (2, length), b (2, blockSize);
        size_t next = 0;

        for (int at = 0; at < length; at += blockSize)
        {
            const auto n = juce::jmin (blockSize, length - at);
            juce::MidiBuffer midi;

            while (next < events.size() && events[next].frame < at + n)
            {
                midi.addEvent (events[next].message, juce::jmax (0, events[next].frame - at));
                ++next;
            }

            b.setSize (2, n, false, false, true);
            b.clear();
            s.processBlock (b, midi);

            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, at, b, ch, 0, n);

            if (afterBlock != nullptr)
                afterBlock (at + n);
        }

        return out;
    }

    std::vector<Event> held (std::initializer_list<int> notes, double seconds, float velocity = 1.0f, double from = 0.0)
    {
        std::vector<Event> e;

        for (auto n : notes)
        {
            e.push_back ({ (int) (from * rate), juce::MidiMessage::noteOn (1, n, velocity) });
            e.push_back ({ (int) ((from + seconds) * rate), juce::MidiMessage::noteOff (1, n) });
        }

        return e;
    }

    bool identical (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        if (a.getNumSamples() != b.getNumSamples() || a.getNumChannels() != b.getNumChannels())
            return false;

        for (int ch = 0; ch < a.getNumChannels(); ++ch)
            if (std::memcmp (a.getReadPointer (ch), b.getReadPointer (ch), sizeof (float) * (size_t) a.getNumSamples()) != 0)
                return false;

        return true;
    }

    /** The power at each FFT bin (Blackman-Harris window, 2^order points, averaged over hops through [from, to)). */
    std::vector<double> spectrum (const juce::AudioBuffer<float>& b, int channel, int from, int to, int order = 14)
    {
        const auto size = 1 << order;
        juce::dsp::FFT fft (order);
        std::vector<double> power ((size_t) size / 2 + 1, 0.0);
        std::vector<float> work ((size_t) size * 2);
        int frames = 0;

        for (int start = from; start + size <= to; start += size / 2, ++frames)
        {
            std::fill (work.begin(), work.end(), 0.0f);

            for (int i = 0; i < size; ++i)
            {
                // Blackman-Harris: sidelobes under -92 dB, so a loud harmonic doesn't spill into the bins between.
                const auto w = juce::MathConstants<double>::twoPi * i / size;
                const auto window = 0.35875 - 0.48829 * std::cos (w) + 0.14128 * std::cos (2.0 * w) - 0.01168 * std::cos (3.0 * w);
                work[(size_t) i] = b.getSample (channel, start + i) * (float) window;
            }

            fft.performFrequencyOnlyForwardTransform (work.data(), true);

            for (size_t k = 0; k < power.size(); ++k)
                power[k] += (double) work[k] * (double) work[k];
        }

        for (auto& p : power)
            p /= juce::jmax (1, frames);

        return power;
    }

    double binHz (int order)   { return rate / (double) (1 << order); }

    /** The power within a few bins of a frequency. */
    double powerNear (const std::vector<double>& power, double hz, int order, int halfWidth = 3)
    {
        const auto centre = (int) std::lround (hz / binHz (order));
        double sum = 0.0;

        for (int k = juce::jmax (0, centre - halfWidth); k <= juce::jmin ((int) power.size() - 1, centre + halfWidth); ++k)
            sum += power[(size_t) k];

        return sum;
    }

    /** The gain (dB) of b against a over a band, from their spectra. */
    double bandGainDb (const std::vector<double>& filtered, const std::vector<double>& open, double hz, int order)
    {
        const auto band = [&] (const std::vector<double>& p)
        {
            double sum = 0.0;
            const auto lo = (int) std::floor (hz * 0.94 / binHz (order)), hi = (int) std::ceil (hz * 1.06 / binHz (order));
            for (int k = lo; k <= hi; ++k) sum += p[(size_t) k];
            return sum;
        };

        return 10.0 * std::log10 (band (filtered) / juce::jmax (1e-30, band (open)));
    }

    double correlation (const juce::AudioBuffer<float>& b, int from, int n)
    {
        double lr = 0.0, ll = 0.0, rr = 0.0;

        for (int i = from; i < from + n; ++i)
        {
            const auto l = (double) b.getSample (0, i), r = (double) b.getSample (1, i);
            lr += l * r;
            ll += l * l;
            rr += r * r;
        }

        return lr / juce::jmax (1e-30, std::sqrt (ll * rr));
    }

    //==========================================================================
    void testBasics (Synth& synth)
    {
        // A4 bare: a sine at its velocity, starting from silence, silent soon after its hold, the same both sides.
        bare (synth);
        auto out = play (synth, held ({ 69 }, 1.0), 72000);
        const auto amp = amplitude (out, 0, 440.0, 2400, 45600);
        const auto tail = out.getMagnitude (0, 48000 + 960, 72000 - 48000 - 960);
        auto same = true;

        for (int i = 0; i < out.getNumSamples() && same; ++i)
            same = juce::exactlyEqual (out.getSample (0, i), out.getSample (1, i));

        check (std::abs (amp - 1.0) < 0.01 && std::abs (out.getSample (0, 0)) < 0.01f && tail < 1e-4f && same,
               "synth: bare, A4 plays a sine at its velocity (" + juce::String (amp, 4) + "), from silence, silent 20 ms after its hold, the same on both sides");

        // Velocity and level.
        set (synth, "ampLevel", -6.0f);
        out = play (synth, held ({ 69 }, 1.0, 0.5f), 48000);
        const auto amp2 = amplitude (out, 0, 440.0, 2400, 45600);
        const auto expected2 = 0.5 * juce::Decibels::decibelsToGain (-6.0);
        check (std::abs (amp2 / expected2 - 1.0) < 0.01, "synth: velocity 0.5 at -6 dB plays at " + juce::String (amp2, 4) + " (" + juce::String (expected2, 4) + ")");

        // A chord's notes each whole, notes in turn one after another.
        bare (synth);
        out = play (synth, held ({ 60, 64, 67 }, 1.0), 48000);
        auto chordOk = true;

        for (auto n : { 60, 64, 67 })
            chordOk = chordOk && std::abs (amplitude (out, 0, juce::MidiMessage::getMidiNoteInHertz (n), 2400, 45600) - 1.0) < 0.02;

        check (chordOk, "synth: a chord plays each of its notes whole");

        // Sixteen voices; a seventeenth note steals the oldest; All Notes Off ends them all.
        synth.prepareToPlay (rate, 512);
        juce::AudioBuffer<float> block (2, 512);
        juce::MidiBuffer midi;

        for (int i = 0; i < 17; ++i)
            midi.addEvent (juce::MidiMessage::noteOn (1, 40 + i, 1.0f), i);

        synth.processBlock (block, midi);
        const auto sounding = synth.getNumSounding();
        midi.clear();
        midi.addEvent (juce::MidiMessage::allNotesOff (1), 0);
        synth.processBlock (block, midi);
        midi.clear();
        synth.processBlock (block, midi);
        synth.processBlock (block, midi);
        check (sounding == 16 && synth.getNumSounding() == 0,
               "synth: sixteen voices sound, a seventeenth note taking the oldest, and All Notes Off ends them (" + juce::String (sounding) + ")");

        // A patch changed with notes down: three notes struck in poly, the patch then set to mono (as
        // choosing another patch does while a phrase plays), and the keys let go. Every note ends: none
        // is left sounding because the mono voice was the only one a note-off looked at.
        for (const auto mode : { 1.0f, 2.0f })
        {
            bare (synth);
            synth.prepareToPlay (rate, 512);
            midi.clear();

            for (auto n : { 48, 55, 60 })
                midi.addEvent (juce::MidiMessage::noteOn (1, n, 1.0f), 0);

            synth.processBlock (block, midi);
            midi.clear();
            const auto struck = synth.getNumSounding();
            set (synth, "voiceMode", mode);
            synth.processBlock (block, midi);

            for (auto n : { 48, 55, 60 })
                midi.addEvent (juce::MidiMessage::noteOff (1, n), 0);

            synth.processBlock (block, midi);
            midi.clear();

            for (int i = 0; i < 400; ++i)   // four seconds: past any release
                synth.processBlock (block, midi);

            check (struck == 3 && synth.getNumSounding() == 0,
                   juce::String ("synth: three notes struck in poly all end when their keys are let go after the patch is set to ") + (mode < 1.5f ? "mono" : "legato")
                       + " (" + juce::String (synth.getNumSounding()) + " left sounding)");
        }
    }

    void testControllers (Synth& synth)
    {
        const auto withFirst = [&synth] (std::function<void (std::vector<Event>&)> first, std::function<void()> setUp = {}, bool keyUp = false)
        {
            bare (synth);
            if (setUp != nullptr) setUp();
            std::vector<Event> e;
            first (e);
            e.push_back ({ 1, juce::MidiMessage::noteOn (1, 69, 1.0f) });
            if (keyUp) e.push_back ({ 4800, juce::MidiMessage::noteOff (1, 69) });
            return play (synth, e, 48000);
        };
        const auto cc = [] (int number, int value) { return [number, value] (std::vector<Event>& e) { e.push_back ({ 0, juce::MidiMessage::controllerEvent (1, number, value) }); }; };
        const auto none = [] (std::vector<Event>&) {};
        const auto saw = [&synth] { set (synth, "osc1Position", 66.67f); set (synth, "filter1Type", (float) WaveSynth::FilterType::ladder24); set (synth, "filter1Cutoff", 20000.0f); set (synth, "filter1Resonance", 0.0f); };

        const auto bent = withFirst ([] (std::vector<Event>& e) { e.push_back ({ 0, juce::MidiMessage::pitchWheel (1, 16383) }); });
        const auto quiet = withFirst (cc (7, 64));
        const auto left = withFirst (cc (10, 0));
        const auto dark = withFirst (cc (74, 0), saw);
        const auto bright = withFirst (none, saw);
        const auto pedalled = withFirst (cc (64, 127), {}, true);
        const auto unpedalled = withFirst (none, {}, true);
        const auto wheel = withFirst (cc (1, 127), [&synth] { synth.addModulation (WaveSynth::modWheel, "osc1Pitch", 0.25f); });

        const auto b4 = 440.0 * std::exp2 (2.0 / 12.0 * 8191.0 / 8192.0);
        check (amplitude (bent, 0, b4, 4800, 43200) > 0.95 && std::abs (amplitude (quiet, 0, 440.0, 4800, 43200) - std::pow (64.0 / 127.0, 2.0)) < 0.01
                   && amplitude (left, 0, 440.0, 4800, 43200) > 0.99 && left.getMagnitude (1, 4800, 43200) < 1e-6f
                   && amplitude (dark, 0, 440.0 * 8, 4800, 43200) < 0.1 * amplitude (bright, 0, 440.0 * 8, 4800, 43200)
                   && pedalled.getMagnitude (0, 24000, 24000) > 0.5f && unpedalled.getMagnitude (0, 24000, 24000) < 1e-5f
                   && amplitude (wheel, 0, 880.0, 4800, 43200) > 0.95,
               "synth: full pitch bend plays B4 for A4, Volume 64 plays at (64/127) squared, Pan 0 on the left alone, Cutoff (CC 74) at 0 takes a sawtooth's "
               "8th harmonic down 20 dB, the pedal holds a note past its key, and the mod wheel moves what a slot routes it to (an octave up here)");
    }

    void testState (AudioEngine& engine, Synth& synth)
    {
        juce::PluginDescription description;

        for (auto& d : BuiltInEffects::getDescriptions())
            if (d.fileOrIdentifier == BuiltInEffects::basicSynthId)
                description = d;

        // A patch, a change and a macro's name come back in a new instance.
        synth.loadPatch ("Acid Corrosion");
        set (synth, "filter1Cutoff", 1234.0f);
        set (synth, "osc2Table", (float) WaveTables::Table::metal);
        synth.setMacroName (3, "Scream");
        juce::MemoryBlock state;
        synth.getStateInformation (state);
        juce::String error;
        auto second = engine.pluginFormatManager.createPluginInstance (description, rate, 512, error);
        auto* again = dynamic_cast<Synth*> (second.get());

        if (again != nullptr)
            again->setStateInformation (state.getData(), (int) state.getSize());

        check (again != nullptr && std::abs (plainOf (*again, "filter1Cutoff") - 1234.0f) < 0.01f && (int) plainOf (*again, "osc2Table") == (int) WaveTables::Table::metal
                   && again->getPatchName() == "Acid Corrosion" && again->getMacroName (3) == "Scream" && again->getMacroName (0) == "Cutoff",
               "synth: its state brings back the patch's name, its settings and its macros' names");

        // A 0.19.2 session: the old settings, under their old IDs, map to the nearest new ones.
        juce::XmlElement old ("TORQWAVE_BASIC_SYNTH");
        old.setAttribute ("version", 1);
        for (auto [id, v] : { std::pair { "wave", 2.0 }, { "level", -6.0 }, { "attack", 10.0 }, { "decay", 200.0 }, { "sustain", 50.0 }, { "release", 300.0 },
                              { "cutoff", 1200.0 }, { "resonance", 60.0 }, { "osc2Wave", 3.0 }, { "osc2Octave", 1.0 }, { "osc2Detune", 7.0 }, { "osc2Level", 50.0 },
                              { "osc3Wave", 0.0 }, { "osc3Octave", -1.0 }, { "osc3Detune", 0.0 }, { "osc3Level", 0.0 },
                              { "filterEnv", 50.0 }, { "filterDecay", 300.0 }, { "velocityToFilter", 50.0 }, { "glide", 60.0 } })
            old.setAttribute (id, v);

        juce::MemoryBlock oldState;
        juce::AudioProcessor::copyXmlToBinary (old, oldState);
        auto third = engine.pluginFormatManager.createPluginInstance (description, rate, 512, error);
        auto* fromOld = dynamic_cast<Synth*> (third.get());

        if (fromOld != nullptr)
            fromOld->setStateInformation (oldState.getData(), (int) oldState.getSize());

        const auto slots = fromOld != nullptr ? fromOld->modulationOf ("filter1Cutoff") : decltype (fromOld->modulationOf ({})) {};
        auto fromEnvelope = false, fromVelocity = false;

        for (auto& [slot, source, amount] : slots)
        {
            fromEnvelope = fromEnvelope || (source == WaveSynth::env2 && amount > 0.0f);
            fromVelocity = fromVelocity || (source == WaveSynth::velocity && amount > 0.0f);
        }

        check (fromOld != nullptr && std::abs (plainOf (*fromOld, "osc1Position") - 66.67f) < 0.1f && std::abs (plainOf (*fromOld, "ampLevel") + 6.0f) < 0.01f
                   && std::abs (plainOf (*fromOld, "env1Attack") - 10.0f) < 0.01f && std::abs (plainOf (*fromOld, "env1Sustain") - 50.0f) < 0.01f
                   && plainOf (*fromOld, "osc2On") > 0.5f && std::abs (plainOf (*fromOld, "osc2Pitch") - 12.0f) < 0.01f && std::abs (plainOf (*fromOld, "osc2Fine") - 7.0f) < 0.01f
                   && plainOf (*fromOld, "osc3On") < 0.5f && (int) plainOf (*fromOld, "filter1Type") == (int) WaveSynth::FilterType::svfLow
                   && std::abs (plainOf (*fromOld, "filter1Resonance") - 60.0f) < 0.01f && fromEnvelope && fromVelocity
                   && (int) plainOf (*fromOld, "voiceMode") == 2 && std::abs (plainOf (*fromOld, "glideTime") - 60.0f) < 0.01f
                   && fromOld->getPatchName() == "Old Basic Synth",
               "synth: a 0.19.2 session's settings map to the new ones: its wave to oscillator 1's position, its oscillators 2 and 3, its envelope, its filter (an SVF), "
               "the filter envelope and velocity as modulation slots, glide as legato");
    }

    void testOscillatorPages (Synth&, WaveSynthEditor&);

    void testEditor (Synth& synth)
    {
        synth.loadPatch ("Init");
        std::unique_ptr<juce::AudioProcessorEditor> editor (synth.createEditorAndMakeActive());
        auto* window = dynamic_cast<WaveSynthEditor*> (editor.get());
        auto room = juce::Rectangle<int> (0, 0, 1440, 900);

        if (const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
            room = display->userBounds.toNearestInt();

        const auto ratio = editor != nullptr ? (double) editor->getWidth() / editor->getHeight() : 0.0;
        check (window != nullptr && std::abs (ratio - (double) WaveSynthEditor::designWidth / WaveSynthEditor::designHeight) < 0.01
                   && editor->getWidth() >= WaveSynthEditor::designWidth * 7 / 10 && editor->getWidth() <= WaveSynthEditor::designWidth
                   && editor->getHeight() <= juce::jmax (WaveSynthEditor::designHeight * 7 / 10, room.getHeight() - 100),
               "synth: its window opens at " + juce::String (editor != nullptr ? editor->getWidth() : 0) + " x " + juce::String (editor != nullptr ? editor->getHeight() : 0)
                   + ", its shape kept, inside the screen");

        if (window == nullptr)
            return;

        // An LFO dropped on the cutoff knob routes it there, and the knob shows the ring.
        auto* cutoff = window->knobFor ("filter1Cutoff");
        juce::Component from;
        const juce::DragAndDropTarget::SourceDetails lfo (SoundControls::modDragText (WaveSynth::lfo1), &from, {});

        if (cutoff != nullptr && cutoff->isInterestedInDragSource (lfo))
            cutoff->itemDropped (lfo);

        window->refresh();
        const auto routed = synth.modulationOf ("filter1Cutoff");
        check (cutoff != nullptr && routed.size() == 1 && std::get<1> (routed[0]) == WaveSynth::lfo1 && cutoff->getRings().size() == 1,
               "synth: an LFO dropped on the cutoff knob modulates the cutoff, and the knob shows its ring");

        // A source dropped on a slot's knob moves that slot's depth (in the next free slot, its Destination showing
        // "Slot 1 Depth"), and the knob shows the ring; the Destination menu has the depths under "Slot Depths",
        // and choosing one there routes the slot to it.
        {
            auto* depthKnob = window->knobFor ("mod1Amount");
            const juce::DragAndDropTarget::SourceDetails env (SoundControls::modDragText (WaveSynth::env3), &from, {});
            const auto interested = depthKnob != nullptr && depthKnob->isInterestedInDragSource (env);

            if (interested)
                depthKnob->itemDropped (env);

            window->refresh();
            const auto moved = synth.modulationOf ("mod1Amount");
            const auto filled = moved.size() == 1 ? std::get<0> (moved[0]) : -1;
            SoundControls::StepSelector* filledMenu = nullptr;
            const std::function<void (juce::Component&)> findMenu = [&] (juce::Component& in)
            {
                for (auto* c : in.getChildren())
                {
                    if (auto* s = dynamic_cast<SoundControls::StepSelector*> (c); s != nullptr && s->getText() == "Slot 1 Depth")
                        filledMenu = s;

                    findMenu (*c);
                }
            };
            findMenu (*window);

            // The menu as a click opens it: the heading, then the 24 depths.
            juce::String headingBefore, previous;
            int depthsListed = 0;

            const auto menu = filledMenu != nullptr ? filledMenu->makeMenu() : juce::PopupMenu();

            for (juce::PopupMenu::MenuItemIterator it (menu); it.next();)
            {
                const auto& item = it.getItem();

                if (item.isSectionHeader) { previous = item.text; continue; }
                if (item.text == "Slot 1 Depth") headingBefore = previous;
                if (item.text.startsWith ("Slot ") && item.text.endsWith (" Depth")) ++depthsListed;
                previous = {};
            }

            if (filledMenu != nullptr)
                filledMenu->setIndex (WaveSynth::firstSlotDepth + 17);   // the item "Slot 17 Depth" (None is first)

            const auto chosen = filled >= 0 ? synth.parameter ("mod" + juce::String (filled + 1) + "Destination")->getCurrentValueAsText() : juce::String();
            check (interested && filled > 0 && std::get<1> (moved[0]) == WaveSynth::env3 && depthKnob->getRings().size() == 1 && filledMenu != nullptr
                       && headingBefore == "Slot Depths" && depthsListed == WaveSynth::numSlots && chosen == "Slot 17 Depth",
                   "synth window: Env 3 dropped on slot 1's knob moves slot 1's depth from the next free slot, and the knob shows its ring; that slot's "
                   "Destination menu lists the 24 depths under Slot Depths, and choosing Slot 17 Depth there routes it so (" + chosen + ")");

            if (filled >= 0)
                synth.removeModulation (filled);
        }

        // Every page draws, in dark and light, at twice the scale.
        const auto before = Theme::current();
        auto drawn = true;

        for (auto theme : { "Dark", "Light" })
        {
            Theme::apply (Theme::make (theme, "Blue"));

            for (int page = 0; page <= WaveSynthEditor::oscLevelsPage; ++page)
            {
                window->showModulator (page);
                window->showRightPage (page % (WaveSynthEditor::effectsPage + 1));
                window->showOscillatorPage (page % 3);
                window->refresh();
                const auto image = window->createComponentSnapshot (window->getLocalBounds(), true, 2.0f);
                drawn = drawn && image.getWidth() == window->getWidth() * 2;
            }
        }

        Theme::apply (before);
        testOscillatorPages (synth, *window);
        synth.loadPatch ("Wailing Fuzz Lead");
        window->refresh();
        synth.loadPatch ("Wailing Fuzz Lead");
        window->refresh();
        check (drawn && window->knobFor ("osc1Position") != nullptr && window->knobFor ("osc1Position")->getLabel() == "Bright",
               "synth: every page of its window draws in dark and light at 2x, and a string's Position reads as its brightness");
        editor.reset();
    }

    /** The window's nine oscillators: three at a time in the panels, the strip choosing which (a click on a
        cell, its dot switching the oscillator), the Osc Levels page following, and FM From beside the warp
        while the warp is FM. */
    void testOscillatorPages (Synth& synth, WaveSynthEditor& window)
    {
        synth.loadPatch ("Init");
        window.showModulator (0);
        window.showOscillatorPage (0);
        window.refresh();

        // Shown in the window: it and every parent up to the window visible (none is on the screen in a test).
        const auto shown = [&window] (const juce::Component* c)
        {
            for (; c != nullptr && c != &window; c = c->getParentComponent())
                if (! c->isVisible())
                    return false;

            return c == &window;
        };
        const auto knobShown = [&] (const juce::String& id) { return shown (window.knobFor (id)); };

        std::vector<juce::Component*> cells, fmFrom;
        const std::function<void (juce::Component&)> find = [&] (juce::Component& in)
        {
            for (auto* c : in.getChildren())
            {
                if (auto* t = dynamic_cast<juce::SettableTooltipClient*> (c))
                {
                    if (t->getTooltip().startsWith ("Oscillator ") && t->getTooltip().contains ("click the dot")) cells.push_back (c);
                    if (t->getTooltip().startsWith ("FM From:")) fmFrom.push_back (c);
                }

                find (*c);
            }
        };
        find (window);

        const auto click = [] (juce::Component& c, juce::Point<float> at)
        {
            const auto event = juce::MouseEvent (juce::Desktop::getInstance().getMainMouseSource(), at, {}, 0.5f, 0, 0, 0, 0, &c, &c, juce::Time::getCurrentTime(), at, juce::Time::getCurrentTime(), 1, false);
            c.mouseDown (event);
            c.mouseUp (event);
        };

        const auto firstPage = knobShown ("osc1Position") && knobShown ("osc3Unison") && ! knobShown ("osc4Position") && ! knobShown ("osc7Position");
        check (cells.size() == 9 && fmFrom.size() == 9 && firstPage && window.knobFor ("osc9Position") != nullptr,
               "synth window: nine cells in the oscillator strip and nine oscillator panels, oscillators 1 to 3 shown first");

        if (cells.size() != 9 || fmFrom.size() != 9)
            return;

        // A click on oscillator 8's cell shows 7 to 9; one on its dot switches it on.
        const auto wasOn = synth.parameter ("osc8On")->getValue();
        click (*cells[7], cells[7]->getLocalBounds().getCentre().toFloat());
        window.refresh();
        const auto thirdPage = window.getOscillatorPage() == 2 && knobShown ("osc8Position") && knobShown ("osc9Pitch") && ! knobShown ("osc1Position") && ! knobShown ("osc5Position");
        click (*cells[7], { 8.0f, 8.0f });
        window.refresh();
        const auto switched = wasOn < 0.5f && synth.parameter ("osc8On")->getValue() > 0.5f && window.getOscillatorPage() == 2;
        const auto samePlace = window.knobFor ("osc8Position") != nullptr && window.knobFor ("osc2Position") != nullptr
                                && window.knobFor ("osc8Position")->getBoundsInParent() == window.knobFor ("osc2Position")->getBoundsInParent()
                                && window.knobFor ("osc8Position")->getParentComponent()->getBounds() == window.knobFor ("osc2Position")->getParentComponent()->getBounds();
        check (thirdPage && switched && samePlace,
               "synth window: a click on oscillator 8's cell shows oscillators 7 to 9 in the three panels (8 where 2 was), and a click on its dot switches it on");

        // The Osc Levels page shows the page's three.
        window.showModulator (WaveSynthEditor::oscLevelsPage);
        const auto levelsThird = knobShown ("osc7EnvAttack") && knobShown ("osc9EnvSustain") && ! knobShown ("osc1EnvAttack");
        window.showOscillatorPage (1);
        const auto levelsSecond = knobShown ("osc4EnvAttack") && knobShown ("osc6EnvDecay") && ! knobShown ("osc7EnvAttack") && knobShown ("osc5Position");
        window.showModulator (0);
        const auto levelsGone = ! knobShown ("osc4EnvAttack") && knobShown ("osc4Position");
        check (levelsThird && levelsSecond && levelsGone, "synth window: the Osc Levels page shows the level envelopes of the oscillators the strip's page shows");

        // FM From appears beside the warp while the warp is FM.
        set (synth, "osc5Warp", (float) WaveSynth::Warp::fm);
        window.refresh();
        const auto fmShown = shown (fmFrom[4]) && ! shown (fmFrom[3]) && ! shown (fmFrom[5]) && ! fmFrom[4]->getBounds().isEmpty();
        set (synth, "osc5Warp", (float) WaveSynth::Warp::off);
        window.refresh();
        check (fmShown && ! shown (fmFrom[4]), "synth window: FM From shows beside oscillator 5's warp while the warp is FM, and goes with it");

        window.showOscillatorPage (0);
        synth.loadPatch ("Init");
        window.refresh();
    }

    //==========================================================================
    void testPatches()
    {
        const auto& all = WaveSynth::factoryPatches();
        std::set<juce::String> names;
        auto categoriesKnown = true;

        for (auto& p : all)
        {
            names.insert (p.name);
            categoriesKnown = categoriesKnown && WaveSynth::patchCategories().contains (p.category);
        }

        check (all.size() >= 1000 && names.size() == all.size() && categoriesKnown && names.count ("Wailing Fuzz Lead") == 1 && names.count ("Swarm Drone") == 1,
               "synth: " + juce::String ((int) all.size()) + " factory patches, each named once, in known categories, Wailing Fuzz Lead and Swarm Drone among them");

        // Each plays a held minor chord (C3, E flat 3, G3 for two seconds) between -24 and -1 dB. Eight are
        // played at a time, each on a synth of its own, so nothing the patch before left behind is heard.
        std::vector<float> peaks (all.size(), 0.0f);
        {
            std::atomic<size_t> next { 0 };
            std::vector<std::thread> threads;

            for (int t = 0; t < 8; ++t)
                threads.emplace_back ([&]
                {
                    for (auto i = next++; i < all.size(); i = next++)
                    {
                        Synth own;
                        own.loadPatch ((int) i);
                        const auto out = play (own, held ({ 48, 51, 55 }, 2.0, 0.8f), (int) (4.0 * rate));
                        peaks[i] = juce::jmax (out.getMagnitude (0, 0, out.getNumSamples()), out.getMagnitude (1, 0, out.getNumSamples()));
                    }
                });

            for (auto& t : threads)
                t.join();
        }

        juce::StringArray problems;
        float loudest = 0.0f, quietest = 1.0f;
        juce::String loudestName, quietestName;

        for (size_t i = 0; i < all.size(); ++i)
        {
            const auto peak = peaks[i];
            const auto db = juce::Decibels::gainToDecibels (peak, -100.0f);

            if (db < -24.0f || db > -1.0f || ! std::isfinite (db))
                problems.add (all[i].name + " " + juce::String (db, 1));

            if (peak > loudest) { loudest = peak; loudestName = all[i].name; }
            if (peak < quietest) { quietest = peak; quietestName = all[i].name; }
        }

        check (problems.isEmpty(), "synth: every factory patch plays a held chord between -24 and -1 dB (the loudest " + loudestName + " at "
                                       + juce::String (juce::Decibels::gainToDecibels (loudest), 1) + " dB, the quietest " + quietestName + " at "
                                       + juce::String (juce::Decibels::gainToDecibels (quietest), 1) + " dB)"
                                       + (problems.isEmpty() ? juce::String() : " (" + problems.joinIntoString ("; ") + ")"));
    }

    void testAliasing (Synth& synth)
    {
        // C7 (2093 Hz) from each table at its first, middle and last frame: what isn't a harmonic is aliasing.
        constexpr int order = 15;
        const auto f = juce::MidiMessage::getMidiNoteInHertz (96);
        double worst = -300.0;
        juce::String worstName;

        for (int t = 0; t < (int) WaveTables::Table::count; ++t)
        {
            if (t == (int) WaveTables::Table::string)
                continue;

            for (auto position : { 0.0f, 50.0f, 100.0f })
            {
                bare (synth);
                set (synth, "osc1Table", (float) t);
                set (synth, "osc1Position", position);
                const auto out = play (synth, held ({ 96 }, 1.5), (int) (1.5 * rate));
                const auto power = spectrum (out, 0, 4800, out.getNumSamples(), order);
                double total = 0.0, harmonic = 0.0;

                for (auto p : power)
                    total += p;

                for (int k = 1; k * f < rate / 2; ++k)
                    harmonic += powerNear (power, k * f, order, 6);

                const auto aliasDb = 10.0 * std::log10 (juce::jmax (1e-30, total - harmonic) / total);

                if (aliasDb > worst)
                {
                    worst = aliasDb;
                    worstName = WaveTables::names()[t] + " at " + juce::String ((int) position) + " %";
                }
            }
        }

        check (worst < -60.0, "synth: every wavetable plays C7 with its aliasing under -60 dB of its sound (the most " + worstName + ", " + juce::String (worst, 1) + " dB)");
    }

    void testUnison (Synth& synth)
    {
        const auto with = [&synth] (int voices, float spread)
        {
            bare (synth);
            set (synth, "osc1Position", 66.67f);
            set (synth, "osc1Unison", (float) voices);
            set (synth, "osc1UnisonDetune", 20.0f);
            set (synth, "osc1UnisonSpread", spread);
            return play (synth, held ({ 57 }, 2.0), (int) (2.0 * rate));
        };

        const auto one = correlation (with (1, 100.0f), 4800, 86400);
        const auto narrow = correlation (with (8, 0.0f), 4800, 86400);
        const auto wide = correlation (with (8, 100.0f), 4800, 86400);
        const auto half = correlation (with (8, 50.0f), 4800, 86400);
        check (one > 0.9999 && narrow > 0.9999 && wide < 0.6 && half > wide + 0.2 && half < 0.95,
               "synth: unison spreads across the stereo field as far as its Spread: one voice or none spread is mono (correlation " + juce::String (one, 4) + ", "
                   + juce::String (narrow, 4) + "), eight at half spread " + juce::String (half, 3) + ", at full spread " + juce::String (wide, 3));
    }

    void testFilters (Synth& synth)
    {
        // White noise through the ladder at 1 kHz against the same noise unfiltered: the gain at each frequency.
        constexpr int order = 13;
        const auto noiseThrough = [&synth] (float type, float cutoff, float resonance)
        {
            bare (synth);
            set (synth, "osc1On", 0.0f);
            set (synth, "noiseLevel", 25.0f);
            set (synth, "filter1Type", type);
            set (synth, "filter1Cutoff", cutoff);
            set (synth, "filter1Resonance", resonance);
            const auto out = play (synth, held ({ 60 }, 4.0), (int) (4.0 * rate));
            return spectrum (out, 0, 4800, out.getNumSamples(), order);
        };

        const auto open = noiseThrough (0.0f, 20000.0f, 0.0f);
        const auto ladder = (float) WaveSynth::FilterType::ladder24;
        const auto at1k = noiseThrough (ladder, 1000.0f, 0.0f);
        const auto at300 = noiseThrough (ladder, 300.0f, 0.0f);
        const auto pass = bandGainDb (at1k, open, 200.0, order), atCutoff = bandGainDb (at1k, open, 1000.0, order), twoOctaves = bandGainDb (at1k, open, 4000.0, order);
        const auto atCutoff300 = bandGainDb (at300, open, 300.0, order);

        // Four poles: -0.7 dB at a fifth of the cutoff, -12 dB at it (3 dB a pole), -49.2 dB two octaves up (4 x -12.3).
        check (std::abs (pass + 0.68) < 1.0 && std::abs (atCutoff + 12.0) < 1.5 && std::abs (twoOctaves + 49.2) < 3.0 && std::abs (atCutoff300 + 12.0) < 1.5,
               "synth: the ladder at 1 kHz passes 200 Hz (" + juce::String (pass, 1) + " dB), is -12 dB at its cutoff (" + juce::String (atCutoff, 1) + " dB; at 300 Hz "
                   + juce::String (atCutoff300, 1) + " dB) and -49.2 dB two octaves up (" + juce::String (twoOctaves, 1) + " dB)");

        // Resonance: a peak at the cutoff, growing with the setting.
        const auto peakOf = [&] (const std::vector<double>& p)
        {
            auto best = 0.0;
            auto bestHz = 0.0;

            for (double hz = 500.0; hz < 2000.0; hz *= 1.01)
            {
                const auto g = bandGainDb (p, open, hz, order);
                if (g > best || bestHz == 0.0) { best = g; bestHz = hz; }
            }

            return std::pair { bestHz, best - bandGainDb (p, open, 200.0, order) };
        };

        const auto [hz50, peak50] = peakOf (noiseThrough (ladder, 1000.0f, 50.0f));
        const auto [hz90, peak90] = peakOf (noiseThrough (ladder, 1000.0f, 90.0f));
        check (std::abs (hz90 / 1000.0 - 1.0) < 0.1 && peak90 > 12.0 && peak90 > peak50 + 4.0 && peak50 > 0.0,
               "synth: the ladder's resonance peaks at its cutoff (" + juce::String (hz90, 0) + " Hz at 90 %) and grows with it (" + juce::String (peak50, 1) + " dB at 50 %, "
                   + juce::String (peak90, 1) + " dB at 90 %, over its pass band)");

        // The SVF with no resonance is damped at k = 2: -6 dB at its cutoff, 12 dB an octave above it.
        const auto svf = noiseThrough ((float) WaveSynth::FilterType::svfLow, 1000.0f, 0.0f);
        const auto svfCut = bandGainDb (svf, open, 1000.0, order), svfUp = bandGainDb (svf, open, 4000.0, order);
        check (std::abs (svfCut + 6.0) < 1.5 && std::abs (svfUp + 24.6) < 3.0,
               "synth: the SVF low-pass at 1 kHz is -6 dB at its cutoff (" + juce::String (svfCut, 1) + " dB) and falls 12 dB an octave (" + juce::String (svfUp, 1) + " dB two octaves up)");
    }

    /** Everything on, so every destination is heard. */
    void everything (Synth& synth)
    {
        bare (synth);
        for (int o = 1; o <= 3; ++o)
        {
            set (synth, "osc" + juce::String (o) + "On", 1.0f);
            set (synth, "osc" + juce::String (o) + "Position", 40.0f);
            set (synth, "osc" + juce::String (o) + "Volume", 50.0f);
            set (synth, "osc" + juce::String (o) + "Unison", 2.0f);
            set (synth, "osc" + juce::String (o) + "Warp", (float) WaveSynth::Warp::bend);
            set (synth, "osc" + juce::String (o) + "WarpAmount", 30.0f);
        }

        set (synth, "osc3Route", 50.0f);
        set (synth, "subLevel", 40.0f);
        set (synth, "noiseLevel", 20.0f);
        set (synth, "filter1Type", (float) WaveSynth::FilterType::ladder24);
        set (synth, "filter1Cutoff", 2000.0f);
        set (synth, "filter1Resonance", 30.0f);
        set (synth, "filter1Drive", 30.0f);
        set (synth, "filter2Type", (float) WaveSynth::FilterType::svfLow);
        set (synth, "filter2Drive", 30.0f);
        set (synth, "ampLevel", -12.0f);
        set (synth, "lfo1Rate", 3.0f);
        set (synth, "lfo2Rate", 1.0f);
        set (synth, "lfo3Rate", 2.0f);
        set (synth, "lfo4Rate", 0.7f);

        for (auto fx : { "Drive", "Chorus", "Phaser", "Delay", "Reverb" })
            set (synth, "fx" + juce::String (fx) + "On", 1.0f);

        set (synth, "fxDelayMix", 30.0f);

        // The LFOs heard, so their rates are.
        synth.addModulation (WaveSynth::lfo1, "osc2Position", 0.2f);
        synth.addModulation (WaveSynth::lfo2, "filter2Cutoff", 0.1f);
        synth.addModulation (WaveSynth::lfo3, "osc3Position", 0.2f);
        synth.addModulation (WaveSynth::lfo4, "filter1Cutoff", 0.1f);
    }

    void testModulation (Synth& synth)
    {
        // Every destination, from a macro: its value moves by the amount, and the sound changes.
        juce::StringArray stuck;
        const auto& ids = WaveSynth::destinationIds();

        for (int d = 0; d < ids.size(); ++d)
        {
            everything (synth);
            set (synth, "lfo1Sync", 0.0f);

            // One of oscillators 4 to 9: switched on, so it is heard.
            if (const auto o = WaveSynth::oscillatorOf (ids[d]); o > WaveSynth::numFirstOscillators)
            {
                set (synth, "osc" + juce::String (o) + "On", 1.0f);
                set (synth, "osc" + juce::String (o) + "Position", 40.0f);
                set (synth, "osc" + juce::String (o) + "Volume", 50.0f);
                set (synth, "osc" + juce::String (o) + "Unison", 2.0f);
                set (synth, "osc" + juce::String (o) + "Warp", (float) WaveSynth::Warp::bend);
                set (synth, "osc" + juce::String (o) + "Route", 50.0f);
            }

            // A slot's depth: that slot routes LFO 1 to oscillator 1's position, if it routes nothing yet.
            if (d >= WaveSynth::firstSlotDepth)
            {
                const auto n = "mod" + juce::String (d - WaveSynth::firstSlotDepth + 1);

                if (juce::exactlyEqual (synth.parameter (n + "Source")->getValue(), 0.0f))
                {
                    set (synth, n + "Source", (float) WaveSynth::lfo1);
                    set (synth, n + "Destination", (float) (ids.indexOf ("osc1Position") + 1));
                    set (synth, n + "Amount", 30.0f);
                }
            }

            const auto base = synth.parameter (ids[d])->getValue();
            const auto amount = base > 0.5f ? -0.4f : 0.4f;
            synth.addModulation (WaveSynth::macro1, ids[d], amount);
            float reading = -1.0f;
            const auto phrase = held ({ 48, 55 }, 0.4);
            const auto length = (int) (1.2 * rate);   // the delay's second repeat, where its feedback is heard
            const auto at = (int) (0.3 * rate);
            const auto watch = [&synth, &reading, d] (int done) { if (done >= at && reading < 0.0f) reading = synth.getReadings().destination[(size_t) d].load(); };
            const auto without = play (synth, phrase, length, 512, watch);
            const auto readingWithout = reading;
            set (synth, "macro1", 100.0f);
            reading = -1.0f;
            const auto with = play (synth, phrase, length, 512, watch);

            if (std::abs ((reading - readingWithout) - amount) > 0.02f || identical (with, without))
                stuck.add (ids[d] + " (" + juce::String (readingWithout, 3) + " to " + juce::String (reading, 3) + ")");
        }

        check (stuck.isEmpty(), "synth: a macro routed to each of the " + juce::String (ids.size()) + " destinations moves it by the slot's amount and changes the sound"
                                    + (stuck.isEmpty() ? juce::String() : " (not: " + stuck.joinIntoString (", ") + ")"));

        // Every source moves what it is routed to.
        juce::StringArray still;

        for (int source = 1; source < WaveSynth::numSources; ++source)
        {
            const auto reading = [&synth] (bool routed, int s)
            {
                everything (synth);
                set (synth, "env2Decay", 2000.0f); set (synth, "env3Decay", 2000.0f); set (synth, "env4Decay", 2000.0f);
                for (int m = 1; m <= WaveSynth::numMacros; ++m) set (synth, "macro" + juce::String (m), 100.0f);
                if (routed) synth.addModulation (s, "osc1Position", 0.4f);
                float r = -1.0f;
                std::vector<Event> e { { 0, juce::MidiMessage::controllerEvent (1, 1, 127) }, { 0, juce::MidiMessage::channelPressureChange (1, 127) },
                                       { 1, juce::MidiMessage::noteOn (1, 84, 1.0f) } };
                play (synth, e, (int) (0.2 * rate), 512, [&synth, &r] (int done) { if (done >= 4800 && r < 0.0f) r = synth.getReadings().destination[0].load(); });
                return r;
            };

            const auto plain = reading (false, source), moved = reading (true, source);

            if (std::abs (moved - plain) < 0.02f)
                still.add (WaveSynth::sourceNames()[source]);
        }

        check (still.isEmpty(), "synth: every modulation source (envelopes, LFOs, velocity, wheel, pressure, key, macros, random) moves what a slot routes it to"
                                    + (still.isEmpty() ? juce::String() : " (not: " + still.joinIntoString (", ") + ")"));

        // A free-running LFO's rate follows a macro, as a retriggered one's does: a 1 Hz square LFO on the
        // level, its rate on macro 1, counted by how often the level crosses its mean in two seconds.
        const auto crossings = [&synth] (float macro)
        {
            bare (synth);
            set (synth, "lfo1Shape", 4.0f);
            set (synth, "lfo1Mode", 0.0f);
            set (synth, "lfo1Rate", 1.0f);
            synth.addModulation (WaveSynth::lfo1, "ampLevel", 0.3f);
            synth.addModulation (WaveSynth::macro1, "lfo1Rate", 0.3f);
            set (synth, "macro1", macro);
            const auto out = play (synth, held ({ 60 }, 2.0), (int) (2.0 * rate));
            std::vector<float> level;

            for (int i = 0; i + 480 <= out.getNumSamples(); i += 480)
                level.push_back (out.getRMSLevel (0, i, 480));

            const auto mean = std::accumulate (level.begin(), level.end(), 0.0f) / (float) level.size();
            int count = 0;

            for (size_t i = 1; i < level.size(); ++i)
                count += (level[i] > mean) != (level[i - 1] > mean) ? 1 : 0;

            return count;
        };
        const auto slow = crossings (0.0f), fast = crossings (100.0f);
        check (slow >= 3 && slow <= 5 && fast >= 30 && fast <= 38,
               "synth: a free-running LFO's rate follows a macro: a 1 Hz square LFO on the level crosses its mean " + juce::String (slow)
                   + " times in two seconds, and " + juce::String (fast) + " with the macro at 100 (8.6 Hz)");
    }

    //==========================================================================
    // The signal console's groundwork: a table from a sound, a level envelope for each oscillator, sixteen slots.
    void testGroundwork()
    {
        using namespace WaveTables;
        const auto sameSamples = [] (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
        {
            if (a.getNumSamples() != b.getNumSamples() || a.getNumChannels() != b.getNumChannels())
                return false;

            for (int ch = 0; ch < a.getNumChannels(); ++ch)
                if (std::memcmp (a.getReadPointer (ch), b.getReadPointer (ch), sizeof (float) * (size_t) a.getNumSamples()) != 0)
                    return false;

            return true;
        };

        // A sine swept from 220 Hz to 1,980 Hz over four seconds, made into a table on a root of 55 Hz (A1).
        constexpr double root = 55.0, low = 220.0, high = 1980.0, seconds = 4.0;
        const auto length = (int) (seconds * rate);
        std::vector<float> sweep ((size_t) length);
        double phase = 0.0;

        for (int i = 0; i < length; ++i)
        {
            sweep[(size_t) i] = (float) (0.5 * std::sin (phase));
            phase += juce::MathConstants<double>::twoPi * (low + (high - low) * i / length) / rate;
        }

        const auto table = sampledFromAudio (sweep.data(), length, rate, root, "Sweep");
        const auto window = juce::roundToInt (2.0 * rate / root);
        Synth synth;
        bare (synth);
        set (synth, "osc1Table", (float) Table::sampled);
        synth.setSampledTable (0, table);

        // Where the sweep is at a harmonic of the root, the table played at the root has its energy at
        // that harmonic: the share of all 40 harmonics' power in it and its two neighbours, in dB.
        float worstShare = 0.0f;
        auto loudestRight = table != nullptr && table->frames == Sampled::maxFrames;
        juce::String shares;

        for (int harmonic : { 8, 14, 20, 26, 32 })
        {
            const auto centre = (harmonic * root - low) / (high - low) * length;
            const auto position = (centre - window * 0.5) / (double) (length - window);
            set (synth, "osc1Position", (float) (position * 100.0));
            const auto out = play (synth, held ({ 33 }, 1.0), 48000);
            double all = 0.0, there = 0.0, most = 0.0;
            int loudest = 0;

            for (int h = 1; h <= 40; ++h)
            {
                const auto a = amplitude (out, 0, h * root, 9600, 28800);
                all += a * a;
                if (std::abs (h - harmonic) <= 1) there += a * a;
                if (a > most) { most = a; loudest = h; }
            }

            const auto share = (float) (10.0 * std::log10 (juce::jmax (1.0e-12, there / juce::jmax (1.0e-12, all))));
            worstShare = juce::jmin (worstShare, share);
            loudestRight = loudestRight && loudest == harmonic;
            shares << (shares.isEmpty() ? "" : ", ") << juce::String (share, 2);
        }

        check (loudestRight && worstShare > -1.0f,
               "synth: a sine swept from 220 Hz to 1.98 kHz made into a Sampled table (64 frames on a 55 Hz root) plays the sweep's own harmonic loudest at five positions (harmonics 8, 14, 20, 26 and 32), "
               "with it and its neighbours holding all the power but " + shares + " dB");

        // A patch's state holds the table: it comes back the same, and plays the same.
        set (synth, "osc1Position", 37.0f);
        juce::MemoryBlock state;
        synth.getStateInformation (state);
        Synth second;
        second.setStateInformation (state.getData(), (int) state.getSize());
        const auto back = second.getSampledTable (0);
        const auto first = play (synth, held ({ 45, 52 }, 0.5), 36000), again = play (second, held ({ 45, 52 }, 0.5), 36000);
        second.loadPatch (0);
        check (back != nullptr && table != nullptr && back != table && back->frames == table->frames && back->strengths == table->strengths && back->name == "Sweep"
                   && juce::exactlyEqual (back->rootHz, table->rootHz) && sameSamples (first, again) && first.getMagnitude (0, 36000) > 0.1f
                   && second.getSampledTable (0) == nullptr && state.getSize() < 64 * 1024,
               "synth: a Sampled table saved in a patch's state comes back the same (its frames, root and name) and plays the same sample for sample, in "
                   + juce::String ((int) state.getSize() / 1024) + " KB; a factory patch loaded after it has none");

        // A sound's own pitch is found, and a sound without one gets the low root.
        std::vector<float> saw ((size_t) 24000), hiss ((size_t) 24000);
        juce::Random random (7);

        for (int i = 0; i < 24000; ++i)
        {
            const auto p = std::fmod (110.0 * i / rate, 1.0);
            saw[(size_t) i] = (float) (0.5 - p);
            hiss[(size_t) i] = random.nextFloat() - 0.5f;
        }

        const auto pitched = sampledFromAudio (saw.data(), 24000, rate, 0.0, "Saw"), unpitched = sampledFromAudio (hiss.data(), 24000, rate, 0.0, "Hiss");
        check (pitched != nullptr && std::abs (pitched->rootHz - 110.0) < 0.5 && unpitched != nullptr && juce::approximatelyEqual (unpitched->rootHz, rate / 2048.0)
                   && sampledFromAudio (saw.data(), 2400, rate, 0.0, "Short") == nullptr,
               "synth: a table from a 110 Hz sawtooth takes its root from the sound (" + juce::String (pitched != nullptr ? pitched->rootHz : 0.0, 2)
                   + " Hz), one from noise the low root of 23.4 Hz, and under a tenth of a second makes none");

        // An oscillator's level envelope: a 200 ms attack is at half level 100 ms in, with the amp envelope open;
        // then a 100 ms hold and a 300 ms decay to 25 % has covered 99 % of the way 300 ms after the hold.
        bare (synth);
        synth.setSampledTable (0, nullptr);
        const auto plain = play (synth, held ({ 69 }, 1.0), 48000);
        set (synth, "osc1EnvOn", 1.0f);
        set (synth, "osc1EnvAttack", 200.0f);
        set (synth, "osc1EnvHold", 100.0f);
        set (synth, "osc1EnvDecay", 300.0f);
        set (synth, "osc1EnvSustain", 25.0f);
        const auto shaped = play (synth, held ({ 69 }, 1.0), 48000);
        // Each measured over 1,200 samples: eleven whole cycles of the A.
        const auto full = amplitude (plain, 0, 440.0, 24000, 1200);
        const auto half = amplitude (shaped, 0, 440.0, 4800 - 600, 1200) / full, top = amplitude (shaped, 0, 440.0, 12000, 1200) / full,
                   settled = amplitude (shaped, 0, 440.0, 28800 - 600, 1200) / full, later = amplitude (shaped, 0, 440.0, 43200, 1200) / full;
        set (synth, "osc1EnvOn", 0.0f);
        const auto off = play (synth, held ({ 69 }, 1.0), 48000);
        check (std::abs (half - 0.5) < 0.01 && std::abs (top - 1.0) < 0.005 && std::abs (settled - 0.2575) < 0.01 && std::abs (later - 0.25) < 0.005 && sameSamples (plain, off),
               "synth: an oscillator's level envelope with a 200 ms attack is at " + juce::String (half, 3) + " of its level 100 ms in and " + juce::String (top, 3)
                   + " through its 100 ms hold, with the amp envelope open; its 300 ms decay to 25 % is at " + juce::String (settled, 4) + " 300 ms on and "
                   + juce::String (later, 3) + " after; switched off, the oscillator plays as it did, sample for sample");

        // Slots 16 and 24 do what slot 1 does.
        const auto routed = [&synth] (int slot)
        {
            synth.loadPatch ("Warehouse Sub");
            for (int i = 1; i <= WaveSynth::numSlots; ++i) set (synth, "mod" + juce::String (i) + "Source", 0.0f);
            const auto n = "mod" + juce::String (slot);
            set (synth, n + "Source", (float) WaveSynth::env2);
            set (synth, n + "Destination", (float) (WaveSynth::destinationIds().indexOf ("filter1Cutoff") + 1));
            set (synth, n + "Amount", 60.0f);
            return play (synth, held ({ 36, 43 }, 0.5), 36000);
        };
        const auto none = [&synth] { synth.loadPatch ("Warehouse Sub"); for (int i = 1; i <= WaveSynth::numSlots; ++i) set (synth, "mod" + juce::String (i) + "Source", 0.0f); return play (synth, held ({ 36, 43 }, 0.5), 36000); }();
        const auto one = routed (1), sixteen = routed (16), last = routed (24);
        check (WaveSynth::numSlots == 24 && synth.parameter ("mod24Amount") != nullptr && sameSamples (one, sixteen) && sameSamples (one, last) && ! sameSamples (one, none),
               "synth: 24 modulation slots, and envelope 2 to the cutoff in slot 16 or slot 24 renders the same as in slot 1, sample for sample");

        // LFOs 3 and 4 do what LFO 1 does: each set as LFO 1 is (a synced triangle, then a free square with a fade),
        // routed to the cutoff by itself, renders the same, sample for sample; and its rate as a destination moves it.
        const auto byLfo = [&synth] (int l, bool synced, float rateMacro)
        {
            synth.loadPatch ("Warehouse Sub");
            for (int i = 1; i <= WaveSynth::numSlots; ++i) set (synth, "mod" + juce::String (i) + "Source", 0.0f);
            const auto n = "lfo" + juce::String (l + 1);
            set (synth, n + "Shape", synced ? 1.0f : 4.0f);
            set (synth, n + "Sync", synced ? 1.0f : 0.0f);
            set (synth, n + "Division", 10.0f);
            set (synth, n + "Rate", 3.0f);
            set (synth, n + "Mode", synced ? 1.0f : 0.0f);
            set (synth, n + "Fade", synced ? 0.0f : 200.0f);
            set (synth, "macro1", rateMacro);
            synth.addModulation (WaveSynth::lfoSource (l), "filter1Cutoff", 0.3f);
            synth.addModulation (WaveSynth::macro1, n + "Rate", 0.3f);
            return play (synth, held ({ 36, 43 }, 0.8), 48000);
        };
        auto sameAsFirst = true;

        for (auto synced : { true, false })
            for (int l = WaveSynth::numFirstLfos; l < WaveSynth::numLfos; ++l)
                sameAsFirst = sameAsFirst && sameSamples (byLfo (0, synced, 0.0f), byLfo (l, synced, 0.0f)) && sameSamples (byLfo (0, synced, 100.0f), byLfo (l, synced, 100.0f));

        const auto rateMoves = ! sameSamples (byLfo (3, false, 0.0f), byLfo (3, false, 100.0f));
        check (WaveSynth::numLfos == 4 && sameAsFirst && rateMoves && WaveSynth::sourceNames()[WaveSynth::lfo4] == "LFO 4" && WaveSynth::destinationIds().contains ("lfo4Rate"),
               "synth: LFOs 3 and 4, set as LFO 1 is (synced, or free with a fade) and routed to the cutoff, render as LFO 1 does, sample for sample, "
               "with a macro on their rates too; and LFO 4's rate as a destination changes the sound");

        // A slot's depth as a destination: a macro moving slot 1's depth renders as that depth set on its knob, sample
        // for sample; an envelope on it fades the LFO in (neither as with no depth nor as at a fixed one); a slot moving
        // a depth on a slot that routes nothing changes nothing; and the menus list each slot's depth.
        {
            const auto depthPatch = [&synth] (float depth)
            {
                synth.loadPatch ("Warehouse Sub");
                for (int i = 1; i <= WaveSynth::numSlots; ++i) set (synth, "mod" + juce::String (i) + "Source", 0.0f);
                set (synth, "lfo1Shape", 1.0f);
                set (synth, "lfo1Sync", 0.0f);
                set (synth, "lfo1Rate", 4.0f);
                set (synth, "mod1Source", (float) WaveSynth::lfo1);
                set (synth, "mod1Destination", (float) (WaveSynth::destinationIds().indexOf ("filter1Cutoff") + 1));
                set (synth, "mod1Amount", depth);
            };
            const auto render = [&synth] { return play (synth, held ({ 36, 43 }, 1.2), 72000); };

            depthPatch (50.0f);
            const auto byKnob = render();
            depthPatch (0.0f);
            const auto noDepth = render();
            depthPatch (0.0f);
            set (synth, "macro1", 100.0f);
            synth.addModulation (WaveSynth::macro1, "mod1Amount", 0.25f);   // a quarter of the depth's range: 0 to 50
            const auto byMacro = render();

            depthPatch (0.0f);
            set (synth, "env3Attack", 1000.0f);
            set (synth, "env3Sustain", 100.0f);
            synth.addModulation (WaveSynth::env3, "mod1Amount", 0.25f);   // 0 to 50 over a second
            const auto faded = render();
            const auto fades = ! sameSamples (faded, noDepth) && ! sameSamples (faded, byKnob);

            depthPatch (50.0f);
            set (synth, "mod5Source", (float) WaveSynth::macro2);
            set (synth, "mod5Destination", (float) (WaveSynth::destinationIds().indexOf ("mod9Amount") + 1));
            set (synth, "mod5Amount", 80.0f);
            set (synth, "macro2", 100.0f);
            const auto idleDepth = render();

            const auto& ids = WaveSynth::destinationIds();
            const auto listed = ids.size() == WaveSynth::numDestinations && ids[WaveSynth::firstSlotDepth] == "mod1Amount" && ids[WaveSynth::numDestinations - 1] == "mod24Amount"
                                && WaveSynth::destinationNames()[WaveSynth::numDestinations - 1] == "Slot 24 Depth";
            check (sameSamples (byKnob, byMacro) && ! sameSamples (byKnob, noDepth) && fades && sameSamples (byKnob, idleDepth) && listed,
                   "synth: a slot's depth as a destination: a macro moving slot 1's depth from 0 to 50 renders as 50 on its knob, sample for sample; an envelope "
                   "on it (0 to 50 over a second) renders unlike both no depth and 50; a depth moved on a slot that routes nothing changes nothing; and the 24 depths close the destinations");
        }

        // A patch saved with eight slots (none of the new settings in its state) loads as it was.
        synth.loadPatch ("Acid Corrosion");
        const auto before = play (synth, held ({ 45 }, 0.5), 36000);
        juce::MemoryBlock whole;
        synth.getStateInformation (whole);
        auto xml = juce::AudioProcessor::getXmlFromBinary (whole.getData(), (int) whole.getSize());
        int removed = 0;

        if (xml != nullptr)
            for (int i = xml->getNumAttributes(); --i >= 0;)
            {
                const auto name = xml->getAttributeName (i);
                const auto slot = name.startsWith ("mod") ? name.substring (3).getIntValue() : 0;

                if (slot > 8 || (name.contains ("Env") && name.startsWith ("osc") && WaveSynth::oscillatorOf (name) <= WaveSynth::numFirstOscillators))
                {
                    xml->removeAttribute (name);
                    ++removed;
                }
            }

        juce::MemoryBlock old;
        if (xml != nullptr) juce::AudioProcessor::copyXmlToBinary (*xml, old);
        Synth third;
        set (third, "mod12Source", (float) WaveSynth::lfo1);   // set here, absent from the old state: must not survive a patch's defaults
        third.loadPatch (0);
        third.setStateInformation (old.getData(), (int) old.getSize());
        check (removed == (WaveSynth::numSlots - 8) * 3 + 3 * 5 && sameSamples (before, play (third, held ({ 45 }, 0.5), 36000)),
               "synth: a state saved with eight slots and no level envelopes (the " + juce::String (removed) + " new settings taken out of it) loads and plays as it was, sample for sample");

        // A session's lane on a slot's Source or Destination saved before 0.23.0 (LFOs 3 and 4, the slots' depths) keeps its
        // choice: values over the menus as they were (16 sources, 100 destinations) put back on the same ones in the longer menus.
        {
            const auto base64 = [] (const juce::XmlElement& x) { juce::MemoryBlock b; juce::AudioProcessor::copyXmlToBinary (x, b); return b.toBase64Encoding(); };
            juce::MemoryBlock now;
            synth.getStateInformation (now);
            auto current = juce::AudioProcessor::getXmlFromBinary (now.getData(), (int) now.getSize());
            auto older = std::make_unique<juce::XmlElement> (*current);
            for (auto id : { "lfo3Rate", "lfo4Rate" }) older->removeAttribute (id);
            auto oldest = std::make_unique<juce::XmlElement> (*older);
            oldest->removeAttribute ("osc4On");
            const auto thenNow = WaveSynth::destinationsWhenSaved (base64 (*current)), thenOlder = WaveSynth::destinationsWhenSaved (base64 (*older)),
                       thenOldest = WaveSynth::destinationsWhenSaved (base64 (*oldest));
            const auto choice = [] (float normalised, int lastIndex) { return juce::roundToInt (normalised * (float) lastIndex); };
            const auto randomThen = (float) WaveSynth::random / 15.0f, lastThen = 1.0f, cutoffThen = 37.0f / 100.0f;
            const auto ok = thenNow == WaveSynth::numDestinations && thenOlder == 100 && thenOldest == 46
                            && choice (WaveSynth::slotChoiceFrom ("mod3Source", randomThen, thenOlder), WaveSynth::numSources - 1) == WaveSynth::random
                            && choice (WaveSynth::slotChoiceFrom ("mod20Destination", lastThen, thenOlder), WaveSynth::numDestinations) == 100
                            && choice (WaveSynth::slotChoiceFrom ("mod1Destination", cutoffThen, thenOlder), WaveSynth::numDestinations) == 37
                            && juce::exactlyEqual (WaveSynth::slotChoiceFrom ("mod1Amount", 0.3f, thenOlder), 0.3f)
                            && juce::exactlyEqual (WaveSynth::slotChoiceFrom ("mod1Source", 0.4f, thenNow), 0.4f);
            check (ok, "synth: a lane on a slot's Source or Destination saved before 0.23.0 (or before nine oscillators) keeps its choice: Random stays Random "
                       "and the last destination (oscillator 9's Route) stays it in the longer menus; a lane saved since, and an Amount, are left as they are");
        }

        // No clicks: a Sampled table with its level envelope struck again and again, a steal, and All Sound Off.
        bare (synth);
        set (synth, "osc1Table", (float) Table::sampled);
        synth.setSampledTable (0, table);
        set (synth, "osc1Position", 50.0f);
        set (synth, "osc1EnvOn", 1.0f);
        set (synth, "osc1EnvAttack", 0.0f);
        set (synth, "osc1EnvDecay", 120.0f);
        set (synth, "osc1EnvSustain", 10.0f);
        set (synth, "voiceMode", 1.0f);
        std::vector<Event> events;

        for (int i = 0; i < 16; ++i)
        {
            const auto at = (int) (i * 0.125 * rate);
            events.push_back ({ at, juce::MidiMessage::noteOn (1, i % 4 == 3 ? 52 : 45, 0.9f) });
            if (i % 2 == 0) events.push_back ({ at + 4000, juce::MidiMessage::noteOff (1, 45) });
        }

        events.push_back ({ (int) (2.2 * rate), juce::MidiMessage::allSoundOff (1) });
        const auto struck = play (synth, events, (int) (2.5 * rate));
        const auto click = ClickCheck::worst (struck, rate, 0.03f);
        check (click.ratio <= 4.0f && struck.getMagnitude (0, struck.getNumSamples()) > 0.1f,
               "synth: no click with a Sampled table and its level envelope struck sixteen times, overlapping, in mono, then All Sound Off (the nearest a jump "
                   + juce::String (click.ratio, 1) + " times the steps around it" + (click.ratio > 0.0f ? " at " + juce::String (click.seconds, 3) + " s" : juce::String()) + ")");
    }

    //==========================================================================
    // Faults found in the engine and the first patches, each kept fixed.
    void testFaults()
    {
        const auto sameSamples = [] (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
        {
            for (int ch = 0; ch < a.getNumChannels(); ++ch)
                if (std::memcmp (a.getReadPointer (ch), b.getReadPointer (ch), sizeof (float) * (size_t) a.getNumSamples()) != 0)
                    return false;

            return a.getNumSamples() == b.getNumSamples();
        };

        // Pitch modulation is smooth: an LFO to an oscillator's Pitch by 1 % (half a semitone either way) is a vibrato.
        Synth synth;
        bare (synth);
        const auto steady = play (synth, held ({ 69 }, 1.0), 48000);
        set (synth, "lfo1Rate", 5.0f);
        set (synth, "mod1Source", (float) WaveSynth::lfo1);
        set (synth, "mod1Destination", (float) (WaveSynth::destinationIds().indexOf ("osc1Pitch") + 1));
        set (synth, "mod1Amount", 1.0f);
        const auto vibrato = play (synth, held ({ 69 }, 1.0), 48000);
        // The pitch over each 20 ms, from the zero crossings: it swings where the steady note doesn't.
        const auto swing = [] (const juce::AudioBuffer<float>& b)
        {
            double lowest = 1.0e9, highest = 0.0;

            for (int from = 9600; from + 960 <= 43200; from += 960)
            {
                int crossings = 0;
                double first = -1.0, last = -1.0;

                for (int i = from + 1; i < from + 960; ++i)
                    if (b.getSample (0, i - 1) < 0.0f && b.getSample (0, i) >= 0.0f)
                    {
                        const auto at = (double) (i - 1) + (double) -b.getSample (0, i - 1) / (double) (b.getSample (0, i) - b.getSample (0, i - 1));
                        if (first < 0.0) first = at;
                        last = at;
                        ++crossings;
                    }

                if (crossings > 1)
                {
                    const auto hz = (crossings - 1) * rate / (last - first);
                    lowest = juce::jmin (lowest, hz);
                    highest = juce::jmax (highest, hz);
                }
            }

            return 1200.0 * std::log2 (highest / lowest);
        };
        const auto cents = swing (vibrato), still = swing (steady);
        check (! sameSamples (steady, vibrato) && cents > 70.0 && cents < 100.0 && still < 1.0,
               "synth: an LFO to an oscillator's Pitch at 1 % is a vibrato " + juce::String (cents, 0) + " cents from its lowest to its highest (96 in full; " + juce::String (still, 2)
                   + " without it): pitch modulation moves smoothly, not in whole semitones");

        // A Steps LFO synced to the tempo plays its sixteen steps over a bar or more, in the hand-made patches
        // and the first bank (the second bank sets its own, some of them faster on purpose).
        juce::StringArray fast;
        int stepped = 0;
        const std::set<juce::String> secondBank (WaveSynthPatchBuilder::secondBankNames().begin(), WaveSynthPatchBuilder::secondBankNames().end());

        for (auto& p : WaveSynth::factoryPatches())
            for (int l = 1; l <= WaveSynth::numLfos && secondBank.count (p.name) == 0; ++l)
            {
                const auto n = "lfo" + juce::String (l);
                const auto value = [&p] (const juce::String& id, double otherwise) { const auto f = p.values.find (id); return f != p.values.end() ? f->second : otherwise; };

                if ((int) value (n + "Shape", 0.0) == 7 && value (n + "Sync", 0.0) > 0.5)
                {
                    ++stepped;

                    if (WaveSynth::divisionBeats ((int) value (n + "Division", 7.0)) < 4.0)
                        fast.add (p.name);
                }
            }

        check (fast.isEmpty() && stepped >= 80,
               "synth: each of the " + juce::String (stepped) + " synced Steps LFOs in the hand-made patches and the first bank takes a bar or more over its sixteen steps (a step a sixteenth note or longer), not a buzz"
                   + (fast.isEmpty() ? juce::String() : " (" + juce::String (fast.size()) + ": " + fast.joinIntoString (", ").substring (0, 400) + ")"));

        // A macro named Rate changes the sound: Wobble Press's wobble is free, so its rate can move.
        synth.loadPatch ("Wobble Press");
        const auto slow = play (synth, held ({ 36 }, 1.0), 48000);
        set (synth, "macro1", 100.0f);
        const auto quick = play (synth, held ({ 36 }, 1.0), 48000);
        check (! sameSamples (slow, quick), "synth: Wobble Press's Rate macro changes its wobble (it did nothing while the LFO was synced to the tempo)");

        // The same note renders the same whatever the synth played before: coloured noise starts from nothing.
        bare (synth);
        set (synth, "osc1On", 0.0f);
        set (synth, "noiseLevel", 80.0f);
        set (synth, "noiseType", 1.0f);
        const auto first = play (synth, held ({ 60 }, 0.3), 24000);
        set (synth, "noiseType", 2.0f);
        play (synth, held ({ 72, 76, 79 }, 0.3, 1.0f), 24000);
        set (synth, "noiseType", 1.0f);
        const auto again = play (synth, held ({ 60 }, 0.3), 24000);
        check (sameSamples (first, again) && first.getMagnitude (0, 24000) > 0.01f, "synth: a note of pink noise renders the same, sample for sample, after the synth has played other noise in between");

        // A delay longer than its line (8 bars at 120 BPM is 16 s, the line 4 s) starts at the line's length, not past its start.
        Synth delayed;
        bare (delayed);
        set (delayed, "fxDelayOn", 1.0f);
        set (delayed, "fxDelayMix", 100.0f);
        set (delayed, "fxDelayDivision", 0.0f);
        const auto echoed = play (delayed, held ({ 60 }, 0.3), 48000 * 5);
        check (echoed.getMagnitude (0, 48000 * 5) < 4.0f, "synth: a delay of 8 bars, longer than its 4-second line, plays from the first block (it read before the line's start)");
    }

    void testPresetTexts()
    {
        const auto sameSamples = [] (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
        {
            for (int ch = 0; ch < a.getNumChannels(); ++ch)
                if (std::memcmp (a.getReadPointer (ch), b.getReadPointer (ch), sizeof (float) * (size_t) a.getNumSamples()) != 0)
                    return false;

            return a.getNumSamples() == b.getNumSamples();
        };

        const auto folder = juce::File::createTempFile ("synthpresets");
        EffectPresets::Store store (folder);
        Synth synth;
        const auto key = synth.getPresetKey();

        // A synth preset with a Sampled table on oscillator 2 and named macros, saved and loaded into another synth.
        std::vector<float> saw (24000);
        for (size_t i = 0; i < saw.size(); ++i)
            saw[i] = (float) (0.4 * (2.0 * std::fmod ((double) i * 110.0 / rate, 1.0) - 1.0));

        synth.loadPatch ("Abyss Reese");
        set (synth, "osc2Table", (float) WaveTables::Table::sampled);
        synth.setSampledTable (1, WaveTables::sampledFromAudio (saw.data(), (int) saw.size(), rate, 0.0, "Saw"));
        synth.setMacroName (0, "Grind");
        synth.setMacroName (2, "Open");
        synth.setPatchName ("Abyss Reese");
        store.save (key, "Mine", synth.getPresetValues(), synth.getPresetTexts());
        const auto original = play (synth, held ({ 45 }, 0.5), 24000);

        Synth other;
        other.loadPatch ("Cold Bell");
        other.setPresetValues (*store.load (key, "Mine"));
        other.setPresetTexts (store.loadTexts (key, "Mine"));
        const auto table = other.getSampledTable (1);
        const auto again = play (other, held ({ 45 }, 0.5), 24000);
        check (table != nullptr && table->toText() == synth.getSampledTable (1)->toText() && table->frames == synth.getSampledTable (1)->frames
                   && juce::exactlyEqual (table->rootHz, synth.getSampledTable (1)->rootHz) && table->name == "Saw"
                   && other.getSampledTable (0) == nullptr && other.getMacroName (0) == "Grind" && other.getMacroName (1) == synth.getMacroName (1)
                   && other.getMacroName (2) == "Open" && other.getPatchName() == "Abyss Reese" && sameSamples (original, again),
               "synth: a preset saved with a Sampled table and named macros comes back whole in another synth (the table's "
                   + juce::String (table != nullptr ? table->frames : 0) + " frames, its root and name, the macros' names, and its render sample for sample)");

        // A preset file written before texts (values only) loads the same, and leaves the tables and macro names alone.
        juce::XmlElement old ("TORQWAVE_PRESETS");
        old.setAttribute ("effect", key);
        auto* preset = old.createNewChildElement ("PRESET");
        preset->setAttribute ("name", "Old");
        const auto saved = *store.load (key, "Mine");
        for (auto& [id, value] : saved)
        {
            auto* e = preset->createNewChildElement ("VALUE");
            e->setAttribute ("id", id);
            e->setAttribute ("value", juce::String (value, 17));
        }

        const auto oldFolder = juce::File::createTempFile ("oldsynthpresets");
        oldFolder.createDirectory();
        EffectPresets::Store oldStore (oldFolder);
        const auto files = folder.findChildFiles (juce::File::findFiles, false);
        if (! files.isEmpty())
            old.writeTo (oldFolder.getChildFile (files[0].getFileName()));

        const auto oldValues = oldStore.load (key, "Old");
        Synth third;
        third.loadPatch ("Abyss Reese");
        third.setMacroName (0, "Kept");
        third.setPresetValues (*oldValues);
        third.setPresetTexts (oldStore.loadTexts (key, "Old"));
        check (oldValues.has_value() && *oldValues == saved && oldStore.loadTexts (key, "Old").empty()
                   && third.getMacroName (0) == "Kept" && third.getSampledTable (1) == nullptr,
               "synth: a preset file of values alone (as saved before tables and macro names were kept) loads the same values, with no texts, and leaves the macros' names and tables as they were");

        // Through the presets bar: choosing the saved preset brings its table and names with it.
        EffectPresets::Bar bar (store, key, [&other] { return other.getPresetValues(); }, [&other] (const EffectPresets::Values& v) { other.setPresetValues (v); });
        bar.carryTexts ([&other] { return other.getPresetTexts(); }, [&other] (const EffectPresets::Texts& t) { other.setPresetTexts (t); });
        other.loadPatch ("Cold Bell");
        const auto cleared = other.getSampledTable (1) == nullptr && other.getMacroName (0) != "Grind";
        bar.refresh();
        for (int i = 0; i < bar.getOrder().size() && bar.getChosen() != "Mine"; ++i)
            bar.step (1);
        check (cleared && other.getSampledTable (1) != nullptr && other.getMacroName (0) == "Grind",
               "synth: choosing a preset in the window's presets bar sets its Sampled table and macro names too");

        folder.deleteRecursively();
        oldFolder.deleteRecursively();
    }

    void testBlockSizes (Synth& synth)
    {
        // Patches with moving parts sound the same, sample for sample, whatever the blocks.
        std::vector<Event> phrase;

        for (int i = 0; i < 8; ++i)
        {
            const auto at = 777 + i * 6007;
            phrase.push_back ({ at, juce::MidiMessage::noteOn (1, 48 + (i * 5) % 17, 0.5f + 0.06f * (float) i) });
            phrase.push_back ({ at + 9001, juce::MidiMessage::noteOff (1, 48 + (i * 5) % 17) });
        }

        phrase.push_back ({ 20011, juce::MidiMessage::controllerEvent (1, 74, 40) });
        phrase.push_back ({ 30011, juce::MidiMessage::pitchWheel (1, 12000) });
        juce::StringArray differ;

        for (auto name : { "Acid Drip", "Wailing Fuzz Lead", "Gated Machine", "Ghost Signal", "Abyss Reese", "Concrete Pad" })
        {
            synth.loadPatch (name);
            const auto reference = play (synth, phrase, (int) (1.5 * rate), 512);

            for (auto size : { 1, 37, 64, 333 })
            {
                synth.loadPatch (name);

                if (! identical (play (synth, phrase, (int) (1.5 * rate), size), reference))
                    differ.add (juce::String (name) + " in " + juce::String (size));
            }
        }

        check (differ.isEmpty(), "synth: six patches with LFOs, steps, strings, random and effects render bit for bit the same in blocks of 512, 333, 64, 37 and 1 sample"
                                     + (differ.isEmpty() ? juce::String() : " (not: " + differ.joinIntoString (", ") + ")"));
    }

    /** Oscillators 4 to 9 on with a little of everything: their own tables, warps, unison, pans and routes. */
    void nine (Synth& synth)
    {
        everything (synth);

        for (int o = 4; o <= WaveSynth::numOscillators; ++o)
        {
            const auto id = "osc" + juce::String (o);
            set (synth, id + "On", 1.0f);
            set (synth, id + "Table", (float) (o % 5));
            set (synth, id + "Position", 10.0f * (float) o);
            set (synth, id + "Volume", 40.0f);
            set (synth, id + "Pitch", (float) ((o % 3) * 7 - 7));
            set (synth, id + "Pan", (float) (o - 6) * 25.0f);
            set (synth, id + "Unison", 2.0f);
            set (synth, id + "Warp", (float) (o % 8));
            set (synth, id + "WarpAmount", 35.0f);
            set (synth, id + "Route", (float) (o % 2) * 60.0f);
        }
    }

    void testNine (Synth& synth)
    {
        // The parameters: the 235 there were keep their places (their IDs in the order a state written by
        // 0.21.1 lists them), oscillators 4 to 9 and FM From after them.
        const auto& all = synth.getParameters();
        juce::uint64 firstIds = 1469598103934665603ull;

        for (int i = 0; i < 235 && i < all.size(); ++i)
            for (auto c : dynamic_cast<juce::RangedAudioParameter*> (all[i])->paramID)
                firstIds = (firstIds ^ (juce::uint64) c) * 1099511628211ull;

        const auto idAt = [&all] (int i) { return dynamic_cast<juce::RangedAudioParameter*> (all[i])->paramID; };
        check (all.size() == 456 && firstIds == 0x76c5005fb974585bull && idAt (235) == "osc4On" && idAt (366) == "osc9EnvSustain" && idAt (367) == "osc1FmFrom"
                   && idAt (375) == "osc9FmFrom" && idAt (376) == "lfo3Shape" && idAt (447) == "mod24Amount" && idAt (448) == "arpOn" && idAt (455) == "sceneCount"
                   && WaveSynth::destinationIds().size() == 126 && WaveSynth::destinationIds()[45] == "fxReverbMix" && WaveSynth::destinationIds()[46] == "osc4Position"
                   && WaveSynth::destinationIds()[99] == "osc9Route" && WaveSynth::destinationIds()[100] == "lfo3Rate" && WaveSynth::destinationIds()[102] == "mod1Amount",
               "synth: nine oscillators, four LFOs, arp and scenes, 456 parameters: the first 235 as 0.21.1 had them, in its order, then oscillators 4 to 9 and each one's "
               "FM From (to 376, as 0.22.0 had them), then LFOs 3 and 4, slots 17 to 24, arp and scenes; 126 modulation destinations (LFO 3 and 4's rates and the 24 slots' depths after), the first 100 where they were");

        // Off, 4 to 9 change nothing: a patch with them off renders as it does with them switched on and off again.
        synth.loadPatch ("Abyss Reese");
        const auto phrase = held ({ 36, 43 }, 1.0, 0.8f);
        const auto reference = play (synth, phrase, (int) (1.5 * rate));
        synth.loadPatch ("Abyss Reese");

        for (int o = 4; o <= 9; ++o)
        {
            set (synth, "osc" + juce::String (o) + "On", 1.0f);
            set (synth, "osc" + juce::String (o) + "On", 0.0f);
        }

        check (identical (play (synth, phrase, (int) (1.5 * rate)), reference), "synth: oscillators 4 to 9 off leave a patch sample for sample as it was");

        // A state, and a whole preset, saved before there were nine: whatever the synth had on, 4 to 9 go off.
        synth.loadPatch ("Abyss Reese");
        juce::MemoryBlock state;
        synth.getStateInformation (state);
        auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), (int) state.getSize());
        auto values = synth.getPresetValues();

        for (int i = xml->getNumAttributes(); --i >= 0;)
            if (WaveSynth::isAddedWithNine (xml->getAttributeName (i)))
                xml->removeAttribute (xml->getAttributeName (i));

        for (auto it = values.begin(); it != values.end();)
            it = WaveSynth::isAddedWithNine (it->first) ? values.erase (it) : std::next (it);

        juce::MemoryBlock old;
        juce::AudioProcessor::copyXmlToBinary (*xml, old);
        nine (synth);
        set (synth, "osc1FmFrom", 5.0f);
        synth.setStateInformation (old.getData(), (int) old.getSize());
        const auto fromState = play (synth, phrase, (int) (1.5 * rate));
        nine (synth);
        synth.setPresetValues (values);
        const auto fromPreset = play (synth, phrase, (int) (1.5 * rate));
        check (identical (fromState, reference) && identical (fromPreset, reference),
               "synth: a state or a whole preset saved before there were nine oscillators loads with 4 to 9 off and FM From at Before, sample for sample as it was");

        // A session saved before: an automation lane on slot 3's Destination, its values normalised over the 47
        // choices the menu had, lands on the same destinations in the longer menu; a lane saved since is left alone.
        {
            const auto track = [&] (const juce::MemoryBlock& synthState, int slotId)
            {
                juce::XmlElement xmlTrack ("TRACK");
                auto* slot = xmlTrack.createNewChildElement ("EFFECTS")->createNewChildElement ("SLOT");
                slot->setAttribute ("id", slotId);
                slot->createNewChildElement ("STATE")->addTextElement (synthState.toBase64Encoding());
                auto* lane = xmlTrack.createNewChildElement ("ENVELOPE");
                lane->setAttribute ("parameter", "effect");
                lane->setAttribute ("slot", slotId);
                lane->setAttribute ("id", "mod3Destination");

                for (auto [frame, index] : { std::pair { 0, 1 }, std::pair { 48000, 28 }, std::pair { 96000, 46 } })
                {
                    auto* point = lane->createNewChildElement ("POINT");
                    point->setAttribute ("frame", frame);
                    point->setAttribute ("value", (double) index / 46.0);
                }

                SessionState s;
                s.tracks.emplace_back();
                s.tracks.back().id = 5;
                SessionFile::readEnvelopes (s, 5, xmlTrack);
                juce::StringArray landed;
                auto* destination = synth.parameter ("mod3Destination");

                for (auto& p : s.tracks.back().envelope (AutomationTarget::effect ((juce::uint32) slotId, "mod3Destination")).points)
                {
                    destination->setValueNotifyingHost (p.value);
                    landed.add (dynamic_cast<juce::AudioParameterChoice*> (destination)->getCurrentChoiceName());
                }

                return landed.joinIntoString (", ");
            };

            juce::MemoryBlock now;
            synth.getStateInformation (now);
            const auto oldLanes = track (old, 7);
            const auto newLanes = track (now, 8);
            check (oldLanes == "Osc 1 Position, Sub Level, Reverb Mix" && newLanes != oldLanes,
                   "synth: a lane drawn on a slot's Destination in a session saved before there were nine oscillators lands on the destinations it did (" + oldLanes
                       + "); one saved since is read as it is");
        }

        // The automation menu: a sub-menu for each oscillator, then the synth's sections, every parameter once; a
        // group holding a lane says so.
        {
            SessionState s;
            s.tracks.emplace_back();
            s.tracks.back().id = 3;
            Envelope lane;
            lane.points.push_back ({ 0, 0.5f });
            SessionEdits::setEnvelope (s, 3, AutomationTarget::effect (9, "osc7Volume"), std::move (lane));
            std::vector<AutomationTarget> targets;
            const auto menu = MultitrackView::parameterMenu (synth, 9, s.tracks.back(), std::nullopt, 1000, targets);
            juce::StringArray groups, seventh;
            int items = 0;

            for (juce::PopupMenu::MenuItemIterator group (menu); group.next();)
                if (const auto& g = group.getItem(); g.subMenu != nullptr)
                {
                    groups.add (g.text);

                    for (juce::PopupMenu::MenuItemIterator item (*g.subMenu); item.next(); ++items)
                        if (g.text.startsWith ("Oscillator 7"))
                            seventh.add (item.getItem().text);
                }

            check (groups.joinIntoString (", ") == "Oscillator 1, Oscillator 2, Oscillator 3, Oscillator 4, Oscillator 5, Oscillator 6, Oscillator 7 (automated), Oscillator 8, "
                                                   "Oscillator 9, Sub and Noise, Filters, Amp and Voice, Envelopes, LFOs, Modulation Slots, Macros, Effects, Arpeggiator & Scenes"
                       && items == 456 && (int) targets.size() == 456 && seventh.size() == 23 && seventh.contains ("Osc 7 Level (automated)") && seventh.contains ("Osc 7 FM From"),
                   "synth: its automation menu groups its " + juce::String (items) + " parameters in " + juce::String (groups.size())
                       + " sub-menus (an oscillator's 23 in each of nine, then sub and noise, filters, amp and voice, envelopes, LFOs, slots, macros, effects, arp & scenes); an automated one marks its group");
        }

        // FM From: Before is the chain it always was (1 takes 3, 2 takes 1, 3 takes 2); any oscillator can be chosen.
        const auto fm = [&] (std::array<int, 3> from)
        {
            everything (synth);

            for (int o = 1; o <= 3; ++o)
            {
                set (synth, "osc" + juce::String (o) + "Warp", (float) WaveSynth::Warp::fm);
                set (synth, "osc" + juce::String (o) + "WarpAmount", 50.0f);
                set (synth, "osc" + juce::String (o) + "FmFrom", (float) from[(size_t) o - 1]);
            }

            return play (synth, phrase, (int) (1.0 * rate));
        };

        const auto before = fm ({ 0, 0, 0 });
        check (identical (before, fm ({ 3, 1, 2 })) && ! identical (before, fm ({ 2, 3, 1 })),
               "synth: FM From at Before takes the oscillator before, as FM always has (1 from 3, 2 from 1, 3 from 2), the same sample for sample as choosing those; other choices sound otherwise");

        nine (synth);
        set (synth, "osc5Warp", (float) WaveSynth::Warp::fm);
        set (synth, "osc5FmFrom", 9.0f);
        const auto fromNine = play (synth, phrase, (int) (1.0 * rate));
        set (synth, "osc5FmFrom", 0.0f);
        check (! identical (fromNine, play (synth, phrase, (int) (1.0 * rate))), "synth: oscillator 5's FM can come from oscillator 9");

        // Silent modulators: an oscillator an FM From names is worked out even when off, its samples driving the FM
        // and none heard; Before still takes an oscillator that is off as nothing, as it always has.
        const auto modulated = [&] (int from, float amount, bool fiveOn)
        {
            bare (synth);
            set (synth, "osc1On", 0.0f);
            set (synth, "osc2On", 1.0f);
            set (synth, "osc2Volume", 100.0f);
            set (synth, "osc2RandomPhase", 0.0f);
            set (synth, "osc2Warp", (float) WaveSynth::Warp::fm);
            set (synth, "osc2WarpAmount", amount);
            set (synth, "osc2FmFrom", (float) from);
            set (synth, "osc5On", fiveOn ? 1.0f : 0.0f);
            set (synth, "osc5Volume", 0.0f);
            set (synth, "osc5Pitch", 7.0f);
            set (synth, "osc1Pitch", 7.0f);
            return play (synth, held ({ 57 }, 0.4), (int) (0.5 * rate));
        };

        const auto plain = modulated (0, 0.0f, false);
        const auto byFive = modulated (5, 60.0f, false), byFiveSilentOn = modulated (5, 60.0f, true);
        const auto byOneOff = modulated (0, 60.0f, false), byOneNamed = modulated (1, 60.0f, false);
        check (! identical (byFive, plain) && identical (byFive, byFiveSilentOn) && identical (modulated (5, 0.0f, false), plain)
                   && identical (byOneOff, plain) && ! identical (byOneNamed, plain),
               "synth: an oscillator named by FM From drives the FM though it is off or at no level, and is never heard itself (FM of nothing from it is the plain wave); "
               "Before still takes oscillator 1, off, as nothing, as FM always has, while naming oscillator 1 brings it in");

        // ...and counts against the unison budget: three heard in 8-voice unison, each driven by two silent
        // oscillators of 4 to 9 in 8-voice unison, is 72 unison voices a note, so five notes at once.
        everything (synth);

        for (int o = 1; o <= 9; ++o)
        {
            set (synth, "osc" + juce::String (o) + "Unison", 8.0f);
            set (synth, "osc" + juce::String (o) + "Warp", (float) WaveSynth::Warp::fm);
            set (synth, "osc" + juce::String (o) + "WarpAmount", 30.0f);
        }

        for (int o = 1; o <= 3; ++o)
            set (synth, "osc" + juce::String (o) + "FmFrom", (float) (o + 3));   // 1 from 4, 2 from 5, 3 from 6

        for (int o = 4; o <= 6; ++o)
            set (synth, "osc" + juce::String (o) + "FmFrom", (float) (o + 3));   // 4 from 7, 5 from 8, 6 from 9

        std::vector<Event> eight;

        for (int i = 0; i < 8; ++i)
            eight.push_back ({ i * 2400, juce::MidiMessage::noteOn (1, 40 + i * 3, 0.8f) });

        int mostWithModulators = 0;
        play (synth, eight, (int) (0.6 * rate), 512, [&] (int) { mostWithModulators = juce::jmax (mostWithModulators, synth.getNumSounding()); });
        check (mostWithModulators == 5, "synth: silent modulators count against the unison budget: three oscillators heard, six driving them two deep, all in 8-voice unison, play "
                                            + juce::String (mostWithModulators) + " notes at once");

        // Each of 4 to 9 sounds, as loud as oscillator 1 would at its settings, and its destinations move it.
        juce::StringArray quiet;

        for (int o = 4; o <= 9; ++o)
        {
            bare (synth);
            set (synth, "osc1On", 0.0f);
            const auto id = "osc" + juce::String (o);
            set (synth, id + "On", 1.0f);
            set (synth, id + "Volume", 100.0f);
            set (synth, id + "RandomPhase", 0.0f);
            const auto alone = play (synth, held ({ 57 }, 0.5), (int) (0.4 * rate));
            const auto level = amplitude (alone, 0, 220.0, 4800, 9600);
            set (synth, "mod1Source", (float) WaveSynth::macro1);
            set (synth, "mod1Destination", (float) (WaveSynth::destinationIds().indexOf (id + "Volume") + 1));
            set (synth, "mod1Amount", -100.0f);
            set (synth, "macro1", 100.0f);
            const auto moved = amplitude (play (synth, held ({ 57 }, 0.5), (int) (0.4 * rate)), 0, 220.0, 4800, 9600);
            set (synth, "mod1Source", 0.0f);

            if (level < 0.3 || moved > level * 0.01)
                quiet.add (juce::String (o));
        }

        check (quiet.isEmpty(), "synth: oscillators 4 to 9 each sound on their own, and a slot turns each one's level down to nothing" + (quiet.isEmpty() ? juce::String() : " (not: " + quiet.joinIntoString (", ") + ")"));

        // Nine sounding: the same at any block size; no click switching 7 on and off in a note, nor in steals.
        std::vector<Event> events;

        for (int i = 0; i < 8; ++i)
        {
            events.push_back ({ 500 + i * 5003, juce::MidiMessage::noteOn (1, 40 + (i * 7) % 19, 0.6f + 0.05f * (float) i) });
            events.push_back ({ 500 + i * 5003 + 11000, juce::MidiMessage::noteOff (1, 40 + (i * 7) % 19) });
        }

        const auto playNine = [&] (int block)
        {
            nine (synth);
            return play (synth, events, (int) (1.5 * rate), block, [&] (int at)
            {
                // Oscillator 7 off at 18,944 samples (a whole number of blocks of every size here), on again at twice that.
                if (at == 18944) set (synth, "osc7On", 0.0f);
                if (at == 2 * 18944) set (synth, "osc7On", 1.0f);
            });
        };

        const auto whole = playNine (512);
        juce::StringArray differ;

        for (auto size : { 1, 37, 64 })
            if (! identical (playNine (size), whole))
                differ.add (juce::String (size));

        const auto click = ClickCheck::worst (whole, rate, 0.03f);
        check (differ.isEmpty() && click.ratio <= 4.0f && whole.getMagnitude (0, whole.getNumSamples()) > 0.05f,
               "synth: nine oscillators sounding, oscillator 7 switched off and on in the notes, render bit for bit the same in blocks of 512, 64, 37 and 1"
                   + (differ.isEmpty() ? juce::String() : " (not: " + differ.joinIntoString (", ") + ")") + " with no click (the nearest a jump "
                   + juce::String (click.ratio, 1) + " times the steps around it)");

        // Notes end: all nine on, a chord released goes to silence (the delay and reverb off, which ring on by design).
        nine (synth);

        for (auto fx : { "Delay", "Reverb", "Chorus", "Phaser" })
            set (synth, "fx" + juce::String (fx) + "On", 0.0f);

        const auto ending = play (synth, held ({ 48, 52, 55 }, 0.3), (int) (1.5 * rate));
        check (ending.getMagnitude (0, (int) (0.3 * rate)) > 0.05f && ending.getMagnitude ((int) (1.3 * rate), (int) (0.2 * rate)) < 1.0e-6f && synth.getNumSounding() == 0,
               "synth: with nine oscillators sounding, a chord released ends: no voice left, and under -120 dB a second after (the drive's tone filter dying away)");

        // The guard: nine in 8-voice unison is 72 unison voices a note, so five notes at once (384 / 72),
        // the oldest fading out as a sixth starts, with no click.
        nine (synth);

        for (int o = 1; o <= 9; ++o)
            set (synth, "osc" + juce::String (o) + "Unison", 8.0f);

        std::vector<Event> chord;

        for (int i = 0; i < 8; ++i)
            chord.push_back ({ i * 2400, juce::MidiMessage::noteOn (1, 40 + i * 3, 0.8f) });

        int most = 0;
        const auto guarded = play (synth, chord, (int) (0.8 * rate), 512, [&] (int) { most = juce::jmax (most, synth.getNumSounding()); });
        const auto stolen = ClickCheck::worst (guarded, rate, 0.03f);
        check (most == WaveSynth::unisonBudget / 72 && synth.getNumSounding() == 5 && stolen.ratio <= 4.0f,
               "synth: nine oscillators in 8-voice unison play " + juce::String (most) + " notes at once (" + juce::String (WaveSynth::unisonBudget)
                   + " unison voices at most), the oldest fading out for a new one with no click (the nearest a jump " + juce::String (stolen.ratio, 1) + " times the steps around it)");
    }

    void testCpu (Synth& synth)
    {
        // Sixteen voices of one oscillator in 8-voice unison, through both filters and every effect; then
        // all three oscillators so, then all nine (the guard lets five notes of that sound: 384 unison voices),
        // and nine in 2-voice unison (288 unison voices, all sixteen notes). The time to render, against the
        // time it plays. (A build without optimisation, as the tests are built, is several times slower than the app.)
        const auto load = [&synth] (int oscillators, int unison)
        {
            everything (synth);

            for (int o = 1; o <= WaveSynth::numOscillators; ++o)
            {
                set (synth, "osc" + juce::String (o) + "On", o <= oscillators ? 1.0f : 0.0f);
                set (synth, "osc" + juce::String (o) + "Unison", (float) unison);
                set (synth, "osc" + juce::String (o) + "Position", 40.0f);
                set (synth, "osc" + juce::String (o) + "Volume", 50.0f);
                set (synth, "osc" + juce::String (o) + "Warp", (float) WaveSynth::Warp::bend);
                set (synth, "osc" + juce::String (o) + "WarpAmount", 30.0f);
            }

            std::vector<Event> e;

            for (int i = 0; i < 16; ++i)
                e.push_back ({ i, juce::MidiMessage::noteOn (1, 36 + i * 2, 0.8f) });

            constexpr auto seconds = 4.0;
            const auto start = juce::Time::getMillisecondCounterHiRes();
            const auto out = play (synth, e, (int) (seconds * rate), 512);
            const auto took = (juce::Time::getMillisecondCounterHiRes() - start) / 1000.0;
            const auto notes = juce::jmin (16, WaveSynth::unisonBudget / (oscillators * unison));
            return std::pair { took / seconds, out.getMagnitude (0, out.getNumSamples()) > 0.0f && synth.getNumSounding() == notes };
        };

        const auto [one, sounded] = load (1, 8);
        const auto [three, soundedAll] = load (3, 8);
        const auto [nineFull, soundedNine] = load (9, 8);
        const auto [nineTwo, soundedNineTwo] = load (9, 2);
       #if defined (__OPTIMIZE__)
        constexpr auto optimised = true;
       #else
        constexpr auto optimised = false;
       #endif
        check (sounded && soundedAll && soundedNine && soundedNineTwo && (! optimised || (one < 0.25 && nineFull < 0.25)),
               "synth: sixteen voices in 8-voice unison through both filters and every effect take " + juce::String (one * 100.0, 1) + " % of a core at 48 kHz with one oscillator, "
                   + juce::String (three * 100.0, 1) + " % with three; nine in 8-voice unison (five notes, as the guard allows) " + juce::String (nineFull * 100.0, 1)
                   + " %, nine in 2-voice unison (sixteen notes) " + juce::String (nineTwo * 100.0, 1) + " %" + (optimised ? " (under 25 % with one, and with nine)" : " (unoptimised build)"));
    }
}

void runWaveSynthTests (AudioEngine& engine)
{
    Synth synth;
    testBasics (synth);
    testControllers (synth);
    testState (engine, synth);
    testEditor (synth);
    testPatches();
    testAliasing (synth);
    testUnison (synth);
    testFilters (synth);
    testModulation (synth);
    testGroundwork();
    testFaults();
    testPresetTexts();
    testBlockSizes (synth);
    testNine (synth);
    testCpu (synth);
    runComplexPatchTests();
    runSynthEventTests();
}

void writePatchHashes (const juce::File& file)
{
    // The factory patches, then the Complex ones, each by name.
    auto all = WaveSynth::factoryPatches();
    all.insert (all.end(), WaveSynth::complexPatches().begin(), WaveSynth::complexPatches().end());
    juce::StringArray lines;

    for (size_t i = 0; i < all.size(); ++i)
    {
        Synth synth;
        synth.loadPatch (all[i].name);
        const auto out = play (synth, held ({ 48, 51, 55 }, 2.0, 0.8f), (int) (4.0 * rate));
        juce::uint64 hash = 1469598103934665603ull;   // FNV-1a over the samples' bits

        for (int ch = 0; ch < out.getNumChannels(); ++ch)
            for (int n = 0; n < out.getNumSamples(); ++n)
            {
                juce::uint32 bits;
                const auto sample = out.getSample (ch, n);
                std::memcpy (&bits, &sample, sizeof (bits));
                hash = (hash ^ bits) * 1099511628211ull;
            }

        lines.add (all[i].name + "|" + juce::String::toHexString ((juce::int64) hash));
    }

    file.replaceWithText (lines.joinIntoString ("\n") + "\n");
    std::cout << lines.size() << " patches" << std::endl;
}

void writeSynthReference (const juce::File& folder)
{
    // Renders that reach what the factory patches don't: every warp on each oscillator with full unison,
    // strum and drift; the FM chain; Sampled tables; level envelopes; odd block sizes; and a synth loaded
    // from a state file. A state file is written the first time and loaded every time after, so a build
    // made later loads the states an earlier one saved. Each render's hash goes to hashes.txt.
    folder.createDirectory();
    juce::StringArray lines;

    const auto hashOf = [] (const juce::AudioBuffer<float>& out)
    {
        juce::uint64 hash = 1469598103934665603ull;

        for (int ch = 0; ch < out.getNumChannels(); ++ch)
            for (int n = 0; n < out.getNumSamples(); ++n)
            {
                juce::uint32 bits;
                const auto sample = out.getSample (ch, n);
                std::memcpy (&bits, &sample, sizeof (bits));
                hash = (hash ^ bits) * 1099511628211ull;
            }

        return juce::String::toHexString ((juce::int64) hash);
    };

    const auto phrase = []
    {
        auto e = held ({ 36, 43, 48 }, 1.2, 0.9f);
        const auto more = held ({ 55, 62 }, 0.8, 0.5f, 0.6);
        e.insert (e.end(), more.begin(), more.end());
        return e;
    };

    std::vector<float> saw (48000), hum (48000);

    for (size_t i = 0; i < saw.size(); ++i)
    {
        saw[i] = (float) (0.6 * (std::fmod ((double) i * 110.0 / rate, 1.0) * 2.0 - 1.0));
        hum[i] = (float) (0.5 * std::sin (juce::MathConstants<double>::twoPi * 165.0 * (double) i / rate) * (0.5 + 0.5 * std::sin ((double) i * 0.0003)));
    }

    const auto sampledSaw = WaveTables::sampledFromAudio (saw.data(), (int) saw.size(), rate, 0.0, "Saw");
    const auto sampledHum = WaveTables::sampledFromAudio (hum.data(), (int) hum.size(), rate, 0.0, "Hum");

    /** A synth set up for a case: three oscillators sounding, the one under test with full unison. */
    const auto setUp = [&] (Synth& s, int warp, int oscillator)
    {
        defaults (s);

        for (int o = 1; o <= 3; ++o)
        {
            const auto id = "osc" + juce::String (o);
            set (s, id + "On", 1.0f);
            set (s, id + "Table", (float) ((o + warp) % 4));
            set (s, id + "Position", 20.0f + 25.0f * (float) o);
            set (s, id + "Pitch", (float) (o == 2 ? 7 : o == 3 ? -12 : 0));
            set (s, id + "Volume", 60.0f);
            set (s, id + "Pan", (float) (o - 2) * 40.0f);
        }

        const auto id = "osc" + juce::String (oscillator);
        set (s, id + "Warp", (float) warp);
        set (s, id + "WarpAmount", 60.0f);
        set (s, id + "Unison", 8.0f);
        set (s, id + "UnisonDetune", 30.0f);
        set (s, id + "UnisonStrum", 20.0f);
        set (s, id + "UnisonDrift", 50.0f);
        set (s, id + "Route", 40.0f);
        set (s, "filter2Type", 5.0f);
        set (s, "mod1Source", (float) WaveSynth::lfo1);
        set (s, "mod1Destination", (float) (WaveSynth::destinationIds().indexOf (id + "Position") + 1));
        set (s, "mod1Amount", 40.0f);
    };

    for (int oscillator = 1; oscillator <= 3; ++oscillator)
        for (int warp = 0; warp < WaveSynth::warpNames().size(); ++warp)
        {
            Synth synth;
            setUp (synth, warp, oscillator);
            lines.add ("warp " + juce::String (warp) + " on osc " + juce::String (oscillator) + "|" + hashOf (play (synth, phrase(), (int) (2.0 * rate))));
        }

    {
        Synth synth;
        setUp (synth, (int) WaveSynth::Warp::fm, 1);

        for (int o = 2; o <= 3; ++o)
        {
            set (synth, "osc" + juce::String (o) + "Warp", (float) WaveSynth::Warp::fm);
            set (synth, "osc" + juce::String (o) + "WarpAmount", 45.0f);
            set (synth, "osc" + juce::String (o) + "Unison", 3.0f);
        }

        lines.add ("fm chain|" + hashOf (play (synth, phrase(), (int) (2.0 * rate))));
    }

    {
        Synth synth;
        setUp (synth, 0, 2);

        for (int o = 1; o <= 3; ++o)
        {
            const auto id = "osc" + juce::String (o) + "Env";
            set (synth, id + "On", 1.0f);
            set (synth, id + "Attack", 5.0f * (float) o);
            set (synth, id + "Hold", 30.0f);
            set (synth, id + "Decay", 200.0f * (float) o);
            set (synth, id + "Sustain", 20.0f * (float) o);
        }

        lines.add ("level envelopes|" + hashOf (play (synth, phrase(), (int) (2.0 * rate))));

        for (auto block : { 37, 64, 1000 })
        {
            Synth again;
            juce::MemoryBlock state;
            synth.getStateInformation (state);
            again.setStateInformation (state.getData(), (int) state.getSize());
            lines.add ("level envelopes in blocks of " + juce::String (block) + "|" + hashOf (play (again, phrase(), (int) (2.0 * rate), block)));
        }
    }

    // States: written once, loaded ever after.
    for (int k = 0; k < 4; ++k)
    {
        const auto file = folder.getChildFile ("state-" + juce::String (k) + ".bin");

        if (! file.existsAsFile())
        {
            Synth synth;
            setUp (synth, k * 2 + 1, k % 3 + 1);

            if (k >= 2)
            {
                set (synth, "osc1Table", (float) WaveTables::Table::sampled);
                synth.setSampledTable (0, sampledSaw);
                set (synth, "osc3Table", (float) WaveTables::Table::sampled);
                synth.setSampledTable (2, sampledHum);
            }

            if (k == 3)
                synth.loadPatch (100);

            juce::MemoryBlock state;
            synth.getStateInformation (state);
            file.replaceWithData (state.getData(), state.getSize());
        }

        juce::MemoryBlock state;
        file.loadFileAsData (state);
        Synth synth;
        synth.setStateInformation (state.getData(), (int) state.getSize());
        lines.add ("state " + juce::String (k) + "|" + hashOf (play (synth, phrase(), (int) (2.0 * rate))));
        lines.add ("state " + juce::String (k) + " in blocks of 37|" + hashOf (play (synth, phrase(), (int) (2.0 * rate), 37)));
    }

    // Factory patches in odd blocks, every 97th.
    for (size_t i = 0; i < WaveSynth::factoryPatches().size(); i += 97)
    {
        Synth synth;
        synth.loadPatch ((int) i);
        lines.add ("patch " + juce::String ((int) i) + " in blocks of 37|" + hashOf (play (synth, phrase(), (int) (2.0 * rate), 37)));
    }

    folder.getChildFile ("hashes.txt").replaceWithText (lines.joinIntoString ("\n") + "\n");
    std::cout << lines.size() << " renders" << std::endl;
}

void saveSynthWindow (const juce::File& folder)
{
    folder.createDirectory();
    Theme::apply (Theme::make ("Dark", "Blue"));
    Synth synth;
    synth.loadPatch ("Abyss Reese");
    std::unique_ptr<juce::AudioProcessorEditor> made (synth.createEditorAndMakeActive());
    auto* window = dynamic_cast<WaveSynthEditor*> (made.get());

    if (window == nullptr)
        return;

    const auto save = [&] (const juce::String& name)
    {
        window->refresh();
        const auto image = window->createComponentSnapshot (window->getLocalBounds(), true, 2.0f);
        auto file = folder.getChildFile ("synth-" + name + ".png");
        file.deleteFile();
        juce::FileOutputStream out (file);
        juce::PNGImageFormat().writeImageToStream (image, out);
        std::cout << file.getFullPathName() << std::endl;
    };

    save ("patch");

    // The oscillators' level envelopes, with slots 9 to 16 beside them.
    set (synth, "osc2EnvOn", 1.0f);
    set (synth, "osc2EnvAttack", 400.0f);
    set (synth, "osc2EnvSustain", 60.0f);
    window->showModulator (WaveSynthEditor::oscLevelsPage);
    window->showRightPage (1);
    save ("levels");

    // LFO 4 on steps, routed in slot 17 to the cutoff, with slots 17 to 24 beside it.
    set (synth, "lfo4Shape", 7.0f);
    set (synth, "lfo4Rate", 4.0f);
    set (synth, "mod17Source", (float) WaveSynth::lfo4);
    set (synth, "mod17Destination", (float) (WaveSynth::destinationIds().indexOf ("filter1Cutoff") + 1));
    set (synth, "mod17Amount", 30.0f);
    window->showModulator (WaveSynthEditor::oscLevelsPage - 1);
    window->showRightPage (2);
    save ("lfo4");

    // Envelope 3 moving slot 17's depth (in slot 18): slot 17's knob shows the ring.
    set (synth, "mod18Source", (float) WaveSynth::env3);
    set (synth, "mod18Destination", (float) (WaveSynth::destinationIds().indexOf ("mod17Amount") + 1));
    set (synth, "mod18Amount", 25.0f);
    save ("depth");
    set (synth, "mod18Source", 0.0f);
    set (synth, "mod17Source", 0.0f);

    // Oscillators 4 to 6: 4 on a vowel, 5 an FM warp taking oscillator 1, 6 off.
    set (synth, "osc4On", 1.0f);
    set (synth, "osc4Table", (float) WaveTables::Table::vowel);
    set (synth, "osc4Position", 40.0f);
    set (synth, "osc5On", 1.0f);
    set (synth, "osc5Table", (float) WaveTables::Table::pulse);
    set (synth, "osc5Warp", (float) WaveSynth::Warp::fm);
    set (synth, "osc5WarpAmount", 45.0f);
    set (synth, "osc5FmFrom", 1.0f);
    set (synth, "osc5Volume", 50.0f);
    window->showModulator (0);
    window->showRightPage (0);
    window->showOscillatorPage (1);
    save ("oscillators-4-6");
    window->showOscillatorPage (0);

    // Oscillator 1 playing a table made from a sound: a sawtooth whose filter closes over two seconds.
    std::vector<float> sound ((size_t) (2.0 * rate));
    float low = 0.0f;

    for (size_t i = 0; i < sound.size(); ++i)
    {
        const auto t = (float) i / (float) sound.size();
        const auto saw = (float) (1.0 - 2.0 * std::fmod (110.0 * (double) i / rate, 1.0));
        low += (0.9f * std::pow (1.0f - t, 3.0f) + 0.01f) * (saw - low);
        sound[i] = low;
    }

    synth.setSampledTable (0, WaveTables::sampledFromAudio (sound.data(), (int) sound.size(), rate, 0.0, "Closing Saw"));
    set (synth, "osc1Table", (float) WaveTables::Table::sampled);
    set (synth, "osc1Position", 35.0f);
    window->showModulator (0);
    window->showRightPage (0);
    save ("sampled");

    // The guide's picture: Turbine Throat with two more layers on oscillators 4 and 5 (a sub an octave down,
    // and a metal edge whose FM takes oscillator 1), C2 held.
    synth.loadPatch ("Turbine Throat");
    set (synth, "osc4On", 1.0f);
    set (synth, "osc4Table", (float) WaveTables::Table::sub);
    set (synth, "osc4Pitch", -12.0f);
    set (synth, "osc4Volume", 45.0f);
    set (synth, "osc5On", 1.0f);
    set (synth, "osc5Table", (float) WaveTables::Table::metal);
    set (synth, "osc5Position", 30.0f);
    set (synth, "osc5Warp", (float) WaveSynth::Warp::fm);
    set (synth, "osc5WarpAmount", 35.0f);
    set (synth, "osc5FmFrom", 1.0f);
    set (synth, "osc5Volume", 25.0f);
    play (synth, { { 0, juce::MidiMessage::noteOn (1, 36, 0.8f) } }, (int) (0.6 * rate));
    window->showOscillatorPage (0);
    window->refresh();
    {
        const auto image = window->createComponentSnapshot (window->getLocalBounds(), true, 2.0f);
        auto file = folder.getChildFile ("guide-synth.png");
        file.deleteFile();
        juce::FileOutputStream out (file);
        juce::PNGImageFormat().writeImageToStream (image, out);
        std::cout << file.getFullPathName() << std::endl;
    }
}

void printPatchLevels()
{
    const auto& all = WaveSynth::factoryPatches();
    Synth synth;

    for (size_t i = 0; i < all.size(); ++i)
    {
        synth.loadPatch ((int) i);
        const auto out = play (synth, held ({ 48, 51, 55 }, 2.0, 0.8f), (int) (4.0 * rate));
        const auto peak = juce::jmax (out.getMagnitude (0, 0, out.getNumSamples()), out.getMagnitude (1, 0, out.getNumSamples()));
        std::cout << all[i].name << "|" << all[i].category << "|" << juce::String (juce::Decibels::gainToDecibels (peak, -100.0f), 1) << std::endl;
    }
}
