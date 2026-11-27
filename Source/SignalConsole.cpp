#if defined (__APPLE__)
 // CoreFoundation's own Point and Component, kept out of JUCE's way.
 #define Point CarbonDummyPointName
 #define Component CarbonDummyCompName
 #include <CoreFoundation/CoreFoundation.h>
 #undef Point
 #undef Component
#endif

#include "SignalConsole.h"

#if JUCE_WINDOWS
 #include <windows.h>
#endif

namespace SignalConsole
{
namespace
{
    constexpr double renderRate = 48000.0;
    constexpr int renderBlock = 512, renderLength = 48000, renderHeld = 28800, renderNote = 60;

    //==========================================================================
    // Numbers that are the same every time: a hash of the seed, the node, the setting and a counter.
    juce::uint32 mix (juce::uint32 x) noexcept
    {
        x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
        return x;
    }

    juce::uint32 hashOf (const juce::String& s) noexcept
    {
        juce::uint32 h = 2166136261u;

        for (auto c : s)
            h = (h ^ (juce::uint32) c) * 16777619u;

        return h;
    }

    /** 0 to 1. */
    float uniform (juce::uint32 seed, int node, juce::uint32 what, int counter) noexcept
    {
        const auto h = mix (mix (seed * 0x9e3779b9u + (juce::uint32) node * 0x85ebca6bu) ^ mix (what + (juce::uint32) counter * 0xc2b2ae35u));
        return (float) (h >> 8) * (1.0f / 16777216.0f);
    }

    /** About a bell curve round 0, one wide: three throws added. */
    float bell (juce::uint32 seed, int node, juce::uint32 what) noexcept
    {
        return (uniform (seed, node, what, 1) + uniform (seed, node, what, 2) + uniform (seed, node, what, 3)) * 2.0f - 3.0f;
    }

    //==========================================================================
    // The reach map: which settings move, how far, and how.
    enum class Kind { fixed, slide, choose, flip, step };

    struct Rule
    {
        Kind kind = Kind::fixed;
        float amount = 0.0f;   // a slide's width as a share of its range at full reach; the others' chance at full reach
    };

    double valueOf (const EffectPresets::Values& v, const juce::String& id, double otherwise = 0.0)
    {
        const auto found = v.find (id);
        return found != v.end() ? found->second : otherwise;
    }

    bool isOn (const EffectPresets::Values& v, const juce::String& id)   { return valueOf (v, id) > 0.5; }

    /** Whether a slot routes this source somewhere. */
    bool sourceUsed (const EffectPresets::Values& v, int source)
    {
        for (int s = 1; s <= WaveSynth::numSlots; ++s)
            if ((int) valueOf (v, "mod" + juce::String (s) + "Source") == source && (int) valueOf (v, "mod" + juce::String (s) + "Destination") > 0)
                return true;

        return false;
    }

