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

    /** Whether Apply makes the whole plan as one undo step: rack changes, instrument loads, track effects and renders
        are made after it, outside that step. */
    bool allInOneStep (const Workspace& w)
    {
        return ! w.rackChanged() && w.getInstrumentLoads().empty() && w.getVoiceLoads().empty() && w.getChainChanges().empty() && w.getRenders().empty()
               && w.getSynthChanges().empty();
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
    banner.setText ("Experimental. Describe an edit to the open file, to the session in the multitrack view, or (from the Sequencer's Ask Torq button) to "
                    "the sequence in the Sequencer. The model on your server (" + settings.model + ") "
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

        const auto oneStep = allInOneStep (agent->getWorkspace());

        if (const auto error = applyPlan (agent->getWorkspace(), lastRequest); error.isNotEmpty())
        {
            addLog ("\n" + error + "\n", Theme::danger);
            return;
        }

        addLog (oneStep ? "\nApplied, as one undo step.\n" : "\nApplied; the notes above say what isn't in its undo step.\n", Theme::good);
        planReady = false;

        if (suggestionButtons.isEmpty())   // suggestions stay to apply
            agent.reset();

        updateButtons();
    };
    discardButton.onClick = [this] { discard(); };
    applyAllButton.onClick = [this] { applyAllSuggestions(); };
    applyAllButton.setTooltip ("Applies every suggestion not yet applied, together as one undo step");

    for (auto* b : { &askButton, &stopButton, &applyButton, &discardButton })
        addAndMakeVisible (b);

    addChildComponent (applyAllButton);
    suggestionView.setViewedComponent (&suggestionRows, false);
    suggestionView.setScrollBarsShown (true, false);
    addChildComponent (suggestionView);

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
    applyAllButton.setBounds (bottom.removeFromLeft (100));
    r.removeFromBottom (8);
    r.removeFromTop (8);

    // The suggestions, under the log: a row each, up to about half the room.
    constexpr int rowHeight = 44;

    if (! suggestionButtons.isEmpty())
    {
        suggestionView.setBounds (r.removeFromBottom (juce::jmin (suggestionButtons.size() * rowHeight, r.getHeight() / 2)));
        r.removeFromBottom (8);
        const auto width = suggestionView.getMaximumVisibleWidth();
        suggestionRows.setSize (width, suggestionButtons.size() * rowHeight);

        for (int i = 0; i < suggestionButtons.size(); ++i)
        {
            auto row = juce::Rectangle<int> (0, i * rowHeight, width, rowHeight).reduced (0, 2);
            suggestionButtons[i]->setBounds (row.removeFromRight (90).withSizeKeepingCentre (90, 26));
            suggestionLabels[i]->setBounds (row.withTrimmedRight (8));
        }
    }

    log.setBounds (r);
}

void AskPanel::setSubject (const juce::String& text, const juce::String& example)
{
    prompt.setText (text, juce::dontSendNotification);
    requestBox.setTextToShowWhenEmpty (example, Theme::faintText);
}

void AskPanel::showSuggestions()
{
    suggestionLabels.clear();
    suggestionButtons.clear();
    applied.clear();

    const auto* tools = agent != nullptr ? agent->getWorkspace().getSequenceTools() : nullptr;

    if (tools != nullptr)
    {
        const auto beatsPerBar = tools->getSequence().beatsPerBar;

        for (auto& suggestion : tools->getSuggestions())
        {
            const auto index = suggestionButtons.size();
            auto* label = suggestionLabels.add (new juce::Label ({}, juce::String (index + 1) + ". " + suggestion.describe (beatsPerBar)));
            label->setJustificationType (juce::Justification::centredLeft);
            label->setFont (juce::FontOptions (13.0f));
            label->setMinimumHorizontalScale (1.0f);
            label->setTooltip (suggestion.describe (beatsPerBar));

            auto* button = suggestionButtons.add (new juce::TextButton (suggestion.canApply() ? "Apply" : "Advice"));
            button->setTooltip (suggestion.canApply() ? "Applies this suggestion by itself, as one undo step in the Sequencer"
                                                      : "A sequence can't keep instrument automation: draw it on the track once the sequence is in a session");
            button->onClick = [this, index] { applySuggestion (index); };
            applied.push_back (false);

            suggestionRows.addAndMakeVisible (label);
            suggestionRows.addAndMakeVisible (button);
        }
    }

    suggestionView.setVisible (! suggestionButtons.isEmpty());
    applyAllButton.setVisible (! suggestionButtons.isEmpty());
    resized();
}

