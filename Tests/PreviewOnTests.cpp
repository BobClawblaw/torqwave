#include <JuceHeader.h>
#include "../Source/SequencePreview.h"
#include "../Source/AudioEngine.h"
#include "../Source/BuiltInEffects.h"
#include "TestUtils.h"
#include "PreviewOnTests.h"

namespace
{
    using TestUtils::check;
    using PlayOn = SequencePreview::PlayOn;

    juce::PluginDescription builtIn (const juce::String& id)
    {
        for (auto& d : BuiltInEffects::getDescriptions())
            if (d.fileOrIdentifier == id)
                return d;

        return {};
    }

    SequenceFile::Sequence bassLine (const juce::PluginDescription& voice)
    {
        SequenceFile::Sequence q;
        q.name = "Low";
        q.role = "Bass";
        q.key = "A minor";
        q.length = 4 * 960;
        q.notes = { { 36, 0, 480, 0.9f }, { 38, 960, 480, 0.9f }, { 42, 1920, 480, 0.9f }, { 36, 2880, 480, 0.9f } };
        q.voice = std::shared_ptr<const juce::XmlElement> (SequenceFile::voiceFor (voice));
        return q;
    }

    bool same (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples() || a.getNumSamples() == 0)
            return false;

        for (int c = 0; c < a.getNumChannels(); ++c)
            if (std::memcmp (a.getReadPointer (c), b.getReadPointer (c), sizeof (float) * (size_t) a.getNumSamples()) != 0)
                return false;

        return true;
    }
}

void runPreviewOnTests (AudioEngine& engine)
{
    const auto synth = builtIn (BuiltInEffects::basicSynthId);
    const auto drums = builtIn (BuiltInEffects::basicDrumsId);
    const auto rate = 48000.0;
    const auto q = bassLine (synth);
    auto pad = q;
    pad.role = "Pads";

    // A plug-in chosen for a role: that role's previews render through it, others on their own voices.
    {
        PlayOn on;
        on.kind = PlayOn::Kind::perRole;
        on.forRole["Bass"] = drums;
        juce::String playsOn, padPlaysOn, ownPlaysOn;
        const auto voiced = SequencePreview::voiced ({ q }, false, on, nullptr, playsOn);
        const auto padVoiced = SequencePreview::voiced ({ pad }, false, on, nullptr, padPlaysOn);
        auto off = on;
        off.kind = PlayOn::Kind::own;
        const auto own = SequencePreview::voiced ({ q }, false, off, nullptr, ownPlaysOn);

        auto onDrums = q;
        onDrums.voice = std::shared_ptr<const juce::XmlElement> (SequenceFile::voiceFor (drums));
        juce::AudioBuffer<float> through, expected, ownAudio;
        const auto e1 = SequencePreview::render (voiced.front(), 120.0, 4, rate, engine.pluginFormatManager, through);
        const auto e2 = SequencePreview::render (onDrums, 120.0, 4, rate, engine.pluginFormatManager, expected);
        const auto e3 = SequencePreview::render (q, 120.0, 4, rate, engine.pluginFormatManager, ownAudio);

        check (SequenceFile::instrumentOf (voiced.front()) == drums.name && playsOn == drums.name && SequenceFile::instrumentOf (padVoiced.front()) == synth.name
                   && padPlaysOn.isEmpty() && SequenceFile::instrumentOf (own.front()) == synth.name && ownPlaysOn.isEmpty()
                   && e1.isEmpty() && e2.isEmpty() && e3.isEmpty() && same (through, expected) && ! same (through, ownAudio)
                   && through.getRMSLevel (0, 0, 4800) > 0.001f,
               "Preview On: with Basic Drums chosen for Bass, a bass sequence renders through it exactly as on that voice, unlike on its own; a pad and the Own Voice choice keep their own");
    }

    // The player: says what it plays on, and plays again on a new choice.
    {
        const auto folder = juce::File::createTempFile ("previewon");
        const auto file = folder.getChildFile (juce::String ("Low") + SequenceFile::extension);
        SequenceFile::save (q, file);

        SequencePreview preview (engine);
        PlayOn on;
        on.kind = PlayOn::Kind::perRole;
        on.forRole["Bass"] = drums;
        preview.setPlayOn (on);
        preview.play (file);
        TestUtils::runUntil ([&] { return preview.getState() == SequencePreview::State::playing; }, 20000);
        const auto onDrums = preview.describe();

        preview.setPlayOn ({});
        const auto again = preview.getState() == SequencePreview::State::rendering;
        TestUtils::runUntil ([&] { return preview.getState() == SequencePreview::State::playing; }, 20000);
        const auto onOwn = preview.describe();
        preview.stop();
        folder.deleteRecursively();

        check (onDrums == "Low at 120 BPM on " + drums.name && again && onOwn == "Low at 120 BPM",
               "Preview On: the preview says what it plays on, and a new choice while it plays renders it again on that: " + onDrums + " / " + onOwn);
    }

    // The selected track's instrument: followed as the selection changes; a kit keeps its own voices; no instrument track, its own.
    {
        PlayOn on;
        on.kind = PlayOn::Kind::selectedTrack;
        juce::String first, second, none, kitSays;
        const auto onFirst = SequencePreview::voiced ({ pad }, false, on, SequenceFile::voiceFor (drums), first);
        const auto onSecond = SequencePreview::voiced ({ pad }, false, on, SequenceFile::voiceFor (synth), second);
        auto drumPad = pad;
        drumPad.voice = std::shared_ptr<const juce::XmlElement> (SequenceFile::voiceFor (drums));
        const auto onNone = SequencePreview::voiced ({ drumPad }, false, on, nullptr, none);
        const auto kit = SequencePreview::voiced ({ q, drumPad }, true, on, SequenceFile::voiceFor (synth), kitSays);

        check (SequenceFile::instrumentOf (onFirst.front()) == drums.name && first.contains (drums.name) && SequenceFile::instrumentOf (onSecond.front()) == synth.name
                   && second.contains (synth.name) && SequenceFile::instrumentOf (onNone.front()) == drums.name && none.isEmpty()
                   && SequenceFile::instrumentOf (kit[0]) == synth.name && SequenceFile::instrumentOf (kit[1]) == drums.name && kitSays.isEmpty(),
               "Preview On: on the selected track's instrument a sequence plays on whichever track is selected now; with none selected, and for a kit, on its own voices");
    }

    // The choice comes back from the settings.
    {
        PlayOn on;
        on.kind = PlayOn::Kind::perRole;
        on.forRole["Drums"] = drums;
        on.forRole["Bass"] = synth;
        juce::PropertySet settings;
        const auto empty = PlayOn::load (settings);
        on.save (settings);
        const auto back = PlayOn::load (settings);
        PlayOn track;
        track.kind = PlayOn::Kind::selectedTrack;
        track.save (settings);
        const auto trackBack = PlayOn::load (settings);

        check (empty == PlayOn {} && back == on && back.forRole.at ("Drums").fileOrIdentifier == BuiltInEffects::basicDrumsId && trackBack == track && ! (trackBack == on),
               "Preview On: the choice, with each role's plug-in, comes back from the settings; with none saved, each plays on its own voice");
    }
}
