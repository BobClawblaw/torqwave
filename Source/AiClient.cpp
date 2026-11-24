#include "AiClient.h"

namespace Ai
{
namespace
{
    constexpr const char* enabledKey = "aiEnabled";
    constexpr const char* serverUrlKey = "aiServerUrl";
    constexpr const char* modelKey = "aiModel";
    constexpr const char* endpointsKey = "aiEndpoints";
    constexpr const char* currentKey = "aiEndpoint";
    constexpr const char* hearClipsKey = "aiHearClips";

    juce::var parse (const juce::String& json)
    {
        juce::var v;
        return juce::JSON::parse (json, v).wasOk() ? v : juce::var();
    }
}

//==============================================================================
juce::String Settings::baseUrl() const
{
    auto url = serverUrl.trim();

    while (url.endsWithChar ('/'))
        url = url.dropLastCharacters (1);

    if (url.isEmpty())
        return {};

    if (! url.containsIgnoreCase ("://"))
        url = "http://" + url;

    // "http://host:port" alone: the API's usual place.
    if (! url.fromFirstOccurrenceOf ("://", false, false).containsChar ('/'))
        url << "/v1";

    return url;
}

std::vector<Endpoint> Settings::listed() const
{
    auto list = endpoints;

    if (list.empty() && (serverUrl.isNotEmpty() || model.isNotEmpty()))
        list.push_back ({ "Server 1", {}, {} });

    if (juce::isPositiveAndBelow (current, (int) list.size()))
    {
        list[(size_t) current].serverUrl = serverUrl;
        list[(size_t) current].model = model;
    }

    return list;
}

void Settings::choose (int index)
{
    endpoints = listed();

    if (! juce::isPositiveAndBelow (index, (int) endpoints.size()))
        return;

    current = index;
    serverUrl = endpoints[(size_t) index].serverUrl;
    model = endpoints[(size_t) index].model;
}

void Settings::addEndpoint (const juce::String& name, const juce::String& url)
{
    endpoints = listed();
    auto n = (int) endpoints.size() + 1;
    auto taken = [this] (const juce::String& x) { return std::any_of (endpoints.begin(), endpoints.end(), [&] (auto& e) { return e.name == x; }); };

    while (name.isEmpty() && taken ("Server " + juce::String (n)))
        ++n;

    endpoints.push_back ({ name.isNotEmpty() ? name : "Server " + juce::String (n), url, {} });
    current = -1;   // the one in use was written back above: nothing more to keep from the fields
    choose ((int) endpoints.size() - 1);
}

void Settings::removeEndpoint (int index)
{
    endpoints = listed();

    if (! juce::isPositiveAndBelow (index, (int) endpoints.size()))
        return;

    endpoints.erase (endpoints.begin() + index);

    if (endpoints.empty())
    {
        current = 0;
        serverUrl = model = {};
        return;
    }

    // The one in use stays in use; if it went, the one after it (or the last) takes its place.
    const auto next = index < current ? current - 1 : juce::jmin (current, (int) endpoints.size() - 1);
    current = -1;
    choose (next);
}

Settings Settings::load (const juce::PropertySet& p)
{
    Settings s;
    s.enabled = p.getBoolValue (enabledKey, false);
    s.serverUrl = p.getValue (serverUrlKey);
    s.model = p.getValue (modelKey);

    if (const auto list = juce::JSON::parse (p.getValue (endpointsKey)); list.isArray())
        for (auto& e : *list.getArray())
            s.endpoints.push_back ({ e["name"].toString(), e["url"].toString(), e["model"].toString() });

    s.current = juce::jlimit (0, juce::jmax (0, (int) s.endpoints.size() - 1), p.getIntValue (currentKey, 0));
    s.hearClips = p.getBoolValue (hearClipsKey, false);

    // Saved before the list: that server is the list. Otherwise the one in use is the list's.
    if (s.endpoints.empty())
        s.endpoints = s.listed();
    else
        s.choose (s.current);

    return s;
}

void Settings::save (juce::PropertySet& p) const
{
    p.setValue (enabledKey, enabled);
    p.setValue (serverUrlKey, serverUrl);
    p.setValue (modelKey, model);

    juce::Array<juce::var> list;

    for (auto& e : listed())
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("name", e.name);
        o->setProperty ("url", e.serverUrl);
        o->setProperty ("model", e.model);
        list.add (juce::var (o));
    }

    p.setValue (endpointsKey, juce::JSON::toString (juce::var (list), true));
    p.setValue (currentKey, current);
    p.setValue (hearClipsKey, hearClips);
}

//==============================================================================
Client::Client (Settings s, juce::String k) : settings (std::move (s)), key (std::move (k)) {}

void Client::cancel()
{
    cancelled = true;
    const juce::ScopedLock sl (lock);

    if (current != nullptr)
        current->cancel();
}

