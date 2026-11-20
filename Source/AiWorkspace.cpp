#include "AiWorkspace.h"
#include "AudioEdits.h"
#include "FadeShape.h"
#include "Loudness.h"
#include "MatchEq.h"
#include "Sequences.h"

namespace Ai
{
namespace
{
    using Snapshot = AudioDocument::Snapshot;

    //==============================================================================
    // The effects the edit tool runs, through Sequences (the code the menu commands run).
    // Settings are in the commands' dialog units, with the dialogs' ranges; a missing setting
    // takes the default here.
    struct Setting
    {
        const char* id;
        const char* meaning;
        double low, high, fallback;
    };

    struct Effect
    {
        const char* kind;
        const char* meaning;
        std::vector<Setting> settings;
    };

    const std::vector<Effect>& effects()
    {
        static const std::vector<Effect> list {
            { "gain", "Change the level", { { "db", "gain in dB", -48, 48, 0 } } },
            { "normalize", "Scale so the peak is at a level", { { "db", "peak level in dBFS", -48, 0, -1 },
                                                                { "apart", "0: channels together, 1: each channel on its own", 0, 1, 0 } } },
            { "matchLoudness", "Set the integrated loudness (EBU R128)", { { "target", "target in LUFS", -70, 0, -16 },
                                                                           { "ceiling", "true peak ceiling in dBTP", -30, 0, -1 },
                                                                           { "limit", "0: stop short when the ceiling stops the gain, 1: reach the target with a true-peak limiter", 0, 1, 0 } } },
            { "deliverySpec", "Meet a delivery spec: integrated loudness within a tolerance of a target and the true peak under a ceiling (EBU R128: -23, 0.5, -1; ATSC A/85: -24, 2, -2; online: -16 or -14, 1, -1)",
              { { "target", "LUFS", -70, 0, -23 }, { "tolerance", "LU", 0.1, 10, 0.5 }, { "ceiling", "dBTP", -30, 0, -1 } } },
            { "dither", "Dither to a bit depth: the last step before saving at that depth", { { "bits", "0: 16 bits, 1: 24 bits", 0, 1, 0 },
                                                                                               { "shaping", "noise shaping: 0 none, 1 light, 2 strong", 0, 2, 1 } } },
            { "invert", "Invert the polarity", {} },
            { "reverse", "Play backwards", {} },
            { "removeDc", "Remove DC offset", {} },
            { "swapChannels", "Swap left and right", {} },
            { "invertLeft", "Invert the polarity of the left channel only (stereo)", {} },
            { "invertRight", "Invert the polarity of the right channel only (stereo)", {} },
            { "leftToRight", "Copy the left channel over the right (stereo)", {} },
            { "rightToLeft", "Copy the right channel over the left (stereo)", {} },
            { "mono", "Mix the whole file to mono", {} },
            { "stereo", "Make the whole file stereo", {} },
            { "balance", "Balance between channels", { { "pan", "-100 (left) to 100 (right)", -100, 100, 0 } } },
            { "stereoWidth", "Stereo width", { { "width", "% (0 mono, 100 unchanged, 200 widest)", 0, 200, 100 } } },
            { "multibandWidth", "Stereo width in three bands", { { "lowCrossover", "Hz", 20, 10000, 200 }, { "highCrossover", "Hz", 40, 20000, 4000 },
                                                                 { "low", "% for the lows", 0, 200, 100 }, { "mid", "% for the middle", 0, 200, 100 },
                                                                 { "high", "% for the highs", 0, 200, 100 } } },
            { "transientShaper", "Transient shaper in three bands: each band's attacks and sustain turned up or down",
              { { "lowCrossover", "Hz", 20, 10000, 200 }, { "highCrossover", "Hz, at least an octave above the low", 40, 20000, 3000 },
                { "lowAttack", "dB for the lows' attacks", -12, 12, 0 }, { "lowSustain", "dB for the lows' sustain", -12, 12, 0 },
                { "midAttack", "dB for the middle's attacks", -12, 12, 0 }, { "midSustain", "dB for the middle's sustain", -12, 12, 0 },
                { "highAttack", "dB for the highs' attacks", -12, 12, 0 }, { "highSustain", "dB for the highs' sustain", -12, 12, 0 },
                { "output", "dB", -24, 24, 0 }, { "linearPhase", "0: minimum-phase band split, 1: linear phase (peaks held)", 0, 1, 0 } } },
            { "exciter", "Exciter (saturation) in three bands, each driven into a curve and mixed back",
              { { "lowCrossover", "Hz", 20, 10000, 200 }, { "highCrossover", "Hz, at least an octave above the low", 40, 20000, 3000 },
                { "lowDrive", "dB into the curve for the lows (0 leaves them)", 0, 36, 0 }, { "lowCurve", "0 tube, 1 tape, 2 transistor", 0, 2, 0 }, { "lowMix", "% of the shaped lows", 0, 100, 100 },
                { "midDrive", "dB for the middle", 0, 36, 0 }, { "midCurve", "0 tube, 1 tape, 2 transistor", 0, 2, 0 }, { "midMix", "% of the shaped middle", 0, 100, 100 },
                { "highDrive", "dB for the highs", 0, 36, 0 }, { "highCurve", "0 tube, 1 tape, 2 transistor", 0, 2, 0 }, { "highMix", "% of the shaped highs", 0, 100, 100 },
                { "output", "dB", -24, 24, 0 }, { "linearPhase", "0: minimum-phase band split, 1: linear phase (peaks held)", 0, 1, 0 } } },
            { "echo", "Echo", { { "delay", "ms", 1, 5000, 250 }, { "feedback", "%", 0, 95, 30 } } },
            { "tremolo", "Tremolo", { { "rate", "Hz", 0.1, 50, 5 }, { "depth", "%", 0, 100, 50 } } },
            { "eqBand", "One EQ band", { { "type", "0 bell, 1 low shelf, 2 high shelf, 3 high-pass, 4 low-pass", 0, 4, 0 },
                                         { "frequency", "Hz", 20, 20000, 1000 }, { "gain", "dB (bell and shelves)", -48, 48, 0 },
                                         { "q", "Q (0.71 is a gentle slope)", 0.05, 50, 0.71 } } },
            { "compressor", "Compressor", { { "threshold", "dB", -80, 0, -20 }, { "ratio", "to 1", 1, 100, 4 }, { "knee", "dB", 0, 24, 6 },
                                            { "attack", "ms", 0.01, 1000, 10 }, { "release", "ms", 1, 5000, 100 }, { "makeup", "dB", -48, 48, 0 } } },
            { "limiter", "Limiter", { { "gain", "dB before the limiter", -48, 48, 0 }, { "ceiling", "dBFS", -48, 0, -1 }, { "release", "ms", 1, 5000, 50 },
                                      { "lookahead", "ms", 0.1, 50, 5 }, { "peaks", "0 sample peaks, 1 true peaks", 0, 1, 1 } } },
            { "maximizer", "Loudness maximizer", { { "gain", "dB", 0, 24, 3 }, { "ceiling", "dBTP", -24, 0, -1 },
                                                   { "character", "0 transparent, 1 balanced, 2 loud", 0, 2, 1 },
                                                   { "emphasis", "transient emphasis %", 0, 100, 0 } } },
            { "deEsser", "De-esser", { { "frequency", "Hz", 1000, 16000, 6000 }, { "threshold", "dB", -60, 0, -30 }, { "reduction", "dB", 0, 24, 6 },
                                       { "mode", "0 turn down the high band, 1 the whole sound", 0, 1, 0 } } },
            { "deplosive", "De-plosive (pops on p and b)", { { "corner", "Hz", 60, 400, 150 }, { "threshold", "dB over the typical lows", 3, 30, 10 },
                                                             { "reduction", "dB", 0, 40, 12 } } },
            { "breath", "Breath control", { { "threshold", "dB under the speech", 6, 40, 20 }, { "reduction", "dB", 0, 40, 12 } } },
            { "levelSpeech", "Level speech to an even loudness", { { "target", "LUFS", -40, -5, -20 }, { "boost", "largest boost dB", 0, 30, 12 },
                                                                   { "cut", "largest cut dB", 0, 30, 12 }, { "silence", "silence below, LUFS", -80, -20, -50 },
                                                                   { "speed", "fastest change dB/s", 1, 30, 30 } } },
            { "dereverb", "Reduce reverb", { { "time", "the room's reverb time in s", 0.1, 10, 1 }, { "reduction", "dB", 0, 30, 12 } } },
            { "chorus", "Chorus", { { "rate", "Hz", 0.01, 10, 0.8 }, { "delay", "ms", 0, 50, 20 }, { "depth", "ms", 0, 50, 5 },
                                    { "feedback", "-0.95 to 0.95", -0.95, 0.95, 0 }, { "mix", "%", 0, 100, 50 }, { "spread", "stereo degrees", 0, 180, 90 } } },
            { "flanger", "Flanger", { { "rate", "Hz", 0.01, 10, 0.25 }, { "delay", "ms", 0, 50, 2 }, { "depth", "ms", 0, 50, 2 },
                                      { "feedback", "-0.95 to 0.95", -0.95, 0.95, 0.5 }, { "mix", "%", 0, 100, 50 }, { "spread", "stereo degrees", 0, 180, 90 } } },
            { "phaser", "Phaser", { { "rate", "Hz", 0.01, 10, 0.5 }, { "low", "Hz", 20, 20000, 200 }, { "high", "Hz", 20, 20000, 4000 },
                                    { "stages", "0 to 5 for 2, 4, 6, 8, 10 or 12 stages", 0, 5, 1 }, { "feedback", "-0.95 to 0.95", -0.95, 0.95, 0.5 },
                                    { "mix", "%", 0, 100, 50 }, { "spread", "stereo degrees", 0, 180, 90 } } },
            { "reverb", "Reverb", { { "preDelay", "ms", 0, 200, 20 }, { "decay", "s", 0.1, 20, 1.5 }, { "size", "%", 50, 150, 100 },
                                    { "damping", "%", 0, 100, 50 }, { "width", "%", 0, 100, 100 }, { "mix", "%", 0, 100, 25 } } },
            { "centre", "Remove the centre of a stereo mix (what is the same in both channels, such as a lead vocal) between two frequencies, or keep it alone",
              { { "keep", "0: remove the centre, 1: keep the centre alone", 0, 1, 0 }, { "low", "Hz", 20, 20000, 100 }, { "high", "Hz, above low", 20, 20000, 8000 },
                { "reduction", "dB", 0, 40, 20 } } },
            { "phase", "Automatic phase correction of a stereo file: line up a channel that lags the other, and turn over one found upside down",
              { { "most", "the most offset looked for, ms", 0.1, 50, 5 }, { "turnOver", "1: turn over a channel found upside down, 0: leave it", 0, 1, 1 } } },
            { "extremeStretch", "Extreme stretch: many times longer, smeared into a texture (changes the file's length)",
              { { "factor", "times as long", 1, 1000, 8 }, { "window", "s", 0.05, 2, 0.25 } } },
            { "pitchCorrect", "Pitch correction of a solo voice or instrument to a key's notes",
              { { "key", "0 C, 1 C#, 2 D, 3 D#, 4 E, 5 F, 6 F#, 7 G, 8 G#, 9 A, 10 A#, 11 B", 0, 11, 0 },
                { "scale", "0 chromatic (every note), 1 major, 2 minor", 0, 2, 0 }, { "speed", "ms to reach the note (0 instant)", 0, 500, 20 },
                { "strength", "%", 0, 100, 100 } } },
            { "removeHum", "Remove mains hum and its harmonics with narrow notches", { { "frequency", "Hz: 50 or 60, or any hum", 20, 1000, 50 },
                                                                                        { "harmonics", "how many multiples to notch", 1, 40, 8 },
                                                                                        { "q", "notch narrowness (5 wide, 200 narrow)", 5, 200, 30 } } },
            { "declip", "Repair clipping: rebuild flattened peaks", { { "threshold", "% of the peak counted as clipped", 50, 100, 99 },
                                                                      { "keepPeak", "1: scale so the rebuilt peaks reach the old level, 0: let them rise above it", 0, 1, 1 } } },
            { "clickRepair", "Click repair: remove clicks, crackle and mouth clicks", { { "threshold", "dB a click stands above the background (lower finds more)", 3, 60, 18 },
                                                                                        { "length", "ms: longer events are left", 0.1, 20, 3 },
                                                                                        { "voice", "1 for speech (mouth de-click), 0 otherwise", 0, 1, 0 } } },
            { "noiseReduction", "Noise reduction from a print: a range of the noise alone (printStart to printEnd, seconds) is learnt, then lowered in the range",
              { { "printStart", "s", 0, 1.0e7, 0 }, { "printEnd", "s, at least about 0.1 s after printStart", 0, 1.0e7, 0.5 },
                { "reduction", "dB", 0, 60, 12 }, { "threshold", "dB above the print counted as signal", -10, 20, 3 } } },
            { "adaptiveDenoise", "Adaptive noise reduction: follows changing noise with no print", { { "reduction", "dB", 0, 40, 12 },
                                                                                                    { "threshold", "dB above the tracked noise counted as signal", 0, 12, 3 },
                                                                                                    { "tracking", "s", 0.5, 5, 1.5 } } },
            { "dialogueIsolation", "Lower everything that isn't speech", { { "reduction", "dB", 0, 40, 12 }, { "sensitivity", "how readily sound counts as speech, %", 0, 100, 50 } } },
            { "spill", "Lower wind, clothing rustle and bleed under speech", { { "wind", "dB", 0, 40, 12 }, { "corner", "wind's top, Hz", 60, 400, 150 },
                                                                               { "rustle", "dB", 0, 40, 12 }, { "bleed", "dB", 0, 40, 12 },
                                                                               { "bleedThreshold", "dB under the speech that counts as bleed", 6, 40, 20 } } },
            { "ambienceMatch", "Fill gaps and quiet places with room tone learnt from a range of the room alone (printStart to printEnd, seconds)",
              { { "printStart", "s", 0, 1.0e7, 0 }, { "printEnd", "s", 0, 1.0e7, 0.5 }, { "level", "dB against the print", -24, 12, 0 },
                { "fillQuiet", "1: only where quieter than the print, 0: add everywhere", 0, 1, 1 } } },
            { "wowFlutter", "Correct tape or vinyl wow and flutter (slow pitch drift)", { { "fastest", "Hz: faster drift is left (vibrato)", 0.5, 20, 4 },
                                                                                          { "amount", "%", 0, 100, 100 } } },
            { "azimuth", "Azimuth correction: line up a stereo tape's channels as they drift", { { "most", "ms looked for", 0.5, 50, 5 }, { "amount", "%", 0, 100, 100 } } },
            { "spectralRecovery", "Rebuild highs lost to a low-pass (old MP3s, phone lines)", { { "cutoff", "Hz where the highs stop (0: found from the audio, else 1000 to 16000)", 0, 16000, 0 },
                                                                                                { "level", "dB against the continued slope", -24, 6, -3 },
                                                                                                { "highest", "Hz rebuilt up to", 2000, 20000, 16000 } } },
            { "rebalance", "Rebalance a mix: raise or lower the voice, bass, drums and the rest", { { "voice", "dB", -40, 12, 0 }, { "bass", "dB", -40, 12, 0 },
                                                                                                    { "drums", "dB", -40, 12, 0 }, { "rest", "dB", -40, 12, 0 },
                                                                                                    { "corner", "bass's top, Hz", 60, 300, 150 } } },
            { "dialogueContour", "Reshape the pitch at the end of each spoken phrase (turn a question into a statement)",
              { { "semitones", "where the ending lands", -12, 12, -2 }, { "ending", "s of the phrase's end reshaped", 0.1, 1.5, 0.4 },
                { "curve", "-1 early, 0 straight, 1 late", -1, 1, 0 } } },
            { "spectralAttenuate", "Raise or lower one band of frequencies in the range", { { "low", "Hz", 0, 96000, 1000 }, { "high", "Hz, above low", 0, 96000, 2000 },
                                                                                             { "db", "dB", -60, 24, -12 } } },
            { "spectralRemove", "Remove one band of frequencies in the range entirely", { { "low", "Hz", 0, 96000, 1000 }, { "high", "Hz, above low", 0, 96000, 2000 } } },
            { "graphicEq10", "10-band graphic EQ (octaves from 31.5 Hz to 16 kHz)", { { "hz31", "dB", -12, 12, 0 }, { "hz63", "dB", -12, 12, 0 }, { "hz125", "dB", -12, 12, 0 }, { "hz250", "dB", -12, 12, 0 }, { "hz500", "dB", -12, 12, 0 }, { "hz1k", "dB", -12, 12, 0 }, { "hz2k", "dB", -12, 12, 0 }, { "hz4k", "dB", -12, 12, 0 }, { "hz8k", "dB", -12, 12, 0 }, { "hz16k", "dB", -12, 12, 0 } } },
            { "multibandCompressor", "Three-band compressor", { { "lowCrossover", "Hz", 20, 20000, 200 }, { "highCrossover", "Hz", 20, 20000, 2000 }, { "output", "dB", -24, 24, 0 },
                                                                 { "lowThreshold", "dBFS", -80, 0, -20 }, { "lowRatio", ":1", 1, 100, 2 }, { "lowAttack", "ms", 0.01, 1000, 10 },
                                                                 { "lowRelease", "ms", 1, 5000, 100 }, { "lowMakeup", "dB", -48, 48, 0 },
                                                                 { "midThreshold", "dBFS", -80, 0, -20 }, { "midRatio", ":1", 1, 100, 2 }, { "midAttack", "ms", 0.01, 1000, 10 },
                                                                 { "midRelease", "ms", 1, 5000, 100 }, { "midMakeup", "dB", -48, 48, 0 },
                                                                 { "highThreshold", "dBFS", -80, 0, -20 }, { "highRatio", ":1", 1, 100, 2 }, { "highAttack", "ms", 0.01, 1000, 10 },
                                                                 { "highRelease", "ms", 1, 5000, 100 }, { "highMakeup", "dB", -48, 48, 0 } } },
            { "noiseGate", "Silence everything quieter than a level for at least a length", { { "level", "dBFS", -120, 0, -60 }, { "length", "s", 0.001, 600, 0.5 } } },
            { "gainRamp", "Gain that changes evenly across the range", { { "from", "dB at the start", -96, 24, 0 }, { "to", "dB at the end", -96, 24, -6 } } },
            { "panRamp", "Pan (balance) that moves evenly across the range", { { "from", "-100 left to 100 right", -100, 100, -100 }, { "to", "-100 to 100", -100, 100, 100 } } },
            { "fadeEdges", "Short fades at both edges of the range (stops clicks at the edits)", { { "ms", "ms", 0.1, 10000, 10 },
                                                                                                   { "shape", "0 S-curve, 1 linear, 2 equal power, 3 slow, 4 fast", 0, 4, 0 } } },
            { "crossfadeLoop", "Crossfade the range's end into its start so it loops without a click", { { "ms", "ms", 0.1, 10000, 50 } } },
            { "silenceOutside", "Silence everything outside the range", {} },
            { "silenceLeft", "Silence the left channel in the range", {} },
            { "silenceRight", "Silence the right channel in the range", {} },
            { "normalizeRegions", "Normalize each region on its own (the range is ignored)", { { "db", "peak in dBFS", -48, 0, -1 } } },
            { "fadeRegions", "Fade in and out at each region's edges (the range is ignored)", { { "ms", "ms", 0.1, 10000, 10 },
                                                                                               { "shape", "0 S-curve, 1 linear, 2 equal power, 3 slow, 4 fast", 0, 4, 0 } } },
            { "reverseRegions", "Reverse each region in place (the range is ignored)", {} },
            { "shortenPauses", "Shorten every pause in the whole file (quieter than a level) to a length (changes the file's length; the range is ignored)",
              { { "level", "dBFS", -120, 0, -50 }, { "length", "s each pause is left at", 0.001, 600, 0.5 } } },
            { "generateTone", "Insert a tone or sweep at the range's start (changes the file's length)",
              { { "wave", "0 sine, 1 square, 2 triangle, 3 sawtooth", 0, 3, 0 }, { "frequency", "Hz", 1, 20000, 1000 }, { "endFrequency", "Hz (a sweep when it differs)", 1, 20000, 1000 },
                { "level", "peak dBFS", -96, 0, -12 }, { "length", "s", 0.001, 600, 1 }, { "fade", "ms", 0, 1000, 5 } } },
            { "generateNoise", "Insert noise at the range's start (changes the file's length)",
              { { "colour", "0 white, 1 pink, 2 brown", 0, 2, 0 }, { "level", "RMS dBFS (at most -6 white, -20 pink or brown)", -96, -6, -20 },
                { "length", "s", 0.001, 600, 1 }, { "fade", "ms", 0, 1000, 5 } } },
            { "stretch", "Change the length, keeping the pitch", { { "percent", "the new length in % of now", 50, 200, 100 } } },
            { "pitch", "Change the pitch, keeping the length", { { "semitones", "semitones", -12, 12, 0 } } },
        };
        return list;
    }

