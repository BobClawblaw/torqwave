#pragma once

#include "BuiltInEffects.h"
#include "Oversampling.h"

namespace BuiltInEffects
{
    constexpr const char* delayId        = "torqwave:delay";
    constexpr const char* distortionId   = "torqwave:distortion";
    constexpr const char* autoFilterId   = "torqwave:auto-filter";
    constexpr const char* volumeShaperId = "torqwave:volume-shaper";
    constexpr const char* noiseGateId    = "torqwave:noise-gate";
    constexpr const char* tremoloId      = "torqwave:tremolo";

    juce::PluginDescription delayDescription();
    juce::PluginDescription distortionDescription();
    juce::PluginDescription autoFilterDescription();
    juce::PluginDescription volumeShaperDescription();
    juce::PluginDescription noiseGateDescription();
    juce::PluginDescription tremoloDescription();

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
}
