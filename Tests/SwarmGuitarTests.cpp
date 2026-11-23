#include <JuceHeader.h>
#include "../Source/AudioEngine.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/InstrumentKeys.h"
#include "../Source/SwarmGuitar.h"
#include "../Source/SwarmGuitarEditor.h"
#include "../Source/Theme.h"
#include "TestUtils.h"
#include "SwarmGuitarTests.h"
#include "ClickCheck.h"

namespace
{
    using TestUtils::check;
    using Swarm = BuiltInEffects::SwarmGuitarProcessor;

    constexpr double rate = 48000.0;

    struct Event { double when; juce::MidiMessage message; };

    Event on (double when, int note, float velocity = 1.0f)   { return { when, juce::MidiMessage::noteOn (1, note, velocity) }; }
    Event off (double when, int note)                         { return { when, juce::MidiMessage::noteOff (1, note) }; }

    /** Plays the events for seconds in blocks of blockSize, from a fresh start. */
    juce::AudioBuffer<float> play (Swarm& guitar, const std::vector<Event>& events, double seconds, int blockSize = 512)
    {
        guitar.prepareToPlay (rate, blockSize);
        const auto total = (int) (seconds * rate);
        juce::AudioBuffer<float> out (2, total), block (2, blockSize);

        for (int at = 0; at < total; at += blockSize)
        {
            const auto n = juce::jmin (blockSize, total - at);
            juce::MidiBuffer midi;

            for (auto& e : events)
                if (const auto sample = (int) (e.when * rate); sample >= at && sample < at + n)
                    midi.addEvent (e.message, sample - at);

            block.setSize (2, n, false, false, true);
            guitar.processBlock (block, midi);

            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, at, block, ch, 0, n);
        }

        return out;
    }

    void set (Swarm& guitar, const juce::String& id, float plain)
    {
        for (auto* p : guitar.getParameters())
            if (auto* hosted = dynamic_cast<juce::AudioProcessorParameterWithID*> (p); hosted != nullptr && hosted->paramID == id)
            {
                if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
                    p->setValueNotifyingHost (ranged->convertTo0to1 (plain));

                return;
            }

        jassertfalse;
    }

    void defaults (Swarm& guitar)
    {
        for (auto* p : guitar.getParameters())
            p->setValueNotifyingHost (p->getDefaultValue());
    }

    bool same (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples())
            return false;

        for (int c = 0; c < a.getNumChannels(); ++c)
            if (std::memcmp (a.getReadPointer (c), b.getReadPointer (c), sizeof (float) * (size_t) a.getNumSamples()) != 0)
                return false;

        return true;
    }

    /** The pitch of channel 0 over a stretch, from its autocorrelation's peak nearest the period expected. */
    double pitchOf (const juce::AudioBuffer<float>& audio, int from, int length, double expectedHz)
    {
        const auto* x = audio.getReadPointer (0) + from;
        const auto around = rate / expectedHz;

        auto correlation = [&] (int lag)
        {
            double sum = 0.0;

            for (int i = 0; i + lag < length; ++i)
                sum += (double) x[i] * x[i + lag];

            return sum;
        };

        int best = (int) around;
        double most = -1.0e30;

        for (int lag = (int) (around * 0.94); lag <= (int) (around * 1.06) + 1; ++lag)
            if (const auto c = correlation (lag); c > most)
            {
                most = c;
                best = lag;
            }

        const auto before = correlation (best - 1), after = correlation (best + 1);
        const auto shift = 0.5 * (before - after) / (before - 2.0 * most + after);
        return rate / (best + shift);
    }

    double centsOff (double hz, double wanted)     { return 1200.0 * std::log2 (hz / wanted); }

    float largestJump (const juce::AudioBuffer<float>& audio, int from, int length)
    {
        float jump = 0.0f;

        for (int c = 0; c < audio.getNumChannels(); ++c)
            for (int i = juce::jmax (1, from); i < from + length; ++i)
                jump = juce::jmax (jump, std::abs (audio.getSample (c, i) - audio.getSample (c, i - 1)));

        return jump;
    }

    float rms (const juce::AudioBuffer<float>& audio, double fromSeconds, double seconds)
    {
        return audio.getRMSLevel (0, (int) (fromSeconds * rate), (int) (seconds * rate));
    }

    /** How many times channel 0 rises through 0 over a stretch: a sawtooth's cycles. */
    int risings (const juce::AudioBuffer<float>& audio, double fromSeconds, double seconds)
    {
        const auto* x = audio.getReadPointer (0);
        int count = 0;

        for (auto i = juce::jmax (1, (int) (fromSeconds * rate)); i < (int) ((fromSeconds + seconds) * rate); ++i)
            count += x[i - 1] < 0.0f && x[i] >= 0.0f ? 1 : 0;

        return count;
    }

    /** How alike channel 0 is to itself a period later (1 the same, near 0 nothing alike): how much one pitch it is. */
    double periodicity (const juce::AudioBuffer<float>& audio, double fromSeconds, double seconds, double hz)
    {
        const auto* x = audio.getReadPointer (0) + (int) (fromSeconds * rate);
        const auto length = (int) (seconds * rate), lag = juce::roundToInt (rate / hz);
        double together = 0.0, alone = 0.0;

        for (int i = 0; i + lag < length; ++i)
        {
            together += (double) x[i] * x[i + lag];
            alone += (double) x[i] * x[i];
        }

        return alone > 0.0 ? together / alone : 0.0;
    }

    /** As it comes, then playing a swarm: no amp, one guitar, the filter wide open, no drift. */
    void swarm (Swarm& guitar)
    {
        defaults (guitar);
        set (guitar, "source", 1.0f);
        set (guitar, "amp", 2.0f);
        set (guitar, "layers", 1.0f);
        set (guitar, "drift", 0.0f);
        set (guitar, "detune", 0.0f);
        set (guitar, "cutoff", 18000.0f);
        set (guitar, "resonance", 0.0f);
        set (guitar, "filterEnv", 0.0f);
        set (guitar, "brightness", 100.0f);
    }
}