    const Effect* effectFor (const juce::String& kind)
    {
        for (auto& e : effects())
            if (kind == e.kind)
                return &e;

        return nullptr;
    }

    //==============================================================================
    juce::var object (std::initializer_list<std::pair<const char*, juce::var>> fields)
    {
        auto o = std::make_unique<juce::DynamicObject>();

        for (auto& [name, value] : fields)
            o->setProperty (name, value);

        return juce::var (o.release());
    }

    juce::var failure (const juce::String& error)    { return object ({ { "ok", false }, { "error", error } }); }

    double round (double x, int places)
    {
        const auto scale = std::pow (10.0, places);
        return std::round (x * scale) / scale;
    }

    juce::var decibels (double x)    { return std::isfinite (x) ? juce::var (round (x, 2)) : juce::var ("-inf"); }

    bool isNumber (const juce::var& v)   { return v.isInt() || v.isInt64() || v.isDouble(); }

    //==============================================================================
    // The tool definitions, as JSON. Times are seconds from the start of the file.
    juce::String effectCatalogue()
    {
        juce::String text;

        for (auto& e : effects())
        {
            text << e.kind << ": " << e.meaning;

            if (! e.settings.empty())
            {
                juce::StringArray parts;

                for (auto& s : e.settings)
                    parts.add (juce::String (s.id) + " (" + s.meaning + ", " + juce::String (s.low) + " to " + juce::String (s.high)
                               + ", default " + juce::String (s.fallback) + ")");

                text << ". Settings: " << parts.joinIntoString ("; ");
            }

            text << ".\n";
        }

        return text;
    }
}

//==============================================================================
juce::var Workspace::toolDefinitions()
{
    static const juce::var tools = []
    {
        const auto range = R"("start": { "type": "number", "description": "seconds; leave out start and end for the selection, or the whole file without one" },
                              "end": { "type": "number", "description": "seconds" })";

        juce::String json = R"([
  { "name": "get_state", "description": "The file: name, sample rate, channels, length, the selection, markers and the effect chain's size. Call this first.",
    "parameters": { "type": "object", "properties": {} } },
  { "name": "measure", "description": "Measure a range: integrated loudness (LUFS), loudness range (LU), maximum short-term loudness, true peak (dBTP), sample peak, and each channel's peak, RMS and DC offset, and the stereo correlation.",
    "parameters": { "type": "object", "properties": { RANGE } } },
  { "name": "spectrum", "description": "The long-term spectrum of a range: the level in dBFS of each octave band from 31.5 Hz to 16 kHz.",
    "parameters": { "type": "object", "properties": { RANGE } } },
  { "name": "find", "description": "Find sounds (stretches above the threshold between quiet stretches), silences (quiet stretches with sound on both sides) or clipping. Returns their times, at most 100.",
    "parameters": { "type": "object", "properties": {
      "what": { "type": "string", "enum": ["sounds", "silences", "clipping"] },
      "threshold_db": { "type": "number", "description": "dBFS; default -50 for sounds and silences, -0.1 for clipping" },
      "min_seconds": { "type": "number", "description": "shortest quiet stretch that separates sounds or counts as a silence; default 0.3" } },
      "required": ["what"] } },
  { "name": "list_markers", "description": "The markers and regions, with their numbers, names and times.",
    "parameters": { "type": "object", "properties": {} } },
  { "name": "select", "description": "Set the selection that later tools work on when they are given no times. Give start and end, or all: true for the whole file, or none: true to clear it.",
    "parameters": { "type": "object", "properties": { RANGE, "all": { "type": "boolean" }, "none": { "type": "boolean" } } } },
  { "name": "edit", "description": "Run one of Torqwave's effects or edits on a range. Settings not given take their defaults. The kinds:\nEFFECTS",
    "parameters": { "type": "object", "properties": {
      "kind": { "type": "string", "enum": [KINDS] },
      "settings": { "type": "object", "description": "the kind's settings by name, as numbers", "additionalProperties": { "type": "number" } },
      RANGE },
      "required": ["kind"] } },
  { "name": "fade", "description": "Fade in at the start of a range, or out at its end.",
    "parameters": { "type": "object", "properties": {
      "direction": { "type": "string", "enum": ["in", "out"] },
      "seconds": { "type": "number", "description": "the fade's length; default the whole range" },
      "shape": { "type": "string", "enum": ["s-curve", "linear", "equal-power", "slow", "fast"], "description": "default s-curve" },
      RANGE },
      "required": ["direction"] } },
  { "name": "trim_silence", "description": "Cut the quiet ends off the file: everything before the first and after the last moment above the threshold.",
    "parameters": { "type": "object", "properties": { "threshold_db": { "type": "number", "description": "dBFS; default -60" } } } },
  { "name": "delete", "description": "Delete a range: the audio after it moves earlier.",
    "parameters": { "type": "object", "properties": { RANGE } } },
  { "name": "crop", "description": "Keep only a range and delete the rest.",
    "parameters": { "type": "object", "properties": { RANGE } } },
  { "name": "silence", "description": "Replace a range with silence, keeping its length.",
    "parameters": { "type": "object", "properties": { RANGE } } },
  { "name": "insert_silence", "description": "Insert silence at a time.",
    "parameters": { "type": "object", "properties": { "at": { "type": "number", "description": "seconds" }, "seconds": { "type": "number" } }, "required": ["at", "seconds"] } },
  { "name": "add_marker", "description": "Add a marker at a time, or a region from start to end.",
    "parameters": { "type": "object", "properties": { RANGE, "name": { "type": "string" } }, "required": ["start"] } },
  { "name": "rename_marker", "description": "Rename a marker or region by its number from list_markers.",
    "parameters": { "type": "object", "properties": { "number": { "type": "integer" }, "name": { "type": "string" } }, "required": ["number", "name"] } },
  { "name": "delete_marker", "description": "Delete a marker or region by its number from list_markers.",
    "parameters": { "type": "object", "properties": { "number": { "type": "integer" } }, "required": ["number"] } },
  { "name": "list_plugins", "description": "The plug-ins Torqwave knows: its built-in effects and the AU, VST3 and LV2 plug-ins on this computer, with a summary of what each does when the plug-in catalogue has one. Search by name, maker, category or summary.",
    "parameters": { "type": "object", "properties": { "search": { "type": "string" }, "instruments": { "type": "boolean", "description": "true for instruments only; default effects only" } } } },
  { "name": "describe_plugin", "description": "What the plug-in catalogue knows of a plug-in, without opening it: what it does and is for, its parameters (name, unit, range or choices, default) and its presets' names. Use it to choose a plug-in before adding one.",
    "parameters": { "type": "object", "properties": { "plugin": { "type": "string", "description": "its id or name from list_plugins" } }, "required": ["plugin"] } },
  { "name": "list_chain", "description": "The effect chain (the effects rack): each slot's number, plug-in and whether it is bypassed.",
    "parameters": { "type": "object", "properties": {} } },
  { "name": "add_plugin", "description": "Add an effect plug-in to the chain, by its id or name from list_plugins. Returns its slot number.",
    "parameters": { "type": "object", "properties": { "plugin": { "type": "string" }, "position": { "type": "integer", "description": "slot number to insert at; default the end" } }, "required": ["plugin"] } },
  { "name": "get_parameters", "description": "A slot's parameters: name, value as the plug-in shows it, and the choices for a switch or menu.",
    "parameters": { "type": "object", "properties": { "slot": { "type": "integer" }, "search": { "type": "string", "description": "only parameters whose name contains this" } }, "required": ["slot"] } },
  { "name": "set_parameters", "description": "Set a slot's parameters by name. Write each value as the plug-in shows it (\"-12 dB\", \"4:1\", \"2.5 kHz\") or as a choice's name. Returns what the plug-in took.",
    "parameters": { "type": "object", "properties": { "slot": { "type": "integer" },
      "values": { "type": "object", "description": "parameter name: value", "additionalProperties": { "type": ["string", "number", "boolean"] } } },
      "required": ["slot", "values"] } },
  { "name": "bypass_plugin", "description": "Bypass a slot, or turn it back on.",
    "parameters": { "type": "object", "properties": { "slot": { "type": "integer" }, "bypassed": { "type": "boolean" } }, "required": ["slot", "bypassed"] } },
  { "name": "move_plugin", "description": "Move a slot to another place in the chain.",
    "parameters": { "type": "object", "properties": { "slot": { "type": "integer" }, "to": { "type": "integer" } }, "required": ["slot", "to"] } },
  { "name": "remove_plugin", "description": "Remove a slot from the chain.",
    "parameters": { "type": "object", "properties": { "slot": { "type": "integer" } }, "required": ["slot"] } },
  { "name": "apply_chain", "description": "Render a range through the chain's plug-ins into the audio, then empty the chain (as Apply Effects Rack does), so they aren't heard twice.",
    "parameters": { "type": "object", "properties": { RANGE } } }
])";

        juce::StringArray kinds;

        for (auto& e : effects())
            kinds.add (juce::String (e.kind).quoted());

        json = json.replace ("RANGE", range)
                   .replace ("KINDS", kinds.joinIntoString (", "))
                   .replace ("EFFECTS", juce::JSON::toString (effectCatalogue()).unquoted());

        juce::var parsed;
        const auto result = juce::JSON::parse (json, parsed);
        jassert (result.wasOk());
        juce::ignoreUnused (result);

        juce::Array<juce::var> list;

        if (auto* functions = parsed.getArray())
            for (auto& f : *functions)
                list.add (object ({ { "type", "function" }, { "function", f } }));

        return juce::var (list);
    }();

    return tools;
}

