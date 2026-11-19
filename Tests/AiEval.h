#pragma once

#include <JuceHeader.h>

/** Experimental AI assistance, measured: sentences a user might type, each run through Ask
    Torqwave's agent against a real model server on a generated test file, with a check of what
    the plan did. Needs the network, so it runs only on request (--ai-eval <server URL> <model>).
    set: empty for the ten requests the instructions were tuned against, "unseen" for 25 written
    later and never tuned on, "all" for both, "session" for 16 on a multitrack session. Prints one line a case and a summary; returns the
    number of cases that failed. */
int runAiEval (const juce::String& serverUrl, const juce::String& model, const juce::String& key, const juce::String& set = {});
