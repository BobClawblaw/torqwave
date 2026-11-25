#include "AiEval.h"
#include "../Source/AiAssistant.h"
#include "../Source/AudioEdits.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/AiWorkspace.h"
#include "../Source/SignalConsole.h"
#include "../Source/Loudness.h"
#include "../Source/DrumMachine.h"
#include "../Source/DrumMaps.h"
#include "../Source/SequenceLibrary.h"
#include "../Source/SwarmGuitar.h"
#include "../Source/WaveSynth.h"
#include "TestUtils.h"

namespace
{
    constexpr double rate = 48000.0;

    double sine (double hz, int i)     { return std::sin (juce::MathConstants<double>::twoPi * hz * i / rate); }

    /** The test files, five seconds of stereo each:
        - tone: one second of silence, three of a 440 Hz tone (left at -12 dBFS, right 6 dB lower), one of silence;
        - steady: the 440 Hz tone all five seconds, both channels at -12 dBFS;
        - hum: a 1 kHz tone at -12 dBFS over a 60 Hz hum at -20 dBFS, both channels;
        - gap: the tone of "tone" from 0 to 1.5 s and from 2.5 to 4 s, silent between and after. */
    enum class TestFile { tone, steady, hum, gap };

    AudioDocument::Snapshot testFile (TestFile kind = TestFile::tone)
    {
        juce::AudioBuffer<float> b (2, (int) rate * 5);
        b.clear();
        const auto g12 = juce::Decibels::decibelsToGain (-12.0), g18 = juce::Decibels::decibelsToGain (-18.0), g20 = juce::Decibels::decibelsToGain (-20.0);

        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const auto t = i / rate;
            double left = 0.0, right = 0.0;

            switch (kind)
            {
                case TestFile::tone:   if (t >= 1.0 && t < 4.0) { left = g12 * sine (440.0, i); right = g18 * sine (440.0, i); } break;
                case TestFile::steady: left = right = g12 * sine (440.0, i); break;
                case TestFile::hum:    left = right = g12 * sine (1000.0, i) + g20 * sine (60.0, i); break;
                case TestFile::gap:    if (t < 1.5 || (t >= 2.5 && t < 4.0)) { left = g12 * sine (440.0, i); right = g18 * sine (440.0, i); } break;
            }

            b.setSample (0, i, (float) left);
            b.setSample (1, i, (float) right);
        }

