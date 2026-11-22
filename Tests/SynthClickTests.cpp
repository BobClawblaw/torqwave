#include <JuceHeader.h>
#include "../Source/AudioEngine.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/SequenceFile.h"
#include "../Source/SequenceLibrary.h"
#include "TestUtils.h"
#include "SynthClickTests.h"

// Basic Synth must not click: a note stolen, retriggered, released or silenced by All Sound Off
// fades, and an oscillator starts where it is silent. Three presets that clicked in 0.19.2 are
// played through the synth at the settings they carry, with their notes and lanes at their tempo,
// and a torture case retriggers, steals and silences notes on top. The measure is the largest
// step between two samples: a voice cut or started on a step puts its level there (0.25 to 0.3
// in 0.19.2), where these sounds' own movement stays under 0.12. This file uses only what 0.19.2
// had, so it can be run against that synth too.

namespace
{
    using TestUtils::check;
    constexpr double rate = 48000.0;
    constexpr int block = 256;

    struct Event
    {
        juce::int64 frame;
        juce::MidiMessage message;
    };

    /** A preset's notes and lanes as MIDI at its tempo, with the voice's synth state. */
    struct Played
    {
        juce::String name;
        juce::MemoryBlock state;
        std::vector<Event> events;
        juce::int64 length = 0;
    };

    std::optional<Played> fromPreset (const juce::File& folder, const juce::String& path)
    {
        juce::String error;
        const auto q = SequenceFile::load (folder.getChildFile (path), error);

        if (! q.has_value() || q->voice == nullptr)
            return {};

        Played p;
        p.name = q->name;
        auto* effects = q->voice->getChildByName ("EFFECTS");
        auto* slot = effects != nullptr ? effects->getChildByName ("SLOT") : nullptr;
        auto* state = slot != nullptr ? slot->getChildByName ("STATE") : nullptr;

        if (state == nullptr || ! p.state.fromBase64Encoding (state->getAllSubText()))
            return {};

        const auto framesPerTick = rate * 60.0 / (q->writtenTempo * SequenceFile::ticksPerBeat);
        const auto frame = [framesPerTick] (juce::int64 tick) { return (juce::int64) std::llround ((double) tick * framesPerTick); };

        for (auto& n : q->notes)
        {
            p.events.push_back ({ frame (n.start + n.length), juce::MidiMessage::noteOff (1, n.pitch) });
            p.events.push_back ({ frame (n.start), juce::MidiMessage::noteOn (1, n.pitch, n.velocity) });
        }

        // Lanes, a point every block between their points (128 is the pitch bend).
        for (auto& lane : q->lanes)
            for (size_t i = 0; i + 1 < lane.points.size(); ++i)
            {
                const auto& a = lane.points[i];
                const auto& b = lane.points[i + 1];

                for (auto f = frame (a.tick); f < frame (b.tick); f += block)
                {
                    const auto t = (double) (f - frame (a.tick)) / (double) juce::jmax ((juce::int64) 1, frame (b.tick) - frame (a.tick));
                    const auto v = a.value + (float) t * (b.value - a.value);
                    p.events.push_back ({ f, lane.controller == 128 ? juce::MidiMessage::pitchWheel (1, juce::jlimit (0, 16383, (int) std::lround (v * 16383.0f)))
                                                                    : juce::MidiMessage::controllerEvent (1, juce::jlimit (0, 127, lane.controller), juce::jlimit (0, 127, (int) std::lround (v * 127.0f))) });
                }
            }

        p.length = frame (q->length) + (juce::int64) (2.0 * rate);
        return p;
    }

    /** Plays the events through a fresh synth at the state, in blocks; offs before ons at a frame. */
    juce::AudioBuffer<float> play (AudioEngine& engine, const Played& p)
    {
        juce::PluginDescription description;

        for (auto& d : BuiltInEffects::getDescriptions())
            if (d.fileOrIdentifier == BuiltInEffects::basicSynthId)
                description = d;

        juce::String error;
        auto synth = engine.pluginFormatManager.createPluginInstance (description, rate, block, error);
        juce::AudioBuffer<float> out (2, (int) p.length);
        out.clear();

        if (synth == nullptr)
            return out;

        if (p.state.getSize() > 0)
            synth->setStateInformation (p.state.getData(), (int) p.state.getSize());

        synth->setPlayConfigDetails (0, 2, rate, block);
        synth->setNonRealtime (true);
        synth->prepareToPlay (rate, block);
        auto events = p.events;
        std::stable_sort (events.begin(), events.end(), [] (const Event& a, const Event& b)
        {
            return a.frame < b.frame || (a.frame == b.frame && a.message.isNoteOff() && ! b.message.isNoteOff());
        });
        size_t next = 0;
        juce::AudioBuffer<float> b (2, block);

        for (juce::int64 at = 0; at < p.length; at += block)
        {
            const auto n = (int) juce::jmin ((juce::int64) block, p.length - at);
            juce::MidiBuffer midi;

            while (next < events.size() && events[next].frame < at + n)
            {
                midi.addEvent (events[next].message, (int) juce::jmax ((juce::int64) 0, events[next].frame - at));
                ++next;
            }

            b.setSize (2, n, false, false, true);
            b.clear();
            synth->processBlock (b, midi);

            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, (int) at, b, ch, 0, n);
        }

