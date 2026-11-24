#pragma once

#include <JuceHeader.h>

/** Experimental: a language model on a server that speaks the OpenAI-compatible API (Chat
    Completions with tools), meant for a model you run yourself, on this computer or your
    network: Ollama, LM Studio, llama.cpp, vLLM, TensorFold and the like.

    Torqwave ships no model. Requests carry text only, except one: with Let the model hear clips
    on (off by default), Decrypt sends the note it is matching, once, for a description. */
namespace Ai
{
    /** A server kept in Settings > AI's list: a name to choose it by, its URL and the model used on it. */
    struct Endpoint
    {
        juce::String name, serverUrl, model;

        bool operator== (const Endpoint& o) const noexcept { return name == o.name && serverUrl == o.serverUrl && model == o.model; }
    };

    /** Settings > AI. Off by default. The key is never here: it lives in AiKeychain. */
    struct Settings
    {
        bool enabled = false;
        juce::String serverUrl;       // the server in use: its base URL, e.g. "http://localhost:11434/v1"
        juce::String model;           // as the server names it in /v1/models

        /** The servers kept to choose from, and which is in use: serverUrl and model are its. A
            setting saved before there was a list comes back as a list of that one server. */
        std::vector<Endpoint> endpoints;
        int current = 0;

        /** Decrypt may send a clip's audio to the server, for a model that hears to describe it.
            Off by default; with it off, nothing but text is ever sent. */
        bool hearClips = false;

        /** Makes endpoint `index` the one in use. */
        void choose (int index);

        /** Adds a server to the list, named (or "Server N"), and makes it the one in use. */
        void addEndpoint (const juce::String& name = {}, const juce::String& url = {});

        /** Removes endpoint `index`; the one after it (or the last) is then in use, or none. */
        void removeEndpoint (int index);

        /** The list with serverUrl and model written into the endpoint in use (a first one made for them if there is none). */
        std::vector<Endpoint> listed() const;

        bool isOn() const             { return enabled && baseUrl().isNotEmpty(); }

        /** The base URL requests go to, without a trailing slash. "host:port" alone gets "http://" and "/v1". */
        juce::String baseUrl() const;

        /** Where this server's key is kept (AiKeychain's account). */
        juce::String keyAccount() const   { return baseUrl(); }

        static Settings load (const juce::PropertySet&);
        void save (juce::PropertySet&) const;
    };

    struct ToolCall
    {
        juce::String id, name, arguments;   // arguments: JSON text as the model wrote it
    };

    struct Reply
    {
        juce::String content;               // the answer's text
        juce::String reasoning;             // a reasoning model's thinking, when the server sends it apart
        std::vector<ToolCall> toolCalls;
        juce::String finishReason;
        juce::String error;                 // empty when the request worked
        int status = 0;                     // the HTTP status
        int promptTokens = -1;              // what the server counted for the request (its "usage"), or -1 when it sent no count
        int completionTokens = -1;          // and for its answer
        int requestChars = 0;               // the request's length as sent, for an estimate when there is no count
    };

    /** Talks to one server. Each call blocks: call from a background thread. cancel() from any
        thread stops the call in progress, which then returns the error "Cancelled.". */
    class Client
    {
    public:
        Client (Settings, juce::String key);

        /** The models the server offers (/v1/models), sorted; nothing with the error set when it fails. */
        juce::StringArray listModels (juce::String& error);

        /** The model's context window in tokens, as the server's /v1/models entry for it says
            (servers name it differently: context_length, max_context_length, max_model_len,
            context_window, n_ctx, or meta.n_ctx_train), or 0 when it doesn't say. Asked once for
            each server and model while the program runs. */
        int contextWindow();

        /** The window in a /v1/models answer's entry for a model, or 0. Public for the tests. */
        static int contextWindowFrom (const juce::String& modelsBody, const juce::String& model);

        /** One chat completion, streamed: messages and tools as the API takes them (tools may be
            void). onText hears the answer's text as it comes (reasoning true for thinking). */
        Reply chat (const juce::var& messages, const juce::var& tools,
                    const std::function<void (const juce::String& text, bool reasoning)>& onText = {});

        void cancel();

        /** Fields added to each chat request as they are, beside the model and messages (such as
            chat_template_kwargs, which many local servers read). Empty to begin with. */
        juce::NamedValueSet requestExtras;

        /** How long to wait for the server to connect or to send the next part of an answer. */
        static constexpr int timeoutMs = 180000;

        /** Reads a streamed answer (server-sent events), or a whole one when the server didn't
            stream. Public for the tests. */
        static Reply readAnswer (juce::InputStream&, const std::function<void (const juce::String&, bool)>& onText,
                                 const std::function<bool()>& cancelled);

        /** The error message in an API error body, or the body's start. */
        static juce::String errorFrom (const juce::String& body, int status);

    private:
        std::unique_ptr<juce::WebInputStream> open (const juce::String& path, const juce::String* postJson, juce::String& error, int& status);

        Settings settings;
        juce::String key;
        juce::CriticalSection lock;
        juce::WebInputStream* current = nullptr;   // guarded by lock
        std::atomic<bool> cancelled { false };
    };
}
