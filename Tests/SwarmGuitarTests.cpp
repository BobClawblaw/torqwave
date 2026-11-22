#include <JuceHeader.h>
#include "../Source/AudioEngine.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/SwarmGuitar.h"
#include "TestUtils.h"
#include "SwarmGuitarTests.h"

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

    check (guitar != nullptr && error.isEmpty() && description.isInstrument && description.numInputChannels == 0 && description.name == "Swarm Guitar"
               && guitar->acceptsMidi() && guitar->getTotalNumInputChannels() == 0 && guitar->getTotalNumOutputChannels() == 2
               && guitar->getParameters().size() == 15,
           "swarm guitar: made from its description, an instrument with no audio input, a stereo output and 15 parameters");

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
               "swarm guitar: six strings tuned Unison are all on the note; Low Octave puts two an octave down; Three Octaves two down, three on the note and one up");
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

        check (worst < 3.0, "swarm guitar: one string with no amp is in tune from A1 to A5, within " + juce::String (worst, 1) + " cents (" + found.joinIntoString (", ") + ")");
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
               "swarm guitar: as it comes, a D and A held a second peak at " + juce::String (peak, 1) + " dB, its two guitars different left and right; one guitar is the same on both");
    }

    // The same twice and at any block size, strummed again and again as well.
    {
        defaults (*guitar);
        set (*guitar, "strumRate", 9.0f);
        const auto a = play (*guitar, chord, 1.5), b = play (*guitar, chord, 1.5), small = play (*guitar, chord, 1.5, 64), odd = play (*guitar, chord, 1.5, 37);
        check (same (a, b) && same (a, small) && same (a, odd), "swarm guitar: a part renders the same twice and in blocks of 512, 64 and 37, with Strum Rate on");
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
               "swarm guitar: with a 0.3 s sustain a held note is at " + juce::String (quiet, 4) + " RMS two seconds in, and at " + juce::String (loud, 3) + " strummed 8 times a second");
    }

    // Let go, a note is damped over the release; held, it rings on.
    {
        defaults (*guitar);
        const auto held = play (*guitar, { on (0.0, 45) }, 2.0), let = play (*guitar, { on (0.0, 45), off (0.5, 45) }, 2.0);
        const auto ringing = rms (held, 1.2, 0.5), damped = rms (let, 1.2, 0.5);
        check (ringing > 0.02f && damped < ringing * 0.01f && guitar->getNumSounding() == 0,
               "swarm guitar: a held note rings on (" + juce::String (ringing, 3) + " RMS after 1.2 s); let go at 0.5 s it is damped to " + juce::String (damped, 5) + " and its voice is free");
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
               "swarm guitar: All Sound Off in a fuzzed chord moves the output at most " + juce::String (stopping, 3) + " a sample (" + juce::String (playing, 3)
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
               "swarm guitar: a seventh note takes the oldest's place, seven sounding 5 ms on and six once it has faded, with no jump larger than the playing's");
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
        check (one <= 1.0f && six > 1.1f, "swarm guitar: six strings up to 25 cents apart beat against each other, their level climbing by up to " + juce::String (six, 2)
                                              + " times from one 50 ms to the next; one string's only falls (" + juce::String (one, 3) + " at most)");
    }

    defaults (*guitar);
}
