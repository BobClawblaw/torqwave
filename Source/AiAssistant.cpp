#include "AiAssistant.h"
#include "AiKeychain.h"
#include "Theme.h"

namespace Ai
{
namespace
{
    juce::var message (const juce::String& role, const juce::String& content)
    {
        auto m = std::make_unique<juce::DynamicObject>();
        m->setProperty ("role", role);
        m->setProperty ("content", content);
        return juce::var (m.release());
    }

    juce::String savedKey (const Settings& s)
    {
        const auto key = AiKeychain::load (s.keyAccount().toStdString());
        return key ? juce::String::fromUTF8 (key->c_str(), (int) key->size()) : juce::String();
    }

    /** A tool call as a line for the log: its name and its arguments, compactly. */
    juce::String callText (const juce::String& tool, const juce::String& arguments)
    {
        juce::var args;
        juce::JSON::parse (arguments, args);
        const auto compact = args.isObject() && args.getDynamicObject()->getProperties().isEmpty() ? juce::String()
                                                                                                    : " " + juce::JSON::toString (args, true);
        return tool + compact.substring (0, 300);
    }
}

//==============================================================================
Agent::Agent (Settings s, juce::String key, Workspace::Start start, juce::String r)
    : juce::Thread ("Ask Torq"), client (std::move (s), std::move (key)), workspace (std::move (start)), request (std::move (r))
{
    workspace.shouldStop = [this] { return threadShouldExit(); };
}

Agent::~Agent()
{
    cancel();
    stopThread (10000);
    cancelPendingUpdate();
}

void Agent::start()
{
    working = true;
    startThread();
}

void Agent::cancel()
{
    signalThreadShouldExit();
    client.cancel();
}

void Agent::post (Event e)
{
    {
        const juce::ScopedLock sl (lock);
        events.push_back (std::move (e));
    }

    triggerAsyncUpdate();
}

void Agent::run()
{
    juce::Array<juce::var> messages { message ("system", workspace.systemMessage()), message ("user", request) };
    const auto tools = workspace.tools();
    std::map<juce::String, int> callsMade;   // each tool and its arguments, how often called

    for (int round = 0; round < maxRounds; ++round)
    {
        const auto reply = client.chat (messages, tools, [this] (const juce::String& text, bool reasoning)
        {
            post ({ reasoning ? Event::Kind::reasoning : Event::Kind::text, text, {}, {} });
        });

        if (reply.error.isNotEmpty() || threadShouldExit())
        {
            post ({ Event::Kind::done, reply.content, threadShouldExit() ? juce::String ("Cancelled.") : reply.error, {} });
            return;
        }

        // The answer goes back into the conversation as it came, tool calls and all.
        auto said = std::make_unique<juce::DynamicObject>();
        said->setProperty ("role", "assistant");
        said->setProperty ("content", reply.content);

        if (! reply.toolCalls.empty())
        {
            juce::Array<juce::var> calls;

            for (auto& c : reply.toolCalls)
            {
                auto function = std::make_unique<juce::DynamicObject>();
                function->setProperty ("name", c.name);
                function->setProperty ("arguments", c.arguments.isEmpty() ? juce::String ("{}") : c.arguments);
                auto call = std::make_unique<juce::DynamicObject>();
                call->setProperty ("id", c.id);
                call->setProperty ("type", "function");
                call->setProperty ("function", juce::var (function.release()));
                calls.add (juce::var (call.release()));
            }

            said->setProperty ("tool_calls", calls);
        }

        messages.add (juce::var (said.release()));

        if (reply.toolCalls.empty())
        {
            post ({ Event::Kind::done, reply.content, {}, {} });
            return;
        }

        for (auto& c : reply.toolCalls)
        {
            if (threadShouldExit())
            {
                post ({ Event::Kind::done, {}, "Cancelled.", {} });
                return;
            }

            // A model in a loop asks the same thing again and again. Counted with the plan's length,
            // so a call after a change is a new one: the third time with nothing changed in between,
            // it is told so instead.
            const auto times = ++callsMade[juce::String (workspace.getSteps().size()) + " " + c.name + " " + c.arguments.trim()];
            const auto result = times > 2
                                    ? juce::String ("{\"ok\": false, \"error\": \"This exact call was already made twice and its result has not changed. Use what it returned, try something else, or finish.\"}")
                                    : workspace.run (c.name, c.arguments);
            post ({ Event::Kind::tool, c.name, c.arguments, result });

            auto answer = std::make_unique<juce::DynamicObject>();
            answer->setProperty ("role", "tool");
            answer->setProperty ("tool_call_id", c.id);
            answer->setProperty ("content", result);
            messages.add (juce::var (answer.release()));
        }
    }

    // Out of rounds: the model gets one last turn, without tools, to say what it planned, and
    // the plan so far is offered as it stands rather than thrown away.
    post ({ Event::Kind::text, "\n\n[It used all " + juce::String (maxRounds) + " rounds of tool calls, so it was asked to finish. The plan may be incomplete.]\n", {}, {} });
    messages.add (message ("user", "You have used every round of tool calls; no more can be made. Say in a few sentences what the plan "
                                   "you made does, and what of the request it leaves undone."));

    const auto reply = client.chat (messages, juce::var (juce::Array<juce::var>()), [this] (const juce::String& text, bool reasoning)
    {
        post ({ reasoning ? Event::Kind::reasoning : Event::Kind::text, text, {}, {} });
    });

    post ({ Event::Kind::done, reply.content, threadShouldExit() ? juce::String ("Cancelled.") : juce::String(), {} });
}

void Agent::handleAsyncUpdate()
{
    std::vector<Event> taken;

    {
        const juce::ScopedLock sl (lock);
        taken.swap (events);
    }

    for (auto& e : taken)
    {
        if (e.kind == Event::Kind::text || e.kind == Event::Kind::reasoning)
        {
            if (onText != nullptr)
                onText (e.a, e.kind == Event::Kind::reasoning);
        }
        else if (e.kind == Event::Kind::tool)
        {
            if (onTool != nullptr)
                onTool (e.a, e.b, e.c);
        }
        else
        {
            working = false;

            if (onDone != nullptr)
                onDone (e.a, e.b);
        }
    }
}

//==============================================================================
AskPanel::AskPanel (Settings s, std::function<std::optional<Workspace::Start>()> start,
                    std::function<juce::String (Workspace&, const juce::String&)> apply)
    : settings (std::move (s)), makeStart (std::move (start)), applyPlan (std::move (apply))
{
    banner.setText ("Experimental. Describe an edit to the open file. The model on your server (" + settings.model + ") "
                    "works on a copy: nothing changes until you read its plan and press Apply, and Apply is one undo step.",
                    juce::dontSendNotification);
    banner.setColour (juce::Label::textColourId, Theme::warn);
    banner.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (banner);

    prompt.setText ("What should change?", juce::dontSendNotification);
    addAndMakeVisible (prompt);

    requestBox.setMultiLine (true, true);
    requestBox.setReturnKeyStartsNewLine (false);
    requestBox.setTextToShowWhenEmpty ("e.g. trim the silence at both ends, bring it to -16 LUFS and fade out the last second", Theme::faintText);
    requestBox.onReturnKey = [this] { ask(); };
    addAndMakeVisible (requestBox);

    log.setMultiLine (true, true);
    log.setReadOnly (true);
    log.setCaretVisible (false);
    log.setScrollbarsShown (true);
    addAndMakeVisible (log);

    askButton.onClick = [this] { ask(); };
    stopButton.onClick = [this] { if (agent != nullptr) agent->cancel(); };
    applyButton.onClick = [this]
    {
        if (agent == nullptr || ! planReady)
            return;

        if (const auto error = applyPlan (agent->getWorkspace(), lastRequest); error.isNotEmpty())
        {
            addLog ("\n" + error + "\n", Theme::danger);
            return;
        }

        addLog ("\nApplied, as one undo step.\n", Theme::good);
        agent.reset();
        planReady = false;
        updateButtons();
    };
    discardButton.onClick = [this] { discard(); };

    for (auto* b : { &askButton, &stopButton, &applyButton, &discardButton })
        addAndMakeVisible (b);

    updateButtons();
    setSize (620, 560);
}

AskPanel::~AskPanel()
{
    agent.reset();
}

void AskPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::base);
}

