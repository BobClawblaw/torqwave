#include <JuceHeader.h>
#include "../Source/AiAssistant.h"
#include "../Source/InstrumentPresets.h"
#include "../Source/AiCatalogue.h"
#include "../Source/PluginCatalogue.h"
#include "../Source/PluginScanner.h"
#include "../Source/AiClient.h"
#include "../Source/AiWorkspace.h"
#include "../Source/AudioEdits.h"
#include "../Source/AudioEngine.h"
#include "../Source/AiSequence.h"
#include "../Source/SequencerView.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/ClipEffects.h"
#include "../Source/Loudness.h"
#include "../Source/DrumMachine.h"
#include "../Source/DrumMaps.h"
#include "../Source/NoteEditor.h"
#include "../Source/SequenceLibrary.h"
#include "../Source/SwarmGuitar.h"
#include "../Source/WaveSynth.h"
#include "TestUtils.h"
#include "AiTests.h"

namespace
{
    using TestUtils::check;

    juce::var parse (const juce::String& json)
    {
        juce::var v;
        juce::JSON::parse (json, v);
        return v;
    }

    juce::MemoryBlock bytes (const juce::String& text)
    {
        return { text.toRawUTF8(), text.getNumBytesAsUTF8() };
    }

    //==============================================================================
    /** A server on 127.0.0.1 that answers each connection with the next of its replies, and keeps
        what each request sent. Stands in for a model server, so the client is tested with no network. */
    class MockServer final : private juce::Thread
    {
    public:
        struct Reply
        {
            int status = 200;
            juce::String contentType = "application/json", body;
            int delayMs = 0;           // before answering
        };

        struct Request
        {
            juce::String firstLine, headers, body;
        };

        explicit MockServer (std::vector<Reply> r) : juce::Thread ("Mock AI server"), replies (std::move (r))
        {
            listener.createListener (0, "127.0.0.1");
            startThread();
        }

        ~MockServer() override
        {
            signalThreadShouldExit();
            listener.close();
            stopThread (5000);
        }

        juce::String url() const    { return "http://127.0.0.1:" + juce::String (listener.getBoundPort()) + "/v1"; }

        std::vector<Request> requests() const
        {
            const juce::ScopedLock sl (lock);
            return received;
        }

    private:
        void run() override
        {
            for (auto& reply : replies)
            {
                std::unique_ptr<juce::StreamingSocket> s (listener.waitForNextConnection());

                if (s == nullptr || threadShouldExit())
                    return;

                Request request;
                juce::MemoryBlock data;
                char buffer[4096];

                // Headers, then as much body as Content-Length says.
                for (;;)
                {
                    if (s->waitUntilReady (true, 5000) != 1)
                        break;

                    const auto n = s->read (buffer, sizeof (buffer), false);

                    if (n <= 0)
                        break;

                    data.append (buffer, (size_t) n);
                    const auto text = data.toString();
                    const auto split = text.indexOf ("\r\n\r\n");

                    if (split < 0)
                        continue;

                    request.firstLine = text.upToFirstOccurrenceOf ("\r\n", false, false);
                    request.headers = text.substring (0, split);
                    const auto lengthAt = request.headers.indexOfIgnoreCase ("content-length:");
                    const auto length = lengthAt < 0 ? 0 : request.headers.substring (lengthAt + 15).upToFirstOccurrenceOf ("\r\n", false, false).trim().getIntValue();
                    const auto bodyBytes = (int) data.getSize() - (split + 4);

                    if (bodyBytes >= length)
                    {
                        request.body = juce::String::fromUTF8 ((const char*) data.getData() + split + 4, bodyBytes);
                        break;
                    }
                }

                {
                    const juce::ScopedLock sl (lock);
                    received.push_back (request);
                }

                if (reply.delayMs > 0)
                    wait (reply.delayMs);

                if (threadShouldExit())
                    return;

                const auto body = reply.body.toStdString();
                const auto head = "HTTP/1.1 " + juce::String (reply.status) + (reply.status < 400 ? " OK" : " Error") + "\r\n"
                                  + "Content-Type: " + reply.contentType + "\r\n"
                                  + "Content-Length: " + juce::String ((int) body.size()) + "\r\n"
                                  + "Connection: close\r\n\r\n";
                s->write (head.toRawUTF8(), (int) head.getNumBytesAsUTF8());
                s->write (body.data(), (int) body.size());
                juce::Thread::sleep (50);
                s->close();
            }
        }

        juce::StreamingSocket listener;
        std::vector<Reply> replies;
        juce::CriticalSection lock;
        std::vector<Request> received;
    };

    Ai::Settings settingsFor (const juce::String& url)
    {
        Ai::Settings s;
        s.enabled = true;
        s.serverUrl = url;
        s.model = "test-model";
        return s;
    }

    //==============================================================================
    void testSettings()
    {
        Ai::Settings s;
        check (! s.isOn() && s.baseUrl().isEmpty(), "AI: off by default, with no server");

        s.serverUrl = "localhost:11434";
        check (s.baseUrl() == "http://localhost:11434/v1", "AI: a host and port alone gets http:// and /v1");
        s.serverUrl = "http://box.lan:8080/v1/";
        check (s.baseUrl() == "http://box.lan:8080/v1", "AI: a full URL is kept, without its trailing slash");
        check (! s.isOn(), "AI: a server URL alone doesn't turn it on");
        s.enabled = true;
        check (s.isOn(), "AI: on with a server and the switch");

        juce::PropertySet p;
        s.model = "some-model";
        s.save (p);
        const auto back = Ai::Settings::load (p);
        check (back.enabled && back.serverUrl == s.serverUrl && back.model == "some-model", "AI: settings save and load");
        check (! p.getAllProperties().getAllValues().joinIntoString (" ").containsIgnoreCase ("key"), "AI: no key in the settings");
    }

    void testEndpoints()
    {
        // Saved before the list: that server comes back as a list of one.
        juce::PropertySet old;
        old.setValue ("aiEnabled", true);
        old.setValue ("aiServerUrl", "http://box.lan:8080/v1");
        old.setValue ("aiModel", "qwen");
        auto s = Ai::Settings::load (old);
        check (s.endpoints == std::vector<Ai::Endpoint> { { "Server 1", "http://box.lan:8080/v1", "qwen" } } && s.current == 0
                   && s.serverUrl == "http://box.lan:8080/v1" && s.model == "qwen" && s.isOn(),
               "AI endpoints: a server saved before the list comes back as the list's one endpoint, in use");

        // Adding: each new one is in use, the old keeps its address and model.
        s.addEndpoint ("Studio", "localhost:1234");
        s.model = "mistral";
        s.addEndpoint();
        s.serverUrl = "http://gpu.lan:8000/v1";
        s.model = "llama";
        const auto three = s.listed();
        check (three.size() == 3 && s.current == 2 && three[0] == Ai::Endpoint { "Server 1", "http://box.lan:8080/v1", "qwen" }
                   && three[1] == Ai::Endpoint { "Studio", "localhost:1234", "mistral" } && three[2] == Ai::Endpoint { "Server 3", "http://gpu.lan:8000/v1", "llama" },
               "AI endpoints: added servers each keep their own address and model, the new one in use and named in turn");

        // Choosing one puts its address and model in use; the requests go there.
        s.choose (1);
        check (s.current == 1 && s.baseUrl() == "http://localhost:1234/v1" && s.model == "mistral" && s.keyAccount() == "http://localhost:1234/v1",
               "AI endpoints: choosing a server uses its address, model and key");

        // Saved and loaded, as it was.
        juce::PropertySet p;
        s.save (p);
        const auto back = Ai::Settings::load (p);
        check (back.listed() == s.listed() && back.current == 1 && back.serverUrl == "localhost:1234" && back.model == "mistral" && back.enabled,
               "AI endpoints: the list and the one in use come back from the settings");

        // Deleting: one before the one in use, the one in use, and the last.
        s.removeEndpoint (0);
        const auto before = s.current == 0 && s.serverUrl == "localhost:1234" && s.listed().size() == 2;
        s.removeEndpoint (0);
        const auto inUse = s.current == 0 && s.serverUrl == "http://gpu.lan:8000/v1" && s.model == "llama" && s.listed().size() == 1;
        s.removeEndpoint (0);
        check (before && inUse && s.listed().empty() && s.serverUrl.isEmpty() && s.model.isEmpty() && ! s.isOn(),
               "AI endpoints: deleting one keeps the one in use; deleting the one in use moves to the next; deleting the last leaves none, and AI off");

        juce::PropertySet none;
        s.save (none);
        const auto empty = Ai::Settings::load (none);
        s.addEndpoint ({}, "http://first.lan:8000/v1");
        check (empty.listed().empty() && ! empty.isOn() && s.listed() == std::vector<Ai::Endpoint> { { "Server 1", "http://first.lan:8000/v1", {} } }
                   && s.serverUrl == "http://first.lan:8000/v1",
               "AI endpoints: an empty list stays empty, and a server added to it is the first, with its address");
    }

    /** Settings > AI's list, driven as a person would: Add, a name and address typed, choosing, Delete. */
    void testEndpointsView()
    {
        juce::PropertySet p;
        Ai::Settings start;
        start.enabled = true;
        start.addEndpoint ("Here", "http://127.0.0.1:9/v1");
        start.save (p);
        Ai::SettingsView view ([&p] { return Ai::Settings::load (p); }, [&p] (const Ai::Settings& x) { x.save (p); });
        view.setSize (600, Ai::SettingsView::preferredHeight);

        juce::TextButton* add = nullptr;
        juce::TextButton* remove = nullptr;
        juce::ComboBox* list = nullptr;
        std::vector<juce::TextEditor*> boxes;

        for (auto* c : view.getChildren())
        {
            if (auto* b = dynamic_cast<juce::TextButton*> (c); b != nullptr && b->getButtonText() == "Add")
                add = b;
            else if (b != nullptr && b->getButtonText() == "Delete")
                remove = b;
            else if (auto* box = dynamic_cast<juce::ComboBox*> (c); box != nullptr && list == nullptr)
                list = box;
            else if (auto* e = dynamic_cast<juce::TextEditor*> (c))
                boxes.push_back (e);
        }

        if (add == nullptr || remove == nullptr || list == nullptr || boxes.size() < 2)
        {
            check (false, "AI endpoints: the settings page has an endpoint list with Add and Delete");
            return;
        }

        auto* name = boxes[0];
        auto* url = boxes[1];
        const auto shownFirst = list->getNumItems() == 1 && list->getText() == "Here" && name->getText() == "Here" && url->getText() == "http://127.0.0.1:9/v1";

        add->onClick();
        // Each is kept as it is typed, the list showing the name at once.
        name->setText ("Rack", false);
        name->onTextChange();
        url->setText ("rack.lan:8000", false);
        url->onTextChange();
        const auto savedShown = [&view] (const juce::String& what)
        {
            for (auto* c : view.getChildren())
                if (auto* l = dynamic_cast<juce::Label*> (c); l != nullptr && l->getText().startsWith ("Saved at ") && l->getText().endsWith (": " + what + "."))
                    return true;

            return false;
        };
        const auto noted = savedShown ("Rack's address");
        name->setText ("Rack 2", false);
        name->onTextChange();
        const auto notedName = savedShown ("the name Rack 2") && list->getText() == "Rack 2";
        name->setText ("Rack", false);
        name->onTextChange();
        const auto added = Ai::Settings::load (p);
        const auto addedOk = added.listed().size() == 2 && added.current == 1 && added.listed()[1] == Ai::Endpoint { "Rack", "rack.lan:8000", {} }
                             && list->getNumItems() == 2 && list->getText() == "Rack" && noted && notedName;

        list->setSelectedItemIndex (0, juce::sendNotificationSync);
        const auto chosen = Ai::Settings::load (p).current == 0 && url->getText() == "http://127.0.0.1:9/v1" && name->getText() == "Here";

        remove->onClick();
        const auto left = Ai::Settings::load (p);
        const auto deleted = left.listed() == std::vector<Ai::Endpoint> { { "Rack", "rack.lan:8000", {} } } && left.serverUrl == "rack.lan:8000"
                             && list->getNumItems() == 1 && url->getText() == "rack.lan:8000";

        remove->onClick();
        const auto gone = Ai::Settings::load (p).listed().empty() && list->getNumItems() == 0 && ! remove->isEnabled() && url->getText().isEmpty();

        check (shownFirst && addedOk && chosen && deleted && gone,
               "AI endpoints: on the settings page, Add makes a server to name and address (each kept as it is typed, the list and a \"Saved at\" line showing it), the list chooses which is used, and Delete removes it, down to none");
    }

