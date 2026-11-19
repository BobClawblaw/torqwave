#include <JuceHeader.h>
#include "../Source/AiAssistant.h"
#include "../Source/AiCatalogue.h"
#include "../Source/PluginCatalogue.h"
#include "../Source/PluginScanner.h"
#include "../Source/AiClient.h"
#include "../Source/AiWorkspace.h"
#include "../Source/AudioEdits.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/Loudness.h"
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
        check (! (bool) call ("add_plugin", R"({"plugin": "Basic Synth"})")["ok"], "AI plug-ins: an instrument isn't put in the effect chain");

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
                   && ! catalogue.getUnderstanding (builtIn ("Basic Synth")).has_value() && ! catalogue.hasCurrentFacts (builtIn ("Basic Synth")),
               "plug-in catalogue: Learn All reads and describes every built-in effect, and leaves instruments for later (" + p.message + ")");
        check ((int) requests.size() == effects.size() && requests[0].body.contains ("parameterCount") && ! requests[0].body.contains ("\"tools\""),
               "plug-in catalogue: the model is sent each plug-in's facts, with no tools");
        check (catalogue.saveIfNeeded() && PluginCatalogue (temp.getFile()).getNumUnderstood() == effects.size(), "plug-in catalogue: what was learnt is on disk");

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
    testReadingAnswers();
    testServer();
    testCancelling();
    testLookAndChange();
    testParameters();
    testPlugins();
    testCatalogueStore();
    testCatalogueWorker();
    testCatalogueTools();
    testCatalogueLearner();
    testAskPanel();
    testName();
}
