#include "WaveSynth.h"
#include "WaveSynthEditor.h"

namespace WaveSynth
{
const juce::StringArray& sourceNames()
{
    static const juce::StringArray n { "None", "Amp Env", "Env 2", "Env 3", "Env 4", "LFO 1", "LFO 2", "Velocity", "Mod Wheel", "Aftertouch", "Key",
                                       "Macro 1", "Macro 2", "Macro 3", "Macro 4", "Random" };
    return n;
}

namespace
{
    struct Destination { juce::String id, name; };

    const std::vector<Destination>& destinations()
    {
        static const std::vector<Destination> d = [] {
            std::vector<Destination> all;
            const auto add = [&all] (const juce::String& id, const juce::String& name) { all.push_back ({ id, name }); };

            for (int o = 1; o <= numOscillators; ++o)
            {
                const auto p = "osc" + juce::String (o), n = "Osc " + juce::String (o) + " ";
                add (p + "Position", n + "Position");
                add (p + "WarpAmount", n + "Warp");
                add (p + "Pitch", n + "Pitch");
                add (p + "Fine", n + "Fine");
                add (p + "Volume", n + "Level");
                add (p + "Pan", n + "Pan");
                add (p + "UnisonDetune", n + "Detune");
                add (p + "UnisonSpread", n + "Spread");
                add (p + "Route", n + "Filter Route");
            }

            add ("subLevel", "Sub Level");
            add ("noiseLevel", "Noise Level");
            add ("noiseColour", "Noise Colour");

            for (int f = 1; f <= 2; ++f)
            {
                const auto p = "filter" + juce::String (f), n = "Filter " + juce::String (f) + " ";
                add (p + "Cutoff", n + "Cutoff");
                add (p + "Resonance", n + "Resonance");
                add (p + "Drive", n + "Drive");
            }

            add ("ampLevel", "Amp Level");
            add ("ampPan", "Amp Pan");
            add ("lfo1Rate", "LFO 1 Rate");
            add ("lfo2Rate", "LFO 2 Rate");
            add ("fxDriveAmount", "Drive Amount");
            add ("fxChorusMix", "Chorus Mix");
            add ("fxPhaserMix", "Phaser Mix");
            add ("fxDelayMix", "Delay Mix");
            add ("fxDelayFeedback", "Delay Feedback");
            add ("fxReverbMix", "Reverb Mix");
            return all;
        }();
        return d;
    }

    // Where each destination sits in the list (fixed by the order above).
    constexpr int oscDest (int o, int field)        { return o * 9 + field; }   // fields: position, warp, pitch, fine, volume, pan, detune, spread, route
    enum : int { dSubLevel = 27, dNoiseLevel, dNoiseColour, dF1Cutoff, dF1Res, dF1Drive, dF2Cutoff, dF2Res, dF2Drive, dAmpLevel, dAmpPan,
                 dLfo1Rate, dLfo2Rate, dFxDrive, dFxChorus, dFxPhaser, dFxDelayMix, dFxDelayFeedback, dFxReverb, numDestinations };
    enum : int { fPosition, fWarp, fPitch, fFine, fVolume, fPan, fDetune, fSpread, fRoute };
}

const juce::StringArray& destinationIds()
{
    static const juce::StringArray ids = [] { juce::StringArray a; for (auto& d : destinations()) a.add (d.id); return a; }();
    return ids;
}

const juce::StringArray& destinationNames()
{
    static const juce::StringArray names = [] { juce::StringArray a; for (auto& d : destinations()) a.add (d.name); return a; }();
    return names;
}

const std::vector<std::pair<int, juce::String>>& destinationSections()
{
    // In a slot's menu the destinations come after "None", so each index is one on.
    static const std::vector<std::pair<int, juce::String>> s { { 1, "Oscillator 1" }, { 10, "Oscillator 2" }, { 19, "Oscillator 3" }, { 28, "Sub and Noise" },
                                                                { 31, "Filters" }, { 37, "Amp" }, { 39, "LFOs" }, { 41, "Effects" } };
    return s;
}

const juce::StringArray& warpNames()
{
    static const juce::StringArray n { "Off", "Sync", "Bend", "Squeeze", "Mirror", "FM", "Crush", "Fold" };
    return n;
}

const juce::StringArray& filterNames()
{
    static const juce::StringArray n { "Off", "Ladder 24", "Ladder 12", "Ladder HP", "Ladder BP", "SVF Low", "SVF Band", "SVF High", "SVF Notch", "Comb" };
    return n;
}

const juce::StringArray& lfoShapeNames()
{
    static const juce::StringArray n { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "Random", "Drift", "Steps" };
    return n;
}

const juce::StringArray& divisionNames()
{
    static const juce::StringArray n { "8 bars", "4 bars", "2 bars", "1 bar", "1/2", "1/2 T", "1/4 D", "1/4", "1/4 T", "1/8 D", "1/8", "1/8 T",
                                       "1/16 D", "1/16", "1/16 T", "1/32", "1/64" };
    return n;
}

double divisionBeats (int i)
{
    static const double beats[] = { 32.0, 16.0, 8.0, 4.0, 2.0, 4.0 / 3.0, 1.5, 1.0, 2.0 / 3.0, 0.75, 0.5, 1.0 / 3.0, 0.375, 0.25, 1.0 / 6.0, 0.125, 0.0625 };
    return beats[juce::jlimit (0, (int) std::size (beats) - 1, i)];
}
}

//==============================================================================
namespace
{
    using namespace WaveSynth;

    constexpr float twoPiF = juce::MathConstants<float>::twoPi;

    /** A deterministic hash to a float in [-1, 1). */
    inline float hashNoise (juce::uint32 x) noexcept
    {
        x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
        return (float) (x >> 8) * (2.0f / 16777216.0f) - 1.0f;
    }

    struct Rng
    {
        juce::uint32 state = 0x12345678u;
        float next() noexcept   { state = state * 1664525u + 1013904223u; return (float) (state >> 8) * (2.0f / 16777216.0f) - 1.0f; }
    };

