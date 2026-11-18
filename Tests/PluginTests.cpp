#include <JuceHeader.h>
#include "../Source/OfflineRender.h"
#include "../Source/PluginScanner.h"
#include "TestUtils.h"
#include "PluginTests.h"

namespace
{
    using TestUtils::check;
    using TestUtils::runUntil;

    constexpr double rate = 48000.0;

    /** Keeps what is logged, so a test can see that a failure was written down. */
    struct CapturingLogger final : public juce::Logger
    {
        void logMessage (const juce::String& message) override
        {
            const std::lock_guard<std::mutex> lock (mutex);
            lines.add (message);
        }

        juce::String find (const juce::String& text) const
        {
            const std::lock_guard<std::mutex> lock (mutex);

            for (auto& line : lines)
                if (line.contains (text))
                    return line;

            return {};
        }

        mutable std::mutex mutex;
        juce::StringArray lines;
    };

    juce::AudioPluginFormat* findFormat (AudioEngine& engine, const juce::String& name)
    {
        for (auto* f : engine.pluginFormatManager.getFormats())
            if (f->getName() == name)
                return f;

        return nullptr;
    }

    /** How many scan workers are running on this machine. */
    int countWorkers()
    {
        juce::ChildProcess pgrep;
       #if JUCE_WINDOWS
        pgrep.start (juce::StringArray { "powershell", "-NoProfile", "-Command",
                                         juce::String ("Get-CimInstance Win32_Process | Where-Object { $_.CommandLine -like '*")
                                             + scannerProcessUID + "*' -and $_.Name -ne 'powershell.exe' } | ForEach-Object ProcessId" });
       #else
        pgrep.start (juce::StringArray { "/usr/bin/pgrep", "-f", scannerProcessUID });
       #endif
        auto pids = juce::StringArray::fromLines (pgrep.readAllProcessOutput());
        pids.removeEmptyStrings();
        return pids.size();
    }

    /** Runs fn on a background thread with the message loop going, as the app's scans do. */
    void inBackground (const std::function<void()>& fn)
    {
        std::atomic<bool> done { false };
        std::thread thread ([&] { fn(); done = true; });
        runUntil ([&] { return done.load(); }, 600000);
        thread.join();
    }

    // Kilohearts' free gain plug-in, installed as both VST3 and AU: it opens no licence window,
    // and at its default setting it passes audio through unchanged.
    const juce::String gainVst3 = "/Library/Audio/Plug-Ins/VST3/Kilohearts/kHs Gain.vst3";
    const juce::String gainComponent = "/Library/Audio/Plug-Ins/Components/kHs Gain.component";

    void testCheck (AudioEngine& engine)
    {
        auto* au = findFormat (engine, "AudioUnit");

        if (au == nullptr)
            return;

        juce::OwnedArray<juce::PluginDescription> types;
        au->findAllTypesForFile (types, "AudioUnit:Effects/aufx,dely,appl");
        check (types.size() == 1 && ScannerWorker::check (*au, *types[0]).isEmpty(),
               "plug-in check: Apple's AUDelay loads, plays noise and restores its state");
    }

