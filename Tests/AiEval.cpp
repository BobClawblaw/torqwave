#include "AiEval.h"
#include "../Source/AiAssistant.h"
#include "../Source/AudioEdits.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/Loudness.h"
#include "TestUtils.h"

namespace
{
    constexpr double rate = 48000.0;

    double sine (double hz, int i)     { return std::sin (juce::MathConstants<double>::twoPi * hz * i / rate); }

    /** The test files, five seconds of stereo each:
        - tone: one second of silence, three of a 440 Hz tone (left at -12 dBFS, right 6 dB lower), one of silence;
        - steady: the 440 Hz tone all five seconds, both channels at -12 dBFS;
        - hum: a 1 kHz tone at -12 dBFS over a 60 Hz hum at -20 dBFS, both channels;
        - gap: the tone of "tone" from 0 to 1.5 s and from 2.5 to 4 s, silent between and after. */
    enum class TestFile { tone, steady, hum, gap };

    AudioDocument::Snapshot testFile (TestFile kind = TestFile::tone)
    {
        juce::AudioBuffer<float> b (2, (int) rate * 5);
        b.clear();
        const auto g12 = juce::Decibels::decibelsToGain (-12.0), g18 = juce::Decibels::decibelsToGain (-18.0), g20 = juce::Decibels::decibelsToGain (-20.0);

        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const auto t = i / rate;
            double left = 0.0, right = 0.0;

            switch (kind)
            {
                case TestFile::tone:   if (t >= 1.0 && t < 4.0) { left = g12 * sine (440.0, i); right = g18 * sine (440.0, i); } break;
                case TestFile::steady: left = right = g12 * sine (440.0, i); break;
                case TestFile::hum:    left = right = g12 * sine (1000.0, i) + g20 * sine (60.0, i); break;
                case TestFile::gap:    if (t < 1.5 || (t >= 2.5 && t < 4.0)) { left = g12 * sine (440.0, i); right = g18 * sine (440.0, i); } break;
            }

            b.setSample (0, i, (float) left);
            b.setSample (1, i, (float) right);
        }