void AskPanel::resized()
{
    auto r = getLocalBounds().reduced (12);
    banner.setBounds (r.removeFromTop (52));
    prompt.setBounds (r.removeFromTop (22));
    requestBox.setBounds (r.removeFromTop (64));
    r.removeFromTop (8);

    auto top = r.removeFromTop (28);
    askButton.setBounds (top.removeFromLeft (90));
    top.removeFromLeft (8);
    stopButton.setBounds (top.removeFromLeft (90));

    auto bottom = r.removeFromBottom (28);
    discardButton.setBounds (bottom.removeFromRight (100));
    bottom.removeFromRight (8);
    applyButton.setBounds (bottom.removeFromRight (100));
    r.removeFromBottom (8);
    r.removeFromTop (8);
    log.setBounds (r);
}

void AskPanel::updateButtons()
{
    const auto working = agent != nullptr && agent->isWorking();
    askButton.setEnabled (! working);
    stopButton.setEnabled (working);
    applyButton.setEnabled (! working && planReady);
    discardButton.setEnabled (! working && planReady);
    requestBox.setReadOnly (working);
}

void AskPanel::addLog (const juce::String& text, juce::Colour colour)
{
    log.moveCaretToEnd();
    log.setColour (juce::TextEditor::textColourId, colour);
    log.insertTextAtCaret (text);
}

