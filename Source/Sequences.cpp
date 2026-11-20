#include "Sequences.h"
#include "PreviousName.h"
#include "Delivery.h"
#include "TimeStretch.h"
#include "ExtremeStretch.h"
#include "AudioEdits.h"
#include "CentreChannel.h"
#include "PhaseCorrection.h"
#include "PitchCorrection.h"
#include "Dereverb.h"
#include "Leveller.h"
#include "Dynamics.h"
#include "Equalizer.h"
#include "Loudness.h"
#include "Modulation.h"
#include "Multiband.h"
#include "Exciter.h"
#include "Maximizer.h"
#include "TransientShaper.h"
#include "Dither.h"
#include "Plosive.h"
#include "Breath.h"
#include "OfflineRender.h"
#include "Reverb.h"
#include "SampleRateConverter.h"
#include "Restoration.h"
#include "ClickRepair.h"
#include "AdaptiveDenoise.h"
#include "DialogueIsolation.h"
#include "Spill.h"
#include "WowFlutter.h"
#include "SpectralRecovery.h"
#include "Rebalance.h"
#include "Azimuth.h"
#include "DialogueContour.h"
#include "GraphicEq.h"
#include "NoiseReduction.h"
#include "Ambience.h"
#include "SpectralEdit.h"
#include "Generator.h"
#include "FadeShape.h"

namespace Sequences
{
namespace
{
    using Snapshot = AudioDocument::Snapshot;

    double get (const Values& v, const char* id)
    {
        const auto it = v.find (id);
        return it != v.end() ? it->second : 0.0;
    }

    struct Kind
    {
        const char* id;
        const char* name;
        std::vector<const char*> values;   // the settings it needs (an optional one, "method", "linearPhase" or "speed", takes its default when missing)
    };

    // In menu order. The names are the commands' undo names.
    const std::vector<Kind>& table()
    {
        static const std::vector<Kind> list {
            { "gain", "Gain", { "db" } },
            { "normalize", "Normalize", { "db", "apart" } },
            { "matchLoudness", "Match Loudness", { "target", "ceiling", "limit" } },
            { "deliverySpec", "Meet Delivery Spec", { "target", "tolerance", "ceiling" } },
            { "invert", "Invert", {} },
            { "reverse", "Reverse", {} },
            { "removeDc", "Remove DC Offset", {} },
            { "swapChannels", "Swap Channels", {} },
            { "invertLeft", "Invert Left Channel", {} },
            { "invertRight", "Invert Right Channel", {} },
            { "leftToRight", "Copy Left to Right", {} },
            { "rightToLeft", "Copy Right to Left", {} },
            { "mono", "Mix to Mono", {} },
            { "stereo", "Make Stereo", {} },
            { "balance", "Balance", { "pan" } },
            { "stereoWidth", "Stereo Width", { "width" } },
            { "multibandWidth", "Multiband Stereo Width", { "lowCrossover", "highCrossover", "low", "mid", "high" } },
            { "maximizer", "Maximizer", { "gain", "ceiling", "character", "emphasis" } },
            { "dither", "Dither", { "bits", "shaping" } },
            { "deplosive", "De-plosive", { "corner", "threshold", "reduction" } },
            { "breath", "Breath Control", { "threshold", "reduction" } },
            { "transientShaper", "Transient Shaper", { "lowCrossover", "highCrossover", "lowAttack", "lowSustain", "midAttack", "midSustain", "highAttack", "highSustain", "output" } },
            { "exciter", "Exciter", { "lowCrossover", "highCrossover", "lowDrive", "lowCurve", "lowMix", "midDrive", "midCurve", "midMix", "highDrive", "highCurve", "highMix", "output" } },
            { "centre", "Centre Channel", { "keep", "low", "high", "reduction" } },
            { "phase", "Automatic Phase Correction", { "most", "turnOver" } },
            { "echo", "Echo", { "delay", "feedback" } },
            { "tremolo", "Tremolo", { "rate", "depth" } },
            { "eqBand", "EQ Band", { "type", "frequency", "gain", "q" } },
            { "compressor", "Compressor", { "threshold", "ratio", "knee", "attack", "release", "makeup" } },
            { "limiter", "Limiter", { "gain", "ceiling", "release", "lookahead", "peaks" } },
            { "deEsser", "De-esser", { "frequency", "threshold", "reduction", "mode" } },
            { "chorus", "Chorus", { "rate", "delay", "depth", "feedback", "mix", "spread" } },
            { "flanger", "Flanger", { "rate", "delay", "depth", "feedback", "mix", "spread" } },
            { "phaser", "Phaser", { "rate", "low", "high", "stages", "feedback", "mix", "spread" } },
            { "reverb", "Reverb", { "preDelay", "decay", "size", "damping", "width", "mix" } },
            { "dereverb", "Reduce Reverb", { "time", "reduction" } },
            { "levelSpeech", "Level Speech", { "target", "boost", "cut", "silence" } },
            { "stretch", "Stretch", { "percent" } },
            { "extremeStretch", "Extreme Stretch", { "factor", "window" } },
            { "pitch", "Shift Pitch", { "semitones" } },
            { "pitchCorrect", "Pitch Correction", { "key", "scale", "speed", "strength" } },
            { "convertRate", "Convert Sample Rate", { "rate" } },
            { "rack", "Apply Effects Rack", {} },

            // Commands that run their own code rather than a recorded step; here so a plan (Ask
            // Torq) can run them through the same functions. Times are in seconds.
            { "removeHum", "Remove Hum", { "frequency", "harmonics", "q" } },
            { "declip", "Repair Clipping", { "threshold", "keepPeak" } },
            { "gainRamp", "Gain Ramp", { "from", "to" } },
            { "panRamp", "Pan Ramp", { "from", "to" } },
            { "fadeEdges", "Fade Selection Edges", { "ms", "shape" } },
            { "noiseGate", "Noise Gate", { "level", "length" } },
            { "silenceOutside", "Silence Outside", {} },
            { "silenceLeft", "Silence Left Channel", {} },
            { "silenceRight", "Silence Right Channel", {} },
            { "crossfadeLoop", "Crossfade Loop", { "ms" } },
            { "clickRepair", "Click Repair", { "threshold", "length", "voice" } },
            { "adaptiveDenoise", "Adaptive Noise Reduction", { "reduction", "threshold", "tracking" } },
            { "dialogueIsolation", "Dialogue Isolation", { "reduction", "sensitivity" } },
            { "spill", "Wind, Rustle and Bleed", { "wind", "corner", "rustle", "bleed", "bleedThreshold" } },
            { "wowFlutter", "Wow and Flutter", { "fastest", "amount" } },
            { "spectralRecovery", "Spectral Recovery", { "cutoff", "level", "highest" } },
            { "rebalance", "Rebalance", { "voice", "bass", "drums", "rest", "corner" } },
            { "azimuth", "Azimuth Correction", { "most", "amount" } },
            { "dialogueContour", "Dialogue Contour", { "semitones", "ending", "curve" } },
            { "graphicEq10", "Graphic EQ (10 bands)", { "hz31", "hz63", "hz125", "hz250", "hz500", "hz1k", "hz2k", "hz4k", "hz8k", "hz16k" } },
            { "multibandCompressor", "Multiband Compressor", { "lowCrossover", "highCrossover", "output",
                                                              "lowThreshold", "lowRatio", "lowAttack", "lowRelease", "lowMakeup",
                                                              "midThreshold", "midRatio", "midAttack", "midRelease", "midMakeup",
                                                              "highThreshold", "highRatio", "highAttack", "highRelease", "highMakeup" } },
            { "normalizeRegions", "Normalize Each Region", { "db" } },
            { "fadeRegions", "Fade Each Region", { "ms", "shape" } },
            { "reverseRegions", "Reverse Each Region", {} },
            { "shortenPauses", "Shorten Pauses", { "level", "length" } },
            { "noiseReduction", "Noise Reduction", { "printStart", "printEnd", "reduction", "threshold" } },
            { "ambienceMatch", "Ambience Match", { "printStart", "printEnd", "level", "fillQuiet" } },
            { "spectralAttenuate", "Spectral Attenuate", { "low", "high", "db" } },
            { "spectralRemove", "Spectral Remove", { "low", "high" } },
            { "generateTone", "Generate Tone", { "wave", "frequency", "endFrequency", "level", "length", "fade" } },
            { "generateNoise", "Generate Noise", { "colour", "level", "length", "fade" } },
        };
        return list;
    }