void AskPanel::applySuggestion (int index)
{
    applyChosen ({ index });
}

void AskPanel::applyAllSuggestions()
{
    std::vector<int> left;

    for (int i = 0; i < (int) applied.size(); ++i)
        if (! applied[(size_t) i])
            left.push_back (i);

    applyChosen (left);
}

void AskPanel::applyChosen (const std::vector<int>& which)
{
    const auto* tools = agent != nullptr ? agent->getWorkspace().getSequenceTools() : nullptr;

    if (tools == nullptr || applySuggestions == nullptr || agent->isWorking())
        return;

    std::vector<int> chosen;

    for (auto i : which)
        if (i >= 0 && i < (int) applied.size() && ! applied[(size_t) i] && tools->getSuggestions()[(size_t) i].canApply())
            chosen.push_back (i);

    if (chosen.empty())
        return;

    if (const auto error = applySuggestions (agent->getWorkspace(), chosen); error.isNotEmpty())
    {
        addLog ("\n" + error + "\n", Theme::danger);
        return;
    }

    juce::StringArray numbers;

    for (auto i : chosen)
    {
        applied[(size_t) i] = true;
        numbers.add (juce::String (i + 1));
    }

    addLog ("\nApplied " + (chosen.size() == 1 ? "suggestion " : juce::String ("suggestions ")) + numbers.joinIntoString (", ")
                + " as one undo step: the Sequencer's Undo takes it back.\n", Theme::good);
    updateButtons();
}

void AskPanel::updateButtons()
{
    const auto working = agent != nullptr && agent->isWorking();
    askButton.setEnabled (! working);
    stopButton.setEnabled (working);
    applyButton.setEnabled (! working && planReady);
    discardButton.setEnabled (! working && (planReady || ! suggestionButtons.isEmpty()));
    requestBox.setReadOnly (working);

    auto anyLeft = false;

    for (int i = 0; i < suggestionButtons.size(); ++i)
    {
        const auto* tools = agent != nullptr ? agent->getWorkspace().getSequenceTools() : nullptr;
        const auto applies = tools != nullptr && applySuggestions != nullptr && tools->getSuggestions()[(size_t) i].canApply();
        suggestionButtons[i]->setEnabled (! working && applies && ! applied[(size_t) i]);

        if (applies && applied[(size_t) i])
            suggestionButtons[i]->setButtonText ("Applied");

        anyLeft = anyLeft || (applies && ! applied[(size_t) i]);
    }

    applyAllButton.setEnabled (! working && anyLeft);
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
    showSuggestions();
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
        addLog ("Open an audio file, a session in the multitrack view, or a sequence in the Sequencer, first.\n", Theme::danger);
        return;
    }

    agent.reset();
    planReady = false;
    showSuggestions();
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

    // A review of a sequence: what it says, the notes outside the key, and its suggestions.
    if (const auto* tools = w.getSequenceTools(); tools != nullptr && tools->getReview().isNotEmpty())
    {
        addLog ("\n\nReview: ", Theme::textStrong);
        addLog (tools->getReview() + "\n", Theme::text);

        if (const auto outside = tools->getOutsideKey(); ! outside.isEmpty())
            addLog ("Outside " + tools->getSequence().key + ": " + outside.joinIntoString ("; ") + ".\n", Theme::warn);

        if (tools->getSuggestions().empty())
            addLog ("No suggestions.\n", Theme::dimText);
        else
            addLog (applySuggestions != nullptr ? "Apply a suggestion below by itself, or Apply All for every one as one undo step. The Sequencer's Undo takes each back.\n"
                                                : "The suggestions are below.\n",
                    Theme::dimText);
    }

    showSuggestions();

    if (! planReady)
    {
        if (suggestionButtons.isEmpty())
            addLog ("\n\nNo changes were planned.\n", Theme::dimText);
    }
    else
    {
        addLog (allInOneStep (w) ? "\n\nThe plan, applied as one undo step:\n" : "\n\nThe plan:\n", Theme::textStrong);

        for (int i = 0; i < steps.size(); ++i)
            addLog (juce::String (i + 1) + ". " + steps[i] + "\n", Theme::text);

        if (w.rackChanged())
            addLog ("The effects rack is changed too; that isn't part of the undo step.\n", Theme::warn);

        if (! w.getInstrumentLoads().empty() || ! w.getVoiceLoads().empty())
            addLog ("Instruments are loaded once the plan is applied; that isn't part of the undo step.\n", Theme::warn);

        if (! w.getChainChanges().empty())
            addLog ("Track effects are changed once the plan is applied; that isn't part of the undo step.\n", Theme::warn);

        if (! w.getSynthChanges().empty())
            addLog ("Torqwave Synth's patch and tables are changed once the plan is applied; that isn't part of the undo step.\n", Theme::warn);

        if (! w.getRenders().empty())
            addLog ("Freezes and bounces render once the plan is applied, each as its own undo step.\n", Theme::warn);

        addLog ("Apply to make these changes, or Discard.\n", Theme::dimText);
    }

    updateButtons();
}

