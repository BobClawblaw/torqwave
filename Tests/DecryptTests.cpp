#include <JuceHeader.h>
#include "../Source/Decrypt.h"
#include "../Source/AiDecrypt.h"
#include "TestUtils.h"
#include "../Source/DecryptView.h"
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

    /** Ten factory patches of different kinds: the first of each category, at a note that suits it. */
    struct Case { const char* category; int note; };
    const Case cases[] = { { "Bass", 45 }, { "Reese", 45 }, { "Acid", 45 }, { "Sub", 33 }, { "Lead", 57 },
                           { "Pad", 57 }, { "Pluck", 57 }, { "Bell", 69 }, { "Stab", 57 }, { "Brass", 57 } };

    /** Where a match counts as found: changes at or under the edge of hearing (3 cents of pitch, the
        filters 3 % higher, the envelopes' times 5 % longer, a recording made at 44.1 kHz) measure under it. */
    constexpr float foundWithin = 3.0f;

    void testThreshold()
    {
        const auto& all = WaveSynth::factoryPatches();
        juce::StringArray lines;
        auto worst = 0.0f;
        int measured = 0;

        for (auto& c : cases)
        {
            const auto it = std::find_if (all.begin(), all.end(), [&c] (const WaveSynth::Patch& p) { return p.category == c.category; });

            if (it == all.end())
                continue;

            const auto target = prepare (recordingOf (it->name, c.note, 0.8, 1.5), rate);
            const auto scale = [] (Patch& p, const juce::String& id, double by) { if (auto v = p.values.find (id); v != p.values.end()) v->second *= by; };
            const std::pair<const char*, std::function<void (Patch&)>> changes[] = {
                { "3 ct", [] (Patch& p) { for (auto* id : { "osc1Fine", "osc2Fine", "osc3Fine" }) p.values[id] += 3.0; } },
                { "cutoff +3 %", [scale] (Patch& p) { scale (p, "filter1Cutoff", 1.03); scale (p, "filter2Cutoff", 1.03); } },
                { "env +5 %", [scale] (Patch& p) { for (int e = 1; e <= WaveSynth::numEnvelopes; ++e) for (auto* stage : { "Attack", "Decay", "Release" }) scale (p, "env" + juce::String (e) + stage, 1.05); } },
            };
            juce::StringArray here;

            for (auto& [what, change] : changes)
            {
                const auto d = distance (target.measures, prepare (recordingOf (it->name, c.note, 0.8, 1.5, rate, 0.0, change), rate).measures).total;
                worst = juce::jmax (worst, d);
                here.add (juce::String (what) + " " + juce::String (d, 2));
            }

            const auto resampled = distance (target.measures, prepare (recordingOf (it->name, c.note, 0.8, 1.5, 44100.0), 44100.0).measures).total;
            worst = juce::jmax (worst, resampled);
            here.add ("44.1 kHz " + juce::String (resampled, 2));
            lines.add (it->name + ": " + here.joinIntoString (", "));
            ++measured;
        }

        std::printf ("DECRYPT-EDGE %s\n", lines.joinIntoString ("; ").toRawUTF8());
        check (measured == 10 && worst < foundWithin,
               "decrypt: changes at the edge of hearing (3 cents, the filters 3 % up, the envelopes 5 % longer, a 44.1 kHz recording) measure under the "
                   + juce::String (foundWithin, 1) + " dB a match must come within, on ten patches (the furthest " + juce::String (worst, 2) + " dB)");
    }

    void testSearch()
    {
        // Ten factory patches of different kinds, each recorded dry and decrypted without its own name to start from.
        const auto& all = WaveSynth::factoryPatches();
        juce::StringArray lines;
        auto worst = 0.0f;
        auto longest = 0.0;
        int found = 0, near = 0;

        for (auto& c : cases)
        {
            const auto it = std::find_if (all.begin(), all.end(), [&c] (const WaveSynth::Patch& p) { return p.category == c.category; });

            if (it == all.end())
                continue;

            const auto name = it->name;
            const auto target = prepare (recordingOf (name, c.note, 0.8, 1.5), rate);
            Options options;
            options.leaveOut.add (name);
            Search search (target, options);
            const auto result = search.runNow();
            const auto best = result.best.empty() ? 1000.0f : result.best.front().distance.total;
            worst = juce::jmax (worst, best);
            longest = juce::jmax (longest, result.seconds);
            ++found;
            if (best < foundWithin) ++near;
            lines.add (name + " " + juce::String (best, 1) + " dB (" + juce::String (result.best.empty() ? 0.0f : result.best.front().lock, 0) + " %, from "
                       + (result.best.empty() ? juce::String ("nothing") : result.best.front().from) + ", " + juce::String (result.seconds, 1) + " s)");
        }

        std::printf ("DECRYPT %s\n", lines.joinIntoString ("; ").toRawUTF8());
        check (found == 10 && near == 10,
               "decrypt: ten factory patches of different kinds, recorded dry and decrypted without their own names to start from, each come back within " + juce::String (foundWithin, 1) + " dB ("
                   + juce::String (near) + " of " + juce::String (found) + ", the furthest " + juce::String (worst, 1) + " dB, the longest search "
                   + juce::String (longest, 1) + " s: " + lines.joinIntoString ("; ").substring (0, 900) + ")");
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
        const auto ownLock = lockOf (distance (target.measures, measure (own.getReadPointer (0), own.getNumSamples())).total);

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
        TestUtils::runUntil ([&view] { return ! view.getSearch()->isRunning(); }, 30000);
        const auto best = view.getSearch()->getProgress().best;
        click (view.takeBounds().getCentre());
        const auto taken = ! best.empty() && SignalConsole::addressOf (SignalConsole::capture (synth)) == best.front().address
                           && synth.getPatchName() == "Node " + SignalConsole::addressText (best.front().address);
        TestUtils::runUntil ([&view] { return view.offeredName().name.isNotEmpty(); }, 5000);
        const auto offered = view.offeredName().name == "Rust Reese";
        view.useOfferedName (false);
        const auto refusedName = synth.getPatchName().startsWith ("Node ");
        view.decrypt (clip, rate, "reese");
        TestUtils::runUntil ([&view] { return ! view.getSearch()->isRunning(); }, 30000);
        click (view.takeBounds().getCentre());
        TestUtils::runUntil ([&view] { return view.offeredName().name.isNotEmpty(); }, 5000);
        view.useOfferedName (true);
        const auto used = synth.getPatchName() == "Rust Reese" && synth.getMacroName (0) == "Grit";
        setAdvisorMaker (nullptr);
        check (taken && offered && refusedName && used,
               "decrypt: Take makes the best the synth's patch, named for its address; a name a model offers can be refused (the address stays) or used (the name and macros)");
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
    const auto& all = WaveSynth::factoryPatches();
    double aloneLock = 0.0, helpedLock = 0.0, aloneSeconds = 0.0, helpedSeconds = 0.0;
    int n = 0, better = 0, worse = 0, kept = 0, proposed = 0, unfinished = 0;

    for (auto& c : cases)
    {
        const auto it = std::find_if (all.begin(), all.end(), [&c] (const WaveSynth::Patch& p) { return p.category == c.category; });

        if (it == all.end())
            continue;

        const auto name = it->name;
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
