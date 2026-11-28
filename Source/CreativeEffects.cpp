#include "CreativeEffects.h"

namespace BuiltInEffects
{
namespace
{
    juce::PluginDescription describeCreative (const char* name, const char* category, const char* id, int uid)
    {
        juce::PluginDescription d;
        d.name = d.descriptiveName = name;
        d.pluginFormatName = formatName;
        d.category = category;
        d.manufacturerName = "Torqwave";
        d.version = "1";
        d.fileOrIdentifier = id;
        d.uniqueId = d.deprecatedUid = uid;
        d.isInstrument = false;
        d.numInputChannels = d.numOutputChannels = 2;
        return d;
    }

    const juce::StringArray& syncDivisionNames()
    {
        static const juce::StringArray names {
            "2 bars", "1 bar", "1/2", "1/2 T", "1/4 D", "1/4", "1/4 T",
            "1/8 D", "1/8", "1/8 T", "1/16 D", "1/16", "1/16 T", "1/32"
        };
        return names;
    }

    double syncDivisionBeats (int index) noexcept
    {
        static constexpr double beats[] = {
            8.0, 4.0, 2.0, 4.0 / 3.0, 1.5, 1.0, 2.0 / 3.0,
            0.75, 0.5, 1.0 / 3.0, 0.375, 0.25, 1.0 / 6.0, 0.125
        };
        return beats[juce::jlimit (0, (int) std::size (beats) - 1, index)];
    }

    struct PlayheadTiming
    {
        double bpm = 120.0;
        double ppq = 0.0;
        bool playing = false;
        bool hasPpq = false;
    };

    PlayheadTiming readPlayhead (juce::AudioPlayHead* head) noexcept
    {
        PlayheadTiming t;
        if (head == nullptr)
            return t;

        if (const auto pos = head->getPosition())
        {
            if (const auto b = pos->getBpm(); b.hasValue() && *b > 10.0 && *b < 400.0)
                t.bpm = *b;

            t.playing = pos->getIsPlaying();
            if (const auto p = pos->getPpqPosition(); p.hasValue())
            {
                t.ppq = *p;
                t.hasPpq = true;
            }
        }
        return t;
    }

    inline float onePoleCoeff (double hz, double rate) noexcept
    {
        return (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * juce::jlimit (5.0, rate * 0.45, hz) / juce::jmax (1.0, rate)));
    }

    inline float smoothCoeffMs (double ms, double rate) noexcept
    {
        if (ms <= 0.01)
            return 1.0f;
        return (float) (1.0 - std::exp (-1.0 / (ms * 0.001 * juce::jmax (1.0, rate))));
    }

    inline float nextRand01 (juce::uint32& state) noexcept
    {
        state = state * 1664525u + 1013904223u;
        return (float) (state >> 8) * (1.0f / 16777216.0f);
    }

    inline float evalLfo (int shape, double phase, float shVal) noexcept
    {
        const auto p = phase - std::floor (phase);
        switch (shape)
        {
            case 0: return (float) std::sin (juce::MathConstants<double>::twoPi * p);            // Sine
            case 1: return (float) (1.0 - 4.0 * std::abs (p - 0.5));                              // Triangle
            case 2: return (float) (2.0 * p - 1.0);                                               // Saw Up
            case 3: return (float) (1.0 - 2.0 * p);                                               // Saw Down
            case 4: return p < 0.5 ? 1.0f : -1.0f;                                                // Square
            default: return shVal * 2.0f - 1.0f;                                                  // S&H
        }
    }

    inline double shapeDistortion (int mode, double x, double drive, double bias) noexcept
    {
        const auto y = x * drive + bias;
        switch (mode)
        {
            case 0: // Tape (warm asymmetric saturation)
                return std::tanh (y + 0.12 * y * y * (y >= 0.0 ? 1.0 : -0.5)) - std::tanh (bias);
            case 1: // Tube (soft even-harmonic curve)
            {
                const auto s = y >= 0.0 ? (1.0 - std::exp (-y)) : (-std::tanh (-y * 0.85));
                const auto b0 = bias >= 0.0 ? (1.0 - std::exp (-bias)) : (-std::tanh (-bias * 0.85));
                return s - b0;
            }
            case 2: // Soft Clip
                return (2.0 / juce::MathConstants<double>::pi) * std::atan (y * 1.5)
                     - (2.0 / juce::MathConstants<double>::pi) * std::atan (bias * 1.5);
            case 3: // Hard Clip
                return juce::jlimit (-1.0, 1.0, y) - juce::jlimit (-1.0, 1.0, bias);
            case 4: // Wavefold
            {
                const auto f = y * 0.25 + 0.25;
                return 4.0 * std::abs (f - std::floor (f + 0.5)) - 1.0;
            }
            default: // Bitcrush falls through to tanh safety ceiling after quantisation
                return std::tanh (y);
        }
    }
}

//==============================================================================
juce::PluginDescription delayDescription()        { return describeCreative ("Delay", "Delay", delayId, 0x5451444c); }
juce::PluginDescription distortionDescription()   { return describeCreative ("Distortion", "Distortion", distortionId, 0x54514453); }
juce::PluginDescription autoFilterDescription()   { return describeCreative ("Auto-Filter", "Filter", autoFilterId, 0x54514146); }
juce::PluginDescription volumeShaperDescription() { return describeCreative ("Volume Shaper", "Dynamics", volumeShaperId, 0x54515653); }
juce::PluginDescription noiseGateDescription()    { return describeCreative ("Noise Gate", "Dynamics", noiseGateId, 0x54514e47); }
juce::PluginDescription tremoloDescription()          { return describeCreative ("Tremolo & Auto-Pan", "Modulation", tremoloId, 0x54515452); }
juce::PluginDescription frequencyShifterDescription() { return describeCreative ("Frequency Shifter", "Modulation", frequencyShifterId, 0x54514653); }
juce::PluginDescription shimmerDelayDescription()     { return describeCreative ("Shimmer Delay", "Delay", shimmerDelayId, 0x54515348); }

