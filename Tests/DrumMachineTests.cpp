#include <JuceHeader.h>
#include "../Source/DrumMachine.h"
#include "../Source/Instrument.h"
#include "../Source/AudioEngine.h"
#include "TestUtils.h"
#include "DrumMachineTests.h"

namespace
{
    using TestUtils::check;
    using DrumMachine::Sound;
    using Notes = std::vector<std::tuple<int, double, float>>;
    constexpr double rate = 48000.0;

    /** Plays notes (pitch, second, velocity; each held 10 ms) for seconds in blocks of blockSize,
        with All Sound Off at allOffAt seconds if it is given. */
    juce::AudioBuffer<float> play (BuiltInEffects::BasicDrumsProcessor& drums, const Notes& notes, double seconds, int blockSize = 512, double allOffAt = -1.0)
    {
        drums.prepareToPlay (rate, blockSize);
        const auto total = (int) (seconds * rate);
        juce::AudioBuffer<float> out (2, total), block (2, blockSize);

        for (int at = 0; at < total; at += blockSize)
        {
            const auto n = juce::jmin (blockSize, total - at);
            juce::MidiBuffer midi;

            for (auto [pitch, when, velocity] : notes)
            {
                const auto on = (int) std::round (when * rate), off = on + (int) (0.01 * rate);

                if (on >= at && on < at + n)
                    midi.addEvent (juce::MidiMessage::noteOn (10, pitch, velocity), on - at);

                if (off >= at && off < at + n)
                    midi.addEvent (juce::MidiMessage::noteOff (10, pitch), off - at);
            }

            if (const auto off = (int) std::round (allOffAt * rate); allOffAt >= 0.0 && off >= at && off < at + n)
                midi.addEvent (juce::MidiMessage::allSoundOff (10), off - at);

            block.setSize (2, n, false, false, true);
            drums.processBlock (block, midi);

            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, at, block, ch, 0, n);
        }