    /** How a setting moves in this patch: nothing that can't be heard moves (an oscillator that is
        off, a filter that is off, an effect that is off, an envelope or LFO nothing uses). */
    Rule ruleFor (const juce::String& id, const EffectPresets::Values& v)
    {
        const auto ends = [&id] (const char* suffix) { return id.endsWith (suffix); };

        if (id.startsWith ("osc"))
        {
            const auto osc = id.substring (0, 4);

            if (ends ("EnvOn"))                                       return {};
            if (id.contains ("Env"))                                  return isOn (v, osc + "On") && isOn (v, osc + "EnvOn") ? Rule { Kind::slide, 0.25f } : Rule {};
            if (ends ("On"))                                          return osc == "osc1" ? Rule {} : Rule { Kind::flip, 0.12f };
            if (! isOn (v, osc + "On"))                               return {};
            if (ends ("Table"))                                       return { Kind::choose, 0.2f };
            if (ends ("Position"))                                    return { Kind::slide, 0.35f };
            if (ends ("WarpAmount"))                                  return (int) valueOf (v, osc + "Warp") != 0 || (int) valueOf (v, osc + "Table") == (int) WaveTables::Table::string ? Rule { Kind::slide, 0.3f } : Rule {};
            if (ends ("Warp"))                                        return { Kind::choose, 0.15f };
            if (ends ("Pitch"))                                       return { Kind::step, 0.12f };
            if (ends ("Fine"))                                        return { Kind::slide, 0.1f };
            if (ends ("Volume"))                                      return { Kind::slide, 0.2f };
            if (ends ("Pan"))                                         return { Kind::slide, 0.15f };
            if (ends ("UnisonDetune") || ends ("UnisonSpread"))       return valueOf (v, osc + "Unison") > 1.5 ? Rule { Kind::slide, 0.3f } : Rule {};
            if (ends ("UnisonStrum"))                                 return valueOf (v, osc + "Unison") > 1.5 ? Rule { Kind::slide, 0.08f } : Rule {};
            if (ends ("UnisonDrift"))                                 return { Kind::slide, 0.2f };
            if (ends ("Unison"))                                      return { Kind::step, 0.2f };
            if (ends ("Route"))                                       return (int) valueOf (v, "filter2Type") != 0 ? Rule { Kind::slide, 0.2f } : Rule {};
            return {};   // Phase, RandomPhase
        }

        if (id == "subLevel")                                         return { Kind::slide, 0.2f };
        if (id == "subOctave" || id == "subShape")                    return valueOf (v, "subLevel") > 0.5 ? Rule { Kind::choose, 0.12f } : Rule {};
        if (id == "noiseLevel")                                       return { Kind::slide, 0.15f };
        if (id == "noiseType")                                        return valueOf (v, "noiseLevel") > 0.5 ? Rule { Kind::choose, 0.15f } : Rule {};
        if (id == "noiseColour")                                      return valueOf (v, "noiseLevel") > 0.5 ? Rule { Kind::slide, 0.3f } : Rule {};

        if (id.startsWith ("filter"))
        {
            if (id == "filterRouting")                                return (int) valueOf (v, "filter1Type") != 0 && (int) valueOf (v, "filter2Type") != 0 ? Rule { Kind::choose, 0.1f } : Rule {};

            const auto filter = id.substring (0, 7);

            if ((int) valueOf (v, filter + "Type") == 0)              return {};   // an off filter stays off
            if (ends ("Type"))                                        return { Kind::choose, 0.15f };
            if (ends ("Cutoff"))                                      return { Kind::slide, 0.22f };
            if (ends ("Resonance") || ends ("Drive"))                 return { Kind::slide, 0.25f };
            if (ends ("KeyTrack"))                                    return { Kind::slide, 0.2f };
            return {};
        }

        if (id.startsWith ("env"))
        {
            const auto n = id.substring (3, 4).getIntValue();

            if (n > 1 && ! sourceUsed (v, WaveSynth::env1 + n - 1))  return {};
            return { Kind::slide, ends ("Sustain") ? 0.25f : 0.2f };
        }

        if (id.startsWith ("lfo"))
        {
            const auto n = id.substring (3, 4).getIntValue();
            const auto lfo = id.substring (0, 4);

            if (! sourceUsed (v, WaveSynth::lfoSource (n - 1)))      return {};
            if (ends ("Rate"))                                        return isOn (v, lfo + "Sync") ? Rule {} : Rule { Kind::slide, 0.25f };
            if (ends ("Division"))                                    return isOn (v, lfo + "Sync") ? Rule { Kind::choose, 0.15f } : Rule {};
            if (ends ("Shape"))                                       return { Kind::choose, 0.15f };
            if (ends ("Phase") || ends ("Smooth"))                    return { Kind::slide, 0.2f };
            if (ends ("Fade"))                                        return { Kind::slide, 0.12f };
            if (id.contains ("Step"))                                 return (int) valueOf (v, lfo + "Shape") == 7 ? Rule { Kind::slide, 0.2f } : Rule {};
            return {};   // Sync, Mode
        }

        if (id.startsWith ("mod"))
        {
            const auto slot = id.upToFirstOccurrenceOf ("Amount", false, false);
            return ends ("Amount") && (int) valueOf (v, slot + "Source") > 0 && (int) valueOf (v, slot + "Destination") > 0 ? Rule { Kind::slide, 0.3f } : Rule {};
        }

        if (id.startsWith ("fx"))
        {
            for (auto* effect : { "fxDrive", "fxChorus", "fxPhaser", "fxDelay", "fxReverb" })
                if (id.startsWith (effect))
                {
                    if (ends ("On"))                                  return { Kind::flip, 0.12f };
                    if (! isOn (v, juce::String (effect) + "On"))     return {};
                    if (ends ("Type") || ends ("Division"))           return { Kind::choose, 0.15f };
                    if (ends ("PingPong"))                            return {};
                    return { Kind::slide, ends ("Tone") ? 0.2f : 0.25f };
                }
        }

        return {};
    }

    juce::String deltaText (juce::RangedAudioParameter& p, double from, double to)
    {
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (&p))
            return c->choices[juce::jlimit (0, c->choices.size() - 1, (int) std::lround (to))];

        if (dynamic_cast<juce::AudioParameterBool*> (&p) != nullptr)
            return to > 0.5 ? "on" : "off";

        const auto delta = to - from, size = std::abs (delta);
        const auto unit = p.getLabel().trimCharactersAtStart ("+");
        const auto number = size >= 10000.0 ? juce::String (size / 1000.0, 1) + " k" : size >= 100.0 ? juce::String (juce::roundToInt (size)) + " "
                                                                                       : size >= 10.0 ? juce::String (size, 1) + " " : juce::String (size, 2) + " ";
        return (delta < 0.0 ? "-" : "+") + (number + unit).trim();
    }

    /** One setting moved in a node. */
    struct Moved
    {
        juce::String id;
        double from = 0.0, to = 0.0;
        float share = 0.0f;   // of its range (a choice or switch counts as a third)
        int part = 0;
    };

    void finish (Node& node, const std::vector<Moved>& moved, Synth& synth)
    {
        std::array<float, numParts> weight {};

        for (auto& m : moved)
            weight[(size_t) m.part] += m.share;

        node.part = (int) (std::max_element (weight.begin(), weight.end()) - weight.begin());
        auto order = moved;
        std::stable_sort (order.begin(), order.end(), [] (const Moved& a, const Moved& b) { return a.share > b.share; });

        for (size_t i = 0; i < order.size() && i < 4; ++i)
            if (auto* p = synth.parameter (order[i].id))
                node.changes.push_back ({ order[i].id, p->getName (64), deltaText (*p, order[i].from, order[i].to), order[i].share });
    }

    /** A setting that can move, with its rule, part and parameter (for its range). */
    struct Movable { juce::String id; Rule rule; int part; juce::RangedAudioParameter* parameter; };

    /** The settings of a patch that can move, by part, leaving out firewalled parts. */
    std::array<std::vector<Movable>, numParts> movableIn (const Patch& held, Synth& synth, const std::array<bool, numParts>& firewall)
    {
        std::array<std::vector<Movable>, numParts> movable;

        for (auto& [id, value] : held.values)
        {
            juce::ignoreUnused (value);
            const auto part = partOf (id);

            if (part < 0 || firewall[(size_t) part])
                continue;

            if (auto* p = synth.parameter (id))
                if (const auto rule = ruleFor (id, held.values); rule.kind != Kind::fixed)
                    movable[(size_t) part].push_back ({ id, rule, part, p });
        }

        return movable;
    }