        return AudioEdits::fromBuffer (std::move (b));
    }

    /** The level in dBFS of one frequency in a channel over a range, by correlation. */
    double toneDb (const Ai::Workspace& w, double hz, double from, double to, int channel = 0)
    {
        const auto& audio = *w.getAudio();
        const auto start = (juce::int64) (from * rate), end = juce::jmin ((juce::int64) (to * rate), audio.getNumFrames());
        const auto n = (int) juce::jmax ((juce::int64) 0, end - start);
        juce::AudioBuffer<float> samples (audio.getNumChannels(), juce::jmax (1, n));
        samples.clear();
        audio.read (samples, 0, start, n);
        double re = 0.0, im = 0.0;

        for (int i = 0; i < n; ++i)
        {
            const auto phase = juce::MathConstants<double>::twoPi * hz * (double) i / rate;
            re += samples.getSample (channel, i) * std::cos (phase);
            im += samples.getSample (channel, i) * std::sin (phase);
        }

        const auto amplitude = n == 0 ? 0.0 : 2.0 * std::sqrt (re * re + im * im) / (double) n;
        return juce::Decibels::gainToDecibels (amplitude, -200.0);
    }

    double seconds (const Ai::Workspace& w)     { return (double) w.getAudio()->getNumFrames() / rate; }

    double peakDb (const Ai::Workspace& w, double from, double to, int channel = -1)
    {
        const auto levels = AudioEdits::channelLevels (*w.getAudio(), (juce::int64) (from * rate), (juce::int64) (to * rate));
        auto peak = 0.0f;

        for (size_t c = 0; c < levels.size(); ++c)
            if (channel < 0 || (int) c == channel)
                peak = juce::jmax (peak, levels[c].peak);

        return juce::Decibels::gainToDecibels ((double) peak, -200.0);
    }

    bool near (double a, double b, double within)     { return std::abs (a - b) <= within; }

    juce::String parameterText (Ai::Workspace& w, int slot, const juce::String& name)
    {
        juce::var result;
        juce::JSON::parse (w.run ("get_parameters", "{\"slot\": " + juce::String (slot) + ", \"search\": " + name.quoted() + "}"), result);

        if (auto* list = result["parameters"].getArray())
            for (auto& p : *list)
                if (p["name"].toString().equalsIgnoreCase (name))
                    return p["value"].toString();

        return {};
    }

    struct Case
    {
        juce::String request;
        std::function<juce::String (Ai::Workspace&)> check;   // empty when the plan is right, else what is wrong
        TestFile file = TestFile::tone;
        std::function<juce::String (const juce::String&)> checkAnswer = {};   // when what it says matters too
    };

    juce::String unchangedLength (const Ai::Workspace& w, double expected)
    {
        return near (seconds (w), expected, 0.01) ? juce::String() : "length " + juce::String (seconds (w), 3) + " s, not " + juce::String (expected) + " s";
    }

    juce::String noPlan (const Ai::Workspace& w)
    {
        return w.getSteps().isEmpty() && ! w.rackChanged() ? juce::String() : "it planned changes: " + w.getSteps().joinIntoString (" | ");
    }

    juce::String planHas (const Ai::Workspace& w, const juce::String& text)
    {
        for (auto& s : w.getSteps())
            if (s.containsIgnoreCase (text))
                return {};

        return "no " + text + " in the plan: " + w.getSteps().joinIntoString (" | ");
    }

    juce::String peaksAre (const Ai::Workspace& w, double left, double right, double within = 0.1)
    {
        const auto l = peakDb (w, 0, seconds (w), 0), r = peakDb (w, 0, seconds (w), 1);
        return near (l, left, within) && near (r, right, within) ? juce::String() : "left " + juce::String (l, 2) + " dB, right " + juce::String (r, 2) + " dB";
    }

    double integrated (const Ai::Workspace& w)
    {
        const auto m = Loudness::measure (*w.getAudio(), 0, w.getAudio()->getNumFrames(), rate);
        return m ? m->integrated : -200.0;
    }

    /** Requests written after the model's instructions were last changed, and never used to
        change them: what to expect from requests it hasn't been tuned on. */
    std::vector<Case> unseenCases()
    {
        using F = TestFile;
        return {
            { "Cut off the first second.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (auto p = unchangedLength (w, 4.0); p.isNotEmpty()) return p;
                  return peakDb (w, 0.0, 0.1, 0) > -12.5 ? juce::String() : "the tone doesn't start at 0";
              } },
            { "Swap the left and right channels.", [] (Ai::Workspace& w) { return peaksAre (w, -18.0, -12.0); } },
            { "Turn the whole thing down 10 dB.", [] (Ai::Workspace& w) { return peaksAre (w, -22.0, -28.0); } },
            { "Add two seconds of silence to the beginning.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (auto p = unchangedLength (w, 7.0); p.isNotEmpty()) return p;
                  return peakDb (w, 0.0, 2.95) < -100.0 && near (peakDb (w, 3.1, 3.5, 0), -12.0, 0.1) ? juce::String() : "the silence or the tone is in the wrong place";
              } },
            { "Make the right channel as loud as the left one.", [] (Ai::Workspace& w) { return peaksAre (w, -12.0, -12.0, 0.3); } },
            { "Put a marker named Start where the tone begins.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto& m = w.getMarkers();
                  return m.size() == 1 && ! m[0].isRegion() && m[0].name == "Start" && near ((double) m[0].start / rate, 1.0, 0.05)
                             ? juce::String() : "markers: " + juce::String ((int) m.size()) + (m.empty() ? juce::String() : ", first " + m[0].name.quoted() + " at " + juce::String ((double) m[0].start / rate, 3));
              } },
            { "Normalize the peaks to -1 dBFS.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto peak = peakDb (w, 0, seconds (w));
                  return near (peak, -1.0, 0.05) ? juce::String() : "peak " + juce::String (peak, 2) + " dB";
              } },
            { "Get it ready for broadcast at -23 LUFS.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto i = integrated (w);
                  return near (i, -23.0, 0.5) ? juce::String() : "loudness " + juce::String (i, 2) + " LUFS";
              } },
            { "Delete everything after the three-second mark.", [] (Ai::Workspace& w) { return unchangedLength (w, 3.0); } },
            { "Take out the silent gap in the middle.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (! near (seconds (w), 4.0, 0.1)) return "length " + juce::String (seconds (w), 3) + " s, not about 4 s";
                  return peakDb (w, 1.0, 2.5, 0) > -12.5 && peakDb (w, 1.45, 1.6, 0) > -13.0 ? juce::String() : "a gap is still there";
              }, F::gap },
            { "There's a 60 Hz hum under this. Get rid of it without touching the tone.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto hum = toneDb (w, 60.0, 1.0, 4.0), tone = toneDb (w, 1000.0, 1.0, 4.0);
                  return hum < -35.0 && near (tone, -12.0, 1.0) ? juce::String() : "60 Hz at " + juce::String (hum, 1) + " dB, 1 kHz at " + juce::String (tone, 1) + " dB";
              }, F::hum },
            { "Fade in the tone over its first second.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (auto p = unchangedLength (w, 5.0); p.isNotEmpty()) return p;
                  const auto start = peakDb (w, 1.0, 1.05, 0), later = peakDb (w, 2.1, 2.5, 0);
                  return start < later - 15.0 && later > -12.5 ? juce::String() : "start " + juce::String (start, 1) + " dB, after the fade " + juce::String (later, 1) + " dB";
              } },
            { "Fade in the first second and fade out the last second.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (auto p = unchangedLength (w, 5.0); p.isNotEmpty()) return p;
                  const auto in = peakDb (w, 0.0, 0.02), out = peakDb (w, 4.98, 5.0), middle = peakDb (w, 2.0, 3.0);
                  return in < -30.0 && out < -30.0 && near (middle, -12.0, 0.2) ? juce::String()
                                                                                 : "start " + juce::String (in, 1) + " dB, end " + juce::String (out, 1) + " dB, middle " + juce::String (middle, 1) + " dB";
              }, F::steady },
            { "Insert half a second of silence at the two-second mark.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (auto p = unchangedLength (w, 5.5); p.isNotEmpty()) return p;
                  return peakDb (w, 2.05, 2.45) < -100.0 && near (peakDb (w, 2.6, 3.0), -12.0, 0.1) ? juce::String() : "the silence isn't at 2 s";
              }, F::steady },
            { "Flip the polarity of the left channel only.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (auto p = unchangedLength (w, 5.0); p.isNotEmpty()) return p;
                  juce::AudioBuffer<float> b (2, 1);
                  w.getAudio()->read (b, 0, (juce::int64) (rate / 1760.0), 1);   // a quarter cycle in: the tone's crest
                  return b.getSample (0, 0) < -0.2f && b.getSample (1, 0) > 0.2f ? juce::String()
                                                                                  : "left " + juce::String (b.getSample (0, 0), 3) + ", right " + juce::String (b.getSample (1, 0), 3);
              }, F::steady },
            { "Make it twice as long without changing the pitch.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (! near (seconds (w), 10.0, 0.1)) return "length " + juce::String (seconds (w), 3) + " s, not 10 s";
                  const auto a = toneDb (w, 440.0, 3.0, 6.0);
                  return a > -16.0 ? juce::String() : "440 Hz at " + juce::String (a, 1) + " dB";
              }, F::steady },
            { "Raise the pitch by an octave but keep the length.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (auto p = unchangedLength (w, 5.0); p.isNotEmpty()) return p;
                  const auto up = toneDb (w, 880.0, 1.0, 4.0), old = toneDb (w, 440.0, 1.0, 4.0);
                  return up > -18.0 && old < -30.0 ? juce::String() : "880 Hz at " + juce::String (up, 1) + " dB, 440 Hz at " + juce::String (old, 1) + " dB";
              }, F::steady },
            { "Add a region called Middle from 2 to 3 seconds.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto& m = w.getMarkers();
                  return m.size() == 1 && m[0].name == "Middle" && near ((double) m[0].start / rate, 2.0, 0.01) && near ((double) m[0].getEnd() / rate, 3.0, 0.01)
                             ? juce::String() : "markers: " + juce::String ((int) m.size());
              } },
            { "Don't change anything, just tell me how loud this file is in LUFS.", [] (Ai::Workspace& w) { return noPlan (w); },
              F::tone, [] (const juce::String& said) -> juce::String
              {
                  return said.containsIgnoreCase ("LUFS") ? juce::String() : "the answer gives no loudness in LUFS";
              } },
            { "Upload this file to SoundCloud.", [] (Ai::Workspace& w) { return noPlan (w); } },
            { "Make it louder, but don't let it clip.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto file = testFile();
                  const auto before = Loudness::measure (*file, 0, file->getNumFrames(), rate);
                  const auto peak = peakDb (w, 0, seconds (w)), now = integrated (w);
                  return before && peak <= 0.0 && now >= before->integrated + 3.0 ? juce::String()
                                                                                  : "peak " + juce::String (peak, 2) + " dB, loudness " + juce::String (now, 2) + " LUFS";
              } },
            { "Add some light chorus to the whole file.", [] (Ai::Workspace& w) { return planHas (w, "chorus"); }, F::steady },
            { "Put the built-in Limiter in the rack with its ceiling at -1 dB, but don't apply it.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (w.audioChanged()) return "the audio changed";
                  const auto rack = w.createRackXml();
                  if (! w.rackChanged() || rack == nullptr || rack->getNumChildElements() != 1) return "the rack doesn't hold one plug-in";
                  const auto ceiling = parameterText (w, 1, "Ceiling");
                  return near (ceiling.getDoubleValue(), -1.0, 0.01) ? juce::String() : "ceiling " + ceiling;
              } },
            { "Make the left channel 3 dB quieter and leave the right alone.", [] (Ai::Workspace& w) { return peaksAre (w, -15.0, -18.0); } },
            { "Mix it down to mono.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (w.getAudio()->getNumChannels() == 1) return {};
                  juce::AudioBuffer<float> b (2, 1);
                  w.getAudio()->read (b, 0, (juce::int64) (2.0 * rate + rate / 1760.0), 1);
                  return std::abs (b.getSample (0, 0) - b.getSample (1, 0)) < 1.0e-4f ? juce::String() : "the channels differ";
              } },
        };
    }

    std::vector<Case> cases()
    {
        return {
            { "Trim the silence off both ends.", [] (Ai::Workspace& w) -> juce::String
              {
                  return near (seconds (w), 3.0, 0.05) ? juce::String() : "length " + juce::String (seconds (w), 3) + " s, not 3 s";
              } },
            { "Make the whole file -20 LUFS integrated.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto m = Loudness::measure (*w.getAudio(), 0, w.getAudio()->getNumFrames(), rate);
                  return m && near (m->integrated, -20.0, 0.5) ? juce::String() : "loudness " + juce::String (m ? m->integrated : 0.0, 2) + " LUFS";
              } },
            { "Fade out the last half second of the sound.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (! near (seconds (w), 5.0, 0.001))
                      return "the length changed";

                  const auto end = peakDb (w, 3.98, 4.0), before = peakDb (w, 3.0, 3.4);
                  return end < before - 20.0 && before > -12.5 ? juce::String()
                                                               : "peak near the end " + juce::String (end, 1) + " dB, before the fade " + juce::String (before, 1) + " dB";
              } },
            { "Add a region named Tone over the part that has sound in it.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto& m = w.getMarkers();
                  return m.size() == 1 && m[0].name == "Tone" && near ((double) m[0].start / rate, 1.0, 0.05) && near ((double) m[0].getEnd() / rate, 4.0, 0.05)
                             ? juce::String() : "markers: " + juce::String ((int) m.size()) + (m.empty() ? juce::String() : ", first " + m[0].name.quoted() + " at " + juce::String ((double) m[0].start / rate, 3) + " to " + juce::String ((double) m[0].getEnd() / rate, 3));
              } },
            { "Put the built-in Compressor in the effects rack with a 4:1 ratio and a threshold of -24 dB. Don't render it into the audio.", [] (Ai::Workspace& w) -> juce::String
              {
                  if (w.audioChanged())
                      return "the audio changed";

                  const auto rack = w.createRackXml();

                  if (! w.rackChanged() || rack == nullptr || rack->getNumChildElements() != 1)
                      return "the rack doesn't hold one plug-in";

                  const auto ratio = parameterText (w, 1, "Ratio"), threshold = parameterText (w, 1, "Threshold");
                  return near (ratio.getDoubleValue(), 4.0, 0.01) && near (threshold.getDoubleValue(), -24.0, 0.01) ? juce::String() : "ratio " + ratio + ", threshold " + threshold;
              } },
            { "Raise the gain by 6 dB, then limit it so the peaks stay under -6 dB.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto peak = peakDb (w, 0.0, 5.0);
                  const auto rms = AudioEdits::channelLevels (*w.getAudio(), (juce::int64) (2 * rate), (juce::int64) (3 * rate))[0].rms;
                  return peak <= -5.9 && peak > -6.6 && juce::Decibels::gainToDecibels (rms) > -15.5 ? juce::String()
                                                                                                     : "peak " + juce::String (peak, 2) + " dB, left RMS " + juce::String (juce::Decibels::gainToDecibels (rms), 2) + " dB";
              } },
            { "Reverse the second half of the file.", [] (Ai::Workspace& w) -> juce::String
              {
                  for (auto& s : w.getSteps())
                      if (s.startsWith ("Reverse") && s.contains ("2.500 s to 5.000 s"))
                          return {};

                  return "steps: " + w.getSteps().joinIntoString (" | ");
              } },
            { "Normalize each channel on its own to -3 dB.", [] (Ai::Workspace& w) -> juce::String
              {
                  const auto left = peakDb (w, 0, 5, 0), right = peakDb (w, 0, 5, 1);
                  return near (left, -3.0, 0.05) && near (right, -3.0, 0.05) ? juce::String() : "left " + juce::String (left, 2) + " dB, right " + juce::String (right, 2) + " dB";
              } },
            { "Make it sound like it was played in a big concert hall.", [] (Ai::Workspace& w) -> juce::String
              {
                  for (auto& s : w.getSteps())
                      if (s.containsIgnoreCase ("reverb"))
                          return {};

                  return "no reverb in the plan: " + w.getSteps().joinIntoString (" | ");
              } },
            { "Write out the words spoken in this recording.", [] (Ai::Workspace& w) -> juce::String
              {
                  return w.getSteps().isEmpty() ? juce::String() : "it planned changes: " + w.getSteps().joinIntoString (" | ");
              } },
        };
    }
}