std::unique_ptr<juce::WebInputStream> Client::open (const juce::String& path, const juce::String* postJson, juce::String& error, int& status)
{
    auto url = juce::URL (settings.baseUrl() + path);

    if (postJson != nullptr)
        url = url.withPOSTData (*postJson);

    auto stream = std::make_unique<juce::WebInputStream> (url, postJson != nullptr);
    juce::String headers = "Accept: application/json, text/event-stream\r\n";

    if (postJson != nullptr)
        headers << "Content-Type: application/json\r\n";

    if (key.isNotEmpty())
        headers << "Authorization: Bearer " << key << "\r\n";

    stream->withExtraHeaders (headers).withConnectionTimeout (timeoutMs);

    {
        const juce::ScopedLock sl (lock);

        if (cancelled)
        {
            error = "Cancelled.";
            return nullptr;
        }

        current = stream.get();
    }

    const auto connected = stream->connect (nullptr);
    status = stream->getStatusCode();

    if (cancelled)
    {
        error = "Cancelled.";
    }
    else if (! connected || status == 0)
    {
        error = "Couldn't reach " + settings.baseUrl() + ".";
    }
    else if (status >= 400)
    {
        error = errorFrom (stream->readEntireStreamAsString(), status);
    }

    if (error.isNotEmpty())
    {
        const juce::ScopedLock sl (lock);
        current = nullptr;
        return nullptr;
    }

    return stream;
}

juce::String Client::errorFrom (const juce::String& body, int status)
{
    const auto v = parse (body);
    auto message = v["error"]["message"].toString();

    if (message.isEmpty())
        message = v["error"].isString() ? v["error"].toString() : v["detail"].toString();

    if (message.isEmpty())
        message = body.trim().substring (0, 300);

    return "The server answered " + juce::String (status) + (message.isNotEmpty() ? ": " + message : juce::String ("."));
}

juce::StringArray Client::listModels (juce::String& error)
{
    cancelled = false;
    int status = 0;
    auto stream = open ("/models", nullptr, error, status);

    if (stream == nullptr)
        return {};

    const auto body = stream->readEntireStreamAsString();

    {
        const juce::ScopedLock sl (lock);
        current = nullptr;
    }

    juce::StringArray names;
    const auto answer = parse (body);

    if (auto* list = answer["data"].getArray())
        for (auto& m : *list)
            if (const auto id = m["id"].toString(); id.isNotEmpty())
                names.addIfNotAlreadyThere (id);

    if (names.isEmpty())
        error = "The server listed no models.";

    names.sortNatural();
    return names;
}

int Client::contextWindowFrom (const juce::String& modelsBody, const juce::String& model)
{
    const auto answer = parse (modelsBody);

    if (auto* list = answer["data"].getArray())
        for (auto& m : *list)
            if (m["id"].toString() == model)
            {
                for (auto* field : { "context_length", "max_context_length", "max_model_len", "context_window", "n_ctx" })
                    if (const auto n = (int) m[field]; n > 0)
                        return n;

                return juce::jmax (0, (int) m["meta"]["n_ctx_train"]);
            }

    return 0;
}

int Client::contextWindow()
{
    static juce::CriticalSection knownLock;
    static std::map<juce::String, int> known;   // by server and model
    const auto name = settings.baseUrl() + " " + settings.model;

    {
        const juce::ScopedLock sl (knownLock);

        if (const auto found = known.find (name); found != known.end())
            return found->second;
    }

    cancelled = false;
    juce::String error;
    int status = 0;
    auto stream = open ("/models", nullptr, error, status);

    if (stream == nullptr)
        return 0;   // not kept: asked again next time

    const auto window = contextWindowFrom (stream->readEntireStreamAsString(), settings.model);

    {
        const juce::ScopedLock sl (lock);
        current = nullptr;
    }

    const juce::ScopedLock sl (knownLock);
    known[name] = window;
    return window;
}