juce::String Workspace::instructions()
{
    return "You are the assistant inside Torqwave, an audio editor. The user asks for changes to the audio file open in it, "
           "and you make them with the tools. You work on a copy: the user sees your changes as a plan and decides whether to "
           "apply it, as one undo step. So make the changes asked for, then stop and say in a sentence or two what the plan "
           "does. Times are in seconds from the start of the file. Look before you change: call get_state first, and measure "
           "or find to choose settings from the audio itself rather than guessing. A file can have silence around its sound: when "
           "the request speaks of the sound (its start, its end, its last second), find where the sound is first. Change only "
           "what was asked. If the request "
           "is unclear or can't be done with these tools, make no changes and say so. Plug-ins are set through their "
           "parameters; you can't see or use their windows, so say when a plug-in needs something only its window offers. To "
           "choose a plug-in, read the summaries list_plugins gives and describe_plugin, and add only the one you will use, "
           "rather than adding plug-ins to look at them. A summary that begins \"Not known:\" means the catalogue doesn't know "
           "what that plug-in does.";
}

juce::var Workspace::tools() const
{
    if (! isSession() && ! isSequence())
        return toolDefinitions();

    // The session's or sequence's tools, and the plug-in look tools so it can say what is there.
    auto list = isSequence() ? SequenceTools::toolDefinitions() : SessionTools::toolDefinitions();
    const auto fileTools = toolDefinitions();

    if (auto* file = fileTools.getArray())
        for (auto& t : *file)
            if (const auto name = t["function"]["name"].toString(); name == "list_plugins" || name == "describe_plugin")
                list.add (t);

    return juce::var (list);
}

