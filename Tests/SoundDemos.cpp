#include <JuceHeader.h>
#include <map>
#include <numeric>
#include <regex>
#include <set>
#include "../Source/AudioEdits.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/ClickRepair.h"
#include "../Source/DrumMachine.h"
#include "../Source/EffectPresets.h"
#include "../Source/InstrumentKeys.h"
#include "../Source/LossyFiles.h"
#include "../Source/Loudness.h"
#include "../Source/Maximizer.h"
#include "../Source/NoiseReduction.h"
#include "../Source/Restoration.h"
#include "../Source/Sampler.h"
#include "../Source/SequenceLibrary.h"
#include "../Source/SequencePreview.h"
#include "../Source/SpectralRecovery.h"
#include "../Source/Spectrogram.h"
#include "../Source/SwarmGuitar.h"
#include "../Source/WaveSynth.h"
#include "DrumMachineTests.h"
#include "SoundDemos.h"

namespace
{
    constexpr double rate = 48000.0, bpm = 120.0, beat = 60.0 / bpm, bar = beat * 4.0;

    struct Note { double at, length; int pitch; float velocity; };
    using Part = std::vector<Note>;

    /** The session a demo plays in: at its tempo, playing, from the start. */
    struct Session final : juce::AudioPlayHead
    {
        explicit Session (double tempoToUse) : tempo (tempoToUse) {}

        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo p;
            p.setBpm (tempo);
            p.setIsPlaying (true);
            return p;
        }