    /** tanh, near enough (within 3 %, meeting 1 at |x| = 3) and a good deal cheaper. */
    inline float softClip (float x) noexcept
    {
        if (x <= -3.0f) return -1.0f;
        if (x >= 3.0f) return 1.0f;
        const auto x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    inline float fold (float x) noexcept
    {
        // A triangle fold: keeps within [-1, 1], reflecting at the edges.
        const auto f = x * 0.25f + 0.25f;
        return 4.0f * std::abs (f - std::floor (f + 0.5f)) - 1.0f;
    }

    inline void panGains (float pan, float& l, float& r) noexcept
    {
        // Equal power, -1 left to 1 right.
        const auto a = (juce::jlimit (-1.0f, 1.0f, pan) + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        l = std::cos (a) * juce::MathConstants<float>::sqrt2;
        r = std::sin (a) * juce::MathConstants<float>::sqrt2;
    }

    struct Ramp
    {
        float value = 0.0f, step = 0.0f;
        void to (float target, bool jump) noexcept   { if (jump) { value = target; step = 0.0f; } else step = (target - value) / (float) controlInterval; }
        float next() noexcept                         { value += step; return value; }
    };

    struct Envelope
    {
        enum Stage { idle, attack, decay, sustain, release };
        Stage stage = idle;
        float level = 0.0f, start = 0.0f, t = 0.0f;
        float ex = 1.0f;   // exp (-k t), kept by multiplying as t moves on, so a sample needs no exp

        void trigger() noexcept                       { start = level; t = 0.0f; ex = 1.0f; stage = attack; }
        void releaseNow() noexcept                    { if (stage != idle) { start = level; t = 0.0f; ex = 1.0f; stage = release; } }
    };

    /** A stage's step and its curve: k = 0 is a straight line, k > 0 falls fast and settles (as an
        analogue envelope's exponential), k < 0 the other way. */
    struct Segment
    {
        float step = 1.0f, multiplier = 1.0f, scale = 1.0f;
        bool straight = true;

        void set (float s, float k) noexcept
        {
            step = s;
            straight = std::abs (k) < 0.01f;
            multiplier = straight ? 1.0f : std::exp (-k * s);
            scale = straight ? 1.0f : 1.0f / (1.0f - std::exp (-k));
        }

        /** The segment's progress after a step: 0 to 1, curved. */
        float advance (Envelope& e) const noexcept
        {
            e.t += step;
            e.ex *= multiplier;
            return straight ? e.t : (1.0f - e.ex) * scale;
        }
    };

    struct EnvelopeSettings
    {
        Segment attack, decay, release;
        float sustain = 1.0f;
    };

    inline void advance (Envelope& e, const EnvelopeSettings& s) noexcept
    {
        switch (e.stage)
        {
            case Envelope::idle: break;
            case Envelope::attack:
            {
                const auto c = s.attack.advance (e);
                if (e.t >= 1.0f) { e.level = 1.0f; e.start = 1.0f; e.t = 0.0f; e.ex = 1.0f; e.stage = Envelope::decay; }
                else e.level = e.start + (1.0f - e.start) * c;
                break;
            }
            case Envelope::decay:
            {
                const auto c = s.decay.advance (e);
                if (e.t >= 1.0f) { e.level = s.sustain; e.stage = Envelope::sustain; }
                else e.level = s.sustain + (e.start - s.sustain) * (1.0f - c);
                break;
            }
            case Envelope::sustain:
                e.level += (s.sustain - e.level) * 0.002f;   // follows a moved Sustain smoothly
                break;
            case Envelope::release:
            {
                const auto c = s.release.advance (e);
                if (e.t >= 1.0f) { e.level = 0.0f; e.stage = Envelope::idle; }
                else e.level = e.start * (1.0f - c);
                break;
            }
        }
    }

    /** The ladder: four one-pole stages solved with their feedback in one step (topology-preserving),
        a soft clip where the feedback meets the input. */
    struct Ladder
    {
        float s1 = 0, s2 = 0, s3 = 0, s4 = 0;

        inline float process (float x, float g, float k, int type) noexcept
        {
            const auto G = g / (1.0f + g);
            const auto G2 = G * G;
            const auto sigma = (G2 * G * s1 + G2 * s2 + G * s3 + s4) / (1.0f + g);
            const auto y4est = (G2 * G2 * x + sigma) / (1.0f + k * G2 * G2);
            auto u = x - k * y4est;
            u = u / (1.0f + std::abs (u) * 0.25f);   // gentle limiting, keeps resonance musical when driven
            const auto stage = [G] (float in, float& s) { const auto v = (in - s) * G; const auto y = v + s; s = y + v; return y; };
            const auto y1 = stage (u, s1), y2 = stage (y1, s2), y3 = stage (y2, s3), y4 = stage (y3, s4);

            switch (type)
            {
                case 1:  return y2;                                              // 12 dB
                case 2:  return u - 4.0f * y1 + 6.0f * y2 - 4.0f * y3 + y4;      // high-pass 24
                case 3:  return 4.0f * (y2 - 2.0f * y3 + y4);                    // band-pass
                default: return y4;
            }
        }

        void clear() noexcept   { s1 = s2 = s3 = s4 = 0.0f; }
    };

    /** The state-variable filter (a trapezoidal SVF). */
    struct Svf
    {
        float ic1 = 0, ic2 = 0;

        inline float process (float x, float g, float k, int type) noexcept
        {
            const auto a1 = 1.0f / (1.0f + g * (g + k));
            const auto v3 = x - ic2;
            const auto v1 = a1 * ic1 + g * a1 * v3;
            const auto v2 = ic2 + g * v1;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;

            switch (type)
            {
                case 1:  return v1 * k;                    // band, peak at unity
                case 2:  return x - k * v1 - v2;           // high
                case 3:  return x - k * v1;                // notch
                default: return v2;                        // low
            }
        }

        void clear() noexcept   { ic1 = ic2 = 0.0f; }
    };

    /** A comb: a delay at the cutoff's period with feedback from the resonance. */
    struct Comb
    {
        std::vector<float> line;
        int write = 0;
        float damp = 0.0f;

        inline float process (float x, float period, float feedback) noexcept
        {
            const auto n = (int) line.size();
            if (n < 4) return x;
            auto readPos = (float) write - juce::jlimit (1.0f, (float) n - 2.0f, period);
            if (readPos < 0.0f) readPos += (float) n;
            if (readPos >= (float) n) readPos -= (float) n;   // a hair under 0 rounds up to n: that is the line's start
            const auto i = (int) readPos;
            const auto f = readPos - (float) i;
            const auto a = line[(size_t) i], b = line[(size_t) ((i + 1) % n)];
            const auto delayed = a + f * (b - a);
            damp += 0.5f * (delayed - damp);
            const auto y = x + feedback * damp;
            line[(size_t) write] = juce::jlimit (-4.0f, 4.0f, y);
            write = write + 1 == n ? 0 : write + 1;
            return y * (1.0f - 0.5f * std::abs (feedback));
        }

        void clear() noexcept   { std::fill (line.begin(), line.end(), 0.0f); damp = 0.0f; write = 0; }
    };

    struct FilterState
    {
        Ladder ladder;
        Svf svf;
        Comb comb;
        void clear() noexcept   { ladder.clear(); svf.clear(); comb.clear(); }
    };

    struct Unison
    {
        double phase = 0.0, increment = 0.0;
        Ramp gainL, gainR;
        int delay = 0;                     // samples until it starts (strum)
        float gate = 1.0f;                 // its fade-in after a strum's wait
        float drift = 0.0f, driftFrom = 0.0f, driftTo = 0.0f, driftT = 1.0f, driftStep = 0.0f;
        float last = 0.0f;                 // its last sample, for FM
        // The plucked string.
        float* line = nullptr;
        int lineSize = 0, write = 0, exciting = 0;
        float lowpass = 0.0f, period = 100.0f, excite = 0.0f;
        juce::uint32 noise = 1;
    };

    struct OscillatorState
    {
        std::array<Unison, maxUnison> unison;
        int level = 0;
        double warpRatio = 1.0;
    };
}

//==============================================================================
struct BuiltInEffects::BasicSynthProcessor::Engine
{
    explicit Engine (BasicSynthProcessor& p) : owner (p) {}

    BasicSynthProcessor& owner;

    //==========================================================================
    // The parameters it reads, found once by ID.
    struct OscParams
    {
        juce::AudioParameterBool* on;
        juce::AudioParameterChoice *table, *warp;
        juce::AudioParameterFloat *phase, *unison, *strum, *drift;
        juce::AudioParameterBool* randomPhase;
    };

    std::array<OscParams, numOscillators> oscP {};
    juce::AudioParameterChoice *subOctave = nullptr, *subShape = nullptr, *noiseType = nullptr, *routing = nullptr, *voiceMode = nullptr, *glideMode = nullptr;
    juce::AudioParameterBool* subDirect = nullptr;
    std::array<juce::AudioParameterChoice*, 2> filterType {};
    std::array<juce::AudioParameterFloat*, 2> keyTrack {};
    struct EnvParams { juce::AudioParameterFloat *attack, *decay, *sustain, *release, *curve; };
    std::array<EnvParams, numEnvelopes> envP {};
    juce::AudioParameterFloat *velocitySense = nullptr, *glideTime = nullptr, *bendRange = nullptr;
    struct LfoParams
    {
        juce::AudioParameterChoice *shape, *division, *mode;
        juce::AudioParameterBool* sync;
        juce::AudioParameterFloat *phase, *fade, *smooth;
        std::array<juce::AudioParameterFloat*, numSteps> steps;
    };
    std::array<LfoParams, numLfos> lfoP {};
    struct SlotParams { juce::AudioParameterChoice *source, *destination; juce::AudioParameterFloat* amount; };
    std::array<SlotParams, numSlots> slotP {};
    std::array<juce::AudioParameterFloat*, numMacros> macroP {};
    juce::AudioParameterBool *driveOn = nullptr, *chorusOn = nullptr, *phaserOn = nullptr, *delayOn = nullptr, *reverbOn = nullptr, *pingPong = nullptr;
    juce::AudioParameterChoice *driveType = nullptr, *delayDivision = nullptr;
    juce::AudioParameterFloat *driveTone = nullptr, *driveMix = nullptr, *chorusRate = nullptr, *chorusDepth = nullptr, *phaserRate = nullptr,
                              *phaserFeedback = nullptr, *delayTone = nullptr, *reverbSize = nullptr, *reverbDecay = nullptr, *reverbDamping = nullptr;
    std::array<juce::RangedAudioParameter*, numDestinations> destP {};

    template <typename T> T* find (const juce::String& id)
    {
        auto* p = dynamic_cast<T*> (owner.parameter (id));
        jassert (p != nullptr);
        return p;
    }

    void bind()
    {
        for (int o = 0; o < numOscillators; ++o)
        {
            const auto n = "osc" + juce::String (o + 1);
            oscP[(size_t) o] = { find<juce::AudioParameterBool> (n + "On"), find<juce::AudioParameterChoice> (n + "Table"), find<juce::AudioParameterChoice> (n + "Warp"),
                                 find<juce::AudioParameterFloat> (n + "Phase"), find<juce::AudioParameterFloat> (n + "Unison"), find<juce::AudioParameterFloat> (n + "UnisonStrum"),
                                 find<juce::AudioParameterFloat> (n + "UnisonDrift"), find<juce::AudioParameterBool> (n + "RandomPhase") };
        }

        subOctave = find<juce::AudioParameterChoice> ("subOctave");
        subShape = find<juce::AudioParameterChoice> ("subShape");
        subDirect = find<juce::AudioParameterBool> ("subDirect");
        noiseType = find<juce::AudioParameterChoice> ("noiseType");
        routing = find<juce::AudioParameterChoice> ("filterRouting");

        for (int f = 0; f < 2; ++f)
        {
            filterType[(size_t) f] = find<juce::AudioParameterChoice> ("filter" + juce::String (f + 1) + "Type");
            keyTrack[(size_t) f] = find<juce::AudioParameterFloat> ("filter" + juce::String (f + 1) + "KeyTrack");
        }

        for (int e = 0; e < numEnvelopes; ++e)
        {
            const auto n = "env" + juce::String (e + 1);
            envP[(size_t) e] = { find<juce::AudioParameterFloat> (n + "Attack"), find<juce::AudioParameterFloat> (n + "Decay"), find<juce::AudioParameterFloat> (n + "Sustain"),
                                 find<juce::AudioParameterFloat> (n + "Release"), find<juce::AudioParameterFloat> (n + "Curve") };
        }

        velocitySense = find<juce::AudioParameterFloat> ("ampVelocity");
        voiceMode = find<juce::AudioParameterChoice> ("voiceMode");
        glideMode = find<juce::AudioParameterChoice> ("glideMode");
        glideTime = find<juce::AudioParameterFloat> ("glideTime");
        bendRange = find<juce::AudioParameterFloat> ("bendRange");

        for (int l = 0; l < numLfos; ++l)
        {
            const auto n = "lfo" + juce::String (l + 1);
            auto& p = lfoP[(size_t) l];
            p.shape = find<juce::AudioParameterChoice> (n + "Shape");
            p.division = find<juce::AudioParameterChoice> (n + "Division");
            p.mode = find<juce::AudioParameterChoice> (n + "Mode");
            p.sync = find<juce::AudioParameterBool> (n + "Sync");
            p.phase = find<juce::AudioParameterFloat> (n + "Phase");
            p.fade = find<juce::AudioParameterFloat> (n + "Fade");
            p.smooth = find<juce::AudioParameterFloat> (n + "Smooth");

            for (int s = 0; s < numSteps; ++s)
                p.steps[(size_t) s] = find<juce::AudioParameterFloat> (n + "Step" + juce::String (s + 1));
        }

        for (int s = 0; s < numSlots; ++s)
        {
            const auto n = "mod" + juce::String (s + 1);
            slotP[(size_t) s] = { find<juce::AudioParameterChoice> (n + "Source"), find<juce::AudioParameterChoice> (n + "Destination"), find<juce::AudioParameterFloat> (n + "Amount") };
        }

        for (int m = 0; m < numMacros; ++m)
            macroP[(size_t) m] = find<juce::AudioParameterFloat> ("macro" + juce::String (m + 1));

        driveOn = find<juce::AudioParameterBool> ("fxDriveOn");
        driveType = find<juce::AudioParameterChoice> ("fxDriveType");
        driveTone = find<juce::AudioParameterFloat> ("fxDriveTone");
        driveMix = find<juce::AudioParameterFloat> ("fxDriveMix");
        chorusOn = find<juce::AudioParameterBool> ("fxChorusOn");
        chorusRate = find<juce::AudioParameterFloat> ("fxChorusRate");
        chorusDepth = find<juce::AudioParameterFloat> ("fxChorusDepth");
        phaserOn = find<juce::AudioParameterBool> ("fxPhaserOn");
        phaserRate = find<juce::AudioParameterFloat> ("fxPhaserRate");
        phaserFeedback = find<juce::AudioParameterFloat> ("fxPhaserFeedback");
        delayOn = find<juce::AudioParameterBool> ("fxDelayOn");
        delayDivision = find<juce::AudioParameterChoice> ("fxDelayDivision");
        pingPong = find<juce::AudioParameterBool> ("fxDelayPingPong");
        delayTone = find<juce::AudioParameterFloat> ("fxDelayTone");
        reverbOn = find<juce::AudioParameterBool> ("fxReverbOn");
        reverbSize = find<juce::AudioParameterFloat> ("fxReverbSize");
        reverbDecay = find<juce::AudioParameterFloat> ("fxReverbDecay");
        reverbDamping = find<juce::AudioParameterFloat> ("fxReverbDamping");

        for (int d = 0; d < numDestinations; ++d)
            destP[(size_t) d] = owner.parameter (destinationIds()[d]);
    }

    //==========================================================================
    // What every voice shares, taken at each control tick.
    struct Shared
    {
        std::array<float, numDestinations> base {};    // normalised
        std::array<bool, numOscillators> on {};
        std::array<int, numOscillators> table {}, warp {}, unison {};
        std::array<float, numOscillators> strumSamples {}, drift {}, phase {};
        std::array<bool, numOscillators> randomPhase {};
        int subOctave = 1, subShape = 0, noiseType = 0, routing = 0, voiceMode = 0, glideMode = 0;
        bool subDirect = false;
        std::array<int, 2> filterType {};
        std::array<float, 2> keyTrack {};
        std::array<EnvelopeSettings, numEnvelopes> env {};
        float velocitySense = 1.0f, glideSeconds = 0.0f, bendRange = 2.0f;
        struct Lfo { int shape = 0, mode = 0; bool sync = false; double beats = 1.0; float phase = 0.0f, fadeSamples = 0.0f, smooth = 0.0f; std::array<float, numSteps> steps {}; };
        std::array<Lfo, numLfos> lfo {};
        struct Slot { int source = 0, destination = -1; float amount = 0.0f; };
        std::array<Slot, numSlots> slots {};
        std::array<float, numMacros> macros {};
        bool anyModulation = false;
    };

    Shared shared;

    void readShared()
    {
        auto& s = shared;

        for (int d = 0; d < numDestinations; ++d)
            s.base[(size_t) d] = destP[(size_t) d]->getValue();

        for (int o = 0; o < numOscillators; ++o)
        {
            const auto& p = oscP[(size_t) o];
            s.on[(size_t) o] = p.on->get();
            s.table[(size_t) o] = p.table->getIndex();
            s.warp[(size_t) o] = p.warp->getIndex();
            s.unison[(size_t) o] = juce::jlimit (1, maxUnison, juce::roundToInt (p.unison->get()));
            s.strumSamples[(size_t) o] = p.strum->get() * 0.001f * (float) rate;
            s.drift[(size_t) o] = p.drift->get() * 0.01f;
            s.phase[(size_t) o] = p.phase->get() * 0.01f;
            s.randomPhase[(size_t) o] = p.randomPhase->get();
        }

        s.subOctave = subOctave->getIndex() + 1;
        s.subShape = subShape->getIndex();
        s.subDirect = subDirect->get();
        s.noiseType = noiseType->getIndex();
        s.routing = routing->getIndex();

        for (int f = 0; f < 2; ++f)
        {
            s.filterType[(size_t) f] = filterType[(size_t) f]->getIndex();
            s.keyTrack[(size_t) f] = keyTrack[(size_t) f]->get() * 0.01f;
        }

        for (int e = 0; e < numEnvelopes; ++e)
        {
            const auto& p = envP[(size_t) e];
            auto& out = s.env[(size_t) e];
            // The amp's never jumps: 1.5 ms at least to rise or fall, 3 ms to release.
            const auto least = e == 0 ? minimumAttackSeconds : 0.0;
            const auto seconds = [&] (juce::AudioParameterFloat* t, double minimum) { return juce::jmax (minimum, (double) t->get() * 0.001); };
            const auto stepFor = [this] (double secs) { return secs <= 0.0 ? 1.0f : (float) (1.0 / (secs * rate)); };
            const auto k = p.curve->get() * 0.01f * 6.0f;
            out.attack.set (stepFor (seconds (p.attack, least)), k * 0.5f);
            out.decay.set (stepFor (seconds (p.decay, least)), k);
            out.release.set (stepFor (seconds (p.release, e == 0 ? 0.003 : 0.0)), k);
            out.sustain = p.sustain->get() * 0.01f;
        }

        s.velocitySense = velocitySense->get() * 0.01f;
        s.voiceMode = voiceMode->getIndex();
        s.glideMode = glideMode->getIndex();
        s.glideSeconds = glideTime->get() * 0.001f;
        s.bendRange = bendRange->get();

        for (int l = 0; l < numLfos; ++l)
        {
            const auto& p = lfoP[(size_t) l];
            auto& out = s.lfo[(size_t) l];
            out.shape = p.shape->getIndex();
            out.mode = p.mode->getIndex();
            out.sync = p.sync->get();
            out.beats = divisionBeats (p.division->getIndex());
            out.phase = p.phase->get() * 0.01f;
            out.fadeSamples = p.fade->get() * 0.001f * (float) rate;
            out.smooth = p.smooth->get() * 0.01f;

            for (int i = 0; i < numSteps; ++i)
                out.steps[(size_t) i] = p.steps[(size_t) i]->get() * 0.01f;
        }

        s.anyModulation = false;

        for (int i = 0; i < numSlots; ++i)
        {
            const auto& p = slotP[(size_t) i];
            auto& out = s.slots[(size_t) i];
            out.source = p.source->getIndex();
            out.destination = p.destination->getIndex() - 1;
            out.amount = p.amount->get() * 0.01f;
            s.anyModulation = s.anyModulation || (out.source != 0 && out.destination >= 0 && out.amount != 0.0f);
        }

        for (int m = 0; m < numMacros; ++m)
            s.macros[(size_t) m] = macroP[(size_t) m]->get() * 0.01f;
    }

    //==========================================================================
    struct Voice
    {
        // Where each oscillator's position and warp are now: they move toward their modulated values no
        // faster than the amp may rise, so an envelope that jumps (a note struck again) doesn't change the
        // wave between two samples. `settle` puts them there at once, for a voice starting from silence.
        std::array<float, numOscillators> positionNow {}, warpNow {};
        bool settle = true;
        bool active = false, fading = false, down = false, held = false;
        int note = -1;
        float velocity = 0.0f, randomValue = 0.0f;
        juce::uint32 age = 0, serial = 0;
        double pitch = 60.0, target = 60.0, glidePerSample = 0.0;
        std::array<Envelope, numEnvelopes> env;
        std::array<double, numLfos> lfoPhase {};
        std::array<float, numLfos> lfoValue {}, lfoSmoothed {};
        juce::int64 sinceStart = 0;
        std::array<OscillatorState, numOscillators> osc;
        double subPhase = 0.0, subIncrement = 0.0;
        Rng noiseL, noiseR;
        float pinkL[3] {}, pinkR[3] {}, tiltL = 0, tiltR = 0, crackle = 0, holdL = 0, holdR = 0;
        double holdPhase = 0.0;
        std::array<std::array<FilterState, 2>, 2> filters;   // [filter][channel]
        float dcL = 0, dcR = 0, dcInL = 0, dcInR = 0;
        float fade = 1.0f, fadeStep = 0.0f;
        std::array<float, numDestinations> value {};       // its own settings this tick, normalised
        std::array<Ramp, numOscillators> oscGain, oscRoute;
        Ramp subGain, noiseGain, ampGain, panL, panR;
        std::array<Ramp, 2> cutoffG, resonance, drive;
        std::array<float, 2> combPeriod {};
        float noiseTilt = 0.0f;
    };

    std::array<Voice, numVoices + numFadingVoices> voices;
    std::vector<float> stringMemory;
    juce::uint32 counter = 0, notesStarted = 0;
    double rate = 48000.0, bpm = 120.0;
    juce::int64 clock = 0, lastTick = -1;
    std::array<double, numLfos> globalLfoPhase {};
    std::array<float, numLfos> globalLfoValue {};

    // The controllers, at rest.
    float bend = 0.0f, wheel = 0.0f, channelPressure = 0.0f, volume = 1.0f, expression = 1.0f, ccPan = 0.5f, brightness = 1.0f, ccResonance = 0.0f;
    bool pedal = false;
    std::array<float, 128> polyPressure {};
    std::vector<int> heldNotes;   // for mono and legato: the keys down, oldest first
    int lastNote = -1;
    Ramp outGain;

    //==========================================================================
    void prepare (double sampleRate)
    {
        rate = sampleRate;
        // Each string's delay line holds a period of 20 Hz.
        const auto stringSize = (int) std::ceil (rate / 20.0) + 8;
        stringMemory.assign ((size_t) stringSize * maxUnison * numOscillators * voices.size(), 0.0f);
        const auto combSize = (int) std::ceil (rate / 20.0) + 8;
        size_t at = 0;

        for (auto& v : voices)
        {
            for (auto& o : v.osc)
                for (auto& u : o.unison)
                {
                    u.line = stringMemory.data() + at;
                    u.lineSize = stringSize;
                    at += (size_t) stringSize;
                }

            for (auto& f : v.filters)
                for (auto& ch : f)
                {
                    ch.comb.line.assign ((size_t) combSize, 0.0f);
                }
        }

        prepareEffects();
        reset();
    }

    void reset()
    {
        for (auto& v : voices)
        {
            v.active = v.fading = v.down = v.held = false;
            v.note = -1;
            for (auto& f : v.filters) for (auto& ch : f) ch.clear();
            for (auto& o : v.osc) for (auto& u : o.unison) if (u.line != nullptr) std::fill (u.line, u.line + u.lineSize, 0.0f);
        }

        bend = wheel = channelPressure = 0.0f;
        volume = expression = 1.0f;
        ccPan = 0.5f;
        brightness = 1.0f;
        ccResonance = 0.0f;
        pedal = false;
        polyPressure.fill (0.0f);
        heldNotes.clear();
        lastNote = -1;
        clock = 0;
        lastTick = -1;
        counter = notesStarted = 0;
        globalLfoPhase.fill (0.0);
        globalLfoValue.fill (0.0f);
        outGain.to (1.0f, true);
        resetEffects();
    }

    int sounding() const
    {
        return (int) std::count_if (voices.begin(), voices.end(), [] (const Voice& v) { return v.active && ! v.fading; });
    }

    //==========================================================================
    // Notes.
    Voice* freeVoice()
    {
        for (auto& v : voices)
            if (! v.active)
                return &v;

        // Every slot busy, most of them fading: the fade nearest its end is cut (nearly silent by now).
        Voice* quietest = nullptr;

        for (auto& v : voices)
            if (v.fading && (quietest == nullptr || v.fade < quietest->fade))
                quietest = &v;

        return quietest != nullptr ? quietest : &voices[0];
    }

    void fadeOut (Voice& v)
    {
        if (! v.active || v.fading)
            return;

        v.fading = true;
        v.fadeStep = (float) (1.0 / (fadeSeconds * rate));
        v.down = v.held = false;
    }

    void startFresh (Voice& v, int note, float velocity)
    {
        v.settle = true;
        v.active = true;
        v.fading = false;
        v.fade = 1.0f;
        v.fadeStep = 0.0f;
        v.sinceStart = 0;
        v.serial = ++notesStarted;
        v.randomValue = hashNoise (v.serial * 7919u + 17u);

        for (auto& e : v.env)
            e = {};

        for (auto& f : v.filters)
            for (auto& ch : f)
                ch.clear();

        v.dcL = v.dcR = v.dcInL = v.dcInR = 0.0f;
        v.noiseL.state = 0x9e3779b9u ^ (v.serial * 2654435761u);
        v.noiseR.state = 0x7f4a7c15u ^ (v.serial * 40503u);
        v.lfoSmoothed.fill (0.0f);

        // Each oscillator's unison voices: their start phases, fixed or spread at random (the same
        // every time the song plays from its start).
        for (int o = 0; o < numOscillators; ++o)
            for (int u = 0; u < maxUnison; ++u)
            {
                auto& un = v.osc[(size_t) o].unison[(size_t) u];
                const auto seed = v.serial * 977u + (juce::uint32) (o * 31 + u * 7);
                un.phase = shared.randomPhase[(size_t) o] ? (double) (hashNoise (seed) * 0.5f + 0.5f) : (double) shared.phase[(size_t) o];
                un.drift = un.driftFrom = un.driftTo = 0.0f;
                un.driftT = 1.0f;
                un.last = 0.0f;
                un.lowpass = 0.0f;
                un.excite = 0.0f;
                un.exciting = 0;
                un.write = 0;
                un.noise = seed | 1u;

                if (un.line != nullptr && shared.table[(size_t) o] == (int) WaveTables::Table::string)
                    std::fill (un.line, un.line + un.lineSize, 0.0f);
            }

        v.subPhase = 0.0;
        v.holdPhase = 0.0;
        v.pitch = v.target = note;

        // Glide in poly mode, set to Always: from the last note played.
        if (shared.glideMode == 1 && shared.glideSeconds > 0.0f && lastNote >= 0)
            v.pitch = lastNote;

        trigger (v, note, velocity, true);
    }

    /** Starts (or starts again) a voice's note: envelopes from where they are, LFOs set to restart,
        strings plucked and each unison voice's strum wait set. */
    void trigger (Voice& v, int note, float velocity, bool fresh)
    {
        v.note = note;
        v.velocity = velocity;
        v.down = true;
        v.held = false;
        v.age = ++counter;
        v.target = note;
        setGlide (v);

        for (auto& e : v.env)
            e.trigger();

        for (int l = 0; l < numLfos; ++l)
            if (fresh || shared.lfo[(size_t) l].mode != 0)
                v.lfoPhase[(size_t) l] = shared.lfo[(size_t) l].phase;

        v.sinceStart = 0;

        for (int o = 0; o < numOscillators; ++o)
        {
            const auto n = shared.unison[(size_t) o];

            for (int u = 0; u < maxUnison; ++u)
            {
                auto& un = v.osc[(size_t) o].unison[(size_t) u];
                const auto wait = n > 1 ? juce::roundToInt (shared.strumSamples[(size_t) o] * (float) u / (float) (n - 1)) : 0;
                const auto string = shared.table[(size_t) o] == (int) WaveTables::Table::string;

                // A string is plucked again after its wait, ringing on meanwhile. A wave waits only on a
                // fresh note, silent until then, and fades in when it starts; a sounding one carries on.
                if (string)
                {
                    un.delay = wait;
                    if (wait == 0) un.exciting = -1;
                }
                else
                {
                    un.delay = fresh ? wait : 0;
                    if (fresh && wait > 0) un.gate = 0.0f;
                }
            }
        }

        computeVoice (v, fresh);
    }

    void setGlide (Voice& v)
    {
        if (shared.glideSeconds > 0.0f && std::abs (v.target - v.pitch) > 1.0e-6)
            v.glidePerSample = std::abs (v.target - v.pitch) / juce::jmax (1.0, (double) shared.glideSeconds * rate);
        else
        {
            v.pitch = v.target;
            v.glidePerSample = 0.0;
        }
    }

    Voice* monoVoice()
    {
        Voice* newest = nullptr;

        for (auto& v : voices)
            if (v.active && ! v.fading && (newest == nullptr || v.age > newest->age))
                newest = &v;

        return newest;
    }

    void noteOn (int note, float velocity)
    {
        const auto mode = shared.voiceMode;   // 0 poly, 1 mono, 2 legato

        if (mode != 0)
        {
            const auto wasHeld = ! heldNotes.empty();
            heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
            heldNotes.push_back (note);

            if (auto* v = monoVoice())
            {
                // Legato with a key still down: slide there, the envelopes going on.
                if (mode == 2 && wasHeld && v->down)
                {
                    v->note = note;
                    v->target = note;
                    v->velocity = velocity;
                    v->age = ++counter;
                    setGlide (*v);
                }
                else
                {
                    // Struck again: it glides from where it was only when set to Always, or over a held key.
                    trigger (*v, note, velocity, false);

                    if (! (shared.glideMode == 1 || wasHeld))
                    {
                        v->pitch = v->target;
                        v->glidePerSample = 0.0;
                        computeVoice (*v, false);
                    }
                }

                // Any other voice (left from poly mode) goes.
                for (auto& other : voices)
                    if (&other != v)
                        fadeOut (other);

                lastNote = note;
                return;
            }

            startFresh (*freeVoice(), note, velocity);
            lastNote = note;
            return;
        }

        // Poly: the voice already playing the note, struck again in place.
        for (auto& v : voices)
            if (v.active && ! v.fading && v.note == note)
            {
                trigger (v, note, velocity, false);
                lastNote = note;
                return;
            }

        // All sixteen sounding: the oldest (one released first, if any) fades out to make room.
        if (sounding() >= numVoices)
        {
            Voice* oldest = nullptr;

            for (auto& v : voices)
                if (v.active && ! v.fading)
                {
                    const auto releasing = v.env[0].stage == Envelope::release;
                    const auto oldestReleasing = oldest != nullptr && oldest->env[0].stage == Envelope::release;

                    if (oldest == nullptr || (releasing && ! oldestReleasing) || (releasing == oldestReleasing && v.age < oldest->age))
                        oldest = &v;
                }

            if (oldest != nullptr)
                fadeOut (*oldest);
        }

        startFresh (*freeVoice(), note, velocity);
        lastNote = note;
    }

    void noteOff (int note)
    {
        if (shared.voiceMode != 0)
        {
            heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());

            if (auto* v = monoVoice(); v != nullptr && v->note == note && v->down)
            {
                if (! heldNotes.empty())
                {
                    // Back to the key still held, as a monosynth does: sliding in legato, struck in mono.
                    const auto back = heldNotes.back();

                    if (shared.voiceMode == 2)
                    {
                        v->note = back;
                        v->target = back;
                        setGlide (*v);
                    }
                    else
                        trigger (*v, back, v->velocity, false);

                    return;
                }

                release (*v);
            }

            return;
        }

        for (auto& v : voices)
            if (v.active && ! v.fading && v.note == note && v.down)
                release (v);
    }

    void release (Voice& v)
    {
        v.down = false;

        if (pedal)
        {
            v.held = true;
            return;
        }

        for (auto& e : v.env)
            e.releaseNow();
    }

    void allOff (bool sound)
    {
        heldNotes.clear();

        for (auto& v : voices)
        {
            if (sound)
                fadeOut (v);
            else if (v.active && ! v.fading)
            {
                v.held = false;
                v.down = false;
                for (auto& e : v.env) e.releaseNow();
            }
        }
    }

    void controller (const juce::MidiMessage& m)
    {
        if (m.isPitchWheel())
        {
            bend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
            return;
        }

        if (m.isChannelPressure())
        {
            channelPressure = (float) m.getChannelPressureValue() / 127.0f;
            return;
        }

        if (m.isAftertouch())
        {
            polyPressure[(size_t) juce::jlimit (0, 127, m.getNoteNumber())] = (float) m.getAfterTouchValue() / 127.0f;
            return;
        }

        if (! m.isController())
            return;

        const auto v = (float) m.getControllerValue() / 127.0f;

        switch (m.getControllerNumber())
        {
            case 1:  wheel = v; break;
            case 7:  volume = v; break;
            case 10: ccPan = v; break;
            case 11: expression = v; break;
            case 71: ccResonance = v; break;
            case 74: brightness = v; break;
            case 64:
                pedal = m.getControllerValue() >= 64;
                if (! pedal)
                    for (auto& voice : voices)
                        if (std::exchange (voice.held, false))
                            for (auto& e : voice.env) e.releaseNow();
                break;
            case 121:
                bend = wheel = channelPressure = 0.0f;
                volume = expression = brightness = 1.0f;
                ccPan = 0.5f;
                ccResonance = 0.0f;
                polyPressure.fill (0.0f);
                pedal = false;
                for (auto& voice : voices)
                    if (std::exchange (voice.held, false))
                        for (auto& e : voice.env) e.releaseNow();
                break;
            default: break;
        }
    }

    //==========================================================================
    // Modulation.
    static float lfoShape (int shape, double phase, const std::array<float, numSteps>& steps) noexcept
    {
        const auto p = (float) (phase - std::floor (phase));
        const auto cycle = (juce::uint32) (juce::int64) std::floor (phase);

        switch (shape)
        {
            case 0: return std::sin (twoPiF * p);
            case 1: return p < 0.25f ? 4.0f * p : p < 0.75f ? 2.0f - 4.0f * p : 4.0f * p - 4.0f;
            case 2: return 2.0f * p - 1.0f;
            case 3: return 1.0f - 2.0f * p;
            case 4: return p < 0.5f ? 1.0f : -1.0f;
            case 5: return hashNoise (cycle * 2654435761u + 11u);
            case 6:
            {
                const auto a = hashNoise (cycle * 2654435761u + 11u), b = hashNoise ((cycle + 1) * 2654435761u + 11u);
                const auto t = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi * p);
                return a + (b - a) * t;
            }
            default: return steps[(size_t) juce::jlimit (0, numSteps - 1, (int) (p * numSteps))];
        }
    }