//==============================================================================
SettingsView::SettingsView (std::function<Settings()> get, std::function<void (const Settings&)> set)
    : juce::Thread ("AI settings"), getSettings (std::move (get)), setSettings (std::move (set))
{
    endpointLabel.setText ("Endpoint", juce::dontSendNotification);
    nameLabel.setText ("Name", juce::dontSendNotification);
    urlLabel.setText ("Server", juce::dontSendNotification);
    keyLabel.setText ("Key", juce::dontSendNotification);
    modelLabel.setText ("Model", juce::dontSendNotification);

    for (auto* l : { &endpointLabel, &nameLabel, &urlLabel, &keyLabel, &modelLabel, &keyNote, &status })
        addAndMakeVisible (l);

    // The list of servers: choose one to use, add one, delete the one shown.
    endpointBox.setTextWhenNothingSelected ("Add a server");
    endpointBox.setTextWhenNoChoicesAvailable ("Add a server");
    endpointBox.onChange = [this]
    {
        if (const auto index = endpointBox.getSelectedItemIndex(); index >= 0)
        {
            saveFromFields();
            auto s = getSettings();
            s.choose (index);
            setSettings (s);
            showEndpoint();
            noteSaved ("Ask Torq uses " + endpointBox.getText());
        }
    };
    addButton.onClick = [this] { addEndpoint(); };
    deleteButton.onClick = [this] { deleteEndpoint(); };
    nameBox.setTextToShowWhenEmpty ("a name for this server", Theme::faintText);
    nameBox.onTextChange = [this] { saveFromFields(); };   // kept as it is typed

    for (auto* c : std::initializer_list<juce::Component*> { &endpointBox, &addButton, &deleteButton, &nameBox })
        addAndMakeVisible (c);

    keyNote.setColour (juce::Label::textColourId, Theme::dimText);
    status.setColour (juce::Label::textColourId, Theme::dimText);
    status.setJustificationType (juce::Justification::topLeft);

    urlBox.setTextToShowWhenEmpty ("http://localhost:11434/v1", Theme::faintText);
    urlBox.onTextChange = [this] { saveFromFields(); };    // kept as it is typed
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

    modelBox.onChange = [this]
    {
        auto changed = getSettings();
        const auto model = modelBox.getText().trim();

        if (model == changed.model)
            return;

        changed.model = model;
        setSettings (changed);
        noteSaved ("the model " + model);
    };

    refreshButton.onClick = [this] { startJob (false); };
    testButton.onClick = [this] { startJob (true); };

    for (auto* c : std::initializer_list<juce::Component*> { &modelBox, &saveKeyButton, &forgetKeyButton, &refreshButton, &testButton })
        addAndMakeVisible (c);

    keyNote.setText (AiKeychain::canStore() ? "Kept in " + juce::String (AiKeychain::whereKept()) + ", never in Torqwave's settings."
                                            : "Read from " + juce::String (AiKeychain::whereKept()) + ".",
                     juce::dontSendNotification);
    status.setText ("Any server that speaks the OpenAI-compatible API: Ollama, LM Studio, llama.cpp, vLLM and the like.", juce::dontSendNotification);
    savedNote.setColour (juce::Label::textColourId, Theme::dimText);
    savedNote.setText ("Changes here are saved as you make them.", juce::dontSendNotification);
    addAndMakeVisible (savedNote);
    showEndpoint();
}