    /** Node i's moves from the patch held, at `scale` of the full reach: mostly one part (the next one in
        turn), half the nodes a little of a second. Written into `patch`; what moved is returned. */
    std::vector<Moved> moveSettings (Patch& patch, const Patch& held, const std::array<std::vector<Movable>, numParts>& movable,
                                     const std::vector<int>& open, int firstPart, int i, int attempt, juce::uint32 seed, float scale)
    {
        std::vector<Moved> moved;

        if (open.empty() || scale <= 0.0f)
            return moved;

        // Mostly one part (the next one, if the last try moved nothing in it); half the nodes a little of a second.
        const auto first = open[(size_t) ((firstPart + i + attempt) % (int) open.size())];
        const auto second = open.size() > 1 && uniform (seed, i, 0x5345434fu, 0) < 0.5f
                                ? open[(size_t) ((firstPart + i + 1 + (int) (uniform (seed, i, 0x5345434fu, 1) * (float) (open.size() - 1))) % (int) open.size())] : -1;

        for (auto part : { first, second })
        {
            if (part < 0)
                continue;

            const auto amount = part == first ? scale : scale * 0.35f;
            // A part with nothing to slide (every effect off) has only its switches: they are thrown more readily.
            const auto onlySwitches = std::none_of (movable[(size_t) part].begin(), movable[(size_t) part].end(), [] (const Movable& m) { return m.rule.kind == Kind::slide; });

            for (auto& m : movable[(size_t) part])
            {
                const auto what = hashOf (m.id);
                const auto from = valueOf (held.values, m.id);
                auto to = from;
                auto share = 0.0f;

                if (m.rule.kind == Kind::slide)
                {
                    const auto was = m.parameter->convertTo0to1 ((float) from);
                    const auto now = juce::jlimit (0.0f, 1.0f, was + amount * m.rule.amount * 1.6f * bell (seed, i, what));
                    to = m.parameter->convertFrom0to1 (now);
                    share = std::abs (now - was);
                }
                else if (uniform (seed, i, what, 4) < amount * m.rule.amount * (onlySwitches ? 12.0f : 2.5f))
                {
                    share = 1.0f / 3.0f;

                    if (m.rule.kind == Kind::flip)
                        to = from > 0.5 ? 0.0 : 1.0;
                    else if (m.rule.kind == Kind::step)
                    {
                        // An oscillator's pitch moves by a fifth or an octave; its unison by a voice or two.
                        const auto pitch = m.id.endsWith ("Pitch");
                        const auto by = pitch ? (uniform (seed, i, what, 5) < 0.5f ? 12.0 : 7.0) : (uniform (seed, i, what, 5) < 0.6f ? 1.0 : 2.0);
                        const auto range = m.parameter->getNormalisableRange();
                        to = juce::jlimit ((double) range.start, (double) range.end, from + (uniform (seed, i, what, 6) < 0.5f ? -by : by));
                    }
                    else if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (m.parameter))
                    {
                        // Another of its choices: a filter never to Off, a table never to Sampled without one.
                        juce::Array<int> others;

                        for (int c = 0; c < choice->choices.size(); ++c)
                            if (c != (int) from && ! (m.id.endsWith ("Type") && m.id.startsWith ("filter") && c == 0)
                                && ! (m.id.endsWith ("Table") && c == (int) WaveTables::Table::sampled && held.tables[(size_t) (m.id.substring (3, 4).getIntValue() - 1)] == nullptr))
                                others.add (c);

                        if (! others.isEmpty())
                            to = others[juce::jmin (others.size() - 1, (int) (uniform (seed, i, what, 5) * (float) others.size()))];
                    }
                }

                if (! juce::exactlyEqual (to, from))
                {
                    patch.values[m.id] = to;
                    moved.push_back ({ m.id, from, to, share, m.part });
                }
            }
        }

        return moved;
    }

    /** The node's amp level set so it peaks where the held one does, then measured. False for a node that makes no sound
        in its first second, or far less than the one held. */
    bool levelAndMeasure (Node& node, float heldPeak, Renderer& renderer)
    {
        auto measured = renderer.measure (node.patch);

        if (measured.peak < 0.001f)
            return false;

        // Twice: the effects after the amp don't answer in proportion.
        for (int pass = 0; pass < 2 && heldPeak > 0.0f; ++pass)
        {
            // Where the held one peaks; a patch that is nearly silent in its first second (a slow riser) is taken up to -30 dB.
            const auto wanted = juce::jlimit (0.0316f, 0.89f, heldPeak);

            if (std::abs (juce::Decibels::gainToDecibels (measured.peak / wanted)) < 0.3f)
                break;

            auto& level = node.patch.values["ampLevel"];
            level = juce::jlimit (-40.0, 6.0, level + (double) juce::Decibels::gainToDecibels (wanted / measured.peak));
            measured = renderer.measure (node.patch);
        }

        node.signature = measured;
        node.address = addressOf (node.patch);
        // A node the amp's level can't bring within 6 dB of the one held (a slower swell, its level already at +6 dB) is drawn
        // again, and so is one that still clips (a drive after the amp that doesn't answer in proportion, as on Cinder Circuit).
        return measured.peak >= 0.5f * juce::jlimit (0.0316f, 0.89f, heldPeak) && measured.peak <= 1.0f;
    }
}

