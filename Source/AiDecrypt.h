#pragma once

#include "AiClient.h"
#include "Decrypt.h"

/** Experimental: Ask Torq's model helping Decrypt, when Ask Torq is on.

    It is told the target's measures, the best candidate's settings and where the two differ, and
    proposes where to start, changes of structure (which table, which filter, a layer an octave
    up) and a name for the patch taken. It never decides anything: Decrypt renders and scores
    whatever it proposes, and keeps it only if it scores better (Decrypt::Advisor).

    Text only, unless the user has turned on Let the model hear clips: then the clip goes to the
    server once, as audio, for a description that joins the starting hints. A model that doesn't
    take audio answers that with an error, and nothing changes. */
namespace Ai
{
    class DecryptAdvisor final : public Decrypt::Advisor
    {
    public:
        DecryptAdvisor (Settings, juce::String key);

        Start start (const Brief&) override;
        std::vector<Change> structure (const Brief&) override;
        Name name (const Brief&) override;
        void cancel() override;

        /** An answer read: the JSON object in it (thinking and fences around it ignored), as each
            request asks for it. Settings are read against the synth's parameters: a choice by its
            name or index, a switch as on, off, true, false, 1 or 0, anything else as a number in
            the setting's own units. What can't be read is left out. Public for the tests. */
        static Start readStart (const juce::String& answer);
        static std::vector<Change> readChanges (const juce::String& answer);
        static Name readName (const juce::String& answer);

        /** A clip as a 16-bit WAV file at 16 kHz, base64: what a model that hears is sent. */
        static juce::String wavBase64 (const juce::AudioBuffer<float>&, double sampleRate);

        /** The factory: an advisor for Ask Torq's settings as they are now, or none when it is off. */
        static std::shared_ptr<Decrypt::Advisor> fromSettings (const Settings&);

    private:
        /** One request, text and (with audio) a clip; the answer's text, or empty on any error. */
        juce::String ask (const juce::String& system, const juce::String& user, const juce::String& audioBase64 = {});

        Settings settings;
        juce::String key;
        juce::CriticalSection lock;
        Client* current = nullptr;      // guarded by lock
        std::atomic<bool> cancelled { false };
    };
}