        return AudioEdits::fromBuffer (std::move (b));
    }

    /** The level in dBFS of one frequency in a channel over a range, by correlation in 0.1 s
        blocks (a whole number of cycles of each test frequency), averaged. Blocks keep a phase
        that drifts slowly, as a pitch shifter's may, from cancelling itself out. */
    double toneDb (const Ai::Workspace& w, double hz, double from, double to, int channel = 0)
    {
        const auto& audio = *w.getAudio();
        const auto block = (int) (rate / 10.0);
        const auto start = (juce::int64) (from * rate), end = juce::jmin ((juce::int64) (to * rate), audio.getNumFrames());
        juce::AudioBuffer<float> samples (audio.getNumChannels(), block);
        double sum = 0.0;
        int blocks = 0;

        for (auto at = start; at + block <= end; at += block, ++blocks)
        {
            audio.read (samples, 0, at, block);
            double re = 0.0, im = 0.0;

            for (int i = 0; i < block; ++i)
            {
                const auto phase = juce::MathConstants<double>::twoPi * hz * (double) i / rate;
                re += samples.getSample (channel, i) * std::cos (phase);
                im += samples.getSample (channel, i) * std::sin (phase);
            }

            sum += 2.0 * std::sqrt (re * re + im * im) / (double) block;
        }

        return juce::Decibels::gainToDecibels (blocks == 0 ? 0.0 : sum / blocks, -200.0);
    }

    double seconds (const Ai::Workspace& w)     { return (double) w.getAudio()->getNumFrames() / rate; }

    double peakDb (const Ai::Workspace& w, double from, double to, int channel = -1)
    {
        const auto levels = AudioEdits::channelLevels (*w.getAudio(), (juce::int64) (from * rate), (juce::int64) (to * rate));
        auto peak = 0.0f;

        for (size_t c = 0; c < levels.size(); ++c)
            if (channel < 0 || (int) c == channel)
                peak = juce::jmax (peak, levels[c].peak);

        return juce::Decibels::gainToDecibels ((double) peak, -200.0);
    }

    bool near (double a, double b, double within)     { return std::abs (a - b) <= within; }

    juce::String parameterText (Ai::Workspace& w, int slot, const juce::String& name)
    {
        juce::var result;
        juce::JSON::parse (w.run ("get_parameters", "{\"slot\": " + juce::String (slot) + ", \"search\": " + name.quoted() + "}"), result);

        if (auto* list = result["parameters"].getArray())
            for (auto& p : *list)
                if (p["name"].toString().equalsIgnoreCase (name))
                    return p["value"].toString();

        return {};
    }

    struct Case
    {
        juce::String request;
        std::function<juce::String (Ai::Workspace&)> check;   // empty when the plan is right, else what is wrong
        TestFile file = TestFile::tone;
        std::function<juce::String (const juce::String&)> checkAnswer = {};   // when what it says matters too
        bool session = false;                                                  // works on testSession() rather than a file
        std::function<Ai::SequenceTools::Context()> sequence = {};             // works on this sequence in the Sequencer rather than a file
        std::function<SessionState()> library = {};                            // works on this session, with the factory sounds, sequences, kits and drum maps to reach
        bool decrypt = false;                                                  // works on decryptSession(), its Bass track playing Torqwave Synth
        std::vector<Ai::Turn> earlier = {};                                    // the conversation before the request, as the window sends it
    };

    /** The session test: Drums and Bass with a clip each over 0 to 4 s, Vocals with one over 1 to
        3 s, and a bus, Reverb. */
    SessionState testSession()
    {
        SessionState s;
        s.sampleRate = rate;
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory);
        SessionEdits::addClip (s, 0, folder.getChildFile ("drums.wav"), testFile (TestFile::steady), 0);
        SessionEdits::addClip (s, 1, folder.getChildFile ("bass.wav"), testFile (TestFile::steady), 0);
        SessionEdits::addClip (s, 2, folder.getChildFile ("vocals.wav"), testFile (TestFile::steady), (juce::int64) rate);

        s.tracks[0].name = "Drums";
        s.tracks[1].name = "Bass";
        s.tracks[2].name = "Vocals";

        SessionEdits::trimClip (s, s.tracks[0].clips[0].id, 0, (juce::int64) (4 * rate));
        SessionEdits::trimClip (s, s.tracks[1].clips[0].id, 0, (juce::int64) (4 * rate));
        SessionEdits::trimClip (s, s.tracks[2].clips[0].id, (juce::int64) rate, (juce::int64) (3 * rate));
        s.addBus().name = "Reverb";
        return s;
    }

    juce::String unchangedLength (const Ai::Workspace& w, double expected)
    {
        return near (seconds (w), expected, 0.01) ? juce::String() : "length " + juce::String (seconds (w), 3) + " s, not " + juce::String (expected) + " s";
    }

    juce::String noPlan (const Ai::Workspace& w)
    {
        return w.getSteps().isEmpty() && ! w.rackChanged() ? juce::String() : "it planned changes: " + w.getSteps().joinIntoString (" | ");
    }

    juce::String planHas (const Ai::Workspace& w, const juce::String& text)
    {
        for (auto& s : w.getSteps())
            if (s.containsIgnoreCase (text))
                return {};

        return "no " + text + " in the plan: " + w.getSteps().joinIntoString (" | ");
    }

    juce::String peaksAre (const Ai::Workspace& w, double left, double right, double within = 0.1)
    {
        const auto l = peakDb (w, 0, seconds (w), 0), r = peakDb (w, 0, seconds (w), 1);
        return near (l, left, within) && near (r, right, within) ? juce::String() : "left " + juce::String (l, 2) + " dB, right " + juce::String (r, 2) + " dB";
    }

    double integrated (const Ai::Workspace& w)
    {
        const auto m = Loudness::measure (*w.getAudio(), 0, w.getAudio()->getNumFrames(), rate);
        return m ? m->integrated : -200.0;
    }

    const SessionTrack* trackNamed (const Ai::Workspace& w, const juce::String& name)
    {
        for (auto& t : w.getSession()->tracks)
            if (t.name.equalsIgnoreCase (name))
                return &t;

        return nullptr;
    }

    /** One note of a factory patch, dry, as a recording of it would be. */
    juce::AudioBuffer<float> synthNote (const juce::String& patch, int note, double hold, double length)
    {
        BuiltInEffects::BasicSynthProcessor synth;
        synth.setPlayConfigDetails (0, 2, rate, 512);
        synth.setNonRealtime (true);
        synth.loadPatch (patch);

        for (auto* effect : { "fxDriveOn", "fxChorusOn", "fxPhaserOn", "fxDelayOn", "fxReverbOn" })
            if (auto* p = synth.parameter (effect))
                p->setValueNotifyingHost (0.0f);

        synth.prepareToPlay (rate, 512);
        const auto total = (int) (length * rate), off = (int) (hold * rate);
        juce::AudioBuffer<float> out (2, total), part (2, 512);

        for (int at = 0; at < total; at += 512)
        {
            const auto n = juce::jmin (512, total - at);
            juce::MidiBuffer midi;
            if (at == 0) midi.addEvent (juce::MidiMessage::noteOn (1, note, 0.8f), 0);
            if (off >= at && off < at + n) midi.addEvent (juce::MidiMessage::noteOff (1, note), off - at);
            part.setSize (2, n, false, false, true);
            part.clear();
            synth.processBlock (part, midi);
            for (int ch = 0; ch < 2; ++ch) out.copyFrom (ch, at, part, ch, 0, n);
        }

        return out;
    }

    /** The Decrypt test: Recording with two clips, "reese note" (Abyss Reese at A2) and "bell hit"
        (Cold Bell at C5); Bass plays Torqwave Synth, Keys has no instrument. */
    SessionState decryptSession()
    {
        SessionState s;
        s.sampleRate = rate;
        auto& recording = s.addTrack();
        recording.name = "Recording";

        const auto addNote = [&] (juce::String name, const juce::String& patch, int note, juce::int64 at)
        {
            SessionClip c;
            c.id = (juce::uint32) (8001 + recording.clips.size());
            c.name = std::move (name);
            c.audio = AudioEdits::fromBuffer (synthNote (patch, note, 0.8, 1.5));
            c.length = c.audio->getNumFrames();
            c.position = at;
            recording.clips.push_back (c);
        };

        addNote ("reese note", "Abyss Reese", 45, 0);
        addNote ("bell hit", "Cold Bell", 72, (juce::int64) (2 * rate));
        s.addInstrumentTrack().name = "Bass";
        s.addInstrumentTrack().name = "Keys";
        return s;
    }

    /** Requests for Torqwave Synth's Decrypt on decryptSession(). */
    std::vector<Case> decryptCases()
    {
        const auto onSession = [] (juce::String request, std::function<juce::String (Ai::Workspace&)> check)
        {
            Case c { std::move (request), std::move (check) };
            c.decrypt = true;
            return c;
        };

        const auto patchFrom = [] (Ai::Workspace& w, bool factoryTablesOnly) -> juce::String
        {
            const auto changes = w.getSynthChanges();

            if (changes.size() != 1 || changes[0].patch == nullptr)
                return "not one patch for the Bass synth in the plan: " + w.getSteps().joinIntoString (" | ");

            if (factoryTablesOnly)
                for (auto& t : changes[0].patch->tables)
                    if (t != nullptr)
                        return "the patch has the clip's table, though it was asked for factory tables only: " + changes[0].step;

            return planHas (w, "reese note");
        };

        return {
            onSession ("Make the Bass synth sound like the clip called reese note.", [=] (Ai::Workspace& w) { return patchFrom (w, false); }),
            onSession ("Decrypt the reese note clip onto the Bass track, with the synth's own tables only, not one made from the clip.",
                       [=] (Ai::Workspace& w) { return patchFrom (w, true); }),
            onSession ("Use the bell hit clip as a wavetable on oscillator 2 of the Bass synth.", [] (Ai::Workspace& w) -> juce::String
            {
                const auto changes = w.getSynthChanges();
                return changes.size() == 1 && changes[0].table != nullptr && changes[0].oscillator == 1 && changes[0].patch == nullptr
                           ? juce::String() : "not one table for oscillator 2 in the plan: " + w.getSteps().joinIntoString (" | ");
            }),
            onSession ("Make the Keys track sound like the bell hit clip.", [] (Ai::Workspace& w) -> juce::String
            {
                // Keys has no Torqwave Synth: nothing may be planned for it (loading one first, then a decrypt, is refused too).
                return w.getSynthChanges().empty() ? juce::String() : "a synth change was planned for a track without Torqwave Synth: " + w.getSteps().joinIntoString (" | ");
            }),
        };
    }

    /** Requests on a session (testSession), for Ask Torq in the multitrack view. */
    std::vector<Case> sessionCases()
    {
        const auto onSession = [] (juce::String request, std::function<juce::String (Ai::Workspace&)> check,
                                   std::function<juce::String (const juce::String&)> checkAnswer = {})
        {
            Case c { std::move (request), std::move (check) };
            c.checkAnswer = std::move (checkAnswer);
            c.session = true;
            return c;
        };

        return {
            onSession ("Make a folder called Rhythm and put the drums and bass in it.", [] (Ai::Workspace& w) -> juce::String
            {
                const auto* folder = trackNamed (w, "Rhythm");
                const auto* drums = trackNamed (w, "Drums");
                const auto* bass = trackNamed (w, "Bass");
                const auto* vocals = trackNamed (w, "Vocals");
                return folder != nullptr && folder->isFolder && drums != nullptr && bass != nullptr && vocals != nullptr
                           && drums->folderId == folder->id && bass->folderId == folder->id && vocals->folderId == 0
                           ? juce::String() : "plan: " + w.getSteps().joinIntoString (" | ");
            }),
            onSession ("Send the vocals to the reverb bus at -8 dB.", [] (Ai::Workspace& w) -> juce::String
            {
                const auto* vocals = trackNamed (w, "Vocals");
                const auto* reverb = trackNamed (w, "Reverb");
                return vocals != nullptr && reverb != nullptr && vocals->sends.size() == 1 && vocals->sends[0].busId == reverb->id
                           && near (vocals->sends[0].gainDb, -8.0, 0.01) ? juce::String() : "plan: " + w.getSteps().joinIntoString (" | ");
            }),
            onSession ("Turn the bass down 4 dB.", [] (Ai::Workspace& w) -> juce::String
            {
                const auto* bass = trackNamed (w, "Bass");
                return bass != nullptr && near (bass->gainDb, -4.0, 0.01) && near (bass->clips[0].gainDb, 0.0, 0.01)
                           ? juce::String() : "plan: " + w.getSteps().joinIntoString (" | ");
            }),
            onSession ("Pan the drums a little to the left.", [] (Ai::Workspace& w) -> juce::String
            {
                const auto* drums = trackNamed (w, "Drums");
                return drums != nullptr && drums->pan <= -0.1f && drums->pan >= -0.6f ? juce::String() : "plan: " + w.getSteps().joinIntoString (" | ");
            }),
            onSession ("I only want to hear the vocals.", [] (Ai::Workspace& w) -> juce::String
            {
                const auto& s = *w.getSession();
                const auto* vocals = trackNamed (w, "Vocals");
                const auto heard = [&s] (const SessionTrack* t) { return t != nullptr && ! s.isMuted (*t) && (! s.anySolo() || s.isSoloed (*t)); };
                return heard (vocals) && ! heard (trackNamed (w, "Drums")) && ! heard (trackNamed (w, "Bass")) && s.tracks.size() == 4
                           ? juce::String() : "plan: " + w.getSteps().joinIntoString (" | ");
            }),
            onSession ("Move the vocal clip so it starts at 2 seconds.", [] (Ai::Workspace& w) -> juce::String
            {
                const auto* vocals = trackNamed (w, "Vocals");
                return vocals != nullptr && vocals->clips.size() == 1 && vocals->clips[0].position == (juce::int64) (2 * rate)
                           && vocals->clips[0].length == (juce::int64) (2 * rate) ? juce::String() : "plan: " + w.getSteps().joinIntoString (" | ");
            }),
            onSession ("Give every clip a half-second fade in.", [] (Ai::Workspace& w) -> juce::String
            {
                for (auto& t : w.getSession()->tracks)
                    for (auto& c : t.clips)
                        if (c.fadeIn != (juce::int64) (rate / 2))
                            return t.name + "'s clip fades in over " + juce::String ((double) c.fadeIn / rate, 3) + " s";

                return {};
            }),
            onSession ("Set the tempo to 128 BPM.", [] (Ai::Workspace& w) -> juce::String
            {
                return near (w.getSession()->tempo, 128.0, 0.001) ? juce::String() : "tempo " + juce::String (w.getSession()->tempo);
            }),
            onSession ("Add a region named Chorus from 4 to 8 seconds.", [] (Ai::Workspace& w) -> juce::String
            {
                const auto& m = w.getSession()->markers;
                return m.size() == 1 && m[0].name == "Chorus" && m[0].start == (juce::int64) (4 * rate) && m[0].getEnd() == (juce::int64) (8 * rate)
                           ? juce::String() : "markers: " + juce::String ((int) m.size());
            }),
            onSession ("Put a compressor on the vocal track.", [] (Ai::Workspace& w) -> juce::String
            {
                // The built-in Compressor added to the Vocals track's chain, and nothing else.
                const auto changes = w.getChainChanges();
                const auto* vocals = trackNamed (w, "Vocals");
                return changes.size() == 1 && vocals != nullptr && changes[0].trackId == vocals->id && changes[0].chain.size() == 1
                           && changes[0].chain[0].plugin.name.containsIgnoreCase ("Compressor") && w.getSession()->tracks.size() == 4
                           ? juce::String() : "plan: " + w.getSteps().joinIntoString (" | ");
            }),
            onSession ("Delete the bass track.", [] (Ai::Workspace& w) -> juce::String
            {
                return trackNamed (w, "Bass") == nullptr && w.getSession()->tracks.size() == 3 ? juce::String() : "plan: " + w.getSteps().joinIntoString (" | ");
            }),
            onSession ("Add a drum track playing Test Drums with a four-on-the-floor kick for the first two bars.", [] (Ai::Workspace& w) -> juce::String
            {
                // At the session's 120 BPM in 4/4: a kick (36) on every beat, 0 to 3.5 s.
                const auto loads = w.getInstrumentLoads();
                const auto& s = *w.getSession();
                const SessionTrack* drums = nullptr;

                for (auto& t : s.tracks)
                    if (! loads.empty() && t.id == loads[0].trackId)
                        drums = &t;

                if (loads.size() != 1 || drums == nullptr || loads[0].plugin.name != "Test Drums")
                    return "plan: " + w.getSteps().joinIntoString (" | ");

                std::set<juce::int64> kicks;

                for (auto& c : drums->noteClips)
                    for (juce::int64 pass = 0; pass < (c.loopLength > 0 ? c.length / c.loopLength + 1 : 1); ++pass)
                        for (auto& n : c.notes)
                            if (const auto at = c.position - c.offset + pass * c.loopLength + n.start;
                                n.pitch == 36 && at >= c.position && at < c.getEnd() && (c.loopLength == 0 || n.start < c.loopLength))
                                kicks.insert (at);

                std::set<juce::int64> wanted;

                for (int beat = 0; beat < 8; ++beat)
                    wanted.insert ((juce::int64) (beat * rate / 2));

                return kicks == wanted ? juce::String() : "kicks at " + juce::String ((int) kicks.size()) + " places; plan: " + w.getSteps().joinIntoString (" | ");
            }),
            onSession ("Add a drum track on Test Drums with its vintage rock kit, and a snare on beats 2 and 4 of the first bar.", [] (Ai::Workspace& w) -> juce::String
            {
                const auto loads = w.getInstrumentLoads();

                if (loads.size() != 1 || loads[0].plugin.name != "Test Drums" || loads[0].preset != "Vintage Rock Kit")
                    return "plan: " + w.getSteps().joinIntoString (" | ");

                const auto* drums = w.getSession()->findTrack (loads[0].trackId);
                std::set<juce::int64> snares;

                for (auto& c : drums->noteClips)
                    for (auto& n : c.notes)
                        if (n.pitch == 38 && (c.loopLength == 0 || n.start < c.loopLength))
                            snares.insert (c.position - c.offset + n.start);

                juce::StringArray times;

                for (auto at : snares)
                    times.add (juce::String ((double) at / rate, 3) + " s");

                return snares == std::set<juce::int64> { (juce::int64) (rate / 2), (juce::int64) (3 * rate / 2) }
                           ? juce::String() : "snares at " + times.joinIntoString (", ") + ", not 0.5 s and 1.5 s; plan: " + w.getSteps().joinIntoString (" | ");
            }),
            onSession ("Add a drum track on Test Drums with its trap kit.", [] (Ai::Workspace& w) -> juce::String
            {
                // There is no trap kit among the saved ones: the right plan is none, saying which there are.
                return w.getInstrumentLoads().empty() && w.getSteps().isEmpty() ? juce::String() : "plan: " + w.getSteps().joinIntoString (" | ");
            }, [] (const juce::String& said) -> juce::String
            {
                return said.containsIgnoreCase ("Vintage Rock") || said.containsIgnoreCase ("Deep House") || said.containsIgnoreCase ("Brushes")
                           ? juce::String() : "it didn't say which kits there are";
            }),
            onSession ("On a new instrument track called Bass, write a bass line of C2, C2, G2, F2 as quarter notes in the first bar.", [] (Ai::Workspace& w) -> juce::String
            {
                const SessionTrack* bass = nullptr;

                for (auto& t : w.getSession()->tracks)
                    if (t.name.equalsIgnoreCase ("Bass") && t.isInstrument)
                        bass = &t;

                if (bass == nullptr || ! bass->isInstrument || bass->noteClips.size() != 1)
                    return "plan: " + w.getSteps().joinIntoString (" | ");

                const auto& c = bass->noteClips[0];
                std::vector<std::pair<juce::int64, int>> heard;

                for (auto& n : c.notes)
                    heard.push_back ({ c.position - c.offset + n.start, n.pitch });

                std::sort (heard.begin(), heard.end());
                const std::vector<std::pair<juce::int64, int>> wanted { { 0, 36 }, { 24000, 36 }, { 48000, 43 }, { 72000, 41 } };
                juce::StringArray got;

                for (auto& [at, pitch] : heard)
                    got.add (juce::String (pitch) + " at " + juce::String ((double) at / rate, 3) + " s");

                return heard == wanted ? juce::String() : "notes " + got.joinIntoString (", ") + "; wanted 36, 36, 43, 41 a quarter note apart";
            }),
            onSession ("Make a new bus called FX Master and have the reverb bus play into it.", [] (Ai::Workspace& w) -> juce::String
            {
                const auto* master = trackNamed (w, "FX Master");
                const auto* reverb = trackNamed (w, "Reverb");
                return master != nullptr && master->isBus && reverb != nullptr && reverb->outputBusId == master->id
                           ? juce::String() : "plan: " + w.getSteps().joinIntoString (" | ");
            }),
        };
    }

    //==============================================================================
    // The library set: the factory sounds, sequences and kits and the saved drum maps.

    const char* const savedMapName = "Sampler - Dark Kit";

    /** A kit laid out its own way: the kick on key 60 and the snare on key 40, each standing for its General MIDI note. */
    std::vector<DrumSound> samplerMap()
    {
        auto map = DrumSound::generalMidi();

        for (auto& d : map)
        {
            if (d.pitch == 36)      { d.pitch = 60; d.standsFor = 36; d.name = "Kit Kick"; }
            else if (d.pitch == 38) { d.pitch = 40; d.standsFor = 38; d.name = "Kit Snare"; }
            else if (d.pitch == 40) { d.pitch = 39; d.standsFor = 40; }
            else if (d.pitch == 39) { d.pitch = 38; d.standsFor = 39; }
        }

        return map;
    }

    /** The drum maps the library set reaches: one saved map, in a folder of its own. */
    DrumMaps::Store& evalMaps()
    {
        static DrumMaps::Store store (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("TorqwaveEvalDrumMaps"));
        return store;
    }

    /** The library set's session, at 120 BPM: instrument tracks Bass, Drums (playing Basic Drums,
        with a one-bar clip, Beat: kicks on the beats, snares on 2 and 4, on General MIDI's keys),
        Guitar, Lead (playing Torqwave Synth) and Hats. With onKit, Drums has the saved map and
        Beat's notes are on its keys. */
    SessionState librarySession (bool onKit = false)
    {
        SessionState s;
        s.sampleRate = rate;
        s.tempo = 120.0;

        for (auto* name : { "Bass", "Drums", "Guitar", "Lead", "Hats" })
            s.addInstrumentTrack().name = name;

        const auto beat = (juce::int64) (rate / 2);
        NoteClip c;
        c.name = "Beat";
        c.length = 4 * beat;
        c.channel = 10;

        for (int i = 0; i < 4; ++i)
        {
            c.notes.push_back ({ 36, i * beat, beat / 4, 0.9f });

            if (i % 2 == 1)
                c.notes.push_back ({ 38, i * beat, beat / 4, 0.8f });
        }

        if (onKit)
        {
            s.tracks[1].drumMap = samplerMap();

            for (auto& n : c.notes)
                n.pitch = DrumSound::toKit (s.tracks[1].drumMap, n.pitch);
        }

        SessionEdits::addNoteClip (s, s.tracks[1].id, c);
        return s;
    }

    const std::vector<SequenceLibrary::Entry>& factoryEntries()
    {
        static const auto entries = SequenceLibrary (SequenceLibrary::factoryFolder()).scan();
        return entries;
    }

    juce::String planOf (const Ai::Workspace& w)     { return "plan: " + w.getSteps().joinIntoString (" | "); }

    /** The one load the plan makes, when it is a factory sound of this instrument on this track; else what is wrong. */
    juce::String factoryLoadOn (const Ai::Workspace& w, const juce::String& track, const juce::String& instrument, Ai::SessionTools::InstrumentLoad& load)
    {
        const auto loads = w.getInstrumentLoads();
        const auto* t = trackNamed (w, track);

        if (loads.size() != 1 || t == nullptr || loads[0].trackId != t->id)
            return juce::String ((int) loads.size()) + " instrument loads; " + planOf (w);

        if (loads[0].plugin.name != instrument || ! loads[0].factory)
            return "it loads " + loads[0].plugin.name + " with " + (loads[0].factory ? "the factory sound " : "the saved preset ") + loads[0].preset.quoted();

        load = loads[0];
        return w.getSession()->tracks.size() == 5 ? juce::String() : "the tracks changed; " + planOf (w);
    }

    /** A factory kit landed at a bar as a drop there lands it: a folder named for a factory kit
        (of this style, or of this name) and, in it, each part's track with the clip
        SequenceKit::place makes at the session's tempo; and a voice to load for each part that
        brings one. Empty when so, else what is wrong. */
    juce::String kitLanded (const Ai::Workspace& w, double tempo, int bar, const juce::String& style, const juce::String& name)
    {
        const auto& state = *w.getSession();

        if (! near (state.tempo, tempo, 0.001))
            return "the tempo is " + juce::String (state.tempo, 2) + ", not " + juce::String (tempo, 2) + "; " + planOf (w);

        const SessionTrack* folder = nullptr;
        const SequenceLibrary::Entry* entry = nullptr;
        int folders = 0;

        for (auto& t : state.tracks)
            if (t.isFolder)
            {
                ++folders;

                for (auto& e : factoryEntries())
                    if (e.isKit && e.name == t.name && (style.isEmpty() || e.style == style) && (name.isEmpty() || e.name == name))
                    {
                        folder = &t;
                        entry = &e;
                    }
            }

        if (folders != 1 || folder == nullptr)
            return juce::String (folders) + " folders, " + (folder == nullptr ? "none" : "one") + " a factory kit's of the kind asked for; " + planOf (w);

        juce::String error;
        const auto kit = SequenceKit::load (entry->file, error);

        if (! kit)
            return "the kit couldn't be read: " + error;

        auto expected = librarySession();
        expected.tempo = tempo;
        const auto placed = SequenceKit::place (expected, *kit, expected.tempoMap().barFrame (bar - 1));
        int voices = 0;

        for (auto& part : placed.parts)
        {
            const auto* want = expected.findTrack (part.trackId);
            const SessionTrack* got = nullptr;

            for (auto& t : state.tracks)
                if (t.folderId == folder->id && t.name == want->name && t.noteClips.size() == 1)
                    got = &t;

            if (got == nullptr)
                return "no track " + want->name.quoted() + " with one clip in the folder; " + planOf (w);

            const auto &a = got->noteClips[0], &b = want->noteClips[0];

            if (a.position != b.position || a.length != b.length || a.loopLength != b.loopLength || ! (a.notes == b.notes))
                return want->name.quoted() + "'s clip is at " + juce::String ((double) a.position / rate, 3) + " s, " + juce::String ((double) a.length / rate, 3) + " s long, "
                       + juce::String ((int) a.notes.size()) + " notes; a drop gives " + juce::String ((double) b.position / rate, 3) + " s, " + juce::String ((double) b.length / rate, 3) + " s, "
                       + juce::String ((int) b.notes.size()) + " notes; " + planOf (w);

            voices += part.effects != nullptr ? 1 : 0;
        }

        return (int) w.getVoiceLoads().size() == voices ? juce::String()
                                                        : juce::String ((int) w.getVoiceLoads().size()) + " voices to load, not " + juce::String (voices) + "; " + planOf (w);
    }

    /** The Beat clip's pitches in order of time, as "36 38 ...". */
    juce::String beatPitches (const Ai::Workspace& w)
    {
        const auto* drums = trackNamed (w, "Drums");
        juce::StringArray pitches;

        if (drums != nullptr && drums->noteClips.size() == 1)
            for (auto& n : drums->noteClips[0].notes)
                pitches.add (juce::String (n.pitch));

        return pitches.joinIntoString (" ");
    }

    /** Requests that need the factory sounds, sequences and kits and the saved drum maps
        (librarySession, the presets the app ships and one saved map), each checked against the
        plan's session and loads. The names asked for are read from what ships, so they are there. */
    std::vector<Case> libraryCases()
    {
        const auto onLibrary = [] (juce::String request, std::function<juce::String (Ai::Workspace&)> check,
                                   std::function<juce::String (const juce::String&)> checkAnswer = {}, bool onKit = false)
        {
            Case c { std::move (request), std::move (check) };
            c.checkAnswer = std::move (checkAnswer);
            c.library = [onKit] { return librarySession (onKit); };
            return c;
        };

        const auto& patches = WaveSynth::factoryPatches();
        const auto patch = patches[patches.size() / 2].name;
        const auto kit = DrumMachine::kits().back().name;
        const auto guitar = EffectPresets::factory (BuiltInEffects::swarmGuitarId).front().name;
        const juce::String ebm = "Industrial/EBM";
        juce::String kitByName;

        for (auto& e : factoryEntries())
            if (e.isKit && e.style != ebm && kitByName.isEmpty())
                kitByName = e.name;

        const juce::String gmBeat = "36 36 38 36 36 38", kitBeat = "60 60 40 60 60 40";

        return {
            onLibrary ("Put a dark reese bass on the Bass track.", [] (Ai::Workspace& w) -> juce::String
            {
                Ai::SessionTools::InstrumentLoad load;

                if (const auto problem = factoryLoadOn (w, "Bass", "Torqwave Synth", load); problem.isNotEmpty())
                    return problem;

                for (auto& p : WaveSynth::factoryPatches())
                    if (p.name == load.preset)
                        return p.category == "Reese" || p.name.containsIgnoreCase ("reese") ? juce::String() : "it chose " + p.name.quoted() + " (" + p.category + "), not a reese";

                return "it chose " + load.preset.quoted() + ", which is no factory patch";
            }),
            onLibrary ("Load the Torqwave Synth patch " + patch + " on the Lead track.", [patch] (Ai::Workspace& w) -> juce::String
            {
                Ai::SessionTools::InstrumentLoad load;
                const auto problem = factoryLoadOn (w, "Lead", "Torqwave Synth", load);
                return problem.isNotEmpty() ? problem : load.preset == patch && load.onlyPreset ? juce::String() : "it chose " + load.preset.quoted();
            }),
            onLibrary ("Put the Torqwave Synth patch called Velvet Harpsichord 9000 on the Guitar track.", [] (Ai::Workspace& w) -> juce::String
            {
                // There is no such patch: the right plan loads nothing.
                return w.getInstrumentLoads().empty() && w.getSession()->tracks.size() == 5 ? juce::String()
                           : "it loads " + (w.getInstrumentLoads().empty() ? juce::String ("nothing") : w.getInstrumentLoads()[0].preset.quoted()) + "; " + planOf (w);
            }),
            onLibrary ("Change the Drums track's kit to " + kit + ".", [kit] (Ai::Workspace& w) -> juce::String
            {
                Ai::SessionTools::InstrumentLoad load;
                const auto problem = factoryLoadOn (w, "Drums", "Basic Drums", load);
                return problem.isNotEmpty() ? problem : load.preset == kit && load.onlyPreset ? juce::String() : "it chose " + load.preset.quoted();
            }),
            onLibrary ("Put StringSwarm on the Guitar track with its preset " + guitar + ".", [guitar] (Ai::Workspace& w) -> juce::String
            {
                Ai::SessionTools::InstrumentLoad load;
                const auto problem = factoryLoadOn (w, "Guitar", "StringSwarm", load);
                return problem.isNotEmpty() ? problem : load.preset == guitar && ! load.onlyPreset ? juce::String() : "it chose " + load.preset.quoted();
            }),
            onLibrary ("Give me an EBM kit at 124.", [ebm] (Ai::Workspace& w) { return kitLanded (w, 124.0, 1, ebm, {}); }),
            onLibrary ("Add the factory kit called " + kitByName + " at bar 9.", [kitByName] (Ai::Workspace& w) { return kitLanded (w, 120.0, 9, {}, kitByName); }),
            onLibrary ("Add the factory kit called Polka Wedding Kit.", [] (Ai::Workspace& w) -> juce::String
            {
                // There is no such kit: the right plan adds nothing.
                return w.getSession()->tracks.size() == 5 && w.getVoiceLoads().empty() ? juce::String() : planOf (w);
            }),
            onLibrary ("Find a techno bass line among the factory sequences and put it on a new track at bar 5.", [] (Ai::Workspace& w) -> juce::String
            {
                const auto& state = *w.getSession();

                if (state.tracks.size() != 6 || ! state.tracks[5].isInstrument || state.tracks[5].noteClips.size() != 1)
                    return juce::String ((int) state.tracks.size()) + " tracks; " + planOf (w);

                const auto& t = state.tracks[5];

                if (t.noteClips[0].position != (juce::int64) (8 * rate))
                    return "the clip starts at " + juce::String ((double) t.noteClips[0].position / rate, 3) + " s, not bar 5's 8 s; " + planOf (w);

                for (auto& e : factoryEntries())
                    if (! e.isKit && e.name == t.name && e.path[0] == "Bass" && SequenceFile::splitStyle (e.style).first == "Techno")
                        return w.getVoiceLoads().size() == 1 && w.getVoiceLoads()[0].trackId == t.id ? juce::String() : "no voice to load for it; " + planOf (w);

                return t.name.quoted() + " is not a factory techno bass sequence; " + planOf (w);
            }),
            onLibrary ("Put a factory hi-hat pattern on the Hats track at bar 3.", [] (Ai::Workspace& w) -> juce::String
            {
                const auto& state = *w.getSession();
                const auto* hats = trackNamed (w, "Hats");

                if (state.tracks.size() != 5 || hats == nullptr || hats->noteClips.size() != 1)
                    return juce::String ((int) state.tracks.size()) + " tracks, " + juce::String (hats != nullptr ? (int) hats->noteClips.size() : 0) + " clips on Hats; " + planOf (w);

                if (hats->noteClips[0].position != (juce::int64) (4 * rate))
                    return "the clip starts at " + juce::String ((double) hats->noteClips[0].position / rate, 3) + " s, not bar 3's 4 s; " + planOf (w);

                for (auto& e : factoryEntries())
                    if (! e.isKit && e.name == hats->noteClips[0].name && e.path[0] == "Drums" && e.path[1] == "Hats & Tops")
                        return w.getVoiceLoads().empty() ? juce::String() : "it loads a voice onto a track that has its own; " + planOf (w);

                return hats->noteClips[0].name.quoted() + " is not one of the factory Hats & Tops; " + planOf (w);
            }),
            onLibrary ("Which drum maps have I saved?", [] (Ai::Workspace& w) { return noPlan (w); }, [] (const juce::String& said) -> juce::String
            {
                return said.containsIgnoreCase ("Dark Kit") ? juce::String() : "the answer doesn't name the saved map";
            }),
            onLibrary ("Give the Drums track the saved drum map " + juce::String (savedMapName) + ". Leave its notes where they are.", [gmBeat] (Ai::Workspace& w) -> juce::String
            {
                const auto* drums = trackNamed (w, "Drums");
                return drums != nullptr && drums->drumMap == samplerMap() && beatPitches (w) == gmBeat ? juce::String()
                           : "the map " + juce::String (drums != nullptr && drums->drumMap == samplerMap() ? "is set" : "is not set") + ", Beat plays " + beatPitches (w) + "; " + planOf (w);
            }),
            onLibrary ("The Drums track is going to play a sampler laid out as my saved drum map: give the track that map and move the Beat clip's notes onto its keys.",
                       [kitBeat] (Ai::Workspace& w) -> juce::String
            {
                const auto* drums = trackNamed (w, "Drums");
                return drums != nullptr && drums->drumMap == samplerMap() && beatPitches (w) == kitBeat ? juce::String()
                           : "the map " + juce::String (drums != nullptr && drums->drumMap == samplerMap() ? "is set" : "is not set") + ", Beat plays " + beatPitches (w) + "; " + planOf (w);
            }),
            onLibrary ("Move the Beat clip's notes back to General MIDI's keys. Keep the track's drum map.", [gmBeat] (Ai::Workspace& w) -> juce::String
            {
                const auto* drums = trackNamed (w, "Drums");
                return drums != nullptr && drums->drumMap == samplerMap() && beatPitches (w) == gmBeat ? juce::String()
                           : "the map " + juce::String (drums != nullptr && drums->drumMap == samplerMap() ? "is kept" : "is gone") + ", Beat plays " + beatPitches (w) + "; " + planOf (w);
            }, {}, true),
        };
    }

    /** A sequence open in the Sequencer: empty, or with these notes; for a part (role) played on an instrument. */
    Ai::SequenceTools::Context sequenceFor (const juce::String& name, const juce::String& role, const juce::String& key, int bars,
                                            const juce::String& instrument, std::vector<SequenceFile::Note> notes = {})
    {
        Ai::SequenceTools::Context c;
        c.sequence.name = name;
        c.sequence.role = role;
        c.sequence.key = key;
        c.sequence.beatsPerBar = 4;
        c.sequence.length = (juce::int64) bars * 4 * SequenceFile::ticksPerBeat;
        c.sequence.notes = std::move (notes);
        c.instrument = instrument;
        c.tempo = 120.0;
        return c;
    }

    /** Requests for Ask Torq in the Sequencer: writing parts, and reviewing one. How good the
        music is depends on the model; these check only that the request was understood. */
    std::vector<Case> sequenceCases()
    {
        const auto onSequence = [] (juce::String request, std::function<Ai::SequenceTools::Context()> sequence,
                                    std::function<juce::String (Ai::Workspace&)> check)
        {
            Case c { std::move (request), std::move (check) };
            c.sequence = std::move (sequence);
            return c;
        };

        const auto written = [] (Ai::Workspace& w) -> const SequenceFile::Sequence*
        {
            const auto* tools = w.getSequenceTools();
            return tools != nullptr && tools->written() ? &tools->getSequence() : nullptr;
        };

        return {
            onSequence ("Write two bars of broken-beat hats in A minor.",
                        [] { return sequenceFor ("Hats", "Drums", "A minor", 2, "Test Drums"); },
                        [written] (Ai::Workspace& w) -> juce::String
            {
                const auto* q = written (w);

                if (q == nullptr)
                    return "nothing was written";

                for (auto& n : q->notes)
                    if (n.pitch != 42 && n.pitch != 44 && n.pitch != 46)
                        return "a note at pitch " + juce::String (n.pitch) + " isn't a hi-hat";

                return q->notes.size() >= 8 ? juce::String() : "only " + juce::String ((int) q->notes.size()) + " hats";
            }),
            onSequence ("Write a pad progression, i-VI-III-VII, a bar each.",
                        [] { return sequenceFor ("Night Pad", "Pads", "A minor", 4, "Test Pad"); },
                        [written] (Ai::Workspace& w) -> juce::String
            {
                const auto* q = written (w);

                if (q == nullptr)
                    return "nothing was written";

                // A minor: Am, F, C, G, one a bar, every pitch class of each chord in its bar.
                const std::vector<std::set<int>> chords { { 9, 0, 4 }, { 5, 9, 0 }, { 0, 4, 7 }, { 7, 11, 2 } };

                for (size_t bar = 0; bar < chords.size(); ++bar)
                {
                    std::set<int> heard;

                    for (auto& n : q->notes)
                        if (n.start / (4 * SequenceFile::ticksPerBeat) == (juce::int64) bar)
                            heard.insert (n.pitch % 12);

                    if (! std::includes (heard.begin(), heard.end(), chords[bar].begin(), chords[bar].end()))
                        return "bar " + juce::String ((int) bar + 1) + " doesn't hold its chord";
                }

                return {};
            }),
            onSequence ("Look at this bass line and tell me how to make it better.",
                        []
                        {
                            return sequenceFor ("Night Bass", "Bass", "A minor", 2, "Test Bass",
                                                { { 45, 0, 960, 0.8f }, { 48, 960, 960, 0.8f }, { 54, 1920, 960, 0.8f }, { 45, 2880, 960, 0.8f },
                                                  { 45, 3840, 960, 0.8f }, { 48, 4800, 960, 0.8f }, { 54, 5760, 960, 0.8f }, { 45, 6720, 960, 0.8f } });
                        },
                        [] (Ai::Workspace& w) -> juce::String
            {
                const auto* tools = w.getSequenceTools();

                if (tools == nullptr || tools->written())
                    return "it wrote the sequence instead of reviewing it";

                if (tools->getSuggestions().empty())
                    return "no suggestions";

                return tools->getReview().contains ("F#") ? juce::String() : "the review doesn't name the F# outside the key: " + tools->getReview();
            }),
            onSequence ("Suggest an effect to give this arp some space.",
                        []
                        {
                            return sequenceFor ("Glass Arp", "Arps", "C major", 1, "Test Synth",
                                                { { 60, 0, 240, 0.8f }, { 64, 480, 240, 0.8f }, { 67, 960, 240, 0.8f }, { 72, 1440, 240, 0.8f },
                                                  { 67, 1920, 240, 0.8f }, { 64, 2400, 240, 0.8f }, { 60, 2880, 240, 0.8f }, { 64, 3360, 240, 0.8f } });
                        },
                        [] (Ai::Workspace& w) -> juce::String
            {
                if (const auto* tools = w.getSequenceTools())
                    for (auto& s : tools->getSuggestions())
                        if (s.kind == Ai::SequenceTools::Suggestion::Kind::effect)
                            return {};

                return "no effect was suggested";
            }),
        };
    }

    /** Requests written after the model's instructions were last changed, and never used to
        change them: what to expect from requests it hasn't been tuned on. */
    std::vector<Case> unseenCases()
    {
        using F = TestFile;
        return {
            { "Cut off the first second.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (auto p = unchangedLength (w, 4.0); p.isNotEmpty()) return p;
                  return peakDb (w, 0.0, 0.1, 0) > -12.5 ? juce::String() : "the tone doesn't start at 0";
              } },
            { "Swap the left and right channels.", [] (Ai::Workspace& w) { return peaksAre (w, -18.0, -12.0); } },
            { "Turn the whole thing down 10 dB.", [] (Ai::Workspace& w) { return peaksAre (w, -22.0, -28.0); } },
            { "Add two seconds of silence to the beginning.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (auto p = unchangedLength (w, 7.0); p.isNotEmpty()) return p;
                  return peakDb (w, 0.0, 2.95) < -100.0 && near (peakDb (w, 3.1, 3.5, 0), -12.0, 0.1) ? juce::String() : "the silence or the tone is in the wrong place";
              } },
            { "Make the right channel as loud as the left one.", [] (Ai::Workspace& w) { return peaksAre (w, -12.0, -12.0, 0.3); } },
            { "Put a marker named Start where the tone begins.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto& m = w.getMarkers();
                  return m.size() == 1 && ! m[0].isRegion() && m[0].name == "Start" && near ((double) m[0].start / rate, 1.0, 0.05)
                             ? juce::String() : "markers: " + juce::String ((int) m.size()) + (m.empty() ? juce::String() : ", first " + m[0].name.quoted() + " at " + juce::String ((double) m[0].start / rate, 3));
              } },
            { "Normalize the peaks to -1 dBFS.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto peak = peakDb (w, 0, seconds (w));
                  return near (peak, -1.0, 0.05) ? juce::String() : "peak " + juce::String (peak, 2) + " dB";
              } },
            { "Get it ready for broadcast at -23 LUFS.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto i = integrated (w);
                  return near (i, -23.0, 0.5) ? juce::String() : "loudness " + juce::String (i, 2) + " LUFS";
              } },
            { "Delete everything after the three-second mark.", [] (Ai::Workspace& w) { return unchangedLength (w, 3.0); } },
            { "Take out the silent gap in the middle.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (! near (seconds (w), 4.0, 0.1)) return "length " + juce::String (seconds (w), 3) + " s, not about 4 s";
                  return peakDb (w, 1.0, 2.5, 0) > -12.5 && peakDb (w, 1.45, 1.6, 0) > -13.0 ? juce::String() : "a gap is still there";
              }, F::gap },
            { "There's a 60 Hz hum under this. Get rid of it without touching the tone.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto hum = toneDb (w, 60.0, 1.0, 4.0), tone = toneDb (w, 1000.0, 1.0, 4.0);
                  return hum < -35.0 && near (tone, -12.0, 1.0) ? juce::String() : "60 Hz at " + juce::String (hum, 1) + " dB, 1 kHz at " + juce::String (tone, 1) + " dB";
              }, F::hum },
            { "Fade in the tone over its first second.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (auto p = unchangedLength (w, 5.0); p.isNotEmpty()) return p;
                  const auto start = peakDb (w, 1.0, 1.05, 0), later = peakDb (w, 2.1, 2.5, 0);
                  return start < later - 15.0 && later > -12.5 ? juce::String() : "start " + juce::String (start, 1) + " dB, after the fade " + juce::String (later, 1) + " dB";
              } },
            { "Fade in the first second and fade out the last second.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (auto p = unchangedLength (w, 5.0); p.isNotEmpty()) return p;
                  const auto in = peakDb (w, 0.0, 0.02), out = peakDb (w, 4.98, 5.0), middle = peakDb (w, 2.0, 3.0);
                  return in < -30.0 && out < -30.0 && near (middle, -12.0, 0.2) ? juce::String()
                                                                                 : "start " + juce::String (in, 1) + " dB, end " + juce::String (out, 1) + " dB, middle " + juce::String (middle, 1) + " dB";
              }, F::steady },
            { "Insert half a second of silence at the two-second mark.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (auto p = unchangedLength (w, 5.5); p.isNotEmpty()) return p;
                  return peakDb (w, 2.05, 2.45) < -100.0 && near (peakDb (w, 2.6, 3.0), -12.0, 0.1) ? juce::String() : "the silence isn't at 2 s";
              }, F::steady },
            { "Flip the polarity of the left channel only.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (auto p = unchangedLength (w, 5.0); p.isNotEmpty()) return p;
                  juce::AudioBuffer<float> b (2, 1);
                  w.getAudio()->read (b, 0, (juce::int64) (rate / 1760.0), 1);   // a quarter cycle in: the tone's crest
                  return b.getSample (0, 0) < -0.2f && b.getSample (1, 0) > 0.2f ? juce::String()
                                                                                  : "left " + juce::String (b.getSample (0, 0), 3) + ", right " + juce::String (b.getSample (1, 0), 3);
              }, F::steady },
            { "Make it twice as long without changing the pitch.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (! near (seconds (w), 10.0, 0.1)) return "length " + juce::String (seconds (w), 3) + " s, not 10 s";
                  const auto a = toneDb (w, 440.0, 3.0, 6.0);
                  return a > -16.0 ? juce::String() : "440 Hz at " + juce::String (a, 1) + " dB";
              }, F::steady },
            { "Raise the pitch by an octave but keep the length.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (auto p = unchangedLength (w, 5.0); p.isNotEmpty()) return p;
                  const auto up = toneDb (w, 880.0, 1.0, 4.0), old = toneDb (w, 440.0, 1.0, 4.0);
                  return up > -18.0 && old < -30.0 ? juce::String() : "880 Hz at " + juce::String (up, 1) + " dB, 440 Hz at " + juce::String (old, 1) + " dB";
              }, F::steady },
            { "Add a region called Middle from 2 to 3 seconds.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto& m = w.getMarkers();
                  return m.size() == 1 && m[0].name == "Middle" && near ((double) m[0].start / rate, 2.0, 0.01) && near ((double) m[0].getEnd() / rate, 3.0, 0.01)
                             ? juce::String() : "markers: " + juce::String ((int) m.size());
              } },
            { "Don't change anything, just tell me how loud this file is in LUFS.", [] (Ai::Workspace& w) { return noPlan (w); },
              F::tone, [] (const juce::String& said) -> juce::String
              {
                  return said.containsIgnoreCase ("LUFS") ? juce::String() : "the answer gives no loudness in LUFS";
              } },
            { "Upload this file to SoundCloud.", [] (Ai::Workspace& w) { return noPlan (w); } },
            { "Make it louder, but don't let it clip.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto file = testFile();
                  const auto before = Loudness::measure (*file, 0, file->getNumFrames(), rate);
                  const auto peak = peakDb (w, 0, seconds (w)), now = integrated (w);
                  return before && peak <= 0.0 && now >= before->integrated + 3.0 ? juce::String()
                                                                                  : "peak " + juce::String (peak, 2) + " dB, loudness " + juce::String (now, 2) + " LUFS";
              } },
            { "Add some light chorus to the whole file.", [] (Ai::Workspace& w) { return planHas (w, "chorus"); }, F::steady },
            { "Put the built-in Limiter in the rack with its ceiling at -1 dB, but don't apply it.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (w.audioChanged()) return "the audio changed";
                  const auto rack = w.createRackXml();
                  if (! w.rackChanged() || rack == nullptr || rack->getNumChildElements() != 1) return "the rack doesn't hold one plug-in";
                  const auto ceiling = parameterText (w, 1, "Ceiling");
                  return near (ceiling.getDoubleValue(), -1.0, 0.01) ? juce::String() : "ceiling " + ceiling;
              } },
            { "Make the left channel 3 dB quieter and leave the right alone.", [] (Ai::Workspace& w) { return peaksAre (w, -15.0, -18.0); } },
            { "Mix it down to mono.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (w.getAudio()->getNumChannels() == 1) return {};
                  juce::AudioBuffer<float> b (2, 1);
                  w.getAudio()->read (b, 0, (juce::int64) (2.0 * rate + rate / 1760.0), 1);
                  return std::abs (b.getSample (0, 0) - b.getSample (1, 0)) < 1.0e-4f ? juce::String() : "the channels differ";
              } },
        };
    }

    /** Follow-ups: each request only makes sense with the conversation before it, which goes to the
        model as the window sends it (what was asked, what was answered, the plan, and whether it was
        applied). The file is as it would be now: an applied plan is in it already. */
    std::vector<Case> followUpCases()
    {
        using Outcome = Ai::Turn::Outcome;
        const auto nothing = [] (Ai::Workspace& w) -> juce::String
        {
            return w.getSteps().isEmpty() && ! w.audioChanged() ? juce::String() : "it planned " + juce::String (w.getSteps().size()) + " steps for a question";
        };

        Case less { "Too much. Take half of that back off.", [] (Ai::Workspace& w) -> juce::String
                    {
                        const auto now = peakDb (w, 2.0, 3.0);
                        return near (now, -15.0, 0.6) ? juce::String() : "peak " + juce::String (now, 1) + " dB, not 3 dB under the -12 dB it was";
                    } };
        less.earlier = { { "Make it 6 dB louder", "Raised by 6 dB.", { "Gain 6.00 dB, the whole file" }, Outcome::applied } };

        Case again { "Do that again, but over the last two seconds of the sound this time.", [] (Ai::Workspace& w) -> juce::String
                     {
                         if (! near (seconds (w), 5.0, 0.001))
                             return "the length changed";

                         const auto end = peakDb (w, 3.98, 4.0), middle = peakDb (w, 2.9, 3.1), before = peakDb (w, 1.2, 1.8);
                         return end < before - 20.0 && middle < before - 3.0 && middle > before - 12.0 && before > -12.5
                                    ? juce::String() : "peaks: before " + juce::String (before, 1) + ", a second from the end " + juce::String (middle, 1) + ", at the end " + juce::String (end, 1) + " dB";
                     } };
        again.earlier = { { "Fade out the last half second of the sound.", "Faded out the last half second of the tone.",
                            { "Fade Out, 3.500 s to 4.000 s" }, Outcome::discarded } };

        Case recall { "What did I ask you for just before this?", nothing, TestFile::tone,
                      [] (const juce::String& said) { return said.containsIgnoreCase ("normal") ? juce::String() : "it did not name the request to normalize"; } };
        recall.earlier = { { "Trim the silence off both ends.", "Trimmed the silence: the file is 3 s long now.", { "Trim to 1.000 s to 4.000 s" }, Outcome::applied },
                           { "Normalize it to -1 dB.", "Normalized to a peak of -1 dB.", { "Normalize to -1.00 dB, the whole file" }, Outcome::pending } };

        Case whatsNew { "What is new in this version of Torqwave?", nothing, TestFile::tone,
                        [] (const juce::String& said)
                        {
                            return said.containsIgnoreCase ("Modern") || said.containsIgnoreCase ("drum") || said.containsIgnoreCase ("Maximizer")
                                       ? juce::String() : "it named nothing from the release's notes";
                        } };

        return { less, again, recall, whatsNew };
    }

    std::vector<Case> cases()
    {
        return {
            { "Trim the silence off both ends.", [] (Ai::Workspace& w) -> juce::String
              {
                  return near (seconds (w), 3.0, 0.05) ? juce::String() : "length " + juce::String (seconds (w), 3) + " s, not 3 s";
              } },
            { "Make the whole file -20 LUFS integrated.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto m = Loudness::measure (*w.getAudio(), 0, w.getAudio()->getNumFrames(), rate);
                  return m && near (m->integrated, -20.0, 0.5) ? juce::String() : "loudness " + juce::String (m ? m->integrated : 0.0, 2) + " LUFS";
              } },
            { "Fade out the last half second of the sound.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (! near (seconds (w), 5.0, 0.001))
                      return "the length changed";

                  const auto end = peakDb (w, 3.98, 4.0), before = peakDb (w, 3.0, 3.4);
                  return end < before - 20.0 && before > -12.5 ? juce::String()
                                                               : "peak near the end " + juce::String (end, 1) + " dB, before the fade " + juce::String (before, 1) + " dB";
              } },
            { "Add a region named Tone over the part that has sound in it.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto& m = w.getMarkers();
                  return m.size() == 1 && m[0].name == "Tone" && near ((double) m[0].start / rate, 1.0, 0.05) && near ((double) m[0].getEnd() / rate, 4.0, 0.05)
                             ? juce::String() : "markers: " + juce::String ((int) m.size()) + (m.empty() ? juce::String() : ", first " + m[0].name.quoted() + " at " + juce::String ((double) m[0].start / rate, 3) + " to " + juce::String ((double) m[0].getEnd() / rate, 3));
              } },
            { "Put the built-in Compressor in the effects rack with a 4:1 ratio and a threshold of -24 dB. Don't render it into the audio.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (w.audioChanged())
                      return "the audio changed";

                  const auto rack = w.createRackXml();

                  if (! w.rackChanged() || rack == nullptr || rack->getNumChildElements() != 1)
                      return "the rack doesn't hold one plug-in";

                  const auto ratio = parameterText (w, 1, "Ratio"), threshold = parameterText (w, 1, "Threshold");
                  return near (ratio.getDoubleValue(), 4.0, 0.01) && near (threshold.getDoubleValue(), -24.0, 0.01) ? juce::String() : "ratio " + ratio + ", threshold " + threshold;
              } },
            { "Raise the gain by 6 dB, then limit it so the peaks stay under -6 dB.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto peak = peakDb (w, 0.0, 5.0);
                  const auto rms = AudioEdits::channelLevels (*w.getAudio(), (juce::int64) (2 * rate), (juce::int64) (3 * rate))[0].rms;
                  return peak <= -5.9 && peak > -6.6 && juce::Decibels::gainToDecibels (rms) > -15.5 ? juce::String()
                                                                                                     : "peak " + juce::String (peak, 2) + " dB, left RMS " + juce::String (juce::Decibels::gainToDecibels (rms), 2) + " dB";
              } },
            { "Reverse the second half of the file.", [] (Ai::Workspace& w) -> juce::String
              {
                  for (auto& s : w.getSteps())
                      if (s.startsWith ("Reverse") && s.contains ("2.500 s to 5.000 s"))
                          return {};

                  return "steps: " + w.getSteps().joinIntoString (" | ");
              } },
            { "Normalize each channel on its own to -3 dB.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto left = peakDb (w, 0, 5, 0), right = peakDb (w, 0, 5, 1);
                  return near (left, -3.0, 0.05) && near (right, -3.0, 0.05) ? juce::String() : "left " + juce::String (left, 2) + " dB, right " + juce::String (right, 2) + " dB";
              } },
            { "Make it sound like it was played in a big concert hall.", [] (Ai::Workspace& w) -> juce::String
              {
                  for (auto& s : w.getSteps())
                      if (s.containsIgnoreCase ("reverb"))
                          return {};

                  return "no reverb in the plan: " + w.getSteps().joinIntoString (" | ");
              } },
            { "Write out the words spoken in this recording.", [] (Ai::Workspace& w) -> juce::String
              {
                  return w.getSteps().isEmpty() ? juce::String() : "it planned changes: " + w.getSteps().joinIntoString (" | ");
              } },
        };
    }
}

