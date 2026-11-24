#include <JuceHeader.h>
#include "../Source/DrumMachine.h"
#include "../Source/DrumMachineEditor.h"
#include "../Source/Instrument.h"
#include "../Source/AudioEngine.h"
#include "../Source/Theme.h"
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
        silent once the fade is done. With the hit as it sounds when nothing cuts it, the sound's own
        moves are also those it would have made while it fades: noise does not move the same from one
        moment to the next. */
    bool fadesOut (const juce::AudioBuffer<float>& left, int at, juce::String& why, const juce::AudioBuffer<float>* uncut = nullptr)
    {
        const auto fade = (int) std::round (0.003 * rate);
        const auto before = juce::jmax (largestStep (left, at - 96, at), uncut != nullptr ? largestStep (*uncut, at, at + fade + 4) : 0.0f);
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
                   && drums->getTotalNumOutputChannels() == 2 && drums->getParameters().size() == DrumMachine::numSounds * 12 + 21 + 7,
               "drums: Basic Drums is made from its description (the same ID, so sessions find it), an instrument with no audio input, a stereo output and "
                   + juce::String (drums != nullptr ? drums->getParameters().size() : 0) + " parameters (twelve for each of 30 sounds, a type for the 21 that have two, the kick's character, the kit's drive, glue, level, reverb size, delay time and feedback)");

        BuiltInEffects::addTo (engine.knownPlugins);
        const auto known = Instrument::known (engine.knownPlugins);
        check (known.size() >= 2 && known[0].fileOrIdentifier == BuiltInEffects::basicSynthId && known[1].fileOrIdentifier == BuiltInEffects::basicDrumsId
                   && Instrument::title (known[1]) == "Basic Drums (built in)",
               "drums: offered as an instrument, Basic Drums (built in), after the synth");
    }

    void testWindow()
    {
        BuiltInEffects::BasicDrumsProcessor drums;
        drums.prepareToPlay (rate, 512);
        std::unique_ptr<juce::AudioProcessorEditor> made (drums.createEditorAndMakeActive());
        auto* window = dynamic_cast<DrumMachineEditor*> (made.get());
        check (window != nullptr && window->getWidth() >= 900 && window->getHeight() >= 560 && window->getSelected() == Sound::kick,
               "drums window: opens on the kick (" + juce::String (made != nullptr ? made->getWidth() : 0) + " by " + juce::String (made != nullptr ? made->getHeight() : 0) + ")");

        if (window == nullptr)
            return;

        // A pad for every sound, all inside the window and none overlapping.
        auto inside = true, apart = true;

        for (auto s : DrumMachineEditor::padOrder())
        {
            const auto b = window->padBounds (s);
            inside = inside && ! b.isEmpty() && window->getLocalBounds().contains (b);

            for (auto t : DrumMachineEditor::padOrder())
                apart = apart && (s == t || ! b.reduced (1).intersects (window->padBounds (t).reduced (1)));
        }

        std::set<Sound> each (DrumMachineEditor::padOrder().begin(), DrumMachineEditor::padOrder().end());
        check (inside && apart && each.size() == (size_t) DrumMachine::numSounds, "drums window: a pad for each of the 30 sounds, inside the window, none overlapping");

        // Clicking a pad plays it (louder nearer its top) and selects it.
        const auto hits = drums.getEngine().hitsOf (Sound::snare);
        window->clickPad (Sound::snare, 0.02f);
        juce::AudioBuffer<float> block (2, 512);
        juce::MidiBuffer midi;
        drums.processBlock (block, midi);
        check (window->getSelected() == Sound::snare && drums.getEngine().hitsOf (Sound::snare) == hits + 1 && drums.getEngine().lastVelocityOf (Sound::snare) > 0.95f
                   && block.getMagnitude (0, 512) > 0.05f && DrumMachineEditor::velocityAt (0.0f) > DrumMachineEditor::velocityAt (0.5f)
                   && DrumMachineEditor::velocityAt (0.5f) > DrumMachineEditor::velocityAt (1.0f) && DrumMachineEditor::velocityAt (1.0f) >= 0.3f,
               "drums window: clicking the snare's pad near its top plays it at full velocity and selects it; lower on a pad plays softer");

        // The selected sound's knobs read in real units and follow it.
        BuiltInEffects::setExactly (*drums.parameterOf (Sound::snare, "decay"), 640.0f);
        BuiltInEffects::setExactly (*drums.parameterOf (Sound::snare, "pan"), -30.0f);
        BuiltInEffects::setExactly (*drums.parameterOf (Sound::snare, "tune"), 3.0f);
        BuiltInEffects::setExactly (*drums.parameterOf (Sound::snare, "level"), -4.5f);
        check (window->getKnobText ("Decay") == "640 ms" && window->getKnobText ("Pan") == "L 30" && window->getKnobText ("Tune") == "+3 st"
                   && window->getKnobText ("Level") == "-4.5 dB" && window->getKnobText ("Tone") == "0 %",
               "drums window: the snare's knobs read 640 ms, L 30, +3 st, -4.5 dB and 0 % (read: " + window->getKnobText ("Decay") + ", " + window->getKnobText ("Pan") + ", "
                   + window->getKnobText ("Tune") + ", " + window->getKnobText ("Level") + ", " + window->getKnobText ("Tone") + ")");

        // Its picture is the hit, as long as the sound lasts: longer when the decay is.
        window->select (Sound::snare);
        const auto longPicture = window->getPictureSeconds();
        BuiltInEffects::setExactly (*drums.parameterOf (Sound::snare, "decay"), 160.0f);
        window->select (Sound::snare);
        const auto shortPicture = window->getPictureSeconds();
        check (longPicture > 0.6 && longPicture < 0.8 && shortPicture > 0.15 && shortPicture < 0.4,
               "drums window: the picture shows the whole hit, " + juce::String (longPicture * 1000.0, 0) + " ms at a 640 ms decay and "
                   + juce::String (shortPicture * 1000.0, 0) + " ms at 160 ms");

        // A sound with two types has a selector for them, and its picture follows it; a sound with one has none.
        const auto classicShown = window->getTypeText();
        const auto classicSeconds = window->getPictureSeconds();
        drums.typeParameter (Sound::snare)->setValueNotifyingHost (1.0f);
        window->select (Sound::snare);
        const auto modernShown = window->getTypeText();
        window->select (Sound::rim);
        const auto rimShown = window->getTypeText();
        drums.typeParameter (Sound::snare)->setValueNotifyingHost (0.0f);
        window->select (Sound::kick);
        check (classicShown == "Classic" && modernShown == "Modern" && rimShown.isEmpty() && window->getTypeText() == "Classic" && classicSeconds > 0.0
                   && drums.typeParameter (Sound::rim) == nullptr,
               "drums window: the snare's type reads Classic, then Modern once chosen; the rim, which has one type, shows no selector; the kick has its type beside its character");

        // The kits: choosing one sets every sound; a knob moved marks it edited.
        drums.applyKit (DrumMachine::kits()[(size_t) DrumMachine::kitIndex ("Dark Techno")]);
        const auto chosen = window->getKitText();
        BuiltInEffects::setExactly (*drums.parameterOf (Sound::kick, "drive"), 90.0f);
        check (chosen == "Dark Techno" && window->getKitText() == "Dark Techno (edited)", "drums window: shows the kit's name, and \"(edited)\" once a knob moves");

        // It paints in a light and a dark theme.
        for (auto theme : { "Light", "Dark" })
        {
            Theme::apply (Theme::make (theme, "Blue"));
            const auto image = window->createComponentSnapshot (window->getLocalBounds());
            check (image.isValid() && image.getWidth() == window->getWidth(), juce::String ("drums window: paints in the ") + theme + " theme");
        }
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

    /** In a kit with no drive or glue on the bus, so hits add as they are: Standard for the Classic
        sounds, Modern for the Modern ones. */
    void testNoClicks (BuiltInEffects::BasicDrumsProcessor& drums, const juce::String& kitName)
    {
        drums.applyKit (DrumMachine::kits()[(size_t) DrumMachine::kitIndex (kitName)]);
        const auto tag = kitName == "Standard" ? juce::String ("drums: ") : "drums (the " + kitName + " kit's sounds): ";

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

            if (! fadesOut (minus (both, second60), at, why, &alone))
                cuts.add (juce::String (note) + " (" + why + ")");
        }

        check (starts.isEmpty(), tag + "every note starts from silence, its second sample at most " + juce::String (worstStart * 100.0f, 1)
                                     + " % of its peak" + (starts.isEmpty() ? juce::String() : " (steps: " + starts.joinIntoString (", ") + ")"));
        check (ends.isEmpty(), tag + "every note ends falling to nothing, its last millisecond at most " + juce::String (worstEnd * 100.0f, 2)
                                   + " % of its peak" + (ends.isEmpty() ? juce::String() : " (cut off: " + ends.joinIntoString (", ") + ")"));
        check (cuts.isEmpty(), tag + "every note struck again 60 ms in fades its last hit over 3 ms, moving no further than the sound itself does"
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

            if (! (fadesOut (minus (both, chokerAlone), at, why, &ringingAlone) && rangOn))
                chokes.add (juce::String (choker) + " on " + juce::String (ringing) + " (" + why + ")");
        }

        // A note in no group doesn't choke another: the open hat rings on under a kick.
        const auto hatAndKick = play (drums, { { 46, 0.0, 1.0f }, { 36, 0.05, 1.0f } }, 0.4);
        const auto kickAlone = play (drums, { { 36, 0.05, 1.0f } }, 0.4);
        const auto hatLeft = peakOf (minus (hatAndKick, kickAlone), (int) (0.2 * rate), (int) (0.1 * rate));

        check (chokes.isEmpty() && hatLeft > 1.0e-3f,
               tag + "the choke groups (closed and pedal hat on the open hat, mute and open triangle, mute and open cuica) fade the choked hit over 3 ms; a kick chokes nothing"
                   + (chokes.isEmpty() ? juce::String() : " (failed: " + chokes.joinIntoString ("; ") + ")"));

        // A tom fill rings over itself: 41 and 43 share the low tom's settings but not a voice.
        const auto fill = play (drums, { { 41, 0.0, 1.0f }, { 43, 0.05, 1.0f } }, 0.4);
        const auto secondTom = play (drums, { { 43, 0.05, 1.0f } }, 0.4);
        check (peakOf (minus (fill, secondTom), (int) (0.1 * rate), (int) (0.05 * rate)) > 0.01f,
               tag + "notes of one sound (low tom 41 and 43) each ring on their own voice");

        // All Sound Off (a stop, a loop, a seek) fades every hit over 3 ms.
        Notes kit;

        for (int note = DrumMachine::firstNote; note <= DrumMachine::lastNote; ++note)
            kit.push_back ({ note, 0.0, 0.5f });

        const auto at = (int) std::round (0.1 * rate);
        const auto stopped = play (drums, kit, 0.3, 512, 0.1);
        juce::String why;
        check (fadesOut (stopped, at, why) && drums.getNumSounding() == 0, tag + "All Sound Off fades a whole kit out over 3 ms, no click (" + why + ")");

        // Every sound ends: a whole kit is silent within six and a half seconds.
        const auto all = play (drums, kit, 7.0);
        check (drums.getNumSounding() == 0 && peakOf (all, (int) (6.5 * rate)) == 0.0f, tag + "every sound ends, a whole kit silent within 6.5 seconds");
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
        const auto kitLevel = drums.levelParameter()->get();
        set (drums.levelParameter(), kitLevel - 6.0f);
        const auto down = play (drums, groove, 0.6);
        set (drums.levelParameter(), kitLevel);
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

    /** How bright a hit is: the energy of its sample-to-sample moves over its energy. */
    double brightnessOf (const juce::AudioBuffer<float>& b)
    {
        double moves = 0.0, energy = 0.0;

        for (int n = 1; n < b.getNumSamples(); ++n)
        {
            moves += std::pow (b.getSample (0, n) - b.getSample (0, n - 1), 2.0);
            energy += std::pow (b.getSample (0, n), 2.0);
        }

        return moves / juce::jmax (1.0e-12, energy);
    }

    void testTypes (BuiltInEffects::BasicDrumsProcessor& drums)
    {
        drums.applyKit (DrumMachine::kits()[0]);
        const auto setType = [&drums] (Sound s, int index) { drums.typeParameter (s)->setValueNotifyingHost ((float) index); };

        // Twenty-one sounds have a Modern type: another sound at the level of the Classic one, which is left as it was.
        juce::StringArray sameSound, offLevel, noTone, rigid;
        int withTypes = 0;
        float mostApart = 0.0f;

        for (int i = 0; i < DrumMachine::numSounds; ++i)
        {
            const auto sound = (Sound) i;

            if (! DrumMachine::hasTypes (sound))
            {
                if (drums.typeParameter (sound) != nullptr)
                    sameSound.add (DrumMachine::nameOf (sound) + " has a type");

                continue;
            }

            ++withTypes;
            const auto note = DrumMachine::padNote (sound);
            const auto classic = play (drums, { { note, 0.0, 1.0f } }, 1.0);
            setType (sound, 1);
            const auto modern = play (drums, { { note, 0.0, 1.0f } }, 1.0);
            const auto soft = play (drums, { { note, 0.0, 0.3f } }, 1.0);

            // Each of its six settings changes the Modern sound too.
            for (auto what : { "tune", "decay", "tone", "level", "pan", "drive" })
            {
                auto* p = drums.parameterOf (sound, what);
                const auto was = p->get();
                const auto range = p->getNormalisableRange();
                set (p, juce::String (what) == "decay" ? was * 0.5f : juce::String (what) == "drive" ? 60.0f
                        : was + (range.end - was > was - range.start ? 1.0f : -1.0f) * 0.3f * (range.end - range.start));

                if (same (modern, play (drums, { { note, 0.0, 1.0f } }, 1.0)))
                    noTone.add (DrumMachine::nameOf (sound) + " " + what);

                set (p, was);
            }

            setType (sound, 0);
            const auto apart = std::abs (juce::Decibels::gainToDecibels (peakOf (modern)) - juce::Decibels::gainToDecibels (peakOf (classic)));
            mostApart = juce::jmax (mostApart, apart);

            if (same (classic, modern) || ! same (classic, play (drums, { { note, 0.0, 1.0f } }, 1.0)))
                sameSound.add (DrumMachine::nameOf (sound));

            if (apart > 1.0f)
                offLevel.add (DrumMachine::nameOf (sound) + " " + juce::String (apart, 1) + " dB");

            // Struck softer a Modern sound is quieter and duller, not the same hit turned down.
            if (! (peakOf (soft) < 0.45f * peakOf (modern) && brightnessOf (soft) < 0.97 * brightnessOf (modern)))
                rigid.add (DrumMachine::nameOf (sound) + " " + juce::String (brightnessOf (soft) / brightnessOf (modern), 2));
        }

        check (withTypes == 21 && DrumMachine::typeNames() == juce::StringArray ({ "Classic", "Modern" }) && sameSound.isEmpty(),
               "drums: 21 of the 30 sounds have a second type, Modern, another sound than Classic, which plays as before once chosen again"
                   + (sameSound.isEmpty() ? juce::String() : " (failed: " + sameSound.joinIntoString (", ") + ")"));
        check (offLevel.isEmpty(), "drums: each Modern sound peaks within 1 dB of its Classic one at full velocity (the furthest " + juce::String (mostApart, 2) + " dB apart)"
                                       + (offLevel.isEmpty() ? juce::String() : " (off: " + offLevel.joinIntoString (", ") + ")"));
        check (noTone.isEmpty(), "drums: tune, decay, tone, level, pan and drive each change every Modern sound"
                                     + (noTone.isEmpty() ? juce::String() : " (no change: " + noTone.joinIntoString (", ") + ")"));
        check (rigid.isEmpty(), "drums: a Modern sound struck at a third of full velocity is quieter and duller than at full"
                                    + (rigid.isEmpty() ? juce::String() : " (not duller: " + rigid.joinIntoString (", ") + ")"));
    }

    void testEffects (BuiltInEffects::BasicDrumsProcessor& drums)
    {
        drums.applyKit (DrumMachine::kits()[0]);

        // Each effect changes each sound: the filter (and, with it on, its cutoff and resonance),
        // the crush, and the two sends.
        const auto choose = [] (juce::AudioParameterChoice* p, int index) { p->setValueNotifyingHost (p->convertTo0to1 ((float) index)); };
        juce::StringArray unchanged;

        for (int i = 0; i < DrumMachine::numSounds; ++i)
        {
            const auto sound = (Sound) i;
            const auto note = DrumMachine::padNote (sound);
            const auto differs = [&] (const juce::String& what, auto&& change, auto&& undo)
            {
                const auto before = play (drums, { { note, 0.0, 1.0f } }, 0.6);
                change();
                const auto after = play (drums, { { note, 0.0, 1.0f } }, 0.6);
                undo();

                if (same (before, after))
                    unchanged.add (DrumMachine::nameOf (sound) + " " + what);
            };

            auto* filter = drums.filterParameter (sound);
            auto* cutoff = drums.parameterOf (sound, "cutoff");
            auto* resonance = drums.parameterOf (sound, "resonance");

            for (int type = 1; type <= 3; ++type)
                differs (DrumMachine::filterNames()[type], [&] { choose (filter, type); }, [&] { choose (filter, 0); });

            choose (filter, 2);
            differs ("cutoff", [&] { set (cutoff, 600.0f); }, [&] { set (cutoff, 2000.0f); });
            differs ("resonance", [&] { set (resonance, 80.0f); }, [&] { set (resonance, 20.0f); });
            choose (filter, 0);

            for (auto what : { "crush", "reverb", "delay" })
            {
                auto* p = drums.parameterOf (sound, what);
                differs (what, [&] { set (p, 60.0f); }, [&] { set (p, 0.0f); });
            }
        }

        check (unchanged.isEmpty(), "drums: the filter's three types, its cutoff and resonance, the crush and the reverb and delay sends each change every one of the 30 sounds"
                                        + (unchanged.isEmpty() ? juce::String() : " (no change: " + unchanged.joinIntoString (", ") + ")"));

        // The reverb rings on after the snare has ended, more in a bigger room.
        const auto snare = play (drums, { { 38, 0.0, 1.0f } }, 2.0);
        set (drums.parameterOf (Sound::snare, "reverb"), 50.0f);
        const auto room = play (drums, { { 38, 0.0, 1.0f } }, 2.0);
        set (drums.reverbSizeParameter(), 100.0f);
        const auto hall = play (drums, { { 38, 0.0, 1.0f } }, 2.0);
        set (drums.reverbSizeParameter(), DrumMachine::defaultKit().reverbSize);
        set (drums.parameterOf (Sound::snare, "reverb"), 0.0f);
        const auto after = (int) (0.6 * rate), lateFrom = (int) (1.2 * rate);
        check (peakOf (snare, after) == 0.0f && peakOf (room, after) > 1.0e-3f && peakOf (hall, lateFrom) > peakOf (room, lateFrom) && peakOf (room) < 1.0f,
               "drums: a snare's reverb send rings on after it ends (" + juce::String (juce::Decibels::gainToDecibels (peakOf (room, after) / peakOf (snare)), 1)
                   + " dB at 0.6 s against its hit), longer in a bigger room");

        // The delay echoes on the beat: at 120 bpm an eighth goes left at 0.25 s, then right at 0.5 s.
        set (drums.parameterOf (Sound::rim, "delay"), 100.0f);
        set (drums.feedbackParameter(), 50.0f);
        choose (drums.delayTimeParameter(), 2);
        const auto echoes = play (drums, { { 37, 0.0, 1.0f } }, 1.0);
        choose (drums.delayTimeParameter(), 4);
        const auto quarter = play (drums, { { 37, 0.0, 1.0f } }, 1.0);
        choose (drums.delayTimeParameter(), DrumMachine::defaultKit().delayTime);
        set (drums.feedbackParameter(), DrumMachine::defaultKit().feedback);
        set (drums.parameterOf (Sound::rim, "delay"), 0.0f);
        const auto within = [] (const juce::AudioBuffer<float>& b, int channel, double from, double to)
        {
            return b.getMagnitude (channel, (int) (from * rate), (int) ((to - from) * rate));
        };
        check (within (echoes, 0, 0.15, 0.24) < 1.0e-6f && within (echoes, 0, 0.25, 0.3) > 0.02f && within (echoes, 1, 0.25, 0.3) < 1.0e-6f
                   && within (echoes, 1, 0.5, 0.55) > 0.01f && within (echoes, 0, 0.5, 0.55) < 1.0e-6f
                   && within (quarter, 0, 0.25, 0.3) < 1.0e-6f && within (quarter, 0, 0.5, 0.55) > 0.02f,
               "drums: the delay ping-pongs on the beat: an eighth at 120 bpm echoes left at 0.25 s and right at 0.5 s; a quarter at 0.5 s");

        // All Sound Off fades the reverb and delay too, and they start again empty.
        set (drums.parameterOf (Sound::snare, "reverb"), 80.0f);
        set (drums.parameterOf (Sound::snare, "delay"), 80.0f);
        const auto at = (int) std::round (0.4 * rate);
        const auto stopped = play (drums, { { 38, 0.0, 1.0f } }, 1.0, 512, 0.4);
        set (drums.parameterOf (Sound::snare, "reverb"), 0.0f);
        set (drums.parameterOf (Sound::snare, "delay"), 0.0f);
        juce::String why;
        juce::AudioBuffer<float> left (1, stopped.getNumSamples());
        left.copyFrom (0, 0, stopped, 0, 0, stopped.getNumSamples());
        check (peakOf (stopped, at - 200, 200) > 1.0e-3f && fadesOut (left, at, why) && peakOf (stopped, at + (int) (0.004 * rate)) == 0.0f,
               "drums: All Sound Off fades the reverb and delay over 3 ms with the hits (" + why + "), then they are silent");

        // A crushed sound steps: from 16 bits down, held samples.
        set (drums.parameterOf (Sound::snare, "crush"), 100.0f);
        const auto crushed = play (drums, { { 38, 0.0, 1.0f } }, 0.2);
        set (drums.parameterOf (Sound::snare, "crush"), 0.0f);
        auto held = 0;
        const auto from = (int) (0.02 * rate), to = (int) (0.03 * rate);

        // At full crush a sample is held for 20, so the level ramp aside, runs of near-equal values.
        for (int n = from + 1; n < to; ++n)
            held += std::abs (crushed.getSample (0, n) - crushed.getSample (0, n - 1)) < 1.0e-4f ? 1 : 0;

        check (held > (to - from) * 3 / 4, "drums: crush at 100 % holds each sample for 20 (" + juce::String (held) + " of " + juce::String (to - from) + " steps flat)");
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

        // The seven kits of Classic sounds come first and use no Modern sound; the five after them are made of Modern ones.
        auto classicFirst = true;

        for (size_t i = 0; i < all.size(); ++i)
        {
            int modernSounds = 0;

            for (auto& sound : all[i].settings.sounds)
                modernSounds += sound.type;

            classicFirst = classicFirst && (i < 7 ? modernSounds == 0 : modernSounds == 21);
        }

        check (all.size() == 12 && names[0] == "Standard" && names[1] == "Dark Techno" && names[2] == "Industrial" && names[3] == "EBM Machine"
                   && names[4] == "Electro" && names[5] == "Breaks" && names[6] == "Industrial Rock" && names[7] == "Modern" && names.contains ("Warehouse")
                   && names.contains ("Body Music") && names.contains ("Foundry") && names.contains ("Broken Beat") && classicFirst && clean && distinct && modified,
               "drums: twelve kits (" + names.joinIntoString (", ") + "), the first seven all Classic sounds, the five after them Modern, each sounding different; "
               "a kit chosen reads as itself until a knob moves");

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

        // The types are kept too; a state from before there were types (none in it) plays every sound Classic.
        drums.applyKit (all[(size_t) DrumMachine::kitIndex ("Warehouse")]);
        juce::MemoryBlock modernState;
        drums.getStateInformation (modernState);
        BuiltInEffects::BasicDrumsProcessor warehouse;
        warehouse.setStateInformation (modernState.getData(), (int) modernState.getSize());

        drums.applyKit (all[(size_t) DrumMachine::kitIndex ("Dark Techno")]);
        juce::MemoryBlock full;
        drums.getStateInformation (full);
        auto before = juce::AudioProcessor::getXmlFromBinary (full.getData(), (int) full.getSize());
        int removed = 0;

        for (int i = 0; before != nullptr && i < DrumMachine::numSounds; ++i)
            if (const auto id = DrumMachine::idOf ((Sound) i) + "_type"; before->hasAttribute (id))
            {
                before->removeAttribute (id);
                ++removed;
            }

        juce::MemoryBlock beforeTypes;

        if (before != nullptr)
            juce::AudioProcessor::copyXmlToBinary (*before, beforeTypes);

        BuiltInEffects::BasicDrumsProcessor untyped;
        untyped.setStateInformation (beforeTypes.getData(), (int) beforeTypes.getSize());
        check (warehouse.getKitName() == "Warehouse" && ! warehouse.kitModified() && warehouse.currentSettings().sounds[0].type == 1
                   && removed == 21 && untyped.getKitName() == "Dark Techno" && ! untyped.kitModified() && untyped.currentSettings() == drums.currentSettings()
                   && same (play (untyped, groove, 0.6), play (drums, groove, 0.6)),
               "drums: a kit of Modern sounds comes back from the state as itself; a state saved before there were types loads as its kit, unedited, every sound Classic and sounding the same");

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
               "drums: a pattern renders bit-identically twice, in 37-sample blocks and sample by sample, the Industrial kit's crush, reverb, delay, drive and glue on (peak "
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
        const auto kitLevel = drums.levelParameter()->get();
        set (drums.levelParameter(), 0.0f);
        const auto played = play (drums, { { 36, 0.0, 1.0f } }, 0.5);
        set (drums.levelParameter(), kitLevel);
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
                      << " dB, rms(100ms) " << juce::Decibels::gainToDecibels ((float) std::sqrt (sum / n)) << " dB";

            // Its Modern type against it, for modernTrimDb.
            if (auto* type = drums.typeParameter (*DrumMachine::soundFor (note)))
            {
                type->setValueNotifyingHost (1.0f);
                const auto modern = play (drums, { { note, 0.0, 1.0f } }, 0.5);
                type->setValueNotifyingHost (0.0f);
                std::cout << "; Modern peak " << juce::Decibels::gainToDecibels (peakOf (modern)) << " dB ("
                          << juce::Decibels::gainToDecibels (peakOf (modern)) - juce::Decibels::gainToDecibels (peakOf (out)) << " dB against it)";
            }

            std::cout << std::endl;
        }
    }
}