        double tempo;
    };

    /** A note in seconds. */
    struct Hit { double at, length; int pitch; float velocity; };

    /** Plays the notes through the instrument in a session at the tempo, for so many seconds, in blocks of 512. */
    juce::AudioBuffer<float> play (juce::AudioProcessor& instrument, const std::vector<Hit>& hits, double seconds, double tempo)
    {
        Session session (tempo);
        instrument.setPlayHead (&session);
        instrument.setPlayConfigDetails (0, 2, rate, 512);
        instrument.setNonRealtime (true);
        instrument.prepareToPlay (rate, 512);

        struct Event { int frame; juce::MidiMessage message; };
        std::vector<Event> events;

        for (auto& n : hits)
        {
            events.push_back ({ (int) std::lround (n.at * rate), juce::MidiMessage::noteOn (1, n.pitch, n.velocity) });
            events.push_back ({ (int) std::lround ((n.at + n.length) * rate) - 32, juce::MidiMessage::noteOff (1, n.pitch) });
        }

        std::stable_sort (events.begin(), events.end(), [] (const Event& a, const Event& b)
        {
            return a.frame < b.frame || (a.frame == b.frame && a.message.isNoteOff() && ! b.message.isNoteOff());
        });

        const auto total = (int) (seconds * rate);
        juce::AudioBuffer<float> out (2, total), block (2, 512);
        size_t next = 0;

        for (int at = 0; at < total; at += 512)
        {
            const auto n = juce::jmin (512, total - at);
            juce::MidiBuffer midi;

            for (; next < events.size() && events[next].frame < at + n; ++next)
                midi.addEvent (events[next].message, juce::jmax (0, events[next].frame - at));

            block.setSize (2, n, false, false, true);
            block.clear();
            instrument.processBlock (block, midi);

            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, at, block, ch, 0, n);
        }

        instrument.setPlayHead (nullptr);
        return out;
    }

    /** A part written in beats, at 120 BPM. */
    juce::AudioBuffer<float> play (juce::AudioProcessor& instrument, const Part& part, double seconds)
    {
        std::vector<Hit> hits;

        for (auto& n : part)
            hits.push_back ({ n.at * beat, n.length * beat, n.pitch, n.velocity });

        return play (instrument, hits, seconds, bpm);
    }

    /** A note every step of a pattern: its pitches in turn, the first of each beat accented. */
    Part steps (const std::vector<int>& pitches, double step, int count, double hold = 0.8)
    {
        Part p;

        for (int i = 0; i < count; ++i)
            p.push_back ({ i * step, step * hold, pitches[(size_t) i % pitches.size()], std::fmod (i * step, 1.0) < 1.0e-6 ? 1.0f : 0.78f });

        return p;
    }

    Part chords (const std::vector<std::vector<int>>& each, double length)
    {
        Part p;

        for (size_t i = 0; i < each.size(); ++i)
            for (auto pitch : each[i])
                p.push_back ({ (double) i * length, length * 0.95, pitch, 0.9f });

        return p;
    }

    Part line (const std::vector<std::pair<int, double>>& notes, bool legato)
    {
        Part p;
        double at = 0.0;

        for (auto [pitch, length] : notes)
        {
            p.push_back ({ at, legato ? length + 0.05 : length * 0.9, pitch, 0.9f });
            at += length;
        }

        return p;
    }

    // Four bars at 120 BPM, in beats.
    const Part bassLine = [] { Part p; const int notes[] = { 34, 34, 46, 34, 34, 37, 34, 39 }; for (int i = 0; i < 64; ++i) p.push_back ({ i * 0.25, 0.2, notes[i % 8] - (i >= 32 && i < 48 ? 2 : 0), i % 4 == 0 ? 1.0f : 0.78f }); return p; }();
    const Part sequence = steps ({ 45, 45, 57, 45, 48, 45, 52, 43 }, 0.25, 64);
    const Part arpeggio = steps ({ 64, 67, 71, 76, 71, 67, 62, 66, 69, 74, 69, 66 }, 0.5, 30, 0.9);
    const Part minorChords = chords ({ { 52, 55, 59, 64 }, { 50, 53, 57, 62 }, { 48, 52, 55, 60 }, { 47, 50, 54, 59 } }, 4.0);
    const Part stabs = [] { Part p; const double at[] = { 0, 0.75, 1.5, 2.5, 3.0 }; for (int b = 0; b < 4; ++b) for (auto t : at) p.push_back ({ b * 4.0 + t, 0.3, b == 2 ? 50 : 52, 0.9f }); return p; }();   // the patches make the chord
    const Part melody = line ({ { 64, 1.5 }, { 67, 0.5 }, { 71, 2.0 }, { 69, 1.0 }, { 67, 1.0 }, { 64, 2.0 }, { 62, 1.0 }, { 64, 1.0 }, { 67, 2.0 }, { 64, 4.0 } }, true);
    const Part power = line ({ { 40, 2.0 }, { 40, 1.0 }, { 43, 1.0 }, { 45, 2.0 }, { 43, 1.0 }, { 38, 1.0 }, { 40, 2.0 }, { 40, 1.0 }, { 43, 1.0 }, { 47, 2.0 }, { 45, 2.0 } }, false);
    const Part chug = [] { Part p; for (int i = 0; i < 64; ++i) p.push_back ({ i * 0.25, 0.18, i % 16 >= 12 ? (i % 32 >= 16 ? 38 : 43) : 40, i % 4 == 0 ? 1.0f : 0.72f }); return p; }();
    // Sixteenths with gaps and octave leaps, a bar each on E, E, C and D: for a sound that dies away under a held key.
    const Part pulse = [] {
        const int bars[4][16] = { { 40, 40, 52, 40, 0, 40, 47, 40, 40, 52, 0, 40, 43, 40, 55, 52 }, { 40, 40, 52, 40, 0, 40, 47, 40, 40, 52, 0, 40, 45, 47, 50, 47 },
                                  { 36, 36, 48, 36, 0, 36, 43, 36, 36, 48, 0, 36, 38, 36, 50, 48 }, { 38, 38, 50, 38, 0, 38, 45, 38, 38, 50, 0, 45, 47, 45, 43, 42 } };
        Part p;

        for (int i = 0; i < 64; ++i)
            if (const auto pitch = bars[i / 16][i % 16]; pitch > 0)
                p.push_back ({ i * 0.25, 0.2, pitch, i % 4 == 0 ? 1.0f : 0.78f });

        return p;
    }();
    const Part held = { { 0.0, 7.5, 43, 0.95f }, { 8.0, 7.5, 45, 0.95f } };
    const Part one = { { 0.0, 15.5, 45, 0.95f } };
    const Part hits = { { 0.0, 1.0, 36, 1.0f }, { 6.0, 1.0, 33, 1.0f }, { 10.0, 1.0, 36, 0.8f } };

    //==============================================================================
    // What was written: a line for each clip in index.json, which the Sounds pages are made from.
    struct Index
    {
        juce::Array<juce::var> entries;

        juce::DynamicObject* add (const juce::String& group, const juce::String& category, const juce::String& name, const juce::String& file)
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("group", group);
            o->setProperty ("category", category);
            o->setProperty ("name", name);
            o->setProperty ("file", file);
            entries.add (juce::var (o));
            return o;
        }
    };

    /** A name as a file's: lower case, a hyphen for anything but letters and digits. */
    juce::String slug (const juce::String& name)
    {
        juce::String out;

        for (auto c : name.toLowerCase())
            if (juce::CharacterFunctions::isLetterOrDigit (c) && c < 128)
                out << juce::String::charToString (c);
            else if (! out.endsWithChar ('-') && out.isNotEmpty())
                out << "-";

        return out.trimCharactersAtEnd ("-");
    }

    double peakDb (const juce::AudioBuffer<float>& audio)
    {
        return juce::Decibels::gainToDecibels (audio.getMagnitude (0, audio.getNumSamples()), -100.0f);
    }

    /** Where a clip's sound has arrived: the start of the first hundredth of a second in which it reaches a
        quarter of its peak, in frames. 0 for silence. */
    int arrival (const juce::AudioBuffer<float>& audio)
    {
        const auto frames = audio.getNumSamples(), window = (int) (0.01 * rate);
        const auto peak = audio.getMagnitude (0, frames);

        if (peak > 0.0f)
            for (int at = 0; at < frames; at += window)
                if (audio.getMagnitude (at, juce::jmin (window, frames - at)) >= 0.25f * peak)
                    return at;

        return 0;
    }

    /** The clip without its dead air: joined 30 ms before its sound arrives, with a fade in over those 30 ms,
        where that is more than a tenth of a second in; and ended a twentieth of a second after it last stands
        within 60 dB of its peak, where more than a fifth of a second of nothing follows. Gives the seconds
        cut from its start. */
    double withoutDeadAir (juce::AudioBuffer<float>& audio)
    {
        const auto window = (int) (0.01 * rate), fade = (int) (0.03 * rate);
        const auto peak = audio.getMagnitude (0, audio.getNumSamples());

        if (peak <= 0.0f)
            return 0.0;

        auto end = audio.getNumSamples();

        for (int at = (end - 1) / window * window; at >= 0; at -= window)
            if (audio.getMagnitude (at, juce::jmin (window, end - at)) >= 0.001f * peak)
            {
                if (end - (at + window) > (int) (0.2 * rate))
                    end = at + window + (int) (0.05 * rate);

                break;
            }

        const auto arrives = arrival (audio);
        const auto start = arrives > (int) (0.1 * rate) ? arrives - fade : 0;

        if (start == 0 && end == audio.getNumSamples())
            return 0.0;

        juce::AudioBuffer<float> cut (audio.getNumChannels(), end - start);

        for (int ch = 0; ch < cut.getNumChannels(); ++ch)
            cut.copyFrom (ch, 0, audio, ch, start, end - start);

        if (start > 0)
            cut.applyGainRamp (0, juce::jmin (fade, cut.getNumSamples()), 0.0f, 1.0f);

        audio = std::move (cut);
        return start / rate;
    }

    /** Writes the clip as an MP3 (variable bit rate, quality 4 unless given) and notes its length, peak and size. A clip
        still sounding where it stops is faded over its last 30 ms, so the cut does not click. With `trim`, the
        clip is written without its dead air (withoutDeadAir) and the seconds cut from its start are noted.
        Nothing else is done to a render. */
    bool save (const juce::File& folder, juce::DynamicObject* entry, juce::AudioBuffer<float> audio, int vbrQuality = 4, bool trim = true)
    {
        const auto file = folder.getChildFile (entry->getProperty ("file").toString());
        file.getParentDirectory().createDirectory();

        if (const auto joined = trim ? withoutDeadAir (audio) : 0.0; joined > 0.0)
            entry->setProperty ("joined", juce::roundToInt (joined * 10.0) / 10.0);

        const auto frames = audio.getNumSamples();
        const auto fade = juce::jmin (frames, (int) (0.03 * rate));
        const auto faded = frames > 0 && audio.getMagnitude (frames - fade, fade) > 0.001f;

        if (faded)
            audio.applyGainRamp (frames - fade, fade, 1.0f, 0.0f);

        entry->setProperty ("seconds", juce::roundToInt (frames / rate * 10.0) / 10.0);
        entry->setProperty ("peakDb", juce::roundToInt (peakDb (audio) * 10.0) / 10.0);
        entry->setProperty ("faded", faded);

        LossyFiles::Settings mp3;
        mp3.variable = true;
        mp3.vbrQuality = vbrQuality;
        const auto error = LossyFiles::write (*AudioEdits::fromBuffer (std::move (audio)), rate, file, mp3);
        entry->setProperty ("bytes", file.getSize());
        std::cout << (error.isEmpty() ? entry->getProperty ("file").toString() + " (peak " + entry->getProperty ("peakDb").toString() + " dB)"
                                      : "FAILED " + file.getFileName() + ": " + error) << std::endl;
        return error.isEmpty();
    }

    /** The audio with each stretch of nothing inside it (under a thousandth of the peak, in hundredths
        of a second) cut to a quarter of a second: its first fifth of a second and its last twentieth
        are kept, so what dies away and what starts are whole. */
    juce::AudioBuffer<float> withShortGaps (const juce::AudioBuffer<float>& audio)
    {
        const auto window = (int) (0.01 * rate), keepFirst = 20, keepLast = 5;
        const auto floor = 0.001f * audio.getMagnitude (0, audio.getNumSamples());
        const auto windows = audio.getNumSamples() / window;
        std::vector<bool> kept ((size_t) windows, true);

        for (int w = 0, run = 0; w <= windows; ++w)
        {
            if (w < windows && audio.getMagnitude (w * window, window) < floor)
            {
                ++run;
                continue;
            }

            if (w < windows)
                for (int i = w - run + keepFirst; i < w - keepLast; ++i)
                    kept[(size_t) i] = false;

            run = 0;
        }

        juce::AudioBuffer<float> out (audio.getNumChannels(), audio.getNumSamples());
        int frames = 0;

        for (int w = 0; w < windows; ++w)
            if (kept[(size_t) w])
            {
                for (int ch = 0; ch < audio.getNumChannels(); ++ch)
                    out.copyFrom (ch, frames, audio, ch, w * window, window);

                frames += window;
            }

        out.setSize (out.getNumChannels(), frames, true);
        return out;
    }

    /** The share of the audio, in hundredths of a second, that stands at a tenth of its peak or more. */
    double soundingShare (const juce::AudioBuffer<float>& audio)
    {
        const auto window = (int) (0.01 * rate), windows = audio.getNumSamples() / window;
        const auto level = 0.1f * audio.getMagnitude (0, audio.getNumSamples());
        int sounding = 0;

        for (int w = 0; w < windows; ++w)
            if (audio.getMagnitude (w * window, window) >= level)
                ++sounding;

        return windows > 0 && level > 0.0f ? (double) sounding / windows : 0.0;
    }

    /** A WAV file's audio in stereo; nothing when it cannot be read. */
    juce::AudioBuffer<float> wavFile (const juce::File& file)
    {
        juce::AudioBuffer<float> audio;

        if (std::unique_ptr<juce::AudioFormatReader> reader { juce::WavAudioFormat().createReaderFor (file.createInputStream().release(), true) })
        {
            audio.setSize (2, (int) reader->lengthInSamples);
            reader->read (&audio, 0, audio.getNumSamples(), 0, true, true);
        }

        return audio;
    }

    //==============================================================================
    /** A sound on one of Torqwave's own audition phrases (written at 125 BPM), as Audition plays it:
        the phrase round twice when it is short, and time after it for the sound to ring out. */
    juce::AudioBuffer<float> audition (juce::AudioProcessor& instrument, const InstrumentKeys::Pattern& pattern, double tail)
    {
        auto end = 0.0;

        for (auto& n : pattern.notes)
            end = juce::jmax (end, n.startMs + n.lengthMs);

        const auto beatMs = 480.0;
        const auto round = pattern.lengthMs > 0.0 ? pattern.lengthMs : juce::jmax (2.0, std::ceil (end / beatMs)) * beatMs;
        const auto rounds = round < 3500.0 ? 2 : 1;
        std::vector<Hit> played;

        for (int r = 0; r < rounds; ++r)
            for (auto& n : pattern.notes)
                played.push_back ({ (r * round + n.startMs) * 0.001, n.lengthMs * 0.001, n.note, n.velocity });

        auto audio = play (instrument, played, rounds * round * 0.001 + tail, 125.0);

        // A sound slow to arrive plays the phrase once more, so the clip is not short once it is joined partway in.
        if (arrival (audio) > (int) (0.3 * rate))
        {
            for (auto& n : pattern.notes)
                played.push_back ({ (rounds * round + n.startMs) * 0.001, n.lengthMs * 0.001, n.note, n.velocity });

            audio = play (instrument, played, (rounds + 1) * round * 0.001 + tail, 125.0);
        }

        return audio;
    }

    /** An instrument's settings, read by parameter ID. */
    struct Settings
    {
        explicit Settings (juce::AudioProcessor& p)
        {
            for (auto* parameter : p.getParameters())
                if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
                    byId[ranged->getParameterID()] = ranged;
        }

        double number (const juce::String& id) const
        {
            const auto found = byId.find (id);
            return found != byId.end() ? (double) found->second->convertFrom0to1 (found->second->getValue()) : 0.0;
        }

        juce::String choice (const juce::String& id) const
        {
            const auto found = byId.find (id);

            if (found != byId.end())
                if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (found->second))
                    return c->getCurrentChoiceName();

            return {};
        }

        bool on (const juce::String& id) const   { return number (id) > 0.5; }

        std::map<juce::String, juce::RangedAudioParameter*> byId;
    };

    juce::String hertz (double hz)
    {
        return hz >= 1000.0 ? juce::String (hz / 1000.0, hz >= 10000.0 ? 0 : 1) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
    }

    /** What a synth patch is made of, read off the synth with the patch loaded: its oscillators, filters,
        effects and macros. */
    juce::String describe (BuiltInEffects::BasicSynthProcessor& synth, const WaveSynth::Patch& patch)
    {
        const Settings p (synth);
        juce::StringArray sources, filters, effects, macros;

        for (int o = 1; o <= WaveSynth::numOscillators; ++o)
        {
            const auto id = "osc" + juce::String (o);

            if (! p.on (id + "On"))
                continue;

            auto text = p.choice (id + "Table");

            if (text == "Analog")
            {
                const char* waves[] = { "sine", "triangle", "saw", "square" };
                text << " " << waves[juce::jlimit (0, 3, juce::roundToInt (p.number (id + "Position") / 33.33))];
            }

            if (const auto voices = juce::roundToInt (p.number (id + "Unison")); voices > 1)
                text << " x" << voices;

            if (const auto pitch = juce::roundToInt (p.number (id + "Pitch")); pitch != 0)
                text << (pitch == 12 ? " an octave up" : pitch == -12 ? " an octave down" : pitch == 24 ? " two octaves up" : pitch == -24 ? " two octaves down"
                                     : juce::String (pitch > 0 ? " +" : " ") + juce::String (pitch) + " st");

            if (const auto warp = p.choice (id + "Warp"); warp.isNotEmpty() && warp != "Off")
                text << " (" << warp << " warp)";

            sources.add (text);
        }

        if (p.number ("subLevel") > 0.5)
            sources.add ("a sub");

        if (p.number ("noiseLevel") > 0.5)
            sources.add (p.choice ("noiseType").toLowerCase() + " noise");

        for (int f = 1; f <= 2; ++f)
        {
            const auto id = "filter" + juce::String (f);

            if (const auto type = p.choice (id + "Type"); type.isNotEmpty() && type != "Off")
                filters.add (type + " at " + hertz (p.number (id + "Cutoff")));
        }

        if (p.on ("fxDriveOn") && p.number ("fxDriveAmount") > 0.5)   effects.add (p.choice ("fxDriveType").toLowerCase() + " drive");
        if (p.on ("fxChorusOn") && p.number ("fxChorusMix") > 0.5)    effects.add ("chorus");
        if (p.on ("fxPhaserOn") && p.number ("fxPhaserMix") > 0.5)    effects.add ("phaser");
        if (p.on ("fxDelayOn") && p.number ("fxDelayMix") > 0.5)      effects.add (p.choice ("fxDelayDivision") + " delay");
        if (p.on ("fxReverbOn") && p.number ("fxReverbMix") > 0.5)    effects.add ("reverb");

        for (auto& name : patch.macroNames)
            if (name.isNotEmpty())
                macros.add (name);

        juce::String text;
        text << sources.joinIntoString (", ") << ".";

        if (! filters.isEmpty())    text << " " << (filters.size() > 1 ? "Filters: " : "Filter: ") << filters.joinIntoString (" and ") << ".";
        if (const auto mode = p.choice ("voiceMode"); mode.isNotEmpty() && mode != "Poly")   text << " " << mode << ".";
        if (! effects.isEmpty())    text << " Effects: " << effects.joinIntoString (", ") << ".";
        if (! macros.isEmpty())     text << " Macros: " << macros.joinIntoString (", ") << ".";

        return text;
    }

    /** How much of the synth a patch uses, counted off the synth with the patch loaded: a point for each
        oscillator on, each of those with unison voices, each with a warp, the sub, the noise, each filter,
        each effect on, each modulation routing in use and each named macro. */
    int partsInUse (BuiltInEffects::BasicSynthProcessor& synth, const WaveSynth::Patch& patch)
    {
        const Settings p (synth);
        int parts = 0;

        for (int o = 1; o <= WaveSynth::numOscillators; ++o)
        {
            const auto id = "osc" + juce::String (o);

            if (! p.on (id + "On"))
                continue;

            ++parts;
            parts += p.number (id + "Unison") > 1.5 ? 1 : 0;

            if (const auto warp = p.choice (id + "Warp"); warp.isNotEmpty() && warp != "Off")
                ++parts;
        }

        parts += p.number ("subLevel") > 0.5 ? 1 : 0;
        parts += p.number ("noiseLevel") > 0.5 ? 1 : 0;

        for (int f = 1; f <= 2; ++f)
            if (const auto type = p.choice ("filter" + juce::String (f) + "Type"); type.isNotEmpty() && type != "Off")
                ++parts;

        parts += p.on ("fxDriveOn") && p.number ("fxDriveAmount") > 0.5 ? 1 : 0;
        parts += p.on ("fxChorusOn") && p.number ("fxChorusMix") > 0.5 ? 1 : 0;
        parts += p.on ("fxPhaserOn") && p.number ("fxPhaserMix") > 0.5 ? 1 : 0;
        parts += p.on ("fxDelayOn") && p.number ("fxDelayMix") > 0.5 ? 1 : 0;
        parts += p.on ("fxReverbOn") && p.number ("fxReverbMix") > 0.5 ? 1 : 0;

        for (int m = 1; m <= WaveSynth::numSlots; ++m)
        {
            const auto id = "mod" + juce::String (m);
            parts += p.number (id + "Source") > 0.5 && p.number (id + "Destination") > 0.5
                     && std::abs (p.number (id + "Amount")) > 0.5 ? 1 : 0;
        }

        for (auto& name : patch.macroNames)
            parts += name.isNotEmpty() ? 1 : 0;

        return parts;
    }

    /** A StringSwarm preset in a line, from its values. */
    juce::String describe (BuiltInEffects::SwarmGuitarProcessor& guitar)
    {
        const Settings p (guitar);
        const auto swarm = p.choice ("source") == "Swarm";
        juce::String text;
        text << juce::roundToInt (p.number ("strings")) << (swarm ? " oscillators" : " strings") << ", " << p.choice ("tuning") << " tuning";

        if (const auto layers = juce::roundToInt (p.number ("layers")); layers > 1 && ! swarm)
            text << ", " << layers << " guitars";

        text << "; " << (p.choice ("amp") == "None" ? juce::String ("no amp") : p.choice ("amp") + " amp");

        if (const auto sync = p.choice ("strumSync"); sync.isNotEmpty() && sync != "Off")
            text << "; strummed in " << sync << " with the tempo";

        return text + ".";
    }

    /** `count` of n places in a list: the first, then spread evenly over the rest. */
    std::vector<int> spread (int n, int count)
    {
        std::vector<int> picks;

        for (int k = 0; k < juce::jmin (n, count); ++k)
        {
            const auto at = k == 0 ? 0 : n <= count ? k : 1 + (int) ((juce::int64) (n - 1) * (k - 1) / (count - 1));

            if (std::find (picks.begin(), picks.end(), at) == picks.end())
                picks.push_back (at);
        }

        return picks;
    }

    //==============================================================================
    void writeSynth (const juce::File& folder, Index& index)
    {
        const auto length = 4.0 * bar + 2.0;

        // Chosen by ear for the first Sounds page, each on a part that suits it.
        const std::vector<std::pair<const char*, const Part*>> featured {
            { "Target Lock Bass", &bassLine }, { "Machine Sequence", &bassLine }, { "Warehouse Sub", &bassLine }, { "Abyss Reese", &bassLine },
            { "Acid Corrosion", &bassLine }, { "Siren Burn", &melody }, { "Mercury Lead", &melody }, { "Minor Stab", &stabs },
            { "Glass Shard", &arpeggio }, { "Wailing Fuzz Lead", &melody }, { "Body Riff Power Chord", &power }, { "Swarm Drone", &held },
            { "Concrete Pad", &minorChords }, { "Reactor Drone", &one }, { "Cold Bell", &arpeggio }, { "Gated Machine", &minorChords },
            { "Tension Riser", &one }, { "Low Boom", &hits } };

        const auto& patches = WaveSynth::factoryPatches();
        juce::StringArray done, files;

        for (auto& [name, part] : featured)
        {
            BuiltInEffects::BasicSynthProcessor synth;
            const auto found = std::find_if (patches.begin(), patches.end(), [&] (const WaveSynth::Patch& p) { return p.name == name; });

            if (found == patches.end() || ! synth.loadPatch (juce::String (name)))
            {
                std::cout << "FAILED no patch " << name << std::endl;
                continue;
            }

            auto* entry = index.add ("synth", found->category, name, "synth/" + slug (name) + ".mp3");
            entry->setProperty ("featured", true);
            entry->setProperty ("about", describe (synth, *found));
            entry->setProperty ("phrase", "four bars at 120 BPM");
            entry->setProperty ("parts", partsInUse (synth, *found));
            save (folder, entry, play (synth, *part, length));
            done.add (name);
            files.add (slug (name));
        }

        // Then the patches that use the most of the synth, 750 with those above: each category's share is
        // its share of the list (ten at least), and within a category the patches with the most parts in
        // use come first (partsInUse), the earlier in the list among equals. Each plays one of the
        // category's audition phrases.
        const int wanted = 750;
        std::vector<std::vector<const WaveSynth::Patch*>> lists;
        std::vector<int> share;
        int all = 0, given = 0;

        for (auto& category : WaveSynth::patchCategories())
        {
            if (category == "Init")
                continue;

            lists.emplace_back();

            for (auto& patch : patches)
                if (patch.category == category)
                    lists.back().push_back (&patch);

            all += (int) lists.back().size();
        }

        for (auto& list : lists)
        {
            share.push_back (juce::jmin ((int) list.size(), juce::jmax (10, juce::roundToInt ((double) wanted * (double) list.size() / all))));
            given += share.back();
        }

        // Rounding leaves a few over or under: taken from, or given to, the lists in turn, the longest first.
        std::vector<size_t> bySize (lists.size());
        std::iota (bySize.begin(), bySize.end(), (size_t) 0);
        std::stable_sort (bySize.begin(), bySize.end(), [&lists] (size_t x, size_t y) { return lists[x].size() > lists[y].size(); });

        for (int turn = 0; given != wanted && turn < 10000; ++turn)
        {
            const auto c = bySize[(size_t) turn % bySize.size()];
            const auto step = given > wanted ? -1 : 1;

            if (share[c] + step >= juce::jmin (10, (int) lists[c].size()) && share[c] + step <= (int) lists[c].size())
            {
                share[c] += step;
                given += step;
            }
        }

        for (size_t c = 0; c < lists.size(); ++c)
        {
            const auto& inCategory = lists[c];
            std::vector<std::pair<int, int>> scored;   // parts in use, place in the list

            for (int at = 0; at < (int) inCategory.size(); ++at)
            {
                BuiltInEffects::BasicSynthProcessor synth;

                if (synth.loadPatch (inCategory[(size_t) at]->name))
                    scored.emplace_back (partsInUse (synth, *inCategory[(size_t) at]), at);
            }

            std::stable_sort (scored.begin(), scored.end(), [] (const auto& x, const auto& y) { return x.first > y.first; });
            int number = 0, shown = 0;

            for (auto& patch : inCategory)
                shown += done.contains (patch->name) ? 1 : 0;

            for (int rank = 0; rank < (int) scored.size() && shown < share[c]; ++rank)
            {
                const auto at = scored[(size_t) rank].second;
                const auto& patch = *inCategory[(size_t) at];

                if (done.contains (patch.name))
                    continue;

                BuiltInEffects::BasicSynthProcessor synth;

                if (! synth.loadPatch (patch.name))
                {
                    std::cout << "FAILED no patch " << patch.name << std::endl;
                    continue;
                }

                auto file = slug (patch.name);

                while (files.contains (file))
                    file << "-2";

                files.add (file);
                const auto kind = InstrumentKeys::kindFor (synth);
                const auto& patterns = InstrumentKeys::patternsFor (kind);
                const auto& pattern = patterns[(size_t) number++ % patterns.size()];
                const auto longTail = kind == InstrumentKeys::Kind::chord || kind == InstrumentKeys::Kind::held || kind == InstrumentKeys::Kind::hit;
                auto* entry = index.add ("synth", patch.category, patch.name, "synth/" + file + ".mp3");
                entry->setProperty ("about", describe (synth, patch));
                entry->setProperty ("phrase", pattern.name);
                entry->setProperty ("place", at + 1);
                entry->setProperty ("of", (int) inCategory.size());
                entry->setProperty ("parts", scored[(size_t) rank].first);
                save (folder, entry, audition (synth, pattern, longTail ? 3.0 : 1.5));
                done.add (patch.name);
                ++shown;
            }
        }
    }

    void writeSwarm (const juce::File& folder, Index& index)
    {
        const auto length = 4.0 * bar + 2.0;

        const std::vector<std::pair<const char*, const Part*>> featured {
            { "Tremolo Swarm Wall", &held }, { "Every String One Note", &held }, { "Low Octave Roar", &power }, { "Palm Muted Chug", &chug },
            { "Crunch Riff", &power }, { "Fifths Riff", &power }, { "Tremolo Strum", &minorChords }, { "Clean Shimmer", &arpeggio },
            { "Long Low Drone", &one }, { "Eight Voice Swarm Pulse", &sequence }, { "Racing Swarm Sixteenths", &sequence },
            { "Swarm Pulse Engine", &held }, { "Fuzzed Swarm Motor", &held }, { "Converging Swarm", &one }, { "Opening Swarm", &one },
            { "Bruised Swarm", &minorChords }, { "Gliding Swarm", &melody }, { "Smeared Swarm Lead", &melody }, { "Swarm Bass", &bassLine },
            { "Swarm Opening Out", &one }, { "Swarm Through Fuzz", &melody }, { "Wailing Swarm", &melody } };

        const auto& factory = EffectPresets::factory (BuiltInEffects::swarmGuitarId);
        juce::StringArray done, categories;

        for (auto& [name, part] : featured)
        {
            const auto found = std::find_if (factory.begin(), factory.end(), [&] (const EffectPresets::Factory& f) { return f.name == name; });

            if (found == factory.end())
            {
                std::cout << "FAILED no preset " << name << std::endl;
                continue;
            }

            BuiltInEffects::SwarmGuitarProcessor guitar;
            guitar.setPresetValues (found->values);
            auto* entry = index.add ("swarm", found->category, name, "swarm/" + slug (name) + ".mp3");
            entry->setProperty ("featured", true);
            entry->setProperty ("about", describe (guitar));
            entry->setProperty ("phrase", "four bars at 120 BPM");
            save (folder, entry, play (guitar, *part, length));
            done.add (name);
        }

        for (auto& preset : factory)
            categories.addIfNotAlreadyThere (preset.category);

        // Every category: three presets by their place in the list, on the strings' audition phrases.
        for (auto& category : categories)
        {
            std::vector<const EffectPresets::Factory*> inCategory;

            for (auto& preset : factory)
                if (preset.category == category)
                    inCategory.push_back (&preset);

            int number = 0;

            for (auto at : spread ((int) inCategory.size(), 3))
            {
                const auto& preset = *inCategory[(size_t) at];

                if (done.contains (preset.name))
                    continue;

                BuiltInEffects::SwarmGuitarProcessor guitar;
                guitar.setPresetValues (preset.values);
                const auto& patterns = InstrumentKeys::patternsFor (InstrumentKeys::Kind::strings);
                const auto& pattern = patterns[(size_t) number++ % patterns.size()];
                auto* entry = index.add ("swarm", category, preset.name, "swarm/" + slug (preset.name) + ".mp3");
                entry->setProperty ("about", describe (guitar));
                entry->setProperty ("phrase", pattern.name);
                entry->setProperty ("place", at + 1);
                entry->setProperty ("of", (int) inCategory.size());
                auto audio = audition (guitar, pattern, 2.5);

                // A sound that dies away under a held key is a blip and then nothing on the audition
                // phrases, which hold their notes: it plays a moving part, as it is made to be played.
                if (const auto standing = soundingShare (audio); standing < 0.3)
                {
                    BuiltInEffects::SwarmGuitarProcessor again;
                    again.setPresetValues (preset.values);
                    audio = play (again, pulse, length);
                    entry->setProperty ("phrase", "four bars of sixteenths at 120 BPM");
                    entry->setProperty ("moving", true);
                    entry->setProperty ("heldShare", juce::roundToInt (standing * 100.0));
                    entry->setProperty ("share", juce::roundToInt (soundingShare (audio) * 100.0));
                }

                save (folder, entry, std::move (audio));
                done.add (preset.name);
            }
        }
    }

    /** Every Basic Drums kit on two of the kits' audition beats. */
    /** Torqwave Sampler's factory presets, each on the phrase Audition gives it. */
    void writeSampler (const juce::File& folder, Index& index)
    {
        for (auto& preset : BuiltInEffects::samplerPresets())
        {
            BuiltInEffects::SamplerProcessor sampler;

            if (! sampler.loadFactory (preset.name))
            {
                std::cout << "FAILED no sampler preset " << preset.name << std::endl;
                continue;
            }

            const auto kind = InstrumentKeys::kindFor (sampler);
            const auto& pattern = InstrumentKeys::patternsFor (kind).front();
            const auto zones = (int) sampler.getZones().size();
            auto* entry = index.add ("sampler", preset.category, preset.name, "sampler/" + slug (preset.name) + ".mp3");
            entry->setProperty ("about", sampler.getSlicing().on ? "A loop cut into " + juce::String (zones) + " slices, each on a key of its own."
                                                                 : juce::String (zones) + (zones == 1 ? " sample" : " samples") + " across the keys.");
            entry->setProperty ("phrase", pattern.name);
            save (folder, entry, audition (sampler, pattern, 2.5));
        }
    }

    /** The Sequence Library's factory presets unpacked into a folder of this run's own, taken away after. */
    struct UnpackedPresets
    {
        UnpackedPresets()
        {
            SequenceLibrary::setFactoryFolder (presets);

            if (const auto error = SequenceLibrary::unpackFactoryPresets(); error.isNotEmpty())
                std::cout << "FAILED " << error << std::endl;
        }

        ~UnpackedPresets()
        {
            SequenceLibrary::setFactoryFolder (before);

            // The unpacked files are read-only: made writable again to be deleted.
            for (const auto& f : juce::RangedDirectoryIterator (base, true, "*", juce::File::findFilesAndDirectories))
                f.getFile().setReadOnly (false);

            base.deleteRecursively();
        }

        const juce::File before = SequenceLibrary::factoryFolder(), base = juce::File::createTempFile ("sounddemos-presets"), presets = base.getChildFile ("set");
    };

    /** A sequence's or a kit's loop, as the Sequence Library's preview renders it at its written tempo:
        round again until it is five seconds or more from where its sound arrives. A part that is an event
        and not a loop (a riser, a transition, a hit) plays once. */
    bool writeLoop (const juce::File& folder, juce::DynamicObject* entry, const std::vector<SequenceFile::Sequence>& parts, juce::AudioPluginFormatManager& formats, bool once = false)
    {
        const auto tempo = parts.front().writtenTempo;
        juce::AudioBuffer<float> pass;
        const auto error = SequencePreview::render (parts, tempo, parts.front().beatsPerBar, rate, formats, pass);

        if (error.isNotEmpty() || pass.getNumSamples() == 0)
        {
            std::cout << "FAILED " << entry->getProperty ("name").toString() << ": " << error << std::endl;
            return false;
        }

        // Five seconds or more after the sound has arrived: a part slow to arrive goes round once more.
        const auto arrives = arrival (pass);
        const auto rounds = once ? 1 : juce::jlimit (1, 4, (int) std::ceil ((5.0 * rate + (arrives > (int) (0.1 * rate) ? arrives : 0)) / pass.getNumSamples()));
        juce::AudioBuffer<float> audio (pass.getNumChannels(), pass.getNumSamples() * rounds);

        for (int r = 0; r < rounds; ++r)
            for (int ch = 0; ch < pass.getNumChannels(); ++ch)
                audio.copyFrom (ch, r * pass.getNumSamples(), pass, ch, 0, pass.getNumSamples());

        // The step from one sample to the next where the loop joins, against the largest step anywhere else:
        // a join that steps much more than the sound itself does is a click each time round.
        if (rounds > 1)
        {
            auto join = 0.0f, inside = 0.0f;

            for (int ch = 0; ch < audio.getNumChannels(); ++ch)
                for (int i = 1; i < audio.getNumSamples(); ++i)
                {
                    const auto step = std::abs (audio.getSample (ch, i) - audio.getSample (ch, i - 1));
                    auto& into = i % pass.getNumSamples() == 0 ? join : inside;
                    into = juce::jmax (into, step);
                }

            entry->setProperty ("joinStep", juce::roundToInt (join * 10000.0f) / 10000.0);
            entry->setProperty ("insideStep", juce::roundToInt (inside * 10000.0f) / 10000.0);
        }

        juce::Array<juce::var> voices;

        for (auto& part : parts)
        {
            auto* v = new juce::DynamicObject();
            v->setProperty ("name", part.name);
            v->setProperty ("role", part.role);
            v->setProperty ("voice", part.voice != nullptr ? part.voice->getStringAttribute ("name") : juce::String());
            voices.add (juce::var (v));
        }

        entry->setProperty ("tempo", tempo);
        entry->setProperty ("rounds", rounds);
        entry->setProperty ("parts", voices);
        return save (folder, entry, std::move (audio));
    }

    /** Basic Drums' kits, each on the Sequence Library's factory grooves written for it (Drums > Full
        Grooves: the three with the most energy, the first by name among equals), as the library's
        preview plays them. A kit no factory groove uses plays two of the audition beats instead. */
    void writeDrums (const juce::File& folder, Index& index)
    {
        // The grooves of --drums-demo, for the kits no factory groove is written for.
        const auto demos = folder.getChildFile ("wav").getChildFile ("kits");
        saveDrumMachineDemos (demos);
        const UnpackedPresets unpacked;
        juce::AudioPluginFormatManager formats;
        formats.addFormat (BuiltInEffects::createFormat());
        const auto entries = SequenceLibrary (unpacked.presets).scan();
        const auto& patterns = InstrumentKeys::patternsFor (InstrumentKeys::Kind::kit);
        int number = 0;

        for (auto& kit : DrumMachine::kits())
        {
            std::vector<std::pair<const SequenceLibrary::Entry*, SequenceFile::Sequence>> grooves;

            for (auto& e : entries)
                if (! e.isKit && e.path.size() > 1 && e.path[0] == "Drums" && e.path[1] == "Full Grooves")
                {
                    juce::String error;

                    if (auto q = SequenceFile::load (e.file, error); q.has_value() && q->voice != nullptr
                                                                      && q->voice->getStringAttribute ("name") == "Basic Drums: " + kit.name)
                        grooves.emplace_back (&e, std::move (*q));
                }

            std::stable_sort (grooves.begin(), grooves.end(), [] (const auto& x, const auto& y)
            {
                return x.first->energy != y.first->energy ? x.first->energy > y.first->energy : x.first->name.compareNatural (y.first->name) < 0;
            });

            std::cout << kit.name << ": " << grooves.size() << " factory grooves" << std::endl;
            const auto shown = juce::jmin ((int) grooves.size(), 3);

            for (int g = 0; g < shown; ++g)
            {
                const auto& e = *grooves[(size_t) g].first;
                auto* entry = index.add ("drums", kit.name, e.name, "drums/" + slug (kit.name + " " + e.name) + ".mp3");
                entry->setProperty ("about", kit.description);
                entry->setProperty ("groove", e.description);
                entry->setProperty ("style", e.style);
                entry->setProperty ("bars", e.bars);
                entry->setProperty ("energy", e.energy);
                entry->setProperty ("of", (int) grooves.size());
                writeLoop (folder, entry, { grooves[(size_t) g].second }, formats);
            }

            if (shown == 0)
                for (auto which : { 0, 1 + number % juce::jmax (1, (int) patterns.size() - 1) })
                {
                    const auto& pattern = patterns[(size_t) which % patterns.size()];
                    BuiltInEffects::BasicDrumsProcessor drums;
                    drums.applyKit (kit);
                    auto* entry = index.add ("drums", kit.name, kit.name + ": " + pattern.name, "drums/" + slug (kit.name + " " + pattern.name) + ".mp3");
                    entry->setProperty ("about", kit.description);
                    entry->setProperty ("phrase", pattern.name);
                    save (folder, entry, audition (drums, pattern, 1.5));
                }

            // Such a kit's own groove too: the audition beats play the sounds' notes only, and a kit
            // made with sources has user pads on notes of their own.
            if (auto demo = wavFile (demos.getChildFile ("Basic Drums - " + kit.name + ".wav")); shown == 0 && demo.getNumSamples() > 0)
            {
                auto* entry = index.add ("drums", kit.name, kit.name + ": demo groove", "drums/" + slug (kit.name + " demo groove") + ".mp3");
                entry->setProperty ("about", kit.description);
                entry->setProperty ("demo", true);
                save (folder, entry, std::move (demo));
            }

            ++number;
        }
    }

    /** Basic Drums' synthesized sources that any pad can play, as --drums-sources renders them: each
        alone (a hard hit, a softer one, then five semitones down and five up, the nothing between the
        hits cut to a quarter of a second) and in two bars over a plain groove of the Low Rumble kit. */
    void writeSources (const juce::File& folder, Index& index)
    {
        const auto rendered = folder.getChildFile ("wav").getChildFile ("sources");
        saveDrumSourceClips (rendered);

        for (int s = 1; s <= DrumMachine::numSources; ++s)
        {
            const auto& info = DrumMachine::sourceInfo (s);
            const auto file = juce::String (s).paddedLeft ('0', 2) + " " + info.name;

            for (auto pattern : { false, true })
            {
                auto* entry = index.add ("sources", info.family, info.name, "sources/" + slug (juce::String (info.name) + (pattern ? " pattern" : "")) + ".mp3");
                entry->setProperty ("pattern", pattern);
                entry->setProperty ("number", s);
                const auto audio = wavFile (rendered.getChildFile (file + (pattern ? " - pattern" : "") + ".wav"));
                save (folder, entry, pattern ? audio : withShortGaps (audio));
            }
        }
    }

    /** The Sequence Library's factory presets: every kit, and sequences from every style (one of each role
        from the styles the library leads with; a bass line and a melody, arp or harmony part from the
        others; of each the one that does the most by measure, on a voice the style has not played yet). */
    void writeLibrary (const juce::File& folder, Index& index, bool kits, bool sequences)
    {
        const UnpackedPresets unpacked;
        const auto& presets = unpacked.presets;

        juce::AudioPluginFormatManager formats;
        formats.addFormat (BuiltInEffects::createFormat());
        auto entries = SequenceLibrary (presets).scan();
        juce::StringArray used;

        const auto fileFor = [&used] (const juce::String& group, const SequenceLibrary::Entry& e)
        {
            auto name = slug (e.style.fromFirstOccurrenceOf ("/", false, false) + " " + e.name);

            while (used.contains (group + name))
                name << "-2";

            used.add (group + name);
            return group + "/" + name + ".mp3";
        };

        const auto note = [] (juce::DynamicObject* entry, const SequenceLibrary::Entry& e)
        {
            entry->setProperty ("about", e.description);
            entry->setProperty ("key", e.key);
            entry->setProperty ("bars", e.bars);
            entry->setProperty ("energy", e.energy);
            entry->setProperty ("role", e.path.joinIntoString (" > "));
        };

        if (kits)
            for (auto& e : entries)
                if (e.isKit)
                {
                    juce::String error;

                    if (const auto kit = SequenceKit::load (e.file, error); kit.has_value() && ! kit->parts.empty())
                    {
                        auto* entry = index.add ("kits", e.style, e.name, fileFor ("kits", e));
                        note (entry, e);
                        writeLoop (folder, entry, kit->parts, formats);
                    }
                    else
                    {
                        std::cout << "FAILED " << e.name << ": " << error << std::endl;
                    }
                }

        if (sequences)
        {
            const juce::StringArray leading { "Floor/Dark Floor", "Floor/Factory Floor", "Floor/Hard Floor", "Floor/Squelch", "Machine/Machine Pulse", "Machine/Broken Circuit",
                                              "Machine/Dark Circuit", "Machine/Overdrive", "Machine/Machine Drive" };
            // What can be measured of how much a sequence does: its notes to a bar, its different pitches
            // (a drum part's different sounds), and its rhythm (the different gaps between one note's start
            // and the next, and the different note lengths).
            struct Found
            {
                const SequenceLibrary::Entry* entry = nullptr;
                SequenceFile::Sequence sequence;
                juce::String voice;
                double notesPerBar = 0.0;
                int pitches = 0, gaps = 0, lengths = 0;
                bool still = false;   // two notes a bar or fewer, or one pitch (one drum sound) only
                bool oneNoteSub = false;   // a Bass > Sub part on one pitch: the last thing to show
                juce::String notes;        // what it plays, to know it when another sequence plays the same
                juce::String figure;       // the line about it with the bars, chords and key taken out, after its role
                bool own = false;          // no other style has a sequence of that figure
                double score = 0.0;
            };

            // What a sequence does, apart from how long, on which chords and in what key.
            const auto figureOf = [] (const juce::String& description)
            {
                static const std::string chord = "(A|B flat|B|C|D|E flat|E|F sharp|F|G sharp|G)( harmonic)?( (minor|major))?( (seventh|ninth))?";
                static const std::regex bars ("\\b(for|over|through) (\\d+|two|four|eight) bars\\b"), barCount ("\\d+ bars?"),
                                        placed ("(, )?(through|on|in|moving through the roots of) (" + chord + ")(, " + chord + ")*( Phrygian| Dorian)?"),
                                        chords (chord + "(, " + chord + ")+"),
                                        key ("\\bA minor( seventh| ninth)?\\b|\\bA Phrygian\\b|\\bA harmonic minor\\b|\\bA Dorian\\b"), spaces ("\\s+");
                auto text = description.toStdString();
                text = std::regex_replace (text, bars, "");
                text = std::regex_replace (text, barCount, "");
                text = std::regex_replace (text, placed, "");
                text = std::regex_replace (text, chords, "CHORDS");
                text = std::regex_replace (text, key, "KEY");
                text = std::regex_replace (text, spaces, " ");
                return juce::String (text).trimCharactersAtStart (" ,").trimCharactersAtEnd (" ,");
            };

            const auto measure = [] (const SequenceLibrary::Entry& e, SequenceFile::Sequence q)
            {
                Found f;
                f.entry = &e;
                f.voice = q.voice != nullptr ? q.voice->getStringAttribute ("name") : juce::String();
                std::set<int> pitches;
                std::set<juce::int64> gaps, lengths, starts;

                for (auto& n : q.notes)
                {
                    pitches.insert (n.pitch);
                    lengths.insert (n.length);
                    starts.insert (n.start);
                }

                for (auto at = starts.begin(); at != starts.end() && std::next (at) != starts.end(); ++at)
                    gaps.insert (*std::next (at) - *at);

                f.notesPerBar = q.bars() > 0.0 ? (double) q.notes.size() / q.bars() : 0.0;
                f.pitches = (int) pitches.size();
                f.gaps = (int) gaps.size();
                f.lengths = (int) lengths.size();
                // An effect is an event: a hit or a sweep of one note is not a part standing still.
                f.still = (f.notesPerBar <= 2.0 || f.pitches <= 1) && ! (e.path.size() > 0 && e.path[0] == "SFX");

                // The same notes: for drums the same sounds at the same places; for the rest the same
                // pitches in the same order, so a climb written over two bars and over four is one climb.
                auto played = q.notes;
                std::stable_sort (played.begin(), played.end(), [] (const auto& x, const auto& y) { return x.start != y.start ? x.start < y.start : x.pitch < y.pitch; });
                const auto drums = e.path.size() > 0 && e.path[0] == "Drums";
                int last = -1;

                for (auto& n : played)
                    if (drums)
                        f.notes << n.pitch << "@" << juce::String (n.start) << " ";
                    else if (n.pitch != last)
                        f.notes << (last = n.pitch) << " ";

                f.oneNoteSub = f.pitches <= 1 && e.path.size() > 1 && e.path[0] == "Bass" && e.path[1] == "Sub";
                f.score = juce::jmin (16.0, f.notesPerBar) + 2.0 * juce::jmin (8, f.pitches) + 2.0 * juce::jmin (6, f.gaps) + juce::jmin (4, f.lengths);
                f.sequence = std::move (q);
                return f;
            };

            // Every sequence measured once, with its figure. A figure only one style has is that style's own;
            // most are written once and given to many styles with other chords, another voice and tempo.
            std::vector<Found> all;

            for (auto& e : entries)
                if (! e.isKit && e.path.size() > 1)
                {
                    juce::String error;

                    if (auto q = SequenceFile::load (e.file, error))
                    {
                        all.push_back (measure (e, std::move (*q)));
                        all.back().figure = e.path[0] + ": " + figureOf (e.description);
                    }
                    else
                    {
                        std::cout << "FAILED " << e.name << ": " << error << std::endl;
                    }
                }

            std::map<juce::String, std::set<juce::String>> stylesOf;

            for (auto& f : all)
                stylesOf[f.figure].insert (f.entry->style);

            for (auto& f : all)
                f.own = stylesOf[f.figure].size() == 1;

            // A sub-role that names the style (Drones in Drone, Breaks in Breaks) is what the style promises.
            const auto named = [] (const Found& f)
            {
                const auto leaf = f.entry->style.fromFirstOccurrenceOf ("/", false, false), sub = f.entry->path[1];
                return sub.startsWithIgnoreCase (leaf) || leaf.startsWithIgnoreCase (sub);
            };

            // The leading styles choose first, in the page's order, then the styles a sub-role names.
            auto styles = leading;

            for (auto& f : all)
                if (named (f))
                    styles.addIfNotAlreadyThere (f.entry->style);

            for (auto& style : SequenceLibrary::stylesIn (entries))
                styles.addIfNotAlreadyThere (style);

            juce::StringArray heardNotes, heardFigures;   // what the clips chosen so far play, on the whole page
            juce::Array<juce::var> facts;
            const juce::StringArray roles { "Drums", "Bass", "Harmony", "Pads", "Arps", "Melody", "SFX" };

            for (auto& style : styles)
            {
                juce::StringArray voices;   // the voices this style's clips already play on
                int presets = 0, own = 0, shown = 0;

                for (auto& f : all)
                    if (f.entry->style == style)
                    {
                        ++presets;
                        own += f.own ? 1 : 0;
                    }

                // The best of the style's sequences that pass, and that no clip on the page has played: neither
                // its notes nor its figure. One that moves before one that stands still, a voice the style has
                // not played before one it has, then the highest score, the most energy, the first by name.
                const auto choose = [&] (const std::function<bool (const Found&)>& passes)
                {
                    const Found* best = nullptr;
                    int of = 0;

                    for (auto& f : all)
                    {
                        if (f.entry->style != style || f.oneNoteSub || ! passes (f) || heardNotes.contains (f.notes) || heardFigures.contains (f.figure))
                            continue;

                        ++of;
                        const auto better = [&voices] (const Found& x, const Found& y)
                        {
                            const auto xUsed = voices.contains (x.voice), yUsed = voices.contains (y.voice);

                            if (x.still != y.still)   return ! x.still;
                            if (xUsed != yUsed)       return ! xUsed;
                            if (! juce::approximatelyEqual (x.score, y.score))   return x.score > y.score;
                            if (x.entry->energy != y.entry->energy)              return x.entry->energy > y.entry->energy;
                            return x.entry->name.compareNatural (y.entry->name) < 0;
                        };

                        if (best == nullptr || better (f, *best))
                            best = &f;
                    }

                    if (best == nullptr)
                        return false;

                    heardNotes.add (best->notes);
                    heardFigures.add (best->figure);
                    voices.add (best->voice);
                    ++shown;
                    auto* entry = index.add ("sequences", style, best->entry->name, fileFor ("sequences", *best->entry));
                    note (entry, *best->entry);
                    entry->setProperty ("voice", best->voice);
                    entry->setProperty ("notesPerBar", juce::roundToInt (best->notesPerBar * 10.0) / 10.0);
                    entry->setProperty ("pitches", best->pitches);
                    entry->setProperty ("of", of);
                    entry->setProperty ("still", best->still);
                    entry->setProperty ("own", best->own);
                    entry->setProperty ("named", named (*best));
                    entry->setProperty ("voiceAgain", voices.indexOf (best->voice) != voices.size() - 1);
                    writeLoop (folder, entry, { best->sequence }, formats, best->entry->path[0] == "SFX");
                    return true;
                };

                const auto is = [] (const Found& f, const char* role)   { return f.entry->path[0] == role; };

                if (leading.contains (style))
                {
                    // One of each role, where the style has a figure of its own in it; a melody's notes are
                    // always its own, whatever the line about it says.
                    for (auto& role : roles)
                        choose ([&] (const Found& f) { return f.entry->path[0] == role && (f.own || is (f, "Melody")); });
                }
                else
                {
                    // Two at most: what the style's name promises, a part of its own that is not drums, a
                    // groove of its own, a melody.
                    const std::function<bool (const Found&)> kinds[] {
                        [&] (const Found& f) { return named (f); },
                        [&] (const Found& f) { return f.own && ! is (f, "Drums") && ! is (f, "Melody"); },
                        [&] (const Found& f) { return f.own && is (f, "Drums"); },
                        [&] (const Found& f) { return is (f, "Melody"); } };

                    for (auto& kind : kinds)
                        if (shown < 2)
                            choose (kind);
                }

                auto* fact = new juce::DynamicObject();
                fact->setProperty ("style", style);
                fact->setProperty ("presets", presets);
                fact->setProperty ("own", own);
                fact->setProperty ("shown", shown);
                facts.add (juce::var (fact));
                std::cout << style << ": " << shown << " shown, " << own << " of " << presets << " presets are figures of its own" << std::endl;
            }

            folder.getChildFile ("sequence-styles.json").replaceWithText (juce::JSON::toString (juce::var (facts)) + "\n");
        }
    }
    //==============================================================================
    // Before and after: damage of a known kind and amount put on a clip Torqwave rendered, then repaired
    // by the same code the Effects menu runs. The clean clip is kept, so each repair is measured against it.

    juce::AudioBuffer<float> bufferOf (const AudioSnapshot& s)
    {
        juce::AudioBuffer<float> out (s.getNumChannels(), (int) s.getNumFrames());
        s.read (out, 0, 0, out.getNumSamples());
        return out;
    }

    AudioEdits::Snapshot snapshotOf (const juce::AudioBuffer<float>& audio)   { return AudioEdits::fromBuffer (juce::AudioBuffer<float> (audio)); }

    double rmsDb (const juce::AudioBuffer<float>& a, int from, int frames)
    {
        auto sum = 0.0;

        for (int ch = 0; ch < a.getNumChannels(); ++ch)
            for (int i = from; i < from + frames; ++i)
                sum += (double) a.getSample (ch, i) * a.getSample (ch, i);

        return 10.0 * std::log10 (juce::jmax (1.0e-20, sum / juce::jmax (1, frames * a.getNumChannels())));
    }

    /** How far a clip is from the clean one, in dB against the clean one's level, once the clean one is
        scaled to fit it best (so a repair that changes the level is not counted as an error for that). */
    double differenceDb (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& clean, int from = 0)
    {
        auto ac = 0.0, cc = 0.0;
        const auto frames = juce::jmin (a.getNumSamples(), clean.getNumSamples());

        for (int ch = 0; ch < clean.getNumChannels(); ++ch)
            for (int i = from; i < frames; ++i)
            {
                ac += (double) a.getSample (ch, i) * clean.getSample (ch, i);
                cc += (double) clean.getSample (ch, i) * clean.getSample (ch, i);
            }

        const auto g = cc > 0.0 ? ac / cc : 1.0;
        auto error = 0.0;

        for (int ch = 0; ch < clean.getNumChannels(); ++ch)
            for (int i = from; i < frames; ++i)
            {
                const auto d = a.getSample (ch, i) - g * clean.getSample (ch, i);
                error += d * d;
            }

        return 10.0 * std::log10 (juce::jmax (1.0e-20, error) / juce::jmax (1.0e-20, g * g * cc));
    }

    /** The level between two frequencies, in dB: the mean power of Hann-windowed 4,096-point frames. */
    double bandDb (const juce::AudioBuffer<float>& a, double lowHz, double highHz)
    {
        constexpr int order = 12, size = 1 << order;
        juce::dsp::FFT fft (order);
        juce::dsp::WindowingFunction<float> window ((size_t) size, juce::dsp::WindowingFunction<float>::hann);
        std::vector<float> frame ((size_t) size * 2);
        auto sum = 0.0;
        int frames = 0;

        for (int ch = 0; ch < a.getNumChannels(); ++ch)
            for (int at = 0; at + size <= a.getNumSamples(); at += size / 2, ++frames)
            {
                std::fill (frame.begin(), frame.end(), 0.0f);
                std::copy (a.getReadPointer (ch, at), a.getReadPointer (ch, at) + size, frame.begin());
                window.multiplyWithWindowingTable (frame.data(), (size_t) size);
                fft.performFrequencyOnlyForwardTransform (frame.data());

                for (int bin = (int) (lowHz * size / rate); bin <= juce::jmin (size / 2, (int) (highHz * size / rate)); ++bin)
                    sum += (double) frame[(size_t) bin] * frame[(size_t) bin] / ((double) size * size);
            }

        return 10.0 * std::log10 (juce::jmax (1.0e-20, sum / juce::jmax (1, frames)));
    }

    /** The level of one frequency over the whole clip (the left channel), in dBFS: a sine of that level would read the same. */
    double toneDb (const juce::AudioBuffer<float>& a, double hz)
    {
        auto re = 0.0, im = 0.0;

        for (int i = 0; i < a.getNumSamples(); ++i)
        {
            const auto phase = juce::MathConstants<double>::twoPi * hz * i / rate;
            re += a.getSample (0, i) * std::cos (phase);
            im += a.getSample (0, i) * std::sin (phase);
        }

        return 20.0 * std::log10 (juce::jmax (1.0e-10, 2.0 * std::sqrt (re * re + im * im) / juce::jmax (1, a.getNumSamples())));
    }

    double tenth (double v)   { return std::round (v * 10.0) / 10.0; }

    struct Quality
    {
        juce::File folder;
        juce::Array<juce::var> demos;

        juce::DynamicObject* demo (const juce::String& id, const juce::String& title)
        {
            auto* o = new juce::DynamicObject();
            o->setProperty ("id", id);
            o->setProperty ("title", title);
            demos.add (juce::var (o));
            return o;
        }

        /** A clip of a demo: its MP3 (variable bit rate, quality 2) and its spectrogram, the left channel as the editor draws it. */
        void clip (juce::DynamicObject* demo, const juce::String& which, const juce::AudioBuffer<float>& audio)
        {
            const auto name = demo->getProperty ("id").toString() + "-" + which;
            juce::DynamicObject::Ptr entry = new juce::DynamicObject();
            entry->setProperty ("file", "audio/" + name + ".mp3");
            save (folder, entry.get(), audio, 2, false);

            const auto image = Spectrogram::render (*snapshotOf (audio), 0, rate, 0.0, audio.getNumSamples() / 900.0, 900, 220);
            const auto png = folder.getChildFile ("img").getChildFile (name + ".png");
            png.getParentDirectory().createDirectory();
            png.deleteFile();

            if (juce::FileOutputStream out (png); out.openedOk())
                juce::PNGImageFormat().writeImageToStream (image, out);

            demo->setProperty (which, entry->getProperty ("file"));
            demo->setProperty (which + "Picture", "img/" + name + ".png");
            demo->setProperty (which + "Bytes", (juce::int64) entry->getProperty ("bytes") + png.getSize());
        }
    };
}