//==============================================================================
// DelayProcessor
//==============================================================================
DelayProcessor::DelayProcessor()
    : BuiltInEffect (delayDescription(), "TORQWAVE_DELAY")
{
    mode     = addChoice ("mode", "Mode", { "Stereo", "Ping-Pong", "Tape" }, 1);
    sync     = addBool   ("sync", "Tempo Sync", true);
    division = addChoice ("division", "Division", syncDivisionNames(), 7); // 1/8 D
    timeMs   = addFloat  ("timeMs", "Time", { 1.0f, 2000.0f, 0.0f, 0.4f }, 375.0f, "ms");
    offset   = addFloat  ("offset", "Stereo Offset", { -50.0f, 50.0f }, 0.0f, "%");
    feedback = addFloat  ("feedback", "Feedback", { 0.0f, 95.0f }, 40.0f, "%");
    lowCut   = addFloat  ("lowCut", "Low Cut", { 20.0f, 2000.0f, 0.0f, 0.3f }, 120.0f, "Hz");
    highCut  = addFloat  ("highCut", "High Cut", { 500.0f, 20000.0f, 0.0f, 0.3f }, 7500.0f, "Hz");
    wow      = addFloat  ("wow", "Tape Wow", { 0.0f, 100.0f }, 15.0f, "%");
    drive    = addFloat  ("drive", "Tape Drive", { 0.0f, 100.0f }, 10.0f, "%");
    ducking  = addFloat  ("ducking", "Ducking", { 0.0f, 100.0f }, 0.0f, "%");
    width    = addFloat  ("width", "Width", { 0.0f, 200.0f }, 100.0f, "%");
    mix      = addFloat  ("mix", "Mix", { 0.0f, 100.0f }, 30.0f, "%");
}

void DelayProcessor::prepareToPlay (double sampleRate, int)
{
    rate = juce::jmax (8000.0, sampleRate);
    const auto maxSamples = (int) std::ceil (rate * 4.5) + 64;
    for (auto& l : lines)
        l.assign ((size_t) maxSamples, 0.0f);
    reset();
}

void DelayProcessor::reset()
{
    for (auto& l : lines)
        std::fill (l.begin(), l.end(), 0.0f);
    writePos = 0;
    wowPhase = 0.0;
    hpState = {};
    lpState = {};
    duckEnv = 0.0f;
    smoothDelayL = smoothDelayR = (float) (0.375 * rate);
}

double DelayProcessor::getTailLengthSeconds() const
{
    const auto fb = juce::jlimit (0.0, 0.95, (double) feedback->get() * 0.01);
    const auto baseSec = sync->get() ? (syncDivisionBeats (division->getIndex()) * 0.5) : ((double) timeMs->get() * 0.001);
    if (fb < 0.01)
        return baseSec;
    const auto repeats = std::min (24.0, std::log (0.001) / std::log (fb));
    return juce::jlimit (0.05, 16.0, baseSec * repeats);
}

void DelayProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numCh = buffer.getNumChannels();
    if (numSamples <= 0 || numCh <= 0 || lines[0].empty())
        return;

    const auto timing = readPlayhead (getPlayHead());
    const auto lineSize = (int) lines[0].size();
    const auto maxDelay = (float) (lineSize - 8);

    double baseSec = (double) timeMs->get() * 0.001;
    if (sync->get())
        baseSec = syncDivisionBeats (division->getIndex()) * (60.0 / timing.bpm);

    const auto offFrac = (double) offset->get() * 0.01;
    const auto targetDelayL = juce::jlimit (1.0f, maxDelay, (float) (baseSec * (1.0 - 0.5 * juce::jmax (0.0, offFrac)) * rate));
    const auto targetDelayR = juce::jlimit (1.0f, maxDelay, (float) (baseSec * (1.0 + 0.5 * juce::jmin (0.0, offFrac) + 0.5 * juce::jmax (0.0, offFrac)) * rate));

    const auto m = mode->getIndex();
    const auto fb = feedback->get() * 0.01f;
    const auto hpC = onePoleCoeff ((double) lowCut->get(), rate);
    const auto lpC = onePoleCoeff ((double) highCut->get(), rate);
    const auto wowAmt = (m == 2 ? 1.0f : 0.25f) * (wow->get() * 0.01f);
    const auto driveAmt = (m == 2 ? 1.0f : 0.4f) * (drive->get() * 0.01f);
    const auto duckAmt = ducking->get() * 0.01f;
    const auto widthVal = width->get() * 0.01f;
    const auto wetMix = mix->get() * 0.01f;
    const auto dryMix = 1.0f - wetMix * 0.5f; // gentle constant-feel blend (100% mix -> full wet if desired below)
    const auto actualDry = wetMix >= 0.999f ? 0.0f : 1.0f - wetMix;
    const auto delaySmooth = smoothCoeffMs (40.0, rate);
    const auto duckAtk = smoothCoeffMs (5.0, rate);
    const auto duckRel = smoothCoeffMs (160.0, rate);
    const auto wowInc = 0.65 / rate;

    auto* left = buffer.getWritePointer (0);
    auto* right = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    const auto readLine = [lineSize] (const std::vector<float>& line, int wPos, float delaySamp) noexcept -> float
    {
        auto r = (float) wPos - delaySamp;
        while (r < 0.0f) r += (float) lineSize;
        while (r >= (float) lineSize) r -= (float) lineSize;
        const auto i0 = (int) r;
        const auto i1 = (i0 + 1) % lineSize;
        const auto frac = r - (float) i0;
        return line[(size_t) i0] + frac * (line[(size_t) i1] - line[(size_t) i0]);
    };

    for (int i = 0; i < numSamples; ++i)
    {
        const auto inL = left[i];
        const auto inR = right != nullptr ? right[i] : inL;

        // Input envelope for wet ducking
        const auto inMag = juce::jmax (std::abs (inL), std::abs (inR));
        duckEnv += (inMag > duckEnv ? duckAtk : duckRel) * (inMag - duckEnv);
        const auto duckGain = duckAmt > 0.0f ? 1.0f / (1.0f + duckEnv * duckAmt * 12.0f) : 1.0f;

        smoothDelayL += delaySmooth * (targetDelayL - smoothDelayL);
        smoothDelayR += delaySmooth * (targetDelayR - smoothDelayR);

        wowPhase += wowInc;
        if (wowPhase >= 1.0) wowPhase -= 1.0;
        const auto modSamp = wowAmt > 0.0f ? (float) (std::sin (juce::MathConstants<double>::twoPi * wowPhase) * 0.003 * rate * wowAmt) : 0.0f;

        const auto dL = readLine (lines[0], writePos, juce::jlimit (1.0f, maxDelay, smoothDelayL + modSamp));
        const auto dR = readLine (lines[1], writePos, juce::jlimit (1.0f, maxDelay, smoothDelayR - modSamp * 0.7f));

        // Filter and saturate feedback
        auto filt = [&] (float x, int ch) noexcept -> float
        {
            hpState[(size_t) ch] += hpC * (x - hpState[(size_t) ch]);
            const auto hp = x - hpState[(size_t) ch];
            lpState[(size_t) ch] += lpC * (hp - lpState[(size_t) ch]);
            auto y = lpState[(size_t) ch];
            if (driveAmt > 0.001f)
                y = std::tanh (y * (1.0f + driveAmt * 3.5f)) / (1.0f + driveAmt * 1.2f);
            return juce::jlimit (-4.0f, 4.0f, y);
        };

        const auto fL = filt (dL, 0);
        const auto fR = filt (dR, 1);

        if (m == 1) // Ping-Pong
        {
            const auto monoIn = 0.5f * (inL + inR);
            lines[0][(size_t) writePos] = monoIn + fR * fb;
            lines[1][(size_t) writePos] = fL * fb;
        }
        else // Stereo or Tape
        {
            lines[0][(size_t) writePos] = inL + fL * fb;
            lines[1][(size_t) writePos] = inR + fR * fb;
        }

        writePos = (writePos + 1) % lineSize;

        // Stereo width on wet signal
        const auto mid = 0.5f * (fL + fR);
        const auto side = 0.5f * (fL - fR) * widthVal;
        const auto wetL = (mid + side) * duckGain;
        const auto wetR = (mid - side) * duckGain;

        left[i] = inL * actualDry + wetL * wetMix;
        if (right != nullptr)
            right[i] = inR * actualDry + wetR * wetMix;
    }
    juce::ignoreUnused (dryMix);
}

