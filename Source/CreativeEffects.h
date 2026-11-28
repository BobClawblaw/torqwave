#pragma once

#include "BuiltInEffects.h"
#include "Oversampling.h"

namespace BuiltInEffects
{
    constexpr const char* delayId        = "torqwave:delay";
    constexpr const char* distortionId   = "torqwave:distortion";
    constexpr const char* autoFilterId   = "torqwave:auto-filter";
    constexpr const char* volumeShaperId = "torqwave:volume-shaper";
    constexpr const char* noiseGateId        = "torqwave:noise-gate";
    constexpr const char* tremoloId          = "torqwave:tremolo";
    constexpr const char* frequencyShifterId = "torqwave:frequency-shifter";
    constexpr const char* shimmerDelayId     = "torqwave:shimmer-delay";

    juce::PluginDescription delayDescription();
    juce::PluginDescription distortionDescription();
    juce::PluginDescription autoFilterDescription();
    juce::PluginDescription volumeShaperDescription();
    juce::PluginDescription noiseGateDescription();
    juce::PluginDescription tremoloDescription();
    juce::PluginDescription frequencyShifterDescription();
    juce::PluginDescription shimmerDelayDescription();

    /** Tempo-synced or free stereo / ping-pong / tape delay with feedback filtering, tape wow/drive,
        and input-triggered ducking of the wet signal. */
    class DelayProcessor final : public BuiltInEffect
    {
    public:
        DelayProcessor();

        void prepareToPlay (double sampleRate, int maximumBlockSize) override;
        void reset() override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
        double getTailLengthSeconds() const override;

    private:
        juce::AudioParameterChoice *mode = nullptr, *division = nullptr;
        juce::AudioParameterBool* sync = nullptr;
        juce::AudioParameterFloat *timeMs = nullptr, *offset = nullptr, *feedback = nullptr;
        juce::AudioParameterFloat *lowCut = nullptr, *highCut = nullptr;
        juce::AudioParameterFloat *wow = nullptr, *drive = nullptr, *ducking = nullptr;
        juce::AudioParameterFloat *width = nullptr, *mix = nullptr;