void writeQualityDemos (const juce::File& folder, const juce::StringArray& also)
{
    folder.createDirectory();
    Quality q { folder, {} };
    juce::Random random (20261124);

    // The clips: a pad's chords under a bell's arpeggio (four bars at 120 BPM and two seconds to ring), and a factory kit's loop twice.
    juce::AudioBuffer<float> tonal, beatLoop;
    juce::String kitName;

    {
        BuiltInEffects::BasicSynthProcessor pad, bell;
        pad.loadPatch (juce::String ("Concrete Pad"));
        bell.loadPatch (juce::String ("Cold Bell"));
        tonal = play (pad, minorChords, 4.0 * bar + 2.0);
        const auto arp = play (bell, arpeggio, 4.0 * bar + 2.0);

        for (int ch = 0; ch < 2; ++ch)
            tonal.addFrom (ch, 0, arp, ch, 0, arp.getNumSamples());

        tonal.applyGain (0.6f);
    }

    {
        const UnpackedPresets unpacked;
        juce::AudioPluginFormatManager formats;
        formats.addFormat (BuiltInEffects::createFormat());

        for (auto& e : SequenceLibrary (unpacked.presets).scan())
            if (e.isKit && e.style == "Machine/Machine Pulse" && (kitName.isEmpty() || e.name.compareNatural (kitName) < 0))
            {
                juce::String error;

                if (const auto kit = SequenceKit::load (e.file, error); kit.has_value() && ! kit->parts.empty())
                {
                    juce::AudioBuffer<float> pass;

                    if (SequencePreview::render (kit->parts, kit->parts.front().writtenTempo, kit->parts.front().beatsPerBar, rate, formats, pass).isEmpty())
                    {
                        kitName = e.name;
                        beatLoop.setSize (2, pass.getNumSamples() * 2);

                        for (int r = 0; r < 2; ++r)
                            for (int ch = 0; ch < 2; ++ch)
                                beatLoop.copyFrom (ch, r * pass.getNumSamples(), pass, ch, 0, pass.getNumSamples());
                    }
                }
            }
    }

    if (beatLoop.getNumSamples() == 0)
    {
        std::cout << "FAILED no kit to play" << std::endl;
        return;
    }

    const auto tonalSource = juce::String ("Torqwave Synth: Concrete Pad's chords under Cold Bell's arpeggio, four bars at 120 BPM");
    const auto beatSource = "The factory kit " + kitName + " (Machine/Machine Pulse), its loop twice, as the Sequence Library plays it";

    // Clicks: 60 of them, one to three samples long, at up to nine tenths of full scale.
    if (also.contains ("clicks"))
    {
        auto* d = q.demo ("clicks", "Clicks");
        auto damaged = tonal;
        const int clicks = 60;
        std::vector<int> places;

        for (int i = 0; i < clicks; ++i)
        {
            const auto at = 4800 + random.nextInt (damaged.getNumSamples() - 9600);
            places.push_back (at);
            const auto length = 1 + random.nextInt (3);
            const auto value = (random.nextBool() ? 1.0f : -1.0f) * (0.4f + 0.5f * random.nextFloat());

            for (int ch = 0; ch < 2; ++ch)
                for (int k = 0; k < length; ++k)
                    damaged.setSample (ch, at + k, value);
        }

        const auto repaired = ClickRepair::repair (*snapshotOf (damaged), 0, damaged.getNumSamples(), rate, ClickRepair::Settings());
        const auto after = repaired.has_value() && repaired->snapshot != nullptr ? bufferOf (*repaired->snapshot) : damaged;
        d->setProperty ("source", tonalSource);
        d->setProperty ("damage", juce::String (clicks) + " clicks put in, each one to three samples long at 40 to 90% of full scale, in both channels");
        d->setProperty ("repair", "Effects > Noise Reduction and Repair > Click and Pop Repair at its defaults (threshold 18 dB, clicks up to 3 ms)");
        d->setProperty ("clicksAdded", clicks);
        d->setProperty ("clicksRepaired", repaired.has_value() ? repaired->repaired : 0);
        d->setProperty ("skipped", repaired.has_value() ? repaired->skipped : 0);

        // What is left where each click was: how far the repaired clip stands from the clean one there.
        int left = 0, gone = 0;   // still more than a tenth of full scale away; within a hundredth

        for (auto at : places)
        {
            auto worst = 0.0f;

            for (int ch = 0; ch < 2; ++ch)
                for (int k = 0; k < 3; ++k)
                    worst = juce::jmax (worst, std::abs (after.getSample (ch, at + k) - tonal.getSample (ch, at + k)));

            left += worst > 0.1f ? 1 : 0;
            gone += worst < 0.01f ? 1 : 0;
        }

        d->setProperty ("clicksLeft", left);
        d->setProperty ("clicksGone", gone);
        d->setProperty ("clipRms", tenth (rmsDb (tonal, 0, tonal.getNumSamples())));
        d->setProperty ("differenceBefore", tenth (differenceDb (damaged, tonal)));
        d->setProperty ("differenceAfter", tenth (differenceDb (after, tonal)));
        q.clip (d, "before", damaged);
        q.clip (d, "after", after);
    }

    // Hiss: white noise 38 dB under full scale, with a second of it alone first for the noise print.
    {
        auto* d = q.demo ("hiss", "Hiss");
        const auto lead = (int) rate;
        juce::AudioBuffer<float> clean (2, lead + tonal.getNumSamples());
        clean.clear();

        for (int ch = 0; ch < 2; ++ch)
            clean.copyFrom (ch, lead, tonal, ch, 0, tonal.getNumSamples());

        auto damaged = clean;
        const auto amplitude = juce::Decibels::decibelsToGain (-38.0f) * std::sqrt (3.0f);   // a uniform noise of that RMS

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < damaged.getNumSamples(); ++i)
                damaged.addSample (ch, i, amplitude * (2.0f * random.nextFloat() - 1.0f));

        const auto snapshot = snapshotOf (damaged);
        const auto print = NoiseReduction::capture (*snapshot, 0, lead - 2400, rate);
        d->setProperty ("source", tonalSource + ", after a second of the noise alone");
        d->setProperty ("damage", "white noise at -38 dBFS RMS over the whole clip");
        d->setProperty ("repair", "Effects > Noise Reduction and Repair > Noise Reduction: the print taken from the first second, then the whole clip reduced by 12 dB (the default) and by 24 dB");
        d->setProperty ("noiseBefore", tenth (rmsDb (damaged, 4800, lead - 9600)));
        d->setProperty ("differenceBefore", tenth (differenceDb (damaged, clean, lead)));
        q.clip (d, "before", damaged);

        if (print.has_value())
            for (auto reduction : { 12.0f, 24.0f })
            {
                NoiseReduction::Settings settings;
                settings.reductionDb = reduction;
                const auto reduced = NoiseReduction::reduce (*snapshot, 0, damaged.getNumSamples(), rate, *print, settings);
                const auto after = reduced != nullptr ? bufferOf (*reduced) : damaged;
                const auto which = juce::String (reduction > 12.0f ? "after24" : "after");
                d->setProperty (reduction > 12.0f ? "noiseAfter24" : "noiseAfter", tenth (rmsDb (after, 4800, lead - 9600)));
                d->setProperty (reduction > 12.0f ? "differenceAfter24" : "differenceAfter", tenth (differenceDb (after, clean, lead)));
                q.clip (d, which, after);
            }
    }

    // Hum: 50 Hz and five harmonics, the fundamental 34 dB under full scale and each harmonic 6 dB under the one before.
    {
        auto* d = q.demo ("hum", "Mains hum");
        auto damaged = tonal;

        for (int h = 1; h <= 6; ++h)
        {
            const auto amplitude = juce::Decibels::decibelsToGain (-34.0f - 6.0f * (float) (h - 1));

            for (int i = 0; i < damaged.getNumSamples(); ++i)
            {
                const auto v = amplitude * (float) std::sin (juce::MathConstants<double>::twoPi * 50.0 * h * i / rate + 0.7 * h);
                damaged.addSample (0, i, v);
                damaged.addSample (1, i, v);
            }
        }

        const auto removed = Restoration::removeHum (*snapshotOf (damaged), 0, damaged.getNumSamples(), rate, Restoration::Hum());
        const auto after = removed != nullptr ? bufferOf (*removed) : damaged;
        d->setProperty ("source", tonalSource);
        d->setProperty ("damage", "a 50 Hz hum with five harmonics: the fundamental at -34 dBFS, each harmonic 6 dB under the one before");
        d->setProperty ("repair", "Effects > Noise Reduction and Repair > Remove Hum at its defaults (50 Hz, 8 harmonics, Q 30)");

        for (auto hz : { 50, 100, 150 })
        {
            d->setProperty ("clean" + juce::String (hz), tenth (toneDb (tonal, hz)));
            d->setProperty ("before" + juce::String (hz), tenth (toneDb (damaged, hz)));
            d->setProperty ("after" + juce::String (hz), tenth (toneDb (after, hz)));
        }
        q.clip (d, "before", damaged);
        q.clip (d, "after", after);
    }

    // Clipping: the kit pushed 4 dB over full scale and cut off flat there (then everything 6 dB down, to leave room for the rebuilt peaks).
    if (also.contains ("clipping"))
    {
        auto* d = q.demo ("clipping", "Clipping");
        auto clean = beatLoop;
        clean.applyGain (1.0f / juce::jmax (1.0e-6f, clean.getMagnitude (0, clean.getNumSamples())) * juce::Decibels::decibelsToGain (4.0f));
        auto damaged = clean;
        int clippedSamples = 0;

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < damaged.getNumSamples(); ++i)
                if (std::abs (damaged.getSample (ch, i)) > 1.0f)
                {
                    damaged.setSample (ch, i, damaged.getSample (ch, i) > 0.0f ? 1.0f : -1.0f);
                    ++clippedSamples;
                }

        clean.applyGain (0.5f);
        damaged.applyGain (0.5f);
        const auto rebuilt = Restoration::declip (*snapshotOf (damaged), 0, damaged.getNumSamples(), rate, Restoration::Declip());
        const auto after = rebuilt.audio != nullptr ? bufferOf (*rebuilt.audio) : damaged;
        d->setProperty ("source", beatSource);
        d->setProperty ("damage", "the loop turned up until its peak stood 4 dB over full scale, and cut off flat there");
        d->setProperty ("repair", "Effects > Noise Reduction and Repair > Repair Clipping at its defaults (threshold 99% of the peak, the peak kept)");
        d->setProperty ("clippedSamples", clippedSamples);
        d->setProperty ("runsRebuilt", rebuilt.repaired);
        d->setProperty ("runsTooLong", rebuilt.tooLong);
        d->setProperty ("differenceBefore", tenth (differenceDb (damaged, clean)));
        d->setProperty ("differenceAfter", tenth (differenceDb (after, clean)));
        q.clip (d, "before", damaged);
        q.clip (d, "after", after);
    }

    // Missing highs: everything over 5 kHz filtered away (eight low-pass sections in a row), then rebuilt.
    {
        auto* d = q.demo ("highs", "Missing highs");
        auto damaged = beatLoop;

        for (int ch = 0; ch < 2; ++ch)
            for (int stage = 0; stage < 8; ++stage)
            {
                juce::IIRFilter filter;
                filter.setCoefficients (juce::IIRCoefficients::makeLowPass (rate, 5000.0));
                filter.processSamples (damaged.getWritePointer (ch), damaged.getNumSamples());
            }

        const auto snapshot = snapshotOf (damaged);
        const auto cutoff = SpectralRecovery::findCutoff (*snapshot, 0, damaged.getNumSamples(), rate);
        const auto recovered = SpectralRecovery::recover (*snapshot, 0, damaged.getNumSamples(), rate, SpectralRecovery::Settings());
        const auto after = recovered != nullptr ? bufferOf (*recovered) : damaged;
        d->setProperty ("source", beatSource);
        d->setProperty ("damage", "everything over 5 kHz filtered away (eight 12 dB low-pass sections in a row at 5 kHz)");
        d->setProperty ("repair", "Effects > Noise Reduction and Repair > Spectral Recovery at its defaults (the cutoff found from the audio, level -3 dB, rebuilt up to 16 kHz)");
        d->setProperty ("recovered", recovered != nullptr);
        d->setProperty ("cutoffFound", juce::roundToInt (cutoff));
        d->setProperty ("highsOriginal", tenth (bandDb (beatLoop, 6000.0, 16000.0)));
        d->setProperty ("highsBefore", tenth (bandDb (damaged, 6000.0, 16000.0)));
        d->setProperty ("highsAfter", tenth (bandDb (after, 6000.0, 16000.0)));
        q.clip (d, "original", beatLoop);
        q.clip (d, "before", damaged);
        q.clip (d, "after", after);
    }

    // Loudness: the loop as rendered, then through the Maximizer with its gain set so it measures -10 LUFS.
    {
        auto* d = q.demo ("loudness", "Loudness");
        const auto snapshot = snapshotOf (beatLoop);
        const auto before = Loudness::measure (beatLoop, rate);
        const auto target = -10.0;
        Maximizer::Settings settings;
        settings.ceilingDb = -1.0;
        settings.gainDb = juce::jlimit (0.0, 24.0, target - before.integrated);
        auto after = beatLoop;
        Loudness::Result measured = before;

        for (int round = 0; round < 6; ++round)
        {
            if (const auto made = Maximizer::apply (*snapshot, 0, beatLoop.getNumSamples(), rate, settings))
                after = bufferOf (*made);

            measured = Loudness::measure (after, rate);

            if (std::abs (measured.integrated - target) < 0.05)
                break;

            settings.gainDb = juce::jlimit (0.0, 24.0, settings.gainDb + target - measured.integrated);
        }

        d->setProperty ("source", beatSource);
        d->setProperty ("damage", "nothing: the loop as the instruments made it");
        d->setProperty ("repair", "Effects > Dynamics > Maximizer: Balanced, ceiling -1 dBTP, its gain set so the clip measures " + juce::String (target, 0) + " LUFS");
        d->setProperty ("gainDb", tenth (settings.gainDb));
        d->setProperty ("loudnessBefore", tenth (before.integrated));
        d->setProperty ("loudnessAfter", tenth (measured.integrated));
        d->setProperty ("truePeakBefore", std::round (before.truePeak * 100.0) / 100.0);
        d->setProperty ("truePeakAfter", std::round (measured.truePeak * 100.0) / 100.0);
        d->setProperty ("rangeBefore", tenth (before.range));
        d->setProperty ("rangeAfter", tenth (measured.range));
        q.clip (d, "before", beatLoop);
        q.clip (d, "after", after);
    }

    folder.getChildFile ("facts.json").replaceWithText (juce::JSON::toString (juce::var (q.demos)) + "\n");
    std::cout << juce::JSON::toString (juce::var (q.demos)) << std::endl;
}