Reply Client::readAnswer (juce::InputStream& in, const std::function<void (const juce::String&, bool)>& onText,
                                  const std::function<bool()>& isCancelled)
{
    Reply r;

    // The server's count of the request and the answer, where it sends one (with a streamed answer,
    // in a last event with no choices).
    const auto takeUsage = [&r] (const juce::var& from)
    {
        if (const auto& usage = from["usage"]; usage.isObject())
        {
            if (usage.hasProperty ("prompt_tokens"))
                r.promptTokens = (int) usage["prompt_tokens"];

            if (usage.hasProperty ("completion_tokens"))
                r.completionTokens = (int) usage["completion_tokens"];
        }
    };

    std::map<int, ToolCall> calls;   // by the index the stream gives each

    const auto take = [&] (const juce::var& choice, bool whole)
    {
        const auto& part = whole ? choice["message"] : choice["delta"];

        for (auto* field : { "reasoning_content", "reasoning" })
            if (const auto text = part[field].toString(); text.isNotEmpty())
            {
                r.reasoning << text;

                if (onText != nullptr)
                    onText (text, true);

                break;
            }

        if (const auto text = part["content"].toString(); text.isNotEmpty())
        {
            r.content << text;

            if (onText != nullptr)
                onText (text, false);
        }

        if (auto* list = part["tool_calls"].getArray())
            for (int i = 0; i < list->size(); ++i)
            {
                const auto& c = list->getReference (i);
                const auto index = whole || ! c.hasProperty ("index") ? i : (int) c["index"];
                auto& call = calls[index];

                if (const auto id = c["id"].toString(); id.isNotEmpty())
                    call.id = id;

                const auto& f = c["function"];

                if (const auto name = f["name"].toString(); name.isNotEmpty())
                    call.name = whole ? name : call.name + name;

                const auto& args = f["arguments"];
                call.arguments << (args.isString() ? args.toString() : juce::JSON::toString (args, true));
            }

        if (const auto reason = choice["finish_reason"].toString(); reason.isNotEmpty())
            r.finishReason = reason;
    };

    juce::String unstreamed;

    while (! in.isExhausted())
    {
        if (isCancelled != nullptr && isCancelled())
        {
            r.error = "Cancelled.";
            return r;
        }

        const auto line = in.readNextLine().trimEnd();

        if (line.isEmpty() || line.startsWithChar (':'))
            continue;

        if (! line.startsWith ("data:"))
        {
            // Not a stream: the server sent the whole answer as one body.
            unstreamed << line << "\n" << in.readEntireStreamAsString();
            break;
        }

        const auto data = line.substring (5).trim();

        if (data == "[DONE]")
            break;

        const auto event = parse (data);

        if (event.hasProperty ("error"))
        {
            r.error = errorFrom (data, 200);
            return r;
        }

        takeUsage (event);

        if (auto* choices = event["choices"].getArray(); choices != nullptr && ! choices->isEmpty())
            take (choices->getReference (0), false);
    }

    if (unstreamed.isNotEmpty())
    {
        const auto whole = parse (unstreamed);
        takeUsage (whole);

        if (whole.hasProperty ("error"))
            r.error = errorFrom (unstreamed, 200);
        else if (auto* choices = whole["choices"].getArray(); choices != nullptr && ! choices->isEmpty())
            take (choices->getReference (0), true);
        else
            r.error = "The server's answer wasn't a chat completion.";
    }

    for (auto& [index, call] : calls)
    {
        if (call.id.isEmpty())
            call.id = "call_" + juce::String (index);

        r.toolCalls.push_back (call);
    }

    if (r.error.isEmpty() && r.content.isEmpty() && r.toolCalls.empty())
        r.error = r.finishReason == "length" ? "The model ran out of room before it answered."
                                             : "The server's answer was empty.";

    return r;
}

Reply Client::chat (const juce::var& messages, const juce::var& tools,
                            const std::function<void (const juce::String&, bool)>& onText)
{
    cancelled = false;

    const auto bodyOf = [&] (bool askForUsage)
    {
        auto request = std::make_unique<juce::DynamicObject>();
        request->setProperty ("model", settings.model);
        request->setProperty ("messages", messages);
        request->setProperty ("stream", true);

        // Asks for the token counts with the streamed answer.
        if (askForUsage)
        {
            auto options = std::make_unique<juce::DynamicObject>();
            options->setProperty ("include_usage", true);
            request->setProperty ("stream_options", juce::var (options.release()));
        }

        if (tools.isArray() && tools.size() > 0)
            request->setProperty ("tools", tools);

        for (auto& extra : requestExtras)
            request->setProperty (extra.name, extra.value);

        return juce::JSON::toString (juce::var (request.release()), true);
    };

    Reply r;

    // A server that refuses the request for counts by name (a 400 or 422 that names the field) is asked again without it.
    for (const auto askForUsage : { true, false })
    {
        const auto body = bodyOf (askForUsage);
        r = {};
        auto stream = open ("/chat/completions", &body, r.error, r.status);

        if (stream != nullptr)
        {
            const auto status = r.status;
            r = readAnswer (*stream, onText, [this] { return cancelled.load(); });
            r.status = status;
        }

        r.requestChars = body.length();

        {
            const juce::ScopedLock sl (lock);
            current = nullptr;
        }

        const auto refusedByName = (r.status == 400 || r.status == 422) && (r.error.contains ("stream_options") || r.error.contains ("include_usage"));

        if (stream != nullptr || cancelled || ! refusedByName)
            break;
    }

    if (cancelled)
        r.error = "Cancelled.";

    return r;
}
}
