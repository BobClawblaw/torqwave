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
namespace
{
    /** The requests made in any Ask Torq window while the program runs, oldest first. */
    juce::StringArray& asked()
    {
        static juce::StringArray requests;
        return requests;
    }

    juce::String thousands (int n)
    {
        auto digits = juce::String (n);

        for (int i = digits.length() - 3; i > 0; i -= 3)
            digits = digits.substring (0, i) + "," + digits.substring (i);

        return digits;
    }
}

juce::String Usage::text() const
{
    juce::String t;
    t << "Context: " << (estimated ? "about " : "") << thousands (contextTokens);

    if (window > 0)
        t << " of " << thousands (window) << " tokens (" << juce::roundToInt (100.0 * share()) << " %)";
    else
        t << " tokens (the server doesn't say the model's limit)";

    if (estimated)
        t << ", estimated: the server sent no count";

    t << ". Answers: " << thousands (answerTokens) << " tokens in " << rounds << (rounds == 1 ? " round" : " rounds")
      << ". Sent with a request: the last " << maxTurnsSent << " before it, " << maxAnswerSent << " characters of each answer.";
    return t;
}

juce::String Turn::said() const
{
    return answer.substring (0, maxAnswerSent).trim();
}

juce::String Turn::after() const
{
    juce::String text;

    if (! plan.isEmpty())
    {
        text << "The plan shown for the request before this: ";

        for (int i = 0; i < juce::jmin (plan.size(), maxPlanStepsSent); ++i)
            text << (i + 1) << ". " << plan[i] << (i + 1 < plan.size() ? "; " : "");

        if (plan.size() > maxPlanStepsSent)
            text << " and " << (plan.size() - maxPlanStepsSent) << " more";

        text << ". ";
    }

    switch (outcome)
    {
        case Outcome::applied:    text << "The user applied it: those changes are made."; break;
        case Outcome::discarded:  text << "The user discarded it: nothing was changed."; break;
        case Outcome::pending:    text << "The user did not apply it: nothing was changed."; break;
        case Outcome::none:       break;
    }

    return text.isEmpty() ? text : "[" + text.trim() + "]";
}

juce::String releaseNotesFor()
{
    return "0.24.0";
}

bool asksAboutRelease (const juce::String& request)
{
    const auto r = request.toLowerCase();

    for (auto* words : { "what's new", "whats new", "what is new", "new in ", "new feature", "version", "release", "changelog", "change log", "what changed" })
        if (r.contains (words))
            return true;

    return false;
}

juce::String aboutThisRelease (bool withNotes)
{
    const juce::String line ("This is Torqwave " JUCE_APPLICATION_VERSION_STRING ", a free, open-source audio editor and DAW.");

    if (! withNotes)
        return line;

    return line + " The user may be asking about the program, not for a change: then answer from these notes in a few lines, make no changes, and "
           "say the full list is the changelog at https://github.com/BobClawblaw/torqwave/blob/main/CHANGELOG.md. "
           "New in 0.24.0: Zero Telemetry & Zero Tracking commitment (100% offline-first, zero background network calls, verified by source and unit checks); "
           "clean-room NI Maschine 2/3 (.mxgrp/.mxsnd) kit import into Basic Drums with automatic sample resolution, Drum Synth and per-pad FX translation, 960 PPQN MIDI pattern export, and one-click pattern drop onto the track; "
           "Akai MPC (.xpm) Drum and Keygroup kit import into Basic Drums and Torqwave Sampler; "
           "SoundFont 2 (.sf2) RIFF bank import with multi-preset browser in Torqwave Sampler; "
           "four new Live Creative FX bringing the built-in suite to 36 plug-ins — Hilbert SSB Frequency Shifter, granular Shimmer Delay, 24-band Vocoder (internal chord or sidechain carrier), and live STFT Spectral Freeze; "
           "and an interactive Grand-Staff Notation (Score) View inside the Note Editor with MusicXML 3.1 export, plus a redesigned one-click Instrument Track layout across the Multitrack header, Track Inspector, and Mixer strips (Option-I). "
           "0.23.0: six Live Creative FX (Delay, 4x-oversampled Distortion & Bitcrusher, Auto-Filter, Volume Shaper / Sidechain Pump, Noise Gate, Tremolo & Auto-Pan); "
           "Torqwave Synth expanded with LFOs 3–4, 24 mod slots, Slot Depth modulation, 16-node MSEG Curve LFOs, built-in Arpeggiator, 4-scene Evolve morphing and Complex matching in Decrypt; "
           "four Live MIDI FX (MIDI Arpeggiator, Chord Trigger, Scale Lock, Note Echo); Piano Roll ratchets, micro-timing, groove templates and ghost notes; "
           "Audio to MIDI (melody and drums with groove extraction); Torqwave Sampler SFZ import, round-robin groups and Signalsmith Warp; "
           "Basic Drums 8-bus multi-out pad routing; Track-level LFO and envelope follower modulators; CLAP 1.2 plug-in hosting; and headless CI tests on Windows and Linux. "
           "0.22.1: 58 more nine-oscillator musical club patches in Torqwave Synth's Complex folder (two new per category, "
           "116 Complex patches in all, three nine-oscillator patches in every category), calibrated around -17 LUFS with parallel "
           "mono low end and stereo air; reworked website demos showcasing the Complex patches and Sequence Library patterns. "
           "0.22.0: Torqwave Synth has nine oscillators, each with FM From to choose which oscillator drives it, and a "
           "Complex folder of 58 patches that use four to nine of them and move as they are held; re-striking a held note, sweeping "
           "the cutoff and legato slides no longer click; 174 near-copies are taken out of the factory patches, 3,569 left; the "
           "Sequence Library is rewritten so no two presets play the same notes (1,024 sequences, 116 kits), its drones and pads "
           "are levelled, and its styles, kits and audition patterns are named for what they play. "
           "0.21.1: Basic Drums is a drum synth and sampler in one: any pad can play your own samples in up to 16 velocity layers "
           "with round robin, there are 64 pads (34 user pads), every pad has its own envelope, transient shaper, saturator, EQ and "
           "compressor, 24 more synthesized sources make 54, three more kits make fifteen, pads take choke groups of your own, and Files > "
           "Import Battery Kit and Import Folder of Sounds bring kits in; Continue where you left off keeps a session never saved; "
           "Transport > Silence All Instruments is a panic. "
           "0.21.0: Basic Drums has a second type, Modern, for 21 of its 30 sounds, and five kits made of them; the Maximizer holds its "
           "true-peak ceiling as the loudness meter reads it; Click and Pop Repair finds flat-topped clicks and leaves drum hits alone; Repair "
           "Clipping rebuilds from the sound round the clip; Ask Torq remembers the conversation. "
           "0.20.1: Continue where you left off, a start that reopens the files and session that were open. "
           "0.20.0: Torqwave Synth (wavetable, 3,569 patches), Decrypt (a synth patch from a recording), Torqwave Sampler, StringSwarm, Basic "
           "Drums, instruments played from the computer's keys, and over a thousand factory sequence presets.";
}