void writeSequenceClips (const juce::File& from, const juce::File& into, bool exact)
{
    into.createDirectory();
    juce::AudioPluginFormatManager formats;
    formats.addFormat (BuiltInEffects::createFormat());

    // A half's level and how bright it is (zero crossings a second), to say whether a clip rises.
    const auto half = [] (const juce::AudioBuffer<float>& a, int from, int frames)
    {
        auto sum = 0.0;
        int crossings = 0;

        for (int i = from + 1; i < from + frames; ++i)
        {
            sum += (double) a.getSample (0, i) * a.getSample (0, i);
            crossings += (a.getSample (0, i) >= 0.0f) != (a.getSample (0, i - 1) >= 0.0f) ? 1 : 0;
        }

        return std::pair<double, double> { 10.0 * std::log10 (juce::jmax (1.0e-12, sum / juce::jmax (1, frames))), crossings * rate / juce::jmax (1, frames) };
    };

    for (const auto& f : juce::RangedDirectoryIterator (from, true, "*.tqseq", juce::File::findFiles))
    {
        juce::String error;
        auto q = SequenceFile::load (f.getFile(), error);

        if (! q.has_value())
        {
            std::cout << "FAILED " << f.getFile().getFileName() << ": " << error << std::endl;
            continue;
        }

        // An event (a riser, a hit, anything under SFX) plays once. Anything else is the pass the library's
        // preview loops, round until four bars or more and seven seconds or more have played. Either way the
        // sound rings out after, for as long as it takes to fall 45 dB under the clip's peak, and no longer.
        const auto once = q->role == "SFX";
        const auto played = q->length;
        juce::AudioBuffer<float> pass, rung;
        error = once ? juce::String() : SequencePreview::render (*q, q->writtenTempo, q->beatsPerBar, rate, formats, pass);
        auto longer = *q;
        longer.length += 4 * q->beatsPerBar * SequenceFile::ticksPerBeat;

        if (error.isEmpty())
            error = SequencePreview::render (longer, q->writtenTempo, q->beatsPerBar, rate, formats, rung);

        if (error.isNotEmpty() || rung.getNumSamples() == 0 || (! once && pass.getNumSamples() == 0))
        {
            std::cout << "FAILED " << q->name << ": " << error << std::endl;
            continue;
        }

        const auto playedFrames = (int) ((double) rung.getNumSamples() * (double) played / (double) longer.length);
        const auto barFrames = (double) playedFrames / juce::jmax (1.0, (double) played / (q->beatsPerBar * SequenceFile::ticksPerBeat));
        const auto rounds = once ? 1 : juce::jlimit (1, 8, (int) std::ceil (juce::jmax (4.0 * barFrames, 7.0 * rate) / juce::jmax (1, pass.getNumSamples()) - 0.001));
        const auto& head = once ? rung : pass;
        const auto headFrames = once ? playedFrames : pass.getNumSamples();

        // The ring after the last round: where the once-through render goes on past its length.
        auto peak = 0.0f;

        for (int ch = 0; ch < rung.getNumChannels(); ++ch)
            peak = juce::jmax (peak, rung.getMagnitude (ch, 0, rung.getNumSamples()), head.getMagnitude (ch, 0, headFrames));

        const auto floor = peak * juce::Decibels::decibelsToGain (-45.0f);
        auto last = playedFrames;

        for (int i = playedFrames; i < rung.getNumSamples(); ++i)
            for (int ch = 0; ch < rung.getNumChannels(); ++ch)
                if (std::abs (rung.getSample (ch, i)) > floor)
                    last = i;

        const auto fade = (int) (0.02 * rate);
        const auto tail = juce::jmin (rung.getNumSamples() - playedFrames, last - playedFrames + fade);
        juce::AudioBuffer<float> audio (rung.getNumChannels(), headFrames * rounds + tail);

        for (int r = 0; r < rounds; ++r)
            for (int ch = 0; ch < audio.getNumChannels(); ++ch)
                audio.copyFrom (ch, r * headFrames, head, juce::jmin (ch, head.getNumChannels() - 1), 0, headFrames);

        for (int ch = 0; ch < audio.getNumChannels(); ++ch)
        {
            audio.copyFrom (ch, headFrames * rounds, rung, ch, playedFrames, tail);
            audio.applyGainRamp (ch, audio.getNumSamples() - juce::jmin (fade, tail), juce::jmin (fade, tail), 1.0f, 0.0f);
        }

        const auto frames = playedFrames;
        const auto first = half (audio, 0, frames / 2), second = half (audio, frames / 2, frames / 2);
        auto file = into.getChildFile (q->style.fromFirstOccurrenceOf ("/", false, false) + " - " + q->name + ".wav");
        file.deleteFile();
        std::unique_ptr<juce::OutputStream> stream (new juce::FileOutputStream (file));

        if (auto writer = juce::WavAudioFormat().createWriterFor (stream, juce::AudioFormatWriterOptions{}.withSampleRate (rate).withNumChannels (2).withBitsPerSample (exact ? 32 : 24)))
            writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());

        std::cout << q->style << "|" << q->name << "|" << (q->voice != nullptr ? q->voice->getStringAttribute ("name") : juce::String()) << "|peak " << juce::String (peakDb (audio), 1)
                  << "|level " << juce::String (first.first, 1) << " to " << juce::String (second.first, 1) << "|crossings " << juce::roundToInt (first.second) << " to "
                  << juce::roundToInt (second.second) << std::endl;
    }
}