    juce::Array<juce::PluginDescription> testScan (AudioEngine& engine)
    {
        juce::Array<juce::PluginDescription> found;
        CapturingLogger log;
        juce::Logger::setCurrentLogger (&log);

        // A bundle that looks like a VST3 but holds no code.
        const auto dir = juce::File::createTempFile ("plugins");
        const auto broken = dir.getChildFile ("Broken.vst3");
        broken.getChildFile ("Contents/MacOS").createDirectory();
        broken.getChildFile ("Contents/MacOS/Broken").replaceWithText ("not a plug-in");
        broken.getChildFile ("Contents/Info.plist").replaceWithText ("<?xml version=\"1.0\"?><plist version=\"1.0\"><dict>"
                                                                      "<key>CFBundleExecutable</key><string>Broken</string></dict></plist>");

        inBackground ([&]
        {
            OutOfProcessScanner scanner (TestUtils::appExecutable);
            juce::OwnedArray<juce::PluginDescription> results;

            if (auto* vst3 = findFormat (engine, "VST3"))
            {
                const auto start = juce::Time::getMillisecondCounter();
                const auto ok = scanner.findPluginTypesFor (*vst3, results, broken.getFullPathName());
                check (! ok && results.isEmpty() && juce::Time::getMillisecondCounter() - start < 20000,
                       "plug-in scan: a broken VST3 is reported as not working, without waiting out the timeout");

                const juce::File bundles (TORQWAVE_TEST_BUNDLES);

               #if JUCE_ARM
                results.clear();
                const auto intelOnly = scanner.findPluginTypesFor (*vst3, results, bundles.getChildFile ("IntelOnlyBundle.vst3").getFullPathName());
                check (! intelOnly && log.find ("IntelOnlyBundle").contains ("no code for Apple silicon"),
                       "plug-in scan: a VST3 built only for Intel is reported as having no code for Apple silicon");
               #elif JUCE_WINDOWS
                results.clear();
                const auto only32 = scanner.findPluginTypesFor (*vst3, results, bundles.getChildFile ("IntelOnlyBundle.vst3").getFullPathName());
                check (! only32 && log.find ("IntelOnlyBundle").contains ("no 64-bit code"),
                       "plug-in scan: a VST3 built only for 32-bit Windows is reported as having no 64-bit code ("
                           + log.find ("IntelOnlyBundle") + ")");
               #endif

                results.clear();
                const auto crashed = scanner.findPluginTypesFor (*vst3, results, bundles.getChildFile ("CrashingBundle.vst3").getFullPathName());
                check (! crashed && log.find ("CrashingBundle").contains ("crashed while loading"),
                       "plug-in scan: a VST3 that crashes as it loads is marked not working, and the scan goes on (" + log.find ("CrashingBundle") + ")");

                // A plug-in that hangs holds the worker's message thread for good; once the scan
                // gives up on it, the worker must still go away.
                const auto workersBefore = countWorkers();
                {
                    OutOfProcessScanner quick (TestUtils::appExecutable, 3000);
                    results.clear();
                    const auto hung = quick.findPluginTypesFor (*vst3, results, bundles.getChildFile ("HangingBundle.vst3").getFullPathName());
                    check (! hung && log.find ("HangingBundle").contains ("took longer than 3 s"),
                           "plug-in scan: a VST3 that hangs as it loads is given up on after the timeout (" + log.find ("HangingBundle") + ")");
                }

                const auto workersGone = runUntil ([&] { return countWorkers() <= workersBefore; }, 5000);
                check (workersGone, "plug-in scan: the worker stuck in the hung plug-in exits (" + juce::String (countWorkers())
                                        + " workers running, " + juce::String (workersBefore) + " before)");

                if (juce::File (gainVst3).isDirectory())
                {
                    results.clear();
                    check (scanner.findPluginTypesFor (*vst3, results, gainVst3) && results.size() == 1,
                           "plug-in scan: a third-party VST3 (kHs Gain) loads and plays in the app's worker");

                    for (auto* d : results)
                        found.add (*d);
                }
                else
                {
                    std::cout << "      (kHs Gain VST3 is not installed; skipped)" << std::endl;
                }
            }

            if (auto* au = findFormat (engine, "AudioUnit"))
            {
                for (auto& id : au->searchPathsForPlugins ({}, true, false))
                {
                    if (au->getNameOfPluginFromIdentifier (id) != "kHs Gain")
                        continue;

                    results.clear();
                    const auto ok = scanner.findPluginTypesFor (*au, results, id);
                    check (ok && results.size() == 1, "plug-in scan: a third-party Audio Unit (kHs Gain) loads and plays in the app's worker"
                                                          + (ok ? juce::String() : " (" + log.find (id) + ")"));

                    for (auto* d : results)
                        found.add (*d);
                }
            }

            scanner.scanFinished();
        });

        juce::Logger::setCurrentLogger (nullptr);
        const auto line = log.find ("Broken.vst3");
        check (line.contains ("Plug-in not working (VST3)"), "plug-in scan: the failure is logged with its reason (" + line + ")");
        dir.deleteRecursively();
        return found;
    }

