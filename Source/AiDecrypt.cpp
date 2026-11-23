#include "AiDecrypt.h"
#include "AiKeychain.h"

namespace Ai
{
namespace
{
    juce::var textMessage (const juce::String& role, const juce::String& content)
    {
        auto* m = new juce::DynamicObject();
        m->setProperty ("role", role);
        m->setProperty ("content", content);
        return juce::var (m);
    }

    /** The JSON object in an answer: from its first { to its last }. */
    juce::var objectIn (const juce::String& answer)
    {
        const auto from = answer.indexOfChar ('{'), to = answer.lastIndexOfChar ('}');

        if (from < 0 || to <= from)
            return {};

        const auto parsed = juce::JSON::parse (answer.substring (from, to + 1));
        return parsed.isObject() ? parsed : juce::var();
    }

    juce::StringArray stringsIn (const juce::var& v)
    {
        juce::StringArray out;

        if (const auto* a = v.getArray())
            for (auto& x : *a)
                if (x.isString() && x.toString().trim().isNotEmpty())
                    out.add (x.toString().trim());

        return out;
    }

    const char* aboutDecrypt =
        "You help Decrypt, in Torqwave Synth (a wavetable synth with three oscillators, a sub, noise, two filters, four envelopes, two LFOs "
        "and 16 modulation slots), find a patch that sounds like a recording of one note. A search renders candidate patches and scores "
        "each against the recording by its spectrum over time and its loudness; whatever you propose is scored the same way and kept "
        "only if it comes closer. The effects are off while matching. Answer with one JSON object and nothing else.";
}

DecryptAdvisor::DecryptAdvisor (Settings s, juce::String k) : settings (std::move (s)), key (std::move (k)) {}

std::shared_ptr<Decrypt::Advisor> DecryptAdvisor::fromSettings (const Settings& s)
{
    if (! s.isOn() || s.model.isEmpty())
        return nullptr;

    const auto saved = AiKeychain::load (s.keyAccount().toStdString());
    return std::make_shared<DecryptAdvisor> (s, saved ? juce::String::fromUTF8 (saved->c_str(), (int) saved->size()) : juce::String());
}

void DecryptAdvisor::cancel()
{
    gaveUp = true;      // for whatever is left of the call in hand: its other requests are not made
    cancelled = true;
    const juce::ScopedLock sl (lock);

    if (current != nullptr)
        current->cancel();
}

juce::String DecryptAdvisor::ask (const juce::String& system, const juce::String& user, const juce::String& audioBase64)
{
    Client client (settings, key);
    {
        const juce::ScopedLock sl (lock);

        if (cancelled.exchange (false) || gaveUp)
            return {};

        current = &client;
    }

    juce::Array<juce::var> messages { textMessage ("system", system) };

    if (audioBase64.isEmpty())
        messages.add (textMessage ("user", user));
    else
    {
        // The Chat Completions form for a clip: text, then the audio as a WAV file.
        auto* text = new juce::DynamicObject();
        text->setProperty ("type", "text");
        text->setProperty ("text", user);
        auto* data = new juce::DynamicObject();
        data->setProperty ("data", audioBase64);
        data->setProperty ("format", "wav");
        auto* audio = new juce::DynamicObject();
        audio->setProperty ("type", "input_audio");
        audio->setProperty ("input_audio", juce::var (data));
        auto* m = new juce::DynamicObject();
        m->setProperty ("role", "user");
        m->setProperty ("content", juce::Array<juce::var> { juce::var (text), juce::var (audio) });
        messages.add (juce::var (m));
    }

    // Thinking off: a reasoning model that thinks first takes minutes over an answer the search
    // needs in seconds. A server that refuses the field by name is asked again without it.
    auto* kwargs = new juce::DynamicObject();
    kwargs->setProperty ("enable_thinking", false);
    client.requestExtras.set ("chat_template_kwargs", juce::var (kwargs));
    auto reply = client.chat (messages, {});

    if (reply.error.isNotEmpty() && reply.status >= 400 && reply.error.contains ("chat_template_kwargs") && ! cancelled)
    {
        client.requestExtras.clear();
        reply = client.chat (messages, {});
    }

    {
        const juce::ScopedLock sl (lock);
        current = nullptr;
    }

    return reply.error.isEmpty() && ! cancelled.exchange (false) ? reply.content : juce::String();
}

//==============================================================================
Decrypt::Advisor::Start DecryptAdvisor::start (const Brief& b)
{
    Start s;
    gaveUp = false;

    // A model that hears is sent the clip, only when the user has allowed it.
    if (settings.hearClips && b.clip != nullptr && b.clip->getNumSamples() > 0)
        s.heard = ask ("You describe sounds for a sound designer, in one plain sentence: the instrument or source it sounds like, its tone, "
                       "its attack and how it dies away. No preamble.",
                       "Describe this recording of one note.", wavBase64 (*b.clip, Decrypt::rate)).trim().substring (0, 300);

    juce::String user;
    user << "The recording: " << b.target << ".\n";

    if (s.heard.isNotEmpty())
        user << "Heard by a model: " << s.heard << "\n";

    if (b.words.isNotEmpty())
        user << "The user says: " << b.words << "\n";

    user << "\nThe factory patches' categories: " << WaveSynth::patchCategories().joinIntoString (", ") << ".\n"
         << "Name up to four categories whose patches are the likeliest starting points, any factory patch names you are sure of, and "
            "the parts of a patch the search should hold as its starting points have them while it varies the rest (any of oscillators, "
            "filter, envelopes, modulation; none unless the user's words call for it).\n"
         << R"(Answer: {"categories": ["..."], "patches": ["..."], "hold": ["..."]})";

    const auto answer = ask (aboutDecrypt, user);
    auto read = readStart (answer);
    read.heard = s.heard;
    return read;
}

std::vector<Decrypt::Advisor::Change> DecryptAdvisor::structure (const Brief& b)
{
    gaveUp = false;
    juce::String user;
    user << "The recording: " << b.target << ".\n";

    if (b.words.isNotEmpty())
        user << "The user says: " << b.words << "\n";

    user << "\nThe best candidate so far (lock " << juce::String (b.lock, 0) << " %), its settings that differ from the synth's defaults, by ID:\n"
         << b.best << "\n\nWhere it differs from the recording: " << b.difference << ".\n\n"
         << "The search moves knobs well but changes structure badly. Propose up to five changes of structure to this candidate, each a few "
            "settings that go together: which table an oscillator plays, which filter type, a second or third oscillator an octave or a "
            "fifth away, a sub or noise layer, unison, an envelope's shape. Give each setting by its ID, a choice by its name, a switch as "
            "on or off, a number in the setting's own units.\n"
         << R"(Answer: {"changes": [{"what": "a few words", "settings": {"osc2On": "on", "osc2Table": "...", "osc2Pitch": 12}}]})";

    return readChanges (ask (aboutDecrypt, user));
}

Decrypt::Advisor::Name DecryptAdvisor::name (const Brief& b)
{
    gaveUp = false;
    juce::String user;
    user << "Decrypt found this patch for a recording: " << b.target << " (lock " << juce::String (b.lock, 0) << " %).\n";

    if (b.words.isNotEmpty())
        user << "The user says: " << b.words << "\n";

    user << "Its settings:\n" << b.best << "\n\n"
         << "Name it as a factory patch would be named (two or three words, evocative, no brand or artist names), choose its category from: "
         << WaveSynth::patchCategories().joinIntoString (", ") << "; describe it in one line; and name its four macros (a word each, or "
            "empty for one that does nothing).\n"
         << R"(Answer: {"name": "...", "category": "...", "description": "...", "macros": ["...", "...", "...", "..."]})";

    return readName (ask (aboutDecrypt, user));
}

//==============================================================================
Decrypt::Advisor::Start DecryptAdvisor::readStart (const juce::String& answer)
{
    Start s;
    const auto o = objectIn (answer);

    if (! o.isObject())
        return s;

    s.categories = stringsIn (o["categories"]);
    s.patches = stringsIn (o["patches"]);

    for (auto& part : stringsIn (o["hold"]))
        for (int p = 0; p < SignalConsole::effects; ++p)
            if (part.equalsIgnoreCase (SignalConsole::partNames()[p]) || part.equalsIgnoreCase (SignalConsole::partNames()[p].dropLastCharacters (1)))
                s.firewall[(size_t) p] = true;

    return s;
}

std::vector<Decrypt::Advisor::Change> DecryptAdvisor::readChanges (const juce::String& answer)
{
    std::vector<Change> changes;
    const auto o = objectIn (answer);
    const auto* list = o["changes"].getArray();

    if (list == nullptr)
        return changes;

    Decrypt::Synth synth;   // its parameters read the settings

    for (auto& item : *list)
    {
        Change c;
        c.what = item["what"].toString().trim().substring (0, 80);

        if (const auto* set = item["settings"].getDynamicObject())
            for (auto& [name, value] : set->getProperties())
            {
                const auto id = name.toString();
                auto* p = synth.parameter (id);

                if (p == nullptr)
                {
                    c.values[id] = std::numeric_limits<double>::quiet_NaN();   // named, but no setting: the search ignores it
                    continue;
                }

                const auto text = value.toString().trim();

                if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (p))
                {
                    auto index = choice->choices.indexOf (text, true);

                    if (index < 0 && (value.isInt() || value.isDouble() || value.isInt64()))
                        index = (int) value;

                    if (juce::isPositiveAndBelow (index, choice->choices.size()))
                        c.values[id] = index;
                }
                else if (dynamic_cast<juce::AudioParameterBool*> (p) != nullptr)
                {
                    if (text.equalsIgnoreCase ("on") || text.equalsIgnoreCase ("true") || text == "1")
                        c.values[id] = 1.0;
                    else if (text.equalsIgnoreCase ("off") || text.equalsIgnoreCase ("false") || text == "0")
                        c.values[id] = 0.0;
                }
                else if (value.isInt() || value.isDouble() || value.isInt64() || text.containsOnly ("-+.0123456789"))
                    c.values[id] = text.getDoubleValue();
            }

        if (c.what.isEmpty())
            c.what = "a change";

        if (! c.values.empty())
            changes.push_back (std::move (c));

        if (changes.size() >= 5)
            break;
    }