    void testReadingAnswers()
    {
        // A streamed answer: text, thinking, and a tool call split over several events.
        juce::MemoryInputStream streamed (bytes (juce::String (
            ": keep-alive\n\n"
            "data: {\"choices\":[{\"delta\":{\"reasoning_content\":\"Let me look.\"}}]}\n\n"
            "data: {\"choices\":[{\"delta\":{\"content\":\"Measuring\"}}]}\n\n"
            "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"c1\",\"function\":{\"name\":\"meas\",\"arguments\":\"{\\\"st\"}}]}}]}\n\n"
            "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"function\":{\"name\":\"ure\",\"arguments\":\"art\\\": 1}\"}}]}}]}\n\n"
            "data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":1,\"id\":\"c2\",\"function\":{\"name\":\"get_state\",\"arguments\":\"{}\"}}]}}]}\n\n"
            "data: {\"choices\":[{\"delta\":{},\"finish_reason\":\"tool_calls\"}]}\n\n"
            "data: [DONE]\n\n")), true);
        juce::String heard;
        const auto r = Ai::Client::readAnswer (streamed, [&heard] (const juce::String& t, bool reasoning) { heard << (reasoning ? "[" + t + "]" : t); }, {});
        check (r.error.isEmpty() && r.content == "Measuring" && r.reasoning == "Let me look." && heard == "[Let me look.]Measuring",
               "AI: a streamed answer's text and thinking are read, and heard as they come");
        check (r.toolCalls.size() == 2 && r.toolCalls[0].id == "c1" && r.toolCalls[0].name == "measure"
                   && parse (r.toolCalls[0].arguments)["start"] == juce::var (1) && r.toolCalls[1].name == "get_state" && r.finishReason == "tool_calls",
               "AI: tool calls streamed in pieces are put together in order");

        // A server that ignores "stream": the whole answer at once.
        juce::MemoryInputStream whole (bytes (juce::String (R"({"choices":[{"message":{"content":"Done.","tool_calls":[{"id":"x","type":"function","function":{"name":"select","arguments":"{\"all\":true}"}}]},"finish_reason":"stop"}]})")), true);
        const auto w = Ai::Client::readAnswer (whole, {}, {});
        check (w.error.isEmpty() && w.content == "Done." && w.toolCalls.size() == 1 && w.toolCalls[0].name == "select", "AI: an answer that wasn't streamed is read whole");

        juce::MemoryInputStream failed (bytes (juce::String ("data: {\"error\":{\"message\":\"model not loaded\"}}\n\n")), true);
        check (Ai::Client::readAnswer (failed, {}, {}).error.contains ("model not loaded"), "AI: an error in the stream is reported");

        juce::MemoryInputStream empty (bytes (juce::String ("data: {\"choices\":[{\"delta\":{},\"finish_reason\":\"length\"}]}\n\ndata: [DONE]\n\n")), true);
        check (Ai::Client::readAnswer (empty, {}, {}).error.contains ("ran out of room"), "AI: an answer cut off before it said anything is an error");

        check (Ai::Client::errorFrom (R"({"error":{"message":"bad key"}})", 401) == "The server answered 401: bad key", "AI: an API error body's message is shown");
    }

    void testServer()
    {
        const auto chunk = [] (const juce::String& json) { return "data: " + json + "\n\n"; };
        const auto stream = chunk (R"({"choices":[{"delta":{"content":"Hello"}}]})") + chunk (R"({"choices":[{"delta":{"content":" there"},"finish_reason":"stop"}]})") + "data: [DONE]\n\n";

        MockServer server ({ { 200, "application/json", R"({"object":"list","data":[{"id":"zeta"},{"id":"alpha"}]})" },
                             { 200, "text/event-stream", stream },
                             { 401, "application/json", R"({"error":{"message":"invalid key"}})" } });

        Ai::Client client (settingsFor (server.url()), "secret-key");
        juce::String error;
        const auto models = client.listModels (error);
        check (error.isEmpty() && models == juce::StringArray { "alpha", "zeta" }, "AI: the server's models are listed, sorted (" + models.joinIntoString (",") + error + ")");

        auto messages = juce::Array<juce::var> { parse (R"({"role":"user","content":"hi"})") };
        const auto r = client.chat (messages, Ai::Workspace::toolDefinitions());
        check (r.error.isEmpty() && r.status == 200 && r.content == "Hello there", "AI: a chat answer streams back from the server");

        const auto e = client.chat (messages, {});
        check (e.error == "The server answered 401: invalid key" && e.status == 401, "AI: the server's error is shown with its status");

        const auto requests = server.requests();
        check (requests.size() == 3 && requests[0].firstLine.startsWith ("GET /v1/models"), "AI: models come from /v1/models");

        if (requests.size() == 3)
        {
            const auto body = parse (requests[1].body);
            check (requests[1].firstLine.startsWith ("POST /v1/chat/completions") && body["model"] == juce::var ("test-model")
                       && (bool) body["stream"] && body["messages"].size() == 1 && body["tools"].size() == Ai::Workspace::toolDefinitions().size(),
                   "AI: the request names the model, streams, and carries the messages and tools");
            check (requests[1].headers.contains ("Authorization: Bearer secret-key") && requests[1].headers.containsIgnoreCase ("Content-Type: application/json"),
                   "AI: the key goes in the Authorization header");
            check (! parse (requests[2].body).hasProperty ("tools"), "AI: no tools, no tools field");
        }

        // No key: no Authorization header at all.
        MockServer keyless ({ { 200, "text/event-stream", stream } });
        Ai::Client noKey (settingsFor (keyless.url()), {});
        noKey.chat (messages, {});
        check (keyless.requests().size() == 1 && ! keyless.requests()[0].headers.containsIgnoreCase ("Authorization"), "AI: a server without a key gets no Authorization header");
    }

    void testCancelling()
    {
        MockServer slow ({ { 200, "text/event-stream", "data: [DONE]\n\n", 20000 } });
        Ai::Client client (settingsFor (slow.url()), {});
        auto messages = juce::Array<juce::var> { parse (R"({"role":"user","content":"hi"})") };

        std::atomic<bool> done { false };
        Ai::Reply r;
        const auto started = juce::Time::getMillisecondCounter();
        std::thread asking ([&] { r = client.chat (messages, {}); done = true; });

        juce::Thread::sleep (300);
        client.cancel();

        while (! done && juce::Time::getMillisecondCounter() - started < 10000)
            juce::Thread::sleep (10);

        const auto took = juce::Time::getMillisecondCounter() - started;
        asking.join();
        check (done && r.error == "Cancelled." && took < 5000, "AI: cancelling stops a request the server is slow to answer (" + juce::String (took) + " ms)");

        // Nothing listening there.
        Ai::Settings nowhere = settingsFor ("http://127.0.0.1:9");
        Ai::Client unreachable (nowhere, {});
        const auto u = unreachable.chat (messages, {});
        check (u.error.startsWith ("Couldn't reach"), "AI: a server that isn't there is reported");
    }

    //==============================================================================
    AudioDocument::Snapshot tone (double rate, int seconds)
    {
        // One second of silence, a 1 kHz tone at -12 dBFS, one second of silence.
        juce::AudioBuffer<float> b (2, (int) rate * seconds);
        b.clear();
        const auto level = juce::Decibels::decibelsToGain (-12.0f);

        for (int i = (int) rate; i < (int) rate * (seconds - 1); ++i)
            for (int c = 0; c < 2; ++c)
                b.setSample (c, i, level * (float) std::sin (juce::MathConstants<double>::twoPi * 1000.0 * i / rate));

        return AudioEdits::fromBuffer (std::move (b));
    }

    void testLookAndChange()
    {
        constexpr double rate = 48000.0;
        Ai::Workspace::Start start;
        start.audio = tone (rate, 5);
        start.sampleRate = rate;
        start.fileName = "tone.wav";
        start.markers = { { (juce::int64) rate * 2, 0, "Middle", -1 } };
        Ai::Workspace w (std::move (start));

        const auto call = [&w] (const juce::String& tool, const juce::String& args) { return parse (w.run (tool, args)); };

        const auto state = call ("get_state", "{}");
        check ((bool) state["ok"] && state["seconds"] == juce::var (5.0) && (int) state["channels"] == 2 && state["file"] == juce::var ("tone.wav"),
               "AI tools: get_state reads the file");

        const auto m = call ("measure", R"({"start": 1, "end": 4})");
        check ((bool) m["ok"] && std::abs ((double) m["channels"][0]["peak_dbfs"] + 12.0) < 0.05 && std::abs ((double) m["stereo_correlation"] - 1.0) < 0.001,
               "AI tools: measure gives each channel's peak and the correlation");

        const auto spectrum = call ("spectrum", R"({"start": 1, "end": 4})");
        check ((double) spectrum["octave_bands_dbfs"]["1 kHz"] > (double) spectrum["octave_bands_dbfs"]["125 Hz"] + 40.0,
               "AI tools: spectrum puts a 1 kHz tone in the 1 kHz band");

        const auto sounds = call ("find", R"({"what": "sounds"})");
        check ((int) sounds["count"] == 1 && std::abs ((double) sounds["found"][0]["start"] - 1.0) < 0.01, "AI tools: find locates the sound");

        check (! (bool) call ("edit", R"({"kind": "gain", "settings": {"db": 60}})")["ok"], "AI tools: a setting out of its range is refused");
        check (! (bool) call ("edit", R"({"kind": "gain", "settings": {"volume": 3}})")["ok"], "AI tools: a setting the kind doesn't have is refused");
        check (! (bool) call ("edit", R"({"kind": "warp"})")["ok"], "AI tools: an unknown kind is refused");
        check (! (bool) call ("measure", R"({"start": 4, "end": 2})")["ok"] && ! (bool) call ("measure", R"({"start": 1, "end": 99})")["ok"],
               "AI tools: a range backwards or past the end is refused");
        check (w.getSteps().isEmpty() && ! w.audioChanged(), "AI tools: refused calls change nothing");

        const auto gained = call ("edit", R"({"kind": "normalize", "settings": {"db": -1}})");
        check ((bool) gained["ok"] && w.audioChanged() && std::abs (juce::Decibels::gainToDecibels (AudioEdits::peakLevel (*w.getAudio(), 0, w.getAudio()->getNumFrames())) + 1.0f) < 0.01f,
               "AI tools: edit runs Normalize on the working copy");

        const auto loud = call ("edit", R"({"kind": "matchLoudness", "settings": {"target": -20, "ceiling": -1}})");
        const auto measured = Loudness::measure (*w.getAudio(), 0, w.getAudio()->getNumFrames(), rate);
        check ((bool) loud["ok"] && measured && std::abs (measured->integrated + 20.0) < 0.1, "AI tools: Match Loudness reaches the target");

        call ("select", R"({"start": 1, "end": 4})");
        call ("fade", R"({"direction": "in", "seconds": 0.5})");
        check (w.getSteps()[w.getSteps().size() - 1].startsWith ("Fade In (S-Curve), 1.000 s to 1.500 s"), "AI tools: a fade takes the selection's start and its length");

        // The fade starts below -60 dB, so a few milliseconds of it go too.
        const auto trimmed = call ("trim_silence", R"({"threshold_db": -60})");
        const auto cut = 5.0 - (double) w.getAudio()->getNumFrames() / rate;
        check ((bool) trimmed["ok"] && cut > 2.0 && cut < 2.05 && w.getMarkers().size() == 1
                   && std::abs ((double) w.getMarkers()[0].start / rate - 1.0) < 0.05,
               "AI tools: trim_silence cuts the quiet ends and moves the markers with the audio (" + trimmed["step"].toString() + ")");

        call ("add_marker", R"({"start": 0.5, "end": 1.5, "name": "Hit"})");
        call ("rename_marker", R"({"number": 2, "name": "Centre"})");
        const auto list = call ("list_markers", "{}");
        check (list["markers"].size() == 2 && list["markers"][0]["name"] == juce::var ("Hit") && list["markers"][1]["name"] == juce::var ("Centre")
                   && w.markersChanged(), "AI tools: markers are added and renamed");

        const auto before = w.getAudio()->getNumFrames();
        const auto centre = w.getMarkers()[1].start;
        call ("delete", R"({"start": 0, "end": 0.5})");
        check (before - w.getAudio()->getNumFrames() == (juce::int64) (rate / 2) && w.getMarkers()[0].start == 0 && w.getMarkers()[1].start == centre - (juce::int64) (rate / 2),
               "AI tools: delete takes the time out, markers follow");
        check (w.getSteps().size() == 7, "AI tools: each change is a step in the plan (" + juce::String (w.getSteps().size()) + ")");
        check (call ("nothing", "{}")["error"].toString().contains ("no tool"), "AI tools: an unknown tool is refused");
        check (! (bool) call ("measure", "{not json")["ok"], "AI tools: arguments that aren't JSON are refused");

        // Every tool definition is a well-formed function with a name.
        const auto tools = Ai::Workspace::toolDefinitions();
        auto named = 0;

        for (auto& t : *tools.getArray())
            named += t["type"] == juce::var ("function") && t["function"]["name"].toString().isNotEmpty() && t["function"]["parameters"].isObject() ? 1 : 0;

        check (tools.size() == 26 && named == tools.size(), "AI tools: 26 tools, each a function with parameters");
    }

    void testChannelSteps()
    {
        // The steps the app has for one channel of a stereo file, and the band-split shapers.
        constexpr double rate = 48000.0;
        Ai::Workspace::Start start;
        start.audio = tone (rate, 3);
        start.sampleRate = rate;
        start.fileName = "tone.wav";
        Ai::Workspace w (std::move (start));
        const auto call = [&w] (const juce::String& tool, const juce::String& args) { return parse (w.run (tool, args)); };
        constexpr int at = 48012;   // a sample of the tone, well off zero
        const auto sample = [&w] (int channel)
        {
            juce::AudioBuffer<float> b (2, 1);
            w.getAudio()->read (b, 0, at, 1);
            return b.getSample (channel, 0);
        };
        const auto before = sample (0);

        const auto inverted = call ("edit", R"({"kind": "invertLeft"})");
        const auto leftOnly = (bool) inverted["ok"] && std::abs (sample (0) + before) < 1.0e-6f && std::abs (sample (1) - before) < 1.0e-6f;
        call ("edit", R"({"kind": "rightToLeft"})");
        const auto copied = std::abs (sample (0) - before) < 1.0e-6f;
        const auto shaped = (bool) call ("edit", R"({"kind": "transientShaper", "settings": {"midAttack": 6}})")["ok"]
                            && (bool) call ("edit", R"({"kind": "exciter", "settings": {"highDrive": 12, "highCurve": 1}})")["ok"];
        const auto refused = ! (bool) call ("edit", R"({"kind": "exciter", "settings": {"lowCurve": 3}})")["ok"];
        const auto others = (bool) call ("edit", R"({"kind": "phase"})")["ok"] && (bool) call ("edit", R"({"kind": "centre", "settings": {"reduction": 12}})")["ok"]
                            && (bool) call ("edit", R"({"kind": "dither", "settings": {"bits": 1}})")["ok"];
        const auto longer = call ("edit", R"({"kind": "extremeStretch", "settings": {"factor": 2}})");
        const auto stretched = (bool) longer["ok"] && std::abs ((double) w.getAudio()->getNumFrames() / rate - 6.0) < 0.5;
        check (leftOnly && copied && shaped && refused && others && stretched,
               "AI tools: one channel inverted or copied over the other, the transient shaper, exciter, phase correction, centre, dither and extreme stretch, as the app's own steps");
    }

    void testEffectsMenuSteps()
    {
        // The Effects menu's commands that run their own code, through Ask Torq's edit tool: each
        // runs with its defaults, and a few are checked for what they do.
        constexpr double rate = 48000.0;
        const auto workspace = [] (int seconds)
        {
            Ai::Workspace::Start start;
            start.audio = tone (rate, seconds);
            start.sampleRate = rate;
            start.fileName = "tone.wav";
            return std::make_unique<Ai::Workspace> (std::move (start));
        };

        juce::StringArray failed;

        for (auto* kind : { "removeHum", "declip", "clickRepair", "noiseReduction", "adaptiveDenoise", "dialogueIsolation", "spill", "ambienceMatch",
                            "wowFlutter", "azimuth", "spectralRecovery", "rebalance", "dialogueContour", "spectralAttenuate", "spectralRemove", "graphicEq10",
                            "multibandCompressor", "noiseGate", "gainRamp", "panRamp", "fadeEdges", "crossfadeLoop", "silenceOutside", "silenceLeft",
                            "silenceRight", "normalizeRegions", "fadeRegions", "reverseRegions", "shortenPauses", "generateTone", "generateNoise" })
        {
            auto w = workspace (2);
            const auto r = parse (w->run ("edit", juce::String (R"({"kind": "KIND", "start": 0.5, "end": 1.5})").replace ("KIND", kind)));

            if (! (bool) r["ok"])
                failed.add (juce::String (kind) + ": " + r["error"].toString());
        }

        check (failed.isEmpty(), "AI tools: every Effects menu step runs with its defaults" + (failed.isEmpty() ? juce::String() : " (" + failed.joinIntoString ("; ") + ")"));

        const auto sampleAt = [] (Ai::Workspace& w, double seconds, int channel)
        {
            juce::AudioBuffer<float> b (2, 64);
            w.getAudio()->read (b, 0, (juce::int64) (seconds * rate), 64);
            return b.getMagnitude (channel, 0, 64);
        };

        auto w = workspace (5);   // the tone sounds from 1 s to 4 s
        const auto peak = sampleAt (*w, 2.0, 0);
        w->run ("edit", R"({"kind": "gainRamp", "start": 1, "end": 4, "settings": {"from": 0, "to": -96}})");
        const auto ramped = peak > 0.2f && sampleAt (*w, 1.01, 0) > peak * 0.8f && sampleAt (*w, 3.99, 0) < peak * 0.01f;
        w->run ("edit", R"({"kind": "silenceRight", "start": 1, "end": 4})");
        const auto silenced = sampleAt (*w, 1.5, 1) == 0.0f && sampleAt (*w, 1.5, 0) > 0.01f;
        const auto inserted = parse (w->run ("edit", R"({"kind": "generateTone", "start": 1, "settings": {"length": 1.5}})"));
        const auto longer = (bool) inserted["ok"] && std::abs ((double) inserted["seconds_now"] - 6.5) < 0.01;
        const auto badBand = ! (bool) parse (w->run ("edit", R"({"kind": "spectralRemove", "settings": {"low": 3000, "high": 2000}})"))["ok"];
        const auto badPrint = ! (bool) parse (w->run ("edit", R"({"kind": "noiseReduction", "settings": {"printStart": 1, "printEnd": 1}})"))["ok"];
        const auto badNoise = ! (bool) parse (w->run ("edit", R"({"kind": "generateNoise", "settings": {"colour": 1, "level": -10}})"))["ok"];
        check (ramped, "AI tools: a gain ramp fades the file as Gain Ramp does");
        check (silenced, "AI tools: one channel silenced, the other kept");
        check (longer, "AI tools: a generated tone is inserted, making the file longer (" + inserted["seconds_now"].toString() + " s)");
        check (badBand && badPrint && badNoise, "AI tools: impossible settings are refused (a band upside down, an empty noise print, pink noise too loud)");
    }

    //==============================================================================
    /** A parameter whose plug-in can't read text back (as some can't): 0 to 100 "units", linear. */
    class Unreadable final : public juce::AudioProcessorParameter
    {
    public:
        float getValue() const override                     { return value; }
        void setValue (float v) override                    { value = v; }
        float getDefaultValue() const override              { return 0.0f; }
        juce::String getName (int) const override           { return "Amount"; }
        juce::String getLabel() const override              { return "units"; }
        juce::String getText (float v, int) const override  { return juce::String (v * 100.0f, 1) + " units"; }
        float getValueForText (const juce::String&) const override   { return 0.0f; }

        float value = 0.5f;
    };

    void testParameters()
    {
        check (Ai::Parameters::numberIn ("-6.0 dB") == -6.0 && Ai::Parameters::numberIn ("1.2 kHz") == 1200.0 && Ai::Parameters::numberIn ("4:1") == 4.0
                   && ! Ai::Parameters::numberIn ("Hall").has_value() && ! Ai::Parameters::numberIn ("-inf dB").has_value(),
               "AI parameters: numbers read from values' text");

        Unreadable p;
        juce::String error;
        const auto now = Ai::Parameters::set (p, "37 units", error);
        check (error.isEmpty() && now == "37.0 units", "AI parameters: a value the plug-in can't read is found by searching its range (" + now + ")");

        Ai::Parameters::set (p, "250", error);
        check (error.contains ("outside its range"), "AI parameters: a number outside the range is reported, not clamped silently");
    }

    void testPlugins()
    {
        juce::AudioPluginFormatManager formats;
        formats.addFormat (BuiltInEffects::createFormat());

        constexpr double rate = 48000.0;
        Ai::Workspace::Start start;
        start.audio = tone (rate, 3);
        start.sampleRate = rate;
        start.plugins = BuiltInEffects::getDescriptions();
        start.formats = &formats;
        Ai::Workspace w (std::move (start));
        const auto call = [&w] (const juce::String& tool, const juce::String& args) { return parse (w.run (tool, args)); };

        const auto found = call ("list_plugins", R"({"search": "limit"})");
        check ((int) found["count"] == 1 && found["plugins"][0]["name"] == juce::var ("Limiter"), "AI plug-ins: list_plugins searches by name");
        check (! (bool) call ("add_plugin", R"({"plugin": "Torqwave Synth"})")["ok"], "AI plug-ins: an instrument isn't put in the effect chain");

        const auto added = call ("add_plugin", R"({"plugin": "Limiter"})");
        check ((bool) added["ok"] && (int) added["slot"] == 1 && w.rackChanged(), "AI plug-ins: a plug-in is added by name");

        const auto params = call ("get_parameters", R"({"slot": 1})");
        auto hasCeiling = false;

        for (auto& p : *params["parameters"].getArray())
            hasCeiling = hasCeiling || p["name"] == juce::var ("Ceiling");

        check ((bool) params["ok"] && hasCeiling, "AI plug-ins: get_parameters lists the plug-in's parameters");

        const auto set = call ("set_parameters", R"({"slot": 1, "values": {"Ceiling": "-6 dBFS", "Gain": 12, "Warmth": 3}})");
        check (set["results"][0]["ok"] && set["results"][0]["value"].toString().startsWith ("-6") && set["results"][1]["ok"]
                   && set["results"][1]["value"].toString().startsWith ("12") && ! (bool) set["results"][2]["ok"],
               "AI plug-ins: parameters are set by name from text and numbers, and an unknown one is reported (" + juce::JSON::toString (set["results"], true) + ")");

        const auto rendered = call ("apply_chain", R"({"start": 0.5, "end": 2.5})");
        const auto peak = juce::Decibels::gainToDecibels (AudioEdits::peakLevel (*w.getAudio(), 0, w.getAudio()->getNumFrames()));
        check ((bool) rendered["ok"] && peak < -5.9f && peak > -6.5f, "AI plug-ins: apply_chain renders through the chain (the limiter holds the +12 dB tone at -6 dBFS: " + juce::String (peak, 2) + ")");
        check (w.getRackRendered() != nullptr && w.getRackRendered()->getNumChildElements() == 1 && w.createRackXml()->getNumChildElements() == 0,
               "AI plug-ins: the rendered chain is kept for the undo step and the chain is emptied");
        check (! (bool) call ("get_parameters", R"({"slot": 1})")["ok"], "AI plug-ins: an empty chain has no slot 1");

        // A workspace with no plug-in host refuses plug-in tools.
        Ai::Workspace::Start bare;
        bare.audio = tone (rate, 3);
        Ai::Workspace none (std::move (bare));
        check (! (bool) parse (none.run ("add_plugin", R"({"plugin": "Limiter"})"))["ok"], "AI plug-ins: refused where there's no plug-in host");
    }

    //==============================================================================
    juce::PluginDescription builtIn (const juce::String& name)
    {
        for (auto& d : BuiltInEffects::getDescriptions())
            if (d.name == name)
                return d;

        return {};
    }

    void testCatalogueStore()
    {
        const juce::TemporaryFile temp (".json");
        const auto file = temp.getFile();
        juce::AudioPluginFormatManager formats;
        formats.addFormat (BuiltInEffects::createFormat());
        const auto limiter = builtIn ("Limiter");
        juce::String error;
        auto plugin = formats.createPluginInstance (limiter, 48000.0, 512, error);
        const auto facts = PluginCatalogue::factsOf (*plugin);

        juce::var ceiling;

        for (auto& p : *facts["parameters"].getArray())
            if (p["name"] == juce::var ("Ceiling"))
                ceiling = p;

        check ((int) facts["parameterCount"] == plugin->getParameters().size() && (int) facts["inputs"] == plugin->getTotalNumInputChannels()
                   && ceiling["lowest"].toString().isNotEmpty() && ceiling["highest"].toString().isNotEmpty() && ceiling["default"].toString().isNotEmpty()
                   && ! ceiling.hasProperty ("value"),
               "plug-in catalogue: a plug-in's facts are its parameters with their ranges and defaults, and its channels ("
                   + juce::JSON::toString (ceiling, true) + ")");

        {
            PluginCatalogue catalogue (file);
            catalogue.setFacts (limiter, facts);
            auto compressor = builtIn ("Compressor");
            catalogue.setFactsFailed (compressor, "it crashed while opening");
            catalogue.setUnderstanding (limiter, { "A brickwall limiter that holds peaks under a ceiling. Good for the last stage.", "test-model", limiter.version, juce::Time::getCurrentTime() });
            check (catalogue.summaryOf (limiter) == "A brickwall limiter that holds peaks under a ceiling." && ! catalogue.needsUnderstanding (limiter)
                       && catalogue.needsUnderstanding (compressor) && catalogue.getNumFacts() == 1 && catalogue.getNumUnderstood() == 1,
                   "plug-in catalogue: facts, a failure and an understanding are kept; the summary is the first sentence");
            check (catalogue.saveIfNeeded() && file.existsAsFile(), "plug-in catalogue: written to its file");
        }

        PluginCatalogue again (file);
        auto newer = limiter;
        newer.version = "99.0";
        const auto otherFormat = [&] { auto d = limiter; d.pluginFormatName = "VST3"; d.fileOrIdentifier = "/elsewhere/Limiter.vst3"; return d; }();
        check (again.hasCurrentFacts (limiter) && ! again.hasCurrentFacts (newer) && again.hasCurrentFacts (builtIn ("Compressor"))
                   && again.getFacts (builtIn ("Compressor"))["error"] == juce::var ("it crashed while opening")
                   && again.getUnderstanding (otherFormat).has_value() && ! again.hasCurrentFacts (otherFormat),
               "plug-in catalogue: read back from its file; facts are per format and version (a new version is read again), the understanding is shared by formats");

        again.forget (limiter);
        check (! again.getUnderstanding (limiter) && ! again.hasCurrentFacts (limiter) && again.hasCurrentFacts (builtIn ("Compressor")),
               "plug-in catalogue: forgetting a plug-in drops its facts and understanding, nothing else");

        // Facts read by an older catalogue (before presets were listed) are read again.
        {
            auto old = juce::JSON::parse (file);
            auto* entries = old["facts"].getDynamicObject();

            for (auto& p : entries->getProperties())
                p.value.getDynamicObject()->removeProperty ("factsVersion");

            file.replaceWithText (juce::JSON::toString (old));
        }

        check (! PluginCatalogue (file).hasCurrentFacts (builtIn ("Compressor")), "plug-in catalogue: facts from an older version of it are read again");

        // What Torqwave ships is used where this computer has written nothing of its own.
        const auto shipped = juce::JSON::parse (R"({"version": 1, "plugins": {"torqwave|limiter": {"name": "Limiter", "maker": "Torqwave",
            "about": "Shipped: holds peaks under a ceiling.", "written": "test-model", "reviewed": "2026-11-18T00:00:00Z"}}})");
        const juce::TemporaryFile other (".json");
        PluginCatalogue withShipped (other.getFile(), shipped);
        auto limiterHere = limiter;
        limiterHere.manufacturerName = "Torqwave";
        const auto fromShip = withShipped.getUnderstanding (limiterHere);
        const auto shippedUsed = fromShip && fromShip->shipped && fromShip->model == "test-model" && ! withShipped.needsUnderstanding (limiterHere)
                                 && withShipped.summaryOf (limiterHere) == "Shipped: holds peaks under a ceiling.";
        withShipped.setUnderstanding (limiterHere, { "Mine.", "you", {}, juce::Time::getCurrentTime() });
        const auto mineWins = withShipped.getUnderstanding (limiterHere)->text == "Mine." && ! withShipped.getUnderstanding (limiterHere)->shipped;
        withShipped.forget (limiterHere);
        const auto backToShipped = withShipped.getUnderstanding (limiterHere).has_value() && withShipped.getUnderstanding (limiterHere)->shipped
                                   && withShipped.getNumUnderstood() == 0 && withShipped.getNumShipped() == 1;
        check (shippedUsed && mineWins && backToShipped,
               "plug-in catalogue: a shipped description is used where none was written here; one written here wins, and forgetting it brings the shipped one back");
    }

    void testCatalogueWorker()
    {
        // Third-party plug-ins are read in the scanner's worker process.
        juce::String reason;
        juce::var facts;
        juce::String crashReason;
        juce::var crashed;
        const auto crashing = juce::File (TORQWAVE_TEST_BUNDLES).getChildFile ("CrashingBundle.vst3");

        std::thread ([&]
        {
            OutOfProcessScanner::Describer describer (TestUtils::appExecutable, 20000);
           #if JUCE_MAC
            juce::PluginDescription delay;
            delay.name = "AUDelay";
            delay.pluginFormatName = "AudioUnit";
            delay.fileOrIdentifier = "AudioUnit:Effects/aufx,dely,appl";
            facts = describer.describe (delay, reason, [] { return false; });
           #endif

            if (crashing.exists())
            {
                juce::PluginDescription bad;
                bad.name = "Crashing";
                bad.pluginFormatName = "VST3";
                bad.fileOrIdentifier = crashing.getFullPathName();
                crashed = describer.describe (bad, crashReason, [] { return false; });
            }
        }).join();

       #if JUCE_MAC
        juce::StringArray names;

        if (auto* list = facts["parameters"].getArray())
            for (auto& p : *list)
                names.add (p["name"].toString());

        check (facts.isObject() && names.contains ("Delay Time") && (int) facts["parameterCount"] == names.size(),
               "plug-in catalogue: Apple's AUDelay is read in the worker process (" + names.joinIntoString (", ") + reason + ")");
       #endif

        if (crashing.exists())
            check (crashed.isVoid() && crashReason.isNotEmpty(),
                   "plug-in catalogue: a plug-in that crashes as it opens takes only the worker with it (" + crashReason + ")");
    }

    void testCatalogueTools()
    {
        const juce::TemporaryFile temp (".json");
        PluginCatalogue catalogue (temp.getFile());
        juce::AudioPluginFormatManager formats;
        formats.addFormat (BuiltInEffects::createFormat());
        const auto limiter = builtIn ("Limiter");
        catalogue.setUnderstanding (limiter, { "Holds peaks under a ceiling, transparently. Use Ceiling for the most it lets through.", "test-model", {}, {} });

        Ai::Workspace::Start start;
        start.audio = tone (48000.0, 2);
        start.plugins = BuiltInEffects::getDescriptions();
        start.formats = &formats;
        start.catalogue = &catalogue;
        Ai::Workspace w (std::move (start));
        const auto call = [&w] (const juce::String& tool, const juce::String& args) { return parse (w.run (tool, args)); };

        const auto found = call ("list_plugins", R"({"search": "ceiling"})");
        check ((int) found["count"] == 1 && found["plugins"][0]["name"] == juce::var ("Limiter")
                   && found["plugins"][0]["summary"] == juce::var ("Holds peaks under a ceiling, transparently."),
               "plug-in catalogue: list_plugins shows each one's summary and searches it");

        const auto nothing = call ("describe_plugin", R"({"plugin": "Compressor"})");
        check (! (bool) nothing["ok"] && nothing["error"].toString().contains ("nothing"), "plug-in catalogue: describe_plugin says when it knows nothing");

        const auto added = call ("add_plugin", R"({"plugin": "Compressor"})");
        const auto compressor = builtIn ("Compressor");
        check ((bool) added["ok"] && catalogue.hasCurrentFacts (compressor) && (int) catalogue.getFacts (compressor)["parameterCount"] > 0,
               "plug-in catalogue: a plug-in Ask Torq opens has its facts kept");

        const auto steps = w.getSteps().size();
        const auto described = call ("describe_plugin", R"({"plugin": "Compressor"})");
        const auto about = call ("describe_plugin", R"({"plugin": "Limiter"})");
        check ((bool) described["ok"] && described["parameters"].size() > 0 && (bool) about["ok"] && about["about"].toString().startsWith ("Holds peaks")
                   && about.hasProperty ("note") && w.getSteps().size() == steps && w.createRackXml()->getNumChildElements() == 1,
               "plug-in catalogue: describe_plugin gives the entry and the parameters without opening the plug-in or changing the plan");
    }

    void testSessionComp()
    {
        // A 4 s clip at 1 s with three takes, the last in use.
        SessionState s;
        s.sampleRate = 48000.0;
        s.addTrack().name = "Vocal";
        const auto clipId = SessionEdits::recordTakes (s, 0, juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("vocal.wav"),
                                                       tone (48000.0, 12), 48000, 4 * 48000, { 0, 4 * 48000, 8 * 48000 });
        Ai::Workspace::Start start;
        start.session = s;
        start.sessionName = "Song";
        Ai::Workspace w (std::move (start));
        const auto call = [&w] (const juce::String& tool, const juce::String& args) { return parse (w.run (tool, args)); };
        const auto& clip = w.getSession()->tracks[0].clips[0];
        const auto id = juce::String ((int) clipId);

        const auto listed = call ("list_clips", R"({"track": "Vocal"})")["clips"][0];
        const auto part = call ("comp_takes", "{\"clip\": " + id + ", \"take\": 1, \"start\": 2, \"end\": 3}");
        const auto none = call ("comp_takes", "{\"clip\": " + id + ", \"take\": 4, \"start\": 2, \"end\": 3}");
        const auto outside = call ("comp_takes", "{\"clip\": " + id + ", \"take\": 1, \"start\": 8, \"end\": 9}");
        check ((int) listed["takes"] == 3 && listed["playing"] == juce::var ("take 3") && (bool) part["ok"] && ! (bool) none["ok"] && ! (bool) outside["ok"]
                   && clip.comp.size() == 3 && clip.takeAt (clip.takeZeroFrame (2 * 48000)) == 0 && clip.takeAt (clip.takeZeroFrame (3 * 48000)) == 2
                   && call ("list_clips", R"({"track": "Vocal"})")["clips"][0]["playing"] == juce::var ("a comp"),
               "Ask Torq sessions: comp_takes comps a take over a range; a take that isn't there or a range outside the clip refused");

        const auto whole = call ("comp_takes", "{\"clip\": " + id + ", \"take\": 2}");
        check ((bool) whole["ok"] && clip.comp.empty() && clip.activeTake == 1 && w.getSteps().size() == 2,
               "Ask Torq sessions: comp_takes with no range plays the take throughout; two steps");
    }

    void testSessionSections()
    {
        // Four bars of 2 s at 120 BPM: an Intro (bars 1-2) and a Chorus (bars 3-4) with a clip in it.
        SessionState s;
        s.sampleRate = 48000.0;
        s.tempo = 120.0;
        SessionEdits::addClip (s, 0, juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("hook.wav"), tone (48000.0, 4), 4 * 48000);
        s.tracks[0].name = "Hook";
        Ai::Workspace::Start start;
        start.session = s;
        start.sessionName = "Song";
        Ai::Workspace w (std::move (start));
        const auto call = [&w] (const juce::String& tool, const juce::String& args) { return parse (w.run (tool, args)); };
        const auto& state = *w.getSession();
        constexpr juce::int64 bar = 2 * 48000;

        const auto intro = call ("add_section", R"({"name": "Intro", "start": 0, "end": 4})");
        const auto chorus = call ("add_section", R"({"name": "Chorus", "start": 4, "end": 8})");
        const auto overlap = call ("add_section", R"({"name": "Bridge", "start": 6, "end": 10})");
        const auto session = call ("get_session", "{}");
        check ((bool) intro["ok"] && (bool) chorus["ok"] && ! (bool) overlap["ok"] && state.sections.size() == 2
                   && session["sections"][1]["name"] == juce::var ("Chorus") && juce::exactlyEqual ((double) session["sections"][1]["end"], 8.0),
               "Ask Torq sessions: add_section adds sections, refuses an overlap, and get_session lists them");

        // Double the chorus: a copy at its own end, carrying the clip.
        const auto doubled = call ("move_section", R"({"section": "chorus", "to": 8, "copy": true})");
        check ((bool) doubled["ok"] && state.sections.size() == 3 && state.sections[2].start == 4 * bar && state.sections[2].length == 2 * bar
                   && state.tracks[0].clips.size() == 2 && state.tracks[0].clips[1].position == 4 * bar,
               "Ask Torq sessions: move_section with copy doubles the chorus, its clip with it");

        // The intro two bars longer at its start, then one bar shorter at its end.
        const auto longer = call ("change_section_bars", R"({"section": "Intro", "bars": 2, "at": "start"})");
        check ((bool) longer["ok"] && state.sections[0].start == 0 && state.sections[0].length == 4 * bar && state.sections[1].start == 4 * bar
                   && state.tracks[0].clips[0].position == 4 * bar,
               "Ask Torq sessions: change_section_bars puts empty bars in at a section's start, and it takes them in");

        const auto shorter = call ("change_section_bars", R"({"section": "Intro", "bars": -1, "at": "end"})");
        const auto tooMany = call ("change_section_bars", R"({"section": "Intro", "bars": -3})");
        check ((bool) shorter["ok"] && ! (bool) tooMany["ok"] && state.sections[0].length == 3 * bar && state.sections[1].start == 3 * bar
                   && state.tracks[0].clips[0].position == 3 * bar,
               "Ask Torq sessions: change_section_bars takes bars out of a section's end and closes up; not every bar of it");

        const auto chain = call ("set_arrangement", R"({"sections": ["Intro", "Chorus", "Intro"], "play": true})");
        const auto unknown = call ("set_arrangement", R"({"sections": ["Outro"]})");
        check ((bool) chain["ok"] && ! (bool) unknown["ok"] && state.arrangement.size() == 3 && state.playArrangement
                   && call ("get_session", "{}")["arrangement"].toString().startsWith ("Intro, Chorus, Intro"),
               "Ask Torq sessions: set_arrangement sets the chain by name and plays it; an unknown name refused");

        const auto gone = call ("delete_section", R"({"section": "Intro"})");
        check ((bool) gone["ok"] && state.sections.size() == 2 && state.sections[0].start == 0 && state.tracks[0].clips[0].position == 0
                   && state.arrangement.size() == 1 && w.getSteps().size() == 7,
               "Ask Torq sessions: delete_section deletes it with its time; each edit one step");
    }

    void testSessionTools()
    {
        // A session of one audio track with a 4 s clip at 1 s, and a bus.
        SessionState s;
        s.sampleRate = 48000.0;
        const auto clipId = SessionEdits::addClip (s, 0, juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("drums.wav"), tone (48000.0, 4), 48000);
        s.tracks[0].name = "Drums";
        s.addBus().name = "Reverb";

        Ai::Workspace::Start start;
        start.session = s;
        start.sessionName = "Song";
        start.plugins = BuiltInEffects::getDescriptions();
        Ai::Workspace w (std::move (start));
        const auto call = [&w] (const juce::String& tool, const juce::String& args) { return parse (w.run (tool, args)); };
        const auto& state = *w.getSession();

        juce::StringArray names;

        const auto tools = w.tools();

        for (auto& tool : *tools.getArray())
            names.add (tool["function"]["name"].toString());

        check (w.isSession() && names.size() == 45 && names.contains ("find_sequence_presets") && names.contains ("add_sequence_preset") && names.contains ("add_kit") && names.contains ("list_drum_maps") && names.contains ("set_drum_map") && names.contains ("remap_clip_notes") && names.contains ("get_session") && names.contains ("set_clip_effects") && names.contains ("fit_tempo_to_hits") && names.contains ("store_scene") && names.contains ("recall_scene") && names.contains ("comp_takes") && names.contains ("move_section") && names.contains ("set_automation") && names.contains ("add_track_effect") && names.contains ("add_note_clip") && names.contains ("set_instrument") && names.contains ("list_instrument_presets") && names.contains ("list_plugins") && ! names.contains ("get_state")
                   && w.systemMessage().contains ("multitrack session"),
               "Ask Torq sessions: a session's workspace offers the session tools and the plug-in look tools, and says it is a session");

        const auto session = call ("get_session", "{}");
        check (session["name"] == juce::var ("Song") && session["tracks"].size() == 2 && session["tracks"][0]["kind"] == juce::var ("audio")
                   && (int) session["tracks"][0]["clips"] == 1 && juce::exactlyEqual ((double) session["tracks"][0]["from"], 1.0)
                   && session["tracks"][1]["kind"] == juce::var ("bus") && ! w.sessionChanged(),
               "Ask Torq sessions: get_session lists the tracks with their kinds and clips, changing nothing");

        const auto folder = call ("add_track", R"({"kind": "folder", "name": "Rhythm", "after": 1})");
        const auto inFolder = call ("set_folder", R"({"track": "drums", "folder": "Rhythm"})");
        check ((bool) folder["ok"] && (int) folder["number"] == 2 && state.tracks[1].isFolder && state.tracks[1].name == "Rhythm"
                   && (bool) inFolder["ok"] && state.tracks[0].folderId == state.tracks[1].id,
               "Ask Torq sessions: a folder added after a track, and a track named in any case put in it");

        const auto send = call ("set_send", R"({"track": 1, "bus": "Reverb", "level": -6})");
        const auto loop = call ("set_output", R"({"track": "Reverb", "bus": "Reverb"})");
        const auto mix = call ("set_track", R"({"track": "Drums", "volume": -3, "pan": -0.5})");
        const auto loud = call ("set_track", R"({"track": "Drums", "volume": 20})");
        const auto nobody = call ("set_track", R"({"track": "Bass", "mute": true})");
        check ((bool) send["ok"] && state.tracks[0].sends.size() == 1 && juce::exactlyEqual (state.tracks[0].sends[0].gainDb, -6.0f)
                   && ! (bool) loop["ok"] && (bool) mix["ok"] && juce::exactlyEqual (state.tracks[0].gainDb, -3.0f) && juce::exactlyEqual (state.tracks[0].pan, -0.5f)
                   && ! (bool) loud["ok"] && ! (bool) nobody["ok"] && nobody["error"].toString().contains ("Bass"),
               "Ask Torq sessions: sends, outputs and the mix set through the session's own edits; a bus into itself, a volume out of range and a track that isn't there refused");

        const auto clips = call ("list_clips", R"({"track": "Drums"})");
        const auto id = juce::String ((int) clipId);
        const auto moved = call ("move_clip", "{\"clip\": " + id + ", \"start\": 2}");
        const auto split = call ("split_clip", "{\"clip\": " + id + ", \"at\": 4}");
        const auto outside = call ("split_clip", "{\"clip\": " + id + ", \"at\": 30}");
        const auto set = call ("set_clip", "{\"clip\": " + id + ", \"gain\": -4, \"fade_in\": 0.1, \"fade_out\": 0.2}");
        const auto& drums = state.tracks[0];
        check (clips["clips"].size() == 1 && (int) clips["clips"][0]["id"] == (int) clipId && (bool) moved["ok"] && (bool) split["ok"] && ! (bool) outside["ok"]
                   && (bool) set["ok"] && drums.clips.size() == 2 && drums.clips[0].position == 96000 && drums.clips[0].getEnd() == 192000
                   && juce::exactlyEqual (drums.clips[0].gainDb, -4.0f) && drums.clips[0].fadeIn == 4800 && drums.clips[0].fadeOut == 9600,
               "Ask Torq sessions: a clip moved, split, given a gain and fades; a split outside it refused");

        const auto region = call ("add_marker", R"({"start": 2, "end": 6, "name": "Verse"})");
        const auto tempo = call ("set_tempo", R"({"bpm": 90, "beats_per_bar": 3})");
        const auto removed = call ("remove_track", R"({"track": "Rhythm"})");
        const auto fileTool = call ("get_state", "{}");
        check ((bool) region["ok"] && state.markers.size() == 1 && state.markers[0].name == "Verse" && state.markers[0].length == 4 * 48000
                   && (bool) tempo["ok"] && juce::exactlyEqual (state.tempo, 90.0) && state.beatsPerBar == 3
                   && (bool) removed["ok"] && state.tracks.size() == 2 && state.tracks[0].folderId == 0
                   && ! (bool) fileTool["ok"] && w.sessionChanged() && w.getSteps().size() == 10 && s.tracks.size() == 2 && s.tracks[0].clips.size() == 1,
               "Ask Torq sessions: markers, the tempo and removing a folder (its track left out of any); a file's tool refused; ten steps, and the session itself untouched");

        const auto ramp = call ("set_automation", R"({"track": "Drums", "parameter": "volume", "points": [{"time": 4, "value": -20}, {"time": 2, "value": 0}]})");
        const auto again = call ("set_automation", R"({"track": "Drums", "parameter": "volume", "points": [{"time": 2, "value": 0}, {"time": 4, "value": -20}]})");
        const auto wide = call ("set_automation", R"({"track": "Drums", "parameter": "pan", "points": [{"time": 1, "value": 2}]})");
        const auto listed = call ("get_session", "{}")["tracks"][0]["automation"].toString();
        const auto& volume = state.tracks[0].volumeEnvelope;
        const auto drawn = (bool) ramp["ok"] && volume.points.size() == 2 && volume.points[0].frame == 96000 && juce::exactlyEqual (volume.points[1].value, -20.0f)
                           && juce::approximatelyEqual (state.tracks[0].valueAt (AutomationParameter::volume, 144000), -10.0f);
        const auto cleared = (bool) call ("set_automation", R"({"track": "Drums", "parameter": "volume", "points": []})")["ok"] && volume.isEmpty();
        check (drawn && ! (bool) again["ok"] && ! (bool) wide["ok"] && listed.startsWith ("volume (2 points, 2.000 to 4.000 s)") && cleared,
               "Ask Torq sessions: volume automation drawn from points in any order, the same again and a pan out of range refused, listed by get_session, and removed");

        const auto added = call ("add_track_effect", R"({"track": "Reverb", "plugin": "Reverb"})");
        const auto synth = call ("add_track_effect", R"({"track": "Drums", "plugin": "Torqwave Synth"})");
        const auto unknown = call ("add_track_effect", R"({"track": "Drums", "plugin": "Nothing Like It"})");
        const auto loads = w.getChainChanges();
        check ((bool) added["ok"] && ! (bool) synth["ok"] && ! (bool) unknown["ok"] && loads.size() == 1 && loads[0].chain.size() == 1
                   && loads[0].chain[0].plugin.name == "Reverb" && loads[0].chain[0].slotId == 0 && loads[0].trackId == state.tracks[1].id
                   && call ("get_session", "{}")["tracks"][1]["effects"].toString() == "1. Reverb (when the plan is applied)",
               "Ask Torq sessions: an effect planned for a bus, shown by get_session; an instrument and a plug-in that isn't there refused");

        const auto shifted = call ("set_clip_effects", "{\"clip\": " + id + ", \"pitch\": -5, \"add\": \"Reverb\"}");
        const auto tooFar = call ("set_clip_effects", "{\"clip\": " + id + ", \"pitch\": 13}");
        const auto instrument = call ("set_clip_effects", "{\"clip\": " + id + ", \"add\": \"Torqwave Synth\"}");
        const auto nothing = call ("set_clip_effects", "{\"clip\": " + id + "}");
        const auto& fx = state.tracks[0].clips[0].effects;
        const auto shown = call ("list_clips", R"({"track": "Drums"})")["clips"][0]["effects"].toString();
        const auto withEffects = (bool) shifted["ok"] && juce::exactlyEqual (fx.semitones, -5.0) && ClipEffects::slotsOf (fx.chain).size() == 1
                                 && ClipEffects::slotsOf (fx.chain)[0].plugin.name == "Reverb" && shown == ClipEffects::describe (fx) && shown.contains ("Reverb");
        const auto clearedFx = (bool) call ("set_clip_effects", "{\"clip\": " + id + ", \"pitch\": 0, \"clear\": true}")["ok"] && ! state.tracks[0].clips[0].effects.isOn();
        check (withEffects && ! (bool) tooFar["ok"] && ! (bool) instrument["ok"] && ! (bool) nothing["ok"] && clearedFx && state.tracks[0].clips[1].effects.chain.isEmpty(),
               "Ask Torq sessions: set_clip_effects puts a pitch shift and an effect on one clip, shown by list_clips, and takes them off; a pitch past an octave, an instrument and no change refused");
    }

    void testSessionChains()
    {
        // Drums with a clip, a frozen Pad, and a bus whose chain is a compressor with two parameters.
        SessionState s;
        s.sampleRate = 48000.0;
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory);
        SessionEdits::addClip (s, 0, folder.getChildFile ("drums.wav"), tone (48000.0, 4), 48000);
        s.tracks[0].name = "Drums";
        SessionEdits::addClip (s, 1, folder.getChildFile ("pad.wav"), tone (48000.0, 2), 0);
        s.tracks[1].name = "Pad";
        SessionEdits::freezeTrack (s, s.tracks[1].id, folder.getChildFile ("pad frozen.wav"), tone (48000.0, 3));
        auto& bus = s.addBus();
        bus.name = "Verb Bus";
        const auto busId = bus.id;

        Ai::SessionTools::Effect compressor;
        compressor.slotId = 7;
        compressor.plugin.name = "Compressor";
        compressor.parameters.push_back ({ "thr", "Threshold", "-20 dB", "-60 dB", "0 dB", 0.5f, {} });
        compressor.parameters.push_back ({ "mode", "Mode", "Soft", "Soft", "Hard", 0.0f, juce::StringArray { "Soft", "Hard" } });

        Ai::Workspace::Start start;
        start.session = s;
        start.sessionName = "Song";
        start.plugins = BuiltInEffects::getDescriptions();
        start.effects[busId] = { compressor };
        Ai::Workspace w (std::move (start));
        const auto call = [&w] (const juce::String& tool, const juce::String& args) { return parse (w.run (tool, args)); };
        const auto& state = *w.getSession();

        const auto listed = call ("list_effect_parameters", R"({"track": "Verb Bus", "effect": 1})");
        const auto found = call ("list_effect_parameters", R"({"track": "Verb Bus", "effect": "compressor", "find": "mod"})");
        const auto none = call ("list_effect_parameters", R"({"track": "Drums", "effect": 1})");
        check (call ("get_session", "{}")["tracks"][2]["effects"] == juce::var ("1. Compressor") && listed["parameters"].size() == 2
                   && listed["parameters"][0]["reads"] == juce::var ("-20 dB") && listed["parameters"][0]["range"] == juce::var ("-60 dB to 0 dB")
                   && found["parameters"].size() == 1 && found["parameters"][0]["choices"] == juce::var ("Soft | Hard") && ! (bool) none["ok"],
               "Ask Torq chains: get_session numbers a bus's effects, list_effect_parameters reads one's parameters (narrowed by find); a track without effects refused");

        const auto set = call ("set_track_effect", R"({"track": "Verb Bus", "effect": 1, "bypass": true, "parameters": [{"parameter": "Threshold", "value": 0.25}, {"parameter": "mode", "text": "hard"}]})");
        const auto tooHigh = call ("set_track_effect", R"({"track": "Verb Bus", "effect": 1, "parameters": [{"parameter": "Threshold", "value": 1.5}]})");
        const auto badChoice = call ("set_track_effect", R"({"track": "Verb Bus", "effect": 1, "parameters": [{"parameter": "Mode", "text": "Medium"}]})");
        const auto noParameter = call ("set_track_effect", R"({"track": "Verb Bus", "effect": 1, "parameters": [{"parameter": "Ratio", "value": 0.5}]})");
        const auto planned = w.getChainChanges();
        const auto settings = planned.size() == 1 ? planned[0].chain[0].settings : std::vector<Ai::SessionTools::Setting>();
        check ((bool) set["ok"] && ! (bool) tooHigh["ok"] && ! (bool) badChoice["ok"] && badChoice["error"].toString().contains ("Soft, Hard") && ! (bool) noParameter["ok"]
                   && planned.size() == 1 && planned[0].trackId == busId && planned[0].chain.size() == 1 && planned[0].chain[0].slotId == 7 && planned[0].chain[0].bypassed
                   && settings.size() == 2 && settings[0].parameterId == "thr" && juce::exactlyEqual (settings[0].value.value_or (-1.0f), 0.25f)
                   && settings[1].parameterId == "mode" && juce::exactlyEqual (settings[1].value.value_or (-1.0f), 1.0f),
               "Ask Torq chains: an effect switched off and set by value and by a stepped parameter's choice; a value past 1, a choice it hasn't and a parameter it hasn't refused, leaving the plan as it was");

        const auto added = call ("add_track_effect", R"({"track": "Verb Bus", "plugin": "Reverb", "position": 1})");
        const auto shown = call ("get_session", "{}")["tracks"][2]["effects"].toString();
        const auto setNew = call ("set_track_effect", R"({"track": "Verb Bus", "effect": "Reverb", "parameters": [{"parameter": "Mix", "value": 0.5}]})");
        const auto automateNew = call ("set_automation", R"({"track": "Verb Bus", "effect": "Reverb", "parameter": "Mix", "points": [{"time": 0, "value": 0.5}]})");
        const auto moved = call ("move_track_effect", R"({"track": "Verb Bus", "effect": "Compressor", "to": 1})");
        const auto movedTooFar = call ("move_track_effect", R"({"track": "Verb Bus", "effect": "Compressor", "to": 3})");
        const auto order = w.getChainChanges()[0].chain;
        check ((bool) added["ok"] && shown == "1. Reverb (when the plan is applied), 2. Compressor (off)" && ! (bool) setNew["ok"] && ! (bool) automateNew["ok"]
                   && (bool) moved["ok"] && ! (bool) movedTooFar["ok"] && order.size() == 2 && order[0].slotId == 7 && order[1].plugin.name == "Reverb",
               "Ask Torq chains: an effect added at a place in the chain, listed as planned; its settings and automation wait for a later request; an effect moved, and a place past the end refused");

        const auto automated = call ("set_automation", R"({"track": "Verb Bus", "effect": "Compressor", "parameter": "Threshold", "points": [{"time": 1, "value": 0.2}, {"time": 3, "value": 0.8}]})");
        const auto outOfRange = call ("set_automation", R"({"track": "Verb Bus", "effect": 1, "parameter": "thr", "points": [{"time": 1, "value": 2}]})");
        const auto& envelopes = state.tracks[2].effectEnvelopes;
        const auto drawn = (bool) automated["ok"] && ! (bool) outOfRange["ok"] && envelopes.size() == 1 && envelopes[0].slotId == 7 && envelopes[0].parameterId == "thr"
                           && envelopes[0].envelope.points.size() == 2 && call ("get_session", "{}")["tracks"][2]["automation"] == juce::var ("Compressor's \"Threshold\" (2 points)");
        const auto removed = call ("remove_track_effect", R"({"track": "Verb Bus", "effect": 1})");
        const auto after = w.getChainChanges()[0].chain;
        check (drawn && (bool) removed["ok"] && envelopes.empty() && after.size() == 1 && after[0].slotId == 0 && after[0].plugin.name == "Reverb",
               "Ask Torq chains: an effect's parameter automated from 0 to 1 and listed (a value past 1 refused); removing the effect takes its automation with it");

        const auto frozenFx = call ("add_track_effect", R"({"track": "Pad", "plugin": "Reverb"})");
        const auto freeze = call ("freeze_track", R"({"track": "Drums"})");
        const auto twice = call ("freeze_track", R"({"track": "Drums", "freeze": true})");
        const auto busFreeze = call ("freeze_track", R"({"track": "Verb Bus"})");
        const auto cancel = call ("freeze_track", R"({"track": "Drums", "freeze": false})");
        const auto unfreeze = call ("freeze_track", R"({"track": "Pad", "freeze": false})");
        const auto bounce = call ("bounce_track", R"({"track": "Drums", "start": 1, "end": 3})");
        const auto backwards = call ("bounce_track", R"({"track": "Drums", "start": 3, "end": 1})");
        const auto padFreeze = call ("freeze_track", R"({"track": "Pad"})");
        const auto renders = w.getRenders();
        check (! (bool) frozenFx["ok"] && (bool) freeze["ok"] && ! (bool) twice["ok"] && ! (bool) busFreeze["ok"] && (bool) cancel["ok"]
                   && (bool) unfreeze["ok"] && ! state.tracks[1].isFrozen() && (bool) bounce["ok"] && ! (bool) backwards["ok"] && (bool) padFreeze["ok"]
                   && renders.size() == 2 && ! renders[0].freeze && renders[0].trackId == state.tracks[0].id && renders[0].range == juce::Range<juce::int64> (48000, 144000)
                   && renders[1].freeze && renders[1].trackId == state.tracks[1].id && ! w.getSteps().contains ("Freeze \"Drums\" (rendered once the plan is applied)"),
               "Ask Torq chains: a frozen track's effects refused; a freeze planned, taken back out and its step with it; a frozen track unfrozen in the plan, then frozen again; a bounce over a range; a bus and a backwards range refused");
    }

    void testChainOnEngine()
    {
        // A bus with a compressor and a reverb on the engine, read, planned and made.
        AudioEngine engine;
        SessionState s;
        s.sampleRate = 48000.0;
        auto& bus = s.addBus();
        bus.name = "Verb Bus";
        const auto busId = bus.id;
        engine.session.reset (s, {});

        for (auto& name : { "Compressor", "Reverb" })
        {
            juce::PluginDescription d;

            for (auto& known : BuiltInEffects::getDescriptions())
                if (known.name == name)
                    d = known;

            auto loaded = false;
            engine.addTrackPlugin (busId, d, [&] (const juce::String&) { loaded = true; });
            TestUtils::runUntil ([&] { return loaded; }, 5000);
        }

        const auto chain = Ai::readChain (engine.getTrackEffects(), busId);
        const Ai::SessionTools::Parameter* threshold = nullptr;

        if (! chain.empty())
            for (auto& p : chain[0].parameters)
                if (p.name.containsIgnoreCase ("threshold"))
                    threshold = &p;

        check (chain.size() == 2 && chain[0].plugin.name == "Compressor" && chain[1].plugin.name == "Reverb" && ! chain[0].instrument
                   && chain[0].slotId == engine.getTrackEffects().getSlotId (busId, 0) && threshold != nullptr
                   && threshold->text.isNotEmpty() && threshold->lowest != threshold->highest,
               "Ask Torq chains: a track's chain read from the engine, each plug-in by its slot with its parameters and how they read");

        if (chain.size() != 2 || threshold == nullptr)
            return;

        Ai::Workspace::Start start;
        start.session = s;
        start.sessionName = "Song";
        start.plugins = BuiltInEffects::getDescriptions();
        start.effects[busId] = chain;
        Ai::Workspace w (std::move (start));
        const auto call = [&w] (const juce::String& tool, const juce::String& args) { return parse (w.run (tool, args)); };
        const auto calls = (bool) call ("add_track_effect", R"({"track": "Verb Bus", "plugin": "Limiter", "position": 1})")["ok"]
                           && (bool) call ("remove_track_effect", R"({"track": "Verb Bus", "effect": "Reverb"})")["ok"]
                           && (bool) call ("set_track_effect", "{\"track\": \"Verb Bus\", \"effect\": \"Compressor\", \"bypass\": true, \"parameters\": [{\"parameter\": \""
                                                                   + threshold->id + "\", \"text\": \"-24 dB\"}]}")["ok"];
        const auto changes = w.getChainChanges();
        auto made = false;
        juce::String errors;

        if (changes.size() == 1)
            Ai::applyChainChange (engine, changes[0], [&] (const juce::String& e) { errors = e; made = true; });

        TestUtils::runUntil ([&] { return made; }, 5000);
        const auto& fx = engine.getTrackEffects();
        auto* compressor = fx.getNumSlots (busId) == 2 ? fx.getPlugin (busId, 1) : nullptr;
        auto* set = compressor != nullptr ? TrackEffects::parametersById (*compressor)[threshold->id] : nullptr;
        check (calls && made && errors.isEmpty() && fx.getNumSlots (busId) == 2 && fx.getPlugin (busId, 0)->getName() == "Limiter"
                   && compressor != nullptr && fx.getSlotId (busId, 1) == chain[0].slotId && fx.isBypassed (busId, 1) && ! fx.isBypassed (busId, 0)
                   && set != nullptr && set->getCurrentValueAsText().contains ("-24"),
               "Ask Torq chains: the plan made on the engine's chain: an effect added first, one taken off, one switched off and set from text");
        engine.clearTrackEffects();

        // An instrument track: Torqwave Synth and a compressor; a reverb put first among the effects
        // and a parameter of the synth set, the synth staying first in the chain.
        SessionState withSynth;
        withSynth.sampleRate = 48000.0;
        auto& keys = withSynth.addTrack();
        keys.name = "Keys";
        keys.isInstrument = true;
        const auto keysId = keys.id;
        engine.session.reset (withSynth, {});

        for (auto& name : { "Torqwave Synth", "Compressor" })
        {
            juce::PluginDescription d;

            for (auto& known : BuiltInEffects::getDescriptions())
                if (known.name == name)
                    d = known;

            auto loaded = false;
            engine.addTrackPlugin (keysId, d, [&] (const juce::String&) { loaded = true; });
            TestUtils::runUntil ([&] { return loaded; }, 5000);
        }

        const auto synthChain = Ai::readChain (engine.getTrackEffects(), keysId);
        const auto synthReady = synthChain.size() == 2 && synthChain[0].instrument && ! synthChain[0].parameters.empty();
        Ai::Workspace::Start keysStart;
        keysStart.session = withSynth;
        keysStart.plugins = BuiltInEffects::getDescriptions();
        keysStart.effects[keysId] = synthChain;
        Ai::Workspace kw (std::move (keysStart));
        juce::String first;   // a parameter that isn't stepped, so it takes the value as it is

        for (auto& p : synthReady ? synthChain[0].parameters : std::vector<Ai::SessionTools::Parameter>())
            if (p.choices.isEmpty() && first.isEmpty())
                first = p.id;

        const auto planned = synthReady && (bool) parse (kw.run ("add_track_effect", R"({"track": "Keys", "plugin": "Reverb", "position": 1})"))["ok"]
                             && (bool) parse (kw.run ("set_track_effect", "{\"track\": \"Keys\", \"effect\": \"instrument\", \"parameters\": [{\"parameter\": \"" + first + "\", \"value\": 0.75}]}"))["ok"]
                             && ! (bool) parse (kw.run ("remove_track_effect", R"({"track": "Keys", "effect": "instrument"})"))["ok"]
                             && parse (kw.run ("get_session", "{}"))["tracks"][0]["effects"] == juce::var ("1. Reverb (when the plan is applied), 2. Compressor");
        made = false;

        if (const auto keyChanges = kw.getChainChanges(); keyChanges.size() == 1)
            Ai::applyChainChange (engine, keyChanges[0], [&] (const juce::String& e) { errors = e; made = true; });

        TestUtils::runUntil ([&] { return made; }, 5000);
        auto* synth = fx.getInstrument (keysId);
        check (planned && made && errors.isEmpty() && fx.getNumSlots (keysId) == 3 && synth != nullptr && fx.getPlugin (keysId, 0) == synth
                   && fx.getPlugin (keysId, 1)->getName() == "Reverb" && fx.getPlugin (keysId, 2)->getName() == "Compressor"
                   && juce::approximatelyEqual (TrackEffects::parametersById (*synth)[first]->getValue(), 0.75f),
               "Ask Torq chains: on an instrument track an effect goes first among the effects, after the instrument, whose parameter is set; the instrument can't be taken off");
        engine.clearTrackEffects();
    }

    void testInstrumentPresets()
    {
        const auto folder = juce::File::createTempFile ("instrument-presets");
        InstrumentPresets::Store store (folder);
        juce::PluginDescription vst3, au;
        vst3.name = au.name = "Battery 4";
        vst3.manufacturerName = au.manufacturerName = "Native Instruments";
        vst3.pluginFormatName = "VST3";
        au.pluginFormatName = "AudioUnit";
        const juce::MemoryBlock kitA ("kit a \0 with a zero", 20), kitB ("kit b", 5);

        const auto saved = store.save (vst3, "Deep House: Kit", kitA) && store.save (vst3, "Acoustic", kitB) && ! store.save (vst3, "  ", kitA);
        const auto names = store.getNames (vst3);
        const auto loaded = store.load (vst3, "deep house: kit");
        const auto replaced = store.save (vst3, "ACOUSTIC", kitA) && store.getNames (vst3).size() == 2 && store.load (vst3, "Acoustic") == kitA;
        const auto otherFormat = store.getNames (au).isEmpty() && ! store.load (au, "Acoustic");
        const auto removed = store.remove (vst3, "Acoustic") && store.getNames (vst3) == juce::StringArray { "Deep House: Kit" } && ! store.remove (vst3, "Acoustic");
        folder.deleteRecursively();

        check (saved && names == juce::StringArray { "Acoustic", "Deep House: Kit" } && loaded == kitA && replaced && otherFormat && removed,
               "instrument presets: a plug-in's state saved and read back byte for byte by name (any case, any characters), a name replaced, kept apart by format, removed; an empty name refused");
    }

    void testSessionNotes()
    {
        // An audio track, and an instrument track already playing a piano.
        SessionState s;
        s.sampleRate = 48000.0;
        s.addTrack().name = "Guitar";
        s.addInstrumentTrack().name = "Keys";
        const auto keysId = s.tracks[1].id;

        juce::PluginDescription drumKit;
        drumKit.name = "Test Drums";
        drumKit.manufacturerName = "Torqwave";
        drumKit.pluginFormatName = "VST3";
        drumKit.fileOrIdentifier = "/Library/Audio/Plug-Ins/VST3/Test Drums.vst3";
        drumKit.isInstrument = true;

        Ai::Workspace::Start start;
        start.session = s;
        start.sessionName = "Song";
        start.plugins = BuiltInEffects::getDescriptions();
        start.plugins.add (drumKit);
        juce::PluginDescription piano;
        piano.name = "Test Piano";
        piano.pluginFormatName = "VST3";
        piano.fileOrIdentifier = "/Library/Audio/Plug-Ins/VST3/Test Piano.vst3";
        piano.isInstrument = true;
        start.plugins.add (piano);
        start.instruments[keysId] = piano;
        start.instrumentPresets = [] (const juce::PluginDescription& d)
        {
            return d.name == "Test Drums" ? juce::StringArray { "Deep House Kit", "Vintage Rock Kit" }
                 : d.name == "Test Piano" ? juce::StringArray { "Bright Grand" } : juce::StringArray();
        };
        Ai::Workspace w (std::move (start));
        const auto call = [&w] (const juce::String& tool, const juce::String& args) { return parse (w.run (tool, args)); };
        const auto& state = *w.getSession();

        const auto added = call ("add_track", R"({"kind": "instrument", "name": "Drums"})");
        const auto loaded = call ("set_instrument", R"({"track": "Drums", "plugin": "Test Drums"})");
        const auto onAudio = call ("set_instrument", R"({"track": "Guitar", "plugin": "Test Drums"})");
        const auto effect = call ("set_instrument", R"({"track": "Drums", "plugin": "Compressor"})");
        const auto taken = call ("set_instrument", R"({"track": "Keys", "plugin": "Test Drums"})");
        const auto session = call ("get_session", "{}");
        const auto loads = w.getInstrumentLoads();
        const auto drumsId = state.tracks.back().id;
        check ((bool) added["ok"] && (bool) loaded["ok"] && ! (bool) onAudio["ok"] && ! (bool) effect["ok"] && effect["error"].toString().contains ("effect")
                   && ! (bool) taken["ok"] && taken["error"].toString().contains ("Test Piano")
                   && session["tracks"][1]["instrument"] == juce::var ("Test Piano") && session["tracks"][2]["instrument"].toString().startsWith ("Test Drums")
                   && loads.size() == 1 && loads[0].trackId == drumsId && loads[0].plugin.name == "Test Drums",
               "Ask Torq sessions: an instrument chosen for a new instrument track, loaded after the plan; an audio track, an effect and a track that has one refused");

        const auto pattern = call ("add_note_clip", R"({"track": "Drums", "start": 0, "length": 4, "loop_length": 2, "name": "Kick",
            "notes": [ { "pitch": 36, "start": 0, "length": 0.1, "velocity": 110 }, { "pitch": 36, "start": 0.5, "length": 0.1 },
                       { "pitch": 36, "start": 1, "length": 0.1 }, { "pitch": 36, "start": 1.5, "length": 0.1 } ]})");
        const auto pastLoop = call ("add_note_clip", R"({"track": "Drums", "start": 4, "length": 4, "loop_length": 2, "notes": [ { "pitch": 36, "start": 2, "length": 0.1 } ]})");
        const auto badPitch = call ("add_note_clip", R"({"track": "Drums", "start": 4, "length": 4, "notes": [ { "pitch": 128, "start": 0, "length": 0.1 } ]})");
        const auto audioTrack = call ("add_note_clip", R"({"track": "Guitar", "start": 0, "length": 4, "notes": [ { "pitch": 60, "start": 0, "length": 1 } ]})");
        const auto listed = call ("list_clips", R"({"track": "Drums"})");
        const auto& drums = state.tracks.back();
        const auto& clip = drums.noteClips.empty() ? NoteClip() : drums.noteClips[0];
        check ((bool) pattern["ok"] && ! (bool) pastLoop["ok"] && ! (bool) badPitch["ok"] && ! (bool) audioTrack["ok"]
                   && drums.noteClips.size() == 1 && clip.name == "Kick" && clip.position == 0 && clip.length == 192000 && clip.loopLength == 96000
                   && clip.notes.size() == 4 && clip.notes[1].start == 24000 && clip.notes[1].length == 4800 && clip.notes[0].pitch == 36
                   && juce::approximatelyEqual (clip.notes[0].velocity, 110.0f / 127.0f) && juce::approximatelyEqual (clip.notes[1].velocity, 100.0f / 127.0f)
                   && (int) listed["clips"][0]["notes"] == 4 && juce::approximatelyEqual ((double) listed["clips"][0]["loop_length"], 2.0),
               "Ask Torq sessions: a looping note clip of a kick pattern on the new track; a note past the loop, a pitch over 127 and an audio track refused");

        const auto kits = call ("list_instrument_presets", R"({"track": "Drums"})");
        const auto rock = call ("set_instrument", R"({"track": "Drums", "preset": "vintage rock kit"})");
        const auto missing = call ("set_instrument", R"({"track": "Drums", "preset": "Trap Kit"})");
        const auto grand = call ("set_instrument", R"({"track": "Keys", "preset": "Bright Grand"})");
        const auto noPreset = call ("set_instrument", R"({"track": "Keys", "plugin": "Test Piano"})");
        const auto withPreset = w.getInstrumentLoads();
        check ((bool) kits["ok"] && kits["presets"].size() == 2 && (bool) rock["ok"] && ! (bool) missing["ok"] && missing["error"].toString().contains ("Deep House Kit")
                   && (bool) grand["ok"] && ! (bool) noPreset["ok"] && withPreset.size() == 2
                   && withPreset[0].trackId == drumsId && withPreset[0].preset == "Vintage Rock Kit" && ! withPreset[0].onlyPreset
                   && withPreset[1].trackId == keysId && withPreset[1].preset == "Bright Grand" && withPreset[1].onlyPreset && withPreset[1].plugin.name == "Test Piano",
               "Ask Torq sessions: instrument presets listed and chosen (in any case), for a new instrument and for one a track already plays; a preset not saved refused, naming those there are");

        const auto removed = call ("remove_track", R"({"track": "Drums"})");
        check ((bool) removed["ok"] && w.getInstrumentLoads().size() == 1 && w.getSteps().size() == 5
                   && ! w.getSteps().contains ("Load Test Drums on \"Drums\"") && s.tracks.size() == 2 && s.tracks[1].noteClips.empty(),
               "Ask Torq sessions: removing the track drops its instrument load; a replaced choice leaves the plan; five steps, and the session itself untouched");
    }

    //==============================================================================
    // Step 19: factory sounds, sequences, kits and drum maps.

    /** A session as its file's text, to compare two states whole. */
    juce::String sessionText (const SessionState& s)
    {
        const auto file = juce::File::createTempFile ("tqsession");
        SessionFile::save (s, file);
        const auto text = file.loadFileAsString();
        file.deleteFile();
        return text;
    }

    /** The same session: the same file text, and the same tracks, folders and note clips by id. */
    bool sameSession (const SessionState& a, const SessionState& b)
    {
        if (a.tracks.size() != b.tracks.size() || sessionText (a) != sessionText (b))
            return false;

        for (size_t i = 0; i < a.tracks.size(); ++i)
            if (a.tracks[i].id != b.tracks[i].id || a.tracks[i].name != b.tracks[i].name || a.tracks[i].folderId != b.tracks[i].folderId
                || ! (a.tracks[i].noteClips == b.tracks[i].noteClips) || ! (a.tracks[i].drumMap == b.tracks[i].drumMap))
                return false;

        return true;
    }

    bool sameXml (const std::shared_ptr<juce::XmlElement>& a, const std::shared_ptr<juce::XmlElement>& b)
    {
        return (a == nullptr && b == nullptr) || (a != nullptr && b != nullptr && a->isEquivalentTo (b.get(), false));
    }

    void testFactorySounds()
    {
        // The closeness the refusals name the nearest by: shared words first, then fewest letters away, then the order given.
        const juce::StringArray names { "Sub Sine", "Reese Growl", "Acid Line", "Dark Reese", "Dark Reeds" };
        check (Ai::nearestNames (names, "dark reese bass", 3) == juce::StringArray { "Dark Reese", "Dark Reeds", "Reese Growl" }
                   && Ai::nearestNames (names, "sub sin", 1) == juce::StringArray { "Sub Sine" }
                   && Ai::nearestNames (names, "zzz", 2) == Ai::nearestNames (names, "zzz", 2) && Ai::nearestNames (names, "zzz", 9).size() == 5
                   && Ai::hasWords ("Dark Reese Bass", "bass dark") && ! Ai::hasWords ("Dark Reese", "dark lead") && Ai::hasWords ("anything", "  "),
               "Ask Torq factory sounds: the nearest names share the most words, then are the fewest letters away, the same every time; a search needs every word");

        const auto synth = builtIn ("Torqwave Synth"), drums = builtIn ("Basic Drums"), guitar = builtIn ("StringSwarm");
        const auto& patches = WaveSynth::factoryPatches();
        const auto& kits = DrumMachine::kits();
        const auto& guitarPresets = EffectPresets::factory (BuiltInEffects::swarmGuitarId);
        juce::PluginDescription other;
        other.name = "Test Piano";
        other.pluginFormatName = "VST3";
        other.isInstrument = true;
        check (synth.isInstrument && drums.isInstrument && guitar.isInstrument && patches.size() > 1000 && Ai::FactorySounds::of (synth).size() == patches.size()
                   && Ai::FactorySounds::of (drums).size() == kits.size() && Ai::FactorySounds::of (guitar).size() == guitarPresets.size() && ! guitarPresets.empty()
                   && Ai::FactorySounds::of (other).empty() && Ai::FactorySounds::of (builtIn ("Compressor")).empty(),
               "Ask Torq factory sounds: Torqwave Synth's " + juce::String ((int) patches.size()) + " patches, Basic Drums' " + juce::String ((int) kits.size())
                   + " kits and StringSwarm's " + juce::String ((int) guitarPresets.size()) + " presets are known; an effect and another maker's instrument have none");

        if (patches.size() < 100 || kits.size() < 2 || guitarPresets.empty())
            return;

        // Three instrument tracks without an instrument, and one that already plays the synth.
        SessionState s;
        s.sampleRate = 48000.0;
        s.addInstrumentTrack().name = "Bass";
        s.addInstrumentTrack().name = "Drums";
        s.addInstrumentTrack().name = "Guitar";
        s.addInstrumentTrack().name = "Lead";
        const auto leadId = s.tracks[3].id;
        const auto savedName = patches[5].name;   // the user has saved a preset under a factory patch's name

        Ai::Workspace::Start start;
        start.session = s;
        start.sessionName = "Song";
        start.plugins = BuiltInEffects::getDescriptions();
        start.instruments[leadId] = synth;
        start.instrumentPresets = [savedName] (const juce::PluginDescription& d) { return d.name == "Torqwave Synth" ? juce::StringArray { savedName } : juce::StringArray(); };
        start.library.factorySounds = [] (const juce::PluginDescription& d) { return Ai::FactorySounds::of (d); };
        Ai::Workspace w (std::move (start));
        const auto call = [&w] (const juce::String& tool, const juce::String& args) { return parse (w.run (tool, args)); };
        const auto quoted = [] (const juce::String& text) { return juce::JSON::toString (juce::var (text)); };

        // Everything: capped at 60, with how many more and the categories.
        const auto all = call ("list_instrument_presets", R"({"plugin": "Torqwave Synth"})");
        check ((bool) all["ok"] && all["factory"].size() == 60 && (int) all["factory_total"] == (int) patches.size() && (int) all["factory_matching"] == (int) patches.size()
                   && (int) all["more"] == (int) patches.size() - 60 && all["categories"].size() == WaveSynth::patchCategories().size()
                   && all["factory"][0]["name"] == juce::var (patches[0].name) && all["factory"][0]["category"] == juce::var (patches[0].category)
                   && all["presets"].size() == 1 && all["note"].toString().contains ("narrow with category or search"),
               "Ask Torq factory sounds: list_instrument_presets lists Torqwave Synth's patches 60 at a time, says how many more there are (" + all["more"].toString()
                   + ") and names its " + juce::String (all["categories"].size()) + " categories, beside the presets the user saved");

        // A category and a search word: only the matches, and how many more.
        // The largest category, and the first word of a name in it that some but not all of its patches have.
        juce::String category, word;
        int inCategory = 0, expected = 0;

        for (auto& name : WaveSynth::patchCategories())
            if (const auto count = (int) std::count_if (patches.begin(), patches.end(), [&name] (auto& patch) { return patch.category == name; }); count > inCategory)
            {
                category = name;
                inCategory = count;
            }

        const auto having = [&] (const juce::String& text)
        {
            return (int) std::count_if (patches.begin(), patches.end(), [&] (auto& patch) { return patch.category == category && (patch.name + " " + patch.category).containsIgnoreCase (text); });
        };

        for (auto& patch : patches)
            if (const auto first = patch.name.upToFirstOccurrenceOf (" ", false, false); word.isEmpty() && patch.category == category && first.length() >= 3
                                                                                           && having (first) >= 2 && having (first) < inCategory)
                word = first;

        expected = having (word);

        const auto narrowed = call ("list_instrument_presets", "{\"plugin\": \"Torqwave Synth\", \"category\": " + quoted (category.toLowerCase()) + ", \"search\": " + quoted (word.toUpperCase()) + "}");
        const auto byCategory = call ("list_instrument_presets", "{\"track\": \"Lead\", \"category\": " + quoted (category) + "}");
        auto onlyMatches = narrowed["factory"].size() == juce::jmin (60, expected) && word.isNotEmpty() && expected >= 2 && expected < inCategory && inCategory > 60;

        if (auto* list = narrowed["factory"].getArray())
            for (auto& sound : *list)
                onlyMatches = onlyMatches && sound["category"] == juce::var (category) && (sound["name"].toString() + " " + category).containsIgnoreCase (word);

        const auto noCategory = call ("list_instrument_presets", R"({"plugin": "Torqwave Synth", "category": "Kazoos"})");
        const auto noMatch = call ("list_instrument_presets", R"({"plugin": "Torqwave Synth", "search": "zzqx"})");
        check (onlyMatches && (int) narrowed["factory_matching"] == expected && (expected > 60 ? (int) narrowed["more"] == expected - 60 : narrowed["more"].isVoid())
                   && (int) byCategory["factory_matching"] == inCategory && (inCategory > 60 ? (int) byCategory["more"] == inCategory - 60 : byCategory["more"].isVoid())
                   && ! (bool) noCategory["ok"] && noCategory["error"].toString().contains (category)
                   && (bool) noMatch["ok"] && noMatch["factory"].size() == 0 && (int) noMatch["factory_matching"] == 0 && w.getSteps().isEmpty(),
               "Ask Torq factory sounds: a category and a search word (in any case) list only the patches matching both (" + juce::String (expected) + " of " + juce::String (inCategory)
                   + " in " + category + " have " + word.quoted() + "), and a category alone says how many more there are past 60 (" + byCategory["more"].toString()
                   + "); a category it hasn't is refused naming them; listing plans nothing");

        // A patch by name on a new instrument: a factory load, by the patch's own name.
        const auto patch = patches[patches.size() / 2].name;
        const auto loaded = call ("set_instrument", "{\"track\": \"Bass\", \"plugin\": \"Torqwave Synth\", \"preset\": " + quoted (patch.toLowerCase()) + "}");
        const auto loads = w.getInstrumentLoads();
        check ((bool) loaded["ok"] && loaded["step"] == juce::var ("Load Torqwave Synth with the factory sound " + patch.quoted() + " on \"Bass\"")
                   && loads.size() == 1 && loads[0].factory && loads[0].preset == patch && ! loads[0].onlyPreset && loads[0].plugin.name == "Torqwave Synth"
                   && w.getSteps().size() == 1 && call ("get_session", "{}")["tracks"][0]["instrument"].toString().contains (patch),
               "Ask Torq factory sounds: set_instrument takes a factory patch by its name in any case: one plan step, a factory load of " + patch.quoted());

        // A word that matches nothing: refused with the nearest names, the plan as it was.
        juce::StringArray everyName { savedName };

        for (auto& factoryPatch : patches)
            everyName.add (factoryPatch.name);

        const auto wanted = patches[7].name + " zzqx";
        const auto nearest = Ai::nearestNames (everyName, wanted);
        const auto refusedPatch = call ("set_instrument", "{\"track\": \"Drums\", \"plugin\": \"Torqwave Synth\", \"preset\": " + quoted (wanted) + "}");
        check (! (bool) refusedPatch["ok"] && nearest.size() == 5 && nearest.contains (patches[7].name)
                   && refusedPatch["error"].toString().contains ("the nearest are " + nearest.joinIntoString (", ")) && w.getSteps().size() == 1 && w.getInstrumentLoads().size() == 1
                   && call ("get_session", "{}")["tracks"][1]["instrument"] == juce::var ("none"),
               "Ask Torq factory sounds: a patch asked for by words that match none is refused with the five nearest names (" + nearest.joinIntoString (", ")
                   + "), and the plan and the track are left as they were");

        // A kit and a StringSwarm preset; on a track that plays the synth already, a patch alone; the user's own preset before a factory one of its name.
        const auto kit = kits.back().name;
        const auto preset = guitarPresets.front();
        const auto kitsListed = call ("list_instrument_presets", R"({"plugin": "Basic Drums"})");
        const auto kitLoaded = call ("set_instrument", "{\"track\": \"Drums\", \"plugin\": \"Basic Drums\", \"preset\": " + quoted (kit) + "}");
        const auto guitarListed = call ("list_instrument_presets", "{\"plugin\": \"StringSwarm\", \"category\": " + quoted (preset.category) + "}");
        const auto guitarLoaded = call ("set_instrument", "{\"track\": \"Guitar\", \"plugin\": \"StringSwarm\", \"preset\": " + quoted (preset.name) + "}");
        const auto changed = call ("set_instrument", "{\"track\": \"Lead\", \"preset\": " + quoted (patches[9].name) + "}");
        const auto after = w.getInstrumentLoads();
        check ((bool) kitsListed["ok"] && kitsListed["factory"].size() == (int) kits.size() && kitsListed["categories"].isVoid() && kitsListed["factory"][0]["about"].toString().isNotEmpty()
                   && (bool) kitLoaded["ok"] && (bool) guitarListed["ok"] && guitarListed["factory"].size() > 0 && guitarListed["factory"][0]["category"] == juce::var (preset.category)
                   && (bool) guitarLoaded["ok"] && (bool) changed["ok"] && changed["step"] == juce::var ("Load the factory sound " + patches[9].name.quoted() + " in Torqwave Synth on \"Lead\"")
                   && after.size() == 4 && after[1].factory && after[1].preset == kit && after[2].factory && after[2].preset == preset.name
                   && after[3].factory && after[3].onlyPreset && after[3].trackId == leadId && w.getSteps().size() == 4,
               "Ask Torq factory sounds: a Basic Drums kit and a StringSwarm preset are listed and chosen the same way, and a patch alone changes a synth a track already plays");

        const auto own = call ("set_instrument", "{\"track\": \"Lead\", \"preset\": " + quoted (savedName) + "}");
        const auto last = w.getInstrumentLoads();
        check ((bool) own["ok"] && last.size() == 4 && ! last[3].factory && last[3].preset == savedName && w.getSteps().size() == 4
                   && own["step"] == juce::var ("Load the preset " + savedName.quoted() + " in Torqwave Synth on \"Lead\""),
               "Ask Torq factory sounds: a preset the user saved under a factory patch's name is the one chosen, and a later choice for a track replaces the earlier in the plan");

        // The state each factory sound loads: the instrument's own, with the sound in it.
        const auto synthState = Ai::FactorySounds::stateOf (synth, patch), kitState = Ai::FactorySounds::stateOf (drums, kit), guitarState = Ai::FactorySounds::stateOf (guitar, preset.name);
        BuiltInEffects::BasicSynthProcessor synthNow, synthDirect;
        BuiltInEffects::BasicDrumsProcessor drumsNow;
        BuiltInEffects::SwarmGuitarProcessor guitarNow;
        synthDirect.loadPatch (patch);

        if (synthState)  synthNow.setStateInformation (synthState->getData(), (int) synthState->getSize());
        if (kitState)    drumsNow.setStateInformation (kitState->getData(), (int) kitState->getSize());
        if (guitarState) guitarNow.setStateInformation (guitarState->getData(), (int) guitarState->getSize());

        auto guitarSame = guitarState.has_value();

        for (auto& [id, value] : preset.values)
            guitarSame = guitarSame && std::abs (guitarNow.getPresetValues()[id] - value) <= 1.0e-3 * juce::jmax (1.0, std::abs (value));

        juce::String error;
        const auto forLoad = Ai::stateFor (loads[0], error);
        check (synthState && synthNow.getPatchName() == patch && synthNow.getPresetValues() == synthDirect.getPresetValues() && kitState && drumsNow.getKitName() == kit && ! drumsNow.kitModified()
                   && guitarSame && ! Ai::FactorySounds::stateOf (synth, "zzqx") && ! Ai::FactorySounds::stateOf (drums, "zzqx") && ! Ai::FactorySounds::stateOf (guitar, "zzqx")
                   && ! Ai::FactorySounds::stateOf (other, patch) && forLoad == synthState && error.isEmpty(),
               "Ask Torq factory sounds: a factory load's state is the instrument's own with the sound in it (the synth's patch as Load Patch leaves it, the kit unmodified, "
               "the guitar's preset values); a name it hasn't gives none");
    }

    void testFactorySoundsOnEngine()
    {
        // A track playing Torqwave Synth on the engine: a factory patch chosen by Ask Torq, and loaded as the plan's apply loads it.
        const auto& patches = WaveSynth::factoryPatches();

        if (patches.size() < 100)
            return;

        AudioEngine engine;
        SessionState s;
        s.sampleRate = 48000.0;
        s.addInstrumentTrack().name = "Bass";
        const auto trackId = s.tracks[0].id;
        engine.session.reset (s, {});
        auto ready = false;
        engine.addTrackPlugin (trackId, builtIn ("Torqwave Synth"), [&] (const juce::String&) { ready = true; });
        TestUtils::runUntil ([&] { return ready; }, 5000);
        auto* synth = dynamic_cast<BuiltInEffects::BasicSynthProcessor*> (engine.getTrackEffects().getInstrument (trackId));

        if (synth == nullptr)
        {
            check (false, "Ask Torq factory sounds: Torqwave Synth loads onto a track");
            return;
        }

        const auto patch = patches[patches.size() - 3].name;
        const auto before = synth->getPatchName();
        Ai::Workspace::Start start;
        start.session = *engine.session.getState();
        start.plugins = BuiltInEffects::getDescriptions();
        start.instruments[trackId] = synth->getPluginDescription();
        start.library = Ai::Library::standard();
        Ai::Workspace w (std::move (start));
        const auto planned = parse (w.run ("set_instrument", "{\"track\": \"Bass\", \"preset\": " + juce::JSON::toString (juce::var (patch)) + "}"));
        const auto loads = w.getInstrumentLoads();
        juce::String error;
        const auto state = loads.size() == 1 ? Ai::stateFor (loads[0], error) : std::nullopt;
        const auto untouched = synth->getPatchName() == before && ! w.sessionChanged();
        const auto set = state && engine.setInstrumentState (trackId, *state);
        BuiltInEffects::BasicSynthProcessor direct;
        direct.loadPatch (patch);
        check ((bool) planned["ok"] && loads.size() == 1 && loads[0].onlyPreset && loads[0].factory && untouched && set && before != patch
                   && synth->getPatchName() == patch && synth->getPresetValues() == direct.getPresetValues(),
               "Ask Torq factory sounds: on the engine, a patch planned for a track's Torqwave Synth changes nothing until applied; applied, the synth holds the patch as Load Patch gives it");
    }

    /** A made-up factory folder: four sequences, a kit of two of them, two sequences sharing a name, and 45 fillers to pass the cap. */
    struct MadePresets
    {
        juce::File folder = juce::File::createTempFile ("ask-presets");
        SequenceFile::Sequence kick, hats, reese, pluck;
        SequenceKit::Kit kit;

        static SequenceFile::Sequence sequence (const juce::String& name, const juce::String& role, const juce::String& subRole, const juce::String& style, int energy,
                                                juce::StringArray character, std::vector<SequenceFile::Note> notes, const juce::String& instrument)
        {
            SequenceFile::Sequence q;
            q.name = name;
            q.role = role;
            q.subRole = subRole;
            q.style = style;
            q.energy = energy;
            q.character = std::move (character);
            q.channel = role == "Drums" ? 10 : 1;
            q.length = q.loopLength = 4 * 4 * SequenceFile::ticksPerBeat;
            q.notes = std::move (notes);
            q.added = 1000;

            if (instrument.isNotEmpty())
                q.voice = SequenceFile::voiceFor (builtIn (instrument));

            return q;
        }

        juce::File fileFor (const SequenceFile::Sequence& q) const
        {
            auto dir = folder.getChildFile (q.role);

            if (q.subRole.isNotEmpty())
                dir = dir.getChildFile (q.subRole);

            dir.createDirectory();
            return dir.getChildFile (q.name + SequenceFile::extension);
        }

        MadePresets()
        {
            constexpr juce::int64 beat = SequenceFile::ticksPerBeat;
            kick = sequence ("Iron Kick", "Drums", "Kick", "Industrial/EBM", 4, { "Dark", "Distorted" },
                             { { 36, 0, 240, 0.9f }, { 36, beat, 240, 0.8f }, { 38, 2 * beat, 240, 0.7f }, { 36, 3 * beat, 240, 0.8f }, { 42, 3 * beat + 480, 240, 0.5f } }, "Basic Drums");
            hats = sequence ("Thin Hats", "Drums", "Hats & Tops", "Techno/Dark", 2, { "Sparse" }, { { 42, 480, 120, 0.6f }, { 42, beat + 480, 120, 0.6f } }, "Basic Drums");
            reese = sequence ("Slow Reese", "Bass", {}, "Techno/Dark", 4, { "Dark" }, { { 33, 0, 2 * beat, 0.8f }, { 36, 2 * beat, 2 * beat, 0.8f } }, "Torqwave Synth");
            reese.key = "A minor";
            reese.note = "A slow detuned bass under the kick";
            pluck = sequence ("Bright Pluck", "Bass", {}, "House/Deep", 3, { "Bright" }, { { 45, 0, 240, 0.8f } }, {});
            auto twin = sequence ("Thin Hats", "Drums", "Kick", "Techno/Dark", 2, {}, { { 42, 0, 120, 0.6f } }, {});
            auto ok = true;

            for (auto* q : { &kick, &hats, &reese, &pluck, &twin })
                ok = SequenceFile::save (*q, fileFor (*q)).isEmpty() && ok;

            for (int i = 1; i <= 45; ++i)
            {
                const auto filler = sequence ("Filler " + juce::String (i).paddedLeft ('0', 2), "Arps", {}, "Techno/Dark", 1, {}, { { 60, 0, 240, 0.8f } }, {});
                ok = SequenceFile::save (filler, fileFor (filler)).isEmpty() && ok;
            }

            kit.name = "EBM Engine";
            kit.style = "Industrial/EBM";
            kit.note = "A kick and a bass made to play together";
            kit.added = 1000;
            kit.parts = { kick, reese };
            folder.getChildFile ("Kits").createDirectory();
            ok = SequenceKit::save (kit, folder.getChildFile ("Kits").getChildFile ("EBM Engine.tqkit")).isEmpty() && ok;
            check (ok, "Ask Torq presets: the made-up factory folder is written");
        }

        ~MadePresets()    { folder.deleteRecursively(); }

        Ai::Library library() const
        {
            Ai::Library l;
            l.factorySounds = [] (const juce::PluginDescription& d) { return Ai::FactorySounds::of (d); };
            l.sequencePresets = [f = folder] { return SequenceLibrary (f).scan(); };
            return l;
        }
    };

    /** General MIDI's map with the kick on key 60 and the snare on key 40, and the two that trade keys (as the drum grid's tests'). */
    std::vector<DrumSound> askKitMap()
    {
        auto map = DrumSound::generalMidi();

        for (auto& d : map)
        {
            if (d.pitch == 36) { d.pitch = 60; d.standsFor = 36; d.name = "Kit Kick"; }
            else if (d.pitch == 38) { d.pitch = 40; d.standsFor = 38; d.name = "Kit Snare"; }
            else if (d.pitch == 40) { d.pitch = 39; d.standsFor = 40; }
            else if (d.pitch == 39) { d.pitch = 38; d.standsFor = 39; }
        }

        return map;
    }

    void testSequencePresetTools()
    {
        MadePresets made;

        // 124 BPM: an audio track, a plain instrument track, and one whose kit has its own drum map.
        SessionState s;
        s.sampleRate = 48000.0;
        s.tempo = 124.0;
        s.addTrack().name = "Guitar";
        s.addInstrumentTrack().name = "Keys";
        s.addInstrumentTrack().name = "Kit";
        s.tracks[2].drumMap = askKitMap();
        const auto bar = [&s] (int number) { return s.tempoMap().barFrame (number - 1); };

        const auto workspace = [&] (const SessionState& session, bool transpose = false)
        {
            Ai::Workspace::Start start;
            start.session = session;
            start.sessionName = "Song";
            start.plugins = BuiltInEffects::getDescriptions();
            start.library = made.library();
            start.library.transposeDrops = transpose;
            return std::make_unique<Ai::Workspace> (std::move (start));
        };

        auto w = workspace (s);
        const auto call = [&w] (const juce::String& tool, const juce::String& args) { return parse (w->run (tool, args)); };
        const auto namesOf = [] (const juce::var& found)
        {
            juce::StringArray names;

            if (auto* list = found["presets"].getArray())
                for (auto& p : *list)
                    names.add (p["name"].toString());

            return names;
        };

        // Finding: by role, style (or its genre), energy, character and words; capped, with how many more.
        const auto everything = call ("find_sequence_presets", "{}");
        const auto drums = call ("find_sequence_presets", R"({"role": "drums"})");
        const auto techno = call ("find_sequence_presets", R"({"style": "techno", "energy": 4})");
        const auto ebm = call ("find_sequence_presets", R"({"style": "Industrial/EBM", "character": ["dark", "Distorted"]})");
        const auto kits = call ("find_sequence_presets", R"({"role": "Kits"})");
        const auto words = call ("find_sequence_presets", R"({"search": "detuned BASS"})");
        const auto arps = call ("find_sequence_presets", R"({"role": "Arps", "search": "filler"})");
        const auto nothing = call ("find_sequence_presets", R"({"role": "Bass", "energy": 1})");
        check ((bool) everything["ok"] && (int) everything["total"] == 51 && (int) everything["matching"] == 51 && everything["presets"].size() == 40 && (int) everything["more"] == 11
                   && everything["roles"] == juce::var ("Arps, Bass, Drums, Kits") && everything["styles"] == juce::var ("House/Deep, Industrial/EBM, Techno/Dark")
                   && namesOf (drums) == juce::StringArray { "Thin Hats", "Iron Kick", "Thin Hats" } && drums["roles"].isVoid()
                   && namesOf (techno) == juce::StringArray { "Slow Reese" } && namesOf (ebm) == juce::StringArray { "Iron Kick" }
                   && namesOf (kits) == juce::StringArray { "EBM Engine" } && kits["presets"][0]["kind"] == juce::var ("kit") && (int) kits["presets"][0]["parts"] == 2
                   && kits["presets"][0]["path"] == juce::var ("Kits/EBM Engine.tqkit") && (bool) kits["presets"][0]["brings_instrument"]
                   && namesOf (words) == juce::StringArray { "Slow Reese" } && words["presets"][0]["path"] == juce::var ("Bass/Slow Reese.tqseq")
                   && words["presets"][0]["key"] == juce::var ("A minor") && (int) words["presets"][0]["energy"] == 4 && juce::exactlyEqual ((double) words["presets"][0]["bars"], 4.0)
                   && words["presets"][0]["character"] == juce::var ("Dark") && words["presets"][0]["about"].toString().startsWith ("A slow")
                   && (int) arps["matching"] == 45 && arps["presets"].size() == 40 && (int) arps["more"] == 5
                   && (bool) nothing["ok"] && nothing["presets"].size() == 0 && nothing["styles"].toString().isNotEmpty() && w->getSteps().isEmpty(),
               "Ask Torq presets: find_sequence_presets narrows the factory sequences and kits by role, style or genre, energy, character and words (in any case), "
               "40 at a time with how many more; with nothing to narrow by, or nothing found, it names the roles and styles; finding plans nothing");

        const auto badRole = call ("find_sequence_presets", R"({"role": "Vocals"})");
        const auto badStyle = call ("find_sequence_presets", R"({"style": "Polka"})");
        const auto badEnergy = call ("find_sequence_presets", R"({"energy": 9})");
        const auto badCharacter = call ("find_sequence_presets", R"({"character": ["Wet"]})");
        check (! (bool) badRole["ok"] && badRole["error"].toString().contains ("Drums") && ! (bool) badStyle["ok"] && badStyle["error"].toString().contains ("Techno/Dark")
                   && ! (bool) badEnergy["ok"] && ! (bool) badCharacter["ok"] && badCharacter["error"].toString().contains ("Swung"),
               "Ask Torq presets: a role, style, energy or character there isn't is refused, naming those there are");

        // A preset on a new track at bar 3: the same session as SequenceFile::place gives, and its voice to load.
        const auto added = call ("add_sequence_preset", R"({"preset": "slow reese", "bar": 3})");
        auto direct = s;
        const auto placed = SequenceFile::place (direct, made.reese, -1, bar (3), false);
        const auto voices = w->getVoiceLoads();
        const auto session = call ("get_session", "{}");
        check ((bool) added["ok"] && bar (3) == std::llround (2.0 * 4.0 * 60.0 / 124.0 * 48000.0) && placed.clipId != 0 && sameSession (*w->getSession(), direct) && ! sameSession (*w->getSession(), s)
                   && (int) added["clip"] == (int) placed.clipId && (int) added["track"] == 4
                   && added["step"] == juce::var ("Add the sequence \"Slow Reese\" at bar 3 on a new track (4 bars, 2 notes), playing Torqwave Synth")
                   && voices.size() == 1 && voices[0].trackId == placed.trackId && sameXml (voices[0].effects, placed.effects) && voices[0].instrument == "Torqwave Synth"
                   && session["tracks"][3]["instrument"] == juce::var ("Torqwave Synth (when the plan is applied)") && w->sessionChanged() && w->getSteps().size() == 1,
               "Ask Torq presets: add_sequence_preset puts a factory sequence at a bar on a new track: the session is the same as SequenceFile::place makes it "
               "(at 124 BPM, bar 3 at frame " + juce::String (bar (3)) + "), with the same voice to load; one plan step");

        // Its instrument and effects arrive with it: they can't be chosen or changed in the same plan.
        const auto instrument = call ("set_instrument", R"({"track": "Slow Reese", "plugin": "Basic Drums"})");
        const auto effect = call ("add_track_effect", R"({"track": "Slow Reese", "plugin": "Compressor"})");
        check (! (bool) instrument["ok"] && instrument["error"].toString().contains ("later request") && ! (bool) effect["ok"] && w->getSteps().size() == 1
                   && w->getInstrumentLoads().empty() && w->getChainChanges().empty(),
               "Ask Torq presets: the new track's instrument and effects arrive with the sequence, so choosing or changing them in the same plan is refused");

        // A drum part by its path onto the track with a drum map: its notes only, on the kit's keys, as a drop there.
        const auto onKit = call ("add_sequence_preset", R"({"preset": "Drums/Kick/Iron Kick.tqseq", "track": "Kit", "bar": 1})");
        const auto placedOnKit = SequenceFile::place (direct, made.kick, 2, bar (1), true);
        const auto& kitClips = w->getSession()->tracks[2].noteClips;
        check ((bool) onKit["ok"] && sameSession (*w->getSession(), direct) && kitClips.size() == 1 && kitClips[0].id == placedOnKit.clipId && kitClips[0].notes.size() == 5
                   && kitClips[0].notes[0].pitch == 60 && kitClips[0].notes[2].pitch == 40 && kitClips[0].notes[4].pitch == 42 && placedOnKit.effects == nullptr
                   && w->getVoiceLoads().size() == 1 && (int) onKit["track"] == 3
                   && onKit["step"] == juce::var ("Add the sequence \"Iron Kick\" to \"Kit\" at bar 1 (4 bars, 5 notes), its drums on the keys of the track's drum map"),
               "Ask Torq presets: on a track with a drum map a factory drum part lands as a drop there does: its notes only, the kick on 60 and the snare on 40, no voice loaded");

        // Refusals leave the plan and the session as they were.
        const auto beforeRefusals = *w->getSession();
        const auto unknown = call ("add_sequence_preset", R"({"preset": "slow rese"})");
        const auto isKit = call ("add_sequence_preset", R"({"preset": "EBM Engine"})");
        const auto twins = call ("add_sequence_preset", R"({"preset": "Thin Hats"})");
        const auto onAudio = call ("add_sequence_preset", R"({"preset": "Slow Reese", "track": "Guitar"})");
        const auto barZero = call ("add_sequence_preset", R"({"preset": "Slow Reese", "bar": 0})");
        const auto notKit = call ("add_kit", R"({"kit": "Slow Reese"})");
        const auto noKit = call ("add_kit", R"({"kit": "EBM Engin"})");
        check (! (bool) unknown["ok"] && unknown["error"].toString().contains ("the nearest are Slow Reese") && ! (bool) isKit["ok"] && isKit["error"].toString().contains ("add_kit")
                   && ! (bool) twins["ok"] && twins["error"].toString().contains ("Drums/Hats & Tops/Thin Hats.tqseq") && twins["error"].toString().contains ("Drums/Kick/Thin Hats.tqseq")
                   && ! (bool) onAudio["ok"] && ! (bool) barZero["ok"] && ! (bool) notKit["ok"] && notKit["error"].toString().contains ("add_sequence_preset")
                   && ! (bool) noKit["ok"] && noKit["error"].toString().contains ("the nearest are EBM Engine")
                   && sameSession (*w->getSession(), beforeRefusals) && w->getSteps().size() == 2 && w->getVoiceLoads().size() == 1,
               "Ask Torq presets: a name that matches nothing is refused with the nearest names, a name two presets share with their paths, a kit given as a sequence "
               "(and a sequence as a kit), an audio track and bar 0 too; the plan and the session are left as they were");

        // A kit at bar 5: the same session as a dropped kit's, and each part's voice to load.
        const auto kitAdded = call ("add_kit", R"({"kit": "Kits/EBM Engine.tqkit", "bar": 5})");
        const auto kitPlaced = SequenceKit::place (direct, made.kit, bar (5));
        const auto kitVoices = w->getVoiceLoads();
        auto sameVoices = kitVoices.size() == 3 && kitPlaced.parts.size() == 2;

        for (size_t i = 0; sameVoices && i < kitPlaced.parts.size(); ++i)
            sameVoices = kitVoices[i + 1].trackId == kitPlaced.parts[i].trackId && sameXml (kitVoices[i + 1].effects, kitPlaced.parts[i].effects);

        check ((bool) kitAdded["ok"] && sameSession (*w->getSession(), direct) && sameVoices && kitVoices[1].instrument == "Basic Drums" && kitVoices[2].instrument == "Torqwave Synth"
                   && kitAdded["tracks"].size() == 2 && kitAdded["tracks"][0]["name"] == juce::var ("Iron Kick") && (int) kitAdded["tracks"][1]["clip"] == (int) kitPlaced.parts[1].clipId
                   && kitAdded["step"] == juce::var ("Add the kit \"EBM Engine\" at bar 5: a folder with 2 tracks (Iron Kick, Slow Reese), 4 bars")
                   && w->getSession()->findTrack (kitPlaced.folderId) != nullptr && w->getSession()->findTrack (kitPlaced.folderId)->isFolder && w->getSteps().size() == 3,
               "Ask Torq presets: add_kit puts a factory kit at a bar as a drop does: the session is the same as SequenceKit::place makes it (a folder, a track a part), "
               "with each part's voice to load; one plan step");

        // A part's track removed again in the same plan loads nothing.
        const auto removed = call ("remove_track", R"({"track": "Iron Kick"})");
        check ((bool) removed["ok"] && w->getVoiceLoads().size() == 2 && w->getVoiceLoads()[1].instrument == "Torqwave Synth",
               "Ask Torq presets: a kit part's track removed in the same plan drops its voice");

        // With drops moved into the session's key, the tool moves it as the drop does (A minor into C minor: three semitones up).
        auto keyed = s;
        keyed.musicalKey = "C minor";
        w = workspace (keyed, true);
        const auto moved = call ("add_sequence_preset", R"({"preset": "Slow Reese"})");
        auto keyedDirect = keyed;
        SequenceFile::place (keyedDirect, SequenceFile::transposedTo (made.reese, "C minor"), -1, 0, false);
        const auto& movedNotes = w->getSession()->tracks.back().noteClips[0].notes;
        check ((bool) moved["ok"] && sameSession (*w->getSession(), keyedDirect) && movedNotes.size() == 2 && movedNotes[0].pitch == 36 && movedNotes[1].pitch == 39
                   && moved["step"].toString().contains ("at bar 1 on a new track"),
               "Ask Torq presets: with drops set to move into the session's key, a preset added by the tool is moved as a dropped one is (A minor to C minor), at bar 1 when no bar is given");

        // No library: the tools say so.
        Ai::Workspace::Start bare;
        bare.session = s;
        Ai::Workspace none (std::move (bare));
        check (! (bool) parse (none.run ("find_sequence_presets", "{}"))["ok"] && ! (bool) parse (none.run ("add_kit", R"({"kit": "EBM Engine"})"))["ok"]
                   && (bool) parse (none.run ("list_drum_maps", "{}"))["ok"] && parse (none.run ("list_drum_maps", "{}"))["maps"].size() == 0 && none.getSteps().isEmpty(),
               "Ask Torq presets: with no factory presets or drum maps to reach, the tools say there are none");
    }

    void testRealFactoryPresets()
    {
        // The presets the app ships, unpacked into a folder of their own, through the library the app hands Ask Torq.
        const auto before = SequenceLibrary::factoryFolder();
        const auto base = juce::File::createTempFile ("ask-factory");
        SequenceLibrary::setFactoryFolder (base.getChildFile ("set"));
        const auto unpackError = SequenceLibrary::unpackFactoryPresets();

        SessionState s;
        s.sampleRate = 48000.0;
        s.tempo = 124.0;
        Ai::Workspace::Start start;
        start.session = s;
        start.library = Ai::Library::standard();
        start.library.transposeDrops = false;
        Ai::Workspace w (std::move (start));
        const auto call = [&w] (const juce::String& tool, const juce::String& args) { return parse (w.run (tool, args)); };

        const auto began = juce::Time::getMillisecondCounterHiRes();
        const auto everything = call ("find_sequence_presets", "{}");
        const auto scanMs = juce::Time::getMillisecondCounterHiRes() - began;
        const auto kits = call ("find_sequence_presets", R"({"role": "Kits", "style": "Industrial/EBM"})");
        auto allKits = kits["presets"].size() > 0;

        if (auto* list = kits["presets"].getArray())
            for (auto& kit : *list)
                allKits = allKits && kit["kind"] == juce::var ("kit") && kit["style"] == juce::var ("Industrial/EBM") && kit["path"].toString().startsWith ("Kits/");

        check (unpackError.isEmpty() && (bool) everything["ok"] && (int) everything["total"] > 1000 && everything["presets"].size() == 40
                   && (int) everything["more"] == (int) everything["total"] - 40 && everything["styles"].toString().contains ("Industrial/EBM") && allKits,
               "Ask Torq presets: the app's own " + everything["total"].toString() + " factory sequences and kits are found through the library the app hands over (read in "
                   + juce::String ((int) std::lround (scanMs)) + " ms); role Kits with style Industrial/EBM finds " + juce::String (kits["presets"].size()) + " kits, each of that style");

        if (allKits)
        {
            const auto path = kits["presets"][0]["path"].toString();
            juce::String error;
            const auto kit = SequenceKit::load (SequenceLibrary::factoryFolder().getChildFile (path), error);
            const auto added = call ("add_kit", "{\"kit\": " + juce::JSON::toString (juce::var (path)) + ", \"bar\": 9}");
            auto direct = s;
            const auto placed = kit ? SequenceKit::place (direct, *kit, s.tempoMap().barFrame (8)) : SequenceKit::Placed();
            const auto voices = w.getVoiceLoads();
            auto sameVoices = voices.size() == placed.parts.size();

            for (size_t i = 0; sameVoices && i < voices.size(); ++i)
                sameVoices = voices[i].trackId == placed.parts[i].trackId && sameXml (voices[i].effects, placed.parts[i].effects) && voices[i].instrument.isNotEmpty();

            check ((bool) added["ok"] && kit.has_value() && ! placed.parts.empty() && sameSession (*w.getSession(), direct) && sameVoices && w.getSteps().size() == 1,
                   "Ask Torq presets: the factory kit " + (kit ? kit->name.quoted() : juce::String ("?")) + " added at bar 9 by add_kit is the session SequenceKit::place makes, with its "
                       + juce::String ((int) placed.parts.size()) + " parts' voices to load");
        }

        SequenceLibrary::setFactoryFolder (before);
        base.deleteRecursively();
    }

    void testKitOnEngine()
    {
        // The same kit put in by Ask Torq and applied, and dropped the usual way, on two engines.
        MadePresets made;
        SessionState s;
        s.sampleRate = 48000.0;
        s.tempo = 124.0;
        s.addTrack().name = "Guitar";
        const auto frame = s.tempoMap().barFrame (2);

        AudioEngine asked, dropped;
        asked.session.reset (s, {});
        dropped.session.reset (s, {});

        Ai::Workspace::Start start;
        start.session = *asked.session.getState();
        start.library = made.library();
        Ai::Workspace w (std::move (start));
        const auto planned = parse (w.run ("add_kit", R"({"kit": "EBM Engine", "bar": 3})"));
        const auto untouched = asked.session.getState()->tracks.size() == 1;
        asked.session.apply (*w.getSession(), "Ask Torq: an EBM kit");
        auto askedDone = false, droppedDone = false;
        juce::String askedErrors, droppedErrors;
        Ai::loadVoices (asked, w.getVoiceLoads(), [&] (const juce::String& e) { askedErrors = e; askedDone = true; });
        const auto placed = SequenceKit::addToSession (dropped, made.kit, frame, [&] (const juce::String& e) { droppedErrors = e; droppedDone = true; });
        TestUtils::runUntil ([&] { return askedDone && droppedDone; }, 10000);

        const auto a = asked.session.getState(), d = dropped.session.getState();
        auto sameInstruments = placed.parts.size() == 2;
        juce::StringArray playing;

        for (auto& part : placed.parts)
        {
            auto* one = asked.getTrackEffects().getInstrument (part.trackId);
            auto* other = dropped.getTrackEffects().getInstrument (part.trackId);
            juce::MemoryBlock oneState, otherState;

            if (one != nullptr)   one->getStateInformation (oneState);
            if (other != nullptr) other->getStateInformation (otherState);

            sameInstruments = sameInstruments && one != nullptr && other != nullptr && one->getName() == other->getName() && oneState == otherState
                                  && asked.getTrackEffects().getNumSlots (part.trackId) == dropped.getTrackEffects().getNumSlots (part.trackId);
            playing.add (one != nullptr ? one->getName() : juce::String ("nothing"));
        }

        check ((bool) planned["ok"] && untouched && askedDone && droppedDone && askedErrors.isEmpty() && droppedErrors.isEmpty() && sameSession (*a, *d) && a->tracks.size() == 4
                   && sameInstruments && playing == juce::StringArray { "Basic Drums", "Torqwave Synth" },
               "Ask Torq presets: a kit added by Ask Torq and applied equals the kit dropped the usual way: the same session, and on each part's track the same instrument in the same state ("
                   + playing.joinIntoString (", ") + ")");

        asked.session.undo();
        check (sameSession (*asked.session.getState(), s), "Ask Torq presets: one undo takes the applied kit out again, tracks and folder");
    }

    void testDrumMapTools()
    {
        const auto folder = juce::File::createTempFile ("ask-drum-maps");
        DrumMaps::Store store (folder);
        const auto kitMap = askKitMap();
        auto plain = DrumSound::generalMidi();
        plain.resize (8);
        const auto saved = store.save ("Sampler: Dark Kit", kitMap).isEmpty() && store.save ("Short List", plain).isEmpty();

        // The drum grid's own test pattern: kick, snare, hat, clap and electric snare on General MIDI's keys.
        constexpr juce::int64 beat = 24000;
        SessionState s;
        s.sampleRate = 48000.0;
        s.addInstrumentTrack().name = "Drums";
        s.addTrack().name = "Guitar";
        const auto drumsId = s.tracks[0].id;
        NoteClip c;
        c.name = "Beat";
        c.length = 4 * beat;
        c.channel = 10;
        c.notes = { { 36, 0, 100, 0.9f }, { 38, beat, 100, 0.8f }, { 42, 2 * beat, 100, 0.7f }, { 39, 3 * beat, 100, 0.6f }, { 40, 3 * beat + 6000, 100, 0.5f } };
        const auto clipId = SessionEdits::addNoteClip (s, drumsId, c);
        const auto audioId = SessionEdits::addClip (s, 1, juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("riff.wav"), tone (48000.0, 2), 0);
        const auto before = s.tracks[0].noteClips[0].notes;

        Ai::Workspace::Start start;
        start.session = s;
        start.sessionName = "Song";
        start.library.drumMapNames = [&store] { return store.names(); };
        start.library.drumMap = [&store] (const juce::String& name) { return store.load (name); };
        Ai::Workspace w (std::move (start));
        const auto call = [&w] (const juce::String& tool, const juce::String& args) { return parse (w.run (tool, args)); };
        const auto id = juce::String ((int) clipId);
        const auto& notes = w.getSession()->tracks[0].noteClips[0].notes;

        const auto listed = call ("list_drum_maps", R"({"track": "Drums"})");
        const auto tooSoon = call ("remap_clip_notes", "{\"clip\": " + id + ", \"to\": \"map\"}");
        check (saved && (bool) listed["ok"] && listed["maps"].size() == 2 && listed["maps"][0]["name"] == juce::var ("Sampler - Dark Kit") && (int) listed["maps"][0]["sounds"] == (int) kitMap.size()
                   && (int) listed["maps"][0]["off_general_midi_keys"] == 4 && (int) listed["maps"][1]["off_general_midi_keys"] == 0 && listed["track_map"] == juce::var ("General MIDI")
                   && ! (bool) tooSoon["ok"] && tooSoon["error"].toString().contains ("set_drum_map") && w.getSteps().isEmpty(),
               "Ask Torq drum maps: list_drum_maps names the saved maps with how many sounds are off General MIDI's keys, and a track's map; on General MIDI's layout there is nothing to remap");

        // The track's map from a saved one, by name in any case.
        const auto set = call ("set_drum_map", R"({"track": "Drums", "map": "sampler - dark kit"})");
        const auto again = call ("set_drum_map", R"({"track": "Drums", "map": "Sampler - Dark Kit"})");
        const auto unknown = call ("set_drum_map", R"({"track": "Drums", "map": "Sampler Dark"})");
        const auto onAudio = call ("set_drum_map", R"({"track": "Guitar", "map": "Short List"})");
        const auto relisted = call ("list_drum_maps", R"({"track": "Drums"})");
        check ((bool) set["ok"] && set["step"] == juce::var ("Give \"Drums\" the drum map \"Sampler - Dark Kit\" (" + juce::String ((int) kitMap.size()) + " sounds)")
                   && (bool) set["remaps"] && set["note"].toString().startsWith ("4 notes on the track are on General MIDI keys the map moves") && w.getSession()->tracks[0].drumMap == kitMap && notes == before
                   && ! (bool) again["ok"] && ! (bool) unknown["ok"] && unknown["error"].toString().contains ("the nearest are Sampler - Dark Kit, Short List") && ! (bool) onAudio["ok"]
                   && relisted["track_map"] == juce::var ("Sampler - Dark Kit") && relisted["track_sounds"].size() == (int) kitMap.size()
                   && relisted["track_sounds"][1] == juce::var ("60 Kit Kick (stands for 36)") && w.getSteps().size() == 1,
               "Ask Torq drum maps: set_drum_map gives a track a saved map by name, the notes staying put; the same map again, a name there isn't (with the nearest) "
               "and an audio track are refused, leaving the plan as it was");

        // Remap: as the piano roll's Remap Notes moves them.
        NoteEditor::Shown shown;
        shown.clip = s.tracks[0].noteClips[0];
        shown.trackName = "Drums";
        shown.drums = true;
        shown.drumMap = kitMap;
        std::vector<juce::String> editorSteps;
        NoteEditor editor;
        editor.readClip = [&shown] { return std::optional<NoteEditor::Shown> (shown); };
        editor.apply = [&] (const NoteClip& clip, const juce::String& description) { shown.clip = clip; editorSteps.push_back (description); };
        editor.setBounds (0, 0, 1000, 600);
        editor.refresh();
        const auto editorMoved = editor.remapNotes (true);
        const auto editorNotes = shown.clip.notes;

        const auto remapped = call ("remap_clip_notes", "{\"clip\": " + id + ", \"to\": \"map\"}");
        const auto toKit = notes;
        check ((bool) remapped["ok"] && editorMoved == 4 && editorSteps.size() == 1 && (int) remapped["moved"] == editorMoved && toKit == editorNotes
                   && toKit.size() == 5 && toKit[0].pitch == 60 && toKit[1].pitch == 40 && toKit[2].pitch == 42 && toKit[3].pitch == 38 && toKit[4].pitch == 39
                   && remapped["step"] == juce::var ("Remap 4 notes of the clip \"Beat\" on \"Drums\" to the keys of its drum map") && w.getSteps().size() == 2,
               "Ask Torq drum maps: remap_clip_notes moves a clip's notes as the piano roll's Remap Notes does (kick 36 to 60, snare 38 to 40, the two that trade keys each once, the hat staying)");

        // Applied to a session and undone: the notes and the map come back exactly.
        Ai::Workspace::Start fresh;
        fresh.session = s;
        fresh.library.drumMapNames = [&store] { return store.names(); };
        fresh.library.drumMap = [&store] (const juce::String& name) { return store.load (name); };
        Ai::Workspace plan (std::move (fresh));
        const auto planned = (bool) parse (plan.run ("set_drum_map", R"({"track": "Drums", "map": "Sampler - Dark Kit"})"))["ok"]
                                 && (bool) parse (plan.run ("remap_clip_notes", "{\"clip\": " + id + ", \"to\": \"map\"}"))["ok"];
        AudioEngine engine;
        engine.session.reset (s, {});
        engine.session.apply (*plan.getSession(), "Ask Torq: onto the kit");
        const auto applied = engine.session.getState();
        const auto appliedRight = applied->tracks[0].noteClips[0].notes == editorNotes && applied->tracks[0].drumMap == kitMap;
        engine.session.undo();
        const auto undone = engine.session.getState();
        check (planned && plan.getSteps().size() == 2 && appliedRight && undone->tracks[0].noteClips[0].notes == before && undone->tracks[0].drumMap.empty() && sameSession (*undone, s),
               "Ask Torq drum maps: set_drum_map then remap_clip_notes, applied, is one undo step: undone, the notes and the map are back exactly");

        // Back to General MIDI in the plan, as the piano roll's own command; then nothing left to move.
        NoteEditor::Shown onKit;
        onKit.clip = plan.getSession()->tracks[0].noteClips[0];
        onKit.drumMap = kitMap;
        shown = onKit;
        editor.refresh();
        const auto editorBack = editor.remapNotes (false);
        const auto back = parse (plan.run ("remap_clip_notes", "{\"clip\": " + id + ", \"to\": \"general_midi\"}"));
        const auto& planNotes = plan.getSession()->tracks[0].noteClips[0].notes;
        const auto badTo = parse (plan.run ("remap_clip_notes", "{\"clip\": " + id + ", \"to\": \"kit\"}"));
        const auto audio = parse (plan.run ("remap_clip_notes", "{\"clip\": " + juce::String ((int) audioId) + ", \"to\": \"map\"}"));
        const auto noClip = parse (plan.run ("remap_clip_notes", R"({"clip": 9999, "to": "map"})"));
        const auto general = parse (plan.run ("set_drum_map", R"({"track": "Drums", "map": "General MIDI"})"));
        const auto generalAgain = parse (plan.run ("set_drum_map", R"({"track": "Drums", "map": "general midi"})"));
        check ((bool) back["ok"] && editorBack == 4 && (int) back["moved"] == 4 && planNotes == before && shown.clip.notes == before
                   && back["step"] == juce::var ("Remap 4 notes of the clip \"Beat\" on \"Drums\" to General MIDI's keys")
                   && ! (bool) badTo["ok"] && ! (bool) audio["ok"] && audio["error"].toString().contains ("audio clip") && ! (bool) noClip["ok"]
                   && (bool) general["ok"] && plan.getSession()->tracks[0].drumMap.empty() && ! (bool) generalAgain["ok"] && plan.getSteps().size() == 4,
               "Ask Torq drum maps: remapping back to General MIDI gives the notes back exactly, as the piano roll's does; an audio clip, a clip that isn't there and another direction are refused; "
               "\"General MIDI\" takes the map off");

        folder.deleteRecursively();
    }

    void testSwarmGuitarParameters()
    {
        // StringSwarm on a track: its parameters reach the model as any instrument's do, the Swarm source and Strum Sync among them.
        AudioEngine engine;
        SessionState s;
        s.sampleRate = 48000.0;
        s.addInstrumentTrack().name = "Guitar";
        s.addInstrumentTrack().name = "Synth";
        const auto guitarId = s.tracks[0].id, synthId = s.tracks[1].id;
        engine.session.reset (s, {});
        int ready = 0;
        engine.addTrackPlugin (guitarId, builtIn ("StringSwarm"), [&] (const juce::String&) { ++ready; });
        engine.addTrackPlugin (synthId, builtIn ("Torqwave Synth"), [&] (const juce::String&) { ++ready; });
        TestUtils::runUntil ([&] { return ready == 2; }, 5000);
        auto* guitar = engine.getTrackEffects().getInstrument (guitarId);
        auto* synth = engine.getTrackEffects().getInstrument (synthId);

        if (guitar == nullptr || synth == nullptr)
        {
            check (false, "Ask Torq parameters: StringSwarm and Torqwave Synth load onto tracks");
            return;
        }

        const auto chain = Ai::readChain (engine.getTrackEffects(), guitarId);
        const auto synthChain = Ai::readChain (engine.getTrackEffects(), synthId);
        const Ai::SessionTools::Parameter *source = nullptr, *sync = nullptr;

        if (! chain.empty())
            for (auto& p : chain[0].parameters)
            {
                if (p.id == "source")    source = &p;
                if (p.id == "strumSync") sync = &p;
            }

        // Torqwave Synth's newest: each oscillator's level envelope, and modulation slots 9 to 16.
        int levelEnvelopes = 0, lateSlots = 0;

        if (! synthChain.empty())
            for (auto& p : synthChain[0].parameters)
            {
                levelEnvelopes += p.name.endsWith ("Level Envelope") ? 1 : 0;
                lateSlots += p.name.startsWith ("Mod 9 ") || p.name.startsWith ("Mod 16 ") ? 1 : 0;
            }

        const auto guitarCount = guitar->getParameters().size(), synthCount = synth->getParameters().size();
        check (chain.size() == 1 && chain[0].instrument && (int) chain[0].parameters.size() == guitarCount && guitarCount == 27
                   && source != nullptr && source->name == "Source" && source->choices.size() == 2 && sync != nullptr && sync->name == "Strum Sync" && sync->choices.size() > 2
                   && synthChain.size() == 1 && (int) synthChain[0].parameters.size() == synthCount && levelEnvelopes == WaveSynth::numOscillators && lateSlots > 0,
               "Ask Torq parameters: every one of StringSwarm's " + juce::String (guitarCount) + " parameters is read for the model, as Torqwave Synth's "
                   + juce::String (synthCount) + " are, Source and Strum Sync with their choices, and the synth's level envelopes and slots 9 to 16");

        // The Sequencer names an instrument's parameters to the model up to 256, the plug-in catalogue up to 300.
        const auto facts = PluginCatalogue::factsOf (*guitar);
        check (guitarCount <= 256 && synthCount <= 256 && synthCount <= PluginCatalogue::maxParameters && facts["parameters"].size() == guitarCount,
               "Ask Torq parameters: both instruments' parameters fit under the 256 the Sequencer's tools name and the 300 the catalogue keeps (StringSwarm "
                   + juce::String (guitarCount) + ", Torqwave Synth " + juce::String (synthCount) + ")");

        if (source == nullptr || sync == nullptr)
            return;

        Ai::Workspace::Start start;
        start.session = *engine.session.getState();
        start.plugins = BuiltInEffects::getDescriptions();
        start.instruments[guitarId] = guitar->getPluginDescription();
        start.effects[guitarId] = chain;
        start.instruments[synthId] = synth->getPluginDescription();
        start.effects[synthId] = synthChain;
        Ai::Workspace w (std::move (start));
        const auto call = [&w] (const juce::String& tool, const juce::String& args) { return parse (w.run (tool, args)); };
        const auto all = call ("list_effect_parameters", R"({"track": "Guitar", "effect": "instrument"})");
        const auto strum = call ("list_effect_parameters", R"({"track": "Guitar", "effect": "instrument", "find": "strum sync"})");
        const auto swarm = source->choices[1], division = sync->choices[2];
        const auto set = call ("set_track_effect", "{\"track\": \"Guitar\", \"effect\": \"instrument\", \"parameters\": [{\"parameter\": \"Source\", \"text\": "
                                                       + juce::JSON::toString (juce::var (swarm)) + "}, {\"parameter\": \"Strum Sync\", \"text\": " + juce::JSON::toString (juce::var (division)) + "}]}");
        const auto changes = w.getChainChanges();
        auto made = false;

        if (changes.size() == 1)
            Ai::applyChainChange (engine, changes[0], [&] (const juce::String&) { made = true; });

        TestUtils::runUntil ([&] { return made; }, 5000);
        const auto ids = TrackEffects::parametersById (*guitar);
        const auto sourceNow = ids.count ("source") > 0 ? ids.at ("source")->getCurrentValueAsText() : juce::String();
        const auto syncNow = ids.count ("strumSync") > 0 ? ids.at ("strumSync")->getCurrentValueAsText() : juce::String();
        const auto synthAll = call ("list_effect_parameters", R"({"track": "Synth", "effect": "instrument"})");
        const auto envelopes = call ("list_effect_parameters", R"({"track": "Synth", "effect": "instrument", "find": "level envelope"})");
        const auto slot16 = call ("list_effect_parameters", R"({"track": "Synth", "effect": "instrument", "find": "Mod 16 "})");
        check (synthAll["parameters"].size() == 150 && synthAll["note"].toString().contains ("of " + juce::String (synthCount)) && envelopes["parameters"].size() == WaveSynth::numOscillators
                   && slot16["parameters"].size() > 0,
               "Ask Torq parameters: Torqwave Synth's are listed 150 at a time, and find reaches the rest: the " + juce::String (envelopes["parameters"].size())
                   + " level envelopes' switches and slot 16's " + juce::String (slot16["parameters"].size()) + " settings");
        check (all["parameters"].size() == guitarCount && all["note"].isVoid() && strum["parameters"].size() == 1 && strum["parameters"][0]["choices"].toString().contains (division)
                   && (bool) set["ok"] && made && sourceNow == swarm && syncNow == division,
               "Ask Torq parameters: list_effect_parameters lists all of StringSwarm's, and set_track_effect sets Source to " + swarm.quoted() + " and Strum Sync to " + division.quoted()
                   + " by their choices, made on the engine's instrument");
    }

    void testAskForFactoryThings()
    {
        // A made-up model asks for a factory patch on the bass track and a kit at bar 5; the window shows the plan and what is outside its undo step.
        MadePresets made;
        const auto patch = WaveSynth::factoryPatches().empty() ? juce::String ("Init") : WaveSynth::factoryPatches()[3].name;
        const auto chunk = [] (const juce::String& json) { return "data: " + json + "\n\n"; };
        const auto toolCall = [&chunk] (const juce::String& callId, const juce::String& tool, const juce::String& arguments)
        {
            return chunk ("{\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"" + callId + "\",\"function\":{\"name\":\"" + tool + "\",\"arguments\":"
                              + juce::JSON::toString (juce::var (arguments)) + "}}]},\"finish_reason\":\"tool_calls\"}]}") + "data: [DONE]\n\n";
        };
        const auto last = chunk (R"({"choices":[{"delta":{"content":"A patch on the bass and a kit at bar 5."},"finish_reason":"stop"}]})") + "data: [DONE]\n\n";
        MockServer server ({ { 200, "text/event-stream", toolCall ("a", "set_instrument", "{\"track\": \"Bass\", \"plugin\": \"Torqwave Synth\", \"preset\": " + juce::JSON::toString (juce::var (patch)) + "}") },
                             { 200, "text/event-stream", toolCall ("b", "add_kit", R"({"kit": "EBM Engine", "bar": 5})") },
                             { 200, "text/event-stream", last } });

        SessionState s;
        s.sampleRate = 48000.0;
        s.addInstrumentTrack().name = "Bass";
        std::optional<SessionState> applied;
        size_t loads = 0, voices = 0;

        Ai::AskPanel panel (settingsFor (server.url()),
                            [&]
                            {
                                Ai::Workspace::Start start;
                                start.session = s;
                                start.sessionName = "Song";
                                start.plugins = BuiltInEffects::getDescriptions();
                                start.library = made.library();
                                return std::optional<Ai::Workspace::Start> (std::move (start));
                            },
                            [&] (Ai::Workspace& w, const juce::String&)
                            {
                                applied = *w.getSession();
                                loads = w.getInstrumentLoads().size();
                                voices = w.getVoiceLoads().size();
                                return juce::String();
                            });

        panel.getRequestBox().setText ("A dark patch on the bass, and an EBM kit at bar 5");
        panel.ask();
        TestUtils::runUntil ([&] { return panel.getLog().contains ("Apply to make these changes"); }, 10000);
        const auto log = panel.getLog();
        const auto requests = server.requests();
        const auto body = requests.empty() ? juce::var() : parse (requests[0].body);
        juce::StringArray tools;

        if (auto* list = body["tools"].getArray())
            for (auto& t : *list)
                tools.add (t["function"]["name"].toString());

        check (log.contains ("1. Load Torqwave Synth with the factory sound " + patch.quoted() + " on \"Bass\"")
                   && log.contains ("2. Add the kit \"EBM Engine\" at bar 5: a folder with 2 tracks (Iron Kick, Slow Reese), 4 bars")
                   && log.contains ("Instruments are loaded once the plan is applied; that isn't part of the undo step.") && ! log.contains ("applied as one undo step"),
               "Ask Torq: asked by a made-up model for a factory patch and a kit, the window shows both plan steps and says the instruments load outside the undo step");
        check (requests.size() == 3 && tools.size() == Ai::SessionTools::toolDefinitions().size() + 2 && tools.contains ("find_sequence_presets") && tools.contains ("add_sequence_preset")
                   && tools.contains ("add_kit") && tools.contains ("list_drum_maps") && tools.contains ("set_drum_map") && tools.contains ("remap_clip_notes")
                   && body["messages"][0]["content"].toString().contains ("find_sequence_presets") && body["messages"][0]["content"].toString().contains ("remap_clip_notes")
                   && body["messages"][0]["content"].toString().contains ("list_instrument_presets (with category and search"),
               "Ask Torq: the model is given the six new tools (" + juce::String (tools.size()) + " for a session) and told of them in its instructions");

        for (auto* c : panel.getChildren())
            if (auto* b = dynamic_cast<juce::TextButton*> (c); b != nullptr && b->getButtonText() == "Apply")
                b->triggerClick();

        TestUtils::runUntil ([&] { return applied.has_value(); }, 5000);
        check (applied.has_value() && applied->tracks.size() == 4 && loads == 1 && voices == 2 && s.tracks.size() == 1,
               "Ask Torq: Apply hands over the session with the kit's folder and tracks, the patch to load and the kit's two voices; the session itself is untouched until then");
    }

    void testCatalogueLearner()
    {
        juce::AudioPluginFormatManager formats;
        formats.addFormat (BuiltInEffects::createFormat());
        juce::Array<juce::PluginDescription> effects;

        for (auto& d : BuiltInEffects::getDescriptions())
            if (Ai::CatalogueLearner::isLearnt (d))
                effects.add (d);

        std::vector<MockServer::Reply> replies;

        for (int i = 0; i < effects.size(); ++i)
            replies.push_back ({ 200, "application/json", R"({"choices":[{"message":{"content":"Entry )" + juce::String (i) + R"(. More."},"finish_reason":"stop"}]})" });

        MockServer server (replies);
        const juce::TemporaryFile temp (".json");
        PluginCatalogue catalogue (temp.getFile());
        auto settings = settingsFor (server.url());
        Ai::CatalogueLearner learner (catalogue, formats, [] { return BuiltInEffects::getDescriptions(); }, [&] { return settings; });

        learner.start();
        TestUtils::runUntil ([&] { return ! learner.isRunning(); }, 120000);
        const auto p = learner.getProgress();
        const auto requests = server.requests();
        auto allLearnt = true;

        for (auto& d : effects)
            allLearnt = allLearnt && catalogue.hasCurrentFacts (d) && catalogue.getUnderstanding (d).has_value()
                        && catalogue.getUnderstanding (d)->model == "test-model";

        check (p.done == effects.size() && p.read == effects.size() && p.written == effects.size() && allLearnt
                   && ! catalogue.getUnderstanding (builtIn ("Torqwave Synth")).has_value() && ! catalogue.hasCurrentFacts (builtIn ("Torqwave Synth")),
               "plug-in catalogue: Learn All reads and describes every built-in effect, and leaves instruments for later (" + p.message + ")");
        check ((int) requests.size() == effects.size() && requests[0].body.contains ("parameterCount") && ! requests[0].body.contains ("\"tools\""),
               "plug-in catalogue: the model is sent each plug-in's facts, with no tools");
        check (catalogue.saveIfNeeded() && PluginCatalogue (temp.getFile()).getNumUnderstood() == effects.size(), "plug-in catalogue: what was learnt is on disk");

        {
            // A plug-in Torqwave ships a description of is read, not described again.
            const auto limiter = builtIn ("Limiter");
            const auto knowledge = juce::JSON::parse ("{\"plugins\": {\"" + (limiter.manufacturerName + "|" + limiter.name).toLowerCase()
                                                      + "\": {\"about\": \"Shipped.\", \"written\": \"m\", \"reviewed\": \"2026-11-18T00:00:00Z\"}}}");
            MockServer one ({ { 200, "application/json", R"({"choices":[{"message":{"content":"Written."},"finish_reason":"stop"}]})" } });
            const juce::TemporaryFile fresh (".json");
            PluginCatalogue shippedCatalogue (fresh.getFile(), knowledge);
            auto oneSettings = settingsFor (one.url());
            Ai::CatalogueLearner two (shippedCatalogue, formats,
                                      [&] { return juce::Array<juce::PluginDescription> { limiter, builtIn ("Compressor") }; }, [&] { return oneSettings; });
            two.start();
            TestUtils::runUntil ([&] { return ! two.isRunning(); }, 60000);
            check (two.getProgress().read == 2 && two.getProgress().written == 1 && one.requests().size() == 1
                       && shippedCatalogue.getUnderstanding (limiter)->text == "Shipped." && shippedCatalogue.getUnderstanding (builtIn ("Compressor"))->text == "Written.",
                   "plug-in catalogue: Learn All reads a shipped plug-in's parameters but asks the model only about the rest");
        }

        // A second run has nothing left to do; a server that can't be reached stops a run at once.
        settings.serverUrl = "http://127.0.0.1:1/v1";
        learner.start();
        TestUtils::runUntil ([&] { return ! learner.isRunning(); }, 30000);
        check (learner.getProgress().done == effects.size() && learner.getProgress().written == 0 && learner.getProgress().read == 0,
               "plug-in catalogue: a second run skips what is known");

        catalogue.forget (builtIn ("Limiter"));
        catalogue.forget (builtIn ("Compressor"));
        learner.start();
        TestUtils::runUntil ([&] { return ! learner.isRunning(); }, 30000);
        check (learner.getProgress().message.contains ("Learning stopped") && learner.getProgress().written == 0 && learner.getProgress().done < 2,
               "plug-in catalogue: an unreachable server stops learning at once (" + learner.getProgress().message + ")");

        // The window lists one row a plug-in and shows the selected one's entry.
        Ai::CatalogueView view (catalogue, learner, [] { return BuiltInEffects::getDescriptions(); }, [&] { return settings; });
        view.setSize (700, 600);
        view.select (0);
        check (view.getNumShown() == effects.size() && view.getEntryText().startsWith ("Entry"),
               "plug-in catalogue: the window lists each effect and shows its entry");
    }

    //==============================================================================
    /** A bass line for the sequence tests: two bars of A minor, two of its four notes outside the
        key (F#3 on beat 3, G#2 on beat 4). */
    SequenceFile::Sequence bassLine()
    {
        SequenceFile::Sequence q;
        q.name = "Night Bass";
        q.role = "Bass";
        q.key = "A minor";
        q.beatsPerBar = 4;
        q.length = 2 * 4 * SequenceFile::ticksPerBeat;
        q.notes = { { 45, 0, 960, 100.0f / 127.0f }, { 48, 960, 960, 90.0f / 127.0f }, { 54, 1920, 960, 90.0f / 127.0f }, { 44, 2880, 960, 80.0f / 127.0f } };
        return q;
    }

    /** One streamed answer that calls a tool with these arguments. */
    juce::String toolCallAnswer (const juce::String& id, const juce::String& tool, const juce::String& argumentsJson)
    {
        auto function = std::make_unique<juce::DynamicObject>();
        function->setProperty ("name", tool);
        function->setProperty ("arguments", argumentsJson);
        auto call = std::make_unique<juce::DynamicObject>();
        call->setProperty ("index", 0);
        call->setProperty ("id", id);
        call->setProperty ("function", juce::var (function.release()));
        auto delta = std::make_unique<juce::DynamicObject>();
        delta->setProperty ("tool_calls", juce::Array<juce::var> { juce::var (call.release()) });
        auto choice = std::make_unique<juce::DynamicObject>();
        choice->setProperty ("delta", juce::var (delta.release()));
        choice->setProperty ("finish_reason", "tool_calls");
        auto root = std::make_unique<juce::DynamicObject>();
        root->setProperty ("choices", juce::Array<juce::var> { juce::var (choice.release()) });
        return "data: " + juce::JSON::toString (juce::var (root.release()), true) + "\n\ndata: [DONE]\n\n";
    }

    juce::String textAnswer (const juce::String& text)
    {
        return "data: {\"choices\":[{\"delta\":{\"content\":\"" + text + "\"},\"finish_reason\":\"stop\"}]}\n\ndata: [DONE]\n\n";
    }

    const char* writtenLine = R"({"notes": [ { "pitch": 45, "start": 0, "length": 480, "velocity": 110 }, { "pitch": 45, "start": 720, "length": 240, "velocity": 64, "probability": 0.5 },
        { "pitch": 48, "start": 960, "length": 960, "velocity": 96 }, { "pitch": 54, "start": 1920, "length": 480, "velocity": 88 },
        { "pitch": 52, "start": 2880, "length": 960, "velocity": 100 }, { "pitch": 57, "start": 3840, "length": 3840, "velocity": 72 } ],
        "lanes": [ { "controller": 74, "points": [ { "tick": 0, "value": 0.25 }, { "tick": 7680, "value": 0.75 } ] } ] })";

    const char* review = R"({"review": "A steady line; the F#3 on beat 3 is outside A minor and sounds borrowed.", "suggestions": [
        { "title": "F#3 to G3", "why": "Keeps it in the key.", "kind": "notes", "change": [ { "pitch": 54, "start": 1920, "to": { "pitch": 55 } } ] },
        { "title": "Mod wheel swell", "kind": "lanes", "lanes": [ { "controller": 1, "points": [ { "tick": 0, "value": 0 }, { "tick": 3840, "value": 1 } ] } ] },
        { "title": "Chorus on the voice", "kind": "effect", "effect": "Chorus" },
        { "title": "Open the filter", "kind": "automation", "parameter": "filter 1 cutoff", "points": [ { "tick": 0, "value": 0.2 }, { "tick": 7680, "value": 0.9 } ] } ] })";

    /** The sequence tools on a working copy: what get_sequence tells, write_sequence's checks
        and exact notes, and suggest_for_sequence's checks and its suggestions applied. */
    void testSequenceTools()
    {
        Ai::Workspace::Start start;
        Ai::SequenceTools::Context context;
        context.sequence = bassLine();
        context.instrument = "Torqwave Synth";
        context.instrumentAbout = "A simple subtractive synth.";
        context.instrumentParameters = { "Filter 1 Cutoff", "Filter 1 Resonance" };
        context.tempo = 96.0;
        context.sessionTempo = 128.0;
        context.sessionKey = "E minor";
        start.sequence = context;
        start.plugins = BuiltInEffects::getDescriptions();
        Ai::Workspace w (std::move (start));
        const auto call = [&w] (const juce::String& tool, const juce::String& args) { return parse (w.run (tool, args)); };

        juce::StringArray names;
        const auto tools = w.tools();

        if (auto* list = tools.getArray())
            for (auto& t : *list)
                names.add (t["function"]["name"].toString());

        const auto system = w.systemMessage();
        check (w.isSequence() && names == juce::StringArray { "get_sequence", "write_sequence", "suggest_for_sequence", "list_plugins", "describe_plugin" }
                   && system.contains ("\"Night Bass\" open: a Bass part") && system.contains ("in A minor") && system.contains ("Torqwave Synth (A simple subtractive synth)")
                   && system.contains ("96 BPM") && system.contains ("session's tempo is 128 BPM") && system.contains ("the session is in E minor"),
               "Ask Torq sequences: five tools (" + names.joinIntoString (", ") + "), and the system message gives the part's role, key, instrument, the tempos and the session's key");

        const auto got = call ("get_sequence", "{}");
        check ((bool) got["ok"] && got["role"] == juce::var ("Bass") && got["key"] == juce::var ("A minor") && (int) got["length"] == 7680 && (int) got["ticks_per_beat"] == 960
                   && got["notes"].size() == 4 && (int) got["notes"][1]["velocity"] == 90 && got["notes"][2]["name"] == juce::var ("F#3")
                   && got["instrument"] == juce::var ("Torqwave Synth") && got["instrument_about"] == juce::var ("A simple subtractive synth.")
                   && got["instrument_parameters"].size() == 2 && (double) got["session_tempo"] == 128.0 && got["session_key"] == juce::var ("E minor")
                   && got["outside_key"].size() == 2 && got["outside_key"][0] == juce::var ("F#3 at bar 1, beat 3") && got["outside_key"][1] == juce::var ("G#2 at bar 1, beat 4"),
               "Ask Torq sequences: get_sequence gives the notes in ticks with their names, the instrument and what it is, and names the notes outside the key ("
                   + juce::JSON::toString (got["outside_key"], true) + ")");

        const auto refusedFor = [&] (const juce::String& args) { const auto r = call ("write_sequence", args); return ! (bool) r["ok"] ? r["error"].toString() : juce::String(); };
        const auto badPitch = refusedFor (R"({"notes": [ { "pitch": 128, "start": 0, "length": 240 } ]})");
        const auto pastEnd = refusedFor (R"({"notes": [ { "pitch": 45, "start": 7680, "length": 240 } ]})");
        const auto overEnd = refusedFor (R"({"notes": [ { "pitch": 45, "start": 7600, "length": 240 } ]})");
        const auto noLength = refusedFor (R"({"notes": [ { "pitch": 45, "start": 0, "length": 0 } ]})");
        const auto loudness = refusedFor (R"({"notes": [ { "pitch": 45, "start": 0, "length": 240, "velocity": 0 } ]})");
        const auto badLane = refusedFor (R"({"notes": [], "lanes": [ { "controller": 129, "points": [ { "tick": 0, "value": 0.5 } ] } ]})");
        const auto badValue = refusedFor (R"({"notes": [], "lanes": [ { "controller": 1, "points": [ { "tick": 0, "value": 1.5 } ] } ]})");
        const auto badKey = refusedFor (R"({"notes": [], "key": "H minor"})");
        const auto badLength = refusedFor (R"({"notes": [], "length": 1000})");
        check (badPitch.contains ("0 to 127") && pastEnd.contains ("never play") && overEnd.contains ("past the sequence's end") && noLength.contains ("at least 1")
                   && loudness.contains ("velocity") && badLane.contains ("pitch bend") && badValue.contains ("0 to 1") && badKey.contains ("isn't a key")
                   && badLength.contains ("whole number of beats") && w.getSteps().isEmpty() && ! w.getSequenceTools()->written(),
               "Ask Torq sequences: write_sequence refuses, with a reason, a pitch past 127, a note past the end, no length, velocity 0, a bad controller or value, a key that isn't one and a length that isn't whole beats; nothing is written");

        const auto wrote = call ("write_sequence", writtenLine);
        const auto& q = w.getSequenceTools()->getSequence();
        const std::vector<SequenceFile::Note> expected { { 45, 0, 480, (float) (110.0 / 127.0) }, { 45, 720, 240, (float) (64.0 / 127.0), 0.5f },
                                                         { 48, 960, 960, (float) (96.0 / 127.0) }, { 54, 1920, 480, (float) (88.0 / 127.0) },
                                                         { 52, 2880, 960, (float) (100.0 / 127.0) }, { 57, 3840, 3840, (float) (72.0 / 127.0) } };
        check ((bool) wrote["ok"] && q.notes == expected && q.lanes == std::vector<SequenceFile::Lane> { { 74, { { 0, 0.25f }, { 7680, 0.75f } } } }
                   && w.getSteps() == juce::StringArray { "Write 6 notes and 1 lane into \"Night Bass\", 8 beats in A minor" }
                   && wrote["outside_key"].size() == 1 && wrote["outside_key"][0] == juce::var ("F#3 at bar 1, beat 3"),
               "Ask Torq sequences: write_sequence writes the notes and lanes exactly as given (velocity n / 127, probability as given), as one step of the plan ("
                   + w.getSteps().joinIntoString (" | ") + ")");

        const auto suggestRefused = [&] (const juce::String& suggestion)
        {
            const auto r = call ("suggest_for_sequence", R"({"review": "Fine.", "suggestions": [ )" + suggestion + " ] }");
            return ! (bool) r["ok"] ? r["error"].toString() : juce::String();
        };
        const auto noNote = suggestRefused (R"({ "title": "x", "kind": "notes", "remove": [ { "pitch": 60, "start": 0 } ] })");
        const auto instrument = suggestRefused (R"({ "title": "x", "kind": "effect", "effect": "Torqwave Synth" })");
        const auto noParameter = suggestRefused (R"({ "title": "x", "kind": "automation", "parameter": "Warmth", "points": [ { "tick": 0, "value": 0.5 } ] })");
        const auto noKind = suggestRefused (R"({ "title": "x", "kind": "magic" })");
        check (noNote.contains ("no note to take out") && instrument.contains ("No effect plug-in is called \"Torqwave Synth\"") && noParameter.contains ("no parameter called \"Warmth\"")
                   && noKind.contains ("kind is") && w.getSequenceTools()->getSuggestions().empty(),
               "Ask Torq sequences: suggest_for_sequence refuses a note that isn't there, an instrument as an effect, a parameter the instrument hasn't and an unknown kind");

        const auto reviewed = call ("suggest_for_sequence", review);
        const auto& suggestions = w.getSequenceTools()->getSuggestions();
        check ((bool) reviewed["ok"] && suggestions.size() == 4 && w.getSequenceTools()->getReview().startsWith ("A steady line")
                   && suggestions[2].effect.name == "Chorus" && suggestions[3].parameter == "Filter 1 Cutoff" && ! suggestions[3].canApply() && suggestions[0].canApply()
                   && (bool) reviewed["suggestions"][2]["applies"] && ! (bool) reviewed["suggestions"][3]["applies"] && w.getSteps().size() == 1,
               "Ask Torq sequences: a review keeps its suggestions (notes, a lane, an effect, and automation as advice only, its parameter named as the instrument names it); they aren't steps of the plan");

        // Applied to the sequence one at a time, then all together.
        auto withVoice = q;
        withVoice.voice = std::make_shared<juce::XmlElement> ("TRACKTEMPLATE");
        juce::String error;
        const auto one = Ai::SequenceTools::apply (withVoice, { suggestions[0] }, error);
        const auto all = Ai::SequenceTools::apply (withVoice, suggestions, error);
        const auto again = one ? Ai::SequenceTools::apply (*one, { suggestions[0] }, error) : std::nullopt;
        const auto* slot = all && all->voice != nullptr && all->voice->getChildByName ("EFFECTS") != nullptr ? all->voice->getChildByName ("EFFECTS")->getChildByName ("SLOT") : nullptr;
        check (one && one->notes[3].pitch == 55 && one->notes[3].start == 1920 && juce::exactlyEqual (one->notes[3].velocity, q.notes[3].velocity) && one->lanes == q.lanes
                   && all && all->notes == one->notes && all->lanes.size() == 2 && all->lanes[1].controller == 1 && slot != nullptr
                   && slot->getChildByName ("PLUGIN") != nullptr && slot->getChildByName ("PLUGIN")->getStringAttribute ("name") == "Chorus"
                   && ! again && error.contains ("isn't there now"),
               "Ask Torq sequences: a suggestion applies by itself (F#3 to G3, its velocity kept exactly), all of them together (the note, the mod wheel lane, Chorus in the voice); applied twice, the note it changes isn't there");
    }

    /** Ask Torq on a sequence open in the Sequencer, from end to end through the window: a
        written sequence lands as one undo step and is exact; a review names the notes outside the
        key, and its suggestions apply one at a time and all together, each undone by the
        Sequencer's Undo (an effect in the voice too). */
    void testSequencerAsk()
    {
        const auto folder = juce::File::createTempFile ("ask-sequencer");
        folder.createDirectory();
        const auto file = folder.getChildFile (juce::String ("Night Bass") + SequenceFile::extension);
        // Its voice: the Torqwave Synth.
        auto line = bassLine();
        {
            SessionState s;
            s.sampleRate = 48000.0;
            const auto trackId = s.addInstrumentTrack().id;
            auto effects = std::make_unique<juce::XmlElement> ("EFFECTS");

            for (auto& d : BuiltInEffects::getDescriptions())
                if (d.fileOrIdentifier == BuiltInEffects::basicSynthId)
                    effects->createNewChildElement ("SLOT")->addChildElement (d.createXml().release());

            line.voice = std::shared_ptr<const juce::XmlElement> (SequenceFile::voiceOf (s, trackId, std::move (effects)).release());
        }

        SequenceFile::save (line, file);

        AudioEngine engine;
        SequencerView view (engine);
        view.openFile (file);
        TestUtils::runUntil ([&] { return view.getPlayer().getEffects().getInstrument (view.getTrackId()) != nullptr; }, 5000);
        const auto original = view.getDocument().get();

        MockServer server ({ { 200, "text/event-stream", toolCallAnswer ("a", "get_sequence", "{}") },
                             { 200, "text/event-stream", toolCallAnswer ("b", "write_sequence", writtenLine) },
                             { 200, "text/event-stream", textAnswer ("Wrote two bars.") },
                             { 200, "text/event-stream", toolCallAnswer ("c", "get_sequence", "{}") },
                             { 200, "text/event-stream", toolCallAnswer ("d", "suggest_for_sequence", review) },
                             { 200, "text/event-stream", textAnswer ("Four suggestions.") },
                             { 200, "text/event-stream", toolCallAnswer ("e", "suggest_for_sequence", review) },
                             { 200, "text/event-stream", textAnswer ("The same again.") } });

        Ai::AskPanel panel (settingsFor (server.url()),
                            [&view]
                            {
                                Ai::Workspace::Start s;
                                s.sequence = Ai::SequencerLink::contextFor (view, nullptr, 0.0, {});
                                s.plugins = BuiltInEffects::getDescriptions();
                                return std::optional<Ai::Workspace::Start> (std::move (s));
                            },
                            [&view] (Ai::Workspace& w, const juce::String& request) { return Ai::SequencerLink::applyPlan (view, w, request); });
        panel.applySuggestions = [&view] (Ai::Workspace& w, const std::vector<int>& which) { return Ai::SequencerLink::applySuggestions (view, w, which); };

        const auto button = [&panel] (const juce::String& name) -> juce::TextButton*
        {
            for (auto* c : panel.getChildren())
                if (auto* b = dynamic_cast<juce::TextButton*> (c); b != nullptr && b->getButtonText() == name)
                    return b;

            return nullptr;
        };

        // Writing: the plan, then Apply puts it in as one undo step, exactly.
        panel.getRequestBox().setText ("Two bars of bass in A minor");
        panel.ask();
        TestUtils::runUntil ([&] { return panel.getLog().contains ("Apply to make these changes"); }, 10000);
        const auto untouched = view.getDocument().get().notes == original.notes && ! view.getDocument().canUndo();
        button ("Apply")->triggerClick();
        const auto& doc = view.getDocument();
        TestUtils::runUntil ([&] { return doc.canUndo(); }, 5000);
        const std::vector<SequenceFile::Note> expected { { 45, 0, 480, (float) (110.0 / 127.0) }, { 45, 720, 240, (float) (64.0 / 127.0), 0.5f },
                                                         { 48, 960, 960, (float) (96.0 / 127.0) }, { 54, 1920, 480, (float) (88.0 / 127.0) },
                                                         { 52, 2880, 960, (float) (100.0 / 127.0) }, { 57, 3840, 3840, (float) (72.0 / 127.0) } };
        const auto written = doc.get().notes == expected && doc.get().lanes == std::vector<SequenceFile::Lane> { { 74, { { 0, 0.25f }, { 7680, 0.75f } } } }
                             && doc.get().name == "Night Bass" && doc.get().key == "A minor";
        const auto described = doc.getUndoDescription();
        view.undo();
        const auto oneStep = doc.get().notes == original.notes && doc.get().lanes.empty() && ! doc.canUndo();
        view.redo();
        const auto shown = view.getEditor().getKey().name() == "A minor";
        check (untouched && written && described == "Ask Torq: Two bars of bass in A minor" && oneStep && doc.get().notes == expected && shown
                   && panel.getLog().contains ("Applied, as one undo step."),
               "Ask Torq in the Sequencer: nothing changes until Apply; then the written notes and lane are in the Sequencer exactly as written, as one undo step (\""
                   + described + "\"), which Undo takes back whole and Redo puts back");

        // Reviewing: the notes outside the key named, the suggestions shown with Apply, automation as advice.
        const auto beforeReview = doc.get();
        panel.getRequestBox().setText ("How could this be better?");
        panel.ask();
        TestUtils::runUntil ([&] { return panel.getLog().contains ("Four suggestions."); }, 10000);
        TestUtils::runUntil ([&] { return panel.getNumSuggestions() == 4; }, 2000);
        const auto log = panel.getLog();
        check (log.contains ("Review: A steady line") && log.contains ("Outside A minor: F#3 at bar 1, beat 3.") && panel.getNumSuggestions() == 4
                   && button ("Apply All") != nullptr && button ("Apply All")->isVisible() && button ("Apply All")->isEnabled()
                   && ! button ("Apply")->isEnabled() && doc.get().notes == beforeReview.notes,
               "Ask Torq in the Sequencer: a review is shown with the notes outside the key named by the app, and its suggestions with Apply All; nothing changes yet");

        // One at a time: each is one undo step of its own.
        const auto chorusIn = [&]
        {
            const auto& chain = view.getPlayer().getEffects();

            for (int i = 0; i < chain.getNumSlots (view.getTrackId()); ++i)
                if (auto* p = chain.getPlugin (view.getTrackId(), i); p != nullptr && p->getName() == "Chorus")
                    return true;

            return false;
        };
        const auto steps = [&] { auto n = 0; for (auto& d = view.getDocument(); n < 300 && d.canUndo(); ++n) view.undo(); for (int i = 0; i < n; ++i) view.redo(); return n; };
        const auto stepsBefore = steps();
        panel.applySuggestion (0);
        const auto noteChanged = doc.get().notes[3].pitch == 55 && doc.get().notes[3].start == 1920 && doc.getUndoDescription() == "Ask Torq: F#3 to G3";
        panel.applySuggestion (1);
        const auto laneAdded = doc.get().lanes.size() == 2 && doc.get().lanes[1].controller == 1;
        panel.applySuggestion (2);
        TestUtils::runUntil (chorusIn, 5000);
        const auto effectAdded = chorusIn() && SequenceFile::toXml (doc.get())->toString().contains ("Chorus");
        panel.applySuggestion (3);
        const auto adviceOnly = ! panel.isSuggestionApplied (3) && panel.isSuggestionApplied (0) && panel.isSuggestionApplied (2) && ! button ("Apply All")->isEnabled();
        const auto stepsAfter = steps();
        TestUtils::runUntil (chorusIn, 5000);

        view.undo();
        TestUtils::runUntil ([&] { return ! chorusIn(); }, 5000);
        const auto effectUndone = ! chorusIn() && doc.get().lanes.size() == 2;
        view.undo();
        const auto laneUndone = doc.get().lanes == beforeReview.lanes && doc.get().notes[3].pitch == 55;
        view.undo();
        const auto noteUndone = doc.get().notes == beforeReview.notes;
        check (noteChanged && laneAdded && effectAdded && adviceOnly && stepsAfter == stepsBefore + 3 && effectUndone && laneUndone && noteUndone,
               "Ask Torq in the Sequencer: suggestions apply one at a time, an undo step each (" + juce::String (stepsAfter - stepsBefore)
                   + "): the note, the mod wheel lane, Chorus loaded into the voice; automation stays advice; Undo takes back each, Chorus too");

        // All together: one step, and one Undo takes it all back.
        panel.getRequestBox().setText ("And again");
        panel.ask();
        TestUtils::runUntil ([&] { return panel.getLog().contains ("The same again."); }, 10000);
        TestUtils::runUntil ([&] { return panel.getNumSuggestions() == 4; }, 2000);
        const auto beforeAll = steps();
        panel.applyAllSuggestions();
        TestUtils::runUntil (chorusIn, 5000);
        const auto allIn = doc.get().notes[3].pitch == 55 && doc.get().lanes.size() == 2 && chorusIn() && steps() == beforeAll + 1
                           && doc.getUndoDescription() == "Ask Torq: 3 suggestions" && ! panel.isSuggestionApplied (3) && panel.isSuggestionApplied (1);
        view.undo();
        TestUtils::runUntil ([&] { return ! chorusIn(); }, 5000);
        const auto allOut = doc.get().notes == beforeReview.notes && doc.get().lanes == beforeReview.lanes && ! chorusIn();
        view.redo();
        TestUtils::runUntil (chorusIn, 5000);
        const auto redone = chorusIn() && doc.get().lanes.size() == 2;
        check (allIn && allOut && redone,
               "Ask Torq in the Sequencer: Apply All puts the three suggestions in as one undo step (\"Ask Torq: 3 suggestions\"); one Undo takes them all back, Chorus too, and Redo brings them back");

        juce::MessageManager::getInstance()->runDispatchLoopUntil (50);
        folder.deleteRecursively();
    }

    //==============================================================================
    /** The Ask window from end to end: a request, a model that calls one tool and then answers,
        the plan shown, and Apply handing the working copy over. */
    void testAskPanel()
    {
        const auto chunk = [] (const juce::String& json) { return "data: " + json + "\n\n"; };
        const auto first = chunk (R"({"choices":[{"delta":{"reasoning_content":"Six dB up."}}]})")
                         + chunk (R"({"choices":[{"delta":{"tool_calls":[{"index":0,"id":"a","function":{"name":"edit","arguments":"{\"kind\": \"gain\", \"settings\": {\"db\": 6}}"}}]},"finish_reason":"tool_calls"}]})")
                         + "data: [DONE]\n\n";
        const auto second = chunk (R"({"choices":[{"delta":{"content":"Raised by 6 dB."},"finish_reason":"stop"}]})") + "data: [DONE]\n\n";
        MockServer server ({ { 200, "text/event-stream", first }, { 200, "text/event-stream", second } });

        const auto audio = tone (48000.0, 3);
        AudioDocument::Snapshot applied;
        juce::String appliedFor;

        Ai::AskPanel panel (settingsFor (server.url()),
                            [audio]
                            {
                                Ai::Workspace::Start s;
                                s.audio = audio;
                                s.sampleRate = 48000.0;
                                return std::optional<Ai::Workspace::Start> (std::move (s));
                            },
                            [&] (Ai::Workspace& w, const juce::String& request) { applied = w.getAudio(); appliedFor = request; return juce::String(); });

        const auto button = [&panel] (const juce::String& name) -> juce::TextButton*
        {
            for (auto* c : panel.getChildren())
                if (auto* b = dynamic_cast<juce::TextButton*> (c); b != nullptr && b->getButtonText() == name)
                    return b;

            return nullptr;
        };

        panel.getRequestBox().setText ("Make it 6 dB louder");
        panel.ask();
        TestUtils::runUntil ([&] { return panel.getLog().contains ("Apply to make these changes"); }, 10000);
        const auto log = panel.getLog();
        check (log.contains ("Thinking: Six dB up.") && log.contains ("> edit") && log.contains ("plan: Gain 6.00 dB, the whole file")
                   && log.contains ("Raised by 6 dB.") && log.contains ("1. Gain 6.00 dB, the whole file"),
               "Ask Torq: the window shows the thinking, the tool call, the answer and the plan");
        check (button ("Apply") != nullptr && button ("Apply")->isEnabled() && ! button ("Stop")->isEnabled() && applied == nullptr,
               "Ask Torq: Apply waits for the user");

        const auto requests = server.requests();
        check (requests.size() == 2 && parse (requests[1].body)["messages"].size() == 4
                   && parse (requests[1].body)["messages"][3]["role"] == juce::var ("tool")
                   && parse (requests[1].body)["messages"][2]["tool_calls"][0]["id"] == juce::var ("a"),
               "Ask Torq: the tool's result goes back to the model with the call it answers");

        button ("Apply")->triggerClick();
        TestUtils::runUntil ([&] { return applied != nullptr; }, 5000);
        check (applied != nullptr && appliedFor == "Make it 6 dB louder"
                   && std::abs (juce::Decibels::gainToDecibels (AudioEdits::peakLevel (*applied, 0, applied->getNumFrames())) + 6.0f) < 0.01f
                   && AudioEdits::peakLevel (*audio, 0, audio->getNumFrames()) < 0.26f,
               "Ask Torq: Apply hands over the edited copy; the original is untouched until then");
        check (panel.getLog().contains ("Applied, as one undo step.") && ! button ("Apply")->isEnabled(), "Ask Torq: once applied, the plan is gone");
    }

    /** The name in menus and titles carries the puppy: drawn with the app's fonts, it shows in colour. */
    void testName()
    {
        juce::Image image (juce::Image::ARGB, 300, 40, true);
        {
            juce::Graphics g (image);
            g.fillAll (juce::Colours::white);
            g.setColour (juce::Colours::black);
            g.setFont (juce::FontOptions (28.0f));
            g.drawText (Ai::askName(), image.getBounds(), juce::Justification::centredLeft);
        }

        auto coloured = 0;

        for (int y = 0; y < image.getHeight(); ++y)
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto c = image.getPixelAt (x, y);
                coloured += std::abs ((int) c.getRed() - (int) c.getBlue()) > 40 ? 1 : 0;
            }

        check (Ai::askName().endsWith (juce::String (juce::CharPointer_UTF8 ("\xf0\x9f\x90\xb6"))) && coloured > 20,
               "Ask Torq: the puppy in its name draws in colour (" + juce::String (coloured) + " coloured pixels)");
    }
}

void runAiTests()
{
    testSettings();
    testEndpoints();
    testEndpointsView();
    testReadingAnswers();
    testServer();
    testCancelling();
    testLookAndChange();
    testChannelSteps();
    testEffectsMenuSteps();
    testParameters();
    testPlugins();
    testCatalogueStore();
    testCatalogueWorker();
    testCatalogueTools();
    testSessionTools();
    testSessionChains();
    testChainOnEngine();
    testSessionComp();
    testSessionSections();
    testSessionNotes();
    testInstrumentPresets();
    testFactorySounds();
    testFactorySoundsOnEngine();
    testSequencePresetTools();
    testRealFactoryPresets();
    testKitOnEngine();
    testDrumMapTools();
    testSwarmGuitarParameters();
    testAskForFactoryThings();
    testCatalogueLearner();
    testAskPanel();
    testSequenceTools();
    testSequencerAsk();
    testName();
}