//==============================================================================
const juce::StringArray& partNames()
{
    static const juce::StringArray n { "Oscillators", "Filter", "Envelopes", "Modulation", "Effects" };
    return n;
}

int partOf (const juce::String& id)
{
    static const juce::StringArray never { "ampLevel", "ampPan", "ampVelocity", "voiceMode", "glideMode", "glideTime", "bendRange" };

    // Oscillators 4 to 9 and FM From are outside the search (Decrypt's and Trace's move 1 to 3 alone).
    if (never.contains (id) || id.startsWith ("macro") || WaveSynth::isAddedWithNine (id))   return -1;
    if (id.startsWith ("osc"))                            return id.contains ("Env") ? envelopes : oscillators;
    if (id.startsWith ("sub") || id.startsWith ("noise")) return oscillators;
    if (id.startsWith ("filter"))                         return filter;
    if (id.startsWith ("env"))                            return envelopes;
    if (id.startsWith ("lfo") || id.startsWith ("mod"))   return modulation;
    if (id.startsWith ("fx"))                             return effects;
    return -1;
}

Patch capture (const Synth& synth)
{
    Patch p;
    p.values = synth.getPresetValues();
    p.name = synth.getPatchName();

    for (int o = 0; o < WaveSynth::numOscillators; ++o)
        p.tables[(size_t) o] = synth.getSampledTable (o);

    for (int m = 0; m < WaveSynth::numMacros; ++m)
        p.macroNames[(size_t) m] = synth.getMacroName (m);

    return p;
}

void apply (Synth& synth, const Patch& p)
{
    synth.setPresetValues (p.values);

    for (int o = 0; o < WaveSynth::numOscillators; ++o)
        synth.setSampledTable (o, p.tables[(size_t) o]);

    for (int m = 0; m < WaveSynth::numMacros; ++m)
        synth.setMacroName (m, p.macroNames[(size_t) m]);

    if (p.name.isNotEmpty())
        synth.setPatchName (p.name);
}

juce::uint32 addressOf (const Patch& p)
{
    juce::uint32 h = 2166136261u;
    const auto add = [&h] (juce::uint32 x) { h = (h ^ x) * 16777619u; };

    // The settings in the order of their IDs, each to a thousandth. Oscillators 4 to 9 count only while on,
    // and FM From only when it isn't Before, so a patch that doesn't use them keeps the address it had.
    for (auto& [id, value] : p.values)
    {
        if (WaveSynth::isAddedWithNine (id))
        {
            const auto o = WaveSynth::oscillatorOf (id);
            const auto on = p.values.find ("osc" + juce::String (o) + "On");

            if (id.endsWith ("FmFrom") ? value == 0.0 : (on == p.values.end() || on->second == 0.0))
                continue;
        }

        add (hashOf (id));
        add ((juce::uint32) (juce::int32) std::llround (value * 1000.0));
    }

    for (auto& t : p.tables)
        if (t != nullptr)
        {
            add ((juce::uint32) t->frames);

            for (auto s : t->strengths)
                add (s);
        }

    return mix (h);
}

juce::String addressText (juce::uint32 address)
{
    return juce::String::toHexString ((int) ((address ^ (address >> 16)) & 0xffffu)).paddedLeft ('0', 4).toUpperCase();
}

//==============================================================================
Renderer::Renderer() : scratch (std::make_unique<Synth>())
{
    scratch->setPlayConfigDetails (0, 2, renderRate, renderBlock);
    scratch->setNonRealtime (true);
}

Renderer::~Renderer() = default;

juce::AudioBuffer<float> Renderer::render (const Patch& patch)
{
    apply (*scratch, patch);
    scratch->prepareToPlay (renderRate, renderBlock);
    juce::AudioBuffer<float> out (2, renderLength), block (2, renderBlock);

    for (int at = 0; at < renderLength; at += renderBlock)
    {
        const auto n = juce::jmin (renderBlock, renderLength - at);
        juce::MidiBuffer midi;

        if (at == 0)
            midi.addEvent (juce::MidiMessage::noteOn (1, renderNote, 0.8f), 0);

        if (renderHeld >= at && renderHeld < at + n)
            midi.addEvent (juce::MidiMessage::noteOff (1, renderNote), renderHeld - at);

        block.setSize (2, n, false, false, true);
        block.clear();
        scratch->processBlock (block, midi);

        for (int ch = 0; ch < 2; ++ch)
            out.copyFrom (ch, at, block, ch, 0, n);
    }

    return out;
}

Signature Renderer::measure (const Patch& patch)
{
    return measure (render (patch), renderRate);
}