   #if JUCE_PLUGINHOST_LV2
    /** LV2: the test gain bundle (built with the tests) is found by the scan in the app's worker,
        then loads here, plays at the gain its control is set to and keeps it in its state. */
    void testLv2 (AudioEngine& engine, juce::Array<juce::PluginDescription>& found)
    {
        auto* lv2 = findFormat (engine, "LV2");
        check (lv2 != nullptr, "LV2: the plug-in format is there");

        if (lv2 == nullptr)
            return;

        const auto bundle = juce::File (TORQWAVE_TEST_BUNDLES).getChildFile ("TorqwaveTestGain.lv2").getFullPathName();
        juce::OwnedArray<juce::PluginDescription> results;
        bool ok = false;

        inBackground ([&]
        {
            OutOfProcessScanner scanner (TestUtils::appExecutable);
            ok = scanner.findPluginTypesFor (*lv2, results, bundle);
            scanner.scanFinished();
        });

        check (ok && results.size() == 1 && results[0]->name == "Torqwave Test Gain" && results[0]->pluginFormatName == "LV2"
                   && results[0]->fileOrIdentifier == "urn:torqwave:test:gain" && results[0]->numInputChannels == 2,
               "LV2: the scan finds the test gain in its bundle and checks it in the app's worker");

        if (results.size() != 1)
            return;

        // The app knows the plug-ins in the LV2 folders from the start; this bundle is elsewhere.
        juce::OwnedArray<juce::PluginDescription> here;
        lv2->findAllTypesForFile (here, bundle);

        juce::String error;
        auto plugin = lv2->createInstanceFromDescription (*results[0], rate, 512, error);
        check (plugin != nullptr, "LV2: the test gain loads in the app" + (error.isEmpty() ? juce::String() : ": " + error));

        if (plugin == nullptr)
            return;

        juce::AudioProcessorParameter* gain = nullptr;

        for (auto* p : plugin->getParameters())
            if (p->getName (32) == "Gain")
                gain = p;

        check (gain != nullptr, "LV2: the test gain's control is a parameter");

        if (gain == nullptr)
            return;

        // 0 to 2: a quarter of the way is half the level. A change reaches the plug-in at the
        // next block it plays, so one block goes first.
        plugin->prepareToPlay (rate, 512);
        gain->setValueNotifyingHost (0.25f);
        juce::AudioBuffer<float> audio (2, 512);
        juce::MidiBuffer midi;
        plugin->processBlock (audio, midi);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
                audio.setSample (ch, i, 0.8f * std::sin ((float) i * 0.05f + (float) ch));

        const juce::AudioBuffer<float> input (audio);
        plugin->processBlock (audio, midi);
        float worst = 0.0f;

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
                worst = juce::jmax (worst, std::abs (audio.getSample (ch, i) - 0.5f * input.getSample (ch, i)));

        check (worst < 1.0e-6f, "LV2: the test gain plays at the level its control is set to (worst " + juce::String (worst, 8) + ")");

        juce::MemoryBlock state;
        plugin->getStateInformation (state);
        plugin->releaseResources();
        auto second = lv2->createInstanceFromDescription (*results[0], rate, 512, error);
        juce::AudioProcessorParameter* secondGain = nullptr;

        if (second != nullptr)
        {
            second->setStateInformation (state.getData(), (int) state.getSize());

            for (auto* p : second->getParameters())
                if (p->getName (32) == "Gain")
                    secondGain = p;
        }

        check (secondGain != nullptr && std::abs (secondGain->getValue() - 0.25f) < 1.0e-4f,
               "LV2: the control's setting comes back from the plug-in's state");

        found.add (*results[0]);
    }
   #endif