void AskPanel::discard()
{
    if (agent != nullptr && planReady)
        addLog ("\nDiscarded. Nothing was changed.\n", Theme::dimText);

    agent.reset();
    planReady = false;
    updateButtons();
}

void AskPanel::ask()
{
    if (agent != nullptr && agent->isWorking())
        return;

    const auto text = requestBox.getText().trim();

    if (text.isEmpty())
        return;

    auto start = makeStart();

    if (! start)
    {
        addLog ("Open an audio file first. In 0.17 the model works on audio files, not sessions.\n", Theme::danger);
        return;
    }

    agent.reset();
    planReady = false;
    reasoningShown = false;
    lastRequest = text;
    log.clear();
    addLog ("You: " + text + "\n\n", Theme::textStrong);

    agent = std::make_unique<Agent> (settings, savedKey (settings), std::move (*start), text);
    agent->onText = [this] (const juce::String& t, bool reasoning)
    {
        if (reasoning && ! reasoningShown)
        {
            addLog ("Thinking: ", Theme::faintText);
            reasoningShown = true;
        }
        else if (! reasoning && reasoningShown)
        {
            addLog ("\n", Theme::faintText);
            reasoningShown = false;
        }

        addLog (t, reasoning ? Theme::faintText : Theme::text);
    };
    agent->onTool = [this] (const juce::String& tool, const juce::String& arguments, const juce::String& result)
    {
        if (reasoningShown)
        {
            addLog ("\n", Theme::faintText);
            reasoningShown = false;
        }

        juce::var r;
        juce::JSON::parse (result, r);
        addLog ("\n> " + callText (tool, arguments) + "\n", Theme::dimText);

        if (! (bool) r["ok"])
            addLog ("  refused: " + r["error"].toString() + "\n", Theme::warn);
        else if (const auto step = r["step"].toString(); step.isNotEmpty())
            addLog ("  plan: " + step + "\n", Theme::accentLight);
    };
    agent->onDone = [this] (const juce::String& answer, const juce::String& error) { finish (answer, error); };
    agent->start();
    updateButtons();
}