    double lfoHz (int l, float rateNormalised) const
    {
        const auto& s = shared.lfo[(size_t) l];

        if (s.sync)
            return bpm / 60.0 / s.beats;

        return destP[(size_t) (dLfo1Rate + l)]->convertFrom0to1 (rateNormalised);
    }

    float sourceValue (const Voice* v, int source) const noexcept
    {
        switch (source)
        {
            case env1: case env2: case env3: case env4: return v != nullptr ? v->env[(size_t) (source - env1)].level : 0.0f;
            case lfo1: case lfo2: return v != nullptr ? v->lfoValue[(size_t) (source - lfo1)] : globalLfoValue[(size_t) (source - lfo1)];
            case velocity:   return v != nullptr ? v->velocity : 0.0f;
            case modWheel:   return wheel;
            case aftertouch: return juce::jmax (channelPressure, v != nullptr && v->note >= 0 ? polyPressure[(size_t) v->note] : 0.0f);
            case key:        return v != nullptr ? juce::jlimit (-1.0f, 1.0f, (float) (v->note - 60) / 60.0f) : 0.0f;
            case macro1: case macro2: case macro3: case macro4: return shared.macros[(size_t) (source - macro1)];
            case WaveSynth::random:     return v != nullptr ? v->randomValue : 0.0f;
            default:         return 0.0f;
        }
    }