juce::String Workspace::systemMessage() const
{
    if (isSequence())
    {
        const auto& c = sequenceTools->getContext();
        const auto& q = c.sequence;
        juce::String open;
        open << "The Sequencer has " << q.name.quoted() << " open: " << (q.role.isNotEmpty() ? "a " + q.role + " part" : juce::String ("a part"))
             << (q.subRole.isNotEmpty() ? " (" + q.subRole + ")" : juce::String()) << ", " << q.beatsPerBar << " beats to a bar, "
             << juce::String (q.bars(), 2).trimCharactersAtEnd ("0").trimCharactersAtEnd (".") << " bars long"
             << (q.key.isNotEmpty() ? ", in " + q.key : juce::String (", with no key set"))
             << ", played on " << (c.instrument.isNotEmpty() ? c.instrument : juce::String ("no instrument yet"))
             << (c.instrumentAbout.isNotEmpty() ? " (" + c.instrumentAbout.trimCharactersAtEnd (".") + ")" : juce::String())
             << " at " << c.tempo << " BPM"
             << (c.sessionTempo > 0.0 ? "; the open session's tempo is " + juce::String (c.sessionTempo) + " BPM" : juce::String()) << ". ";

        return "You are the assistant inside Torqwave, an audio editor with a Sequencer. The user asks you to write or look at the "
               "sequence open in it. You work on a copy: what you write is shown as a plan and goes in only when the user applies it, as "
               "one undo step; suggestions are shown for the user to apply. Change only what was asked. If the request is unclear or "
               "can't be done with these tools, make no changes and say so. "
               + open + SequenceTools::instructions();
    }

    if (! isSession())
        return instructions();

    return "You are the assistant inside Torqwave, an audio editor. The user asks for changes to the multitrack session open in it, "
           "and you make them with the tools. You work on a copy: the user sees your changes as a plan and decides whether to "
           "apply it, as one undo step. So make the changes asked for, then stop and say in a sentence or two what the plan does. "
           "Look before you change: call get_session first. Change only what was asked. If the request is unclear or can't be "
           "done with these tools, make no changes and say so. "
           + SessionTools::instructions();
}

//==============================================================================
Workspace::Workspace (Start s)
    : original (s.audio), audio (s.audio), sampleRate (s.sampleRate), selection (s.selection), markers (std::move (s.markers)),
      fileName (std::move (s.fileName)), plugins (std::move (s.plugins)), rackXml (std::move (s.rack)), formats (s.formats),
      catalogue (s.catalogue)
{
    if (s.session)
        sessionTools = std::make_unique<SessionTools> (std::move (*s.session), std::move (s.sessionName), steps, std::move (s.instruments),
                                                       [this] (const juce::String& wanted, juce::String& error) { return findPlugin (wanted, error, true); },
                                                       std::move (s.instrumentPresets), std::move (s.effects),
                                                       [this] (const juce::String& wanted, juce::String& error) { return findPlugin (wanted, error); });

    if (s.sequence)
        sequenceTools = std::make_unique<SequenceTools> (std::move (*s.sequence), steps,
                                                         [this] (const juce::String& wanted, juce::String& error) { return findPlugin (wanted, error); });
}

Workspace::~Workspace() = default;

juce::String Workspace::secondsText (juce::int64 frame) const
{
    return juce::String ((double) frame / sampleRate, 3);
}

juce::String Workspace::rangeText (juce::Range<juce::int64> r) const
{
    if (audio != nullptr && r.getStart() == 0 && r.getEnd() == audio->getNumFrames())
        return "the whole file";

    return secondsText (r.getStart()) + " s to " + secondsText (r.getEnd()) + " s";
}

std::optional<juce::int64> Workspace::frameOf (const juce::var& seconds) const
{
    if (! isNumber (seconds) || ! std::isfinite ((double) seconds))
        return std::nullopt;

    return (juce::int64) std::llround ((double) seconds * sampleRate);
}

juce::Range<juce::int64> Workspace::rangeOf (const juce::var& args, juce::String& error) const
{
    const auto total = audio->getNumFrames();
    const auto hasStart = args.hasProperty ("start"), hasEnd = args.hasProperty ("end");

    if (! hasStart && ! hasEnd)
        return selection.isEmpty() ? juce::Range<juce::int64> (0, total) : selection;

    const auto start = hasStart ? frameOf (args["start"]) : std::optional<juce::int64> (0);
    const auto end = hasEnd ? frameOf (args["end"]) : std::optional<juce::int64> (total);

    if (! start || ! end)
    {
        error = "start and end must be numbers of seconds.";
        return {};
    }

    // A little past the end (a rounded length) is the end.
    const auto slack = (juce::int64) (sampleRate * 0.01);

    if (*start < 0 || *start >= total || *end > total + slack || *end <= *start)
    {
        error = "the range must be inside the file (0 to " + secondsText (total) + " s), with end after start.";
        return {};
    }

    return { *start, juce::jmin (*end, total) };
}

void Workspace::change (Snapshot next, const juce::String& step)
{
    audio = std::move (next);
    steps.add (step);
}

