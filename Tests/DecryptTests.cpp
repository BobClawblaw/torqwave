#include <JuceHeader.h>
#include "../Source/Decrypt.h"
#include "../Source/AiDecrypt.h"
#include "TestUtils.h"
#include "../Source/DecryptView.h"
#include "../Source/SoundDrops.h"
#include "../Source/WaveSynthEditor.h"
#include "../Source/Theme.h"
#include "DecryptTests.h"

namespace
{
    using TestUtils::check;
    using namespace Decrypt;

    /** A factory patch played dry, as a recording of it: `before` seconds of silence, then the key
        held for `hold` seconds, `length` in all, at `sampleRate` in stereo. */
    juce::AudioBuffer<float> recordingOf (const juce::String& name, int note, double hold, double length, double sampleRate = rate, double before = 0.0,
                                          std::function<void (Patch&)> change = nullptr)
    {
        Synth synth;
        synth.setPlayConfigDetails (0, 2, sampleRate, 512);
        synth.setNonRealtime (true);
        synth.loadPatch (name);

        if (change != nullptr)
        {
            auto patch = SignalConsole::capture (synth);
            change (patch);
            SignalConsole::apply (synth, patch);
        }

        for (auto* effect : { "fxDriveOn", "fxChorusOn", "fxPhaserOn", "fxDelayOn", "fxReverbOn" })
            if (auto* p = synth.parameter (effect))
                p->setValueNotifyingHost (0.0f);

        synth.prepareToPlay (sampleRate, 512);
        const auto total = (int) (length * sampleRate), start = (int) (before * sampleRate), off = start + (int) (hold * sampleRate);
        juce::AudioBuffer<float> out (2, total), part (2, 512);

        for (int at = 0; at < total; at += 512)
        {
            const auto n = juce::jmin (512, total - at);
            juce::MidiBuffer midi;
            if (start >= at && start < at + n) midi.addEvent (juce::MidiMessage::noteOn (1, note, 0.8f), start - at);
            if (off >= at && off < at + n) midi.addEvent (juce::MidiMessage::noteOff (1, note), off - at);
            part.setSize (2, n, false, false, true);
            part.clear();
            synth.processBlock (part, midi);
            for (int ch = 0; ch < 2; ++ch) out.copyFrom (ch, at, part, ch, 0, n);
        }

        return out;
    }

    void testPrepare()
    {
        // A bell at A3 recorded at 44.1 kHz after a third of a second of silence: found at 220 Hz, from its start, at -6 dB.
        const auto clip = recordingOf ("Cold Bell", 57, 0.6, 1.5, 44100.0, 0.3);
        const auto t = prepare (clip, 44100.0);
        const auto cents = t.pitchHz > 0.0 ? 1200.0 * std::log2 (t.pitchHz / 220.0) : 9999.0;
        const auto peak = t.audio.getNumSamples() > 0 ? t.audio.getMagnitude (0, 0, t.audio.getNumSamples()) : 0.0f;
        const auto expectedLength = (int) ((1.5 - 0.3) * rate);
        check (t.problem.isEmpty() && std::abs (cents) < 20.0 && t.note == 57 && std::abs (peak - 0.5f) < 0.001f
                   && t.audio.getNumSamples() <= expectedLength + 600 && t.audio.getNumSamples() > expectedLength / 2,
               "decrypt: a note recorded at 44.1 kHz after silence is found at its pitch (" + juce::String (cents, 1) + " cents from 220 Hz, key "
                   + juce::String (t.note) + "), trimmed to the note (" + juce::String (t.audio.getNumSamples() / rate, 2) + " s) and levelled to -6 dB");

        juce::AudioBuffer<float> silent (2, 48000);
        silent.clear();
        juce::AudioBuffer<float> blip (1, 48000);
        blip.clear();
        for (int i = 0; i < 1000; ++i) blip.setSample (0, 10000 + i, 0.5f * std::sin ((float) i * 0.1f));
        check (prepare (silent, rate).problem.isNotEmpty() && prepare (blip, rate).problem.isNotEmpty(),
               "decrypt: a silent clip and a note shorter than a tenth of a second are refused, with a reason");

        // A sound is no distance from itself, and a different sound is some way off.
        const auto bell = prepare (recordingOf ("Cold Bell", 57, 0.6, 1.5), rate);
        const auto reese = prepare (recordingOf ("Abyss Reese", 57, 0.6, 1.5), rate);
        const auto self = distance (bell.measures, bell.measures).total;
        const auto other = distance (bell.measures, reese.measures).total;
        check (juce::exactlyEqual (self, 0.0f) && juce::exactlyEqual (lockOf (self), 100.0f) && other > 6.0f && lockOf (other) < 75.0f,
               "decrypt: a sound measures as no distance from itself (a lock of 100 %), and a bell is " + juce::String (other, 1) + " dB from a reese (a lock of "
                   + juce::String (lockOf (other), 0) + " %)");
    }

