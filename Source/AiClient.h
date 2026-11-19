#pragma once

#include <JuceHeader.h>

/** Experimental: a language model on a server that speaks the OpenAI-compatible API (Chat
    Completions with tools), meant for a model you run yourself, on this computer or your
    network: Ollama, LM Studio, llama.cpp, vLLM, TensorFold and the like.

    Torqwave ships no model and sends no audio: requests carry text only. */
namespace Ai
{
    /** Settings > AI. Off by default. The key is never here: it lives in AiKeychain. */
    struct Settings
    {
        bool enabled = false;
        juce::String serverUrl;       // the server's base URL, e.g. "http://localhost:11434/v1"
        juce::String model;           // as the server names it in /v1/models

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
    };

    /** Talks to one server. Each call blocks: call from a background thread. cancel() from any
        thread stops the call in progress, which then returns the error "Cancelled.". */
    class Client
    {
    public:
        Client (Settings, juce::String key);

        /** The models the server offers (/v1/models), sorted; nothing with the error set when it fails. */
        juce::StringArray listModels (juce::String& error);

        /** One chat completion, streamed: messages and tools as the API takes them (tools may be
            void). onText hears the answer's text as it comes (reasoning true for thinking). */
        Reply chat (const juce::var& messages, const juce::var& tools,
                    const std::function<void (const juce::String& text, bool reasoning)>& onText = {});

        void cancel();

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