juce::String Workspace::run (const juce::String& tool, const juce::String& argumentsJson)
{
    juce::var args;

    if (argumentsJson.trim().isNotEmpty() && ! juce::JSON::parse (argumentsJson, args).wasOk())
        return juce::JSON::toString (failure ("The arguments weren't valid JSON."), true);

    if (! args.isObject())
        args = juce::var (new juce::DynamicObject());

    juce::var result;

    try
    {
        result = dispatch (tool, args);
    }
    catch (const std::exception& e)
    {
        result = failure (juce::String ("It failed: ") + e.what());
    }

    return juce::JSON::toString (result, true);
}

juce::var Workspace::dispatch (const juce::String& tool, const juce::var& a)
{
    if (sequenceTools != nullptr)
    {
        if (const auto result = sequenceTools->run (tool, a))
            return *result;

        if (tool == "list_plugins")     return listPlugins (a);
        if (tool == "describe_plugin")  return describePlugin (a);

        return failure ("There is no tool called " + tool.quoted() + " for a sequence.");
    }

    if (sessionTools != nullptr)
    {
        if (const auto result = sessionTools->run (tool, a))
            return *result;

        if (tool == "list_plugins")     return listPlugins (a);
        if (tool == "describe_plugin")  return describePlugin (a);

        return failure ("There is no tool called " + tool.quoted() + " for a session.");
    }

    if (audio == nullptr || audio->getNumFrames() == 0)
        return failure ("There is no audio open.");

    if (tool == "get_state")        return getState();
    if (tool == "measure")          return measure (a);
    if (tool == "spectrum")         return spectrum (a);
    if (tool == "find")             return find (a);
    if (tool == "list_markers")     return listMarkers();
    if (tool == "select")           return select (a);
    if (tool == "edit")             return edit (a);
    if (tool == "fade")             return fade (a);
    if (tool == "trim_silence")     return trimSilence (a);
    if (tool == "delete")           return cutOrCrop (a, false);
    if (tool == "crop")             return cutOrCrop (a, true);
    if (tool == "silence")          return silence (a);
    if (tool == "insert_silence")   return insertSilence (a);
    if (tool == "add_marker")       return addMarker (a);
    if (tool == "rename_marker")    return renameMarker (a);
    if (tool == "delete_marker")    return deleteMarker (a);
    if (tool == "list_plugins")     return listPlugins (a);
    if (tool == "describe_plugin")  return describePlugin (a);
    if (tool == "list_chain")       return listChain();
    if (tool == "add_plugin")       return addPlugin (a);
    if (tool == "get_parameters")   return getParameters (a);
    if (tool == "set_parameters")   return setParameters (a);
    if (tool == "bypass_plugin")    return bypassPlugin (a);
    if (tool == "move_plugin")      return movePlugin (a);
    if (tool == "remove_plugin")    return removePlugin (a);
    if (tool == "apply_chain")      return applyChain (a);

    return failure ("There is no tool called " + tool.quoted() + ".");
}

//==============================================================================
juce::var Workspace::getState()
{
    juce::var sel;

    if (! selection.isEmpty())
        sel = object ({ { "start", round ((double) selection.getStart() / sampleRate, 3) },
                        { "end", round ((double) selection.getEnd() / sampleRate, 3) } });

    const auto slots = chainLoaded ? (int) chain.size() : (rackXml != nullptr ? rackXml->getNumChildElements() : 0);

    return object ({ { "ok", true },
                     { "file", fileName },
                     { "sample_rate", sampleRate },
                     { "channels", audio->getNumChannels() },
                     { "seconds", round ((double) audio->getNumFrames() / sampleRate, 3) },
                     { "selection", sel },
                     { "markers", (int) markers.size() },
                     { "chain_slots", slots },
                     { "changes_so_far", steps.size() } });
}

juce::var Workspace::measure (const juce::var& a)
{
    juce::String error;
    const auto r = rangeOf (a, error);

    if (error.isNotEmpty())
        return failure (error);

    const auto loud = Loudness::measure (*audio, r.getStart(), r.getEnd(), sampleRate);

    if (! loud.has_value())
        return failure ("Stopped.");

    juce::Array<juce::var> channels;
    const auto levels = AudioEdits::channelLevels (*audio, r.getStart(), r.getEnd());

    for (size_t c = 0; c < levels.size(); ++c)
        channels.add (object ({ { "channel", (int) c + 1 },
                                { "peak_dbfs", decibels (juce::Decibels::gainToDecibels ((double) levels[c].peak, -300.0)) },
                                { "rms_dbfs", decibels (juce::Decibels::gainToDecibels (levels[c].rms, -300.0)) },
                                { "dc_offset_percent", round (levels[c].mean * 100.0, 3) } }));

    const auto correlation = AudioEdits::stereoCorrelation (*audio, r.getStart(), r.getEnd());

    return object ({ { "ok", true },
                     { "range", rangeText (r) },
                     { "integrated_lufs", decibels (loud->integrated) },
                     { "loudness_range_lu", round (loud->range, 2) },
                     { "max_short_term_lufs", decibels (loud->maxShortTerm) },
                     { "true_peak_dbtp", decibels (loud->truePeak) },
                     { "sample_peak_dbfs", decibels (loud->samplePeak) },
                     { "channels", channels },
                     { "stereo_correlation", correlation ? juce::var (round (*correlation, 3)) : juce::var() } });
}

juce::var Workspace::spectrum (const juce::var& a)
{
    juce::String error;
    const auto r = rangeOf (a, error);

    if (error.isNotEmpty())
        return failure (error);

    const auto profile = MatchEq::measure (*audio, r.getStart(), r.getEnd(), sampleRate);

    if (! profile.has_value())
        return failure ("The range is too short (under 8,192 samples) or silent.");

    // Twelfth-octave points from 20 Hz: each octave band's power is the sum over its twelve points.
    auto bands = std::make_unique<juce::DynamicObject>();
    const std::array<const char*, 10> names { "31.5 Hz", "63 Hz", "125 Hz", "250 Hz", "500 Hz", "1 kHz", "2 kHz", "4 kHz", "8 kHz", "16 kHz" };

    for (size_t b = 0; b < names.size(); ++b)
    {
        const auto centre = 31.25 * std::pow (2.0, (double) b);
        double power = 0.0;
        int count = 0;

        for (int p = 0; p < MatchEq::numPoints; ++p)
        {
            const auto f = 20.0 * std::pow (2.0, (double) p / MatchEq::pointsPerOctave);

            if (f >= centre / std::sqrt (2.0) && f < centre * std::sqrt (2.0))
            {
                power += std::pow (10.0, profile->levelDb[(size_t) p] / 10.0);
                ++count;
            }
        }

        bands->setProperty (names[b], count > 0 ? decibels (10.0 * std::log10 (juce::jmax (power, 1.0e-30))) : juce::var());
    }

    return object ({ { "ok", true }, { "range", rangeText (r) }, { "octave_bands_dbfs", juce::var (bands.release()) } });
}

juce::var Workspace::find (const juce::var& a)
{
    const auto what = a["what"].toString();
    const auto clipping = what == "clipping";
    const auto thresholdDb = isNumber (a["threshold_db"]) ? (double) a["threshold_db"] : clipping ? -0.1 : -50.0;
    const auto level = (float) juce::Decibels::decibelsToGain (thresholdDb, -300.0);
    const auto minFrames = (juce::int64) ((isNumber (a["min_seconds"]) ? juce::jmax (0.001, (double) a["min_seconds"]) : 0.3) * sampleRate);
    constexpr size_t most = 100;
    juce::Array<juce::var> found;

    const auto add = [&] (juce::Range<juce::int64> r)
    {
        found.add (object ({ { "start", round ((double) r.getStart() / sampleRate, 3) }, { "end", round ((double) r.getEnd() / sampleRate, 3) } }));
    };

    if (what == "sounds")
        for (auto r : AudioEdits::soundsBetween (*audio, level, minFrames, most))
            add (r);
    else if (what == "silences")
        for (auto r : AudioEdits::pauses (*audio, level, minFrames, most))
            add (r);
    else if (clipping)
        for (auto start : AudioEdits::clippingStarts (*audio, level, 3, most))
            found.add (round ((double) start / sampleRate, 3));
    else
        return failure ("what must be sounds, silences or clipping.");

    return object ({ { "ok", true }, { "found", found }, { "count", found.size() }, { "at_most", (int) most } });
}

juce::var Workspace::listMarkers()
{
    juce::Array<juce::var> list;

    for (size_t i = 0; i < markers.size(); ++i)
    {
        const auto& m = markers[i];
        auto entry = object ({ { "number", (int) i + 1 }, { "name", m.name }, { "start", round ((double) m.start / sampleRate, 3) } });

        if (m.isRegion())
            entry.getDynamicObject()->setProperty ("end", round ((double) m.getEnd() / sampleRate, 3));

        list.add (entry);
    }

    return object ({ { "ok", true }, { "markers", list } });
}

//==============================================================================
juce::var Workspace::select (const juce::var& a)
{
    if ((bool) a["none"])
    {
        selection = {};
        return object ({ { "ok", true }, { "selection", "none" } });
    }

    if ((bool) a["all"])
    {
        selection = { 0, audio->getNumFrames() };
        return object ({ { "ok", true }, { "selection", rangeText (selection) } });
    }

    if (! a.hasProperty ("start") || ! a.hasProperty ("end"))
        return failure ("Give start and end, or all, or none.");

    juce::String error;
    const auto r = rangeOf (a, error);

    if (error.isNotEmpty())
        return failure (error);

    selection = r;
    return object ({ { "ok", true }, { "selection", rangeText (selection) } });
}