    /** Ten factory patches of different kinds, by name so new patches never move them, at a note that
        suits each: the simple set (a bass, reese, acid line, sub, lead, pad, pluck, bell, stab and brass,
        mostly one or two layers and few modulation routes), and the complex set, the first of the same
        categories since every category opened with patches that move (three layers with level
        envelopes, step gates, 6 to 16 modulation routes). */
    struct Case { const char* name; const char* category; int note; };
    const Case simpleCases[] = { { "Warehouse Sub", "Bass", 45 }, { "Abyss Reese", "Reese", 45 }, { "Acid Corrosion", "Acid", 45 }, { "Bare Shiver", "Sub", 33 },
                                 { "Siren Burn", "Lead", 57 }, { "Concrete Pad", "Pad", 57 }, { "Black Pluck", "Pluck", 57 }, { "Cold Bell", "Bell", 69 },
                                 { "Dub Chord Stab", "Stab", 57 }, { "Abrasive Bray", "Brass", 57 } };
    const Case complexCases[] = { { "Hydraulic Maw", "Bass", 45 }, { "Undertow Gnash", "Reese", 45 }, { "Caustic Gnaw", "Acid", 45 }, { "Bedrock Growl", "Sub", 33 },
                                  { "Voltage Scream", "Lead", 57 }, { "Furnace Halo", "Pad", 57 }, { "Rivet Snarl", "Pluck", 57 }, { "Foundry Knell", "Bell", 69 },
                                  { "Furnace Clamp", "Stab", 57 }, { "Foundry Horns", "Brass", 57 } };

    bool isFactoryPatch (const char* name)
    {
        const auto& all = WaveSynth::factoryPatches();
        return std::any_of (all.begin(), all.end(), [name] (const WaveSynth::Patch& p) { return p.name == name; });
    }

    /** Where a simple patch's match counts as found: set from what the search measured on the simple
        set in 16 rounds (the furthest 2.06 dB, Siren Burn), with a little room. Changes at or under the
        edge of hearing (3 cents of pitch, the filters 3 % higher, the envelopes' times 5 % longer, a
        recording made at 44.1 kHz) measure well under it (0.87 dB at most). */
    constexpr float foundWithin = 2.5f;

    void testThreshold()
    {
        juce::StringArray lines;
        auto worst = 0.0f;
        int measured = 0;

        for (auto* set : { &simpleCases, &complexCases })
        for (auto& c : *set)
        {
            if (! isFactoryPatch (c.name))
                continue;

            const juce::String name (c.name);
            const auto target = prepare (recordingOf (name, c.note, 0.8, 1.5), rate);
            const auto scale = [] (Patch& p, const juce::String& id, double by) { if (auto v = p.values.find (id); v != p.values.end()) v->second *= by; };
            const std::pair<const char*, std::function<void (Patch&)>> changes[] = {
                { "3 ct", [] (Patch& p) { for (auto* id : { "osc1Fine", "osc2Fine", "osc3Fine" }) p.values[id] += 3.0; } },
                { "cutoff +3 %", [scale] (Patch& p) { scale (p, "filter1Cutoff", 1.03); scale (p, "filter2Cutoff", 1.03); } },
                { "env +5 %", [scale] (Patch& p) { for (int e = 1; e <= WaveSynth::numEnvelopes; ++e) for (auto* stage : { "Attack", "Decay", "Release" }) scale (p, "env" + juce::String (e) + stage, 1.05); } },
            };
            juce::StringArray here;

            for (auto& [what, change] : changes)
            {
                const auto d = distance (target.measures, prepare (recordingOf (name, c.note, 0.8, 1.5, rate, 0.0, change), rate).measures).total;
                worst = juce::jmax (worst, d);
                here.add (juce::String (what) + " " + juce::String (d, 2));
            }

            const auto resampled = distance (target.measures, prepare (recordingOf (name, c.note, 0.8, 1.5, 44100.0), 44100.0).measures).total;
            worst = juce::jmax (worst, resampled);
            here.add ("44.1 kHz " + juce::String (resampled, 2));
            lines.add (name + ": " + here.joinIntoString (", "));
            ++measured;
        }

        std::printf ("DECRYPT-EDGE %s\n", lines.joinIntoString ("; ").toRawUTF8());
        check (measured == 20 && worst < foundWithin,
               "decrypt: changes at the edge of hearing (3 cents, the filters 3 % up, the envelopes 5 % longer, a 44.1 kHz recording) measure under the "
                   + juce::String (foundWithin, 1) + " dB a simple patch's match must come within, on the twenty patches of both sets (the furthest " + juce::String (worst, 2) + " dB)");
    }

    /** One set decrypted: each patch recorded dry and decrypted without its own name to start from.
        Each line has the distance found, its lock, the key it was heard at for the key that played
        it (a patch with a sub an octave down is heard an octave low: the patch found plays the
        recording's pitch from that key), where it grew from, the rounds and the time. */
    struct SetResult
    {
        juce::StringArray lines;
        float worst = 0.0f, meanLock = 0.0f;
        double longest = 0.0, meanSeconds = 0.0;
        int found = 0, within = 0, mostRounds = 0;
    };