    void testThirdPartyInRack (AudioEngine& engine, const juce::Array<juce::PluginDescription>& found)
    {
        for (auto& d : found)
        {
            if (d.manufacturerName == "Apple")
                continue;

            bool finished = false;
            juce::String error;
            engine.clearRack();
            engine.addPlugin (d, [&] (const juce::String& e) { error = e; finished = true; });
            runUntil ([&] { return finished; }, 30000);
            check (finished && error.isEmpty() && engine.getNumRackSlots() == 1,
                   "rack: " + d.pluginFormatName + " " + d.name + " loads in the app's rack" + (error.isEmpty() ? juce::String() : ": " + error));

            if (engine.getNumRackSlots() != 1)
                continue;

            // Noise through it offline, the way Effects > Apply Rack renders: at its default it
            // must hand the audio back unchanged (within float rounding), in time.
            juce::AudioBuffer<float> audio (2, 48000);
            juce::Random r (7);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < audio.getNumSamples(); ++i)
                    audio.setSample (ch, i, (r.nextFloat() - 0.5f) * 0.5f);

            const juce::AudioBuffer<float> input (audio);
            const auto xml = engine.createRackXml();
            juce::String renderError;
            inBackground ([&] { renderError = OfflineRender::processThroughRack (*xml, engine.pluginFormatManager, rate, audio, {}); });

            float worst = 0.0f;

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < audio.getNumSamples(); ++i)
                    worst = juce::jmax (worst, std::abs (audio.getSample (ch, i) - input.getSample (ch, i)));

            check (renderError.isEmpty() && worst < 1.0e-5f,
                   "rack: audio through " + d.pluginFormatName + " " + d.name + " comes back unchanged at unity gain (worst "
                       + juce::String (worst, 8) + ")");

            // Its state survives a save and reload of the rack.
            finished = false;
            engine.clearRack();
            engine.loadRackXml (*xml, [&] (const juce::String& e) { error = e; finished = true; });
            runUntil ([&] { return finished; }, 30000);
            check (finished && error.isEmpty() && engine.getNumRackSlots() == 1 && engine.getRackPlugin (0)->getName() == d.name,
                   "rack: " + d.pluginFormatName + " " + d.name + " restores from a saved rack");
        }

        engine.clearRack();
    }
}

void runPluginTests (AudioEngine& engine)
{
    testCheck (engine);
    auto found = testScan (engine);
   #if JUCE_PLUGINHOST_LV2
    testLv2 (engine, found);
   #endif
    testThirdPartyInRack (engine, found);
}

int runPluginSweep (AudioEngine& engine, const juce::File& report)
{
    juce::KnownPluginList list;
    CapturingLogger log;
    juce::Logger::setCurrentLogger (&log);

    const auto start = juce::Time::getMillisecondCounterHiRes();
    PluginScan scan (engine.pluginFormatManager, list, false, TestUtils::appExecutable);
    auto lastShown = -1;

    while (! scan.isFinished())
    {
        juce::MessageManager::getInstance()->runDispatchLoopUntil (200);
        const auto percent = (int) (scan.getProgress() * 100.0);

        if (percent / 5 != lastShown)
        {
            lastShown = percent / 5;
            std::cout << "      sweep " << percent << "%: " << scan.getNumWorking() << " files work, " << scan.getNumNotWorking()
                      << " don't (" << scan.getCurrentName() << ")" << std::endl;
        }
    }

    juce::Logger::setCurrentLogger (nullptr);
    const auto minutes = (juce::Time::getMillisecondCounterHiRes() - start) / 60000.0;

    juce::StringArray lines;
    std::map<juce::String, int> byFormat;

    for (auto& t : list.getTypes())
        ++byFormat[t.pluginFormatName + (t.isInstrument ? " instruments" : " effects")];

    lines.add ("Plug-in sweep: " + juce::String (list.getNumTypes()) + " working types, "
               + juce::String (list.getBlacklistedFiles().size()) + " files not working, " + juce::String (minutes, 1) + " min");

    for (auto& [kind, count] : byFormat)
        lines.add ("  " + kind + ": " + juce::String (count));

    lines.add ({});
    lines.add ("Not working:");

    for (auto& line : log.lines)
        if (line.startsWith ("Plug-in not working") || line.startsWith ("Plug-in type not working"))
            lines.add ("  " + line);

    lines.add ({});
    lines.add ("Working:");

    for (auto& t : list.getTypes())
        lines.add ("  " + t.pluginFormatName + "  " + t.manufacturerName + "  " + t.name + (t.isInstrument ? "  (instrument)" : ""));

    report.replaceWithText (lines.joinIntoString ("\n") + "\n");
    std::cout << lines[0] << std::endl;
    return list.getBlacklistedFiles().size();
}