        synth->releaseResources();
        return out;
    }

    /** The largest step between two samples, and where. */
    std::pair<float, int> largestStep (const juce::AudioBuffer<float>& b)
    {
        auto jump = 0.0f;
        int at = 0;

        for (int ch = 0; ch < b.getNumChannels(); ++ch)
        {
            const auto* x = b.getReadPointer (ch);

            for (int i = 1; i < b.getNumSamples(); ++i)
                if (std::abs (x[i] - x[i - 1]) > jump)
                {
                    jump = std::abs (x[i] - x[i - 1]);
                    at = i;
                }
        }

        return { jump, at };
    }
}

void runSynthClickTests (AudioEngine& engine)
{
    // The limit: just above the steepest a full-scale sine at A5, the highest note these play,
    // moves (2 pi 880 / 48000 = 0.115).
    constexpr float limit = 0.15f;
    std::vector<Played> cases;

    // The presets as the app unpacks them, into a folder of their own.
    const auto before = SequenceLibrary::factoryFolder();
    const auto base = juce::File::createTempFile ("click-presets");
    const auto folder = base.getChildFile ("set");
    SequenceLibrary::setFactoryFolder (folder);
    SequenceLibrary::unpackFactoryPresets();

    for (auto path : { "SFX/Downlifters/Falling Note Ramp.tqseq", "Melody/Bells & Mallets/Minor Bell Motif.tqseq", "SFX/Impacts/Low Boom.tqseq" })
        if (auto p = fromPreset (folder, path))
            cases.push_back (*p);

    SequenceLibrary::setFactoryFolder (before);

    // The unpacked files are read-only: made writable again to be deleted.
    for (const auto& f : juce::RangedDirectoryIterator (base, true, "*", juce::File::findFilesAndDirectories))
        f.getFile().setReadOnly (false);

    base.deleteRecursively();

    // Torture, on Low Boom's voice: a note retriggered while it sounds, seventeen notes so the
    // oldest is stolen, a release mid-attack, and All Sound Off over a chord.
    if (! cases.empty())
    {
        Played t;
        t.name = "retrigger, steal, release and All Sound Off";
        t.state = cases.back().state;
        const auto f = [] (double seconds) { return (juce::int64) (seconds * rate); };

        t.events.push_back ({ f (0.0), juce::MidiMessage::noteOn (1, 45, 1.0f) });
        t.events.push_back ({ f (0.3), juce::MidiMessage::noteOn (1, 45, 1.0f) });     // again while it sounds
        t.events.push_back ({ f (0.61), juce::MidiMessage::noteOn (1, 45, 0.6f) });
        t.events.push_back ({ f (0.9), juce::MidiMessage::noteOff (1, 45) });

        for (int i = 0; i < 17; ++i)                                                    // the seventeenth steals the first
            t.events.push_back ({ f (1.0 + 0.02 * i), juce::MidiMessage::noteOn (1, 36 + i, 0.9f) });

        t.events.push_back ({ f (1.6), juce::MidiMessage::noteOn (1, 30, 1.0f) });     // the eighteenth too
        t.events.push_back ({ f (1.9), juce::MidiMessage::allNotesOff (1) });
        t.events.push_back ({ f (2.5), juce::MidiMessage::noteOn (1, 40, 1.0f) });
        t.events.push_back ({ f (2.5013), juce::MidiMessage::noteOff (1, 40) });       // released 1.3 ms into its attack
        t.events.push_back ({ f (3.0), juce::MidiMessage::noteOn (1, 33, 1.0f) });
        t.events.push_back ({ f (3.0), juce::MidiMessage::noteOn (1, 40, 1.0f) });
        t.events.push_back ({ f (3.0), juce::MidiMessage::noteOn (1, 45, 1.0f) });
        t.events.push_back ({ f (3.4), juce::MidiMessage::allSoundOff (1) });
        t.events.push_back ({ f (3.8), juce::MidiMessage::noteOn (1, 33, 1.0f) });
        t.events.push_back ({ f (4.1), juce::MidiMessage::allSoundOff (1) });
        t.length = f (5.0);
        cases.push_back (t);
    }

    juce::StringArray results, clicks;
    auto loud = true;

    for (auto& c : cases)
    {
        const auto out = play (engine, c);
        const auto [jump, at] = largestStep (out);
        loud = loud && out.getMagnitude (0, out.getNumSamples()) > 0.01f;
        results.add (c.name + " " + juce::String (jump, 3) + " at " + juce::String ((double) at / rate, 3) + " s");

        if (jump > limit)
            clicks.add (c.name);
    }

    check (cases.size() == 4 && loud && clicks.isEmpty(),
           "synth: no clicks: Falling Note Ramp, Minor Bell Motif and Low Boom on their voices, and a retrigger, a steal, a release in the attack "
           "and All Sound Off, each sound's largest step between samples under " + juce::String (limit) + " (" + results.joinIntoString ("; ") + ")");
}
