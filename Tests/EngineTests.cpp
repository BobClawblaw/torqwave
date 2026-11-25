// Integration checks against the real machine: document editing and file I/O, the plug-in scanner
// worker, the audio device (including multi-output interfaces) and rack save/restore with real plug-ins.
// The player is never started, so nothing is heard.

#include <JuceHeader.h>
#include "../Source/Sampler.h"
#include "../Source/SequenceLibrary.h"
#include "../Source/AudioEngine.h"
#include "../Source/PluginScanner.h"
#include "../Source/AudioEdits.h"
#include "../Source/PluginMenu.h"
#include "../Source/RackView.h"
#include "../Source/BuiltInEffects.h"
#include "DocumentTests.h"
#include "RecorderTests.h"
#include "MarkerTests.h"
#include "ConverterTests.h"
#include "SpectralTests.h"
#include "LoudnessTests.h"
#include "SoundLibraryTests.h"
#include "EqTests.h"
#include "DynamicsTests.h"
#include "ModulationTests.h"
#include "ReverbTests.h"
#include "ConvolverTests.h"
#include "EffectPresetsTests.h"
#include "GraphicEqTests.h"
#include "DockTests.h"
#include "OpenFilesTests.h"
#include "SettingsPanelTests.h"
#include "PanelTests.h"
#include "ThemeTests.h"
#include "EditorBarTests.h"
#include "NavigationTests.h"
#include "RestorationTests.h"
#include "GeneratorTests.h"
#include "InstrumentTests.h"
#include "WaveSynthBankTests.h"
#include "../Source/WaveSynth.h"
#include "../Source/DrumMachine.h"
#include "SwarmGuitarTests.h"
#include "SamplerTests.h"
#include "SamplerContent.h"
#include "InstrumentKeysTests.h"
#include "SoundDemos.h"
#include "SignalConsoleTests.h"
#include "DecryptTests.h"
#include "DecryptReal.h"
#include "WaveSynthTests.h"
#include "DrumMachineTests.h"
#include "SoundControlsTests.h"
#include "InstrumentTrackTests.h"
#include "NoteEditorTests.h"
#include "MidiInputTests.h"
#include "LossyFilesTests.h"
#include "FfmpegTests.h"
#include "FileInfoTests.h"
#include "SpectralShapeTests.h"
#include "DuckingTests.h"
#include "FadeHandleTests.h"
#include "TimeViewStripTests.h"
#include "MultibandTests.h"
#include "DynamicEqTests.h"
#include "ExciterTests.h"
#include "MaximizerTests.h"
#include "TransientShaperTests.h"
#include "MatchEqTests.h"
#include "TonalBalanceTests.h"
#include "DitherTests.h"
#include "DialogueIsolationTests.h"
#include "AdaptiveDenoiseTests.h"
#include "PlosiveTests.h"
#include "BreathTests.h"
#include "SpillTests.h"
#include "WowFlutterTests.h"
#include "SpectralRecoveryTests.h"
#include "RebalanceTests.h"
#include "AmbienceTests.h"
#include "DialogueContourTests.h"
#include "AzimuthTests.h"
#include "GuideTests.h"
#include "HomeTests.h"
#include "ResumeTests.h"
#include "AiTests.h"
#include "AiEval.h"
#include "CodecPreviewTests.h"
#include "BuiltInEffectsTests.h"
#include "EffectPreviewTests.h"
#include "NoiseTests.h"
#include "DereverbTests.h"
#include "LevellerTests.h"
#include "CentreChannelTests.h"
#include "PhaseCorrectionTests.h"
#include "PitchCorrectionTests.h"
#include "ScrubberTests.h"
#include "ClickTests.h"
#include "SessionTests.h"
#include "TrackEffectsTests.h"
#include "BusTests.h"
#include "SidechainTests.h"
#include "BatchTests.h"
#include "SequencesTests.h"
#include "SurroundTests.h"
#include "SurroundSessionTests.h"
#include "SurroundEffectsTests.h"
#include "SessionScrubTests.h"
#include "VideoTests.h"
#include "ProjectTests.h"
#include "AutomationTests.h"
#include "MultitrackViewTests.h"
#include "MixDownTests.h"
#include "EffectAutomationTests.h"
#include "TrackRecordingTests.h"
#include "CompTests.h"
#include "ArrangerTests.h"
#include "AutomationModeTests.h"
#include "DrumGridTests.h"
#include "MidiOutTests.h"
#include "ControlSurfaceTests.h"
#include "TempoFollowTests.h"
#include "StressTests.h"
#include "SequenceTests.h"
#include "CrossfadeTests.h"
#include "SoundDesignTests.h"
#include "PictureTests.h"
#include "SmallPiecesTests.h"
#include "MetronomeTests.h"
#include "MixerTests.h"
#include "BounceTests.h"
#include "AuditionTests.h"
#include "PluginTests.h"
#include "CommandAuditTests.h"
#include "PlayheadTests.h"
#include "StatisticsTests.h"
#include "ExtremeStretchTests.h"
#include "TimeFormatTests.h"
#include "TestPlugins.h"
#include "TestUtils.h"
#include "../Source/EditorBar.h"
#include "../Source/Statistics.h"
#include "../Source/TonalBalance.h"
#include "../Source/MultitrackView.h"
#include "../Source/MarkerTable.h"
#include "../Source/TimeFormat.h"
#include "../Source/Theme.h"
#include "../Source/Session.h"

namespace
{
    using TestUtils::check;
    using TestUtils::runUntil;

    juce::AudioPluginFormat* findFormat (AudioEngine& engine, const juce::String& name)
    {
        for (auto* f : engine.pluginFormatManager.getFormats())
            if (f->getName() == name)
                return f;

        return nullptr;
    }