void runSwarmGuitarTests (AudioEngine& engine)
{

    juce::PluginDescription description;

    for (auto& d : BuiltInEffects::getDescriptions())
        if (d.fileOrIdentifier == BuiltInEffects::swarmGuitarId)
            description = d;

    juce::String error;
    auto instance = engine.pluginFormatManager.createPluginInstance (description, rate, 512, error);
    auto* guitar = dynamic_cast<Swarm*> (instance.get());

    check (guitar != nullptr && error.isEmpty() && description.isInstrument && description.numInputChannels == 0 && description.name == "StringSwarm"
               && guitar->acceptsMidi() && guitar->getTotalNumInputChannels() == 0 && guitar->getTotalNumOutputChannels() == 2
               && guitar->getParameters().size() == 28,
           "stringswarm: made from its description, an instrument with no audio input, a stereo output and 28 parameters");

    if (guitar == nullptr)
        return;

    // Every string on the played note: Three Octaves on six strings is two an octave down, three on the note and one an octave up.
    {
        juce::Array<int> three, low, unison;

        for (int s = 0; s < 6; ++s)
        {
            three.add (Swarm::octaveOf (2, s, 6));
            low.add (Swarm::octaveOf (1, s, 6));
            unison.add (Swarm::octaveOf (0, s, 6));
        }

        check (three == juce::Array<int> { -12, -12, 0, 0, 0, 12 } && low == juce::Array<int> { -12, -12, 0, 0, 0, 0 } && unison == juce::Array<int> { 0, 0, 0, 0, 0, 0 },
               "stringswarm: six strings tuned Unison are all on the note; Low Octave puts two an octave down; Three Octaves two down, three on the note and one up");
    }

    // In tune: one string, no amp, is at the note's pitch.
    {
        defaults (*guitar);
        set (*guitar, "strings", 1.0f);
        set (*guitar, "layers", 1.0f);
        set (*guitar, "detune", 0.0f);
        set (*guitar, "tuning", 0.0f);
        set (*guitar, "amp", 2.0f);

        double worst = 0.0;
        juce::StringArray found;

        for (auto [note, hz] : { std::pair { 33, 55.0 }, std::pair { 45, 110.0 }, std::pair { 57, 220.0 }, std::pair { 69, 440.0 }, std::pair { 81, 880.0 } })
        {
            const auto out = play (*guitar, { on (0.0, note) }, 1.0);
            const auto cents = centsOff (pitchOf (out, (int) (0.3 * rate), (int) (0.5 * rate), hz), hz);
            worst = std::isfinite (cents) ? juce::jmax (worst, std::abs (cents)) : 1.0e9;
            found.add (juce::String (cents, 1));
        }

        check (worst < 3.0, "stringswarm: one string with no amp is in tune from A1 to A5, within " + juce::String (worst, 1) + " cents (" + found.joinIntoString (", ") + ")");
    }

    // As it comes: loud enough, not clipping, the two guitars apart in stereo; one guitar is in the middle.
    defaults (*guitar);
    const auto chord = std::vector<Event> { on (0.0, 38), on (0.0, 45), off (1.0, 38), off (1.0, 45) };
    const auto asItComes = play (*guitar, chord, 2.0);

    {
        const auto peak = juce::Decibels::gainToDecibels (asItComes.getMagnitude (0, asItComes.getNumSamples()));
        auto apart = false;

        for (int i = 0; i < asItComes.getNumSamples() && ! apart; ++i)
            apart = ! juce::exactlyEqual (asItComes.getSample (0, i), asItComes.getSample (1, i));

        set (*guitar, "layers", 1.0f);
        const auto one = play (*guitar, chord, 1.0);
        auto middle = true;

        for (int i = 0; i < one.getNumSamples(); ++i)
            middle = middle && juce::exactlyEqual (one.getSample (0, i), one.getSample (1, i));

        check (peak > -24.0f && peak < -1.0f && apart && middle && one.getMagnitude (0, one.getNumSamples()) > 0.05f,
               "stringswarm: as it comes, a D and A held a second peak at " + juce::String (peak, 1) + " dB, its two guitars different left and right; one guitar is the same on both");
    }

    // The same twice and at any block size, strummed again and again as well.
    {
        defaults (*guitar);
        set (*guitar, "strumRate", 9.0f);
        const auto a = play (*guitar, chord, 1.5), b = play (*guitar, chord, 1.5), small = play (*guitar, chord, 1.5, 64), odd = play (*guitar, chord, 1.5, 37);
        check (same (a, b) && same (a, small) && same (a, odd), "stringswarm: a part renders the same twice and in blocks of 512, 64 and 37, with Strum Rate on");
    }

    // Strum Rate keeps it going: with a short sustain, a held note has died away two seconds in unless it is strummed again.
    {
        defaults (*guitar);
        set (*guitar, "sustain", 0.3f);
        const auto once = play (*guitar, { on (0.0, 45) }, 3.0);
        set (*guitar, "strumRate", 8.0f);
        const auto again = play (*guitar, { on (0.0, 45) }, 3.0);
        const auto quiet = rms (once, 2.0, 1.0), loud = rms (again, 2.0, 1.0);
        check (loud > 0.02f && quiet < loud * 0.05f,
               "stringswarm: with a 0.3 s sustain a held note is at " + juce::String (quiet, 4) + " RMS two seconds in, and at " + juce::String (loud, 3) + " strummed 8 times a second");
    }

    // Let go, a note is damped over the release; held, it rings on.
    {
        defaults (*guitar);
        const auto held = play (*guitar, { on (0.0, 45) }, 2.0), let = play (*guitar, { on (0.0, 45), off (0.5, 45) }, 2.0);
        const auto ringing = rms (held, 1.2, 0.5), damped = rms (let, 1.2, 0.5);
        check (ringing > 0.02f && damped < ringing * 0.01f && guitar->getNumSounding() == 0,
               "stringswarm: a held note rings on (" + juce::String (ringing, 3) + " RMS after 1.2 s); let go at 0.5 s it is damped to " + juce::String (damped, 5) + " and its voice is free");
    }

    // Nothing stops dead: All Sound Off in the middle of a fuzzed chord moves the output no more a sample than
    // the playing did, and it is silent 100 ms on.
    {
        defaults (*guitar);
        const auto cut = play (*guitar, { on (0.0, 38), on (0.0, 45), on (0.0, 50), { 0.5, juce::MidiMessage::allSoundOff (1) } }, 1.0);
        const auto at = (int) (0.5 * rate);
        const auto playing = largestJump (cut, at - (int) (0.2 * rate), (int) (0.2 * rate)), stopping = largestJump (cut, at, (int) (0.1 * rate));
        const auto after = cut.getMagnitude (at + (int) (0.1 * rate), (int) (0.3 * rate));
        check (stopping <= playing && after < 1.0e-4f && guitar->getNumSounding() == 0,
               "stringswarm: All Sound Off in a fuzzed chord moves the output at most " + juce::String (stopping, 3) + " a sample (" + juce::String (playing, 3)
                   + " while it played) and leaves " + juce::String (after, 6) + " 100 ms on");
    }

    // Six notes at once: a seventh takes the oldest's place, which fades over 20 ms.
    {
        defaults (*guitar);
        std::vector<Event> notes;

        for (int i = 0; i < 7; ++i)
            notes.push_back (on (0.05 * i, 40 + i));

        play (*guitar, notes, 0.3 + 0.005);
        const auto fading = guitar->getNumSounding();
        const auto out = play (*guitar, notes, 0.5);
        const auto at = (int) (0.3 * rate);
        check (fading == 7 && guitar->getNumSounding() == 6 && largestJump (out, at, (int) (0.03 * rate)) <= largestJump (out, at - (int) (0.2 * rate), (int) (0.2 * rate)) * 1.5f,
               "stringswarm: a seventh note takes the oldest's place, seven sounding 5 ms on and six once it has faded, with no jump larger than the playing's");
    }

    // More strings beat more: six detuned strings through the fuzz move in level over a second far more than one string does.
    {
        auto wobble = [&] (float count)
        {
            defaults (*guitar);
            set (*guitar, "layers", 1.0f);
            set (*guitar, "tuning", 0.0f);
            set (*guitar, "amp", 2.0f);
            set (*guitar, "detune", 25.0f);
            set (*guitar, "strings", count);
            const auto out = play (*guitar, { on (0.0, 57) }, 2.0);
            float rise = 0.0f;

            // How far the level climbs from one 50 ms to the next: a lone string only dies away.
            for (int i = 0; i < 20; ++i)
                rise = juce::jmax (rise, rms (out, 0.55 + 0.05 * i, 0.05) / juce::jmax (1.0e-9f, rms (out, 0.5 + 0.05 * i, 0.05)));

            return rise;
        };

        const auto one = wobble (1.0f), six = wobble (6.0f);
        check (one <= 1.0f && six > 1.1f, "stringswarm: six strings up to 25 cents apart beat against each other, their level climbing by up to " + juce::String (six, 2)
                                              + " times from one 50 ms to the next; one string's only falls (" + juce::String (one, 3) + " at most)");
    }

    // The swarm: oscillators in place of the strings.
    {
        swarm (*guitar);
        set (*guitar, "strings", 1.0f);
        set (*guitar, "tuning", 0.0f);
        double worst = 0.0;

        for (int note = 33; note <= 81; note += 12)
        {
            const auto hz = 440.0 * std::pow (2.0, (note - 69) / 12.0);
            const auto out = play (*guitar, { on (0.0, note) }, 0.6);
            worst = juce::jmax (worst, std::abs (centsOff (pitchOf (out, (int) (0.2 * rate), (int) (0.3 * rate), hz), hz)));
        }

        check (worst < 3.0, "stringswarm: one swarm oscillator, no drift, is in tune from A1 to A5, within " + juce::String (worst, 2) + " cents");
    }

    {
        // Everything moving at once: glide, converging, drift, Strum Rate, the filter's envelope, three guitars through the fuzz.
        swarm (*guitar);
        set (*guitar, "amp", 0.0f);
        set (*guitar, "layers", 3.0f);
        set (*guitar, "strings", 8.0f);
        set (*guitar, "drift", 60.0f);
        set (*guitar, "glide", 300.0f);
        set (*guitar, "converge", 800.0f);
        set (*guitar, "spread", 200.0f);
        set (*guitar, "strumRate", 9.0f);
        set (*guitar, "strum", 6.0f);
        set (*guitar, "cutoff", 900.0f);
        set (*guitar, "resonance", 50.0f);
        set (*guitar, "filterEnv", 70.0f);
        set (*guitar, "decay", 150.0f);
        set (*guitar, "sustainLevel", 10.0f);
        const std::vector<Event> part { on (0.0, 40), on (0.3, 47), off (0.6, 40), on (0.7, 43), off (1.1, 47), off (1.3, 43) };
        const auto a = play (*guitar, part, 1.8), b = play (*guitar, part, 1.8), small = play (*guitar, part, 1.8, 64), odd = play (*guitar, part, 1.8, 37);
        check (same (a, b) && same (a, small) && same (a, odd) && a.getMagnitude (0, a.getNumSamples()) > 0.01f,
               "stringswarm: a swarm part (glide, converging, drift, Strum Rate, three fuzzed guitars) renders the same twice and in blocks of 512, 64 and 37");
    }

    {
        // Glide: a note an octave up starts from the last and gets there.
        swarm (*guitar);
        set (*guitar, "strings", 1.0f);
        set (*guitar, "tuning", 0.0f);
        set (*guitar, "glide", 1000.0f);
        set (*guitar, "release", 20.0f);
        const auto out = play (*guitar, { on (0.0, 45), off (0.4, 45), on (0.5, 57) }, 3.0);
        const auto before = risings (out, 0.2, 0.2), early = risings (out, 0.505, 0.05), late = risings (out, 2.6, 0.2);
        check (std::abs (before - 22) <= 1 && early <= 7 && std::abs (late - 44) <= 1,
               "stringswarm: Glide slides from A2 (" + juce::String (before) + " cycles in 0.2 s) towards A3: " + juce::String (early)
                   + " cycles in the 50 ms after the new note, " + juce::String (late) + " two seconds on");
    }

    {
        // Converge closes a wide fan to one pitch; below 0 it opens one pitch to the fan.
        const auto alike = [&] (float time, double at)
        {
            swarm (*guitar);
            set (*guitar, "strings", 8.0f);
            set (*guitar, "tuning", 0.0f);
            set (*guitar, "spread", 600.0f);
            set (*guitar, "converge", time);
            const auto out = play (*guitar, { on (0.0, 45) }, 2.5);
            return periodicity (out, at, at < 1.0 ? 0.05 : 0.2, 110.0);
        };

        const auto closingEarly = alike (300.0f, 0.02), closingLate = alike (300.0f, 2.2), openingEarly = alike (-300.0f, 0.02), openingLate = alike (-300.0f, 2.2);
        check (closingEarly < 0.85 && closingLate > 0.95 && openingEarly > 0.9 && openingLate < 0.6,
               "stringswarm: a 600-cent fan converging over 300 ms is one pitch 2 s on (alike a period later " + juce::String (closingEarly, 2) + " at first, "
                   + juce::String (closingLate, 2) + " then), and below 0 opens from one (" + juce::String (openingEarly, 2) + ", then " + juce::String (openingLate, 2) + ")");
    }

    {
        // Strum Rate fires the envelope again: a held note with nothing to sustain pulses.
        swarm (*guitar);
        set (*guitar, "strumRate", 8.0f);
        set (*guitar, "decay", 100.0f);
        set (*guitar, "sustainLevel", 0.0f);
        const auto out = play (*guitar, { on (0.0, 45) }, 1.3);
        const auto pulse = rms (out, 1.0, 0.02), between = rms (out, 1.1, 0.02);
        check (pulse > 0.02f && between < pulse * 0.1f, "stringswarm: Strum Rate at 8 /s fires a swarm's envelope again, " + juce::String (pulse, 3)
                                                             + " RMS as a pulse begins, " + juce::String (between, 4) + " 100 ms on");
    }

    {
        // Strum Sync: the pulses in note values at the session's tempo, from the note's start.
        struct Head final : juce::AudioPlayHead
        {
            juce::Optional<PositionInfo> getPosition() const override  { PositionInfo p; p.setBpm (90.0); return p; }
        } head;

        swarm (*guitar);
        set (*guitar, "strumRate", 3.0f);
        set (*guitar, "strumSync", 4.0f);
        set (*guitar, "decay", 60.0f);
        set (*guitar, "sustainLevel", 0.0f);
        guitar->setPlayHead (&head);
        const auto out = play (*guitar, { on (0.0, 45) }, 1.3);
        guitar->setPlayHead (nullptr);
        const auto sixteenth = 60.0 / 90.0 / 4.0;
        const auto pulse = rms (out, 6.0 * sixteenth, 0.015), between = rms (out, 6.6 * sixteenth, 0.015);
        check (pulse > 0.02f && between < pulse * 0.1f, "stringswarm: Strum Sync at 1/16 pulses every 16th at the session's 90 BPM, not Strum Rate's 3 /s ("
                                                             + juce::String (pulse, 3) + " RMS as the 7th begins, " + juce::String (between, 4) + " 60 % of the way to the 8th)");
    }

    {
        // Nothing stops dead: the attack from silence, a release, a steal and All Sound Off move the output no more than playing does.
        swarm (*guitar);
        set (*guitar, "strings", 8.0f);
        set (*guitar, "amp", 0.0f);
        set (*guitar, "attack", 0.0f);
        set (*guitar, "release", 20.0f);
        std::vector<Event> events;

        for (int i = 0; i < 7; ++i)
            events.push_back (on (0.2 * i, 40 + 3 * i));

        events.push_back (off (1.5, 40 + 18));
        events.push_back ({ 1.8, juce::MidiMessage::allSoundOff (1) });
        const auto out = play (*guitar, events, 2.2);
        const auto playing = largestJump (out, (int) (1.0 * rate), (int) (0.2 * rate));
        const auto starting = largestJump (out, 0, (int) (0.01 * rate)), stealing = largestJump (out, (int) (1.2 * rate), (int) (0.03 * rate));
        const auto releasing = largestJump (out, (int) (1.5 * rate), (int) (0.05 * rate)), stopping = largestJump (out, (int) (1.8 * rate), (int) (0.03 * rate));
        const auto after = out.getMagnitude ((int) (1.9 * rate), (int) (0.2 * rate));
        check (starting <= playing && stealing <= playing * 1.5f && releasing <= playing * 1.5f && stopping <= playing * 1.5f && after < 1.0e-4f
                   && guitar->getNumSounding() == 0,
               "stringswarm: a fuzzed swarm moves the output at most " + juce::String (playing, 3) + " a sample playing, " + juce::String (starting, 3) + " starting, "
                   + juce::String (stealing, 3) + " as a seventh note steals, " + juce::String (releasing, 3) + " let go and " + juce::String (stopping, 3)
                   + " at All Sound Off, then silence and every voice free");
    }

    {
        // The factory presets: at least 250, each in a category, named once, setting every parameter, and each at a level that sits.
        const auto& presets = EffectPresets::factory (BuiltInEffects::swarmGuitarId);
        juce::StringArray names, categories;
        auto whole = true;

        for (auto& p : presets)
        {
            names.addIfNotAlreadyThere (p.name);
            categories.addIfNotAlreadyThere (p.category);
            whole = whole && p.category.isNotEmpty() && (int) p.values.size() == guitar->getParameters().size();

            for (auto* param : guitar->getParameters())
                whole = whole && p.values.count (dynamic_cast<juce::AudioProcessorParameterWithID*> (param)->paramID) == 1;
        }

        check (presets.size() >= 250 && names.size() == (int) presets.size() && whole && categories.contains ("Swarm Sequences"),
               "stringswarm: " + juce::String ((int) presets.size()) + " factory presets in " + juce::String (categories.size()) + " categories ("
                   + categories.joinIntoString (", ") + "), each named once and setting every parameter");

        float loudest = -100.0f, quietest = 0.0f, worstRatio = 0.0f;
        juce::String loud, quiet, worstName;
        juce::StringArray clicks;

        // A part that strikes, restrikes, steals and lets go: a chord, then 16ths over it, then All Sound Off.
        std::vector<Event> part { on (0.0, 40, 0.9f), on (0.0, 47, 0.9f), off (0.9, 40), off (0.9, 47) };

        for (int i = 0; i < 12; ++i)
        {
            const auto n = std::array<int, 4> { 45, 48, 52, 43 }[(size_t) (i % 4)];
            part.push_back (on (1.0 + 0.125 * i, n, i % 4 == 0 ? 1.0f : 0.75f));
            part.push_back (off (1.0 + 0.125 * i + 0.1, n));
        }

        part.push_back ({ 2.6, juce::MidiMessage::allSoundOff (1) });

        for (auto& p : presets)
        {
            defaults (*guitar);
            guitar->setPresetValues (p.values);
            const auto out = play (*guitar, { on (0.0, 40, 0.9f), on (0.0, 47, 0.9f), off (2.0, 40), off (2.0, 47) }, 2.5);
            const auto peak = juce::Decibels::gainToDecibels (out.getMagnitude (0, out.getNumSamples()), -100.0f);

            if (peak > loudest) { loudest = peak; loud = p.name; }
            if (peak < quietest) { quietest = peak; quiet = p.name; }

            defaults (*guitar);
            guitar->setPresetValues (p.values);
            const auto played = play (*guitar, part, 2.8);
            const auto click = ClickCheck::worst (played, rate, 0.03f);

            if (click.ratio > worstRatio) { worstRatio = click.ratio; worstName = p.name + " at " + juce::String (click.seconds, 3) + " s"; }
            if (click.ratio > 4.0f) clicks.add (p.name + " " + juce::String (click.step, 3) + " at " + juce::String (click.seconds, 3) + " s, " + juce::String (click.ratio, 1) + " times");
        }

        check (clicks.isEmpty(), "stringswarm: no factory preset clicks through a chord, 16ths over it, restrikes, steals, releases and All Sound Off: no jump in one sample of 0.03 "
                                 "or more stands alone, four times the largest step in the 30 ms either side (the nearest, " + worstName + ", "
                                     + juce::String (worstRatio, 1) + " times)" + (clicks.isEmpty() ? juce::String() : " (" + clicks.joinIntoString ("; ").substring (0, 600) + ")"));

        check (loudest < -1.0f && quietest > -24.0f, "stringswarm: every factory preset plays a held E2 and B2 between -24 and -1 dB (the loudest " + loud + " at "
                                                         + juce::String (loudest, 1) + " dB, the quietest " + quiet + " at " + juce::String (quietest, 1) + " dB)");
    }

    // Velocity through the amp: Fuzz evens a soft note out to nearly a hard one's level, so Velocity (70 % as it
    // comes) sets each note's loudness after the amp; with no amp, velocity already sets it and Velocity changes nothing.
    {
        const auto peakDb = [] (float velocity, float sense, int amp)
        {
            Swarm g;
            set (g, "velocity", sense);
            set (g, "amp", (float) amp);
            const auto out = play (g, { on (0.0, 40, velocity) }, 1.0);
            return juce::Decibels::gainToDecibels (out.getMagnitude (0, out.getNumSamples()));
        };

        const auto fuzzFlat = peakDb (1.0f, 0.0f, 0) - peakDb (0.25f, 0.0f, 0);
        const auto fuzzSensed = peakDb (1.0f, 70.0f, 0) - peakDb (0.25f, 70.0f, 0);
        const auto crunchSensed = peakDb (1.0f, 70.0f, 1) - peakDb (0.25f, 70.0f, 1);
        const auto noneSoft = peakDb (0.25f, 0.0f, 2), noneSoftSensed = peakDb (0.25f, 70.0f, 2);
        check (fuzzFlat < 3.0f && fuzzSensed > fuzzFlat + 5.0f && crunchSensed > 5.0f && juce::exactlyEqual (noneSoft, noneSoftSensed),
               "stringswarm: a note at velocity 32 against one at 127 through the Fuzz amp is " + juce::String (fuzzSensed, 1) + " dB quieter with Velocity at 70 % ("
                   + juce::String (fuzzFlat, 1) + " dB at 0 %, as the fuzz evens it out), " + juce::String (crunchSensed, 1)
                   + " dB through Crunch, and with no amp Velocity changes nothing (velocity already sets the level)");
    }

    // Its window: knobs in real units, the strings drawn with what each is tuned to, the guitars across the field.
    {
        // A new one, so no note has been played on it yet.
        Swarm fresh;
        fresh.prepareToPlay (rate, 512);
        std::unique_ptr<juce::AudioProcessorEditor> made (fresh.createEditorAndMakeActive());
        auto* window = dynamic_cast<SwarmGuitarEditor*> (made.get());
        const auto sized = window != nullptr && window->getWidth() == SwarmGuitarEditor::windowWidth && window->getHeight() == SwarmGuitarEditor::windowHeight;

        if (window != nullptr)
        {
            const auto text = [&] (const char* name) { auto* k = window->knob (name); return k != nullptr ? k->getValueText() : juce::String ("none"); };
            const auto readings = text ("Drive") == "70 %" && text ("Tone") == "4.50 kHz" && text ("Strings") == "6" && text ("Rate") == "Once"
                                       && text ("Sustain") == "6.00 s" && text ("Level") == "-12.0 dB" && text ("Guitars") == "2" && text ("Detune") == "+7 ct";
            const auto before = window->getStringsShown() == 6 && window->getGuitarsShown() == 2 && window->getStringLabel (0) == "oct down"
                                    && window->getStringLabel (3) == "note" && window->getStringLabel (5) == "oct up";

            // A D played: the strings are named, D1 for the low pair, D2 for the middle and D3 on top.
            juce::AudioBuffer<float> block (2, 512);
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 38, 1.0f), 0);
            fresh.processBlock (block, midi);
            midi.clear();

            for (int i = 0; i < 8; ++i)
                fresh.processBlock (block, midi);

            window->refreshNow();
            const auto named = window->getStringLabel (0) == "D1" && window->getStringLabel (2) == "D2" && window->getStringLabel (5) == "D3" && fresh.getStringLevel (0) > 0.0f;

            // Turning a knob sets its parameter; the pictures follow the parameters.
            window->knob ("Drive")->setNormalised (0.0f);
            set (fresh, "strings", 12.0f);
            set (fresh, "layers", 4.0f);
            set (fresh, "strumRate", 8.0f);
            window->refreshNow();
            const auto follows = fresh.parameterFor ("drive")->getValue() == 0.0f && window->getStringsShown() == 12 && window->getGuitarsShown() == 4
                                     && text ("Rate") == "8.0 /s" && text ("Drive") == "0 %";

            // The swarm chosen: the strings' knob counts oscillators, and the swarm's knobs read.
            set (fresh, "source", 1.0f);
            window->refreshNow();
            const auto swarmed = window->knob ("Oscillators") != nullptr && window->knob ("Strings") == nullptr && text ("Spread").contains ("30")
                                     && text ("Cutoff") == "5.00 kHz";
            set (fresh, "source", 0.0f);
            window->refreshNow();

            check (sized && readings && before && named && follows && swarmed,
                   "stringswarm window: 900 by 690, its knobs reading 70 %, 4.50 kHz, 6 strings, Once, 6.00 s, -12.0 dB, 2 guitars and +7 ct as it comes; the strings "
                   "labelled by their place (oct down, note, oct up) and, once a D is played, D1, D2 and D3; a knob turned sets its parameter and 12 strings and 4 guitars are drawn when set; "
                   "with the swarm chosen its strings knob counts Oscillators and its swarm knobs read (30 ct, 5.00 kHz)");

            // Its presets row with everything an instrument's window adds (Audition, Loop, Stop and the pattern menu)
            // still leaves the preset menu room for a name: it was squeezed to "..." (fix log 87).
            int menuWidth = 0;

            if (auto* row = InstrumentKeys::presetsRow (*window))
            {
                row->showAudition (true, [] (bool) {});
                row->showAuditionLoop (false, [] (bool) {}, [] {});
                row->showPatterns ([] { return juce::StringArray { "Low Riff", "Octave Run" }; }, [] { return 0; }, [] (int) {});
                window->resized();

                for (auto* c : row->getChildren())
                    if (auto* menu = dynamic_cast<juce::ComboBox*> (c))
                        menuWidth = menu->getWidth();
            }

            check (menuWidth >= 180, "stringswarm window: with Audition, Loop, Stop and the pattern menu showing, the preset menu is " + juce::String (menuWidth)
                                         + " px wide, room for a name");
        }
        else
        {
            check (false, "stringswarm window: it has one of its own");
        }
    }

    defaults (*guitar);
}

void saveSwarmGuitarWindow (const juce::File& folder)
{
    BuiltInEffects::SwarmGuitarProcessor guitar;
    guitar.prepareToPlay (rate, 512);
    juce::AudioBuffer<float> block (2, 512);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 38, 1.0f), 0);
    guitar.processBlock (block, midi);
    std::unique_ptr<juce::AudioProcessorEditor> made (guitar.createEditorAndMakeActive());

    if (auto* window = dynamic_cast<SwarmGuitarEditor*> (made.get()))
        window->showRinging (0.8f);

    for (auto theme : { "Dark", "Light" })
    {
        Theme::apply (Theme::make (theme, "Blue"));
        const auto image = made->createComponentSnapshot (made->getLocalBounds(), true, 2.0f);
        auto file = folder.getChildFile (juce::String ("swarm-") + theme + ".png");
        file.deleteFile();
        juce::FileOutputStream out (file);
        juce::PNGImageFormat().writeImageToStream (image, out);
        std::cout << file.getFullPathName() << std::endl;
    }
}