        double rate = 48000.0;
        std::array<std::vector<float>, 2> lines;
        int writePos = 0;
        double wowPhase = 0.0;
        std::array<float, 2> hpState {}, lpState {};
        float smoothDelayL = 12000.0f, smoothDelayR = 12000.0f;
        float duckEnv = 0.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayProcessor)
    };

    /** Multi-mode distortion, saturation, wavefolder and bitcrusher with optional 4x oversampling,
        pre high-pass, post low-pass, asymmetry bias, output trim and wet/dry mix. */
    class DistortionProcessor final : public BuiltInEffect
    {
    public:
        DistortionProcessor();

        void prepareToPlay (double sampleRate, int maximumBlockSize) override;
        void reset() override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    private:
        juce::AudioParameterChoice *mode = nullptr, *oversample = nullptr;
        juce::AudioParameterFloat *driveDb = nullptr, *bias = nullptr, *bits = nullptr, *downsample = nullptr;
        juce::AudioParameterFloat *preHp = nullptr, *postLp = nullptr, *outputDb = nullptr, *mix = nullptr;

        double rate = 48000.0;
        std::array<Oversampler4, 2> oversamplers;
        std::array<std::array<float, Oversampler4::latency + 1>, 2> dryDelay {};
        int dryPos = 0;
        std::array<float, 2> hpState {}, lpState {}, holdSample {};
        std::array<float, 2> holdCounter {};
        std::array<float, 2> dcIn {}, dcOut {};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistortionProcessor)
    };

    /** Resonant state-variable / comb auto-filter driven by a tempo-synced LFO and an audio envelope
        follower (with optional external sidechain key input). */
    class AutoFilterProcessor final : public BuiltInEffect
    {
    public:
        AutoFilterProcessor();

        void prepareToPlay (double sampleRate, int maximumBlockSize) override;
        void reset() override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    private:
        struct SvfStage
        {
            float ic1 = 0.0f, ic2 = 0.0f;
            void clear() noexcept { ic1 = ic2 = 0.0f; }
        };

        juce::AudioParameterChoice *filterType = nullptr, *lfoShape = nullptr, *division = nullptr;
        juce::AudioParameterBool *sync = nullptr, *sidechain = nullptr;
        juce::AudioParameterFloat *cutoff = nullptr, *resonance = nullptr, *drive = nullptr;
        juce::AudioParameterFloat *lfoRate = nullptr, *lfoDepth = nullptr, *stereoPhase = nullptr;
        juce::AudioParameterFloat *envAmount = nullptr, *envAttack = nullptr, *envRelease = nullptr, *mix = nullptr;

        double rate = 48000.0;
        double lfoPhase = 0.0;
        float envLevel = 0.0f;
        float shValueL = 0.0f, shValueR = 0.0f;
        int lastCycleL = -1, lastCycleR = -1;
        juce::uint32 rng = 0x51a7f329u;
        std::array<SvfStage, 2> stage1 {}, stage2 {};
        std::array<std::vector<float>, 2> combLines;
        std::array<int, 2> combWrite {};
        std::array<float, 2> combDamp {};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoFilterProcessor)
    };

    /** Tempo-synced rhythmic volume shaper / sidechain pump with preset curves, custom 8-step curve,
        clickless smoothing, phase offset, and optional low-band split crossover. */
    class VolumeShaperProcessor final : public BuiltInEffect
    {
    public:
        static constexpr int numSteps = 8;

        VolumeShaperProcessor();

        void prepareToPlay (double sampleRate, int maximumBlockSize) override;
        void reset() override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    private:
        float curveGainAt (double phase) const noexcept;

        juce::AudioParameterChoice *shape = nullptr, *division = nullptr;
        juce::AudioParameterBool* sync = nullptr;
        juce::AudioParameterFloat *rateHz = nullptr, *depth = nullptr, *smoothMs = nullptr;
        juce::AudioParameterFloat *phaseOffset = nullptr, *splitHz = nullptr, *mix = nullptr;
        std::array<juce::AudioParameterFloat*, numSteps> steps {};

        double rate = 48000.0;
        double freePhase = 0.0;
        float smoothedGain = 1.0f;
        std::array<float, 2> splitLp {};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VolumeShaperProcessor)
    };

    /** Live noise gate with hysteresis, hold, floor range, sidechain filtering, external key input,
        and linked multi-channel surround processing. */
    class NoiseGateProcessor final : public BuiltInEffect
    {
    public:
        NoiseGateProcessor();

        void prepareToPlay (double sampleRate, int maximumBlockSize) override;
        void reset() override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
        bool linksChannels() const override { return true; }
        bool processLinked (juce::AudioBuffer<float>&) override;

    private:
        void processInternal (juce::AudioBuffer<float>& main, const juce::AudioBuffer<float>* key);

        juce::AudioParameterFloat *thresholdDb = nullptr, *hysteresisDb = nullptr, *rangeDb = nullptr;
        juce::AudioParameterFloat *attackMs = nullptr, *holdMs = nullptr, *releaseMs = nullptr;
        juce::AudioParameterFloat *scHp = nullptr, *scLp = nullptr;
        juce::AudioParameterBool *sidechain = nullptr, *flip = nullptr;

        double rate = 48000.0;
        bool open = false;
        int holdRemaining = 0;
        float gain = 0.0f;
        float scHpState = 0.0f, scLpState = 0.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoiseGateProcessor)
    };

    /** Tempo-synced or free stereo tremolo and auto-panner (stereo phase 0 deg = tremolo, 180 deg = auto-pan). */
    class TremoloProcessor final : public BuiltInEffect
    {
    public:
        TremoloProcessor();

        void prepareToPlay (double sampleRate, int maximumBlockSize) override;
        void reset() override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    private:
        juce::AudioParameterChoice *shape = nullptr, *division = nullptr;
        juce::AudioParameterBool* sync = nullptr;
        juce::AudioParameterFloat *rateHz = nullptr, *depth = nullptr, *stereoPhase = nullptr, *smooth = nullptr;

        double rate = 48000.0;
        double phase = 0.0;
        std::array<float, 2> smoothed { 1.0f, 1.0f };
        std::array<float, 2> shVal { 0.5f, 0.5f };
        std::array<int, 2> lastCycle { -1, -1 };
        juce::uint32 rng = 0x3b71a9c5u;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TremoloProcessor)
    };

    /** Clean-room Bode single-sideband Frequency Shifter using a 12-pole all-pass Hilbert network
        (Up, Down, Stereo Split, and Ring Mod) with LFO modulation and soft-clipped feedback. */
    class FrequencyShifterProcessor final : public BuiltInEffect
    {
    public:
        FrequencyShifterProcessor();

        void prepareToPlay (double sampleRate, int maximumBlockSize) override;
        void reset() override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    private:
        struct AllPass
        {
            double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
            void clear() noexcept { x1 = x2 = y1 = y2 = 0.0; }
            double process (double x, double a2) noexcept
            {
                // H(z) = (a^2 - z^-2) / (1 - a^2 z^-2)
                const double y = a2 * (x + y2) - x2;
                x2 = x1; x1 = x;
                y2 = y1; y1 = y;
                return y;
            }
        };

        struct HilbertChannel
        {
            std::array<AllPass, 4> branchA {}, branchB {};
            double prevA = 0.0;
            void clear() noexcept
            {
                for (auto& s : branchA) s.clear();
                for (auto& s : branchB) s.clear();
                prevA = 0.0;
            }
        };

        juce::AudioParameterChoice* mode = nullptr;
        juce::AudioParameterFloat *shiftHz = nullptr, *fineHz = nullptr, *feedback = nullptr;
        juce::AudioParameterFloat *lfoRate = nullptr, *lfoDepth = nullptr, *mix = nullptr;

        double rate = 48000.0;
        double oscPhase = 0.0, lfoPhase = 0.0;
        std::array<HilbertChannel, 2> hilbert {};
        std::array<float, 2> fbState {};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyShifterProcessor)
    };

    /** Clean-room Granular / Shimmer Feedback Delay with dual Hann-windowed pitch-shifting grains,
        tempo sync, feedback damping, all-pass diffusion, and stereo spread. */
    class ShimmerDelayProcessor final : public BuiltInEffect
    {
    public:
        ShimmerDelayProcessor();

        void prepareToPlay (double sampleRate, int maximumBlockSize) override;
        void reset() override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
        double getTailLengthSeconds() const override;

    private:
        juce::AudioParameterBool* sync = nullptr;
        juce::AudioParameterChoice* division = nullptr;
        juce::AudioParameterFloat *timeMs = nullptr, *pitchSt = nullptr, *grainMs = nullptr;
        juce::AudioParameterFloat *feedback = nullptr, *dampHz = nullptr, *diffusion = nullptr;
        juce::AudioParameterFloat *spread = nullptr, *mix = nullptr;

        double rate = 48000.0;
        std::array<std::vector<float>, 2> delayLines;
        std::array<std::vector<float>, 2> grainBuf;
        int delayWrite = 0, grainWrite = 0;
        double grainPhase = 0.0;
        std::array<float, 2> lpState {};
        std::array<std::array<float, 257>, 2> diffBuf {};
        std::array<int, 2> diffPos {};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShimmerDelayProcessor)
    };
}