Agent::Agent (Settings s, juce::String key, Workspace::Start start, juce::String r, std::vector<Turn> before)
    : juce::Thread ("Ask Torq"), client (std::move (s), std::move (key)), workspace (std::move (start)), request (std::move (r)),
      earlier (std::move (before))
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

bool Agent::askServerForWindow = true;

void Agent::count (const Reply& reply)
{
    // The server's count where it sent one; otherwise four characters to a token, of what was sent
    // and of what came back.
    const auto answerChars = reply.content.length() + reply.reasoning.length();
    const auto counted = reply.promptTokens >= 0;
    const auto answered = reply.completionTokens >= 0 ? reply.completionTokens : (answerChars + 3) / 4;
    usage.estimated = ! counted;
    usage.contextTokens = (counted ? reply.promptTokens : (reply.requestChars + 3) / 4) + answered;
    usage.answerTokens += answered;
    ++usage.rounds;
    Event e { Event::Kind::usage, {}, {}, {} };
    e.usage = usage;
    post (std::move (e));
}

void Agent::run()
{
    usage.window = askServerForWindow ? client.contextWindow() : 0;

    if (threadShouldExit())
    {
        post ({ Event::Kind::done, {}, "Cancelled.", {} });
        return;
    }

    // The conversation: what the program and the open file are, the turns before (the last few,
    // cut short: the model's context is small), and the request. Each request works on a fresh
    // copy of the file as it is now, so the turns say whether their plans were applied.
    juce::Array<juce::var> messages { message ("system", workspace.systemMessage() + "\n\n" + aboutThisRelease (asksAboutRelease (request))
                                                             + (earlier.empty() ? "" : "\n\nThis conversation has earlier requests. Each request starts "
                                                                                       "from the file as it is now: an earlier plan is in it only if the "
                                                                                       "user applied it.")) };

    // What became of each plan goes in with the request after it, as the program's note in
    // brackets, not in the model's own words: given as its words, a real model took to ending its
    // answers with a plan and a line saying the user had applied it.
    juce::String note;

    for (auto i = earlier.size() > (size_t) maxTurnsSent ? earlier.size() - (size_t) maxTurnsSent : (size_t) 0; i < earlier.size(); ++i)
    {
        messages.add (message ("user", note.isEmpty() ? earlier[i].request : note + "\n\n" + earlier[i].request));
        messages.add (message ("assistant", earlier[i].said()));
        note = earlier[i].after();
    }

    messages.add (message ("user", note.isEmpty() ? request : note + "\n\n" + request));
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

        count (reply);

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

    if (reply.error.isEmpty())
        count (reply);

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
        else if (e.kind == Event::Kind::usage)
        {
            if (onUsage != nullptr)
                onUsage (e.usage);
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
    requestBox.addKeyListener (this);   // up and down: the requests made before
    addAndMakeVisible (requestBox);

    stats.setColour (juce::Label::textColourId, Theme::dimText);
    stats.setFont (juce::FontOptions (12.0f));
    stats.setJustificationType (juce::Justification::topLeft);
    stats.setMinimumHorizontalScale (1.0f);
    addAndMakeVisible (stats);
    showStats (nullptr);

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
        settle (Turn::Outcome::applied);

        if (suggestionButtons.isEmpty())   // suggestions stay to apply
            agent.reset();

        updateButtons();
    };
    discardButton.onClick = [this] { discard(); };
    newChatButton.onClick = [this] { newChat(); };
    newChatButton.setTooltip ("Forgets this conversation and starts another. Until then, each request is sent with the ones before it");
    applyAllButton.onClick = [this] { applyAllSuggestions(); };
    applyAllButton.setTooltip ("Applies every suggestion not yet applied, together as one undo step");

    for (auto* b : { &askButton, &stopButton, &newChatButton, &applyButton, &discardButton })
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
    newChatButton.setBounds (top.removeFromRight (100));

    auto bottom = r.removeFromBottom (28);
    discardButton.setBounds (bottom.removeFromRight (100));
    bottom.removeFromRight (8);
    applyButton.setBounds (bottom.removeFromRight (100));
    applyAllButton.setBounds (bottom.removeFromLeft (100));
    r.removeFromBottom (6);
    stats.setBounds (r.removeFromBottom (32));
    r.removeFromBottom (4);
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
    newChatButton.setEnabled (! working && (! turns.empty() || ! log.isEmpty()));
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
    {
        addLog ("\nDiscarded. Nothing was changed.\n", Theme::dimText);
        settle (Turn::Outcome::discarded);
    }

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

    // The log is the conversation: a request follows the ones before it, and the box is ready for the next.
    if (! log.isEmpty())
        addLog ("\n\n", Theme::text);

    addLog ("You: " + text + "\n\n", Theme::textStrong);
    requestBox.setText ({}, false);   // not clear(): that leaves the box's value behind, which can put the text back

    // Kept for the arrows (the newest last, each once), up to fifty.
    asked().removeString (text);
    asked().add (text);

    while (asked().size() > 50)
        asked().remove (0);

    recalled = -1;
    draft.clear();

    // With the context nearly full, the earlier turns are left out, so the request still fits.
    auto earlier = turns;

    if (lastUsage.share() > fullEnoughToForget && ! earlier.empty())
    {
        earlier.clear();
        addLog ("The model's context is " + juce::String (juce::roundToInt (100.0 * lastUsage.share()))
                    + " % full, so this request goes without the earlier ones.\n\n", Theme::warn);
    }

    agent = std::make_unique<Agent> (settings, savedKey (settings), std::move (*start), text, std::move (earlier));
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
    agent->onUsage = [this] (const Usage& u) { showStats (&u); };
    agent->start();
    updateButtons();
}