void AskPanel::finish (const juce::String&, const juce::String& error)
{
    if (error.isNotEmpty())
    {
        // The agent is kept until the next request: this runs inside its own callback.
        addLog ("\n\n" + error + " Nothing was changed.\n", Theme::danger);
        planReady = false;
        updateButtons();
        return;
    }

    auto& w = agent->getWorkspace();
    const auto& steps = w.getSteps();
    planReady = ! steps.isEmpty();

    if (! planReady)
        addLog ("\n\nNo changes were planned.\n", Theme::dimText);
    else
    {
        addLog ("\n\nThe plan, applied as one undo step:\n", Theme::textStrong);

        for (int i = 0; i < steps.size(); ++i)
            addLog (juce::String (i + 1) + ". " + steps[i] + "\n", Theme::text);

        if (w.rackChanged())
            addLog ("The effects rack is changed too; that isn't part of the undo step.\n", Theme::warn);

        addLog ("Apply to make these changes, or Discard.\n", Theme::dimText);
    }

    updateButtons();
}

//==============================================================================
SettingsView::SettingsView (std::function<Settings()> get, std::function<void (const Settings&)> set)
    : juce::Thread ("AI settings"), getSettings (std::move (get)), setSettings (std::move (set))
{
    const auto s = getSettings();

    urlLabel.setText ("Server", juce::dontSendNotification);
    keyLabel.setText ("Key", juce::dontSendNotification);
    modelLabel.setText ("Model", juce::dontSendNotification);

    for (auto* l : { &urlLabel, &keyLabel, &modelLabel, &keyNote, &status })
        addAndMakeVisible (l);

    keyNote.setColour (juce::Label::textColourId, Theme::dimText);
    status.setColour (juce::Label::textColourId, Theme::dimText);
    status.setJustificationType (juce::Justification::topLeft);

    urlBox.setText (s.serverUrl, false);
    urlBox.setTextToShowWhenEmpty ("http://localhost:11434/v1", Theme::faintText);
    urlBox.onReturnKey = [this] { saveFromFields(); };
    urlBox.onFocusLost = [this] { saveFromFields(); };
    addAndMakeVisible (urlBox);

    keyBox.setPasswordCharacter ((juce::juce_wchar) 0x2022);
    keyBox.setTextToShowWhenEmpty ("only if the server asks for one", Theme::faintText);
    keyBox.setEnabled (AiKeychain::canStore());
    addAndMakeVisible (keyBox);

    saveKeyButton.onClick = [this]
    {
        saveFromFields();
        const auto account = getSettings().keyAccount();

        if (account.isEmpty() || keyBox.getText().isEmpty())
            return;

        const auto ok = AiKeychain::store (account.toStdString(), keyBox.getText().toStdString());
        keyBox.clear();
        status.setText (ok ? "The key is saved in " + juce::String (AiKeychain::whereKept()) + "." : "The key couldn't be saved.",
                        juce::dontSendNotification);
        resized();
    };
    forgetKeyButton.onClick = [this]
    {
        if (const auto account = getSettings().keyAccount(); account.isNotEmpty())
            AiKeychain::erase (account.toStdString());

        status.setText ("No key is kept for this server.", juce::dontSendNotification);
        resized();
    };
    saveKeyButton.setEnabled (AiKeychain::canStore());
    forgetKeyButton.setEnabled (AiKeychain::canStore());

    modelBox.setTextWhenNothingSelected ("List the server's models");
    modelBox.setEditableText (true);

    if (s.model.isNotEmpty())
    {
        modelBox.addItem (s.model, 1);
        modelBox.setSelectedId (1, juce::dontSendNotification);
    }

    modelBox.onChange = [this]
    {
        auto changed = getSettings();
        changed.model = modelBox.getText().trim();
        setSettings (changed);
    };

    refreshButton.onClick = [this] { startJob (false); };
    testButton.onClick = [this] { startJob (true); };

    for (auto* c : std::initializer_list<juce::Component*> { &modelBox, &saveKeyButton, &forgetKeyButton, &refreshButton, &testButton })
        addAndMakeVisible (c);

    keyNote.setText (AiKeychain::canStore() ? "Kept in " + juce::String (AiKeychain::whereKept()) + ", never in Torqwave's settings."
                                            : "Read from " + juce::String (AiKeychain::whereKept()) + ".",
                     juce::dontSendNotification);
    status.setText ("Any server that speaks the OpenAI-compatible API: Ollama, LM Studio, llama.cpp, vLLM and the like.", juce::dontSendNotification);
}