    const Kind* find (const juce::String& id)
    {
        for (auto& k : table())
            if (id == k.id)
                return &k;

        return nullptr;
    }

    Dynamics::Compressor compressorOf (const Values& v)
    {
        return { get (v, "threshold"), get (v, "ratio"), get (v, "knee"), get (v, "attack"), get (v, "release"), get (v, "makeup") };
    }

    Dynamics::Limiter limiterOf (const Values& v)
    {
        return { get (v, "gain"), get (v, "ceiling"), get (v, "release"), get (v, "lookahead"), get (v, "peaks") > 0.5 };
    }

    // "linearPhase" on the band-split steps is optional: 1 for the split in linear phase, else minimum phase.
    Multiband::Width widthOf (const Values& v)
    {
        return { get (v, "lowCrossover"), get (v, "highCrossover"), { get (v, "low"), get (v, "mid"), get (v, "high") }, get (v, "linearPhase") > 0.5 };
    }

    TransientShaper::Settings transientShaperOf (const Values& v)
    {
        TransientShaper::Settings s;
        s.lowCrossover = get (v, "lowCrossover");
        s.highCrossover = get (v, "highCrossover");
        s.outputDb = get (v, "output");
        s.linearPhase = get (v, "linearPhase") > 0.5;
        const std::array<const char*, 3> ids { "low", "mid", "high" };

        for (size_t b = 0; b < 3; ++b)
        {
            const juce::String id (ids[b]);
            s.bands[b] = { get (v, (id + "Attack").toRawUTF8()), get (v, (id + "Sustain").toRawUTF8()) };
        }

        return s;
    }

    Breath::Settings breathOf (const Values& v)
    {
        return { (float) get (v, "threshold"), (float) get (v, "reduction") };
    }

    Plosive::Settings plosiveOf (const Values& v)
    {
        return { get (v, "corner"), get (v, "threshold"), get (v, "reduction") };
    }

    Dither::Settings ditherOf (const Values& v)
    {
        return { juce::roundToInt (get (v, "bits")) == 1 ? 24 : 16, (Dither::Shaping) juce::jlimit (0, 2, juce::roundToInt (get (v, "shaping"))) };
    }

    Maximizer::Settings maximizerOf (const Values& v)
    {
        return { get (v, "gain"), get (v, "ceiling"), (Maximizer::Character) juce::jlimit (0, 2, juce::roundToInt (get (v, "character"))), get (v, "emphasis") };
    }

    Exciter::Settings exciterOf (const Values& v)
    {
        Exciter::Settings s;
        s.lowCrossover = get (v, "lowCrossover");
        s.highCrossover = get (v, "highCrossover");
        s.outputDb = get (v, "output");
        s.linearPhase = get (v, "linearPhase") > 0.5;
        const std::array<const char*, 3> ids { "low", "mid", "high" };

        for (size_t b = 0; b < 3; ++b)
        {
            const juce::String id (ids[b]);
            s.bands[b] = { get (v, (id + "Drive").toRawUTF8()), (Exciter::Curve) juce::jlimit (0, 2, juce::roundToInt (get (v, (id + "Curve").toRawUTF8()))),
                           get (v, (id + "Mix").toRawUTF8()) };
        }

        return s;
    }

    Multiband::DeEsser deEsserOf (const Values& v)
    {
        return { get (v, "frequency"), get (v, "threshold"), get (v, "reduction"), juce::roundToInt (get (v, "mode")) == 1, get (v, "linearPhase") > 0.5 };
    }

    Modulation::Delay delayOf (const Values& v)
    {
        return { get (v, "rate"), get (v, "delay"), get (v, "depth"), get (v, "feedback"), get (v, "mix"), get (v, "spread") };
    }

    Modulation::Phaser phaserOf (const Values& v)
    {
        return { get (v, "rate"), get (v, "low"), get (v, "high"), 2 * ((int) std::lround (get (v, "stages")) + 1),
                 get (v, "feedback"), get (v, "mix"), get (v, "spread") };
    }

    CentreChannel::Settings centreOf (const Values& v)
    {
        return { get (v, "keep") > 0.5, (float) get (v, "low"), (float) get (v, "high"), (float) get (v, "reduction") };
    }

    PitchCorrection::Settings tuningOf (const Values& v)
    {
        return { (int) std::lround (get (v, "key")), (PitchCorrection::Scale) juce::jlimit (0, 2, (int) std::lround (get (v, "scale"))),
                 (float) get (v, "speed"), (float) get (v, "strength") };
    }

    // "speed" (the fastest change, dB a second) is optional: a step without it moves as fast as the smoothing lets it.
    Leveller::Settings levellerOf (const Values& v)
    {
        Leveller::Settings s { (float) get (v, "target"), (float) get (v, "boost"), (float) get (v, "cut"), (float) get (v, "silence") };

        if (v.count ("speed") > 0)
            s.speedDbPerSecond = (float) get (v, "speed");

        return s;
    }

    RoomReverb::Settings reverbOf (const Values& v)
    {
        return { get (v, "preDelay"), get (v, "decay"), get (v, "size"), get (v, "damping"), get (v, "width"), get (v, "mix") };
    }

    Equalizer::Band bandOf (const Values& v)
    {
        return { (Equalizer::BandType) juce::jlimit (0, 4, (int) get (v, "type")), get (v, "frequency"), get (v, "gain"), get (v, "q") };
    }

