#include <JuceHeader.h>
#include "../Source/DrumMachine.h"
#include "../Source/DrumMachineEditor.h"
#include "../Source/InstrumentKeys.h"
#include "../Source/NoteEditor.h"
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
                   && drums->getTotalNumOutputChannels() == 2 && drums->getParameters().size() == DrumMachine::numSounds * 12 + 21 + 7 + 9 + DrumMachine::numSounds * 3 + DrumMachine::numUserPads * 15,
               "drums: Basic Drums is made from its description (the same ID, so sessions find it), an instrument with no audio input, a stereo output and "
                   + juce::String (drums != nullptr ? drums->getParameters().size() : 0) + " parameters (twelve for each of 30 sounds, a type for the 21 that have a Modern one, the kick's character, the kit's drive, glue, level, reverb size, delay time and feedback, and after all those a type for the other nine, a Sample's start, reverse and pitch envelope for each sound, and fifteen for each of 34 user pads)");

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

        // Clicking a pad plays it and selects it; with no keys under the window, at the usual velocity of 100.
        const auto hits = drums.getEngine().hitsOf (Sound::snare);
        window->clickPad (Sound::snare);
        juce::AudioBuffer<float> block (2, 512);
        juce::MidiBuffer midi;
        drums.processBlock (block, midi);
        check (window->getSelected() == Sound::snare && drums.getEngine().hitsOf (Sound::snare) == hits + 1 && juce::roundToInt (drums.getEngine().lastVelocityOf (Sound::snare) * 127.0f) == 100
                   && block.getMagnitude (0, 512) > 0.05f,
               "drums window: clicking the snare's pad plays it and selects it, at velocity 100 with no keys under the window");

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

        // Every sound has a selector for its type, and its picture follows it; a sound with no Modern type is offered Classic and Sample.
        const auto classicShown = window->getTypeText();
        const auto classicSeconds = window->getPictureSeconds();
        drums.setType (Sound::snare, DrumMachine::modernType);
        window->select (Sound::snare);
        const auto modernShown = window->getTypeText();
        window->select (Sound::rim);
        const auto rimShown = window->getTypeText();
        drums.setType (Sound::snare, DrumMachine::classicType);
        window->select (Sound::kick);
        check (classicShown == "Classic" && modernShown == "Modern" && rimShown == "Classic" && window->getTypeText() == "Classic" && classicSeconds > 0.0
                   && DrumMachine::typeNamesFor (Sound::rim) == juce::StringArray ({ "Classic", "Sample" })
                   && DrumMachine::typeNamesFor (Sound::snare) == juce::StringArray ({ "Classic", "Modern", "Sample" }) && ! window->showsLayers(),
               "drums window: the snare's type reads Classic, then Modern once chosen, of Classic, Modern and Sample; the rim's reads Classic, of Classic and Sample; no layers are shown for either");

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
    /** The kit as it stands: nothing in it steps. */
    void checkNoClicks (BuiltInEffects::BasicDrumsProcessor& drums, const juce::String& tag)
    {
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

    void testNoClicks (BuiltInEffects::BasicDrumsProcessor& drums, const juce::String& kitName)
    {
        drums.applyKit (DrumMachine::kits()[(size_t) DrumMachine::kitIndex (kitName)]);
        checkNoClicks (drums, kitName == "Standard" ? juce::String ("drums: ") : "drums (the " + kitName + " kit's sounds): ");
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
        const auto setType = [&drums] (Sound s, int index) { drums.setType (s, index); };

        // Twenty-one sounds have a Modern type: another sound at the level of the Classic one, which is left as it was.
        juce::StringArray sameSound, offLevel, noTone, rigid;
        int withTypes = 0;
        float mostApart = 0.0f;

        for (int i = 0; i < DrumMachine::numSounds; ++i)
        {
            const auto sound = (Sound) i;

            if (! DrumMachine::hasTypes (sound))
            {
                if (DrumMachine::typeNamesFor (sound).contains ("Modern"))
                    sameSound.add (DrumMachine::nameOf (sound) + " has a Modern type");

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

        // (Nor had it what came with samples on the pads: nine more types, and a Sample's own three settings.)
        for (int i = 0; before != nullptr && i < DrumMachine::numSounds; ++i)
            for (auto what : { "_type", "_start", "_reverse", "_bend" })
                if (const auto id = DrumMachine::idOf ((Sound) i) + what; before->hasAttribute (id))
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
                   && removed == 4 * DrumMachine::numSounds && before->getChildByName ("SAMPLES") == nullptr && untyped.getKitName() == "Dark Techno" && ! untyped.kitModified() && untyped.currentSettings() == drums.currentSettings()
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
    //==============================================================================
    // Samples on the pads. Every sound here is made in memory and written into a folder of this
    // run's own, which is taken away when the tests are done.

    /** A tone: a sine (a cosine when fromPeak, so it starts off zero) at a level, dying away with
        a time constant when one is given. */
    juce::AudioBuffer<float> tone (double hz, int frames, float level, bool fromPeak = false, double dieAway = 0.0, double sampleRate = rate, int channels = 1)
    {
        juce::AudioBuffer<float> b (channels, frames);

        for (int i = 0; i < frames; ++i)
        {
            const auto angle = juce::MathConstants<double>::twoPi * hz * (double) i / sampleRate;
            const auto fall = dieAway > 0.0 ? std::exp (-(double) i / (dieAway * sampleRate)) : 1.0;
            b.setSample (0, i, level * (float) ((fromPeak ? std::cos (angle) : std::sin (angle)) * fall));

            for (int ch = 1; ch < channels; ++ch)
                b.setSample (ch, i, 0.0f);
        }

        return b;
    }

    juce::File writeSound (const juce::File& folder, const juce::String& name, const juce::AudioBuffer<float>& audio, double sampleRate = rate)
    {
        juce::String error;
        return Sampler::writeSample (audio, sampleRate, name, {}, error, folder);
    }

    /** The furthest the left side of `out` is, from `at` on, from `expected` times `gain` (its
        frames from `from`, read backwards when `back`), over `count` samples after the first `skip`. */
    float furthestFrom (const juce::AudioBuffer<float>& out, int at, const juce::AudioBuffer<float>& expected, float gain, int count, int skip = 12, int from = 0, bool back = false)
    {
        float most = 0.0f;

        for (int i = skip; i < count; ++i)
        {
            const auto frame = back ? expected.getNumSamples() - 1 - from - i : from + i;
            const auto want = juce::isPositiveAndBelow (frame, expected.getNumSamples()) ? expected.getSample (0, frame) * gain : 0.0f;
            most = juce::jmax (most, std::abs (out.getSample (0, at + i) - want));
        }

        return most;
    }

    void testSamples()
    {
        using BuiltInEffects::BasicDrumsProcessor;
        const auto folder = juce::File::createTempFile ("drumsamples");
        folder.createDirectory();

        // The sounds: a thud that starts off zero and dies away, a tone cut off at its peak, three
        // short tones to tell layers apart, a long tone, a tone at half the rate, and one on the left alone.
        const auto thud = tone (110.0, 12000, 0.8f, true, 0.05);
        const auto cut = tone (100.0, 4920, 0.8f);
        const auto a = tone (200.0, 2400, 0.5f), b = tone (400.0, 2400, 0.5f), c = tone (800.0, 2400, 0.5f);
        const auto held = tone (300.0, 28800, 0.5f);
        const auto slow = tone (110.0, 6000, 0.5f, false, 0.0, 24000.0);
        const auto leftOnly = tone (220.0, 4800, 0.5f, false, 0.0, rate, 2);
        const auto thudFile = writeSound (folder, "Thud", thud), cutFile = writeSound (folder, "Cut", cut);
        const auto aFile = writeSound (folder, "A", a), bFile = writeSound (folder, "B", b), cFile = writeSound (folder, "C", c);
        const auto heldFile = writeSound (folder, "Held", held), slowFile = writeSound (folder, "Slow", slow, 24000.0), leftFile = writeSound (folder, "Left", leftOnly);
        check (thudFile.existsAsFile() && cutFile.existsAsFile() && aFile.existsAsFile() && bFile.existsAsFile() && cFile.existsAsFile()
                   && heldFile.existsAsFile() && slowFile.existsAsFile() && leftFile.existsAsFile(),
               "drum samples: the test's own sounds are written into a folder of this run's");

        // A kit with nothing on its bus and its level at 0 dB, so what a pad plays is what comes out.
        const auto clean = [] (BasicDrumsProcessor& d)
        {
            d.applyKit (DrumMachine::kits()[0]);
            set (d.levelParameter(), 0.0f);

            for (int i = 0; i < DrumMachine::numSounds; ++i)
                for (auto what : { "tune", "tone", "level", "pan", "drive" })
                    set (d.parameterOf ((Sound) i, what), 0.0f);
        };

        const auto kick = DrumMachine::padNote (Sound::kick), snare = DrumMachine::padNote (Sound::snare);

        // Velocities as MIDI carries them, in 127 steps: a soft hit, a middling one and a hard one.
        constexpr auto soft = 38.0f / 127.0f, middling = 64.0f / 127.0f, hard = 114.0f / 127.0f;

        // A pad given a sound plays it: its type becomes Sample, and at its own pitch and rate the
        // sample comes out as it is, at the level it was recorded at.
        {
            BasicDrumsProcessor drums;
            clean (drums);
            const auto silentBefore = [&] { drums.setType (Sound::kick, DrumMachine::sampleType); const auto out = play (drums, { { kick, 0.0, 1.0f } }, 0.3); return peakOf (out); }();
            const auto error = drums.addFiles (Sound::kick, { thudFile });
            const auto out = play (drums, { { kick, 0.0, 1.0f } }, 0.5);
            const auto off = furthestFrom (out, 0, thud, 1.0f, 12000 - 30);
            const auto half = play (drums, { { kick, 0.0, middling } }, 0.5);
            check (error.isEmpty() && juce::exactlyEqual (silentBefore, 0.0f) && drums.getType (Sound::kick) == DrumMachine::sampleType && drums.getLayers (Sound::kick).size() == 1 && off < 2.0e-6f
                       && furthestFrom (half, 0, thud, middling, 12000 - 30) < 2.0e-6f && std::abs (peakOf (out) - 0.8f) < 0.02f,
                   "drum samples: a pad whose type is Sample is silent with no layers; given a sound it plays it as it is, frame for frame (at most "
                       + juce::String (off, 8) + " away), at the level it was recorded at, and at half the velocity at half that");

            // It starts off zero, so it rises over a quarter of a millisecond; its end is where the sample's is.
            const auto startStep = largestStep (out, 0, 14);
            check (juce::exactlyEqual (out.getSample (0, 0), 0.0f) && startStep < 0.8f / 12.0f * 1.3f && lastSounding (out) < 12000 && lastSounding (out) > 11900,
                   "drum samples: a sample that starts off zero rises from silence (its largest step " + juce::String (startStep, 4) + " of a 0.8 start) and ends where the sample does");

            // The decay: shorter than the sample, it fades it to nothing; at its top or longer, the sample plays whole.
            set (drums.parameterOf (Sound::kick, "decay"), 100.0f);
            const auto shortened = play (drums, { { kick, 0.0, 1.0f } }, 0.5);
            const auto fadeStep = largestStep (shortened, 2880, 4810), ownStep = largestStep (out, 2880, 4810);
            set (drums.parameterOf (Sound::kick, "decay"), 5000.0f);
            const auto whole = play (drums, { { kick, 0.0, 1.0f } }, 0.5);
            check (lastSounding (shortened) < 4800 && lastSounding (shortened) > 4000 && furthestFrom (shortened, 0, thud, 1.0f, 2880) < 2.0e-6f
                       && fadeStep <= ownStep * 1.05f + 1.0e-4f && same (whole, out),
                   "drum samples: a decay of 100 ms fades a 250 ms sample to nothing by then, untouched for the first 60 ms and with no step (" + juce::String (fadeStep, 4)
                       + " against the sample's own " + juce::String (ownStep, 4) + "); at the decay's top it plays whole");

            // Tune repitches it; start skips into it; reverse plays it backwards; the pitch envelope bends its start.
            set (drums.parameterOf (Sound::kick, "tune"), 12.0f);
            const auto up = lastSounding (play (drums, { { kick, 0.0, 1.0f } }, 0.5));
            set (drums.parameterOf (Sound::kick, "tune"), 0.0f);
            set (drums.parameterOf (Sound::kick, "start"), 50.0f);
            const auto later = play (drums, { { kick, 0.0, 1.0f } }, 0.5);
            set (drums.parameterOf (Sound::kick, "start"), 0.0f);
            drums.reverseParameter (Sound::kick)->setValueNotifyingHost (1.0f);
            const auto backwards = play (drums, { { kick, 0.0, 1.0f } }, 0.5);
            drums.reverseParameter (Sound::kick)->setValueNotifyingHost (0.0f);
            set (drums.parameterOf (Sound::kick, "bend"), 12.0f);
            const auto bent = play (drums, { { kick, 0.0, 1.0f } }, 0.5);
            set (drums.parameterOf (Sound::kick, "bend"), 0.0f);
            check (std::abs (up - 6000) < 40 && furthestFrom (later, 0, thud, 1.0f, 5900, 12, 6000) < 2.0e-6f && furthestFrom (backwards, 0, thud, 1.0f, 11900, 12, 0, true) < 2.0e-6f
                       && ! same (bent, out) && lastSounding (bent) < lastSounding (out) - 200,
                   "drum samples: an octave up it lasts half as long (" + juce::String (up) + " samples of 12000); a start of 50 % plays from its middle; reverse plays it backwards; a pitch envelope of an octave makes it another sound that ends sooner");

            // A sample cut off at its own end is faded there; one at another rate plays at its own pitch; a stereo one keeps its sides.
            drums.setLayers (Sound::kick, {});
            drums.addFiles (Sound::kick, { cutFile });
            const auto ended = play (drums, { { kick, 0.0, 1.0f } }, 0.3);
            const auto endStep = largestStep (ended, 4880, 4930);
            drums.setLayers (Sound::kick, {});
            drums.addFiles (Sound::kick, { slowFile });
            const auto slowed = lastSounding (play (drums, { { kick, 0.0, 1.0f } }, 0.5));
            drums.setLayers (Sound::kick, {});
            drums.addFiles (Sound::kick, { leftFile });
            const auto sides = play (drums, { { kick, 0.0, 1.0f } }, 0.3);
            check (endStep < 0.8f / 24.0f * 1.3f + 0.011f && lastSounding (ended) < 4920 && std::abs (slowed - 12000) < 60
                       && sides.getMagnitude (0, 0, sides.getNumSamples()) > 0.45f && juce::exactlyEqual (sides.getMagnitude (1, 0, sides.getNumSamples()), 0.0f),
                   "drum samples: a sample that stops at its peak is faded over its last half millisecond (largest step " + juce::String (endStep, 4)
                       + "); one recorded at 24 kHz lasts as long at 48 kHz (" + juce::String (slowed) + " samples for 6000 frames); a sample on the left alone stays there");
        }

        // Which of the three short tones a hit at `at` played ('A', 'B', 'C'; '-' silence, '?' something else).
        const auto heard = [&] (const juce::AudioBuffer<float>& out, double second, float velocity)
        {
            const auto at = (int) std::round (second * rate);

            if (peakOf (out, at, 2400) < 1.0e-6f)
                return '-';

            for (auto [name, sound] : { std::pair { 'A', &a }, std::pair { 'B', &b }, std::pair { 'C', &c } })
                if (furthestFrom (out, at, *sound, velocity, 2300) < 1.0e-4f)
                    return name;

            return '?';
        };

        const auto layersOf = [&] (std::initializer_list<std::tuple<juce::File, int, int>> wanted)
        {
            std::vector<DrumMachine::Layer> layers;

            for (auto& [file, low, high] : wanted)
            {
                juce::String error;
                DrumMachine::Layer l;
                l.sample = Sampler::sharedSample (file, error);
                l.file = file;
                l.lowVelocity = low;
                l.highVelocity = high;
                layers.push_back (l);
            }

            return layers;
        };

        // Layers by velocity, and round robin among those that share a hit's velocity.
        {
            BasicDrumsProcessor drums;
            clean (drums);
            drums.addFiles (Sound::snare, { aFile, bFile, cFile });
            const auto three = drums.getLayers (Sound::snare);
            const Notes six { { snare, 0.0, 1.0f }, { snare, 0.1, 1.0f }, { snare, 0.2, 1.0f }, { snare, 0.3, 1.0f }, { snare, 0.4, 1.0f }, { snare, 0.5, 1.0f } };
            const auto turns = play (drums, six, 0.7);
            juce::String order;

            for (int i = 0; i < 6; ++i)
                order << heard (turns, 0.1 * i, 1.0f);

            check (three.size() == 3 && three[0].lowVelocity == 1 && three[2].highVelocity == 127 && order == "ABCABC" && same (turns, play (drums, six, 0.7))
                       && same (turns, play (drums, six, 0.7, 64)) && same (turns, play (drums, six, 0.7, 37)),
                   "drum samples: three sounds dropped together are three layers over every velocity, taking turns in their order (" + order
                       + "), the same played again and at blocks of 512, 64 and 37 samples");

            drums.setLayers (Sound::snare, layersOf ({ { aFile, 1, 63 }, { bFile, 64, 127 } }));
            const auto split = play (drums, { { snare, 0.0, soft }, { snare, 0.1, hard }, { snare, 0.2, soft } }, 0.4);
            const auto splitOrder = juce::String::charToString ((juce::juce_wchar) heard (split, 0.0, soft)) + heard (split, 0.1, hard) + heard (split, 0.2, soft);
            drums.setLayers (Sound::snare, layersOf ({ { aFile, 1, 40 }, { bFile, 80, 127 } }));
            const auto gap = play (drums, { { snare, 0.0, middling } }, 0.2);
            drums.setLayers (Sound::snare, layersOf ({ { aFile, 1, 127 }, { bFile, 64, 127 }, { cFile, 64, 127 } }));
            const auto mixed = play (drums, { { snare, 0.0, 1.0f }, { snare, 0.1, soft }, { snare, 0.2, 1.0f }, { snare, 0.3, 1.0f }, { snare, 0.4, soft } }, 0.6);
            const auto mixedOrder = juce::String::charToString ((juce::juce_wchar) heard (mixed, 0.0, 1.0f)) + heard (mixed, 0.1, soft) + heard (mixed, 0.2, 1.0f) + heard (mixed, 0.3, 1.0f) + heard (mixed, 0.4, soft);
            check (splitOrder == "ABA" && juce::exactlyEqual (peakOf (gap), 0.0f) && mixedOrder == "AACAA",
                   "drum samples: a soft layer and a hard one play for their own velocities (" + splitOrder + "); a velocity no layer takes is silent; with one layer for every velocity and two more for hard hits, each hit moves the turn on among the layers for its velocity ("
                       + mixedOrder + ")");

            // The nine sounds with no Modern type take samples too, and a pad's settings work on a sample.
            const auto rim = DrumMachine::padNote (Sound::rim);
            drums.addFiles (Sound::rim, { aFile });
            const auto rimOut = play (drums, { { rim, 0.0, 1.0f } }, 0.5);
            juce::StringArray unchanged;

            for (auto [what, value] : { std::pair { "tone", 60.0f }, std::pair { "level", -6.0f }, std::pair { "pan", 50.0f }, std::pair { "drive", 60.0f },
                                        std::pair { "crush", 60.0f }, std::pair { "reverb", 50.0f }, std::pair { "delay", 50.0f } })
            {
                const auto was = drums.parameterOf (Sound::rim, what)->get();
                set (drums.parameterOf (Sound::rim, what), value);

                if (same (play (drums, { { rim, 0.0, 1.0f } }, 0.5), rimOut))
                    unchanged.add (what);

                set (drums.parameterOf (Sound::rim, what), was);
            }

            drums.filterParameter (Sound::rim)->setValueNotifyingHost (drums.filterParameter (Sound::rim)->convertTo0to1 (1.0f));
            set (drums.parameterOf (Sound::rim, "cutoff"), 150.0f);

            if (same (play (drums, { { rim, 0.0, 1.0f } }, 0.5), rimOut))
                unchanged.add ("filter");

            check (drums.getType (Sound::rim) == DrumMachine::sampleType && heard (rimOut, 0.0, 1.0f) == 'A' && unchanged.isEmpty(),
                   "drum samples: the rim, which has no Modern type, plays a sample given to it; tone, level, pan, drive, crush, the two sends and the filter each change a sampled sound"
                       + (unchanged.isEmpty() ? juce::String() : " (no change: " + unchanged.joinIntoString (", ") + ")"));
        }

        // Chokes work across types, and nothing stops dead.
        {
            BasicDrumsProcessor drums;
            clean (drums);
            const auto open = DrumMachine::padNote (Sound::openHat), closed = DrumMachine::padNote (Sound::closedHat);
            const auto at = (int) (0.1 * rate);
            juce::String whySampled, whySynth, whyAgain, whyOff;

            // A sampled open hat cut by a synthesized closed one.
            drums.addFiles (Sound::openHat, { heldFile });
            const auto sampledCut = fadesOut (minus (play (drums, { { open, 0.0, 1.0f }, { closed, 0.1, 1.0f } }, 0.4), play (drums, { { closed, 0.1, 1.0f } }, 0.4)), at, whySampled);
            const auto again = fadesOut (minus (play (drums, { { open, 0.0, 1.0f }, { open, 0.1, 1.0f } }, 0.4), play (drums, { { open, 0.1, 1.0f } }, 0.4)), at, whyAgain);
            const auto off = fadesOut (play (drums, { { open, 0.0, 1.0f } }, 0.4, 512, 0.1), at, whyOff);
            const auto ringing = peakOf (play (drums, { { open, 0.0, 1.0f } }, 0.4), at + 400, 4000);

            // A synthesized open hat cut by a sampled closed one.
            drums.setLayers (Sound::openHat, {});
            drums.setType (Sound::openHat, DrumMachine::classicType);
            drums.addFiles (Sound::closedHat, { aFile });
            const auto alone = play (drums, { { open, 0.0, 1.0f } }, 0.4);
            const auto synthCut = fadesOut (minus (play (drums, { { open, 0.0, 1.0f }, { closed, 0.1, 1.0f } }, 0.4), play (drums, { { closed, 0.1, 1.0f } }, 0.4)), at, whySynth, &alone);
            check (sampledCut && synthCut && again && off && ringing > 0.4f,
                   "drum samples: a sampled open hat is cut by a synthesized closed one (" + whySampled + "), a synthesized open hat by a sampled closed one (" + whySynth
                       + "), a sample struck again fades its last hit (" + whyAgain + "), and All Sound Off fades it (" + whyOff + "): each over 3 ms, none stopping dead");
        }

        // The state keeps the layers, each sample named by its file; a kit with samples is its own, edited.
        const Notes groove { { kick, 0.0, 1.0f }, { snare, 0.1, soft }, { snare, 0.2, 1.0f }, { kick, 0.3, middling }, { snare, 0.4, 1.0f } };
        juce::MemoryBlock state;
        juce::AudioBuffer<float> sound;
        EffectPresets::Values presetValues;
        EffectPresets::Texts presetTexts;
        {
            BasicDrumsProcessor drums;
            clean (drums);
            drums.addFiles (Sound::kick, { thudFile });
            drums.setLayers (Sound::snare, layersOf ({ { aFile, 1, 80 }, { bFile, 81, 127 }, { cFile, 81, 127 } }));
            drums.setType (Sound::snare, DrumMachine::sampleType);
            set (drums.parameterOf (Sound::kick, "start"), 5.0f);
            sound = play (drums, groove, 0.7);
            drums.getStateInformation (state);
            presetValues = drums.getPresetValues();
            presetTexts = drums.getPresetTexts();

            BasicDrumsProcessor another;
            another.setStateInformation (state.getData(), (int) state.getSize());
            const auto back = another.getLayers (Sound::snare);
            check (drums.kitModified() && another.getMissing().isEmpty() && back.size() == 3 && back[0].file == aFile && back[0].highVelocity == 80 && back[1].lowVelocity == 81
                       && another.getLayers (Sound::kick).size() == 1 && another.getType (Sound::snare) == DrumMachine::sampleType
                       && another.getFiles().size() == 4 && another.bytesHeld() == drums.bytesHeld() && another.bytesHeld() > 0 && same (play (another, groove, 0.7), sound),
                   "drum samples: the state brings back each pad's layers, their files and velocity ranges, and the same sound; a kit with samples reads as edited");

            // A preset (a kit of the user's) carries them; a factory kit, and a preset saved before pads played samples, has none.
            BasicDrumsProcessor fromPreset;
            fromPreset.setPresetValues (presetValues);
            fromPreset.setPresetTexts (presetTexts);
            const auto presetSame = same (play (fromPreset, groove, 0.7), sound);
            auto older = presetValues;

            for (int i = 0; i < DrumMachine::numSounds; ++i)
                for (auto what : { "_start", "_reverse", "_bend" })
                    older.erase (DrumMachine::idOf ((Sound) i) + what);

            for (auto s : { Sound::rim, Sound::cowbell })
                older.erase (DrumMachine::idOf (s) + "_type");

            older[DrumMachine::idOf (Sound::kick) + "_type"] = 0.0;
            older[DrumMachine::idOf (Sound::snare) + "_type"] = 1.0;
            fromPreset.setType (Sound::rim, DrumMachine::sampleType);
            fromPreset.setPresetValues (older);
            fromPreset.setPresetTexts ({});
            const auto olderClean = fromPreset.getFiles().isEmpty() && fromPreset.getType (Sound::rim) == DrumMachine::classicType && fromPreset.getType (Sound::snare) == DrumMachine::modernType
                                    && juce::exactlyEqual (fromPreset.parameterOf (Sound::kick, "start")->get(), 0.0f);
            another.applyKit (DrumMachine::kits()[1]);
            check (presetSame && olderClean && another.getFiles().isEmpty() && ! another.kitModified() && another.getType (Sound::snare) == DrumMachine::classicType,
                   "drum samples: a preset carries the layers and plays the same; a preset from before samples takes them away and sets what it does not name back to where it starts; a factory kit has no samples");
        }

        // A sample that has moved: missing when the state is read, silent, and found again in the folder it is in now.
        {
            const auto moved = juce::File::createTempFile ("drumsmoved");
            moved.createDirectory();

            for (auto& f : { thudFile, aFile, bFile, cFile })
                f.moveFileTo (moved.getChildFile (f.getFileName()));

            BasicDrumsProcessor drums;
            drums.setStateInformation (state.getData(), (int) state.getSize());
            const auto missing = drums.getMissing().size();
            const auto silent = peakOf (play (drums, groove, 0.7));
            const auto layersKept = drums.getLayers (Sound::snare).size();
            const auto found = drums.locate (moved);
            check (missing == 4 && juce::exactlyEqual (silent, 0.0f) && layersKept == 3 && found == 4 && drums.getMissing().isEmpty() && same (play (drums, groove, 0.7), sound)
                       && drums.getLayers (Sound::kick)[0].file == moved.getChildFile (thudFile.getFileName()),
                   "drum samples: with its four samples moved, the state loads with four missing and the pads silent, their layers kept; pointed at the folder they are in now, all four come back and it sounds the same");

            // Saved into a session, a sample is named relative to it too, so the session's folder can be moved.
            const auto session = juce::File::createTempFile ("drumsession");
            session.getChildFile ("Samples").createDirectory();

            for (auto name : { "Thud", "A", "B", "C" })
                moved.getChildFile (juce::String (name) + ".wav").moveFileTo (session.getChildFile ("Samples").getChildFile (juce::String (name) + ".wav"));

            BasicDrumsProcessor inSession;
            inSession.setStateInformation (state.getData(), (int) state.getSize());
            inSession.locate (session.getChildFile ("Samples"));
            juce::MemoryBlock sessionState;
            {
                const Sampler::SavingInto saving (session);
                inSession.getStateInformation (sessionState);
            }

            const auto elsewhere = juce::File::createTempFile ("drumsessionmoved");
            session.moveFileTo (elsewhere);
            const auto folderBefore = Sampler::getSessionFolder();
            Sampler::setSessionFolder (elsewhere);
            BasicDrumsProcessor reopened;
            reopened.setStateInformation (sessionState.getData(), (int) sessionState.getSize());
            Sampler::setSessionFolder (folderBefore);
            check (reopened.getMissing().isEmpty() && same (play (reopened, groove, 0.7), sound)
                       && reopened.getLayers (Sound::kick)[0].file == elsewhere.getChildFile ("Samples").getChildFile ("Thud.wav"),
                   "drum samples: saved into a session its samples are named relative to it, and are found beside the session after its folder has moved");

            // Save Kit with Samples: the kit and copies of its samples in one folder, which opens from wherever it is moved to.
            const auto kitFolder = juce::File::createTempFile ("drumkit").getSiblingFile ("Test Kit " + juce::String::toHexString (juce::Random::getSystemRandom().nextInt()));
            juce::StringArray notThere;
            const auto saveError = reopened.saveKitWithSamples (kitFolder, notThere);
            const auto kitFile = kitFolder.getChildFile (kitFolder.getFileName() + BasicDrumsProcessor::kitFileExtension);
            const auto copies = kitFolder.getChildFile ("Samples").findChildFiles (juce::File::findFiles, false, "*.wav").size();
            const auto kitMoved = kitFolder.getSiblingFile (kitFolder.getFileName() + " moved");
            const auto written = kitFile.existsAsFile();
            kitFolder.moveFileTo (kitMoved);
            elsewhere.deleteRecursively();
            BasicDrumsProcessor opened;
            const auto openError = opened.loadKitFile (kitMoved.getChildFile (kitFile.getFileName()));
            BasicDrumsProcessor notAKit;
            const auto refused = notAKit.loadKitFile (kitMoved.getChildFile ("Samples").getChildFile ("A.wav"));
            check (saveError.isEmpty() && notThere.isEmpty() && written && copies == 4 && openError.isEmpty() && opened.getMissing().isEmpty()
                       && opened.getKitName() == kitFolder.getFileName() && same (play (opened, groove, 0.7), sound)
                       && opened.getLayers (Sound::kick)[0].file == kitMoved.getChildFile ("Samples").getChildFile ("Thud.wav")
                       && refused.isNotEmpty() && notAKit.getKitName() == "Standard",
                   "drum samples: Save Kit with Samples writes the kit file and four copies of its samples into one folder; moved, with the originals gone, the kit opens from it and sounds the same; a file that is no kit is refused"
                       + (saveError.isEmpty() && openError.isEmpty() ? juce::String() : " (" + saveError + openError + ")"));

            kitMoved.deleteRecursively();
            moved.deleteRecursively();
        }

        // The limits: sixteen layers a pad, and what one kit may hold in memory. A sample longer than the pad's decay lengthens it.
        {
            const auto many = juce::File::createTempFile ("drumsmany");
            many.createDirectory();
            juce::Array<juce::File> seventeen;

            for (int i = 0; i < 17; ++i)
                seventeen.add (writeSound (many, "Hit " + juce::String (i + 1), tone (150.0 + 20.0 * i, 1200, 0.4f)));

            BasicDrumsProcessor drums;
            const auto tooMany = drums.addFiles (Sound::clap, seventeen);
            BasicDrumsProcessor small;
            small.setMostBytes (3 * 1200 * (juce::int64) sizeof (float));
            const auto tooMuch = small.addFiles (Sound::clap, seventeen);
            const auto longOne = writeSound (many, "Long", tone (90.0, 48000, 0.4f));
            const auto before = drums.parameterOf (Sound::closedHat, "decay")->get();
            drums.addFiles (Sound::closedHat, { longOne });
            check (drums.getLayers (Sound::clap).size() == 16 && tooMany.contains ("16 layers") && small.getLayers (Sound::clap).size() == 3 && tooMuch.contains ("held in memory")
                       && before < 200.0f && juce::exactlyEqual (drums.parameterOf (Sound::closedHat, "decay")->get(), 1000.0f),
                   "drum samples: a pad takes sixteen layers and says so of the seventeenth; a kit that may hold three samples' memory takes three and says why; a one-second sample on the closed hat (decay "
                       + juce::String (before, 0) + " ms) lengthens its decay to 1000 ms");
            many.deleteRecursively();
        }

        // The window says what a hit was: its velocity and the layer it played, lit in the list, whether
        // the hit came from a click on the pad or from a note. A click plays at the velocity of the keys
        // under the window. (From use: two layers spread on a user pad seemed to be one, because a click's
        // velocity came from its height on the pad, not from the keys' Velocity, and nothing showed it.)
        {
            const auto there = juce::File::createTempFile ("drumshits");
            there.createDirectory();
            const auto quiet = writeSound (there, "Soft", a), loud = writeSound (there, "Hard", b);
            BasicDrumsProcessor drums;
            drums.prepareToPlay (rate, 512);
            std::unique_ptr<juce::AudioProcessorEditor> made (drums.createEditorAndMakeActive());

            if (auto* window = dynamic_cast<DrumMachineEditor*> (made.get()))
            {
                // The window with its keys under it, as an instrument's window holds them.
                juce::Component holder;
                InstrumentKeys::Strip keys ([] (const juce::MidiMessage&) {});
                holder.addAndMakeVisible (*window);
                holder.addAndMakeVisible (keys);
                constexpr int pad = 51;   // the 52nd pad, a user pad
                const auto note = drums.getPadNote (pad);
                window->addFiles (pad, { quiet, loud });
                window->pressLayerButton ("spread");
                const auto before = window->getHitText().isEmpty() && window->getLitLayer() == -1;

                juce::AudioBuffer<float> block (2, 512);
                const auto play = [&] (int velocity)
                {
                    juce::MidiBuffer midi;

                    if (velocity > 0)
                        midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) velocity), 0);

                    drums.processBlock (block, midi);
                    window->showHits();
                };

                // A click on the pad plays at the keys' velocity: at 20 the first layer, at 100 the second.
                keys.setVelocity (20);
                window->clickPad (pad);
                play (0);
                const auto low = window->getHitText() == "hit 20: layer 1" && window->getLitLayer() == 0;
                keys.setVelocity (100);
                window->clickPad (pad);
                play (0);
                const auto high = window->getHitText() == "hit 100: layer 2" && window->getLitLayer() == 1;

                // A note, as the keys, a controller or a sequence send it.
                play (20);
                const auto played = window->getHitText() == "hit 20: layer 1" && window->getLitLayer() == 0;
                play (64);
                const auto harder = window->getHitText() == "hit 64: layer 2" && window->getLitLayer() == 1;

                // A velocity no layer takes says so, and lights none.
                auto layers = drums.getLayers (pad);
                layers[0].lowVelocity = 30;
                drums.setLayers (pad, layers);
                play (20);
                const auto none = window->getHitText() == "hit 20: no layer" && window->getLitLayer() == -1;

                // Another pad's hits are not this pad's, and another pad selected starts with nothing said.
                window->addFiles ((int) Sound::clap, { quiet });
                window->selectPad (pad);
                play (64);
                drums.auditionPad ((int) Sound::clap, 1.0f);
                play (0);
                const auto apart = window->getHitText() == "hit 64: layer 2" && window->getLitLayer() == 1;
                window->selectPad ((int) Sound::clap);
                const auto other = apart && window->getHitText().isEmpty() && window->getLitLayer() == -1;

                // A user pad's note is named as the keys under the window name it: middle C is C4.
                const auto named = note == 103 && window->getPadText (pad) == "Pad 52 / 103 G7";
                holder.removeAllChildren();

                check (before && low && high && played && harder && none && other && named,
                       juce::String ("drums window: the layers say the last hit's velocity and light the layer it played, for a click on the pad (at the velocity of the keys under the window) and for a note; a velocity no layer takes says so; the pad's note is named with middle C as C4")
                           + (before ? "" : " [before]") + (low ? "" : " [low]") + (high ? "" : " [high]") + (played ? "" : " [note]") + (harder ? "" : " [harder]")
                           + (none ? "" : " [none]") + (other ? "" : " [other]") + (named ? "" : " [named " + window->getPadText (pad) + "]"));
            }

            there.deleteRecursively();
        }

        // The keys strip under the window: a key clicked on its piano and a typed key both play at the
        // strip's Velocity (C and V), and that is the velocity the layer is chosen by.
        {
            const auto there = juce::File::createTempFile ("drumskeys");
            there.createDirectory();
            const auto quiet = writeSound (there, "Soft", a), loud = writeSound (there, "Hard", b);
            BasicDrumsProcessor drums;
            drums.prepareToPlay (rate, 512);
            constexpr int pad = 51;
            const auto note = drums.getPadNote (pad);
            drums.addFiles (pad, { quiet, loud });
            auto layers = drums.getLayers (pad);
            layers[0].highVelocity = 63;
            layers[1].lowVelocity = 64;
            drums.setLayers (pad, layers);

            juce::MidiBuffer sent;
            InstrumentKeys::Strip strip ([&sent] (const juce::MidiMessage& m) { sent.addEvent (m, 0); });
            strip.setSize (1100, InstrumentKeys::Strip::height);
            strip.setVisible (true);
            strip.addToDesktop (0);   // a click is taken only by a piano that is somewhere
            strip.setAlwaysOnTop (true);
            strip.setTopLeftPosition (40, 80);
            strip.toFront (false);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (300);
            juce::AudioBuffer<float> block (2, 512);
            const auto takeIn = [&]
            {
                drums.processBlock (block, sent);
                sent.clear();
                return std::pair { juce::roundToInt (drums.getEngine().lastVelocityOfPad (pad) * 127.0f), drums.getEngine().lastLayerOfPad (pad) };
            };

            juce::MidiKeyboardComponent* piano = nullptr;

            for (auto* child : strip.getChildren())
                if (auto* k = dynamic_cast<juce::MidiKeyboardComponent*> (child))
                    piano = k;

            // A click is taken only by a piano the screen shows at that place: where it cannot be (a
            // locked screen, a window over it), the typed keys alone are checked, and the line says so.
            const auto clickable = piano != nullptr && piano->contains (piano->getRectangleForKey (note).getCentre());
            juce::String got = clickable ? "" : " piano not clickable here, typed keys only;";
            auto good = piano != nullptr;

            for (auto velocity : { 20, 100 })
            {
                strip.setVelocity (velocity);
                const auto layer = velocity < 64 ? 0 : 1;

                if (clickable)
                {
                    const auto key = piano->getRectangleForKey (note);

                    for (auto down : { 0.2f, 0.9f })   // high and low on the key: its height sets nothing
                    {
                        const auto at = juce::Point<float> (key.getCentreX(), key.getY() + key.getHeight() * down);
                        const juce::MouseEvent e (juce::Desktop::getInstance().getMainMouseSource(), at, juce::ModifierKeys::leftButtonModifier, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                                  piano, piano, juce::Time::getCurrentTime(), at, juce::Time::getCurrentTime(), 1, false);
                        piano->mouseDown (e);
                        const auto [v, l] = takeIn();
                        piano->mouseUp (e);
                        takeIn();
                        got << " piano " << v << "/" << l;
                        good = good && v == velocity && l == layer;
                    }
                }

                // A typed key: the octave is set so that the first key of the row is the pad's note's C.
                strip.setOctave (note / 12 - 1);
                juce::juce_wchar typed = 0;

                for (auto letter : juce::String ("qwertyuiop234567890asdfghjkl"))
                    if (InstrumentKeys::noteFor (letter, strip.getOctave()) == note)
                        typed = letter;

                strip.keyDown (typed);
                const auto [v, l] = takeIn();
                strip.keyUp (typed);
                takeIn();
                got << " typed " << v << "/" << l;
                good = good && v == velocity && l == layer;
            }

            check (good, "drums keys: a key clicked on the window's piano and a typed key play a sampled user pad at the strip's velocity, 20 the soft layer and 100 the hard one, wherever on the key the click is (velocity/layer:" + got + ")");
            there.deleteRecursively();
        }

        // The window: sounds dropped on a pad become its layers; the list shows them, and its buttons work on them.
        {
            const auto there = juce::File::createTempFile ("drumswindow");
            there.createDirectory();
            const auto one = writeSound (there, "One", a), two = writeSound (there, "Two", b);
            BasicDrumsProcessor drums;
            drums.prepareToPlay (rate, 512);
            std::unique_ptr<juce::AudioProcessorEditor> made (drums.createEditorAndMakeActive());

            if (auto* window = dynamic_cast<DrumMachineEditor*> (made.get()))
            {
                const auto pad = window->padBounds (Sound::clap).getCentre();
                const auto interested = window->isInterestedInFileDrag ({ one.getFullPathName() }) && ! window->isInterestedInFileDrag ({ there.getChildFile ("notes.txt").getFullPathName() });
                window->filesDropped ({ one.getFullPathName(), two.getFullPathName() }, pad.x, pad.y);
                const auto dropped = window->getSelected() == Sound::clap && window->getTypeText() == "Sample" && window->showsLayers() && window->getNumLayersShown() == 2
                                     && window->getLayerText (0) == "One  1-127" && window->getLayerText (1) == "Two  1-127" && window->getKnobText ("Start").isNotEmpty();
                window->pressLayerButton ("spread");
                const auto spread = window->getLayerText (0) == "One  1-63" && window->getLayerText (1) == "Two  64-127";
                window->selectLayer (1);
                window->pressLayerButton ("up");
                const auto reordered = window->getLayerText (0) == "Two  64-127" && drums.getLayers (Sound::clap)[0].file == two;
                window->pressLayerButton ("remove");
                const auto removed = window->getNumLayersShown() == 1 && window->getLayerText (0) == "One  1-63";

                // On the selected sound's panel a drop goes to that sound; anywhere else it goes nowhere, and says so.
                window->select (Sound::cowbell);
                const auto panel = window->padAt ({ window->getWidth() - 60, window->getHeight() - 200 });
                window->filesDropped ({ two.getFullPathName() }, window->getWidth() - 60, window->getHeight() - 200);
                const auto onPanel = panel == (int) Sound::cowbell && drums.getLayers (Sound::cowbell).size() == 1 && window->getTypeText() == "Sample";
                window->filesDropped ({ two.getFullPathName() }, 30, 30);
                const auto nowhere = ! window->padAt ({ 30, 30 }).has_value() && window->getMessage().isNotEmpty() && drums.getLayers (Sound::cowbell).size() == 1;
                window->select (Sound::kick);
                check (interested && dropped && spread && reordered && removed && onPanel && nowhere && ! window->showsLayers() && window->getTypeText() == "Classic",
                       juce::String ("drum samples window: two sounds dropped on the clap's pad select it, make it a Sample and show two layers with their names and velocities; ")
                           + "Spread gives each its own range, the arrows reorder, - removes; a drop on the selected sound's panel goes to it, one on neither is refused with a line; a pad that is not a Sample shows no layers ("
                           + (interested ? "" : "interest ") + (dropped ? "" : "drop ") + (spread ? "" : "spread ") + (reordered ? "" : "order ") + (removed ? "" : "remove ")
                           + (onPanel ? "" : "panel ") + (nowhere ? "" : "nowhere ") + ")");
            }
            else
            {
                check (false, "drum samples window: it opens");
            }

            made = nullptr;
            there.deleteRecursively();
        }

        // The 34 user pads: each plays samples on a note of its own, with a name and a colour.
        {
            const auto there = juce::File::createTempFile ("drumuserpads");
            there.createDirectory();
            const auto aHere = writeSound (there, "A", a), bHere = writeSound (there, "B", b), thudHere = writeSound (there, "Thud", thud);
            const auto first = DrumMachine::numSounds, second = first + 1;
            BasicDrumsProcessor drums;
            clean (drums);
            const auto startsEmpty = drums.getPadName (first) == "Pad 31" && drums.getPadNote (first) == 82 && drums.getPadNote (DrumMachine::numPads - 1) == 115
                                     && drums.playsSamples (first) && ! BasicDrumsProcessor().kitModified() && juce::exactlyEqual (peakOf (play (drums, { { 82, 0.0, 1.0f } }, 0.2)), 0.0f);
            const auto error = drums.addFiles (first, { thudHere });
            const auto on82 = play (drums, { { 82, 0.0, 1.0f } }, 0.5);
            const auto elsewhere = peakOf (play (drums, { { 83, 0.0, 1.0f }, { 20, 0.1, 1.0f } }, 0.3));
            check (startsEmpty && error.isEmpty() && furthestFrom (on82, 0, thud, 1.0f, 12000 - 30) < 2.0e-6f && juce::exactlyEqual (elsewhere, 0.0f) && drums.kitModified(),
                   "drum user pads: 34 pads after the 30 sounds, named Pad 31 to Pad 64 on notes 82 to 115, silent until given a sound; pad 31 then plays its sample whole on note 82, as it is, and on no other note");

            // On a note a sound has, both play; clicked, the pad plays alone.
            drums.setPadNote (first, kick);
            const auto kickAlone = [&] { BasicDrumsProcessor plain; clean (plain); return play (plain, { { kick, 0.0, 1.0f } }, 0.5); }();
            const auto both = play (drums, { { kick, 0.0, 1.0f } }, 0.5);
            const auto layered = furthestFrom (minus (both, kickAlone), 0, thud, 1.0f, 12000 - 30);
            drums.auditionPad (first, 1.0f);
            const auto clicked = play (drums, {}, 0.5);
            check (layered < 1.0e-5f && furthestFrom (clicked, 0, thud, 1.0f, 12000 - 30) < 2.0e-6f && drums.getPadNote (first) == kick,
                   "drum user pads: moved to the kick's note, the pad's sample plays with the kick (the two together less the kick alone is the sample, at most " + juce::String (layered, 7)
                       + " away); a click on the pad plays it alone");

            // Struck again a user pad fades its last hit; its layers take turns; the render is the same at any block size.
            drums.setPadNote (first, 90);
            drums.setLayers (first, {});
            drums.addFiles (first, { heldFile.existsAsFile() ? heldFile : writeSound (there, "Held", held) });
            juce::String whyAgain;
            const auto restruck = fadesOut (minus (play (drums, { { 90, 0.0, 1.0f }, { 90, 0.1, 1.0f } }, 0.4), play (drums, { { 90, 0.1, 1.0f } }, 0.4)), 4800, whyAgain);
            drums.addFiles (second, { aHere, bHere });
            const Notes four { { 83, 0.0, 1.0f }, { 83, 0.1, 1.0f }, { 83, 0.2, 1.0f }, { 83, 0.3, 1.0f } };
            const auto turns = play (drums, four, 0.5);
            const auto order = juce::String::charToString ((juce::juce_wchar) heard (turns, 0.0, 1.0f)) + heard (turns, 0.1, 1.0f) + heard (turns, 0.2, 1.0f) + heard (turns, 0.3, 1.0f);
            check (restruck && order == "ABAB" && same (turns, play (drums, four, 0.5, 64)) && same (turns, play (drums, four, 0.5, 37)),
                   "drum user pads: a pad struck again fades its last hit over 3 ms (" + whyAgain + "); two layers take turns (" + order + "), the same at blocks of 512, 64 and 37");

            // The state and a preset keep a user pad's name, colour, note, settings and layers; a factory kit and an older state have none.
            drums.setPadName (second, "Clang");
            drums.setPadColour (second, juce::Colour (0xff2fb8a6));
            drums.setPadNote (second, 60);
            set (drums.parameterOf (second, "tune"), -5.0f);
            const Notes line { { 60, 0.0, 1.0f }, { 90, 0.1, soft }, { 60, 0.2, hard } };
            const auto padSound = play (drums, line, 0.8);
            juce::MemoryBlock padState;
            drums.getStateInformation (padState);
            BasicDrumsProcessor another;
            another.setStateInformation (padState.getData(), (int) padState.getSize());
            BasicDrumsProcessor fromPreset;
            fromPreset.setPresetValues (drums.getPresetValues());
            fromPreset.setPresetTexts (drums.getPresetTexts());
            const auto kept = another.getPadName (second) == "Clang" && another.getPadColour (second) == juce::Colour (0xff2fb8a6) && another.getPadNote (second) == 60
                              && another.getPadNote (first) == 90 && another.getLayers (second).size() == 2 && another.currentSettings() == drums.currentSettings()
                              && same (play (another, line, 0.8), padSound);
            const auto presetKept = fromPreset.getPadName (second) == "Clang" && fromPreset.getPadNote (second) == 60 && same (play (fromPreset, line, 0.8), padSound);

            auto older = juce::AudioProcessor::getXmlFromBinary (padState.getData(), (int) padState.getSize());
            int removed = 0;

            if (older != nullptr)
            {
                for (int i = older->getNumAttributes(); --i >= 0;)
                    if (older->getAttributeName (i).startsWith ("user"))
                    {
                        older->removeAttribute (older->getAttributeName (i));
                        ++removed;
                    }

                older->deleteAllChildElementsWithTagName ("PADS");
                older->deleteAllChildElementsWithTagName ("SAMPLES");
            }

            juce::MemoryBlock olderState;

            if (older != nullptr)
                juce::AudioProcessor::copyXmlToBinary (*older, olderState);

            BasicDrumsProcessor fromOlder;
            fromOlder.setPadName (second, "Left Over");
            fromOlder.setStateInformation (olderState.getData(), (int) olderState.getSize());
            const auto olderPlain = fromOlder.getPadName (second) == "Pad 32" && fromOlder.getPadNote (second) == 83 && fromOlder.getFiles().isEmpty()
                                    && juce::exactlyEqual (fromOlder.parameterOf (second, "tune")->get(), 0.0f);
            fromPreset.applyKit (DrumMachine::kits()[2]);
            const auto kitPlain = fromPreset.getPadName (second) == "Pad 32" && fromPreset.getPadNote (second) == 83 && fromPreset.getFiles().isEmpty() && ! fromPreset.kitModified()
                                  && juce::exactlyEqual (fromPreset.parameterOf (second, "tune")->get(), 0.0f);
            drums.clearPad (second);
            check (kept && presetKept && removed == DrumMachine::numUserPads * 15 && olderPlain && kitPlain && drums.getLayers (second).empty() && drums.getPadName (second) == "Pad 32"
                       && drums.getPadNote (second) == 83 && juce::exactlyEqual (drums.parameterOf (second, "tune")->get(), 0.0f),
                   juce::String ("drum user pads: the state and a preset bring back a pad's name, colour, note, settings and layers, and the same sound; a state with none of that in it, and a factory kit, leave every user pad as it starts; emptying a pad does the same for one (")
                       + (kept ? "" : "state ") + (presetKept ? "" : "preset ") + (olderPlain ? "" : "older ") + (kitPlain ? "" : "kit ") + ")");

            // The window: all 64 pads; a drop on a user pad fills it; its menu's choices work on it.
            BasicDrumsProcessor shown;
            shown.prepareToPlay (rate, 512);
            std::unique_ptr<juce::AudioProcessorEditor> made (shown.createEditorAndMakeActive());

            if (auto* window = dynamic_cast<DrumMachineEditor*> (made.get()))
            {
                std::set<int> places;
                auto allInside = true;

                for (int pad = 0; pad < DrumMachine::numPads; ++pad)
                {
                    const auto bounds = window->padBounds (pad);
                    allInside = allInside && ! bounds.isEmpty() && window->getLocalBounds().contains (bounds) && places.insert (bounds.getX() * 10000 + bounds.getY()).second;
                }

                const auto before = window->getPadText (first);
                const auto pad = window->padBounds (first).getCentre();
                window->filesDropped ({ aHere.getFullPathName() }, pad.x, pad.y);
                const auto dropped = window->getSelectedPad() == first && window->getTypeText() == "Sample" && window->showsLayers() && window->getNumLayersShown() == 1
                                     && shown.getLayers (first).size() == 1 && window->getSelected() == Sound::kick;
                window->changeUserPad (first, "note", 36);
                shown.setPadName (first, "Sub Layer");
                window->selectPad (first);
                const auto renamed = window->getPadText (first);
                window->changeUserPad (first, "colour", (int) 0xffe8604c);
                const auto coloured = shown.getPadColour (first) == juce::Colour (0xffe8604c);
                window->changeUserPad (first, "clear");
                window->select (Sound::snare);
                check (allInside && places.size() == 64 && before == "Pad 31 / 82 " + juce::MidiMessage::getMidiNoteName (82, true, true, 4) && dropped
                           && renamed == "Sub Layer / 36 " + juce::MidiMessage::getMidiNoteName (36, true, true, 4) && coloured
                           && window->getPadText (first) == before && shown.getLayers (first).empty() && window->getSelectedPad() == (int) Sound::snare && ! window->showsLayers(),
                       "drum user pads window: 64 pads, each in its own place inside the window; a sound dropped on pad 31 selects it and shows its layer; given another note, a name and a colour its card reads "
                           + renamed + "; emptied, it reads " + before + " again");
            }
            else
            {
                check (false, "drum user pads window: it opens");
            }

            // A sound's pad can be named and coloured too, while its type is Sample; its note stays its own.
            {
                BasicDrumsProcessor named;
                named.addFiles (Sound::clap, { aHere });
                named.setPadName ((int) Sound::clap, "Metal Hit");
                named.setPadColour ((int) Sound::clap, juce::Colour (0xff4f9be8));
                named.setPadNote ((int) Sound::clap, 90);
                const auto asSample = named.getPadName ((int) Sound::clap) == "Metal Hit" && named.getPadColour ((int) Sound::clap) == juce::Colour (0xff4f9be8)
                                      && named.getPadNote ((int) Sound::clap) == DrumMachine::padNote (Sound::clap) && named.kitModified();
                juce::MemoryBlock block;
                named.getStateInformation (block);
                BasicDrumsProcessor back;
                back.setStateInformation (block.getData(), (int) block.getSize());
                BasicDrumsProcessor viaPreset;
                viaPreset.setPresetValues (named.getPresetValues());
                viaPreset.setPresetTexts (named.getPresetTexts());
                const auto lookKept = back.getPadName ((int) Sound::clap) == "Metal Hit" && back.getPadColour ((int) Sound::clap) == juce::Colour (0xff4f9be8)
                                  && viaPreset.getPadName ((int) Sound::clap) == "Metal Hit";
                back.setType (Sound::clap, DrumMachine::classicType);
                const auto asClassic = back.getPadName ((int) Sound::clap) == "Clap" && back.getPadColour ((int) Sound::clap).isTransparent() && back.hasOwnLook ((int) Sound::clap);
                back.setType (Sound::clap, DrumMachine::sampleType);
                const auto shownAgain = back.getPadName ((int) Sound::clap) == "Metal Hit";

                named.prepareToPlay (rate, 512);
                std::unique_ptr<juce::AudioProcessorEditor> shownNamed (named.createEditorAndMakeActive());
                auto* namedWindow = dynamic_cast<DrumMachineEditor*> (shownNamed.get());
                const auto card = namedWindow != nullptr ? namedWindow->getPadText ((int) Sound::clap) : juce::String();

                if (namedWindow != nullptr)
                    namedWindow->changeUserPad ((int) Sound::clap, "clear");

                const auto cardAfter = namedWindow != nullptr ? namedWindow->getPadText ((int) Sound::clap) : juce::String();
                shownNamed = nullptr;
                viaPreset.applyKit (DrumMachine::kits()[0]);
                check (asSample && lookKept && asClassic && shownAgain && card == "Metal Hit / 39" && cardAfter == "Clap / 39" && named.getLayers (Sound::clap).size() == 1
                           && ! named.hasOwnLook ((int) Sound::clap) && ! viaPreset.hasOwnLook ((int) Sound::clap),
                       "drum pads: a sound's pad that plays samples takes a name and a colour, kept by the state and a preset, its note staying its own; as Classic again it reads Clap, as a Sample again its given name; its own name and colour come back from its menu, its layers staying, and from a factory kit ("
                           + card + ", then " + cardAfter + ")");
            }

            made = nullptr;
            there.deleteRecursively();
        }

        folder.deleteRecursively();
    }

    //==============================================================================
    // Each pad's own effects: its envelope, transient shaper, saturator, EQ and compressor.
    using DrumMachine::Fx;

    /** One pad's hit alone at full velocity, through nothing of the kit's (its bus, reverb and delay). */
    juce::AudioBuffer<float> hitOf (BuiltInEffects::BasicDrumsProcessor& d, int pad, double seconds)
    {
        return DrumMachine::Engine::renderPad (d.currentSettings(), pad, rate, seconds, d.getSamples());
    }

    float dbOf (double gain)   { return (float) (20.0 * std::log10 (juce::jmax (1.0e-9, gain))); }

    /** How much of one frequency there is in a stretch of the left side, as an amplitude, with its phase. */
    std::complex<double> partialOf (const juce::AudioBuffer<float>& b, int from, int n, double hz)
    {
        std::complex<double> sum;

        for (int i = 0; i < n; ++i)
            sum += (double) b.getSample (0, from + i) * std::polar (1.0, -juce::MathConstants<double>::twoPi * hz * (double) i / rate);

        return sum * (2.0 / (double) n);
    }

    /** Every pad's effects switched on, with settings that each do something. */
    void allEffectsOn (BuiltInEffects::BasicDrumsProcessor& d)
    {
        DrumMachine::PadEffects fx;
        fx[Fx::envOn] = 1.0f;       fx[Fx::envAttack] = 1.0f;     fx[Fx::envHold] = 80.0f;     fx[Fx::envDecay] = 600.0f;   fx[Fx::envCurve] = 30.0f;
        fx[Fx::transOn] = 1.0f;     fx[Fx::transAttack] = 40.0f;  fx[Fx::transSustain] = 20.0f;
        fx[Fx::satOn] = 1.0f;       fx[Fx::satType] = 1.0f;       fx[Fx::satAmount] = 50.0f;   fx[Fx::satMix] = 80.0f;
        fx[Fx::eqOn] = 1.0f;        fx[Fx::eqLowGain] = 4.0f;     fx[Fx::eqBell1Gain] = -5.0f; fx[Fx::eqBell2Gain] = 3.0f;  fx[Fx::eqHighGain] = -2.0f;
        fx[Fx::compOn] = 1.0f;      fx[Fx::compThreshold] = -20.0f;  fx[Fx::compGain] = 3.0f;

        for (int pad = 0; pad < DrumMachine::numPads; ++pad)
            d.setEffects (pad, fx);
    }

    void testPadEffects()
    {
        using BuiltInEffects::BasicDrumsProcessor;
        using DrumMachine::PadEffects;
        const auto folder = juce::File::createTempFile ("drumeffects");
        folder.createDirectory();

        // A steady tone, a thud that dies away, and a single tick, each on a user pad of its own.
        const auto held = tone (300.0, 28800, 0.5f);
        const auto thud = tone (110.0, 12000, 0.8f, true, 0.05);
        juce::AudioBuffer<float> tick (1, 9600);
        tick.clear();
        tick.setSample (0, 2400, 0.5f);
        const auto heldFile = writeSound (folder, "Held", held), thudFile = writeSound (folder, "Thud", thud), tickFile = writeSound (folder, "Tick", tick);
        const auto tonePad = DrumMachine::numSounds, thudPad = tonePad + 1, tickPad = tonePad + 2;

        BasicDrumsProcessor drums;
        drums.applyKit (DrumMachine::kits()[0]);
        const auto added = drums.addFiles (tonePad, { heldFile }) + drums.addFiles (thudPad, { thudFile }) + drums.addFiles (tickPad, { tickFile });
        const auto parameters = drums.getParameters().size();
        const auto set1 = [&drums] (int pad, std::initializer_list<std::pair<Fx, float>> values)
        {
            for (auto [f, v] : values)
                drums.setEffect (pad, f, v);
        };
        const auto off = [&drums] (int pad) { drums.setEffects (pad, {}); };

        Notes pattern;

        for (int step = 0; step < 16; ++step)
        {
            const auto at = step * 0.125;

            if (step % 4 == 0) pattern.push_back ({ 36, at, 1.0f });
            if (step % 8 == 4) pattern.push_back ({ 39, at, 0.9f });
            if (step == 10) pattern.push_back ({ 49, at, 0.8f });
            if (step >= 12) pattern.push_back ({ step % 2 == 0 ? 45 : 47, at, 0.8f });
            if (step % 4 == 3) pattern.push_back ({ DrumMachine::firstUserNote + 1, at, 0.7f });
            pattern.push_back ({ step % 4 == 2 ? 46 : 42, at, step % 2 == 0 ? 0.8f : 0.6f });
        }

        // Off, they are not there: every value of every pad moved, every switch left off, and a
        // pattern is the same to the last bit. Nor are they hosted parameters.
        {
            const auto before = play (drums, pattern, 2.5);

            for (int pad = 0; pad < DrumMachine::numPads; ++pad)
            {
                PadEffects moved;

                for (int f = 0; f < DrumMachine::numFx; ++f)
                    if (const auto& info = DrumMachine::fxInfo (f); info.unit[0] != 0 || f == Fx::envCurve || f == Fx::satType)
                        moved.values[(size_t) f] = info.low + 0.8f * (info.high - info.low);

                drums.setEffects (pad, moved);
            }

            const auto movedButOff = play (drums, pattern, 2.5);
            const auto changed = ! (drums.getEffects (0) == PadEffects()) && drums.kitModified();

            for (int pad = 0; pad < DrumMachine::numPads; ++pad)
                off (pad);

            // On with nothing to do (an EQ with every band at 0 dB, a saturator mixed out, a shaper at 0 and 0) is the same too.
            set1 (0, { { Fx::eqOn, 1.0f }, { Fx::satOn, 1.0f }, { Fx::satMix, 0.0f }, { Fx::transOn, 1.0f } });
            const auto idle = play (drums, pattern, 2.5);
            off (0);

            check (added.isEmpty() && same (before, movedButOff) && changed && same (before, idle) && parameters == 997 && drums.getParameters().size() == 997
                       && peakOf (before) > 0.05f,
                   "drum pad effects: with every pad's 24 settings moved and its five switches off, a pattern of sounds and a sample is bit-identical to before; "
                   "so is an EQ with every band at 0 dB, a saturator mixed out and a shaper at 0; they add no hosted parameter (997 as before)");
        }

        // The envelope: the hit times the shape drawn, on a sample and on a synthesized sound, and no longer than it.
        {
            const auto dry = hitOf (drums, tonePad, 0.7);
            juce::StringArray said;
            auto good = true;

            for (auto curve : { 0.0f, 100.0f, -100.0f })
            {
                set1 (tonePad, { { Fx::envOn, 1.0f }, { Fx::envAttack, 10.0f }, { Fx::envHold, 50.0f }, { Fx::envDecay, 100.0f }, { Fx::envCurve, curve } });
                const auto fx = drums.getEffects (tonePad);
                const auto wet = hitOf (drums, tonePad, 0.7);
                const auto length = (int) std::round (DrumMachine::envelopeSeconds (fx) * rate), tail = (int) (0.005 * rate);
                auto furthest = 0.0f;

                for (int i = 0; i < length - tail; ++i)
                    furthest = juce::jmax (furthest, std::abs (wet.getSample (0, i) - dry.getSample (0, i) * DrumMachine::envelopeGain (fx, (double) i / rate)));

                const auto middle = peakOf (wet, (int) (0.105 * rate), (int) (0.01 * rate)) / peakOf (dry, (int) (0.105 * rate), (int) (0.01 * rate));
                const auto last = lastSounding (wet);
                good = good && furthest < 2.0e-5f && last < length && last > length - 4 && std::abs (wet.getSample (0, last)) < 0.01f
                    && largestStep (wet, length - tail - 48, length + 48) < 0.03f;
                said.add ("curve " + juce::String ((int) curve) + ": " + juce::String (furthest, 7) + " away, " + juce::String (juce::roundToInt (middle * 100.0f)) + " % halfway down its fall");
            }

            off (tonePad);
            const auto kickPad = (int) Sound::kick;
            const auto kickDry = hitOf (drums, kickPad, 0.6);
            set1 (kickPad, { { Fx::envOn, 1.0f }, { Fx::envAttack, 0.0f }, { Fx::envHold, 20.0f }, { Fx::envDecay, 60.0f } });
            const auto kickFx = drums.getEffects (kickPad);
            const auto kickWet = hitOf (drums, kickPad, 0.6);
            auto kickFurthest = 0.0f;

            for (int i = 0; i < (int) (0.075 * rate); ++i)
                kickFurthest = juce::jmax (kickFurthest, std::abs (kickWet.getSample (0, i) - kickDry.getSample (0, i) * DrumMachine::envelopeGain (kickFx, (double) i / rate)));

            const auto kickLast = lastSounding (kickWet);
            off (kickPad);
            check (good && kickFurthest < 2.0e-5f && kickLast < (int) (0.08 * rate) && kickLast > (int) (0.079 * rate) && lastSounding (kickDry) > (int) (0.2 * rate),
                   "drum pad effects: the envelope (attack 10 ms, hold 50, decay 100) is the hit times the shape drawn, to its last 5 ms, and ends the hit where it ends ("
                       + said.joinIntoString ("; ") + "); the same on the synthesized kick, which stops at 80 ms with hold 20 and decay 60 (" + juce::String (kickFurthest, 7) + " away)");
        }

        // The transient shaper: Attack moves the start of a hit and leaves its tail, Sustain the other way round.
        {
            const auto dry = hitOf (drums, thudPad, 0.3);
            const auto startOf = [] (const juce::AudioBuffer<float>& b) { return peakOf (b, 0, (int) (0.004 * rate)); };
            const auto tailOf = [] (const juce::AudioBuffer<float>& b) { return b.getRMSLevel (0, (int) (0.1 * rate), (int) (0.06 * rate)); };
            const auto shapedBy = [&] (float attack, float sustain, float& start, float& tail)
            {
                set1 (thudPad, { { Fx::transOn, 1.0f }, { Fx::transAttack, attack }, { Fx::transSustain, sustain } });
                const auto wet = hitOf (drums, thudPad, 0.3);
                start = dbOf (startOf (wet) / startOf (dry));
                tail = dbOf (tailOf (wet) / tailOf (dry));
                return largestStep (wet, 0, wet.getNumSamples());
            };

            float upStart, upTail, downStart, downTail, longStart, longTail, shortStart, shortTail;
            const auto stepUp = shapedBy (100.0f, 0.0f, upStart, upTail);
            shapedBy (-100.0f, 0.0f, downStart, downTail);
            shapedBy (0.0f, 100.0f, longStart, longTail);
            shapedBy (0.0f, -100.0f, shortStart, shortTail);
            off (thudPad);
            check (upStart > 6.0f && upStart <= 12.1f && std::abs (upTail) < 1.5f && downStart < -6.0f && downStart >= -12.1f && std::abs (downTail) < 1.5f
                       && longTail > 4.0f && longTail <= 12.1f && std::abs (longStart) < 1.0f && shortTail < -4.0f && shortTail >= -12.1f && std::abs (shortStart) < 1.0f
                       && stepUp < 4.1f * largestStep (dry, 0, dry.getNumSamples()),
                   "drum pad effects: the transient shaper's Attack at +100 % lifts a thud's first 4 ms by " + juce::String (upStart, 1) + " dB and its tail by " + juce::String (upTail, 1)
                       + "; at -100 % " + juce::String (downStart, 1) + " and " + juce::String (downTail, 1) + "; Sustain at +100 % lifts the tail (100 to 160 ms) by " + juce::String (longTail, 1)
                       + " dB and the start by " + juce::String (longStart, 1) + "; at -100 % " + juce::String (shortTail, 1) + " and " + juce::String (shortStart, 1) + "; never more than 12 dB");
        }

        // The saturator: each curve is the curve drawn, sample for sample; Tube's lopsidedness leaves no offset; Mix blends.
        {
            const auto dry = hitOf (drums, tonePad, 0.5);
            const auto from = 2400, to = 19200;
            juce::StringArray said;
            auto good = true;
            std::array<double, 4> second {};

            for (int curve = 0; curve < 4; ++curve)
            {
                set1 (tonePad, { { Fx::satOn, 1.0f }, { Fx::satType, (float) curve }, { Fx::satAmount, 60.0f }, { Fx::satMix, 100.0f } });
                const auto fx = drums.getEffects (tonePad);
                const auto wet = hitOf (drums, tonePad, 0.5);
                auto furthest = 0.0f;
                auto mean = 0.0, curveMean = 0.0;

                for (int i = from; i < to; ++i)
                {
                    const auto want = DrumMachine::saturated (fx, dry.getSample (0, i));
                    furthest = juce::jmax (furthest, std::abs (wet.getSample (0, i) - want));
                    mean += wet.getSample (0, i) / (double) (to - from);
                    curveMean += want / (double) (to - from);
                }

                second[(size_t) curve] = std::abs (partialOf (wet, from, to - from, 600.0));

                // Tube's curve is lopsided, so it has an offset, which is taken off: what is left is the curve less its mean.
                good = good && (curve == 1 ? std::abs (mean) < 0.004 && std::abs (curveMean) > 0.02 && second[1] > 0.01
                                           : furthest < 2.0e-5f && second[(size_t) curve] < 1.0e-4);
                said.add (DrumMachine::saturationNames()[curve] + (curve == 1 ? " offset " + juce::String (mean, 4) + " (its curve's " + juce::String (curveMean, 3) + "), second harmonic " + juce::String (second[1], 3)
                                                                            : " " + juce::String (furthest, 7) + " away"));
            }

            set1 (tonePad, { { Fx::satType, 2.0f }, { Fx::satAmount, 100.0f } });
            const auto clipped = hitOf (drums, tonePad, 0.5);
            set1 (tonePad, { { Fx::satMix, 50.0f } });
            const auto half = hitOf (drums, tonePad, 0.5);
            auto halfFurthest = 0.0f;

            for (int i = from; i < to; ++i)
                halfFurthest = juce::jmax (halfFurthest, std::abs (half.getSample (0, i) - 0.5f * (dry.getSample (0, i) + clipped.getSample (0, i))));

            // Then the EQ and the compressor come after it: a clipped tone is no longer square, nor at full scale.
            set1 (tonePad, { { Fx::satMix, 100.0f }, { Fx::eqOn, 1.0f }, { Fx::eqHighFreq, 1000.0f }, { Fx::eqHighGain, -18.0f } });
            const auto fifth = std::abs (partialOf (hitOf (drums, tonePad, 0.5), from, to - from, 1500.0)) / std::abs (partialOf (clipped, from, to - from, 1500.0));
            set1 (tonePad, { { Fx::eqOn, 0.0f }, { Fx::compOn, 1.0f }, { Fx::compThreshold, -30.0f }, { Fx::compRatio, 20.0f }, { Fx::compAttack, 0.1f } });
            const auto held1 = peakOf (hitOf (drums, tonePad, 0.5), from, to - from);
            off (tonePad);
            check (good && std::abs (peakOf (clipped, from, to - from) - 1.0f) < 1.0e-6f && halfFurthest < 1.0e-6f && fifth < 0.3 && held1 < 0.1f,
                   "drum pad effects: the saturator's curves at 60 % on a steady tone: " + said.joinIntoString ("; ") + "; Clip at 100 % holds it at full scale (" + juce::String (peakOf (clipped, from, to - from), 7) + "); Mix at 50 % is halfway ("
                       + juce::String (halfFurthest, 8) + " away); the EQ after it (a high shelf, -18 dB from 1 kHz) takes the clipped tone's fifth harmonic to " + juce::String (juce::roundToInt (fifth * 100.0))
                       + " %, and the compressor after it holds the square to " + juce::String (dbOf (held1), 1) + " dB");
        }

        // The EQ: a tick through it measures as the curve drawn.
        {
            const auto dry = hitOf (drums, tickPad, 0.25);
            set1 (tickPad, { { Fx::eqOn, 1.0f }, { Fx::eqLowFreq, 100.0f }, { Fx::eqLowGain, 6.0f }, { Fx::eqBell1Freq, 500.0f }, { Fx::eqBell1Gain, -9.0f }, { Fx::eqBell1Width, 1.0f },
                             { Fx::eqBell2Freq, 3000.0f }, { Fx::eqBell2Gain, 4.0f }, { Fx::eqBell2Width, 0.5f }, { Fx::eqHighFreq, 8000.0f }, { Fx::eqHighGain, -6.0f } });
            const auto fx = drums.getEffects (tickPad);
            const auto wet = hitOf (drums, tickPad, 0.25);
            const auto n = (int) (0.2 * rate);
            auto furthest = 0.0f;
            juce::StringArray said;

            for (auto hz : { 30.0, 100.0, 300.0, 500.0, 1000.0, 3000.0, 8000.0, 15000.0 })
            {
                const auto measured = dbOf (std::abs (partialOf (wet, 0, n, hz)) / std::abs (partialOf (dry, 0, n, hz)));
                furthest = juce::jmax (furthest, std::abs (measured - DrumMachine::eqGainDb (fx, hz, rate)));
                said.add (juce::String (juce::roundToInt (hz)) + " Hz " + juce::String (measured, 1));
            }

            // And the curve is what its settings say: a bell alone at its centre and its width's edges, a shelf far from its corner and at it.
            PadEffects bell, low, high;
            bell[Fx::eqBell1Freq] = 500.0f;  bell[Fx::eqBell1Gain] = -9.0f;  bell[Fx::eqBell1Width] = 1.0f;
            low[Fx::eqLowFreq] = 100.0f;     low[Fx::eqLowGain] = 6.0f;
            high[Fx::eqHighFreq] = 8000.0f;  high[Fx::eqHighGain] = -6.0f;
            const auto g = [] (const PadEffects& e, double hz) { return DrumMachine::eqGainDb (e, hz, rate); };
            const auto asSet = std::abs (g (bell, 500.0) + 9.0f) < 0.01f && std::abs (g (bell, 500.0 * std::sqrt (2.0)) + 4.5f) < 0.3f && std::abs (g (bell, 500.0 / std::sqrt (2.0)) + 4.5f) < 0.3f
                            && std::abs (g (bell, 50.0)) < 0.3f && std::abs (g (bell, 5000.0)) < 0.3f
                            && std::abs (g (low, 10.0) - 6.0f) < 0.1f && std::abs (g (low, 100.0) - 3.0f) < 0.1f && std::abs (g (low, 2000.0)) < 0.1f
                            && std::abs (g (high, 20000.0) + 6.0f) < 0.2f && std::abs (g (high, 8000.0) + 3.0f) < 0.2f && std::abs (g (high, 400.0)) < 0.1f;
            off (tickPad);
            check (furthest < 0.05f && asSet,
                   "drum pad effects: the EQ (low shelf +6 dB at 100 Hz, bells -9 at 500 and +4 at 3 kHz, high shelf -6 at 8 kHz) measures as its curve, at most "
                       + juce::String (furthest, 3) + " dB away (" + said.joinIntoString (", ") + " dB); a bell is its gain at its centre and half of it an octave apart, a shelf its gain far out and half at its corner");
        }

        // The compressor: a steady tone settles where its curve says, the gain is added after, and the window's meter says how far.
        {
            const auto dry = hitOf (drums, tonePad, 0.55);
            const auto from = 14400, n = 9600;
            set1 (tonePad, { { Fx::compOn, 1.0f }, { Fx::compThreshold, -18.0f }, { Fx::compRatio, 4.0f }, { Fx::compAttack, 20.0f }, { Fx::compRelease, 100.0f }, { Fx::compGain, 0.0f } });
            const auto fx = drums.getEffects (tonePad);
            const auto wet = hitOf (drums, tonePad, 0.55);
            const auto in = dbOf (peakOf (dry, from, n)), out = dbOf (peakOf (wet, from, n)), want = DrumMachine::compressedDb (fx, in);
            const auto onset = dbOf (peakOf (wet, 0, (int) (0.005 * rate)));
            set1 (tonePad, { { Fx::compGain, 6.0f } });
            const auto louder = dbOf (peakOf (hitOf (drums, tonePad, 0.55), from, n));
            set1 (tonePad, { { Fx::compGain, 0.0f }, { Fx::compRatio, 1.0f } });
            const auto one = dbOf (peakOf (hitOf (drums, tonePad, 0.55), from, n));
            set1 (tonePad, { { Fx::compRatio, 4.0f } });
            play (drums, { { DrumMachine::firstUserNote, 0.0, 1.0f } }, 0.4);
            const auto meter = drums.getEngine().padReductionDb (tonePad), other = drums.getEngine().padReductionDb (thudPad);
            off (tonePad);
            check (std::abs (want - (in - 9.0f)) < 0.05f && std::abs (out - want) < 0.5f && onset > out + 6.0f && std::abs (louder - out - 6.0f) < 0.01f && std::abs (one - in) < 0.01f
                       && std::abs (meter + 9.0f) < 0.5f && juce::exactlyEqual (other, 0.0f),
                   "drum pad effects: the compressor (threshold -18 dB, 4:1) settles a " + juce::String (in, 1) + " dB tone at " + juce::String (out, 1) + " dB (its curve says "
                       + juce::String (want, 1) + "), its first 5 ms through a 20 ms attack reaching " + juce::String (onset, 1) + "; Gain +6 dB adds 6.0; at 1:1 it does nothing; the pad's meter reads "
                       + juce::String (meter, 1) + " dB, another pad's 0");
        }

        // A change is heard from the next hit: the one sounding goes on as it was, to the last bit, so no switch can click.
        {
            const auto run = [&drums] (bool change)
            {
                drums.prepareToPlay (rate, 2400);
                juce::AudioBuffer<float> out (2, 9600), block (2, 2400);

                for (int at = 0; at < 9600; at += 2400)
                {
                    juce::MidiBuffer midi;

                    if (at == 0)
                        midi.addEvent (juce::MidiMessage::noteOn (10, 49, 1.0f), 0);

                    if (at == 4800)
                        midi.addEvent (juce::MidiMessage::noteOn (10, 38, 1.0f), 0);

                    if (change && at == 2400)
                        allEffectsOn (drums);

                    drums.processBlock (block, midi);

                    for (int ch = 0; ch < 2; ++ch)
                        out.copyFrom (ch, at, block, ch, 0, 2400);
                }

                return out;
            };

            const auto steady = run (false), changed = run (true);
            auto sameUntil = 0;

            while (sameUntil < 9600 && juce::exactlyEqual (steady.getSample (0, sameUntil), changed.getSample (0, sameUntil)) && juce::exactlyEqual (steady.getSample (1, sameUntil), changed.getSample (1, sameUntil)))
                ++sameUntil;

            for (int pad = 0; pad < DrumMachine::numPads; ++pad)
                off (pad);

            check (sameUntil >= 4800 && sameUntil < 4800 + 48,
                   "drum pad effects: every effect switched on 50 ms into a crash leaves the crash as it was, bit for bit; the snare struck 50 ms later is the first to have them (the two differ from sample "
                       + juce::String (sameUntil) + ")");
        }

        // With every effect on on every pad: no clicks, and the same at any block size.
        {
            BasicDrumsProcessor busy;
            busy.applyKit (DrumMachine::kits()[0]);
            allEffectsOn (busy);
            checkNoClicks (busy, "drums (every pad's five effects on): ");

            drums.applyKit (DrumMachine::kits()[(size_t) DrumMachine::kitIndex ("Industrial")]);
            allEffectsOn (drums);
            const auto first = play (drums, pattern, 2.5), again = play (drums, pattern, 2.5), odd = play (drums, pattern, 2.5, 37), one = play (drums, pattern, 2.5, 1);
            check (same (first, again) && same (first, odd) && same (first, one) && peakOf (first) > 0.05f,
                   "drum pad effects: with all five on on every pad, a pattern renders bit-identically twice, in 37-sample blocks and sample by sample");

            // What they cost: a busy bar, 12 notes on every sixteenth, with them off and with them all on.
            Notes dense;

            for (int step = 0; step < 64; ++step)
                for (int k = 0; k < 12; ++k)
                    dense.push_back ({ DrumMachine::firstNote + (step * 5 + k * 4) % DrumMachine::numNotes, step * 0.125, 0.8f });

            const auto timed = [&] (BasicDrumsProcessor& d, int& sounding)
            {
                auto best = 1.0e9;

                for (int pass = 0; pass < 3; ++pass)
                {
                    const auto began = juce::Time::getMillisecondCounterHiRes();
                    play (d, dense, 8.0);
                    best = juce::jmin (best, juce::Time::getMillisecondCounterHiRes() - began);
                }

                play (d, dense, 7.9);
                sounding = d.getNumSounding();
                return best;
            };

            BasicDrumsProcessor plain;
            plain.applyKit (DrumMachine::kits()[(size_t) DrumMachine::kitIndex ("Industrial")]);
            int plainSounding = 0, busySounding = 0;
            const auto without = timed (plain, plainSounding), with = timed (drums, busySounding);
            check (with < 2000.0,
                   "drum pad effects: 8 seconds of 96 notes a second (" + juce::String (plainSounding) + " hits sounding at the end with them off, " + juce::String (busySounding)
                       + " with them on) render in " + juce::String (juce::roundToInt (without)) + " ms with every pad's effects off and " + juce::String (juce::roundToInt (with)) + " ms with all five on on every pad: "
                       + juce::String (without / 80.0, 1) + " % and " + juce::String (with / 80.0, 1) + " % of real time on one core");
        }

        // Kept: by the state, a preset and a kit file; a state or preset from before has none; a factory kit takes them off.
        {
            drums.applyKit (DrumMachine::kits()[(size_t) DrumMachine::kitIndex ("Industrial")]);
            const auto unedited = ! drums.kitModified();
            juce::MemoryBlock before;
            drums.getStateInformation (before);
            const auto plainXml = juce::AudioProcessor::getXmlFromBinary (before.getData(), (int) before.getSize());

            set1 ((int) Sound::kick, { { Fx::compOn, 1.0f }, { Fx::compRatio, 500.0f }, { Fx::eqOn, 1.0f }, { Fx::eqBell1Gain, 7.5f } });
            set1 (thudPad, { { Fx::satOn, 1.0f }, { Fx::satType, 3.0f }, { Fx::envOn, 1.0f }, { Fx::envDecay, 90.0f } });
            const auto limited = juce::exactlyEqual (drums.getEffect ((int) Sound::kick, Fx::compRatio), 20.0f);
            const auto edited = drums.kitModified();
            const auto sound = play (drums, pattern, 2.5);

            juce::MemoryBlock state;
            drums.getStateInformation (state);
            const auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), (int) state.getSize());
            const auto* kept = xml != nullptr ? xml->getChildByName ("PADEFFECTS") : nullptr;
            BasicDrumsProcessor another;
            another.setStateInformation (state.getData(), (int) state.getSize());
            const auto fromState = another.currentSettings() == drums.currentSettings() && same (play (another, pattern, 2.5), sound);

            // The state from before they were set, loaded over them: they are off again.
            another.setStateInformation (before.getData(), (int) before.getSize());
            const auto oldState = another.getEffects ((int) Sound::kick) == PadEffects() && another.getEffects (thudPad) == PadEffects() && ! another.kitModified();

            BasicDrumsProcessor viaPreset;
            viaPreset.setPresetTexts (drums.getPresetTexts());
            const auto fromPreset = viaPreset.getEffects ((int) Sound::kick) == drums.getEffects ((int) Sound::kick) && viaPreset.getEffects (thudPad) == drums.getEffects (thudPad);
            viaPreset.setPresetTexts ({});
            const auto oldPreset = viaPreset.getEffects ((int) Sound::kick) == PadEffects() && viaPreset.getEffects (thudPad) == PadEffects();

            const auto kitFolder = folder.getChildFile ("Effects Kit");
            juce::StringArray notThere;
            const auto saveError = drums.saveKitWithSamples (kitFolder, notThere);
            BasicDrumsProcessor opened;
            const auto openError = opened.loadKitFile (kitFolder.getChildFile (kitFolder.getFileName() + BasicDrumsProcessor::kitFileExtension));
            const auto fromKit = opened.getEffects ((int) Sound::kick) == drums.getEffects ((int) Sound::kick) && opened.getEffects (thudPad) == drums.getEffects (thudPad);

            drums.clearPad (thudPad);
            const auto cleared = drums.getEffects (thudPad) == PadEffects();
            drums.applyKit (DrumMachine::kits()[0]);
            const auto factory = drums.getEffects ((int) Sound::kick) == PadEffects() && ! drums.kitModified();

            check (unedited && plainXml != nullptr && plainXml->getChildByName ("PADEFFECTS") == nullptr && limited && edited && kept != nullptr && kept->getNumChildElements() == 2
                       && fromState && oldState && fromPreset && oldPreset && saveError.isEmpty() && openError.isEmpty() && fromKit && cleared && factory,
                   "drum pad effects: a kit that uses none says nothing of them in its state; set on two pads they mark the kit edited and are kept by the state (those two pads alone), a preset and a kit file, "
                   "sounding the same; a value set outside its range is brought inside it; a state or preset from before them takes them off; so does emptying the pad and choosing a factory kit");
        }

        // The window: the Effects part of a pad's panel, a page for each.
        {
            BasicDrumsProcessor shown;
            shown.applyKit (DrumMachine::kits()[0]);
            shown.addFiles (thudPad, { thudFile });
            std::unique_ptr<juce::AudioProcessorEditor> made (shown.createEditor());
            auto* window = dynamic_cast<DrumMachineEditor*> (made.get());
            check (window != nullptr, "drum pad effects: the window opens");

            if (window != nullptr)
            {
                auto& view = window->getEffectsView();
                const auto kickPad = (int) Sound::kick;
                window->select (Sound::kick);
                const auto pages = DrumPadEffectsView::pageNames().joinIntoString (", ");
                auto noneLit = true;

                for (int p = 0; p < DrumPadEffectsView::numPages; ++p)
                    noneLit = noneLit && ! view.isLit (p);

                const auto first = view.getPage() == DrumPadEffectsView::filterPage && window->showsFilterControls() && window->getKnobText ("Cutoff").isNotEmpty();
                shown.filterParameter (kickPad)->setValueNotifyingHost (shown.filterParameter (kickPad)->convertTo0to1 (1.0f));
                window->select (Sound::kick);
                const auto filterLit = view.isLit (DrumPadEffectsView::filterPage);

                // The envelope's page: its switch and knobs set the pad's values, and read them.
                view.setPage (DrumPadEffectsView::envPage);
                const auto envLabels = view.getKnobLabels().joinIntoString (", ");
                const auto hidden = ! window->showsFilterControls() && ! view.isOn();
                view.clickSwitch();
                const auto turned = view.turnKnob ("Decay", 120.0f) && view.turnKnob ("Curve", -30.0f);
                const auto env = shown.getEffects (kickPad);
                const auto envSet = env.on (Fx::envOn) && std::abs (env[Fx::envDecay] - 120.0f) < 0.5f && std::abs (env[Fx::envCurve] + 30.0f) < 0.5f && view.isLit (DrumPadEffectsView::envPage)
                                 && view.getKnobText ("Decay") == "120 ms" && view.getKnobText ("Curve") == "-30" && shown.kitModified();

                // Another pad has its own; back on the first, its are there again.
                window->select (Sound::snare);
                const auto others = view.getPage() == DrumPadEffectsView::envPage && ! view.isOn() && ! view.isLit (DrumPadEffectsView::envPage) && view.getKnobText ("Decay") == "300 ms";
                window->select (Sound::kick);
                const auto back = view.isOn() && view.getKnobText ("Decay") == "120 ms";

                // The saturator's curve by its selector; the EQ's bands by a drag of a point or the selector.
                view.setPage (DrumPadEffectsView::satPage);
                view.stepSelector (1);
                const auto tube = view.getSelectorText() == "Tube" && juce::exactlyEqual (shown.getEffect (kickPad, Fx::satType), 1.0f) && view.getKnobLabels().joinIntoString (", ") == "Amount, Mix";
                view.setPage (DrumPadEffectsView::eqPage);
                view.dragPoint (2, 1000.0, -6.0f);
                const auto dragged = view.getSelectorText() == "3 Bell" && std::abs (shown.getEffect (kickPad, Fx::eqBell2Freq) - 1000.0f) < 0.5f
                                  && juce::exactlyEqual (shown.getEffect (kickPad, Fx::eqBell2Gain), -6.0f) && view.getKnobText ("Gain") == "-6.0 dB" && view.getKnobLabels().joinIntoString (", ") == "Freq, Gain, Width";
                view.stepSelector (-2);
                const auto shelf = view.getSelectorText() == "1 Low" && view.getKnobLabels().joinIntoString (", ") == "Freq, Gain" && view.getKnobText ("Freq") == "100 Hz";

                // The transient shaper's two, and the compressor's five with its meter after a hit.
                view.setPage (DrumPadEffectsView::transPage);
                const auto shaper = view.getKnobLabels().joinIntoString (", ") == "Attack, Sustain" && view.turnKnob ("Attack", 50.0f) && view.getKnobText ("Attack") == "+50 %";
                view.setPage (DrumPadEffectsView::compPage);
                view.clickSwitch();
                view.turnKnob ("Thresh", -30.0f);
                const auto still = view.getMeterText();
                play (shown, { { 36, 0.0, 1.0f } }, 0.3);
                view.refresh();
                const auto meter = view.getMeterText();

                // Changed from elsewhere (a preset, a kit), the page follows at the window's next look.
                shown.setEffect (kickPad, Fx::compRatio, 8.0f);
                view.refresh();
                const auto followed = view.getKnobText ("Ratio") == "8.0:1";
                shown.applyKit (DrumMachine::kits()[0]);
                view.refresh();
                const auto cleared = ! view.isOn() && ! view.isLit (DrumPadEffectsView::envPage) && view.getKnobText ("Ratio") == "4.0:1";
                view.setPage (DrumPadEffectsView::filterPage);

                check (pages == "FILTER, ENV, TRANSIENT, SATURATE, EQ, COMP" && noneLit && first && filterLit && envLabels == "Attack, Hold, Decay, Curve" && hidden && turned && envSet
                           && others && back && tube && dragged && shelf && shaper && still == "0.0 dB" && meter != still && meter.getFloatValue() < -3.0f && followed && cleared
                           && window->showsFilterControls(),
                       "drum pad effects: the window's Effects are six pages (" + pages + "), the first the filter, crusher and sends as before, its tab's dot lit with the filter on; "
                       "a page's switch and knobs set the selected pad's values and read them (Decay 120 ms, Curve -30), and its dot lights; another pad shows its own; the saturator's "
                       "selector chooses its curve, an EQ point dragged sets its band (3 Bell at 1 kHz, -6.0 dB) and a shelf has no Width; the compressor's meter reads "
                       + meter + " after a kick; a change from elsewhere and a factory kit show at the next look");
            }
        }

        folder.deleteRecursively();
    }

    //==============================================================================
    // The kit as the piano roll's drum grid lists it.
    void testDrumRows()
    {
        using BuiltInEffects::BasicDrumsProcessor;
        const auto folder = juce::File::createTempFile ("drumrows");
        folder.createDirectory();
        const auto file = writeSound (folder, "Hit", tone (200.0, 2400, 0.5f));
        const auto user = DrumMachine::numSounds;
        const auto nameAt = [] (const std::vector<DrumSound>& rows, int pitch)
        {
            juce::StringArray names;

            for (auto& r : rows)
                if (r.pitch == pitch)
                    names.add (r.name);

            return names.joinIntoString ("|");
        };

        BasicDrumsProcessor drums;
        const auto plain = drums.drumSounds() == DrumSound::generalMidi();

        // Names given to pads that do not play samples, and user pads with none, change nothing.
        drums.setPadName ((int) Sound::snare, "Metal Hit");
        drums.setPadName (user + 2, "Empty");
        const auto stillPlain = drums.drumSounds() == DrumSound::generalMidi();

        drums.addFiles (Sound::snare, { file });
        drums.addFiles (user, { file });
        drums.setPadName (user, "Zap");
        drums.addFiles (user + 1, { file });
        drums.setPadName (user + 1, "Kick Layer");
        drums.setPadNote (user + 1, 36);
        drums.addFiles (user + 3, { file });
        drums.addFiles (user + 4, { file });
        drums.setPadNote (user + 3, 100);
        drums.setPadNote (user + 4, 100);
        const auto rows = drums.drumSounds();
        const auto gm = DrumSound::generalMidi();
        const auto listed = rows.size() == 49 && nameAt (rows, 38) == "Metal Hit" && nameAt (rows, 40) == "Metal Hit" && nameAt (rows, 36) == nameAt (gm, 36) + " + Kick Layer"
                         && nameAt (rows, 82) == "Zap" && nameAt (rows, 100) == "Pad 34 + Pad 35" && rows[47].pitch == 82 && rows[48].pitch == 100
                         && nameAt (rows, 42) == nameAt (gm, 42) && nameAt (rows, 84).isEmpty();
        drums.setType (Sound::snare, DrumMachine::classicType);
        const auto classicAgain = nameAt (drums.drumSounds(), 38) == nameAt (gm, 38);

        // In the grid: its rows are those; a user pad's row made to stand for the snare takes a General MIDI pattern's snares.
        NoteEditor::Shown shown;
        shown.clip.id = 3;
        shown.clip.length = 96000;
        shown.clip.notes.push_back ({ 38, 24000, 6000, 0.8f });
        shown.clip.notes.push_back ({ 36, 0, 6000, 0.8f });
        shown.sampleRate = rate;
        shown.beatSeconds = 0.5;
        shown.drums = true;
        shown.drumMap = drums.drumSounds();
        NoteEditor editor;
        editor.readClip = [&shown] { return std::optional<NoteEditor::Shown> (shown); };
        editor.apply = [&shown] (const NoteClip& c, const juce::String&) { shown.clip = c; };
        editor.applyDrumMap = [&shown] (const std::vector<DrumSound>& map, const juce::String&) { shown.drumMap = map.empty() ? DrumSound::generalMidi() : map; };
        editor.setBounds (0, 0, 1000, 600);
        editor.refresh();
        const auto inGrid = editor.drumRows() == drums.drumSounds() && nameAt (editor.drumRows(), 82) == "Zap";
        auto map = shown.drumMap;

        for (auto& r : map)
            if (r.pitch == 82)
                r.standsFor = 38;

        editor.setDrumMap (map, "Edit Drum Sound");
        editor.refresh();
        const auto moved = editor.remapNotes (true);
        auto snareNow = -1, kickNow = -1;

        for (auto& n : shown.clip.notes)
            (n.start == 0 ? kickNow : snareNow) = n.pitch;

        check (plain && stillPlain && listed && classicAgain && inGrid && moved == 1 && snareNow == 82 && kickNow == 36,
               "drum pads: with no samples the kit lists as General MIDI's map exactly; a sound's pad that plays samples lists under its given name (Metal Hit on 38 and 40), a user pad with samples "
               "gets a row on its note under its name (Zap on 82), one on a sound's note joins that row (" + nameAt (rows, 36) + "), two on one note share a row (" + nameAt (rows, 100)
                   + "), a user pad with no samples has none; in the drum grid those are the rows, and a user pad's row made to stand for the snare takes a pattern's snares with Remap Notes");
        folder.deleteRecursively();
    }

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
            if (const auto sound = *DrumMachine::soundFor (note); DrumMachine::hasTypes (sound))
            {
                drums.setType (sound, DrumMachine::modernType);
                const auto modern = play (drums, { { note, 0.0, 1.0f } }, 0.5);
                drums.setType (sound, DrumMachine::classicType);
                std::cout << "; Modern peak " << juce::Decibels::gainToDecibels (peakOf (modern)) << " dB ("
                          << juce::Decibels::gainToDecibels (peakOf (modern)) - juce::Decibels::gainToDecibels (peakOf (out)) << " dB against it)";
            }

            std::cout << std::endl;
        }
    }
}