juce::var Workspace::edit (const juce::var& a)
{
    const auto kind = a["kind"].toString();
    const auto* effect = effectFor (kind);

    if (effect == nullptr)
        return failure ("There is no edit kind " + kind.quoted() + ".");

    juce::String error;
    const auto r = rangeOf (a, error);

    if (error.isNotEmpty())
        return failure (error);

    Sequences::Step step { kind, {}, nullptr };
    const auto& given = a["settings"];

    if (auto* o = given.getDynamicObject())
        for (auto& p : o->getProperties())
            if (std::none_of (effect->settings.begin(), effect->settings.end(), [&p] (const Setting& s) { return p.name.toString() == s.id; }))
                return failure (kind + " has no setting " + p.name.toString().quoted() + ".");

    for (auto& s : effect->settings)
    {
        auto value = s.fallback;

        if (given.hasProperty (s.id))
        {
            const auto& v = given[s.id];

            if (! isNumber (v) && ! v.isBool())
                return failure (juce::String (s.id) + " must be a number.");

            value = (double) v;

            if (value < s.low || value > s.high)
                return failure (juce::String (s.id) + " must be from " + juce::String (s.low) + " to " + juce::String (s.high) + ".");
        }

        step.values[s.id] = value;
    }

    const auto result = Sequences::apply (step, audio, r.getStart(), r.getEnd(), sampleRate, markers, nullptr,
                                          [this] (float) { return shouldStop == nullptr || ! shouldStop(); });

    if (result.error.isNotEmpty())
        return failure (result.error);

    if (result.snapshot == nullptr)
        return object ({ { "ok", true }, { "changed", false },
                         { "notes", result.notes.isEmpty() ? juce::String ("nothing was changed: the edit found nothing to do in that range, or the range is too short for it")
                                                           : result.notes.joinIntoString ("; ") } });

    const auto lengthChanged = result.snapshot->getNumFrames() != audio->getNumFrames();
    const auto settings = step.settingsText();
    change (result.snapshot, step.name() + (settings.isNotEmpty() && kind != "gain" ? " (" + settings + ")" : juce::String())
                                 + ", " + rangeText (r) + (result.notes.isEmpty() ? juce::String() : ": " + result.notes.joinIntoString ("; ")));

    if (lengthChanged)
    {
        markers = result.markers;
        markersEdited = true;

        if (! selection.isEmpty())
            selection = { result.start, result.end };
    }

    return object ({ { "ok", true }, { "changed", true }, { "step", steps[steps.size() - 1] },
                     { "notes", result.notes.joinIntoString ("; ") },
                     { "seconds_now", round ((double) audio->getNumFrames() / sampleRate, 3) } });
}

juce::var Workspace::fade (const juce::var& a)
{
    const auto direction = a["direction"].toString();

    if (direction != "in" && direction != "out")
        return failure ("direction must be in or out.");

    juce::String error;
    auto r = rangeOf (a, error);

    if (error.isNotEmpty())
        return failure (error);

    if (a.hasProperty ("seconds"))
    {
        const auto length = frameOf (a["seconds"]);

        if (! length || *length <= 0)
            return failure ("seconds must be a positive number.");

        const auto frames = juce::jmin (*length, r.getLength());
        r = direction == "in" ? juce::Range<juce::int64> (r.getStart(), r.getStart() + frames)
                              : juce::Range<juce::int64> (r.getEnd() - frames, r.getEnd());
    }

    const auto shapeId = a["shape"].toString();
    const std::map<juce::String, FadeShape> shapes { { "s-curve", FadeShape::sCurve }, { "linear", FadeShape::linear },
                                                     { "equal-power", FadeShape::equalPower }, { "slow", FadeShape::slow }, { "fast", FadeShape::fast } };
    const auto found = shapes.find (shapeId.isEmpty() ? juce::String ("s-curve") : shapeId);

    if (found == shapes.end())
        return failure ("shape must be one of s-curve, linear, equal-power, slow, fast.");

    change ((direction == "in" ? AudioEdits::fadeIn : AudioEdits::fadeOut) (*audio, r.getStart(), r.getEnd(), found->second),
            "Fade " + juce::String (direction == "in" ? "In" : "Out") + " (" + FadeShapes::displayName (found->second) + "), " + rangeText (r));
    return object ({ { "ok", true }, { "step", steps[steps.size() - 1] } });
}

juce::var Workspace::trimSilence (const juce::var& a)
{
    const auto thresholdDb = isNumber (a["threshold_db"]) ? (double) a["threshold_db"] : -60.0;
    const auto audible = AudioEdits::audibleRange (*audio, (float) juce::Decibels::decibelsToGain (thresholdDb, -300.0));

    if (! audible)
        return failure ("The whole file is at or below " + juce::String (thresholdDb, 1) + " dB.");

    if (audible->getLength() == audio->getNumFrames())
        return object ({ { "ok", true }, { "changed", false }, { "notes", "neither end is at or below the threshold" } });

    const auto before = audio->getNumFrames();
    markers = MarkerEdits::afterCrop (markers, audible->getStart(), audible->getEnd());
    markersEdited = true;
    selection = {};
    change (AudioEdits::crop (*audio, audible->getStart(), audible->getEnd()),
            "Trim Silence below " + juce::String (thresholdDb, 1) + " dB: " + secondsText (audible->getStart()) + " s off the start, "
                + secondsText (before - audible->getEnd()) + " s off the end");
    return object ({ { "ok", true }, { "changed", true }, { "step", steps[steps.size() - 1] },
                     { "seconds_now", round ((double) audio->getNumFrames() / sampleRate, 3) } });
}

juce::var Workspace::cutOrCrop (const juce::var& a, bool crop)
{
    juce::String error;
    const auto r = rangeOf (a, error);

    if (error.isNotEmpty())
        return failure (error);

    if (r.getLength() == audio->getNumFrames())
        return failure (crop ? "That keeps the whole file." : "That would delete the whole file.");

    const auto text = rangeText (r);

    if (crop)
    {
        markers = MarkerEdits::afterCrop (markers, r.getStart(), r.getEnd());
        change (AudioEdits::crop (*audio, r.getStart(), r.getEnd()), "Crop to " + text);
    }
    else
    {
        markers = MarkerEdits::afterReplace (markers, r.getStart(), r.getEnd(), 0);
        change (AudioEdits::remove (*audio, r.getStart(), r.getEnd()), "Delete " + text);
    }

    markersEdited = true;
    selection = {};
    return object ({ { "ok", true }, { "step", steps[steps.size() - 1] },
                     { "seconds_now", round ((double) audio->getNumFrames() / sampleRate, 3) } });
}

juce::var Workspace::silence (const juce::var& a)
{
    juce::String error;
    const auto r = rangeOf (a, error);

    if (error.isNotEmpty())
        return failure (error);

    change (AudioEdits::silence (*audio, r.getStart(), r.getEnd()), "Silence " + rangeText (r));
    return object ({ { "ok", true }, { "step", steps[steps.size() - 1] } });
}

juce::var Workspace::insertSilence (const juce::var& a)
{
    const auto at = frameOf (a["at"]);
    const auto length = frameOf (a["seconds"]);

    if (! at || *at < 0 || *at > audio->getNumFrames())
        return failure ("at must be inside the file (0 to " + secondsText (audio->getNumFrames()) + " s).");

    if (! length || *length <= 0 || (double) *length > sampleRate * 3600.0)
        return failure ("seconds must be more than 0 and at most an hour.");

    markers = MarkerEdits::afterReplace (markers, *at, *at, *length);
    markersEdited = true;
    selection = {};
    change (AudioEdits::insertSilence (*audio, *at, *length),
            "Insert " + secondsText (*length) + " s of Silence at " + secondsText (*at) + " s");
    return object ({ { "ok", true }, { "step", steps[steps.size() - 1] } });
}

juce::var Workspace::addMarker (const juce::var& a)
{
    const auto start = frameOf (a["start"]);

    if (! start || *start < 0 || *start > audio->getNumFrames())
        return failure ("start must be inside the file.");

    Marker m;
    m.start = *start;

    if (a.hasProperty ("end"))
    {
        juce::String error;
        const auto r = rangeOf (a, error);

        if (error.isNotEmpty())
            return failure (error);

        m.length = r.getLength();
    }

    m.name = a["name"].toString().trim();

    if (m.name.isEmpty())
        m.name = MarkerEdits::nextName (markers, m.isRegion());

    markers.push_back (m);
    MarkerEdits::sort (markers);
    markersEdited = true;
    steps.add ("Add " + juce::String (m.isRegion() ? "Region " : "Marker ") + m.name.quoted() + " at "
               + (m.isRegion() ? rangeText ({ m.start, m.getEnd() }) : secondsText (m.start) + " s"));
    return object ({ { "ok", true }, { "step", steps[steps.size() - 1] } });
}

juce::var Workspace::renameMarker (const juce::var& a)
{
    const auto n = (int) a["number"];
    const auto name = a["name"].toString().trim();

    if (! juce::isPositiveAndNotGreaterThan (n, (int) markers.size()) || n == 0)
        return failure ("There is no marker " + juce::String (n) + "; list_markers numbers them.");

    if (name.isEmpty())
        return failure ("The name is empty.");

    auto& m = markers[(size_t) n - 1];
    steps.add ("Rename " + m.name.quoted() + " to " + name.quoted());
    m.name = name;
    markersEdited = true;
    return object ({ { "ok", true }, { "step", steps[steps.size() - 1] } });
}