int runAiEval (const juce::String& serverUrl, const juce::String& model, const juce::String& key, const juce::String& set)
{
    Ai::Settings settings;
    settings.enabled = true;
    settings.serverUrl = serverUrl;
    settings.model = model;

    juce::AudioPluginFormatManager formats;
    formats.addFormat (BuiltInEffects::createFormat());

    int failed = 0, n = 0;
    double totalSeconds = 0.0;

    if (set == "library")
    {
        // The presets the app ships, in the tests' own folder, and one saved drum map in a folder of its own.
        if (const auto error = SequenceLibrary::unpackFactoryPresets(); error.isNotEmpty())
        {
            std::cout << "The factory presets could not be unpacked: " << error << std::endl;
            return 1;
        }

        evalMaps().save (savedMapName, samplerMap());
    }

    auto chosen = set == "unseen" ? unseenCases() : set == "session" ? sessionCases() : set == "sequence" ? sequenceCases()
                : set == "library" ? libraryCases() : set == "decrypt" ? decryptCases() : set == "followup" ? followUpCases() : cases();

    if (set == "all")
        for (auto& c : unseenCases())
            chosen.push_back (c);

    for (auto& c : chosen)
    {
        ++n;
        Ai::Workspace::Start start;

        if (c.library != nullptr)
        {
            start.session = c.library();
            start.sessionName = "Song";

            // Drums plays Basic Drums and Lead Torqwave Synth; the rest have no instrument yet.
            for (auto& d : BuiltInEffects::getDescriptions())
            {
                if (d.name == "Basic Drums")    start.instruments[start.session->tracks[1].id] = d;
                if (d.name == "Torqwave Synth") start.instruments[start.session->tracks[3].id] = d;
            }

            start.library = Ai::Library::standard();
            start.library.transposeDrops = false;
            start.library.drumMapNames = [] { return evalMaps().names(); };
            start.library.drumMap = [] (const juce::String& name) { return evalMaps().load (name); };
        }
        else if (c.session)
        {
            start.session = testSession();
            start.sessionName = "Song";
        }
        else if (c.decrypt)
        {
            start.session = decryptSession();
            start.sessionName = "Song";

            for (auto& d : BuiltInEffects::getDescriptions())
                if (d.name == "Torqwave Synth")
                    start.instruments[start.session->tracks[1].id] = d;
        }
        else if (c.sequence != nullptr)
            start.sequence = c.sequence();
        else
            start.audio = testFile (c.file);

        start.sampleRate = rate;
        start.fileName = "test tone.wav";
        start.plugins = BuiltInEffects::getDescriptions();

        // An instrument for the session requests to choose (it is never loaded here).
        juce::PluginDescription drumKit;
        drumKit.name = "Test Drums";
        drumKit.manufacturerName = "Torqwave";
        drumKit.pluginFormatName = "VST3";
        drumKit.fileOrIdentifier = "/Library/Audio/Plug-Ins/VST3/Test Drums.vst3";
        drumKit.category = "Drum";
        drumKit.isInstrument = true;
        start.plugins.add (drumKit);
        start.instrumentPresets = [] (const juce::PluginDescription& d)
        {
            return d.name == "Test Drums" ? juce::StringArray { "Deep House Kit", "Vintage Rock Kit", "Brushes" } : juce::StringArray();
        };
        start.formats = &formats;

        Ai::Agent agent (settings, key, std::move (start), c.request, c.earlier);
        juce::String answer, error;
        bool done = false;
        int calls = 0, refused = 0;
        agent.onTool = [&] (const juce::String&, const juce::String&, const juce::String& result)
        {
            ++calls;
            refused += result.contains ("\"ok\": false") ? 1 : 0;
        };
        agent.onDone = [&] (const juce::String& a, const juce::String& e) { answer = a; error = e; done = true; };

        const auto started = juce::Time::getMillisecondCounterHiRes();
        agent.start();
        TestUtils::runUntil ([&] { return done; }, 15 * 60 * 1000);
        const auto took = (juce::Time::getMillisecondCounterHiRes() - started) / 1000.0;
        totalSeconds += took;

        auto problem = ! done ? juce::String ("no answer in 15 minutes") : error.isNotEmpty() ? error : c.check (agent.getWorkspace());

        if (problem.isEmpty() && c.checkAnswer != nullptr)
            problem = c.checkAnswer (answer);

        failed += problem.isEmpty() ? 0 : 1;

        std::cout << (problem.isEmpty() ? "PASS  " : "FAIL  ") << c.request << "  (" << juce::String (took, 1) << " s, "
                  << calls << " tool calls, " << refused << " refused)" << std::endl;

        if (problem.isNotEmpty())
            std::cout << "      " << problem << std::endl;

        for (auto& s : agent.getWorkspace().getSteps())
            std::cout << "      plan: " << s << std::endl;

        std::cout << "      said: " << answer.trim().replace ("\n", " ").substring (0, 300) << std::endl;
    }

    std::cout << model << " (" << (set.isEmpty() ? juce::String ("tuned") : set) << " set): " << (n - failed) << " of " << n << " right, " << juce::roundToInt (totalSeconds) << " s in all" << std::endl;
    return failed;
}
