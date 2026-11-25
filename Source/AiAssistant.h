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

        /** What the model is told it answered: the start of its answer. */
        juce::String said() const;

        /** The program's note of what came of it, in brackets, sent at the head of the request that
            follows: the plan's first steps and whether the user applied it. Empty when there was no plan. */
        juce::String after() const;
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

    /** How full the model's context is, for the window to show: the last request to the server as
        the server counted it (or an estimate from its length, four characters to a token, when the
        server sent no count), the answers' tokens so far, and the model's window when the server
        tells it. */
    struct Usage
    {
        int contextTokens = 0;      // the last request and its answer: what the conversation holds now
        int answerTokens = 0;       // every answer of this request, added up
        int window = 0;             // the model's context window, or 0 when the server doesn't say
        bool estimated = false;     // contextTokens is from lengths, not the server's count
        int rounds = 0;             // requests made to the server for this one

        /** Of the window, 0 to 1, or -1 when it isn't known. */
        double share() const noexcept   { return window > 0 ? (double) contextTokens / (double) window : -1.0; }

        /** The line the window shows. */
        juce::String text() const;
    };

    /** With the context this full (of a known window), a request is sent without the earlier turns. */
    constexpr double fullEnoughToForget = 0.7;

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
        std::function<void (const Usage&)> onUsage;      // after each answer from the server

        /** Whether a request first asks the server for the model's context window (once for each
            server and model). The tests turn it off: their made-up servers answer in a set order. */
        static bool askServerForWindow;

        /** The most rounds of tool calls in one request. */
        static constexpr int maxRounds = 60;

    private:
        struct Event
        {
            enum class Kind { text, reasoning, tool, done, usage } kind;
            juce::String a, b, c;
            Usage usage {};
        };

        void run() override;
        void handleAsyncUpdate() override;
        void post (Event);
        void count (const Reply&);          // adds an answer's tokens to the usage and reports it
        Usage usage;                        // the agent's thread only

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
    class AskPanel final : public juce::Component,
                           private juce::KeyListener
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

        /** The line under the log: how full the model's context is, and what is sent. */
        juce::String getStats() const                    { return stats.getText(); }

        /** Earlier requests, as the up and down arrows walk them in the request box: back one
            (-1) or forward one (+1). Forward past the newest brings back what was being typed.
            The requests are kept while the program runs, for every Ask Torq window. */
        void recall (int direction);

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

        bool keyPressed (const juce::KeyPress&, juce::Component*) override;
        void showStats (const Usage*);

        juce::Label banner, prompt, stats;
        Usage lastUsage;
        int recalled = -1;                  // which earlier request the box shows, or -1
        juce::String draft;                 // what was in the box before the arrows were used
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