    SetResult decryptSet (const Case (&set)[10], bool untilSettled, float within)
    {
        SetResult r;

        for (auto& c : set)
        {
            if (! isFactoryPatch (c.name))
                continue;

            const juce::String name (c.name);
            const auto target = prepare (recordingOf (name, c.note, 0.8, 1.5), rate);
            Options options;
            options.leaveOut.add (name);
            options.untilSettled = untilSettled;
            Search search (target, options);
            const auto result = search.runNow();
            const auto best = result.best.empty() ? 1000.0f : result.best.front().distance.total;
            const auto lock = result.best.empty() ? 0.0f : result.best.front().lock;
            r.worst = juce::jmax (r.worst, best);
            r.longest = juce::jmax (r.longest, result.seconds);
            r.meanSeconds += result.seconds / 10.0;
            r.meanLock += lock / 10.0f;
            r.mostRounds = juce::jmax (r.mostRounds, result.round);
            ++r.found;
            r.within += best < within ? 1 : 0;
            r.lines.add (name + " " + juce::String (best, 1) + " dB (" + juce::String (lock, 0) + " %, heard at " + noteName (target.note) + " for " + noteName (c.note) + ", from "
                         + (result.best.empty() ? juce::String ("nothing") : result.best.front().from) + ", " + juce::String (result.round) + " rounds, "
                         + juce::String (result.seconds, 1) + " s)");
        }

        return r;
    }

    /** Where a complex patch's match counts as found: set from what the search measured on the
        complex set until settled (the furthest 2.76 dB, Rivet Snarl; the mean lock 84 %), with a
        little room. It is wider than the simple set's: those patches move in ways a knob search
        reaches only in part. */
    constexpr float complexWithin = 3.5f;

    void testSearch()
    {
        const auto simple = decryptSet (simpleCases, false, foundWithin);
        std::printf ("DECRYPT simple %s\n", simple.lines.joinIntoString ("; ").toRawUTF8());
        check (simple.found == 10 && simple.within == 10,
               "decrypt: ten simple factory patches, recorded dry and decrypted in 16 rounds without their own names to start from, each come back within " + juce::String (foundWithin, 1) + " dB ("
                   + juce::String (simple.within) + " of " + juce::String (simple.found) + ", the furthest " + juce::String (simple.worst, 1) + " dB, the longest search "
                   + juce::String (simple.longest, 1) + " s: " + simple.lines.joinIntoString ("; ").substring (0, 900) + ")");

        const auto complex = decryptSet (complexCases, true, complexWithin);
        std::printf ("DECRYPT complex %s\n", complex.lines.joinIntoString ("; ").toRawUTF8());
        check (complex.found == 10 && complex.within == 10,
               "decrypt: ten complex factory patches (three layers, step gates, many modulation routes), decrypted until settled as the window does, each come back within "
                   + juce::String (complexWithin, 1) + " dB (" + juce::String (complex.within) + " of " + juce::String (complex.found) + ", the furthest " + juce::String (complex.worst, 1)
                   + " dB, mean lock " + juce::String (complex.meanLock, 1) + " %, at most " + juce::String (complex.mostRounds) + " rounds, the longest search "
                   + juce::String (complex.longest, 1) + " s: " + complex.lines.joinIntoString ("; ").substring (0, 1200) + ")");
    }

    void testRepeatable()
    {
        const auto target = prepare (recordingOf ("Abyss Reese", 45, 0.8, 1.5), rate);
        Options a, b;
        a.leaveOut.add ("Abyss Reese");
        a.rounds = 3;
        a.threads = 2;
        b = a;
        b.threads = 5;
        Search one (target, a), two (target, b);
        const auto first = one.runNow(), second = two.runNow();
        auto same = first.best.size() == second.best.size() && ! first.best.empty();

        for (size_t i = 0; same && i < first.best.size(); ++i)
            same = first.best[i].address == second.best[i].address && juce::exactlyEqual (first.best[i].distance.total, second.best[i].distance.total);

        Options c = a;
        c.seed = 2;
        Search three (target, c);
        const auto other = three.runNow();
        check (same && ! other.best.empty() && other.best.front().address != first.best.front().address,
               "decrypt: the same clip and seed give the same twelve on 2 threads and on 5, and another seed others");
    }

    void testStopAndLoad()
    {
        // A synth playing alongside: its patch untouched, and no block of it late while a search runs.
        Synth playing;
        playing.setPlayConfigDetails (0, 2, rate, 512);
        playing.loadPatch ("Cathedral Stack Pad");
        playing.prepareToPlay (rate, 512);
        const auto before = SignalConsole::addressOf (SignalConsole::capture (playing));
        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer start;
        start.addEvent (juce::MidiMessage::noteOn (1, 48, 0.8f), 0);
        start.addEvent (juce::MidiMessage::noteOn (1, 55, 0.8f), 0);
        start.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
        playing.processBlock (buffer, start);

        const auto blockTimes = [&playing, &buffer] (double seconds)
        {
            std::vector<double> times;
            const auto until = juce::Time::getMillisecondCounterHiRes() + seconds * 1000.0;

            while (juce::Time::getMillisecondCounterHiRes() < until)
            {
                juce::MidiBuffer none;
                const auto t0 = juce::Time::getMillisecondCounterHiRes();
                playing.processBlock (buffer, none);
                times.push_back (juce::Time::getMillisecondCounterHiRes() - t0);
                juce::Thread::sleep (8);   // a block's worth of time, as a device would call it (10.7 ms a block)
            }

            std::sort (times.begin(), times.end());
            return std::make_pair (times[times.size() * 99 / 100], times.back());
        };

        const auto quiet = blockTimes (2.0);
        // A search far longer than the measuring, so it is still running when told to stop.
        const auto target = prepare (recordingOf ("Abyss Reese", 45, 3.0, 4.0), rate);
        Options options;
        options.rounds = 1000;
        Search search (target, options);
        search.start();
        const auto loaded = blockTimes (3.0);
        const auto stopAsked = juce::Time::getMillisecondCounterHiRes();
        search.stop();

        while (search.isRunning() && juce::Time::getMillisecondCounterHiRes() - stopAsked < 5000.0)
            juce::Thread::sleep (5);

        const auto stopTook = juce::Time::getMillisecondCounterHiRes() - stopAsked;
        const auto progress = search.getProgress();
        const auto budget = 512.0 / rate * 1000.0;
        const auto wasRunning = progress.stopped && progress.round < options.rounds;
        check (wasRunning && loaded.second < budget && ! search.isRunning() && stopTook < 1000.0 && SignalConsole::addressOf (SignalConsole::capture (playing)) == before
                   && progress.rendered > 0,
               "decrypt: a synth playing a chord while a search runs is never late (its slowest block " + juce::String (loaded.second, 2) + " ms against "
                   + juce::String (budget, 1) + " ms, 99 % under " + juce::String (loaded.first, 2) + " ms; without a search " + juce::String (quiet.second, 2)
                   + " and " + juce::String (quiet.first, 2) + " ms), the search stops " + juce::String (stopTook, 0) + " ms after it is told to (after "
                   + juce::String (progress.rendered) + " renders), and the playing synth's patch is as it was");
    }
}

