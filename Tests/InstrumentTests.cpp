#include <JuceHeader.h>
#include "../Source/Instrument.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/AudioEngine.h"
#include "TestUtils.h"
#include "TestPlugins.h"
#include "InstrumentTests.h"

namespace
{
    using TestUtils::check;
    constexpr double rate = 48000.0;

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

    /** Sets a parameter by its ID to a plain value (a choice's index). */
    void setPlain (juce::AudioPluginInstance& p, const juce::String& id, float plain)
    {
        for (auto* param : p.getParameters())
        {
            if (auto* f = dynamic_cast<juce::AudioParameterFloat*> (param); f != nullptr && f->paramID == id)
                BuiltInEffects::setExactly (*f, plain);
            else if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (param); c != nullptr && c->paramID == id)
                *c = (int) plain;
        }
    }

    float plainOf (juce::AudioPluginInstance& p, const juce::String& id)
    {
        for (auto* param : p.getParameters())
        {
            if (auto* f = dynamic_cast<juce::AudioParameterFloat*> (param); f != nullptr && f->paramID == id)
                return f->get();

            if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (param); c != nullptr && c->paramID == id)
                return (float) c->getIndex();
        }

        return std::numeric_limits<float>::quiet_NaN();
    }

    /** The synth bare: a sine at 0 dB, no envelope, the filter open. */
    void setBare (juce::AudioPluginInstance& synth)
    {
        setPlain (synth, "wave", 0);
        setPlain (synth, "level", 0.0f);
        setPlain (synth, "attack", 0.0f);
        setPlain (synth, "decay", 0.0f);
        setPlain (synth, "sustain", 100.0f);
        setPlain (synth, "release", 0.0f);
        setPlain (synth, "cutoff", 20000.0f);
        setPlain (synth, "resonance", 0.0f);
    }

    bool channelsSame (const juce::AudioBuffer<float>& b)
    {
        for (int ch = 1; ch < b.getNumChannels(); ++ch)
            for (int i = 0; i < b.getNumSamples(); ++i)
                if (! juce::exactlyEqual (b.getSample (ch, i), b.getSample (0, i)))
                    return false;

        return true;
    }

    int countNonZero (const juce::AudioBuffer<float>& b, int channel)
    {
        int n = 0;

        for (int i = 0; i < b.getNumSamples(); ++i)
            n += b.getSample (channel, i) != 0.0f ? 1 : 0;

        return n;
    }


    void testSynthAdditions (BuiltInEffects::BasicSynthProcessor& synth)
    {
        using namespace Instrument;
        const auto reset = [&synth]
        {
            for (auto* p : synth.getParameters())
                p->setValueNotifyingHost (p->getDefaultValue());

            setBare (synth);
        };

        // Oscillators 2 and 3: each at its octave, detune and level, beside the first.
        {
            reset();
            setPlain (synth, "osc2Wave", 0);
            setPlain (synth, "osc2Octave", 1.0f);
            setPlain (synth, "osc2Level", 50.0f);
            setPlain (synth, "osc3Wave", 0);
            setPlain (synth, "osc3Octave", -1.0f);
            setPlain (synth, "osc3Detune", 50.0f);
            setPlain (synth, "osc3Level", 25.0f);
            juce::AudioBuffer<float> out;
            const auto err = render (synth, together ({ 69 }, 1.0, 1.0, 1.0f), rate, 1, out);
            const auto first = amplitude (out, 0, 440.0, 0, 48000), second = amplitude (out, 0, 880.0, 0, 48000);
            const auto third = amplitude (out, 0, 220.0 * std::exp2 (50.0 / 1200.0), 0, 48000), plain220 = amplitude (out, 0, 220.0, 0, 48000);
            check (err.isEmpty() && std::abs (first - 1.0) < 0.01 && std::abs (second - 0.5) < 0.01 && std::abs (third - 0.25) < 0.01 && plain220 < 0.05,
                   "synth: oscillator 2 an octave up at 50 % and oscillator 3 an octave and 50 cents down at 25 % sound at those pitches and levels beside the first ("
                       + juce::String (first, 3) + ", " + juce::String (second, 3) + ", " + juce::String (third, 3) + ")");
        }

        // The filter envelope opens a low cutoff at the note's start and closes over its decay.
        {
            reset();
            setPlain (synth, "wave", 2);
            setPlain (synth, "cutoff", 100.0f);
            juce::AudioBuffer<float> out;
            render (synth, together ({ 45 }, 1.0, 1.0, 1.0f), rate, 1, out);
            const auto staticEarly = amplitude (out, 0, 880.0, 0, 4800), staticLate = amplitude (out, 0, 880.0, 24000, 4800);
            setPlain (synth, "filterEnv", 100.0f);
            setPlain (synth, "filterDecay", 100.0f);
            render (synth, together ({ 45 }, 1.0, 1.0, 1.0f), rate, 1, out);
            const auto early = amplitude (out, 0, 880.0, 0, 4800), late = amplitude (out, 0, 880.0, 24000, 4800);
            check (early > 10.0 * late && early > 10.0 * staticEarly && std::abs (late - staticLate) < 0.1 * staticLate + 1e-4,
                   "synth: with the filter envelope at 100 %, a sawtooth's 8th harmonic through a 100 Hz cutoff is "
                       + juce::String (early / juce::jmax (1e-9, late), 0) + " times as loud at the note's start as half a second on, where it is as without it");
        }

        // Velocity to filter: a soft note is darker than a loud one, against the same note without it.
        {
            reset();
            setPlain (synth, "wave", 2);
            setPlain (synth, "cutoff", 2000.0f);
            const auto brightness = [&synth] (float velocity)
            {
                juce::AudioBuffer<float> out;
                render (synth, together ({ 45 }, 1.0, 1.0, velocity), rate, 1, out);
                return amplitude (out, 0, 1760.0, 4800, 24000) / amplitude (out, 0, 110.0, 4800, 24000);
            };
            const auto softPlain = brightness (0.25f), loudPlain = brightness (1.0f);
            setPlain (synth, "velocityToFilter", 100.0f);
            const auto soft = brightness (0.25f), loud = brightness (1.0f);
            check (std::abs (softPlain - loudPlain) < 1e-3 && std::abs (loud - loudPlain) < 1e-3 && soft < 0.2 * loud,
                   "synth: velocity to filter makes a soft note darker (its 16th harmonic " + juce::String (soft / loud, 3)
                       + " of a loud note's against its fundamental), a full-velocity note as without it, and without it velocity sets the level alone");
        }

        // Glide: a note played while another is held slides there in the glide time, its envelope going on.
        {
            reset();
            setPlain (synth, "attack", 200.0f);
            setPlain (synth, "glide", 100.0f);
            auto phrase = together ({ 57 }, 1.0, 1.2, 1.0f);
            phrase.notes.push_back ({ 69, 0.5, 0.5, 1.0f });
            juce::AudioBuffer<float> out;
            const auto err = render (synth, phrase, rate, 1, out);
            const auto lowAfter = amplitude (out, 0, 220.0, 33600, 4800), highAfter = amplitude (out, 0, 440.0, 33600, 4800);
            const auto midLow = amplitude (out, 0, 220.0, 24480, 960), midHigh = amplitude (out, 0, 440.0, 24480, 960);
            const auto kept = out.getMagnitude (0, 24000, 480);
            reset();
            setPlain (synth, "attack", 200.0f);
            render (synth, phrase, rate, 1, out);
            const auto lowWithout = amplitude (out, 0, 220.0, 33600, 4800);
            const auto restarted = out.getMagnitude (0, 24000, 480);
            check (err.isEmpty() && highAfter > 0.95 && lowAfter < 0.02 && midLow < 0.8 && midHigh < 0.8 && kept > 0.95
                       && lowWithout > 0.95 && restarted > 0.95,
                   "synth: with a 100 ms glide, A4 played over a held A3 slides to it (neither pitch whole 10 ms in, A4 alone 200 ms on) and its 200 ms attack doesn't start again; "
                   "without glide both sound");
        }

        // A synth saved before these existed loads with them off.
        {
            reset();
            setPlain (synth, "osc2Level", 80.0f);
            setPlain (synth, "glide", 50.0f);
            juce::XmlElement old ("TORQWAVE_BASIC_SYNTH");
            old.setAttribute ("version", 1);
            old.setAttribute ("wave", 3);
            old.setAttribute ("cutoff", 1200.0);
            juce::MemoryBlock block;
            juce::AudioProcessor::copyXmlToBinary (old, block);
            BuiltInEffects::BasicSynthProcessor fresh;
            fresh.setStateInformation (block.getData(), (int) block.getSize());
            check (juce::exactlyEqual (plainOf (fresh, "wave"), 3.0f) && std::abs (plainOf (fresh, "cutoff") - 1200.0f) < 0.01f
                       && juce::exactlyEqual (plainOf (fresh, "osc2Level"), 0.0f) && juce::exactlyEqual (plainOf (fresh, "osc3Level"), 0.0f)
                       && juce::exactlyEqual (plainOf (fresh, "filterEnv"), 0.0f) && juce::exactlyEqual (plainOf (fresh, "velocityToFilter"), 0.0f)
                       && juce::exactlyEqual (plainOf (fresh, "glide"), 0.0f),
                   "synth: a synth saved before 0.19.3 loads its settings with oscillators 2 and 3, the filter envelope, velocity to filter and glide off");
        }

        reset();
    }

    void processInto (juce::AudioPluginInstance& p, juce::AudioBuffer<float>& block, juce::MidiBuffer& midi, juce::AudioBuffer<float>& out, int at)
    {
        p.processBlock (block, midi);

        for (int ch = 0; ch < out.getNumChannels(); ++ch)
            out.copyFrom (ch, at, block, ch, 0, block.getNumSamples());
    }

    /** A checksum of every sample (FNV-1a over the floats' bits). */
    juce::uint64 checksum (const juce::AudioBuffer<float>& b)
    {
        juce::uint64 h = 14695981039346656037ull;

        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                juce::uint32 bits;
                const auto s = b.getSample (ch, i);
                std::memcpy (&bits, &s, sizeof bits);
                h = (h ^ bits) * 1099511628211ull;
            }

        return h;
    }

    /** Basic Synth's sound as 0.19.2 made it: three settings (its defaults; a resonant square at
        800 Hz with a slow envelope; a triangle with its filter swept by CC 74), each playing a
        chord, a line, a bend, the mod wheel and the sustain pedal, in 100-sample blocks. */
    juce::uint64 synthSoundChecksum (juce::AudioPluginInstance& synth)
    {
        juce::uint64 h = 0;

        for (int setting = 0; setting < 3; ++setting)
        {
            for (auto* p : synth.getParameters())
                p->setValueNotifyingHost (p->getDefaultValue());

            if (setting == 1)
            {
                setPlain (synth, "wave", 3);
                setPlain (synth, "cutoff", 800.0f);
                setPlain (synth, "resonance", 40.0f);
                setPlain (synth, "attack", 120.0f);
                setPlain (synth, "release", 600.0f);
            }
            else if (setting == 2)
            {
                setPlain (synth, "wave", 1);
                setPlain (synth, "cutoff", 5000.0f);
            }

            synth.prepareToPlay (rate, 100);
            juce::AudioBuffer<float> out (2, 96000), block (2, 100);

            for (int at = 0; at < out.getNumSamples(); at += 100)
            {
                juce::MidiBuffer midi;
                const auto event = [&] (int frame, const juce::MidiMessage& m) { if (frame >= at && frame < at + 100) midi.addEvent (m, frame - at); };

                for (auto note : { 48, 55, 60, 64 })
                {
                    event (0, juce::MidiMessage::noteOn (1, note, 0.8f));
                    event (30000, juce::MidiMessage::noteOff (1, note));
                }

                for (int step = 0; step < 8; ++step)
                {
                    event (36000 + step * 6000, juce::MidiMessage::noteOn (1, 36 + step * 5, (float) (step + 2) / 10.0f));
                    event (36000 + step * 6000 + 4000, juce::MidiMessage::noteOff (1, 36 + step * 5));
                }

                event (10000, juce::MidiMessage::pitchWheel (1, 12000));
                event (20000, juce::MidiMessage::controllerEvent (1, 1, 100));
                event (40000, juce::MidiMessage::controllerEvent (1, 64, 127));
                event (60000, juce::MidiMessage::controllerEvent (1, 64, 0));

                if (setting == 2)
                    for (int k = 0; k < 16; ++k)
                        event (k * 6000, juce::MidiMessage::controllerEvent (1, 74, 127 - k * 8));

                processInto (synth, block, midi, out, at);
            }

            h = (h * 31) ^ checksum (out);
        }

        for (auto* p : synth.getParameters())
            p->setValueNotifyingHost (p->getDefaultValue());

        return h;
    }

    /** Plays notes on the drums (pitch, second, velocity; each note held 10 ms) for seconds, in blocks of blockSize. */
    juce::AudioBuffer<float> drumsPlay (BuiltInEffects::BasicDrumsProcessor& drums, const std::vector<std::tuple<int, double, float>>& notes,
                                        double seconds, int blockSize = 512)
    {
        drums.prepareToPlay (rate, blockSize);
        const auto total = (int) (seconds * rate);
        juce::AudioBuffer<float> out (2, total), block (2, blockSize);

        for (int at = 0; at < total; at += blockSize)
        {
            const auto n = juce::jmin (blockSize, total - at);
            juce::MidiBuffer midi;

            for (auto [pitch, when, velocity] : notes)
            {
                const auto on = (int) (when * rate), off = on + (int) (0.01 * rate);

                if (on >= at && on < at + n)
                    midi.addEvent (juce::MidiMessage::noteOn (10, pitch, velocity), on - at);

                if (off >= at && off < at + n)
                    midi.addEvent (juce::MidiMessage::noteOff (10, pitch), off - at);
            }

            block.setSize (2, n, false, false, true);
            drums.processBlock (block, midi);

            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, at, block, ch, 0, n);
        }

        return out;
    }

    bool sameAudio (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        if (a.getNumSamples() != b.getNumSamples() || a.getNumChannels() != b.getNumChannels())
            return false;

        for (int ch = 0; ch < a.getNumChannels(); ++ch)
            for (int i = 0; i < a.getNumSamples(); ++i)
                if (! juce::exactlyEqual (a.getSample (ch, i), b.getSample (ch, i)))
                    return false;

        return true;
    }

    void testBasicDrums (AudioEngine& engine)
    {
        juce::PluginDescription description;

        for (auto& d : BuiltInEffects::getDescriptions())
            if (d.fileOrIdentifier == BuiltInEffects::basicDrumsId)
                description = d;

        juce::String error;
        auto instance = engine.pluginFormatManager.createPluginInstance (description, rate, 512, error);
        auto* drums = dynamic_cast<BuiltInEffects::BasicDrumsProcessor*> (instance.get());
        check (drums != nullptr && error.isEmpty() && description.isInstrument && description.numInputChannels == 0 && description.name == "Basic Drums"
                   && drums->acceptsMidi() && drums->getTotalNumInputChannels() == 0 && drums->getTotalNumOutputChannels() == 2
                   && drums->getParameters().size() == 9,
               "drums: Basic Drums is made from its description, an instrument with no audio input, a stereo output and nine parameters");

        if (drums == nullptr)
            return;

        // Every General MIDI drum it maps sounds, on both channels alike; other notes are silent.
        const std::vector<int> mapped { 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 56, 57, 59, 69, 70 };
        auto allSound = true, allSilent = true, stereo = true;

        for (auto note : mapped)
        {
            const auto out = drumsPlay (*drums, { { note, 0.0, 1.0f } }, 0.5);
            allSound = allSound && out.getMagnitude (0, 0, out.getNumSamples()) > 0.05f;
            stereo = stereo && channelsSame (out);
        }

        for (auto note : { 0, 34, 52, 53, 55, 60, 64, 71, 127 })
            allSilent = allSilent && drumsPlay (*drums, { { note, 0.0, 1.0f } }, 0.2).getMagnitude (0, 0, (int) (0.2 * rate)) == 0.0f;

        check (allSound && allSilent && stereo && ! BuiltInEffects::BasicDrumsProcessor::soundFor (60).has_value(),
               "drums: each mapped drum note sounds, the same on both channels, and unmapped notes are silent");

        // Velocity scales it exactly (64 is 64/127 of 127); a note's length doesn't matter, the sound plays to its end.
        const auto full = drumsPlay (*drums, { { 36, 0.0, 1.0f } }, 0.5);
        const auto half = drumsPlay (*drums, { { 36, 0.0, 64.0f / 127.0f } }, 0.5);
        auto scaled = true;

        for (int i = 0; i < full.getNumSamples(); ++i)
            scaled = scaled && std::abs (half.getSample (0, i) - 64.0f / 127.0f * full.getSample (0, i)) < 1e-6f;

        check (scaled && full.getMagnitude (0, (int) (0.1 * rate), (int) (0.05 * rate)) > 0.05f,
               "drums: velocity scales a sound exactly, and a kick held 10 ms still sounds 100 ms on");

        // A closed hat chokes the open one; alone the open hat rings on.
        const auto open = drumsPlay (*drums, { { 46, 0.0, 1.0f } }, 0.8);
        const auto choked = drumsPlay (*drums, { { 46, 0.0, 1.0f }, { 42, 0.1, 1.0f } }, 0.8);
        check (open.getMagnitude (0, (int) (0.5 * rate), (int) (0.3 * rate)) > 0.01f
                   && choked.getMagnitude (0, (int) (0.5 * rate), (int) (0.3 * rate)) == 0.0f,
               "drums: a closed hat chokes the open hat, which otherwise rings on");

        // Everything stops: a full kit has ended and gone silent in six seconds.
        std::vector<std::tuple<int, double, float>> kit;

        for (auto note : mapped)
            kit.push_back ({ note, 0.0, 1.0f });

        const auto all = drumsPlay (*drums, kit, 6.0);
        check (drums->getNumSounding() == 0 && all.getMagnitude (0, (int) (5.5 * rate), (int) (0.5 * rate)) == 0.0f,
               "drums: every sound ends, a whole kit silent within six seconds");

        // A pattern renders the same twice and at any block size.
        std::vector<std::tuple<int, double, float>> pattern;

        for (int step = 0; step < 16; ++step)
        {
            const auto at = step * 0.125;

            if (step % 4 == 0) pattern.push_back ({ 36, at, 1.0f });
            if (step % 8 == 4) pattern.push_back ({ 39, at, 0.9f });
            pattern.push_back ({ step % 4 == 2 ? 46 : 42, at, step % 2 == 0 ? 0.8f : 0.6f });
        }

        const auto first = drumsPlay (*drums, pattern, 2.5), again = drumsPlay (*drums, pattern, 2.5), odd = drumsPlay (*drums, pattern, 2.5, 37);
        check (sameAudio (first, again) && sameAudio (first, odd) && first.getMagnitude (0, 0, first.getNumSamples()) < 4.0f,
               "drums: a pattern renders bit-identically twice and in 37-sample blocks");

        // The parameters change the sound and come back with the state.
        setPlain (*drums, "kickTune", -5.0f);
        setPlain (*drums, "hatDecay", 250.0f);
        const auto tuned = drumsPlay (*drums, { { 36, 0.0, 1.0f } }, 0.5);
        juce::MemoryBlock state;
        drums->getStateInformation (state);
        auto another = engine.pluginFormatManager.createPluginInstance (description, rate, 512, error);

        if (another != nullptr)
            another->setStateInformation (state.getData(), (int) state.getSize());

        check (another != nullptr && ! sameAudio (tuned, full) && std::abs (plainOf (*another, "kickTune") + 5.0f) < 1e-4f
                   && std::abs (plainOf (*another, "hatDecay") - 250.0f) < 1e-3f && std::abs (plainOf (*another, "snareTune")) < 1e-4f,
               "drums: tuning changes the kick, and the parameters come back with the state");

        // It is offered with the instruments, after the synth.
        BuiltInEffects::addTo (engine.knownPlugins);
        const auto known = Instrument::known (engine.knownPlugins);
        check (known.size() >= 2 && known[0].fileOrIdentifier == BuiltInEffects::basicSynthId && known[1].fileOrIdentifier == BuiltInEffects::basicDrumsId
                   && Instrument::title (known[1]) == "Basic Drums (built in)",
               "drums: offered as an instrument, Basic Drums (built in), after the synth");
    }

}