void saveDrumMachineWindow (const juce::File& folder)
{
    BuiltInEffects::BasicDrumsProcessor drums;
    drums.prepareToPlay (rate, 512);
    drums.applyKit (DrumMachine::kits()[(size_t) DrumMachine::kitIndex ("Dark Techno")]);
    std::unique_ptr<juce::AudioProcessorEditor> window (drums.createEditorAndMakeActive());

    for (auto theme : { "Dark", "Light" })
    {
        Theme::apply (Theme::make (theme, "Blue"));
        const auto image = window->createComponentSnapshot (window->getLocalBounds(), true, 2.0f);
        auto file = folder.getChildFile (juce::String ("drums-") + theme + ".png");
        file.deleteFile();
        juce::FileOutputStream out (file);
        juce::PNGImageFormat().writeImageToStream (image, out);
        std::cout << file.getFullPathName() << std::endl;
    }
}

/** A step pattern: each row a note and 16 steps ('x' loud, 'o' soft, '.' rest). */
struct DemoRow
{
    int note;
    const char* steps;
};

/** Renders four bars of a groove fitting each kit (a fill in the last bar, a crash on the first)
    to <folder>/Basic Drums - <kit>.wav, 48 kHz, 24-bit, with the tail. */
void saveDrumMachineDemos (const juce::File& folder)
{
    struct Groove
    {
        const char* kit;
        double bpm;
        std::vector<DemoRow> rows, fill;
    };

    const std::vector<DemoRow> fourFloor { { 36, "x...x...x...x..." }, { 39, "....x.......x..." }, { 42, "x.oxx.oxx.oxx.ox" }, { 46, "..x...x...x...x." } };
    const std::vector<Groove> grooves {
        { "Standard", 126.0, fourFloor, { { 36, "x...x...x...x..." }, { 38, "....x...x.x.xxxx" }, { 42, "x.oxx.oxx.ox...." } } },
        { "Dark Techno", 132.0, { { 36, "x...x...x...x..." }, { 36, "..o...o...o..o.o" }, { 39, "....x.......x..." }, { 42, "xoxoxoxoxoxoxoxo" },
                                  { 46, "..x...x...x...x." }, { 51, "x.x.x.x.x.x.x.x." }, { 37, ".......o.....o.." } },
                                { { 36, "x...x...x...x..." }, { 39, "....x...x.x.x.xx" }, { 42, "xoxoxoxoxoxo...." }, { 45, "............x.x." } } },
        { "Industrial", 128.0, { { 36, "x...x...x...x..." }, { 43, "..x...x.....x.x." }, { 38, "....x.......x..." }, { 56, "x..x..x..x..x.x." },
                                 { 42, "x.x.x.x.x.x.x.x." }, { 52, "........x......." }, { 37, "...o.......o...." } },
                               { { 36, "x...x...x.x.xxxx" }, { 38, "....x.x.x.x.x.xx" }, { 41, "x.x.x.x........." } } },
        { "EBM Machine", 120.0, { { 36, "x...x...x...x..." }, { 38, "....x.......x..." }, { 39, "....x.......x..." }, { 42, "x.x.x.x.x.x.x.x." }, { 46, "..x...x...x...x." } },
                                { { 36, "x...x...x...x..." }, { 38, "....x...xxxxxxxx" }, { 42, "x.x.x.x........." } } },
        { "Electro", 116.0, { { 36, "x.....x...x....." }, { 38, "....x.......x..." }, { 42, "x.xxx.x.x.xxx.x." }, { 56, "...x.......x...." }, { 63, "......o......o.." },
                              { 39, "....x.......x..." }, { 37, ".......o........" } },
                            { { 36, "x.....x...x....." }, { 38, "....x.......x..." }, { 50, "........x.x....." }, { 47, "..........x.x..." }, { 43, "............x.x." } } },
        { "Breaks", 168.0, { { 36, "x.x.......x....." }, { 38, "....x..o.o..x..o" }, { 42, "x.x.x.x.x.x.x.x." }, { 51, "x...x...x...x..." } },
                           { { 36, "x.x.......x....." }, { 38, "....x..o.oxxxxxx" }, { 42, "x.x.x.x.x......." } } },
        { "Industrial Rock", 110.0, { { 36, "x.....x.x......." }, { 38, "....x.......x..." }, { 42, "x.x.x.x.x.x.x.x." } },
                                    { { 36, "x.......x......." }, { 38, "....x..." }, { 48, "........xx......" }, { 45, "..........xx...." }, { 41, "............xxxx" } } },
    };

    // Each kit of Modern sounds plays the groove of the kit it stands beside, to hear one against the other.
    auto all = grooves;

    for (auto [kit, beside] : std::vector<std::pair<const char*, size_t>> { { "Modern", 0 }, { "Warehouse", 1 }, { "Foundry", 2 }, { "Body Music", 3 }, { "Broken Beat", 5 } })
    {
        all.push_back (grooves[beside]);
        all.back().kit = kit;
    }

    folder.createDirectory();

    for (auto& groove : all)
    {
        // A playhead at the groove's tempo, so the delay echoes in time with it.
        struct Tempo final : juce::AudioPlayHead
        {
            double bpm = 120.0;

            juce::Optional<PositionInfo> getPosition() const override
            {
                PositionInfo info;
                info.setBpm (bpm);
                return info;
            }
        } tempo;

        tempo.bpm = groove.bpm;
        BuiltInEffects::BasicDrumsProcessor drums;
        drums.setPlayHead (&tempo);
        drums.applyKit (DrumMachine::kits()[(size_t) DrumMachine::kitIndex (groove.kit)]);
        const auto step = 60.0 / groove.bpm / 4.0;
        Notes notes;

        for (int bar = 0; bar < 4; ++bar)
            for (auto& row : bar == 3 ? groove.fill : groove.rows)
                for (int i = 0; row.steps[i] != 0 && i < 16; ++i)
                    if (row.steps[i] != '.')
                        notes.push_back ({ row.note, (bar * 16 + i) * step, row.steps[i] == 'x' ? 1.0f : 0.6f });

        notes.push_back ({ 49, 0.0, 0.9f });
        notes.push_back ({ 49, 64 * step, 0.9f });
        notes.push_back ({ 36, 64 * step, 1.0f });
        const auto audio = play (drums, notes, 64 * step + 3.0);
        auto file = folder.getChildFile (juce::String ("Basic Drums - ") + groove.kit + ".wav");
        file.deleteFile();

        std::unique_ptr<juce::OutputStream> stream (new juce::FileOutputStream (file));
        const auto options = juce::AudioFormatWriterOptions{}.withSampleRate (rate).withNumChannels (2).withBitsPerSample (24);

        if (auto writer = juce::WavAudioFormat().createWriterFor (stream, options))
            writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());

        std::cout << file.getFileName() << ": peak " << juce::Decibels::gainToDecibels (peakOf (audio)) << " dB" << std::endl;
    }
}