Signature Renderer::measure (const juce::AudioBuffer<float>& audio, double sampleRate)
{
    Signature s;
    const auto length = audio.getNumSamples();

    if (length < 2048 || audio.getNumChannels() < 1)
        return s;

    const auto* left = audio.getReadPointer (0);
    const auto* right = audio.getReadPointer (audio.getNumChannels() > 1 ? 1 : 0);
    s.peak = juce::jmax (audio.getMagnitude (0, 0, length), audio.getNumChannels() > 1 ? audio.getMagnitude (1, 0, length) : 0.0f);

    // Its spectrum: the power in each bin, added up over windows half a window apart.
    constexpr int order = 11, size = 1 << order;
    juce::dsp::FFT fft (order);
    std::vector<float> work ((size_t) size * 2), power ((size_t) size / 2, 0.0f);

    for (int start = 0; start + size <= length; start += size / 2)
    {
        for (int i = 0; i < size; ++i)
            work[(size_t) i] = 0.5f * (left[start + i] + right[start + i]) * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) size));

        std::fill (work.begin() + size, work.end(), 0.0f);
        fft.performFrequencyOnlyForwardTransform (work.data());

        for (int i = 0; i < size / 2; ++i)
            power[(size_t) i] += work[(size_t) i] * work[(size_t) i];
    }

    constexpr double lowest = 40.0, highest = 16000.0;
    const auto binWidth = sampleRate / size;
    const auto powerAt = [&] (double hz)
    {
        const auto x = juce::jlimit (0.0, (double) (size / 2 - 2), hz / binWidth);
        const auto i = (int) x;
        return (double) power[(size_t) i] + (x - i) * ((double) power[(size_t) i + 1] - (double) power[(size_t) i]);
    };

    // 48 bands, each as wide in pitch as the next: the power in a band is the density at its middle times its width.
    std::array<double, Signature::bands> band {};
    double most = 0.0, total = 0.0, weighted = 0.0;

    for (int b = 0; b < Signature::bands; ++b)
    {
        const auto from = lowest * std::pow (highest / lowest, (double) b / Signature::bands), to = lowest * std::pow (highest / lowest, (double) (b + 1) / Signature::bands);
        band[(size_t) b] = powerAt (std::sqrt (from * to)) * (to - from) / binWidth;
        most = juce::jmax (most, band[(size_t) b]);
        total += band[(size_t) b];
        weighted += band[(size_t) b] * ((double) b + 0.5) / Signature::bands;
    }

    if (most <= 0.0)
        return s;

    for (int b = 0; b < Signature::bands; ++b)
        s.band[(size_t) b] = (float) juce::jlimit (0.0, 1.0, (10.0 * std::log10 (juce::jmax (1.0e-30, band[(size_t) b] / most)) + 48.0) / 48.0);

    s.brightness = (float) (weighted / total);

    // How like noise: the bins' geometric mean over their mean, between the same two frequencies.
    double logs = 0.0, sum = 0.0;
    int count = 0;

    for (int i = (int) (lowest / binWidth) + 1; i < (int) (highest / binWidth) && i < size / 2; ++i)
    {
        logs += std::log ((double) power[(size_t) i] + 1.0e-30);
        sum += power[(size_t) i];
        ++count;
    }

    s.noisiness = count > 0 && sum > 0.0 ? (float) juce::jlimit (0.0, 1.0, std::exp (logs / count) / (sum / count)) : 0.0f;

    // How wide: the difference of the sides against their sum.
    double side = 0.0, middle = 0.0;

    for (int i = 0; i < length; ++i)
    {
        side += (double) (left[i] - right[i]) * (left[i] - right[i]);
        middle += (double) (left[i] + right[i]) * (left[i] + right[i]);
    }

    s.width = middle > 0.0 ? (float) juce::jlimit (0.0, 1.0, std::sqrt (side / middle)) : 0.0f;

    // Its loudness through the second.
    auto loudest = 0.0f;

    for (int k = 0; k < Signature::steps; ++k)
    {
        const auto from = k * length / Signature::steps, to = (k + 1) * length / Signature::steps;
        s.loudness[(size_t) k] = juce::jmax (audio.getRMSLevel (0, from, to - from), audio.getNumChannels() > 1 ? audio.getRMSLevel (1, from, to - from) : 0.0f);
        loudest = juce::jmax (loudest, s.loudness[(size_t) k]);
    }

    if (loudest > 0.0f)
        for (auto& l : s.loudness)
            l /= loudest;

    return s;
}

//==============================================================================
Node nodeOf (const Patch& patch, Renderer& renderer)
{
    Node n;
    n.patch = patch;
    n.signature = renderer.measure (patch);
    n.address = addressOf (patch);
    return n;
}