    juce::String number (double x)
    {
        auto s = juce::String (x, 3);

        if (s.containsChar ('.'))
            s = s.trimCharactersAtEnd ("0").trimCharactersAtEnd (".");

        return s == "-0" ? juce::String ("0") : s;
    }
}

const juce::StringArray& kinds()
{
    static const juce::StringArray list = []
    {
        juce::StringArray ids;

        for (auto& k : table())
            ids.add (k.id);

        return ids;
    }();
    return list;
}

juce::String Step::name() const
{
    if (kind == "gain")
        return "Gain " + juce::String ((float) get (values, "db"), 2) + " dB";

    if (kind == "matchLoudness")
        return "Match Loudness " + Loudness::format (get (values, "target"), "LUFS");

    if (kind == "deliverySpec")
        return "Meet " + Delivery::specFor (get (values, "target"), get (values, "tolerance"), get (values, "ceiling")).name;

    if (kind == "convertRate")
        return "Convert to " + juce::String (get (values, "rate") / 1000.0, 1) + " kHz";

    if (kind == "levelSpeech")
        return "Level Speech to " + Loudness::format (get (values, "target"), "LUFS");

    if (kind == "centre")
        return juce::String (get (values, "keep") > 0.5 ? "Extract" : "Remove") + " Centre " + juce::String (get (values, "reduction"), 1) + " dB";

    if (kind == "dereverb")
        return "Reduce Reverb " + juce::String (get (values, "reduction"), 1) + " dB";

    // Torqwave's own stretch is named; Signalsmith Stretch, the default, is not.
    const auto method = juce::String (get (values, "method") > 0.5 ? " (Torqwave Stretch)" : "");

    if (kind == "stretch")
        return "Stretch to " + juce::String (get (values, "percent"), 1) + "%" + method;

    if (kind == "extremeStretch")
        return "Extreme Stretch " + juce::String (get (values, "factor"), 1) + "x";

    if (kind == "pitch")
    {
        const auto semitones = get (values, "semitones");
        return "Shift Pitch " + juce::String (semitones > 0.0 ? "+" : "") + juce::String (semitones, 2) + " Semitones" + method;
    }

    const auto* k = find (kind);
    return k != nullptr ? juce::String (k->name) : kind;
}

juce::String Step::settingsText() const
{
    if (kind == "rack")
    {
        const auto slots = rack != nullptr ? rack->getNumChildElements() : 0;
        return juce::String (slots) + (slots == 1 ? " effect" : " effects");
    }

    juce::StringArray parts;

    if (const auto* k = find (kind))
        for (auto* id : k->values)
            parts.add (juce::String (id) + " " + number (get (values, id)));

    return parts.joinIntoString (", ");
}

namespace
{
    /** The ranges of the newer kinds' settings, as their dialogs allow. */
    const std::map<juce::String, std::pair<double, double>>& rangesOf (const juce::String& kind)
    {
        using R = std::map<juce::String, std::pair<double, double>>;
        static const std::map<juce::String, R> all {
            { "removeHum", { { "frequency", { 20, 1000 } }, { "harmonics", { 1, 40 } }, { "q", { 5, 200 } } } },
            { "declip", { { "threshold", { 50, 100 } }, { "keepPeak", { 0, 1 } } } },
            { "gainRamp", { { "from", { -96, 24 } }, { "to", { -96, 24 } } } },
            { "panRamp", { { "from", { -100, 100 } }, { "to", { -100, 100 } } } },
            { "fadeEdges", { { "ms", { 0.1, 10000 } }, { "shape", { 0, 4 } } } },
            { "noiseGate", { { "level", { -120, 0 } }, { "length", { 0.001, 600 } } } },
            { "crossfadeLoop", { { "ms", { 0.1, 10000 } } } },
            { "clickRepair", { { "threshold", { 3, 60 } }, { "length", { 0.1, 20 } }, { "voice", { 0, 1 } } } },
            { "adaptiveDenoise", { { "reduction", { 0, 40 } }, { "threshold", { 0, 12 } }, { "tracking", { 0.5, 5 } } } },
            { "dialogueIsolation", { { "reduction", { 0, 40 } }, { "sensitivity", { 0, 100 } } } },
            { "spill", { { "wind", { 0, 40 } }, { "corner", { 60, 400 } }, { "rustle", { 0, 40 } }, { "bleed", { 0, 40 } }, { "bleedThreshold", { 6, 40 } } } },
            { "wowFlutter", { { "fastest", { 0.5, 20 } }, { "amount", { 0, 100 } } } },
            { "spectralRecovery", { { "cutoff", { 0, 16000 } }, { "level", { -24, 6 } }, { "highest", { 2000, 20000 } } } },
            { "rebalance", { { "voice", { -40, 12 } }, { "bass", { -40, 12 } }, { "drums", { -40, 12 } }, { "rest", { -40, 12 } }, { "corner", { 60, 300 } } } },
            { "azimuth", { { "most", { 0.5, 50 } }, { "amount", { 0, 100 } } } },
            { "dialogueContour", { { "semitones", { -12, 12 } }, { "ending", { 0.1, 1.5 } }, { "curve", { -1, 1 } } } },
            { "graphicEq10", { { "hz31", { -12, 12 } }, { "hz63", { -12, 12 } }, { "hz125", { -12, 12 } }, { "hz250", { -12, 12 } }, { "hz500", { -12, 12 } },
                               { "hz1k", { -12, 12 } }, { "hz2k", { -12, 12 } }, { "hz4k", { -12, 12 } }, { "hz8k", { -12, 12 } }, { "hz16k", { -12, 12 } } } },
            { "multibandCompressor", { { "lowCrossover", { 20, 20000 } }, { "highCrossover", { 20, 20000 } }, { "output", { -24, 24 } },
                                       { "lowThreshold", { -80, 0 } }, { "lowRatio", { 1, 100 } }, { "lowAttack", { 0.01, 1000 } }, { "lowRelease", { 1, 5000 } }, { "lowMakeup", { -48, 48 } },
                                       { "midThreshold", { -80, 0 } }, { "midRatio", { 1, 100 } }, { "midAttack", { 0.01, 1000 } }, { "midRelease", { 1, 5000 } }, { "midMakeup", { -48, 48 } },
                                       { "highThreshold", { -80, 0 } }, { "highRatio", { 1, 100 } }, { "highAttack", { 0.01, 1000 } }, { "highRelease", { 1, 5000 } }, { "highMakeup", { -48, 48 } } } },
            { "normalizeRegions", { { "db", { -48, 0 } } } },
            { "fadeRegions", { { "ms", { 0.1, 10000 } }, { "shape", { 0, 4 } } } },
            { "shortenPauses", { { "level", { -120, 0 } }, { "length", { 0.001, 600 } } } },
            { "noiseReduction", { { "printStart", { 0, 1.0e7 } }, { "printEnd", { 0, 1.0e7 } }, { "reduction", { 0, 60 } }, { "threshold", { -10, 20 } } } },
            { "ambienceMatch", { { "printStart", { 0, 1.0e7 } }, { "printEnd", { 0, 1.0e7 } }, { "level", { -24, 12 } }, { "fillQuiet", { 0, 1 } } } },
            { "spectralAttenuate", { { "low", { 0, 96000 } }, { "high", { 0, 96000 } }, { "db", { -60, 24 } } } },
            { "spectralRemove", { { "low", { 0, 96000 } }, { "high", { 0, 96000 } } } },
            { "generateTone", { { "wave", { 0, 3 } }, { "frequency", { 1, 20000 } }, { "endFrequency", { 1, 20000 } }, { "level", { -96, 0 } },
                                { "length", { 0.001, 600 } }, { "fade", { 0, 1000 } } } },
            { "generateNoise", { { "colour", { 0, 2 } }, { "level", { -96, -6 } }, { "length", { 0.001, 600 } }, { "fade", { 0, 1000 } } } },
        };
        static const R none;
        const auto it = all.find (kind);
        return it != all.end() ? it->second : none;
    }
}

juce::String problem (const Step& step, double rate)
{
    const auto* k = find (step.kind);

    if (k == nullptr)
        return "\"" + step.kind + "\" isn't a step this version knows.";

    for (auto* id : k->values)
        if (step.values.count (id) == 0 || ! std::isfinite (step.values.at (id)))
            return "it has no " + juce::String (id) + " setting.";

    const auto& v = step.values;
    const auto bad = [] { return juce::String ("a setting is out of range."); };

    for (const auto& [id, range] : rangesOf (step.kind))
        if (get (v, id.toRawUTF8()) < range.first || get (v, id.toRawUTF8()) > range.second)
            return bad();

    if ((step.kind == "removeHum" && get (v, "frequency") * 2.0 >= rate)
        || ((step.kind == "generateTone") && (get (v, "frequency") * 2.0 >= rate || get (v, "endFrequency") * 2.0 >= rate))
        || ((step.kind == "spectralAttenuate" || step.kind == "spectralRemove") && get (v, "high") <= get (v, "low"))
        || ((step.kind == "noiseReduction" || step.kind == "ambienceMatch") && get (v, "printEnd") <= get (v, "printStart"))
        || (step.kind == "spectralRecovery" && get (v, "cutoff") > 0.0 && get (v, "cutoff") < 1000.0)
        || (step.kind == "generateNoise"
            && get (v, "level") > Generator::maxLevelDb ((Generator::Colour) juce::jlimit (0, 2, juce::roundToInt (get (v, "colour"))))))
        return bad();

    if (step.kind == "rack" && step.rack == nullptr)
        return "it has no rack.";

    if (step.kind == "convertRate" && (get (v, "rate") < 8000.0 || get (v, "rate") > 384000.0))
        return bad();

    if (step.kind == "extremeStretch" && ! ExtremeStretch::isValid ({ get (v, "factor"), get (v, "window") }))
        return bad();

    if ((step.kind == "stretch" && (get (v, "percent") < TimeStretch::shortest * 100.0 || get (v, "percent") > TimeStretch::longest * 100.0))
        || (step.kind == "pitch" && std::abs (get (v, "semitones")) > TimeStretch::widestShift)
        || ((step.kind == "stretch" || step.kind == "pitch") && ! juce::exactlyEqual (get (v, "method"), 0.0) && ! juce::exactlyEqual (get (v, "method"), 1.0)))
        return bad();

    if ((step.kind == "deEsser" || step.kind == "multibandWidth" || step.kind == "exciter" || step.kind == "transientShaper")
        && ! juce::exactlyEqual (get (v, "linearPhase"), 0.0) && ! juce::exactlyEqual (get (v, "linearPhase"), 1.0))
        return bad();

    if (step.kind == "deliverySpec" && ! Delivery::isValid (get (v, "target"), get (v, "tolerance"), get (v, "ceiling")))
        return bad();

    if (step.kind == "matchLoudness" && (get (v, "target") < -70.0 || get (v, "target") > 0.0 || get (v, "ceiling") < -30.0 || get (v, "ceiling") > 0.0))
        return bad();

    if ((step.kind == "balance" && std::abs (get (v, "pan")) > 100.0) || (step.kind == "stereoWidth" && (get (v, "width") < 0.0 || get (v, "width") > 200.0))
        || (step.kind == "echo" && (get (v, "delay") < 1.0 || get (v, "delay") > 5000.0 || get (v, "feedback") < 0.0 || get (v, "feedback") > 95.0))
        || (step.kind == "tremolo" && (get (v, "rate") < 0.1 || get (v, "rate") > 50.0 || get (v, "depth") < 0.0 || get (v, "depth") > 100.0))
        || (step.kind == "eqBand" && ! Equalizer::isValid (bandOf (v), rate))
        || (step.kind == "compressor" && ! Dynamics::isValid (compressorOf (v)))
        || (step.kind == "limiter" && ! Dynamics::isValid (limiterOf (v)))
        || (step.kind == "deEsser" && ! Multiband::isValid (deEsserOf (v), rate))
        || (step.kind == "multibandWidth" && ! Multiband::isValid (widthOf (v), rate))
        || (step.kind == "exciter" && ! Exciter::isValid (exciterOf (v), rate))
        || (step.kind == "maximizer" && ! Maximizer::isValid (maximizerOf (v)))
        || (step.kind == "dither" && (get (v, "bits") < 0.0 || get (v, "bits") > 1.0))
        || (step.kind == "deplosive" && ! Plosive::isValid (plosiveOf (v), rate))
        || (step.kind == "breath" && ! Breath::isValid (breathOf (v)))
        || (step.kind == "transientShaper" && ! TransientShaper::isValid (transientShaperOf (v), rate))
        || ((step.kind == "chorus" || step.kind == "flanger") && ! Modulation::isValid (delayOf (v)))
        || (step.kind == "phaser" && ! Modulation::isValid (phaserOf (v)))
        || (step.kind == "reverb" && ! RoomReverb::isValid (reverbOf (v)))
        || (step.kind == "levelSpeech" && ! Leveller::isValid (levellerOf (v)))
        || (step.kind == "centre" && ! CentreChannel::isValid (centreOf (v)))
        || (step.kind == "pitchCorrect" && (get (v, "scale") < 0.0 || get (v, "scale") > 2.0 || ! PitchCorrection::isValid (tuningOf (v))))
        || (step.kind == "phase" && (get (v, "most") < PhaseCorrection::shortestMs || get (v, "most") > PhaseCorrection::longestMs))
        || (step.kind == "dereverb" && ! Dereverb::isValid ({ (float) get (v, "time"), (float) get (v, "reduction") })))
        return bad();

    return {};
}

Result apply (const Step& step, const Snapshot& audio, juce::int64 start, juce::int64 end, double rate,
              const Markers& markers, juce::AudioPluginFormatManager* plugins, const std::function<bool (float)>& progress)
{
    Result out;
    out.sampleRate = rate;
    out.markers = markers;
    start = juce::jlimit ((juce::int64) 0, audio->getNumFrames(), start);
    end = juce::jlimit (start, audio->getNumFrames(), end);
    out.start = start;
    out.end = end;

    if (out.error = problem (step, rate); out.error.isNotEmpty())
        return out;

    const auto& s = *audio;
    const auto& v = step.values;
    const auto& k = step.kind;

    const auto toFrames = [rate] (double seconds) { return (juce::int64) std::llround (seconds * rate); };
    const auto keepGoing = [&progress] (float f) { return progress == nullptr || progress (f); };
    const auto shapeOf = [&v] { return (FadeShape) juce::jlimit (0, 4, juce::roundToInt (get (v, "shape"))); };
    int skipped = 0;

    if (k == "removeHum")
    {
        out.snapshot = Restoration::removeHum (s, start, end, rate, { get (v, "frequency"), juce::roundToInt (get (v, "harmonics")), get (v, "q") });
    }
    else if (k == "declip")
    {
        const auto r = Restoration::declip (s, start, end, rate, { get (v, "threshold") / 100.0, get (v, "keepPeak") > 0.5 });
        out.snapshot = r.audio;

        if (r.audio == nullptr)
            out.notes.add ("no clipping found to repair");
        else if (r.tooLong > 0)
            out.notes.add (juce::String (r.tooLong) + " clipped runs too long to rebuild");
    }
    else if (k == "gainRamp")       out.snapshot = AudioEdits::gainRamp (s, start, end, (float) get (v, "from"), (float) get (v, "to"));
    else if (k == "panRamp")        out.snapshot = AudioEdits::balanceRamp (s, start, end, (float) (get (v, "from") / 100.0), (float) (get (v, "to") / 100.0));
    else if (k == "fadeEdges")      out.snapshot = AudioEdits::fadeEdges (s, start, end, toFrames (get (v, "ms") / 1000.0), shapeOf());
    else if (k == "noiseGate")      out.snapshot = AudioEdits::gateQuiet (s, start, end, juce::Decibels::decibelsToGain ((float) get (v, "level"), -200.0f),
                                                                          toFrames (get (v, "length")), toFrames (0.01));
    else if (k == "silenceOutside") out.snapshot = AudioEdits::silenceOutside (s, start, end);
    else if (k == "silenceLeft")    out.snapshot = AudioEdits::silenceChannel (s, start, end, 0);
    else if (k == "silenceRight")   out.snapshot = AudioEdits::silenceChannel (s, start, end, 1);
    else if (k == "crossfadeLoop")  out.snapshot = AudioEdits::crossfadeLoop (s, start, end, toFrames (get (v, "ms") / 1000.0));
    else if (k == "clickRepair")
    {
        const auto r = ClickRepair::repair (s, start, end, rate, { (float) get (v, "threshold"), (float) get (v, "length"), get (v, "voice") > 0.5 }, keepGoing);

        if (! r.has_value())
        {
            out.error = "Stopped.";
            return out;
        }

        out.snapshot = r->snapshot;
        out.notes.add (juce::String (r->repaired) + " clicks repaired" + (r->skipped > 0 ? ", " + juce::String (r->skipped) + " left (too long)" : juce::String()));
    }
    else if (k == "adaptiveDenoise")
        out.snapshot = AdaptiveDenoise::reduce (s, start, end, rate, { (float) get (v, "reduction"), (float) get (v, "threshold"), (float) get (v, "tracking") }, keepGoing);
    else if (k == "dialogueIsolation")
        out.snapshot = DialogueIsolation::isolate (s, start, end, rate, { (float) get (v, "reduction"), (float) get (v, "sensitivity") }, keepGoing);
    else if (k == "spill")
        out.snapshot = Spill::apply (s, start, end, rate, { get (v, "wind"), get (v, "corner"), get (v, "rustle"), get (v, "bleed"), get (v, "bleedThreshold") }, keepGoing);
    else if (k == "wowFlutter")     out.snapshot = WowFlutter::correct (s, start, end, rate, { get (v, "fastest"), get (v, "amount") }, keepGoing);
    else if (k == "spectralRecovery") out.snapshot = SpectralRecovery::recover (s, start, end, rate, { get (v, "cutoff"), get (v, "level"), get (v, "highest") }, keepGoing);
    else if (k == "rebalance")
        out.snapshot = Rebalance::apply (s, start, end, rate, { get (v, "voice"), get (v, "bass"), get (v, "drums"), get (v, "rest"), get (v, "corner") }, keepGoing);
    else if (k == "azimuth")        out.snapshot = Azimuth::correct (s, start, end, rate, { get (v, "most"), get (v, "amount") }, keepGoing);
    else if (k == "dialogueContour")
        out.snapshot = DialogueContour::apply (s, start, end, rate, { (float) get (v, "semitones"), (float) get (v, "ending"), (float) get (v, "curve") }, keepGoing);
    else if (k == "graphicEq10")
    {
        std::vector<double> gains;

        for (auto* id : { "hz31", "hz63", "hz125", "hz250", "hz500", "hz1k", "hz2k", "hz4k", "hz8k", "hz16k" })
            gains.push_back (get (v, id));

        out.snapshot = GraphicEq::apply (s, start, end, rate, gains);
    }
    else if (k == "multibandCompressor")
    {
        Multiband::Compressor m;
        m.lowCrossover = get (v, "lowCrossover");
        m.highCrossover = get (v, "highCrossover");
        m.outputDb = get (v, "output");
        const std::array<const char*, 3> ids { "low", "mid", "high" };

        for (size_t b = 0; b < 3; ++b)
        {
            const juce::String id (ids[b]);
            auto& c = m.bands[b];
            c.thresholdDb = get (v, (id + "Threshold").toRawUTF8());
            c.ratio = get (v, (id + "Ratio").toRawUTF8());
            c.attackMs = get (v, (id + "Attack").toRawUTF8());
            c.releaseMs = get (v, (id + "Release").toRawUTF8());
            c.makeupDb = get (v, (id + "Makeup").toRawUTF8());
        }

        m = Multiband::spaced (m, rate);

        if (! Multiband::isValid (m, rate))
        {
            out.error = "a setting is out of range.";
            return out;
        }

        out.snapshot = Multiband::compress (s, start, end, rate, m);
    }
    else if (k == "normalizeRegions" || k == "fadeRegions" || k == "reverseRegions")
    {
        out.snapshot = k == "normalizeRegions" ? AudioEdits::normalizeRegions (s, markers, (float) get (v, "db"), skipped)
                     : k == "fadeRegions"      ? AudioEdits::fadeRegions (s, markers, toFrames (get (v, "ms") / 1000.0), shapeOf(), skipped)
                                               : AudioEdits::reverseRegions (s, markers, skipped);

        if (out.snapshot == nullptr)
            out.notes.add ("there are no regions to work on");
    }
    else if (k == "shortenPauses")
    {
        const auto keep = toFrames (get (v, "length"));
        const auto cuts = AudioEdits::pauseCuts (AudioEdits::pauses (s, juce::Decibels::decibelsToGain ((float) get (v, "level"), -200.0f),
                                                                     keep + 1, std::numeric_limits<size_t>::max()), keep);
        auto shortened = audio;

        for (auto cut = cuts.rbegin(); cut != cuts.rend(); ++cut)
        {
            shortened = AudioEdits::remove (*shortened, cut->getStart(), cut->getEnd());
            out.markers = MarkerEdits::afterReplace (out.markers, cut->getStart(), cut->getEnd(), 0);
        }

        if (cuts.empty())
            out.notes.add ("no pause longer than that");
        else
            out.snapshot = shortened;

        out.start = 0;
        out.end = shortened->getNumFrames();
    }
    else if (k == "noiseReduction" || k == "ambienceMatch")
    {
        const auto printStart = juce::jlimit ((juce::int64) 0, s.getNumFrames(), toFrames (get (v, "printStart")));
        const auto printEnd = juce::jlimit (printStart, s.getNumFrames(), toFrames (get (v, "printEnd")));

        if (k == "noiseReduction")
        {
            if (const auto print = NoiseReduction::capture (s, printStart, printEnd, rate))
                out.snapshot = NoiseReduction::reduce (s, start, end, rate, *print, { (float) get (v, "reduction"), (float) get (v, "threshold") }, keepGoing);
            else
                out.error = "the noise print's range is too short (it needs at least 4,096 frames of the noise alone).";
        }
        else
        {
            if (const auto print = Ambience::capture (s, printStart, printEnd, rate))
                out.snapshot = Ambience::apply (s, start, end, rate, *print, { (float) get (v, "level"), get (v, "fillQuiet") > 0.5 });
            else
                out.error = "the room tone's range is too short (it needs at least 4,096 frames of the room alone).";
        }

        if (out.error.isNotEmpty())
            return out;
    }
    else if (k == "spectralAttenuate" || k == "spectralRemove")
    {
        const auto db = k == "spectralRemove" ? -std::numeric_limits<float>::infinity() : (float) get (v, "db");
        out.snapshot = SpectralEdit::changeBand (s, start, end, { get (v, "low"), juce::jmin (get (v, "high"), rate / 2.0) }, rate, db);
    }
    else if (k == "generateTone" || k == "generateNoise")
    {
        // Inserted at the range's start, pushing what follows later.
        const auto length = (int) toFrames (get (v, "length"));
        juce::AudioBuffer<float> made;

        if (k == "generateTone")
            made = Generator::tone ({ (Generator::Wave) juce::jlimit (0, 3, juce::roundToInt (get (v, "wave"))), get (v, "frequency"), get (v, "endFrequency"),
                                      get (v, "level"), get (v, "fade") }, s.getNumChannels(), length, rate);
        else
            made = Generator::noise ({ (Generator::Colour) juce::jlimit (0, 2, juce::roundToInt (get (v, "colour"))), get (v, "level"), get (v, "fade"), 1 },
                                     s.getNumChannels(), length, rate);

        out.snapshot = Generator::insert (s, start, made);
        out.markers = MarkerEdits::afterReplace (markers, start, start, (juce::int64) made.getNumSamples());
        out.start = start;
        out.end = start + made.getNumSamples();
    }
    else if (k == "gain")                out.snapshot = AudioEdits::gain (s, start, end, (float) get (v, "db"));
    else if (k == "invert")         out.snapshot = AudioEdits::invert (s, start, end);
    else if (k == "reverse")        out.snapshot = AudioEdits::reverse (s, start, end);
    else if (k == "removeDc")       out.snapshot = AudioEdits::removeDcOffset (s, start, end);
    else if (k == "swapChannels")   out.snapshot = AudioEdits::swapChannels (s, start, end);
    else if (k == "invertLeft")     out.snapshot = AudioEdits::invertChannel (s, start, end, 0);
    else if (k == "invertRight")    out.snapshot = AudioEdits::invertChannel (s, start, end, 1);
    else if (k == "leftToRight")    out.snapshot = AudioEdits::copyChannel (s, start, end, 0, 1);
    else if (k == "rightToLeft")    out.snapshot = AudioEdits::copyChannel (s, start, end, 1, 0);
    else if (k == "eqBand")         out.snapshot = Equalizer::apply (s, start, end, rate, { bandOf (v) });
    else if (k == "compressor")     out.snapshot = Dynamics::compress (s, start, end, rate, compressorOf (v));
    else if (k == "limiter")        out.snapshot = Dynamics::limit (s, start, end, rate, limiterOf (v));
    else if (k == "deEsser")        out.snapshot = Multiband::deEss (s, start, end, rate, deEsserOf (v));
    else if (k == "multibandWidth") out.snapshot = Multiband::widen (s, start, end, rate, widthOf (v));
    else if (k == "exciter")        out.snapshot = Exciter::apply (s, start, end, rate, exciterOf (v));
    else if (k == "maximizer")      out.snapshot = Maximizer::apply (s, start, end, rate, maximizerOf (v));
    else if (k == "dither")         out.snapshot = Dither::apply (s, start, end, ditherOf (v));
    else if (k == "deplosive")      out.snapshot = Plosive::apply (s, start, end, rate, plosiveOf (v));
    else if (k == "breath")         out.snapshot = Breath::apply (s, start, end, rate, breathOf (v));
    else if (k == "transientShaper") out.snapshot = TransientShaper::apply (s, start, end, rate, transientShaperOf (v));
    else if (k == "chorus" || k == "flanger") out.snapshot = Modulation::sweptDelay (s, start, end, rate, delayOf (v));
    else if (k == "phaser")         out.snapshot = Modulation::phaser (s, start, end, rate, phaserOf (v));
    else if (k == "reverb")         out.snapshot = RoomReverb::apply (s, start, end, rate, reverbOf (v));
    else if (k == "levelSpeech")
    {
        if (end > start)
        {
            out.snapshot = Leveller::level (s, start, end, rate, levellerOf (v), progress);

            if (out.snapshot == nullptr)
            {
                out.error = "Stopped.";
                return out;
            }
        }
    }
    else if (k == "centre")
    {
        if (s.getNumChannels() != 2)
        {
            out.notes.add ("not stereo, so its centre wasn't changed");
        }
        else if (end > start && get (v, "reduction") > 0.0)
        {
            out.snapshot = CentreChannel::process (s, start, end, rate, centreOf (v), progress);

            if (out.snapshot == nullptr)
            {
                out.error = "Stopped.";
                return out;
            }
        }
    }
    else if (k == "pitchCorrect")
    {
        if (end > start && get (v, "strength") > 0.0)
        {
            out.snapshot = PitchCorrection::correct (s, start, end, rate, tuningOf (v), progress);

            if (out.snapshot == nullptr)
            {
                out.error = "Stopped.";
                return out;
            }
        }
    }
    else if (k == "phase")
    {
        const auto most = get (v, "most");
        const auto ms = [rate] (double frames) { return juce::String (std::abs (frames) * 1000.0 / rate, 3) + " ms (" + juce::String (std::abs (frames), 2) + " frames)"; };

        if (s.getNumChannels() != 2)
        {
            out.notes.add ("not stereo, so there were no channels to line up");
        }
        else if (end > start)
        {
            auto stopped = false;
            const auto found = PhaseCorrection::measure (s, start, end, rate, most, [&] (float p)
            {
                if (progress == nullptr || progress (p))
                    return true;

                stopped = true;
                return false;
            });

            if (stopped)
            {
                out.error = "Stopped.";
                return out;
            }

            if (! found.has_value() || std::abs (found->correlation) < PhaseCorrection::weakest)
            {
                out.notes.add ("no offset between the channels was found within " + juce::String (most, 1) + " ms (they may be unrelated, too short, "
                               "silent or further apart), so they were left as they were");
            }
            else
            {
                const auto upsideDown = found->correlation < 0.0;
                const auto turnOver = upsideDown && get (v, "turnOver") > 0.5;
                const auto moved = std::abs (found->offset) >= 0.01;

                out.notes.add (moved ? "the right channel was " + ms (found->offset) + (found->offset > 0.0 ? " late" : " early") + " on the left"
                                     : juce::String ("the channels were already in line"));

                if (upsideDown)
                    out.notes.add (turnOver ? juce::String ("the right channel was upside down, and was turned over")
                                            : juce::String ("the right channel looks upside down; it was left so"));

                out.notes.add ("correlation " + juce::String (std::abs (found->correlation), 2));

                if (moved || turnOver)
                    out.snapshot = PhaseCorrection::correct (s, start, end, rate, moved ? found->offset : 0.0, turnOver);
            }
        }
    }
    else if (k == "dereverb")
    {
        if (end > start && get (v, "reduction") > 0.0)
        {
            const auto set = (float) get (v, "time");
            auto used = set;
            out.snapshot = Dereverb::reduce (s, start, end, rate, { set, (float) get (v, "reduction") }, progress, &used);

            if (out.snapshot == nullptr)
            {
                out.error = "Stopped.";
                return out;
            }

            if (used < set - 0.005f)
                out.notes.add ("the recording's own sounds die away in about " + juce::String (used, 2) + " s, so the reverb time was taken as that, not "
                               + juce::String (set, 2) + " s (a longer time would cut into the sounds themselves)");
        }
    }
    else if (k == "normalize")
    {
        const auto db = juce::jmin ((float) get (v, "db"), 0.0f);
        out.snapshot = get (v, "apart") > 0.5 ? AudioEdits::normalizeChannels (s, start, end, db) : AudioEdits::normalize (s, start, end, db);

        if (out.snapshot == nullptr)
            out.notes.add ("silent, so not normalized");
    }
    else if (k == "balance")
    {
        if (const auto pan = juce::roundToInt (get (v, "pan")); pan != 0 && end > start)
            out.snapshot = AudioEdits::balance (s, start, end, (float) pan / 100.0f);
    }
    else if (k == "stereoWidth")
    {
        if (const auto width = juce::roundToInt (get (v, "width")); width != 100 && end > start)
            out.snapshot = AudioEdits::stereoWidth (s, start, end, (float) width / 100.0f);
    }
    else if (k == "echo")
    {
        const auto feedback = juce::roundToInt (get (v, "feedback"));
        const auto delay = (juce::int64) std::llround (juce::roundToInt (get (v, "delay")) / 1000.0 * rate);

        if (feedback > 0 && end > start)
            out.snapshot = AudioEdits::echo (s, start, end, delay, (float) feedback / 100.0f);
    }
    else if (k == "tremolo")
    {
        if (const auto depth = juce::roundToInt (get (v, "depth")); depth > 0 && end > start)
            out.snapshot = AudioEdits::tremolo (s, start, end, rate, get (v, "rate"), (float) depth / 100.0f);
    }
    else if (k == "mono" || k == "stereo")
    {
        const auto channels = k == "mono" ? 1 : 2;

        if (s.getNumFrames() > 0 && s.getNumChannels() != channels)
            out.snapshot = AudioEdits::convertChannels (s, channels);
    }
    else if (k == "deliverySpec" && end > start)
    {
        // As Meet Delivery Spec does it, and what the result measures against the spec, as notes.
        const auto spec = Delivery::specFor (get (v, "target"), get (v, "tolerance"), get (v, "ceiling"));
        auto d = Delivery::deliver (s, start, end, rate, spec, progress);

        if (d.error.isNotEmpty())
        {
            out.notes.add ("nothing above the -70 LUFS gate, so its loudness wasn't changed");
        }
        else if (d.snapshot == nullptr)
        {
            out.error = "Stopped.";
            return out;
        }
        else
        {
            out.snapshot = std::move (d.snapshot);

            if (d.limited)
                out.notes.add ("limited, with " + juce::String (d.gainDb, 1) + " dB of gain into the true-peak limiter");

            out.notes.add (Delivery::summary (spec, d.after));
        }
    }
    else if (k == "matchLoudness")
    {
        const auto target = get (v, "target"), ceiling = get (v, "ceiling");
        const auto measured = Loudness::measure (s, start, end, rate);

        if (! measured.has_value() || ! std::isfinite (measured->integrated))
        {
            out.notes.add ("nothing above the -70 LUFS gate, so its loudness wasn't changed");
        }
        else
        {
            const auto wanted = target - measured->integrated;
            const auto gain = juce::jmin (wanted, ceiling - measured->truePeak);

            if (get (v, "limit") > 0.5 && gain < wanted)
            {
                auto matched = Dynamics::matchLoudness (s, start, end, rate, measured->integrated, target, ceiling);
                out.snapshot = std::move (matched.snapshot);
                out.notes.add ("limited, with " + juce::String (matched.gainDb, 1) + " dB of gain into the limiter");

                if (std::abs (matched.integrated - target) > 0.05)
                    out.notes.add ("reached " + Loudness::format (matched.integrated, "LUFS"));
            }
            else
            {
                out.snapshot = AudioEdits::gain (s, start, end, (float) gain);

                if (gain < wanted - 0.05)
                    out.notes.add ("the true-peak ceiling allowed " + juce::String (gain, 1) + " dB, so it is at "
                                   + Loudness::format (measured->integrated + gain, "LUFS"));
            }
        }
    }
    else if (k == "extremeStretch")
    {
        if (end > start)
        {
            out.snapshot = ExtremeStretch::stretchRange (s, start, end, { get (v, "factor"), get (v, "window") }, rate, progress);

            if (out.snapshot == nullptr)
            {
                out.error = "Stopped.";
                return out;
            }

            const auto newEnd = end + out.snapshot->getNumFrames() - s.getNumFrames();
            out.markers = MarkerEdits::afterStretch (markers, start, end, newEnd - start);
            out.end = newEnd;
        }
    }
    else if (k == "stretch" || k == "pitch")
    {
        // A whole number of hundredths, as the dialog takes them; nothing to do at 100% or 0.
        const auto amount = std::round (get (v, k == "stretch" ? "percent" : "semitones") * 100.0) / 100.0;

        if (end > start && ! juce::exactlyEqual (amount, k == "stretch" ? 100.0 : 0.0))
        {
            // An optional setting: 1 for Torqwave's own stretch, else Signalsmith Stretch.
            const auto method = get (v, "method") > 0.5 ? TimeStretch::Method::torqwave : TimeStretch::Method::signalsmith;
            out.snapshot = k == "stretch" ? TimeStretch::stretchRange (s, start, end, amount / 100.0, rate, progress, method)
                                          : TimeStretch::shiftRange (s, start, end, amount, rate, progress, method);

            if (out.snapshot == nullptr)
            {
                out.error = "Stopped.";
                return out;
            }

            const auto newEnd = end + out.snapshot->getNumFrames() - s.getNumFrames();
            out.markers = MarkerEdits::afterStretch (markers, start, end, newEnd - start);
            out.end = newEnd;
        }
    }
    else if (k == "convertRate")
    {
        const auto newRate = get (v, "rate");

        if (! juce::exactlyEqual (newRate, rate))
        {
            out.snapshot = SampleRateConverter::convert (s, rate, newRate, progress);

            if (out.snapshot == nullptr)
            {
                out.error = "Stopped.";
                return out;
            }

            const auto scale = [&] (juce::int64 frame) { return (juce::int64) std::llround ((double) frame * newRate / rate); };
            out.markers = SampleRateConverter::convert (markers, rate, newRate);
            out.sampleRate = newRate;
            out.start = scale (start);
            out.end = scale (end);
        }
    }
    else if (k == "rack")
    {
        if (plugins == nullptr)
        {
            out.error = "Apply Effects Rack can't run here.";
            return out;
        }

        if (step.rack->getNumChildElements() == 0 || end <= start)
            return out;

        juce::AudioBuffer<float> buffer (s.getNumChannels(), (int) (end - start));
        s.read (buffer, 0, start, buffer.getNumSamples());

        if (const auto error = OfflineRender::processThroughRack (*step.rack, *plugins, rate, buffer, progress); error.isNotEmpty())
        {
            out.error = error;
            return out;
        }

        AudioClip rendered { buffer.getNumChannels(), rate, AudioEdits::fromBuffer (std::move (buffer))->getSegments() };
        out.snapshot = AudioEdits::replaceWithClip (s, start, end, rendered);
    }

    return out;
}

Result run (const Sequence& sequence, const Snapshot& audio, juce::int64 start, juce::int64 end, double rate,
            const Markers& markers, juce::AudioPluginFormatManager* plugins, const std::function<bool (float)>& progress)
{
    Result out;
    out.snapshot = audio;
    out.sampleRate = rate;
    out.markers = markers;
    out.start = start;
    out.end = end;
    const auto count = (float) juce::jmax ((size_t) 1, sequence.steps.size());

    for (size_t i = 0; i < sequence.steps.size(); ++i)
    {
        const auto& step = sequence.steps[i];
        const auto stepProgress = [&progress, i, count] (float p) { return progress == nullptr || progress (((float) i + p) / count); };

        // Stopped or refused, nothing is changed.
        const auto fail = [&] (const juce::String& error)
        {
            Result failed;
            failed.snapshot = audio;
            failed.sampleRate = rate;
            failed.markers = markers;
            failed.start = start;
            failed.end = end;
            failed.error = error;
            return failed;
        };

        if (! stepProgress (0.0f))
            return fail ("Stopped.");

        auto r = apply (step, out.snapshot, out.start, out.end, out.sampleRate, out.markers, plugins, stepProgress);

        if (r.error.isNotEmpty())
            return fail (r.error == "Stopped." ? r.error : step.name() + ": " + r.error);

        for (auto& n : r.notes)
            out.notes.add (step.name() + ": " + n);

        if (r.snapshot != nullptr)
            out.snapshot = std::move (r.snapshot);

        out.sampleRate = r.sampleRate;
        out.markers = std::move (r.markers);
        out.start = r.start;
        out.end = r.end;
    }

    return out;
}

std::unique_ptr<juce::XmlElement> toXml (const Sequence& sequence)
{
    auto xml = std::make_unique<juce::XmlElement> ("TORQWAVE_SEQUENCE");
    xml->setAttribute ("version", 1);
    xml->setAttribute ("name", sequence.name);

    for (auto& step : sequence.steps)
    {
        auto* e = xml->createNewChildElement ("STEP");
        e->setAttribute ("kind", step.kind);

        for (auto& [id, value] : step.values)
        {
            auto* value_ = e->createNewChildElement ("VALUE");
            value_->setAttribute ("id", id);
            value_->setAttribute ("value", value);
        }

        if (step.rack != nullptr)
            e->addChildElement (new juce::XmlElement (*step.rack));
    }

    return xml;
}

std::optional<Sequence> fromXml (const juce::XmlElement& xml)
{
    if (! PreviousName::hasTag (xml, "TORQWAVE_SEQUENCE"))
        return std::nullopt;

    Sequence sequence;
    sequence.name = xml.getStringAttribute ("name");

    for (auto* e : xml.getChildWithTagNameIterator ("STEP"))
    {
        Step step;
        step.kind = e->getStringAttribute ("kind");

        if (find (step.kind) == nullptr)
            return std::nullopt;

        for (auto* value : e->getChildWithTagNameIterator ("VALUE"))
            step.values[value->getStringAttribute ("id")] = value->getDoubleAttribute ("value");

        if (auto* rack = PreviousName::getChild (*e, "TORQWAVE_RACK"))
            step.rack = std::make_shared<const juce::XmlElement> (*rack);

        sequence.steps.push_back (std::move (step));
    }

    return sequence;
}

Store::Store (juce::File f) : folder (std::move (f)) {}

juce::File Store::fileFor (const juce::String& name) const
{
    return folder.getChildFile (juce::File::createLegalFileName (name) + ".sbsequence");
}

juce::StringArray Store::names() const
{
    juce::StringArray list;

    for (auto& f : folder.findChildFiles (juce::File::findFiles, false, "*.sbsequence"))
        if (auto xml = juce::XmlDocument::parse (f))
            if (auto s = fromXml (*xml); s.has_value() && s->name.isNotEmpty())
                list.addIfNotAlreadyThere (s->name);

    list.sortNatural();
    return list;
}

std::optional<Sequence> Store::load (const juce::String& name) const
{
    if (auto xml = juce::XmlDocument::parse (fileFor (name)))
        return fromXml (*xml);

    return std::nullopt;
}

bool Store::contains (const juce::String& name) const    { return fileFor (name).existsAsFile(); }

bool Store::save (const Sequence& sequence)
{
    if (sequence.name.trim().isEmpty() || ! folder.createDirectory())
        return false;

    return toXml (sequence)->writeTo (fileFor (sequence.name));
}

bool Store::remove (const juce::String& name)    { return fileFor (name).deleteFile(); }

Store& userStore()
{
    static Store store (juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                            .getChildFile ("Torqwave").getChildFile ("Sequences"));
    return store;
}

void Recorder::start()
{
    recording = true;
    recorded.clear();
    notRecorded.clear();
}

void Recorder::add (Step step)
{
    if (recording)
        recorded.push_back (std::move (step));
}

void Recorder::skip (const juce::String& edit)
{
    if (recording)
        notRecorded.add (edit);
}

Sequence Recorder::stop()
{
    recording = false;
    return { {}, recorded };
}
}
