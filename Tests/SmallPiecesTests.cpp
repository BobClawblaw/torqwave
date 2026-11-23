// Exact values are what the session holds.
#pragma clang diagnostic ignored "-Wfloat-equal"

#include <JuceHeader.h>
#include "../Source/AiSession.h"
#include "../Source/AudioEdits.h"
#include "../Source/AudioEngine.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/HardwareInsert.h"
#include "../Source/MixerView.h"
#include "../Source/SessionMixer.h"
#include "../Source/TrackTemplate.h"
#include "TestUtils.h"
#include "SmallPiecesTests.h"

namespace
{
    using TestUtils::check;

    constexpr double rate = 48000.0;

    juce::AudioBuffer<float> noiseBuffer (int frames, int seed)
    {
        juce::AudioBuffer<float> b (2, frames);
        juce::Random r (seed);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < frames; ++i)
                b.setSample (ch, i, r.nextFloat() * 0.5f - 0.25f);

        return b;
    }

    AudioDocument::Snapshot noise (int frames, int seed)   { return AudioEdits::fromBuffer (noiseBuffer (frames, seed)); }

    void settle()   { juce::MessageManager::getInstance()->runDispatchLoopUntil (30); }

    /** The item with this text in a menu or its submenus, or null. */
    const juce::PopupMenu::Item* findItem (const juce::PopupMenu& menu, const juce::String& text)
    {
        for (juce::PopupMenu::MenuItemIterator it (menu, true); it.next();)
            if (it.getItem().text == text)
                return &it.getItem();

        return nullptr;
    }

    /** A session of two tracks and two buses, mixed away from the defaults. */
    SessionState mixed()
    {
        SessionState s;
        s.sampleRate = rate;
        s.addTrack().name = "Voice";
        s.addTrack().name = "Music";
        s.addBus().name = "Reverb";
        s.addBus().name = "Delay";
        auto& voice = s.tracks[0];
        voice.gainDb = -3.5f;
        voice.pan = -0.25f;
        voice.invertPolarity = true;
        voice.surroundPan = { 0.5f, -0.5f, 0.75f, -6.0f };
        voice.sends = { { s.tracks[2].id, -12.0f, false }, { s.tracks[3].id, -20.5f, true } };
        s.tracks[1].mute = true;
        s.tracks[1].gainDb = 2.25f;
        s.tracks[2].solo = true;
        s.tracks[2].pan = 0.5f;
        return s;
    }

    /** Everything a scene holds, for every track. */
    std::vector<MixScene::Strip> mixOf (const SessionState& s)
    {
        std::vector<MixScene::Strip> strips;

        for (auto& t : s.tracks)
            strips.push_back (MixScene::stripOf (t));

        return strips;
    }

    void testScenes (AudioEngine& engine)
    {
        auto s = mixed();
        const auto stored = mixOf (s);
        const auto sceneId = SessionEdits::storeScene (s, "Verse");
        const auto second = SessionEdits::storeScene (s);
        const auto* scene = s.findScene (sceneId);
        const auto named = scene != nullptr && scene->name == "Verse" && s.findScene (second)->name == "Scene 2" && scene->strips == stored;

        // Everything changed, then recalled.
        auto changed = s;

        for (auto& t : changed.tracks)
        {
            t.gainDb = -40.0f;
            t.pan = 1.0f;
            t.mute = ! t.mute;
            t.solo = ! t.solo;
            t.invertPolarity = ! t.invertPolarity;
            t.surroundPan = {};
            t.sends.clear();
        }

        changed.tracks[1].sends = { { changed.tracks[2].id, 6.0f, true } };
        const auto recalled = SessionEdits::recallScene (changed, sceneId);
        const auto exact = recalled && mixOf (changed) == stored;
        const auto again = ! SessionEdits::recallScene (changed, sceneId) && ! SessionEdits::recallScene (changed, 999);
        check (named && exact && again,
               "mixer scenes: a stored scene recalls every volume, pan, surround pan, mute, solo, polarity and send exactly; recalling it "
               "again changes nothing");

        // A track added since keeps its mix; a bus deleted since loses its sends; a track deleted is passed over.
        auto later = s;
        later.addTrack().gainDb = 5.0f;
        later.tracks.erase (later.tracks.begin() + 3);   // Delay
        later.tracks[0].gainDb = 0.0f;
        SessionEdits::recallScene (later, sceneId);
        const auto partial = later.tracks[0].gainDb == -3.5f && later.tracks[0].sends.size() == 1 && later.tracks[0].sends[0].busId == s.tracks[2].id
                             && later.tracks.back().gainDb == 5.0f;

        // Update, rename, remove.
        auto edited = s;
        edited.tracks[0].gainDb = -9.0f;
        const auto updated = SessionEdits::updateScene (edited, sceneId) && edited.findScene (sceneId)->strips[0].gainDb == -9.0f
                             && ! SessionEdits::updateScene (edited, sceneId);
        const auto renamed = SessionEdits::renameScene (edited, sceneId, " Chorus ") && edited.findScene (sceneId)->name == "Chorus"
                             && ! SessionEdits::renameScene (edited, sceneId, "  ");
        const auto removed = SessionEdits::removeScene (edited, second) && edited.scenes.size() == 1 && ! SessionEdits::removeScene (edited, second);
        check (partial && updated && renamed && removed,
               "mixer scenes: tracks added since keep their mix and sends to deleted buses are left out; a scene updates, renames and goes");

        // As undo steps through the session.
        engine.session.reset (s, {});
        auto next = *engine.session.getState();
        next.tracks[0].gainDb = -30.0f;
        next.tracks[1].mute = false;
        engine.session.apply (std::move (next), "Track Volume");
        next = *engine.session.getState();
        SessionEdits::recallScene (next, sceneId);
        engine.session.apply (std::move (next), "Recall Mixer Scene");
        const auto applied = mixOf (*engine.session.getState()) == stored;
        engine.session.undo();
        const auto undone = engine.session.getState()->tracks[0].gainDb == -30.0f && ! engine.session.getState()->tracks[1].mute;
        engine.session.redo();
        check (applied && undone && mixOf (*engine.session.getState()) == stored, "mixer scenes: a recall is one undo step, undone and redone exactly");

        // Through a session file, and an older file without scenes.
        const auto file = juce::File::createTempFile (".tqsession");
        const auto error = SessionFile::save (s, file);
        const auto loaded = SessionFile::load (file, engine.formatManager);
        auto filed = error.isEmpty() && loaded.error.isEmpty() && loaded.state.scenes.size() == 2 && loaded.state.scenes[0].name == "Verse";

        if (filed)
        {
            // Ids are new in the loaded session: the scene must name the loaded tracks.
            auto back = loaded.state;
            const auto stripsBack = back.scenes[0].strips;
            auto same = stripsBack.size() == stored.size();
            const auto placeOf = [&s] (juce::uint32 id)
            {
                return (size_t) std::distance (s.tracks.begin(), std::find_if (s.tracks.begin(), s.tracks.end(), [id] (auto& t) { return t.id == id; }));
            };

            for (size_t i = 0; same && i < stored.size(); ++i)
            {
                auto want = stored[i];
                want.trackId = back.tracks[i].id;

                for (auto& send : want.sends)
                    send.busId = back.tracks[placeOf (send.busId)].id;

                same = stripsBack[i] == want;
            }

            for (auto& t : back.tracks)
                t.gainDb = 1.0f;

            filed = same && SessionEdits::recallScene (back, back.scenes[0].id) && back.tracks[0].gainDb == -3.5f && back.tracks[1].gainDb == 2.25f;
        }

        const auto older = juce::File::createTempFile (".tqsession");
        SessionFile::save (mixed(), older);
        const auto oldLoaded = SessionFile::load (older, engine.formatManager);
        file.deleteFile();
        older.deleteFile();
        check (filed && oldLoaded.error.isEmpty() && oldLoaded.state.scenes.empty() && oldLoaded.state.tracks[0].gainDb == -3.5f,
               "mixer scenes: saved with the session by each track's place and recalled exactly after loading; a session without them opens unchanged");
    }

    void testSceneBar (AudioEngine& engine)
    {
        engine.session.reset (mixed(), {});
        MixerView mixer (engine);
        mixer.setBounds (0, 0, 800, 500);

        mixer.findStoreSceneButton()->triggerClick();
        settle();
        const auto state = engine.session.getState();
        const auto stored = state->scenes.size() == 1 && state->scenes[0].name == "Scene 1" && engine.session.getUndoDescription() == "Store Mixer Scene";
        const auto sceneId = stored ? state->scenes[0].id : 0u;
        auto* button = mixer.findSceneButton (sceneId);
        const auto shown = button != nullptr && button->getButtonText() == "Scene 1" && button->getWidth() > 0;

        // Move the mix, then click the scene's button.
        mixer.findMute (state->tracks[0].id)->triggerClick();
        settle();
        auto next = *engine.session.getState();
        next.tracks[0].gainDb = -20.0f;
        engine.session.apply (std::move (next), "Track Volume");

        if (button != nullptr)
            button->triggerClick();

        settle();
        const auto recalled = mixOf (*engine.session.getState()) == mixOf (mixed()) && engine.session.getUndoDescription() == "Recall Mixer Scene";

        // Its menu: update, rename, delete.
        next = *engine.session.getState();
        next.tracks[1].gainDb = -1.0f;
        engine.session.apply (std::move (next), "Track Volume");
        auto menu = mixer.getSceneMenu (sceneId);

        if (const auto* update = findItem (menu, "Update Scene"))
            update->action();

        settle();
        const auto updated = engine.session.getState()->findScene (sceneId)->strips[1].gainDb == -1.0f
                             && engine.session.getUndoDescription() == "Update Mixer Scene";
        const auto renamedScene = mixer.renameScene (sceneId, "Mix B");
        settle();
        const auto renamed = renamedScene && mixer.findSceneButton (sceneId) != nullptr && mixer.findSceneButton (sceneId)->getButtonText() == "Mix B";

        menu = mixer.getSceneMenu (sceneId);

        if (const auto* remove = findItem (menu, "Delete Scene"))
            remove->action();

        settle();
        const auto deleted = engine.session.getState()->scenes.empty() && mixer.findSceneButton (sceneId) == nullptr
                             && mixer.getSceneMenu (sceneId).getNumItems() == 0;
        check (stored && shown && recalled && updated && renamed && deleted,
               "mixer scenes in the mixer: Store Scene adds a button that recalls the scene; its menu updates, renames and deletes it, each an undo step");

        // Ask Torq: store and recall by name.
        juce::StringArray steps;
        Ai::SessionTools tools (mixed(), "Song", steps);
        const auto storedByAi = (bool) (*tools.run ("store_scene", juce::JSON::parse (R"({"name": "Loud"})")))["ok"];
        const auto listed = (*tools.run ("get_session", {}))["mixer_scenes"].toString() == "Loud";
        Ai::SessionTools again (tools.getState(), "Song", steps);
        again.run ("set_track", juce::JSON::parse (R"({"track": "Voice", "volume": -30})"));
        const auto recalledByAi = (bool) (*again.run ("recall_scene", juce::JSON::parse (R"({"name": "loud"})")))["ok"]
                                  && again.getState().tracks[0].gainDb == -3.5f;
        const auto unknown = ! (bool) (*again.run ("recall_scene", juce::JSON::parse (R"({"name": "Quiet"})")))["ok"];
        // Both are among the session's tools (by name: their number changes as tools are added).
        juce::String defined;

        for (auto& d : Ai::SessionTools::toolDefinitions())
            defined << juce::JSON::toString (d, true) << "\n";

        check (storedByAi && listed && recalledByAi && unknown && defined.contains ("\"store_scene\"") && defined.contains ("\"recall_scene\""),
               "mixer scenes in Ask Torq: store_scene and recall_scene by name, get_session lists them, and an unknown name is refused");
    }

    /** A track's chain as TrackEffects saves it: a compressor at -30 dB and a reverb. */
    std::unique_ptr<juce::XmlElement> compressorAndReverb()
    {
        TrackEffects chain;
        chain.prepare (rate);
        auto compressor = std::make_unique<BuiltInEffects::CompressorProcessor>();
        auto settings = compressor->getSettings();
        settings.thresholdDb = -30.0;
        compressor->setSettings (settings);
        chain.add (1, std::move (compressor));
        chain.add (1, std::make_unique<BuiltInEffects::ReverbProcessor>());
        return chain.createXml (1);
    }

    juce::PluginDescription builtIn (const char* id)
    {
        for (auto& d : BuiltInEffects::getDescriptions())
            if (d.fileOrIdentifier == id)
                return d;

        return {};
    }

    bool addPlugin (AudioEngine& engine, juce::uint32 trackId, const char* id)
    {
        auto finished = false;
        engine.addTrackPlugin (trackId, builtIn (id), [&finished] (const juce::String&) { finished = true; });
        return TestUtils::runUntil ([&] { return finished; }, 10000);
    }

    void testChannelStrips (AudioEngine& engine)
    {
        // The preset: a track mixed into two buses, through a compressor and a reverb.
        SessionState from;
        from.sampleRate = rate;
        from.addTrack().name = "Lead";
        from.addBus().name = "Reverb";
        from.addBus().name = "Delay";
        from.tracks[0].gainDb = -4.5f;
        from.tracks[0].pan = 0.25f;
        from.tracks[0].invertPolarity = true;
        from.tracks[0].sends = { { from.tracks[1].id, -12.0f, true } };
        from.tracks[0].outputBusId = from.tracks[2].id;
        const auto strip = TrackTemplate::create (from, from.tracks[0].id, compressorAndReverb());

        const auto folder = juce::File::createTempFile ("strips");
        ChannelStrips::Store store (folder);
        const auto saved = strip != nullptr && store.save ("Bright Vocal", *strip) && store.getNames() == juce::StringArray { "Bright Vocal" };
        const auto read = store.load ("Bright Vocal");

        // The target: a track with an EQ and a send of its own, beside buses of the same names.
        SessionState s;
        s.sampleRate = rate;
        s.addTrack().name = "Voice";
        s.addBus().name = "Delay";
        s.addBus().name = "Reverb";
        s.addInstrumentTrack().name = "Keys";
        s.tracks[0].sends = { { s.tracks[1].id, 0.0f, false } };
        s.tracks[0].colour = 3;
        engine.clearTrackEffects();
        engine.session.reset (s, {});
        const auto voice = s.tracks[0].id, keys = s.tracks[3].id;
        const auto reverbId = s.tracks[2].id, delayId = s.tracks[1].id;
        const auto ready = addPlugin (engine, voice, BuiltInEffects::parametricEqId) && addPlugin (engine, keys, BuiltInEffects::basicSynthId);

        auto finished = false;
        juce::String problems;
        const auto applied = read != nullptr && engine.applyChannelStrip (voice, *read, [&] (const juce::String& p) { problems = p; finished = true; });
        TestUtils::runUntil ([&] { return finished; }, 10000);

        const auto& effects = engine.getTrackEffects();
        const auto* t = engine.session.getState()->findTrack (voice);
        auto* compressor = dynamic_cast<BuiltInEffects::CompressorProcessor*> (effects.getPlugin (voice, 0));
        const auto chain = effects.getNumSlots (voice) == 2 && compressor != nullptr && compressor->getSettings().thresholdDb == -30.0
                           && dynamic_cast<BuiltInEffects::ReverbProcessor*> (effects.getPlugin (voice, 1)) != nullptr;
        const auto settings = t != nullptr && t->gainDb == -4.5f && t->pan == 0.25f && t->invertPolarity && t->name == "Voice" && t->colour == 3
                              && t->sends == std::vector<SessionSend> { { reverbId, -12.0f, true } } && t->outputBusId == delayId;
        const auto oneStep = engine.session.getUndoDescription() == "Apply Channel Strip" && problems.isEmpty();
        check (saved && ready && applied && finished && chain && settings && oneStep,
               "channel strips: a preset applied to an existing track gives it the preset's effects in place of its own (their settings "
               "kept), its sends and output by bus name, its volume, pan and polarity, keeping its name and colour, as one undo step");

        engine.session.undo();
        const auto* back = engine.session.getState()->findTrack (voice);
        const auto undone = back != nullptr && back->gainDb == 0.0f && back->pan == 0.0f && ! back->invertPolarity
                            && back->sends == std::vector<SessionSend> { { delayId, 0.0f, false } } && back->outputBusId == 0;
        check (undone, "channel strips: an undo gives the track back its volume, pan, polarity, sends and output");

        // An instrument track keeps its instrument when the preset has none.
        finished = false;

        if (read != nullptr)
            engine.applyChannelStrip (keys, *read, [&] (const juce::String&) { finished = true; });

        TestUtils::runUntil ([&] { return finished; }, 10000);
        const auto kept = effects.getNumSlots (keys) == 3 && effects.getPlugin (keys, 0) != nullptr && TrackEffects::isInstrument (*effects.getPlugin (keys, 0))
                          && dynamic_cast<BuiltInEffects::CompressorProcessor*> (effects.getPlugin (keys, 1)) != nullptr;
        const auto notPreset = ! engine.applyChannelStrip (voice, juce::XmlElement ("SESSION"), {});
        check (kept && notPreset, "channel strips: an instrument track keeps its own instrument first, the preset's effects after it; "
                                  "anything that isn't a preset is refused");

        // From a mixer strip's menu.
        engine.clearTrackEffects();
        engine.session.reset (s, {});
        MixerView mixer (engine);
        mixer.setChannelStrips (store);
        const auto menu = mixer.getStripMenu (voice);
        const auto offered = findItem (menu, "Save Channel Strip...") != nullptr && findItem (menu, "Other File...") != nullptr;

        if (const auto* item = findItem (menu, "Bright Vocal"))
            item->action();

        TestUtils::runUntil ([&] { return effects.getNumSlots (voice) == 2; }, 10000);
        const auto fromMenu = effects.getNumSlots (voice) == 2 && engine.session.getState()->findTrack (voice)->gainDb == -4.5f;
        const auto copy = mixer.saveChannelStrip (voice, "Voice Copy") ? store.load ("Voice Copy") : nullptr;
        const auto savedFromMixer = copy != nullptr && copy->getChildByName ("EFFECTS") != nullptr
                                    && store.getNames() == juce::StringArray { "Bright Vocal", "Voice Copy" };
        engine.clearTrackEffects();
        folder.deleteRecursively();
        check (offered && fromMenu && savedFromMixer,
               "channel strips in the mixer: a strip's menu saves its channel strip by name and applies a saved one");
    }

    //==============================================================================
    /** An interface whose outputs 3/4 are cabled back into its inputs 3/4. */
    class LoopbackDevice final : public juce::AudioIODevice
    {
    public:
        LoopbackDevice() : AudioIODevice ("Loopback", "Test") {}

        juce::StringArray getOutputChannelNames() override        { return { "1", "2", "3", "4" }; }
        juce::StringArray getInputChannelNames() override         { return { "1", "2", "3", "4" }; }
        juce::Array<double> getAvailableSampleRates() override    { return { rate }; }
        juce::Array<int> getAvailableBufferSizes() override       { return { 512 }; }
        int getDefaultBufferSize() override                       { return 512; }
        juce::String open (const juce::BigInteger&, const juce::BigInteger&, double, int) override { return {}; }
        void close() override                                     {}
        bool isOpen() override                                    { return true; }
        void start (juce::AudioIODeviceCallback*) override        {}
        void stop() override                                      {}
        bool isPlaying() override                                 { return true; }
        juce::String getLastError() override                      { return {}; }
        int getCurrentBufferSizeSamples() override                { return 512; }
        double getCurrentSampleRate() override                    { return rate; }
        int getCurrentBitDepth() override                         { return 32; }
        juce::BigInteger getActiveOutputChannels() const override { juce::BigInteger b; b.setRange (0, 4, true); return b; }
        juce::BigInteger getActiveInputChannels() const override  { juce::BigInteger b; b.setRange (0, 4, true); return b; }
        int getOutputLatencyInSamples() override                  { return 100; }
        int getInputLatencyInSamples() override                   { return 50; }
    };

    /** The engine's graph player, standing in: plays the session's mix into outputs 1/2. */
    class MixCallback final : public juce::AudioIODeviceCallback
    {
    public:
        std::function<void (float* const*, int)> render;

        void audioDeviceIOCallbackWithContext (const float* const*, int, float* const* outputs, int, int numSamples,
                                               const juce::AudioIODeviceCallbackContext&) override
        {
            if (render != nullptr)
                render (outputs, numSamples);
        }

        void audioDeviceAboutToStart (juce::AudioIODevice*) override {}
        void audioDeviceStopped() override {}
    };

    /** A device running cycles through the Port's wrapper, outputs 3/4 coming back on inputs
        3/4 `delay` frames later. */
    struct Rig
    {
        explicit Rig (int d) : delay (d)
        {
            wrapper->audioDeviceAboutToStart (&device);
        }

        void cycle (int n)
        {
            juce::AudioBuffer<float> in (4, n), out (4, n);
            in.clear();
            out.clear();

            for (int ch = 2; ch < 4; ++ch)
                for (int i = 0; i < n; ++i)
                    if (const auto f = frame + i - delay; f >= 0)
                        in.setSample (ch, i, sent[(size_t) ch - 2][(size_t) f]);

            wrapper->audioDeviceIOCallbackWithContext (in.getArrayOfReadPointers(), 4, out.getArrayOfWritePointers(), 4, n, {});

            for (int ch = 0; ch < 2; ++ch)
            {
                sent[(size_t) ch].insert (sent[(size_t) ch].end(), out.getReadPointer (ch + 2), out.getReadPointer (ch + 2) + n);
                heard[(size_t) ch].insert (heard[(size_t) ch].end(), out.getReadPointer (ch), out.getReadPointer (ch) + n);
            }

            frame += n;
        }

        int delay;
        HardwareInsert::Port port;
        LoopbackDevice device;
        MixCallback mix;
        std::unique_ptr<juce::AudioIODeviceCallback> wrapper = port.wrap (mix);
        juce::int64 frame = 0;
        std::array<std::vector<float>, 2> sent, heard;   // what went out of 3/4, and of 1/2
    };

    /** Pings until the insert has its result (or many cycles pass). */
    HardwareInsert::Port::PingState pingThrough (Rig& rig, HardwareInsert::Processor& insert, int block)
    {
        if (! insert.ping())
            return HardwareInsert::Port::PingState::failed;

        for (int i = 0; i < 1000; ++i)
        {
            rig.cycle (block);

            if (const auto state = insert.collectPing(); state != HardwareInsert::Port::PingState::waiting)
                return state;
        }

        return HardwareInsert::Port::PingState::waiting;
    }

    void testPing()
    {
        using PingState = HardwareInsert::Port::PingState;
        constexpr auto guard = HardwareInsert::Port::guard;
        juce::StringArray found;
        auto exact = true;

        for (auto [delay, block] : { std::pair { 300, 128 }, std::pair { 1013, 441 }, std::pair { 4000, 1000 }, std::pair { 64, 64 } })
        {
            Rig rig (delay);
            HardwareInsert::Processor insert (rig.port);
            insert.prepareToPlay (rate, 1024);
            const auto estimated = insert.getLatencySamples() == 150 + guard && insert.getMeasuredRoundTrip() < 0;
            insert.setPairs (2, 2);
            const auto state = pingThrough (rig, insert, block);
            found.add (juce::String (insert.getMeasuredRoundTrip()));
            exact = exact && estimated && state == PingState::done && insert.getMeasuredRoundTrip() == delay
                    && insert.getLatencySamples() == delay + guard;
        }

        // Nothing cabled back: the ping fails; no device running: it doesn't start.
        Rig unplugged (500);
        HardwareInsert::Processor lonely (unplugged.port);
        lonely.prepareToPlay (rate, 1024);
        lonely.setPairs (0, 0);   // outputs 1/2 go nowhere here
        const auto failed = pingThrough (unplugged, lonely, 256) == PingState::failed && lonely.getMeasuredRoundTrip() < 0;
        HardwareInsert::Port stopped;
        HardwareInsert::Processor idle (stopped);
        const auto refused = ! idle.ping();

        // The measured round trip is kept in its state at its rate.
        Rig rig (777);
        HardwareInsert::Processor insert (rig.port);
        insert.prepareToPlay (rate, 1024);
        insert.setPairs (2, 2);
        pingThrough (rig, insert, 256);
        juce::MemoryBlock state;
        insert.getStateInformation (state);
        HardwareInsert::Processor copy (rig.port), otherRate (rig.port);
        copy.setStateInformation (state.getData(), (int) state.getSize());
        copy.prepareToPlay (rate, 1024);
        otherRate.setStateInformation (state.getData(), (int) state.getSize());
        otherRate.prepareToPlay (44100.0, 1024);
        const auto kept = copy.getMeasuredRoundTrip() == 777 && copy.getOutputFirst() == 2 && copy.getInputFirst() == 2
                          && copy.getLatencySamples() == 777 + guard && otherRate.getLatencySamples() == 150 + guard;

        check (exact && failed && refused && kept,
               "hardware insert: a ping through a simulated loopback measures round trips of 300, 1013, 4000 and 64 frames exactly at blocks "
               "of 128, 441, 1000 and 64 (found " + found.joinIntoString (", ") + "), the latency being that plus the guard of "
               + juce::String (guard) + "; nothing coming back fails; no device refuses; the round trip is saved with its rate");
    }

    /** The session played through the rig in blocks (from `blocks`, round and round), the insert
        on its first track's chain; the mix heard on outputs 1/2. */
    std::array<std::vector<float>, 2> playThrough (const SessionState& s, Rig& rig, TrackEffects& effects, const std::vector<int>& blocks, int frames)
    {
        SessionMixer::Buffers buffers;
        buffers.reserve (1024);
        juce::int64 at = 0;
        auto primed = false;
        const auto end = rig.frame + frames;

        rig.mix.render = [&] (float* const* outputs, int n)
        {
            // A jump to the start primes every chain, as the player does, in the cycle it plays in.
            if (! std::exchange (primed, true))
                SessionMixer::prime (s, 0, buffers, effects);

            juce::AudioBuffer<float> mix (outputs, 2, n);
            SessionMixer::render (s, at, mix, 0, n, buffers, &effects);
            at += n;
        };

        for (size_t k = 0; rig.frame < end; ++k)
            rig.cycle (blocks[k % blocks.size()]);

        rig.mix.render = nullptr;
        return rig.heard;
    }

    void testCompensation()
    {
        constexpr int delay = 1013, frames = 3 * (int) rate;
        auto aligned = true;
        juce::StringArray sizes;
        juce::Random random (42);
        std::vector<int> mixedSizes;

        for (int i = 0; i < 200; ++i)
            mixedSizes.push_back (32 + random.nextInt (969));

        for (const auto& blocks : std::vector<std::vector<int>> { { 64 }, { 100 }, { 256 }, { 441 }, { 512 }, { 1000 }, mixedSizes })
        {
            // Two tracks, from 1 s: noise through the insert, and other noise dry.
            SessionState s;
            s.sampleRate = rate;
            s.addTrack();
            s.addTrack();
            SessionEdits::addClip (s, 0, juce::File ("/takes/a.wav"), noise ((int) rate, 1), (juce::int64) rate);
            SessionEdits::addClip (s, 1, juce::File ("/takes/b.wav"), noise ((int) rate, 2), (juce::int64) rate + 1000);

            Rig rig (delay);
            TrackEffects effects;
            effects.prepare (rate);
            auto made = std::make_unique<HardwareInsert::Processor> (rig.port);
            auto* insert = made.get();
            effects.add (s.tracks[0].id, std::move (made));
            insert->setPairs (2, 2);
            pingThrough (rig, *insert, blocks[0]);

            // What it should give: the same session with no insert at all, sample for sample.
            SessionMixer::Buffers buffers;
            buffers.reserve (1024);
            juce::AudioBuffer<float> want (2, frames);

            for (int at = 0; at < frames; at += 512)
                SessionMixer::render (s, at, want, at, juce::jmin (512, frames - at), buffers);

            const auto heardFrom = rig.heard[0].size();
            const auto got = playThrough (s, rig, effects, blocks, frames);
            auto same = insert->getLatencySamples() == delay + HardwareInsert::Port::guard && effects.getLatency (s.tracks[0].id) == insert->getLatencySamples()
                        && insert->isLive();

            for (int ch = 0; ch < 2 && same; ++ch)
                for (int i = 0; i < frames && same; ++i)
                    same = got[(size_t) ch][heardFrom + (size_t) i] == want.getSample (ch, i);

            sizes.add (blocks.size() == 1 ? juce::String (blocks[0]) : "32 to 1,000 at random");
            aligned = aligned && same;
        }

        check (aligned, "hardware insert: through a simulated 1,013-frame loopback, delay compensation lines the return up with the dry track "
                        "sample for sample, the mix identical to the session without the insert, at blocks of " + sizes.joinIntoString (", "));
    }

    void testAwayFromHardware()
    {
        // Not in a device cycle (an offline render): the audio passes through, as late as its latency.
        HardwareInsert::Port port;
        HardwareInsert::Processor insert (port);
        insert.prepareToPlay (rate, 1024);
        const auto latency = insert.getLatencySamples();
        const auto input = noiseBuffer (5000, 3);
        auto b = input;

        juce::MidiBuffer midi;

        for (int at = 0; at < 5000; at += 300)
        {
            juce::AudioBuffer<float> part (b.getArrayOfWritePointers(), 2, at, juce::jmin (300, 5000 - at));
            insert.processBlock (part, midi);
        }

        auto delayed = ! insert.isLive();

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 5000; ++i)
                delayed = delayed && b.getSample (ch, i) == (i >= latency ? input.getSample (ch, i - latency) : 0.0f);

        // A device at another rate than the insert's isn't live either.
        Rig rig (500);
        HardwareInsert::Processor slower (rig.port);
        slower.prepareToPlay (44100.0, 1024);
        rig.mix.render = [&] (float* const*, int n)
        {
            juce::AudioBuffer<float> part (2, n);
            part.clear();
            slower.processBlock (part, midi);
        };
        rig.cycle (256);
        const auto otherRate = ! slower.isLive();

        check (delayed && otherRate && BuiltInEffects::getDescriptions().size() == 21,
               "hardware insert: away from the device's cycle (an offline render) or at another rate it passes the audio through, "
               "delayed by its latency (" + juce::String (latency) + " frames) sample for sample; it is among the built-in effects");
    }
}

void runSmallPiecesTests (AudioEngine& engine)
{
    std::cout << "Mixer scenes, channel strips and hardware inserts:" << std::endl;
    testScenes (engine);
    testSceneBar (engine);
    testChannelStrips (engine);
    testPing();
    testCompensation();
    testAwayFromHardware();
}