void runInstrumentTests (AudioEngine& engine)
{
    using namespace Instrument;

    // Pitches by name and number.
    check (parsePitch ("C4") == 60 && parsePitch ("c4") == 60 && parsePitch ("C-1") == 0 && parsePitch ("G9") == 127 && parsePitch ("F#3") == 54
               && parsePitch ("Bb2") == 46 && parsePitch ("bb2") == 46 && parsePitch (" 69 ") == 69 && parsePitch ("A4") == 69 && parsePitch ("0") == 0
               && ! parsePitch ("H2") && ! parsePitch ("C") && ! parsePitch ("G#9") && ! parsePitch ("128") && ! parsePitch ("C10") && ! parsePitch ("")
               && ! parsePitch ("C4x") && ! parsePitch ("-5") && ! parsePitch ("C-") && ! parsePitch ("#4")
               && pitchName (60) == "C4" && pitchName (61) == "C#4" && pitchName (0) == "C-1" && pitchName (127) == "G9" && pitchName (46) == "A#2",
           "instrument: pitches from names (C4 is 60, sharps and flats, octaves -1 to 9) and numbers, the rest refused, and named back with sharps");

    const auto chord = parsePitches ("C4, E4,G4");
    const auto spaced = parsePitches ("60 64 67");
    check (chord.has_value() && *chord == std::vector<int> { 60, 64, 67 } && spaced.has_value() && *spaced == *chord
               && ! parsePitches ("") && ! parsePitches ("C4, X") && ! parsePitches (", ,") && parsePitches ("C4,,E4").has_value(),
           "instrument: pitches split at commas or spaces, an empty list or a bad one refusing the lot");

    // Phrases: together, in turn, their limits and their MIDI.
    const auto tog = together ({ 60, 64 }, 0.5, 2.0, 0.8f);
    const auto turn = inTurn ({ 60, 64, 67 }, 0.25, 1.0, 1.0f);
    check (tog.notes.size() == 2 && tog.notes[1] == Note { 64, 0.0, 0.5, 0.8f } && juce::exactlyEqual (tog.lengthSeconds, 2.0)
               && turn.notes.size() == 3 && turn.notes[1] == Note { 64, 0.25, 0.25, 1.0f } && turn.notes[2] == Note { 67, 0.5, 0.25, 1.0f }
               && isValid (tog) && isValid (turn)
               && ! isValid (together ({ 128 }, 1.0, 1.0, 1.0f)) && ! isValid (together ({ 60 }, 0.0, 1.0, 1.0f)) && ! isValid (together ({ 60 }, 1.0, 0.0, 1.0f))
               && ! isValid (together ({ 60 }, 1.0, 601.0, 1.0f)) && ! isValid (together ({ 60 }, 1.0, 1.0, 1.5f)) && isValid (together ({}, 1.0, 1.0, 1.0f)),
           "instrument: notes together all start at once, in turn each as the last ends; pitches, holds, lengths and velocities out of range refused");

    {
        const auto midi = midiOf (turn, 1000.0);   // a frame a millisecond
        std::vector<std::tuple<int, bool, int>> seen;   // frame, on, pitch

        for (int i = 0; i < midi.getNumEvents(); ++i)
        {
            const auto& m = midi.getEventPointer (i)->message;
            seen.push_back ({ (int) m.getTimeStamp(), m.isNoteOn(), m.getNoteNumber() });
        }

        const std::vector<std::tuple<int, bool, int>> expected { { 0, true, 60 }, { 250, false, 60 }, { 250, true, 64 }, { 500, false, 64 }, { 500, true, 67 }, { 750, false, 67 } };
        const auto past = midiOf (inTurn ({ 60, 62 }, 1.0, 1.0, 1.0f), 1000.0);
        const auto held = midiOf (together ({ 60 }, 5.0, 1.0, 0.5f), 1000.0);
        check (seen == expected && midi.getEventPointer (0)->message.getChannel() == 1
                   && std::abs (midi.getEventPointer (0)->message.getFloatVelocity() - 1.0f) < 0.01f
                   && past.getNumEvents() == 2 && held.getNumEvents() == 2 && held.getEventPointer (1)->message.getTimeStamp() == 5000.0,
               "instrument: a note-on at each start and a note-off where its hold ends, the off before the on at the same frame, "
               "a note starting at the length left out, one held past it ending past it");
    }

    // The built-in synth, made from its description by the app's format manager.
    juce::PluginDescription synthDescription;

    for (auto& d : BuiltInEffects::getDescriptions())
        if (d.fileOrIdentifier == BuiltInEffects::basicSynthId)
            synthDescription = d;

    juce::String error;
    auto instance = engine.pluginFormatManager.createPluginInstance (synthDescription, rate, 512, error);
    auto* synth = dynamic_cast<BuiltInEffects::BasicSynthProcessor*> (instance.get());
    check (synth != nullptr && error.isEmpty() && synthDescription.isInstrument && synthDescription.numInputChannels == 0
               && synth->acceptsMidi() && ! synth->producesMidi() && synth->getTotalNumInputChannels() == 0 && synth->getTotalNumOutputChannels() == 2
               && synth->getPluginDescription().isInstrument && synth->getParameters().size() == 20,
           "instrument: the basic synth is made from its description, is an instrument that takes MIDI, has no audio input and a stereo output, and twenty parameters");

    if (synth == nullptr)
        return;

    {
        // Its sound as 0.19.2 made it, bit for bit, whatever was added since (checksums for each processor half,
        // as their maths libraries round differently).
        const auto sum = synthSoundChecksum (*synth);
       #if JUCE_ARM || defined (__aarch64__)
        constexpr juce::uint64 expected = 0x8330fed2ec4f2d73ull;
       #else
        constexpr juce::uint64 expected = 0x62848f0e381ec333ull;
       #endif
        check (sum == expected, "instrument: the synth at its 0.19.2 settings sounds as 0.19.2 made it, bit for bit (checksum "
                                    + juce::String::toHexString ((juce::int64) sum) + ")");
    }

    testSynthAdditions (*synth);

    {
        // Bare, A4 for a second in a second and a half: a sine at exactly the velocity, from the start of its cycle, silent after its hold.
        setBare (*synth);
        juce::AudioBuffer<float> out;
        const auto err = render (*synth, together ({ 69 }, 1.0, 1.5, 1.0f), rate, 2, out);
        const auto amp = out.getNumSamples() == 72000 ? amplitude (out, 0, 440.0, 0, 48000) : 0.0;
        auto worst = 0.0f;

        for (int i = 0; i < 480 && out.getNumSamples() == 72000; ++i)
            worst = juce::jmax (worst, std::abs (out.getSample (0, i) - (float) std::sin (juce::MathConstants<double>::twoPi * 440.0 * i / rate)));

        check (err.isEmpty() && out.getNumChannels() == 2 && out.getNumSamples() == 72000 && std::abs (amp - 1.0) < 1e-3 && worst < 1e-5f
                   && out.getMagnitude (0, 48000, 24000) == 0.0f && out.getMagnitude (0, 0, 48000) > 0.99f && channelsSame (out),
               "instrument: the bare synth plays A4 as a sine at exactly its velocity, from its cycle's start, silent once the hold ends, the same on both channels");

        // Velocity and level scale it; the release rings on after the hold and then stops.
        setPlain (*synth, "level", -6.0f);
        setPlain (*synth, "release", 100.0f);
        const auto err2 = render (*synth, together ({ 69 }, 0.5, 1.0, 0.5f), rate, 1, out);
        const auto amp2 = out.getNumSamples() == 48000 ? amplitude (out, 0, 440.0, 0, 24000) : 0.0;
        const auto expected2 = 0.5 * juce::Decibels::decibelsToGain (-6.0);
        check (err2.isEmpty() && out.getNumChannels() == 1 && std::abs (amp2 - expected2) < 2e-3
                   && out.getMagnitude (0, 24000, 2400) > 0.05f && out.getMagnitude (0, 24000 + 4800, 48000 - 24000 - 4800) == 0.0f,
               "instrument: velocity and the level scale the sine; a 100 ms release rings past the hold and is silent 100 ms later");

        // A chord's partials and notes in turn.
        setBare (*synth);
        const auto err3 = render (*synth, together ({ 60, 64, 67 }, 1.0, 1.0, 1.0f), rate, 1, out);
        const auto c = amplitude (out, 0, juce::MidiMessage::getMidiNoteInHertz (60), 0, 48000);
        const auto e = amplitude (out, 0, juce::MidiMessage::getMidiNoteInHertz (64), 0, 48000);
        const auto g = amplitude (out, 0, juce::MidiMessage::getMidiNoteInHertz (67), 0, 48000);
        const auto err4 = render (*synth, inTurn ({ 60, 72 }, 0.5, 1.0, 1.0f), rate, 1, out);
        const auto lowFirst = amplitude (out, 0, juce::MidiMessage::getMidiNoteInHertz (60), 0, 24000);
        const auto highFirst = amplitude (out, 0, juce::MidiMessage::getMidiNoteInHertz (72), 0, 24000);
        const auto lowSecond = amplitude (out, 0, juce::MidiMessage::getMidiNoteInHertz (60), 24000, 24000);
        const auto highSecond = amplitude (out, 0, juce::MidiMessage::getMidiNoteInHertz (72), 24000, 24000);
        check (err3.isEmpty() && err4.isEmpty() && std::abs (c - 1.0) < 0.02 && std::abs (e - 1.0) < 0.02 && std::abs (g - 1.0) < 0.02
                   && lowFirst > 0.95 && highFirst < 0.05 && lowSecond < 0.05 && highSecond > 0.95,
               "instrument: a chord plays each of its notes at full amplitude; notes in turn play one after another");

        // Sixteen voices: a seventeenth note takes the oldest voice over.
        synth->prepareToPlay (rate, 512);
        juce::AudioBuffer<float> block (2, 512);
        juce::MidiBuffer midi;

        for (int i = 0; i < 17; ++i)
            midi.addEvent (juce::MidiMessage::noteOn (1, 40 + i, 1.0f), i);

        synth->processBlock (block, midi);
        const auto sounding = synth->getNumSounding();
        midi.clear();
        midi.addEvent (juce::MidiMessage::noteOff (1, 40), 0);   // the first note's voice was taken: nothing to end
        synth->processBlock (block, midi);
        const auto stillSounding = synth->getNumSounding();
        midi.clear();
        midi.addEvent (juce::MidiMessage::allNotesOff (1), 0);
        synth->processBlock (block, midi);
        midi.clear();
        synth->processBlock (block, midi);
        check (sounding == 16 && stillSounding == 16 && synth->getNumSounding() == 0,
               "instrument: the synth plays sixteen voices, the oldest taken over by a seventeenth note, and All Notes Off ends them all");

        // Its state round-trips, and the sawtooth and square keep their peaks near full scale (no aliasing blow-up).
        setPlain (*synth, "wave", 3);
        setPlain (*synth, "cutoff", 1234.0f);
        juce::MemoryBlock state;
        synth->getStateInformation (state);
        auto second = engine.pluginFormatManager.createPluginInstance (synthDescription, rate, 512, error);
        second->setStateInformation (state.getData(), (int) state.getSize());
        setBare (*synth);
        setPlain (*synth, "wave", 2);
        const auto err5 = render (*synth, together ({ 57 }, 1.0, 1.0, 1.0f), rate, 1, out);
        const auto sawPeak = out.getMagnitude (0, 0, out.getNumSamples());
        const auto sawFundamental = amplitude (out, 0, juce::MidiMessage::getMidiNoteInHertz (57), 0, 48000);
        setPlain (*synth, "wave", 3);
        const auto err6 = render (*synth, together ({ 57 }, 1.0, 1.0, 1.0f), rate, 1, out);
        const auto squarePeak = out.getMagnitude (0, 0, out.getNumSamples());
        const auto squareFundamental = amplitude (out, 0, juce::MidiMessage::getMidiNoteInHertz (57), 0, 48000);
        check (second != nullptr && plainOf (*second, "wave") == 3.0f && std::abs (plainOf (*second, "cutoff") - 1234.0f) < 0.5f
                   && err5.isEmpty() && err6.isEmpty() && sawPeak > 0.9f && sawPeak < 1.2f && std::abs (sawFundamental - 2.0 / juce::MathConstants<double>::pi) < 0.03
                   && squarePeak > 0.9f && squarePeak < 1.3f && std::abs (squareFundamental - 4.0 / juce::MathConstants<double>::pi) < 0.04,
               "instrument: the synth's state restores its wave and cutoff; a sawtooth's and a square's fundamentals are 2/pi and 4/pi of their peaks");

        // The controllers a lane draws: one second of A4 (a sawtooth for the filter) with a controller sent first.
        const auto withController = [&] (std::function<void (juce::MidiBuffer&)> first, int wave, bool keyUp)
        {
            setBare (*synth);
            setPlain (*synth, "wave", (float) wave);
            synth->prepareToPlay (rate, 512);
            juce::AudioBuffer<float> all (2, 48000), b (2, 512);

            for (int at = 0; at < 48000; at += 512)
            {
                juce::MidiBuffer m;

                if (at == 0)
                {
                    first (m);
                    m.addEvent (juce::MidiMessage::noteOn (1, 69, 1.0f), 0);
                }

                if (keyUp && at == 512)
                    m.addEvent (juce::MidiMessage::noteOff (1, 69), 0);

                const auto n = juce::jmin (512, 48000 - at);
                b.setSize (2, n, false, false, true);
                synth->processBlock (b, m);

                for (int ch = 0; ch < 2; ++ch)
                    all.copyFrom (ch, at, b, ch, 0, n);
            }

            return all;
        };
        const auto cc = [] (int number, int value) { return [number, value] (juce::MidiBuffer& m) { m.addEvent (juce::MidiMessage::controllerEvent (1, number, value), 0); }; };
        const auto none = [] (juce::MidiBuffer&) {};

        const auto plain = withController (none, 0, false);
        const auto bent = withController ([] (juce::MidiBuffer& m) { m.addEvent (juce::MidiMessage::pitchWheel (1, 16383), 0); }, 0, false);
        const auto quiet = withController (cc (7, 64), 0, false);
        const auto left = withController (cc (10, 0), 0, false);
        const auto dark = withController (cc (74, 0), 2, false);
        const auto bright = withController (none, 2, false);
        const auto pedalled = withController (cc (64, 127), 0, true);
        const auto unpedalled = withController (none, 0, true);
        const auto wobbly = withController (cc (1, 127), 0, false);

        const auto b4 = 440.0 * std::exp2 (2.0 / 12.0 - 1.0 / 8192.0 * 2.0 / 12.0);
        const auto vibratoWide = amplitude (wobbly, 0, 440.0, 0, 48000) < 0.5;   // a steady A4 is 1: the vibrato spreads it
        check (amplitude (bent, 0, b4, 0, 48000) > 0.95 && amplitude (bent, 0, 440.0, 0, 48000) < 0.05
                   && std::abs (amplitude (quiet, 0, 440.0, 0, 48000) - std::pow (64.0 / 127.0, 2.0)) < 0.01
                   && amplitude (left, 0, 440.0, 0, 48000) > 0.99 && left.getMagnitude (1, 0, 48000) == 0.0f
                   && amplitude (dark, 0, 440.0 * 8, 0, 48000) < 0.1 * amplitude (bright, 0, 440.0 * 8, 0, 48000)
                   && pedalled.getMagnitude (0, 24000, 24000) > 0.5f && unpedalled.getMagnitude (0, 24000, 24000) == 0.0f
                   && vibratoWide && plain.getMagnitude (0, 0, 48000) > 0.99f,
               "instrument: the synth answers its controllers: full pitch bend plays B4 for A4 (two semitones), Volume 64 plays at (64/127) squared, "
               "Pan 0 plays on the left alone, Cutoff 0 takes a sawtooth's 8th harmonic down by more than 20 dB, Sustain holds a note past its key going up, "
               "and the mod wheel's vibrato spreads A4 (under half of it left at 440 Hz)");
    }

    // Rendering through any instrument: latency dropped, a mono output on every channel, the notes delivered in order, stopping.
    {
        TestPlugins::ClickInstrument click (100);
        juce::AudioBuffer<float> out;
        const auto err = render (click, inTurn ({ 60, 62 }, 0.01, 0.05, 1.0f), rate, 2, out);   // notes at frames 0 and 480
        check (err.isEmpty() && out.getNumChannels() == 2 && out.getNumSamples() == 2400
                   && out.getSample (0, 0) == 1.0f && out.getSample (1, 0) == 1.0f && out.getSample (0, 480) == 1.0f && out.getSample (1, 480) == 1.0f
                   && countNonZero (out, 0) == 2 && countNonZero (out, 1) == 2
                   && click.notes == std::vector<std::pair<int, juce::int64>> { { 60, 0 }, { 62, 480 } }
                   && click.offs == std::vector<std::pair<int, juce::int64>> { { 60, 480 }, { 62, 960 } }
                   && click.prepared == 1 && click.released == 1 && juce::exactlyEqual (click.preparedRate, rate) && click.wasNonRealtime,
               "instrument: an instrument's latency is dropped so its sound starts where the note does, a mono output plays on every channel, "
               "the notes arrive at their frames, and it is prepared offline at the rate and released");

        TestPlugins::ClickInstrument late (3000);   // longer than the phrase and than a block
        const auto err2 = render (late, together ({ 60 }, 0.01, 0.02, 1.0f), rate, 1, out);
        check (err2.isEmpty() && out.getNumSamples() == 960 && out.getSample (0, 0) == 1.0f && countNonZero (out, 0) == 1,
               "instrument: a latency longer than the sound itself is still dropped");

        TestPlugins::ClickInstrument stopped (0);
        const auto err3 = render (stopped, together ({ 60 }, 1.0, 2.0, 1.0f), rate, 2, out, [] (float) { return false; });
        check (err3.isNotEmpty() && out.getNumSamples() == 0 && stopped.released == 1,
               "instrument: a render stopped by its progress callback gives nothing and releases the instrument");

        const auto err4 = render (click, together ({ 60 }, 1.0, 0.0, 1.0f), rate, 2, out);
        check (err4.isNotEmpty() && out.getNumSamples() == 0, "instrument: a phrase that can't be played is refused");
    }

    // A fresh instance from a description and a state, so the one with its window open isn't touched.
    {
        setBare (*synth);
        setPlain (*synth, "wave", 1);
        setPlain (*synth, "level", -3.0f);
        juce::MemoryBlock state;
        synth->getStateInformation (state);
        juce::AudioBuffer<float> fromInstance, fromDescription, fromDefaults;
        const auto phrase = together ({ 64 }, 0.5, 0.75, 0.9f);
        const auto err1 = render (*synth, phrase, rate, 2, fromInstance);
        const auto err2 = render (engine.pluginFormatManager, synthDescription, state, phrase, rate, 2, fromDescription);
        const auto err3 = render (engine.pluginFormatManager, synthDescription, {}, phrase, rate, 2, fromDefaults);
        auto same = fromInstance.getNumSamples() == fromDescription.getNumSamples() && fromInstance.getNumSamples() > 0;

        for (int i = 0; same && i < fromInstance.getNumSamples(); ++i)
            same = juce::exactlyEqual (fromInstance.getSample (0, i), fromDescription.getSample (0, i));

        auto differs = false;

        for (int i = 0; ! differs && i < juce::jmin (fromDefaults.getNumSamples(), fromInstance.getNumSamples()); ++i)
            differs = ! juce::exactlyEqual (fromDefaults.getSample (0, i), fromInstance.getSample (0, i));

        juce::PluginDescription unknown = synthDescription;
        unknown.fileOrIdentifier = "torqwave:no-such-instrument";
        juce::AudioBuffer<float> none;
        const auto err4 = render (engine.pluginFormatManager, unknown, {}, phrase, rate, 2, none);
        check (err1.isEmpty() && err2.isEmpty() && err3.isEmpty() && same && differs && err4.isNotEmpty() && none.getNumSamples() == 0,
               "instrument: a fresh instance from the description and state plays exactly what the instance plays, the defaults play otherwise, "
               "and an unknown instrument is an error");
    }

    testBasicDrums (engine);
}