std::vector<Node> nodesAround (const Patch& held, const Settings& settings, Renderer& renderer)
{
    const auto heldNode = nodeOf (held, renderer);
    auto& synth = renderer.synth();   // holds the held patch now: its parameters give the ranges
    const auto reach = juce::jlimit (0.0f, 1.0f, settings.reach);
    std::vector<Node> nodes;

    // The settings that can move, by part.
    const auto movable = movableIn (held, synth, settings.firewall);

    std::vector<int> open;

    for (int part = 0; part < numParts; ++part)
        if (! movable[(size_t) part].empty())
            open.push_back (part);

    // The line to a second patch: each node a step further along it.
    if (settings.bridge != nullptr)
    {
        for (int i = 0; i < numNodes; ++i)
        {
            const auto t = (float) (i + 1) / (float) (numNodes + 1);
            Node node;
            node.patch = held;
            node.distance = t;
            std::vector<Moved> moved;

            for (auto& [id, from] : held.values)
            {
                const auto part = partOf (id);
                const auto other = settings.bridge->values.find (id);
                auto* p = synth.parameter (id);

                if (part < 0 || settings.firewall[(size_t) part] || p == nullptr || other == settings.bridge->values.end() || juce::exactlyEqual (other->second, from))
                    continue;

                double to;

                if (dynamic_cast<juce::AudioParameterFloat*> (p) != nullptr && p->getNumSteps() > 1000)
                {
                    const auto a = p->convertTo0to1 ((float) from), b = p->convertTo0to1 ((float) other->second);
                    to = p->convertFrom0to1 (a + (b - a) * t);
                }
                else
                    to = t >= 0.5f ? other->second : from;   // a choice, a switch or a stepped setting changes over half way

                if (! juce::exactlyEqual (to, from))
                {
                    node.patch.values[id] = to;
                    moved.push_back ({ id, from, to, std::abs (p->convertTo0to1 ((float) to) - p->convertTo0to1 ((float) from)), part });
                }
            }

            if (! settings.firewall[oscillators] && t >= 0.5f)
                node.patch.tables = settings.bridge->tables;

            finish (node, moved, synth);

            if (! levelAndMeasure (node, heldNode.signature.peak, renderer))
            {
                node.patch = held;
                node.signature = heldNode.signature;
                node.address = heldNode.address;
                node.changes.clear();
            }

            nodes.push_back (std::move (node));
        }

        return nodes;
    }

    // How far each node goes, a different order for each seed: from a third of Reach to all of it.
    std::array<int, numNodes> rank {};
    std::iota (rank.begin(), rank.end(), 0);
    std::stable_sort (rank.begin(), rank.end(), [&] (int a, int b) { return uniform (settings.seed, a, 0x52414e4bu, 0) < uniform (settings.seed, b, 0x52414e4bu, 0); });
    const auto firstPart = open.empty() ? 0 : (int) (mix (settings.seed) % (juce::uint32) open.size());

    for (int i = 0; i < numNodes; ++i)
    {
        const auto place = (int) (std::find (rank.begin(), rank.end(), i) - rank.begin());
        const auto scale = reach * (0.3f + 0.7f * (float) place / (float) (numNodes - 1));
        Node node;

        // A node that makes no sound is drawn again, a few times; then it is the node held.
        for (int attempt = 0; attempt < 4; ++attempt)
        {
            node = {};
            node.patch = held;
            node.distance = scale;
            const auto seed = settings.seed + (juce::uint32) attempt * 0x51ed270bu;
            const auto moved = moveSettings (node.patch, held, movable, open, firstPart, i, attempt, seed, scale);

            finish (node, moved, synth);

            if (moved.empty())
            {
                node.signature = heldNode.signature;
                node.address = heldNode.address;

                if (open.empty() || scale <= 0.0f)
                    break;   // nothing can move: the node held

                continue;
            }

            if (levelAndMeasure (node, heldNode.signature.peak, renderer))
                break;

            node = {};
            node.patch = held;
            node.signature = heldNode.signature;
            node.address = heldNode.address;
        }

        nodes.push_back (std::move (node));
    }

    return nodes;
}

std::vector<Patch> variations (const Patch& held, Synth& rangesFrom, float reach, juce::uint32 seed, const std::array<bool, numParts>& firewall, int count)
{
    const auto movable = movableIn (held, rangesFrom, firewall);
    std::vector<int> open;

    for (int part = 0; part < numParts; ++part)
        if (! movable[(size_t) part].empty())
            open.push_back (part);

    std::vector<Patch> made;

    if (open.empty() || count <= 0)
        return made;

    // As nodesAround moves its twelve, without rendering them: from a third of reach to all of it, each part in turn.
    const auto firstPart = (int) (mix (seed) % (juce::uint32) open.size());

    for (int i = 0; i < count; ++i)
    {
        const auto scale = juce::jlimit (0.0f, 1.0f, reach) * (0.3f + 0.7f * (float) i / (float) juce::jmax (1, count - 1));
        auto patch = held;

        if (! moveSettings (patch, held, movable, open, firstPart, i, 0, seed, scale).empty())
            made.push_back (std::move (patch));
    }

    return made;
}

std::vector<Patch> sweeps (const Patch& held, Synth& rangesFrom, const std::array<bool, numParts>& firewall, int first, int settings, int points, int& total)
{
    // The settings a sweep can take, the ones that shape the sound most first: the filters (an off
    // one too, so a high-pass can come in), levels, the amp's envelope, tables and pitches; then the rest.
    static const juce::StringArray firstOnes { "Type", "Cutoff", "Level", "Volume", "Table", "Position", "Pitch", "Resonance", "Attack", "Decay", "Sustain", "Release" };
    std::vector<std::pair<int, juce::String>> order;

    for (auto& [id, value] : held.values)
    {
        juce::ignoreUnused (value);
        const auto part = partOf (id);

        if (part < 0 || firewall[(size_t) part] || rangesFrom.parameter (id) == nullptr)
            continue;

        const auto filterType = id.startsWith ("filter") && id.endsWith ("Type") && id.length() == 11;

        if (! filterType && ruleFor (id, held.values).kind == Kind::fixed)
            continue;

        int rank = firstOnes.size();

        for (int k = 0; k < firstOnes.size(); ++k)
            if (id.endsWith (firstOnes[k]))
            {
                rank = k;
                break;
            }

        if (id.startsWith ("env") && ! id.startsWith ("env1"))
            rank += firstOnes.size();   // the amp's envelope before the others

        order.push_back ({ rank, id });
    }

    std::sort (order.begin(), order.end());
    total = (int) order.size();
    std::vector<Patch> made;

    if (order.empty() || settings <= 0)
        return made;

    for (int k = 0; k < settings && k < total; ++k)
    {
        const auto& id = order[(size_t) ((first + k) % total)].second;
        auto* p = rangesFrom.parameter (id);
        const auto from = valueOf (held.values, id);
        const auto range = p->getNormalisableRange();
        std::vector<double> to;

        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (p))
        {
            for (int c = 0; c < choice->choices.size(); ++c)
                if (c != (int) std::lround (from) && ! (id.endsWith ("Table") && c == (int) WaveTables::Table::sampled
                                                        && held.tables[(size_t) juce::jlimit (0, 2, id.substring (3, 4).getIntValue() - 1)] == nullptr))
                    to.push_back (c);
        }
        else if (dynamic_cast<juce::AudioParameterBool*> (p) != nullptr)
            to.push_back (from > 0.5 ? 0.0 : 1.0);
        else if (id.endsWith ("Pitch"))
        {
            for (auto by : { -24.0, -12.0, -7.0, -5.0, 5.0, 7.0, 12.0, 24.0 })
                to.push_back (juce::jlimit ((double) range.start, (double) range.end, from + by));
        }
        else if (id.endsWith ("Unison"))
        {
            for (int u = (int) range.start; u <= (int) range.end; ++u)
                to.push_back (u);
        }
        else
        {
            // Across its whole range, and a little either side of where it is.
            const auto was = p->convertTo0to1 ((float) from);

            for (int i = 0; i < points; ++i)
                to.push_back (p->convertFrom0to1 ((float) i / (float) juce::jmax (1, points - 1)));

            for (auto by : { -0.04f, 0.04f })
                to.push_back (p->convertFrom0to1 (juce::jlimit (0.0f, 1.0f, was + by)));
        }

        for (auto value : to)
            if (! juce::approximatelyEqual (value, from))
            {
                auto patch = held;
                patch.values[id] = value;
                made.push_back (std::move (patch));
            }
    }

    return made;
}