namespace
{
    /** A made-up model, as each kind the search must cope with. */
    class MadeUp final : public Advisor
    {
    public:
        enum class Kind { good, bad, nonsense, late, silent, namer };

        MadeUp (Kind k, Patch truth = {}) : kind (k), answer (std::move (truth))   {}

        Start start (const Brief& b) override
        {
            ++asked;
            sawClip = sawClip || b.clip != nullptr;
            Start s;

            if (kind == Kind::nonsense)
                s.patches = { "No Such Patch", "{]" };
            else if (! wait())
                return {};

            return s;
        }

        std::vector<Change> structure (const Brief& b) override
        {
            ++asked;
            briefs.add (b.difference);

            if (kind == Kind::nonsense)
                return { { "make it wetter", { { "noSuchSetting", 3.0 }, { "osc1Volume", std::nan ("") } } } };

            if (! wait())
                return {};

            if (kind == Kind::good)
                return { { "the original", answer.values } };

            if (kind == Kind::bad)
                return { { "a closed filter on noise", { { "osc1On", 0.0 }, { "osc2On", 0.0 }, { "osc3On", 0.0 }, { "subLevel", 0.0 }, { "noiseLevel", 100.0 },
                                                         { "filter1Type", 1.0 }, { "filter1Cutoff", 40.0 } } } };

            return {};
        }

        Name name (const Brief&) override
        {
            Name n;
            n.name = "Rust Reese";
            n.category = "Reese";
            n.macros = { "Grit", "Width", "", "" };
            return n;
        }

        void cancel() override      { cancelled = true; }

        std::atomic<int> asked { 0 };
        std::atomic<bool> sawClip { false }, cancelled { false };
        juce::StringArray briefs;

    private:
        /** The late one answers after its time; the silent one not until it is cancelled. False when cancelled. */
        bool wait()
        {
            if (kind == Kind::late)
                juce::Thread::sleep (timeLimitMs + 400);

            if (kind == Kind::silent)
                while (! cancelled)
                    juce::Thread::sleep (5);

            return ! cancelled.exchange (false);
        }

        Kind kind;
        Patch answer;
    };

    bool sameResult (const Progress& a, const Progress& b)
    {
        auto same = a.best.size() == b.best.size() && ! a.best.empty();

        for (size_t i = 0; same && i < a.best.size(); ++i)
            same = a.best[i].address == b.best[i].address && juce::exactlyEqual (a.best[i].distance.total, b.best[i].distance.total);

        return same;
    }