    void modulate (const Voice* v, std::array<float, numDestinations>& value) const noexcept
    {
        value = shared.base;

        if (shared.anyModulation)
            for (auto& slot : shared.slots)
                if (slot.source != 0 && slot.destination >= 0 && slot.destination < numDestinations)
                    value[(size_t) slot.destination] += slot.amount * sourceValue (v, slot.source);

        // Cutoff (CC 74) takes both filters down as far as eight octaves; Resonance (CC 71) up toward the most.
        if (brightness < 1.0f || ccResonance > 0.0f)
            for (auto [c, r] : { std::pair { dF1Cutoff, dF1Res }, std::pair { dF2Cutoff, dF2Res } })
            {
                value[(size_t) c] -= (1.0f - brightness) * 0.8f;
                value[(size_t) r] += ccResonance * (1.0f - value[(size_t) r]);
            }

        for (auto& x : value)
            x = juce::jlimit (0.0f, 1.0f, x);
    }

    float plain (int d, float normalised) const   { return destP[(size_t) d]->convertFrom0to1 (normalised); }

    /** A voice's settings for the next 16 samples: modulation, pitches, gains and filters, each
        ramped from where it was (or set at once for a note just started). */
    void computeVoice (Voice& v, bool jump)
    {
        // The LFOs first (their rates may be modulated: by last tick's values).
        for (int l = 0; l < numLfos; ++l)
        {
            const auto& s = shared.lfo[(size_t) l];
            double phase;

            if (s.mode == 0)
                phase = globalLfoPhase[(size_t) l];
            else
            {
                phase = v.lfoPhase[(size_t) l];
                if (s.mode == 2) phase = juce::jmin (phase, (double) s.phase + 0.999999);   // once through, then holds
            }

            auto out = lfoShape (s.shape, phase, s.steps);

            if (s.shape == 7 && s.smooth > 0.0f)
            {
                const auto c = 1.0f - s.smooth * 0.97f;
                v.lfoSmoothed[(size_t) l] += c * (out - v.lfoSmoothed[(size_t) l]);
                out = v.lfoSmoothed[(size_t) l];
            }
            else
                v.lfoSmoothed[(size_t) l] = out;

            if (s.fadeSamples > 0.0f)
                out *= juce::jmin (1.0f, (float) v.sinceStart / s.fadeSamples);

            v.lfoValue[(size_t) l] = out;
        }

        modulate (&v, v.value);
        const auto& value = v.value;

        // Pitch: the glide's, the bend's, each oscillator's own, each unison voice's detune and drift.
        const auto bendSemis = bend * shared.bendRange;

        for (int o = 0; o < numOscillators; ++o)
        {
            auto& os = v.osc[(size_t) o];
            const auto n = shared.unison[(size_t) o];
            const auto semis = plain (oscDest (o, fPitch), value[(size_t) oscDest (o, fPitch)]);
            const auto cents = plain (oscDest (o, fFine), value[(size_t) oscDest (o, fFine)]);
            const auto detune = plain (oscDest (o, fDetune), value[(size_t) oscDest (o, fDetune)]);
            const auto spread = value[(size_t) oscDest (o, fSpread)];
            const auto pan = plain (oscDest (o, fPan), value[(size_t) oscDest (o, fPan)]) * 0.01f;
            const auto warpAmount = value[(size_t) oscDest (o, fWarp)];
            const auto unisonGain = 1.0f / std::sqrt ((float) n);
            const auto drift = shared.drift[(size_t) o];
            const auto string = shared.table[(size_t) o] == (int) WaveTables::Table::string;
            os.warpRatio = shared.warp[(size_t) o] == (int) Warp::sync ? 1.0 + warpAmount * 7.0 : 1.0;

            for (int u = 0; u < n; ++u)
            {
                auto& un = os.unison[(size_t) u];
                const auto position = n > 1 ? (float) u / (float) (n - 1) * 2.0f - 1.0f : 0.0f;

                // Drift: a slow wander to a new pitch every quarter to half second, its own for each voice.
                if (drift > 0.0f)
                {
                    if (un.driftT >= 1.0f)
                    {
                        un.driftFrom = un.drift;
                        const auto seed = v.serial * 131u + (juce::uint32) (o * 17 + u) * 7u + (juce::uint32) (v.sinceStart >> 12);
                        un.driftTo = hashNoise (seed);
                        un.driftT = 0.0f;
                        un.driftStep = (float) controlInterval / (float) (rate * (0.25 + 0.25 * (hashNoise (seed ^ 0x55u) * 0.5 + 0.5)));
                    }

                    un.driftT = juce::jmin (1.0f, un.driftT + un.driftStep);
                    const auto smooth = un.driftT * un.driftT * (3.0f - 2.0f * un.driftT);
                    un.drift = un.driftFrom + (un.driftTo - un.driftFrom) * smooth;
                }
                else
                    un.drift = 0.0f;

                const auto voiceCents = cents + position * detune + un.drift * drift * 25.0f;
                const auto note = v.pitch + semis + bendSemis + voiceCents / 100.0;
                const auto hz = 440.0 * std::exp2 ((note - 69.0) / 12.0);
                un.increment = hz / rate;
                un.period = (float) juce::jlimit (2.0, (double) un.lineSize - 4.0, rate / hz);

                float l, r;
                panGains (pan + position * spread, l, r);
                un.gainL.to (l * unisonGain * (string ? 1.6f : 1.0f), jump);
                un.gainR.to (r * unisonGain * (string ? 1.6f : 1.0f), jump);
            }

            os.level = WaveTables::levelFor (os.unison[0].increment * os.warpRatio * (1.0 + detune / 1200.0));
            const auto oscOn = shared.on[(size_t) o];
            const auto oscVolume = value[(size_t) oscDest (o, fVolume)];
            v.oscGain[(size_t) o].to (oscOn ? oscVolume * oscVolume : 0.0f, jump);
            v.oscRoute[(size_t) o].to (value[(size_t) oscDest (o, fRoute)], jump);
        }

        v.subIncrement = 440.0 * std::exp2 ((v.pitch + bendSemis - 69.0) / 12.0 - shared.subOctave) / rate;
        const auto sub = value[(size_t) dSubLevel], noise = value[(size_t) dNoiseLevel];
        v.subGain.to (sub * sub, jump);
        v.noiseGain.to (noise * noise * 0.5f, jump);
        v.noiseTilt = plain (dNoiseColour, value[(size_t) dNoiseColour]) * 0.01f;

        // The filters: cutoff with key tracking (from middle C), resonance and drive.
        for (int f = 0; f < 2; ++f)
        {
            const auto base = f == 0 ? dF1Cutoff : dF2Cutoff;
            auto hz = plain (base, value[(size_t) base]) * std::exp2 ((float) (v.pitch - 60.0) / 12.0f * shared.keyTrack[(size_t) f]);
            hz = juce::jlimit (16.0f, (float) (rate * 0.45), hz);
            v.cutoffG[(size_t) f].to (std::tan (juce::MathConstants<float>::pi * hz / (float) rate), jump);
            v.combPeriod[(size_t) f] = (float) (rate / hz);
            v.resonance[(size_t) f].to (value[(size_t) base + 1], jump);
            v.drive[(size_t) f].to (juce::Decibels::decibelsToGain (value[(size_t) base + 2] * 24.0f), jump);
        }

        // The amp: level (dB), velocity, pan with the controller's.
        const auto levelDb = plain (dAmpLevel, value[(size_t) dAmpLevel]);
        const auto velocityGain = 1.0f - shared.velocitySense + shared.velocitySense * v.velocity;
        v.ampGain.to (juce::Decibels::decibelsToGain (levelDb, -100.0f) * velocityGain, jump);
        float l, r;
        panGains (plain (dAmpPan, value[(size_t) dAmpPan]) * 0.01f + (ccPan - 0.5f) * 2.0f, l, r);
        v.panL.to (l, jump);
        v.panR.to (r, jump);
    }

    //==========================================================================
    // Rendering.
    inline float oscillatorSample (Voice& v, int o, Unison& un, int warp, float warpAmount, float position, const WaveTables::Level& level) noexcept
    {
        auto phase = un.phase;

        switch ((Warp) warp)
        {
            case Warp::sync:    phase = phase * v.osc[(size_t) o].warpRatio; phase -= std::floor (phase); break;
            case Warp::bend:
            {
                const auto q = (float) phase * 2.0f - 1.0f, k = warpAmount * 6.0f;
                phase = (double) ((q * (1.0f + k) / (1.0f + k * std::abs (q))) * 0.5f + 0.5f);
                break;
            }
            case Warp::squeeze:
            {
                const auto w = 0.5f - 0.47f * warpAmount;
                const auto p = (float) phase;
                phase = p < w ? (double) (p * 0.5f / w) : (double) (0.5f + (p - w) * 0.5f / (1.0f - w));
                break;
            }
            case Warp::mirror:
            {
                const auto p = (float) phase;
                const auto m = p < 0.5f ? p * 2.0f : (1.0f - p) * 2.0f;
                phase = (double) (p + warpAmount * (m - p));
                break;
            }
            case Warp::fm:
            {
                const auto& from = v.osc[(size_t) ((o + numOscillators - 1) % numOscillators)].unison[(size_t) (&un - v.osc[(size_t) o].unison.data())];
                phase += (double) (warpAmount * 1.5f * from.last);
                phase -= std::floor (phase);
                break;
            }
            case Warp::off: case Warp::crush: case Warp::fold: break;
        }

        auto s = WaveTables::read (level, position, phase);

        if (warp == (int) Warp::crush && warpAmount > 0.0f)
        {
            const auto steps = std::exp2 (8.0f - warpAmount * 7.0f);
            s = std::round (s * steps) / steps;
        }
        else if (warp == (int) Warp::fold && warpAmount > 0.0f)
            s = fold (s * (1.0f + warpAmount * 5.0f));

        return s;
    }