namespace
{
    void writeComplexDemos (const juce::File& folder, Index& index);

    void writeSoundDemosImpl (const juce::File& folder, const juce::StringArray& only)
    {
        folder.createDirectory();
        const auto wanted = [&only] (const char* group) { return only.isEmpty() || only.contains (group); };
        const auto indexFile = folder.getChildFile ("index.json");
        Index index;

        // Rendering some of the groups keeps the others' lines.
        if (! only.isEmpty())
            if (const auto old = juce::JSON::parse (indexFile); old.isArray())
                for (auto& entry : *old.getArray())
                    if (! only.contains (entry["group"].toString()))
                        index.entries.add (entry);

        if (wanted ("synth"))      writeSynth (folder, index);
        if (wanted ("complex"))    writeComplexDemos (folder, index);
        if (wanted ("swarm"))      writeSwarm (folder, index);
        if (wanted ("drums"))      writeDrums (folder, index);
        if (wanted ("sources"))    writeSources (folder, index);
        if (wanted ("sampler"))    writeSampler (folder, index);

        if (wanted ("kits") || wanted ("sequences"))
            writeLibrary (folder, index, wanted ("kits"), wanted ("sequences"));

        indexFile.replaceWithText (juce::JSON::toString (juce::var (index.entries)) + "\n");

        juce::int64 bytes = 0;

        for (auto& entry : index.entries)
            bytes += (juce::int64) entry["bytes"];

        std::cout << index.entries.size() << " clips, " << juce::String ((double) bytes / 1048576.0, 1) << " MB" << std::endl;
    }
}

