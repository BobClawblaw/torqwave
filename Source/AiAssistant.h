#pragma once

#include <JuceHeader.h>
#include <map>
#include "AiClient.h"
#include "AiWorkspace.h"

/** Experimental: Ask Torq. A sentence goes to the language model named in Settings > AI,
    which works the tools (AiWorkspace) on a copy of the file; the plan it makes is shown, and
    applied only when the user says so, as one undo step. */
namespace Ai
{
    /** The feature's name as menus and titles show it: "Ask Torq" and the puppy, Torqwave's mascot. */
    inline juce::String askName()    { return juce::String (juce::CharPointer_UTF8 ("Ask Torq \xf0\x9f\x90\xb6")); }

    /** Runs the conversation on a background thread: the model's answer, the tools it calls, their
        results back to it, until it answers without a tool or runs out of rounds. What happens is
        reported on the message thread, through the callbacks, as it happens. */
    class Agent final : private juce::Thread,
                        private juce::AsyncUpdater
    {
    public:
        Agent (Settings, juce::String key, Workspace::Start, juce::String request);
        ~Agent() override;

        void start();

        /** Stops the request in progress; done() follows with the error "Cancelled.". */
        void cancel();

        bool isWorking() const noexcept                   { return working; }

        /** The working copy, read after done(). */
        Workspace& getWorkspace() noexcept                { return workspace; }

        std::function<void (const juce::String& text, bool reasoning)> onText;
        std::function<void (const juce::String& tool, const juce::String& arguments, const juce::String& result)> onTool;
        std::function<void (const juce::String& answer, const juce::String& error)> onDone;

        /** The most rounds of tool calls in one request. */
        static constexpr int maxRounds = 60;

    private:
        struct Event
        {
            enum class Kind { text, reasoning, tool, done } kind;
            juce::String a, b, c;
        };

        void run() override;
        void handleAsyncUpdate() override;
        void post (Event);

        Client client;
        Workspace workspace;
        juce::String request;
        std::atomic<bool> working { false };

        juce::CriticalSection lock;
        std::vector<Event> events;   // guarded by lock
    };

    //==============================================================================
    /** The Ask Torq window's content: the request, what the model is doing, the plan, and
        Apply or Discard. Made with what the plan starts from; apply is called with the finished
        working copy when the user applies it. */
    class AskPanel final : public juce::Component
    {
    public:
        /** start makes a fresh copy of the file and rack for each request. */
        AskPanel (Settings, std::function<std::optional<Workspace::Start>()> start,
                  std::function<juce::String (Workspace&, const juce::String& request)> apply);
        ~AskPanel() override;

        void paint (juce::Graphics&) override;
        void resized() override;

        /** For the tests: the request box, and the log as shown. */
        juce::TextEditor& getRequestBox() noexcept      { return requestBox; }
        juce::String getLog() const                     { return log.getText(); }
        void ask();

    private:
        void addLog (const juce::String& text, juce::Colour);
        void finish (const juce::String& answer, const juce::String& error);
        void discard();
        void updateButtons();

        Settings settings;
        std::function<std::optional<Workspace::Start>()> makeStart;
        std::function<juce::String (Workspace&, const juce::String&)> applyPlan;

        juce::Label banner, prompt;
        juce::TextEditor requestBox, log;
        juce::TextButton askButton { "Ask" }, stopButton { "Stop" }, applyButton { "Apply" }, discardButton { "Discard" };
        std::unique_ptr<Agent> agent;
        juce::String lastRequest;
        bool reasoningShown = false, planReady = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AskPanel)
    };

    //==============================================================================
    /** Settings > AI: the server, its key, the model, and a test. Changes are saved at once. */
    class SettingsView final : public juce::Component,
                               private juce::Thread
    {
    public:
        SettingsView (std::function<Settings()> get, std::function<void (const Settings&)> set);
        ~SettingsView() override;

        void resized() override;

        static constexpr int preferredHeight = 250;

    private:
        void run() override;
        void startJob (bool test);
        void saveFromFields();
        juce::String keyFor (const Settings&) const;

        std::function<Settings()> getSettings;
        std::function<void (const Settings&)> setSettings;

        juce::Label urlLabel, keyLabel, modelLabel, keyNote, status;
        juce::TextEditor urlBox, keyBox;
        juce::ComboBox modelBox;
        juce::TextButton saveKeyButton { "Save Key" }, forgetKeyButton { "Forget Key" }, refreshButton { "List Models" }, testButton { "Test" };

        // The background job: listing models, or a test request.
        juce::CriticalSection lock;
        bool jobIsTest = false;
        Settings jobSettings;
        juce::String jobKey;
        std::unique_ptr<Client> jobClient;   // guarded by lock
        juce::Component::SafePointer<SettingsView> self { this };   // made here, on the message thread

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsView)
    };
}