void saveDrumMachineWindow (const juce::File& folder)
{
    folder.createDirectory();
    BuiltInEffects::BasicDrumsProcessor drums;
    drums.prepareToPlay (rate, 512);
    drums.applyKit (DrumMachine::kits()[(size_t) DrumMachine::kitIndex ("Warehouse")]);
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

    // And with a pad that plays samples selected: three layers on the snare, made here, spread across the velocities.
    const auto sounds = juce::File::createTempFile ("drumwindowsounds");
    sounds.createDirectory();
    juce::Array<juce::File> hits;
    int n = 0;

    for (auto name : { "Snare Soft", "Snare Medium", "Snare Hard" })
    {
        juce::Random random (17 + n);
        juce::AudioBuffer<float> hit (1, 9600 + 2400 * n);

        for (int i = 0; i < hit.getNumSamples(); ++i)
            hit.setSample (0, i, (0.3f + 0.2f * (float) n) * (float) std::exp (-(double) i / (1500.0 + 500.0 * n))
                                     * (0.6f * (random.nextFloat() * 2.0f - 1.0f) + 0.4f * (float) std::sin (0.024 * i)));

        hits.add (writeSound (sounds, name, hit));
        ++n;
    }

    if (auto* drumWindow = dynamic_cast<DrumMachineEditor*> (window.get()))
    {
        // A user pad too, with a name and a colour of its own, on the kick's note.
        drums.addFiles (DrumMachine::numSounds, { hits[2] });
        drums.setPadName (DrumMachine::numSounds, "Kick Layer");
        drums.setPadColour (DrumMachine::numSounds, juce::Colour (0xffe8604c));
        drums.setPadNote (DrumMachine::numSounds, 36);
        drumWindow->addFiles (Sound::snare, hits);
        drumWindow->pressLayerButton ("spread");
        drumWindow->selectLayer (1);

        // A hit on it, so the layers say its velocity and the one that played is lit.
        {
            drums.auditionPad ((int) Sound::snare, 100.0f / 127.0f);
            juce::AudioBuffer<float> block (2, 512);
            juce::MidiBuffer midi;
            drums.processBlock (block, midi);
            drumWindow->showHits();
        }

        for (auto theme : { "Dark", "Light" })
        {
            Theme::apply (Theme::make (theme, "Blue"));
            const auto image = window->createComponentSnapshot (window->getLocalBounds(), true, 2.0f);
            auto file = folder.getChildFile (juce::String ("drums-sample-") + theme + ".png");
            file.deleteFile();
            juce::FileOutputStream out (file);
            juce::PNGImageFormat().writeImageToStream (image, out);
            std::cout << file.getFullPathName() << std::endl;
        }

        // Each page of the pad's effects, every one switched on, after a hit (for the compressor's meter).
        const auto snare = (int) Sound::snare;
        using DrumMachine::Fx;

        for (auto [f, v] : std::initializer_list<std::pair<Fx, float>> {
                 { Fx::envOn, 1.0f }, { Fx::envAttack, 2.0f }, { Fx::envHold, 40.0f }, { Fx::envDecay, 180.0f }, { Fx::envCurve, 40.0f },
                 { Fx::transOn, 1.0f }, { Fx::transAttack, 60.0f }, { Fx::transSustain, -40.0f },
                 { Fx::satOn, 1.0f }, { Fx::satType, 1.0f }, { Fx::satAmount, 55.0f },
                 { Fx::eqOn, 1.0f }, { Fx::eqLowFreq, 90.0f }, { Fx::eqLowGain, -4.0f }, { Fx::eqBell1Freq, 220.0f }, { Fx::eqBell1Gain, 5.0f },
                 { Fx::eqBell2Freq, 1800.0f }, { Fx::eqBell2Gain, -6.0f }, { Fx::eqBell2Width, 0.6f }, { Fx::eqHighFreq, 7000.0f }, { Fx::eqHighGain, 3.0f },
                 { Fx::compOn, 1.0f }, { Fx::compThreshold, -24.0f }, { Fx::compGain, 4.0f } })
            drums.setEffect (snare, f, v);

        drums.auditionPad (snare, 100.0f / 127.0f);
        juce::AudioBuffer<float> block (2, 4800);
        juce::MidiBuffer midi;
        drums.processBlock (block, midi);
        Theme::apply (Theme::make ("Dark", "Blue"));
        auto& effects = drumWindow->getEffectsView();

        for (int page = 0; page < DrumPadEffectsView::numPages; ++page)
        {
            effects.setPage (page);
            effects.refresh();
            const auto image = window->createComponentSnapshot (window->getLocalBounds(), true, 2.0f);
            auto file = folder.getChildFile ("drums-effects-" + DrumPadEffectsView::pageNames()[page].toLowerCase() + ".png");
            file.deleteFile();
            juce::FileOutputStream out (file);
            juce::PNGImageFormat().writeImageToStream (image, out);
            std::cout << file.getFullPathName() << std::endl;
        }
    }

    window = nullptr;
    sounds.deleteRecursively();
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

            if (DrumMachine::hasTypes (sound))
            {
                drums.setType (sound, DrumMachine::modernType);
                write ("Modern " + juce::String (note) + " " + DrumMachine::nameOf (sound) + " v" + juce::String (velocity),
                       play (drums, { { note, 0.0, (float) velocity / 127.0f } }, seconds));
                drums.setType (sound, DrumMachine::classicType);
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
    testSamples();
    testPadEffects();
    testDrumRows();
}