SettingsView::~SettingsView()
{
    {
        const juce::ScopedLock sl (lock);

        if (jobClient != nullptr)
            jobClient->cancel();
    }

    stopThread (10000);
}

void SettingsView::resized()
{
    auto r = getLocalBounds();
    const auto row = [&r]
    {
        auto line = r.removeFromTop (26);
        r.removeFromTop (6);
        return line;
    };

    auto url = row();
    urlLabel.setBounds (url.removeFromLeft (70));
    urlBox.setBounds (url);

    auto key = row();
    keyLabel.setBounds (key.removeFromLeft (70));
    forgetKeyButton.setBounds (key.removeFromRight (100));
    key.removeFromRight (6);
    saveKeyButton.setBounds (key.removeFromRight (90));
    key.removeFromRight (6);
    keyBox.setBounds (key);

    auto note = r.removeFromTop (20);
    note.removeFromLeft (70);
    keyNote.setBounds (note);
    r.removeFromTop (6);

    auto model = row();
    modelLabel.setBounds (model.removeFromLeft (70));
    testButton.setBounds (model.removeFromRight (70));
    model.removeFromRight (6);
    refreshButton.setBounds (model.removeFromRight (110));
    model.removeFromRight (6);
    modelBox.setBounds (model);

    r.removeFromTop (4);
    status.setBounds (r.withTrimmedLeft (70));
}

void SettingsView::saveFromFields()
{
    auto s = getSettings();
    s.serverUrl = urlBox.getText().trim();
    setSettings (s);
}

juce::String SettingsView::keyFor (const Settings& s) const
{
    return savedKey (s);
}

void SettingsView::startJob (bool test)
{
    if (isThreadRunning())
        return;

    saveFromFields();
    jobSettings = getSettings();

    if (jobSettings.baseUrl().isEmpty())
    {
        status.setText ("Give the server's address first.", juce::dontSendNotification);
        return;
    }

    if (test && jobSettings.model.isEmpty())
    {
        status.setText ("Choose a model first: List Models asks the server for them.", juce::dontSendNotification);
        return;
    }

    jobIsTest = test;
    jobKey = keyFor (jobSettings);
    status.setText (test ? "Asking " + jobSettings.model + "..." : "Asking " + jobSettings.baseUrl() + " for its models...", juce::dontSendNotification);
    startThread();
}

void SettingsView::run()
{
    {
        const juce::ScopedLock sl (lock);
        jobClient = std::make_unique<Client> (jobSettings, jobKey);
    }

    juce::String text, error;
    juce::StringArray models;
    const auto started = juce::Time::getMillisecondCounterHiRes();

    if (jobIsTest)
    {
        const auto r = jobClient->chat (juce::Array<juce::var> { message ("user", "Reply with exactly these words: Torqwave is connected.") }, {});
        const auto seconds = (juce::Time::getMillisecondCounterHiRes() - started) / 1000.0;
        error = r.error;
        text = "It answered in " + juce::String (seconds, 1) + " s: " + r.content.trim().substring (0, 200).quoted();
    }
    else
    {
        models = jobClient->listModels (error);
        text = juce::String (models.size()) + (models.size() == 1 ? " model." : " models.");
    }

    {
        const juce::ScopedLock sl (lock);
        jobClient.reset();
    }

    juce::MessageManager::callAsync ([safe = self, text, error, models]
    {
        if (safe == nullptr)
            return;

        safe->status.setText (error.isNotEmpty() ? error : text, juce::dontSendNotification);

        if (! models.isEmpty())
        {
            const auto current = safe->getSettings().model;
            safe->modelBox.clear (juce::dontSendNotification);
            safe->modelBox.addItemList (models, 1);
            const auto index = models.indexOf (current);

            if (index >= 0)
                safe->modelBox.setSelectedItemIndex (index, juce::dontSendNotification);
            else
                safe->modelBox.setSelectedItemIndex (0, juce::sendNotification);
        }
    });
}
}