//==============================================================================
// DistortionProcessor
//==============================================================================
DistortionProcessor::DistortionProcessor()
    : BuiltInEffect (distortionDescription(), "TORQWAVE_DISTORTION")
{
    mode       = addChoice ("mode", "Mode", { "Tape", "Tube", "Soft Clip", "Hard Clip", "Wavefold", "Bitcrush" }, 0);
    driveDb    = addFloat  ("driveDb", "Drive", { 0.0f, 36.0f }, 12.0f, "dB");
    bias       = addFloat  ("bias", "Bias", { -100.0f, 100.0f }, 0.0f, "%");
    bits       = addFloat  ("bits", "Crush Bits", { 2.0f, 16.0f, 0.1f }, 8.0f, "bit");
    downsample = addFloat  ("downsample", "Downsample", { 1.0f, 32.0f, 0.1f }, 1.0f, "x");
    oversample = addChoice ("oversample", "Oversampling", { "Off", "4x" }, 1);
    preHp      = addFloat  ("preHp", "Pre High-Pass", { 20.0f, 2000.0f, 0.0f, 0.3f }, 20.0f, "Hz");
    postLp     = addFloat  ("postLp", "Post Low-Pass", { 500.0f, 20000.0f, 0.0f, 0.3f }, 18000.0f, "Hz");
    outputDb   = addFloat  ("outputDb", "Output", { -24.0f, 12.0f }, 0.0f, "dB");
    mix        = addFloat  ("mix", "Mix", { 0.0f, 100.0f }, 100.0f, "%");
}

void DistortionProcessor::prepareToPlay (double sampleRate, int)
{
    rate = juce::jmax (8000.0, sampleRate);
    reset();
    setLatencySamples (oversample->getIndex() == 1 ? Oversampler4::latency : 0);
}

void DistortionProcessor::reset()
{
    for (auto& o : oversamplers) o.reset();
    for (auto& d : dryDelay) d.fill (0.0f);
    dryPos = 0;
    hpState = {};
    lpState = {};
    holdSample = {};
    holdCounter = {};
    dcIn = {};
    dcOut = {};
}

void DistortionProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numCh = juce::jmin (2, buffer.getNumChannels());
    if (numSamples <= 0 || numCh <= 0)
        return;

    const auto m = mode->getIndex();
    const auto useOs = (oversample->getIndex() == 1) && (m != 5);
    setLatencySamples (useOs ? Oversampler4::latency : 0);

    const auto driveGain = (double) juce::Decibels::decibelsToGain (driveDb->get());
    const auto biasVal = (double) bias->get() * 0.005;
    const auto outGain = juce::Decibels::decibelsToGain (outputDb->get());
    const auto wetMix = mix->get() * 0.01f;
    const auto hpOn = preHp->get() > 21.0f;
    const auto lpOn = postLp->get() < 19900.0f;
    const auto hpC = onePoleCoeff ((double) preHp->get(), rate);
    const auto lpC = onePoleCoeff ((double) postLp->get(), rate);
    const auto crushSteps = std::exp2 (juce::jlimit (2.0f, 16.0f, bits->get()) - 1.0f);
    const auto dsFactor = juce::jmax (1.0f, downsample->get());
    constexpr int delayLen = Oversampler4::latency + 1;

    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < numCh; ++ch)
        {
            const auto dryIn = buffer.getSample (ch, i);
            dryDelay[(size_t) ch][(size_t) dryPos] = dryIn;
            const auto alignedDry = useOs ? dryDelay[(size_t) ch][(size_t) ((dryPos + 1) % delayLen)] : dryIn;

            auto x = dryIn;
            if (hpOn)
            {
                hpState[(size_t) ch] += hpC * (x - hpState[(size_t) ch]);
                x -= hpState[(size_t) ch];
            }

            float shaped = 0.0f;
            if (m == 5) // Bitcrush + Downsample
            {
                holdCounter[(size_t) ch] += 1.0f;
                if (holdCounter[(size_t) ch] >= dsFactor)
                {
                    holdCounter[(size_t) ch] -= dsFactor;
                    const auto driven = (float) std::tanh ((double) x * driveGain + biasVal);
                    holdSample[(size_t) ch] = std::round (driven * crushSteps) / crushSteps;
                }
                shaped = holdSample[(size_t) ch];
            }
            else if (useOs)
            {
                shaped = (float) oversamplers[(size_t) ch].tick ((double) x, [m, driveGain, biasVal] (double s) noexcept
                {
                    return shapeDistortion (m, s, driveGain, biasVal);
                });
            }
            else
            {
                shaped = (float) shapeDistortion (m, (double) x, driveGain, biasVal);
            }

            // DC blocker for asymmetric modes
            const auto dcY = shaped - dcIn[(size_t) ch] + 0.997f * dcOut[(size_t) ch];
            dcIn[(size_t) ch] = shaped;
            dcOut[(size_t) ch] = dcY;
            shaped = dcY;

            if (lpOn)
            {
                lpState[(size_t) ch] += lpC * (shaped - lpState[(size_t) ch]);
                shaped = lpState[(size_t) ch];
            }

            const auto wet = shaped * outGain;
            buffer.setSample (ch, i, alignedDry * (1.0f - wetMix) + wet * wetMix);
        }
        dryPos = (dryPos + 1) % delayLen;
    }
}