std::vector<Patch> fewMoves (const Patch& held, Synth& rangesFrom, float reach, juce::uint32 seed, const std::array<bool, numParts>& firewall, int count)
{
    const auto movable = movableIn (held, rangesFrom, firewall);
    std::vector<int> open;

    for (int part = 0; part < numParts; ++part)
        if (! movable[(size_t) part].empty())
            open.push_back (part);

    std::vector<Patch> made;

    if (open.empty() || count <= 0)
        return made;

    const auto firstPart = (int) (mix (seed) % (juce::uint32) open.size());

    for (int i = 0; i < count; ++i)
    {
        // One part in turn, and in it one, two or three of its settings, chosen at random.
        const auto part = open[(size_t) ((firstPart + i) % (int) open.size())];
        const auto& in = movable[(size_t) part];
        std::array<std::vector<Movable>, numParts> chosen;
        const auto many = juce::jmin ((int) in.size(), 1 + i % 3);

        for (int k = 0; k < many; ++k)
        {
            const auto pick = juce::jmin ((int) in.size() - 1, (int) (uniform (seed, i, 0x4645574du, k) * (float) in.size()));
            chosen[(size_t) part].push_back (in[(size_t) pick]);
        }

        auto patch = held;

        if (! moveSettings (patch, held, chosen, { part }, 0, i, 0, mix (seed + (juce::uint32) i), juce::jlimit (0.0f, 1.0f, reach)).empty())
            made.push_back (std::move (patch));
    }

    return made;
}

//==============================================================================
bool reduceMotion()
{
   #if JUCE_MAC
    Boolean set = false;
    const auto on = CFPreferencesGetAppBooleanValue (CFSTR ("reduceMotion"), CFSTR ("com.apple.universalaccess"), &set);
    return set && on;
   #elif JUCE_WINDOWS
    BOOL animations = TRUE;
    return SystemParametersInfo (SPI_GETCLIENTAREAANIMATION, 0, &animations, 0) && ! animations;
   #else
    return false;
   #endif
}

//==============================================================================
/** Makes the twelve off the message thread: the newest request only. */
class Console::Scanner final : public juce::Thread
{
public:
    explicit Scanner (Console& c) : juce::Thread ("Torqwave Synth console"), console (c)   { startThread (juce::Thread::Priority::low); }

    ~Scanner() override
    {
        signalThreadShouldExit();
        wake.signal();
        stopThread (20000);
    }

    void request (Patch patch, Settings s, juce::uint32 scan)
    {
        {
            const juce::ScopedLock lock (section);
            job = { std::move (patch), std::move (s), scan };
            waiting = true;
        }

        wake.signal();
    }

    void run() override
    {
        Renderer own;

        while (! threadShouldExit())
        {
            wake.wait (-1);
            Job now;

            {
                const juce::ScopedLock lock (section);

                if (! waiting)
                    continue;

                now = job;
                waiting = false;
            }

            auto made = nodesAround (now.patch, now.settings, own);

            if (threadShouldExit())
                return;

            juce::MessageManager::callAsync ([&c = console, stillThere = console.alive, twelve = std::move (made), scan = now.scan]() mutable
            {
                if (stillThere->load())
                    c.install (std::move (twelve), scan);
            });
        }
    }

private:
    struct Job { Patch patch; Settings settings; juce::uint32 scan = 0; };
    Console& console;
    juce::CriticalSection section;
    juce::WaitableEvent wake;
    Job job;
    bool waiting = false;
};

Console::Console (Synth& s) : synth (s)
{
    player = std::make_unique<Synth>();
    player->setPlayConfigDetails (0, 2, rate, 512);
    hold (capture (synth), false);
    route.push_back (held);
    position = 0;
}

Console::~Console()
{
    alive->store (false);
    scanner.reset();
}

void Console::hold (const Patch& patch, bool addToRoute)
{
    held = nodeOf (patch, renderer);

    if (addToRoute)
    {
        route.push_back (held);

        if (route.size() > 32)
            route.erase (route.begin());

        position = (int) route.size() - 1;
    }
}

void Console::startScan()
{
    ++scanCount;
    scanning = true;

    if (scanner == nullptr)
        scanner = std::make_unique<Scanner> (*this);

    scanner->request (held.patch, settings, scanCount);
}

void Console::install (std::vector<Node> made, juce::uint32 scan)
{
    if (scan != scanCount)
        return;   // asked again since: these are for a node or settings that have moved on

    nodes = std::move (made);
    scanning = false;

    if (onChange != nullptr)
        onChange();
}

void Console::scanNow()
{
    ++scanCount;   // anything on its way from the thread is dropped
    nodes = nodesAround (held.patch, settings, renderer);
    scanning = false;

    if (onChange != nullptr)
        onChange();
}

