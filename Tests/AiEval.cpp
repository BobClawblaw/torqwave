#include "AiEval.h"
#include "../Source/AiAssistant.h"
#include "../Source/AudioEdits.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/Loudness.h"
#include "TestUtils.h"

namespace
{
    constexpr double rate = 48000.0;

    /** Five seconds: one of silence, three of a 440 Hz tone (left at -12 dBFS, right 6 dB lower), one of silence. */
    AudioDocument::Snapshot testFile()
    {
        juce::AudioBuffer<float> b (2, (int) rate * 5);
        b.clear();

        for (int i = (int) rate; i < (int) rate * 4; ++i)
        {
            const auto v = (float) std::sin (juce::MathConstants<double>::twoPi * 440.0 * i / rate);
            b.setSample (0, i, juce::Decibels::decibelsToGain (-12.0f) * v);
            b.setSample (1, i, juce::Decibels::decibelsToGain (-18.0f) * v);
        }

        return AudioEdits::fromBuffer (std::move (b));
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
    };

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

int runAiEval (const juce::String& serverUrl, const juce::String& model, const juce::String& key)
{
    Ai::Settings settings;
    settings.enabled = true;
    settings.serverUrl = serverUrl;
    settings.model = model;

    juce::AudioPluginFormatManager formats;
    formats.addFormat (BuiltInEffects::createFormat());

    int failed = 0, n = 0;
    double totalSeconds = 0.0;

    for (auto& c : cases())
    {
        ++n;
        Ai::Workspace::Start start;
        start.audio = testFile();
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

        const auto problem = ! done ? juce::String ("no answer in 15 minutes") : error.isNotEmpty() ? error : c.check (agent.getWorkspace());
        failed += problem.isEmpty() ? 0 : 1;

        std::cout << (problem.isEmpty() ? "PASS  " : "FAIL  ") << c.request << "  (" << juce::String (took, 1) << " s, "
                  << calls << " tool calls, " << refused << " refused)" << std::endl;

        if (problem.isNotEmpty())
            std::cout << "      " << problem << std::endl;

        for (auto& s : agent.getWorkspace().getSteps())
            std::cout << "      plan: " << s << std::endl;

        std::cout << "      said: " << answer.trim().replace ("\n", " ").substring (0, 300) << std::endl;
    }

    std::cout << model << ": " << (n - failed) << " of " << n << " right, " << juce::roundToInt (totalSeconds) << " s in all" << std::endl;
    return failed;
}