    inline float stringSample (Unison& un, float bright, float decayAmount) noexcept
    {
        // Karplus-Strong: a burst of noise into a delay line a period long, round and round
        // through a low-pass that loses a little each time.
        if (un.exciting == -1)
        {
            un.exciting = juce::jmax (2, (int) un.period);
            un.excite = 0.0f;
        }

        float in = 0.0f;

        if (un.exciting > 0)
        {
            un.noise = un.noise * 1664525u + 1013904223u;
            const auto white = (float) (un.noise >> 8) * (2.0f / 16777216.0f) - 1.0f;
            un.excite += (0.15f + 0.85f * bright) * (white - un.excite);
            in = un.excite * 0.8f;
            --un.exciting;
        }

        const auto c = 0.12f + 0.88f * bright;
        // The low-pass delays the loop by about (1 - c) / c samples: taken off the line, so it stays in tune.
        const auto d = juce::jlimit (1.0f, (float) un.lineSize - 3.0f, un.period - (1.0f - c) / c);
        auto read = (float) un.write - d;
        if (read < 0.0f) read += (float) un.lineSize;
        if (read >= (float) un.lineSize) read -= (float) un.lineSize;   // a hair under 0 rounds up to the size
        const auto i = (int) read;
        const auto f = read - (float) i;
        const auto a = un.line[i], b = un.line[i + 1 == un.lineSize ? 0 : i + 1];
        const auto y = a + f * (b - a);
        un.lowpass += c * (y - un.lowpass);
        un.line[un.write] = in + decayAmount * un.lowpass;
        un.write = un.write + 1 == un.lineSize ? 0 : un.write + 1;
        return un.lowpass;
    }

    inline float filterSample (FilterState& st, int type, float x, float g, float res, float drive, float period) noexcept
    {
        if (type == (int) FilterType::off)
            return x;

        // Drive: up to 24 dB into a soft clip, then back down by half of it so it thickens more than
        // it gets louder. `drive` is that gain (1 for none), ramped.
        if (drive > 1.001f)
            x = softClip (x * drive) / std::sqrt (drive);

        switch ((FilterType) type)
        {
            case FilterType::ladder24: case FilterType::ladder12: case FilterType::ladderHigh: case FilterType::ladderBand:
            {
                const auto k = res * 4.1f;
                const auto mode = type - (int) FilterType::ladder24;
                return st.ladder.process (x * (mode <= 1 ? 1.0f + 0.6f * k : 1.0f), g, k, mode);
            }
            case FilterType::svfLow: case FilterType::svfBand: case FilterType::svfHigh: case FilterType::svfNotch:
            {
                const auto k = 2.0f - 1.97f * res;   // damping: 2 (none) down to a sharp peak
                return st.svf.process (x, g, k, type - (int) FilterType::svfLow);
            }
            case FilterType::comb:
                return st.comb.process (x, period, res * 0.97f);
            case FilterType::off:
            default:
                return x;
        }
    }

    void renderVoice (Voice& v, float* outL, float* outR, int count)
    {
        const auto& s = shared;
        const auto serial = s.routing == 0;
        std::array<const WaveTables::Level*, numOscillators> levels {};
        std::array<float, numOscillators> positions {}, warpAmounts {}, decays {};

        for (int o = 0; o < numOscillators; ++o)
        {
            levels[(size_t) o] = &WaveTables::get ((WaveTables::Table) s.table[(size_t) o]).levels[(size_t) v.osc[(size_t) o].level];
            positions[(size_t) o] = v.value[(size_t) oscDest (o, fPosition)];
            warpAmounts[(size_t) o] = v.value[(size_t) oscDest (o, fWarp)];

            if (v.settle)
            {
                v.positionNow[(size_t) o] = positions[(size_t) o];
                v.warpNow[(size_t) o] = warpAmounts[(size_t) o];
            }
            // A string's warp is how long it rings: 0.15 s up to 15 s to fall 60 dB.
            const auto t60 = 0.15f * std::pow (100.0f, warpAmounts[(size_t) o]);
            const auto hz = (float) (v.osc[(size_t) o].unison[0].increment * rate);
            decays[(size_t) o] = std::pow (10.0f, -3.0f / juce::jmax (0.01f, t60 * juce::jmax (1.0f, hz)));
        }

        v.settle = false;
        const auto slew = (float) (1.0 / (minimumAttackSeconds * rate));
        const auto& subTable = WaveTables::get (s.subShape == 2 ? WaveTables::Table::sub : WaveTables::Table::analog);
        const auto subPosition = s.subShape == 0 ? 0.0f : s.subShape == 1 ? 1.0f / 3.0f : 1.0f;
        const auto& subLevel = subTable.levels[(size_t) WaveTables::levelFor (v.subIncrement)];
        auto& e = v.env;

        for (int i = 0; i < count; ++i)
        {
            for (int k = 0; k < numEnvelopes; ++k)
                advance (e[(size_t) k], s.env[(size_t) k]);

            float f1L = 0, f1R = 0, f2L = 0, f2R = 0, directL = 0, directR = 0;

            for (int o = 0; o < numOscillators; ++o)
            {
                const auto gain = v.oscGain[(size_t) o].next();
                const auto route = v.oscRoute[(size_t) o].next();
                auto& os = v.osc[(size_t) o];
                const auto n = s.unison[(size_t) o];

                if (gain <= 0.0f && v.oscGain[(size_t) o].step == 0.0f)
                {
                    for (int u = 0; u < n; ++u) { os.unison[(size_t) u].gainL.next(); os.unison[(size_t) u].gainR.next(); }
                    continue;
                }

                const auto string = s.table[(size_t) o] == (int) WaveTables::Table::string;
                float sumL = 0.0f, sumR = 0.0f;
                const auto position = v.positionNow[(size_t) o] += juce::jlimit (-slew, slew, positions[(size_t) o] - v.positionNow[(size_t) o]);
                const auto warpAmount = v.warpNow[(size_t) o] += juce::jlimit (-slew, slew, warpAmounts[(size_t) o] - v.warpNow[(size_t) o]);

                for (int u = 0; u < n; ++u)
                {
                    auto& un = os.unison[(size_t) u];
                    const auto gl = un.gainL.next(), gr = un.gainR.next();

                    float x;

                    if (string)
                    {
                        if (un.delay > 0 && --un.delay == 0)
                            un.exciting = -1;

                        x = stringSample (un, position, decays[(size_t) o]);
                    }
                    else if (un.delay > 0)
                    {
                        --un.delay;
                        un.phase += un.increment;
                        un.phase -= std::floor (un.phase);
                        continue;
                    }
                    else
                    {
                        x = oscillatorSample (v, o, un, s.warp[(size_t) o], warpAmount, position, *levels[(size_t) o]);
                        un.phase += un.increment;
                        un.phase -= std::floor (un.phase);
                    }

                    if (un.gate < 1.0f)
                    {
                        un.gate = juce::jmin (1.0f, un.gate + (float) (1.0 / (minimumAttackSeconds * rate)));
                        x *= un.gate;
                    }

                    un.last = x;
                    sumL += x * gl;
                    sumR += x * gr;
                }

                sumL *= gain;
                sumR *= gain;
                f1L += sumL * (1.0f - route);
                f1R += sumR * (1.0f - route);
                f2L += sumL * route;
                f2R += sumR * route;
            }

            // The sub, after the oscillators' unison: one voice, centred.
            if (const auto sub = v.subGain.next(); sub > 0.0f || v.subGain.step != 0.0f)
            {
                const auto x = WaveTables::read (subLevel, subPosition, v.subPhase) * sub;
                v.subPhase += v.subIncrement;
                v.subPhase -= std::floor (v.subPhase);
                if (s.subDirect) { directL += x; directR += x; }
                else { f1L += x; f1R += x; }
            }
            else
            {
                v.subPhase += v.subIncrement;
                v.subPhase -= std::floor (v.subPhase);
            }

            // Noise: white, pink, crackle or digital (held at the note's pitch), tilted dark or bright; its own on each side.
            if (const auto noise = v.noiseGain.next(); noise > 0.0f || v.noiseGain.step != 0.0f)
            {
                float nl = v.noiseL.next(), nr = v.noiseR.next();

                switch (s.noiseType)
                {
                    case 1:
                        v.pinkL[0] = 0.99765f * v.pinkL[0] + nl * 0.0990460f; v.pinkL[1] = 0.96300f * v.pinkL[1] + nl * 0.2965164f; v.pinkL[2] = 0.57000f * v.pinkL[2] + nl * 1.0526913f;
                        v.pinkR[0] = 0.99765f * v.pinkR[0] + nr * 0.0990460f; v.pinkR[1] = 0.96300f * v.pinkR[1] + nr * 0.2965164f; v.pinkR[2] = 0.57000f * v.pinkR[2] + nr * 1.0526913f;
                        nl = (v.pinkL[0] + v.pinkL[1] + v.pinkL[2] + nl * 0.1848f) * 0.25f;
                        nr = (v.pinkR[0] + v.pinkR[1] + v.pinkR[2] + nr * 0.1848f) * 0.25f;
                        break;
                    case 2:
                        v.crackle *= 0.995f;
                        if (std::abs (nl) > 0.996f) v.crackle = nl > 0.0f ? 1.0f : -1.0f;
                        nl = v.crackle * (0.5f + 0.5f * nr);
                        nr = v.crackle * (0.5f - 0.5f * nr);
                        break;
                    case 3:
                        v.holdPhase += v.subIncrement * 8.0 * std::exp2 (2.0 * v.noiseTilt);
                        if (v.holdPhase >= 1.0) { v.holdPhase -= std::floor (v.holdPhase); v.holdL = nl; v.holdR = nr; }
                        nl = v.holdL;
                        nr = v.holdR;
                        break;
                    default: break;
                }

                if (s.noiseType != 3)
                {
                    v.tiltL += 0.15f * (nl - v.tiltL);
                    v.tiltR += 0.15f * (nr - v.tiltR);
                    const auto t = v.noiseTilt;
                    nl = t < 0.0f ? nl + (v.tiltL - nl) * -t : nl + (nl - v.tiltL - nl) * t * 0.85f;
                    nr = t < 0.0f ? nr + (v.tiltR - nr) * -t : nr + (nr - v.tiltR - nr) * t * 0.85f;
                }

                const auto route = v.value[(size_t) oscDest (0, fRoute)];   // follows oscillator 1's route
                f1L += nl * noise * (1.0f - route); f1R += nr * noise * (1.0f - route);
                f2L += nl * noise * route;          f2R += nr * noise * route;
            }

            // The filters, in series (filter 2 after filter 1, sources routed to 2 joining there) or side by side.
            const auto g1 = v.cutoffG[0].next(), g2 = v.cutoffG[1].next();
            const auto r1 = v.resonance[0].next(), r2 = v.resonance[1].next();
            const auto d1 = v.drive[0].next(), d2 = v.drive[1].next();
            auto& fs = v.filters;
            float l, r;

            if (serial)
            {
                l = filterSample (fs[0][0], s.filterType[0], f1L, g1, r1, d1, v.combPeriod[0]) + f2L;
                r = filterSample (fs[0][1], s.filterType[0], f1R, g1, r1, d1, v.combPeriod[0]) + f2R;
                l = filterSample (fs[1][0], s.filterType[1], l, g2, r2, d2, v.combPeriod[1]);
                r = filterSample (fs[1][1], s.filterType[1], r, g2, r2, d2, v.combPeriod[1]);
            }
            else
            {
                l = filterSample (fs[0][0], s.filterType[0], f1L, g1, r1, d1, v.combPeriod[0]) + filterSample (fs[1][0], s.filterType[1], f2L, g2, r2, d2, v.combPeriod[1]);
                r = filterSample (fs[0][1], s.filterType[0], f1R, g1, r1, d1, v.combPeriod[0]) + filterSample (fs[1][1], s.filterType[1], f2R, g2, r2, d2, v.combPeriod[1]);
            }

            l += directL;
            r += directR;

            // A DC blocker (about 8 Hz), for drive and asymmetric waves; before the amp, so a voice
            // faded or released to nothing ends at exactly nothing, its blocker's tail with it.
            const auto coefficient = 1.0f - (float) (50.0 / rate);
            const auto yl = l - v.dcInL + coefficient * v.dcL;
            const auto yr = r - v.dcInR + coefficient * v.dcR;
            v.dcInL = l; v.dcInR = r; v.dcL = yl; v.dcR = yr;

            // The amp, its pan, and the fade of a voice taken or silenced.
            const auto amp = e[0].level * v.ampGain.next();
            const auto pl = v.panL.next(), pr = v.panR.next();
            auto fade = 1.0f;

            if (v.fading)
            {
                v.fade = juce::jmax (0.0f, v.fade - v.fadeStep);
                fade = v.fade;
            }

            outL[i] += yl * amp * pl * fade;
            outR[i] += yr * amp * pr * fade;

            ++v.sinceStart;

            if ((v.fading && v.fade <= 0.0f) || e[0].stage == Envelope::idle)
            {
                v.active = v.fading = false;
                v.note = -1;
                return;
            }
        }
    }