//==============================================================================
// AutoFilterProcessor
//==============================================================================
AutoFilterProcessor::AutoFilterProcessor()
    : BuiltInEffect (autoFilterDescription(), "TORQWAVE_AUTO_FILTER", true)
{
    filterType  = addChoice ("filterType", "Type", { "LP 12", "LP 24", "BP 12", "HP 12", "Notch", "Comb" }, 1);
    cutoff      = addFloat  ("cutoff", "Cutoff", { 20.0f, 20000.0f, 0.0f, 0.28f }, 1200.0f, "Hz");
    resonance   = addFloat  ("resonance", "Resonance", { 0.0f, 95.0f }, 30.0f, "%");
    drive       = addFloat  ("drive", "Drive", { 0.0f, 100.0f }, 15.0f, "%");
    sync        = addBool   ("sync", "LFO Sync", true);
    division    = addChoice ("division", "Division", syncDivisionNames(), 5); // 1/4
    lfoRate     = addFloat  ("lfoRate", "LFO Rate", { 0.05f, 20.0f, 0.0f, 0.35f }, 1.0f, "Hz");
    lfoShape    = addChoice ("lfoShape", "LFO Shape", { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H" }, 0);
    lfoDepth    = addFloat  ("lfoDepth", "LFO Depth", { -100.0f, 100.0f }, 40.0f, "%");
    stereoPhase = addFloat  ("stereoPhase", "Stereo Phase", { 0.0f, 180.0f }, 0.0f, "deg");
    envAmount   = addFloat  ("envAmount", "Env Amount", { -100.0f, 100.0f }, 0.0f, "%");
    envAttack   = addFloat  ("envAttack", "Env Attack", { 1.0f, 200.0f, 0.0f, 0.4f }, 10.0f, "ms");
    envRelease  = addFloat  ("envRelease", "Env Release", { 10.0f, 1000.0f, 0.0f, 0.4f }, 120.0f, "ms");
    sidechain   = addBool   ("sidechain", "Sidechain Key", false);
    mix         = addFloat  ("mix", "Mix", { 0.0f, 100.0f }, 100.0f, "%");
}

void AutoFilterProcessor::prepareToPlay (double sampleRate, int)
{
    rate = juce::jmax (8000.0, sampleRate);
    const auto maxComb = (int) std::ceil (rate / 20.0) + 16;
    for (auto& c : combLines)
        c.assign ((size_t) maxComb, 0.0f);
    reset();
}

void AutoFilterProcessor::reset()
{
    lfoPhase = 0.0;
    envLevel = 0.0f;
    shValueL = shValueR = 0.5f;
    lastCycleL = lastCycleR = -1;
    rng = 0x51a7f329u;
    for (auto& s : stage1) s.clear();
    for (auto& s : stage2) s.clear();
    for (auto& c : combLines) std::fill (c.begin(), c.end(), 0.0f);
    combWrite = {};
    combDamp = {};
}

void AutoFilterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    auto main = getBusBuffer (buffer, true, 0);
    const auto numSamples = main.getNumSamples();
    const auto numCh = juce::jmin (2, main.getNumChannels());
    if (numSamples <= 0 || numCh <= 0)
        return;

    const bool hasKey = sidechain->get() && getBusCount (true) > 1 && getChannelCountOfBus (true, 1) > 0;
    auto keyBuf = hasKey ? getBusBuffer (buffer, true, 1) : main;

    const auto timing = readPlayhead (getPlayHead());
    const auto fType = filterType->getIndex();
    const auto baseCut = (double) cutoff->get();
    const auto res = juce::jlimit (0.0f, 0.95f, resonance->get() * 0.01f);
    const auto drv = 1.0f + drive->get() * 0.04f;
    const auto lDepth = lfoDepth->get() * 0.01f;
    const auto eAmt = envAmount->get() * 0.01f;
    const auto sPhase = (double) stereoPhase->get() / 360.0;
    const auto wetMix = mix->get() * 0.01f;
    const auto atkC = smoothCoeffMs ((double) envAttack->get(), rate);
    const auto relC = smoothCoeffMs ((double) envRelease->get(), rate);
    const auto shapeIdx = lfoShape->getIndex();

    const double cycleHz = sync->get()
        ? (timing.bpm / (60.0 * syncDivisionBeats (division->getIndex())))
        : (double) lfoRate->get();
    const double phaseInc = cycleHz / rate;

    if (sync->get() && timing.playing && timing.hasPpq)
        lfoPhase = std::fmod (juce::jmax (0.0, timing.ppq) / syncDivisionBeats (division->getIndex()), 1.0);

    const auto runSvf = [] (SvfStage& st, float x, float g, float k, int modeIdx) noexcept -> float
    {
        const auto a1 = 1.0f / (1.0f + g * (g + k));
        const auto a2 = g * a1;
        const auto a3 = g * a2;
        const auto v3 = x - st.ic2;
        const auto v1 = a1 * st.ic1 + a2 * v3;
        const auto v2 = st.ic2 + a2 * st.ic1 + a3 * v3;
        st.ic1 = 2.0f * v1 - st.ic1;
        st.ic2 = 2.0f * v2 - st.ic2;
        switch (modeIdx)
        {
            case 0: case 1: return v2;                   // LP
            case 2:         return v1;                   // BP
            case 3:         return x - k * v1 - v2;      // HP
            default:        return x - k * v1;           // Notch
        }
    };

    for (int i = 0; i < numSamples; ++i)
    {
        // Key envelope
        float keyMag = 0.0f;
        for (int kc = 0; kc < keyBuf.getNumChannels(); ++kc)
            keyMag = juce::jmax (keyMag, std::abs (keyBuf.getSample (kc, i)));
        envLevel += (keyMag > envLevel ? atkC : relC) * (keyMag - envLevel);

        const auto pL = lfoPhase;
        const auto pR = lfoPhase + sPhase;
        const auto cycL = (int) std::floor (pL);
        const auto cycR = (int) std::floor (pR);
        if (cycL != lastCycleL) { lastCycleL = cycL; shValueL = nextRand01 (rng); }
        if (cycR != lastCycleR) { lastCycleR = cycR; shValueR = nextRand01 (rng); }

        const auto modL = evalLfo (shapeIdx, pL, shValueL) * lDepth * 3.5f + envLevel * eAmt * 5.0f;
        const auto modR = evalLfo (shapeIdx, pR, shValueR) * lDepth * 3.5f + envLevel * eAmt * 5.0f;
        lfoPhase += phaseInc;

        for (int ch = 0; ch < numCh; ++ch)
        {
            const auto dry = main.getSample (ch, i);
            const auto oct = ch == 0 ? modL : modR;
            const auto hz = juce::jlimit (20.0, rate * 0.44, baseCut * std::exp2 ((double) oct));
            auto x = drv > 1.001f ? std::tanh (dry * drv) / std::tanh (drv) : dry;
            float y = x;

            if (fType == 5) // Comb
            {
                auto& line = combLines[(size_t) ch];
                const auto n = (int) line.size();
                if (n >= 4)
                {
                    const auto period = juce::jlimit (2.0f, (float) (n - 3), (float) (rate / hz));
                    auto r = (float) combWrite[(size_t) ch] - period;
                    if (r < 0.0f) r += (float) n;
                    const auto i0 = (int) r;
                    const auto frac = r - (float) i0;
                    const auto delayed = line[(size_t) i0] + frac * (line[(size_t) ((i0 + 1) % n)] - line[(size_t) i0]);
                    combDamp[(size_t) ch] += 0.5f * (delayed - combDamp[(size_t) ch]);
                    const auto fb = res * 0.92f;
                    y = x + fb * combDamp[(size_t) ch];
                    line[(size_t) combWrite[(size_t) ch]] = juce::jlimit (-4.0f, 4.0f, y);
                    combWrite[(size_t) ch] = (combWrite[(size_t) ch] + 1) % n;
                    y *= (1.0f - 0.45f * fb);
                }
            }
            else
            {
                const auto g = (float) std::tan (juce::MathConstants<double>::pi * hz / rate);
                const auto k = 2.0f - 1.92f * res;
                y = runSvf (stage1[(size_t) ch], x, g, k, fType);
                if (fType == 1) // 24 dB = 2 cascaded LP stages
                    y = runSvf (stage2[(size_t) ch], y, g, juce::jmax (0.35f, k * 1.15f), 0);
            }

            main.setSample (ch, i, dry * (1.0f - wetMix) + y * wetMix);
        }
    }
}

//==============================================================================
// VolumeShaperProcessor
//==============================================================================
VolumeShaperProcessor::VolumeShaperProcessor()
    : BuiltInEffect (volumeShaperDescription(), "TORQWAVE_VOLUME_SHAPER")
{
    shape       = addChoice ("shape", "Curve", { "Club Pump", "Tight Duck", "Deep Duck", "Trance Gate", "Half-Time", "Saw Up", "Saw Down", "Custom" }, 0);
    sync        = addBool   ("sync", "Tempo Sync", true);
    division    = addChoice ("division", "Division", syncDivisionNames(), 5); // 1/4
    rateHz      = addFloat  ("rateHz", "Rate", { 0.1f, 20.0f, 0.0f, 0.4f }, 2.0f, "Hz");
    depth       = addFloat  ("depth", "Depth", { 0.0f, 100.0f }, 80.0f, "%");
    smoothMs    = addFloat  ("smoothMs", "Smooth", { 0.5f, 30.0f, 0.0f, 0.5f }, 4.0f, "ms");
    phaseOffset = addFloat  ("phaseOffset", "Phase", { 0.0f, 100.0f }, 0.0f, "%");
    splitHz     = addFloat  ("splitHz", "Low Band Split", { 20.0f, 2000.0f, 0.0f, 0.3f }, 20.0f, "Hz");
    mix         = addFloat  ("mix", "Mix", { 0.0f, 100.0f }, 100.0f, "%");

    static constexpr float defaultSteps[numSteps] = { 0.0f, 35.0f, 80.0f, 100.0f, 100.0f, 95.0f, 85.0f, 60.0f };
    for (int s = 0; s < numSteps; ++s)
        steps[(size_t) s] = addFloat ("step" + juce::String (s + 1), "Step " + juce::String (s + 1), { 0.0f, 100.0f }, defaultSteps[s], "%");
}

void VolumeShaperProcessor::prepareToPlay (double sampleRate, int)
{
    rate = juce::jmax (8000.0, sampleRate);
    reset();
}

void VolumeShaperProcessor::reset()
{
    freePhase = 0.0;
    smoothedGain = 1.0f;
    splitLp = {};
}

float VolumeShaperProcessor::curveGainAt (double phase) const noexcept
{
    const auto p = phase - std::floor (phase);
    switch (shape->getIndex())
    {
        case 0: // Club Pump (smooth quarter-note rise after kick transient)
            return p < 0.08 ? 0.0f : (float) std::pow (juce::jlimit (0.0, 1.0, (p - 0.08) / 0.55), 0.7);
        case 1: // Tight Duck (quick 25% dip, fast recovery)
            return p < 0.04 ? 0.0f : (float) juce::jlimit (0.0, 1.0, (p - 0.04) / 0.28);
        case 2: // Deep Duck (long scoop, peaks near end of beat)
            return p < 0.15 ? 0.0f : (float) std::pow ((p - 0.15) / 0.85, 1.3);
        case 3: // Trance Gate (16th-note rhythmic pattern across cycle)
        {
            const auto idx = juce::jlimit (0, 7, (int) (p * 8.0));
            static constexpr float gatePat[8] = { 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f };
            return gatePat[idx];
        }
        case 4: // Half-Time (off first half, smooth swell second half)
            return p < 0.45 ? 0.0f : (float) juce::jlimit (0.0, 1.0, (p - 0.45) / 0.35);
        case 5: // Saw Up
            return (float) p;
        case 6: // Saw Down
            return (float) (1.0 - p);
        default: // Custom 8-step piecewise linear
        {
            const auto pos = p * numSteps;
            const auto i0 = juce::jlimit (0, numSteps - 1, (int) std::floor (pos));
            const auto i1 = (i0 + 1) % numSteps;
            const auto f = (float) (pos - std::floor (pos));
            const auto v0 = steps[(size_t) i0]->get() * 0.01f;
            const auto v1 = steps[(size_t) i1]->get() * 0.01f;
            return v0 + f * (v1 - v0);
        }
    }
}

void VolumeShaperProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numCh = juce::jmin (2, buffer.getNumChannels());
    if (numSamples <= 0 || numCh <= 0)
        return;

    const auto timing = readPlayhead (getPlayHead());
    const auto d = depth->get() * 0.01f;
    const auto wetMix = mix->get() * 0.01f;
    const auto smC = smoothCoeffMs ((double) smoothMs->get(), rate);
    const auto pOff = (double) phaseOffset->get() * 0.01;
    const auto useSplit = splitHz->get() > 21.0f;
    const auto splitC = onePoleCoeff ((double) splitHz->get(), rate);

    const double cycleHz = sync->get()
        ? (timing.bpm / (60.0 * syncDivisionBeats (division->getIndex())))
        : (double) rateHz->get();
    const double phaseInc = cycleHz / rate;

    double ph = freePhase;
    if (sync->get() && timing.playing && timing.hasPpq)
        ph = std::fmod (juce::jmax (0.0, timing.ppq) / syncDivisionBeats (division->getIndex()), 1.0);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto targetG = 1.0f - d * (1.0f - curveGainAt (ph + pOff));
        smoothedGain += smC * (targetG - smoothedGain);
        const auto effG = (1.0f - wetMix) + wetMix * smoothedGain;
        ph += phaseInc;

        for (int ch = 0; ch < numCh; ++ch)
        {
            const auto x = buffer.getSample (ch, i);
            if (useSplit)
            {
                splitLp[(size_t) ch] += splitC * (x - splitLp[(size_t) ch]);
                const auto low = splitLp[(size_t) ch];
                const auto high = x - low;
                buffer.setSample (ch, i, low * effG + high);
            }
            else
            {
                buffer.setSample (ch, i, x * effG);
            }
        }
    }

    freePhase = ph - std::floor (ph);
}