void AskPanel::showStats (const Usage* u)
{
    lastUsage = u != nullptr ? *u : Usage();
    stats.setText (u != nullptr ? u->text()
                                : "Context: nothing sent yet. Sent with a request: the last " + juce::String (maxTurnsSent) + " before it, "
                                      + juce::String (maxAnswerSent) + " characters of each answer.",
                   juce::dontSendNotification);
    stats.setColour (juce::Label::textColourId, lastUsage.share() > fullEnoughToForget ? Theme::warn : Theme::dimText);
}

void AskPanel::recall (int direction)
{
    const auto& requests = asked();

    if (requests.isEmpty() || (agent != nullptr && agent->isWorking()))
        return;

    if (recalled < 0)
    {
        if (direction > 0)
            return;

        draft = requestBox.getText();
        recalled = requests.size();
    }

    recalled = juce::jmax (0, recalled + direction);

    if (recalled >= requests.size())
    {
        recalled = -1;
        requestBox.setText (draft, false);
    }
    else
    {
        requestBox.setText (requests[recalled], false);
    }

    requestBox.moveCaretToEnd();
}

bool AskPanel::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    // Up on the box's first line and down on its last walk the earlier requests; elsewhere in a
    // request of several lines they move the caret, as they do in any text.
    const auto text = requestBox.getText();
    const auto caret = requestBox.getCaretPosition();

    if (key == juce::KeyPress::upKey && ! text.substring (0, caret).containsChar ('\n'))
    {
        recall (-1);
        return true;
    }

    if (key == juce::KeyPress::downKey && recalled >= 0 && ! text.substring (caret).containsChar ('\n'))
    {
        recall (1);
        return true;
    }

    return false;
}

void AskPanel::settle (Turn::Outcome outcome)
{
    if (! turns.empty() && turns.back().outcome == Turn::Outcome::pending)
        turns.back().outcome = outcome;
}

void AskPanel::newChat()
{
    if (agent != nullptr && agent->isWorking())
        return;

    agent.reset();
    planReady = false;
    turns.clear();
    log.clear();
    showStats (nullptr);
    showSuggestions();
    updateButtons();
}

void AskPanel::finish (const juce::String& answer, const juce::String& error)
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
    turns.push_back ({ lastRequest, answer, steps, planReady ? Turn::Outcome::pending : Turn::Outcome::none });

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