    //==========================================================================
    /** The control tick: the shared settings read again, the LFOs moved on, every voice's
        settings worked out afresh, and the readings for the window. */
    void tick()
    {
        readShared();

        for (int l = 0; l < numLfos; ++l)
        {
            const auto hz = lfoHz (l, shared.base[(size_t) (dLfo1Rate + l)]);
            globalLfoPhase[(size_t) l] += hz * controlInterval / rate;
            globalLfoValue[(size_t) l] = lfoShape (shared.lfo[(size_t) l].shape, globalLfoPhase[(size_t) l] + shared.lfo[(size_t) l].phase, shared.lfo[(size_t) l].steps);
        }

        Voice* newest = nullptr;

        for (auto& v : voices)
        {
            if (! v.active)
                continue;

            // Glide moves the pitch on a tick at a time.
            if (v.glidePerSample > 0.0)
            {
                const auto step = v.glidePerSample * controlInterval;
                v.pitch = v.pitch < v.target ? juce::jmin (v.target, v.pitch + step) : juce::jmax (v.target, v.pitch - step);
                if (juce::exactlyEqual (v.pitch, v.target)) v.glidePerSample = 0.0;
            }

            for (int l = 0; l < numLfos; ++l)
                v.lfoPhase[(size_t) l] += lfoHz (l, v.value[(size_t) (dLfo1Rate + l)]) * controlInterval / rate;

            computeVoice (v, false);

            if (! v.fading && (newest == nullptr || v.age > newest->age))
                newest = &v;
        }

        // The effects follow the global sources (the newest voice's envelopes and LFOs).
        modulate (newest, fxValue);
        setEffects();

        for (int d = 0; d < numDestinations; ++d)
            owner.readings.destination[(size_t) d].store (newest != nullptr ? newest->value[(size_t) d] : (d >= dFxDrive ? fxValue[(size_t) d] : -1.0f), std::memory_order_relaxed);

        for (int l = 0; l < numLfos; ++l)
            owner.readings.lfo[(size_t) l].store (newest != nullptr ? newest->lfoValue[(size_t) l] : globalLfoValue[(size_t) l], std::memory_order_relaxed);

        for (int k = 0; k < numEnvelopes; ++k)
            owner.readings.envelope[(size_t) k].store (newest != nullptr ? newest->env[(size_t) k].level : 0.0f, std::memory_order_relaxed);

        owner.readings.sounding.store (sounding(), std::memory_order_relaxed);
        outGain.to (volume * volume * expression * expression, false);
    }

    //==========================================================================
    // The effects.
    std::array<float, numDestinations> fxValue {};
    Ramp driveMixRamp, chorusMixRamp, phaserMixRamp, delayMixRamp, reverbMixRamp;
    Modulation::DelayCore chorus;
    Modulation::PhaserCore phaser;
    RoomReverb::Core reverb;
    RoomReverb::Settings reverbSettings;
    std::vector<float> delayL, delayR;
    int delayWrite = 0;
    float delaySamples = 0.0f, delayLowL = 0.0f, delayLowR = 0.0f, driveLowL = 0.0f, driveLowR = 0.0f, driveDcL = 0, driveDcR = 0, driveDcInL = 0, driveDcInR = 0;
    float crushHoldL = 0.0f, crushHoldR = 0.0f, crushPhase = 0.0f;
    bool chorusLive = false, phaserLive = false, reverbLive = false;

    void prepareEffects()
    {
        chorus.prepare (rate, 2);
        phaser.prepare (rate, 2);
        reverb.prepare (rate);
        delayL.assign ((size_t) (rate * 4.0) + 4, 0.0f);
        delayR.assign (delayL.size(), 0.0f);
    }

    void resetEffects()
    {
        chorus.reset();
        phaser.reset();
        reverb.reset();
        std::fill (delayL.begin(), delayL.end(), 0.0f);
        std::fill (delayR.begin(), delayR.end(), 0.0f);
        delayWrite = 0;
        delaySamples = -1.0f;
        delayLowL = delayLowR = driveLowL = driveLowR = driveDcL = driveDcR = driveDcInL = driveDcInR = 0.0f;
        crushHoldL = crushHoldR = crushPhase = 0.0f;
        for (auto* r : { &driveMixRamp, &chorusMixRamp, &phaserMixRamp, &delayMixRamp, &reverbMixRamp }) r->to (0.0f, true);
        chorusLive = phaserLive = reverbLive = false;
        reverbSettings = {};
        reverbSettings.mixPercent = -1.0;
    }

    float fxPlain (int d) const   { return plain (d, fxValue[(size_t) d]); }

    void setEffects()
    {
        // Each effect's mix ramps to 0 when it is switched off, so switching never clicks.
        driveMixRamp.to (driveOn->get() ? driveMix->get() * 0.01f : 0.0f, false);
        chorusMixRamp.to (chorusOn->get() ? fxPlain (dFxChorus) * 0.01f : 0.0f, false);
        phaserMixRamp.to (phaserOn->get() ? fxPlain (dFxPhaser) * 0.01f : 0.0f, false);
        delayMixRamp.to (delayOn->get() ? fxPlain (dFxDelayMix) * 0.01f : 0.0f, false);
        reverbMixRamp.to (reverbOn->get() ? fxPlain (dFxReverb) * 0.01f : 0.0f, false);

        if (chorusOn->get() || chorusLive)
        {
            Modulation::Delay d;
            d.rateHz = chorusRate->get();
            d.delayMs = 8.0;
            d.depthMs = chorusDepth->get() * 0.01 * 12.0;
            d.feedback = 0.0;
            d.mixPercent = 100.0;   // wet only: mixed here
            d.spreadDegrees = 120.0;
            chorus.set (d);
        }

        if (phaserOn->get() || phaserLive)
        {
            Modulation::Phaser p;
            p.rateHz = phaserRate->get();
            p.lowHz = 200.0;
            p.highHz = juce::jmin (6000.0, rate * 0.4);
            p.stages = 8;
            p.feedback = phaserFeedback->get() * 0.01 * 0.9;
            p.mixPercent = 100.0;
            p.spreadDegrees = 90.0;
            phaser.set (p);
        }

        if (reverbOn->get() || reverbLive)
        {
            RoomReverb::Settings r;
            r.preDelayMs = 12.0;
            r.decaySeconds = reverbDecay->get();
            r.sizePercent = 50.0 + reverbSize->get();
            r.dampingPercent = reverbDamping->get();
            r.widthPercent = 100.0;
            r.mixPercent = 100.0;

            if (! juce::exactlyEqual (r.decaySeconds, reverbSettings.decaySeconds) || ! juce::exactlyEqual (r.sizePercent, reverbSettings.sizePercent)
                || ! juce::exactlyEqual (r.dampingPercent, reverbSettings.dampingPercent) || reverbSettings.mixPercent < 0.0)
            {
                reverbSettings = r;
                reverb.set (r);
            }
        }

        const auto target = (float) (divisionBeats (delayDivision->getIndex()) * 60.0 / bpm * rate);
        delaySamples = delaySamples < 0.0f ? target : delaySamples;
        delayTarget = juce::jlimit (1.0f, (float) delayL.size() - 4.0f, target);
    }

    float delayTarget = 1.0f;

    static float saturate (int type, float x) noexcept
    {
        switch (type)
        {
            case 1:  return juce::jlimit (-1.0f, 1.0f, x);                                      // hard
            case 2:  return x >= 0.0f ? std::tanh (x) : std::tanh (x * 0.6f) / 0.6f * 0.8f;    // tube: uneven halves
            case 3:  { const auto y = std::tanh (x * 3.0f + 0.35f * std::abs (x)); return y; }  // fuzz: gated, rectified edge
            case 4:  return fold (x);
            default: return std::tanh (x);                                                     // soft
        }
    }

    void renderEffects (float* l, float* r, int n)
    {
        // Drive.
        if (driveMixRamp.value > 0.0f || driveMixRamp.step != 0.0f)
        {
            const auto type = driveType->getIndex();
            const auto amount = fxValue[(size_t) dFxDrive];
            const auto gain = juce::Decibels::decibelsToGain (amount * 36.0f);
            const auto makeup = 1.0f / (1.0f + amount * 1.2f);
            const auto tone = 1.0f - std::exp (-twoPiF * driveTone->get() / (float) rate);
            const auto crush = type == 5;
            const auto crushRate = 1.0f - amount * 0.94f;
            const auto crushSteps = std::exp2 (12.0f - amount * 9.0f);

            for (int i = 0; i < n; ++i)
            {
                const auto mix = driveMixRamp.next();
                float wl, wr;

                if (crush)
                {
                    crushPhase += crushRate;
                    if (crushPhase >= 1.0f) { crushPhase -= 1.0f; crushHoldL = l[i]; crushHoldR = r[i]; }
                    wl = std::round (crushHoldL * crushSteps) / crushSteps;
                    wr = std::round (crushHoldR * crushSteps) / crushSteps;
                }
                else
                {
                    wl = saturate (type, l[i] * gain) * makeup;
                    wr = saturate (type, r[i] * gain) * makeup;
                }

                driveLowL += tone * (wl - driveLowL);
                driveLowR += tone * (wr - driveLowR);
                // Its own DC blocker: the uneven curves leave an offset.
                const auto c = 1.0f - (float) (60.0 / rate);
                const auto yl = driveLowL - driveDcInL + c * driveDcL, yr = driveLowR - driveDcInR + c * driveDcR;
                driveDcInL = driveLowL; driveDcInR = driveLowR; driveDcL = yl; driveDcR = yr;
                l[i] += mix * (yl - l[i]);
                r[i] += mix * (yr - r[i]);
            }
        }

        const auto wetPass = [this, n, l, r] (Ramp& mixRamp, auto&& process)
        {
            float* wet[] = { scratch.data(), scratch.data() + controlInterval };
            std::copy (l, l + n, wet[0]);
            std::copy (r, r + n, wet[1]);
            process (wet);

            for (int i = 0; i < n; ++i)
            {
                const auto mix = mixRamp.next();
                l[i] += mix * (wet[0][i] - l[i]);
                r[i] += mix * (wet[1][i] - r[i]);
            }
        };

        // Chorus and phaser: wet against dry.
        chorusLive = chorusMixRamp.value > 0.0f || chorusMixRamp.step != 0.0f;
        if (chorusLive)
            wetPass (chorusMixRamp, [this, n] (float** w) { chorus.process (w, 2, n); });

        phaserLive = phaserMixRamp.value > 0.0f || phaserMixRamp.step != 0.0f;
        if (phaserLive)
            wetPass (phaserMixRamp, [this, n] (float** w) { phaser.process (w, 2, n); });

        // Delay: tempo-synced, its feedback darkened, ping-pong across the sides; added to the dry sound.
        if (delayMixRamp.value > 0.0f || delayMixRamp.step != 0.0f)
        {
            const auto size = (int) delayL.size();
            const auto feedback = fxPlain (dFxDelayFeedback) * 0.01f * 0.95f;
            const auto tone = 1.0f - std::exp (-twoPiF * delayTone->get() / (float) rate);
            const auto ping = pingPong->get();

            for (int i = 0; i < n; ++i)
            {
                const auto mix = delayMixRamp.next();
                delaySamples += 0.001f * (delayTarget - delaySamples);   // a tempo change glides rather than jumps
                auto read = (float) delayWrite - delaySamples;
                if (read < 0.0f) read += (float) size;
                if (read >= (float) size) read -= (float) size;   // a hair under 0 rounds up to the size
                const auto j = (int) read;
                const auto f = read - (float) j;
                const auto j2 = j + 1 == size ? 0 : j + 1;
                const auto dl = delayL[(size_t) j] + f * (delayL[(size_t) j2] - delayL[(size_t) j]);
                const auto dr = delayR[(size_t) j] + f * (delayR[(size_t) j2] - delayR[(size_t) j]);
                delayLowL += tone * (dl - delayLowL);
                delayLowR += tone * (dr - delayLowR);

                if (ping)
                {
                    delayL[(size_t) delayWrite] = (l[i] + r[i]) * 0.5f + feedback * delayLowR;
                    delayR[(size_t) delayWrite] = feedback * delayLowL;
                }
                else
                {
                    delayL[(size_t) delayWrite] = l[i] + feedback * delayLowL;
                    delayR[(size_t) delayWrite] = r[i] + feedback * delayLowR;
                }

                delayWrite = delayWrite + 1 == size ? 0 : delayWrite + 1;
                l[i] += mix * dl;
                r[i] += mix * dr;
            }
        }

        // Reverb: added to the dry sound.
        reverbLive = reverbMixRamp.value > 0.0f || reverbMixRamp.step != 0.0f;
        if (reverbLive)
        {
            float* wet[] = { scratch.data(), scratch.data() + controlInterval };
            std::copy (l, l + n, wet[0]);
            std::copy (r, r + n, wet[1]);
            reverb.process (wet, 2, n);

            for (int i = 0; i < n; ++i)
            {
                const auto mix = reverbMixRamp.next();
                l[i] += mix * wet[0][i];
                r[i] += mix * wet[1][i];
            }
        }
    }

    std::array<float, 2 * controlInterval> scratch {};