//==============================================================================
namespace
{
    /** A Complex category's phrase: four bars at 132 BPM in A minor, in beats. */
    Part complexPhrase (const juce::String& category)
    {
        Part p;
        const auto note = [&p] (double at, double length, int pitch, float velocity = 0.9f) { p.push_back ({ at, length, pitch, velocity }); };
        const auto chord = [&note] (double at, double length, std::initializer_list<int> pitches, float velocity = 0.9f) { for (auto n : pitches) note (at, length, n, velocity); };
        const std::vector<std::vector<int>> progression { { 57, 60, 64, 69 }, { 53, 57, 60, 64 }, { 50, 57, 62, 65 }, { 52, 59, 64, 67 } };
        const int roots[] = { 33, 33, 29, 31 };

        if (category == "Bass")            // rolling: the three sixteenths after each kick
            for (int b = 0; b < 16; ++b)
                for (int s = 1; s < 4; ++s)
                    note (b + s * 0.25, 0.2, roots[b / 4] + (b % 4 == 3 && s == 3 ? 12 : 0), s == 2 ? 0.95f : 0.8f);
        else if (category == "Sub")        // the off-beats
            for (int b = 0; b < 16; ++b)
                note (b + 0.5, 0.42, roots[b / 4], 0.9f);
        else if (category == "Reese")      // held, legato
        {
            note (0, 8.1, 33); note (8, 4.1, 29); note (12, 3.9, 31);
        }
        else if (category == "Acid")       // sixteenths with rests, octave leaps and slides (a note held into the next)
        {
            const int pitches[16] = { 45, 45, 57, 45, 0, 48, 45, 57, 45, 0, 52, 45, 57, 55, 45, 43 };
            const bool slide[16] = { false, true, false, false, false, false, true, false, false, false, true, false, false, true, false, false };

            for (int i = 0; i < 64; ++i)
                if (const auto pitch = pitches[i % 16]; pitch > 0)
                    note (i * 0.25, slide[i % 16] ? 0.3 : 0.18, pitch - (i >= 32 && i < 48 ? 4 : 0), i % 4 == 0 || i % 16 == 7 ? 1.0f : 0.72f);
        }
        else if (category == "Lead" || category == "Vox" || category == "Brass")
        {
            const std::vector<std::pair<int, double>> line = category == "Lead" ? std::vector<std::pair<int, double>> { { 69, 1.5 }, { 72, 0.5 }, { 76, 2 }, { 74, 1 }, { 72, 1 }, { 69, 2 }, { 67, 1 }, { 69, 1 }, { 72, 2 }, { 69, 4 } }
                                                           : category == "Vox" ? std::vector<std::pair<int, double>> { { 64, 2 }, { 67, 2 }, { 69, 4 }, { 67, 2 }, { 64, 2 }, { 62, 4 } }
                                                                               : std::vector<std::pair<int, double>> { { 57, 4 }, { 60, 2 }, { 59, 2 }, { 57, 3 }, { 55, 1 }, { 52, 4 } };
            double at = 0.0;

            for (auto [pitch, length] : line)
            {
                note (at, length + 0.05, pitch);
                at += length;
            }
        }
        else if (category == "Stab")       // syncopated single notes (a patch may make the chord)
        {
            const int pitches[] = { 57, 57, 53, 55 };

            for (int bar = 0; bar < 4; ++bar)
                for (auto t : { 0.5, 1.75, 2.5, 3.25 })
                    note (bar * 4 + t, 0.3, pitches[bar]);
        }
        else if (category == "Chord" || category == "Strings")
            for (int bar = 0; bar < 4; ++bar)
                chord (bar * 4.0, 3.9, { progression[(size_t) bar][0], progression[(size_t) bar][1], progression[(size_t) bar][2], progression[(size_t) bar][3] });
        else if (category == "Pad")
        {
            chord (0, 7.9, { 57, 60, 64, 71 });
            chord (8, 7.9, { 53, 57, 60, 64 });
        }
        else if (category == "Keys" || category == "Organ")
        {
            const auto rhythm = category == "Keys" ? std::vector<double> { 0.5, 1.5, 2.5, 3.0 } : std::vector<double> { 0.0, 0.75, 1.5, 2.5, 3.25 };

            for (int bar = 0; bar < 4; ++bar)
                for (auto t : rhythm)
                    chord (bar * 4 + t, category == "Keys" ? 0.45 : 0.4, { progression[(size_t) bar][0], progression[(size_t) bar][1], progression[(size_t) bar][2] });
        }
        else if (category == "Pluck")
        {
            const int pitches[] = { 69, 76, 72, 74, 67, 72 };
            int n = 0;

            for (int bar = 0; bar < 4; ++bar)
                for (auto t : { 0.0, 0.75, 1.5, 2.0, 2.75, 3.5 })
                    note (bar * 4 + t, 0.3, pitches[n++ % 6]);
        }
        else if (category == "Arp")
        {
            const std::vector<std::vector<int>> arps { { 57, 60, 64, 69 }, { 53, 57, 60, 65 }, { 50, 53, 57, 62 }, { 52, 56, 59, 64 } };

            for (int i = 0; i < 64; ++i)
            {
                const auto& a = arps[(size_t) i / 16];
                const int order[] = { 0, 1, 2, 3, 2, 1 };
                note (i * 0.25, 0.2, a[(size_t) order[i % 6]] + (i % 16 >= 12 ? 12 : 0), i % 4 == 0 ? 1.0f : 0.75f);
            }
        }
        else if (category == "Guitar")     // a chug on A, a turn at the end of each bar
            for (int i = 0; i < 64; ++i)
                note (i * 0.25, 0.18, i % 16 >= 12 ? (i % 32 >= 16 ? 43 : 48) : 45, i % 4 == 0 ? 1.0f : 0.72f);
        else if (category == "Sequence")
        {
            const int pitches[] = { 45, 45, 57, 45, 48, 45, 52, 43 };

            for (int i = 0; i < 64; ++i)
                note (i * 0.25, 0.2, pitches[i % 8] - (i >= 32 && i < 48 ? 4 : 0), i % 4 == 0 ? 1.0f : 0.78f);
        }
        else if (category == "Drone" || category == "Riser" || category == "Texture")
            note (0, 15.5, category == "Riser" ? 57 : 45, 0.95f);
        else if (category == "Cinematic")
        {
            note (0, 7.5, 45, 1.0f); note (8, 7.5, 41, 1.0f);
        }
        else if (category == "Bell")
        {
            const std::pair<double, int> hits[] = { { 0, 69 }, { 1.5, 76 }, { 3, 72 }, { 4.5, 74 }, { 6, 67 }, { 8, 69 }, { 9.5, 72 }, { 11, 79 }, { 12.5, 76 } };

            for (auto [at, pitch] : hits)
                note (at, 1.0, pitch);
        }
        else if (category == "Impact")
        {
            note (0, 1.0, 33, 1.0f); note (8, 1.0, 29, 1.0f);
        }
        else if (category == "Perc")
        {
            const int pitches[] = { 57, 60, 64, 55, 62, 57, 67, 60 };
            int n = 0;

            for (int bar = 0; bar < 4; ++bar)
                for (auto t : { 0.25, 0.75, 1.0, 1.5, 1.75, 2.25, 2.75, 3.5 })
                    note (bar * 4 + t, 0.2, pitches[n++ % 8], t == 1.0 || t == 2.75 ? 1.0f : 0.75f);
        }
        else if (category == "Drum")       // on two and four, a ghost before the next bar, a fill at the end
            for (int bar = 0; bar < 4; ++bar)
            {
                note (bar * 4 + 1, 0.2, 50, 1.0f);
                note (bar * 4 + 3, 0.2, 50, 1.0f);
                note (bar * 4 + 3.75, 0.15, 50, 0.5f);

                if (bar == 3)
                    for (auto t : { 2.25, 2.5 })
                        note (bar * 4 + t, 0.15, 47, 0.7f);
            }
        else if (category == "FX")
            for (auto [at, pitch] : std::initializer_list<std::pair<double, int>> { { 0, 60 }, { 3.5, 67 }, { 8, 60 }, { 11.5, 72 } })
                note (at, 1.0, pitch);
        else if (category == "Glitch")
        {
            const bool on[16] = { true, false, true, true, false, true, false, false, true, true, false, true, false, false, true, true };

            for (int i = 0; i < 64; ++i)
                if (on[(i * 7 + i / 16) % 16])
                    note (i * 0.25, 0.12, i % 8 == 3 ? 69 : 57, i % 4 == 0 ? 1.0f : 0.7f);
        }
        else if (category == "Game")
        {
            const int motif[] = { 57, 60, 64, 69, 68, 64, 60, 59 };

            for (int i = 0; i < 32; ++i)
                note (i * 0.5, 0.35, motif[i % 8] + (i >= 16 && i < 24 ? 5 : 0), i % 2 == 0 ? 0.95f : 0.8f);
        }
        else if (category == "Chip")
        {
            const std::vector<std::pair<int, double>> line { { 69, 1 }, { 72, 0.5 }, { 74, 0.5 }, { 76, 1 }, { 74, 1 }, { 72, 1 }, { 69, 1 }, { 67, 1 }, { 69, 1 },
                                                             { 65, 1 }, { 67, 1 }, { 69, 2 }, { 64, 2 }, { 69, 2 } };
            double at = 0.0;

            for (auto [pitch, length] : line)
            {
                note (at, length * 0.9, pitch);
                at += length;
            }
        }

        return p;
    }