//==============================================================================
// NoiseGateProcessor
//==============================================================================
NoiseGateProcessor::NoiseGateProcessor()
    : BuiltInEffect (noiseGateDescription(), "TORQWAVE_NOISE_GATE", true)
{
    thresholdDb  = addFloat ("thresholdDb", "Threshold", { -80.0f, 0.0f }, -36.0f, "dB");
    hysteresisDb = addFloat ("hysteresisDb", "Hysteresis", { 0.0f, 24.0f }, 4.0f, "dB");
    rangeDb      = addFloat ("rangeDb", "Floor", { -80.0f, 0.0f }, -60.0f, "dB");
    attackMs     = addFloat ("attackMs", "Attack", { 0.1f, 200.0f, 0.0f, 0.4f }, 2.0f, "ms");
    holdMs       = addFloat ("holdMs", "Hold", { 0.0f, 1000.0f, 0.0f, 0.4f }, 50.0f, "ms");
    releaseMs    = addFloat ("releaseMs", "Release", { 5.0f, 3000.0f, 0.0f, 0.4f }, 150.0f, "ms");
    scHp         = addFloat ("scHp", "Key High-Pass", { 20.0f, 4000.0f, 0.0f, 0.3f }, 20.0f, "Hz");
    scLp         = addFloat ("scLp", "Key Low-Pass", { 500.0f, 20000.0f, 0.0f, 0.3f }, 20000.0f, "Hz");
    sidechain    = addBool  ("sidechain", "Sidechain Key", false);
    flip         = addBool  ("flip", "Invert (Duck)", false);
}