juce::var Workspace::deleteMarker (const juce::var& a)
{
    const auto n = (int) a["number"];

    if (! juce::isPositiveAndNotGreaterThan (n, (int) markers.size()) || n == 0)
        return failure ("There is no marker " + juce::String (n) + "; list_markers numbers them.");

    steps.add ("Delete " + juce::String (markers[(size_t) n - 1].isRegion() ? "Region " : "Marker ") + markers[(size_t) n - 1].name.quoted());
    markers.erase (markers.begin() + (n - 1));
    markersEdited = true;
    return object ({ { "ok", true }, { "step", steps[steps.size() - 1] }, { "note", "the markers after it are numbered one lower now" } });
}

//==============================================================================
juce::var Workspace::listPlugins (const juce::var& a)
{
    const auto search = a["search"].toString().trim();
    const auto instruments = (bool) a["instruments"];
    juce::Array<juce::var> list;
    constexpr int most = 200;
    int matched = 0;

    for (auto& d : plugins)
    {
        if (d.isInstrument != instruments)
            continue;

        const auto summary = catalogue != nullptr ? catalogue->summaryOf (d) : juce::String();

        if (search.isNotEmpty() && ! (d.name.containsIgnoreCase (search) || d.manufacturerName.containsIgnoreCase (search)
                                      || d.category.containsIgnoreCase (search) || d.descriptiveName.containsIgnoreCase (search)
                                      || summary.containsIgnoreCase (search)))
            continue;

        if (++matched <= most)
            list.add (object ({ { "name", d.name }, { "maker", d.manufacturerName }, { "format", d.pluginFormatName },
                                { "category", d.category }, { "id", d.createIdentifierString() },
                                { "summary", summary.isNotEmpty() ? juce::var (summary) : juce::var() } }));
    }

    return object ({ { "ok", true }, { "plugins", list }, { "count", matched },
                     { "note", matched > most ? juce::var ("only the first " + juce::String (most) + " are listed; search to narrow it") : juce::var() } });
}

juce::var Workspace::describePlugin (const juce::var& a)
{
    juce::String error;
    const auto d = findPlugin (a["plugin"].toString().trim(), error, true);

    if (! d)
        return failure (error);

    const auto facts = catalogue != nullptr ? catalogue->getFacts (*d) : juce::var();
    const auto understanding = catalogue != nullptr ? catalogue->getUnderstanding (*d) : std::nullopt;

    if (! understanding && (facts.isVoid() || facts.hasProperty ("error")))
        return failure ("The plug-in catalogue has nothing on " + d->name + " yet. Add it and use get_parameters to see its parameters.");

    auto result = object ({ { "ok", true }, { "plugin", d->name }, { "maker", d->manufacturerName }, { "format", d->pluginFormatName },
                            { "category", d->category }, { "id", d->createIdentifierString() } });

    if (understanding)
        result.getDynamicObject()->setProperty ("about", understanding->text);

    if (facts.isObject() && ! facts.hasProperty ("error"))
        for (auto* field : { "parameters", "parameterCount", "inputs", "outputs", "latency", "programs", "programCount" })
            result.getDynamicObject()->setProperty (field, facts[field]);
    else
        result.getDynamicObject()->setProperty ("note", "Its parameters haven't been read; add it and use get_parameters.");

    return result;
}

juce::String Workspace::loadChain()
{
    if (chainLoaded)
        return {};

    if (formats == nullptr)
        return "Plug-ins can't be used here.";

    chainLoaded = true;
    juce::StringArray missing;

    if (rackXml != nullptr)
        for (auto* slotXml : rackXml->getChildWithTagNameIterator ("SLOT"))
        {
            juce::PluginDescription d;

            if (auto* dx = slotXml->getChildByName ("PLUGIN"); dx == nullptr || ! d.loadFromXml (*dx))
                continue;

            juce::String error;
            auto plugin = formats->createPluginInstance (d, sampleRate, 512, error);

            if (plugin == nullptr)
            {
                missing.add (d.name);
                continue;
            }

            if (auto* stateXml = slotXml->getChildByName ("STATE"))
            {
                juce::MemoryBlock state;

                if (state.fromBase64Encoding (stateXml->getAllSubText()))
                    plugin->setStateInformation (state.getData(), (int) state.getSize());
            }

            chain.push_back ({ std::move (plugin), slotXml->getBoolAttribute ("bypassed") });
        }

    return missing.isEmpty() ? juce::String() : "These couldn't be opened and were left out: " + missing.joinIntoString (", ") + ".";
}

std::unique_ptr<juce::XmlElement> Workspace::createRackXml() const
{
    if (! chainLoaded)
        return rackXml != nullptr ? std::make_unique<juce::XmlElement> (*rackXml) : nullptr;

    auto xml = std::make_unique<juce::XmlElement> ("TORQWAVE_RACK");
    xml->setAttribute ("version", 1);

    for (auto& s : chain)
    {
        auto* slot = xml->createNewChildElement ("SLOT");
        slot->setAttribute ("bypassed", s.bypassed);
        slot->addChildElement (s.plugin->getPluginDescription().createXml().release());
        juce::MemoryBlock state;
        s.plugin->getStateInformation (state);
        slot->createNewChildElement ("STATE")->addTextElement (state.toBase64Encoding());
    }

    return xml;
}

Workspace::Slot* Workspace::slotOf (const juce::var& a, juce::String& error)
{
    if (error = loadChain(); error.isNotEmpty() && formats == nullptr)
        return nullptr;

    error = {};
    const auto n = (int) a["slot"];

    if (n < 1 || n > (int) chain.size())
    {
        error = chain.empty() ? "The chain is empty." : "There is no slot " + juce::String (n) + "; the chain has " + juce::String ((int) chain.size()) + ".";
        return nullptr;
    }

    return &chain[(size_t) n - 1];
}

juce::var Workspace::listChain()
{
    const auto note = loadChain();

    if (formats == nullptr)
        return failure (note);

    juce::Array<juce::var> list;

    for (size_t i = 0; i < chain.size(); ++i)
        list.add (object ({ { "slot", (int) i + 1 }, { "plugin", chain[i].plugin->getName() }, { "bypassed", chain[i].bypassed } }));

    return object ({ { "ok", true }, { "chain", list }, { "note", note.isNotEmpty() ? juce::var (note) : juce::var() } });
}

std::optional<juce::PluginDescription> Workspace::findPlugin (const juce::String& wanted, juce::String& error, bool instrumentsToo) const
{
    for (auto& d : plugins)
        if (d.createIdentifierString() == wanted)
            return d;

    juce::Array<juce::PluginDescription> named, partly;

    for (auto& d : plugins)
    {
        if (d.isInstrument && ! instrumentsToo)
            continue;

        if (d.name.equalsIgnoreCase (wanted))
            named.add (d);
        else if (d.name.containsIgnoreCase (wanted))
            partly.add (d);
    }

    const auto& matches = ! named.isEmpty() ? named : partly;

    if (matches.size() == 1)
        return matches.getFirst();

    // The same plug-in in several formats: the built-in, then VST3, then the rest.
    if (! matches.isEmpty() && std::all_of (matches.begin(), matches.end(), [&] (const juce::PluginDescription& d) { return d.name == matches.getFirst().name; }))
    {
        for (auto* format : { "Torqwave", "VST3", "AudioUnit", "LV2" })
            for (auto& d : matches)
                if (d.pluginFormatName == format)
                    return d;

        return matches.getFirst();
    }

    if (matches.isEmpty())
        error = (instrumentsToo ? "No plug-in is called " : "No effect plug-in is called ") + wanted.quoted() + "; list_plugins shows them.";
    else
    {
        juce::StringArray names;

        for (auto& d : matches)
            names.add (d.name + " (" + d.manufacturerName + ", " + d.pluginFormatName + ")");

        error = wanted.quoted() + " could be any of these; give its id from list_plugins: " + names.joinIntoString ("; ") + ".";
    }

    return std::nullopt;
}

juce::var Workspace::addPlugin (const juce::var& a)
{
    if (formats == nullptr)
        return failure ("Plug-ins can't be used here.");

    const auto note = loadChain();
    juce::String error;
    const auto d = findPlugin (a["plugin"].toString().trim(), error);

    if (! d)
        return failure (error);

    if (d->isInstrument)
        return failure (d->name + " is an instrument; the chain takes effects.");

    auto plugin = formats->createPluginInstance (*d, sampleRate, 512, error);

    if (plugin == nullptr)
        return failure (d->name + " couldn't be opened: " + error);

    // Opened anyway, so its facts go into the catalogue for next time.
    if (catalogue != nullptr && ! catalogue->hasCurrentFacts (*d))
        catalogue->setFacts (*d, PluginCatalogue::factsOf (*plugin));

    const auto position = a.hasProperty ("position") ? juce::jlimit (1, (int) chain.size() + 1, (int) a["position"]) : (int) chain.size() + 1;
    const auto parameters = plugin->getParameters().size();
    chain.insert (chain.begin() + (position - 1), { std::move (plugin), false });
    chainEdited = true;
    steps.add ("Add " + d->name + " to the effects rack, slot " + juce::String (position));

    return object ({ { "ok", true }, { "slot", position }, { "plugin", d->name }, { "parameters", parameters },
                     { "note", note.isNotEmpty() ? juce::var (note) : juce::var() } });
}