    void testModel()
    {
        Synth truth;
        truth.loadPatch ("Abyss Reese");
        const auto original = SignalConsole::capture (truth);
        const auto target = prepare (recordingOf ("Abyss Reese", 45, 0.8, 1.5), rate);
        const auto run = [&target] (std::shared_ptr<Advisor> advisor)
        {
            Options o;
            o.leaveOut.add ("Abyss Reese");
            o.rounds = 5;
            o.threads = 4;
            o.advisor = std::move (advisor);
            Search search (target, o);
            return search.runNow();
        };

        const auto alone = run (nullptr);
        const auto nonsense = std::make_shared<MadeUp> (MadeUp::Kind::nonsense);
        const auto late = std::make_shared<MadeUp> (MadeUp::Kind::late);
        late->timeLimitMs = 150;
        const auto silent = std::make_shared<MadeUp> (MadeUp::Kind::silent);
        silent->timeLimitMs = 150;
        const auto withNonsense = run (nonsense), withLate = run (late), withSilent = run (silent);
        check (sameResult (alone, withNonsense) && sameResult (alone, withLate) && sameResult (alone, withSilent) && nonsense->asked > 1 && late->asked > 0 && silent->asked > 0
                   && withSilent.seconds < alone.seconds * 2.0 + 1.0,
               "decrypt: a made-up model that answers nonsense, late or not at all leaves the result as it is with Ask Torq off (the search alone "
                   + juce::String (alone.seconds, 1) + " s, with the silent one " + juce::String (withSilent.seconds, 1) + " s: it never waits); its nonsense is logged as ignored ("
                   + withNonsense.model.joinIntoString (" | ").substring (0, 160) + ")");

        // The best a proposal of the reese's own settings can do: its own render against the clip (its
        // unison drifts, so a render is never the recording sample for sample).
        Synth renderer;
        const auto own = render (renderer, original, target);
        const auto ownLock = lockOf (distance (target.measures, measureRender (own)).total);

        const auto good = std::make_shared<MadeUp> (MadeUp::Kind::good, original);
        const auto bad = std::make_shared<MadeUp> (MadeUp::Kind::bad);
        const auto withGood = run (good), withBad = run (bad);
        const auto kept = withGood.model.joinIntoString ("|").contains ("\"the original\": kept");
        const auto dropped = withBad.model.joinIntoString ("|").contains (": dropped");
        check (kept && ! withGood.best.empty() && withGood.best.front().distance.total < alone.best.front().distance.total && withGood.best.front().lock > ownLock - 0.5f
                   && dropped && sameResult (alone, withBad) && good->sawClip && ! good->briefs.isEmpty(),
               "decrypt: a proposal that scores better is kept (the reese's own settings, proposed by a made-up model: lock " + juce::String (alone.best.empty() ? 0.0f : alone.best.front().lock, 1)
                   + " to " + juce::String (withGood.best.empty() ? 0.0f : withGood.best.front().lock, 1) + " %, its own render " + juce::String (ownLock, 1) + " %) and one that scores worse is dropped, leaving the result as without it; the model is told where the two differ ("
                   + (good->briefs.isEmpty() ? juce::String() : good->briefs[0].substring (0, 120)) + ")");
    }