    //==========================================================================
    void process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
    {
        const auto n = buffer.getNumSamples();
        buffer.clear();

        if (auto* head = owner.getPlayHead())
            if (const auto position = head->getPosition())
                if (const auto b = position->getBpm())
                    bpm = juce::jlimit (20.0, 999.0, *b);

        std::array<float, controlInterval> l {}, r {};
        auto event = midi.cbegin();
        int pos = 0;

        while (pos < n)
        {
            // The tick comes first at its sample; then the events at that sample.
            if (clock % controlInterval == 0 && lastTick != clock)
            {
                lastTick = clock;
                tick();
            }

            for (; event != midi.cend() && (*event).samplePosition <= pos; ++event)
                handle ((*event).getMessage());

            const auto toTick = (int) (controlInterval - clock % controlInterval);
            auto end = juce::jmin (n, pos + toTick);

            if (event != midi.cend())
                end = juce::jmin (end, juce::jmax (pos + 1, (*event).samplePosition));

            const auto count = end - pos;
            std::fill (l.begin(), l.begin() + count, 0.0f);
            std::fill (r.begin(), r.begin() + count, 0.0f);

            for (auto& v : voices)
                if (v.active)
                    renderVoice (v, l.data(), r.data(), count);

            renderEffects (l.data(), r.data(), count);

            for (int i = 0; i < count; ++i)
            {
                const auto g = outGain.next();
                l[(size_t) i] *= g;
                r[(size_t) i] *= g;
            }

            if (buffer.getNumChannels() == 1)
            {
                auto* out = buffer.getWritePointer (0, pos);
                for (int i = 0; i < count; ++i) out[i] = 0.5f * (l[(size_t) i] + r[(size_t) i]);
            }
            else
            {
                buffer.copyFrom (0, pos, l.data(), count);
                buffer.copyFrom (1, pos, r.data(), count);

                for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
                    buffer.copyFrom (ch, pos, buffer, ch % 2, pos, count);
            }

            pos = end;
            clock += count;
        }

        midi.clear();
    }

    void handle (const juce::MidiMessage& m)
    {
        if (m.isNoteOn())
            noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())
            noteOff (m.getNoteNumber());
        else if (m.isAllSoundOff())
            allOff (true);
        else if (m.isAllNotesOff())
            allOff (false);
        else
            controller (m);
    }
};

//==============================================================================
namespace BuiltInEffects
{
namespace
{
    juce::PluginDescription synthDescription()
    {
        juce::PluginDescription d;
        d.name = d.descriptiveName = "Torqwave Synth";
        d.pluginFormatName = formatName;
        d.category = "Instrument";
        d.manufacturerName = "Torqwave";
        d.version = "1";
        d.fileOrIdentifier = basicSynthId;
        d.uniqueId = d.deprecatedUid = 0x53425359;   // "SBSY", as it has always been
        d.isInstrument = true;
        d.numInputChannels = 0;
        d.numOutputChannels = 2;
        return d;
    }

    // The settings it had before 0.20.0, by ID.
    const juce::StringArray oldIds { "wave", "level", "attack", "decay", "sustain", "release", "cutoff", "resonance",
                                     "osc2Wave", "osc2Octave", "osc2Detune", "osc2Level", "osc3Wave", "osc3Octave", "osc3Detune", "osc3Level",
                                     "filterEnv", "filterDecay", "velocityToFilter", "glide" };
}

juce::PluginDescription basicSynthDescription()
{
    return synthDescription();
}

BasicSynthProcessor::BasicSynthProcessor()
    : BuiltInEffect (synthDescription(), "TORQWAVE_BASIC_SYNTH", false, true)
{
    using namespace WaveSynth;
    const auto percent = juce::NormalisableRange<float> (0.0f, 100.0f);
    const auto bipolar = juce::NormalisableRange<float> (-100.0f, 100.0f);
    const auto time = [] (float most) { return juce::NormalisableRange<float> (0.0f, most, 0.0f, 0.3f); };
    // Cutoffs move in octaves: a tenth of the knob is an octave (20 Hz to 20 kHz is ten of them).
    const auto hz = juce::NormalisableRange<float> (20.0f, 20000.0f,
                                                     [] (float, float, float x) { return 20.0f * std::pow (1000.0f, x); },
                                                     [] (float, float, float v) { return std::log (juce::jlimit (20.0f, 20000.0f, v) / 20.0f) / std::log (1000.0f); });
    const auto keep = [this] (juce::RangedAudioParameter* p) { byId[p->paramID] = p; return p; };

    for (int o = 1; o <= numOscillators; ++o)
    {
        const auto id = "osc" + juce::String (o);
        const auto name = "Osc " + juce::String (o) + " ";
        keep (addBool (id + "On", name + "On", o == 1));
        keep (addChoice (id + "Table", name + "Table", WaveTables::names(), 0));
        keep (addFloat (id + "Position", name + "Position", percent, o == 1 ? 66.67f : 0.0f, "%"));
        keep (addChoice (id + "Warp", name + "Warp", warpNames(), 0));
        keep (addFloat (id + "WarpAmount", name + "Warp Amount", percent, 0.0f, "%"));
        keep (addFloat (id + "Pitch", name + "Pitch", { -24.0f, 24.0f, 1.0f }, 0.0f, "st"));
        keep (addFloat (id + "Fine", name + "Fine", { -100.0f, 100.0f }, 0.0f, "ct"));
        keep (addFloat (id + "Volume", name + "Level", percent, 80.0f, "%"));
        keep (addFloat (id + "Pan", name + "Pan", bipolar, 0.0f, "+%"));
        keep (addFloat (id + "Phase", name + "Phase", percent, 0.0f, "%"));
        keep (addBool (id + "RandomPhase", name + "Random Phase", true));
        keep (addFloat (id + "Unison", name + "Unison", { 1.0f, (float) maxUnison, 1.0f }, 1.0f, "voices"));
        keep (addFloat (id + "UnisonDetune", name + "Detune", { 0.0f, 100.0f, 0.0f, 0.5f }, 15.0f, "ct"));
        keep (addFloat (id + "UnisonSpread", name + "Spread", percent, 70.0f, "%"));
        keep (addFloat (id + "UnisonStrum", name + "Strum", { 0.0f, 60.0f, 0.0f, 0.6f }, 0.0f, "ms"));
        keep (addFloat (id + "UnisonDrift", name + "Drift", percent, 0.0f, "%"));
        keep (addFloat (id + "Route", name + "Filter Route", percent, 0.0f, "%"));
    }

    keep (addFloat ("subLevel", "Sub Level", percent, 0.0f, "%"));
    keep (addChoice ("subOctave", "Sub Octave", { "-1 oct", "-2 oct" }, 0));
    keep (addChoice ("subShape", "Sub Shape", { "Sine", "Triangle", "Round Square" }, 0));
    keep (addBool ("subDirect", "Sub Direct", false));
    keep (addFloat ("noiseLevel", "Noise Level", percent, 0.0f, "%"));
    keep (addChoice ("noiseType", "Noise Type", { "White", "Pink", "Crackle", "Digital" }, 0));
    keep (addFloat ("noiseColour", "Noise Colour", bipolar, 0.0f, "+%"));

    for (int f = 1; f <= 2; ++f)
    {
        const auto id = "filter" + juce::String (f);
        const auto name = "Filter " + juce::String (f) + " ";
        keep (addChoice (id + "Type", name + "Type", filterNames(), f == 1 ? (int) FilterType::ladder24 : (int) FilterType::off));
        keep (addFloat (id + "Cutoff", name + "Cutoff", hz, f == 1 ? 8000.0f : 2000.0f, "Hz"));
        keep (addFloat (id + "Resonance", name + "Resonance", percent, 10.0f, "%"));
        keep (addFloat (id + "Drive", name + "Drive", percent, 0.0f, "%"));
        keep (addFloat (id + "KeyTrack", name + "Key Track", percent, 0.0f, "%"));
    }

    keep (addChoice ("filterRouting", "Filter Routing", { "Serial", "Parallel" }, 0));
    keep (addFloat ("ampLevel", "Amp Level", { -40.0f, 6.0f }, -9.0f, "dB"));
    keep (addFloat ("ampPan", "Amp Pan", bipolar, 0.0f, "+%"));
    keep (addFloat ("ampVelocity", "Velocity", percent, 70.0f, "%"));

    for (int e = 1; e <= numEnvelopes; ++e)
    {
        const auto id = "env" + juce::String (e);
        const auto name = (e == 1 ? juce::String ("Amp Env ") : "Env " + juce::String (e) + " ");
        keep (addFloat (id + "Attack", name + "Attack", time (10000.0f), e == 1 ? 2.0f : 0.0f, "ms"));
        keep (addFloat (id + "Decay", name + "Decay", time (10000.0f), e == 1 ? 300.0f : 400.0f, "ms"));
        keep (addFloat (id + "Sustain", name + "Sustain", percent, e == 1 ? 80.0f : 0.0f, "%"));
        keep (addFloat (id + "Release", name + "Release", time (10000.0f), e == 1 ? 120.0f : 300.0f, "ms"));
        keep (addFloat (id + "Curve", name + "Curve", bipolar, 50.0f, "+%"));
    }

    keep (addChoice ("voiceMode", "Voice Mode", { "Poly", "Mono", "Legato" }, 0));
    keep (addChoice ("glideMode", "Glide Mode", { "Overlapping", "Always" }, 0));
    keep (addFloat ("glideTime", "Glide", time (2000.0f), 0.0f, "ms"));
    keep (addFloat ("bendRange", "Bend Range", { 0.0f, 24.0f, 1.0f }, 2.0f, "st"));

    for (int l = 1; l <= numLfos; ++l)
    {
        const auto id = "lfo" + juce::String (l);
        const auto name = "LFO " + juce::String (l) + " ";
        keep (addChoice (id + "Shape", name + "Shape", lfoShapeNames(), 0));
        keep (addBool (id + "Sync", name + "Sync", false));
        keep (addFloat (id + "Rate", name + "Rate", { 0.01f, 40.0f, 0.0f, 0.3f }, l == 1 ? 2.0f : 0.5f, "Hz"));
        keep (addChoice (id + "Division", name + "Division", divisionNames(), 7));
        keep (addChoice (id + "Mode", name + "Mode", { "Free", "Retrigger", "One Shot" }, 1));
        keep (addFloat (id + "Phase", name + "Phase", percent, 0.0f, "%"));
        keep (addFloat (id + "Fade", name + "Fade In", time (5000.0f), 0.0f, "ms"));
        keep (addFloat (id + "Smooth", name + "Smooth", percent, 0.0f, "%"));

        for (int s = 1; s <= numSteps; ++s)
        {
            // A falling pattern with accents, so Steps does something when first chosen.
            const auto start = (s % 4 == 1) ? 100.0f : (s % 2 == 0 ? -40.0f : 30.0f);
            keep (addFloat (id + "Step" + juce::String (s), name + "Step " + juce::String (s), bipolar, start, "+%"));
        }
    }

    juce::StringArray destinationChoices { "None" };
    destinationChoices.addArray (destinationNames());

    for (int s = 1; s <= numSlots; ++s)
    {
        const auto id = "mod" + juce::String (s);
        const auto name = "Mod " + juce::String (s) + " ";
        keep (addChoice (id + "Source", name + "Source", sourceNames(), 0));
        keep (addChoice (id + "Destination", name + "Destination", destinationChoices, 0));
        keep (addFloat (id + "Amount", name + "Amount", bipolar, 0.0f, "+%"));
    }

    for (int m = 1; m <= numMacros; ++m)
        keep (addFloat ("macro" + juce::String (m), "Macro " + juce::String (m), percent, 0.0f, "%"));

    keep (addBool ("fxDriveOn", "Drive On", false));
    keep (addChoice ("fxDriveType", "Drive Type", { "Soft", "Hard", "Tube", "Fuzz", "Fold", "Crush" }, 0));
    keep (addFloat ("fxDriveAmount", "Drive Amount", percent, 30.0f, "%"));
    keep (addFloat ("fxDriveTone", "Drive Tone", { 500.0f, 20000.0f, 0.0f, 0.3f }, 12000.0f, "Hz"));
    keep (addFloat ("fxDriveMix", "Drive Mix", percent, 100.0f, "%"));
    keep (addBool ("fxChorusOn", "Chorus On", false));
    keep (addFloat ("fxChorusRate", "Chorus Rate", { 0.05f, 5.0f, 0.0f, 0.5f }, 0.6f, "Hz"));
    keep (addFloat ("fxChorusDepth", "Chorus Depth", percent, 50.0f, "%"));
    keep (addFloat ("fxChorusMix", "Chorus Mix", percent, 50.0f, "%"));
    keep (addBool ("fxPhaserOn", "Phaser On", false));
    keep (addFloat ("fxPhaserRate", "Phaser Rate", { 0.02f, 5.0f, 0.0f, 0.5f }, 0.3f, "Hz"));
    keep (addFloat ("fxPhaserFeedback", "Phaser Feedback", percent, 50.0f, "%"));
    keep (addFloat ("fxPhaserMix", "Phaser Mix", percent, 50.0f, "%"));
    keep (addBool ("fxDelayOn", "Delay On", false));
    keep (addChoice ("fxDelayDivision", "Delay Time", divisionNames(), 9));
    keep (addFloat ("fxDelayFeedback", "Delay Feedback", percent, 35.0f, "%"));
    keep (addBool ("fxDelayPingPong", "Delay Ping-Pong", true));
    keep (addFloat ("fxDelayTone", "Delay Tone", { 500.0f, 20000.0f, 0.0f, 0.3f }, 5000.0f, "Hz"));
    keep (addFloat ("fxDelayMix", "Delay Mix", percent, 25.0f, "%"));
    keep (addBool ("fxReverbOn", "Reverb On", false));
    keep (addFloat ("fxReverbSize", "Reverb Size", percent, 60.0f, "%"));
    keep (addFloat ("fxReverbDecay", "Reverb Decay", { 0.2f, 20.0f, 0.0f, 0.4f }, 2.5f, "s"));
    keep (addFloat ("fxReverbDamping", "Reverb Damping", percent, 45.0f, "%"));
    keep (addFloat ("fxReverbMix", "Reverb Mix", percent, 25.0f, "%"));

    engine = std::make_unique<Engine> (*this);
    engine->bind();
    WaveTables::get (WaveTables::Table::analog);   // the tables, built now rather than on the audio thread
}

BasicSynthProcessor::~BasicSynthProcessor() = default;

juce::RangedAudioParameter* BasicSynthProcessor::parameter (const juce::String& id) const
{
    const auto found = byId.find (id);
    return found != byId.end() ? found->second : nullptr;
}

void BasicSynthProcessor::prepareToPlay (double sampleRate, int)
{
    engine->prepare (sampleRate);
}

void BasicSynthProcessor::reset()
{
    engine->reset();
}

void BasicSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    engine->process (buffer, midi);
}

