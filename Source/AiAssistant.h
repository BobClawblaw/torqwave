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

    /** A request made earlier in the window's conversation and what came of it, given to the model
        before the next request so a follow-up ("now louder", "undo that idea") has something to follow. */
    struct Turn
    {
        enum class Outcome { none, pending, applied, discarded };

        juce::String request, answer;
        juce::StringArray plan;             // the plan's steps, if it made one
        Outcome outcome = Outcome::none;

        /** What the model is told it answered: the answer, the plan, and whether the user applied it. */
        juce::String said() const;
    };

    /** What the model is told about the program it is in. Every request gets one line, the name
        and version. A request that asks about the program (asksAboutRelease: "what is new", a
        version, the changelog) also gets what the release brought, so the question has an answer;
        other requests don't carry it. Written by hand for each release (releaseNotesFor names the
        version it was written for; a test holds it to the build's version). */
    juce::String aboutThisRelease (bool withNotes);
    juce::String releaseNotesFor();
    bool asksAboutRelease (const juce::String& request);

    /** The model's context is small and the tools' descriptions fill much of it, so little of the
        conversation is sent: the last few turns, the start of each answer and of each plan. */
    constexpr int maxTurnsSent = 4, maxAnswerSent = 600, maxPlanStepsSent = 6;

    /** Runs the conversation on a background thread: the model's answer, the tools it calls, their
        results back to it, until it answers without a tool or runs out of rounds. What happens is
        reported on the message thread, through the callbacks, as it happens. */
    class Agent final : private juce::Thread,
                        private juce::AsyncUpdater
    {
    public:
        Agent (Settings, juce::String key, Workspace::Start, juce::String request, std::vector<Turn> earlier = {});
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
        std::vector<Turn> earlier;
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

        /** Applies suggestions from a review of a sequence (by their index in it), as one undo
            step; returns an error, or empty. Without it, suggestions are shown with no Apply. */
        std::function<juce::String (Workspace&, const std::vector<int>& which)> applySuggestions;

        /** What the window asks about, and an example request ("What should change?" by default). */
        void setSubject (const juce::String& prompt, const juce::String& example);

        /** The conversation so far: each finished request, its answer and what became of its plan. */
        const std::vector<Turn>& getTurns() const noexcept   { return turns; }

        /** Forgets the conversation and empties the log, as the New Chat button does. */
        void newChat();

        /** For the tests: the request box, and the log as shown. */
        juce::TextEditor& getRequestBox() noexcept      { return requestBox; }
        juce::String getLog() const                     { return log.getText(); }
        void ask();

        /** For the tests, as the buttons do: the suggestions shown, Apply on one, and Apply All. */
        int getNumSuggestions() const noexcept          { return suggestionButtons.size(); }
        bool isSuggestionApplied (int index) const      { return index >= 0 && index < (int) applied.size() && applied[(size_t) index]; }
        void applySuggestion (int index);
        void applyAllSuggestions();

    private:
        void addLog (const juce::String& text, juce::Colour);
        void finish (const juce::String& answer, const juce::String& error);
        void discard();
        void updateButtons();
        void showSuggestions();             // the review's suggestions as rows, each with Apply
        void applyChosen (const std::vector<int>&);

        Settings settings;
        std::function<std::optional<Workspace::Start>()> makeStart;
        std::function<juce::String (Workspace&, const juce::String&)> applyPlan;

        juce::Label banner, prompt;
        juce::TextEditor requestBox, log;
        juce::TextButton askButton { "Ask" }, stopButton { "Stop" }, newChatButton { "New Chat" }, applyButton { "Apply" }, discardButton { "Discard" };
        juce::TextButton applyAllButton { "Apply All" };
        juce::Component suggestionRows;
        juce::OwnedArray<juce::Label> suggestionLabels;
        juce::OwnedArray<juce::TextButton> suggestionButtons;
        juce::Viewport suggestionView;      // shows suggestionRows; gone before them
        std::vector<bool> applied;
        std::unique_ptr<Agent> agent;
        juce::String lastRequest;
        std::vector<Turn> turns;
        void settle (Turn::Outcome);        // what became of the last turn's plan, if it is still pending
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

        static constexpr int preferredHeight = 340;

    private:
        void run() override;
        void startJob (bool test);
        void saveFromFields();
        void showEndpoint();            // the list, and the fields of the endpoint in use
        void addEndpoint();
        void deleteEndpoint();
        juce::String keyFor (const Settings&) const;

        std::function<Settings()> getSettings;
        std::function<void (const Settings&)> setSettings;

        juce::Label endpointLabel, nameLabel, urlLabel, keyLabel, modelLabel, keyNote, status;
        juce::Label savedNote;                       // "Saved at 07:53:12: Qwen", after each change kept
        void noteSaved (const juce::String& what);
        juce::TextEditor nameBox, urlBox, keyBox;
        juce::ComboBox endpointBox, modelBox;
        juce::TextButton addButton { "Add" }, deleteButton { "Delete" };
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