    void testWindow()
    {
        Theme::apply (Theme::make ("Dark", "Blue"));
        Synth synth;
        synth.setPlayConfigDetails (0, 2, rate, 512);
        synth.prepareToPlay (rate, 512);
        synth.loadPatch ("Cathedral Stack Pad");
        const auto before = SignalConsole::addressOf (SignalConsole::capture (synth));
        std::unique_ptr<juce::AudioProcessorEditor> made (synth.createEditorAndMakeActive());
        auto* window = dynamic_cast<WaveSynthEditor*> (made.get());

        if (window == nullptr || window->getDecryptView() == nullptr)
        {
            check (false, "decrypt: the synth's window has Decrypt");
            return;
        }

        auto& view = *window->getDecryptView();
        view.setMotion (false);
        window->showView (2);
        auto drawn = true;

        const auto snapshots = [&]
        {
            for (auto scale : { 0.7f, 1.0f, 1.6f })
            {
                window->setSize (juce::roundToInt (WaveSynthEditor::designWidth * scale), juce::roundToInt (WaveSynthEditor::designHeight * scale));
                drawn = drawn && window->createComponentSnapshot (window->getLocalBounds(), true, 1.0f).getWidth() == window->getWidth();
            }

            window->setSize (WaveSynthEditor::designWidth, WaveSynthEditor::designHeight);
        };

        const auto click = [&view] (juce::Point<float> at, int clicks = 1)
        {
            const auto event = juce::MouseEvent (juce::Desktop::getInstance().getMainMouseSource(), at, {}, 0.5f, 0, 0, 0, 0, &view, &view, juce::Time::getCurrentTime(), at, juce::Time::getCurrentTime(), clicks, false);
            if (clicks == 2) view.mouseDoubleClick (event); else view.mouseDown (event);
        };

        snapshots();
        juce::AudioBuffer<float> silence (2, 48000);
        silence.clear();
        const auto refused = ! view.decrypt (silence, rate, "silence") && view.getSearch() == nullptr;
        click (view.tablesBounds().getCentre());
        const auto tables = view.factoryTablesOnly();
        click (view.tablesBounds().getCentre());

        // Started, a candidate put on the keys, then Stop: the patch the synth had comes back.
        const auto clip = recordingOf ("Abyss Reese", 45, 0.8, 1.5);
        const auto started = view.decrypt (clip, rate, "reese");
        TestUtils::runUntil ([&view] { return view.getSearch() != nullptr && view.getSearch()->getProgress().best.size() >= 12; }, 20000);
        snapshots();
        click (view.candidateCentre (3), 2);
        const auto onKeys = SignalConsole::addressOf (SignalConsole::capture (synth)) != before;
        click (view.stopBounds().getCentre());
        TestUtils::runUntil ([&view] { return ! view.getSearch()->isRunning(); }, 5000);
        const auto stoppedBack = SignalConsole::addressOf (SignalConsole::capture (synth)) == before;
        check (refused && tables && ! view.factoryTablesOnly() && started && onKeys && stoppedBack && drawn,
               "decrypt: the window refuses a silent clip, switches Tables only from the factory, starts on a clip, puts a candidate on the keys with a double-click, "
               "and Stop gives back the patch it had; it draws at 70 %, 100 % and 160 % before and during a search");

        // Take keeps the best; a model's name for it can be refused, and used.
        const auto namer = std::make_shared<MadeUp> (MadeUp::Kind::namer);
        setAdvisorMaker ([namer] { return namer; });
        view.decrypt (clip, rate, "reese");
        TestUtils::runUntil ([&view] { return ! view.getSearch()->isRunning(); }, 60000);
        const auto best = view.getSearch()->getProgress().best;
        click (view.takeBounds().getCentre());
        const auto taken = ! best.empty() && SignalConsole::addressOf (SignalConsole::capture (synth)) == best.front().address
                           && synth.getPatchName() == "Node " + SignalConsole::addressText (best.front().address);
        TestUtils::runUntil ([&view] { return view.offeredName().name.isNotEmpty(); }, 5000);
        const auto offered = view.offeredName().name == "Rust Reese";
        view.useOfferedName (false);
        const auto refusedName = synth.getPatchName().startsWith ("Node ");
        view.decrypt (clip, rate, "reese");
        TestUtils::runUntil ([&view] { return ! view.getSearch()->isRunning(); }, 60000);
        click (view.takeBounds().getCentre());
        TestUtils::runUntil ([&view] { return view.offeredName().name.isNotEmpty(); }, 5000);
        view.useOfferedName (true);
        const auto used = synth.getPatchName() == "Rust Reese" && synth.getMacroName (0) == "Grit";
        setAdvisorMaker (nullptr);
        check (taken && offered && refusedName && used,
               "decrypt: Take makes the best the synth's patch, named for its address; a name a model offers can be refused (the address stays) or used (the name and macros)");

        // Again: the search carried on from its best twelve, never ending further away than it began.
        {
            view.decrypt (clip, rate, "reese");
            const auto notYet = ! view.canGoAgain() && ! view.again();
            TestUtils::runUntil ([&view] { return ! view.getSearch()->isRunning(); }, 60000);
            const auto first = view.getSearch()->getProgress().best.front().lock;
            const auto ready = view.canGoAgain() && view.getPasses() == 1;
            click (view.againBounds().getCentre());
            const auto going = view.getPasses() == 2 && view.getSearch() != nullptr;
            TestUtils::runUntil ([&view] { return ! view.getSearch()->isRunning(); }, 60000);
            const auto second = view.getSearch()->getProgress().best.front().lock;
            view.again();
            TestUtils::runUntil ([&view] { return ! view.getSearch()->isRunning(); }, 60000);
            const auto third = view.getSearch()->getProgress().best.front().lock;

            // From a candidate taken: the next search goes on from that one alone.
            const auto picked = view.getSearch()->getProgress().best[3];
            view.setHovered (3);
            view.take();
            const auto tookIt = SignalConsole::addressOf (SignalConsole::capture (synth)) == picked.address && view.canGoAgain();
            view.again();
            TestUtils::runUntil ([&view] { return ! view.getSearch()->isRunning(); }, 60000);
            const auto fromPicked = view.getSearch()->getProgress().best.front().lock >= picked.lock - 0.01f && view.getPasses() == 4;
            auto allFromIt = true;

            for (auto& c : view.getSearch()->getProgress().best)
                allFromIt = allFromIt && c.from == "the last search";


            // The chain: a step each time the search came nearer, oldest first, never falling; Play goes through it, and a double-click goes back.
            // The locks rise along it, but for a candidate picked to go on from, which may be a little further away.
            int falls = 0;

            for (int i = 1; i < view.getChainLength(); ++i)
                falls += view.chainLock (i) < view.chainLock (i - 1) ? 1 : 0;

            const auto rising = view.getChainLength() >= 2 && falls <= 1;

            click (view.playChainBounds().getCentre());
            const auto playing = view.isPlayingChain();
            click (view.playChainBounds().getCentre());
            const auto stoppedPlaying = ! view.isPlayingChain();
            const auto length = view.getChainLength();
            const auto passesBefore = view.getPasses();
            click (view.chainBounds (length - 1).getCentre());
            const auto heardOnly = view.getPasses() == passesBefore && view.getChainLength() == length;
            view.goBackTo (0);
            const auto wentBack = view.getChainLength() == 1 && view.canGoAgain()
                                  && SignalConsole::addressOf (SignalConsole::capture (synth)) != before;
            view.again();
            TestUtils::runUntil ([&view] { return ! view.getSearch()->isRunning(); }, 60000);
            const auto onFromIt = view.getPasses() == passesBefore + 1 && view.getSearch()->getProgress().best.front().lock >= view.chainLock (0) - 0.01f;
            check (rising && playing && stoppedPlaying && heardOnly && wentBack && onFromIt,
                   "decrypt: the chain gains a step each time the search comes nearer (" + juce::String (length) + " here); Play goes through it and stops, a click only plays a step, "
                   "and going back to the first drops the later ones, puts it on the keys and lets Again go on from it");
            view.stop();
            check (tookIt && fromPicked && allFromIt,
                   "decrypt: after Take, Again searches on from the candidate taken alone, and ends no further from the signal than it was");
            check (notYet && ready && going && second >= first && third >= second,
                   "decrypt: Again carries a finished search on from its best twelve, as often as asked, and never ends further from the signal (lock "
                       + juce::String (first, 1) + ", then " + juce::String (second, 1) + ", then " + juce::String (third, 1) + " %)");
        }

        // Again with a candidate on the keys, not taken: Stop still gives back the patch from before the first pass.
        {
            const auto patchBefore = SignalConsole::addressOf (SignalConsole::capture (synth));
            view.decrypt (clip, rate, "reese");
            TestUtils::runUntil ([&view] { return ! view.getSearch()->isRunning(); }, 60000);
            click (view.candidateCentre (2), 2);
            const auto wasOnKeys = SignalConsole::addressOf (SignalConsole::capture (synth)) != patchBefore;
            view.again();
            view.stop();
            check (wasOnKeys && SignalConsole::addressOf (SignalConsole::capture (synth)) == patchBefore,
                   "decrypt: Again with a candidate on the keys but not taken, then Stop, gives back the patch the synth had before the search");
        }

        // Crowded text: the heading and its status never meet, the chain's hint fits its panel, and the lock labels on
        // the scope never print over each other, the one under the pointer always labelled.
        {
            view.decrypt (clip, rate, "reese");
            TestUtils::runUntil ([&view] { return ! view.getSearch()->isRunning(); }, 60000);
            auto apart = true, fits = true, unprinted = true, keyLabelled = true, under = false;
            float hint = 0.0f, room = 0.0f;
            int labelled = 0, candidates = 0;

            for (auto scale : { 0.7f, 1.0f, 1.6f })
            {
                window->setSize (juce::roundToInt (WaveSynthEditor::designWidth * scale), juce::roundToInt (WaveSynthEditor::designHeight * scale));
                apart = apart && ! view.headingBounds().isEmpty() && ! view.headingBounds().intersects (view.statusBounds())
                        && view.statusBounds().getWidth() <= view.panelTextWidth() + 0.5f;
                under = under || view.statusBounds().getY() > view.headingBounds().getY() + 4.0f;
                hint = view.chainHintWidth();
                room = view.panelTextWidth();
                fits = fits && view.getChainLength() > 0 && hint <= room;

                for (int h : { -2, 1, 7 })
                {
                    view.setHovered (h);
                    const auto labels = view.lockLabelBounds();
                    candidates = (int) labels.size();
                    labelled = 0;

                    for (size_t i = 0; i < labels.size(); ++i)
                    {
                        labelled += labels[i].isEmpty() ? 0 : 1;

                        for (size_t j = i + 1; j < labels.size(); ++j)
                            unprinted = unprinted && ! labels[i].intersects (labels[j]);
                    }

                    keyLabelled = keyLabelled && (juce::isPositiveAndBelow (h, (int) labels.size()) ? ! labels[(size_t) h].isEmpty() : labelled > 0);
                }
            }

            view.setHovered (-2);
            window->setSize (WaveSynthEditor::designWidth, WaveSynthEditor::designHeight);
            check (apart && fits && unprinted && keyLabelled,
                   "decrypt: at 70 %, 100 % and 160 % the panel's heading and status never meet (the status " + juce::String (under ? "under" : "beside")
                       + " the heading here), the chain's hint fits (" + juce::String (hint, 0) + " of " + juce::String (room, 0) + " points), and no lock label on the scope "
                       "prints over another, the one under the pointer always labelled (" + juce::String (labelled) + " of " + juce::String (candidates) + " labelled)");
        }

        // Until settled: the window's searches go on past 16 rounds while they come closer, and say when they no longer do.
        {
            Options settle;
            settle.untilSettled = true;
            settle.threads = 4;
            Search search (prepare (clip, rate), settle);
            const auto began = juce::Time::getMillisecondCounterHiRes();
            const auto end = search.runNow();
            const auto took = (juce::Time::getMillisecondCounterHiRes() - began) / 1000.0;
            Options fixed;
            fixed.threads = 4;
            Search plain (prepare (clip, rate), fixed);
            const auto sixteen = plain.runNow();
            check (end.finished && ! end.stopped && end.round >= 16 && end.round <= 64 && (end.noCloser || end.round == 64)
                       && sixteen.round == 16 && ! sixteen.noCloser && end.best.front().lock >= sixteen.best.front().lock,
                   "decrypt: a search until settled runs at least its 16 rounds, ends once six in a row come no closer (here after " + juce::String (end.round)
                       + " rounds and " + juce::String (took, 1) + " s, lock " + juce::String (end.best.front().lock, 1) + " % against "
                       + juce::String (sixteen.best.front().lock, 1) + " % at 16 rounds), and says so; a search of fixed rounds runs exactly 16");
        }

        // A sound dragged from inside the app (an open file in the Files panel): the app says it is one and hands over its audio.
        {
            view.stop();
            const juce::DragAndDropTarget::SourceDetails openFile ("openfile:0", nullptr, {}), other ("dockPanel:Files", nullptr, {});
            const auto unhooked = ! view.isInterestedInDragSource (openFile);
            SoundDrops::isSound = [] (const juce::var& d) { return d.toString() == "openfile:0"; };
            SoundDrops::soundOf = [&clip] (const juce::var&, double) -> std::optional<SoundDrops::Sound> { return SoundDrops::Sound { clip, rate, "Dropped" }; };
            const auto wanted = view.isInterestedInDragSource (openFile) && ! view.isInterestedInDragSource (other);
            view.itemDropped (openFile);
            const auto searching = view.getSearch() != nullptr;
            view.stop();
            SoundDrops::isSound = nullptr;
            SoundDrops::soundOf = nullptr;
            check (unhooked && wanted && searching,
                   "decrypt: an open file dragged from the Files panel is taken and searched for, as a file from the Finder is; any other drag is not");
        }

        window->showView (0);
    }
}