    void testScanner (AudioEngine& engine, juce::Array<juce::PluginDescription>& found)
    {
        // The built app is the worker, exactly as in production.
        OutOfProcessScanner scanner (TestUtils::appExecutable);

        if (auto* au = findFormat (engine, "AudioUnit"))
        {
            // Apple's AUDelay ships with macOS: type aufx, subtype dely, manufacturer appl.
            juce::OwnedArray<juce::PluginDescription> results;
            const auto ok = scanner.findPluginTypesFor (*au, results, "AudioUnit:Effects/aufx,dely,appl");
            check (ok && results.size() == 1, "out-of-process scan finds Apple AUDelay (" + juce::String (results.size()) + ")");

            for (auto* d : results)
                found.add (*d);
        }

        if (auto* vst3 = findFormat (engine, "VST3"))
        {
            juce::StringArray candidates;

           #if JUCE_WINDOWS
            const auto env = [] (const char* name) { return juce::SystemStats::getEnvironmentVariable (name, {}); };
            const juce::Array<juce::File> dirs { juce::File (env ("CommonProgramFiles") + "/VST3"),
                                                 juce::File (env ("LOCALAPPDATA") + "/Programs/Common/VST3") };
           #else
            const juce::Array<juce::File> dirs { juce::File ("/Library/Audio/Plug-Ins/VST3"),
                                                 juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Library/Audio/Plug-Ins/VST3") };
           #endif

            for (auto& dir : dirs)
                for (auto& f : dir.findChildFiles (juce::File::findDirectories, false, "*.vst3"))
                    candidates.add (f.getFullPathName());

            candidates.sort (true);
            int scanned = 0, withTypes = 0;

            for (auto& path : candidates)
            {
                if (scanned == 8)
                    break;

                juce::OwnedArray<juce::PluginDescription> results;
                const auto ok = scanner.findPluginTypesFor (*vst3, results, path);
                ++scanned;
                std::cout << "      VST3 " << juce::File (path).getFileName() << ": "
                          << (ok ? juce::String (results.size()) + " type(s)" : juce::String ("worker failed")) << std::endl;

                if (ok && ! results.isEmpty())
                {
                    ++withTypes;

                    for (auto* d : results)
                        if (! d->isInstrument)
                            found.add (*d);
                }
            }

           #if ! JUCE_MAC
            if (candidates.isEmpty())   // macOS always has some; a fresh Windows machine none
                std::cout << "      (no VST3 plug-ins installed; scan of installed bundles skipped)" << std::endl;
            else
           #endif
            check (withTypes > 0, "out-of-process scan reads VST3 bundles (" + juce::String (withTypes) + "/" + juce::String (scanned) + ")");
        }

        scanner.scanFinished();
    }

    void testDevice (AudioEngine& engine)
    {
        const auto error = engine.openDevice (nullptr);
        check (error.isEmpty(), "device opens" + (error.isEmpty() ? juce::String() : ": " + error));

        auto* device = engine.deviceManager.getCurrentAudioDevice();

        if (device == nullptr)
            return;

        {
            // The probe at launch asks for the same device the engine opens, off the message thread,
            // and for a saved one that has gone, the default.
            juce::String probed, savedProbed, goneProbed;
            juce::XmlElement saved ("DEVICESETUP"), gone ("DEVICESETUP");
            saved.setAttribute ("audioOutputDeviceName", device->getName());
            gone.setAttribute ("audioOutputDeviceName", "No Such Interface");
            std::thread ([&]
            {
                probed = AudioEngine::probeDevice (nullptr);
                savedProbed = AudioEngine::probeDevice (&saved);
                goneProbed = AudioEngine::probeDevice (&gone);
            }).join();
            check (probed == device->getName() && savedProbed == device->getName() && goneProbed == probed,
                   "device probe: off the message thread it finds the device the engine opens (" + probed + "), the saved one, "
                   "and the default for one that has gone");

            // A setup saved while no device was open names none; it is passed over, so the default opens.
            juce::XmlElement none ("DEVICESETUP");
            none.setAttribute ("deviceType", "");
            none.setAttribute ("audioOutputDeviceName", "");
            none.setAttribute ("audioInputDeviceName", "");
            AudioEngine fresh;
            const auto freshError = fresh.openDevice (&none);
            auto* opened = fresh.deviceManager.getCurrentAudioDevice();
            check (AudioEngine::namingADevice (&none) == nullptr && AudioEngine::namingADevice (&saved) == &saved
                       && freshError.isEmpty() && opened != nullptr && opened->getName() == probed,
                   "device: a saved setup naming no device opens the default (" + (opened != nullptr ? opened->getName() : juce::String ("none")) + ")");
        }

        const auto names = device->getOutputChannelNames();
        std::cout << "      device: " << device->getName() << ", " << names.size() << " outputs, "
                  << device->getCurrentSampleRate() << " Hz, " << device->getCurrentBufferSizeSamples() << " samples" << std::endl;

        if (names.size() >= 10)
        {
            // Route to the fifth pair (channels 9/10) and check those device channels are active.
            engine.setOutputPair (8);
            const auto active = device->getActiveOutputChannels();
            check (engine.getOutputPair() == 8 && active[8] && active[9], "output pair 9/10 selected and active");
        }

        engine.setOutputPair (0);
        const auto active = engine.deviceManager.getCurrentAudioDevice()->getActiveOutputChannels();
        check (engine.getOutputPair() == 0 && active[0] && active[1], "output pair 1/2 selected and active");

        engine.setOutputPair (10000);
        check (engine.getOutputPair() == 0, "out-of-range output pair falls back to 1/2");

        if (names.size() >= 8)
        {
            // A track out of outputs 7/8 and a bus out of 31/32, which an interface with fewer
            // outputs doesn't have: the first pair is turned on and connected, the other left out.
            SessionState s;
            s.sampleRate = 48000.0;
            const auto trackId = s.addTrack().id;
            const auto busId = s.addBus().id;
            SessionEdits::setTrackDeviceOutput (s, trackId, 6);
            SessionEdits::setTrackDeviceOutput (s, busId, 30);
            engine.getPlayer().setMode (DocumentPlayerProcessor::Mode::session);   // connected while the session plays
            engine.session.reset (s, {});
            engine.useDeviceOutputs();
            TestUtils::runUntil ([&] { return engine.playsOutOf (6) && engine.playsOutOf (7); }, 5000);
            const auto on = engine.deviceManager.getCurrentAudioDevice()->getActiveOutputChannels();
            const auto connected = engine.playsOutOf (6) && engine.playsOutOf (7) && on[6] && on[7] && ! engine.playsOutOf (5) && ! engine.playsOutOf (8);
            const auto beyond = names.size() > 31 || (! engine.playsOutOf (30) && ! engine.playsOutOf (31));

            // Back to the mix: disconnected.
            SessionEdits::setTrackOutput (s, trackId, 0);
            engine.session.apply (s, "Track Output");
            TestUtils::runUntil ([&] { return ! engine.playsOutOf (6); }, 5000);
            check (connected && beyond && ! engine.playsOutOf (6) && ! engine.playsOutOf (7),
                   "interface outputs: a track set to outputs 7/8 of the " + juce::String (names.size()) + "-output interface turns them on "
                   "and is connected to them; a pair it doesn't have is left out; back to the mix, disconnected");
            engine.session.reset (SessionState(), {});
            engine.getPlayer().setMode (DocumentPlayerProcessor::Mode::document);
            engine.useDeviceOutputs();
        }
    }

    /** Arms the device's first two inputs and records a second while the document plays,
        checking the real device's callback order gives an anchor. */
    void testOutputLatency (AudioEngine& engine)
    {
        // With the device open the graph is prepared: a preview effect with 64 frames of latency is
        // reported to the player as it joins the chain, and taken back when it leaves.
        if (engine.deviceManager.getCurrentAudioDevice() == nullptr)
            return;

        auto& player = engine.getPlayer();
        engine.setPreviewEffect (std::make_unique<TestPlugins::DelayPlugin> (64, 1.0f));
        runUntil ([&] { return player.getOutputLatency() == 64; }, 5000);
        const auto told = player.getOutputLatency() == 64 && engine.getOutputLatency() == 64;
        engine.setPreviewEffect (nullptr);
        runUntil ([&] { return player.getOutputLatency() == 0; }, 5000);
        check (told && player.getOutputLatency() == 0 && engine.getOutputLatency() == 0,
               "output latency: a preview effect's 64 frames reach the player through the graph, and go when it is cleared");
    }

    void testLiveInput (AudioEngine& engine)
    {
        auto* device = engine.deviceManager.getCurrentAudioDevice();
        const auto names = engine.getDeviceInputChannelNames();
        std::cout << "      inputs: " << names.size() << std::endl;

        if (device == nullptr || names.size() < 2)
            return;

        const auto error = engine.setInput (0, 2);
        device = engine.deviceManager.getCurrentAudioDevice();
        const auto active = device->getActiveInputChannels();
        check (error.isEmpty() && active[0] && active[1] && engine.getRecorder().isInputActive(),
               "input 1/2 armed" + (error.isEmpty() ? juce::String() : ": " + error));

        const auto rate = device->getCurrentSampleRate();
        const auto latency = engine.getRoundTripLatency();
        std::cout << "      round trip " << latency << " frames at " << rate << " Hz, "
                  << device->getCurrentBufferSizeSamples() << "-frame buffer" << std::endl;
        check (latency > 0, "round-trip latency is reported");

        juce::AudioBuffer<float> silence (2, (int) (rate * 3.0));
        silence.clear();
        engine.document.reset (AudioEdits::fromBuffer (std::move (silence)), rate, {}, 24);
        engine.getPlayer().resetSource();

        auto& recorder = engine.getRecorder();
        check (recorder.start(), "recorder starts on the live device");
        engine.getPlayer().play (0, -1);
        const auto startMs = juce::Time::getMillisecondCounterHiRes();
        runUntil ([] { return false; }, 1000);
        const auto elapsed = (juce::Time::getMillisecondCounterHiRes() - startMs) / 1000.0;
        const auto take = recorder.stop();
        engine.getPlayer().pause();

        const auto expected = elapsed * rate;
        std::cout << "      recorded " << take.clip.getNumFrames() << " frames in " << elapsed << " s, anchor "
                  << take.anchorRecorded << " -> " << take.anchorPlayed << std::endl;
        check (take.droppedFrames == 0 && std::abs ((double) take.clip.getNumFrames() - expected) < rate * 0.1,
               "live take has the expected length with nothing dropped");
        check (take.anchorPlayed >= 0 && (double) take.anchorPlayed < rate * 0.2, "live take is anchored to playback");

        engine.setInput (0, 0);
        check (! engine.getRecorder().isInputActive()
                   && engine.deviceManager.getCurrentAudioDevice()->getActiveInputChannels().isZero(),
               "inputs turn off again");
    }

    /** The plug-ins to add, as the rack and the tracks offer them: a submenu a format, then a maker. */
    void testPluginMenu (const juce::Array<juce::PluginDescription>& found)
    {
        juce::Array<juce::PluginDescription> types;

        for (auto& d : found)
            if (! d.isInstrument)
                types.add (d);

        juce::PopupMenu menu;
        PluginMenu::add (menu, types);
        juce::StringArray formats;
        std::vector<int> seen ((size_t) types.size(), 0);
        auto placed = true, sorted = true;
        int maxId = 0;

        for (juce::PopupMenu::MenuItemIterator top (menu); top.next();)
        {
            const auto& format = top.getItem();
            formats.add (format.text);
            juce::String lastMaker;

            for (juce::PopupMenu::MenuItemIterator makers (*format.subMenu); makers.next();)
            {
                const auto& maker = makers.getItem();
                sorted = sorted && lastMaker.compareIgnoreCase (maker.text) < 0;
                lastMaker = maker.text;
                juce::String lastName;

                for (juce::PopupMenu::MenuItemIterator items (*maker.subMenu); items.next();)
                {
                    const auto& item = items.getItem();
                    const auto index = PluginMenu::chosen (types, item.itemID);

                    if (index < 0)
                    {
                        placed = false;
                        continue;
                    }

                    const auto& d = types.getReference (index);
                    seen[(size_t) index]++;
                    maxId = juce::jmax (maxId, item.itemID);
                    placed = placed && d.name == item.text && PluginMenu::formatTitle (d.pluginFormatName) == format.text
                             && (d.manufacturerName.trim().isEmpty() ? juce::String ("Unknown maker") : d.manufacturerName.trim()) == maker.text;
                    sorted = sorted && lastName.compareIgnoreCase (item.text) <= 0;
                    lastName = item.text;
                }
            }
        }

        const auto once = std::all_of (seen.begin(), seen.end(), [] (int n) { return n == 1; });
        const auto auFirst = ! formats.contains ("Audio Units") || formats[0] == "Audio Units";
        const auto vst3Next = ! formats.contains ("VST3") || formats.indexOf ("VST3") == (formats.contains ("Audio Units") ? 1 : 0);
        check (! types.isEmpty() && once && placed && sorted && auFirst && vst3Next && maxId < 1000000 && PluginMenu::chosen (types, 0) == -1
                   && PluginMenu::chosen (types, types.size() + 1) == -1,
               "plug-in menu: " + juce::String (types.size()) + " effects under " + formats.joinIntoString (", ")
                   + ", each once, under its maker, sorted; its ids stay below the track menu's slot actions");
    }

    void testRack (AudioEngine& engine, const juce::Array<juce::PluginDescription>& found)
    {
        int added = 0;

        // Apple's own effects first: third-party plug-ins may stop for a licence or activation
        // alert, which blocks an unattended run (and puts a dialog on the user's screen).
        juce::Array<juce::PluginDescription> candidates;

        for (auto& d : found)
            if (d.manufacturerName == "Apple" && ! d.isInstrument)
                candidates.add (d);

        if (candidates.isEmpty())
            candidates = found;

        for (auto& d : candidates)
        {
            if (added == 3)
                break;

            bool finished = false;
            juce::String error;
            engine.addPlugin (d, [&] (const juce::String& e) { error = e; finished = true; });
            runUntil ([&] { return finished; }, 30000);
            std::cout << "      add " << d.pluginFormatName << " " << d.name << ": " << (error.isEmpty() ? juce::String ("ok") : error) << std::endl;
            added += error.isEmpty() ? 1 : 0;
        }

        check (engine.getNumRackSlots() == added && added > 0, "plug-ins load into the rack (" + juce::String (added) + ")");

        if (added == 0)
            return;

        {
            // The device manager also announces device-list changes, which some plug-ins set off many
            // times a second; they must neither reach the engine's listeners nor rebuild the rack's
            // rows (a row replaced under the mouse loses its click).
            struct Counter final : public juce::ChangeListener
            {
                void changeListenerCallback (juce::ChangeBroadcaster*) override  { ++count; }
                int count = 0;
            } counter;

            RackView view (engine);
            view.setSize (400, 300);
            juce::Array<juce::Component*> before;

            for (auto* c : view.getChildren())
                before.add (c);

            engine.addChangeListener (&counter);

            for (int i = 0; i < 20; ++i)
                engine.deviceManager.sendSynchronousChangeMessage();

            juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
            const auto quietEngine = counter.count == 0;
            engine.sendChangeMessage();
            runUntil ([&] { return counter.count > 0; }, 1000);

            juce::Array<juce::Component*> after;

            for (auto* c : view.getChildren())
                after.add (c);

            const juce::Component::SafePointer<juce::Component> row (before.getLast());   // a row is gone once rebuilt
            engine.setRackSlotBypassed (0, true);
            runUntil ([&] { return counter.count > 1; }, 1000);
            const auto rebuilt = row == nullptr;
            engine.setRackSlotBypassed (0, false);
            runUntil ([&] { return counter.count > 2; }, 1000);
            engine.removeChangeListener (&counter);

            check (quietEngine && after == before && rebuilt,
                   "rack: device-list announcements pass the engine by, an engine change that leaves the rack alone keeps its rows, "
                   "and turning a slot off rebuilds them");
        }

        // Change a parameter and bypass so restore can be told apart from defaults.
        auto* first = engine.getRackPlugin (0);
        juce::AudioProcessorParameter* param = nullptr;

        // A continuous one: a switch or a choice would snap the value set below to another.
        for (auto* p : first->getParameters())
            if (p->isAutomatable() && ! p->isBoolean() && ! p->isDiscrete())
            {
                param = p;
                break;
            }

        float setValue = -1.0f;
        juce::String paramName;   // the plug-in is destroyed by clearRack(), so keep the name, not the pointer

        if (param != nullptr)
        {
            setValue = param->getValue() < 0.5f ? 0.8f : 0.2f;
            param->setValueNotifyingHost (setValue);
            paramName = param->getName (64);
        }

        engine.setRackSlotBypassed (0, true);

        if (added > 1)
            engine.moveRackSlot (1, 0);   // order becomes [second, first]

        const auto expectedFirstName = engine.getRackPlugin (0)->getName();
        auto xml = engine.createRackXml();

        engine.clearRack();
        check (engine.getNumRackSlots() == 0, "rack clears");

        bool finished = false;
        juce::String errors;
        engine.loadRackXml (*xml, [&] (const juce::String& e) { errors = e; finished = true; });
        runUntil ([&] { return finished; }, 60000);

        check (finished && errors.isEmpty() && engine.getNumRackSlots() == added, "rack restores from XML");
        check (engine.getRackPlugin (0)->getName() == expectedFirstName, "rack order survives save/load");

        const auto restoredIndex = added > 1 ? 1 : 0;
        check (engine.isRackSlotBypassed (restoredIndex), "bypass survives save/load");

        if (paramName.isNotEmpty())
        {
            for (auto* p : engine.getRackPlugin (restoredIndex)->getParameters())
            {
                if (p->getName (64) == paramName)
                {
                    check (std::abs (p->getValue() - setValue) < 0.01f,
                           "parameter '" + p->getName (64) + "' survives save/load (" + juce::String (p->getValue(), 3) + ")");
                    break;
                }
            }
        }

        engine.clearRack();

        // The same plug-in on a session track, saved and loaded back.
        SessionState s;
        s.sampleRate = 48000.0;
        const auto trackId = s.addTrack().id;
        engine.session.reset (s, {});

        finished = false;
        engine.addTrackPlugin (trackId, candidates[0], [&] (const juce::String& e) { errors = e; finished = true; });
        runUntil ([&] { return finished; }, 30000);
        check (finished && errors.isEmpty() && engine.getTrackEffects().getNumSlots (trackId) == 1 && engine.session.isDirty(),
               "a real plug-in loads onto a session track");

        engine.setTrackPluginBypassed (trackId, 0, true);
        std::map<juce::uint32, std::shared_ptr<juce::XmlElement>> saved;
        saved[trackId] = engine.getTrackEffects().createXml (trackId);
        engine.clearTrackEffects();

        finished = false;
        engine.loadTrackEffects (saved, [&] (const juce::String& e) { errors = e; finished = true; });
        runUntil ([&] { return finished; }, 30000);
        check (finished && errors.isEmpty() && engine.getTrackEffects().getNumSlots (trackId) == 1
                   && engine.getTrackEffects().isBypassed (trackId, 0),
               "track effects restore with their bypass");
        engine.clearTrackEffects();
    }
}

int main (int argc, char* argv[])
{
    // The live input test opens the interface's inputs. macOS asks for microphone access for
    // every new build of this binary and blocks until someone answers, so it runs only on request.
    //   --live-input            also record from the interface
    //   --app <executable>      use this copy of the app as the scan worker
    //   --plugin-sweep <report> only scan every plug-in on this machine and write what works
    //   --library <folder>      only scan this folder of sounds into a new Sound Library index, and time it
    //   --write-check <folder>  only write categories and tags into copies of WAV files from this folder, and check them
    //   --theme-sweep <folder>  only draw a page of the main panels in each theme into this folder, as <theme>.png
    //   --synth-window <folder> only save pictures of the synth's window (a patch, the level envelopes, a Sampled table)
    //   --patch-levels          only print each factory patch's level (name|category|dB of a held chord's peak)
    //   --patch-hashes <file>   only write a hash of each factory patch's render, to compare two builds sample for sample
    //   --sound-demos <folder>  only write the Sounds pages' demos (the built-in instruments' patches, presets and kits, and
    //                           the Sequence Library's factory kits and sequences) as MP3s with index.json; --only synth,kits for some
    //   --quality-demos <folder> only write the Quality page's before-and-after clips, their spectrograms and facts.json; --also clicks,clipping for two more
    //   --console-window <folder> only save pictures of the synth's window showing the Trace view
    //   --console               only the signal console's tests (Trace: the nodes within reach of a patch)
    //   --instruments           only the tests of the built-in instruments and their controls
    //   --patch-settings        only print the synth's factory patches with their settings' hashes (Scripts/patch_settings.txt)
    //   --patch-measure <file>  only measure the synth's factory patches (or those in --patches <file>) into this file,
    //                           for Scripts/make_patch_bank.py; --part k/n measures every nth, --wav <folder> writes each note
    //   --ai                    only the AI assistance tests (a mock server; no network)
    //   --metronome             only the tests of the click, the tempo map, loop playback and snapping
    //   --maximizer             only the Maximizer's tests
    //   --clicks                only Click and Pop Repair's tests
    //   --restoration           only the tests of hum removal and Repair Clipping
    //   --tempo                 only the tests of audio that follows the tempo
    //   --sequences             only the tests of sequences and the Sequence Library
    //   --sounddesign           only the tests of sound design in the session (edit in place, clip effects, layering)
    //   --picture               only the tests of working to picture
    //   --stress                only the stress test: how many tracks the mix keeps up with (minutes; not in the suite)
    //   --small                 only the tests of mixer scenes, channel strips and hardware inserts
    //   --ai-eval <url> <model> only run the AI evaluation sentences against that server (key: TORQWAVE_AI_KEY);
    //   --decrypt-real [files]  only Decrypt on recordings not from the synth: the files, some system sounds, wet patches
    //   --decrypt-chords [files]  only what Decrypt's chord finder makes of the files, of factory patches playing notes and chords, and of noise
    //   --decrypt-variety [files] [quick]  only how different from each other the patches Decrypt shows are, and its chain's links
    //   --decrypt-repeatable    only the check that a search shows the same patches on 2 threads and on 5
    //   --decrypt-window        only Decrypt's window checks
    //   --decrypt-model         only the check that a slow or hung model never makes Decrypt's page wait or go silent
    //   --decrypt-heard         only the check that a second sound given to Decrypt is heard, the synth's audio running
    //   --decrypt-pictures <folder>  only save pictures of the synth's window on Decrypt, and print each chain
    //   --decrypt-measure       only Decrypt's two sets of ten, in 16 rounds and until settled, with times (minutes)
    //   --decrypt-eval <url> <model>  only Decrypt's ten clips, alone and helped by the model at that server
    //                           add --ai-eval-set unseen (or all) for the requests it was never tuned on, session, sequence, library, decrypt or followup (requests that follow a conversation)
    juce::StringArray args;

    for (int i = 1; i < argc; ++i)
        args.add (argv[i]);

    const auto liveInput = args.contains ("--live-input");
    const auto valueOf = [&] (const char* flag) { const auto i = args.indexOf (flag); return i >= 0 ? args[i + 1] : juce::String(); };

    if (const auto app = valueOf ("--app"); app.isNotEmpty())
        TestUtils::appExecutable = juce::File::getCurrentWorkingDirectory().getChildFile (app);

    juce::ScopedJuceInitialiser_GUI juceInit;

    // The patch tool reads and writes only what it is given: it runs before anything shared is touched.
    if (int result = 0; runPatchTool (args, result))
        return result;

    juce::Array<juce::PluginDescription> found;

    // The factory presets stay out of the tests' libraries (and out of this Mac's own copy) unless a test unpacks them.
    const auto factoryPresets = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("TorqwaveTestFactoryPresets").getChildFile ("set");
    factoryPresets.getParentDirectory().deleteRecursively();
    SequenceLibrary::setFactoryFolder (factoryPresets);
    SequenceLibrary::setSearchesFile (factoryPresets.getSiblingFile ("Sequence Searches.xml"));

    // The sampler's factory samples too: unpacked into a folder of this run's own, taken away when it ends.
    struct FactorySamples
    {
        FactorySamples()
        {
            Sampler::unpackFactorySamples (folder);
            Sampler::setFactoryFolder (folder);
        }

        ~FactorySamples()       { folder.deleteRecursively(); }

        juce::File folder = juce::File::createTempFile ("factorysamples");
    } factorySamples;

    if (args.contains ("--decrypt-repeatable"))
    {
        runDecryptRepeatableTest();
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (const auto i = args.indexOf ("--decrypt-variety"); i >= 0)
    {
        juce::StringArray files;

        for (int f = i + 1; f < args.size() && ! args[f].startsWith ("--"); ++f)
            files.add (args[f]);

        return runDecryptVariety (files);
    }

    if (const auto i = args.indexOf ("--decrypt-chords"); i >= 0)
    {
        juce::StringArray files;

        for (int f = i + 1; f < args.size() && ! args[f].startsWith ("--"); ++f)
            files.add (args[f]);

        return runDecryptChords (files);
    }

    if (const auto i = args.indexOf ("--decrypt-real"); i >= 0)
    {
        juce::StringArray files;

        for (int f = i + 1; f < args.size() && ! args[f].startsWith ("--"); ++f)
            files.add (args[f]);

        return runDecryptReal (files);
    }

    if (const auto i = args.indexOf ("--decrypt-file"); i >= 0)
        return runDecryptFile (args[i + 1]);

    if (args.contains ("--decrypt-measure"))
        return runDecryptMeasure();

    if (const auto i = args.indexOf ("--decrypt-eval"); i >= 0)
        return runDecryptEval (args[i + 1], args[i + 2], juce::SystemStats::getEnvironmentVariable ("TORQWAVE_AI_KEY", {}));

    if (const auto i = args.indexOf ("--ai-eval"); i >= 0)
        return runAiEval (args[i + 1], args[i + 2], juce::SystemStats::getEnvironmentVariable ("TORQWAVE_AI_KEY", {}), valueOf ("--ai-eval-set")) == 0 ? 0 : 1;

    if (args.contains ("--crossfades"))
    {
        AudioEngine engine;
        runCrossfadeTests (engine);
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (const auto folder = valueOf ("--drums-demo"); folder.isNotEmpty())
    {
        saveDrumMachineDemos (juce::File (folder));
        return 0;
    }

    if (const auto folder = valueOf ("--drums-sounds"); folder.isNotEmpty())
    {
        saveDrumMachineSounds (juce::File::getCurrentWorkingDirectory().getChildFile (folder));
        return 0;
    }

    if (const auto folder = valueOf ("--drums-window"); folder.isNotEmpty())
    {
        saveDrumMachineWindow (juce::File (folder));
        return 0;
    }

    if (const auto folder = valueOf ("--drums-sources"); folder.isNotEmpty())
    {
        saveDrumSourceClips (juce::File (folder));
        return 0;
    }

    if (args.contains ("--drums") || args.contains ("--drums-levels"))
    {
        AudioEngine engine;
        runDrumMachineTests (engine, args.contains ("--drums-levels"));
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--sequences"))
    {
        AudioEngine engine;
        runSequenceTests (engine);
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (const auto folder = valueOf ("--decrypt-pictures"); folder.isNotEmpty())
    {
        saveDecryptWindow (juce::File (folder));
        return 0;
    }

    if (const auto folder = valueOf ("--console-window"); folder.isNotEmpty())
    {
        saveSignalConsoleWindow (juce::File (folder));
        return 0;
    }

    if (args.contains ("--decrypt-model"))
    {
        runDecryptModelTests();
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--decrypt-heard"))
    {
        runDecryptHeardTests();
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--decrypt-window"))
    {
        runDecryptWindowTests();
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--decrypt"))
    {
        runDecryptTests();
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--console"))
    {
        runSignalConsoleTests();
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (const auto folder = valueOf ("--synth-window"); folder.isNotEmpty())
    {
        saveSynthWindow (juce::File (folder));
        return 0;
    }

    if (args.contains ("--patch-levels"))
    {
        printPatchLevels();
        return 0;
    }

    if (const auto file = valueOf ("--patch-hashes"); file.isNotEmpty())
    {
        writePatchHashes (juce::File (file));
        return 0;
    }

    if (const auto folder = valueOf ("--sound-demos"); folder.isNotEmpty())
    {
        writeSoundDemos (juce::File::getCurrentWorkingDirectory().getChildFile (folder), juce::StringArray::fromTokens (valueOf ("--only"), ",", ""));
        return 0;
    }

    if (const auto folder = valueOf ("--quality-demos"); folder.isNotEmpty())
    {
        writeQualityDemos (juce::File::getCurrentWorkingDirectory().getChildFile (folder), juce::StringArray::fromTokens (valueOf ("--also"), ",", ""));
        return 0;
    }

    if (const auto folder = valueOf ("--swarm-window"); folder.isNotEmpty())
    {
        saveSwarmGuitarWindow (juce::File (folder));
        return 0;
    }

    if (const auto folder = valueOf ("--sampler-window"); folder.isNotEmpty())
    {
        saveSamplerWindow (juce::File (folder));
        return 0;
    }

    if (const auto folder = valueOf ("--sampler-folder"); folder.isNotEmpty())
        return trySamplerFolder (juce::File (folder));

    if (const auto folder = valueOf ("--sampler-content"); folder.isNotEmpty())
    {
        writeSamplerContent (juce::File (folder));
        return 0;
    }

    if (args.contains ("--voice-names"))
    {
        // The factory patches and kits a sequence preset's voice can name, for Scripts/make_presets.py.
        for (auto& patch : WaveSynth::factoryPatches())
            std::cout << "patch|" << patch.category << "|" << patch.name << std::endl;

        for (auto& kit : DrumMachine::kits())
            std::cout << "kit|" << kit.name << "|" << kit.description << std::endl;

        return 0;
    }

    if (args.contains ("--drumgrid"))
    {
        AudioEngine engine;
        runDrumGridTests (engine.formatManager);
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--synth"))
    {
        AudioEngine engine;
        runWaveSynthTests (engine);
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--swarm"))
    {
        AudioEngine engine;
        runSwarmGuitarTests (engine);
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--sampler"))
    {
        AudioEngine engine;
        runSamplerTests (engine);
        runSamplerTrackTests (engine);
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--keys"))
    {
        runInstrumentKeysTests();
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--effect-previews"))
    {
        AudioEngine engine;
        runEffectPreviewTests (engine);
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--stress"))
    {
        AudioEngine engine;
        runStressTests (engine);
        return 0;
    }

    if (args.contains ("--restoration"))
    {
        runRestorationTests();
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--clicks"))
    {
        runClickTests();
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--metronome"))
    {
        AudioEngine engine;
        runMetronomeTests (engine.formatManager);
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--maximizer"))
    {
        runMaximizerTests();
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--tempo"))
    {
        runTempoFollowTests();
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--sounddesign"))
    {
        AudioEngine engine;
        runSoundDesignTests (engine);
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (const auto folder = valueOf ("--resume-demo-unsaved"); folder.isNotEmpty())
    {
        const auto made = makeResumeDemo (juce::File::getCurrentWorkingDirectory().getChildFile (folder), true);
        std::cout << (made ? "Made " : "Could not make ") << folder << std::endl;
        return made ? 0 : 1;
    }

    if (const auto folder = valueOf ("--resume-demo"); folder.isNotEmpty())
    {
        const auto made = makeResumeDemo (juce::File::getCurrentWorkingDirectory().getChildFile (folder));
        std::cout << (made ? "Made " : "Could not make ") << folder << std::endl;
        return made ? 0 : 1;
    }

    if (args.contains ("--resume"))
    {
        runResumeTests();
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--picture"))
    {
        AudioEngine engine;
        runPictureTests (engine);
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--small"))
    {
        AudioEngine engine;
        runSmallPiecesTests (engine);
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--instruments"))
    {
        AudioEngine engine;
        runSoundControlsTests();
        runInstrumentTests (engine);
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (args.contains ("--ai"))
    {
        runAiTests();
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    // A page per theme, to look at: the editor with a lasso in hand, its strip, a session, the
    // statistics, the tonal balance and the marker list. Anything a component leaves unpainted
    // shows red.
    if (const auto dir = valueOf ("--theme-sweep"); dir.isNotEmpty())
    {
        Theme::LookAndFeel look;
        juce::LookAndFeel::setDefaultLookAndFeel (&look);
        const juce::File folder (dir);
        folder.createDirectory();
        const double rate = 48000.0;

        for (const auto& name : Theme::themeNames())
        {
            Theme::apply (Theme::make (name, "Blue"));
            AudioEngine engine;
            juce::AudioBuffer<float> buf (2, (int) (2 * rate));
            juce::Random rnd (1);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < buf.getNumSamples(); ++i)
                    buf.setSample (ch, i, (float) (0.4 * std::sin (juce::MathConstants<double>::twoPi * 220.0 * i / rate) * (0.5 + 0.5 * std::sin (juce::MathConstants<double>::twoPi * 0.7 * i / rate))
                                                   + 0.05 * (rnd.nextFloat() * 2.0f - 1.0f)));

            engine.document.reset (AudioEdits::fromBuffer (juce::AudioBuffer<float> (buf)), rate, {}, 24, Markers { { 12000, 0, "Intro" }, { 48000, 30000, "Verse" } });
            WaveformView view (engine);
            view.setBounds (0, 0, 900, 420);
            view.setDisplay (WaveformView::Display::both);
            view.zoomToFit();
            view.setSelection ({ 30000, 60000 });
            juce::ApplicationCommandManager commands;
            EditorBar bar (commands, view, engine.document, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 });
            bar.setBounds (0, 0, 900, EditorBar::height);
            view.setSpectralTool (WaveformView::SpectralTool::lasso);

            Statistics::Panel stats;
            stats.setBounds (0, 0, 450, 420);
            auto snapshot = engine.document.getSnapshot();
            stats.readSource = [snapshot, rate] { return std::optional<Statistics::Panel::Source> (Statistics::Panel::Source { snapshot, 0, 96000, rate }); };
            stats.formatTime = [rate] (juce::int64 f) { return TimeFormat::format (f, rate); };
            stats.scan();

            TonalBalance::Panel tonal;
            tonal.setBounds (0, 0, 450, 200);

            SessionState s;
            s.sampleRate = rate;
            s.addTrack(); s.addTrack(); s.addBus();
            s.tracks[0].name = "Voice"; s.tracks[1].name = "Music";
            s.markers = { { 12000, 0, "Intro" }, { 48000, 30000, "Verse" } };
            SessionEdits::addClip (s, 0, juce::File ("/takes/voice.wav"), AudioEdits::fromBuffer (juce::AudioBuffer<float> (buf)), 6000);
            SessionEdits::addClip (s, 1, juce::File ("/takes/music.wav"), AudioEdits::fromBuffer (juce::AudioBuffer<float> (buf)), 40000);
            engine.session.reset (s, {});
            MultitrackView multi (engine);
            multi.setBounds (0, 0, 900, 300);
            multi.zoomToRange ({ 0, 160000 });
            MarkerTable markers (engine.session, { {}, {}, {} });
            markers.setBounds (0, 0, 450, 160);
            TestUtils::runUntil ([&] { return ! stats.isScanning(); }, 5000);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (200);

            juce::Image page (juce::Image::RGB, 1360, 1000, true);
            juce::Graphics g (page);
            g.fillAll (juce::Colours::red);
            g.drawImageAt (view.createComponentSnapshot (view.getLocalBounds()), 0, 0);
            g.drawImageAt (bar.createComponentSnapshot (bar.getLocalBounds()), 0, 424);
            g.drawImageAt (multi.createComponentSnapshot (multi.getLocalBounds()), 0, 452);
            g.drawImageAt (stats.createComponentSnapshot (stats.getLocalBounds()), 905, 0);
            g.drawImageAt (tonal.createComponentSnapshot (tonal.getLocalBounds()), 905, 424);
            g.drawImageAt (markers.createComponentSnapshot (markers.getLocalBounds()), 905, 628);
            g.setColour (juce::Colours::white);
            g.drawText (name, 0, 760, 400, 30, juce::Justification::centredLeft);
            const auto file = folder.getChildFile (name.replaceCharacter (' ', '-') + ".png");
            file.deleteFile();
            juce::FileOutputStream out { file };
            juce::PNGImageFormat().writeImageToStream (page, out);
        }

        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
        return 0;
    }

    if (const auto report = valueOf ("--plugin-sweep"); report.isNotEmpty())
    {
        AudioEngine engine;
        runPluginSweep (engine, juce::File::getCurrentWorkingDirectory().getChildFile (report));
        return 0;
    }

    if (const auto folder = valueOf ("--write-check"); folder.isNotEmpty())
    {
        runWriteCheckOn (juce::File::getCurrentWorkingDirectory().getChildFile (folder));
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    if (const auto folder = valueOf ("--library"); folder.isNotEmpty())
    {
        runSoundLibraryOn (juce::File::getCurrentWorkingDirectory().getChildFile (folder));
        std::cout << (TestUtils::failures == 0 ? "ALL PASSED" : juce::String (TestUtils::failures) + " FAILED") << std::endl;
        return TestUtils::failures == 0 ? 0 : 1;
    }

    std::cout << "Scan worker: " << TestUtils::appExecutable.getFullPathName() << std::endl;

    {
        AudioEngine engine;
        runDocumentTests (engine);
        runRecorderTests();
        runMarkerTests (engine);
        runConverterTests();
        runCommandAuditTests();
        runPlayheadTests();
        runSpectralTests();
        runLoudnessTests();
        runSoundLibraryTests();
        runEqTests();
        runGraphicEqTests();
        runMultibandTests();
        runDynamicEqTests();
        runExciterTests();
        runMaximizerTests();
        runTransientShaperTests();
        runMatchEqTests();
        runTonalBalanceTests();
        runStatisticsTests();
        runExtremeStretchTests();
        runTimeFormatTests();
        runDitherTests();
        runDialogueIsolationTests();
        runAdaptiveDenoiseTests();
        runPlosiveTests();
        runBreathTests();
        runSpillTests();
        runWowFlutterTests();
        runSpectralRecoveryTests();
        runRebalanceTests();
        runAmbienceTests();
        runDialogueContourTests();
        runAzimuthTests();
        runGuideTests();
        runHomeTests();
        runResumeTests();
        runAiTests();
        runCodecPreviewTests();
        runDynamicsTests();
        runModulationTests();
        runReverbTests();
        runConvolverTests();
        runEffectPresetsTests();
        runBuiltInEffectsTests (engine);
        runNoiseTests();
        runDereverbTests();
        runLevellerTests();
        runScrubberTests();
        runCentreChannelTests();
        runPhaseCorrectionTests();
        runPitchCorrectionTests();
        runClickTests();
        runSessionTests (engine.formatManager);
        runTrackEffectsTests();
        runBusTests();
        runSidechainTests();
        runBatchTests (engine);
        runSequencesTests (engine);
        runEffectPreviewTests (engine);
        runSurroundTests (engine);
        runSurroundSessionTests (engine);
        runSurroundEffectsTests (engine);
        runSessionScrubTests (engine);
        runVideoTests();
        runProjectTests();
        runAutomationTests (engine.formatManager);
        runMultitrackViewTests (engine);
        runMixDownTests (engine);
        runEffectAutomationTests (engine.formatManager);
        runTrackRecordingTests (engine.formatManager);
        runCompTests (engine.formatManager);
        runArrangerTests (engine.formatManager);
        runAutomationModeTests (engine);
        runDrumGridTests (engine.formatManager);
        runMidiOutTests();
        runControlSurfaceTests (engine);
        runTempoFollowTests();
        runSoundDesignTests (engine);
        runPictureTests (engine);
        runSmallPiecesTests (engine);
        runSequenceTests (engine);
        runCrossfadeTests (engine);
        runMetronomeTests (engine.formatManager);
        runMixerTests (engine);
        runBounceTests();
        runAuditionTests (engine);
        runDockTests();
        runOpenFilesTests();
        runSettingsPanelTests();
        runPanelTests();
        runThemeTests();
        runEditorBarTests();
        runNavigationTests();
        runRestorationTests();
        runGeneratorTests();
        runInstrumentTests (engine);
        runSwarmGuitarTests (engine);
        runSamplerTests (engine);
        runDrumMachineTests (engine);
        runSoundControlsTests();
        runInstrumentTrackTests (engine);
        runNoteEditorTests();
        runMidiInputTests();
        runLossyFilesTests();
        runFfmpegTests();
        runFileInfoTests();
        runSpectralShapeTests();
        runDuckingTests();
        runFadeHandleTests();
        runTimeViewStripTests();

        // Scan the way the app does: on a background thread, with the message loop running.
        std::atomic<bool> scanned { false };
        std::thread scanThread ([&] { testScanner (engine, found); scanned = true; });
        runUntil ([&] { return scanned.load(); }, 600000);
        scanThread.join();

       #if ! JUCE_MAC
        // Without Apple's effects, and with no plug-ins installed, the menu and rack are checked with Torqwave's own.
        if (found.isEmpty())
            for (auto& d : BuiltInEffects::getDescriptions())
                if (! d.isInstrument)
                    found.add (d);
       #endif

        runPluginTests (engine);
        testPluginMenu (found);

        testDevice (engine);
        testOutputLatency (engine);
        runSurroundDeviceTests (engine);
        runMasterRackDeviceTests (engine);
        if (liveInput)
            testLiveInput (engine);
        testRack (engine, found);
    }

    const auto failures = TestUtils::failures;
    std::cout << (failures == 0 ? "ALL PASSED" : juce::String (failures) + " FAILED") << std::endl;
    return failures == 0 ? 0 : 1;
}
