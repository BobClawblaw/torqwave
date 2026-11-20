#include "ClipEffects.h"
#include "AudioEdits.h"
#include "OfflineRender.h"
#include "TempoFollow.h"
#include "TimeStretch.h"

namespace ClipEffects
{
namespace
{
    juce::String address (const AudioDocument::Snapshot& s)
    {
        return juce::String::toHexString ((juce::pointer_sized_int) s.get());
    }

    std::unique_ptr<juce::XmlElement> parse (const juce::String& chain)
    {
        auto xml = chain.isNotEmpty() ? juce::parseXML (chain) : nullptr;
        return xml != nullptr && xml->hasTagName ("TORQWAVE_RACK") ? std::move (xml) : std::make_unique<juce::XmlElement> ("TORQWAVE_RACK");
    }

    juce::String text (const juce::XmlElement& rack)
    {
        return rack.getNumChildElements() > 0 ? rack.toString (juce::XmlElement::TextFormat().singleLine().withoutHeader()) : juce::String();
    }

    juce::XmlElement* slotAt (juce::XmlElement& rack, int index)
    {
        return juce::isPositiveAndBelow (index, rack.getNumChildElements()) ? rack.getChildElement (index) : nullptr;
    }
}

juce::String key (const SessionState& s, const SessionClip& c)
{
    if (! c.effects.isOn() || c.audio == nullptr)
        return {};

    juce::String k;
    k << juce::String (s.sampleRate, 1) << " st" << juce::String (c.effects.semitones, 6)
      << " fx" << juce::String::toHexString (c.effects.chain.hashCode64()) << ":" << c.effects.chain.length()
      << " n" << c.length;

    if (c.comp.empty() || c.takes.empty())
    {
        k << " a" << address (c.audio) << "@" << c.offset;
        return k;
    }

    // A comp: every take where it lies against take 0, the comp and its crossfades, from the
    // take-0 frame where the clip starts.
    k << " z" << c.takeZeroFrame (c.position) << " f" << c.compFade;

    for (auto& t : c.takes)
        k << " t" << address (t.audio) << "@" << t.shift;

    for (auto& seg : c.comp)
        k << " c" << seg.from << ":" << seg.take;

    return k;
}

bool isStale (const SessionState& s, const SessionClip& c)
{
    return c.effects.isOn() && c.audio != nullptr && (c.effects.rendered == nullptr || c.effects.renderedFor != key (s, c));
}

void dropStale (SessionState& s)
{
    for (auto& t : s.tracks)
        for (auto& c : t.clips)
            if (c.effects.rendered != nullptr && (! c.effects.isOn() || c.effects.renderedFor != key (s, c)))
            {
                c.effects.rendered = nullptr;
                c.effects.renderedFor = {};
            }
}

AudioDocument::Snapshot render (const SessionState& s, const SessionClip& c, juce::AudioPluginFormatManager& formats, juce::String& error,
                                const std::function<bool (float)>& progress)
{
    error = {};
    const auto dry = c.comp.empty() || c.takes.empty() ? SessionEdits::playedPart (c, c.audio) : SessionEdits::compAudio (c);

    if (dry == nullptr || ! c.effects.isOn())
        return nullptr;

    juce::AudioBuffer<float> audio (dry->getNumChannels(), (int) dry->getNumFrames());
    dry->read (audio, 0, 0, audio.getNumSamples());

    // The pitch first, then the chain, each its share of the progress.
    const auto shifting = ! juce::exactlyEqual (c.effects.semitones, 0.0);
    const auto chained = c.effects.chain.isNotEmpty();
    const auto pitchShare = shifting && chained ? 0.5f : 1.0f;

    if (shifting)
    {
        audio = TimeStretch::shiftPitch (audio, c.effects.semitones, s.sampleRate, [&progress, pitchShare] (float p)
        {
            return progress == nullptr || progress (pitchShare * p);
        });

        if (audio.getNumSamples() != dry->getNumFrames())
            return nullptr;
    }

    if (chained)
    {
        const auto rack = parse (c.effects.chain);
        const auto from = shifting ? pitchShare : 0.0f;
        error = OfflineRender::processThroughRack (*rack, formats, s.sampleRate, audio, [&progress, from] (float p)
        {
            return progress == nullptr || progress (from + (1.0f - from) * p);
        });

        if (error == "Cancelled.")
            error = {};

        if (error.isNotEmpty() || audio.getNumSamples() != dry->getNumFrames())
            return nullptr;
    }

    if (progress != nullptr && ! progress (1.0f))
        return nullptr;

    return AudioEdits::fromBuffer (std::move (audio));
}

juce::String renderAll (SessionState& s, juce::AudioPluginFormatManager& formats)
{
    juce::StringArray errors;

    for (auto& t : s.tracks)
        for (auto& c : t.clips)
            if (isStale (s, c))
            {
                juce::String error;

                if (auto audio = render (s, c, formats, error))
                {
                    c.effects.rendered = std::move (audio);
                    c.effects.renderedFor = key (s, c);
                }
                else if (error.isNotEmpty())
                {
                    errors.add (c.name + ": " + error);
                }
            }

    return errors.joinIntoString ("\n");
}

//==============================================================================
std::vector<Slot> slotsOf (const juce::String& chain)
{
    std::vector<Slot> slots;
    const auto rack = parse (chain);

    for (auto* slot : rack->getChildWithTagNameIterator ("SLOT"))
    {
        Slot s;

        if (auto* d = slot->getChildByName ("PLUGIN"))
            s.plugin.loadFromXml (*d);

        s.name = s.plugin.name.isNotEmpty() ? s.plugin.name : juce::String ("Unknown effect");
        s.bypassed = slot->getBoolAttribute ("bypassed");
        slots.push_back (std::move (s));
    }

    return slots;
}

juce::String withSlot (const juce::String& chain, const juce::PluginDescription& plugin, const juce::MemoryBlock& state)
{
    auto rack = parse (chain);
    rack->setAttribute ("version", 1);
    auto* slot = rack->createNewChildElement ("SLOT");
    slot->setAttribute ("bypassed", false);
    slot->addChildElement (plugin.createXml().release());

    if (state.getSize() > 0)
        slot->createNewChildElement ("STATE")->addTextElement (state.toBase64Encoding());

    return text (*rack);
}

juce::String withoutSlot (const juce::String& chain, int index)
{
    auto rack = parse (chain);

    if (auto* slot = slotAt (*rack, index))
        rack->removeChildElement (slot, true);

    return text (*rack);
}

juce::String withBypass (const juce::String& chain, int index, bool bypassed)
{
    auto rack = parse (chain);

    if (auto* slot = slotAt (*rack, index))
        slot->setAttribute ("bypassed", bypassed);

    return text (*rack);
}

juce::String withState (const juce::String& chain, int index, const juce::MemoryBlock& state)
{
    auto rack = parse (chain);

    if (auto* slot = slotAt (*rack, index))
    {
        slot->deleteAllChildElementsWithTagName ("STATE");

        if (state.getSize() > 0)
            slot->createNewChildElement ("STATE")->addTextElement (state.toBase64Encoding());
    }

    return text (*rack);
}

juce::String withMove (const juce::String& chain, int from, int to)
{
    auto rack = parse (chain);
    const auto count = rack->getNumChildElements();

    if (! juce::isPositiveAndBelow (from, count) || ! juce::isPositiveAndBelow (to, count) || from == to)
        return text (*rack);

    std::vector<std::unique_ptr<juce::XmlElement>> slots;

    while (rack->getNumChildElements() > 0)
    {
        auto* first = rack->getChildElement (0);
        rack->removeChildElement (first, false);
        slots.emplace_back (first);
    }

    auto moving = std::move (slots[(size_t) from]);
    slots.erase (slots.begin() + from);
    slots.insert (slots.begin() + to, std::move (moving));

    for (auto& slot : slots)
        rack->addChildElement (slot.release());

    return text (*rack);
}

juce::MemoryBlock stateOf (const juce::String& chain, int index)
{
    const auto rack = parse (chain);
    juce::MemoryBlock state;

    if (auto* slot = slotAt (*rack, index))
        if (auto* stateXml = slot->getChildByName ("STATE"))
            state.fromBase64Encoding (stateXml->getAllSubText());

    return state;
}

juce::String describe (const SessionClip::Effects& e)
{
    juce::StringArray parts;

    if (! juce::exactlyEqual (e.semitones, 0.0))
        parts.add ((e.semitones > 0.0 ? "+" : "") + juce::String (e.semitones, 2).trimCharactersAtEnd ("0").trimCharactersAtEnd (".") + " st");

    for (auto& slot : slotsOf (e.chain))
        parts.add (slot.bypassed ? slot.name + " (off)" : slot.name);

    return parts.joinIntoString (", ");
}

//==============================================================================
Renderer::Renderer (Session& s, juce::AudioPluginFormatManager& f) : juce::Thread ("Clip effects"), session (s), formats (f)
{
    session.addChangeListener (this);
    startThread (juce::Thread::Priority::low);
    look();
}

Renderer::~Renderer()
{
    session.removeChangeListener (this);
    signalThreadShouldExit();
    work.signal();
    stopThread (10000);
}

void Renderer::whenDone (std::function<void()> then)
{
    if (then == nullptr)
        return;

    waiting.push_back (std::move (then));
    look();
}

bool Renderer::isUpToDate() const
{
    const auto state = session.getState();

    for (auto& t : state->tracks)
        for (auto& c : t.clips)
            if (isStale (*state, c) && failed.count (key (*state, c)) == 0)
                return false;

    return true;
}

void Renderer::changeListenerCallback (juce::ChangeBroadcaster*)
{
    look();
}

void Renderer::look()
{
    const auto state = session.getState();
    std::optional<SessionState> next;
    std::vector<Job> wanting;
    std::set<juce::String> keys;

    for (size_t t = 0; t < state->tracks.size(); ++t)
        for (size_t i = 0; i < state->tracks[t].clips.size(); ++i)
        {
            const auto& c = state->tracks[t].clips[i];

            if (! isStale (*state, c))
                continue;

            const auto k = key (*state, c);

            if (failed.count (k) > 0)
                continue;

            const auto found = std::find_if (kept.begin(), kept.end(), [&k] (auto& e) { return e.key == k; });

            if (found == kept.end())
            {
                // A clip still being stretched to the tempo is rendered once it has been.
                if (! TempoFollow::isStale (*state, c) && keys.insert (k).second)
                    wanting.push_back ({ state, c.id, k });
                else
                    keys.insert (k);

                continue;
            }

            if (! next.has_value())
                next = *state;

            auto& clip = next->tracks[t].clips[i];
            clip.effects.rendered = found->render;
            clip.effects.renderedFor = k;
        }

    {
        const juce::ScopedLock sl (lock);

        // The one being rendered goes on if it is still wanted.
        wanting.erase (std::remove_if (wanting.begin(), wanting.end(), [this] (auto& j) { return j.key == working; }), wanting.end());
        jobs = std::move (wanting);
        wanted = keys;
    }

    if (! keys.empty())
        work.signal();

    // In place of the state as it is: not a step of its own, and nothing new to save.
    if (next.has_value())
        session.refresh (std::move (*next));

    // Everything rendered: what was waiting for it goes ahead (once each).
    if (! waiting.empty() && isUpToDate())
    {
        auto now = std::move (waiting);
        waiting.clear();

        for (auto& then : now)
            then();
    }
}

void Renderer::run()
{
    while (! threadShouldExit())
    {
        work.wait (-1);

        for (;;)
        {
            Job job;

            {
                const juce::ScopedLock sl (lock);

                if (jobs.empty())
                {
                    working = {};
                    break;
                }

                job = jobs.front();
                jobs.erase (jobs.begin());
                working = job.key;
            }

            const auto where = job.state->findClip (job.clipId);

            if (! where.has_value())
                continue;

            const auto& clip = job.state->tracks[(size_t) where->first].clips[(size_t) where->second];
            juce::String error;

            // Stops when no clip wants this any more.
            auto audio = render (*job.state, clip, formats, error, [this, &job] (float)
            {
                const juce::ScopedLock sl (lock);
                return ! threadShouldExit() && wanted.count (job.key) > 0;
            });

            if (threadShouldExit())
                return;

            if (audio == nullptr && error.isEmpty())
                continue;

            juce::MessageManager::callAsync ([safe = juce::WeakReference<Renderer> (this), k = job.key, audio, error, from = job.state,
                                              name = clip.name]
            {
                auto* r = safe.get();

                if (r == nullptr)
                    return;

                if (audio == nullptr)
                {
                    r->failed.insert (k);

                    if (r->onFailed != nullptr)
                        r->onFailed (name, error);
                }
                else
                {
                    r->kept.push_back ({ k, audio, from });

                    if (r->kept.size() > 16)
                        r->kept.erase (r->kept.begin());
                }

                r->look();
            });
        }
    }
}
}