void saveDrumMachineSounds (const juce::File& folder)
{
    folder.createDirectory();

    const auto write = [&folder] (const juce::String& name, const juce::AudioBuffer<float>& audio)
    {
        auto file = folder.getChildFile (name + ".wav");
        file.deleteFile();
        std::unique_ptr<juce::OutputStream> stream (new juce::FileOutputStream (file));
        const auto options = juce::AudioFormatWriterOptions{}.withSampleRate (rate).withNumChannels (2).withBitsPerSample (24);

        if (auto writer = juce::WavAudioFormat().createWriterFor (stream, options))
            writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
    };

    // Every note of the Standard kit, hard and soft, the kit's level at 0 dB.
    BuiltInEffects::BasicDrumsProcessor drums;
    drums.applyKit (DrumMachine::kits()[0]);
    set (drums.levelParameter(), 0.0f);

    for (int note = DrumMachine::firstNote; note <= DrumMachine::lastNote; ++note)
    {
        const auto sound = *DrumMachine::soundFor (note);
        const auto seconds = juce::jmin (6.0, drums.parameterOf (sound, "decay")->get() * 0.001 * 1.3 + 0.3);

        for (auto velocity : { 127, 80, 40 })
        {
            write ("Standard " + juce::String (note) + " " + DrumMachine::nameOf (sound) + " v" + juce::String (velocity),
                   play (drums, { { note, 0.0, (float) velocity / 127.0f } }, seconds));

            if (auto* type = drums.typeParameter (sound))
            {
                type->setValueNotifyingHost (1.0f);
                write ("Modern " + juce::String (note) + " " + DrumMachine::nameOf (sound) + " v" + juce::String (velocity),
                       play (drums, { { note, 0.0, (float) velocity / 127.0f } }, seconds));
                type->setValueNotifyingHost (0.0f);
            }
        }
    }

    // Each kit's main sounds as the kit plays them, one after another: kick, snare, clap, rim, closed and open hat, the toms, ride, crash.
    for (auto& kit : DrumMachine::kits())
    {
        BuiltInEffects::BasicDrumsProcessor one;
        one.applyKit (kit);
        Notes notes;
        int i = 0;

        for (auto note : { 36, 36, 38, 38, 39, 37, 42, 42, 42, 42, 46, 41, 45, 48, 51, 51, 49 })
        {
            notes.push_back ({ note, i * 0.5, i == 1 || i == 3 || i == 7 || i == 9 || i == 15 ? 0.4f : 1.0f });
            ++i;
        }

        write ("Kit sounds - " + kit.name, play (one, notes, i * 0.5 + 3.0));
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
    testWindow();
    testEverySound (drums);
    testNoClicks (drums, "Standard");
    testNoClicks (drums, "Modern");
    testSettings (drums);
    testTypes (drums);
    testEffects (drums);
    testKitsAndState (drums);
    testRendering (drums);
}