    return changes;
}

Decrypt::Advisor::Name DecryptAdvisor::readName (const juce::String& answer)
{
    Name n;
    const auto o = objectIn (answer);

    if (! o.isObject())
        return n;

    n.name = o["name"].toString().trim().substring (0, 40);
    n.category = o["category"].toString().trim().substring (0, 30);
    n.description = o["description"].toString().trim().substring (0, 200);
    const auto macros = stringsIn (o["macros"]);

    for (int m = 0; m < WaveSynth::numMacros && m < macros.size(); ++m)
        n.macros[(size_t) m] = macros[m].substring (0, 16);

    return n;
}

juce::String DecryptAdvisor::wavBase64 (const juce::AudioBuffer<float>& audio, double sampleRate)
{
    constexpr double outRate = 16000.0;
    const auto ratio = sampleRate / outRate;
    juce::AudioBuffer<float> mono (1, juce::jmax (1, (int) (audio.getNumSamples() / ratio)));
    juce::LagrangeInterpolator interpolator;
    interpolator.process (ratio, audio.getReadPointer (0), mono.getWritePointer (0), mono.getNumSamples(), audio.getNumSamples(), 0);

    juce::MemoryBlock bytes;
    {
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::MemoryOutputStream> (bytes, false);
        const auto options = juce::AudioFormatWriterOptions{}.withSampleRate (outRate).withNumChannels (1).withBitsPerSample (16);

        if (auto writer = wav.createWriterFor (stream, options))
            writer->writeFromAudioSampleBuffer (mono, 0, mono.getNumSamples());
    }

    return juce::Base64::toBase64 (bytes.getData(), bytes.getSize());
}
}