void runDecryptTests()
{
    testThreshold();
    testModel();
    testWindow();
    testPrepare();
    testRepeatable();
    testStopAndLoad();
    testSearch();
}

int runDecryptEval (const juce::String& url, const juce::String& model, const juce::String& key)
{
    using namespace Decrypt;
    Ai::Settings settings;
    settings.enabled = true;
    settings.serverUrl = url;
    settings.model = model;
    double aloneLock = 0.0, helpedLock = 0.0, aloneSeconds = 0.0, helpedSeconds = 0.0;
    int n = 0, better = 0, worse = 0, kept = 0, proposed = 0, unfinished = 0;

    for (auto& c : simpleCases)
    {
        if (! isFactoryPatch (c.name))
            continue;

        const juce::String name (c.name);
        const auto target = prepare (recordingOf (name, c.note, 0.8, 1.5), rate);
        const auto run = [&] (std::shared_ptr<Advisor> advisor)
        {
            Options options;
            options.leaveOut.add (name);
            options.words = "a " + juce::String (c.category).toLowerCase();
            options.advisor = std::move (advisor);
            Search search (target, options);
            return search.runNow();
        };

        const auto alone = run (nullptr);
        const auto helped = run (std::make_shared<Ai::DecryptAdvisor> (settings, key));

        if (alone.best.empty() || helped.best.empty() || ! helped.finished)
        {
            ++unfinished;
            std::printf ("%-18s no result\n", name.toRawUTF8());
            continue;
        }

        ++n;
        const auto a = alone.best.front().lock, h = helped.best.front().lock;
        aloneLock += a; helpedLock += h; aloneSeconds += alone.seconds; helpedSeconds += helped.seconds;
        better += h > a + 0.05f ? 1 : 0;
        worse += h < a - 0.05f ? 1 : 0;

        for (auto& line : helped.model)
        {
            proposed += line.startsWith ("tried") ? 1 : 0;
            kept += line.contains (": kept") ? 1 : 0;
        }

        std::printf ("%-18s alone %5.1f %% (%4.1f s)  with the model %5.1f %% (%4.1f s), from %s\n", name.toRawUTF8(), a, alone.seconds, h, helped.seconds,
                     helped.best.front().from.toRawUTF8());

        for (auto& line : helped.model)
            std::printf ("    %s\n", line.toRawUTF8());
    }

    if (n > 0)
        std::printf ("DECRYPT-EVAL %d clips: mean lock %.1f %% alone, %.1f %% with the model; better on %d, worse on %d; %d of %d proposals kept; "
                     "mean time %.1f s alone, %.1f s with the model\n",
                     n, aloneLock / n, helpedLock / n, better, worse, kept, proposed, aloneSeconds / n, helpedSeconds / n);

    return unfinished == 0 ? 0 : 1;
}