    /** An RBJ biquad, low- or high-pass, run over a copy of one channel. */
    std::vector<float> biquad (const float* x, int n, double hz, bool high)
    {
        const auto w = juce::MathConstants<double>::twoPi * hz / rate, c = std::cos (w), a = std::sin (w) / (2.0 * 0.7071);
        const auto b0 = high ? (1 + c) / 2 : (1 - c) / 2, b1 = high ? -(1 + c) : 1 - c, a0 = 1 + a, a1 = -2 * c, a2 = 1 - a;
        std::vector<float> y ((size_t) n);
        double x1 = 0, x2 = 0, y1 = 0, y2 = 0;

        for (int i = 0; i < n; ++i)
        {
            const auto out = (b0 * x[i] + b1 * x1 + b0 * x2 - a1 * y1 - a2 * y2) / a0;
            x2 = x1; x1 = x[i]; y2 = y1; y1 = out;
            y[(size_t) i] = (float) out;
        }

        return y;
    }

    double meanSquare (const std::vector<float>& v)   { double sum = 0; for (auto x : v) sum += (double) x * x; return sum / juce::jmax ((size_t) 1, v.size()); }

    /** Where a render's energy sits: the side's level below 120 Hz against the mid's there (dB), and the
        shares of its power below 200 Hz and from 1 to 5 kHz (%). */
    struct Balance { double lowSideDb, below200, presence; };