juce::var Workspace::getParameters (const juce::var& a)
{
    juce::String error;
    auto* slot = slotOf (a, error);

    if (slot == nullptr)
        return failure (error);

    const auto search = a["search"].toString().trim();
    juce::Array<juce::var> list;
    constexpr int most = 300;
    int matched = 0;

    for (auto* p : slot->plugin->getParameters())
    {
        const auto name = p->getName (100);

        if (search.isNotEmpty() && ! name.containsIgnoreCase (search))
            continue;

        if (++matched > most)
            continue;

        list.add (PluginCatalogue::parameterFacts (*p, true));
    }

    return object ({ { "ok", true }, { "plugin", slot->plugin->getName() }, { "parameters", list }, { "count", matched },
                     { "note", matched > most ? juce::var ("only the first " + juce::String (most) + "; search to narrow it") : juce::var() } });
}

juce::var Workspace::setParameters (const juce::var& a)
{
    juce::String error;
    auto* slot = slotOf (a, error);

    if (slot == nullptr)
        return failure (error);

    auto* values = a["values"].getDynamicObject();

    if (values == nullptr || values->getProperties().isEmpty())
        return failure ("values must name at least one parameter.");

    const auto& all = slot->plugin->getParameters();
    juce::Array<juce::var> results;
    juce::StringArray changed;

    for (auto& entry : values->getProperties())
    {
        const auto wanted = entry.name.toString();
        juce::AudioProcessorParameter* parameter = nullptr;

        for (auto* p : all)
            if (p->getName (100).equalsIgnoreCase (wanted))
                parameter = p;

        if (parameter == nullptr)
            for (auto* p : all)
                if (auto* hosted = dynamic_cast<juce::HostedAudioProcessorParameter*> (p); hosted != nullptr && hosted->getParameterID() == wanted)
                    parameter = p;

        if (parameter == nullptr)
        {
            results.add (object ({ { "name", wanted }, { "ok", false }, { "error", "no such parameter; get_parameters lists them" } }));
            continue;
        }

        const auto& v = entry.value;
        const auto text = v.isBool() ? juce::String ((bool) v ? "On" : "Off") : v.toString();
        juce::String problem;
        const auto now = Parameters::set (*parameter, text, problem);

        if (problem.isNotEmpty())
            results.add (object ({ { "name", parameter->getName (100) }, { "ok", false }, { "error", problem }, { "value", now } }));
        else
        {
            results.add (object ({ { "name", parameter->getName (100) }, { "ok", true }, { "value", now } }));
            changed.add (parameter->getName (100) + " " + now);
        }
    }

    if (! changed.isEmpty())
    {
        chainEdited = true;
        const auto n = (int) a["slot"];
        steps.add ("Set " + slot->plugin->getName() + " (slot " + juce::String (n) + "): " + changed.joinIntoString (", "));
    }

    return object ({ { "ok", ! changed.isEmpty() }, { "results", results } });
}

juce::var Workspace::bypassPlugin (const juce::var& a)
{
    juce::String error;
    auto* slot = slotOf (a, error);

    if (slot == nullptr)
        return failure (error);

    slot->bypassed = (bool) a["bypassed"];
    chainEdited = true;
    steps.add (juce::String (slot->bypassed ? "Bypass " : "Turn on ") + slot->plugin->getName() + " (slot " + a["slot"].toString() + ")");
    return object ({ { "ok", true }, { "step", steps[steps.size() - 1] } });
}

juce::var Workspace::movePlugin (const juce::var& a)
{
    juce::String error;

    if (slotOf (a, error) == nullptr)
        return failure (error);

    const auto from = (int) a["slot"] - 1;
    const auto to = juce::jlimit (0, (int) chain.size() - 1, (int) a["to"] - 1);
    auto moved = std::move (chain[(size_t) from]);
    chain.erase (chain.begin() + from);
    chain.insert (chain.begin() + to, std::move (moved));
    chainEdited = true;
    steps.add ("Move " + chain[(size_t) to].plugin->getName() + " to slot " + juce::String (to + 1));
    return object ({ { "ok", true }, { "step", steps[steps.size() - 1] } });
}

juce::var Workspace::removePlugin (const juce::var& a)
{
    juce::String error;

    if (slotOf (a, error) == nullptr)
        return failure (error);

    const auto index = (int) a["slot"] - 1;
    steps.add ("Remove " + chain[(size_t) index].plugin->getName() + " from the effects rack");
    chain.erase (chain.begin() + index);
    chainEdited = true;
    return object ({ { "ok", true }, { "step", steps[steps.size() - 1] }, { "note", "the slots after it are numbered one lower now" } });
}

juce::var Workspace::applyChain (const juce::var& a)
{
    if (formats == nullptr)
        return failure ("Plug-ins can't be used here.");

    loadChain();

    if (std::all_of (chain.begin(), chain.end(), [] (const Slot& s) { return s.bypassed; }))
        return failure ("The chain has no plug-in that isn't bypassed.");

    juce::String error;
    const auto r = rangeOf (a, error);

    if (error.isNotEmpty())
        return failure (error);

    std::shared_ptr<const juce::XmlElement> xml (createRackXml().release());
    const auto result = Sequences::apply ({ "rack", {}, xml }, audio, r.getStart(), r.getEnd(), sampleRate, markers, formats,
                                          [this] (float) { return shouldStop == nullptr || ! shouldStop(); });

    if (result.error.isNotEmpty())
        return failure (result.error);

    juce::StringArray names;

    for (auto& s : chain)
        if (! s.bypassed)
            names.add (s.plugin->getName());

    rendered = xml;
    chain.clear();
    chainEdited = true;
    change (result.snapshot, "Apply Effects Rack (" + names.joinIntoString (", ") + "), " + rangeText (r) + "; the rack is emptied");
    return object ({ { "ok", true }, { "step", steps[steps.size() - 1] } });
}

//==============================================================================
namespace Parameters
{
std::optional<double> numberIn (const juce::String& text)
{
    const auto t = text.trim();
    const auto start = t.indexOfAnyOf ("+-0123456789.");

    if (start < 0)
        return std::nullopt;

    auto end = start + 1;

    while (end < t.length() && juce::String ("0123456789.eE").containsChar (t[end]))
        ++end;

    const auto digits = t.substring (start, end);

    if (! digits.containsAnyOf ("0123456789"))
        return std::nullopt;

    auto value = digits.getDoubleValue();
    const auto rest = t.substring (end).trimStart();

    if (rest.startsWithChar ('k') || rest.startsWithChar ('K'))
        value *= 1000.0;

    // "-inf", "-∞": as low as a decibel reads.
    if (t.containsIgnoreCase ("inf") || t.contains (juce::CharPointer_UTF8 ("\xe2\x88\x9e")))
        return std::nullopt;

    return value;
}

juce::String set (juce::AudioProcessorParameter& p, const juce::String& value, juce::String& error)
{
    const auto text = value.trim();
    const auto choices = p.getAllValueStrings();

    // A choice by its name.
    if (! choices.isEmpty())
    {
        const auto index = choices.indexOf (text, true);

        if (index >= 0)
        {
            p.setValueNotifyingHost (choices.size() > 1 ? (float) index / (float) (choices.size() - 1) : 0.0f);
            return p.getCurrentValueAsText();
        }
    }

    const auto readsAs = [&p] (float normalised) { return p.getText (normalised, 100).trim(); };
    const auto wanted = numberIn (text);

    // The plug-in's own reading of the text.
    const auto first = juce::jlimit (0.0f, 1.0f, p.getValueForText (text));
    const auto firstReads = readsAs (first);

    const auto close = [] (double a, double b) { return std::abs (a - b) <= juce::jmax (0.01, std::abs (b) * 0.01); };

    if (firstReads.equalsIgnoreCase (text) || (wanted && numberIn (firstReads) && close (*numberIn (firstReads), *wanted)))
    {
        p.setValueNotifyingHost (first);
        return p.getCurrentValueAsText();
    }

    if (! wanted)
    {
        error = "it isn't one of the choices or a number";

        if (! choices.isEmpty())
            error << ": " << choices.joinIntoString (", ");

        return p.getCurrentValueAsText();
    }

    // The plug-in read it wrongly: search the range for the number, which rises or falls with it.
    const auto low = numberIn (readsAs (0.0f)), high = numberIn (readsAs (1.0f));

    if (! low || ! high || juce::exactlyEqual (*low, *high))
    {
        error = "the plug-in didn't take " + text.quoted() + " and its values aren't numbers to search";
        return p.getCurrentValueAsText();
    }

    const auto rising = *high > *low;

    if (*wanted < juce::jmin (*low, *high) || *wanted > juce::jmax (*low, *high))
    {
        error = "outside its range, " + readsAs (0.0f) + " to " + readsAs (1.0f);
        return p.getCurrentValueAsText();
    }

    auto a = 0.0f, b = 1.0f;

    for (int i = 0; i < 40; ++i)
    {
        const auto mid = 0.5f * (a + b);
        const auto at = numberIn (readsAs (mid));

        if (! at)
            break;

        if ((*at < *wanted) == rising)
            a = mid;
        else
            b = mid;
    }

    const auto best = std::abs (numberIn (readsAs (a)).value_or (*low) - *wanted) <= std::abs (numberIn (readsAs (b)).value_or (*high) - *wanted) ? a : b;
    p.setValueNotifyingHost (best);
    const auto now = p.getCurrentValueAsText();

    if (const auto reached = numberIn (readsAs (best)); ! reached || ! close (*reached, *wanted))
        error = "the nearest it reaches is " + now;

    return now;
}
}
}