void SettingsView::showEndpoint()
{
    const auto s = getSettings();
    const auto list = s.listed();
    endpointBox.clear (juce::dontSendNotification);

    for (size_t i = 0; i < list.size(); ++i)
        endpointBox.addItem (list[i].name.isNotEmpty() ? list[i].name : "Server " + juce::String ((int) i + 1), (int) i + 1);

    if (juce::isPositiveAndBelow (s.current, (int) list.size()))
        endpointBox.setSelectedItemIndex (s.current, juce::dontSendNotification);

    const auto any = ! list.empty();
    const auto* shown = any && juce::isPositiveAndBelow (s.current, (int) list.size()) ? &list[(size_t) s.current] : nullptr;
    nameBox.setText (shown != nullptr ? shown->name : juce::String(), false);
    urlBox.setText (s.serverUrl, false);
    modelBox.clear (juce::dontSendNotification);

    if (s.model.isNotEmpty())
    {
        modelBox.addItem (s.model, 1);
        modelBox.setSelectedId (1, juce::dontSendNotification);
    }

    deleteButton.setEnabled (any);
    nameBox.setEnabled (any);
}

void SettingsView::addEndpoint()
{
    saveFromFields();
    auto s = getSettings();
    s.addEndpoint();
    setSettings (s);
    showEndpoint();
    noteSaved ("a new endpoint");
    status.setText ("Give the new server's address, then list its models.", juce::dontSendNotification);
    urlBox.grabKeyboardFocus();
}

void SettingsView::deleteEndpoint()
{
    auto s = getSettings();
    const auto list = s.listed();

    if (! juce::isPositiveAndBelow (s.current, (int) list.size()))
        return;

    // Its key goes too, unless another server in the list has the same address.
    const auto account = s.keyAccount();
    const auto name = list[(size_t) s.current].name;
    s.removeEndpoint (s.current);
    const auto shared = std::any_of (s.endpoints.begin(), s.endpoints.end(), [&] (auto& e)
    {
        Settings other;
        other.serverUrl = e.serverUrl;
        return other.keyAccount() == account;
    });

    if (account.isNotEmpty() && ! shared && AiKeychain::canStore())
        AiKeychain::erase (account.toStdString());

    setSettings (s);
    showEndpoint();
    noteSaved (name.quoted() + " deleted");
    status.setText (name.quoted() + " is deleted" + (account.isNotEmpty() && ! shared && AiKeychain::canStore() ? ", with its key." : "."),
                    juce::dontSendNotification);
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

    auto endpoint = row();
    endpointLabel.setBounds (endpoint.removeFromLeft (70));
    deleteButton.setBounds (endpoint.removeFromRight (70));
    endpoint.removeFromRight (6);
    addButton.setBounds (endpoint.removeFromRight (60));
    endpoint.removeFromRight (6);
    endpointBox.setBounds (endpoint);

    auto name = row();
    nameLabel.setBounds (name.removeFromLeft (70));
    nameBox.setBounds (name);

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
    savedNote.setBounds (r.removeFromBottom (20).withTrimmedLeft (70));
    status.setBounds (r.withTrimmedLeft (70));
}

void SettingsView::noteSaved (const juce::String& what)
{
    savedNote.setText ("Saved at " + juce::Time::getCurrentTime().toString (false, true, true, true) + ": " + what + ".", juce::dontSendNotification);
}

void SettingsView::saveFromFields()
{
    auto s = getSettings();
    const auto oldUrl = s.serverUrl;
    s.serverUrl = urlBox.getText().trim();

    // The fields edit the endpoint in use; with none yet, typing an address makes the first.
    s.endpoints = s.listed();
    auto renamed = false;

    if (juce::isPositiveAndBelow (s.current, (int) s.endpoints.size()))
    {
        const auto name = nameBox.getText().trim();
        auto& e = s.endpoints[(size_t) s.current];

        renamed = name.isNotEmpty() && name != e.name;

        if (renamed)
            e.name = name;
    }

    const auto first = endpointBox.getNumItems() == 0 && ! s.endpoints.empty();
    setSettings (s);
    const auto shownName = juce::isPositiveAndBelow (s.current, (int) s.endpoints.size()) && s.endpoints[(size_t) s.current].name.isNotEmpty()
                               ? s.endpoints[(size_t) s.current].name : juce::String ("the server");

    if (renamed)
        noteSaved ("the name " + shownName);
    else if (s.serverUrl != oldUrl || first)
        noteSaved (shownName + "'s address");

    // Only the list changes with a new name, so the fields being typed in are left alone.
    if (first)
        showEndpoint();
    else if (renamed)
    {
        endpointBox.changeItemText (s.current + 1, s.endpoints[(size_t) s.current].name);
        endpointBox.setSelectedId (s.current + 1, juce::dontSendNotification);   // shows the new text
    }
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