    Balance balanceOf (const juce::AudioBuffer<float>& audio)
    {
        const auto n = audio.getNumSamples();
        std::vector<float> mid ((size_t) n), side ((size_t) n);

        for (int i = 0; i < n; ++i)
        {
            mid[(size_t) i] = 0.5f * (audio.getSample (0, i) + audio.getSample (1, i));
            side[(size_t) i] = 0.5f * (audio.getSample (0, i) - audio.getSample (1, i));
        }

        const auto low = [n] (const std::vector<float>& v, double hz) { auto a = biquad (v.data(), n, hz, false); return biquad (a.data(), n, hz, false); };
        const auto high = [n] (const std::vector<float>& v, double hz) { auto a = biquad (v.data(), n, hz, true); return biquad (a.data(), n, hz, true); };
        const auto all = meanSquare (mid) + meanSquare (side);
        const auto lowMid = meanSquare (low (mid, 120.0)), lowSide = meanSquare (low (side, 120.0));
        const auto under = meanSquare (low (mid, 200.0)) + meanSquare (low (side, 200.0));
        const auto band = [&] (const std::vector<float>& v) { auto h = high (v, 1000.0); return meanSquare (low (h, 5000.0)); };

        return { 10.0 * std::log10 ((lowSide + 1.0e-12) / (lowMid + 1.0e-12)), 100.0 * under / juce::jmax (1.0e-12, all),
                 100.0 * (band (mid) + band (side)) / juce::jmax (1.0e-12, all) };
    }

    /** The oscillators a patch plays (on) and those it only runs for another's FM (named by an explicit
        FM From of one played or run, its warp FM, and itself off), and the unison voices a note asks for. */
    struct Used { juce::Array<int> on, modulators; int unison = 0; };

    Used oscillatorsUsed (const WaveSynth::Patch& patch)
    {
        const auto value = [&patch] (const juce::String& id, double otherwise)
        {
            const auto found = patch.values.find (id);
            return found != patch.values.end() ? found->second : otherwise;
        };
        const auto field = [] (int o, const char* f) { return "osc" + juce::String (o) + f; };
        Used u;

        for (int o = 1; o <= WaveSynth::numOscillators; ++o)
            if (value (field (o, "On"), o == 1 ? 1.0 : 0.0) > 0.5)
                u.on.add (o);

        for (bool grown = true; grown;)
        {
            grown = false;
            auto all = u.on;
            all.addArray (u.modulators);

            for (auto o : all)
                if ((int) value (field (o, "Warp"), 0.0) == (int) WaveSynth::Warp::fm)
                    if (const auto from = (int) value (field (o, "FmFrom"), 0.0); from > 0 && ! u.on.contains (from) && ! u.modulators.contains (from))
                    {
                        u.modulators.add (from);
                        grown = true;
                    }
        }

        for (auto o : u.on)          u.unison += (int) value (field (o, "Unison"), 1.0);
        for (auto o : u.modulators)  u.unison += (int) value (field (o, "Unison"), 1.0);
        return u;
    }

    void writeComplexDemos (const juce::File& folder, Index& index)
    {
        constexpr double tempo = 132.0, beatSeconds = 60.0 / tempo, seconds = 16.0 * beatSeconds + 2.5;
        const auto toHits = [] (const Part& part)
        {
            std::vector<Hit> hits;

            for (auto& n : part)
                hits.push_back ({ n.at * beatSeconds, n.length * beatSeconds, n.pitch, n.velocity });

            return hits;
        };

        std::map<juce::String, int> totals, seen;

        for (auto& p : WaveSynth::complexPatches())
            ++totals[p.category];

        for (auto& patch : WaveSynth::complexPatches())
        {
            BuiltInEffects::BasicSynthProcessor synth;

            if (! synth.loadPatch (patch.name))
                continue;

            const auto used = oscillatorsUsed (patch);
            auto about = describe (synth, patch);

            if (! used.modulators.isEmpty())
                about = juce::String (used.on.size() + used.modulators.size()) + " oscillators (" + juce::String (used.on.size())
                        + " sounding, " + juce::String (used.modulators.size()) + " silent FM): " + about;
            else
                about = juce::String (used.on.size()) + " oscillators: " + about;

            const auto place = ++seen[patch.category];
            auto* entry = index.add ("complex", patch.category, patch.name, "complex/" + slug (patch.name) + ".mp3");
            entry->setProperty ("about", about);
            entry->setProperty ("phrase", "four bars at 132 BPM");
            entry->setProperty ("oscillators", used.on.size() + used.modulators.size());
            entry->setProperty ("modulators", used.modulators.size());
            entry->setProperty ("parts", partsInUse (synth, patch));
            entry->setProperty ("place", place);
            entry->setProperty ("of", totals[patch.category]);
            save (folder, entry, play (synth, toHits (complexPhrase (patch.category)), seconds, tempo));
        }
    }
}

void writeSoundDemos (const juce::File& folder, const juce::StringArray& only)
{
    writeSoundDemosImpl (folder, only);
}

void writeComplexClips (const juce::File& folder)
{
    folder.createDirectory();
    constexpr double tempo = 132.0, beatSeconds = 60.0 / tempo, seconds = 16.0 * beatSeconds + 2.5;
    const auto toHits = [] (const Part& part)
    {
        std::vector<Hit> hits;

        for (auto& n : part)
            hits.push_back ({ n.at * beatSeconds, n.length * beatSeconds, n.pitch, n.velocity });

        return hits;
    };

    // The floor: a kick on every beat, a closed hat on every off-beat, Basic Drums' Standard kit, at
    // -15 LUFS on its own.
    Part beat;

    for (int b = 0; b < 16; ++b)
    {
        beat.push_back ({ (double) b, 0.2, 36, 1.0f });
        beat.push_back ({ b + 0.5, 0.1, 42, 0.8f });
    }

    BuiltInEffects::BasicDrumsProcessor drumMachine;
    auto drums = play (drumMachine, toHits (beat), seconds, tempo);
    drums.applyGain ((float) juce::Decibels::decibelsToGain (-15.0 - Loudness::measure (drums, rate).integrated));

    juce::StringArray factoryNames;

    for (auto& p : WaveSynth::factoryPatches())
        factoryNames.add (p.name.toLowerCase());

    Index index;

    for (auto& patch : WaveSynth::complexPatches())
    {
        if (factoryNames.contains (patch.name.toLowerCase()))
            std::cout << "FAILED " << patch.name << " is a factory patch's name too" << std::endl;

        BuiltInEffects::BasicSynthProcessor synth;

        if (! synth.loadPatch (patch.name))
        {
            std::cout << "FAILED no patch " << patch.name << std::endl;
            continue;
        }

        const auto used = oscillatorsUsed (patch);
        const auto file = patch.category.toLowerCase() + "/" + slug (patch.name);
        auto solo = play (synth, toHits (complexPhrase (patch.category)), seconds, tempo);
        const auto loudness = Loudness::measure (solo, rate);

        // The level check's measure: the peak of a held C minor chord for two seconds and its tail.
        BuiltInEffects::BasicSynthProcessor fresh;
        fresh.loadPatch (patch.name);
        const auto chordPeak = peakDb (play (fresh, std::vector<Hit> { { 0.0, 2.0, 48, 0.8f }, { 0.0, 2.0, 51, 0.8f }, { 0.0, 2.0, 55, 0.8f } }, 4.0, tempo));

        juce::AudioBuffer<float> club (2, solo.getNumSamples());

        for (int ch = 0; ch < 2; ++ch)
        {
            club.copyFrom (ch, 0, drums, ch, 0, club.getNumSamples());
            club.addFrom (ch, 0, solo, ch, 0, club.getNumSamples());
        }

        // Never clipped: a mix that would be is turned down as a whole (and says so).
        const auto clubPeak = club.getMagnitude (0, club.getNumSamples());
        const auto turnedDown = clubPeak > 0.944f ? juce::Decibels::gainToDecibels (0.944f / clubPeak) : 0.0f;

        if (turnedDown < 0.0f)
            club.applyGain (juce::Decibels::decibelsToGain (turnedDown));

        juce::StringArray on, modulators;
        for (auto o : used.on)          on.add (juce::String (o));
        for (auto o : used.modulators)  modulators.add (juce::String (o));

        auto* entry = index.add ("complex", patch.category, patch.name, file + ".mp3");
        entry->setProperty ("club", file + "-club.mp3");
        entry->setProperty ("oscillators", used.on.size() + used.modulators.size());
        entry->setProperty ("on", on.joinIntoString (","));
        entry->setProperty ("modulators", modulators.joinIntoString (","));
        entry->setProperty ("unisonPerNote", used.unison);
        entry->setProperty ("macros", juce::StringArray (patch.macroNames.data(), (int) patch.macroNames.size()).joinIntoString (", "));
        entry->setProperty ("loudness", juce::roundToInt (loudness.integrated * 10.0) / 10.0);
        entry->setProperty ("chordPeakDb", juce::roundToInt (chordPeak * 10.0) / 10.0);
        entry->setProperty ("clubTurnedDownDb", juce::roundToInt (turnedDown * 10.0) / 10.0);
        const auto balance = balanceOf (solo);
        entry->setProperty ("lowSideDb", juce::roundToInt (balance.lowSideDb * 10.0) / 10.0);
        entry->setProperty ("below200", juce::roundToInt (balance.below200));
        entry->setProperty ("presence", juce::roundToInt (balance.presence));
        std::cout << "LEVEL|" << patch.name << "|" << patch.category << "|" << juce::String (loudness.integrated, 1) << "|" << juce::String (peakDb (solo), 1)
                  << "|" << juce::String (chordPeak, 1) << "|" << (used.on.size() + used.modulators.size()) << "|" << used.unison << "|" << juce::String (balance.lowSideDb, 1) << "|" << juce::roundToInt (balance.below200) << "|" << juce::roundToInt (balance.presence) << std::endl;

        save (folder, entry, std::move (solo));

        juce::DynamicObject::Ptr clubEntry = new juce::DynamicObject();
        clubEntry->setProperty ("file", file + "-club.mp3");
        save (folder, clubEntry.get(), std::move (club), 4, false);
        entry->setProperty ("clubPeakDb", clubEntry->getProperty ("peakDb"));
        entry->setProperty ("clubBytes", clubEntry->getProperty ("bytes"));
    }

    folder.getChildFile ("index.json").replaceWithText (juce::JSON::toString (juce::var (index.entries)) + "\n");
    std::cout << index.entries.size() << " patches" << std::endl;
}