void NoiseGateProcessor::prepareToPlay (double sampleRate, int)
{
    rate = juce::jmax (8000.0, sampleRate);
    reset();
}

void NoiseGateProcessor::reset()
{
    open = false;
    holdRemaining = 0;
    gain = 0.0f;
    scHpState = scLpState = 0.0f;
}

void NoiseGateProcessor::processInternal (juce::AudioBuffer<float>& main, const juce::AudioBuffer<float>* key)
{
    const auto numSamples = main.getNumSamples();
    const auto numCh = main.getNumChannels();
    if (numSamples <= 0 || numCh <= 0)
        return;

    const auto& det = key != nullptr && key->getNumChannels() > 0 ? *key : main;
    const auto detCh = det.getNumChannels();

    const auto openThresh = juce::Decibels::decibelsToGain (thresholdDb->get());
    const auto closeThresh = juce::Decibels::decibelsToGain (thresholdDb->get() - hysteresisDb->get());
    const auto floorGain = rangeDb->get() <= -79.5f ? 0.0f : juce::Decibels::decibelsToGain (rangeDb->get());
    const auto atkC = smoothCoeffMs ((double) attackMs->get(), rate);
    const auto relC = smoothCoeffMs ((double) releaseMs->get(), rate);
    const auto holdSamp = juce::roundToInt (holdMs->get() * 0.001 * rate);
    const auto useHp = scHp->get() > 21.0f;
    const auto useLp = scLp->get() < 19900.0f;
    const auto hpC = onePoleCoeff ((double) scHp->get(), rate);
    const auto lpC = onePoleCoeff ((double) scLp->get(), rate);
    const auto inverted = flip->get();

    for (int i = 0; i < numSamples; ++i)
    {
        float monoKey = 0.0f;
        for (int c = 0; c < detCh; ++c)
            monoKey += det.getSample (c, i);
        monoKey /= (float) detCh;

        if (useHp)
        {
            scHpState += hpC * (monoKey - scHpState);
            monoKey -= scHpState;
        }
        if (useLp)
        {
            scLpState += lpC * (monoKey - scLpState);
            monoKey = scLpState;
        }

        const auto level = std::abs (monoKey);
        if (open)
        {
            if (level >= closeThresh)
                holdRemaining = holdSamp;
            else if (holdRemaining > 0)
                --holdRemaining;
            else
                open = false;
        }
        else if (level >= openThresh)
        {
            open = true;
            holdRemaining = holdSamp;
        }

        const auto target = open ? 1.0f : 0.0f;
        gain += (target > gain ? atkC : relC) * (target - gain);
        const auto g = inverted ? (1.0f - gain * (1.0f - floorGain)) : (floorGain + gain * (1.0f - floorGain));

        for (int c = 0; c < numCh; ++c)
            main.setSample (c, i, main.getSample (c, i) * g);
    }
}

void NoiseGateProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    auto main = getBusBuffer (buffer, true, 0);
    const bool hasKey = sidechain->get() && getBusCount (true) > 1 && getChannelCountOfBus (true, 1) > 0;
    if (hasKey)
    {
        auto key = getBusBuffer (buffer, true, 1);
        processInternal (main, &key);
    }
    else
    {
        processInternal (main, nullptr);
    }
}

bool NoiseGateProcessor::processLinked (juce::AudioBuffer<float>& buffer)
{
    processInternal (buffer, nullptr);
    return true;
}

//==============================================================================
// TremoloProcessor
//==============================================================================
TremoloProcessor::TremoloProcessor()
    : BuiltInEffect (tremoloDescription(), "TORQWAVE_TREMOLO")
{
    shape       = addChoice ("shape", "Shape", { "Sine", "Triangle", "Square", "Saw Up", "Saw Down", "Random" }, 0);
    sync        = addBool   ("sync", "Tempo Sync", true);
    division    = addChoice ("division", "Division", syncDivisionNames(), 8); // 1/8
    rateHz      = addFloat  ("rateHz", "Rate", { 0.05f, 30.0f, 0.0f, 0.35f }, 4.0f, "Hz");
    depth       = addFloat  ("depth", "Depth", { 0.0f, 100.0f }, 60.0f, "%");
    stereoPhase = addFloat  ("stereoPhase", "Stereo Phase", { 0.0f, 180.0f }, 180.0f, "deg");
    smooth      = addFloat  ("smooth", "Smooth", { 0.0f, 100.0f }, 15.0f, "%");
}