double BasicSynthProcessor::getTailLengthSeconds() const
{
    auto tail = (double) dynamic_cast<juce::AudioParameterFloat*> (parameter ("env1Release"))->get() * 0.001 + WaveSynth::fadeSeconds;

    if (auto* on = dynamic_cast<juce::AudioParameterBool*> (parameter ("fxDelayOn")); on != nullptr && on->get())
        tail += 4.0;

    if (auto* on = dynamic_cast<juce::AudioParameterBool*> (parameter ("fxReverbOn")); on != nullptr && on->get())
        tail += dynamic_cast<juce::AudioParameterFloat*> (parameter ("fxReverbDecay"))->get() + 0.1;

    return tail;
}

int BasicSynthProcessor::getNumSounding() const
{
    return engine->sounding();
}

juce::AudioProcessorEditor* BasicSynthProcessor::createEditor()
{
    return new WaveSynthEditor (*this);
}

//==============================================================================
void BasicSynthProcessor::setAllToDefault()
{
    for (auto* p : getParameters())
        p->setValueNotifyingHost (p->getDefaultValue());

    for (auto& m : macroNames)
        m = {};
}

void BasicSynthProcessor::loadPatch (int index)
{
    const auto& all = WaveSynth::factoryPatches();

    if (! juce::isPositiveAndBelow (index, (int) all.size()))
        return;

    const auto& patch = all[(size_t) index];
    setAllToDefault();
    BuiltInEffect::setPresetValues (patch.values);
    macroNames = patch.macroNames;
    patchName = patch.name;
}

bool BasicSynthProcessor::loadPatch (const juce::String& name)
{
    const auto& all = WaveSynth::factoryPatches();

    for (size_t i = 0; i < all.size(); ++i)
        if (all[i].name.equalsIgnoreCase (name))
        {
            loadPatch ((int) i);
            return true;
        }

    return false;
}

int BasicSynthProcessor::addModulation (int source, const juce::String& destinationId, float amount)
{
    using namespace WaveSynth;
    const auto destination = destinationIds().indexOf (destinationId);

    if (source <= 0 || source >= numSources || destination < 0)
        return -1;

    const auto slotParam = [this] (int s, const char* field) { return parameter ("mod" + juce::String (s + 1) + field); };
    int chosen = -1;

    for (int s = 0; s < numSlots && chosen < 0; ++s)
        if ((int) slotParam (s, "Source")->convertFrom0to1 (slotParam (s, "Source")->getValue()) == source
            && (int) slotParam (s, "Destination")->convertFrom0to1 (slotParam (s, "Destination")->getValue()) == destination + 1)
            chosen = s;

    for (int s = 0; s < numSlots && chosen < 0; ++s)
        if ((int) slotParam (s, "Source")->convertFrom0to1 (slotParam (s, "Source")->getValue()) == 0
            || (int) slotParam (s, "Destination")->convertFrom0to1 (slotParam (s, "Destination")->getValue()) == 0)
            chosen = s;

    if (chosen < 0)
        return -1;

    const auto set = [] (juce::RangedAudioParameter* p, float plainValue)
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
        p->endChangeGesture();
    };
    set (slotParam (chosen, "Source"), (float) source);
    set (slotParam (chosen, "Destination"), (float) destination + 1);
    set (slotParam (chosen, "Amount"), juce::jlimit (-1.0f, 1.0f, amount) * 100.0f);
    return chosen;
}

void BasicSynthProcessor::removeModulation (int slot)
{
    if (! juce::isPositiveAndBelow (slot, WaveSynth::numSlots))
        return;

    for (auto field : { "Source", "Destination", "Amount" })
    {
        auto* p = parameter ("mod" + juce::String (slot + 1) + field);
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->getDefaultValue());
        p->endChangeGesture();
    }
}

void BasicSynthProcessor::setModulationAmount (int slot, float amount)
{
    if (! juce::isPositiveAndBelow (slot, WaveSynth::numSlots))
        return;

    auto* p = parameter ("mod" + juce::String (slot + 1) + "Amount");
    p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (-1.0f, 1.0f, amount) * 100.0f));
}

std::vector<std::tuple<int, int, float>> BasicSynthProcessor::modulationOf (const juce::String& id) const
{
    std::vector<std::tuple<int, int, float>> found;
    const auto destination = WaveSynth::destinationIds().indexOf (id);

    if (destination < 0)
        return found;

    for (int s = 0; s < WaveSynth::numSlots; ++s)
    {
        auto* source = dynamic_cast<juce::AudioParameterChoice*> (parameter ("mod" + juce::String (s + 1) + "Source"));
        auto* dest = dynamic_cast<juce::AudioParameterChoice*> (parameter ("mod" + juce::String (s + 1) + "Destination"));
        auto* amount = dynamic_cast<juce::AudioParameterFloat*> (parameter ("mod" + juce::String (s + 1) + "Amount"));

        if (source->getIndex() != 0 && dest->getIndex() == destination + 1)
            found.emplace_back (s, source->getIndex(), amount->get() * 0.01f);
    }

    return found;
}

//==============================================================================
EffectPresets::Values BasicSynthProcessor::translateOldSettings (const EffectPresets::Values& old)
{
    // The old defaults, for what an old state left out.
    const auto get = [&old] (const char* id, double fallback) { const auto f = old.find (id); return f != old.end() && std::isfinite (f->second) ? f->second : fallback; };
    const auto wavePosition = [] (double wave) { return juce::jlimit (0.0, 3.0, std::round (wave)) / 3.0 * 100.0; };   // sine, triangle, saw, square along Analog

    EffectPresets::Values v;
    // Oscillator 1 was always on at full level; 2 and 3 at their levels.
    v["osc1On"] = 1.0;
    v["osc1Table"] = 0.0;
    v["osc1Position"] = wavePosition (get ("wave", 2.0));
    v["osc1Volume"] = 100.0;
    v["osc1RandomPhase"] = 0.0;
    v["osc1Unison"] = 1.0;

    for (int o = 2; o <= 3; ++o)
    {
        const auto n = juce::String (o);
        const auto level = get (("osc" + n + "Level").toRawUTF8(), 0.0);
        v["osc" + n + "On"] = level > 0.0 ? 1.0 : 0.0;
        v["osc" + n + "Table"] = 0.0;
        v["osc" + n + "Position"] = wavePosition (get (("osc" + n + "Wave").toRawUTF8(), 2.0));
        v["osc" + n + "Pitch"] = 12.0 * get (("osc" + n + "Octave").toRawUTF8(), 0.0);
        v["osc" + n + "Fine"] = get (("osc" + n + "Detune").toRawUTF8(), 0.0);
        // Its level was a share of oscillator 1's; the new level is squared, so its root.
        v["osc" + n + "Volume"] = std::sqrt (juce::jlimit (0.0, 100.0, level) / 100.0) * 100.0;
        v["osc" + n + "RandomPhase"] = 0.0;
        v["osc" + n + "Unison"] = 1.0;
    }

    v["ampLevel"] = get ("level", -12.0);
    v["ampVelocity"] = 100.0;
    v["env1Attack"] = get ("attack", 5.0);
    v["env1Decay"] = get ("decay", 100.0);
    v["env1Sustain"] = get ("sustain", 70.0);
    v["env1Release"] = get ("release", 200.0);
    v["env1Curve"] = 0.0;

    // The old filter was a 12 dB state-variable low-pass (out of the way when wide open).
    const auto cutoff = get ("cutoff", 20000.0), resonance = get ("resonance", 0.0);
    const auto open = cutoff >= 19999.0 && resonance <= 0.0 && get ("filterEnv", 0.0) <= 0.0 && get ("velocityToFilter", 0.0) <= 0.0;
    v["filter1Type"] = open ? (double) WaveSynth::FilterType::off : (double) WaveSynth::FilterType::svfLow;
    v["filter2Type"] = (double) WaveSynth::FilterType::off;
    v["filter1Resonance"] = resonance;
    v["filter1Drive"] = 0.0;
    v["filter1KeyTrack"] = 0.0;

    // Velocity to filter: a soft note was up to four octaves darker. The cutoff goes down by that
    // much and velocity brings it back (the cutoff's range is about ten octaves).
    const auto velocityOctaves = get ("velocityToFilter", 0.0) * 0.04;
    const auto cutoffOctaves = std::log2 (1000.0);
    v["filter1Cutoff"] = juce::jlimit (20.0, 20000.0, cutoff * std::exp2 (-velocityOctaves));
    int slot = 1;
    const auto addSlot = [&v, &slot] (int source, const char* destination, double amount)
    {
        const auto n = juce::String (slot++);
        v["mod" + n + "Source"] = source;
        v["mod" + n + "Destination"] = WaveSynth::destinationIds().indexOf (destination) + 1;
        v["mod" + n + "Amount"] = juce::jlimit (-100.0, 100.0, amount);
    };

    // The filter envelope opened it up to six octaves at a note's start, then fell over its decay.
    if (const auto amount = get ("filterEnv", 0.0); amount > 0.0)
    {
        const auto decay = get ("filterDecay", 300.0);
        v["env2Attack"] = 0.0;
        // The old one fell by e every `decay`; this one, at its steepest curve, falls by e every sixth of its time.
        v["env2Decay"] = juce::jmin (10000.0, decay * 6.0);
        v["env2Sustain"] = 0.0;
        v["env2Release"] = juce::jmin (10000.0, decay * 6.0);
        v["env2Curve"] = 100.0;
        addSlot (WaveSynth::env2, "filter1Cutoff", amount * 0.06 / cutoffOctaves * 100.0);
    }

    if (velocityOctaves > 0.0)
        addSlot (WaveSynth::velocity, "filter1Cutoff", velocityOctaves / cutoffOctaves * 100.0);

    for (int s = slot; s <= WaveSynth::numSlots; ++s)
    {
        v["mod" + juce::String (s) + "Source"] = 0.0;
        v["mod" + juce::String (s) + "Destination"] = 0.0;
        v["mod" + juce::String (s) + "Amount"] = 0.0;
    }

    // Glide slid a note played over a held one: legato now.
    const auto glide = get ("glide", 0.0);
    v["voiceMode"] = glide > 0.0 ? 2.0 : 0.0;
    v["glideTime"] = glide;
    v["glideMode"] = 0.0;
    return v;
}

void BasicSynthProcessor::setPresetValues (const EffectPresets::Values& values)
{
    // A preset saved before 0.20.0 names the old settings and none of the new.
    const auto old = std::any_of (oldIds.begin(), oldIds.end(), [&values] (const juce::String& id) { return values.count (id) > 0; })
                     && values.count ("osc1Table") == 0;

    if (old)
    {
        setAllToDefault();
        BuiltInEffect::setPresetValues (translateOldSettings (values));
        patchName = "Old Basic Synth";
        return;
    }

    BuiltInEffect::setPresetValues (values);
}

void BasicSynthProcessor::addToState (juce::XmlElement& xml) const
{
    xml.setAttribute ("engine", 2);
    xml.setAttribute ("patch", patchName);

    for (int m = 0; m < WaveSynth::numMacros; ++m)
        if (macroNames[(size_t) m].isNotEmpty())
            xml.setAttribute ("macro" + juce::String (m + 1) + "Name", macroNames[(size_t) m]);
}

void BasicSynthProcessor::readFromState (const juce::XmlElement& xml)
{
    // A state that only names a factory patch (the factory sequence presets' voices): the patch as it is now.
    if (xml.hasAttribute ("factoryPatch"))
    {
        if (! loadPatch (xml.getStringAttribute ("factoryPatch")))
            loadPatch (0);

        return;
    }

    if (xml.getIntAttribute ("engine", 1) < 2)
    {
        // Saved before 0.20.0: none of the new IDs were set from it. Its settings, translated.
        EffectPresets::Values old;

        for (auto& id : oldIds)
            if (xml.hasAttribute (id))
                old[id] = xml.getDoubleAttribute (id);

        setAllToDefault();
        BuiltInEffect::setPresetValues (translateOldSettings (old));
        patchName = "Old Basic Synth";
        return;
    }

    patchName = xml.getStringAttribute ("patch", "Init");

    for (int m = 0; m < WaveSynth::numMacros; ++m)
        macroNames[(size_t) m] = xml.getStringAttribute ("macro" + juce::String (m + 1) + "Name");
}
}