int runDecryptMeasure()
{
    // The two sets of the search test, both ways, with their times: for tuning the search.
    for (auto settled : { false, true })
        for (auto* set : { &simpleCases, &complexCases })
        {
            const auto r = decryptSet (*set, settled, set == &simpleCases ? foundWithin : complexWithin);
            std::printf ("%s, %s: furthest %.2f dB, mean lock %.1f %%, %d of 10 within, at most %d rounds, mean %.1f s, longest %.1f s\n",
                         set == &simpleCases ? "simple" : "complex", settled ? "until settled" : "16 rounds", r.worst, r.meanLock, r.within,
                         r.mostRounds, r.meanSeconds, r.longest);

            for (auto& line : r.lines)
                std::printf ("    %s\n", line.toRawUTF8());
        }

    return 0;
}

int runDecryptFile (const juce::String& path)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    const std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (juce::File (path)));

    if (reader == nullptr)
    {
        std::cout << "can't read " << path << std::endl;
        return 1;
    }

    const auto length = (int) juce::jmin (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 10.0));
    juce::AudioBuffer<float> audio ((int) juce::jmin (2u, reader->numChannels), length);
    reader->read (&audio, 0, length, 0, true, audio.getNumChannels() > 1);
    auto target = Decrypt::prepare (audio, reader->sampleRate);

    if (target.problem.isNotEmpty())
    {
        std::cout << "refused: " << target.problem << std::endl;
        return 1;
    }

    std::cout << Decrypt::describe (target) << std::endl;
    Decrypt::Options options;
    options.untilSettled = true;
    Decrypt::Search search (std::move (target), options);
    int shown = -1;
    const auto end = search.runNow ([&]
    {
        const auto p = search.getProgress();

        if (p.round != shown && ! p.best.empty())
        {
            shown = p.round;
            std::cout << "round " << p.round << ": lock " << juce::String (p.best.front().lock, 2) << " %, " << juce::String (p.best.front().distance.total, 2)
                      << " dB, from " << p.best.front().from << "  [" << p.stage << "]" << std::endl;
        }

        return false;
    });
    std::cout << "ended: " << end.stage << ", lock " << juce::String (end.best.front().lock, 2) << " %, " << juce::String (end.best.front().distance.total, 2)
              << " dB, " << juce::String (end.seconds, 1) << " s" << std::endl;
    return 0;
}