void TremoloProcessor::prepareToPlay (double sampleRate, int)
{
    rate = juce::jmax (8000.0, sampleRate);
    reset();
}

void TremoloProcessor::reset()
{
    phase = 0.0;
    smoothed = { 1.0f, 1.0f };
    shVal = { 0.5f, 0.5f };
    lastCycle = { -1, -1 };
    rng = 0x3b71a9c5u;
}

void TremoloProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numCh = juce::jmin (2, buffer.getNumChannels());
    if (numSamples <= 0 || numCh <= 0)
        return;

    const auto timing = readPlayhead (getPlayHead());
    const auto d = depth->get() * 0.01f;
    const auto sOff = (double) stereoPhase->get() / 360.0;
    const auto smC = smoothCoeffMs (0.5 + (double) smooth->get() * 0.25, rate);
    const auto shIdx = shape->getIndex();

    const double cycleHz = sync->get()
        ? (timing.bpm / (60.0 * syncDivisionBeats (division->getIndex())))
        : (double) rateHz->get();
    const double phaseInc = cycleHz / rate;

    if (sync->get() && timing.playing && timing.hasPpq)
        phase = std::fmod (juce::jmax (0.0, timing.ppq) / syncDivisionBeats (division->getIndex()), 1.0);

    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < numCh; ++ch)
        {
            const auto p = phase + (ch == 1 ? sOff : 0.0);
            const auto cyc = (int) std::floor (p);
            if (cyc != lastCycle[(size_t) ch])
            {
                lastCycle[(size_t) ch] = cyc;
                shVal[(size_t) ch] = nextRand01 (rng);
            }

            // Map LFO [-1, 1] -> [0, 1] gain modulation
            const auto lfo01 = 0.5f * (evalLfo (shIdx == 2 ? 4 : shIdx == 3 ? 2 : shIdx == 4 ? 3 : shIdx, p, shVal[(size_t) ch]) + 1.0f);
            const auto targetG = 1.0f - d * (1.0f - lfo01);
            smoothed[(size_t) ch] += smC * (targetG - smoothed[(size_t) ch]);
            buffer.setSample (ch, i, buffer.getSample (ch, i) * smoothed[(size_t) ch]);
        }
        phase += phaseInc;
    }

    phase -= std::floor (phase);
}

//==============================================================================
// FrequencyShifterProcessor
//==============================================================================
FrequencyShifterProcessor::FrequencyShifterProcessor()
    : BuiltInEffect (frequencyShifterDescription(), "TORQWAVE_FREQ_SHIFTER")
{
    mode     = addChoice ("mode", "Mode", { "Up", "Down", "Stereo Split", "Ring Mod" }, 0);
    shiftHz  = addFloat  ("shiftHz", "Shift", { -1000.0f, 1000.0f, 0.0f, 1.0f }, 40.0f, "Hz");
    fineHz   = addFloat  ("fineHz", "Fine", { -20.0f, 20.0f }, 0.0f, "Hz");
    feedback = addFloat  ("feedback", "Feedback", { 0.0f, 85.0f }, 0.0f, "%");
    lfoRate  = addFloat  ("lfoRate", "LFO Rate", { 0.05f, 20.0f, 0.0f, 0.35f }, 0.5f, "Hz");
    lfoDepth = addFloat  ("lfoDepth", "LFO Depth", { 0.0f, 200.0f, 0.0f, 0.5f }, 0.0f, "Hz");
    mix      = addFloat  ("mix", "Mix", { 0.0f, 100.0f }, 100.0f, "%");
}

void FrequencyShifterProcessor::prepareToPlay (double sampleRate, int)
{
    rate = juce::jmax (8000.0, sampleRate);
    reset();
}

void FrequencyShifterProcessor::reset()
{
    oscPhase = 0.0;
    lfoPhase = 0.0;
    for (auto& h : hilbert) h.clear();
    fbState = {};
}

void FrequencyShifterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numCh = juce::jmin (2, buffer.getNumChannels());
    if (numSamples <= 0 || numCh <= 0)
        return;

    // Classic Olli Niemitalo 8th-order all-pass Hilbert transformer (90-deg quadrature pair within +/-0.7 deg, 30 Hz - 20 kHz)
    static constexpr double coeffA2[4] {
        0.6923878 * 0.6923878,
        0.9360654322959 * 0.9360654322959,
        0.9882295226860 * 0.9882295226860,
        0.9987488452737 * 0.9987488452737
    };
    static constexpr double coeffB2[4] {
        0.4021921162426 * 0.4021921162426,
        0.8561710882420 * 0.8561710882420,
        0.9722909545651 * 0.9722909545651,
        0.9952884791278 * 0.9952884791278
    };

    const int modeIdx = mode->getIndex();
    const double baseShift = (double) shiftHz->get() + (double) fineHz->get();
    const float fb = feedback->get() * 0.01f;
    const double lfoInc = (double) lfoRate->get() / rate;
    const double lfoD = (double) lfoDepth->get();
    const float wet = mix->get() * 0.01f;
    const float dry = 1.0f - wet;
    constexpr double twoPi = juce::MathConstants<double>::twoPi;

    for (int i = 0; i < numSamples; ++i)
    {
        const double curShift = baseShift + lfoD * std::sin (twoPi * lfoPhase);
        lfoPhase += lfoInc;
        if (lfoPhase >= 1.0) lfoPhase -= 1.0;

        oscPhase += curShift / rate;
        oscPhase -= std::floor (oscPhase);
        const double cs = std::cos (twoPi * oscPhase);
        const double sn = std::sin (twoPi * oscPhase);

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float in = buffer.getSample (ch, i);
            const double driven = (double) in + (double) fb * std::tanh ((double) fbState[(size_t) ch]);

            auto& h = hilbert[(size_t) ch];
            double a = driven;
            double b = driven;

            for (int st = 0; st < 4; ++st)
            {
                a = h.branchA[(size_t) st].process (a, coeffA2[st]);
                b = h.branchB[(size_t) st].process (b, coeffB2[st]);
            }

            // 1-sample delay aligns branch A at -90 deg relative to branch B (I = b, Q = prevA)
            const double q = h.prevA;
            h.prevA = a;

            double shifted = 0.0;
            if (modeIdx == 0)      shifted = b * cs - q * sn;                                     // Up
            else if (modeIdx == 1) shifted = b * cs + q * sn;                                     // Down
            else if (modeIdx == 2) shifted = (ch == 0) ? (b * cs - q * sn) : (b * cs + q * sn);   // Stereo Split
            else                   shifted = driven * cs;                                         // Ring Mod

            fbState[(size_t) ch] = (float) shifted;
            buffer.setSample (ch, i, dry * in + wet * (float) shifted);
        }
    }
}