int runAiEval (const juce::String& serverUrl, const juce::String& model, const juce::String& key, const juce::String& set)
{
    Ai::Settings settings;
    settings.enabled = true;
    settings.serverUrl = serverUrl;
    settings.model = model;

    juce::AudioPluginFormatManager formats;
    formats.addFormat (BuiltInEffects::createFormat());

    int failed = 0, n = 0;
    double totalSeconds = 0.0;

    auto chosen = set == "unseen" ? unseenCases() : cases();

    if (set == "all")
        for (auto& c : unseenCases())
            chosen.push_back (c);

    for (auto& c : chosen)
    {
        ++n;
        Ai::Workspace::Start start;
        start.audio = testFile (c.file);
        start.sampleRate = rate;
        start.fileName = "test tone.wav";
        start.plugins = BuiltInEffects::getDescriptions();
        start.formats = &formats;

        Ai::Agent agent (settings, key, std::move (start), c.request);
        juce::String answer, error;
        bool done = false;
        int calls = 0, refused = 0;
        agent.onTool = [&] (const juce::String&, const juce::String&, const juce::String& result)
        {
            ++calls;
            refused += result.contains ("\"ok\": false") ? 1 : 0;
        };
        agent.onDone = [&] (const juce::String& a, const juce::String& e) { answer = a; error = e; done = true; };

        const auto started = juce::Time::getMillisecondCounterHiRes();
        agent.start();
        TestUtils::runUntil ([&] { return done; }, 15 * 60 * 1000);
        const auto took = (juce::Time::getMillisecondCounterHiRes() - started) / 1000.0;
        totalSeconds += took;

        auto problem = ! done ? juce::String ("no answer in 15 minutes") : error.isNotEmpty() ? error : c.check (agent.getWorkspace());

        if (problem.isEmpty() && c.checkAnswer != nullptr)
            problem = c.checkAnswer (answer);

        failed += problem.isEmpty() ? 0 : 1;

        std::cout << (problem.isEmpty() ? "PASS  " : "FAIL  ") << c.request << "  (" << juce::String (took, 1) << " s, "
                  << calls << " tool calls, " << refused << " refused)" << std::endl;

        if (problem.isNotEmpty())
            std::cout << "      " << problem << std::endl;

        for (auto& s : agent.getWorkspace().getSteps())
            std::cout << "      plan: " << s << std::endl;

        std::cout << "      said: " << answer.trim().replace ("\n", " ").substring (0, 300) << std::endl;
    }

    std::cout << model << " (" << (set.isEmpty() ? juce::String ("tuned") : set) << " set): " << (n - failed) << " of " << n << " right, " << juce::roundToInt (totalSeconds) << " s in all" << std::endl;
    return failed;
}