        return out;
    }

    bool same (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        if (a.getNumSamples() != b.getNumSamples() || a.getNumChannels() != b.getNumChannels())
            return false;

        for (int ch = 0; ch < a.getNumChannels(); ++ch)
            for (int i = 0; i < a.getNumSamples(); ++i)
                if (! juce::exactlyEqual (a.getSample (ch, i), b.getSample (ch, i)))
                    return false;

        return true;
    }

    float peakOf (const juce::AudioBuffer<float>& b, int from = 0, int n = -1)
    {
        n = n < 0 ? b.getNumSamples() - from : n;
        return b.getNumSamples() == 0 ? 0.0f : b.getMagnitude (from, juce::jmin (n, b.getNumSamples() - from));
    }

    /** a minus b, sample by sample. */
    juce::AudioBuffer<float> minus (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        juce::AudioBuffer<float> d (a);

        for (int ch = 0; ch < d.getNumChannels(); ++ch)
            d.addFrom (ch, 0, b, ch, 0, d.getNumSamples(), -1.0f);

        return d;
    }

    /** The largest move from one sample to the next in [from, to), on either side. */
    float largestStep (const juce::AudioBuffer<float>& b, int from, int to)
    {
        float most = 0.0f;

        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            for (int i = juce::jmax (1, from); i < juce::jmin (to, b.getNumSamples()); ++i)
                most = juce::jmax (most, std::abs (b.getSample (ch, i) - b.getSample (ch, i - 1)));

        return most;
    }

    /** The sample a buffer last sounds at (-1 for none). */
    int lastSounding (const juce::AudioBuffer<float>& b)
    {
        for (int i = b.getNumSamples(); --i >= 0;)
            if (b.getSample (0, i) != 0.0f || b.getSample (1, i) != 0.0f)
                return i;

        return -1;
    }

    void set (juce::AudioParameterFloat* p, float v)                   { BuiltInEffects::setExactly (*p, v); }

    /** Whether a hit cut at `at` (with the rest of what plays subtracted, so the hit alone is
        left) fades out rather than stopping: from one sample to the next it moves no further than
        the sound itself moves just before, plus its 3 ms fade's share of its level; and it is
        silent once the fade is done. */
    bool fadesOut (const juce::AudioBuffer<float>& left, int at, juce::String& why)
    {
        const auto fade = (int) std::round (0.003 * rate);
        const auto before = largestStep (left, at - 96, at);
        const auto level = peakOf (left, at - 96, 96);
        const auto step = largestStep (left, at, at + fade + 4);
        const auto allowed = before + level / (float) fade * 1.1f + 1.0e-4f;
        const auto after = peakOf (left, at + fade + 1, (int) (0.05 * rate));
        why = "step " + juce::String (step, 4) + " (allowed " + juce::String (allowed, 4) + "), " + juce::String (after, 6) + " after";
        return step <= allowed && after < 1.0e-6f;
    }

    void testMade (AudioEngine& engine)
    {
        juce::PluginDescription description;

        for (auto& d : BuiltInEffects::getDescriptions())
            if (d.fileOrIdentifier == BuiltInEffects::basicDrumsId)
                description = d;

        juce::String error;
        auto instance = engine.pluginFormatManager.createPluginInstance (description, rate, 512, error);
        auto* drums = dynamic_cast<BuiltInEffects::BasicDrumsProcessor*> (instance.get());
        check (drums != nullptr && error.isEmpty() && description.isInstrument && description.numInputChannels == 0 && description.name == "Basic Drums"
                   && description.uniqueId == 0x53424452 && drums->acceptsMidi() && drums->getTotalNumInputChannels() == 0
                   && drums->getTotalNumOutputChannels() == 2 && drums->getParameters().size() == DrumMachine::numSounds * 6 + 4,
               "drums: Basic Drums is made from its description (the same ID, so sessions find it), an instrument with no audio input, a stereo output and "
                   + juce::String (drums != nullptr ? drums->getParameters().size() : 0) + " parameters (six for each of 30 sounds, the kick's character, the kit's drive, glue and level)");

        BuiltInEffects::addTo (engine.knownPlugins);
        const auto known = Instrument::known (engine.knownPlugins);
        check (known.size() >= 2 && known[0].fileOrIdentifier == BuiltInEffects::basicSynthId && known[1].fileOrIdentifier == BuiltInEffects::basicDrumsId
                   && Instrument::title (known[1]) == "Basic Drums (built in)",
               "drums: offered as an instrument, Basic Drums (built in), after the synth");
    }

    void testEverySound (BuiltInEffects::BasicDrumsProcessor& drums)
    {
        // Every General MIDI drum sounds in every kit, none clipping; notes outside them are silent.
        for (auto& kit : DrumMachine::kits())
        {
            drums.applyKit (kit);
            auto ok = true;
            float loudest = 0.0f, quietest = 10.0f;
            juce::StringArray bad;

            for (int note = DrumMachine::firstNote; note <= DrumMachine::lastNote; ++note)
            {
                const auto out = play (drums, { { note, 0.0, 1.0f } }, 0.6);
                const auto peak = peakOf (out);
                loudest = juce::jmax (loudest, peak);
                quietest = juce::jmin (quietest, peak);

                if (peak <= 0.03f || peak >= 0.98f)
                {
                    ok = false;
                    bad.add (juce::String (note) + " " + juce::String (peak, 3));
                }
            }

            check (ok, "drums: in the " + kit.name + " kit every note from 35 to 81 sounds, none clipping: peaks from " + juce::String (quietest, 2) + " to "
                           + juce::String (loudest, 2) + " at full velocity" + (bad.isEmpty() ? juce::String() : " (out of range: " + bad.joinIntoString (", ") + ")"));
        }

        drums.applyKit (DrumMachine::kits()[0]);
        auto silent = true;

        for (auto note : { 0, 34, 82, 127 })
            silent = silent && peakOf (play (drums, { { note, 0.0, 1.0f } }, 0.2)) == 0.0f;

        check (silent && ! DrumMachine::soundFor (82).has_value() && ! DrumMachine::soundFor (34).has_value(), "drums: notes outside 35 to 81 are silent");
    }

    void testNoClicks (BuiltInEffects::BasicDrumsProcessor& drums)
    {
        drums.applyKit (DrumMachine::kits()[0]);   // Standard: no drive or glue on the bus, so hits add as they are

        // Each note starts from silence, ends falling to nothing, and struck again 60 ms in, fades its
        // last hit rather than cutting it.
        juce::StringArray starts, ends, cuts;
        float worstStart = 0.0f, worstEnd = 0.0f;

        for (int note = DrumMachine::firstNote; note <= DrumMachine::lastNote; ++note)
        {
            const auto alone = play (drums, { { note, 0.0, 1.0f } }, 6.5);
            const auto peak = peakOf (alone);
            const auto first = juce::jmax (std::abs (alone.getSample (0, 0)), std::abs (alone.getSample (1, 0)));
            const auto second = juce::jmax (std::abs (alone.getSample (0, 1)), std::abs (alone.getSample (1, 1)));
            worstStart = juce::jmax (worstStart, second / peak);

            if (first > 0.02f * peak || second > 0.2f * peak)
                starts.add (juce::String (note));

            const auto last = lastSounding (alone);
            const auto tail = peakOf (alone, juce::jmax (0, last - 48), 49);
            worstEnd = juce::jmax (worstEnd, tail / peak);

            if (last < 0 || last >= alone.getNumSamples() - 1 || tail > 0.01f * peak)
                ends.add (juce::String (note));

            const auto at = (int) std::round (0.06 * rate);
            const auto both = play (drums, { { note, 0.0, 1.0f }, { note, 0.06, 1.0f } }, 0.2);
            const auto second60 = play (drums, { { note, 0.06, 1.0f } }, 0.2);
            juce::String why;

            if (! fadesOut (minus (both, second60), at, why))
                cuts.add (juce::String (note) + " (" + why + ")");
        }

        check (starts.isEmpty(), "drums: every note starts from silence, its second sample at most " + juce::String (worstStart * 100.0f, 1)
                                     + " % of its peak" + (starts.isEmpty() ? juce::String() : " (steps: " + starts.joinIntoString (", ") + ")"));
        check (ends.isEmpty(), "drums: every note ends falling to nothing, its last millisecond at most " + juce::String (worstEnd * 100.0f, 2)
                                   + " % of its peak" + (ends.isEmpty() ? juce::String() : " (cut off: " + ends.joinIntoString (", ") + ")"));
        check (cuts.isEmpty(), "drums: every note struck again 60 ms in fades its last hit over 3 ms, moving no further than the sound itself does"
                                   + (cuts.isEmpty() ? juce::String() : " (clicks: " + cuts.joinIntoString ("; ") + ")"));

        // The choke groups: a closed or pedal hat cuts the open one, the mute triangle and cuica the
        // open ones (and back); each choked hit fades rather than stops.
        juce::StringArray chokes;

        for (auto [ringing, choker] : std::vector<std::pair<int, int>> { { 46, 42 }, { 46, 44 }, { 42, 46 }, { 81, 80 }, { 80, 81 }, { 79, 78 }, { 78, 79 } })
        {
            const auto at = (int) std::round (0.05 * rate);
            const auto both = play (drums, { { ringing, 0.0, 1.0f }, { choker, 0.05, 1.0f } }, 0.4);
            const auto chokerAlone = play (drums, { { choker, 0.05, 1.0f } }, 0.4);
            const auto ringingAlone = play (drums, { { ringing, 0.0, 1.0f } }, 0.4);
            juce::String why;
            const auto rangOn = peakOf (ringingAlone, at + 400, 2000) > 1.0e-3f;

            if (! (fadesOut (minus (both, chokerAlone), at, why) && rangOn))
                chokes.add (juce::String (choker) + " on " + juce::String (ringing) + " (" + why + ")");
        }

        // A note in no group doesn't choke another: the open hat rings on under a kick.
        const auto hatAndKick = play (drums, { { 46, 0.0, 1.0f }, { 36, 0.05, 1.0f } }, 0.4);
        const auto kickAlone = play (drums, { { 36, 0.05, 1.0f } }, 0.4);
        const auto hatLeft = peakOf (minus (hatAndKick, kickAlone), (int) (0.2 * rate), (int) (0.1 * rate));

        check (chokes.isEmpty() && hatLeft > 1.0e-3f,
               "drums: the choke groups (closed and pedal hat on the open hat, mute and open triangle, mute and open cuica) fade the choked hit over 3 ms; a kick chokes nothing"
                   + (chokes.isEmpty() ? juce::String() : " (failed: " + chokes.joinIntoString ("; ") + ")"));

        // A tom fill rings over itself: 41 and 43 share the low tom's settings but not a voice.
        const auto fill = play (drums, { { 41, 0.0, 1.0f }, { 43, 0.05, 1.0f } }, 0.4);
        const auto secondTom = play (drums, { { 43, 0.05, 1.0f } }, 0.4);
        check (peakOf (minus (fill, secondTom), (int) (0.1 * rate), (int) (0.05 * rate)) > 0.01f,
               "drums: notes of one sound (low tom 41 and 43) each ring on their own voice");

        // All Sound Off (a stop, a loop, a seek) fades every hit over 3 ms.
        Notes kit;

        for (int note = DrumMachine::firstNote; note <= DrumMachine::lastNote; ++note)
            kit.push_back ({ note, 0.0, 0.5f });

        const auto at = (int) std::round (0.1 * rate);
        const auto stopped = play (drums, kit, 0.3, 512, 0.1);
        juce::String why;
        check (fadesOut (stopped, at, why) && drums.getNumSounding() == 0, "drums: All Sound Off fades a whole kit out over 3 ms, no click (" + why + ")");

        // Every sound ends: a whole kit is silent within six and a half seconds.
        const auto all = play (drums, kit, 7.0);
        check (drums.getNumSounding() == 0 && peakOf (all, (int) (6.5 * rate)) == 0.0f, "drums: every sound ends, a whole kit silent within 6.5 seconds");
    }

    void testSettings (BuiltInEffects::BasicDrumsProcessor& drums)
    {
        drums.applyKit (DrumMachine::kits()[0]);

        // Each of the six settings changes each sound.
        juce::StringArray unchanged;

        for (int i = 0; i < DrumMachine::numSounds; ++i)
        {
            const auto sound = (Sound) i;
            const auto note = DrumMachine::padNote (sound);
            const auto before = play (drums, { { note, 0.0, 1.0f } }, 1.0);

            for (auto what : { "tune", "decay", "tone", "level", "pan", "drive" })
            {
                auto* p = drums.parameterOf (sound, what);
                const auto was = p->get();
                const auto range = p->getNormalisableRange();
                set (p, juce::String (what) == "decay" ? was * 0.5f : juce::String (what) == "drive" ? 60.0f
                        : was + (range.end - was > was - range.start ? 1.0f : -1.0f) * 0.3f * (range.end - range.start));
                const auto after = play (drums, { { note, 0.0, 1.0f } }, 1.0);
                set (p, was);

                if (same (before, after))
                    unchanged.add (DrumMachine::nameOf (sound) + " " + what);
            }
        }

        check (unchanged.isEmpty(), "drums: tune, decay, tone, level, pan and drive each change every one of the 30 sounds"
                                        + (unchanged.isEmpty() ? juce::String() : " (no change: " + unchanged.joinIntoString (", ") + ")"));

        // Level in dB, exactly; decay halves the length; tune moves the kick's pitch.
        const auto kick = play (drums, { { 36, 0.0, 1.0f } }, 1.0);
        set (drums.parameterOf (Sound::kick, "level"), -6.0f);
        const auto quieter = play (drums, { { 36, 0.0, 1.0f } }, 1.0);
        set (drums.parameterOf (Sound::kick, "level"), 0.0f);
        check (std::abs (peakOf (quieter) / peakOf (kick) - juce::Decibels::decibelsToGain (-6.0f)) < 1.0e-3f, "drums: a sound's level is in dB, exactly");

        set (drums.parameterOf (Sound::snare, "decay"), 640.0f);
        const auto longer = play (drums, { { 38, 0.0, 1.0f } }, 1.0);
        set (drums.parameterOf (Sound::snare, "decay"), 320.0f);
        const auto shorter = play (drums, { { 38, 0.0, 1.0f } }, 1.0);
        check (lastSounding (longer) > (int) (0.6 * rate) && lastSounding (shorter) < (int) (0.36 * rate),
               "drums: decay is in ms: the snare at 320 ms ends by " + juce::String (lastSounding (shorter) / rate * 1000.0, 0) + " ms, at 640 ms it rings to "
                   + juce::String (lastSounding (longer) / rate * 1000.0, 0) + " ms");

        // Stereo: pan moves a sound between the sides; the crash and the clap's room are wide of themselves.
        set (drums.parameterOf (Sound::snare, "pan"), -100.0f);
        const auto leftSnare = play (drums, { { 38, 0.0, 1.0f } }, 0.5);
        set (drums.parameterOf (Sound::snare, "pan"), 0.0f);
        const auto centred = play (drums, { { 38, 0.0, 1.0f } }, 0.5);
        auto centredSame = true;

        for (int n = 0; n < centred.getNumSamples(); ++n)
            centredSame = centredSame && juce::exactlyEqual (centred.getSample (0, n), centred.getSample (1, n));

        const auto crash = play (drums, { { 49, 0.0, 1.0f } }, 0.5), clap = play (drums, { { 39, 0.0, 1.0f } }, 0.5);
        const auto differs = [] (const juce::AudioBuffer<float>& b)
        {
            double sum = 0.0, diff = 0.0;

            for (int n = 0; n < b.getNumSamples(); ++n)
            {
                sum += std::pow (b.getSample (0, n) + b.getSample (1, n), 2.0);
                diff += std::pow (b.getSample (0, n) - b.getSample (1, n), 2.0);
            }

            return diff / juce::jmax (1.0e-12, sum);
        };

        check (peakOf (leftSnare) > 0.1f && leftSnare.getMagnitude (1, 0, leftSnare.getNumSamples()) < 1.0e-6f && centredSame
                   && differs (crash) > 0.05 && differs (clap) > 0.005,
               "drums: stereo out: a snare panned hard left is in the left alone, centred it is the same both sides; the crash ("
                   + juce::String (differs (crash), 3) + ") and the clap's room (" + juce::String (differs (clap), 3) + ") differ side to side");

        // The kick's characters differ.
        const auto punch = play (drums, { { 36, 0.0, 1.0f } }, 0.5);
        drums.kickCharacterParameter()->setValueNotifyingHost (0.0f);
        const auto boom = play (drums, { { 36, 0.0, 1.0f } }, 0.5);
        drums.kickCharacterParameter()->setValueNotifyingHost (1.0f);
        const auto hard = play (drums, { { 36, 0.0, 1.0f } }, 0.5);
        drums.kickCharacterParameter()->setValueNotifyingHost (0.5f);
        check (! same (punch, boom) && ! same (punch, hard) && ! same (boom, hard), "drums: the kick's Boom, Punch and Hard characters each sound different");

        // Velocity scales a hit exactly where nothing on the bus bends it; the kit's drive and glue change the kit.
        const auto full = play (drums, { { 36, 0.0, 1.0f } }, 0.5);
        const auto half = play (drums, { { 36, 0.0, 64.0f / 127.0f } }, 0.5);
        auto scaled = true;

        for (int n = 0; n < full.getNumSamples(); ++n)
            scaled = scaled && std::abs (half.getSample (0, n) - 64.0f / 127.0f * full.getSample (0, n)) < 1.0e-6f;

        Notes groove { { 36, 0.0, 1.0f }, { 42, 0.0, 0.8f }, { 38, 0.25, 1.0f }, { 42, 0.25, 0.7f } };
        const auto dry = play (drums, groove, 0.6);
        set (drums.kitDriveParameter(), 60.0f);
        const auto driven = play (drums, groove, 0.6);
        set (drums.kitDriveParameter(), 0.0f);
        set (drums.glueParameter(), 80.0f);
        const auto glued = play (drums, groove, 0.6);
        drums.prepareToPlay (rate, 512);
        juce::AudioBuffer<float> hit (2, 512);
        juce::MidiBuffer kickNow;
        kickNow.addEvent (juce::MidiMessage::noteOn (10, 36, 1.0f), 0);
        drums.processBlock (hit, kickNow);
        const auto reduction = drums.getEngine().getReductionDb();
        set (drums.glueParameter(), 0.0f);
        set (drums.levelParameter(), -12.0f);
        const auto down = play (drums, groove, 0.6);
        set (drums.levelParameter(), -6.0f);
        check (scaled && ! same (dry, driven) && ! same (dry, glued) && reduction < -3.0f && std::abs (peakOf (down) / peakOf (dry) - juce::Decibels::decibelsToGain (-6.0f)) < 1.0e-3f,
               "drums: velocity scales a hit exactly; the kit's drive and glue change it (glue at 80 % last turning it down "
                   + juce::String (-reduction, 1) + " dB), its level is in dB");

        // CC 7 turns it down, squared.
        drums.prepareToPlay (rate, 512);
        juce::AudioBuffer<float> block (2, 4800);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::controllerEvent (10, 7, 64), 0);
        midi.addEvent (juce::MidiMessage::noteOn (10, 36, 1.0f), 0);
        drums.processBlock (block, midi);
        check (std::abs (block.getMagnitude (0, 0, 4800) / peakOf (full, 0, 4800) - std::pow (64.0f / 127.0f, 2.0f)) < 1.0e-3f, "drums: CC 7 sets its volume, squared");
    }

    void testKitsAndState (BuiltInEffects::BasicDrumsProcessor& drums)
    {
        const auto& all = DrumMachine::kits();
        juce::StringArray names;

        for (auto& k : all)
            names.add (k.name);

        // Switching kits changes the sound; a kit set is reported as unchanged until a knob moves.
        Notes groove { { 36, 0.0, 1.0f }, { 42, 0.0, 0.8f }, { 38, 0.25, 1.0f }, { 46, 0.25, 0.7f } };
        std::vector<juce::AudioBuffer<float>> renders;
        auto clean = true;

        for (auto& kit : all)
        {
            drums.applyKit (kit);
            clean = clean && ! drums.kitModified() && drums.getKitName() == kit.name;
            renders.push_back (play (drums, groove, 0.6));
        }

        auto distinct = true;

        for (size_t a = 0; a < renders.size(); ++a)
            for (size_t b = a + 1; b < renders.size(); ++b)
                distinct = distinct && ! same (renders[a], renders[b]);

        set (drums.parameterOf (Sound::kick, "tune"), 5.0f);
        const auto modified = drums.kitModified();

        check (all.size() == 7 && names[0] == "Standard" && names.contains ("Dark Techno") && names.contains ("Industrial") && names.contains ("EBM Machine")
                   && names.contains ("Electro") && names.contains ("Breaks") && names.contains ("Acoustic") && clean && distinct && modified,
               "drums: seven kits (" + names.joinIntoString (", ") + "), each sounding different; a kit chosen reads as itself until a knob moves");

        // The state keeps the kit's name and every setting.
        drums.applyKit (all[(size_t) DrumMachine::kitIndex ("Industrial")]);
        set (drums.parameterOf (Sound::clap, "pan"), -40.0f);
        juce::MemoryBlock state;
        drums.getStateInformation (state);
        BuiltInEffects::BasicDrumsProcessor another;
        another.setStateInformation (state.getData(), (int) state.getSize());
        check (another.getKitName() == "Industrial" && another.kitModified() && another.currentSettings() == drums.currentSettings()
                   && same (play (another, groove, 0.6), play (drums, groove, 0.6)),
               "drums: the state brings back the kit's name and every setting, and the same sound");

        // A session from before (a kick, snare, hat and tom tune and decay in percent) loads into
        // the nearest settings. First a state as 0.19.2 wrote it, then the factory presets' Machine kit.
        juce::XmlElement old ("TORQWAVE_BASIC_DRUMS");
        old.setAttribute ("version", 1);
        old.setAttribute ("level", -3.0);
        old.setAttribute ("kickTune", -5.0);
        old.setAttribute ("kickDecay", 200.0);
        old.setAttribute ("hatTune", 2.0);
        old.setAttribute ("hatDecay", 50.0);
        old.setAttribute ("tomDecay", 150.0);
        juce::MemoryBlock oldState;
        juce::AudioProcessor::copyXmlToBinary (old, oldState);
        BuiltInEffects::BasicDrumsProcessor loaded;
        loaded.setStateInformation (oldState.getData(), (int) oldState.getSize());
        const auto k = loaded.currentSettings();
        const auto near = [] (float a, float b) { return std::abs (a - b) < 1.0e-2f * juce::jmax (1.0f, std::abs (b)); };
        const auto standard = DrumMachine::defaultKit();

        juce::MemoryBlock machine;
        machine.fromBase64Encoding ("79.VMjLgXD....OT8jTQcUPVUzWBEzTIMzWDIUUMMEH1Ulbyk1at0iHwHBHy4VXxUFQkMVX40iHwXCLt.iHffVXzQTYiEVd8HxMv3BLh7hO..");
        BuiltInEffects::BasicDrumsProcessor machineKit;
        machineKit.setStateInformation (machine.getData(), (int) machine.getSize());
        const auto m = machineKit.currentSettings();

        check (near (k.level, -3.0f) && near (k.sounds[0].tune, -5.0f) && near (k.sounds[0].decay, standard.sounds[0].decay * 2.0f)
                   && near (k.sounds[(size_t) Sound::openHat].tune, 2.0f) && near (k.sounds[(size_t) Sound::closedHat].decay, standard.sounds[(size_t) Sound::closedHat].decay * 0.5f)
                   && near (k.sounds[(size_t) Sound::midTom].decay, standard.sounds[(size_t) Sound::midTom].decay * 1.5f) && near (k.sounds[(size_t) Sound::snare].tune, 0.0f)
                   && loaded.getKitName() == "Standard" && peakOf (play (loaded, groove, 0.6)) > 0.05f
                   && near (m.sounds[(size_t) Sound::snare].decay, standard.sounds[(size_t) Sound::snare].decay * 1.6f)
                   && near (m.sounds[(size_t) Sound::openHat].decay, standard.sounds[(size_t) Sound::openHat].decay * 0.7f),
               "drums: a 0.19.2 session loads: its level, the kick's tune, and decays in percent made from the new sounds' own (kick 200 %, hats 50 %, toms 150 %); "
               "the factory presets' Machine kit (snare 160 %, hats 70 %) likewise");
    }

    void testRendering (BuiltInEffects::BasicDrumsProcessor& drums)
    {
        // A pattern renders the same twice and at any block size, in a kit with drive and glue on the bus.
        Notes pattern;

        for (int step = 0; step < 16; ++step)
        {
            const auto at = step * 0.125;

            if (step % 4 == 0) pattern.push_back ({ 36, at, 1.0f });
            if (step % 8 == 4) pattern.push_back ({ 39, at, 0.9f });
            if (step == 10) pattern.push_back ({ 49, at, 0.8f });
            if (step >= 12) pattern.push_back ({ step % 2 == 0 ? 45 : 47, at, 0.8f });
            pattern.push_back ({ step % 4 == 2 ? 46 : 42, at, step % 2 == 0 ? 0.8f : 0.6f });
        }

        drums.applyKit (DrumMachine::kits()[(size_t) DrumMachine::kitIndex ("Industrial")]);
        const auto first = play (drums, pattern, 2.5), again = play (drums, pattern, 2.5), odd = play (drums, pattern, 2.5, 37), one = play (drums, pattern, 2.5, 1);
        check (same (first, again) && same (first, odd) && same (first, one) && peakOf (first) < 1.0f,
               "drums: a pattern renders bit-identically twice, in 37-sample blocks and sample by sample, the Industrial kit's drive and glue on (peak "
                   + juce::String (peakOf (first), 2) + ")");

        // A pad clicked in the window plays in the next block, and the engine counts the hit for its light.
        drums.applyKit (DrumMachine::kits()[0]);
        drums.prepareToPlay (rate, 512);
        const auto hits = drums.getEngine().hitsOf (Sound::snare);
        drums.audition (38, 0.9f);
        juce::AudioBuffer<float> block (2, 512);
        juce::MidiBuffer none;
        drums.processBlock (block, none);
        check (peakOf (block) > 0.05f && drums.getEngine().hitsOf (Sound::snare) == hits + 1 && std::abs (drums.getEngine().lastVelocityOf (Sound::snare) - 0.9f) < 1.0e-6f,
               "drums: a pad's audition plays at once, and the hit is counted for the pad's light");

        // The picture the window draws is the hit alone.
        const auto picture = DrumMachine::Engine::renderOne (drums.currentSettings(), Sound::kick, rate, 0.5);
        set (drums.levelParameter(), 0.0f);
        const auto played = play (drums, { { 36, 0.0, 1.0f } }, 0.5);
        set (drums.levelParameter(), -6.0f);
        check (same (picture, played), "drums: the window's picture of a sound is the sound exactly as it plays (at 0 dB)");

        // Mono: the two sides mixed.
        drums.prepareToPlay (rate, 512);
        juce::AudioBuffer<float> mono (1, 4800);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (10, 49, 1.0f), 0);
        drums.processBlock (mono, midi);
        check (peakOf (mono) > 0.05f, "drums: played into one channel, it mixes its two sides");
    }

    /** Peak and RMS of every note at full velocity, for setting the sounds' levels (--drums-levels). */
    void printLevels (BuiltInEffects::BasicDrumsProcessor& drums)
    {
        drums.applyKit (DrumMachine::kits()[0]);
        set (drums.levelParameter(), 0.0f);

        for (int note = DrumMachine::firstNote; note <= DrumMachine::lastNote; ++note)
        {
            const auto out = play (drums, { { note, 0.0, 1.0f } }, 0.5);
            double sum = 0.0;
            const auto n = (int) (0.1 * rate);

            for (int i = 0; i < n; ++i)
                sum += std::pow (0.5 * (out.getSample (0, i) + out.getSample (1, i)), 2.0);

            std::cout << note << " " << DrumMachine::nameOf (*DrumMachine::soundFor (note)) << ": peak " << juce::Decibels::gainToDecibels (peakOf (out))
                      << " dB, rms(100ms) " << juce::Decibels::gainToDecibels ((float) std::sqrt (sum / n)) << " dB" << std::endl;
        }
    }
}

void runDrumMachineTests (AudioEngine& engine, bool levels)
{
    BuiltInEffects::BasicDrumsProcessor drums;

    if (levels)
    {
        printLevels (drums);
        return;
    }

    testMade (engine);
    testEverySound (drums);
    testNoClicks (drums);
    testSettings (drums);
    testKitsAndState (drums);
    testRendering (drums);
}