void Console::setReach (float reach)
{
    reach = juce::jlimit (0.0f, 1.0f, reach);

    if (! juce::exactlyEqual (reach, settings.reach))
    {
        settings.reach = reach;
        startScan();
    }
}

void Console::setFirewall (int part, bool on)
{
    if (juce::isPositiveAndBelow (part, (int) numParts) && settings.firewall[(size_t) part] != on)
    {
        settings.firewall[(size_t) part] = on;
        startScan();
    }
}

void Console::setBridge (std::shared_ptr<const Patch> other)
{
    settings.bridge = std::move (other);
    startScan();
}

void Console::rescan()
{
    ++settings.seed;
    startScan();
}

void Console::hear (int node)
{
    const auto* which = node < 0 ? &held : juce::isPositiveAndBelow (node, (int) nodes.size()) ? &nodes[(size_t) node] : nullptr;

    if (which == nullptr)
        return;

    apply (*player, which->patch);
    strike.store (lastNote.load (std::memory_order_relaxed), std::memory_order_release);
}

void Console::play (const juce::AudioBuffer<float>& audio, double sampleRate)
{
    int slot = 0;

    while (slot == soundPlaying.load (std::memory_order_acquire) || slot == soundReady.load (std::memory_order_acquire))
        ++slot;

    // At the synth's rate, two channels.
    const auto ratio = sampleRate / rate;
    const auto length = juce::jmax (1, (int) (audio.getNumSamples() / ratio));
    auto& out = sounds[(size_t) slot];
    out.setSize (2, length);

    for (int ch = 0; ch < 2; ++ch)
    {
        juce::LagrangeInterpolator interpolator;
        interpolator.process (ratio, audio.getReadPointer (juce::jmin (ch, audio.getNumChannels() - 1)), out.getWritePointer (ch), length,
                              audio.getNumSamples(), 0);
    }

    soundReady.store (slot, std::memory_order_release);
}

void Console::jump (int node)
{
    if (! juce::isPositiveAndBelow (node, (int) nodes.size()))
        return;

    auto patch = nodes[(size_t) node].patch;
    patch.name = "Node " + addressText (nodes[(size_t) node].address);
    apply (synth, patch);
    // What the synth holds now is the node: its parameters may have taken a value a hair off the one asked for.
    hold (capture (synth), true);
    nodes.clear();
    startScan();

    if (onChange != nullptr)
        onChange();
}

void Console::goTo (int routeIndex)
{
    if (! juce::isPositiveAndBelow (routeIndex, (int) route.size()) || routeIndex == position)
        return;

    apply (synth, route[(size_t) routeIndex].patch);
    held = route[(size_t) routeIndex];
    position = routeIndex;
    nodes.clear();
    startScan();

    if (onChange != nullptr)
        onChange();
}

bool Console::sync()
{
    auto now = capture (synth);

    if (addressOf (now) == held.address)
        return false;

    hold (now, true);
    nodes.clear();
    startScan();

    if (onChange != nullptr)
        onChange();

    return true;
}

void Console::prepare (double sampleRate, int blockSize)
{
    rate = sampleRate;
    player->setPlayConfigDetails (0, 2, sampleRate, blockSize);
    player->prepareToPlay (sampleRate, blockSize);
    scratch.setSize (2, juce::jmax (blockSize, 8192), false, true, false);
    sounding = -1;
    remaining = tail = 0;
}

void Console::addAudition (juce::AudioBuffer<float>& out) noexcept
{
    const auto n = out.getNumSamples();

    // A sound played.
    if (const auto ready = soundReady.exchange (-1, std::memory_order_acq_rel); ready >= 0)
    {
        soundPlaying.store (ready, std::memory_order_release);
        soundAt = 0;
    }

    if (const auto playing = soundPlaying.load (std::memory_order_relaxed); playing >= 0)
    {
        const auto& sound = sounds[(size_t) playing];
        const auto count = juce::jmin (n, sound.getNumSamples() - soundAt);

        for (int ch = 0; ch < out.getNumChannels() && count > 0; ++ch)
            out.addFrom (ch, 0, sound, ch % 2, soundAt, count);

        soundAt += juce::jmax (0, count);

        if (soundAt >= sound.getNumSamples())
            soundPlaying.store (-1, std::memory_order_release);
    }

    const auto note = strike.exchange (-1, std::memory_order_acq_rel);

    if ((note < 0 && sounding < 0 && tail <= 0) || n > scratch.getNumSamples() || out.getNumChannels() < 1)
        return;

    juce::MidiBuffer midi;

    if (note >= 0)
    {
        // A node struck while another sounds: that one is cut (it fades over 4 ms), this one starts.
        if (sounding >= 0)
            midi.addEvent (juce::MidiMessage::allSoundOff (1), 0);

        midi.addEvent (juce::MidiMessage::noteOn (1, note, 0.8f), 0);
        sounding = note;
        remaining = (int) (0.6 * rate);
        tail = (int) (8.0 * rate);
    }

    if (sounding >= 0)
    {
        if (remaining < n)
        {
            midi.addEvent (juce::MidiMessage::noteOff (1, sounding), juce::jmax (0, remaining));
            sounding = -1;
        }

        remaining -= n;
    }
    else
        tail -= n;

    juce::AudioBuffer<float> block (scratch.getArrayOfWritePointers(), 2, n);
    block.clear();
    player->processBlock (block, midi);

    for (int ch = 0; ch < out.getNumChannels(); ++ch)
        out.addFrom (ch, 0, block, ch % 2, 0, n);
}
}
