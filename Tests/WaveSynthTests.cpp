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

        // Every page draws, in dark and light, at twice the scale.
        const auto before = Theme::current();
        auto drawn = true;

        for (auto theme : { "Dark", "Light" })
        {
            Theme::apply (Theme::make (theme, "Blue"));

            for (int page = 0; page < 7; ++page)
            {
                window->showModulator (page);
                window->showRightPage (page % 3);
                window->refresh();
                const auto image = window->createComponentSnapshot (window->getLocalBounds(), true, 2.0f);
                drawn = drawn && image.getWidth() == window->getWidth() * 2;
            }
        }

        Theme::apply (before);
        synth.loadPatch ("Wailing Fuzz Lead");
        window->refresh();
        synth.loadPatch ("Wailing Fuzz Lead");
        window->refresh();
        check (drawn && window->knobFor ("osc1Position") != nullptr && window->knobFor ("osc1Position")->getLabel() == "Bright",
               "synth: every page of its window draws in dark and light at 2x, and a string's Position reads as its brightness");
        editor.reset();
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

        for (auto fx : { "Drive", "Chorus", "Phaser", "Delay", "Reverb" })
            set (synth, "fx" + juce::String (fx) + "On", 1.0f);

        set (synth, "fxDelayMix", 30.0f);

        // The LFOs heard, so their rates are.
        synth.addModulation (WaveSynth::lfo1, "osc2Position", 0.2f);
        synth.addModulation (WaveSynth::lfo2, "filter2Cutoff", 0.1f);
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

        // Slot 16 does what slot 1 does.
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
        const auto one = routed (1), sixteen = routed (16);
        check (WaveSynth::numSlots == 16 && synth.parameter ("mod16Amount") != nullptr && sameSamples (one, sixteen) && ! sameSamples (one, none),
               "synth: sixteen modulation slots, and envelope 2 to the cutoff in slot 16 renders the same as in slot 1, sample for sample");

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

                if (slot > 8 || (name.contains ("Env") && name.startsWith ("osc")))
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
        check (removed == 8 * 3 + 3 * 5 && sameSamples (before, play (third, held ({ 45 }, 0.5), 36000)),
               "synth: a state saved with eight slots and no level envelopes (the " + juce::String (removed) + " new settings taken out of it) loads and plays as it was, sample for sample");

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

    void testCpu (Synth& synth)
    {
        // Sixteen voices of one oscillator in 8-voice unison, through both filters and every effect; then
        // all three oscillators so. The time to render, against the time it plays. (A build without
        // optimisation, as the tests are built, is several times slower than the app.)
        const auto load = [&synth] (int oscillators)
        {
            everything (synth);

            for (int o = 1; o <= 3; ++o)
            {
                set (synth, "osc" + juce::String (o) + "On", o <= oscillators ? 1.0f : 0.0f);
                set (synth, "osc" + juce::String (o) + "Unison", 8.0f);
            }

            std::vector<Event> e;

            for (int i = 0; i < 16; ++i)
                e.push_back ({ i, juce::MidiMessage::noteOn (1, 36 + i * 2, 0.8f) });

            constexpr auto seconds = 4.0;
            const auto start = juce::Time::getMillisecondCounterHiRes();
            const auto out = play (synth, e, (int) (seconds * rate), 512);
            const auto took = (juce::Time::getMillisecondCounterHiRes() - start) / 1000.0;
            return std::pair { took / seconds, out.getMagnitude (0, out.getNumSamples()) > 0.0f && synth.getNumSounding() == 16 };
        };

        const auto [one, sounded] = load (1);
        const auto [three, soundedAll] = load (3);
       #if defined (__OPTIMIZE__)
        constexpr auto optimised = true;
       #else
        constexpr auto optimised = false;
       #endif
        check (sounded && soundedAll && (! optimised || one < 0.25),
               "synth: sixteen voices in 8-voice unison through both filters and every effect take " + juce::String (one * 100.0, 1) + " % of a core at 48 kHz with one oscillator, "
                   + juce::String (three * 100.0, 1) + " % with all three" + (optimised ? " (under 25 % with one)" : " (unoptimised build)"));
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
    testBlockSizes (synth);
    testCpu (synth);
}

void writePatchHashes (const juce::File& file)
{
    const auto& all = WaveSynth::factoryPatches();
    juce::StringArray lines;

    for (size_t i = 0; i < all.size(); ++i)
    {
        Synth synth;
        synth.loadPatch ((int) i);
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
    window->showModulator (6);
    window->showRightPage (1);
    save ("levels");

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
}
