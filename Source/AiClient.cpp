#include "AiClient.h"

namespace Ai
{
namespace
{
    constexpr const char* enabledKey = "aiEnabled";
    constexpr const char* serverUrlKey = "aiServerUrl";
    constexpr const char* modelKey = "aiModel";

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

Settings Settings::load (const juce::PropertySet& p)
{
    Settings s;
    s.enabled = p.getBoolValue (enabledKey, false);
    s.serverUrl = p.getValue (serverUrlKey);
    s.model = p.getValue (modelKey);
    return s;
}

void Settings::save (juce::PropertySet& p) const
{
    p.setValue (enabledKey, enabled);
    p.setValue (serverUrlKey, serverUrl);
    p.setValue (modelKey, model);
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

Reply Client::readAnswer (juce::InputStream& in, const std::function<void (const juce::String&, bool)>& onText,
                                  const std::function<bool()>& isCancelled)
{
    Reply r;
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

        if (auto* choices = event["choices"].getArray(); choices != nullptr && ! choices->isEmpty())
            take (choices->getReference (0), false);
    }

    if (unstreamed.isNotEmpty())
    {
        const auto whole = parse (unstreamed);

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

    auto request = std::make_unique<juce::DynamicObject>();
    request->setProperty ("model", settings.model);
    request->setProperty ("messages", messages);
    request->setProperty ("stream", true);

    if (tools.isArray() && tools.size() > 0)
        request->setProperty ("tools", tools);

    const auto body = juce::JSON::toString (juce::var (request.release()), true);
    Reply r;
    auto stream = open ("/chat/completions", &body, r.error, r.status);

    if (stream != nullptr)
    {
        const auto status = r.status;
        r = readAnswer (*stream, onText, [this] { return cancelled.load(); });
        r.status = status;
    }

    const juce::ScopedLock sl (lock);
    current = nullptr;

    if (cancelled)
        r.error = "Cancelled.";

    return r;
}
}