//==============================================================================
// ShimmerDelayProcessor
//==============================================================================
ShimmerDelayProcessor::ShimmerDelayProcessor()
    : BuiltInEffect (shimmerDelayDescription(), "TORQWAVE_SHIMMER_DELAY")
{
    sync      = addBool   ("sync", "Tempo Sync", true);
    division  = addChoice ("division", "Division", syncDivisionNames(), 9); // 1/4
    timeMs    = addFloat  ("timeMs", "Time", { 20.0f, 2000.0f, 0.0f, 0.4f }, 380.0f, "ms");
    pitchSt   = addFloat  ("pitchSt", "Pitch", { -24.0f, 24.0f, 1.0f }, 12.0f, "st");
    grainMs   = addFloat  ("grainMs", "Grain Size", { 25.0f, 180.0f }, 75.0f, "ms");
    feedback  = addFloat  ("feedback", "Feedback", { 0.0f, 92.0f }, 55.0f, "%");
    dampHz    = addFloat  ("dampHz", "Damp", { 600.0f, 18000.0f, 0.0f, 0.3f }, 7500.0f, "Hz");
    diffusion = addFloat  ("diffusion", "Diffusion", { 0.0f, 100.0f }, 45.0f, "%");
    spread    = addFloat  ("spread", "Stereo Spread", { 0.0f, 100.0f }, 25.0f, "%");
    mix       = addFloat  ("mix", "Mix", { 0.0f, 100.0f }, 40.0f, "%");
}

void ShimmerDelayProcessor::prepareToPlay (double sampleRate, int)
{
    rate = juce::jmax (8000.0, sampleRate);
    const size_t maxDelay = (size_t) std::ceil (rate * 2.5) + 16;
    const size_t maxGrain = (size_t) std::ceil (rate * 0.25) + 16;

    for (auto& dl : delayLines) dl.assign (maxDelay, 0.0f);
    for (auto& gb : grainBuf)   gb.assign (maxGrain, 0.0f);

    reset();
}

void ShimmerDelayProcessor::reset()
{
    for (auto& dl : delayLines) std::fill (dl.begin(), dl.end(), 0.0f);
    for (auto& gb : grainBuf)   std::fill (gb.begin(), gb.end(), 0.0f);
    delayWrite = 0;
    grainWrite = 0;
    grainPhase = 0.0;
    lpState = {};
    for (auto& db : diffBuf) db.fill (0.0f);
    diffPos = {};
}

double ShimmerDelayProcessor::getTailLengthSeconds() const
{
    return 3.0;
}

void ShimmerDelayProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numCh = juce::jmin (2, buffer.getNumChannels());
    if (numSamples <= 0 || numCh <= 0 || delayLines[0].empty() || grainBuf[0].empty())
        return;

    const auto timing = readPlayhead (getPlayHead());
    const int dLen = (int) delayLines[0].size();
    const int gLen = (int) grainBuf[0].size();

    const double baseDelaySec = sync->get()
        ? (syncDivisionBeats (division->getIndex()) * 60.0 / timing.bpm)
        : ((double) timeMs->get() * 0.001);
    const double baseSamples = juce::jlimit (32.0, (double) (dLen - 4), baseDelaySec * rate);
    const double spreadOffset = (double) spread->get() * 0.0015 * baseSamples;

    const double ratio = std::pow (2.0, (double) pitchSt->get() / 12.0);
    const double windowSamples = juce::jlimit (256.0, (double) (gLen - 4), (double) grainMs->get() * 0.001 * rate);
    const double phaseInc = (1.0 - ratio) / windowSamples;

    const float fb = feedback->get() * 0.01f;
    const float dampC = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * dampHz->get() / (float) rate);
    const float diffG = diffusion->get() * 0.0065f;
    const float wet = mix->get() * 0.01f;
    const float dry = 1.0f - wet;

    auto readInterp = [] (const std::vector<float>& line, int wPos, double delay) -> float
    {
        const int len = (int) line.size();
        double r = (double) wPos - delay;
        while (r < 0.0) r += (double) len;
        while (r >= (double) len) r -= (double) len;
        const int i0 = (int) r;
        const int i1 = (i0 + 1) % len;
        const float frac = (float) (r - (double) i0);
        return line[(size_t) i0] + frac * (line[(size_t) i1] - line[(size_t) i0]);
    };

    for (int i = 0; i < numSamples; ++i)
    {
        // Two 180-deg out-of-phase Hann grains for clickless pitch shifting
        const double p0 = grainPhase;
        double p1 = grainPhase + 0.5;
        if (p1 >= 1.0) p1 -= 1.0;

        const float w0 = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) p0);
        const float w1 = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) p1);
        const double d0 = p0 * windowSamples;
        const double d1 = p1 * windowSamples;

        grainPhase += phaseInc;
        grainPhase -= std::floor (grainPhase);

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float in = buffer.getSample (ch, i);
            const double chDelay = juce::jlimit (32.0, (double) (dLen - 4), baseSamples + (ch == 1 ? spreadOffset : -spreadOffset));
            const float delayed = readInterp (delayLines[(size_t) ch], delayWrite, chDelay);

            // Feed delayed tap into granular pitch shifter
            grainBuf[(size_t) ch][(size_t) grainWrite] = delayed;
            const float pitched = w0 * readInterp (grainBuf[(size_t) ch], grainWrite, d0)
                                + w1 * readInterp (grainBuf[(size_t) ch], grainWrite, d1);

            // All-pass diffusion smear
            const int apLen = ch == 0 ? 193 : 241;
            auto& ap = diffBuf[(size_t) ch];
            int& apIdx = diffPos[(size_t) ch];
            const float apDelayed = ap[(size_t) apIdx];
            const float diffused = -diffG * pitched + apDelayed;
            ap[(size_t) apIdx] = pitched + diffG * diffused;
            apIdx = (apIdx + 1) % apLen;

            // Low-pass feedback damping + soft saturation guard
            lpState[(size_t) ch] += dampC * (diffused - lpState[(size_t) ch]);
            const float fbSig = std::tanh (lpState[(size_t) ch] * fb);

            delayLines[(size_t) ch][(size_t) delayWrite] = in + fbSig;
            buffer.setSample (ch, i, dry * in + wet * diffused);
        }

        delayWrite = (delayWrite + 1) % dLen;
        grainWrite = (grainWrite + 1) % gLen;
    }
}
}
